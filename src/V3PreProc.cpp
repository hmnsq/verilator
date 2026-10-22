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

#define VL_MT_DISABLED_CODE_UNIT 1

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

#define yyFlexLexer V3Lexer
#ಅನ್ತರ್ಭಾವ "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

ವರ್ಗ VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_ಸಾಲು* ಧ್ರುವ m_fileline;  // Where it was declared
    ಧ್ರುವ string m_ಮೌಲ್ಯ;  // Value of define
    ಧ್ರುವ string m_params;  // Parameters
    ಧ್ರುವ ತಾರ್ಕಿಕ m_cmdline;  // Set on command line, don't `undefineall
ಸಾರ್ವಜನಿಕ:
    VDefine(File_ಸಾಲು* fl, ಧ್ರುವ string& ಮೌಲ್ಯ, ಧ್ರುವ string& params, ತಾರ್ಕಿಕ cmdline)
        : m_fileline{fl}
        , m_ಮೌಲ್ಯ{ಮೌಲ್ಯ}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fileline; }
    string ಮೌಲ್ಯ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ; }
    string params() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_params; }
    ತಾರ್ಕಿಕ cmdline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cmdline; }
};

//*************************************************************************

ವರ್ಗ VDefineRef final {
    // One for each pending define substitution
    ಧ್ರುವ string m_name;  // Define last name being defined
    ಧ್ರುವ string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    ಪೂರ್ಣಾಙ್ಕ m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
ಸಾರ್ವಜನಿಕ:
    string name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
    string params() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_params; }
    string nextarg() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nextarg; }
    ಶೂನ್ಯ nextarg(ಧ್ರುವ string& ಮೌಲ್ಯ) { m_nextarg = ಮೌಲ್ಯ; }
    ಪೂರ್ಣಾಙ್ಕ parenLevel() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parenLevel; }
    ಶೂನ್ಯ parenLevel(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) { m_parenLevel = ಮೌಲ್ಯ; }
    std::vector<string>& args() { ಪ್ರತ್ಯಾಗಮ m_args; }
    VDefineRef(ಧ್ರುವ string& name, ಧ್ರುವ string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = ಸ್ವಭಾವ;
};

//*************************************************************************
/// Data for parsing on/off

ವರ್ಗ VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ಧ್ರುವ ತಾರ್ಕಿಕ m_on;  // Current parse for this ifdef level is "on"
    ಧ್ರುವ ತಾರ್ಕಿಕ m_everOn;  // Some if term in elsif tree has been on
ಸಾರ್ವಜನಿಕ:
    ತಾರ್ಕಿಕ on() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_on; }
    ತಾರ್ಕಿಕ everOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_everOn; }
    VPreIfEntry(ತಾರ್ಕಿಕ on, ತಾರ್ಕಿಕ everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = ಸ್ವಭಾವ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

ವರ್ಗ V3PreProcImp final : ಸಾರ್ವಜನಿಕ V3PreProc {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using DefinesMap = std::map<ಧ್ರುವ std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ಧ್ರುವ V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    ಪೂರ್ಣಾಙ್ಕ m_lastLineno = 0;  // Last line number (stall detection)
    ಪೂರ್ಣಾಙ್ಕ m_tokens_On_ಸಾಲು = 0;  // Number of tokens on line (stall detection)

    ಗಣನಾ ProcState : uint8_t {
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
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* procStateName(ProcState s) {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ಪ್ರತ್ಯಾಗಮ states[s];
    }

    std::ರಾಶಿ<ProcState> m_states;  ///< Current state of parser
    ಪೂರ್ಣಾಙ್ಕ m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    ತಾರ್ಕಿಕ m_incError = ಅಸತ್ಯ;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_ಸಾಲು_Cmt;  ///< Line comment(s) to be returned
    ತಾರ್ಕಿಕ m_ಸಾಲು_Cmt_Nl = ಅಸತ್ಯ;  ///< Newline needed before inserting lineCmt
    ಪೂರ್ಣಾಙ್ಕ m_ಸಾಲು_Add = 0;  ///< Empty lines to return to maintain line count
    ತಾರ್ಕಿಕ m_rawAtBol = ಸತ್ಯ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    ತಾರ್ಕಿಕ m_finAhead = ಅಸತ್ಯ;  ///< Have read a token ahead
    ಪೂರ್ಣಾಙ್ಕ m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    ತಾರ್ಕಿಕ m_finAtBol = ಸತ್ಯ;  ///< Last getFinalToken left us at beginning of line
    File_ಸಾಲು* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ರಾಶಿ<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ರಾಶಿ<VPreIfEntry> m_ifdef_ರಾಶಿ;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    ತಾರ್ಕಿಕ m_defPutJoin = ಅಸತ್ಯ;  ///< Insert `` after substitution

    // For `` join
    std::ರಾಶಿ<string> m_join_ರಾಶಿ;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ m_ಅಭಿವ್ಯಕ್ತಿ_Parser;  ///< Parser for () expression
    ಪೂರ್ಣಾಙ್ಕ m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_ಸಾಲು_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    ಶೂನ್ಯ v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* tokenName(ಪೂರ್ಣಾಙ್ಕ tok);
    ಶೂನ್ಯ debugToken(ಪೂರ್ಣಾಙ್ಕ tok, ಧ್ರುವ ವರ್ಣ* cmtp);
    ಶೂನ್ಯ parse_ತುದಿ();
    ಶೂನ್ಯ parseUndef();

ಗುಪ್ತ:
    // Internal methods
    ಶೂನ್ಯ endOfOneFile();
    string defineSubst(VDefineRef* refp);

    ತಾರ್ಕಿಕ defExists(ಧ್ರುವ string& name);
    ತಾರ್ಕಿಕ defCmdline(ಧ್ರುವ string& name);
    string def_ಮೌಲ್ಯ(ಧ್ರುವ string& name);
    string defParams(ಧ್ರುವ string& name);
    File_ಸಾಲು* defFileline(ಧ್ರುವ string& name);

    string commentCleanup(ಧ್ರುವ string& text);
    ತಾರ್ಕಿಕ commentTokenMatch(string& cmdr, ಧ್ರುವ ವರ್ಣ* strg);
    ಸ್ಥಿರ string trimWhitespace(ಧ್ರುವ string& strg, ತಾರ್ಕಿಕ trailing);
    ಶೂನ್ಯ unputString(ಧ್ರುವ string& strg);
    ಶೂನ್ಯ unputDefrefString(ಧ್ರುವ string& strg);

    ಶೂನ್ಯ parsingOn() {
        m_off--;
        ಯದಿ (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    ಶೂನ್ಯ parsingOff() { m_off++; }

    ಪೂರ್ಣಾಙ್ಕ getRawToken();
    ಪೂರ್ಣಾಙ್ಕ getStateToken();
    ಪೂರ್ಣಾಙ್ಕ getFinalToken(string& buf);

    ProcState state() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_states.ತುದಿ(); }
    ತಾರ್ಕಿಕ stateIsDefname() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    ಶೂನ್ಯ state_ತಳ್ಳು(ProcState state) { m_states.ತಳ್ಳು(state); }
    ಶೂನ್ಯ state_ತೆಗೆ() {
        m_states.ತೆಗೆ();
        ಯದಿ (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.ತಳ್ಳು(ps_ತುದಿ);  // LCOV_EXCL_LINE
        }
    }
    ಶೂನ್ಯ stateChange(ProcState state) {
        state_ತೆಗೆ();
        state_ತಳ್ಳು(state);
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS, called from upper level shell
    ಶೂನ್ಯ openFile(File_ಸಾಲು* fl, VInFilter* filterp, ಧ್ರುವ string& filename) override;
    ತಾರ್ಕಿಕ isEof() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ m_lexp->curStreamp()->m_eof; }
    ಶೂನ್ಯ forceEof() { m_lexp->curStreamp()->m_eof = ಸತ್ಯ; }
    string getline() override;
    ಶೂನ್ಯ ಸೇರಿಸು_Unreadback(ಧ್ರುವ string& text) override { m_ಸಾಲು_Cmt += text; }
    ಶೂನ್ಯ ಸೇರಿಸು_Unreadback_At_Bol(ಧ್ರುವ string& text);
    ಶೂನ್ಯ add_ಸಾಲು_Comment(ಪೂರ್ಣಾಙ್ಕ enterExit);
    ಶೂನ್ಯ dumpDefines(std::ostream& os) override;
    ಶೂನ್ಯ candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    ಶೂನ್ಯ comment(ಧ್ರುವ string& text) override;  // Comment detected (if keepComments==2)
    ಶೂನ್ಯ include(ಧ್ರುವ string& filename) override;  // Request a include file be processed
    ಶೂನ್ಯ undef(ಧ್ರುವ string& name) override;
    ಆಭಾಸ ಶೂನ್ಯ undefineall();
    ಶೂನ್ಯ define(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& ಮೌಲ್ಯ, ಧ್ರುವ string& params,
                ತಾರ್ಕಿಕ cmdline) override;
    string removeDefines(ಧ್ರುವ string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.ತಳ್ಳು(ps_ತುದಿ); }
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ configure(File_ಸಾಲು* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = ಏತತ್;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = ನೂತನ File_ಸಾಲು{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = ನೂತನ V3PreLex{ಏತತ್, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        ಯದಿ (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_ಸಾಲು* fl) {
    V3PreProcImp* ಧ್ರುವ preprocp = ನೂತನ V3PreProcImp;
    preprocp->configure(fl);
    ಪ್ರತ್ಯಾಗಮ preprocp;
}

ಶೂನ್ಯ V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_ಅಭಿವ್ಯಕ್ತಿ::selfTest(); }

//*************************************************************************
// Defines

ಶೂನ್ಯ V3PreProcImp::undef(ಧ್ರುವ string& name) {
    ಯದಿ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ಸೇರಿಸು_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
ಶೂನ್ಯ V3PreProcImp::undefineall() {
    ಯದಿ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ಸೇರಿಸು_Unreadback("`undefineall\n");
    ಪ್ರತಿ (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        ಯದಿ (!it->second.cmdline()) m_defines.erase(it);
    }
}
ತಾರ್ಕಿಕ V3PreProcImp::defExists(ಧ್ರುವ string& name) {
    ಧ್ರುವ auto iter = m_defines.find(name);
    ಪ್ರತ್ಯಾಗಮ (iter != m_defines.end());
}
ತಾರ್ಕಿಕ V3PreProcImp::defCmdline(ಧ್ರುವ string& name) {
    ಧ್ರುವ auto iter = m_defines.find(name);
    ಯದಿ (iter == m_defines.end()) { ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ; }
    ಪ್ರತ್ಯಾಗಮ iter->second.cmdline();
}
string V3PreProcImp::def_ಮೌಲ್ಯ(ಧ್ರುವ string& name) {
    ಧ್ರುವ auto iter = m_defines.find(name);
    ಯದಿ (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ಪ್ರತ್ಯಾಗಮ "";
    }
    ಪ್ರತ್ಯಾಗಮ iter->second.ಮೌಲ್ಯ();
}
string V3PreProcImp::defParams(ಧ್ರುವ string& name) {
    ಧ್ರುವ auto iter = m_defines.find(name);
    ಯದಿ (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ಪ್ರತ್ಯಾಗಮ "";
    }
    ಪ್ರತ್ಯಾಗಮ iter->second.params();
}
File_ಸಾಲು* V3PreProcImp::defFileline(ಧ್ರುವ string& name) {
    ಧ್ರುವ auto iter = m_defines.find(name);
    ಯದಿ (iter == m_defines.end()) ಪ್ರತ್ಯಾಗಮ nullptr;
    ಪ್ರತ್ಯಾಗಮ iter->second.fileline();
}
ಶೂನ್ಯ V3PreProcImp::define(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string& ಮೌಲ್ಯ,
                          ಧ್ರುವ string& params, ತಾರ್ಕಿಕ cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << ಮೌಲ್ಯ << "' params '" << params << "'");
    ಯದಿ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(ಮೌಲ್ಯ.size());
        ಪ್ರತಿ (string::size_type pos = 0; pos < ಮೌಲ್ಯ.size(); ++pos) {
            ಯದಿ (ಮೌಲ್ಯ[pos] == '\n' && (pos == 0 || ಮೌಲ್ಯ[pos - 1] != '\\')) out += '\\';
            out += ಮೌಲ್ಯ[pos];
        }
        ಸೇರಿಸು_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    ಯದಿ (!V3_Language_ಪದಗಳು::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } ಅನ್ಯಥಾ {
        ಯದಿ (defExists(name)) {
            ಯದಿ (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << ಮೌಲ್ಯ
                               << "' to existing command line define value: '" << def_ಮೌಲ್ಯ(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_ಮೌಲ್ಯ(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                ಯದಿ (!(def_ಮೌಲ್ಯ(name) == ಮೌಲ್ಯ
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << ಮೌಲ್ಯ
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_ಮೌಲ್ಯ(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, ಮೌಲ್ಯ, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(ಧ್ರುವ string& text) {
    string val;
    string rtnsym = text;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        ಯದಿ (xsym[0] == '`') xsym.erase(0, 1);
        ಯದಿ (defExists(xsym)) {
            val = def_ಮೌಲ್ಯ(xsym);
            ಯದಿ (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } ಅನ್ಯಥಾ {
                ಭಙ್ಗ;
            }
        } ಅನ್ಯಥಾ {
            ಭಙ್ಗ;
        }
    }
    ಪ್ರತ್ಯಾಗಮ rtnsym;  // NA
}

//**********************************************************************
// Comments

ಶೂನ್ಯ V3PreProcImp::include(ಧ್ರುವ string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ಧ್ರುವ string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    ಯಾವತ್ ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    ಯಾವತ್ ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    ಯಾವತ್ ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    ಯಾವತ್ ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    ಯಾವತ್ ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    ಯಾವತ್ ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    ಯಾವತ್ (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    ಪ್ರತ್ಯಾಗಮ cmd;
}

ತಾರ್ಕಿಕ V3PreProcImp::commentTokenMatch(string& cmdr, ಧ್ರುವ ವರ್ಣ* strg) {
    ಪೂರ್ಣಾಙ್ಕ len = std::strlen(strg);
    ಯದಿ (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        ಯದಿ (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
}

ಶೂನ್ಯ V3PreProcImp::comment(ಧ್ರುವ string& text) {
    // Comment detected.  Only keep relevant data.
    ತಾರ್ಕಿಕ printed = ಅಸತ್ಯ;
    ಯದಿ (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ಸೇರಿಸು_Unreadback(text);
        printed = ಸತ್ಯ;
    }

    ಧ್ರುವ ವರ್ಣ* cp = text.c_str();
    ಯದಿ (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯಾವತ್ (std::isspace(*cp)) ++cp;

    ತಾರ್ಕಿಕ synth = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ vlcomment = ಅಸತ್ಯ;
    ಯದಿ ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        ಯದಿ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ಪ್ರತ್ಯಾಗಮ;
        }
        vlcomment = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        ಯದಿ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ಪ್ರತ್ಯಾಗಮ;
        }
        synth = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = ಸತ್ಯ;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯಾವತ್ (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    ಯದಿ (synth) {
        ಯದಿ (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            ಯದಿ (commentTokenMatch(cmd /*ref*/, "full_case")) {
                ಯದಿ (!printed) ಸೇರಿಸು_Unreadback("/*verilator full_case*/");
            }
            ಯದಿ (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                ಯದಿ (!printed) ಸೇರಿಸು_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } ಅನ್ಯಥಾ ಯದಿ (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        ಯದಿ (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            ಯಾವತ್ (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            ಯಾವತ್ (std::isspace(arg[0])) arg = arg.substr(1);
            ಯದಿ (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            ಯದಿ (!printed) ಸೇರಿಸು_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } ಅನ್ಯಥಾ ಯದಿ (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            ಯಾವತ್ (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            ಯದಿ (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (!printed) ಸೇರಿಸು_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } ಅನ್ಯಥಾ {
            ಯದಿ (!printed) ಸೇರಿಸು_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_ಸಾಲು* V3PreProc::fileline() {
    ಧ್ರುವ V3PreProcImp* idatap = static_cast<V3PreProcImp*>(ಏತತ್);
    ಪ್ರತ್ಯಾಗಮ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ಧ್ರುವ ವರ್ಣ* V3PreProcImp::tokenName(ಪೂರ್ಣಾಙ್ಕ tok) {
    ವಿಕಲ್ಪ (tok) {
    ಪಕ್ಷ VP_BACKQUOTE: ಪ್ರತ್ಯಾಗಮ ("BACKQUOTE");
    ಪಕ್ಷ VP_COMMENT: ಪ್ರತ್ಯಾಗಮ ("COMMENT");
    ಪಕ್ಷ VP_DEFARG: ಪ್ರತ್ಯಾಗಮ ("DEFARG");
    ಪಕ್ಷ VP_DEFFORM: ಪ್ರತ್ಯಾಗಮ ("DEFFORM");
    ಪಕ್ಷ VP_DEFINE: ಪ್ರತ್ಯಾಗಮ ("DEFINE");
    ಪಕ್ಷ VP_DEFREF: ಪ್ರತ್ಯಾಗಮ ("DEFREF");
    ಪಕ್ಷ VP_DEFREF_JOIN: ಪ್ರತ್ಯಾಗಮ ("DEFREF_JOIN");
    ಪಕ್ಷ VP_DEFVALUE: ಪ್ರತ್ಯಾಗಮ ("DEFVALUE");
    ಪಕ್ಷ VP_ELSE: ಪ್ರತ್ಯಾಗಮ ("ELSE");
    ಪಕ್ಷ VP_ELSIF: ಪ್ರತ್ಯಾಗಮ ("ELSIF");
    ಪಕ್ಷ VP_ENDIF: ಪ್ರತ್ಯಾಗಮ ("ENDIF");
    ಪಕ್ಷ VP_EOF: ಪ್ರತ್ಯಾಗಮ ("EOF");
    ಪಕ್ಷ VP_EOF_ERROR: ಪ್ರತ್ಯಾಗಮ ("EOF_ERROR");
    ಪಕ್ಷ VP_ERROR: ಪ್ರತ್ಯಾಗಮ ("ERROR");
    ಪಕ್ಷ VP_IFDEF: ಪ್ರತ್ಯಾಗಮ ("IFDEF");
    ಪಕ್ಷ VP_IFNDEF: ಪ್ರತ್ಯಾಗಮ ("IFNDEF");
    ಪಕ್ಷ VP_JOIN: ಪ್ರತ್ಯಾಗಮ ("JOIN");
    ಪಕ್ಷ VP_INCLUDE: ಪ್ರತ್ಯಾಗಮ ("INCLUDE");
    ಪಕ್ಷ VP_ಸಾಲು: ಪ್ರತ್ಯಾಗಮ ("LINE");
    ಪಕ್ಷ VP_STRIFY: ಪ್ರತ್ಯಾಗಮ ("STRIFY");
    ಪಕ್ಷ VP_STRING: ಪ್ರತ್ಯಾಗಮ ("STRING");
    ಪಕ್ಷ VP_ಚಿಹ್ನೆ: ಪ್ರತ್ಯಾಗಮ ("SYMBOL");
    ಪಕ್ಷ VP_ಚಿಹ್ನೆ_JOIN: ಪ್ರತ್ಯಾಗಮ ("SYMBOL_JOIN");
    ಪಕ್ಷ VP_TEXT: ಪ್ರತ್ಯಾಗಮ ("TEXT");
    ಪಕ್ಷ VP_UNDEF: ಪ್ರತ್ಯಾಗಮ ("UNDEF");
    ಪಕ್ಷ VP_UNDEFINEALL: ಪ್ರತ್ಯಾಗಮ ("UNDEFINEALL");
    ಪಕ್ಷ VP_WHITE: ಪ್ರತ್ಯಾಗಮ ("WHITE");
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ ("?");
    }
}

ಶೂನ್ಯ V3PreProcImp::unputString(ಧ್ರುವ string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    ಯದಿ (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

ಶೂನ್ಯ V3PreProcImp::unputDefrefString(ಧ್ರುವ string& strg) {
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ multiline = std::ಎಣಿಕೆ(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ಧ್ರುವ string& strg, ತಾರ್ಕಿಕ trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    ಯಾವತ್ (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    ಯದಿ (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    ಯದಿ (trailing) {
        string::size_type trailspace = 0;
        ಯಾವತ್ (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        ಯದಿ (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        ಯದಿ (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    ಪ್ರತ್ಯಾಗಮ out;
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
    ಪ್ರತಿ (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ಧ್ರುವ string ಮೌಲ್ಯ = def_ಮೌಲ್ಯ(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(ಮೌಲ್ಯ) << "'");

    std::map<ಧ್ರುವ string, string> arg_ಮೌಲ್ಯ_By_Name;
    {  // Parse argument list into map
        unsigned ಅಂಕೆ_Args = 0;
        string argName;
        ಪೂರ್ಣಾಙ್ಕ paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        ತಾರ್ಕಿಕ quote = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ haveDefault = ಅಸತ್ಯ;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ಧ್ರುವ string params = refp->params();  // Must keep str in scope to get pointer
        ಧ್ರುವ ವರ್ಣ* cp = params.c_str();
        ಯದಿ (*cp == '(') cp++;
        ಪ್ರತಿ (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            ಯದಿ (!quote && paren == 1) {
                ಯದಿ (*cp == ')' || *cp == ',') {
                    string ಮೌಲ್ಯ_Def;
                    ಯದಿ (haveDefault) {
                        ಮೌಲ್ಯ_Def = token;
                    } ಅನ್ಯಥಾ {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, ಸತ್ಯ);
                    UINFO(4, "    Got Arg=" << ಅಂಕೆ_Args << "  argName='" << argName
                                            << "'  default='" << ಮೌಲ್ಯ_Def << "'");
                    // Parse it
                    ಯದಿ (argName != "") {
                        ಯದಿ (refp->args().size() > ಅಂಕೆ_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ಧ್ರುವ string arg = trimWhitespace(refp->args()[ಅಂಕೆ_Args], ಸತ್ಯ);
                            ಯದಿ (arg != "") ಮೌಲ್ಯ_Def = arg;
                        } ಅನ್ಯಥಾ ಯದಿ (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ಪ್ರತ್ಯಾಗಮ " `" + refp->name() + " ";
                        }
                        ಅಂಕೆ_Args++;
                    }
                    arg_ಮೌಲ್ಯ_By_Name[argName] = ಮೌಲ್ಯ_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = ಅಸತ್ಯ;
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (*cp == '=') {
                    haveDefault = ಸತ್ಯ;
                    argName = token;
                    token = "";
                    ಅನುವರ್ತ;
                }
            }
            ಯದಿ (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                ಅನುವರ್ತ;
            }
            ಯದಿ (!quote) {
                ಯದಿ (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } ಅನ್ಯಥಾ ಯದಿ (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            ಯದಿ (*cp == '"') quote = !quote;
            ಯದಿ (*cp) token += *cp;
        }
        ಯದಿ (refp->args().size() > ಅಂಕೆ_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && ಅಂಕೆ_Args == 0
                 && trimWhitespace(refp->args()[0], ಅಸತ್ಯ) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ಪ್ರತ್ಯಾಗಮ " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        ತಾರ್ಕಿಕ quote = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ triquote = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ backslashesc = ಅಸತ್ಯ;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = ಮೌಲ್ಯ.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            ಯದಿ (!quote && !triquote && *cp == '\\') {
                backslashesc = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (std::isspace(*cp)) {
                backslashesc = ಅಸತ್ಯ;
            }
            ಯದಿ (!quote && !triquote) {
                ಯದಿ (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    ಅನುವರ್ತ;
                }
                ಯದಿ (argName != "") {
                    // Found a possible variable substitution
                    ಧ್ರುವ auto iter = arg_ಮೌಲ್ಯ_By_Name.find(argName);
                    ಯದಿ (iter != arg_ಮೌಲ್ಯ_By_Name.end()) {
                        // Substitute
                        ಧ್ರುವ string subst = iter->second;
                        ಯದಿ (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            ಯದಿ (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } ಅನ್ಯಥಾ {
                            out += subst;
                        }
                    } ಅನ್ಯಥಾ {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                ಯದಿ (cp[0] == '`' && cp[1] == '`') {
                    ಯದಿ (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } ಅನ್ಯಥಾ {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    ಅನುವರ್ತ;
                }
            }
            ಯದಿ (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                ಅನುವರ್ತ;
            } ಅನ್ಯಥಾ ಯದಿ (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                ಅನುವರ್ತ;
            }
            ಯದಿ (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                ಅನುವರ್ತ;
            }
            ಯದಿ (*cp == '"') quote = !quote;
            ಯದಿ (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    ಪ್ರತ್ಯಾಗಮ out;
}

//**********************************************************************
// Parser routines

ಶೂನ್ಯ V3PreProcImp::openFile(File_ಸಾಲು*, VInFilter* filterp, ಧ್ರುವ string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    ಯದಿ (m_incError) ಪ್ರತ್ಯಾಗಮ;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ಧ್ರುವ ತಾರ್ಕಿಕ ok = filterp->readWholefile(filename, wholefile /*ref*/);
    ಯದಿ (!ok) {
        fileline()->v3error("File not found: " + filename);
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        ಯದಿ (m_lexp->m_streamp_ರಾಶಿ.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = ಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        // There's already a file active.  Push it to work on the new one.
        add_ಸಾಲು_Comment(0);
    }

    // Save file contents for future error reporting
    File_ಸಾಲು* ಧ್ರುವ flsp = ನೂತನ File_ಸಾಲು{filename};
    flsp->lineno(1);
    flsp->newContent();
    ಪ್ರತಿ (ಧ್ರುವ string& i : wholefile) flsp->contentp()->ತಳ್ಳು_Text(i);

    // Save contents for V3Control --contents
    ಯದಿ (filename != V3Options::getStdPackagePath()) {
        ತಾರ್ಕಿಕ containsVlt = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            ಯದಿ (i.find("`verilator_config") != string::npos) {
                containsVlt = ಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (!containsVlt) {
            ಪ್ರತಿ (ಧ್ರುವ string& i : wholefile) V3Control::contents_ತಳ್ಳು_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_ಸಾಲು_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    ಪೂರ್ಣಾಙ್ಕ eof_newline = 0;  // Number of characters following last newline
    ಪೂರ್ಣಾಙ್ಕ eof_lineno = 1;
    ಪ್ರತಿ (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        ತಾರ್ಕಿಕ strip = ಅಸತ್ಯ;
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ sp = it->data();
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = sp; cp < ep; cp++) {
            ಯದಿ (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } ಅನ್ಯಥಾ {
                ++eof_newline;
            }
        }
        ಯದಿ (strip) {
            string out;
            out.reserve(it->length());
            ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = sp; cp < ep; cp++) {
                ಯದಿ (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    ಯದಿ (eof_newline) {
        File_ಸಾಲು* ಧ್ರುವ fl = ನೂತನ File_ಸಾಲು{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

ಶೂನ್ಯ V3PreProcImp::ಸೇರಿಸು_Unreadback_At_Bol(ಧ್ರುವ string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    ಯದಿ (m_ಸಾಲು_Cmt == "") {
        m_ಸಾಲು_Cmt_Nl = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (m_ಸಾಲು_Cmt[m_ಸಾಲು_Cmt.length() - 1] != '\n') {
        ಸೇರಿಸು_Unreadback("\n");
    }
    ಸೇರಿಸು_Unreadback(text);
}

ಶೂನ್ಯ V3PreProcImp::add_ಸಾಲು_Comment(ಪೂರ್ಣಾಙ್ಕ enterExit) {
    ಯದಿ (ಸಾಲು_Directives()) {
        ಸೇರಿಸು_Unreadback_At_Bol(m_lexp->curFilelinep()->ಸಾಲು_Directive_Strg(enterExit));
    }
}

ಶೂನ್ಯ V3PreProcImp::dumpDefines(std::ostream& os) {
    ಪ್ರತಿ (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        ಯದಿ (!it->second.params().empty()) os << it->second.params();
        ಯದಿ (!it->second.ಮೌಲ್ಯ().empty()) os << " " << it->second.ಮೌಲ್ಯ();
        os << '\n';
    }
}

ಶೂನ್ಯ V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    ಪ್ರತಿ (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->ತಳ್ಳು_Candidate("`"s + it->first);
    }
}

ಪೂರ್ಣಾಙ್ಕ V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    ಯಾವತ್ (ಸತ್ಯ) {
    next_tok:
        ಯದಿ (m_ಸಾಲು_Add) {
            m_ಸಾಲು_Add--;
            m_rawAtBol = ಸತ್ಯ;
            yyourtext("\n", 1);
            ಯದಿ (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ಪ್ರತ್ಯಾಗಮ VP_WHITE;
        }
        ಯದಿ (m_ಸಾಲು_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ಸ್ಥಿರ string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_ಸಾಲು_Cmt;
            ಯದಿ (m_ಸಾಲು_Cmt_Nl) {
                ಯದಿ (!m_rawAtBol) s_rtncmt.ಸೇರಿಸು(0, "\n");
                m_ಸಾಲು_Cmt_Nl = ಅಸತ್ಯ;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_ಸಾಲು_Cmt = "";
            ಯದಿ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            ಯದಿ (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_ಮೌಲ್ಯ(yyourtext(), yyourleng());
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                ಯದಿ (debug() >= 5) debugToken(VP_TEXT, "LCM");
                ಪ್ರತ್ಯಾಗಮ VP_TEXT;
            }
        }
        ಯದಿ (isEof()) ಪ್ರತ್ಯಾಗಮ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        ಪೂರ್ಣಾಙ್ಕ tok = m_lexp->lex();
        ಯದಿ (debug() >= 5) debugToken(tok, "RAW");

        ಯದಿ (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_ಸಾಲು = 0;
        } ಅನ್ಯಥಾ ಯದಿ (++m_tokens_On_ಸಾಲು > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        ಯದಿ (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            ಯದಿ (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            ಗಚ್ಛ next_tok;
        }

        ಯದಿ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ಪ್ರತ್ಯಾಗಮ tok;
    }
}

ಶೂನ್ಯ V3PreProcImp::debugToken(ಪೂರ್ಣಾಙ್ಕ tok, ಧ್ರುವ ವರ್ಣ* cmtp) {
    ಧ್ರುವ ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    ಯದಿ (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        ಯಾವತ್ ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        ಯಾವತ್ ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ಧ್ರುವ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        ಯದಿ (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

ಪೂರ್ಣಾಙ್ಕ V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    ಯಾವತ್ (ಸತ್ಯ) {
    next_tok:
        ಯದಿ (isEof()) ಪ್ರತ್ಯಾಗಮ VP_EOF;
        ಪೂರ್ಣಾಙ್ಕ tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        ಯದಿ (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ಪ್ರತ್ಯಾಗಮ tok;
        ಯದಿ (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        ಯದಿ (tok == VP_COMMENT) {
            ಯದಿ (!m_off) {
                ಯದಿ (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ಯದಿ (m_ಸಾಲು_Cmt.empty()) ಸೇರಿಸು_Unreadback(" ");
                } ಅನ್ಯಥಾ ಯದಿ (m_lexp->m_keepComments) {
                    ಪ್ರತ್ಯಾಗಮ tok;
                } ಅನ್ಯಥಾ {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ಸೇರಿಸು_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            ಪ್ರತಿ (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                ಯದಿ (yyourtext()[len] == '\n') m_ಸಾಲು_Add++;
            }
            ಗಚ್ಛ next_tok;
        }
        ಯದಿ (tok == VP_ಸಾಲು) {
            add_ಸಾಲು_Comment(m_lexp->m_enterExit);
            ಗಚ್ಛ next_tok;
        }

        ಯದಿ (tok == VP_DEFREF_JOIN) {
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
            ಧ್ರುವ std::string name(yyourtext() + 1, yyourleng() - 1);
            ಯದಿ (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = ಸತ್ಯ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } ಅನ್ಯಥಾ {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        ಯದಿ (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            ಯದಿ (tok == VP_JOIN) ಗಚ್ಛ next_tok;
            ಯದಿ (tok == VP_ಚಿಹ್ನೆ_JOIN) tok = VP_ಚಿಹ್ನೆ;
        }
        ಯದಿ (tok == VP_ಚಿಹ್ನೆ_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ಧ್ರುವ std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ರಾಶಿ.ತಳ್ಳು(out);
            state_ತಳ್ಳು(ps_JOIN);
            ಗಚ್ಛ next_tok;
        }

        // Deal with some special parser states
        ವಿಕಲ್ಪ (state()) {
        ಪಕ್ಷ ps_ತುದಿ: {
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ ps_DEFNAME_UNDEF:  // FALLTHRU
        ಪಕ್ಷ ps_DEFNAME_DEFINE:  // FALLTHRU
        ಪಕ್ಷ ps_DEFNAME_IFDEF:  // FALLTHRU
        ಪಕ್ಷ ps_DEFNAME_IFNDEF:  // FALLTHRU
        ಪಕ್ಷ ps_DEFNAME_ELSIF: {
            ಯದಿ (tok == VP_ಚಿಹ್ನೆ) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ಯದಿ (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    ತಾರ್ಕಿಕ enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    ಯದಿ (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ರಾಶಿ.emplace(VPreIfEntry{enable, ಅಸತ್ಯ});
                    ಯದಿ (!enable) parsingOff();
                    state_ತೆಗೆ();
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ ಯದಿ (state() == ps_DEFNAME_ELSIF) {
                    ಯದಿ (m_ifdef_ರಾಶಿ.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } ಅನ್ಯಥಾ {
                        // Handle `else portion
                        ಧ್ರುವ VPreIfEntry lastIf = m_ifdef_ರಾಶಿ.ತುದಿ();
                        m_ifdef_ರಾಶಿ.ತೆಗೆ();
                        ಯದಿ (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ಧ್ರುವ ತಾರ್ಕಿಕ enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_ರಾಶಿ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        ಯದಿ (!enable) parsingOff();
                    }
                    state_ತೆಗೆ();
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ ಯದಿ (state() == ps_DEFNAME_UNDEF) {
                    ಯದಿ (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_ತೆಗೆ();
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ ಯದಿ (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ತಳ್ಳು_State_Def_Form();
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                ಯದಿ (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ತಳ್ಳು_State_ಅಭಿವ್ಯಕ್ತಿ();
                    m_ಅಭಿವ್ಯಕ್ತಿ_Parser.reset(fileline());
                    m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level = 1;
                    ವಿಕಲ್ಪ (state()) {
                    ಪಕ್ಷ ps_DEFNAME_IFDEF: stateChange(ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF); ಭಙ್ಗ;
                    ಪಕ್ಷ ps_DEFNAME_IFNDEF: stateChange(ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF); ಭಙ್ಗ;
                    ಪಕ್ಷ ps_DEFNAME_ELSIF: stateChange(ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF); ಭಙ್ಗ;
                    ಸ್ವಭಾವ: v3fatalSrc("bad case");
                    }
                    ಗಚ್ಛ next_tok;
                }
                ಯದಿ (!m_off) {
                    ಪ್ರತ್ಯಾಗಮ tok;
                } ಅನ್ಯಥಾ {
                    ಗಚ್ಛ next_tok;
                }
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                ಭಙ್ಗ;
            } ಅನ್ಯಥಾ {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF:  // FALLTHRU
        ಪಕ್ಷ ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF:  // FALLTHRU
        ಪಕ್ಷ ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF: {
            // `ifdef ( *here*
            File_ಸಾಲು* ಧ್ರುವ flp = m_lexp->m_tokFilelinep;
            ಯದಿ (tok == VP_ಚಿಹ್ನೆ) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ಧ್ರುವ ತಾರ್ಕಿಕ exists = defExists(m_lastSym);
                ಯದಿ (exists) {
                    ಧ್ರುವ std::string ಮೌಲ್ಯ = def_ಮೌಲ್ಯ(m_lastSym);
                    ಯದಿ (VString::removeWhitespace(ಮೌಲ್ಯ) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << ಮೌಲ್ಯ
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, exists});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_WHITE) {
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                ಯದಿ (--m_ಅಭಿವ್ಯಕ್ತಿ_Paren_Level > 0) {
                    m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::KET});
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ {
                    // Done with parsing expression
                    ತಾರ್ಕಿಕ enable = m_ಅಭಿವ್ಯಕ್ತಿ_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    ಯದಿ (state() == ps_ಅಭಿವ್ಯಕ್ತಿ_IFDEF || state() == ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF) {
                        ಯದಿ (state() == ps_ಅಭಿವ್ಯಕ್ತಿ_IFNDEF) enable = !enable;
                        m_ifdef_ರಾಶಿ.emplace(VPreIfEntry{enable, ಅಸತ್ಯ});
                        ಯದಿ (!enable) parsingOff();
                        state_ತೆಗೆ();
                        ಗಚ್ಛ next_tok;
                    } ಅನ್ಯಥಾ ಯದಿ (state() == ps_ಅಭಿವ್ಯಕ್ತಿ_ELSIF) {
                        ಯದಿ (m_ifdef_ರಾಶಿ.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } ಅನ್ಯಥಾ {
                            // Handle `else portion
                            ಧ್ರುವ VPreIfEntry lastIf = m_ifdef_ರಾಶಿ.ತುದಿ();
                            m_ifdef_ರಾಶಿ.ತೆಗೆ();
                            ಯದಿ (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_ರಾಶಿ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            ಯದಿ (!enable) parsingOff();
                        }
                        state_ತೆಗೆ();
                    }
                    ಗಚ್ಛ next_tok;
                }
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LNOT});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LAND});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LOR});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::IMP});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_ಅಭಿವ್ಯಕ್ತಿ_Parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::EQV});
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                ಯದಿ (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    ಪ್ರತ್ಯಾಗಮ tok;
                } ಅನ್ಯಥಾ {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ ps_DEFFORM: {
            ಯದಿ (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_ಮೌಲ್ಯ;
                ಯದಿ (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ತಳ್ಳು_State_Def_ಮೌಲ್ಯ();
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT) {
                // IE, something like comment in formals
                ಯದಿ (!m_off) ಪ್ರತ್ಯಾಗಮ tok;
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ ps_DEFVALUE: {
            ಸ್ಥಿರ string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            ಯದಿ (tok == VP_DEFVALUE) {
                ಯದಿ (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_ಮೌಲ್ಯ)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ಧ್ರುವ string formals = m_formals;
                string ಮೌಲ್ಯ = m_lexp->m_def_ಮೌಲ್ಯ;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                ಪ್ರತಿ (size_t i = 0; i < formals.length(); i++) {
                    ಯದಿ (formals[i] == '\n') s_newlines += "\n";
                }
                ಪ್ರತಿ (size_t i = 0; i < ಮೌಲ್ಯ.length(); i++) {
                    ಯದಿ (ಮೌಲ್ಯ[i] == '\n') s_newlines += "\n";
                }
                ಯದಿ (!m_off) {
                    // Remove leading and trailing whitespace
                    ಮೌಲ್ಯ = trimWhitespace(ಮೌಲ್ಯ, ಸತ್ಯ);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(ಮೌಲ್ಯ) << "'");
                    define(fileline(), m_lastSym, ಮೌಲ್ಯ, formals, ಅಸತ್ಯ);
                }
            } ಅನ್ಯಥಾ {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_ತೆಗೆ();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            ಪ್ರತ್ಯಾಗಮ VP_WHITE;
        }
        ಪಕ್ಷ ps_DEFPAREN: {
            ಯದಿ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ಧ್ರುವ VDefineRef* ಧ್ರುವ refp = &(m_defRefs.ತುದಿ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.ತುದಿ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_ಮೌಲ್ಯ);
            m_lexp->m_def_ಮೌಲ್ಯ = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            ಯದಿ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().ತಳ್ಳು_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ತಳ್ಳು_State_Def_Arg(1);
                refp->nextarg("");
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().ತಳ್ಳು_back(refp->nextarg());
                string out;
                ಯದಿ (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.ತೆಗೆ(), refp);
                state_ತೆಗೆ();
                ಯದಿ (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    ಯದಿ (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ರಾಶಿ.empty(), "`` join stack empty, but in a ``");
                        ಧ್ರುವ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                        m_join_ರಾಶಿ.ತೆಗೆ();
                        out.ಸೇರಿಸು(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_ತೆಗೆ();
                    }
                    ಯದಿ (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    ಅನ್ಯಥಾ ಯದಿ (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.ತುದಿ().parenLevel();
                } ಅನ್ಯಥಾ {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.ತುದಿ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_ಮೌಲ್ಯ + out);
                    m_lexp->m_def_ಮೌಲ್ಯ = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                ಭಙ್ಗ;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_ಚಿಹ್ನೆ || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ತಳ್ಳು(ps_STRIFY);
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ ps_INCNAME: {
            ಯದಿ (tok == VP_STRING) {
                state_ತೆಗೆ();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ತಳ್ಳು_State_Inc_Filename();
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                ಭಙ್ಗ;
            } ಅನ್ಯಥಾ {
                state_ತೆಗೆ();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                ಗಚ್ಛ next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        ಪಕ್ಷ ps_ERRORNAME: {
            ಯದಿ (tok == VP_STRING) {
                ಯದಿ (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ ps_JOIN: {
            ಯದಿ (tok == VP_ಚಿಹ್ನೆ || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ರಾಶಿ.empty(), "`` join stack empty, but in a ``");
                ಧ್ರುವ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                m_join_ರಾಶಿ.ತೆಗೆ();
                UINFO(5, "`` LHS:" << lhs);
                ಧ್ರುವ string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ಧ್ರುವ string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ಧ್ರುವ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                m_join_ರಾಶಿ.ತೆಗೆ();
                unputString(lhs);
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ {
                // `define, etc, fall through and expand.  Pop back here.
                ಭಙ್ಗ;
            }
        }
        ಪಕ್ಷ ps_STRIFY: {
            ಯದಿ (tok == VP_STRIFY) {
                // Quote what's in the middle of the stringification
                // Note a `" MACRO_WITH(`") `" doesn't need to be handled (we don't need a stack)
                // That behavior isn't specified, and other simulators vary widely
                string out = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(out.begin(), out.end(), '\n', ' ');
                unputString("\""s + out + "\"");
                state_ತೆಗೆ();
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                ಭಙ್ಗ;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                ಗಚ್ಛ next_tok;
            } ಅನ್ಯಥಾ ಯದಿ (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                ಭಙ್ಗ;
            } ಅನ್ಯಥಾ {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                ಗಚ್ಛ next_tok;
            }
        }
        ಸ್ವಭಾವ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        ವಿಕಲ್ಪ (tok) {
        ಪಕ್ಷ VP_INCLUDE:
            ಯದಿ (!m_off) {
                state_ತಳ್ಳು(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_UNDEF: state_ತಳ್ಳು(ps_DEFNAME_UNDEF); ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ತಳ್ಳು(ps_DEFNAME_DEFINE);
            ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_IFDEF: state_ತಳ್ಳು(ps_DEFNAME_IFDEF); ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_IFNDEF: state_ತಳ್ಳು(ps_DEFNAME_IFNDEF); ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_ELSIF: state_ತಳ್ಳು(ps_DEFNAME_ELSIF); ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_ELSE:
            ಯದಿ (m_ifdef_ರಾಶಿ.empty()) {
                fileline()->v3error("`else with no matching `if");
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ VPreIfEntry lastIf = m_ifdef_ರಾಶಿ.ತುದಿ();
                m_ifdef_ರಾಶಿ.ತೆಗೆ();
                ಧ್ರುವ ತಾರ್ಕಿಕ enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ರಾಶಿ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                ಯದಿ (!lastIf.on()) parsingOn();
                ಯದಿ (!enable) parsingOff();
            }
            ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_ENDIF:
            UINFO(4, "Endif ");
            ಯದಿ (m_ifdef_ರಾಶಿ.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ VPreIfEntry lastIf = m_ifdef_ರಾಶಿ.ತುದಿ();
                m_ifdef_ರಾಶಿ.ತೆಗೆ();
                ಯದಿ (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            ಗಚ್ಛ next_tok;

        ಪಕ್ಷ VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ಧ್ರುವ string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            ಯದಿ (m_defPutJoin) {
                m_defPutJoin = ಅಸತ್ಯ;
                unputString("``");
            }
            ಯದಿ (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                ಗಚ್ಛ next_tok;
            }
            // Substitute
            ಯದಿ (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                ಯದಿ (m_off) {
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    ಗಚ್ಛ next_tok;
                }
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ string params = defParams(name);
                ಯದಿ (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    ಯದಿ (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    ಯದಿ (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        ಯದಿ (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ರಾಶಿ.empty(), "`` join stack empty, but in a ``");
                            ಧ್ರುವ string lhs = m_join_ರಾಶಿ.ತುದಿ();
                            m_join_ರಾಶಿ.ತೆಗೆ();
                            out.ಸೇರಿಸು(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_ತೆಗೆ();
                        }
                        ಯದಿ (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        ಅನ್ಯಥಾ ಯದಿ (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } ಅನ್ಯಥಾ {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ಧ್ರುವ refp = &(m_defRefs.ತುದಿ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_ಮೌಲ್ಯ + out);
                        m_lexp->m_def_ಮೌಲ್ಯ = "";
                    }
                    ಗಚ್ಛ next_tok;
                } ಅನ್ಯಥಾ {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    ಯದಿ (!m_defRefs.empty()) m_defRefs.ತುದಿ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ತಳ್ಳು(ps_DEFPAREN);
                    m_lexp->ತಳ್ಳು_State_Def_Arg(0);
                    ಗಚ್ಛ next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            ಗಚ್ಛ next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        ಪಕ್ಷ VP_ERROR: {
            state_ತಳ್ಳು(ps_ERRORNAME);
            ಗಚ್ಛ next_tok;
        }
        ಪಕ್ಷ VP_EOF:
            ಯದಿ (!m_ifdef_ರಾಶಿ.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            ಪ್ರತ್ಯಾಗಮ tok;
        ಪಕ್ಷ VP_UNDEFINEALL:
            ಯದಿ (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ತಳ್ಳು(ps_STRIFY);
            ಗಚ್ಛ next_tok;
        ಪಕ್ಷ VP_ಚಿಹ್ನೆ:
        ಪಕ್ಷ VP_STRING:
        ಪಕ್ಷ VP_TEXT: {
            m_defDepth = 0;
            ಯದಿ (!m_off) {
                ಪ್ರತ್ಯಾಗಮ tok;
            } ಅನ್ಯಥಾ {
                ಗಚ್ಛ next_tok;
            }
        }
        ಪಕ್ಷ VP_WHITE:  // Handled at top of loop
        ಪಕ್ಷ VP_COMMENT:  // Handled at top of loop
        ಪಕ್ಷ VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        ಪಕ್ಷ VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        ಸ್ವಭಾವ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            ಭಙ್ಗ;  // LCOV_EXCL_LINE
        }
        ಪ್ರತ್ಯಾಗಮ tok;
    }
}

ಪೂರ್ಣಾಙ್ಕ V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    ಯದಿ (!m_finAhead) {
        m_finAhead = ಸತ್ಯ;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ tok = m_finToken;
    buf = m_finBuf;
    ಯದಿ (ಅಸತ್ಯ && debug() >= 5) {
        ಧ್ರುವ string bufcln = V3PreLex::cleanDbgStrg(buf);
        ಧ್ರುವ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ಧ್ರುವ ವರ್ಣ* bufp = buf.c_str();
    ಯಾವತ್ (*bufp == '\n') bufp++;
    ಯದಿ ((tok == VP_TEXT || tok == VP_ಸಾಲು) && VString::startsWith(bufp, "`line ")) {
        ಪೂರ್ಣಾಙ್ಕ enter;
        m_finFilelinep->ಸಾಲು_Directive(bufp, enter /*ref*/);
    } ಅನ್ಯಥಾ {
        ಯದಿ (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->ಸಾಲು_Directives()) {
            ಯದಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                ಯದಿ (debug() >= 5) {
                    ಧ್ರುವ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                ಯದಿ (outBehind > 0
                    && (outBehind <= static_cast<ಪೂರ್ಣಾಙ್ಕ>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    ಯದಿ (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        ಪ್ರತ್ಯಾಗಮ VP_TEXT;
                    }
                } ಅನ್ಯಥಾ {
                    // Need to backup, use `line
                    buf = m_finFilelinep->ಸಾಲು_Directive_Strg(0);
                    ಪ್ರತ್ಯಾಗಮ VP_ಸಾಲು;
                }
            }
        }
        // Track newlines in prep for next token
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ& c : buf) {
            ಯದಿ (c == '\n') {
                m_finAtBol = ಸತ್ಯ;
                m_finFilelinep->linenoInc();
            } ಅನ್ಯಥಾ {
                m_finAtBol = ಅಸತ್ಯ;
            }
        }
    }
    m_finAhead = ಅಸತ್ಯ;  // Consumed the token
    ಪ್ರತ್ಯಾಗಮ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    ಯದಿ (isEof()) ಪ್ರತ್ಯಾಗಮ "";
    ಧ್ರುವ ವರ್ಣ* rtnp;
    ತಾರ್ಕಿಕ gotEof = ಅಸತ್ಯ;
    ಯಾವತ್ (nullptr == (rtnp = std::strchr(m_ಸಾಲು_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ tok = getFinalToken(m_tokenBuf /*ref*/);
        ಯದಿ (debug() >= 5) {
            ಧ್ರುವ string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ಧ್ರುವ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        ಯದಿ (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            ಯದಿ (m_ಸಾಲು_Chars != "" && m_ಸಾಲು_Chars[m_ಸಾಲು_Chars.length() - 1] != '\n') {
                m_ಸಾಲು_Chars.append("\n");
            }
            gotEof = ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            m_ಸಾಲು_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ len = rtnp - m_ಸಾಲು_Chars.c_str() + 1;
    string the_ಸಾಲು(m_ಸಾಲು_Chars, 0, len);
    m_ಸಾಲು_Chars = m_ಸಾಲು_Chars.erase(0, len);  // Remove returned characters
    ಯದಿ (debug() >= 4) {
        ಧ್ರುವ string lncln = V3PreLex::cleanDbgStrg(the_ಸಾಲು);
        ಧ್ರುವ string flcol = m_lexp->m_tokFilelinep->ascii_ಸಾಲು_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ಪ್ರತ್ಯಾಗಮ the_ಸಾಲು;
}
