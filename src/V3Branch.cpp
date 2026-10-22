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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Branch.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Branch state, as a visitor of each AstNode

श्रेणी BranchVisitor final : खुला VNVisitorConst {
    // STATE - for current visit position (use VL_RESTORER)
    पूर्णांक m_unlikely = 0;  // Excuses for branch likely not taken

    // VISITORS
    व्योम visit(Ast_गाँठ_If* nodep) override {
        UINFO(4, " IF: " << nodep);
        VL_RESTORER(m_unlikely);
        // Do then
        m_unlikely = 0;
        iterate_And_अग्र_Const_Null(nodep->thensp());
        स्थिर पूर्णांक thenUnlikely = m_unlikely;
        // Do else
        m_unlikely = 0;
        iterate_And_अग्र_Const_Null(nodep->elsesp());
        स्थिर पूर्णांक elseUnlikely = m_unlikely;
        // Compute
        अगर (elseUnlikely > thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_LIKELY);
        } अन्यथा अगर (elseUnlikely < thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_UNLIKELY);
        }  // else leave unknown
    }

    व्योम visit(Ast_गाँठ* nodep) override {
        अगर (nodep->isUnlikely()) {
            UINFO(4, "  UNLIKELY: " << nodep);
            ++m_unlikely;
        }
        iterateChildrenConst(nodep);
    }

खुला:
    // CONSTRUCTORS
    explicit BranchVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~BranchVisitor() override = शेष;
};

//######################################################################
// Branch class functions

व्योम V3Branch::branchAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { BranchVisitor{nodep}; }
    V3Global::dump_Check_Global_वृक्ष("branch", 0, dump_वृक्ष_Either_Level() >= 3);
}
