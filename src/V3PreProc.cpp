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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3PreProc.h"

#ضمن "V3Control.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3LanguageWords.h"
#ضمن "V3PreExpr.h"
#ضمن "V3PreLex.h"
#ضمن "V3PreShell.h"
#ضمن "V3String.h"

#ضمن <algorithm>
#ضمن <cstdlib>
#ضمن <fstream>
#ضمن <stack>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#ضمن "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

صنف VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    FileLine* ثابت m_fileline;  // Where it was declared
    ثابت string m_value;  // Value of define
    ثابت string m_params;  // Parameters
    ثابت منطقي m_cmdline;  // Set on command line, don't `undefineall
عام:
    VDefine(FileLine* fl, ثابت string& value, ثابت string& params, منطقي cmdline)
        : m_fileline{fl}
        , m_value{value}
        , m_params{params}
        , m_cmdline{cmdline} {}
    FileLine* fileline() ثابت { ܦܢܐ m_fileline; }
    string value() ثابت { ܦܢܐ m_value; }
    string params() ثابت { ܦܢܐ m_params; }
    منطقي cmdline() ثابت { ܦܢܐ m_cmdline; }
};

//*************************************************************************

صنف VDefineRef final {
    // One for each pending define substitution
    ثابت string m_name;  // Define last name being defined
    ثابت string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    صحيح m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
عام:
    string name() ثابت { ܦܢܐ m_name; }
    string params() ثابت { ܦܢܐ m_params; }
    string nextarg() ثابت { ܦܢܐ m_nextarg; }
    فراغ nextarg(ثابت string& value) { m_nextarg = value; }
    صحيح parenLevel() ثابت { ܦܢܐ m_parenLevel; }
    فراغ parenLevel(صحيح value) { m_parenLevel = value; }
    std::vector<string>& args() { ܦܢܐ m_args; }
    VDefineRef(ثابت string& name, ثابت string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = افتراضي;
};

//*************************************************************************
/// Data for parsing on/off

صنف VPreIfEntry final {
    // One for each pending ifdef/ifndef
    ثابت منطقي m_on;  // Current parse for this ifdef level is "on"
    ثابت منطقي m_everOn;  // Some if term in elsif tree has been on
عام:
    منطقي on() ثابت { ܦܢܐ m_on; }
    منطقي everOn() ثابت { ܦܢܐ m_everOn; }
    VPreIfEntry(منطقي on, منطقي everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = افتراضي;
};

//*************************************************************************
// Data for a preprocessor instantiation.

صنف V3PreProcImp final : عام V3PreProc {
عام:
    // TYPES
    using DefinesMap = std::map<ثابت std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    ثابت V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    صحيح m_lastLineno = 0;  // Last line number (stall detection)
    صحيح m_tokensOnLine = 0;  // Number of tokens on line (stall detection)

    تعداد ProcState : uint8_t {
        ps_TOP,
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
        ps_EXPR_IFDEF,
        ps_EXPR_IFNDEF,
        ps_EXPR_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ساكن ثابت حرف* procStateName(ProcState s) {
        ساكن ثابت حرف* ثابت states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        ܦܢܐ states[s];
    }

    std::stack<ProcState> m_states;  ///< Current state of parser
    صحيح m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    منطقي m_incError = ܕܓܠܐ;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_lineCmt;  ///< Line comment(s) to be returned
    منطقي m_lineCmtNl = ܕܓܠܐ;  ///< Newline needed before inserting lineCmt
    صحيح m_lineAdd = 0;  ///< Empty lines to return to maintain line count
    منطقي m_rawAtBol = ܫܪܪܐ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    منطقي m_finAhead = ܕܓܠܐ;  ///< Have read a token ahead
    صحيح m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    منطقي m_finAtBol = ܫܪܪܐ;  ///< Last getFinalToken left us at beginning of line
    FileLine* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::stack<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::stack<VPreIfEntry> m_ifdefStack;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    منطقي m_defPutJoin = ܕܓܠܐ;  ///< Insert `` after substitution

    // For `` join
    std::stack<string> m_joinStack;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3PreExpr m_exprParser;  ///< Parser for () expression
    صحيح m_exprParenLevel = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_lineChars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    فراغ v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    ساكن ثابت حرف* tokenName(صحيح tok);
    فراغ debugToken(صحيح tok, ثابت حرف* cmtp);
    فراغ parseTop();
    فراغ parseUndef();

خاص:
    // Internal methods
    فراغ endOfOneFile();
    string defineSubst(VDefineRef* refp);

    منطقي defExists(ثابت string& name);
    منطقي defCmdline(ثابت string& name);
    string defValue(ثابت string& name);
    string defParams(ثابت string& name);
    FileLine* defFileline(ثابت string& name);

    string commentCleanup(ثابت string& text);
    منطقي commentTokenMatch(string& cmdr, ثابت حرف* strg);
    ساكن string trimWhitespace(ثابت string& strg, منطقي trailing);
    فراغ unputString(ثابت string& strg);
    فراغ unputDefrefString(ثابت string& strg);

    فراغ parsingOn() {
        m_off--;
        ܐܢ (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    فراغ parsingOff() { m_off++; }

    صحيح getRawToken();
    صحيح getStateToken();
    صحيح getFinalToken(string& buf);

    ProcState state() ثابت { ܦܢܐ m_states.top(); }
    منطقي stateIsDefname() ثابت {
        ܦܢܐ state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    فراغ statePush(ProcState state) { m_states.push(state); }
    فراغ statePop() {
        m_states.pop();
        ܐܢ (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.push(ps_TOP);  // LCOV_EXCL_LINE
        }
    }
    فراغ stateChange(ProcState state) {
        statePop();
        statePush(state);
    }

عام:
    // METHODS, called from upper level shell
    فراغ openFile(FileLine* fl, VInFilter* filterp, ثابت string& filename) override;
    منطقي isEof() ثابت override { ܦܢܐ m_lexp->curStreamp()->m_eof; }
    فراغ forceEof() { m_lexp->curStreamp()->m_eof = ܫܪܪܐ; }
    string getline() override;
    فراغ insertUnreadback(ثابت string& text) override { m_lineCmt += text; }
    فراغ insertUnreadbackAtBol(ثابت string& text);
    فراغ addLineComment(صحيح enterExit);
    فراغ dumpDefines(std::ostream& os) override;
    فراغ candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    فراغ comment(ثابت string& text) override;  // Comment detected (if keepComments==2)
    فراغ include(ثابت string& filename) override;  // Request a include file be processed
    فراغ undef(ثابت string& name) override;
    virtual فراغ undefineall();
    فراغ define(FileLine* fl, ثابت string& name, ثابت string& value, ثابت string& params,
                منطقي cmdline) override;
    string removeDefines(ثابت string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.push(ps_TOP); }
    // cppcheck-suppress duplInheritedMember
    فراغ configure(FileLine* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = هذا;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = جديد FileLine{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = جديد V3PreLex{هذا, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        ܐܢ (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(FileLine* fl) {
    V3PreProcImp* ثابت preprocp = جديد V3PreProcImp;
    preprocp->configure(fl);
    ܦܢܐ preprocp;
}

فراغ V3PreProc::selfTest() VL_MT_DISABLED { V3PreExpr::selfTest(); }

//*************************************************************************
// Defines

فراغ V3PreProcImp::undef(ثابت string& name) {
    ܐܢ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        insertUnreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
فراغ V3PreProcImp::undefineall() {
    ܐܢ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        insertUnreadback("`undefineall\n");
    لكل (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        ܐܢ (!it->second.cmdline()) m_defines.erase(it);
    }
}
منطقي V3PreProcImp::defExists(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    ܦܢܐ (iter != m_defines.end());
}
منطقي V3PreProcImp::defCmdline(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    ܐܢ (iter == m_defines.end()) { ܦܢܐ ܕܓܠܐ; }
    ܦܢܐ iter->second.cmdline();
}
string V3PreProcImp::defValue(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    ܐܢ (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ܦܢܐ "";
    }
    ܦܢܐ iter->second.value();
}
string V3PreProcImp::defParams(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    ܐܢ (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        ܦܢܐ "";
    }
    ܦܢܐ iter->second.params();
}
FileLine* V3PreProcImp::defFileline(ثابت string& name) {
    ثابت auto iter = m_defines.find(name);
    ܐܢ (iter == m_defines.end()) ܦܢܐ nullptr;
    ܦܢܐ iter->second.fileline();
}
فراغ V3PreProcImp::define(FileLine* fl, ثابت string& name, ثابت string& value,
                          ثابت string& params, منطقي cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << value << "' params '" << params << "'");
    ܐܢ (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(value.size());
        لكل (string::size_type pos = 0; pos < value.size(); ++pos) {
            ܐܢ (value[pos] == '\n' && (pos == 0 || value[pos - 1] != '\\')) out += '\\';
            out += value[pos];
        }
        insertUnreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    ܐܢ (!V3LanguageWords::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } ܐܚܪܢܐ {
        ܐܢ (defExists(name)) {
            ܐܢ (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << value
                               << "' to existing command line define value: '" << defValue(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << defValue(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                ܦܢܐ;
            } ܐܚܪܢܐ {
                ܐܢ (!(defValue(name) == value
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << value
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << defValue(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, value, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(ثابت string& text) {
    string val;
    string rtnsym = text;
    لكل (صحيح loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        ܐܢ (xsym[0] == '`') xsym.erase(0, 1);
        ܐܢ (defExists(xsym)) {
            val = defValue(xsym);
            ܐܢ (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } ܐܚܪܢܐ {
                اكسر;
            }
        } ܐܚܪܢܐ {
            اكسر;
        }
    }
    ܦܢܐ rtnsym;  // NA
}

//**********************************************************************
// Comments

فراغ V3PreProcImp::include(ثابت string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ثابت string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    ܟܡܐ ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    ܟܡܐ ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    ܟܡܐ ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    ܟܡܐ ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    ܟܡܐ ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    ܟܡܐ ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    ܟܡܐ (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    ܦܢܐ cmd;
}

منطقي V3PreProcImp::commentTokenMatch(string& cmdr, ثابت حرف* strg) {
    صحيح len = std::strlen(strg);
    ܐܢ (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        ܐܢ (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        ܦܢܐ ܫܪܪܐ;
    } ܐܚܪܢܐ {
        ܦܢܐ ܕܓܠܐ;
    }
}

فراغ V3PreProcImp::comment(ثابت string& text) {
    // Comment detected.  Only keep relevant data.
    منطقي printed = ܕܓܠܐ;
    ܐܢ (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        insertUnreadback(text);
        printed = ܫܪܪܐ;
    }

    ثابت حرف* cp = text.c_str();
    ܐܢ (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } ܐܚܪܢܐ {
        ܦܢܐ;
    }

    ܟܡܐ (std::isspace(*cp)) ++cp;

    منطقي synth = ܕܓܠܐ;
    منطقي vlcomment = ܕܓܠܐ;
    ܐܢ ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        ܐܢ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            ܦܢܐ;
        }
        vlcomment = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        ܐܢ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            ܦܢܐ;
        }
        synth = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = ܫܪܪܐ;
    } ܐܚܪܢܐ {
        ܦܢܐ;
    }

    ܟܡܐ (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    ܐܢ (synth) {
        ܐܢ (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            ܐܢ (commentTokenMatch(cmd /*ref*/, "full_case")) {
                ܐܢ (!printed) insertUnreadback("/*verilator full_case*/");
            }
            ܐܢ (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                ܐܢ (!printed) insertUnreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } ܐܚܪܢܐ ܐܢ (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        ܐܢ (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            ܟܡܐ (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            ܟܡܐ (std::isspace(arg[0])) arg = arg.substr(1);
            ܐܢ (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            ܐܢ (!printed) insertUnreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } ܐܚܪܢܐ ܐܢ (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            ܟܡܐ (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            ܐܢ (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                ܦܢܐ;
            }
            ܐܢ (!printed) insertUnreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } ܐܚܪܢܐ {
            ܐܢ (!printed) insertUnreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

FileLine* V3PreProc::fileline() {
    ثابت V3PreProcImp* idatap = static_cast<V3PreProcImp*>(هذا);
    ܦܢܐ idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ثابت حرف* V3PreProcImp::tokenName(صحيح tok) {
    بدل (tok) {
    حالة VP_BACKQUOTE: ܦܢܐ ("BACKQUOTE");
    حالة VP_COMMENT: ܦܢܐ ("COMMENT");
    حالة VP_DEFARG: ܦܢܐ ("DEFARG");
    حالة VP_DEFFORM: ܦܢܐ ("DEFFORM");
    حالة VP_DEFINE: ܦܢܐ ("DEFINE");
    حالة VP_DEFREF: ܦܢܐ ("DEFREF");
    حالة VP_DEFREF_JOIN: ܦܢܐ ("DEFREF_JOIN");
    حالة VP_DEFVALUE: ܦܢܐ ("DEFVALUE");
    حالة VP_ELSE: ܦܢܐ ("ELSE");
    حالة VP_ELSIF: ܦܢܐ ("ELSIF");
    حالة VP_ENDIF: ܦܢܐ ("ENDIF");
    حالة VP_EOF: ܦܢܐ ("EOF");
    حالة VP_EOF_ERROR: ܦܢܐ ("EOF_ERROR");
    حالة VP_ERROR: ܦܢܐ ("ERROR");
    حالة VP_IFDEF: ܦܢܐ ("IFDEF");
    حالة VP_IFNDEF: ܦܢܐ ("IFNDEF");
    حالة VP_JOIN: ܦܢܐ ("JOIN");
    حالة VP_INCLUDE: ܦܢܐ ("INCLUDE");
    حالة VP_LINE: ܦܢܐ ("LINE");
    حالة VP_STRIFY: ܦܢܐ ("STRIFY");
    حالة VP_STRING: ܦܢܐ ("STRING");
    حالة VP_SYMBOL: ܦܢܐ ("SYMBOL");
    حالة VP_SYMBOL_JOIN: ܦܢܐ ("SYMBOL_JOIN");
    حالة VP_TEXT: ܦܢܐ ("TEXT");
    حالة VP_UNDEF: ܦܢܐ ("UNDEF");
    حالة VP_UNDEFINEALL: ܦܢܐ ("UNDEFINEALL");
    حالة VP_WHITE: ܦܢܐ ("WHITE");
    افتراضي: ܦܢܐ ("?");
    }
}

فراغ V3PreProcImp::unputString(ثابت string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    ܐܢ (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

فراغ V3PreProcImp::unputDefrefString(ثابت string& strg) {
    ثابت صحيح multiline = std::count(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ثابت string& strg, منطقي trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    ܟܡܐ (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    ܐܢ (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    ܐܢ (trailing) {
        string::size_type trailspace = 0;
        ܟܡܐ (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        ܐܢ (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        ܐܢ (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    ܦܢܐ out;
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
    لكل (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    ثابت string value = defValue(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(value) << "'");

    std::map<ثابت string, string> argValueByName;
    {  // Parse argument list into map
        unsigned numArgs = 0;
        string argName;
        صحيح paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        منطقي quote = ܕܓܠܐ;
        منطقي haveDefault = ܕܓܠܐ;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ثابت string params = refp->params();  // Must keep str in scope to get pointer
        ثابت حرف* cp = params.c_str();
        ܐܢ (*cp == '(') cp++;
        لكل (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            ܐܢ (!quote && paren == 1) {
                ܐܢ (*cp == ')' || *cp == ',') {
                    string valueDef;
                    ܐܢ (haveDefault) {
                        valueDef = token;
                    } ܐܚܪܢܐ {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, ܫܪܪܐ);
                    UINFO(4, "    Got Arg=" << numArgs << "  argName='" << argName
                                            << "'  default='" << valueDef << "'");
                    // Parse it
                    ܐܢ (argName != "") {
                        ܐܢ (refp->args().size() > numArgs) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ثابت string arg = trimWhitespace(refp->args()[numArgs], ܫܪܪܐ);
                            ܐܢ (arg != "") valueDef = arg;
                        } ܐܚܪܢܐ ܐܢ (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            ܦܢܐ " `" + refp->name() + " ";
                        }
                        numArgs++;
                    }
                    argValueByName[argName] = valueDef;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = ܕܓܠܐ;
                    استمر;
                } ܐܚܪܢܐ ܐܢ (*cp == '=') {
                    haveDefault = ܫܪܪܐ;
                    argName = token;
                    token = "";
                    استمر;
                }
            }
            ܐܢ (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                استمر;
            }
            ܐܢ (!quote) {
                ܐܢ (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } ܐܚܪܢܐ ܐܢ (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            ܐܢ (*cp == '"') quote = !quote;
            ܐܢ (*cp) token += *cp;
        }
        ܐܢ (refp->args().size() > numArgs
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && numArgs == 0
                 && trimWhitespace(refp->args()[0], ܕܓܠܐ) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            ܦܢܐ " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        منطقي quote = ܕܓܠܐ;
        منطقي triquote = ܕܓܠܐ;
        منطقي backslashesc = ܕܓܠܐ;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        لكل (ثابت حرف* cp = value.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            ܐܢ (!quote && !triquote && *cp == '\\') {
                backslashesc = ܫܪܪܐ;
            } ܐܚܪܢܐ ܐܢ (std::isspace(*cp)) {
                backslashesc = ܕܓܠܐ;
            }
            ܐܢ (!quote && !triquote) {
                ܐܢ (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    استمر;
                }
                ܐܢ (argName != "") {
                    // Found a possible variable substitution
                    ثابت auto iter = argValueByName.find(argName);
                    ܐܢ (iter != argValueByName.end()) {
                        // Substitute
                        ثابت string subst = iter->second;
                        ܐܢ (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            ܐܢ (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } ܐܚܪܢܐ {
                            out += subst;
                        }
                    } ܐܚܪܢܐ {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                ܐܢ (cp[0] == '`' && cp[1] == '`') {
                    ܐܢ (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } ܐܚܪܢܐ {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    استمر;
                } ܐܚܪܢܐ ܐܢ (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    استمر;
                } ܐܚܪܢܐ ܐܢ (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    استمر;
                } ܐܚܪܢܐ ܐܢ (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    استمر;
                } ܐܚܪܢܐ ܐܢ (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    استمر;
                }
            }
            ܐܢ (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                استمر;
            } ܐܚܪܢܐ ܐܢ (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                استمر;
            }
            ܐܢ (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                استمر;
            }
            ܐܢ (*cp == '"') quote = !quote;
            ܐܢ (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    ܦܢܐ out;
}

//**********************************************************************
// Parser routines

فراغ V3PreProcImp::openFile(FileLine*, VInFilter* filterp, ثابت string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    ܐܢ (m_incError) ܦܢܐ;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ثابت منطقي ok = filterp->readWholefile(filename, wholefile /*ref*/);
    ܐܢ (!ok) {
        fileline()->v3error("File not found: " + filename);
        ܦܢܐ;
    }

    ܐܢ (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        ܐܢ (m_lexp->m_streampStack.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = ܫܪܪܐ;
            ܦܢܐ;
        }
        // There's already a file active.  Push it to work on the new one.
        addLineComment(0);
    }

    // Save file contents for future error reporting
    FileLine* ثابت flsp = جديد FileLine{filename};
    flsp->lineno(1);
    flsp->newContent();
    لكل (ثابت string& i : wholefile) flsp->contentp()->pushText(i);

    // Save contents for V3Control --contents
    ܐܢ (filename != V3Options::getStdPackagePath()) {
        منطقي containsVlt = ܕܓܠܐ;
        لكل (ثابت string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            ܐܢ (i.find("`verilator_config") != string::npos) {
                containsVlt = ܫܪܪܐ;
                اكسر;
            }
        }
        ܐܢ (!containsVlt) {
            لكل (ثابت string& i : wholefile) V3Control::contentsPushText(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    addLineComment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    صحيح eof_newline = 0;  // Number of characters following last newline
    صحيح eof_lineno = 1;
    لكل (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        منطقي strip = ܕܓܠܐ;
        ثابت حرف* ثابت sp = it->data();
        ثابت حرف* ثابت ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        لكل (ثابت حرف* cp = sp; cp < ep; cp++) {
            ܐܢ (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = ܫܪܪܐ;
            } ܐܚܪܢܐ ܐܢ (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } ܐܚܪܢܐ {
                ++eof_newline;
            }
        }
        ܐܢ (strip) {
            string out;
            out.reserve(it->length());
            لكل (ثابت حرف* cp = sp; cp < ep; cp++) {
                ܐܢ (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    ܐܢ (eof_newline) {
        FileLine* ثابت fl = جديد FileLine{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

فراغ V3PreProcImp::insertUnreadbackAtBol(ثابت string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    ܐܢ (m_lineCmt == "") {
        m_lineCmtNl = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (m_lineCmt[m_lineCmt.length() - 1] != '\n') {
        insertUnreadback("\n");
    }
    insertUnreadback(text);
}

فراغ V3PreProcImp::addLineComment(صحيح enterExit) {
    ܐܢ (lineDirectives()) {
        insertUnreadbackAtBol(m_lexp->curFilelinep()->lineDirectiveStrg(enterExit));
    }
}

فراغ V3PreProcImp::dumpDefines(std::ostream& os) {
    لكل (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        ܐܢ (!it->second.params().empty()) os << it->second.params();
        ܐܢ (!it->second.value().empty()) os << " " << it->second.value();
        os << '\n';
    }
}

فراغ V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    لكل (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->pushCandidate("`"s + it->first);
    }
}

صحيح V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    ܟܡܐ (ܫܪܪܐ) {
    next_tok:
        ܐܢ (m_lineAdd) {
            m_lineAdd--;
            m_rawAtBol = ܫܪܪܐ;
            yyourtext("\n", 1);
            ܐܢ (debug() >= 5) debugToken(VP_WHITE, "LNA");
            ܦܢܐ VP_WHITE;
        }
        ܐܢ (m_lineCmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ساكن string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_lineCmt;
            ܐܢ (m_lineCmtNl) {
                ܐܢ (!m_rawAtBol) s_rtncmt.insert(0, "\n");
                m_lineCmtNl = ܕܓܠܐ;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_lineCmt = "";
            ܐܢ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            ܐܢ (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->appendDefValue(yyourtext(), yyourleng());
                اذهب next_tok;
            } ܐܚܪܢܐ {
                ܐܢ (debug() >= 5) debugToken(VP_TEXT, "LCM");
                ܦܢܐ VP_TEXT;
            }
        }
        ܐܢ (isEof()) ܦܢܐ VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        صحيح tok = m_lexp->lex();
        ܐܢ (debug() >= 5) debugToken(tok, "RAW");

        ܐܢ (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokensOnLine = 0;
        } ܐܚܪܢܐ ܐܢ (++m_tokensOnLine > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        ܐܢ (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            ܐܢ (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            اذهب next_tok;
        }

        ܐܢ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        ܦܢܐ tok;
    }
}

فراغ V3PreProcImp::debugToken(صحيح tok, ثابت حرف* cmtp) {
    ثابت ساكن صحيح s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    ܐܢ (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        ܟܡܐ ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        ܟܡܐ ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ثابت string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<صحيح>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        ܐܢ (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

صحيح V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    ܟܡܐ (ܫܪܪܐ) {
    next_tok:
        ܐܢ (isEof()) ܦܢܐ VP_EOF;
        صحيح tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        ܐܢ (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) ܦܢܐ tok;
        ܐܢ (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        ܐܢ (tok == VP_COMMENT) {
            ܐܢ (!m_off) {
                ܐܢ (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ܐܢ (m_lineCmt.empty()) insertUnreadback(" ");
                } ܐܚܪܢܐ ܐܢ (m_lexp->m_keepComments) {
                    ܦܢܐ tok;
                } ܐܚܪܢܐ {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    insertUnreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            لكل (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                ܐܢ (yyourtext()[len] == '\n') m_lineAdd++;
            }
            اذهب next_tok;
        }
        ܐܢ (tok == VP_LINE) {
            addLineComment(m_lexp->m_enterExit);
            اذهب next_tok;
        }

        ܐܢ (tok == VP_DEFREF_JOIN) {
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
            ܐܢ (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = ܫܪܪܐ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } ܐܚܪܢܐ {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        ܐܢ (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            ܐܢ (tok == VP_JOIN) اذهب next_tok;
            ܐܢ (tok == VP_SYMBOL_JOIN) tok = VP_SYMBOL;
        }
        ܐܢ (tok == VP_SYMBOL_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ثابت std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_joinStack.push(out);
            statePush(ps_JOIN);
            اذهب next_tok;
        }

        // Deal with some special parser states
        بدل (state()) {
        حالة ps_TOP: {
            اكسر;
        }
        حالة ps_DEFNAME_UNDEF:  // FALLTHRU
        حالة ps_DEFNAME_DEFINE:  // FALLTHRU
        حالة ps_DEFNAME_IFDEF:  // FALLTHRU
        حالة ps_DEFNAME_IFNDEF:  // FALLTHRU
        حالة ps_DEFNAME_ELSIF: {
            ܐܢ (tok == VP_SYMBOL) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ܐܢ (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    منطقي enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    ܐܢ (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdefStack.emplace(VPreIfEntry{enable, ܕܓܠܐ});
                    ܐܢ (!enable) parsingOff();
                    statePop();
                    اذهب next_tok;
                } ܐܚܪܢܐ ܐܢ (state() == ps_DEFNAME_ELSIF) {
                    ܐܢ (m_ifdefStack.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } ܐܚܪܢܐ {
                        // Handle `else portion
                        ثابت VPreIfEntry lastIf = m_ifdefStack.top();
                        m_ifdefStack.pop();
                        ܐܢ (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        ثابت منطقي enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdefStack.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        ܐܢ (!enable) parsingOff();
                    }
                    statePop();
                    اذهب next_tok;
                } ܐܚܪܢܐ ܐܢ (state() == ps_DEFNAME_UNDEF) {
                    ܐܢ (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    statePop();
                    اذهب next_tok;
                } ܐܚܪܢܐ ܐܢ (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->pushStateDefForm();
                    اذهب next_tok;
                } ܐܚܪܢܐ {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                ܐܢ (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->pushStateExpr();
                    m_exprParser.reset(fileline());
                    m_exprParenLevel = 1;
                    بدل (state()) {
                    حالة ps_DEFNAME_IFDEF: stateChange(ps_EXPR_IFDEF); اكسر;
                    حالة ps_DEFNAME_IFNDEF: stateChange(ps_EXPR_IFNDEF); اكسر;
                    حالة ps_DEFNAME_ELSIF: stateChange(ps_EXPR_ELSIF); اكسر;
                    افتراضي: v3fatalSrc("bad case");
                    }
                    اذهب next_tok;
                }
                ܐܢ (!m_off) {
                    ܦܢܐ tok;
                } ܐܚܪܢܐ {
                    اذهب next_tok;
                }
            } ܐܚܪܢܐ ܐܢ (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                اكسر;
            } ܐܚܪܢܐ {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                اذهب next_tok;
            }
        }
        حالة ps_EXPR_IFDEF:  // FALLTHRU
        حالة ps_EXPR_IFNDEF:  // FALLTHRU
        حالة ps_EXPR_ELSIF: {
            // `ifdef ( *here*
            FileLine* ثابت flp = m_lexp->m_tokFilelinep;
            ܐܢ (tok == VP_SYMBOL) {
                m_lastSym.assign(yyourtext(), yyourleng());
                ثابت منطقي exists = defExists(m_lastSym);
                ܐܢ (exists) {
                    ثابت std::string value = defValue(m_lastSym);
                    ܐܢ (VString::removeWhitespace(value) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << value
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_exprParser.pushInput(V3PreExprToken{flp, exists});
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_WHITE) {
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::BRA});
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_exprParenLevel, "Underflow of ); should have exited ps_EXPR earlier?");
                ܐܢ (--m_exprParenLevel > 0) {
                    m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::KET});
                    اذهب next_tok;
                } ܐܚܪܢܐ {
                    // Done with parsing expression
                    منطقي enable = m_exprParser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    ܐܢ (state() == ps_EXPR_IFDEF || state() == ps_EXPR_IFNDEF) {
                        ܐܢ (state() == ps_EXPR_IFNDEF) enable = !enable;
                        m_ifdefStack.emplace(VPreIfEntry{enable, ܕܓܠܐ});
                        ܐܢ (!enable) parsingOff();
                        statePop();
                        اذهب next_tok;
                    } ܐܚܪܢܐ ܐܢ (state() == ps_EXPR_ELSIF) {
                        ܐܢ (m_ifdefStack.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } ܐܚܪܢܐ {
                            // Handle `else portion
                            ثابت VPreIfEntry lastIf = m_ifdefStack.top();
                            m_ifdefStack.pop();
                            ܐܢ (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdefStack.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            ܐܢ (!enable) parsingOff();
                        }
                        statePop();
                    }
                    اذهب next_tok;
                }
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::LNOT});
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::LAND});
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::LOR});
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::IMP});
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::EQV});
                اذهب next_tok;
            } ܐܚܪܢܐ {
                ܐܢ (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    ܦܢܐ tok;
                } ܐܚܪܢܐ {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                اذهب next_tok;
            }
        }
        حالة ps_DEFFORM: {
            ܐܢ (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_defValue;
                ܐܢ (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->pushStateDefValue();
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT) {
                // IE, something like comment in formals
                ܐܢ (!m_off) ܦܢܐ tok;
                اذهب next_tok;
            } ܐܚܪܢܐ {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                اذهب next_tok;
            }
        }
        حالة ps_DEFVALUE: {
            ساكن string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            ܐܢ (tok == VP_DEFVALUE) {
                ܐܢ (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_defValue)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ثابت string formals = m_formals;
                string value = m_lexp->m_defValue;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                لكل (size_t i = 0; i < formals.length(); i++) {
                    ܐܢ (formals[i] == '\n') s_newlines += "\n";
                }
                لكل (size_t i = 0; i < value.length(); i++) {
                    ܐܢ (value[i] == '\n') s_newlines += "\n";
                }
                ܐܢ (!m_off) {
                    // Remove leading and trailing whitespace
                    value = trimWhitespace(value, ܫܪܪܐ);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(value) << "'");
                    define(fileline(), m_lastSym, value, formals, ܕܓܠܐ);
                }
            } ܐܚܪܢܐ {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            statePop();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            ܦܢܐ VP_WHITE;
        }
        حالة ps_DEFPAREN: {
            ܐܢ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                اذهب next_tok;
            } ܐܚܪܢܐ {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                ثابت VDefineRef* ثابت refp = &(m_defRefs.top());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                statePop();
                اذهب next_tok;
            }
        }
        حالة ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.top());
            refp->nextarg(refp->nextarg() + m_lexp->m_defValue);
            m_lexp->m_defValue = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            ܐܢ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().push_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->pushStateDefArg(1);
                refp->nextarg("");
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().push_back(refp->nextarg());
                string out;
                ܐܢ (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.pop(), refp);
                statePop();
                ܐܢ (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    ܐܢ (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_joinStack.empty(), "`` join stack empty, but in a ``");
                        ثابت string lhs = m_joinStack.top();
                        m_joinStack.pop();
                        out.insert(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        statePop();
                    }
                    ܐܢ (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    ܐܚܪܢܐ ܐܢ (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.top().parenLevel();
                } ܐܚܪܢܐ {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.top());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_defValue + out);
                    m_lexp->m_defValue = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                اكسر;
            } ܐܚܪܢܐ ܐܢ (tok == VP_SYMBOL || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                statePush(ps_STRIFY);
                اذهب next_tok;
            } ܐܚܪܢܐ {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                statePop();
                اذهب next_tok;
            }
        }
        حالة ps_INCNAME: {
            ܐܢ (tok == VP_STRING) {
                statePop();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->pushStateIncFilename();
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                اكسر;
            } ܐܚܪܢܐ {
                statePop();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                اذهب next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        حالة ps_ERRORNAME: {
            ܐܢ (tok == VP_STRING) {
                ܐܢ (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                statePop();
                اذهب next_tok;
            } ܐܚܪܢܐ {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                statePop();
                اذهب next_tok;
            }
        }
        حالة ps_JOIN: {
            ܐܢ (tok == VP_SYMBOL || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_joinStack.empty(), "`` join stack empty, but in a ``");
                ثابت string lhs = m_joinStack.top();
                m_joinStack.pop();
                UINFO(5, "`` LHS:" << lhs);
                ثابت string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ثابت string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                statePop();
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ثابت string lhs = m_joinStack.top();
                m_joinStack.pop();
                unputString(lhs);
                statePop();
                اذهب next_tok;
            } ܐܚܪܢܐ {
                // `define, etc, fall through and expand.  Pop back here.
                اكسر;
            }
        }
        حالة ps_STRIFY: {
            ܐܢ (tok == VP_STRIFY) {
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
                statePop();
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                اكسر;
            } ܐܚܪܢܐ ܐܢ (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                اذهب next_tok;
            } ܐܚܪܢܐ ܐܢ (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                اكسر;
            } ܐܚܪܢܐ {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                اذهب next_tok;
            }
        }
        افتراضي: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        بدل (tok) {
        حالة VP_INCLUDE:
            ܐܢ (!m_off) {
                statePush(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            اذهب next_tok;
        حالة VP_UNDEF: statePush(ps_DEFNAME_UNDEF); اذهب next_tok;
        حالة VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            statePush(ps_DEFNAME_DEFINE);
            اذهب next_tok;
        حالة VP_IFDEF: statePush(ps_DEFNAME_IFDEF); اذهب next_tok;
        حالة VP_IFNDEF: statePush(ps_DEFNAME_IFNDEF); اذهب next_tok;
        حالة VP_ELSIF: statePush(ps_DEFNAME_ELSIF); اذهب next_tok;
        حالة VP_ELSE:
            ܐܢ (m_ifdefStack.empty()) {
                fileline()->v3error("`else with no matching `if");
            } ܐܚܪܢܐ {
                ثابت VPreIfEntry lastIf = m_ifdefStack.top();
                m_ifdefStack.pop();
                ثابت منطقي enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdefStack.emplace(VPreIfEntry{enable, lastIf.everOn()});
                ܐܢ (!lastIf.on()) parsingOn();
                ܐܢ (!enable) parsingOff();
            }
            اذهب next_tok;
        حالة VP_ENDIF:
            UINFO(4, "Endif ");
            ܐܢ (m_ifdefStack.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } ܐܚܪܢܐ {
                ثابت VPreIfEntry lastIf = m_ifdefStack.top();
                m_ifdefStack.pop();
                ܐܢ (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            اذهب next_tok;

        حالة VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ثابت string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            ܐܢ (m_defPutJoin) {
                m_defPutJoin = ܕܓܠܐ;
                unputString("``");
            }
            ܐܢ (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                اذهب next_tok;
            }
            // Substitute
            ܐܢ (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                ܐܢ (m_off) {
                    اذهب next_tok;
                } ܐܚܪܢܐ {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    اذهب next_tok;
                }
            } ܐܚܪܢܐ {
                ثابت string params = defParams(name);
                ܐܢ (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    ܐܢ (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    ܐܢ (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        ܐܢ (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_joinStack.empty(), "`` join stack empty, but in a ``");
                            ثابت string lhs = m_joinStack.top();
                            m_joinStack.pop();
                            out.insert(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            statePop();
                        }
                        ܐܢ (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        ܐܚܪܢܐ ܐܢ (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } ܐܚܪܢܐ {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ثابت refp = &(m_defRefs.top());
                        refp->nextarg(refp->nextarg() + m_lexp->m_defValue + out);
                        m_lexp->m_defValue = "";
                    }
                    اذهب next_tok;
                } ܐܚܪܢܐ {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    ܐܢ (!m_defRefs.empty()) m_defRefs.top().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    statePush(ps_DEFPAREN);
                    m_lexp->pushStateDefArg(0);
                    اذهب next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            اذهب next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        حالة VP_ERROR: {
            statePush(ps_ERRORNAME);
            اذهب next_tok;
        }
        حالة VP_EOF:
            ܐܢ (!m_ifdefStack.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            ܦܢܐ tok;
        حالة VP_UNDEFINEALL:
            ܐܢ (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            اذهب next_tok;
        حالة VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            statePush(ps_STRIFY);
            اذهب next_tok;
        حالة VP_SYMBOL:
        حالة VP_STRING:
        حالة VP_TEXT: {
            m_defDepth = 0;
            ܐܢ (!m_off) {
                ܦܢܐ tok;
            } ܐܚܪܢܐ {
                اذهب next_tok;
            }
        }
        حالة VP_WHITE:  // Handled at top of loop
        حالة VP_COMMENT:  // Handled at top of loop
        حالة VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        حالة VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        افتراضي:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            اكسر;  // LCOV_EXCL_LINE
        }
        ܦܢܐ tok;
    }
}

صحيح V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    ܐܢ (!m_finAhead) {
        m_finAhead = ܫܪܪܐ;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    ثابت صحيح tok = m_finToken;
    buf = m_finBuf;
    ܐܢ (ܕܓܠܐ && debug() >= 5) {
        ثابت string bufcln = V3PreLex::cleanDbgStrg(buf);
        ثابت string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ثابت حرف* bufp = buf.c_str();
    ܟܡܐ (*bufp == '\n') bufp++;
    ܐܢ ((tok == VP_TEXT || tok == VP_LINE) && VString::startsWith(bufp, "`line ")) {
        صحيح enter;
        m_finFilelinep->lineDirective(bufp, enter /*ref*/);
    } ܐܚܪܢܐ {
        ܐܢ (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->lineDirectives()) {
            ܐܢ (ثابت صحيح outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                ܐܢ (debug() >= 5) {
                    ثابت string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                ܐܢ (outBehind > 0
                    && (outBehind <= static_cast<صحيح>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    ܐܢ (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        ܦܢܐ VP_TEXT;
                    }
                } ܐܚܪܢܐ {
                    // Need to backup, use `line
                    buf = m_finFilelinep->lineDirectiveStrg(0);
                    ܦܢܐ VP_LINE;
                }
            }
        }
        // Track newlines in prep for next token
        لكل (ثابت حرف& c : buf) {
            ܐܢ (c == '\n') {
                m_finAtBol = ܫܪܪܐ;
                m_finFilelinep->linenoInc();
            } ܐܚܪܢܐ {
                m_finAtBol = ܕܓܠܐ;
            }
        }
    }
    m_finAhead = ܕܓܠܐ;  // Consumed the token
    ܦܢܐ tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    ܐܢ (isEof()) ܦܢܐ "";
    ثابت حرف* rtnp;
    منطقي gotEof = ܕܓܠܐ;
    ܟܡܐ (nullptr == (rtnp = std::strchr(m_lineChars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ثابت صحيح tok = getFinalToken(m_tokenBuf /*ref*/);
        ܐܢ (debug() >= 5) {
            ثابت string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ثابت string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        ܐܢ (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            ܐܢ (m_lineChars != "" && m_lineChars[m_lineChars.length() - 1] != '\n') {
                m_lineChars.append("\n");
            }
            gotEof = ܫܪܪܐ;
        } ܐܚܪܢܐ {
            m_lineChars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ثابت صحيح len = rtnp - m_lineChars.c_str() + 1;
    string theLine(m_lineChars, 0, len);
    m_lineChars = m_lineChars.erase(0, len);  // Remove returned characters
    ܐܢ (debug() >= 4) {
        ثابت string lncln = V3PreLex::cleanDbgStrg(theLine);
        ثابت string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    ܦܢܐ theLine;
}
