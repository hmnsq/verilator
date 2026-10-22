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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreProc.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LanguageWords.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreExpr.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreLex.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreShell.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdlib>
#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <stack>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ yyFlexLexer V3Lexer
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreLex.yy.cpp"
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

ਵਰਗ VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_ਸਤਰ* ਅਚਲ m_fileline;  // Where it was declared
    ਅਚਲ string m_ਮੁੱਲ;  // Value of define
    ਅਚਲ string m_params;  // Parameters
    ਅਚਲ ਤਾਰਕਿਕ m_cmdline;  // Set on command line, don't `undefineall
ਜਨਤਕ:
    VDefine(File_ਸਤਰ* fl, ਅਚਲ string& ਮੁੱਲ, ਅਚਲ string& params, ਤਾਰਕਿਕ cmdline)
        : m_fileline{fl}
        , m_ਮੁੱਲ{ਮੁੱਲ}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ m_fileline; }
    string ਮੁੱਲ() ਅਚਲ { ਵਾਪਸ m_ਮੁੱਲ; }
    string params() ਅਚਲ { ਵਾਪਸ m_params; }
    ਤਾਰਕਿਕ cmdline() ਅਚਲ { ਵਾਪਸ m_cmdline; }
};

//*************************************************************************

ਵਰਗ VDefineRef final {
    // One for each pending define substitution
    ਅਚਲ string m_name;  // Define last name being defined
    ਅਚਲ string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    ਪੂਰਨ m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::ਸਦਿਸ਼<string> m_ਦਲੀਲਾਂ;  // List of define arguments
ਜਨਤਕ:
    string name() ਅਚਲ { ਵਾਪਸ m_name; }
    string params() ਅਚਲ { ਵਾਪਸ m_params; }
    string nextarg() ਅਚਲ { ਵਾਪਸ m_nextarg; }
    ਖਾਲੀ nextarg(ਅਚਲ string& ਮੁੱਲ) { m_nextarg = ਮੁੱਲ; }
    ਪੂਰਨ parenLevel() ਅਚਲ { ਵਾਪਸ m_parenLevel; }
    ਖਾਲੀ parenLevel(ਪੂਰਨ ਮੁੱਲ) { m_parenLevel = ਮੁੱਲ; }
    std::ਸਦਿਸ਼<string>& ਦਲੀਲਾਂ() { ਵਾਪਸ m_ਦਲੀਲਾਂ; }
    VDefineRef(ਅਚਲ string& name, ਅਚਲ string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = ਮੂਲ;
};

//*************************************************************************
/// Data for parsing on/off

ਵਰਗ VPre_If_ਇੰਦਰਾਜ final {
    // One for each pending ifdef/ifndef
    ਅਚਲ ਤਾਰਕਿਕ m_on;  // Current parse for this ifdef level is "on"
    ਅਚਲ ਤਾਰਕਿਕ m_everOn;  // Some if term in elsif tree has been on
ਜਨਤਕ:
    ਤਾਰਕਿਕ on() ਅਚਲ { ਵਾਪਸ m_on; }
    ਤਾਰਕਿਕ everOn() ਅਚਲ { ਵਾਪਸ m_everOn; }
    VPre_If_ਇੰਦਰਾਜ(ਤਾਰਕਿਕ on, ਤਾਰਕਿਕ everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_ਇੰਦਰਾਜ() = ਮੂਲ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

ਵਰਗ V3PreProcImp final : ਜਨਤਕ V3PreProc {
ਜਨਤਕ:
    // TYPES
    ਵਰਤੋਂ ਨਿਰਧਾਰਤ_ਕਰਦਾ_Map = std::map<ਅਚਲ std::string, VDefine>;
    ਵਰਤੋਂ StrList = VInFilter::StrList;

    // Defines list
    ਨਿਰਧਾਰਤ_ਕਰਦਾ_Map m_ਨਿਰਧਾਰਤ_ਕਰਦਾ;  ///< Map of defines

    // STATE
    ਅਚਲ V3PreProc* m_preprocp = ਖਾਲੀ_ਸੂਚਕ;  ///< Object we're holding data for
    V3PreLex* m_lexp = ਖਾਲੀ_ਸੂਚਕ;  ///< Current lexer state (nullptr = closed)
    ਪੂਰਨ m_ਆਖਰੀ_Lineno = 0;  // Last line number (stall detection)
    ਪੂਰਨ m_ਟੋਕਨ_On_ਸਤਰ = 0;  // Number of tokens on line (stall detection)

    ਗਿਣਤੀ ProcState : uint8_t {
        ps_ਸਿਖਰ,
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
        ps_ਵਿਅੰਜਕ_IFDEF,
        ps_ਵਿਅੰਜਕ_IFNDEF,
        ps_ਵਿਅੰਜਕ_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* procStateName(ProcState s) {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਹਾਲਤਾਂ[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ਵਾਪਸ ਹਾਲਤਾਂ[s];
    }

    std::ਢੇਰ<ProcState> m_ਹਾਲਤਾਂ;  ///< Current state of parser
    ਪੂਰਨ m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    ਤਾਰਕਿਕ m_incError = ਝੂਠ;  ///< Include error found
    string m_ਆਖਰੀ_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_ਸਤਰ_Cmt;  ///< Line comment(s) to be returned
    ਤਾਰਕਿਕ m_ਸਤਰ_Cmt_Nl = ਝੂਠ;  ///< Newline needed before inserting lineCmt
    ਪੂਰਨ m_ਸਤਰ_ਜੋੜੋ = 0;  ///< Empty lines to return to maintain line count
    ਤਾਰਕਿਕ m_rawAtBol = ਸੱਚ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    ਤਾਰਕਿਕ m_finAhead = ਝੂਠ;  ///< Have read a token ahead
    ਪੂਰਨ m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    ਤਾਰਕਿਕ m_finAtBol = ਸੱਚ;  ///< Last getFinalToken left us at beginning of line
    File_ਸਤਰ* m_finFilelinep = ਖਾਲੀ_ਸੂਚਕ;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ਢੇਰ<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ਢੇਰ<VPre_If_ਇੰਦਰਾਜ> m_ifdef_ਢੇਰ;  ///< Stack of true/false emitting evaluations
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_def_ਡੂੰਘਾਈ = 0;  ///< How many `defines deep
    ਤਾਰਕਿਕ m_defPutJoin = ਝੂਠ;  ///< Insert `` after substitution

    // For `` join
    std::ਢੇਰ<string> m_join_ਢੇਰ;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_ਵਿਅੰਜਕ m_ਵਿਅੰਜਕ_Parser;  ///< Parser for () expression
    ਪੂਰਨ m_ਵਿਅੰਜਕ_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_ਸਤਰ_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    ਖਾਲੀ v3error_ਸਿਰਾ(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_ਸਿਰਾ(str);
    }

    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* tokenName(ਪੂਰਨ tok);
    ਖਾਲੀ debugToken(ਪੂਰਨ tok, ਅਚਲ ਅੱਖਰ* cmtp);
    ਖਾਲੀ parse_ਸਿਖਰ();
    ਖਾਲੀ parseUndef();

ਨਿਜੀ:
    // Internal methods
    ਖਾਲੀ ਸਿਰਾ_ਦਾ_One_File();
    string defineSubst(VDefineRef* refp);

    ਤਾਰਕਿਕ defExists(ਅਚਲ string& name);
    ਤਾਰਕਿਕ defCmdline(ਅਚਲ string& name);
    string def_ਮੁੱਲ(ਅਚਲ string& name);
    string defParams(ਅਚਲ string& name);
    File_ਸਤਰ* defFileline(ਅਚਲ string& name);

    string commentCleanup(ਅਚਲ string& ਪਾਠ);
    ਤਾਰਕਿਕ comment_Token_ਮੇਲ(string& cmdr, ਅਚਲ ਅੱਖਰ* strg);
    ਸਥਿਰ string trimWhitespace(ਅਚਲ string& strg, ਤਾਰਕਿਕ trailing);
    ਖਾਲੀ unputString(ਅਚਲ string& strg);
    ਖਾਲੀ unputDefrefString(ਅਚਲ string& strg);

    ਖਾਲੀ parsingOn() {
        m_off--;
        ਜੇ (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    ਖਾਲੀ parsingOff() { m_off++; }

    ਪੂਰਨ getRawToken();
    ਪੂਰਨ getStateToken();
    ਪੂਰਨ getFinalToken(string& buf);

    ProcState state() ਅਚਲ { ਵਾਪਸ m_ਹਾਲਤਾਂ.ਸਿਖਰ(); }
    ਤਾਰਕਿਕ stateIsDefname() ਅਚਲ {
        ਵਾਪਸ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    ਖਾਲੀ state_ਧੱਕੋ(ProcState state) { m_ਹਾਲਤਾਂ.ਧੱਕੋ(state); }
    ਖਾਲੀ state_ਕੱਢੋ() {
        m_ਹਾਲਤਾਂ.ਕੱਢੋ();
        ਜੇ (VL_UNCOVERABLE(m_ਹਾਲਤਾਂ.ਖਾਲੀ_ਥਾਵਾਂ())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_ਹਾਲਤਾਂ.ਧੱਕੋ(ps_ਸਿਖਰ);  // LCOV_EXCL_LINE
        }
    }
    ਖਾਲੀ stateChange(ProcState state) {
        state_ਕੱਢੋ();
        state_ਧੱਕੋ(state);
    }

ਜਨਤਕ:
    // METHODS, called from upper level shell
    ਖਾਲੀ openFile(File_ਸਤਰ* fl, VInFilter* filterp, ਅਚਲ string& filename) override;
    ਤਾਰਕਿਕ isEof() ਅਚਲ override { ਵਾਪਸ m_lexp->curStreamp()->m_eof; }
    ਖਾਲੀ forceEof() { m_lexp->curStreamp()->m_eof = ਸੱਚ; }
    string getline() override;
    ਖਾਲੀ ਪਾਓ_Unreadback(ਅਚਲ string& ਪਾਠ) override { m_ਸਤਰ_Cmt += ਪਾਠ; }
    ਖਾਲੀ ਪਾਓ_Unreadback_At_Bol(ਅਚਲ string& ਪਾਠ);
    ਖਾਲੀ ਜੋੜੋ_ਸਤਰ_Comment(ਪੂਰਨ enterExit);
    ਖਾਲੀ dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ(std::ostream& os) override;
    ਖਾਲੀ candidate_ਨਿਰਧਾਰਤ_ਕਰਦਾ(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    ਖਾਲੀ comment(ਅਚਲ string& ਪਾਠ) override;  // Comment detected (if keepComments==2)
    ਖਾਲੀ include(ਅਚਲ string& filename) override;  // Request a include file be processed
    ਖਾਲੀ undef(ਅਚਲ string& name) override;
    ਆਭਾਸੀ ਖਾਲੀ undefineall();
    ਖਾਲੀ define(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& ਮੁੱਲ, ਅਚਲ string& params,
                ਤਾਰਕਿਕ cmdline) override;
    string remove_ਨਿਰਧਾਰਤ_ਕਰਦਾ(ਅਚਲ string& ਪਾਠ) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_ਹਾਲਤਾਂ.ਧੱਕੋ(ps_ਸਿਖਰ); }
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ configure(File_ਸਤਰ* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = ਇਹ;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = ਨਵਾਂ File_ਸਤਰ{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = ਨਵਾਂ V3PreLex{ਇਹ, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        ਜੇ (m_lexp) VL_DO_CLEAR(ਮਿਟਾਓ m_lexp, m_lexp = ਖਾਲੀ_ਸੂਚਕ);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_ਸਤਰ* fl) {
    V3PreProcImp* ਅਚਲ preprocp = ਨਵਾਂ V3PreProcImp;
    preprocp->configure(fl);
    ਵਾਪਸ preprocp;
}

ਖਾਲੀ V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_ਵਿਅੰਜਕ::selfTest(); }

//*************************************************************************
//ਨਿਰਧਾਰਤ_ਕਰਦਾ

ਖਾਲੀ V3PreProcImp::undef(ਅਚਲ string& name) {
    ਜੇ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ())
        ਪਾਓ_Unreadback("`undef " + name + "\n");
    m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.erase(name);
}
ਖਾਲੀ V3PreProcImp::undefineall() {
    ਜੇ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ())
        ਪਾਓ_Unreadback("`undefineall\n");
    ਲਈ (ਨਿਰਧਾਰਤ_ਕਰਦਾ_Map::iterator nextit, it = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.begin(); it != m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ(); it = nextit) {
        nextit = it;
        ++nextit;
        ਜੇ (!it->ਦੂਜਾ.cmdline()) m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.erase(it);
    }
}
ਤਾਰਕਿਕ V3PreProcImp::defExists(ਅਚਲ string& name) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਲੱਭੋ(name);
    ਵਾਪਸ (iter != m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ());
}
ਤਾਰਕਿਕ V3PreProcImp::defCmdline(ਅਚਲ string& name) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਲੱਭੋ(name);
    ਜੇ (iter == m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ()) { ਵਾਪਸ ਝੂਠ; }
    ਵਾਪਸ iter->ਦੂਜਾ.cmdline();
}
string V3PreProcImp::def_ਮੁੱਲ(ਅਚਲ string& name) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਲੱਭੋ(name);
    ਜੇ (iter == m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ਵਾਪਸ "";
    }
    ਵਾਪਸ iter->ਦੂਜਾ.ਮੁੱਲ();
}
string V3PreProcImp::defParams(ਅਚਲ string& name) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਲੱਭੋ(name);
    ਜੇ (iter == m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ਵਾਪਸ "";
    }
    ਵਾਪਸ iter->ਦੂਜਾ.params();
}
File_ਸਤਰ* V3PreProcImp::defFileline(ਅਚਲ string& name) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਲੱਭੋ(name);
    ਜੇ (iter == m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    ਵਾਪਸ iter->ਦੂਜਾ.fileline();
}
ਖਾਲੀ V3PreProcImp::define(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& ਮੁੱਲ,
                          ਅਚਲ string& params, ਤਾਰਕਿਕ cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << ਮੁੱਲ << "' params '" << params << "'");
    ਜੇ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string ਬਾਹਰਲਾ;
        ਬਾਹਰਲਾ.reserve(ਮੁੱਲ.ਮਾਪ());
        ਲਈ (string::ਮਾਪ_type pos = 0; pos < ਮੁੱਲ.ਮਾਪ(); ++pos) {
            ਜੇ (ਮੁੱਲ[pos] == '\n' && (pos == 0 || ਮੁੱਲ[pos - 1] != '\\')) ਬਾਹਰਲਾ += '\\';
            ਬਾਹਰਲਾ += ਮੁੱਲ[pos];
        }
        ਪਾਓ_Unreadback("`define " + name + params + (ਬਾਹਰਲਾ.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : " ") + ਬਾਹਰਲਾ + "\n");
    }
    ਜੇ (!V3_Language_ਸ਼ਬਦ::isKeyword("`"s + name).ਖਾਲੀ_ਥਾਵਾਂ()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } ਨਹੀਂਤਾਂ {
        ਜੇ (defExists(name)) {
            ਜੇ (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << ਮੁੱਲ
                               << "' to existing command line define value: '" << def_ਮੁੱਲ(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_ਮੁੱਲ(name) << (defParams(name).ਖਾਲੀ_ਥਾਵਾਂ() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!(def_ਮੁੱਲ(name) == ਮੁੱਲ
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << ਮੁੱਲ
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_ਮੁੱਲ(name) << (defParams(name).ਖਾਲੀ_ਥਾਵਾਂ() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.emplace(name, VDefine{fl, ਮੁੱਲ, params, cmdline});
    }
}

string V3PreProcImp::remove_ਨਿਰਧਾਰਤ_ਕਰਦਾ(ਅਚਲ string& ਪਾਠ) {
    string val;
    string rtnsym = ਪਾਠ;
    ਲਈ (ਪੂਰਨ loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        ਜੇ (xsym[0] == '`') xsym.erase(0, 1);
        ਜੇ (defExists(xsym)) {
            val = def_ਮੁੱਲ(xsym);
            ਜੇ (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } ਨਹੀਂਤਾਂ {
                ਰੋਕੋ;
            }
        } ਨਹੀਂਤਾਂ {
            ਰੋਕੋ;
        }
    }
    ਵਾਪਸ rtnsym;  // NA
}

//**********************************************************************
// Comments

ਖਾਲੀ V3PreProcImp::include(ਅਚਲ string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ਅਚਲ string& ਪਾਠ) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = ਪਾਠ;
    string::ਮਾਪ_type pos;
    ਜਦੋਂ ((pos = cmd.ਲੱਭੋ("//")) != string::npos) cmd.replace(pos, 2, "");
    ਜਦੋਂ ((pos = cmd.ਲੱਭੋ("/*")) != string::npos) cmd.replace(pos, 2, "");
    ਜਦੋਂ ((pos = cmd.ਲੱਭੋ("*/")) != string::npos) cmd.replace(pos, 2, "");
    ਜਦੋਂ ((pos = cmd.ਲੱਭੋ('\"')) != string::npos) cmd.replace(pos, 1, " ");
    ਜਦੋਂ ((pos = cmd.ਲੱਭੋ('\t')) != string::npos) cmd.replace(pos, 1, " ");
    ਜਦੋਂ ((pos = cmd.ਲੱਭੋ("  ")) != string::npos) cmd.replace(pos, 2, " ");
    ਜਦੋਂ (!cmd.ਖਾਲੀ_ਥਾਵਾਂ() && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(cmd[cmd.ਮਾਪ() - 1])) cmd.erase(cmd.ਮਾਪ() - 1);
    ਵਾਪਸ cmd;
}

ਤਾਰਕਿਕ V3PreProcImp::comment_Token_ਮੇਲ(string& cmdr, ਅਚਲ ਅੱਖਰ* strg) {
    ਪੂਰਨ len = std::ਲੜੀ_ਲੰਬਾਈ(strg);
    ਜੇ (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::ਖਾਲੀ_ਥਾਂ_ਹੈ(cmdr[len]))) {
        ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ਵਾਪਸ ਸੱਚ;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ ਝੂਠ;
    }
}

ਖਾਲੀ V3PreProcImp::comment(ਅਚਲ string& ਪਾਠ) {
    // Comment detected.  Only keep relevant data.
    ਤਾਰਕਿਕ printed = ਝੂਠ;
    ਜੇ (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ਪਾਓ_Unreadback(ਪਾਠ);
        printed = ਸੱਚ;
    }

    ਅਚਲ ਅੱਖਰ* cp = ਪਾਠ.c_str();
    ਜੇ (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ;
    }

    ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp)) ++cp;

    ਤਾਰਕਿਕ synth = ਝੂਠ;
    ਤਾਰਕਿਕ vlcomment = ਝੂਠ;
    ਜੇ ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::ਲੜੀ_ਲੰਬਾਈ("verilator");
        ਜੇ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ਵਾਪਸ;
        }
        vlcomment = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (VString::startsWith(cp, "synopsys")) {
        cp += std::ਲੜੀ_ਲੰਬਾਈ("synopsys");
        ਜੇ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ਵਾਪਸ;
        }
        synth = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (VString::startsWith(cp, "cadence")) {
        cp += std::ਲੜੀ_ਲੰਬਾਈ("cadence");
        synth = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (VString::startsWith(cp, "pragma")) {
        cp += std::ਲੜੀ_ਲੰਬਾਈ("pragma");
        synth = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::ਲੜੀ_ਲੰਬਾਈ("ambit synthesis");
        synth = ਸੱਚ;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ;
    }

    ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    ਜੇ (synth) {
        ਜੇ (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            ਜੇ (comment_Token_ਮੇਲ(cmd /*ref*/, "full_case")) {
                ਜੇ (!printed) ਪਾਓ_Unreadback("/*verilator full_case*/");
            }
            ਜੇ (comment_Token_ਮੇਲ(cmd /*ref*/, "parallel_case")) {
                ਜੇ (!printed) ਪਾਓ_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } ਨਹੀਂਤਾਂ ਜੇ (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.ਜਨਤਕ_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        ਜੇ (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::ਮਾਪ_type ਸਿਰਾ_ਦਾ_Cmd = std::ਲੜੀ_ਲੰਬਾਈ("public_flat_rw");
            ਜਦੋਂ (VString::isIdentifierChar(cmd[ਸਿਰਾ_ਦਾ_Cmd])) ++ਸਿਰਾ_ਦਾ_Cmd;
            string ਅਧਾਰ_Cmd = cmd.substr(0, ਸਿਰਾ_ਦਾ_Cmd);
            string arg = cmd.substr(ਸਿਰਾ_ਦਾ_Cmd);
            ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(arg[0])) arg = arg.substr(1);
            ਜੇ (arg.ਮਾਪ() && ਅਧਾਰ_Cmd == "public_flat_rw_on")
                ਅਧਾਰ_Cmd += "_sns";  // different cmd for applying sensitivity
            ਜੇ (!printed) ਪਾਓ_Unreadback("/*verilator " + ਅਧਾਰ_Cmd + "*/ " + arg + " /**/");
        } ਨਹੀਂਤਾਂ ਜੇ (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string ਅਧਾਰ_Cmd = cmd.substr(0, std::ਲੜੀ_ਲੰਬਾਈ("dpi_c_decl"));
            string::ਮਾਪ_type ਸ਼ੁਰੂ_ਦਾ_Arg = ਅਧਾਰ_Cmd.ਮਾਪ();
            ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(cmd[ਸ਼ੁਰੂ_ਦਾ_Arg])) ਸ਼ੁਰੂ_ਦਾ_Arg++;
            string arg = cmd.substr(ਸ਼ੁਰੂ_ਦਾ_Arg);
            ਜੇ (arg.ਖਾਲੀ_ਥਾਵਾਂ()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ਵਾਪਸ;
            }
            ਜੇ (!printed) ਪਾਓ_Unreadback("/*verilator " + ਅਧਾਰ_Cmd + "*/ \"" + arg + "\" /**/");
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!printed) ਪਾਓ_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_ਸਤਰ* V3PreProc::fileline() {
    ਅਚਲ V3PreProcImp* idatap = static_cast<V3PreProcImp*>(ਇਹ);
    ਵਾਪਸ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ਅਚਲ ਅੱਖਰ* V3PreProcImp::tokenName(ਪੂਰਨ tok) {
    ਚੋਣ (tok) {
    ਹਾਲਤ VP_BACKQUOTE: ਵਾਪਸ ("BACKQUOTE");
    ਹਾਲਤ VP_COMMENT: ਵਾਪਸ ("COMMENT");
    ਹਾਲਤ VP_DEFARG: ਵਾਪਸ ("DEFARG");
    ਹਾਲਤ VP_DEFFORM: ਵਾਪਸ ("DEFFORM");
    ਹਾਲਤ VP_DEFINE: ਵਾਪਸ ("DEFINE");
    ਹਾਲਤ VP_DEFREF: ਵਾਪਸ ("DEFREF");
    ਹਾਲਤ VP_DEFREF_JOIN: ਵਾਪਸ ("DEFREF_JOIN");
    ਹਾਲਤ VP_DEFVALUE: ਵਾਪਸ ("DEFVALUE");
    ਹਾਲਤ VP_ELSE: ਵਾਪਸ ("ELSE");
    ਹਾਲਤ VP_ELSIF: ਵਾਪਸ ("ELSIF");
    ਹਾਲਤ VP_ENDIF: ਵਾਪਸ ("ENDIF");
    ਹਾਲਤ VP_EOF: ਵਾਪਸ ("EOF");
    ਹਾਲਤ VP_EOF_ERROR: ਵਾਪਸ ("EOF_ERROR");
    ਹਾਲਤ VP_ERROR: ਵਾਪਸ ("ERROR");
    ਹਾਲਤ VP_IFDEF: ਵਾਪਸ ("IFDEF");
    ਹਾਲਤ VP_IFNDEF: ਵਾਪਸ ("IFNDEF");
    ਹਾਲਤ VP_JOIN: ਵਾਪਸ ("JOIN");
    ਹਾਲਤ VP_INCLUDE: ਵਾਪਸ ("INCLUDE");
    ਹਾਲਤ VP_ਸਤਰ: ਵਾਪਸ ("LINE");
    ਹਾਲਤ VP_STRIFY: ਵਾਪਸ ("STRIFY");
    ਹਾਲਤ VP_STRING: ਵਾਪਸ ("STRING");
    ਹਾਲਤ VP_ਚਿੰਨ੍ਹ: ਵਾਪਸ ("SYMBOL");
    ਹਾਲਤ VP_ਚਿੰਨ੍ਹ_JOIN: ਵਾਪਸ ("SYMBOL_JOIN");
    ਹਾਲਤ VP_ਪਾਠ: ਵਾਪਸ ("TEXT");
    ਹਾਲਤ VP_UNDEF: ਵਾਪਸ ("UNDEF");
    ਹਾਲਤ VP_UNDEFINEALL: ਵਾਪਸ ("UNDEFINEALL");
    ਹਾਲਤ VP_WHITE: ਵਾਪਸ ("WHITE");
    ਮੂਲ: ਵਾਪਸ ("?");
    }
}

ਖਾਲੀ V3PreProcImp::unputString(ਅਚਲ string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    ਜੇ (VL_UNCOVERABLE(m_lexp->m_ਬਫਰ_State != m_lexp->current_ਬਫਰ())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->ਜਾਂਚ_ਬਾਈਟਾਂ(strg);
}

ਖਾਲੀ V3PreProcImp::unputDefrefString(ਅਚਲ string& strg) {
    ਅਚਲ ਪੂਰਨ multiline = std::ਗਿਣਤੀ_2(strg.begin(), strg.ਸਿਰਾ(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ਅਚਲ string& strg, ਤਾਰਕਿਕ trailing) {
    // Remove leading whitespace
    string ਬਾਹਰਲਾ = strg;
    string::ਮਾਪ_type leadspace = 0;
    ਜਦੋਂ (ਬਾਹਰਲਾ.ਲੰਮਾਈ() > leadspace && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(ਬਾਹਰਲਾ[leadspace])) ++leadspace;
    ਜੇ (leadspace) ਬਾਹਰਲਾ.erase(0, leadspace);
    // Remove trailing whitespace
    ਜੇ (trailing) {
        string::ਮਾਪ_type trailspace = 0;
        ਜਦੋਂ (ਬਾਹਰਲਾ.ਲੰਮਾਈ() > trailspace && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(ਬਾਹਰਲਾ[ਬਾਹਰਲਾ.ਲੰਮਾਈ() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        ਜੇ (trailspace && ਬਾਹਰਲਾ.ਲੰਮਾਈ() > trailspace && ਬਾਹਰਲਾ[ਬਾਹਰਲਾ.ਲੰਮਾਈ() - 1 - trailspace] == '\\')
            --trailspace;
        ਜੇ (trailspace) ਬਾਹਰਲਾ.erase(ਬਾਹਰਲਾ.ਲੰਮਾਈ() - trailspace, trailspace);
    }
    ਵਾਪਸ ਬਾਹਰਲਾ;
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
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < refp->ਦਲੀਲਾਂ().ਮਾਪ(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->ਦਲੀਲਾਂ()[i] << "'");
    }
    // Grab value
    ਅਚਲ string ਮੁੱਲ = def_ਮੁੱਲ(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(ਮੁੱਲ) << "'");

    std::map<ਅਚਲ string, string> arg_ਮੁੱਲ_By_Name;
    {  // Parse argument list into map
        ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੰਕ_ਦਲੀਲਾਂ = 0;
        string argName;
        ਪੂਰਨ paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        ਤਾਰਕਿਕ quote = ਝੂਠ;
        ਤਾਰਕਿਕ haveDefault = ਝੂਠ;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ਅਚਲ string params = refp->params();  // Must keep str in scope to get pointer
        ਅਚਲ ਅੱਖਰ* cp = params.c_str();
        ਜੇ (*cp == '(') cp++;
        ਲਈ (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            ਜੇ (!quote && paren == 1) {
                ਜੇ (*cp == ')' || *cp == ',') {
                    string ਮੁੱਲ_Def;
                    ਜੇ (haveDefault) {
                        ਮੁੱਲ_Def = token;
                    } ਨਹੀਂਤਾਂ {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, ਸੱਚ);
                    UINFO(4, "    Got Arg=" << ਅੰਕ_ਦਲੀਲਾਂ << "  argName='" << argName
                                            << "'  default='" << ਮੁੱਲ_Def << "'");
                    // Parse it
                    ਜੇ (argName != "") {
                        ਜੇ (refp->ਦਲੀਲਾਂ().ਮਾਪ() > ਅੰਕ_ਦਲੀਲਾਂ) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ਅਚਲ string arg = trimWhitespace(refp->ਦਲੀਲਾਂ()[ਅੰਕ_ਦਲੀਲਾਂ], ਸੱਚ);
                            ਜੇ (arg != "") ਮੁੱਲ_Def = arg;
                        } ਨਹੀਂਤਾਂ ਜੇ (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ਵਾਪਸ " `" + refp->name() + " ";
                        }
                        ਅੰਕ_ਦਲੀਲਾਂ++;
                    }
                    arg_ਮੁੱਲ_By_Name[argName] = ਮੁੱਲ_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = ਝੂਠ;
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == '=') {
                    haveDefault = ਸੱਚ;
                    argName = token;
                    token = "";
                    ਜਾਰੀ;
                }
            }
            ਜੇ (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                ਜਾਰੀ;
            }
            ਜੇ (!quote) {
                ਜੇ (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            ਜੇ (*cp == '"') quote = !quote;
            ਜੇ (*cp) token += *cp;
        }
        ਜੇ (refp->ਦਲੀਲਾਂ().ਮਾਪ() > ਅੰਕ_ਦਲੀਲਾਂ
            // `define X() is ok to call with nothing
            && !(refp->ਦਲੀਲਾਂ().ਮਾਪ() == 1 && ਅੰਕ_ਦਲੀਲਾਂ == 0
                 && trimWhitespace(refp->ਦਲੀਲਾਂ()[0], ਝੂਠ) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ਵਾਪਸ " `" + refp->name() + " ";
        }
    }

    string ਬਾਹਰਲਾ;
    {  // Parse substitution define using arguments
        string argName;
        ਤਾਰਕਿਕ quote = ਝੂਠ;
        ਤਾਰਕਿਕ triquote = ਝੂਠ;
        ਤਾਰਕਿਕ backslashesc = ਝੂਠ;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = ਮੁੱਲ.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            ਜੇ (!quote && !triquote && *cp == '\\') {
                backslashesc = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp)) {
                backslashesc = ਝੂਠ;
            }
            ਜੇ (!quote && !triquote) {
                ਜੇ (std::ਅੱਖਰ_ਹੈ(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::ਅੰਕ_ਹੈ(*cp))) {
                    argName += *cp;
                    ਜਾਰੀ;
                }
                ਜੇ (argName != "") {
                    // Found a possible variable substitution
                    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = arg_ਮੁੱਲ_By_Name.ਲੱਭੋ(argName);
                    ਜੇ (iter != arg_ਮੁੱਲ_By_Name.ਸਿਰਾ()) {
                        // Substitute
                        ਅਚਲ string subst = iter->ਦੂਜਾ;
                        ਜੇ (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            ਜੇ (ਬਾਹਰਲਾ.ਮਾਪ() >= 2 && ਬਾਹਰਲਾ.substr(ਬਾਹਰਲਾ.ਮਾਪ() - 2) == "``") {
                                ਬਾਹਰਲਾ.resize(ਬਾਹਰਲਾ.ਮਾਪ() - 2);
                            }
                        } ਨਹੀਂਤਾਂ {
                            ਬਾਹਰਲਾ += subst;
                        }
                    } ਨਹੀਂਤਾਂ {
                        ਬਾਹਰਲਾ += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                ਜੇ (cp[0] == '`' && cp[1] == '`') {
                    ਜੇ (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } ਨਹੀਂਤਾਂ {
                        ਬਾਹਰਲਾ += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (cp[0] == '`' && cp[1] == '"') {
                    ਬਾਹਰਲਾ += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    ਬਾਹਰਲਾ += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (cp[0] == '`' && cp[1] == '\\') {
                    ਬਾਹਰਲਾ += '\\';  // `\ means to put out a backslash
                    cp++;
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    ਬਾਹਰਲਾ += "\n";  // \\n newline
                    cp++;
                    ਜਾਰੀ;
                }
            }
            ਜੇ (cp[0] == '\\' && cp[1] == '\"') {
                ਬਾਹਰਲਾ += cp[0];  // \{any} Put out literal next character
                ਬਾਹਰਲਾ += cp[1];
                cp++;
                ਜਾਰੀ;
            } ਨਹੀਂਤਾਂ ਜੇ (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                ਬਾਹਰਲਾ += cp[0];
                ਜਾਰੀ;
            }
            ਜੇ (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                ਬਾਹਰਲਾ += "\"\"\"";
                cp += 2;
                ਜਾਰੀ;
            }
            ਜੇ (*cp == '"') quote = !quote;
            ਜੇ (*cp) ਬਾਹਰਲਾ += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(ਬਾਹਰਲਾ) << "'");
    ਵਾਪਸ ਬਾਹਰਲਾ;
}

//**********************************************************************
// Parser routines

ਖਾਲੀ V3PreProcImp::openFile(File_ਸਤਰ*, VInFilter* filterp, ਅਚਲ string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    ਜੇ (m_incError) ਵਾਪਸ;
    m_lexp->ਸੈੱਟ_YYDebug(debug() >= 5);
    V3File::ਜੋੜੋ_Src_Depend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ਅਚਲ ਤਾਰਕਿਕ ok = filterp->readWholefile(filename, wholefile /*ref*/);
    ਜੇ (!ok) {
        fileline()->v3error("File not found: " + filename);
        ਵਾਪਸ;
    }

    ਜੇ (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        ਜੇ (m_lexp->m_streamp_ਢੇਰ.ਮਾਪ() > V3PreProc::INCLUDE_ਡੂੰਘਾਈ_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = ਸੱਚ;
            ਵਾਪਸ;
        }
        // There's already a file active.  Push it to work on the new one.
        ਜੋੜੋ_ਸਤਰ_Comment(0);
    }

    // Save file contents for future error reporting
    File_ਸਤਰ* ਅਚਲ flsp = ਨਵਾਂ File_ਸਤਰ{filename};
    flsp->lineno(1);
    flsp->newContent();
    ਲਈ (ਅਚਲ string& i : wholefile) flsp->contentp()->ਧੱਕੋ_ਪਾਠ(i);

    // Save contents for V3Control --contents
    ਜੇ (filename != V3Options::get_Std_Package_ਰਸਤਾ()) {
        ਤਾਰਕਿਕ ਸ਼ਾਮਲ_ਹੈ_Vlt = ਝੂਠ;
        ਲਈ (ਅਚਲ string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            ਜੇ (i.ਲੱਭੋ("`verilator_config") != string::npos) {
                ਸ਼ਾਮਲ_ਹੈ_Vlt = ਸੱਚ;
                ਰੋਕੋ;
            }
        }
        ਜੇ (!ਸ਼ਾਮਲ_ਹੈ_Vlt) {
            ਲਈ (ਅਚਲ string& i : wholefile) V3Control::contents_ਧੱਕੋ_ਪਾਠ(i);
        }
    }

    // Create new stream structure
    m_lexp->ਜਾਂਚ_New_File(flsp);
    ਜੋੜੋ_ਸਤਰ_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    ਪੂਰਨ eof_newline = 0;  // Number of characters following last newline
    ਪੂਰਨ eof_lineno = 1;
    ਲਈ (StrList::iterator it = wholefile.begin(); it != wholefile.ਸਿਰਾ(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        ਤਾਰਕਿਕ strip = ਝੂਠ;
        ਅਚਲ ਅੱਖਰ* ਅਚਲ sp = it->ਅੰਕੜੇ();
        ਅਚਲ ਅੱਖਰ* ਅਚਲ ep = sp + it->ਲੰਮਾਈ();
        // Only process if needed, as saves extra string allocations
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = sp; cp < ep; cp++) {
            ਜੇ (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } ਨਹੀਂਤਾਂ {
                ++eof_newline;
            }
        }
        ਜੇ (strip) {
            string ਬਾਹਰਲਾ;
            ਬਾਹਰਲਾ.reserve(it->ਲੰਮਾਈ());
            ਲਈ (ਅਚਲ ਅੱਖਰ* cp = sp; cp < ep; cp++) {
                ਜੇ (!(*cp == '\r' || *cp == '\0')) ਬਾਹਰਲਾ += *cp;
            }
            *it = ਬਾਹਰਲਾ;
        }

        // Push the data to an internal buffer.
        m_lexp->ਜਾਂਚ_ਬਾਈਟਾਂ_ਪਿੱਠ(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    ਜੇ (eof_newline) {
        File_ਸਤਰ* ਅਚਲ fl = ਨਵਾਂ File_ਸਤਰ{flsp};
        fl->contentLineno(eof_lineno);
        fl->ਕਾਲਮ_2(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_ਹੋਰ() << "... Suggest add newline.");
    }
}

ਖਾਲੀ V3PreProcImp::ਪਾਓ_Unreadback_At_Bol(ਅਚਲ string& ਪਾਠ) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    ਜੇ (m_ਸਤਰ_Cmt == "") {
        m_ਸਤਰ_Cmt_Nl = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (m_ਸਤਰ_Cmt[m_ਸਤਰ_Cmt.ਲੰਮਾਈ() - 1] != '\n') {
        ਪਾਓ_Unreadback("\n");
    }
    ਪਾਓ_Unreadback(ਪਾਠ);
}

ਖਾਲੀ V3PreProcImp::ਜੋੜੋ_ਸਤਰ_Comment(ਪੂਰਨ enterExit) {
    ਜੇ (ਸਤਰ_Directives()) {
        ਪਾਓ_Unreadback_At_Bol(m_lexp->curFilelinep()->ਸਤਰ_Directive_Strg(enterExit));
    }
}

ਖਾਲੀ V3PreProcImp::dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ(std::ostream& os) {
    ਲਈ (ਨਿਰਧਾਰਤ_ਕਰਦਾ_Map::const_iterator it = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.begin(); it != m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ(); ++it) {
        os << "`define " << it->ਪਹਿਲਾ;
        // No need to print "()" below as already part of params()
        ਜੇ (!it->ਦੂਜਾ.params().ਖਾਲੀ_ਥਾਵਾਂ()) os << it->ਦੂਜਾ.params();
        ਜੇ (!it->ਦੂਜਾ.ਮੁੱਲ().ਖਾਲੀ_ਥਾਵਾਂ()) os << " " << it->ਦੂਜਾ.ਮੁੱਲ();
        os << '\n';
    }
}

ਖਾਲੀ V3PreProcImp::candidate_ਨਿਰਧਾਰਤ_ਕਰਦਾ(VSpellCheck* spellerp) {
    ਲਈ (ਨਿਰਧਾਰਤ_ਕਰਦਾ_Map::const_iterator it = m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.begin(); it != m_ਨਿਰਧਾਰਤ_ਕਰਦਾ.ਸਿਰਾ(); ++it) {
        spellerp->ਧੱਕੋ_Candidate("`"s + it->ਪਹਿਲਾ);
    }
}

ਪੂਰਨ V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    ਜਦੋਂ (ਸੱਚ) {
    ਅਗਲਾ_tok:
        ਜੇ (m_ਸਤਰ_ਜੋੜੋ) {
            m_ਸਤਰ_ਜੋੜੋ--;
            m_rawAtBol = ਸੱਚ;
            yyourtext("\n", 1);
            ਜੇ (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ਵਾਪਸ VP_WHITE;
        }
        ਜੇ (m_ਸਤਰ_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ਸਥਿਰ string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_ਸਤਰ_Cmt;
            ਜੇ (m_ਸਤਰ_Cmt_Nl) {
                ਜੇ (!m_rawAtBol) s_rtncmt.ਪਾਓ(0, "\n");
                m_ਸਤਰ_Cmt_Nl = ਝੂਠ;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.ਲੰਮਾਈ());
            m_ਸਤਰ_Cmt = "";
            ਜੇ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            ਜੇ (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_ਮੁੱਲ(yyourtext(), yyourleng());
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (debug() >= 5) debugToken(VP_ਪਾਠ, "LCM");
                ਵਾਪਸ VP_ਪਾਠ;
            }
        }
        ਜੇ (isEof()) ਵਾਪਸ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->ਸ਼ੁਰੂ_Token();
        ਪੂਰਨ tok = m_lexp->lex();
        ਜੇ (debug() >= 5) debugToken(tok, "RAW");

        ਜੇ (m_ਆਖਰੀ_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_ਆਖਰੀ_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_ਟੋਕਨ_On_ਸਤਰ = 0;
        } ਨਹੀਂਤਾਂ ਜੇ (++m_ਟੋਕਨ_On_ਸਤਰ > v3Global.opt.preproc_Token_ਹੱਦਬੰਦੀ()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_ਮੰਜ਼ਿਲ_Str(v3Global.opt.preproc_Token_ਹੱਦਬੰਦੀ())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        ਜੇ (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            ਜੇ (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            ਜਾਓ ਅਗਲਾ_tok;
        }

        ਜੇ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ਵਾਪਸ tok;
    }
}

ਖਾਲੀ V3PreProcImp::debugToken(ਪੂਰਨ tok, ਅਚਲ ਅੱਖਰ* cmtp) {
    ਅਚਲ ਸਥਿਰ ਪੂਰਨ s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    ਜੇ (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = buf.ਲੱਭੋ('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        ਜਦੋਂ ((pos = buf.ਲੱਭੋ('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ਅਚਲ string flcol = m_lexp->m_tokFilelinep->ascii_ਸਤਰ_ਖਾਨਾ();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<ਪੂਰਨ>(m_ਹਾਲਤਾਂ.ਮਾਪ())
                       << ") dr" << m_defRefs.ਮਾਪ() << ":  <" << m_lexp->current_ਸ਼ੁਰੂ_State()
                       << ">" << tokenName(tok) << ": " << buf);
        ਜੇ (s_debugFileline >= 9) {
            std::ਬਾਹਰ << m_lexp->m_tokFilelinep->warnContextSecondary() << ਸਤਰ_ਅੰਤ;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

ਪੂਰਨ V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    ਜਦੋਂ (ਸੱਚ) {
    ਅਗਲਾ_tok:
        ਜੇ (isEof()) ਵਾਪਸ VP_EOF;
        ਪੂਰਨ tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        ਜੇ (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ਵਾਪਸ tok;
        ਜੇ (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_ਪਾਠ;
        ਜੇ (tok == VP_COMMENT) {
            ਜੇ (!m_off) {
                ਜੇ (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ਜੇ (m_ਸਤਰ_Cmt.ਖਾਲੀ_ਥਾਵਾਂ()) ਪਾਓ_Unreadback(" ");
                } ਨਹੀਂਤਾਂ ਜੇ (m_lexp->m_keepComments) {
                    ਵਾਪਸ tok;
                } ਨਹੀਂਤਾਂ {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ਪਾਓ_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            ਲਈ (ਮਾਪ_t len = 0; len < static_cast<ਮਾਪ_t>(yyourleng()); len++) {
                ਜੇ (yyourtext()[len] == '\n') m_ਸਤਰ_ਜੋੜੋ++;
            }
            ਜਾਓ ਅਗਲਾ_tok;
        }
        ਜੇ (tok == VP_ਸਤਰ) {
            ਜੋੜੋ_ਸਤਰ_Comment(m_lexp->m_enterExit);
            ਜਾਓ ਅਗਲਾ_tok;
        }

        ਜੇ (tok == VP_DEFREF_JOIN) {
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
            ਅਚਲ std::string name(yyourtext() + 1, yyourleng() - 1);
            ਜੇ (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = ਸੱਚ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } ਨਹੀਂਤਾਂ {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        ਜੇ (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            ਜੇ (tok == VP_JOIN) ਜਾਓ ਅਗਲਾ_tok;
            ਜੇ (tok == VP_ਚਿੰਨ੍ਹ_JOIN) tok = VP_ਚਿੰਨ੍ਹ;
        }
        ਜੇ (tok == VP_ਚਿੰਨ੍ਹ_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ਅਚਲ std::string ਬਾਹਰਲਾ(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << ਬਾਹਰਲਾ);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ਢੇਰ.ਧੱਕੋ(ਬਾਹਰਲਾ);
            state_ਧੱਕੋ(ps_JOIN);
            ਜਾਓ ਅਗਲਾ_tok;
        }

        // Deal with some special parser states
        ਚੋਣ (state()) {
        ਹਾਲਤ ps_ਸਿਖਰ: {
            ਰੋਕੋ;
        }
        ਹਾਲਤ ps_DEFNAME_UNDEF:  // FALLTHRU
        ਹਾਲਤ ps_DEFNAME_DEFINE:  // FALLTHRU
        ਹਾਲਤ ps_DEFNAME_IFDEF:  // FALLTHRU
        ਹਾਲਤ ps_DEFNAME_IFNDEF:  // FALLTHRU
        ਹਾਲਤ ps_DEFNAME_ELSIF: {
            ਜੇ (tok == VP_ਚਿੰਨ੍ਹ) {
                m_ਆਖਰੀ_Sym.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
                ਜੇ (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    ਤਾਰਕਿਕ enable = defExists(m_ਆਖਰੀ_Sym);
                    UINFO(4, "Ifdef " << m_ਆਖਰੀ_Sym << (enable ? " ON" : " OFF"));
                    ਜੇ (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ਢੇਰ.emplace(VPre_If_ਇੰਦਰਾਜ{enable, ਝੂਠ});
                    ਜੇ (!enable) parsingOff();
                    state_ਕੱਢੋ();
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ ਜੇ (state() == ps_DEFNAME_ELSIF) {
                    ਜੇ (m_ifdef_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } ਨਹੀਂਤਾਂ {
                        // Handle `else portion
                        ਅਚਲ VPre_If_ਇੰਦਰਾਜ ਆਖਰੀ_If = m_ifdef_ਢੇਰ.ਸਿਖਰ();
                        m_ifdef_ਢੇਰ.ਕੱਢੋ();
                        ਜੇ (!ਆਖਰੀ_If.on()) parsingOn();
                        // Handle `if portion
                        ਅਚਲ ਤਾਰਕਿਕ enable = !ਆਖਰੀ_If.everOn() && defExists(m_ਆਖਰੀ_Sym);
                        UINFO(4, "Elsif " << m_ਆਖਰੀ_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_ਢੇਰ.emplace(VPre_If_ਇੰਦਰਾਜ{enable, ਆਖਰੀ_If.everOn()});
                        ਜੇ (!enable) parsingOff();
                    }
                    state_ਕੱਢੋ();
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ ਜੇ (state() == ps_DEFNAME_UNDEF) {
                    ਜੇ (!m_off) {
                        UINFO(4, "Undef " << m_ਆਖਰੀ_Sym);
                        undef(m_ਆਖਰੀ_Sym);
                    }
                    state_ਕੱਢੋ();
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ ਜੇ (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ਧੱਕੋ_State_Def_Form();
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ) {
                // IE, something like comment between define and symbol
                ਜੇ (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ਧੱਕੋ_State_ਵਿਅੰਜਕ();
                    m_ਵਿਅੰਜਕ_Parser.reset(fileline());
                    m_ਵਿਅੰਜਕ_Paren_Level = 1;
                    ਚੋਣ (state()) {
                    ਹਾਲਤ ps_DEFNAME_IFDEF: stateChange(ps_ਵਿਅੰਜਕ_IFDEF); ਰੋਕੋ;
                    ਹਾਲਤ ps_DEFNAME_IFNDEF: stateChange(ps_ਵਿਅੰਜਕ_IFNDEF); ਰੋਕੋ;
                    ਹਾਲਤ ps_DEFNAME_ELSIF: stateChange(ps_ਵਿਅੰਜਕ_ELSIF); ਰੋਕੋ;
                    ਮੂਲ: v3fatalSrc("bad case");
                    }
                    ਜਾਓ ਅਗਲਾ_tok;
                }
                ਜੇ (!m_off) {
                    ਵਾਪਸ tok;
                } ਨਹੀਂਤਾਂ {
                    ਜਾਓ ਅਗਲਾ_tok;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                ਰੋਕੋ;
            } ਨਹੀਂਤਾਂ {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ ps_ਵਿਅੰਜਕ_IFDEF:  // FALLTHRU
        ਹਾਲਤ ps_ਵਿਅੰਜਕ_IFNDEF:  // FALLTHRU
        ਹਾਲਤ ps_ਵਿਅੰਜਕ_ELSIF: {
            // `ifdef ( *here*
            File_ਸਤਰ* ਅਚਲ flp = m_lexp->m_tokFilelinep;
            ਜੇ (tok == VP_ਚਿੰਨ੍ਹ) {
                m_ਆਖਰੀ_Sym.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
                ਅਚਲ ਤਾਰਕਿਕ exists = defExists(m_ਆਖਰੀ_Sym);
                ਜੇ (exists) {
                    ਅਚਲ std::string ਮੁੱਲ = def_ਮੁੱਲ(m_ਆਖਰੀ_Sym);
                    ਜੇ (VString::removeWhitespace(ਮੁੱਲ) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_ਆਖਰੀ_Sym << "' with value '" << ਮੁੱਲ
                                << "'\n"
                                   "... Suggest change define '"
                                << m_ਆਖਰੀ_Sym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, exists});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_WHITE) {
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::BRA});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_ਵਿਅੰਜਕ_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                ਜੇ (--m_ਵਿਅੰਜਕ_Paren_Level > 0) {
                    m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::KET});
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ {
                    // Done with parsing expression
                    ਤਾਰਕਿਕ enable = m_ਵਿਅੰਜਕ_Parser.ਨਤੀਜਾ();
                    UINFO(4, "ifdef() result=" << enable);
                    ਜੇ (state() == ps_ਵਿਅੰਜਕ_IFDEF || state() == ps_ਵਿਅੰਜਕ_IFNDEF) {
                        ਜੇ (state() == ps_ਵਿਅੰਜਕ_IFNDEF) enable = !enable;
                        m_ifdef_ਢੇਰ.emplace(VPre_If_ਇੰਦਰਾਜ{enable, ਝੂਠ});
                        ਜੇ (!enable) parsingOff();
                        state_ਕੱਢੋ();
                        ਜਾਓ ਅਗਲਾ_tok;
                    } ਨਹੀਂਤਾਂ ਜੇ (state() == ps_ਵਿਅੰਜਕ_ELSIF) {
                        ਜੇ (m_ifdef_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } ਨਹੀਂਤਾਂ {
                            // Handle `else portion
                            ਅਚਲ VPre_If_ਇੰਦਰਾਜ ਆਖਰੀ_If = m_ifdef_ਢੇਰ.ਸਿਖਰ();
                            m_ifdef_ਢੇਰ.ਕੱਢੋ();
                            ਜੇ (!ਆਖਰੀ_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !ਆਖਰੀ_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_ਆਖਰੀ_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_ਢੇਰ.emplace(VPre_If_ਇੰਦਰਾਜ{enable, ਆਖਰੀ_If.everOn()});
                            ਜੇ (!enable) parsingOff();
                        }
                        state_ਕੱਢੋ();
                    }
                    ਜਾਓ ਅਗਲਾ_tok;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::LNOT});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 2 && 0 == ਲੜੀ_ਤੁਲਨਾ(yyourtext(), "&&", 2)) {
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::LAND});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 2 && 0 == ਲੜੀ_ਤੁਲਨਾ(yyourtext(), "||", 2)) {
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::LOR});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 2 && 0 == ਲੜੀ_ਤੁਲਨਾ(yyourtext(), "->", 2)) {
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::IMP});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 3 && 0 == ਲੜੀ_ਤੁਲਨਾ(yyourtext(), "<->", 3)) {
                m_ਵਿਅੰਜਕ_Parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::EQV});
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (VString::removeWhitespace(string{yyourtext(), yyourleng()}).ਖਾਲੀ_ਥਾਵਾਂ()) {
                    ਵਾਪਸ tok;
                } ਨਹੀਂਤਾਂ {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ ps_DEFFORM: {
            ਜੇ (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_ਮੁੱਲ;
                ਜੇ (debug() >= 5) {
                    ਬਾਹਰ << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ਧੱਕੋ_State_Def_ਮੁੱਲ();
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ) {
                // IE, something like comment in formals
                ਜੇ (!m_off) ਵਾਪਸ tok;
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ ps_DEFVALUE: {
            ਸਥਿਰ string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            ਜੇ (tok == VP_DEFVALUE) {
                ਜੇ (debug() >= 5) {  // LCOV_EXCL_START
                    ਬਾਹਰ << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_ਮੁੱਲ)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ਅਚਲ string formals = m_formals;
                string ਮੁੱਲ = m_lexp->m_def_ਮੁੱਲ;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                ਲਈ (ਮਾਪ_t i = 0; i < formals.ਲੰਮਾਈ(); i++) {
                    ਜੇ (formals[i] == '\n') s_newlines += "\n";
                }
                ਲਈ (ਮਾਪ_t i = 0; i < ਮੁੱਲ.ਲੰਮਾਈ(); i++) {
                    ਜੇ (ਮੁੱਲ[i] == '\n') s_newlines += "\n";
                }
                ਜੇ (!m_off) {
                    // Remove leading and trailing whitespace
                    ਮੁੱਲ = trimWhitespace(ਮੁੱਲ, ਸੱਚ);
                    // Define it
                    UINFO(4, "Define " << m_ਆਖਰੀ_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(ਮੁੱਲ) << "'");
                    define(fileline(), m_ਆਖਰੀ_Sym, ਮੁੱਲ, formals, ਝੂਠ);
                }
            } ਨਹੀਂਤਾਂ {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_ਕੱਢੋ();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.ਲੰਮਾਈ());
            ਵਾਪਸ VP_WHITE;
        }
        ਹਾਲਤ ps_DEFPAREN: {
            ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                UASSERT(!m_defRefs.ਖਾਲੀ_ਥਾਵਾਂ(), "Shouldn't be in DEFPAREN w/o active defref");
                ਅਚਲ VDefineRef* ਅਚਲ refp = &(m_defRefs.ਸਿਖਰ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ ps_DEFARG: {
            UASSERT(!m_defRefs.ਖਾਲੀ_ਥਾਵਾਂ(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.ਸਿਖਰ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_ਮੁੱਲ);
            m_lexp->m_def_ਮੁੱਲ = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            ਜੇ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->ਦਲੀਲਾਂ().ਧੱਕੋ_ਪਿੱਠ(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ਧੱਕੋ_State_Def_Arg(1);
                refp->nextarg("");
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->ਦਲੀਲਾਂ().ਧੱਕੋ_ਪਿੱਠ(refp->nextarg());
                string ਬਾਹਰਲਾ;
                ਜੇ (!m_off) {
                    ਬਾਹਰਲਾ = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.ਕੱਢੋ(), refp);
                state_ਕੱਢੋ();
                ਜੇ (m_defRefs.ਖਾਲੀ_ਥਾਵਾਂ() || state() == ps_STRIFY || state() == ps_JOIN) {
                    ਜੇ (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ(), "`` join stack empty, but in a ``");
                        ਅਚਲ string lhs = m_join_ਢੇਰ.ਸਿਖਰ();
                        m_join_ਢੇਰ.ਕੱਢੋ();
                        ਬਾਹਰਲਾ.ਪਾਓ(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << ਬਾਹਰਲਾ);
                        state_ਕੱਢੋ();
                    }
                    ਜੇ (!m_off) {
                        unputDefrefString(ਬਾਹਰਲਾ);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    ਨਹੀਂਤਾਂ ਜੇ (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.ਖਾਲੀ_ਥਾਵਾਂ() ? 0 : m_defRefs.ਸਿਖਰ().parenLevel();
                } ਨਹੀਂਤਾਂ {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.ਸਿਖਰ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_ਮੁੱਲ + ਬਾਹਰਲਾ);
                    m_lexp->m_def_ਮੁੱਲ = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                ਰੋਕੋ;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਚਿੰਨ੍ਹ || tok == VP_STRING || tok == VP_ਪਾਠ || tok == VP_WHITE) {
                string rtn;
                rtn.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ਧੱਕੋ(ps_STRIFY);
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ ps_INCNAME: {
            ਜੇ (tok == VP_STRING) {
                state_ਕੱਢੋ();
                m_ਆਖਰੀ_Sym.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_ਆਖਰੀ_Sym);
                // Drop leading and trailing quotes.
                m_ਆਖਰੀ_Sym.erase(0, 1);
                m_ਆਖਰੀ_Sym.erase(m_ਆਖਰੀ_Sym.ਲੰਮਾਈ() - 1, 1);
                include(m_ਆਖਰੀ_Sym);
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_ਪਾਠ && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ਧੱਕੋ_State_Inc_Filename();
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                ਰੋਕੋ;
            } ਨਹੀਂਤਾਂ {
                state_ਕੱਢੋ();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                ਜਾਓ ਅਗਲਾ_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        ਹਾਲਤ ps_ERRORNAME: {
            ਜੇ (tok == VP_STRING) {
                ਜੇ (!m_off) {
                    m_ਆਖਰੀ_Sym.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_ਆਖਰੀ_Sym);
                }
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ ps_JOIN: {
            ਜੇ (tok == VP_ਚਿੰਨ੍ਹ || tok == VP_ਪਾਠ || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ(), "`` join stack empty, but in a ``");
                ਅਚਲ string lhs = m_join_ਢੇਰ.ਸਿਖਰ();
                m_join_ਢੇਰ.ਕੱਢੋ();
                UINFO(5, "`` LHS:" << lhs);
                ਅਚਲ string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ਅਚਲ string ਬਾਹਰਲਾ = lhs + rhs;
                UINFO(5, "`` Out:" << ਬਾਹਰਲਾ);
                unputString(ਬਾਹਰਲਾ);
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ਅਚਲ string lhs = m_join_ਢੇਰ.ਸਿਖਰ();
                m_join_ਢੇਰ.ਕੱਢੋ();
                unputString(lhs);
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ {
                // `define, etc, fall through and expand.  Pop back here.
                ਰੋਕੋ;
            }
        }
        ਹਾਲਤ ps_STRIFY: {
            ਜੇ (tok == VP_STRIFY) {
                // Quote what's in the middle of the stringification
                // Note a `" MACRO_WITH(`") `" doesn't need to be handled (we don't need a stack)
                // That behavior isn't specified, and other simulators vary widely
                string ਬਾਹਰਲਾ = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(ਬਾਹਰਲਾ.begin(), ਬਾਹਰਲਾ.ਸਿਰਾ(), '\n', ' ');
                unputString("\""s + ਬਾਹਰਲਾ + "\"");
                state_ਕੱਢੋ();
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                ਰੋਕੋ;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                ਜਾਓ ਅਗਲਾ_tok;
            } ਨਹੀਂਤਾਂ ਜੇ (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                ਰੋਕੋ;
            } ਨਹੀਂਤਾਂ {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਮੂਲ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        ਚੋਣ (tok) {
        ਹਾਲਤ VP_INCLUDE:
            ਜੇ (!m_off) {
                state_ਧੱਕੋ(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_UNDEF: state_ਧੱਕੋ(ps_DEFNAME_UNDEF); ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ਧੱਕੋ(ps_DEFNAME_DEFINE);
            ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_IFDEF: state_ਧੱਕੋ(ps_DEFNAME_IFDEF); ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_IFNDEF: state_ਧੱਕੋ(ps_DEFNAME_IFNDEF); ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_ELSIF: state_ਧੱਕੋ(ps_DEFNAME_ELSIF); ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_ELSE:
            ਜੇ (m_ifdef_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                fileline()->v3error("`else with no matching `if");
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ VPre_If_ਇੰਦਰਾਜ ਆਖਰੀ_If = m_ifdef_ਢੇਰ.ਸਿਖਰ();
                m_ifdef_ਢੇਰ.ਕੱਢੋ();
                ਅਚਲ ਤਾਰਕਿਕ enable = !ਆਖਰੀ_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ਢੇਰ.emplace(VPre_If_ਇੰਦਰਾਜ{enable, ਆਖਰੀ_If.everOn()});
                ਜੇ (!ਆਖਰੀ_If.on()) parsingOn();
                ਜੇ (!enable) parsingOff();
            }
            ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_ENDIF:
            UINFO(4, "Endif ");
            ਜੇ (m_ifdef_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
                fileline()->v3error("`endif with no matching `if");
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ VPre_If_ਇੰਦਰਾਜ ਆਖਰੀ_If = m_ifdef_ਢੇਰ.ਸਿਖਰ();
                m_ifdef_ਢੇਰ.ਕੱਢੋ();
                ਜੇ (!ਆਖਰੀ_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            ਜਾਓ ਅਗਲਾ_tok;

        ਹਾਲਤ VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ਅਚਲ string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            ਜੇ (m_defPutJoin) {
                m_defPutJoin = ਝੂਠ;
                unputString("``");
            }
            ਜੇ (m_def_ਡੂੰਘਾਈ++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                ਜਾਓ ਅਗਲਾ_tok;
            }
            // Substitute
            ਜੇ (!defExists(name)) {  // Not found, return original string as-is
                m_def_ਡੂੰਘਾਈ = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                ਜੇ (m_off) {
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    ਜਾਓ ਅਗਲਾ_tok;
                }
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ string params = defParams(name);
                ਜੇ (params == "0" || params == "") {  // Found, as simple substitution
                    string ਬਾਹਰਲਾ;
                    ਜੇ (!m_off) {
                        VDefineRef tempref{name, ""};
                        ਬਾਹਰਲਾ = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    ਜੇ (m_defRefs.ਖਾਲੀ_ਥਾਵਾਂ() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        ਜੇ (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ(), "`` join stack empty, but in a ``");
                            ਅਚਲ string lhs = m_join_ਢੇਰ.ਸਿਖਰ();
                            m_join_ਢੇਰ.ਕੱਢੋ();
                            ਬਾਹਰਲਾ.ਪਾਓ(0, lhs);
                            UINFO(5, "``-end-defref Out:" << ਬਾਹਰਲਾ);
                            state_ਕੱਢੋ();
                        }
                        ਜੇ (!m_off) {
                            unputDefrefString(ਬਾਹਰਲਾ);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        ਨਹੀਂਤਾਂ ਜੇ (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } ਨਹੀਂਤਾਂ {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ਅਚਲ refp = &(m_defRefs.ਸਿਖਰ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_ਮੁੱਲ + ਬਾਹਰਲਾ);
                        m_lexp->m_def_ਮੁੱਲ = "";
                    }
                    ਜਾਓ ਅਗਲਾ_tok;
                } ਨਹੀਂਤਾਂ {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    ਜੇ (!m_defRefs.ਖਾਲੀ_ਥਾਵਾਂ()) m_defRefs.ਸਿਖਰ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ਧੱਕੋ(ps_DEFPAREN);
                    m_lexp->ਧੱਕੋ_State_Def_Arg(0);
                    ਜਾਓ ਅਗਲਾ_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            ਜਾਓ ਅਗਲਾ_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        ਹਾਲਤ VP_ERROR: {
            state_ਧੱਕੋ(ps_ERRORNAME);
            ਜਾਓ ਅਗਲਾ_tok;
        }
        ਹਾਲਤ VP_EOF:
            ਜੇ (!m_ifdef_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) fileline()->v3error("`ifdef not terminated at EOF");
            ਵਾਪਸ tok;
        ਹਾਲਤ VP_UNDEFINEALL:
            ਜੇ (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ਧੱਕੋ(ps_STRIFY);
            ਜਾਓ ਅਗਲਾ_tok;
        ਹਾਲਤ VP_ਚਿੰਨ੍ਹ:
        ਹਾਲਤ VP_STRING:
        ਹਾਲਤ VP_ਪਾਠ: {
            m_def_ਡੂੰਘਾਈ = 0;
            ਜੇ (!m_off) {
                ਵਾਪਸ tok;
            } ਨਹੀਂਤਾਂ {
                ਜਾਓ ਅਗਲਾ_tok;
            }
        }
        ਹਾਲਤ VP_WHITE:  // Handled at top of loop
        ਹਾਲਤ VP_COMMENT:  // Handled at top of loop
        ਹਾਲਤ VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        ਹਾਲਤ VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        ਮੂਲ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            ਰੋਕੋ;  // LCOV_EXCL_LINE
        }
        ਵਾਪਸ tok;
    }
}

ਪੂਰਨ V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    ਜੇ (!m_finAhead) {
        m_finAhead = ਸੱਚ;
        m_finToken = getStateToken();
        m_finBuf.ਸੌਂਪੋ_2(yyourtext(), yyourleng());
    }
    ਅਚਲ ਪੂਰਨ tok = m_finToken;
    buf = m_finBuf;
    ਜੇ (ਝੂਠ && debug() >= 5) {
        ਅਚਲ string bufcln = V3PreLex::cleanDbgStrg(buf);
        ਅਚਲ string flcol = m_lexp->m_tokFilelinep->ascii_ਸਤਰ_ਖਾਨਾ();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ਅਚਲ ਅੱਖਰ* bufp = buf.c_str();
    ਜਦੋਂ (*bufp == '\n') bufp++;
    ਜੇ ((tok == VP_ਪਾਠ || tok == VP_ਸਤਰ) && VString::startsWith(bufp, "`line ")) {
        ਪੂਰਨ enter;
        m_finFilelinep->ਸਤਰ_Directive(bufp, enter /*ref*/);
    } ਨਹੀਂਤਾਂ {
        ਜੇ (m_finAtBol && !(tok == VP_ਪਾਠ && buf == "\n") && m_preprocp->ਸਤਰ_Directives()) {
            ਜੇ (ਅਚਲ ਪੂਰਨ ਬਾਹਰਲਾ_Behind
                = (m_lexp->m_tokFilelinep->ਆਖਰੀ_Lineno() - m_finFilelinep->ਆਖਰੀ_Lineno())) {
                ਜੇ (debug() >= 5) {
                    ਅਚਲ string flcol = m_lexp->m_tokFilelinep->ascii_ਸਤਰ_ਖਾਨਾ();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->ਆਖਰੀ_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->ਆਖਰੀ_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->ਆਖਰੀ_Lineno());
                ਜੇ (ਬਾਹਰਲਾ_Behind > 0
                    && (ਬਾਹਰਲਾ_Behind <= static_cast<ਪੂਰਨ>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    ਜੇ (m_preprocp->keepWhitespace()) {
                        buf = std::string(ਬਾਹਰਲਾ_Behind, '\n');  // () for char repeat
                        ਵਾਪਸ VP_ਪਾਠ;
                    }
                } ਨਹੀਂਤਾਂ {
                    // Need to backup, use `line
                    buf = m_finFilelinep->ਸਤਰ_Directive_Strg(0);
                    ਵਾਪਸ VP_ਸਤਰ;
                }
            }
        }
        // Track newlines in prep for next token
        ਲਈ (ਅਚਲ ਅੱਖਰ& c : buf) {
            ਜੇ (c == '\n') {
                m_finAtBol = ਸੱਚ;
                m_finFilelinep->linenoInc();
            } ਨਹੀਂਤਾਂ {
                m_finAtBol = ਝੂਠ;
            }
        }
    }
    m_finAhead = ਝੂਠ;  // Consumed the token
    ਵਾਪਸ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    ਜੇ (isEof()) ਵਾਪਸ "";
    ਅਚਲ ਅੱਖਰ* rtnp;
    ਤਾਰਕਿਕ gotEof = ਝੂਠ;
    ਜਦੋਂ (ਖਾਲੀ_ਸੂਚਕ == (rtnp = std::strchr(m_ਸਤਰ_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ਅਚਲ ਪੂਰਨ tok = getFinalToken(m_tokenBuf /*ref*/);
        ਜੇ (debug() >= 5) {
            ਅਚਲ string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ਅਚਲ string flcol = m_lexp->m_tokFilelinep->ascii_ਸਤਰ_ਖਾਨਾ();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        ਜੇ (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            ਜੇ (m_ਸਤਰ_Chars != "" && m_ਸਤਰ_Chars[m_ਸਤਰ_Chars.ਲੰਮਾਈ() - 1] != '\n') {
                m_ਸਤਰ_Chars.append("\n");
            }
            gotEof = ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            m_ਸਤਰ_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ਅਚਲ ਪੂਰਨ len = rtnp - m_ਸਤਰ_Chars.c_str() + 1;
    string the_ਸਤਰ(m_ਸਤਰ_Chars, 0, len);
    m_ਸਤਰ_Chars = m_ਸਤਰ_Chars.erase(0, len);  // Remove returned characters
    ਜੇ (debug() >= 4) {
        ਅਚਲ string lncln = V3PreLex::cleanDbgStrg(the_ਸਤਰ);
        ਅਚਲ string flcol = m_lexp->m_tokFilelinep->ascii_ਸਤਰ_ਖਾਨਾ();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ਵਾਪਸ the_ਸਤਰ;
}
