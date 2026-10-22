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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3ParseImp.h"

#અન્તર્ભાવ <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#અન્તર્ભાવ "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
વર્ગ V3Lexer final : જાહેર V3LexerBase {
જાહેર:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = મૂળભૂત;
    // METHODS
    શૂન્ય unputString(અચલ અક્ષર* textp, size_t length) {
        // Add characters to input stream in back-to-front order
        અચલ અક્ષર* cp = textp;
        માટે (cp += length - 1; length--; cp--) unput(*cp);
    }
};

શૂન્ય V3ParseImp::lexUnputString(અચલ અક્ષર* textp, size_t length) {
    parsep()->m_lexerp->unputString(textp, length);
}

શૂન્ય V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    અચલ પૂર્ણાંક token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

શૂન્ય V3ParseImp::lexNew() {
    જો (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = નવું V3Lexer;
    જો (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

શૂન્ય V3ParseImp::lexDestroy() {
    જો (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
