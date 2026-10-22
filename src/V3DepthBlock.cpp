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

वर्ग गभीरता_खण्डम्_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ध्रुव Ast_ग्रन्थिः_Module* m_modp = शून्यसूचकः;  // Current module
    ध्रुव AstCFunc* m_cfuncp = शून्यसूचकः;  // Current function
    पूर्णाङ्क m_गभीरता = 0;  // How deep in an expression
    पूर्णाङ्क m_deep_अङ्कः = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ग्रन्थिः_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_पृष्ठभागः(&relinkHandle);
        // Create sub function
        AstScope* ध्रुव scopep = m_cfuncp->scopep();
        ध्रुव string name = m_cfuncp->name() + "__deep" + cvt_गन्तव्यम्_Str(++m_deep_अङ्कः);
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->सङ्कलय_Stmtsp(nodep);
        scopep->सङ्कलय_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ध्रुव callp = नूतन AstCCall{nodep->fileline(), funcp};
        callp->dtype_समुच्चयः_Void();
        यदि (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //समाप्तम्
        प्रत्यागम funcp;
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_अङ्कः = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_गभीरता);
        VL_RESTORER(m_cfuncp);
        m_गभीरता = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {}  // Stop recursion after introducing new function
    शून्य दर्शय(Ast_Jump_खण्डम्*) override {}  // Stop recursion as can't break up across a jump
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override {
        ++m_गभीरता;
        यदि (m_गभीरता > v3Global.opt.comp_सीमा_खण्डानि()) {  // Already done
            UINFO(4, "DeepBlocks " << m_गभीरता << " " << nodep);
            ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ध्रुव funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } अन्यथा {
            iterateChildren(nodep);
        }
        --m_गभीरता;
    }

    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्*) override {}  // Accelerate
    //--------------------
    शून्य दर्शय(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट गभीरता_खण्डम्_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~गभीरता_खण्डम्_Visitor() override = स्वभाव;
};

//######################################################################
// DepthBlock class functions

शून्य V3_गभीरता_खण्डम्::गभीरता_खण्डम्_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { गभीरता_खण्डम्_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("deepblock", 0, dump_वृक्षः_Either_Level() >= 3);
}
