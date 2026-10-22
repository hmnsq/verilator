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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3ActiveTop.h"

#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

വർഗ്ഗം Active_മുകൾ_Visitor final : പൊതു VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_വൃക്ഷം_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    സ്ഥിതം തർക്കം isInitial(Ast_കണ്ണി* nodep) {
        സ്ഥിരം VNUser1InUse user1InUse;
        // Return true if no variables that read.
        തിരികെ nodep->forall([&](സ്ഥിരം AstVarRef* refp) -> തർക്കം {
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            എങ്കിൽ (refp->access().isWriteOnly()) {
                vscp->user1(സത്യം);
                തിരികെ സത്യം;
            }
            // Read or ReadWrite: OK if written before
            തിരികെ vscp->user1();
        });
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstActive* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_വൃക്ഷം* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        എങ്കിൽ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        // Delete empty procedures
        വേണ്ടി (Ast_കണ്ണി *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            എങ്കിൽ (Ast_കണ്ണി_Procedure* സ്ഥിരം procp = VN_CAST(stmtp, കണ്ണി_Procedure)) {
                എങ്കിൽ (!procp->stmtsp()) VL_DO_DANGLING(തള്ളുക_Deletep(procp->unlinkFrBack()), stmtp);
            }
        }

        // Delete empty actives
        എങ്കിൽ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_വൃക്ഷം* സ്ഥിരം wantp = m_finder.get_Sen_വൃക്ഷം(sentreep);
        UINFO(4, "   lookdone");
        എങ്കിൽ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            എങ്കിൽ (nodep->sen_വൃക്ഷം_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_വൃക്ഷം_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlinkFrBack();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(തള്ളുക_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        എങ്കിൽ (nodep->sentreep()->hasCombo()) {
            File_വരി* സ്ഥിരം flp = nodep->fileline();
            AstActive* initialp = nullptr;
            വേണ്ടി (Ast_കണ്ണി *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                എങ്കിൽ (!isInitial(logicp)) തുടരുക;
                എങ്കിൽ (!initialp) initialp = പുതിയ AstActive{flp, "", m_finder.getInitial()};
                initialp->addStmtsp(logicp->unlinkFrBack());
            }
            എങ്കിൽ (initialp) nodep->addHereThisAsNext(initialp);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    ശൂന്യം visit(AstVarScope*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit Active_മുകൾ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~Active_മുകൾ_Visitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Active class functions

ശൂന്യം V3_Active_മുകൾ::active_മുകൾ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { Active_മുകൾ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("activetop", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
