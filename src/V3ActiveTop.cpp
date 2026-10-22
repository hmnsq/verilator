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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ActiveTop.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

ਵਰਗ ਸਰਗਰਮ_ਸਿਖਰ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_ਰੁੱਖ_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    ਸਥਿਰ ਤਾਰਕਿਕ isInitial(Ast_ਗੰਢ* nodep) {
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        // Return true if no variables that read.
        ਵਾਪਸ nodep->forall([&](ਅਚਲ AstVarRef* refp) -> ਤਾਰਕਿਕ {
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            ਜੇ (refp->access().isWriteOnly()) {
                vscp->user1(ਸੱਚ);
                ਵਾਪਸ ਸੱਚ;
            }
            // Read or ReadWrite: OK if written before
            ਵਾਪਸ vscp->user1();
        });
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_ਰੁੱਖ* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        ਜੇ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        // Delete empty procedures
        ਲਈ (Ast_ਗੰਢ *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ਜੇ (Ast_ਗੰਢ_Procedure* ਅਚਲ procp = VN_CAST(stmtp, ਗੰਢ_Procedure)) {
                ਜੇ (!procp->stmtsp()) VL_DO_DANGLING(ਧੱਕੋ_Deletep(procp->unlink_Fr_ਪਿੱਠ()), stmtp);
            }
        }

        // Delete empty actives
        ਜੇ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_ਰੁੱਖ* ਅਚਲ wantp = m_finder.get_Sen_ਰੁੱਖ(sentreep);
        UINFO(4, "   lookdone");
        ਜੇ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            ਜੇ (nodep->sen_ਰੁੱਖ_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_ਰੁੱਖ_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlink_Fr_ਪਿੱਠ();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        ਜੇ (nodep->sentreep()->hasCombo()) {
            File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
            Ast_ਸਰਗਰਮ* initialp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                ਜੇ (!isInitial(logicp)) ਜਾਰੀ;
                ਜੇ (!initialp) initialp = ਨਵਾਂ Ast_ਸਰਗਰਮ{flp, "", m_finder.getInitial()};
                initialp->ਜੋੜੋ_Stmtsp(logicp->unlink_Fr_ਪਿੱਠ());
            }
            ਜੇ (initialp) nodep->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(initialp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(AstVarScope*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਸਰਗਰਮ_ਸਿਖਰ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਸਰਗਰਮ_ਸਿਖਰ_Visitor() override = ਮੂਲ;
};

//######################################################################
// Active class functions

ਖਾਲੀ V3_ਸਰਗਰਮ_ਸਿਖਰ::ਸਰਗਰਮ_ਸਿਖਰ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਸਰਗਰਮ_ਸਿਖਰ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("activetop", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
