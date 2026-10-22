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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

ଶ୍ରେଣୀ StatsVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    ସଂରଚନା Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::ଅଙ୍କ_TYPES()> m_stat_Type_ଗଣନା{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ଅଚଳ ତାର୍କିକ m_fastOnly;  // When true, consider only fast functions
    ତାର୍କିକ m_empty = ସତ୍ୟ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ଅଚଳ std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    ଶୂନ୍ୟ ଗଣନା_Then_Iterate_Children(Ast_ଗଣ୍ଠି* nodep) {
        ++m_accump->m_stat_Type_ଗଣନା[nodep->type()];
        ଯଦି (nodep->type() != VNType::Netlist) m_empty = ମିଥ୍ୟା;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (nodep->dtypep()) {
            ଯଦି (!nodep->constPoolEntry()) {
                // Count proper variables
                ଯଦି (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    ଯଦି (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                ଯଦି (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        ଗଣନା_Then_Iterate_Children(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        ଗଣନା_Then_Iterate_Children(nodep);
    }

    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        ଯଦି (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        ଗଣନା_Then_Iterate_Children(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { ଗଣନା_Then_Iterate_Children(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ଅଚଳ std::string& stage, ତାର୍କିକ fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        ଯଦି (m_empty) ଫେରନ୍ତୁ;

        // Shorthand
        ଅଚଳ auto addStat = [&](ଅଚଳ std::string& name, ଦ୍ୱିଗୁଣ ଗଣନା_2, unsigned precision = 0) {
            V3Stats::addStat(stage, name, ଗଣନା_2, precision);
        };

        // Variable widths
        ପାଇଁ (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            ଯଦି (ଅଚଳ uint64_t ଗଣନା_2 = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ଅଚଳ std::string prefix = ss.str();
                ଯଦି (v3Global.opt.statsVars()) {
                    ପାଇଁ (ଅଚଳ auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } ଅନ୍ୟଥା {
                    addStat(prefix, ଗଣନା_2);
                }
            }
        }

        // Node types (also total memory usage)
        ଅଚଳ auto typeName
            = [](size_t t) { ଫେରନ୍ତୁ std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ଅଚଳ auto typeSize
            = [](size_t t) { ଫେରନ୍ତୁ VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_ଗଣ୍ଠି_Memory_Usage = 0;
        ପାଇଁ (size_t t = 0; t < VNType::ଅଙ୍କ_TYPES(); ++t) {
            ଯଦି (ଅଚଳ uint64_t ଗଣନା_2 = m_counters.m_stat_Type_ଗଣନା[t]) {
                total_ଗଣ୍ଠି_Memory_Usage += ଗଣନା_2 * typeSize(t);
                addStat("Node count, " + typeName(t), ଗଣନା_2);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_ଗଣ୍ଠି_Memory_Usage >> 20);

        // Node Memory usage
        ପାଇଁ (size_t t = 0; t < VNType::ଅଙ୍କ_TYPES(); ++t) {
            ଯଦି (ଅଚଳ uint64_t ଗଣନା_2 = m_counters.m_stat_Type_ଗଣନା[t]) {
                ଅଚଳ ଦ୍ୱିଗୁଣ share = 100.0 * ଗଣନା_2 * typeSize(t) / total_ଗଣ୍ଠି_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା t = 0; t < VBranchPred::_ENUM_END; ++t) {
            ଯଦି (ଅଚଳ uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

ଶୂନ୍ୟ V3Stats::statsStageAll(AstNetlist* nodep, ଅଚଳ std::string& stage, ତାର୍କିକ fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
