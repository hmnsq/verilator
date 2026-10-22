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

#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"

#অন্তর্ভাৱ <deque>
#অন্তর্ভাৱ <stack>

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
#define VP_সাৰি         264
#define VP_UNDEFINEALL  265

#define VP_প্ৰতীক       300
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
#define VP_প্ৰতীক_JOIN  312
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
সংরচনা yy_buffer_state;
প্রকারনির্দেশ সংরচনা yy_buffer_state* YY_BUFFER_STATE;
# define YY_BUF_SIZE 16384
#endif
// clang-format on

বাহ্য পূর্ণাঙ্ক yylex();
বাহ্য শূন্য yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
বাহ্য ৱর্ণ* yyourtext();
বাহ্য size_t yyourleng();
বাহ্য শূন্য yyourtext(ধ্রুৱ ৱর্ণ* textp, size_t size);  // Must call with static

YY_BUFFER_STATE yy_create_buffer(FILE* file, পূর্ণাঙ্ক size);
শূন্য yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);
শূন্য yy_delete_buffer(YY_BUFFER_STATE b);
পূর্ণাঙ্ক yylex_destroy();

//======================================================================

#define KEEPCMT_SUB 2
#define KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
সার্ৱজনিক:
    File_সাৰি* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ধ্রুৱ m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    পূর্ণাঙ্ক m_ignNewlines = 0;  // Ignore multiline newlines
    পূর্ণাঙ্ক m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_সাৰি* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

গুপ্ত:
    অন্তঃস্থ শূন্য lexStreamDepthAdd(পূর্ণাঙ্ক delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
গণনা class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
সার্ৱজনিক:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ধ্রুৱ m_preimpp;  // Preprocessor lexor belongs to
    std::স্তূপ<VPreStream*> m_streamp_স্তূপ;  // Stack of processing files
    পূর্ণাঙ্ক m_streamDepth = 0;  // Depth of stream processing
    YY_BUFFER_STATE m_bufferState;  // Flex state
    File_সাৰি* m_tokFilelinep;  // Starting position of current token
    std::deque<File_সাৰি> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    স্থির V3PreLex* s_currentLexp;  ///< Current lexing point
    পূর্ণাঙ্ক m_keepComments = 0;  ///< Emit comments in output text
    পূর্ণাঙ্ক m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    পূর্ণাঙ্ক m_formalLevel = 0;  // Parenthesis counting inside def formals
    পূর্ণাঙ্ক m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_মান;  // Definition value being built.
    পূর্ণাঙ্ক m_enterExit = 0;  // For VL_LINE, the enter/exit level
    পূর্ণাঙ্ক m_protLength = 0;  // unencoded length for BASE64
    পূর্ণাঙ্ক m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_সাৰি* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        initFirstBuffer(filelinep);
    }
    ~V3PreLex() {
        যেতিয়ালৈ (!m_streamp_স্তূপ.empty()) {
            delete m_streamp_স্তূপ.শীৰ্ষ();
            m_streamp_স্তূপ.বেৰ_কৰো();
        }
        VL_DO_CLEAR(yy_delete_buffer(m_bufferState), m_bufferState = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_সাৰি* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { ঘূৰাই m_streamp_স্তূপ.শীৰ্ষ(); }  // Can't be empty, "EOF" is on top
    File_সাৰি* curFilelinep() { ঘূৰাই curStreamp()->m_curFilelinep; }
    শূন্য curFilelinep(File_সাৰি* fl) { curStreamp()->m_curFilelinep = fl; }
    শূন্য append_Def_মান(ধ্রুৱ ৱর্ণ* textp, size_t len) { m_def_মান.append(textp, len); }
    শূন্য সাৰি_Directive(ধ্রুৱ ৱর্ণ* textp);
    শূন্য linenoInc() {
        যদি (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } অন্যথা {
            curFilelinep()->linenoInc();
        }
    }
    শূন্য warnBackslashSpace();
    শূন্য verilatorCmtLint(ধ্রুৱ ৱর্ণ* textp, bool turnOff);
    শূন্য verilatorCmtLintRestore();
    শূন্য verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    শূন্য ঠেলো_State_Def_Arg(পূর্ণাঙ্ক level);
    শূন্য ঠেলো_State_Def_Form();
    শূন্য ঠেলো_State_Def_মান();
    শূন্য ঠেলো_State_ৰাশিমালা();
    শূন্য ঠেলো_State_Inc_Filename();
    শূন্য ঠেলো_State_Passthru();
    শূন্য scanNewFile(File_সাৰি* filelinep);
    শূন্য scanBytes(ধ্রুৱ string& str);
    শূন্য scanBytesBack(ধ্রুৱ string& str);
    size_t প্ৰবেশ_To_Lex(ৱর্ণ* buf, size_t max_size);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_BUFFER_STATE currentBuffer();
    পূর্ণাঙ্ক lex();
    পূর্ণাঙ্ক currentStartState() ধ্রুৱ;
    শূন্য dumpSummary();
    শূন্য dump_স্তূপ();
    শূন্য unused();
    // Called by VPreStream
    শূন্য streamDepthAdd(পূর্ণাঙ্ক delta) { m_streamDepth += delta; }
    পূর্ণাঙ্ক streamDepth() ধ্রুৱ { ঘূৰাই m_streamDepth; }
    // Utility
    স্থির শূন্য setYYDebug(bool on);
    স্থির string cleanDbgStrg(ধ্রুৱ string& in);

গুপ্ত:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    শূন্য initFirstBuffer(File_সাৰি* filelinep);
    শূন্য scanSwitchStream(VPreStream* streamp);
};

শূন্য VPreStream::lexStreamDepthAdd(পূর্ণাঙ্ক delta) { m_lexp->streamDepthAdd(delta); }

#endif  // Guard
