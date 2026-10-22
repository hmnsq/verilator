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

מחלקה StatsVisitor final : عام VNVisitorConst {
    بنية Counters final {
        // Nodes of given type
        std::array<uint64_t, VNType::NUM_TYPES()> m_statTypeCount{};
        // Prediction of given type
        std::array<uint64_t, VBranchPred::_ENUM_END> m_statPred{};
    };

    // STATE
    ثابت منطقي m_fastOnly;  // When true, consider only fast functions
    منطقي m_empty = אמת;  // Netlist is empty
    Counters m_counters;  // The actual counts we will display
    Counters m_dumpster;  // Alternate buffer to make discarding parts of the tree easier
    Counters* m_accump;  // The currently active accumulator
    std::vector<uint64_t> m_statVarWidths;  // Variables of given width
    std::vector<std::map<ثابت std::string, uint32_t>>
        m_statVarWidthNames;  // Var names of given width

    // METHODS
    فراغ countThenIterateChildren(AstNode* nodep) {
        ++m_accump->m_statTypeCount[nodep->type()];
        אם (nodep->type() != VNType::Netlist) m_empty = שקר;
        iterateChildrenConst(nodep);
    }

    // VISITORS
    فراغ visit(AstVar* nodep) override {
        אם (nodep->dtypep()) {
            אם (!nodep->constPoolEntry()) {
                // Count proper variables
                אם (m_statVarWidths.size() <= static_cast<size_t>(nodep->width())) {
                    m_statVarWidths.resize(nodep->width() + 5);
                    אם (v3Global.opt.statsVars()) {  //
                        m_statVarWidthNames.resize(nodep->width() + 5);
                    }
                }
                ++m_statVarWidths.at(nodep->width());
                אם (v3Global.opt.statsVars()) {
                    ++m_statVarWidthNames.at(nodep->width())[nodep->prettyName()];
                }
            }
        }

        countThenIterateChildren(nodep);
    }

    فراغ visit(AstNodeIf* nodep) override {
        // Track prediction
        ++m_accump->m_statPred[nodep->branchPred()];
        countThenIterateChildren(nodep);
    }

    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_accump);
        אם (m_fastOnly && !nodep->slow()) m_accump = &m_counters;
        countThenIterateChildren(nodep);
    }

    فراغ visit(AstNode* nodep) override { countThenIterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    StatsVisitor(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly)
        : m_fastOnly{fastOnly}
        , m_accump{fastOnly ? &m_dumpster : &m_counters} {
        UINFO(9, "Starting stats, fastOnly=" << fastOnly);
        iterateConst(nodep);
        אם (m_empty) החזר;

        // Shorthand
        ثابت auto addStat = [&](ثابت std::string& name, مزدوج count, unsigned precision = 0) {
            V3Stats::addStat(stage, name, count, precision);
        };

        // Variable widths
        עבור (unsigned i = 0; i < m_statVarWidths.size(); i++) {
            אם (ثابت uint64_t count = m_statVarWidths.at(i)) {
                std::stringstream ss;
                ss << "Vars, width " << std::setw(5) << std::dec << i;
                ثابت std::string prefix = ss.str();
                אם (v3Global.opt.statsVars()) {
                    עבור (ثابت auto& it : m_statVarWidthNames.at(i)) {
                        addStat(prefix + " " + it.first, it.second);
                    }
                } אחרת {
                    addStat(prefix, count);
                }
            }
        }

        // Node types (also total memory usage)
        ثابت auto typeName
            = [](size_t t) { החזר std::string{VNType{static_cast<VNType::en>(t)}.ascii()}; };
        ثابت auto typeSize
            = [](size_t t) { החזר VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof; };
        size_t totalNodeMemoryUsage = 0;
        עבור (size_t t = 0; t < VNType::NUM_TYPES(); ++t) {
            אם (ثابت uint64_t count = m_counters.m_statTypeCount[t]) {
                totalNodeMemoryUsage += count * typeSize(t);
                addStat("Node count, " + typeName(t), count);
            }
        }
        addStat("Node mem TOTAL (MiB)", totalNodeMemoryUsage >> 20);

        // Node Memory usage
        עבור (size_t t = 0; t < VNType::NUM_TYPES(); ++t) {
            אם (ثابت uint64_t count = m_counters.m_statTypeCount[t]) {
                ثابت مزدوج share = 100.0 * count * typeSize(t) / totalNodeMemoryUsage;
                addStat("Node mem %, " + typeName(t), share, 2);
            }
        }

        // Branch predictions
        עבור (صحيح t = 0; t < VBranchPred::_ENUM_END; ++t) {
            אם (ثابت uint64_t c = m_counters.m_statPred[t]) {
                addStat("Branch prediction, "s + VBranchPred{t}.ascii(), c);
            }
        }
    }
};

//######################################################################
// Top Stats class

فراغ V3Stats::statsStageAll(AstNetlist* nodep, ثابت std::string& stage, منطقي fastOnly) {
    StatsVisitor{nodep, stage, fastOnly};
}
