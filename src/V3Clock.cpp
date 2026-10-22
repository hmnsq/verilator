// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Post scheduling transformations
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
// V3Clock's Transformations:
//
//  This pass is historic and does some arbitray post scheduling rewrites
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Clock.h"

#అన్తర్భావ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

తరగతి Convert_Write_Refs_గమ్యం_Read final : బహిరంగ VNVisitor {
    // MEMBERS
    Ast_కణుపు_సమాసం* m_ఫలితం = శూన్యసూచిక;

    // CONSTRUCTORS
    స్పష్టమైన Convert_Write_Refs_గమ్యం_Read(Ast_కణుపు_సమాసం* nodep) {
        m_ఫలితం = VN_AS(iterateSubtreeReturnEdits(nodep), కణుపు_సమాసం);
    }

    // VISITORS
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        ఒకవేళ (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                కొత్త AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    స్థిర Ast_కణుపు_సమాసం* ప్రధాన(Ast_కణుపు_సమాసం* nodep) { తిరిగి Convert_Write_Refs_గమ్యం_Read{nodep}.m_ఫలితం; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

తరగతి ClockVisitor final : బహిరంగ VNVisitor {
    // VISITORS
    శూన్య సందర్శించు(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* స్థిరరాశి incp = nodep->incp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి origp = nodep->origp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి changeWrp = nodep->changep()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి changeRdp = Convert_Write_Refs_గమ్యం_Read::ప్రధాన(changeWrp->clone_వృక్షం(అబద్ధం));
        Ast_కణుపు_సమాసం* comparedp = శూన్యసూచిక;
        incp->toggleExprp(origp->clone_వృక్షం(అబద్ధం));
        incp->toggleCovExprp(changeRdp->clone_వృక్షం(అబద్ధం));
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            ఒకవేళ (!bdtypep->isOpaque()) {
                Ast_కణుపు_సమాసం* స్థిరరాశి zp = కొత్త AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = కొత్త AstNeq{flp, zp, కొత్త AstXor{flp, origp, changeRdp}};
            }
        }
        ఒకవేళ (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* స్థిరరాశి newp = కొత్త AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->కూడు_Thensp(కొత్త Ast_కేటాయించు{flp, changeWrp, origp->clone_వృక్షం(అబద్ధం)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);  // No longer needed
    }

    //--------------------
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = అప్రమేయ;
};

//######################################################################
// Clock class functions

శూన్య V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("clock", 0, dump_వృక్షం_Either_Level() >= 3);
}
