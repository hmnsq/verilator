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

صنف اونهائي_Visitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = خالي_اشارو;  // Current block
    Ast_ڳنڍ* m_stmtp = خالي_اشارو;  // Current statement
    صحيح m_اونهائي = 0;  // How deep in an expression
    صحيح m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_نالا m_عارضي_نالا;  // For generating unique temporary variable names

    // METHODS

    فراغ create_Deep_عارضي(Ast_ڳنڍ_اظهار* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_عارضي_نالا.get(nodep), nodep->dtypep()};
        إذا (m_cfuncp) {
            m_cfuncp->جوڙيو_Varsp(varp);
        } وإلا {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ثابت newp = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_سونپيو* ثابت assp = جديد Ast_سونپيو{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->جوڙيو_هتي_This_As_ايندڙ(assp);
    }

    // VISITORS
    فراغ گهمو(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_اونهائي);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_اونهائي = 0;
        m_maxdepth = 0;
        m_عارضي_نالا.reset();
        iterateChildren(nodep);
    }
    فراغ گهمو_Stmt(Ast_ڳنڍ_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_اونهائي);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_اونهائي = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override { گهمو_Stmt(nodep); }
    فراغ گهمو(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    فراغ گهمو(Ast_ڳنڍ_Termop* nodep) override {}
    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_اونهائي);
            m_اونهائي += 2;
            إذا (m_اونهائي > m_maxdepth) m_maxdepth = m_اونهائي;
            iterateChildren(nodep);
        }
        إذا (m_stmtp && (v3Global.opt.comp_حد_بندي_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_اونهائي) > v3Global.opt.comp_حد_بندي_Parens()
            && !VN_IS(nodep->backp(), ڳنڍ_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_اونهائي;
            create_Deep_عارضي(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    فراغ needNonStaticFunc(Ast_ڳنڍ* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        إذا (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(خطأ);
        }
    }
    فراغ گهمو(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    فراغ گهمو(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        گهمو_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    فراغ گهمو(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    واضح اونهائي_Visitor(AstNetlist* nodep)
        : m_عارضي_نالا{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_وڻ();
    }
    ~اونهائي_Visitor() override = افتراضي;
};

//######################################################################
// Depth class functions

فراغ V3_اونهائي::اونهائي_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { اونهائي_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("depth", 0, dump_وڻ_Either_Level() >= 6);
}
