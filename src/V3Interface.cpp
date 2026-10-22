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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

வகுப்பு InlineIntfRefVisitor final : பொது VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    மாறிலி VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->உச்சி_Modulep()); }
    வெறுமை visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        எனில் (m_scope.empty()) {
            m_scope = nodep->name();
        } இல்லையெனில் {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_முனை_Module* மாறிலி modp = nodep->modp();
        // Pass Cell pointers down to the next module
        க்கு (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* மாறிலி varp = pinp->modVarp();
            மாறிலி AstVarRef* மாறிலி varrefp = VN_CAST(pinp->exprp(), VarRef);
            எனில் (!varrefp) தொடர்;

            மாறிலி AstVar* மாறிலி fromVarp = varrefp->varp();
            மாறிலி AstIfaceRefDType* மாறிலி irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            எனில் (!irdtp) தொடர்;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            எனில் (!cellp) cellp = irdtp->cellp();
            எனில் (!cellp) தொடர்;
            varp->user1p(cellp);
            மாறிலி string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            மாறிலி AstIfaceRefDType* மாறிலி portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            மாறிலி string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_வரி* மாறிலி flp = pinp->fileline();
            cellp->addIntfRefsp(புதிய AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    வெறுமை visit(Ast_முனை_கோவை*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை_Stmt*) override {}  // Accelerate
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

பொது:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_முனை* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = இயல்பு;
};

//######################################################################
// Interface class functions

வெறுமை V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_மரம்("interface", 0, dump_மரம்_Either_Level() >= 3);
}
