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

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"

#অন্তর্ভাব <deque>
#অন্তর্ভাব <stack>

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
#define VP_সারি         264
#define VP_UNDEFINEALL  265

#define VP_প্রতীক       300
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
#define VP_প্রতীক_JOIN  312
#define VP_DEFREF_JOIN  313
#define VP_JOIN         314

#define VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#ifndef yy_create_বাফার
# define yy_create_বাফার V3_Pre_Lex_create_বাফার
# define yy_delete_বাফার V3_Pre_Lex_delete_বাফার
# define yy_scan_বাফার V3_Pre_Lex_scan_বাফার
# define yy_scan_string V3PreLex_scan_string
# define yy_scan_bytes V3PreLex_scan_bytes
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_বাফার V3_Pre_Lex_init_বাফার
# define yy_flush_বাফার V3_Pre_Lex_flush_বাফার
# define yy_load_বাফার_state V3_Pre_Lex_load_বাফার_state
# define yy_switch_to_বাফার V3_Pre_Lex_switch_to_বাফার
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

#ifndef YY_বাফার_STATE
কাঠামো yy_বাফার_state;
প্রকারলেখ কাঠামো yy_বাফার_state* YY_বাফার_STATE;
# define YY_BUF_আকার 16384
#endif
// clang-format on

বাহ্যিক পূর্ণসংখ্যা yylex();
বাহ্যিক শূন্য yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
বাহ্যিক অক্ষর* yyourtext();
বাহ্যিক আকার_t yyourleng();
বাহ্যিক শূন্য yyourtext(ধ্রুবক অক্ষর* textp, আকার_t আকার_2);  // Must call with static

YY_বাফার_STATE yy_create_বাফার(FILE* file, পূর্ণসংখ্যা আকার_2);
শূন্য yy_switch_to_বাফার(YY_বাফার_STATE new_বাফার);
শূন্য yy_delete_বাফার(YY_বাফার_STATE b);
পূর্ণসংখ্যা yylex_destroy();

//======================================================================

#define KEEPCMT_SUB 2
#define KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
সর্বজনীন:
    File_সারি* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ধ্রুবক m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    পূর্ণসংখ্যা m_ignNewlines = 0;  // Ignore multiline newlines
    পূর্ণসংখ্যা m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_সারি* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

ব্যক্তিগত:
    অন্তঃরেখ শূন্য lexStreamDepthAdd(পূর্ণসংখ্যা delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
গণনা class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
সর্বজনীন:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ধ্রুবক m_preimpp;  // Preprocessor lexor belongs to
    std::স্তূপ<VPreStream*> m_streamp_স্তূপ;  // Stack of processing files
    পূর্ণসংখ্যা m_streamDepth = 0;  // Depth of stream processing
    YY_বাফার_STATE m_বাফার_State;  // Flex state
    File_সারি* m_tokFilelinep;  // Starting position of current token
    std::deque<File_সারি> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    স্থির V3PreLex* s_currentLexp;  ///< Current lexing point
    পূর্ণসংখ্যা m_keepComments = 0;  ///< Emit comments in output text
    পূর্ণসংখ্যা m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    পূর্ণসংখ্যা m_formalLevel = 0;  // Parenthesis counting inside def formals
    পূর্ণসংখ্যা m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_মান;  // Definition value being built.
    পূর্ণসংখ্যা m_enterExit = 0;  // For VL_LINE, the enter/exit level
    পূর্ণসংখ্যা m_prot_দৈর্ঘ্য = 0;  // unencoded length for BASE64
    পূর্ণসংখ্যা m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_সারি* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_First_বাফার(filelinep);
    }
    ~V3PreLex() {
        যখন (!m_streamp_স্তূপ.empty()) {
            delete m_streamp_স্তূপ.শীর্ষ();
            m_streamp_স্তূপ.বের_করো();
        }
        VL_DO_CLEAR(yy_delete_বাফার(m_বাফার_State), m_বাফার_State = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_সারি* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { ফেরত m_streamp_স্তূপ.শীর্ষ(); }  // Can't be empty, "EOF" is on top
    File_সারি* curFilelinep() { ফেরত curStreamp()->m_curFilelinep; }
    শূন্য curFilelinep(File_সারি* fl) { curStreamp()->m_curFilelinep = fl; }
    শূন্য append_Def_মান(ধ্রুবক অক্ষর* textp, আকার_t len) { m_def_মান.append(textp, len); }
    শূন্য সারি_Directive(ধ্রুবক অক্ষর* textp);
    শূন্য linenoInc() {
        যদি (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } অন্যথা {
            curFilelinep()->linenoInc();
        }
    }
    শূন্য warnBackslashSpace();
    শূন্য verilatorCmtLint(ধ্রুবক অক্ষর* textp, bool turnOff);
    শূন্য verilatorCmtLintRestore();
    শূন্য verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    শূন্য ঠেলো_State_Def_Arg(পূর্ণসংখ্যা level);
    শূন্য ঠেলো_State_Def_Form();
    শূন্য ঠেলো_State_Def_মান();
    শূন্য ঠেলো_State_রাশিমালা();
    শূন্য ঠেলো_State_Inc_Filename();
    শূন্য ঠেলো_State_Passthru();
    শূন্য scanNewFile(File_সারি* filelinep);
    শূন্য scanBytes(ধ্রুবক string& str);
    শূন্য scanBytesBack(ধ্রুবক string& str);
    আকার_t প্রবেশ_To_Lex(অক্ষর* buf, আকার_t max_আকার_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_বাফার_STATE current_বাফার();
    পূর্ণসংখ্যা lex();
    পূর্ণসংখ্যা currentStartState() ধ্রুবক;
    শূন্য dumpSummary();
    শূন্য dump_স্তূপ();
    শূন্য unused();
    // Called by VPreStream
    শূন্য streamDepthAdd(পূর্ণসংখ্যা delta) { m_streamDepth += delta; }
    পূর্ণসংখ্যা streamDepth() ধ্রুবক { ফেরত m_streamDepth; }
    // Utility
    স্থির শূন্য setYYDebug(bool on);
    স্থির string cleanDbgStrg(ধ্রুবক string& in);

ব্যক্তিগত:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    শূন্য init_First_বাফার(File_সারি* filelinep);
    শূন্য scanSwitchStream(VPreStream* streamp);
};

শূন্য VPreStream::lexStreamDepthAdd(পূর্ণসংখ্যা delta) { m_lexp->streamDepthAdd(delta); }

#endif  // Guard
