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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3ParseImp.h"

#অন্তর্ভাব <fstream>

//======================================================================
// Build in LEX script

#define yyFlexLexer V3LexerBase
#অন্তর্ভাব "V3Lexer.yy.cpp"
#undef yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
শ্রেণী V3Lexer final : সর্বজনীন V3LexerBase {
সর্বজনীন:
    // CONSTRUCTORS
    V3Lexer()
        : V3LexerBase{nullptr} {}
    ~V3Lexer() override = পূর্বনির্ধারিত;
    // METHODS
    শূন্য unputString(ধ্রুবক অক্ষর* textp, আকার_t দৈর্ঘ্য) {
        // Add characters to input stream in back-to-front order
        ধ্রুবক অক্ষর* cp = textp;
        জন্য (cp += দৈর্ঘ্য - 1; দৈর্ঘ্য--; cp--) unput(*cp);
    }
};

শূন্য V3ParseImp::lexUnputString(ধ্রুবক অক্ষর* textp, আকার_t দৈর্ঘ্য) {
    parsep()->m_lexerp->unputString(textp, দৈর্ঘ্য);
}

শূন্য V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    ধ্রুবক পূর্ণসংখ্যা token = parsep()->m_lexerp->yylex();
    m_lex_পূর্ব_Token = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

শূন্য V3ParseImp::lexNew() {
    যদি (m_lexerp) delete m_lexerp;  // Restart from clean slate.
    m_lexerp = নতুন V3Lexer;
    যদি (debugFlex() >= 9) m_lexerp->set_debug(~0);
}

শূন্য V3ParseImp::lexDestroy() {
    যদি (m_lexerp) VL_DO_CLEAR(delete m_lexerp, m_lexerp = nullptr);
}
