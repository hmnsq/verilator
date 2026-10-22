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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3ActiveTop.h"

#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

শ্রেণী Active_শীর্ষ_Visitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_বৃক্ষ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    স্থির যৌক্তিক isInitial(Ast_গ্রন্থি* nodep) {
        ধ্রুবক VNUser1InUse user1InUse;
        // Return true if no variables that read.
        ফেরত nodep->forall([&](ধ্রুবক AstVarRef* refp) -> যৌক্তিক {
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            যদি (refp->access().isWriteOnly()) {
                vscp->user1(সত্য);
                ফেরত সত্য;
            }
            // Read or ReadWrite: OK if written before
            ফেরত vscp->user1();
        });
    }

    // VISITORS
    শূন্য visit(Ast_গ্রন্থি_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    শূন্য visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_বৃক্ষ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        যদি (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }

        // Delete empty procedures
        জন্য (Ast_গ্রন্থি *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            যদি (Ast_গ্রন্থি_Procedure* ধ্রুবক procp = VN_CAST(stmtp, গ্রন্থি_Procedure)) {
                যদি (!procp->stmtsp()) VL_DO_DANGLING(ঠেলো_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        যদি (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_বৃক্ষ(), nodep);
            ফেরত;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_বৃক্ষ* ধ্রুবক wantp = m_finder.get_Sen_বৃক্ষ(sentreep);
        UINFO(4, "   lookdone");
        যদি (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            যদি (nodep->sen_বৃক্ষ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_বৃক্ষ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ঠেলো_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        যদি (nodep->sentreep()->hasCombo()) {
            File_সারি* ধ্রুবক flp = nodep->fileline();
            AstActive* initialp = nullptr;
            জন্য (Ast_গ্রন্থি *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                যদি (!isInitial(logicp)) চালু;
                যদি (!initialp) initialp = নতুন AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            যদি (initialp) nodep->add_Here_This_As_পরবর্তী(initialp);
        }
    }
    শূন্য visit(Ast_গ্রন্থি_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(AstVarScope*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit Active_শীর্ষ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_শীর্ষ_Visitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Active class functions

শূন্য V3_Active_শীর্ষ::active_শীর্ষ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_শীর্ষ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("activetop", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
