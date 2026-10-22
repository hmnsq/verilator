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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

वर्ग InlineIntfRefVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    स्थिरांक VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->शीर्ष_Modulep()); }
    शून्य visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        जर (m_scope.empty()) {
            m_scope = nodep->name();
        } अन्यथा {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_गाठ_Module* स्थिरांक modp = nodep->modp();
        // Pass Cell pointers down to the next module
        साठी (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* स्थिरांक varp = pinp->modVarp();
            स्थिरांक AstVarRef* स्थिरांक varrefp = VN_CAST(pinp->exprp(), VarRef);
            जर (!varrefp) सुरूठेवा;

            स्थिरांक AstVar* स्थिरांक fromVarp = varrefp->varp();
            स्थिरांक AstIfaceRefDType* स्थिरांक irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            जर (!irdtp) सुरूठेवा;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            जर (!cellp) cellp = irdtp->cellp();
            जर (!cellp) सुरूठेवा;
            varp->user1p(cellp);
            स्थिरांक string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            स्थिरांक AstIfaceRefDType* स्थिरांक portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            स्थिरांक string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_ओळ* स्थिरांक flp = pinp->fileline();
            cellp->addIntfRefsp(नवीन AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    शून्य visit(Ast_गाठ_अभिव्यक्ती*) override {}  // Accelerate
    शून्य visit(Ast_गाठ_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_गाठ* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Interface class functions

शून्य V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_वृक्ष("interface", 0, dump_वृक्ष_Either_Level() >= 3);
}
