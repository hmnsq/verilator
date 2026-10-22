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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Clock.h"

#شامل "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

رده ConvertWriteRefsToRead final : عمومی VNVisitor {
    // MEMBERS
    Ast_گره_عبارت* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_گره_عبارت* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), گره_عبارت);
    }

    // VISITORS
    تهی visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        اگر (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                جدید AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    ایستا Ast_گره_عبارت* اصلی(Ast_گره_عبارت* nodep) { بازگشت ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

رده ClockVisitor final : عمومی VNVisitor {
    // VISITORS
    تهی visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ثابت incp = nodep->incp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت origp = nodep->origp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت changeWrp = nodep->changep()->unlinkFrBack();
        Ast_گره_عبارت* ثابت changeRdp = ConvertWriteRefsToRead::اصلی(changeWrp->clone_درخت(نادرست));
        Ast_گره_عبارت* comparedp = nullptr;
        incp->toggleExprp(origp->clone_درخت(نادرست));
        incp->toggleCovExprp(changeRdp->clone_درخت(نادرست));
        File_سطر* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        اگر (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            اگر (!bdtypep->isOpaque()) {
                Ast_گره_عبارت* ثابت zp = جدید AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = جدید AstNeq{flp, zp, جدید AstXor{flp, origp, changeRdp}};
            }
        }
        اگر (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = جدید AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(جدید AstAssign{flp, changeWrp, origp->clone_درخت(نادرست)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    تهی visit(Ast_Sen_درخت* nodep) override {
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = پیشفرض;
};

//######################################################################
// Clock class functions

تهی V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("clock", 0, dump_درخت_Either_Level() >= 3);
}
