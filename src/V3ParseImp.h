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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Parse.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

എണ്ണൽ V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_PRIORITY };

എണ്ണൽ V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

എണ്ണൽ V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_PURE };

//============================================================================
// Member qualifiers

ഘടന VMemberQualifiers final {
    സംഘം {
        uint32_t m_flags;
        ഘടന {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    സ്ഥിതം VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        തിരികെ q;
    }
    സ്ഥിതം VMemberQualifiers combine(സ്ഥിരം VMemberQualifiers& a, സ്ഥിരം VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        തിരികെ q;
    }
    VRandAttr randAttr() സ്ഥിരം {
        എങ്കിൽ (m_randc) തിരികെ VRandAttr::RAND_CYCLIC;
        എങ്കിൽ (m_rand) തിരികെ VRandAttr::RAND;
        തിരികെ VRandAttr::NONE;
    }
    ശൂന്യം applyToNodes(Ast_കണ്ണി_FTask* nodesp) സ്ഥിരം {
        വേണ്ടി (Ast_കണ്ണി_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), കണ്ണി_FTask)) {
            എങ്കിൽ (m_local) nodep->isHideLocal(true);
            എങ്കിൽ (m_protected) nodep->isHideProtected(true);
            എങ്കിൽ (m_static) nodep->isStatic(true);
            എങ്കിൽ (m_virtual) nodep->isVirtual(true);
            എങ്കിൽ (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    ശൂന്യം applyToNodes(AstTypedef* nodep) സ്ഥിരം {
        എങ്കിൽ (m_local) nodep->isHideLocal(true);
        എങ്കിൽ (m_protected) nodep->isHideProtected(true);
        എങ്കിൽ (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    ശൂന്യം applyToNodes(AstVar* nodesp) സ്ഥിരം {
        വേണ്ടി (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            എങ്കിൽ (m_rand) nodep->rand(VRandAttr::RAND);
            എങ്കിൽ (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            എങ്കിൽ (m_local) nodep->isHideLocal(true);
            എങ്കിൽ (m_protected) nodep->isHideProtected(true);
            എങ്കിൽ (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            എങ്കിൽ (m_const) nodep->isConst(true);
            എങ്കിൽ (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

ഘടന V3ParseBisonYYSType final {
    File_വരി* fl = nullptr;
    പൂർണ്ണസംഖ്യ token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    സംഘം {
        V3_സംഖ്യ* nump;
        std::string* strp;
        പൂർണ്ണസംഖ്യ cint;
        ഇരട്ട cdouble;
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

#അന്തര്ഭാവ "V3Ast__gen_yystype.h"
    };
};
std::ostream& operator<<(std::ostream& os, സ്ഥിരം V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* സ്ഥിരം m_rootp;  // Root of the design
    VInFilter* സ്ഥിരം m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    സ്ഥിതം V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_വരി* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_വരി* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    പൂർണ്ണസംഖ്യ m_lexKwdDepth = 0;  // Inside a `begin_keywords
    പൂർണ്ണസംഖ്യ m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    പൂർണ്ണസംഖ്യ m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_tokenLastBison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_tokensAhead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_സംഖ്യ*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_വരി> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    size_t m_ppBytes = 0;  // Preprocessor->lex bytes transferred

    Ast_കണ്ണി* m_tagNodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_timeLastUnit;  // Last `timescale's unit
    VTimescale m_timeLastPrec;  // Last `timescale's precision

പൊതു:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    പൂർണ്ണസംഖ്യ yylexThis();
    സ്ഥിതം bool optFuture(സ്ഥിരം string& flag) { തിരികെ v3Global.opt.isFuture(flag); }

    ശൂന്യം tagNodep(Ast_കണ്ണി* nodep) { m_tagNodep = nodep; }
    Ast_കണ്ണി* tagNodep() സ്ഥിരം { തിരികെ m_tagNodep; }
    ശൂന്യം lexTimescaleParse(File_വരി* fl, സ്ഥിരം അക്ഷരം* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_വരി* fl, bool unitSet, ഇരട്ട unitVal, bool precSet,
                               ഇരട്ട precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() സ്ഥിരം { തിരികെ m_timeLastUnit; }
    VTimescale timeLastPrec() സ്ഥിരം { തിരികെ m_timeLastPrec; }

    ശൂന്യം lexFileline(File_വരി* fl) { m_lexFileline = fl; }
    File_വരി* lexFileline() സ്ഥിരം { തിരികെ m_lexFileline; }
    std::string& lexString() { തിരികെ m_lexString; }
    സ്ഥിതം ശൂന്യം lexErrorPreprocDirective(File_വരി* fl, സ്ഥിരം അക്ഷരം* textp) VL_MT_DISABLED;
    സ്ഥിതം string lexParseTag(സ്ഥിരം അക്ഷരം* textp) VL_MT_DISABLED;
    സ്ഥിതം ഇരട്ട lexParseTimenum(സ്ഥിരം അക്ഷരം* text) VL_MT_DISABLED;
    ശൂന്യം lexPpline(സ്ഥിരം അക്ഷരം* textp) VL_MT_DISABLED;
    ശൂന്യം lexVerilatorCmtLint(File_വരി* fl, സ്ഥിരം അക്ഷരം* textp, bool turnOff) VL_MT_DISABLED;
    ശൂന്യം lexVerilatorCmtLintSave(സ്ഥിരം File_വരി* fl) VL_MT_DISABLED;
    ശൂന്യം lexVerilatorCmtLintRestore(File_വരി* fl) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം lexVerilatorCmtBad(File_വരി* fl, സ്ഥിരം അക്ഷരം* textp) VL_MT_DISABLED;

    ശൂന്യം lex_തള്ളുക_Keywords(പൂർണ്ണസംഖ്യ state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_എടുക്കുക_Keywords() {
        എങ്കിൽ (m_lexKwdDepth) {
            --m_lexKwdDepth;
            തിരികെ true;
        }
        തിരികെ false;
    }
    പൂർണ്ണസംഖ്യ lexKwdLastState() സ്ഥിരം { തിരികെ m_lexKwdLast; }
    സ്ഥിതം സ്ഥിരം അക്ഷരം* tokenName(പൂർണ്ണസംഖ്യ tok) VL_MT_DISABLED;
    സ്ഥിതം bool isStrengthToken(പൂർണ്ണസംഖ്യ tok) VL_MT_DISABLED;

    ശൂന്യം pp_തള്ളുക_Text(സ്ഥിരം string& text) {
        m_ppBuffers.തള്ളുക_back(text);
        m_ppBytes += text.length();
        എങ്കിൽ (lexFileline()->contentp()) lexFileline()->contentp()->തള്ളുക_Text(text);
    }
    size_t pp_നിവേശം_To_Lex(അക്ഷരം* buf, size_t max_size) VL_MT_DISABLED;

    സ്ഥിതം V3ParseImp* parsep() { തിരികെ s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(സ്ഥിരം string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* സ്ഥിരം strp = new std::string{text};
        m_stringps.തള്ളുക_back(strp);
        തിരികെ strp;
    }
    string* newString(സ്ഥിരം അക്ഷരം* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* സ്ഥിരം strp = new std::string{text};
        m_stringps.തള്ളുക_back(strp);
        തിരികെ strp;
    }
    string* newString(സ്ഥിരം അക്ഷരം* text, size_t length) {
        string* സ്ഥിരം strp = new string(text, length);  // Need () constructor
        m_stringps.തള്ളുക_back(strp);
        തിരികെ strp;
    }
    V3_സംഖ്യ* new_സംഖ്യ(File_വരി* flp, സ്ഥിരം അക്ഷരം* text) {
        V3_സംഖ്യ* nump = new V3_സംഖ്യ{flp, V3_സംഖ്യ::Verilog_സംഖ്യ_Literal{}, text};
        m_numberps.തള്ളുക_back(nump);
        തിരികെ nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_കണ്ണി* newBlock(File_വരി* fl, Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep) തിരികെ nodep;
        തിരികെ new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_വരി* bisonLastFileline() സ്ഥിരം { തിരികെ m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() സ്ഥിരം { തിരികെ m_rootp; }
    bool inLibMap() സ്ഥിരം { തിരികെ m_inLibMap; }
    bool inLibrary() സ്ഥിരം { തിരികെ m_inLibrary; }
    string libname() സ്ഥിരം { തിരികെ m_libname; }
    VOptionBool unconnectedDrive() സ്ഥിരം { തിരികെ m_unconnectedDrive; }
    ശൂന്യം unconnectedDrive(സ്ഥിരം VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    പൂർണ്ണസംഖ്യ bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    ശൂന്യം lexNew() VL_MT_DISABLED;
    ശൂന്യം lexDestroy() VL_MT_DISABLED;
    സ്ഥിതം പൂർണ്ണസംഖ്യ stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    പൂർണ്ണസംഖ്യ lexPrevToken() സ്ഥിരം { തിരികെ m_lexPrevToken; }  // Parser -> lexer communication
    size_t flex_Pp_നിവേശം_To_Lex(അക്ഷരം* buf, size_t max_size) { തിരികെ pp_നിവേശം_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_വരി* /*fl*/) { തിരികെ parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lexKwdLast = stateVerilogRecent();
        m_timeLastUnit = v3Global.opt.timeDefaultUnit();
        m_timeLastPrec = v3Global.opt.timeDefaultPrec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    ശൂന്യം parserClear() VL_MT_DISABLED;
    ശൂന്യം lexUnputString(സ്ഥിരം അക്ഷരം* textp, size_t length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    പൂർണ്ണസംഖ്യ tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    ശൂന്യം parseFile(File_വരി* fileline, സ്ഥിരം string& modfilename, bool inLibrary, bool inLibMap,
                   സ്ഥിരം string& libname, സ്ഥിരം string& errmsg,
                   സ്ഥിരം std::string& notFoundName) VL_MT_DISABLED;
    ശൂന്യം dumpInputsFile() VL_MT_DISABLED;
    ശൂന്യം dumpTokensAhead(പൂർണ്ണസംഖ്യ വരി) VL_MT_DISABLED;
    സ്ഥിതം ശൂന്യം candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    ശൂന്യം importIfInStd(File_വരി* fileline, സ്ഥിരം string& id, bool doImport);
    Ast_കണ്ണി_വ്യഞ്ജകം* makePropertyCase(File_വരി* flp, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, AstCaseItem* itemsp);

സ്വകാര്യ:
    ശൂന്യം preprocDumps(std::ostream& os);
    ശൂന്യം lexFile(സ്ഥിരം string& modname) VL_MT_DISABLED;
    ശൂന്യം yylexReadTok() VL_MT_DISABLED;
    ശൂന്യം tokenPull() VL_MT_DISABLED;
    ശൂന്യം tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    പൂർണ്ണസംഖ്യ tokenPipelineId(പൂർണ്ണസംഖ്യ token) VL_MT_DISABLED;
    ശൂന്യം tokenPipelineSym() VL_MT_DISABLED;
    size_t tokenPipeScanIdInst(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanIdType(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanBracket(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanParam(size_t depth, bool forInst) VL_MT_DISABLED;
    size_t tokenPipeScanParens(size_t depth) VL_MT_DISABLED;
    size_t tokenPipeScanEqNew(size_t depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(size_t depth) VL_MT_DISABLED;
    സ്ഥിരം V3ParseBisonYYSType* tokenPeekp(size_t depth) VL_MT_DISABLED;
    ശൂന്യം preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
