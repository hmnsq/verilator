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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3AstPatterns.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

ವರ್ಗ V3PatternStats VL_NOT_FINAL {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    ಸ್ಥಿರ constexpr uint32_t MAX_PATTERN_DEPTH = 4;

ಗುಪ್ತ:
    ಧ್ರುವ std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    ಶೂನ್ಯ dump(std::ostream& os) {
        using ಸಾಲು_2 = std::pair<std::string, size_t>;
        ಪ್ರತಿ (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            ಪ್ರತಿ (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<ಸಾಲು_2> ಸಾಲುಗಳು;
            ಸಾಲುಗಳು.reserve(patternCounts.size());
            ಪ್ರತಿ (ಧ್ರುವ auto& pair : patternCounts) ಸಾಲುಗಳು.emplace_back(pair);
            std::sort(ಸಾಲುಗಳು.begin(), ಸಾಲುಗಳು.end(), [](ಧ್ರುವ ಸಾಲು_2& a, ಧ್ರುವ ಸಾಲು_2& b) {
                ಯದಿ (a.second != b.second) ಪ್ರತ್ಯಾಗಮ a.second > b.second;
                ಪ್ರತ್ಯಾಗಮ a.first < b.first;
            });

            // Print each pattern
            ಪ್ರತಿ (ಧ್ರುವ auto& ಸಾಲು : ಸಾಲುಗಳು) {
                os << ' ' << std::setw(12) << std::ಬಲ << ಸಾಲು.second;
                os << ' ' << std::ಎಡ << ಸಾಲು.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

ಸಾರ್ವಜನಿಕ:
    V3PatternStats(ಧ್ರುವ std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = ಸ್ವಭಾವ;

    ಶೂನ್ಯ accumulate(ಧ್ರುವ std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    ಶೂನ್ಯ dumpToFile(ಧ್ರುವ std::string& filename) {
        // Open, write, close
        ಧ್ರುವ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        ಯದಿ (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

ಶೂನ್ಯ V3AstPatterns::dumpAll(ಧ್ರುವ AstNetlist* nodep, ಧ್ರುವ std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಪ್ರತಿ (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ಧ್ರುವ std::string fileName = v3Global.opt.hier_ತುದಿ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("astpatterns", 0, ಅಸತ್ಯ, ಅಸತ್ಯ);
}

//============================================================================
// V3DfgPasses top level

ಶೂನ್ಯ V3DfgPasses::dumpPatterns(ಧ್ರುವ std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ಧ್ರುವ std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    ಪ್ರತಿ (auto& cp : graphs) {
        cp->forEachVertex([&](ಧ್ರುವ DfgVertex& vtx) {
            ಪ್ರತಿ (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ಧ್ರುವ std::string fileName = v3Global.opt.hier_ತುದಿ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
