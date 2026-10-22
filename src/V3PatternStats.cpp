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

वर्ग V3PatternStats VL_NOT_FINAL {
सार्वजनिक:
    स्थिर constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    स्थिर constexpr uint32_t MAX_PATTERN_DEPTH = 4;

गुप्त:
    अचर std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, माप_प्रकार>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    शून्य dump(std::ostream& os) {
        using पङ्क्ति_2 = std::pair<std::string, माप_प्रकार>;
        लागि (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            स्वतः& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            लागि (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                लागि (अचर स्वतः& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<पङ्क्ति_2> पङ्क्तिहरू;
            पङ्क्तिहरू.reserve(patternCounts.size());
            लागि (अचर स्वतः& pair : patternCounts) पङ्क्तिहरू.emplace_back(pair);
            std::sort(पङ्क्तिहरू.begin(), पङ्क्तिहरू.end(), [](अचर पङ्क्ति_2& a, अचर पङ्क्ति_2& b) {
                यदि (a.second != b.second) फर्काऊ a.second > b.second;
                फर्काऊ a.first < b.first;
            });

            // Print each pattern
            लागि (अचर स्वतः& पङ्क्ति : पङ्क्तिहरू) {
                os << ' ' << std::setw(12) << std::दायाँ << पङ्क्ति.second;
                os << ' ' << std::बायाँ << पङ्क्ति.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

सार्वजनिक:
    V3PatternStats(अचर std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = स्वभाव;

    शून्य accumulate(अचर std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    शून्य dumpToFile(अचर std::string& filename) {
        // Open, write, close
        अचर std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        यदि (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

शून्य V3AstPatterns::dumpAll(अचर AstNetlist* nodep, अचर std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](अचर Ast_गाँठो_अभिव्यक्ति* exprp) {
        लागि (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    अचर std::string fileName = v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_रूख("astpatterns", 0, असत्य, असत्य);
}

//============================================================================
// V3DfgPasses top level

शून्य V3DfgPasses::dumpPatterns(अचर std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               अचर std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    लागि (स्वतः& cp : graphs) {
        cp->forEachVertex([&](अचर DfgVertex& vtx) {
            लागि (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    अचर std::string fileName = v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
