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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Clock.h"

#شامل "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

جماعت Convert_Write_Refs_مقصد_Read final : عوامی VNVisitor {
    // MEMBERS
    Ast_گرہ_اظہاریہ* m_نتیجہ = خالی_اشاریہ;

    // CONSTRUCTORS
    صریح Convert_Write_Refs_مقصد_Read(Ast_گرہ_اظہاریہ* nodep) {
        m_نتیجہ = VN_AS(iterateSubtreeReturnEdits(nodep), گرہ_اظہاریہ);
    }

    // VISITORS
    خالی دورہ(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        اگر (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                نیا AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ڈالو_Deletep(nodep), nodep);
        }
    }

    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    ساکن Ast_گرہ_اظہاریہ* مین(Ast_گرہ_اظہاریہ* nodep) { واپس Convert_Write_Refs_مقصد_Read{nodep}.m_نتیجہ; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

جماعت ClockVisitor final : عوامی VNVisitor {
    // VISITORS
    خالی دورہ(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ثابت incp = nodep->incp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت origp = nodep->origp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت changeWrp = nodep->changep()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت changeRdp = Convert_Write_Refs_مقصد_Read::مین(changeWrp->clone_درخت(جھوٹ));
        Ast_گرہ_اظہاریہ* comparedp = خالی_اشاریہ;
        incp->toggleExprp(origp->clone_درخت(جھوٹ));
        incp->toggleCovExprp(changeRdp->clone_درخت(جھوٹ));
        File_سطر* ثابت flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        اگر (ثابت AstBasicDType* ثابت bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            اگر (!bdtypep->isOpaque()) {
                Ast_گرہ_اظہاریہ* ثابت zp = نیا AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = نیا AstNeq{flp, zp, نیا AstXor{flp, origp, changeRdp}};
            }
        }
        اگر (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ثابت newp = نیا AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->جوڑو_Thensp(نیا Ast_تفویض{flp, changeWrp, origp->clone_درخت(جھوٹ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        VL_DO_DANGLING(ڈالو_Deletep(nodep->unlink_Fr_عقب()), nodep);  // No longer needed
    }

    //--------------------
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = طےشدہ;
};

//######################################################################
// Clock class functions

خالی V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("clock", 0, dump_درخت_Either_Level() >= 3);
}
