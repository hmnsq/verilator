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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

വർഗ്ഗം StatsVisitor final : പൊതു VNVisitorConst {
    ഘടന Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::അക്കം_TYPES()> m_stat_Type_എണ്ണം{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    സ്ഥിരം തർക്കം m_fastOnly;  // When true, consider only fast functions
    തർക്കം m_empty = സത്യം;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<സ്ഥിരം std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    ശൂന്യം എണ്ണം_Then_Iterate_Children(Ast_കണ്ണി* nodep) {
        ++m_accump->m_stat_Type_എണ്ണം[nodep->type()];
        എങ്കിൽ (nodep->type() != VNType::Netlist) m_empty = അസത്യം;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ശൂന്യം visit(AstVar* nodep) override {
        എങ്കിൽ (nodep->dtypep()) {
            എങ്കിൽ (!nodep->constPoolEntry()) {
                // Count proper variables
                എങ്കിൽ (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    എങ്കിൽ (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                എങ്കിൽ (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        എണ്ണം_Then_Iterate_Children(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        എണ്ണം_Then_Iterate_Children(nodep);
    }

    ശൂന്യം visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        എങ്കിൽ (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        എണ്ണം_Then_Iterate_Children(nodep);
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { എണ്ണം_Then_Iterate_Children(nodep); }

പൊതു:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, സ്ഥിരം std::string& stage, തർക്കം fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        എങ്കിൽ (m_empty) തിരികെ;

        // Shorthand
        സ്ഥിരം auto addStat = [&](സ്ഥിരം std::string& name, ഇരട്ട എണ്ണം, unsigned precision = 0) {
            V3Stats::addStat(stage, name, എണ്ണം, precision);
        };

        // Variable widths
        വേണ്ടി (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            എങ്കിൽ (സ്ഥിരം uint64_t എണ്ണം = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                സ്ഥിരം std::string prefix = ss.str();
                എങ്കിൽ (v3Global.opt.statsVars()) {
                    വേണ്ടി (സ്ഥിരം auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } അല്ലെങ്കിൽ {
                    addStat(prefix, എണ്ണം);
                }
            }
        }

        // Node types (also total memory usage)
        സ്ഥിരം auto typeName
            = [](size_t t) { തിരികെ std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        സ്ഥിരം auto typeSize
            = [](size_t t) { തിരികെ VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_കണ്ണി_Memory_Usage = 0;
        വേണ്ടി (size_t t = 0; t < VNType::അക്കം_TYPES(); ++t) {
            എങ്കിൽ (സ്ഥിരം uint64_t എണ്ണം = m_counters.m_stat_Type_എണ്ണം[t]) {
                total_കണ്ണി_Memory_Usage += എണ്ണം * typeSize(t);
                addStat("Node count, " + typeName(t), എണ്ണം);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_കണ്ണി_Memory_Usage >> 20);

        // Node Memory usage
        വേണ്ടി (size_t t = 0; t < VNType::അക്കം_TYPES(); ++t) {
            എങ്കിൽ (സ്ഥിരം uint64_t എണ്ണം = m_counters.m_stat_Type_എണ്ണം[t]) {
                സ്ഥിരം ഇരട്ട share = 100.0 * എണ്ണം * typeSize(t) / total_കണ്ണി_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        വേണ്ടി (പൂർണ്ണസംഖ്യ t = 0; t < VBranchPred::_ENUM_END; ++t) {
            എങ്കിൽ (സ്ഥിരം uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

ശൂന്യം V3Stats::statsStageAll(AstNetlist* nodep, സ്ഥിരം std::string& stage, തർക്കം fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
