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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

वर्ग StatsVisitor final : सार्वजनिक VNVisitorConst {
    संरचना Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::अंक_TYPES()> m_stat_Type_मोजणी{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    स्थिरांक तार्किक m_fastOnly;  // When true, consider only fast functions
    तार्किक m_empty = सत्य;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<स्थिरांक std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    शून्य मोजणी_Then_Iterate_Children(Ast_गाठ* nodep) {
        ++m_accump->m_stat_Type_मोजणी[nodep->type()];
        जर (nodep->type() != VNType::Netlist) m_empty = असत्य;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य visit(AstVar* nodep) override {
        जर (nodep->dtypep()) {
            जर (!nodep->constPoolEntry()) {
                // Count proper variables
                जर (m_statVarWidths.size() <= static_cast<माप_प्रकार>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    जर (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                जर (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        मोजणी_Then_Iterate_Children(nodep);
    }

    शून्य visit(Ast_गाठ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        मोजणी_Then_Iterate_Children(nodep);
    }

    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        जर (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        मोजणी_Then_Iterate_Children(nodep);
    }

    शून्य visit(Ast_गाठ* nodep) override { मोजणी_Then_Iterate_Children(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, स्थिरांक std::string& stage, तार्किक fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        जर (m_empty) परत;

        // Shorthand
        स्थिरांक स्वतः addStat = [&](स्थिरांक std::string& name, द्विगुण मोजणी, अचिन्हित precision = 0) {
            V3Stats::addStat(stage, name, मोजणी, precision);
        };

        // Variable widths
        साठी (अचिन्हित i = 0; i < m_statVarWidths.size(); i++) {
            जर (स्थिरांक uint64_t मोजणी = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                स्थिरांक std::string prefix = ss.str();
                जर (v3Global.opt.statsVars()) {
                    साठी (स्थिरांक स्वतः& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } अन्यथा {
                    addStat(prefix, मोजणी);
                }
            }
        }

        // Node types (also total memory usage)
        स्थिरांक स्वतः typeName
            = [](माप_प्रकार t) { परत std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        स्थिरांक स्वतः typeSize
            = [](माप_प्रकार t) { परत VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        माप_प्रकार total_गाठ_Memory_Usage = 0;
        साठी (माप_प्रकार t = 0; t < VNType::अंक_TYPES(); ++t) {
            जर (स्थिरांक uint64_t मोजणी = m_counters.m_stat_Type_मोजणी[t]) {
                total_गाठ_Memory_Usage += मोजणी * typeSize(t);
                addStat("Node count, " + typeName(t), मोजणी);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_गाठ_Memory_Usage >> 20);

        // Node Memory usage
        साठी (माप_प्रकार t = 0; t < VNType::अंक_TYPES(); ++t) {
            जर (स्थिरांक uint64_t मोजणी = m_counters.m_stat_Type_मोजणी[t]) {
                स्थिरांक द्विगुण share = 100.0 * मोजणी * typeSize(t) / total_गाठ_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        साठी (पूर्णांक t = 0; t < VBranchPred::_ENUM_END; ++t) {
            जर (स्थिरांक uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

शून्य V3Stats::statsStageAll(AstNetlist* nodep, स्थिरांक std::string& stage, तार्किक fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
