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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3DfgOptimizer.h"

#समावेश "V3Const.h"
#समावेश "V3Dfg.h"
#समावेश "V3DfgPasses.h"
#समावेश "V3Error.h"

#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

श्रेणी DataflowOptimize final {
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
    स्थिर VNUser1InUse m_user1InUse;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    व्योम endOfStage(स्थिर std::string& name) {
        अगर (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    व्योम endOfStage(स्थिर std::string& name, स्थिर DfgGraph& dfg,
                    स्थिर std::vector<std::unique_ptr<DfgGraph>>& componentps) {
        // Dump the graphs for debugging
        अगर (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            अगर (dfg.आकार() > 0) dfg.dumpDotFilePrefixed(name);
            क्रम (स्थिर std::unique_ptr<DfgGraph>& componentp : componentps) {
                अगर (componentp->आकार() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        अगर (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            क्रम (स्थिर std::unique_ptr<DfgGraph>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        endOfStage(name);
    }

    // Mark variables with external references
    व्योम markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_गाँठ* nodep) {
            // Check variable flags
            अगर (AstVarScope* स्थिर vscp = VN_CAST(nodep, VarScope)) {
                स्थिर AstVar* स्थिर varp = vscp->varp();
                // Force and trace have already been processed
                स्थिर bool hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->isSigUserRdPublic()  // Readable by user
                    || varp->constPoolEntry()  // Held in V3ConstPool lookup cache, keep
                    ;
                स्थिर bool hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                अगर (hasExtRd) DfgVertexVar::setHasExtRdRefs(vscp);
                अगर (hasExtWr) DfgVertexVar::setHasExtWrRefs(vscp);
                वापस;
            }
            // Check references
            अगर (स्थिर AstVarRef* स्थिर refp = VN_CAST(nodep, VarRef)) {
                अगर (refp->access().isRW()) DfgVertexVar::setHasRWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                वापस;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            अगर (स्थिर AstCell* स्थिर cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(true || !cellp->pinsp(), cellp, "Pins should have been lowered");
                वापस;
            }
        });
    }

    व्योम optimize(DfgGraph& dfg) {
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
            क्रम (स्थिर स्वतः& cp : comps) V3DfgPasses::breakCycles(*cp, m_ctx);
            dfg.mergeGraphs(std::move(comps));
            endOfStage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::vector<std::unique_ptr<DfgGraph>> comps = dfg.splitIntoComponents("acyclic");
        UASSERT(dfg.आकार() == 0, "DfgGraph should have become empty");
        endOfStage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            क्रम (स्वतः& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            endOfStage("removeSelects", dfg, comps);
            क्रम (स्थिर स्वतः& cp : comps) V3DfgPasses::inlineVars(*cp);
            endOfStage("inlineVars", dfg, comps);
            क्रम (स्वतः& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            endOfStage("cse0", dfg, comps);
            क्रम (स्वतः& cp : comps) V3DfgPasses::binToOneHot(*cp, m_ctx.m_binToOneHotContext);
            endOfStage("binToOneHot", dfg, comps);
            क्रम (स्वतः& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            endOfStage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            अगर (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            क्रम (स्वतः& cp : comps) V3DfgPasses::धकेलो_Down_Sels(*cp, m_ctx.m_धकेलो_Down_Sels_Context);
            endOfStage("pushDownSels", dfg, comps);
            क्रम (स्वतः& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
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

    व्योम removeNeverActives(AstNetlist* netlistp) {
        std::vector<AstActive*> neverActiveps;
        netlistp->foreach([&](AstActive* activep) {
            Ast_Sen_वृक्ष* स्थिर senTreep = activep->sentreep();
            अगर (!senTreep) वापस;
            स्थिर Ast_गाँठ* स्थिर nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            अगर (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_back(activep);
            }
        });
        क्रम (AstActive* स्थिर activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlinkFrBack()->delete_वृक्ष(), activep);
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
        स्थिर std::unique_ptr<DfgGraph> dfgp = V3DfgPasses::astToDfg(*netlistp, m_ctx);
        endOfStage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfgToAst(*dfgp, m_ctx);
        endOfStage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        Dfg_तथ्य_Type::reset();
        // Dump stage stats
        endOfStage("fini");
    }

खुला:
    अटल व्योम apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

व्योम V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_वृक्ष("dfg-optimize", 0, dump_वृक्ष_Either_Level() >= 3);
}
