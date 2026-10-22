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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3DfgOptimizer.h"

#شامل "V3Const.h"
#شامل "V3Dfg.h"
#شامل "V3DfgPasses.h"
#شامل "V3Error.h"

#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

رده DataflowOptimize final {
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
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    تهی endOfStage(ثابت std::string& name) {
        اگر (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    تهی endOfStage(ثابت std::string& name, ثابت DfgGraph& dfg,
                    ثابت std::vector<std::unique_ptr<DfgGraph>>& componentps) {
        // Dump the graphs for debugging
        اگر (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            اگر (dfg.size() > 0) dfg.dumpDotFilePrefixed(name);
            برای (ثابت std::unique_ptr<DfgGraph>& componentp : componentps) {
                اگر (componentp->size() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        اگر (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            برای (ثابت std::unique_ptr<DfgGraph>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        endOfStage(name);
    }

    // Mark variables with external references
    تهی markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_گره* nodep) {
            // Check variable flags
            اگر (AstVarScope* ثابت vscp = VN_CAST(nodep, VarScope)) {
                ثابت AstVar* ثابت varp = vscp->varp();
                // Force and trace have already been processed
                ثابت منطقی hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->isSigUserRdPublic()  // Readable by user
                    || varp->constPoolEntry()  // Held in V3ConstPool lookup cache, keep
                    ;
                ثابت منطقی hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                اگر (hasExtRd) DfgVertexVar::setHasExtRdRefs(vscp);
                اگر (hasExtWr) DfgVertexVar::setHasExtWrRefs(vscp);
                بازگشت;
            }
            // Check references
            اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                اگر (refp->access().isRW()) DfgVertexVar::setHasRWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                بازگشت;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(درست || !cellp->pinsp(), cellp, "Pins should have been lowered");
                بازگشت;
            }
        });
    }

    تهی optimize(DfgGraph& dfg) {
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
            برای (ثابت auto& cp : comps) V3DfgPasses::breakCycles(*cp, m_ctx);
            dfg.mergeGraphs(std::move(comps));
            endOfStage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::vector<std::unique_ptr<DfgGraph>> comps = dfg.splitIntoComponents("acyclic");
        UASSERT(dfg.size() == 0, "DfgGraph should have become empty");
        endOfStage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            برای (auto& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            endOfStage("removeSelects", dfg, comps);
            برای (ثابت auto& cp : comps) V3DfgPasses::inlineVars(*cp);
            endOfStage("inlineVars", dfg, comps);
            برای (auto& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            endOfStage("cse0", dfg, comps);
            برای (auto& cp : comps) V3DfgPasses::binToOneHot(*cp, m_ctx.m_binToOneHotContext);
            endOfStage("binToOneHot", dfg, comps);
            برای (auto& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            endOfStage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            اگر (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            برای (auto& cp : comps) V3DfgPasses::افزودن_Down_Sels(*cp, m_ctx.m_افزودن_Down_Sels_Context);
            endOfStage("pushDownSels", dfg, comps);
            برای (auto& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
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

    تهی removeNeverActives(AstNetlist* netlistp) {
        std::vector<AstActive*> neverActiveps;
        netlistp->foreach([&](AstActive* activep) {
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            اگر (!senTreep) بازگشت;
            ثابت Ast_گره* ثابت nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            اگر (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_back(activep);
            }
        });
        برای (AstActive* ثابت activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlinkFrBack()->delete_درخت(), activep);
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
        ثابت std::unique_ptr<DfgGraph> dfgp = V3DfgPasses::astToDfg(*netlistp, m_ctx);
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

عمومی:
    ایستا تهی apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

تهی V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_درخت("dfg-optimize", 0, dump_درخت_Either_Level() >= 3);
}
