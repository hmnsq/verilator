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

#അന്തര്ഭാവ "V3PchAstNoMT.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3AstPatterns.h"
#അന്തര്ഭാവ "V3Dfg.h"
#അന്തര്ഭാവ "V3DfgPasses.h"
#അന്തര്ഭാവ "V3File.h"

#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

വർഗ്ഗം V3PatternStats VL_NOT_FINAL {
പൊതു:
    സ്ഥിതം constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    സ്ഥിതം constexpr uint32_t MAX_PATTERN_DEPTH = 4;

സ്വകാര്യ:
    സ്ഥിരം std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    ശൂന്യം dump(std::ostream& os) {
        using വരി_2 = std::pair<std::string, size_t>;
        വേണ്ടി (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            വേണ്ടി (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                വേണ്ടി (സ്ഥിരം auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<വരി_2> വരികൾ;
            വരികൾ.reserve(patternCounts.size());
            വേണ്ടി (സ്ഥിരം auto& pair : patternCounts) വരികൾ.emplace_back(pair);
            std::sort(വരികൾ.begin(), വരികൾ.end(), [](സ്ഥിരം വരി_2& a, സ്ഥിരം വരി_2& b) {
                എങ്കിൽ (a.second != b.second) തിരികെ a.second > b.second;
                തിരികെ a.first < b.first;
            });

            // Print each pattern
            വേണ്ടി (സ്ഥിരം auto& വരി : വരികൾ) {
                os << ' ' << std::setw(12) << std::വലത് << വരി.second;
                os << ' ' << std::ഇടത് << വരി.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

പൊതു:
    V3PatternStats(സ്ഥിരം std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = സ്ഥിരസ്ഥിതി;

    ശൂന്യം accumulate(സ്ഥിരം std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    ശൂന്യം dumpToFile(സ്ഥിരം std::string& filename) {
        // Open, write, close
        സ്ഥിരം std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        എങ്കിൽ (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

ശൂന്യം V3AstPatterns::dumpAll(സ്ഥിരം AstNetlist* nodep, സ്ഥിരം std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        വേണ്ടി (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    സ്ഥിരം std::string fileName = v3Global.opt.hier_മുകൾ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_വൃക്ഷം("astpatterns", 0, അസത്യം, അസത്യം);
}

//============================================================================
// V3DfgPasses top level

ശൂന്യം V3DfgPasses::dumpPatterns(സ്ഥിരം std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               സ്ഥിരം std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    വേണ്ടി (auto& cp : graphs) {
        cp->forEachVertex([&](സ്ഥിരം DfgVertex& vtx) {
            വേണ്ടി (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    സ്ഥിരം std::string fileName = v3Global.opt.hier_മുകൾ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
