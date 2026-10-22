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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_VPRELEX_H_  // Guard
#وصف_ڪريو VERILATOR_VPRELEX_H_

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
#وصف_ڪريو VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#وصف_ڪريو VP_EOF_ERROR    400

#وصف_ڪريو VP_INCLUDE      256
#وصف_ڪريو VP_IFDEF        257
#وصف_ڪريو VP_IFNDEF       258
#وصف_ڪريو VP_ENDIF        259
#وصف_ڪريو VP_UNDEF        260
#وصف_ڪريو VP_DEFINE       261
#وصف_ڪريو VP_ELSE         262
#وصف_ڪريو VP_ELSIF        263
#وصف_ڪريو VP_سٽ         264
#وصف_ڪريو VP_UNDEFINEALL  265

#وصف_ڪريو VP_نشان       300
#وصف_ڪريو VP_STRING       301
#وصف_ڪريو VP_DEFVALUE     302
#وصف_ڪريو VP_COMMENT      303
#وصف_ڪريو VP_متن         304
#وصف_ڪريو VP_WHITE        305
#وصف_ڪريو VP_DEFREF       306
#وصف_ڪريو VP_DEFARG       307
#وصف_ڪريو VP_ERROR        308
#وصف_ڪريو VP_DEFFORM      309
#وصف_ڪريو VP_STRIFY       310
#وصف_ڪريو VP_BACKQUOTE    311
#وصف_ڪريو VP_نشان_JOIN  312
#وصف_ڪريو VP_DEFREF_JOIN  313
#وصف_ڪريو VP_JOIN         314

#وصف_ڪريو VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#جيڪڏهن_وصف_نه_ٿيل yy_create_بفر
# define yy_create_بفر V3_Pre_Lex_create_بفر
# define yy_delete_بفر V3_Pre_Lex_delete_بفر
# define yy_جاچ_بفر V3_Pre_Lex_جاچ_بفر
# define yy_جاچ_string V3_Pre_Lex_جاچ_string
# define yy_جاچ_بائيٽون V3_Pre_Lex_جاچ_بائيٽون
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_بفر V3_Pre_Lex_init_بفر
# define yy_flush_بفر V3_Pre_Lex_flush_بفر
# define yy_ڀريو_بفر_state V3_Pre_Lex_ڀريو_بفر_state
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
#جيڪڏهن_پورو

#جيڪڏهن_وصف_نه_ٿيل yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#جيڪڏهن_پورو

#جيڪڏهن_وصف_نه_ٿيل YY_بفر_STATE
بنية yy_بفر_state;
تعريفنوع بنية yy_بفر_state* YY_بفر_STATE;
# define YY_BUF_سائيز 16384
#جيڪڏهن_پورو
// clang-format on

خارجي صحيح yylex();
خارجي فراغ yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
خارجي حرف* yyourtext();
خارجي سائيز_t yyourleng();
خارجي فراغ yyourtext(ثابت حرف* textp, سائيز_t سائيز);  // Must call with static

YY_بفر_STATE yy_create_بفر(FILE* file, صحيح سائيز);
فراغ yy_switch_منزل_بفر(YY_بفر_STATE new_بفر);
فراغ yy_delete_بفر(YY_بفر_STATE b);
صحيح yylex_destroy();

//======================================================================

#وصف_ڪريو KEEPCMT_SUB 2
#وصف_ڪريو KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_وهڪرو final {
عام:
    File_سٽ* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ثابت m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    صحيح m_ignNewlines = 0;  // Ignore multiline newlines
    صحيح m_رڪن_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_وهڪرو(File_سٽ* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_وهڪرو_اونهائي_جوڙيو(1);
    }
    ~VPre_وهڪرو() { lex_وهڪرو_اونهائي_جوڙيو(-1); }

خاص:
    مضمن فراغ lex_وهڪرو_اونهائي_جوڙيو(صحيح delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
تعداد class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
عام:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ثابت m_preimpp;  // Preprocessor lexor belongs to
    std::ڍير<VPre_وهڪرو*> m_streamp_ڍير;  // Stack of processing files
    صحيح m_وهڪرو_اونهائي = 0;  // Depth of stream processing
    YY_بفر_STATE m_بفر_State;  // Flex state
    File_سٽ* m_tokFilelinep;  // Starting position of current token
    std::deque<File_سٽ> m_lexLintState;  // Current lint state for save/restore

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
    صحيح m_prot_ڊيگهائي = 0;  // unencoded length for BASE64
    صحيح m_prot_بائيٽون = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_سٽ* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_پهريون_بفر(filelinep);
    }
    ~V3PreLex() {
        طالما (!m_streamp_ڍير.خالي_جايون()) {
            delete m_streamp_ڍير.چوٽي();
            m_streamp_ڍير.ڪڍو();
        }
        VL_DO_CLEAR(yy_delete_بفر(m_بفر_State), m_بفر_State = nullptr);
        yylex_destroy();
    }
    VPre_وهڪرو* new_وهڪرو(File_سٽ* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_وهڪرو* curStreamp() { أرجع m_streamp_ڍير.چوٽي(); }  // Can't be empty, "EOF" is on top
    File_سٽ* curFilelinep() { أرجع curStreamp()->m_curFilelinep; }
    فراغ curFilelinep(File_سٽ* fl) { curStreamp()->m_curFilelinep = fl; }
    فراغ append_Def_قدر(ثابت حرف* textp, سائيز_t len) { m_def_قدر.append(textp, len); }
    فراغ سٽ_Directive(ثابت حرف* textp);
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
    فراغ ڌڪو_State_Def_Arg(صحيح level);
    فراغ ڌڪو_State_Def_Form();
    فراغ ڌڪو_State_Def_قدر();
    فراغ ڌڪو_State_اظهار();
    فراغ ڌڪو_State_Inc_Filename();
    فراغ ڌڪو_State_Passthru();
    فراغ جاچ_New_File(File_سٽ* filelinep);
    فراغ جاچ_بائيٽون(ثابت string& str);
    فراغ جاچ_بائيٽون_پٺ(ثابت string& str);
    سائيز_t داخلا_منزل_Lex(حرف* buf, سائيز_t max_سائيز_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_بفر_STATE current_بفر();
    صحيح lex();
    صحيح current_شروعات_State() ثابت;
    فراغ dumpSummary();
    فراغ dump_ڍير();
    فراغ اڻ_استعمال_ٿيل();
    // Called by VPreStream
    فراغ وهڪرو_اونهائي_جوڙيو(صحيح delta) { m_وهڪرو_اونهائي += delta; }
    صحيح وهڪرو_اونهائي() ثابت { أرجع m_وهڪرو_اونهائي; }
    // Utility
    ساكن فراغ سيٽ_YYDebug(bool on);
    ساكن string cleanDbgStrg(ثابت string& اندريون);

خاص:
    string currentUnreadChars();
    string پڇاڙي_جو_وهڪرو(bool& againr);
    فراغ init_پهريون_بفر(File_سٽ* filelinep);
    فراغ جاچ_Switch_وهڪرو(VPre_وهڪرو* streamp);
};

فراغ VPre_وهڪرو::lex_وهڪرو_اونهائي_جوڙيو(صحيح delta) { m_lexp->وهڪرو_اونهائي_جوڙيو(delta); }

#جيڪڏهن_پورو  // Guard
