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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
ವರ್ಗ DfgCseHash final {
    // STATE
    ಬದಲಾಯಿಸಬಹುದಾದ DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಸ್ಪಷ್ಟ DfgCseHash(Dfg_ನಕ್ಷೆ& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Var& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Ast& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx : dfg.opVertices()) {
            ಆದರೆ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ DfgConst& vtx : dfg.constVertices()) {
            ಸ್ಥಿರಾಂಕ V3Hash hash = vtx.ಅಂಕೆ().ಗಮ್ಯ_Hash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.ಮೌಲ್ಯ()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(Dfg_ಶೃಂಗ* vtxp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಶೃಂಗ_Hash(*vtxp).ಮೌಲ್ಯ(); }

ಖಾಸಗಿ:
    // Returns hash of vertex dependent on information internal to the vertex
    ಸ್ಥಿರ V3Hash ಶೃಂಗ_Self_Hash(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
        ಆಯ್ಕೆ (vtx.type()) {
        // Unhandled vertices
        ಸ್ಥಿತಿ VDfgType::Logic:  // LCOV_EXCL_START
        ಸ್ಥಿತಿ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        ಸ್ಥಿತಿ VDfgType::Const:  // LCOV_EXCL_START
        ಸ್ಥಿತಿ VDfgType::CReset:
        ಸ್ಥಿತಿ VDfgType::VarArray:
        ಸ್ಥಿತಿ VDfgType::VarPacked:
        ಸ್ಥಿತಿ VDfgType::Prev:
        ಸ್ಥಿತಿ VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        ಸ್ಥಿತಿ VDfgType::Sel: ಹಿಂತಿರುಗಿ V3Hash{vtx.as<DfgSel>()->lsb()};

        ಸ್ಥಿತಿ VDfgType::SpliceArray:
        ಸ್ಥಿತಿ VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<Dfg_ಶೃಂಗ_Splice>()->foreachDriver([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&, uint32_t lo) {
                hash += lo;
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಹಿಂತಿರುಗಿ hash;
        }

        // Vertices with no internal information
        ಸ್ಥಿತಿ VDfgType::ಹೊಂದಿಕೆ_Masked:
        ಸ್ಥಿತಿ VDfgType::Mux:
        ಸ್ಥಿತಿ VDfgType::UnitArray: ಹಿಂತಿರುಗಿ V3Hash{};

        // Generated classes - none of them have internal information
        ಸ್ಥಿತಿ VDfgType::ಕೂಡಿಸು_2:
        ಸ್ಥಿತಿ VDfgType::And:
        ಸ್ಥಿತಿ VDfgType::ArraySel:
        ಸ್ಥಿತಿ VDfgType::Concat:
        ಸ್ಥಿತಿ VDfgType::Cond:
        ಸ್ಥಿತಿ VDfgType::ಎಣಿಕೆ_Ones_2:
        ಸ್ಥಿತಿ VDfgType::Div:
        ಸ್ಥಿತಿ VDfgType::DivS:
        ಸ್ಥಿತಿ VDfgType::Eq:
        ಸ್ಥಿತಿ VDfgType::EqCase:
        ಸ್ಥಿತಿ VDfgType::EqWild:
        ಸ್ಥಿತಿ VDfgType::Extend:
        ಸ್ಥಿತಿ VDfgType::ExtendS:
        ಸ್ಥಿತಿ VDfgType::Gt:
        ಸ್ಥಿತಿ VDfgType::GtS:
        ಸ್ಥಿತಿ VDfgType::Gte:
        ಸ್ಥಿತಿ VDfgType::GteS:
        ಸ್ಥಿತಿ VDfgType::LogAnd:
        ಸ್ಥಿತಿ VDfgType::LogEq:
        ಸ್ಥಿತಿ VDfgType::LogIf:
        ಸ್ಥಿತಿ VDfgType::LogNot:
        ಸ್ಥಿತಿ VDfgType::LogOr:
        ಸ್ಥಿತಿ VDfgType::Lt:
        ಸ್ಥಿತಿ VDfgType::LtS:
        ಸ್ಥಿತಿ VDfgType::Lte:
        ಸ್ಥಿತಿ VDfgType::LteS:
        ಸ್ಥಿತಿ VDfgType::ModDiv:
        ಸ್ಥಿತಿ VDfgType::ModDivS:
        ಸ್ಥಿತಿ VDfgType::Mul:
        ಸ್ಥಿತಿ VDfgType::MulS:
        ಸ್ಥಿತಿ VDfgType::Negate:
        ಸ್ಥಿತಿ VDfgType::Neq:
        ಸ್ಥಿತಿ VDfgType::NeqCase:
        ಸ್ಥಿತಿ VDfgType::NeqWild:
        ಸ್ಥಿತಿ VDfgType::Not:
        ಸ್ಥಿತಿ VDfgType::OneHot:
        ಸ್ಥಿತಿ VDfgType::OneHot0:
        ಸ್ಥಿತಿ VDfgType::Or:
        ಸ್ಥಿತಿ VDfgType::Pow:
        ಸ್ಥಿತಿ VDfgType::PowSS:
        ಸ್ಥಿತಿ VDfgType::PowSU:
        ಸ್ಥಿತಿ VDfgType::PowUS:
        ಸ್ಥಿತಿ VDfgType::RedAnd:
        ಸ್ಥಿತಿ VDfgType::RedOr:
        ಸ್ಥಿತಿ VDfgType::RedXor:
        ಸ್ಥಿತಿ VDfgType::Rep:
        ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_L:
        ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_R:
        ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_RS:
        ಸ್ಥಿತಿ VDfgType::ಹರಿವು_L:
        ಸ್ಥಿತಿ VDfgType::ಹರಿವು_R:
        ಸ್ಥಿತಿ VDfgType::Sub:
        ಸ್ಥಿತಿ VDfgType::Xor: ಹಿಂತಿರುಗಿ V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash ಶೃಂಗ_Hash(Dfg_ಶೃಂಗ& vtx) ಸ್ಥಿರಾಂಕ {
        V3Hash& ಫಲಿತಾಂಶ = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ಆದರೆ (!ಫಲಿತಾಂಶ.ಮೌಲ್ಯ()) {
            V3Hash hash{ಶೃಂಗ_Self_Hash(vtx)};
            hash += vtx.type();
            hash += vtx.ಗಾತ್ರ_2();
            vtx.foreach_ಮೂಲ([&](Dfg_ಶೃಂಗ& src) {
                hash += ಶೃಂಗ_Hash(src);  // Graph is acyclic, so this terminates
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            ಫಲಿತಾಂಶ = hash;
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
ವರ್ಗ DfgCseEqual final {
    // TYPES
    ಬಳಸಿ ಶೃಂಗ_Pair = std::pair<ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ*>;
    ರಚನೆ ಶೃಂಗ_Pair_Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಶೃಂಗ_Pair& pair) ಸ್ಥಿರಾಂಕ {
            V3Hash hash;
            hash += pair.ಮೊದಲ;
            hash += pair.ಎರಡನೇ;
            ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
        }
    };

    // STATE
    ಬದಲಾಯಿಸಬಹುದಾದ V3HashMap<ಶೃಂಗ_Pair, ತಾರ್ಕಿಕ, ಶೃಂಗ_Pair_Hash> m_cache;  // Cache for vertex equality
    ಬದಲಾಯಿಸಬಹುದಾದ std::ಸದಿಶ<uint32_t> m_driverLo;  // Low indices of drivers
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ಗಾತ್ರ;  // Size of the graph

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ DfgCseEqual(ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg)
        : m_ಗಾತ್ರ{dfg.ಗಾತ್ರ_2()} {}

    // METHODS
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(Dfg_ಶೃಂಗ* ap, Dfg_ಶೃಂಗ* bp) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಶೃಂಗ_Equivalent(*ap, *bp); }

ಖಾಸಗಿ:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    ತಾರ್ಕಿಕ ಶೃಂಗ_Self_Equivalent(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& a, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& b) ಸ್ಥಿರಾಂಕ {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        ಆಯ್ಕೆ (a.type()) {
        // Unhandled vertices
        ಸ್ಥಿತಿ VDfgType::Logic:  // LCOV_EXCL_START
        ಸ್ಥಿತಿ VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        ಸ್ಥಿತಿ VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        ಸ್ಥಿತಿ VDfgType::Const: ಹಿಂತಿರುಗಿ a.as<DfgConst>()->ಅಂಕೆ().isCaseEq(b.as<DfgConst>()->ಅಂಕೆ());
        ಸ್ಥಿತಿ VDfgType::CReset: ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಸ್ಥಿತಿ VDfgType::Prev: ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        ಸ್ಥಿತಿ VDfgType::VarArray:
        ಸ್ಥಿತಿ VDfgType::VarPacked:  // CSE does not combine variables
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Vertices with internal information
        ಸ್ಥಿತಿ VDfgType::Sel: ಹಿಂತಿರುಗಿ a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        ಸ್ಥಿತಿ VDfgType::SpliceArray:
        ಸ್ಥಿತಿ VDfgType::SplicePacked: {
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ_Splice* ಸ್ಥಿರಾಂಕ ap = a.as<Dfg_ಶೃಂಗ_Splice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&, uint32_t lo) {
                m_driverLo.ತಳ್ಳು_ಹಿಂಭಾಗ(lo);
                ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.ದತ್ತಾಂಶ();
            ಹಿಂತಿರುಗಿ !b.as<Dfg_ಶೃಂಗ_Splice>()->foreachDriver([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ&, uint32_t lo) {  //
                ಹಿಂತಿರುಗಿ *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        ಸ್ಥಿತಿ VDfgType::ಹೊಂದಿಕೆ_Masked:
        ಸ್ಥಿತಿ VDfgType::Mux:
        ಸ್ಥಿತಿ VDfgType::UnitArray: ಹಿಂತಿರುಗಿ ನಿಜ;

        // Generated classes - none of them have internal information
        ಸ್ಥಿತಿ VDfgType::ಕೂಡಿಸು_2:
        ಸ್ಥಿತಿ VDfgType::And:
        ಸ್ಥಿತಿ VDfgType::ArraySel:
        ಸ್ಥಿತಿ VDfgType::Concat:
        ಸ್ಥಿತಿ VDfgType::Cond:
        ಸ್ಥಿತಿ VDfgType::ಎಣಿಕೆ_Ones_2:
        ಸ್ಥಿತಿ VDfgType::Div:
        ಸ್ಥಿತಿ VDfgType::DivS:
        ಸ್ಥಿತಿ VDfgType::Eq:
        ಸ್ಥಿತಿ VDfgType::EqCase:
        ಸ್ಥಿತಿ VDfgType::EqWild:
        ಸ್ಥಿತಿ VDfgType::Extend:
        ಸ್ಥಿತಿ VDfgType::ExtendS:
        ಸ್ಥಿತಿ VDfgType::Gt:
        ಸ್ಥಿತಿ VDfgType::GtS:
        ಸ್ಥಿತಿ VDfgType::Gte:
        ಸ್ಥಿತಿ VDfgType::GteS:
        ಸ್ಥಿತಿ VDfgType::LogAnd:
        ಸ್ಥಿತಿ VDfgType::LogEq:
        ಸ್ಥಿತಿ VDfgType::LogIf:
        ಸ್ಥಿತಿ VDfgType::LogNot:
        ಸ್ಥಿತಿ VDfgType::LogOr:
        ಸ್ಥಿತಿ VDfgType::Lt:
        ಸ್ಥಿತಿ VDfgType::LtS:
        ಸ್ಥಿತಿ VDfgType::Lte:
        ಸ್ಥಿತಿ VDfgType::LteS:
        ಸ್ಥಿತಿ VDfgType::ModDiv:
        ಸ್ಥಿತಿ VDfgType::ModDivS:
        ಸ್ಥಿತಿ VDfgType::Mul:
        ಸ್ಥಿತಿ VDfgType::MulS:
        ಸ್ಥಿತಿ VDfgType::Negate:
        ಸ್ಥಿತಿ VDfgType::Neq:
        ಸ್ಥಿತಿ VDfgType::NeqCase:
        ಸ್ಥಿತಿ VDfgType::NeqWild:
        ಸ್ಥಿತಿ VDfgType::Not:
        ಸ್ಥಿತಿ VDfgType::OneHot:
        ಸ್ಥಿತಿ VDfgType::OneHot0:
        ಸ್ಥಿತಿ VDfgType::Or:
        ಸ್ಥಿತಿ VDfgType::Pow:
        ಸ್ಥಿತಿ VDfgType::PowSS:
        ಸ್ಥಿತಿ VDfgType::PowSU:
        ಸ್ಥಿತಿ VDfgType::PowUS:
        ಸ್ಥಿತಿ VDfgType::RedAnd:
        ಸ್ಥಿತಿ VDfgType::RedOr:
        ಸ್ಥಿತಿ VDfgType::RedXor:
        ಸ್ಥಿತಿ VDfgType::Rep:
        ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_L:
        ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_R:
        ಸ್ಥಿತಿ VDfgType::ಸರಿಸು_RS:
        ಸ್ಥಿತಿ VDfgType::ಹರಿವು_L:
        ಸ್ಥಿತಿ VDfgType::ಹರಿವು_R:
        ಸ್ಥಿತಿ VDfgType::Sub:
        ಸ್ಥಿತಿ VDfgType::Xor: ಹಿಂತಿರುಗಿ ನಿಜ;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    ತಾರ್ಕಿಕ sourcesEquivalent(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& a, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& b) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < a.nInputs(); ++i) {
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ap = a.inputp(i);
            ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ bp = b.inputp(i);
            ಆದರೆ (!ap && !bp) ಮುಂದುವರಿ;
            ಆದರೆ (!ap || !bp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಆದರೆ (!ಶೃಂಗ_Equivalent(*ap, *bp)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // Graph is acyclic, so this terminates
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Compares 'a' and 'b' for equivalence
    ತಾರ್ಕಿಕ ಶೃಂಗ_Equivalent(ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& a, ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& b) ಸ್ಥಿರಾಂಕ {
        // If same vertex, then equal
        ಆದರೆ (&a == &b) ಹಿಂತಿರುಗಿ ನಿಜ;

        // If different type, then not equal
        ಆದರೆ (a.type() != b.type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // If different data type, then not equal
        ಆದರೆ (a.dtype() != b.dtype()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // If different number of inputs, then not equal
        ಆದರೆ (a.nInputs() != b.nInputs()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // Check vertex specifics
        ಆದರೆ (!ಶೃಂಗ_Self_Equivalent(a, b)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ಆದರೆ (!a.has_ಅಪವರ್ತ್ಯ_Sinks() && !b.has_ಅಪವರ್ತ್ಯ_Sinks()) ಹಿಂತಿರುಗಿ sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        ಸ್ಥಿರಾಂಕ ಶೃಂಗ_Pair ಕೀಲಿ = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_cache.ಕಂಡುಹಿಡಿ(ಕೀಲಿ);
        ಆದರೆ (it != m_cache.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ equal = sourcesEquivalent(a, b);
        ಆದರೆ (VL_UNLIKELY(m_cache.ಖಾಲಿಗಳು())) m_cache.reserve(m_ಗಾತ್ರ / 4);
        m_cache.ಸೇರಿಸು({ಕೀಲಿ, equal});

        // The predicate result
        ಹಿಂತಿರುಗಿ equal;
    }
};

// Combine equivalent operation vertices
ಶೂನ್ಯ dfgCseCombineEquivalent(Dfg_ನಕ್ಷೆ& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    ಗಾಗಿ (DfgConst* ಸ್ಥಿರಾಂಕ vtxp : dfg.constVertices().unlinkable()) {
        ಆದರೆ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3_Hash_ಗಣ<Dfg_ಶೃಂಗ*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.ಗಾತ್ರ_2());

    // Combine operation vertices
    ಗಾಗಿ (Dfg_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ಆದರೆ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            ಮುಂದುವರಿ;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = uniqueVtxps.ಸೇರಿಸು(vtxp);
        ಆದರೆ (!pair.ಎರಡನೇ) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.ಮೊದಲ);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

ಶೂನ್ಯ V3DfgPasses::cse(Dfg_ನಕ್ಷೆ& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::remove_ಬಳಸದ(dfg);
}
