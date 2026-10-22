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

جماعت فعال_چوٹی_Visitor final : عوامی VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_درخت_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ساکن منطقی isInitial(Ast_گرہ* nodep) {
        ثابت VNUser1_اندر_Use user1_اندر_Use;
        // Return true if no variables that read.
        واپس nodep->forall([&](ثابت AstVarRef* refp) -> منطقی {
            AstVarScope* ثابت vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            اگر (refp->access().isWriteOnly()) {
                vscp->user1(سچ);
                واپس سچ;
            }
            // Read or ReadWrite: OK if written before
            واپس vscp->user1();
        });
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_فعال* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_درخت* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        اگر (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
        }

        // Delete empty procedures
        برائے (Ast_گرہ *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            اگر (Ast_گرہ_Procedure* ثابت procp = VN_CAST(stmtp, گرہ_Procedure)) {
                اگر (!procp->stmtsp()) VL_DO_DANGLING(ڈالو_Deletep(procp->unlink_Fr_عقب()), stmtp);
            }
        }

        // Delete empty actives
        اگر (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlink_Fr_عقب()->delete_درخت(), nodep);
            واپس;
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
                sentreep->unlink_Fr_عقب();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ڈالو_Deletep(sentreep), sentreep);
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
            Ast_فعال* initialp = خالی_اشاریہ;
            برائے (Ast_گرہ *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                اگر (!isInitial(logicp)) جاری;
                اگر (!initialp) initialp = نیا Ast_فعال{flp, "", m_finder.getInitial()};
                initialp->جوڑو_Stmtsp(logicp->unlink_Fr_عقب());
            }
            اگر (initialp) nodep->جوڑو_یہاں_This_As_اگلا(initialp);
        }
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    خالی دورہ(AstVarScope*) override {}  // Accelerate
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح فعال_چوٹی_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~فعال_چوٹی_Visitor() override = طےشدہ;
};

//######################################################################
// Active class functions

خالی V3_فعال_چوٹی::فعال_چوٹی_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { فعال_چوٹی_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("activetop", 0, dump_درخت_Either_Level() >= 3);
}
