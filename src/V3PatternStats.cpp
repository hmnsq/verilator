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

#அந்தர்பாவ "V3PchAstNoMT.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3AstPatterns.h"
#அந்தர்பாவ "V3Dfg.h"
#அந்தர்பாவ "V3DfgPasses.h"
#அந்தர்பாவ "V3File.h"

#அந்தர்பாவ <memory>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

வகுப்பு V3PatternStats VL_NOT_FINAL {
பொது:
    நிலையான constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    நிலையான constexpr uint32_t MAX_PATTERN_DEPTH = 4;

தனிப்பட்ட:
    மாறிலி std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    வெறுமை dump(std::ostream& os) {
        using வரி_2 = std::pair<std::string, size_t>;
        க்கு (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            க்கு (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                க்கு (மாறிலி auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<வரி_2> வரிகள்;
            வரிகள்.reserve(patternCounts.size());
            க்கு (மாறிலி auto& pair : patternCounts) வரிகள்.emplace_back(pair);
            std::sort(வரிகள்.begin(), வரிகள்.end(), [](மாறிலி வரி_2& a, மாறிலி வரி_2& b) {
                எனில் (a.second != b.second) திரும்பு a.second > b.second;
                திரும்பு a.first < b.first;
            });

            // Print each pattern
            க்கு (மாறிலி auto& வரி : வரிகள்) {
                os << ' ' << std::setw(12) << std::வலது << வரி.second;
                os << ' ' << std::இடது << வரி.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

பொது:
    V3PatternStats(மாறிலி std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = இயல்பு;

    வெறுமை accumulate(மாறிலி std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    வெறுமை dumpToFile(மாறிலி std::string& filename) {
        // Open, write, close
        மாறிலி std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        எனில் (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

வெறுமை V3AstPatterns::dumpAll(மாறிலி AstNetlist* nodep, மாறிலி std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](மாறிலி Ast_முனை_கோவை* exprp) {
        க்கு (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    மாறிலி std::string fileName = v3Global.opt.hier_உச்சி_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_மரம்("astpatterns", 0, பொய், பொய்);
}

//============================================================================
// V3DfgPasses top level

வெறுமை V3DfgPasses::dumpPatterns(மாறிலி std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               மாறிலி std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    க்கு (auto& cp : graphs) {
        cp->forEachVertex([&](மாறிலி DfgVertex& vtx) {
            க்கு (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    மாறிலி std::string fileName = v3Global.opt.hier_உச்சி_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
