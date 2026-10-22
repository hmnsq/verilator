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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3DepthBlock.h"

#അന്തര്ഭാവ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം DepthBlockVisitor final : പൊതു VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    സ്ഥിരം Ast_കണ്ണി_Module* m_modp = nullptr;  // Current module
    സ്ഥിരം AstCFunc* m_cfuncp = nullptr;  // Current function
    പൂർണ്ണസംഖ്യ m_depth = 0;  // How deep in an expression
    പൂർണ്ണസംഖ്യ m_deep_അക്കം = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_കണ്ണി_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* സ്ഥിരം scopep = m_cfuncp->scopep();
        സ്ഥിരം string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_അക്കം);
        AstCFunc* സ്ഥിരം funcp = പുതിയ AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* സ്ഥിരം callp = പുതിയ AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        എങ്കിൽ (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        തിരികെ funcp;
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_അക്കം = 0;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {}  // Stop recursion after introducing new function
    ശൂന്യം visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    ശൂന്യം visit(Ast_കണ്ണി_Stmt* nodep) override {
        ++m_depth;
        എങ്കിൽ (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* സ്ഥിരം funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } അല്ലെങ്കിൽ {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    //--------------------
    ശൂന്യം visit(AstVar*) override {}  // Don't hit varrefs under vars
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// DepthBlock class functions

ശൂന്യം V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("deepblock", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
