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

ವರ್ಗ ಸಕ್ರಿಯ_ತುದಿ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_ವೃಕ್ಷ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isInitial(Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        // Return true if no variables that read.
        ಹಿಂತಿರುಗಿ nodep->forall([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) -> ತಾರ್ಕಿಕ {
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            ಆದರೆ (refp->access().isWriteOnly()) {
                vscp->user1(ನಿಜ);
                ಹಿಂತಿರುಗಿ ನಿಜ;
            }
            // Read or ReadWrite: OK if written before
            ಹಿಂತಿರುಗಿ vscp->user1();
        });
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_ವೃಕ್ಷ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        ಆದರೆ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Delete empty procedures
        ಗಾಗಿ (Ast_ಗಂಟು *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ಆದರೆ (Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ procp = VN_CAST(stmtp, ಗಂಟು_Procedure)) {
                ಆದರೆ (!procp->stmtsp()) VL_DO_DANGLING(ತಳ್ಳು_Deletep(procp->unlink_Fr_ಹಿಂಭಾಗ()), stmtp);
            }
        }

        // Delete empty actives
        ಆದರೆ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ wantp = m_finder.get_Sen_ವೃಕ್ಷ(sentreep);
        UINFO(4, "   lookdone");
        ಆದರೆ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            ಆದರೆ (nodep->sen_ವೃಕ್ಷ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_ವೃಕ್ಷ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlink_Fr_ಹಿಂಭಾಗ();
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
        ಆದರೆ (nodep->sentreep()->hasCombo()) {
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
            Ast_ಸಕ್ರಿಯ* initialp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                ಆದರೆ (!isInitial(logicp)) ಮುಂದುವರಿ;
                ಆದರೆ (!initialp) initialp = ಹೊಸ Ast_ಸಕ್ರಿಯ{flp, "", m_finder.getInitial()};
                initialp->ಕೂಡಿಸು_Stmtsp(logicp->unlink_Fr_ಹಿಂಭಾಗ());
            }
            ಆದರೆ (initialp) nodep->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(initialp);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಸಕ್ರಿಯ_ತುದಿ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ಸಕ್ರಿಯ_ತುದಿ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Active class functions

ಶೂನ್ಯ V3_ಸಕ್ರಿಯ_ತುದಿ::ಸಕ್ರಿಯ_ತುದಿ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಸಕ್ರಿಯ_ತುದಿ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("activetop", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
