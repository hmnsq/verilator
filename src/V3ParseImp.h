// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Common header between parser and lex
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2009-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3PARSEIMP_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3PARSEIMP_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Parse.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

ਗਿਣਤੀ V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_ਤਰਜੀਹ };

ਗਿਣਤੀ V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

ਗਿਣਤੀ V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_ਸ਼ੁੱਧ };

//============================================================================
// Member qualifiers

ਬਣਤਰ VMemberQualifiers final {
    ਸੰਘ {
        uint32_t m_flags;
        ਬਣਤਰ {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ਸਥਿਰ VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        ਵਾਪਸ q;
    }
    ਸਥਿਰ VMemberQualifiers combine(ਅਚਲ VMemberQualifiers& a, ਅਚਲ VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        ਵਾਪਸ q;
    }
    VRandAttr randAttr() ਅਚਲ {
        ਜੇ (m_randc) ਵਾਪਸ VRandAttr::RAND_CYCLIC;
        ਜੇ (m_rand) ਵਾਪਸ VRandAttr::RAND;
        ਵਾਪਸ VRandAttr::NONE;
    }
    ਖਾਲੀ apply_ਮੰਜ਼ਿਲ_Nodes(Ast_ਗੰਢ_FTask* nodesp) ਅਚਲ {
        ਲਈ (Ast_ਗੰਢ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), ਗੰਢ_FTask)) {
            ਜੇ (m_local) nodep->isHideLocal(true);
            ਜੇ (m_protected) nodep->isHideProtected(true);
            ਜੇ (m_static) nodep->isStatic(true);
            ਜੇ (m_virtual) nodep->isVirtual(true);
            ਜੇ (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    ਖਾਲੀ apply_ਮੰਜ਼ਿਲ_Nodes(AstTypedef* nodep) ਅਚਲ {
        ਜੇ (m_local) nodep->isHideLocal(true);
        ਜੇ (m_protected) nodep->isHideProtected(true);
        ਜੇ (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    ਖਾਲੀ apply_ਮੰਜ਼ਿਲ_Nodes(AstVar* nodesp) ਅਚਲ {
        ਲਈ (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            ਜੇ (m_rand) nodep->rand(VRandAttr::RAND);
            ਜੇ (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            ਜੇ (m_local) nodep->isHideLocal(true);
            ਜੇ (m_protected) nodep->isHideProtected(true);
            ਜੇ (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            ਜੇ (m_const) nodep->isConst(true);
            ਜੇ (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

ਬਣਤਰ V3ParseBisonYYSType final {
    File_ਸਤਰ* fl = nullptr;
    ਪੂਰਨ token = 0;  // Read token, aka tok
    VBaseOverride ਅਧਾਰ_Override;
    bool flag = false;  // Passed up some rules
    ਸੰਘ {
        V3_ਸੰਖਿਆ* nump;
        std::string* strp;
        ਪੂਰਨ cint;
        ਦੁੱਗਣਾ cdouble;
        bool cbool;
        VMemberQualifiers qualifiers;
        V3UniqState uniqstate;
        V3TaggedState taggedstate;
        V3ImportProperty iprop;
        VSigning::en signstate;
        V3_Error_ਕੋਡ::en errcodeen;
        VAttrType::en attrtypeen;
        VAssertType::en asserttypeen;
        VAssertDirectiveType::en assertdirectivetypeen;
        VFwdType::en fwdtype;
        VLifetime::en lifetime;
        VStrength::en strength;
        VJoinType::en joinType;

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_yystype.h"
    };
};
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ਅਚਲ m_rootp;  // Root of the design
    VInFilter* ਅਚਲ m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ਸਥਿਰ V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_ਸਤਰ* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_ਸਤਰ* m_bison_ਆਖਰੀ_Fileline = nullptr;  // Filename/linenumber of last token

    bool m_ਵਿੱਚ_Library = false;  // Currently reading a library vs. regular file
    bool m_ਵਿੱਚ_Lib_Map = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    ਪੂਰਨ m_lex_Kwd_ਡੂੰਘਾਈ = 0;  // Inside a `begin_keywords
    ਪੂਰਨ m_lex_Kwd_ਆਖਰੀ;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    ਪੂਰਨ m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_token_ਆਖਰੀ_Bison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_ਟੋਕਨ_Ahead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_ਸੰਖਿਆ*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_ਸਤਰ> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    ਮਾਪ_t m_pp_ਬਾਈਟਾਂ = 0;  // Preprocessor->lex bytes transferred

    Ast_ਗੰਢ* m_ਟੈਗ_Nodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_ਸਮਾਂ_ਆਖਰੀ_Unit;  // Last `timescale's unit
    VTimescale m_ਸਮਾਂ_ਆਖਰੀ_Prec;  // Last `timescale's precision

ਜਨਤਕ:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    ਪੂਰਨ yylexThis();
    ਸਥਿਰ bool optFuture(ਅਚਲ string& flag) { ਵਾਪਸ v3Global.opt.isFuture(flag); }

    ਖਾਲੀ ਟੈਗ_Nodep(Ast_ਗੰਢ* nodep) { m_ਟੈਗ_Nodep = nodep; }
    Ast_ਗੰਢ* ਟੈਗ_Nodep() ਅਚਲ { ਵਾਪਸ m_ਟੈਗ_Nodep; }
    ਖਾਲੀ lexTimescaleParse(File_ਸਤਰ* fl, ਅਚਲ ਅੱਖਰ* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_ਸਤਰ* fl, bool unit_ਸੈੱਟ, ਦੁੱਗਣਾ unitVal, bool prec_ਸੈੱਟ,
                               ਦੁੱਗਣਾ precVal) VL_MT_DISABLED;
    VTimescale ਸਮਾਂ_ਆਖਰੀ_Unit() ਅਚਲ { ਵਾਪਸ m_ਸਮਾਂ_ਆਖਰੀ_Unit; }
    VTimescale ਸਮਾਂ_ਆਖਰੀ_Prec() ਅਚਲ { ਵਾਪਸ m_ਸਮਾਂ_ਆਖਰੀ_Prec; }

    ਖਾਲੀ lexFileline(File_ਸਤਰ* fl) { m_lexFileline = fl; }
    File_ਸਤਰ* lexFileline() ਅਚਲ { ਵਾਪਸ m_lexFileline; }
    std::string& lexString() { ਵਾਪਸ m_lexString; }
    ਸਥਿਰ ਖਾਲੀ lexErrorPreprocDirective(File_ਸਤਰ* fl, ਅਚਲ ਅੱਖਰ* textp) VL_MT_DISABLED;
    ਸਥਿਰ string lex_Parse_ਟੈਗ(ਅਚਲ ਅੱਖਰ* textp) VL_MT_DISABLED;
    ਸਥਿਰ ਦੁੱਗਣਾ lexParseTimenum(ਅਚਲ ਅੱਖਰ* ਪਾਠ) VL_MT_DISABLED;
    ਖਾਲੀ lexPpline(ਅਚਲ ਅੱਖਰ* textp) VL_MT_DISABLED;
    ਖਾਲੀ lexVerilatorCmtLint(File_ਸਤਰ* fl, ਅਚਲ ਅੱਖਰ* textp, bool turnOff) VL_MT_DISABLED;
    ਖਾਲੀ lexVerilatorCmtLintSave(ਅਚਲ File_ਸਤਰ* fl) VL_MT_DISABLED;
    ਖਾਲੀ lexVerilatorCmtLintRestore(File_ਸਤਰ* fl) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ lexVerilatorCmtBad(File_ਸਤਰ* fl, ਅਚਲ ਅੱਖਰ* textp) VL_MT_DISABLED;

    ਖਾਲੀ lex_ਧੱਕੋ_Keywords(ਪੂਰਨ state) {
        ++m_lex_Kwd_ਡੂੰਘਾਈ;
        m_lex_Kwd_ਆਖਰੀ = state;
    }
    bool lex_ਕੱਢੋ_Keywords() {
        ਜੇ (m_lex_Kwd_ਡੂੰਘਾਈ) {
            --m_lex_Kwd_ਡੂੰਘਾਈ;
            ਵਾਪਸ true;
        }
        ਵਾਪਸ false;
    }
    ਪੂਰਨ lex_Kwd_ਆਖਰੀ_State() ਅਚਲ { ਵਾਪਸ m_lex_Kwd_ਆਖਰੀ; }
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* tokenName(ਪੂਰਨ tok) VL_MT_DISABLED;
    ਸਥਿਰ bool isStrengthToken(ਪੂਰਨ tok) VL_MT_DISABLED;

    ਖਾਲੀ pp_ਧੱਕੋ_ਪਾਠ(ਅਚਲ string& ਪਾਠ) {
        m_ppBuffers.ਧੱਕੋ_ਪਿੱਠ(ਪਾਠ);
        m_pp_ਬਾਈਟਾਂ += ਪਾਠ.ਲੰਮਾਈ();
        ਜੇ (lexFileline()->contentp()) lexFileline()->contentp()->ਧੱਕੋ_ਪਾਠ(ਪਾਠ);
    }
    ਮਾਪ_t pp_ਨਿਵੇਸ਼_ਮੰਜ਼ਿਲ_Lex(ਅੱਖਰ* buf, ਮਾਪ_t max_ਮਾਪ_2) VL_MT_DISABLED;

    ਸਥਿਰ V3ParseImp* parsep() { ਵਾਪਸ s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ਅਚਲ string& ਪਾਠ) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ਅਚਲ strp = new std::string{ਪਾਠ};
        m_stringps.ਧੱਕੋ_ਪਿੱਠ(strp);
        ਵਾਪਸ strp;
    }
    string* newString(ਅਚਲ ਅੱਖਰ* ਪਾਠ) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ਅਚਲ strp = new std::string{ਪਾਠ};
        m_stringps.ਧੱਕੋ_ਪਿੱਠ(strp);
        ਵਾਪਸ strp;
    }
    string* newString(ਅਚਲ ਅੱਖਰ* ਪਾਠ, ਮਾਪ_t ਲੰਮਾਈ) {
        string* ਅਚਲ strp = new string(ਪਾਠ, ਲੰਮਾਈ);  // Need () constructor
        m_stringps.ਧੱਕੋ_ਪਿੱਠ(strp);
        ਵਾਪਸ strp;
    }
    V3_ਸੰਖਿਆ* new_ਸੰਖਿਆ(File_ਸਤਰ* flp, ਅਚਲ ਅੱਖਰ* ਪਾਠ) {
        V3_ਸੰਖਿਆ* nump = new V3_ਸੰਖਿਆ{flp, V3_ਸੰਖਿਆ::Verilog_ਸੰਖਿਆ_Literal{}, ਪਾਠ};
        m_numberps.ਧੱਕੋ_ਪਿੱਠ(nump);
        ਵਾਪਸ nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_ਗੰਢ* new_ਖੰਡ(File_ਸਤਰ* fl, Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep) ਵਾਪਸ nodep;
        ਵਾਪਸ new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_ਸਤਰ* bison_ਆਖਰੀ_Fileline() ਅਚਲ { ਵਾਪਸ m_bison_ਆਖਰੀ_Fileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ਅਚਲ { ਵਾਪਸ m_rootp; }
    bool ਵਿੱਚ_Lib_Map() ਅਚਲ { ਵਾਪਸ m_ਵਿੱਚ_Lib_Map; }
    bool ਵਿੱਚ_Library() ਅਚਲ { ਵਾਪਸ m_ਵਿੱਚ_Library; }
    string libname() ਅਚਲ { ਵਾਪਸ m_libname; }
    VOptionBool unconnectedDrive() ਅਚਲ { ਵਾਪਸ m_unconnectedDrive; }
    ਖਾਲੀ unconnectedDrive(ਅਚਲ VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    ਪੂਰਨ bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    ਖਾਲੀ lexNew() VL_MT_DISABLED;
    ਖਾਲੀ lexDestroy() VL_MT_DISABLED;
    ਸਥਿਰ ਪੂਰਨ stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    ਪੂਰਨ lexPrevToken() ਅਚਲ { ਵਾਪਸ m_lexPrevToken; }  // Parser -> lexer communication
    ਮਾਪ_t flex_Pp_ਨਿਵੇਸ਼_ਮੰਜ਼ਿਲ_Lex(ਅੱਖਰ* buf, ਮਾਪ_t max_ਮਾਪ_2) { ਵਾਪਸ pp_ਨਿਵੇਸ਼_ਮੰਜ਼ਿਲ_Lex(buf, max_ਮਾਪ_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_ਸਤਰ* /*fl*/) { ਵਾਪਸ parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lex_Kwd_ਆਖਰੀ = stateVerilogRecent();
        m_ਸਮਾਂ_ਆਖਰੀ_Unit = v3Global.opt.ਸਮਾਂ_Default_Unit();
        m_ਸਮਾਂ_ਆਖਰੀ_Prec = v3Global.opt.ਸਮਾਂ_Default_Prec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    ਖਾਲੀ parserClear() VL_MT_DISABLED;
    ਖਾਲੀ lexUnputString(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਲੰਮਾਈ) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ਪੂਰਨ token_ਮੰਜ਼ਿਲ_Bison() VL_MT_DISABLED;  // Pass token to bison

    ਖਾਲੀ parseFile(File_ਸਤਰ* fileline, ਅਚਲ string& modfilename, bool ਵਿੱਚ_Library, bool ਵਿੱਚ_Lib_Map,
                   ਅਚਲ string& libname, ਅਚਲ string& errmsg,
                   ਅਚਲ std::string& not_ਮਿਲਿਆ_Name) VL_MT_DISABLED;
    ਖਾਲੀ dumpInputsFile() VL_MT_DISABLED;
    ਖਾਲੀ dump_ਟੋਕਨ_Ahead(ਪੂਰਨ ਸਤਰ) VL_MT_DISABLED;
    ਸਥਿਰ ਖਾਲੀ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    ਖਾਲੀ import_If_ਵਿੱਚ_Std(File_ਸਤਰ* fileline, ਅਚਲ string& id, bool doImport);
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* makePropertyCase(File_ਸਤਰ* flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Case_ਵਸਤੂ* itemsp);

ਨਿਜੀ:
    ਖਾਲੀ preprocDumps(std::ostream& os);
    ਖਾਲੀ lexFile(ਅਚਲ string& modname) VL_MT_DISABLED;
    ਖਾਲੀ yylexReadTok() VL_MT_DISABLED;
    ਖਾਲੀ tokenPull() VL_MT_DISABLED;
    ਖਾਲੀ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    ਪੂਰਨ tokenPipelineId(ਪੂਰਨ token) VL_MT_DISABLED;
    ਖਾਲੀ tokenPipelineSym() VL_MT_DISABLED;
    ਮਾਪ_t token_Pipe_ਜਾਂਚ_Id_Inst(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    ਮਾਪ_t token_Pipe_ਜਾਂਚ_Id_Type(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    ਮਾਪ_t token_Pipe_ਜਾਂਚ_Bracket(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    ਮਾਪ_t token_Pipe_ਜਾਂਚ_Param(ਮਾਪ_t ਡੂੰਘਾਈ, bool forInst) VL_MT_DISABLED;
    ਮਾਪ_t token_Pipe_ਜਾਂਚ_Parens(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    ਮਾਪ_t token_Pipe_ਜਾਂਚ_Eq_New(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    bool token_Pipe_ਜਾਂਚ_Tagged_Follows_Primary(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    ਅਚਲ V3ParseBisonYYSType* tokenPeekp(ਮਾਪ_t ਡੂੰਘਾਈ) VL_MT_DISABLED;
    ਖਾਲੀ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
