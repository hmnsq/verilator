// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Netlist (top level) functions
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

#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3ParseImp.h"

#ضمن <fstream>

//======================================================================
// Build in LEX script

#وصف_ڪريو yyFlexLexer V3_Lexer_بنياد
#ضمن "V3Lexer.yy.cpp"
#وصف_رد yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
صنف V3Lexer final : عام V3_Lexer_بنياد {
عام:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_بنياد{خالي_اشارو} {}
    ~V3Lexer() override = افتراضي;
    // METHODS
    فراغ unputString(ثابت حرف* textp, سائيز_t ڊيگهائي) {
        // Add characters to input stream in back-to-front order
        ثابت حرف* cp = textp;
        لكل (cp += ڊيگهائي - 1; ڊيگهائي--; cp--) unput(*cp);
    }
};

فراغ V3ParseImp::lexUnputString(ثابت حرف* textp, سائيز_t ڊيگهائي) {
    parsep()->m_lexerp->unputString(textp, ڊيگهائي);
}

فراغ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->شروعات_Token();
    ثابت صحيح token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

فراغ V3ParseImp::lexNew() {
    إذا (m_lexerp) ميٽيو m_lexerp;  // Restart from clean slate.
    m_lexerp = جديد V3Lexer;
    إذا (debugFlex() >= 9) m_lexerp->سيٽ_debug(~0);
}

فراغ V3ParseImp::lexDestroy() {
    إذا (m_lexerp) VL_DO_CLEAR(ميٽيو m_lexerp, m_lexerp = خالي_اشارو);
}
