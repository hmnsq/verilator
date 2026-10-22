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

#समावेश "V3PchAstNoMT.h"

#समावेश "V3Ast.h"
#समावेश "V3AstPatterns.h"
#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3File.h"

#समावेश <memory>
#समावेश <unordered_map>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

श्रेणी V3PatternStats VL_NOT_FINAL {
खुला:
    अटल constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    अटल constexpr uint32_t MAX_PATTERN_DEPTH = 4;

निजी:
    स्थिर std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, माप_प्रकार>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    व्योम dump(std::ब_प्रवाह& os) {
        using पंक्ति_2 = std::pair<std::string, माप_प्रकार>;
        क्रम (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            स्वतः& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            क्रम (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                क्रम (स्थिर स्वतः& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<पंक्ति_2> पंक्तियाँ;
            पंक्तियाँ.reserve(patternCounts.आकार());
            क्रम (स्थिर स्वतः& pair : patternCounts) पंक्तियाँ.emplace_back(pair);
            std::sort(पंक्तियाँ.begin(), पंक्तियाँ.end(), [](स्थिर पंक्ति_2& a, स्थिर पंक्ति_2& b) {
                अगर (a.second != b.second) वापस a.second > b.second;
                वापस a.first < b.first;
            });

            // Print each pattern
            क्रम (स्थिर स्वतः& पंक्ति : पंक्तियाँ) {
                os << ' ' << std::रखो_चौड़ाई(12) << std::दाँए << पंक्ति.second;
                os << ' ' << std::बाँए << पंक्ति.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

खुला:
    V3PatternStats(स्थिर std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = शेष;

    व्योम accumulate(स्थिर std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    व्योम dumpToFile(स्थिर std::string& filename) {
        // Open, write, close
        स्थिर std::unique_ptr<std::बख_प्रवाह> ofp{V3File::new_ofstream(filename)};
        अगर (ofp->हार()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

व्योम V3AstPatterns::dumpAll(स्थिर AstNetlist* nodep, स्थिर std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](स्थिर Ast_गाँठ_व्यंजक* exprp) {
        क्रम (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    स्थिर std::string fileName = v3Global.opt.hier_शीर्ष_तथ्य_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_वृक्ष("astpatterns", 0, false, false);
}

//============================================================================
// V3DfgPasses top level

व्योम V3DfgPasses::dumpPatterns(स्थिर std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               स्थिर std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    क्रम (स्वतः& cp : graphs) {
        cp->forEachVertex([&](स्थिर DfgVertex& vtx) {
            क्रम (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    स्थिर std::string fileName = v3Global.opt.hier_शीर्ष_तथ्य_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
