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

صنف V3_الگو_Stats VL_NOT_FINAL {
عام:
    ساكن constexpr uint32_t MIN_الگو_عمق = 1;
    ساكن constexpr uint32_t MAX_الگو_عمق = 4;

خاص:
    ثابت std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::وکتور<std::unordered_map<std::string, اندازه_t>> m_الگو_Counts{MAX_الگو_عمق + 1};

    فراغ dump(std::ostream& os) {
        استفاده سطر_2 = std::pair<std::string, اندازه_t>;
        لكل (uint32_t i = MIN_الگو_عمق; i <= MAX_الگو_عمق; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            خودکار& الگو_Counts = m_الگو_Counts[i];

            // Remove patterns also present at shallower depths
            لكل (uint32_t j = MIN_الگو_عمق; j < i; ++j) {
                لكل (ثابت خودکار& pair : m_الگو_Counts[j]) الگو_Counts.erase(pair.اولی);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::وکتور<سطر_2> سطرها;
            سطرها.reserve(الگو_Counts.اندازه());
            لكل (ثابت خودکار& pair : الگو_Counts) سطرها.emplace_پشت(pair);
            std::مرتب_سازی(سطرها.begin(), سطرها.انتها(), [](ثابت سطر_2& a, ثابت سطر_2& b) {
                إذا (a.دومی != b.دومی) أرجع a.دومی > b.دومی;
                أرجع a.اولی < b.اولی;
            });

            // Print each pattern
            لكل (ثابت خودکار& سطر : سطرها) {
                os << ' ' << std::setw(12) << std::راست << سطر.دومی;
                os << ' ' << std::چپ << سطر.اولی << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

عام:
    V3_الگو_Stats(ثابت std::string& what)
        : m_what{what} {}
    ~V3_الگو_Stats() = افتراضي;

    فراغ accumulate(ثابت std::string& الگو_2, uint32_t عمق) {
        m_الگو_Counts[عمق][الگو_2] += 1;
    }

    فراغ dump_مقصد_File(ثابت std::string& filename) {
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
    V3_الگو_Stats الگو_Stats{"AST"};
    nodep->foreach([&](ثابت Ast_گره_عبارت* exprp) {
        لكل (uint32_t i = V3_الگو_Stats::MIN_الگو_عمق;
             i <= V3_الگو_Stats::MAX_الگو_عمق; ++i) {
            الگو_Stats.accumulate(exprp->الگو_String(i), i);
        }
    });
    ثابت std::string fileName = v3Global.opt.hier_بالاترین_داده_Dir() + "/" + v3Global.opt.پیشوند()
                                 + "__ast_patterns_" + suffix + ".txt";
    الگو_Stats.dump_مقصد_File(fileName);
    V3Global::dump_Check_Global_درخت("astpatterns", 0, خطأ, خطأ);
}

//============================================================================
// V3DfgPasses top level

فراغ V3DfgPasses::dumpPatterns(ثابت std::وکتور<std::unique_ptr<Dfg_گراف>>& graphs,
                               ثابت std::string& suffix) {
    V3_الگو_Stats الگو_Stats{"DFG"};
    لكل (خودکار& cp : graphs) {
        cp->for_Each_رأس([&](ثابت Dfg_رأس& vtx) {
            لكل (uint32_t i = V3_الگو_Stats::MIN_الگو_عمق;
                 i <= V3_الگو_Stats::MAX_الگو_عمق; ++i) {
                الگو_Stats.accumulate(vtx.الگو_String(i), i);
            }
        });
    }
    ثابت std::string fileName = v3Global.opt.hier_بالاترین_داده_Dir() + "/" + v3Global.opt.پیشوند()
                                 + "__dfg_patterns" + suffix + ".txt";
    الگو_Stats.dump_مقصد_File(fileName);
}
