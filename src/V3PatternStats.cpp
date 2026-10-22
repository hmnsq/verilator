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

#অন্তর্ভাৱ "V3PchAstNoMT.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3AstPatterns.h"
#অন্তর্ভাৱ "V3Dfg.h"
#অন্তর্ভাৱ "V3DfgPasses.h"
#অন্তর্ভাৱ "V3File.h"

#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

শ্ৰেণী V3PatternStats VL_NOT_FINAL {
সার্ৱজনিক:
    স্থির constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    স্থির constexpr uint32_t MAX_PATTERN_DEPTH = 4;

গুপ্ত:
    ধ্রুৱ std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    শূন্য dump(std::ostream& os) {
        using সাৰি_2 = std::pair<std::string, size_t>;
        বাবে (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            বাবে (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                বাবে (ধ্রুৱ auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<সাৰি_2> সাৰিগুলি;
            সাৰিগুলি.reserve(patternCounts.size());
            বাবে (ধ্রুৱ auto& pair : patternCounts) সাৰিগুলি.emplace_back(pair);
            std::sort(সাৰিগুলি.begin(), সাৰিগুলি.end(), [](ধ্রুৱ সাৰি_2& a, ধ্রুৱ সাৰি_2& b) {
                যদি (a.second != b.second) ঘূৰাই a.second > b.second;
                ঘূৰাই a.first < b.first;
            });

            // Print each pattern
            বাবে (ধ্রুৱ auto& সাৰি : সাৰিগুলি) {
                os << ' ' << std::setw(12) << std::ডান << সাৰি.second;
                os << ' ' << std::বাম << সাৰি.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

সার্ৱজনিক:
    V3PatternStats(ধ্রুৱ std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = স্ৱভাৱ;

    শূন্য accumulate(ধ্রুৱ std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    শূন্য dumpToFile(ধ্রুৱ std::string& filename) {
        // Open, write, close
        ধ্রুৱ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        যদি (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

শূন্য V3AstPatterns::dumpAll(ধ্রুৱ AstNetlist* nodep, ধ্রুৱ std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ধ্রুৱ Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        বাবে (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ধ্রুৱ std::string fileName = v3Global.opt.hier_শীৰ্ষ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_বৃক্ষ("astpatterns", 0, মিছা, মিছা);
}

//============================================================================
// V3DfgPasses top level

শূন্য V3DfgPasses::dumpPatterns(ধ্রুৱ std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ধ্রুৱ std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    বাবে (auto& cp : graphs) {
        cp->forEachVertex([&](ধ্রুৱ DfgVertex& vtx) {
            বাবে (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ধ্রুৱ std::string fileName = v3Global.opt.hier_শীৰ্ষ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
