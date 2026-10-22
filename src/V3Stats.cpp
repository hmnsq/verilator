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
        std::array<uint64_t, VNType::अङ्कः_TYPES()> m_stat_Type_गणनम्{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_अन्तः> m_statPred{};
    };

    // STATE
    ध्रुव तार्किक m_fastOnly;  // When true, consider only fast functions
    तार्किक m_रिक्तानि = सत्य;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::सदिशम्<uint64_t> m_statVarWidths;  // Variables of given width
    std::सदिशम्<std::map<ध्रुव std::string, uint32_t>>
        m_stat_Var_विस्तारः_नामानि;  // Var names of given width

    // METHODS
    शून्य गणनम्_Then_Iterate_Children(Ast_ग्रन्थिः* nodep) {
        ++m_accump->m_stat_Type_गणनम्[nodep->type()];
        यदि (nodep->type() != VNType::Netlist) m_रिक्तानि = असत्य;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य दर्शय(AstVar* nodep) override {
        यदि (nodep->dtypep()) {
            यदि (!nodep->const_Pool_प्रविष्टिः()) {
                // Count proper variables
                यदि (m_statVarWidths.आकार() <= static_cast<आकार_t>(nodep->विस्तारः())) {
                    m_statVarWidths.resize(nodep->विस्तारः() + 5);
                    यदि (v3Global.opt.statsVars()) {  //
                        m_stat_Var_विस्तारः_नामानि.resize(nodep->विस्तारः() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->विस्तारः());
                यदि (v3Global.opt.statsVars()) {
                    ++m_stat_Var_विस्तारः_नामानि.at(nodep->विस्तारः())[nodep->prettyName()];
                }
            }
        }

        गणनम्_Then_Iterate_Children(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        गणनम्_Then_Iterate_Children(nodep);
    }

    शून्य दर्शय(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        यदि (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        गणनम्_Then_Iterate_Children(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { गणनम्_Then_Iterate_Children(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ध्रुव std::string& stage, तार्किक fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        यदि (m_रिक्तानि) प्रत्यागम;

        // Shorthand
        ध्रुव स्वचालित सङ्कलय_Stat = [&](ध्रुव std::string& name, द्विगुण गणनम्, अचिह्नित precision = 0) {
            V3Stats::सङ्कलय_Stat(stage, name, गणनम्, precision);
        };

        // Variable widths
        प्रति (अचिह्नित i = 0; i < m_statVarWidths.आकार(); i++) {
            यदि (ध्रुव uint64_t गणनम् = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ध्रुव std::string उपसर्गः = ss.str();
                यदि (v3Global.opt.statsVars()) {
                    प्रति (ध्रुव स्वचालित& it : m_stat_Var_विस्तारः_नामानि.at(i)) {
                        सङ्कलय_Stat(उपसर्गः + " " + it.first, it.second);
                    }
                } अन्यथा {
                    सङ्कलय_Stat(उपसर्गः, गणनम्);
                }
            }
        }

        // Node types (also total memory usage)
        ध्रुव स्वचालित typeName
            = [](आकार_t t) { प्रत्यागम std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ध्रुव स्वचालित type_आकार
            = [](आकार_t t) { प्रत्यागम VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        आकार_t योग_ग्रन्थिः_स्मृतिः_Usage = 0;
        प्रति (आकार_t t = 0; t < VNType::अङ्कः_TYPES(); ++t) {
            यदि (ध्रुव uint64_t गणनम् = m_counters.m_stat_Type_गणनम्[t]) {
                योग_ग्रन्थिः_स्मृतिः_Usage += गणनम् * type_आकार(t);
                सङ्कलय_Stat("Node count, " + typeName(t), गणनम्);
            }
        }
        सङ्कलय_Stat("Node mem TOTAL (MiB)", योग_ग्रन्थिः_स्मृतिः_Usage >> 20);

        // Node Memory usage
        प्रति (आकार_t t = 0; t < VNType::अङ्कः_TYPES(); ++t) {
            यदि (ध्रुव uint64_t गणनम् = m_counters.m_stat_Type_गणनम्[t]) {
                ध्रुव द्विगुण share = 100.0 * गणनम् * type_आकार(t) / योग_ग्रन्थिः_स्मृतिः_Usage;
                सङ्कलय_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        प्रति (पूर्णाङ्क t = 0; t < VBranchPred::ENUM_अन्तः; ++t) {
            यदि (ध्रुव uint64_t c = m_counters.m_statPred[t]) {
                सङ्कलय_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

शून्य V3Stats::statsStageAll(AstNetlist* nodep, ध्रुव std::string& stage, तार्किक fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
