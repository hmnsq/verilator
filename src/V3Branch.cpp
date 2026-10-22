// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Branch prediction
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
// BRANCH TRANSFORMATIONS:
//      At each IF/(IF else).
//         Count underneath $display/$stop statements.
//         If more on if than else, this branch is unlikely, or vice-versa.
//      At each FTASKREF,
//         Count calls into the function
//      Then, if FTASK is called only once, add inline attribute
//
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Branch.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Branch state, as a visitor of each AstNode

తరగతి BranchVisitor final : బహిరంగ VNVisitorConst {
    // STATE - for current visit position (use VL_RESTORER)
    పూర్ణాంకం m_unlikely = 0;  // Excuses for branch likely not taken

    // VISITORS
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        UINFO(4, " IF: " << nodep);
        VL_RESTORER(m_unlikely);
        // Do then
        m_unlikely = 0;
        iterate_And_తదుపరి_Const_Null(nodep->thensp());
        స్థిరరాశి పూర్ణాంకం thenUnlikely = m_unlikely;
        // Do else
        m_unlikely = 0;
        iterate_And_తదుపరి_Const_Null(nodep->elsesp());
        స్థిరరాశి పూర్ణాంకం elseUnlikely = m_unlikely;
        // Compute
        ఒకవేళ (elseUnlikely > thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_LIKELY);
        } లేకపోతే ఒకవేళ (elseUnlikely < thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_UNLIKELY);
        }  // else leave unknown
    }

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        ఒకవేళ (nodep->isUnlikely()) {
            UINFO(4, "  UNLIKELY: " << nodep);
            ++m_unlikely;
        }
        iterateChildrenConst(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన BranchVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~BranchVisitor() override = అప్రమేయ;
};

//######################################################################
// Branch class functions

శూన్య V3Branch::branchAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { BranchVisitor{nodep}; }
    V3Global::dump_Check_Global_వృక్షం("branch", 0, dump_వృక్షం_Either_Level() >= 3);
}
