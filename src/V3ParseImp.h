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

#నిర్వచితం_కాకపోతే VERILATOR_V3PARSEIMP_H_
#నిర్వచించు VERILATOR_V3PARSEIMP_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Parse.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <deque>

class V3Lexer;

// IMPORTANT: Don't include this file other than in the bison and flex,
// as it's definitions will confuse other parsers

//======================================================================
// Types (between parser & lexer)

గణన V3UniqState : uint8_t { uniq_NONE, uniq_UNIQUE, uniq_UNIQUE0, uniq_ప్రాధాన్యత };

గణన V3TaggedState : uint8_t { tagged_NONE, tagged_SOFT, tagged_TAGGED };

గణన V3ImportProperty : uint8_t { iprop_NONE, iprop_CONTEXT, iprop_శుద్ధ };

//============================================================================
// Member qualifiers

నిర్మాణం VMemberQualifiers final {
    సంఘం {
        uint32_t m_flags;
        నిర్మాణం {
            uint32_t m_local : 1;  // Local class item (ignored until warning implemented)
            uint32_t m_protected : 1;  // Protected class item (ignored until warning implemented)
            uint32_t m_rand : 1;  // Rand property/member qualifier
            uint32_t m_randc : 1;  // Randc property/member qualifier (ignored until supported)
            uint32_t m_virtual : 1;  // Virtual property/method qualifier
            uint32_t m_const : 1;  // Const property/method qualifier
            uint32_t m_static : 1;  // Static class member
        };
    };
    స్థిర VMemberQualifiers none() {
        VMemberQualifiers q;
        q.m_flags = 0;
        తిరిగి q;
    }
    స్థిర VMemberQualifiers combine(స్థిరరాశి VMemberQualifiers& a, స్థిరరాశి VMemberQualifiers& b) {
        VMemberQualifiers q;
        q.m_flags = a.m_flags | b.m_flags;
        తిరిగి q;
    }
    VRandAttr randAttr() స్థిరరాశి {
        ఒకవేళ (m_randc) తిరిగి VRandAttr::RAND_CYCLIC;
        ఒకవేళ (m_rand) తిరిగి VRandAttr::RAND;
        తిరిగి VRandAttr::NONE;
    }
    శూన్య apply_గమ్యం_Nodes(Ast_కణుపు_FTask* nodesp) స్థిరరాశి {
        కొరకు (Ast_కణుపు_FTask* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), కణుపు_FTask)) {
            ఒకవేళ (m_local) nodep->isHideLocal(true);
            ఒకవేళ (m_protected) nodep->isHideProtected(true);
            ఒకవేళ (m_static) nodep->isStatic(true);
            ఒకవేళ (m_virtual) nodep->isVirtual(true);
            ఒకవేళ (m_const || m_rand || m_randc) {
                nodep->v3error("Syntax error: 'const'/'rand'/'randc' not allowed "
                               "before function/task declaration");
            }
        }
    }
    శూన్య apply_గమ్యం_Nodes(AstTypedef* nodep) స్థిరరాశి {
        ఒకవేళ (m_local) nodep->isHideLocal(true);
        ఒకవేళ (m_protected) nodep->isHideProtected(true);
        ఒకవేళ (m_static || m_virtual || m_rand || m_randc) {
            nodep->v3error("Syntax error: 'static'/'virtual'/'rand'/'randc' not allowed "
                           "before typedef declaration");
        }
    }
    శూన్య apply_గమ్యం_Nodes(AstVar* nodesp) స్థిరరాశి {
        కొరకు (AstVar* nodep = nodesp; nodep; nodep = VN_AS(nodep->nextp(), Var)) {
            ఒకవేళ (m_rand) nodep->rand(VRandAttr::RAND);
            ఒకవేళ (m_randc) nodep->rand(VRandAttr::RAND_CYCLIC);
            ఒకవేళ (m_local) nodep->isHideLocal(true);
            ఒకవేళ (m_protected) nodep->isHideProtected(true);
            ఒకవేళ (m_static) nodep->lifetime(VLifetime::STATIC_EXPLICIT);
            ఒకవేళ (m_const) nodep->isConst(true);
            ఒకవేళ (m_virtual) {
                nodep->v3error("Syntax error: 'virtual' not allowed before var declaration");
            }
        }
    }
};

//============================================================================
// Parser YYSType, e.g. for parser's yylval
// We can't use bison's %union as we want to pass the fileline with all tokens

నిర్మాణం V3ParseBisonYYSType final {
    File_పంక్తి* fl = nullptr;
    పూర్ణాంకం token = 0;  // Read token, aka tok
    VBaseOverride పునాది_Override;
    bool flag = false;  // Passed up some rules
    సంఘం {
        V3_సంఖ్య* nump;
        std::string* strp;
        పూర్ణాంకం cint;
        ద్విగుణ cdouble;
        bool cbool;
        VMemberQualifiers qualifiers;
        V3UniqState uniqstate;
        V3TaggedState taggedstate;
        V3ImportProperty iprop;
        VSigning::en signstate;
        V3_Error_సంకేతం::en errcodeen;
        VAttrType::en attrtypeen;
        VAssertType::en asserttypeen;
        VAssertDirectiveType::en assertdirectivetypeen;
        VFwdType::en fwdtype;
        VLifetime::en lifetime;
        VStrength::en strength;
        VJoinType::en joinType;

#అన్తర్భావ "V3Ast__gen_yystype.h"
    };
};
std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3ParseBisonYYSType& rhs) VL_MT_DISABLED;

#నిర్వచించు YYSTYPE V3ParseBisonYYSType

//######################################################################

class V3ParseImp final {
    // MEMBERS
    AstNetlist* స్థిరరాశి m_rootp;  // Root of the design
    VInFilter* స్థిరరాశి m_filterp;  // Reading filter

    V3Lexer* m_lexerp = nullptr;  // Current FlexLexer
    స్థిర V3ParseImp* s_parsep;  // Current THIS, bison() isn't class based
    File_పంక్తి* m_lexFileline = nullptr;  // Filename/linenumber currently active for lexing

    File_పంక్తి* m_bison_చివరి_Fileline = nullptr;  // Filename/linenumber of last token

    bool m_లోపల_Library = false;  // Currently reading a library vs. regular file
    bool m_లోపల_Lib_Map = false;  // Currently reading a libmap file
    string m_libname;  // Config library name (or --work)
    పూర్ణాంకం m_lex_Kwd_లోతు = 0;  // Inside a `begin_keywords
    పూర్ణాంకం m_lex_Kwd_చివరి;  // Last LEX state in `begin_keywords
    VOptionBool m_unconnectedDrive;  // Last unconnected drive

    పూర్ణాంకం m_lexPrevToken = 0;  // previous parsed token (for lexer)
    bool m_afterColonColon = false;  // The previous token was '::'
    V3ParseBisonYYSType m_token_చివరి_Bison{};  // Token we last sent to Bison
    std::deque<V3ParseBisonYYSType> m_టోకెన్లు_Ahead;  // Tokens we parsed ahead of parser

    std::deque<string*> m_stringps;  // Created strings for later cleanup
    std::deque<V3_సంఖ్య*> m_numberps;  // Created numbers for later cleanup
    std::deque<File_పంక్తి> m_lexLintState;  // Current lint state for save/restore
    std::string m_lexString;  // Text of the quoted or triple-quoted string currently being lexed
    std::deque<string> m_ppBuffers;  // Preprocessor->lex buffer of characters to process
    కొలత_t m_pp_బైట్లు = 0;  // Preprocessor->lex bytes transferred

    Ast_కణుపు* m_ట్యాగ్_Nodep = nullptr;  // Points to the node to set to m_tag or nullptr to not set.
    VTimescale m_సమయం_చివరి_Unit;  // Last `timescale's unit
    VTimescale m_సమయం_చివరి_Prec;  // Last `timescale's precision

బహిరంగ:
    VL_DEFINE_DEBUG_FUNCTIONS;
    // Note these are an exception to using the filename as the debug type
    VL_DEFINE_DEBUG(Bison);  // Define 'unsigned debugBison()'
    VL_DEFINE_DEBUG(Flex);  // Define 'unsigned debugFlex()'

    // Functions called by lex rules:
    పూర్ణాంకం yylexThis();
    స్థిర bool optFuture(స్థిరరాశి string& flag) { తిరిగి v3Global.opt.isFuture(flag); }

    శూన్య ట్యాగ్_Nodep(Ast_కణుపు* nodep) { m_ట్యాగ్_Nodep = nodep; }
    Ast_కణుపు* ట్యాగ్_Nodep() స్థిరరాశి { తిరిగి m_ట్యాగ్_Nodep; }
    శూన్య lexTimescaleParse(File_పంక్తి* fl, స్థిరరాశి అక్షరం* textp) VL_MT_DISABLED;
    AstPragma* createTimescale(File_పంక్తి* fl, bool unit_సమితి, ద్విగుణ unitVal, bool prec_సమితి,
                               ద్విగుణ precVal) VL_MT_DISABLED;
    VTimescale సమయం_చివరి_Unit() స్థిరరాశి { తిరిగి m_సమయం_చివరి_Unit; }
    VTimescale సమయం_చివరి_Prec() స్థిరరాశి { తిరిగి m_సమయం_చివరి_Prec; }

    శూన్య lexFileline(File_పంక్తి* fl) { m_lexFileline = fl; }
    File_పంక్తి* lexFileline() స్థిరరాశి { తిరిగి m_lexFileline; }
    std::string& lexString() { తిరిగి m_lexString; }
    స్థిర శూన్య lexErrorPreprocDirective(File_పంక్తి* fl, స్థిరరాశి అక్షరం* textp) VL_MT_DISABLED;
    స్థిర string lex_Parse_ట్యాగ్(స్థిరరాశి అక్షరం* textp) VL_MT_DISABLED;
    స్థిర ద్విగుణ lexParseTimenum(స్థిరరాశి అక్షరం* పాఠ్యం) VL_MT_DISABLED;
    శూన్య lexPpline(స్థిరరాశి అక్షరం* textp) VL_MT_DISABLED;
    శూన్య lexVerilatorCmtLint(File_పంక్తి* fl, స్థిరరాశి అక్షరం* textp, bool turnOff) VL_MT_DISABLED;
    శూన్య lexVerilatorCmtLintSave(స్థిరరాశి File_పంక్తి* fl) VL_MT_DISABLED;
    శూన్య lexVerilatorCmtLintRestore(File_పంక్తి* fl) VL_MT_DISABLED;
    స్థిర శూన్య lexVerilatorCmtBad(File_పంక్తి* fl, స్థిరరాశి అక్షరం* textp) VL_MT_DISABLED;

    శూన్య lex_నెట్టు_Keywords(పూర్ణాంకం state) {
        ++m_lex_Kwd_లోతు;
        m_lex_Kwd_చివరి = state;
    }
    bool lex_తీయి_Keywords() {
        ఒకవేళ (m_lex_Kwd_లోతు) {
            --m_lex_Kwd_లోతు;
            తిరిగి true;
        }
        తిరిగి false;
    }
    పూర్ణాంకం lex_Kwd_చివరి_State() స్థిరరాశి { తిరిగి m_lex_Kwd_చివరి; }
    స్థిర స్థిరరాశి అక్షరం* tokenName(పూర్ణాంకం tok) VL_MT_DISABLED;
    స్థిర bool isStrengthToken(పూర్ణాంకం tok) VL_MT_DISABLED;

    శూన్య pp_నెట్టు_పాఠ్యం(స్థిరరాశి string& పాఠ్యం) {
        m_ppBuffers.నెట్టు_వెనుక(పాఠ్యం);
        m_pp_బైట్లు += పాఠ్యం.నిడివి();
        ఒకవేళ (lexFileline()->contentp()) lexFileline()->contentp()->నెట్టు_పాఠ్యం(పాఠ్యం);
    }
    కొలత_t pp_ఆదానం_గమ్యం_Lex(అక్షరం* buf, కొలత_t max_కొలత_2) VL_MT_DISABLED;

    స్థిర V3ParseImp* parsep() { తిరిగి s_parsep; }

    // TODO: Many of these functions are the old interface; they'd be better as non-static
    // and called as READP->newString(...) etc.
    // These can be called by either parser or lexer, as not lex/parser-position aware
    string* newString(స్థిరరాశి string& పాఠ్యం) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* స్థిరరాశి strp = new std::string{పాఠ్యం};
        m_stringps.నెట్టు_వెనుక(strp);
        తిరిగి strp;
    }
    string* newString(స్థిరరాశి అక్షరం* పాఠ్యం) {
        // Allocate a string, remembering it so we can reclaim storage at lex end
        string* స్థిరరాశి strp = new std::string{పాఠ్యం};
        m_stringps.నెట్టు_వెనుక(strp);
        తిరిగి strp;
    }
    string* newString(స్థిరరాశి అక్షరం* పాఠ్యం, కొలత_t నిడివి) {
        string* స్థిరరాశి strp = new string(పాఠ్యం, నిడివి);  // Need () constructor
        m_stringps.నెట్టు_వెనుక(strp);
        తిరిగి strp;
    }
    V3_సంఖ్య* new_సంఖ్య(File_పంక్తి* flp, స్థిరరాశి అక్షరం* పాఠ్యం) {
        V3_సంఖ్య* nump = new V3_సంఖ్య{flp, V3_సంఖ్య::Verilog_సంఖ్య_Literal{}, పాఠ్యం};
        m_numberps.నెట్టు_వెనుక(nump);
        తిరిగి nump;
    }

    // For 'unique if' cannot have empty else statements as then assertion
    // would misfire
    Ast_కణుపు* new_ఖండం(File_పంక్తి* fl, Ast_కణుపు* nodep) {
        ఒకవేళ (nodep) తిరిగి nodep;
        తిరిగి new AstBegin{fl, "", nullptr, true};
    }

    // Bison sometimes needs error context without a token, so remember last token's line
    // Only use this if do not have and cannot get a token-relevant fileline
    File_పంక్తి* bison_చివరి_Fileline() స్థిరరాశి { తిరిగి m_bison_చివరి_Fileline; }

    // Return next token, for bison, since bison isn't class based, use a global THIS
    AstNetlist* rootp() స్థిరరాశి { తిరిగి m_rootp; }
    bool లోపల_Lib_Map() స్థిరరాశి { తిరిగి m_లోపల_Lib_Map; }
    bool లోపల_Library() స్థిరరాశి { తిరిగి m_లోపల_Library; }
    string libname() స్థిరరాశి { తిరిగి m_libname; }
    VOptionBool unconnectedDrive() స్థిరరాశి { తిరిగి m_unconnectedDrive; }
    శూన్య unconnectedDrive(స్థిరరాశి VOptionBool flag) { m_unconnectedDrive = flag; }

    // Interactions with parser
    పూర్ణాంకం bisonParse() VL_MT_DISABLED;

    // Interactions with lexer
    శూన్య lexNew() VL_MT_DISABLED;
    శూన్య lexDestroy() VL_MT_DISABLED;
    స్థిర పూర్ణాంకం stateVerilogRecent() VL_MT_DISABLED;  // Parser -> lexer communication
    పూర్ణాంకం lexPrevToken() స్థిరరాశి { తిరిగి m_lexPrevToken; }  // Parser -> lexer communication
    కొలత_t flex_Pp_ఆదానం_గమ్యం_Lex(అక్షరం* buf, కొలత_t max_కొలత_2) { తిరిగి pp_ఆదానం_గమ్యం_Lex(buf, max_కొలత_2); }

    //==== Symbol tables
    AstPackage* unitPackage(File_పంక్తి* /*fl*/) { తిరిగి parsep()->rootp()->dollarUnitPkgp(); }

    // CONSTRUCTORS
    V3ParseImp(AstNetlist* rootp, VInFilter* filterp)
        : m_rootp{rootp}
        , m_filterp{filterp} {
        m_lex_Kwd_చివరి = stateVerilogRecent();
        m_సమయం_చివరి_Unit = v3Global.opt.సమయం_Default_Unit();
        m_సమయం_చివరి_Prec = v3Global.opt.సమయం_Default_Prec();
    }
    ~V3ParseImp() VL_MT_DISABLED;
    శూన్య parserClear() VL_MT_DISABLED;
    శూన్య lexUnputString(స్థిరరాశి అక్షరం* textp, కొలత_t నిడివి) VL_MT_DISABLED;

    // METHODS
    // Preprocess and read the Verilog file specified into the netlist database
    పూర్ణాంకం token_గమ్యం_Bison() VL_MT_DISABLED;  // Pass token to bison

    శూన్య parseFile(File_పంక్తి* fileline, స్థిరరాశి string& modfilename, bool లోపల_Library, bool లోపల_Lib_Map,
                   స్థిరరాశి string& libname, స్థిరరాశి string& errmsg,
                   స్థిరరాశి std::string& not_దొరికింది_Name) VL_MT_DISABLED;
    శూన్య dumpInputsFile() VL_MT_DISABLED;
    శూన్య dump_టోకెన్లు_Ahead(పూర్ణాంకం పంక్తి) VL_MT_DISABLED;
    స్థిర శూన్య candidatePli(VSpellCheck* spellerp) VL_MT_DISABLED;
    శూన్య import_If_లోపల_Std(File_పంక్తి* fileline, స్థిరరాశి string& id, bool doImport);
    Ast_కణుపు_సమాసం* makePropertyCase(File_పంక్తి* flp, Ast_కణుపు_సమాసం* exprp, Ast_Case_అంశం* itemsp);

అంతరంగ:
    శూన్య preprocDumps(std::ostream& os);
    శూన్య lexFile(స్థిరరాశి string& modname) VL_MT_DISABLED;
    శూన్య yylexReadTok() VL_MT_DISABLED;
    శూన్య tokenPull() VL_MT_DISABLED;
    శూన్య tokenPipeline() VL_MT_DISABLED;  // Internal; called from tokenToBison
    పూర్ణాంకం tokenPipelineId(పూర్ణాంకం token) VL_MT_DISABLED;
    శూన్య tokenPipelineSym() VL_MT_DISABLED;
    కొలత_t token_Pipe_పరిశీలన_Id_Inst(కొలత_t లోతు) VL_MT_DISABLED;
    కొలత_t token_Pipe_పరిశీలన_Id_Type(కొలత_t లోతు) VL_MT_DISABLED;
    కొలత_t token_Pipe_పరిశీలన_Bracket(కొలత_t లోతు) VL_MT_DISABLED;
    కొలత_t token_Pipe_పరిశీలన_Param(కొలత_t లోతు, bool forInst) VL_MT_DISABLED;
    కొలత_t token_Pipe_పరిశీలన_Parens(కొలత_t లోతు) VL_MT_DISABLED;
    కొలత_t token_Pipe_పరిశీలన_Eq_New(కొలత_t లోతు) VL_MT_DISABLED;
    bool token_Pipe_పరిశీలన_Tagged_Follows_Primary(కొలత_t లోతు) VL_MT_DISABLED;
    స్థిరరాశి V3ParseBisonYYSType* tokenPeekp(కొలత_t లోతు) VL_MT_DISABLED;
    శూన్య preprocDumps(std::ostream& os, bool forInputs) VL_MT_DISABLED;
};

#అయితే_ముగింపు  // Guard
