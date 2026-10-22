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

#اگر_غیر_تعریف_شدہ VERILATOR_V3PARSEIMP_H_
#تعریف_کرو VERILATOR_V3PARSEIMP_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Global.h"
#شامل "V3Parse.h"

#شامل <algorithm>
#شامل <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

شمار V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_ترجیح };

شمار V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

شمار V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_خالص };

//============================================================================
// Member qualifiers

ساخت VMemberQualifiers final {
    اتحاد {
        uint32_t m_flags;
        ساخت {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ساکن VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        واپس q;
    }
    ساکن VMemberQualifiers combine(ثابت VMemberQualifiers& a, ثابت VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        واپس q;
    }
    VRandAttr randAttr() ثابت {
        اگر (m_randc) واپس VRandAttr::RAND_CYCLIC;
        اگر (m_rand) واپس VRandAttr::RAND;
        واپس VRandAttr::NONE;
    }
    خالی apply_مقصد_Nodes(Ast_گرہ_FTask* nodesp) ثابت {
        برائے (Ast_گرہ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), گرہ_FTask)) {
            اگر (m_local) nodep->isHideLocal(true);
            اگر (m_protected) nodep->isHideProtected(true);
            اگر (m_static) nodep->isStatic(true);
            اگر (m_virtual) nodep->isVirtual(true);
            اگر (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    خالی apply_مقصد_Nodes(AstTypedef* nodep) ثابت {
        اگر (m_local) nodep->isHideLocal(true);
        اگر (m_protected) nodep->isHideProtected(true);
        اگر (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    خالی apply_مقصد_Nodes(AstVar* nodesp) ثابت {
        برائے (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            اگر (m_rand) nodep->rand(VRandAttr::RAND);
            اگر (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            اگر (m_local) nodep->isHideLocal(true);
            اگر (m_protected) nodep->isHideProtected(true);
            اگر (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            اگر (m_const) nodep->isConst(true);
            اگر (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

ساخت V3ParseBisonYYSType final {
    File_سطر* fl = nullptr;
    صحیح token = 0;  // Read token, aka tok
    VBaseOverride بنیاد_Override;
    bool flag = false;  // Passed up some rules
    اتحاد {
        V3_نمبر* nump;
        std::string* strp;
        صحیح cint;
        دوہرا cdouble;
        bool cbool;
        VMemberQualifiers qualifiers;
        V3UniqState uniqstate;
        V3TaggedState taggedstate;
        V3ImportProperty iprop;
        VSigning::en signstate;
        V3_Error_رمز::en errcodeen;
        VAttrType::en attrtypeen;
        VAssertType::en asserttypeen;
        VAssertDirectiveType::en assertdirectivetypeen;
        VFwdType::en fwdtype;
        VLifetime::en lifetime;
        VStrength::en strength;
        VJoinType::en joinType;

#شامل "V3Ast__gen_yystype.h"
    };
};
std::ostream& عامل<<(std::ostream& os, ثابت V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#تعریف_کرو YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ثابت m_rootp;  // Root of the design
    VInFilter* ثابت m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ساکن V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_سطر* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_سطر* m_bison_آخری_Fileline = nullptr;  // Filename/linenumber of last token

    bool m_اندر_Library = false;  // Currently reading a library vs. regular file
    bool m_اندر_Lib_Map = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    صحیح m_lex_Kwd_گہرائی = 0;  // Inside a `begin_keywords
    صحیح m_lex_Kwd_آخری;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    صحیح m_lex_پچھلا_Token = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_token_آخری_Bison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_علامات_Ahead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_نمبر*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_سطر> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    سائز_t m_pp_بائٹیں = 0;  // Preprocessor->lex bytes transferred

    Ast_گرہ* m_لیبل_Nodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_وقت_آخری_Unit;  // Last `timescale's unit
    VTimescale m_وقت_آخری_Prec;  // Last `timescale's precision

عوامی:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    صحیح yylexThis();
    ساکن bool optFuture(ثابت string& flag) { واپس v3Global.opt.isFuture(flag); }

    خالی لیبل_Nodep(Ast_گرہ* nodep) { m_لیبل_Nodep = nodep; }
    Ast_گرہ* لیبل_Nodep() ثابت { واپس m_لیبل_Nodep; }
    خالی lexTimescaleParse(File_سطر* fl, ثابت حرف* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_سطر* fl, bool unit_سیٹ, دوہرا unitVal, bool prec_سیٹ,
                               دوہرا precVal) VL_MT_DISABLED;
    VTimescale وقت_آخری_Unit() ثابت { واپس m_وقت_آخری_Unit; }
    VTimescale وقت_آخری_Prec() ثابت { واپس m_وقت_آخری_Prec; }

    خالی lexFileline(File_سطر* fl) { m_lexFileline = fl; }
    File_سطر* lexFileline() ثابت { واپس m_lexFileline; }
    std::string& lexString() { واپس m_lexString; }
    ساکن خالی lexErrorPreprocDirective(File_سطر* fl, ثابت حرف* textp) VL_MT_DISABLED;
    ساکن string lex_Parse_لیبل(ثابت حرف* textp) VL_MT_DISABLED;
    ساکن دوہرا lexParseTimenum(ثابت حرف* متن) VL_MT_DISABLED;
    خالی lexPpline(ثابت حرف* textp) VL_MT_DISABLED;
    خالی lexVerilatorCmtLint(File_سطر* fl, ثابت حرف* textp, bool turnOff) VL_MT_DISABLED;
    خالی lexVerilatorCmtLintSave(ثابت File_سطر* fl) VL_MT_DISABLED;
    خالی lexVerilatorCmtLintRestore(File_سطر* fl) VL_MT_DISABLED;
    ساکن خالی lexVerilatorCmtBad(File_سطر* fl, ثابت حرف* textp) VL_MT_DISABLED;

    خالی lex_ڈالو_Keywords(صحیح state) {
        ++m_lex_Kwd_گہرائی;
        m_lex_Kwd_آخری = state;
    }
    bool lex_نکالو_Keywords() {
        اگر (m_lex_Kwd_گہرائی) {
            --m_lex_Kwd_گہرائی;
            واپس true;
        }
        واپس false;
    }
    صحیح lex_Kwd_آخری_State() ثابت { واپس m_lex_Kwd_آخری; }
    ساکن ثابت حرف* tokenName(صحیح tok) VL_MT_DISABLED;
    ساکن bool isStrengthToken(صحیح tok) VL_MT_DISABLED;

    خالی pp_ڈالو_متن(ثابت string& متن) {
        m_ppBuffers.ڈالو_عقب(متن);
        m_pp_بائٹیں += متن.لمبائی_2();
        اگر (lexFileline()->contentp()) lexFileline()->contentp()->ڈالو_متن(متن);
    }
    سائز_t pp_داخلہ_مقصد_Lex(حرف* buf, سائز_t max_سائز_2) VL_MT_DISABLED;

    ساکن V3ParseImp* parsep() { واپس s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ثابت string& متن) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{متن};
        m_stringps.ڈالو_عقب(strp);
        واپس strp;
    }
    string* newString(ثابت حرف* متن) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{متن};
        m_stringps.ڈالو_عقب(strp);
        واپس strp;
    }
    string* newString(ثابت حرف* متن, سائز_t لمبائی_2) {
        string* ثابت strp = new string(متن, لمبائی_2);  // Need () constructor
        m_stringps.ڈالو_عقب(strp);
        واپس strp;
    }
    V3_نمبر* new_نمبر(File_سطر* flp, ثابت حرف* متن) {
        V3_نمبر* nump = new V3_نمبر{flp, V3_نمبر::Verilog_نمبر_Literal{}, متن};
        m_numberps.ڈالو_عقب(nump);
        واپس nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_گرہ* new_بلاک(File_سطر* fl, Ast_گرہ* nodep) {
        اگر (nodep) واپس nodep;
        واپس new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_سطر* bison_آخری_Fileline() ثابت { واپس m_bison_آخری_Fileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ثابت { واپس m_rootp; }
    bool اندر_Lib_Map() ثابت { واپس m_اندر_Lib_Map; }
    bool اندر_Library() ثابت { واپس m_اندر_Library; }
    string libname() ثابت { واپس m_libname; }
    VOptionBool unconnectedDrive() ثابت { واپس m_unconnectedDrive; }
    خالی unconnectedDrive(ثابت VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    صحیح bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    خالی lexNew() VL_MT_DISABLED;
    خالی lexDestroy() VL_MT_DISABLED;
    ساکن صحیح stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    صحیح lex_پچھلا_Token() ثابت { واپس m_lex_پچھلا_Token; }  // Parser -> lexer communication
    سائز_t flex_Pp_داخلہ_مقصد_Lex(حرف* buf, سائز_t max_سائز_2) { واپس pp_داخلہ_مقصد_Lex(buf, max_سائز_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_سطر* /*fl*/) { واپس parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lex_Kwd_آخری = stateVerilogRecent();
        m_وقت_آخری_Unit = v3Global.opt.وقت_Default_Unit();
        m_وقت_آخری_Prec = v3Global.opt.وقت_Default_Prec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    خالی parserClear() VL_MT_DISABLED;
    خالی lexUnputString(ثابت حرف* textp, سائز_t لمبائی_2) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    صحیح token_مقصد_Bison() VL_MT_DISABLED;  // Pass token to bison

    خالی parseFile(File_سطر* fileline, ثابت string& modfilename, bool اندر_Library, bool اندر_Lib_Map,
                   ثابت string& libname, ثابت string& errmsg,
                   ثابت std::string& not_ملا_Name) VL_MT_DISABLED;
    خالی dumpInputsFile() VL_MT_DISABLED;
    خالی dump_علامات_Ahead(صحیح سطر) VL_MT_DISABLED;
    ساکن خالی candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    خالی import_If_اندر_Std(File_سطر* fileline, ثابت string& id, bool doImport);
    Ast_گرہ_اظہاریہ* makePropertyCase(File_سطر* flp, Ast_گرہ_اظہاریہ* exprp, Ast_Case_شے* itemsp);

نجی:
    خالی preprocDumps(std::ostream& os);
    خالی lexFile(ثابت string& modname) VL_MT_DISABLED;
    خالی yylexReadTok() VL_MT_DISABLED;
    خالی tokenPull() VL_MT_DISABLED;
    خالی tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    صحیح tokenPipelineId(صحیح token) VL_MT_DISABLED;
    خالی tokenPipelineSym() VL_MT_DISABLED;
    سائز_t token_Pipe_جائزہ_Id_Inst(سائز_t گہرائی) VL_MT_DISABLED;
    سائز_t token_Pipe_جائزہ_Id_Type(سائز_t گہرائی) VL_MT_DISABLED;
    سائز_t token_Pipe_جائزہ_Bracket(سائز_t گہرائی) VL_MT_DISABLED;
    سائز_t token_Pipe_جائزہ_Param(سائز_t گہرائی, bool forInst) VL_MT_DISABLED;
    سائز_t token_Pipe_جائزہ_Parens(سائز_t گہرائی) VL_MT_DISABLED;
    سائز_t token_Pipe_جائزہ_Eq_New(سائز_t گہرائی) VL_MT_DISABLED;
    bool token_Pipe_جائزہ_Tagged_Follows_Primary(سائز_t گہرائی) VL_MT_DISABLED;
    ثابت V3ParseBisonYYSType* tokenPeekp(سائز_t گہرائی) VL_MT_DISABLED;
    خالی preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
