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

#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

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

#परिभाषय yyFlexLexer V3Lexer
#अन्तर्भाव "V3PreLex.yy.cpp"
#अपरिभाषय yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

वर्ग VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_पङ्क्तिः* ध्रुव m_fileline;  // Where it was declared
    ध्रुव string m_मानम्;  // Value of define
    ध्रुव string m_params;  // Parameters
    ध्रुव तार्किक m_cmdline;  // Set on command line, don't `undefineall
सार्वजनिक:
    VDefine(File_पङ्क्तिः* fl, ध्रुव string& मानम्, ध्रुव string& params, तार्किक cmdline)
        : m_fileline{fl}
        , m_मानम्{मानम्}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_fileline; }
    string मानम्() ध्रुव { प्रत्यागम m_मानम्; }
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

    std::सदिशम्<string> m_तर्काः;  // List of define arguments
सार्वजनिक:
    string name() ध्रुव { प्रत्यागम m_name; }
    string params() ध्रुव { प्रत्यागम m_params; }
    string nextarg() ध्रुव { प्रत्यागम m_nextarg; }
    शून्य nextarg(ध्रुव string& मानम्) { m_nextarg = मानम्; }
    पूर्णाङ्क parenLevel() ध्रुव { प्रत्यागम m_parenLevel; }
    शून्य parenLevel(पूर्णाङ्क मानम्) { m_parenLevel = मानम्; }
    std::सदिशम्<string>& तर्काः() { प्रत्यागम m_तर्काः; }
    VDefineRef(ध्रुव string& name, ध्रुव string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = स्वभाव;
};

//*************************************************************************
/// Data for parsing on/off

वर्ग VPre_If_प्रविष्टिः final {
    // One for each pending ifdef/ifndef
    ध्रुव तार्किक m_on;  // Current parse for this ifdef level is "on"
    ध्रुव तार्किक m_everOn;  // Some if term in elsif tree has been on
सार्वजनिक:
    तार्किक on() ध्रुव { प्रत्यागम m_on; }
    तार्किक everOn() ध्रुव { प्रत्यागम m_everOn; }
    VPre_If_प्रविष्टिः(तार्किक on, तार्किक everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_प्रविष्टिः() = स्वभाव;
};

//*************************************************************************
// Data for a preprocessor instantiation.

वर्ग V3PreProcImp final : सार्वजनिक V3PreProc {
सार्वजनिक:
    // TYPES
    उपयुज्य परिभाषते_Map = std::map<ध्रुव std::string, VDefine>;
    उपयुज्य StrList = VInFilter::StrList;

    // Defines list
    परिभाषते_Map m_परिभाषते;  ///< Map of defines

    // STATE
    ध्रुव V3PreProc* m_preprocp = शून्यसूचकः;  ///< Object we're holding data for
    V3PreLex* m_lexp = शून्यसूचकः;  ///< Current lexer state (nullptr = closed)
    पूर्णाङ्क m_चरमः_Lineno = 0;  // Last line number (stall detection)
    पूर्णाङ्क m_tokens_On_पङ्क्तिः = 0;  // Number of tokens on line (stall detection)

    गणना ProcState : uint8_t {
        ps_शिखरम्,
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
        ps_व्यञ्जकम्_IFDEF,
        ps_व्यञ्जकम्_IFNDEF,
        ps_व्यञ्जकम्_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    स्थिर ध्रुव वर्ण* procStateName(ProcState s) {
        स्थिर ध्रुव वर्ण* ध्रुव अवस्थाः[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        प्रत्यागम अवस्थाः[s];
    }

    std::राशिः<ProcState> m_अवस्थाः;  ///< Current state of parser
    पूर्णाङ्क m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    तार्किक m_incError = असत्य;  ///< Include error found
    string m_चरमः_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_पङ्क्तिः_Cmt;  ///< Line comment(s) to be returned
    तार्किक m_पङ्क्तिः_Cmt_Nl = असत्य;  ///< Newline needed before inserting lineCmt
    पूर्णाङ्क m_पङ्क्तिः_सङ्कलय = 0;  ///< Empty lines to return to maintain line count
    तार्किक m_rawAtBol = सत्य;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    तार्किक m_finAhead = असत्य;  ///< Have read a token ahead
    पूर्णाङ्क m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    तार्किक m_finAtBol = सत्य;  ///< Last getFinalToken left us at beginning of line
    File_पङ्क्तिः* m_finFilelinep = शून्यसूचकः;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::राशिः<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::राशिः<VPre_If_प्रविष्टिः> m_ifdef_राशिः;  ///< Stack of true/false emitting evaluations
    अचिह्नित m_def_गभीरता = 0;  ///< How many `defines deep
    तार्किक m_defPutJoin = असत्य;  ///< Insert `` after substitution

    // For `` join
    std::राशिः<string> m_join_राशिः;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_व्यञ्जकम् m_व्यञ्जकम्_Parser;  ///< Parser for () expression
    पूर्णाङ्क m_व्यञ्जकम्_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_पङ्क्तिः_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    शून्य v3error_अन्तः(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_अन्तः(str);
    }

    स्थिर ध्रुव वर्ण* tokenName(पूर्णाङ्क tok);
    शून्य debugToken(पूर्णाङ्क tok, ध्रुव वर्ण* cmtp);
    शून्य parse_शिखरम्();
    शून्य parseUndef();

गुप्त:
    // Internal methods
    शून्य अन्तः_सम्बन्धि_One_File();
    string defineSubst(VDefineRef* refp);

    तार्किक defExists(ध्रुव string& name);
    तार्किक defCmdline(ध्रुव string& name);
    string def_मानम्(ध्रुव string& name);
    string defParams(ध्रुव string& name);
    File_पङ्क्तिः* defFileline(ध्रुव string& name);

    string commentCleanup(ध्रुव string& text);
    तार्किक comment_Token_मेलनम्(string& cmdr, ध्रुव वर्ण* strg);
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

    ProcState state() ध्रुव { प्रत्यागम m_अवस्थाः.शिखरम्(); }
    तार्किक stateIsDefname() ध्रुव {
        प्रत्यागम state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    शून्य state_प्रक्षिप(ProcState state) { m_अवस्थाः.प्रक्षिप(state); }
    शून्य state_निष्कासय() {
        m_अवस्थाः.निष्कासय();
        यदि (VL_UNCOVERABLE(m_अवस्थाः.रिक्तानि())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_अवस्थाः.प्रक्षिप(ps_शिखरम्);  // LCOV_EXCL_LINE
        }
    }
    शून्य stateChange(ProcState state) {
        state_निष्कासय();
        state_प्रक्षिप(state);
    }

सार्वजनिक:
    // METHODS, called from upper level shell
    शून्य openFile(File_पङ्क्तिः* fl, VInFilter* filterp, ध्रुव string& filename) override;
    तार्किक isEof() ध्रुव override { प्रत्यागम m_lexp->curStreamp()->m_eof; }
    शून्य forceEof() { m_lexp->curStreamp()->m_eof = सत्य; }
    string getline() override;
    शून्य निवेशय_Unreadback(ध्रुव string& text) override { m_पङ्क्तिः_Cmt += text; }
    शून्य निवेशय_Unreadback_At_Bol(ध्रुव string& text);
    शून्य सङ्कलय_पङ्क्तिः_Comment(पूर्णाङ्क enterExit);
    शून्य dump_परिभाषते(std::ostream& os) override;
    शून्य candidate_परिभाषते(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    शून्य comment(ध्रुव string& text) override;  // Comment detected (if keepComments==2)
    शून्य include(ध्रुव string& filename) override;  // Request a include file be processed
    शून्य undef(ध्रुव string& name) override;
    आभास शून्य undefineall();
    शून्य define(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& मानम्, ध्रुव string& params,
                तार्किक cmdline) override;
    string remove_परिभाषते(ध्रुव string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_अवस्थाः.प्रक्षिप(ps_शिखरम्); }
    // cppcheck-suppress duplInheritedMember
    शून्य configure(File_पङ्क्तिः* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = एतत्;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = नूतन File_पङ्क्तिः{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = नूतन V3PreLex{एतत्, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        यदि (m_lexp) VL_DO_CLEAR(विलोपय m_lexp, m_lexp = शून्यसूचकः);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_पङ्क्तिः* fl) {
    V3PreProcImp* ध्रुव preprocp = नूतन V3PreProcImp;
    preprocp->configure(fl);
    प्रत्यागम preprocp;
}

शून्य V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_व्यञ्जकम्::selfTest(); }

//*************************************************************************
//परिभाषते

शून्य V3PreProcImp::undef(ध्रुव string& name) {
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preproc_परिभाषते())
        निवेशय_Unreadback("`undef " + name + "\n");
    m_परिभाषते.erase(name);
}
शून्य V3PreProcImp::undefineall() {
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preproc_परिभाषते())
        निवेशय_Unreadback("`undefineall\n");
    प्रति (परिभाषते_Map::iterator nextit, it = m_परिभाषते.begin(); it != m_परिभाषते.अन्तः_2(); it = nextit) {
        nextit = it;
        ++nextit;
        यदि (!it->second.cmdline()) m_परिभाषते.erase(it);
    }
}
तार्किक V3PreProcImp::defExists(ध्रुव string& name) {
    ध्रुव स्वचालित iter = m_परिभाषते.find(name);
    प्रत्यागम (iter != m_परिभाषते.अन्तः_2());
}
तार्किक V3PreProcImp::defCmdline(ध्रुव string& name) {
    ध्रुव स्वचालित iter = m_परिभाषते.find(name);
    यदि (iter == m_परिभाषते.अन्तः_2()) { प्रत्यागम असत्य; }
    प्रत्यागम iter->second.cmdline();
}
string V3PreProcImp::def_मानम्(ध्रुव string& name) {
    ध्रुव स्वचालित iter = m_परिभाषते.find(name);
    यदि (iter == m_परिभाषते.अन्तः_2()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        प्रत्यागम "";
    }
    प्रत्यागम iter->second.मानम्();
}
string V3PreProcImp::defParams(ध्रुव string& name) {
    ध्रुव स्वचालित iter = m_परिभाषते.find(name);
    यदि (iter == m_परिभाषते.अन्तः_2()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        प्रत्यागम "";
    }
    प्रत्यागम iter->second.params();
}
File_पङ्क्तिः* V3PreProcImp::defFileline(ध्रुव string& name) {
    ध्रुव स्वचालित iter = m_परिभाषते.find(name);
    यदि (iter == m_परिभाषते.अन्तः_2()) प्रत्यागम शून्यसूचकः;
    प्रत्यागम iter->second.fileline();
}
शून्य V3PreProcImp::define(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& मानम्,
                          ध्रुव string& params, तार्किक cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << मानम् << "' params '" << params << "'");
    यदि (v3Global.opt.preprocOnly() && v3Global.opt.preproc_परिभाषते()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string बहिर्गमनम्;
        बहिर्गमनम्.reserve(मानम्.आकार());
        प्रति (string::आकार_type pos = 0; pos < मानम्.आकार(); ++pos) {
            यदि (मानम्[pos] == '\n' && (pos == 0 || मानम्[pos - 1] != '\\')) बहिर्गमनम् += '\\';
            बहिर्गमनम् += मानम्[pos];
        }
        निवेशय_Unreadback("`define " + name + params + (बहिर्गमनम्.रिक्तानि() ? "" : " ") + बहिर्गमनम् + "\n");
    }
    यदि (!V3_Language_शब्दाः::isKeyword("`"s + name).रिक्तानि()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } अन्यथा {
        यदि (defExists(name)) {
            यदि (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << मानम्
                               << "' to existing command line define value: '" << def_मानम्(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_मानम्(name) << (defParams(name).रिक्तानि() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                प्रत्यागम;
            } अन्यथा {
                यदि (!(def_मानम्(name) == मानम्
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << मानम्
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_मानम्(name) << (defParams(name).रिक्तानि() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_परिभाषते.emplace(name, VDefine{fl, मानम्, params, cmdline});
    }
}

string V3PreProcImp::remove_परिभाषते(ध्रुव string& text) {
    string val;
    string rtnsym = text;
    प्रति (पूर्णाङ्क loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        यदि (xsym[0] == '`') xsym.erase(0, 1);
        यदि (defExists(xsym)) {
            val = def_मानम्(xsym);
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
    string::आकार_type pos;
    यावत् ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    यावत् ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    यावत् ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    यावत् ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    यावत् ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    यावत् ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    यावत् (!cmd.रिक्तानि() && std::रिक्तम्_वा(cmd[cmd.आकार() - 1])) cmd.erase(cmd.आकार() - 1);
    प्रत्यागम cmd;
}

तार्किक V3PreProcImp::comment_Token_मेलनम्(string& cmdr, ध्रुव वर्ण* strg) {
    पूर्णाङ्क len = std::वाक्यदैर्घ्यम्(strg);
    यदि (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::रिक्तम्_वा(cmdr[len]))) {
        यदि (std::रिक्तम्_वा(cmdr[len])) len++;
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
        निवेशय_Unreadback(text);
        printed = सत्य;
    }

    ध्रुव वर्ण* cp = text.c_str();
    यदि (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } अन्यथा {
        प्रत्यागम;
    }

    यावत् (std::रिक्तम्_वा(*cp)) ++cp;

    तार्किक synth = असत्य;
    तार्किक vlcomment = असत्य;
    यदि ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::वाक्यदैर्घ्यम्("verilator");
        यदि (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            प्रत्यागम;
        }
        vlcomment = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "synopsys")) {
        cp += std::वाक्यदैर्घ्यम्("synopsys");
        यदि (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            प्रत्यागम;
        }
        synth = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "cadence")) {
        cp += std::वाक्यदैर्घ्यम्("cadence");
        synth = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "pragma")) {
        cp += std::वाक्यदैर्घ्यम्("pragma");
        synth = सत्य;
    } अन्यथा यदि (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::वाक्यदैर्घ्यम्("ambit synthesis");
        synth = सत्य;
    } अन्यथा {
        प्रत्यागम;
    }

    यावत् (std::रिक्तम्_वा(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    यदि (synth) {
        यदि (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            यदि (comment_Token_मेलनम्(cmd /*ref*/, "full_case")) {
                यदि (!printed) निवेशय_Unreadback("/*verilator full_case*/");
            }
            यदि (comment_Token_मेलनम्(cmd /*ref*/, "parallel_case")) {
                यदि (!printed) निवेशय_Unreadback("/*verilator parallel_case*/");
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
               && !(v3Global.opt.लौकिकम्_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        यदि (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::आकार_type अन्तः_सम्बन्धि_Cmd = std::वाक्यदैर्घ्यम्("public_flat_rw");
            यावत् (VString::isIdentifierChar(cmd[अन्तः_सम्बन्धि_Cmd])) ++अन्तः_सम्बन्धि_Cmd;
            string आधारः_Cmd = cmd.substr(0, अन्तः_सम्बन्धि_Cmd);
            string arg = cmd.substr(अन्तः_सम्बन्धि_Cmd);
            यावत् (std::रिक्तम्_वा(arg[0])) arg = arg.substr(1);
            यदि (arg.आकार() && आधारः_Cmd == "public_flat_rw_on")
                आधारः_Cmd += "_sns";  // different cmd for applying sensitivity
            यदि (!printed) निवेशय_Unreadback("/*verilator " + आधारः_Cmd + "*/ " + arg + " /**/");
        } अन्यथा यदि (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string आधारः_Cmd = cmd.substr(0, std::वाक्यदैर्घ्यम्("dpi_c_decl"));
            string::आकार_type start_सम्बन्धि_Arg = आधारः_Cmd.आकार();
            यावत् (std::रिक्तम्_वा(cmd[start_सम्बन्धि_Arg])) start_सम्बन्धि_Arg++;
            string arg = cmd.substr(start_सम्बन्धि_Arg);
            यदि (arg.रिक्तानि()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                प्रत्यागम;
            }
            यदि (!printed) निवेशय_Unreadback("/*verilator " + आधारः_Cmd + "*/ \"" + arg + "\" /**/");
        } अन्यथा {
            यदि (!printed) निवेशय_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_पङ्क्तिः* V3PreProc::fileline() {
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
    पक्ष VP_पङ्क्तिः: प्रत्यागम ("LINE");
    पक्ष VP_STRIFY: प्रत्यागम ("STRIFY");
    पक्ष VP_STRING: प्रत्यागम ("STRING");
    पक्ष VP_चिह्नम्: प्रत्यागम ("SYMBOL");
    पक्ष VP_चिह्नम्_JOIN: प्रत्यागम ("SYMBOL_JOIN");
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
    यदि (VL_UNCOVERABLE(m_lexp->m_बफर_State != m_lexp->current_बफर())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->अवलोकनम्_अष्टकानि(strg);
}

शून्य V3PreProcImp::unputDefrefString(ध्रुव string& strg) {
    ध्रुव पूर्णाङ्क multiline = std::गणनम्(strg.begin(), strg.अन्तः_2(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(ध्रुव string& strg, तार्किक trailing) {
    // Remove leading whitespace
    string बहिर्गमनम् = strg;
    string::आकार_type leadspace = 0;
    यावत् (बहिर्गमनम्.दीर्घता_2() > leadspace && std::रिक्तम्_वा(बहिर्गमनम्[leadspace])) ++leadspace;
    यदि (leadspace) बहिर्गमनम्.erase(0, leadspace);
    // Remove trailing whitespace
    यदि (trailing) {
        string::आकार_type trailspace = 0;
        यावत् (बहिर्गमनम्.दीर्घता_2() > trailspace && std::रिक्तम्_वा(बहिर्गमनम्[बहिर्गमनम्.दीर्घता_2() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        यदि (trailspace && बहिर्गमनम्.दीर्घता_2() > trailspace && बहिर्गमनम्[बहिर्गमनम्.दीर्घता_2() - 1 - trailspace] == '\\')
            --trailspace;
        यदि (trailspace) बहिर्गमनम्.erase(बहिर्गमनम्.दीर्घता_2() - trailspace, trailspace);
    }
    प्रत्यागम बहिर्गमनम्;
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
    प्रति (अचिह्नित i = 0; i < refp->तर्काः().आकार(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->तर्काः()[i] << "'");
    }
    // Grab value
    ध्रुव string मानम् = def_मानम्(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(मानम्) << "'");

    std::map<ध्रुव string, string> arg_मानम्_By_Name;
    {  // Parse argument list into map
        अचिह्नित अङ्कः_तर्काः = 0;
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
                    string मानम्_Def;
                    यदि (haveDefault) {
                        मानम्_Def = token;
                    } अन्यथा {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, सत्य);
                    UINFO(4, "    Got Arg=" << अङ्कः_तर्काः << "  argName='" << argName
                                            << "'  default='" << मानम्_Def << "'");
                    // Parse it
                    यदि (argName != "") {
                        यदि (refp->तर्काः().आकार() > अङ्कः_तर्काः) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            ध्रुव string arg = trimWhitespace(refp->तर्काः()[अङ्कः_तर्काः], सत्य);
                            यदि (arg != "") मानम्_Def = arg;
                        } अन्यथा यदि (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            प्रत्यागम " `" + refp->name() + " ";
                        }
                        अङ्कः_तर्काः++;
                    }
                    arg_मानम्_By_Name[argName] = मानम्_Def;
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
        यदि (refp->तर्काः().आकार() > अङ्कः_तर्काः
            // `define X() is ok to call with nothing
            && !(refp->तर्काः().आकार() == 1 && अङ्कः_तर्काः == 0
                 && trimWhitespace(refp->तर्काः()[0], असत्य) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            प्रत्यागम " `" + refp->name() + " ";
        }
    }

    string बहिर्गमनम्;
    {  // Parse substitution define using arguments
        string argName;
        तार्किक quote = असत्य;
        तार्किक triquote = असत्य;
        तार्किक backslashesc = असत्य;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        प्रति (ध्रुव वर्ण* cp = मानम्.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            यदि (!quote && !triquote && *cp == '\\') {
                backslashesc = सत्य;
            } अन्यथा यदि (std::रिक्तम्_वा(*cp)) {
                backslashesc = असत्य;
            }
            यदि (!quote && !triquote) {
                यदि (std::अक्षरम्_वा(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::अङ्कः_वा(*cp))) {
                    argName += *cp;
                    अनुवर्त;
                }
                यदि (argName != "") {
                    // Found a possible variable substitution
                    ध्रुव स्वचालित iter = arg_मानम्_By_Name.find(argName);
                    यदि (iter != arg_मानम्_By_Name.अन्तः_2()) {
                        // Substitute
                        ध्रुव string subst = iter->second;
                        यदि (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            यदि (बहिर्गमनम्.आकार() >= 2 && बहिर्गमनम्.substr(बहिर्गमनम्.आकार() - 2) == "``") {
                                बहिर्गमनम्.resize(बहिर्गमनम्.आकार() - 2);
                            }
                        } अन्यथा {
                            बहिर्गमनम् += subst;
                        }
                    } अन्यथा {
                        बहिर्गमनम् += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                यदि (cp[0] == '`' && cp[1] == '`') {
                    यदि (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } अन्यथा {
                        बहिर्गमनम् += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '"') {
                    बहिर्गमनम् += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    बहिर्गमनम् += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '`' && cp[1] == '\\') {
                    बहिर्गमनम् += '\\';  // `\ means to put out a backslash
                    cp++;
                    अनुवर्त;
                } अन्यथा यदि (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    बहिर्गमनम् += "\n";  // \\n newline
                    cp++;
                    अनुवर्त;
                }
            }
            यदि (cp[0] == '\\' && cp[1] == '\"') {
                बहिर्गमनम् += cp[0];  // \{any} Put out literal next character
                बहिर्गमनम् += cp[1];
                cp++;
                अनुवर्त;
            } अन्यथा यदि (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                बहिर्गमनम् += cp[0];
                अनुवर्त;
            }
            यदि (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                बहिर्गमनम् += "\"\"\"";
                cp += 2;
                अनुवर्त;
            }
            यदि (*cp == '"') quote = !quote;
            यदि (*cp) बहिर्गमनम् += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(बहिर्गमनम्) << "'");
    प्रत्यागम बहिर्गमनम्;
}

//**********************************************************************
// Parser routines

शून्य V3PreProcImp::openFile(File_पङ्क्तिः*, VInFilter* filterp, ध्रुव string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    यदि (m_incError) प्रत्यागम;
    m_lexp->समुच्चयः_YYDebug(debug() >= 5);
    V3File::सङ्कलय_Src_Depend(filename);

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
        यदि (m_lexp->m_streamp_राशिः.आकार() > V3PreProc::INCLUDE_गभीरता_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = सत्य;
            प्रत्यागम;
        }
        // There's already a file active.  Push it to work on the new one.
        सङ्कलय_पङ्क्तिः_Comment(0);
    }

    // Save file contents for future error reporting
    File_पङ्क्तिः* ध्रुव flsp = नूतन File_पङ्क्तिः{filename};
    flsp->lineno(1);
    flsp->newContent();
    प्रति (ध्रुव string& i : wholefile) flsp->contentp()->प्रक्षिप_Text(i);

    // Save contents for V3Control --contents
    यदि (filename != V3Options::get_Std_Package_मार्गः()) {
        तार्किक अन्तर्भवति_Vlt = असत्य;
        प्रति (ध्रुव string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            यदि (i.find("`verilator_config") != string::npos) {
                अन्तर्भवति_Vlt = सत्य;
                भङ्ग;
            }
        }
        यदि (!अन्तर्भवति_Vlt) {
            प्रति (ध्रुव string& i : wholefile) V3Control::contents_प्रक्षिप_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->अवलोकनम्_New_File(flsp);
    सङ्कलय_पङ्क्तिः_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    पूर्णाङ्क eof_newline = 0;  // Number of characters following last newline
    पूर्णाङ्क eof_lineno = 1;
    प्रति (StrList::iterator it = wholefile.begin(); it != wholefile.अन्तः_2(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        तार्किक strip = असत्य;
        ध्रुव वर्ण* ध्रुव sp = it->दत्त();
        ध्रुव वर्ण* ध्रुव ep = sp + it->दीर्घता_2();
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
            string बहिर्गमनम्;
            बहिर्गमनम्.reserve(it->दीर्घता_2());
            प्रति (ध्रुव वर्ण* cp = sp; cp < ep; cp++) {
                यदि (!(*cp == '\r' || *cp == '\0')) बहिर्गमनम् += *cp;
            }
            *it = बहिर्गमनम्;
        }

        // Push the data to an internal buffer.
        m_lexp->अवलोकनम्_अष्टकानि_पृष्ठभागः(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    यदि (eof_newline) {
        File_पङ्क्तिः* ध्रुव fl = नूतन File_पङ्क्तिः{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_अधिकम्() << "... Suggest add newline.");
    }
}

शून्य V3PreProcImp::निवेशय_Unreadback_At_Bol(ध्रुव string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    यदि (m_पङ्क्तिः_Cmt == "") {
        m_पङ्क्तिः_Cmt_Nl = सत्य;
    } अन्यथा यदि (m_पङ्क्तिः_Cmt[m_पङ्क्तिः_Cmt.दीर्घता_2() - 1] != '\n') {
        निवेशय_Unreadback("\n");
    }
    निवेशय_Unreadback(text);
}

शून्य V3PreProcImp::सङ्कलय_पङ्क्तिः_Comment(पूर्णाङ्क enterExit) {
    यदि (पङ्क्तिः_Directives()) {
        निवेशय_Unreadback_At_Bol(m_lexp->curFilelinep()->पङ्क्तिः_Directive_Strg(enterExit));
    }
}

शून्य V3PreProcImp::dump_परिभाषते(std::ostream& os) {
    प्रति (परिभाषते_Map::const_iterator it = m_परिभाषते.begin(); it != m_परिभाषते.अन्तः_2(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        यदि (!it->second.params().रिक्तानि()) os << it->second.params();
        यदि (!it->second.मानम्().रिक्तानि()) os << " " << it->second.मानम्();
        os << '\n';
    }
}

शून्य V3PreProcImp::candidate_परिभाषते(VSpellCheck* spellerp) {
    प्रति (परिभाषते_Map::const_iterator it = m_परिभाषते.begin(); it != m_परिभाषते.अन्तः_2(); ++it) {
        spellerp->प्रक्षिप_Candidate("`"s + it->first);
    }
}

पूर्णाङ्क V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    यावत् (सत्य) {
    अग्र_tok:
        यदि (m_पङ्क्तिः_सङ्कलय) {
            m_पङ्क्तिः_सङ्कलय--;
            m_rawAtBol = सत्य;
            yyourtext("\n", 1);
            यदि (debug() >= 5) debugToken(VP_WHITE, "LNA");
            प्रत्यागम VP_WHITE;
        }
        यदि (m_पङ्क्तिः_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            स्थिर string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_पङ्क्तिः_Cmt;
            यदि (m_पङ्क्तिः_Cmt_Nl) {
                यदि (!m_rawAtBol) s_rtncmt.निवेशय(0, "\n");
                m_पङ्क्तिः_Cmt_Nl = असत्य;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.दीर्घता_2());
            m_पङ्क्तिः_Cmt = "";
            यदि (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            यदि (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_मानम्(yyourtext(), yyourleng());
                गच्छ अग्र_tok;
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

        यदि (m_चरमः_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_चरमः_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_पङ्क्तिः = 0;
        } अन्यथा यदि (++m_tokens_On_पङ्क्तिः > v3Global.opt.preproc_Token_सीमा()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_गन्तव्यम्_Str(v3Global.opt.preproc_Token_सीमा())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        यदि (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            यदि (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            गच्छ अग्र_tok;
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
        string::आकार_type pos;
        यावत् ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        यावत् ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्तिः_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<पूर्णाङ्क>(m_अवस्थाः.आकार())
                       << ") dr" << m_defRefs.आकार() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        यदि (s_debugFileline >= 9) {
            std::निर्गमः << m_lexp->m_tokFilelinep->warnContextSecondary() << पङ्क्त्यन्तः;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

पूर्णाङ्क V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    यावत् (सत्य) {
    अग्र_tok:
        यदि (isEof()) प्रत्यागम VP_EOF;
        पूर्णाङ्क tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        यदि (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) प्रत्यागम tok;
        यदि (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        यदि (tok == VP_COMMENT) {
            यदि (!m_off) {
                यदि (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.नियोजय_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    यदि (m_पङ्क्तिः_Cmt.रिक्तानि()) निवेशय_Unreadback(" ");
                } अन्यथा यदि (m_lexp->m_keepComments) {
                    प्रत्यागम tok;
                } अन्यथा {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    निवेशय_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            प्रति (आकार_t len = 0; len < static_cast<आकार_t>(yyourleng()); len++) {
                यदि (yyourtext()[len] == '\n') m_पङ्क्तिः_सङ्कलय++;
            }
            गच्छ अग्र_tok;
        }
        यदि (tok == VP_पङ्क्तिः) {
            सङ्कलय_पङ्क्तिः_Comment(m_lexp->m_enterExit);
            गच्छ अग्र_tok;
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
            यदि (tok == VP_JOIN) गच्छ अग्र_tok;
            यदि (tok == VP_चिह्नम्_JOIN) tok = VP_चिह्नम्;
        }
        यदि (tok == VP_चिह्नम्_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            ध्रुव std::string बहिर्गमनम्(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << बहिर्गमनम्);
            // a``b``c can have multiple joins, so we need a stack
            m_join_राशिः.प्रक्षिप(बहिर्गमनम्);
            state_प्रक्षिप(ps_JOIN);
            गच्छ अग्र_tok;
        }

        // Deal with some special parser states
        विकल्प (state()) {
        पक्ष ps_शिखरम्: {
            भङ्ग;
        }
        पक्ष ps_DEFNAME_UNDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_DEFINE:  // FALLTHRU
        पक्ष ps_DEFNAME_IFDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_IFNDEF:  // FALLTHRU
        पक्ष ps_DEFNAME_ELSIF: {
            यदि (tok == VP_चिह्नम्) {
                m_चरमः_Sym.नियोजय_2(yyourtext(), yyourleng());
                यदि (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    तार्किक enable = defExists(m_चरमः_Sym);
                    UINFO(4, "Ifdef " << m_चरमः_Sym << (enable ? " ON" : " OFF"));
                    यदि (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_राशिः.emplace(VPre_If_प्रविष्टिः{enable, असत्य});
                    यदि (!enable) parsingOff();
                    state_निष्कासय();
                    गच्छ अग्र_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_ELSIF) {
                    यदि (m_ifdef_राशिः.रिक्तानि()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } अन्यथा {
                        // Handle `else portion
                        ध्रुव VPre_If_प्रविष्टिः चरमः_If = m_ifdef_राशिः.शिखरम्();
                        m_ifdef_राशिः.निष्कासय();
                        यदि (!चरमः_If.on()) parsingOn();
                        // Handle `if portion
                        ध्रुव तार्किक enable = !चरमः_If.everOn() && defExists(m_चरमः_Sym);
                        UINFO(4, "Elsif " << m_चरमः_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_राशिः.emplace(VPre_If_प्रविष्टिः{enable, चरमः_If.everOn()});
                        यदि (!enable) parsingOff();
                    }
                    state_निष्कासय();
                    गच्छ अग्र_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_UNDEF) {
                    यदि (!m_off) {
                        UINFO(4, "Undef " << m_चरमः_Sym);
                        undef(m_चरमः_Sym);
                    }
                    state_निष्कासय();
                    गच्छ अग्र_tok;
                } अन्यथा यदि (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->प्रक्षिप_State_Def_Form();
                    गच्छ अग्र_tok;
                } अन्यथा {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                यदि (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->प्रक्षिप_State_व्यञ्जकम्();
                    m_व्यञ्जकम्_Parser.reset(fileline());
                    m_व्यञ्जकम्_Paren_Level = 1;
                    विकल्प (state()) {
                    पक्ष ps_DEFNAME_IFDEF: stateChange(ps_व्यञ्जकम्_IFDEF); भङ्ग;
                    पक्ष ps_DEFNAME_IFNDEF: stateChange(ps_व्यञ्जकम्_IFNDEF); भङ्ग;
                    पक्ष ps_DEFNAME_ELSIF: stateChange(ps_व्यञ्जकम्_ELSIF); भङ्ग;
                    स्वभाव: v3fatalSrc("bad case");
                    }
                    गच्छ अग्र_tok;
                }
                यदि (!m_off) {
                    प्रत्यागम tok;
                } अन्यथा {
                    गच्छ अग्र_tok;
                }
            } अन्यथा यदि (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                भङ्ग;
            } अन्यथा {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                गच्छ अग्र_tok;
            }
        }
        पक्ष ps_व्यञ्जकम्_IFDEF:  // FALLTHRU
        पक्ष ps_व्यञ्जकम्_IFNDEF:  // FALLTHRU
        पक्ष ps_व्यञ्जकम्_ELSIF: {
            // `ifdef ( *here*
            File_पङ्क्तिः* ध्रुव flp = m_lexp->m_tokFilelinep;
            यदि (tok == VP_चिह्नम्) {
                m_चरमः_Sym.नियोजय_2(yyourtext(), yyourleng());
                ध्रुव तार्किक exists = defExists(m_चरमः_Sym);
                यदि (exists) {
                    ध्रुव std::string मानम् = def_मानम्(m_चरमः_Sym);
                    यदि (VString::removeWhitespace(मानम्) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_चरमः_Sym << "' with value '" << मानम्
                                << "'\n"
                                   "... Suggest change define '"
                                << m_चरमः_Sym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, exists});
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_WHITE) {
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::BRA});
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_व्यञ्जकम्_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                यदि (--m_व्यञ्जकम्_Paren_Level > 0) {
                    m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::KET});
                    गच्छ अग्र_tok;
                } अन्यथा {
                    // Done with parsing expression
                    तार्किक enable = m_व्यञ्जकम्_Parser.परिणाम();
                    UINFO(4, "ifdef() result=" << enable);
                    यदि (state() == ps_व्यञ्जकम्_IFDEF || state() == ps_व्यञ्जकम्_IFNDEF) {
                        यदि (state() == ps_व्यञ्जकम्_IFNDEF) enable = !enable;
                        m_ifdef_राशिः.emplace(VPre_If_प्रविष्टिः{enable, असत्य});
                        यदि (!enable) parsingOff();
                        state_निष्कासय();
                        गच्छ अग्र_tok;
                    } अन्यथा यदि (state() == ps_व्यञ्जकम्_ELSIF) {
                        यदि (m_ifdef_राशिः.रिक्तानि()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } अन्यथा {
                            // Handle `else portion
                            ध्रुव VPre_If_प्रविष्टिः चरमः_If = m_ifdef_राशिः.शिखरम्();
                            m_ifdef_राशिः.निष्कासय();
                            यदि (!चरमः_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !चरमः_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_चरमः_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_राशिः.emplace(VPre_If_प्रविष्टिः{enable, चरमः_If.everOn()});
                            यदि (!enable) parsingOff();
                        }
                        state_निष्कासय();
                    }
                    गच्छ अग्र_tok;
                }
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::LNOT});
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == वाक्ये_तुलय(yyourtext(), "&&", 2)) {
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::LAND});
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == वाक्ये_तुलय(yyourtext(), "||", 2)) {
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::LOR});
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 2 && 0 == वाक्ये_तुलय(yyourtext(), "->", 2)) {
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::IMP});
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 3 && 0 == वाक्ये_तुलय(yyourtext(), "<->", 3)) {
                m_व्यञ्जकम्_Parser.प्रक्षिप_निवेशः(V3_Pre_व्यञ्जकम्_Token{flp, V3_Pre_व्यञ्जकम्_Token::EQV});
                गच्छ अग्र_tok;
            } अन्यथा {
                यदि (VString::removeWhitespace(string{yyourtext(), yyourleng()}).रिक्तानि()) {
                    प्रत्यागम tok;
                } अन्यथा {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                गच्छ अग्र_tok;
            }
        }
        पक्ष ps_DEFFORM: {
            यदि (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_मानम्;
                यदि (debug() >= 5) {
                    निर्गमः << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->प्रक्षिप_State_Def_मानम्();
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT) {
                // IE, something like comment in formals
                यदि (!m_off) प्रत्यागम tok;
                गच्छ अग्र_tok;
            } अन्यथा {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                गच्छ अग्र_tok;
            }
        }
        पक्ष ps_DEFVALUE: {
            स्थिर string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            यदि (tok == VP_DEFVALUE) {
                यदि (debug() >= 5) {  // LCOV_EXCL_START
                    निर्गमः << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_मानम्)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                ध्रुव string formals = m_formals;
                string मानम् = m_lexp->m_def_मानम्;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                प्रति (आकार_t i = 0; i < formals.दीर्घता_2(); i++) {
                    यदि (formals[i] == '\n') s_newlines += "\n";
                }
                प्रति (आकार_t i = 0; i < मानम्.दीर्घता_2(); i++) {
                    यदि (मानम्[i] == '\n') s_newlines += "\n";
                }
                यदि (!m_off) {
                    // Remove leading and trailing whitespace
                    मानम् = trimWhitespace(मानम्, सत्य);
                    // Define it
                    UINFO(4, "Define " << m_चरमः_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(मानम्) << "'");
                    define(fileline(), m_चरमः_Sym, मानम्, formals, असत्य);
                }
            } अन्यथा {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_निष्कासय();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.दीर्घता_2());
            प्रत्यागम VP_WHITE;
        }
        पक्ष ps_DEFPAREN: {
            यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                गच्छ अग्र_tok;
            } अन्यथा {
                UASSERT(!m_defRefs.रिक्तानि(), "Shouldn't be in DEFPAREN w/o active defref");
                ध्रुव VDefineRef* ध्रुव refp = &(m_defRefs.शिखरम्());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_निष्कासय();
                गच्छ अग्र_tok;
            }
        }
        पक्ष ps_DEFARG: {
            UASSERT(!m_defRefs.रिक्तानि(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.शिखरम्());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_मानम्);
            m_lexp->m_def_मानम् = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            यदि (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->तर्काः().प्रक्षिप_पृष्ठभागः(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->प्रक्षिप_State_Def_Arg(1);
                refp->nextarg("");
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->तर्काः().प्रक्षिप_पृष्ठभागः(refp->nextarg());
                string बहिर्गमनम्;
                यदि (!m_off) {
                    बहिर्गमनम् = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.निष्कासय(), refp);
                state_निष्कासय();
                यदि (m_defRefs.रिक्तानि() || state() == ps_STRIFY || state() == ps_JOIN) {
                    यदि (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_राशिः.रिक्तानि(), "`` join stack empty, but in a ``");
                        ध्रुव string lhs = m_join_राशिः.शिखरम्();
                        m_join_राशिः.निष्कासय();
                        बहिर्गमनम्.निवेशय(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << बहिर्गमनम्);
                        state_निष्कासय();
                    }
                    यदि (!m_off) {
                        unputDefrefString(बहिर्गमनम्);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    अन्यथा यदि (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.रिक्तानि() ? 0 : m_defRefs.शिखरम्().parenLevel();
                } अन्यथा {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.शिखरम्());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_मानम् + बहिर्गमनम्);
                    m_lexp->m_def_मानम् = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                भङ्ग;
            } अन्यथा यदि (tok == VP_चिह्नम् || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.नियोजय_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_प्रक्षिप(ps_STRIFY);
                गच्छ अग्र_tok;
            } अन्यथा {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_निष्कासय();
                गच्छ अग्र_tok;
            }
        }
        पक्ष ps_INCNAME: {
            यदि (tok == VP_STRING) {
                state_निष्कासय();
                m_चरमः_Sym.नियोजय_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_चरमः_Sym);
                // Drop leading and trailing quotes.
                m_चरमः_Sym.erase(0, 1);
                m_चरमः_Sym.erase(m_चरमः_Sym.दीर्घता_2() - 1, 1);
                include(m_चरमः_Sym);
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->प्रक्षिप_State_Inc_Filename();
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                भङ्ग;
            } अन्यथा {
                state_निष्कासय();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                गच्छ अग्र_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        पक्ष ps_ERRORNAME: {
            यदि (tok == VP_STRING) {
                यदि (!m_off) {
                    m_चरमः_Sym.नियोजय_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_चरमः_Sym);
                }
                state_निष्कासय();
                गच्छ अग्र_tok;
            } अन्यथा {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_निष्कासय();
                गच्छ अग्र_tok;
            }
        }
        पक्ष ps_JOIN: {
            यदि (tok == VP_चिह्नम् || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_राशिः.रिक्तानि(), "`` join stack empty, but in a ``");
                ध्रुव string lhs = m_join_राशिः.शिखरम्();
                m_join_राशिः.निष्कासय();
                UINFO(5, "`` LHS:" << lhs);
                ध्रुव string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                ध्रुव string बहिर्गमनम् = lhs + rhs;
                UINFO(5, "`` Out:" << बहिर्गमनम्);
                unputString(बहिर्गमनम्);
                state_निष्कासय();
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                ध्रुव string lhs = m_join_राशिः.शिखरम्();
                m_join_राशिः.निष्कासय();
                unputString(lhs);
                state_निष्कासय();
                गच्छ अग्र_tok;
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
                string बहिर्गमनम् = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(बहिर्गमनम्.begin(), बहिर्गमनम्.अन्तः_2(), '\n', ' ');
                unputString("\""s + बहिर्गमनम् + "\"");
                state_निष्कासय();
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                भङ्ग;
            } अन्यथा यदि (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                गच्छ अग्र_tok;
            } अन्यथा यदि (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                भङ्ग;
            } अन्यथा {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                गच्छ अग्र_tok;
            }
        }
        स्वभाव: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        विकल्प (tok) {
        पक्ष VP_INCLUDE:
            यदि (!m_off) {
                state_प्रक्षिप(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            गच्छ अग्र_tok;
        पक्ष VP_UNDEF: state_प्रक्षिप(ps_DEFNAME_UNDEF); गच्छ अग्र_tok;
        पक्ष VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_प्रक्षिप(ps_DEFNAME_DEFINE);
            गच्छ अग्र_tok;
        पक्ष VP_IFDEF: state_प्रक्षिप(ps_DEFNAME_IFDEF); गच्छ अग्र_tok;
        पक्ष VP_IFNDEF: state_प्रक्षिप(ps_DEFNAME_IFNDEF); गच्छ अग्र_tok;
        पक्ष VP_ELSIF: state_प्रक्षिप(ps_DEFNAME_ELSIF); गच्छ अग्र_tok;
        पक्ष VP_ELSE:
            यदि (m_ifdef_राशिः.रिक्तानि()) {
                fileline()->v3error("`else with no matching `if");
            } अन्यथा {
                ध्रुव VPre_If_प्रविष्टिः चरमः_If = m_ifdef_राशिः.शिखरम्();
                m_ifdef_राशिः.निष्कासय();
                ध्रुव तार्किक enable = !चरमः_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_राशिः.emplace(VPre_If_प्रविष्टिः{enable, चरमः_If.everOn()});
                यदि (!चरमः_If.on()) parsingOn();
                यदि (!enable) parsingOff();
            }
            गच्छ अग्र_tok;
        पक्ष VP_ENDIF:
            UINFO(4, "Endif ");
            यदि (m_ifdef_राशिः.रिक्तानि()) {
                fileline()->v3error("`endif with no matching `if");
            } अन्यथा {
                ध्रुव VPre_If_प्रविष्टिः चरमः_If = m_ifdef_राशिः.शिखरम्();
                m_ifdef_राशिः.निष्कासय();
                यदि (!चरमः_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            गच्छ अग्र_tok;

        पक्ष VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            ध्रुव string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            यदि (m_defPutJoin) {
                m_defPutJoin = असत्य;
                unputString("``");
            }
            यदि (m_def_गभीरता++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                गच्छ अग्र_tok;
            }
            // Substitute
            यदि (!defExists(name)) {  // Not found, return original string as-is
                m_def_गभीरता = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                यदि (m_off) {
                    गच्छ अग्र_tok;
                } अन्यथा {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    गच्छ अग्र_tok;
                }
            } अन्यथा {
                ध्रुव string params = defParams(name);
                यदि (params == "0" || params == "") {  // Found, as simple substitution
                    string बहिर्गमनम्;
                    यदि (!m_off) {
                        VDefineRef tempref{name, ""};
                        बहिर्गमनम् = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    यदि (m_defRefs.रिक्तानि() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        यदि (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_राशिः.रिक्तानि(), "`` join stack empty, but in a ``");
                            ध्रुव string lhs = m_join_राशिः.शिखरम्();
                            m_join_राशिः.निष्कासय();
                            बहिर्गमनम्.निवेशय(0, lhs);
                            UINFO(5, "``-end-defref Out:" << बहिर्गमनम्);
                            state_निष्कासय();
                        }
                        यदि (!m_off) {
                            unputDefrefString(बहिर्गमनम्);
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
                        VDefineRef* ध्रुव refp = &(m_defRefs.शिखरम्());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_मानम् + बहिर्गमनम्);
                        m_lexp->m_def_मानम् = "";
                    }
                    गच्छ अग्र_tok;
                } अन्यथा {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    यदि (!m_defRefs.रिक्तानि()) m_defRefs.शिखरम्().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_प्रक्षिप(ps_DEFPAREN);
                    m_lexp->प्रक्षिप_State_Def_Arg(0);
                    गच्छ अग्र_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            गच्छ अग्र_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        पक्ष VP_ERROR: {
            state_प्रक्षिप(ps_ERRORNAME);
            गच्छ अग्र_tok;
        }
        पक्ष VP_EOF:
            यदि (!m_ifdef_राशिः.रिक्तानि()) fileline()->v3error("`ifdef not terminated at EOF");
            प्रत्यागम tok;
        पक्ष VP_UNDEFINEALL:
            यदि (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            गच्छ अग्र_tok;
        पक्ष VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_प्रक्षिप(ps_STRIFY);
            गच्छ अग्र_tok;
        पक्ष VP_चिह्नम्:
        पक्ष VP_STRING:
        पक्ष VP_TEXT: {
            m_def_गभीरता = 0;
            यदि (!m_off) {
                प्रत्यागम tok;
            } अन्यथा {
                गच्छ अग्र_tok;
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
        m_finBuf.नियोजय_2(yyourtext(), yyourleng());
    }
    ध्रुव पूर्णाङ्क tok = m_finToken;
    buf = m_finBuf;
    यदि (असत्य && debug() >= 5) {
        ध्रुव string bufcln = V3PreLex::cleanDbgStrg(buf);
        ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्तिः_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    ध्रुव वर्ण* bufp = buf.c_str();
    यावत् (*bufp == '\n') bufp++;
    यदि ((tok == VP_TEXT || tok == VP_पङ्क्तिः) && VString::startsWith(bufp, "`line ")) {
        पूर्णाङ्क enter;
        m_finFilelinep->पङ्क्तिः_Directive(bufp, enter /*ref*/);
    } अन्यथा {
        यदि (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->पङ्क्तिः_Directives()) {
            यदि (ध्रुव पूर्णाङ्क बहिर्गमनम्_Behind
                = (m_lexp->m_tokFilelinep->चरमः_Lineno() - m_finFilelinep->चरमः_Lineno())) {
                यदि (debug() >= 5) {
                    ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्तिः_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->चरमः_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->चरमः_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->चरमः_Lineno());
                यदि (बहिर्गमनम्_Behind > 0
                    && (बहिर्गमनम्_Behind <= static_cast<पूर्णाङ्क>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    यदि (m_preprocp->keepWhitespace()) {
                        buf = std::string(बहिर्गमनम्_Behind, '\n');  // () for char repeat
                        प्रत्यागम VP_TEXT;
                    }
                } अन्यथा {
                    // Need to backup, use `line
                    buf = m_finFilelinep->पङ्क्तिः_Directive_Strg(0);
                    प्रत्यागम VP_पङ्क्तिः;
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
    यावत् (शून्यसूचकः == (rtnp = std::strchr(m_पङ्क्तिः_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        ध्रुव पूर्णाङ्क tok = getFinalToken(m_tokenBuf /*ref*/);
        यदि (debug() >= 5) {
            ध्रुव string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्तिः_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        यदि (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            यदि (m_पङ्क्तिः_Chars != "" && m_पङ्क्तिः_Chars[m_पङ्क्तिः_Chars.दीर्घता_2() - 1] != '\n') {
                m_पङ्क्तिः_Chars.append("\n");
            }
            gotEof = सत्य;
        } अन्यथा {
            m_पङ्क्तिः_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    ध्रुव पूर्णाङ्क len = rtnp - m_पङ्क्तिः_Chars.c_str() + 1;
    string the_पङ्क्तिः(m_पङ्क्तिः_Chars, 0, len);
    m_पङ्क्तिः_Chars = m_पङ्क्तिः_Chars.erase(0, len);  // Remove returned characters
    यदि (debug() >= 4) {
        ध्रुव string lncln = V3PreLex::cleanDbgStrg(the_पङ्क्तिः);
        ध्रुव string flcol = m_lexp->m_tokFilelinep->ascii_पङ्क्तिः_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    प्रत्यागम the_पङ्क्तिः;
}
