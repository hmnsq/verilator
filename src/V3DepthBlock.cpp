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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3DepthBlock.h"

#অন্তর্ভাৱ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্ৰেণী DepthBlockVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ধ্রুৱ Ast_গ্ৰন্থি_Module* m_modp = nullptr;  // Current module
    ধ্রুৱ AstCFunc* m_cfuncp = nullptr;  // Current function
    পূর্ণাঙ্ক m_depth = 0;  // How deep in an expression
    পূর্ণাঙ্ক m_deep_অঙ্ক = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_গ্ৰন্থি_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* ধ্রুৱ scopep = m_cfuncp->scopep();
        ধ্রুৱ string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_অঙ্ক);
        AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ধ্রুৱ callp = নূতন AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        যদি (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        ঘূৰাই funcp;
    }

    // VISITORS
    শূন্য visit(Ast_গ্ৰন্থি_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_অঙ্ক = 0;
        iterateChildren(nodep);
    }
    শূন্য visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_Stmt_ৰাশিমালা* nodep) override {}  // Stop recursion after introducing new function
    শূন্য visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    শূন্য visit(Ast_গ্ৰন্থি_Stmt* nodep) override {
        ++m_depth;
        যদি (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            ধ্রুৱ Ast_গ্ৰন্থি* ধ্রুৱ backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ধ্রুৱ funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } অন্যথা {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    শূন্য visit(Ast_গ্ৰন্থি_ৰাশিমালা*) override {}  // Accelerate
    //--------------------
    শূন্য visit(AstVar*) override {}  // Don't hit varrefs under vars
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = স্ৱভাৱ;
};

//######################################################################
// DepthBlock class functions

শূন্য V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("deepblock", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
