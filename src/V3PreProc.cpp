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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3PreProc.h"

#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3LanguageWords.h"
#അന്തര്ഭാവ "V3PreExpr.h"
#അന്തര്ഭാവ "V3PreLex.h"
#അന്തര്ഭാവ "V3PreShell.h"
#അന്തര്ഭാവ "V3String.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cstdlib>
#അന്തര്ഭാവ <fstream>
#അന്തര്ഭാവ <stack>
#അന്തര്ഭാവ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#അന്തര്ഭാവ "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

വർഗ്ഗം VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_വരി* സ്ഥിരം m_fileline;  // Where it was declared
    സ്ഥിരം string m_മൂല്യം;  // Value of define
    സ്ഥിരം string m_params;  // Parameters
    സ്ഥിരം തർക്കം m_cmdline;  // Set on command line, don't `undefineall
പൊതു:
    VDefine(File_വരി* fl, സ്ഥിരം string& മൂല്യം, സ്ഥിരം string& params, തർക്കം cmdline)
        : m_fileline{fl}
        , m_മൂല്യം{മൂല്യം}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_വരി* fileline() സ്ഥിരം { തിരികെ m_fileline; }
    string മൂല്യം() സ്ഥിരം { തിരികെ m_മൂല്യം; }
    string params() സ്ഥിരം { തിരികെ m_params; }
    തർക്കം cmdline() സ്ഥിരം { തിരികെ m_cmdline; }
};

//*************************************************************************

വർഗ്ഗം VDefineRef final {
    // One for each pending define substitution
    സ്ഥിരം string m_name;  // Define last name being defined
    സ്ഥിരം string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    പൂർണ്ണസംഖ്യ m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
പൊതു:
    string name() സ്ഥിരം { തിരികെ m_name; }
    string params() സ്ഥിരം { തിരികെ m_params; }
    string nextarg() സ്ഥിരം { തിരികെ m_nextarg; }
    ശൂന്യം nextarg(സ്ഥിരം string& മൂല്യം) { m_nextarg = മൂല്യം; }
    പൂർണ്ണസംഖ്യ parenLevel() സ്ഥിരം { തിരികെ m_parenLevel; }
    ശൂന്യം parenLevel(പൂർണ്ണസംഖ്യ മൂല്യം) { m_parenLevel = മൂല്യം; }
    std::vector<string>& args() { തിരികെ m_args; }
    VDefineRef(സ്ഥിരം string& name, സ്ഥിരം string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = സ്ഥിരസ്ഥിതി;
};

//*************************************************************************
/// Data for parsing on/off

വർഗ്ഗം VPreIfEntry final {
    // One for each pending ifdef/ifndef
    സ്ഥിരം തർക്കം m_on;  // Current parse for this ifdef level is "on"
    സ്ഥിരം തർക്കം m_everOn;  // Some if term in elsif tree has been on
പൊതു:
    തർക്കം on() സ്ഥിരം { തിരികെ m_on; }
    തർക്കം everOn() സ്ഥിരം { തിരികെ m_everOn; }
    VPreIfEntry(തർക്കം on, തർക്കം everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = സ്ഥിരസ്ഥിതി;
};

//*************************************************************************
// Data for a preprocessor instantiation.

വർഗ്ഗം V3PreProcImp final : പൊതു V3PreProc {
പൊതു:
    // TYPES
    using DefinesMap = std::map<സ്ഥിരം std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    സ്ഥിരം V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    പൂർണ്ണസംഖ്യ m_lastLineno = 0;  // Last line number (stall detection)
    പൂർണ്ണസംഖ്യ m_tokens_On_വരി = 0;  // Number of tokens on line (stall detection)

    എണ്ണൽ ProcState : uint8_t {
        ps_മുകൾ,
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
        ps_വ്യഞ്ജകം_IFDEF,
        ps_വ്യഞ്ജകം_IFNDEF,
        ps_വ്യഞ്ജകം_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    സ്ഥിതം സ്ഥിരം അക്ഷരം* procStateName(ProcState s) {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        തിരികെ states[s];
    }

    std::അട്ടി<ProcState> m_states;  ///< Current state of parser
    പൂർണ്ണസംഖ്യ m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    തർക്കം m_incError = അസത്യം;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_വരി_Cmt;  ///< Line comment(s) to be returned
    തർക്കം m_വരി_Cmt_Nl = അസത്യം;  ///< Newline needed before inserting lineCmt
    പൂർണ്ണസംഖ്യ m_വരി_Add = 0;  ///< Empty lines to return to maintain line count
    തർക്കം m_rawAtBol = സത്യം;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    തർക്കം m_finAhead = അസത്യം;  ///< Have read a token ahead
    പൂർണ്ണസംഖ്യ m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    തർക്കം m_finAtBol = സത്യം;  ///< Last getFinalToken left us at beginning of line
    File_വരി* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::അട്ടി<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::അട്ടി<VPreIfEntry> m_ifdef_അട്ടി;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    തർക്കം m_defPutJoin = അസത്യം;  ///< Insert `` after substitution

    // For `` join
    std::അട്ടി<string> m_join_അട്ടി;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_വ്യഞ്ജകം m_വ്യഞ്ജകം_Parser;  ///< Parser for () expression
    പൂർണ്ണസംഖ്യ m_വ്യഞ്ജകം_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_വരി_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    ശൂന്യം v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    സ്ഥിതം സ്ഥിരം അക്ഷരം* tokenName(പൂർണ്ണസംഖ്യ tok);
    ശൂന്യം debugToken(പൂർണ്ണസംഖ്യ tok, സ്ഥിരം അക്ഷരം* cmtp);
    ശൂന്യം parse_മുകൾ();
    ശൂന്യം parseUndef();

സ്വകാര്യ:
    // Internal methods
    ശൂന്യം endOfOneFile();
    string defineSubst(VDefineRef* refp);

    തർക്കം defExists(സ്ഥിരം string& name);
    തർക്കം defCmdline(സ്ഥിരം string& name);
    string def_മൂല്യം(സ്ഥിരം string& name);
    string defParams(സ്ഥിരം string& name);
    File_വരി* defFileline(സ്ഥിരം string& name);

    string commentCleanup(സ്ഥിരം string& text);
    തർക്കം commentTokenMatch(string& cmdr, സ്ഥിരം അക്ഷരം* strg);
    സ്ഥിതം string trimWhitespace(സ്ഥിരം string& strg, തർക്കം trailing);
    ശൂന്യം unputString(സ്ഥിരം string& strg);
    ശൂന്യം unputDefrefString(സ്ഥിരം string& strg);

    ശൂന്യം parsingOn() {
        m_off--;
        എങ്കിൽ (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    ശൂന്യം parsingOff() { m_off++; }

    പൂർണ്ണസംഖ്യ getRawToken();
    പൂർണ്ണസംഖ്യ getStateToken();
    പൂർണ്ണസംഖ്യ getFinalToken(string& buf);

    ProcState state() സ്ഥിരം { തിരികെ m_states.മുകൾ(); }
    തർക്കം stateIsDefname() സ്ഥിരം {
        തിരികെ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    ശൂന്യം state_തള്ളുക(ProcState state) { m_states.തള്ളുക(state); }
    ശൂന്യം state_എടുക്കുക() {
        m_states.എടുക്കുക();
        എങ്കിൽ (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.തള്ളുക(ps_മുകൾ);  // LCOV_EXCL_LINE
        }
    }
    ശൂന്യം stateChange(ProcState state) {
        state_എടുക്കുക();
        state_തള്ളുക(state);
    }

പൊതു:
    // METHODS, called from upper level shell
    ശൂന്യം openFile(File_വരി* fl, VInFilter* filterp, സ്ഥിരം string& filename) override;
    തർക്കം isEof() സ്ഥിരം override { തിരികെ m_lexp->curStreamp()->m_eof; }
    ശൂന്യം forceEof() { m_lexp->curStreamp()->m_eof = സത്യം; }
    string getline() override;
    ശൂന്യം തിരുകുക_Unreadback(സ്ഥിരം string& text) override { m_വരി_Cmt += text; }
    ശൂന്യം തിരുകുക_Unreadback_At_Bol(സ്ഥിരം string& text);
    ശൂന്യം add_വരി_Comment(പൂർണ്ണസംഖ്യ enterExit);
    ശൂന്യം dumpDefines(std::ostream& os) override;
    ശൂന്യം candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    ശൂന്യം comment(സ്ഥിരം string& text) override;  // Comment detected (if keepComments==2)
    ശൂന്യം include(സ്ഥിരം string& filename) override;  // Request a include file be processed
    ശൂന്യം undef(സ്ഥിരം string& name) override;
    മായാ ശൂന്യം undefineall();
    ശൂന്യം define(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& മൂല്യം, സ്ഥിരം string& params,
                തർക്കം cmdline) override;
    string removeDefines(സ്ഥിരം string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.തള്ളുക(ps_മുകൾ); }
    // cppcheck-suppress duplInheritedMember
    ശൂന്യം configure(File_വരി* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = ഇത്;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = പുതിയ File_വരി{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = പുതിയ V3PreLex{ഇത്, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        എങ്കിൽ (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_വരി* fl) {
    V3PreProcImp* സ്ഥിരം preprocp = പുതിയ V3PreProcImp;
    preprocp->configure(fl);
    തിരികെ preprocp;
}

ശൂന്യം V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_വ്യഞ്ജകം::selfTest(); }

//*************************************************************************
// Defines

ശൂന്യം V3PreProcImp::undef(സ്ഥിരം string& name) {
    എങ്കിൽ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        തിരുകുക_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
ശൂന്യം V3PreProcImp::undefineall() {
    എങ്കിൽ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        തിരുകുക_Unreadback("`undefineall\n");
    വേണ്ടി (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        എങ്കിൽ (!it->second.cmdline()) m_defines.erase(it);
    }
}
തർക്കം V3PreProcImp::defExists(സ്ഥിരം string& name) {
    സ്ഥിരം auto iter = m_defines.find(name);
    തിരികെ (iter != m_defines.end());
}
തർക്കം V3PreProcImp::defCmdline(സ്ഥിരം string& name) {
    സ്ഥിരം auto iter = m_defines.find(name);
    എങ്കിൽ (iter == m_defines.end()) { തിരികെ അസത്യം; }
    തിരികെ iter->second.cmdline();
}
string V3PreProcImp::def_മൂല്യം(സ്ഥിരം string& name) {
    സ്ഥിരം auto iter = m_defines.find(name);
    എങ്കിൽ (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        തിരികെ "";
    }
    തിരികെ iter->second.മൂല്യം();
}
string V3PreProcImp::defParams(സ്ഥിരം string& name) {
    സ്ഥിരം auto iter = m_defines.find(name);
    എങ്കിൽ (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        തിരികെ "";
    }
    തിരികെ iter->second.params();
}
File_വരി* V3PreProcImp::defFileline(സ്ഥിരം string& name) {
    സ്ഥിരം auto iter = m_defines.find(name);
    എങ്കിൽ (iter == m_defines.end()) തിരികെ nullptr;
    തിരികെ iter->second.fileline();
}
ശൂന്യം V3PreProcImp::define(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string& മൂല്യം,
                          സ്ഥിരം string& params, തർക്കം cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << മൂല്യം << "' params '" << params << "'");
    എങ്കിൽ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(മൂല്യം.size());
        വേണ്ടി (string::size_type pos = 0; pos < മൂല്യം.size(); ++pos) {
            എങ്കിൽ (മൂല്യം[pos] == '\n' && (pos == 0 || മൂല്യം[pos - 1] != '\\')) out += '\\';
            out += മൂല്യം[pos];
        }
        തിരുകുക_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    എങ്കിൽ (!V3_Language_വാക്കുകൾ::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (defExists(name)) {
            എങ്കിൽ (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << മൂല്യം
                               << "' to existing command line define value: '" << def_മൂല്യം(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_മൂല്യം(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                തിരികെ;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!(def_മൂല്യം(name) == മൂല്യം
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << മൂല്യം
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_മൂല്യം(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, മൂല്യം, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(സ്ഥിരം string& text) {
    string val;
    string rtnsym = text;
    വേണ്ടി (പൂർണ്ണസംഖ്യ loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        എങ്കിൽ (xsym[0] == '`') xsym.erase(0, 1);
        എങ്കിൽ (defExists(xsym)) {
            val = def_മൂല്യം(xsym);
            എങ്കിൽ (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } അല്ലെങ്കിൽ {
                നിർത്തുക;
            }
        } അല്ലെങ്കിൽ {
            നിർത്തുക;
        }
    }
    തിരികെ rtnsym;  // NA
}

//**********************************************************************
// Comments

ശൂന്യം V3PreProcImp::include(സ്ഥിരം string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(സ്ഥിരം string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    ആകുമ്പോൾ ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    ആകുമ്പോൾ ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    ആകുമ്പോൾ ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    ആകുമ്പോൾ ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    ആകുമ്പോൾ ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    ആകുമ്പോൾ ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    ആകുമ്പോൾ (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    തിരികെ cmd;
}

തർക്കം V3PreProcImp::commentTokenMatch(string& cmdr, സ്ഥിരം അക്ഷരം* strg) {
    പൂർണ്ണസംഖ്യ len = std::strlen(strg);
    എങ്കിൽ (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        എങ്കിൽ (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        തിരികെ സത്യം;
    } അല്ലെങ്കിൽ {
        തിരികെ അസത്യം;
    }
}

ശൂന്യം V3PreProcImp::comment(സ്ഥിരം string& text) {
    // Comment detected.  Only keep relevant data.
    തർക്കം printed = അസത്യം;
    എങ്കിൽ (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        തിരുകുക_Unreadback(text);
        printed = സത്യം;
    }

    സ്ഥിരം അക്ഷരം* cp = text.c_str();
    എങ്കിൽ (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } അല്ലെങ്കിൽ {
        തിരികെ;
    }

    ആകുമ്പോൾ (std::isspace(*cp)) ++cp;

    തർക്കം synth = അസത്യം;
    തർക്കം vlcomment = അസത്യം;
    എങ്കിൽ ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        എങ്കിൽ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            തിരികെ;
        }
        vlcomment = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        എങ്കിൽ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            തിരികെ;
        }
        synth = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = സത്യം;
    } അല്ലെങ്കിൽ {
        തിരികെ;
    }

    ആകുമ്പോൾ (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    എങ്കിൽ (synth) {
        എങ്കിൽ (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            എങ്കിൽ (commentTokenMatch(cmd /*ref*/, "full_case")) {
                എങ്കിൽ (!printed) തിരുകുക_Unreadback("/*verilator full_case*/");
            }
            എങ്കിൽ (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                എങ്കിൽ (!printed) തിരുകുക_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } അല്ലെങ്കിൽ എങ്കിൽ (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        എങ്കിൽ (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            ആകുമ്പോൾ (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            ആകുമ്പോൾ (std::isspace(arg[0])) arg = arg.substr(1);
            എങ്കിൽ (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            എങ്കിൽ (!printed) തിരുകുക_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } അല്ലെങ്കിൽ എങ്കിൽ (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            ആകുമ്പോൾ (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            എങ്കിൽ (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                തിരികെ;
            }
            എങ്കിൽ (!printed) തിരുകുക_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!printed) തിരുകുക_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_വരി* V3PreProc::fileline() {
    സ്ഥിരം V3PreProcImp* idatap = static_cast<V3PreProcImp*>(ഇത്);
    തിരികെ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

സ്ഥിരം അക്ഷരം* V3PreProcImp::tokenName(പൂർണ്ണസംഖ്യ tok) {
    തിരഞ്ഞെടുപ്പ് (tok) {
    അവസ്ഥ VP_BACKQUOTE: തിരികെ ("BACKQUOTE");
    അവസ്ഥ VP_COMMENT: തിരികെ ("COMMENT");
    അവസ്ഥ VP_DEFARG: തിരികെ ("DEFARG");
    അവസ്ഥ VP_DEFFORM: തിരികെ ("DEFFORM");
    അവസ്ഥ VP_DEFINE: തിരികെ ("DEFINE");
    അവസ്ഥ VP_DEFREF: തിരികെ ("DEFREF");
    അവസ്ഥ VP_DEFREF_JOIN: തിരികെ ("DEFREF_JOIN");
    അവസ്ഥ VP_DEFVALUE: തിരികെ ("DEFVALUE");
    അവസ്ഥ VP_ELSE: തിരികെ ("ELSE");
    അവസ്ഥ VP_ELSIF: തിരികെ ("ELSIF");
    അവസ്ഥ VP_ENDIF: തിരികെ ("ENDIF");
    അവസ്ഥ VP_EOF: തിരികെ ("EOF");
    അവസ്ഥ VP_EOF_ERROR: തിരികെ ("EOF_ERROR");
    അവസ്ഥ VP_ERROR: തിരികെ ("ERROR");
    അവസ്ഥ VP_IFDEF: തിരികെ ("IFDEF");
    അവസ്ഥ VP_IFNDEF: തിരികെ ("IFNDEF");
    അവസ്ഥ VP_JOIN: തിരികെ ("JOIN");
    അവസ്ഥ VP_INCLUDE: തിരികെ ("INCLUDE");
    അവസ്ഥ VP_വരി: തിരികെ ("LINE");
    അവസ്ഥ VP_STRIFY: തിരികെ ("STRIFY");
    അവസ്ഥ VP_STRING: തിരികെ ("STRING");
    അവസ്ഥ VP_ചിഹ്നം: തിരികെ ("SYMBOL");
    അവസ്ഥ VP_ചിഹ്നം_JOIN: തിരികെ ("SYMBOL_JOIN");
    അവസ്ഥ VP_TEXT: തിരികെ ("TEXT");
    അവസ്ഥ VP_UNDEF: തിരികെ ("UNDEF");
    അവസ്ഥ VP_UNDEFINEALL: തിരികെ ("UNDEFINEALL");
    അവസ്ഥ VP_WHITE: തിരികെ ("WHITE");
    സ്ഥിരസ്ഥിതി: തിരികെ ("?");
    }
}

ശൂന്യം V3PreProcImp::unputString(സ്ഥിരം string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    എങ്കിൽ (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

ശൂന്യം V3PreProcImp::unputDefrefString(സ്ഥിരം string& strg) {
    സ്ഥിരം പൂർണ്ണസംഖ്യ multiline = std::എണ്ണം(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(സ്ഥിരം string& strg, തർക്കം trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    ആകുമ്പോൾ (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    എങ്കിൽ (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    എങ്കിൽ (trailing) {
        string::size_type trailspace = 0;
        ആകുമ്പോൾ (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        എങ്കിൽ (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        എങ്കിൽ (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    തിരികെ out;
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
    വേണ്ടി (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    സ്ഥിരം string മൂല്യം = def_മൂല്യം(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(മൂല്യം) << "'");

    std::map<സ്ഥിരം string, string> arg_മൂല്യം_By_Name;
    {  // Parse argument list into map
        unsigned അക്കം_Args = 0;
        string argName;
        പൂർണ്ണസംഖ്യ paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        തർക്കം quote = അസത്യം;
        തർക്കം haveDefault = അസത്യം;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        സ്ഥിരം string params = refp->params();  // Must keep str in scope to get pointer
        സ്ഥിരം അക്ഷരം* cp = params.c_str();
        എങ്കിൽ (*cp == '(') cp++;
        വേണ്ടി (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            എങ്കിൽ (!quote && paren == 1) {
                എങ്കിൽ (*cp == ')' || *cp == ',') {
                    string മൂല്യം_Def;
                    എങ്കിൽ (haveDefault) {
                        മൂല്യം_Def = token;
                    } അല്ലെങ്കിൽ {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, സത്യം);
                    UINFO(4, "    Got Arg=" << അക്കം_Args << "  argName='" << argName
                                            << "'  default='" << മൂല്യം_Def << "'");
                    // Parse it
                    എങ്കിൽ (argName != "") {
                        എങ്കിൽ (refp->args().size() > അക്കം_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            സ്ഥിരം string arg = trimWhitespace(refp->args()[അക്കം_Args], സത്യം);
                            എങ്കിൽ (arg != "") മൂല്യം_Def = arg;
                        } അല്ലെങ്കിൽ എങ്കിൽ (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            തിരികെ " `" + refp->name() + " ";
                        }
                        അക്കം_Args++;
                    }
                    arg_മൂല്യം_By_Name[argName] = മൂല്യം_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = അസത്യം;
                    തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == '=') {
                    haveDefault = സത്യം;
                    argName = token;
                    token = "";
                    തുടരുക;
                }
            }
            എങ്കിൽ (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                തുടരുക;
            }
            എങ്കിൽ (!quote) {
                എങ്കിൽ (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            എങ്കിൽ (*cp == '"') quote = !quote;
            എങ്കിൽ (*cp) token += *cp;
        }
        എങ്കിൽ (refp->args().size() > അക്കം_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && അക്കം_Args == 0
                 && trimWhitespace(refp->args()[0], അസത്യം) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            തിരികെ " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        തർക്കം quote = അസത്യം;
        തർക്കം triquote = അസത്യം;
        തർക്കം backslashesc = അസത്യം;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = മൂല്യം.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            എങ്കിൽ (!quote && !triquote && *cp == '\\') {
                backslashesc = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (std::isspace(*cp)) {
                backslashesc = അസത്യം;
            }
            എങ്കിൽ (!quote && !triquote) {
                എങ്കിൽ (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    തുടരുക;
                }
                എങ്കിൽ (argName != "") {
                    // Found a possible variable substitution
                    സ്ഥിരം auto iter = arg_മൂല്യം_By_Name.find(argName);
                    എങ്കിൽ (iter != arg_മൂല്യം_By_Name.end()) {
                        // Substitute
                        സ്ഥിരം string subst = iter->second;
                        എങ്കിൽ (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            എങ്കിൽ (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } അല്ലെങ്കിൽ {
                            out += subst;
                        }
                    } അല്ലെങ്കിൽ {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                എങ്കിൽ (cp[0] == '`' && cp[1] == '`') {
                    എങ്കിൽ (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } അല്ലെങ്കിൽ {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    തുടരുക;
                }
            }
            എങ്കിൽ (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                തുടരുക;
            } അല്ലെങ്കിൽ എങ്കിൽ (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                തുടരുക;
            }
            എങ്കിൽ (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                തുടരുക;
            }
            എങ്കിൽ (*cp == '"') quote = !quote;
            എങ്കിൽ (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    തിരികെ out;
}

//**********************************************************************
// Parser routines

ശൂന്യം V3PreProcImp::openFile(File_വരി*, VInFilter* filterp, സ്ഥിരം string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    എങ്കിൽ (m_incError) തിരികെ;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    സ്ഥിരം തർക്കം ok = filterp->readWholefile(filename, wholefile /*ref*/);
    എങ്കിൽ (!ok) {
        fileline()->v3error("File not found: " + filename);
        തിരികെ;
    }

    എങ്കിൽ (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        എങ്കിൽ (m_lexp->m_streamp_അട്ടി.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = സത്യം;
            തിരികെ;
        }
        // There's already a file active.  Push it to work on the new one.
        add_വരി_Comment(0);
    }

    // Save file contents for future error reporting
    File_വരി* സ്ഥിരം flsp = പുതിയ File_വരി{filename};
    flsp->lineno(1);
    flsp->newContent();
    വേണ്ടി (സ്ഥിരം string& i : wholefile) flsp->contentp()->തള്ളുക_Text(i);

    // Save contents for V3Control --contents
    എങ്കിൽ (filename != V3Options::getStdPackagePath()) {
        തർക്കം containsVlt = അസത്യം;
        വേണ്ടി (സ്ഥിരം string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            എങ്കിൽ (i.find("`verilator_config") != string::npos) {
                containsVlt = സത്യം;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (!containsVlt) {
            വേണ്ടി (സ്ഥിരം string& i : wholefile) V3Control::contents_തള്ളുക_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_വരി_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    പൂർണ്ണസംഖ്യ eof_newline = 0;  // Number of characters following last newline
    പൂർണ്ണസംഖ്യ eof_lineno = 1;
    വേണ്ടി (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        തർക്കം strip = അസത്യം;
        സ്ഥിരം അക്ഷരം* സ്ഥിരം sp = it->data();
        സ്ഥിരം അക്ഷരം* സ്ഥിരം ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = sp; cp < ep; cp++) {
            എങ്കിൽ (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } അല്ലെങ്കിൽ {
                ++eof_newline;
            }
        }
        എങ്കിൽ (strip) {
            string out;
            out.reserve(it->length());
            വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = sp; cp < ep; cp++) {
                എങ്കിൽ (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    എങ്കിൽ (eof_newline) {
        File_വരി* സ്ഥിരം fl = പുതിയ File_വരി{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

ശൂന്യം V3PreProcImp::തിരുകുക_Unreadback_At_Bol(സ്ഥിരം string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    എങ്കിൽ (m_വരി_Cmt == "") {
        m_വരി_Cmt_Nl = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (m_വരി_Cmt[m_വരി_Cmt.length() - 1] != '\n') {
        തിരുകുക_Unreadback("\n");
    }
    തിരുകുക_Unreadback(text);
}

ശൂന്യം V3PreProcImp::add_വരി_Comment(പൂർണ്ണസംഖ്യ enterExit) {
    എങ്കിൽ (വരി_Directives()) {
        തിരുകുക_Unreadback_At_Bol(m_lexp->curFilelinep()->വരി_Directive_Strg(enterExit));
    }
}

ശൂന്യം V3PreProcImp::dumpDefines(std::ostream& os) {
    വേണ്ടി (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        എങ്കിൽ (!it->second.params().empty()) os << it->second.params();
        എങ്കിൽ (!it->second.മൂല്യം().empty()) os << " " << it->second.മൂല്യം();
        os << '\n';
    }
}

ശൂന്യം V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    വേണ്ടി (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->തള്ളുക_Candidate("`"s + it->first);
    }
}

പൂർണ്ണസംഖ്യ V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    ആകുമ്പോൾ (സത്യം) {
    next_tok:
        എങ്കിൽ (m_വരി_Add) {
            m_വരി_Add--;
            m_rawAtBol = സത്യം;
            yyourtext("\n", 1);
            എങ്കിൽ (debug() >= 5) debugToken(VP_WHITE, "LNA");
            തിരികെ VP_WHITE;
        }
        എങ്കിൽ (m_വരി_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            സ്ഥിതം string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_വരി_Cmt;
            എങ്കിൽ (m_വരി_Cmt_Nl) {
                എങ്കിൽ (!m_rawAtBol) s_rtncmt.തിരുകുക(0, "\n");
                m_വരി_Cmt_Nl = അസത്യം;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_വരി_Cmt = "";
            എങ്കിൽ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            എങ്കിൽ (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_മൂല്യം(yyourtext(), yyourleng());
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (debug() >= 5) debugToken(VP_TEXT, "LCM");
                തിരികെ VP_TEXT;
            }
        }
        എങ്കിൽ (isEof()) തിരികെ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        പൂർണ്ണസംഖ്യ tok = m_lexp->lex();
        എങ്കിൽ (debug() >= 5) debugToken(tok, "RAW");

        എങ്കിൽ (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_വരി = 0;
        } അല്ലെങ്കിൽ എങ്കിൽ (++m_tokens_On_വരി > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        എങ്കിൽ (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            എങ്കിൽ (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            പോകുക next_tok;
        }

        എങ്കിൽ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        തിരികെ tok;
    }
}

ശൂന്യം V3PreProcImp::debugToken(പൂർണ്ണസംഖ്യ tok, സ്ഥിരം അക്ഷരം* cmtp) {
    സ്ഥിരം സ്ഥിതം പൂർണ്ണസംഖ്യ s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    എങ്കിൽ (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        ആകുമ്പോൾ ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        ആകുമ്പോൾ ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        സ്ഥിരം string flcol = m_lexp->m_tokFilelinep->ascii_വരി_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<പൂർണ്ണസംഖ്യ>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        എങ്കിൽ (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

പൂർണ്ണസംഖ്യ V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    ആകുമ്പോൾ (സത്യം) {
    next_tok:
        എങ്കിൽ (isEof()) തിരികെ VP_EOF;
        പൂർണ്ണസംഖ്യ tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        എങ്കിൽ (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) തിരികെ tok;
        എങ്കിൽ (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        എങ്കിൽ (tok == VP_COMMENT) {
            എങ്കിൽ (!m_off) {
                എങ്കിൽ (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    എങ്കിൽ (m_വരി_Cmt.empty()) തിരുകുക_Unreadback(" ");
                } അല്ലെങ്കിൽ എങ്കിൽ (m_lexp->m_keepComments) {
                    തിരികെ tok;
                } അല്ലെങ്കിൽ {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    തിരുകുക_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            വേണ്ടി (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                എങ്കിൽ (yyourtext()[len] == '\n') m_വരി_Add++;
            }
            പോകുക next_tok;
        }
        എങ്കിൽ (tok == VP_വരി) {
            add_വരി_Comment(m_lexp->m_enterExit);
            പോകുക next_tok;
        }

        എങ്കിൽ (tok == VP_DEFREF_JOIN) {
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
            സ്ഥിരം std::string name(yyourtext() + 1, yyourleng() - 1);
            എങ്കിൽ (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = സത്യം;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } അല്ലെങ്കിൽ {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        എങ്കിൽ (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            എങ്കിൽ (tok == VP_JOIN) പോകുക next_tok;
            എങ്കിൽ (tok == VP_ചിഹ്നം_JOIN) tok = VP_ചിഹ്നം;
        }
        എങ്കിൽ (tok == VP_ചിഹ്നം_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            സ്ഥിരം std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_അട്ടി.തള്ളുക(out);
            state_തള്ളുക(ps_JOIN);
            പോകുക next_tok;
        }

        // Deal with some special parser states
        തിരഞ്ഞെടുപ്പ് (state()) {
        അവസ്ഥ ps_മുകൾ: {
            നിർത്തുക;
        }
        അവസ്ഥ ps_DEFNAME_UNDEF:  // FALLTHRU
        അവസ്ഥ ps_DEFNAME_DEFINE:  // FALLTHRU
        അവസ്ഥ ps_DEFNAME_IFDEF:  // FALLTHRU
        അവസ്ഥ ps_DEFNAME_IFNDEF:  // FALLTHRU
        അവസ്ഥ ps_DEFNAME_ELSIF: {
            എങ്കിൽ (tok == VP_ചിഹ്നം) {
                m_lastSym.assign(yyourtext(), yyourleng());
                എങ്കിൽ (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    തർക്കം enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    എങ്കിൽ (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_അട്ടി.emplace(VPreIfEntry{enable, അസത്യം});
                    എങ്കിൽ (!enable) parsingOff();
                    state_എടുക്കുക();
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ എങ്കിൽ (state() == ps_DEFNAME_ELSIF) {
                    എങ്കിൽ (m_ifdef_അട്ടി.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } അല്ലെങ്കിൽ {
                        // Handle `else portion
                        സ്ഥിരം VPreIfEntry lastIf = m_ifdef_അട്ടി.മുകൾ();
                        m_ifdef_അട്ടി.എടുക്കുക();
                        എങ്കിൽ (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        സ്ഥിരം തർക്കം enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_അട്ടി.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        എങ്കിൽ (!enable) parsingOff();
                    }
                    state_എടുക്കുക();
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ എങ്കിൽ (state() == ps_DEFNAME_UNDEF) {
                    എങ്കിൽ (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_എടുക്കുക();
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ എങ്കിൽ (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->തള്ളുക_State_Def_Form();
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                എങ്കിൽ (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->തള്ളുക_State_വ്യഞ്ജകം();
                    m_വ്യഞ്ജകം_Parser.reset(fileline());
                    m_വ്യഞ്ജകം_Paren_Level = 1;
                    തിരഞ്ഞെടുപ്പ് (state()) {
                    അവസ്ഥ ps_DEFNAME_IFDEF: stateChange(ps_വ്യഞ്ജകം_IFDEF); നിർത്തുക;
                    അവസ്ഥ ps_DEFNAME_IFNDEF: stateChange(ps_വ്യഞ്ജകം_IFNDEF); നിർത്തുക;
                    അവസ്ഥ ps_DEFNAME_ELSIF: stateChange(ps_വ്യഞ്ജകം_ELSIF); നിർത്തുക;
                    സ്ഥിരസ്ഥിതി: v3fatalSrc("bad case");
                    }
                    പോകുക next_tok;
                }
                എങ്കിൽ (!m_off) {
                    തിരികെ tok;
                } അല്ലെങ്കിൽ {
                    പോകുക next_tok;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                നിർത്തുക;
            } അല്ലെങ്കിൽ {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                പോകുക next_tok;
            }
        }
        അവസ്ഥ ps_വ്യഞ്ജകം_IFDEF:  // FALLTHRU
        അവസ്ഥ ps_വ്യഞ്ജകം_IFNDEF:  // FALLTHRU
        അവസ്ഥ ps_വ്യഞ്ജകം_ELSIF: {
            // `ifdef ( *here*
            File_വരി* സ്ഥിരം flp = m_lexp->m_tokFilelinep;
            എങ്കിൽ (tok == VP_ചിഹ്നം) {
                m_lastSym.assign(yyourtext(), yyourleng());
                സ്ഥിരം തർക്കം exists = defExists(m_lastSym);
                എങ്കിൽ (exists) {
                    സ്ഥിരം std::string മൂല്യം = def_മൂല്യം(m_lastSym);
                    എങ്കിൽ (VString::removeWhitespace(മൂല്യം) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << മൂല്യം
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, exists});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_WHITE) {
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::BRA});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_വ്യഞ്ജകം_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                എങ്കിൽ (--m_വ്യഞ്ജകം_Paren_Level > 0) {
                    m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::KET});
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ {
                    // Done with parsing expression
                    തർക്കം enable = m_വ്യഞ്ജകം_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    എങ്കിൽ (state() == ps_വ്യഞ്ജകം_IFDEF || state() == ps_വ്യഞ്ജകം_IFNDEF) {
                        എങ്കിൽ (state() == ps_വ്യഞ്ജകം_IFNDEF) enable = !enable;
                        m_ifdef_അട്ടി.emplace(VPreIfEntry{enable, അസത്യം});
                        എങ്കിൽ (!enable) parsingOff();
                        state_എടുക്കുക();
                        പോകുക next_tok;
                    } അല്ലെങ്കിൽ എങ്കിൽ (state() == ps_വ്യഞ്ജകം_ELSIF) {
                        എങ്കിൽ (m_ifdef_അട്ടി.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } അല്ലെങ്കിൽ {
                            // Handle `else portion
                            സ്ഥിരം VPreIfEntry lastIf = m_ifdef_അട്ടി.മുകൾ();
                            m_ifdef_അട്ടി.എടുക്കുക();
                            എങ്കിൽ (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_അട്ടി.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            എങ്കിൽ (!enable) parsingOff();
                        }
                        state_എടുക്കുക();
                    }
                    പോകുക next_tok;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::LNOT});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::LAND});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::LOR});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::IMP});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_വ്യഞ്ജകം_Parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::EQV});
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    തിരികെ tok;
                } അല്ലെങ്കിൽ {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                പോകുക next_tok;
            }
        }
        അവസ്ഥ ps_DEFFORM: {
            എങ്കിൽ (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_മൂല്യം;
                എങ്കിൽ (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->തള്ളുക_State_Def_മൂല്യം();
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT) {
                // IE, something like comment in formals
                എങ്കിൽ (!m_off) തിരികെ tok;
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                പോകുക next_tok;
            }
        }
        അവസ്ഥ ps_DEFVALUE: {
            സ്ഥിതം string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            എങ്കിൽ (tok == VP_DEFVALUE) {
                എങ്കിൽ (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_മൂല്യം)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                സ്ഥിരം string formals = m_formals;
                string മൂല്യം = m_lexp->m_def_മൂല്യം;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                വേണ്ടി (size_t i = 0; i < formals.length(); i++) {
                    എങ്കിൽ (formals[i] == '\n') s_newlines += "\n";
                }
                വേണ്ടി (size_t i = 0; i < മൂല്യം.length(); i++) {
                    എങ്കിൽ (മൂല്യം[i] == '\n') s_newlines += "\n";
                }
                എങ്കിൽ (!m_off) {
                    // Remove leading and trailing whitespace
                    മൂല്യം = trimWhitespace(മൂല്യം, സത്യം);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(മൂല്യം) << "'");
                    define(fileline(), m_lastSym, മൂല്യം, formals, അസത്യം);
                }
            } അല്ലെങ്കിൽ {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_എടുക്കുക();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            തിരികെ VP_WHITE;
        }
        അവസ്ഥ ps_DEFPAREN: {
            എങ്കിൽ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                സ്ഥിരം VDefineRef* സ്ഥിരം refp = &(m_defRefs.മുകൾ());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_എടുക്കുക();
                പോകുക next_tok;
            }
        }
        അവസ്ഥ ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.മുകൾ());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_മൂല്യം);
            m_lexp->m_def_മൂല്യം = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            എങ്കിൽ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().തള്ളുക_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->തള്ളുക_State_Def_Arg(1);
                refp->nextarg("");
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().തള്ളുക_back(refp->nextarg());
                string out;
                എങ്കിൽ (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.എടുക്കുക(), refp);
                state_എടുക്കുക();
                എങ്കിൽ (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    എങ്കിൽ (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_അട്ടി.empty(), "`` join stack empty, but in a ``");
                        സ്ഥിരം string lhs = m_join_അട്ടി.മുകൾ();
                        m_join_അട്ടി.എടുക്കുക();
                        out.തിരുകുക(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_എടുക്കുക();
                    }
                    എങ്കിൽ (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    അല്ലെങ്കിൽ എങ്കിൽ (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.മുകൾ().parenLevel();
                } അല്ലെങ്കിൽ {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.മുകൾ());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_മൂല്യം + out);
                    m_lexp->m_def_മൂല്യം = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                നിർത്തുക;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_ചിഹ്നം || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_തള്ളുക(ps_STRIFY);
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_എടുക്കുക();
                പോകുക next_tok;
            }
        }
        അവസ്ഥ ps_INCNAME: {
            എങ്കിൽ (tok == VP_STRING) {
                state_എടുക്കുക();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->തള്ളുക_State_Inc_Filename();
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                നിർത്തുക;
            } അല്ലെങ്കിൽ {
                state_എടുക്കുക();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                പോകുക next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        അവസ്ഥ ps_ERRORNAME: {
            എങ്കിൽ (tok == VP_STRING) {
                എങ്കിൽ (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_എടുക്കുക();
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_എടുക്കുക();
                പോകുക next_tok;
            }
        }
        അവസ്ഥ ps_JOIN: {
            എങ്കിൽ (tok == VP_ചിഹ്നം || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_അട്ടി.empty(), "`` join stack empty, but in a ``");
                സ്ഥിരം string lhs = m_join_അട്ടി.മുകൾ();
                m_join_അട്ടി.എടുക്കുക();
                UINFO(5, "`` LHS:" << lhs);
                സ്ഥിരം string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                സ്ഥിരം string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_എടുക്കുക();
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                സ്ഥിരം string lhs = m_join_അട്ടി.മുകൾ();
                m_join_അട്ടി.എടുക്കുക();
                unputString(lhs);
                state_എടുക്കുക();
                പോകുക next_tok;
            } അല്ലെങ്കിൽ {
                // `define, etc, fall through and expand.  Pop back here.
                നിർത്തുക;
            }
        }
        അവസ്ഥ ps_STRIFY: {
            എങ്കിൽ (tok == VP_STRIFY) {
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
                state_എടുക്കുക();
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                നിർത്തുക;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                പോകുക next_tok;
            } അല്ലെങ്കിൽ എങ്കിൽ (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                നിർത്തുക;
            } അല്ലെങ്കിൽ {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                പോകുക next_tok;
            }
        }
        സ്ഥിരസ്ഥിതി: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        തിരഞ്ഞെടുപ്പ് (tok) {
        അവസ്ഥ VP_INCLUDE:
            എങ്കിൽ (!m_off) {
                state_തള്ളുക(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            പോകുക next_tok;
        അവസ്ഥ VP_UNDEF: state_തള്ളുക(ps_DEFNAME_UNDEF); പോകുക next_tok;
        അവസ്ഥ VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_തള്ളുക(ps_DEFNAME_DEFINE);
            പോകുക next_tok;
        അവസ്ഥ VP_IFDEF: state_തള്ളുക(ps_DEFNAME_IFDEF); പോകുക next_tok;
        അവസ്ഥ VP_IFNDEF: state_തള്ളുക(ps_DEFNAME_IFNDEF); പോകുക next_tok;
        അവസ്ഥ VP_ELSIF: state_തള്ളുക(ps_DEFNAME_ELSIF); പോകുക next_tok;
        അവസ്ഥ VP_ELSE:
            എങ്കിൽ (m_ifdef_അട്ടി.empty()) {
                fileline()->v3error("`else with no matching `if");
            } അല്ലെങ്കിൽ {
                സ്ഥിരം VPreIfEntry lastIf = m_ifdef_അട്ടി.മുകൾ();
                m_ifdef_അട്ടി.എടുക്കുക();
                സ്ഥിരം തർക്കം enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_അട്ടി.emplace(VPreIfEntry{enable, lastIf.everOn()});
                എങ്കിൽ (!lastIf.on()) parsingOn();
                എങ്കിൽ (!enable) parsingOff();
            }
            പോകുക next_tok;
        അവസ്ഥ VP_ENDIF:
            UINFO(4, "Endif ");
            എങ്കിൽ (m_ifdef_അട്ടി.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } അല്ലെങ്കിൽ {
                സ്ഥിരം VPreIfEntry lastIf = m_ifdef_അട്ടി.മുകൾ();
                m_ifdef_അട്ടി.എടുക്കുക();
                എങ്കിൽ (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            പോകുക next_tok;

        അവസ്ഥ VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            സ്ഥിരം string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            എങ്കിൽ (m_defPutJoin) {
                m_defPutJoin = അസത്യം;
                unputString("``");
            }
            എങ്കിൽ (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                പോകുക next_tok;
            }
            // Substitute
            എങ്കിൽ (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                എങ്കിൽ (m_off) {
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    പോകുക next_tok;
                }
            } അല്ലെങ്കിൽ {
                സ്ഥിരം string params = defParams(name);
                എങ്കിൽ (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    എങ്കിൽ (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    എങ്കിൽ (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        എങ്കിൽ (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_അട്ടി.empty(), "`` join stack empty, but in a ``");
                            സ്ഥിരം string lhs = m_join_അട്ടി.മുകൾ();
                            m_join_അട്ടി.എടുക്കുക();
                            out.തിരുകുക(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_എടുക്കുക();
                        }
                        എങ്കിൽ (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        അല്ലെങ്കിൽ എങ്കിൽ (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } അല്ലെങ്കിൽ {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* സ്ഥിരം refp = &(m_defRefs.മുകൾ());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_മൂല്യം + out);
                        m_lexp->m_def_മൂല്യം = "";
                    }
                    പോകുക next_tok;
                } അല്ലെങ്കിൽ {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    എങ്കിൽ (!m_defRefs.empty()) m_defRefs.മുകൾ().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_തള്ളുക(ps_DEFPAREN);
                    m_lexp->തള്ളുക_State_Def_Arg(0);
                    പോകുക next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            പോകുക next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        അവസ്ഥ VP_ERROR: {
            state_തള്ളുക(ps_ERRORNAME);
            പോകുക next_tok;
        }
        അവസ്ഥ VP_EOF:
            എങ്കിൽ (!m_ifdef_അട്ടി.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            തിരികെ tok;
        അവസ്ഥ VP_UNDEFINEALL:
            എങ്കിൽ (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            പോകുക next_tok;
        അവസ്ഥ VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_തള്ളുക(ps_STRIFY);
            പോകുക next_tok;
        അവസ്ഥ VP_ചിഹ്നം:
        അവസ്ഥ VP_STRING:
        അവസ്ഥ VP_TEXT: {
            m_defDepth = 0;
            എങ്കിൽ (!m_off) {
                തിരികെ tok;
            } അല്ലെങ്കിൽ {
                പോകുക next_tok;
            }
        }
        അവസ്ഥ VP_WHITE:  // Handled at top of loop
        അവസ്ഥ VP_COMMENT:  // Handled at top of loop
        അവസ്ഥ VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        അവസ്ഥ VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            നിർത്തുക;  // LCOV_EXCL_LINE
        }
        തിരികെ tok;
    }
}

പൂർണ്ണസംഖ്യ V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    എങ്കിൽ (!m_finAhead) {
        m_finAhead = സത്യം;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    സ്ഥിരം പൂർണ്ണസംഖ്യ tok = m_finToken;
    buf = m_finBuf;
    എങ്കിൽ (അസത്യം && debug() >= 5) {
        സ്ഥിരം string bufcln = V3PreLex::cleanDbgStrg(buf);
        സ്ഥിരം string flcol = m_lexp->m_tokFilelinep->ascii_വരി_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    സ്ഥിരം അക്ഷരം* bufp = buf.c_str();
    ആകുമ്പോൾ (*bufp == '\n') bufp++;
    എങ്കിൽ ((tok == VP_TEXT || tok == VP_വരി) && VString::startsWith(bufp, "`line ")) {
        പൂർണ്ണസംഖ്യ enter;
        m_finFilelinep->വരി_Directive(bufp, enter /*ref*/);
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->വരി_Directives()) {
            എങ്കിൽ (സ്ഥിരം പൂർണ്ണസംഖ്യ outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                എങ്കിൽ (debug() >= 5) {
                    സ്ഥിരം string flcol = m_lexp->m_tokFilelinep->ascii_വരി_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                എങ്കിൽ (outBehind > 0
                    && (outBehind <= static_cast<പൂർണ്ണസംഖ്യ>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    എങ്കിൽ (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        തിരികെ VP_TEXT;
                    }
                } അല്ലെങ്കിൽ {
                    // Need to backup, use `line
                    buf = m_finFilelinep->വരി_Directive_Strg(0);
                    തിരികെ VP_വരി;
                }
            }
        }
        // Track newlines in prep for next token
        വേണ്ടി (സ്ഥിരം അക്ഷരം& c : buf) {
            എങ്കിൽ (c == '\n') {
                m_finAtBol = സത്യം;
                m_finFilelinep->linenoInc();
            } അല്ലെങ്കിൽ {
                m_finAtBol = അസത്യം;
            }
        }
    }
    m_finAhead = അസത്യം;  // Consumed the token
    തിരികെ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    എങ്കിൽ (isEof()) തിരികെ "";
    സ്ഥിരം അക്ഷരം* rtnp;
    തർക്കം gotEof = അസത്യം;
    ആകുമ്പോൾ (nullptr == (rtnp = std::strchr(m_വരി_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        സ്ഥിരം പൂർണ്ണസംഖ്യ tok = getFinalToken(m_tokenBuf /*ref*/);
        എങ്കിൽ (debug() >= 5) {
            സ്ഥിരം string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            സ്ഥിരം string flcol = m_lexp->m_tokFilelinep->ascii_വരി_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        എങ്കിൽ (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            എങ്കിൽ (m_വരി_Chars != "" && m_വരി_Chars[m_വരി_Chars.length() - 1] != '\n') {
                m_വരി_Chars.append("\n");
            }
            gotEof = സത്യം;
        } അല്ലെങ്കിൽ {
            m_വരി_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    സ്ഥിരം പൂർണ്ണസംഖ്യ len = rtnp - m_വരി_Chars.c_str() + 1;
    string the_വരി(m_വരി_Chars, 0, len);
    m_വരി_Chars = m_വരി_Chars.erase(0, len);  // Remove returned characters
    എങ്കിൽ (debug() >= 4) {
        സ്ഥിരം string lncln = V3PreLex::cleanDbgStrg(the_വരി);
        സ്ഥിരം string flcol = m_lexp->m_tokFilelinep->ascii_വരി_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    തിരികെ the_വരി;
}
