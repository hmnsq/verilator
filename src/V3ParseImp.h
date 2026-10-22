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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Parse.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

ગણના V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

ગણના V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

ગણના V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

સંરચના VMemberQualifiers final {
    સંઘ {
        uint32_t m_flags;
        સંરચના {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    સ્થિર VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        પાછા q;
    }
    સ્થિર VMemberQualifiers combine(અચલ VMemberQualifiers& a, અચલ VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        પાછા q;
    }
    VRandAttr randAttr() અચલ {
        જો (m_randc) પાછા VRandAttr::RAND_CYCLIC;
        જો (m_rand) પાછા VRandAttr::RAND;
        પાછા VRandAttr::NONE;
    }
    શૂન્ય applyToNodes(Ast_ગાંઠ_FTask* nodesp) અચલ {
        માટે (Ast_ગાંઠ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), ગાંઠ_FTask)) {
            જો (m_local) nodep->isHideLocal(true);
            જો (m_protected) nodep->isHideProtected(true);
            જો (m_static) nodep->isStatic(true);
            જો (m_virtual) nodep->isVirtual(true);
            જો (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    શૂન્ય applyToNodes(AstTypedef* nodep) અચલ {
        જો (m_local) nodep->isHideLocal(true);
        જો (m_protected) nodep->isHideProtected(true);
        જો (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    શૂન્ય applyToNodes(AstVar* nodesp) અચલ {
        માટે (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            જો (m_rand) nodep->rand(VRandAttr::RAND);
            જો (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            જો (m_local) nodep->isHideLocal(true);
            જો (m_protected) nodep->isHideProtected(true);
            જો (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            જો (m_const) nodep->isConst(true);
            જો (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

સંરચના V3ParseBisonYYSType final {
    File_લીટી* fl = nullptr;
    પૂર્ણાંક token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    સંઘ {
        V3_સંખ્યા* nump;
        std::string* strp;
        પૂર્ણાંક cint;
        દ્વિગુણ cdouble;
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

#અન્તર્ભાવ "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, અચલ V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* અચલ m_rootp;  // Root of the design
    VInFilter* અચલ m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    સ્થિર V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_લીટી* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_લીટી* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    પૂર્ણાંક m_lexKwdDepth = 0;  // Inside a `begin_keywords
    પૂર્ણાંક m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    પૂર્ણાંક m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_સંખ્યા*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_લીટી> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_ગાંઠ* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

જાહેર:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    પૂર્ણાંક yylexThis();
    સ્થિર bool optFuture(અચલ string& flag) { પાછા v3Global.opt.isFuture(flag); }

    શૂન્ય tagNodep(Ast_ગાંઠ* nodep) { m_tagNodep = nodep; }
    Ast_ગાંઠ* tagNodep() અચલ { પાછા m_tagNodep; }
    શૂન્ય lexTimescaleParse(File_લીટી* fl, અચલ અક્ષર* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_લીટી* fl, bool unitSet, દ્વિગુણ unitVal, bool precSet,
                               દ્વિગુણ precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() અચલ { પાછા m_timeLastUnit; }
    VTimescale timeLastPrec() અચલ { પાછા m_timeLastPrec; }

    શૂન્ય lexFileline(File_લીટી* fl) { m_lexFileline = fl; }
    File_લીટી* lexFileline() અચલ { પાછા m_lexFileline; }
    std::string& lexString() { પાછા m_lexString; }
    સ્થિર શૂન્ય lexErrorPreprocDirective(File_લીટી* fl, અચલ અક્ષર* textp) VL_MT_DISABLED;
    સ્થિર string lexParseTag(અચલ અક્ષર* textp) VL_MT_DISABLED;
    સ્થિર દ્વિગુણ lexParseTimenum(અચલ અક્ષર* text) VL_MT_DISABLED;
    શૂન્ય lexPpline(અચલ અક્ષર* textp) VL_MT_DISABLED;
    શૂન્ય lexVerilatorCmtLint(File_લીટી* fl, અચલ અક્ષર* textp, bool turnOff) VL_MT_DISABLED;
    શૂન્ય lexVerilatorCmtLintSave(અચલ File_લીટી* fl) VL_MT_DISABLED;
    શૂન્ય lexVerilatorCmtLintRestore(File_લીટી* fl) VL_MT_DISABLED;
    સ્થિર શૂન્ય lexVerilatorCmtBad(File_લીટી* fl, અચલ અક્ષર* textp) VL_MT_DISABLED;

    શૂન્ય lex_ધકેલો_Keywords(પૂર્ણાંક state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_કાઢો_Keywords() {
        જો (m_lexKwdDepth) {
            --m_lexKwdDepth;
            પાછા true;
        }
        પાછા false;
    }
    પૂર્ણાંક lexKwdLastState() અચલ { પાછા m_lexKwdLast; }
    સ્થિર અચલ અક્ષર* tokenName(પૂર્ણાંક tok) VL_MT_DISABLED;
    સ્થિર bool isStrengthToken(પૂર્ણાંક tok) VL_MT_DISABLED;

    શૂન્ય pp_ધકેલો_Text(અચલ string& text) {
        m_ppBuffers.ધકેલો_back(text);
        m_ppBytes += text.length();
        જો (lexFileline()->contentp()) lexFileline()->contentp()->ધકેલો_Text(text);
    }
    size_t pp_આગત_To_Lex(અક્ષર* buf, size_t max_size) VL_MT_DISABLED;

    સ્થિર V3ParseImp* parsep() { પાછા s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(અચલ string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* અચલ strp = new std::string{text};
        m_stringps.ધકેલો_back(strp);
        પાછા strp;
    }
    string* newString(અચલ અક્ષર* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* અચલ strp = new std::string{text};
        m_stringps.ધકેલો_back(strp);
        પાછા strp;
    }
    string* newString(અચલ અક્ષર* text, size_t length) {
        string* અચલ strp = new string(text, length);  // Need () constructor
        m_stringps.ધકેલો_back(strp);
        પાછા strp;
    }
    V3_સંખ્યા* new_સંખ્યા(File_લીટી* flp, અચલ અક્ષર* text) {
        V3_સંખ્યા* nump = new V3_સંખ્યા{flp, V3_સંખ્યા::Verilog_સંખ્યા_Literal{}, text};
        m_numberps.ધકેલો_back(nump);
        પાછા nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_ગાંઠ* newBlock(File_લીટી* fl, Ast_ગાંઠ* nodep) {
        જો (nodep) પાછા nodep;
        પાછા new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_લીટી* bisonLastFileline() અચલ { પાછા m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() અચલ { પાછા m_rootp; }
    bool inLibMap() અચલ { પાછા m_inLibMap; }
    bool inLibrary() અચલ { પાછા m_inLibrary; }
    string libname() અચલ { પાછા m_libname; }
    VOptionBool unconnectedDrive() અચલ { પાછા m_unconnectedDrive; }
    શૂન્ય unconnectedDrive(અચલ VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    પૂર્ણાંક bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    શૂન્ય lexNew() VL_MT_DISABLED;
    શૂન્ય lexDestroy() VL_MT_DISABLED;
    સ્થિર પૂર્ણાંક stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    પૂર્ણાંક lexPrevToken() અચલ { પાછા m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_આગત_To_Lex(અક્ષર* buf, size_t max_size) { પાછા pp_આગત_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_લીટી* /*fl*/) { પાછા parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    શૂન્ય parserClear() VL_MT_DISABLED;
    શૂન્ય lexUnputString(અચલ અક્ષર* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    પૂર્ણાંક tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    શૂન્ય parseFile(File_લીટી* fileline, અચલ string& modfilename, bool inLibrary, bool inLibMap,
                   અચલ string& libname, અચલ string& errmsg,
                   અચલ std::string& notFoundName) VL_MT_DISABLED;
    શૂન્ય dumpInputsFile() VL_MT_DISABLED;
    શૂન્ય dumpTokensAhead(પૂર્ણાંક લીટી) VL_MT_DISABLED;
    સ્થિર શૂન્ય candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    શૂન્ય importIfInStd(File_લીટી* fileline, અચલ string& id, bool doImport);
    Ast_ગાંઠ_પદાવલિ* makePropertyCase(File_લીટી* flp, Ast_ગાંઠ_પદાવલિ* exprp, AstCaseItem* itemsp);

ખાનગી:
    શૂન્ય preprocDumps(std::ostream& os);
    શૂન્ય lexFile(અચલ string& modname) VL_MT_DISABLED;
    શૂન્ય yylexReadTok() VL_MT_DISABLED;
    શૂન્ય tokenPull() VL_MT_DISABLED;
    શૂન્ય tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    પૂર્ણાંક tokenPipelineId(પૂર્ણાંક token) VL_MT_DISABLED;
    શૂન્ય tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    અચલ V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    શૂન્ય preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
