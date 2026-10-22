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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3ParseImp.h"

#ضمن <fstream>

//======================================================================
// Build in LEX script

#تعریف_کړه yyFlexLexer V3_Lexer_بنسټ
#ضمن "V3Lexer.yy.cpp"
#تعریف_لغوه yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
صنف V3Lexer final : عام V3_Lexer_بنسټ {
عام:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_بنسټ{تش_اشاره} {}
    ~V3Lexer() override = افتراضي;
    // METHODS
    فراغ unputString(ثابت حرف* textp, اندازه_t اوږدتیا) {
        // Add characters to input stream in back-to-front order
        ثابت حرف* cp = textp;
        لكل (cp += اوږدتیا - 1; اوږدتیا--; cp--) unput(*cp);
    }
};

فراغ V3ParseImp::lexUnputString(ثابت حرف* textp, اندازه_t اوږدتیا) {
    parsep()->m_lexerp->unputString(textp, اوږدتیا);
}

فراغ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->پیل_Token();
    ثابت صحيح token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

فراغ V3ParseImp::lexNew() {
    إذا (m_lexerp) ړنګول m_lexerp;  // Restart from clean slate.
    m_lexerp = جديد V3Lexer;
    إذا (debugFlex() >= 9) m_lexerp->سېټ_debug(~0);
}

فراغ V3ParseImp::lexDestroy() {
    إذا (m_lexerp) VL_DO_CLEAR(ړنګول m_lexerp, m_lexerp = تش_اشاره);
}
