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
    File_पङ्क्ति* अचर m_fileline;  // Where it was declared
    अचर string m_मान;  // Value of define
    अचर string m_params;  // Parameters
    अचर तार्किक m_cmdline;  // Set on command line, don't `undefineall
सार्वजनिक:
    VDefine(File_पङ्क्ति* fl, अचर string& मान, अचर string& params, तार्किक cmdline)
        : m_fileline{fl}
        , m_मान{मान}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_पङ्क्ति* fileline() अचर { फर्काऊ m_fileline; }
    string मान() अचर { फर्काऊ m_मान; }
    string params() अचर { फर्काऊ m_params; }
    तार्किक cmdline() अचर { फर्काऊ m_cmdline; }
};

//*************************************************************************

वर्ग VDefineRef final {
    // One for each pending define substitution
    अचर string m_name;  // Define last name being defined
    अचर string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    पूर्णाङ्क m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
सार्वजनिक:
    string name() अचर { फर्काऊ m_name; }
    string params() अचर { फर्काऊ m_params; }
    string nextarg() अचर { फर्काऊ m_nextarg; }
    शून्य nextarg(अचर string& मान) { m_nextarg = मान; }
    पूर्णाङ्क parenLevel() अचर { फर्काऊ m_parenLevel; }
    शून्य parenLevel(पूर्णाङ्क मान) { m_parenLevel = मान; }
    std::vector<string>& args() { फर्काऊ m_args; }
    VDefineRef(अचर string& name, अचर string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = स्वभाव;
};

//*************************************************************************
/// Data for parsing on/off

वर्ग VPreIfEntry final {
    // One for each pending ifdef/ifndef
    अचर तार्किक m_on;  // Current parse for this ifdef level is "on"
    अचर तार्किक m_everOn;  // Some if term in elsif tree has been on
सार्वजनिक:
    तार्किक on() अचर { फर्काऊ m_on; }
    तार्किक everOn() अचर { फर्काऊ m_everOn; }
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
    using DefinesMap = std::map<अचर std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    अचर V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    पूर्णाङ्क m_lastLineno = 0;  // Last line number (stall detection)
    पूर्णाङ्क m_tokens_On_पङ्क्ति = 0;  // Number of tokens on line (stall detection)

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
        ps_अभिव्यक्ति_IFDEF,
        ps_अभिव्यक्ति_IFNDEF,
        ps_अभिव्यक्ति_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    स्थिर अचर वर्ण* procStateName(ProcState s) {
        स्थिर अचर वर्ण* अचर states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        फर्काऊ states[s];
    }

    std::थाक<ProcState> m_states;  ///< Current state of parser
    पूर्णाङ्क m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    तार्किक m_incError = असत्य;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_पङ्क्ति_Cmt;  ///< Line comment(s) to be returned
    तार्किक m_पङ्क्ति_Cmt_Nl = असत्य;  ///< Newline needed before inserting lineCmt
    पूर्णाङ्क m_पङ्क्ति_Add = 0;  ///< Empty lines to return to maintain line count
    तार्किक m_rawAtBol = सत्य;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    तार्किक m_finAhead = असत्य;  ///< Have read a token ahead
    पूर्णाङ्क m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    तार्किक m_finAtBol = सत्य;  ///< Last getFinalToken left us at beginning of line
    File_पङ्क्ति* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::थाक<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::थाक<VPreIfEntry> m_ifdef_थाक;  ///< Stack of true/false emitting evaluations
    अचिन्हित m_defDepth = 0;  ///< How many `defines deep
    तार्किक m_defPutJoin = असत्य;  ///< Insert `` after substitution

    // For `` join
    std::थाक<string> m_join_थाक;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_अभिव्यक्ति m_अभिव्यक्ति_Parser;  ///< Parser for () expression
    पूर्णाङ्क m_अभिव्यक्ति_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_पङ्क्ति_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    शून्य v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    स्थिर अचर वर्ण* tokenName(पूर्णाङ्क tok);
    शून्य debugToken(पूर्णाङ्क tok, अचर वर्ण* cmtp);
    शून्य parse_शीर्ष();
    शून्य parseUndef();

गुप्त:
    // Internal methods
    शून्य endOfOneFile();
    string defineSubst(VDefineRef* refp);

    तार्किक defExists(अचर string& name);
    तार्किक defCmdline(अचर string& name);
    string def_मान(अचर string& name);
    string defParams(अचर string& name);
    File_पङ्क्ति* defFileline(अचर string& name);

    string commentCleanup(अचर string& text);
    तार्किक commentTokenMatch(string& cmdr, अचर वर्ण* strg);
    स्थिर string trimWhitespace(अचर string& strg, तार्किक trailing);
    शून्य unputString(अचर string& strg);
    शून्य unputDefrefString(अचर string& strg);

    शून्य parsingOn() {
        m_off--;
        यदि (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    शून्य parsingOff() { m_off++; }

    पूर्णाङ्क getRawToken();
    पूर्णाङ्क getStateToken();
    पूर्णाङ्क getFinalToken(string& buf);

    ProcState state() अचर { फर्काऊ m_states.शीर्ष(); }
    तार्किक stateIsDefname() अचर {
        फर्काऊ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    शून्य state_थप(ProcState state) { m_states.थप(state); }
    शून्य state_झिक() {
        m_states.झिक();
        यदि (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.थप(ps_शीर्ष);  // LCOV_EXCL_LINE
        }
    }
    शून्य stateChange(ProcState state) {
        state_झिक();
        state_थप(state);
    }

सार्वजनिक:
    // METHODS, called from upper level shell
    शून्य openFile(File_पङ्क्ति* fl, VInFilter* filterp, अचर string& filename) override;
    तार्किक isEof() अचर override { फर्काऊ m_lexp->curStreamp()->m_eof; }
    शून्य forceEof() { m_lexp->curStreamp()->m_eof = सत्य; }
    string getline() override;
    शून्य घुसाउ_Unreadback(अचर string& text) override { m_पङ्क्ति_Cmt += text; }
    शून्य घुसाउ_Unreadback_At_Bol(अचर string& text);
    शून्य add_पङ्क्ति_Comment(पूर्णाङ्क enterExit);
    शून्य dumpDefines(std::ostream& os) override;
    शून्य candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    शून्य comment(अचर string& text) override;  // Comment detected (if keepComments==2)
    शून्य include(अचर string& filename) override;  // Request a include file be processed
    शून्य undef(अचर string& name) override;
    आभास शून्य undefineall();
    शून्य define(File_पङ्क्ति* fl, अचर string& name, अचर string& मान, अचर string& params,
                तार्किक cmdline) override;
    string removeDefines(अचर string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.थप(ps_शीर्ष); }
    // cppcheck-suppress duplInheritedMember
    शून्य configure(File_पङ्क्ति* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = एतत्;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = नूतन File_पङ्क्ति{filelinep->filename()};
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

V3PreProc* V3PreProc::createPreProc(File_पङ्क्ति* fl) {
    V3PreProcImp* अचर preprocp = नूतन V3PreProcImp;
    preprocp->configure(fl);
    फर्काऊ preprocp;
}

शून्य V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_अभिव्यक्ति::selfTest(); }

//*************************************************************************
// Defines

शून्य V3PreProcImp::undef(अचर string& name) {
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        घुसाउ_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
शून्य V3PreProcImp::undefineall() {
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        घुसाउ_Unreadback("`undefineall\n");
    लागि (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        यदि (!it->second.cmdline()) m_defines.erase(it);
    }
}
तार्किक V3PreProcImp::defExists(अचर string& name) {
    अचर स्वतः iter = m_defines.find(name);
    फर्काऊ (iter != m_defines.end());
}
तार्किक V3PreProcImp::defCmdline(अचर string& name) {
    अचर स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) { फर्काऊ असत्य; }
    फर्काऊ iter->second.cmdline();
}
string V3PreProcImp::def_मान(अचर string& name) {
    अचर स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        फर्काऊ "";
    }
    फर्काऊ iter->second.मान();
}
string V3PreProcImp::defParams(अचर string& name) {
    अचर स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        फर्काऊ "";
    }
    फर्काऊ iter->second.params();
}
File_पङ्क्ति* V3PreProcImp::defFileline(अचर string& name) {
    अचर स्वतः iter = m_defines.find(name);
    यदि (iter == m_defines.end()) फर्काऊ nullptr;
    फर्काऊ iter->second.fileline();
}
शून्य V3PreProcImp::define(File_पङ्क्ति* fl, अचर string& name, अचर string& मान,
                          अचर string& params, तार्किक cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << मान << "' params '" << params << "'");
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(मान.size());
        लागि (string::size_type pos = 0; pos < मान.size(); ++pos) {
            यदि (मान[pos] == '\n' && (pos == 0 || मान[pos - 1] != '\\')) out += '\\';
            out += मान[pos];
        }
        घुसाउ_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    यदि (!V3_Language_शब्दहरू::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } अन्यथा {
        यदि (defExists(name)) {
            यदि (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << मान
                               << "' to existing command line define value: '" << def_मान(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_मान(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                फर्काऊ;
            } अन्यथा {
                यदि (!(def_मान(name) == मान
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << मान
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_मान(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, मान, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(अचर string& text) {
    string val;
    string rtnsym = text;
    लागि (पूर्णाङ्क loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        यदि (xsym[0] == '`') xsym.erase(0, 1);
        यदि (defExists(xsym)) {
            val = def_मान(xsym);
            यदि (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } अन्यथा {
                रोक;
            }
        } अन्यथा {
            रोक;
        }
    }
    फर्काऊ rtnsym;  // NA
}

//**********************************************************************
// Comments

शून्य V3PreProcImp::include(अचर string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(अचर string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    जबसम्म ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    जबसम्म ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    जबसम्म ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    जबसम्म ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    जबसम्म ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    जबसम्म ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    जबसम्म (!cmd.empty() && std::है_खाली(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    फर्काऊ cmd;
}

तार्किक V3PreProcImp::commentTokenMatch(string& cmdr, अचर वर्ण* strg) {
    पूर्णाङ्क len = std::म_माप(strg);
    यदि (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::है_खाली(cmdr[len]))) {
        यदि (std::है_खाली(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        फर्काऊ सत्य;
    } अन्यथा {
        फर्काऊ असत्य;
    }
}

शून्य V3PreProcImp::comment(अचर string& text) {
    // Comment detected.  Only keep relevant data.
    तार्किक printed = असत्य;
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        घुसाउ_Unreadback(text);
        printed = सत्य;
    }

    अचर वर्ण* cp = text.c_str();
    यदि (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } अन्यथा {
        फर्काऊ;
    }

    जबसम्म (std::है_खाली(*cp)) ++cp;

    तार्किक synth = असत्य;
    तार्किक vlcomment = असत्य;
    यदि ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::म_माप("verilator");
        यदि (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            फर्काऊ;
        }
        vlcomment = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "synopsys")) {
        cp += std::म_माप("synopsys");
        यदि (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            फर्काऊ;
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
        फर्काऊ;
    }

    जबसम्म (std::है_खाली(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    यदि (synth) {
        यदि (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            यदि (commentTokenMatch(cmd /*ref*/, "full_case")) {
                यदि (!printed) घुसाउ_Unreadback("/*verilator full_case*/");
            }
            यदि (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                यदि (!printed) घुसाउ_Unreadback("/*verilator parallel_case*/");
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
            जबसम्म (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            जबसम्म (std::है_खाली(arg[0])) arg = arg.substr(1);
            यदि (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            यदि (!printed) घुसाउ_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } अन्यथा यदि (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::म_माप("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            जबसम्म (std::है_खाली(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            यदि (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                फर्काऊ;
            }
            यदि (!printed) घुसाउ_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } अन्यथा {
            यदि (!printed) घुसाउ_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_पङ्क्ति* V3PreProc::fileline() {
    अचर V3PreProcImp* idatap = static_cast<V3PreProcImp*>(एतत्);
    फर्काऊ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

अचर वर्ण* V3PreProcImp::tokenName(पूर्णाङ्क tok) {
    विकल्प (tok) {
    पक्ष VP_BACKQUOTE: फर्काऊ ("BACKQUOTE");
    पक्ष VP_COMMENT: फर्काऊ ("COMMENT");
    पक्ष VP_DEFARG: फर्काऊ ("DEFARG");
    पक्ष VP_DEFFORM: फर्काऊ ("DEFFORM");
    पक्ष VP_DEFINE: फर्काऊ ("DEFINE");
    पक्ष VP_DEFREF: फर्काऊ ("DEFREF");
    पक्ष VP_DEFREF_JOIN: फर्काऊ ("DEFREF_JOIN");
    पक्ष VP_DEFVALUE: फर्काऊ ("DEFVALUE");
    पक्ष VP_ELSE: फर्काऊ ("ELSE");
    पक्ष VP_ELSIF: फर्काऊ ("ELSIF");
    पक्ष VP_ENDIF: फर्काऊ ("ENDIF");
    पक्ष VP_EOF: फर्काऊ ("EOF");
    पक्ष VP_EOF_ERROR: फर्काऊ ("EOF_ERROR");
    पक्ष VP_ERROR: फर्काऊ ("ERROR");
    पक्ष VP_IFDEF: फर्काऊ ("IFDEF");
    पक्ष VP_IFNDEF: फर्काऊ ("IFNDEF");
    पक्ष VP_JOIN: फर्काऊ ("JOIN");
    पक्ष VP_INCLUDE: फर्काऊ ("INCLUDE");
    पक्ष VP_पङ्क्ति: फर्काऊ ("LINE");
    पक्ष VP_STRIFY: फर्काऊ ("STRIFY");
    पक्ष VP_STRING: फर्काऊ ("STRING");
    पक्ष VP_चिह्न: फर्काऊ ("SYMBOL");
    पक्ष VP_चिह्न_JOIN: फर्काऊ ("SYMBOL_JOIN");
    पक्ष VP_TEXT: फर्काऊ ("TEXT");
    पक्ष VP_UNDEF: फर्काऊ ("UNDEF");
    पक्ष VP_UNDEFINEALL: फर्काऊ ("UNDEFINEALL");
    पक्ष VP_WHITE: फर्काऊ ("WHITE");
    स्वभाव: फर्काऊ ("?");
    }
}

शून्य V3PreProcImp::unputString(अचर string& strg) {
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

शून्य V3PreProcImp::unputDefrefString(अचर string& strg) {
    अचर पूर्णाङ्क multiline = std::गन्ती(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(अचर string& strg, तार्किक trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    जबसम्म (out.length() > leadspace && std::है_खाली(out[leadspace])) ++leadspace;
    यदि (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    यदि (trailing) {
        string::size_type trailspace = 0;
        जबसम्म (out.length() > trailspace && std::है_खाली(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        यदि (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        यदि (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    फर्काऊ out;
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
    लागि (अचिन्हित i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    अचर string मान = def_मान(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(मान) << "'");

    std::map<अचर string, string> arg_मान_By_Name;
    {  // Parse argument list into map
        अचिन्हित अङ्क_Args = 0;
        string argName;
        पूर्णाङ्क paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        तार्किक quote = असत्य;
        तार्किक haveDefault = असत्य;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        अचर string params = refp->params();  // Must keep str in scope to get pointer
        अचर वर्ण* cp = params.c_str();
        यदि (*cp == '(') cp++;
        लागि (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            यदि (!quote && paren == 1) {
                यदि (*cp == ')' || *cp == ',') {
                    string मान_Def;
                    यदि (haveDefault) {
                        मान_Def = token;
                    } अन्यथा {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, सत्य);
                    UINFO(4, "    Got Arg=" << अङ्क_Args << "  argName='" << argName
                                            << "'  default='" << मान_Def << "'");
                    // Parse it
                    यदि (argName != "") {
                        यदि (refp->args().size() > अङ्क_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            अचर string arg = trimWhitespace(refp->args()[अङ्क_Args], सत्य);
                            यदि (arg != "") मान_Def = arg;
                        } अन्यथा यदि (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            फर्काऊ " `" + refp->name() + " ";
                        }
                        अङ्क_Args++;
                    }
                    arg_मान_By_Name[argName] = मान_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = असत्य;
                    जारी;
                } अन्यथा यदि (*cp == '=') {
                    haveDefault = सत्य;
                    argName = token;
                    token = "";
                    जारी;
                }
            }
            यदि (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                जारी;
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
        यदि (refp->args().size() > अङ्क_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && अङ्क_Args == 0
                 && trimWhitespace(refp->args()[0], असत्य) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            फर्काऊ " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        तार्किक quote = असत्य;
        तार्किक triquote = असत्य;
        तार्किक backslashesc = असत्य;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        लागि (अचर वर्ण* cp = मान.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
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
                    जारी;
                }
                यदि (argName != "") {
                    // Found a possible variable substitution
                    अचर स्वतः iter = arg_मान_By_Name.find(argName);
                    यदि (iter != arg_मान_By_Name.end()) {
                        // Substitute
                        अचर string subst = iter->second;
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
                    जारी;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    जारी;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    जारी;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    जारी;
                } अन्यथा यदि (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    जारी;
                }
            }
            यदि (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                जारी;
            } अन्यथा यदि (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                जारी;
            }
            यदि (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                जारी;
            }
            यदि (*cp == '"') quote = !quote;
            यदि (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    फर्काऊ out;
}

//**********************************************************************
// Parser routines

शून्य V3PreProcImp::openFile(File_पङ्क्ति*, VInFilter* filterp, अचर string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    यदि (m_incError) फर्काऊ;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    अचर तार्किक ok = filterp->readWholefile(filename, wholefile /*ref*/);
    यदि (!ok) {
        fileline()->v3error("File not found: " + filename);
        फर्काऊ;
    }

    यदि (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        यदि (m_lexp->m_streamp_थाक.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = सत्य;
            फर्काऊ;
        }
        // There's already a file active.  Push it to work on the new one.
        add_पङ्क्ति_Comment(0);
    }

    // Save file contents for future error reporting
    File_पङ्क्ति* अचर flsp = नूतन File_पङ्क्ति{filename};
    flsp->lineno(1);
    flsp->newContent();
    लागि (अचर string& i : wholefile) flsp->contentp()->थप_Text(i);

    // Save contents for V3Control --contents
    यदि (filename != V3Options::getStdPackagePath()) {
        तार्किक containsVlt = असत्य;
        लागि (अचर string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            यदि (i.find("`verilator_config") != string::npos) {
                containsVlt = सत्य;
                रोक;
            }
        }
        यदि (!containsVlt) {
            लागि (अचर string& i : wholefile) V3Control::contents_थप_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_पङ्क्ति_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    पूर्णाङ्क eof_newline = 0;  // Number of characters following last newline
    पूर्णाङ्क eof_lineno = 1;
    लागि (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        तार्किक strip = असत्य;
        अचर वर्ण* अचर sp = it->data();
        अचर वर्ण* अचर ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        लागि (अचर वर्ण* cp = sp; cp < ep; cp++) {
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
            लागि (अचर वर्ण* cp = sp; cp < ep; cp++) {
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
        File_पङ्क्ति* अचर fl = नूतन File_पङ्क्ति{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

शून्य V3PreProcImp::घुसाउ_Unreadback_At_Bol(अचर string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    यदि (m_पङ्क्ति_Cmt == "") {
        m_पङ्क्ति_Cmt_Nl = सत्य;
    } अन्यथा यदि (m_पङ्क्ति_Cmt[m_पङ्क्ति_Cmt.length() - 1] != '\n') {
        घुसाउ_Unreadback("\n");
    }
    घुसाउ_Unreadback(text);
}

शून्य V3PreProcImp::add_पङ्क्ति_Comment(पूर्णाङ्क enterExit) {
    यदि (पङ्क्ति_Directives()) {
        घुसाउ_Unreadback_At_Bol(m_lexp->curFilelinep()->पङ्क्ति_Directive_Strg(enterExit));
    }
}

शून्य V3PreProcImp::dumpDefines(std::ostream& os) {
    लागि (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        यदि (!it->second.params().empty()) os << it->second.params();
        यदि (!it->second.मान().empty()) os << " " << it->second.मान();
        os << '\n';
    }
}

शून्य V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    लागि (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->थप_Candidate("`"s + it->first);
    }
}

पूर्णाङ्क V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    जबसम्म (सत्य) {
    next_tok:
        यदि (m_पङ्क्ति_Add) {
            m_पङ्क्ति_Add--;
            m_rawAtBol = सत्य;
            yyourtext("\n", 1);
            यदि (debug() >= 5) debugToken(VP_WHITE, "LNA");
            फर्काऊ VP_WHITE;
        }
        यदि (m_पङ्क्ति_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            स्थिर string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_पङ्क्ति_Cmt;
            यदि (m_पङ्क्ति_Cmt_Nl) {
                यदि (!m_rawAtBol) s_rtncmt.घुसाउ(0, "\n");
                m_पङ्क्ति_Cmt_Nl = असत्य;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_पङ्क्ति_Cmt = "";
            यदि (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            यदि (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_मान(yyourtext(), yyourleng());
                गच्छ next_tok;
            } अन्यथा {
                यदि (debug() >= 5) debugToken(VP_TEXT, "LCM");
                फर्काऊ VP_TEXT;
            }
        }
        यदि (isEof()) फर्काऊ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        पूर्णाङ्क tok = m_lexp->lex();
        यदि (debug() >= 5) debugToken(tok, "RAW");

        यदि (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_पङ्क्ति = 0;
        } अन्यथा यदि (++m_tokens_On_पङ्क्ति > v3Global.opt.preprocTokenLimit()) {
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
        फर्काऊ tok;
    }
}

शून्य V3PreProcImp::debugToken(पूर्णाङ्क tok, अचर वर्ण* cmtp) {
    अचर स्थिर पूर्णाङ्क s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    यदि (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        जबसम्म ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        जबसम्म ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        अचर string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्ति_Col();
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
    जबसम्म (सत्य) {
    next_tok:
        यदि (isEof()) फर्काऊ VP_EOF;
        पूर्णाङ्क tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        यदि (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) फर्काऊ tok;
        यदि (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        यदि (tok == VP_COMMENT) {
            यदि (!m_off) {
                यदि (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    यदि (m_पङ्क्ति_Cmt.empty()) घुसाउ_Unreadback(" ");
                } अन्यथा यदि (m_lexp->m_keepComments) {
                    फर्काऊ tok;
                } अन्यथा {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    घुसाउ_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            लागि (माप_प्रकार len = 0; len < static_cast<माप_प्रकार>(yyourleng()); len++) {
                यदि (yyourtext()[len] == '\n') m_पङ्क्ति_Add++;
            }
            गच्छ next_tok;
        }
        यदि (tok == VP_पङ्क्ति) {
            add_पङ्क्ति_Comment(m_lexp->m_enterExit);
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
            अचर std::string name(yyourtext() + 1, yyourleng() - 1);
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
            यदि (tok == VP_चिह्न_JOIN) tok = VP_चिह्न;
        }
        यदि (tok == VP_चिह्न_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            अचर std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_थाक.थप(out);
            state_थप(ps_JOIN);
            गच्छ next_tok;
        }

        // Deal with some special parser states
        विकल्प (state()) {
        पक्ष ps_शीर्ष: {
            रोक;
        }
        पक्ष ps_DEFNAME_UNDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_DEFINE:  // FALLTHRU
        पक्ष ps_DEFNAME_IFDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_IFNDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_ELSIF: {
            यदि (tok == VP_चिह्न) {
                m_lastSym.assign(yyourtext(), yyourleng());
                यदि (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    तार्किक enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    यदि (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_थाक.emplace(VPreIfEntry{enable, असत्य});
                    यदि (!enable) parsingOff();
                    state_झिक();
                    गच्छ next_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_ELSIF) {
                    यदि (m_ifdef_थाक.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } अन्यथा {
                        // Handle `else portion
                        अचर VPreIfEntry lastIf = m_ifdef_थाक.शीर्ष();
                        m_ifdef_थाक.झिक();
                        यदि (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        अचर तार्किक enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_थाक.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        यदि (!enable) parsingOff();
                    }
                    state_झिक();
                    गच्छ next_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_UNDEF) {
                    यदि (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_झिक();
                    गच्छ next_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->थप_State_Def_Form();
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
                    m_lexp->थप_State_अभिव्यक्ति();
                    m_अभिव्यक्ति_Parser.reset(fileline());
                    m_अभिव्यक्ति_Paren_Level = 1;
                    विकल्प (state()) {
                    पक्ष ps_DEFNAME_IFDEF: stateChange(ps_अभिव्यक्ति_IFDEF); रोक;
                    पक्ष ps_DEFNAME_IFNDEF: stateChange(ps_अभिव्यक्ति_IFNDEF); रोक;
                    पक्ष ps_DEFNAME_ELSIF: stateChange(ps_अभिव्यक्ति_ELSIF); रोक;
                    स्वभाव: v3fatalSrc("bad case");
                    }
                    गच्छ next_tok;
                }
                यदि (!m_off) {
                    फर्काऊ tok;
                } अन्यथा {
                    गच्छ next_tok;
                }
            } अन्यथा यदि (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                रोक;
            } अन्यथा {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                गच्छ next_tok;
            }
        }
        पक्ष ps_अभिव्यक्ति_IFDEF:  // FALLTHRU
        पक्ष ps_अभिव्यक्ति_IFNDEF:  // FALLTHRU
        पक्ष ps_अभिव्यक्ति_ELSIF: {
            // `ifdef ( *here*
            File_पङ्क्ति* अचर flp = m_lexp->m_tokFilelinep;
            यदि (tok == VP_चिह्न) {
                m_lastSym.assign(yyourtext(), yyourleng());
                अचर तार्किक exists = defExists(m_lastSym);
                यदि (exists) {
                    अचर std::string मान = def_मान(m_lastSym);
                    यदि (VString::removeWhitespace(मान) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << मान
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, exists});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_WHITE) {
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::BRA});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_अभिव्यक्ति_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                यदि (--m_अभिव्यक्ति_Paren_Level > 0) {
                    m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::KET});
                    गच्छ next_tok;
                } अन्यथा {
                    // Done with parsing expression
                    तार्किक enable = m_अभिव्यक्ति_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    यदि (state() == ps_अभिव्यक्ति_IFDEF || state() == ps_अभिव्यक्ति_IFNDEF) {
                        यदि (state() == ps_अभिव्यक्ति_IFNDEF) enable = !enable;
                        m_ifdef_थाक.emplace(VPreIfEntry{enable, असत्य});
                        यदि (!enable) parsingOff();
                        state_झिक();
                        गच्छ next_tok;
                    } अन्यथा यदि (state() == ps_अभिव्यक्ति_ELSIF) {
                        यदि (m_ifdef_थाक.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } अन्यथा {
                            // Handle `else portion
                            अचर VPreIfEntry lastIf = m_ifdef_थाक.शीर्ष();
                            m_ifdef_थाक.झिक();
                            यदि (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_थाक.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            यदि (!enable) parsingOff();
                        }
                        state_झिक();
                    }
                    गच्छ next_tok;
                }
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::LNOT});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "&&", 2)) {
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::LAND});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "||", 2)) {
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::LOR});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "->", 2)) {
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::IMP});
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 3 && 0 == म_भेदन(yyourtext(), "<->", 3)) {
                m_अभिव्यक्ति_Parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::EQV});
                गच्छ next_tok;
            } अन्यथा {
                यदि (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    फर्काऊ tok;
                } अन्यथा {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                गच्छ next_tok;
            }
        }
        पक्ष ps_DEFFORM: {
            यदि (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_मान;
                यदि (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->थप_State_Def_मान();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_TEXT) {
                // IE, something like comment in formals
                यदि (!m_off) फर्काऊ tok;
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
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_मान)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                अचर string formals = m_formals;
                string मान = m_lexp->m_def_मान;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                लागि (माप_प्रकार i = 0; i < formals.length(); i++) {
                    यदि (formals[i] == '\n') s_newlines += "\n";
                }
                लागि (माप_प्रकार i = 0; i < मान.length(); i++) {
                    यदि (मान[i] == '\n') s_newlines += "\n";
                }
                यदि (!m_off) {
                    // Remove leading and trailing whitespace
                    मान = trimWhitespace(मान, सत्य);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(मान) << "'");
                    define(fileline(), m_lastSym, मान, formals, असत्य);
                }
            } अन्यथा {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_झिक();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            फर्काऊ VP_WHITE;
        }
        पक्ष ps_DEFPAREN: {
            यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                गच्छ next_tok;
            } अन्यथा {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                अचर VDefineRef* अचर refp = &(m_defRefs.शीर्ष());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_झिक();
                गच्छ next_tok;
            }
        }
        पक्ष ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.शीर्ष());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_मान);
            m_lexp->m_def_मान = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            यदि (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().थप_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->थप_State_Def_Arg(1);
                refp->nextarg("");
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().थप_back(refp->nextarg());
                string out;
                यदि (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.झिक(), refp);
                state_झिक();
                यदि (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    यदि (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_थाक.empty(), "`` join stack empty, but in a ``");
                        अचर string lhs = m_join_थाक.शीर्ष();
                        m_join_थाक.झिक();
                        out.घुसाउ(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_झिक();
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
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_मान + out);
                    m_lexp->m_def_मान = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                रोक;
            } अन्यथा यदि (tok == VP_चिह्न || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_थप(ps_STRIFY);
                गच्छ next_tok;
            } अन्यथा {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_झिक();
                गच्छ next_tok;
            }
        }
        पक्ष ps_INCNAME: {
            यदि (tok == VP_STRING) {
                state_झिक();
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
                m_lexp->थप_State_Inc_Filename();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                रोक;
            } अन्यथा {
                state_झिक();
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
                state_झिक();
                गच्छ next_tok;
            } अन्यथा {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_झिक();
                गच्छ next_tok;
            }
        }
        पक्ष ps_JOIN: {
            यदि (tok == VP_चिह्न || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_थाक.empty(), "`` join stack empty, but in a ``");
                अचर string lhs = m_join_थाक.शीर्ष();
                m_join_थाक.झिक();
                UINFO(5, "`` LHS:" << lhs);
                अचर string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                अचर string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_झिक();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                अचर string lhs = m_join_थाक.शीर्ष();
                m_join_थाक.झिक();
                unputString(lhs);
                state_झिक();
                गच्छ next_tok;
            } अन्यथा {
                // `define, etc, fall through and expand.  Pop back here.
                रोक;
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
                state_झिक();
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                रोक;
            } अन्यथा यदि (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                गच्छ next_tok;
            } अन्यथा यदि (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                रोक;
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
                state_थप(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            गच्छ next_tok;
        पक्ष VP_UNDEF: state_थप(ps_DEFNAME_UNDEF); गच्छ next_tok;
        पक्ष VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_थप(ps_DEFNAME_DEFINE);
            गच्छ next_tok;
        पक्ष VP_IFDEF: state_थप(ps_DEFNAME_IFDEF); गच्छ next_tok;
        पक्ष VP_IFNDEF: state_थप(ps_DEFNAME_IFNDEF); गच्छ next_tok;
        पक्ष VP_ELSIF: state_थप(ps_DEFNAME_ELSIF); गच्छ next_tok;
        पक्ष VP_ELSE:
            यदि (m_ifdef_थाक.empty()) {
                fileline()->v3error("`else with no matching `if");
            } अन्यथा {
                अचर VPreIfEntry lastIf = m_ifdef_थाक.शीर्ष();
                m_ifdef_थाक.झिक();
                अचर तार्किक enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_थाक.emplace(VPreIfEntry{enable, lastIf.everOn()});
                यदि (!lastIf.on()) parsingOn();
                यदि (!enable) parsingOff();
            }
            गच्छ next_tok;
        पक्ष VP_ENDIF:
            UINFO(4, "Endif ");
            यदि (m_ifdef_थाक.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } अन्यथा {
                अचर VPreIfEntry lastIf = m_ifdef_थाक.शीर्ष();
                m_ifdef_थाक.झिक();
                यदि (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            गच्छ next_tok;

        पक्ष VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            अचर string name(yyourtext() + 1, yyourleng() - 1);
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
                अचर string params = defParams(name);
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
                            UASSERT(!m_join_थाक.empty(), "`` join stack empty, but in a ``");
                            अचर string lhs = m_join_थाक.शीर्ष();
                            m_join_थाक.झिक();
                            out.घुसाउ(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_झिक();
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
                        VDefineRef* अचर refp = &(m_defRefs.शीर्ष());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_मान + out);
                        m_lexp->m_def_मान = "";
                    }
                    गच्छ next_tok;
                } अन्यथा {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    यदि (!m_defRefs.empty()) m_defRefs.शीर्ष().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_थप(ps_DEFPAREN);
                    m_lexp->थप_State_Def_Arg(0);
                    गच्छ next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            गच्छ next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        पक्ष VP_ERROR: {
            state_थप(ps_ERRORNAME);
            गच्छ next_tok;
        }
        पक्ष VP_EOF:
            यदि (!m_ifdef_थाक.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            फर्काऊ tok;
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
            state_थप(ps_STRIFY);
            गच्छ next_tok;
        पक्ष VP_चिह्न:
        पक्ष VP_STRING:
        पक्ष VP_TEXT: {
            m_defDepth = 0;
            यदि (!m_off) {
                फर्काऊ tok;
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
            रोक;  // LCOV_EXCL_LINE
        }
        फर्काऊ tok;
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
    अचर पूर्णाङ्क tok = m_finToken;
    buf = m_finBuf;
    यदि (असत्य && debug() >= 5) {
        अचर string bufcln = V3PreLex::cleanDbgStrg(buf);
        अचर string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्ति_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    अचर वर्ण* bufp = buf.c_str();
    जबसम्म (*bufp == '\n') bufp++;
    यदि ((tok == VP_TEXT || tok == VP_पङ्क्ति) && VString::startsWith(bufp, "`line ")) {
        पूर्णाङ्क enter;
        m_finFilelinep->पङ्क्ति_Directive(bufp, enter /*ref*/);
    } अन्यथा {
        यदि (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->पङ्क्ति_Directives()) {
            यदि (अचर पूर्णाङ्क outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                यदि (debug() >= 5) {
                    अचर string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्ति_Col();
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
                        फर्काऊ VP_TEXT;
                    }
                } अन्यथा {
                    // Need to backup, use `line
                    buf = m_finFilelinep->पङ्क्ति_Directive_Strg(0);
                    फर्काऊ VP_पङ्क्ति;
                }
            }
        }
        // Track newlines in prep for next token
        लागि (अचर वर्ण& c : buf) {
            यदि (c == '\n') {
                m_finAtBol = सत्य;
                m_finFilelinep->linenoInc();
            } अन्यथा {
                m_finAtBol = असत्य;
            }
        }
    }
    m_finAhead = असत्य;  // Consumed the token
    फर्काऊ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    यदि (isEof()) फर्काऊ "";
    अचर वर्ण* rtnp;
    तार्किक gotEof = असत्य;
    जबसम्म (nullptr == (rtnp = std::म_अक्षर(m_पङ्क्ति_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        अचर पूर्णाङ्क tok = getFinalToken(m_tokenBuf /*ref*/);
        यदि (debug() >= 5) {
            अचर string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            अचर string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्ति_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        यदि (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            यदि (m_पङ्क्ति_Chars != "" && m_पङ्क्ति_Chars[m_पङ्क्ति_Chars.length() - 1] != '\n') {
                m_पङ्क्ति_Chars.append("\n");
            }
            gotEof = सत्य;
        } अन्यथा {
            m_पङ्क्ति_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    अचर पूर्णाङ्क len = rtnp - m_पङ्क्ति_Chars.c_str() + 1;
    string the_पङ्क्ति(m_पङ्क्ति_Chars, 0, len);
    m_पङ्क्ति_Chars = m_पङ्क्ति_Chars.erase(0, len);  // Remove returned characters
    यदि (debug() >= 4) {
        अचर string lncln = V3PreLex::cleanDbgStrg(the_पङ्क्ति);
        अचर string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्ति_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    फर्काऊ the_पङ्क्ति;
}
