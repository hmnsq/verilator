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
جماعت DfgCseHash final {
    // STATE
    تغیرپذیر DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

عوامی:
    // CONSTRUCTOR
    صریح DfgCseHash(Dfg_گراف& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        برائے (ثابت Dfg_راس_Var& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        برائے (ثابت Dfg_راس_Ast& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        برائے (ثابت Dfg_راس& vtx : dfg.opVertices()) {
            اگر (vtx.is<DfgCReset>() || vtx.is<Dfg_پچھلا>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        برائے (ثابت DfgConst& vtx : dfg.constVertices()) {
            ثابت V3Hash hash = vtx.عدد().مقصد_Hash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.قدر()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    سائز_t عامل()(Dfg_راس* vtxp) ثابت { واپس راس_Hash(*vtxp).قدر(); }

نجی:
    // Returns hash of vertex dependent on information internal to the vertex
    ساکن V3Hash راس_Self_Hash(ثابت Dfg_راس& vtx) {
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
        حالت VDfgType::پچھلا:
        حالت VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        حالت VDfgType::Sel: واپس V3Hash{vtx.as<DfgSel>()->lsb()};

        حالت VDfgType::SpliceArray:
        حالت VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<Dfg_راس_Splice>()->foreachDriver([&](ثابت Dfg_راس&, uint32_t lo) {
                hash += lo;
                واپس جھوٹ;
            });
            واپس hash;
        }

        // Vertices with no internal information
        حالت VDfgType::مطابقت_Masked:
        حالت VDfgType::Mux:
        حالت VDfgType::UnitArray: واپس V3Hash{};

        // Generated classes - none of them have internal information
        حالت VDfgType::جوڑو_2:
        حالت VDfgType::And:
        حالت VDfgType::ArraySel:
        حالت VDfgType::Concat:
        حالت VDfgType::Cond:
        حالت VDfgType::گنتی_Ones_2:
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
        حالت VDfgType::سرکاؤ_L:
        حالت VDfgType::سرکاؤ_R:
        حالت VDfgType::سرکاؤ_RS:
        حالت VDfgType::دھارا_L:
        حالت VDfgType::دھارا_R:
        حالت VDfgType::Sub:
        حالت VDfgType::Xor: واپس V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash راس_Hash(Dfg_راس& vtx) ثابت {
        V3Hash& نتیجہ = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        اگر (!نتیجہ.قدر()) {
            V3Hash hash{راس_Self_Hash(vtx)};
            hash += vtx.type();
            hash += vtx.سائز();
            vtx.foreach_ماخذ([&](Dfg_راس& src) {
                hash += راس_Hash(src);  // Graph is acyclic, so this terminates
                واپس جھوٹ;
            });
            نتیجہ = hash;
        }
        واپس نتیجہ;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
جماعت DfgCseEqual final {
    // TYPES
    استعمال راس_Pair = std::pair<ثابت Dfg_راس*, ثابت Dfg_راس*>;
    ساخت راس_Pair_Hash final {
        سائز_t عامل()(ثابت راس_Pair& pair) ثابت {
            V3Hash hash;
            hash += pair.پہلا;
            hash += pair.دوسرا;
            واپس hash.قدر();
        }
    };

    // STATE
    تغیرپذیر V3HashMap<راس_Pair, منطقی, راس_Pair_Hash> m_cache;  // Cache for vertex equality
    تغیرپذیر std::سمتیہ<uint32_t> m_driverLo;  // Low indices of drivers
    ثابت سائز_t m_سائز;  // Size of the graph

عوامی:
    // CONSTRUCTORS
    صریح DfgCseEqual(ثابت Dfg_گراف& dfg)
        : m_سائز{dfg.سائز()} {}

    // METHODS
    منطقی عامل()(Dfg_راس* ap, Dfg_راس* bp) ثابت { واپس راس_Equivalent(*ap, *bp); }

نجی:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    منطقی راس_Self_Equivalent(ثابت Dfg_راس& a, ثابت Dfg_راس& b) ثابت {
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
        حالت VDfgType::Const: واپس a.as<DfgConst>()->عدد().isCaseEq(b.as<DfgConst>()->عدد());
        حالت VDfgType::CReset: واپس جھوٹ;
        حالت VDfgType::پچھلا: واپس جھوٹ;

        حالت VDfgType::VarArray:
        حالت VDfgType::VarPacked:  // CSE does not combine variables
            واپس جھوٹ;

        // Vertices with internal information
        حالت VDfgType::Sel: واپس a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        حالت VDfgType::SpliceArray:
        حالت VDfgType::SplicePacked: {
            ثابت Dfg_راس_Splice* ثابت ap = a.as<Dfg_راس_Splice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ثابت Dfg_راس&, uint32_t lo) {
                m_driverLo.ڈالو_عقب(lo);
                واپس جھوٹ;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.ڈیٹا();
            واپس !b.as<Dfg_راس_Splice>()->foreachDriver([&](ثابت Dfg_راس&, uint32_t lo) {  //
                واپس *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        حالت VDfgType::مطابقت_Masked:
        حالت VDfgType::Mux:
        حالت VDfgType::UnitArray: واپس سچ;

        // Generated classes - none of them have internal information
        حالت VDfgType::جوڑو_2:
        حالت VDfgType::And:
        حالت VDfgType::ArraySel:
        حالت VDfgType::Concat:
        حالت VDfgType::Cond:
        حالت VDfgType::گنتی_Ones_2:
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
        حالت VDfgType::سرکاؤ_L:
        حالت VDfgType::سرکاؤ_R:
        حالت VDfgType::سرکاؤ_RS:
        حالت VDfgType::دھارا_L:
        حالت VDfgType::دھارا_R:
        حالت VDfgType::Sub:
        حالت VDfgType::Xor: واپس سچ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    منطقی sourcesEquivalent(ثابت Dfg_راس& a, ثابت Dfg_راس& b) ثابت {
        برائے (سائز_t i = 0; i < a.nInputs(); ++i) {
            ثابت Dfg_راس* ثابت ap = a.inputp(i);
            ثابت Dfg_راس* ثابت bp = b.inputp(i);
            اگر (!ap && !bp) جاری;
            اگر (!ap || !bp) واپس جھوٹ;
            اگر (!راس_Equivalent(*ap, *bp)) واپس جھوٹ;  // Graph is acyclic, so this terminates
        }
        واپس سچ;
    }

    // Compares 'a' and 'b' for equivalence
    منطقی راس_Equivalent(ثابت Dfg_راس& a, ثابت Dfg_راس& b) ثابت {
        // If same vertex, then equal
        اگر (&a == &b) واپس سچ;

        // If different type, then not equal
        اگر (a.type() != b.type()) واپس جھوٹ;

        // If different data type, then not equal
        اگر (a.dtype() != b.dtype()) واپس جھوٹ;

        // If different number of inputs, then not equal
        اگر (a.nInputs() != b.nInputs()) واپس جھوٹ;

        // Check vertex specifics
        اگر (!راس_Self_Equivalent(a, b)) واپس جھوٹ;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        اگر (!a.has_مضرب_Sinks() && !b.has_مضرب_Sinks()) واپس sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ثابت راس_Pair کلید = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ثابت خودکار it = m_cache.ڈھونڈو(کلید);
        اگر (it != m_cache.انتہا()) واپس it->دوسرا;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ثابت منطقی equal = sourcesEquivalent(a, b);
        اگر (VL_UNLIKELY(m_cache.خالی_خانے())) m_cache.reserve(m_سائز / 4);
        m_cache.داخل_کرو({کلید, equal});

        // The predicate result
        واپس equal;
    }
};

// Combine equivalent operation vertices
خالی dfgCseCombineEquivalent(Dfg_گراف& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    برائے (DfgConst* ثابت vtxp : dfg.constVertices().unlinkable()) {
        اگر (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3_Hash_سیٹ<Dfg_راس*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.سائز());

    // Combine operation vertices
    برائے (Dfg_راس* ثابت vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        اگر (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            جاری;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ثابت خودکار pair = uniqueVtxps.داخل_کرو(vtxp);
        اگر (!pair.دوسرا) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.پہلا);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

خالی V3DfgPasses::cse(Dfg_گراف& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::remove_غیر_مستعمل(dfg);
}
