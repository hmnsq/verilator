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

#यदि_अपरिभाषितम् VERILATOR_VPRELEX_H_  // Guard
#परिभाषय VERILATOR_VPRELEX_H_

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"

#अन्तर्भाव <deque>
#अन्तर्भाव <stack>

//======================================================================

class V3PreLex;
class V3PreProcImp;

// Token codes
// If changing, see V3PreProc.cpp's V3PreProcImp::tokenName()
// clang-format off
#परिभाषय VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#परिभाषय VP_EOF_ERROR    400

#परिभाषय VP_INCLUDE      256
#परिभाषय VP_IFDEF        257
#परिभाषय VP_IFNDEF       258
#परिभाषय VP_ENDIF        259
#परिभाषय VP_UNDEF        260
#परिभाषय VP_DEFINE       261
#परिभाषय VP_ELSE         262
#परिभाषय VP_ELSIF        263
#परिभाषय VP_पङ्क्तिः         264
#परिभाषय VP_UNDEFINEALL  265

#परिभाषय VP_चिह्नम्       300
#परिभाषय VP_STRING       301
#परिभाषय VP_DEFVALUE     302
#परिभाषय VP_COMMENT      303
#परिभाषय VP_TEXT         304
#परिभाषय VP_WHITE        305
#परिभाषय VP_DEFREF       306
#परिभाषय VP_DEFARG       307
#परिभाषय VP_ERROR        308
#परिभाषय VP_DEFFORM      309
#परिभाषय VP_STRIFY       310
#परिभाषय VP_BACKQUOTE    311
#परिभाषय VP_चिह्नम्_JOIN  312
#परिभाषय VP_DEFREF_JOIN  313
#परिभाषय VP_JOIN         314

#परिभाषय VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#यदि_अपरिभाषितम् yy_create_बफर
# define yy_create_बफर V3_Pre_Lex_create_बफर
# define yy_delete_बफर V3_Pre_Lex_delete_बफर
# define yy_अवलोकनम्_बफर V3_Pre_Lex_अवलोकनम्_बफर
# define yy_अवलोकनम्_string V3_Pre_Lex_अवलोकनम्_string
# define yy_अवलोकनम्_अष्टकानि V3_Pre_Lex_अवलोकनम्_अष्टकानि
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_बफर V3_Pre_Lex_init_बफर
# define yy_flush_बफर V3_Pre_Lex_flush_बफर
# define yy_आरोपय_बफर_state V3_Pre_Lex_आरोपय_बफर_state
# define yy_switch_गन्तव्यम्_बफर V3_Pre_Lex_switch_गन्तव्यम्_बफर
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#यदि_अन्तः

#यदि_अपरिभाषितम् yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#यदि_अन्तः

#यदि_अपरिभाषितम् YY_बफर_STATE
संरचना yy_बफर_state;
प्रकारनिर्देश संरचना yy_बफर_state* YY_बफर_STATE;
# define YY_BUF_आकार 16384
#यदि_अन्तः
// clang-format on

बाह्य पूर्णाङ्क yylex();
बाह्य शून्य yyrestart(FILE*);

// Accessors, because flex keeps changing the type of yyleng
बाह्य वर्ण* yyourtext();
बाह्य आकार_t yyourleng();
बाह्य शून्य yyourtext(ध्रुव वर्ण* textp, आकार_t आकार);  // Must call with static

YY_बफर_STATE yy_create_बफर(FILE* file, पूर्णाङ्क आकार);
शून्य yy_switch_गन्तव्यम्_बफर(YY_बफर_STATE new_बफर);
शून्य yy_delete_बफर(YY_बफर_STATE b);
पूर्णाङ्क yylex_destroy();

//======================================================================

#परिभाषय KEEPCMT_SUB 2
#परिभाषय KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPre_प्रवाहः final {
सार्वजनिक:
    File_पङ्क्तिः* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* ध्रुव m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    पूर्णाङ्क m_ignNewlines = 0;  // Ignore multiline newlines
    पूर्णाङ्क m_पदांशः_State = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPre_प्रवाहः(File_पङ्क्तिः* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lex_प्रवाहः_गभीरता_सङ्कलय(1);
    }
    ~VPre_प्रवाहः() { lex_प्रवाहः_गभीरता_सङ्कलय(-1); }

गुप्त:
    अन्तःस्थ शून्य lex_प्रवाहः_गभीरता_सङ्कलय(पूर्णाङ्क delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
गणना class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
सार्वजनिक:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* ध्रुव m_preimpp;  // Preprocessor lexor belongs to
    std::राशिः<VPre_प्रवाहः*> m_streamp_राशिः;  // Stack of processing files
    पूर्णाङ्क m_प्रवाहः_गभीरता = 0;  // Depth of stream processing
    YY_बफर_STATE m_बफर_State;  // Flex state
    File_पङ्क्तिः* m_tokFilelinep;  // Starting position of current token
    std::deque<File_पङ्क्तिः> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    स्थिर V3PreLex* s_currentLexp;  ///< Current lexing point
    पूर्णाङ्क m_keepComments = 0;  ///< Emit comments in output text
    पूर्णाङ्क m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    पूर्णाङ्क m_formalLevel = 0;  // Parenthesis counting inside def formals
    पूर्णाङ्क m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_मानम्;  // Definition value being built.
    पूर्णाङ्क m_enterExit = 0;  // For VL_LINE, the enter/exit level
    पूर्णाङ्क m_prot_दीर्घता = 0;  // unencoded length for BASE64
    पूर्णाङ्क m_prot_अष्टकानि = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_पङ्क्तिः* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_First_बफर(filelinep);
    }
    ~V3PreLex() {
        यावत् (!m_streamp_राशिः.रिक्तानि()) {
            delete m_streamp_राशिः.शिखरम्();
            m_streamp_राशिः.निष्कासय();
        }
        VL_DO_CLEAR(yy_delete_बफर(m_बफर_State), m_बफर_State = nullptr);
        yylex_destroy();
    }
    VPre_प्रवाहः* new_प्रवाहः(File_पङ्क्तिः* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPre_प्रवाहः* curStreamp() { प्रत्यागम m_streamp_राशिः.शिखरम्(); }  // Can't be empty, "EOF" is on top
    File_पङ्क्तिः* curFilelinep() { प्रत्यागम curStreamp()->m_curFilelinep; }
    शून्य curFilelinep(File_पङ्क्तिः* fl) { curStreamp()->m_curFilelinep = fl; }
    शून्य append_Def_मानम्(ध्रुव वर्ण* textp, आकार_t len) { m_def_मानम्.append(textp, len); }
    शून्य पङ्क्तिः_Directive(ध्रुव वर्ण* textp);
    शून्य linenoInc() {
        यदि (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } अन्यथा {
            curFilelinep()->linenoInc();
        }
    }
    शून्य warnBackslashSpace();
    शून्य verilatorCmtLint(ध्रुव वर्ण* textp, bool turnOff);
    शून्य verilatorCmtLintRestore();
    शून्य verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    शून्य प्रक्षिप_State_Def_Arg(पूर्णाङ्क level);
    शून्य प्रक्षिप_State_Def_Form();
    शून्य प्रक्षिप_State_Def_मानम्();
    शून्य प्रक्षिप_State_व्यञ्जकम्();
    शून्य प्रक्षिप_State_Inc_Filename();
    शून्य प्रक्षिप_State_Passthru();
    शून्य अवलोकनम्_New_File(File_पङ्क्तिः* filelinep);
    शून्य अवलोकनम्_अष्टकानि(ध्रुव string& str);
    शून्य अवलोकनम्_अष्टकानि_पृष्ठभागः(ध्रुव string& str);
    आकार_t निवेशः_गन्तव्यम्_Lex(वर्ण* buf, आकार_t max_आकार_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_बफर_STATE current_बफर();
    पूर्णाङ्क lex();
    पूर्णाङ्क currentStartState() ध्रुव;
    शून्य dumpSummary();
    शून्य dump_राशिः();
    शून्य अप्रयुक्तम्();
    // Called by VPreStream
    शून्य प्रवाहः_गभीरता_सङ्कलय(पूर्णाङ्क delta) { m_प्रवाहः_गभीरता += delta; }
    पूर्णाङ्क प्रवाहः_गभीरता() ध्रुव { प्रत्यागम m_प्रवाहः_गभीरता; }
    // Utility
    स्थिर शून्य समुच्चयः_YYDebug(bool on);
    स्थिर string cleanDbgStrg(ध्रुव string& अन्तर्);

गुप्त:
    string currentUnreadChars();
    string अन्तः_सम्बन्धि_प्रवाहः(bool& againr);
    शून्य init_First_बफर(File_पङ्क्तिः* filelinep);
    शून्य अवलोकनम्_Switch_प्रवाहः(VPre_प्रवाहः* streamp);
};

शून्य VPre_प्रवाहः::lex_प्रवाहः_गभीरता_सङ्कलय(पूर्णाङ्क delta) { m_lexp->प्रवाहः_गभीरता_सङ्कलय(delta); }

#यदि_अन्तः  // Guard
