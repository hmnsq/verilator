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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3PARSEIMP_H_
#وصف_ڪريو VERILATOR_V3PARSEIMP_H_

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

تعداد V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_ترجيح };

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
    فراغ apply_منزل_Nodes(Ast_ڳنڍ_FTask* nodesp) ثابت {
        لكل (Ast_ڳنڍ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), ڳنڍ_FTask)) {
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
    فراغ apply_منزل_Nodes(AstTypedef* nodep) ثابت {
        إذا (m_local) nodep->isHideLocal(true);
        إذا (m_protected) nodep->isHideProtected(true);
        إذا (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    فراغ apply_منزل_Nodes(AstVar* nodesp) ثابت {
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
    File_سٽ* fl = nullptr;
    صحيح token = 0;  // Read token, aka tok
    VBaseOverride بنياد_Override;
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
        V3_Error_ڪوڊ::en errcodeen;
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
std::ostream& عامل<<(std::ostream& os, ثابت V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#وصف_ڪريو YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ثابت m_rootp;  // Root of the design
    VInFilter* ثابت m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ساكن V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_سٽ* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_سٽ* m_bison_آخري_Fileline = nullptr;  // Filename/linenumber of last token

    bool m_اندريون_Library = false;  // Currently reading a library vs. regular file
    bool m_اندريون_Lib_Map = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    صحيح m_lex_Kwd_اونهائي = 0;  // Inside a `begin_keywords
    صحيح m_lex_Kwd_آخري;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    صحيح m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_token_آخري_Bison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_ٽوڪن_Ahead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_عدد*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_سٽ> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    سائيز_t m_pp_بائيٽون = 0;  // Preprocessor->lex bytes transferred

    Ast_ڳنڍ* m_ٽيگ_Nodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_وقت_آخري_Unit;  // Last `timescale's unit
    VTimescale m_وقت_آخري_Prec;  // Last `timescale's precision

عام:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    صحيح yylexThis();
    ساكن bool optFuture(ثابت string& flag) { أرجع v3Global.opt.isFuture(flag); }

    فراغ ٽيگ_Nodep(Ast_ڳنڍ* nodep) { m_ٽيگ_Nodep = nodep; }
    Ast_ڳنڍ* ٽيگ_Nodep() ثابت { أرجع m_ٽيگ_Nodep; }
    فراغ lexTimescaleParse(File_سٽ* fl, ثابت حرف* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_سٽ* fl, bool unit_سيٽ, مزدوج unitVal, bool prec_سيٽ,
                               مزدوج precVal) VL_MT_DISABLED;
    VTimescale وقت_آخري_Unit() ثابت { أرجع m_وقت_آخري_Unit; }
    VTimescale وقت_آخري_Prec() ثابت { أرجع m_وقت_آخري_Prec; }

    فراغ lexFileline(File_سٽ* fl) { m_lexFileline = fl; }
    File_سٽ* lexFileline() ثابت { أرجع m_lexFileline; }
    std::string& lexString() { أرجع m_lexString; }
    ساكن فراغ lexErrorPreprocDirective(File_سٽ* fl, ثابت حرف* textp) VL_MT_DISABLED;
    ساكن string lex_Parse_ٽيگ(ثابت حرف* textp) VL_MT_DISABLED;
    ساكن مزدوج lexParseTimenum(ثابت حرف* متن) VL_MT_DISABLED;
    فراغ lexPpline(ثابت حرف* textp) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLint(File_سٽ* fl, ثابت حرف* textp, bool turnOff) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLintSave(ثابت File_سٽ* fl) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLintRestore(File_سٽ* fl) VL_MT_DISABLED;
    ساكن فراغ lexVerilatorCmtBad(File_سٽ* fl, ثابت حرف* textp) VL_MT_DISABLED;

    فراغ lex_ڌڪو_Keywords(صحيح state) {
        ++m_lex_Kwd_اونهائي;
        m_lex_Kwd_آخري = state;
    }
    bool lex_ڪڍو_Keywords() {
        إذا (m_lex_Kwd_اونهائي) {
            --m_lex_Kwd_اونهائي;
            أرجع true;
        }
        أرجع false;
    }
    صحيح lex_Kwd_آخري_State() ثابت { أرجع m_lex_Kwd_آخري; }
    ساكن ثابت حرف* tokenName(صحيح tok) VL_MT_DISABLED;
    ساكن bool isStrengthToken(صحيح tok) VL_MT_DISABLED;

    فراغ pp_ڌڪو_متن(ثابت string& متن) {
        m_ppBuffers.ڌڪو_پٺ(متن);
        m_pp_بائيٽون += متن.ڊيگهائي();
        إذا (lexFileline()->contentp()) lexFileline()->contentp()->ڌڪو_متن(متن);
    }
    سائيز_t pp_داخلا_منزل_Lex(حرف* buf, سائيز_t max_سائيز_2) VL_MT_DISABLED;

    ساكن V3ParseImp* parsep() { أرجع s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ثابت string& متن) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{متن};
        m_stringps.ڌڪو_پٺ(strp);
        أرجع strp;
    }
    string* newString(ثابت حرف* متن) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{متن};
        m_stringps.ڌڪو_پٺ(strp);
        أرجع strp;
    }
    string* newString(ثابت حرف* متن, سائيز_t ڊيگهائي) {
        string* ثابت strp = new string(متن, ڊيگهائي);  // Need () constructor
        m_stringps.ڌڪو_پٺ(strp);
        أرجع strp;
    }
    V3_عدد* new_عدد(File_سٽ* flp, ثابت حرف* متن) {
        V3_عدد* nump = new V3_عدد{flp, V3_عدد::Verilog_عدد_Literal{}, متن};
        m_numberps.ڌڪو_پٺ(nump);
        أرجع nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_ڳنڍ* new_بلاڪ(File_سٽ* fl, Ast_ڳنڍ* nodep) {
        إذا (nodep) أرجع nodep;
        أرجع new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_سٽ* bison_آخري_Fileline() ثابت { أرجع m_bison_آخري_Fileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ثابت { أرجع m_rootp; }
    bool اندريون_Lib_Map() ثابت { أرجع m_اندريون_Lib_Map; }
    bool اندريون_Library() ثابت { أرجع m_اندريون_Library; }
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
    سائيز_t flex_Pp_داخلا_منزل_Lex(حرف* buf, سائيز_t max_سائيز_2) { أرجع pp_داخلا_منزل_Lex(buf, max_سائيز_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_سٽ* /*fl*/) { أرجع parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lex_Kwd_آخري = stateVerilogRecent();
        m_وقت_آخري_Unit = v3Global.opt.وقت_Default_Unit();
        m_وقت_آخري_Prec = v3Global.opt.وقت_Default_Prec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    فراغ parserClear() VL_MT_DISABLED;
    فراغ lexUnputString(ثابت حرف* textp, سائيز_t ڊيگهائي) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    صحيح token_منزل_Bison() VL_MT_DISABLED;  // Pass token to bison

    فراغ parseFile(File_سٽ* fileline, ثابت string& modfilename, bool اندريون_Library, bool اندريون_Lib_Map,
                   ثابت string& libname, ثابت string& errmsg,
                   ثابت std::string& not_مليو_Name) VL_MT_DISABLED;
    فراغ dumpInputsFile() VL_MT_DISABLED;
    فراغ dump_ٽوڪن_Ahead(صحيح سٽ) VL_MT_DISABLED;
    ساكن فراغ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    فراغ import_If_اندريون_Std(File_سٽ* fileline, ثابت string& id, bool doImport);
    Ast_ڳنڍ_اظهار* makePropertyCase(File_سٽ* flp, Ast_ڳنڍ_اظهار* exprp, Ast_Case_شي* itemsp);

خاص:
    فراغ preprocDumps(std::ostream& os);
    فراغ lexFile(ثابت string& modname) VL_MT_DISABLED;
    فراغ yylexReadTok() VL_MT_DISABLED;
    فراغ tokenPull() VL_MT_DISABLED;
    فراغ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    صحيح tokenPipelineId(صحيح token) VL_MT_DISABLED;
    فراغ tokenPipelineSym() VL_MT_DISABLED;
    سائيز_t token_Pipe_جاچ_Id_Inst(سائيز_t اونهائي) VL_MT_DISABLED;
    سائيز_t token_Pipe_جاچ_Id_Type(سائيز_t اونهائي) VL_MT_DISABLED;
    سائيز_t token_Pipe_جاچ_Bracket(سائيز_t اونهائي) VL_MT_DISABLED;
    سائيز_t token_Pipe_جاچ_Param(سائيز_t اونهائي, bool forInst) VL_MT_DISABLED;
    سائيز_t token_Pipe_جاچ_Parens(سائيز_t اونهائي) VL_MT_DISABLED;
    سائيز_t token_Pipe_جاچ_Eq_New(سائيز_t اونهائي) VL_MT_DISABLED;
    bool token_Pipe_جاچ_Tagged_Follows_Primary(سائيز_t اونهائي) VL_MT_DISABLED;
    ثابت V3ParseBisonYYSType* tokenPeekp(سائيز_t اونهائي) VL_MT_DISABLED;
    فراغ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#جيڪڏهن_پورو  // Guard
