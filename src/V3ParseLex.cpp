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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3ParseImp.h"

#ಅನ್ತರ್ಭಾವ <fstream>

//======================================================================
// Build in LEX script

#ವ್ಯಾಖ್ಯಾನಿಸು yyFlexLexer V3_Lexer_ತಳ
#ಅನ್ತರ್ಭಾವ "V3Lexer.yy.cpp"
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
ವರ್ಗ V3Lexer final : ಸಾರ್ವಜನಿಕ V3_Lexer_ತಳ {
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_ತಳ{ಶೂನ್ಯಸೂಚಕ} {}
    ~V3Lexer() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    ಶೂನ್ಯ unputString(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಉದ್ದಳತೆ) {
        // Add characters to input stream in back-to-front order
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = textp;
        ಗಾಗಿ (cp += ಉದ್ದಳತೆ - 1; ಉದ್ದಳತೆ--; cp--) unput(*cp);
    }
};

ಶೂನ್ಯ V3ParseImp::lexUnputString(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಉದ್ದಳತೆ) {
    parsep()->m_lexerp->unputString(textp, ಉದ್ದಳತೆ);
}

ಶೂನ್ಯ V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->ಆರಂಭ_Token();
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

ಶೂನ್ಯ V3ParseImp::lexNew() {
    ಆದರೆ (m_lexerp) ಅಳಿಸು m_lexerp;  // Restart from clean slate.
    m_lexerp = ಹೊಸ V3Lexer;
    ಆದರೆ (debugFlex() >= 9) m_lexerp->ಗಣ_debug(~0);
}

ಶೂನ್ಯ V3ParseImp::lexDestroy() {
    ಆದರೆ (m_lexerp) VL_DO_CLEAR(ಅಳಿಸು m_lexerp, m_lexerp = ಶೂನ್ಯಸೂಚಕ);
}
