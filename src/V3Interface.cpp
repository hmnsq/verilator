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
    अचर VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    शून्य visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->शीर्ष_Modulep()); }
    शून्य visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        यदि (m_scope.empty()) {
            m_scope = nodep->name();
        } अन्यथा {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_गाँठो_Module* अचर modp = nodep->modp();
        // Pass Cell pointers down to the next module
        लागि (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* अचर varp = pinp->modVarp();
            अचर AstVarRef* अचर varrefp = VN_CAST(pinp->exprp(), VarRef);
            यदि (!varrefp) जारी;

            अचर AstVar* अचर fromVarp = varrefp->varp();
            अचर AstIfaceRefDType* अचर irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            यदि (!irdtp) जारी;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            यदि (!cellp) cellp = irdtp->cellp();
            यदि (!cellp) जारी;
            varp->user1p(cellp);
            अचर string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            अचर AstIfaceRefDType* अचर portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            अचर string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_पङ्क्ति* अचर flp = pinp->fileline();
            cellp->addIntfRefsp(नूतन AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    शून्य visit(Ast_गाँठो_अभिव्यक्ति*) override {}  // Accelerate
    शून्य visit(Ast_गाँठो_Stmt*) override {}  // Accelerate
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_गाँठो* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = स्वभाव;
};

//######################################################################
// Interface class functions

शून्य V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_रूख("interface", 0, dump_रूख_Either_Level() >= 3);
}
