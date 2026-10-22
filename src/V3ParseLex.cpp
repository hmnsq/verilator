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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3ParseImp.h"

#ضمن <fstream>

//======================================================================
// Build in LEX script

#تعریف_کرو yyFlexLexer V3_Lexer_ادھار
#ضمن "V3Lexer.yy.cpp"
#تعریف_رد yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
صنف V3Lexer final : عام V3_Lexer_ادھار {
عام:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_ادھار{خالی_سوچک} {}
    ~V3Lexer() override = افتراضي;
    // METHODS
    فراغ unputString(ثابت حرف* textp, ناپ_t لمائی) {
        // Add characters to input stream in back-to-front order
        ثابت حرف* cp = textp;
        لكل (cp += لمائی - 1; لمائی--; cp--) unput(*cp);
    }
};

فراغ V3ParseImp::lexUnputString(ثابت حرف* textp, ناپ_t لمائی) {
    parsep()->m_lexerp->unputString(textp, لمائی);
}

فراغ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->شروع_Token();
    ثابت صحيح token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

فراغ V3ParseImp::lexNew() {
    إذا (m_lexerp) مٹاؤ m_lexerp;  // Restart from clean slate.
    m_lexerp = جديد V3Lexer;
    إذا (debugFlex() >= 9) m_lexerp->سیٹ_debug(~0);
}

فراغ V3ParseImp::lexDestroy() {
    إذا (m_lexerp) VL_DO_CLEAR(مٹاؤ m_lexerp, m_lexerp = خالی_سوچک);
}
