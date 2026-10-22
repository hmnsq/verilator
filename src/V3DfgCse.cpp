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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
श्रेणी DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

खुला:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        क्रम (स्थिर DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        क्रम (स्थिर DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        क्रम (स्थिर DfgVertex& vtx : dfg.opVertices()) {
            अगर (vtx.is<DfgCReset>() || vtx.is<Dfg_पूर्व>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        क्रम (स्थिर DfgConst& vtx : dfg.constVertices()) {
            स्थिर V3Hash hash = vtx.अंक().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.मूल्य()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    माप_प्रकार चालक()(DfgVertex* vtxp) स्थिर { वापस vertexHash(*vtxp).मूल्य(); }

निजी:
    // Returns hash of vertex dependent on information internal to the vertex
    अटल V3Hash vertexSelfHash(स्थिर DfgVertex& vtx) {
        चयन (vtx.type()) {
        // Unhandled vertices
        हाल VDfgType::Logic:  // LCOV_EXCL_START
        हाल VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        हाल VDfgType::Const:  // LCOV_EXCL_START
        हाल VDfgType::CReset:
        हाल VDfgType::VarArray:
        हाल VDfgType::VarPacked:
        हाल VDfgType::पूर्व:
        हाल VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        हाल VDfgType::Sel: वापस V3Hash{vtx.as<DfgSel>()->lsb()};

        हाल VDfgType::SpliceArray:
        हाल VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](स्थिर DfgVertex&, uint32_t lo) {
                hash += lo;
                वापस false;
            });
            वापस hash;
        }

        // Vertices with no internal information
        हाल VDfgType::MatchMasked:
        हाल VDfgType::Mux:
        हाल VDfgType::UnitArray: वापस V3Hash{};

        // Generated classes - none of them have internal information
        हाल VDfgType::Add:
        हाल VDfgType::And:
        हाल VDfgType::ArraySel:
        हाल VDfgType::Concat:
        हाल VDfgType::Cond:
        हाल VDfgType::गिनती_Ones_2:
        हाल VDfgType::Div:
        हाल VDfgType::DivS:
        हाल VDfgType::Eq:
        हाल VDfgType::EqCase:
        हाल VDfgType::EqWild:
        हाल VDfgType::Extend:
        हाल VDfgType::ExtendS:
        हाल VDfgType::Gt:
        हाल VDfgType::GtS:
        हाल VDfgType::Gte:
        हाल VDfgType::GteS:
        हाल VDfgType::LogAnd:
        हाल VDfgType::LogEq:
        हाल VDfgType::LogIf:
        हाल VDfgType::LogNot:
        हाल VDfgType::LogOr:
        हाल VDfgType::Lt:
        हाल VDfgType::LtS:
        हाल VDfgType::Lte:
        हाल VDfgType::LteS:
        हाल VDfgType::ModDiv:
        हाल VDfgType::ModDivS:
        हाल VDfgType::Mul:
        हाल VDfgType::MulS:
        हाल VDfgType::Negate:
        हाल VDfgType::Neq:
        हाल VDfgType::NeqCase:
        हाल VDfgType::NeqWild:
        हाल VDfgType::Not:
        हाल VDfgType::OneHot:
        हाल VDfgType::OneHot0:
        हाल VDfgType::Or:
        हाल VDfgType::Pow:
        हाल VDfgType::PowSS:
        हाल VDfgType::PowSU:
        हाल VDfgType::PowUS:
        हाल VDfgType::RedAnd:
        हाल VDfgType::RedOr:
        हाल VDfgType::RedXor:
        हाल VDfgType::Rep:
        हाल VDfgType::ShiftL:
        हाल VDfgType::ShiftR:
        हाल VDfgType::ShiftRS:
        हाल VDfgType::StreamL:
        हाल VDfgType::StreamR:
        हाल VDfgType::Sub:
        हाल VDfgType::Xor: वापस V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) स्थिर {
        V3Hash& परिणाम = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        अगर (!परिणाम.मूल्य()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.आकार();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                वापस false;
            });
            परिणाम = hash;
        }
        वापस परिणाम;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
श्रेणी DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<स्थिर DfgVertex*, स्थिर DfgVertex*>;
    काष्ठा VertexPairHash final {
        माप_प्रकार चालक()(स्थिर VertexPair& pair) स्थिर {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            वापस hash.मूल्य();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, bool, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    स्थिर माप_प्रकार m_आकार;  // Size of the graph

खुला:
    // CONSTRUCTORS
    explicit DfgCseEqual(स्थिर DfgGraph& dfg)
        : m_आकार{dfg.आकार()} {}

    // METHODS
    bool चालक()(DfgVertex* ap, DfgVertex* bp) स्थिर { वापस vertexEquivalent(*ap, *bp); }

निजी:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    bool vertexSelfEquivalent(स्थिर DfgVertex& a, स्थिर DfgVertex& b) स्थिर {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        चयन (a.type()) {
        // Unhandled vertices
        हाल VDfgType::Logic:  // LCOV_EXCL_START
        हाल VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        हाल VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        हाल VDfgType::Const: वापस a.as<DfgConst>()->अंक().isCaseEq(b.as<DfgConst>()->अंक());
        हाल VDfgType::CReset: वापस false;
        हाल VDfgType::पूर्व: वापस false;

        हाल VDfgType::VarArray:
        हाल VDfgType::VarPacked:  // CSE does not combine variables
            वापस false;

        // Vertices with internal information
        हाल VDfgType::Sel: वापस a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        हाल VDfgType::SpliceArray:
        हाल VDfgType::SplicePacked: {
            स्थिर DfgVertexSplice* स्थिर ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.सफा();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](स्थिर DfgVertex&, uint32_t lo) {
                m_driverLo.धकेलो_back(lo);
                वापस false;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.तथ्य();
            वापस !b.as<DfgVertexSplice>()->foreachDriver([&](स्थिर DfgVertex&, uint32_t lo) {  //
                वापस *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        हाल VDfgType::MatchMasked:
        हाल VDfgType::Mux:
        हाल VDfgType::UnitArray: वापस true;

        // Generated classes - none of them have internal information
        हाल VDfgType::Add:
        हाल VDfgType::And:
        हाल VDfgType::ArraySel:
        हाल VDfgType::Concat:
        हाल VDfgType::Cond:
        हाल VDfgType::गिनती_Ones_2:
        हाल VDfgType::Div:
        हाल VDfgType::DivS:
        हाल VDfgType::Eq:
        हाल VDfgType::EqCase:
        हाल VDfgType::EqWild:
        हाल VDfgType::Extend:
        हाल VDfgType::ExtendS:
        हाल VDfgType::Gt:
        हाल VDfgType::GtS:
        हाल VDfgType::Gte:
        हाल VDfgType::GteS:
        हाल VDfgType::LogAnd:
        हाल VDfgType::LogEq:
        हाल VDfgType::LogIf:
        हाल VDfgType::LogNot:
        हाल VDfgType::LogOr:
        हाल VDfgType::Lt:
        हाल VDfgType::LtS:
        हाल VDfgType::Lte:
        हाल VDfgType::LteS:
        हाल VDfgType::ModDiv:
        हाल VDfgType::ModDivS:
        हाल VDfgType::Mul:
        हाल VDfgType::MulS:
        हाल VDfgType::Negate:
        हाल VDfgType::Neq:
        हाल VDfgType::NeqCase:
        हाल VDfgType::NeqWild:
        हाल VDfgType::Not:
        हाल VDfgType::OneHot:
        हाल VDfgType::OneHot0:
        हाल VDfgType::Or:
        हाल VDfgType::Pow:
        हाल VDfgType::PowSS:
        हाल VDfgType::PowSU:
        हाल VDfgType::PowUS:
        हाल VDfgType::RedAnd:
        हाल VDfgType::RedOr:
        हाल VDfgType::RedXor:
        हाल VDfgType::Rep:
        हाल VDfgType::ShiftL:
        हाल VDfgType::ShiftR:
        हाल VDfgType::ShiftRS:
        हाल VDfgType::StreamL:
        हाल VDfgType::StreamR:
        हाल VDfgType::Sub:
        हाल VDfgType::Xor: वापस true;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    bool sourcesEquivalent(स्थिर DfgVertex& a, स्थिर DfgVertex& b) स्थिर {
        क्रम (माप_प्रकार i = 0; i < a.nInputs(); ++i) {
            स्थिर DfgVertex* स्थिर ap = a.inputp(i);
            स्थिर DfgVertex* स्थिर bp = b.inputp(i);
            अगर (!ap && !bp) जारी;
            अगर (!ap || !bp) वापस false;
            अगर (!vertexEquivalent(*ap, *bp)) वापस false;  // Graph is acyclic, so this terminates
        }
        वापस true;
    }

    // Compares 'a' and 'b' for equivalence
    bool vertexEquivalent(स्थिर DfgVertex& a, स्थिर DfgVertex& b) स्थिर {
        // If same vertex, then equal
        अगर (&a == &b) वापस true;

        // If different type, then not equal
        अगर (a.type() != b.type()) वापस false;

        // If different data type, then not equal
        अगर (a.dtype() != b.dtype()) वापस false;

        // If different number of inputs, then not equal
        अगर (a.nInputs() != b.nInputs()) वापस false;

        // Check vertex specifics
        अगर (!vertexSelfEquivalent(a, b)) वापस false;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        अगर (!a.hasMultipleSinks() && !b.hasMultipleSinks()) वापस sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        स्थिर VertexPair कुंजी = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        स्थिर स्वतः it = m_cache.find(कुंजी);
        अगर (it != m_cache.end()) वापस it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        स्थिर bool equal = sourcesEquivalent(a, b);
        अगर (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_आकार / 4);
        m_cache.जोड़ो({कुंजी, equal});

        // The predicate result
        वापस equal;
    }
};

// Combine equivalent operation vertices
व्योम dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    क्रम (DfgConst* स्थिर vtxp : dfg.constVertices().unlinkable()) {
        अगर (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.आकार());

    // Combine operation vertices
    क्रम (DfgVertex* स्थिर vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        अगर (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            जारी;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        स्थिर स्वतः pair = uniqueVtxps.जोड़ो(vtxp);
        अगर (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

व्योम V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
