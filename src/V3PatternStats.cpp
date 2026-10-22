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

#અન્તર્ભાવ "V3PchAstNoMT.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3AstPatterns.h"
#અન્તર્ભાવ "V3Dfg.h"
#અન્તર્ભાવ "V3DfgPasses.h"
#અન્તર્ભાવ "V3File.h"

#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

વર્ગ V3PatternStats VL_NOT_FINAL {
જાહેર:
    સ્થિર constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    સ્થિર constexpr uint32_t MAX_PATTERN_DEPTH = 4;

ખાનગી:
    અચલ std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    શૂન્ય dump(std::ostream& os) {
        using લીટી_2 = std::pair<std::string, size_t>;
        માટે (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            માટે (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                માટે (અચલ auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<લીટી_2> લીટીઓ;
            લીટીઓ.reserve(patternCounts.size());
            માટે (અચલ auto& pair : patternCounts) લીટીઓ.emplace_back(pair);
            std::sort(લીટીઓ.begin(), લીટીઓ.end(), [](અચલ લીટી_2& a, અચલ લીટી_2& b) {
                જો (a.second != b.second) પાછા a.second > b.second;
                પાછા a.first < b.first;
            });

            // Print each pattern
            માટે (અચલ auto& લીટી : લીટીઓ) {
                os << ' ' << std::setw(12) << std::જમણું << લીટી.second;
                os << ' ' << std::ડાબું << લીટી.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

જાહેર:
    V3PatternStats(અચલ std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = મૂળભૂત;

    શૂન્ય accumulate(અચલ std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    શૂન્ય dumpToFile(અચલ std::string& filename) {
        // Open, write, close
        અચલ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        જો (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

શૂન્ય V3AstPatterns::dumpAll(અચલ AstNetlist* nodep, અચલ std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](અચલ Ast_ગાંઠ_પદાવલિ* exprp) {
        માટે (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    અચલ std::string fileName = v3Global.opt.hier_ટોચ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_વૃક્ષ("astpatterns", 0, ખોટું, ખોટું);
}

//============================================================================
// V3DfgPasses top level

શૂન્ય V3DfgPasses::dumpPatterns(અચલ std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               અચલ std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    માટે (auto& cp : graphs) {
        cp->forEachVertex([&](અચલ DfgVertex& vtx) {
            માટે (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    અચલ std::string fileName = v3Global.opt.hier_ટોચ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
