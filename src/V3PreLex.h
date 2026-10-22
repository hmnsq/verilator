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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VPRELEX_H_  // Guard
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VPRELEX_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <stack>

//======================================================================

class V3PreLex;
class V3PreProcImp;

// Token codes
// If changing, see V3PreProc.cpp's V3PreProcImp::tokenName()
// clang-format off
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_EOF_ERROR    400

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_INCLUDE      256
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_IFDEF        257
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_IFNDEF       258
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ENDIF        259
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_UNDEF        260
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_DEFINE       261
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ELSE         262
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ELSIF        263
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ਸਤਰ         264
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_UNDEFINEALL  265

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ਚਿੰਨ੍ਹ       300
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_STRING       301
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_DEFVALUE     302
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_COMMENT      303
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ਪਾਠ         304
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_WHITE        305
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_DEFREF       306
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_DEFARG       307
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ERROR        308
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_DEFFORM      309
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_STRIFY       310
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_BACKQUOTE    311
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_ਚਿੰਨ੍ਹ_JOIN  312
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_DEFREF_JOIN  313
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_JOIN         314

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ yy_create_ਬਫਰ
# define yy_create_ਬਫਰ V3_Pre_Lex_create_ਬਫਰ
# define yy_delete_ਬਫਰ V3_Pre_Lex_delete_ਬਫਰ
# define yy_ਜਾਂਚ_ਬਫਰ V3_Pre_Lex_ਜਾਂਚ_ਬਫਰ
# define yy_ਜਾਂਚ_string V3_Pre_Lex_ਜਾਂਚ_string
# define yy_ਜਾਂਚ_ਬਾਈਟਾਂ V3_Pre_Lex_ਜਾਂਚ_ਬਾਈਟਾਂ
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_ਬਫਰ V3_Pre_Lex_init_ਬਫਰ
# define yy_flush_ਬਫਰ V3_Pre_Lex_flush_ਬਫਰ
# define yy_ਲੱਦੋ_ਬਫਰ_state V3_Pre_Lex_ਲੱਦੋ_ਬਫਰ_state
# define yy_switch_ਮੰਜ਼ਿਲ_ਬਫਰ V3_Pre_Lex_switch_ਮੰਜ਼ਿਲ_ਬਫਰ
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#ਜੇ_ਅੰਤ

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#ਜੇ_ਅੰਤ

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ YY_ਬਫਰ_STATE
ਬਣਤਰ yy_ਬਫਰ_state;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ yy_ਬਫਰ_state* YY_ਬਫਰ_STATE;
# define YY_BUF_ਮਾਪ 16384
#ਜੇ_ਅੰਤ
// clang-format on

ਬਾਹਰੀ ਪੂਰਨ yylex();
ਬਾਹਰੀ ਖਾਲੀ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
ਬਾਹਰੀ ਅੱਖਰ* yyourtext();
ਬਾਹਰੀ ਮਾਪ_t yyourleng();
ਬਾਹਰੀ ਖਾਲੀ yyourtext(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਮਾਪ);  // Must call with static

YY_ਬਫਰ_STATE yy_create_ਬਫਰ(FILE* file, ਪੂਰਨ ਮਾਪ);
ਖਾਲੀ yy_switch_ਮੰਜ਼ਿਲ_ਬਫਰ(YY_ਬਫਰ_STATE new_ਬਫਰ);
ਖਾਲੀ yy_delete_ਬਫਰ(YY_ਬਫਰ_STATE b);
ਪੂਰਨ yylex_destroy();

//======================================================================

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ KEEPCMT_SUB 2
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_ਧਾਰਾ final {
ਜਨਤਕ:
    File_ਸਤਰ* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ਅਚਲ m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    ਪੂਰਨ m_ignNewlines = 0;  // Ignore multiline newlines
    ਪੂਰਨ m_ਪਦ_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_ਧਾਰਾ(File_ਸਤਰ* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_ਧਾਰਾ_ਡੂੰਘਾਈ_ਜੋੜੋ(1);
    }
    ~VPre_ਧਾਰਾ() { lex_ਧਾਰਾ_ਡੂੰਘਾਈ_ਜੋੜੋ(-1); }

ਨਿਜੀ:
    ਸਤਰਅੰਦਰ ਖਾਲੀ lex_ਧਾਰਾ_ਡੂੰਘਾਈ_ਜੋੜੋ(ਪੂਰਨ delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
ਗਿਣਤੀ class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
ਜਨਤਕ:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ਅਚਲ m_preimpp;  // Preprocessor lexor belongs to
    std::ਢੇਰ<VPre_ਧਾਰਾ*> m_streamp_ਢੇਰ;  // Stack of processing files
    ਪੂਰਨ m_ਧਾਰਾ_ਡੂੰਘਾਈ = 0;  // Depth of stream processing
    YY_ਬਫਰ_STATE m_ਬਫਰ_State;  // Flex state
    File_ਸਤਰ* m_tokFilelinep;  // Starting position of current token
    std::deque<File_ਸਤਰ> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    ਸਥਿਰ V3PreLex* s_currentLexp;  ///< Current lexing point
    ਪੂਰਨ m_keepComments = 0;  ///< Emit comments in output text
    ਪੂਰਨ m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    ਪੂਰਨ m_formalLevel = 0;  // Parenthesis counting inside def formals
    ਪੂਰਨ m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_ਮੁੱਲ;  // Definition value being built.
    ਪੂਰਨ m_enterExit = 0;  // For VL_LINE, the enter/exit level
    ਪੂਰਨ m_prot_ਲੰਮਾਈ = 0;  // unencoded length for BASE64
    ਪੂਰਨ m_prot_ਬਾਈਟਾਂ = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_ਸਤਰ* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_ਪਹਿਲਾ_ਬਫਰ(filelinep);
    }
    ~V3PreLex() {
        ਜਦੋਂ (!m_streamp_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            delete m_streamp_ਢੇਰ.ਸਿਖਰ();
            m_streamp_ਢੇਰ.ਕੱਢੋ();
        }
        VL_DO_CLEAR(yy_delete_ਬਫਰ(m_ਬਫਰ_State), m_ਬਫਰ_State = nullptr);
        yylex_destroy();
    }
    VPre_ਧਾਰਾ* new_ਧਾਰਾ(File_ਸਤਰ* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_ਧਾਰਾ* curStreamp() { ਵਾਪਸ m_streamp_ਢੇਰ.ਸਿਖਰ(); }  // Can't be empty, "EOF" is on top
    File_ਸਤਰ* curFilelinep() { ਵਾਪਸ curStreamp()->m_curFilelinep; }
    ਖਾਲੀ curFilelinep(File_ਸਤਰ* fl) { curStreamp()->m_curFilelinep = fl; }
    ਖਾਲੀ append_Def_ਮੁੱਲ(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t len) { m_def_ਮੁੱਲ.append(textp, len); }
    ਖਾਲੀ ਸਤਰ_Directive(ਅਚਲ ਅੱਖਰ* textp);
    ਖਾਲੀ linenoInc() {
        ਜੇ (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } ਨਹੀਂਤਾਂ {
            curFilelinep()->linenoInc();
        }
    }
    ਖਾਲੀ warnBackslashSpace();
    ਖਾਲੀ verilatorCmtLint(ਅਚਲ ਅੱਖਰ* textp, bool turnOff);
    ਖਾਲੀ verilatorCmtLintRestore();
    ਖਾਲੀ verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    ਖਾਲੀ ਧੱਕੋ_State_Def_Arg(ਪੂਰਨ level);
    ਖਾਲੀ ਧੱਕੋ_State_Def_Form();
    ਖਾਲੀ ਧੱਕੋ_State_Def_ਮੁੱਲ();
    ਖਾਲੀ ਧੱਕੋ_State_ਵਿਅੰਜਕ();
    ਖਾਲੀ ਧੱਕੋ_State_Inc_Filename();
    ਖਾਲੀ ਧੱਕੋ_State_Passthru();
    ਖਾਲੀ ਜਾਂਚ_New_File(File_ਸਤਰ* filelinep);
    ਖਾਲੀ ਜਾਂਚ_ਬਾਈਟਾਂ(ਅਚਲ string& str);
    ਖਾਲੀ ਜਾਂਚ_ਬਾਈਟਾਂ_ਪਿੱਠ(ਅਚਲ string& str);
    ਮਾਪ_t ਨਿਵੇਸ਼_ਮੰਜ਼ਿਲ_Lex(ਅੱਖਰ* buf, ਮਾਪ_t max_ਮਾਪ_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_ਬਫਰ_STATE current_ਬਫਰ();
    ਪੂਰਨ lex();
    ਪੂਰਨ current_ਸ਼ੁਰੂ_State() ਅਚਲ;
    ਖਾਲੀ dumpSummary();
    ਖਾਲੀ dump_ਢੇਰ();
    ਖਾਲੀ ਅਣਵਰਤਿਆ();
    // Called by VPreStream
    ਖਾਲੀ ਧਾਰਾ_ਡੂੰਘਾਈ_ਜੋੜੋ(ਪੂਰਨ delta) { m_ਧਾਰਾ_ਡੂੰਘਾਈ += delta; }
    ਪੂਰਨ ਧਾਰਾ_ਡੂੰਘਾਈ() ਅਚਲ { ਵਾਪਸ m_ਧਾਰਾ_ਡੂੰਘਾਈ; }
    // Utility
    ਸਥਿਰ ਖਾਲੀ ਸੈੱਟ_YYDebug(bool on);
    ਸਥਿਰ string cleanDbgStrg(ਅਚਲ string& ਵਿੱਚ_2);

ਨਿਜੀ:
    string currentUnreadChars();
    string ਸਿਰਾ_ਦਾ_ਧਾਰਾ(bool& againr);
    ਖਾਲੀ init_ਪਹਿਲਾ_ਬਫਰ(File_ਸਤਰ* filelinep);
    ਖਾਲੀ ਜਾਂਚ_Switch_ਧਾਰਾ(VPre_ਧਾਰਾ* streamp);
};

ਖਾਲੀ VPre_ਧਾਰਾ::lex_ਧਾਰਾ_ਡੂੰਘਾਈ_ਜੋੜੋ(ਪੂਰਨ delta) { m_lexp->ਧਾਰਾ_ਡੂੰਘਾਈ_ਜੋੜੋ(delta); }

#ਜੇ_ਅੰਤ  // Guard
