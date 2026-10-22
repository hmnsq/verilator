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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3DepthBlock.h"

#ಅನ್ತರ್ಭಾವ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ DepthBlockVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp = nullptr;  // Current module
    ಧ್ರುವ AstCFunc* m_cfuncp = nullptr;  // Current function
    ಪೂರ್ಣಾಙ್ಕ m_depth = 0;  // How deep in an expression
    ಪೂರ್ಣಾಙ್ಕ m_deep_ಅಂಕೆ = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ಗಂಟು_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* ಧ್ರುವ scopep = m_cfuncp->scopep();
        ಧ್ರುವ string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_ಅಂಕೆ);
        AstCFunc* ಧ್ರುವ funcp = ನೂತನ AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ಧ್ರುವ callp = ನೂತನ AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        ಯದಿ (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        ಪ್ರತ್ಯಾಗಮ funcp;
    }

    // VISITORS
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}  // Stop recursion after introducing new function
    ಶೂನ್ಯ visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt* nodep) override {
        ++m_depth;
        ಯದಿ (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ಧ್ರುವ funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } ಅನ್ಯಥಾ {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    //--------------------
    ಶೂನ್ಯ visit(AstVar*) override {}  // Don't hit varrefs under vars
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// DepthBlock class functions

ಶೂನ್ಯ V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deepblock", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
