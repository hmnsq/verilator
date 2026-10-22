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

वर्ग ConvertWriteRefsToRead final : सार्वजनिक VNVisitor {
    // MEMBERS
    Ast_गाठ_अभिव्यक्ती* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_गाठ_अभिव्यक्ती* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), गाठ_अभिव्यक्ती);
    }

    // VISITORS
    शून्य visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        जर (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                नवीन AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    स्थिर Ast_गाठ_अभिव्यक्ती* main(Ast_गाठ_अभिव्यक्ती* nodep) { परत ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

वर्ग ClockVisitor final : सार्वजनिक VNVisitor {
    // VISITORS
    शून्य visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* स्थिरांक incp = nodep->incp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक origp = nodep->origp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक changeWrp = nodep->changep()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक changeRdp = ConvertWriteRefsToRead::main(changeWrp->clone_वृक्ष(असत्य));
        Ast_गाठ_अभिव्यक्ती* comparedp = nullptr;
        incp->toggleExprp(origp->clone_वृक्ष(असत्य));
        incp->toggleCovExprp(changeRdp->clone_वृक्ष(असत्य));
        File_ओळ* स्थिरांक flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        जर (स्थिरांक AstBasicDType* स्थिरांक bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            जर (!bdtypep->isOpaque()) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक zp = नवीन AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = नवीन AstNeq{flp, zp, नवीन AstXor{flp, origp, changeRdp}};
            }
        }
        जर (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* स्थिरांक newp = नवीन AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(नवीन AstAssign{flp, changeWrp, origp->clone_वृक्ष(असत्य)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(Ast_Sen_वृक्ष* nodep) override {
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Clock class functions

शून्य V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("clock", 0, dump_वृक्ष_Either_Level() >= 3);
}
