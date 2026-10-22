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

#اگر_تعریف_نشده VERILATOR_VPRELEX_H_  // Guard
#تعریف_کن VERILATOR_VPRELEX_H_

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
#تعریف_کن VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#تعریف_کن VP_EOF_ERROR    400

#تعریف_کن VP_INCLUDE      256
#تعریف_کن VP_IFDEF        257
#تعریف_کن VP_IFNDEF       258
#تعریف_کن VP_ENDIF        259
#تعریف_کن VP_UNDEF        260
#تعریف_کن VP_DEFINE       261
#تعریف_کن VP_ELSE         262
#تعریف_کن VP_ELSIF        263
#تعریف_کن VP_سطر         264
#تعریف_کن VP_UNDEFINEALL  265

#تعریف_کن VP_نماد       300
#تعریف_کن VP_STRING       301
#تعریف_کن VP_DEFVALUE     302
#تعریف_کن VP_COMMENT      303
#تعریف_کن VP_متن         304
#تعریف_کن VP_WHITE        305
#تعریف_کن VP_DEFREF       306
#تعریف_کن VP_DEFARG       307
#تعریف_کن VP_ERROR        308
#تعریف_کن VP_DEFFORM      309
#تعریف_کن VP_STRIFY       310
#تعریف_کن VP_BACKQUOTE    311
#تعریف_کن VP_نماد_JOIN  312
#تعریف_کن VP_DEFREF_JOIN  313
#تعریف_کن VP_JOIN         314

#تعریف_کن VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#اگر_تعریف_نشده yy_create_میانگیر
# define yy_create_میانگیر V3_Pre_Lex_create_میانگیر
# define yy_delete_میانگیر V3_Pre_Lex_delete_میانگیر
# define yy_پیمایش_میانگیر V3_Pre_Lex_پیمایش_میانگیر
# define yy_پیمایش_string V3_Pre_Lex_پیمایش_string
# define yy_پیمایش_بایتها V3_Pre_Lex_پیمایش_بایتها
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_میانگیر V3_Pre_Lex_init_میانگیر
# define yy_flush_میانگیر V3_Pre_Lex_flush_میانگیر
# define yy_بارگیری_میانگیر_state V3_Pre_Lex_بارگیری_میانگیر_state
# define yy_switch_مقصد_میانگیر V3_Pre_Lex_switch_مقصد_میانگیر
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#پایان_اگر

#اگر_تعریف_نشده yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#پایان_اگر

#اگر_تعریف_نشده YY_میانگیر_STATE
بنية yy_میانگیر_state;
تعريفنوع بنية yy_میانگیر_state* YY_میانگیر_STATE;
# define YY_BUF_اندازه 16384
#پایان_اگر
// clang-format on

خارجي صحيح yylex();
خارجي فراغ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
خارجي حرف* yyourtext();
خارجي اندازه_t yyourleng();
خارجي فراغ yyourtext(ثابت حرف* textp, اندازه_t اندازه);  // Must call with static

YY_میانگیر_STATE yy_create_میانگیر(FILE* file, صحيح اندازه);
فراغ yy_switch_مقصد_میانگیر(YY_میانگیر_STATE new_میانگیر);
فراغ yy_delete_میانگیر(YY_میانگیر_STATE b);
صحيح yylex_destroy();

//======================================================================

#تعریف_کن KEEPCMT_SUB 2
#تعریف_کن KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_جریان final {
عام:
    File_سطر* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ثابت m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    صحيح m_ignNewlines = 0;  // Ignore multiline newlines
    صحيح m_جمله_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_جریان(File_سطر* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_جریان_عمق_جمع_کن(1);
    }
    ~VPre_جریان() { lex_جریان_عمق_جمع_کن(-1); }

خاص:
    مضمن فراغ lex_جریان_عمق_جمع_کن(صحيح delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
تعداد class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
عام:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ثابت m_preimpp;  // Preprocessor lexor belongs to
    std::پشته<VPre_جریان*> m_streamp_پشته;  // Stack of processing files
    صحيح m_جریان_عمق = 0;  // Depth of stream processing
    YY_میانگیر_STATE m_میانگیر_State;  // Flex state
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
    string m_def_مقدار;  // Definition value being built.
    صحيح m_enterExit = 0;  // For VL_LINE, the enter/exit level
    صحيح m_prot_درازی = 0;  // unencoded length for BASE64
    صحيح m_prot_بایتها = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_سطر* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_اولی_میانگیر(filelinep);
    }
    ~V3PreLex() {
        طالما (!m_streamp_پشته.خالی()) {
            delete m_streamp_پشته.بالاترین();
            m_streamp_پشته.برداشت();
        }
        VL_DO_CLEAR(yy_delete_میانگیر(m_میانگیر_State), m_میانگیر_State = nullptr);
        yylex_destroy();
    }
    VPre_جریان* new_جریان(File_سطر* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_جریان* curStreamp() { أرجع m_streamp_پشته.بالاترین(); }  // Can't be empty, "EOF" is on top
    File_سطر* curFilelinep() { أرجع curStreamp()->m_curFilelinep; }
    فراغ curFilelinep(File_سطر* fl) { curStreamp()->m_curFilelinep = fl; }
    فراغ append_Def_مقدار(ثابت حرف* textp, اندازه_t len) { m_def_مقدار.append(textp, len); }
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
    فراغ افزودن_State_Def_Arg(صحيح level);
    فراغ افزودن_State_Def_Form();
    فراغ افزودن_State_Def_مقدار();
    فراغ افزودن_State_عبارت();
    فراغ افزودن_State_Inc_Filename();
    فراغ افزودن_State_Passthru();
    فراغ پیمایش_New_File(File_سطر* filelinep);
    فراغ پیمایش_بایتها(ثابت string& str);
    فراغ پیمایش_بایتها_پشت(ثابت string& str);
    اندازه_t درون_داد_مقصد_Lex(حرف* buf, اندازه_t max_اندازه_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_میانگیر_STATE current_میانگیر();
    صحيح lex();
    صحيح current_شروع_State() ثابت;
    فراغ dumpSummary();
    فراغ dump_پشته();
    فراغ استفادهنشده();
    // Called by VPreStream
    فراغ جریان_عمق_جمع_کن(صحيح delta) { m_جریان_عمق += delta; }
    صحيح جریان_عمق() ثابت { أرجع m_جریان_عمق; }
    // Utility
    ساكن فراغ مجموعه_YYDebug(bool on);
    ساكن string cleanDbgStrg(ثابت string& درون);

خاص:
    string currentUnreadChars();
    string انتها_از_جریان(bool& againr);
    فراغ init_اولی_میانگیر(File_سطر* filelinep);
    فراغ پیمایش_Switch_جریان(VPre_جریان* streamp);
};

فراغ VPre_جریان::lex_جریان_عمق_جمع_کن(صحيح delta) { m_lexp->جریان_عمق_جمع_کن(delta); }

#پایان_اگر  // Guard
