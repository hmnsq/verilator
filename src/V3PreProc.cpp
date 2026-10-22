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
    File_ओळ* स्थिरांक m_fileline;  // Where it was declared
    स्थिरांक string m_मूल्य;  // Value of define
    स्थिरांक string m_params;  // Parameters
    स्थिरांक तार्किक m_cmdline;  // Set on command line, don't `undefineall
सार्वजनिक:
    VDefine(File_ओळ* fl, स्थिरांक string& मूल्य, स्थिरांक string& params, तार्किक cmdline)
        : m_fileline{fl}
        , m_मूल्य{मूल्य}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_ओळ* fileline() स्थिरांक { परत m_fileline; }
    string मूल्य() स्थिरांक { परत m_मूल्य; }
    string params() स्थिरांक { परत m_params; }
    तार्किक cmdline() स्थिरांक { परत m_cmdline; }
};

//*************************************************************************

वर्ग VDefineRef final {
    // One for each pending define substitution
    स्थिरांक string m_name;  // Define last name being defined
    स्थिरांक string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    पूर्णांक m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
सार्वजनिक:
    string name() स्थिरांक { परत m_name; }
    string params() स्थिरांक { परत m_params; }
    string nextarg() स्थिरांक { परत m_nextarg; }
    शून्य nextarg(स्थिरांक string& मूल्य) { m_nextarg = मूल्य; }
    पूर्णांक parenLevel() स्थिरांक { परत m_parenLevel; }
    शून्य parenLevel(पूर्णांक मूल्य) { m_parenLevel = मूल्य; }
    std::vector<string>& args() { परत m_args; }
    VDefineRef(स्थिरांक string& name, स्थिरांक string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = पूर्वनिर्धारित;
};

//*************************************************************************
/// Data for parsing on/off

वर्ग VPreIfEntry final {
    // One for each pending ifdef/ifndef
    स्थिरांक तार्किक m_on;  // Current parse for this ifdef level is "on"
    स्थिरांक तार्किक m_everOn;  // Some if term in elsif tree has been on
सार्वजनिक:
    तार्किक on() स्थिरांक { परत m_on; }
    तार्किक everOn() स्थिरांक { परत m_everOn; }
    VPreIfEntry(तार्किक on, तार्किक everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = पूर्वनिर्धारित;
};

//*************************************************************************
// Data for a preprocessor instantiation.

वर्ग V3PreProcImp final : सार्वजनिक V3PreProc {
सार्वजनिक:
    // TYPES
    using DefinesMap = std::map<स्थिरांक std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    स्थिरांक V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    पूर्णांक m_lastLineno = 0;  // Last line number (stall detection)
    पूर्णांक m_tokens_On_ओळ = 0;  // Number of tokens on line (stall detection)

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
        ps_अभिव्यक्ती_IFDEF,
        ps_अभिव्यक्ती_IFNDEF,
        ps_अभिव्यक्ती_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    स्थिर स्थिरांक अक्षर* procStateName(ProcState s) {
        स्थिर स्थिरांक अक्षर* स्थिरांक states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        परत states[s];
    }

    std::चळत<ProcState> m_states;  ///< Current state of parser
    पूर्णांक m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    तार्किक m_incError = असत्य;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_ओळ_Cmt;  ///< Line comment(s) to be returned
    तार्किक m_ओळ_Cmt_Nl = असत्य;  ///< Newline needed before inserting lineCmt
    पूर्णांक m_ओळ_Add = 0;  ///< Empty lines to return to maintain line count
    तार्किक m_rawAtBol = सत्य;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    तार्किक m_finAhead = असत्य;  ///< Have read a token ahead
    पूर्णांक m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    तार्किक m_finAtBol = सत्य;  ///< Last getFinalToken left us at beginning of line
    File_ओळ* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::चळत<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::चळत<VPreIfEntry> m_ifdef_चळत;  ///< Stack of true/false emitting evaluations
    अचिन्हित m_defDepth = 0;  ///< How many `defines deep
    तार्किक m_defPutJoin = असत्य;  ///< Insert `` after substitution

    // For `` join
    std::चळत<string> m_join_चळत;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_अभिव्यक्ती m_अभिव्यक्ती_Parser;  ///< Parser for () expression
    पूर्णांक m_अभिव्यक्ती_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_ओळ_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    शून्य v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    स्थिर स्थिरांक अक्षर* tokenName(पूर्णांक tok);
    शून्य debugToken(पूर्णांक tok, स्थिरांक अक्षर* cmtp);
    शून्य parse_शीर्ष();
    शून्य parseUndef();

खाजगी:
    // Internal methods
    शून्य endOfOneFile();
    string defineSubst(VDefineRef* refp);

    तार्किक defExists(स्थिरांक string& name);
    तार्किक defCmdline(स्थिरांक string& name);
    string def_मूल्य(स्थिरांक string& name);
    string defParams(स्थिरांक string& name);
    File_ओळ* defFileline(स्थिरांक string& name);

    string commentCleanup(स्थिरांक string& text);
    तार्किक commentTokenMatch(string& cmdr, स्थिरांक अक्षर* strg);
    स्थिर string trimWhitespace(स्थिरांक string& strg, तार्किक trailing);
    शून्य unputString(स्थिरांक string& strg);
    शून्य unputDefrefString(स्थिरांक string& strg);

    शून्य parsingOn() {
        m_off--;
        जर (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    शून्य parsingOff() { m_off++; }

    पूर्णांक getRawToken();
    पूर्णांक getStateToken();
    पूर्णांक getFinalToken(string& buf);

    ProcState state() स्थिरांक { परत m_states.शीर्ष(); }
    तार्किक stateIsDefname() स्थिरांक {
        परत state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    शून्य state_ढकला(ProcState state) { m_states.ढकला(state); }
    शून्य state_काढा() {
        m_states.काढा();
        जर (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.ढकला(ps_शीर्ष);  // LCOV_EXCL_LINE
        }
    }
    शून्य stateChange(ProcState state) {
        state_काढा();
        state_ढकला(state);
    }

सार्वजनिक:
    // METHODS, called from upper level shell
    शून्य openFile(File_ओळ* fl, VInFilter* filterp, स्थिरांक string& filename) override;
    तार्किक isEof() स्थिरांक override { परत m_lexp->curStreamp()->m_eof; }
    शून्य forceEof() { m_lexp->curStreamp()->m_eof = सत्य; }
    string getline() override;
    शून्य घाला_Unreadback(स्थिरांक string& text) override { m_ओळ_Cmt += text; }
    शून्य घाला_Unreadback_At_Bol(स्थिरांक string& text);
    शून्य add_ओळ_Comment(पूर्णांक enterExit);
    शून्य dumpDefines(std::ostream& os) override;
    शून्य candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    शून्य comment(स्थिरांक string& text) override;  // Comment detected (if keepComments==2)
    शून्य include(स्थिरांक string& filename) override;  // Request a include file be processed
    शून्य undef(स्थिरांक string& name) override;
    आभासी शून्य undefineall();
    शून्य define(File_ओळ* fl, स्थिरांक string& name, स्थिरांक string& मूल्य, स्थिरांक string& params,
                तार्किक cmdline) override;
    string removeDefines(स्थिरांक string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.ढकला(ps_शीर्ष); }
    // cppcheck-suppress duplInheritedMember
    शून्य configure(File_ओळ* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = हे;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = नवीन File_ओळ{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = नवीन V3PreLex{हे, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        जर (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_ओळ* fl) {
    V3PreProcImp* स्थिरांक preprocp = नवीन V3PreProcImp;
    preprocp->configure(fl);
    परत preprocp;
}

शून्य V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_अभिव्यक्ती::selfTest(); }

//*************************************************************************
// Defines

शून्य V3PreProcImp::undef(स्थिरांक string& name) {
    जर (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        घाला_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
शून्य V3PreProcImp::undefineall() {
    जर (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        घाला_Unreadback("`undefineall\n");
    साठी (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        जर (!it->second.cmdline()) m_defines.erase(it);
    }
}
तार्किक V3PreProcImp::defExists(स्थिरांक string& name) {
    स्थिरांक स्वतः iter = m_defines.find(name);
    परत (iter != m_defines.end());
}
तार्किक V3PreProcImp::defCmdline(स्थिरांक string& name) {
    स्थिरांक स्वतः iter = m_defines.find(name);
    जर (iter == m_defines.end()) { परत असत्य; }
    परत iter->second.cmdline();
}
string V3PreProcImp::def_मूल्य(स्थिरांक string& name) {
    स्थिरांक स्वतः iter = m_defines.find(name);
    जर (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        परत "";
    }
    परत iter->second.मूल्य();
}
string V3PreProcImp::defParams(स्थिरांक string& name) {
    स्थिरांक स्वतः iter = m_defines.find(name);
    जर (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        परत "";
    }
    परत iter->second.params();
}
File_ओळ* V3PreProcImp::defFileline(स्थिरांक string& name) {
    स्थिरांक स्वतः iter = m_defines.find(name);
    जर (iter == m_defines.end()) परत nullptr;
    परत iter->second.fileline();
}
शून्य V3PreProcImp::define(File_ओळ* fl, स्थिरांक string& name, स्थिरांक string& मूल्य,
                          स्थिरांक string& params, तार्किक cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << मूल्य << "' params '" << params << "'");
    जर (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(मूल्य.size());
        साठी (string::size_type pos = 0; pos < मूल्य.size(); ++pos) {
            जर (मूल्य[pos] == '\n' && (pos == 0 || मूल्य[pos - 1] != '\\')) out += '\\';
            out += मूल्य[pos];
        }
        घाला_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    जर (!V3_Language_शब्द::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } अन्यथा {
        जर (defExists(name)) {
            जर (defCmdline(name) && !cmdline) {
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
                परत;
            } अन्यथा {
                जर (!(def_मूल्य(name) == मूल्य
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

string V3PreProcImp::removeDefines(स्थिरांक string& text) {
    string val;
    string rtnsym = text;
    साठी (पूर्णांक loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        जर (xsym[0] == '`') xsym.erase(0, 1);
        जर (defExists(xsym)) {
            val = def_मूल्य(xsym);
            जर (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } अन्यथा {
                थांबा;
            }
        } अन्यथा {
            थांबा;
        }
    }
    परत rtnsym;  // NA
}

//**********************************************************************
// Comments

शून्य V3PreProcImp::include(स्थिरांक string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(स्थिरांक string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    पर्यंत ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    पर्यंत ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    पर्यंत ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    पर्यंत ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    पर्यंत ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    पर्यंत ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    पर्यंत (!cmd.empty() && std::है_खाली(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    परत cmd;
}

तार्किक V3PreProcImp::commentTokenMatch(string& cmdr, स्थिरांक अक्षर* strg) {
    पूर्णांक len = std::म_माप(strg);
    जर (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::है_खाली(cmdr[len]))) {
        जर (std::है_खाली(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        परत सत्य;
    } अन्यथा {
        परत असत्य;
    }
}

शून्य V3PreProcImp::comment(स्थिरांक string& text) {
    // Comment detected.  Only keep relevant data.
    तार्किक printed = असत्य;
    जर (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        घाला_Unreadback(text);
        printed = सत्य;
    }

    स्थिरांक अक्षर* cp = text.c_str();
    जर (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } अन्यथा {
        परत;
    }

    पर्यंत (std::है_खाली(*cp)) ++cp;

    तार्किक synth = असत्य;
    तार्किक vlcomment = असत्य;
    जर ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::म_माप("verilator");
        जर (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            परत;
        }
        vlcomment = सत्य;
    } अन्यथा जर (VString::startsWith(cp, "synopsys")) {
        cp += std::म_माप("synopsys");
        जर (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            परत;
        }
        synth = सत्य;
    } अन्यथा जर (VString::startsWith(cp, "cadence")) {
        cp += std::म_माप("cadence");
        synth = सत्य;
    } अन्यथा जर (VString::startsWith(cp, "pragma")) {
        cp += std::म_माप("pragma");
        synth = सत्य;
    } अन्यथा जर (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::म_माप("ambit synthesis");
        synth = सत्य;
    } अन्यथा {
        परत;
    }

    पर्यंत (std::है_खाली(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    जर (synth) {
        जर (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            जर (commentTokenMatch(cmd /*ref*/, "full_case")) {
                जर (!printed) घाला_Unreadback("/*verilator full_case*/");
            }
            जर (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                जर (!printed) घाला_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } अन्यथा जर (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        जर (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::म_माप("public_flat_rw");
            पर्यंत (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            पर्यंत (std::है_खाली(arg[0])) arg = arg.substr(1);
            जर (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            जर (!printed) घाला_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } अन्यथा जर (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::म_माप("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            पर्यंत (std::है_खाली(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            जर (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                परत;
            }
            जर (!printed) घाला_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } अन्यथा {
            जर (!printed) घाला_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_ओळ* V3PreProc::fileline() {
    स्थिरांक V3PreProcImp* idatap = static_cast<V3PreProcImp*>(हे);
    परत idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

स्थिरांक अक्षर* V3PreProcImp::tokenName(पूर्णांक tok) {
    निवड (tok) {
    स्थिती VP_BACKQUOTE: परत ("BACKQUOTE");
    स्थिती VP_COMMENT: परत ("COMMENT");
    स्थिती VP_DEFARG: परत ("DEFARG");
    स्थिती VP_DEFFORM: परत ("DEFFORM");
    स्थिती VP_DEFINE: परत ("DEFINE");
    स्थिती VP_DEFREF: परत ("DEFREF");
    स्थिती VP_DEFREF_JOIN: परत ("DEFREF_JOIN");
    स्थिती VP_DEFVALUE: परत ("DEFVALUE");
    स्थिती VP_ELSE: परत ("ELSE");
    स्थिती VP_ELSIF: परत ("ELSIF");
    स्थिती VP_ENDIF: परत ("ENDIF");
    स्थिती VP_EOF: परत ("EOF");
    स्थिती VP_EOF_ERROR: परत ("EOF_ERROR");
    स्थिती VP_ERROR: परत ("ERROR");
    स्थिती VP_IFDEF: परत ("IFDEF");
    स्थिती VP_IFNDEF: परत ("IFNDEF");
    स्थिती VP_JOIN: परत ("JOIN");
    स्थिती VP_INCLUDE: परत ("INCLUDE");
    स्थिती VP_ओळ: परत ("LINE");
    स्थिती VP_STRIFY: परत ("STRIFY");
    स्थिती VP_STRING: परत ("STRING");
    स्थिती VP_चिन्ह: परत ("SYMBOL");
    स्थिती VP_चिन्ह_JOIN: परत ("SYMBOL_JOIN");
    स्थिती VP_TEXT: परत ("TEXT");
    स्थिती VP_UNDEF: परत ("UNDEF");
    स्थिती VP_UNDEFINEALL: परत ("UNDEFINEALL");
    स्थिती VP_WHITE: परत ("WHITE");
    पूर्वनिर्धारित: परत ("?");
    }
}

शून्य V3PreProcImp::unputString(स्थिरांक string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    जर (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

शून्य V3PreProcImp::unputDefrefString(स्थिरांक string& strg) {
    स्थिरांक पूर्णांक multiline = std::मोजणी(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(स्थिरांक string& strg, तार्किक trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    पर्यंत (out.length() > leadspace && std::है_खाली(out[leadspace])) ++leadspace;
    जर (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    जर (trailing) {
        string::size_type trailspace = 0;
        पर्यंत (out.length() > trailspace && std::है_खाली(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        जर (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        जर (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    परत out;
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
    साठी (अचिन्हित i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    स्थिरांक string मूल्य = def_मूल्य(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(मूल्य) << "'");

    std::map<स्थिरांक string, string> arg_मूल्य_By_Name;
    {  // Parse argument list into map
        अचिन्हित अंक_Args = 0;
        string argName;
        पूर्णांक paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        तार्किक quote = असत्य;
        तार्किक haveDefault = असत्य;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        स्थिरांक string params = refp->params();  // Must keep str in scope to get pointer
        स्थिरांक अक्षर* cp = params.c_str();
        जर (*cp == '(') cp++;
        साठी (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            जर (!quote && paren == 1) {
                जर (*cp == ')' || *cp == ',') {
                    string मूल्य_Def;
                    जर (haveDefault) {
                        मूल्य_Def = token;
                    } अन्यथा {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, सत्य);
                    UINFO(4, "    Got Arg=" << अंक_Args << "  argName='" << argName
                                            << "'  default='" << मूल्य_Def << "'");
                    // Parse it
                    जर (argName != "") {
                        जर (refp->args().size() > अंक_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            स्थिरांक string arg = trimWhitespace(refp->args()[अंक_Args], सत्य);
                            जर (arg != "") मूल्य_Def = arg;
                        } अन्यथा जर (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            परत " `" + refp->name() + " ";
                        }
                        अंक_Args++;
                    }
                    arg_मूल्य_By_Name[argName] = मूल्य_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = असत्य;
                    सुरूठेवा;
                } अन्यथा जर (*cp == '=') {
                    haveDefault = सत्य;
                    argName = token;
                    token = "";
                    सुरूठेवा;
                }
            }
            जर (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                सुरूठेवा;
            }
            जर (!quote) {
                जर (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } अन्यथा जर (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            जर (*cp == '"') quote = !quote;
            जर (*cp) token += *cp;
        }
        जर (refp->args().size() > अंक_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && अंक_Args == 0
                 && trimWhitespace(refp->args()[0], असत्य) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            परत " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        तार्किक quote = असत्य;
        तार्किक triquote = असत्य;
        तार्किक backslashesc = असत्य;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        साठी (स्थिरांक अक्षर* cp = मूल्य.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            जर (!quote && !triquote && *cp == '\\') {
                backslashesc = सत्य;
            } अन्यथा जर (std::है_खाली(*cp)) {
                backslashesc = असत्य;
            }
            जर (!quote && !triquote) {
                जर (std::है_अक्षर(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::है_अंक(*cp))) {
                    argName += *cp;
                    सुरूठेवा;
                }
                जर (argName != "") {
                    // Found a possible variable substitution
                    स्थिरांक स्वतः iter = arg_मूल्य_By_Name.find(argName);
                    जर (iter != arg_मूल्य_By_Name.end()) {
                        // Substitute
                        स्थिरांक string subst = iter->second;
                        जर (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            जर (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
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
                जर (cp[0] == '`' && cp[1] == '`') {
                    जर (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } अन्यथा {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    सुरूठेवा;
                } अन्यथा जर (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    सुरूठेवा;
                } अन्यथा जर (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    सुरूठेवा;
                } अन्यथा जर (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    सुरूठेवा;
                } अन्यथा जर (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    सुरूठेवा;
                }
            }
            जर (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                सुरूठेवा;
            } अन्यथा जर (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                सुरूठेवा;
            }
            जर (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                सुरूठेवा;
            }
            जर (*cp == '"') quote = !quote;
            जर (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    परत out;
}

//**********************************************************************
// Parser routines

शून्य V3PreProcImp::openFile(File_ओळ*, VInFilter* filterp, स्थिरांक string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    जर (m_incError) परत;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    स्थिरांक तार्किक ok = filterp->readWholefile(filename, wholefile /*ref*/);
    जर (!ok) {
        fileline()->v3error("File not found: " + filename);
        परत;
    }

    जर (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        जर (m_lexp->m_streamp_चळत.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = सत्य;
            परत;
        }
        // There's already a file active.  Push it to work on the new one.
        add_ओळ_Comment(0);
    }

    // Save file contents for future error reporting
    File_ओळ* स्थिरांक flsp = नवीन File_ओळ{filename};
    flsp->lineno(1);
    flsp->newContent();
    साठी (स्थिरांक string& i : wholefile) flsp->contentp()->ढकला_Text(i);

    // Save contents for V3Control --contents
    जर (filename != V3Options::getStdPackagePath()) {
        तार्किक containsVlt = असत्य;
        साठी (स्थिरांक string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            जर (i.find("`verilator_config") != string::npos) {
                containsVlt = सत्य;
                थांबा;
            }
        }
        जर (!containsVlt) {
            साठी (स्थिरांक string& i : wholefile) V3Control::contents_ढकला_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_ओळ_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    पूर्णांक eof_newline = 0;  // Number of characters following last newline
    पूर्णांक eof_lineno = 1;
    साठी (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        तार्किक strip = असत्य;
        स्थिरांक अक्षर* स्थिरांक sp = it->data();
        स्थिरांक अक्षर* स्थिरांक ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        साठी (स्थिरांक अक्षर* cp = sp; cp < ep; cp++) {
            जर (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = सत्य;
            } अन्यथा जर (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } अन्यथा {
                ++eof_newline;
            }
        }
        जर (strip) {
            string out;
            out.reserve(it->length());
            साठी (स्थिरांक अक्षर* cp = sp; cp < ep; cp++) {
                जर (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    जर (eof_newline) {
        File_ओळ* स्थिरांक fl = नवीन File_ओळ{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

शून्य V3PreProcImp::घाला_Unreadback_At_Bol(स्थिरांक string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    जर (m_ओळ_Cmt == "") {
        m_ओळ_Cmt_Nl = सत्य;
    } अन्यथा जर (m_ओळ_Cmt[m_ओळ_Cmt.length() - 1] != '\n') {
        घाला_Unreadback("\n");
    }
    घाला_Unreadback(text);
}

शून्य V3PreProcImp::add_ओळ_Comment(पूर्णांक enterExit) {
    जर (ओळ_Directives()) {
        घाला_Unreadback_At_Bol(m_lexp->curFilelinep()->ओळ_Directive_Strg(enterExit));
    }
}

शून्य V3PreProcImp::dumpDefines(std::ostream& os) {
    साठी (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        जर (!it->second.params().empty()) os << it->second.params();
        जर (!it->second.मूल्य().empty()) os << " " << it->second.मूल्य();
        os << '\n';
    }
}

शून्य V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    साठी (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->ढकला_Candidate("`"s + it->first);
    }
}

पूर्णांक V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    पर्यंत (सत्य) {
    next_tok:
        जर (m_ओळ_Add) {
            m_ओळ_Add--;
            m_rawAtBol = सत्य;
            yyourtext("\n", 1);
            जर (debug() >= 5) debugToken(VP_WHITE, "LNA");
            परत VP_WHITE;
        }
        जर (m_ओळ_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            स्थिर string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_ओळ_Cmt;
            जर (m_ओळ_Cmt_Nl) {
                जर (!m_rawAtBol) s_rtncmt.घाला(0, "\n");
                m_ओळ_Cmt_Nl = असत्य;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_ओळ_Cmt = "";
            जर (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            जर (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_मूल्य(yyourtext(), yyourleng());
                जा next_tok;
            } अन्यथा {
                जर (debug() >= 5) debugToken(VP_TEXT, "LCM");
                परत VP_TEXT;
            }
        }
        जर (isEof()) परत VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        पूर्णांक tok = m_lexp->lex();
        जर (debug() >= 5) debugToken(tok, "RAW");

        जर (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_ओळ = 0;
        } अन्यथा जर (++m_tokens_On_ओळ > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        जर (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            जर (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            जा next_tok;
        }

        जर (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        परत tok;
    }
}

शून्य V3PreProcImp::debugToken(पूर्णांक tok, स्थिरांक अक्षर* cmtp) {
    स्थिरांक स्थिर पूर्णांक s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    जर (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        पर्यंत ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        पर्यंत ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        स्थिरांक string flcol = m_lexp->m_tokFilelinep->ascii_ओळ_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<पूर्णांक>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        जर (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

पूर्णांक V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    पर्यंत (सत्य) {
    next_tok:
        जर (isEof()) परत VP_EOF;
        पूर्णांक tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        जर (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) परत tok;
        जर (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        जर (tok == VP_COMMENT) {
            जर (!m_off) {
                जर (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    जर (m_ओळ_Cmt.empty()) घाला_Unreadback(" ");
                } अन्यथा जर (m_lexp->m_keepComments) {
                    परत tok;
                } अन्यथा {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    घाला_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            साठी (माप_प्रकार len = 0; len < static_cast<माप_प्रकार>(yyourleng()); len++) {
                जर (yyourtext()[len] == '\n') m_ओळ_Add++;
            }
            जा next_tok;
        }
        जर (tok == VP_ओळ) {
            add_ओळ_Comment(m_lexp->m_enterExit);
            जा next_tok;
        }

        जर (tok == VP_DEFREF_JOIN) {
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
            स्थिरांक std::string name(yyourtext() + 1, yyourleng() - 1);
            जर (defExists(name)) {  // JOIN(DEFREF)
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
        जर (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            जर (tok == VP_JOIN) जा next_tok;
            जर (tok == VP_चिन्ह_JOIN) tok = VP_चिन्ह;
        }
        जर (tok == VP_चिन्ह_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            स्थिरांक std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_चळत.ढकला(out);
            state_ढकला(ps_JOIN);
            जा next_tok;
        }

        // Deal with some special parser states
        निवड (state()) {
        स्थिती ps_शीर्ष: {
            थांबा;
        }
        स्थिती ps_DEFNAME_UNDEF:  // FALLTHRU
        स्थिती ps_DEFNAME_DEFINE:  // FALLTHRU
        स्थिती ps_DEFNAME_IFDEF:  // FALLTHRU
        स्थिती ps_DEFNAME_IFNDEF:  // FALLTHRU
        स्थिती ps_DEFNAME_ELSIF: {
            जर (tok == VP_चिन्ह) {
                m_lastSym.assign(yyourtext(), yyourleng());
                जर (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    तार्किक enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    जर (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_चळत.emplace(VPreIfEntry{enable, असत्य});
                    जर (!enable) parsingOff();
                    state_काढा();
                    जा next_tok;
                } अन्यथा जर (state() == ps_DEFNAME_ELSIF) {
                    जर (m_ifdef_चळत.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } अन्यथा {
                        // Handle `else portion
                        स्थिरांक VPreIfEntry lastIf = m_ifdef_चळत.शीर्ष();
                        m_ifdef_चळत.काढा();
                        जर (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        स्थिरांक तार्किक enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_चळत.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        जर (!enable) parsingOff();
                    }
                    state_काढा();
                    जा next_tok;
                } अन्यथा जर (state() == ps_DEFNAME_UNDEF) {
                    जर (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_काढा();
                    जा next_tok;
                } अन्यथा जर (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ढकला_State_Def_Form();
                    जा next_tok;
                } अन्यथा {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                जर (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ढकला_State_अभिव्यक्ती();
                    m_अभिव्यक्ती_Parser.reset(fileline());
                    m_अभिव्यक्ती_Paren_Level = 1;
                    निवड (state()) {
                    स्थिती ps_DEFNAME_IFDEF: stateChange(ps_अभिव्यक्ती_IFDEF); थांबा;
                    स्थिती ps_DEFNAME_IFNDEF: stateChange(ps_अभिव्यक्ती_IFNDEF); थांबा;
                    स्थिती ps_DEFNAME_ELSIF: stateChange(ps_अभिव्यक्ती_ELSIF); थांबा;
                    पूर्वनिर्धारित: v3fatalSrc("bad case");
                    }
                    जा next_tok;
                }
                जर (!m_off) {
                    परत tok;
                } अन्यथा {
                    जा next_tok;
                }
            } अन्यथा जर (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                थांबा;
            } अन्यथा {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                जा next_tok;
            }
        }
        स्थिती ps_अभिव्यक्ती_IFDEF:  // FALLTHRU
        स्थिती ps_अभिव्यक्ती_IFNDEF:  // FALLTHRU
        स्थिती ps_अभिव्यक्ती_ELSIF: {
            // `ifdef ( *here*
            File_ओळ* स्थिरांक flp = m_lexp->m_tokFilelinep;
            जर (tok == VP_चिन्ह) {
                m_lastSym.assign(yyourtext(), yyourleng());
                स्थिरांक तार्किक exists = defExists(m_lastSym);
                जर (exists) {
                    स्थिरांक std::string मूल्य = def_मूल्य(m_lastSym);
                    जर (VString::removeWhitespace(मूल्य) == "0") {
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
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, exists});
                जा next_tok;
            } अन्यथा जर (tok == VP_WHITE) {
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::BRA});
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_अभिव्यक्ती_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                जर (--m_अभिव्यक्ती_Paren_Level > 0) {
                    m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::KET});
                    जा next_tok;
                } अन्यथा {
                    // Done with parsing expression
                    तार्किक enable = m_अभिव्यक्ती_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    जर (state() == ps_अभिव्यक्ती_IFDEF || state() == ps_अभिव्यक्ती_IFNDEF) {
                        जर (state() == ps_अभिव्यक्ती_IFNDEF) enable = !enable;
                        m_ifdef_चळत.emplace(VPreIfEntry{enable, असत्य});
                        जर (!enable) parsingOff();
                        state_काढा();
                        जा next_tok;
                    } अन्यथा जर (state() == ps_अभिव्यक्ती_ELSIF) {
                        जर (m_ifdef_चळत.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } अन्यथा {
                            // Handle `else portion
                            स्थिरांक VPreIfEntry lastIf = m_ifdef_चळत.शीर्ष();
                            m_ifdef_चळत.काढा();
                            जर (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_चळत.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            जर (!enable) parsingOff();
                        }
                        state_काढा();
                    }
                    जा next_tok;
                }
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::LNOT});
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "&&", 2)) {
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::LAND});
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "||", 2)) {
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::LOR});
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "->", 2)) {
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::IMP});
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 3 && 0 == म_भेदन(yyourtext(), "<->", 3)) {
                m_अभिव्यक्ती_Parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::EQV});
                जा next_tok;
            } अन्यथा {
                जर (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    परत tok;
                } अन्यथा {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                जा next_tok;
            }
        }
        स्थिती ps_DEFFORM: {
            जर (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_मूल्य;
                जर (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ढकला_State_Def_मूल्य();
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT) {
                // IE, something like comment in formals
                जर (!m_off) परत tok;
                जा next_tok;
            } अन्यथा {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                जा next_tok;
            }
        }
        स्थिती ps_DEFVALUE: {
            स्थिर string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            जर (tok == VP_DEFVALUE) {
                जर (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_मूल्य)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                स्थिरांक string formals = m_formals;
                string मूल्य = m_lexp->m_def_मूल्य;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                साठी (माप_प्रकार i = 0; i < formals.length(); i++) {
                    जर (formals[i] == '\n') s_newlines += "\n";
                }
                साठी (माप_प्रकार i = 0; i < मूल्य.length(); i++) {
                    जर (मूल्य[i] == '\n') s_newlines += "\n";
                }
                जर (!m_off) {
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
            state_काढा();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            परत VP_WHITE;
        }
        स्थिती ps_DEFPAREN: {
            जर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                जा next_tok;
            } अन्यथा {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                स्थिरांक VDefineRef* स्थिरांक refp = &(m_defRefs.शीर्ष());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_काढा();
                जा next_tok;
            }
        }
        स्थिती ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.शीर्ष());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य);
            m_lexp->m_def_मूल्य = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            जर (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().ढकला_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ढकला_State_Def_Arg(1);
                refp->nextarg("");
                जा next_tok;
            } अन्यथा जर (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().ढकला_back(refp->nextarg());
                string out;
                जर (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.काढा(), refp);
                state_काढा();
                जर (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    जर (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_चळत.empty(), "`` join stack empty, but in a ``");
                        स्थिरांक string lhs = m_join_चळत.शीर्ष();
                        m_join_चळत.काढा();
                        out.घाला(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_काढा();
                    }
                    जर (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    अन्यथा जर (stateIsDefname()) {
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
                जा next_tok;
            } अन्यथा जर (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                थांबा;
            } अन्यथा जर (tok == VP_चिन्ह || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                जा next_tok;
            } अन्यथा जर (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ढकला(ps_STRIFY);
                जा next_tok;
            } अन्यथा {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_काढा();
                जा next_tok;
            }
        }
        स्थिती ps_INCNAME: {
            जर (tok == VP_STRING) {
                state_काढा();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                जा next_tok;
            } अन्यथा जर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ढकला_State_Inc_Filename();
                जा next_tok;
            } अन्यथा जर (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                थांबा;
            } अन्यथा {
                state_काढा();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                जा next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        स्थिती ps_ERRORNAME: {
            जर (tok == VP_STRING) {
                जर (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_काढा();
                जा next_tok;
            } अन्यथा {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_काढा();
                जा next_tok;
            }
        }
        स्थिती ps_JOIN: {
            जर (tok == VP_चिन्ह || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_चळत.empty(), "`` join stack empty, but in a ``");
                स्थिरांक string lhs = m_join_चळत.शीर्ष();
                m_join_चळत.काढा();
                UINFO(5, "`` LHS:" << lhs);
                स्थिरांक string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                स्थिरांक string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_काढा();
                जा next_tok;
            } अन्यथा जर (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                स्थिरांक string lhs = m_join_चळत.शीर्ष();
                m_join_चळत.काढा();
                unputString(lhs);
                state_काढा();
                जा next_tok;
            } अन्यथा {
                // `define, etc, fall through and expand.  Pop back here.
                थांबा;
            }
        }
        स्थिती ps_STRIFY: {
            जर (tok == VP_STRIFY) {
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
                state_काढा();
                जा next_tok;
            } अन्यथा जर (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                थांबा;
            } अन्यथा जर (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                जा next_tok;
            } अन्यथा जर (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                थांबा;
            } अन्यथा {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                जा next_tok;
            }
        }
        पूर्वनिर्धारित: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        निवड (tok) {
        स्थिती VP_INCLUDE:
            जर (!m_off) {
                state_ढकला(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            जा next_tok;
        स्थिती VP_UNDEF: state_ढकला(ps_DEFNAME_UNDEF); जा next_tok;
        स्थिती VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ढकला(ps_DEFNAME_DEFINE);
            जा next_tok;
        स्थिती VP_IFDEF: state_ढकला(ps_DEFNAME_IFDEF); जा next_tok;
        स्थिती VP_IFNDEF: state_ढकला(ps_DEFNAME_IFNDEF); जा next_tok;
        स्थिती VP_ELSIF: state_ढकला(ps_DEFNAME_ELSIF); जा next_tok;
        स्थिती VP_ELSE:
            जर (m_ifdef_चळत.empty()) {
                fileline()->v3error("`else with no matching `if");
            } अन्यथा {
                स्थिरांक VPreIfEntry lastIf = m_ifdef_चळत.शीर्ष();
                m_ifdef_चळत.काढा();
                स्थिरांक तार्किक enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_चळत.emplace(VPreIfEntry{enable, lastIf.everOn()});
                जर (!lastIf.on()) parsingOn();
                जर (!enable) parsingOff();
            }
            जा next_tok;
        स्थिती VP_ENDIF:
            UINFO(4, "Endif ");
            जर (m_ifdef_चळत.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } अन्यथा {
                स्थिरांक VPreIfEntry lastIf = m_ifdef_चळत.शीर्ष();
                m_ifdef_चळत.काढा();
                जर (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            जा next_tok;

        स्थिती VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            स्थिरांक string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            जर (m_defPutJoin) {
                m_defPutJoin = असत्य;
                unputString("``");
            }
            जर (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                जा next_tok;
            }
            // Substitute
            जर (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                जर (m_off) {
                    जा next_tok;
                } अन्यथा {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    जा next_tok;
                }
            } अन्यथा {
                स्थिरांक string params = defParams(name);
                जर (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    जर (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    जर (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        जर (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_चळत.empty(), "`` join stack empty, but in a ``");
                            स्थिरांक string lhs = m_join_चळत.शीर्ष();
                            m_join_चळत.काढा();
                            out.घाला(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_काढा();
                        }
                        जर (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        अन्यथा जर (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } अन्यथा {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* स्थिरांक refp = &(m_defRefs.शीर्ष());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य + out);
                        m_lexp->m_def_मूल्य = "";
                    }
                    जा next_tok;
                } अन्यथा {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    जर (!m_defRefs.empty()) m_defRefs.शीर्ष().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ढकला(ps_DEFPAREN);
                    m_lexp->ढकला_State_Def_Arg(0);
                    जा next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            जा next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        स्थिती VP_ERROR: {
            state_ढकला(ps_ERRORNAME);
            जा next_tok;
        }
        स्थिती VP_EOF:
            जर (!m_ifdef_चळत.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            परत tok;
        स्थिती VP_UNDEFINEALL:
            जर (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            जा next_tok;
        स्थिती VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ढकला(ps_STRIFY);
            जा next_tok;
        स्थिती VP_चिन्ह:
        स्थिती VP_STRING:
        स्थिती VP_TEXT: {
            m_defDepth = 0;
            जर (!m_off) {
                परत tok;
            } अन्यथा {
                जा next_tok;
            }
        }
        स्थिती VP_WHITE:  // Handled at top of loop
        स्थिती VP_COMMENT:  // Handled at top of loop
        स्थिती VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        स्थिती VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        पूर्वनिर्धारित:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            थांबा;  // LCOV_EXCL_LINE
        }
        परत tok;
    }
}

पूर्णांक V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    जर (!m_finAhead) {
        m_finAhead = सत्य;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    स्थिरांक पूर्णांक tok = m_finToken;
    buf = m_finBuf;
    जर (असत्य && debug() >= 5) {
        स्थिरांक string bufcln = V3PreLex::cleanDbgStrg(buf);
        स्थिरांक string flcol = m_lexp->m_tokFilelinep->ascii_ओळ_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    स्थिरांक अक्षर* bufp = buf.c_str();
    पर्यंत (*bufp == '\n') bufp++;
    जर ((tok == VP_TEXT || tok == VP_ओळ) && VString::startsWith(bufp, "`line ")) {
        पूर्णांक enter;
        m_finFilelinep->ओळ_Directive(bufp, enter /*ref*/);
    } अन्यथा {
        जर (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->ओळ_Directives()) {
            जर (स्थिरांक पूर्णांक outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                जर (debug() >= 5) {
                    स्थिरांक string flcol = m_lexp->m_tokFilelinep->ascii_ओळ_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                जर (outBehind > 0
                    && (outBehind <= static_cast<पूर्णांक>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    जर (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        परत VP_TEXT;
                    }
                } अन्यथा {
                    // Need to backup, use `line
                    buf = m_finFilelinep->ओळ_Directive_Strg(0);
                    परत VP_ओळ;
                }
            }
        }
        // Track newlines in prep for next token
        साठी (स्थिरांक अक्षर& c : buf) {
            जर (c == '\n') {
                m_finAtBol = सत्य;
                m_finFilelinep->linenoInc();
            } अन्यथा {
                m_finAtBol = असत्य;
            }
        }
    }
    m_finAhead = असत्य;  // Consumed the token
    परत tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    जर (isEof()) परत "";
    स्थिरांक अक्षर* rtnp;
    तार्किक gotEof = असत्य;
    पर्यंत (nullptr == (rtnp = std::म_अक्षर(m_ओळ_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        स्थिरांक पूर्णांक tok = getFinalToken(m_tokenBuf /*ref*/);
        जर (debug() >= 5) {
            स्थिरांक string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            स्थिरांक string flcol = m_lexp->m_tokFilelinep->ascii_ओळ_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        जर (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            जर (m_ओळ_Chars != "" && m_ओळ_Chars[m_ओळ_Chars.length() - 1] != '\n') {
                m_ओळ_Chars.append("\n");
            }
            gotEof = सत्य;
        } अन्यथा {
            m_ओळ_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    स्थिरांक पूर्णांक len = rtnp - m_ओळ_Chars.c_str() + 1;
    string the_ओळ(m_ओळ_Chars, 0, len);
    m_ओळ_Chars = m_ओळ_Chars.erase(0, len);  // Remove returned characters
    जर (debug() >= 4) {
        स्थिरांक string lncln = V3PreLex::cleanDbgStrg(the_ओळ);
        स्थिरांक string flcol = m_lexp->m_tokFilelinep->ascii_ओळ_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    परत the_ओळ;
}
