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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3PreProc.h"

#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3LanguageWords.h"
#অন্তর্ভাৱ "V3PreExpr.h"
#অন্তর্ভাৱ "V3PreLex.h"
#অন্তর্ভাৱ "V3PreShell.h"
#অন্তর্ভাৱ "V3String.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <cstdlib>
#অন্তর্ভাৱ <fstream>
#অন্তর্ভাৱ <stack>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#অন্তর্ভাৱ "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

শ্ৰেণী VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_সাৰি* ধ্রুৱ m_fileline;  // Where it was declared
    ধ্রুৱ string m_মান;  // Value of define
    ধ্রুৱ string m_params;  // Parameters
    ধ্রুৱ তার্কিক m_cmdline;  // Set on command line, don't `undefineall
সার্ৱজনিক:
    VDefine(File_সাৰি* fl, ধ্রুৱ string& মান, ধ্রুৱ string& params, তার্কিক cmdline)
        : m_fileline{fl}
        , m_মান{মান}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_সাৰি* fileline() ধ্রুৱ { ঘূৰাই m_fileline; }
    string মান() ধ্রুৱ { ঘূৰাই m_মান; }
    string params() ধ্রুৱ { ঘূৰাই m_params; }
    তার্কিক cmdline() ধ্রুৱ { ঘূৰাই m_cmdline; }
};

//*************************************************************************

শ্ৰেণী VDefineRef final {
    // One for each pending define substitution
    ধ্রুৱ string m_name;  // Define last name being defined
    ধ্রুৱ string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    পূর্ণাঙ্ক m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
সার্ৱজনিক:
    string name() ধ্রুৱ { ঘূৰাই m_name; }
    string params() ধ্রুৱ { ঘূৰাই m_params; }
    string nextarg() ধ্রুৱ { ঘূৰাই m_nextarg; }
    শূন্য nextarg(ধ্রুৱ string& মান) { m_nextarg = মান; }
    পূর্ণাঙ্ক parenLevel() ধ্রুৱ { ঘূৰাই m_parenLevel; }
    শূন্য parenLevel(পূর্ণাঙ্ক মান) { m_parenLevel = মান; }
    std::vector<string>& args() { ঘূৰাই m_args; }
    VDefineRef(ধ্রুৱ string& name, ধ্রুৱ string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = স্ৱভাৱ;
};

//*************************************************************************
/// Data for parsing on/off

শ্ৰেণী VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ধ্রুৱ তার্কিক m_on;  // Current parse for this ifdef level is "on"
    ধ্রুৱ তার্কিক m_everOn;  // Some if term in elsif tree has been on
সার্ৱজনিক:
    তার্কিক on() ধ্রুৱ { ঘূৰাই m_on; }
    তার্কিক everOn() ধ্রুৱ { ঘূৰাই m_everOn; }
    VPreIfEntry(তার্কিক on, তার্কিক everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = স্ৱভাৱ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

শ্ৰেণী V3PreProcImp final : সার্ৱজনিক V3PreProc {
সার্ৱজনিক:
    // TYPES
    using DefinesMap = std::map<ধ্রুৱ std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ধ্রুৱ V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    পূর্ণাঙ্ক m_lastLineno = 0;  // Last line number (stall detection)
    পূর্ণাঙ্ক m_tokens_On_সাৰি = 0;  // Number of tokens on line (stall detection)

    গণনা ProcState : uint8_t {
        ps_শীৰ্ষ,
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
        ps_ৰাশিমালা_IFDEF,
        ps_ৰাশিমালা_IFNDEF,
        ps_ৰাশিমালা_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    স্থির ধ্রুৱ ৱর্ণ* procStateName(ProcState s) {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ঘূৰাই states[s];
    }

    std::স্তূপ<ProcState> m_states;  ///< Current state of parser
    পূর্ণাঙ্ক m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    তার্কিক m_incError = মিছা;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_সাৰি_Cmt;  ///< Line comment(s) to be returned
    তার্কিক m_সাৰি_Cmt_Nl = মিছা;  ///< Newline needed before inserting lineCmt
    পূর্ণাঙ্ক m_সাৰি_Add = 0;  ///< Empty lines to return to maintain line count
    তার্কিক m_rawAtBol = সঁচা;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    তার্কিক m_finAhead = মিছা;  ///< Have read a token ahead
    পূর্ণাঙ্ক m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    তার্কিক m_finAtBol = সঁচা;  ///< Last getFinalToken left us at beginning of line
    File_সাৰি* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::স্তূপ<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::স্তূপ<VPreIfEntry> m_ifdef_স্তূপ;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    তার্কিক m_defPutJoin = মিছা;  ///< Insert `` after substitution

    // For `` join
    std::স্তূপ<string> m_join_স্তূপ;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_ৰাশিমালা m_ৰাশিমালা_Parser;  ///< Parser for () expression
    পূর্ণাঙ্ক m_ৰাশিমালা_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_সাৰি_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    শূন্য v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    স্থির ধ্রুৱ ৱর্ণ* tokenName(পূর্ণাঙ্ক tok);
    শূন্য debugToken(পূর্ণাঙ্ক tok, ধ্রুৱ ৱর্ণ* cmtp);
    শূন্য parse_শীৰ্ষ();
    শূন্য parseUndef();

গুপ্ত:
    // Internal methods
    শূন্য endOfOneFile();
    string defineSubst(VDefineRef* refp);

    তার্কিক defExists(ধ্রুৱ string& name);
    তার্কিক defCmdline(ধ্রুৱ string& name);
    string def_মান(ধ্রুৱ string& name);
    string defParams(ধ্রুৱ string& name);
    File_সাৰি* defFileline(ধ্রুৱ string& name);

    string commentCleanup(ধ্রুৱ string& text);
    তার্কিক commentTokenMatch(string& cmdr, ধ্রুৱ ৱর্ণ* strg);
    স্থির string trimWhitespace(ধ্রুৱ string& strg, তার্কিক trailing);
    শূন্য unputString(ধ্রুৱ string& strg);
    শূন্য unputDefrefString(ধ্রুৱ string& strg);

    শূন্য parsingOn() {
        m_off--;
        যদি (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    শূন্য parsingOff() { m_off++; }

    পূর্ণাঙ্ক getRawToken();
    পূর্ণাঙ্ক getStateToken();
    পূর্ণাঙ্ক getFinalToken(string& buf);

    ProcState state() ধ্রুৱ { ঘূৰাই m_states.শীৰ্ষ(); }
    তার্কিক stateIsDefname() ধ্রুৱ {
        ঘূৰাই state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    শূন্য state_ঠেলো(ProcState state) { m_states.ঠেলো(state); }
    শূন্য state_বেৰ_কৰো() {
        m_states.বেৰ_কৰো();
        যদি (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.ঠেলো(ps_শীৰ্ষ);  // LCOV_EXCL_LINE
        }
    }
    শূন্য stateChange(ProcState state) {
        state_বেৰ_কৰো();
        state_ঠেলো(state);
    }

সার্ৱজনিক:
    // METHODS, called from upper level shell
    শূন্য openFile(File_সাৰি* fl, VInFilter* filterp, ধ্রুৱ string& filename) override;
    তার্কিক isEof() ধ্রুৱ override { ঘূৰাই m_lexp->curStreamp()->m_eof; }
    শূন্য forceEof() { m_lexp->curStreamp()->m_eof = সঁচা; }
    string getline() override;
    শূন্য ঢোকাও_Unreadback(ধ্রুৱ string& text) override { m_সাৰি_Cmt += text; }
    শূন্য ঢোকাও_Unreadback_At_Bol(ধ্রুৱ string& text);
    শূন্য add_সাৰি_Comment(পূর্ণাঙ্ক enterExit);
    শূন্য dumpDefines(std::ostream& os) override;
    শূন্য candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    শূন্য comment(ধ্রুৱ string& text) override;  // Comment detected (if keepComments==2)
    শূন্য include(ধ্রুৱ string& filename) override;  // Request a include file be processed
    শূন্য undef(ধ্রুৱ string& name) override;
    আভাস শূন্য undefineall();
    শূন্য define(File_সাৰি* fl, ধ্রুৱ string& name, ধ্রুৱ string& মান, ধ্রুৱ string& params,
                তার্কিক cmdline) override;
    string removeDefines(ধ্রুৱ string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.ঠেলো(ps_শীৰ্ষ); }
    // cppcheck-suppress duplInheritedMember
    শূন্য configure(File_সাৰি* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = এতত্;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = নূতন File_সাৰি{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = নূতন V3PreLex{এতত্, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        যদি (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_সাৰি* fl) {
    V3PreProcImp* ধ্রুৱ preprocp = নূতন V3PreProcImp;
    preprocp->configure(fl);
    ঘূৰাই preprocp;
}

শূন্য V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_ৰাশিমালা::selfTest(); }

//*************************************************************************
// Defines

শূন্য V3PreProcImp::undef(ধ্রুৱ string& name) {
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ঢোকাও_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
শূন্য V3PreProcImp::undefineall() {
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ঢোকাও_Unreadback("`undefineall\n");
    বাবে (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        যদি (!it->second.cmdline()) m_defines.erase(it);
    }
}
তার্কিক V3PreProcImp::defExists(ধ্রুৱ string& name) {
    ধ্রুৱ auto iter = m_defines.find(name);
    ঘূৰাই (iter != m_defines.end());
}
তার্কিক V3PreProcImp::defCmdline(ধ্রুৱ string& name) {
    ধ্রুৱ auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) { ঘূৰাই মিছা; }
    ঘূৰাই iter->second.cmdline();
}
string V3PreProcImp::def_মান(ধ্রুৱ string& name) {
    ধ্রুৱ auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ঘূৰাই "";
    }
    ঘূৰাই iter->second.মান();
}
string V3PreProcImp::defParams(ধ্রুৱ string& name) {
    ধ্রুৱ auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ঘূৰাই "";
    }
    ঘূৰাই iter->second.params();
}
File_সাৰি* V3PreProcImp::defFileline(ধ্রুৱ string& name) {
    ধ্রুৱ auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) ঘূৰাই nullptr;
    ঘূৰাই iter->second.fileline();
}
শূন্য V3PreProcImp::define(File_সাৰি* fl, ধ্রুৱ string& name, ধ্রুৱ string& মান,
                          ধ্রুৱ string& params, তার্কিক cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << মান << "' params '" << params << "'");
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(মান.size());
        বাবে (string::size_type pos = 0; pos < মান.size(); ++pos) {
            যদি (মান[pos] == '\n' && (pos == 0 || মান[pos - 1] != '\\')) out += '\\';
            out += মান[pos];
        }
        ঢোকাও_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    যদি (!V3_Language_শব্দগুলি::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } অন্যথা {
        যদি (defExists(name)) {
            যদি (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << মান
                               << "' to existing command line define value: '" << def_মান(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_মান(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                ঘূৰাই;
            } অন্যথা {
                যদি (!(def_মান(name) == মান
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << মান
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_মান(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, মান, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(ধ্রুৱ string& text) {
    string val;
    string rtnsym = text;
    বাবে (পূর্ণাঙ্ক loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        যদি (xsym[0] == '`') xsym.erase(0, 1);
        যদি (defExists(xsym)) {
            val = def_মান(xsym);
            যদি (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } অন্যথা {
                ভঙ্গ;
            }
        } অন্যথা {
            ভঙ্গ;
        }
    }
    ঘূৰাই rtnsym;  // NA
}

//**********************************************************************
// Comments

শূন্য V3PreProcImp::include(ধ্রুৱ string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ধ্রুৱ string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    যেতিয়ালৈ ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    যেতিয়ালৈ ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    যেতিয়ালৈ ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    যেতিয়ালৈ ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    যেতিয়ালৈ ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    যেতিয়ালৈ ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    যেতিয়ালৈ (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    ঘূৰাই cmd;
}

তার্কিক V3PreProcImp::commentTokenMatch(string& cmdr, ধ্রুৱ ৱর্ণ* strg) {
    পূর্ণাঙ্ক len = std::strlen(strg);
    যদি (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        যদি (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ঘূৰাই সঁচা;
    } অন্যথা {
        ঘূৰাই মিছা;
    }
}

শূন্য V3PreProcImp::comment(ধ্রুৱ string& text) {
    // Comment detected.  Only keep relevant data.
    তার্কিক printed = মিছা;
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ঢোকাও_Unreadback(text);
        printed = সঁচা;
    }

    ধ্রুৱ ৱর্ণ* cp = text.c_str();
    যদি (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } অন্যথা {
        ঘূৰাই;
    }

    যেতিয়ালৈ (std::isspace(*cp)) ++cp;

    তার্কিক synth = মিছা;
    তার্কিক vlcomment = মিছা;
    যদি ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        যদি (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ঘূৰাই;
        }
        vlcomment = সঁচা;
    } অন্যথা যদি (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        যদি (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ঘূৰাই;
        }
        synth = সঁচা;
    } অন্যথা যদি (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = সঁচা;
    } অন্যথা যদি (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = সঁচা;
    } অন্যথা যদি (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = সঁচা;
    } অন্যথা {
        ঘূৰাই;
    }

    যেতিয়ালৈ (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    যদি (synth) {
        যদি (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            যদি (commentTokenMatch(cmd /*ref*/, "full_case")) {
                যদি (!printed) ঢোকাও_Unreadback("/*verilator full_case*/");
            }
            যদি (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                যদি (!printed) ঢোকাও_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } অন্যথা যদি (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        যদি (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            যেতিয়ালৈ (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            যেতিয়ালৈ (std::isspace(arg[0])) arg = arg.substr(1);
            যদি (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            যদি (!printed) ঢোকাও_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } অন্যথা যদি (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            যেতিয়ালৈ (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            যদি (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ঘূৰাই;
            }
            যদি (!printed) ঢোকাও_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } অন্যথা {
            যদি (!printed) ঢোকাও_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_সাৰি* V3PreProc::fileline() {
    ধ্রুৱ V3PreProcImp* idatap = static_cast<V3PreProcImp*>(এতত্);
    ঘূৰাই idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ধ্রুৱ ৱর্ণ* V3PreProcImp::tokenName(পূর্ণাঙ্ক tok) {
    ৱিকল্প (tok) {
    পক্ষ VP_BACKQUOTE: ঘূৰাই ("BACKQUOTE");
    পক্ষ VP_COMMENT: ঘূৰাই ("COMMENT");
    পক্ষ VP_DEFARG: ঘূৰাই ("DEFARG");
    পক্ষ VP_DEFFORM: ঘূৰাই ("DEFFORM");
    পক্ষ VP_DEFINE: ঘূৰাই ("DEFINE");
    পক্ষ VP_DEFREF: ঘূৰাই ("DEFREF");
    পক্ষ VP_DEFREF_JOIN: ঘূৰাই ("DEFREF_JOIN");
    পক্ষ VP_DEFVALUE: ঘূৰাই ("DEFVALUE");
    পক্ষ VP_ELSE: ঘূৰাই ("ELSE");
    পক্ষ VP_ELSIF: ঘূৰাই ("ELSIF");
    পক্ষ VP_ENDIF: ঘূৰাই ("ENDIF");
    পক্ষ VP_EOF: ঘূৰাই ("EOF");
    পক্ষ VP_EOF_ERROR: ঘূৰাই ("EOF_ERROR");
    পক্ষ VP_ERROR: ঘূৰাই ("ERROR");
    পক্ষ VP_IFDEF: ঘূৰাই ("IFDEF");
    পক্ষ VP_IFNDEF: ঘূৰাই ("IFNDEF");
    পক্ষ VP_JOIN: ঘূৰাই ("JOIN");
    পক্ষ VP_INCLUDE: ঘূৰাই ("INCLUDE");
    পক্ষ VP_সাৰি: ঘূৰাই ("LINE");
    পক্ষ VP_STRIFY: ঘূৰাই ("STRIFY");
    পক্ষ VP_STRING: ঘূৰাই ("STRING");
    পক্ষ VP_প্ৰতীক: ঘূৰাই ("SYMBOL");
    পক্ষ VP_প্ৰতীক_JOIN: ঘূৰাই ("SYMBOL_JOIN");
    পক্ষ VP_TEXT: ঘূৰাই ("TEXT");
    পক্ষ VP_UNDEF: ঘূৰাই ("UNDEF");
    পক্ষ VP_UNDEFINEALL: ঘূৰাই ("UNDEFINEALL");
    পক্ষ VP_WHITE: ঘূৰাই ("WHITE");
    স্ৱভাৱ: ঘূৰাই ("?");
    }
}

শূন্য V3PreProcImp::unputString(ধ্রুৱ string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    যদি (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

শূন্য V3PreProcImp::unputDefrefString(ধ্রুৱ string& strg) {
    ধ্রুৱ পূর্ণাঙ্ক multiline = std::গুনতি(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ধ্রুৱ string& strg, তার্কিক trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    যেতিয়ালৈ (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    যদি (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    যদি (trailing) {
        string::size_type trailspace = 0;
        যেতিয়ালৈ (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        যদি (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        যদি (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    ঘূৰাই out;
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
    বাবে (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ধ্রুৱ string মান = def_মান(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(মান) << "'");

    std::map<ধ্রুৱ string, string> arg_মান_By_Name;
    {  // Parse argument list into map
        unsigned অঙ্ক_Args = 0;
        string argName;
        পূর্ণাঙ্ক paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        তার্কিক quote = মিছা;
        তার্কিক haveDefault = মিছা;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ধ্রুৱ string params = refp->params();  // Must keep str in scope to get pointer
        ধ্রুৱ ৱর্ণ* cp = params.c_str();
        যদি (*cp == '(') cp++;
        বাবে (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            যদি (!quote && paren == 1) {
                যদি (*cp == ')' || *cp == ',') {
                    string মান_Def;
                    যদি (haveDefault) {
                        মান_Def = token;
                    } অন্যথা {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, সঁচা);
                    UINFO(4, "    Got Arg=" << অঙ্ক_Args << "  argName='" << argName
                                            << "'  default='" << মান_Def << "'");
                    // Parse it
                    যদি (argName != "") {
                        যদি (refp->args().size() > অঙ্ক_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ধ্রুৱ string arg = trimWhitespace(refp->args()[অঙ্ক_Args], সঁচা);
                            যদি (arg != "") মান_Def = arg;
                        } অন্যথা যদি (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ঘূৰাই " `" + refp->name() + " ";
                        }
                        অঙ্ক_Args++;
                    }
                    arg_মান_By_Name[argName] = মান_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = মিছা;
                    অনুৱর্ত;
                } অন্যথা যদি (*cp == '=') {
                    haveDefault = সঁচা;
                    argName = token;
                    token = "";
                    অনুৱর্ত;
                }
            }
            যদি (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                অনুৱর্ত;
            }
            যদি (!quote) {
                যদি (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } অন্যথা যদি (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            যদি (*cp == '"') quote = !quote;
            যদি (*cp) token += *cp;
        }
        যদি (refp->args().size() > অঙ্ক_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && অঙ্ক_Args == 0
                 && trimWhitespace(refp->args()[0], মিছা) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ঘূৰাই " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        তার্কিক quote = মিছা;
        তার্কিক triquote = মিছা;
        তার্কিক backslashesc = মিছা;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        বাবে (ধ্রুৱ ৱর্ণ* cp = মান.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            যদি (!quote && !triquote && *cp == '\\') {
                backslashesc = সঁচা;
            } অন্যথা যদি (std::isspace(*cp)) {
                backslashesc = মিছা;
            }
            যদি (!quote && !triquote) {
                যদি (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    অনুৱর্ত;
                }
                যদি (argName != "") {
                    // Found a possible variable substitution
                    ধ্রুৱ auto iter = arg_মান_By_Name.find(argName);
                    যদি (iter != arg_মান_By_Name.end()) {
                        // Substitute
                        ধ্রুৱ string subst = iter->second;
                        যদি (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            যদি (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } অন্যথা {
                            out += subst;
                        }
                    } অন্যথা {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                যদি (cp[0] == '`' && cp[1] == '`') {
                    যদি (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } অন্যথা {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    অনুৱর্ত;
                } অন্যথা যদি (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    অনুৱর্ত;
                } অন্যথা যদি (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    অনুৱর্ত;
                } অন্যথা যদি (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    অনুৱর্ত;
                } অন্যথা যদি (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    অনুৱর্ত;
                }
            }
            যদি (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                অনুৱর্ত;
            } অন্যথা যদি (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                অনুৱর্ত;
            }
            যদি (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                অনুৱর্ত;
            }
            যদি (*cp == '"') quote = !quote;
            যদি (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    ঘূৰাই out;
}

//**********************************************************************
// Parser routines

শূন্য V3PreProcImp::openFile(File_সাৰি*, VInFilter* filterp, ধ্রুৱ string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    যদি (m_incError) ঘূৰাই;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ধ্রুৱ তার্কিক ok = filterp->readWholefile(filename, wholefile /*ref*/);
    যদি (!ok) {
        fileline()->v3error("File not found: " + filename);
        ঘূৰাই;
    }

    যদি (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        যদি (m_lexp->m_streamp_স্তূপ.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = সঁচা;
            ঘূৰাই;
        }
        // There's already a file active.  Push it to work on the new one.
        add_সাৰি_Comment(0);
    }

    // Save file contents for future error reporting
    File_সাৰি* ধ্রুৱ flsp = নূতন File_সাৰি{filename};
    flsp->lineno(1);
    flsp->newContent();
    বাবে (ধ্রুৱ string& i : wholefile) flsp->contentp()->ঠেলো_Text(i);

    // Save contents for V3Control --contents
    যদি (filename != V3Options::getStdPackagePath()) {
        তার্কিক containsVlt = মিছা;
        বাবে (ধ্রুৱ string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            যদি (i.find("`verilator_config") != string::npos) {
                containsVlt = সঁচা;
                ভঙ্গ;
            }
        }
        যদি (!containsVlt) {
            বাবে (ধ্রুৱ string& i : wholefile) V3Control::contents_ঠেলো_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_সাৰি_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    পূর্ণাঙ্ক eof_newline = 0;  // Number of characters following last newline
    পূর্ণাঙ্ক eof_lineno = 1;
    বাবে (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        তার্কিক strip = মিছা;
        ধ্রুৱ ৱর্ণ* ধ্রুৱ sp = it->data();
        ধ্রুৱ ৱর্ণ* ধ্রুৱ ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        বাবে (ধ্রুৱ ৱর্ণ* cp = sp; cp < ep; cp++) {
            যদি (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = সঁচা;
            } অন্যথা যদি (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } অন্যথা {
                ++eof_newline;
            }
        }
        যদি (strip) {
            string out;
            out.reserve(it->length());
            বাবে (ধ্রুৱ ৱর্ণ* cp = sp; cp < ep; cp++) {
                যদি (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    যদি (eof_newline) {
        File_সাৰি* ধ্রুৱ fl = নূতন File_সাৰি{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

শূন্য V3PreProcImp::ঢোকাও_Unreadback_At_Bol(ধ্রুৱ string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    যদি (m_সাৰি_Cmt == "") {
        m_সাৰি_Cmt_Nl = সঁচা;
    } অন্যথা যদি (m_সাৰি_Cmt[m_সাৰি_Cmt.length() - 1] != '\n') {
        ঢোকাও_Unreadback("\n");
    }
    ঢোকাও_Unreadback(text);
}

শূন্য V3PreProcImp::add_সাৰি_Comment(পূর্ণাঙ্ক enterExit) {
    যদি (সাৰি_Directives()) {
        ঢোকাও_Unreadback_At_Bol(m_lexp->curFilelinep()->সাৰি_Directive_Strg(enterExit));
    }
}

শূন্য V3PreProcImp::dumpDefines(std::ostream& os) {
    বাবে (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        যদি (!it->second.params().empty()) os << it->second.params();
        যদি (!it->second.মান().empty()) os << " " << it->second.মান();
        os << '\n';
    }
}

শূন্য V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    বাবে (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->ঠেলো_Candidate("`"s + it->first);
    }
}

পূর্ণাঙ্ক V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    যেতিয়ালৈ (সঁচা) {
    next_tok:
        যদি (m_সাৰি_Add) {
            m_সাৰি_Add--;
            m_rawAtBol = সঁচা;
            yyourtext("\n", 1);
            যদি (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ঘূৰাই VP_WHITE;
        }
        যদি (m_সাৰি_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            স্থির string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_সাৰি_Cmt;
            যদি (m_সাৰি_Cmt_Nl) {
                যদি (!m_rawAtBol) s_rtncmt.ঢোকাও(0, "\n");
                m_সাৰি_Cmt_Nl = মিছা;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_সাৰি_Cmt = "";
            যদি (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            যদি (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_মান(yyourtext(), yyourleng());
                গচ্ছ next_tok;
            } অন্যথা {
                যদি (debug() >= 5) debugToken(VP_TEXT, "LCM");
                ঘূৰাই VP_TEXT;
            }
        }
        যদি (isEof()) ঘূৰাই VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        পূর্ণাঙ্ক tok = m_lexp->lex();
        যদি (debug() >= 5) debugToken(tok, "RAW");

        যদি (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_সাৰি = 0;
        } অন্যথা যদি (++m_tokens_On_সাৰি > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        যদি (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            যদি (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            গচ্ছ next_tok;
        }

        যদি (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ঘূৰাই tok;
    }
}

শূন্য V3PreProcImp::debugToken(পূর্ণাঙ্ক tok, ধ্রুৱ ৱর্ণ* cmtp) {
    ধ্রুৱ স্থির পূর্ণাঙ্ক s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    যদি (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        যেতিয়ালৈ ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        যেতিয়ালৈ ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ধ্রুৱ string flcol = m_lexp->m_tokFilelinep->ascii_সাৰি_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<পূর্ণাঙ্ক>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        যদি (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

পূর্ণাঙ্ক V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    যেতিয়ালৈ (সঁচা) {
    next_tok:
        যদি (isEof()) ঘূৰাই VP_EOF;
        পূর্ণাঙ্ক tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        যদি (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ঘূৰাই tok;
        যদি (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        যদি (tok == VP_COMMENT) {
            যদি (!m_off) {
                যদি (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    যদি (m_সাৰি_Cmt.empty()) ঢোকাও_Unreadback(" ");
                } অন্যথা যদি (m_lexp->m_keepComments) {
                    ঘূৰাই tok;
                } অন্যথা {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ঢোকাও_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            বাবে (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                যদি (yyourtext()[len] == '\n') m_সাৰি_Add++;
            }
            গচ্ছ next_tok;
        }
        যদি (tok == VP_সাৰি) {
            add_সাৰি_Comment(m_lexp->m_enterExit);
            গচ্ছ next_tok;
        }

        যদি (tok == VP_DEFREF_JOIN) {
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
            ধ্রুৱ std::string name(yyourtext() + 1, yyourleng() - 1);
            যদি (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = সঁচা;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } অন্যথা {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        যদি (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            যদি (tok == VP_JOIN) গচ্ছ next_tok;
            যদি (tok == VP_প্ৰতীক_JOIN) tok = VP_প্ৰতীক;
        }
        যদি (tok == VP_প্ৰতীক_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ধ্রুৱ std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_স্তূপ.ঠেলো(out);
            state_ঠেলো(ps_JOIN);
            গচ্ছ next_tok;
        }

        // Deal with some special parser states
        ৱিকল্প (state()) {
        পক্ষ ps_শীৰ্ষ: {
            ভঙ্গ;
        }
        পক্ষ ps_DEFNAME_UNDEF:  // FALLTHRU
        পক্ষ ps_DEFNAME_DEFINE:  // FALLTHRU
        পক্ষ ps_DEFNAME_IFDEF:  // FALLTHRU
        পক্ষ ps_DEFNAME_IFNDEF:  // FALLTHRU
        পক্ষ ps_DEFNAME_ELSIF: {
            যদি (tok == VP_প্ৰতীক) {
                m_lastSym.assign(yyourtext(), yyourleng());
                যদি (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    তার্কিক enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    যদি (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, মিছা});
                    যদি (!enable) parsingOff();
                    state_বেৰ_কৰো();
                    গচ্ছ next_tok;
                } অন্যথা যদি (state() == ps_DEFNAME_ELSIF) {
                    যদি (m_ifdef_স্তূপ.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } অন্যথা {
                        // Handle `else portion
                        ধ্রুৱ VPreIfEntry lastIf = m_ifdef_স্তূপ.শীৰ্ষ();
                        m_ifdef_স্তূপ.বেৰ_কৰো();
                        যদি (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ধ্রুৱ তার্কিক enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        যদি (!enable) parsingOff();
                    }
                    state_বেৰ_কৰো();
                    গচ্ছ next_tok;
                } অন্যথা যদি (state() == ps_DEFNAME_UNDEF) {
                    যদি (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_বেৰ_কৰো();
                    গচ্ছ next_tok;
                } অন্যথা যদি (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ঠেলো_State_Def_Form();
                    গচ্ছ next_tok;
                } অন্যথা {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                যদি (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ঠেলো_State_ৰাশিমালা();
                    m_ৰাশিমালা_Parser.reset(fileline());
                    m_ৰাশিমালা_Paren_Level = 1;
                    ৱিকল্প (state()) {
                    পক্ষ ps_DEFNAME_IFDEF: stateChange(ps_ৰাশিমালা_IFDEF); ভঙ্গ;
                    পক্ষ ps_DEFNAME_IFNDEF: stateChange(ps_ৰাশিমালা_IFNDEF); ভঙ্গ;
                    পক্ষ ps_DEFNAME_ELSIF: stateChange(ps_ৰাশিমালা_ELSIF); ভঙ্গ;
                    স্ৱভাৱ: v3fatalSrc("bad case");
                    }
                    গচ্ছ next_tok;
                }
                যদি (!m_off) {
                    ঘূৰাই tok;
                } অন্যথা {
                    গচ্ছ next_tok;
                }
            } অন্যথা যদি (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                ভঙ্গ;
            } অন্যথা {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                গচ্ছ next_tok;
            }
        }
        পক্ষ ps_ৰাশিমালা_IFDEF:  // FALLTHRU
        পক্ষ ps_ৰাশিমালা_IFNDEF:  // FALLTHRU
        পক্ষ ps_ৰাশিমালা_ELSIF: {
            // `ifdef ( *here*
            File_সাৰি* ধ্রুৱ flp = m_lexp->m_tokFilelinep;
            যদি (tok == VP_প্ৰতীক) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ধ্রুৱ তার্কিক exists = defExists(m_lastSym);
                যদি (exists) {
                    ধ্রুৱ std::string মান = def_মান(m_lastSym);
                    যদি (VString::removeWhitespace(মান) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << মান
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, exists});
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_WHITE) {
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::BRA});
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_ৰাশিমালা_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                যদি (--m_ৰাশিমালা_Paren_Level > 0) {
                    m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::KET});
                    গচ্ছ next_tok;
                } অন্যথা {
                    // Done with parsing expression
                    তার্কিক enable = m_ৰাশিমালা_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    যদি (state() == ps_ৰাশিমালা_IFDEF || state() == ps_ৰাশিমালা_IFNDEF) {
                        যদি (state() == ps_ৰাশিমালা_IFNDEF) enable = !enable;
                        m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, মিছা});
                        যদি (!enable) parsingOff();
                        state_বেৰ_কৰো();
                        গচ্ছ next_tok;
                    } অন্যথা যদি (state() == ps_ৰাশিমালা_ELSIF) {
                        যদি (m_ifdef_স্তূপ.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } অন্যথা {
                            // Handle `else portion
                            ধ্রুৱ VPreIfEntry lastIf = m_ifdef_স্তূপ.শীৰ্ষ();
                            m_ifdef_স্তূপ.বেৰ_কৰো();
                            যদি (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            যদি (!enable) parsingOff();
                        }
                        state_বেৰ_কৰো();
                    }
                    গচ্ছ next_tok;
                }
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::LNOT});
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::LAND});
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::LOR});
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::IMP});
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_ৰাশিমালা_Parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::EQV});
                গচ্ছ next_tok;
            } অন্যথা {
                যদি (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    ঘূৰাই tok;
                } অন্যথা {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                গচ্ছ next_tok;
            }
        }
        পক্ষ ps_DEFFORM: {
            যদি (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_মান;
                যদি (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ঠেলো_State_Def_মান();
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT) {
                // IE, something like comment in formals
                যদি (!m_off) ঘূৰাই tok;
                গচ্ছ next_tok;
            } অন্যথা {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                গচ্ছ next_tok;
            }
        }
        পক্ষ ps_DEFVALUE: {
            স্থির string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            যদি (tok == VP_DEFVALUE) {
                যদি (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_মান)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ধ্রুৱ string formals = m_formals;
                string মান = m_lexp->m_def_মান;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                বাবে (size_t i = 0; i < formals.length(); i++) {
                    যদি (formals[i] == '\n') s_newlines += "\n";
                }
                বাবে (size_t i = 0; i < মান.length(); i++) {
                    যদি (মান[i] == '\n') s_newlines += "\n";
                }
                যদি (!m_off) {
                    // Remove leading and trailing whitespace
                    মান = trimWhitespace(মান, সঁচা);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(মান) << "'");
                    define(fileline(), m_lastSym, মান, formals, মিছা);
                }
            } অন্যথা {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_বেৰ_কৰো();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            ঘূৰাই VP_WHITE;
        }
        পক্ষ ps_DEFPAREN: {
            যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                গচ্ছ next_tok;
            } অন্যথা {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ধ্রুৱ VDefineRef* ধ্রুৱ refp = &(m_defRefs.শীৰ্ষ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            }
        }
        পক্ষ ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.শীৰ্ষ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_মান);
            m_lexp->m_def_মান = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            যদি (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().ঠেলো_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ঠেলো_State_Def_Arg(1);
                refp->nextarg("");
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().ঠেলো_back(refp->nextarg());
                string out;
                যদি (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.বেৰ_কৰো(), refp);
                state_বেৰ_কৰো();
                যদি (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    যদি (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_স্তূপ.empty(), "`` join stack empty, but in a ``");
                        ধ্রুৱ string lhs = m_join_স্তূপ.শীৰ্ষ();
                        m_join_স্তূপ.বেৰ_কৰো();
                        out.ঢোকাও(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_বেৰ_কৰো();
                    }
                    যদি (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    অন্যথা যদি (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.শীৰ্ষ().parenLevel();
                } অন্যথা {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.শীৰ্ষ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_মান + out);
                    m_lexp->m_def_মান = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                ভঙ্গ;
            } অন্যথা যদি (tok == VP_প্ৰতীক || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ঠেলো(ps_STRIFY);
                গচ্ছ next_tok;
            } অন্যথা {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            }
        }
        পক্ষ ps_INCNAME: {
            যদি (tok == VP_STRING) {
                state_বেৰ_কৰো();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ঠেলো_State_Inc_Filename();
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                ভঙ্গ;
            } অন্যথা {
                state_বেৰ_কৰো();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                গচ্ছ next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        পক্ষ ps_ERRORNAME: {
            যদি (tok == VP_STRING) {
                যদি (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            } অন্যথা {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            }
        }
        পক্ষ ps_JOIN: {
            যদি (tok == VP_প্ৰতীক || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_স্তূপ.empty(), "`` join stack empty, but in a ``");
                ধ্রুৱ string lhs = m_join_স্তূপ.শীৰ্ষ();
                m_join_স্তূপ.বেৰ_কৰো();
                UINFO(5, "`` LHS:" << lhs);
                ধ্রুৱ string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ধ্রুৱ string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ধ্রুৱ string lhs = m_join_স্তূপ.শীৰ্ষ();
                m_join_স্তূপ.বেৰ_কৰো();
                unputString(lhs);
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            } অন্যথা {
                // `define, etc, fall through and expand.  Pop back here.
                ভঙ্গ;
            }
        }
        পক্ষ ps_STRIFY: {
            যদি (tok == VP_STRIFY) {
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
                state_বেৰ_কৰো();
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                ভঙ্গ;
            } অন্যথা যদি (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                গচ্ছ next_tok;
            } অন্যথা যদি (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                ভঙ্গ;
            } অন্যথা {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                গচ্ছ next_tok;
            }
        }
        স্ৱভাৱ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        ৱিকল্প (tok) {
        পক্ষ VP_INCLUDE:
            যদি (!m_off) {
                state_ঠেলো(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            গচ্ছ next_tok;
        পক্ষ VP_UNDEF: state_ঠেলো(ps_DEFNAME_UNDEF); গচ্ছ next_tok;
        পক্ষ VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ঠেলো(ps_DEFNAME_DEFINE);
            গচ্ছ next_tok;
        পক্ষ VP_IFDEF: state_ঠেলো(ps_DEFNAME_IFDEF); গচ্ছ next_tok;
        পক্ষ VP_IFNDEF: state_ঠেলো(ps_DEFNAME_IFNDEF); গচ্ছ next_tok;
        পক্ষ VP_ELSIF: state_ঠেলো(ps_DEFNAME_ELSIF); গচ্ছ next_tok;
        পক্ষ VP_ELSE:
            যদি (m_ifdef_স্তূপ.empty()) {
                fileline()->v3error("`else with no matching `if");
            } অন্যথা {
                ধ্রুৱ VPreIfEntry lastIf = m_ifdef_স্তূপ.শীৰ্ষ();
                m_ifdef_স্তূপ.বেৰ_কৰো();
                ধ্রুৱ তার্কিক enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                যদি (!lastIf.on()) parsingOn();
                যদি (!enable) parsingOff();
            }
            গচ্ছ next_tok;
        পক্ষ VP_ENDIF:
            UINFO(4, "Endif ");
            যদি (m_ifdef_স্তূপ.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } অন্যথা {
                ধ্রুৱ VPreIfEntry lastIf = m_ifdef_স্তূপ.শীৰ্ষ();
                m_ifdef_স্তূপ.বেৰ_কৰো();
                যদি (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            গচ্ছ next_tok;

        পক্ষ VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ধ্রুৱ string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            যদি (m_defPutJoin) {
                m_defPutJoin = মিছা;
                unputString("``");
            }
            যদি (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                গচ্ছ next_tok;
            }
            // Substitute
            যদি (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                যদি (m_off) {
                    গচ্ছ next_tok;
                } অন্যথা {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    গচ্ছ next_tok;
                }
            } অন্যথা {
                ধ্রুৱ string params = defParams(name);
                যদি (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    যদি (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    যদি (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        যদি (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_স্তূপ.empty(), "`` join stack empty, but in a ``");
                            ধ্রুৱ string lhs = m_join_স্তূপ.শীৰ্ষ();
                            m_join_স্তূপ.বেৰ_কৰো();
                            out.ঢোকাও(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_বেৰ_কৰো();
                        }
                        যদি (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        অন্যথা যদি (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } অন্যথা {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ধ্রুৱ refp = &(m_defRefs.শীৰ্ষ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_মান + out);
                        m_lexp->m_def_মান = "";
                    }
                    গচ্ছ next_tok;
                } অন্যথা {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    যদি (!m_defRefs.empty()) m_defRefs.শীৰ্ষ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ঠেলো(ps_DEFPAREN);
                    m_lexp->ঠেলো_State_Def_Arg(0);
                    গচ্ছ next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            গচ্ছ next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        পক্ষ VP_ERROR: {
            state_ঠেলো(ps_ERRORNAME);
            গচ্ছ next_tok;
        }
        পক্ষ VP_EOF:
            যদি (!m_ifdef_স্তূপ.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            ঘূৰাই tok;
        পক্ষ VP_UNDEFINEALL:
            যদি (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            গচ্ছ next_tok;
        পক্ষ VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ঠেলো(ps_STRIFY);
            গচ্ছ next_tok;
        পক্ষ VP_প্ৰতীক:
        পক্ষ VP_STRING:
        পক্ষ VP_TEXT: {
            m_defDepth = 0;
            যদি (!m_off) {
                ঘূৰাই tok;
            } অন্যথা {
                গচ্ছ next_tok;
            }
        }
        পক্ষ VP_WHITE:  // Handled at top of loop
        পক্ষ VP_COMMENT:  // Handled at top of loop
        পক্ষ VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        পক্ষ VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        স্ৱভাৱ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            ভঙ্গ;  // LCOV_EXCL_LINE
        }
        ঘূৰাই tok;
    }
}

পূর্ণাঙ্ক V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    যদি (!m_finAhead) {
        m_finAhead = সঁচা;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ধ্রুৱ পূর্ণাঙ্ক tok = m_finToken;
    buf = m_finBuf;
    যদি (মিছা && debug() >= 5) {
        ধ্রুৱ string bufcln = V3PreLex::cleanDbgStrg(buf);
        ধ্রুৱ string flcol = m_lexp->m_tokFilelinep->ascii_সাৰি_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ধ্রুৱ ৱর্ণ* bufp = buf.c_str();
    যেতিয়ালৈ (*bufp == '\n') bufp++;
    যদি ((tok == VP_TEXT || tok == VP_সাৰি) && VString::startsWith(bufp, "`line ")) {
        পূর্ণাঙ্ক enter;
        m_finFilelinep->সাৰি_Directive(bufp, enter /*ref*/);
    } অন্যথা {
        যদি (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->সাৰি_Directives()) {
            যদি (ধ্রুৱ পূর্ণাঙ্ক outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                যদি (debug() >= 5) {
                    ধ্রুৱ string flcol = m_lexp->m_tokFilelinep->ascii_সাৰি_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                যদি (outBehind > 0
                    && (outBehind <= static_cast<পূর্ণাঙ্ক>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    যদি (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        ঘূৰাই VP_TEXT;
                    }
                } অন্যথা {
                    // Need to backup, use `line
                    buf = m_finFilelinep->সাৰি_Directive_Strg(0);
                    ঘূৰাই VP_সাৰি;
                }
            }
        }
        // Track newlines in prep for next token
        বাবে (ধ্রুৱ ৱর্ণ& c : buf) {
            যদি (c == '\n') {
                m_finAtBol = সঁচা;
                m_finFilelinep->linenoInc();
            } অন্যথা {
                m_finAtBol = মিছা;
            }
        }
    }
    m_finAhead = মিছা;  // Consumed the token
    ঘূৰাই tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    যদি (isEof()) ঘূৰাই "";
    ধ্রুৱ ৱর্ণ* rtnp;
    তার্কিক gotEof = মিছা;
    যেতিয়ালৈ (nullptr == (rtnp = std::strchr(m_সাৰি_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ধ্রুৱ পূর্ণাঙ্ক tok = getFinalToken(m_tokenBuf /*ref*/);
        যদি (debug() >= 5) {
            ধ্রুৱ string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ধ্রুৱ string flcol = m_lexp->m_tokFilelinep->ascii_সাৰি_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        যদি (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            যদি (m_সাৰি_Chars != "" && m_সাৰি_Chars[m_সাৰি_Chars.length() - 1] != '\n') {
                m_সাৰি_Chars.append("\n");
            }
            gotEof = সঁচা;
        } অন্যথা {
            m_সাৰি_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ধ্রুৱ পূর্ণাঙ্ক len = rtnp - m_সাৰি_Chars.c_str() + 1;
    string the_সাৰি(m_সাৰি_Chars, 0, len);
    m_সাৰি_Chars = m_সাৰি_Chars.erase(0, len);  // Remove returned characters
    যদি (debug() >= 4) {
        ধ্রুৱ string lncln = V3PreLex::cleanDbgStrg(the_সাৰি);
        ধ্রুৱ string flcol = m_lexp->m_tokFilelinep->ascii_সাৰি_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ঘূৰাই the_সাৰি;
}
