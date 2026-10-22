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

#यदि_अपरिभाषितम् VERILATOR_V3DFGCONTEXT_H_
#परिभाषय VERILATOR_V3DFGCONTEXT_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3DfgPeepholePatterns.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

class V3DfgContext;

//######################################################################
// Base class for all context objects

class V3DfgSubContext VL_NOT_FINAL {
    ध्रुव std::string m_name;  // Pass/algorithm name, to be used in statistics

रक्षित:
    VL_DEFINE_DEBUG_FUNCTIONS;

    V3DfgSubContext(ध्रुव std::string& name)
        : m_name{name} {}

    शून्य सङ्कलय_Stat(ध्रुव std::string& what, द्विगुण मानम्) {
        V3Stats::सङ्कलय_Stat("Optimizations, DFG, " + m_name + ", " + what, मानम्);
    }
};

//######################################################################
// Contexts for various algorithms - keep sorted

class V3_Dfg_Ast_गन्तव्यम्_Dfg_Context final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_inputs;  // Number of input processes (logic constructs)
    VDouble0 m_representable;  // Number of combinational equations representable
    VDouble0 m_nonRepCfg;  // Non-representable due to failing to build CFG
    VDouble0 m_non_Rep_जीवितम्;  // Non-representable due to failing liveness analysis
    VDouble0 m_nonRepVar;  // Non-representable due to unsupported variable properties

गुप्त:
    V3_Dfg_Ast_गन्तव्यम्_Dfg_Context()
        : V3DfgSubContext{"AstToDfg"} {}
    ~V3_Dfg_Ast_गन्तव्यम्_Dfg_Context() {
        सङ्कलय_Stat("input processes", m_inputs);
        सङ्कलय_Stat("representable", m_representable);
        सङ्कलय_Stat("non-representable (cfg)", m_nonRepCfg);
        सङ्कलय_Stat("non-representable (live)", m_non_Rep_जीवितम्);
        सङ्कलय_Stat("non-representable (var)", m_nonRepVar);
        // Check the stats are consistent
        UASSERT(m_representable + m_nonRepCfg + m_non_Rep_जीवितम् + m_nonRepVar == m_inputs,
                "Inconsistent statistics");
    }
};
class V3_Dfg_Bin_गन्तव्यम्_One_Hot_Context final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_decodersCreated;  // Number of bianry to one-hot decoders created

गुप्त:
    V3_Dfg_Bin_गन्तव्यम्_One_Hot_Context()
        : V3DfgSubContext{"BinToOneHot"} {}
    ~V3_Dfg_Bin_गन्तव्यम्_One_Hot_Context() { सङ्कलय_Stat("decoders created", m_decodersCreated); }
};
class V3_Dfg_Break_आवृत्तयः_Context final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_nFixed;  // Number of graphs that became acyclic
    VDouble0 m_nImproved;  // Number of graphs that were improved but still cyclic
    VDouble0 m_nUnchanged;  // Number of graphs that were left unchanged
    VDouble0 m_nImprovements;  // Number of changes made to graphs
    VDouble0 m_n_पूर्व_Inserted;  // Number of Prev vertices inserted

गुप्त:
    V3_Dfg_Break_आवृत्तयः_Context()
        : V3DfgSubContext{"BreakCycles"} {}
    ~V3_Dfg_Break_आवृत्तयः_Context() {
        सङ्कलय_Stat("made acyclic", m_nFixed);
        सङ्कलय_Stat("improved", m_nImproved);
        सङ्कलय_Stat("left unchanged", m_nUnchanged);
        सङ्कलय_Stat("changes applied", m_nImprovements);
        सङ्कलय_Stat("prev vertices inserted", m_n_पूर्व_Inserted);
    }
};
class V3DfgCseContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_eliminated;  // Number of common sub-expressions eliminated

गुप्त:
    explicit V3DfgCseContext(ध्रुव std::string& label)
        : V3DfgSubContext{"CSE " + label} {}
    ~V3DfgCseContext() { सङ्कलय_Stat("expressions eliminated", m_eliminated); }
};

class V3_Dfg_Dfg_गन्तव्यम्_Ast_Context final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_outputVariables;  // Number of output variables
    VDouble0 m_outputVariablesWithDefault;  // Number of outptu variables with a default driver
    VDouble0 m_परिणाम_Equations;  // Number of result combinational equations
    VDouble0 m_expressionsInlined;  // Number of expressions inlined directly into the Ast
    VDouble0 m_varRefsSubstituted;  // Number of variable references substituted in the Ast

गुप्त:
    V3_Dfg_Dfg_गन्तव्यम्_Ast_Context()
        : V3DfgSubContext{"DfgToAst"} {}
    ~V3_Dfg_Dfg_गन्तव्यम्_Ast_Context() {
        सङ्कलय_Stat("output variables", m_outputVariables);
        सङ्कलय_Stat("output variables with default driver", m_outputVariablesWithDefault);
        सङ्कलय_Stat("result equations", m_परिणाम_Equations);
        सङ्कलय_Stat("expressions inlined", m_expressionsInlined);
        सङ्कलय_Stat("var refs substituted", m_varRefsSubstituted);
    }
};
class V3DfgPeepholeContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    // Enable flags for each optimization
    std::array<bool, VDfg_Peephole_आदर्शः::ENUM_अन्तः> m_enabled;
    // Count of applications for each optimization (for statistics)
    std::array<VDouble0, VDfg_Peephole_आदर्शः::ENUM_अन्तः> m_गणनम्;
    std::सदिशम्<Ast_ग्रन्थिः*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end

गुप्त:
    V3DfgPeepholeContext()
        : V3DfgSubContext{"Peephole"} {
        ध्रुव स्वचालित checkEnabled = [this](VDfg_Peephole_आदर्शः id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.अन्तः_2(), str.begin(), [](अचिह्नित वर्ण c) {  //
                प्रत्यागम c == '_' ? '-' : std::tolower(c);
            });
            m_enabled[id] = v3Global.opt.fDfgPeepholeEnabled(str);
        };
#परिभाषय OPTIMIZATION_CHECK_ENABLED(id, name) checkEnabled(VDfg_Peephole_आदर्शः::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_CHECK_ENABLED)
#अपरिभाषय OPTIMIZATION_CHECK_ENABLED
    }
    ~V3DfgPeepholeContext() {
        प्रति (Ast_ग्रन्थिः* ध्रुव nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        }
        ध्रुव स्वचालित emitStat = [this](VDfg_Peephole_आदर्शः id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.अन्तः_2(), str.begin(), [](अचिह्नित वर्ण c) {  //
                प्रत्यागम c == '_' ? ' ' : std::tolower(c);
            });
            सङ्कलय_Stat(str, m_गणनम्[id]);
        };
#परिभाषय OPTIMIZATION_EMIT_STATS(id, name) emitStat(VDfg_Peephole_आदर्शः::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_EMIT_STATS)
#अपरिभाषय OPTIMIZATION_EMIT_STATS
    }
};
class V3_Dfg_प्रक्षिप_अधस्तात्_Sels_Context final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    आकार_t m_pushed_अधस्तात् = 0;  // Number of selects pushed down through concatenations
    आकार_t m_wouldBeCyclic = 0;  // Number of selects not pushed due to cycle
गुप्त:
    V3_Dfg_प्रक्षिप_अधस्तात्_Sels_Context()
        : V3DfgSubContext{"PushDownSels"} {}
    ~V3_Dfg_प्रक्षिप_अधस्तात्_Sels_Context() {
        सङ्कलय_Stat("sels pushed down", m_pushed_अधस्तात्);
        सङ्कलय_Stat("would be cyclic", m_wouldBeCyclic);
    }
};
class V3DfgRegularizeContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    std::सदिशम्<Ast_ग्रन्थिः*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end
    VDouble0 m_प्रयुक्तम्_Vars_Inlined;  // Number of used variables inlined
    VDouble0 m_अप्रयुक्तम्_Removed;  // Number of unused vertices remoevd
    VDouble0 m_temporariesIntroduced;  // Number of temporaries introduced for reuse

गुप्त:
    V3DfgRegularizeContext()
        : V3DfgSubContext{"Regularize"} {}
    ~V3DfgRegularizeContext() {
        प्रति (Ast_ग्रन्थिः* ध्रुव nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        }
        सङ्कलय_Stat("used variables inlined", m_प्रयुक्तम्_Vars_Inlined);
        सङ्कलय_Stat("unused vertices removed", m_अप्रयुक्तम्_Removed);
        सङ्कलय_Stat("temporaries introduced", m_temporariesIntroduced);
    }
};
class V3DfgRemoveSelectsContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_removed_पूर्णम्_विस्तारः;  // Number of full width selects removed
    VDouble0 m_replacedWithSelFromDriver;  // Number of selects replaced with sel from a driver
    VDouble0 m_replaced_With_सम्पूर्णम्_Driver;  // Number of selects replaced with a whole driver
गुप्त:
    V3DfgRemoveSelectsContext()
        : V3DfgSubContext{"RemoveSelects"} {}
    ~V3DfgRemoveSelectsContext() {
        सङ्कलय_Stat("full width selects removed", m_removed_पूर्णम्_विस्तारः);
        सङ्कलय_Stat("replaced with sel from driver", m_replacedWithSelFromDriver);
        सङ्कलय_Stat("replaced with whole driver", m_replaced_With_सम्पूर्णम्_Driver);
    }
};
class V3DfgRemoveUnobservableContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_varsRemoved;  // Number of variables removed from the Dfg
    VDouble0 m_varsDeleted;  // Number of variables removed from the Dfg and the Ast
    VDouble0 m_logicRemoved;  // Number of logic blocks removed from the Dfg
    VDouble0 m_logicDeleted;  // Number of logic blocks removed from the Dfg and the Ast

गुप्त:
    V3DfgRemoveUnobservableContext()
        : V3DfgSubContext{"RemoveUnobservable"} {}
    ~V3DfgRemoveUnobservableContext() {
        सङ्कलय_Stat("variables removed", m_varsRemoved);
        सङ्कलय_Stat("variables deleted", m_varsDeleted);
        सङ्कलय_Stat("logic removed", m_logicRemoved);
        सङ्कलय_Stat("logic deleted", m_logicDeleted);
    }
};
class V3DfgSynthesisContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    मित्रम् class V3DfgContext;

सार्वजनिक:
    // STATE

    // Stats for conversion
    संरचना {
        // Inputs
        VDouble0 निवेशः_Assignments;  // Number of input assignments
        VDouble0 निवेशः_Expressions;  // Number of input equations
        // Successful
        VDouble0 representable;  // Number of representable constructs
        // Unsuccessful
        VDouble0 nonRepImpure;  // Non representable: impure
        VDouble0 nonRepDType;  // Non representable: unsupported data type
        VDouble0 nonRepLValue;  // Non representable: unsupported LValue form
        VDouble0 nonRepVarRef;  // Non representable: unsupported var reference
        VDouble0 nonRepOOBSel;  // Non representable: out of bounds select
        VDouble0 non_Rep_ग्रन्थिः;  // Non representable: unsupported AstNode type
        VDouble0 nonRepUnknown;  // Non representable: unhandled AstNode type
    } m_conv;

    // Stats for synthesis
    संरचना {
        // Inputs
        VDouble0 निवेशः_Always;  // Number of always blocks attempted
        VDouble0 निवेशः_नियोजय;  // Number of continuous assignments attempted
        // Successful
        VDouble0 synthAlways;  // Number of always blocks successfully synthesized
        VDouble0 synth_नियोजय;  // Number of continuous assignments successfully synthesized
        // Unsuccessful
        VDouble0 nonSynConv;  // Non-synthesizable: non-representable (above)
        VDouble0 nonSynExtWrite;  // Non-synthesizable: has externally written variable
        VDouble0 nonSynLoop;  // Non-synthesizable: loop in CFG
        VDouble0 nonSynStmt;  // Non-synthesizable: unsupported statement
        VDouble0 nonSynMultidrive;  // Non-synthesizable: multidriven value within statement
        VDouble0 nonSynArray;  // Non-synthesizable: array type unhandled
        VDouble0 nonSynLatch;  // Non-synthesizable: maybe latch
        VDouble0 non_Syn_Join_निवेशः;  // Non-synthesizable: needing to join input variable
        VDouble0 nonSynFalseWrite;  // Non-synthesizable: does not write output
        // Reverted
        VDouble0 revertNonSyn;  // Reverted due to being driven from non-synthesizable vertex
        VDouble0 revertMultidrive;  // Reverted due to multiple drivers
        // Additional stats
        VDouble0 cfgTrivial;  // Trivial input CFGs
        VDouble0 cfgSp;  // Series-paralel input CFGs
        VDouble0 cfgDag;  // Generic loop free input CFGs
        VDouble0 cfgCyclic;  // Cyclic input CFGs
        VDouble0 join_Using_मार्गः_Predicate;  // Num control flow joins using full path predicate
        VDouble0 joinUsingBranchCondition;  // Num Control flow joins using dominating branch cond
    } m_synt;

गुप्त:
    V3DfgSynthesisContext()
        : V3DfgSubContext{"Synthesis"} {}
    ~V3DfgSynthesisContext() {
        // Conversion statistics
        सङ्कलय_Stat("conv / input assignments", m_conv.निवेशः_Assignments);
        सङ्कलय_Stat("conv / input expressions", m_conv.निवेशः_Expressions);
        सङ्कलय_Stat("conv / representable inputs", m_conv.representable);
        सङ्कलय_Stat("conv / non-representable (impure)", m_conv.nonRepImpure);
        सङ्कलय_Stat("conv / non-representable (dtype)", m_conv.nonRepDType);
        सङ्कलय_Stat("conv / non-representable (lhs)", m_conv.nonRepLValue);
        सङ्कलय_Stat("conv / non-representable (varref)", m_conv.nonRepVarRef);
        सङ्कलय_Stat("conv / non-representable (oobsel)", m_conv.nonRepOOBSel);
        सङ्कलय_Stat("conv / non-representable (node)", m_conv.non_Rep_ग्रन्थिः);
        सङ्कलय_Stat("conv / non-representable (unknown)", m_conv.nonRepUnknown);
        VDouble0 nConvNonRep;
        nConvNonRep += m_conv.nonRepImpure;
        nConvNonRep += m_conv.nonRepDType;
        nConvNonRep += m_conv.nonRepLValue;
        nConvNonRep += m_conv.nonRepVarRef;
        nConvNonRep += m_conv.nonRepOOBSel;
        nConvNonRep += m_conv.non_Rep_ग्रन्थिः;
        nConvNonRep += m_conv.nonRepUnknown;
        VDouble0 nConvExpect;
        nConvExpect += m_conv.निवेशः_Assignments;
        nConvExpect += m_conv.निवेशः_Expressions;
        nConvExpect -= m_conv.representable;
        UASSERT(nConvNonRep == nConvExpect, "Inconsistent statistics / conv");

        // Synthesis statistics
        सङ्कलय_Stat("synt / always blocks considered", m_synt.निवेशः_Always);
        सङ्कलय_Stat("synt / always blocks synthesized", m_synt.synthAlways);
        सङ्कलय_Stat("synt / continuous assignments considered", m_synt.निवेशः_नियोजय);
        सङ्कलय_Stat("synt / continuous assignments synthesized", m_synt.synth_नियोजय);
        सङ्कलय_Stat("synt / non-synthesizable (conv)", m_synt.nonSynConv);
        सङ्कलय_Stat("synt / non-synthesizable (ext write)", m_synt.nonSynExtWrite);
        सङ्कलय_Stat("synt / non-synthesizable (loop)", m_synt.nonSynLoop);
        सङ्कलय_Stat("synt / non-synthesizable (stmt)", m_synt.nonSynStmt);
        सङ्कलय_Stat("synt / non-synthesizable (multidrive)", m_synt.nonSynMultidrive);
        सङ्कलय_Stat("synt / non-synthesizable (array)", m_synt.nonSynArray);
        सङ्कलय_Stat("synt / non-synthesizable (latch)", m_synt.nonSynLatch);
        सङ्कलय_Stat("synt / non-synthesizable (join input)", m_synt.non_Syn_Join_निवेशः);
        सङ्कलय_Stat("synt / non-synthesizable (false write)", m_synt.nonSynFalseWrite);
        सङ्कलय_Stat("synt / reverted (non-synthesizable)", m_synt.revertNonSyn);
        सङ्कलय_Stat("synt / reverted (multidrive)", m_synt.revertMultidrive);

        VDouble0 nSyntNonSyn;
        nSyntNonSyn += m_synt.nonSynConv;
        nSyntNonSyn += m_synt.nonSynExtWrite;
        nSyntNonSyn += m_synt.nonSynLoop;
        nSyntNonSyn += m_synt.nonSynStmt;
        nSyntNonSyn += m_synt.nonSynMultidrive;
        nSyntNonSyn += m_synt.nonSynArray;
        nSyntNonSyn += m_synt.nonSynLatch;
        nSyntNonSyn += m_synt.non_Syn_Join_निवेशः;
        nSyntNonSyn += m_synt.nonSynFalseWrite;
        VDouble0 nSyntExpect;
        nSyntExpect += m_synt.निवेशः_Always;
        nSyntExpect += m_synt.निवेशः_नियोजय;
        nSyntExpect -= m_synt.synthAlways;
        nSyntExpect -= m_synt.synth_नियोजय;
        UASSERT(nSyntNonSyn == nSyntExpect, "Inconsistent statistics / synt");

        सङ्कलय_Stat("synt / input CFG trivial", m_synt.cfgTrivial);
        सङ्कलय_Stat("synt / input CFG sp", m_synt.cfgSp);
        सङ्कलय_Stat("synt / input CFG dag", m_synt.cfgDag);
        सङ्कलय_Stat("synt / input CFG cyclic", m_synt.cfgCyclic);

        सङ्कलय_Stat("synt / joins using path predicate", m_synt.join_Using_मार्गः_Predicate);
        सङ्कलय_Stat("synt / joins using branch condition", m_synt.joinUsingBranchCondition);
    }
};

//######################################################################
// Top level V3DfgContext

class V3DfgContext final {
सार्वजनिक:
    // STATE

    // Sub contexts - keep sorted by type
    V3_Dfg_Ast_गन्तव्यम्_Dfg_Context m_ast2DfgContext;
    V3_Dfg_Bin_गन्तव्यम्_One_Hot_Context m_bin_गन्तव्यम्_One_Hot_Context;  // Statistics context for V3DfgBinToOneHot
    V3_Dfg_Break_आवृत्तयः_Context m_break_आवृत्तयः_Context;  // Statistics context for V3DfgBreakCycles
    V3DfgCseContext m_cseContext0{"1st"};  // Statistics context for CSE stage 1
    V3DfgCseContext m_cseContext1{"2nd"};  // Statistics context for CSE stage 2
    V3_Dfg_Dfg_गन्तव्यम्_Ast_Context m_dfg2AstContext;  // Statistics context for V3DfgDfgToAst
    V3DfgPeepholeContext m_peepholeContext;  // Statistics context for V3DfgPeephole
    V3_Dfg_प्रक्षिप_अधस्तात्_Sels_Context m_प्रक्षिप_अधस्तात्_Sels_Context;  // Statistics context for V3DfgPushDownSels
    V3DfgRegularizeContext m_regularizeContext;  // Statistics context for V3DfgRegularize
    V3DfgRemoveSelectsContext m_removeSelectsContext;  // Stats context for V3DfgRemoveSelects
    V3DfgRemoveUnobservableContext
        m_removeUnobservableContext;  // Stats context for V3DfgRemoveUnobservable
    V3DfgSynthesisContext m_synthContext;  // Statistics context for V3DfgSynthesize

    // Temporary variables created by all algorithms, shared by instances
    V3SharedTmps m_sharedTmps{"__Vdfg", VVarType::MODULETEMP};

    // CONSTRUCTOR
    V3DfgContext() = स्वभाव;
    ~V3DfgContext() {
        V3Stats::सङ्कलय_Stat("Optimizations, DFG, temporary declarations reused",
                         m_sharedTmps.nReused());
    }
};

#यदि_अन्तः  //VERILATOR_V3DFGCONTEXT_H_
