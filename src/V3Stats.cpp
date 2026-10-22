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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Statics

V3Mutex V3Stats::s_mutex;

//######################################################################
// Stats class functions

ਵਰਗ StatsVisitor final : ਜਨਤਕ VNVisitorConst {
    ਬਣਤਰ Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::ਅੰਕ_TYPES()> m_stat_Type_ਗਿਣਤੀ{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::ENUM_ਸਿਰਾ> m_statPred{};
    };

    // STATE
    ਅਚਲ ਤਾਰਕਿਕ m_fastOnly;  // When true, consider only fast functions
    ਤਾਰਕਿਕ m_ਖਾਲੀ_ਥਾਵਾਂ = ਸੱਚ;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::ਸਦਿਸ਼<uint64_t> m_statVarWidths;  // Variables of given width
    std::ਸਦਿਸ਼<std::map<ਅਚਲ std::string, uint32_t>>
        m_stat_Var_ਚੌੜਾਈ_ਨਾਂ;  // Var names of given width

    // METHODS
    ਖਾਲੀ ਗਿਣਤੀ_Then_Iterate_Children(Ast_ਗੰਢ* nodep) {
        ++m_accump->m_stat_Type_ਗਿਣਤੀ[nodep->type()];
        ਜੇ (nodep->type() != VNType::Netlist) m_ਖਾਲੀ_ਥਾਵਾਂ = ਝੂਠ;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (nodep->dtypep()) {
            ਜੇ (!nodep->const_Pool_ਇੰਦਰਾਜ()) {
                // Count proper variables
                ਜੇ (m_statVarWidths.ਮਾਪ() <= static_cast<ਮਾਪ_t>(nodep->ਚੌੜਾਈ())) {
                    m_statVarWidths.resize(nodep->ਚੌੜਾਈ() + 5);
                    ਜੇ (v3Global.opt.statsVars()) {  //
                        m_stat_Var_ਚੌੜਾਈ_ਨਾਂ.resize(nodep->ਚੌੜਾਈ() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->ਚੌੜਾਈ());
                ਜੇ (v3Global.opt.statsVars()) {
                    ++m_stat_Var_ਚੌੜਾਈ_ਨਾਂ.at(nodep->ਚੌੜਾਈ())[nodep->prettyName()];
                }
            }
        }

        ਗਿਣਤੀ_Then_Iterate_Children(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        ਗਿਣਤੀ_Then_Iterate_Children(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        ਜੇ (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        ਗਿਣਤੀ_Then_Iterate_Children(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { ਗਿਣਤੀ_Then_Iterate_Children(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ਅਚਲ std::string& stage, ਤਾਰਕਿਕ fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        ਜੇ (m_ਖਾਲੀ_ਥਾਵਾਂ) ਵਾਪਸ;

        // Shorthand
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਜੋੜੋ_Stat = [&](ਅਚਲ std::string& name, ਦੁੱਗਣਾ ਗਿਣਤੀ_2, ਬਿਨਾਨਿਸ਼ਾਨੀ precision = 0) {
            V3Stats::ਜੋੜੋ_Stat(stage, name, ਗਿਣਤੀ_2, precision);
        };

        // Variable widths
        ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < m_statVarWidths.ਮਾਪ(); i++) {
            ਜੇ (ਅਚਲ uint64_t ਗਿਣਤੀ_2 = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ਅਚਲ std::string ਅਗੇਤਰ = ss.str();
                ਜੇ (v3Global.opt.statsVars()) {
                    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& it : m_stat_Var_ਚੌੜਾਈ_ਨਾਂ.at(i)) {
                        ਜੋੜੋ_Stat(ਅਗੇਤਰ + " " + it.ਪਹਿਲਾ, it.ਦੂਜਾ);
                    }
                } ਨਹੀਂਤਾਂ {
                    ਜੋੜੋ_Stat(ਅਗੇਤਰ, ਗਿਣਤੀ_2);
                }
            }
        }

        // Node types (also total memory usage)
        ਅਚਲ ਸਵੈਚਾਲਿਤ typeName
            = [](ਮਾਪ_t t) { ਵਾਪਸ std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ਅਚਲ ਸਵੈਚਾਲਿਤ type_ਮਾਪ
            = [](ਮਾਪ_t t) { ਵਾਪਸ VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        ਮਾਪ_t ਕੁੱਲ_ਗੰਢ_ਸਮ੍ਰਿਤੀ_Usage = 0;
        ਲਈ (ਮਾਪ_t t = 0; t < VNType::ਅੰਕ_TYPES(); ++t) {
            ਜੇ (ਅਚਲ uint64_t ਗਿਣਤੀ_2 = m_counters.m_stat_Type_ਗਿਣਤੀ[t]) {
                ਕੁੱਲ_ਗੰਢ_ਸਮ੍ਰਿਤੀ_Usage += ਗਿਣਤੀ_2 * type_ਮਾਪ(t);
                ਜੋੜੋ_Stat("Node count, " + typeName(t), ਗਿਣਤੀ_2);
            }
        }
        ਜੋੜੋ_Stat("Node mem TOTAL (MiB)", ਕੁੱਲ_ਗੰਢ_ਸਮ੍ਰਿਤੀ_Usage >> 20);

        // Node Memory usage
        ਲਈ (ਮਾਪ_t t = 0; t < VNType::ਅੰਕ_TYPES(); ++t) {
            ਜੇ (ਅਚਲ uint64_t ਗਿਣਤੀ_2 = m_counters.m_stat_Type_ਗਿਣਤੀ[t]) {
                ਅਚਲ ਦੁੱਗਣਾ share = 100.0 * ਗਿਣਤੀ_2 * type_ਮਾਪ(t) / ਕੁੱਲ_ਗੰਢ_ਸਮ੍ਰਿਤੀ_Usage;
                ਜੋੜੋ_Stat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        ਲਈ (ਪੂਰਨ t = 0; t < VBranchPred::ENUM_ਸਿਰਾ; ++t) {
            ਜੇ (ਅਚਲ uint64_t c = m_counters.m_statPred[t]) {
                ਜੋੜੋ_Stat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

ਖਾਲੀ V3Stats::statsStageAll(AstNetlist* nodep, ਅਚਲ std::string& stage, ਤਾਰਕਿਕ fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
