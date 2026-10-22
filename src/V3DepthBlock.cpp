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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3DepthBlock.h"

#अन्तर्भाव "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग DepthBlockVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव Ast_गाँठ_Module* m_modp = nullptr;  // Current module
    ध्रुव AstCFunc* m_cfuncp = nullptr;  // Current function
    पूर्णाङ्क m_depth = 0;  // How deep in an expression
    पूर्णाङ्क m_deep_अंक = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_गाँठ_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlinkFrBack(&relinkHandle);
        // Create sub function
        AstScope* ध्रुव scopep = m_cfuncp->scopep();
        ध्रुव string name = m_cfuncp->name() + "__deep" + cvtToStr(++m_deep_अंक);
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->addStmtsp(nodep);
        scopep->addBlocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ध्रुव callp = नूतन AstCCall{nodep->fileline(), funcp};
        callp->dtypeSetVoid();
        यदि (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        // Done
        प्रत्यागम funcp;
    }

    // VISITORS
    शून्य visit(Ast_गाँठ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_अंक = 0;
        iterateChildren(nodep);
    }
    शून्य visit(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_depth);
        VL_RESTORER(m_cfuncp);
        m_depth = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_Stmt_व्यंजक* nodep) override {}  // Stop recursion after introducing new function
    शून्य visit(AstJumpBlock*) override {}  // Stop recursion as can't break up across a jump
    शून्य visit(Ast_गाँठ_Stmt* nodep) override {
        ++m_depth;
        यदि (m_depth > v3Global.opt.compLimitBlocks()) {  // Already done
            UINFO(4, "DeepBlocks " << m_depth << " " << nodep);
            ध्रुव Ast_गाँठ* ध्रुव backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ध्रुव funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } अन्यथा {
            iterateChildren(nodep);
        }
        --m_depth;
    }

    शून्य visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    //--------------------
    शून्य visit(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit DepthBlockVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~DepthBlockVisitor() override = स्वभाव;
};

//######################################################################
// DepthBlock class functions

शून्य V3DepthBlock::depthBlockAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthBlockVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("deepblock", 0, dump_वृक्ष_Either_Level() >= 3);
}
