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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3ParseImp.h"

#அந்தர்பாவ <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#அந்தர்பாவ "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
வகுப்பு V3Lexer final : பொது V3LexerBase {
பொது:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = இயல்பு;
    // METHODS
    வெறுமை unputString(மாறிலி எழுத்து* textp, size_t length) {
        // Add characters to input stream in back-to-front order
        மாறிலி எழுத்து* cp = textp;
        க்கு (cp += length - 1; length--; cp--) unput(*cp);
    }
};

வெறுமை V3ParseImp::lexUnputString(மாறிலி எழுத்து* textp, size_t length) {
    parsep()->m_lexerp->unputString(textp, length);
}

வெறுமை V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    மாறிலி முழு token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

வெறுமை V3ParseImp::lexNew() {
    எனில் (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = புதிய V3Lexer;
    எனில் (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

வெறுமை V3ParseImp::lexDestroy() {
    எனில் (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
