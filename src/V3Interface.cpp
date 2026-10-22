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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

শ্রেণী InlineIntfRefVisitor final : সর্বজনীন VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    ধ্রুবক VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    শূন্য visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->শীর্ষ_Modulep()); }
    শূন্য visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        যদি (m_scope.empty()) {
            m_scope = nodep->name();
        } অন্যথা {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_গ্রন্থি_Module* ধ্রুবক modp = nodep->modp();
        // Pass Cell pointers down to the next module
        জন্য (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ধ্রুবক varp = pinp->modVarp();
            ধ্রুবক AstVarRef* ধ্রুবক varrefp = VN_CAST(pinp->exprp(), VarRef);
            যদি (!varrefp) চালু;

            ধ্রুবক AstVar* ধ্রুবক fromVarp = varrefp->varp();
            ধ্রুবক AstIfaceRefDType* ধ্রুবক irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            যদি (!irdtp) চালু;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            যদি (!cellp) cellp = irdtp->cellp();
            যদি (!cellp) চালু;
            varp->user1p(cellp);
            ধ্রুবক string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ধ্রুবক AstIfaceRefDType* ধ্রুবক portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ধ্রুবক string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_সারি* ধ্রুবক flp = pinp->fileline();
            cellp->addIntfRefsp(নতুন AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    শূন্য visit(Ast_গ্রন্থি_রাশিমালা*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি_Stmt*) override {}  // Accelerate
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_গ্রন্থি* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = পূর্বনির্ধারিত;
};

//######################################################################
// Interface class functions

শূন্য V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_বৃক্ষ("interface", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
