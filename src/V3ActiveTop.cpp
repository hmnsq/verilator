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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3ActiveTop.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

ವರ್ಗ Active_ತುದಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_ವೃಕ್ಷ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isInitial(Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ VNUser1InUse user1InUse;
        // Return true if no variables that read.
        ಪ್ರತ್ಯಾಗಮ nodep->forall([&](ಧ್ರುವ AstVarRef* refp) -> ತಾರ್ಕಿಕ {
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            ಯದಿ (refp->access().isWriteOnly()) {
                vscp->user1(ಸತ್ಯ);
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            // Read or ReadWrite: OK if written before
            ಪ್ರತ್ಯಾಗಮ vscp->user1();
        });
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        ಯದಿ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Delete empty procedures
        ಪ್ರತಿ (Ast_ಗಂಟು *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ಯದಿ (Ast_ಗಂಟು_Procedure* ಧ್ರುವ procp = VN_CAST(stmtp, ಗಂಟು_Procedure)) {
                ಯದಿ (!procp->stmtsp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        ಯದಿ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_ವೃಕ್ಷ* ಧ್ರುವ wantp = m_finder.get_Sen_ವೃಕ್ಷ(sentreep);
        UINFO(4, "   lookdone");
        ಯದಿ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            ಯದಿ (nodep->sen_ವೃಕ್ಷ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_ವೃಕ್ಷ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        ಯದಿ (nodep->sentreep()->hasCombo()) {
            File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
            AstActive* initialp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                ಯದಿ (!isInitial(logicp)) ಅನುವರ್ತ;
                ಯದಿ (!initialp) initialp = ನೂತನ AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            ಯದಿ (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ visit(AstVarScope*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit Active_ತುದಿ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_ತುದಿ_Visitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Active class functions

ಶೂನ್ಯ V3_Active_ತುದಿ::active_ತುದಿ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_ತುದಿ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("activetop", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
