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

#ifndef VERILATOR_VPRELEX_H_  // Guard
#define VERILATOR_VPRELEX_H_

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
#define VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#define VP_EOF_ERROR    400

#define VP_INCLUDE      256
#define VP_IFDEF        257
#define VP_IFNDEF       258
#define VP_ENDIF        259
#define VP_UNDEF        260
#define VP_DEFINE       261
#define VP_ELSE         262
#define VP_ELSIF        263
#define VP_ಸಾಲು         264
#define VP_UNDEFINEALL  265

#define VP_ಚಿಹ್ನೆ       300
#define VP_STRING       301
#define VP_DEFVALUE     302
#define VP_COMMENT      303
#define VP_TEXT         304
#define VP_WHITE        305
#define VP_DEFREF       306
#define VP_DEFARG       307
#define VP_ERROR        308
#define VP_DEFFORM      309
#define VP_STRIFY       310
#define VP_BACKQUOTE    311
#define VP_ಚಿಹ್ನೆ_JOIN  312
#define VP_DEFREF_JOIN  313
#define VP_JOIN         314

#define VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#ifndef yy_create_buffer
# define yy_create_buffer V3PreLex_create_buffer
# define yy_delete_buffer V3PreLex_delete_buffer
# define yy_scan_buffer V3PreLex_scan_buffer
# define yy_scan_string V3PreLex_scan_string
# define yy_scan_bytes V3PreLex_scan_bytes
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_buffer V3PreLex_init_buffer
# define yy_flush_buffer V3PreLex_flush_buffer
# define yy_load_buffer_state V3PreLex_load_buffer_state
# define yy_switch_to_buffer V3PreLex_switch_to_buffer
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#endif

#ifndef yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#endif

#ifndef YY_BUFFER_STATE
ಸಂರಚನಾ yy_buffer_state;
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ yy_buffer_state* YY_BUFFER_STATE;
# define YY_BUF_SIZE 16384
#endif
// clang-format on

ಬಾಹ್ಯ ಪೂರ್ಣಾಙ್ಕ yylex();
ಬಾಹ್ಯ ಶೂನ್ಯ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
ಬಾಹ್ಯ ವರ್ಣ* yyourtext();
ಬಾಹ್ಯ size_t yyourleng();
ಬಾಹ್ಯ ಶೂನ್ಯ yyourtext(ಧ್ರುವ ವರ್ಣ* textp, size_t size);  // Must call with static

YY_BUFFER_STATE yy_create_buffer(FILE* file, ಪೂರ್ಣಾಙ್ಕ size);
ಶೂನ್ಯ yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);
ಶೂನ್ಯ yy_delete_buffer(YY_BUFFER_STATE b);
ಪೂರ್ಣಾಙ್ಕ yylex_destroy();

//======================================================================

#define KEEPCMT_SUB 2
#define KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
ಸಾರ್ವಜನಿಕ:
    File_ಸಾಲು* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ಧ್ರುವ m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    ಪೂರ್ಣಾಙ್ಕ m_ignNewlines = 0;  // Ignore multiline newlines
    ಪೂರ್ಣಾಙ್ಕ m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_ಸಾಲು* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

ಗುಪ್ತ:
    ಅನ್ತಃಸ್ಥ ಶೂನ್ಯ lexStreamDepthAdd(ಪೂರ್ಣಾಙ್ಕ delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
ಗಣನಾ class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
ಸಾರ್ವಜನಿಕ:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ಧ್ರುವ m_preimpp;  // Preprocessor lexor belongs to
    std::ರಾಶಿ<VPreStream*> m_streamp_ರಾಶಿ;  // Stack of processing files
    ಪೂರ್ಣಾಙ್ಕ m_streamDepth = 0;  // Depth of stream processing
    YY_BUFFER_STATE m_bufferState;  // Flex state
    File_ಸಾಲು* m_tokFilelinep;  // Starting position of current token
    std::deque<File_ಸಾಲು> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    ಸ್ಥಿರ V3PreLex* s_currentLexp;  ///< Current lexing point
    ಪೂರ್ಣಾಙ್ಕ m_keepComments = 0;  ///< Emit comments in output text
    ಪೂರ್ಣಾಙ್ಕ m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    ಪೂರ್ಣಾಙ್ಕ m_formalLevel = 0;  // Parenthesis counting inside def formals
    ಪೂರ್ಣಾಙ್ಕ m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_ಮೌಲ್ಯ;  // Definition value being built.
    ಪೂರ್ಣಾಙ್ಕ m_enterExit = 0;  // For VL_LINE, the enter/exit level
    ಪೂರ್ಣಾಙ್ಕ m_protLength = 0;  // unencoded length for BASE64
    ಪೂರ್ಣಾಙ್ಕ m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_ಸಾಲು* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        initFirstBuffer(filelinep);
    }
    ~V3PreLex() {
        ಯಾವತ್ (!m_streamp_ರಾಶಿ.empty()) {
            delete m_streamp_ರಾಶಿ.ತುದಿ();
            m_streamp_ರಾಶಿ.ತೆಗೆ();
        }
        VL_DO_CLEAR(yy_delete_buffer(m_bufferState), m_bufferState = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_ಸಾಲು* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { ಪ್ರತ್ಯಾಗಮ m_streamp_ರಾಶಿ.ತುದಿ(); }  // Can't be empty, "EOF" is on top
    File_ಸಾಲು* curFilelinep() { ಪ್ರತ್ಯಾಗಮ curStreamp()->m_curFilelinep; }
    ಶೂನ್ಯ curFilelinep(File_ಸಾಲು* fl) { curStreamp()->m_curFilelinep = fl; }
    ಶೂನ್ಯ append_Def_ಮೌಲ್ಯ(ಧ್ರುವ ವರ್ಣ* textp, size_t len) { m_def_ಮೌಲ್ಯ.append(textp, len); }
    ಶೂನ್ಯ ಸಾಲು_Directive(ಧ್ರುವ ವರ್ಣ* textp);
    ಶೂನ್ಯ linenoInc() {
        ಯದಿ (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } ಅನ್ಯಥಾ {
            curFilelinep()->linenoInc();
        }
    }
    ಶೂನ್ಯ warnBackslashSpace();
    ಶೂನ್ಯ verilatorCmtLint(ಧ್ರುವ ವರ್ಣ* textp, bool turnOff);
    ಶೂನ್ಯ verilatorCmtLintRestore();
    ಶೂನ್ಯ verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    ಶೂನ್ಯ ತಳ್ಳು_State_Def_Arg(ಪೂರ್ಣಾಙ್ಕ level);
    ಶೂನ್ಯ ತಳ್ಳು_State_Def_Form();
    ಶೂನ್ಯ ತಳ್ಳು_State_Def_ಮೌಲ್ಯ();
    ಶೂನ್ಯ ತಳ್ಳು_State_ಅಭಿವ್ಯಕ್ತಿ();
    ಶೂನ್ಯ ತಳ್ಳು_State_Inc_Filename();
    ಶೂನ್ಯ ತಳ್ಳು_State_Passthru();
    ಶೂನ್ಯ scanNewFile(File_ಸಾಲು* filelinep);
    ಶೂನ್ಯ scanBytes(ಧ್ರುವ string& str);
    ಶೂನ್ಯ scanBytesBack(ಧ್ರುವ string& str);
    size_t ಒಳಹರಿವು_To_Lex(ವರ್ಣ* buf, size_t max_size);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_BUFFER_STATE currentBuffer();
    ಪೂರ್ಣಾಙ್ಕ lex();
    ಪೂರ್ಣಾಙ್ಕ currentStartState() ಧ್ರುವ;
    ಶೂನ್ಯ dumpSummary();
    ಶೂನ್ಯ dump_ರಾಶಿ();
    ಶೂನ್ಯ unused();
    // Called by VPreStream
    ಶೂನ್ಯ streamDepthAdd(ಪೂರ್ಣಾಙ್ಕ delta) { m_streamDepth += delta; }
    ಪೂರ್ಣಾಙ್ಕ streamDepth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_streamDepth; }
    // Utility
    ಸ್ಥಿರ ಶೂನ್ಯ setYYDebug(bool on);
    ಸ್ಥಿರ string cleanDbgStrg(ಧ್ರುವ string& in);

ಗುಪ್ತ:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    ಶೂನ್ಯ initFirstBuffer(File_ಸಾಲು* filelinep);
    ಶೂನ್ಯ scanSwitchStream(VPreStream* streamp);
};

ಶೂನ್ಯ VPreStream::lexStreamDepthAdd(ಪೂರ್ಣಾಙ್ಕ delta) { m_lexp->streamDepthAdd(delta); }

#endif  // Guard
