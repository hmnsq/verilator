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
        std::array<uint64_t, VNType::رقم_TYPES()> m_stat_Type_شمار{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_انتها> m_statPred{};
    };

    // STATE
    ثابت منطقي m_fastOnly;  // When true, consider only fast functions
    منطقي m_خالی = درست;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::وکتور<uint64_t> m_statVarWidths;  // Variables of given width
    std::وکتور<std::map<ثابت std::string, uint32_t>>
        m_stat_Var_عرض_نامها;  // Var names of given width

    // METHODS
    فراغ شمار_Then_Iterate_Children(Ast_گره* nodep) {
        ++m_accump->m_stat_Type_شمار[nodep->type()];
        إذا (nodep->type() != VNType::Netlist) m_خالی = خطأ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ بازدید(AstVar* nodep) override {
        إذا (nodep->dtypep()) {
            إذا (!nodep->const_Pool_مدخل()) {
                // Count proper variables
                إذا (m_statVarWidths.اندازه() <= static_cast<اندازه_t>(nodep->عرض())) {
                    m_statVarWidths.resize(nodep->عرض() + 5);
                    إذا (v3Global.opt.statsVars()) {  //
                        m_stat_Var_عرض_نامها.resize(nodep->عرض() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->عرض());
                إذا (v3Global.opt.statsVars()) {
                    ++m_stat_Var_عرض_نامها.at(nodep->عرض())[nodep->prettyName()];
                }
            }
        }

        شمار_Then_Iterate_Children(nodep);
    }

    فراغ بازدید(Ast_گره_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        شمار_Then_Iterate_Children(nodep);
    }

    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        إذا (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        شمار_Then_Iterate_Children(nodep);
    }

    فراغ بازدید(Ast_گره* nodep) override { شمار_Then_Iterate_Children(nodep); }

عام:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        إذا (m_خالی) أرجع;

        // Shorthand
        ثابت خودکار جمع_کن_Stat = [&](ثابت std::string& name, مزدوج شمار, بیعلامت precision = 0) {
            V3Stats::جمع_کن_Stat(stage, name, شمار, precision);
        };

        // Variable widths
        لكل (بیعلامت i = 0; i < m_statVarWidths.اندازه(); i++) {
            إذا (ثابت uint64_t شمار = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string پیشوند = ss.str();
                إذا (v3Global.opt.statsVars()) {
                    لكل (ثابت خودکار& it : m_stat_Var_عرض_نامها.at(i)) {
                        جمع_کن_Stat(پیشوند + " " + it.اولی, it.دومی);
                    }
                } وإلا {
                    جمع_کن_Stat(پیشوند, شمار);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت خودکار typeName
            = [](اندازه_t t) { أرجع std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت خودکار type_اندازه
            = [](اندازه_t t) { أرجع VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        اندازه_t مجموع_گره_حافظه_Usage = 0;
        لكل (اندازه_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            إذا (ثابت uint64_t شمار = m_counters.m_stat_Type_شمار[t]) {
                مجموع_گره_حافظه_Usage += شمار * type_اندازه(t);
                جمع_کن_Stat("Node count, " + typeName(t), شمار);
            }
        }
        جمع_کن_Stat("Node mem TOTAL (MiB)", مجموع_گره_حافظه_Usage >> 20);

        // Node Memory usage
        لكل (اندازه_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            إذا (ثابت uint64_t شمار = m_counters.m_stat_Type_شمار[t]) {
                ثابت مزدوج share = 100.0 * شمار * type_اندازه(t) / مجموع_گره_حافظه_Usage;
                جمع_کن_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        لكل (صحيح t = 0; t < VBranchPred::ENUM_انتها; ++t) {
            إذا (ثابت uint64_t c = m_counters.m_statPred[t]) {
                جمع_کن_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

فراغ V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
