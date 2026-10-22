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

#include "config_build.h"
#include "verilatedos.h"

#include "V3PreProc.h"

#include "V3Control.h"
#include "V3Error.h"
#include "V3File.h"
#include "V3Global.h"
#include "V3LanguageWords.h"
#include "V3PreExpr.h"
#include "V3PreLex.h"
#include "V3PreShell.h"
#include "V3String.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <stack>
#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#include "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

class VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    FileLine* const m_fileline;  // Where it was declared
    const string m_value;  // Value of define
    const string m_params;  // Parameters
    const bool m_cmdline;  // Set on command line, don't `undefineall
public:
    VDefine(FileLine* fl, const string& value, const string& params, bool cmdline)
        : m_fileline{fl}
        , m_value{value}
        , m_params{params}
        , m_cmdline{cmdline} {}
    FileLine* fileline() const { повернути m_fileline; }
    string value() const { повернути m_value; }
    string params() const { повернути m_params; }
    bool cmdline() const { повернути m_cmdline; }
};

//*************************************************************************

class VDefineRef final {
    // One for each pending define substitution
    const string m_name;  // Define last name being defined
    const string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    ціле m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
public:
    string name() const { повернути m_name; }
    string params() const { повернути m_params; }
    string nextarg() const { повернути m_nextarg; }
    void nextarg(const string& value) { m_nextarg = value; }
    ціле parenLevel() const { повернути m_parenLevel; }
    void parenLevel(ціле value) { m_parenLevel = value; }
    std::vector<string>& args() { повернути m_args; }
    VDefineRef(const string& name, const string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = default;
};

//*************************************************************************
/// Data for parsing on/off

class VPreIfEntry final {
    // One for each pending ifdef/ifndef
    const bool m_on;  // Current parse for this ifdef level is "on"
    const bool m_everOn;  // Some if term in elsif tree has been on
public:
    bool on() const { повернути m_on; }
    bool everOn() const { повернути m_everOn; }
    VPreIfEntry(bool on, bool everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = default;
};

//*************************************************************************
// Data for a preprocessor instantiation.

class V3PreProcImp final : public V3PreProc {
public:
    // TYPES
    using DefinesMap = std::map<const std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    const V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    ціле m_lastLineno = 0;  // Last line number (stall detection)
    ціле m_tokensOnLine = 0;  // Number of tokens on line (stall detection)

    enum ProcState : uint8_t {
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
    static const char* procStateName(ProcState s) {
        static const char* const states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        повернути states[s];
    }

    std::stack<ProcState> m_states;  ///< Current state of parser
    ціле m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    bool m_incError = false;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_lineCmt;  ///< Line comment(s) to be returned
    bool m_lineCmtNl = false;  ///< Newline needed before inserting lineCmt
    ціле m_lineAdd = 0;  ///< Empty lines to return to maintain line count
    bool m_rawAtBol = true;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    bool m_finAhead = false;  ///< Have read a token ahead
    ціле m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    bool m_finAtBol = true;  ///< Last getFinalToken left us at beginning of line
    FileLine* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::stack<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::stack<VPreIfEntry> m_ifdefStack;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    bool m_defPutJoin = false;  ///< Insert `` after substitution

    // For `` join
    std::stack<string> m_joinStack;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3PreExpr m_exprParser;  ///< Parser for () expression
    ціле m_exprParenLevel = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_lineChars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    void v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    static const char* tokenName(ціле tok);
    void debugToken(ціле tok, const char* cmtp);
    void parseTop();
    void parseUndef();

private:
    // Internal methods
    void endOfOneFile();
    string defineSubst(VDefineRef* refp);

    bool defExists(const string& name);
    bool defCmdline(const string& name);
    string defValue(const string& name);
    string defParams(const string& name);
    FileLine* defFileline(const string& name);

    string commentCleanup(const string& text);
    bool commentTokenMatch(string& cmdr, const char* strg);
    static string trimWhitespace(const string& strg, bool trailing);
    void unputString(const string& strg);
    void unputDefrefString(const string& strg);

    void parsingOn() {
        m_off--;
        якщо (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    void parsingOff() { m_off++; }

    ціле getRawToken();
    ціле getStateToken();
    ціле getFinalToken(string& buf);

    ProcState state() const { повернути m_states.top(); }
    bool stateIsDefname() const {
        повернути state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    void statePush(ProcState state) { m_states.push(state); }
    void statePop() {
        m_states.pop();
        якщо (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.push(ps_TOP);  // LCOV_EXCL_LINE
        }
    }
    void stateChange(ProcState state) {
        statePop();
        statePush(state);
    }

public:
    // METHODS, called from upper level shell
    void openFile(FileLine* fl, VInFilter* filterp, const string& filename) override;
    bool isEof() const override { повернути m_lexp->curStreamp()->m_eof; }
    void forceEof() { m_lexp->curStreamp()->m_eof = true; }
    string getline() override;
    void insertUnreadback(const string& text) override { m_lineCmt += text; }
    void insertUnreadbackAtBol(const string& text);
    void addLineComment(ціле enterExit);
    void dumpDefines(std::ostream& os) override;
    void candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    void comment(const string& text) override;  // Comment detected (if keepComments==2)
    void include(const string& filename) override;  // Request a include file be processed
    void undef(const string& name) override;
    virtual void undefineall();
    void define(FileLine* fl, const string& name, const string& value, const string& params,
                bool cmdline) override;
    string removeDefines(const string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.push(ps_TOP); }
    // cppcheck-suppress duplInheritedMember
    void configure(FileLine* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = this;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = new FileLine{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = new V3PreLex{this, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        якщо (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(FileLine* fl) {
    V3PreProcImp* const preprocp = new V3PreProcImp;
    preprocp->configure(fl);
    повернути preprocp;
}

void V3PreProc::selfTest() VL_MT_DISABLED { V3PreExpr::selfTest(); }

//*************************************************************************
// Defines

void V3PreProcImp::undef(const string& name) {
    якщо (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        insertUnreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
void V3PreProcImp::undefineall() {
    якщо (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        insertUnreadback("`undefineall\n");
    для (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        якщо (!it->second.cmdline()) m_defines.erase(it);
    }
}
bool V3PreProcImp::defExists(const string& name) {
    const auto iter = m_defines.find(name);
    повернути (iter != m_defines.end());
}
bool V3PreProcImp::defCmdline(const string& name) {
    const auto iter = m_defines.find(name);
    якщо (iter == m_defines.end()) { повернути false; }
    повернути iter->second.cmdline();
}
string V3PreProcImp::defValue(const string& name) {
    const auto iter = m_defines.find(name);
    якщо (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        повернути "";
    }
    повернути iter->second.value();
}
string V3PreProcImp::defParams(const string& name) {
    const auto iter = m_defines.find(name);
    якщо (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        повернути "";
    }
    повернути iter->second.params();
}
FileLine* V3PreProcImp::defFileline(const string& name) {
    const auto iter = m_defines.find(name);
    якщо (iter == m_defines.end()) повернути nullptr;
    повернути iter->second.fileline();
}
void V3PreProcImp::define(FileLine* fl, const string& name, const string& value,
                          const string& params, bool cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << value << "' params '" << params << "'");
    якщо (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(value.size());
        для (string::size_type pos = 0; pos < value.size(); ++pos) {
            якщо (value[pos] == '\n' && (pos == 0 || value[pos - 1] != '\\')) out += '\\';
            out += value[pos];
        }
        insertUnreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    якщо (!V3LanguageWords::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } інакше {
        якщо (defExists(name)) {
            якщо (defCmdline(name) && !cmdline) {
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
                повернути;
            } інакше {
                якщо (!(defValue(name) == value
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

string V3PreProcImp::removeDefines(const string& text) {
    string val;
    string rtnsym = text;
    для (ціле loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        якщо (xsym[0] == '`') xsym.erase(0, 1);
        якщо (defExists(xsym)) {
            val = defValue(xsym);
            якщо (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } інакше {
                break;
            }
        } інакше {
            break;
        }
    }
    повернути rtnsym;  // NA
}

//**********************************************************************
// Comments

void V3PreProcImp::include(const string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(const string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    поки ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    поки ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    поки ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    поки ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    поки ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    поки ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    поки (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    повернути cmd;
}

bool V3PreProcImp::commentTokenMatch(string& cmdr, const char* strg) {
    ціле len = std::strlen(strg);
    якщо (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        якщо (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        повернути true;
    } інакше {
        повернути false;
    }
}

void V3PreProcImp::comment(const string& text) {
    // Comment detected.  Only keep relevant data.
    bool printed = false;
    якщо (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        insertUnreadback(text);
        printed = true;
    }

    const char* cp = text.c_str();
    якщо (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } інакше {
        повернути;
    }

    поки (std::isspace(*cp)) ++cp;

    bool synth = false;
    bool vlcomment = false;
    якщо ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        якщо (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            повернути;
        }
        vlcomment = true;
    } інакше якщо (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        якщо (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            повернути;
        }
        synth = true;
    } інакше якщо (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = true;
    } інакше якщо (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = true;
    } інакше якщо (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = true;
    } інакше {
        повернути;
    }

    поки (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    якщо (synth) {
        якщо (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            якщо (commentTokenMatch(cmd /*ref*/, "full_case")) {
                якщо (!printed) insertUnreadback("/*verilator full_case*/");
            }
            якщо (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                якщо (!printed) insertUnreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } інакше якщо (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        якщо (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            поки (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            поки (std::isspace(arg[0])) arg = arg.substr(1);
            якщо (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            якщо (!printed) insertUnreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } інакше якщо (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            поки (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            якщо (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                повернути;
            }
            якщо (!printed) insertUnreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } інакше {
            якщо (!printed) insertUnreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

FileLine* V3PreProc::fileline() {
    const V3PreProcImp* idatap = static_cast<V3PreProcImp*>(this);
    повернути idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

const char* V3PreProcImp::tokenName(ціле tok) {
    switch (tok) {
    case VP_BACKQUOTE: повернути ("BACKQUOTE");
    case VP_COMMENT: повернути ("COMMENT");
    case VP_DEFARG: повернути ("DEFARG");
    case VP_DEFFORM: повернути ("DEFFORM");
    case VP_DEFINE: повернути ("DEFINE");
    case VP_DEFREF: повернути ("DEFREF");
    case VP_DEFREF_JOIN: повернути ("DEFREF_JOIN");
    case VP_DEFVALUE: повернути ("DEFVALUE");
    case VP_ELSE: повернути ("ELSE");
    case VP_ELSIF: повернути ("ELSIF");
    case VP_ENDIF: повернути ("ENDIF");
    case VP_EOF: повернути ("EOF");
    case VP_EOF_ERROR: повернути ("EOF_ERROR");
    case VP_ERROR: повернути ("ERROR");
    case VP_IFDEF: повернути ("IFDEF");
    case VP_IFNDEF: повернути ("IFNDEF");
    case VP_JOIN: повернути ("JOIN");
    case VP_INCLUDE: повернути ("INCLUDE");
    case VP_LINE: повернути ("LINE");
    case VP_STRIFY: повернути ("STRIFY");
    case VP_STRING: повернути ("STRING");
    case VP_SYMBOL: повернути ("SYMBOL");
    case VP_SYMBOL_JOIN: повернути ("SYMBOL_JOIN");
    case VP_TEXT: повернути ("TEXT");
    case VP_UNDEF: повернути ("UNDEF");
    case VP_UNDEFINEALL: повернути ("UNDEFINEALL");
    case VP_WHITE: повернути ("WHITE");
    default: повернути ("?");
    }
}

void V3PreProcImp::unputString(const string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    якщо (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

void V3PreProcImp::unputDefrefString(const string& strg) {
    const ціле multiline = std::count(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(const string& strg, bool trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    поки (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    якщо (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    якщо (trailing) {
        string::size_type trailspace = 0;
        поки (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        якщо (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        якщо (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    повернути out;
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
    для (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    const string value = defValue(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(value) << "'");

    std::map<const string, string> argValueByName;
    {  // Parse argument list into map
        unsigned numArgs = 0;
        string argName;
        ціле paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        bool quote = false;
        bool haveDefault = false;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        const string params = refp->params();  // Must keep str in scope to get pointer
        const char* cp = params.c_str();
        якщо (*cp == '(') cp++;
        для (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            якщо (!quote && paren == 1) {
                якщо (*cp == ')' || *cp == ',') {
                    string valueDef;
                    якщо (haveDefault) {
                        valueDef = token;
                    } інакше {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, true);
                    UINFO(4, "    Got Arg=" << numArgs << "  argName='" << argName
                                            << "'  default='" << valueDef << "'");
                    // Parse it
                    якщо (argName != "") {
                        якщо (refp->args().size() > numArgs) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            const string arg = trimWhitespace(refp->args()[numArgs], true);
                            якщо (arg != "") valueDef = arg;
                        } інакше якщо (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            повернути " `" + refp->name() + " ";
                        }
                        numArgs++;
                    }
                    argValueByName[argName] = valueDef;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = false;
                    continue;
                } інакше якщо (*cp == '=') {
                    haveDefault = true;
                    argName = token;
                    token = "";
                    continue;
                }
            }
            якщо (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                continue;
            }
            якщо (!quote) {
                якщо (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } інакше якщо (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            якщо (*cp == '"') quote = !quote;
            якщо (*cp) token += *cp;
        }
        якщо (refp->args().size() > numArgs
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && numArgs == 0
                 && trimWhitespace(refp->args()[0], false) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            повернути " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        bool quote = false;
        bool triquote = false;
        bool backslashesc = false;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        для (const char* cp = value.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            якщо (!quote && !triquote && *cp == '\\') {
                backslashesc = true;
            } інакше якщо (std::isspace(*cp)) {
                backslashesc = false;
            }
            якщо (!quote && !triquote) {
                якщо (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    continue;
                }
                якщо (argName != "") {
                    // Found a possible variable substitution
                    const auto iter = argValueByName.find(argName);
                    якщо (iter != argValueByName.end()) {
                        // Substitute
                        const string subst = iter->second;
                        якщо (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            якщо (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } інакше {
                            out += subst;
                        }
                    } інакше {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                якщо (cp[0] == '`' && cp[1] == '`') {
                    якщо (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } інакше {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    continue;
                } інакше якщо (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    continue;
                } інакше якщо (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    continue;
                } інакше якщо (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    continue;
                } інакше якщо (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    continue;
                }
            }
            якщо (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                continue;
            } інакше якщо (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                continue;
            }
            якщо (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                continue;
            }
            якщо (*cp == '"') quote = !quote;
            якщо (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    повернути out;
}

//**********************************************************************
// Parser routines

void V3PreProcImp::openFile(FileLine*, VInFilter* filterp, const string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    якщо (m_incError) повернути;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    const bool ok = filterp->readWholefile(filename, wholefile /*ref*/);
    якщо (!ok) {
        fileline()->v3error("File not found: " + filename);
        повернути;
    }

    якщо (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        якщо (m_lexp->m_streampStack.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = true;
            повернути;
        }
        // There's already a file active.  Push it to work on the new one.
        addLineComment(0);
    }

    // Save file contents for future error reporting
    FileLine* const flsp = new FileLine{filename};
    flsp->lineno(1);
    flsp->newContent();
    для (const string& i : wholefile) flsp->contentp()->pushText(i);

    // Save contents for V3Control --contents
    якщо (filename != V3Options::getStdPackagePath()) {
        bool containsVlt = false;
        для (const string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            якщо (i.find("`verilator_config") != string::npos) {
                containsVlt = true;
                break;
            }
        }
        якщо (!containsVlt) {
            для (const string& i : wholefile) V3Control::contentsPushText(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    addLineComment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    ціле eof_newline = 0;  // Number of characters following last newline
    ціле eof_lineno = 1;
    для (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        bool strip = false;
        const char* const sp = it->data();
        const char* const ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        для (const char* cp = sp; cp < ep; cp++) {
            якщо (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = true;
            } інакше якщо (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } інакше {
                ++eof_newline;
            }
        }
        якщо (strip) {
            string out;
            out.reserve(it->length());
            для (const char* cp = sp; cp < ep; cp++) {
                якщо (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    якщо (eof_newline) {
        FileLine* const fl = new FileLine{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

void V3PreProcImp::insertUnreadbackAtBol(const string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    якщо (m_lineCmt == "") {
        m_lineCmtNl = true;
    } інакше якщо (m_lineCmt[m_lineCmt.length() - 1] != '\n') {
        insertUnreadback("\n");
    }
    insertUnreadback(text);
}

void V3PreProcImp::addLineComment(ціле enterExit) {
    якщо (lineDirectives()) {
        insertUnreadbackAtBol(m_lexp->curFilelinep()->lineDirectiveStrg(enterExit));
    }
}

void V3PreProcImp::dumpDefines(std::ostream& os) {
    для (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        якщо (!it->second.params().empty()) os << it->second.params();
        якщо (!it->second.value().empty()) os << " " << it->second.value();
        os << '\n';
    }
}

void V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    для (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->pushCandidate("`"s + it->first);
    }
}

ціле V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    поки (true) {
    next_tok:
        якщо (m_lineAdd) {
            m_lineAdd--;
            m_rawAtBol = true;
            yyourtext("\n", 1);
            якщо (debug() >= 5) debugToken(VP_WHITE, "LNA");
            повернути VP_WHITE;
        }
        якщо (m_lineCmt != "") {
            // We have some `line directive or other processed data to return to the user.
            static string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_lineCmt;
            якщо (m_lineCmtNl) {
                якщо (!m_rawAtBol) s_rtncmt.insert(0, "\n");
                m_lineCmtNl = false;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_lineCmt = "";
            якщо (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            якщо (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->appendDefValue(yyourtext(), yyourleng());
                goto next_tok;
            } інакше {
                якщо (debug() >= 5) debugToken(VP_TEXT, "LCM");
                повернути VP_TEXT;
            }
        }
        якщо (isEof()) повернути VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        ціле tok = m_lexp->lex();
        якщо (debug() >= 5) debugToken(tok, "RAW");

        якщо (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokensOnLine = 0;
        } інакше якщо (++m_tokensOnLine > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        якщо (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            якщо (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            goto next_tok;
        }

        якщо (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        повернути tok;
    }
}

void V3PreProcImp::debugToken(ціле tok, const char* cmtp) {
    const static ціле s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    якщо (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        поки ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        поки ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        const string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<ціле>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        якщо (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

ціле V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    поки (true) {
    next_tok:
        якщо (isEof()) повернути VP_EOF;
        ціле tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        якщо (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) повернути tok;
        якщо (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        якщо (tok == VP_COMMENT) {
            якщо (!m_off) {
                якщо (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    якщо (m_lineCmt.empty()) insertUnreadback(" ");
                } інакше якщо (m_lexp->m_keepComments) {
                    повернути tok;
                } інакше {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    insertUnreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            для (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                якщо (yyourtext()[len] == '\n') m_lineAdd++;
            }
            goto next_tok;
        }
        якщо (tok == VP_LINE) {
            addLineComment(m_lexp->m_enterExit);
            goto next_tok;
        }

        якщо (tok == VP_DEFREF_JOIN) {
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
            const std::string name(yyourtext() + 1, yyourleng() - 1);
            якщо (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = true;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } інакше {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        якщо (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            якщо (tok == VP_JOIN) goto next_tok;
            якщо (tok == VP_SYMBOL_JOIN) tok = VP_SYMBOL;
        }
        якщо (tok == VP_SYMBOL_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            const std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_joinStack.push(out);
            statePush(ps_JOIN);
            goto next_tok;
        }

        // Deal with some special parser states
        switch (state()) {
        case ps_TOP: {
            break;
        }
        case ps_DEFNAME_UNDEF:  // FALLTHRU
        case ps_DEFNAME_DEFINE:  // FALLTHRU
        case ps_DEFNAME_IFDEF:  // FALLTHRU
        case ps_DEFNAME_IFNDEF:  // FALLTHRU
        case ps_DEFNAME_ELSIF: {
            якщо (tok == VP_SYMBOL) {
                m_lastSym.assign(yyourtext(), yyourleng());
                якщо (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    bool enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    якщо (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdefStack.emplace(VPreIfEntry{enable, false});
                    якщо (!enable) parsingOff();
                    statePop();
                    goto next_tok;
                } інакше якщо (state() == ps_DEFNAME_ELSIF) {
                    якщо (m_ifdefStack.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } інакше {
                        // Handle `else portion
                        const VPreIfEntry lastIf = m_ifdefStack.top();
                        m_ifdefStack.pop();
                        якщо (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        const bool enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdefStack.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        якщо (!enable) parsingOff();
                    }
                    statePop();
                    goto next_tok;
                } інакше якщо (state() == ps_DEFNAME_UNDEF) {
                    якщо (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    statePop();
                    goto next_tok;
                } інакше якщо (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->pushStateDefForm();
                    goto next_tok;
                } інакше {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                goto next_tok;
            } інакше якщо (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                якщо (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->pushStateExpr();
                    m_exprParser.reset(fileline());
                    m_exprParenLevel = 1;
                    switch (state()) {
                    case ps_DEFNAME_IFDEF: stateChange(ps_EXPR_IFDEF); break;
                    case ps_DEFNAME_IFNDEF: stateChange(ps_EXPR_IFNDEF); break;
                    case ps_DEFNAME_ELSIF: stateChange(ps_EXPR_ELSIF); break;
                    default: v3fatalSrc("bad case");
                    }
                    goto next_tok;
                }
                якщо (!m_off) {
                    повернути tok;
                } інакше {
                    goto next_tok;
                }
            } інакше якщо (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                break;
            } інакше {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                goto next_tok;
            }
        }
        case ps_EXPR_IFDEF:  // FALLTHRU
        case ps_EXPR_IFNDEF:  // FALLTHRU
        case ps_EXPR_ELSIF: {
            // `ifdef ( *here*
            FileLine* const flp = m_lexp->m_tokFilelinep;
            якщо (tok == VP_SYMBOL) {
                m_lastSym.assign(yyourtext(), yyourleng());
                const bool exists = defExists(m_lastSym);
                якщо (exists) {
                    const std::string value = defValue(m_lastSym);
                    якщо (VString::removeWhitespace(value) == "0") {
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
                goto next_tok;
            } інакше якщо (tok == VP_WHITE) {
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::BRA});
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_exprParenLevel, "Underflow of ); should have exited ps_EXPR earlier?");
                якщо (--m_exprParenLevel > 0) {
                    m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::KET});
                    goto next_tok;
                } інакше {
                    // Done with parsing expression
                    bool enable = m_exprParser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    якщо (state() == ps_EXPR_IFDEF || state() == ps_EXPR_IFNDEF) {
                        якщо (state() == ps_EXPR_IFNDEF) enable = !enable;
                        m_ifdefStack.emplace(VPreIfEntry{enable, false});
                        якщо (!enable) parsingOff();
                        statePop();
                        goto next_tok;
                    } інакше якщо (state() == ps_EXPR_ELSIF) {
                        якщо (m_ifdefStack.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } інакше {
                            // Handle `else portion
                            const VPreIfEntry lastIf = m_ifdefStack.top();
                            m_ifdefStack.pop();
                            якщо (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdefStack.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            якщо (!enable) parsingOff();
                        }
                        statePop();
                    }
                    goto next_tok;
                }
            } інакше якщо (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::LNOT});
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::LAND});
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::LOR});
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::IMP});
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_exprParser.pushInput(V3PreExprToken{flp, V3PreExprToken::EQV});
                goto next_tok;
            } інакше {
                якщо (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    повернути tok;
                } інакше {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                goto next_tok;
            }
        }
        case ps_DEFFORM: {
            якщо (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_defValue;
                якщо (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->pushStateDefValue();
                goto next_tok;
            } інакше якщо (tok == VP_TEXT) {
                // IE, something like comment in formals
                якщо (!m_off) повернути tok;
                goto next_tok;
            } інакше {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                goto next_tok;
            }
        }
        case ps_DEFVALUE: {
            static string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            якщо (tok == VP_DEFVALUE) {
                якщо (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_defValue)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                const string formals = m_formals;
                string value = m_lexp->m_defValue;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                для (size_t i = 0; i < formals.length(); i++) {
                    якщо (formals[i] == '\n') s_newlines += "\n";
                }
                для (size_t i = 0; i < value.length(); i++) {
                    якщо (value[i] == '\n') s_newlines += "\n";
                }
                якщо (!m_off) {
                    // Remove leading and trailing whitespace
                    value = trimWhitespace(value, true);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(value) << "'");
                    define(fileline(), m_lastSym, value, formals, false);
                }
            } інакше {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            statePop();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            повернути VP_WHITE;
        }
        case ps_DEFPAREN: {
            якщо (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                goto next_tok;
            } інакше {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                const VDefineRef* const refp = &(m_defRefs.top());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                statePop();
                goto next_tok;
            }
        }
        case ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.top());
            refp->nextarg(refp->nextarg() + m_lexp->m_defValue);
            m_lexp->m_defValue = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            якщо (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().push_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->pushStateDefArg(1);
                refp->nextarg("");
                goto next_tok;
            } інакше якщо (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().push_back(refp->nextarg());
                string out;
                якщо (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.pop(), refp);
                statePop();
                якщо (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    якщо (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_joinStack.empty(), "`` join stack empty, but in a ``");
                        const string lhs = m_joinStack.top();
                        m_joinStack.pop();
                        out.insert(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        statePop();
                    }
                    якщо (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    інакше якщо (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.top().parenLevel();
                } інакше {  // Finished a defref inside a upper defref,
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
                goto next_tok;
            } інакше якщо (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                break;
            } інакше якщо (tok == VP_SYMBOL || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                goto next_tok;
            } інакше якщо (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                statePush(ps_STRIFY);
                goto next_tok;
            } інакше {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                statePop();
                goto next_tok;
            }
        }
        case ps_INCNAME: {
            якщо (tok == VP_STRING) {
                statePop();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                goto next_tok;
            } інакше якщо (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->pushStateIncFilename();
                goto next_tok;
            } інакше якщо (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                break;
            } інакше {
                statePop();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                goto next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        case ps_ERRORNAME: {
            якщо (tok == VP_STRING) {
                якщо (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                statePop();
                goto next_tok;
            } інакше {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                statePop();
                goto next_tok;
            }
        }
        case ps_JOIN: {
            якщо (tok == VP_SYMBOL || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_joinStack.empty(), "`` join stack empty, but in a ``");
                const string lhs = m_joinStack.top();
                m_joinStack.pop();
                UINFO(5, "`` LHS:" << lhs);
                const string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                const string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                statePop();
                goto next_tok;
            } інакше якщо (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                const string lhs = m_joinStack.top();
                m_joinStack.pop();
                unputString(lhs);
                statePop();
                goto next_tok;
            } інакше {
                // `define, etc, fall through and expand.  Pop back here.
                break;
            }
        }
        case ps_STRIFY: {
            якщо (tok == VP_STRIFY) {
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
                goto next_tok;
            } інакше якщо (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                break;
            } інакше якщо (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                goto next_tok;
            } інакше якщо (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                break;
            } інакше {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                goto next_tok;
            }
        }
        default: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        switch (tok) {
        case VP_INCLUDE:
            якщо (!m_off) {
                statePush(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            goto next_tok;
        case VP_UNDEF: statePush(ps_DEFNAME_UNDEF); goto next_tok;
        case VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            statePush(ps_DEFNAME_DEFINE);
            goto next_tok;
        case VP_IFDEF: statePush(ps_DEFNAME_IFDEF); goto next_tok;
        case VP_IFNDEF: statePush(ps_DEFNAME_IFNDEF); goto next_tok;
        case VP_ELSIF: statePush(ps_DEFNAME_ELSIF); goto next_tok;
        case VP_ELSE:
            якщо (m_ifdefStack.empty()) {
                fileline()->v3error("`else with no matching `if");
            } інакше {
                const VPreIfEntry lastIf = m_ifdefStack.top();
                m_ifdefStack.pop();
                const bool enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdefStack.emplace(VPreIfEntry{enable, lastIf.everOn()});
                якщо (!lastIf.on()) parsingOn();
                якщо (!enable) parsingOff();
            }
            goto next_tok;
        case VP_ENDIF:
            UINFO(4, "Endif ");
            якщо (m_ifdefStack.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } інакше {
                const VPreIfEntry lastIf = m_ifdefStack.top();
                m_ifdefStack.pop();
                якщо (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            goto next_tok;

        case VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            const string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            якщо (m_defPutJoin) {
                m_defPutJoin = false;
                unputString("``");
            }
            якщо (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                goto next_tok;
            }
            // Substitute
            якщо (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                якщо (m_off) {
                    goto next_tok;
                } інакше {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    goto next_tok;
                }
            } інакше {
                const string params = defParams(name);
                якщо (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    якщо (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    якщо (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        якщо (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_joinStack.empty(), "`` join stack empty, but in a ``");
                            const string lhs = m_joinStack.top();
                            m_joinStack.pop();
                            out.insert(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            statePop();
                        }
                        якщо (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        інакше якщо (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } інакше {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* const refp = &(m_defRefs.top());
                        refp->nextarg(refp->nextarg() + m_lexp->m_defValue + out);
                        m_lexp->m_defValue = "";
                    }
                    goto next_tok;
                } інакше {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    якщо (!m_defRefs.empty()) m_defRefs.top().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    statePush(ps_DEFPAREN);
                    m_lexp->pushStateDefArg(0);
                    goto next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            goto next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        case VP_ERROR: {
            statePush(ps_ERRORNAME);
            goto next_tok;
        }
        case VP_EOF:
            якщо (!m_ifdefStack.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            повернути tok;
        case VP_UNDEFINEALL:
            якщо (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            goto next_tok;
        case VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            statePush(ps_STRIFY);
            goto next_tok;
        case VP_SYMBOL:
        case VP_STRING:
        case VP_TEXT: {
            m_defDepth = 0;
            якщо (!m_off) {
                повернути tok;
            } інакше {
                goto next_tok;
            }
        }
        case VP_WHITE:  // Handled at top of loop
        case VP_COMMENT:  // Handled at top of loop
        case VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        case VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        default:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            break;  // LCOV_EXCL_LINE
        }
        повернути tok;
    }
}

ціле V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    якщо (!m_finAhead) {
        m_finAhead = true;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    const ціле tok = m_finToken;
    buf = m_finBuf;
    якщо (false && debug() >= 5) {
        const string bufcln = V3PreLex::cleanDbgStrg(buf);
        const string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    const char* bufp = buf.c_str();
    поки (*bufp == '\n') bufp++;
    якщо ((tok == VP_TEXT || tok == VP_LINE) && VString::startsWith(bufp, "`line ")) {
        ціле enter;
        m_finFilelinep->lineDirective(bufp, enter /*ref*/);
    } інакше {
        якщо (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->lineDirectives()) {
            якщо (const ціле outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                якщо (debug() >= 5) {
                    const string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                якщо (outBehind > 0
                    && (outBehind <= static_cast<ціле>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    якщо (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        повернути VP_TEXT;
                    }
                } інакше {
                    // Need to backup, use `line
                    buf = m_finFilelinep->lineDirectiveStrg(0);
                    повернути VP_LINE;
                }
            }
        }
        // Track newlines in prep for next token
        для (const char& c : buf) {
            якщо (c == '\n') {
                m_finAtBol = true;
                m_finFilelinep->linenoInc();
            } інакше {
                m_finAtBol = false;
            }
        }
    }
    m_finAhead = false;  // Consumed the token
    повернути tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    якщо (isEof()) повернути "";
    const char* rtnp;
    bool gotEof = false;
    поки (nullptr == (rtnp = std::strchr(m_lineChars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        const ціле tok = getFinalToken(m_tokenBuf /*ref*/);
        якщо (debug() >= 5) {
            const string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            const string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        якщо (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            якщо (m_lineChars != "" && m_lineChars[m_lineChars.length() - 1] != '\n') {
                m_lineChars.append("\n");
            }
            gotEof = true;
        } інакше {
            m_lineChars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    const ціле len = rtnp - m_lineChars.c_str() + 1;
    string theLine(m_lineChars, 0, len);
    m_lineChars = m_lineChars.erase(0, len);  // Remove returned characters
    якщо (debug() >= 4) {
        const string lncln = V3PreLex::cleanDbgStrg(theLine);
        const string flcol = m_lexp->m_tokFilelinep->asciiLineCol();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    повернути theLine;
}
