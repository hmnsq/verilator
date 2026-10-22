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

صنف Convert_Write_Refs_مقصد_Read final : عام VNVisitor {
    // MEMBERS
    Ast_گره_عبارت* m_نتیجه = اشاره_تهی;

    // CONSTRUCTORS
    صریح Convert_Write_Refs_مقصد_Read(Ast_گره_عبارت* nodep) {
        m_نتیجه = VN_AS(iterateSubtreeReturnEdits(nodep), گره_عبارت);
    }

    // VISITORS
    فراغ بازدید(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        إذا (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                جديد AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    ساكن Ast_گره_عبارت* اصلی(Ast_گره_عبارت* nodep) { أرجع Convert_Write_Refs_مقصد_Read{nodep}.m_نتیجه; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

صنف ClockVisitor final : عام VNVisitor {
    // VISITORS
    فراغ بازدید(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ثابت incp = nodep->incp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت origp = nodep->origp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت changeWrp = nodep->changep()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت changeRdp = Convert_Write_Refs_مقصد_Read::اصلی(changeWrp->clone_درخت(خطأ));
        Ast_گره_عبارت* comparedp = اشاره_تهی;
        incp->toggleExprp(origp->clone_درخت(خطأ));
        incp->toggleCovExprp(changeRdp->clone_درخت(خطأ));
        File_سطر* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        إذا (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            إذا (!bdtypep->isOpaque()) {
                Ast_گره_عبارت* ثابت zp = جديد AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = جديد AstNeq{flp, zp, جديد AstXor{flp, origp, changeRdp}};
            }
        }
        إذا (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = جديد AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->جمع_کن_Thensp(جديد Ast_تخصیص_بده{flp, changeWrp, origp->clone_درخت(خطأ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(Ast_Sen_درخت* nodep) override {
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);  // No longer needed
    }

    //--------------------
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = افتراضي;
};

//######################################################################
// Clock class functions

فراغ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("clock", 0, dump_درخت_Either_Level() >= 3);
}
