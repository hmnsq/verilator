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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Clock.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

ਵਰਗ Convert_Write_Refs_ਮੰਜ਼ਿਲ_Read final : ਜਨਤਕ VNVisitor {
    // MEMBERS
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_ਨਤੀਜਾ = ਖਾਲੀ_ਸੂਚਕ;

    // CONSTRUCTORS
    ਸਪਸ਼ਟ Convert_Write_Refs_ਮੰਜ਼ਿਲ_Read(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        m_ਨਤੀਜਾ = VN_AS(iterateSubtreeReturnEdits(nodep), ਗੰਢ_ਵਿਅੰਜਕ);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        ਜੇ (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                ਨਵਾਂ AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੁੱਖ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) { ਵਾਪਸ Convert_Write_Refs_ਮੰਜ਼ਿਲ_Read{nodep}.m_ਨਤੀਜਾ; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

ਵਰਗ ClockVisitor final : ਜਨਤਕ VNVisitor {
    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* ਅਚਲ incp = nodep->incp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = nodep->origp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ changeWrp = nodep->changep()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ changeRdp = Convert_Write_Refs_ਮੰਜ਼ਿਲ_Read::ਮੁੱਖ(changeWrp->clone_ਰੁੱਖ(ਝੂਠ));
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* comparedp = ਖਾਲੀ_ਸੂਚਕ;
        incp->toggleExprp(origp->clone_ਰੁੱਖ(ਝੂਠ));
        incp->toggleCovExprp(changeRdp->clone_ਰੁੱਖ(ਝੂਠ));
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            ਜੇ (!bdtypep->isOpaque()) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ zp = ਨਵਾਂ AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = ਨਵਾਂ AstNeq{flp, zp, ਨਵਾਂ AstXor{flp, origp, changeRdp}};
            }
        }
        ਜੇ (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* ਅਚਲ newp = ਨਵਾਂ AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->ਜੋੜੋ_Thensp(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, changeWrp, origp->clone_ਰੁੱਖ(ਝੂਠ)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);  // No longer needed
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = ਮੂਲ;
};

//######################################################################
// Clock class functions

ਖਾਲੀ V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("clock", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
