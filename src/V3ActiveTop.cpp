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

صنف فعال_سر_Visitor final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_ونه_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ساكن منطقي isInitial(Ast_غوټه* nodep) {
        ثابت VNUser1_دننه_Use user1_دننه_Use;
        // Return true if no variables that read.
        أرجع nodep->forall([&](ثابت AstVarRef* refp) -> منطقي {
            AstVarScope* ثابت vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            إذا (refp->access().isWriteOnly()) {
                vscp->user1(ریښتیا);
                أرجع ریښتیا;
            }
            // Read or ReadWrite: OK if written before
            أرجع vscp->user1();
        });
    }

    // VISITORS
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    فراغ لیدنه(Ast_فعال* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_ونه* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        إذا (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }

        // Delete empty procedures
        لكل (Ast_غوټه *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            إذا (Ast_غوټه_Procedure* ثابت procp = VN_CAST(stmtp, غوټه_Procedure)) {
                إذا (!procp->stmtsp()) VL_DO_DANGLING(ټېل_Deletep(procp->unlink_Fr_شا()), stmtp);
            }
        }

        // Delete empty actives
        إذا (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_ونه* ثابت wantp = m_finder.get_Sen_ونه(sentreep);
        UINFO(4, "   lookdone");
        إذا (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            إذا (nodep->sen_ونه_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_ونه_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlink_Fr_شا();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ټېل_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        إذا (nodep->sentreep()->hasCombo()) {
            File_کرښه* ثابت flp = nodep->fileline();
            Ast_فعال* initialp = تش_اشاره;
            لكل (Ast_غوټه *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                إذا (!isInitial(logicp)) استمر;
                إذا (!initialp) initialp = جديد Ast_فعال{flp, "", m_finder.getInitial()};
                initialp->جمع_کړه_Stmtsp(logicp->unlink_Fr_شا());
            }
            إذا (initialp) nodep->جمع_کړه_دلته_This_As_بل(initialp);
        }
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    فراغ لیدنه(Ast_غوټه_بیان*) override {}  // Accelerate
    فراغ لیدنه(AstVarScope*) override {}  // Accelerate
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند فعال_سر_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~فعال_سر_Visitor() override = افتراضي;
};

//######################################################################
// Active class functions

فراغ V3_فعال_سر::فعال_سر_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { فعال_سر_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("activetop", 0, dump_ونه_Either_Level() >= 3);
}
