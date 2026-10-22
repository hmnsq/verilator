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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Depth.h"

#अन्तर्भाव "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग गभीरता_Visitor final : सार्वजनिक VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = शून्यसूचकः;  // Current block
    Ast_ग्रन्थिः* m_stmtp = शून्यसूचकः;  // Current statement
    पूर्णाङ्क m_गभीरता = 0;  // How deep in an expression
    पूर्णाङ्क m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_नामानि m_अस्थायी_नामानि;  // For generating unique temporary variable names

    // METHODS

    शून्य create_Deep_अस्थायी(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ध्रुव varp = नूतन AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_अस्थायी_नामानि.get(nodep), nodep->dtypep()};
        यदि (m_cfuncp) {
            m_cfuncp->सङ्कलय_Varsp(varp);
        } अन्यथा {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ध्रुव newp = नूतन AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_नियोजय* ध्रुव assp = नूतन Ast_नियोजय{
            nodep->fileline(), नूतन AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->सङ्कलय_अत्र_This_As_अग्र(assp);
    }

    // VISITORS
    शून्य दर्शय(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_गभीरता);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_गभीरता = 0;
        m_maxdepth = 0;
        m_अस्थायी_नामानि.reset();
        iterateChildren(nodep);
    }
    शून्य दर्शय_Stmt(Ast_ग्रन्थिः_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_गभीरता);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_गभीरता = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Stmt* nodep) override { दर्शय_Stmt(nodep); }
    शून्य दर्शय(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    शून्य दर्शय(Ast_ग्रन्थिः_Termop* nodep) override {}
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_गभीरता);
            m_गभीरता += 2;
            यदि (m_गभीरता > m_maxdepth) m_maxdepth = m_गभीरता;
            iterateChildren(nodep);
        }
        यदि (m_stmtp && (v3Global.opt.comp_सीमा_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_गभीरता) > v3Global.opt.comp_सीमा_Parens()
            && !VN_IS(nodep->backp(), ग्रन्थिः_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_गभीरता;
            create_Deep_अस्थायी(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    शून्य needNonStaticFunc(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        यदि (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(असत्य);
        }
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        दर्शय_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    शून्य दर्शय(AstVar*) override {}  // Don't hit varrefs under vars
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट गभीरता_Visitor(AstNetlist* nodep)
        : m_अस्थायी_नामानि{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_वृक्षः();
    }
    ~गभीरता_Visitor() override = स्वभाव;
};

//######################################################################
// Depth class functions

शून्य V3_गभीरता::गभीरता_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { गभीरता_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("depth", 0, dump_वृक्षः_Either_Level() >= 6);
}
