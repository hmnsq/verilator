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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3ParseImp.h"

#شامل <fstream>

//======================================================================
// Build in LEX script

#تعریف_کرو yyFlexLexer V3_Lexer_بنیاد
#شامل "V3Lexer.yy.cpp"
#تعریف_منسوخ yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
جماعت V3Lexer final : عوامی V3_Lexer_بنیاد {
عوامی:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_بنیاد{خالی_اشاریہ} {}
    ~V3Lexer() override = طےشدہ;
    // METHODS
    خالی unputString(ثابت حرف* textp, سائز_t لمبائی_2) {
        // Add characters to input stream in back-to-front order
        ثابت حرف* cp = textp;
        برائے (cp += لمبائی_2 - 1; لمبائی_2--; cp--) unput(*cp);
    }
};

خالی V3ParseImp::lexUnputString(ثابت حرف* textp, سائز_t لمبائی_2) {
    parsep()->m_lexerp->unputString(textp, لمبائی_2);
}

خالی V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->آغاز_Token();
    ثابت صحیح token = parsep()->m_lexerp->yylex();
    m_lex_پچھلا_Token = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

خالی V3ParseImp::lexNew() {
    اگر (m_lexerp) حذف m_lexerp;  // Restart from clean slate.
    m_lexerp = نیا V3Lexer;
    اگر (debugFlex() >= 9) m_lexerp->سیٹ_debug(~0);
}

خالی V3ParseImp::lexDestroy() {
    اگر (m_lexerp) VL_DO_CLEAR(حذف m_lexerp, m_lexerp = خالی_اشاریہ);
}
