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

ವರ್ಗ ಆಳ_ಖಂಡ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp = ಶೂನ್ಯಸೂಚಕ;  // Current module
    ಸ್ಥಿರಾಂಕ AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current function
    ಪೂರ್ಣಾಂಕ m_ಆಳ = 0;  // How deep in an expression
    ಪೂರ್ಣಾಂಕ m_deep_ಅಂಕೆ = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ಗಂಟು_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_ಹಿಂಭಾಗ(&relinkHandle);
        // Create sub function
        AstScope* ಸ್ಥಿರಾಂಕ scopep = m_cfuncp->scopep();
        ಸ್ಥಿರಾಂಕ string name = m_cfuncp->name() + "__deep" + cvt_ಗಮ್ಯ_Str(++m_deep_ಅಂಕೆ);
        AstCFunc* ಸ್ಥಿರಾಂಕ funcp = ಹೊಸ AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->ಕೂಡಿಸು_Stmtsp(nodep);
        scopep->ಕೂಡಿಸು_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ಸ್ಥಿರಾಂಕ callp = ಹೊಸ AstCCall{nodep->fileline(), funcp};
        callp->dtype_ಗಣ_Void();
        ಆದರೆ (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //ಮುಗಿದ
        ಹಿಂತಿರುಗಿ funcp;
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_ಅಂಕೆ = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_ಆಳ);
        VL_RESTORER(m_cfuncp);
        m_ಆಳ = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {}  // Stop recursion after introducing new function
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Jump_ಖಂಡ*) override {}  // Stop recursion as can't break up across a jump
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override {
        ++m_ಆಳ;
        ಆದರೆ (m_ಆಳ > v3Global.opt.comp_ಎಲ್ಲೆ_ಖಂಡಗಳು()) {  // Already done
            UINFO(4, "DeepBlocks " << m_ಆಳ << " " << nodep);
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ಸ್ಥಿರಾಂಕ funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildren(nodep);
        }
        --m_ಆಳ;
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Don't hit varrefs under vars
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಆಳ_ಖಂಡ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ಆಳ_ಖಂಡ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// DepthBlock class functions

ಶೂನ್ಯ V3_ಆಳ_ಖಂಡ::ಆಳ_ಖಂಡ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಆಳ_ಖಂಡ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("deepblock", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
