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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Parse.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

எண்ணிக்கை V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

எண்ணிக்கை V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

எண்ணிக்கை V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

கட்டமைப்பு VMemberQualifiers final {
    ஒன்றிப்பு {
        uint32_t m_flags;
        கட்டமைப்பு {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    நிலையான VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        திரும்பு q;
    }
    நிலையான VMemberQualifiers combine(மாறிலி VMemberQualifiers& a, மாறிலி VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        திரும்பு q;
    }
    VRandAttr randAttr() மாறிலி {
        எனில் (m_randc) திரும்பு VRandAttr::RAND_CYCLIC;
        எனில் (m_rand) திரும்பு VRandAttr::RAND;
        திரும்பு VRandAttr::NONE;
    }
    வெறுமை applyToNodes(Ast_முனை_FTask* nodesp) மாறிலி {
        க்கு (Ast_முனை_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), முனை_FTask)) {
            எனில் (m_local) nodep->isHideLocal(true);
            எனில் (m_protected) nodep->isHideProtected(true);
            எனில் (m_static) nodep->isStatic(true);
            எனில் (m_virtual) nodep->isVirtual(true);
            எனில் (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    வெறுமை applyToNodes(AstTypedef* nodep) மாறிலி {
        எனில் (m_local) nodep->isHideLocal(true);
        எனில் (m_protected) nodep->isHideProtected(true);
        எனில் (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    வெறுமை applyToNodes(AstVar* nodesp) மாறிலி {
        க்கு (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            எனில் (m_rand) nodep->rand(VRandAttr::RAND);
            எனில் (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            எனில் (m_local) nodep->isHideLocal(true);
            எனில் (m_protected) nodep->isHideProtected(true);
            எனில் (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            எனில் (m_const) nodep->isConst(true);
            எனில் (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

கட்டமைப்பு V3ParseBisonYYSType final {
    File_வரி* fl = nullptr;
    முழு token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    ஒன்றிப்பு {
        V3_எண்* nump;
        std::string* strp;
        முழு cint;
        இரட்டை cdouble;
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

#அந்தர்பாவ "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, மாறிலி V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* மாறிலி m_rootp;  // Root of the design
    VInFilter* மாறிலி m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    நிலையான V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_வரி* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_வரி* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    முழு m_lexKwdDepth = 0;  // Inside a `begin_keywords
    முழு m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    முழு m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_எண்*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_வரி> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_முனை* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

பொது:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    முழு yylexThis();
    நிலையான bool optFuture(மாறிலி string& flag) { திரும்பு v3Global.opt.isFuture(flag); }

    வெறுமை tagNodep(Ast_முனை* nodep) { m_tagNodep = nodep; }
    Ast_முனை* tagNodep() மாறிலி { திரும்பு m_tagNodep; }
    வெறுமை lexTimescaleParse(File_வரி* fl, மாறிலி எழுத்து* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_வரி* fl, bool unitSet, இரட்டை unitVal, bool precSet,
                               இரட்டை precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() மாறிலி { திரும்பு m_timeLastUnit; }
    VTimescale timeLastPrec() மாறிலி { திரும்பு m_timeLastPrec; }

    வெறுமை lexFileline(File_வரி* fl) { m_lexFileline = fl; }
    File_வரி* lexFileline() மாறிலி { திரும்பு m_lexFileline; }
    std::string& lexString() { திரும்பு m_lexString; }
    நிலையான வெறுமை lexErrorPreprocDirective(File_வரி* fl, மாறிலி எழுத்து* textp) VL_MT_DISABLED;
    நிலையான string lexParseTag(மாறிலி எழுத்து* textp) VL_MT_DISABLED;
    நிலையான இரட்டை lexParseTimenum(மாறிலி எழுத்து* text) VL_MT_DISABLED;
    வெறுமை lexPpline(மாறிலி எழுத்து* textp) VL_MT_DISABLED;
    வெறுமை lexVerilatorCmtLint(File_வரி* fl, மாறிலி எழுத்து* textp, bool turnOff) VL_MT_DISABLED;
    வெறுமை lexVerilatorCmtLintSave(மாறிலி File_வரி* fl) VL_MT_DISABLED;
    வெறுமை lexVerilatorCmtLintRestore(File_வரி* fl) VL_MT_DISABLED;
    நிலையான வெறுமை lexVerilatorCmtBad(File_வரி* fl, மாறிலி எழுத்து* textp) VL_MT_DISABLED;

    வெறுமை lex_தள்ளு_Keywords(முழு state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_எடு_Keywords() {
        எனில் (m_lexKwdDepth) {
            --m_lexKwdDepth;
            திரும்பு true;
        }
        திரும்பு false;
    }
    முழு lexKwdLastState() மாறிலி { திரும்பு m_lexKwdLast; }
    நிலையான மாறிலி எழுத்து* tokenName(முழு tok) VL_MT_DISABLED;
    நிலையான bool isStrengthToken(முழு tok) VL_MT_DISABLED;

    வெறுமை pp_தள்ளு_Text(மாறிலி string& text) {
        m_ppBuffers.தள்ளு_back(text);
        m_ppBytes += text.length();
        எனில் (lexFileline()->contentp()) lexFileline()->contentp()->தள்ளு_Text(text);
    }
    size_t pp_உள்ளீடு_To_Lex(எழுத்து* buf, size_t max_size) VL_MT_DISABLED;

    நிலையான V3ParseImp* parsep() { திரும்பு s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(மாறிலி string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* மாறிலி strp = new std::string{text};
        m_stringps.தள்ளு_back(strp);
        திரும்பு strp;
    }
    string* newString(மாறிலி எழுத்து* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* மாறிலி strp = new std::string{text};
        m_stringps.தள்ளு_back(strp);
        திரும்பு strp;
    }
    string* newString(மாறிலி எழுத்து* text, size_t length) {
        string* மாறிலி strp = new string(text, length);  // Need () constructor
        m_stringps.தள்ளு_back(strp);
        திரும்பு strp;
    }
    V3_எண்* new_எண்(File_வரி* flp, மாறிலி எழுத்து* text) {
        V3_எண்* nump = new V3_எண்{flp, V3_எண்::Verilog_எண்_Literal{}, text};
        m_numberps.தள்ளு_back(nump);
        திரும்பு nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_முனை* newBlock(File_வரி* fl, Ast_முனை* nodep) {
        எனில் (nodep) திரும்பு nodep;
        திரும்பு new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_வரி* bisonLastFileline() மாறிலி { திரும்பு m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() மாறிலி { திரும்பு m_rootp; }
    bool inLibMap() மாறிலி { திரும்பு m_inLibMap; }
    bool inLibrary() மாறிலி { திரும்பு m_inLibrary; }
    string libname() மாறிலி { திரும்பு m_libname; }
    VOptionBool unconnectedDrive() மாறிலி { திரும்பு m_unconnectedDrive; }
    வெறுமை unconnectedDrive(மாறிலி VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    முழு bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    வெறுமை lexNew() VL_MT_DISABLED;
    வெறுமை lexDestroy() VL_MT_DISABLED;
    நிலையான முழு stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    முழு lexPrevToken() மாறிலி { திரும்பு m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_உள்ளீடு_To_Lex(எழுத்து* buf, size_t max_size) { திரும்பு pp_உள்ளீடு_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_வரி* /*fl*/) { திரும்பு parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    வெறுமை parserClear() VL_MT_DISABLED;
    வெறுமை lexUnputString(மாறிலி எழுத்து* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    முழு tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    வெறுமை parseFile(File_வரி* fileline, மாறிலி string& modfilename, bool inLibrary, bool inLibMap,
                   மாறிலி string& libname, மாறிலி string& errmsg,
                   மாறிலி std::string& notFoundName) VL_MT_DISABLED;
    வெறுமை dumpInputsFile() VL_MT_DISABLED;
    வெறுமை dumpTokensAhead(முழு வரி) VL_MT_DISABLED;
    நிலையான வெறுமை candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    வெறுமை importIfInStd(File_வரி* fileline, மாறிலி string& id, bool doImport);
    Ast_முனை_கோவை* makePropertyCase(File_வரி* flp, Ast_முனை_கோவை* exprp, AstCaseItem* itemsp);

தனிப்பட்ட:
    வெறுமை preprocDumps(std::ostream& os);
    வெறுமை lexFile(மாறிலி string& modname) VL_MT_DISABLED;
    வெறுமை yylexReadTok() VL_MT_DISABLED;
    வெறுமை tokenPull() VL_MT_DISABLED;
    வெறுமை tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    முழு tokenPipelineId(முழு token) VL_MT_DISABLED;
    வெறுமை tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    மாறிலி V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    வெறுமை preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
