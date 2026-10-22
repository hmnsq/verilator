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

جماعت گہرائی_بلاک_Visitor final : عوامی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گرہ_Module* m_modp = خالی_اشاریہ;  // Current module
    ثابت AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current function
    صحیح m_گہرائی = 0;  // How deep in an expression
    صحیح m_deep_عدد = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_گرہ_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_عقب(&relinkHandle);
        // Create sub function
        AstScope* ثابت scopep = m_cfuncp->scopep();
        ثابت string name = m_cfuncp->name() + "__deep" + cvt_مقصد_Str(++m_deep_عدد);
        AstCFunc* ثابت funcp = نیا AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->جوڑو_Stmtsp(nodep);
        scopep->جوڑو_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ثابت callp = نیا AstCCall{nodep->fileline(), funcp};
        callp->dtype_سیٹ_Void();
        اگر (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //مکمل
        واپس funcp;
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_عدد = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_گہرائی);
        VL_RESTORER(m_cfuncp);
        m_گہرائی = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_Stmt_اظہاریہ* nodep) override {}  // Stop recursion after introducing new function
    خالی دورہ(Ast_Jump_بلاک*) override {}  // Stop recursion as can't break up across a jump
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        ++m_گہرائی;
        اگر (m_گہرائی > v3Global.opt.comp_حد_بندی_بلاکیں()) {  // Already done
            UINFO(4, "DeepBlocks " << m_گہرائی << " " << nodep);
            ثابت Ast_گرہ* ثابت backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ثابت funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } ورنہ {
            iterateChildren(nodep);
        }
        --m_گہرائی;
    }

    خالی دورہ(Ast_گرہ_اظہاریہ*) override {}  // Accelerate
    //--------------------
    خالی دورہ(AstVar*) override {}  // Don't hit varrefs under vars
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح گہرائی_بلاک_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~گہرائی_بلاک_Visitor() override = طےشدہ;
};

//######################################################################
// DepthBlock class functions

خالی V3_گہرائی_بلاک::گہرائی_بلاک_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { گہرائی_بلاک_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deepblock", 0, dump_درخت_Either_Level() >= 3);
}
