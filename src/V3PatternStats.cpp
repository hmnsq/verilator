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

#شامل "V3PchAstNoMT.h"

#شامل "V3Ast.h"
#شامل "V3AstPatterns.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3File.h"

#شامل <memory>
#شامل <unordered_map>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

جماعت V3_نمونہ_Stats VL_NOT_FINAL {
عوامی:
    ساکن constexpr uint32_t MIN_نمونہ_گہرائی = 1;
    ساکن constexpr uint32_t MAX_نمونہ_گہرائی = 4;

نجی:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::سمتیہ<std::unordered_map<std::string, سائز_t>> m_نمونہ_Counts{MAX_نمونہ_گہرائی + 1};

    خالی dump(std::ostream& os) {
        استعمال سطر_2 = std::pair<std::string, سائز_t>;
        برائے (uint32_t i = MIN_نمونہ_گہرائی; i <= MAX_نمونہ_گہرائی; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            خودکار& نمونہ_Counts = m_نمونہ_Counts[i];

            // Remove patterns also present at shallower depths
            برائے (uint32_t j = MIN_نمونہ_گہرائی; j < i; ++j) {
                برائے (ثابت خودکار& pair : m_نمونہ_Counts[j]) نمونہ_Counts.erase(pair.پہلا);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::سمتیہ<سطر_2> سطریں;
            سطریں.reserve(نمونہ_Counts.سائز());
            برائے (ثابت خودکار& pair : نمونہ_Counts) سطریں.emplace_عقب(pair);
            std::ترتیب_دو(سطریں.begin(), سطریں.انتہا(), [](ثابت سطر_2& a, ثابت سطر_2& b) {
                اگر (a.دوسرا != b.دوسرا) واپس a.دوسرا > b.دوسرا;
                واپس a.پہلا < b.پہلا;
            });

            // Print each pattern
            برائے (ثابت خودکار& سطر : سطریں) {
                os << ' ' << std::setw(12) << std::دائیں << سطر.دوسرا;
                os << ' ' << std::بائیں << سطر.پہلا << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عوامی:
    V3_نمونہ_Stats(ثابت std::string& what)
        : m_what{what} {}
    ~V3_نمونہ_Stats() = طےشدہ;

    خالی accumulate(ثابت std::string& نمونہ_2, uint32_t گہرائی) {
        m_نمونہ_Counts[گہرائی][نمونہ_2] += 1;
    }

    خالی dump_مقصد_File(ثابت std::string& filename) {
        // Open, write, close
        ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        اگر (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

خالی V3AstPatterns::dumpAll(ثابت AstNetlist* nodep, ثابت std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3_نمونہ_Stats نمونہ_Stats{"AST"};
    nodep->foreach([&](ثابت Ast_گرہ_اظہاریہ* exprp) {
        برائے (uint32_t i = V3_نمونہ_Stats::MIN_نمونہ_گہرائی;
             i <= V3_نمونہ_Stats::MAX_نمونہ_گہرائی; ++i) {
            نمونہ_Stats.accumulate(exprp->نمونہ_String(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hier_چوٹی_ڈیٹا_Dir() + "/" + v3Global.opt.سابقہ()
                                 + "__ast_patterns_" + suffix + ".txt";
    نمونہ_Stats.dump_مقصد_File(fileName);
    V3Global::dump_Check_Global_درخت("astpatterns", 0, جھوٹ, جھوٹ);
}

//============================================================================
// V3DfgPasses top level

خالی V3DfgPasses::dumpPatterns(ثابت std::سمتیہ<std::unique_ptr<Dfg_گراف>>& graphs,
                               ثابت std::string& suffix) {
    V3_نمونہ_Stats نمونہ_Stats{"DFG"};
    برائے (خودکار& cp : graphs) {
        cp->for_Each_راس([&](ثابت Dfg_راس& vtx) {
            برائے (uint32_t i = V3_نمونہ_Stats::MIN_نمونہ_گہرائی;
                 i <= V3_نمونہ_Stats::MAX_نمونہ_گہرائی; ++i) {
                نمونہ_Stats.accumulate(vtx.نمونہ_String(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hier_چوٹی_ڈیٹا_Dir() + "/" + v3Global.opt.سابقہ()
                                 + "__dfg_patterns" + suffix + ".txt";
    نمونہ_Stats.dump_مقصد_File(fileName);
}
