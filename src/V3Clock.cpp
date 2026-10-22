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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Clock.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

ವರ್ಗ Convert_Write_Refs_ಗಮ್ಯ_Read final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // MEMBERS
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_ಫಲಿತಾಂಶ = ಶೂನ್ಯಸೂಚಕ;

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ Convert_Write_Refs_ಗಮ್ಯ_Read(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        m_ಫಲಿತಾಂಶ = VN_AS(iterateSubtreeReturnEdits(nodep), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        ಆದರೆ (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                ಹೊಸ AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮುಖ್ಯ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) { ಹಿಂತಿರುಗಿ Convert_Write_Refs_ಗಮ್ಯ_Read{nodep}.m_ಫಲಿತಾಂಶ; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

ವರ್ಗ ClockVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ಸ್ಥಿರಾಂಕ incp = nodep->incp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = nodep->origp()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ changeWrp = nodep->changep()->unlink_Fr_ಹಿಂಭಾಗ();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ changeRdp = Convert_Write_Refs_ಗಮ್ಯ_Read::ಮುಖ್ಯ(changeWrp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* comparedp = ಶೂನ್ಯಸೂಚಕ;
        incp->toggleExprp(origp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        incp->toggleCovExprp(changeRdp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            ಆದರೆ (!bdtypep->isOpaque()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ zp = ಹೊಸ AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = ಹೊಸ AstNeq{flp, zp, ಹೊಸ AstXor{flp, origp, changeRdp}};
            }
        }
        ಆದರೆ (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->ಕೂಡಿಸು_Thensp(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, changeWrp, origp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlink_Fr_ಹಿಂಭಾಗ()), nodep);  // No longer needed
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Clock class functions

ಶೂನ್ಯ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("clock", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
