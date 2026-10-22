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
        साठी (स्थिरांक DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        साठी (स्थिरांक DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        साठी (स्थिरांक DfgVertex& vtx : dfg.opVertices()) {
            जर (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        साठी (स्थिरांक DfgConst& vtx : dfg.constVertices()) {
            स्थिरांक V3Hash hash = vtx.अंक().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.मूल्य()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    माप_प्रकार चालक()(DfgVertex* vtxp) स्थिरांक { परत vertexHash(*vtxp).मूल्य(); }

खाजगी:
    // Returns hash of vertex dependent on information internal to the vertex
    स्थिर V3Hash vertexSelfHash(स्थिरांक DfgVertex& vtx) {
        निवड (vtx.type()) {
        // Unhandled vertices
        स्थिती VDfgType::Logic:  // LCOV_EXCL_START
        स्थिती VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        स्थिती VDfgType::Const:  // LCOV_EXCL_START
        स्थिती VDfgType::CReset:
        स्थिती VDfgType::VarArray:
        स्थिती VDfgType::VarPacked:
        स्थिती VDfgType::Prev:
        स्थिती VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        स्थिती VDfgType::Sel: परत V3Hash{vtx.as<DfgSel>()->lsb()};

        स्थिती VDfgType::SpliceArray:
        स्थिती VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](स्थिरांक DfgVertex&, uint32_t lo) {
                hash += lo;
                परत असत्य;
            });
            परत hash;
        }

        // Vertices with no internal information
        स्थिती VDfgType::MatchMasked:
        स्थिती VDfgType::Mux:
        स्थिती VDfgType::UnitArray: परत V3Hash{};

        // Generated classes - none of them have internal information
        स्थिती VDfgType::Add:
        स्थिती VDfgType::And:
        स्थिती VDfgType::ArraySel:
        स्थिती VDfgType::Concat:
        स्थिती VDfgType::Cond:
        स्थिती VDfgType::मोजणी_Ones_2:
        स्थिती VDfgType::Div:
        स्थिती VDfgType::DivS:
        स्थिती VDfgType::Eq:
        स्थिती VDfgType::EqCase:
        स्थिती VDfgType::EqWild:
        स्थिती VDfgType::Extend:
        स्थिती VDfgType::ExtendS:
        स्थिती VDfgType::Gt:
        स्थिती VDfgType::GtS:
        स्थिती VDfgType::Gte:
        स्थिती VDfgType::GteS:
        स्थिती VDfgType::LogAnd:
        स्थिती VDfgType::LogEq:
        स्थिती VDfgType::LogIf:
        स्थिती VDfgType::LogNot:
        स्थिती VDfgType::LogOr:
        स्थिती VDfgType::Lt:
        स्थिती VDfgType::LtS:
        स्थिती VDfgType::Lte:
        स्थिती VDfgType::LteS:
        स्थिती VDfgType::ModDiv:
        स्थिती VDfgType::ModDivS:
        स्थिती VDfgType::Mul:
        स्थिती VDfgType::MulS:
        स्थिती VDfgType::Negate:
        स्थिती VDfgType::Neq:
        स्थिती VDfgType::NeqCase:
        स्थिती VDfgType::NeqWild:
        स्थिती VDfgType::Not:
        स्थिती VDfgType::OneHot:
        स्थिती VDfgType::OneHot0:
        स्थिती VDfgType::Or:
        स्थिती VDfgType::Pow:
        स्थिती VDfgType::PowSS:
        स्थिती VDfgType::PowSU:
        स्थिती VDfgType::PowUS:
        स्थिती VDfgType::RedAnd:
        स्थिती VDfgType::RedOr:
        स्थिती VDfgType::RedXor:
        स्थिती VDfgType::Rep:
        स्थिती VDfgType::ShiftL:
        स्थिती VDfgType::ShiftR:
        स्थिती VDfgType::ShiftRS:
        स्थिती VDfgType::StreamL:
        स्थिती VDfgType::StreamR:
        स्थिती VDfgType::Sub:
        स्थिती VDfgType::Xor: परत V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) स्थिरांक {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        जर (!result.मूल्य()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                परत असत्य;
            });
            result = hash;
        }
        परत result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
वर्ग DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<स्थिरांक DfgVertex*, स्थिरांक DfgVertex*>;
    संरचना VertexPairHash final {
        माप_प्रकार चालक()(स्थिरांक VertexPair& pair) स्थिरांक {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            परत hash.मूल्य();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, तार्किक, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    स्थिरांक माप_प्रकार m_size;  // Size of the graph

सार्वजनिक:
    // CONSTRUCTORS
    explicit DfgCseEqual(स्थिरांक DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    तार्किक चालक()(DfgVertex* ap, DfgVertex* bp) स्थिरांक { परत vertexEquivalent(*ap, *bp); }

खाजगी:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    तार्किक vertexSelfEquivalent(स्थिरांक DfgVertex& a, स्थिरांक DfgVertex& b) स्थिरांक {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        निवड (a.type()) {
        // Unhandled vertices
        स्थिती VDfgType::Logic:  // LCOV_EXCL_START
        स्थिती VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        स्थिती VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        स्थिती VDfgType::Const: परत a.as<DfgConst>()->अंक().isCaseEq(b.as<DfgConst>()->अंक());
        स्थिती VDfgType::CReset: परत असत्य;
        स्थिती VDfgType::Prev: परत असत्य;

        स्थिती VDfgType::VarArray:
        स्थिती VDfgType::VarPacked:  // CSE does not combine variables
            परत असत्य;

        // Vertices with internal information
        स्थिती VDfgType::Sel: परत a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        स्थिती VDfgType::SpliceArray:
        स्थिती VDfgType::SplicePacked: {
            स्थिरांक DfgVertexSplice* स्थिरांक ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](स्थिरांक DfgVertex&, uint32_t lo) {
                m_driverLo.ढकला_back(lo);
                परत असत्य;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            परत !b.as<DfgVertexSplice>()->foreachDriver([&](स्थिरांक DfgVertex&, uint32_t lo) {  //
                परत *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        स्थिती VDfgType::MatchMasked:
        स्थिती VDfgType::Mux:
        स्थिती VDfgType::UnitArray: परत सत्य;

        // Generated classes - none of them have internal information
        स्थिती VDfgType::Add:
        स्थिती VDfgType::And:
        स्थिती VDfgType::ArraySel:
        स्थिती VDfgType::Concat:
        स्थिती VDfgType::Cond:
        स्थिती VDfgType::मोजणी_Ones_2:
        स्थिती VDfgType::Div:
        स्थिती VDfgType::DivS:
        स्थिती VDfgType::Eq:
        स्थिती VDfgType::EqCase:
        स्थिती VDfgType::EqWild:
        स्थिती VDfgType::Extend:
        स्थिती VDfgType::ExtendS:
        स्थिती VDfgType::Gt:
        स्थिती VDfgType::GtS:
        स्थिती VDfgType::Gte:
        स्थिती VDfgType::GteS:
        स्थिती VDfgType::LogAnd:
        स्थिती VDfgType::LogEq:
        स्थिती VDfgType::LogIf:
        स्थिती VDfgType::LogNot:
        स्थिती VDfgType::LogOr:
        स्थिती VDfgType::Lt:
        स्थिती VDfgType::LtS:
        स्थिती VDfgType::Lte:
        स्थिती VDfgType::LteS:
        स्थिती VDfgType::ModDiv:
        स्थिती VDfgType::ModDivS:
        स्थिती VDfgType::Mul:
        स्थिती VDfgType::MulS:
        स्थिती VDfgType::Negate:
        स्थिती VDfgType::Neq:
        स्थिती VDfgType::NeqCase:
        स्थिती VDfgType::NeqWild:
        स्थिती VDfgType::Not:
        स्थिती VDfgType::OneHot:
        स्थिती VDfgType::OneHot0:
        स्थिती VDfgType::Or:
        स्थिती VDfgType::Pow:
        स्थिती VDfgType::PowSS:
        स्थिती VDfgType::PowSU:
        स्थिती VDfgType::PowUS:
        स्थिती VDfgType::RedAnd:
        स्थिती VDfgType::RedOr:
        स्थिती VDfgType::RedXor:
        स्थिती VDfgType::Rep:
        स्थिती VDfgType::ShiftL:
        स्थिती VDfgType::ShiftR:
        स्थिती VDfgType::ShiftRS:
        स्थिती VDfgType::StreamL:
        स्थिती VDfgType::StreamR:
        स्थिती VDfgType::Sub:
        स्थिती VDfgType::Xor: परत सत्य;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    तार्किक sourcesEquivalent(स्थिरांक DfgVertex& a, स्थिरांक DfgVertex& b) स्थिरांक {
        साठी (माप_प्रकार i = 0; i < a.nInputs(); ++i) {
            स्थिरांक DfgVertex* स्थिरांक ap = a.inputp(i);
            स्थिरांक DfgVertex* स्थिरांक bp = b.inputp(i);
            जर (!ap && !bp) सुरूठेवा;
            जर (!ap || !bp) परत असत्य;
            जर (!vertexEquivalent(*ap, *bp)) परत असत्य;  // Graph is acyclic, so this terminates
        }
        परत सत्य;
    }

    // Compares 'a' and 'b' for equivalence
    तार्किक vertexEquivalent(स्थिरांक DfgVertex& a, स्थिरांक DfgVertex& b) स्थिरांक {
        // If same vertex, then equal
        जर (&a == &b) परत सत्य;

        // If different type, then not equal
        जर (a.type() != b.type()) परत असत्य;

        // If different data type, then not equal
        जर (a.dtype() != b.dtype()) परत असत्य;

        // If different number of inputs, then not equal
        जर (a.nInputs() != b.nInputs()) परत असत्य;

        // Check vertex specifics
        जर (!vertexSelfEquivalent(a, b)) परत असत्य;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        जर (!a.hasMultipleSinks() && !b.hasMultipleSinks()) परत sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        स्थिरांक VertexPair किल्ली = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        स्थिरांक स्वतः it = m_cache.find(किल्ली);
        जर (it != m_cache.end()) परत it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        स्थिरांक तार्किक equal = sourcesEquivalent(a, b);
        जर (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.घाला({किल्ली, equal});

        // The predicate result
        परत equal;
    }
};

// Combine equivalent operation vertices
शून्य dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    साठी (DfgConst* स्थिरांक vtxp : dfg.constVertices().unlinkable()) {
        जर (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    साठी (DfgVertex* स्थिरांक vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        जर (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            सुरूठेवा;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        स्थिरांक स्वतः pair = uniqueVtxps.घाला(vtxp);
        जर (!pair.second) {
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
