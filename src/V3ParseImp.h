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

#ifndef VERILATOR_V3PARSEIMP_H_
#define VERILATOR_V3PARSEIMP_H_

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

شمارش V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

شمارش V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

شمارش V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

ساختار VMemberQualifiers final {
    اجتماع {
        uint32_t m_flags;
        ساختار {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ایستا VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        بازگشت q;
    }
    ایستا VMemberQualifiers combine(ثابت VMemberQualifiers& a, ثابت VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        بازگشت q;
    }
    VRandAttr randAttr() ثابت {
        اگر (m_randc) بازگشت VRandAttr::RAND_CYCLIC;
        اگر (m_rand) بازگشت VRandAttr::RAND;
        بازگشت VRandAttr::NONE;
    }
    تهی applyToNodes(Ast_گره_FTask* nodesp) ثابت {
        برای (Ast_گره_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), گره_FTask)) {
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
    تهی applyToNodes(AstTypedef* nodep) ثابت {
        اگر (m_local) nodep->isHideLocal(true);
        اگر (m_protected) nodep->isHideProtected(true);
        اگر (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    تهی applyToNodes(AstVar* nodesp) ثابت {
        برای (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
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

ساختار V3ParseBisonYYSType final {
    File_سطر* fl = nullptr;
    صحیح token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    اجتماع {
        V3_عدد* nump;
        std::string* strp;
        صحیح cint;
        دوگانه cdouble;
        bool cbool;
        VMemberQualifiers qualifiers;
        V3UniqState uniqstate;
        V3TaggedState taggedstate;
        V3ImportProperty iprop;
        VSigning::en signstate;
        V3ErrorCode::en errcodeen;
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
std::ostream& operator<<(std::ostream& os, ثابت V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ثابت m_rootp;  // Root of the design
    VInFilter* ثابت m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ایستا V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_سطر* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_سطر* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    صحیح m_lexKwdDepth = 0;  // Inside a `begin_keywords
    صحیح m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    صحیح m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_عدد*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_سطر> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_گره* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

عمومی:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    صحیح yylexThis();
    ایستا bool optFuture(ثابت string& flag) { بازگشت v3Global.opt.isFuture(flag); }

    تهی tagNodep(Ast_گره* nodep) { m_tagNodep = nodep; }
    Ast_گره* tagNodep() ثابت { بازگشت m_tagNodep; }
    تهی lexTimescaleParse(File_سطر* fl, ثابت نویسه* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_سطر* fl, bool unitSet, دوگانه unitVal, bool precSet,
                               دوگانه precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() ثابت { بازگشت m_timeLastUnit; }
    VTimescale timeLastPrec() ثابت { بازگشت m_timeLastPrec; }

    تهی lexFileline(File_سطر* fl) { m_lexFileline = fl; }
    File_سطر* lexFileline() ثابت { بازگشت m_lexFileline; }
    std::string& lexString() { بازگشت m_lexString; }
    ایستا تهی lexErrorPreprocDirective(File_سطر* fl, ثابت نویسه* textp) VL_MT_DISABLED;
    ایستا string lexParseTag(ثابت نویسه* textp) VL_MT_DISABLED;
    ایستا دوگانه lexParseTimenum(ثابت نویسه* text) VL_MT_DISABLED;
    تهی lexPpline(ثابت نویسه* textp) VL_MT_DISABLED;
    تهی lexVerilatorCmtLint(File_سطر* fl, ثابت نویسه* textp, bool turnOff) VL_MT_DISABLED;
    تهی lexVerilatorCmtLintSave(ثابت File_سطر* fl) VL_MT_DISABLED;
    تهی lexVerilatorCmtLintRestore(File_سطر* fl) VL_MT_DISABLED;
    ایستا تهی lexVerilatorCmtBad(File_سطر* fl, ثابت نویسه* textp) VL_MT_DISABLED;

    تهی lex_افزودن_Keywords(صحیح state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_برداشت_Keywords() {
        اگر (m_lexKwdDepth) {
            --m_lexKwdDepth;
            بازگشت true;
        }
        بازگشت false;
    }
    صحیح lexKwdLastState() ثابت { بازگشت m_lexKwdLast; }
    ایستا ثابت نویسه* tokenName(صحیح tok) VL_MT_DISABLED;
    ایستا bool isStrengthToken(صحیح tok) VL_MT_DISABLED;

    تهی pp_افزودن_Text(ثابت string& text) {
        m_ppBuffers.افزودن_back(text);
        m_ppBytes += text.length();
        اگر (lexFileline()->contentp()) lexFileline()->contentp()->افزودن_Text(text);
    }
    size_t pp_درون_داد_To_Lex(نویسه* buf, size_t max_size) VL_MT_DISABLED;

    ایستا V3ParseImp* parsep() { بازگشت s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ثابت string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{text};
        m_stringps.افزودن_back(strp);
        بازگشت strp;
    }
    string* newString(ثابت نویسه* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{text};
        m_stringps.افزودن_back(strp);
        بازگشت strp;
    }
    string* newString(ثابت نویسه* text, size_t length) {
        string* ثابت strp = new string(text, length);  // Need () constructor
        m_stringps.افزودن_back(strp);
        بازگشت strp;
    }
    V3_عدد* new_عدد(File_سطر* flp, ثابت نویسه* text) {
        V3_عدد* nump = new V3_عدد{flp, V3_عدد::Verilog_عدد_Literal{}, text};
        m_numberps.افزودن_back(nump);
        بازگشت nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_گره* newBlock(File_سطر* fl, Ast_گره* nodep) {
        اگر (nodep) بازگشت nodep;
        بازگشت new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_سطر* bisonLastFileline() ثابت { بازگشت m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ثابت { بازگشت m_rootp; }
    bool inLibMap() ثابت { بازگشت m_inLibMap; }
    bool inLibrary() ثابت { بازگشت m_inLibrary; }
    string libname() ثابت { بازگشت m_libname; }
    VOptionBool unconnectedDrive() ثابت { بازگشت m_unconnectedDrive; }
    تهی unconnectedDrive(ثابت VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    صحیح bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    تهی lexNew() VL_MT_DISABLED;
    تهی lexDestroy() VL_MT_DISABLED;
    ایستا صحیح stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    صحیح lexPrevToken() ثابت { بازگشت m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_درون_داد_To_Lex(نویسه* buf, size_t max_size) { بازگشت pp_درون_داد_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_سطر* /*fl*/) { بازگشت parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    تهی parserClear() VL_MT_DISABLED;
    تهی lexUnputString(ثابت نویسه* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    صحیح tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    تهی parseFile(File_سطر* fileline, ثابت string& modfilename, bool inLibrary, bool inLibMap,
                   ثابت string& libname, ثابت string& errmsg,
                   ثابت std::string& notFoundName) VL_MT_DISABLED;
    تهی dumpInputsFile() VL_MT_DISABLED;
    تهی dumpTokensAhead(صحیح سطر) VL_MT_DISABLED;
    ایستا تهی candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    تهی importIfInStd(File_سطر* fileline, ثابت string& id, bool doImport);
    Ast_گره_عبارت* makePropertyCase(File_سطر* flp, Ast_گره_عبارت* exprp, AstCaseItem* itemsp);

خصوصی:
    تهی preprocDumps(std::ostream& os);
    تهی lexFile(ثابت string& modname) VL_MT_DISABLED;
    تهی yylexReadTok() VL_MT_DISABLED;
    تهی tokenPull() VL_MT_DISABLED;
    تهی tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    صحیح tokenPipelineId(صحیح token) VL_MT_DISABLED;
    تهی tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    ثابت V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    تهی preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
