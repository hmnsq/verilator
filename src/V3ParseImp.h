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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Parse.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

गणना V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

गणना V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

गणना V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

संरचना VMemberQualifiers final {
    संघ {
        uint32_t m_flags;
        संरचना {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    स्थिर VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        परत q;
    }
    स्थिर VMemberQualifiers combine(स्थिरांक VMemberQualifiers& a, स्थिरांक VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        परत q;
    }
    VRandAttr randAttr() स्थिरांक {
        जर (m_randc) परत VRandAttr::RAND_CYCLIC;
        जर (m_rand) परत VRandAttr::RAND;
        परत VRandAttr::NONE;
    }
    शून्य applyToNodes(Ast_गाठ_FTask* nodesp) स्थिरांक {
        साठी (Ast_गाठ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), गाठ_FTask)) {
            जर (m_local) nodep->isHideLocal(true);
            जर (m_protected) nodep->isHideProtected(true);
            जर (m_static) nodep->isStatic(true);
            जर (m_virtual) nodep->isVirtual(true);
            जर (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    शून्य applyToNodes(AstTypedef* nodep) स्थिरांक {
        जर (m_local) nodep->isHideLocal(true);
        जर (m_protected) nodep->isHideProtected(true);
        जर (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    शून्य applyToNodes(AstVar* nodesp) स्थिरांक {
        साठी (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            जर (m_rand) nodep->अक्रम(VRandAttr::RAND);
            जर (m_randc) nodep->अक्रम(VRandAttr::RAND_CYCLIC);
            जर (m_local) nodep->isHideLocal(true);
            जर (m_protected) nodep->isHideProtected(true);
            जर (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            जर (m_const) nodep->isConst(true);
            जर (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

संरचना V3ParseBisonYYSType final {
    File_ओळ* fl = nullptr;
    पूर्णांक token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    संघ {
        V3_संख्या* nump;
        std::string* strp;
        पूर्णांक cint;
        द्विगुण cdouble;
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

#अन्तर्भाव "V3Ast__gen_yystype.h"
    };
};
std::ostream& चालक<<(std::ostream& os, स्थिरांक V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* स्थिरांक m_rootp;  // Root of the design
    VInFilter* स्थिरांक m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    स्थिर V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_ओळ* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_ओळ* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    पूर्णांक m_lexKwdDepth = 0;  // Inside a `begin_keywords
    पूर्णांक m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    पूर्णांक m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_संख्या*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_ओळ> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    माप_प्रकार m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_गाठ* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

सार्वजनिक:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    पूर्णांक yylexThis();
    स्थिर bool optFuture(स्थिरांक string& flag) { परत v3Global.opt.isFuture(flag); }

    शून्य tagNodep(Ast_गाठ* nodep) { m_tagNodep = nodep; }
    Ast_गाठ* tagNodep() स्थिरांक { परत m_tagNodep; }
    शून्य lexTimescaleParse(File_ओळ* fl, स्थिरांक अक्षर* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_ओळ* fl, bool unitSet, द्विगुण unitVal, bool precSet,
                               द्विगुण precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() स्थिरांक { परत m_timeLastUnit; }
    VTimescale timeLastPrec() स्थिरांक { परत m_timeLastPrec; }

    शून्य lexFileline(File_ओळ* fl) { m_lexFileline = fl; }
    File_ओळ* lexFileline() स्थिरांक { परत m_lexFileline; }
    std::string& lexString() { परत m_lexString; }
    स्थिर शून्य lexErrorPreprocDirective(File_ओळ* fl, स्थिरांक अक्षर* textp) VL_MT_DISABLED;
    स्थिर string lexParseTag(स्थिरांक अक्षर* textp) VL_MT_DISABLED;
    स्थिर द्विगुण lexParseTimenum(स्थिरांक अक्षर* text) VL_MT_DISABLED;
    शून्य lexPpline(स्थिरांक अक्षर* textp) VL_MT_DISABLED;
    शून्य lexVerilatorCmtLint(File_ओळ* fl, स्थिरांक अक्षर* textp, bool turnOff) VL_MT_DISABLED;
    शून्य lexVerilatorCmtLintSave(स्थिरांक File_ओळ* fl) VL_MT_DISABLED;
    शून्य lexVerilatorCmtLintRestore(File_ओळ* fl) VL_MT_DISABLED;
    स्थिर शून्य lexVerilatorCmtBad(File_ओळ* fl, स्थिरांक अक्षर* textp) VL_MT_DISABLED;

    शून्य lex_ढकला_Keywords(पूर्णांक state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_काढा_Keywords() {
        जर (m_lexKwdDepth) {
            --m_lexKwdDepth;
            परत true;
        }
        परत false;
    }
    पूर्णांक lexKwdLastState() स्थिरांक { परत m_lexKwdLast; }
    स्थिर स्थिरांक अक्षर* tokenName(पूर्णांक tok) VL_MT_DISABLED;
    स्थिर bool isStrengthToken(पूर्णांक tok) VL_MT_DISABLED;

    शून्य pp_ढकला_Text(स्थिरांक string& text) {
        m_ppBuffers.ढकला_back(text);
        m_ppBytes += text.length();
        जर (lexFileline()->contentp()) lexFileline()->contentp()->ढकला_Text(text);
    }
    माप_प्रकार pp_आदान_To_Lex(अक्षर* buf, माप_प्रकार max_size) VL_MT_DISABLED;

    स्थिर V3ParseImp* parsep() { परत s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(स्थिरांक string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* स्थिरांक strp = new std::string{text};
        m_stringps.ढकला_back(strp);
        परत strp;
    }
    string* newString(स्थिरांक अक्षर* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* स्थिरांक strp = new std::string{text};
        m_stringps.ढकला_back(strp);
        परत strp;
    }
    string* newString(स्थिरांक अक्षर* text, माप_प्रकार length) {
        string* स्थिरांक strp = new string(text, length);  // Need () constructor
        m_stringps.ढकला_back(strp);
        परत strp;
    }
    V3_संख्या* new_संख्या(File_ओळ* flp, स्थिरांक अक्षर* text) {
        V3_संख्या* nump = new V3_संख्या{flp, V3_संख्या::Verilog_संख्या_Literal{}, text};
        m_numberps.ढकला_back(nump);
        परत nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_गाठ* newBlock(File_ओळ* fl, Ast_गाठ* nodep) {
        जर (nodep) परत nodep;
        परत new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_ओळ* bisonLastFileline() स्थिरांक { परत m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() स्थिरांक { परत m_rootp; }
    bool inLibMap() स्थिरांक { परत m_inLibMap; }
    bool inLibrary() स्थिरांक { परत m_inLibrary; }
    string libname() स्थिरांक { परत m_libname; }
    VOptionBool unconnectedDrive() स्थिरांक { परत m_unconnectedDrive; }
    शून्य unconnectedDrive(स्थिरांक VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    पूर्णांक bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    शून्य lexNew() VL_MT_DISABLED;
    शून्य lexDestroy() VL_MT_DISABLED;
    स्थिर पूर्णांक stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    पूर्णांक lexPrevToken() स्थिरांक { परत m_lexPrevToken; }  // Parser -> lexer communication
    माप_प्रकार flex_Pp_आदान_To_Lex(अक्षर* buf, माप_प्रकार max_size) { परत pp_आदान_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_ओळ* /*fl*/) { परत parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    शून्य parserClear() VL_MT_DISABLED;
    शून्य lexUnputString(स्थिरांक अक्षर* textp, माप_प्रकार length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    पूर्णांक tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    शून्य parseFile(File_ओळ* fileline, स्थिरांक string& modfilename, bool inLibrary, bool inLibMap,
                   स्थिरांक string& libname, स्थिरांक string& errmsg,
                   स्थिरांक std::string& notFoundName) VL_MT_DISABLED;
    शून्य dumpInputsFile() VL_MT_DISABLED;
    शून्य dumpTokensAhead(पूर्णांक ओळ) VL_MT_DISABLED;
    स्थिर शून्य candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    शून्य importIfInStd(File_ओळ* fileline, स्थिरांक string& id, bool doImport);
    Ast_गाठ_अभिव्यक्ती* makePropertyCase(File_ओळ* flp, Ast_गाठ_अभिव्यक्ती* exprp, AstCaseItem* itemsp);

खाजगी:
    शून्य preprocDumps(std::ostream& os);
    शून्य lexFile(स्थिरांक string& modname) VL_MT_DISABLED;
    शून्य yylexReadTok() VL_MT_DISABLED;
    शून्य tokenPull() VL_MT_DISABLED;
    शून्य tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    पूर्णांक tokenPipelineId(पूर्णांक token) VL_MT_DISABLED;
    शून्य tokenPipelineSym() VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanIdInst(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanIdType(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanBracket(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanParam(माप_प्रकार depth, bool forInst) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanParens(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanEqNew(माप_प्रकार depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(माप_प्रकार depth) VL_MT_DISABLED;
    स्थिरांक V3ParseBisonYYSType* tokenPeekp(माप_प्रकार depth) VL_MT_DISABLED;
    शून्य preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
