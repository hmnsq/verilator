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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3AstPatterns.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

ವರ್ಗ V3_ನಮೂನೆ_Stats VL_NOT_FINAL {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ constexpr uint32_t MIN_ನಮೂನೆ_ಆಳ = 1;
    ಸ್ಥಿರ constexpr uint32_t MAX_ನಮೂನೆ_ಆಳ = 4;

ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::ಸದಿಶ<std::unordered_map<std::string, ಗಾತ್ರ_t>> m_ನಮೂನೆ_Counts{MAX_ನಮೂನೆ_ಆಳ + 1};

    ಶೂನ್ಯ dump(std::ostream& os) {
        ಬಳಸಿ ಸಾಲು_2 = std::pair<std::string, ಗಾತ್ರ_t>;
        ಗಾಗಿ (uint32_t i = MIN_ನಮೂನೆ_ಆಳ; i <= MAX_ನಮೂನೆ_ಆಳ; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            ಸ್ವಯಂಚಾಲಿತ& ನಮೂನೆ_Counts = m_ನಮೂನೆ_Counts[i];

            // Remove patterns also present at shallower depths
            ಗಾಗಿ (uint32_t j = MIN_ನಮೂನೆ_ಆಳ; j < i; ++j) {
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_ನಮೂನೆ_Counts[j]) ನಮೂನೆ_Counts.erase(pair.ಮೊದಲ);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::ಸದಿಶ<ಸಾಲು_2> ಸಾಲುಗಳು;
            ಸಾಲುಗಳು.reserve(ನಮೂನೆ_Counts.ಗಾತ್ರ_2());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : ನಮೂನೆ_Counts) ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(pair);
            std::ವಿಂಗಡಿಸು(ಸಾಲುಗಳು.begin(), ಸಾಲುಗಳು.ಕೊನೆ(), [](ಸ್ಥಿರಾಂಕ ಸಾಲು_2& a, ಸ್ಥಿರಾಂಕ ಸಾಲು_2& b) {
                ಆದರೆ (a.ಎರಡನೇ != b.ಎರಡನೇ) ಹಿಂತಿರುಗಿ a.ಎರಡನೇ > b.ಎರಡನೇ;
                ಹಿಂತಿರುಗಿ a.ಮೊದಲ < b.ಮೊದಲ;
            });

            // Print each pattern
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಸಾಲು : ಸಾಲುಗಳು) {
                os << ' ' << std::setw(12) << std::ಬಲ << ಸಾಲು.ಎರಡನೇ;
                os << ' ' << std::ಎಡ << ಸಾಲು.ಮೊದಲ << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

ಸಾರ್ವಜನಿಕ:
    V3_ನಮೂನೆ_Stats(ಸ್ಥಿರಾಂಕ std::string& what)
        : m_what{what} {}
    ~V3_ನಮೂನೆ_Stats() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ accumulate(ಸ್ಥಿರಾಂಕ std::string& ನಮೂನೆ_2, uint32_t ಆಳ) {
        m_ನಮೂನೆ_Counts[ಆಳ][ನಮೂನೆ_2] += 1;
    }

    ಶೂನ್ಯ dump_ಗಮ್ಯ_File(ಸ್ಥಿರಾಂಕ std::string& filename) {
        // Open, write, close
        ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        ಆದರೆ (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

ಶೂನ್ಯ V3AstPatterns::dumpAll(ಸ್ಥಿರಾಂಕ AstNetlist* nodep, ಸ್ಥಿರಾಂಕ std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3_ನಮೂನೆ_Stats ನಮೂನೆ_Stats{"AST"};
    nodep->foreach([&](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಗಾಗಿ (uint32_t i = V3_ನಮೂನೆ_Stats::MIN_ನಮೂನೆ_ಆಳ;
             i <= V3_ನಮೂನೆ_Stats::MAX_ನಮೂನೆ_ಆಳ; ++i) {
            ನಮೂನೆ_Stats.accumulate(exprp->ನಮೂನೆ_String(i), i);
        }
    });
    ಸ್ಥಿರಾಂಕ std::string fileName = v3Global.opt.hier_ತುದಿ_ದತ್ತಾಂಶ_Dir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ()
                                 + "__ast_patterns_" + suffix + ".txt";
    ನಮೂನೆ_Stats.dump_ಗಮ್ಯ_File(fileName);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("astpatterns", 0, ಸುಳ್ಳು, ಸುಳ್ಳು);
}

//============================================================================
// V3DfgPasses top level

ಶೂನ್ಯ V3DfgPasses::dumpPatterns(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>>& graphs,
                               ಸ್ಥಿರಾಂಕ std::string& suffix) {
    V3_ನಮೂನೆ_Stats ನಮೂನೆ_Stats{"DFG"};
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : graphs) {
        cp->for_Each_ಶೃಂಗ([&](ಸ್ಥಿರಾಂಕ Dfg_ಶೃಂಗ& vtx) {
            ಗಾಗಿ (uint32_t i = V3_ನಮೂನೆ_Stats::MIN_ನಮೂನೆ_ಆಳ;
                 i <= V3_ನಮೂನೆ_Stats::MAX_ನಮೂನೆ_ಆಳ; ++i) {
                ನಮೂನೆ_Stats.accumulate(vtx.ನಮೂನೆ_String(i), i);
            }
        });
    }
    ಸ್ಥಿರಾಂಕ std::string fileName = v3Global.opt.hier_ತುದಿ_ದತ್ತಾಂಶ_Dir() + "/" + v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ()
                                 + "__dfg_patterns" + suffix + ".txt";
    ನಮೂನೆ_Stats.dump_ಗಮ್ಯ_File(fileName);
}
