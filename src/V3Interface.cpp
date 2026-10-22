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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

ଶ୍ରେଣୀ InlineIntfRefVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    ଅଚଳ VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->ଶୀର୍ଷ_Modulep()); }
    ଶୂନ୍ୟ visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        ଯଦି (m_scope.empty()) {
            m_scope = nodep->name();
        } ଅନ୍ୟଥା {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = nodep->modp();
        // Pass Cell pointers down to the next module
        ପାଇଁ (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ଅଚଳ varp = pinp->modVarp();
            ଅଚଳ AstVarRef* ଅଚଳ varrefp = VN_CAST(pinp->exprp(), VarRef);
            ଯଦି (!varrefp) ଜାରି;

            ଅଚଳ AstVar* ଅଚଳ fromVarp = varrefp->varp();
            ଅଚଳ AstIfaceRefDType* ଅଚଳ irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            ଯଦି (!irdtp) ଜାରି;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            ଯଦି (!cellp) cellp = irdtp->cellp();
            ଯଦି (!cellp) ଜାରି;
            varp->user1p(cellp);
            ଅଚଳ string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ଅଚଳ AstIfaceRefDType* ଅଚଳ portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ଅଚଳ string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_ଧାଡ଼ି* ଅଚଳ flp = pinp->fileline();
            cellp->addIntfRefsp(ନୂତନ AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Stmt*) override {}  // Accelerate
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_ଗଣ୍ଠି* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// Interface class functions

ଶୂନ୍ୟ V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_ବୃକ୍ଷ("interface", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
