// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilog::Preproc: Internal header for lex interfacing
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2000-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// This header provides the interface between the lex proper V3PreLex.l/.cpp
// and the class implementation file V3Pre.cpp
// It is not intended for user applications.
//*************************************************************************

#నిర్వచితం_కాకపోతే VERILATOR_VPRELEX_H_  // Guard
#నిర్వచించు VERILATOR_VPRELEX_H_

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"

#అన్తర్భావ <deque>
#అన్తర్భావ <stack>

//======================================================================

class V3PreLex;
class V3PreProcImp;

// Token codes
// If changing, see V3PreProc.cpp's V3PreProcImp::tokenName()
// clang-format off
#నిర్వచించు VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#నిర్వచించు VP_EOF_ERROR    400

#నిర్వచించు VP_INCLUDE      256
#నిర్వచించు VP_IFDEF        257
#నిర్వచించు VP_IFNDEF       258
#నిర్వచించు VP_ENDIF        259
#నిర్వచించు VP_UNDEF        260
#నిర్వచించు VP_DEFINE       261
#నిర్వచించు VP_ELSE         262
#నిర్వచించు VP_ELSIF        263
#నిర్వచించు VP_పంక్తి         264
#నిర్వచించు VP_UNDEFINEALL  265

#నిర్వచించు VP_చిహ్నం       300
#నిర్వచించు VP_STRING       301
#నిర్వచించు VP_DEFVALUE     302
#నిర్వచించు VP_COMMENT      303
#నిర్వచించు VP_పాఠ్యం         304
#నిర్వచించు VP_WHITE        305
#నిర్వచించు VP_DEFREF       306
#నిర్వచించు VP_DEFARG       307
#నిర్వచించు VP_ERROR        308
#నిర్వచించు VP_DEFFORM      309
#నిర్వచించు VP_STRIFY       310
#నిర్వచించు VP_BACKQUOTE    311
#నిర్వచించు VP_చిహ్నం_JOIN  312
#నిర్వచించు VP_DEFREF_JOIN  313
#నిర్వచించు VP_JOIN         314

#నిర్వచించు VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#నిర్వచితం_కాకపోతే yy_create_బఫరు
# define yy_create_బఫరు V3_Pre_Lex_create_బఫరు
# define yy_delete_బఫరు V3_Pre_Lex_delete_బఫరు
# define yy_పరిశీలన_బఫరు V3_Pre_Lex_పరిశీలన_బఫరు
# define yy_పరిశీలన_string V3_Pre_Lex_పరిశీలన_string
# define yy_పరిశీలన_బైట్లు V3_Pre_Lex_పరిశీలన_బైట్లు
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_బఫరు V3_Pre_Lex_init_బఫరు
# define yy_flush_బఫరు V3_Pre_Lex_flush_బఫరు
# define yy_ఎక్కించు_బఫరు_state V3_Pre_Lex_ఎక్కించు_బఫరు_state
# define yy_switch_గమ్యం_బఫరు V3_Pre_Lex_switch_గమ్యం_బఫరు
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే YY_బఫరు_STATE
నిర్మాణం yy_బఫరు_state;
రకనిర్వచనం నిర్మాణం yy_బఫరు_state* YY_బఫరు_STATE;
# define YY_BUF_కొలత 16384
#అయితే_ముగింపు
// clang-format on

బాహ్య పూర్ణాంకం yylex();
బాహ్య శూన్య yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
బాహ్య అక్షరం* yyourtext();
బాహ్య కొలత_t yyourleng();
బాహ్య శూన్య yyourtext(స్థిరరాశి అక్షరం* textp, కొలత_t కొలత);  // Must call with static

YY_బఫరు_STATE yy_create_బఫరు(FILE* file, పూర్ణాంకం కొలత);
శూన్య yy_switch_గమ్యం_బఫరు(YY_బఫరు_STATE new_బఫరు);
శూన్య yy_delete_బఫరు(YY_బఫరు_STATE b);
పూర్ణాంకం yylex_destroy();

//======================================================================

#నిర్వచించు KEEPCMT_SUB 2
#నిర్వచించు KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_ప్రవాహం final {
బహిరంగ:
    File_పంక్తి* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* స్థిరరాశి m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    పూర్ణాంకం m_ignNewlines = 0;  // Ignore multiline newlines
    పూర్ణాంకం m_పదం_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_ప్రవాహం(File_పంక్తి* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_ప్రవాహం_లోతు_కూడు(1);
    }
    ~VPre_ప్రవాహం() { lex_ప్రవాహం_లోతు_కూడు(-1); }

అంతరంగ:
    అంతఃవరుస శూన్య lex_ప్రవాహం_లోతు_కూడు(పూర్ణాంకం delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
గణన class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
బహిరంగ:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* స్థిరరాశి m_preimpp;  // Preprocessor lexor belongs to
    std::దొంతర<VPre_ప్రవాహం*> m_streamp_దొంతర;  // Stack of processing files
    పూర్ణాంకం m_ప్రవాహం_లోతు = 0;  // Depth of stream processing
    YY_బఫరు_STATE m_బఫరు_State;  // Flex state
    File_పంక్తి* m_tokFilelinep;  // Starting position of current token
    std::deque<File_పంక్తి> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    స్థిర V3PreLex* s_currentLexp;  ///< Current lexing point
    పూర్ణాంకం m_keepComments = 0;  ///< Emit comments in output text
    పూర్ణాంకం m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    పూర్ణాంకం m_formalLevel = 0;  // Parenthesis counting inside def formals
    పూర్ణాంకం m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_విలువ;  // Definition value being built.
    పూర్ణాంకం m_enterExit = 0;  // For VL_LINE, the enter/exit level
    పూర్ణాంకం m_prot_నిడివి = 0;  // unencoded length for BASE64
    పూర్ణాంకం m_prot_బైట్లు = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_పంక్తి* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_మొదటి_బఫరు(filelinep);
    }
    ~V3PreLex() {
        అయితే (!m_streamp_దొంతర.ఖాళీలు()) {
            delete m_streamp_దొంతర.శిఖరం();
            m_streamp_దొంతర.తీయి();
        }
        VL_DO_CLEAR(yy_delete_బఫరు(m_బఫరు_State), m_బఫరు_State = nullptr);
        yylex_destroy();
    }
    VPre_ప్రవాహం* new_ప్రవాహం(File_పంక్తి* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_ప్రవాహం* curStreamp() { తిరిగి m_streamp_దొంతర.శిఖరం(); }  // Can't be empty, "EOF" is on top
    File_పంక్తి* curFilelinep() { తిరిగి curStreamp()->m_curFilelinep; }
    శూన్య curFilelinep(File_పంక్తి* fl) { curStreamp()->m_curFilelinep = fl; }
    శూన్య append_Def_విలువ(స్థిరరాశి అక్షరం* textp, కొలత_t len) { m_def_విలువ.append(textp, len); }
    శూన్య పంక్తి_Directive(స్థిరరాశి అక్షరం* textp);
    శూన్య linenoInc() {
        ఒకవేళ (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } లేకపోతే {
            curFilelinep()->linenoInc();
        }
    }
    శూన్య warnBackslashSpace();
    శూన్య verilatorCmtLint(స్థిరరాశి అక్షరం* textp, bool turnOff);
    శూన్య verilatorCmtLintRestore();
    శూన్య verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    శూన్య నెట్టు_State_Def_Arg(పూర్ణాంకం level);
    శూన్య నెట్టు_State_Def_Form();
    శూన్య నెట్టు_State_Def_విలువ();
    శూన్య నెట్టు_State_సమాసం();
    శూన్య నెట్టు_State_Inc_Filename();
    శూన్య నెట్టు_State_Passthru();
    శూన్య పరిశీలన_New_File(File_పంక్తి* filelinep);
    శూన్య పరిశీలన_బైట్లు(స్థిరరాశి string& str);
    శూన్య పరిశీలన_బైట్లు_వెనుక(స్థిరరాశి string& str);
    కొలత_t ఆదానం_గమ్యం_Lex(అక్షరం* buf, కొలత_t max_కొలత_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_బఫరు_STATE current_బఫరు();
    పూర్ణాంకం lex();
    పూర్ణాంకం current_ప్రారంభం_State() స్థిరరాశి;
    శూన్య dumpSummary();
    శూన్య dump_దొంతర();
    శూన్య వాడని();
    // Called by VPreStream
    శూన్య ప్రవాహం_లోతు_కూడు(పూర్ణాంకం delta) { m_ప్రవాహం_లోతు += delta; }
    పూర్ణాంకం ప్రవాహం_లోతు() స్థిరరాశి { తిరిగి m_ప్రవాహం_లోతు; }
    // Utility
    స్థిర శూన్య సమితి_YYDebug(bool on);
    స్థిర string cleanDbgStrg(స్థిరరాశి string& లోపల);

అంతరంగ:
    string currentUnreadChars();
    string చివర_యొక్క_ప్రవాహం(bool& againr);
    శూన్య init_మొదటి_బఫరు(File_పంక్తి* filelinep);
    శూన్య పరిశీలన_Switch_ప్రవాహం(VPre_ప్రవాహం* streamp);
};

శూన్య VPre_ప్రవాహం::lex_ప్రవాహం_లోతు_కూడు(పూర్ణాంకం delta) { m_lexp->ప్రవాహం_లోతు_కూడు(delta); }

#అయితే_ముగింపు  // Guard
