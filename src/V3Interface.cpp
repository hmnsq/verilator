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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

વર્ગ InlineIntfRefVisitor final : જાહેર VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    અચલ VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->ટોચ_Modulep()); }
    શૂન્ય visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        જો (m_scope.empty()) {
            m_scope = nodep->name();
        } નહીંતર {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_ગાંઠ_Module* અચલ modp = nodep->modp();
        // Pass Cell pointers down to the next module
        માટે (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* અચલ varp = pinp->modVarp();
            અચલ AstVarRef* અચલ varrefp = VN_CAST(pinp->exprp(), VarRef);
            જો (!varrefp) ચાલુ;

            અચલ AstVar* અચલ fromVarp = varrefp->varp();
            અચલ AstIfaceRefDType* અચલ irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            જો (!irdtp) ચાલુ;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            જો (!cellp) cellp = irdtp->cellp();
            જો (!cellp) ચાલુ;
            varp->user1p(cellp);
            અચલ string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            અચલ AstIfaceRefDType* અચલ portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            અચલ string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_લીટી* અચલ flp = pinp->fileline();
            cellp->addIntfRefsp(નવું AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ_Stmt*) override {}  // Accelerate
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_ગાંઠ* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = મૂળભૂત;
};

//######################################################################
// Interface class functions

શૂન્ય V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_વૃક્ષ("interface", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
