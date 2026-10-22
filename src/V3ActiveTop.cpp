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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3ActiveTop.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

ଶ୍ରେଣୀ Active_ଶୀର୍ଷ_Visitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_ବୃକ୍ଷ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ସ୍ଥିର ତାର୍କିକ isInitial(Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ VNUser1InUse user1InUse;
        // Return true if no variables that read.
        ଫେରନ୍ତୁ nodep->forall([&](ଅଚଳ AstVarRef* refp) -> ତାର୍କିକ {
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            ଯଦି (refp->access().isWriteOnly()) {
                vscp->user1(ସତ୍ୟ);
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
            // Read or ReadWrite: OK if written before
            ଫେରନ୍ତୁ vscp->user1();
        });
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_ବୃକ୍ଷ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        ଯଦି (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        // Delete empty procedures
        ପାଇଁ (Ast_ଗଣ୍ଠି *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ଯଦି (Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ procp = VN_CAST(stmtp, ଗଣ୍ଠି_Procedure)) {
                ଯଦି (!procp->stmtsp()) VL_DO_DANGLING(ଠେଲ_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        ଯଦି (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ବୃକ୍ଷ(), nodep);
            ଫେରନ୍ତୁ;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_ବୃକ୍ଷ* ଅଚଳ wantp = m_finder.get_Sen_ବୃକ୍ଷ(sentreep);
        UINFO(4, "   lookdone");
        ଯଦି (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            ଯଦି (nodep->sen_ବୃକ୍ଷ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_ବୃକ୍ଷ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ଠେଲ_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        ଯଦି (nodep->sentreep()->hasCombo()) {
            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            AstActive* initialp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                ଯଦି (!isInitial(logicp)) ଜାରି;
                ଯଦି (!initialp) initialp = ନୂତନ AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            ଯଦି (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(AstVarScope*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit Active_ଶୀର୍ଷ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_ଶୀର୍ଷ_Visitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Active class functions

ଶୂନ୍ୟ V3_Active_ଶୀର୍ଷ::active_ଶୀର୍ଷ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_ଶୀର୍ଷ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("activetop", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
