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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3DepthBlock.h"

#అన్తర్భావ "V3EmitCBase.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి లోతు_ఖండం_Visitor final : బహిరంగ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    స్థిరరాశి Ast_కణుపు_Module* m_modp = శూన్యసూచిక;  // Current module
    స్థిరరాశి AstCFunc* m_cfuncp = శూన్యసూచిక;  // Current function
    పూర్ణాంకం m_లోతు = 0;  // How deep in an expression
    పూర్ణాంకం m_deep_అంకె = 0;  // How many functions made

    // METHODS

    AstCFunc* createDeepFunc(Ast_కణుపు_Stmt* nodep) {
        VNRelinker relinkHandle;
        nodep->unlink_Fr_వెనుక(&relinkHandle);
        // Create sub function
        AstScope* స్థిరరాశి scopep = m_cfuncp->scopep();
        స్థిరరాశి string name = m_cfuncp->name() + "__deep" + cvt_గమ్యం_Str(++m_deep_అంకె);
        AstCFunc* స్థిరరాశి funcp = కొత్త AstCFunc{nodep->fileline(), name, scopep};
        funcp->slow(m_cfuncp->slow());
        funcp->isStatic(m_cfuncp->isStatic());
        funcp->isLoose(m_cfuncp->isLoose());
        funcp->కూడు_Stmtsp(nodep);
        scopep->కూడు_Blocksp(funcp);
        // Call sub function at the point where the body was removed from
        AstCCall* స్థిరరాశి callp = కొత్త AstCCall{nodep->fileline(), funcp};
        callp->dtype_సమితి_Void();
        ఒకవేళ (VN_IS(m_modp, Class)) {
            funcp->argTypes(EmitCUtil::symClassVar());
            callp->argTypes("vlSymsp");
        }
        UINFO(6, "      New " << callp);
        relinkHandle.relink(callp->makeStmt());
        //పూర్తి
        తిరిగి funcp;
    }

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        UINFO(4, " MOD   " << nodep);
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_deep_అంకె = 0;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        // We recurse into this.
        VL_RESTORER(m_లోతు);
        VL_RESTORER(m_cfuncp);
        m_లోతు = 0;
        m_cfuncp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {}  // Stop recursion after introducing new function
    శూన్య సందర్శించు(Ast_Jump_ఖండం*) override {}  // Stop recursion as can't break up across a jump
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        ++m_లోతు;
        ఒకవేళ (m_లోతు > v3Global.opt.comp_హద్దు_ఖండాలు()) {  // Already done
            UINFO(4, "DeepBlocks " << m_లోతు << " " << nodep);
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();  // Only for debug
            UINFOTREE(9, backp, "", "pre ");
            AstCFunc* స్థిరరాశి funcp = createDeepFunc(nodep);
            iterate(funcp);
            UINFOTREE(9, backp, "", "post");
            UINFOTREE(9, funcp, "", "func");
        } లేకపోతే {
            iterateChildren(nodep);
        }
        --m_లోతు;
    }

    శూన్య సందర్శించు(Ast_కణుపు_సమాసం*) override {}  // Accelerate
    //--------------------
    శూన్య సందర్శించు(AstVar*) override {}  // Don't hit varrefs under vars
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన లోతు_ఖండం_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~లోతు_ఖండం_Visitor() override = అప్రమేయ;
};

//######################################################################
// DepthBlock class functions

శూన్య V3_లోతు_ఖండం::లోతు_ఖండం_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { లోతు_ఖండం_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("deepblock", 0, dump_వృక్షం_Either_Level() >= 3);
}
