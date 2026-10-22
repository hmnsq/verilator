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

جماعت DataflowOptimize final {
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
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    خالی انتہا_کا_Stage(ثابت std::string& name) {
        اگر (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    خالی انتہا_کا_Stage(ثابت std::string& name, ثابت Dfg_گراف& dfg,
                    ثابت std::سمتیہ<std::unique_ptr<Dfg_گراف>>& componentps) {
        // Dump the graphs for debugging
        اگر (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            اگر (dfg.سائز() > 0) dfg.dumpDotFilePrefixed(name);
            برائے (ثابت std::unique_ptr<Dfg_گراف>& componentp : componentps) {
                اگر (componentp->سائز() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        اگر (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            برائے (ثابت std::unique_ptr<Dfg_گراف>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        انتہا_کا_Stage(name);
    }

    // Mark variables with external references
    خالی markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_گرہ* nodep) {
            // Check variable flags
            اگر (AstVarScope* ثابت vscp = VN_CAST(nodep, VarScope)) {
                ثابت AstVar* ثابت varp = vscp->varp();
                // Force and trace have already been processed
                ثابت منطقی hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->is_Sig_User_Rd_عوامی()  // Readable by user
                    || varp->const_Pool_اندراج()  // Held in V3ConstPool lookup cache, keep
                    ;
                ثابت منطقی hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                اگر (hasExtRd) Dfg_راس_Var::سیٹ_Has_Ext_Rd_Refs(vscp);
                اگر (hasExtWr) Dfg_راس_Var::سیٹ_Has_Ext_Wr_Refs(vscp);
                واپس;
            }
            // Check references
            اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                اگر (refp->access().isRW()) Dfg_راس_Var::سیٹ_Has_RWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                واپس;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            اگر (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(سچ || !cellp->pinsp(), cellp, "Pins should have been lowered");
                واپس;
            }
        });
    }

    خالی optimize(Dfg_گراف& dfg) {
        // Remove unobservable variabels and logic that drives only such variables
        V3DfgPasses::removeUnobservable(dfg, m_ctx);
        انتہا_کا_Stage("removeUnobservable", dfg, {});

        // Synthesize DfgLogic vertices
        V3DfgPasses::synthesize(dfg, m_ctx);
        انتہا_کا_Stage("synthesize", dfg, {});

        // Extract the cyclic sub-graphs, so breakCycles can operate on small graphs,
        // make all of them acyclic, then merge them back to the main graph
        {
            std::سمتیہ<std::unique_ptr<Dfg_گراف>> comps = dfg.نکالو_Cyclic_Components("cyclic");
            برائے (ثابت خودکار& cp : comps) V3DfgPasses::break_ادوار(*cp, m_ctx);
            dfg.ملاؤ_Graphs(std::move(comps));
            انتہا_کا_Stage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::سمتیہ<std::unique_ptr<Dfg_گراف>> comps = dfg.بانٹو_Into_Components("acyclic");
        UASSERT(dfg.سائز() == 0, "DfgGraph should have become empty");
        انتہا_کا_Stage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            برائے (خودکار& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            انتہا_کا_Stage("removeSelects", dfg, comps);
            برائے (ثابت خودکار& cp : comps) V3DfgPasses::inlineVars(*cp);
            انتہا_کا_Stage("inlineVars", dfg, comps);
            برائے (خودکار& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            انتہا_کا_Stage("cse0", dfg, comps);
            برائے (خودکار& cp : comps) V3DfgPasses::bin_مقصد_One_Hot(*cp, m_ctx.m_bin_مقصد_One_Hot_Context);
            انتہا_کا_Stage("binToOneHot", dfg, comps);
            برائے (خودکار& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            انتہا_کا_Stage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            اگر (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            برائے (خودکار& cp : comps) V3DfgPasses::ڈالو_نیچے_کو_Sels(*cp, m_ctx.m_ڈالو_نیچے_کو_Sels_Context);
            انتہا_کا_Stage("pushDownSels", dfg, comps);
            برائے (خودکار& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
            انتہا_کا_Stage("cse1", dfg, comps);
        }

        // Merge everything back under the main graph
        dfg.ملاؤ_Graphs(std::move(comps));
        انتہا_کا_Stage("optimized", dfg, {});

        // Regularize the graph after merging it all back together so all
        // references are known and we only need to iterate the Ast once
        // to replace redundant variables.
        V3DfgPasses::regularize(dfg, m_ctx.m_regularizeContext);
        انتہا_کا_Stage("regularize", dfg, {});
    }

    خالی removeNeverActives(AstNetlist* netlistp) {
        std::سمتیہ<Ast_فعال*> neverActiveps;
        netlistp->foreach([&](Ast_فعال* activep) {
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            اگر (!senTreep) واپس;
            ثابت Ast_گرہ* ثابت nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            اگر (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_عقب(activep);
            }
        });
        برائے (Ast_فعال* ثابت activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlink_Fr_عقب()->delete_درخت(), activep);
        }
    }

    DataflowOptimize(AstNetlist* netlistp) {
        // Mark variables with external references
        markExternallyReferencedVariables(netlistp);
        // Dump stage stats
        انتہا_کا_Stage("init");
        // Post V3Scope application. Run on whole netlist.
        UINFO(4, "Applying DFG optimization to entire netlist");
        // Build the DFG of the entire netlist
        ثابت std::unique_ptr<Dfg_گراف> dfgp = V3DfgPasses::ast_مقصد_Dfg(*netlistp, m_ctx);
        انتہا_کا_Stage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfg_مقصد_Ast(*dfgp, m_ctx);
        انتہا_کا_Stage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        Dfg_ڈیٹا_Type::reset();
        // Dump stage stats
        انتہا_کا_Stage("fini");
    }

عوامی:
    ساکن خالی apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

خالی V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_درخت("dfg-optimize", 0, dump_درخت_Either_Level() >= 3);
}
