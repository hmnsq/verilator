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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Depth.h"

#شامل "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

جماعت گہرائی_Visitor final : عوامی VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = خالی_اشاریہ;  // Current block
    Ast_گرہ* m_stmtp = خالی_اشاریہ;  // Current statement
    صحیح m_گہرائی = 0;  // How deep in an expression
    صحیح m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_نام m_عارضی_نام;  // For generating unique temporary variable names

    // METHODS

    خالی create_Deep_عارضی(Ast_گرہ_اظہاریہ* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ثابت varp = نیا AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_عارضی_نام.get(nodep), nodep->dtypep()};
        اگر (m_cfuncp) {
            m_cfuncp->جوڑو_Varsp(varp);
        } ورنہ {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ثابت newp = نیا AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_تفویض* ثابت assp = نیا Ast_تفویض{
            nodep->fileline(), نیا AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->جوڑو_یہاں_This_As_اگلا(assp);
    }

    // VISITORS
    خالی دورہ(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_گہرائی);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_گہرائی = 0;
        m_maxdepth = 0;
        m_عارضی_نام.reset();
        iterateChildren(nodep);
    }
    خالی دورہ_Stmt(Ast_گرہ_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_گہرائی);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_گہرائی = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    خالی دورہ(Ast_گرہ_Stmt* nodep) override { دورہ_Stmt(nodep); }
    خالی دورہ(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    خالی دورہ(Ast_گرہ_Termop* nodep) override {}
    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_گہرائی);
            m_گہرائی += 2;
            اگر (m_گہرائی > m_maxdepth) m_maxdepth = m_گہرائی;
            iterateChildren(nodep);
        }
        اگر (m_stmtp && (v3Global.opt.comp_حد_بندی_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_گہرائی) > v3Global.opt.comp_حد_بندی_Parens()
            && !VN_IS(nodep->backp(), گرہ_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_گہرائی;
            create_Deep_عارضی(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    خالی needNonStaticFunc(Ast_گرہ* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        اگر (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(جھوٹ);
        }
    }
    خالی دورہ(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        دورہ_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    خالی دورہ(AstVar*) override {}  // Don't hit varrefs under vars
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildren(nodep); }

عوامی:
    // CONSTRUCTORS
    صریح گہرائی_Visitor(AstNetlist* nodep)
        : m_عارضی_نام{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_درخت();
    }
    ~گہرائی_Visitor() override = طےشدہ;
};

//######################################################################
// Depth class functions

خالی V3_گہرائی::گہرائی_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { گہرائی_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("depth", 0, dump_درخت_Either_Level() >= 6);
}
