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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

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

#تعریف_کړه yyFlexLexer V3Lexer
#ضمن "V3PreLex.yy.cpp"
#تعریف_لغوه yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

صنف VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_کرښه* ثابت m_fileline;  // Where it was declared
    ثابت string m_ارزښت;  // Value of define
    ثابت string m_params;  // Parameters
    ثابت منطقي m_cmdline;  // Set on command line, don't `undefineall
عام:
    VDefine(File_کرښه* fl, ثابت string& ارزښت, ثابت string& params, منطقي cmdline)
        : m_fileline{fl}
        , m_ارزښت{ارزښت}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_کرښه* fileline() ثابت { أرجع m_fileline; }
    string ارزښت() ثابت { أرجع m_ارزښت; }
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

    std::ویکټور<string> m_دلیلونه;  // List of define arguments
عام:
    string name() ثابت { أرجع m_name; }
    string params() ثابت { أرجع m_params; }
    string nextarg() ثابت { أرجع m_nextarg; }
    فراغ nextarg(ثابت string& ارزښت) { m_nextarg = ارزښت; }
    صحيح parenLevel() ثابت { أرجع m_parenLevel; }
    فراغ parenLevel(صحيح ارزښت) { m_parenLevel = ارزښت; }
    std::ویکټور<string>& دلیلونه() { أرجع m_دلیلونه; }
    VDefineRef(ثابت string& name, ثابت string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = افتراضي;
};

//*************************************************************************
/// Data for parsing on/off

صنف VPre_If_ثبت final {
    // One for each pending ifdef/ifndef
    ثابت منطقي m_on;  // Current parse for this ifdef level is "on"
    ثابت منطقي m_everOn;  // Some if term in elsif tree has been on
عام:
    منطقي on() ثابت { أرجع m_on; }
    منطقي everOn() ثابت { أرجع m_everOn; }
    VPre_If_ثبت(منطقي on, منطقي everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_ثبت() = افتراضي;
};

//*************************************************************************
// Data for a preprocessor instantiation.

صنف V3PreProcImp final : عام V3PreProc {
عام:
    // TYPES
    کارول تعریفوي_Map = std::map<ثابت std::string, VDefine>;
    کارول StrList = VInFilter::StrList;

    // Defines list
    تعریفوي_Map m_تعریفوي;  ///< Map of defines

    // STATE
    ثابت V3PreProc* m_preprocp = تش_اشاره;  ///< Object we're holding data for
    V3PreLex* m_lexp = تش_اشاره;  ///< Current lexer state (nullptr = closed)
    صحيح m_وروستی_Lineno = 0;  // Last line number (stall detection)
    صحيح m_نښې_On_کرښه = 0;  // Number of tokens on line (stall detection)

    تعداد ProcState : uint8_t {
        ps_سر,
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
        ps_بیان_IFDEF,
        ps_بیان_IFNDEF,
        ps_بیان_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    ساكن ثابت حرف* procStateName(ProcState s) {
        ساكن ثابت حرف* ثابت حالتونه[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        أرجع حالتونه[s];
    }

    std::ډېری<ProcState> m_حالتونه;  ///< Current state of parser
    صحيح m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    منطقي m_incError = خطأ;  ///< Include error found
    string m_وروستی_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_کرښه_Cmt;  ///< Line comment(s) to be returned
    منطقي m_کرښه_Cmt_Nl = خطأ;  ///< Newline needed before inserting lineCmt
    صحيح m_کرښه_جمع_کړه = 0;  ///< Empty lines to return to maintain line count
    منطقي m_rawAtBol = ریښتیا;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    منطقي m_finAhead = خطأ;  ///< Have read a token ahead
    صحيح m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    منطقي m_finAtBol = ریښتیا;  ///< Last getFinalToken left us at beginning of line
    File_کرښه* m_finFilelinep = تش_اشاره;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ډېری<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ډېری<VPre_If_ثبت> m_ifdef_ډېری;  ///< Stack of true/false emitting evaluations
    بینښه m_def_ژوروالی = 0;  ///< How many `defines deep
    منطقي m_defPutJoin = خطأ;  ///< Insert `` after substitution

    // For `` join
    std::ډېری<string> m_join_ډېری;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_بیان m_بیان_Parser;  ///< Parser for () expression
    صحيح m_بیان_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_کرښه_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    فراغ v3error_پای_ټکی(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_پای_ټکی(str);
    }

    ساكن ثابت حرف* tokenName(صحيح tok);
    فراغ debugToken(صحيح tok, ثابت حرف* cmtp);
    فراغ parse_سر();
    فراغ parseUndef();

خاص:
    // Internal methods
    فراغ پای_ټکی_د_One_File();
    string defineSubst(VDefineRef* refp);

    منطقي defExists(ثابت string& name);
    منطقي defCmdline(ثابت string& name);
    string def_ارزښت(ثابت string& name);
    string defParams(ثابت string& name);
    File_کرښه* defFileline(ثابت string& name);

    string commentCleanup(ثابت string& متن);
    منطقي comment_Token_سمون(string& cmdr, ثابت حرف* strg);
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

    ProcState state() ثابت { أرجع m_حالتونه.سر(); }
    منطقي stateIsDefname() ثابت {
        أرجع state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    فراغ state_ټېل(ProcState state) { m_حالتونه.ټېل(state); }
    فراغ state_راوباسه() {
        m_حالتونه.راوباسه();
        إذا (VL_UNCOVERABLE(m_حالتونه.تشه())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_حالتونه.ټېل(ps_سر);  // LCOV_EXCL_LINE
        }
    }
    فراغ stateChange(ProcState state) {
        state_راوباسه();
        state_ټېل(state);
    }

عام:
    // METHODS, called from upper level shell
    فراغ openFile(File_کرښه* fl, VInFilter* filterp, ثابت string& filename) override;
    منطقي isEof() ثابت override { أرجع m_lexp->curStreamp()->m_eof; }
    فراغ forceEof() { m_lexp->curStreamp()->m_eof = ریښتیا; }
    string getline() override;
    فراغ ورننباسه_Unreadback(ثابت string& متن) override { m_کرښه_Cmt += متن; }
    فراغ ورننباسه_Unreadback_At_Bol(ثابت string& متن);
    فراغ جمع_کړه_کرښه_Comment(صحيح enterExit);
    فراغ dump_تعریفوي(std::ostream& os) override;
    فراغ candidate_تعریفوي(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    فراغ comment(ثابت string& متن) override;  // Comment detected (if keepComments==2)
    فراغ include(ثابت string& filename) override;  // Request a include file be processed
    فراغ undef(ثابت string& name) override;
    مجازي فراغ undefineall();
    فراغ define(File_کرښه* fl, ثابت string& name, ثابت string& ارزښت, ثابت string& params,
                منطقي cmdline) override;
    string remove_تعریفوي(ثابت string& متن) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_حالتونه.ټېل(ps_سر); }
    // cppcheck-suppress duplInheritedMember
    فراغ configure(File_کرښه* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = هذا;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = جديد File_کرښه{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = جديد V3PreLex{هذا, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        إذا (m_lexp) VL_DO_CLEAR(ړنګول m_lexp, m_lexp = تش_اشاره);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_کرښه* fl) {
    V3PreProcImp* ثابت preprocp = جديد V3PreProcImp;
    preprocp->configure(fl);
    أرجع preprocp;
}

فراغ V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_بیان::selfTest(); }

//*************************************************************************
//تعریفوي

فراغ V3PreProcImp::undef(ثابت string& name) {
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preproc_تعریفوي())
        ورننباسه_Unreadback("`undef " + name + "\n");
    m_تعریفوي.erase(name);
}
فراغ V3PreProcImp::undefineall() {
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preproc_تعریفوي())
        ورننباسه_Unreadback("`undefineall\n");
    لكل (تعریفوي_Map::iterator nextit, it = m_تعریفوي.begin(); it != m_تعریفوي.پای_ټکی(); it = nextit) {
        nextit = it;
        ++nextit;
        إذا (!it->دوهم.cmdline()) m_تعریفوي.erase(it);
    }
}
منطقي V3PreProcImp::defExists(ثابت string& name) {
    ثابت خپلکاری iter = m_تعریفوي.پیدا_کړه(name);
    أرجع (iter != m_تعریفوي.پای_ټکی());
}
منطقي V3PreProcImp::defCmdline(ثابت string& name) {
    ثابت خپلکاری iter = m_تعریفوي.پیدا_کړه(name);
    إذا (iter == m_تعریفوي.پای_ټکی()) { أرجع خطأ; }
    أرجع iter->دوهم.cmdline();
}
string V3PreProcImp::def_ارزښت(ثابت string& name) {
    ثابت خپلکاری iter = m_تعریفوي.پیدا_کړه(name);
    إذا (iter == m_تعریفوي.پای_ټکی()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        أرجع "";
    }
    أرجع iter->دوهم.ارزښت();
}
string V3PreProcImp::defParams(ثابت string& name) {
    ثابت خپلکاری iter = m_تعریفوي.پیدا_کړه(name);
    إذا (iter == m_تعریفوي.پای_ټکی()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        أرجع "";
    }
    أرجع iter->دوهم.params();
}
File_کرښه* V3PreProcImp::defFileline(ثابت string& name) {
    ثابت خپلکاری iter = m_تعریفوي.پیدا_کړه(name);
    إذا (iter == m_تعریفوي.پای_ټکی()) أرجع تش_اشاره;
    أرجع iter->دوهم.fileline();
}
فراغ V3PreProcImp::define(File_کرښه* fl, ثابت string& name, ثابت string& ارزښت,
                          ثابت string& params, منطقي cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << ارزښت << "' params '" << params << "'");
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preproc_تعریفوي()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string بهر;
        بهر.reserve(ارزښت.اندازه());
        لكل (string::اندازه_type pos = 0; pos < ارزښت.اندازه(); ++pos) {
            إذا (ارزښت[pos] == '\n' && (pos == 0 || ارزښت[pos - 1] != '\\')) بهر += '\\';
            بهر += ارزښت[pos];
        }
        ورننباسه_Unreadback("`define " + name + params + (بهر.تشه() ? "" : " ") + بهر + "\n");
    }
    إذا (!V3_Language_ټکي::isKeyword("`"s + name).تشه()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } وإلا {
        إذا (defExists(name)) {
            إذا (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << ارزښت
                               << "' to existing command line define value: '" << def_ارزښت(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_ارزښت(name) << (defParams(name).تشه() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                أرجع;
            } وإلا {
                إذا (!(def_ارزښت(name) == ارزښت
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << ارزښت
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_ارزښت(name) << (defParams(name).تشه() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_تعریفوي.emplace(name, VDefine{fl, ارزښت, params, cmdline});
    }
}

string V3PreProcImp::remove_تعریفوي(ثابت string& متن) {
    string val;
    string rtnsym = متن;
    لكل (صحيح loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        إذا (xsym[0] == '`') xsym.erase(0, 1);
        إذا (defExists(xsym)) {
            val = def_ارزښت(xsym);
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
    string::اندازه_type pos;
    طالما ((pos = cmd.پیدا_کړه("//")) != string::npos) cmd.replace(pos, 2, "");
    طالما ((pos = cmd.پیدا_کړه("/*")) != string::npos) cmd.replace(pos, 2, "");
    طالما ((pos = cmd.پیدا_کړه("*/")) != string::npos) cmd.replace(pos, 2, "");
    طالما ((pos = cmd.پیدا_کړه('\"')) != string::npos) cmd.replace(pos, 1, " ");
    طالما ((pos = cmd.پیدا_کړه('\t')) != string::npos) cmd.replace(pos, 1, " ");
    طالما ((pos = cmd.پیدا_کړه("  ")) != string::npos) cmd.replace(pos, 2, " ");
    طالما (!cmd.تشه() && std::که_تشه(cmd[cmd.اندازه() - 1])) cmd.erase(cmd.اندازه() - 1);
    أرجع cmd;
}

منطقي V3PreProcImp::comment_Token_سمون(string& cmdr, ثابت حرف* strg) {
    صحيح len = std::مزي_اوږدوالی(strg);
    إذا (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::که_تشه(cmdr[len]))) {
        إذا (std::که_تشه(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        أرجع ریښتیا;
    } وإلا {
        أرجع خطأ;
    }
}

فراغ V3PreProcImp::comment(ثابت string& متن) {
    // Comment detected.  Only keep relevant data.
    منطقي printed = خطأ;
    إذا (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        ورننباسه_Unreadback(متن);
        printed = ریښتیا;
    }

    ثابت حرف* cp = متن.c_str();
    إذا (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } وإلا {
        أرجع;
    }

    طالما (std::که_تشه(*cp)) ++cp;

    منطقي synth = خطأ;
    منطقي vlcomment = خطأ;
    إذا ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::مزي_اوږدوالی("verilator");
        إذا (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            أرجع;
        }
        vlcomment = ریښتیا;
    } وإلا إذا (VString::startsWith(cp, "synopsys")) {
        cp += std::مزي_اوږدوالی("synopsys");
        إذا (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            أرجع;
        }
        synth = ریښتیا;
    } وإلا إذا (VString::startsWith(cp, "cadence")) {
        cp += std::مزي_اوږدوالی("cadence");
        synth = ریښتیا;
    } وإلا إذا (VString::startsWith(cp, "pragma")) {
        cp += std::مزي_اوږدوالی("pragma");
        synth = ریښتیا;
    } وإلا إذا (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::مزي_اوږدوالی("ambit synthesis");
        synth = ریښتیا;
    } وإلا {
        أرجع;
    }

    طالما (std::که_تشه(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    إذا (synth) {
        إذا (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            إذا (comment_Token_سمون(cmd /*ref*/, "full_case")) {
                إذا (!printed) ورننباسه_Unreadback("/*verilator full_case*/");
            }
            إذا (comment_Token_سمون(cmd /*ref*/, "parallel_case")) {
                إذا (!printed) ورننباسه_Unreadback("/*verilator parallel_case*/");
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
               && !(v3Global.opt.عامه_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        إذا (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::اندازه_type پای_ټکی_د_Cmd = std::مزي_اوږدوالی("public_flat_rw");
            طالما (VString::isIdentifierChar(cmd[پای_ټکی_د_Cmd])) ++پای_ټکی_د_Cmd;
            string بنسټ_Cmd = cmd.substr(0, پای_ټکی_د_Cmd);
            string arg = cmd.substr(پای_ټکی_د_Cmd);
            طالما (std::که_تشه(arg[0])) arg = arg.substr(1);
            إذا (arg.اندازه() && بنسټ_Cmd == "public_flat_rw_on")
                بنسټ_Cmd += "_sns";  // different cmd for applying sensitivity
            إذا (!printed) ورننباسه_Unreadback("/*verilator " + بنسټ_Cmd + "*/ " + arg + " /**/");
        } وإلا إذا (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string بنسټ_Cmd = cmd.substr(0, std::مزي_اوږدوالی("dpi_c_decl"));
            string::اندازه_type پیل_د_Arg = بنسټ_Cmd.اندازه();
            طالما (std::که_تشه(cmd[پیل_د_Arg])) پیل_د_Arg++;
            string arg = cmd.substr(پیل_د_Arg);
            إذا (arg.تشه()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                أرجع;
            }
            إذا (!printed) ورننباسه_Unreadback("/*verilator " + بنسټ_Cmd + "*/ \"" + arg + "\" /**/");
        } وإلا {
            إذا (!printed) ورننباسه_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_کرښه* V3PreProc::fileline() {
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
    حالة VP_کرښه: أرجع ("LINE");
    حالة VP_STRIFY: أرجع ("STRIFY");
    حالة VP_STRING: أرجع ("STRING");
    حالة VP_نښه: أرجع ("SYMBOL");
    حالة VP_نښه_JOIN: أرجع ("SYMBOL_JOIN");
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
    m_lexp->لټون_بایټونه(strg);
}

فراغ V3PreProcImp::unputDefrefString(ثابت string& strg) {
    ثابت صحيح multiline = std::شمېر(strg.begin(), strg.پای_ټکی(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ثابت string& strg, منطقي trailing) {
    // Remove leading whitespace
    string بهر = strg;
    string::اندازه_type leadspace = 0;
    طالما (بهر.اوږدتیا() > leadspace && std::که_تشه(بهر[leadspace])) ++leadspace;
    إذا (leadspace) بهر.erase(0, leadspace);
    // Remove trailing whitespace
    إذا (trailing) {
        string::اندازه_type trailspace = 0;
        طالما (بهر.اوږدتیا() > trailspace && std::که_تشه(بهر[بهر.اوږدتیا() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        إذا (trailspace && بهر.اوږدتیا() > trailspace && بهر[بهر.اوږدتیا() - 1 - trailspace] == '\\')
            --trailspace;
        إذا (trailspace) بهر.erase(بهر.اوږدتیا() - trailspace, trailspace);
    }
    أرجع بهر;
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
    لكل (بینښه i = 0; i < refp->دلیلونه().اندازه(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->دلیلونه()[i] << "'");
    }
    // Grab value
    ثابت string ارزښت = def_ارزښت(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(ارزښت) << "'");

    std::map<ثابت string, string> arg_ارزښت_By_Name;
    {  // Parse argument list into map
        بینښه عدد_دلیلونه = 0;
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
                    string ارزښت_Def;
                    إذا (haveDefault) {
                        ارزښت_Def = token;
                    } وإلا {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, ریښتیا);
                    UINFO(4, "    Got Arg=" << عدد_دلیلونه << "  argName='" << argName
                                            << "'  default='" << ارزښت_Def << "'");
                    // Parse it
                    إذا (argName != "") {
                        إذا (refp->دلیلونه().اندازه() > عدد_دلیلونه) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ثابت string arg = trimWhitespace(refp->دلیلونه()[عدد_دلیلونه], ریښتیا);
                            إذا (arg != "") ارزښت_Def = arg;
                        } وإلا إذا (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            أرجع " `" + refp->name() + " ";
                        }
                        عدد_دلیلونه++;
                    }
                    arg_ارزښت_By_Name[argName] = ارزښت_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = خطأ;
                    استمر;
                } وإلا إذا (*cp == '=') {
                    haveDefault = ریښتیا;
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
        إذا (refp->دلیلونه().اندازه() > عدد_دلیلونه
            // `define X() is ok to call with nothing
            && !(refp->دلیلونه().اندازه() == 1 && عدد_دلیلونه == 0
                 && trimWhitespace(refp->دلیلونه()[0], خطأ) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            أرجع " `" + refp->name() + " ";
        }
    }

    string بهر;
    {  // Parse substitution define using arguments
        string argName;
        منطقي quote = خطأ;
        منطقي triquote = خطأ;
        منطقي backslashesc = خطأ;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        لكل (ثابت حرف* cp = ارزښت.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            إذا (!quote && !triquote && *cp == '\\') {
                backslashesc = ریښتیا;
            } وإلا إذا (std::که_تشه(*cp)) {
                backslashesc = خطأ;
            }
            إذا (!quote && !triquote) {
                إذا (std::که_توری(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::که_عدد(*cp))) {
                    argName += *cp;
                    استمر;
                }
                إذا (argName != "") {
                    // Found a possible variable substitution
                    ثابت خپلکاری iter = arg_ارزښت_By_Name.پیدا_کړه(argName);
                    إذا (iter != arg_ارزښت_By_Name.پای_ټکی()) {
                        // Substitute
                        ثابت string subst = iter->دوهم;
                        إذا (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            إذا (بهر.اندازه() >= 2 && بهر.substr(بهر.اندازه() - 2) == "``") {
                                بهر.resize(بهر.اندازه() - 2);
                            }
                        } وإلا {
                            بهر += subst;
                        }
                    } وإلا {
                        بهر += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                إذا (cp[0] == '`' && cp[1] == '`') {
                    إذا (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } وإلا {
                        بهر += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    استمر;
                } وإلا إذا (cp[0] == '`' && cp[1] == '"') {
                    بهر += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    استمر;
                } وإلا إذا (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    بهر += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    استمر;
                } وإلا إذا (cp[0] == '`' && cp[1] == '\\') {
                    بهر += '\\';  // `\ means to put out a backslash
                    cp++;
                    استمر;
                } وإلا إذا (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    بهر += "\n";  // \\n newline
                    cp++;
                    استمر;
                }
            }
            إذا (cp[0] == '\\' && cp[1] == '\"') {
                بهر += cp[0];  // \{any} Put out literal next character
                بهر += cp[1];
                cp++;
                استمر;
            } وإلا إذا (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                بهر += cp[0];
                استمر;
            }
            إذا (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                بهر += "\"\"\"";
                cp += 2;
                استمر;
            }
            إذا (*cp == '"') quote = !quote;
            إذا (*cp) بهر += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(بهر) << "'");
    أرجع بهر;
}

//**********************************************************************
// Parser routines

فراغ V3PreProcImp::openFile(File_کرښه*, VInFilter* filterp, ثابت string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    إذا (m_incError) أرجع;
    m_lexp->سېټ_YYDebug(debug() >= 5);
    V3File::جمع_کړه_Src_Depend(filename);

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
        إذا (m_lexp->m_streamp_ډېری.اندازه() > V3PreProc::INCLUDE_ژوروالی_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = ریښتیا;
            أرجع;
        }
        // There's already a file active.  Push it to work on the new one.
        جمع_کړه_کرښه_Comment(0);
    }

    // Save file contents for future error reporting
    File_کرښه* ثابت flsp = جديد File_کرښه{filename};
    flsp->lineno(1);
    flsp->newContent();
    لكل (ثابت string& i : wholefile) flsp->contentp()->ټېل_متن(i);

    // Save contents for V3Control --contents
    إذا (filename != V3Options::get_Std_Package_لار()) {
        منطقي لري_Vlt = خطأ;
        لكل (ثابت string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            إذا (i.پیدا_کړه("`verilator_config") != string::npos) {
                لري_Vlt = ریښتیا;
                اكسر;
            }
        }
        إذا (!لري_Vlt) {
            لكل (ثابت string& i : wholefile) V3Control::contents_ټېل_متن(i);
        }
    }

    // Create new stream structure
    m_lexp->لټون_New_File(flsp);
    جمع_کړه_کرښه_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    صحيح eof_newline = 0;  // Number of characters following last newline
    صحيح eof_lineno = 1;
    لكل (StrList::iterator it = wholefile.begin(); it != wholefile.پای_ټکی(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        منطقي strip = خطأ;
        ثابت حرف* ثابت sp = it->معلومات();
        ثابت حرف* ثابت ep = sp + it->اوږدتیا();
        // Only process if needed, as saves extra string allocations
        لكل (ثابت حرف* cp = sp; cp < ep; cp++) {
            إذا (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = ریښتیا;
            } وإلا إذا (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } وإلا {
                ++eof_newline;
            }
        }
        إذا (strip) {
            string بهر;
            بهر.reserve(it->اوږدتیا());
            لكل (ثابت حرف* cp = sp; cp < ep; cp++) {
                إذا (!(*cp == '\r' || *cp == '\0')) بهر += *cp;
            }
            *it = بهر;
        }

        // Push the data to an internal buffer.
        m_lexp->لټون_بایټونه_شا(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    إذا (eof_newline) {
        File_کرښه* ثابت fl = جديد File_کرښه{flsp};
        fl->contentLineno(eof_lineno);
        fl->ستنه_2(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_نور() << "... Suggest add newline.");
    }
}

فراغ V3PreProcImp::ورننباسه_Unreadback_At_Bol(ثابت string& متن) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    إذا (m_کرښه_Cmt == "") {
        m_کرښه_Cmt_Nl = ریښتیا;
    } وإلا إذا (m_کرښه_Cmt[m_کرښه_Cmt.اوږدتیا() - 1] != '\n') {
        ورننباسه_Unreadback("\n");
    }
    ورننباسه_Unreadback(متن);
}

فراغ V3PreProcImp::جمع_کړه_کرښه_Comment(صحيح enterExit) {
    إذا (کرښه_Directives()) {
        ورننباسه_Unreadback_At_Bol(m_lexp->curFilelinep()->کرښه_Directive_Strg(enterExit));
    }
}

فراغ V3PreProcImp::dump_تعریفوي(std::ostream& os) {
    لكل (تعریفوي_Map::const_iterator it = m_تعریفوي.begin(); it != m_تعریفوي.پای_ټکی(); ++it) {
        os << "`define " << it->لومړی;
        // No need to print "()" below as already part of params()
        إذا (!it->دوهم.params().تشه()) os << it->دوهم.params();
        إذا (!it->دوهم.ارزښت().تشه()) os << " " << it->دوهم.ارزښت();
        os << '\n';
    }
}

فراغ V3PreProcImp::candidate_تعریفوي(VSpellCheck* spellerp) {
    لكل (تعریفوي_Map::const_iterator it = m_تعریفوي.begin(); it != m_تعریفوي.پای_ټکی(); ++it) {
        spellerp->ټېل_Candidate("`"s + it->لومړی);
    }
}

صحيح V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    طالما (ریښتیا) {
    بل_tok:
        إذا (m_کرښه_جمع_کړه) {
            m_کرښه_جمع_کړه--;
            m_rawAtBol = ریښتیا;
            yyourtext("\n", 1);
            إذا (debug() >= 5) debugToken(VP_WHITE, "LNA");
            أرجع VP_WHITE;
        }
        إذا (m_کرښه_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ساكن string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_کرښه_Cmt;
            إذا (m_کرښه_Cmt_Nl) {
                إذا (!m_rawAtBol) s_rtncmt.ورننباسه(0, "\n");
                m_کرښه_Cmt_Nl = خطأ;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.اوږدتیا());
            m_کرښه_Cmt = "";
            إذا (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            إذا (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_ارزښت(yyourtext(), yyourleng());
                اذهب بل_tok;
            } وإلا {
                إذا (debug() >= 5) debugToken(VP_متن, "LCM");
                أرجع VP_متن;
            }
        }
        إذا (isEof()) أرجع VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->پیل_Token();
        صحيح tok = m_lexp->lex();
        إذا (debug() >= 5) debugToken(tok, "RAW");

        إذا (m_وروستی_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_وروستی_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_نښې_On_کرښه = 0;
        } وإلا إذا (++m_نښې_On_کرښه > v3Global.opt.preproc_Token_حد()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_منزل_Str(v3Global.opt.preproc_Token_حد())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        إذا (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            إذا (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            اذهب بل_tok;
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
        string::اندازه_type pos;
        طالما ((pos = buf.پیدا_کړه('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        طالما ((pos = buf.پیدا_کړه('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_کرښه_ستون();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<صحيح>(m_حالتونه.اندازه())
                       << ") dr" << m_defRefs.اندازه() << ":  <" << m_lexp->current_پیل_State()
                       << ">" << tokenName(tok) << ": " << buf);
        إذا (s_debugFileline >= 9) {
            std::وتنه << m_lexp->m_tokFilelinep->warnContextSecondary() << کرښه_پای;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

صحيح V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    طالما (ریښتیا) {
    بل_tok:
        إذا (isEof()) أرجع VP_EOF;
        صحيح tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        إذا (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) أرجع tok;
        إذا (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_متن;
        إذا (tok == VP_COMMENT) {
            إذا (!m_off) {
                إذا (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.ټاکل_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    إذا (m_کرښه_Cmt.تشه()) ورننباسه_Unreadback(" ");
                } وإلا إذا (m_lexp->m_keepComments) {
                    أرجع tok;
                } وإلا {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ورننباسه_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            لكل (اندازه_t len = 0; len < static_cast<اندازه_t>(yyourleng()); len++) {
                إذا (yyourtext()[len] == '\n') m_کرښه_جمع_کړه++;
            }
            اذهب بل_tok;
        }
        إذا (tok == VP_کرښه) {
            جمع_کړه_کرښه_Comment(m_lexp->m_enterExit);
            اذهب بل_tok;
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
                m_defPutJoin = ریښتیا;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } وإلا {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        إذا (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            إذا (tok == VP_JOIN) اذهب بل_tok;
            إذا (tok == VP_نښه_JOIN) tok = VP_نښه;
        }
        إذا (tok == VP_نښه_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ثابت std::string بهر(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << بهر);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ډېری.ټېل(بهر);
            state_ټېل(ps_JOIN);
            اذهب بل_tok;
        }

        // Deal with some special parser states
        بدل (state()) {
        حالة ps_سر: {
            اكسر;
        }
        حالة ps_DEFNAME_UNDEF:  // FALLTHRU
        حالة ps_DEFNAME_DEFINE:  // FALLTHRU
        حالة ps_DEFNAME_IFDEF:  // FALLTHRU
        حالة ps_DEFNAME_IFNDEF:  // FALLTHRU
        حالة ps_DEFNAME_ELSIF: {
            إذا (tok == VP_نښه) {
                m_وروستی_Sym.ټاکل_2(yyourtext(), yyourleng());
                إذا (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    منطقي enable = defExists(m_وروستی_Sym);
                    UINFO(4, "Ifdef " << m_وروستی_Sym << (enable ? " ON" : " OFF"));
                    إذا (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ډېری.emplace(VPre_If_ثبت{enable, خطأ});
                    إذا (!enable) parsingOff();
                    state_راوباسه();
                    اذهب بل_tok;
                } وإلا إذا (state() == ps_DEFNAME_ELSIF) {
                    إذا (m_ifdef_ډېری.تشه()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } وإلا {
                        // Handle `else portion
                        ثابت VPre_If_ثبت وروستی_If = m_ifdef_ډېری.سر();
                        m_ifdef_ډېری.راوباسه();
                        إذا (!وروستی_If.on()) parsingOn();
                        // Handle `if portion
                        ثابت منطقي enable = !وروستی_If.everOn() && defExists(m_وروستی_Sym);
                        UINFO(4, "Elsif " << m_وروستی_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_ډېری.emplace(VPre_If_ثبت{enable, وروستی_If.everOn()});
                        إذا (!enable) parsingOff();
                    }
                    state_راوباسه();
                    اذهب بل_tok;
                } وإلا إذا (state() == ps_DEFNAME_UNDEF) {
                    إذا (!m_off) {
                        UINFO(4, "Undef " << m_وروستی_Sym);
                        undef(m_وروستی_Sym);
                    }
                    state_راوباسه();
                    اذهب بل_tok;
                } وإلا إذا (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ټېل_State_Def_Form();
                    اذهب بل_tok;
                } وإلا {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن) {
                // IE, something like comment between define and symbol
                إذا (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ټېل_State_بیان();
                    m_بیان_Parser.reset(fileline());
                    m_بیان_Paren_Level = 1;
                    بدل (state()) {
                    حالة ps_DEFNAME_IFDEF: stateChange(ps_بیان_IFDEF); اكسر;
                    حالة ps_DEFNAME_IFNDEF: stateChange(ps_بیان_IFNDEF); اكسر;
                    حالة ps_DEFNAME_ELSIF: stateChange(ps_بیان_ELSIF); اكسر;
                    افتراضي: v3fatalSrc("bad case");
                    }
                    اذهب بل_tok;
                }
                إذا (!m_off) {
                    أرجع tok;
                } وإلا {
                    اذهب بل_tok;
                }
            } وإلا إذا (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                اكسر;
            } وإلا {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                اذهب بل_tok;
            }
        }
        حالة ps_بیان_IFDEF:  // FALLTHRU
        حالة ps_بیان_IFNDEF:  // FALLTHRU
        حالة ps_بیان_ELSIF: {
            // `ifdef ( *here*
            File_کرښه* ثابت flp = m_lexp->m_tokFilelinep;
            إذا (tok == VP_نښه) {
                m_وروستی_Sym.ټاکل_2(yyourtext(), yyourleng());
                ثابت منطقي exists = defExists(m_وروستی_Sym);
                إذا (exists) {
                    ثابت std::string ارزښت = def_ارزښت(m_وروستی_Sym);
                    إذا (VString::removeWhitespace(ارزښت) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_وروستی_Sym << "' with value '" << ارزښت
                                << "'\n"
                                   "... Suggest change define '"
                                << m_وروستی_Sym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, exists});
                اذهب بل_tok;
            } وإلا إذا (tok == VP_WHITE) {
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::BRA});
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_بیان_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                إذا (--m_بیان_Paren_Level > 0) {
                    m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::KET});
                    اذهب بل_tok;
                } وإلا {
                    // Done with parsing expression
                    منطقي enable = m_بیان_Parser.پایله();
                    UINFO(4, "ifdef() result=" << enable);
                    إذا (state() == ps_بیان_IFDEF || state() == ps_بیان_IFNDEF) {
                        إذا (state() == ps_بیان_IFNDEF) enable = !enable;
                        m_ifdef_ډېری.emplace(VPre_If_ثبت{enable, خطأ});
                        إذا (!enable) parsingOff();
                        state_راوباسه();
                        اذهب بل_tok;
                    } وإلا إذا (state() == ps_بیان_ELSIF) {
                        إذا (m_ifdef_ډېری.تشه()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } وإلا {
                            // Handle `else portion
                            ثابت VPre_If_ثبت وروستی_If = m_ifdef_ډېری.سر();
                            m_ifdef_ډېری.راوباسه();
                            إذا (!وروستی_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !وروستی_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_وروستی_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_ډېری.emplace(VPre_If_ثبت{enable, وروستی_If.everOn()});
                            إذا (!enable) parsingOff();
                        }
                        state_راوباسه();
                    }
                    اذهب بل_tok;
                }
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::LNOT});
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 2 && 0 == مزي_پرتله(yyourtext(), "&&", 2)) {
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::LAND});
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 2 && 0 == مزي_پرتله(yyourtext(), "||", 2)) {
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::LOR});
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 2 && 0 == مزي_پرتله(yyourtext(), "->", 2)) {
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::IMP});
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 3 && 0 == مزي_پرتله(yyourtext(), "<->", 3)) {
                m_بیان_Parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::EQV});
                اذهب بل_tok;
            } وإلا {
                إذا (VString::removeWhitespace(string{yyourtext(), yyourleng()}).تشه()) {
                    أرجع tok;
                } وإلا {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                اذهب بل_tok;
            }
        }
        حالة ps_DEFFORM: {
            إذا (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_ارزښت;
                إذا (debug() >= 5) {
                    وتنه << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ټېل_State_Def_ارزښت();
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن) {
                // IE, something like comment in formals
                إذا (!m_off) أرجع tok;
                اذهب بل_tok;
            } وإلا {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                اذهب بل_tok;
            }
        }
        حالة ps_DEFVALUE: {
            ساكن string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            إذا (tok == VP_DEFVALUE) {
                إذا (debug() >= 5) {  // LCOV_EXCL_START
                    وتنه << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_ارزښت)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ثابت string formals = m_formals;
                string ارزښت = m_lexp->m_def_ارزښت;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                لكل (اندازه_t i = 0; i < formals.اوږدتیا(); i++) {
                    إذا (formals[i] == '\n') s_newlines += "\n";
                }
                لكل (اندازه_t i = 0; i < ارزښت.اوږدتیا(); i++) {
                    إذا (ارزښت[i] == '\n') s_newlines += "\n";
                }
                إذا (!m_off) {
                    // Remove leading and trailing whitespace
                    ارزښت = trimWhitespace(ارزښت, ریښتیا);
                    // Define it
                    UINFO(4, "Define " << m_وروستی_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(ارزښت) << "'");
                    define(fileline(), m_وروستی_Sym, ارزښت, formals, خطأ);
                }
            } وإلا {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_راوباسه();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.اوږدتیا());
            أرجع VP_WHITE;
        }
        حالة ps_DEFPAREN: {
            إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                اذهب بل_tok;
            } وإلا {
                UASSERT(!m_defRefs.تشه(), "Shouldn't be in DEFPAREN w/o active defref");
                ثابت VDefineRef* ثابت refp = &(m_defRefs.سر());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_راوباسه();
                اذهب بل_tok;
            }
        }
        حالة ps_DEFARG: {
            UASSERT(!m_defRefs.تشه(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.سر());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_ارزښت);
            m_lexp->m_def_ارزښت = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            إذا (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->دلیلونه().ټېل_شا(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ټېل_State_Def_Arg(1);
                refp->nextarg("");
                اذهب بل_tok;
            } وإلا إذا (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->دلیلونه().ټېل_شا(refp->nextarg());
                string بهر;
                إذا (!m_off) {
                    بهر = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.راوباسه(), refp);
                state_راوباسه();
                إذا (m_defRefs.تشه() || state() == ps_STRIFY || state() == ps_JOIN) {
                    إذا (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ډېری.تشه(), "`` join stack empty, but in a ``");
                        ثابت string lhs = m_join_ډېری.سر();
                        m_join_ډېری.راوباسه();
                        بهر.ورننباسه(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << بهر);
                        state_راوباسه();
                    }
                    إذا (!m_off) {
                        unputDefrefString(بهر);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    وإلا إذا (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.تشه() ? 0 : m_defRefs.سر().parenLevel();
                } وإلا {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.سر());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_ارزښت + بهر);
                    m_lexp->m_def_ارزښت = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                اذهب بل_tok;
            } وإلا إذا (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                اكسر;
            } وإلا إذا (tok == VP_نښه || tok == VP_STRING || tok == VP_متن || tok == VP_WHITE) {
                string rtn;
                rtn.ټاکل_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                اذهب بل_tok;
            } وإلا إذا (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ټېل(ps_STRIFY);
                اذهب بل_tok;
            } وإلا {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_راوباسه();
                اذهب بل_tok;
            }
        }
        حالة ps_INCNAME: {
            إذا (tok == VP_STRING) {
                state_راوباسه();
                m_وروستی_Sym.ټاکل_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_وروستی_Sym);
                // Drop leading and trailing quotes.
                m_وروستی_Sym.erase(0, 1);
                m_وروستی_Sym.erase(m_وروستی_Sym.اوږدتیا() - 1, 1);
                include(m_وروستی_Sym);
                اذهب بل_tok;
            } وإلا إذا (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ټېل_State_Inc_Filename();
                اذهب بل_tok;
            } وإلا إذا (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                اكسر;
            } وإلا {
                state_راوباسه();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                اذهب بل_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        حالة ps_ERRORNAME: {
            إذا (tok == VP_STRING) {
                إذا (!m_off) {
                    m_وروستی_Sym.ټاکل_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_وروستی_Sym);
                }
                state_راوباسه();
                اذهب بل_tok;
            } وإلا {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_راوباسه();
                اذهب بل_tok;
            }
        }
        حالة ps_JOIN: {
            إذا (tok == VP_نښه || tok == VP_متن || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ډېری.تشه(), "`` join stack empty, but in a ``");
                ثابت string lhs = m_join_ډېری.سر();
                m_join_ډېری.راوباسه();
                UINFO(5, "`` LHS:" << lhs);
                ثابت string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ثابت string بهر = lhs + rhs;
                UINFO(5, "`` Out:" << بهر);
                unputString(بهر);
                state_راوباسه();
                اذهب بل_tok;
            } وإلا إذا (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ثابت string lhs = m_join_ډېری.سر();
                m_join_ډېری.راوباسه();
                unputString(lhs);
                state_راوباسه();
                اذهب بل_tok;
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
                string بهر = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(بهر.begin(), بهر.پای_ټکی(), '\n', ' ');
                unputString("\""s + بهر + "\"");
                state_راوباسه();
                اذهب بل_tok;
            } وإلا إذا (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                اكسر;
            } وإلا إذا (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                اذهب بل_tok;
            } وإلا إذا (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                اكسر;
            } وإلا {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                اذهب بل_tok;
            }
        }
        افتراضي: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        بدل (tok) {
        حالة VP_INCLUDE:
            إذا (!m_off) {
                state_ټېل(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            اذهب بل_tok;
        حالة VP_UNDEF: state_ټېل(ps_DEFNAME_UNDEF); اذهب بل_tok;
        حالة VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ټېل(ps_DEFNAME_DEFINE);
            اذهب بل_tok;
        حالة VP_IFDEF: state_ټېل(ps_DEFNAME_IFDEF); اذهب بل_tok;
        حالة VP_IFNDEF: state_ټېل(ps_DEFNAME_IFNDEF); اذهب بل_tok;
        حالة VP_ELSIF: state_ټېل(ps_DEFNAME_ELSIF); اذهب بل_tok;
        حالة VP_ELSE:
            إذا (m_ifdef_ډېری.تشه()) {
                fileline()->v3error("`else with no matching `if");
            } وإلا {
                ثابت VPre_If_ثبت وروستی_If = m_ifdef_ډېری.سر();
                m_ifdef_ډېری.راوباسه();
                ثابت منطقي enable = !وروستی_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ډېری.emplace(VPre_If_ثبت{enable, وروستی_If.everOn()});
                إذا (!وروستی_If.on()) parsingOn();
                إذا (!enable) parsingOff();
            }
            اذهب بل_tok;
        حالة VP_ENDIF:
            UINFO(4, "Endif ");
            إذا (m_ifdef_ډېری.تشه()) {
                fileline()->v3error("`endif with no matching `if");
            } وإلا {
                ثابت VPre_If_ثبت وروستی_If = m_ifdef_ډېری.سر();
                m_ifdef_ډېری.راوباسه();
                إذا (!وروستی_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            اذهب بل_tok;

        حالة VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ثابت string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            إذا (m_defPutJoin) {
                m_defPutJoin = خطأ;
                unputString("``");
            }
            إذا (m_def_ژوروالی++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                اذهب بل_tok;
            }
            // Substitute
            إذا (!defExists(name)) {  // Not found, return original string as-is
                m_def_ژوروالی = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                إذا (m_off) {
                    اذهب بل_tok;
                } وإلا {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    اذهب بل_tok;
                }
            } وإلا {
                ثابت string params = defParams(name);
                إذا (params == "0" || params == "") {  // Found, as simple substitution
                    string بهر;
                    إذا (!m_off) {
                        VDefineRef tempref{name, ""};
                        بهر = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    إذا (m_defRefs.تشه() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        إذا (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ډېری.تشه(), "`` join stack empty, but in a ``");
                            ثابت string lhs = m_join_ډېری.سر();
                            m_join_ډېری.راوباسه();
                            بهر.ورننباسه(0, lhs);
                            UINFO(5, "``-end-defref Out:" << بهر);
                            state_راوباسه();
                        }
                        إذا (!m_off) {
                            unputDefrefString(بهر);
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
                        VDefineRef* ثابت refp = &(m_defRefs.سر());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_ارزښت + بهر);
                        m_lexp->m_def_ارزښت = "";
                    }
                    اذهب بل_tok;
                } وإلا {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    إذا (!m_defRefs.تشه()) m_defRefs.سر().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ټېل(ps_DEFPAREN);
                    m_lexp->ټېل_State_Def_Arg(0);
                    اذهب بل_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            اذهب بل_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        حالة VP_ERROR: {
            state_ټېل(ps_ERRORNAME);
            اذهب بل_tok;
        }
        حالة VP_EOF:
            إذا (!m_ifdef_ډېری.تشه()) fileline()->v3error("`ifdef not terminated at EOF");
            أرجع tok;
        حالة VP_UNDEFINEALL:
            إذا (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            اذهب بل_tok;
        حالة VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ټېل(ps_STRIFY);
            اذهب بل_tok;
        حالة VP_نښه:
        حالة VP_STRING:
        حالة VP_متن: {
            m_def_ژوروالی = 0;
            إذا (!m_off) {
                أرجع tok;
            } وإلا {
                اذهب بل_tok;
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
        m_finAhead = ریښتیا;
        m_finToken = getStateToken();
        m_finBuf.ټاکل_2(yyourtext(), yyourleng());
    }
    ثابت صحيح tok = m_finToken;
    buf = m_finBuf;
    إذا (خطأ && debug() >= 5) {
        ثابت string bufcln = V3PreLex::cleanDbgStrg(buf);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_کرښه_ستون();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ثابت حرف* bufp = buf.c_str();
    طالما (*bufp == '\n') bufp++;
    إذا ((tok == VP_متن || tok == VP_کرښه) && VString::startsWith(bufp, "`line ")) {
        صحيح enter;
        m_finFilelinep->کرښه_Directive(bufp, enter /*ref*/);
    } وإلا {
        إذا (m_finAtBol && !(tok == VP_متن && buf == "\n") && m_preprocp->کرښه_Directives()) {
            إذا (ثابت صحيح بهر_Behind
                = (m_lexp->m_tokFilelinep->وروستی_Lineno() - m_finFilelinep->وروستی_Lineno())) {
                إذا (debug() >= 5) {
                    ثابت string flcol = m_lexp->m_tokFilelinep->ascii_کرښه_ستون();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->وروستی_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->وروستی_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->وروستی_Lineno());
                إذا (بهر_Behind > 0
                    && (بهر_Behind <= static_cast<صحيح>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    إذا (m_preprocp->keepWhitespace()) {
                        buf = std::string(بهر_Behind, '\n');  // () for char repeat
                        أرجع VP_متن;
                    }
                } وإلا {
                    // Need to backup, use `line
                    buf = m_finFilelinep->کرښه_Directive_Strg(0);
                    أرجع VP_کرښه;
                }
            }
        }
        // Track newlines in prep for next token
        لكل (ثابت حرف& c : buf) {
            إذا (c == '\n') {
                m_finAtBol = ریښتیا;
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
    طالما (تش_اشاره == (rtnp = std::strchr(m_کرښه_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ثابت صحيح tok = getFinalToken(m_tokenBuf /*ref*/);
        إذا (debug() >= 5) {
            ثابت string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ثابت string flcol = m_lexp->m_tokFilelinep->ascii_کرښه_ستون();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        إذا (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            إذا (m_کرښه_Chars != "" && m_کرښه_Chars[m_کرښه_Chars.اوږدتیا() - 1] != '\n') {
                m_کرښه_Chars.append("\n");
            }
            gotEof = ریښتیا;
        } وإلا {
            m_کرښه_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ثابت صحيح len = rtnp - m_کرښه_Chars.c_str() + 1;
    string the_کرښه(m_کرښه_Chars, 0, len);
    m_کرښه_Chars = m_کرښه_Chars.erase(0, len);  // Remove returned characters
    إذا (debug() >= 4) {
        ثابت string lncln = V3PreLex::cleanDbgStrg(the_کرښه);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_کرښه_ستون();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    أرجع the_کرښه;
}
