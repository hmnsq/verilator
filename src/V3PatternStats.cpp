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

#অন্তর্ভাব "V3PchAstNoMT.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3AstPatterns.h"
#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"
#অন্তর্ভাব "V3File.h"

#অন্তর্ভাব <memory>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

শ্রেণী V3PatternStats VL_NOT_FINAL {
সর্বজনীন:
    স্থির constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    স্থির constexpr uint32_t MAX_PATTERN_DEPTH = 4;

ব্যক্তিগত:
    ধ্রুবক std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, আকার_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    শূন্য dump(std::ostream& os) {
        using সারি_2 = std::pair<std::string, আকার_t>;
        জন্য (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            জন্য (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                জন্য (ধ্রুবক auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<সারি_2> সারিগুলি;
            সারিগুলি.reserve(patternCounts.আকার_2());
            জন্য (ধ্রুবক auto& pair : patternCounts) সারিগুলি.emplace_back(pair);
            std::sort(সারিগুলি.begin(), সারিগুলি.end(), [](ধ্রুবক সারি_2& a, ধ্রুবক সারি_2& b) {
                যদি (a.second != b.second) ফেরত a.second > b.second;
                ফেরত a.first < b.first;
            });

            // Print each pattern
            জন্য (ধ্রুবক auto& সারি : সারিগুলি) {
                os << ' ' << std::setw(12) << std::ডান << সারি.second;
                os << ' ' << std::বাম << সারি.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

সর্বজনীন:
    V3PatternStats(ধ্রুবক std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = পূর্বনির্ধারিত;

    শূন্য accumulate(ধ্রুবক std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    শূন্য dumpToFile(ধ্রুবক std::string& filename) {
        // Open, write, close
        ধ্রুবক std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        যদি (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

শূন্য V3AstPatterns::dumpAll(ধ্রুবক AstNetlist* nodep, ধ্রুবক std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* exprp) {
        জন্য (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ধ্রুবক std::string fileName = v3Global.opt.hier_শীর্ষ_তথ্য_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_বৃক্ষ("astpatterns", 0, মিথ্যা, মিথ্যা);
}

//============================================================================
// V3DfgPasses top level

শূন্য V3DfgPasses::dumpPatterns(ধ্রুবক std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ধ্রুবক std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    জন্য (auto& cp : graphs) {
        cp->forEachVertex([&](ধ্রুবক DfgVertex& vtx) {
            জন্য (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ধ্রুবক std::string fileName = v3Global.opt.hier_শীর্ষ_তথ্য_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
