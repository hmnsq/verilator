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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
رده DfgCseHash final {
    // STATE
    mutable DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

عمومی:
    // CONSTRUCTOR
    explicit DfgCseHash(DfgGraph& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        برای (ثابت DfgVertexVar& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        برای (ثابت DfgVertexAst& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        برای (ثابت DfgVertex& vtx : dfg.opVertices()) {
            اگر (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        برای (ثابت DfgConst& vtx : dfg.constVertices()) {
            ثابت V3Hash hash = vtx.رقم().toHash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.مقدار()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    size_t operator()(DfgVertex* vtxp) ثابت { بازگشت vertexHash(*vtxp).مقدار(); }

خصوصی:
    // Returns hash of vertex dependent on information internal to the vertex
    ایستا V3Hash vertexSelfHash(ثابت DfgVertex& vtx) {
        انتخاب (vtx.type()) {
        // Unhandled vertices
        حالت VDfgType::Logic:  // LCOV_EXCL_START
        حالت VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        حالت VDfgType::Const:  // LCOV_EXCL_START
        حالت VDfgType::CReset:
        حالت VDfgType::VarArray:
        حالت VDfgType::VarPacked:
        حالت VDfgType::Prev:
        حالت VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        حالت VDfgType::Sel: بازگشت V3Hash{vtx.as<DfgSel>()->lsb()};

        حالت VDfgType::SpliceArray:
        حالت VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<DfgVertexSplice>()->foreachDriver([&](ثابت DfgVertex&, uint32_t lo) {
                hash += lo;
                بازگشت نادرست;
            });
            بازگشت hash;
        }

        // Vertices with no internal information
        حالت VDfgType::MatchMasked:
        حالت VDfgType::Mux:
        حالت VDfgType::UnitArray: بازگشت V3Hash{};

        // Generated classes - none of them have internal information
        حالت VDfgType::Add:
        حالت VDfgType::And:
        حالت VDfgType::ArraySel:
        حالت VDfgType::Concat:
        حالت VDfgType::Cond:
        حالت VDfgType::شمار_Ones_2:
        حالت VDfgType::Div:
        حالت VDfgType::DivS:
        حالت VDfgType::Eq:
        حالت VDfgType::EqCase:
        حالت VDfgType::EqWild:
        حالت VDfgType::Extend:
        حالت VDfgType::ExtendS:
        حالت VDfgType::Gt:
        حالت VDfgType::GtS:
        حالت VDfgType::Gte:
        حالت VDfgType::GteS:
        حالت VDfgType::LogAnd:
        حالت VDfgType::LogEq:
        حالت VDfgType::LogIf:
        حالت VDfgType::LogNot:
        حالت VDfgType::LogOr:
        حالت VDfgType::Lt:
        حالت VDfgType::LtS:
        حالت VDfgType::Lte:
        حالت VDfgType::LteS:
        حالت VDfgType::ModDiv:
        حالت VDfgType::ModDivS:
        حالت VDfgType::Mul:
        حالت VDfgType::MulS:
        حالت VDfgType::Negate:
        حالت VDfgType::Neq:
        حالت VDfgType::NeqCase:
        حالت VDfgType::NeqWild:
        حالت VDfgType::Not:
        حالت VDfgType::OneHot:
        حالت VDfgType::OneHot0:
        حالت VDfgType::Or:
        حالت VDfgType::Pow:
        حالت VDfgType::PowSS:
        حالت VDfgType::PowSU:
        حالت VDfgType::PowUS:
        حالت VDfgType::RedAnd:
        حالت VDfgType::RedOr:
        حالت VDfgType::RedXor:
        حالت VDfgType::Rep:
        حالت VDfgType::ShiftL:
        حالت VDfgType::ShiftR:
        حالت VDfgType::ShiftRS:
        حالت VDfgType::StreamL:
        حالت VDfgType::StreamR:
        حالت VDfgType::Sub:
        حالت VDfgType::Xor: بازگشت V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash vertexHash(DfgVertex& vtx) ثابت {
        V3Hash& result = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        اگر (!result.مقدار()) {
            V3Hash hash{vertexSelfHash(vtx)};
            hash += vtx.type();
            hash += vtx.size();
            vtx.foreachSource([&](DfgVertex& src) {
                hash += vertexHash(src);  // Graph is acyclic, so this terminates
                بازگشت نادرست;
            });
            result = hash;
        }
        بازگشت result;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
رده DfgCseEqual final {
    // TYPES
    using VertexPair = std::pair<ثابت DfgVertex*, ثابت DfgVertex*>;
    ساختار VertexPairHash final {
        size_t operator()(ثابت VertexPair& pair) ثابت {
            V3Hash hash;
            hash += pair.first;
            hash += pair.second;
            بازگشت hash.مقدار();
        }
    };

    // STATE
    mutable V3HashMap<VertexPair, منطقی, VertexPairHash> m_cache;  // Cache for vertex equality
    mutable std::vector<uint32_t> m_driverLo;  // Low indices of drivers
    ثابت size_t m_size;  // Size of the graph

عمومی:
    // CONSTRUCTORS
    explicit DfgCseEqual(ثابت DfgGraph& dfg)
        : m_size{dfg.size()} {}

    // METHODS
    منطقی operator()(DfgVertex* ap, DfgVertex* bp) ثابت { بازگشت vertexEquivalent(*ap, *bp); }

خصوصی:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    منطقی vertexSelfEquivalent(ثابت DfgVertex& a, ثابت DfgVertex& b) ثابت {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        انتخاب (a.type()) {
        // Unhandled vertices
        حالت VDfgType::Logic:  // LCOV_EXCL_START
        حالت VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        حالت VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        حالت VDfgType::Const: بازگشت a.as<DfgConst>()->رقم().isCaseEq(b.as<DfgConst>()->رقم());
        حالت VDfgType::CReset: بازگشت نادرست;
        حالت VDfgType::Prev: بازگشت نادرست;

        حالت VDfgType::VarArray:
        حالت VDfgType::VarPacked:  // CSE does not combine variables
            بازگشت نادرست;

        // Vertices with internal information
        حالت VDfgType::Sel: بازگشت a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        حالت VDfgType::SpliceArray:
        حالت VDfgType::SplicePacked: {
            ثابت DfgVertexSplice* ثابت ap = a.as<DfgVertexSplice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ثابت DfgVertex&, uint32_t lo) {
                m_driverLo.افزودن_back(lo);
                بازگشت نادرست;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.data();
            بازگشت !b.as<DfgVertexSplice>()->foreachDriver([&](ثابت DfgVertex&, uint32_t lo) {  //
                بازگشت *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        حالت VDfgType::MatchMasked:
        حالت VDfgType::Mux:
        حالت VDfgType::UnitArray: بازگشت درست;

        // Generated classes - none of them have internal information
        حالت VDfgType::Add:
        حالت VDfgType::And:
        حالت VDfgType::ArraySel:
        حالت VDfgType::Concat:
        حالت VDfgType::Cond:
        حالت VDfgType::شمار_Ones_2:
        حالت VDfgType::Div:
        حالت VDfgType::DivS:
        حالت VDfgType::Eq:
        حالت VDfgType::EqCase:
        حالت VDfgType::EqWild:
        حالت VDfgType::Extend:
        حالت VDfgType::ExtendS:
        حالت VDfgType::Gt:
        حالت VDfgType::GtS:
        حالت VDfgType::Gte:
        حالت VDfgType::GteS:
        حالت VDfgType::LogAnd:
        حالت VDfgType::LogEq:
        حالت VDfgType::LogIf:
        حالت VDfgType::LogNot:
        حالت VDfgType::LogOr:
        حالت VDfgType::Lt:
        حالت VDfgType::LtS:
        حالت VDfgType::Lte:
        حالت VDfgType::LteS:
        حالت VDfgType::ModDiv:
        حالت VDfgType::ModDivS:
        حالت VDfgType::Mul:
        حالت VDfgType::MulS:
        حالت VDfgType::Negate:
        حالت VDfgType::Neq:
        حالت VDfgType::NeqCase:
        حالت VDfgType::NeqWild:
        حالت VDfgType::Not:
        حالت VDfgType::OneHot:
        حالت VDfgType::OneHot0:
        حالت VDfgType::Or:
        حالت VDfgType::Pow:
        حالت VDfgType::PowSS:
        حالت VDfgType::PowSU:
        حالت VDfgType::PowUS:
        حالت VDfgType::RedAnd:
        حالت VDfgType::RedOr:
        حالت VDfgType::RedXor:
        حالت VDfgType::Rep:
        حالت VDfgType::ShiftL:
        حالت VDfgType::ShiftR:
        حالت VDfgType::ShiftRS:
        حالت VDfgType::StreamL:
        حالت VDfgType::StreamR:
        حالت VDfgType::Sub:
        حالت VDfgType::Xor: بازگشت درست;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    منطقی sourcesEquivalent(ثابت DfgVertex& a, ثابت DfgVertex& b) ثابت {
        برای (size_t i = 0; i < a.nInputs(); ++i) {
            ثابت DfgVertex* ثابت ap = a.inputp(i);
            ثابت DfgVertex* ثابت bp = b.inputp(i);
            اگر (!ap && !bp) ادامه;
            اگر (!ap || !bp) بازگشت نادرست;
            اگر (!vertexEquivalent(*ap, *bp)) بازگشت نادرست;  // Graph is acyclic, so this terminates
        }
        بازگشت درست;
    }

    // Compares 'a' and 'b' for equivalence
    منطقی vertexEquivalent(ثابت DfgVertex& a, ثابت DfgVertex& b) ثابت {
        // If same vertex, then equal
        اگر (&a == &b) بازگشت درست;

        // If different type, then not equal
        اگر (a.type() != b.type()) بازگشت نادرست;

        // If different data type, then not equal
        اگر (a.dtype() != b.dtype()) بازگشت نادرست;

        // If different number of inputs, then not equal
        اگر (a.nInputs() != b.nInputs()) بازگشت نادرست;

        // Check vertex specifics
        اگر (!vertexSelfEquivalent(a, b)) بازگشت نادرست;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        اگر (!a.hasMultipleSinks() && !b.hasMultipleSinks()) بازگشت sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ثابت VertexPair کلید = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ثابت auto it = m_cache.find(کلید);
        اگر (it != m_cache.end()) بازگشت it->second;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ثابت منطقی equal = sourcesEquivalent(a, b);
        اگر (VL_UNLIKELY(m_cache.empty())) m_cache.reserve(m_size / 4);
        m_cache.درج({کلید, equal});

        // The predicate result
        بازگشت equal;
    }
};

// Combine equivalent operation vertices
تهی dfgCseCombineEquivalent(DfgGraph& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    برای (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        اگر (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3HashSet<DfgVertex*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.size());

    // Combine operation vertices
    برای (DfgVertex* ثابت vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        اگر (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            ادامه;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ثابت auto pair = uniqueVtxps.درج(vtxp);
        اگر (!pair.second) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.first);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

تهی V3DfgPasses::cse(DfgGraph& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::removeUnused(dfg);
}
