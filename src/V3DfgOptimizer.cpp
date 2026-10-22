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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3DfgOptimizer.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Dfg.h"
#ಅನ್ತರ್ಭಾವ "V3DfgPasses.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"

#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

ವರ್ಗ DataflowOptimize final {
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
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_user1_ಒಳ_Use;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    ಶೂನ್ಯ ಕೊನೆ_ದ_Stage(ಸ್ಥಿರಾಂಕ std::string& name) {
        ಆದರೆ (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    ಶೂನ್ಯ ಕೊನೆ_ದ_Stage(ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ Dfg_ನಕ್ಷೆ& dfg,
                    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>>& componentps) {
        // Dump the graphs for debugging
        ಆದರೆ (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            ಆದರೆ (dfg.ಗಾತ್ರ_2() > 0) dfg.dumpDotFilePrefixed(name);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::unique_ptr<Dfg_ನಕ್ಷೆ>& componentp : componentps) {
                ಆದರೆ (componentp->ಗಾತ್ರ_2() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        ಆದರೆ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::unique_ptr<Dfg_ನಕ್ಷೆ>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        ಕೊನೆ_ದ_Stage(name);
    }

    // Mark variables with external references
    ಶೂನ್ಯ markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_ಗಂಟು* nodep) {
            // Check variable flags
            ಆದರೆ (AstVarScope* ಸ್ಥಿರಾಂಕ vscp = VN_CAST(nodep, VarScope)) {
                ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
                // Force and trace have already been processed
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->is_Sig_User_Rd_ಬಹಿರಂಗ()  // Readable by user
                    || varp->const_Pool_ನಮೂದು()  // Held in V3ConstPool lookup cache, keep
                    ;
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                ಆದರೆ (hasExtRd) Dfg_ಶೃಂಗ_Var::ಗಣ_Has_Ext_Rd_Refs(vscp);
                ಆದರೆ (hasExtWr) Dfg_ಶೃಂಗ_Var::ಗಣ_Has_Ext_Wr_Refs(vscp);
                ಹಿಂತಿರುಗಿ;
            }
            // Check references
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(nodep, VarRef)) {
                ಆದರೆ (refp->access().isRW()) Dfg_ಶೃಂಗ_Var::ಗಣ_Has_RWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                ಹಿಂತಿರುಗಿ;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCell* ಸ್ಥಿರಾಂಕ cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(ನಿಜ || !cellp->pinsp(), cellp, "Pins should have been lowered");
                ಹಿಂತಿರುಗಿ;
            }
        });
    }

    ಶೂನ್ಯ optimize(Dfg_ನಕ್ಷೆ& dfg) {
        // Remove unobservable variabels and logic that drives only such variables
        V3DfgPasses::removeUnobservable(dfg, m_ctx);
        ಕೊನೆ_ದ_Stage("removeUnobservable", dfg, {});

        // Synthesize DfgLogic vertices
        V3DfgPasses::synthesize(dfg, m_ctx);
        ಕೊನೆ_ದ_Stage("synthesize", dfg, {});

        // Extract the cyclic sub-graphs, so breakCycles can operate on small graphs,
        // make all of them acyclic, then merge them back to the main graph
        {
            std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> comps = dfg.ಹೊರತೆಗೆ_Cyclic_Components("cyclic");
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::break_ಚಕ್ರಗಳು(*cp, m_ctx);
            dfg.ವಿಲೀನ_Graphs(std::move(comps));
            ಕೊನೆ_ದ_Stage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::ಸದಿಶ<std::unique_ptr<Dfg_ನಕ್ಷೆ>> comps = dfg.ವಿಭಜಿಸು_Into_Components("acyclic");
        UASSERT(dfg.ಗಾತ್ರ_2() == 0, "DfgGraph should have become empty");
        ಕೊನೆ_ದ_Stage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            ಕೊನೆ_ದ_Stage("removeSelects", dfg, comps);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::inlineVars(*cp);
            ಕೊನೆ_ದ_Stage("inlineVars", dfg, comps);
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            ಕೊನೆ_ದ_Stage("cse0", dfg, comps);
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::bin_ಗಮ್ಯ_One_Hot(*cp, m_ctx.m_bin_ಗಮ್ಯ_One_Hot_Context);
            ಕೊನೆ_ದ_Stage("binToOneHot", dfg, comps);
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            ಕೊನೆ_ದ_Stage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            ಆದರೆ (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels(*cp, m_ctx.m_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels_Context);
            ಕೊನೆ_ದ_Stage("pushDownSels", dfg, comps);
            ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
            ಕೊನೆ_ದ_Stage("cse1", dfg, comps);
        }

        // Merge everything back under the main graph
        dfg.ವಿಲೀನ_Graphs(std::move(comps));
        ಕೊನೆ_ದ_Stage("optimized", dfg, {});

        // Regularize the graph after merging it all back together so all
        // references are known and we only need to iterate the Ast once
        // to replace redundant variables.
        V3DfgPasses::regularize(dfg, m_ctx.m_regularizeContext);
        ಕೊನೆ_ದ_Stage("regularize", dfg, {});
    }

    ಶೂನ್ಯ removeNeverActives(AstNetlist* netlistp) {
        std::ಸದಿಶ<Ast_ಸಕ್ರಿಯ*> neverActiveps;
        netlistp->foreach([&](Ast_ಸಕ್ರಿಯ* activep) {
            Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = activep->sentreep();
            ಆದರೆ (!senTreep) ಹಿಂತಿರುಗಿ;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            ಆದರೆ (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_ಹಿಂಭಾಗ(activep);
            }
        });
        ಗಾಗಿ (Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), activep);
        }
    }

    DataflowOptimize(AstNetlist* netlistp) {
        // Mark variables with external references
        markExternallyReferencedVariables(netlistp);
        // Dump stage stats
        ಕೊನೆ_ದ_Stage("init");
        // Post V3Scope application. Run on whole netlist.
        UINFO(4, "Applying DFG optimization to entire netlist");
        // Build the DFG of the entire netlist
        ಸ್ಥಿರಾಂಕ std::unique_ptr<Dfg_ನಕ್ಷೆ> dfgp = V3DfgPasses::ast_ಗಮ್ಯ_Dfg(*netlistp, m_ctx);
        ಕೊನೆ_ದ_Stage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfg_ಗಮ್ಯ_Ast(*dfgp, m_ctx);
        ಕೊನೆ_ದ_Stage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        Dfg_ದತ್ತಾಂಶ_Type::reset();
        // Dump stage stats
        ಕೊನೆ_ದ_Stage("fini");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

ಶೂನ್ಯ V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("dfg-optimize", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
