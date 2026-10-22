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

صنف ڈونگھائی_Visitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = خالی_سوچک;  // Current block
    Ast_گرہ* m_stmtp = خالی_سوچک;  // Current statement
    صحيح m_ڈونگھائی = 0;  // How deep in an expression
    صحيح m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_ناں m_عارضی_ناں;  // For generating unique temporary variable names

    // METHODS

    فراغ create_Deep_عارضی(Ast_گرہ_اظہاریہ* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_عارضی_ناں.get(nodep), nodep->dtypep()};
        إذا (m_cfuncp) {
            m_cfuncp->جوڑو_Varsp(varp);
        } وإلا {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ثابت newp = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_سونپو* ثابت assp = جديد Ast_سونپو{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->جوڑو_ایتھے_This_As_اگلا(assp);
    }

    // VISITORS
    فراغ ویکھو(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_ڈونگھائی);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_ڈونگھائی = 0;
        m_maxdepth = 0;
        m_عارضی_ناں.reset();
        iterateChildren(nodep);
    }
    فراغ ویکھو_Stmt(Ast_گرہ_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_ڈونگھائی);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_ڈونگھائی = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    فراغ ویکھو(Ast_گرہ_Stmt* nodep) override { ویکھو_Stmt(nodep); }
    فراغ ویکھو(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    فراغ ویکھو(Ast_گرہ_Termop* nodep) override {}
    فراغ ویکھو(Ast_گرہ_اظہاریہ* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_ڈونگھائی);
            m_ڈونگھائی += 2;
            إذا (m_ڈونگھائی > m_maxdepth) m_maxdepth = m_ڈونگھائی;
            iterateChildren(nodep);
        }
        إذا (m_stmtp && (v3Global.opt.comp_حد_بندی_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_ڈونگھائی) > v3Global.opt.comp_حد_بندی_Parens()
            && !VN_IS(nodep->backp(), گرہ_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_ڈونگھائی;
            create_Deep_عارضی(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    فراغ needNonStaticFunc(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        إذا (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(خطأ);
        }
    }
    فراغ ویکھو(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    فراغ ویکھو(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        ویکھو_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    فراغ ویکھو(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ ویکھو(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح ڈونگھائی_Visitor(AstNetlist* nodep)
        : m_عارضی_ناں{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_درخت();
    }
    ~ڈونگھائی_Visitor() override = افتراضي;
};

//######################################################################
// Depth class functions

فراغ V3_ڈونگھائی::ڈونگھائی_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ڈونگھائی_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("depth", 0, dump_درخت_Either_Level() >= 6);
}
