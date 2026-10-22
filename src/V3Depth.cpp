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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Depth.h"

#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ ಆಳ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE

    // STATE - for current visit position (use VL_RESTORER)
    AstCFunc* m_cfuncp = ಶೂನ್ಯಸೂಚಕ;  // Current block
    Ast_ಗಂಟು* m_stmtp = ಶೂನ್ಯಸೂಚಕ;  // Current statement
    ಪೂರ್ಣಾಂಕ m_ಆಳ = 0;  // How deep in an expression
    ಪೂರ್ಣಾಂಕ m_maxdepth = 0;  // Maximum depth in an expression
    V3_Unique_ಹೆಸರುಗಳು m_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು;  // For generating unique temporary variable names

    // METHODS

    ಶೂನ್ಯ create_Deep_ತಾತ್ಕಾಲಿಕ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        UINFO(6, "  Deep  " << nodep);
        // UINFOTREE(9, nodep, "", "deep");
        AstVar* ಸ್ಥಿರಾಂಕ varp = ಹೊಸ AstVar{nodep->fileline(), VVarType::STMTTEMP,
                                        m_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.get(nodep), nodep->dtypep()};
        ಆದರೆ (m_cfuncp) {
            m_cfuncp->ಕೂಡಿಸು_Varsp(varp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3fatalSrc("Deep expression not under a function");
        }
        // Replace node tree with reference to var
        AstVarRef* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::READ};
        nodep->replaceWith(newp);
        // Put assignment before the referencing statement
        Ast_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(), ಹೊಸ AstVarRef{nodep->fileline(), varp, VAccess::WRITE}, nodep};
        m_stmtp->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(assp);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        VL_RESTORER(m_cfuncp);
        VL_RESTORER(m_ಆಳ);
        VL_RESTORER(m_maxdepth);
        m_cfuncp = nodep;
        m_ಆಳ = 0;
        m_maxdepth = 0;
        m_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು.reset();
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ_Stmt(Ast_ಗಂಟು_Stmt* nodep) {
        VL_RESTORER(m_stmtp);
        VL_RESTORER(m_ಆಳ);
        VL_RESTORER(m_maxdepth);
        m_stmtp = nodep;
        m_ಆಳ = 0;
        m_maxdepth = 0;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt* nodep) override { ಭೇಟಿ_Stmt(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstSFormatArg* nodep) override { iterateChildren(nodep); }  // Don't split
    // Operators
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Termop* nodep) override {}
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        // We have some operator defines that use 2 parens, so += 2.
        {
            VL_RESTORER(m_ಆಳ);
            m_ಆಳ += 2;
            ಆದರೆ (m_ಆಳ > m_maxdepth) m_maxdepth = m_ಆಳ;
            iterateChildren(nodep);
        }
        ಆದರೆ (m_stmtp && (v3Global.opt.comp_ಎಲ್ಲೆ_Parens() >= 1)  // Else compiler doesn't need it
            && (m_maxdepth - m_ಆಳ) > v3Global.opt.comp_ಎಲ್ಲೆ_Parens()
            && !VN_IS(nodep->backp(), ಗಂಟು_Stmt)  // Not much point if we're about to use it
        ) {
            m_maxdepth = m_ಆಳ;
            create_Deep_ತಾತ್ಕಾಲಿಕ(nodep);
        }
    }

    //--------------------
    // Marking of non-static functions (because they might need "this")
    // (Here instead of new visitor after V3Descope just to avoid another visitor)
    ಶೂನ್ಯ needNonStaticFunc(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(m_cfuncp, nodep, "Non-static accessor not under a function");
        // DPI export implementations are always invoked via the generic DPI dispatcher,
        // which can only ever supply the symbol table pointer, never a scope-specific
        // "this", so they must remain static regardless of what they contain.
        ಆದರೆ (m_cfuncp->isStatic() && !m_cfuncp->dpiExportImpl()) {
            UINFO(5, "Mark non-public due to " << nodep);
            m_cfuncp->isStatic(ಸುಳ್ಳು);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCExprUser* nodep) override {
        needNonStaticFunc(nodep);
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCStmtUser* nodep) override {
        needNonStaticFunc(nodep);
        ಭೇಟಿ_Stmt(nodep);
    }

    //--------------------
    // Default: Just iterate
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Don't hit varrefs under vars
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಆಳ_Visitor(AstNetlist* nodep)
        : m_ತಾತ್ಕಾಲಿಕ_ಹೆಸರುಗಳು{"__Vdeeptemp"} {
        iterate(nodep);
        // Extracting expressions can effect purity
        VIsCached::clear_Cache_ವೃಕ್ಷ();
    }
    ~ಆಳ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Depth class functions

ಶೂನ್ಯ V3_ಆಳ::ಆಳ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಆಳ_Visitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("depth", 0, dump_ವೃಕ್ಷ_Either_Level() >= 6);
}
