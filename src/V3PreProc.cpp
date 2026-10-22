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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3PreProc.h"

#समावेश "V3Control.h"
#समावेश "V3Error.h"
#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3LanguageWords.h"
#समावेश "V3PreExpr.h"
#समावेश "V3PreLex.h"
#समावेश "V3PreShell.h"
#समावेश "V3String.h"

#समावेश <algorithm>
#समावेश <cstdlib>
#समावेश <fstream>
#समावेश <stack>
#समावेश <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#घोषणा yyFlexLexer V3Lexer
#समावेश "V3PreLex.yy.cpp"
#अघोषित yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

श्रेणी VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_पंक्ति* स्थिर m_fileline;  // Where it was declared
    स्थिर string m_मूल्य;  // Value of define
    स्थिर string m_params;  // Parameters
    स्थिर bool m_cmdline;  // Set on command line, don't `undefineall
खुला:
    VDefine(File_पंक्ति* fl, स्थिर string& मूल्य, स्थिर string& params, bool cmdline)
        : m_fileline{fl}
        , m_मूल्य{मूल्य}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_पंक्ति* fileline() स्थिर { वापस m_fileline; }
    string मूल्य() स्थिर { वापस m_मूल्य; }
    string params() स्थिर { वापस m_params; }
    bool cmdline() स्थिर { वापस m_cmdline; }
};

//*************************************************************************

श्रेणी VDefineRef final {
    // One for each pending define substitution
    स्थिर string m_name;  // Define last name being defined
    स्थिर string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    पूर्णांक m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
खुला:
    string name() स्थिर { वापस m_name; }
    string params() स्थिर { वापस m_params; }
    string nextarg() स्थिर { वापस m_nextarg; }
    व्योम nextarg(स्थिर string& मूल्य) { m_nextarg = मूल्य; }
    पूर्णांक parenLevel() स्थिर { वापस m_parenLevel; }
    व्योम parenLevel(पूर्णांक मूल्य) { m_parenLevel = मूल्य; }
    std::vector<string>& args() { वापस m_args; }
    VDefineRef(स्थिर string& name, स्थिर string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = शेष;
};

//*************************************************************************
/// Data for parsing on/off

श्रेणी VPreIfEntry final {
    // One for each pending ifdef/ifndef
    स्थिर bool m_on;  // Current parse for this ifdef level is "on"
    स्थिर bool m_everOn;  // Some if term in elsif tree has been on
खुला:
    bool on() स्थिर { वापस m_on; }
    bool everOn() स्थिर { वापस m_everOn; }
    VPreIfEntry(bool on, bool everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = शेष;
};

//*************************************************************************
// Data for a preprocessor instantiation.

श्रेणी V3PreProcImp final : खुला V3PreProc {
खुला:
    // TYPES
    using DefinesMap = std::map<स्थिर std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    स्थिर V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    पूर्णांक m_lastLineno = 0;  // Last line number (stall detection)
    पूर्णांक m_tokens_On_पंक्ति = 0;  // Number of tokens on line (stall detection)

    क्रमागत ProcState : uint8_t {
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
    अटल स्थिर अक्षर* procStateName(ProcState s) {
        अटल स्थिर अक्षर* स्थिर states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        वापस states[s];
    }

    std::ढेर<ProcState> m_states;  ///< Current state of parser
    पूर्णांक m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    bool m_incError = false;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_पंक्ति_Cmt;  ///< Line comment(s) to be returned
    bool m_पंक्ति_Cmt_Nl = false;  ///< Newline needed before inserting lineCmt
    पूर्णांक m_पंक्ति_Add = 0;  ///< Empty lines to return to maintain line count
    bool m_rawAtBol = true;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    bool m_finAhead = false;  ///< Have read a token ahead
    पूर्णांक m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    bool m_finAtBol = true;  ///< Last getFinalToken left us at beginning of line
    File_पंक्ति* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ढेर<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ढेर<VPreIfEntry> m_ifdef_ढेर;  ///< Stack of true/false emitting evaluations
    अचिन्हित m_defDepth = 0;  ///< How many `defines deep
    bool m_defPutJoin = false;  ///< Insert `` after substitution

    // For `` join
    std::ढेर<string> m_join_ढेर;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_व्यंजक m_व्यंजक_Parser;  ///< Parser for () expression
    पूर्णांक m_व्यंजक_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_पंक्ति_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    व्योम v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    अटल स्थिर अक्षर* tokenName(पूर्णांक tok);
    व्योम debugToken(पूर्णांक tok, स्थिर अक्षर* cmtp);
    व्योम parse_शीर्ष();
    व्योम parseUndef();

निजी:
    // Internal methods
    व्योम endOfOneFile();
    string defineSubst(VDefineRef* refp);

    bool defExists(स्थिर string& name);
    bool defCmdline(स्थिर string& name);
    string def_मूल्य(स्थिर string& name);
    string defParams(स्थिर string& name);
    File_पंक्ति* defFileline(स्थिर string& name);

    string commentCleanup(स्थिर string& text);
    bool commentTokenMatch(string& cmdr, स्थिर अक्षर* strg);
    अटल string trimWhitespace(स्थिर string& strg, bool trailing);
    व्योम unputString(स्थिर string& strg);
    व्योम unputDefrefString(स्थिर string& strg);

    व्योम parsingOn() {
        m_off--;
        अगर (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    व्योम parsingOff() { m_off++; }

    पूर्णांक getRawToken();
    पूर्णांक getStateToken();
    पूर्णांक getFinalToken(string& buf);

    ProcState state() स्थिर { वापस m_states.शीर्ष(); }
    bool stateIsDefname() स्थिर {
        वापस state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    व्योम state_धकेलो(ProcState state) { m_states.धकेलो(state); }
    व्योम state_निकालो() {
        m_states.निकालो();
        अगर (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.धकेलो(ps_शीर्ष);  // LCOV_EXCL_LINE
        }
    }
    व्योम stateChange(ProcState state) {
        state_निकालो();
        state_धकेलो(state);
    }

खुला:
    // METHODS, called from upper level shell
    व्योम openFile(File_पंक्ति* fl, VInFilter* filterp, स्थिर string& filename) override;
    bool isEof() स्थिर override { वापस m_lexp->curStreamp()->m_eof; }
    व्योम forceEof() { m_lexp->curStreamp()->m_eof = true; }
    string लो_पंक्ति() override;
    व्योम जोड़ो_Unreadback(स्थिर string& text) override { m_पंक्ति_Cmt += text; }
    व्योम जोड़ो_Unreadback_At_Bol(स्थिर string& text);
    व्योम add_पंक्ति_Comment(पूर्णांक enterExit);
    व्योम dumpDefines(std::ब_प्रवाह& os) override;
    व्योम candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    व्योम comment(स्थिर string& text) override;  // Comment detected (if keepComments==2)
    व्योम include(स्थिर string& filename) override;  // Request a include file be processed
    व्योम undef(स्थिर string& name) override;
    भव व्योम undefineall();
    व्योम define(File_पंक्ति* fl, स्थिर string& name, स्थिर string& मूल्य, स्थिर string& params,
                bool cmdline) override;
    string removeDefines(स्थिर string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.धकेलो(ps_शीर्ष); }
    // cppcheck-suppress duplInheritedMember
    व्योम configure(File_पंक्ति* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = यह;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = नया File_पंक्ति{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = नया V3PreLex{यह, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        अगर (m_lexp) VL_DO_CLEAR(मिटाओ m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_पंक्ति* fl) {
    V3PreProcImp* स्थिर preprocp = नया V3PreProcImp;
    preprocp->configure(fl);
    वापस preprocp;
}

व्योम V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_व्यंजक::selfTest(); }

//*************************************************************************
// Defines

व्योम V3PreProcImp::undef(स्थिर string& name) {
    अगर (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        जोड़ो_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
व्योम V3PreProcImp::undefineall() {
    अगर (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        जोड़ो_Unreadback("`undefineall\n");
    क्रम (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        अगर (!it->second.cmdline()) m_defines.erase(it);
    }
}
bool V3PreProcImp::defExists(स्थिर string& name) {
    स्थिर स्वतः iter = m_defines.find(name);
    वापस (iter != m_defines.end());
}
bool V3PreProcImp::defCmdline(स्थिर string& name) {
    स्थिर स्वतः iter = m_defines.find(name);
    अगर (iter == m_defines.end()) { वापस false; }
    वापस iter->second.cmdline();
}
string V3PreProcImp::def_मूल्य(स्थिर string& name) {
    स्थिर स्वतः iter = m_defines.find(name);
    अगर (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        वापस "";
    }
    वापस iter->second.मूल्य();
}
string V3PreProcImp::defParams(स्थिर string& name) {
    स्थिर स्वतः iter = m_defines.find(name);
    अगर (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        वापस "";
    }
    वापस iter->second.params();
}
File_पंक्ति* V3PreProcImp::defFileline(स्थिर string& name) {
    स्थिर स्वतः iter = m_defines.find(name);
    अगर (iter == m_defines.end()) वापस nullptr;
    वापस iter->second.fileline();
}
व्योम V3PreProcImp::define(File_पंक्ति* fl, स्थिर string& name, स्थिर string& मूल्य,
                          स्थिर string& params, bool cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << मूल्य << "' params '" << params << "'");
    अगर (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(मूल्य.आकार());
        क्रम (string::आकार_type pos = 0; pos < मूल्य.आकार(); ++pos) {
            अगर (मूल्य[pos] == '\n' && (pos == 0 || मूल्य[pos - 1] != '\\')) out += '\\';
            out += मूल्य[pos];
        }
        जोड़ो_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    अगर (!V3_Language_शब्दगण::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } अन्यथा {
        अगर (defExists(name)) {
            अगर (defCmdline(name) && !cmdline) {
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
                वापस;
            } अन्यथा {
                अगर (!(def_मूल्य(name) == मूल्य
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

string V3PreProcImp::removeDefines(स्थिर string& text) {
    string val;
    string rtnsym = text;
    क्रम (पूर्णांक loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        अगर (xsym[0] == '`') xsym.erase(0, 1);
        अगर (defExists(xsym)) {
            val = def_मूल्य(xsym);
            अगर (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } अन्यथा {
                अवरोध;
            }
        } अन्यथा {
            अवरोध;
        }
    }
    वापस rtnsym;  // NA
}

//**********************************************************************
// Comments

व्योम V3PreProcImp::include(स्थिर string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(स्थिर string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::आकार_type pos;
    जबतक ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    जबतक ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    जबतक ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    जबतक ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    जबतक ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    जबतक ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    जबतक (!cmd.empty() && std::है_खाली(cmd[cmd.आकार() - 1])) cmd.erase(cmd.आकार() - 1);
    वापस cmd;
}

bool V3PreProcImp::commentTokenMatch(string& cmdr, स्थिर अक्षर* strg) {
    पूर्णांक len = std::म_माप(strg);
    अगर (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::है_खाली(cmdr[len]))) {
        अगर (std::है_खाली(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        वापस true;
    } अन्यथा {
        वापस false;
    }
}

व्योम V3PreProcImp::comment(स्थिर string& text) {
    // Comment detected.  Only keep relevant data.
    bool printed = false;
    अगर (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        जोड़ो_Unreadback(text);
        printed = true;
    }

    स्थिर अक्षर* cp = text.c_str();
    अगर (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } अन्यथा {
        वापस;
    }

    जबतक (std::है_खाली(*cp)) ++cp;

    bool synth = false;
    bool vlcomment = false;
    अगर ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::म_माप("verilator");
        अगर (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            वापस;
        }
        vlcomment = true;
    } अन्यथा अगर (VString::startsWith(cp, "synopsys")) {
        cp += std::म_माप("synopsys");
        अगर (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            वापस;
        }
        synth = true;
    } अन्यथा अगर (VString::startsWith(cp, "cadence")) {
        cp += std::म_माप("cadence");
        synth = true;
    } अन्यथा अगर (VString::startsWith(cp, "pragma")) {
        cp += std::म_माप("pragma");
        synth = true;
    } अन्यथा अगर (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::म_माप("ambit synthesis");
        synth = true;
    } अन्यथा {
        वापस;
    }

    जबतक (std::है_खाली(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    अगर (synth) {
        अगर (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            अगर (commentTokenMatch(cmd /*ref*/, "full_case")) {
                अगर (!printed) जोड़ो_Unreadback("/*verilator full_case*/");
            }
            अगर (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                अगर (!printed) जोड़ो_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } अन्यथा अगर (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        अगर (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::आकार_type endOfCmd = std::म_माप("public_flat_rw");
            जबतक (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            जबतक (std::है_खाली(arg[0])) arg = arg.substr(1);
            अगर (arg.आकार() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            अगर (!printed) जोड़ो_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } अन्यथा अगर (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::म_माप("dpi_c_decl"));
            string::आकार_type startOfArg = baseCmd.आकार();
            जबतक (std::है_खाली(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            अगर (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                वापस;
            }
            अगर (!printed) जोड़ो_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } अन्यथा {
            अगर (!printed) जोड़ो_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_पंक्ति* V3PreProc::fileline() {
    स्थिर V3PreProcImp* idatap = static_cast<V3PreProcImp*>(यह);
    वापस idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

स्थिर अक्षर* V3PreProcImp::tokenName(पूर्णांक tok) {
    चयन (tok) {
    हाल VP_BACKQUOTE: वापस ("BACKQUOTE");
    हाल VP_COMMENT: वापस ("COMMENT");
    हाल VP_DEFARG: वापस ("DEFARG");
    हाल VP_DEFFORM: वापस ("DEFFORM");
    हाल VP_DEFINE: वापस ("DEFINE");
    हाल VP_DEFREF: वापस ("DEFREF");
    हाल VP_DEFREF_JOIN: वापस ("DEFREF_JOIN");
    हाल VP_DEFVALUE: वापस ("DEFVALUE");
    हाल VP_ELSE: वापस ("ELSE");
    हाल VP_ELSIF: वापस ("ELSIF");
    हाल VP_ENDIF: वापस ("ENDIF");
    हाल VP_EOF: वापस ("EOF");
    हाल VP_EOF_ERROR: वापस ("EOF_ERROR");
    हाल VP_ERROR: वापस ("ERROR");
    हाल VP_IFDEF: वापस ("IFDEF");
    हाल VP_IFNDEF: वापस ("IFNDEF");
    हाल VP_JOIN: वापस ("JOIN");
    हाल VP_INCLUDE: वापस ("INCLUDE");
    हाल VP_पंक्ति: वापस ("LINE");
    हाल VP_STRIFY: वापस ("STRIFY");
    हाल VP_STRING: वापस ("STRING");
    हाल VP_प्रतीक: वापस ("SYMBOL");
    हाल VP_प्रतीक_JOIN: वापस ("SYMBOL_JOIN");
    हाल VP_TEXT: वापस ("TEXT");
    हाल VP_UNDEF: वापस ("UNDEF");
    हाल VP_UNDEFINEALL: वापस ("UNDEFINEALL");
    हाल VP_WHITE: वापस ("WHITE");
    शेष: वापस ("?");
    }
}

व्योम V3PreProcImp::unputString(स्थिर string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    अगर (VL_UNCOVERABLE(m_lexp->m_बफर_State != m_lexp->current_बफर())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

व्योम V3PreProcImp::unputDefrefString(स्थिर string& strg) {
    स्थिर पूर्णांक multiline = std::गिनती(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(स्थिर string& strg, bool trailing) {
    // Remove leading whitespace
    string out = strg;
    string::आकार_type leadspace = 0;
    जबतक (out.लंबाई() > leadspace && std::है_खाली(out[leadspace])) ++leadspace;
    अगर (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    अगर (trailing) {
        string::आकार_type trailspace = 0;
        जबतक (out.लंबाई() > trailspace && std::है_खाली(out[out.लंबाई() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        अगर (trailspace && out.लंबाई() > trailspace && out[out.लंबाई() - 1 - trailspace] == '\\')
            --trailspace;
        अगर (trailspace) out.erase(out.लंबाई() - trailspace, trailspace);
    }
    वापस out;
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
    क्रम (अचिन्हित i = 0; i < refp->args().आकार(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    स्थिर string मूल्य = def_मूल्य(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(मूल्य) << "'");

    std::map<स्थिर string, string> arg_मूल्य_By_Name;
    {  // Parse argument list into map
        अचिन्हित अंक_Args = 0;
        string argName;
        पूर्णांक paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        bool quote = false;
        bool haveDefault = false;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        स्थिर string params = refp->params();  // Must keep str in scope to get pointer
        स्थिर अक्षर* cp = params.c_str();
        अगर (*cp == '(') cp++;
        क्रम (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            अगर (!quote && paren == 1) {
                अगर (*cp == ')' || *cp == ',') {
                    string मूल्य_Def;
                    अगर (haveDefault) {
                        मूल्य_Def = token;
                    } अन्यथा {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, true);
                    UINFO(4, "    Got Arg=" << अंक_Args << "  argName='" << argName
                                            << "'  default='" << मूल्य_Def << "'");
                    // Parse it
                    अगर (argName != "") {
                        अगर (refp->args().आकार() > अंक_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            स्थिर string arg = trimWhitespace(refp->args()[अंक_Args], true);
                            अगर (arg != "") मूल्य_Def = arg;
                        } अन्यथा अगर (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            वापस " `" + refp->name() + " ";
                        }
                        अंक_Args++;
                    }
                    arg_मूल्य_By_Name[argName] = मूल्य_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = false;
                    जारी;
                } अन्यथा अगर (*cp == '=') {
                    haveDefault = true;
                    argName = token;
                    token = "";
                    जारी;
                }
            }
            अगर (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                जारी;
            }
            अगर (!quote) {
                अगर (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } अन्यथा अगर (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            अगर (*cp == '"') quote = !quote;
            अगर (*cp) token += *cp;
        }
        अगर (refp->args().आकार() > अंक_Args
            // `define X() is ok to call with nothing
            && !(refp->args().आकार() == 1 && अंक_Args == 0
                 && trimWhitespace(refp->args()[0], false) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            वापस " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        bool quote = false;
        bool triquote = false;
        bool backslashesc = false;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        क्रम (स्थिर अक्षर* cp = मूल्य.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            अगर (!quote && !triquote && *cp == '\\') {
                backslashesc = true;
            } अन्यथा अगर (std::है_खाली(*cp)) {
                backslashesc = false;
            }
            अगर (!quote && !triquote) {
                अगर (std::है_अक्षर(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::है_अंक(*cp))) {
                    argName += *cp;
                    जारी;
                }
                अगर (argName != "") {
                    // Found a possible variable substitution
                    स्थिर स्वतः iter = arg_मूल्य_By_Name.find(argName);
                    अगर (iter != arg_मूल्य_By_Name.end()) {
                        // Substitute
                        स्थिर string subst = iter->second;
                        अगर (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            अगर (out.आकार() >= 2 && out.substr(out.आकार() - 2) == "``") {
                                out.resize(out.आकार() - 2);
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
                अगर (cp[0] == '`' && cp[1] == '`') {
                    अगर (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } अन्यथा {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    जारी;
                } अन्यथा अगर (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    जारी;
                } अन्यथा अगर (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    जारी;
                } अन्यथा अगर (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    जारी;
                } अन्यथा अगर (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    जारी;
                }
            }
            अगर (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                जारी;
            } अन्यथा अगर (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                जारी;
            }
            अगर (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                जारी;
            }
            अगर (*cp == '"') quote = !quote;
            अगर (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    वापस out;
}

//**********************************************************************
// Parser routines

व्योम V3PreProcImp::openFile(File_पंक्ति*, VInFilter* filterp, स्थिर string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    अगर (m_incError) वापस;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    स्थिर bool ok = filterp->readWholefile(filename, wholefile /*ref*/);
    अगर (!ok) {
        fileline()->v3error("File not found: " + filename);
        वापस;
    }

    अगर (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        अगर (m_lexp->m_streamp_ढेर.आकार() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = true;
            वापस;
        }
        // There's already a file active.  Push it to work on the new one.
        add_पंक्ति_Comment(0);
    }

    // Save file contents for future error reporting
    File_पंक्ति* स्थिर flsp = नया File_पंक्ति{filename};
    flsp->lineno(1);
    flsp->newContent();
    क्रम (स्थिर string& i : wholefile) flsp->contentp()->धकेलो_Text(i);

    // Save contents for V3Control --contents
    अगर (filename != V3Options::getStdPackagePath()) {
        bool containsVlt = false;
        क्रम (स्थिर string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            अगर (i.find("`verilator_config") != string::npos) {
                containsVlt = true;
                अवरोध;
            }
        }
        अगर (!containsVlt) {
            क्रम (स्थिर string& i : wholefile) V3Control::contents_धकेलो_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_पंक्ति_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    पूर्णांक eof_newline = 0;  // Number of characters following last newline
    पूर्णांक eof_lineno = 1;
    क्रम (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        bool strip = false;
        स्थिर अक्षर* स्थिर sp = it->तथ्य();
        स्थिर अक्षर* स्थिर ep = sp + it->लंबाई();
        // Only process if needed, as saves extra string allocations
        क्रम (स्थिर अक्षर* cp = sp; cp < ep; cp++) {
            अगर (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = true;
            } अन्यथा अगर (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } अन्यथा {
                ++eof_newline;
            }
        }
        अगर (strip) {
            string out;
            out.reserve(it->लंबाई());
            क्रम (स्थिर अक्षर* cp = sp; cp < ep; cp++) {
                अगर (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    अगर (eof_newline) {
        File_पंक्ति* स्थिर fl = नया File_पंक्ति{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

व्योम V3PreProcImp::जोड़ो_Unreadback_At_Bol(स्थिर string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    अगर (m_पंक्ति_Cmt == "") {
        m_पंक्ति_Cmt_Nl = true;
    } अन्यथा अगर (m_पंक्ति_Cmt[m_पंक्ति_Cmt.लंबाई() - 1] != '\n') {
        जोड़ो_Unreadback("\n");
    }
    जोड़ो_Unreadback(text);
}

व्योम V3PreProcImp::add_पंक्ति_Comment(पूर्णांक enterExit) {
    अगर (पंक्ति_Directives()) {
        जोड़ो_Unreadback_At_Bol(m_lexp->curFilelinep()->पंक्ति_Directive_Strg(enterExit));
    }
}

व्योम V3PreProcImp::dumpDefines(std::ब_प्रवाह& os) {
    क्रम (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        अगर (!it->second.params().empty()) os << it->second.params();
        अगर (!it->second.मूल्य().empty()) os << " " << it->second.मूल्य();
        os << '\n';
    }
}

व्योम V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    क्रम (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->धकेलो_Candidate("`"s + it->first);
    }
}

पूर्णांक V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    जबतक (true) {
    अग्र_tok:
        अगर (m_पंक्ति_Add) {
            m_पंक्ति_Add--;
            m_rawAtBol = true;
            yyourtext("\n", 1);
            अगर (debug() >= 5) debugToken(VP_WHITE, "LNA");
            वापस VP_WHITE;
        }
        अगर (m_पंक्ति_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            अटल string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_पंक्ति_Cmt;
            अगर (m_पंक्ति_Cmt_Nl) {
                अगर (!m_rawAtBol) s_rtncmt.जोड़ो(0, "\n");
                m_पंक्ति_Cmt_Nl = false;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.लंबाई());
            m_पंक्ति_Cmt = "";
            अगर (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            अगर (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_मूल्य(yyourtext(), yyourleng());
                जाओ अग्र_tok;
            } अन्यथा {
                अगर (debug() >= 5) debugToken(VP_TEXT, "LCM");
                वापस VP_TEXT;
            }
        }
        अगर (isEof()) वापस VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        पूर्णांक tok = m_lexp->lex();
        अगर (debug() >= 5) debugToken(tok, "RAW");

        अगर (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_पंक्ति = 0;
        } अन्यथा अगर (++m_tokens_On_पंक्ति > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        अगर (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            अगर (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            जाओ अग्र_tok;
        }

        अगर (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        वापस tok;
    }
}

व्योम V3PreProcImp::debugToken(पूर्णांक tok, स्थिर अक्षर* cmtp) {
    स्थिर अटल पूर्णांक s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    अगर (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::आकार_type pos;
        जबतक ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        जबतक ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        स्थिर string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<पूर्णांक>(m_states.आकार())
                       << ") dr" << m_defRefs.आकार() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        अगर (s_debugFileline >= 9) {
            std::अ_बाहर << m_lexp->m_tokFilelinep->warnContextSecondary() << प_पूर्ण;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

पूर्णांक V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    जबतक (true) {
    अग्र_tok:
        अगर (isEof()) वापस VP_EOF;
        पूर्णांक tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        अगर (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) वापस tok;
        अगर (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        अगर (tok == VP_COMMENT) {
            अगर (!m_off) {
                अगर (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    अगर (m_पंक्ति_Cmt.empty()) जोड़ो_Unreadback(" ");
                } अन्यथा अगर (m_lexp->m_keepComments) {
                    वापस tok;
                } अन्यथा {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    जोड़ो_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            क्रम (माप_प्रकार len = 0; len < static_cast<माप_प्रकार>(yyourleng()); len++) {
                अगर (yyourtext()[len] == '\n') m_पंक्ति_Add++;
            }
            जाओ अग्र_tok;
        }
        अगर (tok == VP_पंक्ति) {
            add_पंक्ति_Comment(m_lexp->m_enterExit);
            जाओ अग्र_tok;
        }

        अगर (tok == VP_DEFREF_JOIN) {
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
            स्थिर std::string name(yyourtext() + 1, yyourleng() - 1);
            अगर (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = true;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } अन्यथा {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        अगर (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            अगर (tok == VP_JOIN) जाओ अग्र_tok;
            अगर (tok == VP_प्रतीक_JOIN) tok = VP_प्रतीक;
        }
        अगर (tok == VP_प्रतीक_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            स्थिर std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ढेर.धकेलो(out);
            state_धकेलो(ps_JOIN);
            जाओ अग्र_tok;
        }

        // Deal with some special parser states
        चयन (state()) {
        हाल ps_शीर्ष: {
            अवरोध;
        }
        हाल ps_DEFNAME_UNDEF:  // FALLTHRU
        हाल ps_DEFNAME_DEFINE:  // FALLTHRU
        हाल ps_DEFNAME_IFDEF:  // FALLTHRU
        हाल ps_DEFNAME_IFNDEF:  // FALLTHRU
        हाल ps_DEFNAME_ELSIF: {
            अगर (tok == VP_प्रतीक) {
                m_lastSym.assign(yyourtext(), yyourleng());
                अगर (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    bool enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    अगर (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ढेर.emplace(VPreIfEntry{enable, false});
                    अगर (!enable) parsingOff();
                    state_निकालो();
                    जाओ अग्र_tok;
                } अन्यथा अगर (state() == ps_DEFNAME_ELSIF) {
                    अगर (m_ifdef_ढेर.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } अन्यथा {
                        // Handle `else portion
                        स्थिर VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                        m_ifdef_ढेर.निकालो();
                        अगर (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        स्थिर bool enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_ढेर.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        अगर (!enable) parsingOff();
                    }
                    state_निकालो();
                    जाओ अग्र_tok;
                } अन्यथा अगर (state() == ps_DEFNAME_UNDEF) {
                    अगर (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_निकालो();
                    जाओ अग्र_tok;
                } अन्यथा अगर (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->धकेलो_State_Def_Form();
                    जाओ अग्र_tok;
                } अन्यथा {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                अगर (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->धकेलो_State_व्यंजक();
                    m_व्यंजक_Parser.reset(fileline());
                    m_व्यंजक_Paren_Level = 1;
                    चयन (state()) {
                    हाल ps_DEFNAME_IFDEF: stateChange(ps_व्यंजक_IFDEF); अवरोध;
                    हाल ps_DEFNAME_IFNDEF: stateChange(ps_व्यंजक_IFNDEF); अवरोध;
                    हाल ps_DEFNAME_ELSIF: stateChange(ps_व्यंजक_ELSIF); अवरोध;
                    शेष: v3fatalSrc("bad case");
                    }
                    जाओ अग्र_tok;
                }
                अगर (!m_off) {
                    वापस tok;
                } अन्यथा {
                    जाओ अग्र_tok;
                }
            } अन्यथा अगर (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                अवरोध;
            } अन्यथा {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                जाओ अग्र_tok;
            }
        }
        हाल ps_व्यंजक_IFDEF:  // FALLTHRU
        हाल ps_व्यंजक_IFNDEF:  // FALLTHRU
        हाल ps_व्यंजक_ELSIF: {
            // `ifdef ( *here*
            File_पंक्ति* स्थिर flp = m_lexp->m_tokFilelinep;
            अगर (tok == VP_प्रतीक) {
                m_lastSym.assign(yyourtext(), yyourleng());
                स्थिर bool exists = defExists(m_lastSym);
                अगर (exists) {
                    स्थिर std::string मूल्य = def_मूल्य(m_lastSym);
                    अगर (VString::removeWhitespace(मूल्य) == "0") {
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
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_WHITE) {
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::BRA});
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_व्यंजक_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                अगर (--m_व्यंजक_Paren_Level > 0) {
                    m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::KET});
                    जाओ अग्र_tok;
                } अन्यथा {
                    // Done with parsing expression
                    bool enable = m_व्यंजक_Parser.परिणाम();
                    UINFO(4, "ifdef() result=" << enable);
                    अगर (state() == ps_व्यंजक_IFDEF || state() == ps_व्यंजक_IFNDEF) {
                        अगर (state() == ps_व्यंजक_IFNDEF) enable = !enable;
                        m_ifdef_ढेर.emplace(VPreIfEntry{enable, false});
                        अगर (!enable) parsingOff();
                        state_निकालो();
                        जाओ अग्र_tok;
                    } अन्यथा अगर (state() == ps_व्यंजक_ELSIF) {
                        अगर (m_ifdef_ढेर.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } अन्यथा {
                            // Handle `else portion
                            स्थिर VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                            m_ifdef_ढेर.निकालो();
                            अगर (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_ढेर.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            अगर (!enable) parsingOff();
                        }
                        state_निकालो();
                    }
                    जाओ अग्र_tok;
                }
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LNOT});
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "&&", 2)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LAND});
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "||", 2)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LOR});
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 2 && 0 == म_भेदन(yyourtext(), "->", 2)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::IMP});
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 3 && 0 == म_भेदन(yyourtext(), "<->", 3)) {
                m_व्यंजक_Parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::EQV});
                जाओ अग्र_tok;
            } अन्यथा {
                अगर (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    वापस tok;
                } अन्यथा {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                जाओ अग्र_tok;
            }
        }
        हाल ps_DEFFORM: {
            अगर (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_मूल्य;
                अगर (debug() >= 5) {
                    अ_बाहर << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->धकेलो_State_Def_मूल्य();
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT) {
                // IE, something like comment in formals
                अगर (!m_off) वापस tok;
                जाओ अग्र_tok;
            } अन्यथा {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                जाओ अग्र_tok;
            }
        }
        हाल ps_DEFVALUE: {
            अटल string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            अगर (tok == VP_DEFVALUE) {
                अगर (debug() >= 5) {  // LCOV_EXCL_START
                    अ_बाहर << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_मूल्य)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                स्थिर string formals = m_formals;
                string मूल्य = m_lexp->m_def_मूल्य;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                क्रम (माप_प्रकार i = 0; i < formals.लंबाई(); i++) {
                    अगर (formals[i] == '\n') s_newlines += "\n";
                }
                क्रम (माप_प्रकार i = 0; i < मूल्य.लंबाई(); i++) {
                    अगर (मूल्य[i] == '\n') s_newlines += "\n";
                }
                अगर (!m_off) {
                    // Remove leading and trailing whitespace
                    मूल्य = trimWhitespace(मूल्य, true);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(मूल्य) << "'");
                    define(fileline(), m_lastSym, मूल्य, formals, false);
                }
            } अन्यथा {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_निकालो();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.लंबाई());
            वापस VP_WHITE;
        }
        हाल ps_DEFPAREN: {
            अगर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                जाओ अग्र_tok;
            } अन्यथा {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                स्थिर VDefineRef* स्थिर refp = &(m_defRefs.शीर्ष());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_निकालो();
                जाओ अग्र_tok;
            }
        }
        हाल ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.शीर्ष());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य);
            m_lexp->m_def_मूल्य = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            अगर (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().धकेलो_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->धकेलो_State_Def_Arg(1);
                refp->nextarg("");
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().धकेलो_back(refp->nextarg());
                string out;
                अगर (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.निकालो(), refp);
                state_निकालो();
                अगर (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    अगर (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ढेर.empty(), "`` join stack empty, but in a ``");
                        स्थिर string lhs = m_join_ढेर.शीर्ष();
                        m_join_ढेर.निकालो();
                        out.जोड़ो(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_निकालो();
                    }
                    अगर (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    अन्यथा अगर (stateIsDefname()) {
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
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                अवरोध;
            } अन्यथा अगर (tok == VP_प्रतीक || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_धकेलो(ps_STRIFY);
                जाओ अग्र_tok;
            } अन्यथा {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_निकालो();
                जाओ अग्र_tok;
            }
        }
        हाल ps_INCNAME: {
            अगर (tok == VP_STRING) {
                state_निकालो();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.लंबाई() - 1, 1);
                include(m_lastSym);
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->धकेलो_State_Inc_Filename();
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                अवरोध;
            } अन्यथा {
                state_निकालो();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                जाओ अग्र_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        हाल ps_ERRORNAME: {
            अगर (tok == VP_STRING) {
                अगर (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_निकालो();
                जाओ अग्र_tok;
            } अन्यथा {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_निकालो();
                जाओ अग्र_tok;
            }
        }
        हाल ps_JOIN: {
            अगर (tok == VP_प्रतीक || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ढेर.empty(), "`` join stack empty, but in a ``");
                स्थिर string lhs = m_join_ढेर.शीर्ष();
                m_join_ढेर.निकालो();
                UINFO(5, "`` LHS:" << lhs);
                स्थिर string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                स्थिर string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_निकालो();
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                स्थिर string lhs = m_join_ढेर.शीर्ष();
                m_join_ढेर.निकालो();
                unputString(lhs);
                state_निकालो();
                जाओ अग्र_tok;
            } अन्यथा {
                // `define, etc, fall through and expand.  Pop back here.
                अवरोध;
            }
        }
        हाल ps_STRIFY: {
            अगर (tok == VP_STRIFY) {
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
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                अवरोध;
            } अन्यथा अगर (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                जाओ अग्र_tok;
            } अन्यथा अगर (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                अवरोध;
            } अन्यथा {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                जाओ अग्र_tok;
            }
        }
        शेष: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        चयन (tok) {
        हाल VP_INCLUDE:
            अगर (!m_off) {
                state_धकेलो(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            जाओ अग्र_tok;
        हाल VP_UNDEF: state_धकेलो(ps_DEFNAME_UNDEF); जाओ अग्र_tok;
        हाल VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_धकेलो(ps_DEFNAME_DEFINE);
            जाओ अग्र_tok;
        हाल VP_IFDEF: state_धकेलो(ps_DEFNAME_IFDEF); जाओ अग्र_tok;
        हाल VP_IFNDEF: state_धकेलो(ps_DEFNAME_IFNDEF); जाओ अग्र_tok;
        हाल VP_ELSIF: state_धकेलो(ps_DEFNAME_ELSIF); जाओ अग्र_tok;
        हाल VP_ELSE:
            अगर (m_ifdef_ढेर.empty()) {
                fileline()->v3error("`else with no matching `if");
            } अन्यथा {
                स्थिर VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                m_ifdef_ढेर.निकालो();
                स्थिर bool enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ढेर.emplace(VPreIfEntry{enable, lastIf.everOn()});
                अगर (!lastIf.on()) parsingOn();
                अगर (!enable) parsingOff();
            }
            जाओ अग्र_tok;
        हाल VP_ENDIF:
            UINFO(4, "Endif ");
            अगर (m_ifdef_ढेर.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } अन्यथा {
                स्थिर VPreIfEntry lastIf = m_ifdef_ढेर.शीर्ष();
                m_ifdef_ढेर.निकालो();
                अगर (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            जाओ अग्र_tok;

        हाल VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            स्थिर string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            अगर (m_defPutJoin) {
                m_defPutJoin = false;
                unputString("``");
            }
            अगर (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                जाओ अग्र_tok;
            }
            // Substitute
            अगर (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                अगर (m_off) {
                    जाओ अग्र_tok;
                } अन्यथा {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    जाओ अग्र_tok;
                }
            } अन्यथा {
                स्थिर string params = defParams(name);
                अगर (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    अगर (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    अगर (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        अगर (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ढेर.empty(), "`` join stack empty, but in a ``");
                            स्थिर string lhs = m_join_ढेर.शीर्ष();
                            m_join_ढेर.निकालो();
                            out.जोड़ो(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_निकालो();
                        }
                        अगर (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        अन्यथा अगर (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } अन्यथा {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* स्थिर refp = &(m_defRefs.शीर्ष());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_मूल्य + out);
                        m_lexp->m_def_मूल्य = "";
                    }
                    जाओ अग्र_tok;
                } अन्यथा {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    अगर (!m_defRefs.empty()) m_defRefs.शीर्ष().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_धकेलो(ps_DEFPAREN);
                    m_lexp->धकेलो_State_Def_Arg(0);
                    जाओ अग्र_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            जाओ अग्र_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        हाल VP_ERROR: {
            state_धकेलो(ps_ERRORNAME);
            जाओ अग्र_tok;
        }
        हाल VP_EOF:
            अगर (!m_ifdef_ढेर.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            वापस tok;
        हाल VP_UNDEFINEALL:
            अगर (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            जाओ अग्र_tok;
        हाल VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_धकेलो(ps_STRIFY);
            जाओ अग्र_tok;
        हाल VP_प्रतीक:
        हाल VP_STRING:
        हाल VP_TEXT: {
            m_defDepth = 0;
            अगर (!m_off) {
                वापस tok;
            } अन्यथा {
                जाओ अग्र_tok;
            }
        }
        हाल VP_WHITE:  // Handled at top of loop
        हाल VP_COMMENT:  // Handled at top of loop
        हाल VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        हाल VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        शेष:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            अवरोध;  // LCOV_EXCL_LINE
        }
        वापस tok;
    }
}

पूर्णांक V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    अगर (!m_finAhead) {
        m_finAhead = true;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    स्थिर पूर्णांक tok = m_finToken;
    buf = m_finBuf;
    अगर (false && debug() >= 5) {
        स्थिर string bufcln = V3PreLex::cleanDbgStrg(buf);
        स्थिर string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    स्थिर अक्षर* bufp = buf.c_str();
    जबतक (*bufp == '\n') bufp++;
    अगर ((tok == VP_TEXT || tok == VP_पंक्ति) && VString::startsWith(bufp, "`line ")) {
        पूर्णांक enter;
        m_finFilelinep->पंक्ति_Directive(bufp, enter /*ref*/);
    } अन्यथा {
        अगर (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->पंक्ति_Directives()) {
            अगर (स्थिर पूर्णांक outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                अगर (debug() >= 5) {
                    स्थिर string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                अगर (outBehind > 0
                    && (outBehind <= static_cast<पूर्णांक>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    अगर (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        वापस VP_TEXT;
                    }
                } अन्यथा {
                    // Need to backup, use `line
                    buf = m_finFilelinep->पंक्ति_Directive_Strg(0);
                    वापस VP_पंक्ति;
                }
            }
        }
        // Track newlines in prep for next token
        क्रम (स्थिर अक्षर& c : buf) {
            अगर (c == '\n') {
                m_finAtBol = true;
                m_finFilelinep->linenoInc();
            } अन्यथा {
                m_finAtBol = false;
            }
        }
    }
    m_finAhead = false;  // Consumed the token
    वापस tok;
}

string V3PreProcImp::लो_पंक्ति() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    अगर (isEof()) वापस "";
    स्थिर अक्षर* rtnp;
    bool gotEof = false;
    जबतक (nullptr == (rtnp = std::म_अक्षर(m_पंक्ति_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.सफा();
        स्थिर पूर्णांक tok = getFinalToken(m_tokenBuf /*ref*/);
        अगर (debug() >= 5) {
            स्थिर string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            स्थिर string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        अगर (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            अगर (m_पंक्ति_Chars != "" && m_पंक्ति_Chars[m_पंक्ति_Chars.लंबाई() - 1] != '\n') {
                m_पंक्ति_Chars.append("\n");
            }
            gotEof = true;
        } अन्यथा {
            m_पंक्ति_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    स्थिर पूर्णांक len = rtnp - m_पंक्ति_Chars.c_str() + 1;
    string the_पंक्ति(m_पंक्ति_Chars, 0, len);
    m_पंक्ति_Chars = m_पंक्ति_Chars.erase(0, len);  // Remove returned characters
    अगर (debug() >= 4) {
        स्थिर string lncln = V3PreLex::cleanDbgStrg(the_पंक्ति);
        स्थिर string flcol = m_lexp->m_tokFilelinep->ascii_पंक्ति_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    वापस the_पंक्ति;
}
