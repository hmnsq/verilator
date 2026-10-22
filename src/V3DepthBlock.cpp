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

صنف عمق_بلوک_Visitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ثابت Ast_گره_Module* m_modp = اشاره_تهی;  // Current module
    ثابت AstCFunc* m_cfuncp = اشاره_تهی;  // Current function
    صحيح m_عمق = 0;  // How deep in an expression
    صحيح m_deep_رقم = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_گره_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_پشت(&relinkHandle);
        // Create sub function
        AstScope* ثابت scopep = m_cfuncp->scopep();
        ثابت string name = m_cfuncp->name() + "__deep" + cvt_مقصد_Str(++m_deep_رقم);
        AstCFunc* ثابت funcp = جديد AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->جمع_کن_Stmtsp(nodep);
        scopep->جمع_کن_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ثابت callp = جديد AstCCall{nodep->fileline(), funcp};
        callp->dtype_مجموعه_Void();
        إذا (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //انجامشده
        أرجع funcp;
    }

    // VISITORS
    فراغ بازدید(Ast_گره_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_رقم = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_عمق);
        VL_RESTORER(m_cfuncp);
        m_عمق = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {}  // Stop recursion after introducing new function
    فراغ بازدید(Ast_Jump_بلوک*) override {}  // Stop recursion as can't break up across a jump
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        ++m_عمق;
        إذا (m_عمق > v3Global.opt.comp_حد_بلوکها()) {  // Already done
            UINFO(4, "DeepBlocks " << m_عمق << " " << nodep);
            ثابت Ast_گره* ثابت backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ثابت funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } وإلا {
            iterateChildren(nodep);
        }
        --m_عمق;
    }

    فراغ بازدید(Ast_گره_عبارت*) override {}  // Accelerate
    //--------------------
    فراغ بازدید(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح عمق_بلوک_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~عمق_بلوک_Visitor() override = افتراضي;
};

//######################################################################
// DepthBlock class functions

فراغ V3_عمق_بلوک::عمق_بلوک_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { عمق_بلوک_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("deepblock", 0, dump_درخت_Either_Level() >= 3);
}
