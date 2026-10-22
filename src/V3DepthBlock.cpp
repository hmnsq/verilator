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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3DepthBlock.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ ਡੂੰਘਾਈ_ਖੰਡ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    ਅਚਲ Ast_ਗੰਢ_Module* m_modp = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current function
    ਪੂਰਨ m_ਡੂੰਘਾਈ = 0;  // How deep in an expression
    ਪੂਰਨ m_deep_ਅੰਕ = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_ਗੰਢ_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_ਪਿੱਠ(&relinkHandle);
        // Create sub function
        AstScope* ਅਚਲ scopep = m_cfuncp->scopep();
        ਅਚਲ string name = m_cfuncp->name() + "__deep" + cvt_ਮੰਜ਼ਿਲ_Str(++m_deep_ਅੰਕ);
        AstCFunc* ਅਚਲ funcp = ਨਵਾਂ AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->ਜੋੜੋ_Stmtsp(nodep);
        scopep->ਜੋੜੋ_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* ਅਚਲ callp = ਨਵਾਂ AstCCall{nodep->fileline(), funcp};
        callp->dtype_ਸੈੱਟ_Void();
        ਜੇ (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //ਪੂਰਾ
        ਵਾਪਸ funcp;
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_ਅੰਕ = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_ਡੂੰਘਾਈ);
        VL_RESTORER(m_cfuncp);
        m_ਡੂੰਘਾਈ = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {}  // Stop recursion after introducing new function
    ਖਾਲੀ ਦੇਖੋ(Ast_Jump_ਖੰਡ*) override {}  // Stop recursion as can't break up across a jump
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        ++m_ਡੂੰਘਾਈ;
        ਜੇ (m_ਡੂੰਘਾਈ > v3Global.opt.comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ()) {  // Already done
            UINFO(4, "DeepBlocks " << m_ਡੂੰਘਾਈ << " " << nodep);
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* ਅਚਲ funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } ਨਹੀਂਤਾਂ {
            iterateChildren(nodep);
        }
        --m_ਡੂੰਘਾਈ;
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ*) override {}  // Accelerate
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Don't hit varrefs under vars
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਡੂੰਘਾਈ_ਖੰਡ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਡੂੰਘਾਈ_ਖੰਡ_Visitor() override = ਮੂਲ;
};

//######################################################################
// DepthBlock class functions

ਖਾਲੀ V3_ਡੂੰਘਾਈ_ਖੰਡ::ਡੂੰਘਾਈ_ਖੰਡ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਡੂੰਘਾਈ_ਖੰਡ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("deepblock", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
