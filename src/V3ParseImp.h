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
    सङ्घ {
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
        प्रत्यागम q;
    }
    स्थिर VMemberQualifiers combine(ध्रुव VMemberQualifiers& a, ध्रुव VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        प्रत्यागम q;
    }
    VRandAttr randAttr() ध्रुव {
        यदि (m_randc) प्रत्यागम VRandAttr::RAND_CYCLIC;
        यदि (m_rand) प्रत्यागम VRandAttr::RAND;
        प्रत्यागम VRandAttr::NONE;
    }
    शून्य applyToNodes(Ast_गाँठ_FTask* nodesp) ध्रुव {
        प्रति (Ast_गाँठ_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), गाँठ_FTask)) {
            यदि (m_local) nodep->isHideLocal(true);
            यदि (m_protected) nodep->isHideProtected(true);
            यदि (m_static) nodep->isStatic(true);
            यदि (m_virtual) nodep->isVirtual(true);
            यदि (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    शून्य applyToNodes(AstTypedef* nodep) ध्रुव {
        यदि (m_local) nodep->isHideLocal(true);
        यदि (m_protected) nodep->isHideProtected(true);
        यदि (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    शून्य applyToNodes(AstVar* nodesp) ध्रुव {
        प्रति (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            यदि (m_rand) nodep->अक्रम(VRandAttr::RAND);
            यदि (m_randc) nodep->अक्रम(VRandAttr::RAND_CYCLIC);
            यदि (m_local) nodep->isHideLocal(true);
            यदि (m_protected) nodep->isHideProtected(true);
            यदि (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            यदि (m_const) nodep->isConst(true);
            यदि (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

संरचना V3ParseBisonYYSType final {
    File_पंक्ति* fl = nullptr;
    पूर्णाङ्क token = 0;  // Read token, aka tok
    VBaseOverride baseOverride;
    bool flag = false;  // Passed up some rules
    सङ्घ {
        V3_संख्या* nump;
        std::string* strp;
        पूर्णाङ्क cint;
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
std::ostream& चालक<<(std::ostream& os, ध्रुव V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#define YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ध्रुव m_rootp;  // Root of the design
    VInFilter* ध्रुव m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    स्थिर V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_पंक्ति* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_पंक्ति* m_bisonLastFileline = nullptr;  // Filename/linenumber of last token

    bool m_inLibrary = false;  // Currently reading a library vs. regular file
    bool m_inLibMap = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    पूर्णाङ्क m_lexKwdDepth = 0;  // Inside a `begin_keywords
    पूर्णाङ्क m_lexKwdLast;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    पूर्णाङ्क m_lexPrevToken = 0;  // previous parsed token (for lexer)
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

सार्वजनिक:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    पूर्णाङ्क yylexThis();
    स्थिर bool optFuture(ध्रुव string& flag) { प्रत्यागम v3Global.opt.isFuture(flag); }

    शून्य tagNodep(Ast_गाँठ* nodep) { m_tagNodep = nodep; }
    Ast_गाँठ* tagNodep() ध्रुव { प्रत्यागम m_tagNodep; }
    शून्य lexTimescaleParse(File_पंक्ति* fl, ध्रुव वर्ण* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_पंक्ति* fl, bool unitSet, द्विगुण unitVal, bool precSet,
                               द्विगुण precVal) VL_MT_DISABLED;
    VTimescale timeLastUnit() ध्रुव { प्रत्यागम m_timeLastUnit; }
    VTimescale timeLastPrec() ध्रुव { प्रत्यागम m_timeLastPrec; }

    शून्य lexFileline(File_पंक्ति* fl) { m_lexFileline = fl; }
    File_पंक्ति* lexFileline() ध्रुव { प्रत्यागम m_lexFileline; }
    std::string& lexString() { प्रत्यागम m_lexString; }
    स्थिर शून्य lexErrorPreprocDirective(File_पंक्ति* fl, ध्रुव वर्ण* textp) VL_MT_DISABLED;
    स्थिर string lexParseTag(ध्रुव वर्ण* textp) VL_MT_DISABLED;
    स्थिर द्विगुण lexParseTimenum(ध्रुव वर्ण* text) VL_MT_DISABLED;
    शून्य lexPpline(ध्रुव वर्ण* textp) VL_MT_DISABLED;
    शून्य lexVerilatorCmtLint(File_पंक्ति* fl, ध्रुव वर्ण* textp, bool turnOff) VL_MT_DISABLED;
    शून्य lexVerilatorCmtLintSave(ध्रुव File_पंक्ति* fl) VL_MT_DISABLED;
    शून्य lexVerilatorCmtLintRestore(File_पंक्ति* fl) VL_MT_DISABLED;
    स्थिर शून्य lexVerilatorCmtBad(File_पंक्ति* fl, ध्रुव वर्ण* textp) VL_MT_DISABLED;

    शून्य lex_धकेलो_Keywords(पूर्णाङ्क state) {
        ++m_lexKwdDepth;
        m_lexKwdLast = state;
    }
    bool lex_निकालो_Keywords() {
        यदि (m_lexKwdDepth) {
            --m_lexKwdDepth;
            प्रत्यागम true;
        }
        प्रत्यागम false;
    }
    पूर्णाङ्क lexKwdLastState() ध्रुव { प्रत्यागम m_lexKwdLast; }
    स्थिर ध्रुव वर्ण* tokenName(पूर्णाङ्क tok) VL_MT_DISABLED;
    स्थिर bool isStrengthToken(पूर्णाङ्क tok) VL_MT_DISABLED;

    शून्य pp_धकेलो_Text(ध्रुव string& text) {
        m_ppBuffers.धकेलो_back(text);
        m_ppBytes += text.length();
        यदि (lexFileline()->contentp()) lexFileline()->contentp()->धकेलो_Text(text);
    }
    माप_प्रकार pp_निवेश_To_Lex(वर्ण* buf, माप_प्रकार max_size) VL_MT_DISABLED;

    स्थिर V3ParseImp* parsep() { प्रत्यागम s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ध्रुव string& text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ध्रुव strp = new std::string{text};
        m_stringps.धकेलो_back(strp);
        प्रत्यागम strp;
    }
    string* newString(ध्रुव वर्ण* text) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ध्रुव strp = new std::string{text};
        m_stringps.धकेलो_back(strp);
        प्रत्यागम strp;
    }
    string* newString(ध्रुव वर्ण* text, माप_प्रकार length) {
        string* ध्रुव strp = new string(text, length);  // Need () constructor
        m_stringps.धकेलो_back(strp);
        प्रत्यागम strp;
    }
    V3_संख्या* new_संख्या(File_पंक्ति* flp, ध्रुव वर्ण* text) {
        V3_संख्या* nump = new V3_संख्या{flp, V3_संख्या::Verilog_संख्या_Literal{}, text};
        m_numberps.धकेलो_back(nump);
        प्रत्यागम nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_गाँठ* newBlock(File_पंक्ति* fl, Ast_गाँठ* nodep) {
        यदि (nodep) प्रत्यागम nodep;
        प्रत्यागम new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_पंक्ति* bisonLastFileline() ध्रुव { प्रत्यागम m_bisonLastFileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ध्रुव { प्रत्यागम m_rootp; }
    bool inLibMap() ध्रुव { प्रत्यागम m_inLibMap; }
    bool inLibrary() ध्रुव { प्रत्यागम m_inLibrary; }
    string libname() ध्रुव { प्रत्यागम m_libname; }
    VOptionBool unconnectedDrive() ध्रुव { प्रत्यागम m_unconnectedDrive; }
    शून्य unconnectedDrive(ध्रुव VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    पूर्णाङ्क bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    शून्य lexNew() VL_MT_DISABLED;
    शून्य lexDestroy() VL_MT_DISABLED;
    स्थिर पूर्णाङ्क stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    पूर्णाङ्क lexPrevToken() ध्रुव { प्रत्यागम m_lexPrevToken; }  // Parser -> lexer communication
    माप_प्रकार flex_Pp_निवेश_To_Lex(वर्ण* buf, माप_प्रकार max_size) { प्रत्यागम pp_निवेश_To_Lex(buf, max_size); }

    //==== Symbol tables
    AstPackage* unitPackage(File_पंक्ति* /*fl*/) { प्रत्यागम parsep()->rootp()->dollarUnitPkgp(); }

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
    शून्य lexUnputString(ध्रुव वर्ण* textp, माप_प्रकार length) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    पूर्णाङ्क tokenToBison() VL_MT_DISABLED;  // Pass token to bison

    शून्य parseFile(File_पंक्ति* fileline, ध्रुव string& modfilename, bool inLibrary, bool inLibMap,
                   ध्रुव string& libname, ध्रुव string& errmsg,
                   ध्रुव std::string& notFoundName) VL_MT_DISABLED;
    शून्य dumpInputsFile() VL_MT_DISABLED;
    शून्य dumpTokensAhead(पूर्णाङ्क पंक्ति) VL_MT_DISABLED;
    स्थिर शून्य candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    शून्य importIfInStd(File_पंक्ति* fileline, ध्रुव string& id, bool doImport);
    Ast_गाँठ_व्यंजक* makePropertyCase(File_पंक्ति* flp, Ast_गाँठ_व्यंजक* exprp, AstCaseItem* itemsp);

गुप्त:
    शून्य preprocDumps(std::ostream& os);
    शून्य lexFile(ध्रुव string& modname) VL_MT_DISABLED;
    शून्य yylexReadTok() VL_MT_DISABLED;
    शून्य tokenPull() VL_MT_DISABLED;
    शून्य tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    पूर्णाङ्क tokenPipelineId(पूर्णाङ्क token) VL_MT_DISABLED;
    शून्य tokenPipelineSym() VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanIdInst(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanIdType(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanBracket(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanParam(माप_प्रकार depth, bool forInst) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanParens(माप_प्रकार depth) VL_MT_DISABLED;
    माप_प्रकार tokenPipeScanEqNew(माप_प्रकार depth) VL_MT_DISABLED;
    bool tokenPipeScanTaggedFollowsPrimary(माप_प्रकार depth) VL_MT_DISABLED;
    ध्रुव V3ParseBisonYYSType* tokenPeekp(माप_प्रकार depth) VL_MT_DISABLED;
    शून्य preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#endif  // Guard
