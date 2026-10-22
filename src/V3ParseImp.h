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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3PARSEIMP_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3PARSEIMP_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Parse.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

ಎಣಿಕೆ V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_ಆದ್ಯತೆ };

ಎಣಿಕೆ V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

ಎಣಿಕೆ V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_ಶುದ್ಧ };

//============================================================================
// Member qualifiers

ರಚನೆ VMemberQualifiers final {
    ಒಕ್ಕೂಟ {
        uint32_t m_flags;
        ರಚನೆ {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    ಸ್ಥಿರ VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        ಹಿಂತಿರುಗಿ q;
    }
    ಸ್ಥಿರ VMemberQualifiers combine(ಸ್ಥಿರಾಂಕ VMemberQualifiers& a, ಸ್ಥಿರಾಂಕ VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        ಹಿಂತಿರುಗಿ q;
    }
    VRandAttr randAttr() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_randc) ಹಿಂತಿರುಗಿ VRandAttr::RAND_CYCLIC;
        ಆದರೆ (m_rand) ಹಿಂತಿರುಗಿ VRandAttr::RAND;
        ಹಿಂತಿರುಗಿ VRandAttr::NONE;
    }
    ಶೂನ್ಯ apply_ಗಮ್ಯ_Nodes(Ast_ಗಂಟು_FTask* nodesp) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (Ast_ಗಂಟು_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), ಗಂಟು_FTask)) {
            ಆದರೆ (m_local) nodep->isHideLocal(true);
            ಆದರೆ (m_protected) nodep->isHideProtected(true);
            ಆದರೆ (m_static) nodep->isStatic(true);
            ಆದರೆ (m_virtual) nodep->isVirtual(true);
            ಆದರೆ (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    ಶೂನ್ಯ apply_ಗಮ್ಯ_Nodes(AstTypedef* nodep) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_local) nodep->isHideLocal(true);
        ಆದರೆ (m_protected) nodep->isHideProtected(true);
        ಆದರೆ (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    ಶೂನ್ಯ apply_ಗಮ್ಯ_Nodes(AstVar* nodesp) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            ಆದರೆ (m_rand) nodep->rand(VRandAttr::RAND);
            ಆದರೆ (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            ಆದರೆ (m_local) nodep->isHideLocal(true);
            ಆದರೆ (m_protected) nodep->isHideProtected(true);
            ಆದರೆ (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            ಆದರೆ (m_const) nodep->isConst(true);
            ಆದರೆ (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

ರಚನೆ V3ParseBisonYYSType final {
    File_ಸಾಲು* fl = nullptr;
    ಪೂರ್ಣಾಂಕ token = 0;  // Read token, aka tok
    VBaseOverride ತಳ_Override;
    bool flag = false;  // Passed up some rules
    ಒಕ್ಕೂಟ {
        V3_ಸಂಖ್ಯೆ* nump;
        std::string* strp;
        ಪೂರ್ಣಾಂಕ cint;
        ದ್ವಿಗುಣ cdouble;
        bool cbool;
        VMemberQualifiers qualifiers;
        V3UniqState uniqstate;
        V3TaggedState taggedstate;
        V3ImportProperty iprop;
        VSigning::en signstate;
        V3_Error_ಸಂಕೇತ::en errcodeen;
        VAttrType::en attrtypeen;
        VAssertType::en asserttypeen;
        VAssertDirectiveType::en assertdirectivetypeen;
        VFwdType::en fwdtype;
        VLifetime::en lifetime;
        VStrength::en strength;
        VJoinType::en joinType;

#ಅನ್ತರ್ಭಾವ "V3Ast__gen_yystype.h"
    };
};
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#ವ್ಯಾಖ್ಯಾನಿಸು YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* ಸ್ಥಿರಾಂಕ m_rootp;  // Root of the design
    VInFilter* ಸ್ಥಿರಾಂಕ m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    ಸ್ಥಿರ V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_ಸಾಲು* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_ಸಾಲು* m_bison_ಕೊನೆಯ_Fileline = nullptr;  // Filename/linenumber of last token

    bool m_ಒಳ_Library = false;  // Currently reading a library vs. regular file
    bool m_ಒಳ_Lib_Map = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    ಪೂರ್ಣಾಂಕ m_lex_Kwd_ಆಳ = 0;  // Inside a `begin_keywords
    ಪೂರ್ಣಾಂಕ m_lex_Kwd_ಕೊನೆಯ;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    ಪೂರ್ಣಾಂಕ m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_token_ಕೊನೆಯ_Bison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_ಟೋಕನುಗಳು_Ahead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_ಸಂಖ್ಯೆ*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_ಸಾಲು> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    ಗಾತ್ರ_t m_pp_ಬೈಟುಗಳು = 0;  // Preprocessor->lex bytes transferred

    Ast_ಗಂಟು* m_ಟ್ಯಾಗ್_Nodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_ಸಮಯ_ಕೊನೆಯ_Unit;  // Last `timescale's unit
    VTimescale m_ಸಮಯ_ಕೊನೆಯ_Prec;  // Last `timescale's precision

ಸಾರ್ವಜನಿಕ:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    ಪೂರ್ಣಾಂಕ yylexThis();
    ಸ್ಥಿರ bool optFuture(ಸ್ಥಿರಾಂಕ string& flag) { ಹಿಂತಿರುಗಿ v3Global.opt.isFuture(flag); }

    ಶೂನ್ಯ ಟ್ಯಾಗ್_Nodep(Ast_ಗಂಟು* nodep) { m_ಟ್ಯಾಗ್_Nodep = nodep; }
    Ast_ಗಂಟು* ಟ್ಯಾಗ್_Nodep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಟ್ಯಾಗ್_Nodep; }
    ಶೂನ್ಯ lexTimescaleParse(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_ಸಾಲು* fl, bool unit_ಗಣ, ದ್ವಿಗುಣ unitVal, bool prec_ಗಣ,
                               ದ್ವಿಗುಣ precVal) VL_MT_DISABLED;
    VTimescale ಸಮಯ_ಕೊನೆಯ_Unit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಮಯ_ಕೊನೆಯ_Unit; }
    VTimescale ಸಮಯ_ಕೊನೆಯ_Prec() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಮಯ_ಕೊನೆಯ_Prec; }

    ಶೂನ್ಯ lexFileline(File_ಸಾಲು* fl) { m_lexFileline = fl; }
    File_ಸಾಲು* lexFileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lexFileline; }
    std::string& lexString() { ಹಿಂತಿರುಗಿ m_lexString; }
    ಸ್ಥಿರ ಶೂನ್ಯ lexErrorPreprocDirective(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) VL_MT_DISABLED;
    ಸ್ಥಿರ string lex_Parse_ಟ್ಯಾಗ್(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) VL_MT_DISABLED;
    ಸ್ಥಿರ ದ್ವಿಗುಣ lexParseTimenum(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪಠ್ಯ) VL_MT_DISABLED;
    ಶೂನ್ಯ lexPpline(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) VL_MT_DISABLED;
    ಶೂನ್ಯ lexVerilatorCmtLint(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, bool turnOff) VL_MT_DISABLED;
    ಶೂನ್ಯ lexVerilatorCmtLintSave(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fl) VL_MT_DISABLED;
    ಶೂನ್ಯ lexVerilatorCmtLintRestore(File_ಸಾಲು* fl) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ lexVerilatorCmtBad(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) VL_MT_DISABLED;

    ಶೂನ್ಯ lex_ತಳ್ಳು_Keywords(ಪೂರ್ಣಾಂಕ state) {
        ++m_lex_Kwd_ಆಳ;
        m_lex_Kwd_ಕೊನೆಯ = state;
    }
    bool lex_ತೆಗೆ_Keywords() {
        ಆದರೆ (m_lex_Kwd_ಆಳ) {
            --m_lex_Kwd_ಆಳ;
            ಹಿಂತಿರುಗಿ true;
        }
        ಹಿಂತಿರುಗಿ false;
    }
    ಪೂರ್ಣಾಂಕ lex_Kwd_ಕೊನೆಯ_State() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lex_Kwd_ಕೊನೆಯ; }
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* tokenName(ಪೂರ್ಣಾಂಕ tok) VL_MT_DISABLED;
    ಸ್ಥಿರ bool isStrengthToken(ಪೂರ್ಣಾಂಕ tok) VL_MT_DISABLED;

    ಶೂನ್ಯ pp_ತಳ್ಳು_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
        m_ppBuffers.ತಳ್ಳು_ಹಿಂಭಾಗ(ಪಠ್ಯ);
        m_pp_ಬೈಟುಗಳು += ಪಠ್ಯ.ಉದ್ದಳತೆ();
        ಆದರೆ (lexFileline()->contentp()) lexFileline()->contentp()->ತಳ್ಳು_ಪಠ್ಯ(ಪಠ್ಯ);
    }
    ಗಾತ್ರ_t pp_ಒಳಹರಿವು_ಗಮ್ಯ_Lex(ಅಕ್ಷರ* buf, ಗಾತ್ರ_t max_ಗಾತ್ರ_2) VL_MT_DISABLED;

    ಸ್ಥಿರ V3ParseImp* parsep() { ಹಿಂತಿರುಗಿ s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ಸ್ಥಿರಾಂಕ strp = new std::string{ಪಠ್ಯ};
        m_stringps.ತಳ್ಳು_ಹಿಂಭಾಗ(strp);
        ಹಿಂತಿರುಗಿ strp;
    }
    string* newString(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪಠ್ಯ) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* ಸ್ಥಿರಾಂಕ strp = new std::string{ಪಠ್ಯ};
        m_stringps.ತಳ್ಳು_ಹಿಂಭಾಗ(strp);
        ಹಿಂತಿರುಗಿ strp;
    }
    string* newString(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪಠ್ಯ, ಗಾತ್ರ_t ಉದ್ದಳತೆ) {
        string* ಸ್ಥಿರಾಂಕ strp = new string(ಪಠ್ಯ, ಉದ್ದಳತೆ);  // Need () constructor
        m_stringps.ತಳ್ಳು_ಹಿಂಭಾಗ(strp);
        ಹಿಂತಿರುಗಿ strp;
    }
    V3_ಸಂಖ್ಯೆ* new_ಸಂಖ್ಯೆ(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪಠ್ಯ) {
        V3_ಸಂಖ್ಯೆ* nump = new V3_ಸಂಖ್ಯೆ{flp, V3_ಸಂಖ್ಯೆ::Verilog_ಸಂಖ್ಯೆ_Literal{}, ಪಠ್ಯ};
        m_numberps.ತಳ್ಳು_ಹಿಂಭಾಗ(nump);
        ಹಿಂತಿರುಗಿ nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_ಗಂಟು* new_ಖಂಡ(File_ಸಾಲು* fl, Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep) ಹಿಂತಿರುಗಿ nodep;
        ಹಿಂತಿರುಗಿ new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_ಸಾಲು* bison_ಕೊನೆಯ_Fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bison_ಕೊನೆಯ_Fileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_rootp; }
    bool ಒಳ_Lib_Map() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳ_Lib_Map; }
    bool ಒಳ_Library() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಳ_Library; }
    string libname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libname; }
    VOptionBool unconnectedDrive() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unconnectedDrive; }
    ಶೂನ್ಯ unconnectedDrive(ಸ್ಥಿರಾಂಕ VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    ಪೂರ್ಣಾಂಕ bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    ಶೂನ್ಯ lexNew() VL_MT_DISABLED;
    ಶೂನ್ಯ lexDestroy() VL_MT_DISABLED;
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    ಪೂರ್ಣಾಂಕ lexPrevToken() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lexPrevToken; }  // Parser -> lexer communication
    ಗಾತ್ರ_t flex_Pp_ಒಳಹರಿವು_ಗಮ್ಯ_Lex(ಅಕ್ಷರ* buf, ಗಾತ್ರ_t max_ಗಾತ್ರ_2) { ಹಿಂತಿರುಗಿ pp_ಒಳಹರಿವು_ಗಮ್ಯ_Lex(buf, max_ಗಾತ್ರ_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_ಸಾಲು* /*fl*/) { ಹಿಂತಿರುಗಿ parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lex_Kwd_ಕೊನೆಯ = stateVerilogRecent();
        m_ಸಮಯ_ಕೊನೆಯ_Unit = v3Global.opt.ಸಮಯ_Default_Unit();
        m_ಸಮಯ_ಕೊನೆಯ_Prec = v3Global.opt.ಸಮಯ_Default_Prec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    ಶೂನ್ಯ parserClear() VL_MT_DISABLED;
    ಶೂನ್ಯ lexUnputString(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಉದ್ದಳತೆ) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    ಪೂರ್ಣಾಂಕ token_ಗಮ್ಯ_Bison() VL_MT_DISABLED;  // Pass token to bison

    ಶೂನ್ಯ parseFile(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& modfilename, bool ಒಳ_Library, bool ಒಳ_Lib_Map,
                   ಸ್ಥಿರಾಂಕ string& libname, ಸ್ಥಿರಾಂಕ string& errmsg,
                   ಸ್ಥಿರಾಂಕ std::string& not_ಸಿಕ್ಕಿತು_Name) VL_MT_DISABLED;
    ಶೂನ್ಯ dumpInputsFile() VL_MT_DISABLED;
    ಶೂನ್ಯ dump_ಟೋಕನುಗಳು_Ahead(ಪೂರ್ಣಾಂಕ ಸಾಲು) VL_MT_DISABLED;
    ಸ್ಥಿರ ಶೂನ್ಯ candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    ಶೂನ್ಯ import_If_ಒಳ_Std(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& id, bool doImport);
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* makePropertyCase(File_ಸಾಲು* flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_Case_ಅಂಶ* itemsp);

ಖಾಸಗಿ:
    ಶೂನ್ಯ preprocDumps(std::ostream& os);
    ಶೂನ್ಯ lexFile(ಸ್ಥಿರಾಂಕ string& modname) VL_MT_DISABLED;
    ಶೂನ್ಯ yylexReadTok() VL_MT_DISABLED;
    ಶೂನ್ಯ tokenPull() VL_MT_DISABLED;
    ಶೂನ್ಯ tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    ಪೂರ್ಣಾಂಕ tokenPipelineId(ಪೂರ್ಣಾಂಕ token) VL_MT_DISABLED;
    ಶೂನ್ಯ tokenPipelineSym() VL_MT_DISABLED;
    ಗಾತ್ರ_t token_Pipe_ಪರಿಶೀಲನೆ_Id_Inst(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    ಗಾತ್ರ_t token_Pipe_ಪರಿಶೀಲನೆ_Id_Type(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    ಗಾತ್ರ_t token_Pipe_ಪರಿಶೀಲನೆ_Bracket(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    ಗಾತ್ರ_t token_Pipe_ಪರಿಶೀಲನೆ_Param(ಗಾತ್ರ_t ಆಳ, bool forInst) VL_MT_DISABLED;
    ಗಾತ್ರ_t token_Pipe_ಪರಿಶೀಲನೆ_Parens(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    ಗಾತ್ರ_t token_Pipe_ಪರಿಶೀಲನೆ_Eq_New(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    bool token_Pipe_ಪರಿಶೀಲನೆ_Tagged_Follows_Primary(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    ಸ್ಥಿರಾಂಕ V3ParseBisonYYSType* tokenPeekp(ಗಾತ್ರ_t ಆಳ) VL_MT_DISABLED;
    ಶೂನ್ಯ preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
