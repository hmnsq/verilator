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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
वर्ग DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

सार्वजनिक:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        लागि (अचर DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        लागि (अचर DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        लागि (अचर DfgVertex& vtx : dfg.opVertices()) {
            यदि (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        लागि (अचर DfgConst& vtx : dfg.constVertices()) {
            अचर V3Hash hash = vtx.अङ्क().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.मान()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    माप_प्रकार चालक()(DfgVertex* vtxp) अचर { फर्काऊ vertexHash(*vtxp).मान(); }

गुप्त:
    // Returns hash of vertex dependent on information internal to the vertex
    स्थिर V3Hash vertexSelfHash(अचर DfgVertex& vtx) {
        विकल्प (vtx.type()) {
        // Unhandled vertices
        पक्ष VDfgType::Logic:  // LCOV_EXCL_START
        पक्ष VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        पक्ष VDfgType::Const:  // LCOV_EXCL_START
        पक्ष VDfgType::CReset:
        पक्ष VDfgType::VarArray:
        पक्ष VDfgType::VarPacked:
        पक्ष VDfgType::Prev:
        पक्ष VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        पक्ष VDfgType::Sel: फर्काऊ V3Hash{vtx.as<DfgSel>()->lsb()};

        पक्ष VDfgType::SpliceArray:
        पक्ष VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](अचर DfgVertex&, uint32_t lo) {
                hash += lo;
                फर्काऊ असत्य;
            });
            फर्काऊ hash;
        }

        // Vertices with no internal information
        पक्ष VDfgType::MatchMasked:
        पक्ष VDfgType::Mux:
        पक्ष VDfgType::UnitArray: फर्काऊ V3Hash{};

        // Generated classes - none of them have internal information
        पक्ष VDfgType::Add:
        पक्ष VDfgType::And:
        पक्ष VDfgType::ArraySel:
        पक्ष VDfgType::Concat:
        पक्ष VDfgType::Cond:
        पक्ष VDfgType::गन्ती_Ones_2:
        पक्ष VDfgType::Div:
        पक्ष VDfgType::DivS:
        पक्ष VDfgType::Eq:
        पक्ष VDfgType::EqCase:
        पक्ष VDfgType::EqWild:
        पक्ष VDfgType::Extend:
        पक्ष VDfgType::ExtendS:
        पक्ष VDfgType::Gt:
        पक्ष VDfgType::GtS:
        पक्ष VDfgType::Gte:
        पक्ष VDfgType::GteS:
        पक्ष VDfgType::LogAnd:
        पक्ष VDfgType::LogEq:
        पक्ष VDfgType::LogIf:
        पक्ष VDfgType::LogNot:
        पक्ष VDfgType::LogOr:
        पक्ष VDfgType::Lt:
        पक्ष VDfgType::LtS:
        पक्ष VDfgType::Lte:
        पक्ष VDfgType::LteS:
        पक्ष VDfgType::ModDiv:
        पक्ष VDfgType::ModDivS:
        पक्ष VDfgType::Mul:
        पक्ष VDfgType::MulS:
        पक्ष VDfgType::Negate:
        पक्ष VDfgType::Neq:
        पक्ष VDfgType::NeqCase:
        पक्ष VDfgType::NeqWild:
        पक्ष VDfgType::Not:
        पक्ष VDfgType::OneHot:
        पक्ष VDfgType::OneHot0:
        पक्ष VDfgType::Or:
        पक्ष VDfgType::Pow:
        पक्ष VDfgType::PowSS:
        पक्ष VDfgType::PowSU:
        पक्ष VDfgType::PowUS:
        पक्ष VDfgType::RedAnd:
        पक्ष VDfgType::RedOr:
        पक्ष VDfgType::RedXor:
        पक्ष VDfgType::Rep:
        पक्ष VDfgType::ShiftL:
        पक्ष VDfgType::ShiftR:
        पक्ष VDfgType::ShiftRS:
        पक्ष VDfgType::StreamL:
        पक्ष VDfgType::StreamR:
        पक्ष VDfgType::Sub:
        पक्ष VDfgType::Xor: फर्काऊ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) अचर {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        यदि (!result.मान()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                फर्काऊ असत्य;
            });
            result = hash;
        }
        फर्काऊ result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
वर्ग DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<अचर DfgVertex*, अचर DfgVertex*>;
    संरचना VertexPairHash final {
        माप_प्रकार चालक()(अचर VertexPair& pair) अचर {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            फर्काऊ hash.मान();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, तार्किक, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    अचर माप_प्रकार m_size;  // Size of the graph

सार्वजनिक:
    // CONSTRUCTORS
    explicit DfgCseEqual(अचर DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    तार्किक चालक()(DfgVertex* ap, DfgVertex* bp) अचर { फर्काऊ vertexEquivalent(*ap, *bp); }

गुप्त:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    तार्किक vertexSelfEquivalent(अचर DfgVertex& a, अचर DfgVertex& b) अचर {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        विकल्प (a.type()) {
        // Unhandled vertices
        पक्ष VDfgType::Logic:  // LCOV_EXCL_START
        पक्ष VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        पक्ष VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        पक्ष VDfgType::Const: फर्काऊ a.as<DfgConst>()->अङ्क().isCaseEq(b.as<DfgConst>()->अङ्क());
        पक्ष VDfgType::CReset: फर्काऊ असत्य;
        पक्ष VDfgType::Prev: फर्काऊ असत्य;

        पक्ष VDfgType::VarArray:
        पक्ष VDfgType::VarPacked:  // CSE does not combine variables
            फर्काऊ असत्य;

        // Vertices with internal information
        पक्ष VDfgType::Sel: फर्काऊ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        पक्ष VDfgType::SpliceArray:
        पक्ष VDfgType::SplicePacked: {
            अचर DfgVertexSplice* अचर ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](अचर DfgVertex&, uint32_t lo) {
                m_driverLo.थप_back(lo);
                फर्काऊ असत्य;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            फर्काऊ !b.as<DfgVertexSplice>()->foreachDriver([&](अचर DfgVertex&, uint32_t lo) {  //
                फर्काऊ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        पक्ष VDfgType::MatchMasked:
        पक्ष VDfgType::Mux:
        पक्ष VDfgType::UnitArray: फर्काऊ सत्य;

        // Generated classes - none of them have internal information
        पक्ष VDfgType::Add:
        पक्ष VDfgType::And:
        पक्ष VDfgType::ArraySel:
        पक्ष VDfgType::Concat:
        पक्ष VDfgType::Cond:
        पक्ष VDfgType::गन्ती_Ones_2:
        पक्ष VDfgType::Div:
        पक्ष VDfgType::DivS:
        पक्ष VDfgType::Eq:
        पक्ष VDfgType::EqCase:
        पक्ष VDfgType::EqWild:
        पक्ष VDfgType::Extend:
        पक्ष VDfgType::ExtendS:
        पक्ष VDfgType::Gt:
        पक्ष VDfgType::GtS:
        पक्ष VDfgType::Gte:
        पक्ष VDfgType::GteS:
        पक्ष VDfgType::LogAnd:
        पक्ष VDfgType::LogEq:
        पक्ष VDfgType::LogIf:
        पक्ष VDfgType::LogNot:
        पक्ष VDfgType::LogOr:
        पक्ष VDfgType::Lt:
        पक्ष VDfgType::LtS:
        पक्ष VDfgType::Lte:
        पक्ष VDfgType::LteS:
        पक्ष VDfgType::ModDiv:
        पक्ष VDfgType::ModDivS:
        पक्ष VDfgType::Mul:
        पक्ष VDfgType::MulS:
        पक्ष VDfgType::Negate:
        पक्ष VDfgType::Neq:
        पक्ष VDfgType::NeqCase:
        पक्ष VDfgType::NeqWild:
        पक्ष VDfgType::Not:
        पक्ष VDfgType::OneHot:
        पक्ष VDfgType::OneHot0:
        पक्ष VDfgType::Or:
        पक्ष VDfgType::Pow:
        पक्ष VDfgType::PowSS:
        पक्ष VDfgType::PowSU:
        पक्ष VDfgType::PowUS:
        पक्ष VDfgType::RedAnd:
        पक्ष VDfgType::RedOr:
        पक्ष VDfgType::RedXor:
        पक्ष VDfgType::Rep:
        पक्ष VDfgType::ShiftL:
        पक्ष VDfgType::ShiftR:
        पक्ष VDfgType::ShiftRS:
        पक्ष VDfgType::StreamL:
        पक्ष VDfgType::StreamR:
        पक्ष VDfgType::Sub:
        पक्ष VDfgType::Xor: फर्काऊ सत्य;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    तार्किक sourcesEquivalent(अचर DfgVertex& a, अचर DfgVertex& b) अचर {
        लागि (माप_प्रकार i = 0; i < a.nInputs(); ++i) {
            अचर DfgVertex* अचर ap = a.inputp(i);
            अचर DfgVertex* अचर bp = b.inputp(i);
            यदि (!ap && !bp) जारी;
            यदि (!ap || !bp) फर्काऊ असत्य;
            यदि (!vertexEquivalent(*ap, *bp)) फर्काऊ असत्य;  // Graph is acyclic, so this terminates
        }
        फर्काऊ सत्य;
    }

    // Compares 'a' and 'b' for equivalence
    तार्किक vertexEquivalent(अचर DfgVertex& a, अचर DfgVertex& b) अचर {
        // If same vertex, then equal
        यदि (&a == &b) फर्काऊ सत्य;

        // If different type, then not equal
        यदि (a.type() != b.type()) फर्काऊ असत्य;

        // If different data type, then not equal
        यदि (a.dtype() != b.dtype()) फर्काऊ असत्य;

        // If different number of inputs, then not equal
        यदि (a.nInputs() != b.nInputs()) फर्काऊ असत्य;

        // Check vertex specifics
        यदि (!vertexSelfEquivalent(a, b)) फर्काऊ असत्य;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        यदि (!a.hasMultipleSinks() && !b.hasMultipleSinks()) फर्काऊ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        अचर VertexPair साँचो = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        अचर स्वतः it = m_cache.find(साँचो);
        यदि (it != m_cache.end()) फर्काऊ it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        अचर तार्किक equal = sourcesEquivalent(a, b);
        यदि (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.घुसाउ({साँचो, equal});

        // The predicate result
        फर्काऊ equal;
    }
};

// Combine equivalent operation vertices
शून्य dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    लागि (DfgConst* अचर vtxp : dfg.constVertices().unlinkable()) {
        यदि (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    लागि (DfgVertex* अचर vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        यदि (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            जारी;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        अचर स्वतः pair = uniqueVtxps.घुसाउ(vtxp);
        यदि (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

शून्य V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
