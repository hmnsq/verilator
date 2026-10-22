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

صنف ActiveTopVisitor final : عام VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    SenTreeFinder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ساكن منطقي isInitial(AstNode* nodep) {
        ثابت VNUser1InUse user1InUse;
        // Return true if no variables that read.
        ܦܢܐ nodep->forall([&](ثابت AstVarRef* refp) -> منطقي {
            AstVarScope* ثابت vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            ܐܢ (refp->access().isWriteOnly()) {
                vscp->user1(ܫܪܪܐ);
                ܦܢܐ ܫܪܪܐ;
            }
            // Read or ReadWrite: OK if written before
            ܦܢܐ vscp->user1();
        });
    }

    // VISITORS
    فراغ visit(AstNodeModule* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    فراغ visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        AstSenTree* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        ܐܢ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            ܦܢܐ;
        }

        // Delete empty procedures
        لكل (AstNode *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ܐܢ (AstNodeProcedure* ثابت procp = VN_CAST(stmtp, NodeProcedure)) {
                ܐܢ (!procp->stmtsp()) VL_DO_DANGLING(pushDeletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        ܐܢ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->deleteTree(), nodep);
            ܦܢܐ;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        AstSenTree* ثابت wantp = m_finder.getSenTree(sentreep);
        UINFO(4, "   lookdone");
        ܐܢ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            ܐܢ (nodep->senTreeStorep()) {
                UASSERT_OBJ(sentreep == nodep->senTreeStorep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(pushDeletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        ܐܢ (nodep->sentreep()->hasCombo()) {
            FileLine* ثابت flp = nodep->fileline();
            AstActive* initialp = nullptr;
            لكل (AstNode *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                ܐܢ (!isInitial(logicp)) استمر;
                ܐܢ (!initialp) initialp = جديد AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            ܐܢ (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    فراغ visit(AstNodeProcedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    فراغ visit(AstNodeExpr*) override {}  // Accelerate
    فراغ visit(AstVarScope*) override {}  // Accelerate
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ActiveTopVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~ActiveTopVisitor() override = افتراضي;
};

//######################################################################
// Active class functions

فراغ V3ActiveTop::activeTopAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ActiveTopVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("activetop", 0, dumpTreeEitherLevel() >= 3);
}
