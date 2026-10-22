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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DfgOptimizer.h"

#ضمن "V3Const.h"
#ضمن "V3Dfg.h"
#ضمن "V3DfgPasses.h"
#ضمن "V3Error.h"

#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

صنف DataflowOptimize final {
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
    ثابت VNUser1_درون_Use m_user1_درون_Use;

    // STATE
    V3DfgContext m_ctx;  // The context holding values that need to persist across multiple graphs

    فراغ انتها_از_Stage(ثابت std::string& name) {
        إذا (VL_UNLIKELY(v3Global.opt.stats())) V3Stats::statsStage("dfg-" + name);
    }

    فراغ انتها_از_Stage(ثابت std::string& name, ثابت Dfg_گراف& dfg,
                    ثابت std::وکتور<std::unique_ptr<Dfg_گراف>>& componentps) {
        // Dump the graphs for debugging
        إذا (VL_UNLIKELY(dumpDfgLevel() >= 5)) {
            إذا (dfg.اندازه() > 0) dfg.dumpDotFilePrefixed(name);
            لكل (ثابت std::unique_ptr<Dfg_گراف>& componentp : componentps) {
                إذا (componentp->اندازه() > 0) componentp->dumpDotFilePrefixed(name);
            }
        }
        // Type check the graphs
        إذا (VL_UNLIKELY(v3Global.opt.debugCheck())) {
            V3DfgPasses::typeCheck(dfg);
            لكل (ثابت std::unique_ptr<Dfg_گراف>& componentp : componentps) {
                V3DfgPasses::typeCheck(*componentp);
            }
        }
        // Dump stage stats
        انتها_از_Stage(name);
    }

    // Mark variables with external references
    فراغ markExternallyReferencedVariables(AstNetlist* netlistp) {
        netlistp->foreach([](Ast_گره* nodep) {
            // Check variable flags
            إذا (AstVarScope* ثابت vscp = VN_CAST(nodep, VarScope)) {
                ثابت AstVar* ثابت varp = vscp->varp();
                // Force and trace have already been processed
                ثابت منطقي hasExtRd =  //
                    varp->isPrimaryIO()  // Top level port - readable
                    || varp->is_Sig_User_Rd_عمومی()  // Readable by user
                    || varp->const_Pool_مدخل()  // Held in V3ConstPool lookup cache, keep
                    ;
                ثابت منطقي hasExtWr =  //
                    (varp->isPrimaryIO() && varp->isNonOutput())  // Top level port - writable
                    || varp->isSigUserRWPublic()  // Writable by user
                    ;
                إذا (hasExtRd) Dfg_رأس_Var::مجموعه_Has_Ext_Rd_Refs(vscp);
                إذا (hasExtWr) Dfg_رأس_Var::مجموعه_Has_Ext_Wr_Refs(vscp);
                أرجع;
            }
            // Check references
            إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
                إذا (refp->access().isRW()) Dfg_رأس_Var::مجموعه_Has_RWRefs(refp->varScopep());
                UASSERT_OBJ(!refp->classOrPackagep(), refp, "V3Scope should have removed");
                أرجع;
            }
            UASSERT_OBJ(!VN_IS(nodep, VarXRef), nodep, "V3Scope should have removed");
            // Check cell ports
            إذا (ثابت AstCell* ثابت cellp = VN_CAST(nodep, Cell)) {
                // Why does this not hold?
                UASSERT_OBJ(درست || !cellp->pinsp(), cellp, "Pins should have been lowered");
                أرجع;
            }
        });
    }

    فراغ optimize(Dfg_گراف& dfg) {
        // Remove unobservable variabels and logic that drives only such variables
        V3DfgPasses::removeUnobservable(dfg, m_ctx);
        انتها_از_Stage("removeUnobservable", dfg, {});

        // Synthesize DfgLogic vertices
        V3DfgPasses::synthesize(dfg, m_ctx);
        انتها_از_Stage("synthesize", dfg, {});

        // Extract the cyclic sub-graphs, so breakCycles can operate on small graphs,
        // make all of them acyclic, then merge them back to the main graph
        {
            std::وکتور<std::unique_ptr<Dfg_گراف>> comps = dfg.استخراج_Cyclic_Components("cyclic");
            لكل (ثابت خودکار& cp : comps) V3DfgPasses::break_چرخهها(*cp, m_ctx);
            dfg.ادغام_Graphs(std::move(comps));
            انتها_از_Stage("breakCycles", dfg, {});
        }

        // Split the now entirely acyclic DFG into [weakly] connected components
        std::وکتور<std::unique_ptr<Dfg_گراف>> comps = dfg.تقسیم_Into_Components("acyclic");
        UASSERT(dfg.اندازه() == 0, "DfgGraph should have become empty");
        انتها_از_Stage("splitAcyclic", dfg, comps);

        // Main pass pipeline - optimize each acyclic component
        {
            لكل (خودکار& cp : comps) V3DfgPasses::removeSelects(*cp, m_ctx.m_removeSelectsContext);
            انتها_از_Stage("removeSelects", dfg, comps);
            لكل (ثابت خودکار& cp : comps) V3DfgPasses::inlineVars(*cp);
            انتها_از_Stage("inlineVars", dfg, comps);
            لكل (خودکار& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext0);
            انتها_از_Stage("cse0", dfg, comps);
            لكل (خودکار& cp : comps) V3DfgPasses::bin_مقصد_One_Hot(*cp, m_ctx.m_bin_مقصد_One_Hot_Context);
            انتها_از_Stage("binToOneHot", dfg, comps);
            لكل (خودکار& cp : comps) V3DfgPasses::peephole(*cp, m_ctx.m_peepholeContext);
            انتها_از_Stage("peephole", dfg, comps);

            // Accumulate patterns for reporting
            إذا (v3Global.opt.dumpDfgPatterns()) V3DfgPasses::dumpPatterns(comps);

            لكل (خودکار& cp : comps) V3DfgPasses::افزودن_پایین_Sels(*cp, m_ctx.m_افزودن_پایین_Sels_Context);
            انتها_از_Stage("pushDownSels", dfg, comps);
            لكل (خودکار& cp : comps) V3DfgPasses::cse(*cp, m_ctx.m_cseContext1);
            انتها_از_Stage("cse1", dfg, comps);
        }

        // Merge everything back under the main graph
        dfg.ادغام_Graphs(std::move(comps));
        انتها_از_Stage("optimized", dfg, {});

        // Regularize the graph after merging it all back together so all
        // references are known and we only need to iterate the Ast once
        // to replace redundant variables.
        V3DfgPasses::regularize(dfg, m_ctx.m_regularizeContext);
        انتها_از_Stage("regularize", dfg, {});
    }

    فراغ removeNeverActives(AstNetlist* netlistp) {
        std::وکتور<Ast_فعال*> neverActiveps;
        netlistp->foreach([&](Ast_فعال* activep) {
            Ast_Sen_درخت* ثابت senTreep = activep->sentreep();
            إذا (!senTreep) أرجع;
            ثابت Ast_گره* ثابت nodep = V3Const::constifyEdit(senTreep);
            UASSERT_OBJ(nodep == senTreep, nodep, "Should not have been repalced");
            إذا (senTreep->sensesp()->isNever()) {
                UASSERT_OBJ(!senTreep->sensesp()->nextp(), nodep,
                            "Never senitem should be alone, else the never should be eliminated.");
                neverActiveps.emplace_پشت(activep);
            }
        });
        لكل (Ast_فعال* ثابت activep : neverActiveps) {
            VL_DO_DANGLING(activep->unlink_Fr_پشت()->delete_درخت(), activep);
        }
    }

    DataflowOptimize(AstNetlist* netlistp) {
        // Mark variables with external references
        markExternallyReferencedVariables(netlistp);
        // Dump stage stats
        انتها_از_Stage("init");
        // Post V3Scope application. Run on whole netlist.
        UINFO(4, "Applying DFG optimization to entire netlist");
        // Build the DFG of the entire netlist
        ثابت std::unique_ptr<Dfg_گراف> dfgp = V3DfgPasses::ast_مقصد_Dfg(*netlistp, m_ctx);
        انتها_از_Stage("astToDfg", *dfgp, {});
        // Actually process the graph
        optimize(*dfgp);
        // Convert back to Ast
        V3DfgPasses::dfg_مقصد_Ast(*dfgp, m_ctx);
        انتها_از_Stage("dfgToAst", *dfgp, {});
        // Some sentrees might have become constant, remove them
        removeNeverActives(netlistp);
        // Reset interned types so the corresponding Ast types can be garbage collected
        Dfg_داده_Type::reset();
        // Dump stage stats
        انتها_از_Stage("fini");
    }

عام:
    ساكن فراغ apply(AstNetlist* netlistp) { DataflowOptimize{netlistp}; }
};

فراغ V3DfgOptimizer::optimize(AstNetlist* netlistp) {
    UINFO(2, __FUNCTION__ << ":");
    DataflowOptimize::apply(netlistp);
    V3Global::dump_Check_Global_درخت("dfg-optimize", 0, dump_درخت_Either_Level() >= 3);
}
