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

#ضمن "V3PchAstMT.h"

#ضمن "V3Stats.h"

#ضمن <iomanip>
#ضمن <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

صنف StatsVisitor final : عام VNVisitorConst {
    بنية Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::رقم_TYPES()> m_stat_Type_تعداد{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ثابت منطقي m_fastOnly;  // When true, consider only fast functions
    منطقي m_empty = true;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ثابت std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    فراغ تعداد_Then_Iterate_Children(Ast_عقدة* nodep) {
        ++m_accump->m_stat_Type_تعداد[nodep->type()];
        إذا (nodep->type() != VNType::Netlist) m_empty = خطأ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ visit(AstVar* nodep) override {
        إذا (nodep->dtypep()) {
            إذا (!nodep->constPoolEntry()) {
                // Count proper variables
                إذا (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    إذا (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                إذا (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        تعداد_Then_Iterate_Children(nodep);
    }

    فراغ visit(Ast_عقدة_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        تعداد_Then_Iterate_Children(nodep);
    }

    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        إذا (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        تعداد_Then_Iterate_Children(nodep);
    }

    فراغ visit(Ast_عقدة* nodep) override { تعداد_Then_Iterate_Children(nodep); }

عام:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        إذا (m_empty) إرجاع;

        // Shorthand
        ثابت auto addStat = [&](ثابت std::string& name, مزدوج تعداد_2, unsigned precision = 0) {
            V3Stats::addStat(stage, name, تعداد_2, precision);
        };

        // Variable widths
        لكل (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            إذا (ثابت uint64_t تعداد_2 = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string prefix = ss.str();
                إذا (v3Global.opt.statsVars()) {
                    لكل (ثابت auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } وإلا {
                    addStat(prefix, تعداد_2);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت auto typeName
            = [](size_t t) { إرجاع std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت auto typeSize
            = [](size_t t) { إرجاع VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_عقدة_Memory_Usage = 0;
        لكل (size_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            إذا (ثابت uint64_t تعداد_2 = m_counters.m_stat_Type_تعداد[t]) {
                total_عقدة_Memory_Usage += تعداد_2 * typeSize(t);
                addStat("Node count, " + typeName(t), تعداد_2);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_عقدة_Memory_Usage >> 20);

        // Node Memory usage
        لكل (size_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            إذا (ثابت uint64_t تعداد_2 = m_counters.m_stat_Type_تعداد[t]) {
                ثابت مزدوج share = 100.0 * تعداد_2 * typeSize(t) / total_عقدة_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        لكل (صحيح t = 0; t < VBranchPred::_ENUM_END; ++t) {
            إذا (ثابت uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

فراغ V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
