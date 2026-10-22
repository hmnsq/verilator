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

#define VL_MT_DISABLED_CODE_UNIT 1

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3ParseImp.h"

#شامل <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#شامل "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
رده V3Lexer final : عمومی V3LexerBase {
عمومی:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = پیشفرض;
    // METHODS
    تهی unputString(ثابت نویسه* textp, size_t length) {
        // Add characters to input stream in back-to-front order
        ثابت نویسه* cp = textp;
        برای (cp += length - 1; length--; cp--) unput(*cp);
    }
};

تهی V3ParseImp::lexUnputString(ثابت نویسه* textp, size_t length) {
    parsep()->m_lexerp->unputString(textp, length);
}

تهی V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    ثابت صحیح token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

تهی V3ParseImp::lexNew() {
    اگر (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = جدید V3Lexer;
    اگر (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

تهی V3ParseImp::lexDestroy() {
    اگر (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
