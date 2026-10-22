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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3DepthBlock.h"

#شامل "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده DepthBlockVisitor final : عمومی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گره_Module* m_modp = nullptr;  // Current module
    ثابت AstCFunc* m_cfuncp = nullptr;  // Current function
    صحیح m_depth = 0;  // How deep in an expression
    صحیح m_deep_رقم = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_گره_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* ثابت scopep = m_cfuncp->scopep();
        ثابت string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_رقم);
        AstCFunc* ثابت funcp = جدید AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ثابت callp = جدید AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        اگر (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        بازگشت funcp;
    }

    // VISITORS
    تهی visit(Ast_گره_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_رقم = 0;
        iterateChildren(nodep);
    }
    تهی visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_Stmt_عبارت* nodep) override {}  // Stop recursion after introducing new function
    تهی visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    تهی visit(Ast_گره_Stmt* nodep) override {
        ++m_depth;
        اگر (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            ثابت Ast_گره* ثابت backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ثابت funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } وگرنه {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    //--------------------
    تهی visit(AstVar*) override {}  // Don't hit varrefs under vars
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = پیشفرض;
};

//######################################################################
// DepthBlock class functions

تهی V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deepblock", 0, dump_درخت_Either_Level() >= 3);
}
