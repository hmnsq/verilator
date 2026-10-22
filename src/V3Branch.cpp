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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Branch.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Branch state, as a visitor of each AstNode

ವರ್ಗ BranchVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // STATE - for current visit position (use VL_RESTORER)
    ಪೂರ್ಣಾಂಕ m_unlikely = 0;  // Excuses for branch likely not taken

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        UINFO(4, " IF: " << nodep);
        VL_RESTORER(m_unlikely);
        // Do then
        m_unlikely = 0;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ thenUnlikely = m_unlikely;
        // Do else
        m_unlikely = 0;
        iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ elseUnlikely = m_unlikely;
        // Compute
        ಆದರೆ (elseUnlikely > thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_LIKELY);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (elseUnlikely < thenUnlikely) {
            nodep->branchPred(VBranchPred::BP_UNLIKELY);
        }  // else leave unknown
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (nodep->isUnlikely()) {
            UINFO(4, "  UNLIKELY: " << nodep);
            ++m_unlikely;
        }
        iterateChildrenConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ BranchVisitor(AstNetlist* nodep) { iterateChildrenConst(nodep); }
    ~BranchVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Branch class functions

ಶೂನ್ಯ V3Branch::branchAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { BranchVisitor{nodep}; }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("branch", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
