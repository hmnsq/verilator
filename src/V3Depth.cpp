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

صنف DepthVisitor final : عام VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    Ast_گرێ* m_stmtp = nullptr;  // Current statement
    صحيح m_depth = 0;  // How deep in an expression
    صحيح m_maxdepth = 0;  // Maximum depth in an expression
    V3UniqueNames m_tempNames;  // For generating unique temporary variable names

    // METHODS

    فراغ createDeepTemp(Ast_گرێ_دەربڕین* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ثابت varp = جديد AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_tempNames.get(nodep), nodep->dtypep()};
        إذا (m_cfuncp) {
            m_cfuncp->addVarsp(varp);
        } وإلا {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ثابت newp = جديد AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        AstAssign* ثابت assp = جديد AstAssign{
            nodep->fileline(), جديد AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->addHereThisAsNext(assp);
    }

    // VISITORS
    فراغ visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_depth);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_depth = 0;
        m_maxdepth = 0;
        m_tempNames.reset();
        iterateChildren(nodep);
    }
    فراغ visitStmt(Ast_گرێ_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_depth);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_depth = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    فراغ visit(Ast_گرێ_Stmt* nodep) override { visitStmt(nodep); }
    فراغ visit(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    فراغ visit(Ast_گرێ_Termop* nodep) override {}
    فراغ visit(Ast_گرێ_دەربڕین* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_depth);
            m_depth += 2;
            إذا (m_depth > m_maxdepth) m_maxdepth = m_depth;
            iterateChildren(nodep);
        }
        إذا (m_stmtp && (v3Global.opt.compLimitParens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_depth) > v3Global.opt.compLimitParens()
            && !VN_IS(nodep->backp(), گرێ_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_depth;
            createDeepTemp(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    فراغ needNonStaticFunc(Ast_گرێ* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        إذا (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(خطأ);
        }
    }
    فراغ visit(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    فراغ visit(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        visitStmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    فراغ visit(AstVar*) override {}  // Don't hit varrefs under vars
    فراغ visit(Ast_گرێ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit DepthVisitor(AstNetlist* nodep)
        : m_tempNames{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_درەخت();
    }
    ~DepthVisitor() override = افتراضي;
};

//######################################################################
// Depth class functions

فراغ V3Depth::depthAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درەخت("depth", 0, dump_درەخت_Either_Level() >= 6);
}
