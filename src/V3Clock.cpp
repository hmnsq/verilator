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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Clock.h"

#समावेश "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

श्रेणी ConvertWriteRefsToRead final : खुला VNVisitor {
    // MEMBERS
    Ast_गाँठ_व्यंजक* m_परिणाम = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_गाँठ_व्यंजक* nodep) {
        m_परिणाम = VN_AS(iterateSubtreeReturnEdits(nodep), गाँठ_व्यंजक);
    }

    // VISITORS
    व्योम visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        अगर (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                नया AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(धकेलो_Deletep(nodep), nodep);
        }
    }

    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    अटल Ast_गाँठ_व्यंजक* मुख्य(Ast_गाँठ_व्यंजक* nodep) { वापस ConvertWriteRefsToRead{nodep}.m_परिणाम; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

श्रेणी ClockVisitor final : खुला VNVisitor {
    // VISITORS
    व्योम visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* स्थिर incp = nodep->incp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर origp = nodep->origp()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर changeWrp = nodep->changep()->unlinkFrBack();
        Ast_गाँठ_व्यंजक* स्थिर changeRdp = ConvertWriteRefsToRead::मुख्य(changeWrp->clone_वृक्ष(false));
        Ast_गाँठ_व्यंजक* comparedp = nullptr;
        incp->toggleExprp(origp->clone_वृक्ष(false));
        incp->toggleCovExprp(changeRdp->clone_वृक्ष(false));
        File_पंक्ति* स्थिर flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        अगर (स्थिर AstBasicDType* स्थिर bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            अगर (!bdtypep->isOpaque()) {
                Ast_गाँठ_व्यंजक* स्थिर zp = नया AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = नया AstNeq{flp, zp, नया AstXor{flp, origp, changeRdp}};
            }
        }
        अगर (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* स्थिर newp = नया AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(नया AstAssign{flp, changeWrp, origp->clone_वृक्ष(false)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    व्योम visit(Ast_Sen_वृक्ष* nodep) override {
        VL_DO_DANGLING(धकेलो_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = शेष;
};

//######################################################################
// Clock class functions

व्योम V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("clock", 0, dump_वृक्ष_Either_Level() >= 3);
}
