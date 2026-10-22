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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Clock.h"

#অন্তর্ভাব "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

শ্রেণী ConvertWriteRefsToRead final : সর্বজনীন VNVisitor {
    // MEMBERS
    Ast_গ্রন্থি_রাশিমালা* m_ফলাফল = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_গ্রন্থি_রাশিমালা* nodep) {
        m_ফলাফল = VN_AS(iterateSubtreeReturnEdits(nodep), গ্রন্থি_রাশিমালা);
    }

    // VISITORS
    শূন্য visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        যদি (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                নতুন AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    স্থির Ast_গ্রন্থি_রাশিমালা* প্রধান(Ast_গ্রন্থি_রাশিমালা* nodep) { ফেরত ConvertWriteRefsToRead{nodep}.m_ফলাফল; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

শ্রেণী ClockVisitor final : সর্বজনীন VNVisitor {
    // VISITORS
    শূন্য visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ধ্রুবক incp = nodep->incp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক origp = nodep->origp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক changeWrp = nodep->changep()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক changeRdp = ConvertWriteRefsToRead::প্রধান(changeWrp->clone_বৃক্ষ(মিথ্যা));
        Ast_গ্রন্থি_রাশিমালা* comparedp = nullptr;
        incp->toggleExprp(origp->clone_বৃক্ষ(মিথ্যা));
        incp->toggleCovExprp(changeRdp->clone_বৃক্ষ(মিথ্যা));
        File_সারি* ধ্রুবক flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        যদি (ধ্রুবক AstBasicDType* ধ্রুবক bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            যদি (!bdtypep->isOpaque()) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক zp = নতুন AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = নতুন AstNeq{flp, zp, নতুন AstXor{flp, origp, changeRdp}};
            }
        }
        যদি (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ধ্রুবক newp = নতুন AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(নতুন AstAssign{flp, changeWrp, origp->clone_বৃক্ষ(মিথ্যা)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Clock class functions

শূন্য V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("clock", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
