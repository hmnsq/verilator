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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3PreProc.h"

#شامل "V3Control.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3LanguageWords.h"
#شامل "V3PreExpr.h"
#شامل "V3PreLex.h"
#شامل "V3PreShell.h"
#شامل "V3String.h"

#شامل <algorithm>
#شامل <cstdlib>
#شامل <fstream>
#شامل <stack>
#شامل <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#شامل "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

رده VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_سطر* ثابت m_fileline;  // Where it was declared
    ثابت string m_مقدار;  // Value of define
    ثابت string m_params;  // Parameters
    ثابت منطقی m_cmdline;  // Set on command line, don't `undefineall
عمومی:
    VDefine(File_سطر* fl, ثابت string& مقدار, ثابت string& params, منطقی cmdline)
        : m_fileline{fl}
        , m_مقدار{مقدار}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_سطر* fileline() ثابت { بازگشت m_fileline; }
    string مقدار() ثابت { بازگشت m_مقدار; }
    string params() ثابت { بازگشت m_params; }
    منطقی cmdline() ثابت { بازگشت m_cmdline; }
};

//*************************************************************************

رده VDefineRef final {
    // One for each pending define substitution
    ثابت string m_name;  // Define last name being defined
    ثابت string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    صحیح m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
عمومی:
    string name() ثابت { بازگشت m_name; }
    string params() ثابت { بازگشت m_params; }
    string nextarg() ثابت { بازگشت m_nextarg; }
    تهی nextarg(ثابت string& مقدار) { m_nextarg = مقدار; }
    صحیح parenLevel() ثابت { بازگشت m_parenLevel; }
    تهی parenLevel(صحیح مقدار) { m_parenLevel = مقدار; }
    std::vector<string>& args() { بازگشت m_args; }
    VDefineRef(ثابت string& name, ثابت string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = پیشفرض;
};

//*************************************************************************
/// Data for parsing on/off

رده VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ثابت منطقی m_on;  // Current parse for this ifdef level is "on"
    ثابت منطقی m_everOn;  // Some if term in elsif tree has been on
عمومی:
    منطقی on() ثابت { بازگشت m_on; }
    منطقی everOn() ثابت { بازگشت m_everOn; }
    VPreIfEntry(منطقی on, منطقی everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = پیشفرض;
};

//*************************************************************************
// Data for a preprocessor instantiation.

رده V3PreProcImp final : عمومی V3PreProc {
عمومی:
    // TYPES
    using DefinesMap = std::map<ثابت std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ثابت V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    صحیح m_lastLineno = 0;  // Last line number (stall detection)
    صحیح m_tokens_On_سطر = 0;  // Number of tokens on line (stall detection)

    شمارش ProcState : uint8_t {
        ps_بالاترین,
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
        ps_عبارت_IFDEF,
        ps_عبارت_IFNDEF,
        ps_عبارت_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ایستا ثابت نویسه* procStateName(ProcState s) {
        ایستا ثابت نویسه* ثابت states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        بازگشت states[s];
    }

    std::پشته<ProcState> m_states;  ///< Current state of parser
    صحیح m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    منطقی m_incError = نادرست;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_سطر_Cmt;  ///< Line comment(s) to be returned
    منطقی m_سطر_Cmt_Nl = نادرست;  ///< Newline needed before inserting lineCmt
    صحیح m_سطر_Add = 0;  ///< Empty lines to return to maintain line count
    منطقی m_rawAtBol = درست;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    منطقی m_finAhead = نادرست;  ///< Have read a token ahead
    صحیح m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    منطقی m_finAtBol = درست;  ///< Last getFinalToken left us at beginning of line
    File_سطر* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::پشته<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::پشته<VPreIfEntry> m_ifdef_پشته;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    منطقی m_defPutJoin = نادرست;  ///< Insert `` after substitution

    // For `` join
    std::پشته<string> m_join_پشته;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_عبارت m_عبارت_Parser;  ///< Parser for () expression
    صحیح m_عبارت_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_سطر_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    تهی v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    ایستا ثابت نویسه* tokenName(صحیح tok);
    تهی debugToken(صحیح tok, ثابت نویسه* cmtp);
    تهی parse_بالاترین();
    تهی parseUndef();

خصوصی:
    // Internal methods
    تهی endOfOneFile();
    string defineSubst(VDefineRef* refp);

    منطقی defExists(ثابت string& name);
    منطقی defCmdline(ثابت string& name);
    string def_مقدار(ثابت string& name);
    string defParams(ثابت string& name);
    File_سطر* defFileline(ثابت string& name);

    string commentCleanup(ثابت string& text);
    منطقی commentTokenMatch(string& cmdr, ثابت نویسه* strg);
    ایستا string trimWhitespace(ثابت string& strg, منطقی trailing);
    تهی unputString(ثابت string& strg);
    تهی unputDefrefString(ثابت string& strg);

    تهی parsingOn() {
        m_off--;
        اگر (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    تهی parsingOff() { m_off++; }

    صحیح getRawToken();
    صحیح getStateToken();
    صحیح getFinalToken(string& buf);

    ProcState state() ثابت { بازگشت m_states.بالاترین(); }
    منطقی stateIsDefname() ثابت {
        بازگشت state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    تهی state_افزودن(ProcState state) { m_states.افزودن(state); }
    تهی state_برداشت() {
        m_states.برداشت();
        اگر (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.افزودن(ps_بالاترین);  // LCOV_EXCL_LINE
        }
    }
    تهی stateChange(ProcState state) {
        state_برداشت();
        state_افزودن(state);
    }

عمومی:
    // METHODS, called from upper level shell
    تهی openFile(File_سطر* fl, VInFilter* filterp, ثابت string& filename) override;
    منطقی isEof() ثابت override { بازگشت m_lexp->curStreamp()->m_eof; }
    تهی forceEof() { m_lexp->curStreamp()->m_eof = درست; }
    string getline() override;
    تهی درج_Unreadback(ثابت string& text) override { m_سطر_Cmt += text; }
    تهی درج_Unreadback_At_Bol(ثابت string& text);
    تهی add_سطر_Comment(صحیح enterExit);
    تهی dumpDefines(std::ostream& os) override;
    تهی candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    تهی comment(ثابت string& text) override;  // Comment detected (if keepComments==2)
    تهی include(ثابت string& filename) override;  // Request a include file be processed
    تهی undef(ثابت string& name) override;
    مجازی تهی undefineall();
    تهی define(File_سطر* fl, ثابت string& name, ثابت string& مقدار, ثابت string& params,
                منطقی cmdline) override;
    string removeDefines(ثابت string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.افزودن(ps_بالاترین); }
    // cppcheck-suppress duplInheritedMember
    تهی configure(File_سطر* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = این;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = جدید File_سطر{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = جدید V3PreLex{این, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        اگر (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_سطر* fl) {
    V3PreProcImp* ثابت preprocp = جدید V3PreProcImp;
    preprocp->configure(fl);
    بازگشت preprocp;
}

تهی V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_عبارت::selfTest(); }

//*************************************************************************
// Defines

تهی V3PreProcImp::undef(ثابت string& name) {
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        درج_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
تهی V3PreProcImp::undefineall() {
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        درج_Unreadback("`undefineall\n");
    برای (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        اگر (!it->second.cmdline()) m_defines.erase(it);
    }
}
منطقی V3PreProcImp::defExists(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    بازگشت (iter != m_defines.end());
}
منطقی V3PreProcImp::defCmdline(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    اگر (iter == m_defines.end()) { بازگشت نادرست; }
    بازگشت iter->second.cmdline();
}
string V3PreProcImp::def_مقدار(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    اگر (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        بازگشت "";
    }
    بازگشت iter->second.مقدار();
}
string V3PreProcImp::defParams(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    اگر (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        بازگشت "";
    }
    بازگشت iter->second.params();
}
File_سطر* V3PreProcImp::defFileline(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    اگر (iter == m_defines.end()) بازگشت nullptr;
    بازگشت iter->second.fileline();
}
تهی V3PreProcImp::define(File_سطر* fl, ثابت string& name, ثابت string& مقدار,
                          ثابت string& params, منطقی cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << مقدار << "' params '" << params << "'");
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(مقدار.size());
        برای (string::size_type pos = 0; pos < مقدار.size(); ++pos) {
            اگر (مقدار[pos] == '\n' && (pos == 0 || مقدار[pos - 1] != '\\')) out += '\\';
            out += مقدار[pos];
        }
        درج_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    اگر (!V3_Language_واژگان::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } وگرنه {
        اگر (defExists(name)) {
            اگر (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << مقدار
                               << "' to existing command line define value: '" << def_مقدار(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_مقدار(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                بازگشت;
            } وگرنه {
                اگر (!(def_مقدار(name) == مقدار
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << مقدار
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_مقدار(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, مقدار, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(ثابت string& text) {
    string val;
    string rtnsym = text;
    برای (صحیح loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        اگر (xsym[0] == '`') xsym.erase(0, 1);
        اگر (defExists(xsym)) {
            val = def_مقدار(xsym);
            اگر (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } وگرنه {
                بشکن;
            }
        } وگرنه {
            بشکن;
        }
    }
    بازگشت rtnsym;  // NA
}

//**********************************************************************
// Comments

تهی V3PreProcImp::include(ثابت string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ثابت string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    وقتی ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    وقتی ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    وقتی ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    وقتی ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    وقتی ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    وقتی ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    وقتی (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    بازگشت cmd;
}

منطقی V3PreProcImp::commentTokenMatch(string& cmdr, ثابت نویسه* strg) {
    صحیح len = std::strlen(strg);
    اگر (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        اگر (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        بازگشت درست;
    } وگرنه {
        بازگشت نادرست;
    }
}

تهی V3PreProcImp::comment(ثابت string& text) {
    // Comment detected.  Only keep relevant data.
    منطقی printed = نادرست;
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        درج_Unreadback(text);
        printed = درست;
    }

    ثابت نویسه* cp = text.c_str();
    اگر (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } وگرنه {
        بازگشت;
    }

    وقتی (std::isspace(*cp)) ++cp;

    منطقی synth = نادرست;
    منطقی vlcomment = نادرست;
    اگر ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        اگر (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            بازگشت;
        }
        vlcomment = درست;
    } وگرنه اگر (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        اگر (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            بازگشت;
        }
        synth = درست;
    } وگرنه اگر (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = درست;
    } وگرنه اگر (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = درست;
    } وگرنه اگر (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = درست;
    } وگرنه {
        بازگشت;
    }

    وقتی (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    اگر (synth) {
        اگر (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            اگر (commentTokenMatch(cmd /*ref*/, "full_case")) {
                اگر (!printed) درج_Unreadback("/*verilator full_case*/");
            }
            اگر (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                اگر (!printed) درج_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } وگرنه اگر (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        اگر (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            وقتی (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            وقتی (std::isspace(arg[0])) arg = arg.substr(1);
            اگر (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            اگر (!printed) درج_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } وگرنه اگر (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            وقتی (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            اگر (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                بازگشت;
            }
            اگر (!printed) درج_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } وگرنه {
            اگر (!printed) درج_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_سطر* V3PreProc::fileline() {
    ثابت V3PreProcImp* idatap = static_cast<V3PreProcImp*>(این);
    بازگشت idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ثابت نویسه* V3PreProcImp::tokenName(صحیح tok) {
    انتخاب (tok) {
    حالت VP_BACKQUOTE: بازگشت ("BACKQUOTE");
    حالت VP_COMMENT: بازگشت ("COMMENT");
    حالت VP_DEFARG: بازگشت ("DEFARG");
    حالت VP_DEFFORM: بازگشت ("DEFFORM");
    حالت VP_DEFINE: بازگشت ("DEFINE");
    حالت VP_DEFREF: بازگشت ("DEFREF");
    حالت VP_DEFREF_JOIN: بازگشت ("DEFREF_JOIN");
    حالت VP_DEFVALUE: بازگشت ("DEFVALUE");
    حالت VP_ELSE: بازگشت ("ELSE");
    حالت VP_ELSIF: بازگشت ("ELSIF");
    حالت VP_ENDIF: بازگشت ("ENDIF");
    حالت VP_EOF: بازگشت ("EOF");
    حالت VP_EOF_ERROR: بازگشت ("EOF_ERROR");
    حالت VP_ERROR: بازگشت ("ERROR");
    حالت VP_IFDEF: بازگشت ("IFDEF");
    حالت VP_IFNDEF: بازگشت ("IFNDEF");
    حالت VP_JOIN: بازگشت ("JOIN");
    حالت VP_INCLUDE: بازگشت ("INCLUDE");
    حالت VP_سطر: بازگشت ("LINE");
    حالت VP_STRIFY: بازگشت ("STRIFY");
    حالت VP_STRING: بازگشت ("STRING");
    حالت VP_نماد: بازگشت ("SYMBOL");
    حالت VP_نماد_JOIN: بازگشت ("SYMBOL_JOIN");
    حالت VP_TEXT: بازگشت ("TEXT");
    حالت VP_UNDEF: بازگشت ("UNDEF");
    حالت VP_UNDEFINEALL: بازگشت ("UNDEFINEALL");
    حالت VP_WHITE: بازگشت ("WHITE");
    پیشفرض: بازگشت ("?");
    }
}

تهی V3PreProcImp::unputString(ثابت string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    اگر (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

تهی V3PreProcImp::unputDefrefString(ثابت string& strg) {
    ثابت صحیح multiline = std::شمار(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ثابت string& strg, منطقی trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    وقتی (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    اگر (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    اگر (trailing) {
        string::size_type trailspace = 0;
        وقتی (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        اگر (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        اگر (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    بازگشت out;
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
    برای (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ثابت string مقدار = def_مقدار(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(مقدار) << "'");

    std::map<ثابت string, string> arg_مقدار_By_Name;
    {  // Parse argument list into map
        unsigned رقم_Args = 0;
        string argName;
        صحیح paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        منطقی quote = نادرست;
        منطقی haveDefault = نادرست;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ثابت string params = refp->params();  // Must keep str in scope to get pointer
        ثابت نویسه* cp = params.c_str();
        اگر (*cp == '(') cp++;
        برای (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            اگر (!quote && paren == 1) {
                اگر (*cp == ')' || *cp == ',') {
                    string مقدار_Def;
                    اگر (haveDefault) {
                        مقدار_Def = token;
                    } وگرنه {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, درست);
                    UINFO(4, "    Got Arg=" << رقم_Args << "  argName='" << argName
                                            << "'  default='" << مقدار_Def << "'");
                    // Parse it
                    اگر (argName != "") {
                        اگر (refp->args().size() > رقم_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ثابت string arg = trimWhitespace(refp->args()[رقم_Args], درست);
                            اگر (arg != "") مقدار_Def = arg;
                        } وگرنه اگر (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            بازگشت " `" + refp->name() + " ";
                        }
                        رقم_Args++;
                    }
                    arg_مقدار_By_Name[argName] = مقدار_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = نادرست;
                    ادامه;
                } وگرنه اگر (*cp == '=') {
                    haveDefault = درست;
                    argName = token;
                    token = "";
                    ادامه;
                }
            }
            اگر (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                ادامه;
            }
            اگر (!quote) {
                اگر (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } وگرنه اگر (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            اگر (*cp == '"') quote = !quote;
            اگر (*cp) token += *cp;
        }
        اگر (refp->args().size() > رقم_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && رقم_Args == 0
                 && trimWhitespace(refp->args()[0], نادرست) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            بازگشت " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        منطقی quote = نادرست;
        منطقی triquote = نادرست;
        منطقی backslashesc = نادرست;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        برای (ثابت نویسه* cp = مقدار.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            اگر (!quote && !triquote && *cp == '\\') {
                backslashesc = درست;
            } وگرنه اگر (std::isspace(*cp)) {
                backslashesc = نادرست;
            }
            اگر (!quote && !triquote) {
                اگر (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    ادامه;
                }
                اگر (argName != "") {
                    // Found a possible variable substitution
                    ثابت auto iter = arg_مقدار_By_Name.find(argName);
                    اگر (iter != arg_مقدار_By_Name.end()) {
                        // Substitute
                        ثابت string subst = iter->second;
                        اگر (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            اگر (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } وگرنه {
                            out += subst;
                        }
                    } وگرنه {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                اگر (cp[0] == '`' && cp[1] == '`') {
                    اگر (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } وگرنه {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    ادامه;
                } وگرنه اگر (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    ادامه;
                } وگرنه اگر (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    ادامه;
                } وگرنه اگر (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    ادامه;
                } وگرنه اگر (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    ادامه;
                }
            }
            اگر (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                ادامه;
            } وگرنه اگر (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                ادامه;
            }
            اگر (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                ادامه;
            }
            اگر (*cp == '"') quote = !quote;
            اگر (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    بازگشت out;
}

//**********************************************************************
// Parser routines

تهی V3PreProcImp::openFile(File_سطر*, VInFilter* filterp, ثابت string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    اگر (m_incError) بازگشت;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ثابت منطقی ok = filterp->readWholefile(filename, wholefile /*ref*/);
    اگر (!ok) {
        fileline()->v3error("File not found: " + filename);
        بازگشت;
    }

    اگر (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        اگر (m_lexp->m_streamp_پشته.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = درست;
            بازگشت;
        }
        // There's already a file active.  Push it to work on the new one.
        add_سطر_Comment(0);
    }

    // Save file contents for future error reporting
    File_سطر* ثابت flsp = جدید File_سطر{filename};
    flsp->lineno(1);
    flsp->newContent();
    برای (ثابت string& i : wholefile) flsp->contentp()->افزودن_Text(i);

    // Save contents for V3Control --contents
    اگر (filename != V3Options::getStdPackagePath()) {
        منطقی containsVlt = نادرست;
        برای (ثابت string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            اگر (i.find("`verilator_config") != string::npos) {
                containsVlt = درست;
                بشکن;
            }
        }
        اگر (!containsVlt) {
            برای (ثابت string& i : wholefile) V3Control::contents_افزودن_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_سطر_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    صحیح eof_newline = 0;  // Number of characters following last newline
    صحیح eof_lineno = 1;
    برای (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        منطقی strip = نادرست;
        ثابت نویسه* ثابت sp = it->data();
        ثابت نویسه* ثابت ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        برای (ثابت نویسه* cp = sp; cp < ep; cp++) {
            اگر (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = درست;
            } وگرنه اگر (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } وگرنه {
                ++eof_newline;
            }
        }
        اگر (strip) {
            string out;
            out.reserve(it->length());
            برای (ثابت نویسه* cp = sp; cp < ep; cp++) {
                اگر (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    اگر (eof_newline) {
        File_سطر* ثابت fl = جدید File_سطر{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

تهی V3PreProcImp::درج_Unreadback_At_Bol(ثابت string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    اگر (m_سطر_Cmt == "") {
        m_سطر_Cmt_Nl = درست;
    } وگرنه اگر (m_سطر_Cmt[m_سطر_Cmt.length() - 1] != '\n') {
        درج_Unreadback("\n");
    }
    درج_Unreadback(text);
}

تهی V3PreProcImp::add_سطر_Comment(صحیح enterExit) {
    اگر (سطر_Directives()) {
        درج_Unreadback_At_Bol(m_lexp->curFilelinep()->سطر_Directive_Strg(enterExit));
    }
}

تهی V3PreProcImp::dumpDefines(std::ostream& os) {
    برای (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        اگر (!it->second.params().empty()) os << it->second.params();
        اگر (!it->second.مقدار().empty()) os << " " << it->second.مقدار();
        os << '\n';
    }
}

تهی V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    برای (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->افزودن_Candidate("`"s + it->first);
    }
}

صحیح V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    وقتی (درست) {
    next_tok:
        اگر (m_سطر_Add) {
            m_سطر_Add--;
            m_rawAtBol = درست;
            yyourtext("\n", 1);
            اگر (debug() >= 5) debugToken(VP_WHITE, "LNA");
            بازگشت VP_WHITE;
        }
        اگر (m_سطر_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ایستا string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_سطر_Cmt;
            اگر (m_سطر_Cmt_Nl) {
                اگر (!m_rawAtBol) s_rtncmt.درج(0, "\n");
                m_سطر_Cmt_Nl = نادرست;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_سطر_Cmt = "";
            اگر (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            اگر (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_مقدار(yyourtext(), yyourleng());
                برو next_tok;
            } وگرنه {
                اگر (debug() >= 5) debugToken(VP_TEXT, "LCM");
                بازگشت VP_TEXT;
            }
        }
        اگر (isEof()) بازگشت VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        صحیح tok = m_lexp->lex();
        اگر (debug() >= 5) debugToken(tok, "RAW");

        اگر (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_سطر = 0;
        } وگرنه اگر (++m_tokens_On_سطر > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        اگر (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            اگر (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            برو next_tok;
        }

        اگر (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        بازگشت tok;
    }
}

تهی V3PreProcImp::debugToken(صحیح tok, ثابت نویسه* cmtp) {
    ثابت ایستا صحیح s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    اگر (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        وقتی ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        وقتی ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<صحیح>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        اگر (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

صحیح V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    وقتی (درست) {
    next_tok:
        اگر (isEof()) بازگشت VP_EOF;
        صحیح tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        اگر (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) بازگشت tok;
        اگر (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        اگر (tok == VP_COMMENT) {
            اگر (!m_off) {
                اگر (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    اگر (m_سطر_Cmt.empty()) درج_Unreadback(" ");
                } وگرنه اگر (m_lexp->m_keepComments) {
                    بازگشت tok;
                } وگرنه {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    درج_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            برای (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                اگر (yyourtext()[len] == '\n') m_سطر_Add++;
            }
            برو next_tok;
        }
        اگر (tok == VP_سطر) {
            add_سطر_Comment(m_lexp->m_enterExit);
            برو next_tok;
        }

        اگر (tok == VP_DEFREF_JOIN) {
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
            ثابت std::string name(yyourtext() + 1, yyourleng() - 1);
            اگر (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = درست;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } وگرنه {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        اگر (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            اگر (tok == VP_JOIN) برو next_tok;
            اگر (tok == VP_نماد_JOIN) tok = VP_نماد;
        }
        اگر (tok == VP_نماد_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ثابت std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_پشته.افزودن(out);
            state_افزودن(ps_JOIN);
            برو next_tok;
        }

        // Deal with some special parser states
        انتخاب (state()) {
        حالت ps_بالاترین: {
            بشکن;
        }
        حالت ps_DEFNAME_UNDEF:  // FALLTHRU
        حالت ps_DEFNAME_DEFINE:  // FALLTHRU
        حالت ps_DEFNAME_IFDEF:  // FALLTHRU
        حالت ps_DEFNAME_IFNDEF:  // FALLTHRU
        حالت ps_DEFNAME_ELSIF: {
            اگر (tok == VP_نماد) {
                m_lastSym.assign(yyourtext(), yyourleng());
                اگر (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    منطقی enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    اگر (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_پشته.emplace(VPreIfEntry{enable, نادرست});
                    اگر (!enable) parsingOff();
                    state_برداشت();
                    برو next_tok;
                } وگرنه اگر (state() == ps_DEFNAME_ELSIF) {
                    اگر (m_ifdef_پشته.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } وگرنه {
                        // Handle `else portion
                        ثابت VPreIfEntry lastIf = m_ifdef_پشته.بالاترین();
                        m_ifdef_پشته.برداشت();
                        اگر (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ثابت منطقی enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_پشته.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        اگر (!enable) parsingOff();
                    }
                    state_برداشت();
                    برو next_tok;
                } وگرنه اگر (state() == ps_DEFNAME_UNDEF) {
                    اگر (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_برداشت();
                    برو next_tok;
                } وگرنه اگر (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->افزودن_State_Def_Form();
                    برو next_tok;
                } وگرنه {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                اگر (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->افزودن_State_عبارت();
                    m_عبارت_Parser.reset(fileline());
                    m_عبارت_Paren_Level = 1;
                    انتخاب (state()) {
                    حالت ps_DEFNAME_IFDEF: stateChange(ps_عبارت_IFDEF); بشکن;
                    حالت ps_DEFNAME_IFNDEF: stateChange(ps_عبارت_IFNDEF); بشکن;
                    حالت ps_DEFNAME_ELSIF: stateChange(ps_عبارت_ELSIF); بشکن;
                    پیشفرض: v3fatalSrc("bad case");
                    }
                    برو next_tok;
                }
                اگر (!m_off) {
                    بازگشت tok;
                } وگرنه {
                    برو next_tok;
                }
            } وگرنه اگر (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                بشکن;
            } وگرنه {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                برو next_tok;
            }
        }
        حالت ps_عبارت_IFDEF:  // FALLTHRU
        حالت ps_عبارت_IFNDEF:  // FALLTHRU
        حالت ps_عبارت_ELSIF: {
            // `ifdef ( *here*
            File_سطر* ثابت flp = m_lexp->m_tokFilelinep;
            اگر (tok == VP_نماد) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ثابت منطقی exists = defExists(m_lastSym);
                اگر (exists) {
                    ثابت std::string مقدار = def_مقدار(m_lastSym);
                    اگر (VString::removeWhitespace(مقدار) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << مقدار
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, exists});
                برو next_tok;
            } وگرنه اگر (tok == VP_WHITE) {
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::BRA});
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_عبارت_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                اگر (--m_عبارت_Paren_Level > 0) {
                    m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::KET});
                    برو next_tok;
                } وگرنه {
                    // Done with parsing expression
                    منطقی enable = m_عبارت_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    اگر (state() == ps_عبارت_IFDEF || state() == ps_عبارت_IFNDEF) {
                        اگر (state() == ps_عبارت_IFNDEF) enable = !enable;
                        m_ifdef_پشته.emplace(VPreIfEntry{enable, نادرست});
                        اگر (!enable) parsingOff();
                        state_برداشت();
                        برو next_tok;
                    } وگرنه اگر (state() == ps_عبارت_ELSIF) {
                        اگر (m_ifdef_پشته.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } وگرنه {
                            // Handle `else portion
                            ثابت VPreIfEntry lastIf = m_ifdef_پشته.بالاترین();
                            m_ifdef_پشته.برداشت();
                            اگر (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_پشته.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            اگر (!enable) parsingOff();
                        }
                        state_برداشت();
                    }
                    برو next_tok;
                }
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LNOT});
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LAND});
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LOR});
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::IMP});
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_عبارت_Parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::EQV});
                برو next_tok;
            } وگرنه {
                اگر (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    بازگشت tok;
                } وگرنه {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                برو next_tok;
            }
        }
        حالت ps_DEFFORM: {
            اگر (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_مقدار;
                اگر (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->افزودن_State_Def_مقدار();
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT) {
                // IE, something like comment in formals
                اگر (!m_off) بازگشت tok;
                برو next_tok;
            } وگرنه {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                برو next_tok;
            }
        }
        حالت ps_DEFVALUE: {
            ایستا string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            اگر (tok == VP_DEFVALUE) {
                اگر (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_مقدار)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ثابت string formals = m_formals;
                string مقدار = m_lexp->m_def_مقدار;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                برای (size_t i = 0; i < formals.length(); i++) {
                    اگر (formals[i] == '\n') s_newlines += "\n";
                }
                برای (size_t i = 0; i < مقدار.length(); i++) {
                    اگر (مقدار[i] == '\n') s_newlines += "\n";
                }
                اگر (!m_off) {
                    // Remove leading and trailing whitespace
                    مقدار = trimWhitespace(مقدار, درست);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(مقدار) << "'");
                    define(fileline(), m_lastSym, مقدار, formals, نادرست);
                }
            } وگرنه {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_برداشت();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            بازگشت VP_WHITE;
        }
        حالت ps_DEFPAREN: {
            اگر (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                برو next_tok;
            } وگرنه {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ثابت VDefineRef* ثابت refp = &(m_defRefs.بالاترین());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_برداشت();
                برو next_tok;
            }
        }
        حالت ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.بالاترین());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_مقدار);
            m_lexp->m_def_مقدار = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            اگر (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().افزودن_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->افزودن_State_Def_Arg(1);
                refp->nextarg("");
                برو next_tok;
            } وگرنه اگر (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().افزودن_back(refp->nextarg());
                string out;
                اگر (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.برداشت(), refp);
                state_برداشت();
                اگر (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    اگر (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_پشته.empty(), "`` join stack empty, but in a ``");
                        ثابت string lhs = m_join_پشته.بالاترین();
                        m_join_پشته.برداشت();
                        out.درج(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_برداشت();
                    }
                    اگر (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    وگرنه اگر (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.بالاترین().parenLevel();
                } وگرنه {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.بالاترین());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_مقدار + out);
                    m_lexp->m_def_مقدار = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                برو next_tok;
            } وگرنه اگر (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                بشکن;
            } وگرنه اگر (tok == VP_نماد || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                برو next_tok;
            } وگرنه اگر (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_افزودن(ps_STRIFY);
                برو next_tok;
            } وگرنه {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_برداشت();
                برو next_tok;
            }
        }
        حالت ps_INCNAME: {
            اگر (tok == VP_STRING) {
                state_برداشت();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                برو next_tok;
            } وگرنه اگر (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->افزودن_State_Inc_Filename();
                برو next_tok;
            } وگرنه اگر (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                بشکن;
            } وگرنه {
                state_برداشت();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                برو next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        حالت ps_ERRORNAME: {
            اگر (tok == VP_STRING) {
                اگر (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_برداشت();
                برو next_tok;
            } وگرنه {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_برداشت();
                برو next_tok;
            }
        }
        حالت ps_JOIN: {
            اگر (tok == VP_نماد || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_پشته.empty(), "`` join stack empty, but in a ``");
                ثابت string lhs = m_join_پشته.بالاترین();
                m_join_پشته.برداشت();
                UINFO(5, "`` LHS:" << lhs);
                ثابت string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ثابت string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_برداشت();
                برو next_tok;
            } وگرنه اگر (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ثابت string lhs = m_join_پشته.بالاترین();
                m_join_پشته.برداشت();
                unputString(lhs);
                state_برداشت();
                برو next_tok;
            } وگرنه {
                // `define, etc, fall through and expand.  Pop back here.
                بشکن;
            }
        }
        حالت ps_STRIFY: {
            اگر (tok == VP_STRIFY) {
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
                state_برداشت();
                برو next_tok;
            } وگرنه اگر (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                بشکن;
            } وگرنه اگر (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                برو next_tok;
            } وگرنه اگر (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                بشکن;
            } وگرنه {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                برو next_tok;
            }
        }
        پیشفرض: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        انتخاب (tok) {
        حالت VP_INCLUDE:
            اگر (!m_off) {
                state_افزودن(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            برو next_tok;
        حالت VP_UNDEF: state_افزودن(ps_DEFNAME_UNDEF); برو next_tok;
        حالت VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_افزودن(ps_DEFNAME_DEFINE);
            برو next_tok;
        حالت VP_IFDEF: state_افزودن(ps_DEFNAME_IFDEF); برو next_tok;
        حالت VP_IFNDEF: state_افزودن(ps_DEFNAME_IFNDEF); برو next_tok;
        حالت VP_ELSIF: state_افزودن(ps_DEFNAME_ELSIF); برو next_tok;
        حالت VP_ELSE:
            اگر (m_ifdef_پشته.empty()) {
                fileline()->v3error("`else with no matching `if");
            } وگرنه {
                ثابت VPreIfEntry lastIf = m_ifdef_پشته.بالاترین();
                m_ifdef_پشته.برداشت();
                ثابت منطقی enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_پشته.emplace(VPreIfEntry{enable, lastIf.everOn()});
                اگر (!lastIf.on()) parsingOn();
                اگر (!enable) parsingOff();
            }
            برو next_tok;
        حالت VP_ENDIF:
            UINFO(4, "Endif ");
            اگر (m_ifdef_پشته.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } وگرنه {
                ثابت VPreIfEntry lastIf = m_ifdef_پشته.بالاترین();
                m_ifdef_پشته.برداشت();
                اگر (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            برو next_tok;

        حالت VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ثابت string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            اگر (m_defPutJoin) {
                m_defPutJoin = نادرست;
                unputString("``");
            }
            اگر (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                برو next_tok;
            }
            // Substitute
            اگر (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                اگر (m_off) {
                    برو next_tok;
                } وگرنه {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    برو next_tok;
                }
            } وگرنه {
                ثابت string params = defParams(name);
                اگر (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    اگر (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    اگر (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        اگر (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_پشته.empty(), "`` join stack empty, but in a ``");
                            ثابت string lhs = m_join_پشته.بالاترین();
                            m_join_پشته.برداشت();
                            out.درج(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_برداشت();
                        }
                        اگر (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        وگرنه اگر (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } وگرنه {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ثابت refp = &(m_defRefs.بالاترین());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_مقدار + out);
                        m_lexp->m_def_مقدار = "";
                    }
                    برو next_tok;
                } وگرنه {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    اگر (!m_defRefs.empty()) m_defRefs.بالاترین().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_افزودن(ps_DEFPAREN);
                    m_lexp->افزودن_State_Def_Arg(0);
                    برو next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            برو next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        حالت VP_ERROR: {
            state_افزودن(ps_ERRORNAME);
            برو next_tok;
        }
        حالت VP_EOF:
            اگر (!m_ifdef_پشته.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            بازگشت tok;
        حالت VP_UNDEFINEALL:
            اگر (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            برو next_tok;
        حالت VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_افزودن(ps_STRIFY);
            برو next_tok;
        حالت VP_نماد:
        حالت VP_STRING:
        حالت VP_TEXT: {
            m_defDepth = 0;
            اگر (!m_off) {
                بازگشت tok;
            } وگرنه {
                برو next_tok;
            }
        }
        حالت VP_WHITE:  // Handled at top of loop
        حالت VP_COMMENT:  // Handled at top of loop
        حالت VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        حالت VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        پیشفرض:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            بشکن;  // LCOV_EXCL_LINE
        }
        بازگشت tok;
    }
}

صحیح V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    اگر (!m_finAhead) {
        m_finAhead = درست;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ثابت صحیح tok = m_finToken;
    buf = m_finBuf;
    اگر (نادرست && debug() >= 5) {
        ثابت string bufcln = V3PreLex::cleanDbgStrg(buf);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ثابت نویسه* bufp = buf.c_str();
    وقتی (*bufp == '\n') bufp++;
    اگر ((tok == VP_TEXT || tok == VP_سطر) && VString::startsWith(bufp, "`line ")) {
        صحیح enter;
        m_finFilelinep->سطر_Directive(bufp, enter /*ref*/);
    } وگرنه {
        اگر (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->سطر_Directives()) {
            اگر (ثابت صحیح outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                اگر (debug() >= 5) {
                    ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                اگر (outBehind > 0
                    && (outBehind <= static_cast<صحیح>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    اگر (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        بازگشت VP_TEXT;
                    }
                } وگرنه {
                    // Need to backup, use `line
                    buf = m_finFilelinep->سطر_Directive_Strg(0);
                    بازگشت VP_سطر;
                }
            }
        }
        // Track newlines in prep for next token
        برای (ثابت نویسه& c : buf) {
            اگر (c == '\n') {
                m_finAtBol = درست;
                m_finFilelinep->linenoInc();
            } وگرنه {
                m_finAtBol = نادرست;
            }
        }
    }
    m_finAhead = نادرست;  // Consumed the token
    بازگشت tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    اگر (isEof()) بازگشت "";
    ثابت نویسه* rtnp;
    منطقی gotEof = نادرست;
    وقتی (nullptr == (rtnp = std::strchr(m_سطر_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ثابت صحیح tok = getFinalToken(m_tokenBuf /*ref*/);
        اگر (debug() >= 5) {
            ثابت string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        اگر (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            اگر (m_سطر_Chars != "" && m_سطر_Chars[m_سطر_Chars.length() - 1] != '\n') {
                m_سطر_Chars.append("\n");
            }
            gotEof = درست;
        } وگرنه {
            m_سطر_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ثابت صحیح len = rtnp - m_سطر_Chars.c_str() + 1;
    string the_سطر(m_سطر_Chars, 0, len);
    m_سطر_Chars = m_سطر_Chars.erase(0, len);  // Remove returned characters
    اگر (debug() >= 4) {
        ثابت string lncln = V3PreLex::cleanDbgStrg(the_سطر);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    بازگشت the_سطر;
}
