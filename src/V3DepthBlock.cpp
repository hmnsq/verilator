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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3DepthBlock.h"

#ضمن "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف اونهائي_بلاڪ_Visitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_ڳنڍ_Module* m_modp = خالي_اشارو;  // Current module
    ثابت AstCFunc* m_cfuncp = خالي_اشارو;  // Current function
    صحيح m_اونهائي = 0;  // How deep in an expression
    صحيح m_deep_انگ = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ڳنڍ_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_پٺ(&relinkHandle);
        // Create sub function
        AstScope* ثابت scopep = m_cfuncp->scopep();
        ثابت string name = m_cfuncp->name() + "__deep" + cvt_منزل_Str(++m_deep_انگ);
        AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->جوڙيو_Stmtsp(nodep);
        scopep->جوڙيو_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ثابت callp = جديد AstCCall{nodep->fileline(), funcp};
        callp->dtype_سيٽ_Void();
        إذا (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //پورو
        أرجع funcp;
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_انگ = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_اونهائي);
        VL_RESTORER(m_cfuncp);
        m_اونهائي = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_Stmt_اظهار* nodep) override {}  // Stop recursion after introducing new function
    فراغ گهمو(Ast_Jump_بلاڪ*) override {}  // Stop recursion as can't break up across a jump
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        ++m_اونهائي;
        إذا (m_اونهائي > v3Global.opt.comp_حد_بندي_بلاڪون()) {  // Already done
            UINFO(4, "DeepBlocks " << m_اونهائي << " " << nodep);
            ثابت Ast_ڳنڍ* ثابت backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ثابت funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } وإلا {
            iterateChildren(nodep);
        }
        --m_اونهائي;
    }

    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}  // Accelerate
    //--------------------
    فراغ گهمو(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح اونهائي_بلاڪ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~اونهائي_بلاڪ_Visitor() override = افتراضي;
};

//######################################################################
// DepthBlock class functions

فراغ V3_اونهائي_بلاڪ::اونهائي_بلاڪ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { اونهائي_بلاڪ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("deepblock", 0, dump_وڻ_Either_Level() >= 3);
}
