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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3PreProc.h"

#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3LanguageWords.h"
#अन्तर्भाव "V3PreExpr.h"
#अन्तर्भाव "V3PreLex.h"
#अन्तर्भाव "V3PreShell.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cstdlib>
#अन्तर्भाव <fstream>
#अन्तर्भाव <stack>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#अन्तर्भाव "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

वर्ग VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_पंक्ति* ध्रुव m_fileline;  // Where it was declared
    ध्रुव string m_मूल्य;  // Value of define
    ध्रुव string m_params;  // Parameters
    ध्रुव तार्किक m_cmdline;  // Set on command line, don't `undefineall
सार्वजनिक:
    VDefine(File_पंक्ति* fl, ध्रुव string& मूल्य, ध्रुव string& params, तार्किक cmdline)
        : m_fileline{fl}
        , m_मूल्य{मूल्य}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_पंक्ति* fileline() ध्रुव { प्रत्यागम m_fileline; }
    string मूल्य() ध्रुव { प्रत्यागम m_मूल्य; }
    string params() ध्रुव { प्रत्यागम m_params; }
    तार्किक cmdline() ध्रुव { प्रत्यागम m_cmdline; }
};

//*************************************************************************

वर्ग VDefineRef final {
    // One for each pending define substitution
    ध्रुव string m_name;  // Define last name being defined
    ध्रुव string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    पूर्णाङ्क m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
सार्वजनिक:
    string name() ध्रुव { प्रत्यागम m_name; }
    string params() ध्रुव { प्रत्यागम m_params; }
    string nextarg() ध्रुव { प्रत्यागम m_nextarg; }
    शून्य nextarg(ध्रुव string& मूल्य) { m_nextarg = मूल्य; }
    पूर्णाङ्क parenLevel() ध्रुव { प्रत्यागम m_parenLevel; }
    शून्य parenLevel(पूर्णाङ्क मूल्य) { m_parenLevel = मूल्य; }
    std::vector<string>& args() { प्रत्यागम m_args; }
    VDefineRef(ध्रुव string& name, ध्रुव string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = स्वभाव;
};

//*************************************************************************
/// Data for parsing on/off

वर्ग VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ध्रुव तार्किक m_on;  // Current parse for this ifdef level is "on"
    ध्रुव तार्किक m_everOn;  // Some if term in elsif tree has been on
सार्वजनिक:
    तार्किक on() ध्रुव { प्रत्यागम m_on; }
    तार्किक everOn() ध्रुव { प्रत्यागम m_everOn; }
    VPreIfEntry(तार्किक on, तार्किक everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = स्वभाव;
};

//*************************************************************************
// Data for a preprocessor instantiation.

वर्ग V3PreProcImp final : सार्वजनिक V3PreProc {
सार्वजनिक:
    // TYPES
    using DefinesMap = std::map<ध्रुव std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ध्रुव V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    पूर्णाङ्क m_lastLineno = 0;  // Last line number (stall detection)
    पूर्णाङ्क m_tokens_On_पंक्ति = 0;  // Number of tokens on line (stall detection)

    गणना ProcState : uint8_t {
        ps_शीर्ष,
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
        ps_व्यंजक_IFDEF,
        ps_व्यंजक_IFNDEF,
        ps_व्यंजक_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    स्थिर ध्रुव वर्ण* procStateName(ProcState s) {
        स्थिर ध्रुव वर्ण* ध्रुव states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        प्रत्यागम states[s];
    }

    std::ढेर<ProcState> m_states;  ///< Current state of parser
    पूर्णाङ्क m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    तार्किक m_incError = असत्य;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_पंक्ति_Cmt;  ///< Line comment(s) to be returned
    तार्किक m_पंक्ति_Cmt_Nl = असत्य;  ///< Newline needed before inserting lineCmt
    पूर्णाङ्क m_पंक्ति_Add = 0;  ///< Empty lines to return to maintain line count
    तार्किक m_rawAtBol = सत्य;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    तार्किक m_finAhead = असत्य;  ///< Have read a token ahead
    पूर्णाङ्क m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    तार्किक m_finAtBol = सत्य;  ///< Last getFinalToken left us at beginning of line
    File_पंक्ति* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ढेर<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ढेर<VPreIfEntry> m_ifdef_ढेर;  ///< Stack of true/false emitting evaluations
    अचिन्हित m_defDepth = 0;  ///< How many `defines deep
    तार्किक m_defPutJoin = असत्य;  ///< Insert `` after substitution

    // For `` join
    std::ढेर<string> m_join_ढेर;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_व्यंजक m_व्यंजक_Parser;  ///< Parser for () expression
    पूर्णाङ्क m_व्यंजक_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_पंक्ति_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    शून्य v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    स्थिर ध्रुव वर्ण* tokenName(पूर्णाङ्क tok);
    शून्य debugToken(पूर्णाङ्क tok, ध्रुव वर्ण* cmtp);
    शून्य parse_शीर्ष();
    शून्य parseUndef();

गुप्त:
    // Internal methods
    शून्य endOfOneFile();
    string defineSubst(VDefineRef* refp);

    तार्किक defExists(ध्रुव string& name);
    तार्किक defCmdline(ध्रुव string& name);
    string def_मूल्य(ध्रुव string& name);
    string defParams(ध्रुव string& name);
    File_पंक्ति* defFileline(ध्रुव string& name);

    string commentCleanup(ध्रुव string& text);
    तार्किक commentTokenMatch(string& cmdr, ध्रुव वर्ण* strg);
    स्थिर string trimWhitespace(ध्रुव string& strg, तार्किक trailing);
    शून्य unputString(ध्रुव string& strg);
    शून्य unputDefrefString(ध्रुव string& strg);

    शून्य parsingOn() {
        m_off--;
        यदि (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    शून्य parsingOff() { m_off++; }

    पूर्णाङ्क getRawToken();
    पूर्णाङ्क getStateToken();
    पूर्णाङ्क getFinalToken(string& buf);

    ProcState state() ध्रुव { प्रत्यागम m_states.शीर्ष(); }
    तार्किक stateIsDefname() ध्रुव {
        प्रत्यागम state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    शून्य state_धकेलो(ProcState state) { m_states.धकेलो(state); }
    शून्य state_निकालो() {
        m_states.निकालो();
        यदि (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.धकेलो(ps_शीर्ष);  // LCOV_EXCL_LINE
        }
    }
    शून्य stateChange(ProcState state) {
        state_निकालो();
        state_धकेलो(state);
    }

सार्वजनिक:
    // METHODS, called from upper level shell
    शून्य openFile(File_पंक्ति* fl, VInFilter* filterp, ध्रुव string& filename) override;
    तार्किक isEof() ध्रुव override { प्रत्यागम m_lexp->curStreamp()->m_eof; }
    शून्य forceEof() { m_lexp->curStreamp()->m_eof = सत्य; }
    string getline() override;
    शून्य जोड़ो_Unreadback(ध्रुव string& text) override { m_पंक्ति_Cmt += text; }
    शून्य जोड़ो_Unreadback_At_Bol(ध्रुव string& text);
    शून्य add_पंक्ति_Comment(पूर्णाङ्क enterExit);
    शून्य dumpDefines(std::ostream& os) override;
    शून्य candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    शून्य comment(ध्रुव string& text) override;  // Comment detected (if keepComments==2)
    शून्य include(ध्रुव string& filename) override;  // Request a include file be processed
    शून्य undef(ध्रुव string& name) override;
    आभास शून्य undefineall();
    शून्य define(File_पंक्ति* fl, ध्रुव string& name, ध्रुव string& मूल्य, ध्रुव string& params,
                तार्किक cmdline) override;
    string removeDefines(ध्रुव string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.धकेलो(ps_शीर्ष); }
    // cppcheck-suppress duplInheritedMember
    शून्य configure(File_पंक्ति* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = एतत्;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = नूतन File_पंक्ति{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = नूतन V3PreLex{एतत्, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        यदि (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_पंक्ति* fl) {
    V3PreProcImp* ध्रुव preprocp = नूतन V3PreProcImp;
    preprocp->configure(fl);
    प्रत्यागम preprocp;
}

शून्य V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_व्यंजक::selfTest(); }

//*************************************************************************
// Defines

शून्य V3PreProcImp::undef(ध्रुव string& name) {
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        जोड़ो_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
शून्य V3PreProcImp::undefineall() {
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        जोड़ो_Unreadback("`undefineall\n");
    प्रति (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        यदि (!it->second.cmdline()) m_defines.erase(it);
    }
}
तार्किक V3PreProcImp::defExists(ध्रुव string& name) {
    ध्रुव स्वतः iter = m_defines.find(name);
    प्रत्यागम (iter != m_defines.end());
}
तार्किक V3PreProcImp::defCmdline(ध्रुव string& name) {
    ध्रुव स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) { प्रत्यागम असत्य; }
    प्रत्यागम iter->second.cmdline();
}
string V3PreProcImp::def_मूल्य(ध्रुव string& name) {
    ध्रुव स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        प्रत्यागम "";
    }
    प्रत्यागम iter->second.मूल्य();
}
string V3PreProcImp::defParams(ध्रुव string& name) {
    ध्रुव स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        प्रत्यागम "";
    }
    प्रत्यागम iter->second.params();
}
File_पंक्ति* V3PreProcImp::defFileline(ध्रुव string& name) {
    ध्रुव स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) प्रत्यागम nullptr;
    प्रत्यागम iter->second.fileline();
}
शून्य V3PreProcImp::define(File_पंक्ति* fl, ध्रुव string& name, ध्रुव string& मूल्य,
                          ध्रुव string& params, तार्किक cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << मूल्य << "' params '" << params << "'");
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(मूल्य.size());
        प्रति (string::size_type pos = 0; pos < मूल्य.size(); ++pos) {
            यदि (मूल्य[pos] == '\n' && (pos == 0 || मूल्य[pos - 1] != '\\')) out += '\\';
            out += मूल्य[pos];
        }
        जोड़ो_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    यदि (!V3_Language_शब्दगण::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } अन्यथा {
        यदि (defExists(name)) {
            यदि (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << मूल्य
                               << "' to existing command line define value: '" << def_मूल्य(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_मूल्य(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                प्रत्यागम;
            } अन्यथा {
                यदि (!(def_मूल्य(name) == मूल्य
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << मूल्य
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_मूल्य(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, मूल्य, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(ध्रुव string& text) {
    string val;
    string rtnsym = text;
    प्रति (पूर्णाङ्क loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        यदि (xsym[0] == '`') xsym.erase(0, 1);
        यदि (defExists(xsym)) {
            val = def_मूल्य(xsym);
            यदि (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } अन्यथा {
                भङ्ग;
            }
        } अन्यथा {
            भङ्ग;
        }
    }
    प्रत्यागम rtnsym;  // NA
}

//**********************************************************************
// Comments

शून्य V3PreProcImp::include(ध्रुव string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ध्रुव string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    यावत् ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    यावत् ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    यावत् ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    यावत् ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    यावत् ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    यावत् ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    यावत् (!cmd.empty() && std::है_खाली(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    प्रत्यागम cmd;
}

तार्किक V3PreProcImp::commentTokenMatch(string& cmdr, ध्रुव वर्ण* strg) {
    पूर्णाङ्क len = std::म_माप(strg);
    यदि (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::है_खाली(cmdr[len]))) {
        यदि (std::है_खाली(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        प्रत्यागम सत्य;
    } अन्यथा {
        प्रत्यागम असत्य;
    }
}

शून्य V3PreProcImp::comment(ध्रुव string& text) {
    // Comment detected.  Only keep relevant data.
    तार्किक printed = असत्य;
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        जोड़ो_Unreadback(text);
        printed = सत्य;
    }

    ध्रुव वर्ण* cp = text.c_str();
    यदि (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } अन्यथा {
        प्रत्यागम;
    }

    यावत् (std::है_खाली(*cp)) ++cp;

    तार्किक synth = असत्य;
    तार्किक vlcomment = असत्य;
    यदि ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::म_माप("verilator");
        यदि (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            प्रत्यागम;
        }
        vlcomment = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "synopsys")) {
        cp += std::म_माप("synopsys");
        यदि (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            प्रत्यागम;
        }
        synth = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "cadence")) {
        cp += std::म_माप("cadence");
        synth = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "pragma")) {
        cp += std::म_माप("pragma");
        synth = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::म_माप("ambit synthesis");
        synth = सत्य;
    } अन्यथा {
        प्रत्यागम;
    }

    यावत् (std::है_खाली(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    यदि (synth) {
        यदि (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            यदि (commentTokenMatch(cmd /*ref*/, "full_case")) {
                यदि (!printed) जोड़ो_Unreadback("/*verilator full_case*/");
            }
            यदि (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                यदि (!printed) जोड़ो_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } अन्यथा यदि (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        यदि (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::म_माप("public_flat_rw");
            यावत् (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            यावत् (std::है_खाली(arg[0])) arg = arg.substr(1);
            यदि (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            यदि (!printed) जोड़ो_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } अन्यथा यदि (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::म_माप("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            यावत् (std::है_खाली(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            यदि (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                प्रत्यागम;
            }
            यदि (!printed) जोड़ो_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } अन्यथा {
            यदि (!printed) जोड़ो_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_पंक्ति* V3PreProc::fileline() {
    ध्रुव V3PreProcImp* idatap = static_cast<V3PreProcImp*>(एतत्);
    प्रत्यागम idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ध्रुव वर्ण* V3PreProcImp::tokenName(पूर्णाङ्क tok) {
    विकल्प (tok) {
    पक्ष VP_BACKQUOTE: प्रत्यागम ("BACKQUOTE");
    पक्ष VP_COMMENT: प्रत्यागम ("COMMENT");
    पक्ष VP_DEFARG: प्रत्यागम ("DEFARG");
    पक्ष VP_DEFFORM: प्रत्यागम ("DEFFORM");
    पक्ष VP_DEFINE: प्रत्यागम ("DEFINE");
    पक्ष VP_DEFREF: प्रत्यागम ("DEFREF");
    पक्ष VP_DEFREF_JOIN: प्रत्यागम ("DEFREF_JOIN");
    पक्ष VP_DEFVALUE: प्रत्यागम ("DEFVALUE");
    पक्ष VP_ELSE: प्रत्यागम ("ELSE");
    पक्ष VP_ELSIF: प्रत्यागम ("ELSIF");
    पक्ष VP_ENDIF: प्रत्यागम ("ENDIF");
    पक्ष VP_EOF: प्रत्यागम ("EOF");
    पक्ष VP_EOF_ERROR: प्रत्यागम ("EOF_ERROR");
    पक्ष VP_ERROR: प्रत्यागम ("ERROR");
    पक्ष VP_IFDEF: प्रत्यागम ("IFDEF");
    पक्ष VP_IFNDEF: प्रत्यागम ("IFNDEF");
    पक्ष VP_JOIN: प्रत्यागम ("JOIN");
    पक्ष VP_INCLUDE: प्रत्यागम ("INCLUDE");
    पक्ष VP_पंक्ति: प्रत्यागम ("LINE");
    पक्ष VP_STRIFY: प्रत्यागम ("STRIFY");
    पक्ष VP_STRING: प्रत्यागम ("STRING");
    पक्ष VP_प्रतीक: प्रत्यागम ("SYMBOL");
    पक्ष VP_प्रतीक_JOIN: प्रत्यागम ("SYMBOL_JOIN");
    पक्ष VP_TEXT: प्रत्यागम ("TEXT");
    पक्ष VP_UNDEF: प्रत्यागम ("UNDEF");
    पक्ष VP_UNDEFINEALL: प्रत्यागम ("UNDEFINEALL");
    पक्ष VP_WHITE: प्रत्यागम ("WHITE");
    स्वभाव: प्रत्यागम ("?");
    }
}

शून्य V3PreProcImp::unputString(ध्रुव string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    यदि (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

शून्य V3PreProcImp::unputDefrefString(ध्रुव string& strg) {
    ध्रुव पूर्णाङ्क multiline = std::गिनती(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ध्रुव string& strg, तार्किक trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    यावत् (out.length() > leadspace && std::है_खाली(out[leadspace])) ++leadspace;
    यदि (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    यदि (trailing) {
        string::size_type trailspace = 0;
        यावत् (out.length() > trailspace && std::है_खाली(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        यदि (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        यदि (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    प्रत्यागम out;
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
    प्रति (अचिन्हित i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ध्रुव string मूल्य = def_मूल्य(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(मूल्य) << "'");

    std::map<ध्रुव string, string> arg_मूल्य_By_Name;
    {  // Parse argument list into map
        अचिन्हित अंक_Args = 0;
        string argName;
        पूर्णाङ्क paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        तार्किक quote = असत्य;
        तार्किक haveDefault = असत्य;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ध्रुव string params = refp->params();  // Must keep str in scope to get pointer
        ध्रुव वर्ण* cp = params.c_str();
        यदि (*cp == '(') cp++;
        प्रति (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            यदि (!quote && paren == 1) {
                यदि (*cp == ')' || *cp == ',') {
                    string मूल्य_Def;
                    यदि (haveDefault) {
                        मूल्य_Def = token;
                    } अन्यथा {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, सत्य);
                    UINFO(4, "    Got Arg=" << अंक_Args << "  argName='" << argName
                                            << "'  default='" << मूल्य_Def << "'");
                    // Parse it
                    यदि (argName != "") {
                        यदि (refp->args().size() > अंक_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ध्रुव string arg = trimWhitespace(refp->args()[अंक_Args], सत्य);
                            यदि (arg != "") मूल्य_Def = arg;
                        } अन्यथा यदि (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            प्रत्यागम " `" + refp->name() + " ";
                        }
                        अंक_Args++;
                    }
                    arg_मूल्य_By_Name[argName] = मूल्य_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = असत्य;
                    अनुवर्त;
                } अन्यथा यदि (*cp == '=') {
                    haveDefault = सत्य;
                    argName = token;
                    token = "";
                    अनुवर्त;
                }
            }
            यदि (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                अनुवर्त;
            }
            यदि (!quote) {
                यदि (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } अन्यथा यदि (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            यदि (*cp == '"') quote = !quote;
            यदि (*cp) token += *cp;
        }
        यदि (refp->args().size() > अंक_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && अंक_Args == 0
                 && trimWhitespace(refp->args()[0], असत्य) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            प्रत्यागम " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        तार्किक quote = असत्य;
        तार्किक triquote = असत्य;
        तार्किक backslashesc = असत्य;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        प्रति (ध्रुव वर्ण* cp = मूल्य.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            यदि (!quote && !triquote && *cp == '\\') {
                backslashesc = सत्य;
            } अन्यथा यदि (std::है_खाली(*cp)) {
                backslashesc = असत्य;
            }
            यदि (!quote && !triquote) {
                यदि (std::है_अक्षर(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::है_अंक(*cp))) {
                    argName += *cp;
                    अनुवर्त;
                }
                यदि (argName != "") {
                    // Found a possible variable substitution
                    ध्रुव स्वतः iter = arg_मूल्य_By_Name.find(argName);
                    यदि (iter != arg_मूल्य_By_Name.end()) {
                        // Substitute
                        ध्रुव string subst = iter->second;
                        यदि (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            यदि (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } अन्यथा {
                            out += subst;
                        }
                    } अन्यथा {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                यदि (cp[0] == '`' && cp[1] == '`') {
                    यदि (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } अन्यथा {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    अनुवर्त;
                }
            }
            यदि (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                अनुवर्त;
            } अन्यथा यदि (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                अनुवर्त;
            }
            यदि (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                अनुवर्त;
            }
            यदि (*cp == '"') quote = !quote;
            यदि (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    प्रत्यागम out;
}

//**********************************************************************
// Parser routines

शून्य V3PreProcImp::openFile(File_पंक्ति*, VInFilter* filterp, ध्रुव string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    यदि (m_incError) प्रत्यागम;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ध्रुव तार्किक ok = filterp->readWholefile(filename, wholefile /*ref*/);
    यदि (!ok) {
        fileline()->v3error("File not found: " + filename);
        प्रत्यागम;
    }

    यदि (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        यदि (m_lexp->m_streamp_ढेर.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = सत्य;
            प्रत्यागम;
        }
        // There's already a file active.  Push it to work on the new one.
        add_पंक्ति_Comment(0);
    }

    // Save file contents for future error reporting
    File_पंक्ति* ध्रुव flsp = नूतन File_पंक्ति{filename};
    flsp->lineno(1);
    flsp->newContent();
    प्रति (ध्रुव string& i : wholefile) flsp->contentp()->धकेलो_Text(i);

    // Save contents for V3Control --contents
    यदि (filename != V3Options::getStdPackagePath()) {
        तार्किक containsVlt = असत्य;
        प्रति (ध्रुव string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            यदि (i.find("`verilator_config") != string::npos) {
                containsVlt = सत्य;
                भङ्ग;
            }
        }
        यदि (!containsVlt) {
            प्रति (ध्रुव string& i : wholefile) V3Control::contents_धकेलो_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_पंक्ति_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    पूर्णाङ्क eof_newline = 0;  // Number of characters following last newline
    पूर्णाङ्क eof_lineno = 1;
    प्रति (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        तार्किक strip = असत्य;
        ध्रुव वर्ण* ध्रुव sp = it->data();
        ध्रुव वर्ण* ध्रुव ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        प्रति (ध्रुव वर्ण* cp = sp; cp < ep; cp++) {
            यदि (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = सत्य;
            } अन्यथा यदि (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } अन्यथा {
                ++eof_newline;
            }
        }
        यदि (strip) {
            string out;
            out.reserve(it->length());
            प्रति (ध्रुव वर्ण* cp = sp; cp < ep; cp++) {
                यदि (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    यदि (eof_newline) {
        File_पंक्ति* ध्रुव fl = नूतन File_पंक्ति{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

शून्य V3PreProcImp::जोड़ो_Unreadback_At_Bol(ध्रुव string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    यदि (m_पंक्ति_Cmt == "") {
        m_पंक्ति_Cmt_Nl = सत्य;
    } अन्यथा यदि (m_पंक्ति_Cmt[m_पंक्ति_Cmt.length() - 1] != '\n') {
        जोड़ो_Unreadback("\n");
    }
    जोड़ो_Unreadback(text);
}

शून्य V3PreProcImp::add_पंक्ति_Comment(पूर्णाङ्क enterExit) {
    यदि (पंक्ति_Directives()) {
        जोड़ो_Unreadback_At_Bol(m_lexp->curFilelinep()->पंक्ति_Directive_Strg(enterExit));
    }
}

शून्य V3PreProcImp::dumpDefines(std::ostream& os) {
    प्रति (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        यदि (!it->second.params().empty()) os << it->second.params();
        यदि (!it->second.मूल्य().empty()) os << " " << it->second.मूल्य();
        os << '\n';
    }
}

शून्य V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    प्रति (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->धकेलो_Candidate("`"s + it->first);
    }
}

पूर्णाङ्क V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    यावत् (सत्य) {
    next_tok:
        यदि (m_पंक्ति_Add) {
            m_पंक्ति_Add--;
            m_rawAtBol = सत्य;
            yyourtext("\n", 1);
            यदि (debug() >= 5) debugToken(VP_WHITE, "LNA");
            प्रत्यागम VP_WHITE;
        }
        यदि (m_पंक्ति_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            स्थिर string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_पंक्ति_Cmt;
            यदि (m_पंक्ति_Cmt_Nl) {
                यदि (!m_rawAtBol) s_rtncmt.जोड़ो(0, "\n");
                m_पंक्ति_Cmt_Nl = असत्य;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_पंक्ति_Cmt = "";
            यदि (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            यदि (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_मूल्य(yyourtext(), yyourleng());
                गच्छ next_tok;
            } अन्यथा {
                यदि (debug() >= 5) debugToken(VP_TEXT, "LCM");
                प्रत्यागम VP_TEXT;
            }
        }
        यदि (isEof()) प्रत्यागम VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        पूर्णाङ्क tok = m_lexp->lex();
        यदि (debug() >= 5) debugToken(tok, "RAW");

        यदि (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_पंक्ति = 0;
        } अन्यथा यदि (++m_tokens_On_पंक्ति > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        यदि (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            यदि (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            गच्छ next_tok;
        }

        यदि (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        प्रत्यागम tok;
    }
}

शून्य V3PreProcImp::debugToken(पूर्णाङ्क tok, ध्रुव वर्ण* cmtp) {
    ध्रुव स्थिर पूर्णाङ्क s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    यदि (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        यावत् ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        यावत् ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<पूर्णाङ्क>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        यदि (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

पूर्णाङ्क V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    यावत् (सत्य) {
    next_tok:
        यदि (isEof()) प्रत्यागम VP_EOF;
        पूर्णाङ्क tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        यदि (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) प्रत्यागम tok;
        यदि (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        यदि (tok == VP_COMMENT) {
            यदि (!m_off) {
                यदि (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    यदि (m_पंक्ति_Cmt.empty()) जोड़ो_Unreadback(" ");
                } अन्यथा यदि (m_lexp->m_keepComments) {
                    प्रत्यागम tok;
                } अन्यथा {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    जोड़ो_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            प्रति (माप_प्रकार len = 0; len < static_cast<माप_प्रकार>(yyourleng()); len++) {
                यदि (yyourtext()[len] == '\n') m_पंक्ति_Add++;
            }
            गच्छ next_tok;
        }
        यदि (tok == VP_पंक्ति) {
            add_पंक्ति_Comment(m_lexp->m_enterExit);
            गच्छ next_tok;
        }

        यदि (tok == VP_DEFREF_JOIN) {
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
            ध्रुव std::string name(yyourtext() + 1, yyourleng() - 1);
            यदि (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = सत्य;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } अन्यथा {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        यदि (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            यदि (tok == VP_JOIN) गच्छ next_tok;
            यदि (tok == VP_प्रतीक_JOIN) tok = VP_प्रतीक;
        }
        यदि (tok == VP_प्रतीक_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ध्रुव std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ढेर.धकेलो(out);
            state_धकेलो(ps_JOIN);
            गच्छ next_tok;
        }

        // Deal with some special parser states
        विकल्प (state()) {
        पक्ष ps_शीर्ष: {
            भङ्ग;
        }
        पक्ष ps_DEFNAME_UNDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_DEFINE:  // FALLTHRU
        पक्ष ps_DEFNAME_IFDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_IFNDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_ELSIF: {
            यदि (tok == VP_प्रतीक) {
                m_lastSym.assign(yyourtext(), yyourleng());
                यदि (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    तार्किक enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    यदि (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ढेर.emplace(VPreIfEntry{enable, असत्य});
                    यदि (!enable) parsingOff();
                    state_निकालो();
                    गच्छ next_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_ELSIF) {
                    यदि (m_ifdef_ढेर.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } अन्यथा {
                        // Handle `else portion
                        ध्रुव VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                        m_ifdef_ढेर.निकालो();
                        यदि (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ध्रुव तार्किक enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_ढेर.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        यदि (!enable) parsingOff();
                    }
                    state_निकालो();
                    गच्छ next_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_UNDEF) {
                    यदि (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_निकालो();
                    गच्छ next_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->धकेलो_State_Def_Form();
                    गच्छ next_tok;
                } अन्यथा {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                यदि (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->धकेलो_State_व्यंजक();
                    m_व्यंजक_Parser.reset(fileline());
                    m_व्यंजक_Paren_Level = 1;
                    विकल्प (state()) {
                    पक्ष ps_DEFNAME_IFDEF: stateChange(ps_व्यंजक_IFDEF); भङ्ग;
                    पक्ष ps_DEFNAME_IFNDEF: stateChange(ps_व्यंजक_IFNDEF); भङ्ग;
                    पक्ष ps_DEFNAME_ELSIF: stateChange(ps_व्यंजक_ELSIF); भङ्ग;
                    स्वभाव: v3fatalSrc("bad case");
                    }
                    गच्छ next_tok;
                }
                यदि (!m_off) {
                    प्रत्यागम tok;
                } अन्यथा {
                    गच्छ next_tok;
                }
            } अन्यथा यदि (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                भङ्ग;
            } अन्यथा {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                गच्छ next_tok;
            }
        }
        पक्ष ps_व्यंजक_IFDEF:  // FALLTHRU
        पक्ष ps_व्यंजक_IFNDEF:  // FALLTHRU
        पक्ष ps_व्यंजक_ELSIF: {
            // `ifdef ( *here*
            File_पंक्ति* ध्रुव flp = m_lexp->m_tokFilelinep;
            यदि (tok == VP_प्रतीक) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ध्रुव तार्किक exists = defExists(m_lastSym);
                यदि (exists) {
                    ध्रुव std::string मूल्य = def_मूल्य(m_lastSym);
                    यदि (VString::removeWhitespace(मूल्य) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << मूल्य
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, exists});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_WHITE) {
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::BRA});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_व्यंजक_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                यदि (--m_व्यंजक_Paren_Level > 0) {
                    m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::KET});
                    गच्छ next_tok;
                } अन्यथा {
                    // Done with parsing expression
                    तार्किक enable = m_व्यंजक_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    यदि (state() == ps_व्यंजक_IFDEF || state() == ps_व्यंजक_IFNDEF) {
                        यदि (state() == ps_व्यंजक_IFNDEF) enable = !enable;
                        m_ifdef_ढेर.emplace(VPreIfEntry{enable, असत्य});
                        यदि (!enable) parsingOff();
                        state_निकालो();
                        गच्छ next_tok;
                    } अन्यथा यदि (state() == ps_व्यंजक_ELSIF) {
                        यदि (m_ifdef_ढेर.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } अन्यथा {
                            // Handle `else portion
                            ध्रुव VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                            m_ifdef_ढेर.निकालो();
                            यदि (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_ढेर.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            यदि (!enable) parsingOff();
                        }
                        state_निकालो();
                    }
                    गच्छ next_tok;
                }
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LNOT});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "&&", 2)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LAND});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "||", 2)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LOR});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "->", 2)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::IMP});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 3 && 0 == म_भेदन(yyourtext(), "<->", 3)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::EQV});
                गच्छ next_tok;
            } अन्यथा {
                यदि (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    प्रत्यागम tok;
                } अन्यथा {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                गच्छ next_tok;
            }
        }
        पक्ष ps_DEFFORM: {
            यदि (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_मूल्य;
                यदि (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->धकेलो_State_Def_मूल्य();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT) {
                // IE, something like comment in formals
                यदि (!m_off) प्रत्यागम tok;
                गच्छ next_tok;
            } अन्यथा {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                गच्छ next_tok;
            }
        }
        पक्ष ps_DEFVALUE: {
            स्थिर string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            यदि (tok == VP_DEFVALUE) {
                यदि (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_मूल्य)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ध्रुव string formals = m_formals;
                string मूल्य = m_lexp->m_def_मूल्य;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                प्रति (माप_प्रकार i = 0; i < formals.length(); i++) {
                    यदि (formals[i] == '\n') s_newlines += "\n";
                }
                प्रति (माप_प्रकार i = 0; i < मूल्य.length(); i++) {
                    यदि (मूल्य[i] == '\n') s_newlines += "\n";
                }
                यदि (!m_off) {
                    // Remove leading and trailing whitespace
                    मूल्य = trimWhitespace(मूल्य, सत्य);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(मूल्य) << "'");
                    define(fileline(), m_lastSym, मूल्य, formals, असत्य);
                }
            } अन्यथा {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_निकालो();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            प्रत्यागम VP_WHITE;
        }
        पक्ष ps_DEFPAREN: {
            यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                गच्छ next_tok;
            } अन्यथा {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ध्रुव VDefineRef* ध्रुव refp = &(m_defRefs.शीर्ष());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_निकालो();
                गच्छ next_tok;
            }
        }
        पक्ष ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.शीर्ष());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य);
            m_lexp->m_def_मूल्य = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            यदि (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().धकेलो_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->धकेलो_State_Def_Arg(1);
                refp->nextarg("");
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().धकेलो_back(refp->nextarg());
                string out;
                यदि (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.निकालो(), refp);
                state_निकालो();
                यदि (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    यदि (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ढेर.empty(), "`` join stack empty, but in a ``");
                        ध्रुव string lhs = m_join_ढेर.शीर्ष();
                        m_join_ढेर.निकालो();
                        out.जोड़ो(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_निकालो();
                    }
                    यदि (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    अन्यथा यदि (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.शीर्ष().parenLevel();
                } अन्यथा {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.शीर्ष());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य + out);
                    m_lexp->m_def_मूल्य = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                भङ्ग;
            } अन्यथा यदि (tok == VP_प्रतीक || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_धकेलो(ps_STRIFY);
                गच्छ next_tok;
            } अन्यथा {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_निकालो();
                गच्छ next_tok;
            }
        }
        पक्ष ps_INCNAME: {
            यदि (tok == VP_STRING) {
                state_निकालो();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->धकेलो_State_Inc_Filename();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                भङ्ग;
            } अन्यथा {
                state_निकालो();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                गच्छ next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        पक्ष ps_ERRORNAME: {
            यदि (tok == VP_STRING) {
                यदि (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_निकालो();
                गच्छ next_tok;
            } अन्यथा {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_निकालो();
                गच्छ next_tok;
            }
        }
        पक्ष ps_JOIN: {
            यदि (tok == VP_प्रतीक || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ढेर.empty(), "`` join stack empty, but in a ``");
                ध्रुव string lhs = m_join_ढेर.शीर्ष();
                m_join_ढेर.निकालो();
                UINFO(5, "`` LHS:" << lhs);
                ध्रुव string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ध्रुव string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_निकालो();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ध्रुव string lhs = m_join_ढेर.शीर्ष();
                m_join_ढेर.निकालो();
                unputString(lhs);
                state_निकालो();
                गच्छ next_tok;
            } अन्यथा {
                // `define, etc, fall through and expand.  Pop back here.
                भङ्ग;
            }
        }
        पक्ष ps_STRIFY: {
            यदि (tok == VP_STRIFY) {
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
                state_निकालो();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                भङ्ग;
            } अन्यथा यदि (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                भङ्ग;
            } अन्यथा {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                गच्छ next_tok;
            }
        }
        स्वभाव: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        विकल्प (tok) {
        पक्ष VP_INCLUDE:
            यदि (!m_off) {
                state_धकेलो(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            गच्छ next_tok;
        पक्ष VP_UNDEF: state_धकेलो(ps_DEFNAME_UNDEF); गच्छ next_tok;
        पक्ष VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_धकेलो(ps_DEFNAME_DEFINE);
            गच्छ next_tok;
        पक्ष VP_IFDEF: state_धकेलो(ps_DEFNAME_IFDEF); गच्छ next_tok;
        पक्ष VP_IFNDEF: state_धकेलो(ps_DEFNAME_IFNDEF); गच्छ next_tok;
        पक्ष VP_ELSIF: state_धकेलो(ps_DEFNAME_ELSIF); गच्छ next_tok;
        पक्ष VP_ELSE:
            यदि (m_ifdef_ढेर.empty()) {
                fileline()->v3error("`else with no matching `if");
            } अन्यथा {
                ध्रुव VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                m_ifdef_ढेर.निकालो();
                ध्रुव तार्किक enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ढेर.emplace(VPreIfEntry{enable, lastIf.everOn()});
                यदि (!lastIf.on()) parsingOn();
                यदि (!enable) parsingOff();
            }
            गच्छ next_tok;
        पक्ष VP_ENDIF:
            UINFO(4, "Endif ");
            यदि (m_ifdef_ढेर.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } अन्यथा {
                ध्रुव VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                m_ifdef_ढेर.निकालो();
                यदि (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            गच्छ next_tok;

        पक्ष VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ध्रुव string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            यदि (m_defPutJoin) {
                m_defPutJoin = असत्य;
                unputString("``");
            }
            यदि (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                गच्छ next_tok;
            }
            // Substitute
            यदि (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                यदि (m_off) {
                    गच्छ next_tok;
                } अन्यथा {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    गच्छ next_tok;
                }
            } अन्यथा {
                ध्रुव string params = defParams(name);
                यदि (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    यदि (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    यदि (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        यदि (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ढेर.empty(), "`` join stack empty, but in a ``");
                            ध्रुव string lhs = m_join_ढेर.शीर्ष();
                            m_join_ढेर.निकालो();
                            out.जोड़ो(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_निकालो();
                        }
                        यदि (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        अन्यथा यदि (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } अन्यथा {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ध्रुव refp = &(m_defRefs.शीर्ष());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य + out);
                        m_lexp->m_def_मूल्य = "";
                    }
                    गच्छ next_tok;
                } अन्यथा {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    यदि (!m_defRefs.empty()) m_defRefs.शीर्ष().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_धकेलो(ps_DEFPAREN);
                    m_lexp->धकेलो_State_Def_Arg(0);
                    गच्छ next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            गच्छ next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        पक्ष VP_ERROR: {
            state_धकेलो(ps_ERRORNAME);
            गच्छ next_tok;
        }
        पक्ष VP_EOF:
            यदि (!m_ifdef_ढेर.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            प्रत्यागम tok;
        पक्ष VP_UNDEFINEALL:
            यदि (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            गच्छ next_tok;
        पक्ष VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_धकेलो(ps_STRIFY);
            गच्छ next_tok;
        पक्ष VP_प्रतीक:
        पक्ष VP_STRING:
        पक्ष VP_TEXT: {
            m_defDepth = 0;
            यदि (!m_off) {
                प्रत्यागम tok;
            } अन्यथा {
                गच्छ next_tok;
            }
        }
        पक्ष VP_WHITE:  // Handled at top of loop
        पक्ष VP_COMMENT:  // Handled at top of loop
        पक्ष VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        पक्ष VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        स्वभाव:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            भङ्ग;  // LCOV_EXCL_LINE
        }
        प्रत्यागम tok;
    }
}

पूर्णाङ्क V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    यदि (!m_finAhead) {
        m_finAhead = सत्य;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ध्रुव पूर्णाङ्क tok = m_finToken;
    buf = m_finBuf;
    यदि (असत्य && debug() >= 5) {
        ध्रुव string bufcln = V3PreLex::cleanDbgStrg(buf);
        ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ध्रुव वर्ण* bufp = buf.c_str();
    यावत् (*bufp == '\n') bufp++;
    यदि ((tok == VP_TEXT || tok == VP_पंक्ति) && VString::startsWith(bufp, "`line ")) {
        पूर्णाङ्क enter;
        m_finFilelinep->पंक्ति_Directive(bufp, enter /*ref*/);
    } अन्यथा {
        यदि (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->पंक्ति_Directives()) {
            यदि (ध्रुव पूर्णाङ्क outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                यदि (debug() >= 5) {
                    ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                यदि (outBehind > 0
                    && (outBehind <= static_cast<पूर्णाङ्क>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    यदि (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        प्रत्यागम VP_TEXT;
                    }
                } अन्यथा {
                    // Need to backup, use `line
                    buf = m_finFilelinep->पंक्ति_Directive_Strg(0);
                    प्रत्यागम VP_पंक्ति;
                }
            }
        }
        // Track newlines in prep for next token
        प्रति (ध्रुव वर्ण& c : buf) {
            यदि (c == '\n') {
                m_finAtBol = सत्य;
                m_finFilelinep->linenoInc();
            } अन्यथा {
                m_finAtBol = असत्य;
            }
        }
    }
    m_finAhead = असत्य;  // Consumed the token
    प्रत्यागम tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    यदि (isEof()) प्रत्यागम "";
    ध्रुव वर्ण* rtnp;
    तार्किक gotEof = असत्य;
    यावत् (nullptr == (rtnp = std::म_अक्षर(m_पंक्ति_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ध्रुव पूर्णाङ्क tok = getFinalToken(m_tokenBuf /*ref*/);
        यदि (debug() >= 5) {
            ध्रुव string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        यदि (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            यदि (m_पंक्ति_Chars != "" && m_पंक्ति_Chars[m_पंक्ति_Chars.length() - 1] != '\n') {
                m_पंक्ति_Chars.append("\n");
            }
            gotEof = सत्य;
        } अन्यथा {
            m_पंक्ति_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ध्रुव पूर्णाङ्क len = rtnp - m_पंक्ति_Chars.c_str() + 1;
    string the_पंक्ति(m_पंक्ति_Chars, 0, len);
    m_पंक्ति_Chars = m_पंक्ति_Chars.erase(0, len);  // Remove returned characters
    यदि (debug() >= 4) {
        ध्रुव string lncln = V3PreLex::cleanDbgStrg(the_पंक्ति);
        ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    प्रत्यागम the_पंक्ति;
}
