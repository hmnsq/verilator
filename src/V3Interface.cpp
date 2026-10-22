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
    ثابت VNUser1_اندريون_Use m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override { iterateChildrenConst(nodep->چوٽي_Modulep()); }
    فراغ گهمو(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        إذا (m_scope.خالي_جايون()) {
            m_scope = nodep->name();
        } وإلا {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_ڳنڍ_Module* ثابت modp = nodep->modp();
        // Pass Cell pointers down to the next module
        لكل (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ثابت varp = pinp->modVarp();
            ثابت AstVarRef* ثابت varrefp = VN_CAST(pinp->exprp(), VarRef);
            إذا (!varrefp) استمر;

            ثابت AstVar* ثابت fromVarp = varrefp->varp();
            ثابت AstIfaceRefDType* ثابت irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            إذا (!irdtp) استمر;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            إذا (!cellp) cellp = irdtp->cellp();
            إذا (!cellp) استمر;
            varp->user1p(cellp);
            ثابت string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ثابت AstIfaceRefDType* ثابت پورٽ_Irdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ثابت string modportName = پورٽ_Irdtp ? پورٽ_Irdtp->modportName() : irdtp->modportName();
            File_سٽ* ثابت flp = pinp->fileline();
            cellp->جوڙيو_Intf_Refsp(جديد AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    فراغ گهمو(Ast_ڳنڍ_اظهار*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ_Stmt*) override {}  // Accelerate
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    // CONSTRUCTORS
    واضح InlineIntfRefVisitor(Ast_ڳنڍ* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = افتراضي;
};

//######################################################################
// Interface class functions

فراغ V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_وڻ("interface", 0, dump_وڻ_Either_Level() >= 3);
}
