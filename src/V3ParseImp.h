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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Parse.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

গণনা V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

গণনা V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

গণনা V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

সংরচনা VMemberQualifiers final {
    সঙ্ঘ {
        uint32_t m_flags;
        সংরচনা {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    স্থির VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        প্রত্যাগম q;
    }
    স্থির VMemberQualifiers combine(ধ্রুব VMemberQualifiers& a, ধ্রুব VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        প্রত্যাগম q;
    }
    VRandAttr randAttr() ধ্রুব {
        যদি (m_randc) প্রত্যাগম VRandAttr::RAND_CYCLIC;
        যদি (m_rand) প্রত্যাগম VRandAttr::RAND;
        প্রত্যাগম VRandAttr::NONE;
    }
    শূন্য applyToNodes(Ast_গ্রন্থি_FTask* nodesp) ধ্রুব {
        প্রতি (Ast_গ্রন্থি_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), গ্রন্থি_FTask)) {
            যদি (m_local) nodep->isHideLocal(true);
            যদি (m_protected) nodep->isHideProtected(true);
            যদি (m_static) nodep->isStatic(true);
            যদি (m_virtual) nodep->isVirtual(true);
            যদি (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    শূন্য applyToNodes(AstTypedef* nodep) ধ্রুব {
        যদি (m_local) nodep->isHideLocal(true);
        যদি (m_protected) nodep->isHideProtected(true);
        যদি (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    শূন্য applyToNodes(AstVar* nodesp) ধ্রুব {
        প্রতি (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            যদি (m_rand) nodep->rand(VRandAttr::RAND);
            যদি (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            যদি (m_local) nodep->isHideLocal(true);
            যদি (m_protected) nodep->isHideProtected(true);
            যদি (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            যদি (m_const) nodep->isConst(true);
            যদি (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

সংরচনা V3ParseBisonYYSType final {
    File_সারি* fl = nullptr;
    পূর্ণাঙ্ক token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    সঙ্ঘ {
        V3_সংখ্যা* nump;
        std::string* strp;
        পূর্ণাঙ্ক cint;
        দ্বিগুণ cdouble;
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

#অন্তর্ভাব "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, ধ্রুব V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ধ্রুব m_rootp;  // Root of the design
    VInFilter* ধ্রুব m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    স্থির V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_সারি* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_সারি* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    পূর্ণাঙ্ক m_lexKwdDepth = 0;  // Inside a `begin_keywords
    পূর্ণাঙ্ক m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    পূর্ণাঙ্ক m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_সংখ্যা*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_সারি> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_গ্রন্থি* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

সার্বজনিক:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    পূর্ণাঙ্ক yylexThis();
    স্থির bool optFuture(ধ্রুব string& flag) { প্রত্যাগম v3Global.opt.isFuture(flag); }

    শূন্য tagNodep(Ast_গ্রন্থি* nodep) { m_tagNodep = nodep; }
    Ast_গ্রন্থি* tagNodep() ধ্রুব { প্রত্যাগম m_tagNodep; }
    শূন্য lexTimescaleParse(File_সারি* fl, ধ্রুব বর্ণ* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_সারি* fl, bool unitSet, দ্বিগুণ unitVal, bool precSet,
                               দ্বিগুণ precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() ধ্রুব { প্রত্যাগম m_timeLastUnit; }
    VTimescale timeLastPrec() ধ্রুব { প্রত্যাগম m_timeLastPrec; }

    শূন্য lexFileline(File_সারি* fl) { m_lexFileline = fl; }
    File_সারি* lexFileline() ধ্রুব { প্রত্যাগম m_lexFileline; }
    std::string& lexString() { প্রত্যাগম m_lexString; }
    স্থির শূন্য lexErrorPreprocDirective(File_সারি* fl, ধ্রুব বর্ণ* textp) VL_MT_DISABLED;
    স্থির string lexParseTag(ধ্রুব বর্ণ* textp) VL_MT_DISABLED;
    স্থির দ্বিগুণ lexParseTimenum(ধ্রুব বর্ণ* text) VL_MT_DISABLED;
    শূন্য lexPpline(ধ্রুব বর্ণ* textp) VL_MT_DISABLED;
    শূন্য lexVerilatorCmtLint(File_সারি* fl, ধ্রুব বর্ণ* textp, bool turnOff) VL_MT_DISABLED;
    শূন্য lexVerilatorCmtLintSave(ধ্রুব File_সারি* fl) VL_MT_DISABLED;
    শূন্য lexVerilatorCmtLintRestore(File_সারি* fl) VL_MT_DISABLED;
    স্থির শূন্য lexVerilatorCmtBad(File_সারি* fl, ধ্রুব বর্ণ* textp) VL_MT_DISABLED;

    শূন্য lex_ঠেলো_Keywords(পূর্ণাঙ্ক state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_বের_করো_Keywords() {
        যদি (m_lexKwdDepth) {
            --m_lexKwdDepth;
            প্রত্যাগম true;
        }
        প্রত্যাগম false;
    }
    পূর্ণাঙ্ক lexKwdLastState() ধ্রুব { প্রত্যাগম m_lexKwdLast; }
    স্থির ধ্রুব বর্ণ* tokenName(পূর্ণাঙ্ক tok) VL_MT_DISABLED;
    স্থির bool isStrengthToken(পূর্ণাঙ্ক tok) VL_MT_DISABLED;

    শূন্য pp_ঠেলো_Text(ধ্রুব string& text) {
        m_ppBuffers.ঠেলো_back(text);
        m_ppBytes += text.length();
        যদি (lexFileline()->contentp()) lexFileline()->contentp()->ঠেলো_Text(text);
    }
    size_t pp_প্রবেশ_To_Lex(বর্ণ* buf, size_t max_size) VL_MT_DISABLED;

    স্থির V3ParseImp* parsep() { প্রত্যাগম s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ধ্রুব string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ধ্রুব strp = new std::string{text};
        m_stringps.ঠেলো_back(strp);
        প্রত্যাগম strp;
    }
    string* newString(ধ্রুব বর্ণ* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ধ্রুব strp = new std::string{text};
        m_stringps.ঠেলো_back(strp);
        প্রত্যাগম strp;
    }
    string* newString(ধ্রুব বর্ণ* text, size_t length) {
        string* ধ্রুব strp = new string(text, length);  // Need () constructor
        m_stringps.ঠেলো_back(strp);
        প্রত্যাগম strp;
    }
    V3_সংখ্যা* new_সংখ্যা(File_সারি* flp, ধ্রুব বর্ণ* text) {
        V3_সংখ্যা* nump = new V3_সংখ্যা{flp, V3_সংখ্যা::Verilog_সংখ্যা_Literal{}, text};
        m_numberps.ঠেলো_back(nump);
        প্রত্যাগম nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_গ্রন্থি* newBlock(File_সারি* fl, Ast_গ্রন্থি* nodep) {
        যদি (nodep) প্রত্যাগম nodep;
        প্রত্যাগম new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_সারি* bisonLastFileline() ধ্রুব { প্রত্যাগম m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ধ্রুব { প্রত্যাগম m_rootp; }
    bool inLibMap() ধ্রুব { প্রত্যাগম m_inLibMap; }
    bool inLibrary() ধ্রুব { প্রত্যাগম m_inLibrary; }
    string libname() ধ্রুব { প্রত্যাগম m_libname; }
    VOptionBool unconnectedDrive() ধ্রুব { প্রত্যাগম m_unconnectedDrive; }
    শূন্য unconnectedDrive(ধ্রুব VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    পূর্ণাঙ্ক bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    শূন্য lexNew() VL_MT_DISABLED;
    শূন্য lexDestroy() VL_MT_DISABLED;
    স্থির পূর্ণাঙ্ক stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    পূর্ণাঙ্ক lexPrevToken() ধ্রুব { প্রত্যাগম m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_প্রবেশ_To_Lex(বর্ণ* buf, size_t max_size) { প্রত্যাগম pp_প্রবেশ_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_সারি* /*fl*/) { প্রত্যাগম parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    শূন্য parserClear() VL_MT_DISABLED;
    শূন্য lexUnputString(ধ্রুব বর্ণ* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    পূর্ণাঙ্ক tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    শূন্য parseFile(File_সারি* fileline, ধ্রুব string& modfilename, bool inLibrary, bool inLibMap,
                   ধ্রুব string& libname, ধ্রুব string& errmsg,
                   ধ্রুব std::string& notFoundName) VL_MT_DISABLED;
    শূন্য dumpInputsFile() VL_MT_DISABLED;
    শূন্য dumpTokensAhead(পূর্ণাঙ্ক সারি) VL_MT_DISABLED;
    স্থির শূন্য candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    শূন্য importIfInStd(File_সারি* fileline, ধ্রুব string& id, bool doImport);
    Ast_গ্রন্থি_রাশিমালা* makePropertyCase(File_সারি* flp, Ast_গ্রন্থি_রাশিমালা* exprp, AstCaseItem* itemsp);

গুপ্ত:
    শূন্য preprocDumps(std::ostream& os);
    শূন্য lexFile(ধ্রুব string& modname) VL_MT_DISABLED;
    শূন্য yylexReadTok() VL_MT_DISABLED;
    শূন্য tokenPull() VL_MT_DISABLED;
    শূন্য tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    পূর্ণাঙ্ক tokenPipelineId(পূর্ণাঙ্ক token) VL_MT_DISABLED;
    শূন্য tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    ধ্রুব V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    শূন্য preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
