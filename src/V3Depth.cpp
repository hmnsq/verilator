// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prevent very deep expressions
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
// V3Depth's Transformations:
//
// Each module:
//      For each wide OP, assign a temporary variable.
//      For each deep expression, assign expression to temporary.
// Each CFunc:
//      Any statements that need "this" are marked non-static
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Depth.h"

#ضمن "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف عمق_Visitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = اشاره_تهی;  // Current block
    Ast_گره* m_stmtp = اشاره_تهی;  // Current statement
    صحيح m_عمق = 0;  // How deep in an expression
    صحيح m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_نامها m_موقت_نامها;  // For generating unique temporary variable names

    // METHODS

    فراغ create_Deep_موقت(Ast_گره_عبارت* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_موقت_نامها.get(nodep), nodep->dtypep()};
        إذا (m_cfuncp) {
            m_cfuncp->جمع_کن_Varsp(varp);
        } وإلا {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ثابت newp = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_تخصیص_بده* ثابت assp = جديد Ast_تخصیص_بده{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->جمع_کن_اینجا_This_As_بعدی(assp);
    }

    // VISITORS
    فراغ بازدید(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_عمق);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_عمق = 0;
        m_maxdepth = 0;
        m_موقت_نامها.reset();
        iterateChildren(nodep);
    }
    فراغ بازدید_Stmt(Ast_گره_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_عمق);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_عمق = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    فراغ بازدید(Ast_گره_Stmt* nodep) override { بازدید_Stmt(nodep); }
    فراغ بازدید(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    فراغ بازدید(Ast_گره_Termop* nodep) override {}
    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_عمق);
            m_عمق += 2;
            إذا (m_عمق > m_maxdepth) m_maxdepth = m_عمق;
            iterateChildren(nodep);
        }
        إذا (m_stmtp && (v3Global.opt.comp_حد_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_عمق) > v3Global.opt.comp_حد_Parens()
            && !VN_IS(nodep->backp(), گره_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_عمق;
            create_Deep_موقت(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    فراغ needNonStaticFunc(Ast_گره* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        إذا (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(خطأ);
        }
    }
    فراغ بازدید(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    فراغ بازدید(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        بازدید_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    فراغ بازدید(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ بازدید(Ast_گره* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    صریح عمق_Visitor(AstNetlist* nodep)
        : m_موقت_نامها{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_درخت();
    }
    ~عمق_Visitor() override = افتراضي;
};

//######################################################################
// Depth class functions

فراغ V3_عمق::عمق_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { عمق_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("depth", 0, dump_درخت_Either_Level() >= 6);
}
