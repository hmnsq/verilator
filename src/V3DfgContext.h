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

#నిర్వచితం_కాకపోతే VERILATOR_V3DFGCONTEXT_H_
#నిర్వచించు VERILATOR_V3DFGCONTEXT_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3DfgPeepholePatterns.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"

class V3DfgContext;

//######################################################################
// Base class for all context objects

class V3DfgSubContext VL_NOT_FINAL {
    స్థిరరాశి std::string m_name;  // Pass/algorithm name, to be used in statistics

రక్షిత:
    VL_DEFINE_DEBUG_FUNCTIONS;

    V3DfgSubContext(స్థిరరాశి std::string& name)
        : m_name{name} {}

    శూన్య కూడు_Stat(స్థిరరాశి std::string& what, ద్విగుణ విలువ) {
        V3Stats::కూడు_Stat("Optimizations, DFG, " + m_name + ", " + what, విలువ);
    }
};

//######################################################################
// Contexts for various algorithms - keep sorted

class V3_Dfg_Ast_గమ్యం_Dfg_Context final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_inputs;  // Number of input processes (logic constructs)
    VDouble0 m_representable;  // Number of combinational equations representable
    VDouble0 m_nonRepCfg;  // Non-representable due to failing to build CFG
    VDouble0 m_non_Rep_సజీవ;  // Non-representable due to failing liveness analysis
    VDouble0 m_nonRepVar;  // Non-representable due to unsupported variable properties

అంతరంగ:
    V3_Dfg_Ast_గమ్యం_Dfg_Context()
        : V3DfgSubContext{"AstToDfg"} {}
    ~V3_Dfg_Ast_గమ్యం_Dfg_Context() {
        కూడు_Stat("input processes", m_inputs);
        కూడు_Stat("representable", m_representable);
        కూడు_Stat("non-representable (cfg)", m_nonRepCfg);
        కూడు_Stat("non-representable (live)", m_non_Rep_సజీవ);
        కూడు_Stat("non-representable (var)", m_nonRepVar);
        // Check the stats are consistent
        UASSERT(m_representable + m_nonRepCfg + m_non_Rep_సజీవ + m_nonRepVar == m_inputs,
                "Inconsistent statistics");
    }
};
class V3_Dfg_Bin_గమ్యం_One_Hot_Context final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_decodersCreated;  // Number of bianry to one-hot decoders created

అంతరంగ:
    V3_Dfg_Bin_గమ్యం_One_Hot_Context()
        : V3DfgSubContext{"BinToOneHot"} {}
    ~V3_Dfg_Bin_గమ్యం_One_Hot_Context() { కూడు_Stat("decoders created", m_decodersCreated); }
};
class V3_Dfg_Break_చక్రాలు_Context final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_nFixed;  // Number of graphs that became acyclic
    VDouble0 m_nImproved;  // Number of graphs that were improved but still cyclic
    VDouble0 m_nUnchanged;  // Number of graphs that were left unchanged
    VDouble0 m_nImprovements;  // Number of changes made to graphs
    VDouble0 m_nPrevInserted;  // Number of Prev vertices inserted

అంతరంగ:
    V3_Dfg_Break_చక్రాలు_Context()
        : V3DfgSubContext{"BreakCycles"} {}
    ~V3_Dfg_Break_చక్రాలు_Context() {
        కూడు_Stat("made acyclic", m_nFixed);
        కూడు_Stat("improved", m_nImproved);
        కూడు_Stat("left unchanged", m_nUnchanged);
        కూడు_Stat("changes applied", m_nImprovements);
        కూడు_Stat("prev vertices inserted", m_nPrevInserted);
    }
};
class V3DfgCseContext final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_eliminated;  // Number of common sub-expressions eliminated

అంతరంగ:
    explicit V3DfgCseContext(స్థిరరాశి std::string& label)
        : V3DfgSubContext{"CSE " + label} {}
    ~V3DfgCseContext() { కూడు_Stat("expressions eliminated", m_eliminated); }
};

class V3_Dfg_Dfg_గమ్యం_Ast_Context final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_outputVariables;  // Number of output variables
    VDouble0 m_outputVariablesWithDefault;  // Number of outptu variables with a default driver
    VDouble0 m_ఫలితం_Equations;  // Number of result combinational equations
    VDouble0 m_expressionsInlined;  // Number of expressions inlined directly into the Ast
    VDouble0 m_varRefsSubstituted;  // Number of variable references substituted in the Ast

అంతరంగ:
    V3_Dfg_Dfg_గమ్యం_Ast_Context()
        : V3DfgSubContext{"DfgToAst"} {}
    ~V3_Dfg_Dfg_గమ్యం_Ast_Context() {
        కూడు_Stat("output variables", m_outputVariables);
        కూడు_Stat("output variables with default driver", m_outputVariablesWithDefault);
        కూడు_Stat("result equations", m_ఫలితం_Equations);
        కూడు_Stat("expressions inlined", m_expressionsInlined);
        కూడు_Stat("var refs substituted", m_varRefsSubstituted);
    }
};
class V3DfgPeepholeContext final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    // Enable flags for each optimization
    std::array<bool, VDfg_Peephole_నమూనా::ENUM_చివర> m_enabled;
    // Count of applications for each optimization (for statistics)
    std::array<VDouble0, VDfg_Peephole_నమూనా::ENUM_చివర> m_లెక్క;
    std::సదిశ<Ast_కణుపు*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end

అంతరంగ:
    V3DfgPeepholeContext()
        : V3DfgSubContext{"Peephole"} {
        స్థిరరాశి స్వయంచాలక checkEnabled = [this](VDfg_Peephole_నమూనా id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.చివర(), str.begin(), [](గుర్తులేని అక్షరం c) {  //
                తిరిగి c == '_' ? '-' : std::tolower(c);
            });
            m_enabled[id] = v3Global.opt.fDfgPeepholeEnabled(str);
        };
#నిర్వచించు OPTIMIZATION_CHECK_ENABLED(id, name) checkEnabled(VDfg_Peephole_నమూనా::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_CHECK_ENABLED)
#నిర్వచనం_రద్దు OPTIMIZATION_CHECK_ENABLED
    }
    ~V3DfgPeepholeContext() {
        కొరకు (Ast_కణుపు* స్థిరరాశి nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        }
        స్థిరరాశి స్వయంచాలక emitStat = [this](VDfg_Peephole_నమూనా id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.చివర(), str.begin(), [](గుర్తులేని అక్షరం c) {  //
                తిరిగి c == '_' ? ' ' : std::tolower(c);
            });
            కూడు_Stat(str, m_లెక్క[id]);
        };
#నిర్వచించు OPTIMIZATION_EMIT_STATS(id, name) emitStat(VDfg_Peephole_నమూనా::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_EMIT_STATS)
#నిర్వచనం_రద్దు OPTIMIZATION_EMIT_STATS
    }
};
class V3_Dfg_నెట్టు_కిందికి_Sels_Context final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    కొలత_t m_pushed_కిందికి = 0;  // Number of selects pushed down through concatenations
    కొలత_t m_wouldBeCyclic = 0;  // Number of selects not pushed due to cycle
అంతరంగ:
    V3_Dfg_నెట్టు_కిందికి_Sels_Context()
        : V3DfgSubContext{"PushDownSels"} {}
    ~V3_Dfg_నెట్టు_కిందికి_Sels_Context() {
        కూడు_Stat("sels pushed down", m_pushed_కిందికి);
        కూడు_Stat("would be cyclic", m_wouldBeCyclic);
    }
};
class V3DfgRegularizeContext final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    std::సదిశ<Ast_కణుపు*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end
    VDouble0 m_వాడిన_Vars_Inlined;  // Number of used variables inlined
    VDouble0 m_వాడని_Removed;  // Number of unused vertices remoevd
    VDouble0 m_temporariesIntroduced;  // Number of temporaries introduced for reuse

అంతరంగ:
    V3DfgRegularizeContext()
        : V3DfgSubContext{"Regularize"} {}
    ~V3DfgRegularizeContext() {
        కొరకు (Ast_కణుపు* స్థిరరాశి nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        }
        కూడు_Stat("used variables inlined", m_వాడిన_Vars_Inlined);
        కూడు_Stat("unused vertices removed", m_వాడని_Removed);
        కూడు_Stat("temporaries introduced", m_temporariesIntroduced);
    }
};
class V3DfgRemoveSelectsContext final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_removed_నిండిన_వెడల్పు;  // Number of full width selects removed
    VDouble0 m_replacedWithSelFromDriver;  // Number of selects replaced with sel from a driver
    VDouble0 m_replaced_With_సంపూర్ణ_Driver;  // Number of selects replaced with a whole driver
అంతరంగ:
    V3DfgRemoveSelectsContext()
        : V3DfgSubContext{"RemoveSelects"} {}
    ~V3DfgRemoveSelectsContext() {
        కూడు_Stat("full width selects removed", m_removed_నిండిన_వెడల్పు);
        కూడు_Stat("replaced with sel from driver", m_replacedWithSelFromDriver);
        కూడు_Stat("replaced with whole driver", m_replaced_With_సంపూర్ణ_Driver);
    }
};
class V3DfgRemoveUnobservableContext final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE
    VDouble0 m_varsRemoved;  // Number of variables removed from the Dfg
    VDouble0 m_varsDeleted;  // Number of variables removed from the Dfg and the Ast
    VDouble0 m_logicRemoved;  // Number of logic blocks removed from the Dfg
    VDouble0 m_logicDeleted;  // Number of logic blocks removed from the Dfg and the Ast

అంతరంగ:
    V3DfgRemoveUnobservableContext()
        : V3DfgSubContext{"RemoveUnobservable"} {}
    ~V3DfgRemoveUnobservableContext() {
        కూడు_Stat("variables removed", m_varsRemoved);
        కూడు_Stat("variables deleted", m_varsDeleted);
        కూడు_Stat("logic removed", m_logicRemoved);
        కూడు_Stat("logic deleted", m_logicDeleted);
    }
};
class V3DfgSynthesisContext final : బహిరంగ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    మిత్ర class V3DfgContext;

బహిరంగ:
    // STATE

    // Stats for conversion
    నిర్మాణం {
        // Inputs
        VDouble0 ఆదానం_Assignments;  // Number of input assignments
        VDouble0 ఆదానం_Expressions;  // Number of input equations
        // Successful
        VDouble0 representable;  // Number of representable constructs
        // Unsuccessful
        VDouble0 nonRepImpure;  // Non representable: impure
        VDouble0 nonRepDType;  // Non representable: unsupported data type
        VDouble0 nonRepLValue;  // Non representable: unsupported LValue form
        VDouble0 nonRepVarRef;  // Non representable: unsupported var reference
        VDouble0 nonRepOOBSel;  // Non representable: out of bounds select
        VDouble0 non_Rep_కణుపు;  // Non representable: unsupported AstNode type
        VDouble0 nonRepUnknown;  // Non representable: unhandled AstNode type
    } m_conv;

    // Stats for synthesis
    నిర్మాణం {
        // Inputs
        VDouble0 ఆదానం_Always;  // Number of always blocks attempted
        VDouble0 ఆదానం_కేటాయించు;  // Number of continuous assignments attempted
        // Successful
        VDouble0 synthAlways;  // Number of always blocks successfully synthesized
        VDouble0 synth_కేటాయించు;  // Number of continuous assignments successfully synthesized
        // Unsuccessful
        VDouble0 nonSynConv;  // Non-synthesizable: non-representable (above)
        VDouble0 nonSynExtWrite;  // Non-synthesizable: has externally written variable
        VDouble0 nonSynLoop;  // Non-synthesizable: loop in CFG
        VDouble0 nonSynStmt;  // Non-synthesizable: unsupported statement
        VDouble0 nonSynMultidrive;  // Non-synthesizable: multidriven value within statement
        VDouble0 nonSynArray;  // Non-synthesizable: array type unhandled
        VDouble0 nonSynLatch;  // Non-synthesizable: maybe latch
        VDouble0 non_Syn_Join_ఆదానం;  // Non-synthesizable: needing to join input variable
        VDouble0 nonSynFalseWrite;  // Non-synthesizable: does not write output
        // Reverted
        VDouble0 revertNonSyn;  // Reverted due to being driven from non-synthesizable vertex
        VDouble0 revertMultidrive;  // Reverted due to multiple drivers
        // Additional stats
        VDouble0 cfgTrivial;  // Trivial input CFGs
        VDouble0 cfgSp;  // Series-paralel input CFGs
        VDouble0 cfgDag;  // Generic loop free input CFGs
        VDouble0 cfgCyclic;  // Cyclic input CFGs
        VDouble0 join_Using_మార్గం_Predicate;  // Num control flow joins using full path predicate
        VDouble0 joinUsingBranchCondition;  // Num Control flow joins using dominating branch cond
    } m_synt;

అంతరంగ:
    V3DfgSynthesisContext()
        : V3DfgSubContext{"Synthesis"} {}
    ~V3DfgSynthesisContext() {
        // Conversion statistics
        కూడు_Stat("conv / input assignments", m_conv.ఆదానం_Assignments);
        కూడు_Stat("conv / input expressions", m_conv.ఆదానం_Expressions);
        కూడు_Stat("conv / representable inputs", m_conv.representable);
        కూడు_Stat("conv / non-representable (impure)", m_conv.nonRepImpure);
        కూడు_Stat("conv / non-representable (dtype)", m_conv.nonRepDType);
        కూడు_Stat("conv / non-representable (lhs)", m_conv.nonRepLValue);
        కూడు_Stat("conv / non-representable (varref)", m_conv.nonRepVarRef);
        కూడు_Stat("conv / non-representable (oobsel)", m_conv.nonRepOOBSel);
        కూడు_Stat("conv / non-representable (node)", m_conv.non_Rep_కణుపు);
        కూడు_Stat("conv / non-representable (unknown)", m_conv.nonRepUnknown);
        VDouble0 nConvNonRep;
        nConvNonRep += m_conv.nonRepImpure;
        nConvNonRep += m_conv.nonRepDType;
        nConvNonRep += m_conv.nonRepLValue;
        nConvNonRep += m_conv.nonRepVarRef;
        nConvNonRep += m_conv.nonRepOOBSel;
        nConvNonRep += m_conv.non_Rep_కణుపు;
        nConvNonRep += m_conv.nonRepUnknown;
        VDouble0 nConvExpect;
        nConvExpect += m_conv.ఆదానం_Assignments;
        nConvExpect += m_conv.ఆదానం_Expressions;
        nConvExpect -= m_conv.representable;
        UASSERT(nConvNonRep == nConvExpect, "Inconsistent statistics / conv");

        // Synthesis statistics
        కూడు_Stat("synt / always blocks considered", m_synt.ఆదానం_Always);
        కూడు_Stat("synt / always blocks synthesized", m_synt.synthAlways);
        కూడు_Stat("synt / continuous assignments considered", m_synt.ఆదానం_కేటాయించు);
        కూడు_Stat("synt / continuous assignments synthesized", m_synt.synth_కేటాయించు);
        కూడు_Stat("synt / non-synthesizable (conv)", m_synt.nonSynConv);
        కూడు_Stat("synt / non-synthesizable (ext write)", m_synt.nonSynExtWrite);
        కూడు_Stat("synt / non-synthesizable (loop)", m_synt.nonSynLoop);
        కూడు_Stat("synt / non-synthesizable (stmt)", m_synt.nonSynStmt);
        కూడు_Stat("synt / non-synthesizable (multidrive)", m_synt.nonSynMultidrive);
        కూడు_Stat("synt / non-synthesizable (array)", m_synt.nonSynArray);
        కూడు_Stat("synt / non-synthesizable (latch)", m_synt.nonSynLatch);
        కూడు_Stat("synt / non-synthesizable (join input)", m_synt.non_Syn_Join_ఆదానం);
        కూడు_Stat("synt / non-synthesizable (false write)", m_synt.nonSynFalseWrite);
        కూడు_Stat("synt / reverted (non-synthesizable)", m_synt.revertNonSyn);
        కూడు_Stat("synt / reverted (multidrive)", m_synt.revertMultidrive);

        VDouble0 nSyntNonSyn;
        nSyntNonSyn += m_synt.nonSynConv;
        nSyntNonSyn += m_synt.nonSynExtWrite;
        nSyntNonSyn += m_synt.nonSynLoop;
        nSyntNonSyn += m_synt.nonSynStmt;
        nSyntNonSyn += m_synt.nonSynMultidrive;
        nSyntNonSyn += m_synt.nonSynArray;
        nSyntNonSyn += m_synt.nonSynLatch;
        nSyntNonSyn += m_synt.non_Syn_Join_ఆదానం;
        nSyntNonSyn += m_synt.nonSynFalseWrite;
        VDouble0 nSyntExpect;
        nSyntExpect += m_synt.ఆదానం_Always;
        nSyntExpect += m_synt.ఆదానం_కేటాయించు;
        nSyntExpect -= m_synt.synthAlways;
        nSyntExpect -= m_synt.synth_కేటాయించు;
        UASSERT(nSyntNonSyn == nSyntExpect, "Inconsistent statistics / synt");

        కూడు_Stat("synt / input CFG trivial", m_synt.cfgTrivial);
        కూడు_Stat("synt / input CFG sp", m_synt.cfgSp);
        కూడు_Stat("synt / input CFG dag", m_synt.cfgDag);
        కూడు_Stat("synt / input CFG cyclic", m_synt.cfgCyclic);

        కూడు_Stat("synt / joins using path predicate", m_synt.join_Using_మార్గం_Predicate);
        కూడు_Stat("synt / joins using branch condition", m_synt.joinUsingBranchCondition);
    }
};

//######################################################################
// Top level V3DfgContext

class V3DfgContext final {
బహిరంగ:
    // STATE

    // Sub contexts - keep sorted by type
    V3_Dfg_Ast_గమ్యం_Dfg_Context m_ast2DfgContext;
    V3_Dfg_Bin_గమ్యం_One_Hot_Context m_bin_గమ్యం_One_Hot_Context;  // Statistics context for V3DfgBinToOneHot
    V3_Dfg_Break_చక్రాలు_Context m_break_చక్రాలు_Context;  // Statistics context for V3DfgBreakCycles
    V3DfgCseContext m_cseContext0{"1st"};  // Statistics context for CSE stage 1
    V3DfgCseContext m_cseContext1{"2nd"};  // Statistics context for CSE stage 2
    V3_Dfg_Dfg_గమ్యం_Ast_Context m_dfg2AstContext;  // Statistics context for V3DfgDfgToAst
    V3DfgPeepholeContext m_peepholeContext;  // Statistics context for V3DfgPeephole
    V3_Dfg_నెట్టు_కిందికి_Sels_Context m_నెట్టు_కిందికి_Sels_Context;  // Statistics context for V3DfgPushDownSels
    V3DfgRegularizeContext m_regularizeContext;  // Statistics context for V3DfgRegularize
    V3DfgRemoveSelectsContext m_removeSelectsContext;  // Stats context for V3DfgRemoveSelects
    V3DfgRemoveUnobservableContext
        m_removeUnobservableContext;  // Stats context for V3DfgRemoveUnobservable
    V3DfgSynthesisContext m_synthContext;  // Statistics context for V3DfgSynthesize

    // Temporary variables created by all algorithms, shared by instances
    V3SharedTmps m_sharedTmps{"__Vdfg", VVarType::MODULETEMP};

    // CONSTRUCTOR
    V3DfgContext() = అప్రమేయ;
    ~V3DfgContext() {
        V3Stats::కూడు_Stat("Optimizations, DFG, temporary declarations reused",
                         m_sharedTmps.nReused());
    }
};

#అయితే_ముగింపు  //VERILATOR_V3DFGCONTEXT_H_
