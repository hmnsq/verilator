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
    Ast_تۈگۈن_ئىپادە* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_تۈگۈن_ئىپادە* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), تۈگۈن_ئىپادە);
    }

    // VISITORS
    فراغ visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        إذا (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                جديد AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep), nodep);
        }
    }

    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

عام:
    ساكن Ast_تۈگۈن_ئىپادە* main(Ast_تۈگۈن_ئىپادە* nodep) { أرجع ConvertWriteRefsToRead{nodep}.m_result; }
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
        Ast_تۈگۈن_ئىپادە* ثابت origp = nodep->origp()->unlinkFrBack();
        Ast_تۈگۈن_ئىپادە* ثابت changeWrp = nodep->changep()->unlinkFrBack();
        Ast_تۈگۈن_ئىپادە* ثابت changeRdp = ConvertWriteRefsToRead::main(changeWrp->clone_دەرەخ(خطأ));
        Ast_تۈگۈن_ئىپادە* comparedp = nullptr;
        incp->toggleExprp(origp->clone_دەرەخ(خطأ));
        incp->toggleCovExprp(changeRdp->clone_دەرەخ(خطأ));
        File_قۇر* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        إذا (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            إذا (!bdtypep->isOpaque()) {
                Ast_تۈگۈن_ئىپادە* ثابت zp = جديد AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = جديد AstNeq{flp, zp, جديد AstXor{flp, origp, changeRdp}};
            }
        }
        إذا (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = جديد AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(جديد AstAssign{flp, changeWrp, origp->clone_دەرەخ(خطأ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_دەرەخ(), nodep);
    }
    فراغ visit(Ast_Sen_دەرەخ* nodep) override {
        VL_DO_DANGLING(ئىتتىرىش_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_دەرەخ("clock", 0, dump_دەرەخ_Either_Level() >= 3);
}
