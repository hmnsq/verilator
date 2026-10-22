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

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"

#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <stack>

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
#define VP_ଧାଡ଼ି         264
#define VP_UNDEFINEALL  265

#define VP_ପ୍ରତୀକ       300
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
#define VP_ପ୍ରତୀକ_JOIN  312
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
ସଂରଚନା yy_buffer_state;
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା yy_buffer_state* YY_BUFFER_STATE;
# define YY_BUF_SIZE 16384
#endif
// clang-format on

ବାହ୍ୟ ପୂର୍ଣ୍ଣସଂଖ୍ୟା yylex();
ବାହ୍ୟ ଶୂନ୍ୟ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
ବାହ୍ୟ ଅକ୍ଷର* yyourtext();
ବାହ୍ୟ size_t yyourleng();
ବାହ୍ୟ ଶୂନ୍ୟ yyourtext(ଅଚଳ ଅକ୍ଷର* textp, size_t size);  // Must call with static

YY_BUFFER_STATE yy_create_buffer(FILE* file, ପୂର୍ଣ୍ଣସଂଖ୍ୟା size);
ଶୂନ୍ୟ yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);
ଶୂନ୍ୟ yy_delete_buffer(YY_BUFFER_STATE b);
ପୂର୍ଣ୍ଣସଂଖ୍ୟା yylex_destroy();

//======================================================================

#define KEEPCMT_SUB 2
#define KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
ସର୍ବସାଧାରଣ:
    File_ଧାଡ଼ି* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ଅଚଳ m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ignNewlines = 0;  // Ignore multiline newlines
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_ଧାଡ଼ି* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

ବ୍ୟକ୍ତିଗତ:
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ lexStreamDepthAdd(ପୂର୍ଣ୍ଣସଂଖ୍ୟା delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
ଗଣନା class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
ସର୍ବସାଧାରଣ:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ଅଚଳ m_preimpp;  // Preprocessor lexor belongs to
    std::ଗଦା<VPreStream*> m_streamp_ଗଦା;  // Stack of processing files
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_streamDepth = 0;  // Depth of stream processing
    YY_BUFFER_STATE m_bufferState;  // Flex state
    File_ଧାଡ଼ି* m_tokFilelinep;  // Starting position of current token
    std::deque<File_ଧାଡ଼ି> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    ସ୍ଥିର V3PreLex* s_currentLexp;  ///< Current lexing point
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_keepComments = 0;  ///< Emit comments in output text
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_formalLevel = 0;  // Parenthesis counting inside def formals
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_ମୂଲ୍ୟ;  // Definition value being built.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_enterExit = 0;  // For VL_LINE, the enter/exit level
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_protLength = 0;  // unencoded length for BASE64
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_ଧାଡ଼ି* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        initFirstBuffer(filelinep);
    }
    ~V3PreLex() {
        ଯେତେବେଳେ (!m_streamp_ଗଦା.empty()) {
            delete m_streamp_ଗଦା.ଶୀର୍ଷ();
            m_streamp_ଗଦା.କାଢ଼();
        }
        VL_DO_CLEAR(yy_delete_buffer(m_bufferState), m_bufferState = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_ଧାଡ଼ି* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { ଫେରନ୍ତୁ m_streamp_ଗଦା.ଶୀର୍ଷ(); }  // Can't be empty, "EOF" is on top
    File_ଧାଡ଼ି* curFilelinep() { ଫେରନ୍ତୁ curStreamp()->m_curFilelinep; }
    ଶୂନ୍ୟ curFilelinep(File_ଧାଡ଼ି* fl) { curStreamp()->m_curFilelinep = fl; }
    ଶୂନ୍ୟ append_Def_ମୂଲ୍ୟ(ଅଚଳ ଅକ୍ଷର* textp, size_t len) { m_def_ମୂଲ୍ୟ.append(textp, len); }
    ଶୂନ୍ୟ ଧାଡ଼ି_Directive(ଅଚଳ ଅକ୍ଷର* textp);
    ଶୂନ୍ୟ linenoInc() {
        ଯଦି (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } ଅନ୍ୟଥା {
            curFilelinep()->linenoInc();
        }
    }
    ଶୂନ୍ୟ warnBackslashSpace();
    ଶୂନ୍ୟ verilatorCmtLint(ଅଚଳ ଅକ୍ଷର* textp, bool turnOff);
    ଶୂନ୍ୟ verilatorCmtLintRestore();
    ଶୂନ୍ୟ verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    ଶୂନ୍ୟ ଠେଲ_State_Def_Arg(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level);
    ଶୂନ୍ୟ ଠେଲ_State_Def_Form();
    ଶୂନ୍ୟ ଠେଲ_State_Def_ମୂଲ୍ୟ();
    ଶୂନ୍ୟ ଠେଲ_State_ପଦସମଷ୍ଟି();
    ଶୂନ୍ୟ ଠେଲ_State_Inc_Filename();
    ଶୂନ୍ୟ ଠେଲ_State_Passthru();
    ଶୂନ୍ୟ scanNewFile(File_ଧାଡ଼ି* filelinep);
    ଶୂନ୍ୟ scanBytes(ଅଚଳ string& str);
    ଶୂନ୍ୟ scanBytesBack(ଅଚଳ string& str);
    size_t ନିବେଶ_To_Lex(ଅକ୍ଷର* buf, size_t max_size);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_BUFFER_STATE currentBuffer();
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lex();
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା currentStartState() ଅଚଳ;
    ଶୂନ୍ୟ dumpSummary();
    ଶୂନ୍ୟ dump_ଗଦା();
    ଶୂନ୍ୟ unused();
    // Called by VPreStream
    ଶୂନ୍ୟ streamDepthAdd(ପୂର୍ଣ୍ଣସଂଖ୍ୟା delta) { m_streamDepth += delta; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା streamDepth() ଅଚଳ { ଫେରନ୍ତୁ m_streamDepth; }
    // Utility
    ସ୍ଥିର ଶୂନ୍ୟ setYYDebug(bool on);
    ସ୍ଥିର string cleanDbgStrg(ଅଚଳ string& in);

ବ୍ୟକ୍ତିଗତ:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    ଶୂନ୍ୟ initFirstBuffer(File_ଧାଡ଼ି* filelinep);
    ଶୂନ୍ୟ scanSwitchStream(VPreStream* streamp);
};

ଶୂନ୍ୟ VPreStream::lexStreamDepthAdd(ପୂର୍ଣ୍ଣସଂଖ୍ୟା delta) { m_lexp->streamDepthAdd(delta); }

#endif  // Guard
