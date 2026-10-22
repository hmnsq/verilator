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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3ActiveTop.h"

#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

વર્ગ Active_ટોચ_Visitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_વૃક્ષ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    સ્થિર તાર્કિક isInitial(Ast_ગાંઠ* nodep) {
        અચલ VNUser1InUse user1InUse;
        // Return true if no variables that read.
        પાછા nodep->forall([&](અચલ AstVarRef* refp) -> તાર્કિક {
            AstVarScope* અચલ vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            જો (refp->access().isWriteOnly()) {
                vscp->user1(સાચું);
                પાછા સાચું;
            }
            // Read or ReadWrite: OK if written before
            પાછા vscp->user1();
        });
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_વૃક્ષ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        જો (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }

        // Delete empty procedures
        માટે (Ast_ગાંઠ *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            જો (Ast_ગાંઠ_Procedure* અચલ procp = VN_CAST(stmtp, ગાંઠ_Procedure)) {
                જો (!procp->stmtsp()) VL_DO_DANGLING(ધકેલો_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        જો (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_વૃક્ષ(), nodep);
            પાછા;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_વૃક્ષ* અચલ wantp = m_finder.get_Sen_વૃક્ષ(sentreep);
        UINFO(4, "   lookdone");
        જો (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            જો (nodep->sen_વૃક્ષ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_વૃક્ષ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ધકેલો_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        જો (nodep->sentreep()->hasCombo()) {
            File_લીટી* અચલ flp = nodep->fileline();
            AstActive* initialp = nullptr;
            માટે (Ast_ગાંઠ *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                જો (!isInitial(logicp)) ચાલુ;
                જો (!initialp) initialp = નવું AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            જો (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    શૂન્ય visit(AstVarScope*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit Active_ટોચ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_ટોચ_Visitor() override = મૂળભૂત;
};

//######################################################################
// Active class functions

શૂન્ય V3_Active_ટોચ::active_ટોચ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_ટોચ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("activetop", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
