// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Interface references for tracing and VPI
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
// V3Interface's Transformations:
//
// Each module:
//      Look for CELL...
//          Keep track of scope and concrete interface along the way
//          Find all interface references
//              Add INTFREF to concrete interface's list of references
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

ವರ್ಗ InlineIntfRefVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override { iterateChildrenConst(nodep->ತುದಿ_Modulep()); }
    ಶೂನ್ಯ ಭೇಟಿ(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        ಆದರೆ (m_scope.ಖಾಲಿಗಳು()) {
            m_scope = nodep->name();
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = nodep->modp();
        // Pass Cell pointers down to the next module
        ಗಾಗಿ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ಸ್ಥಿರಾಂಕ varp = pinp->modVarp();
            ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = VN_CAST(pinp->exprp(), VarRef);
            ಆದರೆ (!varrefp) ಮುಂದುವರಿ;

            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ fromVarp = varrefp->varp();
            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            ಆದರೆ (!irdtp) ಮುಂದುವರಿ;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            ಆದರೆ (!cellp) cellp = irdtp->cellp();
            ಆದರೆ (!cellp) ಮುಂದುವರಿ;
            varp->user1p(cellp);
            ಸ್ಥಿರಾಂಕ string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ಪೋರ್ಟ್_Irdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ಸ್ಥಿರಾಂಕ string modportName = ಪೋರ್ಟ್_Irdtp ? ಪೋರ್ಟ್_Irdtp->modportName() : irdtp->modportName();
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = pinp->fileline();
            cellp->ಕೂಡಿಸು_Intf_Refsp(ಹೊಸ AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ InlineIntfRefVisitor(Ast_ಗಂಟು* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Interface class functions

ಶೂನ್ಯ V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("interface", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
