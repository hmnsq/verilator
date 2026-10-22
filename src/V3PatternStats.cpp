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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3AstPatterns.h"
#ଅନ୍ତର୍ଭାଵ "V3Dfg.h"
#ଅନ୍ତର୍ଭାଵ "V3DfgPasses.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"

#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

ଶ୍ରେଣୀ V3PatternStats VL_NOT_FINAL {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର constexpr uint32_t MIN_PATTERN_DEPTH = 1;
    ସ୍ଥିର constexpr uint32_t MAX_PATTERN_DEPTH = 4;

ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::vector<std::unordered_map<std::string, size_t>> m_patternCounts{MAX_PATTERN_DEPTH + 1};

    ଶୂନ୍ୟ dump(std::ostream& os) {
        using ଧାଡ଼ି_2 = std::pair<std::string, size_t>;
        ପାଇଁ (uint32_t i = MIN_PATTERN_DEPTH; i <= MAX_PATTERN_DEPTH; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            auto& patternCounts = m_patternCounts[i];

            // Remove patterns also present at shallower depths
            ପାଇଁ (uint32_t j = MIN_PATTERN_DEPTH; j < i; ++j) {
                ପାଇଁ (ଅଚଳ auto& pair : m_patternCounts[j]) patternCounts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::vector<ଧାଡ଼ି_2> ଧାଡ଼ିଗୁଡ଼ିକ;
            ଧାଡ଼ିଗୁଡ଼ିକ.reserve(patternCounts.size());
            ପାଇଁ (ଅଚଳ auto& pair : patternCounts) ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(pair);
            std::sort(ଧାଡ଼ିଗୁଡ଼ିକ.begin(), ଧାଡ଼ିଗୁଡ଼ିକ.end(), [](ଅଚଳ ଧାଡ଼ି_2& a, ଅଚଳ ଧାଡ଼ି_2& b) {
                ଯଦି (a.second != b.second) ଫେରନ୍ତୁ a.second > b.second;
                ଫେରନ୍ତୁ a.first < b.first;
            });

            // Print each pattern
            ପାଇଁ (ଅଚଳ auto& ଧାଡ଼ି : ଧାଡ଼ିଗୁଡ଼ିକ) {
                os << ' ' << std::setw(12) << std::ଡାହାଣ << ଧାଡ଼ି.second;
                os << ' ' << std::ବାମ << ଧାଡ଼ି.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

ସର୍ବସାଧାରଣ:
    V3PatternStats(ଅଚଳ std::string& what)
        : m_what{what} {}
    ~V3PatternStats() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ accumulate(ଅଚଳ std::string& pattern, uint32_t depth) {
        m_patternCounts[depth][pattern] += 1;
    }

    ଶୂନ୍ୟ dumpToFile(ଅଚଳ std::string& filename) {
        // Open, write, close
        ଅଚଳ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        ଯଦି (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

ଶୂନ୍ୟ V3AstPatterns::dumpAll(ଅଚଳ AstNetlist* nodep, ଅଚଳ std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3PatternStats patternStats{"AST"};
    nodep->foreach([&](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ପାଇଁ (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
             i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
            patternStats.accumulate(exprp->patternString(i), i);
        }
    });
    ଅଚଳ std::string fileName = v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__ast_patterns_" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
    V3Global::dump_Check_Global_ବୃକ୍ଷ("astpatterns", 0, ମିଥ୍ୟା, ମିଥ୍ୟା);
}

//============================================================================
// V3DfgPasses top level

ଶୂନ୍ୟ V3DfgPasses::dumpPatterns(ଅଚଳ std::vector<std::unique_ptr<DfgGraph>>& graphs,
                               ଅଚଳ std::string& suffix) {
    V3PatternStats patternStats{"DFG"};
    ପାଇଁ (auto& cp : graphs) {
        cp->forEachVertex([&](ଅଚଳ DfgVertex& vtx) {
            ପାଇଁ (uint32_t i = V3PatternStats::MIN_PATTERN_DEPTH;
                 i <= V3PatternStats::MAX_PATTERN_DEPTH; ++i) {
                patternStats.accumulate(vtx.patternString(i), i);
            }
        });
    }
    ଅଚଳ std::string fileName = v3Global.opt.hier_ଶୀର୍ଷ_Data_Dir() + "/" + v3Global.opt.prefix()
                                 + "__dfg_patterns" + suffix + ".txt";
    patternStats.dumpToFile(fileName);
}
