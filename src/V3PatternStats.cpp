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

#شامل "V3PchAstNoMT.h"

#شامل "V3Ast.h"
#شامل "V3AstPatterns.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3File.h"

#شامل <memory>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

رده V3PatternStats VL_NOT_FINAL {
عمومی:
    ایستا constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    ایستا constexpr uint32_t MAX_PATTERN_DEPTH = 4;

خصوصی:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    تهی dump(std::ostream& os) {
        using سطر_2 = std::pair<std::string, size_t>;
        برای (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            برای (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                برای (ثابت auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<سطر_2> سطرها;
            سطرها.reserve(patternCounts.size());
            برای (ثابت auto& pair : patternCounts) سطرها.emplace_back(pair);
            std::sort(سطرها.begin(), سطرها.end(), [](ثابت سطر_2& a, ثابت سطر_2& b) {
                اگر (a.second != b.second) بازگشت a.second > b.second;
                بازگشت a.first < b.first;
            });

            // Print each pattern
            برای (ثابت auto& سطر : سطرها) {
                os << ' ' << std::setw(12) << std::راست << سطر.second;
                os << ' ' << std::چپ << سطر.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عمومی:
    V3PatternStats(ثابت std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = پیشفرض;

    تهی accumulate(ثابت std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    تهی dumpToFile(ثابت std::string& filename) {
        // Open, write, close
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        اگر (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

تهی V3AstPatterns::dumpAll(ثابت AstNetlist* nodep, ثابت std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ثابت Ast_گره_عبارت* exprp) {
        برای (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hier_بالاترین_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_درخت("astpatterns", 0, نادرست, نادرست);
}

//============================================================================
// V3DfgPasses top level

تهی V3DfgPasses::dumpPatterns(ثابت std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ثابت std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    برای (auto& cp : graphs) {
        cp->forEachVertex([&](ثابت DfgVertex& vtx) {
            برای (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hier_بالاترین_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
