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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3ParseImp.h"

#అన్తర్భావ <fstream>

//======================================================================
// Build in LEX script

#నిర్వచించు yyFlexLexer V3_Lexer_పునాది
#అన్తర్భావ "V3Lexer.yy.cpp"
#నిర్వచనం_రద్దు yyFlexLexer

//######################################################################
// Lex-derived class

/// Override the base lexer class so we can add some access functions
తరగతి V3Lexer final : బహిరంగ V3_Lexer_పునాది {
బహిరంగ:
    // CONSTRUCTORS
    V3Lexer()
        : V3_Lexer_పునాది{శూన్యసూచిక} {}
    ~V3Lexer() override = అప్రమేయ;
    // METHODS
    శూన్య unputString(స్థిరరాశి అక్షరం* textp, కొలత_t నిడివి) {
        // Add characters to input stream in back-to-front order
        స్థిరరాశి అక్షరం* cp = textp;
        కొరకు (cp += నిడివి - 1; నిడివి--; cp--) unput(*cp);
    }
};

శూన్య V3ParseImp::lexUnputString(స్థిరరాశి అక్షరం* textp, కొలత_t నిడివి) {
    parsep()->m_lexerp->unputString(textp, నిడివి);
}

శూన్య V3ParseImp::yylexReadTok() {
    // Call yylex() remembering last non-whitespace token
    parsep()->lexFileline()->ప్రారంభం_Token();
    స్థిరరాశి పూర్ణాంకం token = parsep()->m_lexerp->yylex();
    m_lexPrevToken = token;  // Save so can find '#' to parse following number
    yylval.token = token;
}

//######################################################################
// Read class functions

శూన్య V3ParseImp::lexNew() {
    ఒకవేళ (m_lexerp) తొలగించు m_lexerp;  // Restart from clean slate.
    m_lexerp = కొత్త V3Lexer;
    ఒకవేళ (debugFlex() >= 9) m_lexerp->సమితి_debug(~0);
}

శూన్య V3ParseImp::lexDestroy() {
    ఒకవేళ (m_lexerp) VL_DO_CLEAR(తొలగించు m_lexerp, m_lexerp = శూన్యసూచిక);
}
