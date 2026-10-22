// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Collect and print statistics
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <iomanip>
#అన్తర్భావ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

తరగతి StatsVisitor final : బహిరంగ VNVisitorConst {
    నిర్మాణం Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::అంకె_TYPES()> m_stat_Type_లెక్క{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_చివర> m_statPred{};
    };

    // STATE
    స్థిరరాశి తార్కిక m_fastOnly;  // When true, consider only fast functions
    తార్కిక m_ఖాళీలు = నిజం;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::సదిశ<uint64_t> m_statVarWidths;  // Variables of given width
    std::సదిశ<std::map<స్థిరరాశి std::string, uint32_t>>
        m_stat_Var_వెడల్పు_పేర్లు;  // Var names of given width

    // METHODS
    శూన్య లెక్క_Then_Iterate_Children(Ast_కణుపు* nodep) {
        ++m_accump->m_stat_Type_లెక్క[nodep->type()];
        ఒకవేళ (nodep->type() != VNType::Netlist) m_ఖాళీలు = అబద్ధం;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (nodep->dtypep()) {
            ఒకవేళ (!nodep->const_Pool_నమోదు()) {
                // Count proper variables
                ఒకవేళ (m_statVarWidths.కొలత() <= static_cast<కొలత_t>(nodep->వెడల్పు())) {
                    m_statVarWidths.resize(nodep->వెడల్పు() + 5);
                    ఒకవేళ (v3Global.opt.statsVars()) {  //
                        m_stat_Var_వెడల్పు_పేర్లు.resize(nodep->వెడల్పు() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->వెడల్పు());
                ఒకవేళ (v3Global.opt.statsVars()) {
                    ++m_stat_Var_వెడల్పు_పేర్లు.at(nodep->వెడల్పు())[nodep->prettyName()];
                }
            }
        }

        లెక్క_Then_Iterate_Children(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        లెక్క_Then_Iterate_Children(nodep);
    }

    శూన్య సందర్శించు(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        ఒకవేళ (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        లెక్క_Then_Iterate_Children(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { లెక్క_Then_Iterate_Children(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, స్థిరరాశి std::string& stage, తార్కిక fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        ఒకవేళ (m_ఖాళీలు) తిరిగి;

        // Shorthand
        స్థిరరాశి స్వయంచాలక కూడు_Stat = [&](స్థిరరాశి std::string& name, ద్విగుణ లెక్క, గుర్తులేని precision = 0) {
            V3Stats::కూడు_Stat(stage, name, లెక్క, precision);
        };

        // Variable widths
        కొరకు (గుర్తులేని i = 0; i < m_statVarWidths.కొలత(); i++) {
            ఒకవేళ (స్థిరరాశి uint64_t లెక్క = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                స్థిరరాశి std::string ఉపసర్గ = ss.str();
                ఒకవేళ (v3Global.opt.statsVars()) {
                    కొరకు (స్థిరరాశి స్వయంచాలక& it : m_stat_Var_వెడల్పు_పేర్లు.at(i)) {
                        కూడు_Stat(ఉపసర్గ + " " + it.మొదటి, it.రెండవ);
                    }
                } లేకపోతే {
                    కూడు_Stat(ఉపసర్గ, లెక్క);
                }
            }
        }

        // Node types (also total memory usage)
        స్థిరరాశి స్వయంచాలక typeName
            = [](కొలత_t t) { తిరిగి std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        స్థిరరాశి స్వయంచాలక type_కొలత
            = [](కొలత_t t) { తిరిగి VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        కొలత_t మొత్తం_కణుపు_జ్ఞాపకం_Usage = 0;
        కొరకు (కొలత_t t = 0; t < VNType::అంకె_TYPES(); ++t) {
            ఒకవేళ (స్థిరరాశి uint64_t లెక్క = m_counters.m_stat_Type_లెక్క[t]) {
                మొత్తం_కణుపు_జ్ఞాపకం_Usage += లెక్క * type_కొలత(t);
                కూడు_Stat("Node count, " + typeName(t), లెక్క);
            }
        }
        కూడు_Stat("Node mem TOTAL (MiB)", మొత్తం_కణుపు_జ్ఞాపకం_Usage >> 20);

        // Node Memory usage
        కొరకు (కొలత_t t = 0; t < VNType::అంకె_TYPES(); ++t) {
            ఒకవేళ (స్థిరరాశి uint64_t లెక్క = m_counters.m_stat_Type_లెక్క[t]) {
                స్థిరరాశి ద్విగుణ share = 100.0 * లెక్క * type_కొలత(t) / మొత్తం_కణుపు_జ్ఞాపకం_Usage;
                కూడు_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        కొరకు (పూర్ణాంకం t = 0; t < VBranchPred::ENUM_చివర; ++t) {
            ఒకవేళ (స్థిరరాశి uint64_t c = m_counters.m_statPred[t]) {
                కూడు_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

శూన్య V3Stats::statsStageAll(AstNetlist* nodep, స్థిరరాశి std::string& stage, తార్కిక fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
