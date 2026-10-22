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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

ಗಣನಾ V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

ಗಣನಾ V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

ಗಣನಾ V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

ಸಂರಚನಾ VMemberQualifiers final {
    ಸಙ್ಘ {
        uint32_t m_flags;
        ಸಂರಚನಾ {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ಸ್ಥಿರ VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        ಪ್ರತ್ಯಾಗಮ q;
    }
    ಸ್ಥಿರ VMemberQualifiers combine(ಧ್ರುವ VMemberQualifiers& a, ಧ್ರುವ VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        ಪ್ರತ್ಯಾಗಮ q;
    }
    VRandAttr randAttr() ಧ್ರುವ {
        ಯದಿ (m_randc) ಪ್ರತ್ಯಾಗಮ VRandAttr::RAND_CYCLIC;
        ಯದಿ (m_rand) ಪ್ರತ್ಯಾಗಮ VRandAttr::RAND;
        ಪ್ರತ್ಯಾಗಮ VRandAttr::NONE;
    }
    ಶೂನ್ಯ applyToNodes(Ast_ಗಂಟು_FTask* nodesp) ಧ್ರುವ {
        ಪ್ರತಿ (Ast_ಗಂಟು_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), ಗಂಟು_FTask)) {
            ಯದಿ (m_local) nodep->isHideLocal(true);
            ಯದಿ (m_protected) nodep->isHideProtected(true);
            ಯದಿ (m_static) nodep->isStatic(true);
            ಯದಿ (m_virtual) nodep->isVirtual(true);
            ಯದಿ (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    ಶೂನ್ಯ applyToNodes(AstTypedef* nodep) ಧ್ರುವ {
        ಯದಿ (m_local) nodep->isHideLocal(true);
        ಯದಿ (m_protected) nodep->isHideProtected(true);
        ಯದಿ (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    ಶೂನ್ಯ applyToNodes(AstVar* nodesp) ಧ್ರುವ {
        ಪ್ರತಿ (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            ಯದಿ (m_rand) nodep->rand(VRandAttr::RAND);
            ಯದಿ (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            ಯದಿ (m_local) nodep->isHideLocal(true);
            ಯದಿ (m_protected) nodep->isHideProtected(true);
            ಯದಿ (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            ಯದಿ (m_const) nodep->isConst(true);
            ಯದಿ (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

ಸಂರಚನಾ V3ParseBisonYYSType final {
    File_ಸಾಲು* fl = nullptr;
    ಪೂರ್ಣಾಙ್ಕ token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    ಸಙ್ಘ {
        V3_ಸಂಖ್ಯೆ* nump;
        std::string* strp;
        ಪೂರ್ಣಾಙ್ಕ cint;
        ದ್ವಿಗುಣ cdouble;
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

#ಅನ್ತರ್ಭಾವ "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ಧ್ರುವ m_rootp;  // Root of the design
    VInFilter* ಧ್ರುವ m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ಸ್ಥಿರ V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_ಸಾಲು* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_ಸಾಲು* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    ಪೂರ್ಣಾಙ್ಕ m_lexKwdDepth = 0;  // Inside a `begin_keywords
    ಪೂರ್ಣಾಙ್ಕ m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    ಪೂರ್ಣಾಙ್ಕ m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_ಸಂಖ್ಯೆ*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_ಸಾಲು> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_ಗಂಟು* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

ಸಾರ್ವಜನಿಕ:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    ಪೂರ್ಣಾಙ್ಕ yylexThis();
    ಸ್ಥಿರ bool optFuture(ಧ್ರುವ string& flag) { ಪ್ರತ್ಯಾಗಮ v3Global.opt.isFuture(flag); }

    ಶೂನ್ಯ tagNodep(Ast_ಗಂಟು* nodep) { m_tagNodep = nodep; }
    Ast_ಗಂಟು* tagNodep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_tagNodep; }
    ಶೂನ್ಯ lexTimescaleParse(File_ಸಾಲು* fl, ಧ್ರುವ ವರ್ಣ* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_ಸಾಲು* fl, bool unitSet, ದ್ವಿಗುಣ unitVal, bool precSet,
                               ದ್ವಿಗುಣ precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeLastUnit; }
    VTimescale timeLastPrec() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeLastPrec; }

    ಶೂನ್ಯ lexFileline(File_ಸಾಲು* fl) { m_lexFileline = fl; }
    File_ಸಾಲು* lexFileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lexFileline; }
    std::string& lexString() { ಪ್ರತ್ಯಾಗಮ m_lexString; }
    ಸ್ಥಿರ ಶೂನ್ಯ lexErrorPreprocDirective(File_ಸಾಲು* fl, ಧ್ರುವ ವರ್ಣ* textp) VL_MT_DISABLED;
    ಸ್ಥಿರ string lexParseTag(ಧ್ರುವ ವರ್ಣ* textp) VL_MT_DISABLED;
    ಸ್ಥಿರ ದ್ವಿಗುಣ lexParseTimenum(ಧ್ರುವ ವರ್ಣ* text) VL_MT_DISABLED;
    ಶೂನ್ಯ lexPpline(ಧ್ರುವ ವರ್ಣ* textp) VL_MT_DISABLED;
    ಶೂನ್ಯ lexVerilatorCmtLint(File_ಸಾಲು* fl, ಧ್ರುವ ವರ್ಣ* textp, bool turnOff) VL_MT_DISABLED;
    ಶೂನ್ಯ lexVerilatorCmtLintSave(ಧ್ರುವ File_ಸಾಲು* fl) VL_MT_DISABLED;
    ಶೂನ್ಯ lexVerilatorCmtLintRestore(File_ಸಾಲು* fl) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ lexVerilatorCmtBad(File_ಸಾಲು* fl, ಧ್ರುವ ವರ್ಣ* textp) VL_MT_DISABLED;

    ಶೂನ್ಯ lex_ತಳ್ಳು_Keywords(ಪೂರ್ಣಾಙ್ಕ state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_ತೆಗೆ_Keywords() {
        ಯದಿ (m_lexKwdDepth) {
            --m_lexKwdDepth;
            ಪ್ರತ್ಯಾಗಮ true;
        }
        ಪ್ರತ್ಯಾಗಮ false;
    }
    ಪೂರ್ಣಾಙ್ಕ lexKwdLastState() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lexKwdLast; }
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* tokenName(ಪೂರ್ಣಾಙ್ಕ tok) VL_MT_DISABLED;
    ಸ್ಥಿರ bool isStrengthToken(ಪೂರ್ಣಾಙ್ಕ tok) VL_MT_DISABLED;

    ಶೂನ್ಯ pp_ತಳ್ಳು_Text(ಧ್ರುವ string& text) {
        m_ppBuffers.ತಳ್ಳು_back(text);
        m_ppBytes += text.length();
        ಯದಿ (lexFileline()->contentp()) lexFileline()->contentp()->ತಳ್ಳು_Text(text);
    }
    size_t pp_ಒಳಹರಿವು_To_Lex(ವರ್ಣ* buf, size_t max_size) VL_MT_DISABLED;

    ಸ್ಥಿರ V3ParseImp* parsep() { ಪ್ರತ್ಯಾಗಮ s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ಧ್ರುವ string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ಧ್ರುವ strp = new std::string{text};
        m_stringps.ತಳ್ಳು_back(strp);
        ಪ್ರತ್ಯಾಗಮ strp;
    }
    string* newString(ಧ್ರುವ ವರ್ಣ* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ಧ್ರುವ strp = new std::string{text};
        m_stringps.ತಳ್ಳು_back(strp);
        ಪ್ರತ್ಯಾಗಮ strp;
    }
    string* newString(ಧ್ರುವ ವರ್ಣ* text, size_t length) {
        string* ಧ್ರುವ strp = new string(text, length);  // Need () constructor
        m_stringps.ತಳ್ಳು_back(strp);
        ಪ್ರತ್ಯಾಗಮ strp;
    }
    V3_ಸಂಖ್ಯೆ* new_ಸಂಖ್ಯೆ(File_ಸಾಲು* flp, ಧ್ರುವ ವರ್ಣ* text) {
        V3_ಸಂಖ್ಯೆ* nump = new V3_ಸಂಖ್ಯೆ{flp, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, text};
        m_numberps.ತಳ್ಳು_back(nump);
        ಪ್ರತ್ಯಾಗಮ nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_ಗಂಟು* newBlock(File_ಸಾಲು* fl, Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep) ಪ್ರತ್ಯಾಗಮ nodep;
        ಪ್ರತ್ಯಾಗಮ new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_ಸಾಲು* bisonLastFileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_rootp; }
    bool inLibMap() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inLibMap; }
    bool inLibrary() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inLibrary; }
    string libname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libname; }
    VOptionBool unconnectedDrive() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unconnectedDrive; }
    ಶೂನ್ಯ unconnectedDrive(ಧ್ರುವ VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    ಪೂರ್ಣಾಙ್ಕ bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    ಶೂನ್ಯ lexNew() VL_MT_DISABLED;
    ಶೂನ್ಯ lexDestroy() VL_MT_DISABLED;
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    ಪೂರ್ಣಾಙ್ಕ lexPrevToken() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_ಒಳಹರಿವು_To_Lex(ವರ್ಣ* buf, size_t max_size) { ಪ್ರತ್ಯಾಗಮ pp_ಒಳಹರಿವು_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_ಸಾಲು* /*fl*/) { ಪ್ರತ್ಯಾಗಮ parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    ಶೂನ್ಯ parserClear() VL_MT_DISABLED;
    ಶೂನ್ಯ lexUnputString(ಧ್ರುವ ವರ್ಣ* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ಪೂರ್ಣಾಙ್ಕ tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    ಶೂನ್ಯ parseFile(File_ಸಾಲು* fileline, ಧ್ರುವ string& modfilename, bool inLibrary, bool inLibMap,
                   ಧ್ರುವ string& libname, ಧ್ರುವ string& errmsg,
                   ಧ್ರುವ std::string& notFoundName) VL_MT_DISABLED;
    ಶೂನ್ಯ dumpInputsFile() VL_MT_DISABLED;
    ಶೂನ್ಯ dumpTokensAhead(ಪೂರ್ಣಾಙ್ಕ ಸಾಲು) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    ಶೂನ್ಯ importIfInStd(File_ಸಾಲು* fileline, ಧ್ರುವ string& id, bool doImport);
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makePropertyCase(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, AstCaseItem* itemsp);

ಗುಪ್ತ:
    ಶೂನ್ಯ preprocDumps(std::ostream& os);
    ಶೂನ್ಯ lexFile(ಧ್ರುವ string& modname) VL_MT_DISABLED;
    ಶೂನ್ಯ yylexReadTok() VL_MT_DISABLED;
    ಶೂನ್ಯ tokenPull() VL_MT_DISABLED;
    ಶೂನ್ಯ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    ಪೂರ್ಣಾಙ್ಕ tokenPipelineId(ಪೂರ್ಣಾಙ್ಕ token) VL_MT_DISABLED;
    ಶೂನ್ಯ tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    ಧ್ರುವ V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    ಶೂನ್ಯ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
