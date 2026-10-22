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

صنف V3_نمونہ_Stats VL_NOT_FINAL {
عام:
    ساكن constexpr uint32_t MIN_نمونہ_ڈونگھائی = 1;
    ساكن constexpr uint32_t MAX_نمونہ_ڈونگھائی = 4;

خاص:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::سمتیہ<std::unordered_map<std::string, ناپ_t>> m_نمونہ_Counts{MAX_نمونہ_ڈونگھائی + 1};

    فراغ dump(std::ostream& os) {
        ورتوں سطر_2 = std::pair<std::string, ناپ_t>;
        لكل (uint32_t i = MIN_نمونہ_ڈونگھائی; i <= MAX_نمونہ_ڈونگھائی; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            خودکار& نمونہ_Counts = m_نمونہ_Counts[i];

            // Remove patterns also present at shallower depths
            لكل (uint32_t j = MIN_نمونہ_ڈونگھائی; j < i; ++j) {
                لكل (ثابت خودکار& pair : m_نمونہ_Counts[j]) نمونہ_Counts.erase(pair.پہلا);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::سمتیہ<سطر_2> سطریں;
            سطریں.reserve(نمونہ_Counts.ناپ());
            لكل (ثابت خودکار& pair : نمونہ_Counts) سطریں.emplace_پٹھ(pair);
            std::ترتیب_دیو(سطریں.begin(), سطریں.سرا(), [](ثابت سطر_2& a, ثابت سطر_2& b) {
                إذا (a.دوجا != b.دوجا) أرجع a.دوجا > b.دوجا;
                أرجع a.پہلا < b.پہلا;
            });

            // Print each pattern
            لكل (ثابت خودکار& سطر : سطریں) {
                os << ' ' << std::setw(12) << std::سجے << سطر.دوجا;
                os << ' ' << std::کھبے << سطر.پہلا << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عام:
    V3_نمونہ_Stats(ثابت std::string& what)
        : m_what{what} {}
    ~V3_نمونہ_Stats() = افتراضي;

    فراغ accumulate(ثابت std::string& نمونہ_2, uint32_t ڈونگھائی) {
        m_نمونہ_Counts[ڈونگھائی][نمونہ_2] += 1;
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
    V3_نمونہ_Stats نمونہ_Stats{"AST"};
    nodep->foreach([&](ثابت Ast_گرہ_اظہاریہ* exprp) {
        لكل (uint32_t i = V3_نمونہ_Stats::MIN_نمونہ_ڈونگھائی;
             i <= V3_نمونہ_Stats::MAX_نمونہ_ڈونگھائی; ++i) {
            نمونہ_Stats.accumulate(exprp->نمونہ_String(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hier_چوٹی_انکڑے_Dir() + "/" + v3Global.opt.اگیتر()
                                 + "__ast_patterns_" + suffix + ".txt";
    نمونہ_Stats.dump_منزل_File(fileName);
    V3Global::dump_Check_Global_درخت("astpatterns", 0, خطأ, خطأ);
}

//============================================================================
// V3DfgPasses top level

فراغ V3DfgPasses::dumpPatterns(ثابت std::سمتیہ<std::unique_ptr<Dfg_گراف>>& graphs,
                               ثابت std::string& suffix) {
    V3_نمونہ_Stats نمونہ_Stats{"DFG"};
    لكل (خودکار& cp : graphs) {
        cp->for_Each_راس([&](ثابت Dfg_راس& vtx) {
            لكل (uint32_t i = V3_نمونہ_Stats::MIN_نمونہ_ڈونگھائی;
                 i <= V3_نمونہ_Stats::MAX_نمونہ_ڈونگھائی; ++i) {
                نمونہ_Stats.accumulate(vtx.نمونہ_String(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hier_چوٹی_انکڑے_Dir() + "/" + v3Global.opt.اگیتر()
                                 + "__dfg_patterns" + suffix + ".txt";
    نمونہ_Stats.dump_منزل_File(fileName);
}
