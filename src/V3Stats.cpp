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
        std::array<uint64_t, VNType::अंक_TYPES()> m_stat_Type_गिनती{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ध्रुव तार्किक m_fastOnly;  // When true, consider only fast functions
    तार्किक m_empty = सत्य;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ध्रुव std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    शून्य गिनती_Then_Iterate_Children(Ast_गाँठ* nodep) {
        ++m_accump->m_stat_Type_गिनती[nodep->type()];
        यदि (nodep->type() != VNType::Netlist) m_empty = असत्य;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    शून्य visit(AstVar* nodep) override {
        यदि (nodep->dtypep()) {
            यदि (!nodep->constPoolEntry()) {
                // Count proper variables
                यदि (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    यदि (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                यदि (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        गिनती_Then_Iterate_Children(nodep);
    }

    शून्य visit(Ast_गाँठ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        गिनती_Then_Iterate_Children(nodep);
    }

    शून्य visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        यदि (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        गिनती_Then_Iterate_Children(nodep);
    }

    शून्य visit(Ast_गाँठ* nodep) override { गिनती_Then_Iterate_Children(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ध्रुव std::string& stage, तार्किक fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        यदि (m_empty) प्रत्यागम;

        // Shorthand
        ध्रुव auto addStat = [&](ध्रुव std::string& name, द्विगुण गिनती, unsigned precision = 0) {
            V3Stats::addStat(stage, name, गिनती, precision);
        };

        // Variable widths
        प्रति (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            यदि (ध्रुव uint64_t गिनती = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ध्रुव std::string prefix = ss.str();
                यदि (v3Global.opt.statsVars()) {
                    प्रति (ध्रुव auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } अन्यथा {
                    addStat(prefix, गिनती);
                }
            }
        }

        // Node types (also total memory usage)
        ध्रुव auto typeName
            = [](size_t t) { प्रत्यागम std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ध्रुव auto typeSize
            = [](size_t t) { प्रत्यागम VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_गाँठ_Memory_Usage = 0;
        प्रति (size_t t = 0; t < VNType::अंक_TYPES(); ++t) {
            यदि (ध्रुव uint64_t गिनती = m_counters.m_stat_Type_गिनती[t]) {
                total_गाँठ_Memory_Usage += गिनती * typeSize(t);
                addStat("Node count, " + typeName(t), गिनती);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_गाँठ_Memory_Usage >> 20);

        // Node Memory usage
        प्रति (size_t t = 0; t < VNType::अंक_TYPES(); ++t) {
            यदि (ध्रुव uint64_t गिनती = m_counters.m_stat_Type_गिनती[t]) {
                ध्रुव द्विगुण share = 100.0 * गिनती * typeSize(t) / total_गाँठ_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        प्रति (पूर्णाङ्क t = 0; t < VBranchPred::_ENUM_END; ++t) {
            यदि (ध्रुव uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

शून्य V3Stats::statsStageAll(AstNetlist* nodep, ध्रुव std::string& stage, तार्किक fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
