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

বর্গ ConvertWriteRefsToRead final : সার্বজনিক VNVisitor {
    // MEMBERS
    Ast_গ্রন্থি_রাশিমালা* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_গ্রন্থি_রাশিমালা* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), গ্রন্থি_রাশিমালা);
    }

    // VISITORS
    শূন্য visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        যদি (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                নূতন AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ঠেলো_Deletep(nodep), nodep);
        }
    }

    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    স্থির Ast_গ্রন্থি_রাশিমালা* main(Ast_গ্রন্থি_রাশিমালা* nodep) { প্রত্যাগম ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

বর্গ ClockVisitor final : সার্বজনিক VNVisitor {
    // VISITORS
    শূন্য visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ধ্রুব incp = nodep->incp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব origp = nodep->origp()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব changeWrp = nodep->changep()->unlinkFrBack();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুব changeRdp = ConvertWriteRefsToRead::main(changeWrp->clone_বৃক্ষ(অসত্য));
        Ast_গ্রন্থি_রাশিমালা* comparedp = nullptr;
        incp->toggleExprp(origp->clone_বৃক্ষ(অসত্য));
        incp->toggleCovExprp(changeRdp->clone_বৃক্ষ(অসত্য));
        File_সারি* ধ্রুব flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        যদি (ধ্রুব AstBasicDType* ধ্রুব bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            যদি (!bdtypep->isOpaque()) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুব zp = নূতন AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = নূতন AstNeq{flp, zp, নূতন AstXor{flp, origp, changeRdp}};
            }
        }
        যদি (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ধ্রুব newp = নূতন AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(নূতন AstAssign{flp, changeWrp, origp->clone_বৃক্ষ(অসত্য)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }
    শূন্য visit(Ast_Sen_বৃক্ষ* nodep) override {
        VL_DO_DANGLING(ঠেলো_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সার্বজনিক:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = স্বভাব;
};

//######################################################################
// Clock class functions

শূন্য V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("clock", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
