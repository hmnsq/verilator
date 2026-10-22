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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3ParseImp.h"

#ಅನ್ತರ್ಭಾವ <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#ಅನ್ತರ್ಭಾವ "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
ವರ್ಗ V3Lexer final : ಸಾರ್ವಜನಿಕ V3LexerBase {
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = ಸ್ವಭಾವ;
    // METHODS
    ಶೂನ್ಯ unputString(ಧ್ರುವ ವರ್ಣ* textp, size_t length) {
        // Add characters to input stream in back-to-front order
        ಧ್ರುವ ವರ್ಣ* cp = textp;
        ಪ್ರತಿ (cp += length - 1; length--; cp--) unput(*cp);
    }
};

ಶೂನ್ಯ V3ParseImp::lexUnputString(ಧ್ರುವ ವರ್ಣ* textp, size_t length) {
    parsep()->m_lexerp->unputString(textp, length);
}

ಶೂನ್ಯ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

ಶೂನ್ಯ V3ParseImp::lexNew() {
    ಯದಿ (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = ನೂತನ V3Lexer;
    ಯದಿ (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

ಶೂನ್ಯ V3ParseImp::lexDestroy() {
    ಯದಿ (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
