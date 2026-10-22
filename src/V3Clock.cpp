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
    Ast_ڳنڍ_اظهار* m_نتيجو = خالي_اشارو;

    // CONSTRUCTORS
    واضح Convert_Write_Refs_منزل_Read(Ast_ڳنڍ_اظهار* nodep) {
        m_نتيجو = VN_AS(iterateSubtreeReturnEdits(nodep), ڳنڍ_اظهار);
    }

    // VISITORS
    فراغ گهمو(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        إذا (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                جديد AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ڌڪو_Deletep(nodep), nodep);
        }
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    ساكن Ast_ڳنڍ_اظهار* مکيه(Ast_ڳنڍ_اظهار* nodep) { أرجع Convert_Write_Refs_منزل_Read{nodep}.m_نتيجو; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

صنف ClockVisitor final : عام VNVisitor {
    // VISITORS
    فراغ گهمو(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ثابت incp = nodep->incp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت origp = nodep->origp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت changeWrp = nodep->changep()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت changeRdp = Convert_Write_Refs_منزل_Read::مکيه(changeWrp->clone_وڻ(خطأ));
        Ast_ڳنڍ_اظهار* comparedp = خالي_اشارو;
        incp->toggleExprp(origp->clone_وڻ(خطأ));
        incp->toggleCovExprp(changeRdp->clone_وڻ(خطأ));
        File_سٽ* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        إذا (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            إذا (!bdtypep->isOpaque()) {
                Ast_ڳنڍ_اظهار* ثابت zp = جديد AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = جديد AstNeq{flp, zp, جديد AstXor{flp, origp, changeRdp}};
            }
        }
        إذا (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = جديد AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->جوڙيو_Thensp(جديد Ast_سونپيو{flp, changeWrp, origp->clone_وڻ(خطأ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_وڻ(), nodep);
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        VL_DO_DANGLING(ڌڪو_Deletep(nodep->unlink_Fr_پٺ()), nodep);  // No longer needed
    }

    //--------------------
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = افتراضي;
};

//######################################################################
// Clock class functions

فراغ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("clock", 0, dump_وڻ_Either_Level() >= 3);
}
