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

#شامل "V3PchAstMT.h"

#شامل "V3Stats.h"

#شامل <iomanip>
#شامل <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

رده StatsVisitor final : عمومی VNVisitorConst {
    ساختار Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::رقم_TYPES()> m_stat_Type_شمار{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ثابت منطقی m_fastOnly;  // When true, consider only fast functions
    منطقی m_empty = درست;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ثابت std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    تهی شمار_Then_Iterate_Children(Ast_گره* nodep) {
        ++m_accump->m_stat_Type_شمار[nodep->type()];
        اگر (nodep->type() != VNType::Netlist) m_empty = نادرست;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    تهی visit(AstVar* nodep) override {
        اگر (nodep->dtypep()) {
            اگر (!nodep->constPoolEntry()) {
                // Count proper variables
                اگر (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    اگر (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                اگر (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        شمار_Then_Iterate_Children(nodep);
    }

    تهی visit(Ast_گره_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        شمار_Then_Iterate_Children(nodep);
    }

    تهی visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        اگر (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        شمار_Then_Iterate_Children(nodep);
    }

    تهی visit(Ast_گره* nodep) override { شمار_Then_Iterate_Children(nodep); }

عمومی:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقی fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        اگر (m_empty) بازگشت;

        // Shorthand
        ثابت auto addStat = [&](ثابت std::string& name, دوگانه شمار, unsigned precision = 0) {
            V3Stats::addStat(stage, name, شمار, precision);
        };

        // Variable widths
        برای (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            اگر (ثابت uint64_t شمار = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string prefix = ss.str();
                اگر (v3Global.opt.statsVars()) {
                    برای (ثابت auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } وگرنه {
                    addStat(prefix, شمار);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت auto typeName
            = [](size_t t) { بازگشت std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت auto typeSize
            = [](size_t t) { بازگشت VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_گره_Memory_Usage = 0;
        برای (size_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            اگر (ثابت uint64_t شمار = m_counters.m_stat_Type_شمار[t]) {
                total_گره_Memory_Usage += شمار * typeSize(t);
                addStat("Node count, " + typeName(t), شمار);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_گره_Memory_Usage >> 20);

        // Node Memory usage
        برای (size_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            اگر (ثابت uint64_t شمار = m_counters.m_stat_Type_شمار[t]) {
                ثابت دوگانه share = 100.0 * شمار * typeSize(t) / total_گره_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        برای (صحیح t = 0; t < VBranchPred::_ENUM_END; ++t) {
            اگر (ثابت uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

تهی V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقی fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
