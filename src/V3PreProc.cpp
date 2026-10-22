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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3PreProc.h"

#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3File.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3LanguageWords.h"
#অন্তর্ভাব "V3PreExpr.h"
#অন্তর্ভাব "V3PreLex.h"
#অন্তর্ভাব "V3PreShell.h"
#অন্তর্ভাব "V3String.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cstdlib>
#অন্তর্ভাব <fstream>
#অন্তর্ভাব <stack>
#অন্তর্ভাব <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#অন্তর্ভাব "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

শ্রেণী VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_সারি* ধ্রুবক m_fileline;  // Where it was declared
    ধ্রুবক string m_মান;  // Value of define
    ধ্রুবক string m_params;  // Parameters
    ধ্রুবক যৌক্তিক m_cmdline;  // Set on command line, don't `undefineall
সর্বজনীন:
    VDefine(File_সারি* fl, ধ্রুবক string& মান, ধ্রুবক string& params, যৌক্তিক cmdline)
        : m_fileline{fl}
        , m_মান{মান}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_সারি* fileline() ধ্রুবক { ফেরত m_fileline; }
    string মান() ধ্রুবক { ফেরত m_মান; }
    string params() ধ্রুবক { ফেরত m_params; }
    যৌক্তিক cmdline() ধ্রুবক { ফেরত m_cmdline; }
};

//*************************************************************************

শ্রেণী VDefineRef final {
    // One for each pending define substitution
    ধ্রুবক string m_name;  // Define last name being defined
    ধ্রুবক string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    পূর্ণসংখ্যা m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
সর্বজনীন:
    string name() ধ্রুবক { ফেরত m_name; }
    string params() ধ্রুবক { ফেরত m_params; }
    string nextarg() ধ্রুবক { ফেরত m_nextarg; }
    শূন্য nextarg(ধ্রুবক string& মান) { m_nextarg = মান; }
    পূর্ণসংখ্যা parenLevel() ধ্রুবক { ফেরত m_parenLevel; }
    শূন্য parenLevel(পূর্ণসংখ্যা মান) { m_parenLevel = মান; }
    std::vector<string>& args() { ফেরত m_args; }
    VDefineRef(ধ্রুবক string& name, ধ্রুবক string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = পূর্বনির্ধারিত;
};

//*************************************************************************
/// Data for parsing on/off

শ্রেণী VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ধ্রুবক যৌক্তিক m_on;  // Current parse for this ifdef level is "on"
    ধ্রুবক যৌক্তিক m_everOn;  // Some if term in elsif tree has been on
সর্বজনীন:
    যৌক্তিক on() ধ্রুবক { ফেরত m_on; }
    যৌক্তিক everOn() ধ্রুবক { ফেরত m_everOn; }
    VPreIfEntry(যৌক্তিক on, যৌক্তিক everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = পূর্বনির্ধারিত;
};

//*************************************************************************
// Data for a preprocessor instantiation.

শ্রেণী V3PreProcImp final : সর্বজনীন V3PreProc {
সর্বজনীন:
    // TYPES
    using DefinesMap = std::map<ধ্রুবক std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ধ্রুবক V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    পূর্ণসংখ্যা m_lastLineno = 0;  // Last line number (stall detection)
    পূর্ণসংখ্যা m_tokens_On_সারি = 0;  // Number of tokens on line (stall detection)

    গণনা ProcState : uint8_t {
        ps_শীর্ষ,
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
        ps_রাশিমালা_IFDEF,
        ps_রাশিমালা_IFNDEF,
        ps_রাশিমালা_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    স্থির ধ্রুবক অক্ষর* procStateName(ProcState s) {
        স্থির ধ্রুবক অক্ষর* ধ্রুবক states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ফেরত states[s];
    }

    std::স্তূপ<ProcState> m_states;  ///< Current state of parser
    পূর্ণসংখ্যা m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    যৌক্তিক m_incError = মিথ্যা;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_সারি_Cmt;  ///< Line comment(s) to be returned
    যৌক্তিক m_সারি_Cmt_Nl = মিথ্যা;  ///< Newline needed before inserting lineCmt
    পূর্ণসংখ্যা m_সারি_Add = 0;  ///< Empty lines to return to maintain line count
    যৌক্তিক m_rawAtBol = সত্য;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    যৌক্তিক m_finAhead = মিথ্যা;  ///< Have read a token ahead
    পূর্ণসংখ্যা m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    যৌক্তিক m_finAtBol = সত্য;  ///< Last getFinalToken left us at beginning of line
    File_সারি* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::স্তূপ<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::স্তূপ<VPreIfEntry> m_ifdef_স্তূপ;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    যৌক্তিক m_defPutJoin = মিথ্যা;  ///< Insert `` after substitution

    // For `` join
    std::স্তূপ<string> m_join_স্তূপ;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_রাশিমালা m_রাশিমালা_Parser;  ///< Parser for () expression
    পূর্ণসংখ্যা m_রাশিমালা_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_সারি_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    শূন্য v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    স্থির ধ্রুবক অক্ষর* tokenName(পূর্ণসংখ্যা tok);
    শূন্য debugToken(পূর্ণসংখ্যা tok, ধ্রুবক অক্ষর* cmtp);
    শূন্য parse_শীর্ষ();
    শূন্য parseUndef();

ব্যক্তিগত:
    // Internal methods
    শূন্য endOfOneFile();
    string defineSubst(VDefineRef* refp);

    যৌক্তিক defExists(ধ্রুবক string& name);
    যৌক্তিক defCmdline(ধ্রুবক string& name);
    string def_মান(ধ্রুবক string& name);
    string defParams(ধ্রুবক string& name);
    File_সারি* defFileline(ধ্রুবক string& name);

    string commentCleanup(ধ্রুবক string& text);
    যৌক্তিক commentTokenMatch(string& cmdr, ধ্রুবক অক্ষর* strg);
    স্থির string trimWhitespace(ধ্রুবক string& strg, যৌক্তিক trailing);
    শূন্য unputString(ধ্রুবক string& strg);
    শূন্য unputDefrefString(ধ্রুবক string& strg);

    শূন্য parsingOn() {
        m_off--;
        যদি (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    শূন্য parsingOff() { m_off++; }

    পূর্ণসংখ্যা getRawToken();
    পূর্ণসংখ্যা getStateToken();
    পূর্ণসংখ্যা getFinalToken(string& buf);

    ProcState state() ধ্রুবক { ফেরত m_states.শীর্ষ(); }
    যৌক্তিক stateIsDefname() ধ্রুবক {
        ফেরত state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    শূন্য state_ঠেলো(ProcState state) { m_states.ঠেলো(state); }
    শূন্য state_বের_করো() {
        m_states.বের_করো();
        যদি (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.ঠেলো(ps_শীর্ষ);  // LCOV_EXCL_LINE
        }
    }
    শূন্য stateChange(ProcState state) {
        state_বের_করো();
        state_ঠেলো(state);
    }

সর্বজনীন:
    // METHODS, called from upper level shell
    শূন্য openFile(File_সারি* fl, VInFilter* filterp, ধ্রুবক string& filename) override;
    যৌক্তিক isEof() ধ্রুবক override { ফেরত m_lexp->curStreamp()->m_eof; }
    শূন্য forceEof() { m_lexp->curStreamp()->m_eof = সত্য; }
    string getline() override;
    শূন্য ঢোকাও_Unreadback(ধ্রুবক string& text) override { m_সারি_Cmt += text; }
    শূন্য ঢোকাও_Unreadback_At_Bol(ধ্রুবক string& text);
    শূন্য add_সারি_Comment(পূর্ণসংখ্যা enterExit);
    শূন্য dumpDefines(std::ostream& os) override;
    শূন্য candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    শূন্য comment(ধ্রুবক string& text) override;  // Comment detected (if keepComments==2)
    শূন্য include(ধ্রুবক string& filename) override;  // Request a include file be processed
    শূন্য undef(ধ্রুবক string& name) override;
    আভাসী শূন্য undefineall();
    শূন্য define(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& মান, ধ্রুবক string& params,
                যৌক্তিক cmdline) override;
    string removeDefines(ধ্রুবক string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.ঠেলো(ps_শীর্ষ); }
    // cppcheck-suppress duplInheritedMember
    শূন্য configure(File_সারি* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = এই;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = নতুন File_সারি{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = নতুন V3PreLex{এই, filelinep};
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

V3PreProc* V3PreProc::createPreProc(File_সারি* fl) {
    V3PreProcImp* ধ্রুবক preprocp = নতুন V3PreProcImp;
    preprocp->configure(fl);
    ফেরত preprocp;
}

শূন্য V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_রাশিমালা::selfTest(); }

//*************************************************************************
// Defines

শূন্য V3PreProcImp::undef(ধ্রুবক string& name) {
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ঢোকাও_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
শূন্য V3PreProcImp::undefineall() {
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        ঢোকাও_Unreadback("`undefineall\n");
    জন্য (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        যদি (!it->second.cmdline()) m_defines.erase(it);
    }
}
যৌক্তিক V3PreProcImp::defExists(ধ্রুবক string& name) {
    ধ্রুবক auto iter = m_defines.find(name);
    ফেরত (iter != m_defines.end());
}
যৌক্তিক V3PreProcImp::defCmdline(ধ্রুবক string& name) {
    ধ্রুবক auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) { ফেরত মিথ্যা; }
    ফেরত iter->second.cmdline();
}
string V3PreProcImp::def_মান(ধ্রুবক string& name) {
    ধ্রুবক auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ফেরত "";
    }
    ফেরত iter->second.মান();
}
string V3PreProcImp::defParams(ধ্রুবক string& name) {
    ধ্রুবক auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ফেরত "";
    }
    ফেরত iter->second.params();
}
File_সারি* V3PreProcImp::defFileline(ধ্রুবক string& name) {
    ধ্রুবক auto iter = m_defines.find(name);
    যদি (iter == m_defines.end()) ফেরত nullptr;
    ফেরত iter->second.fileline();
}
শূন্য V3PreProcImp::define(File_সারি* fl, ধ্রুবক string& name, ধ্রুবক string& মান,
                          ধ্রুবক string& params, যৌক্তিক cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << মান << "' params '" << params << "'");
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(মান.আকার_2());
        জন্য (string::আকার_type pos = 0; pos < মান.আকার_2(); ++pos) {
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
                ফেরত;
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

string V3PreProcImp::removeDefines(ধ্রুবক string& text) {
    string val;
    string rtnsym = text;
    জন্য (পূর্ণসংখ্যা loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        যদি (xsym[0] == '`') xsym.erase(0, 1);
        যদি (defExists(xsym)) {
            val = def_মান(xsym);
            যদি (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } অন্যথা {
                বিরতি;
            }
        } অন্যথা {
            বিরতি;
        }
    }
    ফেরত rtnsym;  // NA
}

//**********************************************************************
// Comments

শূন্য V3PreProcImp::include(ধ্রুবক string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ধ্রুবক string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::আকার_type pos;
    যখন ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    যখন ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    যখন ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    যখন ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    যখন ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    যখন ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    যখন (!cmd.empty() && std::isspace(cmd[cmd.আকার_2() - 1])) cmd.erase(cmd.আকার_2() - 1);
    ফেরত cmd;
}

যৌক্তিক V3PreProcImp::commentTokenMatch(string& cmdr, ধ্রুবক অক্ষর* strg) {
    পূর্ণসংখ্যা len = std::strlen(strg);
    যদি (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        যদি (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ফেরত সত্য;
    } অন্যথা {
        ফেরত মিথ্যা;
    }
}

শূন্য V3PreProcImp::comment(ধ্রুবক string& text) {
    // Comment detected.  Only keep relevant data.
    যৌক্তিক printed = মিথ্যা;
    যদি (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ঢোকাও_Unreadback(text);
        printed = সত্য;
    }

    ধ্রুবক অক্ষর* cp = text.c_str();
    যদি (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } অন্যথা {
        ফেরত;
    }

    যখন (std::isspace(*cp)) ++cp;

    যৌক্তিক synth = মিথ্যা;
    যৌক্তিক vlcomment = মিথ্যা;
    যদি ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        যদি (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ফেরত;
        }
        vlcomment = সত্য;
    } অন্যথা যদি (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        যদি (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ফেরত;
        }
        synth = সত্য;
    } অন্যথা যদি (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = সত্য;
    } অন্যথা যদি (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = সত্য;
    } অন্যথা যদি (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = সত্য;
    } অন্যথা {
        ফেরত;
    }

    যখন (std::isspace(*cp)) ++cp;
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
            string::আকার_type endOfCmd = std::strlen("public_flat_rw");
            যখন (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            যখন (std::isspace(arg[0])) arg = arg.substr(1);
            যদি (arg.আকার_2() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            যদি (!printed) ঢোকাও_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } অন্যথা যদি (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::আকার_type startOfArg = baseCmd.আকার_2();
            যখন (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            যদি (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ফেরত;
            }
            যদি (!printed) ঢোকাও_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } অন্যথা {
            যদি (!printed) ঢোকাও_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_সারি* V3PreProc::fileline() {
    ধ্রুবক V3PreProcImp* idatap = static_cast<V3PreProcImp*>(এই);
    ফেরত idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ধ্রুবক অক্ষর* V3PreProcImp::tokenName(পূর্ণসংখ্যা tok) {
    নির্বাচন (tok) {
    ক্ষেত্র VP_BACKQUOTE: ফেরত ("BACKQUOTE");
    ক্ষেত্র VP_COMMENT: ফেরত ("COMMENT");
    ক্ষেত্র VP_DEFARG: ফেরত ("DEFARG");
    ক্ষেত্র VP_DEFFORM: ফেরত ("DEFFORM");
    ক্ষেত্র VP_DEFINE: ফেরত ("DEFINE");
    ক্ষেত্র VP_DEFREF: ফেরত ("DEFREF");
    ক্ষেত্র VP_DEFREF_JOIN: ফেরত ("DEFREF_JOIN");
    ক্ষেত্র VP_DEFVALUE: ফেরত ("DEFVALUE");
    ক্ষেত্র VP_ELSE: ফেরত ("ELSE");
    ক্ষেত্র VP_ELSIF: ফেরত ("ELSIF");
    ক্ষেত্র VP_ENDIF: ফেরত ("ENDIF");
    ক্ষেত্র VP_EOF: ফেরত ("EOF");
    ক্ষেত্র VP_EOF_ERROR: ফেরত ("EOF_ERROR");
    ক্ষেত্র VP_ERROR: ফেরত ("ERROR");
    ক্ষেত্র VP_IFDEF: ফেরত ("IFDEF");
    ক্ষেত্র VP_IFNDEF: ফেরত ("IFNDEF");
    ক্ষেত্র VP_JOIN: ফেরত ("JOIN");
    ক্ষেত্র VP_INCLUDE: ফেরত ("INCLUDE");
    ক্ষেত্র VP_সারি: ফেরত ("LINE");
    ক্ষেত্র VP_STRIFY: ফেরত ("STRIFY");
    ক্ষেত্র VP_STRING: ফেরত ("STRING");
    ক্ষেত্র VP_প্রতীক: ফেরত ("SYMBOL");
    ক্ষেত্র VP_প্রতীক_JOIN: ফেরত ("SYMBOL_JOIN");
    ক্ষেত্র VP_TEXT: ফেরত ("TEXT");
    ক্ষেত্র VP_UNDEF: ফেরত ("UNDEF");
    ক্ষেত্র VP_UNDEFINEALL: ফেরত ("UNDEFINEALL");
    ক্ষেত্র VP_WHITE: ফেরত ("WHITE");
    পূর্বনির্ধারিত: ফেরত ("?");
    }
}

শূন্য V3PreProcImp::unputString(ধ্রুবক string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    যদি (VL_UNCOVERABLE(m_lexp->m_বাফার_State != m_lexp->current_বাফার())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

শূন্য V3PreProcImp::unputDefrefString(ধ্রুবক string& strg) {
    ধ্রুবক পূর্ণসংখ্যা multiline = std::গুনতি(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ধ্রুবক string& strg, যৌক্তিক trailing) {
    // Remove leading whitespace
    string out = strg;
    string::আকার_type leadspace = 0;
    যখন (out.দৈর্ঘ্য() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    যদি (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    যদি (trailing) {
        string::আকার_type trailspace = 0;
        যখন (out.দৈর্ঘ্য() > trailspace && std::isspace(out[out.দৈর্ঘ্য() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        যদি (trailspace && out.দৈর্ঘ্য() > trailspace && out[out.দৈর্ঘ্য() - 1 - trailspace] == '\\')
            --trailspace;
        যদি (trailspace) out.erase(out.দৈর্ঘ্য() - trailspace, trailspace);
    }
    ফেরত out;
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
    জন্য (unsigned i = 0; i < refp->args().আকার_2(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ধ্রুবক string মান = def_মান(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(মান) << "'");

    std::map<ধ্রুবক string, string> arg_মান_By_Name;
    {  // Parse argument list into map
        unsigned অঙ্ক_Args = 0;
        string argName;
        পূর্ণসংখ্যা paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        যৌক্তিক quote = মিথ্যা;
        যৌক্তিক haveDefault = মিথ্যা;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ধ্রুবক string params = refp->params();  // Must keep str in scope to get pointer
        ধ্রুবক অক্ষর* cp = params.c_str();
        যদি (*cp == '(') cp++;
        জন্য (; *cp; cp++) {
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
                    argName = trimWhitespace(argName, সত্য);
                    UINFO(4, "    Got Arg=" << অঙ্ক_Args << "  argName='" << argName
                                            << "'  default='" << মান_Def << "'");
                    // Parse it
                    যদি (argName != "") {
                        যদি (refp->args().আকার_2() > অঙ্ক_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ধ্রুবক string arg = trimWhitespace(refp->args()[অঙ্ক_Args], সত্য);
                            যদি (arg != "") মান_Def = arg;
                        } অন্যথা যদি (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ফেরত " `" + refp->name() + " ";
                        }
                        অঙ্ক_Args++;
                    }
                    arg_মান_By_Name[argName] = মান_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = মিথ্যা;
                    চালু;
                } অন্যথা যদি (*cp == '=') {
                    haveDefault = সত্য;
                    argName = token;
                    token = "";
                    চালু;
                }
            }
            যদি (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                চালু;
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
        যদি (refp->args().আকার_2() > অঙ্ক_Args
            // `define X() is ok to call with nothing
            && !(refp->args().আকার_2() == 1 && অঙ্ক_Args == 0
                 && trimWhitespace(refp->args()[0], মিথ্যা) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ফেরত " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        যৌক্তিক quote = মিথ্যা;
        যৌক্তিক triquote = মিথ্যা;
        যৌক্তিক backslashesc = মিথ্যা;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        জন্য (ধ্রুবক অক্ষর* cp = মান.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            যদি (!quote && !triquote && *cp == '\\') {
                backslashesc = সত্য;
            } অন্যথা যদি (std::isspace(*cp)) {
                backslashesc = মিথ্যা;
            }
            যদি (!quote && !triquote) {
                যদি (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    চালু;
                }
                যদি (argName != "") {
                    // Found a possible variable substitution
                    ধ্রুবক auto iter = arg_মান_By_Name.find(argName);
                    যদি (iter != arg_মান_By_Name.end()) {
                        // Substitute
                        ধ্রুবক string subst = iter->second;
                        যদি (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            যদি (out.আকার_2() >= 2 && out.substr(out.আকার_2() - 2) == "``") {
                                out.resize(out.আকার_2() - 2);
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
                    চালু;
                } অন্যথা যদি (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    চালু;
                } অন্যথা যদি (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    চালু;
                } অন্যথা যদি (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    চালু;
                } অন্যথা যদি (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    চালু;
                }
            }
            যদি (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                চালু;
            } অন্যথা যদি (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                চালু;
            }
            যদি (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                চালু;
            }
            যদি (*cp == '"') quote = !quote;
            যদি (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    ফেরত out;
}

//**********************************************************************
// Parser routines

শূন্য V3PreProcImp::openFile(File_সারি*, VInFilter* filterp, ধ্রুবক string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    যদি (m_incError) ফেরত;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ধ্রুবক যৌক্তিক ok = filterp->readWholefile(filename, wholefile /*ref*/);
    যদি (!ok) {
        fileline()->v3error("File not found: " + filename);
        ফেরত;
    }

    যদি (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        যদি (m_lexp->m_streamp_স্তূপ.আকার_2() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = সত্য;
            ফেরত;
        }
        // There's already a file active.  Push it to work on the new one.
        add_সারি_Comment(0);
    }

    // Save file contents for future error reporting
    File_সারি* ধ্রুবক flsp = নতুন File_সারি{filename};
    flsp->lineno(1);
    flsp->newContent();
    জন্য (ধ্রুবক string& i : wholefile) flsp->contentp()->ঠেলো_Text(i);

    // Save contents for V3Control --contents
    যদি (filename != V3Options::getStdPackagePath()) {
        যৌক্তিক containsVlt = মিথ্যা;
        জন্য (ধ্রুবক string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            যদি (i.find("`verilator_config") != string::npos) {
                containsVlt = সত্য;
                বিরতি;
            }
        }
        যদি (!containsVlt) {
            জন্য (ধ্রুবক string& i : wholefile) V3Control::contents_ঠেলো_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_সারি_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    পূর্ণসংখ্যা eof_newline = 0;  // Number of characters following last newline
    পূর্ণসংখ্যা eof_lineno = 1;
    জন্য (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        যৌক্তিক strip = মিথ্যা;
        ধ্রুবক অক্ষর* ধ্রুবক sp = it->তথ্য();
        ধ্রুবক অক্ষর* ধ্রুবক ep = sp + it->দৈর্ঘ্য();
        // Only process if needed, as saves extra string allocations
        জন্য (ধ্রুবক অক্ষর* cp = sp; cp < ep; cp++) {
            যদি (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = সত্য;
            } অন্যথা যদি (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } অন্যথা {
                ++eof_newline;
            }
        }
        যদি (strip) {
            string out;
            out.reserve(it->দৈর্ঘ্য());
            জন্য (ধ্রুবক অক্ষর* cp = sp; cp < ep; cp++) {
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
        File_সারি* ধ্রুবক fl = নতুন File_সারি{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

শূন্য V3PreProcImp::ঢোকাও_Unreadback_At_Bol(ধ্রুবক string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    যদি (m_সারি_Cmt == "") {
        m_সারি_Cmt_Nl = সত্য;
    } অন্যথা যদি (m_সারি_Cmt[m_সারি_Cmt.দৈর্ঘ্য() - 1] != '\n') {
        ঢোকাও_Unreadback("\n");
    }
    ঢোকাও_Unreadback(text);
}

শূন্য V3PreProcImp::add_সারি_Comment(পূর্ণসংখ্যা enterExit) {
    যদি (সারি_Directives()) {
        ঢোকাও_Unreadback_At_Bol(m_lexp->curFilelinep()->সারি_Directive_Strg(enterExit));
    }
}

শূন্য V3PreProcImp::dumpDefines(std::ostream& os) {
    জন্য (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        যদি (!it->second.params().empty()) os << it->second.params();
        যদি (!it->second.মান().empty()) os << " " << it->second.মান();
        os << '\n';
    }
}

শূন্য V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    জন্য (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->ঠেলো_Candidate("`"s + it->first);
    }
}

পূর্ণসংখ্যা V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    যখন (সত্য) {
    পরবর্তী_tok:
        যদি (m_সারি_Add) {
            m_সারি_Add--;
            m_rawAtBol = সত্য;
            yyourtext("\n", 1);
            যদি (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ফেরত VP_WHITE;
        }
        যদি (m_সারি_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            স্থির string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_সারি_Cmt;
            যদি (m_সারি_Cmt_Nl) {
                যদি (!m_rawAtBol) s_rtncmt.ঢোকাও(0, "\n");
                m_সারি_Cmt_Nl = মিথ্যা;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.দৈর্ঘ্য());
            m_সারি_Cmt = "";
            যদি (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            যদি (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_মান(yyourtext(), yyourleng());
                যাও পরবর্তী_tok;
            } অন্যথা {
                যদি (debug() >= 5) debugToken(VP_TEXT, "LCM");
                ফেরত VP_TEXT;
            }
        }
        যদি (isEof()) ফেরত VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        পূর্ণসংখ্যা tok = m_lexp->lex();
        যদি (debug() >= 5) debugToken(tok, "RAW");

        যদি (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_সারি = 0;
        } অন্যথা যদি (++m_tokens_On_সারি > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        যদি (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            যদি (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            যাও পরবর্তী_tok;
        }

        যদি (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ফেরত tok;
    }
}

শূন্য V3PreProcImp::debugToken(পূর্ণসংখ্যা tok, ধ্রুবক অক্ষর* cmtp) {
    ধ্রুবক স্থির পূর্ণসংখ্যা s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    যদি (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::আকার_type pos;
        যখন ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        যখন ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ধ্রুবক string flcol = m_lexp->m_tokFilelinep->ascii_সারি_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<পূর্ণসংখ্যা>(m_states.আকার_2())
                       << ") dr" << m_defRefs.আকার_2() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        যদি (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

পূর্ণসংখ্যা V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    যখন (সত্য) {
    পরবর্তী_tok:
        যদি (isEof()) ফেরত VP_EOF;
        পূর্ণসংখ্যা tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        যদি (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ফেরত tok;
        যদি (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        যদি (tok == VP_COMMENT) {
            যদি (!m_off) {
                যদি (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    যদি (m_সারি_Cmt.empty()) ঢোকাও_Unreadback(" ");
                } অন্যথা যদি (m_lexp->m_keepComments) {
                    ফেরত tok;
                } অন্যথা {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ঢোকাও_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            জন্য (আকার_t len = 0; len < static_cast<আকার_t>(yyourleng()); len++) {
                যদি (yyourtext()[len] == '\n') m_সারি_Add++;
            }
            যাও পরবর্তী_tok;
        }
        যদি (tok == VP_সারি) {
            add_সারি_Comment(m_lexp->m_enterExit);
            যাও পরবর্তী_tok;
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
            ধ্রুবক std::string name(yyourtext() + 1, yyourleng() - 1);
            যদি (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = সত্য;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } অন্যথা {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        যদি (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            যদি (tok == VP_JOIN) যাও পরবর্তী_tok;
            যদি (tok == VP_প্রতীক_JOIN) tok = VP_প্রতীক;
        }
        যদি (tok == VP_প্রতীক_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ধ্রুবক std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_স্তূপ.ঠেলো(out);
            state_ঠেলো(ps_JOIN);
            যাও পরবর্তী_tok;
        }

        // Deal with some special parser states
        নির্বাচন (state()) {
        ক্ষেত্র ps_শীর্ষ: {
            বিরতি;
        }
        ক্ষেত্র ps_DEFNAME_UNDEF:  // FALLTHRU
        ক্ষেত্র ps_DEFNAME_DEFINE:  // FALLTHRU
        ক্ষেত্র ps_DEFNAME_IFDEF:  // FALLTHRU
        ক্ষেত্র ps_DEFNAME_IFNDEF:  // FALLTHRU
        ক্ষেত্র ps_DEFNAME_ELSIF: {
            যদি (tok == VP_প্রতীক) {
                m_lastSym.assign(yyourtext(), yyourleng());
                যদি (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    যৌক্তিক enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    যদি (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, মিথ্যা});
                    যদি (!enable) parsingOff();
                    state_বের_করো();
                    যাও পরবর্তী_tok;
                } অন্যথা যদি (state() == ps_DEFNAME_ELSIF) {
                    যদি (m_ifdef_স্তূপ.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } অন্যথা {
                        // Handle `else portion
                        ধ্রুবক VPreIfEntry lastIf = m_ifdef_স্তূপ.শীর্ষ();
                        m_ifdef_স্তূপ.বের_করো();
                        যদি (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ধ্রুবক যৌক্তিক enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        যদি (!enable) parsingOff();
                    }
                    state_বের_করো();
                    যাও পরবর্তী_tok;
                } অন্যথা যদি (state() == ps_DEFNAME_UNDEF) {
                    যদি (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_বের_করো();
                    যাও পরবর্তী_tok;
                } অন্যথা যদি (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ঠেলো_State_Def_Form();
                    যাও পরবর্তী_tok;
                } অন্যথা {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                যদি (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ঠেলো_State_রাশিমালা();
                    m_রাশিমালা_Parser.reset(fileline());
                    m_রাশিমালা_Paren_Level = 1;
                    নির্বাচন (state()) {
                    ক্ষেত্র ps_DEFNAME_IFDEF: stateChange(ps_রাশিমালা_IFDEF); বিরতি;
                    ক্ষেত্র ps_DEFNAME_IFNDEF: stateChange(ps_রাশিমালা_IFNDEF); বিরতি;
                    ক্ষেত্র ps_DEFNAME_ELSIF: stateChange(ps_রাশিমালা_ELSIF); বিরতি;
                    পূর্বনির্ধারিত: v3fatalSrc("bad case");
                    }
                    যাও পরবর্তী_tok;
                }
                যদি (!m_off) {
                    ফেরত tok;
                } অন্যথা {
                    যাও পরবর্তী_tok;
                }
            } অন্যথা যদি (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                বিরতি;
            } অন্যথা {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র ps_রাশিমালা_IFDEF:  // FALLTHRU
        ক্ষেত্র ps_রাশিমালা_IFNDEF:  // FALLTHRU
        ক্ষেত্র ps_রাশিমালা_ELSIF: {
            // `ifdef ( *here*
            File_সারি* ধ্রুবক flp = m_lexp->m_tokFilelinep;
            যদি (tok == VP_প্রতীক) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ধ্রুবক যৌক্তিক exists = defExists(m_lastSym);
                যদি (exists) {
                    ধ্রুবক std::string মান = def_মান(m_lastSym);
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
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, exists});
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_WHITE) {
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::BRA});
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_রাশিমালা_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                যদি (--m_রাশিমালা_Paren_Level > 0) {
                    m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::KET});
                    যাও পরবর্তী_tok;
                } অন্যথা {
                    // Done with parsing expression
                    যৌক্তিক enable = m_রাশিমালা_Parser.ফলাফল();
                    UINFO(4, "ifdef() result=" << enable);
                    যদি (state() == ps_রাশিমালা_IFDEF || state() == ps_রাশিমালা_IFNDEF) {
                        যদি (state() == ps_রাশিমালা_IFNDEF) enable = !enable;
                        m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, মিথ্যা});
                        যদি (!enable) parsingOff();
                        state_বের_করো();
                        যাও পরবর্তী_tok;
                    } অন্যথা যদি (state() == ps_রাশিমালা_ELSIF) {
                        যদি (m_ifdef_স্তূপ.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } অন্যথা {
                            // Handle `else portion
                            ধ্রুবক VPreIfEntry lastIf = m_ifdef_স্তূপ.শীর্ষ();
                            m_ifdef_স্তূপ.বের_করো();
                            যদি (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            যদি (!enable) parsingOff();
                        }
                        state_বের_করো();
                    }
                    যাও পরবর্তী_tok;
                }
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::LNOT});
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::LAND});
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::LOR});
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::IMP});
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_রাশিমালা_Parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::EQV});
                যাও পরবর্তী_tok;
            } অন্যথা {
                যদি (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    ফেরত tok;
                } অন্যথা {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র ps_DEFFORM: {
            যদি (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_মান;
                যদি (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ঠেলো_State_Def_মান();
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT) {
                // IE, something like comment in formals
                যদি (!m_off) ফেরত tok;
                যাও পরবর্তী_tok;
            } অন্যথা {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র ps_DEFVALUE: {
            স্থির string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            যদি (tok == VP_DEFVALUE) {
                যদি (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_মান)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ধ্রুবক string formals = m_formals;
                string মান = m_lexp->m_def_মান;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                জন্য (আকার_t i = 0; i < formals.দৈর্ঘ্য(); i++) {
                    যদি (formals[i] == '\n') s_newlines += "\n";
                }
                জন্য (আকার_t i = 0; i < মান.দৈর্ঘ্য(); i++) {
                    যদি (মান[i] == '\n') s_newlines += "\n";
                }
                যদি (!m_off) {
                    // Remove leading and trailing whitespace
                    মান = trimWhitespace(মান, সত্য);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(মান) << "'");
                    define(fileline(), m_lastSym, মান, formals, মিথ্যা);
                }
            } অন্যথা {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_বের_করো();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.দৈর্ঘ্য());
            ফেরত VP_WHITE;
        }
        ক্ষেত্র ps_DEFPAREN: {
            যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                যাও পরবর্তী_tok;
            } অন্যথা {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ধ্রুবক VDefineRef* ধ্রুবক refp = &(m_defRefs.শীর্ষ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_বের_করো();
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.শীর্ষ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_মান);
            m_lexp->m_def_মান = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            যদি (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().ঠেলো_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ঠেলো_State_Def_Arg(1);
                refp->nextarg("");
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().ঠেলো_back(refp->nextarg());
                string out;
                যদি (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.বের_করো(), refp);
                state_বের_করো();
                যদি (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    যদি (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_স্তূপ.empty(), "`` join stack empty, but in a ``");
                        ধ্রুবক string lhs = m_join_স্তূপ.শীর্ষ();
                        m_join_স্তূপ.বের_করো();
                        out.ঢোকাও(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_বের_করো();
                    }
                    যদি (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    অন্যথা যদি (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.শীর্ষ().parenLevel();
                } অন্যথা {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.শীর্ষ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_মান + out);
                    m_lexp->m_def_মান = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                বিরতি;
            } অন্যথা যদি (tok == VP_প্রতীক || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ঠেলো(ps_STRIFY);
                যাও পরবর্তী_tok;
            } অন্যথা {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_বের_করো();
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র ps_INCNAME: {
            যদি (tok == VP_STRING) {
                state_বের_করো();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.দৈর্ঘ্য() - 1, 1);
                include(m_lastSym);
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ঠেলো_State_Inc_Filename();
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                বিরতি;
            } অন্যথা {
                state_বের_করো();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                যাও পরবর্তী_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        ক্ষেত্র ps_ERRORNAME: {
            যদি (tok == VP_STRING) {
                যদি (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_বের_করো();
                যাও পরবর্তী_tok;
            } অন্যথা {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_বের_করো();
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র ps_JOIN: {
            যদি (tok == VP_প্রতীক || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_স্তূপ.empty(), "`` join stack empty, but in a ``");
                ধ্রুবক string lhs = m_join_স্তূপ.শীর্ষ();
                m_join_স্তূপ.বের_করো();
                UINFO(5, "`` LHS:" << lhs);
                ধ্রুবক string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ধ্রুবক string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_বের_করো();
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ধ্রুবক string lhs = m_join_স্তূপ.শীর্ষ();
                m_join_স্তূপ.বের_করো();
                unputString(lhs);
                state_বের_করো();
                যাও পরবর্তী_tok;
            } অন্যথা {
                // `define, etc, fall through and expand.  Pop back here.
                বিরতি;
            }
        }
        ক্ষেত্র ps_STRIFY: {
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
                state_বের_করো();
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                বিরতি;
            } অন্যথা যদি (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                যাও পরবর্তী_tok;
            } অন্যথা যদি (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                বিরতি;
            } অন্যথা {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                যাও পরবর্তী_tok;
            }
        }
        পূর্বনির্ধারিত: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        নির্বাচন (tok) {
        ক্ষেত্র VP_INCLUDE:
            যদি (!m_off) {
                state_ঠেলো(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            যাও পরবর্তী_tok;
        ক্ষেত্র VP_UNDEF: state_ঠেলো(ps_DEFNAME_UNDEF); যাও পরবর্তী_tok;
        ক্ষেত্র VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ঠেলো(ps_DEFNAME_DEFINE);
            যাও পরবর্তী_tok;
        ক্ষেত্র VP_IFDEF: state_ঠেলো(ps_DEFNAME_IFDEF); যাও পরবর্তী_tok;
        ক্ষেত্র VP_IFNDEF: state_ঠেলো(ps_DEFNAME_IFNDEF); যাও পরবর্তী_tok;
        ক্ষেত্র VP_ELSIF: state_ঠেলো(ps_DEFNAME_ELSIF); যাও পরবর্তী_tok;
        ক্ষেত্র VP_ELSE:
            যদি (m_ifdef_স্তূপ.empty()) {
                fileline()->v3error("`else with no matching `if");
            } অন্যথা {
                ধ্রুবক VPreIfEntry lastIf = m_ifdef_স্তূপ.শীর্ষ();
                m_ifdef_স্তূপ.বের_করো();
                ধ্রুবক যৌক্তিক enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_স্তূপ.emplace(VPreIfEntry{enable, lastIf.everOn()});
                যদি (!lastIf.on()) parsingOn();
                যদি (!enable) parsingOff();
            }
            যাও পরবর্তী_tok;
        ক্ষেত্র VP_ENDIF:
            UINFO(4, "Endif ");
            যদি (m_ifdef_স্তূপ.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } অন্যথা {
                ধ্রুবক VPreIfEntry lastIf = m_ifdef_স্তূপ.শীর্ষ();
                m_ifdef_স্তূপ.বের_করো();
                যদি (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            যাও পরবর্তী_tok;

        ক্ষেত্র VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ধ্রুবক string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            যদি (m_defPutJoin) {
                m_defPutJoin = মিথ্যা;
                unputString("``");
            }
            যদি (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                যাও পরবর্তী_tok;
            }
            // Substitute
            যদি (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                যদি (m_off) {
                    যাও পরবর্তী_tok;
                } অন্যথা {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    যাও পরবর্তী_tok;
                }
            } অন্যথা {
                ধ্রুবক string params = defParams(name);
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
                            ধ্রুবক string lhs = m_join_স্তূপ.শীর্ষ();
                            m_join_স্তূপ.বের_করো();
                            out.ঢোকাও(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_বের_করো();
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
                        VDefineRef* ধ্রুবক refp = &(m_defRefs.শীর্ষ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_মান + out);
                        m_lexp->m_def_মান = "";
                    }
                    যাও পরবর্তী_tok;
                } অন্যথা {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    যদি (!m_defRefs.empty()) m_defRefs.শীর্ষ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ঠেলো(ps_DEFPAREN);
                    m_lexp->ঠেলো_State_Def_Arg(0);
                    যাও পরবর্তী_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            যাও পরবর্তী_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        ক্ষেত্র VP_ERROR: {
            state_ঠেলো(ps_ERRORNAME);
            যাও পরবর্তী_tok;
        }
        ক্ষেত্র VP_EOF:
            যদি (!m_ifdef_স্তূপ.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            ফেরত tok;
        ক্ষেত্র VP_UNDEFINEALL:
            যদি (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            যাও পরবর্তী_tok;
        ক্ষেত্র VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ঠেলো(ps_STRIFY);
            যাও পরবর্তী_tok;
        ক্ষেত্র VP_প্রতীক:
        ক্ষেত্র VP_STRING:
        ক্ষেত্র VP_TEXT: {
            m_defDepth = 0;
            যদি (!m_off) {
                ফেরত tok;
            } অন্যথা {
                যাও পরবর্তী_tok;
            }
        }
        ক্ষেত্র VP_WHITE:  // Handled at top of loop
        ক্ষেত্র VP_COMMENT:  // Handled at top of loop
        ক্ষেত্র VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        ক্ষেত্র VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        পূর্বনির্ধারিত:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            বিরতি;  // LCOV_EXCL_LINE
        }
        ফেরত tok;
    }
}

পূর্ণসংখ্যা V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    যদি (!m_finAhead) {
        m_finAhead = সত্য;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ধ্রুবক পূর্ণসংখ্যা tok = m_finToken;
    buf = m_finBuf;
    যদি (মিথ্যা && debug() >= 5) {
        ধ্রুবক string bufcln = V3PreLex::cleanDbgStrg(buf);
        ধ্রুবক string flcol = m_lexp->m_tokFilelinep->ascii_সারি_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ধ্রুবক অক্ষর* bufp = buf.c_str();
    যখন (*bufp == '\n') bufp++;
    যদি ((tok == VP_TEXT || tok == VP_সারি) && VString::startsWith(bufp, "`line ")) {
        পূর্ণসংখ্যা enter;
        m_finFilelinep->সারি_Directive(bufp, enter /*ref*/);
    } অন্যথা {
        যদি (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->সারি_Directives()) {
            যদি (ধ্রুবক পূর্ণসংখ্যা outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                যদি (debug() >= 5) {
                    ধ্রুবক string flcol = m_lexp->m_tokFilelinep->ascii_সারি_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                যদি (outBehind > 0
                    && (outBehind <= static_cast<পূর্ণসংখ্যা>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    যদি (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        ফেরত VP_TEXT;
                    }
                } অন্যথা {
                    // Need to backup, use `line
                    buf = m_finFilelinep->সারি_Directive_Strg(0);
                    ফেরত VP_সারি;
                }
            }
        }
        // Track newlines in prep for next token
        জন্য (ধ্রুবক অক্ষর& c : buf) {
            যদি (c == '\n') {
                m_finAtBol = সত্য;
                m_finFilelinep->linenoInc();
            } অন্যথা {
                m_finAtBol = মিথ্যা;
            }
        }
    }
    m_finAhead = মিথ্যা;  // Consumed the token
    ফেরত tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    যদি (isEof()) ফেরত "";
    ধ্রুবক অক্ষর* rtnp;
    যৌক্তিক gotEof = মিথ্যা;
    যখন (nullptr == (rtnp = std::strchr(m_সারি_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ধ্রুবক পূর্ণসংখ্যা tok = getFinalToken(m_tokenBuf /*ref*/);
        যদি (debug() >= 5) {
            ধ্রুবক string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ধ্রুবক string flcol = m_lexp->m_tokFilelinep->ascii_সারি_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        যদি (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            যদি (m_সারি_Chars != "" && m_সারি_Chars[m_সারি_Chars.দৈর্ঘ্য() - 1] != '\n') {
                m_সারি_Chars.append("\n");
            }
            gotEof = সত্য;
        } অন্যথা {
            m_সারি_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ধ্রুবক পূর্ণসংখ্যা len = rtnp - m_সারি_Chars.c_str() + 1;
    string the_সারি(m_সারি_Chars, 0, len);
    m_সারি_Chars = m_সারি_Chars.erase(0, len);  // Remove returned characters
    যদি (debug() >= 4) {
        ধ্রুবক string lncln = V3PreLex::cleanDbgStrg(the_সারি);
        ধ্রুবক string flcol = m_lexp->m_tokFilelinep->ascii_সারি_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ফেরত the_সারি;
}
