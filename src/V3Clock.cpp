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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Clock.h"

#અન્તર્ભાવ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

વર્ગ ConvertWriteRefsToRead final : જાહેર VNVisitor {
    // MEMBERS
    Ast_ગાંઠ_પદાવલિ* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_ગાંઠ_પદાવલિ* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), ગાંઠ_પદાવલિ);
    }

    // VISITORS
    શૂન્ય visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        જો (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                નવું AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ધકેલો_Deletep(nodep), nodep);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    સ્થિર Ast_ગાંઠ_પદાવલિ* મુખ્ય(Ast_ગાંઠ_પદાવલિ* nodep) { પાછા ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

વર્ગ ClockVisitor final : જાહેર VNVisitor {
    // VISITORS
    શૂન્ય visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* અચલ incp = nodep->incp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ origp = nodep->origp()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ changeWrp = nodep->changep()->unlinkFrBack();
        Ast_ગાંઠ_પદાવલિ* અચલ changeRdp = ConvertWriteRefsToRead::મુખ્ય(changeWrp->clone_વૃક્ષ(ખોટું));
        Ast_ગાંઠ_પદાવલિ* comparedp = nullptr;
        incp->toggleExprp(origp->clone_વૃક્ષ(ખોટું));
        incp->toggleCovExprp(changeRdp->clone_વૃક્ષ(ખોટું));
        File_લીટી* અચલ flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        જો (અચલ AstBasicDType* અચલ bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            જો (!bdtypep->isOpaque()) {
                Ast_ગાંઠ_પદાવલિ* અચલ zp = નવું AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = નવું AstNeq{flp, zp, નવું AstXor{flp, origp, changeRdp}};
            }
        }
        જો (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* અચલ newp = નવું AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(નવું AstAssign{flp, changeWrp, origp->clone_વૃક્ષ(ખોટું)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        VL_DO_DANGLING(ધકેલો_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = મૂળભૂત;
};

//######################################################################
// Clock class functions

શૂન્ય V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("clock", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
