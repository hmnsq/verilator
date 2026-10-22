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
    ध्रुव std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    शून्य dump(std::ostream& os) {
        using ओळ_2 = std::pair<std::string, size_t>;
        प्रति (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            प्रति (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                प्रति (ध्रुव auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<ओळ_2> ओळी;
            ओळी.reserve(patternCounts.size());
            प्रति (ध्रुव auto& pair : patternCounts) ओळी.emplace_back(pair);
            std::sort(ओळी.begin(), ओळी.end(), [](ध्रुव ओळ_2& a, ध्रुव ओळ_2& b) {
                यदि (a.second != b.second) प्रत्यागम a.second > b.second;
                प्रत्यागम a.first < b.first;
            });

            // Print each pattern
            प्रति (ध्रुव auto& ओळ : ओळी) {
                os << ' ' << std::setw(12) << std::उजवा << ओळ.second;
                os << ' ' << std::डावा << ओळ.first << '\n';
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
        यदि (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

शून्य V3AstPatterns::dumpAll(ध्रुव AstNetlist* nodep, ध्रुव std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ध्रुव Ast_गाठ_अभिव्यक्ती* exprp) {
        प्रति (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ध्रुव std::string fileName = v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_वृक्ष("astpatterns", 0, असत्य, असत्य);
}

//============================================================================
// V3DfgPasses top level

शून्य V3DfgPasses::dumpPatterns(ध्रुव std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ध्रुव std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    प्रति (auto& cp : graphs) {
        cp->forEachVertex([&](ध्रुव DfgVertex& vtx) {
            प्रति (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ध्रुव std::string fileName = v3Global.opt.hier_शीर्ष_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
