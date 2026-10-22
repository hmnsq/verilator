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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Ast.h"
#include "V3Error.h"
#include "V3FileLine.h"
#include "V3Global.h"
#include "V3Parse.h"

#include <algorithm>
#include <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

ga_nanaa V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

ga_nanaa V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

ga_nanaa V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

sa_nracanaa VMemberQualifiers final {
    sa_ngha {
        uint32_t m_flags;
        sa_nracanaa {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    static VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        ᱨᱩᱣᱟᱹᱲ q;
    }
    static VMemberQualifiers combine(const VMemberQualifiers& a, const VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        ᱨᱩᱣᱟᱹᱲ q;
    }
    VRandAttr randAttr() const {
        ᱡᱩᱫᱤ (m_randc) ᱨᱩᱣᱟᱹᱲ VRandAttr::RAND_CYCLIC;
        ᱡᱩᱫᱤ (m_rand) ᱨᱩᱣᱟᱹᱲ VRandAttr::RAND;
        ᱨᱩᱣᱟᱹᱲ VRandAttr::NONE;
    }
    void applyToNodes(AstNodeFTask* nodesp) const {
        for (AstNodeFTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), NodeFTask)) {
            ᱡᱩᱫᱤ (m_local) nodep->isHideLocal(true);
            ᱡᱩᱫᱤ (m_protected) nodep->isHideProtected(true);
            ᱡᱩᱫᱤ (m_static) nodep->isStatic(true);
            ᱡᱩᱫᱤ (m_virtual) nodep->isVirtual(true);
            ᱡᱩᱫᱤ (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    void applyToNodes(AstTypedef* nodep) const {
        ᱡᱩᱫᱤ (m_local) nodep->isHideLocal(true);
        ᱡᱩᱫᱤ (m_protected) nodep->isHideProtected(true);
        ᱡᱩᱫᱤ (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    void applyToNodes(AstVar* nodesp) const {
        for (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            ᱡᱩᱫᱤ (m_rand) nodep->rand(VRandAttr::RAND);
            ᱡᱩᱫᱤ (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            ᱡᱩᱫᱤ (m_local) nodep->isHideLocal(true);
            ᱡᱩᱫᱤ (m_protected) nodep->isHideProtected(true);
            ᱡᱩᱫᱤ (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            ᱡᱩᱫᱤ (m_const) nodep->isConst(true);
            ᱡᱩᱫᱤ (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

sa_nracanaa V3ParseBisonYYSType final {
    FileLine* fl = nullptr;
    puur_naa_nka token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    sa_ngha {
        V3Number* nump;
        std::string* strp;
        puur_naa_nka cint;
        dviguu_na cdouble;
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

#include "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, const V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* const m_rootp;  // Root of the design
    VInFilter* const m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    static V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    FileLine* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    FileLine* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    puur_naa_nka m_lexKwdDepth = 0;  // Inside a `begin_keywords
    puur_naa_nka m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    puur_naa_nka m_lexPrevToken = 0;  // previous parsed token (for lexer)
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

public:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    puur_naa_nka yylexThis();
    static bool optFuture(const string& flag) { ᱨᱩᱣᱟᱹᱲ v3Global.opt.isFuture(flag); }

    void tagNodep(AstNode* nodep) { m_tagNodep = nodep; }
    AstNode* tagNodep() const { ᱨᱩᱣᱟᱹᱲ m_tagNodep; }
    void lexTimescaleParse(FileLine* fl, const var_na* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(FileLine* fl, bool unitSet, dviguu_na unitVal, bool precSet,
                               dviguu_na precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() const { ᱨᱩᱣᱟᱹᱲ m_timeLastUnit; }
    VTimescale timeLastPrec() const { ᱨᱩᱣᱟᱹᱲ m_timeLastPrec; }

    void lexFileline(FileLine* fl) { m_lexFileline = fl; }
    FileLine* lexFileline() const { ᱨᱩᱣᱟᱹᱲ m_lexFileline; }
    std::string& lexString() { ᱨᱩᱣᱟᱹᱲ m_lexString; }
    static void lexErrorPreprocDirective(FileLine* fl, const var_na* textp) VL_MT_DISABLED;
    static string lexParseTag(const var_na* textp) VL_MT_DISABLED;
    static dviguu_na lexParseTimenum(const var_na* text) VL_MT_DISABLED;
    void lexPpline(const var_na* textp) VL_MT_DISABLED;
    void lexVerilatorCmtLint(FileLine* fl, const var_na* textp, bool turnOff) VL_MT_DISABLED;
    void lexVerilatorCmtLintSave(const FileLine* fl) VL_MT_DISABLED;
    void lexVerilatorCmtLintRestore(FileLine* fl) VL_MT_DISABLED;
    static void lexVerilatorCmtBad(FileLine* fl, const var_na* textp) VL_MT_DISABLED;

    void lexPushKeywords(puur_naa_nka state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lexPopKeywords() {
        ᱡᱩᱫᱤ (m_lexKwdDepth) {
            --m_lexKwdDepth;
            ᱨᱩᱣᱟᱹᱲ true;
        }
        ᱨᱩᱣᱟᱹᱲ false;
    }
    puur_naa_nka lexKwdLastState() const { ᱨᱩᱣᱟᱹᱲ m_lexKwdLast; }
    static const var_na* tokenName(puur_naa_nka tok) VL_MT_DISABLED;
    static bool isStrengthToken(puur_naa_nka tok) VL_MT_DISABLED;

    void ppPushText(const string& text) {
        m_ppBuffers.push_back(text);
        m_ppBytes += text.length();
        ᱡᱩᱫᱤ (lexFileline()->contentp()) lexFileline()->contentp()->pushText(text);
    }
    size_t ppInputToLex(var_na* buf, size_t max_size) VL_MT_DISABLED;

    static V3ParseImp* parsep() { ᱨᱩᱣᱟᱹᱲ s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(const string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* const strp = new std::string{text};
        m_stringps.push_back(strp);
        ᱨᱩᱣᱟᱹᱲ strp;
    }
    string* newString(const var_na* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* const strp = new std::string{text};
        m_stringps.push_back(strp);
        ᱨᱩᱣᱟᱹᱲ strp;
    }
    string* newString(const var_na* text, size_t length) {
        string* const strp = new string(text, length);  // Need () constructor
        m_stringps.push_back(strp);
        ᱨᱩᱣᱟᱹᱲ strp;
    }
    V3Number* newNumber(FileLine* flp, const var_na* text) {
        V3Number* nump = new V3Number{flp, V3Number::VerilogNumberLiteral{}, text};
        m_numberps.push_back(nump);
        ᱨᱩᱣᱟᱹᱲ nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    AstNode* newBlock(FileLine* fl, AstNode* nodep) {
        ᱡᱩᱫᱤ (nodep) ᱨᱩᱣᱟᱹᱲ nodep;
        ᱨᱩᱣᱟᱹᱲ new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    FileLine* bisonLastFileline() const { ᱨᱩᱣᱟᱹᱲ m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() const { ᱨᱩᱣᱟᱹᱲ m_rootp; }
    bool inLibMap() const { ᱨᱩᱣᱟᱹᱲ m_inLibMap; }
    bool inLibrary() const { ᱨᱩᱣᱟᱹᱲ m_inLibrary; }
    string libname() const { ᱨᱩᱣᱟᱹᱲ m_libname; }
    VOptionBool unconnectedDrive() const { ᱨᱩᱣᱟᱹᱲ m_unconnectedDrive; }
    void unconnectedDrive(const VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    puur_naa_nka bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    void lexNew() VL_MT_DISABLED;
    void lexDestroy() VL_MT_DISABLED;
    static puur_naa_nka stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    puur_naa_nka lexPrevToken() const { ᱨᱩᱣᱟᱹᱲ m_lexPrevToken; }  // Parser -> lexer communication
    size_t flexPpInputToLex(var_na* buf, size_t max_size) { ᱨᱩᱣᱟᱹᱲ ppInputToLex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(FileLine* /*fl*/) { ᱨᱩᱣᱟᱹᱲ parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    void parserClear() VL_MT_DISABLED;
    void lexUnputString(const var_na* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    puur_naa_nka tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    void parseFile(FileLine* fileline, const string& modfilename, bool inLibrary, bool inLibMap,
                   const string& libname, const string& errmsg,
                   const std::string& notFoundName) VL_MT_DISABLED;
    void dumpInputsFile() VL_MT_DISABLED;
    void dumpTokensAhead(puur_naa_nka line) VL_MT_DISABLED;
    static void candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    void importIfInStd(FileLine* fileline, const string& id, bool doImport);
    AstNodeExpr* makePropertyCase(FileLine* flp, AstNodeExpr* exprp, AstCaseItem* itemsp);

private:
    void preprocDumps(std::ostream& os);
    void lexFile(const string& modname) VL_MT_DISABLED;
    void yylexReadTok() VL_MT_DISABLED;
    void tokenPull() VL_MT_DISABLED;
    void tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    puur_naa_nka tokenPipelineId(puur_naa_nka token) VL_MT_DISABLED;
    void tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    const V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    void preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
