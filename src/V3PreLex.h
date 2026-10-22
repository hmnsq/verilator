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

#اگر_غیر_تعریف_شدہ VERILATOR_VPRELEX_H_  // Guard
#تعریف_کرو VERILATOR_VPRELEX_H_

#شامل "V3Error.h"
#شامل "V3FileLine.h"

#شامل <deque>
#شامل <stack>

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
#اگر_غیر_تعریف_شدہ yy_create_بفر
# define yy_create_بفر V3_Pre_Lex_create_بفر
# define yy_delete_بفر V3_Pre_Lex_delete_بفر
# define yy_جائزہ_بفر V3_Pre_Lex_جائزہ_بفر
# define yy_جائزہ_string V3_Pre_Lex_جائزہ_string
# define yy_جائزہ_بائٹیں V3_Pre_Lex_جائزہ_بائٹیں
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_بفر V3_Pre_Lex_init_بفر
# define yy_flush_بفر V3_Pre_Lex_flush_بفر
# define yy_بھرو_بفر_state V3_Pre_Lex_بھرو_بفر_state
# define yy_switch_مقصد_بفر V3_Pre_Lex_switch_مقصد_بفر
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#اگر_ختم

#اگر_غیر_تعریف_شدہ yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#اگر_ختم

#اگر_غیر_تعریف_شدہ YY_بفر_STATE
ساخت yy_بفر_state;
قسمتعریف ساخت yy_بفر_state* YY_بفر_STATE;
# define YY_BUF_سائز 16384
#اگر_ختم
// clang-format on

خارجی صحیح yylex();
خارجی خالی yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
خارجی حرف* yyourtext();
خارجی سائز_t yyourleng();
خارجی خالی yyourtext(ثابت حرف* textp, سائز_t سائز);  // Must call with static

YY_بفر_STATE yy_create_بفر(FILE* file, صحیح سائز);
خالی yy_switch_مقصد_بفر(YY_بفر_STATE new_بفر);
خالی yy_delete_بفر(YY_بفر_STATE b);
صحیح yylex_destroy();

//======================================================================

#تعریف_کرو KEEPCMT_SUB 2
#تعریف_کرو KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_دھارا final {
عوامی:
    File_سطر* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ثابت m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    صحیح m_ignNewlines = 0;  // Ignore multiline newlines
    صحیح m_رکن_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_دھارا(File_سطر* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_دھارا_گہرائی_جوڑو(1);
    }
    ~VPre_دھارا() { lex_دھارا_گہرائی_جوڑو(-1); }

نجی:
    اندرونی خالی lex_دھارا_گہرائی_جوڑو(صحیح delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
شمار class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
عوامی:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ثابت m_preimpp;  // Preprocessor lexor belongs to
    std::ڈھیر<VPre_دھارا*> m_streamp_ڈھیر;  // Stack of processing files
    صحیح m_دھارا_گہرائی = 0;  // Depth of stream processing
    YY_بفر_STATE m_بفر_State;  // Flex state
    File_سطر* m_tokFilelinep;  // Starting position of current token
    std::deque<File_سطر> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    ساکن V3PreLex* s_currentLexp;  ///< Current lexing point
    صحیح m_keepComments = 0;  ///< Emit comments in output text
    صحیح m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    صحیح m_formalLevel = 0;  // Parenthesis counting inside def formals
    صحیح m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_قدر;  // Definition value being built.
    صحیح m_enterExit = 0;  // For VL_LINE, the enter/exit level
    صحیح m_prot_لمبائی = 0;  // unencoded length for BASE64
    صحیح m_prot_بائٹیں = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_سطر* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_پہلا_بفر(filelinep);
    }
    ~V3PreLex() {
        جب (!m_streamp_ڈھیر.خالی_خانے()) {
            delete m_streamp_ڈھیر.چوٹی();
            m_streamp_ڈھیر.نکالو();
        }
        VL_DO_CLEAR(yy_delete_بفر(m_بفر_State), m_بفر_State = nullptr);
        yylex_destroy();
    }
    VPre_دھارا* new_دھارا(File_سطر* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_دھارا* curStreamp() { واپس m_streamp_ڈھیر.چوٹی(); }  // Can't be empty, "EOF" is on top
    File_سطر* curFilelinep() { واپس curStreamp()->m_curFilelinep; }
    خالی curFilelinep(File_سطر* fl) { curStreamp()->m_curFilelinep = fl; }
    خالی append_Def_قدر(ثابت حرف* textp, سائز_t len) { m_def_قدر.append(textp, len); }
    خالی سطر_Directive(ثابت حرف* textp);
    خالی linenoInc() {
        اگر (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } ورنہ {
            curFilelinep()->linenoInc();
        }
    }
    خالی warnBackslashSpace();
    خالی verilatorCmtLint(ثابت حرف* textp, bool turnOff);
    خالی verilatorCmtLintRestore();
    خالی verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    خالی ڈالو_State_Def_Arg(صحیح level);
    خالی ڈالو_State_Def_Form();
    خالی ڈالو_State_Def_قدر();
    خالی ڈالو_State_اظہاریہ();
    خالی ڈالو_State_Inc_Filename();
    خالی ڈالو_State_Passthru();
    خالی جائزہ_New_File(File_سطر* filelinep);
    خالی جائزہ_بائٹیں(ثابت string& str);
    خالی جائزہ_بائٹیں_عقب(ثابت string& str);
    سائز_t داخلہ_مقصد_Lex(حرف* buf, سائز_t max_سائز_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_بفر_STATE current_بفر();
    صحیح lex();
    صحیح current_آغاز_State() ثابت;
    خالی dumpSummary();
    خالی dump_ڈھیر();
    خالی غیر_مستعمل();
    // Called by VPreStream
    خالی دھارا_گہرائی_جوڑو(صحیح delta) { m_دھارا_گہرائی += delta; }
    صحیح دھارا_گہرائی() ثابت { واپس m_دھارا_گہرائی; }
    // Utility
    ساکن خالی سیٹ_YYDebug(bool on);
    ساکن string cleanDbgStrg(ثابت string& اندر);

نجی:
    string currentUnreadChars();
    string انتہا_کا_دھارا(bool& againr);
    خالی init_پہلا_بفر(File_سطر* filelinep);
    خالی جائزہ_Switch_دھارا(VPre_دھارا* streamp);
};

خالی VPre_دھارا::lex_دھارا_گہرائی_جوڑو(صحیح delta) { m_lexp->دھارا_گہرائی_جوڑو(delta); }

#اگر_ختم  // Guard
