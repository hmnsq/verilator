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

صنف ConvertWriteRefsToRead final : عام VNVisitor {
    // MEMBERS
    Ast_عقدة_تعبير* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_عقدة_تعبير* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), عقدة_تعبير);
    }

    // VISITORS
    فراغ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        إذا (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                جديد AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ادفع_Deletep(nodep), nodep);
        }
    }

    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    ساكن Ast_عقدة_تعبير* الرئيسية(Ast_عقدة_تعبير* nodep) { إرجاع ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

صنف ClockVisitor final : عام VNVisitor {
    // VISITORS
    فراغ visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ثابت incp = nodep->incp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت origp = nodep->origp()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت changeWrp = nodep->changep()->unlinkFrBack();
        Ast_عقدة_تعبير* ثابت changeRdp = ConvertWriteRefsToRead::الرئيسية(changeWrp->clone_شجرة(خطأ));
        Ast_عقدة_تعبير* comparedp = nullptr;
        incp->toggleExprp(origp->clone_شجرة(خطأ));
        incp->toggleCovExprp(changeRdp->clone_شجرة(خطأ));
        File_سطر* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        إذا (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            إذا (!bdtypep->isOpaque()) {
                Ast_عقدة_تعبير* ثابت zp = جديد AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = جديد AstNeq{flp, zp, جديد AstXor{flp, origp, changeRdp}};
            }
        }
        إذا (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = جديد AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(جديد AstAssign{flp, changeWrp, origp->clone_شجرة(خطأ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_شجرة(), nodep);
    }
    فراغ visit(Ast_Sen_شجرة* nodep) override {
        VL_DO_DANGLING(ادفع_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    فراغ visit(Ast_عقدة* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = افتراضي;
};

//######################################################################
// Clock class functions

فراغ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_شجرة("clock", 0, dump_شجرة_Either_Level() >= 3);
}
