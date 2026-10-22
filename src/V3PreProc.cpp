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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3PreProc.h"

#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3LanguageWords.h"
#అన్తర్భావ "V3PreExpr.h"
#అన్తర్భావ "V3PreLex.h"
#అన్తర్భావ "V3PreShell.h"
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cstdlib>
#అన్తర్భావ <fstream>
#అన్తర్భావ <stack>
#అన్తర్భావ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#నిర్వచించు yyFlexLexer V3Lexer
#అన్తర్భావ "V3PreLex.yy.cpp"
#నిర్వచనం_రద్దు yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

తరగతి VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_పంక్తి* స్థిరరాశి m_fileline;  // Where it was declared
    స్థిరరాశి string m_విలువ;  // Value of define
    స్థిరరాశి string m_params;  // Parameters
    స్థిరరాశి తార్కిక m_cmdline;  // Set on command line, don't `undefineall
బహిరంగ:
    VDefine(File_పంక్తి* fl, స్థిరరాశి string& విలువ, స్థిరరాశి string& params, తార్కిక cmdline)
        : m_fileline{fl}
        , m_విలువ{విలువ}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_పంక్తి* fileline() స్థిరరాశి { తిరిగి m_fileline; }
    string విలువ() స్థిరరాశి { తిరిగి m_విలువ; }
    string params() స్థిరరాశి { తిరిగి m_params; }
    తార్కిక cmdline() స్థిరరాశి { తిరిగి m_cmdline; }
};

//*************************************************************************

తరగతి VDefineRef final {
    // One for each pending define substitution
    స్థిరరాశి string m_name;  // Define last name being defined
    స్థిరరాశి string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    పూర్ణాంకం m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::సదిశ<string> m_వాదనలు;  // List of define arguments
బహిరంగ:
    string name() స్థిరరాశి { తిరిగి m_name; }
    string params() స్థిరరాశి { తిరిగి m_params; }
    string nextarg() స్థిరరాశి { తిరిగి m_nextarg; }
    శూన్య nextarg(స్థిరరాశి string& విలువ) { m_nextarg = విలువ; }
    పూర్ణాంకం parenLevel() స్థిరరాశి { తిరిగి m_parenLevel; }
    శూన్య parenLevel(పూర్ణాంకం విలువ) { m_parenLevel = విలువ; }
    std::సదిశ<string>& వాదనలు() { తిరిగి m_వాదనలు; }
    VDefineRef(స్థిరరాశి string& name, స్థిరరాశి string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = అప్రమేయ;
};

//*************************************************************************
/// Data for parsing on/off

తరగతి VPre_If_నమోదు final {
    // One for each pending ifdef/ifndef
    స్థిరరాశి తార్కిక m_on;  // Current parse for this ifdef level is "on"
    స్థిరరాశి తార్కిక m_everOn;  // Some if term in elsif tree has been on
బహిరంగ:
    తార్కిక on() స్థిరరాశి { తిరిగి m_on; }
    తార్కిక everOn() స్థిరరాశి { తిరిగి m_everOn; }
    VPre_If_నమోదు(తార్కిక on, తార్కిక everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPre_If_నమోదు() = అప్రమేయ;
};

//*************************************************************************
// Data for a preprocessor instantiation.

తరగతి V3PreProcImp final : బహిరంగ V3PreProc {
బహిరంగ:
    // TYPES
    ఉపయోగించి నిర్వచిస్తుంది_Map = std::map<స్థిరరాశి std::string, VDefine>;
    ఉపయోగించి StrList = VInFilter::StrList;

    // Defines list
    నిర్వచిస్తుంది_Map m_నిర్వచిస్తుంది;  ///< Map of defines

    // STATE
    స్థిరరాశి V3PreProc* m_preprocp = శూన్యసూచిక;  ///< Object we're holding data for
    V3PreLex* m_lexp = శూన్యసూచిక;  ///< Current lexer state (nullptr = closed)
    పూర్ణాంకం m_చివరి_Lineno = 0;  // Last line number (stall detection)
    పూర్ణాంకం m_టోకెన్లు_On_పంక్తి = 0;  // Number of tokens on line (stall detection)

    గణన ProcState : uint8_t {
        ps_శిఖరం,
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
        ps_సమాసం_IFDEF,
        ps_సమాసం_IFNDEF,
        ps_సమాసం_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    స్థిర స్థిరరాశి అక్షరం* procStateName(ProcState s) {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి స్థితులు[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        తిరిగి స్థితులు[s];
    }

    std::దొంతర<ProcState> m_స్థితులు;  ///< Current state of parser
    పూర్ణాంకం m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    తార్కిక m_incError = అబద్ధం;  ///< Include error found
    string m_చివరి_Sym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_పంక్తి_Cmt;  ///< Line comment(s) to be returned
    తార్కిక m_పంక్తి_Cmt_Nl = అబద్ధం;  ///< Newline needed before inserting lineCmt
    పూర్ణాంకం m_పంక్తి_కూడు = 0;  ///< Empty lines to return to maintain line count
    తార్కిక m_rawAtBol = నిజం;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    తార్కిక m_finAhead = అబద్ధం;  ///< Have read a token ahead
    పూర్ణాంకం m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    తార్కిక m_finAtBol = నిజం;  ///< Last getFinalToken left us at beginning of line
    File_పంక్తి* m_finFilelinep = శూన్యసూచిక;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::దొంతర<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::దొంతర<VPre_If_నమోదు> m_ifdef_దొంతర;  ///< Stack of true/false emitting evaluations
    గుర్తులేని m_def_లోతు = 0;  ///< How many `defines deep
    తార్కిక m_defPutJoin = అబద్ధం;  ///< Insert `` after substitution

    // For `` join
    std::దొంతర<string> m_join_దొంతర;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_సమాసం m_సమాసం_Parser;  ///< Parser for () expression
    పూర్ణాంకం m_సమాసం_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_పంక్తి_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    శూన్య v3error_చివర(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3error_చివర(str);
    }

    స్థిర స్థిరరాశి అక్షరం* tokenName(పూర్ణాంకం tok);
    శూన్య debugToken(పూర్ణాంకం tok, స్థిరరాశి అక్షరం* cmtp);
    శూన్య parse_శిఖరం();
    శూన్య parseUndef();

అంతరంగ:
    // Internal methods
    శూన్య చివర_యొక్క_One_File();
    string defineSubst(VDefineRef* refp);

    తార్కిక defExists(స్థిరరాశి string& name);
    తార్కిక defCmdline(స్థిరరాశి string& name);
    string def_విలువ(స్థిరరాశి string& name);
    string defParams(స్థిరరాశి string& name);
    File_పంక్తి* defFileline(స్థిరరాశి string& name);

    string commentCleanup(స్థిరరాశి string& పాఠ్యం);
    తార్కిక comment_Token_సరిపోలిక(string& cmdr, స్థిరరాశి అక్షరం* strg);
    స్థిర string trimWhitespace(స్థిరరాశి string& strg, తార్కిక trailing);
    శూన్య unputString(స్థిరరాశి string& strg);
    శూన్య unputDefrefString(స్థిరరాశి string& strg);

    శూన్య parsingOn() {
        m_off--;
        ఒకవేళ (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    శూన్య parsingOff() { m_off++; }

    పూర్ణాంకం getRawToken();
    పూర్ణాంకం getStateToken();
    పూర్ణాంకం getFinalToken(string& buf);

    ProcState state() స్థిరరాశి { తిరిగి m_స్థితులు.శిఖరం(); }
    తార్కిక stateIsDefname() స్థిరరాశి {
        తిరిగి state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    శూన్య state_నెట్టు(ProcState state) { m_స్థితులు.నెట్టు(state); }
    శూన్య state_తీయి() {
        m_స్థితులు.తీయి();
        ఒకవేళ (VL_UNCOVERABLE(m_స్థితులు.ఖాళీలు())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_స్థితులు.నెట్టు(ps_శిఖరం);  // LCOV_EXCL_LINE
        }
    }
    శూన్య stateChange(ProcState state) {
        state_తీయి();
        state_నెట్టు(state);
    }

బహిరంగ:
    // METHODS, called from upper level shell
    శూన్య openFile(File_పంక్తి* fl, VInFilter* filterp, స్థిరరాశి string& filename) override;
    తార్కిక isEof() స్థిరరాశి override { తిరిగి m_lexp->curStreamp()->m_eof; }
    శూన్య forceEof() { m_lexp->curStreamp()->m_eof = నిజం; }
    string getline() override;
    శూన్య చొప్పించు_Unreadback(స్థిరరాశి string& పాఠ్యం) override { m_పంక్తి_Cmt += పాఠ్యం; }
    శూన్య చొప్పించు_Unreadback_At_Bol(స్థిరరాశి string& పాఠ్యం);
    శూన్య కూడు_పంక్తి_Comment(పూర్ణాంకం enterExit);
    శూన్య dump_నిర్వచిస్తుంది(std::ostream& os) override;
    శూన్య candidate_నిర్వచిస్తుంది(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    శూన్య comment(స్థిరరాశి string& పాఠ్యం) override;  // Comment detected (if keepComments==2)
    శూన్య include(స్థిరరాశి string& filename) override;  // Request a include file be processed
    శూన్య undef(స్థిరరాశి string& name) override;
    మాయా శూన్య undefineall();
    శూన్య define(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& విలువ, స్థిరరాశి string& params,
                తార్కిక cmdline) override;
    string remove_నిర్వచిస్తుంది(స్థిరరాశి string& పాఠ్యం) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_స్థితులు.నెట్టు(ps_శిఖరం); }
    // cppcheck-suppress duplInheritedMember
    శూన్య configure(File_పంక్తి* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = ఇది;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = కొత్త File_పంక్తి{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = కొత్త V3PreLex{ఇది, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        ఒకవేళ (m_lexp) VL_DO_CLEAR(తొలగించు m_lexp, m_lexp = శూన్యసూచిక);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_పంక్తి* fl) {
    V3PreProcImp* స్థిరరాశి preprocp = కొత్త V3PreProcImp;
    preprocp->configure(fl);
    తిరిగి preprocp;
}

శూన్య V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_సమాసం::selfTest(); }

//*************************************************************************
//నిర్వచిస్తుంది

శూన్య V3PreProcImp::undef(స్థిరరాశి string& name) {
    ఒకవేళ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_నిర్వచిస్తుంది())
        చొప్పించు_Unreadback("`undef " + name + "\n");
    m_నిర్వచిస్తుంది.erase(name);
}
శూన్య V3PreProcImp::undefineall() {
    ఒకవేళ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_నిర్వచిస్తుంది())
        చొప్పించు_Unreadback("`undefineall\n");
    కొరకు (నిర్వచిస్తుంది_Map::iterator nextit, it = m_నిర్వచిస్తుంది.begin(); it != m_నిర్వచిస్తుంది.చివర(); it = nextit) {
        nextit = it;
        ++nextit;
        ఒకవేళ (!it->రెండవ.cmdline()) m_నిర్వచిస్తుంది.erase(it);
    }
}
తార్కిక V3PreProcImp::defExists(స్థిరరాశి string& name) {
    స్థిరరాశి స్వయంచాలక iter = m_నిర్వచిస్తుంది.కనుగొను(name);
    తిరిగి (iter != m_నిర్వచిస్తుంది.చివర());
}
తార్కిక V3PreProcImp::defCmdline(స్థిరరాశి string& name) {
    స్థిరరాశి స్వయంచాలక iter = m_నిర్వచిస్తుంది.కనుగొను(name);
    ఒకవేళ (iter == m_నిర్వచిస్తుంది.చివర()) { తిరిగి అబద్ధం; }
    తిరిగి iter->రెండవ.cmdline();
}
string V3PreProcImp::def_విలువ(స్థిరరాశి string& name) {
    స్థిరరాశి స్వయంచాలక iter = m_నిర్వచిస్తుంది.కనుగొను(name);
    ఒకవేళ (iter == m_నిర్వచిస్తుంది.చివర()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        తిరిగి "";
    }
    తిరిగి iter->రెండవ.విలువ();
}
string V3PreProcImp::defParams(స్థిరరాశి string& name) {
    స్థిరరాశి స్వయంచాలక iter = m_నిర్వచిస్తుంది.కనుగొను(name);
    ఒకవేళ (iter == m_నిర్వచిస్తుంది.చివర()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        తిరిగి "";
    }
    తిరిగి iter->రెండవ.params();
}
File_పంక్తి* V3PreProcImp::defFileline(స్థిరరాశి string& name) {
    స్థిరరాశి స్వయంచాలక iter = m_నిర్వచిస్తుంది.కనుగొను(name);
    ఒకవేళ (iter == m_నిర్వచిస్తుంది.చివర()) తిరిగి శూన్యసూచిక;
    తిరిగి iter->రెండవ.fileline();
}
శూన్య V3PreProcImp::define(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& విలువ,
                          స్థిరరాశి string& params, తార్కిక cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << విలువ << "' params '" << params << "'");
    ఒకవేళ (v3Global.opt.preprocOnly() && v3Global.opt.preproc_నిర్వచిస్తుంది()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string బహిర్గతం;
        బహిర్గతం.reserve(విలువ.కొలత());
        కొరకు (string::కొలత_type pos = 0; pos < విలువ.కొలత(); ++pos) {
            ఒకవేళ (విలువ[pos] == '\n' && (pos == 0 || విలువ[pos - 1] != '\\')) బహిర్గతం += '\\';
            బహిర్గతం += విలువ[pos];
        }
        చొప్పించు_Unreadback("`define " + name + params + (బహిర్గతం.ఖాళీలు() ? "" : " ") + బహిర్గతం + "\n");
    }
    ఒకవేళ (!V3_Language_పదాలు::isKeyword("`"s + name).ఖాళీలు()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } లేకపోతే {
        ఒకవేళ (defExists(name)) {
            ఒకవేళ (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << విలువ
                               << "' to existing command line define value: '" << def_విలువ(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_విలువ(name) << (defParams(name).ఖాళీలు() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                తిరిగి;
            } లేకపోతే {
                ఒకవేళ (!(def_విలువ(name) == విలువ
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << విలువ
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_విలువ(name) << (defParams(name).ఖాళీలు() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_నిర్వచిస్తుంది.emplace(name, VDefine{fl, విలువ, params, cmdline});
    }
}

string V3PreProcImp::remove_నిర్వచిస్తుంది(స్థిరరాశి string& పాఠ్యం) {
    string val;
    string rtnsym = పాఠ్యం;
    కొరకు (పూర్ణాంకం loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        ఒకవేళ (xsym[0] == '`') xsym.erase(0, 1);
        ఒకవేళ (defExists(xsym)) {
            val = def_విలువ(xsym);
            ఒకవేళ (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } లేకపోతే {
                ఆపు;
            }
        } లేకపోతే {
            ఆపు;
        }
    }
    తిరిగి rtnsym;  // NA
}

//**********************************************************************
// Comments

శూన్య V3PreProcImp::include(స్థిరరాశి string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(స్థిరరాశి string& పాఠ్యం) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = పాఠ్యం;
    string::కొలత_type pos;
    అయితే ((pos = cmd.కనుగొను("//")) != string::npos) cmd.replace(pos, 2, "");
    అయితే ((pos = cmd.కనుగొను("/*")) != string::npos) cmd.replace(pos, 2, "");
    అయితే ((pos = cmd.కనుగొను("*/")) != string::npos) cmd.replace(pos, 2, "");
    అయితే ((pos = cmd.కనుగొను('\"')) != string::npos) cmd.replace(pos, 1, " ");
    అయితే ((pos = cmd.కనుగొను('\t')) != string::npos) cmd.replace(pos, 1, " ");
    అయితే ((pos = cmd.కనుగొను("  ")) != string::npos) cmd.replace(pos, 2, " ");
    అయితే (!cmd.ఖాళీలు() && std::ఖాళీనా(cmd[cmd.కొలత() - 1])) cmd.erase(cmd.కొలత() - 1);
    తిరిగి cmd;
}

తార్కిక V3PreProcImp::comment_Token_సరిపోలిక(string& cmdr, స్థిరరాశి అక్షరం* strg) {
    పూర్ణాంకం len = std::అక్షరశ్రేణి_పొడవు(strg);
    ఒకవేళ (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::ఖాళీనా(cmdr[len]))) {
        ఒకవేళ (std::ఖాళీనా(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        తిరిగి నిజం;
    } లేకపోతే {
        తిరిగి అబద్ధం;
    }
}

శూన్య V3PreProcImp::comment(స్థిరరాశి string& పాఠ్యం) {
    // Comment detected.  Only keep relevant data.
    తార్కిక printed = అబద్ధం;
    ఒకవేళ (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        చొప్పించు_Unreadback(పాఠ్యం);
        printed = నిజం;
    }

    స్థిరరాశి అక్షరం* cp = పాఠ్యం.c_str();
    ఒకవేళ (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } లేకపోతే {
        తిరిగి;
    }

    అయితే (std::ఖాళీనా(*cp)) ++cp;

    తార్కిక synth = అబద్ధం;
    తార్కిక vlcomment = అబద్ధం;
    ఒకవేళ ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::అక్షరశ్రేణి_పొడవు("verilator");
        ఒకవేళ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            తిరిగి;
        }
        vlcomment = నిజం;
    } లేకపోతే ఒకవేళ (VString::startsWith(cp, "synopsys")) {
        cp += std::అక్షరశ్రేణి_పొడవు("synopsys");
        ఒకవేళ (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            తిరిగి;
        }
        synth = నిజం;
    } లేకపోతే ఒకవేళ (VString::startsWith(cp, "cadence")) {
        cp += std::అక్షరశ్రేణి_పొడవు("cadence");
        synth = నిజం;
    } లేకపోతే ఒకవేళ (VString::startsWith(cp, "pragma")) {
        cp += std::అక్షరశ్రేణి_పొడవు("pragma");
        synth = నిజం;
    } లేకపోతే ఒకవేళ (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::అక్షరశ్రేణి_పొడవు("ambit synthesis");
        synth = నిజం;
    } లేకపోతే {
        తిరిగి;
    }

    అయితే (std::ఖాళీనా(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    ఒకవేళ (synth) {
        ఒకవేళ (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            ఒకవేళ (comment_Token_సరిపోలిక(cmd /*ref*/, "full_case")) {
                ఒకవేళ (!printed) చొప్పించు_Unreadback("/*verilator full_case*/");
            }
            ఒకవేళ (comment_Token_సరిపోలిక(cmd /*ref*/, "parallel_case")) {
                ఒకవేళ (!printed) చొప్పించు_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } లేకపోతే ఒకవేళ (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.ప్రజా_Off() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        ఒకవేళ (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::కొలత_type చివర_యొక్క_Cmd = std::అక్షరశ్రేణి_పొడవు("public_flat_rw");
            అయితే (VString::isIdentifierChar(cmd[చివర_యొక్క_Cmd])) ++చివర_యొక్క_Cmd;
            string పునాది_Cmd = cmd.substr(0, చివర_యొక్క_Cmd);
            string arg = cmd.substr(చివర_యొక్క_Cmd);
            అయితే (std::ఖాళీనా(arg[0])) arg = arg.substr(1);
            ఒకవేళ (arg.కొలత() && పునాది_Cmd == "public_flat_rw_on")
                పునాది_Cmd += "_sns";  // different cmd for applying sensitivity
            ఒకవేళ (!printed) చొప్పించు_Unreadback("/*verilator " + పునాది_Cmd + "*/ " + arg + " /**/");
        } లేకపోతే ఒకవేళ (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string పునాది_Cmd = cmd.substr(0, std::అక్షరశ్రేణి_పొడవు("dpi_c_decl"));
            string::కొలత_type ప్రారంభం_యొక్క_Arg = పునాది_Cmd.కొలత();
            అయితే (std::ఖాళీనా(cmd[ప్రారంభం_యొక్క_Arg])) ప్రారంభం_యొక్క_Arg++;
            string arg = cmd.substr(ప్రారంభం_యొక్క_Arg);
            ఒకవేళ (arg.ఖాళీలు()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                తిరిగి;
            }
            ఒకవేళ (!printed) చొప్పించు_Unreadback("/*verilator " + పునాది_Cmd + "*/ \"" + arg + "\" /**/");
        } లేకపోతే {
            ఒకవేళ (!printed) చొప్పించు_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_పంక్తి* V3PreProc::fileline() {
    స్థిరరాశి V3PreProcImp* idatap = static_cast<V3PreProcImp*>(ఇది);
    తిరిగి idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

స్థిరరాశి అక్షరం* V3PreProcImp::tokenName(పూర్ణాంకం tok) {
    ఎంపిక (tok) {
    స్థితి VP_BACKQUOTE: తిరిగి ("BACKQUOTE");
    స్థితి VP_COMMENT: తిరిగి ("COMMENT");
    స్థితి VP_DEFARG: తిరిగి ("DEFARG");
    స్థితి VP_DEFFORM: తిరిగి ("DEFFORM");
    స్థితి VP_DEFINE: తిరిగి ("DEFINE");
    స్థితి VP_DEFREF: తిరిగి ("DEFREF");
    స్థితి VP_DEFREF_JOIN: తిరిగి ("DEFREF_JOIN");
    స్థితి VP_DEFVALUE: తిరిగి ("DEFVALUE");
    స్థితి VP_ELSE: తిరిగి ("ELSE");
    స్థితి VP_ELSIF: తిరిగి ("ELSIF");
    స్థితి VP_ENDIF: తిరిగి ("ENDIF");
    స్థితి VP_EOF: తిరిగి ("EOF");
    స్థితి VP_EOF_ERROR: తిరిగి ("EOF_ERROR");
    స్థితి VP_ERROR: తిరిగి ("ERROR");
    స్థితి VP_IFDEF: తిరిగి ("IFDEF");
    స్థితి VP_IFNDEF: తిరిగి ("IFNDEF");
    స్థితి VP_JOIN: తిరిగి ("JOIN");
    స్థితి VP_INCLUDE: తిరిగి ("INCLUDE");
    స్థితి VP_పంక్తి: తిరిగి ("LINE");
    స్థితి VP_STRIFY: తిరిగి ("STRIFY");
    స్థితి VP_STRING: తిరిగి ("STRING");
    స్థితి VP_చిహ్నం: తిరిగి ("SYMBOL");
    స్థితి VP_చిహ్నం_JOIN: తిరిగి ("SYMBOL_JOIN");
    స్థితి VP_పాఠ్యం: తిరిగి ("TEXT");
    స్థితి VP_UNDEF: తిరిగి ("UNDEF");
    స్థితి VP_UNDEFINEALL: తిరిగి ("UNDEFINEALL");
    స్థితి VP_WHITE: తిరిగి ("WHITE");
    అప్రమేయ: తిరిగి ("?");
    }
}

శూన్య V3PreProcImp::unputString(స్థిరరాశి string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    ఒకవేళ (VL_UNCOVERABLE(m_lexp->m_బఫరు_State != m_lexp->current_బఫరు())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->పరిశీలన_బైట్లు(strg);
}

శూన్య V3PreProcImp::unputDefrefString(స్థిరరాశి string& strg) {
    స్థిరరాశి పూర్ణాంకం multiline = std::లెక్క(strg.begin(), strg.చివర(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(స్థిరరాశి string& strg, తార్కిక trailing) {
    // Remove leading whitespace
    string బహిర్గతం = strg;
    string::కొలత_type leadspace = 0;
    అయితే (బహిర్గతం.నిడివి() > leadspace && std::ఖాళీనా(బహిర్గతం[leadspace])) ++leadspace;
    ఒకవేళ (leadspace) బహిర్గతం.erase(0, leadspace);
    // Remove trailing whitespace
    ఒకవేళ (trailing) {
        string::కొలత_type trailspace = 0;
        అయితే (బహిర్గతం.నిడివి() > trailspace && std::ఖాళీనా(బహిర్గతం[బహిర్గతం.నిడివి() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        ఒకవేళ (trailspace && బహిర్గతం.నిడివి() > trailspace && బహిర్గతం[బహిర్గతం.నిడివి() - 1 - trailspace] == '\\')
            --trailspace;
        ఒకవేళ (trailspace) బహిర్గతం.erase(బహిర్గతం.నిడివి() - trailspace, trailspace);
    }
    తిరిగి బహిర్గతం;
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
    కొరకు (గుర్తులేని i = 0; i < refp->వాదనలు().కొలత(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->వాదనలు()[i] << "'");
    }
    // Grab value
    స్థిరరాశి string విలువ = def_విలువ(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(విలువ) << "'");

    std::map<స్థిరరాశి string, string> arg_విలువ_By_Name;
    {  // Parse argument list into map
        గుర్తులేని అంకె_వాదనలు = 0;
        string argName;
        పూర్ణాంకం paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        తార్కిక quote = అబద్ధం;
        తార్కిక haveDefault = అబద్ధం;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        స్థిరరాశి string params = refp->params();  // Must keep str in scope to get pointer
        స్థిరరాశి అక్షరం* cp = params.c_str();
        ఒకవేళ (*cp == '(') cp++;
        కొరకు (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            ఒకవేళ (!quote && paren == 1) {
                ఒకవేళ (*cp == ')' || *cp == ',') {
                    string విలువ_Def;
                    ఒకవేళ (haveDefault) {
                        విలువ_Def = token;
                    } లేకపోతే {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, నిజం);
                    UINFO(4, "    Got Arg=" << అంకె_వాదనలు << "  argName='" << argName
                                            << "'  default='" << విలువ_Def << "'");
                    // Parse it
                    ఒకవేళ (argName != "") {
                        ఒకవేళ (refp->వాదనలు().కొలత() > అంకె_వాదనలు) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            స్థిరరాశి string arg = trimWhitespace(refp->వాదనలు()[అంకె_వాదనలు], నిజం);
                            ఒకవేళ (arg != "") విలువ_Def = arg;
                        } లేకపోతే ఒకవేళ (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            తిరిగి " `" + refp->name() + " ";
                        }
                        అంకె_వాదనలు++;
                    }
                    arg_విలువ_By_Name[argName] = విలువ_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = అబద్ధం;
                    కొనసాగు;
                } లేకపోతే ఒకవేళ (*cp == '=') {
                    haveDefault = నిజం;
                    argName = token;
                    token = "";
                    కొనసాగు;
                }
            }
            ఒకవేళ (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                కొనసాగు;
            }
            ఒకవేళ (!quote) {
                ఒకవేళ (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } లేకపోతే ఒకవేళ (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            ఒకవేళ (*cp == '"') quote = !quote;
            ఒకవేళ (*cp) token += *cp;
        }
        ఒకవేళ (refp->వాదనలు().కొలత() > అంకె_వాదనలు
            // `define X() is ok to call with nothing
            && !(refp->వాదనలు().కొలత() == 1 && అంకె_వాదనలు == 0
                 && trimWhitespace(refp->వాదనలు()[0], అబద్ధం) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            తిరిగి " `" + refp->name() + " ";
        }
    }

    string బహిర్గతం;
    {  // Parse substitution define using arguments
        string argName;
        తార్కిక quote = అబద్ధం;
        తార్కిక triquote = అబద్ధం;
        తార్కిక backslashesc = అబద్ధం;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        కొరకు (స్థిరరాశి అక్షరం* cp = విలువ.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            ఒకవేళ (!quote && !triquote && *cp == '\\') {
                backslashesc = నిజం;
            } లేకపోతే ఒకవేళ (std::ఖాళీనా(*cp)) {
                backslashesc = అబద్ధం;
            }
            ఒకవేళ (!quote && !triquote) {
                ఒకవేళ (std::అక్షరమా(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::అంకెనా(*cp))) {
                    argName += *cp;
                    కొనసాగు;
                }
                ఒకవేళ (argName != "") {
                    // Found a possible variable substitution
                    స్థిరరాశి స్వయంచాలక iter = arg_విలువ_By_Name.కనుగొను(argName);
                    ఒకవేళ (iter != arg_విలువ_By_Name.చివర()) {
                        // Substitute
                        స్థిరరాశి string subst = iter->రెండవ;
                        ఒకవేళ (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            ఒకవేళ (బహిర్గతం.కొలత() >= 2 && బహిర్గతం.substr(బహిర్గతం.కొలత() - 2) == "``") {
                                బహిర్గతం.resize(బహిర్గతం.కొలత() - 2);
                            }
                        } లేకపోతే {
                            బహిర్గతం += subst;
                        }
                    } లేకపోతే {
                        బహిర్గతం += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                ఒకవేళ (cp[0] == '`' && cp[1] == '`') {
                    ఒకవేళ (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } లేకపోతే {
                        బహిర్గతం += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    కొనసాగు;
                } లేకపోతే ఒకవేళ (cp[0] == '`' && cp[1] == '"') {
                    బహిర్గతం += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    కొనసాగు;
                } లేకపోతే ఒకవేళ (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    బహిర్గతం += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    కొనసాగు;
                } లేకపోతే ఒకవేళ (cp[0] == '`' && cp[1] == '\\') {
                    బహిర్గతం += '\\';  // `\ means to put out a backslash
                    cp++;
                    కొనసాగు;
                } లేకపోతే ఒకవేళ (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    బహిర్గతం += "\n";  // \\n newline
                    cp++;
                    కొనసాగు;
                }
            }
            ఒకవేళ (cp[0] == '\\' && cp[1] == '\"') {
                బహిర్గతం += cp[0];  // \{any} Put out literal next character
                బహిర్గతం += cp[1];
                cp++;
                కొనసాగు;
            } లేకపోతే ఒకవేళ (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                బహిర్గతం += cp[0];
                కొనసాగు;
            }
            ఒకవేళ (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                బహిర్గతం += "\"\"\"";
                cp += 2;
                కొనసాగు;
            }
            ఒకవేళ (*cp == '"') quote = !quote;
            ఒకవేళ (*cp) బహిర్గతం += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(బహిర్గతం) << "'");
    తిరిగి బహిర్గతం;
}

//**********************************************************************
// Parser routines

శూన్య V3PreProcImp::openFile(File_పంక్తి*, VInFilter* filterp, స్థిరరాశి string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    ఒకవేళ (m_incError) తిరిగి;
    m_lexp->సమితి_YYDebug(debug() >= 5);
    V3File::కూడు_Src_Depend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    స్థిరరాశి తార్కిక ok = filterp->readWholefile(filename, wholefile /*ref*/);
    ఒకవేళ (!ok) {
        fileline()->v3error("File not found: " + filename);
        తిరిగి;
    }

    ఒకవేళ (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        ఒకవేళ (m_lexp->m_streamp_దొంతర.కొలత() > V3PreProc::INCLUDE_లోతు_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = నిజం;
            తిరిగి;
        }
        // There's already a file active.  Push it to work on the new one.
        కూడు_పంక్తి_Comment(0);
    }

    // Save file contents for future error reporting
    File_పంక్తి* స్థిరరాశి flsp = కొత్త File_పంక్తి{filename};
    flsp->lineno(1);
    flsp->newContent();
    కొరకు (స్థిరరాశి string& i : wholefile) flsp->contentp()->నెట్టు_పాఠ్యం(i);

    // Save contents for V3Control --contents
    ఒకవేళ (filename != V3Options::get_Std_Package_మార్గం()) {
        తార్కిక కలిగియుంది_Vlt = అబద్ధం;
        కొరకు (స్థిరరాశి string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            ఒకవేళ (i.కనుగొను("`verilator_config") != string::npos) {
                కలిగియుంది_Vlt = నిజం;
                ఆపు;
            }
        }
        ఒకవేళ (!కలిగియుంది_Vlt) {
            కొరకు (స్థిరరాశి string& i : wholefile) V3Control::contents_నెట్టు_పాఠ్యం(i);
        }
    }

    // Create new stream structure
    m_lexp->పరిశీలన_New_File(flsp);
    కూడు_పంక్తి_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    పూర్ణాంకం eof_newline = 0;  // Number of characters following last newline
    పూర్ణాంకం eof_lineno = 1;
    కొరకు (StrList::iterator it = wholefile.begin(); it != wholefile.చివర(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        తార్కిక strip = అబద్ధం;
        స్థిరరాశి అక్షరం* స్థిరరాశి sp = it->దత్తాంశం();
        స్థిరరాశి అక్షరం* స్థిరరాశి ep = sp + it->నిడివి();
        // Only process if needed, as saves extra string allocations
        కొరకు (స్థిరరాశి అక్షరం* cp = sp; cp < ep; cp++) {
            ఒకవేళ (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = నిజం;
            } లేకపోతే ఒకవేళ (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } లేకపోతే {
                ++eof_newline;
            }
        }
        ఒకవేళ (strip) {
            string బహిర్గతం;
            బహిర్గతం.reserve(it->నిడివి());
            కొరకు (స్థిరరాశి అక్షరం* cp = sp; cp < ep; cp++) {
                ఒకవేళ (!(*cp == '\r' || *cp == '\0')) బహిర్గతం += *cp;
            }
            *it = బహిర్గతం;
        }

        // Push the data to an internal buffer.
        m_lexp->పరిశీలన_బైట్లు_వెనుక(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    ఒకవేళ (eof_newline) {
        File_పంక్తి* స్థిరరాశి fl = కొత్త File_పంక్తి{flsp};
        fl->contentLineno(eof_lineno);
        fl->స్తంభం_2(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warn_మరిన్ని() << "... Suggest add newline.");
    }
}

శూన్య V3PreProcImp::చొప్పించు_Unreadback_At_Bol(స్థిరరాశి string& పాఠ్యం) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    ఒకవేళ (m_పంక్తి_Cmt == "") {
        m_పంక్తి_Cmt_Nl = నిజం;
    } లేకపోతే ఒకవేళ (m_పంక్తి_Cmt[m_పంక్తి_Cmt.నిడివి() - 1] != '\n') {
        చొప్పించు_Unreadback("\n");
    }
    చొప్పించు_Unreadback(పాఠ్యం);
}

శూన్య V3PreProcImp::కూడు_పంక్తి_Comment(పూర్ణాంకం enterExit) {
    ఒకవేళ (పంక్తి_Directives()) {
        చొప్పించు_Unreadback_At_Bol(m_lexp->curFilelinep()->పంక్తి_Directive_Strg(enterExit));
    }
}

శూన్య V3PreProcImp::dump_నిర్వచిస్తుంది(std::ostream& os) {
    కొరకు (నిర్వచిస్తుంది_Map::const_iterator it = m_నిర్వచిస్తుంది.begin(); it != m_నిర్వచిస్తుంది.చివర(); ++it) {
        os << "`define " << it->మొదటి;
        // No need to print "()" below as already part of params()
        ఒకవేళ (!it->రెండవ.params().ఖాళీలు()) os << it->రెండవ.params();
        ఒకవేళ (!it->రెండవ.విలువ().ఖాళీలు()) os << " " << it->రెండవ.విలువ();
        os << '\n';
    }
}

శూన్య V3PreProcImp::candidate_నిర్వచిస్తుంది(VSpellCheck* spellerp) {
    కొరకు (నిర్వచిస్తుంది_Map::const_iterator it = m_నిర్వచిస్తుంది.begin(); it != m_నిర్వచిస్తుంది.చివర(); ++it) {
        spellerp->నెట్టు_Candidate("`"s + it->మొదటి);
    }
}

పూర్ణాంకం V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    అయితే (నిజం) {
    తదుపరి_tok:
        ఒకవేళ (m_పంక్తి_కూడు) {
            m_పంక్తి_కూడు--;
            m_rawAtBol = నిజం;
            yyourtext("\n", 1);
            ఒకవేళ (debug() >= 5) debugToken(VP_WHITE, "LNA");
            తిరిగి VP_WHITE;
        }
        ఒకవేళ (m_పంక్తి_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            స్థిర string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_పంక్తి_Cmt;
            ఒకవేళ (m_పంక్తి_Cmt_Nl) {
                ఒకవేళ (!m_rawAtBol) s_rtncmt.చొప్పించు(0, "\n");
                m_పంక్తి_Cmt_Nl = అబద్ధం;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.నిడివి());
            m_పంక్తి_Cmt = "";
            ఒకవేళ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            ఒకవేళ (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_విలువ(yyourtext(), yyourleng());
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                ఒకవేళ (debug() >= 5) debugToken(VP_పాఠ్యం, "LCM");
                తిరిగి VP_పాఠ్యం;
            }
        }
        ఒకవేళ (isEof()) తిరిగి VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->ప్రారంభం_Token();
        పూర్ణాంకం tok = m_lexp->lex();
        ఒకవేళ (debug() >= 5) debugToken(tok, "RAW");

        ఒకవేళ (m_చివరి_Lineno != m_lexp->m_tokFilelinep->lineno()) {
            m_చివరి_Lineno = m_lexp->m_tokFilelinep->lineno();
            m_టోకెన్లు_On_పంక్తి = 0;
        } లేకపోతే ఒకవేళ (++m_టోకెన్లు_On_పంక్తి > v3Global.opt.preproc_Token_హద్దు()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvt_గమ్యం_Str(v3Global.opt.preproc_Token_హద్దు())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        ఒకవేళ (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            ఒకవేళ (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            వెళ్ళు తదుపరి_tok;
        }

        ఒకవేళ (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        తిరిగి tok;
    }
}

శూన్య V3PreProcImp::debugToken(పూర్ణాంకం tok, స్థిరరాశి అక్షరం* cmtp) {
    స్థిరరాశి స్థిర పూర్ణాంకం s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    ఒకవేళ (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::కొలత_type pos;
        అయితే ((pos = buf.కనుగొను('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        అయితే ((pos = buf.కనుగొను('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        స్థిరరాశి string flcol = m_lexp->m_tokFilelinep->ascii_పంక్తి_నిలువు();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<పూర్ణాంకం>(m_స్థితులు.కొలత())
                       << ") dr" << m_defRefs.కొలత() << ":  <" << m_lexp->current_ప్రారంభం_State()
                       << ">" << tokenName(tok) << ": " << buf);
        ఒకవేళ (s_debugFileline >= 9) {
            std::బయటకు << m_lexp->m_tokFilelinep->warnContextSecondary() << పంక్తి_ముగింపు;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

పూర్ణాంకం V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    అయితే (నిజం) {
    తదుపరి_tok:
        ఒకవేళ (isEof()) తిరిగి VP_EOF;
        పూర్ణాంకం tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        ఒకవేళ (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) తిరిగి tok;
        ఒకవేళ (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_పాఠ్యం;
        ఒకవేళ (tok == VP_COMMENT) {
            ఒకవేళ (!m_off) {
                ఒకవేళ (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.కేటాయించు_2(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    ఒకవేళ (m_పంక్తి_Cmt.ఖాళీలు()) చొప్పించు_Unreadback(" ");
                } లేకపోతే ఒకవేళ (m_lexp->m_keepComments) {
                    తిరిగి tok;
                } లేకపోతే {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    చొప్పించు_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            కొరకు (కొలత_t len = 0; len < static_cast<కొలత_t>(yyourleng()); len++) {
                ఒకవేళ (yyourtext()[len] == '\n') m_పంక్తి_కూడు++;
            }
            వెళ్ళు తదుపరి_tok;
        }
        ఒకవేళ (tok == VP_పంక్తి) {
            కూడు_పంక్తి_Comment(m_lexp->m_enterExit);
            వెళ్ళు తదుపరి_tok;
        }

        ఒకవేళ (tok == VP_DEFREF_JOIN) {
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
            స్థిరరాశి std::string name(yyourtext() + 1, yyourleng() - 1);
            ఒకవేళ (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = నిజం;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } లేకపోతే {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        ఒకవేళ (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            ఒకవేళ (tok == VP_JOIN) వెళ్ళు తదుపరి_tok;
            ఒకవేళ (tok == VP_చిహ్నం_JOIN) tok = VP_చిహ్నం;
        }
        ఒకవేళ (tok == VP_చిహ్నం_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            స్థిరరాశి std::string బహిర్గతం(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << బహిర్గతం);
            // a``b``c can have multiple joins, so we need a stack
            m_join_దొంతర.నెట్టు(బహిర్గతం);
            state_నెట్టు(ps_JOIN);
            వెళ్ళు తదుపరి_tok;
        }

        // Deal with some special parser states
        ఎంపిక (state()) {
        స్థితి ps_శిఖరం: {
            ఆపు;
        }
        స్థితి ps_DEFNAME_UNDEF:  // FALLTHRU
        స్థితి ps_DEFNAME_DEFINE:  // FALLTHRU
        స్థితి ps_DEFNAME_IFDEF:  // FALLTHRU
        స్థితి ps_DEFNAME_IFNDEF:  // FALLTHRU
        స్థితి ps_DEFNAME_ELSIF: {
            ఒకవేళ (tok == VP_చిహ్నం) {
                m_చివరి_Sym.కేటాయించు_2(yyourtext(), yyourleng());
                ఒకవేళ (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    తార్కిక enable = defExists(m_చివరి_Sym);
                    UINFO(4, "Ifdef " << m_చివరి_Sym << (enable ? " ON" : " OFF"));
                    ఒకవేళ (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_దొంతర.emplace(VPre_If_నమోదు{enable, అబద్ధం});
                    ఒకవేళ (!enable) parsingOff();
                    state_తీయి();
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే ఒకవేళ (state() == ps_DEFNAME_ELSIF) {
                    ఒకవేళ (m_ifdef_దొంతర.ఖాళీలు()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } లేకపోతే {
                        // Handle `else portion
                        స్థిరరాశి VPre_If_నమోదు చివరి_If = m_ifdef_దొంతర.శిఖరం();
                        m_ifdef_దొంతర.తీయి();
                        ఒకవేళ (!చివరి_If.on()) parsingOn();
                        // Handle `if portion
                        స్థిరరాశి తార్కిక enable = !చివరి_If.everOn() && defExists(m_చివరి_Sym);
                        UINFO(4, "Elsif " << m_చివరి_Sym << (enable ? " ON" : " OFF"));
                        m_ifdef_దొంతర.emplace(VPre_If_నమోదు{enable, చివరి_If.everOn()});
                        ఒకవేళ (!enable) parsingOff();
                    }
                    state_తీయి();
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే ఒకవేళ (state() == ps_DEFNAME_UNDEF) {
                    ఒకవేళ (!m_off) {
                        UINFO(4, "Undef " << m_చివరి_Sym);
                        undef(m_చివరి_Sym);
                    }
                    state_తీయి();
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే ఒకవేళ (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->నెట్టు_State_Def_Form();
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం) {
                // IE, something like comment between define and symbol
                ఒకవేళ (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->నెట్టు_State_సమాసం();
                    m_సమాసం_Parser.reset(fileline());
                    m_సమాసం_Paren_Level = 1;
                    ఎంపిక (state()) {
                    స్థితి ps_DEFNAME_IFDEF: stateChange(ps_సమాసం_IFDEF); ఆపు;
                    స్థితి ps_DEFNAME_IFNDEF: stateChange(ps_సమాసం_IFNDEF); ఆపు;
                    స్థితి ps_DEFNAME_ELSIF: stateChange(ps_సమాసం_ELSIF); ఆపు;
                    అప్రమేయ: v3fatalSrc("bad case");
                    }
                    వెళ్ళు తదుపరి_tok;
                }
                ఒకవేళ (!m_off) {
                    తిరిగి tok;
                } లేకపోతే {
                    వెళ్ళు తదుపరి_tok;
                }
            } లేకపోతే ఒకవేళ (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                ఆపు;
            } లేకపోతే {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి ps_సమాసం_IFDEF:  // FALLTHRU
        స్థితి ps_సమాసం_IFNDEF:  // FALLTHRU
        స్థితి ps_సమాసం_ELSIF: {
            // `ifdef ( *here*
            File_పంక్తి* స్థిరరాశి flp = m_lexp->m_tokFilelinep;
            ఒకవేళ (tok == VP_చిహ్నం) {
                m_చివరి_Sym.కేటాయించు_2(yyourtext(), yyourleng());
                స్థిరరాశి తార్కిక exists = defExists(m_చివరి_Sym);
                ఒకవేళ (exists) {
                    స్థిరరాశి std::string విలువ = def_విలువ(m_చివరి_Sym);
                    ఒకవేళ (VString::removeWhitespace(విలువ) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_చివరి_Sym << "' with value '" << విలువ
                                << "'\n"
                                   "... Suggest change define '"
                                << m_చివరి_Sym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, exists});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_WHITE) {
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::BRA});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_సమాసం_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                ఒకవేళ (--m_సమాసం_Paren_Level > 0) {
                    m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::KET});
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే {
                    // Done with parsing expression
                    తార్కిక enable = m_సమాసం_Parser.ఫలితం();
                    UINFO(4, "ifdef() result=" << enable);
                    ఒకవేళ (state() == ps_సమాసం_IFDEF || state() == ps_సమాసం_IFNDEF) {
                        ఒకవేళ (state() == ps_సమాసం_IFNDEF) enable = !enable;
                        m_ifdef_దొంతర.emplace(VPre_If_నమోదు{enable, అబద్ధం});
                        ఒకవేళ (!enable) parsingOff();
                        state_తీయి();
                        వెళ్ళు తదుపరి_tok;
                    } లేకపోతే ఒకవేళ (state() == ps_సమాసం_ELSIF) {
                        ఒకవేళ (m_ifdef_దొంతర.ఖాళీలు()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } లేకపోతే {
                            // Handle `else portion
                            స్థిరరాశి VPre_If_నమోదు చివరి_If = m_ifdef_దొంతర.శిఖరం();
                            m_ifdef_దొంతర.తీయి();
                            ఒకవేళ (!చివరి_If.on()) parsingOn();
                            // Handle `if portion
                            enable = !చివరి_If.everOn() && enable;
                            UINFO(4, "Elsif " << m_చివరి_Sym << (enable ? " ON" : " OFF"));
                            m_ifdef_దొంతర.emplace(VPre_If_నమోదు{enable, చివరి_If.everOn()});
                            ఒకవేళ (!enable) parsingOff();
                        }
                        state_తీయి();
                    }
                    వెళ్ళు తదుపరి_tok;
                }
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::LNOT});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 2 && 0 == అక్షరశ్రేణి_పోలిక(yyourtext(), "&&", 2)) {
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::LAND});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 2 && 0 == అక్షరశ్రేణి_పోలిక(yyourtext(), "||", 2)) {
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::LOR});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 2 && 0 == అక్షరశ్రేణి_పోలిక(yyourtext(), "->", 2)) {
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::IMP});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 3 && 0 == అక్షరశ్రేణి_పోలిక(yyourtext(), "<->", 3)) {
                m_సమాసం_Parser.నెట్టు_ఆదానం(V3_Pre_సమాసం_Token{flp, V3_Pre_సమాసం_Token::EQV});
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                ఒకవేళ (VString::removeWhitespace(string{yyourtext(), yyourleng()}).ఖాళీలు()) {
                    తిరిగి tok;
                } లేకపోతే {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి ps_DEFFORM: {
            ఒకవేళ (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_విలువ;
                ఒకవేళ (debug() >= 5) {
                    బయటకు << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->నెట్టు_State_Def_విలువ();
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం) {
                // IE, something like comment in formals
                ఒకవేళ (!m_off) తిరిగి tok;
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి ps_DEFVALUE: {
            స్థిర string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            ఒకవేళ (tok == VP_DEFVALUE) {
                ఒకవేళ (debug() >= 5) {  // LCOV_EXCL_START
                    బయటకు << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_విలువ)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                స్థిరరాశి string formals = m_formals;
                string విలువ = m_lexp->m_def_విలువ;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                కొరకు (కొలత_t i = 0; i < formals.నిడివి(); i++) {
                    ఒకవేళ (formals[i] == '\n') s_newlines += "\n";
                }
                కొరకు (కొలత_t i = 0; i < విలువ.నిడివి(); i++) {
                    ఒకవేళ (విలువ[i] == '\n') s_newlines += "\n";
                }
                ఒకవేళ (!m_off) {
                    // Remove leading and trailing whitespace
                    విలువ = trimWhitespace(విలువ, నిజం);
                    // Define it
                    UINFO(4, "Define " << m_చివరి_Sym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(విలువ) << "'");
                    define(fileline(), m_చివరి_Sym, విలువ, formals, అబద్ధం);
                }
            } లేకపోతే {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_తీయి();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.నిడివి());
            తిరిగి VP_WHITE;
        }
        స్థితి ps_DEFPAREN: {
            ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                UASSERT(!m_defRefs.ఖాళీలు(), "Shouldn't be in DEFPAREN w/o active defref");
                స్థిరరాశి VDefineRef* స్థిరరాశి refp = &(m_defRefs.శిఖరం());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి ps_DEFARG: {
            UASSERT(!m_defRefs.ఖాళీలు(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.శిఖరం());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_విలువ);
            m_lexp->m_def_విలువ = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            ఒకవేళ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->వాదనలు().నెట్టు_వెనుక(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->నెట్టు_State_Def_Arg(1);
                refp->nextarg("");
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->వాదనలు().నెట్టు_వెనుక(refp->nextarg());
                string బహిర్గతం;
                ఒకవేళ (!m_off) {
                    బహిర్గతం = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.తీయి(), refp);
                state_తీయి();
                ఒకవేళ (m_defRefs.ఖాళీలు() || state() == ps_STRIFY || state() == ps_JOIN) {
                    ఒకవేళ (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_దొంతర.ఖాళీలు(), "`` join stack empty, but in a ``");
                        స్థిరరాశి string lhs = m_join_దొంతర.శిఖరం();
                        m_join_దొంతర.తీయి();
                        బహిర్గతం.చొప్పించు(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << బహిర్గతం);
                        state_తీయి();
                    }
                    ఒకవేళ (!m_off) {
                        unputDefrefString(బహిర్గతం);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    లేకపోతే ఒకవేళ (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.ఖాళీలు() ? 0 : m_defRefs.శిఖరం().parenLevel();
                } లేకపోతే {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.శిఖరం());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_విలువ + బహిర్గతం);
                    m_lexp->m_def_విలువ = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                ఆపు;
            } లేకపోతే ఒకవేళ (tok == VP_చిహ్నం || tok == VP_STRING || tok == VP_పాఠ్యం || tok == VP_WHITE) {
                string rtn;
                rtn.కేటాయించు_2(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_నెట్టు(ps_STRIFY);
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి ps_INCNAME: {
            ఒకవేళ (tok == VP_STRING) {
                state_తీయి();
                m_చివరి_Sym.కేటాయించు_2(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_చివరి_Sym);
                // Drop leading and trailing quotes.
                m_చివరి_Sym.erase(0, 1);
                m_చివరి_Sym.erase(m_చివరి_Sym.నిడివి() - 1, 1);
                include(m_చివరి_Sym);
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_పాఠ్యం && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->నెట్టు_State_Inc_Filename();
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                ఆపు;
            } లేకపోతే {
                state_తీయి();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                వెళ్ళు తదుపరి_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        స్థితి ps_ERRORNAME: {
            ఒకవేళ (tok == VP_STRING) {
                ఒకవేళ (!m_off) {
                    m_చివరి_Sym.కేటాయించు_2(yyourtext(), yyourleng());
                    fileline()->v3error(m_చివరి_Sym);
                }
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి ps_JOIN: {
            ఒకవేళ (tok == VP_చిహ్నం || tok == VP_పాఠ్యం || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_దొంతర.ఖాళీలు(), "`` join stack empty, but in a ``");
                స్థిరరాశి string lhs = m_join_దొంతర.శిఖరం();
                m_join_దొంతర.తీయి();
                UINFO(5, "`` LHS:" << lhs);
                స్థిరరాశి string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                స్థిరరాశి string బహిర్గతం = lhs + rhs;
                UINFO(5, "`` Out:" << బహిర్గతం);
                unputString(బహిర్గతం);
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                స్థిరరాశి string lhs = m_join_దొంతర.శిఖరం();
                m_join_దొంతర.తీయి();
                unputString(lhs);
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే {
                // `define, etc, fall through and expand.  Pop back here.
                ఆపు;
            }
        }
        స్థితి ps_STRIFY: {
            ఒకవేళ (tok == VP_STRIFY) {
                // Quote what's in the middle of the stringification
                // Note a `" MACRO_WITH(`") `" doesn't need to be handled (we don't need a stack)
                // That behavior isn't specified, and other simulators vary widely
                string బహిర్గతం = m_strify;
                m_strify = "";
                // Convert any newlines to spaces, so we don't get a
                // multiline "..." without \ escapes.
                // The spec is silent about this either way; simulators vary
                std::replace(బహిర్గతం.begin(), బహిర్గతం.చివర(), '\n', ' ');
                unputString("\""s + బహిర్గతం + "\"");
                state_తీయి();
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                ఆపు;
            } లేకపోతే ఒకవేళ (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                వెళ్ళు తదుపరి_tok;
            } లేకపోతే ఒకవేళ (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                ఆపు;
            } లేకపోతే {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                వెళ్ళు తదుపరి_tok;
            }
        }
        అప్రమేయ: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        ఎంపిక (tok) {
        స్థితి VP_INCLUDE:
            ఒకవేళ (!m_off) {
                state_నెట్టు(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            వెళ్ళు తదుపరి_tok;
        స్థితి VP_UNDEF: state_నెట్టు(ps_DEFNAME_UNDEF); వెళ్ళు తదుపరి_tok;
        స్థితి VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_నెట్టు(ps_DEFNAME_DEFINE);
            వెళ్ళు తదుపరి_tok;
        స్థితి VP_IFDEF: state_నెట్టు(ps_DEFNAME_IFDEF); వెళ్ళు తదుపరి_tok;
        స్థితి VP_IFNDEF: state_నెట్టు(ps_DEFNAME_IFNDEF); వెళ్ళు తదుపరి_tok;
        స్థితి VP_ELSIF: state_నెట్టు(ps_DEFNAME_ELSIF); వెళ్ళు తదుపరి_tok;
        స్థితి VP_ELSE:
            ఒకవేళ (m_ifdef_దొంతర.ఖాళీలు()) {
                fileline()->v3error("`else with no matching `if");
            } లేకపోతే {
                స్థిరరాశి VPre_If_నమోదు చివరి_If = m_ifdef_దొంతర.శిఖరం();
                m_ifdef_దొంతర.తీయి();
                స్థిరరాశి తార్కిక enable = !చివరి_If.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_దొంతర.emplace(VPre_If_నమోదు{enable, చివరి_If.everOn()});
                ఒకవేళ (!చివరి_If.on()) parsingOn();
                ఒకవేళ (!enable) parsingOff();
            }
            వెళ్ళు తదుపరి_tok;
        స్థితి VP_ENDIF:
            UINFO(4, "Endif ");
            ఒకవేళ (m_ifdef_దొంతర.ఖాళీలు()) {
                fileline()->v3error("`endif with no matching `if");
            } లేకపోతే {
                స్థిరరాశి VPre_If_నమోదు చివరి_If = m_ifdef_దొంతర.శిఖరం();
                m_ifdef_దొంతర.తీయి();
                ఒకవేళ (!చివరి_If.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            వెళ్ళు తదుపరి_tok;

        స్థితి VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            స్థిరరాశి string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            ఒకవేళ (m_defPutJoin) {
                m_defPutJoin = అబద్ధం;
                unputString("``");
            }
            ఒకవేళ (m_def_లోతు++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                వెళ్ళు తదుపరి_tok;
            }
            // Substitute
            ఒకవేళ (!defExists(name)) {  // Not found, return original string as-is
                m_def_లోతు = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                ఒకవేళ (m_off) {
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    వెళ్ళు తదుపరి_tok;
                }
            } లేకపోతే {
                స్థిరరాశి string params = defParams(name);
                ఒకవేళ (params == "0" || params == "") {  // Found, as simple substitution
                    string బహిర్గతం;
                    ఒకవేళ (!m_off) {
                        VDefineRef tempref{name, ""};
                        బహిర్గతం = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    ఒకవేళ (m_defRefs.ఖాళీలు() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        ఒకవేళ (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_దొంతర.ఖాళీలు(), "`` join stack empty, but in a ``");
                            స్థిరరాశి string lhs = m_join_దొంతర.శిఖరం();
                            m_join_దొంతర.తీయి();
                            బహిర్గతం.చొప్పించు(0, lhs);
                            UINFO(5, "``-end-defref Out:" << బహిర్గతం);
                            state_తీయి();
                        }
                        ఒకవేళ (!m_off) {
                            unputDefrefString(బహిర్గతం);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        లేకపోతే ఒకవేళ (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } లేకపోతే {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* స్థిరరాశి refp = &(m_defRefs.శిఖరం());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_విలువ + బహిర్గతం);
                        m_lexp->m_def_విలువ = "";
                    }
                    వెళ్ళు తదుపరి_tok;
                } లేకపోతే {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    ఒకవేళ (!m_defRefs.ఖాళీలు()) m_defRefs.శిఖరం().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_నెట్టు(ps_DEFPAREN);
                    m_lexp->నెట్టు_State_Def_Arg(0);
                    వెళ్ళు తదుపరి_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            వెళ్ళు తదుపరి_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        స్థితి VP_ERROR: {
            state_నెట్టు(ps_ERRORNAME);
            వెళ్ళు తదుపరి_tok;
        }
        స్థితి VP_EOF:
            ఒకవేళ (!m_ifdef_దొంతర.ఖాళీలు()) fileline()->v3error("`ifdef not terminated at EOF");
            తిరిగి tok;
        స్థితి VP_UNDEFINEALL:
            ఒకవేళ (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            వెళ్ళు తదుపరి_tok;
        స్థితి VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_నెట్టు(ps_STRIFY);
            వెళ్ళు తదుపరి_tok;
        స్థితి VP_చిహ్నం:
        స్థితి VP_STRING:
        స్థితి VP_పాఠ్యం: {
            m_def_లోతు = 0;
            ఒకవేళ (!m_off) {
                తిరిగి tok;
            } లేకపోతే {
                వెళ్ళు తదుపరి_tok;
            }
        }
        స్థితి VP_WHITE:  // Handled at top of loop
        స్థితి VP_COMMENT:  // Handled at top of loop
        స్థితి VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        స్థితి VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        అప్రమేయ:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            ఆపు;  // LCOV_EXCL_LINE
        }
        తిరిగి tok;
    }
}

పూర్ణాంకం V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    ఒకవేళ (!m_finAhead) {
        m_finAhead = నిజం;
        m_finToken = getStateToken();
        m_finBuf.కేటాయించు_2(yyourtext(), yyourleng());
    }
    స్థిరరాశి పూర్ణాంకం tok = m_finToken;
    buf = m_finBuf;
    ఒకవేళ (అబద్ధం && debug() >= 5) {
        స్థిరరాశి string bufcln = V3PreLex::cleanDbgStrg(buf);
        స్థిరరాశి string flcol = m_lexp->m_tokFilelinep->ascii_పంక్తి_నిలువు();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    స్థిరరాశి అక్షరం* bufp = buf.c_str();
    అయితే (*bufp == '\n') bufp++;
    ఒకవేళ ((tok == VP_పాఠ్యం || tok == VP_పంక్తి) && VString::startsWith(bufp, "`line ")) {
        పూర్ణాంకం enter;
        m_finFilelinep->పంక్తి_Directive(bufp, enter /*ref*/);
    } లేకపోతే {
        ఒకవేళ (m_finAtBol && !(tok == VP_పాఠ్యం && buf == "\n") && m_preprocp->పంక్తి_Directives()) {
            ఒకవేళ (స్థిరరాశి పూర్ణాంకం బహిర్గతం_Behind
                = (m_lexp->m_tokFilelinep->చివరి_Lineno() - m_finFilelinep->చివరి_Lineno())) {
                ఒకవేళ (debug() >= 5) {
                    స్థిరరాశి string flcol = m_lexp->m_tokFilelinep->ascii_పంక్తి_నిలువు();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->చివరి_Lineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->చివరి_Lineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->చివరి_Lineno());
                ఒకవేళ (బహిర్గతం_Behind > 0
                    && (బహిర్గతం_Behind <= static_cast<పూర్ణాంకం>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    ఒకవేళ (m_preprocp->keepWhitespace()) {
                        buf = std::string(బహిర్గతం_Behind, '\n');  // () for char repeat
                        తిరిగి VP_పాఠ్యం;
                    }
                } లేకపోతే {
                    // Need to backup, use `line
                    buf = m_finFilelinep->పంక్తి_Directive_Strg(0);
                    తిరిగి VP_పంక్తి;
                }
            }
        }
        // Track newlines in prep for next token
        కొరకు (స్థిరరాశి అక్షరం& c : buf) {
            ఒకవేళ (c == '\n') {
                m_finAtBol = నిజం;
                m_finFilelinep->linenoInc();
            } లేకపోతే {
                m_finAtBol = అబద్ధం;
            }
        }
    }
    m_finAhead = అబద్ధం;  // Consumed the token
    తిరిగి tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    ఒకవేళ (isEof()) తిరిగి "";
    స్థిరరాశి అక్షరం* rtnp;
    తార్కిక gotEof = అబద్ధం;
    అయితే (శూన్యసూచిక == (rtnp = std::strchr(m_పంక్తి_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        స్థిరరాశి పూర్ణాంకం tok = getFinalToken(m_tokenBuf /*ref*/);
        ఒకవేళ (debug() >= 5) {
            స్థిరరాశి string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            స్థిరరాశి string flcol = m_lexp->m_tokFilelinep->ascii_పంక్తి_నిలువు();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        ఒకవేళ (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            ఒకవేళ (m_పంక్తి_Chars != "" && m_పంక్తి_Chars[m_పంక్తి_Chars.నిడివి() - 1] != '\n') {
                m_పంక్తి_Chars.append("\n");
            }
            gotEof = నిజం;
        } లేకపోతే {
            m_పంక్తి_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    స్థిరరాశి పూర్ణాంకం len = rtnp - m_పంక్తి_Chars.c_str() + 1;
    string the_పంక్తి(m_పంక్తి_Chars, 0, len);
    m_పంక్తి_Chars = m_పంక్తి_Chars.erase(0, len);  // Remove returned characters
    ఒకవేళ (debug() >= 4) {
        స్థిరరాశి string lncln = V3PreLex::cleanDbgStrg(the_పంక్తి);
        స్థిరరాశి string flcol = m_lexp->m_tokFilelinep->ascii_పంక్తి_నిలువు();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    తిరిగి the_పంక్తి;
}
