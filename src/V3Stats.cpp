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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

ವರ್ಗ StatsVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    ಸಂರಚನಾ Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::ಅಂಕೆ_TYPES()> m_stat_Type_ಎಣಿಕೆ{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ಧ್ರುವ ತಾರ್ಕಿಕ m_fastOnly;  // When true, consider only fast functions
    ತಾರ್ಕಿಕ m_empty = ಸತ್ಯ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ಧ್ರುವ std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    ಶೂನ್ಯ ಎಣಿಕೆ_Then_Iterate_Children(Ast_ಗಂಟು* nodep) {
        ++m_accump->m_stat_Type_ಎಣಿಕೆ[nodep->type()];
        ಯದಿ (nodep->type() != VNType::Netlist) m_empty = ಅಸತ್ಯ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (nodep->dtypep()) {
            ಯದಿ (!nodep->constPoolEntry()) {
                // Count proper variables
                ಯದಿ (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    ಯದಿ (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                ಯದಿ (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        ಎಣಿಕೆ_Then_Iterate_Children(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        ಎಣಿಕೆ_Then_Iterate_Children(nodep);
    }

    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        ಯದಿ (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        ಎಣಿಕೆ_Then_Iterate_Children(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { ಎಣಿಕೆ_Then_Iterate_Children(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ಧ್ರುವ std::string& stage, ತಾರ್ಕಿಕ fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        ಯದಿ (m_empty) ಪ್ರತ್ಯಾಗಮ;

        // Shorthand
        ಧ್ರುವ auto addStat = [&](ಧ್ರುವ std::string& name, ದ್ವಿಗುಣ ಎಣಿಕೆ, unsigned precision = 0) {
            V3Stats::addStat(stage, name, ಎಣಿಕೆ, precision);
        };

        // Variable widths
        ಪ್ರತಿ (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            ಯದಿ (ಧ್ರುವ uint64_t ಎಣಿಕೆ = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ಧ್ರುವ std::string prefix = ss.str();
                ಯದಿ (v3Global.opt.statsVars()) {
                    ಪ್ರತಿ (ಧ್ರುವ auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } ಅನ್ಯಥಾ {
                    addStat(prefix, ಎಣಿಕೆ);
                }
            }
        }

        // Node types (also total memory usage)
        ಧ್ರುವ auto typeName
            = [](size_t t) { ಪ್ರತ್ಯಾಗಮ std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ಧ್ರುವ auto typeSize
            = [](size_t t) { ಪ್ರತ್ಯಾಗಮ VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t total_ಗಂಟು_Memory_Usage = 0;
        ಪ್ರತಿ (size_t t = 0; t < VNType::ಅಂಕೆ_TYPES(); ++t) {
            ಯದಿ (ಧ್ರುವ uint64_t ಎಣಿಕೆ = m_counters.m_stat_Type_ಎಣಿಕೆ[t]) {
                total_ಗಂಟು_Memory_Usage += ಎಣಿಕೆ * typeSize(t);
                addStat("Node count, " + typeName(t), ಎಣಿಕೆ);
            }
        }
        addStat("Node mem TOTAL (MiB)", total_ಗಂಟು_Memory_Usage >> 20);

        // Node Memory usage
        ಪ್ರತಿ (size_t t = 0; t < VNType::ಅಂಕೆ_TYPES(); ++t) {
            ಯದಿ (ಧ್ರುವ uint64_t ಎಣಿಕೆ = m_counters.m_stat_Type_ಎಣಿಕೆ[t]) {
                ಧ್ರುವ ದ್ವಿಗುಣ share = 100.0 * ಎಣಿಕೆ * typeSize(t) / total_ಗಂಟು_Memory_Usage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ t = 0; t < VBranchPred::_ENUM_END; ++t) {
            ಯದಿ (ಧ್ರುವ uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

ಶೂನ್ಯ V3Stats::statsStageAll(AstNetlist* nodep, ಧ್ರುವ std::string& stage, ತಾರ್ಕಿಕ fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
