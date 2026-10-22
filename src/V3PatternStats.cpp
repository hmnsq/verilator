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

صنف V3_نمونو_Stats VL_NOT_FINAL {
عام:
    ساكن constexpr uint32_t MIN_نمونو_اونهائي = 1;
    ساكن constexpr uint32_t MAX_نمونو_اونهائي = 4;

خاص:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::ويڪٽر<std::unordered_map<std::string, سائيز_t>> m_نمونو_Counts{MAX_نمونو_اونهائي + 1};

    فراغ dump(std::ostream& os) {
        استعمال سٽ_2 = std::pair<std::string, سائيز_t>;
        لكل (uint32_t i = MIN_نمونو_اونهائي; i <= MAX_نمونو_اونهائي; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            خودڪار& نمونو_Counts = m_نمونو_Counts[i];

            // Remove patterns also present at shallower depths
            لكل (uint32_t j = MIN_نمونو_اونهائي; j < i; ++j) {
                لكل (ثابت خودڪار& pair : m_نمونو_Counts[j]) نمونو_Counts.erase(pair.پهريون);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::ويڪٽر<سٽ_2> سٽون;
            سٽون.reserve(نمونو_Counts.سائيز());
            لكل (ثابت خودڪار& pair : نمونو_Counts) سٽون.emplace_پٺ(pair);
            std::ترتيب_ڏيو(سٽون.begin(), سٽون.پڇاڙي(), [](ثابت سٽ_2& a, ثابت سٽ_2& b) {
                إذا (a.ٻيو != b.ٻيو) أرجع a.ٻيو > b.ٻيو;
                أرجع a.پهريون < b.پهريون;
            });

            // Print each pattern
            لكل (ثابت خودڪار& سٽ : سٽون) {
                os << ' ' << std::setw(12) << std::ساڄي << سٽ.ٻيو;
                os << ' ' << std::کاٻي << سٽ.پهريون << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عام:
    V3_نمونو_Stats(ثابت std::string& what)
        : m_what{what} {}
    ~V3_نمونو_Stats() = افتراضي;

    فراغ accumulate(ثابت std::string& نمونو_2, uint32_t اونهائي) {
        m_نمونو_Counts[اونهائي][نمونو_2] += 1;
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
    V3_نمونو_Stats نمونو_Stats{"AST"};
    nodep->foreach([&](ثابت Ast_ڳنڍ_اظهار* exprp) {
        لكل (uint32_t i = V3_نمونو_Stats::MIN_نمونو_اونهائي;
             i <= V3_نمونو_Stats::MAX_نمونو_اونهائي; ++i) {
            نمونو_Stats.accumulate(exprp->نمونو_String(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hier_چوٽي_معلومات_Dir() + "/" + v3Global.opt.اڳياڙي()
                                 + "__ast_patterns_" + suffix + ".txt";
    نمونو_Stats.dump_منزل_File(fileName);
    V3Global::dump_Check_Global_وڻ("astpatterns", 0, خطأ, خطأ);
}

//============================================================================
// V3DfgPasses top level

فراغ V3DfgPasses::dumpPatterns(ثابت std::ويڪٽر<std::unique_ptr<Dfg_گراف>>& graphs,
                               ثابت std::string& suffix) {
    V3_نمونو_Stats نمونو_Stats{"DFG"};
    لكل (خودڪار& cp : graphs) {
        cp->for_Each_راس([&](ثابت Dfg_راس& vtx) {
            لكل (uint32_t i = V3_نمونو_Stats::MIN_نمونو_اونهائي;
                 i <= V3_نمونو_Stats::MAX_نمونو_اونهائي; ++i) {
                نمونو_Stats.accumulate(vtx.نمونو_String(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hier_چوٽي_معلومات_Dir() + "/" + v3Global.opt.اڳياڙي()
                                 + "__dfg_patterns" + suffix + ".txt";
    نمونو_Stats.dump_منزل_File(fileName);
}
