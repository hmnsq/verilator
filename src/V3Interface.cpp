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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

صنف InlineIntfRefVisitor final : عام VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    ثابت VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    فراغ visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->topModulep()); }
    فراغ visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        𐡄𐡍 (m_scope.empty()) {
            m_scope = nodep->name();
        } وإلا {
            m_scope += "__DOT__" + nodep->name();
        }

        AstNodeModule* ثابت modp = nodep->modp();
        // Pass Cell pointers down to the next module
        لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ثابت varp = pinp->modVarp();
            ثابت AstVarRef* ثابت varrefp = VN_CAST(pinp->exprp(), VarRef);
            𐡄𐡍 (!varrefp) استمر;

            ثابت AstVar* ثابت fromVarp = varrefp->varp();
            ثابت AstIfaceRefDType* ثابت irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            𐡄𐡍 (!irdtp) استمر;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            𐡄𐡍 (!cellp) cellp = irdtp->cellp();
            𐡄𐡍 (!cellp) استمر;
            varp->user1p(cellp);
            ثابت string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ثابت AstIfaceRefDType* ثابت portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ثابت string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            FileLine* ثابت flp = pinp->fileline();
            cellp->addIntfRefsp(جديد AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    فراغ visit(AstNodeExpr*) override {}  // Accelerate
    فراغ visit(AstNodeStmt*) override {}  // Accelerate
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(AstNode* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = افتراضي;
};

//######################################################################
// Interface class functions

فراغ V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dumpCheckGlobalTree("interface", 0, dumpTreeEitherLevel() >= 3);
}
