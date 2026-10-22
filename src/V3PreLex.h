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

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"

#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <stack>

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
#define VP_വരി         264
#define VP_UNDEFINEALL  265

#define VP_ചിഹ്നം       300
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
#define VP_ചിഹ്നം_JOIN  312
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
ഘടന yy_buffer_state;
തരനിർവചനം ഘടന yy_buffer_state* YY_BUFFER_STATE;
# define YY_BUF_SIZE 16384
#endif
// clang-format on

ബാഹ്യ പൂർണ്ണസംഖ്യ yylex();
ബാഹ്യ ശൂന്യം yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
ബാഹ്യ അക്ഷരം* yyourtext();
ബാഹ്യ size_t yyourleng();
ബാഹ്യ ശൂന്യം yyourtext(സ്ഥിരം അക്ഷരം* textp, size_t size);  // Must call with static

YY_BUFFER_STATE yy_create_buffer(FILE* file, പൂർണ്ണസംഖ്യ size);
ശൂന്യം yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);
ശൂന്യം yy_delete_buffer(YY_BUFFER_STATE b);
പൂർണ്ണസംഖ്യ yylex_destroy();

//======================================================================

#define KEEPCMT_SUB 2
#define KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
പൊതു:
    File_വരി* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* സ്ഥിരം m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    പൂർണ്ണസംഖ്യ m_ignNewlines = 0;  // Ignore multiline newlines
    പൂർണ്ണസംഖ്യ m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_വരി* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

സ്വകാര്യ:
    വരിയിൽ ശൂന്യം lexStreamDepthAdd(പൂർണ്ണസംഖ്യ delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
എണ്ണൽ class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
പൊതു:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* സ്ഥിരം m_preimpp;  // Preprocessor lexor belongs to
    std::അട്ടി<VPreStream*> m_streamp_അട്ടി;  // Stack of processing files
    പൂർണ്ണസംഖ്യ m_streamDepth = 0;  // Depth of stream processing
    YY_BUFFER_STATE m_bufferState;  // Flex state
    File_വരി* m_tokFilelinep;  // Starting position of current token
    std::deque<File_വരി> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    സ്ഥിതം V3PreLex* s_currentLexp;  ///< Current lexing point
    പൂർണ്ണസംഖ്യ m_keepComments = 0;  ///< Emit comments in output text
    പൂർണ്ണസംഖ്യ m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    പൂർണ്ണസംഖ്യ m_formalLevel = 0;  // Parenthesis counting inside def formals
    പൂർണ്ണസംഖ്യ m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_മൂല്യം;  // Definition value being built.
    പൂർണ്ണസംഖ്യ m_enterExit = 0;  // For VL_LINE, the enter/exit level
    പൂർണ്ണസംഖ്യ m_protLength = 0;  // unencoded length for BASE64
    പൂർണ്ണസംഖ്യ m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_വരി* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        initFirstBuffer(filelinep);
    }
    ~V3PreLex() {
        ആകുമ്പോൾ (!m_streamp_അട്ടി.empty()) {
            delete m_streamp_അട്ടി.മുകൾ();
            m_streamp_അട്ടി.എടുക്കുക();
        }
        VL_DO_CLEAR(yy_delete_buffer(m_bufferState), m_bufferState = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_വരി* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { തിരികെ m_streamp_അട്ടി.മുകൾ(); }  // Can't be empty, "EOF" is on top
    File_വരി* curFilelinep() { തിരികെ curStreamp()->m_curFilelinep; }
    ശൂന്യം curFilelinep(File_വരി* fl) { curStreamp()->m_curFilelinep = fl; }
    ശൂന്യം append_Def_മൂല്യം(സ്ഥിരം അക്ഷരം* textp, size_t len) { m_def_മൂല്യം.append(textp, len); }
    ശൂന്യം വരി_Directive(സ്ഥിരം അക്ഷരം* textp);
    ശൂന്യം linenoInc() {
        എങ്കിൽ (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } അല്ലെങ്കിൽ {
            curFilelinep()->linenoInc();
        }
    }
    ശൂന്യം warnBackslashSpace();
    ശൂന്യം verilatorCmtLint(സ്ഥിരം അക്ഷരം* textp, bool turnOff);
    ശൂന്യം verilatorCmtLintRestore();
    ശൂന്യം verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    ശൂന്യം തള്ളുക_State_Def_Arg(പൂർണ്ണസംഖ്യ level);
    ശൂന്യം തള്ളുക_State_Def_Form();
    ശൂന്യം തള്ളുക_State_Def_മൂല്യം();
    ശൂന്യം തള്ളുക_State_വ്യഞ്ജകം();
    ശൂന്യം തള്ളുക_State_Inc_Filename();
    ശൂന്യം തള്ളുക_State_Passthru();
    ശൂന്യം scanNewFile(File_വരി* filelinep);
    ശൂന്യം scanBytes(സ്ഥിരം string& str);
    ശൂന്യം scanBytesBack(സ്ഥിരം string& str);
    size_t നിവേശം_To_Lex(അക്ഷരം* buf, size_t max_size);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_BUFFER_STATE currentBuffer();
    പൂർണ്ണസംഖ്യ lex();
    പൂർണ്ണസംഖ്യ currentStartState() സ്ഥിരം;
    ശൂന്യം dumpSummary();
    ശൂന്യം dump_അട്ടി();
    ശൂന്യം unused();
    // Called by VPreStream
    ശൂന്യം streamDepthAdd(പൂർണ്ണസംഖ്യ delta) { m_streamDepth += delta; }
    പൂർണ്ണസംഖ്യ streamDepth() സ്ഥിരം { തിരികെ m_streamDepth; }
    // Utility
    സ്ഥിതം ശൂന്യം setYYDebug(bool on);
    സ്ഥിതം string cleanDbgStrg(സ്ഥിരം string& in);

സ്വകാര്യ:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    ശൂന്യം initFirstBuffer(File_വരി* filelinep);
    ശൂന്യം scanSwitchStream(VPreStream* streamp);
};

ശൂന്യം VPreStream::lexStreamDepthAdd(പൂർണ്ണസംഖ്യ delta) { m_lexp->streamDepthAdd(delta); }

#endif  // Guard
