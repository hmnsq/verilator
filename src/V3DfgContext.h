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

#اگر_غیر_تعریف_شدہ VERILATOR_V3DFGCONTEXT_H_
#تعریف_کرو VERILATOR_V3DFGCONTEXT_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3DfgPeepholePatterns.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

class V3DfgContext;

//######################################################################
// Base class for all context objects

class V3DfgSubContext VL_NOT_FINAL {
    ثابت std::string m_name;  // Pass/algorithm name, to be used in statistics

محفوظ:
    VL_DEFINE_DEBUG_FUNCTIONS;

    V3DfgSubContext(ثابت std::string& name)
        : m_name{name} {}

    خالی جوڑو_Stat(ثابت std::string& what, دوہرا قدر) {
        V3Stats::جوڑو_Stat("Optimizations, DFG, " + m_name + ", " + what, قدر);
    }
};

//######################################################################
// Contexts for various algorithms - keep sorted

class V3_Dfg_Ast_مقصد_Dfg_Context final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_inputs;  // Number of input processes (logic constructs)
    VDouble0 m_representable;  // Number of combinational equations representable
    VDouble0 m_nonRepCfg;  // Non-representable due to failing to build CFG
    VDouble0 m_non_Rep_زندہ;  // Non-representable due to failing liveness analysis
    VDouble0 m_nonRepVar;  // Non-representable due to unsupported variable properties

نجی:
    V3_Dfg_Ast_مقصد_Dfg_Context()
        : V3DfgSubContext{"AstToDfg"} {}
    ~V3_Dfg_Ast_مقصد_Dfg_Context() {
        جوڑو_Stat("input processes", m_inputs);
        جوڑو_Stat("representable", m_representable);
        جوڑو_Stat("non-representable (cfg)", m_nonRepCfg);
        جوڑو_Stat("non-representable (live)", m_non_Rep_زندہ);
        جوڑو_Stat("non-representable (var)", m_nonRepVar);
        // Check the stats are consistent
        UASSERT(m_representable + m_nonRepCfg + m_non_Rep_زندہ + m_nonRepVar == m_inputs,
                "Inconsistent statistics");
    }
};
class V3_Dfg_Bin_مقصد_One_Hot_Context final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_decodersCreated;  // Number of bianry to one-hot decoders created

نجی:
    V3_Dfg_Bin_مقصد_One_Hot_Context()
        : V3DfgSubContext{"BinToOneHot"} {}
    ~V3_Dfg_Bin_مقصد_One_Hot_Context() { جوڑو_Stat("decoders created", m_decodersCreated); }
};
class V3_Dfg_Break_ادوار_Context final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_nFixed;  // Number of graphs that became acyclic
    VDouble0 m_nImproved;  // Number of graphs that were improved but still cyclic
    VDouble0 m_nUnchanged;  // Number of graphs that were left unchanged
    VDouble0 m_nImprovements;  // Number of changes made to graphs
    VDouble0 m_n_پچھلا_Inserted;  // Number of Prev vertices inserted

نجی:
    V3_Dfg_Break_ادوار_Context()
        : V3DfgSubContext{"BreakCycles"} {}
    ~V3_Dfg_Break_ادوار_Context() {
        جوڑو_Stat("made acyclic", m_nFixed);
        جوڑو_Stat("improved", m_nImproved);
        جوڑو_Stat("left unchanged", m_nUnchanged);
        جوڑو_Stat("changes applied", m_nImprovements);
        جوڑو_Stat("prev vertices inserted", m_n_پچھلا_Inserted);
    }
};
class V3DfgCseContext final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_eliminated;  // Number of common sub-expressions eliminated

نجی:
    explicit V3DfgCseContext(ثابت std::string& label)
        : V3DfgSubContext{"CSE " + label} {}
    ~V3DfgCseContext() { جوڑو_Stat("expressions eliminated", m_eliminated); }
};

class V3_Dfg_Dfg_مقصد_Ast_Context final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_outputVariables;  // Number of output variables
    VDouble0 m_outputVariablesWithDefault;  // Number of outptu variables with a default driver
    VDouble0 m_نتیجہ_Equations;  // Number of result combinational equations
    VDouble0 m_expressionsInlined;  // Number of expressions inlined directly into the Ast
    VDouble0 m_varRefsSubstituted;  // Number of variable references substituted in the Ast

نجی:
    V3_Dfg_Dfg_مقصد_Ast_Context()
        : V3DfgSubContext{"DfgToAst"} {}
    ~V3_Dfg_Dfg_مقصد_Ast_Context() {
        جوڑو_Stat("output variables", m_outputVariables);
        جوڑو_Stat("output variables with default driver", m_outputVariablesWithDefault);
        جوڑو_Stat("result equations", m_نتیجہ_Equations);
        جوڑو_Stat("expressions inlined", m_expressionsInlined);
        جوڑو_Stat("var refs substituted", m_varRefsSubstituted);
    }
};
class V3DfgPeepholeContext final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    // Enable flags for each optimization
    std::array<bool, VDfg_Peephole_نمونہ::ENUM_انتہا> m_enabled;
    // Count of applications for each optimization (for statistics)
    std::array<VDouble0, VDfg_Peephole_نمونہ::ENUM_انتہا> m_گنتی;
    std::سمتیہ<Ast_گرہ*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end

نجی:
    V3DfgPeepholeContext()
        : V3DfgSubContext{"Peephole"} {
        ثابت خودکار checkEnabled = [this](VDfg_Peephole_نمونہ id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.انتہا(), str.begin(), [](غیرعلامتی حرف c) {  //
                واپس c == '_' ? '-' : std::tolower(c);
            });
            m_enabled[id] = v3Global.opt.fDfgPeepholeEnabled(str);
        };
#تعریف_کرو OPTIMIZATION_CHECK_ENABLED(id, name) checkEnabled(VDfg_Peephole_نمونہ::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_CHECK_ENABLED)
#تعریف_منسوخ OPTIMIZATION_CHECK_ENABLED
    }
    ~V3DfgPeepholeContext() {
        برائے (Ast_گرہ* ثابت nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        }
        ثابت خودکار emitStat = [this](VDfg_Peephole_نمونہ id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.انتہا(), str.begin(), [](غیرعلامتی حرف c) {  //
                واپس c == '_' ? ' ' : std::tolower(c);
            });
            جوڑو_Stat(str, m_گنتی[id]);
        };
#تعریف_کرو OPTIMIZATION_EMIT_STATS(id, name) emitStat(VDfg_Peephole_نمونہ::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_EMIT_STATS)
#تعریف_منسوخ OPTIMIZATION_EMIT_STATS
    }
};
class V3_Dfg_ڈالو_نیچے_کو_Sels_Context final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    سائز_t m_pushed_نیچے_کو = 0;  // Number of selects pushed down through concatenations
    سائز_t m_wouldBeCyclic = 0;  // Number of selects not pushed due to cycle
نجی:
    V3_Dfg_ڈالو_نیچے_کو_Sels_Context()
        : V3DfgSubContext{"PushDownSels"} {}
    ~V3_Dfg_ڈالو_نیچے_کو_Sels_Context() {
        جوڑو_Stat("sels pushed down", m_pushed_نیچے_کو);
        جوڑو_Stat("would be cyclic", m_wouldBeCyclic);
    }
};
class V3DfgRegularizeContext final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    std::سمتیہ<Ast_گرہ*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end
    VDouble0 m_مستعمل_Vars_Inlined;  // Number of used variables inlined
    VDouble0 m_غیر_مستعمل_Removed;  // Number of unused vertices remoevd
    VDouble0 m_temporariesIntroduced;  // Number of temporaries introduced for reuse

نجی:
    V3DfgRegularizeContext()
        : V3DfgSubContext{"Regularize"} {}
    ~V3DfgRegularizeContext() {
        برائے (Ast_گرہ* ثابت nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
        }
        جوڑو_Stat("used variables inlined", m_مستعمل_Vars_Inlined);
        جوڑو_Stat("unused vertices removed", m_غیر_مستعمل_Removed);
        جوڑو_Stat("temporaries introduced", m_temporariesIntroduced);
    }
};
class V3DfgRemoveSelectsContext final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_removed_بھرا_چوڑائی;  // Number of full width selects removed
    VDouble0 m_replacedWithSelFromDriver;  // Number of selects replaced with sel from a driver
    VDouble0 m_replaced_With_پورا_Driver;  // Number of selects replaced with a whole driver
نجی:
    V3DfgRemoveSelectsContext()
        : V3DfgSubContext{"RemoveSelects"} {}
    ~V3DfgRemoveSelectsContext() {
        جوڑو_Stat("full width selects removed", m_removed_بھرا_چوڑائی);
        جوڑو_Stat("replaced with sel from driver", m_replacedWithSelFromDriver);
        جوڑو_Stat("replaced with whole driver", m_replaced_With_پورا_Driver);
    }
};
class V3DfgRemoveUnobservableContext final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE
    VDouble0 m_varsRemoved;  // Number of variables removed from the Dfg
    VDouble0 m_varsDeleted;  // Number of variables removed from the Dfg and the Ast
    VDouble0 m_logicRemoved;  // Number of logic blocks removed from the Dfg
    VDouble0 m_logicDeleted;  // Number of logic blocks removed from the Dfg and the Ast

نجی:
    V3DfgRemoveUnobservableContext()
        : V3DfgSubContext{"RemoveUnobservable"} {}
    ~V3DfgRemoveUnobservableContext() {
        جوڑو_Stat("variables removed", m_varsRemoved);
        جوڑو_Stat("variables deleted", m_varsDeleted);
        جوڑو_Stat("logic removed", m_logicRemoved);
        جوڑو_Stat("logic deleted", m_logicDeleted);
    }
};
class V3DfgSynthesisContext final : عوامی V3DfgSubContext {
    // Only V3DfgContext can create an instance
    دوست class V3DfgContext;

عوامی:
    // STATE

    // Stats for conversion
    ساخت {
        // Inputs
        VDouble0 داخلہ_Assignments;  // Number of input assignments
        VDouble0 داخلہ_Expressions;  // Number of input equations
        // Successful
        VDouble0 representable;  // Number of representable constructs
        // Unsuccessful
        VDouble0 nonRepImpure;  // Non representable: impure
        VDouble0 nonRepDType;  // Non representable: unsupported data type
        VDouble0 nonRepLValue;  // Non representable: unsupported LValue form
        VDouble0 nonRepVarRef;  // Non representable: unsupported var reference
        VDouble0 nonRepOOBSel;  // Non representable: out of bounds select
        VDouble0 non_Rep_گرہ;  // Non representable: unsupported AstNode type
        VDouble0 nonRepUnknown;  // Non representable: unhandled AstNode type
    } m_conv;

    // Stats for synthesis
    ساخت {
        // Inputs
        VDouble0 داخلہ_Always;  // Number of always blocks attempted
        VDouble0 داخلہ_تفویض;  // Number of continuous assignments attempted
        // Successful
        VDouble0 synthAlways;  // Number of always blocks successfully synthesized
        VDouble0 synth_تفویض;  // Number of continuous assignments successfully synthesized
        // Unsuccessful
        VDouble0 nonSynConv;  // Non-synthesizable: non-representable (above)
        VDouble0 nonSynExtWrite;  // Non-synthesizable: has externally written variable
        VDouble0 nonSynLoop;  // Non-synthesizable: loop in CFG
        VDouble0 nonSynStmt;  // Non-synthesizable: unsupported statement
        VDouble0 nonSynMultidrive;  // Non-synthesizable: multidriven value within statement
        VDouble0 nonSynArray;  // Non-synthesizable: array type unhandled
        VDouble0 nonSynLatch;  // Non-synthesizable: maybe latch
        VDouble0 non_Syn_Join_داخلہ;  // Non-synthesizable: needing to join input variable
        VDouble0 nonSynFalseWrite;  // Non-synthesizable: does not write output
        // Reverted
        VDouble0 revertNonSyn;  // Reverted due to being driven from non-synthesizable vertex
        VDouble0 revertMultidrive;  // Reverted due to multiple drivers
        // Additional stats
        VDouble0 cfgTrivial;  // Trivial input CFGs
        VDouble0 cfgSp;  // Series-paralel input CFGs
        VDouble0 cfgDag;  // Generic loop free input CFGs
        VDouble0 cfgCyclic;  // Cyclic input CFGs
        VDouble0 join_Using_راستہ_Predicate;  // Num control flow joins using full path predicate
        VDouble0 joinUsingBranchCondition;  // Num Control flow joins using dominating branch cond
    } m_synt;

نجی:
    V3DfgSynthesisContext()
        : V3DfgSubContext{"Synthesis"} {}
    ~V3DfgSynthesisContext() {
        // Conversion statistics
        جوڑو_Stat("conv / input assignments", m_conv.داخلہ_Assignments);
        جوڑو_Stat("conv / input expressions", m_conv.داخلہ_Expressions);
        جوڑو_Stat("conv / representable inputs", m_conv.representable);
        جوڑو_Stat("conv / non-representable (impure)", m_conv.nonRepImpure);
        جوڑو_Stat("conv / non-representable (dtype)", m_conv.nonRepDType);
        جوڑو_Stat("conv / non-representable (lhs)", m_conv.nonRepLValue);
        جوڑو_Stat("conv / non-representable (varref)", m_conv.nonRepVarRef);
        جوڑو_Stat("conv / non-representable (oobsel)", m_conv.nonRepOOBSel);
        جوڑو_Stat("conv / non-representable (node)", m_conv.non_Rep_گرہ);
        جوڑو_Stat("conv / non-representable (unknown)", m_conv.nonRepUnknown);
        VDouble0 nConvNonRep;
        nConvNonRep += m_conv.nonRepImpure;
        nConvNonRep += m_conv.nonRepDType;
        nConvNonRep += m_conv.nonRepLValue;
        nConvNonRep += m_conv.nonRepVarRef;
        nConvNonRep += m_conv.nonRepOOBSel;
        nConvNonRep += m_conv.non_Rep_گرہ;
        nConvNonRep += m_conv.nonRepUnknown;
        VDouble0 nConvExpect;
        nConvExpect += m_conv.داخلہ_Assignments;
        nConvExpect += m_conv.داخلہ_Expressions;
        nConvExpect -= m_conv.representable;
        UASSERT(nConvNonRep == nConvExpect, "Inconsistent statistics / conv");

        // Synthesis statistics
        جوڑو_Stat("synt / always blocks considered", m_synt.داخلہ_Always);
        جوڑو_Stat("synt / always blocks synthesized", m_synt.synthAlways);
        جوڑو_Stat("synt / continuous assignments considered", m_synt.داخلہ_تفویض);
        جوڑو_Stat("synt / continuous assignments synthesized", m_synt.synth_تفویض);
        جوڑو_Stat("synt / non-synthesizable (conv)", m_synt.nonSynConv);
        جوڑو_Stat("synt / non-synthesizable (ext write)", m_synt.nonSynExtWrite);
        جوڑو_Stat("synt / non-synthesizable (loop)", m_synt.nonSynLoop);
        جوڑو_Stat("synt / non-synthesizable (stmt)", m_synt.nonSynStmt);
        جوڑو_Stat("synt / non-synthesizable (multidrive)", m_synt.nonSynMultidrive);
        جوڑو_Stat("synt / non-synthesizable (array)", m_synt.nonSynArray);
        جوڑو_Stat("synt / non-synthesizable (latch)", m_synt.nonSynLatch);
        جوڑو_Stat("synt / non-synthesizable (join input)", m_synt.non_Syn_Join_داخلہ);
        جوڑو_Stat("synt / non-synthesizable (false write)", m_synt.nonSynFalseWrite);
        جوڑو_Stat("synt / reverted (non-synthesizable)", m_synt.revertNonSyn);
        جوڑو_Stat("synt / reverted (multidrive)", m_synt.revertMultidrive);

        VDouble0 nSyntNonSyn;
        nSyntNonSyn += m_synt.nonSynConv;
        nSyntNonSyn += m_synt.nonSynExtWrite;
        nSyntNonSyn += m_synt.nonSynLoop;
        nSyntNonSyn += m_synt.nonSynStmt;
        nSyntNonSyn += m_synt.nonSynMultidrive;
        nSyntNonSyn += m_synt.nonSynArray;
        nSyntNonSyn += m_synt.nonSynLatch;
        nSyntNonSyn += m_synt.non_Syn_Join_داخلہ;
        nSyntNonSyn += m_synt.nonSynFalseWrite;
        VDouble0 nSyntExpect;
        nSyntExpect += m_synt.داخلہ_Always;
        nSyntExpect += m_synt.داخلہ_تفویض;
        nSyntExpect -= m_synt.synthAlways;
        nSyntExpect -= m_synt.synth_تفویض;
        UASSERT(nSyntNonSyn == nSyntExpect, "Inconsistent statistics / synt");

        جوڑو_Stat("synt / input CFG trivial", m_synt.cfgTrivial);
        جوڑو_Stat("synt / input CFG sp", m_synt.cfgSp);
        جوڑو_Stat("synt / input CFG dag", m_synt.cfgDag);
        جوڑو_Stat("synt / input CFG cyclic", m_synt.cfgCyclic);

        جوڑو_Stat("synt / joins using path predicate", m_synt.join_Using_راستہ_Predicate);
        جوڑو_Stat("synt / joins using branch condition", m_synt.joinUsingBranchCondition);
    }
};

//######################################################################
// Top level V3DfgContext

class V3DfgContext final {
عوامی:
    // STATE

    // Sub contexts - keep sorted by type
    V3_Dfg_Ast_مقصد_Dfg_Context m_ast2DfgContext;
    V3_Dfg_Bin_مقصد_One_Hot_Context m_bin_مقصد_One_Hot_Context;  // Statistics context for V3DfgBinToOneHot
    V3_Dfg_Break_ادوار_Context m_break_ادوار_Context;  // Statistics context for V3DfgBreakCycles
    V3DfgCseContext m_cseContext0{"1st"};  // Statistics context for CSE stage 1
    V3DfgCseContext m_cseContext1{"2nd"};  // Statistics context for CSE stage 2
    V3_Dfg_Dfg_مقصد_Ast_Context m_dfg2AstContext;  // Statistics context for V3DfgDfgToAst
    V3DfgPeepholeContext m_peepholeContext;  // Statistics context for V3DfgPeephole
    V3_Dfg_ڈالو_نیچے_کو_Sels_Context m_ڈالو_نیچے_کو_Sels_Context;  // Statistics context for V3DfgPushDownSels
    V3DfgRegularizeContext m_regularizeContext;  // Statistics context for V3DfgRegularize
    V3DfgRemoveSelectsContext m_removeSelectsContext;  // Stats context for V3DfgRemoveSelects
    V3DfgRemoveUnobservableContext
        m_removeUnobservableContext;  // Stats context for V3DfgRemoveUnobservable
    V3DfgSynthesisContext m_synthContext;  // Statistics context for V3DfgSynthesize

    // Temporary variables created by all algorithms, shared by instances
    V3SharedTmps m_sharedTmps{"__Vdfg", VVarType::MODULETEMP};

    // CONSTRUCTOR
    V3DfgContext() = طےشدہ;
    ~V3DfgContext() {
        V3Stats::جوڑو_Stat("Optimizations, DFG, temporary declarations reused",
                         m_sharedTmps.nReused());
    }
};

#اگر_ختم  //VERILATOR_V3DFGCONTEXT_H_
