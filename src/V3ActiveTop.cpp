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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3ActiveTop.h"

#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

वर्ग Active_शिखरम्_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_वृक्षः_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    स्थिर तार्किक isInitial(Ast_ग्रन्थिः* nodep) {
        ध्रुव VNUser1InUse user1InUse;
        // Return true if no variables that read.
        प्रत्यागम nodep->forall([&](ध्रुव AstVarRef* refp) -> तार्किक {
            AstVarScope* ध्रुव vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            जइ (refp->access().isWriteOnly()) {
                vscp->user1(सच्च);
                प्रत्यागम सच्च;
            }
            // Read or ReadWrite: OK if written before
            प्रत्यागम vscp->user1();
        });
    }

    // VISITORS
    शून्य visit(Ast_ग्रन्थिः_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    शून्य visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_वृक्षः* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        जइ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        // Delete empty procedures
        प्रति (Ast_ग्रन्थिः *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            जइ (Ast_ग्रन्थिः_Procedure* ध्रुव procp = VN_CAST(stmtp, ग्रन्थिः_Procedure)) {
                जइ (!procp->stmtsp()) VL_DO_DANGLING(प्रक्षिप_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        जइ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_वृक्षः* ध्रुव wantp = m_finder.get_Sen_वृक्षः(sentreep);
        UINFO(4, "   lookdone");
        जइ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            जइ (nodep->sen_वृक्षः_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_वृक्षः_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(प्रक्षिप_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        जइ (nodep->sentreep()->hasCombo()) {
            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            AstActive* initialp = nullptr;
            प्रति (Ast_ग्रन्थिः *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                जइ (!isInitial(logicp)) अनुवर्त;
                जइ (!initialp) initialp = नूतन AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            जइ (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    शून्य visit(Ast_ग्रन्थिः_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    शून्य visit(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    शून्य visit(AstVarScope*) override {}  // Accelerate
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit Active_शिखरम्_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_शिखरम्_Visitor() override = स्वभाव;
};

//######################################################################
// Active class functions

शून्य V3_Active_शिखरम्::active_शिखरम्_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_शिखरम्_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("activetop", 0, dump_वृक्षः_Either_Level() >= 3);
}
