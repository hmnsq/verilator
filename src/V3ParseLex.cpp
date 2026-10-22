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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ParseImp.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>

//======================================================================
// Build in LEX script

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ yyFlexLexer V3_Lexer_ਅਧਾਰ
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Lexer.yy.cpp"
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
ਵਰਗ V3Lexer final : ਜਨਤਕ V3_Lexer_ਅਧਾਰ {
ਜਨਤਕ:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_ਅਧਾਰ{ਖਾਲੀ_ਸੂਚਕ} {}
    ~V3Lexer() override = ਮੂਲ;
    // METHODS
    ਖਾਲੀ unputString(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਲੰਮਾਈ) {
        // Add characters to input stream in back-to-front order
        ਅਚਲ ਅੱਖਰ* cp = textp;
        ਲਈ (cp += ਲੰਮਾਈ - 1; ਲੰਮਾਈ--; cp--) unput(*cp);
    }
};

ਖਾਲੀ V3ParseImp::lexUnputString(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਲੰਮਾਈ) {
    parsep()->m_lexerp->unputString(textp, ਲੰਮਾਈ);
}

ਖਾਲੀ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->ਸ਼ੁਰੂ_Token();
    ਅਚਲ ਪੂਰਨ token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

ਖਾਲੀ V3ParseImp::lexNew() {
    ਜੇ (m_lexerp) ਮਿਟਾਓ m_lexerp;  // Restart from clean slate.
    m_lexerp = ਨਵਾਂ V3Lexer;
    ਜੇ (debugFlex() >= 9) m_lexerp->ਸੈੱਟ_debug(~0);
}

ਖਾਲੀ V3ParseImp::lexDestroy() {
    ਜੇ (m_lexerp) VL_DO_CLEAR(ਮਿਟਾਓ m_lexerp, m_lexerp = ਖਾਲੀ_ਸੂਚਕ);
}
