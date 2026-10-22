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

#अगर_अघोषित VERILATOR_VPRELEX_H_  // Guard
#घोषणा VERILATOR_VPRELEX_H_

#समावेश "V3Error.h"
#समावेश "V3FileLine.h"

#समावेश <deque>
#समावेश <stack>

//======================================================================

class V3PreLex;
class V3PreProcImp;

// Token codes
// If changing, see V3PreProc.cpp's V3PreProcImp::tokenName()
// clang-format off
#घोषणा VP_EOF          0     // Must be zero, a.k.a. YY_NULL, a.k.a. yy_terminate();
#घोषणा VP_EOF_ERROR    400

#घोषणा VP_INCLUDE      256
#घोषणा VP_IFDEF        257
#घोषणा VP_IFNDEF       258
#घोषणा VP_ENDIF        259
#घोषणा VP_UNDEF        260
#घोषणा VP_DEFINE       261
#घोषणा VP_ELSE         262
#घोषणा VP_ELSIF        263
#घोषणा VP_पंक्ति         264
#घोषणा VP_UNDEFINEALL  265

#घोषणा VP_प्रतीक       300
#घोषणा VP_STRING       301
#घोषणा VP_DEFVALUE     302
#घोषणा VP_COMMENT      303
#घोषणा VP_TEXT         304
#घोषणा VP_WHITE        305
#घोषणा VP_DEFREF       306
#घोषणा VP_DEFARG       307
#घोषणा VP_ERROR        308
#घोषणा VP_DEFFORM      309
#घोषणा VP_STRIFY       310
#घोषणा VP_BACKQUOTE    311
#घोषणा VP_प्रतीक_JOIN  312
#घोषणा VP_DEFREF_JOIN  313
#घोषणा VP_JOIN         314

#घोषणा VP_PSL          350
// clang-format on

//======================================================================
// Externs created by flex
// We add a prefix so that other lexers/flexers in the same program won't collide.

// clang-format off
#अगर_अघोषित yy_create_बफर
# define yy_create_बफर V3_Pre_Lex_create_बफर
# define yy_delete_बफर V3_Pre_Lex_delete_बफर
# define yy_scan_बफर V3_Pre_Lex_scan_बफर
# define yy_scan_string V3PreLex_scan_string
# define yy_scan_bytes V3PreLex_scan_bytes
# define yy_flex_debug V3PreLex_flex_debug
# define yy_init_बफर V3_Pre_Lex_init_बफर
# define yy_flush_बफर V3_Pre_Lex_flush_बफर
# define yy_load_बफर_state V3_Pre_Lex_load_बफर_state
# define yy_switch_to_बफर V3_Pre_Lex_switch_to_बफर
# define yyin V3PreLexin
# define yyleng V3PreLexleng
# define yylex V3PreLexlex
# define yyout V3PreLexout
# define yyrestart V3PreLexrestart
# define yytext V3PreLextext
# define yyerror V3PreLexerror
# define yyerrorf V3PreLexerrorf
# define yylex_destroy V3PreLexlex_destroy
#पूर्ण_अगर

#अगर_अघोषित yyourleng
# define yyourleng V3PreLexourleng
# define yyourtext V3PreLexourtext
#पूर्ण_अगर

#अगर_अघोषित YY_बफर_STATE
काष्ठा yy_बफर_state;
प्रकार काष्ठा yy_बफर_state* YY_बफर_STATE;
# define YY_BUF_आकार 16384
#पूर्ण_अगर
// clang-format on

बाह्य पूर्णांक yylex();
बाह्य व्योम yyrestart(खाता*);

// Accessors, because flex keeps changing the type of yyleng
बाह्य अक्षर* yyourtext();
बाह्य माप_प्रकार yyourleng();
बाह्य व्योम yyourtext(स्थिर अक्षर* textp, माप_प्रकार आकार);  // Must call with static

YY_बफर_STATE yy_create_बफर(खाता* file, पूर्णांक आकार);
व्योम yy_switch_to_बफर(YY_बफर_STATE new_बफर);
व्योम yy_delete_बफर(YY_बफर_STATE b);
पूर्णांक yylex_destroy();

//======================================================================

#घोषणा KEEPCMT_SUB 2
#घोषणा KEEPCMT_EXP 3

//======================================================================
// Entry for each file processed; a stack of entries included

class VPreStream final {
खुला:
    File_पंक्ति* m_curFilelinep;  // Current processing point (see also m_tokFilelinep)
    V3PreLex* स्थिर m_lexp;  // Lexer, for resource tracking
    std::deque<string> m_buffers;  // Buffer of characters to process
    पूर्णांक m_ignNewlines = 0;  // Ignore multiline newlines
    पूर्णांक m_termState = 0;  // Termination fsm
    bool m_eof = false;  // "EOF" buffer
    bool m_file = false;  // Buffer is start of new file
    VPreStream(File_पंक्ति* fl, V3PreLex* lexp)
        : m_curFilelinep{fl}
        , m_lexp{lexp} {
        lexStreamDepthAdd(1);
    }
    ~VPreStream() { lexStreamDepthAdd(-1); }

निजी:
    अंतरभूत व्योम lexStreamDepthAdd(पूर्णांक delta);
};

//======================================================================
// Enum Class for `pragma protect encoding types
क्रमागत class Enctype : uint8_t { UUENCODE, BASE64, QUOTE_PRINTABLE, RAW, ERR };

//======================================================================
// Class entry for each per-lexer state

class V3PreLex final {
खुला:  // Used only by V3PreLex.cpp and V3PreProc.cpp
    V3PreProcImp* स्थिर m_preimpp;  // Preprocessor lexor belongs to
    std::ढेर<VPreStream*> m_streamp_ढेर;  // Stack of processing files
    पूर्णांक m_streamDepth = 0;  // Depth of stream processing
    YY_बफर_STATE m_बफर_State;  // Flex state
    File_पंक्ति* m_tokFilelinep;  // Starting position of current token
    std::deque<File_पंक्ति> m_lexLintState;  // Current lint state for save/restore

    // State to lexer
    अटल V3PreLex* s_currentLexp;  ///< Current lexing point
    पूर्णांक m_keepComments = 0;  ///< Emit comments in output text
    पूर्णांक m_keepWhitespace = 1;  ///< Emit all whitespace in output text
    bool m_pedantic = false;  ///< Obey standard; don't Substitute `error

    // State from lexer
    पूर्णांक m_formalLevel = 0;  // Parenthesis counting inside def formals
    पूर्णांक m_parenLevel = 0;  // Parenthesis counting inside def args
    bool m_defCmtSlash = false;  // /*...*/ comment in define had \ ending
    bool m_defQuote = false;  // Definition value inside quote
    string m_def_मूल्य;  // Definition value being built.
    पूर्णांक m_enterExit = 0;  // For VL_LINE, the enter/exit level
    पूर्णांक m_prot_लंबाई = 0;  // unencoded length for BASE64
    पूर्णांक m_protBytes = 0;  // decoded length for BASE64
    Enctype m_encType{};  // encoding type for `pragma protect

    // CONSTRUCTORS
    V3PreLex(V3PreProcImp* preimpp, File_पंक्ति* filelinep)
        : m_preimpp{preimpp}
        , m_tokFilelinep{filelinep} {
        init_First_बफर(filelinep);
    }
    ~V3PreLex() {
        जबतक (!m_streamp_ढेर.empty()) {
            delete m_streamp_ढेर.शीर्ष();
            m_streamp_ढेर.निकालो();
        }
        VL_DO_CLEAR(yy_delete_बफर(m_बफर_State), m_बफर_State = nullptr);
        yylex_destroy();
    }
    VPreStream* newStream(File_पंक्ति* fl, V3PreLex* lexp);

    // Called by V3PreLex.l from lexer
    VPreStream* curStreamp() { वापस m_streamp_ढेर.शीर्ष(); }  // Can't be empty, "EOF" is on top
    File_पंक्ति* curFilelinep() { वापस curStreamp()->m_curFilelinep; }
    व्योम curFilelinep(File_पंक्ति* fl) { curStreamp()->m_curFilelinep = fl; }
    व्योम append_Def_मूल्य(स्थिर अक्षर* textp, माप_प्रकार len) { m_def_मूल्य.append(textp, len); }
    व्योम पंक्ति_Directive(स्थिर अक्षर* textp);
    व्योम linenoInc() {
        अगर (curStreamp()->m_ignNewlines) {
            curStreamp()->m_ignNewlines--;
        } अन्यथा {
            curFilelinep()->linenoInc();
        }
    }
    व्योम warnBackslashSpace();
    व्योम verilatorCmtLint(स्थिर अक्षर* textp, bool turnOff);
    व्योम verilatorCmtLintRestore();
    व्योम verilatorCmtLintSave();
    // Called by V3PreProc.cpp to inform lexer
    व्योम धकेलो_State_Def_Arg(पूर्णांक level);
    व्योम धकेलो_State_Def_Form();
    व्योम धकेलो_State_Def_मूल्य();
    व्योम धकेलो_State_व्यंजक();
    व्योम धकेलो_State_Inc_Filename();
    व्योम धकेलो_State_Passthru();
    व्योम scanNewFile(File_पंक्ति* filelinep);
    व्योम scanBytes(स्थिर string& str);
    व्योम scanBytesBack(स्थिर string& str);
    माप_प्रकार निवेश_To_Lex(अक्षर* buf, माप_प्रकार max_आकार_2);
    /// Called by V3PreProc.cpp to get data from lexer
    YY_बफर_STATE current_बफर();
    पूर्णांक lex();
    पूर्णांक currentStartState() स्थिर;
    व्योम dumpSummary();
    व्योम dump_ढेर();
    व्योम unused();
    // Called by VPreStream
    व्योम streamDepthAdd(पूर्णांक delta) { m_streamDepth += delta; }
    पूर्णांक streamDepth() स्थिर { वापस m_streamDepth; }
    // Utility
    अटल व्योम setYYDebug(bool on);
    अटल string cleanDbgStrg(स्थिर string& in);

निजी:
    string currentUnreadChars();
    string endOfStream(bool& againr);
    व्योम init_First_बफर(File_पंक्ति* filelinep);
    व्योम scanSwitchStream(VPreStream* streamp);
};

व्योम VPreStream::lexStreamDepthAdd(पूर्णांक delta) { m_lexp->streamDepthAdd(delta); }

#पूर्ण_अगर  // Guard
