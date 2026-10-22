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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3PreProc.h"

#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3LanguageWords.h"
#ଅନ୍ତର୍ଭାଵ "V3PreExpr.h"
#ଅନ୍ତର୍ଭାଵ "V3PreLex.h"
#ଅନ୍ତର୍ଭାଵ "V3PreShell.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cstdlib>
#ଅନ୍ତର୍ଭାଵ <fstream>
#ଅନ୍ତର୍ଭାଵ <stack>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#ଅନ୍ତର୍ଭାଵ "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

ଶ୍ରେଣୀ VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_ଧାଡ଼ି* ଅଚଳ m_fileline;  // Where it was declared
    ଅଚଳ string m_ମୂଲ୍ୟ;  // Value of define
    ଅଚଳ string m_params;  // Parameters
    ଅଚଳ ତାର୍କିକ m_cmdline;  // Set on command line, don't `undefineall
ସର୍ବସାଧାରଣ:
    VDefine(File_ଧାଡ଼ି* fl, ଅଚଳ string& ମୂଲ୍ୟ, ଅଚଳ string& params, ତାର୍କିକ cmdline)
        : m_fileline{fl}
        , m_ମୂଲ୍ୟ{ମୂଲ୍ୟ}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ m_fileline; }
    string ମୂଲ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ m_ମୂଲ୍ୟ; }
    string params() ଅଚଳ { ଫେରନ୍ତୁ m_params; }
    ତାର୍କିକ cmdline() ଅଚଳ { ଫେରନ୍ତୁ m_cmdline; }
};

//*************************************************************************

ଶ୍ରେଣୀ VDefineRef final {
    // One for each pending define substitution
    ଅଚଳ string m_name;  // Define last name being defined
    ଅଚଳ string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
ସର୍ବସାଧାରଣ:
    string name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    string params() ଅଚଳ { ଫେରନ୍ତୁ m_params; }
    string nextarg() ଅଚଳ { ଫେରନ୍ତୁ m_nextarg; }
    ଶୂନ୍ୟ nextarg(ଅଚଳ string& ମୂଲ୍ୟ) { m_nextarg = ମୂଲ୍ୟ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା parenLevel() ଅଚଳ { ଫେରନ୍ତୁ m_parenLevel; }
    ଶୂନ୍ୟ parenLevel(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) { m_parenLevel = ମୂଲ୍ୟ; }
    std::vector<string>& args() { ଫେରନ୍ତୁ m_args; }
    VDefineRef(ଅଚଳ string& name, ଅଚଳ string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//*************************************************************************
/// Data for parsing on/off

ଶ୍ରେଣୀ VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ଅଚଳ ତାର୍କିକ m_on;  // Current parse for this ifdef level is "on"
    ଅଚଳ ତାର୍କିକ m_everOn;  // Some if term in elsif tree has been on
ସର୍ବସାଧାରଣ:
    ତାର୍କିକ on() ଅଚଳ { ଫେରନ୍ତୁ m_on; }
    ତାର୍କିକ everOn() ଅଚଳ { ଫେରନ୍ତୁ m_everOn; }
    VPreIfEntry(ତାର୍କିକ on, ତାର୍କିକ everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

ଶ୍ରେଣୀ V3PreProcImp final : ସର୍ବସାଧାରଣ V3PreProc {
ସର୍ବସାଧାରଣ:
    // TYPES
    using DefinesMap = std::map<ଅଚଳ std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ଅଚଳ V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lastLineno = 0;  // Last line number (stall detection)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_tokens_On_ଧାଡ଼ି = 0;  // Number of tokens on line (stall detection)

    ଗଣନା ProcState : uint8_t {
        ps_ଶୀର୍ଷ,
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
        ps_ପଦସମଷ୍ଟି_IFDEF,
        ps_ପଦସମଷ୍ଟି_IFNDEF,
        ps_ପଦସମଷ୍ଟି_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* procStateName(ProcState s) {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ଫେରନ୍ତୁ states[s];
    }

    std::ଗଦା<ProcState> m_states;  ///< Current state of parser
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    ତାର୍କିକ m_incError = ମିଥ୍ୟା;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_ଧାଡ଼ି_Cmt;  ///< Line comment(s) to be returned
    ତାର୍କିକ m_ଧାଡ଼ି_Cmt_Nl = ମିଥ୍ୟା;  ///< Newline needed before inserting lineCmt
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଧାଡ଼ି_Add = 0;  ///< Empty lines to return to maintain line count
    ତାର୍କିକ m_rawAtBol = ସତ୍ୟ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    ତାର୍କିକ m_finAhead = ମିଥ୍ୟା;  ///< Have read a token ahead
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    ତାର୍କିକ m_finAtBol = ସତ୍ୟ;  ///< Last getFinalToken left us at beginning of line
    File_ଧାଡ଼ି* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ଗଦା<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ଗଦା<VPreIfEntry> m_ifdef_ଗଦା;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    ତାର୍କିକ m_defPutJoin = ମିଥ୍ୟା;  ///< Insert `` after substitution

    // For `` join
    std::ଗଦା<string> m_join_ଗଦା;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_ପଦସମଷ୍ଟି m_ପଦସମଷ୍ଟି_Parser;  ///< Parser for () expression
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ପଦସମଷ୍ଟି_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_ଧାଡ଼ି_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    ଶୂନ୍ୟ v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* tokenName(ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok);
    ଶୂନ୍ୟ debugToken(ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok, ଅଚଳ ଅକ୍ଷର* cmtp);
    ଶୂନ୍ୟ parse_ଶୀର୍ଷ();
    ଶୂନ୍ୟ parseUndef();

ବ୍ୟକ୍ତିଗତ:
    // Internal methods
    ଶୂନ୍ୟ endOfOneFile();
    string defineSubst(VDefineRef* refp);

    ତାର୍କିକ defExists(ଅଚଳ string& name);
    ତାର୍କିକ defCmdline(ଅଚଳ string& name);
    string def_ମୂଲ୍ୟ(ଅଚଳ string& name);
    string defParams(ଅଚଳ string& name);
    File_ଧାଡ଼ି* defFileline(ଅଚଳ string& name);

    string commentCleanup(ଅଚଳ string& text);
    ତାର୍କିକ commentTokenMatch(string& cmdr, ଅଚଳ ଅକ୍ଷର* strg);
    ସ୍ଥିର string trimWhitespace(ଅଚଳ string& strg, ତାର୍କିକ trailing);
    ଶୂନ୍ୟ unputString(ଅଚଳ string& strg);
    ଶୂନ୍ୟ unputDefrefString(ଅଚଳ string& strg);

    ଶୂନ୍ୟ parsingOn() {
        m_off--;
        ଯଦି (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    ଶୂନ୍ୟ parsingOff() { m_off++; }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା getRawToken();
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା getStateToken();
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା getFinalToken(string& buf);

    ProcState state() ଅଚଳ { ଫେରନ୍ତୁ m_states.ଶୀର୍ଷ(); }
    ତାର୍କିକ stateIsDefname() ଅଚଳ {
        ଫେରନ୍ତୁ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    ଶୂନ୍ୟ state_ଠେଲ(ProcState state) { m_states.ଠେଲ(state); }
    ଶୂନ୍ୟ state_କାଢ଼() {
        m_states.କାଢ଼();
        ଯଦି (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.ଠେଲ(ps_ଶୀର୍ଷ);  // LCOV_EXCL_LINE
        }
    }
    ଶୂନ୍ୟ stateChange(ProcState state) {
        state_କାଢ଼();
        state_ଠେଲ(state);
    }

ସର୍ବସାଧାରଣ:
    // METHODS, called from upper level shell
    ଶୂନ୍ୟ openFile(File_ଧାଡ଼ି* fl, VInFilter* filterp, ଅଚଳ string& filename) override;
    ତାର୍କିକ isEof() ଅଚଳ override { ଫେରନ୍ତୁ m_lexp->curStreamp()->m_eof; }
    ଶୂନ୍ୟ forceEof() { m_lexp->curStreamp()->m_eof = ସତ୍ୟ; }
    string getline() override;
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Unreadback(ଅଚଳ string& text) override { m_ଧାଡ଼ି_Cmt += text; }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Unreadback_At_Bol(ଅଚଳ string& text);
    ଶୂନ୍ୟ add_ଧାଡ଼ି_Comment(ପୂର୍ଣ୍ଣସଂଖ୍ୟା enterExit);
    ଶୂନ୍ୟ dumpDefines(std::ostream& os) override;
    ଶୂନ୍ୟ candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    ଶୂନ୍ୟ comment(ଅଚଳ string& text) override;  // Comment detected (if keepComments==2)
    ଶୂନ୍ୟ include(ଅଚଳ string& filename) override;  // Request a include file be processed
    ଶୂନ୍ୟ undef(ଅଚଳ string& name) override;
    ଆଭାସୀ ଶୂନ୍ୟ undefineall();
    ଶୂନ୍ୟ define(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& ମୂଲ୍ୟ, ଅଚଳ string& params,
                ତାର୍କିକ cmdline) override;
    string removeDefines(ଅଚଳ string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.ଠେଲ(ps_ଶୀର୍ଷ); }
    // cppcheck-suppress duplInheritedMember
    ଶୂନ୍ୟ configure(File_ଧାଡ଼ି* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = ଏହି;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = ନୂତନ File_ଧାଡ଼ି{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = ନୂତନ V3PreLex{ଏହି, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        ଯଦି (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_ଧାଡ଼ି* fl) {
    V3PreProcImp* ଅଚଳ preprocp = ନୂତନ V3PreProcImp;
    preprocp->configure(fl);
    ଫେରନ୍ତୁ preprocp;
}

ଶୂନ୍ୟ V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_ପଦସମଷ୍ଟି::selfTest(); }

//*************************************************************************
// Defines

ଶୂନ୍ୟ V3PreProcImp::undef(ଅଚଳ string& name) {
    ଯଦି (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ଭର୍ତ୍ତି_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
ଶୂନ୍ୟ V3PreProcImp::undefineall() {
    ଯଦି (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ଭର୍ତ୍ତି_Unreadback("`undefineall\n");
    ପାଇଁ (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        ଯଦି (!it->second.cmdline()) m_defines.erase(it);
    }
}
ତାର୍କିକ V3PreProcImp::defExists(ଅଚଳ string& name) {
    ଅଚଳ auto iter = m_defines.find(name);
    ଫେରନ୍ତୁ (iter != m_defines.end());
}
ତାର୍କିକ V3PreProcImp::defCmdline(ଅଚଳ string& name) {
    ଅଚଳ auto iter = m_defines.find(name);
    ଯଦି (iter == m_defines.end()) { ଫେରନ୍ତୁ ମିଥ୍ୟା; }
    ଫେରନ୍ତୁ iter->second.cmdline();
}
string V3PreProcImp::def_ମୂଲ୍ୟ(ଅଚଳ string& name) {
    ଅଚଳ auto iter = m_defines.find(name);
    ଯଦି (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ଫେରନ୍ତୁ "";
    }
    ଫେରନ୍ତୁ iter->second.ମୂଲ୍ୟ();
}
string V3PreProcImp::defParams(ଅଚଳ string& name) {
    ଅଚଳ auto iter = m_defines.find(name);
    ଯଦି (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ଫେରନ୍ତୁ "";
    }
    ଫେରନ୍ତୁ iter->second.params();
}
File_ଧାଡ଼ି* V3PreProcImp::defFileline(ଅଚଳ string& name) {
    ଅଚଳ auto iter = m_defines.find(name);
    ଯଦି (iter == m_defines.end()) ଫେରନ୍ତୁ nullptr;
    ଫେରନ୍ତୁ iter->second.fileline();
}
ଶୂନ୍ୟ V3PreProcImp::define(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& ମୂଲ୍ୟ,
                          ଅଚଳ string& params, ତାର୍କିକ cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << ମୂଲ୍ୟ << "' params '" << params << "'");
    ଯଦି (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(ମୂଲ୍ୟ.size());
        ପାଇଁ (string::size_type pos = 0; pos < ମୂଲ୍ୟ.size(); ++pos) {
            ଯଦି (ମୂଲ୍ୟ[pos] == '\n' && (pos == 0 || ମୂଲ୍ୟ[pos - 1] != '\\')) out += '\\';
            out += ମୂଲ୍ୟ[pos];
        }
        ଭର୍ତ୍ତି_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    ଯଦି (!V3_Language_ଶବ୍ଦଗୁଡ଼ିକ::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } ଅନ୍ୟଥା {
        ଯଦି (defExists(name)) {
            ଯଦି (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << ମୂଲ୍ୟ
                               << "' to existing command line define value: '" << def_ମୂଲ୍ୟ(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_ମୂଲ୍ୟ(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                ଫେରନ୍ତୁ;
            } ଅନ୍ୟଥା {
                ଯଦି (!(def_ମୂଲ୍ୟ(name) == ମୂଲ୍ୟ
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << ମୂଲ୍ୟ
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_ମୂଲ୍ୟ(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, ମୂଲ୍ୟ, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(ଅଚଳ string& text) {
    string val;
    string rtnsym = text;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        ଯଦି (xsym[0] == '`') xsym.erase(0, 1);
        ଯଦି (defExists(xsym)) {
            val = def_ମୂଲ୍ୟ(xsym);
            ଯଦି (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } ଅନ୍ୟଥା {
                ଭାଙ୍ଗ;
            }
        } ଅନ୍ୟଥା {
            ଭାଙ୍ଗ;
        }
    }
    ଫେରନ୍ତୁ rtnsym;  // NA
}

//**********************************************************************
// Comments

ଶୂନ୍ୟ V3PreProcImp::include(ଅଚଳ string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ଅଚଳ string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    ଯେତେବେଳେ ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    ଯେତେବେଳେ ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    ଯେତେବେଳେ ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    ଯେତେବେଳେ ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    ଯେତେବେଳେ ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    ଯେତେବେଳେ ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    ଯେତେବେଳେ (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    ଫେରନ୍ତୁ cmd;
}

ତାର୍କିକ V3PreProcImp::commentTokenMatch(string& cmdr, ଅଚଳ ଅକ୍ଷର* strg) {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା len = std::strlen(strg);
    ଯଦି (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        ଯଦି (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ଫେରନ୍ତୁ ସତ୍ୟ;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
}

ଶୂନ୍ୟ V3PreProcImp::comment(ଅଚଳ string& text) {
    // Comment detected.  Only keep relevant data.
    ତାର୍କିକ printed = ମିଥ୍ୟା;
    ଯଦି (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ଭର୍ତ୍ତି_Unreadback(text);
        printed = ସତ୍ୟ;
    }

    ଅଚଳ ଅକ୍ଷର* cp = text.c_str();
    ଯଦି (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ;
    }

    ଯେତେବେଳେ (std::isspace(*cp)) ++cp;

    ତାର୍କିକ synth = ମିଥ୍ୟା;
    ତାର୍କିକ vlcomment = ମିଥ୍ୟା;
    ଯଦି ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        ଯଦି (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ଫେରନ୍ତୁ;
        }
        vlcomment = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        ଯଦି (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ଫେରନ୍ତୁ;
        }
        synth = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = ସତ୍ୟ;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ;
    }

    ଯେତେବେଳେ (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    ଯଦି (synth) {
        ଯଦି (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            ଯଦି (commentTokenMatch(cmd /*ref*/, "full_case")) {
                ଯଦି (!printed) ଭର୍ତ୍ତି_Unreadback("/*verilator full_case*/");
            }
            ଯଦି (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                ଯଦି (!printed) ଭର୍ତ୍ତି_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } ଅନ୍ୟଥା ଯଦି (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        ଯଦି (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            ଯେତେବେଳେ (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            ଯେତେବେଳେ (std::isspace(arg[0])) arg = arg.substr(1);
            ଯଦି (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            ଯଦି (!printed) ଭର୍ତ୍ତି_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } ଅନ୍ୟଥା ଯଦି (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            ଯେତେବେଳେ (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            ଯଦି (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ଫେରନ୍ତୁ;
            }
            ଯଦି (!printed) ଭର୍ତ୍ତି_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } ଅନ୍ୟଥା {
            ଯଦି (!printed) ଭର୍ତ୍ତି_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_ଧାଡ଼ି* V3PreProc::fileline() {
    ଅଚଳ V3PreProcImp* idatap = static_cast<V3PreProcImp*>(ଏହି);
    ଫେରନ୍ତୁ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ଅଚଳ ଅକ୍ଷର* V3PreProcImp::tokenName(ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok) {
    ବାଛ (tok) {
    ସ୍ଥିତି VP_BACKQUOTE: ଫେରନ୍ତୁ ("BACKQUOTE");
    ସ୍ଥିତି VP_COMMENT: ଫେରନ୍ତୁ ("COMMENT");
    ସ୍ଥିତି VP_DEFARG: ଫେରନ୍ତୁ ("DEFARG");
    ସ୍ଥିତି VP_DEFFORM: ଫେରନ୍ତୁ ("DEFFORM");
    ସ୍ଥିତି VP_DEFINE: ଫେରନ୍ତୁ ("DEFINE");
    ସ୍ଥିତି VP_DEFREF: ଫେରନ୍ତୁ ("DEFREF");
    ସ୍ଥିତି VP_DEFREF_JOIN: ଫେରନ୍ତୁ ("DEFREF_JOIN");
    ସ୍ଥିତି VP_DEFVALUE: ଫେରନ୍ତୁ ("DEFVALUE");
    ସ୍ଥିତି VP_ELSE: ଫେରନ୍ତୁ ("ELSE");
    ସ୍ଥିତି VP_ELSIF: ଫେରନ୍ତୁ ("ELSIF");
    ସ୍ଥିତି VP_ENDIF: ଫେରନ୍ତୁ ("ENDIF");
    ସ୍ଥିତି VP_EOF: ଫେରନ୍ତୁ ("EOF");
    ସ୍ଥିତି VP_EOF_ERROR: ଫେରନ୍ତୁ ("EOF_ERROR");
    ସ୍ଥିତି VP_ERROR: ଫେରନ୍ତୁ ("ERROR");
    ସ୍ଥିତି VP_IFDEF: ଫେରନ୍ତୁ ("IFDEF");
    ସ୍ଥିତି VP_IFNDEF: ଫେରନ୍ତୁ ("IFNDEF");
    ସ୍ଥିତି VP_JOIN: ଫେରନ୍ତୁ ("JOIN");
    ସ୍ଥିତି VP_INCLUDE: ଫେରନ୍ତୁ ("INCLUDE");
    ସ୍ଥିତି VP_ଧାଡ଼ି: ଫେରନ୍ତୁ ("LINE");
    ସ୍ଥିତି VP_STRIFY: ଫେରନ୍ତୁ ("STRIFY");
    ସ୍ଥିତି VP_STRING: ଫେରନ୍ତୁ ("STRING");
    ସ୍ଥିତି VP_ପ୍ରତୀକ: ଫେରନ୍ତୁ ("SYMBOL");
    ସ୍ଥିତି VP_ପ୍ରତୀକ_JOIN: ଫେରନ୍ତୁ ("SYMBOL_JOIN");
    ସ୍ଥିତି VP_TEXT: ଫେରନ୍ତୁ ("TEXT");
    ସ୍ଥିତି VP_UNDEF: ଫେରନ୍ତୁ ("UNDEF");
    ସ୍ଥିତି VP_UNDEFINEALL: ଫେରନ୍ତୁ ("UNDEFINEALL");
    ସ୍ଥିତି VP_WHITE: ଫେରନ୍ତୁ ("WHITE");
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ ("?");
    }
}

ଶୂନ୍ୟ V3PreProcImp::unputString(ଅଚଳ string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    ଯଦି (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

ଶୂନ୍ୟ V3PreProcImp::unputDefrefString(ଅଚଳ string& strg) {
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା multiline = std::ଗଣନା_2(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ଅଚଳ string& strg, ତାର୍କିକ trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    ଯେତେବେଳେ (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    ଯଦି (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    ଯଦି (trailing) {
        string::size_type trailspace = 0;
        ଯେତେବେଳେ (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        ଯଦି (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        ଯଦି (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    ଫେରନ୍ତୁ out;
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
    ପାଇଁ (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ଅଚଳ string ମୂଲ୍ୟ = def_ମୂଲ୍ୟ(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(ମୂଲ୍ୟ) << "'");

    std::map<ଅଚଳ string, string> arg_ମୂଲ୍ୟ_By_Name;
    {  // Parse argument list into map
        unsigned ଅଙ୍କ_Args = 0;
        string argName;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        ତାର୍କିକ quote = ମିଥ୍ୟା;
        ତାର୍କିକ haveDefault = ମିଥ୍ୟା;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ଅଚଳ string params = refp->params();  // Must keep str in scope to get pointer
        ଅଚଳ ଅକ୍ଷର* cp = params.c_str();
        ଯଦି (*cp == '(') cp++;
        ପାଇଁ (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            ଯଦି (!quote && paren == 1) {
                ଯଦି (*cp == ')' || *cp == ',') {
                    string ମୂଲ୍ୟ_Def;
                    ଯଦି (haveDefault) {
                        ମୂଲ୍ୟ_Def = token;
                    } ଅନ୍ୟଥା {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, ସତ୍ୟ);
                    UINFO(4, "    Got Arg=" << ଅଙ୍କ_Args << "  argName='" << argName
                                            << "'  default='" << ମୂଲ୍ୟ_Def << "'");
                    // Parse it
                    ଯଦି (argName != "") {
                        ଯଦି (refp->args().size() > ଅଙ୍କ_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ଅଚଳ string arg = trimWhitespace(refp->args()[ଅଙ୍କ_Args], ସତ୍ୟ);
                            ଯଦି (arg != "") ମୂଲ୍ୟ_Def = arg;
                        } ଅନ୍ୟଥା ଯଦି (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ଫେରନ୍ତୁ " `" + refp->name() + " ";
                        }
                        ଅଙ୍କ_Args++;
                    }
                    arg_ମୂଲ୍ୟ_By_Name[argName] = ମୂଲ୍ୟ_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = ମିଥ୍ୟା;
                    ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (*cp == '=') {
                    haveDefault = ସତ୍ୟ;
                    argName = token;
                    token = "";
                    ଜାରି;
                }
            }
            ଯଦି (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                ଜାରି;
            }
            ଯଦି (!quote) {
                ଯଦି (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } ଅନ୍ୟଥା ଯଦି (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            ଯଦି (*cp == '"') quote = !quote;
            ଯଦି (*cp) token += *cp;
        }
        ଯଦି (refp->args().size() > ଅଙ୍କ_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && ଅଙ୍କ_Args == 0
                 && trimWhitespace(refp->args()[0], ମିଥ୍ୟା) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ଫେରନ୍ତୁ " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        ତାର୍କିକ quote = ମିଥ୍ୟା;
        ତାର୍କିକ triquote = ମିଥ୍ୟା;
        ତାର୍କିକ backslashesc = ମିଥ୍ୟା;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = ମୂଲ୍ୟ.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            ଯଦି (!quote && !triquote && *cp == '\\') {
                backslashesc = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (std::isspace(*cp)) {
                backslashesc = ମିଥ୍ୟା;
            }
            ଯଦି (!quote && !triquote) {
                ଯଦି (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    ଜାରି;
                }
                ଯଦି (argName != "") {
                    // Found a possible variable substitution
                    ଅଚଳ auto iter = arg_ମୂଲ୍ୟ_By_Name.find(argName);
                    ଯଦି (iter != arg_ମୂଲ୍ୟ_By_Name.end()) {
                        // Substitute
                        ଅଚଳ string subst = iter->second;
                        ଯଦି (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            ଯଦି (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } ଅନ୍ୟଥା {
                            out += subst;
                        }
                    } ଅନ୍ୟଥା {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                ଯଦି (cp[0] == '`' && cp[1] == '`') {
                    ଯଦି (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } ଅନ୍ୟଥା {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    ଜାରି;
                } ଅନ୍ୟଥା ଯଦି (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    ଜାରି;
                }
            }
            ଯଦି (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                ଜାରି;
            } ଅନ୍ୟଥା ଯଦି (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                ଜାରି;
            }
            ଯଦି (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                ଜାରି;
            }
            ଯଦି (*cp == '"') quote = !quote;
            ଯଦି (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    ଫେରନ୍ତୁ out;
}

//**********************************************************************
// Parser routines

ଶୂନ୍ୟ V3PreProcImp::openFile(File_ଧାଡ଼ି*, VInFilter* filterp, ଅଚଳ string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    ଯଦି (m_incError) ଫେରନ୍ତୁ;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ଅଚଳ ତାର୍କିକ ok = filterp->readWholefile(filename, wholefile /*ref*/);
    ଯଦି (!ok) {
        fileline()->v3error("File not found: " + filename);
        ଫେରନ୍ତୁ;
    }

    ଯଦି (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        ଯଦି (m_lexp->m_streamp_ଗଦା.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = ସତ୍ୟ;
            ଫେରନ୍ତୁ;
        }
        // There's already a file active.  Push it to work on the new one.
        add_ଧାଡ଼ି_Comment(0);
    }

    // Save file contents for future error reporting
    File_ଧାଡ଼ି* ଅଚଳ flsp = ନୂତନ File_ଧାଡ଼ି{filename};
    flsp->lineno(1);
    flsp->newContent();
    ପାଇଁ (ଅଚଳ string& i : wholefile) flsp->contentp()->ଠେଲ_Text(i);

    // Save contents for V3Control --contents
    ଯଦି (filename != V3Options::getStdPackagePath()) {
        ତାର୍କିକ containsVlt = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            ଯଦି (i.find("`verilator_config") != string::npos) {
                containsVlt = ସତ୍ୟ;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (!containsVlt) {
            ପାଇଁ (ଅଚଳ string& i : wholefile) V3Control::contents_ଠେଲ_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_ଧାଡ଼ି_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା eof_newline = 0;  // Number of characters following last newline
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା eof_lineno = 1;
    ପାଇଁ (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        ତାର୍କିକ strip = ମିଥ୍ୟା;
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ sp = it->data();
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = sp; cp < ep; cp++) {
            ଯଦି (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = ସତ୍ୟ;
            } ଅନ୍ୟଥା ଯଦି (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } ଅନ୍ୟଥା {
                ++eof_newline;
            }
        }
        ଯଦି (strip) {
            string out;
            out.reserve(it->length());
            ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = sp; cp < ep; cp++) {
                ଯଦି (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    ଯଦି (eof_newline) {
        File_ଧାଡ଼ି* ଅଚଳ fl = ନୂତନ File_ଧାଡ଼ି{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

ଶୂନ୍ୟ V3PreProcImp::ଭର୍ତ୍ତି_Unreadback_At_Bol(ଅଚଳ string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    ଯଦି (m_ଧାଡ଼ି_Cmt == "") {
        m_ଧାଡ଼ି_Cmt_Nl = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (m_ଧାଡ଼ି_Cmt[m_ଧାଡ଼ି_Cmt.length() - 1] != '\n') {
        ଭର୍ତ୍ତି_Unreadback("\n");
    }
    ଭର୍ତ୍ତି_Unreadback(text);
}

ଶୂନ୍ୟ V3PreProcImp::add_ଧାଡ଼ି_Comment(ପୂର୍ଣ୍ଣସଂଖ୍ୟା enterExit) {
    ଯଦି (ଧାଡ଼ି_Directives()) {
        ଭର୍ତ୍ତି_Unreadback_At_Bol(m_lexp->curFilelinep()->ଧାଡ଼ି_Directive_Strg(enterExit));
    }
}

ଶୂନ୍ୟ V3PreProcImp::dumpDefines(std::ostream& os) {
    ପାଇଁ (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        ଯଦି (!it->second.params().empty()) os << it->second.params();
        ଯଦି (!it->second.ମୂଲ୍ୟ().empty()) os << " " << it->second.ମୂଲ୍ୟ();
        os << '\n';
    }
}

ଶୂନ୍ୟ V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    ପାଇଁ (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->ଠେଲ_Candidate("`"s + it->first);
    }
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    ଯେତେବେଳେ (ସତ୍ୟ) {
    next_tok:
        ଯଦି (m_ଧାଡ଼ି_Add) {
            m_ଧାଡ଼ି_Add--;
            m_rawAtBol = ସତ୍ୟ;
            yyourtext("\n", 1);
            ଯଦି (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ଫେରନ୍ତୁ VP_WHITE;
        }
        ଯଦି (m_ଧାଡ଼ି_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ସ୍ଥିର string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_ଧାଡ଼ି_Cmt;
            ଯଦି (m_ଧାଡ଼ି_Cmt_Nl) {
                ଯଦି (!m_rawAtBol) s_rtncmt.ଭର୍ତ୍ତି(0, "\n");
                m_ଧାଡ଼ି_Cmt_Nl = ମିଥ୍ୟା;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_ଧାଡ଼ି_Cmt = "";
            ଯଦି (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            ଯଦି (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_ମୂଲ୍ୟ(yyourtext(), yyourleng());
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                ଯଦି (debug() >= 5) debugToken(VP_TEXT, "LCM");
                ଫେରନ୍ତୁ VP_TEXT;
            }
        }
        ଯଦି (isEof()) ଫେରନ୍ତୁ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok = m_lexp->lex();
        ଯଦି (debug() >= 5) debugToken(tok, "RAW");

        ଯଦି (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_ଧାଡ଼ି = 0;
        } ଅନ୍ୟଥା ଯଦି (++m_tokens_On_ଧାଡ଼ି > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        ଯଦି (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            ଯଦି (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            ଯାଅ next_tok;
        }

        ଯଦି (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ଫେରନ୍ତୁ tok;
    }
}

ଶୂନ୍ୟ V3PreProcImp::debugToken(ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok, ଅଚଳ ଅକ୍ଷର* cmtp) {
    ଅଚଳ ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    ଯଦି (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        ଯେତେବେଳେ ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        ଯେତେବେଳେ ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ଅଚଳ string flcol = m_lexp->m_tokFilelinep->ascii_ଧାଡ଼ି_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        ଯଦି (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    ଯେତେବେଳେ (ସତ୍ୟ) {
    next_tok:
        ଯଦି (isEof()) ଫେରନ୍ତୁ VP_EOF;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        ଯଦି (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ଫେରନ୍ତୁ tok;
        ଯଦି (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        ଯଦି (tok == VP_COMMENT) {
            ଯଦି (!m_off) {
                ଯଦି (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ଯଦି (m_ଧାଡ଼ି_Cmt.empty()) ଭର୍ତ୍ତି_Unreadback(" ");
                } ଅନ୍ୟଥା ଯଦି (m_lexp->m_keepComments) {
                    ଫେରନ୍ତୁ tok;
                } ଅନ୍ୟଥା {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ଭର୍ତ୍ତି_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            ପାଇଁ (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                ଯଦି (yyourtext()[len] == '\n') m_ଧାଡ଼ି_Add++;
            }
            ଯାଅ next_tok;
        }
        ଯଦି (tok == VP_ଧାଡ଼ି) {
            add_ଧାଡ଼ି_Comment(m_lexp->m_enterExit);
            ଯାଅ next_tok;
        }

        ଯଦି (tok == VP_DEFREF_JOIN) {
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
            ଅଚଳ std::string name(yyourtext() + 1, yyourleng() - 1);
            ଯଦି (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = ସତ୍ୟ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } ଅନ୍ୟଥା {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        ଯଦି (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            ଯଦି (tok == VP_JOIN) ଯାଅ next_tok;
            ଯଦି (tok == VP_ପ୍ରତୀକ_JOIN) tok = VP_ପ୍ରତୀକ;
        }
        ଯଦି (tok == VP_ପ୍ରତୀକ_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ଅଚଳ std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ଗଦା.ଠେଲ(out);
            state_ଠେଲ(ps_JOIN);
            ଯାଅ next_tok;
        }

        // Deal with some special parser states
        ବାଛ (state()) {
        ସ୍ଥିତି ps_ଶୀର୍ଷ: {
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି ps_DEFNAME_UNDEF:  // FALLTHRU
        ସ୍ଥିତି ps_DEFNAME_DEFINE:  // FALLTHRU
        ସ୍ଥିତି ps_DEFNAME_IFDEF:  // FALLTHRU
        ସ୍ଥିତି ps_DEFNAME_IFNDEF:  // FALLTHRU
        ସ୍ଥିତି ps_DEFNAME_ELSIF: {
            ଯଦି (tok == VP_ପ୍ରତୀକ) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ଯଦି (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    ତାର୍କିକ enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    ଯଦି (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ଗଦା.emplace(VPreIfEntry{enable, ମିଥ୍ୟା});
                    ଯଦି (!enable) parsingOff();
                    state_କାଢ଼();
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା ଯଦି (state() == ps_DEFNAME_ELSIF) {
                    ଯଦି (m_ifdef_ଗଦା.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } ଅନ୍ୟଥା {
                        // Handle `else portion
                        ଅଚଳ VPreIfEntry lastIf = m_ifdef_ଗଦା.ଶୀର୍ଷ();
                        m_ifdef_ଗଦା.କାଢ଼();
                        ଯଦି (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ଅଚଳ ତାର୍କିକ enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_ଗଦା.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        ଯଦି (!enable) parsingOff();
                    }
                    state_କାଢ଼();
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା ଯଦି (state() == ps_DEFNAME_UNDEF) {
                    ଯଦି (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_କାଢ଼();
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା ଯଦି (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ଠେଲ_State_Def_Form();
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                ଯଦି (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ଠେଲ_State_ପଦସମଷ୍ଟି();
                    m_ପଦସମଷ୍ଟି_Parser.reset(fileline());
                    m_ପଦସମଷ୍ଟି_Paren_Level = 1;
                    ବାଛ (state()) {
                    ସ୍ଥିତି ps_DEFNAME_IFDEF: stateChange(ps_ପଦସମଷ୍ଟି_IFDEF); ଭାଙ୍ଗ;
                    ସ୍ଥିତି ps_DEFNAME_IFNDEF: stateChange(ps_ପଦସମଷ୍ଟି_IFNDEF); ଭାଙ୍ଗ;
                    ସ୍ଥିତି ps_DEFNAME_ELSIF: stateChange(ps_ପଦସମଷ୍ଟି_ELSIF); ଭାଙ୍ଗ;
                    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3fatalSrc("bad case");
                    }
                    ଯାଅ next_tok;
                }
                ଯଦି (!m_off) {
                    ଫେରନ୍ତୁ tok;
                } ଅନ୍ୟଥା {
                    ଯାଅ next_tok;
                }
            } ଅନ୍ୟଥା ଯଦି (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                ଭାଙ୍ଗ;
            } ଅନ୍ୟଥା {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି ps_ପଦସମଷ୍ଟି_IFDEF:  // FALLTHRU
        ସ୍ଥିତି ps_ପଦସମଷ୍ଟି_IFNDEF:  // FALLTHRU
        ସ୍ଥିତି ps_ପଦସମଷ୍ଟି_ELSIF: {
            // `ifdef ( *here*
            File_ଧାଡ଼ି* ଅଚଳ flp = m_lexp->m_tokFilelinep;
            ଯଦି (tok == VP_ପ୍ରତୀକ) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ଅଚଳ ତାର୍କିକ exists = defExists(m_lastSym);
                ଯଦି (exists) {
                    ଅଚଳ std::string ମୂଲ୍ୟ = def_ମୂଲ୍ୟ(m_lastSym);
                    ଯଦି (VString::removeWhitespace(ମୂଲ୍ୟ) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << ମୂଲ୍ୟ
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, exists});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_WHITE) {
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::BRA});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_ପଦସମଷ୍ଟି_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                ଯଦି (--m_ପଦସମଷ୍ଟି_Paren_Level > 0) {
                    m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::KET});
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା {
                    // Done with parsing expression
                    ତାର୍କିକ enable = m_ପଦସମଷ୍ଟି_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    ଯଦି (state() == ps_ପଦସମଷ୍ଟି_IFDEF || state() == ps_ପଦସମଷ୍ଟି_IFNDEF) {
                        ଯଦି (state() == ps_ପଦସମଷ୍ଟି_IFNDEF) enable = !enable;
                        m_ifdef_ଗଦା.emplace(VPreIfEntry{enable, ମିଥ୍ୟା});
                        ଯଦି (!enable) parsingOff();
                        state_କାଢ଼();
                        ଯାଅ next_tok;
                    } ଅନ୍ୟଥା ଯଦି (state() == ps_ପଦସମଷ୍ଟି_ELSIF) {
                        ଯଦି (m_ifdef_ଗଦା.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } ଅନ୍ୟଥା {
                            // Handle `else portion
                            ଅଚଳ VPreIfEntry lastIf = m_ifdef_ଗଦା.ଶୀର୍ଷ();
                            m_ifdef_ଗଦା.କାଢ଼();
                            ଯଦି (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_ଗଦା.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            ଯଦି (!enable) parsingOff();
                        }
                        state_କାଢ଼();
                    }
                    ଯାଅ next_tok;
                }
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::LNOT});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::LAND});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::LOR});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::IMP});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_ପଦସମଷ୍ଟି_Parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::EQV});
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                ଯଦି (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    ଫେରନ୍ତୁ tok;
                } ଅନ୍ୟଥା {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି ps_DEFFORM: {
            ଯଦି (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_ମୂଲ୍ୟ;
                ଯଦି (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ଠେଲ_State_Def_ମୂଲ୍ୟ();
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT) {
                // IE, something like comment in formals
                ଯଦି (!m_off) ଫେରନ୍ତୁ tok;
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି ps_DEFVALUE: {
            ସ୍ଥିର string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            ଯଦି (tok == VP_DEFVALUE) {
                ଯଦି (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_ମୂଲ୍ୟ)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ଅଚଳ string formals = m_formals;
                string ମୂଲ୍ୟ = m_lexp->m_def_ମୂଲ୍ୟ;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                ପାଇଁ (size_t i = 0; i < formals.length(); i++) {
                    ଯଦି (formals[i] == '\n') s_newlines += "\n";
                }
                ପାଇଁ (size_t i = 0; i < ମୂଲ୍ୟ.length(); i++) {
                    ଯଦି (ମୂଲ୍ୟ[i] == '\n') s_newlines += "\n";
                }
                ଯଦି (!m_off) {
                    // Remove leading and trailing whitespace
                    ମୂଲ୍ୟ = trimWhitespace(ମୂଲ୍ୟ, ସତ୍ୟ);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(ମୂଲ୍ୟ) << "'");
                    define(fileline(), m_lastSym, ମୂଲ୍ୟ, formals, ମିଥ୍ୟା);
                }
            } ଅନ୍ୟଥା {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_କାଢ଼();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            ଫେରନ୍ତୁ VP_WHITE;
        }
        ସ୍ଥିତି ps_DEFPAREN: {
            ଯଦି (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ଅଚଳ VDefineRef* ଅଚଳ refp = &(m_defRefs.ଶୀର୍ଷ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_କାଢ଼();
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.ଶୀର୍ଷ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_ମୂଲ୍ୟ);
            m_lexp->m_def_ମୂଲ୍ୟ = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            ଯଦି (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().ଠେଲ_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ଠେଲ_State_Def_Arg(1);
                refp->nextarg("");
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().ଠେଲ_back(refp->nextarg());
                string out;
                ଯଦି (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.କାଢ଼(), refp);
                state_କାଢ଼();
                ଯଦି (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    ଯଦି (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ଗଦା.empty(), "`` join stack empty, but in a ``");
                        ଅଚଳ string lhs = m_join_ଗଦା.ଶୀର୍ଷ();
                        m_join_ଗଦା.କାଢ଼();
                        out.ଭର୍ତ୍ତି(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_କାଢ଼();
                    }
                    ଯଦି (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    ଅନ୍ୟଥା ଯଦି (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.ଶୀର୍ଷ().parenLevel();
                } ଅନ୍ୟଥା {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.ଶୀର୍ଷ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_ମୂଲ୍ୟ + out);
                    m_lexp->m_def_ମୂଲ୍ୟ = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                ଭାଙ୍ଗ;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_ପ୍ରତୀକ || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ଠେଲ(ps_STRIFY);
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_କାଢ଼();
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି ps_INCNAME: {
            ଯଦି (tok == VP_STRING) {
                state_କାଢ଼();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ଠେଲ_State_Inc_Filename();
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                ଭାଙ୍ଗ;
            } ଅନ୍ୟଥା {
                state_କାଢ଼();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                ଯାଅ next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        ସ୍ଥିତି ps_ERRORNAME: {
            ଯଦି (tok == VP_STRING) {
                ଯଦି (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_କାଢ଼();
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_କାଢ଼();
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି ps_JOIN: {
            ଯଦି (tok == VP_ପ୍ରତୀକ || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ଗଦା.empty(), "`` join stack empty, but in a ``");
                ଅଚଳ string lhs = m_join_ଗଦା.ଶୀର୍ଷ();
                m_join_ଗଦା.କାଢ଼();
                UINFO(5, "`` LHS:" << lhs);
                ଅଚଳ string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ଅଚଳ string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_କାଢ଼();
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ଅଚଳ string lhs = m_join_ଗଦା.ଶୀର୍ଷ();
                m_join_ଗଦା.କାଢ଼();
                unputString(lhs);
                state_କାଢ଼();
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା {
                // `define, etc, fall through and expand.  Pop back here.
                ଭାଙ୍ଗ;
            }
        }
        ସ୍ଥିତି ps_STRIFY: {
            ଯଦି (tok == VP_STRIFY) {
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
                state_କାଢ଼();
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                ଭାଙ୍ଗ;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                ଯାଅ next_tok;
            } ଅନ୍ୟଥା ଯଦି (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                ଭାଙ୍ଗ;
            } ଅନ୍ୟଥା {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                ଯାଅ next_tok;
            }
        }
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        ବାଛ (tok) {
        ସ୍ଥିତି VP_INCLUDE:
            ଯଦି (!m_off) {
                state_ଠେଲ(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            ଯାଅ next_tok;
        ସ୍ଥିତି VP_UNDEF: state_ଠେଲ(ps_DEFNAME_UNDEF); ଯାଅ next_tok;
        ସ୍ଥିତି VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ଠେଲ(ps_DEFNAME_DEFINE);
            ଯାଅ next_tok;
        ସ୍ଥିତି VP_IFDEF: state_ଠେଲ(ps_DEFNAME_IFDEF); ଯାଅ next_tok;
        ସ୍ଥିତି VP_IFNDEF: state_ଠେଲ(ps_DEFNAME_IFNDEF); ଯାଅ next_tok;
        ସ୍ଥିତି VP_ELSIF: state_ଠେଲ(ps_DEFNAME_ELSIF); ଯାଅ next_tok;
        ସ୍ଥିତି VP_ELSE:
            ଯଦି (m_ifdef_ଗଦା.empty()) {
                fileline()->v3error("`else with no matching `if");
            } ଅନ୍ୟଥା {
                ଅଚଳ VPreIfEntry lastIf = m_ifdef_ଗଦା.ଶୀର୍ଷ();
                m_ifdef_ଗଦା.କାଢ଼();
                ଅଚଳ ତାର୍କିକ enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ଗଦା.emplace(VPreIfEntry{enable, lastIf.everOn()});
                ଯଦି (!lastIf.on()) parsingOn();
                ଯଦି (!enable) parsingOff();
            }
            ଯାଅ next_tok;
        ସ୍ଥିତି VP_ENDIF:
            UINFO(4, "Endif ");
            ଯଦି (m_ifdef_ଗଦା.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } ଅନ୍ୟଥା {
                ଅଚଳ VPreIfEntry lastIf = m_ifdef_ଗଦା.ଶୀର୍ଷ();
                m_ifdef_ଗଦା.କାଢ଼();
                ଯଦି (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            ଯାଅ next_tok;

        ସ୍ଥିତି VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ଅଚଳ string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            ଯଦି (m_defPutJoin) {
                m_defPutJoin = ମିଥ୍ୟା;
                unputString("``");
            }
            ଯଦି (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                ଯାଅ next_tok;
            }
            // Substitute
            ଯଦି (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                ଯଦି (m_off) {
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    ଯାଅ next_tok;
                }
            } ଅନ୍ୟଥା {
                ଅଚଳ string params = defParams(name);
                ଯଦି (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    ଯଦି (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    ଯଦି (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        ଯଦି (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ଗଦା.empty(), "`` join stack empty, but in a ``");
                            ଅଚଳ string lhs = m_join_ଗଦା.ଶୀର୍ଷ();
                            m_join_ଗଦା.କାଢ଼();
                            out.ଭର୍ତ୍ତି(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_କାଢ଼();
                        }
                        ଯଦି (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        ଅନ୍ୟଥା ଯଦି (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } ଅନ୍ୟଥା {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ଅଚଳ refp = &(m_defRefs.ଶୀର୍ଷ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_ମୂଲ୍ୟ + out);
                        m_lexp->m_def_ମୂଲ୍ୟ = "";
                    }
                    ଯାଅ next_tok;
                } ଅନ୍ୟଥା {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    ଯଦି (!m_defRefs.empty()) m_defRefs.ଶୀର୍ଷ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ଠେଲ(ps_DEFPAREN);
                    m_lexp->ଠେଲ_State_Def_Arg(0);
                    ଯାଅ next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            ଯାଅ next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        ସ୍ଥିତି VP_ERROR: {
            state_ଠେଲ(ps_ERRORNAME);
            ଯାଅ next_tok;
        }
        ସ୍ଥିତି VP_EOF:
            ଯଦି (!m_ifdef_ଗଦା.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            ଫେରନ୍ତୁ tok;
        ସ୍ଥିତି VP_UNDEFINEALL:
            ଯଦି (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            ଯାଅ next_tok;
        ସ୍ଥିତି VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ଠେଲ(ps_STRIFY);
            ଯାଅ next_tok;
        ସ୍ଥିତି VP_ପ୍ରତୀକ:
        ସ୍ଥିତି VP_STRING:
        ସ୍ଥିତି VP_TEXT: {
            m_defDepth = 0;
            ଯଦି (!m_off) {
                ଫେରନ୍ତୁ tok;
            } ଅନ୍ୟଥା {
                ଯାଅ next_tok;
            }
        }
        ସ୍ଥିତି VP_WHITE:  // Handled at top of loop
        ସ୍ଥିତି VP_COMMENT:  // Handled at top of loop
        ସ୍ଥିତି VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        ସ୍ଥିତି VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            ଭାଙ୍ଗ;  // LCOV_EXCL_LINE
        }
        ଫେରନ୍ତୁ tok;
    }
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    ଯଦି (!m_finAhead) {
        m_finAhead = ସତ୍ୟ;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok = m_finToken;
    buf = m_finBuf;
    ଯଦି (ମିଥ୍ୟା && debug() >= 5) {
        ଅଚଳ string bufcln = V3PreLex::cleanDbgStrg(buf);
        ଅଚଳ string flcol = m_lexp->m_tokFilelinep->ascii_ଧାଡ଼ି_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ଅଚଳ ଅକ୍ଷର* bufp = buf.c_str();
    ଯେତେବେଳେ (*bufp == '\n') bufp++;
    ଯଦି ((tok == VP_TEXT || tok == VP_ଧାଡ଼ି) && VString::startsWith(bufp, "`line ")) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା enter;
        m_finFilelinep->ଧାଡ଼ି_Directive(bufp, enter /*ref*/);
    } ଅନ୍ୟଥା {
        ଯଦି (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->ଧାଡ଼ି_Directives()) {
            ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                ଯଦି (debug() >= 5) {
                    ଅଚଳ string flcol = m_lexp->m_tokFilelinep->ascii_ଧାଡ଼ି_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                ଯଦି (outBehind > 0
                    && (outBehind <= static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    ଯଦି (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        ଫେରନ୍ତୁ VP_TEXT;
                    }
                } ଅନ୍ୟଥା {
                    // Need to backup, use `line
                    buf = m_finFilelinep->ଧାଡ଼ି_Directive_Strg(0);
                    ଫେରନ୍ତୁ VP_ଧାଡ଼ି;
                }
            }
        }
        // Track newlines in prep for next token
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର& c : buf) {
            ଯଦି (c == '\n') {
                m_finAtBol = ସତ୍ୟ;
                m_finFilelinep->linenoInc();
            } ଅନ୍ୟଥା {
                m_finAtBol = ମିଥ୍ୟା;
            }
        }
    }
    m_finAhead = ମିଥ୍ୟା;  // Consumed the token
    ଫେରନ୍ତୁ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    ଯଦି (isEof()) ଫେରନ୍ତୁ "";
    ଅଚଳ ଅକ୍ଷର* rtnp;
    ତାର୍କିକ gotEof = ମିଥ୍ୟା;
    ଯେତେବେଳେ (nullptr == (rtnp = std::strchr(m_ଧାଡ଼ି_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok = getFinalToken(m_tokenBuf /*ref*/);
        ଯଦି (debug() >= 5) {
            ଅଚଳ string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ଅଚଳ string flcol = m_lexp->m_tokFilelinep->ascii_ଧାଡ଼ି_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        ଯଦି (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            ଯଦି (m_ଧାଡ଼ି_Chars != "" && m_ଧାଡ଼ି_Chars[m_ଧାଡ଼ି_Chars.length() - 1] != '\n') {
                m_ଧାଡ଼ି_Chars.append("\n");
            }
            gotEof = ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            m_ଧାଡ଼ି_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା len = rtnp - m_ଧାଡ଼ି_Chars.c_str() + 1;
    string the_ଧାଡ଼ି(m_ଧାଡ଼ି_Chars, 0, len);
    m_ଧାଡ଼ି_Chars = m_ଧାଡ଼ି_Chars.erase(0, len);  // Remove returned characters
    ଯଦି (debug() >= 4) {
        ଅଚଳ string lncln = V3PreLex::cleanDbgStrg(the_ଧାଡ଼ି);
        ଅଚଳ string flcol = m_lexp->m_tokFilelinep->ascii_ଧାଡ଼ି_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ଫେରନ୍ତୁ the_ଧାଡ଼ି;
}
