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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3ParseImp.h"

#ଅନ୍ତର୍ଭାଵ <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#ଅନ୍ତର୍ଭାଵ "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
ଶ୍ରେଣୀ V3Lexer final : ସର୍ବସାଧାରଣ V3LexerBase {
ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    ଶୂନ୍ୟ unputString(ଅଚଳ ଅକ୍ଷର* textp, size_t length) {
        // Add characters to input stream in back-to-front order
        ଅଚଳ ଅକ୍ଷର* cp = textp;
        ପାଇଁ (cp += length - 1; length--; cp--) unput(*cp);
    }
};

ଶୂନ୍ୟ V3ParseImp::lexUnputString(ଅଚଳ ଅକ୍ଷର* textp, size_t length) {
    parsep()->m_lexerp->unputString(textp, length);
}

ଶୂନ୍ୟ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

ଶୂନ୍ୟ V3ParseImp::lexNew() {
    ଯଦି (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = ନୂତନ V3Lexer;
    ଯଦି (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

ଶୂନ୍ୟ V3ParseImp::lexDestroy() {
    ଯଦି (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
