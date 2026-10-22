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

صنف V3_نمونه_Stats VL_NOT_FINAL {
عام:
    ساكن constexpr uint32_t MIN_نمونه_ژوروالی = 1;
    ساكن constexpr uint32_t MAX_نمونه_ژوروالی = 4;

خاص:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::ویکټور<std::unordered_map<std::string, اندازه_t>> m_نمونه_Counts{MAX_نمونه_ژوروالی + 1};

    فراغ dump(std::ostream& os) {
        کارول کرښه_2 = std::pair<std::string, اندازه_t>;
        لكل (uint32_t i = MIN_نمونه_ژوروالی; i <= MAX_نمونه_ژوروالی; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            خپلکاری& نمونه_Counts = m_نمونه_Counts[i];

            // Remove patterns also present at shallower depths
            لكل (uint32_t j = MIN_نمونه_ژوروالی; j < i; ++j) {
                لكل (ثابت خپلکاری& pair : m_نمونه_Counts[j]) نمونه_Counts.erase(pair.لومړی);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::ویکټور<کرښه_2> کرښې;
            کرښې.reserve(نمونه_Counts.اندازه());
            لكل (ثابت خپلکاری& pair : نمونه_Counts) کرښې.emplace_شا(pair);
            std::ترتیبول(کرښې.begin(), کرښې.پای_ټکی(), [](ثابت کرښه_2& a, ثابت کرښه_2& b) {
                إذا (a.دوهم != b.دوهم) أرجع a.دوهم > b.دوهم;
                أرجع a.لومړی < b.لومړی;
            });

            // Print each pattern
            لكل (ثابت خپلکاری& کرښه : کرښې) {
                os << ' ' << std::setw(12) << std::ښی << کرښه.دوهم;
                os << ' ' << std::کیڼ << کرښه.لومړی << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عام:
    V3_نمونه_Stats(ثابت std::string& what)
        : m_what{what} {}
    ~V3_نمونه_Stats() = افتراضي;

    فراغ accumulate(ثابت std::string& نمونه_2, uint32_t ژوروالی) {
        m_نمونه_Counts[ژوروالی][نمونه_2] += 1;
    }

    فراغ dump_منزل_File(ثابت std::string& filename) {
        // Open, write, close
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        إذا (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

فراغ V3AstPatterns::dumpAll(ثابت AstNetlist* nodep, ثابت std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3_نمونه_Stats نمونه_Stats{"AST"};
    nodep->foreach([&](ثابت Ast_غوټه_بیان* exprp) {
        لكل (uint32_t i = V3_نمونه_Stats::MIN_نمونه_ژوروالی;
             i <= V3_نمونه_Stats::MAX_نمونه_ژوروالی; ++i) {
            نمونه_Stats.accumulate(exprp->نمونه_String(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hier_سر_معلومات_Dir() + "/" + v3Global.opt.مختاړی()
                                 + "__ast_patterns_" + suffix + ".txt";
    نمونه_Stats.dump_منزل_File(fileName);
    V3Global::dump_Check_Global_ونه("astpatterns", 0, خطأ, خطأ);
}

//============================================================================
// V3DfgPasses top level

فراغ V3DfgPasses::dumpPatterns(ثابت std::ویکټور<std::unique_ptr<Dfg_ګراف>>& graphs,
                               ثابت std::string& suffix) {
    V3_نمونه_Stats نمونه_Stats{"DFG"};
    لكل (خپلکاری& cp : graphs) {
        cp->for_Each_راس([&](ثابت Dfg_راس& vtx) {
            لكل (uint32_t i = V3_نمونه_Stats::MIN_نمونه_ژوروالی;
                 i <= V3_نمونه_Stats::MAX_نمونه_ژوروالی; ++i) {
                نمونه_Stats.accumulate(vtx.نمونه_String(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hier_سر_معلومات_Dir() + "/" + v3Global.opt.مختاړی()
                                 + "__dfg_patterns" + suffix + ".txt";
    نمونه_Stats.dump_منزل_File(fileName);
}
