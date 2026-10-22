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

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"

#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <stack>

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
#define VP_લીટી         264
#define VP_UNDEFINEALL  265

#define VP_પ્રતીક       300
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
#define VP_પ્રતીક_JOIN  312
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
સંરચના yy_buffer_state;
પ્રકારવ્યાખ્યા સંરચના yy_buffer_state* YY_BUFFER_STATE;
# define YY_BUF_SIZE 16384
#endif
// clang-format on

બાહ્ય પૂર્ણાંક yylex();
બાહ્ય શૂન્ય yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
બાહ્ય અક્ષર* yyourtext();
બાહ્ય size_t yyourleng();
બાહ્ય શૂન્ય yyourtext(અચલ અક્ષર* textp, size_t size);  // Must call with static

YY_BUFFER_STATE yy_create_buffer(FILE* file, પૂર્ણાંક size);
શૂન્ય yy_switch_to_buffer(YY_BUFFER_STATE new_buffer);
શૂન્ય yy_delete_buffer(YY_BUFFER_STATE b);
પૂર્ણાંક yylex_destroy();

//======================================================================

#define KEEPCMT_SUB 2
#define KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
જાહેર:
    File_લીટી* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* અચલ m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    પૂર્ણાંક m_ignNewlines = 0;  // Ignore multiline newlines
    પૂર્ણાંક m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_લીટી* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

ખાનગી:
    અંતઃરેખ શૂન્ય lexStreamDepthAdd(પૂર્ણાંક delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
ગણના class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
જાહેર:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* અચલ m_preimpp;  // Preprocessor lexor belongs to
    std::ઢગલો<VPreStream*> m_streamp_ઢગલો;  // Stack of processing files
    પૂર્ણાંક m_streamDepth = 0;  // Depth of stream processing
    YY_BUFFER_STATE m_bufferState;  // Flex state
    File_લીટી* m_tokFilelinep;  // Starting position of current token
    std::deque<File_લીટી> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    સ્થિર V3PreLex* s_currentLexp;  ///< Current lexing point
    પૂર્ણાંક m_keepComments = 0;  ///< Emit comments in output text
    પૂર્ણાંક m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    પૂર્ણાંક m_formalLevel = 0;  // Parenthesis counting inside def formals
    પૂર્ણાંક m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_કિંમત;  // Definition value being built.
    પૂર્ણાંક m_enterExit = 0;  // For VL_LINE, the enter/exit level
    પૂર્ણાંક m_protLength = 0;  // unencoded length for BASE64
    પૂર્ણાંક m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_લીટી* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        initFirstBuffer(filelinep);
    }
    ~V3PreLex() {
        જ્યારે (!m_streamp_ઢગલો.empty()) {
            delete m_streamp_ઢગલો.ટોચ();
            m_streamp_ઢગલો.કાઢો();
        }
        VL_DO_CLEAR(yy_delete_buffer(m_bufferState), m_bufferState = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_લીટી* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { પાછા m_streamp_ઢગલો.ટોચ(); }  // Can't be empty, "EOF" is on top
    File_લીટી* curFilelinep() { પાછા curStreamp()->m_curFilelinep; }
    શૂન્ય curFilelinep(File_લીટી* fl) { curStreamp()->m_curFilelinep = fl; }
    શૂન્ય append_Def_કિંમત(અચલ અક્ષર* textp, size_t len) { m_def_કિંમત.append(textp, len); }
    શૂન્ય લીટી_Directive(અચલ અક્ષર* textp);
    શૂન્ય linenoInc() {
        જો (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } નહીંતર {
            curFilelinep()->linenoInc();
        }
    }
    શૂન્ય warnBackslashSpace();
    શૂન્ય verilatorCmtLint(અચલ અક્ષર* textp, bool turnOff);
    શૂન્ય verilatorCmtLintRestore();
    શૂન્ય verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    શૂન્ય ધકેલો_State_Def_Arg(પૂર્ણાંક level);
    શૂન્ય ધકેલો_State_Def_Form();
    શૂન્ય ધકેલો_State_Def_કિંમત();
    શૂન્ય ધકેલો_State_પદાવલિ();
    શૂન્ય ધકેલો_State_Inc_Filename();
    શૂન્ય ધકેલો_State_Passthru();
    શૂન્ય scanNewFile(File_લીટી* filelinep);
    શૂન્ય scanBytes(અચલ string& str);
    શૂન્ય scanBytesBack(અચલ string& str);
    size_t આગત_To_Lex(અક્ષર* buf, size_t max_size);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_BUFFER_STATE currentBuffer();
    પૂર્ણાંક lex();
    પૂર્ણાંક currentStartState() અચલ;
    શૂન્ય dumpSummary();
    શૂન્ય dump_ઢગલો();
    શૂન્ય unused();
    // Called by VPreStream
    શૂન્ય streamDepthAdd(પૂર્ણાંક delta) { m_streamDepth += delta; }
    પૂર્ણાંક streamDepth() અચલ { પાછા m_streamDepth; }
    // Utility
    સ્થિર શૂન્ય setYYDebug(bool on);
    સ્થિર string cleanDbgStrg(અચલ string& in);

ખાનગી:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    શૂન્ય initFirstBuffer(File_લીટી* filelinep);
    શૂન્ય scanSwitchStream(VPreStream* streamp);
};

શૂન્ય VPreStream::lexStreamDepthAdd(પૂર્ણાંક delta) { m_lexp->streamDepthAdd(delta); }

#endif  // Guard
