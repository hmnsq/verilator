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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Parse.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

ଗଣନା V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

ଗଣନା V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

ଗଣନା V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

ସଂରଚନା VMemberQualifiers final {
    ସଂଘ {
        uint32_t m_flags;
        ସଂରଚନା {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ସ୍ଥିର VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        ଫେରନ୍ତୁ q;
    }
    ସ୍ଥିର VMemberQualifiers combine(ଅଚଳ VMemberQualifiers& a, ଅଚଳ VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        ଫେରନ୍ତୁ q;
    }
    VRandAttr randAttr() ଅଚଳ {
        ଯଦି (m_randc) ଫେରନ୍ତୁ VRandAttr::RAND_CYCLIC;
        ଯଦି (m_rand) ଫେରନ୍ତୁ VRandAttr::RAND;
        ଫେରନ୍ତୁ VRandAttr::NONE;
    }
    ଶୂନ୍ୟ applyToNodes(Ast_ଗଣ୍ଠି_FTask* nodesp) ଅଚଳ {
        ପାଇଁ (Ast_ଗଣ୍ଠି_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), ଗଣ୍ଠି_FTask)) {
            ଯଦି (m_local) nodep->isHideLocal(true);
            ଯଦି (m_protected) nodep->isHideProtected(true);
            ଯଦି (m_static) nodep->isStatic(true);
            ଯଦି (m_virtual) nodep->isVirtual(true);
            ଯଦି (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    ଶୂନ୍ୟ applyToNodes(AstTypedef* nodep) ଅଚଳ {
        ଯଦି (m_local) nodep->isHideLocal(true);
        ଯଦି (m_protected) nodep->isHideProtected(true);
        ଯଦି (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    ଶୂନ୍ୟ applyToNodes(AstVar* nodesp) ଅଚଳ {
        ପାଇଁ (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            ଯଦି (m_rand) nodep->rand(VRandAttr::RAND);
            ଯଦି (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            ଯଦି (m_local) nodep->isHideLocal(true);
            ଯଦି (m_protected) nodep->isHideProtected(true);
            ଯଦି (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            ଯଦି (m_const) nodep->isConst(true);
            ଯଦି (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

ସଂରଚନା V3ParseBisonYYSType final {
    File_ଧାଡ଼ି* fl = nullptr;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    ସଂଘ {
        V3_ସଂଖ୍ୟା* nump;
        std::string* strp;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା cint;
        ଦ୍ୱିଗୁଣ cdouble;
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

#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, ଅଚଳ V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ଅଚଳ m_rootp;  // Root of the design
    VInFilter* ଅଚଳ m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ସ୍ଥିର V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_ଧାଡ଼ି* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_ଧାଡ଼ି* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lexKwdDepth = 0;  // Inside a `begin_keywords
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_ସଂଖ୍ୟା*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_ଧାଡ଼ି> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_ଗଣ୍ଠି* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

ସର୍ବସାଧାରଣ:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା yylexThis();
    ସ୍ଥିର bool optFuture(ଅଚଳ string& flag) { ଫେରନ୍ତୁ v3Global.opt.isFuture(flag); }

    ଶୂନ୍ୟ tagNodep(Ast_ଗଣ୍ଠି* nodep) { m_tagNodep = nodep; }
    Ast_ଗଣ୍ଠି* tagNodep() ଅଚଳ { ଫେରନ୍ତୁ m_tagNodep; }
    ଶୂନ୍ୟ lexTimescaleParse(File_ଧାଡ଼ି* fl, ଅଚଳ ଅକ୍ଷର* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_ଧାଡ଼ି* fl, bool unitSet, ଦ୍ୱିଗୁଣ unitVal, bool precSet,
                               ଦ୍ୱିଗୁଣ precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() ଅଚଳ { ଫେରନ୍ତୁ m_timeLastUnit; }
    VTimescale timeLastPrec() ଅଚଳ { ଫେରନ୍ତୁ m_timeLastPrec; }

    ଶୂନ୍ୟ lexFileline(File_ଧାଡ଼ି* fl) { m_lexFileline = fl; }
    File_ଧାଡ଼ି* lexFileline() ଅଚଳ { ଫେରନ୍ତୁ m_lexFileline; }
    std::string& lexString() { ଫେରନ୍ତୁ m_lexString; }
    ସ୍ଥିର ଶୂନ୍ୟ lexErrorPreprocDirective(File_ଧାଡ଼ି* fl, ଅଚଳ ଅକ୍ଷର* textp) VL_MT_DISABLED;
    ସ୍ଥିର string lexParseTag(ଅଚଳ ଅକ୍ଷର* textp) VL_MT_DISABLED;
    ସ୍ଥିର ଦ୍ୱିଗୁଣ lexParseTimenum(ଅଚଳ ଅକ୍ଷର* text) VL_MT_DISABLED;
    ଶୂନ୍ୟ lexPpline(ଅଚଳ ଅକ୍ଷର* textp) VL_MT_DISABLED;
    ଶୂନ୍ୟ lexVerilatorCmtLint(File_ଧାଡ଼ି* fl, ଅଚଳ ଅକ୍ଷର* textp, bool turnOff) VL_MT_DISABLED;
    ଶୂନ୍ୟ lexVerilatorCmtLintSave(ଅଚଳ File_ଧାଡ଼ି* fl) VL_MT_DISABLED;
    ଶୂନ୍ୟ lexVerilatorCmtLintRestore(File_ଧାଡ଼ି* fl) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ lexVerilatorCmtBad(File_ଧାଡ଼ି* fl, ଅଚଳ ଅକ୍ଷର* textp) VL_MT_DISABLED;

    ଶୂନ୍ୟ lex_ଠେଲ_Keywords(ପୂର୍ଣ୍ଣସଂଖ୍ୟା state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_କାଢ଼_Keywords() {
        ଯଦି (m_lexKwdDepth) {
            --m_lexKwdDepth;
            ଫେରନ୍ତୁ true;
        }
        ଫେରନ୍ତୁ false;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lexKwdLastState() ଅଚଳ { ଫେରନ୍ତୁ m_lexKwdLast; }
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* tokenName(ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok) VL_MT_DISABLED;
    ସ୍ଥିର bool isStrengthToken(ପୂର୍ଣ୍ଣସଂଖ୍ୟା tok) VL_MT_DISABLED;

    ଶୂନ୍ୟ pp_ଠେଲ_Text(ଅଚଳ string& text) {
        m_ppBuffers.ଠେଲ_back(text);
        m_ppBytes += text.length();
        ଯଦି (lexFileline()->contentp()) lexFileline()->contentp()->ଠେଲ_Text(text);
    }
    size_t pp_ନିବେଶ_To_Lex(ଅକ୍ଷର* buf, size_t max_size) VL_MT_DISABLED;

    ସ୍ଥିର V3ParseImp* parsep() { ଫେରନ୍ତୁ s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ଅଚଳ string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ଅଚଳ strp = new std::string{text};
        m_stringps.ଠେଲ_back(strp);
        ଫେରନ୍ତୁ strp;
    }
    string* newString(ଅଚଳ ଅକ୍ଷର* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ଅଚଳ strp = new std::string{text};
        m_stringps.ଠେଲ_back(strp);
        ଫେରନ୍ତୁ strp;
    }
    string* newString(ଅଚଳ ଅକ୍ଷର* text, size_t length) {
        string* ଅଚଳ strp = new string(text, length);  // Need () constructor
        m_stringps.ଠେଲ_back(strp);
        ଫେରନ୍ତୁ strp;
    }
    V3_ସଂଖ୍ୟା* new_ସଂଖ୍ୟା(File_ଧାଡ଼ି* flp, ଅଚଳ ଅକ୍ଷର* text) {
        V3_ସଂଖ୍ୟା* nump = new V3_ସଂଖ୍ୟା{flp, V3_ସଂଖ୍ୟା::Verilog_ସଂଖ୍ୟା_Literal{}, text};
        m_numberps.ଠେଲ_back(nump);
        ଫେରନ୍ତୁ nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_ଗଣ୍ଠି* newBlock(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* nodep) {
        ଯଦି (nodep) ଫେରନ୍ତୁ nodep;
        ଫେରନ୍ତୁ new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_ଧାଡ଼ି* bisonLastFileline() ଅଚଳ { ଫେରନ୍ତୁ m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ଅଚଳ { ଫେରନ୍ତୁ m_rootp; }
    bool inLibMap() ଅଚଳ { ଫେରନ୍ତୁ m_inLibMap; }
    bool inLibrary() ଅଚଳ { ଫେରନ୍ତୁ m_inLibrary; }
    string libname() ଅଚଳ { ଫେରନ୍ତୁ m_libname; }
    VOptionBool unconnectedDrive() ଅଚଳ { ଫେରନ୍ତୁ m_unconnectedDrive; }
    ଶୂନ୍ୟ unconnectedDrive(ଅଚଳ VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    ଶୂନ୍ୟ lexNew() VL_MT_DISABLED;
    ଶୂନ୍ୟ lexDestroy() VL_MT_DISABLED;
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lexPrevToken() ଅଚଳ { ଫେରନ୍ତୁ m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_ନିବେଶ_To_Lex(ଅକ୍ଷର* buf, size_t max_size) { ଫେରନ୍ତୁ pp_ନିବେଶ_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_ଧାଡ଼ି* /*fl*/) { ଫେରନ୍ତୁ parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    ଶୂନ୍ୟ parserClear() VL_MT_DISABLED;
    ଶୂନ୍ୟ lexUnputString(ଅଚଳ ଅକ୍ଷର* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    ଶୂନ୍ୟ parseFile(File_ଧାଡ଼ି* fileline, ଅଚଳ string& modfilename, bool inLibrary, bool inLibMap,
                   ଅଚଳ string& libname, ଅଚଳ string& errmsg,
                   ଅଚଳ std::string& notFoundName) VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpInputsFile() VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpTokensAhead(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଧାଡ଼ି) VL_MT_DISABLED;
    ସ୍ଥିର ଶୂନ୍ୟ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    ଶୂନ୍ୟ importIfInStd(File_ଧାଡ଼ି* fileline, ଅଚଳ string& id, bool doImport);
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* makePropertyCase(File_ଧାଡ଼ି* flp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstCaseItem* itemsp);

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ preprocDumps(std::ostream& os);
    ଶୂନ୍ୟ lexFile(ଅଚଳ string& modname) VL_MT_DISABLED;
    ଶୂନ୍ୟ yylexReadTok() VL_MT_DISABLED;
    ଶୂନ୍ୟ tokenPull() VL_MT_DISABLED;
    ଶୂନ୍ୟ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା tokenPipelineId(ପୂର୍ଣ୍ଣସଂଖ୍ୟା token) VL_MT_DISABLED;
    ଶୂନ୍ୟ tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    ଅଚଳ V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    ଶୂନ୍ୟ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
