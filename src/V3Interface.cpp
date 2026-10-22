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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

വർഗ്ഗം InlineIntfRefVisitor final : പൊതു VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    സ്ഥിരം VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    ശൂന്യം visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->മുകൾ_Modulep()); }
    ശൂന്യം visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        എങ്കിൽ (m_scope.empty()) {
            m_scope = nodep->name();
        } അല്ലെങ്കിൽ {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_കണ്ണി_Module* സ്ഥിരം modp = nodep->modp();
        // Pass Cell pointers down to the next module
        വേണ്ടി (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* സ്ഥിരം varp = pinp->modVarp();
            സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(pinp->exprp(), VarRef);
            എങ്കിൽ (!varrefp) തുടരുക;

            സ്ഥിരം AstVar* സ്ഥിരം fromVarp = varrefp->varp();
            സ്ഥിരം AstIfaceRefDType* സ്ഥിരം irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            എങ്കിൽ (!irdtp) തുടരുക;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            എങ്കിൽ (!cellp) cellp = irdtp->cellp();
            എങ്കിൽ (!cellp) തുടരുക;
            varp->user1p(cellp);
            സ്ഥിരം string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            സ്ഥിരം AstIfaceRefDType* സ്ഥിരം portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            സ്ഥിരം string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_വരി* സ്ഥിരം flp = pinp->fileline();
            cellp->addIntfRefsp(പുതിയ AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി_Stmt*) override {}  // Accelerate
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

പൊതു:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_കണ്ണി* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Interface class functions

ശൂന്യം V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_വൃക്ഷം("interface", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
