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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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

#تعریف_کرو yyFlexLexer V3Lexer
#ضمن "V3PreLex.yy.cpp"
#تعریف_رد yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

صنف VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_سطر* ثابت m_fileline;  // Where it was declared
    ثابت string m_قدر;  // Value of define
    ثابت string m_params;  // Parameters
    ثابت منطقي m_cmdline;  // Set on command line, don't `undefineall
عام:
    VDefine(File_سطر* fl, ثابت string& قدر, ثابت string& params, منطقي cmdline)
        : m_fileline{fl}
        , m_قدر{قدر}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_سطر* fileline() ثابت { أرجع m_fileline; }
    string قدر() ثابت { أرجع m_قدر; }
    string params() ثابت { أرجع m_params; }
    منطقي cmdline() ثابت { أرجع m_cmdline; }
};

//*************************************************************************

صنف VDefineRef final {
    // One for each pending define substitution
    ثابت string m_name;  // Define last name being defined
    ثابت string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    صحيح m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::سمتیہ<string> m_دلیلاں;  // List of define arguments
عام:
    string name() ثابت { أرجع m_name; }
    string params() ثابت { أرجع m_params; }
    string nextarg() ثابت { أرجع m_nextarg; }
    فراغ nextarg(ثابت string& قدر) { m_nextarg = قدر; }
    صحيح parenLevel() ثابت { أرجع m_parenLevel; }
    فراغ parenLevel(صحيح قدر) { m_parenLevel = قدر; }
    std::سمتیہ<string>& دلیلاں() { أرجع m_دلیلاں; }
    VDefineRef(ثابت string& name, ثابت string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = افتراضي;
};

//*************************************************************************
/// Data for parsing on/off

صنف VPre_If_اندراج final {
    // One for each pending ifdef/ifndef
    ثابت منطقي m_on;  // Current parse for this ifdef level is "on"
    ثابت منطقي m_everOn;  // Some if term in elsif tree has been on
عام:
    منطقي on() ثابت { أرجع m_on; }
    منطقي everOn() ثابت { أرجع m_everOn; }
    VPre_If_اندراج(منطقي on, منطقي everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_اندراج() = افتراضي;
};

//*************************************************************************
// Data for a preprocessor instantiation.

صنف V3PreProcImp final : عام V3PreProc {
عام:
    // TYPES
    ورتوں مقرر_کردا_Map = std::map<ثابت std::string, VDefine>;
    ورتوں StrList = VInFilter::StrList;

    // Defines list
    مقرر_کردا_Map m_مقرر_کردا;  ///< Map of defines

    // STATE
    ثابت V3PreProc* m_preprocp = خالی_سوچک;  ///< Object we're holding data for
    V3PreLex* m_lexp = خالی_سوچک;  ///< Current lexer state (nullptr = closed)
    صحيح m_آخری_Lineno = 0;  // Last line number (stall detection)
    صحيح m_ٹوکن_On_سطر = 0;  // Number of tokens on line (stall detection)

    تعداد ProcState : uint8_t {
        ps_چوٹی,
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
        ps_اظہاریہ_IFDEF,
        ps_اظہاریہ_IFNDEF,
        ps_اظہاریہ_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ساكن ثابت حرف* procStateName(ProcState s) {
        ساكن ثابت حرف* ثابت حالتاں[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        أرجع حالتاں[s];
    }

    std::ڈھیر<ProcState> m_حالتاں;  ///< Current state of parser
    صحيح m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    منطقي m_incError = خطأ;  ///< Include error found
    string m_آخری_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_سطر_Cmt;  ///< Line comment(s) to be returned
    منطقي m_سطر_Cmt_Nl = خطأ;  ///< Newline needed before inserting lineCmt
    صحيح m_سطر_جوڑو = 0;  ///< Empty lines to return to maintain line count
    منطقي m_rawAtBol = سچ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    منطقي m_finAhead = خطأ;  ///< Have read a token ahead
    صحيح m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    منطقي m_finAtBol = سچ;  ///< Last getFinalToken left us at beginning of line
    File_سطر* m_finFilelinep = خالی_سوچک;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ڈھیر<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ڈھیر<VPre_If_اندراج> m_ifdef_ڈھیر;  ///< Stack of true/false emitting evaluations
    بنانشانی m_def_ڈونگھائی = 0;  ///< How many `defines deep
    منطقي m_defPutJoin = خطأ;  ///< Insert `` after substitution

    // For `` join
    std::ڈھیر<string> m_join_ڈھیر;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_اظہاریہ m_اظہاریہ_Parser;  ///< Parser for () expression
    صحيح m_اظہاریہ_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_سطر_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    فراغ v3error_سرا(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_سرا(str);
    }

    ساكن ثابت حرف* tokenName(صحيح tok);
    فراغ debugToken(صحيح tok, ثابت حرف* cmtp);
    فراغ parse_چوٹی();
    فراغ parseUndef();

خاص:
    // Internal methods
    فراغ سرا_دا_One_File();
    string defineSubst(VDefineRef* refp);

    منطقي defExists(ثابت string& name);
    منطقي defCmdline(ثابت string& name);
    string def_قدر(ثابت string& name);
    string defParams(ثابت string& name);
    File_سطر* defFileline(ثابت string& name);

    string commentCleanup(ثابت string& متن);
    منطقي comment_Token_میل(string& cmdr, ثابت حرف* strg);
    ساكن string trimWhitespace(ثابت string& strg, منطقي trailing);
    فراغ unputString(ثابت string& strg);
    فراغ unputDefrefString(ثابت string& strg);

    فراغ parsingOn() {
        m_off--;
        إذا (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    فراغ parsingOff() { m_off++; }

    صحيح getRawToken();
    صحيح getStateToken();
    صحيح getFinalToken(string& buf);

    ProcState state() ثابت { أرجع m_حالتاں.چوٹی(); }
    منطقي stateIsDefname() ثابت {
        أرجع state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    فراغ state_ڈالو(ProcState state) { m_حالتاں.ڈالو(state); }
    فراغ state_نکالو() {
        m_حالتاں.نکالو();
        إذا (VL_UNCOVERABLE(m_حالتاں.خالی_تھاواں())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_حالتاں.ڈالو(ps_چوٹی);  // LCOV_EXCL_LINE
        }
    }
    فراغ stateChange(ProcState state) {
        state_نکالو();
        state_ڈالو(state);
    }

عام:
    // METHODS, called from upper level shell
    فراغ openFile(File_سطر* fl, VInFilter* filterp, ثابت string& filename) override;
    منطقي isEof() ثابت override { أرجع m_lexp->curStreamp()->m_eof; }
    فراغ forceEof() { m_lexp->curStreamp()->m_eof = سچ; }
    string getline() override;
    فراغ داخل_کرو_Unreadback(ثابت string& متن) override { m_سطر_Cmt += متن; }
    فراغ داخل_کرو_Unreadback_At_Bol(ثابت string& متن);
    فراغ جوڑو_سطر_Comment(صحيح enterExit);
    فراغ dump_مقرر_کردا(std::ostream& os) override;
    فراغ candidate_مقرر_کردا(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    فراغ comment(ثابت string& متن) override;  // Comment detected (if keepComments==2)
    فراغ include(ثابت string& filename) override;  // Request a include file be processed
    فراغ undef(ثابت string& name) override;
    مجازی فراغ undefineall();
    فراغ define(File_سطر* fl, ثابت string& name, ثابت string& قدر, ثابت string& params,
                منطقي cmdline) override;
    string remove_مقرر_کردا(ثابت string& متن) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_حالتاں.ڈالو(ps_چوٹی); }
    // cppcheck-suppress duplInheritedMember
    فراغ configure(File_سطر* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = هذا;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = جديد File_سطر{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = جديد V3PreLex{هذا, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        إذا (m_lexp) VL_DO_CLEAR(مٹاؤ m_lexp, m_lexp = خالی_سوچک);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_سطر* fl) {
    V3PreProcImp* ثابت preprocp = جديد V3PreProcImp;
    preprocp->configure(fl);
    أرجع preprocp;
}

فراغ V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_اظہاریہ::selfTest(); }

//*************************************************************************
//مقرر_کردا

فراغ V3PreProcImp::undef(ثابت string& name) {
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preproc_مقرر_کردا())
        داخل_کرو_Unreadback("`undef " + name + "\n");
    m_مقرر_کردا.erase(name);
}
فراغ V3PreProcImp::undefineall() {
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preproc_مقرر_کردا())
        داخل_کرو_Unreadback("`undefineall\n");
    لكل (مقرر_کردا_Map::iterator nextit, it = m_مقرر_کردا.begin(); it != m_مقرر_کردا.سرا(); it = nextit) {
        nextit = it;
        ++nextit;
        إذا (!it->دوجا.cmdline()) m_مقرر_کردا.erase(it);
    }
}
منطقي V3PreProcImp::defExists(ثابت string& name) {
    ثابت خودکار iter = m_مقرر_کردا.لبھو(name);
    أرجع (iter != m_مقرر_کردا.سرا());
}
منطقي V3PreProcImp::defCmdline(ثابت string& name) {
    ثابت خودکار iter = m_مقرر_کردا.لبھو(name);
    إذا (iter == m_مقرر_کردا.سرا()) { أرجع خطأ; }
    أرجع iter->دوجا.cmdline();
}
string V3PreProcImp::def_قدر(ثابت string& name) {
    ثابت خودکار iter = m_مقرر_کردا.لبھو(name);
    إذا (iter == m_مقرر_کردا.سرا()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        أرجع "";
    }
    أرجع iter->دوجا.قدر();
}
string V3PreProcImp::defParams(ثابت string& name) {
    ثابت خودکار iter = m_مقرر_کردا.لبھو(name);
    إذا (iter == m_مقرر_کردا.سرا()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        أرجع "";
    }
    أرجع iter->دوجا.params();
}
File_سطر* V3PreProcImp::defFileline(ثابت string& name) {
    ثابت خودکار iter = m_مقرر_کردا.لبھو(name);
    إذا (iter == m_مقرر_کردا.سرا()) أرجع خالی_سوچک;
    أرجع iter->دوجا.fileline();
}
فراغ V3PreProcImp::define(File_سطر* fl, ثابت string& name, ثابت string& قدر,
                          ثابت string& params, منطقي cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << قدر << "' params '" << params << "'");
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preproc_مقرر_کردا()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string باہرلا;
        باہرلا.reserve(قدر.ناپ());
        لكل (string::ناپ_type pos = 0; pos < قدر.ناپ(); ++pos) {
            إذا (قدر[pos] == '\n' && (pos == 0 || قدر[pos - 1] != '\\')) باہرلا += '\\';
            باہرلا += قدر[pos];
        }
        داخل_کرو_Unreadback("`define " + name + params + (باہرلا.خالی_تھاواں() ? "" : " ") + باہرلا + "\n");
    }
    إذا (!V3_Language_الفاظ::isKeyword("`"s + name).خالی_تھاواں()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } وإلا {
        إذا (defExists(name)) {
            إذا (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << قدر
                               << "' to existing command line define value: '" << def_قدر(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_قدر(name) << (defParams(name).خالی_تھاواں() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                أرجع;
            } وإلا {
                إذا (!(def_قدر(name) == قدر
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << قدر
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_قدر(name) << (defParams(name).خالی_تھاواں() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_مقرر_کردا.emplace(name, VDefine{fl, قدر, params, cmdline});
    }
}

string V3PreProcImp::remove_مقرر_کردا(ثابت string& متن) {
    string val;
    string rtnsym = متن;
    لكل (صحيح loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        إذا (xsym[0] == '`') xsym.erase(0, 1);
        إذا (defExists(xsym)) {
            val = def_قدر(xsym);
            إذا (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } وإلا {
                اكسر;
            }
        } وإلا {
            اكسر;
        }
    }
    أرجع rtnsym;  // NA
}

//**********************************************************************
// Comments

فراغ V3PreProcImp::include(ثابت string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ثابت string& متن) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = متن;
    string::ناپ_type pos;
    طالما ((pos = cmd.لبھو("//")) != string::npos) cmd.replace(pos, 2, "");
    طالما ((pos = cmd.لبھو("/*")) != string::npos) cmd.replace(pos, 2, "");
    طالما ((pos = cmd.لبھو("*/")) != string::npos) cmd.replace(pos, 2, "");
    طالما ((pos = cmd.لبھو('\"')) != string::npos) cmd.replace(pos, 1, " ");
    طالما ((pos = cmd.لبھو('\t')) != string::npos) cmd.replace(pos, 1, " ");
    طالما ((pos = cmd.لبھو("  ")) != string::npos) cmd.replace(pos, 2, " ");
    طالما (!cmd.خالی_تھاواں() && std::خلا_اے(cmd[cmd.ناپ() - 1])) cmd.erase(cmd.ناپ() - 1);
    أرجع cmd;
}

منطقي V3PreProcImp::comment_Token_میل(string& cmdr, ثابت حرف* strg) {
    صحيح len = std::لڑی_لمبائی(strg);
    إذا (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::خلا_اے(cmdr[len]))) {
        إذا (std::خلا_اے(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        أرجع سچ;
    } وإلا {
        أرجع خطأ;
    }
}

فراغ V3PreProcImp::comment(ثابت string& متن) {
    // Comment detected.  Only keep relevant data.
    منطقي printed = خطأ;
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        داخل_کرو_Unreadback(متن);
        printed = سچ;
    }

    ثابت حرف* cp = متن.c_str();
    إذا (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } وإلا {
        أرجع;
    }

    طالما (std::خلا_اے(*cp)) ++cp;

    منطقي synth = خطأ;
    منطقي vlcomment = خطأ;
    إذا ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::لڑی_لمبائی("verilator");
        إذا (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            أرجع;
        }
        vlcomment = سچ;
    } وإلا إذا (VString::startsWith(cp, "synopsys")) {
        cp += std::لڑی_لمبائی("synopsys");
        إذا (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            أرجع;
        }
        synth = سچ;
    } وإلا إذا (VString::startsWith(cp, "cadence")) {
        cp += std::لڑی_لمبائی("cadence");
        synth = سچ;
    } وإلا إذا (VString::startsWith(cp, "pragma")) {
        cp += std::لڑی_لمبائی("pragma");
        synth = سچ;
    } وإلا إذا (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::لڑی_لمبائی("ambit synthesis");
        synth = سچ;
    } وإلا {
        أرجع;
    }

    طالما (std::خلا_اے(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    إذا (synth) {
        إذا (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            إذا (comment_Token_میل(cmd /*ref*/, "full_case")) {
                إذا (!printed) داخل_کرو_Unreadback("/*verilator full_case*/");
            }
            إذا (comment_Token_میل(cmd /*ref*/, "parallel_case")) {
                إذا (!printed) داخل_کرو_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } وإلا إذا (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.عوامی_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        إذا (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::ناپ_type سرا_دا_Cmd = std::لڑی_لمبائی("public_flat_rw");
            طالما (VString::isIdentifierChar(cmd[سرا_دا_Cmd])) ++سرا_دا_Cmd;
            string ادھار_Cmd = cmd.substr(0, سرا_دا_Cmd);
            string arg = cmd.substr(سرا_دا_Cmd);
            طالما (std::خلا_اے(arg[0])) arg = arg.substr(1);
            إذا (arg.ناپ() && ادھار_Cmd == "public_flat_rw_on")
                ادھار_Cmd += "_sns";  // different cmd for applying sensitivity
            إذا (!printed) داخل_کرو_Unreadback("/*verilator " + ادھار_Cmd + "*/ " + arg + " /**/");
        } وإلا إذا (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string ادھار_Cmd = cmd.substr(0, std::لڑی_لمبائی("dpi_c_decl"));
            string::ناپ_type شروع_دا_Arg = ادھار_Cmd.ناپ();
            طالما (std::خلا_اے(cmd[شروع_دا_Arg])) شروع_دا_Arg++;
            string arg = cmd.substr(شروع_دا_Arg);
            إذا (arg.خالی_تھاواں()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                أرجع;
            }
            إذا (!printed) داخل_کرو_Unreadback("/*verilator " + ادھار_Cmd + "*/ \"" + arg + "\" /**/");
        } وإلا {
            إذا (!printed) داخل_کرو_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_سطر* V3PreProc::fileline() {
    ثابت V3PreProcImp* idatap = static_cast<V3PreProcImp*>(هذا);
    أرجع idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ثابت حرف* V3PreProcImp::tokenName(صحيح tok) {
    بدل (tok) {
    حالة VP_BACKQUOTE: أرجع ("BACKQUOTE");
    حالة VP_COMMENT: أرجع ("COMMENT");
    حالة VP_DEFARG: أرجع ("DEFARG");
    حالة VP_DEFFORM: أرجع ("DEFFORM");
    حالة VP_DEFINE: أرجع ("DEFINE");
    حالة VP_DEFREF: أرجع ("DEFREF");
    حالة VP_DEFREF_JOIN: أرجع ("DEFREF_JOIN");
    حالة VP_DEFVALUE: أرجع ("DEFVALUE");
    حالة VP_ELSE: أرجع ("ELSE");
    حالة VP_ELSIF: أرجع ("ELSIF");
    حالة VP_ENDIF: أرجع ("ENDIF");
    حالة VP_EOF: أرجع ("EOF");
    حالة VP_EOF_ERROR: أرجع ("EOF_ERROR");
    حالة VP_ERROR: أرجع ("ERROR");
    حالة VP_IFDEF: أرجع ("IFDEF");
    حالة VP_IFNDEF: أرجع ("IFNDEF");
    حالة VP_JOIN: أرجع ("JOIN");
    حالة VP_INCLUDE: أرجع ("INCLUDE");
    حالة VP_سطر: أرجع ("LINE");
    حالة VP_STRIFY: أرجع ("STRIFY");
    حالة VP_STRING: أرجع ("STRING");
    حالة VP_علامت: أرجع ("SYMBOL");
    حالة VP_علامت_JOIN: أرجع ("SYMBOL_JOIN");
    حالة VP_متن: أرجع ("TEXT");
    حالة VP_UNDEF: أرجع ("UNDEF");
    حالة VP_UNDEFINEALL: أرجع ("UNDEFINEALL");
    حالة VP_WHITE: أرجع ("WHITE");
    افتراضي: أرجع ("?");
    }
}

فراغ V3PreProcImp::unputString(ثابت string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    إذا (VL_UNCOVERABLE(m_lexp->m_بفر_State != m_lexp->current_بفر())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->جانچ_بائٹاں(strg);
}

فراغ V3PreProcImp::unputDefrefString(ثابت string& strg) {
    ثابت صحيح multiline = std::گنتی(strg.begin(), strg.سرا(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ثابت string& strg, منطقي trailing) {
    // Remove leading whitespace
    string باہرلا = strg;
    string::ناپ_type leadspace = 0;
    طالما (باہرلا.لمائی() > leadspace && std::خلا_اے(باہرلا[leadspace])) ++leadspace;
    إذا (leadspace) باہرلا.erase(0, leadspace);
    // Remove trailing whitespace
    إذا (trailing) {
        string::ناپ_type trailspace = 0;
        طالما (باہرلا.لمائی() > trailspace && std::خلا_اے(باہرلا[باہرلا.لمائی() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        إذا (trailspace && باہرلا.لمائی() > trailspace && باہرلا[باہرلا.لمائی() - 1 - trailspace] == '\\')
            --trailspace;
        إذا (trailspace) باہرلا.erase(باہرلا.لمائی() - trailspace, trailspace);
    }
    أرجع باہرلا;
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
    لكل (بنانشانی i = 0; i < refp->دلیلاں().ناپ(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->دلیلاں()[i] << "'");
    }
    // Grab value
    ثابت string قدر = def_قدر(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(قدر) << "'");

    std::map<ثابت string, string> arg_قدر_By_Name;
    {  // Parse argument list into map
        بنانشانی عدد_دلیلاں = 0;
        string argName;
        صحيح paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        منطقي quote = خطأ;
        منطقي haveDefault = خطأ;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ثابت string params = refp->params();  // Must keep str in scope to get pointer
        ثابت حرف* cp = params.c_str();
        إذا (*cp == '(') cp++;
        لكل (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            إذا (!quote && paren == 1) {
                إذا (*cp == ')' || *cp == ',') {
                    string قدر_Def;
                    إذا (haveDefault) {
                        قدر_Def = token;
                    } وإلا {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, سچ);
                    UINFO(4, "    Got Arg=" << عدد_دلیلاں << "  argName='" << argName
                                            << "'  default='" << قدر_Def << "'");
                    // Parse it
                    إذا (argName != "") {
                        إذا (refp->دلیلاں().ناپ() > عدد_دلیلاں) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ثابت string arg = trimWhitespace(refp->دلیلاں()[عدد_دلیلاں], سچ);
                            إذا (arg != "") قدر_Def = arg;
                        } وإلا إذا (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            أرجع " `" + refp->name() + " ";
                        }
                        عدد_دلیلاں++;
                    }
                    arg_قدر_By_Name[argName] = قدر_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = خطأ;
                    استمر;
                } وإلا إذا (*cp == '=') {
                    haveDefault = سچ;
                    argName = token;
                    token = "";
                    استمر;
                }
            }
            إذا (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                استمر;
            }
            إذا (!quote) {
                إذا (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } وإلا إذا (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            إذا (*cp == '"') quote = !quote;
            إذا (*cp) token += *cp;
        }
        إذا (refp->دلیلاں().ناپ() > عدد_دلیلاں
            // `define X() is ok to call with nothing
            && !(refp->دلیلاں().ناپ() == 1 && عدد_دلیلاں == 0
                 && trimWhitespace(refp->دلیلاں()[0], خطأ) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            أرجع " `" + refp->name() + " ";
        }
    }

    string باہرلا;
    {  // Parse substitution define using arguments
        string argName;
        منطقي quote = خطأ;
        منطقي triquote = خطأ;
        منطقي backslashesc = خطأ;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        لكل (ثابت حرف* cp = قدر.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            إذا (!quote && !triquote && *cp == '\\') {
                backslashesc = سچ;
            } وإلا إذا (std::خلا_اے(*cp)) {
                backslashesc = خطأ;
            }
            إذا (!quote && !triquote) {
                إذا (std::حرف_اے(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::ہندسہ_اے(*cp))) {
                    argName += *cp;
                    استمر;
                }
                إذا (argName != "") {
                    // Found a possible variable substitution
                    ثابت خودکار iter = arg_قدر_By_Name.لبھو(argName);
                    إذا (iter != arg_قدر_By_Name.سرا()) {
                        // Substitute
                        ثابت string subst = iter->دوجا;
                        إذا (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            إذا (باہرلا.ناپ() >= 2 && باہرلا.substr(باہرلا.ناپ() - 2) == "``") {
                                باہرلا.resize(باہرلا.ناپ() - 2);
                            }
                        } وإلا {
                            باہرلا += subst;
                        }
                    } وإلا {
                        باہرلا += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                إذا (cp[0] == '`' && cp[1] == '`') {
                    إذا (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } وإلا {
                        باہرلا += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    استمر;
                } وإلا إذا (cp[0] == '`' && cp[1] == '"') {
                    باہرلا += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    استمر;
                } وإلا إذا (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    باہرلا += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    استمر;
                } وإلا إذا (cp[0] == '`' && cp[1] == '\\') {
                    باہرلا += '\\';  // `\ means to put out a backslash
                    cp++;
                    استمر;
                } وإلا إذا (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    باہرلا += "\n";  // \\n newline
                    cp++;
                    استمر;
                }
            }
            إذا (cp[0] == '\\' && cp[1] == '\"') {
                باہرلا += cp[0];  // \{any} Put out literal next character
                باہرلا += cp[1];
                cp++;
                استمر;
            } وإلا إذا (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                باہرلا += cp[0];
                استمر;
            }
            إذا (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                باہرلا += "\"\"\"";
                cp += 2;
                استمر;
            }
            إذا (*cp == '"') quote = !quote;
            إذا (*cp) باہرلا += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(باہرلا) << "'");
    أرجع باہرلا;
}

//**********************************************************************
// Parser routines

فراغ V3PreProcImp::openFile(File_سطر*, VInFilter* filterp, ثابت string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    إذا (m_incError) أرجع;
    m_lexp->سیٹ_YYDebug(debug() >= 5);
    V3File::جوڑو_Src_Depend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ثابت منطقي ok = filterp->readWholefile(filename, wholefile /*ref*/);
    إذا (!ok) {
        fileline()->v3error("File not found: " + filename);
        أرجع;
    }

    إذا (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        إذا (m_lexp->m_streamp_ڈھیر.ناپ() > V3PreProc::INCLUDE_ڈونگھائی_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = سچ;
            أرجع;
        }
        // There's already a file active.  Push it to work on the new one.
        جوڑو_سطر_Comment(0);
    }

    // Save file contents for future error reporting
    File_سطر* ثابت flsp = جديد File_سطر{filename};
    flsp->lineno(1);
    flsp->newContent();
    لكل (ثابت string& i : wholefile) flsp->contentp()->ڈالو_متن(i);

    // Save contents for V3Control --contents
    إذا (filename != V3Options::get_Std_Package_رستہ()) {
        منطقي شامل_اے_Vlt = خطأ;
        لكل (ثابت string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            إذا (i.لبھو("`verilator_config") != string::npos) {
                شامل_اے_Vlt = سچ;
                اكسر;
            }
        }
        إذا (!شامل_اے_Vlt) {
            لكل (ثابت string& i : wholefile) V3Control::contents_ڈالو_متن(i);
        }
    }

    // Create new stream structure
    m_lexp->جانچ_New_File(flsp);
    جوڑو_سطر_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    صحيح eof_newline = 0;  // Number of characters following last newline
    صحيح eof_lineno = 1;
    لكل (StrList::iterator it = wholefile.begin(); it != wholefile.سرا(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        منطقي strip = خطأ;
        ثابت حرف* ثابت sp = it->انکڑے();
        ثابت حرف* ثابت ep = sp + it->لمائی();
        // Only process if needed, as saves extra string allocations
        لكل (ثابت حرف* cp = sp; cp < ep; cp++) {
            إذا (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = سچ;
            } وإلا إذا (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } وإلا {
                ++eof_newline;
            }
        }
        إذا (strip) {
            string باہرلا;
            باہرلا.reserve(it->لمائی());
            لكل (ثابت حرف* cp = sp; cp < ep; cp++) {
                إذا (!(*cp == '\r' || *cp == '\0')) باہرلا += *cp;
            }
            *it = باہرلا;
        }

        // Push the data to an internal buffer.
        m_lexp->جانچ_بائٹاں_پٹھ(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    إذا (eof_newline) {
        File_سطر* ثابت fl = جديد File_سطر{flsp};
        fl->contentLineno(eof_lineno);
        fl->کالم_2(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_ہور() << "... Suggest add newline.");
    }
}

فراغ V3PreProcImp::داخل_کرو_Unreadback_At_Bol(ثابت string& متن) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    إذا (m_سطر_Cmt == "") {
        m_سطر_Cmt_Nl = سچ;
    } وإلا إذا (m_سطر_Cmt[m_سطر_Cmt.لمائی() - 1] != '\n') {
        داخل_کرو_Unreadback("\n");
    }
    داخل_کرو_Unreadback(متن);
}

فراغ V3PreProcImp::جوڑو_سطر_Comment(صحيح enterExit) {
    إذا (سطر_Directives()) {
        داخل_کرو_Unreadback_At_Bol(m_lexp->curFilelinep()->سطر_Directive_Strg(enterExit));
    }
}

فراغ V3PreProcImp::dump_مقرر_کردا(std::ostream& os) {
    لكل (مقرر_کردا_Map::const_iterator it = m_مقرر_کردا.begin(); it != m_مقرر_کردا.سرا(); ++it) {
        os << "`define " << it->پہلا;
        // No need to print "()" below as already part of params()
        إذا (!it->دوجا.params().خالی_تھاواں()) os << it->دوجا.params();
        إذا (!it->دوجا.قدر().خالی_تھاواں()) os << " " << it->دوجا.قدر();
        os << '\n';
    }
}

فراغ V3PreProcImp::candidate_مقرر_کردا(VSpellCheck* spellerp) {
    لكل (مقرر_کردا_Map::const_iterator it = m_مقرر_کردا.begin(); it != m_مقرر_کردا.سرا(); ++it) {
        spellerp->ڈالو_Candidate("`"s + it->پہلا);
    }
}

صحيح V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    طالما (سچ) {
    اگلا_tok:
        إذا (m_سطر_جوڑو) {
            m_سطر_جوڑو--;
            m_rawAtBol = سچ;
            yyourtext("\n", 1);
            إذا (debug() >= 5) debugToken(VP_WHITE, "LNA");
            أرجع VP_WHITE;
        }
        إذا (m_سطر_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ساكن string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_سطر_Cmt;
            إذا (m_سطر_Cmt_Nl) {
                إذا (!m_rawAtBol) s_rtncmt.داخل_کرو(0, "\n");
                m_سطر_Cmt_Nl = خطأ;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.لمائی());
            m_سطر_Cmt = "";
            إذا (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            إذا (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_قدر(yyourtext(), yyourleng());
                اذهب اگلا_tok;
            } وإلا {
                إذا (debug() >= 5) debugToken(VP_متن, "LCM");
                أرجع VP_متن;
            }
        }
        إذا (isEof()) أرجع VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->شروع_Token();
        صحيح tok = m_lexp->lex();
        إذا (debug() >= 5) debugToken(tok, "RAW");

        إذا (m_آخری_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_آخری_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_ٹوکن_On_سطر = 0;
        } وإلا إذا (++m_ٹوکن_On_سطر > v3Global.opt.preproc_Token_حد_بندی()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_منزل_Str(v3Global.opt.preproc_Token_حد_بندی())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        إذا (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            إذا (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            اذهب اگلا_tok;
        }

        إذا (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        أرجع tok;
    }
}

فراغ V3PreProcImp::debugToken(صحيح tok, ثابت حرف* cmtp) {
    ثابت ساكن صحيح s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    إذا (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::ناپ_type pos;
        طالما ((pos = buf.لبھو('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        طالما ((pos = buf.لبھو('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_خانہ();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<صحيح>(m_حالتاں.ناپ())
                       << ") dr" << m_defRefs.ناپ() << ":  <" << m_lexp->current_شروع_State()
                       << ">" << tokenName(tok) << ": " << buf);
        إذا (s_debugFileline >= 9) {
            std::باہر << m_lexp->m_tokFilelinep->warnContextSecondary() << سطر_انت;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

صحيح V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    طالما (سچ) {
    اگلا_tok:
        إذا (isEof()) أرجع VP_EOF;
        صحيح tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        إذا (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) أرجع tok;
        إذا (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_متن;
        إذا (tok == VP_COMMENT) {
            إذا (!m_off) {
                إذا (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.سونپو_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    إذا (m_سطر_Cmt.خالی_تھاواں()) داخل_کرو_Unreadback(" ");
                } وإلا إذا (m_lexp->m_keepComments) {
                    أرجع tok;
                } وإلا {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    داخل_کرو_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            لكل (ناپ_t len = 0; len < static_cast<ناپ_t>(yyourleng()); len++) {
                إذا (yyourtext()[len] == '\n') m_سطر_جوڑو++;
            }
            اذهب اگلا_tok;
        }
        إذا (tok == VP_سطر) {
            جوڑو_سطر_Comment(m_lexp->m_enterExit);
            اذهب اگلا_tok;
        }

        إذا (tok == VP_DEFREF_JOIN) {
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
            إذا (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = سچ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } وإلا {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        إذا (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            إذا (tok == VP_JOIN) اذهب اگلا_tok;
            إذا (tok == VP_علامت_JOIN) tok = VP_علامت;
        }
        إذا (tok == VP_علامت_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ثابت std::string باہرلا(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << باہرلا);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ڈھیر.ڈالو(باہرلا);
            state_ڈالو(ps_JOIN);
            اذهب اگلا_tok;
        }

        // Deal with some special parser states
        بدل (state()) {
        حالة ps_چوٹی: {
            اكسر;
        }
        حالة ps_DEFNAME_UNDEF:  // FALLTHRU
        حالة ps_DEFNAME_DEFINE:  // FALLTHRU
        حالة ps_DEFNAME_IFDEF:  // FALLTHRU
        حالة ps_DEFNAME_IFNDEF:  // FALLTHRU
        حالة ps_DEFNAME_ELSIF: {
            إذا (tok == VP_علامت) {
                m_آخری_Sym.سونپو_2(yyourtext(), yyourleng());
                إذا (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    منطقي enable = defExists(m_آخری_Sym);
                    UINFO(4, "Ifdef " << m_آخری_Sym << (enable ? " ON" : " OFF"));
                    إذا (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, خطأ});
                    إذا (!enable) parsingOff();
                    state_نکالو();
                    اذهب اگلا_tok;
                } وإلا إذا (state() == ps_DEFNAME_ELSIF) {
                    إذا (m_ifdef_ڈھیر.خالی_تھاواں()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } وإلا {
                        // Handle `else portion
                        ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                        m_ifdef_ڈھیر.نکالو();
                        إذا (!آخری_If.on()) parsingOn();
                        // Handle `if portion
                        ثابت منطقي enable = !آخری_If.everOn() && defExists(m_آخری_Sym);
                        UINFO(4, "Elsif " << m_آخری_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, آخری_If.everOn()});
                        إذا (!enable) parsingOff();
                    }
                    state_نکالو();
                    اذهب اگلا_tok;
                } وإلا إذا (state() == ps_DEFNAME_UNDEF) {
                    إذا (!m_off) {
                        UINFO(4, "Undef " << m_آخری_Sym);
                        undef(m_آخری_Sym);
                    }
                    state_نکالو();
                    اذهب اگلا_tok;
                } وإلا إذا (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ڈالو_State_Def_Form();
                    اذهب اگلا_tok;
                } وإلا {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن) {
                // IE, something like comment between define and symbol
                إذا (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ڈالو_State_اظہاریہ();
                    m_اظہاریہ_Parser.reset(fileline());
                    m_اظہاریہ_Paren_Level = 1;
                    بدل (state()) {
                    حالة ps_DEFNAME_IFDEF: stateChange(ps_اظہاریہ_IFDEF); اكسر;
                    حالة ps_DEFNAME_IFNDEF: stateChange(ps_اظہاریہ_IFNDEF); اكسر;
                    حالة ps_DEFNAME_ELSIF: stateChange(ps_اظہاریہ_ELSIF); اكسر;
                    افتراضي: v3fatalSrc("bad case");
                    }
                    اذهب اگلا_tok;
                }
                إذا (!m_off) {
                    أرجع tok;
                } وإلا {
                    اذهب اگلا_tok;
                }
            } وإلا إذا (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                اكسر;
            } وإلا {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                اذهب اگلا_tok;
            }
        }
        حالة ps_اظہاریہ_IFDEF:  // FALLTHRU
        حالة ps_اظہاریہ_IFNDEF:  // FALLTHRU
        حالة ps_اظہاریہ_ELSIF: {
            // `ifdef ( *here*
            File_سطر* ثابت flp = m_lexp->m_tokFilelinep;
            إذا (tok == VP_علامت) {
                m_آخری_Sym.سونپو_2(yyourtext(), yyourleng());
                ثابت منطقي exists = defExists(m_آخری_Sym);
                إذا (exists) {
                    ثابت std::string قدر = def_قدر(m_آخری_Sym);
                    إذا (VString::removeWhitespace(قدر) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_آخری_Sym << "' with value '" << قدر
                                << "'\n"
                                   "... Suggest change define '"
                                << m_آخری_Sym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, exists});
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_WHITE) {
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::BRA});
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_اظہاریہ_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                إذا (--m_اظہاریہ_Paren_Level > 0) {
                    m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::KET});
                    اذهب اگلا_tok;
                } وإلا {
                    // Done with parsing expression
                    منطقي enable = m_اظہاریہ_Parser.نتیجہ();
                    UINFO(4, "ifdef() result=" << enable);
                    إذا (state() == ps_اظہاریہ_IFDEF || state() == ps_اظہاریہ_IFNDEF) {
                        إذا (state() == ps_اظہاریہ_IFNDEF) enable = !enable;
                        m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, خطأ});
                        إذا (!enable) parsingOff();
                        state_نکالو();
                        اذهب اگلا_tok;
                    } وإلا إذا (state() == ps_اظہاریہ_ELSIF) {
                        إذا (m_ifdef_ڈھیر.خالی_تھاواں()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } وإلا {
                            // Handle `else portion
                            ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                            m_ifdef_ڈھیر.نکالو();
                            إذا (!آخری_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !آخری_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_آخری_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, آخری_If.everOn()});
                            إذا (!enable) parsingOff();
                        }
                        state_نکالو();
                    }
                    اذهب اگلا_tok;
                }
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LNOT});
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 2 && 0 == لڑی_مقابلہ(yyourtext(), "&&", 2)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LAND});
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 2 && 0 == لڑی_مقابلہ(yyourtext(), "||", 2)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LOR});
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 2 && 0 == لڑی_مقابلہ(yyourtext(), "->", 2)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::IMP});
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 3 && 0 == لڑی_مقابلہ(yyourtext(), "<->", 3)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::EQV});
                اذهب اگلا_tok;
            } وإلا {
                إذا (VString::removeWhitespace(string{yyourtext(), yyourleng()}).خالی_تھاواں()) {
                    أرجع tok;
                } وإلا {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                اذهب اگلا_tok;
            }
        }
        حالة ps_DEFFORM: {
            إذا (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_قدر;
                إذا (debug() >= 5) {
                    باہر << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ڈالو_State_Def_قدر();
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن) {
                // IE, something like comment in formals
                إذا (!m_off) أرجع tok;
                اذهب اگلا_tok;
            } وإلا {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                اذهب اگلا_tok;
            }
        }
        حالة ps_DEFVALUE: {
            ساكن string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            إذا (tok == VP_DEFVALUE) {
                إذا (debug() >= 5) {  // LCOV_EXCL_START
                    باہر << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_قدر)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ثابت string formals = m_formals;
                string قدر = m_lexp->m_def_قدر;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                لكل (ناپ_t i = 0; i < formals.لمائی(); i++) {
                    إذا (formals[i] == '\n') s_newlines += "\n";
                }
                لكل (ناپ_t i = 0; i < قدر.لمائی(); i++) {
                    إذا (قدر[i] == '\n') s_newlines += "\n";
                }
                إذا (!m_off) {
                    // Remove leading and trailing whitespace
                    قدر = trimWhitespace(قدر, سچ);
                    // Define it
                    UINFO(4, "Define " << m_آخری_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(قدر) << "'");
                    define(fileline(), m_آخری_Sym, قدر, formals, خطأ);
                }
            } وإلا {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_نکالو();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.لمائی());
            أرجع VP_WHITE;
        }
        حالة ps_DEFPAREN: {
            إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                اذهب اگلا_tok;
            } وإلا {
                UASSERT(!m_defRefs.خالی_تھاواں(), "Shouldn't be in DEFPAREN w/o active defref");
                ثابت VDefineRef* ثابت refp = &(m_defRefs.چوٹی());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_نکالو();
                اذهب اگلا_tok;
            }
        }
        حالة ps_DEFARG: {
            UASSERT(!m_defRefs.خالی_تھاواں(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.چوٹی());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_قدر);
            m_lexp->m_def_قدر = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            إذا (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->دلیلاں().ڈالو_پٹھ(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ڈالو_State_Def_Arg(1);
                refp->nextarg("");
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->دلیلاں().ڈالو_پٹھ(refp->nextarg());
                string باہرلا;
                إذا (!m_off) {
                    باہرلا = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.نکالو(), refp);
                state_نکالو();
                إذا (m_defRefs.خالی_تھاواں() || state() == ps_STRIFY || state() == ps_JOIN) {
                    إذا (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ڈھیر.خالی_تھاواں(), "`` join stack empty, but in a ``");
                        ثابت string lhs = m_join_ڈھیر.چوٹی();
                        m_join_ڈھیر.نکالو();
                        باہرلا.داخل_کرو(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << باہرلا);
                        state_نکالو();
                    }
                    إذا (!m_off) {
                        unputDefrefString(باہرلا);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    وإلا إذا (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.خالی_تھاواں() ? 0 : m_defRefs.چوٹی().parenLevel();
                } وإلا {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.چوٹی());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_قدر + باہرلا);
                    m_lexp->m_def_قدر = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                اكسر;
            } وإلا إذا (tok == VP_علامت || tok == VP_STRING || tok == VP_متن || tok == VP_WHITE) {
                string rtn;
                rtn.سونپو_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ڈالو(ps_STRIFY);
                اذهب اگلا_tok;
            } وإلا {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_نکالو();
                اذهب اگلا_tok;
            }
        }
        حالة ps_INCNAME: {
            إذا (tok == VP_STRING) {
                state_نکالو();
                m_آخری_Sym.سونپو_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_آخری_Sym);
                // Drop leading and trailing quotes.
                m_آخری_Sym.erase(0, 1);
                m_آخری_Sym.erase(m_آخری_Sym.لمائی() - 1, 1);
                include(m_آخری_Sym);
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ڈالو_State_Inc_Filename();
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                اكسر;
            } وإلا {
                state_نکالو();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                اذهب اگلا_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        حالة ps_ERRORNAME: {
            إذا (tok == VP_STRING) {
                إذا (!m_off) {
                    m_آخری_Sym.سونپو_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_آخری_Sym);
                }
                state_نکالو();
                اذهب اگلا_tok;
            } وإلا {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_نکالو();
                اذهب اگلا_tok;
            }
        }
        حالة ps_JOIN: {
            إذا (tok == VP_علامت || tok == VP_متن || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ڈھیر.خالی_تھاواں(), "`` join stack empty, but in a ``");
                ثابت string lhs = m_join_ڈھیر.چوٹی();
                m_join_ڈھیر.نکالو();
                UINFO(5, "`` LHS:" << lhs);
                ثابت string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ثابت string باہرلا = lhs + rhs;
                UINFO(5, "`` Out:" << باہرلا);
                unputString(باہرلا);
                state_نکالو();
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ثابت string lhs = m_join_ڈھیر.چوٹی();
                m_join_ڈھیر.نکالو();
                unputString(lhs);
                state_نکالو();
                اذهب اگلا_tok;
            } وإلا {
                // `define, etc, fall through and expand.  Pop back here.
                اكسر;
            }
        }
        حالة ps_STRIFY: {
            إذا (tok == VP_STRIFY) {
                // Quote what's in the middle of the stringification
                // Note a `" MACRO_WITH(`") `" doesn't need to be handled (we don't need a stack)
                // That behavior isn't specified, and other simulators vary widely
                string باہرلا = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(باہرلا.begin(), باہرلا.سرا(), '\n', ' ');
                unputString("\""s + باہرلا + "\"");
                state_نکالو();
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                اكسر;
            } وإلا إذا (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                اذهب اگلا_tok;
            } وإلا إذا (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                اكسر;
            } وإلا {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                اذهب اگلا_tok;
            }
        }
        افتراضي: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        بدل (tok) {
        حالة VP_INCLUDE:
            إذا (!m_off) {
                state_ڈالو(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            اذهب اگلا_tok;
        حالة VP_UNDEF: state_ڈالو(ps_DEFNAME_UNDEF); اذهب اگلا_tok;
        حالة VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ڈالو(ps_DEFNAME_DEFINE);
            اذهب اگلا_tok;
        حالة VP_IFDEF: state_ڈالو(ps_DEFNAME_IFDEF); اذهب اگلا_tok;
        حالة VP_IFNDEF: state_ڈالو(ps_DEFNAME_IFNDEF); اذهب اگلا_tok;
        حالة VP_ELSIF: state_ڈالو(ps_DEFNAME_ELSIF); اذهب اگلا_tok;
        حالة VP_ELSE:
            إذا (m_ifdef_ڈھیر.خالی_تھاواں()) {
                fileline()->v3error("`else with no matching `if");
            } وإلا {
                ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                m_ifdef_ڈھیر.نکالو();
                ثابت منطقي enable = !آخری_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, آخری_If.everOn()});
                إذا (!آخری_If.on()) parsingOn();
                إذا (!enable) parsingOff();
            }
            اذهب اگلا_tok;
        حالة VP_ENDIF:
            UINFO(4, "Endif ");
            إذا (m_ifdef_ڈھیر.خالی_تھاواں()) {
                fileline()->v3error("`endif with no matching `if");
            } وإلا {
                ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                m_ifdef_ڈھیر.نکالو();
                إذا (!آخری_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            اذهب اگلا_tok;

        حالة VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ثابت string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            إذا (m_defPutJoin) {
                m_defPutJoin = خطأ;
                unputString("``");
            }
            إذا (m_def_ڈونگھائی++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                اذهب اگلا_tok;
            }
            // Substitute
            إذا (!defExists(name)) {  // Not found, return original string as-is
                m_def_ڈونگھائی = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                إذا (m_off) {
                    اذهب اگلا_tok;
                } وإلا {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    اذهب اگلا_tok;
                }
            } وإلا {
                ثابت string params = defParams(name);
                إذا (params == "0" || params == "") {  // Found, as simple substitution
                    string باہرلا;
                    إذا (!m_off) {
                        VDefineRef tempref{name, ""};
                        باہرلا = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    إذا (m_defRefs.خالی_تھاواں() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        إذا (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ڈھیر.خالی_تھاواں(), "`` join stack empty, but in a ``");
                            ثابت string lhs = m_join_ڈھیر.چوٹی();
                            m_join_ڈھیر.نکالو();
                            باہرلا.داخل_کرو(0, lhs);
                            UINFO(5, "``-end-defref Out:" << باہرلا);
                            state_نکالو();
                        }
                        إذا (!m_off) {
                            unputDefrefString(باہرلا);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        وإلا إذا (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } وإلا {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ثابت refp = &(m_defRefs.چوٹی());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_قدر + باہرلا);
                        m_lexp->m_def_قدر = "";
                    }
                    اذهب اگلا_tok;
                } وإلا {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    إذا (!m_defRefs.خالی_تھاواں()) m_defRefs.چوٹی().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ڈالو(ps_DEFPAREN);
                    m_lexp->ڈالو_State_Def_Arg(0);
                    اذهب اگلا_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            اذهب اگلا_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        حالة VP_ERROR: {
            state_ڈالو(ps_ERRORNAME);
            اذهب اگلا_tok;
        }
        حالة VP_EOF:
            إذا (!m_ifdef_ڈھیر.خالی_تھاواں()) fileline()->v3error("`ifdef not terminated at EOF");
            أرجع tok;
        حالة VP_UNDEFINEALL:
            إذا (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            اذهب اگلا_tok;
        حالة VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ڈالو(ps_STRIFY);
            اذهب اگلا_tok;
        حالة VP_علامت:
        حالة VP_STRING:
        حالة VP_متن: {
            m_def_ڈونگھائی = 0;
            إذا (!m_off) {
                أرجع tok;
            } وإلا {
                اذهب اگلا_tok;
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
        أرجع tok;
    }
}

صحيح V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    إذا (!m_finAhead) {
        m_finAhead = سچ;
        m_finToken = getStateToken();
        m_finBuf.سونپو_2(yyourtext(), yyourleng());
    }
    ثابت صحيح tok = m_finToken;
    buf = m_finBuf;
    إذا (خطأ && debug() >= 5) {
        ثابت string bufcln = V3PreLex::cleanDbgStrg(buf);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_خانہ();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ثابت حرف* bufp = buf.c_str();
    طالما (*bufp == '\n') bufp++;
    إذا ((tok == VP_متن || tok == VP_سطر) && VString::startsWith(bufp, "`line ")) {
        صحيح enter;
        m_finFilelinep->سطر_Directive(bufp, enter /*ref*/);
    } وإلا {
        إذا (m_finAtBol && !(tok == VP_متن && buf == "\n") && m_preprocp->سطر_Directives()) {
            إذا (ثابت صحيح باہرلا_Behind
                = (m_lexp->m_tokFilelinep->آخری_Lineno() - m_finFilelinep->آخری_Lineno())) {
                إذا (debug() >= 5) {
                    ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_خانہ();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->آخری_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->آخری_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->آخری_Lineno());
                إذا (باہرلا_Behind > 0
                    && (باہرلا_Behind <= static_cast<صحيح>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    إذا (m_preprocp->keepWhitespace()) {
                        buf = std::string(باہرلا_Behind, '\n');  // () for char repeat
                        أرجع VP_متن;
                    }
                } وإلا {
                    // Need to backup, use `line
                    buf = m_finFilelinep->سطر_Directive_Strg(0);
                    أرجع VP_سطر;
                }
            }
        }
        // Track newlines in prep for next token
        لكل (ثابت حرف& c : buf) {
            إذا (c == '\n') {
                m_finAtBol = سچ;
                m_finFilelinep->linenoInc();
            } وإلا {
                m_finAtBol = خطأ;
            }
        }
    }
    m_finAhead = خطأ;  // Consumed the token
    أرجع tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    إذا (isEof()) أرجع "";
    ثابت حرف* rtnp;
    منطقي gotEof = خطأ;
    طالما (خالی_سوچک == (rtnp = std::strchr(m_سطر_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ثابت صحيح tok = getFinalToken(m_tokenBuf /*ref*/);
        إذا (debug() >= 5) {
            ثابت string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_خانہ();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        إذا (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            إذا (m_سطر_Chars != "" && m_سطر_Chars[m_سطر_Chars.لمائی() - 1] != '\n') {
                m_سطر_Chars.append("\n");
            }
            gotEof = سچ;
        } وإلا {
            m_سطر_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ثابت صحيح len = rtnp - m_سطر_Chars.c_str() + 1;
    string the_سطر(m_سطر_Chars, 0, len);
    m_سطر_Chars = m_سطر_Chars.erase(0, len);  // Remove returned characters
    إذا (debug() >= 4) {
        ثابت string lncln = V3PreLex::cleanDbgStrg(the_سطر);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_خانہ();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    أرجع the_سطر;
}
