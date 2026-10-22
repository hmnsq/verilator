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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VPRELEX_H_  // Guard
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VPRELEX_H_

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <stack>

//======================================================================

class V3PreLex;
class V3PreProcImp;

// Token codes
// If changing, see V3PreProc.cpp's V3PreProcImp::tokenName()
// clang-format off
#ವ್ಯಾಖ್ಯಾನಿಸು VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#ವ್ಯಾಖ್ಯಾನಿಸು VP_EOF_ERROR    400

#ವ್ಯಾಖ್ಯಾನಿಸು VP_INCLUDE      256
#ವ್ಯಾಖ್ಯಾನಿಸು VP_IFDEF        257
#ವ್ಯಾಖ್ಯಾನಿಸು VP_IFNDEF       258
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ENDIF        259
#ವ್ಯಾಖ್ಯಾನಿಸು VP_UNDEF        260
#ವ್ಯಾಖ್ಯಾನಿಸು VP_DEFINE       261
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ELSE         262
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ELSIF        263
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ಸಾಲು         264
#ವ್ಯಾಖ್ಯಾನಿಸು VP_UNDEFINEALL  265

#ವ್ಯಾಖ್ಯಾನಿಸು VP_ಚಿಹ್ನೆ       300
#ವ್ಯಾಖ್ಯಾನಿಸು VP_STRING       301
#ವ್ಯಾಖ್ಯಾನಿಸು VP_DEFVALUE     302
#ವ್ಯಾಖ್ಯಾನಿಸು VP_COMMENT      303
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ಪಠ್ಯ         304
#ವ್ಯಾಖ್ಯಾನಿಸು VP_WHITE        305
#ವ್ಯಾಖ್ಯಾನಿಸು VP_DEFREF       306
#ವ್ಯಾಖ್ಯಾನಿಸು VP_DEFARG       307
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ERROR        308
#ವ್ಯಾಖ್ಯಾನಿಸು VP_DEFFORM      309
#ವ್ಯಾಖ್ಯಾನಿಸು VP_STRIFY       310
#ವ್ಯಾಖ್ಯಾನಿಸು VP_BACKQUOTE    311
#ವ್ಯಾಖ್ಯಾನಿಸು VP_ಚಿಹ್ನೆ_JOIN  312
#ವ್ಯಾಖ್ಯಾನಿಸು VP_DEFREF_JOIN  313
#ವ್ಯಾಖ್ಯಾನಿಸು VP_JOIN         314

#ವ್ಯಾಖ್ಯಾನಿಸು VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ yy_create_ಬಫರ್
# define yy_create_ಬಫರ್ V3_Pre_Lex_create_ಬಫರ್
# define yy_delete_ಬಫರ್ V3_Pre_Lex_delete_ಬಫರ್
# define yy_ಪರಿಶೀಲನೆ_ಬಫರ್ V3_Pre_Lex_ಪರಿಶೀಲನೆ_ಬಫರ್
# define yy_ಪರಿಶೀಲನೆ_string V3_Pre_Lex_ಪರಿಶೀಲನೆ_string
# define yy_ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು V3_Pre_Lex_ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_ಬಫರ್ V3_Pre_Lex_init_ಬಫರ್
# define yy_flush_ಬಫರ್ V3_Pre_Lex_flush_ಬಫರ್
# define yy_ತುಂಬು_ಬಫರ್_state V3_Pre_Lex_ತುಂಬು_ಬಫರ್_state
# define yy_switch_ಗಮ್ಯ_ಬಫರ್ V3_Pre_Lex_switch_ಗಮ್ಯ_ಬಫರ್
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ YY_ಬಫರ್_STATE
ರಚನೆ yy_ಬಫರ್_state;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ yy_ಬಫರ್_state* YY_ಬಫರ್_STATE;
# define YY_BUF_ಗಾತ್ರ 16384
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

ಬಾಹ್ಯ ಪೂರ್ಣಾಂಕ yylex();
ಬಾಹ್ಯ ಶೂನ್ಯ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
ಬಾಹ್ಯ ಅಕ್ಷರ* yyourtext();
ಬಾಹ್ಯ ಗಾತ್ರ_t yyourleng();
ಬಾಹ್ಯ ಶೂನ್ಯ yyourtext(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಗಾತ್ರ_2);  // Must call with static

YY_ಬಫರ್_STATE yy_create_ಬಫರ್(FILE* file, ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2);
ಶೂನ್ಯ yy_switch_ಗಮ್ಯ_ಬಫರ್(YY_ಬಫರ್_STATE new_ಬಫರ್);
ಶೂನ್ಯ yy_delete_ಬಫರ್(YY_ಬಫರ್_STATE b);
ಪೂರ್ಣಾಂಕ yylex_destroy();

//======================================================================

#ವ್ಯಾಖ್ಯಾನಿಸು KEEPCMT_SUB 2
#ವ್ಯಾಖ್ಯಾನಿಸು KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_ಹರಿವು final {
ಸಾರ್ವಜನಿಕ:
    File_ಸಾಲು* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ಸ್ಥಿರಾಂಕ m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    ಪೂರ್ಣಾಂಕ m_ignNewlines = 0;  // Ignore multiline newlines
    ಪೂರ್ಣಾಂಕ m_ಪದ_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_ಹರಿವು(File_ಸಾಲು* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_ಹರಿವು_ಆಳ_ಕೂಡಿಸು(1);
    }
    ~VPre_ಹರಿವು() { lex_ಹರಿವು_ಆಳ_ಕೂಡಿಸು(-1); }

ಖಾಸಗಿ:
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ lex_ಹರಿವು_ಆಳ_ಕೂಡಿಸು(ಪೂರ್ಣಾಂಕ delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
ಎಣಿಕೆ class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
ಸಾರ್ವಜನಿಕ:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ಸ್ಥಿರಾಂಕ m_preimpp;  // Preprocessor lexor belongs to
    std::ರಾಶಿ<VPre_ಹರಿವು*> m_streamp_ರಾಶಿ;  // Stack of processing files
    ಪೂರ್ಣಾಂಕ m_ಹರಿವು_ಆಳ = 0;  // Depth of stream processing
    YY_ಬಫರ್_STATE m_ಬಫರ್_State;  // Flex state
    File_ಸಾಲು* m_tokFilelinep;  // Starting position of current token
    std::deque<File_ಸಾಲು> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    ಸ್ಥಿರ V3PreLex* s_currentLexp;  ///< Current lexing point
    ಪೂರ್ಣಾಂಕ m_keepComments = 0;  ///< Emit comments in output text
    ಪೂರ್ಣಾಂಕ m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    ಪೂರ್ಣಾಂಕ m_formalLevel = 0;  // Parenthesis counting inside def formals
    ಪೂರ್ಣಾಂಕ m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_ಮೌಲ್ಯ;  // Definition value being built.
    ಪೂರ್ಣಾಂಕ m_enterExit = 0;  // For VL_LINE, the enter/exit level
    ಪೂರ್ಣಾಂಕ m_prot_ಉದ್ದಳತೆ = 0;  // unencoded length for BASE64
    ಪೂರ್ಣಾಂಕ m_prot_ಬೈಟುಗಳು = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_ಸಾಲು* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_ಮೊದಲ_ಬಫರ್(filelinep);
    }
    ~V3PreLex() {
        ಆಗ (!m_streamp_ರಾಶಿ.ಖಾಲಿಗಳು()) {
            delete m_streamp_ರಾಶಿ.ತುದಿ();
            m_streamp_ರಾಶಿ.ತೆಗೆ_2();
        }
        VL_DO_CLEAR(yy_delete_ಬಫರ್(m_ಬಫರ್_State), m_ಬಫರ್_State = nullptr);
        yylex_destroy();
    }
    VPre_ಹರಿವು* new_ಹರಿವು(File_ಸಾಲು* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_ಹರಿವು* curStreamp() { ಹಿಂತಿರುಗಿ m_streamp_ರಾಶಿ.ತುದಿ(); }  // Can't be empty, "EOF" is on top
    File_ಸಾಲು* curFilelinep() { ಹಿಂತಿರುಗಿ curStreamp()->m_curFilelinep; }
    ಶೂನ್ಯ curFilelinep(File_ಸಾಲು* fl) { curStreamp()->m_curFilelinep = fl; }
    ಶೂನ್ಯ append_Def_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t len) { m_def_ಮೌಲ್ಯ.append(textp, len); }
    ಶೂನ್ಯ ಸಾಲು_Directive(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp);
    ಶೂನ್ಯ linenoInc() {
        ಆದರೆ (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } ಇಲ್ಲದಿದ್ದರೆ {
            curFilelinep()->linenoInc();
        }
    }
    ಶೂನ್ಯ warnBackslashSpace();
    ಶೂನ್ಯ verilatorCmtLint(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, bool turnOff);
    ಶೂನ್ಯ verilatorCmtLintRestore();
    ಶೂನ್ಯ verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    ಶೂನ್ಯ ತಳ್ಳು_State_Def_Arg(ಪೂರ್ಣಾಂಕ level);
    ಶೂನ್ಯ ತಳ್ಳು_State_Def_Form();
    ಶೂನ್ಯ ತಳ್ಳು_State_Def_ಮೌಲ್ಯ();
    ಶೂನ್ಯ ತಳ್ಳು_State_ಅಭಿವ್ಯಕ್ತಿ();
    ಶೂನ್ಯ ತಳ್ಳು_State_Inc_Filename();
    ಶೂನ್ಯ ತಳ್ಳು_State_Passthru();
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_New_File(File_ಸಾಲು* filelinep);
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು(ಸ್ಥಿರಾಂಕ string& str);
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು_ಹಿಂಭಾಗ(ಸ್ಥಿರಾಂಕ string& str);
    ಗಾತ್ರ_t ಒಳಹರಿವು_ಗಮ್ಯ_Lex(ಅಕ್ಷರ* buf, ಗಾತ್ರ_t max_ಗಾತ್ರ_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_ಬಫರ್_STATE current_ಬಫರ್();
    ಪೂರ್ಣಾಂಕ lex();
    ಪೂರ್ಣಾಂಕ current_ಆರಂಭ_State() ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ dumpSummary();
    ಶೂನ್ಯ dump_ರಾಶಿ();
    ಶೂನ್ಯ ಬಳಸದ();
    // Called by VPreStream
    ಶೂನ್ಯ ಹರಿವು_ಆಳ_ಕೂಡಿಸು(ಪೂರ್ಣಾಂಕ delta) { m_ಹರಿವು_ಆಳ += delta; }
    ಪೂರ್ಣಾಂಕ ಹರಿವು_ಆಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹರಿವು_ಆಳ; }
    // Utility
    ಸ್ಥಿರ ಶೂನ್ಯ ಗಣ_YYDebug(bool on);
    ಸ್ಥಿರ string cleanDbgStrg(ಸ್ಥಿರಾಂಕ string& ಒಳ);

ಖಾಸಗಿ:
    string currentUnreadChars();
    string ಕೊನೆ_ದ_ಹರಿವು(bool& againr);
    ಶೂನ್ಯ init_ಮೊದಲ_ಬಫರ್(File_ಸಾಲು* filelinep);
    ಶೂನ್ಯ ಪರಿಶೀಲನೆ_Switch_ಹರಿವು(VPre_ಹರಿವು* streamp);
};

ಶೂನ್ಯ VPre_ಹರಿವು::lex_ಹರಿವು_ಆಳ_ಕೂಡಿಸು(ಪೂರ್ಣಾಂಕ delta) { m_lexp->ಹರಿವು_ಆಳ_ಕೂಡಿಸು(delta); }

#ಆದರೆ_ಅಂತ್ಯ  // Guard
