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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3DepthBlock.h"

#அந்தர்பாவ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு DepthBlockVisitor final : பொது VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    மாறிலி Ast_முனை_Module* m_modp = nullptr;  // Current module
    மாறிலி AstCFunc* m_cfuncp = nullptr;  // Current function
    முழு m_depth = 0;  // How deep in an expression
    முழு m_deep_இலக்கம் = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_முனை_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* மாறிலி scopep = m_cfuncp->scopep();
        மாறிலி string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_இலக்கம்);
        AstCFunc* மாறிலி funcp = புதிய AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* மாறிலி callp = புதிய AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        எனில் (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        திரும்பு funcp;
    }

    // VISITORS
    வெறுமை visit(Ast_முனை_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_இலக்கம் = 0;
        iterateChildren(nodep);
    }
    வெறுமை visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_Stmt_கோவை* nodep) override {}  // Stop recursion after introducing new function
    வெறுமை visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    வெறுமை visit(Ast_முனை_Stmt* nodep) override {
        ++m_depth;
        எனில் (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            மாறிலி Ast_முனை* மாறிலி backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* மாறிலி funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } இல்லையெனில் {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    //--------------------
    வெறுமை visit(AstVar*) override {}  // Don't hit varrefs under vars
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = இயல்பு;
};

//######################################################################
// DepthBlock class functions

வெறுமை V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("deepblock", 0, dump_மரம்_Either_Level() >= 3);
}
