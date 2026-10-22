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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3ActiveTop.h"

#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3SenTree.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Active class functions

తరగతి చురుకైన_శిఖరం_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE
    // AstVarScope::user1()  // bool.  Processed

    // STATE
    Sen_వృక్షం_Finder m_finder;  // Find global sentree's / add them under the AstTopScope

    // METHODS
    స్థిర తార్కిక isInitial(Ast_కణుపు* nodep) {
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        // Return true if no variables that read.
        తిరిగి nodep->forall([&](స్థిరరాశి AstVarRef* refp) -> తార్కిక {
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            // Note: Use same heuristic as ordering does to ignore written variables
            // TODO: Use live variable analysis.
            ఒకవేళ (refp->access().isWriteOnly()) {
                vscp->user1(నిజం);
                తిరిగి నిజం;
            }
            // Read or ReadWrite: OK if written before
            తిరిగి vscp->user1();
        });
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        // Create required actives and add to module
        // We can start ordering at a module, or a scope
        UINFO(4, " MOD   " << nodep);
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        UINFO(4, "   ACTIVE " << nodep);
        // Remove duplicate clocks and such; sensesp() may change!
        V3Const::constifyExpensiveEdit(nodep);
        Ast_Sen_వృక్షం* sentreep = nodep->sentreep();
        UASSERT_OBJ(sentreep, nodep, "nullptr");
        ఒకవేళ (sentreep->sensesp() && sentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!sentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        // Delete empty procedures
        కొరకు (Ast_కణుపు *stmtp = nodep->stmtsp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            ఒకవేళ (Ast_కణుపు_Procedure* స్థిరరాశి procp = VN_CAST(stmtp, కణుపు_Procedure)) {
                ఒకవేళ (!procp->stmtsp()) VL_DO_DANGLING(నెట్టు_Deletep(procp->unlink_Fr_వెనుక()), stmtp);
            }
        }

        // Delete empty actives
        ఒకవేళ (!nodep->stmtsp()) {
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }

        // Move the SENTREE for each active up to the global level.
        // This way we'll easily see what clock domains are identical
        Ast_Sen_వృక్షం* స్థిరరాశి wantp = m_finder.get_Sen_వృక్షం(sentreep);
        UINFO(4, "   lookdone");
        ఒకవేళ (wantp != sentreep) {
            // Move the active's contents to the other active
            UINFO(4, "   merge active " << sentreep << " into " << wantp);
            ఒకవేళ (nodep->sen_వృక్షం_Storep()) {
                UASSERT_OBJ(sentreep == nodep->sen_వృక్షం_Storep(), nodep,
                            "sensesStore should have been deleted earlier if different");
                sentreep->unlink_Fr_వెనుక();
                // There may be other references to same sense tree,
                // we'll be removing all references when we get to them,
                // but don't dangle our pointer yet!
                VL_DO_DANGLING(నెట్టు_Deletep(sentreep), sentreep);
            }
            nodep->sentreep(wantp);
        }

        // If this is combinational logic that does not read any variables, then it really is an
        // initial block in disguise, so move such logic under an Initial AstActive, V3Order would
        // prune these otherwise.
        // TODO: we should warn for these if they were 'always @*' as some (including strictly
        //       compliant) simulators will never execute these.
        ఒకవేళ (nodep->sentreep()->hasCombo()) {
            File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
            Ast_చురుకైన* initialp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు *logicp = nodep->stmtsp(), *nextp; logicp; logicp = nextp) {
                nextp = logicp->nextp();
                ఒకవేళ (!isInitial(logicp)) కొనసాగు;
                ఒకవేళ (!initialp) initialp = కొత్త Ast_చురుకైన{flp, "", m_finder.getInitial()};
                initialp->కూడు_Stmtsp(logicp->unlink_Fr_వెనుక());
            }
            ఒకవేళ (initialp) nodep->కూడు_ఇక్కడ_This_As_తదుపరి(initialp);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {  // LCOV_EXCL_LINE
        nodep->v3fatalSrc("Node should have been under ACTIVE");
    }
    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    శూన్య సందర్శించు(AstVarScope*) override {}  // Accelerate
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన చురుకైన_శిఖరం_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~చురుకైన_శిఖరం_Visitor() override = అప్రమేయ;
};

//######################################################################
// Active class functions

శూన్య V3_చురుకైన_శిఖరం::చురుకైన_శిఖరం_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { చురుకైన_శిఖరం_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("activetop", 0, dump_వృక్షం_Either_Level() >= 3);
}
