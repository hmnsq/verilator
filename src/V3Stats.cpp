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
        std::array<uint64_t, VNType::عدد_TYPES()> m_stat_Type_گنتی{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_سرا> m_statPred{};
    };

    // STATE
    ثابت منطقي m_fastOnly;  // When true, consider only fast functions
    منطقي m_خالی_تھاواں = سچ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::سمتیہ<uint64_t> m_statVarWidths;  // Variables of given width
    std::سمتیہ<std::map<ثابت std::string, uint32_t>>
        m_stat_Var_چوڑائی_ناں;  // Var names of given width

    // METHODS
    فراغ گنتی_Then_Iterate_Children(Ast_گرہ* nodep) {
        ++m_accump->m_stat_Type_گنتی[nodep->type()];
        إذا (nodep->type() != VNType::Netlist) m_خالی_تھاواں = خطأ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ ویکھو(AstVar* nodep) override {
        إذا (nodep->dtypep()) {
            إذا (!nodep->const_Pool_اندراج()) {
                // Count proper variables
                إذا (m_statVarWidths.ناپ() <= static_cast<ناپ_t>(nodep->چوڑائی())) {
                    m_statVarWidths.resize(nodep->چوڑائی() + 5);
                    إذا (v3Global.opt.statsVars()) {  //
                        m_stat_Var_چوڑائی_ناں.resize(nodep->چوڑائی() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->چوڑائی());
                إذا (v3Global.opt.statsVars()) {
                    ++m_stat_Var_چوڑائی_ناں.at(nodep->چوڑائی())[nodep->prettyName()];
                }
            }
        }

        گنتی_Then_Iterate_Children(nodep);
    }

    فراغ ویکھو(Ast_گرہ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        گنتی_Then_Iterate_Children(nodep);
    }

    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        إذا (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        گنتی_Then_Iterate_Children(nodep);
    }

    فراغ ویکھو(Ast_گرہ* nodep) override { گنتی_Then_Iterate_Children(nodep); }

عام:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        إذا (m_خالی_تھاواں) أرجع;

        // Shorthand
        ثابت خودکار جوڑو_Stat = [&](ثابت std::string& name, مزدوج گنتی, بنانشانی precision = 0) {
            V3Stats::جوڑو_Stat(stage, name, گنتی, precision);
        };

        // Variable widths
        لكل (بنانشانی i = 0; i < m_statVarWidths.ناپ(); i++) {
            إذا (ثابت uint64_t گنتی = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string اگیتر = ss.str();
                إذا (v3Global.opt.statsVars()) {
                    لكل (ثابت خودکار& it : m_stat_Var_چوڑائی_ناں.at(i)) {
                        جوڑو_Stat(اگیتر + " " + it.پہلا, it.دوجا);
                    }
                } وإلا {
                    جوڑو_Stat(اگیتر, گنتی);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت خودکار typeName
            = [](ناپ_t t) { أرجع std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت خودکار type_ناپ
            = [](ناپ_t t) { أرجع VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        ناپ_t کل_گرہ_حافظہ_Usage = 0;
        لكل (ناپ_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            إذا (ثابت uint64_t گنتی = m_counters.m_stat_Type_گنتی[t]) {
                کل_گرہ_حافظہ_Usage += گنتی * type_ناپ(t);
                جوڑو_Stat("Node count, " + typeName(t), گنتی);
            }
        }
        جوڑو_Stat("Node mem TOTAL (MiB)", کل_گرہ_حافظہ_Usage >> 20);

        // Node Memory usage
        لكل (ناپ_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            إذا (ثابت uint64_t گنتی = m_counters.m_stat_Type_گنتی[t]) {
                ثابت مزدوج share = 100.0 * گنتی * type_ناپ(t) / کل_گرہ_حافظہ_Usage;
                جوڑو_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        لكل (صحيح t = 0; t < VBranchPred::ENUM_سرا; ++t) {
            إذا (ثابت uint64_t c = m_counters.m_statPred[t]) {
                جوڑو_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

فراغ V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
