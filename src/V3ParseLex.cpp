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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3ParseImp.h"

#समावेश <fstream>

//======================================================================
// Build in LEX script

#घोषणा yyFlexLexer V3LexerBase
#समावेश "V3Lexer.yy.cpp"
#अघोषित yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
श्रेणी V3Lexer final : खुला V3LexerBase {
खुला:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = शेष;
    // METHODS
    व्योम unputString(स्थिर अक्षर* textp, माप_प्रकार लंबाई) {
        // Add characters to input stream in back-to-front order
        स्थिर अक्षर* cp = textp;
        क्रम (cp += लंबाई - 1; लंबाई--; cp--) unput(*cp);
    }
};

व्योम V3ParseImp::lexUnputString(स्थिर अक्षर* textp, माप_प्रकार लंबाई) {
    parsep()->m_lexerp->unputString(textp, लंबाई);
}

व्योम V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    स्थिर पूर्णांक token = parsep()->m_lexerp->yylex();
    m_lex_पूर्व_Token = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

व्योम V3ParseImp::lexNew() {
    अगर (m_lexerp) मिटाओ m_lexerp;  // Restart from clean slate.
    m_lexerp = नया V3Lexer;
    अगर (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

व्योम V3ParseImp::lexDestroy() {
    अगर (m_lexerp) VL_DO_CLEAR(मिटाओ m_lexerp, m_lexerp = nullptr);
}
