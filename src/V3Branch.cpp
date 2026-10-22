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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Branch.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Branch state, as a visitor of each AstNode

વર્ગ BranchVisitor final : જાહેર VNVisitorConst {
    // STATE - for current visit position (use VL_RESTORER)
    પૂર્ણાંક m_unlikely = 0;  // Excuses for branch likely not taken

    // VISITORS
    શૂન્ય visit(Ast_ગાંઠ_If* nodep) override {
        UINFO(4, " IF: " << nodep);
        VL_RESTORER(m_unlikely);
        // Do then
        m_unlikely = 0;
        iterateAndNextConstNull(nodep->thensp());
        અચલ પૂર્ણાંક thenUnlikely = m_unlikely;
        // Do else
        m_unlikely = 0;
        iterateAndNextConstNull(nodep->elsesp());
        અચલ પૂર્ણાંક elseUnlikely = m_unlikely;
        // Compute
        જો (elseUnlikely > thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_LIKELY);
        } નહીંતર જો (elseUnlikely < thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_UNLIKELY);
        }  // else leave unknown
    }

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
        જો (nodep->isUnlikely()) {
            UINFO(4, "  UNLIKELY: " << nodep);
            ++m_unlikely;
        }
        iterateChildrenConst(nodep);
    }

જાહેર:
    // CONSTRUCTORS
    explicit BranchVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~BranchVisitor() override = મૂળભૂત;
};

//######################################################################
// Branch class functions

શૂન્ય V3Branch::branchAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { BranchVisitor{nodep}; }
    V3Global::dump_Check_Global_વૃક્ષ("branch", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
