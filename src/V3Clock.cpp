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

ವರ್ಗ ConvertWriteRefsToRead final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // MEMBERS
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        ಯದಿ (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                ನೂತನ AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* main(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) { ಪ್ರತ್ಯಾಗಮ ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

ವರ್ಗ ClockVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // VISITORS
    ಶೂನ್ಯ visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ಧ್ರುವ incp = nodep->incp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = nodep->origp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ changeWrp = nodep->changep()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ changeRdp = ConvertWriteRefsToRead::main(changeWrp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* comparedp = nullptr;
        incp->toggleExprp(origp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        incp->toggleCovExprp(changeRdp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            ಯದಿ (!bdtypep->isOpaque()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ zp = ನೂತನ AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = ನೂತನ AstNeq{flp, zp, ನೂತನ AstXor{flp, origp, changeRdp}};
            }
        }
        ಯದಿ (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ಧ್ರುವ newp = ನೂತನ AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(ನೂತನ AstAssign{flp, changeWrp, origp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Clock class functions

ಶೂನ್ಯ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("clock", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
