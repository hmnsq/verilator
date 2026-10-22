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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3DfgOptimizer.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Dfg.h"
#అన్తర్భావ "V3DfgPasses.h"
#అన్తర్భావ "V3Error.h"

#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

తరగతి DataflowOptimize final {
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
    స్థిరరాశి VNUser1_లోపల_Use m_user1_లోపల_Use;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    శూన్య చివర_యొక్క_Stage(స్థిరరాశి std::string& name) {
        ఒకవేళ (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    శూన్య చివర_యొక్క_Stage(స్థిరరాశి std::string& name, స్థిరరాశి Dfg_లేఖాచిత్రం& dfg,
                    స్థిరరాశి std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>>& componentps) {
        // Dump the graphs for debugging
        ఒకవేళ (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            ఒకవేళ (dfg.కొలత() > 0) dfg.dumpDotFilePrefixed(name);
            కొరకు (స్థిరరాశి std::unique_ptr<Dfg_లేఖాచిత్రం>& componentp : componentps) {
                ఒకవేళ (componentp->కొలత() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        ఒకవేళ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            కొరకు (స్థిరరాశి std::unique_ptr<Dfg_లేఖాచిత్రం>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        చివర_యొక్క_Stage(name);
    }

    // Mark variables with external references
    శూన్య markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_కణుపు* nodep) {
            // Check variable flags
            ఒకవేళ (AstVarScope* స్థిరరాశి vscp = VN_CAST(nodep, VarScope)) {
                స్థిరరాశి AstVar* స్థిరరాశి varp = vscp->varp();
                // Force and trace have already been processed
                స్థిరరాశి తార్కిక hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->is_Sig_User_Rd_ప్రజా()  // Readable by user
                    || varp->const_Pool_నమోదు()  // Held in V3ConstPool lookup cache, keep
                    ;
                స్థిరరాశి తార్కిక hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                ఒకవేళ (hasExtRd) Dfg_శీర్షం_Var::సమితి_Has_Ext_Rd_Refs(vscp);
                ఒకవేళ (hasExtWr) Dfg_శీర్షం_Var::సమితి_Has_Ext_Wr_Refs(vscp);
                తిరిగి;
            }
            // Check references
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep, VarRef)) {
                ఒకవేళ (refp->access().isRW()) Dfg_శీర్షం_Var::సమితి_Has_RWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                తిరిగి;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            ఒకవేళ (స్థిరరాశి AstCell* స్థిరరాశి cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(నిజం || !cellp->pinsp(), cellp, "Pins should have been lowered");
                తిరిగి;
            }
        });
    }

    శూన్య optimize(Dfg_లేఖాచిత్రం& dfg) {
        // Remove unobservable variabels and logic that drives only such variables
        V3DfgPasses::removeUnobservable(dfg, m_ctx);
        చివర_యొక్క_Stage("removeUnobservable", dfg, {});

        // Synthesize DfgLogic vertices
        V3DfgPasses::synthesize(dfg, m_ctx);
        చివర_యొక్క_Stage("synthesize", dfg, {});

        // Extract the cyclic sub-graphs, so breakCycles can operate on small graphs,
        // make all of them acyclic, then merge them back to the main graph
        {
            std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> comps = dfg.వెలికితీయి_Cyclic_Components("cyclic");
            కొరకు (స్థిరరాశి స్వయంచాలక& cp : comps) V3DfgPasses::break_చక్రాలు(*cp, m_ctx);
            dfg.విలీనం_Graphs(std::move(comps));
            చివర_యొక్క_Stage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::సదిశ<std::unique_ptr<Dfg_లేఖాచిత్రం>> comps = dfg.విభజించు_Into_Components("acyclic");
        UASSERT(dfg.కొలత() == 0, "DfgGraph should have become empty");
        చివర_యొక్క_Stage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            కొరకు (స్వయంచాలక& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            చివర_యొక్క_Stage("removeSelects", dfg, comps);
            కొరకు (స్థిరరాశి స్వయంచాలక& cp : comps) V3DfgPasses::inlineVars(*cp);
            చివర_యొక్క_Stage("inlineVars", dfg, comps);
            కొరకు (స్వయంచాలక& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            చివర_యొక్క_Stage("cse0", dfg, comps);
            కొరకు (స్వయంచాలక& cp : comps) V3DfgPasses::bin_గమ్యం_One_Hot(*cp, m_ctx.m_bin_గమ్యం_One_Hot_Context);
            చివర_యొక్క_Stage("binToOneHot", dfg, comps);
            కొరకు (స్వయంచాలక& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            చివర_యొక్క_Stage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            ఒకవేళ (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            కొరకు (స్వయంచాలక& cp : comps) V3DfgPasses::నెట్టు_కిందికి_Sels(*cp, m_ctx.m_నెట్టు_కిందికి_Sels_Context);
            చివర_యొక్క_Stage("pushDownSels", dfg, comps);
            కొరకు (స్వయంచాలక& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
            చివర_యొక్క_Stage("cse1", dfg, comps);
        }

        // Merge everything back under the main graph
        dfg.విలీనం_Graphs(std::move(comps));
        చివర_యొక్క_Stage("optimized", dfg, {});

        // Regularize the graph after merging it all back together so all
        // references are known and we only need to iterate the Ast once
        // to replace redundant variables.
        V3DfgPasses::regularize(dfg, m_ctx.m_regularizeContext);
        చివర_యొక్క_Stage("regularize", dfg, {});
    }

    శూన్య removeNeverActives(AstNetlist* netlistp) {
        std::సదిశ<Ast_చురుకైన*> neverActiveps;
        netlistp->foreach([&](Ast_చురుకైన* activep) {
            Ast_Sen_వృక్షం* స్థిరరాశి senTreep = activep->sentreep();
            ఒకవేళ (!senTreep) తిరిగి;
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            ఒకవేళ (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_వెనుక(activep);
            }
        });
        కొరకు (Ast_చురుకైన* స్థిరరాశి activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlink_Fr_వెనుక()->delete_వృక్షం(), activep);
        }
    }

    DataflowOptimize(AstNetlist* netlistp) {
        // Mark variables with external references
        markExternallyReferencedVariables(netlistp);
        // Dump stage stats
        చివర_యొక్క_Stage("init");
        // Post V3Scope application. Run on whole netlist.
        UINFO(4, "Applying DFG optimization to entire netlist");
        // Build the DFG of the entire netlist
        స్థిరరాశి std::unique_ptr<Dfg_లేఖాచిత్రం> dfgp = V3DfgPasses::ast_గమ్యం_Dfg(*netlistp, m_ctx);
        చివర_యొక్క_Stage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfg_గమ్యం_Ast(*dfgp, m_ctx);
        చివర_యొక్క_Stage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        Dfg_దత్తాంశం_Type::reset();
        // Dump stage stats
        చివర_యొక్క_Stage("fini");
    }

బహిరంగ:
    స్థిర శూన్య apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

శూన్య V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_వృక్షం("dfg-optimize", 0, dump_వృక్షం_Either_Level() >= 3);
}
