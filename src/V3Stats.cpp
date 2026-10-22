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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3Stats.h"

#অন্তর্ভাৱ <iomanip>
#অন্তর্ভাৱ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

শ্ৰেণী StatsVisitor final : সার্ৱজনিক VNVisitorConst {
    সংরচনা Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::অঙ্ক_TYPES()> m_stat_Type_গুনতি{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ধ্রুৱ তার্কিক m_fastOnly;  // When true, consider only fast functions
    তার্কিক m_empty = সঁচা;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ধ্রুৱ std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    শূন্য গুনতি_Then_Iterate_Children(Ast_গ্ৰন্থি* nodep) {
        ++m_accump->m_stat_Type_গুনতি[nodep->type()];
        যদি (nodep->type() != VNType::Netlist) m_empty = মিছা;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    শূন্য visit(AstVar* nodep) override {
        যদি (nodep->dtypep()) {
            যদি (!nodep->constPoolEntry()) {
                // Count proper variables
                যদি (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    যদি (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                যদি (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        গুনতি_Then_Iterate_Children(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        গুনতি_Then_Iterate_Children(nodep);
    }

    শূন্য visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        যদি (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        গুনতি_Then_Iterate_Children(nodep);
    }

    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { গুনতি_Then_Iterate_Children(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ধ্রুৱ std::string& stage, তার্কিক fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        যদি (m_empty) ঘূৰাই;

        // Shorthand
        ধ্রুৱ auto addStat = [&](ধ্রুৱ std::string& name, দ্ৱিগুণ গুনতি, unsigned precision = 0) {
            V3Stats::addStat(stage, name, গুনতি, precision);
        };

        // Variable widths
        বাবে (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            যদি (ধ্রুৱ uint64_t গুনতি = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ধ্রুৱ std::string prefix = ss.str();
                যদি (v3Global.opt.statsVars()) {
                    বাবে (ধ্রুৱ auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } অন্যথা {
                    addStat(prefix, গুনতি);
                }
            }
        }

        // Node types (also total memory usage)
        ধ্রুৱ auto typeName
            = [](size_t t) { ঘূৰাই std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ধ্রুৱ auto typeSize
            = [](size_t t) { ঘূৰাই VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_গ্ৰন্থি_Memory_Usage = 0;
        বাবে (size_t t = 0; t < VNType::অঙ্ক_TYPES(); ++t) {
            যদি (ধ্রুৱ uint64_t গুনতি = m_counters.m_stat_Type_গুনতি[t]) {
                total_গ্ৰন্থি_Memory_Usage += গুনতি * typeSize(t);
                addStat("Node count, " + typeName(t), গুনতি);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_গ্ৰন্থি_Memory_Usage >> 20);

        // Node Memory usage
        বাবে (size_t t = 0; t < VNType::অঙ্ক_TYPES(); ++t) {
            যদি (ধ্রুৱ uint64_t গুনতি = m_counters.m_stat_Type_গুনতি[t]) {
                ধ্রুৱ দ্ৱিগুণ share = 100.0 * গুনতি * typeSize(t) / total_গ্ৰন্থি_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        বাবে (পূর্ণাঙ্ক t = 0; t < VBranchPred::_ENUM_END; ++t) {
            যদি (ধ্রুৱ uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

শূন্য V3Stats::statsStageAll(AstNetlist* nodep, ধ্রুৱ std::string& stage, তার্কিক fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
