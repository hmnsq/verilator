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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3PreProc.h"

#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3LanguageWords.h"
#અન્તર્ભાવ "V3PreExpr.h"
#અન્તર્ભાવ "V3PreLex.h"
#અન્તર્ભાવ "V3PreShell.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cstdlib>
#અન્તર્ભાવ <fstream>
#અન્તર્ભાવ <stack>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#અન્તર્ભાવ "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

વર્ગ VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_લીટી* અચલ m_fileline;  // Where it was declared
    અચલ string m_કિંમત;  // Value of define
    અચલ string m_params;  // Parameters
    અચલ તાર્કિક m_cmdline;  // Set on command line, don't `undefineall
જાહેર:
    VDefine(File_લીટી* fl, અચલ string& કિંમત, અચલ string& params, તાર્કિક cmdline)
        : m_fileline{fl}
        , m_કિંમત{કિંમત}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_લીટી* fileline() અચલ { પાછા m_fileline; }
    string કિંમત() અચલ { પાછા m_કિંમત; }
    string params() અચલ { પાછા m_params; }
    તાર્કિક cmdline() અચલ { પાછા m_cmdline; }
};

//*************************************************************************

વર્ગ VDefineRef final {
    // One for each pending define substitution
    અચલ string m_name;  // Define last name being defined
    અચલ string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    પૂર્ણાંક m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
જાહેર:
    string name() અચલ { પાછા m_name; }
    string params() અચલ { પાછા m_params; }
    string nextarg() અચલ { પાછા m_nextarg; }
    શૂન્ય nextarg(અચલ string& કિંમત) { m_nextarg = કિંમત; }
    પૂર્ણાંક parenLevel() અચલ { પાછા m_parenLevel; }
    શૂન્ય parenLevel(પૂર્ણાંક કિંમત) { m_parenLevel = કિંમત; }
    std::vector<string>& args() { પાછા m_args; }
    VDefineRef(અચલ string& name, અચલ string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = મૂળભૂત;
};

//*************************************************************************
/// Data for parsing on/off

વર્ગ VPreIfEntry final {
    // One for each pending ifdef/ifndef
    અચલ તાર્કિક m_on;  // Current parse for this ifdef level is "on"
    અચલ તાર્કિક m_everOn;  // Some if term in elsif tree has been on
જાહેર:
    તાર્કિક on() અચલ { પાછા m_on; }
    તાર્કિક everOn() અચલ { પાછા m_everOn; }
    VPreIfEntry(તાર્કિક on, તાર્કિક everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = મૂળભૂત;
};

//*************************************************************************
// Data for a preprocessor instantiation.

વર્ગ V3PreProcImp final : જાહેર V3PreProc {
જાહેર:
    // TYPES
    using DefinesMap = std::map<અચલ std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    અચલ V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    પૂર્ણાંક m_lastLineno = 0;  // Last line number (stall detection)
    પૂર્ણાંક m_tokens_On_લીટી = 0;  // Number of tokens on line (stall detection)

    ગણના ProcState : uint8_t {
        ps_ટોચ,
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
        ps_પદાવલિ_IFDEF,
        ps_પદાવલિ_IFNDEF,
        ps_પદાવલિ_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    સ્થિર અચલ અક્ષર* procStateName(ProcState s) {
        સ્થિર અચલ અક્ષર* અચલ states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        પાછા states[s];
    }

    std::ઢગલો<ProcState> m_states;  ///< Current state of parser
    પૂર્ણાંક m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    તાર્કિક m_incError = ખોટું;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_લીટી_Cmt;  ///< Line comment(s) to be returned
    તાર્કિક m_લીટી_Cmt_Nl = ખોટું;  ///< Newline needed before inserting lineCmt
    પૂર્ણાંક m_લીટી_Add = 0;  ///< Empty lines to return to maintain line count
    તાર્કિક m_rawAtBol = સાચું;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    તાર્કિક m_finAhead = ખોટું;  ///< Have read a token ahead
    પૂર્ણાંક m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    તાર્કિક m_finAtBol = સાચું;  ///< Last getFinalToken left us at beginning of line
    File_લીટી* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ઢગલો<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ઢગલો<VPreIfEntry> m_ifdef_ઢગલો;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    તાર્કિક m_defPutJoin = ખોટું;  ///< Insert `` after substitution

    // For `` join
    std::ઢગલો<string> m_join_ઢગલો;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_પદાવલિ m_પદાવલિ_Parser;  ///< Parser for () expression
    પૂર્ણાંક m_પદાવલિ_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_લીટી_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    શૂન્ય v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    સ્થિર અચલ અક્ષર* tokenName(પૂર્ણાંક tok);
    શૂન્ય debugToken(પૂર્ણાંક tok, અચલ અક્ષર* cmtp);
    શૂન્ય parse_ટોચ();
    શૂન્ય parseUndef();

ખાનગી:
    // Internal methods
    શૂન્ય endOfOneFile();
    string defineSubst(VDefineRef* refp);

    તાર્કિક defExists(અચલ string& name);
    તાર્કિક defCmdline(અચલ string& name);
    string def_કિંમત(અચલ string& name);
    string defParams(અચલ string& name);
    File_લીટી* defFileline(અચલ string& name);

    string commentCleanup(અચલ string& text);
    તાર્કિક commentTokenMatch(string& cmdr, અચલ અક્ષર* strg);
    સ્થિર string trimWhitespace(અચલ string& strg, તાર્કિક trailing);
    શૂન્ય unputString(અચલ string& strg);
    શૂન્ય unputDefrefString(અચલ string& strg);

    શૂન્ય parsingOn() {
        m_off--;
        જો (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    શૂન્ય parsingOff() { m_off++; }

    પૂર્ણાંક getRawToken();
    પૂર્ણાંક getStateToken();
    પૂર્ણાંક getFinalToken(string& buf);

    ProcState state() અચલ { પાછા m_states.ટોચ(); }
    તાર્કિક stateIsDefname() અચલ {
        પાછા state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    શૂન્ય state_ધકેલો(ProcState state) { m_states.ધકેલો(state); }
    શૂન્ય state_કાઢો() {
        m_states.કાઢો();
        જો (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.ધકેલો(ps_ટોચ);  // LCOV_EXCL_LINE
        }
    }
    શૂન્ય stateChange(ProcState state) {
        state_કાઢો();
        state_ધકેલો(state);
    }

જાહેર:
    // METHODS, called from upper level shell
    શૂન્ય openFile(File_લીટી* fl, VInFilter* filterp, અચલ string& filename) override;
    તાર્કિક isEof() અચલ override { પાછા m_lexp->curStreamp()->m_eof; }
    શૂન્ય forceEof() { m_lexp->curStreamp()->m_eof = સાચું; }
    string getline() override;
    શૂન્ય ઉમેરો_Unreadback(અચલ string& text) override { m_લીટી_Cmt += text; }
    શૂન્ય ઉમેરો_Unreadback_At_Bol(અચલ string& text);
    શૂન્ય add_લીટી_Comment(પૂર્ણાંક enterExit);
    શૂન્ય dumpDefines(std::ostream& os) override;
    શૂન્ય candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    શૂન્ય comment(અચલ string& text) override;  // Comment detected (if keepComments==2)
    શૂન્ય include(અચલ string& filename) override;  // Request a include file be processed
    શૂન્ય undef(અચલ string& name) override;
    આભાસી શૂન્ય undefineall();
    શૂન્ય define(File_લીટી* fl, અચલ string& name, અચલ string& કિંમત, અચલ string& params,
                તાર્કિક cmdline) override;
    string removeDefines(અચલ string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.ધકેલો(ps_ટોચ); }
    // cppcheck-suppress duplInheritedMember
    શૂન્ય configure(File_લીટી* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = આ;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = નવું File_લીટી{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = નવું V3PreLex{આ, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        જો (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_લીટી* fl) {
    V3PreProcImp* અચલ preprocp = નવું V3PreProcImp;
    preprocp->configure(fl);
    પાછા preprocp;
}

શૂન્ય V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_પદાવલિ::selfTest(); }

//*************************************************************************
// Defines

શૂન્ય V3PreProcImp::undef(અચલ string& name) {
    જો (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ઉમેરો_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
શૂન્ય V3PreProcImp::undefineall() {
    જો (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ઉમેરો_Unreadback("`undefineall\n");
    માટે (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        જો (!it->second.cmdline()) m_defines.erase(it);
    }
}
તાર્કિક V3PreProcImp::defExists(અચલ string& name) {
    અચલ auto iter = m_defines.find(name);
    પાછા (iter != m_defines.end());
}
તાર્કિક V3PreProcImp::defCmdline(અચલ string& name) {
    અચલ auto iter = m_defines.find(name);
    જો (iter == m_defines.end()) { પાછા ખોટું; }
    પાછા iter->second.cmdline();
}
string V3PreProcImp::def_કિંમત(અચલ string& name) {
    અચલ auto iter = m_defines.find(name);
    જો (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        પાછા "";
    }
    પાછા iter->second.કિંમત();
}
string V3PreProcImp::defParams(અચલ string& name) {
    અચલ auto iter = m_defines.find(name);
    જો (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        પાછા "";
    }
    પાછા iter->second.params();
}
File_લીટી* V3PreProcImp::defFileline(અચલ string& name) {
    અચલ auto iter = m_defines.find(name);
    જો (iter == m_defines.end()) પાછા nullptr;
    પાછા iter->second.fileline();
}
શૂન્ય V3PreProcImp::define(File_લીટી* fl, અચલ string& name, અચલ string& કિંમત,
                          અચલ string& params, તાર્કિક cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << કિંમત << "' params '" << params << "'");
    જો (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(કિંમત.size());
        માટે (string::size_type pos = 0; pos < કિંમત.size(); ++pos) {
            જો (કિંમત[pos] == '\n' && (pos == 0 || કિંમત[pos - 1] != '\\')) out += '\\';
            out += કિંમત[pos];
        }
        ઉમેરો_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    જો (!V3_Language_શબ્દો::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } નહીંતર {
        જો (defExists(name)) {
            જો (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << કિંમત
                               << "' to existing command line define value: '" << def_કિંમત(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_કિંમત(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                પાછા;
            } નહીંતર {
                જો (!(def_કિંમત(name) == કિંમત
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << કિંમત
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_કિંમત(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, કિંમત, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(અચલ string& text) {
    string val;
    string rtnsym = text;
    માટે (પૂર્ણાંક loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        જો (xsym[0] == '`') xsym.erase(0, 1);
        જો (defExists(xsym)) {
            val = def_કિંમત(xsym);
            જો (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } નહીંતર {
                તોડો;
            }
        } નહીંતર {
            તોડો;
        }
    }
    પાછા rtnsym;  // NA
}

//**********************************************************************
// Comments

શૂન્ય V3PreProcImp::include(અચલ string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(અચલ string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    જ્યારે ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    જ્યારે ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    જ્યારે ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    જ્યારે ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    જ્યારે ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    જ્યારે ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    જ્યારે (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    પાછા cmd;
}

તાર્કિક V3PreProcImp::commentTokenMatch(string& cmdr, અચલ અક્ષર* strg) {
    પૂર્ણાંક len = std::strlen(strg);
    જો (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        જો (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        પાછા સાચું;
    } નહીંતર {
        પાછા ખોટું;
    }
}

શૂન્ય V3PreProcImp::comment(અચલ string& text) {
    // Comment detected.  Only keep relevant data.
    તાર્કિક printed = ખોટું;
    જો (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ઉમેરો_Unreadback(text);
        printed = સાચું;
    }

    અચલ અક્ષર* cp = text.c_str();
    જો (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } નહીંતર {
        પાછા;
    }

    જ્યારે (std::isspace(*cp)) ++cp;

    તાર્કિક synth = ખોટું;
    તાર્કિક vlcomment = ખોટું;
    જો ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        જો (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            પાછા;
        }
        vlcomment = સાચું;
    } નહીંતર જો (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        જો (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            પાછા;
        }
        synth = સાચું;
    } નહીંતર જો (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = સાચું;
    } નહીંતર જો (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = સાચું;
    } નહીંતર જો (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = સાચું;
    } નહીંતર {
        પાછા;
    }

    જ્યારે (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    જો (synth) {
        જો (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            જો (commentTokenMatch(cmd /*ref*/, "full_case")) {
                જો (!printed) ઉમેરો_Unreadback("/*verilator full_case*/");
            }
            જો (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                જો (!printed) ઉમેરો_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } નહીંતર જો (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        જો (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            જ્યારે (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            જ્યારે (std::isspace(arg[0])) arg = arg.substr(1);
            જો (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            જો (!printed) ઉમેરો_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } નહીંતર જો (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            જ્યારે (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            જો (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                પાછા;
            }
            જો (!printed) ઉમેરો_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } નહીંતર {
            જો (!printed) ઉમેરો_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_લીટી* V3PreProc::fileline() {
    અચલ V3PreProcImp* idatap = static_cast<V3PreProcImp*>(આ);
    પાછા idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

અચલ અક્ષર* V3PreProcImp::tokenName(પૂર્ણાંક tok) {
    પસંદગી (tok) {
    સ્થિતિ VP_BACKQUOTE: પાછા ("BACKQUOTE");
    સ્થિતિ VP_COMMENT: પાછા ("COMMENT");
    સ્થિતિ VP_DEFARG: પાછા ("DEFARG");
    સ્થિતિ VP_DEFFORM: પાછા ("DEFFORM");
    સ્થિતિ VP_DEFINE: પાછા ("DEFINE");
    સ્થિતિ VP_DEFREF: પાછા ("DEFREF");
    સ્થિતિ VP_DEFREF_JOIN: પાછા ("DEFREF_JOIN");
    સ્થિતિ VP_DEFVALUE: પાછા ("DEFVALUE");
    સ્થિતિ VP_ELSE: પાછા ("ELSE");
    સ્થિતિ VP_ELSIF: પાછા ("ELSIF");
    સ્થિતિ VP_ENDIF: પાછા ("ENDIF");
    સ્થિતિ VP_EOF: પાછા ("EOF");
    સ્થિતિ VP_EOF_ERROR: પાછા ("EOF_ERROR");
    સ્થિતિ VP_ERROR: પાછા ("ERROR");
    સ્થિતિ VP_IFDEF: પાછા ("IFDEF");
    સ્થિતિ VP_IFNDEF: પાછા ("IFNDEF");
    સ્થિતિ VP_JOIN: પાછા ("JOIN");
    સ્થિતિ VP_INCLUDE: પાછા ("INCLUDE");
    સ્થિતિ VP_લીટી: પાછા ("LINE");
    સ્થિતિ VP_STRIFY: પાછા ("STRIFY");
    સ્થિતિ VP_STRING: પાછા ("STRING");
    સ્થિતિ VP_પ્રતીક: પાછા ("SYMBOL");
    સ્થિતિ VP_પ્રતીક_JOIN: પાછા ("SYMBOL_JOIN");
    સ્થિતિ VP_TEXT: પાછા ("TEXT");
    સ્થિતિ VP_UNDEF: પાછા ("UNDEF");
    સ્થિતિ VP_UNDEFINEALL: પાછા ("UNDEFINEALL");
    સ્થિતિ VP_WHITE: પાછા ("WHITE");
    મૂળભૂત: પાછા ("?");
    }
}

શૂન્ય V3PreProcImp::unputString(અચલ string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    જો (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

શૂન્ય V3PreProcImp::unputDefrefString(અચલ string& strg) {
    અચલ પૂર્ણાંક multiline = std::ગણતરી(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(અચલ string& strg, તાર્કિક trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    જ્યારે (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    જો (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    જો (trailing) {
        string::size_type trailspace = 0;
        જ્યારે (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        જો (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        જો (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    પાછા out;
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
    માટે (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    અચલ string કિંમત = def_કિંમત(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(કિંમત) << "'");

    std::map<અચલ string, string> arg_કિંમત_By_Name;
    {  // Parse argument list into map
        unsigned અંક_Args = 0;
        string argName;
        પૂર્ણાંક paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        તાર્કિક quote = ખોટું;
        તાર્કિક haveDefault = ખોટું;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        અચલ string params = refp->params();  // Must keep str in scope to get pointer
        અચલ અક્ષર* cp = params.c_str();
        જો (*cp == '(') cp++;
        માટે (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            જો (!quote && paren == 1) {
                જો (*cp == ')' || *cp == ',') {
                    string કિંમત_Def;
                    જો (haveDefault) {
                        કિંમત_Def = token;
                    } નહીંતર {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, સાચું);
                    UINFO(4, "    Got Arg=" << અંક_Args << "  argName='" << argName
                                            << "'  default='" << કિંમત_Def << "'");
                    // Parse it
                    જો (argName != "") {
                        જો (refp->args().size() > અંક_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            અચલ string arg = trimWhitespace(refp->args()[અંક_Args], સાચું);
                            જો (arg != "") કિંમત_Def = arg;
                        } નહીંતર જો (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            પાછા " `" + refp->name() + " ";
                        }
                        અંક_Args++;
                    }
                    arg_કિંમત_By_Name[argName] = કિંમત_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = ખોટું;
                    ચાલુ;
                } નહીંતર જો (*cp == '=') {
                    haveDefault = સાચું;
                    argName = token;
                    token = "";
                    ચાલુ;
                }
            }
            જો (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                ચાલુ;
            }
            જો (!quote) {
                જો (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } નહીંતર જો (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            જો (*cp == '"') quote = !quote;
            જો (*cp) token += *cp;
        }
        જો (refp->args().size() > અંક_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && અંક_Args == 0
                 && trimWhitespace(refp->args()[0], ખોટું) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            પાછા " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        તાર્કિક quote = ખોટું;
        તાર્કિક triquote = ખોટું;
        તાર્કિક backslashesc = ખોટું;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        માટે (અચલ અક્ષર* cp = કિંમત.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            જો (!quote && !triquote && *cp == '\\') {
                backslashesc = સાચું;
            } નહીંતર જો (std::isspace(*cp)) {
                backslashesc = ખોટું;
            }
            જો (!quote && !triquote) {
                જો (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    ચાલુ;
                }
                જો (argName != "") {
                    // Found a possible variable substitution
                    અચલ auto iter = arg_કિંમત_By_Name.find(argName);
                    જો (iter != arg_કિંમત_By_Name.end()) {
                        // Substitute
                        અચલ string subst = iter->second;
                        જો (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            જો (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } નહીંતર {
                            out += subst;
                        }
                    } નહીંતર {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                જો (cp[0] == '`' && cp[1] == '`') {
                    જો (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } નહીંતર {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    ચાલુ;
                } નહીંતર જો (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    ચાલુ;
                } નહીંતર જો (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    ચાલુ;
                } નહીંતર જો (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    ચાલુ;
                } નહીંતર જો (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    ચાલુ;
                }
            }
            જો (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                ચાલુ;
            } નહીંતર જો (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                ચાલુ;
            }
            જો (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                ચાલુ;
            }
            જો (*cp == '"') quote = !quote;
            જો (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    પાછા out;
}

//**********************************************************************
// Parser routines

શૂન્ય V3PreProcImp::openFile(File_લીટી*, VInFilter* filterp, અચલ string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    જો (m_incError) પાછા;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    અચલ તાર્કિક ok = filterp->readWholefile(filename, wholefile /*ref*/);
    જો (!ok) {
        fileline()->v3error("File not found: " + filename);
        પાછા;
    }

    જો (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        જો (m_lexp->m_streamp_ઢગલો.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = સાચું;
            પાછા;
        }
        // There's already a file active.  Push it to work on the new one.
        add_લીટી_Comment(0);
    }

    // Save file contents for future error reporting
    File_લીટી* અચલ flsp = નવું File_લીટી{filename};
    flsp->lineno(1);
    flsp->newContent();
    માટે (અચલ string& i : wholefile) flsp->contentp()->ધકેલો_Text(i);

    // Save contents for V3Control --contents
    જો (filename != V3Options::getStdPackagePath()) {
        તાર્કિક containsVlt = ખોટું;
        માટે (અચલ string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            જો (i.find("`verilator_config") != string::npos) {
                containsVlt = સાચું;
                તોડો;
            }
        }
        જો (!containsVlt) {
            માટે (અચલ string& i : wholefile) V3Control::contents_ધકેલો_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_લીટી_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    પૂર્ણાંક eof_newline = 0;  // Number of characters following last newline
    પૂર્ણાંક eof_lineno = 1;
    માટે (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        તાર્કિક strip = ખોટું;
        અચલ અક્ષર* અચલ sp = it->data();
        અચલ અક્ષર* અચલ ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        માટે (અચલ અક્ષર* cp = sp; cp < ep; cp++) {
            જો (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = સાચું;
            } નહીંતર જો (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } નહીંતર {
                ++eof_newline;
            }
        }
        જો (strip) {
            string out;
            out.reserve(it->length());
            માટે (અચલ અક્ષર* cp = sp; cp < ep; cp++) {
                જો (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    જો (eof_newline) {
        File_લીટી* અચલ fl = નવું File_લીટી{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

શૂન્ય V3PreProcImp::ઉમેરો_Unreadback_At_Bol(અચલ string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    જો (m_લીટી_Cmt == "") {
        m_લીટી_Cmt_Nl = સાચું;
    } નહીંતર જો (m_લીટી_Cmt[m_લીટી_Cmt.length() - 1] != '\n') {
        ઉમેરો_Unreadback("\n");
    }
    ઉમેરો_Unreadback(text);
}

શૂન્ય V3PreProcImp::add_લીટી_Comment(પૂર્ણાંક enterExit) {
    જો (લીટી_Directives()) {
        ઉમેરો_Unreadback_At_Bol(m_lexp->curFilelinep()->લીટી_Directive_Strg(enterExit));
    }
}

શૂન્ય V3PreProcImp::dumpDefines(std::ostream& os) {
    માટે (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        જો (!it->second.params().empty()) os << it->second.params();
        જો (!it->second.કિંમત().empty()) os << " " << it->second.કિંમત();
        os << '\n';
    }
}

શૂન્ય V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    માટે (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->ધકેલો_Candidate("`"s + it->first);
    }
}

પૂર્ણાંક V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    જ્યારે (સાચું) {
    next_tok:
        જો (m_લીટી_Add) {
            m_લીટી_Add--;
            m_rawAtBol = સાચું;
            yyourtext("\n", 1);
            જો (debug() >= 5) debugToken(VP_WHITE, "LNA");
            પાછા VP_WHITE;
        }
        જો (m_લીટી_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            સ્થિર string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_લીટી_Cmt;
            જો (m_લીટી_Cmt_Nl) {
                જો (!m_rawAtBol) s_rtncmt.ઉમેરો(0, "\n");
                m_લીટી_Cmt_Nl = ખોટું;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_લીટી_Cmt = "";
            જો (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            જો (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_કિંમત(yyourtext(), yyourleng());
                જાઓ next_tok;
            } નહીંતર {
                જો (debug() >= 5) debugToken(VP_TEXT, "LCM");
                પાછા VP_TEXT;
            }
        }
        જો (isEof()) પાછા VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        પૂર્ણાંક tok = m_lexp->lex();
        જો (debug() >= 5) debugToken(tok, "RAW");

        જો (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_લીટી = 0;
        } નહીંતર જો (++m_tokens_On_લીટી > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        જો (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            જો (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            જાઓ next_tok;
        }

        જો (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        પાછા tok;
    }
}

શૂન્ય V3PreProcImp::debugToken(પૂર્ણાંક tok, અચલ અક્ષર* cmtp) {
    અચલ સ્થિર પૂર્ણાંક s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    જો (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        જ્યારે ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        જ્યારે ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        અચલ string flcol = m_lexp->m_tokFilelinep->ascii_લીટી_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<પૂર્ણાંક>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        જો (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

પૂર્ણાંક V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    જ્યારે (સાચું) {
    next_tok:
        જો (isEof()) પાછા VP_EOF;
        પૂર્ણાંક tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        જો (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) પાછા tok;
        જો (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        જો (tok == VP_COMMENT) {
            જો (!m_off) {
                જો (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    જો (m_લીટી_Cmt.empty()) ઉમેરો_Unreadback(" ");
                } નહીંતર જો (m_lexp->m_keepComments) {
                    પાછા tok;
                } નહીંતર {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ઉમેરો_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            માટે (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                જો (yyourtext()[len] == '\n') m_લીટી_Add++;
            }
            જાઓ next_tok;
        }
        જો (tok == VP_લીટી) {
            add_લીટી_Comment(m_lexp->m_enterExit);
            જાઓ next_tok;
        }

        જો (tok == VP_DEFREF_JOIN) {
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
            અચલ std::string name(yyourtext() + 1, yyourleng() - 1);
            જો (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = સાચું;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } નહીંતર {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        જો (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            જો (tok == VP_JOIN) જાઓ next_tok;
            જો (tok == VP_પ્રતીક_JOIN) tok = VP_પ્રતીક;
        }
        જો (tok == VP_પ્રતીક_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            અચલ std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ઢગલો.ધકેલો(out);
            state_ધકેલો(ps_JOIN);
            જાઓ next_tok;
        }

        // Deal with some special parser states
        પસંદગી (state()) {
        સ્થિતિ ps_ટોચ: {
            તોડો;
        }
        સ્થિતિ ps_DEFNAME_UNDEF:  // FALLTHRU
        સ્થિતિ ps_DEFNAME_DEFINE:  // FALLTHRU
        સ્થિતિ ps_DEFNAME_IFDEF:  // FALLTHRU
        સ્થિતિ ps_DEFNAME_IFNDEF:  // FALLTHRU
        સ્થિતિ ps_DEFNAME_ELSIF: {
            જો (tok == VP_પ્રતીક) {
                m_lastSym.assign(yyourtext(), yyourleng());
                જો (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    તાર્કિક enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    જો (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ઢગલો.emplace(VPreIfEntry{enable, ખોટું});
                    જો (!enable) parsingOff();
                    state_કાઢો();
                    જાઓ next_tok;
                } નહીંતર જો (state() == ps_DEFNAME_ELSIF) {
                    જો (m_ifdef_ઢગલો.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } નહીંતર {
                        // Handle `else portion
                        અચલ VPreIfEntry lastIf = m_ifdef_ઢગલો.ટોચ();
                        m_ifdef_ઢગલો.કાઢો();
                        જો (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        અચલ તાર્કિક enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_ઢગલો.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        જો (!enable) parsingOff();
                    }
                    state_કાઢો();
                    જાઓ next_tok;
                } નહીંતર જો (state() == ps_DEFNAME_UNDEF) {
                    જો (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_કાઢો();
                    જાઓ next_tok;
                } નહીંતર જો (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ધકેલો_State_Def_Form();
                    જાઓ next_tok;
                } નહીંતર {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                જો (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ધકેલો_State_પદાવલિ();
                    m_પદાવલિ_Parser.reset(fileline());
                    m_પદાવલિ_Paren_Level = 1;
                    પસંદગી (state()) {
                    સ્થિતિ ps_DEFNAME_IFDEF: stateChange(ps_પદાવલિ_IFDEF); તોડો;
                    સ્થિતિ ps_DEFNAME_IFNDEF: stateChange(ps_પદાવલિ_IFNDEF); તોડો;
                    સ્થિતિ ps_DEFNAME_ELSIF: stateChange(ps_પદાવલિ_ELSIF); તોડો;
                    મૂળભૂત: v3fatalSrc("bad case");
                    }
                    જાઓ next_tok;
                }
                જો (!m_off) {
                    પાછા tok;
                } નહીંતર {
                    જાઓ next_tok;
                }
            } નહીંતર જો (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                તોડો;
            } નહીંતર {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                જાઓ next_tok;
            }
        }
        સ્થિતિ ps_પદાવલિ_IFDEF:  // FALLTHRU
        સ્થિતિ ps_પદાવલિ_IFNDEF:  // FALLTHRU
        સ્થિતિ ps_પદાવલિ_ELSIF: {
            // `ifdef ( *here*
            File_લીટી* અચલ flp = m_lexp->m_tokFilelinep;
            જો (tok == VP_પ્રતીક) {
                m_lastSym.assign(yyourtext(), yyourleng());
                અચલ તાર્કિક exists = defExists(m_lastSym);
                જો (exists) {
                    અચલ std::string કિંમત = def_કિંમત(m_lastSym);
                    જો (VString::removeWhitespace(કિંમત) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << કિંમત
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, exists});
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_WHITE) {
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::BRA});
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_પદાવલિ_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                જો (--m_પદાવલિ_Paren_Level > 0) {
                    m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::KET});
                    જાઓ next_tok;
                } નહીંતર {
                    // Done with parsing expression
                    તાર્કિક enable = m_પદાવલિ_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    જો (state() == ps_પદાવલિ_IFDEF || state() == ps_પદાવલિ_IFNDEF) {
                        જો (state() == ps_પદાવલિ_IFNDEF) enable = !enable;
                        m_ifdef_ઢગલો.emplace(VPreIfEntry{enable, ખોટું});
                        જો (!enable) parsingOff();
                        state_કાઢો();
                        જાઓ next_tok;
                    } નહીંતર જો (state() == ps_પદાવલિ_ELSIF) {
                        જો (m_ifdef_ઢગલો.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } નહીંતર {
                            // Handle `else portion
                            અચલ VPreIfEntry lastIf = m_ifdef_ઢગલો.ટોચ();
                            m_ifdef_ઢગલો.કાઢો();
                            જો (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_ઢગલો.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            જો (!enable) parsingOff();
                        }
                        state_કાઢો();
                    }
                    જાઓ next_tok;
                }
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::LNOT});
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::LAND});
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::LOR});
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::IMP});
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_પદાવલિ_Parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::EQV});
                જાઓ next_tok;
            } નહીંતર {
                જો (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    પાછા tok;
                } નહીંતર {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                જાઓ next_tok;
            }
        }
        સ્થિતિ ps_DEFFORM: {
            જો (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_કિંમત;
                જો (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ધકેલો_State_Def_કિંમત();
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT) {
                // IE, something like comment in formals
                જો (!m_off) પાછા tok;
                જાઓ next_tok;
            } નહીંતર {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                જાઓ next_tok;
            }
        }
        સ્થિતિ ps_DEFVALUE: {
            સ્થિર string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            જો (tok == VP_DEFVALUE) {
                જો (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_કિંમત)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                અચલ string formals = m_formals;
                string કિંમત = m_lexp->m_def_કિંમત;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                માટે (size_t i = 0; i < formals.length(); i++) {
                    જો (formals[i] == '\n') s_newlines += "\n";
                }
                માટે (size_t i = 0; i < કિંમત.length(); i++) {
                    જો (કિંમત[i] == '\n') s_newlines += "\n";
                }
                જો (!m_off) {
                    // Remove leading and trailing whitespace
                    કિંમત = trimWhitespace(કિંમત, સાચું);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(કિંમત) << "'");
                    define(fileline(), m_lastSym, કિંમત, formals, ખોટું);
                }
            } નહીંતર {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_કાઢો();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            પાછા VP_WHITE;
        }
        સ્થિતિ ps_DEFPAREN: {
            જો (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                જાઓ next_tok;
            } નહીંતર {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                અચલ VDefineRef* અચલ refp = &(m_defRefs.ટોચ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_કાઢો();
                જાઓ next_tok;
            }
        }
        સ્થિતિ ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.ટોચ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_કિંમત);
            m_lexp->m_def_કિંમત = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            જો (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().ધકેલો_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ધકેલો_State_Def_Arg(1);
                refp->nextarg("");
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().ધકેલો_back(refp->nextarg());
                string out;
                જો (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.કાઢો(), refp);
                state_કાઢો();
                જો (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    જો (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ઢગલો.empty(), "`` join stack empty, but in a ``");
                        અચલ string lhs = m_join_ઢગલો.ટોચ();
                        m_join_ઢગલો.કાઢો();
                        out.ઉમેરો(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_કાઢો();
                    }
                    જો (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    નહીંતર જો (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.ટોચ().parenLevel();
                } નહીંતર {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.ટોચ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_કિંમત + out);
                    m_lexp->m_def_કિંમત = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                તોડો;
            } નહીંતર જો (tok == VP_પ્રતીક || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ધકેલો(ps_STRIFY);
                જાઓ next_tok;
            } નહીંતર {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_કાઢો();
                જાઓ next_tok;
            }
        }
        સ્થિતિ ps_INCNAME: {
            જો (tok == VP_STRING) {
                state_કાઢો();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ધકેલો_State_Inc_Filename();
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                તોડો;
            } નહીંતર {
                state_કાઢો();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                જાઓ next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        સ્થિતિ ps_ERRORNAME: {
            જો (tok == VP_STRING) {
                જો (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_કાઢો();
                જાઓ next_tok;
            } નહીંતર {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_કાઢો();
                જાઓ next_tok;
            }
        }
        સ્થિતિ ps_JOIN: {
            જો (tok == VP_પ્રતીક || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ઢગલો.empty(), "`` join stack empty, but in a ``");
                અચલ string lhs = m_join_ઢગલો.ટોચ();
                m_join_ઢગલો.કાઢો();
                UINFO(5, "`` LHS:" << lhs);
                અચલ string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                અચલ string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_કાઢો();
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                અચલ string lhs = m_join_ઢગલો.ટોચ();
                m_join_ઢગલો.કાઢો();
                unputString(lhs);
                state_કાઢો();
                જાઓ next_tok;
            } નહીંતર {
                // `define, etc, fall through and expand.  Pop back here.
                તોડો;
            }
        }
        સ્થિતિ ps_STRIFY: {
            જો (tok == VP_STRIFY) {
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
                state_કાઢો();
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                તોડો;
            } નહીંતર જો (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                જાઓ next_tok;
            } નહીંતર જો (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                તોડો;
            } નહીંતર {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                જાઓ next_tok;
            }
        }
        મૂળભૂત: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        પસંદગી (tok) {
        સ્થિતિ VP_INCLUDE:
            જો (!m_off) {
                state_ધકેલો(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            જાઓ next_tok;
        સ્થિતિ VP_UNDEF: state_ધકેલો(ps_DEFNAME_UNDEF); જાઓ next_tok;
        સ્થિતિ VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ધકેલો(ps_DEFNAME_DEFINE);
            જાઓ next_tok;
        સ્થિતિ VP_IFDEF: state_ધકેલો(ps_DEFNAME_IFDEF); જાઓ next_tok;
        સ્થિતિ VP_IFNDEF: state_ધકેલો(ps_DEFNAME_IFNDEF); જાઓ next_tok;
        સ્થિતિ VP_ELSIF: state_ધકેલો(ps_DEFNAME_ELSIF); જાઓ next_tok;
        સ્થિતિ VP_ELSE:
            જો (m_ifdef_ઢગલો.empty()) {
                fileline()->v3error("`else with no matching `if");
            } નહીંતર {
                અચલ VPreIfEntry lastIf = m_ifdef_ઢગલો.ટોચ();
                m_ifdef_ઢગલો.કાઢો();
                અચલ તાર્કિક enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ઢગલો.emplace(VPreIfEntry{enable, lastIf.everOn()});
                જો (!lastIf.on()) parsingOn();
                જો (!enable) parsingOff();
            }
            જાઓ next_tok;
        સ્થિતિ VP_ENDIF:
            UINFO(4, "Endif ");
            જો (m_ifdef_ઢગલો.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } નહીંતર {
                અચલ VPreIfEntry lastIf = m_ifdef_ઢગલો.ટોચ();
                m_ifdef_ઢગલો.કાઢો();
                જો (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            જાઓ next_tok;

        સ્થિતિ VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            અચલ string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            જો (m_defPutJoin) {
                m_defPutJoin = ખોટું;
                unputString("``");
            }
            જો (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                જાઓ next_tok;
            }
            // Substitute
            જો (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                જો (m_off) {
                    જાઓ next_tok;
                } નહીંતર {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    જાઓ next_tok;
                }
            } નહીંતર {
                અચલ string params = defParams(name);
                જો (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    જો (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    જો (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        જો (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ઢગલો.empty(), "`` join stack empty, but in a ``");
                            અચલ string lhs = m_join_ઢગલો.ટોચ();
                            m_join_ઢગલો.કાઢો();
                            out.ઉમેરો(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_કાઢો();
                        }
                        જો (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        નહીંતર જો (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } નહીંતર {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* અચલ refp = &(m_defRefs.ટોચ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_કિંમત + out);
                        m_lexp->m_def_કિંમત = "";
                    }
                    જાઓ next_tok;
                } નહીંતર {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    જો (!m_defRefs.empty()) m_defRefs.ટોચ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ધકેલો(ps_DEFPAREN);
                    m_lexp->ધકેલો_State_Def_Arg(0);
                    જાઓ next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            જાઓ next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        સ્થિતિ VP_ERROR: {
            state_ધકેલો(ps_ERRORNAME);
            જાઓ next_tok;
        }
        સ્થિતિ VP_EOF:
            જો (!m_ifdef_ઢગલો.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            પાછા tok;
        સ્થિતિ VP_UNDEFINEALL:
            જો (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            જાઓ next_tok;
        સ્થિતિ VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ધકેલો(ps_STRIFY);
            જાઓ next_tok;
        સ્થિતિ VP_પ્રતીક:
        સ્થિતિ VP_STRING:
        સ્થિતિ VP_TEXT: {
            m_defDepth = 0;
            જો (!m_off) {
                પાછા tok;
            } નહીંતર {
                જાઓ next_tok;
            }
        }
        સ્થિતિ VP_WHITE:  // Handled at top of loop
        સ્થિતિ VP_COMMENT:  // Handled at top of loop
        સ્થિતિ VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        સ્થિતિ VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        મૂળભૂત:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            તોડો;  // LCOV_EXCL_LINE
        }
        પાછા tok;
    }
}

પૂર્ણાંક V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    જો (!m_finAhead) {
        m_finAhead = સાચું;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    અચલ પૂર્ણાંક tok = m_finToken;
    buf = m_finBuf;
    જો (ખોટું && debug() >= 5) {
        અચલ string bufcln = V3PreLex::cleanDbgStrg(buf);
        અચલ string flcol = m_lexp->m_tokFilelinep->ascii_લીટી_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    અચલ અક્ષર* bufp = buf.c_str();
    જ્યારે (*bufp == '\n') bufp++;
    જો ((tok == VP_TEXT || tok == VP_લીટી) && VString::startsWith(bufp, "`line ")) {
        પૂર્ણાંક enter;
        m_finFilelinep->લીટી_Directive(bufp, enter /*ref*/);
    } નહીંતર {
        જો (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->લીટી_Directives()) {
            જો (અચલ પૂર્ણાંક outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                જો (debug() >= 5) {
                    અચલ string flcol = m_lexp->m_tokFilelinep->ascii_લીટી_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                જો (outBehind > 0
                    && (outBehind <= static_cast<પૂર્ણાંક>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    જો (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        પાછા VP_TEXT;
                    }
                } નહીંતર {
                    // Need to backup, use `line
                    buf = m_finFilelinep->લીટી_Directive_Strg(0);
                    પાછા VP_લીટી;
                }
            }
        }
        // Track newlines in prep for next token
        માટે (અચલ અક્ષર& c : buf) {
            જો (c == '\n') {
                m_finAtBol = સાચું;
                m_finFilelinep->linenoInc();
            } નહીંતર {
                m_finAtBol = ખોટું;
            }
        }
    }
    m_finAhead = ખોટું;  // Consumed the token
    પાછા tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    જો (isEof()) પાછા "";
    અચલ અક્ષર* rtnp;
    તાર્કિક gotEof = ખોટું;
    જ્યારે (nullptr == (rtnp = std::strchr(m_લીટી_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        અચલ પૂર્ણાંક tok = getFinalToken(m_tokenBuf /*ref*/);
        જો (debug() >= 5) {
            અચલ string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            અચલ string flcol = m_lexp->m_tokFilelinep->ascii_લીટી_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        જો (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            જો (m_લીટી_Chars != "" && m_લીટી_Chars[m_લીટી_Chars.length() - 1] != '\n') {
                m_લીટી_Chars.append("\n");
            }
            gotEof = સાચું;
        } નહીંતર {
            m_લીટી_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    અચલ પૂર્ણાંક len = rtnp - m_લીટી_Chars.c_str() + 1;
    string the_લીટી(m_લીટી_Chars, 0, len);
    m_લીટી_Chars = m_લીટી_Chars.erase(0, len);  // Remove returned characters
    જો (debug() >= 4) {
        અચલ string lncln = V3PreLex::cleanDbgStrg(the_લીટી);
        અચલ string flcol = m_lexp->m_tokFilelinep->ascii_લીટી_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    પાછા the_લીટી;
}
