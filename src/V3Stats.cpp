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
        std::array<uint64_t, VNType::عدد_TYPES()> m_stat_Type_شمېر{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_پای_ټکی> m_statPred{};
    };

    // STATE
    ثابت منطقي m_fastOnly;  // When true, consider only fast functions
    منطقي m_تشه = ریښتیا;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::ویکټور<uint64_t> m_statVarWidths;  // Variables of given width
    std::ویکټور<std::map<ثابت std::string, uint32_t>>
        m_stat_Var_پلنوالی_نومونه;  // Var names of given width

    // METHODS
    فراغ شمېر_Then_Iterate_Children(Ast_غوټه* nodep) {
        ++m_accump->m_stat_Type_شمېر[nodep->type()];
        إذا (nodep->type() != VNType::Netlist) m_تشه = خطأ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ لیدنه(AstVar* nodep) override {
        إذا (nodep->dtypep()) {
            إذا (!nodep->const_Pool_ثبت()) {
                // Count proper variables
                إذا (m_statVarWidths.اندازه() <= static_cast<اندازه_t>(nodep->پلنوالی())) {
                    m_statVarWidths.resize(nodep->پلنوالی() + 5);
                    إذا (v3Global.opt.statsVars()) {  //
                        m_stat_Var_پلنوالی_نومونه.resize(nodep->پلنوالی() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->پلنوالی());
                إذا (v3Global.opt.statsVars()) {
                    ++m_stat_Var_پلنوالی_نومونه.at(nodep->پلنوالی())[nodep->prettyName()];
                }
            }
        }

        شمېر_Then_Iterate_Children(nodep);
    }

    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        شمېر_Then_Iterate_Children(nodep);
    }

    فراغ لیدنه(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        إذا (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        شمېر_Then_Iterate_Children(nodep);
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { شمېر_Then_Iterate_Children(nodep); }

عام:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        إذا (m_تشه) أرجع;

        // Shorthand
        ثابت خپلکاری جمع_کړه_Stat = [&](ثابت std::string& name, مزدوج شمېر, بینښه precision = 0) {
            V3Stats::جمع_کړه_Stat(stage, name, شمېر, precision);
        };

        // Variable widths
        لكل (بینښه i = 0; i < m_statVarWidths.اندازه(); i++) {
            إذا (ثابت uint64_t شمېر = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string مختاړی = ss.str();
                إذا (v3Global.opt.statsVars()) {
                    لكل (ثابت خپلکاری& it : m_stat_Var_پلنوالی_نومونه.at(i)) {
                        جمع_کړه_Stat(مختاړی + " " + it.لومړی, it.دوهم);
                    }
                } وإلا {
                    جمع_کړه_Stat(مختاړی, شمېر);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت خپلکاری typeName
            = [](اندازه_t t) { أرجع std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت خپلکاری type_اندازه
            = [](اندازه_t t) { أرجع VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        اندازه_t ټول_غوټه_حافظه_Usage = 0;
        لكل (اندازه_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            إذا (ثابت uint64_t شمېر = m_counters.m_stat_Type_شمېر[t]) {
                ټول_غوټه_حافظه_Usage += شمېر * type_اندازه(t);
                جمع_کړه_Stat("Node count, " + typeName(t), شمېر);
            }
        }
        جمع_کړه_Stat("Node mem TOTAL (MiB)", ټول_غوټه_حافظه_Usage >> 20);

        // Node Memory usage
        لكل (اندازه_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            إذا (ثابت uint64_t شمېر = m_counters.m_stat_Type_شمېر[t]) {
                ثابت مزدوج share = 100.0 * شمېر * type_اندازه(t) / ټول_غوټه_حافظه_Usage;
                جمع_کړه_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        لكل (صحيح t = 0; t < VBranchPred::ENUM_پای_ټکی; ++t) {
            إذا (ثابت uint64_t c = m_counters.m_statPred[t]) {
                جمع_کړه_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

فراغ V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
