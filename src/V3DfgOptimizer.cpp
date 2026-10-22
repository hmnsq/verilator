// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Dataflow based optimization of combinational logic
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
// High level entry points from Ast world to the DFG optimizer.
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgOptimizer.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Dfg.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3DfgPasses.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ਵਰਗ DataflowOptimize final {
    // NODE STATE
    // AstVar::user1, AstVarScope::user1 -> int, used as a bit-field
    // - bit0: Read via AstVarXRef (hierarchical reference)
    // - bit1: Written via AstVarXRef (hierarchical reference)
    // - bit2: Read by logic in same module/netlist not represented in DFG
    // - bit3: Written by logic in same module/netlist not represented in DFG
    // - bit4: Has READWRITE references
    // - bit5: Has DfgPrev instance
    // - bit31-6: Reference count, how many DfgVertexVar represent this variable
    //
    // AstNode::user2/user3/user4 can be used by various DFG algorithms
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    ਖਾਲੀ ਸਿਰਾ_ਦਾ_Stage(ਅਚਲ std::string& name) {
        ਜੇ (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    ਖਾਲੀ ਸਿਰਾ_ਦਾ_Stage(ਅਚਲ std::string& name, ਅਚਲ Dfg_ਗ੍ਰਾਫ& dfg,
                    ਅਚਲ std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>>& componentps) {
        // Dump the graphs for debugging
        ਜੇ (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            ਜੇ (dfg.ਮਾਪ() > 0) dfg.dumpDotFilePrefixed(name);
            ਲਈ (ਅਚਲ std::unique_ptr<Dfg_ਗ੍ਰਾਫ>& componentp : componentps) {
                ਜੇ (componentp->ਮਾਪ() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        ਜੇ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            ਲਈ (ਅਚਲ std::unique_ptr<Dfg_ਗ੍ਰਾਫ>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        ਸਿਰਾ_ਦਾ_Stage(name);
    }

    // Mark variables with external references
    ਖਾਲੀ markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_ਗੰਢ* nodep) {
            // Check variable flags
            ਜੇ (AstVarScope* ਅਚਲ vscp = VN_CAST(nodep, VarScope)) {
                ਅਚਲ AstVar* ਅਚਲ varp = vscp->varp();
                // Force and trace have already been processed
                ਅਚਲ ਤਾਰਕਿਕ hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->is_Sig_User_Rd_ਜਨਤਕ()  // Readable by user
                    || varp->const_Pool_ਇੰਦਰਾਜ()  // Held in V3ConstPool lookup cache, keep
                    ;
                ਅਚਲ ਤਾਰਕਿਕ hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                ਜੇ (hasExtRd) Dfg_ਸ਼ੀਰਸ਼_Var::ਸੈੱਟ_Has_Ext_Rd_Refs(vscp);
                ਜੇ (hasExtWr) Dfg_ਸ਼ੀਰਸ਼_Var::ਸੈੱਟ_Has_Ext_Wr_Refs(vscp);
                ਵਾਪਸ;
            }
            // Check references
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
                ਜੇ (refp->access().isRW()) Dfg_ਸ਼ੀਰਸ਼_Var::ਸੈੱਟ_Has_RWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                ਵਾਪਸ;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            ਜੇ (ਅਚਲ AstCell* ਅਚਲ cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(ਸੱਚ || !cellp->pinsp(), cellp, "Pins should have been lowered");
                ਵਾਪਸ;
            }
        });
    }

    ਖਾਲੀ optimize(Dfg_ਗ੍ਰਾਫ& dfg) {
        // Remove unobservable variabels and logic that drives only such variables
        V3DfgPasses::removeUnobservable(dfg, m_ctx);
        ਸਿਰਾ_ਦਾ_Stage("removeUnobservable", dfg, {});

        // Synthesize DfgLogic vertices
        V3DfgPasses::synthesize(dfg, m_ctx);
        ਸਿਰਾ_ਦਾ_Stage("synthesize", dfg, {});

        // Extract the cyclic sub-graphs, so breakCycles can operate on small graphs,
        // make all of them acyclic, then merge them back to the main graph
        {
            std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> comps = dfg.ਕੱਢੋ_Cyclic_Components("cyclic");
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::break_ਚੱਕਰਾਂ(*cp, m_ctx);
            dfg.ਮਿਲਾਓ_Graphs(std::move(comps));
            ਸਿਰਾ_ਦਾ_Stage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::ਸਦਿਸ਼<std::unique_ptr<Dfg_ਗ੍ਰਾਫ>> comps = dfg.ਵੰਡੋ_Into_Components("acyclic");
        UASSERT(dfg.ਮਾਪ() == 0, "DfgGraph should have become empty");
        ਸਿਰਾ_ਦਾ_Stage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            ਲਈ (ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            ਸਿਰਾ_ਦਾ_Stage("removeSelects", dfg, comps);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::inlineVars(*cp);
            ਸਿਰਾ_ਦਾ_Stage("inlineVars", dfg, comps);
            ਲਈ (ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            ਸਿਰਾ_ਦਾ_Stage("cse0", dfg, comps);
            ਲਈ (ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::bin_ਮੰਜ਼ਿਲ_One_Hot(*cp, m_ctx.m_bin_ਮੰਜ਼ਿਲ_One_Hot_Context);
            ਸਿਰਾ_ਦਾ_Stage("binToOneHot", dfg, comps);
            ਲਈ (ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            ਸਿਰਾ_ਦਾ_Stage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            ਜੇ (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            ਲਈ (ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::ਧੱਕੋ_ਥੱਲੇ_Sels(*cp, m_ctx.m_ਧੱਕੋ_ਥੱਲੇ_Sels_Context);
            ਸਿਰਾ_ਦਾ_Stage("pushDownSels", dfg, comps);
            ਲਈ (ਸਵੈਚਾਲਿਤ& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
            ਸਿਰਾ_ਦਾ_Stage("cse1", dfg, comps);
        }

        // Merge everything back under the main graph
        dfg.ਮਿਲਾਓ_Graphs(std::move(comps));
        ਸਿਰਾ_ਦਾ_Stage("optimized", dfg, {});

        // Regularize the graph after merging it all back together so all
        // references are known and we only need to iterate the Ast once
        // to replace redundant variables.
        V3DfgPasses::regularize(dfg, m_ctx.m_regularizeContext);
        ਸਿਰਾ_ਦਾ_Stage("regularize", dfg, {});
    }

    ਖਾਲੀ removeNeverActives(AstNetlist* netlistp) {
        std::ਸਦਿਸ਼<Ast_ਸਰਗਰਮ*> neverActiveps;
        netlistp->foreach([&](Ast_ਸਰਗਰਮ* activep) {
            Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = activep->sentreep();
            ਜੇ (!senTreep) ਵਾਪਸ;
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            ਜੇ (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_ਪਿੱਠ(activep);
            }
        });
        ਲਈ (Ast_ਸਰਗਰਮ* ਅਚਲ activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), activep);
        }
    }

    DataflowOptimize(AstNetlist* netlistp) {
        // Mark variables with external references
        markExternallyReferencedVariables(netlistp);
        // Dump stage stats
        ਸਿਰਾ_ਦਾ_Stage("init");
        // Post V3Scope application. Run on whole netlist.
        UINFO(4, "Applying DFG optimization to entire netlist");
        // Build the DFG of the entire netlist
        ਅਚਲ std::unique_ptr<Dfg_ਗ੍ਰਾਫ> dfgp = V3DfgPasses::ast_ਮੰਜ਼ਿਲ_Dfg(*netlistp, m_ctx);
        ਸਿਰਾ_ਦਾ_Stage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfg_ਮੰਜ਼ਿਲ_Ast(*dfgp, m_ctx);
        ਸਿਰਾ_ਦਾ_Stage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        Dfg_ਅੰਕੜੇ_Type::reset();
        // Dump stage stats
        ਸਿਰਾ_ਦਾ_Stage("fini");
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

ਖਾਲੀ V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_ਰੁੱਖ("dfg-optimize", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
