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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Clock.h"

#ضمن "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

صنف Convert_Write_Refs_منزل_Read final : عام VNVisitor {
    // MEMBERS
    Ast_غوټه_بیان* m_پایله = تش_اشاره;

    // CONSTRUCTORS
    څرګند Convert_Write_Refs_منزل_Read(Ast_غوټه_بیان* nodep) {
        m_پایله = VN_AS(iterateSubtreeReturnEdits(nodep), غوټه_بیان);
    }

    // VISITORS
    فراغ لیدنه(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        إذا (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                جديد AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }

    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    ساكن Ast_غوټه_بیان* اصلي(Ast_غوټه_بیان* nodep) { أرجع Convert_Write_Refs_منزل_Read{nodep}.m_پایله; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

صنف ClockVisitor final : عام VNVisitor {
    // VISITORS
    فراغ لیدنه(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ثابت incp = nodep->incp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت origp = nodep->origp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت changeWrp = nodep->changep()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت changeRdp = Convert_Write_Refs_منزل_Read::اصلي(changeWrp->clone_ونه(خطأ));
        Ast_غوټه_بیان* comparedp = تش_اشاره;
        incp->toggleExprp(origp->clone_ونه(خطأ));
        incp->toggleCovExprp(changeRdp->clone_ونه(خطأ));
        File_کرښه* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        إذا (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            إذا (!bdtypep->isOpaque()) {
                Ast_غوټه_بیان* ثابت zp = جديد AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = جديد AstNeq{flp, zp, جديد AstXor{flp, origp, changeRdp}};
            }
        }
        إذا (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = جديد AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->جمع_کړه_Thensp(جديد Ast_ټاکل{flp, changeWrp, origp->clone_ونه(خطأ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(Ast_Sen_ونه* nodep) override {
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);  // No longer needed
    }

    //--------------------
    فراغ لیدنه(Ast_غوټه* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    څرګند ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = افتراضي;
};

//######################################################################
// Clock class functions

فراغ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("clock", 0, dump_ونه_Either_Level() >= 3);
}
