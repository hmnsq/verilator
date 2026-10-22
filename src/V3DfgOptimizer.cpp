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
    ಧ್ರುವ VNUser1InUse m_user1InUse;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    ಶೂನ್ಯ endOfStage(ಧ್ರುವ std::string& name) {
        ಯದಿ (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    ಶೂನ್ಯ endOfStage(ಧ್ರುವ std::string& name, ಧ್ರುವ DfgGraph& dfg,
                    ಧ್ರುವ std::vector<std::unique_ptr<DfgGraph>>& componentps) {
        // Dump the graphs for debugging
        ಯದಿ (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            ಯದಿ (dfg.size() > 0) dfg.dumpDotFilePrefixed(name);
            ಪ್ರತಿ (ಧ್ರುವ std::unique_ptr<DfgGraph>& componentp : componentps) {
                ಯದಿ (componentp->size() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        ಯದಿ (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            ಪ್ರತಿ (ಧ್ರುವ std::unique_ptr<DfgGraph>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        endOfStage(name);
    }

    // Mark variables with external references
    ಶೂನ್ಯ markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_ಗಂಟು* nodep) {
            // Check variable flags
            ಯದಿ (AstVarScope* ಧ್ರುವ vscp = VN_CAST(nodep, VarScope)) {
                ಧ್ರುವ AstVar* ಧ್ರುವ varp = vscp->varp();
                // Force and trace have already been processed
                ಧ್ರುವ ತಾರ್ಕಿಕ hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->isSigUserRdPublic()  // Readable by user
                    || varp->constPoolEntry()  // Held in V3ConstPool lookup cache, keep
                    ;
                ಧ್ರುವ ತಾರ್ಕಿಕ hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                ಯದಿ (hasExtRd) DfgVertexVar::setHasExtRdRefs(vscp);
                ಯದಿ (hasExtWr) DfgVertexVar::setHasExtWrRefs(vscp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Check references
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep, VarRef)) {
                ಯದಿ (refp->access().isRW()) DfgVertexVar::setHasRWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                ಪ್ರತ್ಯಾಗಮ;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            ಯದಿ (ಧ್ರುವ AstCell* ಧ್ರುವ cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(ಸತ್ಯ || !cellp->pinsp(), cellp, "Pins should have been lowered");
                ಪ್ರತ್ಯಾಗಮ;
            }
        });
    }

    ಶೂನ್ಯ optimize(DfgGraph& dfg) {
        // Remove unobservable variabels and logic that drives only such variables
        V3DfgPasses::removeUnobservable(dfg, m_ctx);
        endOfStage("removeUnobservable", dfg, {});

        // Synthesize DfgLogic vertices
        V3DfgPasses::synthesize(dfg, m_ctx);
        endOfStage("synthesize", dfg, {});

        // Extract the cyclic sub-graphs, so breakCycles can operate on small graphs,
        // make all of them acyclic, then merge them back to the main graph
        {
            std::vector<std::unique_ptr<DfgGraph>> comps = dfg.extractCyclicComponents("cyclic");
            ಪ್ರತಿ (ಧ್ರುವ auto& cp : comps) V3DfgPasses::breakCycles(*cp, m_ctx);
            dfg.mergeGraphs(std::move(comps));
            endOfStage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::vector<std::unique_ptr<DfgGraph>> comps = dfg.splitIntoComponents("acyclic");
        UASSERT(dfg.size() == 0, "DfgGraph should have become empty");
        endOfStage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            ಪ್ರತಿ (auto& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            endOfStage("removeSelects", dfg, comps);
            ಪ್ರತಿ (ಧ್ರುವ auto& cp : comps) V3DfgPasses::inlineVars(*cp);
            endOfStage("inlineVars", dfg, comps);
            ಪ್ರತಿ (auto& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            endOfStage("cse0", dfg, comps);
            ಪ್ರತಿ (auto& cp : comps) V3DfgPasses::binToOneHot(*cp, m_ctx.m_binToOneHotContext);
            endOfStage("binToOneHot", dfg, comps);
            ಪ್ರತಿ (auto& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            endOfStage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            ಯದಿ (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            ಪ್ರತಿ (auto& cp : comps) V3DfgPasses::ತಳ್ಳು_Down_Sels(*cp, m_ctx.m_ತಳ್ಳು_Down_Sels_Context);
            endOfStage("pushDownSels", dfg, comps);
            ಪ್ರತಿ (auto& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
            endOfStage("cse1", dfg, comps);
        }

        // Merge everything back under the main graph
        dfg.mergeGraphs(std::move(comps));
        endOfStage("optimized", dfg, {});

        // Regularize the graph after merging it all back together so all
        // references are known and we only need to iterate the Ast once
        // to replace redundant variables.
        V3DfgPasses::regularize(dfg, m_ctx.m_regularizeContext);
        endOfStage("regularize", dfg, {});
    }

    ಶೂನ್ಯ removeNeverActives(AstNetlist* netlistp) {
        std::vector<AstActive*> neverActiveps;
        netlistp->foreach([&](AstActive* activep) {
            Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ senTreep = activep->sentreep();
            ಯದಿ (!senTreep) ಪ್ರತ್ಯಾಗಮ;
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            ಯದಿ (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_back(activep);
            }
        });
        ಪ್ರತಿ (AstActive* ಧ್ರುವ activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlinkFrBack()->delete_ವೃಕ್ಷ(), activep);
        }
    }

    DataflowOptimize(AstNetlist* netlistp) {
        // Mark variables with external references
        markExternallyReferencedVariables(netlistp);
        // Dump stage stats
        endOfStage("init");
        // Post V3Scope application. Run on whole netlist.
        UINFO(4, "Applying DFG optimization to entire netlist");
        // Build the DFG of the entire netlist
        ಧ್ರುವ std::unique_ptr<DfgGraph> dfgp = V3DfgPasses::astToDfg(*netlistp, m_ctx);
        endOfStage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfgToAst(*dfgp, m_ctx);
        endOfStage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        DfgDataType::reset();
        // Dump stage stats
        endOfStage("fini");
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

ಶೂನ್ಯ V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_ವೃಕ್ಷ("dfg-optimize", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
