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

#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3ParseImp.h"

#अन्तर्भाव <fstream>

//======================================================================
// Build in LEX script

#परिभाषय yyFlexLexer V3_Lexer_आधारः
#अन्तर्भाव "V3Lexer.yy.cpp"
#अपरिभाषय yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
वर्ग V3Lexer final : सार्वजनिक V3_Lexer_आधारः {
सार्वजनिक:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_आधारः{शून्यसूचकः} {}
    ~V3Lexer() override = स्वभाव;
    // METHODS
    शून्य unputString(ध्रुव वर्ण* textp, आकार_t दीर्घता_2) {
        // Add characters to input stream in back-to-front order
        ध्रुव वर्ण* cp = textp;
        प्रति (cp += दीर्घता_2 - 1; दीर्घता_2--; cp--) unput(*cp);
    }
};

शून्य V3ParseImp::lexUnputString(ध्रुव वर्ण* textp, आकार_t दीर्घता_2) {
    parsep()->m_lexerp->unputString(textp, दीर्घता_2);
}

शून्य V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->startToken();
    ध्रुव पूर्णाङ्क token = parsep()->m_lexerp->yylex();
    m_lex_पूर्व_Token = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

शून्य V3ParseImp::lexNew() {
    यदि (m_lexerp) विलोपय m_lexerp;  // Restart from clean slate.
    m_lexerp = नूतन V3Lexer;
    यदि (debugFlex() >= 9) m_lexerp->समुच्चयः_debug(~0);
}

शून्य V3ParseImp::lexDestroy() {
    यदि (m_lexerp) VL_DO_CLEAR(विलोपय m_lexerp, m_lexerp = शून्यसूचकः);
}
