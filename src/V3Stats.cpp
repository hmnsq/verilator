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
    ರಚನೆ Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::ಅಂಕೆ_TYPES()> m_stat_Type_ಎಣಿಕೆ{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_ಕೊನೆ> m_statPred{};
    };

    // STATE
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_fastOnly;  // When true, consider only fast functions
    ತಾರ್ಕಿಕ m_ಖಾಲಿಗಳು = ನಿಜ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::ಸದಿಶ<uint64_t> m_statVarWidths;  // Variables of given width
    std::ಸದಿಶ<std::map<ಸ್ಥಿರಾಂಕ std::string, uint32_t>>
        m_stat_Var_ಅಗಲ_ಹೆಸರುಗಳು;  // Var names of given width

    // METHODS
    ಶೂನ್ಯ ಎಣಿಕೆ_Then_Iterate_Children(Ast_ಗಂಟು* nodep) {
        ++m_accump->m_stat_Type_ಎಣಿಕೆ[nodep->type()];
        ಆದರೆ (nodep->type() != VNType::Netlist) m_ಖಾಲಿಗಳು = ಸುಳ್ಳು;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (nodep->dtypep()) {
            ಆದರೆ (!nodep->const_Pool_ನಮೂದು()) {
                // Count proper variables
                ಆದರೆ (m_statVarWidths.ಗಾತ್ರ_2() <= static_cast<ಗಾತ್ರ_t>(nodep->ಅಗಲ())) {
                    m_statVarWidths.resize(nodep->ಅಗಲ() + 5);
                    ಆದರೆ (v3Global.opt.statsVars()) {  //
                        m_stat_Var_ಅಗಲ_ಹೆಸರುಗಳು.resize(nodep->ಅಗಲ() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->ಅಗಲ());
                ಆದರೆ (v3Global.opt.statsVars()) {
                    ++m_stat_Var_ಅಗಲ_ಹೆಸರುಗಳು.at(nodep->ಅಗಲ())[nodep->prettyName()];
                }
            }
        }

        ಎಣಿಕೆ_Then_Iterate_Children(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        ಎಣಿಕೆ_Then_Iterate_Children(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        ಆದರೆ (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        ಎಣಿಕೆ_Then_Iterate_Children(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { ಎಣಿಕೆ_Then_Iterate_Children(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ಸ್ಥಿರಾಂಕ std::string& stage, ತಾರ್ಕಿಕ fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        ಆದರೆ (m_ಖಾಲಿಗಳು) ಹಿಂತಿರುಗಿ;

        // Shorthand
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೂಡಿಸು_Stat = [&](ಸ್ಥಿರಾಂಕ std::string& name, ದ್ವಿಗುಣ ಎಣಿಕೆ_2, ಚಿಹ್ನೆರಹಿತ precision = 0) {
            V3Stats::ಕೂಡಿಸು_Stat(stage, name, ಎಣಿಕೆ_2, precision);
        };

        // Variable widths
        ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < m_statVarWidths.ಗಾತ್ರ_2(); i++) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t ಎಣಿಕೆ_2 = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ಸ್ಥಿರಾಂಕ std::string ಪೂರ್ವಪ್ರತ್ಯಯ = ss.str();
                ಆದರೆ (v3Global.opt.statsVars()) {
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it : m_stat_Var_ಅಗಲ_ಹೆಸರುಗಳು.at(i)) {
                        ಕೂಡಿಸು_Stat(ಪೂರ್ವಪ್ರತ್ಯಯ + " " + it.ಮೊದಲ, it.ಎರಡನೇ);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಕೂಡಿಸು_Stat(ಪೂರ್ವಪ್ರತ್ಯಯ, ಎಣಿಕೆ_2);
                }
            }
        }

        // Node types (also total memory usage)
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ typeName
            = [](ಗಾತ್ರ_t t) { ಹಿಂತಿರುಗಿ std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ type_ಗಾತ್ರ
            = [](ಗಾತ್ರ_t t) { ಹಿಂತಿರುಗಿ VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        ಗಾತ್ರ_t ಒಟ್ಟು_ಗಂಟು_ಸ್ಮರಣೆ_Usage = 0;
        ಗಾಗಿ (ಗಾತ್ರ_t t = 0; t < VNType::ಅಂಕೆ_TYPES(); ++t) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t ಎಣಿಕೆ_2 = m_counters.m_stat_Type_ಎಣಿಕೆ[t]) {
                ಒಟ್ಟು_ಗಂಟು_ಸ್ಮರಣೆ_Usage += ಎಣಿಕೆ_2 * type_ಗಾತ್ರ(t);
                ಕೂಡಿಸು_Stat("Node count, " + typeName(t), ಎಣಿಕೆ_2);
            }
        }
        ಕೂಡಿಸು_Stat("Node mem TOTAL (MiB)", ಒಟ್ಟು_ಗಂಟು_ಸ್ಮರಣೆ_Usage >> 20);

        // Node Memory usage
        ಗಾಗಿ (ಗಾತ್ರ_t t = 0; t < VNType::ಅಂಕೆ_TYPES(); ++t) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t ಎಣಿಕೆ_2 = m_counters.m_stat_Type_ಎಣಿಕೆ[t]) {
                ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ share = 100.0 * ಎಣಿಕೆ_2 * type_ಗಾತ್ರ(t) / ಒಟ್ಟು_ಗಂಟು_ಸ್ಮರಣೆ_Usage;
                ಕೂಡಿಸು_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ t = 0; t < VBranchPred::ENUM_ಕೊನೆ; ++t) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t c = m_counters.m_statPred[t]) {
                ಕೂಡಿಸು_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

ಶೂನ್ಯ V3Stats::statsStageAll(AstNetlist* nodep, ಸ್ಥಿರಾಂಕ std::string& stage, ತಾರ್ಕಿಕ fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
