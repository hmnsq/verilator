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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3DFGCONTEXT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3DFGCONTEXT_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPeepholePatterns.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

class V3DfgContext;

//######################################################################
// Base class for all context objects

class V3DfgSubContext VL_NOT_FINAL {
    ਅਚਲ std::string m_name;  // Pass/algorithm name, to be used in statistics

ਸੁਰੱਖਿਅਤ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    V3DfgSubContext(ਅਚਲ std::string& name)
        : m_name{name} {}

    ਖਾਲੀ ਜੋੜੋ_Stat(ਅਚਲ std::string& what, ਦੁੱਗਣਾ ਮੁੱਲ) {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, DFG, " + m_name + ", " + what, ਮੁੱਲ);
    }
};

//######################################################################
// Contexts for various algorithms - keep sorted

class V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_inputs;  // Number of input processes (logic constructs)
    VDouble0 m_representable;  // Number of combinational equations representable
    VDouble0 m_nonRepCfg;  // Non-representable due to failing to build CFG
    VDouble0 m_non_Rep_ਜੀਵਤ;  // Non-representable due to failing liveness analysis
    VDouble0 m_nonRepVar;  // Non-representable due to unsupported variable properties

ਨਿਜੀ:
    V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context()
        : V3DfgSubContext{"AstToDfg"} {}
    ~V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context() {
        ਜੋੜੋ_Stat("input processes", m_inputs);
        ਜੋੜੋ_Stat("representable", m_representable);
        ਜੋੜੋ_Stat("non-representable (cfg)", m_nonRepCfg);
        ਜੋੜੋ_Stat("non-representable (live)", m_non_Rep_ਜੀਵਤ);
        ਜੋੜੋ_Stat("non-representable (var)", m_nonRepVar);
        // Check the stats are consistent
        UASSERT(m_representable + m_nonRepCfg + m_non_Rep_ਜੀਵਤ + m_nonRepVar == m_inputs,
                "Inconsistent statistics");
    }
};
class V3_Dfg_Bin_ਮੰਜ਼ਿਲ_One_Hot_Context final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_decodersCreated;  // Number of bianry to one-hot decoders created

ਨਿਜੀ:
    V3_Dfg_Bin_ਮੰਜ਼ਿਲ_One_Hot_Context()
        : V3DfgSubContext{"BinToOneHot"} {}
    ~V3_Dfg_Bin_ਮੰਜ਼ਿਲ_One_Hot_Context() { ਜੋੜੋ_Stat("decoders created", m_decodersCreated); }
};
class V3_Dfg_Break_ਚੱਕਰਾਂ_Context final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_nFixed;  // Number of graphs that became acyclic
    VDouble0 m_nImproved;  // Number of graphs that were improved but still cyclic
    VDouble0 m_nUnchanged;  // Number of graphs that were left unchanged
    VDouble0 m_nImprovements;  // Number of changes made to graphs
    VDouble0 m_nPrevInserted;  // Number of Prev vertices inserted

ਨਿਜੀ:
    V3_Dfg_Break_ਚੱਕਰਾਂ_Context()
        : V3DfgSubContext{"BreakCycles"} {}
    ~V3_Dfg_Break_ਚੱਕਰਾਂ_Context() {
        ਜੋੜੋ_Stat("made acyclic", m_nFixed);
        ਜੋੜੋ_Stat("improved", m_nImproved);
        ਜੋੜੋ_Stat("left unchanged", m_nUnchanged);
        ਜੋੜੋ_Stat("changes applied", m_nImprovements);
        ਜੋੜੋ_Stat("prev vertices inserted", m_nPrevInserted);
    }
};
class V3DfgCseContext final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_eliminated;  // Number of common sub-expressions eliminated

ਨਿਜੀ:
    explicit V3DfgCseContext(ਅਚਲ std::string& label)
        : V3DfgSubContext{"CSE " + label} {}
    ~V3DfgCseContext() { ਜੋੜੋ_Stat("expressions eliminated", m_eliminated); }
};

class V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_outputVariables;  // Number of output variables
    VDouble0 m_outputVariablesWithDefault;  // Number of outptu variables with a default driver
    VDouble0 m_ਨਤੀਜਾ_Equations;  // Number of result combinational equations
    VDouble0 m_expressionsInlined;  // Number of expressions inlined directly into the Ast
    VDouble0 m_varRefsSubstituted;  // Number of variable references substituted in the Ast

ਨਿਜੀ:
    V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context()
        : V3DfgSubContext{"DfgToAst"} {}
    ~V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context() {
        ਜੋੜੋ_Stat("output variables", m_outputVariables);
        ਜੋੜੋ_Stat("output variables with default driver", m_outputVariablesWithDefault);
        ਜੋੜੋ_Stat("result equations", m_ਨਤੀਜਾ_Equations);
        ਜੋੜੋ_Stat("expressions inlined", m_expressionsInlined);
        ਜੋੜੋ_Stat("var refs substituted", m_varRefsSubstituted);
    }
};
class V3DfgPeepholeContext final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    // Enable flags for each optimization
    std::array<bool, VDfg_Peephole_ਨਮੂਨਾ::ENUM_ਸਿਰਾ> m_enabled;
    // Count of applications for each optimization (for statistics)
    std::array<VDouble0, VDfg_Peephole_ਨਮੂਨਾ::ENUM_ਸਿਰਾ> m_ਗਿਣਤੀ;
    std::ਸਦਿਸ਼<Ast_ਗੰਢ*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end

ਨਿਜੀ:
    V3DfgPeepholeContext()
        : V3DfgSubContext{"Peephole"} {
        ਅਚਲ ਸਵੈਚਾਲਿਤ checkEnabled = [this](VDfg_Peephole_ਨਮੂਨਾ id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.ਸਿਰਾ(), str.begin(), [](ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ c) {  //
                ਵਾਪਸ c == '_' ? '-' : std::tolower(c);
            });
            m_enabled[id] = v3Global.opt.fDfgPeepholeEnabled(str);
        };
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OPTIMIZATION_CHECK_ENABLED(id, name) checkEnabled(VDfg_Peephole_ਨਮੂਨਾ::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_CHECK_ENABLED)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ OPTIMIZATION_CHECK_ENABLED
    }
    ~V3DfgPeepholeContext() {
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ emitStat = [this](VDfg_Peephole_ਨਮੂਨਾ id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.ਸਿਰਾ(), str.begin(), [](ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ c) {  //
                ਵਾਪਸ c == '_' ? ' ' : std::tolower(c);
            });
            ਜੋੜੋ_Stat(str, m_ਗਿਣਤੀ[id]);
        };
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OPTIMIZATION_EMIT_STATS(id, name) emitStat(VDfg_Peephole_ਨਮੂਨਾ::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_EMIT_STATS)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ OPTIMIZATION_EMIT_STATS
    }
};
class V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    ਮਾਪ_t m_pushed_ਥੱਲੇ = 0;  // Number of selects pushed down through concatenations
    ਮਾਪ_t m_wouldBeCyclic = 0;  // Number of selects not pushed due to cycle
ਨਿਜੀ:
    V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context()
        : V3DfgSubContext{"PushDownSels"} {}
    ~V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context() {
        ਜੋੜੋ_Stat("sels pushed down", m_pushed_ਥੱਲੇ);
        ਜੋੜੋ_Stat("would be cyclic", m_wouldBeCyclic);
    }
};
class V3DfgRegularizeContext final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    std::ਸਦਿਸ਼<Ast_ਗੰਢ*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end
    VDouble0 m_ਵਰਤਿਆ_Vars_Inlined;  // Number of used variables inlined
    VDouble0 m_ਅਣਵਰਤਿਆ_Removed;  // Number of unused vertices remoevd
    VDouble0 m_temporariesIntroduced;  // Number of temporaries introduced for reuse

ਨਿਜੀ:
    V3DfgRegularizeContext()
        : V3DfgSubContext{"Regularize"} {}
    ~V3DfgRegularizeContext() {
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        }
        ਜੋੜੋ_Stat("used variables inlined", m_ਵਰਤਿਆ_Vars_Inlined);
        ਜੋੜੋ_Stat("unused vertices removed", m_ਅਣਵਰਤਿਆ_Removed);
        ਜੋੜੋ_Stat("temporaries introduced", m_temporariesIntroduced);
    }
};
class V3DfgRemoveSelectsContext final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_removed_ਭਰਿਆ_ਚੌੜਾਈ;  // Number of full width selects removed
    VDouble0 m_replacedWithSelFromDriver;  // Number of selects replaced with sel from a driver
    VDouble0 m_replaced_With_ਸਾਰਾ_Driver;  // Number of selects replaced with a whole driver
ਨਿਜੀ:
    V3DfgRemoveSelectsContext()
        : V3DfgSubContext{"RemoveSelects"} {}
    ~V3DfgRemoveSelectsContext() {
        ਜੋੜੋ_Stat("full width selects removed", m_removed_ਭਰਿਆ_ਚੌੜਾਈ);
        ਜੋੜੋ_Stat("replaced with sel from driver", m_replacedWithSelFromDriver);
        ਜੋੜੋ_Stat("replaced with whole driver", m_replaced_With_ਸਾਰਾ_Driver);
    }
};
class V3DfgRemoveUnobservableContext final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE
    VDouble0 m_varsRemoved;  // Number of variables removed from the Dfg
    VDouble0 m_varsDeleted;  // Number of variables removed from the Dfg and the Ast
    VDouble0 m_logicRemoved;  // Number of logic blocks removed from the Dfg
    VDouble0 m_logicDeleted;  // Number of logic blocks removed from the Dfg and the Ast

ਨਿਜੀ:
    V3DfgRemoveUnobservableContext()
        : V3DfgSubContext{"RemoveUnobservable"} {}
    ~V3DfgRemoveUnobservableContext() {
        ਜੋੜੋ_Stat("variables removed", m_varsRemoved);
        ਜੋੜੋ_Stat("variables deleted", m_varsDeleted);
        ਜੋੜੋ_Stat("logic removed", m_logicRemoved);
        ਜੋੜੋ_Stat("logic deleted", m_logicDeleted);
    }
};
class V3DfgSynthesisContext final : ਜਨਤਕ V3DfgSubContext {
    // Only V3DfgContext can create an instance
    ਮਿੱਤਰ class V3DfgContext;

ਜਨਤਕ:
    // STATE

    // Stats for conversion
    ਬਣਤਰ {
        // Inputs
        VDouble0 ਨਿਵੇਸ਼_Assignments;  // Number of input assignments
        VDouble0 ਨਿਵੇਸ਼_Expressions;  // Number of input equations
        // Successful
        VDouble0 representable;  // Number of representable constructs
        // Unsuccessful
        VDouble0 nonRepImpure;  // Non representable: impure
        VDouble0 nonRepDType;  // Non representable: unsupported data type
        VDouble0 nonRepLValue;  // Non representable: unsupported LValue form
        VDouble0 nonRepVarRef;  // Non representable: unsupported var reference
        VDouble0 nonRepOOBSel;  // Non representable: out of bounds select
        VDouble0 non_Rep_ਗੰਢ;  // Non representable: unsupported AstNode type
        VDouble0 nonRepUnknown;  // Non representable: unhandled AstNode type
    } m_conv;

    // Stats for synthesis
    ਬਣਤਰ {
        // Inputs
        VDouble0 ਨਿਵੇਸ਼_Always;  // Number of always blocks attempted
        VDouble0 ਨਿਵੇਸ਼_ਸੌਂਪੋ;  // Number of continuous assignments attempted
        // Successful
        VDouble0 synthAlways;  // Number of always blocks successfully synthesized
        VDouble0 synth_ਸੌਂਪੋ;  // Number of continuous assignments successfully synthesized
        // Unsuccessful
        VDouble0 nonSynConv;  // Non-synthesizable: non-representable (above)
        VDouble0 nonSynExtWrite;  // Non-synthesizable: has externally written variable
        VDouble0 nonSynLoop;  // Non-synthesizable: loop in CFG
        VDouble0 nonSynStmt;  // Non-synthesizable: unsupported statement
        VDouble0 nonSynMultidrive;  // Non-synthesizable: multidriven value within statement
        VDouble0 nonSynArray;  // Non-synthesizable: array type unhandled
        VDouble0 nonSynLatch;  // Non-synthesizable: maybe latch
        VDouble0 non_Syn_Join_ਨਿਵੇਸ਼;  // Non-synthesizable: needing to join input variable
        VDouble0 nonSynFalseWrite;  // Non-synthesizable: does not write output
        // Reverted
        VDouble0 revertNonSyn;  // Reverted due to being driven from non-synthesizable vertex
        VDouble0 revertMultidrive;  // Reverted due to multiple drivers
        // Additional stats
        VDouble0 cfgTrivial;  // Trivial input CFGs
        VDouble0 cfgSp;  // Series-paralel input CFGs
        VDouble0 cfgDag;  // Generic loop free input CFGs
        VDouble0 cfgCyclic;  // Cyclic input CFGs
        VDouble0 join_Using_ਰਸਤਾ_Predicate;  // Num control flow joins using full path predicate
        VDouble0 joinUsingBranchCondition;  // Num Control flow joins using dominating branch cond
    } m_synt;

ਨਿਜੀ:
    V3DfgSynthesisContext()
        : V3DfgSubContext{"Synthesis"} {}
    ~V3DfgSynthesisContext() {
        // Conversion statistics
        ਜੋੜੋ_Stat("conv / input assignments", m_conv.ਨਿਵੇਸ਼_Assignments);
        ਜੋੜੋ_Stat("conv / input expressions", m_conv.ਨਿਵੇਸ਼_Expressions);
        ਜੋੜੋ_Stat("conv / representable inputs", m_conv.representable);
        ਜੋੜੋ_Stat("conv / non-representable (impure)", m_conv.nonRepImpure);
        ਜੋੜੋ_Stat("conv / non-representable (dtype)", m_conv.nonRepDType);
        ਜੋੜੋ_Stat("conv / non-representable (lhs)", m_conv.nonRepLValue);
        ਜੋੜੋ_Stat("conv / non-representable (varref)", m_conv.nonRepVarRef);
        ਜੋੜੋ_Stat("conv / non-representable (oobsel)", m_conv.nonRepOOBSel);
        ਜੋੜੋ_Stat("conv / non-representable (node)", m_conv.non_Rep_ਗੰਢ);
        ਜੋੜੋ_Stat("conv / non-representable (unknown)", m_conv.nonRepUnknown);
        VDouble0 nConvNonRep;
        nConvNonRep += m_conv.nonRepImpure;
        nConvNonRep += m_conv.nonRepDType;
        nConvNonRep += m_conv.nonRepLValue;
        nConvNonRep += m_conv.nonRepVarRef;
        nConvNonRep += m_conv.nonRepOOBSel;
        nConvNonRep += m_conv.non_Rep_ਗੰਢ;
        nConvNonRep += m_conv.nonRepUnknown;
        VDouble0 nConvExpect;
        nConvExpect += m_conv.ਨਿਵੇਸ਼_Assignments;
        nConvExpect += m_conv.ਨਿਵੇਸ਼_Expressions;
        nConvExpect -= m_conv.representable;
        UASSERT(nConvNonRep == nConvExpect, "Inconsistent statistics / conv");

        // Synthesis statistics
        ਜੋੜੋ_Stat("synt / always blocks considered", m_synt.ਨਿਵੇਸ਼_Always);
        ਜੋੜੋ_Stat("synt / always blocks synthesized", m_synt.synthAlways);
        ਜੋੜੋ_Stat("synt / continuous assignments considered", m_synt.ਨਿਵੇਸ਼_ਸੌਂਪੋ);
        ਜੋੜੋ_Stat("synt / continuous assignments synthesized", m_synt.synth_ਸੌਂਪੋ);
        ਜੋੜੋ_Stat("synt / non-synthesizable (conv)", m_synt.nonSynConv);
        ਜੋੜੋ_Stat("synt / non-synthesizable (ext write)", m_synt.nonSynExtWrite);
        ਜੋੜੋ_Stat("synt / non-synthesizable (loop)", m_synt.nonSynLoop);
        ਜੋੜੋ_Stat("synt / non-synthesizable (stmt)", m_synt.nonSynStmt);
        ਜੋੜੋ_Stat("synt / non-synthesizable (multidrive)", m_synt.nonSynMultidrive);
        ਜੋੜੋ_Stat("synt / non-synthesizable (array)", m_synt.nonSynArray);
        ਜੋੜੋ_Stat("synt / non-synthesizable (latch)", m_synt.nonSynLatch);
        ਜੋੜੋ_Stat("synt / non-synthesizable (join input)", m_synt.non_Syn_Join_ਨਿਵੇਸ਼);
        ਜੋੜੋ_Stat("synt / non-synthesizable (false write)", m_synt.nonSynFalseWrite);
        ਜੋੜੋ_Stat("synt / reverted (non-synthesizable)", m_synt.revertNonSyn);
        ਜੋੜੋ_Stat("synt / reverted (multidrive)", m_synt.revertMultidrive);

        VDouble0 nSyntNonSyn;
        nSyntNonSyn += m_synt.nonSynConv;
        nSyntNonSyn += m_synt.nonSynExtWrite;
        nSyntNonSyn += m_synt.nonSynLoop;
        nSyntNonSyn += m_synt.nonSynStmt;
        nSyntNonSyn += m_synt.nonSynMultidrive;
        nSyntNonSyn += m_synt.nonSynArray;
        nSyntNonSyn += m_synt.nonSynLatch;
        nSyntNonSyn += m_synt.non_Syn_Join_ਨਿਵੇਸ਼;
        nSyntNonSyn += m_synt.nonSynFalseWrite;
        VDouble0 nSyntExpect;
        nSyntExpect += m_synt.ਨਿਵੇਸ਼_Always;
        nSyntExpect += m_synt.ਨਿਵੇਸ਼_ਸੌਂਪੋ;
        nSyntExpect -= m_synt.synthAlways;
        nSyntExpect -= m_synt.synth_ਸੌਂਪੋ;
        UASSERT(nSyntNonSyn == nSyntExpect, "Inconsistent statistics / synt");

        ਜੋੜੋ_Stat("synt / input CFG trivial", m_synt.cfgTrivial);
        ਜੋੜੋ_Stat("synt / input CFG sp", m_synt.cfgSp);
        ਜੋੜੋ_Stat("synt / input CFG dag", m_synt.cfgDag);
        ਜੋੜੋ_Stat("synt / input CFG cyclic", m_synt.cfgCyclic);

        ਜੋੜੋ_Stat("synt / joins using path predicate", m_synt.join_Using_ਰਸਤਾ_Predicate);
        ਜੋੜੋ_Stat("synt / joins using branch condition", m_synt.joinUsingBranchCondition);
    }
};

//######################################################################
// Top level V3DfgContext

class V3DfgContext final {
ਜਨਤਕ:
    // STATE

    // Sub contexts - keep sorted by type
    V3_Dfg_Ast_ਮੰਜ਼ਿਲ_Dfg_Context m_ast2DfgContext;
    V3_Dfg_Bin_ਮੰਜ਼ਿਲ_One_Hot_Context m_bin_ਮੰਜ਼ਿਲ_One_Hot_Context;  // Statistics context for V3DfgBinToOneHot
    V3_Dfg_Break_ਚੱਕਰਾਂ_Context m_break_ਚੱਕਰਾਂ_Context;  // Statistics context for V3DfgBreakCycles
    V3DfgCseContext m_cseContext0{"1st"};  // Statistics context for CSE stage 1
    V3DfgCseContext m_cseContext1{"2nd"};  // Statistics context for CSE stage 2
    V3_Dfg_Dfg_ਮੰਜ਼ਿਲ_Ast_Context m_dfg2AstContext;  // Statistics context for V3DfgDfgToAst
    V3DfgPeepholeContext m_peepholeContext;  // Statistics context for V3DfgPeephole
    V3_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels_Context m_ਧੱਕੋ_ਥੱਲੇ_Sels_Context;  // Statistics context for V3DfgPushDownSels
    V3DfgRegularizeContext m_regularizeContext;  // Statistics context for V3DfgRegularize
    V3DfgRemoveSelectsContext m_removeSelectsContext;  // Stats context for V3DfgRemoveSelects
    V3DfgRemoveUnobservableContext
        m_removeUnobservableContext;  // Stats context for V3DfgRemoveUnobservable
    V3DfgSynthesisContext m_synthContext;  // Statistics context for V3DfgSynthesize

    // Temporary variables created by all algorithms, shared by instances
    V3SharedTmps m_sharedTmps{"__Vdfg", VVarType::MODULETEMP};

    // CONSTRUCTOR
    V3DfgContext() = ਮੂਲ;
    ~V3DfgContext() {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, DFG, temporary declarations reused",
                         m_sharedTmps.nReused());
    }
};

#ਜੇ_ਅੰਤ  //VERILATOR_V3DFGCONTEXT_H_
