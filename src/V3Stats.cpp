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

جماعت StatsVisitor final : عوامی VNVisitorConst {
    ساخت Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::عدد_TYPES()> m_stat_Type_گنتی{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_انتہا> m_statPred{};
    };

    // STATE
    ثابت منطقی m_fastOnly;  // When true, consider only fast functions
    منطقی m_خالی_خانے = سچ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::سمتیہ<uint64_t> m_statVarWidths;  // Variables of given width
    std::سمتیہ<std::map<ثابت std::string, uint32_t>>
        m_stat_Var_چوڑائی_نام;  // Var names of given width

    // METHODS
    خالی گنتی_Then_Iterate_Children(Ast_گرہ* nodep) {
        ++m_accump->m_stat_Type_گنتی[nodep->type()];
        اگر (nodep->type() != VNType::Netlist) m_خالی_خانے = جھوٹ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    خالی دورہ(AstVar* nodep) override {
        اگر (nodep->dtypep()) {
            اگر (!nodep->const_Pool_اندراج()) {
                // Count proper variables
                اگر (m_statVarWidths.سائز() <= static_cast<سائز_t>(nodep->چوڑائی())) {
                    m_statVarWidths.resize(nodep->چوڑائی() + 5);
                    اگر (v3Global.opt.statsVars()) {  //
                        m_stat_Var_چوڑائی_نام.resize(nodep->چوڑائی() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->چوڑائی());
                اگر (v3Global.opt.statsVars()) {
                    ++m_stat_Var_چوڑائی_نام.at(nodep->چوڑائی())[nodep->prettyName()];
                }
            }
        }

        گنتی_Then_Iterate_Children(nodep);
    }

    خالی دورہ(Ast_گرہ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        گنتی_Then_Iterate_Children(nodep);
    }

    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        اگر (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        گنتی_Then_Iterate_Children(nodep);
    }

    خالی دورہ(Ast_گرہ* nodep) override { گنتی_Then_Iterate_Children(nodep); }

عوامی:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقی fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        اگر (m_خالی_خانے) واپس;

        // Shorthand
        ثابت خودکار جوڑو_Stat = [&](ثابت std::string& name, دوہرا گنتی, غیرعلامتی precision = 0) {
            V3Stats::جوڑو_Stat(stage, name, گنتی, precision);
        };

        // Variable widths
        برائے (غیرعلامتی i = 0; i < m_statVarWidths.سائز(); i++) {
            اگر (ثابت uint64_t گنتی = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string سابقہ = ss.str();
                اگر (v3Global.opt.statsVars()) {
                    برائے (ثابت خودکار& it : m_stat_Var_چوڑائی_نام.at(i)) {
                        جوڑو_Stat(سابقہ + " " + it.پہلا, it.دوسرا);
                    }
                } ورنہ {
                    جوڑو_Stat(سابقہ, گنتی);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت خودکار typeName
            = [](سائز_t t) { واپس std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت خودکار type_سائز
            = [](سائز_t t) { واپس VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        سائز_t کل_گرہ_حافظہ_Usage = 0;
        برائے (سائز_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            اگر (ثابت uint64_t گنتی = m_counters.m_stat_Type_گنتی[t]) {
                کل_گرہ_حافظہ_Usage += گنتی * type_سائز(t);
                جوڑو_Stat("Node count, " + typeName(t), گنتی);
            }
        }
        جوڑو_Stat("Node mem TOTAL (MiB)", کل_گرہ_حافظہ_Usage >> 20);

        // Node Memory usage
        برائے (سائز_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            اگر (ثابت uint64_t گنتی = m_counters.m_stat_Type_گنتی[t]) {
                ثابت دوہرا share = 100.0 * گنتی * type_سائز(t) / کل_گرہ_حافظہ_Usage;
                جوڑو_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        برائے (صحیح t = 0; t < VBranchPred::ENUM_انتہا; ++t) {
            اگر (ثابت uint64_t c = m_counters.m_statPred[t]) {
                جوڑو_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

خالی V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقی fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
