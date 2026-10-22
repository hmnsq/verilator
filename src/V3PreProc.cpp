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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3PreProc.h"

#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3LanguageWords.h"
#அந்தர்பாவ "V3PreExpr.h"
#அந்தர்பாவ "V3PreLex.h"
#அந்தர்பாவ "V3PreShell.h"
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cstdlib>
#அந்தர்பாவ <fstream>
#அந்தர்பாவ <stack>
#அந்தர்பாவ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Build in LEX script

#define yyFlexLexer V3Lexer
#அந்தர்பாவ "V3PreLex.yy.cpp"
#undef yyFlexLexer

// YYSTYPE yylval;

//*************************************************************************

வகுப்பு VDefine final {
    // Define class.  One for each define.
    // string    m_name;         // Name of the define (list is keyed by this)
    File_வரி* மாறிலி m_fileline;  // Where it was declared
    மாறிலி string m_மதிப்பு;  // Value of define
    மாறிலி string m_params;  // Parameters
    மாறிலி தருக்க m_cmdline;  // Set on command line, don't `undefineall
பொது:
    VDefine(File_வரி* fl, மாறிலி string& மதிப்பு, மாறிலி string& params, தருக்க cmdline)
        : m_fileline{fl}
        , m_மதிப்பு{மதிப்பு}
        , m_params{params}
        , m_cmdline{cmdline} {}
    File_வரி* fileline() மாறிலி { திரும்பு m_fileline; }
    string மதிப்பு() மாறிலி { திரும்பு m_மதிப்பு; }
    string params() மாறிலி { திரும்பு m_params; }
    தருக்க cmdline() மாறிலி { திரும்பு m_cmdline; }
};

//*************************************************************************

வகுப்பு VDefineRef final {
    // One for each pending define substitution
    மாறிலி string m_name;  // Define last name being defined
    மாறிலி string m_params;  // Define parameter list for next expansion
    string m_nextarg;  // String being built for next argument
    முழு m_parenLevel = 0;  // Parenthesis counting inside def args (for PARENT not child)

    std::vector<string> m_args;  // List of define arguments
பொது:
    string name() மாறிலி { திரும்பு m_name; }
    string params() மாறிலி { திரும்பு m_params; }
    string nextarg() மாறிலி { திரும்பு m_nextarg; }
    வெறுமை nextarg(மாறிலி string& மதிப்பு) { m_nextarg = மதிப்பு; }
    முழு parenLevel() மாறிலி { திரும்பு m_parenLevel; }
    வெறுமை parenLevel(முழு மதிப்பு) { m_parenLevel = மதிப்பு; }
    std::vector<string>& args() { திரும்பு m_args; }
    VDefineRef(மாறிலி string& name, மாறிலி string& params)
        : m_name{name}
        , m_params{params} {}
    ~VDefineRef() = இயல்பு;
};

//*************************************************************************
/// Data for parsing on/off

வகுப்பு VPreIfEntry final {
    // One for each pending ifdef/ifndef
    மாறிலி தருக்க m_on;  // Current parse for this ifdef level is "on"
    மாறிலி தருக்க m_everOn;  // Some if term in elsif tree has been on
பொது:
    தருக்க on() மாறிலி { திரும்பு m_on; }
    தருக்க everOn() மாறிலி { திரும்பு m_everOn; }
    VPreIfEntry(தருக்க on, தருக்க everOn)
        : m_on{on}
        , m_everOn{everOn || on} {}  // Note everOn includes new state
    ~VPreIfEntry() = இயல்பு;
};

//*************************************************************************
// Data for a preprocessor instantiation.

வகுப்பு V3PreProcImp final : பொது V3PreProc {
பொது:
    // TYPES
    using DefinesMap = std::map<மாறிலி std::string, VDefine>;
    using StrList = VInFilter::StrList;

    // Defines list
    DefinesMap m_defines;  ///< Map of defines

    // STATE
    மாறிலி V3PreProc* m_preprocp = nullptr;  ///< Object we're holding data for
    V3PreLex* m_lexp = nullptr;  ///< Current lexer state (nullptr = closed)
    முழு m_lastLineno = 0;  // Last line number (stall detection)
    முழு m_tokens_On_வரி = 0;  // Number of tokens on line (stall detection)

    எண்ணிக்கை ProcState : uint8_t {
        ps_உச்சி,
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
        ps_கோவை_IFDEF,
        ps_கோவை_IFNDEF,
        ps_கோவை_ELSIF,
        ps_INCNAME,
        ps_JOIN,
        ps_STRIFY
    };
    நிலையான மாறிலி எழுத்து* procStateName(ProcState s) {
        நிலையான மாறிலி எழுத்து* மாறிலி states[]
            = {"ps_TOP",           "ps_DEFNAME_UNDEF",  "ps_DEFNAME_DEFINE",
               "ps_DEFNAME_IFDEF", "ps_DEFNAME_IFNDEF", "ps_DEFNAME_ELSIF",
               "ps_DEFARG",        "ps_DEFFORM",        "ps_DEFPAREN",
               "ps_DEFVALUE",      "ps_ERRORNAME",      "ps_EXPR_IFDEF",
               "ps_EXPR_IFNDEF",   "ps_EXPR_ELSIF",     "ps_INCNAME",
               "ps_JOIN",          "ps_STRIFY"};
        திரும்பு states[s];
    }

    std::அடுக்கு<ProcState> m_states;  ///< Current state of parser
    முழு m_off = 0;  ///< If non-zero, ifdef level is turned off, don't dump text
    தருக்க m_incError = பொய்;  ///< Include error found
    string m_lastSym;  ///< Last symbol name found.
    string m_formals;  ///< Last formals found

    // For getRawToken/ `line insertion
    string m_வரி_Cmt;  ///< Line comment(s) to be returned
    தருக்க m_வரி_Cmt_Nl = பொய்;  ///< Newline needed before inserting lineCmt
    முழு m_வரி_Add = 0;  ///< Empty lines to return to maintain line count
    தருக்க m_rawAtBol = உண்மை;  ///< Last rawToken left us at beginning of line

    // For getFinalToken
    தருக்க m_finAhead = பொய்;  ///< Have read a token ahead
    முழு m_finToken = 0;  ///< Last token read
    string m_finBuf;  ///< Last yytext read
    தருக்க m_finAtBol = உண்மை;  ///< Last getFinalToken left us at beginning of line
    File_வரி* m_finFilelinep = nullptr;  ///< Location of last returned token (internal only)

    // For stringification
    string m_strify;  ///< Text to be stringified

    // For defines
    std::அடுக்கு<VDefineRef> m_defRefs;  ///< Pending define substitution
    std::அடுக்கு<VPreIfEntry> m_ifdef_அடுக்கு;  ///< Stack of true/false emitting evaluations
    unsigned m_defDepth = 0;  ///< How many `defines deep
    தருக்க m_defPutJoin = பொய்;  ///< Insert `` after substitution

    // For `` join
    std::அடுக்கு<string> m_join_அடுக்கு;  ///< Text on lhs of join

    // for `ifdef () expressions
    V3_Pre_கோவை m_கோவை_Parser;  ///< Parser for () expression
    முழு m_கோவை_Paren_Level = 0;  ///< Number of ( deep in `ifdef () expression

    // For getline()
    string m_வரி_Chars;  ///< Characters left for next line

    string m_tokenBuf;  // Token buffer, to avoid repeated alloc/free

    வெறுமை v3errorEnd(std::ostringstream& str) VL_RELEASE(V3Error::s().m_mutex) {
        fileline()->v3errorEnd(str);
    }

    நிலையான மாறிலி எழுத்து* tokenName(முழு tok);
    வெறுமை debugToken(முழு tok, மாறிலி எழுத்து* cmtp);
    வெறுமை parse_உச்சி();
    வெறுமை parseUndef();

தனிப்பட்ட:
    // Internal methods
    வெறுமை endOfOneFile();
    string defineSubst(VDefineRef* refp);

    தருக்க defExists(மாறிலி string& name);
    தருக்க defCmdline(மாறிலி string& name);
    string def_மதிப்பு(மாறிலி string& name);
    string defParams(மாறிலி string& name);
    File_வரி* defFileline(மாறிலி string& name);

    string commentCleanup(மாறிலி string& text);
    தருக்க commentTokenMatch(string& cmdr, மாறிலி எழுத்து* strg);
    நிலையான string trimWhitespace(மாறிலி string& strg, தருக்க trailing);
    வெறுமை unputString(மாறிலி string& strg);
    வெறுமை unputDefrefString(மாறிலி string& strg);

    வெறுமை parsingOn() {
        m_off--;
        எனில் (m_off < 0) v3fatalSrc("Underflow of parsing cmds");
        // addLineComment no longer needed; getFinalToken will correct.
    }
    வெறுமை parsingOff() { m_off++; }

    முழு getRawToken();
    முழு getStateToken();
    முழு getFinalToken(string& buf);

    ProcState state() மாறிலி { திரும்பு m_states.உச்சி(); }
    தருக்க stateIsDefname() மாறிலி {
        திரும்பு state() == ps_DEFNAME_UNDEF || state() == ps_DEFNAME_DEFINE
               || state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
               || state() == ps_DEFNAME_ELSIF;
    }
    வெறுமை state_தள்ளு(ProcState state) { m_states.தள்ளு(state); }
    வெறுமை state_எடு() {
        m_states.எடு();
        எனில் (VL_UNCOVERABLE(m_states.empty())) {
            fileline()->v3fatalSrc("Pop of parser state with nothing on stack");
            m_states.தள்ளு(ps_உச்சி);  // LCOV_EXCL_LINE
        }
    }
    வெறுமை stateChange(ProcState state) {
        state_எடு();
        state_தள்ளு(state);
    }

பொது:
    // METHODS, called from upper level shell
    வெறுமை openFile(File_வரி* fl, VInFilter* filterp, மாறிலி string& filename) override;
    தருக்க isEof() மாறிலி override { திரும்பு m_lexp->curStreamp()->m_eof; }
    வெறுமை forceEof() { m_lexp->curStreamp()->m_eof = உண்மை; }
    string getline() override;
    வெறுமை செருகு_Unreadback(மாறிலி string& text) override { m_வரி_Cmt += text; }
    வெறுமை செருகு_Unreadback_At_Bol(மாறிலி string& text);
    வெறுமை add_வரி_Comment(முழு enterExit);
    வெறுமை dumpDefines(std::ostream& os) override;
    வெறுமை candidateDefines(VSpellCheck* spellerp) override;

    // METHODS, callbacks
    வெறுமை comment(மாறிலி string& text) override;  // Comment detected (if keepComments==2)
    வெறுமை include(மாறிலி string& filename) override;  // Request a include file be processed
    வெறுமை undef(மாறிலி string& name) override;
    மெய்நிகர் வெறுமை undefineall();
    வெறுமை define(File_வரி* fl, மாறிலி string& name, மாறிலி string& மதிப்பு, மாறிலி string& params,
                தருக்க cmdline) override;
    string removeDefines(மாறிலி string& text) override;  // Remove defines in a text string

    // CONSTRUCTORS
    V3PreProcImp() { m_states.தள்ளு(ps_உச்சி); }
    // cppcheck-suppress duplInheritedMember
    வெறுமை configure(File_வரி* filelinep) {
        // configure() separate from constructor to avoid calling abstract functions
        m_preprocp = இது;  // Silly, but to make code more similar to Verilog-Perl
        m_finFilelinep = புதிய File_வரி{filelinep->filename()};
        m_finFilelinep->lineno(1);
        // Create lexer
        m_lexp = புதிய V3PreLex{இது, filelinep};
        m_lexp->m_keepComments = keepComments();
        m_lexp->m_keepWhitespace = keepWhitespace();
        m_lexp->m_pedantic = pedantic();
    }
    ~V3PreProcImp() override {
        எனில் (m_lexp) VL_DO_CLEAR(delete m_lexp, m_lexp = nullptr);
    }
};

//*************************************************************************
// Creation

V3PreProc* V3PreProc::createPreProc(File_வரி* fl) {
    V3PreProcImp* மாறிலி preprocp = புதிய V3PreProcImp;
    preprocp->configure(fl);
    திரும்பு preprocp;
}

வெறுமை V3PreProc::selfTest() VL_MT_DISABLED { V3_Pre_கோவை::selfTest(); }

//*************************************************************************
// Defines

வெறுமை V3PreProcImp::undef(மாறிலி string& name) {
    எனில் (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        செருகு_Unreadback("`undef " + name + "\n");
    m_defines.erase(name);
}
வெறுமை V3PreProcImp::undefineall() {
    எனில் (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines())
        செருகு_Unreadback("`undefineall\n");
    க்கு (DefinesMap::iterator nextit, it = m_defines.begin(); it != m_defines.end(); it = nextit) {
        nextit = it;
        ++nextit;
        எனில் (!it->second.cmdline()) m_defines.erase(it);
    }
}
தருக்க V3PreProcImp::defExists(மாறிலி string& name) {
    மாறிலி auto iter = m_defines.find(name);
    திரும்பு (iter != m_defines.end());
}
தருக்க V3PreProcImp::defCmdline(மாறிலி string& name) {
    மாறிலி auto iter = m_defines.find(name);
    எனில் (iter == m_defines.end()) { திரும்பு பொய்; }
    திரும்பு iter->second.cmdline();
}
string V3PreProcImp::def_மதிப்பு(மாறிலி string& name) {
    மாறிலி auto iter = m_defines.find(name);
    எனில் (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        திரும்பு "";
    }
    திரும்பு iter->second.மதிப்பு();
}
string V3PreProcImp::defParams(மாறிலி string& name) {
    மாறிலி auto iter = m_defines.find(name);
    எனில் (iter == m_defines.end()) {
        fileline()->v3error("Define or directive not defined: `" + name);
        திரும்பு "";
    }
    திரும்பு iter->second.params();
}
File_வரி* V3PreProcImp::defFileline(மாறிலி string& name) {
    மாறிலி auto iter = m_defines.find(name);
    எனில் (iter == m_defines.end()) திரும்பு nullptr;
    திரும்பு iter->second.fileline();
}
வெறுமை V3PreProcImp::define(File_வரி* fl, மாறிலி string& name, மாறிலி string& மதிப்பு,
                          மாறிலி string& params, தருக்க cmdline) {
    UINFO(4, "DEFINE '" << name << "' as '" << மதிப்பு << "' params '" << params << "'");
    எனில் (v3Global.opt.preprocOnly() && v3Global.opt.preprocDefines()) {
        // Any newlines in the string must have backslash escape so can re-parse properly
        // If there was a comment with backslash, it got stripped because IEEE says
        // the backslash is not part of the value
        string out;
        out.reserve(மதிப்பு.size());
        க்கு (string::size_type pos = 0; pos < மதிப்பு.size(); ++pos) {
            எனில் (மதிப்பு[pos] == '\n' && (pos == 0 || மதிப்பு[pos - 1] != '\\')) out += '\\';
            out += மதிப்பு[pos];
        }
        செருகு_Unreadback("`define " + name + params + (out.empty() ? "" : " ") + out + "\n");
    }
    எனில் (!V3_Language_சொற்கள்::isKeyword("`"s + name).empty()) {
        fl->v3error("Attempting to define built-in directive: '`" << name
                                                                  << "' (IEEE 1800-2023 22.5.1)");
    } இல்லையெனில் {
        எனில் (defExists(name)) {
            எனில் (defCmdline(name) && !cmdline) {
                fl->v3warn(DEFOVERRIDE,
                           "Overriding define: '"
                               << name << "' with value: '" << மதிப்பு
                               << "' to existing command line define value: '" << def_மதிப்பு(name)
                               << (params == "" ? "" : " ") << params << "'\n"
                               << fl->warnContextPrimary() << '\n'
                               << defFileline(name)->warnOther()
                               << "... Location of previous definition, with value: '"
                               << def_மதிப்பு(name) << (defParams(name).empty() ? "" : " ")
                               << defParams(name) << "'\n"
                               << defFileline(name)->warnContextSecondary());
                திரும்பு;
            } இல்லையெனில் {
                எனில் (!(def_மதிப்பு(name) == மதிப்பு
                      && defParams(name) == params)) {  // Duplicate defs are OK
                    fl->v3warn(REDEFMACRO,
                               "Redefining existing define: '"
                                   << name << "', with different value: '" << மதிப்பு
                                   << (params == "" ? "" : " ") << params << "'\n"
                                   << fl->warnContextPrimary() << '\n'
                                   << defFileline(name)->warnOther()
                                   << "... Location of previous definition, with value: '"
                                   << def_மதிப்பு(name) << (defParams(name).empty() ? "" : " ")
                                   << defParams(name) << "'\n"
                                   << defFileline(name)->warnContextSecondary());
                }
                undef(name);
            }
        }

        m_defines.emplace(name, VDefine{fl, மதிப்பு, params, cmdline});
    }
}

string V3PreProcImp::removeDefines(மாறிலி string& text) {
    string val;
    string rtnsym = text;
    க்கு (முழு loopprevent = 0; loopprevent < 100; loopprevent++) {
        string xsym = rtnsym;
        எனில் (xsym[0] == '`') xsym.erase(0, 1);
        எனில் (defExists(xsym)) {
            val = def_மதிப்பு(xsym);
            எனில் (val != rtnsym) {
                rtnsym = val;  // Prevent infinite loop if have `define X X
            } இல்லையெனில் {
                நிறுத்து;
            }
        } இல்லையெனில் {
            நிறுத்து;
        }
    }
    திரும்பு rtnsym;  // NA
}

//**********************************************************************
// Comments

வெறுமை V3PreProcImp::include(மாறிலி string& filename) {
    // Include seen.  Ask the preprocessor shell to call back around to us
    V3PreShell::preprocInclude(fileline(), filename);
}

string V3PreProcImp::commentCleanup(மாறிலி string& text) {
    // Cleanup comment for easier parsing (call before commentTokenMatch)
    string cmd = text;
    string::size_type pos;
    போது ((pos = cmd.find("//")) != string::npos) cmd.replace(pos, 2, "");
    போது ((pos = cmd.find("/*")) != string::npos) cmd.replace(pos, 2, "");
    போது ((pos = cmd.find("*/")) != string::npos) cmd.replace(pos, 2, "");
    போது ((pos = cmd.find('\"')) != string::npos) cmd.replace(pos, 1, " ");
    போது ((pos = cmd.find('\t')) != string::npos) cmd.replace(pos, 1, " ");
    போது ((pos = cmd.find("  ")) != string::npos) cmd.replace(pos, 2, " ");
    போது (!cmd.empty() && std::isspace(cmd[cmd.size() - 1])) cmd.erase(cmd.size() - 1);
    திரும்பு cmd;
}

தருக்க V3PreProcImp::commentTokenMatch(string& cmdr, மாறிலி எழுத்து* strg) {
    முழு len = std::strlen(strg);
    எனில் (VString::startsWith(cmdr, strg) && (cmdr[len] == '\0' || std::isspace(cmdr[len]))) {
        எனில் (std::isspace(cmdr[len])) len++;
        cmdr = cmdr.substr(len);
        திரும்பு உண்மை;
    } இல்லையெனில் {
        திரும்பு பொய்;
    }
}

வெறுமை V3PreProcImp::comment(மாறிலி string& text) {
    // Comment detected.  Only keep relevant data.
    தருக்க printed = பொய்;
    எனில் (v3Global.opt.preprocOnly() && v3Global.opt.preprocComments()) {
        செருகு_Unreadback(text);
        printed = உண்மை;
    }

    மாறிலி எழுத்து* cp = text.c_str();
    எனில் (cp[0] == '/' && (cp[1] == '/' || cp[1] == '*')) {
        cp += 2;
    } இல்லையெனில் {
        திரும்பு;
    }

    போது (std::isspace(*cp)) ++cp;

    தருக்க synth = பொய்;
    தருக்க vlcomment = பொய்;
    எனில் ((cp[0] == 'v' || cp[0] == 'V') && VString::startsWith(cp + 1, "erilator")) {
        cp += std::strlen("verilator");
        எனில் (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*verilator {...}*/ not /*verilator_{...}*/");
            திரும்பு;
        }
        vlcomment = உண்மை;
    } இல்லையெனில் எனில் (VString::startsWith(cp, "synopsys")) {
        cp += std::strlen("synopsys");
        எனில் (*cp == '_') {
            V3Control::applyIgnores(fileline());
            fileline()->v3warn(BADVLTPRAGMA, "Extra underscore in meta-comment, ignoring comment;"
                                             " use /*synopsys {...}*/ not /*synopsys_{...}*/");
            திரும்பு;
        }
        synth = உண்மை;
    } இல்லையெனில் எனில் (VString::startsWith(cp, "cadence")) {
        cp += std::strlen("cadence");
        synth = உண்மை;
    } இல்லையெனில் எனில் (VString::startsWith(cp, "pragma")) {
        cp += std::strlen("pragma");
        synth = உண்மை;
    } இல்லையெனில் எனில் (VString::startsWith(cp, "ambit synthesis")) {
        cp += std::strlen("ambit synthesis");
        synth = உண்மை;
    } இல்லையெனில் {
        திரும்பு;
    }

    போது (std::isspace(*cp)) ++cp;
    string cmd = commentCleanup(string{cp});
    // cmd now is comment without extra spaces and "verilator" prefix

    எனில் (synth) {
        எனில் (v3Global.opt.assertOn()) {
            // one_hot, one_cold, (full_case, parallel_case)
            எனில் (commentTokenMatch(cmd /*ref*/, "full_case")) {
                எனில் (!printed) செருகு_Unreadback("/*verilator full_case*/");
            }
            எனில் (commentTokenMatch(cmd /*ref*/, "parallel_case")) {
                எனில் (!printed) செருகு_Unreadback("/*verilator parallel_case*/");
            }
            // if (commentTokenMatch(cmd/*ref*/, "one_hot")) {
            //  insertUnreadback ("/*verilator one_hot*/ "+cmd+";");
            //}
            // if (commentTokenMatch(cmd/*ref*/, "one_cold")) {
            //  insertUnreadback ("/*verilator one_cold*/ "+cmd+";");
            //}
            // else ignore the comment we don't recognize
        }  // else no assertions
    } இல்லையெனில் எனில் (vlcomment
               // Drop 'public' if disabled on the command line
               && !(v3Global.opt.publicOff() && VString::startsWith(cmd, "public"))
               // Drop 'fargs', they are handled separately during option parsing
               && !VString::startsWith(cmd, "fargs")) {
        எனில் (VString::startsWith(cmd, "public_flat_rw")) {
            // "/*verilator public_flat_rw @(foo) */" -> "/*verilator public_flat_rw*/ @(foo)"
            string::size_type endOfCmd = std::strlen("public_flat_rw");
            போது (VString::isIdentifierChar(cmd[endOfCmd])) ++endOfCmd;
            string baseCmd = cmd.substr(0, endOfCmd);
            string arg = cmd.substr(endOfCmd);
            போது (std::isspace(arg[0])) arg = arg.substr(1);
            எனில் (arg.size() && baseCmd == "public_flat_rw_on")
                baseCmd += "_sns";  // different cmd for applying sensitivity
            எனில் (!printed) செருகு_Unreadback("/*verilator " + baseCmd + "*/ " + arg + " /**/");
        } இல்லையெனில் எனில் (VString::startsWith(cmd, "dpi_c_decl")) {
            // "/*verilator dpi_c_decl foo(bar) */" -> "/*verilator dpi_c_decl*/ \"foo(bar)\""
            string baseCmd = cmd.substr(0, std::strlen("dpi_c_decl"));
            string::size_type startOfArg = baseCmd.size();
            போது (std::isspace(cmd[startOfArg])) startOfArg++;
            string arg = cmd.substr(startOfArg);
            எனில் (arg.empty()) {
                fileline()->v3warn(
                    BADVLTPRAGMA,
                    "No function declaration provided in /*verilator dpi_c_decl*/ meta-comment");
                திரும்பு;
            }
            எனில் (!printed) செருகு_Unreadback("/*verilator " + baseCmd + "*/ \"" + arg + "\" /**/");
        } இல்லையெனில் {
            எனில் (!printed) செருகு_Unreadback("/*verilator " + cmd + "*/");
        }
    }
}

//*************************************************************************
// VPreProc Methods.

File_வரி* V3PreProc::fileline() {
    மாறிலி V3PreProcImp* idatap = static_cast<V3PreProcImp*>(இது);
    திரும்பு idatap->m_lexp->m_tokFilelinep;
}

//**********************************************************************
// Parser Utilities

மாறிலி எழுத்து* V3PreProcImp::tokenName(முழு tok) {
    தேர்வு (tok) {
    நிலை VP_BACKQUOTE: திரும்பு ("BACKQUOTE");
    நிலை VP_COMMENT: திரும்பு ("COMMENT");
    நிலை VP_DEFARG: திரும்பு ("DEFARG");
    நிலை VP_DEFFORM: திரும்பு ("DEFFORM");
    நிலை VP_DEFINE: திரும்பு ("DEFINE");
    நிலை VP_DEFREF: திரும்பு ("DEFREF");
    நிலை VP_DEFREF_JOIN: திரும்பு ("DEFREF_JOIN");
    நிலை VP_DEFVALUE: திரும்பு ("DEFVALUE");
    நிலை VP_ELSE: திரும்பு ("ELSE");
    நிலை VP_ELSIF: திரும்பு ("ELSIF");
    நிலை VP_ENDIF: திரும்பு ("ENDIF");
    நிலை VP_EOF: திரும்பு ("EOF");
    நிலை VP_EOF_ERROR: திரும்பு ("EOF_ERROR");
    நிலை VP_ERROR: திரும்பு ("ERROR");
    நிலை VP_IFDEF: திரும்பு ("IFDEF");
    நிலை VP_IFNDEF: திரும்பு ("IFNDEF");
    நிலை VP_JOIN: திரும்பு ("JOIN");
    நிலை VP_INCLUDE: திரும்பு ("INCLUDE");
    நிலை VP_வரி: திரும்பு ("LINE");
    நிலை VP_STRIFY: திரும்பு ("STRIFY");
    நிலை VP_STRING: திரும்பு ("STRING");
    நிலை VP_குறியீடு: திரும்பு ("SYMBOL");
    நிலை VP_குறியீடு_JOIN: திரும்பு ("SYMBOL_JOIN");
    நிலை VP_TEXT: திரும்பு ("TEXT");
    நிலை VP_UNDEF: திரும்பு ("UNDEF");
    நிலை VP_UNDEFINEALL: திரும்பு ("UNDEFINEALL");
    நிலை VP_WHITE: திரும்பு ("WHITE");
    இயல்பு: திரும்பு ("?");
    }
}

வெறுமை V3PreProcImp::unputString(மாறிலி string& strg) {
    // Note: The preliminary call in ::openFile bypasses this function
    // We used to just m_lexp->unputString(strg.c_str());
    // However this can lead to "flex scanner push-back overflow"
    // so instead we scan from a temporary buffer, then on EOF return.
    // This is also faster than the old scheme, amazingly.
    எனில் (VL_UNCOVERABLE(m_lexp->m_bufferState != m_lexp->currentBuffer())) {
        v3fatalSrc("bufferStack missing current buffer; will return incorrectly");
        // Hard to debug lost text as won't know till much later
    }
    m_lexp->scanBytes(strg);
}

வெறுமை V3PreProcImp::unputDefrefString(மாறிலி string& strg) {
    மாறிலி முழு multiline = std::மொத்தம்(strg.begin(), strg.end(), '\n');
    unputString(strg);
    // A define that inserts multiple newlines are really attributed to one source line,
    // so temporarily don't increment lineno.
    // Must be after unput - applies to new stream
    m_lexp->curStreamp()->m_ignNewlines += multiline;
}

string V3PreProcImp::trimWhitespace(மாறிலி string& strg, தருக்க trailing) {
    // Remove leading whitespace
    string out = strg;
    string::size_type leadspace = 0;
    போது (out.length() > leadspace && std::isspace(out[leadspace])) ++leadspace;
    எனில் (leadspace) out.erase(0, leadspace);
    // Remove trailing whitespace
    எனில் (trailing) {
        string::size_type trailspace = 0;
        போது (out.length() > trailspace && std::isspace(out[out.length() - 1 - trailspace]))
            ++trailspace;
        // Don't remove \{space_or_newline}
        எனில் (trailspace && out.length() > trailspace && out[out.length() - 1 - trailspace] == '\\')
            --trailspace;
        எனில் (trailspace) out.erase(out.length() - trailspace, trailspace);
    }
    திரும்பு out;
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
    க்கு (unsigned i = 0; i < refp->args().size(); i++) {
        UINFO(4, "defineArg[" << i << "] = '" << refp->args()[i] << "'");
    }
    // Grab value
    மாறிலி string மதிப்பு = def_மதிப்பு(refp->name());
    UINFO(4, "defineValue    '" << V3PreLex::cleanDbgStrg(மதிப்பு) << "'");

    std::map<மாறிலி string, string> arg_மதிப்பு_By_Name;
    {  // Parse argument list into map
        unsigned இலக்கம்_Args = 0;
        string argName;
        முழு paren = 1;  // (), {} and [] can use same counter, as must be matched pair per spec
        string token;
        தருக்க quote = பொய்;
        தருக்க haveDefault = பொய்;
        // Note there's a leading ( and trailing ), so parens==1 is the base parsing level
        மாறிலி string params = refp->params();  // Must keep str in scope to get pointer
        மாறிலி எழுத்து* cp = params.c_str();
        எனில் (*cp == '(') cp++;
        க்கு (; *cp; cp++) {
            // UINFO(4, "   Parse  Paren=" << paren << "  Arg=" << numArgs << "  token='" << token
            //                             << "' Parse=" << cp);
            எனில் (!quote && paren == 1) {
                எனில் (*cp == ')' || *cp == ',') {
                    string மதிப்பு_Def;
                    எனில் (haveDefault) {
                        மதிப்பு_Def = token;
                    } இல்லையெனில் {
                        argName = token;
                    }
                    argName = trimWhitespace(argName, உண்மை);
                    UINFO(4, "    Got Arg=" << இலக்கம்_Args << "  argName='" << argName
                                            << "'  default='" << மதிப்பு_Def << "'");
                    // Parse it
                    எனில் (argName != "") {
                        எனில் (refp->args().size() > இலக்கம்_Args) {
                            // A call `def( a ) must be equivalent to `def(a ), so trimWhitespace
                            // At one point we didn't trim trailing
                            // whitespace, but this confuses `"
                            மாறிலி string arg = trimWhitespace(refp->args()[இலக்கம்_Args], உண்மை);
                            எனில் (arg != "") மதிப்பு_Def = arg;
                        } இல்லையெனில் எனில் (!haveDefault) {
                            fileline()->v3error("Define missing argument '" + argName
                                                + "' for: " + refp->name());
                            திரும்பு " `" + refp->name() + " ";
                        }
                        இலக்கம்_Args++;
                    }
                    arg_மதிப்பு_By_Name[argName] = மதிப்பு_Def;
                    // Prepare for next
                    argName = "";
                    token = "";
                    haveDefault = பொய்;
                    தொடர்;
                } இல்லையெனில் எனில் (*cp == '=') {
                    haveDefault = உண்மை;
                    argName = token;
                    token = "";
                    தொடர்;
                }
            }
            எனில் (cp[0] == '\\' && cp[1]) {
                token += cp[0];  // \{any} Put out literal next character
                token += cp[1];
                cp++;
                தொடர்;
            }
            எனில் (!quote) {
                எனில் (*cp == '(' || *cp == '{' || *cp == '[') {
                    paren++;
                } இல்லையெனில் எனில் (*cp == ')' || *cp == '}' || *cp == ']') {
                    paren--;
                }
            }
            எனில் (*cp == '"') quote = !quote;
            எனில் (*cp) token += *cp;
        }
        எனில் (refp->args().size() > இலக்கம்_Args
            // `define X() is ok to call with nothing
            && !(refp->args().size() == 1 && இலக்கம்_Args == 0
                 && trimWhitespace(refp->args()[0], பொய்) == "")) {
            fileline()->v3error("Define passed too many arguments: " + refp->name());
            திரும்பு " `" + refp->name() + " ";
        }
    }

    string out;
    {  // Parse substitution define using arguments
        string argName;
        தருக்க quote = பொய்;
        தருக்க triquote = பொய்;
        தருக்க backslashesc = பொய்;  // In \.....{space} block
        // Note we go through the loop once more at the nullptr end-of-string
        க்கு (மாறிலி எழுத்து* cp = மதிப்பு.c_str(); (*cp) || argName != ""; cp = (*cp ? cp + 1 : cp)) {
            // UINFO(4, "CH " << *cp << "  an " << argName);
            எனில் (!quote && !triquote && *cp == '\\') {
                backslashesc = உண்மை;
            } இல்லையெனில் எனில் (std::isspace(*cp)) {
                backslashesc = பொய்;
            }
            எனில் (!quote && !triquote) {
                எனில் (std::isalpha(*cp)  //
                    || *cp == '_'  //
                    || *cp == '$'  // Won't replace system functions, since no $ in argValueByName
                    || (argName != "" && std::isdigit(*cp))) {
                    argName += *cp;
                    தொடர்;
                }
                எனில் (argName != "") {
                    // Found a possible variable substitution
                    மாறிலி auto iter = arg_மதிப்பு_By_Name.find(argName);
                    எனில் (iter != arg_மதிப்பு_By_Name.end()) {
                        // Substitute
                        மாறிலி string subst = iter->second;
                        எனில் (subst == "") {
                            // Normally `` is removed later, but with no token after, we're
                            // otherwise stuck, so remove proceeding ``
                            எனில் (out.size() >= 2 && out.substr(out.size() - 2) == "``") {
                                out.resize(out.size() - 2);
                            }
                        } இல்லையெனில் {
                            out += subst;
                        }
                    } இல்லையெனில் {
                        out += argName;
                    }
                    argName = "";
                }
                // Check for `` only after we've detected end-of-argname
                எனில் (cp[0] == '`' && cp[1] == '`') {
                    எனில் (backslashesc) {
                        // Don't put out the ``, we're forming an escape
                        // which will not expand further later
                    } இல்லையெனில் {
                        out += "``";  // `` must get removed later, as `FOO```BAR must pre-expand
                                      // FOO and BAR
                        // See also removal in empty substitutes above
                    }
                    cp++;
                    தொடர்;
                } இல்லையெனில் எனில் (cp[0] == '`' && cp[1] == '"') {
                    out += "`\"";  // `" means to put out a " without enabling quote mode (sort of)
                    // however we must expand any macro calls inside it first.
                    // So keep it `", so we don't enter quote mode.
                    cp++;
                    தொடர்;
                } இல்லையெனில் எனில் (cp[0] == '`' && cp[1] == '\\' && cp[2] == '`' && cp[3] == '"') {
                    out += "`\\`\"";  // `\`" means to put out a backslash quote
                    // Leave it literal until we parse the VP_STRIFY string
                    cp += 3;
                    தொடர்;
                } இல்லையெனில் எனில் (cp[0] == '`' && cp[1] == '\\') {
                    out += '\\';  // `\ means to put out a backslash
                    cp++;
                    தொடர்;
                } இல்லையெனில் எனில் (cp[0] == '\\' && cp[1] == '\n') {
                    // We kept the \\n when we lexed because we don't want whitespace
                    // trimming to mis-drop the final \\n
                    // At replacement time we need the standard newline.
                    out += "\n";  // \\n newline
                    cp++;
                    தொடர்;
                }
            }
            எனில் (cp[0] == '\\' && cp[1] == '\"') {
                out += cp[0];  // \{any} Put out literal next character
                out += cp[1];
                cp++;
                தொடர்;
            } இல்லையெனில் எனில் (cp[0] == '\\') {
                // Normally \{any} would put out literal next character
                // Instead we allow "`define A(nm) \nm" to expand, per proposed mantis1537
                out += cp[0];
                தொடர்;
            }
            எனில் (cp[0] == '"' && cp[1] == '"' && cp[2] == '"') {
                triquote = !triquote;
                out += "\"\"\"";
                cp += 2;
                தொடர்;
            }
            எனில் (*cp == '"') quote = !quote;
            எனில் (*cp) out += *cp;
        }
    }

    UINFO(4, "defineSubstOut '" << V3PreLex::cleanDbgStrg(out) << "'");
    திரும்பு out;
}

//**********************************************************************
// Parser routines

வெறுமை V3PreProcImp::openFile(File_வரி*, VInFilter* filterp, மாறிலி string& filename) {
    // Open a new file, possibly overriding the current one which is active.
    எனில் (m_incError) திரும்பு;
    m_lexp->setYYDebug(debug() >= 5);
    V3File::addSrcDepend(filename);

    // Read a list<string> with the whole file.
    StrList wholefile;
    மாறிலி தருக்க ok = filterp->readWholefile(filename, wholefile /*ref*/);
    எனில் (!ok) {
        fileline()->v3error("File not found: " + filename);
        திரும்பு;
    }

    எனில் (!m_preprocp->isEof()) {  // IE not the first file.
        // We allow the same include file twice, because occasionally it pops
        // up, with guards preventing a real recursion.
        எனில் (m_lexp->m_streamp_அடுக்கு.size() > V3PreProc::INCLUDE_DEPTH_MAX) {
            fileline()->v3error("Recursive inclusion of file: " + filename);
            // Include might be a tree of includes that is O(n^2) or worse.
            // Once hit this error then, ignore all further includes so can unwind.
            m_incError = உண்மை;
            திரும்பு;
        }
        // There's already a file active.  Push it to work on the new one.
        add_வரி_Comment(0);
    }

    // Save file contents for future error reporting
    File_வரி* மாறிலி flsp = புதிய File_வரி{filename};
    flsp->lineno(1);
    flsp->newContent();
    க்கு (மாறிலி string& i : wholefile) flsp->contentp()->தள்ளு_Text(i);

    // Save contents for V3Control --contents
    எனில் (filename != V3Options::getStdPackagePath()) {
        தருக்க containsVlt = பொய்;
        க்கு (மாறிலி string& i : wholefile) {
            // TODO this is overly sensitive, might be in a comment
            எனில் (i.find("`verilator_config") != string::npos) {
                containsVlt = உண்மை;
                நிறுத்து;
            }
        }
        எனில் (!containsVlt) {
            க்கு (மாறிலி string& i : wholefile) V3Control::contents_தள்ளு_Text(i);
        }
    }

    // Create new stream structure
    m_lexp->scanNewFile(flsp);
    add_வரி_Comment(1);  // Enter

    // Filter all DOS CR's en masse.  This avoids bugs with lexing CRs in the wrong places.
    // This will also strip them from strings, but strings aren't supposed
    // to be multi-line without a "\"
    முழு eof_newline = 0;  // Number of characters following last newline
    முழு eof_lineno = 1;
    க்கு (StrList::iterator it = wholefile.begin(); it != wholefile.end(); ++it) {
        // We don't end-loop at \0 as we allow and strip mid-string '\0's (for now).
        தருக்க strip = பொய்;
        மாறிலி எழுத்து* மாறிலி sp = it->data();
        மாறிலி எழுத்து* மாறிலி ep = sp + it->length();
        // Only process if needed, as saves extra string allocations
        க்கு (மாறிலி எழுத்து* cp = sp; cp < ep; cp++) {
            எனில் (VL_UNLIKELY(*cp == '\r' || *cp == '\0')) {
                strip = உண்மை;
            } இல்லையெனில் எனில் (VL_UNLIKELY(*cp == '\n')) {
                eof_newline = 0;
                ++eof_lineno;
            } இல்லையெனில் {
                ++eof_newline;
            }
        }
        எனில் (strip) {
            string out;
            out.reserve(it->length());
            க்கு (மாறிலி எழுத்து* cp = sp; cp < ep; cp++) {
                எனில் (!(*cp == '\r' || *cp == '\0')) out += *cp;
            }
            *it = out;
        }

        // Push the data to an internal buffer.
        m_lexp->scanBytesBack(*it);
        // Reclaim memory; the push saved the string contents for us
        *it = "";
    }

    // Warning check
    எனில் (eof_newline) {
        File_வரி* மாறிலி fl = புதிய File_வரி{flsp};
        fl->contentLineno(eof_lineno);
        fl->column(eof_newline + 1, eof_newline + 1);
        V3Control::applyIgnores(fl);  // As preprocessor hasn't otherwise applied yet
        fl->v3warn(EOFNEWLINE, "Missing newline at end of file (POSIX 3.206).\n"
                                   << fl->warnMore() << "... Suggest add newline.");
    }
}

வெறுமை V3PreProcImp::செருகு_Unreadback_At_Bol(மாறிலி string& text) {
    // Insert insuring we're at the beginning of line, for `line
    // We don't always add a leading newline, as it may result in extra unreadback(newlines).
    எனில் (m_வரி_Cmt == "") {
        m_வரி_Cmt_Nl = உண்மை;
    } இல்லையெனில் எனில் (m_வரி_Cmt[m_வரி_Cmt.length() - 1] != '\n') {
        செருகு_Unreadback("\n");
    }
    செருகு_Unreadback(text);
}

வெறுமை V3PreProcImp::add_வரி_Comment(முழு enterExit) {
    எனில் (வரி_Directives()) {
        செருகு_Unreadback_At_Bol(m_lexp->curFilelinep()->வரி_Directive_Strg(enterExit));
    }
}

வெறுமை V3PreProcImp::dumpDefines(std::ostream& os) {
    க்கு (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        os << "`define " << it->first;
        // No need to print "()" below as already part of params()
        எனில் (!it->second.params().empty()) os << it->second.params();
        எனில் (!it->second.மதிப்பு().empty()) os << " " << it->second.மதிப்பு();
        os << '\n';
    }
}

வெறுமை V3PreProcImp::candidateDefines(VSpellCheck* spellerp) {
    க்கு (DefinesMap::const_iterator it = m_defines.begin(); it != m_defines.end(); ++it) {
        spellerp->தள்ளு_Candidate("`"s + it->first);
    }
}

முழு V3PreProcImp::getRawToken() {
    // Get a token from the file, whatever it may be.
    போது (உண்மை) {
    next_tok:
        எனில் (m_வரி_Add) {
            m_வரி_Add--;
            m_rawAtBol = உண்மை;
            yyourtext("\n", 1);
            எனில் (debug() >= 5) debugToken(VP_WHITE, "LNA");
            திரும்பு VP_WHITE;
        }
        எனில் (m_வரி_Cmt != "") {
            // We have some `line directive or other processed data to return to the user.
            நிலையான string s_rtncmt;  // Keep the c string till next call
            s_rtncmt = m_வரி_Cmt;
            எனில் (m_வரி_Cmt_Nl) {
                எனில் (!m_rawAtBol) s_rtncmt.செருகு(0, "\n");
                m_வரி_Cmt_Nl = பொய்;
            }
            yyourtext(s_rtncmt.c_str(), s_rtncmt.length());
            m_வரி_Cmt = "";
            எனில் (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
            எனில் (state() == ps_DEFVALUE) {
                V3PreLex::s_currentLexp->append_Def_மதிப்பு(yyourtext(), yyourleng());
                செல் next_tok;
            } இல்லையெனில் {
                எனில் (debug() >= 5) debugToken(VP_TEXT, "LCM");
                திரும்பு VP_TEXT;
            }
        }
        எனில் (isEof()) திரும்பு VP_EOF;

        // Snarf next token from the file
        m_lexp->curFilelinep()->startToken();
        முழு tok = m_lexp->lex();
        எனில் (debug() >= 5) debugToken(tok, "RAW");

        எனில் (m_lastLineno != m_lexp->m_tokFilelinep->lineno()) {
            m_lastLineno = m_lexp->m_tokFilelinep->lineno();
            m_tokens_On_வரி = 0;
        } இல்லையெனில் எனில் (++m_tokens_On_வரி > v3Global.opt.preprocTokenLimit()) {
            fileline()->v3error("Too many preprocessor tokens on a line (>"
                                + cvtToStr(v3Global.opt.preprocTokenLimit())
                                + "); perhaps recursive `define");
            tok = VP_EOF_ERROR;
        }

        எனில் (tok == VP_EOF || tok == VP_EOF_ERROR) {
            // An error might be in an unexpected point, so stop parsing
            எனில் (tok == VP_EOF_ERROR) forceEof();
            // A EOF on an include, stream will find the EOF, after adding needed `lines
            செல் next_tok;
        }

        எனில் (yyourleng()) m_rawAtBol = (yyourtext()[yyourleng() - 1] == '\n');
        திரும்பு tok;
    }
}

வெறுமை V3PreProcImp::debugToken(முழு tok, மாறிலி எழுத்து* cmtp) {
    மாறிலி நிலையான முழு s_debugFileline
        = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    எனில் (debug() >= 5) {
        string buf{yyourtext(), yyourleng()};
        string::size_type pos;
        போது ((pos = buf.find('\n')) != string::npos) buf.replace(pos, 1, "\\n");
        போது ((pos = buf.find('\r')) != string::npos) buf.replace(pos, 1, "\\r");
        மாறிலி string flcol = m_lexp->m_tokFilelinep->ascii_வரி_Col();
        UINFO(0, flcol << ": " << cmtp << " " << (m_off ? "of" : "on") << " "
                       << procStateName(state()) << "(" << static_cast<முழு>(m_states.size())
                       << ") dr" << m_defRefs.size() << ":  <" << m_lexp->currentStartState()
                       << ">" << tokenName(tok) << ": " << buf);
        எனில் (s_debugFileline >= 9) {
            std::cout << m_lexp->m_tokFilelinep->warnContextSecondary() << endl;
        }
    }
}

// Sorry, we're not using bison/yacc. It doesn't handle returning white space
// in the middle of parsing other tokens.

முழு V3PreProcImp::getStateToken() {
    // Return the next state-determined token
    போது (உண்மை) {
    next_tok:
        எனில் (isEof()) திரும்பு VP_EOF;
        முழு tok = getRawToken();

        // Most states emit white space and comments between tokens. (Unless collecting a string)
        எனில் (tok == VP_WHITE && state() != ps_STRIFY && state() != ps_JOIN) திரும்பு tok;
        எனில் (tok == VP_BACKQUOTE && state() != ps_STRIFY) tok = VP_TEXT;
        எனில் (tok == VP_COMMENT) {
            எனில் (!m_off) {
                எனில் (m_lexp->m_keepComments == KEEPCMT_SUB) {
                    string rtn;
                    rtn.assign(yyourtext(), yyourleng());
                    comment(rtn);
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    எனில் (m_வரி_Cmt.empty()) செருகு_Unreadback(" ");
                } இல்லையெனில் எனில் (m_lexp->m_keepComments) {
                    திரும்பு tok;
                } இல்லையெனில் {
                    // Need to ensure "foo/**/bar" becomes two tokens, not one 'foobar'
                    செருகு_Unreadback(" ");
                }
            }
            // We're off or processed the comment specially.  If there are newlines
            // in it, we also return the newlines as TEXT so that the linenumber
            // count is maintained for downstream tools
            க்கு (size_t len = 0; len < static_cast<size_t>(yyourleng()); len++) {
                எனில் (yyourtext()[len] == '\n') m_வரி_Add++;
            }
            செல் next_tok;
        }
        எனில் (tok == VP_வரி) {
            add_வரி_Comment(m_lexp->m_enterExit);
            செல் next_tok;
        }

        எனில் (tok == VP_DEFREF_JOIN) {
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
            மாறிலி std::string name(yyourtext() + 1, yyourleng() - 1);
            எனில் (defExists(name)) {  // JOIN(DEFREF)
                // Put back the `` and process the defref
                UINFO(5, "```: define " << name << " exists, expand first");
                // After define, unputString("``").  Not now as would lose yyourtext()
                m_defPutJoin = உண்மை;
                UINFO(5, "TOKEN now DEFREF");
                tok = VP_DEFREF;
            } இல்லையெனில் {  // DEFREF(JOIN)
                UINFO(5, "```: define " << name << " doesn't exist, join first");
                // FALLTHRU, handle as with VP_SYMBOL_JOIN
            }
        }
        எனில் (state() == ps_STRIFY) {
            // Ignore joins and symbol joins in stringify as they don't affect the final string
            எனில் (tok == VP_JOIN) செல் next_tok;
            எனில் (tok == VP_குறியீடு_JOIN) tok = VP_குறியீடு;
        }
        எனில் (tok == VP_குறியீடு_JOIN  // not else if, can fallthru from above if()
            || tok == VP_DEFREF_JOIN || tok == VP_JOIN) {
            // a`` -> string doesn't include the ``, so can just grab next and continue
            மாறிலி std::string out(yyourtext(), yyourleng());
            UINFO(5, "`` LHS:" << out);
            // a``b``c can have multiple joins, so we need a stack
            m_join_அடுக்கு.தள்ளு(out);
            state_தள்ளு(ps_JOIN);
            செல் next_tok;
        }

        // Deal with some special parser states
        தேர்வு (state()) {
        நிலை ps_உச்சி: {
            நிறுத்து;
        }
        நிலை ps_DEFNAME_UNDEF:  // FALLTHRU
        நிலை ps_DEFNAME_DEFINE:  // FALLTHRU
        நிலை ps_DEFNAME_IFDEF:  // FALLTHRU
        நிலை ps_DEFNAME_IFNDEF:  // FALLTHRU
        நிலை ps_DEFNAME_ELSIF: {
            எனில் (tok == VP_குறியீடு) {
                m_lastSym.assign(yyourtext(), yyourleng());
                எனில் (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF) {
                    தருக்க enable = defExists(m_lastSym);
                    UINFO(4, "Ifdef " << m_lastSym << (enable ? " ON" : " OFF"));
                    எனில் (state() == ps_DEFNAME_IFNDEF) enable = !enable;
                    m_ifdef_அடுக்கு.emplace(VPreIfEntry{enable, பொய்});
                    எனில் (!enable) parsingOff();
                    state_எடு();
                    செல் next_tok;
                } இல்லையெனில் எனில் (state() == ps_DEFNAME_ELSIF) {
                    எனில் (m_ifdef_அடுக்கு.empty()) {
                        fileline()->v3error("`elsif with no matching `if");
                    } இல்லையெனில் {
                        // Handle `else portion
                        மாறிலி VPreIfEntry lastIf = m_ifdef_அடுக்கு.உச்சி();
                        m_ifdef_அடுக்கு.எடு();
                        எனில் (!lastIf.on()) parsingOn();
                        // Handle `if portion
                        மாறிலி தருக்க enable = !lastIf.everOn() && defExists(m_lastSym);
                        UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                        m_ifdef_அடுக்கு.emplace(VPreIfEntry{enable, lastIf.everOn()});
                        எனில் (!enable) parsingOff();
                    }
                    state_எடு();
                    செல் next_tok;
                } இல்லையெனில் எனில் (state() == ps_DEFNAME_UNDEF) {
                    எனில் (!m_off) {
                        UINFO(4, "Undef " << m_lastSym);
                        undef(m_lastSym);
                    }
                    state_எடு();
                    செல் next_tok;
                } இல்லையெனில் எனில் (state() == ps_DEFNAME_DEFINE) {
                    // m_lastSym already set.
                    stateChange(ps_DEFFORM);
                    m_lexp->தள்ளு_State_Def_Form();
                    செல் next_tok;
                } இல்லையெனில் {  // LCOV_EXCL_LINE
                    v3fatalSrc("Bad case");
                }
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT) {
                // IE, something like comment between define and symbol
                எனில் (yyourleng() == 1 && yyourtext()[0] == '('
                    && (state() == ps_DEFNAME_IFDEF || state() == ps_DEFNAME_IFNDEF
                        || state() == ps_DEFNAME_ELSIF)) {
                    UINFO(4, "ifdef() start (");
                    m_lexp->தள்ளு_State_கோவை();
                    m_கோவை_Parser.reset(fileline());
                    m_கோவை_Paren_Level = 1;
                    தேர்வு (state()) {
                    நிலை ps_DEFNAME_IFDEF: stateChange(ps_கோவை_IFDEF); நிறுத்து;
                    நிலை ps_DEFNAME_IFNDEF: stateChange(ps_கோவை_IFNDEF); நிறுத்து;
                    நிலை ps_DEFNAME_ELSIF: stateChange(ps_கோவை_ELSIF); நிறுத்து;
                    இயல்பு: v3fatalSrc("bad case");
                    }
                    செல் next_tok;
                }
                எனில் (!m_off) {
                    திரும்பு tok;
                } இல்லையெனில் {
                    செல் next_tok;
                }
            } இல்லையெனில் எனில் (tok == VP_DEFREF) {
                // IE, `ifdef `MACRO(x): Substitute and come back here when state pops.
                நிறுத்து;
            } இல்லையெனில் {
                fileline()->v3error("Expecting define name. Found: "s + tokenName(tok));
                செல் next_tok;
            }
        }
        நிலை ps_கோவை_IFDEF:  // FALLTHRU
        நிலை ps_கோவை_IFNDEF:  // FALLTHRU
        நிலை ps_கோவை_ELSIF: {
            // `ifdef ( *here*
            File_வரி* மாறிலி flp = m_lexp->m_tokFilelinep;
            எனில் (tok == VP_குறியீடு) {
                m_lastSym.assign(yyourtext(), yyourleng());
                மாறிலி தருக்க exists = defExists(m_lastSym);
                எனில் (exists) {
                    மாறிலி std::string மதிப்பு = def_மதிப்பு(m_lastSym);
                    எனில் (VString::removeWhitespace(மதிப்பு) == "0") {
                        flp->v3warn(
                            PREPROCZERO,
                            "Preprocessor expression evaluates define with 0: '"
                                << m_lastSym << "' with value '" << மதிப்பு
                                << "'\n"
                                   "... Suggest change define '"
                                << m_lastSym
                                << "' to non-zero value if used in preprocessor expression");
                    }
                }
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, exists});
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_WHITE) {
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::BRA});
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == ')') {
                UASSERT(m_கோவை_Paren_Level, "Underflow of ); should have exited ps_EXPR earlier?");
                எனில் (--m_கோவை_Paren_Level > 0) {
                    m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::KET});
                    செல் next_tok;
                } இல்லையெனில் {
                    // Done with parsing expression
                    தருக்க enable = m_கோவை_Parser.result();
                    UINFO(4, "ifdef() result=" << enable);
                    எனில் (state() == ps_கோவை_IFDEF || state() == ps_கோவை_IFNDEF) {
                        எனில் (state() == ps_கோவை_IFNDEF) enable = !enable;
                        m_ifdef_அடுக்கு.emplace(VPreIfEntry{enable, பொய்});
                        எனில் (!enable) parsingOff();
                        state_எடு();
                        செல் next_tok;
                    } இல்லையெனில் எனில் (state() == ps_கோவை_ELSIF) {
                        எனில் (m_ifdef_அடுக்கு.empty()) {
                            fileline()->v3error("`elsif with no matching `if");
                        } இல்லையெனில் {
                            // Handle `else portion
                            மாறிலி VPreIfEntry lastIf = m_ifdef_அடுக்கு.உச்சி();
                            m_ifdef_அடுக்கு.எடு();
                            எனில் (!lastIf.on()) parsingOn();
                            // Handle `if portion
                            enable = !lastIf.everOn() && enable;
                            UINFO(4, "Elsif " << m_lastSym << (enable ? " ON" : " OFF"));
                            m_ifdef_அடுக்கு.emplace(VPreIfEntry{enable, lastIf.everOn()});
                            எனில் (!enable) parsingOff();
                        }
                        state_எடு();
                    }
                    செல் next_tok;
                }
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '!') {
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::LNOT});
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "&&", 2)) {
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::LAND});
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "||", 2)) {
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::LOR});
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 2 && 0 == strncmp(yyourtext(), "->", 2)) {
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::IMP});
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 3 && 0 == strncmp(yyourtext(), "<->", 3)) {
                m_கோவை_Parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::EQV});
                செல் next_tok;
            } இல்லையெனில் {
                எனில் (VString::removeWhitespace(string{yyourtext(), yyourleng()}).empty()) {
                    திரும்பு tok;
                } இல்லையெனில் {
                    fileline()->v3error("Syntax error in `ifdef () expression; unexpected: '"s
                                        + tokenName(tok) + "'");
                }
                செல் next_tok;
            }
        }
        நிலை ps_DEFFORM: {
            எனில் (tok == VP_DEFFORM) {
                m_formals = m_lexp->m_def_மதிப்பு;
                எனில் (debug() >= 5) {
                    cout << "DefFormals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }
                stateChange(ps_DEFVALUE);
                m_lexp->தள்ளு_State_Def_மதிப்பு();
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT) {
                // IE, something like comment in formals
                எனில் (!m_off) திரும்பு tok;
                செல் next_tok;
            } இல்லையெனில் {
                fileline()->v3error("Expecting define formal arguments. Found: "s + tokenName(tok)
                                    + "\n");
                செல் next_tok;
            }
        }
        நிலை ps_DEFVALUE: {
            நிலையான string s_newlines;
            s_newlines = "\n";  // Always start with trailing return
            எனில் (tok == VP_DEFVALUE) {
                எனில் (debug() >= 5) {  // LCOV_EXCL_START
                    cout << "DefValue='" << V3PreLex::cleanDbgStrg(m_lexp->m_def_மதிப்பு)
                         << "'  formals='" << V3PreLex::cleanDbgStrg(m_formals) << "'\n";
                }  // LCOV_EXCL_STOP
                // Add any formals
                மாறிலி string formals = m_formals;
                string மதிப்பு = m_lexp->m_def_மதிப்பு;
                // Remove returns
                // Not removing returns in values has two problems,
                // 1. we need to correct line numbers with `line after each substitution
                // 2. Substituting in " .... " with embedded returns requires \ escape.
                //    This is very difficult in the presence of `", so we
                //    keep the \ before the newline.
                க்கு (size_t i = 0; i < formals.length(); i++) {
                    எனில் (formals[i] == '\n') s_newlines += "\n";
                }
                க்கு (size_t i = 0; i < மதிப்பு.length(); i++) {
                    எனில் (மதிப்பு[i] == '\n') s_newlines += "\n";
                }
                எனில் (!m_off) {
                    // Remove leading and trailing whitespace
                    மதிப்பு = trimWhitespace(மதிப்பு, உண்மை);
                    // Define it
                    UINFO(4, "Define " << m_lastSym << " " << formals << " = '"
                                       << V3PreLex::cleanDbgStrg(மதிப்பு) << "'");
                    define(fileline(), m_lastSym, மதிப்பு, formals, பொய்);
                }
            } இல்லையெனில் {
                v3fatalSrc("Bad define text, unexpected "s + tokenName(tok) + "\n");
            }
            state_எடு();
            // DEFVALUE is terminated by a return, but lex can't return both tokens.
            // Thus, we emit a return here.
            yyourtext(s_newlines.c_str(), s_newlines.length());
            திரும்பு VP_WHITE;
        }
        நிலை ps_DEFPAREN: {
            எனில் (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '(') {
                stateChange(ps_DEFARG);
                செல் next_tok;
            } இல்லையெனில் {
                UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFPAREN w/o active defref");
                மாறிலி VDefineRef* மாறிலி refp = &(m_defRefs.உச்சி());
                fileline()->v3error("Expecting ( to begin argument list for define reference `"s
                                    + refp->name());
                state_எடு();
                செல் next_tok;
            }
        }
        நிலை ps_DEFARG: {
            UASSERT(!m_defRefs.empty(), "Shouldn't be in DEFARG w/o active defref");
            VDefineRef* refp = &(m_defRefs.உச்சி());
            refp->nextarg(refp->nextarg() + m_lexp->m_def_மதிப்பு);
            m_lexp->m_def_மதிப்பு = "";
            UINFO(4, "defarg++ " << refp->nextarg());
            எனில் (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ',') {
                refp->args().தள்ளு_back(refp->nextarg());
                stateChange(ps_DEFARG);
                m_lexp->தள்ளு_State_Def_Arg(1);
                refp->nextarg("");
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_DEFARG && yyourleng() == 1 && yyourtext()[0] == ')') {
                // Substitute in and prepare for next action
                // Similar code in non-parenthesized define (Search for END_OF_DEFARG)
                refp->args().தள்ளு_back(refp->nextarg());
                string out;
                எனில் (!m_off) {
                    out = defineSubst(refp);
                    // NOP: out = m_preprocp->defSubstitute(out);
                }
                VL_DO_DANGLING(m_defRefs.எடு(), refp);
                state_எடு();
                எனில் (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                    எனில் (state()
                        == ps_JOIN) {  // Handle {left}```FOO(ARG) where `FOO(ARG) might be empty
                        UASSERT(!m_join_அடுக்கு.empty(), "`` join stack empty, but in a ``");
                        மாறிலி string lhs = m_join_அடுக்கு.உச்சி();
                        m_join_அடுக்கு.எடு();
                        out.செருகு(0, lhs);
                        UINFO(5, "``-end-defarg Out:" << out);
                        state_எடு();
                    }
                    எனில் (!m_off) {
                        unputDefrefString(out);
                    }
                    // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                    இல்லையெனில் எனில் (stateIsDefname()) {
                        unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                    }
                    // Stringification may still be inside another macro's argument list.
                    m_lexp->m_parenLevel = m_defRefs.empty() ? 0 : m_defRefs.உச்சி().parenLevel();
                } இல்லையெனில் {  // Finished a defref inside a upper defref,
                          // and not under stringification or join.
                    // Can't subst now, or
                    // `define a(ign) x,y
                    // foo(`a(ign),`b)  would break because a contains comma
                    refp = &(m_defRefs.உச்சி());  // We popped, so new top
                    refp->nextarg(refp->nextarg() + m_lexp->m_def_மதிப்பு + out);
                    m_lexp->m_def_மதிப்பு = "";
                    m_lexp->m_parenLevel = refp->parenLevel();
                    // Will go to ps_DEFARG, as we're under another define
                }
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_DEFREF) {
                // Expand it, then state will come back here
                // Value of building argument is data before the lower defref
                // we'll append it when we push the argument.
                நிறுத்து;
            } இல்லையெனில் எனில் (tok == VP_குறியீடு || tok == VP_STRING || tok == VP_TEXT || tok == VP_WHITE) {
                string rtn;
                rtn.assign(yyourtext(), yyourleng());
                refp->nextarg(refp->nextarg() + rtn);
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_STRIFY) {
                // We must expand stringification, when done will return to this state
                state_தள்ளு(ps_STRIFY);
                செல் next_tok;
            } இல்லையெனில் {
                fileline()->v3error(
                    "Expecting ) or , to end argument list for define reference. Found: "s
                    + tokenName(tok));
                state_எடு();
                செல் next_tok;
            }
        }
        நிலை ps_INCNAME: {
            எனில் (tok == VP_STRING) {
                state_எடு();
                m_lastSym.assign(yyourtext(), yyourleng());
                UINFO(4, "Include " << m_lastSym);
                // Drop leading and trailing quotes.
                m_lastSym.erase(0, 1);
                m_lastSym.erase(m_lastSym.length() - 1, 1);
                include(m_lastSym);
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_TEXT && yyourleng() == 1 && yyourtext()[0] == '<') {
                // include <filename>
                stateChange(ps_INCNAME);  // Still
                m_lexp->தள்ளு_State_Inc_Filename();
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_DEFREF || tok == VP_STRIFY) {
                // Expand it, then state will come back here
                நிறுத்து;
            } இல்லையெனில் {
                state_எடு();
                fileline()->v3error("Expecting include filename. Found: "s + tokenName(tok));
                செல் next_tok;
            }
            // GCC 13.3.0 with -O0 throws a false 'this statement may fall through' without this:
            VL_UNREACHABLE;
        }
        நிலை ps_ERRORNAME: {
            எனில் (tok == VP_STRING) {
                எனில் (!m_off) {
                    m_lastSym.assign(yyourtext(), yyourleng());
                    fileline()->v3error(m_lastSym);
                }
                state_எடு();
                செல் next_tok;
            } இல்லையெனில் {
                fileline()->v3error("Expecting `error string. Found: "s + tokenName(tok));
                state_எடு();
                செல் next_tok;
            }
        }
        நிலை ps_JOIN: {
            எனில் (tok == VP_குறியீடு || tok == VP_TEXT || tok == VP_STRING || tok == VP_WHITE) {
                UASSERT(!m_join_அடுக்கு.empty(), "`` join stack empty, but in a ``");
                மாறிலி string lhs = m_join_அடுக்கு.உச்சி();
                m_join_அடுக்கு.எடு();
                UINFO(5, "`` LHS:" << lhs);
                மாறிலி string rhs(yyourtext(), yyourleng());
                UINFO(5, "`` RHS:" << rhs);
                மாறிலி string out = lhs + rhs;
                UINFO(5, "`` Out:" << out);
                unputString(out);
                state_எடு();
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_EOF || tok == VP_COMMENT) {
                // Other compilers just ignore this, so no warning
                // "Expecting symbol to terminate ``; whitespace etc cannot
                // follow ``. Found: "+tokenName(tok)+"\n"
                மாறிலி string lhs = m_join_அடுக்கு.உச்சி();
                m_join_அடுக்கு.எடு();
                unputString(lhs);
                state_எடு();
                செல் next_tok;
            } இல்லையெனில் {
                // `define, etc, fall through and expand.  Pop back here.
                நிறுத்து;
            }
        }
        நிலை ps_STRIFY: {
            எனில் (tok == VP_STRIFY) {
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
                state_எடு();
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_EOF) {
                fileline()->v3error("`\" not terminated at EOF");
                நிறுத்து;
            } இல்லையெனில் எனில் (tok == VP_BACKQUOTE) {
                m_strify += "\\\"";
                செல் next_tok;
            } இல்லையெனில் எனில் (tok == VP_DEFREF) {
                // Spec says to expand macros inside `"
                // Substitute it into the stream, then return here
                நிறுத்து;
            } இல்லையெனில் {
                // Append token to eventual string
                m_strify.append(yyourtext(), yyourleng());
                செல் next_tok;
            }
        }
        இயல்பு: v3fatalSrc("Bad case");
        }
        // Default is to do top level expansion of some tokens
        தேர்வு (tok) {
        நிலை VP_INCLUDE:
            எனில் (!m_off) {
                state_தள்ளு(ps_INCNAME);
            }  // Else incname looks like normal text, that will be ignored
            செல் next_tok;
        நிலை VP_UNDEF: state_தள்ளு(ps_DEFNAME_UNDEF); செல் next_tok;
        நிலை VP_DEFINE:
            // No m_off check here, as a `ifdef NEVER `define FOO(`endif)  should work
            state_தள்ளு(ps_DEFNAME_DEFINE);
            செல் next_tok;
        நிலை VP_IFDEF: state_தள்ளு(ps_DEFNAME_IFDEF); செல் next_tok;
        நிலை VP_IFNDEF: state_தள்ளு(ps_DEFNAME_IFNDEF); செல் next_tok;
        நிலை VP_ELSIF: state_தள்ளு(ps_DEFNAME_ELSIF); செல் next_tok;
        நிலை VP_ELSE:
            எனில் (m_ifdef_அடுக்கு.empty()) {
                fileline()->v3error("`else with no matching `if");
            } இல்லையெனில் {
                மாறிலி VPreIfEntry lastIf = m_ifdef_அடுக்கு.உச்சி();
                m_ifdef_அடுக்கு.எடு();
                மாறிலி தருக்க enable = !lastIf.everOn();
                UINFO(4, "Else " << (enable ? " ON" : " OFF"));
                m_ifdef_அடுக்கு.emplace(VPreIfEntry{enable, lastIf.everOn()});
                எனில் (!lastIf.on()) parsingOn();
                எனில் (!enable) parsingOff();
            }
            செல் next_tok;
        நிலை VP_ENDIF:
            UINFO(4, "Endif ");
            எனில் (m_ifdef_அடுக்கு.empty()) {
                fileline()->v3error("`endif with no matching `if");
            } இல்லையெனில் {
                மாறிலி VPreIfEntry lastIf = m_ifdef_அடுக்கு.உச்சி();
                m_ifdef_அடுக்கு.எடு();
                எனில் (!lastIf.on()) parsingOn();
                // parsingOn() really only enables parsing if
                // all ifdef's above this want it on
            }
            செல் next_tok;

        நிலை VP_DEFREF: {
            // m_off not right here, but inside substitution, to make this work:
            // `ifdef NEVER `DEFUN(`endif)
            மாறிலி string name(yyourtext() + 1, yyourleng() - 1);
            UINFO(4, "DefRef " << name);
            எனில் (m_defPutJoin) {
                m_defPutJoin = பொய்;
                unputString("``");
            }
            எனில் (m_defDepth++ > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
                fileline()->v3error("Recursive `define substitution: `" + name);
                செல் next_tok;
            }
            // Substitute
            எனில் (!defExists(name)) {  // Not found, return original string as-is
                m_defDepth = 0;
                UINFO(4, "Defref `" << name << " => not_defined");
                எனில் (m_off) {
                    செல் next_tok;
                } இல்லையெனில் {
                    // We want final text of `name, but that would cause
                    // recursion, so use a special character to get it through
                    unputDefrefString("`\032"s + name);
                    செல் next_tok;
                }
            } இல்லையெனில் {
                மாறிலி string params = defParams(name);
                எனில் (params == "0" || params == "") {  // Found, as simple substitution
                    string out;
                    எனில் (!m_off) {
                        VDefineRef tempref{name, ""};
                        out = defineSubst(&tempref);
                    }
                    // Similar code in parenthesized define (Search for END_OF_DEFARG)
                    // NOP: out = m_preprocp->defSubstitute(out);
                    எனில் (m_defRefs.empty() || state() == ps_STRIFY || state() == ps_JOIN) {
                        // Just output the substitution
                        எனில் (state() == ps_JOIN) {  // Handle {left}```FOO where `FOO might be empty
                            UASSERT(!m_join_அடுக்கு.empty(), "`` join stack empty, but in a ``");
                            மாறிலி string lhs = m_join_அடுக்கு.உச்சி();
                            m_join_அடுக்கு.எடு();
                            out.செருகு(0, lhs);
                            UINFO(5, "``-end-defref Out:" << out);
                            state_எடு();
                        }
                        எனில் (!m_off) {
                            unputDefrefString(out);
                        }
                        // Prevent problem when EMPTY="" in `ifdef NEVER `define `EMPTY
                        இல்லையெனில் எனில் (stateIsDefname()) {
                            unputDefrefString("__IF_OFF_IGNORED_DEFINE");
                        }
                    } இல்லையெனில் {
                        // Inside another define, and not under stringification or join.
                        // Can't subst now, or
                        // `define a x,y
                        // foo(`a,`b)  would break because a contains comma
                        VDefineRef* மாறிலி refp = &(m_defRefs.உச்சி());
                        refp->nextarg(refp->nextarg() + m_lexp->m_def_மதிப்பு + out);
                        m_lexp->m_def_மதிப்பு = "";
                    }
                    செல் next_tok;
                } இல்லையெனில் {  // Found, with parameters
                    UINFO(4, "Defref `" << name << " => parameterized");
                    // The CURRENT macro needs the paren saved, it's not a
                    // property of the child macro
                    எனில் (!m_defRefs.empty()) m_defRefs.உச்சி().parenLevel(m_lexp->m_parenLevel);
                    m_defRefs.emplace(VDefineRef{name, params});
                    state_தள்ளு(ps_DEFPAREN);
                    m_lexp->தள்ளு_State_Def_Arg(0);
                    செல் next_tok;
                }
            }
            v3fatalSrc("Bad case");  // FALLTHRU
            செல் next_tok;  // above fatal means unreachable, but fixes static analysis warning
        }
        நிலை VP_ERROR: {
            state_தள்ளு(ps_ERRORNAME);
            செல் next_tok;
        }
        நிலை VP_EOF:
            எனில் (!m_ifdef_அடுக்கு.empty()) fileline()->v3error("`ifdef not terminated at EOF");
            திரும்பு tok;
        நிலை VP_UNDEFINEALL:
            எனில் (!m_off) {
                UINFO(4, "Undefineall ");
                undefineall();
            }
            செல் next_tok;
        நிலை VP_STRIFY:
            // We must expand macros in the body of the stringification
            // Then, when done, form a final string to return
            // (it could be used as a include filename, for example, so need the string token)
            state_தள்ளு(ps_STRIFY);
            செல் next_tok;
        நிலை VP_குறியீடு:
        நிலை VP_STRING:
        நிலை VP_TEXT: {
            m_defDepth = 0;
            எனில் (!m_off) {
                திரும்பு tok;
            } இல்லையெனில் {
                செல் next_tok;
            }
        }
        நிலை VP_WHITE:  // Handled at top of loop
        நிலை VP_COMMENT:  // Handled at top of loop
        நிலை VP_DEFFORM:  // Handled by state=ps_DEFFORM;
        நிலை VP_DEFVALUE:  // Handled by state=ps_DEFVALUE;
        இயல்பு:  // LCOV_EXCL_LINE
            v3fatalSrc("Internal error: Unexpected token "s + tokenName(tok) + "\n");
            நிறுத்து;  // LCOV_EXCL_LINE
        }
        திரும்பு tok;
    }
}

முழு V3PreProcImp::getFinalToken(string& buf) {
    // Return the next user-visible token in the input stream.
    // Includes and such are handled here, and are never seen by the caller.
    எனில் (!m_finAhead) {
        m_finAhead = உண்மை;
        m_finToken = getStateToken();
        m_finBuf.assign(yyourtext(), yyourleng());
    }
    மாறிலி முழு tok = m_finToken;
    buf = m_finBuf;
    எனில் (பொய் && debug() >= 5) {
        மாறிலி string bufcln = V3PreLex::cleanDbgStrg(buf);
        மாறிலி string flcol = m_lexp->m_tokFilelinep->ascii_வரி_Col();
        UINFO(0, flcol << ": FIN:      " << tokenName(tok) << ": " << bufcln);
    }
    // Track `line
    மாறிலி எழுத்து* bufp = buf.c_str();
    போது (*bufp == '\n') bufp++;
    எனில் ((tok == VP_TEXT || tok == VP_வரி) && VString::startsWith(bufp, "`line ")) {
        முழு enter;
        m_finFilelinep->வரி_Directive(bufp, enter /*ref*/);
    } இல்லையெனில் {
        எனில் (m_finAtBol && !(tok == VP_TEXT && buf == "\n") && m_preprocp->வரி_Directives()) {
            எனில் (மாறிலி முழு outBehind
                = (m_lexp->m_tokFilelinep->lastLineno() - m_finFilelinep->lastLineno())) {
                எனில் (debug() >= 5) {
                    மாறிலி string flcol = m_lexp->m_tokFilelinep->ascii_வரி_Col();
                    UINFO(0, flcol << ": FIN: readjust, fin at " << m_finFilelinep->lastLineno()
                                   << "  request at " << m_lexp->m_tokFilelinep->lastLineno());
                }
                m_finFilelinep->filename(m_lexp->m_tokFilelinep->filename());
                m_finFilelinep->lineno(m_lexp->m_tokFilelinep->lastLineno());
                எனில் (outBehind > 0
                    && (outBehind <= static_cast<முழு>(V3PreProc::NEWLINES_VS_TICKLINE))) {
                    // Output stream is behind, send newlines to get back in sync
                    // (Most likely because we're completing a disabled `endif)
                    எனில் (m_preprocp->keepWhitespace()) {
                        buf = std::string(outBehind, '\n');  // () for char repeat
                        திரும்பு VP_TEXT;
                    }
                } இல்லையெனில் {
                    // Need to backup, use `line
                    buf = m_finFilelinep->வரி_Directive_Strg(0);
                    திரும்பு VP_வரி;
                }
            }
        }
        // Track newlines in prep for next token
        க்கு (மாறிலி எழுத்து& c : buf) {
            எனில் (c == '\n') {
                m_finAtBol = உண்மை;
                m_finFilelinep->linenoInc();
            } இல்லையெனில் {
                m_finAtBol = பொய்;
            }
        }
    }
    m_finAhead = பொய்;  // Consumed the token
    திரும்பு tok;
}

string V3PreProcImp::getline() {
    // Get a single line from the parse stream.  Buffer unreturned text until the newline.
    எனில் (isEof()) திரும்பு "";
    மாறிலி எழுத்து* rtnp;
    தருக்க gotEof = பொய்;
    போது (nullptr == (rtnp = std::strchr(m_வரி_Chars.c_str(), '\n')) && !gotEof) {
        m_tokenBuf.clear();
        மாறிலி முழு tok = getFinalToken(m_tokenBuf /*ref*/);
        எனில் (debug() >= 5) {
            மாறிலி string bufcln = V3PreLex::cleanDbgStrg(m_tokenBuf);
            மாறிலி string flcol = m_lexp->m_tokFilelinep->ascii_வரி_Col();
            UINFO(0, flcol << ": GETFETC:  " << tokenName(tok) << ": " << bufcln);
        }
        எனில் (tok == VP_EOF) {
            // Add a final newline, if the user forgot the final \n.
            எனில் (m_வரி_Chars != "" && m_வரி_Chars[m_வரி_Chars.length() - 1] != '\n') {
                m_வரி_Chars.append("\n");
            }
            gotEof = உண்மை;
        } இல்லையெனில் {
            m_வரி_Chars.append(m_tokenBuf);
        }
    }

    // Make new string with data up to the newline.
    மாறிலி முழு len = rtnp - m_வரி_Chars.c_str() + 1;
    string the_வரி(m_வரி_Chars, 0, len);
    m_வரி_Chars = m_வரி_Chars.erase(0, len);  // Remove returned characters
    எனில் (debug() >= 4) {
        மாறிலி string lncln = V3PreLex::cleanDbgStrg(the_வரி);
        மாறிலி string flcol = m_lexp->m_tokFilelinep->ascii_வரி_Col();
        UINFO(0, flcol << ": GETLINE:  " << lncln);
    }
    திரும்பு the_வரி;
}
