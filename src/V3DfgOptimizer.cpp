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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3DfgOptimizer.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Dfg.h"
#অন্তর্ভাব "V3DfgPasses.h"
#অন্তর্ভাব "V3Error.h"

#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

বর্গ DataflowOptimize final {
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
    ধ্রুব VNUser1InUse m_user1InUse;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    শূন্য endOfStage(ধ্রুব std::string& name) {
        যদি (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    শূন্য endOfStage(ধ্রুব std::string& name, ধ্রুব DfgGraph& dfg,
                    ধ্রুব std::vector<std::unique_ptr<DfgGraph>>& componentps) {
        // Dump the graphs for debugging
        যদি (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            যদি (dfg.size() > 0) dfg.dumpDotFilePrefixed(name);
            প্রতি (ধ্রুব std::unique_ptr<DfgGraph>& componentp : componentps) {
                যদি (componentp->size() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        যদি (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            প্রতি (ধ্রুব std::unique_ptr<DfgGraph>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        endOfStage(name);
    }

    // Mark variables with external references
    শূন্য markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_গ্রন্থি* nodep) {
            // Check variable flags
            যদি (AstVarScope* ধ্রুব vscp = VN_CAST(nodep, VarScope)) {
                ধ্রুব AstVar* ধ্রুব varp = vscp->varp();
                // Force and trace have already been processed
                ধ্রুব তার্কিক hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->isSigUserRdPublic()  // Readable by user
                    || varp->constPoolEntry()  // Held in V3ConstPool lookup cache, keep
                    ;
                ধ্রুব তার্কিক hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                যদি (hasExtRd) DfgVertexVar::setHasExtRdRefs(vscp);
                যদি (hasExtWr) DfgVertexVar::setHasExtWrRefs(vscp);
                প্রত্যাগম;
            }
            // Check references
            যদি (ধ্রুব AstVarRef* ধ্রুব refp = VN_CAST(nodep, VarRef)) {
                যদি (refp->access().isRW()) DfgVertexVar::setHasRWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                প্রত্যাগম;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            যদি (ধ্রুব AstCell* ধ্রুব cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(সত্য || !cellp->pinsp(), cellp, "Pins should have been lowered");
                প্রত্যাগম;
            }
        });
    }

    শূন্য optimize(DfgGraph& dfg) {
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
            প্রতি (ধ্রুব auto& cp : comps) V3DfgPasses::breakCycles(*cp, m_ctx);
            dfg.mergeGraphs(std::move(comps));
            endOfStage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::vector<std::unique_ptr<DfgGraph>> comps = dfg.splitIntoComponents("acyclic");
        UASSERT(dfg.size() == 0, "DfgGraph should have become empty");
        endOfStage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            প্রতি (auto& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            endOfStage("removeSelects", dfg, comps);
            প্রতি (ধ্রুব auto& cp : comps) V3DfgPasses::inlineVars(*cp);
            endOfStage("inlineVars", dfg, comps);
            প্রতি (auto& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            endOfStage("cse0", dfg, comps);
            প্রতি (auto& cp : comps) V3DfgPasses::binToOneHot(*cp, m_ctx.m_binToOneHotContext);
            endOfStage("binToOneHot", dfg, comps);
            প্রতি (auto& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            endOfStage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            যদি (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            প্রতি (auto& cp : comps) V3DfgPasses::ঠেলো_Down_Sels(*cp, m_ctx.m_ঠেলো_Down_Sels_Context);
            endOfStage("pushDownSels", dfg, comps);
            প্রতি (auto& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
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

    শূন্য removeNeverActives(AstNetlist* netlistp) {
        std::vector<AstActive*> neverActiveps;
        netlistp->foreach([&](AstActive* activep) {
            Ast_Sen_বৃক্ষ* ধ্রুব senTreep = activep->sentreep();
            যদি (!senTreep) প্রত্যাগম;
            ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            যদি (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_back(activep);
            }
        });
        প্রতি (AstActive* ধ্রুব activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlinkFrBack()->delete_বৃক্ষ(), activep);
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
        ধ্রুব std::unique_ptr<DfgGraph> dfgp = V3DfgPasses::astToDfg(*netlistp, m_ctx);
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

সার্বজনিক:
    স্থির শূন্য apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

শূন্য V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_বৃক্ষ("dfg-optimize", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
