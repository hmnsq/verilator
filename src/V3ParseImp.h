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

تعداد V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

تعداد V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

تعداد V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

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
        𐤀𐤌 (m_randc) أرجع VRandAttr::RAND_CYCLIC;
        𐤀𐤌 (m_rand) أرجع VRandAttr::RAND;
        أرجع VRandAttr::NONE;
    }
    فراغ applyToNodes(AstNodeFTask* nodesp) ثابت {
        لكل (AstNodeFTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), NodeFTask)) {
            𐤀𐤌 (m_local) nodep->isHideLocal(true);
            𐤀𐤌 (m_protected) nodep->isHideProtected(true);
            𐤀𐤌 (m_static) nodep->isStatic(true);
            𐤀𐤌 (m_virtual) nodep->isVirtual(true);
            𐤀𐤌 (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    فراغ applyToNodes(AstTypedef* nodep) ثابت {
        𐤀𐤌 (m_local) nodep->isHideLocal(true);
        𐤀𐤌 (m_protected) nodep->isHideProtected(true);
        𐤀𐤌 (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    فراغ applyToNodes(AstVar* nodesp) ثابت {
        لكل (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            𐤀𐤌 (m_rand) nodep->rand(VRandAttr::RAND);
            𐤀𐤌 (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            𐤀𐤌 (m_local) nodep->isHideLocal(true);
            𐤀𐤌 (m_protected) nodep->isHideProtected(true);
            𐤀𐤌 (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            𐤀𐤌 (m_const) nodep->isConst(true);
            𐤀𐤌 (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

بنية V3ParseBisonYYSType final {
    FileLine* fl = nullptr;
    صحيح token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    اتحاد {
        V3Number* nump;
        std::string* strp;
        صحيح cint;
        مزدوج cdouble;
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

#ضمن "V3Ast__gen_yystype.h"
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
    ساكن V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    FileLine* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    FileLine* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    صحيح m_lexKwdDepth = 0;  // Inside a `begin_keywords
    صحيح m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    صحيح m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3Number*> m_numberps;  // Created numbers for later cleanup
    std::deque<FileLine> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    AstNode* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

عام:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    صحيح yylexThis();
    ساكن bool optFuture(ثابت string& flag) { أرجع v3Global.opt.isFuture(flag); }

    فراغ tagNodep(AstNode* nodep) { m_tagNodep = nodep; }
    AstNode* tagNodep() ثابت { أرجع m_tagNodep; }
    فراغ lexTimescaleParse(FileLine* fl, ثابت حرف* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(FileLine* fl, bool unitSet, مزدوج unitVal, bool precSet,
                               مزدوج precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() ثابت { أرجع m_timeLastUnit; }
    VTimescale timeLastPrec() ثابت { أرجع m_timeLastPrec; }

    فراغ lexFileline(FileLine* fl) { m_lexFileline = fl; }
    FileLine* lexFileline() ثابت { أرجع m_lexFileline; }
    std::string& lexString() { أرجع m_lexString; }
    ساكن فراغ lexErrorPreprocDirective(FileLine* fl, ثابت حرف* textp) VL_MT_DISABLED;
    ساكن string lexParseTag(ثابت حرف* textp) VL_MT_DISABLED;
    ساكن مزدوج lexParseTimenum(ثابت حرف* text) VL_MT_DISABLED;
    فراغ lexPpline(ثابت حرف* textp) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLint(FileLine* fl, ثابت حرف* textp, bool turnOff) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLintSave(ثابت FileLine* fl) VL_MT_DISABLED;
    فراغ lexVerilatorCmtLintRestore(FileLine* fl) VL_MT_DISABLED;
    ساكن فراغ lexVerilatorCmtBad(FileLine* fl, ثابت حرف* textp) VL_MT_DISABLED;

    فراغ lexPushKeywords(صحيح state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lexPopKeywords() {
        𐤀𐤌 (m_lexKwdDepth) {
            --m_lexKwdDepth;
            أرجع true;
        }
        أرجع false;
    }
    صحيح lexKwdLastState() ثابت { أرجع m_lexKwdLast; }
    ساكن ثابت حرف* tokenName(صحيح tok) VL_MT_DISABLED;
    ساكن bool isStrengthToken(صحيح tok) VL_MT_DISABLED;

    فراغ ppPushText(ثابت string& text) {
        m_ppBuffers.push_back(text);
        m_ppBytes += text.length();
        𐤀𐤌 (lexFileline()->contentp()) lexFileline()->contentp()->pushText(text);
    }
    size_t ppInputToLex(حرف* buf, size_t max_size) VL_MT_DISABLED;

    ساكن V3ParseImp* parsep() { أرجع s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ثابت string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{text};
        m_stringps.push_back(strp);
        أرجع strp;
    }
    string* newString(ثابت حرف* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ثابت strp = new std::string{text};
        m_stringps.push_back(strp);
        أرجع strp;
    }
    string* newString(ثابت حرف* text, size_t length) {
        string* ثابت strp = new string(text, length);  // Need () constructor
        m_stringps.push_back(strp);
        أرجع strp;
    }
    V3Number* newNumber(FileLine* flp, ثابت حرف* text) {
        V3Number* nump = new V3Number{flp, V3Number::VerilogNumberLiteral{}, text};
        m_numberps.push_back(nump);
        أرجع nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    AstNode* newBlock(FileLine* fl, AstNode* nodep) {
        𐤀𐤌 (nodep) أرجع nodep;
        أرجع new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    FileLine* bisonLastFileline() ثابت { أرجع m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ثابت { أرجع m_rootp; }
    bool inLibMap() ثابت { أرجع m_inLibMap; }
    bool inLibrary() ثابت { أرجع m_inLibrary; }
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
    size_t flexPpInputToLex(حرف* buf, size_t max_size) { أرجع ppInputToLex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(FileLine* /*fl*/) { أرجع parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    فراغ parserClear() VL_MT_DISABLED;
    فراغ lexUnputString(ثابت حرف* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    صحيح tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    فراغ parseFile(FileLine* fileline, ثابت string& modfilename, bool inLibrary, bool inLibMap,
                   ثابت string& libname, ثابت string& errmsg,
                   ثابت std::string& notFoundName) VL_MT_DISABLED;
    فراغ dumpInputsFile() VL_MT_DISABLED;
    فراغ dumpTokensAhead(صحيح line) VL_MT_DISABLED;
    ساكن فراغ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    فراغ importIfInStd(FileLine* fileline, ثابت string& id, bool doImport);
    AstNodeExpr* makePropertyCase(FileLine* flp, AstNodeExpr* exprp, AstCaseItem* itemsp);

خاص:
    فراغ preprocDumps(std::ostream& os);
    فراغ lexFile(ثابت string& modname) VL_MT_DISABLED;
    فراغ yylexReadTok() VL_MT_DISABLED;
    فراغ tokenPull() VL_MT_DISABLED;
    فراغ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    صحيح tokenPipelineId(صحيح token) VL_MT_DISABLED;
    فراغ tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    ثابت V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    فراغ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
