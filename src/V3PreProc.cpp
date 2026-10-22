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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

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

#تعریف_کرو yyFlexLexer V3Lexer
#شامل "V3PreLex.yy.cpp"
#تعریف_منسوخ yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

جماعت VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_سطر* ثابت m_fileline;  // Where it was declared
    ثابت string m_قدر;  // Value of define
    ثابت string m_params;  // Parameters
    ثابت منطقی m_cmdline;  // Set on command line, don't `undefineall
عوامی:
    VDefine(File_سطر* fl, ثابت string& قدر, ثابت string& params, منطقی cmdline)
        : m_fileline{fl}
        , m_قدر{قدر}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_سطر* fileline() ثابت { واپس m_fileline; }
    string قدر() ثابت { واپس m_قدر; }
    string params() ثابت { واپس m_params; }
    منطقی cmdline() ثابت { واپس m_cmdline; }
};

//*************************************************************************

جماعت VDefineRef final {
    // One for each pending define substitution
    ثابت string m_name;  // Define last name being defined
    ثابت string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    صحیح m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::سمتیہ<string> m_دلائل;  // List of define arguments
عوامی:
    string name() ثابت { واپس m_name; }
    string params() ثابت { واپس m_params; }
    string nextarg() ثابت { واپس m_nextarg; }
    خالی nextarg(ثابت string& قدر) { m_nextarg = قدر; }
    صحیح parenLevel() ثابت { واپس m_parenLevel; }
    خالی parenLevel(صحیح قدر) { m_parenLevel = قدر; }
    std::سمتیہ<string>& دلائل() { واپس m_دلائل; }
    VDefineRef(ثابت string& name, ثابت string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = طےشدہ;
};

//*************************************************************************
/// Data for parsing on/off

جماعت VPre_If_اندراج final {
    // One for each pending ifdef/ifndef
    ثابت منطقی m_on;  // Current parse for this ifdef level is "on"
    ثابت منطقی m_everOn;  // Some if term in elsif tree has been on
عوامی:
    منطقی on() ثابت { واپس m_on; }
    منطقی everOn() ثابت { واپس m_everOn; }
    VPre_If_اندراج(منطقی on, منطقی everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_اندراج() = طےشدہ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

جماعت V3PreProcImp final : عوامی V3PreProc {
عوامی:
    // TYPES
    استعمال متعین_کرتا_Map = std::map<ثابت std::string, VDefine>;
    استعمال StrList = VInFilter::StrList;

    // Defines list
    متعین_کرتا_Map m_متعین_کرتا;  ///< Map of defines

    // STATE
    ثابت V3PreProc* m_preprocp = خالی_اشاریہ;  ///< Object we're holding data for
    V3PreLex* m_lexp = خالی_اشاریہ;  ///< Current lexer state (nullptr = closed)
    صحیح m_آخری_Lineno = 0;  // Last line number (stall detection)
    صحیح m_علامات_On_سطر = 0;  // Number of tokens on line (stall detection)

    شمار ProcState : uint8_t {
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
    ساکن ثابت حرف* procStateName(ProcState s) {
        ساکن ثابت حرف* ثابت حالتیں[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        واپس حالتیں[s];
    }

    std::ڈھیر<ProcState> m_حالتیں;  ///< Current state of parser
    صحیح m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    منطقی m_incError = جھوٹ;  ///< Include error found
    string m_آخری_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_سطر_Cmt;  ///< Line comment(s) to be returned
    منطقی m_سطر_Cmt_Nl = جھوٹ;  ///< Newline needed before inserting lineCmt
    صحیح m_سطر_جوڑو = 0;  ///< Empty lines to return to maintain line count
    منطقی m_rawAtBol = سچ;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    منطقی m_finAhead = جھوٹ;  ///< Have read a token ahead
    صحیح m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    منطقی m_finAtBol = سچ;  ///< Last getFinalToken left us at beginning of line
    File_سطر* m_finFilelinep = خالی_اشاریہ;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::ڈھیر<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::ڈھیر<VPre_If_اندراج> m_ifdef_ڈھیر;  ///< Stack of true/false emitting evaluations
    غیرعلامتی m_def_گہرائی = 0;  ///< How many `defines deep
    منطقی m_defPutJoin = جھوٹ;  ///< Insert `` after substitution

    // For `` join
    std::ڈھیر<string> m_join_ڈھیر;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_اظہاریہ m_اظہاریہ_Parser;  ///< Parser for () expression
    صحیح m_اظہاریہ_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_سطر_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    خالی v3error_انتہا(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_انتہا(str);
    }

    ساکن ثابت حرف* tokenName(صحیح tok);
    خالی debugToken(صحیح tok, ثابت حرف* cmtp);
    خالی parse_چوٹی();
    خالی parseUndef();

نجی:
    // Internal methods
    خالی انتہا_کا_One_File();
    string defineSubst(VDefineRef* refp);

    منطقی defExists(ثابت string& name);
    منطقی defCmdline(ثابت string& name);
    string def_قدر(ثابت string& name);
    string defParams(ثابت string& name);
    File_سطر* defFileline(ثابت string& name);

    string commentCleanup(ثابت string& متن);
    منطقی comment_Token_مطابقت(string& cmdr, ثابت حرف* strg);
    ساکن string trimWhitespace(ثابت string& strg, منطقی trailing);
    خالی unputString(ثابت string& strg);
    خالی unputDefrefString(ثابت string& strg);

    خالی parsingOn() {
        m_off--;
        اگر (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    خالی parsingOff() { m_off++; }

    صحیح getRawToken();
    صحیح getStateToken();
    صحیح getFinalToken(string& buf);

    ProcState state() ثابت { واپس m_حالتیں.چوٹی(); }
    منطقی stateIsDefname() ثابت {
        واپس state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    خالی state_ڈالو(ProcState state) { m_حالتیں.ڈالو(state); }
    خالی state_نکالو() {
        m_حالتیں.نکالو();
        اگر (VL_UNCOVERABLE(m_حالتیں.خالی_خانے())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_حالتیں.ڈالو(ps_چوٹی);  // LCOV_EXCL_LINE
        }
    }
    خالی stateChange(ProcState state) {
        state_نکالو();
        state_ڈالو(state);
    }

عوامی:
    // METHODS, called from upper level shell
    خالی openFile(File_سطر* fl, VInFilter* filterp, ثابت string& filename) override;
    منطقی isEof() ثابت override { واپس m_lexp->curStreamp()->m_eof; }
    خالی forceEof() { m_lexp->curStreamp()->m_eof = سچ; }
    string getline() override;
    خالی داخل_کرو_Unreadback(ثابت string& متن) override { m_سطر_Cmt += متن; }
    خالی داخل_کرو_Unreadback_At_Bol(ثابت string& متن);
    خالی جوڑو_سطر_Comment(صحیح enterExit);
    خالی dump_متعین_کرتا(std::ostream& os) override;
    خالی candidate_متعین_کرتا(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    خالی comment(ثابت string& متن) override;  // Comment detected (if keepComments==2)
    خالی include(ثابت string& filename) override;  // Request a include file be processed
    خالی undef(ثابت string& name) override;
    مجازی خالی undefineall();
    خالی define(File_سطر* fl, ثابت string& name, ثابت string& قدر, ثابت string& params,
                منطقی cmdline) override;
    string remove_متعین_کرتا(ثابت string& متن) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_حالتیں.ڈالو(ps_چوٹی); }
    // cppcheck-suppress duplInheritedMember
    خالی configure(File_سطر* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = یہ;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = نیا File_سطر{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = نیا V3PreLex{یہ, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        اگر (m_lexp) VL_DO_CLEAR(حذف m_lexp, m_lexp = خالی_اشاریہ);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_سطر* fl) {
    V3PreProcImp* ثابت preprocp = نیا V3PreProcImp;
    preprocp->configure(fl);
    واپس preprocp;
}

خالی V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_اظہاریہ::selfTest(); }

//*************************************************************************
//متعین_کرتا

خالی V3PreProcImp::undef(ثابت string& name) {
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preproc_متعین_کرتا())
        داخل_کرو_Unreadback("`undef " + name + "\n");
    m_متعین_کرتا.erase(name);
}
خالی V3PreProcImp::undefineall() {
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preproc_متعین_کرتا())
        داخل_کرو_Unreadback("`undefineall\n");
    برائے (متعین_کرتا_Map::iterator nextit, it = m_متعین_کرتا.begin(); it != m_متعین_کرتا.انتہا(); it = nextit) {
        nextit = it;
        ++nextit;
        اگر (!it->دوسرا.cmdline()) m_متعین_کرتا.erase(it);
    }
}
منطقی V3PreProcImp::defExists(ثابت string& name) {
    ثابت خودکار iter = m_متعین_کرتا.ڈھونڈو(name);
    واپس (iter != m_متعین_کرتا.انتہا());
}
منطقی V3PreProcImp::defCmdline(ثابت string& name) {
    ثابت خودکار iter = m_متعین_کرتا.ڈھونڈو(name);
    اگر (iter == m_متعین_کرتا.انتہا()) { واپس جھوٹ; }
    واپس iter->دوسرا.cmdline();
}
string V3PreProcImp::def_قدر(ثابت string& name) {
    ثابت خودکار iter = m_متعین_کرتا.ڈھونڈو(name);
    اگر (iter == m_متعین_کرتا.انتہا()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        واپس "";
    }
    واپس iter->دوسرا.قدر();
}
string V3PreProcImp::defParams(ثابت string& name) {
    ثابت خودکار iter = m_متعین_کرتا.ڈھونڈو(name);
    اگر (iter == m_متعین_کرتا.انتہا()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        واپس "";
    }
    واپس iter->دوسرا.params();
}
File_سطر* V3PreProcImp::defFileline(ثابت string& name) {
    ثابت خودکار iter = m_متعین_کرتا.ڈھونڈو(name);
    اگر (iter == m_متعین_کرتا.انتہا()) واپس خالی_اشاریہ;
    واپس iter->دوسرا.fileline();
}
خالی V3PreProcImp::define(File_سطر* fl, ثابت string& name, ثابت string& قدر,
                          ثابت string& params, منطقی cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << قدر << "' params '" << params << "'");
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preproc_متعین_کرتا()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string برآمد;
        برآمد.reserve(قدر.سائز());
        برائے (string::سائز_type pos = 0; pos < قدر.سائز(); ++pos) {
            اگر (قدر[pos] == '\n' && (pos == 0 || قدر[pos - 1] != '\\')) برآمد += '\\';
            برآمد += قدر[pos];
        }
        داخل_کرو_Unreadback("`define " + name + params + (برآمد.خالی_خانے() ? "" : " ") + برآمد + "\n");
    }
    اگر (!V3_Language_الفاظ::isKeyword("`"s + name).خالی_خانے()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } ورنہ {
        اگر (defExists(name)) {
            اگر (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << قدر
                               << "' to existing command line define value: '" << def_قدر(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_قدر(name) << (defParams(name).خالی_خانے() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                واپس;
            } ورنہ {
                اگر (!(def_قدر(name) == قدر
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << قدر
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_قدر(name) << (defParams(name).خالی_خانے() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_متعین_کرتا.emplace(name, VDefine{fl, قدر, params, cmdline});
    }
}

string V3PreProcImp::remove_متعین_کرتا(ثابت string& متن) {
    string val;
    string rtnsym = متن;
    برائے (صحیح loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        اگر (xsym[0] == '`') xsym.erase(0, 1);
        اگر (defExists(xsym)) {
            val = def_قدر(xsym);
            اگر (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } ورنہ {
                توڑو;
            }
        } ورنہ {
            توڑو;
        }
    }
    واپس rtnsym;  // NA
}

//**********************************************************************
// Comments

خالی V3PreProcImp::include(ثابت string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(ثابت string& متن) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = متن;
    string::سائز_type pos;
    جب ((pos = cmd.ڈھونڈو("//")) != string::npos) cmd.replace(pos, 2, "");
    جب ((pos = cmd.ڈھونڈو("/*")) != string::npos) cmd.replace(pos, 2, "");
    جب ((pos = cmd.ڈھونڈو("*/")) != string::npos) cmd.replace(pos, 2, "");
    جب ((pos = cmd.ڈھونڈو('\"')) != string::npos) cmd.replace(pos, 1, " ");
    جب ((pos = cmd.ڈھونڈو('\t')) != string::npos) cmd.replace(pos, 1, " ");
    جب ((pos = cmd.ڈھونڈو("  ")) != string::npos) cmd.replace(pos, 2, " ");
    جب (!cmd.خالی_خانے() && std::خلا_ہے(cmd[cmd.سائز() - 1])) cmd.erase(cmd.سائز() - 1);
    واپس cmd;
}

منطقی V3PreProcImp::comment_Token_مطابقت(string& cmdr, ثابت حرف* strg) {
    صحیح len = std::لڑی_لمبائی(strg);
    اگر (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::خلا_ہے(cmdr[len]))) {
        اگر (std::خلا_ہے(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        واپس سچ;
    } ورنہ {
        واپس جھوٹ;
    }
}

خالی V3PreProcImp::comment(ثابت string& متن) {
    // Comment detected.  Only keep relevant data.
    منطقی printed = جھوٹ;
    اگر (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        داخل_کرو_Unreadback(متن);
        printed = سچ;
    }

    ثابت حرف* cp = متن.c_str();
    اگر (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } ورنہ {
        واپس;
    }

    جب (std::خلا_ہے(*cp)) ++cp;

    منطقی synth = جھوٹ;
    منطقی vlcomment = جھوٹ;
    اگر ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::لڑی_لمبائی("verilator");
        اگر (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            واپس;
        }
        vlcomment = سچ;
    } ورنہ اگر (VString::startsWith(cp, "synopsys")) {
        cp += std::لڑی_لمبائی("synopsys");
        اگر (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            واپس;
        }
        synth = سچ;
    } ورنہ اگر (VString::startsWith(cp, "cadence")) {
        cp += std::لڑی_لمبائی("cadence");
        synth = سچ;
    } ورنہ اگر (VString::startsWith(cp, "pragma")) {
        cp += std::لڑی_لمبائی("pragma");
        synth = سچ;
    } ورنہ اگر (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::لڑی_لمبائی("ambit synthesis");
        synth = سچ;
    } ورنہ {
        واپس;
    }

    جب (std::خلا_ہے(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    اگر (synth) {
        اگر (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            اگر (comment_Token_مطابقت(cmd /*ref*/, "full_case")) {
                اگر (!printed) داخل_کرو_Unreadback("/*verilator full_case*/");
            }
            اگر (comment_Token_مطابقت(cmd /*ref*/, "parallel_case")) {
                اگر (!printed) داخل_کرو_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } ورنہ اگر (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.عوامی_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        اگر (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::سائز_type انتہا_کا_Cmd = std::لڑی_لمبائی("public_flat_rw");
            جب (VString::isIdentifierChar(cmd[انتہا_کا_Cmd])) ++انتہا_کا_Cmd;
            string بنیاد_Cmd = cmd.substr(0, انتہا_کا_Cmd);
            string arg = cmd.substr(انتہا_کا_Cmd);
            جب (std::خلا_ہے(arg[0])) arg = arg.substr(1);
            اگر (arg.سائز() && بنیاد_Cmd == "public_flat_rw_on")
                بنیاد_Cmd += "_sns";  // different cmd for applying sensitivity
            اگر (!printed) داخل_کرو_Unreadback("/*verilator " + بنیاد_Cmd + "*/ " + arg + " /**/");
        } ورنہ اگر (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string بنیاد_Cmd = cmd.substr(0, std::لڑی_لمبائی("dpi_c_decl"));
            string::سائز_type آغاز_کا_Arg = بنیاد_Cmd.سائز();
            جب (std::خلا_ہے(cmd[آغاز_کا_Arg])) آغاز_کا_Arg++;
            string arg = cmd.substr(آغاز_کا_Arg);
            اگر (arg.خالی_خانے()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                واپس;
            }
            اگر (!printed) داخل_کرو_Unreadback("/*verilator " + بنیاد_Cmd + "*/ \"" + arg + "\" /**/");
        } ورنہ {
            اگر (!printed) داخل_کرو_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_سطر* V3PreProc::fileline() {
    ثابت V3PreProcImp* idatap = static_cast<V3PreProcImp*>(یہ);
    واپس idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

ثابت حرف* V3PreProcImp::tokenName(صحیح tok) {
    انتخاب (tok) {
    حالت VP_BACKQUOTE: واپس ("BACKQUOTE");
    حالت VP_COMMENT: واپس ("COMMENT");
    حالت VP_DEFARG: واپس ("DEFARG");
    حالت VP_DEFFORM: واپس ("DEFFORM");
    حالت VP_DEFINE: واپس ("DEFINE");
    حالت VP_DEFREF: واپس ("DEFREF");
    حالت VP_DEFREF_JOIN: واپس ("DEFREF_JOIN");
    حالت VP_DEFVALUE: واپس ("DEFVALUE");
    حالت VP_ELSE: واپس ("ELSE");
    حالت VP_ELSIF: واپس ("ELSIF");
    حالت VP_ENDIF: واپس ("ENDIF");
    حالت VP_EOF: واپس ("EOF");
    حالت VP_EOF_ERROR: واپس ("EOF_ERROR");
    حالت VP_ERROR: واپس ("ERROR");
    حالت VP_IFDEF: واپس ("IFDEF");
    حالت VP_IFNDEF: واپس ("IFNDEF");
    حالت VP_JOIN: واپس ("JOIN");
    حالت VP_INCLUDE: واپس ("INCLUDE");
    حالت VP_سطر: واپس ("LINE");
    حالت VP_STRIFY: واپس ("STRIFY");
    حالت VP_STRING: واپس ("STRING");
    حالت VP_علامت: واپس ("SYMBOL");
    حالت VP_علامت_JOIN: واپس ("SYMBOL_JOIN");
    حالت VP_متن: واپس ("TEXT");
    حالت VP_UNDEF: واپس ("UNDEF");
    حالت VP_UNDEFINEALL: واپس ("UNDEFINEALL");
    حالت VP_WHITE: واپس ("WHITE");
    طےشدہ: واپس ("?");
    }
}

خالی V3PreProcImp::unputString(ثابت string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    اگر (VL_UNCOVERABLE(m_lexp->m_بفر_State != m_lexp->current_بفر())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->جائزہ_بائٹیں(strg);
}

خالی V3PreProcImp::unputDefrefString(ثابت string& strg) {
    ثابت صحیح multiline = std::گنتی(strg.begin(), strg.انتہا(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ثابت string& strg, منطقی trailing) {
    // Remove leading whitespace
    string برآمد = strg;
    string::سائز_type leadspace = 0;
    جب (برآمد.لمبائی_2() > leadspace && std::خلا_ہے(برآمد[leadspace])) ++leadspace;
    اگر (leadspace) برآمد.erase(0, leadspace);
    // Remove trailing whitespace
    اگر (trailing) {
        string::سائز_type trailspace = 0;
        جب (برآمد.لمبائی_2() > trailspace && std::خلا_ہے(برآمد[برآمد.لمبائی_2() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        اگر (trailspace && برآمد.لمبائی_2() > trailspace && برآمد[برآمد.لمبائی_2() - 1 - trailspace] == '\\')
            --trailspace;
        اگر (trailspace) برآمد.erase(برآمد.لمبائی_2() - trailspace, trailspace);
    }
    واپس برآمد;
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
    برائے (غیرعلامتی i = 0; i < refp->دلائل().سائز(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->دلائل()[i] << "'");
    }
    // Grab value
    ثابت string قدر = def_قدر(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(قدر) << "'");

    std::map<ثابت string, string> arg_قدر_By_Name;
    {  // Parse argument list into map
        غیرعلامتی عدد_دلائل = 0;
        string argName;
        صحیح paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        منطقی quote = جھوٹ;
        منطقی haveDefault = جھوٹ;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        ثابت string params = refp->params();  // Must keep str in scope to get pointer
        ثابت حرف* cp = params.c_str();
        اگر (*cp == '(') cp++;
        برائے (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            اگر (!quote && paren == 1) {
                اگر (*cp == ')' || *cp == ',') {
                    string قدر_Def;
                    اگر (haveDefault) {
                        قدر_Def = token;
                    } ورنہ {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, سچ);
                    UINFO(4, "    Got Arg=" << عدد_دلائل << "  argName='" << argName
                                            << "'  default='" << قدر_Def << "'");
                    // Parse it
                    اگر (argName != "") {
                        اگر (refp->دلائل().سائز() > عدد_دلائل) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ثابت string arg = trimWhitespace(refp->دلائل()[عدد_دلائل], سچ);
                            اگر (arg != "") قدر_Def = arg;
                        } ورنہ اگر (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            واپس " `" + refp->name() + " ";
                        }
                        عدد_دلائل++;
                    }
                    arg_قدر_By_Name[argName] = قدر_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = جھوٹ;
                    جاری;
                } ورنہ اگر (*cp == '=') {
                    haveDefault = سچ;
                    argName = token;
                    token = "";
                    جاری;
                }
            }
            اگر (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                جاری;
            }
            اگر (!quote) {
                اگر (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } ورنہ اگر (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            اگر (*cp == '"') quote = !quote;
            اگر (*cp) token += *cp;
        }
        اگر (refp->دلائل().سائز() > عدد_دلائل
            // `define X() is ok to call with nothing
            && !(refp->دلائل().سائز() == 1 && عدد_دلائل == 0
                 && trimWhitespace(refp->دلائل()[0], جھوٹ) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            واپس " `" + refp->name() + " ";
        }
    }

    string برآمد;
    {  // Parse substitution define using arguments
        string argName;
        منطقی quote = جھوٹ;
        منطقی triquote = جھوٹ;
        منطقی backslashesc = جھوٹ;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        برائے (ثابت حرف* cp = قدر.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            اگر (!quote && !triquote && *cp == '\\') {
                backslashesc = سچ;
            } ورنہ اگر (std::خلا_ہے(*cp)) {
                backslashesc = جھوٹ;
            }
            اگر (!quote && !triquote) {
                اگر (std::حرف_ہے(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::ہندسہ_ہے(*cp))) {
                    argName += *cp;
                    جاری;
                }
                اگر (argName != "") {
                    // Found a possible variable substitution
                    ثابت خودکار iter = arg_قدر_By_Name.ڈھونڈو(argName);
                    اگر (iter != arg_قدر_By_Name.انتہا()) {
                        // Substitute
                        ثابت string subst = iter->دوسرا;
                        اگر (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            اگر (برآمد.سائز() >= 2 && برآمد.substr(برآمد.سائز() - 2) == "``") {
                                برآمد.resize(برآمد.سائز() - 2);
                            }
                        } ورنہ {
                            برآمد += subst;
                        }
                    } ورنہ {
                        برآمد += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                اگر (cp[0] == '`' && cp[1] == '`') {
                    اگر (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } ورنہ {
                        برآمد += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    جاری;
                } ورنہ اگر (cp[0] == '`' && cp[1] == '"') {
                    برآمد += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    جاری;
                } ورنہ اگر (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    برآمد += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    جاری;
                } ورنہ اگر (cp[0] == '`' && cp[1] == '\\') {
                    برآمد += '\\';  // `\ means to put out a backslash
                    cp++;
                    جاری;
                } ورنہ اگر (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    برآمد += "\n";  // \\n newline
                    cp++;
                    جاری;
                }
            }
            اگر (cp[0] == '\\' && cp[1] == '\"') {
                برآمد += cp[0];  // \{any} Put out literal next character
                برآمد += cp[1];
                cp++;
                جاری;
            } ورنہ اگر (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                برآمد += cp[0];
                جاری;
            }
            اگر (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                برآمد += "\"\"\"";
                cp += 2;
                جاری;
            }
            اگر (*cp == '"') quote = !quote;
            اگر (*cp) برآمد += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(برآمد) << "'");
    واپس برآمد;
}

//**********************************************************************
// Parser routines

خالی V3PreProcImp::openFile(File_سطر*, VInFilter* filterp, ثابت string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    اگر (m_incError) واپس;
    m_lexp->سیٹ_YYDebug(debug() >= 5);
    V3File::جوڑو_Src_Depend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    ثابت منطقی ok = filterp->readWholefile(filename, wholefile /*ref*/);
    اگر (!ok) {
        fileline()->v3error("File not found: " + filename);
        واپس;
    }

    اگر (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        اگر (m_lexp->m_streamp_ڈھیر.سائز() > V3PreProc::INCLUDE_گہرائی_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = سچ;
            واپس;
        }
        // There's already a file active.  Push it to work on the new one.
        جوڑو_سطر_Comment(0);
    }

    // Save file contents for future error reporting
    File_سطر* ثابت flsp = نیا File_سطر{filename};
    flsp->lineno(1);
    flsp->newContent();
    برائے (ثابت string& i : wholefile) flsp->contentp()->ڈالو_متن(i);

    // Save contents for V3Control --contents
    اگر (filename != V3Options::get_Std_Package_راستہ()) {
        منطقی شامل_ہے_Vlt = جھوٹ;
        برائے (ثابت string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            اگر (i.ڈھونڈو("`verilator_config") != string::npos) {
                شامل_ہے_Vlt = سچ;
                توڑو;
            }
        }
        اگر (!شامل_ہے_Vlt) {
            برائے (ثابت string& i : wholefile) V3Control::contents_ڈالو_متن(i);
        }
    }

    // Create new stream structure
    m_lexp->جائزہ_New_File(flsp);
    جوڑو_سطر_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    صحیح eof_newline = 0;  // Number of characters following last newline
    صحیح eof_lineno = 1;
    برائے (StrList::iterator it = wholefile.begin(); it != wholefile.انتہا(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        منطقی strip = جھوٹ;
        ثابت حرف* ثابت sp = it->ڈیٹا();
        ثابت حرف* ثابت ep = sp + it->لمبائی_2();
        // Only process if needed, as saves extra string allocations
        برائے (ثابت حرف* cp = sp; cp < ep; cp++) {
            اگر (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = سچ;
            } ورنہ اگر (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } ورنہ {
                ++eof_newline;
            }
        }
        اگر (strip) {
            string برآمد;
            برآمد.reserve(it->لمبائی_2());
            برائے (ثابت حرف* cp = sp; cp < ep; cp++) {
                اگر (!(*cp == '\r' || *cp == '\0')) برآمد += *cp;
            }
            *it = برآمد;
        }

        // Push the data to an internal buffer.
        m_lexp->جائزہ_بائٹیں_عقب(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    اگر (eof_newline) {
        File_سطر* ثابت fl = نیا File_سطر{flsp};
        fl->contentLineno(eof_lineno);
        fl->عمود_2(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_مزید() << "... Suggest add newline.");
    }
}

خالی V3PreProcImp::داخل_کرو_Unreadback_At_Bol(ثابت string& متن) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    اگر (m_سطر_Cmt == "") {
        m_سطر_Cmt_Nl = سچ;
    } ورنہ اگر (m_سطر_Cmt[m_سطر_Cmt.لمبائی_2() - 1] != '\n') {
        داخل_کرو_Unreadback("\n");
    }
    داخل_کرو_Unreadback(متن);
}

خالی V3PreProcImp::جوڑو_سطر_Comment(صحیح enterExit) {
    اگر (سطر_Directives()) {
        داخل_کرو_Unreadback_At_Bol(m_lexp->curFilelinep()->سطر_Directive_Strg(enterExit));
    }
}

خالی V3PreProcImp::dump_متعین_کرتا(std::ostream& os) {
    برائے (متعین_کرتا_Map::const_iterator it = m_متعین_کرتا.begin(); it != m_متعین_کرتا.انتہا(); ++it) {
        os << "`define " << it->پہلا;
        // No need to print "()" below as already part of params()
        اگر (!it->دوسرا.params().خالی_خانے()) os << it->دوسرا.params();
        اگر (!it->دوسرا.قدر().خالی_خانے()) os << " " << it->دوسرا.قدر();
        os << '\n';
    }
}

خالی V3PreProcImp::candidate_متعین_کرتا(VSpellCheck* spellerp) {
    برائے (متعین_کرتا_Map::const_iterator it = m_متعین_کرتا.begin(); it != m_متعین_کرتا.انتہا(); ++it) {
        spellerp->ڈالو_Candidate("`"s + it->پہلا);
    }
}

صحیح V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    جب (سچ) {
    اگلا_tok:
        اگر (m_سطر_جوڑو) {
            m_سطر_جوڑو--;
            m_rawAtBol = سچ;
            yyourtext("\n", 1);
            اگر (debug() >= 5) debugToken(VP_WHITE, "LNA");
            واپس VP_WHITE;
        }
        اگر (m_سطر_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            ساکن string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_سطر_Cmt;
            اگر (m_سطر_Cmt_Nl) {
                اگر (!m_rawAtBol) s_rtncmt.داخل_کرو(0, "\n");
                m_سطر_Cmt_Nl = جھوٹ;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.لمبائی_2());
            m_سطر_Cmt = "";
            اگر (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            اگر (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_قدر(yyourtext(), yyourleng());
                جاؤ اگلا_tok;
            } ورنہ {
                اگر (debug() >= 5) debugToken(VP_متن, "LCM");
                واپس VP_متن;
            }
        }
        اگر (isEof()) واپس VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->آغاز_Token();
        صحیح tok = m_lexp->lex();
        اگر (debug() >= 5) debugToken(tok, "RAW");

        اگر (m_آخری_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_آخری_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_علامات_On_سطر = 0;
        } ورنہ اگر (++m_علامات_On_سطر > v3Global.opt.preproc_Token_حد_بندی()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_مقصد_Str(v3Global.opt.preproc_Token_حد_بندی())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        اگر (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            اگر (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            جاؤ اگلا_tok;
        }

        اگر (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        واپس tok;
    }
}

خالی V3PreProcImp::debugToken(صحیح tok, ثابت حرف* cmtp) {
    ثابت ساکن صحیح s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    اگر (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::سائز_type pos;
        جب ((pos = buf.ڈھونڈو('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        جب ((pos = buf.ڈھونڈو('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_ستون();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<صحیح>(m_حالتیں.سائز())
                       << ") dr" << m_defRefs.سائز() << ":  <" << m_lexp->current_آغاز_State()
                       << ">" << tokenName(tok) << ": " << buf);
        اگر (s_debugFileline >= 9) {
            std::خارج << m_lexp->m_tokFilelinep->warnContextSecondary() << سطر_ختم;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

صحیح V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    جب (سچ) {
    اگلا_tok:
        اگر (isEof()) واپس VP_EOF;
        صحیح tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        اگر (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) واپس tok;
        اگر (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_متن;
        اگر (tok == VP_COMMENT) {
            اگر (!m_off) {
                اگر (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.تفویض_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    اگر (m_سطر_Cmt.خالی_خانے()) داخل_کرو_Unreadback(" ");
                } ورنہ اگر (m_lexp->m_keepComments) {
                    واپس tok;
                } ورنہ {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    داخل_کرو_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            برائے (سائز_t len = 0; len < static_cast<سائز_t>(yyourleng()); len++) {
                اگر (yyourtext()[len] == '\n') m_سطر_جوڑو++;
            }
            جاؤ اگلا_tok;
        }
        اگر (tok == VP_سطر) {
            جوڑو_سطر_Comment(m_lexp->m_enterExit);
            جاؤ اگلا_tok;
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
                m_defPutJoin = سچ;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } ورنہ {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        اگر (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            اگر (tok == VP_JOIN) جاؤ اگلا_tok;
            اگر (tok == VP_علامت_JOIN) tok = VP_علامت;
        }
        اگر (tok == VP_علامت_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ثابت std::string برآمد(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << برآمد);
            // a``b``c can have multiple joins, so we need a stack
            m_join_ڈھیر.ڈالو(برآمد);
            state_ڈالو(ps_JOIN);
            جاؤ اگلا_tok;
        }

        // Deal with some special parser states
        انتخاب (state()) {
        حالت ps_چوٹی: {
            توڑو;
        }
        حالت ps_DEFNAME_UNDEF:  // FALLTHRU
        حالت ps_DEFNAME_DEFINE:  // FALLTHRU
        حالت ps_DEFNAME_IFDEF:  // FALLTHRU
        حالت ps_DEFNAME_IFNDEF:  // FALLTHRU
        حالت ps_DEFNAME_ELSIF: {
            اگر (tok == VP_علامت) {
                m_آخری_Sym.تفویض_2(yyourtext(), yyourleng());
                اگر (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    منطقی enable = defExists(m_آخری_Sym);
                    UINFO(4, "Ifdef " << m_آخری_Sym << (enable ? " ON" : " OFF"));
                    اگر (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, جھوٹ});
                    اگر (!enable) parsingOff();
                    state_نکالو();
                    جاؤ اگلا_tok;
                } ورنہ اگر (state() == ps_DEFNAME_ELSIF) {
                    اگر (m_ifdef_ڈھیر.خالی_خانے()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } ورنہ {
                        // Handle `else portion
                        ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                        m_ifdef_ڈھیر.نکالو();
                        اگر (!آخری_If.on()) parsingOn();
                        // Handle `if portion
                        ثابت منطقی enable = !آخری_If.everOn() && defExists(m_آخری_Sym);
                        UINFO(4, "Elsif " << m_آخری_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, آخری_If.everOn()});
                        اگر (!enable) parsingOff();
                    }
                    state_نکالو();
                    جاؤ اگلا_tok;
                } ورنہ اگر (state() == ps_DEFNAME_UNDEF) {
                    اگر (!m_off) {
                        UINFO(4, "Undef " << m_آخری_Sym);
                        undef(m_آخری_Sym);
                    }
                    state_نکالو();
                    جاؤ اگلا_tok;
                } ورنہ اگر (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->ڈالو_State_Def_Form();
                    جاؤ اگلا_tok;
                } ورنہ {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن) {
                // IE, something like comment between define and symbol
                اگر (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->ڈالو_State_اظہاریہ();
                    m_اظہاریہ_Parser.reset(fileline());
                    m_اظہاریہ_Paren_Level = 1;
                    انتخاب (state()) {
                    حالت ps_DEFNAME_IFDEF: stateChange(ps_اظہاریہ_IFDEF); توڑو;
                    حالت ps_DEFNAME_IFNDEF: stateChange(ps_اظہاریہ_IFNDEF); توڑو;
                    حالت ps_DEFNAME_ELSIF: stateChange(ps_اظہاریہ_ELSIF); توڑو;
                    طےشدہ: v3fatalSrc("bad case");
                    }
                    جاؤ اگلا_tok;
                }
                اگر (!m_off) {
                    واپس tok;
                } ورنہ {
                    جاؤ اگلا_tok;
                }
            } ورنہ اگر (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                توڑو;
            } ورنہ {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                جاؤ اگلا_tok;
            }
        }
        حالت ps_اظہاریہ_IFDEF:  // FALLTHRU
        حالت ps_اظہاریہ_IFNDEF:  // FALLTHRU
        حالت ps_اظہاریہ_ELSIF: {
            // `ifdef ( *here*
            File_سطر* ثابت flp = m_lexp->m_tokFilelinep;
            اگر (tok == VP_علامت) {
                m_آخری_Sym.تفویض_2(yyourtext(), yyourleng());
                ثابت منطقی exists = defExists(m_آخری_Sym);
                اگر (exists) {
                    ثابت std::string قدر = def_قدر(m_آخری_Sym);
                    اگر (VString::removeWhitespace(قدر) == "0") {
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
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_WHITE) {
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::BRA});
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_اظہاریہ_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                اگر (--m_اظہاریہ_Paren_Level > 0) {
                    m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::KET});
                    جاؤ اگلا_tok;
                } ورنہ {
                    // Done with parsing expression
                    منطقی enable = m_اظہاریہ_Parser.نتیجہ();
                    UINFO(4, "ifdef() result=" << enable);
                    اگر (state() == ps_اظہاریہ_IFDEF || state() == ps_اظہاریہ_IFNDEF) {
                        اگر (state() == ps_اظہاریہ_IFNDEF) enable = !enable;
                        m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, جھوٹ});
                        اگر (!enable) parsingOff();
                        state_نکالو();
                        جاؤ اگلا_tok;
                    } ورنہ اگر (state() == ps_اظہاریہ_ELSIF) {
                        اگر (m_ifdef_ڈھیر.خالی_خانے()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } ورنہ {
                            // Handle `else portion
                            ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                            m_ifdef_ڈھیر.نکالو();
                            اگر (!آخری_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !آخری_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_آخری_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, آخری_If.everOn()});
                            اگر (!enable) parsingOff();
                        }
                        state_نکالو();
                    }
                    جاؤ اگلا_tok;
                }
            } ورنہ اگر (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LNOT});
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 2 && 0 == لڑی_موازنہ(yyourtext(), "&&", 2)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LAND});
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 2 && 0 == لڑی_موازنہ(yyourtext(), "||", 2)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LOR});
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 2 && 0 == لڑی_موازنہ(yyourtext(), "->", 2)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::IMP});
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 3 && 0 == لڑی_موازنہ(yyourtext(), "<->", 3)) {
                m_اظہاریہ_Parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::EQV});
                جاؤ اگلا_tok;
            } ورنہ {
                اگر (VString::removeWhitespace(string{yyourtext(), yyourleng()}).خالی_خانے()) {
                    واپس tok;
                } ورنہ {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                جاؤ اگلا_tok;
            }
        }
        حالت ps_DEFFORM: {
            اگر (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_قدر;
                اگر (debug() >= 5) {
                    خارج << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->ڈالو_State_Def_قدر();
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن) {
                // IE, something like comment in formals
                اگر (!m_off) واپس tok;
                جاؤ اگلا_tok;
            } ورنہ {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                جاؤ اگلا_tok;
            }
        }
        حالت ps_DEFVALUE: {
            ساکن string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            اگر (tok == VP_DEFVALUE) {
                اگر (debug() >= 5) {  // LCOV_EXCL_START
                    خارج << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_قدر)
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
                برائے (سائز_t i = 0; i < formals.لمبائی_2(); i++) {
                    اگر (formals[i] == '\n') s_newlines += "\n";
                }
                برائے (سائز_t i = 0; i < قدر.لمبائی_2(); i++) {
                    اگر (قدر[i] == '\n') s_newlines += "\n";
                }
                اگر (!m_off) {
                    // Remove leading and trailing whitespace
                    قدر = trimWhitespace(قدر, سچ);
                    // Define it
                    UINFO(4, "Define " << m_آخری_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(قدر) << "'");
                    define(fileline(), m_آخری_Sym, قدر, formals, جھوٹ);
                }
            } ورنہ {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_نکالو();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.لمبائی_2());
            واپس VP_WHITE;
        }
        حالت ps_DEFPAREN: {
            اگر (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                جاؤ اگلا_tok;
            } ورنہ {
                UASSERT(!m_defRefs.خالی_خانے(), "Shouldn't be in DEFPAREN w/o active defref");
                ثابت VDefineRef* ثابت refp = &(m_defRefs.چوٹی());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_نکالو();
                جاؤ اگلا_tok;
            }
        }
        حالت ps_DEFARG: {
            UASSERT(!m_defRefs.خالی_خانے(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.چوٹی());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_قدر);
            m_lexp->m_def_قدر = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            اگر (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->دلائل().ڈالو_عقب(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->ڈالو_State_Def_Arg(1);
                refp->nextarg("");
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->دلائل().ڈالو_عقب(refp->nextarg());
                string برآمد;
                اگر (!m_off) {
                    برآمد = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.نکالو(), refp);
                state_نکالو();
                اگر (m_defRefs.خالی_خانے() || state() == ps_STRIFY || state() == ps_JOIN) {
                    اگر (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_ڈھیر.خالی_خانے(), "`` join stack empty, but in a ``");
                        ثابت string lhs = m_join_ڈھیر.چوٹی();
                        m_join_ڈھیر.نکالو();
                        برآمد.داخل_کرو(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << برآمد);
                        state_نکالو();
                    }
                    اگر (!m_off) {
                        unputDefrefString(برآمد);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    ورنہ اگر (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.خالی_خانے() ? 0 : m_defRefs.چوٹی().parenLevel();
                } ورنہ {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.چوٹی());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_قدر + برآمد);
                    m_lexp->m_def_قدر = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                توڑو;
            } ورنہ اگر (tok == VP_علامت || tok == VP_STRING || tok == VP_متن || tok == VP_WHITE) {
                string rtn;
                rtn.تفویض_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_ڈالو(ps_STRIFY);
                جاؤ اگلا_tok;
            } ورنہ {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_نکالو();
                جاؤ اگلا_tok;
            }
        }
        حالت ps_INCNAME: {
            اگر (tok == VP_STRING) {
                state_نکالو();
                m_آخری_Sym.تفویض_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_آخری_Sym);
                // Drop leading and trailing quotes.
                m_آخری_Sym.erase(0, 1);
                m_آخری_Sym.erase(m_آخری_Sym.لمبائی_2() - 1, 1);
                include(m_آخری_Sym);
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_متن && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->ڈالو_State_Inc_Filename();
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                توڑو;
            } ورنہ {
                state_نکالو();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                جاؤ اگلا_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        حالت ps_ERRORNAME: {
            اگر (tok == VP_STRING) {
                اگر (!m_off) {
                    m_آخری_Sym.تفویض_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_آخری_Sym);
                }
                state_نکالو();
                جاؤ اگلا_tok;
            } ورنہ {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_نکالو();
                جاؤ اگلا_tok;
            }
        }
        حالت ps_JOIN: {
            اگر (tok == VP_علامت || tok == VP_متن || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_ڈھیر.خالی_خانے(), "`` join stack empty, but in a ``");
                ثابت string lhs = m_join_ڈھیر.چوٹی();
                m_join_ڈھیر.نکالو();
                UINFO(5, "`` LHS:" << lhs);
                ثابت string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ثابت string برآمد = lhs + rhs;
                UINFO(5, "`` Out:" << برآمد);
                unputString(برآمد);
                state_نکالو();
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ثابت string lhs = m_join_ڈھیر.چوٹی();
                m_join_ڈھیر.نکالو();
                unputString(lhs);
                state_نکالو();
                جاؤ اگلا_tok;
            } ورنہ {
                // `define, etc, fall through and expand.  Pop back here.
                توڑو;
            }
        }
        حالت ps_STRIFY: {
            اگر (tok == VP_STRIFY) {
                // Quote what's in the middle of the stringification
                // Note a `" MACRO_WITH(`") `" doesn't need to be handled (we don't need a stack)
                // That behavior isn't specified, and other simulators vary widely
                string برآمد = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(برآمد.begin(), برآمد.انتہا(), '\n', ' ');
                unputString("\""s + برآمد + "\"");
                state_نکالو();
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                توڑو;
            } ورنہ اگر (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                جاؤ اگلا_tok;
            } ورنہ اگر (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                توڑو;
            } ورنہ {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                جاؤ اگلا_tok;
            }
        }
        طےشدہ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        انتخاب (tok) {
        حالت VP_INCLUDE:
            اگر (!m_off) {
                state_ڈالو(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            جاؤ اگلا_tok;
        حالت VP_UNDEF: state_ڈالو(ps_DEFNAME_UNDEF); جاؤ اگلا_tok;
        حالت VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_ڈالو(ps_DEFNAME_DEFINE);
            جاؤ اگلا_tok;
        حالت VP_IFDEF: state_ڈالو(ps_DEFNAME_IFDEF); جاؤ اگلا_tok;
        حالت VP_IFNDEF: state_ڈالو(ps_DEFNAME_IFNDEF); جاؤ اگلا_tok;
        حالت VP_ELSIF: state_ڈالو(ps_DEFNAME_ELSIF); جاؤ اگلا_tok;
        حالت VP_ELSE:
            اگر (m_ifdef_ڈھیر.خالی_خانے()) {
                fileline()->v3error("`else with no matching `if");
            } ورنہ {
                ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                m_ifdef_ڈھیر.نکالو();
                ثابت منطقی enable = !آخری_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_ڈھیر.emplace(VPre_If_اندراج{enable, آخری_If.everOn()});
                اگر (!آخری_If.on()) parsingOn();
                اگر (!enable) parsingOff();
            }
            جاؤ اگلا_tok;
        حالت VP_ENDIF:
            UINFO(4, "Endif ");
            اگر (m_ifdef_ڈھیر.خالی_خانے()) {
                fileline()->v3error("`endif with no matching `if");
            } ورنہ {
                ثابت VPre_If_اندراج آخری_If = m_ifdef_ڈھیر.چوٹی();
                m_ifdef_ڈھیر.نکالو();
                اگر (!آخری_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            جاؤ اگلا_tok;

        حالت VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ثابت string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            اگر (m_defPutJoin) {
                m_defPutJoin = جھوٹ;
                unputString("``");
            }
            اگر (m_def_گہرائی++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                جاؤ اگلا_tok;
            }
            // Substitute
            اگر (!defExists(name)) {  // Not found, return original string as-is
                m_def_گہرائی = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                اگر (m_off) {
                    جاؤ اگلا_tok;
                } ورنہ {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    جاؤ اگلا_tok;
                }
            } ورنہ {
                ثابت string params = defParams(name);
                اگر (params == "0" || params == "") {  // Found, as simple substitution
                    string برآمد;
                    اگر (!m_off) {
                        VDefineRef tempref{name, ""};
                        برآمد = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    اگر (m_defRefs.خالی_خانے() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        اگر (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_ڈھیر.خالی_خانے(), "`` join stack empty, but in a ``");
                            ثابت string lhs = m_join_ڈھیر.چوٹی();
                            m_join_ڈھیر.نکالو();
                            برآمد.داخل_کرو(0, lhs);
                            UINFO(5, "``-end-defref Out:" << برآمد);
                            state_نکالو();
                        }
                        اگر (!m_off) {
                            unputDefrefString(برآمد);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        ورنہ اگر (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } ورنہ {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* ثابت refp = &(m_defRefs.چوٹی());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_قدر + برآمد);
                        m_lexp->m_def_قدر = "";
                    }
                    جاؤ اگلا_tok;
                } ورنہ {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    اگر (!m_defRefs.خالی_خانے()) m_defRefs.چوٹی().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_ڈالو(ps_DEFPAREN);
                    m_lexp->ڈالو_State_Def_Arg(0);
                    جاؤ اگلا_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            جاؤ اگلا_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        حالت VP_ERROR: {
            state_ڈالو(ps_ERRORNAME);
            جاؤ اگلا_tok;
        }
        حالت VP_EOF:
            اگر (!m_ifdef_ڈھیر.خالی_خانے()) fileline()->v3error("`ifdef not terminated at EOF");
            واپس tok;
        حالت VP_UNDEFINEALL:
            اگر (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            جاؤ اگلا_tok;
        حالت VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_ڈالو(ps_STRIFY);
            جاؤ اگلا_tok;
        حالت VP_علامت:
        حالت VP_STRING:
        حالت VP_متن: {
            m_def_گہرائی = 0;
            اگر (!m_off) {
                واپس tok;
            } ورنہ {
                جاؤ اگلا_tok;
            }
        }
        حالت VP_WHITE:  // Handled at top of loop
        حالت VP_COMMENT:  // Handled at top of loop
        حالت VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        حالت VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        طےشدہ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            توڑو;  // LCOV_EXCL_LINE
        }
        واپس tok;
    }
}

صحیح V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    اگر (!m_finAhead) {
        m_finAhead = سچ;
        m_finToken = getStateToken();
        m_finBuf.تفویض_2(yyourtext(), yyourleng());
    }
    ثابت صحیح tok = m_finToken;
    buf = m_finBuf;
    اگر (جھوٹ && debug() >= 5) {
        ثابت string bufcln = V3PreLex::cleanDbgStrg(buf);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_ستون();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ثابت حرف* bufp = buf.c_str();
    جب (*bufp == '\n') bufp++;
    اگر ((tok == VP_متن || tok == VP_سطر) && VString::startsWith(bufp, "`line ")) {
        صحیح enter;
        m_finFilelinep->سطر_Directive(bufp, enter /*ref*/);
    } ورنہ {
        اگر (m_finAtBol && !(tok == VP_متن && buf == "\n") && m_preprocp->سطر_Directives()) {
            اگر (ثابت صحیح برآمد_Behind
                = (m_lexp->m_tokFilelinep->آخری_Lineno() - m_finFilelinep->آخری_Lineno())) {
                اگر (debug() >= 5) {
                    ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_ستون();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->آخری_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->آخری_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->آخری_Lineno());
                اگر (برآمد_Behind > 0
                    && (برآمد_Behind <= static_cast<صحیح>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    اگر (m_preprocp->keepWhitespace()) {
                        buf = std::string(برآمد_Behind, '\n');  // () for char repeat
                        واپس VP_متن;
                    }
                } ورنہ {
                    // Need to backup, use `line
                    buf = m_finFilelinep->سطر_Directive_Strg(0);
                    واپس VP_سطر;
                }
            }
        }
        // Track newlines in prep for next token
        برائے (ثابت حرف& c : buf) {
            اگر (c == '\n') {
                m_finAtBol = سچ;
                m_finFilelinep->linenoInc();
            } ورنہ {
                m_finAtBol = جھوٹ;
            }
        }
    }
    m_finAhead = جھوٹ;  // Consumed the token
    واپس tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    اگر (isEof()) واپس "";
    ثابت حرف* rtnp;
    منطقی gotEof = جھوٹ;
    جب (خالی_اشاریہ == (rtnp = std::strchr(m_سطر_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ثابت صحیح tok = getFinalToken(m_tokenBuf /*ref*/);
        اگر (debug() >= 5) {
            ثابت string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_ستون();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        اگر (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            اگر (m_سطر_Chars != "" && m_سطر_Chars[m_سطر_Chars.لمبائی_2() - 1] != '\n') {
                m_سطر_Chars.append("\n");
            }
            gotEof = سچ;
        } ورنہ {
            m_سطر_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ثابت صحیح len = rtnp - m_سطر_Chars.c_str() + 1;
    string the_سطر(m_سطر_Chars, 0, len);
    m_سطر_Chars = m_سطر_Chars.erase(0, len);  // Remove returned characters
    اگر (debug() >= 4) {
        ثابت string lncln = V3PreLex::cleanDbgStrg(the_سطر);
        ثابت string flcol = m_lexp->m_tokFilelinep->ascii_سطر_ستون();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    واپس the_سطر;
}
