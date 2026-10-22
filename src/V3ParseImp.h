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

#اگر_تعریف_نشده VERILATOR_V3PARSEIMP_H_
#تعریف_کن VERILATOR_V3PARSEIMP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Global.h"
#ضمن "V3Parse.h"

#ضمن <algorithm>
#ضمن <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

تعداد V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_اولویت };

تعداد V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

تعداد V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_خالص };

//============================================================================
// Member qualifiers

بنية VMemberQualifiers final {
    اتحاد {
        uint32_t m_flags;
        بنية {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ساكن VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        أرجع q;
    }
    ساكن VMemberQualifiers combine(ثابت VMemberQualifiers& a, ثابت VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        أرجع q;
    }
    VRandAttr randAttr() ثابت {
        إذا (m_randc) أرجع VRandAttr::RAND_CYCLIC;
        إذا (m_rand) أرجع VRandAttr::RAND;
        أرجع VRandAttr::NONE;
    }
    فراغ apply_مقصد_Nodes(Ast_گره_FTask* nodesp) ثابت {
        لكل (Ast_گره_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), گره_FTask)) {
            إذا (m_local) nodep->isHideLocal(true);
            إذا (m_protected) nodep->isHideProtected(true);
            إذا (m_static) nodep->isStatic(true);
            إذا (m_virtual) nodep->isVirtual(true);
            إذا (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    فراغ apply_مقصد_Nodes(AstTypedef* nodep) ثابت {
        إذا (m_local) nodep->isHideLocal(true);
        إذا (m_protected) nodep->isHideProtected(true);
        إذا (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    فراغ apply_مقصد_Nodes(AstVar* nodesp) ثابت {
        لكل (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            إذا (m_rand) nodep->rand(VRandAttr::RAND);
            إذا (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            إذا (m_local) nodep->isHideLocal(true);
            إذا (m_protected) nodep->isHideProtected(true);
            إذا (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            إذا (m_const) nodep->isConst(true);
            إذا (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

بنية V3ParseBisonYYSType final {
    File_سطر* fl = nullptr;
    صحيح token = 0;  // Read token, aka tok
    VBaseOverride پایه_Override;
    bool flag = false;  // Passed up some rules
    اتحاد {
        V3_عدد* nump;
        std::string* strp;
        صحيح cint;
        مزدوج cdouble;
        bool cbool;
        VMemberQualifiers qualifiers;
        V3UniqState uniqstate;
        V3TaggedState taggedstate;
        V3ImportProperty iprop;
        VSigning::en signstate;
        V3_Error_کد::en errcodeen;
        VAttrType::en attrtypeen;
        VAssertType::en asserttypeen;
        VAssertDirectiveType::en assertdirectivetypeen;
        VFwdType::en fwdtype;
        VLifetime::en lifetime;
        VStrength::en strength;
        VJoinType::en joinType;

#ضمن "V3Ast__gen_yystype.h"
    };
};
std::ostream& عملگر<<(std::ostream& os, ثابت V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#تعریف_کن YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ثابت m_rootp;  // Root of the design
    VInFilter* ثابت m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ساكن V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_سطر* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_سطر* m_bison_آخرین_Fileline = nullptr;  // Filename/linenumber of last token

    bool m_درون_Library = false;  // Currently reading a library vs. regular file
    bool m_درون_Lib_Map = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    صحيح m_lex_Kwd_عمق = 0;  // Inside a `begin_keywords
    صحيح m_lex_Kwd_آخرین;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    صحيح m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_token_آخرین_Bison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_نشانهها_Ahead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_عدد*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_سطر> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    اندازه_t m_pp_بایتها = 0;  // Preprocessor->lex bytes transferred

    Ast_گره* m_برچسب_Nodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_زمان_آخرین_Unit;  // Last `timescale's unit
    VTimescale m_زمان_آخرین_Prec;  // Last `timescale's precision

عام:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    صحيح yylexThis();
    ساكن bool optFuture(ثابت string& flag) { أرجع v3Global.opt.isFuture(flag); }

    فراغ برچسب_Nodep(Ast_گره* nodep) { m_برچسب_Nodep = nodep; }
    Ast_گره* برچسب_Nodep() ثابت { أرجع m_برچسب_Nodep; }
    فراغ lexTimescaleParse(File_سطر* fl, ثابت حرف* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_سطر* fl, bool unit_مجموعه, مزدوج unitVal, bool prec_مجموعه,
                               مزدوج precVal) VL_MT_DISABLED;
    VTimescale زمان_آخرین_Unit() ثابت { أرجع m_زمان_آخرین_Unit; }
    VTimescale زمان_آخرین_Prec() ثابت { أرجع m_زمان_آخرین_Prec; }

    فراغ lexFileline(File_سطر* fl) { m_lexFileline = fl; }
    File_سطر* lexFileline() ثابت { أرجع m_lexFileline; }
    std::string& lexString() { أرجع m_lexString; }
    ساكن فراغ lexErrorPreprocDirective(File_سطر* fl, ثابت حرف* textp) VL_MT_DISABLED;
    ساكن string lex_Parse_برچسب(ثابت حرف* textp) VL_MT_DISABLED;
    ساكن مزدوج lexParseTimenum(ثابت حرف* متن) VL_MT_DISABLED;
    فراغ lexPpline(ثابت حرف* textp) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLint(File_سطر* fl, ثابت حرف* textp, bool turnOff) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLintSave(ثابت File_سطر* fl) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLintRestore(File_سطر* fl) VL_MT_DISABLED;
    ساكن فراغ lexVerilatorCmtBad(File_سطر* fl, ثابت حرف* textp) VL_MT_DISABLED;

    فراغ lex_افزودن_Keywords(صحيح state) {
        ++m_lex_Kwd_عمق;
        m_lex_Kwd_آخرین = state;
    }
    bool lex_برداشت_Keywords() {
        إذا (m_lex_Kwd_عمق) {
            --m_lex_Kwd_عمق;
            أرجع true;
        }
        أرجع false;
    }
    صحيح lex_Kwd_آخرین_State() ثابت { أرجع m_lex_Kwd_آخرین; }
    ساكن ثابت حرف* tokenName(صحيح tok) VL_MT_DISABLED;
    ساكن bool isStrengthToken(صحيح tok) VL_MT_DISABLED;

    فراغ pp_افزودن_متن(ثابت string& متن) {
        m_ppBuffers.افزودن_پشت(متن);
        m_pp_بایتها += متن.درازی();
        إذا (lexFileline()->contentp()) lexFileline()->contentp()->افزودن_متن(متن);
    }
    اندازه_t pp_درون_داد_مقصد_Lex(حرف* buf, اندازه_t max_اندازه_2) VL_MT_DISABLED;

    ساكن V3ParseImp* parsep() { أرجع s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ثابت string& متن) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{متن};
        m_stringps.افزودن_پشت(strp);
        أرجع strp;
    }
    string* newString(ثابت حرف* متن) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{متن};
        m_stringps.افزودن_پشت(strp);
        أرجع strp;
    }
    string* newString(ثابت حرف* متن, اندازه_t درازی) {
        string* ثابت strp = new string(متن, درازی);  // Need () constructor
        m_stringps.افزودن_پشت(strp);
        أرجع strp;
    }
    V3_عدد* new_عدد(File_سطر* flp, ثابت حرف* متن) {
        V3_عدد* nump = new V3_عدد{flp, V3_عدد::Verilog_عدد_Literal{}, متن};
        m_numberps.افزودن_پشت(nump);
        أرجع nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_گره* new_بلوک(File_سطر* fl, Ast_گره* nodep) {
        إذا (nodep) أرجع nodep;
        أرجع new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_سطر* bison_آخرین_Fileline() ثابت { أرجع m_bison_آخرین_Fileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ثابت { أرجع m_rootp; }
    bool درون_Lib_Map() ثابت { أرجع m_درون_Lib_Map; }
    bool درون_Library() ثابت { أرجع m_درون_Library; }
    string libname() ثابت { أرجع m_libname; }
    VOptionBool unconnectedDrive() ثابت { أرجع m_unconnectedDrive; }
    فراغ unconnectedDrive(ثابت VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    صحيح bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    فراغ lexNew() VL_MT_DISABLED;
    فراغ lexDestroy() VL_MT_DISABLED;
    ساكن صحيح stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    صحيح lexPrevToken() ثابت { أرجع m_lexPrevToken; }  // Parser -> lexer communication
    اندازه_t flex_Pp_درون_داد_مقصد_Lex(حرف* buf, اندازه_t max_اندازه_2) { أرجع pp_درون_داد_مقصد_Lex(buf, max_اندازه_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_سطر* /*fl*/) { أرجع parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lex_Kwd_آخرین = stateVerilogRecent();
        m_زمان_آخرین_Unit = v3Global.opt.زمان_Default_Unit();
        m_زمان_آخرین_Prec = v3Global.opt.زمان_Default_Prec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    فراغ parserClear() VL_MT_DISABLED;
    فراغ lexUnputString(ثابت حرف* textp, اندازه_t درازی) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    صحيح token_مقصد_Bison() VL_MT_DISABLED;  // Pass token to bison

    فراغ parseFile(File_سطر* fileline, ثابت string& modfilename, bool درون_Library, bool درون_Lib_Map,
                   ثابت string& libname, ثابت string& errmsg,
                   ثابت std::string& not_یافت_Name) VL_MT_DISABLED;
    فراغ dumpInputsFile() VL_MT_DISABLED;
    فراغ dump_نشانهها_Ahead(صحيح سطر) VL_MT_DISABLED;
    ساكن فراغ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    فراغ import_If_درون_Std(File_سطر* fileline, ثابت string& id, bool doImport);
    Ast_گره_عبارت* makePropertyCase(File_سطر* flp, Ast_گره_عبارت* exprp, Ast_Case_قلم* itemsp);

خاص:
    فراغ preprocDumps(std::ostream& os);
    فراغ lexFile(ثابت string& modname) VL_MT_DISABLED;
    فراغ yylexReadTok() VL_MT_DISABLED;
    فراغ tokenPull() VL_MT_DISABLED;
    فراغ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    صحيح tokenPipelineId(صحيح token) VL_MT_DISABLED;
    فراغ tokenPipelineSym() VL_MT_DISABLED;
    اندازه_t token_Pipe_پیمایش_Id_Inst(اندازه_t عمق) VL_MT_DISABLED;
    اندازه_t token_Pipe_پیمایش_Id_Type(اندازه_t عمق) VL_MT_DISABLED;
    اندازه_t token_Pipe_پیمایش_Bracket(اندازه_t عمق) VL_MT_DISABLED;
    اندازه_t token_Pipe_پیمایش_Param(اندازه_t عمق, bool forInst) VL_MT_DISABLED;
    اندازه_t token_Pipe_پیمایش_Parens(اندازه_t عمق) VL_MT_DISABLED;
    اندازه_t token_Pipe_پیمایش_Eq_New(اندازه_t عمق) VL_MT_DISABLED;
    bool token_Pipe_پیمایش_Tagged_Follows_Primary(اندازه_t عمق) VL_MT_DISABLED;
    ثابت V3ParseBisonYYSType* tokenPeekp(اندازه_t عمق) VL_MT_DISABLED;
    فراغ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#پایان_اگر  // Guard
