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

#अगर_अघोषित VERILATOR_V3PARSEIMP_H_
#घोषणा VERILATOR_V3PARSEIMP_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3Global.h"
#समावेश "V3Parse.h"

#समावेश <algorithm>
#समावेश <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

क्रमागत V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

क्रमागत V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

क्रमागत V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

काष्ठा VMemberQualifiers final {
    जोड़ {
        uint32_t m_flags;
        काष्ठा {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    अटल VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        वापस q;
    }
    अटल VMemberQualifiers combine(स्थिर VMemberQualifiers& a, स्थिर VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        वापस q;
    }
    VRandAttr randAttr() स्थिर {
        अगर (m_randc) वापस VRandAttr::RAND_CYCLIC;
        अगर (m_rand) वापस VRandAttr::RAND;
        वापस VRandAttr::NONE;
    }
    व्योम applyToNodes(Ast_गाँठ_FTask* nodesp) स्थिर {
        क्रम (Ast_गाँठ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), गाँठ_FTask)) {
            अगर (m_local) nodep->isHideLocal(true);
            अगर (m_protected) nodep->isHideProtected(true);
            अगर (m_static) nodep->isStatic(true);
            अगर (m_virtual) nodep->isVirtual(true);
            अगर (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    व्योम applyToNodes(AstTypedef* nodep) स्थिर {
        अगर (m_local) nodep->isHideLocal(true);
        अगर (m_protected) nodep->isHideProtected(true);
        अगर (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    व्योम applyToNodes(AstVar* nodesp) स्थिर {
        क्रम (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            अगर (m_rand) nodep->अक्रम(VRandAttr::RAND);
            अगर (m_randc) nodep->अक्रम(VRandAttr::RAND_CYCLIC);
            अगर (m_local) nodep->isHideLocal(true);
            अगर (m_protected) nodep->isHideProtected(true);
            अगर (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            अगर (m_const) nodep->isConst(true);
            अगर (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

काष्ठा V3ParseBisonYYSType final {
    File_पंक्ति* fl = nullptr;
    पूर्णांक token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    जोड़ {
        V3_संख्या* nump;
        std::string* strp;
        पूर्णांक cint;
        द्विगुन cdouble;
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

#समावेश "V3Ast__gen_yystype.h"
    };
};
std::ostream& चालक<<(std::ostream& os, स्थिर V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#घोषणा YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* स्थिर m_rootp;  // Root of the design
    VInFilter* स्थिर m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    अटल V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_पंक्ति* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_पंक्ति* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    पूर्णांक m_lexKwdDepth = 0;  // Inside a `begin_keywords
    पूर्णांक m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    पूर्णांक m_lex_पूर्व_Token = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_संख्या*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_पंक्ति> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    माप_प्रकार m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_गाँठ* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

खुला:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    पूर्णांक yylexThis();
    अटल bool optFuture(स्थिर string& flag) { वापस v3Global.opt.isFuture(flag); }

    व्योम tagNodep(Ast_गाँठ* nodep) { m_tagNodep = nodep; }
    Ast_गाँठ* tagNodep() स्थिर { वापस m_tagNodep; }
    व्योम lexTimescaleParse(File_पंक्ति* fl, स्थिर अक्षर* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_पंक्ति* fl, bool unitSet, द्विगुन unitVal, bool precSet,
                               द्विगुन precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() स्थिर { वापस m_timeLastUnit; }
    VTimescale timeLastPrec() स्थिर { वापस m_timeLastPrec; }

    व्योम lexFileline(File_पंक्ति* fl) { m_lexFileline = fl; }
    File_पंक्ति* lexFileline() स्थिर { वापस m_lexFileline; }
    std::string& lexString() { वापस m_lexString; }
    अटल व्योम lexErrorPreprocDirective(File_पंक्ति* fl, स्थिर अक्षर* textp) VL_MT_DISABLED;
    अटल string lexParseTag(स्थिर अक्षर* textp) VL_MT_DISABLED;
    अटल द्विगुन lexParseTimenum(स्थिर अक्षर* text) VL_MT_DISABLED;
    व्योम lexPpline(स्थिर अक्षर* textp) VL_MT_DISABLED;
    व्योम lexVerilatorCmtLint(File_पंक्ति* fl, स्थिर अक्षर* textp, bool turnOff) VL_MT_DISABLED;
    व्योम lexVerilatorCmtLintSave(स्थिर File_पंक्ति* fl) VL_MT_DISABLED;
    व्योम lexVerilatorCmtLintRestore(File_पंक्ति* fl) VL_MT_DISABLED;
    अटल व्योम lexVerilatorCmtBad(File_पंक्ति* fl, स्थिर अक्षर* textp) VL_MT_DISABLED;

    व्योम lex_धकेलो_Keywords(पूर्णांक state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_निकालो_Keywords() {
        अगर (m_lexKwdDepth) {
            --m_lexKwdDepth;
            वापस true;
        }
        वापस false;
    }
    पूर्णांक lexKwdLastState() स्थिर { वापस m_lexKwdLast; }
    अटल स्थिर अक्षर* tokenName(पूर्णांक tok) VL_MT_DISABLED;
    अटल bool isStrengthToken(पूर्णांक tok) VL_MT_DISABLED;

    व्योम pp_धकेलो_Text(स्थिर string& text) {
        m_ppBuffers.धकेलो_back(text);
        m_ppBytes += text.लंबाई();
        अगर (lexFileline()->contentp()) lexFileline()->contentp()->धकेलो_Text(text);
    }
    माप_प्रकार pp_निवेश_To_Lex(अक्षर* buf, माप_प्रकार max_आकार_2) VL_MT_DISABLED;

    अटल V3ParseImp* parsep() { वापस s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(स्थिर string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* स्थिर strp = new std::string{text};
        m_stringps.धकेलो_back(strp);
        वापस strp;
    }
    string* newString(स्थिर अक्षर* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* स्थिर strp = new std::string{text};
        m_stringps.धकेलो_back(strp);
        वापस strp;
    }
    string* newString(स्थिर अक्षर* text, माप_प्रकार लंबाई) {
        string* स्थिर strp = new string(text, लंबाई);  // Need () constructor
        m_stringps.धकेलो_back(strp);
        वापस strp;
    }
    V3_संख्या* new_संख्या(File_पंक्ति* flp, स्थिर अक्षर* text) {
        V3_संख्या* nump = new V3_संख्या{flp, V3_संख्या::Verilog_संख्या_Literal{}, text};
        m_numberps.धकेलो_back(nump);
        वापस nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_गाँठ* newBlock(File_पंक्ति* fl, Ast_गाँठ* nodep) {
        अगर (nodep) वापस nodep;
        वापस new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_पंक्ति* bisonLastFileline() स्थिर { वापस m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() स्थिर { वापस m_rootp; }
    bool inLibMap() स्थिर { वापस m_inLibMap; }
    bool inLibrary() स्थिर { वापस m_inLibrary; }
    string libname() स्थिर { वापस m_libname; }
    VOptionBool unconnectedDrive() स्थिर { वापस m_unconnectedDrive; }
    व्योम unconnectedDrive(स्थिर VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    पूर्णांक bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    व्योम lexNew() VL_MT_DISABLED;
    व्योम lexDestroy() VL_MT_DISABLED;
    अटल पूर्णांक stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    पूर्णांक lex_पूर्व_Token() स्थिर { वापस m_lex_पूर्व_Token; }  // Parser -> lexer communication
    माप_प्रकार flex_Pp_निवेश_To_Lex(अक्षर* buf, माप_प्रकार max_आकार_2) { वापस pp_निवेश_To_Lex(buf, max_आकार_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_पंक्ति* /*fl*/) { वापस parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    व्योम parserClear() VL_MT_DISABLED;
    व्योम lexUnputString(स्थिर अक्षर* textp, माप_प्रकार लंबाई) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    पूर्णांक tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    व्योम parseFile(File_पंक्ति* fileline, स्थिर string& modfilename, bool inLibrary, bool inLibMap,
                   स्थिर string& libname, स्थिर string& errmsg,
                   स्थिर std::string& notFoundName) VL_MT_DISABLED;
    व्योम dumpInputsFile() VL_MT_DISABLED;
    व्योम dumpTokensAhead(पूर्णांक पंक्ति) VL_MT_DISABLED;
    अटल व्योम candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    व्योम importIfInStd(File_पंक्ति* fileline, स्थिर string& id, bool doImport);
    Ast_गाँठ_व्यंजक* makePropertyCase(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* exprp, AstCaseItem* itemsp);

निजी:
    व्योम preprocDumps(std::ostream& os);
    व्योम lexFile(स्थिर string& modname) VL_MT_DISABLED;
    व्योम yylexReadTok() VL_MT_DISABLED;
    व्योम tokenPull() VL_MT_DISABLED;
    व्योम tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    पूर्णांक tokenPipelineId(पूर्णांक token) VL_MT_DISABLED;
    व्योम tokenPipelineSym() VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanIdInst(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanIdType(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanBracket(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanParam(माप_प्रकार depth, bool forInst) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanParens(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanEqNew(माप_प्रकार depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(माप_प्रकार depth) VL_MT_DISABLED;
    स्थिर V3ParseBisonYYSType* tokenPeekp(माप_प्रकार depth) VL_MT_DISABLED;
    व्योम preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
