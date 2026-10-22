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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Depth.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ ਡੂੰਘਾਈ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = ਖਾਲੀ_ਸੂਚਕ;  // Current block
    Ast_ਗੰਢ* m_stmtp = ਖਾਲੀ_ਸੂਚਕ;  // Current statement
    ਪੂਰਨ m_ਡੂੰਘਾਈ = 0;  // How deep in an expression
    ਪੂਰਨ m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_ਨਾਂ m_ਆਰਜ਼ੀ_ਨਾਂ;  // For generating unique temporary variable names

    // METHODS

    ਖਾਲੀ create_Deep_ਆਰਜ਼ੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_ਆਰਜ਼ੀ_ਨਾਂ.get(nodep), nodep->dtypep()};
        ਜੇ (m_cfuncp) {
            m_cfuncp->ਜੋੜੋ_Varsp(varp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ਅਚਲ newp = ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_ਸੌਂਪੋ* ਅਚਲ assp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(), ਨਵਾਂ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(assp);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_ਡੂੰਘਾਈ);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_ਡੂੰਘਾਈ = 0;
        m_maxdepth = 0;
        m_ਆਰਜ਼ੀ_ਨਾਂ.reset();
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ_Stmt(Ast_ਗੰਢ_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_ਡੂੰਘਾਈ);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_ਡੂੰਘਾਈ = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override { ਦੇਖੋ_Stmt(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Termop* nodep) override {}
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_ਡੂੰਘਾਈ);
            m_ਡੂੰਘਾਈ += 2;
            ਜੇ (m_ਡੂੰਘਾਈ > m_maxdepth) m_maxdepth = m_ਡੂੰਘਾਈ;
            iterateChildren(nodep);
        }
        ਜੇ (m_stmtp && (v3Global.opt.comp_ਹੱਦਬੰਦੀ_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_ਡੂੰਘਾਈ) > v3Global.opt.comp_ਹੱਦਬੰਦੀ_Parens()
            && !VN_IS(nodep->backp(), ਗੰਢ_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_ਡੂੰਘਾਈ;
            create_Deep_ਆਰਜ਼ੀ(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    ਖਾਲੀ needNonStaticFunc(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        ਜੇ (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(ਝੂਠ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        ਦੇਖੋ_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Don't hit varrefs under vars
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਡੂੰਘਾਈ_Visitor(AstNetlist* nodep)
        : m_ਆਰਜ਼ੀ_ਨਾਂ{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_ਰੁੱਖ();
    }
    ~ਡੂੰਘਾਈ_Visitor() override = ਮੂਲ;
};

//######################################################################
// Depth class functions

ਖਾਲੀ V3_ਡੂੰਘਾਈ::ਡੂੰਘਾਈ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਡੂੰਘਾਈ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("depth", 0, dump_ਰੁੱਖ_Either_Level() >= 6);
}
