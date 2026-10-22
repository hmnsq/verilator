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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3ActiveTop.h"

#شامل "V3Const.h"
#شامل "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

رده Active_بالاترین_Visitor final : عمومی VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_درخت_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ایستا منطقی isInitial(Ast_گره* nodep) {
        ثابت VNUser1InUse user1InUse;
        // Return true if no variables that read.
        بازگشت nodep->forall([&](ثابت AstVarRef* refp) -> منطقی {
            AstVarScope* ثابت vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            اگر (refp->access().isWriteOnly()) {
                vscp->user1(درست);
                بازگشت درست;
            }
            // Read or ReadWrite: OK if written before
            بازگشت vscp->user1();
        });
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    تهی visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        اگر (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }

        // Delete empty procedures
        برای (Ast_گره *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            اگر (Ast_گره_Procedure* ثابت procp = VN_CAST(stmtp, گره_Procedure)) {
                اگر (!procp->stmtsp()) VL_DO_DANGLING(افزودن_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        اگر (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_درخت(), nodep);
            بازگشت;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_درخت* ثابت wantp = m_finder.get_Sen_درخت(sentreep);
        UINFO(4, "   lookdone");
        اگر (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            اگر (nodep->sen_درخت_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_درخت_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(افزودن_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        اگر (nodep->sentreep()->hasCombo()) {
            File_سطر* ثابت flp = nodep->fileline();
            AstActive* initialp = nullptr;
            برای (Ast_گره *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                اگر (!isInitial(logicp)) ادامه;
                اگر (!initialp) initialp = جدید AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            اگر (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    تهی visit(Ast_گره_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(AstVarScope*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit Active_بالاترین_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_بالاترین_Visitor() override = پیشفرض;
};

//######################################################################
// Active class functions

تهی V3_Active_بالاترین::active_بالاترین_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_بالاترین_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("activetop", 0, dump_درخت_Either_Level() >= 3);
}
