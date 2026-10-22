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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

વર્ગ StatsVisitor final : જાહેર VNVisitorConst {
    સંરચના Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::અંક_TYPES()> m_stat_Type_ગણતરી{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    અચલ તાર્કિક m_fastOnly;  // When true, consider only fast functions
    તાર્કિક m_empty = સાચું;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<અચલ std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    શૂન્ય ગણતરી_Then_Iterate_Children(Ast_ગાંઠ* nodep) {
        ++m_accump->m_stat_Type_ગણતરી[nodep->type()];
        જો (nodep->type() != VNType::Netlist) m_empty = ખોટું;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    શૂન્ય visit(AstVar* nodep) override {
        જો (nodep->dtypep()) {
            જો (!nodep->constPoolEntry()) {
                // Count proper variables
                જો (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    જો (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                જો (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        ગણતરી_Then_Iterate_Children(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        ગણતરી_Then_Iterate_Children(nodep);
    }

    શૂન્ય visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        જો (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        ગણતરી_Then_Iterate_Children(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { ગણતરી_Then_Iterate_Children(nodep); }

જાહેર:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, અચલ std::string& stage, તાર્કિક fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        જો (m_empty) પાછા;

        // Shorthand
        અચલ auto addStat = [&](અચલ std::string& name, દ્વિગુણ ગણતરી, unsigned precision = 0) {
            V3Stats::addStat(stage, name, ગણતરી, precision);
        };

        // Variable widths
        માટે (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            જો (અચલ uint64_t ગણતરી = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                અચલ std::string prefix = ss.str();
                જો (v3Global.opt.statsVars()) {
                    માટે (અચલ auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } નહીંતર {
                    addStat(prefix, ગણતરી);
                }
            }
        }

        // Node types (also total memory usage)
        અચલ auto typeName
            = [](size_t t) { પાછા std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        અચલ auto typeSize
            = [](size_t t) { પાછા VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_ગાંઠ_Memory_Usage = 0;
        માટે (size_t t = 0; t < VNType::અંક_TYPES(); ++t) {
            જો (અચલ uint64_t ગણતરી = m_counters.m_stat_Type_ગણતરી[t]) {
                total_ગાંઠ_Memory_Usage += ગણતરી * typeSize(t);
                addStat("Node count, " + typeName(t), ગણતરી);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_ગાંઠ_Memory_Usage >> 20);

        // Node Memory usage
        માટે (size_t t = 0; t < VNType::અંક_TYPES(); ++t) {
            જો (અચલ uint64_t ગણતરી = m_counters.m_stat_Type_ગણતરી[t]) {
                અચલ દ્વિગુણ share = 100.0 * ગણતરી * typeSize(t) / total_ગાંઠ_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        માટે (પૂર્ણાંક t = 0; t < VBranchPred::_ENUM_END; ++t) {
            જો (અચલ uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

શૂન્ય V3Stats::statsStageAll(AstNetlist* nodep, અચલ std::string& stage, તાર્કિક fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
