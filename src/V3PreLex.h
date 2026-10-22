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

#جے_بنا_تعریف VERILATOR_VPRELEX_H_  // Guard
#تعریف_کرو VERILATOR_VPRELEX_H_

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"

#ضمن <deque>
#ضمن <stack>

//======================================================================

class V3PreLex;
class V3PreProcImp;

// Token codes
// If changing, see V3PreProc.cpp's V3PreProcImp::tokenName()
// clang-format off
#تعریف_کرو VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#تعریف_کرو VP_EOF_ERROR    400

#تعریف_کرو VP_INCLUDE      256
#تعریف_کرو VP_IFDEF        257
#تعریف_کرو VP_IFNDEF       258
#تعریف_کرو VP_ENDIF        259
#تعریف_کرو VP_UNDEF        260
#تعریف_کرو VP_DEFINE       261
#تعریف_کرو VP_ELSE         262
#تعریف_کرو VP_ELSIF        263
#تعریف_کرو VP_سطر         264
#تعریف_کرو VP_UNDEFINEALL  265

#تعریف_کرو VP_علامت       300
#تعریف_کرو VP_STRING       301
#تعریف_کرو VP_DEFVALUE     302
#تعریف_کرو VP_COMMENT      303
#تعریف_کرو VP_متن         304
#تعریف_کرو VP_WHITE        305
#تعریف_کرو VP_DEFREF       306
#تعریف_کرو VP_DEFARG       307
#تعریف_کرو VP_ERROR        308
#تعریف_کرو VP_DEFFORM      309
#تعریف_کرو VP_STRIFY       310
#تعریف_کرو VP_BACKQUOTE    311
#تعریف_کرو VP_علامت_JOIN  312
#تعریف_کرو VP_DEFREF_JOIN  313
#تعریف_کرو VP_JOIN         314

#تعریف_کرو VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#جے_بنا_تعریف yy_create_بفر
# define yy_create_بفر V3_Pre_Lex_create_بفر
# define yy_delete_بفر V3_Pre_Lex_delete_بفر
# define yy_جانچ_بفر V3_Pre_Lex_جانچ_بفر
# define yy_جانچ_string V3_Pre_Lex_جانچ_string
# define yy_جانچ_بائٹاں V3_Pre_Lex_جانچ_بائٹاں
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_بفر V3_Pre_Lex_init_بفر
# define yy_flush_بفر V3_Pre_Lex_flush_بفر
# define yy_لدو_بفر_state V3_Pre_Lex_لدو_بفر_state
# define yy_switch_منزل_بفر V3_Pre_Lex_switch_منزل_بفر
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#جے_انت

#جے_بنا_تعریف yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#جے_انت

#جے_بنا_تعریف YY_بفر_STATE
بنية yy_بفر_state;
تعريفنوع بنية yy_بفر_state* YY_بفر_STATE;
# define YY_BUF_ناپ 16384
#جے_انت
// clang-format on

خارجي صحيح yylex();
خارجي فراغ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
خارجي حرف* yyourtext();
خارجي ناپ_t yyourleng();
خارجي فراغ yyourtext(ثابت حرف* textp, ناپ_t ناپ);  // Must call with static

YY_بفر_STATE yy_create_بفر(FILE* file, صحيح ناپ);
فراغ yy_switch_منزل_بفر(YY_بفر_STATE new_بفر);
فراغ yy_delete_بفر(YY_بفر_STATE b);
صحيح yylex_destroy();

//======================================================================

#تعریف_کرو KEEPCMT_SUB 2
#تعریف_کرو KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_دھارا final {
عام:
    File_سطر* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ثابت m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    صحيح m_ignNewlines = 0;  // Ignore multiline newlines
    صحيح m_رکن_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_دھارا(File_سطر* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_دھارا_ڈونگھائی_جوڑو(1);
    }
    ~VPre_دھارا() { lex_دھارا_ڈونگھائی_جوڑو(-1); }

خاص:
    مضمن فراغ lex_دھارا_ڈونگھائی_جوڑو(صحيح delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
تعداد class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
عام:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ثابت m_preimpp;  // Preprocessor lexor belongs to
    std::ڈھیر<VPre_دھارا*> m_streamp_ڈھیر;  // Stack of processing files
    صحيح m_دھارا_ڈونگھائی = 0;  // Depth of stream processing
    YY_بفر_STATE m_بفر_State;  // Flex state
    File_سطر* m_tokFilelinep;  // Starting position of current token
    std::deque<File_سطر> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    ساكن V3PreLex* s_currentLexp;  ///< Current lexing point
    صحيح m_keepComments = 0;  ///< Emit comments in output text
    صحيح m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    صحيح m_formalLevel = 0;  // Parenthesis counting inside def formals
    صحيح m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_قدر;  // Definition value being built.
    صحيح m_enterExit = 0;  // For VL_LINE, the enter/exit level
    صحيح m_prot_لمائی = 0;  // unencoded length for BASE64
    صحيح m_prot_بائٹاں = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_سطر* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_پہلا_بفر(filelinep);
    }
    ~V3PreLex() {
        طالما (!m_streamp_ڈھیر.خالی_تھاواں()) {
            delete m_streamp_ڈھیر.چوٹی();
            m_streamp_ڈھیر.نکالو();
        }
        VL_DO_CLEAR(yy_delete_بفر(m_بفر_State), m_بفر_State = nullptr);
        yylex_destroy();
    }
    VPre_دھارا* new_دھارا(File_سطر* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_دھارا* curStreamp() { أرجع m_streamp_ڈھیر.چوٹی(); }  // Can't be empty, "EOF" is on top
    File_سطر* curFilelinep() { أرجع curStreamp()->m_curFilelinep; }
    فراغ curFilelinep(File_سطر* fl) { curStreamp()->m_curFilelinep = fl; }
    فراغ append_Def_قدر(ثابت حرف* textp, ناپ_t len) { m_def_قدر.append(textp, len); }
    فراغ سطر_Directive(ثابت حرف* textp);
    فراغ linenoInc() {
        إذا (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } وإلا {
            curFilelinep()->linenoInc();
        }
    }
    فراغ warnBackslashSpace();
    فراغ verilatorCmtLint(ثابت حرف* textp, bool turnOff);
    فراغ verilatorCmtLintRestore();
    فراغ verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    فراغ ڈالو_State_Def_Arg(صحيح level);
    فراغ ڈالو_State_Def_Form();
    فراغ ڈالو_State_Def_قدر();
    فراغ ڈالو_State_اظہاریہ();
    فراغ ڈالو_State_Inc_Filename();
    فراغ ڈالو_State_Passthru();
    فراغ جانچ_New_File(File_سطر* filelinep);
    فراغ جانچ_بائٹاں(ثابت string& str);
    فراغ جانچ_بائٹاں_پٹھ(ثابت string& str);
    ناپ_t داخلہ_منزل_Lex(حرف* buf, ناپ_t max_ناپ_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_بفر_STATE current_بفر();
    صحيح lex();
    صحيح current_شروع_State() ثابت;
    فراغ dumpSummary();
    فراغ dump_ڈھیر();
    فراغ بن_ورتیا();
    // Called by VPreStream
    فراغ دھارا_ڈونگھائی_جوڑو(صحيح delta) { m_دھارا_ڈونگھائی += delta; }
    صحيح دھارا_ڈونگھائی() ثابت { أرجع m_دھارا_ڈونگھائی; }
    // Utility
    ساكن فراغ سیٹ_YYDebug(bool on);
    ساكن string cleanDbgStrg(ثابت string& وچ);

خاص:
    string currentUnreadChars();
    string سرا_دا_دھارا(bool& againr);
    فراغ init_پہلا_بفر(File_سطر* filelinep);
    فراغ جانچ_Switch_دھارا(VPre_دھارا* streamp);
};

فراغ VPre_دھارا::lex_دھارا_ڈونگھائی_جوڑو(صحيح delta) { m_lexp->دھارا_ڈونگھائی_جوڑو(delta); }

#جے_انت  // Guard
