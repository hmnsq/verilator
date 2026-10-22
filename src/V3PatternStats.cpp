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

वर्ग V3_आदर्शः_Stats VL_NOT_FINAL {
सार्वजनिक:
    स्थिर constexpr uint32_t MIN_आदर्शः_गभीरता = 1;
    स्थिर constexpr uint32_t MAX_आदर्शः_गभीरता = 4;

गुप्त:
    ध्रुव std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::सदिशम्<std::unordered_map<std::string, आकार_t>> m_आदर्शः_Counts{MAX_आदर्शः_गभीरता + 1};

    शून्य dump(std::ostream& os) {
        उपयुज्य पङ्क्तिः_2 = std::pair<std::string, आकार_t>;
        प्रति (uint32_t i = MIN_आदर्शः_गभीरता; i <= MAX_आदर्शः_गभीरता; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            स्वचालित& आदर्शः_Counts = m_आदर्शः_Counts[i];

            // Remove patterns also present at shallower depths
            प्रति (uint32_t j = MIN_आदर्शः_गभीरता; j < i; ++j) {
                प्रति (ध्रुव स्वचालित& pair : m_आदर्शः_Counts[j]) आदर्शः_Counts.erase(pair.first);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::सदिशम्<पङ्क्तिः_2> पङ्क्तयः;
            पङ्क्तयः.reserve(आदर्शः_Counts.आकार());
            प्रति (ध्रुव स्वचालित& pair : आदर्शः_Counts) पङ्क्तयः.emplace_पृष्ठभागः(pair);
            std::क्रमय(पङ्क्तयः.begin(), पङ्क्तयः.अन्तः_2(), [](ध्रुव पङ्क्तिः_2& a, ध्रुव पङ्क्तिः_2& b) {
                यदि (a.second != b.second) प्रत्यागम a.second > b.second;
                प्रत्यागम a.first < b.first;
            });

            // Print each pattern
            प्रति (ध्रुव स्वचालित& पङ्क्तिः : पङ्क्तयः) {
                os << ' ' << std::setw(12) << std::दक्षिणतः << पङ्क्तिः.second;
                os << ' ' << std::वामतः << पङ्क्तिः.first << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

सार्वजनिक:
    V3_आदर्शः_Stats(ध्रुव std::string& what)
        : m_what{what} {}
    ~V3_आदर्शः_Stats() = स्वभाव;

    शून्य accumulate(ध्रुव std::string& आदर्शः_2, uint32_t गभीरता) {
        m_आदर्शः_Counts[गभीरता][आदर्शः_2] += 1;
    }

    शून्य dump_गन्तव्यम्_File(ध्रुव std::string& filename) {
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
    V3_आदर्शः_Stats आदर्शः_Stats{"AST"};
    nodep->foreach([&](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        प्रति (uint32_t i = V3_आदर्शः_Stats::MIN_आदर्शः_गभीरता;
             i <= V3_आदर्शः_Stats::MAX_आदर्शः_गभीरता; ++i) {
            आदर्शः_Stats.accumulate(exprp->आदर्शः_String(i), i);
        }
    });
    ध्रुव std::string fileName = v3Global.opt.hier_शिखरम्_दत्त_Dir() + "/" + v3Global.opt.उपसर्गः()
                                 + "__ast_patterns_" + suffix + ".txt";
    आदर्शः_Stats.dump_गन्तव्यम्_File(fileName);
    V3Global::dump_Check_Global_वृक्षः("astpatterns", 0, असत्य, असत्य);
}

//============================================================================
// V3DfgPasses top level

शून्य V3DfgPasses::dumpPatterns(ध्रुव std::सदिशम्<std::unique_ptr<DfgGraph>>& graphs,
                               ध्रुव std::string& suffix) {
    V3_आदर्शः_Stats आदर्शः_Stats{"DFG"};
    प्रति (स्वचालित& cp : graphs) {
        cp->forEachVertex([&](ध्रुव DfgVertex& vtx) {
            प्रति (uint32_t i = V3_आदर्शः_Stats::MIN_आदर्शः_गभीरता;
                 i <= V3_आदर्शः_Stats::MAX_आदर्शः_गभीरता; ++i) {
                आदर्शः_Stats.accumulate(vtx.आदर्शः_String(i), i);
            }
        });
    }
    ध्रुव std::string fileName = v3Global.opt.hier_शिखरम्_दत्त_Dir() + "/" + v3Global.opt.उपसर्गः()
                                 + "__dfg_patterns" + suffix + ".txt";
    आदर्शः_Stats.dump_गन्तव्यम्_File(fileName);
}
