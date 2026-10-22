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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Branch.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Branch state, as a visitor of each AstNode

رده BranchVisitor final : عمومی VNVisitorConst {
    // STATE - for current visit position (use VL_RESTORER)
    صحیح m_unlikely = 0;  // Excuses for branch likely not taken

    // VISITORS
    تهی visit(Ast_گره_If* nodep) override {
        UINFO(4, " IF: " << nodep);
        VL_RESTORER(m_unlikely);
        // Do then
        m_unlikely = 0;
        iterateAndNextConstNull(nodep->thensp());
        ثابت صحیح thenUnlikely = m_unlikely;
        // Do else
        m_unlikely = 0;
        iterateAndNextConstNull(nodep->elsesp());
        ثابت صحیح elseUnlikely = m_unlikely;
        // Compute
        اگر (elseUnlikely > thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_LIKELY);
        } وگرنه اگر (elseUnlikely < thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_UNLIKELY);
        }  // else leave unknown
    }

    تهی visit(Ast_گره* nodep) override {
        اگر (nodep->isUnlikely()) {
            UINFO(4, "  UNLIKELY: " << nodep);
            ++m_unlikely;
        }
        iterateChildrenConst(nodep);
    }

عمومی:
    // CONSTRUCTORS
    explicit BranchVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~BranchVisitor() override = پیشفرض;
};

//######################################################################
// Branch class functions

تهی V3Branch::branchAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { BranchVisitor{nodep}; }
    V3Global::dump_Check_Global_درخت("branch", 0, dump_درخت_Either_Level() >= 3);
}
