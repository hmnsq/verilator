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

#అన్తర్భావ "V3PchAstNoMT.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3AstPatterns.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3File.h"

#అన్తర్భావ <memory>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//============================================================================
// Accumulates and dumps the pattern statistics

తరగతి V3_నమూనా_Stats VL_NOT_FINAL {
బహిరంగ:
    స్థిర constexpr uint32_t MIN_నమూనా_లోతు = 1;
    స్థిర constexpr uint32_t MAX_నమూనా_లోతు = 4;

అంతరంగ:
    స్థిరరాశి std::string m_what;  // Description of what is being dumped
    // Maps from pattern to the number of times it appears, for each pattern depth
    std::సదిశ<std::unordered_map<std::string, కొలత_t>> m_నమూనా_Counts{MAX_నమూనా_లోతు + 1};

    శూన్య dump(std::ostream& os) {
        ఉపయోగించి పంక్తి_2 = std::pair<std::string, కొలత_t>;
        కొరకు (uint32_t i = MIN_నమూనా_లోతు; i <= MAX_నమూనా_లోతు; ++i) {
            os << m_what << " patterns with depth " << i << '\n';

            // Pick up pattern accumulators with given depth
            స్వయంచాలక& నమూనా_Counts = m_నమూనా_Counts[i];

            // Remove patterns also present at shallower depths
            కొరకు (uint32_t j = MIN_నమూనా_లోతు; j < i; ++j) {
                కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_నమూనా_Counts[j]) నమూనా_Counts.erase(pair.మొదటి);
            }

            // Sort patterns, first by descending frequency, then lexically
            std::సదిశ<పంక్తి_2> పంక్తులు;
            పంక్తులు.reserve(నమూనా_Counts.కొలత());
            కొరకు (స్థిరరాశి స్వయంచాలక& pair : నమూనా_Counts) పంక్తులు.emplace_వెనుక(pair);
            std::క్రమీకరించు(పంక్తులు.begin(), పంక్తులు.చివర(), [](స్థిరరాశి పంక్తి_2& a, స్థిరరాశి పంక్తి_2& b) {
                ఒకవేళ (a.రెండవ != b.రెండవ) తిరిగి a.రెండవ > b.రెండవ;
                తిరిగి a.మొదటి < b.మొదటి;
            });

            // Print each pattern
            కొరకు (స్థిరరాశి స్వయంచాలక& పంక్తి : పంక్తులు) {
                os << ' ' << std::setw(12) << std::కుడి << పంక్తి.రెండవ;
                os << ' ' << std::ఎడమ << పంక్తి.మొదటి << '\n';
            }

            // Trailing new-line to separate sections
            os << '\n';
        }
    }

బహిరంగ:
    V3_నమూనా_Stats(స్థిరరాశి std::string& what)
        : m_what{what} {}
    ~V3_నమూనా_Stats() = అప్రమేయ;

    శూన్య accumulate(స్థిరరాశి std::string& నమూనా_2, uint32_t లోతు) {
        m_నమూనా_Counts[లోతు][నమూనా_2] += 1;
    }

    శూన్య dump_గమ్యం_File(స్థిరరాశి std::string& filename) {
        // Open, write, close
        స్థిరరాశి std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
        ఒకవేళ (ofp->fail()) v3fatal("Can't write file: " << filename);
        dump(*ofp);
    }
};

//============================================================================
// V3AstPatterns top level

శూన్య V3AstPatterns::dumpAll(స్థిరరాశి AstNetlist* nodep, స్థిరరాశి std::string& suffix) {
    UINFO(2, __FUNCTION__ << ":");
    V3_నమూనా_Stats నమూనా_Stats{"AST"};
    nodep->foreach([&](స్థిరరాశి Ast_కణుపు_సమాసం* exprp) {
        కొరకు (uint32_t i = V3_నమూనా_Stats::MIN_నమూనా_లోతు;
             i <= V3_నమూనా_Stats::MAX_నమూనా_లోతు; ++i) {
            నమూనా_Stats.accumulate(exprp->నమూనా_String(i), i);
        }
    });
    స్థిరరాశి std::string fileName = v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + v3Global.opt.ఉపసర్గ()
                                 + "__ast_patterns_" + suffix + ".txt";
    నమూనా_Stats.dump_గమ్యం_File(fileName);
    V3Global::dump_Check_Global_వృక్షం("astpatterns", 0, అబద్ధం, అబద్ధం);
}

//============================================================================
// V3DfgPasses top level

శూన్య V3DfgPasses::dumpPatterns(స్థిరరాశి std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>>& graphs,
                               స్థిరరాశి std::string& suffix) {
    V3_నమూనా_Stats నమూనా_Stats{"DFG"};
    కొరకు (స్వయంచాలక& cp : graphs) {
        cp->for_Each_శీర్షం([&](స్థిరరాశి Dfg_శీర్షం& vtx) {
            కొరకు (uint32_t i = V3_నమూనా_Stats::MIN_నమూనా_లోతు;
                 i <= V3_నమూనా_Stats::MAX_నమూనా_లోతు; ++i) {
                నమూనా_Stats.accumulate(vtx.నమూనా_String(i), i);
            }
        });
    }
    స్థిరరాశి std::string fileName = v3Global.opt.hier_శిఖరం_దత్తాంశం_Dir() + "/" + v3Global.opt.ఉపసర్గ()
                                 + "__dfg_patterns" + suffix + ".txt";
    నమూనా_Stats.dump_గమ్యం_File(fileName);
}
