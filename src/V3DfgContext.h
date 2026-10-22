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

#ifndef VERILATOR_V3DFGCONTEXT_H_
#define VERILATOR_V3DFGCONTEXT_H_

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

    शून्य addStat(ध्रुव std::string& what, द्विगुण मूल्य) {
        V3Stats::addStat("Optimizations, DFG, " + m_name + ", " + what, मूल्य);
    }
};

//######################################################################
// Contexts for various algorithms - keep sorted

class V3DfgAstToDfgContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_inputs;  // Number of input processes (logic constructs)
    VDouble0 m_representable;  // Number of combinational equations representable
    VDouble0 m_nonRepCfg;  // Non-representable due to failing to build CFG
    VDouble0 m_nonRepLive;  // Non-representable due to failing liveness analysis
    VDouble0 m_nonRepVar;  // Non-representable due to unsupported variable properties

गुप्त:
    V3DfgAstToDfgContext()
        : V3DfgSubContext{"AstToDfg"} {}
    ~V3DfgAstToDfgContext() {
        addStat("input processes", m_inputs);
        addStat("representable", m_representable);
        addStat("non-representable (cfg)", m_nonRepCfg);
        addStat("non-representable (live)", m_nonRepLive);
        addStat("non-representable (var)", m_nonRepVar);
        // Check the stats are consistent
        UASSERT(m_representable + m_nonRepCfg + m_nonRepLive + m_nonRepVar == m_inputs,
                "Inconsistent statistics");
    }
};
class V3DfgBinToOneHotContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_decodersCreated;  // Number of bianry to one-hot decoders created

गुप्त:
    V3DfgBinToOneHotContext()
        : V3DfgSubContext{"BinToOneHot"} {}
    ~V3DfgBinToOneHotContext() { addStat("decoders created", m_decodersCreated); }
};
class V3DfgBreakCyclesContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_nFixed;  // Number of graphs that became acyclic
    VDouble0 m_nImproved;  // Number of graphs that were improved but still cyclic
    VDouble0 m_nUnchanged;  // Number of graphs that were left unchanged
    VDouble0 m_nImprovements;  // Number of changes made to graphs
    VDouble0 m_nPrevInserted;  // Number of Prev vertices inserted

गुप्त:
    V3DfgBreakCyclesContext()
        : V3DfgSubContext{"BreakCycles"} {}
    ~V3DfgBreakCyclesContext() {
        addStat("made acyclic", m_nFixed);
        addStat("improved", m_nImproved);
        addStat("left unchanged", m_nUnchanged);
        addStat("changes applied", m_nImprovements);
        addStat("prev vertices inserted", m_nPrevInserted);
    }
};
class V3DfgCseContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_eliminated;  // Number of common sub-expressions eliminated

गुप्त:
    explicit V3DfgCseContext(ध्रुव std::string& label)
        : V3DfgSubContext{"CSE " + label} {}
    ~V3DfgCseContext() { addStat("expressions eliminated", m_eliminated); }
};

class V3DfgDfgToAstContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_outputVariables;  // Number of output variables
    VDouble0 m_outputVariablesWithDefault;  // Number of outptu variables with a default driver
    VDouble0 m_resultEquations;  // Number of result combinational equations
    VDouble0 m_expressionsInlined;  // Number of expressions inlined directly into the Ast
    VDouble0 m_varRefsSubstituted;  // Number of variable references substituted in the Ast

गुप्त:
    V3DfgDfgToAstContext()
        : V3DfgSubContext{"DfgToAst"} {}
    ~V3DfgDfgToAstContext() {
        addStat("output variables", m_outputVariables);
        addStat("output variables with default driver", m_outputVariablesWithDefault);
        addStat("result equations", m_resultEquations);
        addStat("expressions inlined", m_expressionsInlined);
        addStat("var refs substituted", m_varRefsSubstituted);
    }
};
class V3DfgPeepholeContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    // Enable flags for each optimization
    std::array<bool, VDfgPeepholePattern::_ENUM_END> m_enabled;
    // Count of applications for each optimization (for statistics)
    std::array<VDouble0, VDfgPeepholePattern::_ENUM_END> m_गिनती;
    std::vector<Ast_गाँठ*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end

गुप्त:
    V3DfgPeepholeContext()
        : V3DfgSubContext{"Peephole"} {
        ध्रुव auto checkEnabled = [this](VDfgPeepholePattern id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned वर्ण c) {  //
                प्रत्यागम c == '_' ? '-' : std::tolower(c);
            });
            m_enabled[id] = v3Global.opt.fDfgPeepholeEnabled(str);
        };
#define OPTIMIZATION_CHECK_ENABLED(id, name) checkEnabled(VDfgPeepholePattern::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_CHECK_ENABLED)
#undef OPTIMIZATION_CHECK_ENABLED
    }
    ~V3DfgPeepholeContext() {
        प्रति (Ast_गाँठ* ध्रुव nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        }
        ध्रुव auto emitStat = [this](VDfgPeepholePattern id) {
            std::string str{id.ascii()};
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned वर्ण c) {  //
                प्रत्यागम c == '_' ? ' ' : std::tolower(c);
            });
            addStat(str, m_गिनती[id]);
        };
#define OPTIMIZATION_EMIT_STATS(id, name) emitStat(VDfgPeepholePattern::id);
        FOR_EACH_DFG_PEEPHOLE_OPTIMIZATION(OPTIMIZATION_EMIT_STATS)
#undef OPTIMIZATION_EMIT_STATS
    }
};
class V3_Dfg_धकेलो_Down_Sels_Context final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    size_t m_pushedDown = 0;  // Number of selects pushed down through concatenations
    size_t m_wouldBeCyclic = 0;  // Number of selects not pushed due to cycle
गुप्त:
    V3_Dfg_धकेलो_Down_Sels_Context()
        : V3DfgSubContext{"PushDownSels"} {}
    ~V3_Dfg_धकेलो_Down_Sels_Context() {
        addStat("sels pushed down", m_pushedDown);
        addStat("would be cyclic", m_wouldBeCyclic);
    }
};
class V3DfgRegularizeContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    std::vector<Ast_गाँठ*> m_deleteps;  // AstVar/AstVarScope that can be deleted at the end
    VDouble0 m_usedVarsInlined;  // Number of used variables inlined
    VDouble0 m_unusedRemoved;  // Number of unused vertices remoevd
    VDouble0 m_temporariesIntroduced;  // Number of temporaries introduced for reuse

गुप्त:
    V3DfgRegularizeContext()
        : V3DfgSubContext{"Regularize"} {}
    ~V3DfgRegularizeContext() {
        प्रति (Ast_गाँठ* ध्रुव nodep : m_deleteps) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        }
        addStat("used variables inlined", m_usedVarsInlined);
        addStat("unused vertices removed", m_unusedRemoved);
        addStat("temporaries introduced", m_temporariesIntroduced);
    }
};
class V3DfgRemoveSelectsContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE
    VDouble0 m_removedFullWidth;  // Number of full width selects removed
    VDouble0 m_replacedWithSelFromDriver;  // Number of selects replaced with sel from a driver
    VDouble0 m_replacedWithWholeDriver;  // Number of selects replaced with a whole driver
गुप्त:
    V3DfgRemoveSelectsContext()
        : V3DfgSubContext{"RemoveSelects"} {}
    ~V3DfgRemoveSelectsContext() {
        addStat("full width selects removed", m_removedFullWidth);
        addStat("replaced with sel from driver", m_replacedWithSelFromDriver);
        addStat("replaced with whole driver", m_replacedWithWholeDriver);
    }
};
class V3DfgRemoveUnobservableContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

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
        addStat("variables removed", m_varsRemoved);
        addStat("variables deleted", m_varsDeleted);
        addStat("logic removed", m_logicRemoved);
        addStat("logic deleted", m_logicDeleted);
    }
};
class V3DfgSynthesisContext final : सार्वजनिक V3DfgSubContext {
    // Only V3DfgContext can create an instance
    friend class V3DfgContext;

सार्वजनिक:
    // STATE

    // Stats for conversion
    संरचना {
        // Inputs
        VDouble0 निवेश_Assignments;  // Number of input assignments
        VDouble0 निवेश_Expressions;  // Number of input equations
        // Successful
        VDouble0 representable;  // Number of representable constructs
        // Unsuccessful
        VDouble0 nonRepImpure;  // Non representable: impure
        VDouble0 nonRepDType;  // Non representable: unsupported data type
        VDouble0 nonRepLValue;  // Non representable: unsupported LValue form
        VDouble0 nonRepVarRef;  // Non representable: unsupported var reference
        VDouble0 nonRepOOBSel;  // Non representable: out of bounds select
        VDouble0 non_Rep_गाँठ;  // Non representable: unsupported AstNode type
        VDouble0 nonRepUnknown;  // Non representable: unhandled AstNode type
    } m_conv;

    // Stats for synthesis
    संरचना {
        // Inputs
        VDouble0 निवेश_Always;  // Number of always blocks attempted
        VDouble0 निवेश_Assign;  // Number of continuous assignments attempted
        // Successful
        VDouble0 synthAlways;  // Number of always blocks successfully synthesized
        VDouble0 synthAssign;  // Number of continuous assignments successfully synthesized
        // Unsuccessful
        VDouble0 nonSynConv;  // Non-synthesizable: non-representable (above)
        VDouble0 nonSynExtWrite;  // Non-synthesizable: has externally written variable
        VDouble0 nonSynLoop;  // Non-synthesizable: loop in CFG
        VDouble0 nonSynStmt;  // Non-synthesizable: unsupported statement
        VDouble0 nonSynMultidrive;  // Non-synthesizable: multidriven value within statement
        VDouble0 nonSynArray;  // Non-synthesizable: array type unhandled
        VDouble0 nonSynLatch;  // Non-synthesizable: maybe latch
        VDouble0 non_Syn_Join_निवेश;  // Non-synthesizable: needing to join input variable
        VDouble0 nonSynFalseWrite;  // Non-synthesizable: does not write output
        // Reverted
        VDouble0 revertNonSyn;  // Reverted due to being driven from non-synthesizable vertex
        VDouble0 revertMultidrive;  // Reverted due to multiple drivers
        // Additional stats
        VDouble0 cfgTrivial;  // Trivial input CFGs
        VDouble0 cfgSp;  // Series-paralel input CFGs
        VDouble0 cfgDag;  // Generic loop free input CFGs
        VDouble0 cfgCyclic;  // Cyclic input CFGs
        VDouble0 joinUsingPathPredicate;  // Num control flow joins using full path predicate
        VDouble0 joinUsingBranchCondition;  // Num Control flow joins using dominating branch cond
    } m_synt;

गुप्त:
    V3DfgSynthesisContext()
        : V3DfgSubContext{"Synthesis"} {}
    ~V3DfgSynthesisContext() {
        // Conversion statistics
        addStat("conv / input assignments", m_conv.निवेश_Assignments);
        addStat("conv / input expressions", m_conv.निवेश_Expressions);
        addStat("conv / representable inputs", m_conv.representable);
        addStat("conv / non-representable (impure)", m_conv.nonRepImpure);
        addStat("conv / non-representable (dtype)", m_conv.nonRepDType);
        addStat("conv / non-representable (lhs)", m_conv.nonRepLValue);
        addStat("conv / non-representable (varref)", m_conv.nonRepVarRef);
        addStat("conv / non-representable (oobsel)", m_conv.nonRepOOBSel);
        addStat("conv / non-representable (node)", m_conv.non_Rep_गाँठ);
        addStat("conv / non-representable (unknown)", m_conv.nonRepUnknown);
        VDouble0 nConvNonRep;
        nConvNonRep += m_conv.nonRepImpure;
        nConvNonRep += m_conv.nonRepDType;
        nConvNonRep += m_conv.nonRepLValue;
        nConvNonRep += m_conv.nonRepVarRef;
        nConvNonRep += m_conv.nonRepOOBSel;
        nConvNonRep += m_conv.non_Rep_गाँठ;
        nConvNonRep += m_conv.nonRepUnknown;
        VDouble0 nConvExpect;
        nConvExpect += m_conv.निवेश_Assignments;
        nConvExpect += m_conv.निवेश_Expressions;
        nConvExpect -= m_conv.representable;
        UASSERT(nConvNonRep == nConvExpect, "Inconsistent statistics / conv");

        // Synthesis statistics
        addStat("synt / always blocks considered", m_synt.निवेश_Always);
        addStat("synt / always blocks synthesized", m_synt.synthAlways);
        addStat("synt / continuous assignments considered", m_synt.निवेश_Assign);
        addStat("synt / continuous assignments synthesized", m_synt.synthAssign);
        addStat("synt / non-synthesizable (conv)", m_synt.nonSynConv);
        addStat("synt / non-synthesizable (ext write)", m_synt.nonSynExtWrite);
        addStat("synt / non-synthesizable (loop)", m_synt.nonSynLoop);
        addStat("synt / non-synthesizable (stmt)", m_synt.nonSynStmt);
        addStat("synt / non-synthesizable (multidrive)", m_synt.nonSynMultidrive);
        addStat("synt / non-synthesizable (array)", m_synt.nonSynArray);
        addStat("synt / non-synthesizable (latch)", m_synt.nonSynLatch);
        addStat("synt / non-synthesizable (join input)", m_synt.non_Syn_Join_निवेश);
        addStat("synt / non-synthesizable (false write)", m_synt.nonSynFalseWrite);
        addStat("synt / reverted (non-synthesizable)", m_synt.revertNonSyn);
        addStat("synt / reverted (multidrive)", m_synt.revertMultidrive);

        VDouble0 nSyntNonSyn;
        nSyntNonSyn += m_synt.nonSynConv;
        nSyntNonSyn += m_synt.nonSynExtWrite;
        nSyntNonSyn += m_synt.nonSynLoop;
        nSyntNonSyn += m_synt.nonSynStmt;
        nSyntNonSyn += m_synt.nonSynMultidrive;
        nSyntNonSyn += m_synt.nonSynArray;
        nSyntNonSyn += m_synt.nonSynLatch;
        nSyntNonSyn += m_synt.non_Syn_Join_निवेश;
        nSyntNonSyn += m_synt.nonSynFalseWrite;
        VDouble0 nSyntExpect;
        nSyntExpect += m_synt.निवेश_Always;
        nSyntExpect += m_synt.निवेश_Assign;
        nSyntExpect -= m_synt.synthAlways;
        nSyntExpect -= m_synt.synthAssign;
        UASSERT(nSyntNonSyn == nSyntExpect, "Inconsistent statistics / synt");

        addStat("synt / input CFG trivial", m_synt.cfgTrivial);
        addStat("synt / input CFG sp", m_synt.cfgSp);
        addStat("synt / input CFG dag", m_synt.cfgDag);
        addStat("synt / input CFG cyclic", m_synt.cfgCyclic);

        addStat("synt / joins using path predicate", m_synt.joinUsingPathPredicate);
        addStat("synt / joins using branch condition", m_synt.joinUsingBranchCondition);
    }
};

//######################################################################
// Top level V3DfgContext

class V3DfgContext final {
सार्वजनिक:
    // STATE

    // Sub contexts - keep sorted by type
    V3DfgAstToDfgContext m_ast2DfgContext;
    V3DfgBinToOneHotContext m_binToOneHotContext;  // Statistics context for V3DfgBinToOneHot
    V3DfgBreakCyclesContext m_breakCyclesContext;  // Statistics context for V3DfgBreakCycles
    V3DfgCseContext m_cseContext0{"1st"};  // Statistics context for CSE stage 1
    V3DfgCseContext m_cseContext1{"2nd"};  // Statistics context for CSE stage 2
    V3DfgDfgToAstContext m_dfg2AstContext;  // Statistics context for V3DfgDfgToAst
    V3DfgPeepholeContext m_peepholeContext;  // Statistics context for V3DfgPeephole
    V3_Dfg_धकेलो_Down_Sels_Context m_धकेलो_Down_Sels_Context;  // Statistics context for V3DfgPushDownSels
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
        V3Stats::addStat("Optimizations, DFG, temporary declarations reused",
                         m_sharedTmps.nReused());
    }
};

#endif  //VERILATOR_V3DFGCONTEXT_H_
