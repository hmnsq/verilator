// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Persistent context for DFG algorithms
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// Various context objects hold data that need to persist across invocations
// of a DFG algorithms.
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3DFGCONTEXT_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3DFGCONTEXT_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPeepholePatterns.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

class V3DfgContext;

//######################################################################
// Base class for all context objects

class V3DfgSubContext VL_NOT_FINAL {
    ಸ್ಥಿರಾಂಕ std::string m_name;  // Pass/algorithm name, to be used in statistics

ಸಂರಕ್ಷಿತ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    V3DfgSubContext(ಸ್ಥಿರಾಂಕ std::string& name)
        : m_name{name} {}

    ಶೂನ್ಯ ಕೂಡಿಸು_Stat(ಸ್ಥಿರಾಂಕ std::string& what, ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, DFG, " + m_name + ", " + what, ಮೌಲ್ಯ);
    }
};

//######################################################################
// Contexts for various algorithms - keep sorted

class V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_inputs;  // Number of input processes (logic constructs)
    VDouble0 m_representable;  // Number of combinational equations representable
    VDouble0 m_nonRepCfg;  // Non-representable due to failing to build CFG
    VDouble0 m_non_Rep_ಜೀವಂತ;  // Non-representable due to failing liveness analysis
    VDouble0 m_nonRepVar;  // Non-representable due to unsupported variable properties

ಖಾಸಗಿ:
    V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context()
        : V3DfgSubContext{"AstToDfg"} {}
    ~V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context() {
        ಕೂಡಿಸು_Stat("input processes", m_inputs);
        ಕೂಡಿಸು_Stat("representable", m_representable);
        ಕೂಡಿಸು_Stat("non-representable (cfg)", m_nonRepCfg);
        ಕೂಡಿಸು_Stat("non-representable (live)", m_non_Rep_ಜೀವಂತ);
        ಕೂಡಿಸು_Stat("non-representable (var)", m_nonRepVar);
        // Check the stats are consistent
        UASSERT(m_representable + m_nonRepCfg + m_non_Rep_ಜೀವಂತ + m_nonRepVar == m_inputs,
                "Inconsistent statistics");
    }
};
class V3_Dfg_Bin_ಗಮ್ಯ_One_Hot_Context final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_decodersCreated;  // Number of bianry to one-hot decoders created

ಖಾಸಗಿ:
    V3_Dfg_Bin_ಗಮ್ಯ_One_Hot_Context()
        : V3DfgSubContext{"BinToOneHot"} {}
    ~V3_Dfg_Bin_ಗಮ್ಯ_One_Hot_Context() { ಕೂಡಿಸು_Stat("decoders created", m_decodersCreated); }
};
class V3_Dfg_Break_ಚಕ್ರಗಳು_Context final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_nFixed;  // Number of graphs that became acyclic
    VDouble0 m_nImproved;  // Number of graphs that were improved but still cyclic
    VDouble0 m_nUnchanged;  // Number of graphs that were left unchanged
    VDouble0 m_nImprovements;  // Number of changes made to graphs
    VDouble0 m_nPrevInserted;  // Number of Prev vertices inserted

ಖಾಸಗಿ:
    V3_Dfg_Break_ಚಕ್ರಗಳು_Context()
        : V3DfgSubContext{"BreakCycles"} {}
    ~V3_Dfg_Break_ಚಕ್ರಗಳು_Context() {
        ಕೂಡಿಸು_Stat("made acyclic", m_nFixed);
        ಕೂಡಿಸು_Stat("improved", m_nImproved);
        ಕೂಡಿಸು_Stat("left unchanged", m_nUnchanged);
        ಕೂಡಿಸು_Stat("changes applied", m_nImprovements);
        ಕೂಡಿಸು_Stat("prev vertices inserted", m_nPrevInserted);
    }
};
class V3DfgCseContext final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_eliminated;  // Number of common sub-expressions eliminated

ಖಾಸಗಿ:
    explicit V3DfgCseContext(ಸ್ಥಿರಾಂಕ std::string& label)
        : V3DfgSubContext{"CSE " + label} {}
    ~V3DfgCseContext() { ಕೂಡಿಸು_Stat("expressions eliminated", m_eliminated); }
};

class V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_outputVariables;  // Number of output variables
    VDouble0 m_outputVariablesWithDefault;  // Number of outptu variables with a default driver
    VDouble0 m_ಫಲಿತಾಂಶ_Equations;  // Number of result combinational equations
    VDouble0 m_expressionsInlined;  // Number of expressions inlined directly into the Ast
    VDouble0 m_varRefsSubstituted;  // Number of variable references substituted in the Ast

ಖಾಸಗಿ:
    V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context()
        : V3DfgSubContext{"DfgToAst"} {}
    ~V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context() {
        ಕೂಡಿಸು_Stat("output variables", m_outputVariables);
        ಕೂಡಿಸು_Stat("output variables with default driver", m_outputVariablesWithDefault);
        ಕೂಡಿಸು_Stat("result equations", m_ಫಲಿತಾಂಶ_Equations);
        ಕೂಡಿಸು_Stat("expressions inlined", m_expressionsInlined);
        ಕೂಡಿಸು_Stat("var refs substituted", m_varRefsSubstituted);
    }
};
class V3DfgPeepholeContext final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    // Enable flags for each optimization
    std::array<bool, VDfg_Peephole_ನಮೂನೆ::ENUM_ಕೊನೆ> m_enabled;
    // Count of applications for each optimization (for statistics)
    std::array<VDouble0, VDfg_Peephole_ನಮೂನೆ::ENUM_ಕೊನೆ> m_ಎಣಿಕೆ;
    std::ಸದಿಶ<Ast_ಗಂಟು*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end

ಖಾಸಗಿ:
    V3DfgPeepholeContext()
        : V3DfgSubContext{"Peephole"} {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ checkEnabled = [this](VDfg_Peephole_ನಮೂನೆ id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.ಕೊನೆ(), str.begin(), [](ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ c) {  //
                ಹಿಂತಿರುಗಿ c == '_' ? '-' : std::tolower(c);
            });
            m_enabled[id] = v3Global.opt.fDfgPeepholeEnabled(str);
        };
#ವ್ಯಾಖ್ಯಾನಿಸು OPTIMIZATION_CHECK_ENABLED(id, name) checkEnabled(VDfg_Peephole_ನಮೂನೆ::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_CHECK_ENABLED)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು OPTIMIZATION_CHECK_ENABLED
    }
    ~V3DfgPeepholeContext() {
        ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        }
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ emitStat = [this](VDfg_Peephole_ನಮೂನೆ id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.ಕೊನೆ(), str.begin(), [](ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ c) {  //
                ಹಿಂತಿರುಗಿ c == '_' ? ' ' : std::tolower(c);
            });
            ಕೂಡಿಸು_Stat(str, m_ಎಣಿಕೆ[id]);
        };
#ವ್ಯಾಖ್ಯಾನಿಸು OPTIMIZATION_EMIT_STATS(id, name) emitStat(VDfg_Peephole_ನಮೂನೆ::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_EMIT_STATS)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು OPTIMIZATION_EMIT_STATS
    }
};
class V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    ಗಾತ್ರ_t m_pushed_ಕೆಳಕ್ಕೆ = 0;  // Number of selects pushed down through concatenations
    ಗಾತ್ರ_t m_wouldBeCyclic = 0;  // Number of selects not pushed due to cycle
ಖಾಸಗಿ:
    V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context()
        : V3DfgSubContext{"PushDownSels"} {}
    ~V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context() {
        ಕೂಡಿಸು_Stat("sels pushed down", m_pushed_ಕೆಳಕ್ಕೆ);
        ಕೂಡಿಸು_Stat("would be cyclic", m_wouldBeCyclic);
    }
};
class V3DfgRegularizeContext final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    std::ಸದಿಶ<Ast_ಗಂಟು*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end
    VDouble0 m_ಬಳಸಿದ_Vars_Inlined;  // Number of used variables inlined
    VDouble0 m_ಬಳಸದ_Removed;  // Number of unused vertices remoevd
    VDouble0 m_temporariesIntroduced;  // Number of temporaries introduced for reuse

ಖಾಸಗಿ:
    V3DfgRegularizeContext()
        : V3DfgSubContext{"Regularize"} {}
    ~V3DfgRegularizeContext() {
        ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
        }
        ಕೂಡಿಸು_Stat("used variables inlined", m_ಬಳಸಿದ_Vars_Inlined);
        ಕೂಡಿಸು_Stat("unused vertices removed", m_ಬಳಸದ_Removed);
        ಕೂಡಿಸು_Stat("temporaries introduced", m_temporariesIntroduced);
    }
};
class V3DfgRemoveSelectsContext final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_removed_ತುಂಬಿದ_ಅಗಲ;  // Number of full width selects removed
    VDouble0 m_replacedWithSelFromDriver;  // Number of selects replaced with sel from a driver
    VDouble0 m_replaced_With_ಸಂಪೂರ್ಣ_Driver;  // Number of selects replaced with a whole driver
ಖಾಸಗಿ:
    V3DfgRemoveSelectsContext()
        : V3DfgSubContext{"RemoveSelects"} {}
    ~V3DfgRemoveSelectsContext() {
        ಕೂಡಿಸು_Stat("full width selects removed", m_removed_ತುಂಬಿದ_ಅಗಲ);
        ಕೂಡಿಸು_Stat("replaced with sel from driver", m_replacedWithSelFromDriver);
        ಕೂಡಿಸು_Stat("replaced with whole driver", m_replaced_With_ಸಂಪೂರ್ಣ_Driver);
    }
};
class V3DfgRemoveUnobservableContext final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE
    VDouble0 m_varsRemoved;  // Number of variables removed from the Dfg
    VDouble0 m_varsDeleted;  // Number of variables removed from the Dfg and the Ast
    VDouble0 m_logicRemoved;  // Number of logic blocks removed from the Dfg
    VDouble0 m_logicDeleted;  // Number of logic blocks removed from the Dfg and the Ast

ಖಾಸಗಿ:
    V3DfgRemoveUnobservableContext()
        : V3DfgSubContext{"RemoveUnobservable"} {}
    ~V3DfgRemoveUnobservableContext() {
        ಕೂಡಿಸು_Stat("variables removed", m_varsRemoved);
        ಕೂಡಿಸು_Stat("variables deleted", m_varsDeleted);
        ಕೂಡಿಸು_Stat("logic removed", m_logicRemoved);
        ಕೂಡಿಸು_Stat("logic deleted", m_logicDeleted);
    }
};
class V3DfgSynthesisContext final : ಸಾರ್ವಜನಿಕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ಮಿತ್ರ class V3DfgContext;

ಸಾರ್ವಜನಿಕ:
    // STATE

    // Stats for conversion
    ರಚನೆ {
        // Inputs
        VDouble0 ಒಳಹರಿವು_Assignments;  // Number of input assignments
        VDouble0 ಒಳಹರಿವು_Expressions;  // Number of input equations
        // Successful
        VDouble0 representable;  // Number of representable constructs
        // Unsuccessful
        VDouble0 nonRepImpure;  // Non representable: impure
        VDouble0 nonRepDType;  // Non representable: unsupported data type
        VDouble0 nonRepLValue;  // Non representable: unsupported LValue form
        VDouble0 nonRepVarRef;  // Non representable: unsupported var reference
        VDouble0 nonRepOOBSel;  // Non representable: out of bounds select
        VDouble0 non_Rep_ಗಂಟು;  // Non representable: unsupported AstNode type
        VDouble0 nonRepUnknown;  // Non representable: unhandled AstNode type
    } m_conv;

    // Stats for synthesis
    ರಚನೆ {
        // Inputs
        VDouble0 ಒಳಹರಿವು_Always;  // Number of always blocks attempted
        VDouble0 ಒಳಹರಿವು_ನಿಯೋಜಿಸು;  // Number of continuous assignments attempted
        // Successful
        VDouble0 synthAlways;  // Number of always blocks successfully synthesized
        VDouble0 synth_ನಿಯೋಜಿಸು;  // Number of continuous assignments successfully synthesized
        // Unsuccessful
        VDouble0 nonSynConv;  // Non-synthesizable: non-representable (above)
        VDouble0 nonSynExtWrite;  // Non-synthesizable: has externally written variable
        VDouble0 nonSynLoop;  // Non-synthesizable: loop in CFG
        VDouble0 nonSynStmt;  // Non-synthesizable: unsupported statement
        VDouble0 nonSynMultidrive;  // Non-synthesizable: multidriven value within statement
        VDouble0 nonSynArray;  // Non-synthesizable: array type unhandled
        VDouble0 nonSynLatch;  // Non-synthesizable: maybe latch
        VDouble0 non_Syn_Join_ಒಳಹರಿವು;  // Non-synthesizable: needing to join input variable
        VDouble0 nonSynFalseWrite;  // Non-synthesizable: does not write output
        // Reverted
        VDouble0 revertNonSyn;  // Reverted due to being driven from non-synthesizable vertex
        VDouble0 revertMultidrive;  // Reverted due to multiple drivers
        // Additional stats
        VDouble0 cfgTrivial;  // Trivial input CFGs
        VDouble0 cfgSp;  // Series-paralel input CFGs
        VDouble0 cfgDag;  // Generic loop free input CFGs
        VDouble0 cfgCyclic;  // Cyclic input CFGs
        VDouble0 join_Using_ಮಾರ್ಗ_Predicate;  // Num control flow joins using full path predicate
        VDouble0 joinUsingBranchCondition;  // Num Control flow joins using dominating branch cond
    } m_synt;

ಖಾಸಗಿ:
    V3DfgSynthesisContext()
        : V3DfgSubContext{"Synthesis"} {}
    ~V3DfgSynthesisContext() {
        // Conversion statistics
        ಕೂಡಿಸು_Stat("conv / input assignments", m_conv.ಒಳಹರಿವು_Assignments);
        ಕೂಡಿಸು_Stat("conv / input expressions", m_conv.ಒಳಹರಿವು_Expressions);
        ಕೂಡಿಸು_Stat("conv / representable inputs", m_conv.representable);
        ಕೂಡಿಸು_Stat("conv / non-representable (impure)", m_conv.nonRepImpure);
        ಕೂಡಿಸು_Stat("conv / non-representable (dtype)", m_conv.nonRepDType);
        ಕೂಡಿಸು_Stat("conv / non-representable (lhs)", m_conv.nonRepLValue);
        ಕೂಡಿಸು_Stat("conv / non-representable (varref)", m_conv.nonRepVarRef);
        ಕೂಡಿಸು_Stat("conv / non-representable (oobsel)", m_conv.nonRepOOBSel);
        ಕೂಡಿಸು_Stat("conv / non-representable (node)", m_conv.non_Rep_ಗಂಟು);
        ಕೂಡಿಸು_Stat("conv / non-representable (unknown)", m_conv.nonRepUnknown);
        VDouble0 nConvNonRep;
        nConvNonRep += m_conv.nonRepImpure;
        nConvNonRep += m_conv.nonRepDType;
        nConvNonRep += m_conv.nonRepLValue;
        nConvNonRep += m_conv.nonRepVarRef;
        nConvNonRep += m_conv.nonRepOOBSel;
        nConvNonRep += m_conv.non_Rep_ಗಂಟು;
        nConvNonRep += m_conv.nonRepUnknown;
        VDouble0 nConvExpect;
        nConvExpect += m_conv.ಒಳಹರಿವು_Assignments;
        nConvExpect += m_conv.ಒಳಹರಿವು_Expressions;
        nConvExpect -= m_conv.representable;
        UASSERT(nConvNonRep == nConvExpect, "Inconsistent statistics / conv");

        // Synthesis statistics
        ಕೂಡಿಸು_Stat("synt / always blocks considered", m_synt.ಒಳಹರಿವು_Always);
        ಕೂಡಿಸು_Stat("synt / always blocks synthesized", m_synt.synthAlways);
        ಕೂಡಿಸು_Stat("synt / continuous assignments considered", m_synt.ಒಳಹರಿವು_ನಿಯೋಜಿಸು);
        ಕೂಡಿಸು_Stat("synt / continuous assignments synthesized", m_synt.synth_ನಿಯೋಜಿಸು);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (conv)", m_synt.nonSynConv);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (ext write)", m_synt.nonSynExtWrite);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (loop)", m_synt.nonSynLoop);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (stmt)", m_synt.nonSynStmt);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (multidrive)", m_synt.nonSynMultidrive);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (array)", m_synt.nonSynArray);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (latch)", m_synt.nonSynLatch);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (join input)", m_synt.non_Syn_Join_ಒಳಹರಿವು);
        ಕೂಡಿಸು_Stat("synt / non-synthesizable (false write)", m_synt.nonSynFalseWrite);
        ಕೂಡಿಸು_Stat("synt / reverted (non-synthesizable)", m_synt.revertNonSyn);
        ಕೂಡಿಸು_Stat("synt / reverted (multidrive)", m_synt.revertMultidrive);

        VDouble0 nSyntNonSyn;
        nSyntNonSyn += m_synt.nonSynConv;
        nSyntNonSyn += m_synt.nonSynExtWrite;
        nSyntNonSyn += m_synt.nonSynLoop;
        nSyntNonSyn += m_synt.nonSynStmt;
        nSyntNonSyn += m_synt.nonSynMultidrive;
        nSyntNonSyn += m_synt.nonSynArray;
        nSyntNonSyn += m_synt.nonSynLatch;
        nSyntNonSyn += m_synt.non_Syn_Join_ಒಳಹರಿವು;
        nSyntNonSyn += m_synt.nonSynFalseWrite;
        VDouble0 nSyntExpect;
        nSyntExpect += m_synt.ಒಳಹರಿವು_Always;
        nSyntExpect += m_synt.ಒಳಹರಿವು_ನಿಯೋಜಿಸು;
        nSyntExpect -= m_synt.synthAlways;
        nSyntExpect -= m_synt.synth_ನಿಯೋಜಿಸು;
        UASSERT(nSyntNonSyn == nSyntExpect, "Inconsistent statistics / synt");

        ಕೂಡಿಸು_Stat("synt / input CFG trivial", m_synt.cfgTrivial);
        ಕೂಡಿಸು_Stat("synt / input CFG sp", m_synt.cfgSp);
        ಕೂಡಿಸು_Stat("synt / input CFG dag", m_synt.cfgDag);
        ಕೂಡಿಸು_Stat("synt / input CFG cyclic", m_synt.cfgCyclic);

        ಕೂಡಿಸು_Stat("synt / joins using path predicate", m_synt.join_Using_ಮಾರ್ಗ_Predicate);
        ಕೂಡಿಸು_Stat("synt / joins using branch condition", m_synt.joinUsingBranchCondition);
    }
};

//######################################################################
// Top level V3DfgContext

class V3DfgContext final {
ಸಾರ್ವಜನಿಕ:
    // STATE

    // Sub contexts - keep sorted by type
    V3_Dfg_Ast_ಗಮ್ಯ_Dfg_Context m_ast2DfgContext;
    V3_Dfg_Bin_ಗಮ್ಯ_One_Hot_Context m_bin_ಗಮ್ಯ_One_Hot_Context;  // Statistics context for V3DfgBinToOneHot
    V3_Dfg_Break_ಚಕ್ರಗಳು_Context m_break_ಚಕ್ರಗಳು_Context;  // Statistics context for V3DfgBreakCycles
    V3DfgCseContext m_cseContext0{"1st"};  // Statistics context for CSE stage 1
    V3DfgCseContext m_cseContext1{"2nd"};  // Statistics context for CSE stage 2
    V3_Dfg_Dfg_ಗಮ್ಯ_Ast_Context m_dfg2AstContext;  // Statistics context for V3DfgDfgToAst
    V3DfgPeepholeContext m_peepholeContext;  // Statistics context for V3DfgPeephole
    V3_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context m_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context;  // Statistics context for V3DfgPushDownSels
    V3DfgRegularizeContext m_regularizeContext;  // Statistics context for V3DfgRegularize
    V3DfgRemoveSelectsContext m_removeSelectsContext;  // Stats context for V3DfgRemoveSelects
    V3DfgRemoveUnobservableContext
        m_removeUnobservableContext;  // Stats context for V3DfgRemoveUnobservable
    V3DfgSynthesisContext m_synthContext;  // Statistics context for V3DfgSynthesize

    // Temporary variables created by all algorithms, shared by instances
    V3SharedTmps m_sharedTmps{"__Vdfg", VVarType::MODULETEMP};

    // CONSTRUCTOR
    V3DfgContext() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3DfgContext() {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, DFG, temporary declarations reused",
                         m_sharedTmps.nReused());
    }
};

#ಆದರೆ_ಅಂತ್ಯ  //VERILATOR_V3DFGCONTEXT_H_
