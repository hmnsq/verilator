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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"
#ଅନ୍ତର୍ଭାଵ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
ଶ୍ରେଣୀ DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

ସର୍ବସାଧାରଣ:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        ପାଇଁ (ଅଚଳ DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        ପାଇଁ (ଅଚଳ DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        ପାଇଁ (ଅଚଳ DfgVertex& vtx : dfg.opVertices()) {
            ଯଦି (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        ପାଇଁ (ଅଚଳ DfgConst& vtx : dfg.constVertices()) {
            ଅଚଳ V3Hash hash = vtx.ଅଙ୍କ().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.ମୂଲ୍ୟ()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ଅଚଳ { ଫେରନ୍ତୁ vertexHash(*vtxp).ମୂଲ୍ୟ(); }

ବ୍ୟକ୍ତିଗତ:
    // Returns hash of vertex dependent on information internal to the vertex
    ସ୍ଥିର V3Hash vertexSelfHash(ଅଚଳ DfgVertex& vtx) {
        ବାଛ (vtx.type()) {
        // Unhandled vertices
        ସ୍ଥିତି VDfgType::Logic:  // LCOV_EXCL_START
        ସ୍ଥିତି VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        ସ୍ଥିତି VDfgType::Const:  // LCOV_EXCL_START
        ସ୍ଥିତି VDfgType::CReset:
        ସ୍ଥିତି VDfgType::VarArray:
        ସ୍ଥିତି VDfgType::VarPacked:
        ସ୍ଥିତି VDfgType::Prev:
        ସ୍ଥିତି VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        ସ୍ଥିତି VDfgType::Sel: ଫେରନ୍ତୁ V3Hash{vtx.as<DfgSel>()->lsb()};

        ସ୍ଥିତି VDfgType::SpliceArray:
        ସ୍ଥିତି VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ଅଚଳ DfgVertex&, uint32_t lo) {
                hash += lo;
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            ଫେରନ୍ତୁ hash;
        }

        // Vertices with no internal information
        ସ୍ଥିତି VDfgType::MatchMasked:
        ସ୍ଥିତି VDfgType::Mux:
        ସ୍ଥିତି VDfgType::UnitArray: ଫେରନ୍ତୁ V3Hash{};

        // Generated classes - none of them have internal information
        ସ୍ଥିତି VDfgType::Add:
        ସ୍ଥିତି VDfgType::And:
        ସ୍ଥିତି VDfgType::ArraySel:
        ସ୍ଥିତି VDfgType::Concat:
        ସ୍ଥିତି VDfgType::Cond:
        ସ୍ଥିତି VDfgType::ଗଣନା_Ones_2:
        ସ୍ଥିତି VDfgType::Div:
        ସ୍ଥିତି VDfgType::DivS:
        ସ୍ଥିତି VDfgType::Eq:
        ସ୍ଥିତି VDfgType::EqCase:
        ସ୍ଥିତି VDfgType::EqWild:
        ସ୍ଥିତି VDfgType::Extend:
        ସ୍ଥିତି VDfgType::ExtendS:
        ସ୍ଥିତି VDfgType::Gt:
        ସ୍ଥିତି VDfgType::GtS:
        ସ୍ଥିତି VDfgType::Gte:
        ସ୍ଥିତି VDfgType::GteS:
        ସ୍ଥିତି VDfgType::LogAnd:
        ସ୍ଥିତି VDfgType::LogEq:
        ସ୍ଥିତି VDfgType::LogIf:
        ସ୍ଥିତି VDfgType::LogNot:
        ସ୍ଥିତି VDfgType::LogOr:
        ସ୍ଥିତି VDfgType::Lt:
        ସ୍ଥିତି VDfgType::LtS:
        ସ୍ଥିତି VDfgType::Lte:
        ସ୍ଥିତି VDfgType::LteS:
        ସ୍ଥିତି VDfgType::ModDiv:
        ସ୍ଥିତି VDfgType::ModDivS:
        ସ୍ଥିତି VDfgType::Mul:
        ସ୍ଥିତି VDfgType::MulS:
        ସ୍ଥିତି VDfgType::Negate:
        ସ୍ଥିତି VDfgType::Neq:
        ସ୍ଥିତି VDfgType::NeqCase:
        ସ୍ଥିତି VDfgType::NeqWild:
        ସ୍ଥିତି VDfgType::Not:
        ସ୍ଥିତି VDfgType::OneHot:
        ସ୍ଥିତି VDfgType::OneHot0:
        ସ୍ଥିତି VDfgType::Or:
        ସ୍ଥିତି VDfgType::Pow:
        ସ୍ଥିତି VDfgType::PowSS:
        ସ୍ଥିତି VDfgType::PowSU:
        ସ୍ଥିତି VDfgType::PowUS:
        ସ୍ଥିତି VDfgType::RedAnd:
        ସ୍ଥିତି VDfgType::RedOr:
        ସ୍ଥିତି VDfgType::RedXor:
        ସ୍ଥିତି VDfgType::Rep:
        ସ୍ଥିତି VDfgType::ShiftL:
        ସ୍ଥିତି VDfgType::ShiftR:
        ସ୍ଥିତି VDfgType::ShiftRS:
        ସ୍ଥିତି VDfgType::StreamL:
        ସ୍ଥିତି VDfgType::StreamR:
        ସ୍ଥିତି VDfgType::Sub:
        ସ୍ଥିତି VDfgType::Xor: ଫେରନ୍ତୁ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ଅଚଳ {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ଯଦି (!result.ମୂଲ୍ୟ()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            result = hash;
        }
        ଫେରନ୍ତୁ result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
ଶ୍ରେଣୀ DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ଅଚଳ DfgVertex*, ଅଚଳ DfgVertex*>;
    ସଂରଚନା VertexPairHash final {
        size_t operator()(ଅଚଳ VertexPair& pair) ଅଚଳ {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, ତାର୍କିକ, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ଅଚଳ size_t m_size;  // Size of the graph

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit DfgCseEqual(ଅଚଳ DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    ତାର୍କିକ operator()(DfgVertex* ap, DfgVertex* bp) ଅଚଳ { ଫେରନ୍ତୁ vertexEquivalent(*ap, *bp); }

ବ୍ୟକ୍ତିଗତ:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    ତାର୍କିକ vertexSelfEquivalent(ଅଚଳ DfgVertex& a, ଅଚଳ DfgVertex& b) ଅଚଳ {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        ବାଛ (a.type()) {
        // Unhandled vertices
        ସ୍ଥିତି VDfgType::Logic:  // LCOV_EXCL_START
        ସ୍ଥିତି VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        ସ୍ଥିତି VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        ସ୍ଥିତି VDfgType::Const: ଫେରନ୍ତୁ a.as<DfgConst>()->ଅଙ୍କ().isCaseEq(b.as<DfgConst>()->ଅଙ୍କ());
        ସ୍ଥିତି VDfgType::CReset: ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ସ୍ଥିତି VDfgType::Prev: ଫେରନ୍ତୁ ମିଥ୍ୟା;

        ସ୍ଥିତି VDfgType::VarArray:
        ସ୍ଥିତି VDfgType::VarPacked:  // CSE does not combine variables
            ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Vertices with internal information
        ସ୍ଥିତି VDfgType::Sel: ଫେରନ୍ତୁ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        ସ୍ଥିତି VDfgType::SpliceArray:
        ସ୍ଥିତି VDfgType::SplicePacked: {
            ଅଚଳ DfgVertexSplice* ଅଚଳ ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ଅଚଳ DfgVertex&, uint32_t lo) {
                m_driverLo.ଠେଲ_back(lo);
                ଫେରନ୍ତୁ ମିଥ୍ୟା;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            ଫେରନ୍ତୁ !b.as<DfgVertexSplice>()->foreachDriver([&](ଅଚଳ DfgVertex&, uint32_t lo) {  //
                ଫେରନ୍ତୁ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        ସ୍ଥିତି VDfgType::MatchMasked:
        ସ୍ଥିତି VDfgType::Mux:
        ସ୍ଥିତି VDfgType::UnitArray: ଫେରନ୍ତୁ ସତ୍ୟ;

        // Generated classes - none of them have internal information
        ସ୍ଥିତି VDfgType::Add:
        ସ୍ଥିତି VDfgType::And:
        ସ୍ଥିତି VDfgType::ArraySel:
        ସ୍ଥିତି VDfgType::Concat:
        ସ୍ଥିତି VDfgType::Cond:
        ସ୍ଥିତି VDfgType::ଗଣନା_Ones_2:
        ସ୍ଥିତି VDfgType::Div:
        ସ୍ଥିତି VDfgType::DivS:
        ସ୍ଥିତି VDfgType::Eq:
        ସ୍ଥିତି VDfgType::EqCase:
        ସ୍ଥିତି VDfgType::EqWild:
        ସ୍ଥିତି VDfgType::Extend:
        ସ୍ଥିତି VDfgType::ExtendS:
        ସ୍ଥିତି VDfgType::Gt:
        ସ୍ଥିତି VDfgType::GtS:
        ସ୍ଥିତି VDfgType::Gte:
        ସ୍ଥିତି VDfgType::GteS:
        ସ୍ଥିତି VDfgType::LogAnd:
        ସ୍ଥିତି VDfgType::LogEq:
        ସ୍ଥିତି VDfgType::LogIf:
        ସ୍ଥିତି VDfgType::LogNot:
        ସ୍ଥିତି VDfgType::LogOr:
        ସ୍ଥିତି VDfgType::Lt:
        ସ୍ଥିତି VDfgType::LtS:
        ସ୍ଥିତି VDfgType::Lte:
        ସ୍ଥିତି VDfgType::LteS:
        ସ୍ଥିତି VDfgType::ModDiv:
        ସ୍ଥିତି VDfgType::ModDivS:
        ସ୍ଥିତି VDfgType::Mul:
        ସ୍ଥିତି VDfgType::MulS:
        ସ୍ଥିତି VDfgType::Negate:
        ସ୍ଥିତି VDfgType::Neq:
        ସ୍ଥିତି VDfgType::NeqCase:
        ସ୍ଥିତି VDfgType::NeqWild:
        ସ୍ଥିତି VDfgType::Not:
        ସ୍ଥିତି VDfgType::OneHot:
        ସ୍ଥିତି VDfgType::OneHot0:
        ସ୍ଥିତି VDfgType::Or:
        ସ୍ଥିତି VDfgType::Pow:
        ସ୍ଥିତି VDfgType::PowSS:
        ସ୍ଥିତି VDfgType::PowSU:
        ସ୍ଥିତି VDfgType::PowUS:
        ସ୍ଥିତି VDfgType::RedAnd:
        ସ୍ଥିତି VDfgType::RedOr:
        ସ୍ଥିତି VDfgType::RedXor:
        ସ୍ଥିତି VDfgType::Rep:
        ସ୍ଥିତି VDfgType::ShiftL:
        ସ୍ଥିତି VDfgType::ShiftR:
        ସ୍ଥିତି VDfgType::ShiftRS:
        ସ୍ଥିତି VDfgType::StreamL:
        ସ୍ଥିତି VDfgType::StreamR:
        ସ୍ଥିତି VDfgType::Sub:
        ସ୍ଥିତି VDfgType::Xor: ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    ତାର୍କିକ sourcesEquivalent(ଅଚଳ DfgVertex& a, ଅଚଳ DfgVertex& b) ଅଚଳ {
        ପାଇଁ (size_t i = 0; i < a.nInputs(); ++i) {
            ଅଚଳ DfgVertex* ଅଚଳ ap = a.inputp(i);
            ଅଚଳ DfgVertex* ଅଚଳ bp = b.inputp(i);
            ଯଦି (!ap && !bp) ଜାରି;
            ଯଦି (!ap || !bp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଯଦି (!vertexEquivalent(*ap, *bp)) ଫେରନ୍ତୁ ମିଥ୍ୟା;  // Graph is acyclic, so this terminates
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Compares 'a' and 'b' for equivalence
    ତାର୍କିକ vertexEquivalent(ଅଚଳ DfgVertex& a, ଅଚଳ DfgVertex& b) ଅଚଳ {
        // If same vertex, then equal
        ଯଦି (&a == &b) ଫେରନ୍ତୁ ସତ୍ୟ;

        // If different type, then not equal
        ଯଦି (a.type() != b.type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // If different data type, then not equal
        ଯଦି (a.dtype() != b.dtype()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // If different number of inputs, then not equal
        ଯଦି (a.nInputs() != b.nInputs()) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // Check vertex specifics
        ଯଦି (!vertexSelfEquivalent(a, b)) ଫେରନ୍ତୁ ମିଥ୍ୟା;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ଯଦି (!a.hasMultipleSinks() && !b.hasMultipleSinks()) ଫେରନ୍ତୁ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ଅଚଳ VertexPair ଚାବି = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ଅଚଳ auto it = m_cache.find(ଚାବି);
        ଯଦି (it != m_cache.end()) ଫେରନ୍ତୁ it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ଅଚଳ ତାର୍କିକ equal = sourcesEquivalent(a, b);
        ଯଦି (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.ଭର୍ତ୍ତି({ଚାବି, equal});

        // The predicate result
        ଫେରନ୍ତୁ equal;
    }
};

// Combine equivalent operation vertices
ଶୂନ୍ୟ dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    ପାଇଁ (DfgConst* ଅଚଳ vtxp : dfg.constVertices().unlinkable()) {
        ଯଦି (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    ପାଇଁ (DfgVertex* ଅଚଳ vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ଯଦି (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            ଜାରି;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ଅଚଳ auto pair = uniqueVtxps.ଭର୍ତ୍ତି(vtxp);
        ଯଦି (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

ଶୂନ୍ୟ V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
