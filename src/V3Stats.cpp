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
        std::array<uint64_t, VNType::انگ_TYPES()> m_stat_Type_ڳڻپ{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_پڇاڙي> m_statPred{};
    };

    // STATE
    ثابت منطقي m_fastOnly;  // When true, consider only fast functions
    منطقي m_خالي_جايون = سچ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::ويڪٽر<uint64_t> m_statVarWidths;  // Variables of given width
    std::ويڪٽر<std::map<ثابت std::string, uint32_t>>
        m_stat_Var_ويڪر_نالا;  // Var names of given width

    // METHODS
    فراغ ڳڻپ_Then_Iterate_Children(Ast_ڳنڍ* nodep) {
        ++m_accump->m_stat_Type_ڳڻپ[nodep->type()];
        إذا (nodep->type() != VNType::Netlist) m_خالي_جايون = خطأ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ گهمو(AstVar* nodep) override {
        إذا (nodep->dtypep()) {
            إذا (!nodep->const_Pool_داخلا()) {
                // Count proper variables
                إذا (m_statVarWidths.سائيز() <= static_cast<سائيز_t>(nodep->ويڪر())) {
                    m_statVarWidths.resize(nodep->ويڪر() + 5);
                    إذا (v3Global.opt.statsVars()) {  //
                        m_stat_Var_ويڪر_نالا.resize(nodep->ويڪر() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->ويڪر());
                إذا (v3Global.opt.statsVars()) {
                    ++m_stat_Var_ويڪر_نالا.at(nodep->ويڪر())[nodep->prettyName()];
                }
            }
        }

        ڳڻپ_Then_Iterate_Children(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        ڳڻپ_Then_Iterate_Children(nodep);
    }

    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        إذا (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        ڳڻپ_Then_Iterate_Children(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { ڳڻپ_Then_Iterate_Children(nodep); }

عام:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        إذا (m_خالي_جايون) أرجع;

        // Shorthand
        ثابت خودڪار جوڙيو_Stat = [&](ثابت std::string& name, مزدوج ڳڻپ, بنانشان precision = 0) {
            V3Stats::جوڙيو_Stat(stage, name, ڳڻپ, precision);
        };

        // Variable widths
        لكل (بنانشان i = 0; i < m_statVarWidths.سائيز(); i++) {
            إذا (ثابت uint64_t ڳڻپ = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string اڳياڙي = ss.str();
                إذا (v3Global.opt.statsVars()) {
                    لكل (ثابت خودڪار& it : m_stat_Var_ويڪر_نالا.at(i)) {
                        جوڙيو_Stat(اڳياڙي + " " + it.پهريون, it.ٻيو);
                    }
                } وإلا {
                    جوڙيو_Stat(اڳياڙي, ڳڻپ);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت خودڪار typeName
            = [](سائيز_t t) { أرجع std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت خودڪار type_سائيز
            = [](سائيز_t t) { أرجع VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        سائيز_t ڪل_ڳنڍ_يادگيري_Usage = 0;
        لكل (سائيز_t t = 0; t < VNType::انگ_TYPES(); ++t) {
            إذا (ثابت uint64_t ڳڻپ = m_counters.m_stat_Type_ڳڻپ[t]) {
                ڪل_ڳنڍ_يادگيري_Usage += ڳڻپ * type_سائيز(t);
                جوڙيو_Stat("Node count, " + typeName(t), ڳڻپ);
            }
        }
        جوڙيو_Stat("Node mem TOTAL (MiB)", ڪل_ڳنڍ_يادگيري_Usage >> 20);

        // Node Memory usage
        لكل (سائيز_t t = 0; t < VNType::انگ_TYPES(); ++t) {
            إذا (ثابت uint64_t ڳڻپ = m_counters.m_stat_Type_ڳڻپ[t]) {
                ثابت مزدوج share = 100.0 * ڳڻپ * type_سائيز(t) / ڪل_ڳنڍ_يادگيري_Usage;
                جوڙيو_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        لكل (صحيح t = 0; t < VBranchPred::ENUM_پڇاڙي; ++t) {
            إذا (ثابت uint64_t c = m_counters.m_statPred[t]) {
                جوڙيو_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

فراغ V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
