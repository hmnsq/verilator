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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Clock.h"

#अन्तर्भाव "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

वर्ग Convert_Write_Refs_गन्तव्यम्_Read final : सार्वजनिक VNVisitor {
    // MEMBERS
    Ast_ग्रन्थिः_व्यञ्जकम्* m_परिणाम = शून्यसूचकः;

    // CONSTRUCTORS
    स्पष्ट Convert_Write_Refs_गन्तव्यम्_Read(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        m_परिणाम = VN_AS(iterateSubtreeReturnEdits(nodep), ग्रन्थिः_व्यञ्जकम्);
    }

    // VISITORS
    शून्य दर्शय(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        यदि (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                नूतन AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* मुख्यम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) { प्रत्यागम Convert_Write_Refs_गन्तव्यम्_Read{nodep}.m_परिणाम; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

वर्ग ClockVisitor final : सार्वजनिक VNVisitor {
    // VISITORS
    शून्य दर्शय(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ध्रुव incp = nodep->incp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव origp = nodep->origp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव changeWrp = nodep->changep()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव changeRdp = Convert_Write_Refs_गन्तव्यम्_Read::मुख्यम्(changeWrp->clone_वृक्षः(असत्य));
        Ast_ग्रन्थिः_व्यञ्जकम्* comparedp = शून्यसूचकः;
        incp->toggleExprp(origp->clone_वृक्षः(असत्य));
        incp->toggleCovExprp(changeRdp->clone_वृक्षः(असत्य));
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        यदि (ध्रुव AstBasicDType* ध्रुव bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            यदि (!bdtypep->isOpaque()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव zp = नूतन AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = नूतन AstNeq{flp, zp, नूतन AstXor{flp, origp, changeRdp}};
            }
        }
        यदि (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ध्रुव newp = नूतन AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->सङ्कलय_Thensp(नूतन Ast_नियोजय{flp, changeWrp, origp->clone_वृक्षः(असत्य)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(Ast_Sen_वृक्षः* nodep) override {
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);  // No longer needed
    }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = स्वभाव;
};

//######################################################################
// Clock class functions

शून्य V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("clock", 0, dump_वृक्षः_Either_Level() >= 3);
}
