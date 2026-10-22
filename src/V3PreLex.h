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

#که_نه_تعریف_شوی VERILATOR_VPRELEX_H_  // Guard
#تعریف_کړه VERILATOR_VPRELEX_H_

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
#تعریف_کړه VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#تعریف_کړه VP_EOF_ERROR    400

#تعریف_کړه VP_INCLUDE      256
#تعریف_کړه VP_IFDEF        257
#تعریف_کړه VP_IFNDEF       258
#تعریف_کړه VP_ENDIF        259
#تعریف_کړه VP_UNDEF        260
#تعریف_کړه VP_DEFINE       261
#تعریف_کړه VP_ELSE         262
#تعریف_کړه VP_ELSIF        263
#تعریف_کړه VP_کرښه         264
#تعریف_کړه VP_UNDEFINEALL  265

#تعریف_کړه VP_نښه       300
#تعریف_کړه VP_STRING       301
#تعریف_کړه VP_DEFVALUE     302
#تعریف_کړه VP_COMMENT      303
#تعریف_کړه VP_متن         304
#تعریف_کړه VP_WHITE        305
#تعریف_کړه VP_DEFREF       306
#تعریف_کړه VP_DEFARG       307
#تعریف_کړه VP_ERROR        308
#تعریف_کړه VP_DEFFORM      309
#تعریف_کړه VP_STRIFY       310
#تعریف_کړه VP_BACKQUOTE    311
#تعریف_کړه VP_نښه_JOIN  312
#تعریف_کړه VP_DEFREF_JOIN  313
#تعریف_کړه VP_JOIN         314

#تعریف_کړه VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#که_نه_تعریف_شوی yy_create_بفر
# define yy_create_بفر V3_Pre_Lex_create_بفر
# define yy_delete_بفر V3_Pre_Lex_delete_بفر
# define yy_لټون_بفر V3_Pre_Lex_لټون_بفر
# define yy_لټون_string V3_Pre_Lex_لټون_string
# define yy_لټون_بایټونه V3_Pre_Lex_لټون_بایټونه
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_بفر V3_Pre_Lex_init_بفر
# define yy_flush_بفر V3_Pre_Lex_flush_بفر
# define yy_بارول_بفر_state V3_Pre_Lex_بارول_بفر_state
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
#د_که_پای

#که_نه_تعریف_شوی yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#د_که_پای

#که_نه_تعریف_شوی YY_بفر_STATE
بنية yy_بفر_state;
تعريفنوع بنية yy_بفر_state* YY_بفر_STATE;
# define YY_BUF_اندازه 16384
#د_که_پای
// clang-format on

خارجي صحيح yylex();
خارجي فراغ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
خارجي حرف* yyourtext();
خارجي اندازه_t yyourleng();
خارجي فراغ yyourtext(ثابت حرف* textp, اندازه_t اندازه);  // Must call with static

YY_بفر_STATE yy_create_بفر(FILE* file, صحيح اندازه);
فراغ yy_switch_منزل_بفر(YY_بفر_STATE new_بفر);
فراغ yy_delete_بفر(YY_بفر_STATE b);
صحيح yylex_destroy();

//======================================================================

#تعریف_کړه KEEPCMT_SUB 2
#تعریف_کړه KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_بهیر final {
عام:
    File_کرښه* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ثابت m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    صحيح m_ignNewlines = 0;  // Ignore multiline newlines
    صحيح m_غړی_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_بهیر(File_کرښه* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_بهیر_ژوروالی_جمع_کړه(1);
    }
    ~VPre_بهیر() { lex_بهیر_ژوروالی_جمع_کړه(-1); }

خاص:
    مضمن فراغ lex_بهیر_ژوروالی_جمع_کړه(صحيح delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
تعداد class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
عام:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ثابت m_preimpp;  // Preprocessor lexor belongs to
    std::ډېری<VPre_بهیر*> m_streamp_ډېری;  // Stack of processing files
    صحيح m_بهیر_ژوروالی = 0;  // Depth of stream processing
    YY_بفر_STATE m_بفر_State;  // Flex state
    File_کرښه* m_tokFilelinep;  // Starting position of current token
    std::deque<File_کرښه> m_lexLintState;  // Current lint state for save/restore

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
    string m_def_ارزښت;  // Definition value being built.
    صحيح m_enterExit = 0;  // For VL_LINE, the enter/exit level
    صحيح m_prot_اوږدتیا = 0;  // unencoded length for BASE64
    صحيح m_prot_بایټونه = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_کرښه* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_لومړی_بفر(filelinep);
    }
    ~V3PreLex() {
        طالما (!m_streamp_ډېری.تشه()) {
            delete m_streamp_ډېری.سر();
            m_streamp_ډېری.راوباسه();
        }
        VL_DO_CLEAR(yy_delete_بفر(m_بفر_State), m_بفر_State = nullptr);
        yylex_destroy();
    }
    VPre_بهیر* new_بهیر(File_کرښه* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_بهیر* curStreamp() { أرجع m_streamp_ډېری.سر(); }  // Can't be empty, "EOF" is on top
    File_کرښه* curFilelinep() { أرجع curStreamp()->m_curFilelinep; }
    فراغ curFilelinep(File_کرښه* fl) { curStreamp()->m_curFilelinep = fl; }
    فراغ append_Def_ارزښت(ثابت حرف* textp, اندازه_t len) { m_def_ارزښت.append(textp, len); }
    فراغ کرښه_Directive(ثابت حرف* textp);
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
    فراغ ټېل_State_Def_Arg(صحيح level);
    فراغ ټېل_State_Def_Form();
    فراغ ټېل_State_Def_ارزښت();
    فراغ ټېل_State_بیان();
    فراغ ټېل_State_Inc_Filename();
    فراغ ټېل_State_Passthru();
    فراغ لټون_New_File(File_کرښه* filelinep);
    فراغ لټون_بایټونه(ثابت string& str);
    فراغ لټون_بایټونه_شا(ثابت string& str);
    اندازه_t ننوت_منزل_Lex(حرف* buf, اندازه_t max_اندازه_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_بفر_STATE current_بفر();
    صحيح lex();
    صحيح current_پیل_State() ثابت;
    فراغ dumpSummary();
    فراغ dump_ډېری();
    فراغ ناکارول_شوی();
    // Called by VPreStream
    فراغ بهیر_ژوروالی_جمع_کړه(صحيح delta) { m_بهیر_ژوروالی += delta; }
    صحيح بهیر_ژوروالی() ثابت { أرجع m_بهیر_ژوروالی; }
    // Utility
    ساكن فراغ سېټ_YYDebug(bool on);
    ساكن string cleanDbgStrg(ثابت string& دننه);

خاص:
    string currentUnreadChars();
    string پای_ټکی_د_بهیر(bool& againr);
    فراغ init_لومړی_بفر(File_کرښه* filelinep);
    فراغ لټون_Switch_بهیر(VPre_بهیر* streamp);
};

فراغ VPre_بهیر::lex_بهیر_ژوروالی_جمع_کړه(صحيح delta) { m_lexp->بهیر_ژوروالی_جمع_کړه(delta); }

#د_که_پای  // Guard
