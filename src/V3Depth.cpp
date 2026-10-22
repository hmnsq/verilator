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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Depth.h"

#समावेश "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी DepthVisitor final : खुला VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = nullptr;  // Current block
    Ast_गाँठ* m_stmtp = nullptr;  // Current statement
    पूर्णांक m_depth = 0;  // How deep in an expression
    पूर्णांक m_maxdepth = 0;  // Maximum depth in an expression
    V3UniqueNames m_अस्थायी_Names;  // For generating unique temporary variable names

    // METHODS

    व्योम create_Deep_अस्थायी(Ast_गाँठ_व्यंजक* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* स्थिर varp = नया AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_अस्थायी_Names.लो(nodep), nodep->dtypep()};
        अगर (m_cfuncp) {
            m_cfuncp->addVarsp(varp);
        } अन्यथा {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* स्थिर newp = नया AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        AstAssign* स्थिर assp = नया AstAssign{
            nodep->fileline(), नया AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->add_Here_This_As_अग्र(assp);
    }

    // VISITORS
    व्योम visit(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_depth);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_depth = 0;
        m_maxdepth = 0;
        m_अस्थायी_Names.reset();
        iterateChildren(nodep);
    }
    व्योम visitStmt(Ast_गाँठ_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_depth);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_depth = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ_Stmt* nodep) override { visitStmt(nodep); }
    व्योम visit(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    व्योम visit(Ast_गाँठ_Termop* nodep) override {}
    व्योम visit(Ast_गाँठ_व्यंजक* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_depth);
            m_depth += 2;
            अगर (m_depth > m_maxdepth) m_maxdepth = m_depth;
            iterateChildren(nodep);
        }
        अगर (m_stmtp && (v3Global.opt.compLimitParens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_depth) > v3Global.opt.compLimitParens()
            && !VN_IS(nodep->backp(), गाँठ_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_depth;
            create_Deep_अस्थायी(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    व्योम needNonStaticFunc(Ast_गाँठ* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        अगर (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(false);
        }
    }
    व्योम visit(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    व्योम visit(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        visitStmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    व्योम visit(AstVar*) override {}  // Don't hit varrefs under vars
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
    // CONSTRUCTORS
    explicit DepthVisitor(AstNetlist* nodep)
        : m_अस्थायी_Names{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_वृक्ष();
    }
    ~DepthVisitor() override = शेष;
};

//######################################################################
// Depth class functions

व्योम V3Depth::depthAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { DepthVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("depth", 0, dump_वृक्ष_Either_Level() >= 6);
}
