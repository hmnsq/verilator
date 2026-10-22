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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3ActiveTop.h"

#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

வகுப்பு Active_உச்சி_Visitor final : பொது VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_மரம்_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    நிலையான தருக்க isInitial(Ast_முனை* nodep) {
        மாறிலி VNUser1InUse user1InUse;
        // Return true if no variables that read.
        திரும்பு nodep->forall([&](மாறிலி AstVarRef* refp) -> தருக்க {
            AstVarScope* மாறிலி vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            எனில் (refp->access().isWriteOnly()) {
                vscp->user1(உண்மை);
                திரும்பு உண்மை;
            }
            // Read or ReadWrite: OK if written before
            திரும்பு vscp->user1();
        });
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    வெறுமை visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_மரம்* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        எனில் (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }

        // Delete empty procedures
        க்கு (Ast_முனை *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            எனில் (Ast_முனை_Procedure* மாறிலி procp = VN_CAST(stmtp, முனை_Procedure)) {
                எனில் (!procp->stmtsp()) VL_DO_DANGLING(தள்ளு_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        எனில் (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_மரம்(), nodep);
            திரும்பு;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_மரம்* மாறிலி wantp = m_finder.get_Sen_மரம்(sentreep);
        UINFO(4, "   lookdone");
        எனில் (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            எனில் (nodep->sen_மரம்_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_மரம்_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(தள்ளு_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        எனில் (nodep->sentreep()->hasCombo()) {
            File_வரி* மாறிலி flp = nodep->fileline();
            AstActive* initialp = nullptr;
            க்கு (Ast_முனை *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                எனில் (!isInitial(logicp)) தொடர்;
                எனில் (!initialp) initialp = புதிய AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            எனில் (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    வெறுமை visit(Ast_முனை_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    வெறுமை visit(AstVarScope*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit Active_உச்சி_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_உச்சி_Visitor() override = இயல்பு;
};

//######################################################################
// Active class functions

வெறுமை V3_Active_உச்சி::active_உச்சி_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_உச்சி_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("activetop", 0, dump_மரம்_Either_Level() >= 3);
}
