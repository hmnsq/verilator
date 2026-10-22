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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Clock.h"

#ଅନ୍ତର୍ଭାଵ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

ଶ୍ରେଣୀ ConvertWriteRefsToRead final : ସର୍ବସାଧାରଣ VNVisitor {
    // MEMBERS
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        ଯଦି (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                ନୂତନ AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ଠେଲ_Deletep(nodep), nodep);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ମୁଖ୍ୟ(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) { ଫେରନ୍ତୁ ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

ଶ୍ରେଣୀ ClockVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // VISITORS
    ଶୂନ୍ୟ visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ଅଚଳ incp = nodep->incp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = nodep->origp()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ changeWrp = nodep->changep()->unlinkFrBack();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ changeRdp = ConvertWriteRefsToRead::ମୁଖ୍ୟ(changeWrp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* comparedp = nullptr;
        incp->toggleExprp(origp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        incp->toggleCovExprp(changeRdp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            ଯଦି (!bdtypep->isOpaque()) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ zp = ନୂତନ AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = ନୂତନ AstNeq{flp, zp, ନୂତନ AstXor{flp, origp, changeRdp}};
            }
        }
        ଯଦି (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ଅଚଳ newp = ନୂତନ AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(ନୂତନ AstAssign{flp, changeWrp, origp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Sen_ବୃକ୍ଷ* nodep) override {
        VL_DO_DANGLING(ଠେଲ_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Clock class functions

ଶୂନ୍ୟ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("clock", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
