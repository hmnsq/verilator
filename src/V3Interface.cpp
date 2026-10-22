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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Interface.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Track interface references under the Cell they reference

رده InlineIntfRefVisitor final : عمومی VNVisitor {
    // NODE STATE
    //   AstVar::user1p()   // AstCell which this Var points to
    ثابت VNUser1InUse m_inuser1;

    string m_scope;  // Scope name

    // VISITORS
    تهی visit(AstNetlist* nodep) override { iterateChildrenConst(nodep->بالاترین_Modulep()); }
    تهی visit(AstCell* nodep) override {
        VL_RESTORER_COPY(m_scope);
        اگر (m_scope.empty()) {
            m_scope = nodep->name();
        } وگرنه {
            m_scope += "__DOT__" + nodep->name();
        }

        Ast_گره_Module* ثابت modp = nodep->modp();
        // Pass Cell pointers down to the next module
        برای (AstPin* pinp = nodep->pinsp(); pinp; pinp = VN_AS(pinp->nextp(), Pin)) {
            AstVar* ثابت varp = pinp->modVarp();
            ثابت AstVarRef* ثابت varrefp = VN_CAST(pinp->exprp(), VarRef);
            اگر (!varrefp) ادامه;

            ثابت AstVar* ثابت fromVarp = varrefp->varp();
            ثابت AstIfaceRefDType* ثابت irdtp = VN_CAST(fromVarp->dtypep(), IfaceRefDType);
            اگر (!irdtp) ادامه;

            AstCell* cellp = VN_CAST(fromVarp->user1p(), Cell);
            اگر (!cellp) cellp = irdtp->cellp();
            اگر (!cellp) ادامه;
            varp->user1p(cellp);
            ثابت string alias = m_scope + "__DOT__" + pinp->name();
            // Prefer the port's own dtype; the source may have no modport
            ثابت AstIfaceRefDType* ثابت portIrdtp = VN_CAST(varp->dtypep(), IfaceRefDType);
            ثابت string modportName = portIrdtp ? portIrdtp->modportName() : irdtp->modportName();
            File_سطر* ثابت flp = pinp->fileline();
            cellp->addIntfRefsp(جدید AstIntfRef{flp, alias, m_scope, pinp->name(), modportName});
        }

        iterateChildrenConst(modp);
    }
    //--------------------
    تهی visit(Ast_گره_عبارت*) override {}  // Accelerate
    تهی visit(Ast_گره_Stmt*) override {}  // Accelerate
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

عمومی:
    // CONSTRUCTORS
    explicit InlineIntfRefVisitor(Ast_گره* nodep) { iterateConst(nodep); }
    ~InlineIntfRefVisitor() override = پیشفرض;
};

//######################################################################
// Interface class functions

تهی V3Interface::interfaceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");

    { InlineIntfRefVisitor{nodep}; }

    V3Global::dump_Check_Global_درخت("interface", 0, dump_درخت_Either_Level() >= 3);
}
