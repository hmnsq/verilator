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

#अन्तर्भाव "V3PchAstNoMT.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3AstPatterns.h"
#अन्तर्भाव "V3Dfg.h"
#अन्तर्भाव "V3DfgPasses.h"
#अन्तर्भाव "V3File.h"

#अन्तर्भाव <memory>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

वग्ग V3PatternStats VL_NOT_FINAL {
सार्वजनिक:
    स्थिर constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    स्थिर constexpr uint32_t MAX_PATTERN_DEPTH = 4;

गुप्त:
    ध्रुव std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    शून्य dump(std::ostream& os) {
        using पङ्क्तिः_2 = std::pair<std::string, size_t>;
        अत्थाय (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            अत्थाय (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                अत्थाय (ध्रुव auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<पङ्क्तिः_2> पङ्क्तयः;
            पङ्क्तयः.reserve(patternCounts.size());
            अत्थाय (ध्रुव auto& pair : patternCounts) पङ्क्तयः.emplace_back(pair);
            std::sort(पङ्क्तयः.begin(), पङ्क्तयः.end(), [](ध्रुव पङ्क्तिः_2& a, ध्रुव पङ्क्तिः_2& b) {
                सचे (a.second != b.second) पटिनिवत्त a.second > b.second;
                पटिनिवत्त a.first < b.first;
            });

            // Print each pattern
            अत्थाय (ध्रुव auto& पङ्क्तिः : पङ्क्तयः) {
                os << ' ' << std::setw(12) << std::दक्षिणः << पङ्क्तिः.second;
                os << ' ' << std::वामः << पङ्क्तिः.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

सार्वजनिक:
    V3PatternStats(ध्रुव std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = स्वभाव;

    शून्य accumulate(ध्रुव std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    शून्य dumpToFile(ध्रुव std::string& filename) {
        // Open, write, close
        ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        सचे (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

शून्य V3AstPatterns::dumpAll(ध्रुव AstNetlist* nodep, ध्रुव std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        अत्थाय (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ध्रुव std::string fileName = v3Global.opt.hier_शिखरम्_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_वृक्षः("astpatterns", 0, मिच्छा, मिच्छा);
}

//============================================================================
// V3DfgPasses top level

शून्य V3DfgPasses::dumpPatterns(ध्रुव std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ध्रुव std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    अत्थाय (auto& cp : graphs) {
        cp->forEachVertex([&](ध्रुव DfgVertex& vtx) {
            अत्थाय (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ध्रुव std::string fileName = v3Global.opt.hier_शिखरम्_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
