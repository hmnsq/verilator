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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3HashTable.h"

VL_DEFINE_DEBUG_FUNCTIONS;

// Hash functor for V3HashSet - depends on vertex and all its inputs
తరగతి DfgCseHash final {
    // STATE
    మార్చదగిన DfgUserMap<V3Hash> m_cache;  // Cache for vertex hashes

బహిరంగ:
    // CONSTRUCTOR
    స్పష్టమైన DfgCseHash(Dfg_లేఖాచిత్రం& dfg)
        : m_cache{dfg.makeUserMap<V3Hash>()} {
        // Pre-hash variables, these are all unique, so just set their hash to a unique value
        uint32_t fixedHash = 0;
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Var& vtx : dfg.varVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash Ast references, these are all unique like variables
        కొరకు (స్థిరరాశి Dfg_శీర్షం_Ast& vtx : dfg.astVertices()) m_cache[vtx] = V3Hash{++fixedHash};
        // Pre-hash CReset and Prev vertices, these are all unique
        కొరకు (స్థిరరాశి Dfg_శీర్షం& vtx : dfg.opVertices()) {
            ఒకవేళ (vtx.is<DfgCReset>() || vtx.is<DfgPrev>()) m_cache[vtx] = V3Hash{++fixedHash};
        }
        // Similarly pre-hash constants for speed. While we don't combine constants, we do want
        // expressions using the same constants to be combined, so we do need to hash equal
        // constants to equal values.
        ++fixedHash;
        కొరకు (స్థిరరాశి DfgConst& vtx : dfg.constVertices()) {
            స్థిరరాశి V3Hash hash = vtx.అంకె().గమ్యం_Hash() + fixedHash;
            // Technically possible for a hash to be zero, 'vertexSelfHash' assumes it isn't
            m_cache[vtx] = VL_LIKELY(hash.విలువ()) ? hash : V3Hash{1};
        }
    }

    // METHODS
    కొలత_t క్రియాకారకం()(Dfg_శీర్షం* vtxp) స్థిరరాశి { తిరిగి శీర్షం_Hash(*vtxp).విలువ(); }

అంతరంగ:
    // Returns hash of vertex dependent on information internal to the vertex
    స్థిర V3Hash శీర్షం_Self_Hash(స్థిరరాశి Dfg_శీర్షం& vtx) {
        ఎంపిక (vtx.type()) {
        // Unhandled vertices
        స్థితి VDfgType::Logic:  // LCOV_EXCL_START
        స్థితి VDfgType::Unresolved:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Should not have reached CSE");

        // Special vertices
        స్థితి VDfgType::Const:  // LCOV_EXCL_START
        స్థితి VDfgType::CReset:
        స్థితి VDfgType::VarArray:
        స్థితి VDfgType::VarPacked:
        స్థితి VDfgType::Prev:
        స్థితి VDfgType::AstRd:  // LCOV_EXCL_STOP
            vtx.v3fatalSrc("Hash should have been pre-computed");

        // Vertices with internal information
        స్థితి VDfgType::Sel: తిరిగి V3Hash{vtx.as<DfgSel>()->lsb()};

        స్థితి VDfgType::SpliceArray:
        స్థితి VDfgType::SplicePacked: {
            V3Hash hash;
            vtx.as<Dfg_శీర్షం_Splice>()->foreachDriver([&](స్థిరరాశి Dfg_శీర్షం&, uint32_t lo) {
                hash += lo;
                తిరిగి అబద్ధం;
            });
            తిరిగి hash;
        }

        // Vertices with no internal information
        స్థితి VDfgType::సరిపోలిక_Masked:
        స్థితి VDfgType::Mux:
        స్థితి VDfgType::UnitArray: తిరిగి V3Hash{};

        // Generated classes - none of them have internal information
        స్థితి VDfgType::కూడు_2:
        స్థితి VDfgType::And:
        స్థితి VDfgType::ArraySel:
        స్థితి VDfgType::Concat:
        స్థితి VDfgType::Cond:
        స్థితి VDfgType::లెక్క_Ones_2:
        స్థితి VDfgType::Div:
        స్థితి VDfgType::DivS:
        స్థితి VDfgType::Eq:
        స్థితి VDfgType::EqCase:
        స్థితి VDfgType::EqWild:
        స్థితి VDfgType::Extend:
        స్థితి VDfgType::ExtendS:
        స్థితి VDfgType::Gt:
        స్థితి VDfgType::GtS:
        స్థితి VDfgType::Gte:
        స్థితి VDfgType::GteS:
        స్థితి VDfgType::LogAnd:
        స్థితి VDfgType::LogEq:
        స్థితి VDfgType::LogIf:
        స్థితి VDfgType::LogNot:
        స్థితి VDfgType::LogOr:
        స్థితి VDfgType::Lt:
        స్థితి VDfgType::LtS:
        స్థితి VDfgType::Lte:
        స్థితి VDfgType::LteS:
        స్థితి VDfgType::ModDiv:
        స్థితి VDfgType::ModDivS:
        స్థితి VDfgType::Mul:
        స్థితి VDfgType::MulS:
        స్థితి VDfgType::Negate:
        స్థితి VDfgType::Neq:
        స్థితి VDfgType::NeqCase:
        స్థితి VDfgType::NeqWild:
        స్థితి VDfgType::Not:
        స్థితి VDfgType::OneHot:
        స్థితి VDfgType::OneHot0:
        స్థితి VDfgType::Or:
        స్థితి VDfgType::Pow:
        స్థితి VDfgType::PowSS:
        స్థితి VDfgType::PowSU:
        స్థితి VDfgType::PowUS:
        స్థితి VDfgType::RedAnd:
        స్థితి VDfgType::RedOr:
        స్థితి VDfgType::RedXor:
        స్థితి VDfgType::Rep:
        స్థితి VDfgType::జరుపు_L:
        స్థితి VDfgType::జరుపు_R:
        స్థితి VDfgType::జరుపు_RS:
        స్థితి VDfgType::ప్రవాహం_L:
        స్థితి VDfgType::ప్రవాహం_R:
        స్థితి VDfgType::Sub:
        స్థితి VDfgType::Xor: తిరిగి V3Hash{};
        }
        VL_UNREACHABLE;
    }

    // Returns hash of vertex dependent on itself and all its inputs - memoized
    V3Hash శీర్షం_Hash(Dfg_శీర్షం& vtx) స్థిరరాశి {
        V3Hash& ఫలితం = m_cache[vtx];
        // Technically possible for a hash to be zero, but rare, so assume 0 means uninitialized
        ఒకవేళ (!ఫలితం.విలువ()) {
            V3Hash hash{శీర్షం_Self_Hash(vtx)};
            hash += vtx.type();
            hash += vtx.కొలత();
            vtx.foreach_ఆధారం([&](Dfg_శీర్షం& src) {
                hash += శీర్షం_Hash(src);  // Graph is acyclic, so this terminates
                తిరిగి అబద్ధం;
            });
            ఫలితం = hash;
        }
        తిరిగి ఫలితం;
    }
};

// Equal functor for V3HashSet - depends on vertex and all its inputs
తరగతి DfgCseEqual final {
    // TYPES
    ఉపయోగించి శీర్షం_Pair = std::pair<స్థిరరాశి Dfg_శీర్షం*, స్థిరరాశి Dfg_శీర్షం*>;
    నిర్మాణం శీర్షం_Pair_Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి శీర్షం_Pair& pair) స్థిరరాశి {
            V3Hash hash;
            hash += pair.మొదటి;
            hash += pair.రెండవ;
            తిరిగి hash.విలువ();
        }
    };

    // STATE
    మార్చదగిన V3HashMap<శీర్షం_Pair, తార్కిక, శీర్షం_Pair_Hash> m_cache;  // Cache for vertex equality
    మార్చదగిన std::సదిశ<uint32_t> m_driverLo;  // Low indices of drivers
    స్థిరరాశి కొలత_t m_కొలత;  // Size of the graph

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన DfgCseEqual(స్థిరరాశి Dfg_లేఖాచిత్రం& dfg)
        : m_కొలత{dfg.కొలత()} {}

    // METHODS
    తార్కిక క్రియాకారకం()(Dfg_శీర్షం* ap, Dfg_శీర్షం* bp) స్థిరరాశి { తిరిగి శీర్షం_Equivalent(*ap, *bp); }

అంతరంగ:
    // Compare 'a' and 'b' for equivalence based on their internal information only
    తార్కిక శీర్షం_Self_Equivalent(స్థిరరాశి Dfg_శీర్షం& a, స్థిరరాశి Dfg_శీర్షం& b) స్థిరరాశి {
        // Note: 'a' and 'b' are of the same Vertex type, data type, and have
        // the same number of inputs with matching types. This is established
        // by 'vertexEquivalent'.
        ఎంపిక (a.type()) {
        // Unhandled vertices
        స్థితి VDfgType::Logic:  // LCOV_EXCL_START
        స్థితి VDfgType::Unresolved:  // LCOV_EXCL_STOP
            a.v3fatalSrc("Should not have reached CSE");

        // Not reachable via operation vertices
        స్థితి VDfgType::AstRd:  // LCOV_EXCL_LINE
            a.v3fatalSrc("Should not be reachable via operation vertices");

        // Special vertices
        స్థితి VDfgType::Const: తిరిగి a.as<DfgConst>()->అంకె().isCaseEq(b.as<DfgConst>()->అంకె());
        స్థితి VDfgType::CReset: తిరిగి అబద్ధం;
        స్థితి VDfgType::Prev: తిరిగి అబద్ధం;

        స్థితి VDfgType::VarArray:
        స్థితి VDfgType::VarPacked:  // CSE does not combine variables
            తిరిగి అబద్ధం;

        // Vertices with internal information
        స్థితి VDfgType::Sel: తిరిగి a.as<DfgSel>()->lsb() == b.as<DfgSel>()->lsb();

        స్థితి VDfgType::SpliceArray:
        స్థితి VDfgType::SplicePacked: {
            స్థిరరాశి Dfg_శీర్షం_Splice* స్థిరరాశి ap = a.as<Dfg_శీర్షం_Splice>();
            // Gather indices of drivers of 'a'
            m_driverLo.clear();
            m_driverLo.reserve(ap->nInputs());
            ap->foreachDriver([&](స్థిరరాశి Dfg_శీర్షం&, uint32_t lo) {
                m_driverLo.నెట్టు_వెనుక(lo);
                తిరిగి అబద్ధం;
            });
            // Compare indices of drivers of 'b', equal if all match
            uint32_t* aLop = m_driverLo.దత్తాంశం();
            తిరిగి !b.as<Dfg_శీర్షం_Splice>()->foreachDriver([&](స్థిరరాశి Dfg_శీర్షం&, uint32_t lo) {  //
                తిరిగి *aLop++ != lo;
            });
        }

        // Vertices with no internal information
        స్థితి VDfgType::సరిపోలిక_Masked:
        స్థితి VDfgType::Mux:
        స్థితి VDfgType::UnitArray: తిరిగి నిజం;

        // Generated classes - none of them have internal information
        స్థితి VDfgType::కూడు_2:
        స్థితి VDfgType::And:
        స్థితి VDfgType::ArraySel:
        స్థితి VDfgType::Concat:
        స్థితి VDfgType::Cond:
        స్థితి VDfgType::లెక్క_Ones_2:
        స్థితి VDfgType::Div:
        స్థితి VDfgType::DivS:
        స్థితి VDfgType::Eq:
        స్థితి VDfgType::EqCase:
        స్థితి VDfgType::EqWild:
        స్థితి VDfgType::Extend:
        స్థితి VDfgType::ExtendS:
        స్థితి VDfgType::Gt:
        స్థితి VDfgType::GtS:
        స్థితి VDfgType::Gte:
        స్థితి VDfgType::GteS:
        స్థితి VDfgType::LogAnd:
        స్థితి VDfgType::LogEq:
        స్థితి VDfgType::LogIf:
        స్థితి VDfgType::LogNot:
        స్థితి VDfgType::LogOr:
        స్థితి VDfgType::Lt:
        స్థితి VDfgType::LtS:
        స్థితి VDfgType::Lte:
        స్థితి VDfgType::LteS:
        స్థితి VDfgType::ModDiv:
        స్థితి VDfgType::ModDivS:
        స్థితి VDfgType::Mul:
        స్థితి VDfgType::MulS:
        స్థితి VDfgType::Negate:
        స్థితి VDfgType::Neq:
        స్థితి VDfgType::NeqCase:
        స్థితి VDfgType::NeqWild:
        స్థితి VDfgType::Not:
        స్థితి VDfgType::OneHot:
        స్థితి VDfgType::OneHot0:
        స్థితి VDfgType::Or:
        స్థితి VDfgType::Pow:
        స్థితి VDfgType::PowSS:
        స్థితి VDfgType::PowSU:
        స్థితి VDfgType::PowUS:
        స్థితి VDfgType::RedAnd:
        స్థితి VDfgType::RedOr:
        స్థితి VDfgType::RedXor:
        స్థితి VDfgType::Rep:
        స్థితి VDfgType::జరుపు_L:
        స్థితి VDfgType::జరుపు_R:
        స్థితి VDfgType::జరుపు_RS:
        స్థితి VDfgType::ప్రవాహం_L:
        స్థితి VDfgType::ప్రవాహం_R:
        స్థితి VDfgType::Sub:
        స్థితి VDfgType::Xor: తిరిగి నిజం;
        }
        VL_UNREACHABLE;
    }

    // Compares the sources of 'a' and 'b' for equivalence
    తార్కిక sourcesEquivalent(స్థిరరాశి Dfg_శీర్షం& a, స్థిరరాశి Dfg_శీర్షం& b) స్థిరరాశి {
        కొరకు (కొలత_t i = 0; i < a.nInputs(); ++i) {
            స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి ap = a.inputp(i);
            స్థిరరాశి Dfg_శీర్షం* స్థిరరాశి bp = b.inputp(i);
            ఒకవేళ (!ap && !bp) కొనసాగు;
            ఒకవేళ (!ap || !bp) తిరిగి అబద్ధం;
            ఒకవేళ (!శీర్షం_Equivalent(*ap, *bp)) తిరిగి అబద్ధం;  // Graph is acyclic, so this terminates
        }
        తిరిగి నిజం;
    }

    // Compares 'a' and 'b' for equivalence
    తార్కిక శీర్షం_Equivalent(స్థిరరాశి Dfg_శీర్షం& a, స్థిరరాశి Dfg_శీర్షం& b) స్థిరరాశి {
        // If same vertex, then equal
        ఒకవేళ (&a == &b) తిరిగి నిజం;

        // If different type, then not equal
        ఒకవేళ (a.type() != b.type()) తిరిగి అబద్ధం;

        // If different data type, then not equal
        ఒకవేళ (a.dtype() != b.dtype()) తిరిగి అబద్ధం;

        // If different number of inputs, then not equal
        ఒకవేళ (a.nInputs() != b.nInputs()) తిరిగి అబద్ధం;

        // Check vertex specifics
        ఒకవేళ (!శీర్షం_Self_Equivalent(a, b)) తిరిగి అబద్ధం;

        // A given pair can only be reached more than once if one of the
        // vertices has multiple sinks, or if there was a hash collision.
        // Collisions are rare, so only memoize the result if it can actually
        // be looked up again through multiple paths.
        ఒకవేళ (!a.has_గుణిజం_Sinks() && !b.has_గుణిజం_Sinks()) తిరిగి sourcesEquivalent(a, b);

        // Need to compare the source vertices, check memo
        స్థిరరాశి శీర్షం_Pair కీలకం = (&a < &b) ? std::make_pair(&a, &b) : std::make_pair(&b, &a);
        స్థిరరాశి స్వయంచాలక it = m_cache.కనుగొను(కీలకం);
        ఒకవేళ (it != m_cache.చివర()) తిరిగి it->రెండవ;

        // Not memoized yet, so compute and memoize, reserve table on first insert
        స్థిరరాశి తార్కిక equal = sourcesEquivalent(a, b);
        ఒకవేళ (VL_UNLIKELY(m_cache.ఖాళీలు())) m_cache.reserve(m_కొలత / 4);
        m_cache.చొప్పించు({కీలకం, equal});

        // The predicate result
        తిరిగి equal;
    }
};

// Combine equivalent operation vertices
శూన్య dfgCseCombineEquivalent(Dfg_లేఖాచిత్రం& dfg, V3DfgCseContext& ctx) {
    // Delete unused constants, so the pre-hashing below need not consider them
    కొరకు (DfgConst* స్థిరరాశి vtxp : dfg.constVertices().unlinkable()) {
        ఒకవేళ (!vtxp->hasSinks()) VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
    }

    // Set of unique vertices. This set does all the work identifying equivalent vertices.
    V3_Hash_సమితి<Dfg_శీర్షం*, DfgCseHash, DfgCseEqual> uniqueVtxps{DfgCseHash{dfg}, DfgCseEqual{dfg}};
    // There is at most one entry per vertex
    uniqueVtxps.reserve(dfg.కొలత());

    // Combine operation vertices
    కొరకు (Dfg_శీర్షం* స్థిరరాశి vtxp : dfg.opVertices().unlinkable()) {
        // Delete unused nodes while we are at it.
        ఒకవేళ (!vtxp->hasSinks()) {
            vtxp->unlinkDelete(dfg);
            కొనసాగు;
        }
        // Insert the vertex into the set, if an equivalent is found, replace the vertex with it
        స్థిరరాశి స్వయంచాలక pair = uniqueVtxps.చొప్పించు(vtxp);
        ఒకవేళ (!pair.రెండవ) {
            ++ctx.m_eliminated;
            vtxp->replaceWith(*pair.మొదటి);
            VL_DO_DANGLING(vtxp->unlinkDelete(dfg), vtxp);
        }
    }
}

శూన్య V3DfgPasses::cse(Dfg_లేఖాచిత్రం& dfg, V3DfgCseContext& ctx) {
    dfgCseCombineEquivalent(dfg, ctx);
    V3DfgPasses::remove_వాడని(dfg);
}
