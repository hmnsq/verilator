// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilog::Preproc: Internal implementation of default preprocessor
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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3PreProc.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3LanguageWords.h"
#ಅನ್ತರ್ಭಾವ "V3PreExpr.h"
#ಅನ್ತರ್ಭಾವ "V3PreLex.h"
#ಅನ್ತರ್ಭಾವ "V3PreShell.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cstdlib>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <stack>
#ಅನ್ತರ್ಭಾವ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#ವ್ಯಾಖ್ಯಾನಿಸು yyFlexLexer V3Lexer
#ಅನ್ತರ್ಭಾವ "V3PreLex.yy.cpp"
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

ವರ್ಗ VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ m_fileline;  // Where it was declared
    ಸ್ಥಿರಾಂಕ string m_ಮೌಲ್ಯ;  // Value of define
    ಸ್ಥಿರಾಂಕ string m_params;  // Parameters
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_cmdline;  // Set on command line, don't `undefineall
ಸಾರ್ವಜನಿಕ:
    VDefine(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ string& params, ತಾರ್ಕಿಕ cmdline)
        : m_fileline{fl}
        , m_ಮೌಲ್ಯ{ಮೌಲ್ಯ}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fileline; }
    string ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ; }
    string params() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_params; }
    ತಾರ್ಕಿಕ cmdline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cmdline; }
};

//*************************************************************************

ವರ್ಗ VDefineRef final {
    // One for each pending define substitution
    ಸ್ಥಿರಾಂಕ string m_name;  // Define last name being defined
    ಸ್ಥಿರಾಂಕ string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    ಪೂರ್ಣಾಂಕ m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::ಸದಿಶ<string> m_ವಾದಗಳು;  // List of define arguments
ಸಾರ್ವಜನಿಕ:
    string name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    string params() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_params; }
    string nextarg() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nextarg; }
    ಶೂನ್ಯ nextarg(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) { m_nextarg = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಂಕ parenLevel() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_parenLevel; }
    ಶೂನ್ಯ parenLevel(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) { m_parenLevel = ಮೌಲ್ಯ; }
    std::ಸದಿಶ<string>& ವಾದಗಳು() { ಹಿಂತಿರುಗಿ m_ವಾದಗಳು; }
    VDefineRef(ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//*************************************************************************
/// Data for parsing on/off

ವರ್ಗ VPre_If_ನಮೂದು final {
    // One for each pending ifdef/ifndef
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_on;  // Current parse for this ifdef level is "on"
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_everOn;  // Some if term in elsif tree has been on
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ on() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_on; }
    ತಾರ್ಕಿಕ everOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_everOn; }
    VPre_If_ನಮೂದು(ತಾರ್ಕಿಕ on, ತಾರ್ಕಿಕ everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_ನಮೂದು() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

ವರ್ಗ V3PreProcImp final : ಸಾರ್ವಜನಿಕ V3PreProc {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ಬಳಸಿ ನಿರೂಪಿಸುತ್ತದೆ_Map = std::map<ಸ್ಥಿರಾಂಕ std::string, VDefine>;
    ಬಳಸಿ StrList = VInFilter::StrList;

    // Defines list
    ನಿರೂಪಿಸುತ್ತದೆ_Map m_ನಿರೂಪಿಸುತ್ತದೆ;  ///< Map of defines

    // STATE
    ಸ್ಥಿರಾಂಕ V3PreProc* m_preprocp = ಶೂನ್ಯಸೂಚಕ;  ///< Object we're holding data for
    V3PreLex* m_lexp = ಶೂನ್ಯಸೂಚಕ;  ///< Current lexer state (nullptr = closed)
    ಪೂರ್ಣಾಂಕ m_ಕೊನೆಯ_Lineno = 0;  // Last line number (stall detection)
    ಪೂರ್ಣಾಂಕ m_ಟೋಕನುಗಳು_On_ಸಾಲು = 0;  // Number of tokens on line (stall detection)

    ಎಣಿಕೆ ProcState : uint8_t {
        ps_ತುದಿ,
        ps_DEFNAME_UNDEF,
        ps_DEFNAME_DEFINE,
        ps_DEFNAME_IFDEF,
        ps_DEFNAME_IFNDEF,
        ps_DEFNAME_ELSIF,
        ps_DEFARG,
        ps_DEFFORM,
        ps_DEFPAREN,
        ps_DEFVALUE,
        ps_ERRORNAME,
        ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF,
        ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF,
        ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* procStateName(ProcState s) {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಸ್ಥಿತಿಗಳು[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ಹಿಂತಿರುಗಿ ಸ್ಥಿತಿಗಳು[s];
    }

    std::ರಾಶಿ<ProcState> m_ಸ್ಥಿತಿಗಳು;  ///< Current state of parser
    ಪೂರ್ಣಾಂಕ m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    ತಾರ್ಕಿಕ m_incError = ಸುಳ್ಳು;  ///< Include error found
    string m_ಕೊನೆಯ_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_ಸಾಲು_Cmt;  ///< Line comment(s) to be returned
    ತಾರ್ಕಿಕ m_ಸಾಲು_Cmt_Nl = ಸುಳ್ಳು;  ///< Newline needed before inserting lineCmt
    ಪೂರ್ಣಾಂಕ m_ಸಾಲು_ಕೂಡಿಸು = 0;  ///< Empty lines to return to maintain line count
    ತಾರ್ಕಿಕ m_rawAtBol = ನಿಜ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    ತಾರ್ಕಿಕ m_finAhead = ಸುಳ್ಳು;  ///< Have read a token ahead
    ಪೂರ್ಣಾಂಕ m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    ತಾರ್ಕಿಕ m_finAtBol = ನಿಜ;  ///< Last getFinalToken left us at beginning of line
    File_ಸಾಲು* m_finFilelinep = ಶೂನ್ಯಸೂಚಕ;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ರಾಶಿ<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ರಾಶಿ<VPre_If_ನಮೂದು> m_ifdef_ರಾಶಿ;  ///< Stack of true/false emitting evaluations
    ಚಿಹ್ನೆರಹಿತ m_def_ಆಳ = 0;  ///< How many `defines deep
    ತಾರ್ಕಿಕ m_defPutJoin = ಸುಳ್ಳು;  ///< Insert `` after substitution

    // For `` join
    std::ರಾಶಿ<string> m_join_ರಾಶಿ;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ m_ಅಭಿವ್ಯಕ್ತಿ_Parser;  ///< Parser for () expression
    ಪೂರ್ಣಾಂಕ m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_ಸಾಲು_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    ಶೂನ್ಯ v3error_ಕೊನೆ(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_ಕೊನೆ(str);
    }

    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* tokenName(ಪೂರ್ಣಾಂಕ tok);
    ಶೂನ್ಯ debugToken(ಪೂರ್ಣಾಂಕ tok, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cmtp);
    ಶೂನ್ಯ parse_ತುದಿ();
    ಶೂನ್ಯ parseUndef();

ಖಾಸಗಿ:
    // Internal methods
    ಶೂನ್ಯ ಕೊನೆ_ದ_One_File();
    string defineSubst(VDefineRef* refp);

    ತಾರ್ಕಿಕ defExists(ಸ್ಥಿರಾಂಕ string& name);
    ತಾರ್ಕಿಕ defCmdline(ಸ್ಥಿರಾಂಕ string& name);
    string def_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ string& name);
    string defParams(ಸ್ಥಿರಾಂಕ string& name);
    File_ಸಾಲು* defFileline(ಸ್ಥಿರಾಂಕ string& name);

    string commentCleanup(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ);
    ತಾರ್ಕಿಕ comment_Token_ಹೊಂದಿಕೆ(string& cmdr, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg);
    ಸ್ಥಿರ string trimWhitespace(ಸ್ಥಿರಾಂಕ string& strg, ತಾರ್ಕಿಕ trailing);
    ಶೂನ್ಯ unputString(ಸ್ಥಿರಾಂಕ string& strg);
    ಶೂನ್ಯ unputDefrefString(ಸ್ಥಿರಾಂಕ string& strg);

    ಶೂನ್ಯ parsingOn() {
        m_off--;
        ಆದರೆ (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    ಶೂನ್ಯ parsingOff() { m_off++; }

    ಪೂರ್ಣಾಂಕ getRawToken();
    ಪೂರ್ಣಾಂಕ getStateToken();
    ಪೂರ್ಣಾಂಕ getFinalToken(string& buf);

    ProcState state() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸ್ಥಿತಿಗಳು.ತುದಿ(); }
    ತಾರ್ಕಿಕ stateIsDefname() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    ಶೂನ್ಯ state_ತಳ್ಳು(ProcState state) { m_ಸ್ಥಿತಿಗಳು.ತಳ್ಳು(state); }
    ಶೂನ್ಯ state_ತೆಗೆ() {
        m_ಸ್ಥಿತಿಗಳು.ತೆಗೆ_2();
        ಆದರೆ (VL_UNCOVERABLE(m_ಸ್ಥಿತಿಗಳು.ಖಾಲಿಗಳು())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_ಸ್ಥಿತಿಗಳು.ತಳ್ಳು(ps_ತುದಿ);  // LCOV_EXCL_LINE
        }
    }
    ಶೂನ್ಯ stateChange(ProcState state) {
        state_ತೆಗೆ();
        state_ತಳ್ಳು(state);
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS, called from upper level shell
    ಶೂನ್ಯ openFile(File_ಸಾಲು* fl, VInFilter* filterp, ಸ್ಥಿರಾಂಕ string& filename) override;
    ತಾರ್ಕಿಕ isEof() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_lexp->curStreamp()->m_eof; }
    ಶೂನ್ಯ forceEof() { m_lexp->curStreamp()->m_eof = ನಿಜ; }
    string getline() override;
    ಶೂನ್ಯ ಸೇರಿಸು_Unreadback(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) override { m_ಸಾಲು_Cmt += ಪಠ್ಯ; }
    ಶೂನ್ಯ ಸೇರಿಸು_Unreadback_At_Bol(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ);
    ಶೂನ್ಯ ಕೂಡಿಸು_ಸಾಲು_Comment(ಪೂರ್ಣಾಂಕ enterExit);
    ಶೂನ್ಯ dump_ನಿರೂಪಿಸುತ್ತದೆ(std::ostream& os) override;
    ಶೂನ್ಯ candidate_ನಿರೂಪಿಸುತ್ತದೆ(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    ಶೂನ್ಯ comment(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) override;  // Comment detected (if keepComments==2)
    ಶೂನ್ಯ include(ಸ್ಥಿರಾಂಕ string& filename) override;  // Request a include file be processed
    ಶೂನ್ಯ undef(ಸ್ಥಿರಾಂಕ string& name) override;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ undefineall();
    ಶೂನ್ಯ define(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ, ಸ್ಥಿರಾಂಕ string& params,
                ತಾರ್ಕಿಕ cmdline) override;
    string remove_ನಿರೂಪಿಸುತ್ತದೆ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_ಸ್ಥಿತಿಗಳು.ತಳ್ಳು(ps_ತುದಿ); }
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ configure(File_ಸಾಲು* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = ಇದು;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = ಹೊಸ File_ಸಾಲು{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = ಹೊಸ V3PreLex{ಇದು, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        ಆದರೆ (m_lexp) VL_DO_CLEAR(ಅಳಿಸು m_lexp, m_lexp = ಶೂನ್ಯಸೂಚಕ);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_ಸಾಲು* fl) {
    V3PreProcImp* ಸ್ಥಿರಾಂಕ preprocp = ಹೊಸ V3PreProcImp;
    preprocp->configure(fl);
    ಹಿಂತಿರುಗಿ preprocp;
}

ಶೂನ್ಯ V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_ಅಭಿವ್ಯಕ್ತಿ::selfTest(); }

//*************************************************************************
//ನಿರೂಪಿಸುತ್ತದೆ

ಶೂನ್ಯ V3PreProcImp::undef(ಸ್ಥಿರಾಂಕ string& name) {
    ಆದರೆ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_ನಿರೂಪಿಸುತ್ತದೆ())
        ಸೇರಿಸು_Unreadback("`undef " + name + "\n");
    m_ನಿರೂಪಿಸುತ್ತದೆ.erase(name);
}
ಶೂನ್ಯ V3PreProcImp::undefineall() {
    ಆದರೆ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_ನಿರೂಪಿಸುತ್ತದೆ())
        ಸೇರಿಸು_Unreadback("`undefineall\n");
    ಗಾಗಿ (ನಿರೂಪಿಸುತ್ತದೆ_Map::iterator nextit, it = m_ನಿರೂಪಿಸುತ್ತದೆ.begin(); it != m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ(); it = nextit) {
        nextit = it;
        ++nextit;
        ಆದರೆ (!it->ಎರಡನೇ.cmdline()) m_ನಿರೂಪಿಸುತ್ತದೆ.erase(it);
    }
}
ತಾರ್ಕಿಕ V3PreProcImp::defExists(ಸ್ಥಿರಾಂಕ string& name) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_ನಿರೂಪಿಸುತ್ತದೆ.ಕಂಡುಹಿಡಿ(name);
    ಹಿಂತಿರುಗಿ (iter != m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ());
}
ತಾರ್ಕಿಕ V3PreProcImp::defCmdline(ಸ್ಥಿರಾಂಕ string& name) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_ನಿರೂಪಿಸುತ್ತದೆ.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (iter == m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ()) { ಹಿಂತಿರುಗಿ ಸುಳ್ಳು; }
    ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ.cmdline();
}
string V3PreProcImp::def_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ string& name) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_ನಿರೂಪಿಸುತ್ತದೆ.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (iter == m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ಹಿಂತಿರುಗಿ "";
    }
    ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ.ಮೌಲ್ಯ();
}
string V3PreProcImp::defParams(ಸ್ಥಿರಾಂಕ string& name) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_ನಿರೂಪಿಸುತ್ತದೆ.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (iter == m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ಹಿಂತಿರುಗಿ "";
    }
    ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ.params();
}
File_ಸಾಲು* V3PreProcImp::defFileline(ಸ್ಥಿರಾಂಕ string& name) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_ನಿರೂಪಿಸುತ್ತದೆ.ಕಂಡುಹಿಡಿ(name);
    ಆದರೆ (iter == m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
    ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ.fileline();
}
ಶೂನ್ಯ V3PreProcImp::define(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ,
                          ಸ್ಥಿರಾಂಕ string& params, ತಾರ್ಕಿಕ cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << ಮೌಲ್ಯ << "' params '" << params << "'");
    ಆದರೆ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_ನಿರೂಪಿಸುತ್ತದೆ()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string ಹೊರಹೋಗು;
        ಹೊರಹೋಗು.reserve(ಮೌಲ್ಯ.ಗಾತ್ರ_2());
        ಗಾಗಿ (string::ಗಾತ್ರ_type pos = 0; pos < ಮೌಲ್ಯ.ಗಾತ್ರ_2(); ++pos) {
            ಆದರೆ (ಮೌಲ್ಯ[pos] == '\n' && (pos == 0 || ಮೌಲ್ಯ[pos - 1] != '\\')) ಹೊರಹೋಗು += '\\';
            ಹೊರಹೋಗು += ಮೌಲ್ಯ[pos];
        }
        ಸೇರಿಸು_Unreadback("`define " + name + params + (ಹೊರಹೋಗು.ಖಾಲಿಗಳು() ? "" : " ") + ಹೊರಹೋಗು + "\n");
    }
    ಆದರೆ (!V3_Language_ಪದಗಳು::isKeyword("`"s + name).ಖಾಲಿಗಳು()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (defExists(name)) {
            ಆದರೆ (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << ಮೌಲ್ಯ
                               << "' to existing command line define value: '" << def_ಮೌಲ್ಯ(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_ಮೌಲ್ಯ(name) << (defParams(name).ಖಾಲಿಗಳು() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                ಹಿಂತಿರುಗಿ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!(def_ಮೌಲ್ಯ(name) == ಮೌಲ್ಯ
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << ಮೌಲ್ಯ
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_ಮೌಲ್ಯ(name) << (defParams(name).ಖಾಲಿಗಳು() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_ನಿರೂಪಿಸುತ್ತದೆ.emplace(name, VDefine{fl, ಮೌಲ್ಯ, params, cmdline});
    }
}

string V3PreProcImp::remove_ನಿರೂಪಿಸುತ್ತದೆ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    string val;
    string rtnsym = ಪಠ್ಯ;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        ಆದರೆ (xsym[0] == '`') xsym.erase(0, 1);
        ಆದರೆ (defExists(xsym)) {
            val = def_ಮೌಲ್ಯ(xsym);
            ಆದರೆ (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } ಇಲ್ಲದಿದ್ದರೆ {
                ನಿಲ್ಲಿಸು;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ನಿಲ್ಲಿಸು;
        }
    }
    ಹಿಂತಿರುಗಿ rtnsym;  // NA
}

//**********************************************************************
// Comments

ಶೂನ್ಯ V3PreProcImp::include(ಸ್ಥಿರಾಂಕ string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = ಪಠ್ಯ;
    string::ಗಾತ್ರ_type pos;
    ಆಗ ((pos = cmd.ಕಂಡುಹಿಡಿ("//")) != string::npos) cmd.replace(pos, 2, "");
    ಆಗ ((pos = cmd.ಕಂಡುಹಿಡಿ("/*")) != string::npos) cmd.replace(pos, 2, "");
    ಆಗ ((pos = cmd.ಕಂಡುಹಿಡಿ("*/")) != string::npos) cmd.replace(pos, 2, "");
    ಆಗ ((pos = cmd.ಕಂಡುಹಿಡಿ('\"')) != string::npos) cmd.replace(pos, 1, " ");
    ಆಗ ((pos = cmd.ಕಂಡುಹಿಡಿ('\t')) != string::npos) cmd.replace(pos, 1, " ");
    ಆಗ ((pos = cmd.ಕಂಡುಹಿಡಿ("  ")) != string::npos) cmd.replace(pos, 2, " ");
    ಆಗ (!cmd.ಖಾಲಿಗಳು() && std::ಖಾಲಿಯೇ(cmd[cmd.ಗಾತ್ರ_2() - 1])) cmd.erase(cmd.ಗಾತ್ರ_2() - 1);
    ಹಿಂತಿರುಗಿ cmd;
}

ತಾರ್ಕಿಕ V3PreProcImp::comment_Token_ಹೊಂದಿಕೆ(string& cmdr, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) {
    ಪೂರ್ಣಾಂಕ len = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(strg);
    ಆದರೆ (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::ಖಾಲಿಯೇ(cmdr[len]))) {
        ಆದರೆ (std::ಖಾಲಿಯೇ(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ಹಿಂತಿರುಗಿ ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
}

ಶೂನ್ಯ V3PreProcImp::comment(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    // Comment detected.  Only keep relevant data.
    ತಾರ್ಕಿಕ printed = ಸುಳ್ಳು;
    ಆದರೆ (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ಸೇರಿಸು_Unreadback(ಪಠ್ಯ);
        printed = ನಿಜ;
    }

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = ಪಠ್ಯ.c_str();
    ಆದರೆ (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ;
    }

    ಆಗ (std::ಖಾಲಿಯೇ(*cp)) ++cp;

    ತಾರ್ಕಿಕ synth = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ vlcomment = ಸುಳ್ಳು;
    ಆದರೆ ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("verilator");
        ಆದರೆ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ಹಿಂತಿರುಗಿ;
        }
        vlcomment = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VString::startsWith(cp, "synopsys")) {
        cp += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("synopsys");
        ಆದರೆ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ಹಿಂತಿರುಗಿ;
        }
        synth = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VString::startsWith(cp, "cadence")) {
        cp += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("cadence");
        synth = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VString::startsWith(cp, "pragma")) {
        cp += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("pragma");
        synth = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("ambit synthesis");
        synth = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ;
    }

    ಆಗ (std::ಖಾಲಿಯೇ(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    ಆದರೆ (synth) {
        ಆದರೆ (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            ಆದರೆ (comment_Token_ಹೊಂದಿಕೆ(cmd /*ref*/, "full_case")) {
                ಆದರೆ (!printed) ಸೇರಿಸು_Unreadback("/*verilator full_case*/");
            }
            ಆದರೆ (comment_Token_ಹೊಂದಿಕೆ(cmd /*ref*/, "parallel_case")) {
                ಆದರೆ (!printed) ಸೇರಿಸು_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.ಬಹಿರಂಗ_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        ಆದರೆ (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::ಗಾತ್ರ_type ಕೊನೆ_ದ_Cmd = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("public_flat_rw");
            ಆಗ (VString::isIdentifierChar(cmd[ಕೊನೆ_ದ_Cmd])) ++ಕೊನೆ_ದ_Cmd;
            string ತಳ_Cmd = cmd.substr(0, ಕೊನೆ_ದ_Cmd);
            string arg = cmd.substr(ಕೊನೆ_ದ_Cmd);
            ಆಗ (std::ಖಾಲಿಯೇ(arg[0])) arg = arg.substr(1);
            ಆದರೆ (arg.ಗಾತ್ರ_2() && ತಳ_Cmd == "public_flat_rw_on")
                ತಳ_Cmd += "_sns";  // different cmd for applying sensitivity
            ಆದರೆ (!printed) ಸೇರಿಸು_Unreadback("/*verilator " + ತಳ_Cmd + "*/ " + arg + " /**/");
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string ತಳ_Cmd = cmd.substr(0, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("dpi_c_decl"));
            string::ಗಾತ್ರ_type ಆರಂಭ_ದ_Arg = ತಳ_Cmd.ಗಾತ್ರ_2();
            ಆಗ (std::ಖಾಲಿಯೇ(cmd[ಆರಂಭ_ದ_Arg])) ಆರಂಭ_ದ_Arg++;
            string arg = cmd.substr(ಆರಂಭ_ದ_Arg);
            ಆದರೆ (arg.ಖಾಲಿಗಳು()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ಹಿಂತಿರುಗಿ;
            }
            ಆದರೆ (!printed) ಸೇರಿಸು_Unreadback("/*verilator " + ತಳ_Cmd + "*/ \"" + arg + "\" /**/");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (!printed) ಸೇರಿಸು_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_ಸಾಲು* V3PreProc::fileline() {
    ಸ್ಥಿರಾಂಕ V3PreProcImp* idatap = static_cast<V3PreProcImp*>(ಇದು);
    ಹಿಂತಿರುಗಿ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* V3PreProcImp::tokenName(ಪೂರ್ಣಾಂಕ tok) {
    ಆಯ್ಕೆ (tok) {
    ಸ್ಥಿತಿ VP_BACKQUOTE: ಹಿಂತಿರುಗಿ ("BACKQUOTE");
    ಸ್ಥಿತಿ VP_COMMENT: ಹಿಂತಿರುಗಿ ("COMMENT");
    ಸ್ಥಿತಿ VP_DEFARG: ಹಿಂತಿರುಗಿ ("DEFARG");
    ಸ್ಥಿತಿ VP_DEFFORM: ಹಿಂತಿರುಗಿ ("DEFFORM");
    ಸ್ಥಿತಿ VP_DEFINE: ಹಿಂತಿರುಗಿ ("DEFINE");
    ಸ್ಥಿತಿ VP_DEFREF: ಹಿಂತಿರುಗಿ ("DEFREF");
    ಸ್ಥಿತಿ VP_DEFREF_JOIN: ಹಿಂತಿರುಗಿ ("DEFREF_JOIN");
    ಸ್ಥಿತಿ VP_DEFVALUE: ಹಿಂತಿರುಗಿ ("DEFVALUE");
    ಸ್ಥಿತಿ VP_ELSE: ಹಿಂತಿರುಗಿ ("ELSE");
    ಸ್ಥಿತಿ VP_ELSIF: ಹಿಂತಿರುಗಿ ("ELSIF");
    ಸ್ಥಿತಿ VP_ENDIF: ಹಿಂತಿರುಗಿ ("ENDIF");
    ಸ್ಥಿತಿ VP_EOF: ಹಿಂತಿರುಗಿ ("EOF");
    ಸ್ಥಿತಿ VP_EOF_ERROR: ಹಿಂತಿರುಗಿ ("EOF_ERROR");
    ಸ್ಥಿತಿ VP_ERROR: ಹಿಂತಿರುಗಿ ("ERROR");
    ಸ್ಥಿತಿ VP_IFDEF: ಹಿಂತಿರುಗಿ ("IFDEF");
    ಸ್ಥಿತಿ VP_IFNDEF: ಹಿಂತಿರುಗಿ ("IFNDEF");
    ಸ್ಥಿತಿ VP_JOIN: ಹಿಂತಿರುಗಿ ("JOIN");
    ಸ್ಥಿತಿ VP_INCLUDE: ಹಿಂತಿರುಗಿ ("INCLUDE");
    ಸ್ಥಿತಿ VP_ಸಾಲು: ಹಿಂತಿರುಗಿ ("LINE");
    ಸ್ಥಿತಿ VP_STRIFY: ಹಿಂತಿರುಗಿ ("STRIFY");
    ಸ್ಥಿತಿ VP_STRING: ಹಿಂತಿರುಗಿ ("STRING");
    ಸ್ಥಿತಿ VP_ಚಿಹ್ನೆ: ಹಿಂತಿರುಗಿ ("SYMBOL");
    ಸ್ಥಿತಿ VP_ಚಿಹ್ನೆ_JOIN: ಹಿಂತಿರುಗಿ ("SYMBOL_JOIN");
    ಸ್ಥಿತಿ VP_ಪಠ್ಯ: ಹಿಂತಿರುಗಿ ("TEXT");
    ಸ್ಥಿತಿ VP_UNDEF: ಹಿಂತಿರುಗಿ ("UNDEF");
    ಸ್ಥಿತಿ VP_UNDEFINEALL: ಹಿಂತಿರುಗಿ ("UNDEFINEALL");
    ಸ್ಥಿತಿ VP_WHITE: ಹಿಂತಿರುಗಿ ("WHITE");
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ ("?");
    }
}

ಶೂನ್ಯ V3PreProcImp::unputString(ಸ್ಥಿರಾಂಕ string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    ಆದರೆ (VL_UNCOVERABLE(m_lexp->m_ಬಫರ್_State != m_lexp->current_ಬಫರ್())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು(strg);
}

ಶೂನ್ಯ V3PreProcImp::unputDefrefString(ಸ್ಥಿರಾಂಕ string& strg) {
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ multiline = std::ಎಣಿಕೆ_2(strg.begin(), strg.ಕೊನೆ(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ಸ್ಥಿರಾಂಕ string& strg, ತಾರ್ಕಿಕ trailing) {
    // Remove leading whitespace
    string ಹೊರಹೋಗು = strg;
    string::ಗಾತ್ರ_type leadspace = 0;
    ಆಗ (ಹೊರಹೋಗು.ಉದ್ದಳತೆ() > leadspace && std::ಖಾಲಿಯೇ(ಹೊರಹೋಗು[leadspace])) ++leadspace;
    ಆದರೆ (leadspace) ಹೊರಹೋಗು.erase(0, leadspace);
    // Remove trailing whitespace
    ಆದರೆ (trailing) {
        string::ಗಾತ್ರ_type trailspace = 0;
        ಆಗ (ಹೊರಹೋಗು.ಉದ್ದಳತೆ() > trailspace && std::ಖಾಲಿಯೇ(ಹೊರಹೋಗು[ಹೊರಹೋಗು.ಉದ್ದಳತೆ() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        ಆದರೆ (trailspace && ಹೊರಹೋಗು.ಉದ್ದಳತೆ() > trailspace && ಹೊರಹೋಗು[ಹೊರಹೋಗು.ಉದ್ದಳತೆ() - 1 - trailspace] == '\\')
            --trailspace;
        ಆದರೆ (trailspace) ಹೊರಹೋಗು.erase(ಹೊರಹೋಗು.ಉದ್ದಳತೆ() - trailspace, trailspace);
    }
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
}

string V3PreProcImp::defineSubst(VDefineRef* refp) {
    // Substitute out defines in a define reference.
    // (We also need to call here on non-param defines to handle `")
    // We could push the define text back into the lexer, but that's slow
    // and would make recursive definitions and parameter handling nasty.
    //
    // Note we parse the definition parameters and value here.  If a
    // parameterized define is used many, many times, we could cache the
    // parsed result.
    UINFO(4, "defineSubstIn  `" << refp->name() << " " << refp->params());
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < refp->ವಾದಗಳು().ಗಾತ್ರ_2(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->ವಾದಗಳು()[i] << "'");
    }
    // Grab value
    ಸ್ಥಿರಾಂಕ string ಮೌಲ್ಯ = def_ಮೌಲ್ಯ(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(ಮೌಲ್ಯ) << "'");

    std::map<ಸ್ಥಿರಾಂಕ string, string> arg_ಮೌಲ್ಯ_By_Name;
    {  // Parse argument list into map
        ಚಿಹ್ನೆರಹಿತ ಅಂಕೆ_ವಾದಗಳು = 0;
        string argName;
        ಪೂರ್ಣಾಂಕ paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        ತಾರ್ಕಿಕ quote = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ haveDefault = ಸುಳ್ಳು;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ಸ್ಥಿರಾಂಕ string params = refp->params();  // Must keep str in scope to get pointer
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = params.c_str();
        ಆದರೆ (*cp == '(') cp++;
        ಗಾಗಿ (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            ಆದರೆ (!quote && paren == 1) {
                ಆದರೆ (*cp == ')' || *cp == ',') {
                    string ಮೌಲ್ಯ_Def;
                    ಆದರೆ (haveDefault) {
                        ಮೌಲ್ಯ_Def = token;
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, ನಿಜ);
                    UINFO(4, "    Got Arg=" << ಅಂಕೆ_ವಾದಗಳು << "  argName='" << argName
                                            << "'  default='" << ಮೌಲ್ಯ_Def << "'");
                    // Parse it
                    ಆದರೆ (argName != "") {
                        ಆದರೆ (refp->ವಾದಗಳು().ಗಾತ್ರ_2() > ಅಂಕೆ_ವಾದಗಳು) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ಸ್ಥಿರಾಂಕ string arg = trimWhitespace(refp->ವಾದಗಳು()[ಅಂಕೆ_ವಾದಗಳು], ನಿಜ);
                            ಆದರೆ (arg != "") ಮೌಲ್ಯ_Def = arg;
                        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ಹಿಂತಿರುಗಿ " `" + refp->name() + " ";
                        }
                        ಅಂಕೆ_ವಾದಗಳು++;
                    }
                    arg_ಮೌಲ್ಯ_By_Name[argName] = ಮೌಲ್ಯ_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = ಸುಳ್ಳು;
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == '=') {
                    haveDefault = ನಿಜ;
                    argName = token;
                    token = "";
                    ಮುಂದುವರಿ;
                }
            }
            ಆದರೆ (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (!quote) {
                ಆದರೆ (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            ಆದರೆ (*cp == '"') quote = !quote;
            ಆದರೆ (*cp) token += *cp;
        }
        ಆದರೆ (refp->ವಾದಗಳು().ಗಾತ್ರ_2() > ಅಂಕೆ_ವಾದಗಳು
            // `define X() is ok to call with nothing
            && !(refp->ವಾದಗಳು().ಗಾತ್ರ_2() == 1 && ಅಂಕೆ_ವಾದಗಳು == 0
                 && trimWhitespace(refp->ವಾದಗಳು()[0], ಸುಳ್ಳು) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ಹಿಂತಿರುಗಿ " `" + refp->name() + " ";
        }
    }

    string ಹೊರಹೋಗು;
    {  // Parse substitution define using arguments
        string argName;
        ತಾರ್ಕಿಕ quote = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ triquote = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ backslashesc = ಸುಳ್ಳು;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = ಮೌಲ್ಯ.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            ಆದರೆ (!quote && !triquote && *cp == '\\') {
                backslashesc = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::ಖಾಲಿಯೇ(*cp)) {
                backslashesc = ಸುಳ್ಳು;
            }
            ಆದರೆ (!quote && !triquote) {
                ಆದರೆ (std::ಅಕ್ಷರವೇ(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::ಅಂಕಿಯೇ(*cp))) {
                    argName += *cp;
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (argName != "") {
                    // Found a possible variable substitution
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = arg_ಮೌಲ್ಯ_By_Name.ಕಂಡುಹಿಡಿ(argName);
                    ಆದರೆ (iter != arg_ಮೌಲ್ಯ_By_Name.ಕೊನೆ()) {
                        // Substitute
                        ಸ್ಥಿರಾಂಕ string subst = iter->ಎರಡನೇ;
                        ಆದರೆ (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            ಆದರೆ (ಹೊರಹೋಗು.ಗಾತ್ರ_2() >= 2 && ಹೊರಹೋಗು.substr(ಹೊರಹೋಗು.ಗಾತ್ರ_2() - 2) == "``") {
                                ಹೊರಹೋಗು.resize(ಹೊರಹೋಗು.ಗಾತ್ರ_2() - 2);
                            }
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            ಹೊರಹೋಗು += subst;
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಹೊರಹೋಗು += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                ಆದರೆ (cp[0] == '`' && cp[1] == '`') {
                    ಆದರೆ (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        ಹೊರಹೋಗು += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp[0] == '`' && cp[1] == '"') {
                    ಹೊರಹೋಗು += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    ಹೊರಹೋಗು += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp[0] == '`' && cp[1] == '\\') {
                    ಹೊರಹೋಗು += '\\';  // `\ means to put out a backslash
                    cp++;
                    ಮುಂದುವರಿ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    ಹೊರಹೋಗು += "\n";  // \\n newline
                    cp++;
                    ಮುಂದುವರಿ;
                }
            }
            ಆದರೆ (cp[0] == '\\' && cp[1] == '\"') {
                ಹೊರಹೋಗು += cp[0];  // \{any} Put out literal next character
                ಹೊರಹೋಗು += cp[1];
                cp++;
                ಮುಂದುವರಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                ಹೊರಹೋಗು += cp[0];
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                ಹೊರಹೋಗು += "\"\"\"";
                cp += 2;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (*cp == '"') quote = !quote;
            ಆದರೆ (*cp) ಹೊರಹೋಗು += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(ಹೊರಹೋಗು) << "'");
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
}

//**********************************************************************
// Parser routines

ಶೂನ್ಯ V3PreProcImp::openFile(File_ಸಾಲು*, VInFilter* filterp, ಸ್ಥಿರಾಂಕ string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    ಆದರೆ (m_incError) ಹಿಂತಿರುಗಿ;
    m_lexp->ಗಣ_YYDebug(debug() >= 5);
    V3File::ಕೂಡಿಸು_Src_Depend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ok = filterp->readWholefile(filename, wholefile /*ref*/);
    ಆದರೆ (!ok) {
        fileline()->v3error("File not found: " + filename);
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        ಆದರೆ (m_lexp->m_streamp_ರಾಶಿ.ಗಾತ್ರ_2() > V3PreProc::INCLUDE_ಆಳ_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }
        // There's already a file active.  Push it to work on the new one.
        ಕೂಡಿಸು_ಸಾಲು_Comment(0);
    }

    // Save file contents for future error reporting
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flsp = ಹೊಸ File_ಸಾಲು{filename};
    flsp->lineno(1);
    flsp->newContent();
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : wholefile) flsp->contentp()->ತಳ್ಳು_ಪಠ್ಯ(i);

    // Save contents for V3Control --contents
    ಆದರೆ (filename != V3Options::get_Std_Package_ಮಾರ್ಗ()) {
        ತಾರ್ಕಿಕ ಒಳಗೊಂಡಿದೆ_Vlt = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            ಆದರೆ (i.ಕಂಡುಹಿಡಿ("`verilator_config") != string::npos) {
                ಒಳಗೊಂಡಿದೆ_Vlt = ನಿಜ;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (!ಒಳಗೊಂಡಿದೆ_Vlt) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : wholefile) V3Control::contents_ತಳ್ಳು_ಪಠ್ಯ(i);
        }
    }

    // Create new stream structure
    m_lexp->ಪರಿಶೀಲನೆ_New_File(flsp);
    ಕೂಡಿಸು_ಸಾಲು_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    ಪೂರ್ಣಾಂಕ eof_newline = 0;  // Number of characters following last newline
    ಪೂರ್ಣಾಂಕ eof_lineno = 1;
    ಗಾಗಿ (StrList::iterator it = wholefile.begin(); it != wholefile.ಕೊನೆ(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        ತಾರ್ಕಿಕ strip = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ sp = it->ದತ್ತಾಂಶ();
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ep = sp + it->ಉದ್ದಳತೆ();
        // Only process if needed, as saves extra string allocations
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = sp; cp < ep; cp++) {
            ಆದರೆ (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = ನಿಜ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ++eof_newline;
            }
        }
        ಆದರೆ (strip) {
            string ಹೊರಹೋಗು;
            ಹೊರಹೋಗು.reserve(it->ಉದ್ದಳತೆ());
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = sp; cp < ep; cp++) {
                ಆದರೆ (!(*cp == '\r' || *cp == '\0')) ಹೊರಹೋಗು += *cp;
            }
            *it = ಹೊರಹೋಗು;
        }

        // Push the data to an internal buffer.
        m_lexp->ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು_ಹಿಂಭಾಗ(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    ಆದರೆ (eof_newline) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಹೊಸ File_ಸಾಲು{flsp};
        fl->contentLineno(eof_lineno);
        fl->ಸ್ತಂಭ_2(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest add newline.");
    }
}

ಶೂನ್ಯ V3PreProcImp::ಸೇರಿಸು_Unreadback_At_Bol(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    ಆದರೆ (m_ಸಾಲು_Cmt == "") {
        m_ಸಾಲು_Cmt_Nl = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_ಸಾಲು_Cmt[m_ಸಾಲು_Cmt.ಉದ್ದಳತೆ() - 1] != '\n') {
        ಸೇರಿಸು_Unreadback("\n");
    }
    ಸೇರಿಸು_Unreadback(ಪಠ್ಯ);
}

ಶೂನ್ಯ V3PreProcImp::ಕೂಡಿಸು_ಸಾಲು_Comment(ಪೂರ್ಣಾಂಕ enterExit) {
    ಆದರೆ (ಸಾಲು_Directives()) {
        ಸೇರಿಸು_Unreadback_At_Bol(m_lexp->curFilelinep()->ಸಾಲು_Directive_Strg(enterExit));
    }
}

ಶೂನ್ಯ V3PreProcImp::dump_ನಿರೂಪಿಸುತ್ತದೆ(std::ostream& os) {
    ಗಾಗಿ (ನಿರೂಪಿಸುತ್ತದೆ_Map::const_iterator it = m_ನಿರೂಪಿಸುತ್ತದೆ.begin(); it != m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ(); ++it) {
        os << "`define " << it->ಮೊದಲ;
        // No need to print "()" below as already part of params()
        ಆದರೆ (!it->ಎರಡನೇ.params().ಖಾಲಿಗಳು()) os << it->ಎರಡನೇ.params();
        ಆದರೆ (!it->ಎರಡನೇ.ಮೌಲ್ಯ().ಖಾಲಿಗಳು()) os << " " << it->ಎರಡನೇ.ಮೌಲ್ಯ();
        os << '\n';
    }
}

ಶೂನ್ಯ V3PreProcImp::candidate_ನಿರೂಪಿಸುತ್ತದೆ(VSpellCheck* spellerp) {
    ಗಾಗಿ (ನಿರೂಪಿಸುತ್ತದೆ_Map::const_iterator it = m_ನಿರೂಪಿಸುತ್ತದೆ.begin(); it != m_ನಿರೂಪಿಸುತ್ತದೆ.ಕೊನೆ(); ++it) {
        spellerp->ತಳ್ಳು_Candidate("`"s + it->ಮೊದಲ);
    }
}

ಪೂರ್ಣಾಂಕ V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    ಆಗ (ನಿಜ) {
    ಮುಂದಿನ_tok:
        ಆದರೆ (m_ಸಾಲು_ಕೂಡಿಸು) {
            m_ಸಾಲು_ಕೂಡಿಸು--;
            m_rawAtBol = ನಿಜ;
            yyourtext("\n", 1);
            ಆದರೆ (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ಹಿಂತಿರುಗಿ VP_WHITE;
        }
        ಆದರೆ (m_ಸಾಲು_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ಸ್ಥಿರ string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_ಸಾಲು_Cmt;
            ಆದರೆ (m_ಸಾಲು_Cmt_Nl) {
                ಆದರೆ (!m_rawAtBol) s_rtncmt.ಸೇರಿಸು(0, "\n");
                m_ಸಾಲು_Cmt_Nl = ಸುಳ್ಳು;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.ಉದ್ದಳತೆ());
            m_ಸಾಲು_Cmt = "";
            ಆದರೆ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            ಆದರೆ (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_ಮೌಲ್ಯ(yyourtext(), yyourleng());
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (debug() >= 5) debugToken(VP_ಪಠ್ಯ, "LCM");
                ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ;
            }
        }
        ಆದರೆ (isEof()) ಹಿಂತಿರುಗಿ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->ಆರಂಭ_Token();
        ಪೂರ್ಣಾಂಕ tok = m_lexp->lex();
        ಆದರೆ (debug() >= 5) debugToken(tok, "RAW");

        ಆದರೆ (m_ಕೊನೆಯ_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_ಕೊನೆಯ_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_ಟೋಕನುಗಳು_On_ಸಾಲು = 0;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (++m_ಟೋಕನುಗಳು_On_ಸಾಲು > v3Global.opt.preproc_Token_ಎಲ್ಲೆ()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_ಗಮ್ಯ_Str(v3Global.opt.preproc_Token_ಎಲ್ಲೆ())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        ಆದರೆ (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            ಆದರೆ (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            ಹೋಗು ಮುಂದಿನ_tok;
        }

        ಆದರೆ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ಹಿಂತಿರುಗಿ tok;
    }
}

ಶೂನ್ಯ V3PreProcImp::debugToken(ಪೂರ್ಣಾಂಕ tok, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cmtp) {
    ಸ್ಥಿರಾಂಕ ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    ಆದರೆ (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = buf.ಕಂಡುಹಿಡಿ('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        ಆಗ ((pos = buf.ಕಂಡುಹಿಡಿ('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ಸ್ಥಿರಾಂಕ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_ಲಂಬಸಾಲು();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<ಪೂರ್ಣಾಂಕ>(m_ಸ್ಥಿತಿಗಳು.ಗಾತ್ರ_2())
                       << ") dr" << m_defRefs.ಗಾತ್ರ_2() << ":  <" << m_lexp->current_ಆರಂಭ_State()
                       << ">" << tokenName(tok) << ": " << buf);
        ಆದರೆ (s_debugFileline >= 9) {
            std::ಹೊರಗೆ << m_lexp->m_tokFilelinep->warnContextSecondary() << ಸಾಲು_ಅಂತ್ಯ;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

ಪೂರ್ಣಾಂಕ V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    ಆಗ (ನಿಜ) {
    ಮುಂದಿನ_tok:
        ಆದರೆ (isEof()) ಹಿಂತಿರುಗಿ VP_EOF;
        ಪೂರ್ಣಾಂಕ tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        ಆದರೆ (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ಹಿಂತಿರುಗಿ tok;
        ಆದರೆ (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_ಪಠ್ಯ;
        ಆದರೆ (tok == VP_COMMENT) {
            ಆದರೆ (!m_off) {
                ಆದರೆ (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ಆದರೆ (m_ಸಾಲು_Cmt.ಖಾಲಿಗಳು()) ಸೇರಿಸು_Unreadback(" ");
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_lexp->m_keepComments) {
                    ಹಿಂತಿರುಗಿ tok;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ಸೇರಿಸು_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            ಗಾಗಿ (ಗಾತ್ರ_t len = 0; len < static_cast<ಗಾತ್ರ_t>(yyourleng()); len++) {
                ಆದರೆ (yyourtext()[len] == '\n') m_ಸಾಲು_ಕೂಡಿಸು++;
            }
            ಹೋಗು ಮುಂದಿನ_tok;
        }
        ಆದರೆ (tok == VP_ಸಾಲು) {
            ಕೂಡಿಸು_ಸಾಲು_Comment(m_lexp->m_enterExit);
            ಹೋಗು ಮುಂದಿನ_tok;
        }

        ಆದರೆ (tok == VP_DEFREF_JOIN) {
            // Here's something fun and unspecified as yet:
            // The existence of non-existence of a base define changes `` expansion
            //  `define QA_b zzz
            //  `define Q1 `QA``_b
            //   1Q1 -> zzz
            //  `define QA a
            //   `Q1 -> a_b
            // Note parenthesis make this unambiguous
            //  `define Q1 `QA()``_b  // -> a_b
            // This may be a side effect of how `UNDEFINED remains as `UNDEFINED,
            // but it screws up our method here.  So hardcode it.
            ಸ್ಥಿರಾಂಕ std::string name(yyourtext() + 1, yyourleng() - 1);
            ಆದರೆ (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = ನಿಜ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } ಇಲ್ಲದಿದ್ದರೆ {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        ಆದರೆ (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            ಆದರೆ (tok == VP_JOIN) ಹೋಗು ಮುಂದಿನ_tok;
            ಆದರೆ (tok == VP_ಚಿಹ್ನೆ_JOIN) tok = VP_ಚಿಹ್ನೆ;
        }
        ಆದರೆ (tok == VP_ಚಿಹ್ನೆ_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ಸ್ಥಿರಾಂಕ std::string ಹೊರಹೋಗು(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << ಹೊರಹೋಗು);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ರಾಶಿ.ತಳ್ಳು(ಹೊರಹೋಗು);
            state_ತಳ್ಳು(ps_JOIN);
            ಹೋಗು ಮುಂದಿನ_tok;
        }

        // Deal with some special parser states
        ಆಯ್ಕೆ (state()) {
        ಸ್ಥಿತಿ ps_ತುದಿ: {
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ ps_DEFNAME_UNDEF:  // FALLTHRU
        ಸ್ಥಿತಿ ps_DEFNAME_DEFINE:  // FALLTHRU
        ಸ್ಥಿತಿ ps_DEFNAME_IFDEF:  // FALLTHRU
        ಸ್ಥಿತಿ ps_DEFNAME_IFNDEF:  // FALLTHRU
        ಸ್ಥಿತಿ ps_DEFNAME_ELSIF: {
            ಆದರೆ (tok == VP_ಚಿಹ್ನೆ) {
                m_ಕೊನೆಯ_Sym.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
                ಆದರೆ (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    ತಾರ್ಕಿಕ enable = defExists(m_ಕೊನೆಯ_Sym);
                    UINFO(4, "Ifdef " << m_ಕೊನೆಯ_Sym << (enable ? " ON" : " OFF"));
                    ಆದರೆ (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ರಾಶಿ.emplace(VPre_If_ನಮೂದು{enable, ಸುಳ್ಳು});
                    ಆದರೆ (!enable) parsingOff();
                    state_ತೆಗೆ();
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state() == ps_DEFNAME_ELSIF) {
                    ಆದರೆ (m_ifdef_ರಾಶಿ.ಖಾಲಿಗಳು()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // Handle `else portion
                        ಸ್ಥಿರಾಂಕ VPre_If_ನಮೂದು ಕೊನೆಯ_If = m_ifdef_ರಾಶಿ.ತುದಿ();
                        m_ifdef_ರಾಶಿ.ತೆಗೆ_2();
                        ಆದರೆ (!ಕೊನೆಯ_If.on()) parsingOn();
                        // Handle `if portion
                        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ enable = !ಕೊನೆಯ_If.everOn() && defExists(m_ಕೊನೆಯ_Sym);
                        UINFO(4, "Elsif " << m_ಕೊನೆಯ_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_ರಾಶಿ.emplace(VPre_If_ನಮೂದು{enable, ಕೊನೆಯ_If.everOn()});
                        ಆದರೆ (!enable) parsingOff();
                    }
                    state_ತೆಗೆ();
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state() == ps_DEFNAME_UNDEF) {
                    ಆದರೆ (!m_off) {
                        UINFO(4, "Undef " << m_ಕೊನೆಯ_Sym);
                        undef(m_ಕೊನೆಯ_Sym);
                    }
                    state_ತೆಗೆ();
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ತಳ್ಳು_State_Def_Form();
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ) {
                // IE, something like comment between define and symbol
                ಆದರೆ (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ತಳ್ಳು_State_ಅಭಿವ್ಯಕ್ತಿ();
                    m_ಅಭಿವ್ಯಕ್ತಿ_Parser.reset(fileline());
                    m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level = 1;
                    ಆಯ್ಕೆ (state()) {
                    ಸ್ಥಿತಿ ps_DEFNAME_IFDEF: stateChange(ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF); ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ ps_DEFNAME_IFNDEF: stateChange(ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF); ನಿಲ್ಲಿಸು;
                    ಸ್ಥಿತಿ ps_DEFNAME_ELSIF: stateChange(ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF); ನಿಲ್ಲಿಸು;
                    ಪೂರ್ವನಿಯೋಜಿತ: v3fatalSrc("bad case");
                    }
                    ಹೋಗು ಮುಂದಿನ_tok;
                }
                ಆದರೆ (!m_off) {
                    ಹಿಂತಿರುಗಿ tok;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಹೋಗು ಮುಂದಿನ_tok;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                ನಿಲ್ಲಿಸು;
            } ಇಲ್ಲದಿದ್ದರೆ {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF:  // FALLTHRU
        ಸ್ಥಿತಿ ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF:  // FALLTHRU
        ಸ್ಥಿತಿ ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF: {
            // `ifdef ( *here*
            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = m_lexp->m_tokFilelinep;
            ಆದರೆ (tok == VP_ಚಿಹ್ನೆ) {
                m_ಕೊನೆಯ_Sym.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ exists = defExists(m_ಕೊನೆಯ_Sym);
                ಆದರೆ (exists) {
                    ಸ್ಥಿರಾಂಕ std::string ಮೌಲ್ಯ = def_ಮೌಲ್ಯ(m_ಕೊನೆಯ_Sym);
                    ಆದರೆ (VString::removeWhitespace(ಮೌಲ್ಯ) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_ಕೊನೆಯ_Sym << "' with value '" << ಮೌಲ್ಯ
                                << "'\n"
                                   "... Suggest change define '"
                                << m_ಕೊನೆಯ_Sym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, exists});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_WHITE) {
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                ಆದರೆ (--m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level > 0) {
                    m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::KET});
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Done with parsing expression
                    ತಾರ್ಕಿಕ enable = m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ಫಲಿತಾಂಶ();
                    UINFO(4, "ifdef() result=" << enable);
                    ಆದರೆ (state() == ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF || state() == ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF) {
                        ಆದರೆ (state() == ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF) enable = !enable;
                        m_ifdef_ರಾಶಿ.emplace(VPre_If_ನಮೂದು{enable, ಸುಳ್ಳು});
                        ಆದರೆ (!enable) parsingOff();
                        state_ತೆಗೆ();
                        ಹೋಗು ಮುಂದಿನ_tok;
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state() == ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF) {
                        ಆದರೆ (m_ifdef_ರಾಶಿ.ಖಾಲಿಗಳು()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } ಇಲ್ಲದಿದ್ದರೆ {
                            // Handle `else portion
                            ಸ್ಥಿರಾಂಕ VPre_If_ನಮೂದು ಕೊನೆಯ_If = m_ifdef_ರಾಶಿ.ತುದಿ();
                            m_ifdef_ರಾಶಿ.ತೆಗೆ_2();
                            ಆದರೆ (!ಕೊನೆಯ_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !ಕೊನೆಯ_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_ಕೊನೆಯ_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_ರಾಶಿ.emplace(VPre_If_ನಮೂದು{enable, ಕೊನೆಯ_If.everOn()});
                            ಆದರೆ (!enable) parsingOff();
                        }
                        state_ತೆಗೆ();
                    }
                    ಹೋಗು ಮುಂದಿನ_tok;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LNOT});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 2 && 0 == ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(yyourtext(), "&&", 2)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LAND});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 2 && 0 == ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(yyourtext(), "||", 2)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LOR});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 2 && 0 == ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(yyourtext(), "->", 2)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::IMP});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 3 && 0 == ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(yyourtext(), "<->", 3)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::EQV});
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (VString::removeWhitespace(string{yyourtext(), yyourleng()}).ಖಾಲಿಗಳು()) {
                    ಹಿಂತಿರುಗಿ tok;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ ps_DEFFORM: {
            ಆದರೆ (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_ಮೌಲ್ಯ;
                ಆದರೆ (debug() >= 5) {
                    ಹೊರಗೆ << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ತಳ್ಳು_State_Def_ಮೌಲ್ಯ();
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ) {
                // IE, something like comment in formals
                ಆದರೆ (!m_off) ಹಿಂತಿರುಗಿ tok;
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ ps_DEFVALUE: {
            ಸ್ಥಿರ string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            ಆದರೆ (tok == VP_DEFVALUE) {
                ಆದರೆ (debug() >= 5) {  // LCOV_EXCL_START
                    ಹೊರಗೆ << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_ಮೌಲ್ಯ)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ಸ್ಥಿರಾಂಕ string formals = m_formals;
                string ಮೌಲ್ಯ = m_lexp->m_def_ಮೌಲ್ಯ;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < formals.ಉದ್ದಳತೆ(); i++) {
                    ಆದರೆ (formals[i] == '\n') s_newlines += "\n";
                }
                ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < ಮೌಲ್ಯ.ಉದ್ದಳತೆ(); i++) {
                    ಆದರೆ (ಮೌಲ್ಯ[i] == '\n') s_newlines += "\n";
                }
                ಆದರೆ (!m_off) {
                    // Remove leading and trailing whitespace
                    ಮೌಲ್ಯ = trimWhitespace(ಮೌಲ್ಯ, ನಿಜ);
                    // Define it
                    UINFO(4, "Define " << m_ಕೊನೆಯ_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(ಮೌಲ್ಯ) << "'");
                    define(fileline(), m_ಕೊನೆಯ_Sym, ಮೌಲ್ಯ, formals, ಸುಳ್ಳು);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_ತೆಗೆ();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.ಉದ್ದಳತೆ());
            ಹಿಂತಿರುಗಿ VP_WHITE;
        }
        ಸ್ಥಿತಿ ps_DEFPAREN: {
            ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT(!m_defRefs.ಖಾಲಿಗಳು(), "Shouldn't be in DEFPAREN w/o active defref");
                ಸ್ಥಿರಾಂಕ VDefineRef* ಸ್ಥಿರಾಂಕ refp = &(m_defRefs.ತುದಿ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ ps_DEFARG: {
            UASSERT(!m_defRefs.ಖಾಲಿಗಳು(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.ತುದಿ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_ಮೌಲ್ಯ);
            m_lexp->m_def_ಮೌಲ್ಯ = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            ಆದರೆ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->ವಾದಗಳು().ತಳ್ಳು_ಹಿಂಭಾಗ(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ತಳ್ಳು_State_Def_Arg(1);
                refp->nextarg("");
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->ವಾದಗಳು().ತಳ್ಳು_ಹಿಂಭಾಗ(refp->nextarg());
                string ಹೊರಹೋಗು;
                ಆದರೆ (!m_off) {
                    ಹೊರಹೋಗು = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.ತೆಗೆ_2(), refp);
                state_ತೆಗೆ();
                ಆದರೆ (m_defRefs.ಖಾಲಿಗಳು() || state() == ps_STRIFY || state() == ps_JOIN) {
                    ಆದರೆ (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ರಾಶಿ.ಖಾಲಿಗಳು(), "`` join stack empty, but in a ``");
                        ಸ್ಥಿರಾಂಕ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                        m_join_ರಾಶಿ.ತೆಗೆ_2();
                        ಹೊರಹೋಗು.ಸೇರಿಸು(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << ಹೊರಹೋಗು);
                        state_ತೆಗೆ();
                    }
                    ಆದರೆ (!m_off) {
                        unputDefrefString(ಹೊರಹೋಗು);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.ಖಾಲಿಗಳು() ? 0 : m_defRefs.ತುದಿ().parenLevel();
                } ಇಲ್ಲದಿದ್ದರೆ {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.ತುದಿ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_ಮೌಲ್ಯ + ಹೊರಹೋಗು);
                    m_lexp->m_def_ಮೌಲ್ಯ = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                ನಿಲ್ಲಿಸು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಚಿಹ್ನೆ || tok == VP_STRING || tok == VP_ಪಠ್ಯ || tok == VP_WHITE) {
                string rtn;
                rtn.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ತಳ್ಳು(ps_STRIFY);
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ ps_INCNAME: {
            ಆದರೆ (tok == VP_STRING) {
                state_ತೆಗೆ();
                m_ಕೊನೆಯ_Sym.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_ಕೊನೆಯ_Sym);
                // Drop leading and trailing quotes.
                m_ಕೊನೆಯ_Sym.erase(0, 1);
                m_ಕೊನೆಯ_Sym.erase(m_ಕೊನೆಯ_Sym.ಉದ್ದಳತೆ() - 1, 1);
                include(m_ಕೊನೆಯ_Sym);
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_ಪಠ್ಯ && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ತಳ್ಳು_State_Inc_Filename();
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                ನಿಲ್ಲಿಸು;
            } ಇಲ್ಲದಿದ್ದರೆ {
                state_ತೆಗೆ();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                ಹೋಗು ಮುಂದಿನ_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        ಸ್ಥಿತಿ ps_ERRORNAME: {
            ಆದರೆ (tok == VP_STRING) {
                ಆದರೆ (!m_off) {
                    m_ಕೊನೆಯ_Sym.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_ಕೊನೆಯ_Sym);
                }
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ ps_JOIN: {
            ಆದರೆ (tok == VP_ಚಿಹ್ನೆ || tok == VP_ಪಠ್ಯ || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ರಾಶಿ.ಖಾಲಿಗಳು(), "`` join stack empty, but in a ``");
                ಸ್ಥಿರಾಂಕ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                m_join_ರಾಶಿ.ತೆಗೆ_2();
                UINFO(5, "`` LHS:" << lhs);
                ಸ್ಥಿರಾಂಕ string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ಸ್ಥಿರಾಂಕ string ಹೊರಹೋಗು = lhs + rhs;
                UINFO(5, "`` Out:" << ಹೊರಹೋಗು);
                unputString(ಹೊರಹೋಗು);
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ಸ್ಥಿರಾಂಕ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                m_join_ರಾಶಿ.ತೆಗೆ_2();
                unputString(lhs);
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // `define, etc, fall through and expand.  Pop back here.
                ನಿಲ್ಲಿಸು;
            }
        }
        ಸ್ಥಿತಿ ps_STRIFY: {
            ಆದರೆ (tok == VP_STRIFY) {
                // Quote what's in the middle of the stringification
                // Note a `" MACRO_WITH(`") `" doesn't need to be handled (we don't need a stack)
                // That behavior isn't specified, and other simulators vary widely
                string ಹೊರಹೋಗು = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(ಹೊರಹೋಗು.begin(), ಹೊರಹೋಗು.ಕೊನೆ(), '\n', ' ');
                unputString("\""s + ಹೊರಹೋಗು + "\"");
                state_ತೆಗೆ();
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                ನಿಲ್ಲಿಸು;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                ಹೋಗು ಮುಂದಿನ_tok;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                ನಿಲ್ಲಿಸು;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಪೂರ್ವನಿಯೋಜಿತ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        ಆಯ್ಕೆ (tok) {
        ಸ್ಥಿತಿ VP_INCLUDE:
            ಆದರೆ (!m_off) {
                state_ತಳ್ಳು(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_UNDEF: state_ತಳ್ಳು(ps_DEFNAME_UNDEF); ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ತಳ್ಳು(ps_DEFNAME_DEFINE);
            ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_IFDEF: state_ತಳ್ಳು(ps_DEFNAME_IFDEF); ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_IFNDEF: state_ತಳ್ಳು(ps_DEFNAME_IFNDEF); ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_ELSIF: state_ತಳ್ಳು(ps_DEFNAME_ELSIF); ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_ELSE:
            ಆದರೆ (m_ifdef_ರಾಶಿ.ಖಾಲಿಗಳು()) {
                fileline()->v3error("`else with no matching `if");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ VPre_If_ನಮೂದು ಕೊನೆಯ_If = m_ifdef_ರಾಶಿ.ತುದಿ();
                m_ifdef_ರಾಶಿ.ತೆಗೆ_2();
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ enable = !ಕೊನೆಯ_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ರಾಶಿ.emplace(VPre_If_ನಮೂದು{enable, ಕೊನೆಯ_If.everOn()});
                ಆದರೆ (!ಕೊನೆಯ_If.on()) parsingOn();
                ಆದರೆ (!enable) parsingOff();
            }
            ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_ENDIF:
            UINFO(4, "Endif ");
            ಆದರೆ (m_ifdef_ರಾಶಿ.ಖಾಲಿಗಳು()) {
                fileline()->v3error("`endif with no matching `if");
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ VPre_If_ನಮೂದು ಕೊನೆಯ_If = m_ifdef_ರಾಶಿ.ತುದಿ();
                m_ifdef_ರಾಶಿ.ತೆಗೆ_2();
                ಆದರೆ (!ಕೊನೆಯ_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            ಹೋಗು ಮುಂದಿನ_tok;

        ಸ್ಥಿತಿ VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ಸ್ಥಿರಾಂಕ string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            ಆದರೆ (m_defPutJoin) {
                m_defPutJoin = ಸುಳ್ಳು;
                unputString("``");
            }
            ಆದರೆ (m_def_ಆಳ++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                ಹೋಗು ಮುಂದಿನ_tok;
            }
            // Substitute
            ಆದರೆ (!defExists(name)) {  // Not found, return original string as-is
                m_def_ಆಳ = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                ಆದರೆ (m_off) {
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    ಹೋಗು ಮುಂದಿನ_tok;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ string params = defParams(name);
                ಆದರೆ (params == "0" || params == "") {  // Found, as simple substitution
                    string ಹೊರಹೋಗು;
                    ಆದರೆ (!m_off) {
                        VDefineRef tempref{name, ""};
                        ಹೊರಹೋಗು = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    ಆದರೆ (m_defRefs.ಖಾಲಿಗಳು() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        ಆದರೆ (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ರಾಶಿ.ಖಾಲಿಗಳು(), "`` join stack empty, but in a ``");
                            ಸ್ಥಿರಾಂಕ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                            m_join_ರಾಶಿ.ತೆಗೆ_2();
                            ಹೊರಹೋಗು.ಸೇರಿಸು(0, lhs);
                            UINFO(5, "``-end-defref Out:" << ಹೊರಹೋಗು);
                            state_ತೆಗೆ();
                        }
                        ಆದರೆ (!m_off) {
                            unputDefrefString(ಹೊರಹೋಗು);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ಸ್ಥಿರಾಂಕ refp = &(m_defRefs.ತುದಿ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_ಮೌಲ್ಯ + ಹೊರಹೋಗು);
                        m_lexp->m_def_ಮೌಲ್ಯ = "";
                    }
                    ಹೋಗು ಮುಂದಿನ_tok;
                } ಇಲ್ಲದಿದ್ದರೆ {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    ಆದರೆ (!m_defRefs.ಖಾಲಿಗಳು()) m_defRefs.ತುದಿ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ತಳ್ಳು(ps_DEFPAREN);
                    m_lexp->ತಳ್ಳು_State_Def_Arg(0);
                    ಹೋಗು ಮುಂದಿನ_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            ಹೋಗು ಮುಂದಿನ_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        ಸ್ಥಿತಿ VP_ERROR: {
            state_ತಳ್ಳು(ps_ERRORNAME);
            ಹೋಗು ಮುಂದಿನ_tok;
        }
        ಸ್ಥಿತಿ VP_EOF:
            ಆದರೆ (!m_ifdef_ರಾಶಿ.ಖಾಲಿಗಳು()) fileline()->v3error("`ifdef not terminated at EOF");
            ಹಿಂತಿರುಗಿ tok;
        ಸ್ಥಿತಿ VP_UNDEFINEALL:
            ಆದರೆ (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ತಳ್ಳು(ps_STRIFY);
            ಹೋಗು ಮುಂದಿನ_tok;
        ಸ್ಥಿತಿ VP_ಚಿಹ್ನೆ:
        ಸ್ಥಿತಿ VP_STRING:
        ಸ್ಥಿತಿ VP_ಪಠ್ಯ: {
            m_def_ಆಳ = 0;
            ಆದರೆ (!m_off) {
                ಹಿಂತಿರುಗಿ tok;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೋಗು ಮುಂದಿನ_tok;
            }
        }
        ಸ್ಥಿತಿ VP_WHITE:  // Handled at top of loop
        ಸ್ಥಿತಿ VP_COMMENT:  // Handled at top of loop
        ಸ್ಥಿತಿ VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        ಸ್ಥಿತಿ VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            ನಿಲ್ಲಿಸು;  // LCOV_EXCL_LINE
        }
        ಹಿಂತಿರುಗಿ tok;
    }
}

ಪೂರ್ಣಾಂಕ V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    ಆದರೆ (!m_finAhead) {
        m_finAhead = ನಿಜ;
        m_finToken = getStateToken();
        m_finBuf.ನಿಯೋಜಿಸು_2(yyourtext(), yyourleng());
    }
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ tok = m_finToken;
    buf = m_finBuf;
    ಆದರೆ (ಸುಳ್ಳು && debug() >= 5) {
        ಸ್ಥಿರಾಂಕ string bufcln = V3PreLex::cleanDbgStrg(buf);
        ಸ್ಥಿರಾಂಕ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_ಲಂಬಸಾಲು();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* bufp = buf.c_str();
    ಆಗ (*bufp == '\n') bufp++;
    ಆದರೆ ((tok == VP_ಪಠ್ಯ || tok == VP_ಸಾಲು) && VString::startsWith(bufp, "`line ")) {
        ಪೂರ್ಣಾಂಕ enter;
        m_finFilelinep->ಸಾಲು_Directive(bufp, enter /*ref*/);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (m_finAtBol && !(tok == VP_ಪಠ್ಯ && buf == "\n") && m_preprocp->ಸಾಲು_Directives()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಹೊರಹೋಗು_Behind
                = (m_lexp->m_tokFilelinep->ಕೊನೆಯ_Lineno() - m_finFilelinep->ಕೊನೆಯ_Lineno())) {
                ಆದರೆ (debug() >= 5) {
                    ಸ್ಥಿರಾಂಕ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_ಲಂಬಸಾಲು();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->ಕೊನೆಯ_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->ಕೊನೆಯ_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->ಕೊನೆಯ_Lineno());
                ಆದರೆ (ಹೊರಹೋಗು_Behind > 0
                    && (ಹೊರಹೋಗು_Behind <= static_cast<ಪೂರ್ಣಾಂಕ>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    ಆದರೆ (m_preprocp->keepWhitespace()) {
                        buf = std::string(ಹೊರಹೋಗು_Behind, '\n');  // () for char repeat
                        ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // Need to backup, use `line
                    buf = m_finFilelinep->ಸಾಲು_Directive_Strg(0);
                    ಹಿಂತಿರುಗಿ VP_ಸಾಲು;
                }
            }
        }
        // Track newlines in prep for next token
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ& c : buf) {
            ಆದರೆ (c == '\n') {
                m_finAtBol = ನಿಜ;
                m_finFilelinep->linenoInc();
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_finAtBol = ಸುಳ್ಳು;
            }
        }
    }
    m_finAhead = ಸುಳ್ಳು;  // Consumed the token
    ಹಿಂತಿರುಗಿ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    ಆದರೆ (isEof()) ಹಿಂತಿರುಗಿ "";
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* rtnp;
    ತಾರ್ಕಿಕ gotEof = ಸುಳ್ಳು;
    ಆಗ (ಶೂನ್ಯಸೂಚಕ == (rtnp = std::strchr(m_ಸಾಲು_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ tok = getFinalToken(m_tokenBuf /*ref*/);
        ಆದರೆ (debug() >= 5) {
            ಸ್ಥಿರಾಂಕ string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ಸ್ಥಿರಾಂಕ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_ಲಂಬಸಾಲು();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        ಆದರೆ (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            ಆದರೆ (m_ಸಾಲು_Chars != "" && m_ಸಾಲು_Chars[m_ಸಾಲು_Chars.ಉದ್ದಳತೆ() - 1] != '\n') {
                m_ಸಾಲು_Chars.append("\n");
            }
            gotEof = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ಸಾಲು_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ len = rtnp - m_ಸಾಲು_Chars.c_str() + 1;
    string the_ಸಾಲು(m_ಸಾಲು_Chars, 0, len);
    m_ಸಾಲು_Chars = m_ಸಾಲು_Chars.erase(0, len);  // Remove returned characters
    ಆದರೆ (debug() >= 4) {
        ಸ್ಥಿರಾಂಕ string lncln = V3PreLex::cleanDbgStrg(the_ಸಾಲು);
        ಸ್ಥಿರಾಂಕ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_ಲಂಬಸಾಲು();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ಹಿಂತಿರುಗಿ the_ಸಾಲು;
}
