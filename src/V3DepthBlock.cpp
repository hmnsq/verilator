// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prevent very deep expressions
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
// V3DepthBlock's Transformations:
//
// Each module:
//      For each deep block, create cfunc including that block.
//
//*************************************************************************

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3DepthBlock.h"

#અન્તર્ભાવ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ DepthBlockVisitor final : જાહેર VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    અચલ Ast_ગાંઠ_Module* m_modp = nullptr;  // Current module
    અચલ AstCFunc* m_cfuncp = nullptr;  // Current function
    પૂર્ણાંક m_depth = 0;  // How deep in an expression
    પૂર્ણાંક m_deep_અંક = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ગાંઠ_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* અચલ scopep = m_cfuncp->scopep();
        અચલ string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_અંક);
        AstCFunc* અચલ funcp = નવું AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* અચલ callp = નવું AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        જો (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        પાછા funcp;
    }

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_અંક = 0;
        iterateChildren(nodep);
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_Stmt_પદાવલિ* nodep) override {}  // Stop recursion after introducing new function
    શૂન્ય visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        ++m_depth;
        જો (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            અચલ Ast_ગાંઠ* અચલ backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* અચલ funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } નહીંતર {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    //--------------------
    શૂન્ય visit(AstVar*) override {}  // Don't hit varrefs under vars
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = મૂળભૂત;
};

//######################################################################
// DepthBlock class functions

શૂન્ય V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("deepblock", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
