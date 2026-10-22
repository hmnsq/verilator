// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity active domains
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
// V3Active's Transformations:
//
// Note this can be called multiple times.
//   Across all ACTIVES
//      SenTrees are now under each ACTIVE statement, we want them global:
//          Find SenTree in under global TopScope, or create it there
//          Move SenTree the global SenTree
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3ActiveTop.h"

#ضمن "V3Const.h"
#ضمن "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

صنف سرگرم_چوٽي_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_وڻ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ساكن منطقي isInitial(Ast_ڳنڍ* nodep) {
        ثابت VNUser1_اندريون_Use user1_اندريون_Use;
        // Return true if no variables that read.
        أرجع nodep->forall([&](ثابت AstVarRef* refp) -> منطقي {
            AstVarScope* ثابت vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            إذا (refp->access().isWriteOnly()) {
                vscp->user1(سچ);
                أرجع سچ;
            }
            // Read or ReadWrite: OK if written before
            أرجع vscp->user1();
        });
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_سرگرم* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_وڻ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        إذا (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }

        // Delete empty procedures
        لكل (Ast_ڳنڍ *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (Ast_ڳنڍ_Procedure* ثابت procp = VN_CAST(stmtp, ڳنڍ_Procedure)) {
                إذا (!procp->stmtsp()) VL_DO_DANGLING(ڌڪو_Deletep(procp->unlink_Fr_پٺ()), stmtp);
            }
        }

        // Delete empty actives
        إذا (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlink_Fr_پٺ()->delete_وڻ(), nodep);
            أرجع;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_وڻ* ثابت wantp = m_finder.get_Sen_وڻ(sentreep);
        UINFO(4, "   lookdone");
        إذا (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            إذا (nodep->sen_وڻ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_وڻ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlink_Fr_پٺ();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ڌڪو_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        إذا (nodep->sentreep()->hasCombo()) {
            File_سٽ* ثابت flp = nodep->fileline();
            Ast_سرگرم* initialp = خالي_اشارو;
            لكل (Ast_ڳنڍ *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                إذا (!isInitial(logicp)) استمر;
                إذا (!initialp) initialp = جديد Ast_سرگرم{flp, "", m_finder.getInitial()};
                initialp->جوڙيو_Stmtsp(logicp->unlink_Fr_پٺ());
            }
            إذا (initialp) nodep->جوڙيو_هتي_This_As_ايندڙ(initialp);
        }
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}  // Accelerate
    فراغ گهمو(AstVarScope*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح سرگرم_چوٽي_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~سرگرم_چوٽي_Visitor() override = افتراضي;
};

//######################################################################
// Active class functions

فراغ V3_سرگرم_چوٽي::سرگرم_چوٽي_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { سرگرم_چوٽي_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("activetop", 0, dump_وڻ_Either_Level() >= 3);
}
