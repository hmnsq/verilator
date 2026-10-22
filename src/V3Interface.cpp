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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

श्रेणी InlineIntfRefVisitor final : खुला VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    स्थिर VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    व्योम visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->शीर्ष_Modulep()); }
    व्योम visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        अगर (m_scope.empty()) {
            m_scope = nodep->name();
        } अन्यथा {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_गाँठ_Module* स्थिर modp = nodep->modp();
        // Pass Cell pointers down to the next module
        क्रम (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* स्थिर varp = pinp->modVarp();
            स्थिर AstVarRef* स्थिर varrefp = VN_CAST(pinp->exprp(), VarRef);
            अगर (!varrefp) जारी;

            स्थिर AstVar* स्थिर fromVarp = varrefp->varp();
            स्थिर AstIfaceRefDType* स्थिर irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            अगर (!irdtp) जारी;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            अगर (!cellp) cellp = irdtp->cellp();
            अगर (!cellp) जारी;
            varp->user1p(cellp);
            स्थिर string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            स्थिर AstIfaceRefDType* स्थिर portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            स्थिर string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_पंक्ति* स्थिर flp = pinp->fileline();
            cellp->addIntfRefsp(नया AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    व्योम visit(Ast_गाँठ_व्यंजक*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ_Stmt*) override {}  // Accelerate
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

खुला:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_गाँठ* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = शेष;
};

//######################################################################
// Interface class functions

व्योम V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_वृक्ष("interface", 0, dump_वृक्ष_Either_Level() >= 3);
}
