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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

வகுப்பு StatsVisitor final : பொது VNVisitorConst {
    கட்டமைப்பு Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::இலக்கம்_TYPES()> m_stat_Type_மொத்தம்{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    மாறிலி தருக்க m_fastOnly;  // When true, consider only fast functions
    தருக்க m_empty = உண்மை;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<மாறிலி std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    வெறுமை மொத்தம்_Then_Iterate_Children(Ast_முனை* nodep) {
        ++m_accump->m_stat_Type_மொத்தம்[nodep->type()];
        எனில் (nodep->type() != VNType::Netlist) m_empty = பொய்;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    வெறுமை visit(AstVar* nodep) override {
        எனில் (nodep->dtypep()) {
            எனில் (!nodep->constPoolEntry()) {
                // Count proper variables
                எனில் (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    எனில் (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                எனில் (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        மொத்தம்_Then_Iterate_Children(nodep);
    }

    வெறுமை visit(Ast_முனை_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        மொத்தம்_Then_Iterate_Children(nodep);
    }

    வெறுமை visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        எனில் (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        மொத்தம்_Then_Iterate_Children(nodep);
    }

    வெறுமை visit(Ast_முனை* nodep) override { மொத்தம்_Then_Iterate_Children(nodep); }

பொது:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, மாறிலி std::string& stage, தருக்க fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        எனில் (m_empty) திரும்பு;

        // Shorthand
        மாறிலி auto addStat = [&](மாறிலி std::string& name, இரட்டை மொத்தம், unsigned precision = 0) {
            V3Stats::addStat(stage, name, மொத்தம், precision);
        };

        // Variable widths
        க்கு (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            எனில் (மாறிலி uint64_t மொத்தம் = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                மாறிலி std::string prefix = ss.str();
                எனில் (v3Global.opt.statsVars()) {
                    க்கு (மாறிலி auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } இல்லையெனில் {
                    addStat(prefix, மொத்தம்);
                }
            }
        }

        // Node types (also total memory usage)
        மாறிலி auto typeName
            = [](size_t t) { திரும்பு std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        மாறிலி auto typeSize
            = [](size_t t) { திரும்பு VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_முனை_Memory_Usage = 0;
        க்கு (size_t t = 0; t < VNType::இலக்கம்_TYPES(); ++t) {
            எனில் (மாறிலி uint64_t மொத்தம் = m_counters.m_stat_Type_மொத்தம்[t]) {
                total_முனை_Memory_Usage += மொத்தம் * typeSize(t);
                addStat("Node count, " + typeName(t), மொத்தம்);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_முனை_Memory_Usage >> 20);

        // Node Memory usage
        க்கு (size_t t = 0; t < VNType::இலக்கம்_TYPES(); ++t) {
            எனில் (மாறிலி uint64_t மொத்தம் = m_counters.m_stat_Type_மொத்தம்[t]) {
                மாறிலி இரட்டை share = 100.0 * மொத்தம் * typeSize(t) / total_முனை_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        க்கு (முழு t = 0; t < VBranchPred::_ENUM_END; ++t) {
            எனில் (மாறிலி uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

வெறுமை V3Stats::statsStageAll(AstNetlist* nodep, மாறிலி std::string& stage, தருக்க fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
