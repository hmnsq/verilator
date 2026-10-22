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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Branch.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Branch state, as a visitor of each AstNode

वर्ग BranchVisitor final : सार्वजनिक VNVisitorConst {
    // STATE - for current visit position (use VL_RESTORER)
    पूर्णाङ्क m_unlikely = 0;  // Excuses for branch likely not taken

    // VISITORS
    शून्य visit(Ast_गाँठ_If* nodep) override {
        UINFO(4, " IF: " << nodep);
        VL_RESTORER(m_unlikely);
        // Do then
        m_unlikely = 0;
        iterateAndNextConstNull(nodep->thensp());
        ध्रुव पूर्णाङ्क thenUnlikely = m_unlikely;
        // Do else
        m_unlikely = 0;
        iterateAndNextConstNull(nodep->elsesp());
        ध्रुव पूर्णाङ्क elseUnlikely = m_unlikely;
        // Compute
        यदि (elseUnlikely > thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_LIKELY);
        } अन्यथा यदि (elseUnlikely < thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_UNLIKELY);
        }  // else leave unknown
    }

    शून्य visit(Ast_गाँठ* nodep) override {
        यदि (nodep->isUnlikely()) {
            UINFO(4, "  UNLIKELY: " << nodep);
            ++m_unlikely;
        }
        iterateChildrenConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit BranchVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~BranchVisitor() override = स्वभाव;
};

//######################################################################
// Branch class functions

शून्य V3Branch::branchAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { BranchVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("branch", 0, dump_वृक्ष_Either_Level() >= 3);
}
