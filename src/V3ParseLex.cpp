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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3ParseImp.h"

#अन्तर्भाव <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#अन्तर्भाव "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
वर्ग V3Lexer final : सार्वजनिक V3LexerBase {
सार्वजनिक:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = पूर्वनिर्धारित;
    // METHODS
    शून्य unputString(स्थिरांक अक्षर* textp, माप_प्रकार length) {
        // Add characters to input stream in back-to-front order
        स्थिरांक अक्षर* cp = textp;
        साठी (cp += length - 1; length--; cp--) unput(*cp);
    }
};

शून्य V3ParseImp::lexUnputString(स्थिरांक अक्षर* textp, माप_प्रकार length) {
    parsep()->m_lexerp->unputString(textp, length);
}

शून्य V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    स्थिरांक पूर्णांक token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

शून्य V3ParseImp::lexNew() {
    जर (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = नवीन V3Lexer;
    जर (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

शून्य V3ParseImp::lexDestroy() {
    जर (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
