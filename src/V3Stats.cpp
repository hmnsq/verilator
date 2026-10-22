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

#समावेश "V3PchAstMT.h"

#समावेश "V3Stats.h"

#समावेश <iomanip>
#समावेश <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

श्रेणी StatsVisitor final : खुला VNVisitorConst {
    काष्ठा Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::अंक_TYPES()> m_stat_Type_गिनती{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    स्थिर bool m_fastOnly;  // When true, consider only fast functions
    bool m_empty = true;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<स्थिर std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    व्योम गिनती_Then_Iterate_Children(Ast_गाँठ* nodep) {
        ++m_accump->m_stat_Type_गिनती[nodep->type()];
        अगर (nodep->type() != VNType::Netlist) m_empty = false;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    व्योम visit(AstVar* nodep) override {
        अगर (nodep->dtypep()) {
            अगर (!nodep->constPoolEntry()) {
                // Count proper variables
                अगर (m_statVarWidths.आकार() <= static_cast<माप_प्रकार>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    अगर (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                अगर (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        गिनती_Then_Iterate_Children(nodep);
    }

    व्योम visit(Ast_गाँठ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        गिनती_Then_Iterate_Children(nodep);
    }

    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        अगर (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        गिनती_Then_Iterate_Children(nodep);
    }

    व्योम visit(Ast_गाँठ* nodep) override { गिनती_Then_Iterate_Children(nodep); }

खुला:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, स्थिर std::string& stage, bool fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        अगर (m_empty) वापस;

        // Shorthand
        स्थिर स्वतः addStat = [&](स्थिर std::string& name, द्विगुन गिनती, अचिन्हित precision = 0) {
            V3Stats::addStat(stage, name, गिनती, precision);
        };

        // Variable widths
        क्रम (अचिन्हित i = 0; i < m_statVarWidths.आकार(); i++) {
            अगर (स्थिर uint64_t गिनती = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::रखो_चौड़ाई(5) << std::डेसी << i;
                स्थिर std::string prefix = ss.str();
                अगर (v3Global.opt.statsVars()) {
                    क्रम (स्थिर स्वतः& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } अन्यथा {
                    addStat(prefix, गिनती);
                }
            }
        }

        // Node types (also total memory usage)
        स्थिर स्वतः typeName
            = [](माप_प्रकार t) { वापस std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        स्थिर स्वतः type_आकार
            = [](माप_प्रकार t) { वापस VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        माप_प्रकार योग_गाँठ_Memory_Usage = 0;
        क्रम (माप_प्रकार t = 0; t < VNType::अंक_TYPES(); ++t) {
            अगर (स्थिर uint64_t गिनती = m_counters.m_stat_Type_गिनती[t]) {
                योग_गाँठ_Memory_Usage += गिनती * type_आकार(t);
                addStat("Node count, " + typeName(t), गिनती);
            }
        }
        addStat("Node mem TOTAL (MiB)", योग_गाँठ_Memory_Usage >> 20);

        // Node Memory usage
        क्रम (माप_प्रकार t = 0; t < VNType::अंक_TYPES(); ++t) {
            अगर (स्थिर uint64_t गिनती = m_counters.m_stat_Type_गिनती[t]) {
                स्थिर द्विगुन share = 100.0 * गिनती * type_आकार(t) / योग_गाँठ_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        क्रम (पूर्णांक t = 0; t < VBranchPred::_ENUM_END; ++t) {
            अगर (स्थिर uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

व्योम V3Stats::statsStageAll(AstNetlist* nodep, स्थिर std::string& stage, bool fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
