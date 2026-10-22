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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3DepthBlock.h"

#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ DepthBlockVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp = nullptr;  // Current module
    ଅଚଳ AstCFunc* m_cfuncp = nullptr;  // Current function
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_depth = 0;  // How deep in an expression
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_deep_ଅଙ୍କ = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ଗଣ୍ଠି_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* ଅଚଳ scopep = m_cfuncp->scopep();
        ଅଚଳ string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_ଅଙ୍କ);
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        ଯଦି (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        ଫେରନ୍ତୁ funcp;
    }

    // VISITORS
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_ଅଙ୍କ = 0;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_Stmt_ପଦସମଷ୍ଟି* nodep) override {}  // Stop recursion after introducing new function
    ଶୂନ୍ୟ visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt* nodep) override {
        ++m_depth;
        ଯଦି (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ଅଚଳ funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } ଅନ୍ୟଥା {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    //--------------------
    ଶୂନ୍ୟ visit(AstVar*) override {}  // Don't hit varrefs under vars
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// DepthBlock class functions

ଶୂନ୍ୟ V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("deepblock", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
