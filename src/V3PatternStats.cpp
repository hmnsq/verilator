// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dump data structure pattern frequencies for analysis
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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstPatterns.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

ਵਰਗ V3_ਨਮੂਨਾ_Stats VL_NOT_FINAL {
ਜਨਤਕ:
    ਸਥਿਰ constexpr uint32_t MIN_ਨਮੂਨਾ_ਡੂੰਘਾਈ = 1;
    ਸਥਿਰ constexpr uint32_t MAX_ਨਮੂਨਾ_ਡੂੰਘਾਈ = 4;

ਨਿਜੀ:
    ਅਚਲ std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::ਸਦਿਸ਼<std::unordered_map<std::string, ਮਾਪ_t>> m_ਨਮੂਨਾ_Counts{MAX_ਨਮੂਨਾ_ਡੂੰਘਾਈ + 1};

    ਖਾਲੀ dump(std::ostream& os) {
        ਵਰਤੋਂ ਸਤਰ_2 = std::pair<std::string, ਮਾਪ_t>;
        ਲਈ (uint32_t i = MIN_ਨਮੂਨਾ_ਡੂੰਘਾਈ; i <= MAX_ਨਮੂਨਾ_ਡੂੰਘਾਈ; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            ਸਵੈਚਾਲਿਤ& ਨਮੂਨਾ_Counts = m_ਨਮੂਨਾ_Counts[i];

            // Remove patterns also present at shallower depths
            ਲਈ (uint32_t j = MIN_ਨਮੂਨਾ_ਡੂੰਘਾਈ; j < i; ++j) {
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_ਨਮੂਨਾ_Counts[j]) ਨਮੂਨਾ_Counts.erase(pair.ਪਹਿਲਾ);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::ਸਦਿਸ਼<ਸਤਰ_2> ਸਤਰਾਂ;
            ਸਤਰਾਂ.reserve(ਨਮੂਨਾ_Counts.ਮਾਪ());
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : ਨਮੂਨਾ_Counts) ਸਤਰਾਂ.emplace_ਪਿੱਠ(pair);
            std::ਛਾਂਟੋ(ਸਤਰਾਂ.begin(), ਸਤਰਾਂ.ਸਿਰਾ(), [](ਅਚਲ ਸਤਰ_2& a, ਅਚਲ ਸਤਰ_2& b) {
                ਜੇ (a.ਦੂਜਾ != b.ਦੂਜਾ) ਵਾਪਸ a.ਦੂਜਾ > b.ਦੂਜਾ;
                ਵਾਪਸ a.ਪਹਿਲਾ < b.ਪਹਿਲਾ;
            });

            // Print each pattern
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਸਤਰ : ਸਤਰਾਂ) {
                os << ' ' << std::setw(12) << std::ਸੱਜੇ << ਸਤਰ.ਦੂਜਾ;
                os << ' ' << std::ਖੱਬੇ << ਸਤਰ.ਪਹਿਲਾ << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

ਜਨਤਕ:
    V3_ਨਮੂਨਾ_Stats(ਅਚਲ std::string& what)
        : m_what{what} {}
    ~V3_ਨਮੂਨਾ_Stats() = ਮੂਲ;

    ਖਾਲੀ accumulate(ਅਚਲ std::string& ਨਮੂਨਾ_2, uint32_t ਡੂੰਘਾਈ) {
        m_ਨਮੂਨਾ_Counts[ਡੂੰਘਾਈ][ਨਮੂਨਾ_2] += 1;
    }

    ਖਾਲੀ dump_ਮੰਜ਼ਿਲ_File(ਅਚਲ std::string& filename) {
        // Open, write, close
        ਅਚਲ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        ਜੇ (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

ਖਾਲੀ V3AstPatterns::dumpAll(ਅਚਲ AstNetlist* nodep, ਅਚਲ std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3_ਨਮੂਨਾ_Stats ਨਮੂਨਾ_Stats{"AST"};
    nodep->foreach([&](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਲਈ (uint32_t i = V3_ਨਮੂਨਾ_Stats::MIN_ਨਮੂਨਾ_ਡੂੰਘਾਈ;
             i <= V3_ਨਮੂਨਾ_Stats::MAX_ਨਮੂਨਾ_ਡੂੰਘਾਈ; ++i) {
            ਨਮੂਨਾ_Stats.accumulate(exprp->ਨਮੂਨਾ_String(i), i);
        }
    });
    ਅਚਲ std::string fileName = v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                                 + "__ast_patterns_" + suffix + ".txt";
    ਨਮੂਨਾ_Stats.dump_ਮੰਜ਼ਿਲ_File(fileName);
    V3Global::dump_Check_Global_ਰੁੱਖ("astpatterns", 0, ਝੂਠ, ਝੂਠ);
}

//============================================================================
// V3DfgPasses top level

ਖਾਲੀ V3DfgPasses::dumpPatterns(ਅਚਲ std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>>& graphs,
                               ਅਚਲ std::string& suffix) {
    V3_ਨਮੂਨਾ_Stats ਨਮੂਨਾ_Stats{"DFG"};
    ਲਈ (ਸਵੈਚਾਲਿਤ& cp : graphs) {
        cp->for_Each_ਸ਼ੀਰਸ਼([&](ਅਚਲ Dfg_ਸ਼ੀਰਸ਼& vtx) {
            ਲਈ (uint32_t i = V3_ਨਮੂਨਾ_Stats::MIN_ਨਮੂਨਾ_ਡੂੰਘਾਈ;
                 i <= V3_ਨਮੂਨਾ_Stats::MAX_ਨਮੂਨਾ_ਡੂੰਘਾਈ; ++i) {
                ਨਮੂਨਾ_Stats.accumulate(vtx.ਨਮੂਨਾ_String(i), i);
            }
        });
    }
    ਅਚਲ std::string fileName = v3Global.opt.hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() + "/" + v3Global.opt.ਅਗੇਤਰ()
                                 + "__dfg_patterns" + suffix + ".txt";
    ਨਮੂਨਾ_Stats.dump_ਮੰਜ਼ਿਲ_File(fileName);
}
