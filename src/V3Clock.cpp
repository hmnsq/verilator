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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Clock.h"

#അന്തര്ഭാവ "V3Const.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert every WRITE AstVarRef to a READ ref

വർഗ്ഗം ConvertWriteRefsToRead final : പൊതു VNVisitor {
    // MEMBERS
    Ast_കണ്ണി_വ്യഞ്ജകം* m_result = nullptr;

    // CONSTRUCTORS
    explicit ConvertWriteRefsToRead(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        m_result = VN_AS(iterateSubtreeReturnEdits(nodep), കണ്ണി_വ്യഞ്ജകം);
    }

    // VISITORS
    ശൂന്യം visit(AstVarRef* nodep) override {
        UASSERT_OBJ(!nodep->access().isRW(), nodep, "Cannot handle a READWRITE reference");
        എങ്കിൽ (nodep->access().isWriteOnly()) {
            nodep->replaceWith(
                പുതിയ AstVarRef{nodep->fileline(), nodep->varScopep(), VAccess::READ});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* പ്രധാന(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) { തിരികെ ConvertWriteRefsToRead{nodep}.m_result; }
};

//######################################################################
// Clock state, as a visitor of each AstNode

വർഗ്ഗം ClockVisitor final : പൊതു VNVisitor {
    // VISITORS
    ശൂന്യം visit(AstCoverToggle* nodep) override {
        // UINFOTREE(1, nodep, "", "ct");
        // COVERTOGGLE(INC, ORIG, CHANGE) ->
        //   IF(ORIG ^ CHANGE) { INC; CHANGE = ORIG; }
        AstCoverInc* സ്ഥിരം incp = nodep->incp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = nodep->origp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം changeWrp = nodep->changep()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം changeRdp = ConvertWriteRefsToRead::പ്രധാന(changeWrp->clone_വൃക്ഷം(അസത്യം));
        Ast_കണ്ണി_വ്യഞ്ജകം* comparedp = nullptr;
        incp->toggleExprp(origp->clone_വൃക്ഷം(അസത്യം));
        incp->toggleCovExprp(changeRdp->clone_വൃക്ഷം(അസത്യം));
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        // Xor will optimize better than Eq, when CoverToggle has bit selects,
        // but can only use Xor with non-opaque types
        എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep
            = VN_CAST(origp->dtypep()->skipRefp(), BasicDType)) {
            എങ്കിൽ (!bdtypep->isOpaque()) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം zp = പുതിയ AstConst{flp, AstConst::DTyped{}, origp->dtypep()};
                comparedp = പുതിയ AstNeq{flp, zp, പുതിയ AstXor{flp, origp, changeRdp}};
            }
        }
        എങ്കിൽ (!comparedp) comparedp = AstNeq::newTyped(flp, origp, changeRdp);
        AstIf* സ്ഥിരം newp = പുതിയ AstIf{flp, comparedp, incp};
        // We could add another IF to detect posedges, and only increment if so.
        // It's another whole branch though versus a potential memory miss.
        // We'll go with the miss.
        newp->addThensp(പുതിയ AstAssign{flp, changeWrp, origp->clone_വൃക്ഷം(അസത്യം)});
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(Ast_Sen_വൃക്ഷം* nodep) override {
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);  // No longer needed
    }

    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit ClockVisitor(AstNetlist* netlistp) { iterate(netlistp); }
    ~ClockVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Clock class functions

ശൂന്യം V3Clock::clockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("clock", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
