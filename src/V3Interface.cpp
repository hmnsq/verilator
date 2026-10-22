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
    ಧ್ರುವ VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->ತುದಿ_Modulep()); }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        ಯದಿ (m_scope.empty()) {
            m_scope = nodep->name();
        } ಅನ್ಯಥಾ {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_ಗಂಟು_Module* ಧ್ರುವ modp = nodep->modp();
        // Pass Cell pointers down to the next module
        ಪ್ರತಿ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ಧ್ರುವ varp = pinp->modVarp();
            ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(pinp->exprp(), VarRef);
            ಯದಿ (!varrefp) ಅನುವರ್ತ;

            ಧ್ರುವ AstVar* ಧ್ರುವ fromVarp = varrefp->varp();
            ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            ಯದಿ (!irdtp) ಅನುವರ್ತ;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            ಯದಿ (!cellp) cellp = irdtp->cellp();
            ಯದಿ (!cellp) ಅನುವರ್ತ;
            varp->user1p(cellp);
            ಧ್ರುವ string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ಧ್ರುವ string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_ಸಾಲು* ಧ್ರುವ flp = pinp->fileline();
            cellp->addIntfRefsp(ನೂತನ AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_ಗಂಟು* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Interface class functions

ಶೂನ್ಯ V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_ವೃಕ್ಷ("interface", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
