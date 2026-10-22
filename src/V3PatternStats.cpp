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

#ضمن "V3PchAstNoMT.h"

#ضمن "V3Ast.h"
#ضمن "V3AstPatterns.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3File.h"

#ضمن <memory>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

صنف V3PatternStats VL_NOT_FINAL {
عام:
    ساكن constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    ساكن constexpr uint32_t MAX_PATTERN_DEPTH = 4;

خاص:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    فراغ dump(std::ostream& os) {
        using Line = std::pair<std::string, size_t>;
        لكل (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            لكل (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                لكل (ثابت auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<Line> lines;
            lines.reserve(patternCounts.size());
            لكل (ثابت auto& pair : patternCounts) lines.emplace_back(pair);
            std::sort(lines.begin(), lines.end(), [](ثابت Line& a, ثابت Line& b) {
                𐤀𐤌 (a.second != b.second) أرجع a.second > b.second;
                أرجع a.first < b.first;
            });

            // Print each pattern
            لكل (ثابت auto& line : lines) {
                os << ' ' << std::setw(12) << std::right << line.second;
                os << ' ' << std::left << line.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عام:
    V3PatternStats(ثابت std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = افتراضي;

    فراغ accumulate(ثابت std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    فراغ dumpToFile(ثابت std::string& filename) {
        // Open, write, close
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        𐤀𐤌 (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

فراغ V3AstPatterns::dumpAll(ثابت AstNetlist* nodep, ثابت std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ثابت AstNodeExpr* exprp) {
        لكل (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hierTopDataDir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dumpCheckGlobalTree("astpatterns", 0, خطأ, خطأ);
}

//============================================================================
// V3DfgPasses top level

فراغ V3DfgPasses::dumpPatterns(ثابت std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ثابت std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    لكل (auto& cp : graphs) {
        cp->forEachVertex([&](ثابت DfgVertex& vtx) {
            لكل (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hierTopDataDir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
