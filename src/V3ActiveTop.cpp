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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3ActiveTop.h"

#समावेश "V3Const.h"
#समावेश "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

श्रेणी Active_शीर्ष_Visitor final : खुला VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_वृक्ष_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    अटल bool isInitial(Ast_गाँठ* nodep) {
        स्थिर VNUser1InUse user1InUse;
        // Return true if no variables that read.
        वापस nodep->forall([&](स्थिर AstVarRef* refp) -> bool {
            AstVarScope* स्थिर vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            अगर (refp->access().isWriteOnly()) {
                vscp->user1(true);
                वापस true;
            }
            // Read or ReadWrite: OK if written before
            वापस vscp->user1();
        });
    }

    // VISITORS
    व्योम visit(Ast_गाँठ_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    व्योम visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_वृक्ष* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        अगर (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }

        // Delete empty procedures
        क्रम (Ast_गाँठ *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            अगर (Ast_गाँठ_Procedure* स्थिर procp = VN_CAST(stmtp, गाँठ_Procedure)) {
                अगर (!procp->stmtsp()) VL_DO_DANGLING(धकेलो_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        अगर (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            वापस;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_वृक्ष* स्थिर wantp = m_finder.get_Sen_वृक्ष(sentreep);
        UINFO(4, "   lookdone");
        अगर (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            अगर (nodep->sen_वृक्ष_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_वृक्ष_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(धकेलो_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        अगर (nodep->sentreep()->hasCombo()) {
            File_पंक्ति* स्थिर flp = nodep->fileline();
            AstActive* initialp = nullptr;
            क्रम (Ast_गाँठ *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                अगर (!isInitial(logicp)) जारी;
                अगर (!initialp) initialp = नया AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            अगर (initialp) nodep->add_Here_This_As_अग्र(initialp);
        }
    }
    व्योम visit(Ast_गाँठ_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    व्योम visit(AstVarScope*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit Active_शीर्ष_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_शीर्ष_Visitor() override = शेष;
};

//######################################################################
// Active class functions

व्योम V3_Active_शीर्ष::active_शीर्ष_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_शीर्ष_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("activetop", 0, dump_वृक्ष_Either_Level() >= 3);
}
