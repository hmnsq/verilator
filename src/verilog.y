// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Bison grammar file
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// Original code here by Paul Wasson and Duane Galbi
//*************************************************************************
// clang-format off

%{
#నిర్వచితమైతే NEVER_JUST_FOR_CLANG_FORMAT
 }
#అయితే_ముగింపు
// clang-format on
#అన్తర్భావ "V3ParseGrammar.h"  // Defines YYTYPE; before including bison header

#నిర్వచించు YYERROR_VERBOSE 1  // For prior to Bison 3.6
#నిర్వచించు YYINITDEPTH 10000  // Older bisons ignore YYMAXDEPTH
#నిర్వచించు YYMAXDEPTH 10000
#నిర్వచించు YYNEWLINE "\n"

// Pick up new lexer
#నిర్వచించు yylex PARSEP->token_గమ్యం_Bison

#నిర్వచించు BBCOVERIGN(fl, సందేశం_2) (fl)->v3warn(COVERIGN, సందేశం_2)
#నిర్వచించు BBUNSUP(fl, సందేశం_2) (fl)->v3warn(E_UNSUPPORTED, సందేశం_2)
#నిర్వచించు GATEUNSUP(fl, tok) \
    { BBUNSUP((fl), "Unsupported: Verilog 1995 gate primitive: " << (tok)); }
#నిర్వచించు RISEFALLDLYUNSUP(nodep) \
    ఒకవేళ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_సమితి_True()) { \
        nodep->v3warn(RISEFALLDLY, "Unsupported: turn-off delays. Ignoring the third delay"); \
    }
#నిర్వచించు MINTYPMAXDLYUNSUP(nodep) \
    ఒకవేళ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_సమితి_True()) { \
        nodep->v3warn( \
            MINTYPMAXDLY, \
            "Unsupported: minimum/typical/maximum delay expressions. Using the typical delay"); \
    }
// Apply a delay to all continuous assignments under listp
స్థిర శూన్య DELAY_LIST(Ast_కణుపు* listp, AstDelay* delayp) {
    ఒకవేళ (!delayp) తిరిగి;
    కొరకు (Ast_కణుపు* nodep = listp; nodep; nodep = nodep->nextp()) {
        ఒకవేళ (VN_IS(nodep, Implicit)) కొనసాగు;
        AstAlways* స్థిరరాశి alwaysp = VN_AS(nodep, Always);
        Ast_కేటాయించు_W* స్థిరరాశి assignp = VN_AS(alwaysp->stmtsp(), కేటాయించు_W);
        assignp->timingControlp(delayp->backp() ? delayp->clone_వృక్షం(false) : delayp);
    }
}
// Apply a strength to all continuous assignments under listp
స్థిర శూన్య STRENGTH_LIST(Ast_కణుపు* listp, AstStrengthSpec* specp) {
    ఒకవేళ (!specp) తిరిగి;
    కొరకు (Ast_కణుపు* nodep = listp; nodep; nodep = nodep->nextp()) {
        ఒకవేళ (VN_IS(nodep, Implicit)) కొనసాగు;
        AstAlways* స్థిరరాశి alwaysp = VN_AS(nodep, Always);
        Ast_కేటాయించు_W* స్థిరరాశి assignp = VN_AS(alwaysp->stmtsp(), కేటాయించు_W);
        assignp->strengthSpecp(specp->backp() ? specp->clone_వృక్షం(false) : specp);
    }
}
//======================================================================
// Statics (for here only)

#నిర్వచించు PARSEP V3ParseImp::parsep()
#నిర్వచించు GRAMMARP V3ParseGrammar::singletonp()

స్థిరరాశి VBasicDTypeKwd LOGIC = VBasicDTypeKwd::LOGIC;  // Shorthand "LOGIC"
స్థిరరాశి VBasicDTypeKwd LOGIC_IMPLICIT = VBasicDTypeKwd::LOGIC_IMPLICIT;

//======================================================================
// Macro functions

// Only use in empty rules, so lines point at beginnings
#నిర్వచించు CRELINE() (PARSEP->bison_చివరి_Fileline()->copy_Or_Same_File_పంక్తి_Applied())
#నిర్వచించు FILELINE_OR_CRE(nodep) ((nodep) ? (nodep)->fileline() : CRELINE())

#నిర్వచించు VARRESET_LIST(decl) VARRESET__PVT(decl, 1)  // Start of pinlist
#నిర్వచించు VARRESET_NONLIST(decl) VARRESET__PVT(decl, 0);  // Not in a pinlist
#నిర్వచించు VARRESET__PVT(decl, pin_అంకె_ప్రారంభం) \
    { \
        VARDECL(decl); \
        VARIO(NONE); \
        VARDTYPE_NDECL(nullptr); \
        GRAMMARP->m_pin_అంకె = (pin_అంకె_ప్రారంభం); \
        GRAMMARP->m_varLifetime = VLifetime::NONE; \
        GRAMMARP->m_varDeclTyped = false; \
    }
#నిర్వచించు VARDECL(రకం) \
    { GRAMMARP->m_varDecl = VVarType::రకం; }
#నిర్వచించు VARIO(రకం) \
    { GRAMMARP->m_varIO = VDirection::రకం; }
// Set direction to default-input when detect inside an ANSI port list
#నిర్వచించు VARIOANSI() \
    { \
        ఒకవేళ (GRAMMARP->m_varIO == VDirection::NONE) VARIO(ఆదానం); \
    }
#నిర్వచించు VARLIFE(flag) \
    { GRAMMARP->m_varLifetime = flag; }
#నిర్వచించు VARDTYPE(dtypep) \
    { \
        GRAMMARP->సమితి_DType(dtypep); \
        GRAMMARP->m_varDeclTyped = true; \
    }
#నిర్వచించు VARDTYPE_NDECL(dtypep) \
    { GRAMMARP->సమితి_DType(dtypep); }  // Port that is range or signed only (not a decl)

#నిర్వచించు VARDONEA(fl, name, array, attrs) GRAMMARP->createVariable((fl), (name), (array), (attrs))
#నిర్వచించు VARDONEP(portp, array, attrs) \
    GRAMMARP->createVariable((portp)->fileline(), (portp)->name(), (array), (attrs))
#నిర్వచించు PINNUMINC() (GRAMMARP->m_pin_అంకె++)

#నిర్వచించు GATERANGE(rangep) \
    { GRAMMARP->m_gateRangep = rangep; }

#నిర్వచించు INSTPREP(modfl, modname, paramsp) \
    { \
        GRAMMARP->m_impliedDecl = true; \
        GRAMMARP->m_instModuleFl = modfl; \
        GRAMMARP->m_instModule = modname; \
        GRAMMARP->m_instParamp = paramsp; \
    }

#నిర్వచించు DEL(...) \
    { \
        /* cppcheck-suppress constVariable */ \
        Ast_కణుపు* స్థిరరాశి nodeps[] = {__VA_ARGS__}; \
        కొరకు (Ast_కణుపు* స్థిరరాశి nodep : nodeps) \
            ఒకవేళ (nodep) nodep->delete_వృక్షం(); \
    }

స్థిర శూన్య ERRSVKWD(File_పంక్తి* fileline, స్థిరరాశి string& tokname) {
    స్థిర పూర్ణాంకం s_toldonce = 0;
    fileline->v3error(
        "Unexpected '"s + tokname + "': '" + tokname
        + "' is a SystemVerilog keyword misused as an identifier."
        + (!s_toldonce++ ? "\n" + fileline->warn_మరిన్ని()
                               + "... Suggest modify the Verilog-2001 code to avoid SV keywords,"
                               + " or use `begin_keywords or --language."
                         : ""));
}

స్థిర శూన్య ASSIGNEQEXPR(File_పంక్తి* fileline) {
    fileline->v3warn(ASSIGNEQEXPR,
                     "Assignment '=' inside expression\n"
                         << fileline->warn_మరిన్ని()
                         << "... Was a '==' intended, or suggest use a separate statement");
}

స్థిర శూన్య UNSUPREAL(File_పంక్తి* fileline) {
    fileline->v3warn(SHORTREAL,
                     "Unsupported: shortreal being promoted to real (suggest use real instead)");
}

//======================================================================

శూన్య yyerror(స్థిరరాశి అక్షరం* errmsg) { PARSEP->bison_చివరి_Fileline()->v3error(errmsg); }

మూస <typename T_కణుపు, typename T_తదుపరి>
స్థిర T_కణుపు* కూడు_తదుపరి_Null(T_కణుపు* nodep, T_తదుపరి* nextp) {
    తిరిగి Ast_కణుపు::కూడు_తదుపరి_Null<T_కణుపు, T_తదుపరి>(nodep, nextp);
}

//======================================================================

class Ast_Sen_వృక్షం;
// clang-format off
%}

// Bison 3.0 and newer
BISONPRE_సంస్కరణ(3.0,%define parse.error verbose)

// We run bison with the -d argument. This tells it to generate a
// header file with token names. Old versions of bison pasted the
// contents of that file into the generated source as well; newer
// versions just include it.
//
// Since we run bison through ../bisonpre, it doesn't know the correct
// header file name, so we need to tell it.
BISONPRE_సంస్కరణ(3.7,%define api.శీర్షిక.include {"V3ParseBison.h"})

// When writing Bison patterns we use yTOKEN instead of "token",
// so Bison will error out on unknown "token"s.

// Generic lexer tokens, for example a number
// IEEE: real_number
%token<cdouble>         yaFLOATNUM      "FLOATING-POINT NUMBER"

// IEEE: identifier, class_identifier, class_variable_identifier,
// covergroup_variable_identifier, dynamic_array_variable_identifier,
// enum_identifier, interface_identifier, interface_instance_identifier,
// package_identifier, type_identifier, variable_identifier,
%token<strp>            yaID__ETC       "IDENTIFIER"
%token<strp>            yaID__CC        "IDENTIFIER-::"
%token<strp>            yaID__LEX       "IDENTIFIER-in-lex"
%token<strp>            yaID__PATHPULSE "IDENTIFIER-for-pathpulse"
%token<strp>            yaID__aINST     "IDENTIFIER-for-instance"
%token<strp>            yaID__aTYPE     "IDENTIFIER-for-type"
//                      Can't predecode aFUNCTION, can declare after use
//                      Can't predecode aINTERFACE, can declare after use
//                      Can't predecode aTASK, can declare after use

// IEEE: integral_number
%token<nump>            yaINTNUM        "INTEGER NUMBER"
// IEEE: time_literal + time_unit
%token<cdouble>         yaTIMENUM       "TIME NUMBER"
// IEEE: string_literal
%token<strp>            yaSTRING        "STRING"
%token<strp>            yaSTRING__IGNORE "STRING-ignored"       // Used when expr:string not allowed
// IEEE: edge_descriptor
%token<nump>            yaEDGEDESC      "EDGE DESCRIPTOR"

%token<fl>              yaTIMINGSPEC    "TIMING SPEC ELEMENT"

%token<fl>              ygenSTRENGTH    "STRENGTH keyword (strong1/etc)"

%token<strp>            ya_పట్టిక_క్షేత్రం   "UDP table field"
%token<fl>              ya_పట్టిక_LRSEP   ":"
%token<fl>              ya_పట్టిక_LINEEND "UDP table line end"

%token<strp>            yaSCCTOR        "`systemc_ctor block"
%token<strp>            yaSCDTOR        "`systemc_dtor block"
%token<strp>            yaSCHDR         "`systemc_header block"
%token<strp>            yaSCHDRP        "`systemc_header_post block"
%token<strp>            yaSCIMP         "`systemc_implementation block"
%token<strp>            yaSCIMPH        "`systemc_imp_header block"
%token<strp>            yaSCINT         "`systemc_interface block"

%token<fl>              yVLT_CLOCKER                "clocker"
%token<fl>              yVLT_CLOCK_ENABLE           "clock_enable"
%token<fl>              y_VLT_COVERAGE_ఖండం_OFF     "coverage_block_off"
%token<fl>              yVLT_COVERAGE_OFF           "coverage_off"
%token<fl>              yVLT_COVERAGE_ON            "coverage_on"
%token<fl>              yVLT_FORCEABLE              "forceable"
%token<fl>              y_VLT_FSM_రిజిస్టరు_WRAPPER   "fsm_register_wrapper"
%token<fl>              y_VLT_నిండిన_CASE              "full_case"
%token<fl>              y_VLT_HIER_ఖండం             "hier_block"
%token<fl>              yVLT_HIER_PARAMS            "hier_params"
%token<fl>              yVLT_HIER_WORKERS           "hier_workers"
%token<fl>              yVLT_INLINE                 "inline"
%token<fl>              yVLT_ISOLATE_ASSIGNMENTS    "isolate_assignments"
%token<fl>              yVLT_LINT_OFF               "lint_off"
%token<fl>              yVLT_LINT_ON                "lint_on"
%token<fl>              yVLT_NO_CLOCKER             "no_clocker"
%token<fl>              yVLT_NO_INLINE              "no_inline"
%token<fl>              yVLT_PARALLEL_CASE          "parallel_case"
%token<fl>              y_VLT_PROFILE_దత్తాంశం           "profile_data"
%token<fl>              y_VLT_ప్రజా                 "public"
%token<fl>              y_VLT_ప్రజా_FLAT            "public_flat"
%token<fl>              y_VLT_ప్రజా_FLAT_RD         "public_flat_rd"
%token<fl>              y_VLT_ప్రజా_FLAT_RW         "public_flat_rw"
%token<fl>              y_VLT_ప్రజా_MODULE          "public_module"
%token<fl>              yVLT_SC_BIGUINT             "sc_biguint"
%token<fl>              yVLT_SC_BV                  "sc_bv"
%token<fl>              yVLT_SFORMAT                "sformat"
%token<fl>              y_VLT_విభజించు_VAR              "split_var"
%token<fl>              yVLT_TIMING_OFF             "timing_off"
%token<fl>              yVLT_TIMING_ON              "timing_on"
%token<fl>              yVLT_TRACING_OFF            "tracing_off"
%token<fl>              yVLT_TRACING_ON             "tracing_on"
%token<fl>              yVLT_VERILATOR_LIB          "verilator_lib"

%token<fl>              y_VLT_D_ఖండం    "--block"
%token<fl>              yVLT_D_CONTENTS "--contents"
%token<fl>              y_VLT_D_ఖర్చు     "--cost"
%token<fl>              yVLT_D_CLOCK    "--clock"
%token<fl>              yVLT_D_D        "--d"
%token<fl>              yVLT_D_FILE     "--file"
%token<fl>              yVLT_D_FUNCTION "--function"
%token<fl>              yVLT_D_HIER_DPI "--hier-dpi"
%token<fl>              y_VLT_D_స్థాయిలు   "--levels"
%token<fl>              y_VLT_D_పంక్తులు    "--lines"
%token<fl>              y_VLT_D_సరిపోలిక    "--match"
%token<fl>              yVLT_D_MODEL    "--model"
%token<fl>              yVLT_D_MODULE   "--module"
%token<fl>              yVLT_D_MTASK    "--mtask"
%token<fl>              yVLT_D_PARAM    "--param"
%token<fl>              y_VLT_D_పోర్టు     "--port"
%token<fl>              y_VLT_D_నియమం     "--rule"
%token<fl>              yVLT_D_Q        "--q"
%token<fl>              yVLT_D_RESET    "--reset"
%token<fl>              y_VLT_D_RESET_విలువ "--reset_value"
%token<fl>              yVLT_D_SCOPE    "--scope"
%token<fl>              yVLT_D_TASK     "--task"
%token<fl>              yVLT_D_VAR      "--var"
%token<fl>              yVLT_D_WORKERS  "--workers"

%token<strp>            yaD_PLI         "${pli-system}"

%token<fl>              yaT_NOUNCONNECTED  "`nounconnecteddrive"
%token<fl>              yaT_RESETALL    "`resetall"
%token<fl>              yaT_UNCONNECTED_PULL0  "`unconnected_drive pull0"
%token<fl>              yaT_UNCONNECTED_PULL1  "`unconnected_drive pull1"

// <fl> is the fileline, abbreviated to shorten "$<fl>1" references
%token<fl>              '!'
%token<fl>              '#'
%token<fl>              '%'
%token<fl>              '&'
%token<fl>              '('  // See also yP_PAR__STRENGTH
%token<fl>              ')'
%token<fl>              '*'
%token<fl>              '+'
%token<fl>              ','
%token<fl>              '-'
%token<fl>              '.'
%token<fl>              '/'
%token<fl>              ':'  // See also yP_COLON__BEGIN or yP_COLON__FORK
%token<fl>              ';'
%token<fl>              '<'
%token<fl>              '='  // See also yP_EQ__NEW
%token<fl>              '>'
%token<fl>              '?'
%token<fl>              '@'
%token<fl>              '['
%token<fl>              ']'
%token<fl>              '^'
%token<fl>              '{'
%token<fl>              '|'
%token<fl>              '}'
%token<fl>              '~'

// Specific keywords
// yKEYWORD means match "keyword"
// Other cases are yXX_KEYWORD where XX makes it unique,
// for example yP_ for punctuation based operators.
// Double underscores "yX__Y" means token X followed by Y,
// and "yX__ETC" means X folled by everything but Y(s).
%token<fl>              y1_అడుగు          "1step"
%token<fl>              yACCEPT_ON      "accept_on"
%token<fl>              yALIAS          "alias"
%token<fl>              yALWAYS         "always"
%token<fl>              yALWAYS_COMB    "always_comb"
%token<fl>              yALWAYS_FF      "always_ff"
%token<fl>              yALWAYS_LATCH   "always_latch"
%token<fl>              yAND            "and"
%token<fl>              yASSERT         "assert"
%token<fl>              y_కేటాయించు         "assign"
%token<fl>              yASSUME         "assume"
%token<fl>              yAUTOMATIC      "automatic"
%token<fl>              yBEFORE         "before"
%token<fl>              yBEGIN          "begin"
%token<fl>              yBIND           "bind"
%token<fl>              yBINS           "bins"
%token<fl>              yBINSOF         "binsof"
%token<fl>              y_బిట్టు            "bit"
%token<fl>              yBREAK          "break"
%token<fl>              yBUF            "buf"
%token<fl>              yBUFIF0         "bufif0"
%token<fl>              yBUFIF1         "bufif1"
%token<fl>              yBYTE           "byte"
%token<fl>              yCASE           "case"
%token<fl>              yCASEX          "casex"
%token<fl>              yCASEZ          "casez"
%token<fl>              yCELL           "cell"
%token<fl>              yCHANDLE        "chandle"
%token<fl>              yCHECKER        "checker"
%token<fl>              yCLASS          "class"
%token<fl>              yCLOCKING       "clocking"
%token<fl>              yCMOS           "cmos"
%token<fl>              yCONFIG         "config"
%token<fl>              yCONSTRAINT     "constraint"
%token<fl>              yCONST__ETC     "const"
%token<fl>              yCONST__LEX     "const-in-lex"
%token<fl>              yCONST__REF     "const-then-ref"
%token<fl>              yCONTEXT        "context"
%token<fl>              yCONTINUE       "continue"
%token<fl>              yCOVER          "cover"
%token<fl>              yCOVERGROUP     "covergroup"
%token<fl>              yCOVERPOINT     "coverpoint"
%token<fl>              yCROSS          "cross"
%token<fl>              yDEASSIGN       "deassign"
%token<fl>              yDEFAULT        "default"
%token<fl>              yDEFPARAM       "defparam"
%token<fl>              yDESIGN         "design"
%token<fl>              yDISABLE        "disable"
%token<fl>              yDIST           "dist"
%token<fl>              yDO             "do"
%token<fl>              y_అంచు           "edge"
%token<fl>              yELSE           "else"
%token<fl>              y_చివర            "end"
%token<fl>              yENDCASE        "endcase"
%token<fl>              yENDCHECKER     "endchecker"
%token<fl>              yENDCLASS       "endclass"
%token<fl>              yENDCLOCKING    "endclocking"
%token<fl>              yENDCONFIG      "endconfig"
%token<fl>              yENDFUNCTION    "endfunction"
%token<fl>              yENDGENERATE    "endgenerate"
%token<fl>              yENDGROUP       "endgroup"
%token<fl>              yENDINTERFACE   "endinterface"
%token<fl>              yENDMODULE      "endmodule"
%token<fl>              yENDPACKAGE     "endpackage"
%token<fl>              yENDPRIMITIVE   "endprimitive"
%token<fl>              yENDPROGRAM     "endprogram"
%token<fl>              yENDPROPERTY    "endproperty"
%token<fl>              yENDSEQUENCE    "endsequence"
%token<fl>              yENDSPECIFY     "endspecify"
%token<fl>              yENDTABLE       "endtable"
%token<fl>              yENDTASK        "endtask"
%token<fl>              yENUM           "enum"
%token<fl>              yEVENT          "event"
%token<fl>              yEVENTUALLY     "eventually"
%token<fl>              yEXPECT         "expect"
%token<fl>              yEXPORT         "export"
%token<fl>              yEXTENDS        "extends"
%token<fl>              yEXTERN         "extern"
%token<fl>              yFINAL          "final"
%token<fl>              y_మొదటి_సరిపోలిక    "first_match"
%token<fl>              yFOR            "for"
%token<fl>              yFORCE          "force"
%token<fl>              yFOREACH        "foreach"
%token<fl>              yFOREVER        "forever"
%token<fl>              yFORK           "fork"
%token<fl>              yFORKJOIN       "forkjoin"
%token<fl>              yFUNCTION       "function"
%token<fl>              yGENERATE       "generate"
%token<fl>              yGENVAR         "genvar"
%token<fl>              yGLOBAL__CLOCKING "global-then-clocking"
%token<fl>              yGLOBAL__ETC    "global"
%token<fl>              yGLOBAL__LEX    "global-in-lex"
%token<fl>              yHIGHZ0         "highz0"
%token<fl>              yHIGHZ1         "highz1"
%token<fl>              yIF             "if"
%token<fl>              yIFF            "iff"
%token<fl>              yIGNORE_BINS    "ignore_bins"
%token<fl>              yILLEGAL_BINS   "illegal_bins"
%token<fl>              yIMPLEMENTS     "implements"
%token<fl>              y_సూచిస్తుంది        "implies"
%token<fl>              yIMPORT         "import"
%token<fl>              yINCDIR         "incdir"
%token<fl>              yINCLUDE        "include"
%token<fl>              yINITIAL        "initial"
%token<fl>              yINOUT          "inout"
%token<fl>              y_ఆదానం          "input"
%token<fl>              yINSIDE         "inside"
%token<fl>              yINSTANCE       "instance"
%token<fl>              yINT            "int"
%token<fl>              yINTEGER        "integer"
%token<fl>              yINTERCONNECT   "interconnect"
%token<fl>              yINTERFACE      "interface"
%token<fl>              yINTERSECT      "intersect"
%token<fl>              yJOIN           "join"
%token<fl>              yJOIN_ANY       "join_any"
%token<fl>              yJOIN_NONE      "join_none"
%token<fl>              yLET            "let"
%token<fl>              yLIBLIST        "liblist"
%token<fl>              yLIBRARY        "library"
%token<fl>              yLOCALPARAM     "localparam"
%token<fl>              yLOCAL__COLONCOLON "local-then-::"
%token<fl>              yLOCAL__ETC     "local"
%token<fl>              yLOCAL__LEX     "local-in-lex"
%token<fl>              yLOGIC          "logic"
%token<fl>              yLONGINT        "longint"
%token<fl>              yMATCHES        "matches"
%token<fl>              yMODPORT        "modport"
%token<fl>              yMODULE         "module"
%token<fl>              yNAND           "nand"
%token<fl>              yNEGEDGE        "negedge"
%token<fl>              yNETTYPE        "nettype"
%token<fl>              yNEW__ETC       "new"
%token<fl>              yNEW__LEX       "new-in-lex"
%token<fl>              yNEW__PAREN     "new-then-paren"
%token<fl>              yNEXTTIME       "nexttime"
%token<fl>              yNMOS           "nmos"
%token<fl>              yNOR            "nor"
%token<fl>              yNOT            "not"
%token<fl>              yNOTIF0         "notif0"
%token<fl>              yNOTIF1         "notif1"
%token<fl>              yNULL           "null"
%token<fl>              yOR             "or"
%token<fl>              yOUTPUT         "output"
%token<fl>              yPACKAGE        "package"
%token<fl>              yPACKED         "packed"
%token<fl>              yPARAMETER      "parameter"
%token<fl>              yPMOS           "pmos"
%token<fl>              yPOSEDGE        "posedge"
%token<fl>              yPRIMITIVE      "primitive"
%token<fl>              y_ప్రాధాన్యత       "priority"
%token<fl>              y_కార్యక్రమం        "program"
%token<fl>              yPROPERTY       "property"
%token<fl>              yPROTECTED      "protected"
%token<fl>              yPULL0          "pull0"
%token<fl>              yPULL1          "pull1"
%token<fl>              yPULLDOWN       "pulldown"
%token<fl>              yPULLUP         "pullup"
%token<fl>              y_శుద్ధ           "pure"
%token<fl>              yRAND           "rand"
%token<fl>              yRANDC          "randc"
%token<fl>              yRANDCASE       "randcase"
%token<fl>              yRANDOMIZE      "randomize"
%token<fl>              yRANDSEQUENCE   "randsequence"
%token<fl>              yRCMOS          "rcmos"
%token<fl>              yREAL           "real"
%token<fl>              yREALTIME       "realtime"
%token<fl>              yREF            "ref"
%token<fl>              yREG            "reg"
%token<fl>              yREJECT_ON      "reject_on"
%token<fl>              yRELEASE        "release"
%token<fl>              yREPEAT         "repeat"
%token<fl>              yRESTRICT       "restrict"
%token<fl>              yRETURN         "return"
%token<fl>              yRNMOS          "rnmos"
%token<fl>              yRPMOS          "rpmos"
%token<fl>              yRTRAN          "rtran"
%token<fl>              yRTRANIF0       "rtranif0"
%token<fl>              yRTRANIF1       "rtranif1"
%token<fl>              ySCALARED       "scalared"
%token<fl>              ySEQUENCE       "sequence"
%token<fl>              ySHORTINT       "shortint"
%token<fl>              ySHORTREAL      "shortreal"
%token<fl>              ySIGNED         "signed"
%token<fl>              ySOFT           "soft"
%token<fl>              ySOLVE          "solve"
%token<fl>              ySPECIFY        "specify"
%token<fl>              ySPECPARAM      "specparam"
%token<fl>              ySTATIC__CONSTRAINT "static-then-constraint"
%token<fl>              ySTATIC__ETC    "static"
%token<fl>              ySTATIC__LEX    "static-in-lex"
%token<fl>              ySTRING         "string"
%token<fl>              ySTRONG         "strong"
%token<fl>              ySTRONG0        "strong0"
%token<fl>              ySTRONG1        "strong1"
%token<fl>              ySTRUCT         "struct"
%token<fl>              ySUPER          "super"
%token<fl>              ySUPPLY0        "supply0"
%token<fl>              ySUPPLY1        "supply1"
%token<fl>              ySYNC_ACCEPT_ON "sync_accept_on"
%token<fl>              ySYNC_REJECT_ON "sync_reject_on"
%token<fl>              yS_ALWAYS       "s_always"
%token<fl>              yS_EVENTUALLY   "s_eventually"
%token<fl>              yS_NEXTTIME     "s_nexttime"
%token<fl>              yS_UNTIL        "s_until"
%token<fl>              yS_UNTIL_WITH   "s_until_with"
%token<fl>              y_పట్టిక          "table"
%token<fl>              yTAGGED         "tagged"
%token<fl>              yTAGGED__LEX    "tagged-in-lex"
%token<fl>              yTAGGED__NONPRIMARY   "tagged-nonprimary"
%token<fl>              yTASK           "task"
%token<fl>              yTHIS           "this"
%token<fl>              yTHROUGHOUT     "throughout"
%token<fl>              y_సమయం           "time"
%token<fl>              yTIMEPRECISION  "timeprecision"
%token<fl>              yTIMEUNIT       "timeunit"
%token<fl>              yTRAN           "tran"
%token<fl>              yTRANIF0        "tranif0"
%token<fl>              yTRANIF1        "tranif1"
%token<fl>              yTRI            "tri"
%token<fl>              yTRI0           "tri0"
%token<fl>              yTRI1           "tri1"
%token<fl>              yTRIAND         "triand"
%token<fl>              yTRIOR          "trior"
%token<fl>              yTRIREG         "trireg"
%token<fl>              yTRUE           "true"
%token<fl>              yTYPEDEF        "typedef"
%token<fl>              yTYPE__EQ       "type-then-eqneq"
%token<fl>              yTYPE__ETC      "type"
%token<fl>              yTYPE__LEX      "type-in-lex"
%token<fl>              yUNION          "union"
%token<fl>              yUNIQUE         "unique"
%token<fl>              yUNIQUE0        "unique0"
%token<fl>              yUNSIGNED       "unsigned"
%token<fl>              yUNTIL          "until"
%token<fl>              yUNTIL_WITH     "until_with"
%token<fl>              yUNTYPED        "untyped"
%token<fl>              yUSE            "use"
%token<fl>              yVAR            "var"
%token<fl>              yVECTORED       "vectored"
%token<fl>              yVIRTUAL__CLASS "virtual-then-class"
%token<fl>              yVIRTUAL__ETC   "virtual"
%token<fl>              yVIRTUAL__INTERFACE     "virtual-then-interface"
%token<fl>              yVIRTUAL__LEX   "virtual-in-lex"
%token<fl>              yVIRTUAL__anyID "virtual-then-identifier"
%token<fl>              yVOID           "void"
%token<fl>              yWAIT           "wait"
%token<fl>              y_WAIT_క్రమం     "wait_order"
%token<fl>              yWAND           "wand"
%token<fl>              yWEAK           "weak"
%token<fl>              yWEAK0          "weak0"
%token<fl>              yWEAK1          "weak1"
%token<fl>              yWHILE          "while"
%token<fl>              yWILDCARD       "wildcard"
%token<fl>              yWIRE           "wire"
%token<fl>              yWITHIN         "within"
%token<fl>              yWITH__BRA      "with-then-["
%token<fl>              yWITH__CUR      "with-then-{"
%token<fl>              yWITH__ETC      "with"
%token<fl>              yWITH__LEX      "with-in-lex"
%token<fl>              yWITH__PAREN    "with-then-("
%token<fl>              yWITH__PAREN_CUR "with-then-(-then-{"
%token<fl>              yWOR            "wor"
%token<fl>              yWREAL          "wreal"
%token<fl>              yXNOR           "xnor"
%token<fl>              yXOR            "xor"

%token<fl>              yD_ACOS         "$acos"
%token<fl>              yD_ACOSH        "$acosh"
%token<fl>              yD_ASIN         "$asin"
%token<fl>              yD_ASINH        "$asinh"
%token<fl>              yD_ASSERTCTL    "$assertcontrol"
%token<fl>              yD_ASSERTFAILOFF "$assertfailoff"
%token<fl>              yD_ASSERTFAILON "$assertfailon"
%token<fl>              yD_ASSERTKILL   "$assertkill"
%token<fl>              yD_ASSERTNONVACUOUSON  "$assertnonvacuouson"
%token<fl>              yD_ASSERTOFF    "$assertoff"
%token<fl>              yD_ASSERTON     "$asserton"
%token<fl>              yD_ASSERTPASSOFF "$assertpassoff"
%token<fl>              yD_ASSERTPASSON  "$assertpasson"
%token<fl>              yD_ASSERTVACUOUSOFF  "$assertvacuousoff"
%token<fl>              yD_ATAN         "$atan"
%token<fl>              yD_ATAN2        "$atan2"
%token<fl>              yD_ATANH        "$atanh"
%token<fl>              y_D_బిట్లు         "$bits"
%token<fl>              yD_BITSTOREAL   "$bitstoreal"
%token<fl>              yD_BITSTOSHORTREAL "$bitstoshortreal"
%token<fl>              yD_C            "$c"
%token<fl>              yD_CPURE        "$cpure"
%token<fl>              yD_CAST         "$cast"
%token<fl>              yD_CEIL         "$ceil"
%token<fl>              y_D_మారింది      "$changed"
%token<fl>              y_D_మారింది_GCLK "$changed_gclk"
%token<fl>              yD_CHANGING_GCLK  "$changing_gclk"
%token<fl>              yD_CLOG2        "$clog2"
%token<fl>              yD_COS          "$cos"
%token<fl>              yD_COSH         "$cosh"
%token<fl>              yD_COUNTBITS    "$countbits"
%token<fl>              yD_COUNTONES    "$countones"
%token<fl>              yD_DIMENSIONS   "$dimensions"
%token<fl>              yD_DISPLAY      "$display"
%token<fl>              yD_DISPLAYB     "$displayb"
%token<fl>              yD_DISPLAYH     "$displayh"
%token<fl>              yD_DISPLAYO     "$displayo"
%token<fl>              y_D_DIST_CHI_వర్గం "$dist_chi_square"
%token<fl>              yD_DIST_ERLANG  "$dist_erlang"
%token<fl>              y_D_DIST_ఘాతాంక "$dist_exponential"
%token<fl>              y_D_DIST_సాధారణ  "$dist_normal"
%token<fl>              y_D_DIST_పాయిసాన్ "$dist_poisson"
%token<fl>              yD_DIST_T       "$dist_t"
%token<fl>              y_D_DIST_ఏకరీతి "$dist_uniform"
%token<fl>              yD_DUMPALL      "$dumpall"
%token<fl>              yD_DUMPFILE     "$dumpfile"
%token<fl>              yD_DUMPFLUSH    "$dumpflush"
%token<fl>              yD_DUMPLIMIT    "$dumplimit"
%token<fl>              yD_DUMPOFF      "$dumpoff"
%token<fl>              yD_DUMPON       "$dumpon"
%token<fl>              yD_DUMPPORTS    "$dumpports"
%token<fl>              yD_DUMPVARS     "$dumpvars"
%token<fl>              yD_ERROR        "$error"
%token<fl>              yD_EXIT         "$exit"
%token<fl>              yD_EXP          "$exp"
%token<fl>              yD_FALLING_GCLK "$falling_gclk"
%token<fl>              yD_FATAL        "$fatal"
%token<fl>              yD_FCLOSE       "$fclose"
%token<fl>              yD_FDISPLAY     "$fdisplay"
%token<fl>              yD_FDISPLAYB    "$fdisplayb"
%token<fl>              yD_FDISPLAYH    "$fdisplayh"
%token<fl>              yD_FDISPLAYO    "$fdisplayo"
%token<fl>              yD_FELL         "$fell"
%token<fl>              yD_FELL_GCLK    "$fell_gclk"
%token<fl>              yD_FEOF         "$feof"
%token<fl>              yD_FERROR       "$ferror"
%token<fl>              yD_FFLUSH       "$fflush"
%token<fl>              yD_FGETC        "$fgetc"
%token<fl>              yD_FGETS        "$fgets"
%token<fl>              y_D_ముగింపు       "$finish"
%token<fl>              yD_FLOOR        "$floor"
%token<fl>              yD_FMONITOR     "$fmonitor"
%token<fl>              yD_FMONITORB    "$fmonitorb"
%token<fl>              yD_FMONITORH    "$fmonitorh"
%token<fl>              yD_FMONITORO    "$fmonitoro"
%token<fl>              yD_FOPEN        "$fopen"
%token<fl>              yD_FREAD        "$fread"
%token<fl>              yD_FREWIND      "$frewind"
%token<fl>              yD_FSCANF       "$fscanf"
%token<fl>              yD_FSEEK        "$fseek"
%token<fl>              yD_FSTROBE      "$fstrobe"
%token<fl>              yD_FSTROBEB     "$fstrobeb"
%token<fl>              yD_FSTROBEH     "$fstrobeh"
%token<fl>              yD_FSTROBEO     "$fstrobeo"
%token<fl>              yD_FTELL        "$ftell"
%token<fl>              yD_FUTURE_GCLK  "$future_gclk"
%token<fl>              yD_FWRITE       "$fwrite"
%token<fl>              yD_FWRITEB      "$fwriteb"
%token<fl>              yD_FWRITEH      "$fwriteh"
%token<fl>              yD_FWRITEO      "$fwriteo"
%token<fl>              y_D_GET_INITIAL_RANDOM_విత్తనం "$get_initial_random_seed"
%token<fl>              yD_GLOBAL_CLOCK "$global_clock"
%token<fl>              y_D_ఎగువ         "$high"
%token<fl>              yD_HYPOT        "$hypot"
%token<fl>              yD_INCREMENT    "$increment"
%token<fl>              yD_INFERRED_DISABLE "$inferred_disable"
%token<fl>              yD_INFO         "$info"
%token<fl>              yD_ISUNBOUNDED  "$isunbounded"
%token<fl>              yD_ISUNKNOWN    "$isunknown"
%token<fl>              yD_ITOR         "$itor"
%token<fl>              y_D_ఎడమ         "$left"
%token<fl>              yD_LN           "$ln"
%token<fl>              yD_LOG10        "$log10"
%token<fl>              y_D_దిగువ          "$low"
%token<fl>              yD_MONITOR      "$monitor"
%token<fl>              yD_MONITORB     "$monitorb"
%token<fl>              yD_MONITORH     "$monitorh"
%token<fl>              yD_MONITORO     "$monitoro"
%token<fl>              yD_MONITOROFF   "$monitoroff"
%token<fl>              yD_MONITORON    "$monitoron"
%token<fl>              yD_ONEHOT       "$onehot"
%token<fl>              yD_ONEHOT0      "$onehot0"
%token<fl>              yD_PAST         "$past"
%token<fl>              yD_PAST_GCLK    "$past_gclk"
%token<fl>              yD_POW          "$pow"
%token<fl>              yD_PRINTTIMESCALE "$printtimescale"
%token<fl>              yD_RANDOM       "$random"
%token<fl>              yD_READMEMB     "$readmemb"
%token<fl>              yD_READMEMH     "$readmemh"
%token<fl>              yD_REALTIME     "$realtime"
%token<fl>              yD_REALTOBITS   "$realtobits"
%token<fl>              yD_REWIND       "$rewind"
%token<fl>              y_D_కుడి        "$right"
%token<fl>              yD_RISING_GCLK  "$rising_gclk"
%token<fl>              y_D_మూలం         "$root"
%token<fl>              yD_ROSE         "$rose"
%token<fl>              yD_ROSE_GCLK    "$rose_gclk"
%token<fl>              yD_RTOI         "$rtoi"
%token<fl>              yD_SAMPLED      "$sampled"
%token<fl>              yD_SDF_ANNOTATE "$sdf_annotate"
%token<fl>              yD_SETUPHOLD    "$setuphold"
%token<fl>              yD_SFORMAT      "$sformat"
%token<fl>              yD_SFORMATF     "$sformatf"
%token<fl>              yD_SHORTREALTOBITS "$shortrealtobits"
%token<fl>              yD_SIGNED       "$signed"
%token<fl>              yD_SIN          "$sin"
%token<fl>              yD_SINH         "$sinh"
%token<fl>              y_D_కొలత         "$size"
%token<fl>              yD_SQRT         "$sqrt"
%token<fl>              yD_SSCANF       "$sscanf"
%token<fl>              yD_STABLE       "$stable"
%token<fl>              yD_STABLE_GCLK  "$stable_gclk"
%token<fl>              yD_STACKTRACE   "$stacktrace"
%token<fl>              yD_STEADY_GCLK  "$steady_gclk"
%token<fl>              yD_STIME        "$stime"
%token<fl>              yD_STOP         "$stop"
%token<fl>              yD_STROBE       "$strobe"
%token<fl>              yD_STROBEB      "$strobeb"
%token<fl>              yD_STROBEH      "$strobeh"
%token<fl>              yD_STROBEO      "$strobeo"
%token<fl>              yD_SWRITE       "$swrite"
%token<fl>              yD_SWRITEB      "$swriteb"
%token<fl>              yD_SWRITEH      "$swriteh"
%token<fl>              yD_SWRITEO      "$swriteo"
%token<fl>              yD_SYSTEM       "$system"
%token<fl>              yD_TAN          "$tan"
%token<fl>              yD_TANH         "$tanh"
%token<fl>              yD_TESTPLUSARGS "$test$plusargs"
%token<fl>              y_D_సమయం         "$time"
%token<fl>              yD_TIMEFORMAT   "$timeformat"
%token<fl>              yD_TIMEPRECISION "$timeprecision"
%token<fl>              yD_TIMEUNIT     "$timeunit"
%token<fl>              yD_TYPENAME     "$typename"
%token<fl>              yD_UNGETC       "$ungetc"
%token<fl>              yD_UNIT         "$unit"
%token<fl>              yD_UNPACKED_DIMENSIONS "$unpacked_dimensions"
%token<fl>              yD_UNSIGNED     "$unsigned"
%token<fl>              yD_URANDOM      "$urandom"
%token<fl>              yD_URANDOM_RANGE "$urandom_range"
%token<fl>              yD_VALUEPLUSARGS "$value$plusargs"
%token<fl>              yD_WARNING      "$warning"
%token<fl>              yD_WRITE        "$write"
%token<fl>              yD_WRITEB       "$writeb"
%token<fl>              yD_WRITEH       "$writeh"
%token<fl>              yD_WRITEMEMB    "$writememb"
%token<fl>              yD_WRITEMEMH    "$writememh"
%token<fl>              yD_WRITEO       "$writeo"

%token<fl>              yVL_CLOCKER               "/*verilator clocker*/"
%token<fl>              yVL_CLOCK_ENABLE          "/*verilator clock_enable*/"
%token<fl>              y_VL_COVERAGE_ఖండం_OFF    "/*verilator coverage_block_off*/"
%token<fl>              yVL_DPI_C_DECL            "/*verilator dpi_c_decl*/"
%token<fl>              yVL_FORCEABLE             "/*verilator forceable*/"
%token<fl>              y_VL_నిండిన_CASE             "/*verilator full_case*/"
%token<fl>              y_VL_HIER_ఖండం            "/*verilator hier_block*/"
%token<fl>              yVL_INLINE_MODULE         "/*verilator inline_module*/"
%token<fl>              yVL_ISOLATE_ASSIGNMENTS   "/*verilator isolate_assignments*/"
%token<fl>              yVL_NO_CLOCKER            "/*verilator no_clocker*/"
%token<fl>              yVL_NO_INLINE_MODULE      "/*verilator no_inline_module*/"
%token<fl>              yVL_NO_INLINE_TASK        "/*verilator no_inline_task*/"
%token<fl>              yVL_PARALLEL_CASE         "/*verilator parallel_case*/"
%token<fl>              y_VL_ప్రజా                "/*verilator public*/"
%token<fl>              y_VL_ప్రజా_FLAT           "/*verilator public_flat*/"
%token<fl>              y_VL_ప్రజా_FLAT_ON        "/*verilator public_flat_on*/"
%token<fl>              y_VL_ప్రజా_FLAT_RD        "/*verilator public_flat_rd*/"
%token<fl>              y_VL_ప్రజా_FLAT_RD_ON     "/*verilator public_flat_rd_on*/"
%token<fl>              y_VL_ప్రజా_FLAT_RW        "/*verilator public_flat_rw*/"
%token<fl>              y_VL_ప్రజా_FLAT_RW_ON     "/*verilator public_flat_rw_on*/"
%token<fl>              y_VL_ప్రజా_FLAT_RW_ON_SNS "/*verilator public_flat_rw_on_sns*/"
%token<fl>              y_VL_ప్రజా_ON             "/*verilator public_on*/"
%token<fl>              y_VL_ప్రజా_OFF            "/*verilator public_off*/"
%token<fl>              y_VL_ప్రజా_MODULE         "/*verilator public_module*/"
%token<fl>              yVL_SC_BIGUINT            "/*verilator sc_biguint*/"
%token<fl>              yVL_SC_BV                 "/*verilator sc_bv*/"
%token<fl>              yVL_SFORMAT               "/*verilator sformat*/"
%token<fl>              y_VL_విభజించు_VAR             "/*verilator split_var*/"
%token<fl>              yVL_FSM_ARC_INCL_COND     "/*verilator fsm_arc_include_cond*/"
%token<fl>              yVL_FSM_ARC_INCL_COND_AUTO        "/*verilator fsm_arc_include_cond_auto*/"
%token<fl>              yVL_FSM_ARC_INCL_COND_AUTO_EXPAND "/*verilator fsm_arc_include_cond_auto_expand*/"
%token<fl>              y_VL_FSM_ARC_INCL_COND_నిండిన        "/*verilator fsm_arc_include_cond_full*/"
%token<fl>              yVL_FSM_STATE             "/*verilator fsm_state*/"
%token<fl>              yVL_FSM_STATE_AUTO        "/*verilator fsm_state_auto*/"
%token<fl>              yVL_FSM_STATE_AUTO_EXPAND "/*verilator fsm_state_auto_expand*/"
%token<fl>              y_VL_FSM_STATE_నిండిన        "/*verilator fsm_state_full*/"
%token<strp>            y_VL_ట్యాగ్                   "/*verilator tag*/"
%token<fl>              yVL_UNROLL_DISABLE        "/*verilator unroll_disable*/"
%token<fl>              y_VL_UNROLL_నిండిన           "/*verilator unroll_full*/"

%token<fl>              yP_TICK         "'"
%token<fl>              yP_TICKBRA      "'{"
%token<fl>              yP_OROR         "||"
%token<fl>              yP_ANDAND       "&&"
%token<fl>              yP_NOR          "~|"
%token<fl>              yP_XNOR         "^~"
%token<fl>              yP_NAND         "~&"
%token<fl>              yP_EQUAL        "=="
%token<fl>              yP_NOTEQUAL     "!="
%token<fl>              yP_CASEEQUAL    "==="
%token<fl>              yP_CASENOTEQUAL "!=="
%token<fl>              yP_WILDEQUAL    "==?"
%token<fl>              yP_WILDNOTEQUAL "!=?"
%token<fl>              yP_GTE          ">="
%token<fl>              yP_LTE          "<="
%token<fl>              yP_LTE__IGNORE  "<=-ignored"    // Used when expr:<= means assignment
%token<fl>              yP_SLEFT        "<<"
%token<fl>              yP_SRIGHT       ">>"
%token<fl>              yP_SSRIGHT      ">>>"
%token<fl>              yP_POW          "**"

%token<fl>              yP_COLON__BEGIN ":-then-begin"
%token<fl>              yP_COLON__FORK  ":-then-fork"
%token<fl>              yP_EQ__NEW      "=-then-new"
%token<fl>              yP_PAR__IGNORE  "(-ignored"     // Used when sequence_expr:expr:( is ignored
%token<fl>              yP_PAR__STRENGTH "(-for-strength"

%token<fl>              yP_LTMINUSGT    "<->"
%token<fl>              yP_PLUSCOLON    "+:"
%token<fl>              yP_MINUSCOLON   "-:"
%token<fl>              yP_MINUSGT      "->"
%token<fl>              yP_MINUSGTGT    "->>"
%token<fl>              yP_EQGT         "=>"
%token<fl>              yP_ASTGT        "*>"
%token<fl>              yP_ANDANDAND    "&&&"
%token<fl>              yP_POUNDPOUND   "##"
%token<fl>              yP_POUNDMINUSPD "#-#"
%token<fl>              yP_POUNDEQPD    "#=#"
%token<fl>              yP_DOTSTAR      ".*"

%token<fl>              yP_ATAT         "@@"
%token<fl>              yP_COLONCOLON   "::"
%token<fl>              yP_COLONEQ      ":="
%token<fl>              yP_COLONDIV     ":/"
%token<fl>              yP_ORMINUSGT    "|->"
%token<fl>              yP_OREQGT       "|=>"
%token<fl>              yP_BRASTAR      "[*"
%token<fl>              yP_BRAEQ        "[="
%token<fl>              yP_BRAMINUSGT   "[->"
%token<fl>              yP_BRAPLUSKET   "[+]"

%token<fl>              yP_PLUSPLUS     "++"
%token<fl>              yP_MINUSMINUS   "--"
%token<fl>              yP_PLUSEQ       "+="
%token<fl>              yP_MINUSEQ      "-="
%token<fl>              yP_TIMESEQ      "*="
%token<fl>              yP_DIVEQ        "/="
%token<fl>              yP_MODEQ        "%="
%token<fl>              yP_ANDEQ        "&="
%token<fl>              yP_OREQ         "|="
%token<fl>              yP_XOREQ        "^="
%token<fl>              yP_SLEFTEQ      "<<="
%token<fl>              yP_SRIGHTEQ     ">>="
%token<fl>              yP_SSRIGHTEQ    ">>>="

%token<fl>              yP_PLUSSLASHMINUS "+/-"
%token<fl>              yP_PLUSPCTMINUS "+%-"

// [* is not an operator, as "[ * ]" is legal
// [= and [-> could be repetition operators, but to match [* we don't add them.
// '( is not an operator, as "' (" is legal

//********************
// Verilog op precedence
//UNSUP %token<fl>      prUNARYARITH
//UNSUP %token<fl>      prREDUCTION
//UNSUP %token<fl>      prNEGATION
//UNSUP %token<fl>      prEVENTBEGIN
%token<fl>      prTAGGED

// These prevent other conflicts
%ఎడమ           yP_ANDANDAND
%ఎడమ           yMATCHES
%ఎడమ           prTAGGED
//UNSUP %left   prSEQ_CLOCKING

// PSL op precedence

// Lowest precedence
// These are in IEEE 17.7.1
%nonassoc       prALWAYS prS_ALWAYS prEVENTUALLY prS_EVENTUALLY prIF prACCEPT_ON prREJECT_ON prSYNC_ACCEPT_ON prSYNC_REJECT_ON

%కుడి          yP_ORMINUSGT yP_OREQGT yP_POUNDMINUSPD yP_POUNDEQPD
%కుడి          yUNTIL yS_UNTIL yUNTIL_WITH yS_UNTIL_WITH y_సూచిస్తుంది
%కుడి          yIFF
%ఎడమ           yOR
%ఎడమ           yAND
%nonassoc       yNOT yNEXTTIME yS_NEXTTIME
%ఎడమ           yINTERSECT
%ఎడమ           yWITHIN
%కుడి          yTHROUGHOUT
%ఎడమ           prPOUNDPOUND_MULTI
%ఎడమ           yP_POUNDPOUND
%ఎడమ           yP_BRASTAR yP_BRAEQ yP_BRAMINUSGT yP_BRAPLUSKET

// Not specified, but needed higher than yOR, lower than normal non-pexpr expressions
//UNSUP %left           yPOSEDGE yNEGEDGE yEDGE

//UNSUP %left           '{' '}'

// Verilog op precedence
%కుడి          yP_MINUSGT yP_LTMINUSGT
%కుడి          '?' ':' yP_COLON__BEGIN yP_COLON__FORK
%ఎడమ           yP_OROR
%ఎడమ           yP_ANDAND
%ఎడమ           '|' yP_NOR
%ఎడమ           '^' yP_XNOR
%ఎడమ           '&' yP_NAND
%ఎడమ           yP_EQUAL yP_NOTEQUAL yP_CASEEQUAL yP_CASENOTEQUAL yP_WILDEQUAL yP_WILDNOTEQUAL
%ఎడమ           '>' '<' yP_GTE yP_LTE yP_LTE__IGNORE yINSIDE yDIST
%ఎడమ           yP_SLEFT yP_SRIGHT yP_SSRIGHT
%ఎడమ           '+' '-'
%ఎడమ           '*' '/' '%'
%ఎడమ           yP_POW
%ఎడమ           prUNARYARITH yP_MINUSMINUS yP_PLUSPLUS prREDUCTION prNEGATION
%ఎడమ           '.'
// Not in IEEE, but need to avoid conflicts; TICK should bind tightly just lower than COLONCOLON
%ఎడమ           yP_TICK
//%left         '(' ')' '[' ']' yP_COLONCOLON '.'

%nonassoc pr_క్రింది_THAN_ELSE
%nonassoc yELSE

//BISONPRE_TYPES
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion
//  Blank lines for type insertion

%start ఆధారం_పాఠ్యం

%%
//**********************************************************************
// Files

ఆధారం_పాఠ్యం:                    // ==IEEE: source_text
                /*ఖాళీలు*/                             { }
        //                      // timeunits_declaration moved into description:package_item
        |       descriptionList                         { }
        ;

descriptionList:                // IEEE: part of source_text
                description                             { }
        |       descriptionList description             { }
        ;

description:                    // ==IEEE: description
                module_declaration                      { }
        //                      // udp_declaration moved into module_declaration
        //                      // library_declaration and include_statement moved from library_description
        |       library_declaration                     { PARSEP->rootp()->కూడు_Miscsp($1); }
        |       include_statement                       { }
        |       interface_declaration                   { }
        |       కార్యక్రమం_declaration                     { }
        |       package_declaration                     { }
        |       package_అంశం_శిఖరం                         { ఒకవేళ ($1) PARSEP->unitPackage($1->fileline())->కూడు_Stmtsp($1); }
        |       bind_directive                          { ఒకవేళ ($1) PARSEP->unitPackage($1->fileline())->కూడు_Stmtsp($1); }
        |       config_declaration                      { }
        //                      // Verilator only
        |       yaT_RESETALL                            { }  // Else, under design, and illegal based on IEEE 22.3
        |       yaT_NOUNCONNECTED                       { PARSEP->unconnectedDrive(VOptionBool::OPT_DEFAULT_FALSE); }
        |       yaT_UNCONNECTED_PULL0                   { PARSEP->unconnectedDrive(VOptionBool::OPT_FALSE); }
        |       yaT_UNCONNECTED_PULL1                   { PARSEP->unconnectedDrive(VOptionBool::OPT_TRUE); }
        |       vlt_అంశం                                 { }
        |       error                                   { }  // LCOV_EXCL_LINE
        ;

timeunits_declaration<nodep>:   // ==IEEE: timeunits_declaration
                yTIMEUNIT yaTIMENUM ';'
                        { $$ = PARSEP->createTimescale($<fl>2, true, $2, false, 0); }
        |       yTIMEUNIT yaTIMENUM '/' yaTIMENUM ';'
                        { $$ = PARSEP->createTimescale($<fl>2, true, $2, true, $4); }
        |       yTIMEPRECISION yaTIMENUM ';'
                        { $$ = PARSEP->createTimescale($<fl>2, false, 0, true, $2); }
        ;

//**********************************************************************
// Packages

package_declaration:            // ==IEEE: package_declaration
                packageFront package_అంశం_List_E yENDPACKAGE చివర_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          ఒకవేళ ($2) $1->కూడు_Stmtsp($2);
                          GRAMMARP->చివర_Label($<fl>4, $1, $4); }
        ;

packageFront<కణుపు_Modulep>:
                yPACKAGE lifetimeE idAny ';'
                        { $$ = new AstPackage{$<fl>3, *$3, PARSEP->libname()};
                          ఒకవేళ ($$->name() == "std") {
                              ఒకవేళ ($$->fileline()->filename() != V3Options::get_Std_Package_మార్గం()) {
                                  $$->v3error("Redeclaring the 'std' package is not allowed");
                              } లేకపోతే {
                                  PARSEP->rootp()->stdPackagep(VN_AS($$, Package));
                              }
                          }
                          $$->లోపల_Library(true);  // packages are always libraries; don't want to make them a "top"
                          $$->lifetime($2);
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->సమయం_చివరి_Unit());
                          PARSEP->rootp()->timeprecision_విలీనం($$->fileline(),
                                                              PARSEP->సమయం_చివరి_Prec());
                          PARSEP->rootp()->కూడు_Modulesp($$); }
        ;

package_అంశం_List_E<nodep>:       // IEEE: [{ package_item }]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       package_అంశం_List                        { $$ = $1; }
        ;

package_అంశం_List<nodep>:        // IEEE: { package_item }
                package_అంశం                            { $$ = $1; }
        |       package_అంశం_List package_అంశం           { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

package_అంశం<nodep>:            // ==IEEE: package_item
                package_or_generate_అంశం_declaration    { $$ = $1; }
        |       anonymous_కార్యక్రమం                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_అంశం_శిఖరం<nodep>:         // ==IEEE: package_item

                package_or_generate_అంశం_decl_No_Checker  { $$ = $1; }
        |       checker_declaration
                       { PARSEP->rootp()->కూడు_Modulesp($1);
                         $$ = nullptr; }
        |       anonymous_కార్యక్రమం                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_or_generate_అంశం_declaration<nodep>:    // ==IEEE: package_or_generate_item_declaration
                package_or_generate_అంశం_decl_No_Checker  { $$ = $1; }
        |       checker_declaration
                        { $1->v3warn(E_UNSUPPORTED, "Unsupported: 'checker' below unit-level");
                          PARSEP->rootp()->కూడు_Modulesp($1);
                          $$ = nullptr; }
        ;

package_or_generate_అంశం_decl_No_Checker<nodep>:
                net_declaration                         { $$ = $1; }
        |       దత్తాంశం_declaration                        { $$ = $1; }
        |       task_declaration                        { $$ = $1; }
        |       function_declaration                    { $$ = $1; }
        //                      // IEEE checker_declaration excluded, to handle Top, see other rules
        //                      // checker_declaration
        |       dpi_import_export                       { $$ = $1; }
        |       extern_constraint_declaration           { $$ = $1; }
        |       class_declaration                       { $$ = $1; }
        //                      // IEEE-2023: Added: interface_class_declaration
        //                      // interface_class_declaration is part of class_declaration
        //                      // class_constructor_declaration is part of function_declaration
        //                      // local_parameter_declaration under parameter_declaration
        |       parameter_declaration ';'               { $$ = $1; }
        |       covergroup_declaration                  { $$ = $1; }
        |       assertion_అంశం_declaration              { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

package_import_declarationList<nodep>:
                package_import_declaration              { $$ = $1; }
        |       package_import_declarationList package_import_declaration
                        { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

package_import_declaration<nodep>:      // ==IEEE: package_import_declaration
                yIMPORT package_import_అంశం_List ';'     { $$ = $2; }
        ;

package_import_అంశం_List<nodep>:
                package_import_అంశం                     { $$ = $1; }
        |       package_import_అంశం_List ',' package_import_అంశం { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

package_import_అంశం<nodep>:     // ==IEEE: package_import_item
                idCC/*package_identifier*/ yP_COLONCOLON package_import_అంశం_Obj
                        { $$ = new AstPackageImport{$<fl>1, *$<strp>1, *$3}; }
        ;

package_import_అంశం_Obj<strp>:   // IEEE: part of package_import_item
                idAny/*package_identifier*/             { $<fl>$ = $<fl>1; $$ = $1; }
        |       '*'                                     { $<fl>$ = $<fl>1; స్థిర string నక్షత్రం = "*"; $$ = &నక్షత్రం; }
        ;

package_export_declaration<nodep>: // IEEE: package_export_declaration
                yEXPORT '*' yP_COLONCOLON '*' ';'
                        { $$ = new Ast_Package_Export_నక్షత్రం_నక్షత్రం{$<fl>2}; }
        |       yEXPORT package_export_అంశం_List ';'     { $$ = $2; }
        ;

package_export_అంశం_List<nodep>:
                package_export_అంశం                     { $$ = $1; }
        |       package_export_అంశం_List ',' package_export_అంశం { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

package_export_అంశం<nodep>:     // ==IEEE: package_export_item
                idCC yP_COLONCOLON package_import_అంశం_Obj
                        { $$ = new AstPackageExport{$<fl>3, *$<strp>1, *$3}; }
        ;

//**********************************************************************
// Module headers

module_declaration:             // ==IEEE: module_declaration
        //                      // timeunits_declaration instead in module_item
        //                      // IEEE: module_nonansi_header + module_ansi_header
                modFront importsAndParametersE ports_నక్షత్రం_E ';'
        /*cont*/    module_అంశం_List_E yENDMODULE చివర_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          ఒకవేళ ($2) $1->కూడు_Stmtsp($2);
                          ఒకవేళ ($3) $1->కూడు_Stmtsp($3);
                          ఒకవేళ ($5) $1->కూడు_Stmtsp($5);
                          GRAMMARP->చివర_Label($<fl>7, $1, $7); }
        |       udpFront ports_నక్షత్రం_E ';'
        /*cont*/    module_అంశం_List_E yENDPRIMITIVE చివర_Label_E
                        { $1->modTrace(false);  // Stash for implicit wires, etc
                          ఒకవేళ ($2) $1->కూడు_Stmtsp($2);
                          ఒకవేళ ($4) $1->కూడు_Stmtsp($4);
                          GRAMMARP->m_tracingParse = true;
                          GRAMMARP->చివర_Label($<fl>6, $1, $6); }
        //
        |       yEXTERN modFront parameter_పోర్టు_list_E ports_నక్షత్రం_E ';'
                        { DEL($2->unlink_Fr_వెనుక()); }
                        // We allow modules to be declared after instantiations, so harmless
        ;

modFront<కణుపు_Modulep>:
        //                      // General note: all *Front functions must call symPushNew before
        //                      // any formal arguments, as the arguments must land in the new scope.
                yMODULE lifetimeE idAny
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname()};
                          $$->lifetime($2);
                          $$->లోపల_Library(PARSEP->లోపల_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->సమయం_చివరి_Unit());
                          PARSEP->rootp()->timeprecision_విలీనం($$->fileline(),
                                                              PARSEP->సమయం_చివరి_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive());
                          PARSEP->rootp()->కూడు_Modulesp($$); }
        |       modFront sigAttrScope                   { $$ = $1; }
        ;

importsAndParametersE<nodep>:   // IEEE: common part of module_declaration, interface_declaration, program_declaration
        //                      // { package_import_declaration } [ parameter_port_list ]
                parameter_పోర్టు_list_E
                        { $$ = $1; $<flag>$ = $<flag>1; }  // hasParameterList
        |       package_import_declarationList parameter_పోర్టు_list_E
                        { $$ = కూడు_తదుపరి_Null($1, $2);
                          $<flag>$ = $<flag>2; }  // hasParameterList
        ;

udpFront<కణుపు_Modulep>:
                yPRIMITIVE lifetimeE idAny
                        { $$ = new AstPrimitive{$<fl>3, *$3, PARSEP->libname()};
                          $$->లోపల_Library(true);
                          $$->lifetime($2);
                          $$->modTrace(false);
                          $$->కూడు_Stmtsp(new AstPragma{$<fl>3, VPragmaType::INLINE_MODULE});
                          GRAMMARP->m_tracingParse = false;
                          PARSEP->rootp()->కూడు_Modulesp($$); }
        ;

parameter_విలువ_assignment_Inst_E<pinp>:      // IEEE: [ parameter_value_assignment ] for instance
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       parameter_విలువ_assignment_Inst          { $$ = $1; }
        ;

parameter_విలువ_assignment_Class_E<pinp>:      // IEEE: [ parameter_value_assignment ] for classes
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       parameter_విలువ_assignment_Class         { $$ = $1; }
        ;

parameter_విలువ_assignment_Inst<pinp>:       // IEEE: parameter_value_assignment for instance
                '#' '(' instParamListE ')'              { $$ = $3; }
        //                      // Parentheses are optional around a single parameter
        //                      // IMPORTANT: Below hardcoded in tokenPipeScanParam
        |       '#' yaINTNUM                            { $$ = new AstPin{$<fl>2, 1, "", new AstConst{$<fl>2, *$2}}; }
        |       '#' yaFLOATNUM                          { $$ = new AstPin{$<fl>2, 1, "",
                                                                          new AstConst{$<fl>2, AstConst::RealDouble{}, $2}}; }
        |       '#' సమయం_అంకె_Adjusted                     { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        |       '#' idClassSel                          { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        //                      // Not needed in Verilator:
        //                      // Side effect of combining *_instantiations
        //                      // '#' delay_value      { UNSUP }
        ;

parameter_విలువ_assignment_Class<pinp>:  // IEEE: parameter_value_assignment (for classes)
        //                      // Like parameter_value_assignment, but for classes only, which always have #()
                '#' '(' instParamListE ')'              { $$ = $3; }
        ;

parameter_పోర్టు_list_E<nodep>:    // IEEE: parameter_port_list + empty == parameter_value_assignment
                /*ఖాళీలు*/                             { $$ = nullptr; $<flag>$ = false; }  // hasParameterList
        |       '#' '(' ')'                             { $$ = nullptr;
                                                          $<flag>$ = true; }  // hasParameterList
        //                      // IEEE: '#' '(' list_of_param_assignments { ',' parameter_port_declaration } ')'
        //                      // IEEE: '#' '(' parameter_port_declaration { ',' parameter_port_declaration } ')'
        //                      // Can't just do that as "," conflicts with between vars and between stmts, so
        //                      // split into pre-comma and post-comma parts
        |       '#' '('                                 { VARRESET_LIST(GPARAM);
                                                          GRAMMARP->m_pinAnsi = true; }
        /*cont*/    param_పోర్టు_Decl_Or_Arg_List ')'          { $$ = $4;
                                                          $<flag>$ = true;  // hasParameterList
                                                          VARRESET_NONLIST(UNKNOWN);
                                                          GRAMMARP->m_pinAnsi = false; }
        //                      // Note legal to start with "a=b" with no parameter statement
        ;

param_పోర్టు_Decl_Or_Arg_List<nodep>:  // IEEE: list_of_param_assignments + { parameter_port_declaration }
                param_పోర్టు_Decl_Or_Arg                              { $$ = $1; }
        |       param_పోర్టు_Decl_Or_Arg_List ',' param_పోర్టు_Decl_Or_Arg   { $$ = $1->కూడు_తదుపరి($3); }
        |       param_పోర్టు_Decl_Or_Arg_List sigAttrScope             { $$ = $1; }
        ;

param_పోర్టు_Decl_Or_Arg<nodep>:      // IEEE: param_assignment + parameter_port_declaration
        //                      // We combine the two as we can't tell which follows a comma
                param_పోర్టు_Decl_Or_Arg_Sub                   { $$ = $1; }
        |       vl_ట్యాగ్                                   { $$ = nullptr; }
        ;

param_పోర్టు_Decl_Or_Arg_Sub<nodep>:
                parameter_పోర్టు_declaration_Front_E param_assignment       { $$ = $2; }
        |       parameter_పోర్టు_declaration_Type_Front_E type_assignment    { $$ = $2; }
        |       sigAttrScope param_పోర్టు_Decl_Or_Arg_Sub                      { $$ = $2; }
        ;

ports_నక్షత్రం_E<nodep>:              // IEEE: .* + list_of_ports + list_of_port_declarations + empty
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        //                      // .* expanded from module_declaration
        //UNSUP '(' yP_DOTSTAR ')'                              { UNSUP }
        |       '('
        /*mid*/         { VARRESET_LIST(పోర్టు); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    list_యొక్క_ports ')'
                       { $$ = $3; VARRESET_NONLIST(UNKNOWN); GRAMMARP->m_pinAnsi = false; }
        ;

list_యొక్క_ports_E<nodep>:          // IEEE: [ list_of_ports + list_of_port_declarations ]
                పోర్టు_And_ట్యాగ్_E                             { $$ = $1; }
        |       list_యొక్క_ports_E ',' పోర్టు_And_ట్యాగ్_E          { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

list_యొక్క_ports<nodep>:           // IEEE: list_of_ports + list_of_port_declarations
                పోర్టు_And_ట్యాగ్                              { $$ = $1; }
        |       list_యొక్క_ports_E ',' పోర్టు_And_ట్యాగ్_E          { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

పోర్టు_And_ట్యాగ్_E<nodep>:
                /*ఖాళీలు*/
                        { స్థిరరాశి పూర్ణాంకం p = PINNUMINC();
                          స్థిరరాశి string name = "__pinNumber" + cvt_గమ్యం_Str(p);
                          $$ = new Ast_పోర్టు{CRELINE(), p, name};
                          AstVar* varp = new AstVar{CRELINE(), VVarType::WIRE, name, VFlag_బిడ్డ_DType{},
                                                    new AstBasicDType{CRELINE(), LOGIC_IMPLICIT}};
                          varp->declDirection(VDirection::ఆదానం);
                          varp->direction(VDirection::ఆదానం);
                          varp->ansi(false);
                          varp->declTyped(true);
                          varp->trace(false);
                          $$ = $$->కూడు_తదుపరి(varp);
                          $$->v3warn(NULLPORT, "Null port on module (perhaps extraneous comma)"); }
        |       పోర్టు_And_ట్యాగ్                              { $$ = $1; }
        |       పోర్టు_And_ట్యాగ్ sigAttrScope                 { $$ = $1; }
        ;

పోర్టు_And_ట్యాగ్<nodep>:
                పోర్టు_3                                    { $$ = $1; }
        |       vl_ట్యాగ్ పోర్టు_3                              { $$ = $2; }  // Tag will associate with previous port
        |       sigAttrScope పోర్టు_And_ట్యాగ్                 { $$ = $2; }
        ;

పోర్టు_3<nodep>:                    // ==IEEE: port
        //                      // SEE ALSO port_declaration, tf_port_declaration,
        //                      // data_declarationVarFront
        //
        //                      // Though not type for interfaces, we factor out the port direction and type
        //                      // so we can handle it in one place
        //
        //                      // IEEE: interface_port_header port_identifier { unpacked_dimension }
        //                      // Expanded interface_port_header
        //                      // We use instantCb here because the non-port form looks just like a module instantiation
        //
        //                      // Looks identical to variable_declaration, so V3LinkDot must resolve when ID known
        //                      // NO: portDirNetE id/*interface*/ portSig variable_dimensionListE sigAttrListE
        //
                పోర్టు_Dir_Net_E id/*interface*/ '.' idAny/*modport*/ పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { // VAR for now, but V3LinkCells may call setIfcaeRef on it later
                          $$ = $5; VARDECL(VAR); VARIO(NONE);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstIfaceRefDType{$<fl>2, $<fl>4, "", *$2, *$4};
                          VARDTYPE(dtp); VARIOANSI();
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $6, $7)); }
        |       పోర్టు_Dir_Net_E yINTERFACE                           పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; GRAMMARP->createGenericIface($3, $4, $5); }
        |       పోర్టు_Dir_Net_E yINTERFACE      '.' idAny/*modport*/ పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5; GRAMMARP->createGenericIface($5, $6, $7, $<fl>4, *$4); }
        //
        |       పోర్టు_Dir_Net_E yINTERCONNECT signingE rangeListE పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5;
                          BBUNSUP($<fl>2, "Unsupported: interconnect");
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); VARIOANSI();
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $6, $7)); }
        //
        //                      // IEEE: ansi_port_declaration, with [port_direction] removed
        //                      //   IEEE: [ net_port_header | interface_port_header ]
        //                      //         port_identifier { unpacked_dimension } [ '=' constant_expression ]
        //                      //   IEEE: [ variable_port_header ] port_identifier
        //                      //              { variable_dimension } [ '=' constant_expression ]
        //                      //   IEEE: '.' port_identifier '(' [ expression ] ')'
        //                      //   Substitute net_port_header = [ port_direction ] net_port_type
        //                      //   Substitute variable_port_header = [ port_direction ] variable_port_type
        //                      //   Substitute net_port_type = [ net_type ] data_type_or_implicit
        //                      //   Substitute variable_port_type = var_data_type
        //                      //   Substitute var_data_type = data_type | yVAR data_type_or_implicit
        //                      //     [ [ port_direction ] net_port_type | interface_port_header]
        //                      //              port_identifier { unpacked_dimension }
        //                      //     [ [ port_direction ] var_data_type ]
        //                      //              port_identifier variable_dimensionListE [ '=' constant_expression ]
        //                      //     [ [ port_direction ] net_port_type | [ port_direction ] var_data_type ]
        //                      //              '.' port_identifier '(' [ expression ] ')'
        //
        //                      // Remove optional '[...] id' is in portAssignment
        //                      // Remove optional '[port_direction]' is in port
        //                      //     net_port_type | interface_port_header
        //                      //            port_identifier { unpacked_dimension }
        //                      //     net_port_type | interface_port_header
        //                      //            port_identifier { unpacked_dimension }
        //                      //     var_data_type
        //                      //            port_identifier variable_dimensionListE [ '=' constExpr ]
        //                      //     net_port_type | [ port_direction ] var_data_type '.' port_identifier '(' [ expr ] ')'
        //                      // Expand implicit_type
        //
        //                      // variable_dimensionListE instead of rangeListE to avoid conflicts
        //
        //                      // Note implicit rules looks just line declaring additional followon port
        //                      // No VARDECL("port") for implicit, as we don't want to declare variables for them
        //                      // IEEE: portDirNetE data_type '.' portSig -> handled with AstDot in expr.
        //
        |       పోర్టు_Dir_Net_E దత్తాంశం_type           పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $4, $5)); }
        |       పోర్టు_Dir_Net_E దత్తాంశం_type           పోర్టు_Sig variable_dimensionListE sigAttrListE '=' const_సమాసం
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          ఒకవేళ (AstVar* vp = VARDONEP($$, $4, $5)) { కూడు_తదుపరి_Null($$, vp); vp->valuep($7); } }
        |       పోర్టు_Dir_Net_E yVAR దత్తాంశం_type      పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $5, $6)); }
        |       పోర్టు_Dir_Net_E yVAR దత్తాంశం_type      పోర్టు_Sig variable_dimensionListE sigAttrListE '=' const_సమాసం
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          ఒకవేళ (AstVar* vp = VARDONEP($$, $5, $6)) { కూడు_తదుపరి_Null($$, vp); vp->valuep($8); } }
        |       పోర్టు_Dir_Net_E yVAR implicit_typeE పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $5, $6)); }
        |       పోర్టు_Dir_Net_E yVAR implicit_typeE పోర్టు_Sig variable_dimensionListE sigAttrListE '=' const_సమాసం
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          ఒకవేళ (AstVar* vp = VARDONEP($$, $5, $6)) { కూడు_తదుపరి_Null($$, vp); vp->valuep($8); } }
        |       పోర్టు_Dir_Net_E signing             పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3;
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $4, $5)); }
        |       పోర్టు_Dir_Net_E signing             పోర్టు_Sig variable_dimensionListE sigAttrListE '=' const_సమాసం
                        { $$ = $3;
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          ఒకవేళ (AstVar* vp = VARDONEP($$, $4, $5)) { కూడు_తదుపరి_Null($$, vp); vp->valuep($7); } }
        |       పోర్టు_Dir_Net_E signingE rangeList  పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4;
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          కూడు_తదుపరి_Null($$, VARDONEP($$, $5, $6)); }
        |       పోర్టు_Dir_Net_E signingE rangeList  పోర్టు_Sig variable_dimensionListE sigAttrListE '=' const_సమాసం
                        { $$ = $4;
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          ఒకవేళ (AstVar* vp = VARDONEP($$, $5, $6)) { కూడు_తదుపరి_Null($$, vp); vp->valuep($8); } }
        |       పోర్టు_Dir_Net_E /*implicit*/        పోర్టు_Sig variable_dimensionListE sigAttrListE
                        { $$ = $2; /*VARDTYPE-same*/ కూడు_తదుపరి_Null($$, VARDONEP($$, $3, $4)); }
        |       పోర్టు_Dir_Net_E /*implicit*/        పోర్టు_Sig variable_dimensionListE sigAttrListE '=' const_సమాసం
                        { $$ = $2; /*VARDTYPE-same*/
                          ఒకవేళ (AstVar* vp = VARDONEP($$, $3, $4)) { కూడు_తదుపరి_Null($$, vp); vp->valuep($6); } }
        //                      //   IEEE: '.' port_identifier '(' [ expression ] ')'
        |       పోర్టు_Dir_Net_E /*implicit*/ '.' పోర్టు_Sig '(' సమాసం ')'
                        { $$ = $3; DEL($5);
                          BBUNSUP($<fl>2, "Unsupported: complex ports (IEEE 1800-2023 23.2.2.1/2)"); }
        //                      // IEEE: part of (non-ansi) port_reference
        |       '{' పోర్టు_వ్యక్తీకరణ_List '}'
                        { $$ = $2; }
        ;

పోర్టు_Dir_Net_E:                    // IEEE: part of port, optional net type and/or direction
                /*ఖాళీలు*/                             { }
        //                      // Per spec, if direction given default the nettype.
        //                      // The higher level rule may override this VARDTYPE with one later in the parse.
        |       పోర్టు_direction                                  { VARDECL(పోర్టు); VARDTYPE_NDECL(nullptr); }
        |       పోర్టు_direction { VARDECL(పోర్టు); } net_type      { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        |       net_type                                        { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        ;

పోర్టు_decl_Net_E:                  // IEEE: part of port_declaration, optional net type
                /*ఖాళీలు*/                             { }
        |       net_type                                { } // net_type calls VARDECL
        ;

పోర్టు_Sig<nodep>:
                id/*పోర్టు*/
                        { $$ = new Ast_పోర్టు{$<fl>1, PINNUMINC(), *$1}; }
        |       idSVKwd
                        { $$ = new Ast_పోర్టు{$<fl>1, PINNUMINC(), *$1}; }
        ;

పోర్టు_వ్యక్తీకరణ_List<nodep>:  // IEEE: part of (non-ansi) port_reference
                పోర్టు_reference                          { $$ = $1; }
        |       పోర్టు_వ్యక్తీకరణ_List ',' పోర్టు_reference  { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

పోర్టు_reference<nodep>:  // IEEE: (non-ansi) port-reference
        //                      // IEEE: port_identifier constant_select
        //                      // constant_select ::= [ '[' constant_part_select_range ']' ]
                id/*port_identifier*/                   { $$ = nullptr; }  // UNSUP above here
        |       id/*port_identifier*/ part_select_range  { $$ = nullptr; DEL($2); }  // UNSUP above here
        ;

//**********************************************************************
// Interface headers

interface_declaration:          // IEEE: interface_declaration + interface_nonansi_header + interface_ansi_header:
        //                      // timeunits_declarationE is instead in interface_item
                intFront importsAndParametersE ports_నక్షత్రం_E ';'
                        interface_అంశం_List_E yENDINTERFACE చివర_Label_E
                        { ఒకవేళ ($2) $1->కూడు_Stmtsp($2);
                          ఒకవేళ ($3) $1->కూడు_Stmtsp($3);
                          ఒకవేళ ($5) $1->కూడు_Stmtsp($5);
                          $1->hasParameterList($<flag>2); }
        |       yEXTERN intFront parameter_పోర్టు_list_E ports_నక్షత్రం_E ';'
                        { DEL($2->unlink_Fr_వెనుక()); }
                        // We allow interfaces to be declared after instantiations, so harmless
        ;

intFront<కణుపు_Modulep>:
                yINTERFACE lifetimeE idAny/*new_interface*/
                        { $$ = new AstIface{$<fl>3, *$3, PARSEP->libname()};
                          $$->లోపల_Library(true);
                          $$->lifetime($2);
                          PARSEP->rootp()->కూడు_Modulesp($$); }
        |       intFront sigAttrScope                   { $$ = $1; }
        ;

interface_అంశం_List_E<nodep>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       interface_అంశం_List                      { $$ = $1; }
        ;

interface_అంశం_List<nodep>:
                interface_అంశం                          { $$ = $1; }
        |       interface_అంశం_List interface_అంశం       { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

interface_అంశం<nodep>:          // IEEE: interface_item + non_port_interface_item
                పోర్టు_declaration ';'                    { $$ = $1; }
        //                      // IEEE: non_port_interface_item
        //                      // IEEE: generate_region
        |       i_generate_region                       { $$ = $1; }
        |       interface_or_generate_అంశం              { $$ = $1; }
        |       కార్యక్రమం_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within interface decls"); }
        //                      // IEEE 1800-2017: modport_item
        //                      // See instead old 2012 position in interface_or_generate_item
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within interface decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // See note in interface_or_generate item
        |       module_common_అంశం                      { $$ = $1; }
        ;

interface_or_generate_అంశం<nodep>:  // ==IEEE: interface_or_generate_item
        //                      // module_common_item in interface_item, as otherwise duplicated
        //                      // with module_or_generate_item's module_common_item
                modport_declaration                     { $$ = $1; }
        |       extern_tf_declaration                   { $$ = $1; }
        ;

//**********************************************************************
// Program headers

anonymous_కార్యక్రమం<nodep>:       // ==IEEE: anonymous_program
        //                      // See the spec - this doesn't change the scope, items still go up "top"
                y_కార్యక్రమం ';' anonymous_కార్యక్రమం_అంశం_List_E yENDPROGRAM
                        { $$ = nullptr;
                         BBUNSUP($<fl>1, "Unsupported: Anonymous programs");
                         VARDTYPE_NDECL(nullptr);
                         DEL($3); }
        ;

anonymous_కార్యక్రమం_అంశం_List_E<nodep>:     // IEEE: { anonymous_program_item }
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       anonymous_కార్యక్రమం_అంశం_List              { $$ = $1; }
        ;

anonymous_కార్యక్రమం_అంశం_List<nodep>:      // IEEE: { anonymous_program_item }
                anonymous_కార్యక్రమం_అంశం                  { $$ = $1; }
        |       anonymous_కార్యక్రమం_అంశం_List anonymous_కార్యక్రమం_అంశం       { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

anonymous_కార్యక్రమం_అంశం<nodep>:  // ==IEEE: anonymous_program_item
                task_declaration                        { $$ = $1; }
        |       function_declaration                    { $$ = $1; }
        |       class_declaration                       { $$ = $1; }
        //                      // IEEE-2023: Added: interface_class_declaration
        //                      // interface_class_declaration is part of class_declaration
        |       covergroup_declaration                  { $$ = $1; }
        //                      // class_constructor_declaration is part of function_declaration
        |       ';'                                     { $$ = nullptr; }
        ;

కార్యక్రమం_declaration:            // IEEE: program_declaration + program_nonansi_header + program_ansi_header:
        //                      // timeunits_declarationE is instead in program_item
                pgmFront parameter_పోర్టు_list_E ports_నక్షత్రం_E ';'
        /*cont*/    కార్యక్రమం_అంశం_List_E yENDPROGRAM చివర_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          ఒకవేళ ($2) $1->కూడు_Stmtsp($2);
                          ఒకవేళ ($3) $1->కూడు_Stmtsp($3);
                          ఒకవేళ ($5) $1->కూడు_Stmtsp($5);
                          GRAMMARP->చివర_Label($<fl>7, $1, $7); }
        |       yEXTERN pgmFront parameter_పోర్టు_list_E ports_నక్షత్రం_E ';'
                        { DEL($2->unlink_Fr_వెనుక()); }
                        // We allow programs to be declared after instantiations, so harmless
        ;

pgmFront<కణుపు_Modulep>:
                y_కార్యక్రమం lifetimeE idAny/*new_program*/
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname(), AstModule::కార్యక్రమం{}};
                          $$->lifetime($2);
                          $$->లోపల_Library(PARSEP->లోపల_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->సమయం_చివరి_Unit());
                          PARSEP->rootp()->timeprecision_విలీనం($$->fileline(),
                                                              PARSEP->సమయం_చివరి_Prec());
                          PARSEP->rootp()->కూడు_Modulesp($$); }
        ;

కార్యక్రమం_అంశం_List_E<nodep>:       // ==IEEE: [{ program_item }]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       కార్యక్రమం_అంశం_List                        { $$ = $1; }
        ;

కార్యక్రమం_అంశం_List<nodep>:        // ==IEEE: { program_item }
                కార్యక్రమం_అంశం                            { $$ = $1; }
        |       కార్యక్రమం_అంశం_List కార్యక్రమం_అంశం           { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

కార్యక్రమం_అంశం<nodep>:            // ==IEEE: program_item
                పోర్టు_declaration ';'                    { $$ = $1; }
        |       non_పోర్టు_కార్యక్రమం_అంశం                   { $$ = $1; }
        ;

non_పోర్టు_కార్యక్రమం_అంశం<nodep>:   // ==IEEE: non_port_program_item
                continuous_కేటాయించు                       { $$ = $1; }
        |       module_or_generate_అంశం_declaration     { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       concurrent_assertion_అంశం               { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       కార్యక్రమం_generate_అంశం                   { $$ = $1; }
        ;

కార్యక్రమం_generate_అంశం<nodep>:           // ==IEEE: program_generate_item
                loop_generate_construct                 { $$ = $1; }
        |       conditional_generate_construct          { $$ = $1; }
        |       generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       gen_అంశం_Begin                            { $$ = $1; }
        |       severity_system_task                    { $$ = $1; }
        ;

extern_tf_declaration<nodep>:           // ==IEEE: extern_tf_declaration
                yEXTERN task_prototype ';'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: extern task"); DEL($2); }
        |       yEXTERN function_prototype ';'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: extern function"); DEL($2); }
        |       yEXTERN yFORKJOIN task_prototype ';'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: extern forkjoin"); DEL($3); }
        ;

modport_declaration<nodep>:             // ==IEEE: modport_declaration
                yMODPORT modport_అంశం_List ';'           { $$ = $2; }
        ;

modport_అంశం_List<nodep>:                // IEEE: part of modport_declaration
                modport_అంశం                            { $$ = $1; }
        |       modport_అంశం_List ',' modport_అంశం       { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

modport_అంశం<nodep>:                    // ==IEEE: modport_item
                idAny/*new-modport*/ '('
        /*mid*/         { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT);
                          GRAMMARP->m_modportProtoTasksp = nullptr; }
        /*cont*/    modportPortsDeclList ')'
                        { $$ = new AstModport{$<fl>1, *$1, $4};
                          // Append any prototype tasks from import/export as interface siblings
                          ఒకవేళ (GRAMMARP->m_modportProtoTasksp) {
                              $$ = Ast_కణుపు::కూడు_తదుపరి_Null(
                                  GRAMMARP->m_modportProtoTasksp, $$);
                              GRAMMARP->m_modportProtoTasksp = nullptr;
                          } }
        ;

modportPortsDeclList<nodep>:
                modportPortsDecl                            { $$ = $1; }
        |       modportPortsDeclList ',' modportPortsDecl   { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

// IEEE: modport_ports_declaration  + modport_simple_ports_declaration
//      + (modport_tf_ports_declaration+import_export) + modport_clocking_declaration
// We've expanded the lists each take to instead just have standalone ID ports.
// We track the type as with the V2k series of defines, then create as each ID is seen.
modportPortsDecl<nodep>:
        //                      // IEEE: modport_simple_ports_declaration
                పోర్టు_direction { GRAMMARP->m_modport_Imp_Exp_చురుకైన = false; } modport_Simple_పోర్టు_Or_TFPort { $$ = $3; }
        //                      // IEEE: modport_clocking_declaration
        |       yCLOCKING idAny/*clocking_identifier*/ { $$ = new AstModportClockingRef{$1, *$2}; }
        //                      // IEEE: yIMPORT modport_tf_port
        //                      // IEEE: yEXPORT modport_tf_port
        //                      // modport_tf_port expanded here
        |       yIMPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, false};
                          GRAMMARP->m_modport_Imp_Exp_చురుకైన = true;
                          GRAMMARP->m_modport_Imp_Exp_చివరి_Is_Export = false; }
        |       yEXPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, true};
                          GRAMMARP->m_modport_Imp_Exp_చురుకైన = true;
                          GRAMMARP->m_modport_Imp_Exp_చివరి_Is_Export = true; }
        |       yIMPORT పద్ధతి_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), false};
                          GRAMMARP->m_modport_Imp_Exp_చురుకైన = true;
                          GRAMMARP->m_modport_Imp_Exp_చివరి_Is_Export = false;
                          // Import prototype: task should already exist in interface
                          // (from export or direct declaration). Delete the prototype.
                          DEL($2); }
        |       yEXPORT పద్ధతి_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), true};
                          GRAMMARP->m_modport_Imp_Exp_చురుకైన = true;
                          GRAMMARP->m_modport_Imp_Exp_చివరి_Is_Export = true;
                          // Export: add prototype task to interface (as sibling of modport)
                          GRAMMARP->m_modportProtoTasksp
                              = Ast_కణుపు::కూడు_తదుపరి_Null(GRAMMARP->m_modportProtoTasksp, $2); }
        // Continuations of above after a comma.
        //                      // IEEE: modport_simple_ports_declaration
        |       modport_Simple_పోర్టు_Or_TFPort                { $$ = $1; }
        ;

modport_Simple_పోర్టు_Or_TFPort<nodep>:// IEEE: modport_simple_port or modport_tf_port, depending what keyword was earlier
                idAny                                   { $$ = GRAMMARP->m_modport_Imp_Exp_చురుకైన ?
                                                                static_cast<Ast_కణుపు*>(
                                                                  new AstModportFTaskRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_modport_Imp_Exp_చివరి_Is_Export} ) :
                                                                static_cast<Ast_కణుపు*>(
                                                                  new AstModportVarRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_varIO} ); }
        |       '.' idAny '(' ')'                       { $$ = new AstModportVarRef{$<fl>2, *$2, GRAMMARP->m_varIO};
                                                          BBUNSUP($<fl>4, "Unsupported: Modport empty expression"); }
        |       '.' idAny '(' సమాసం ')'                  { $$ = new AstModportVarRef{$<fl>2, *$2, $4, GRAMMARP->m_varIO}; }
        ;

//************************************************
// Variable Declarations

genvar_declaration<nodep>:      // ==IEEE: genvar_declaration
                yGENVAR list_యొక్క_genvar_identifiers ';'  { $$ = $2; }
        ;

list_యొక్క_genvar_identifiers<nodep>:      // IEEE: list_of_genvar_identifiers (for declaration)
                genvar_identifierDecl                   { $$ = $1; }
        |       list_యొక్క_genvar_identifiers ',' genvar_identifierDecl    { $$ = $1->కూడు_తదుపరి($3); }
        ;

genvar_identifierDecl<varp>:            // IEEE: genvar_identifier (for declaration)
                idAny/*new-genvar_identifier*/ sigAttrListE
                        { VARRESET_NONLIST(GENVAR);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$<fl>1, VBasicDTypeKwd::INTEGER};
                          VARDTYPE(dtp);
                          $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        ;

parameter_declaration<nodep>:   // IEEE: local_ or parameter_declaration and type_parameter_declaration
        //                      // IEEE: yPARAMETER yTYPE list_of_type_assignments ';'
        //                      // Instead of list_of_type_assignments
        //                      // we use list_of_param_assignments because for port handling
        //                      // it already must accept types, so simpler to have code only one place
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                parameter_declarationFront list_యొక్క_param_assignments            { $$ = $2; }
        |       parameter_declarationTypeFront list_యొక్క_type_assignments         { $$ = $2; }
        ;

parameter_declarationFront:     // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset దత్తాంశం_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        ;

parameter_declarationTypeFront: // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/ VARDTYPE(new AstParseTypeDType{$2}); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        ;

parameter_పోర్టు_declaration_Front_E: // IEEE: local_ or parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset దత్తాంశం_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       implicit_typeE
                        { /*VARRESET-in-varParam*/
                          // Keep previous type to handle subsequent declarations.
                          // This rule is also used when the previous parameter is a type parameter
                          ఒకవేళ ($1) $1->v3error("parameter port declarations require 'parameter'"
                                              " keyword before implicit data types"
                                              " (IEEE 1800-2023 6.20.1/A.2.1.1)\n"
                                              + $1->warn_మరిన్ని()
                                              + "... Suggest add 'parameter' before here");
                       }
        |       దత్తాంశం_type                               { /*VARRESET-in-varParam*/ VARDTYPE($1); }
        ;

parameter_పోర్టు_declaration_Type_Front_E: // IEEE: parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstParseTypeDType{$2}; VARDTYPE(dtp); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        |       yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstParseTypeDType{$1}; VARDTYPE(dtp); }
        |       yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstParseTypeDType{$1, $2}; VARDTYPE(dtp); }
        ;

forward_type<fwdtype>:  // ==IEEE: forward_type
                yENUM                                   { $$ = VFwdType::ENUM; }
        |       ySTRUCT                                 { $$ = VFwdType::STRUCT; }
        |       yUNION                                  { $$ = VFwdType::UNION; }
        |       yCLASS                                  { $$ = VFwdType::CLASS; }
        |       yINTERFACE yCLASS                       { $$ = VFwdType::INTERFACE_CLASS; }
        ;

net_declaration<nodep>:         // IEEE: net_declaration - excluding implict
                net_declarationFront netSigList ';'
                        { $$ = $2;
                          ఒకవేళ (GRAMMARP->m_netStrengthp) {
                              VL_DO_CLEAR(delete GRAMMARP->m_netStrengthp, GRAMMARP->m_netStrengthp = nullptr);
                          }
                          GRAMMARP->సమితి_Net_Delay(nullptr); }
        ;

net_declarationFront:           // IEEE: beginning of net_declaration
                net_declRESET net_type driveStrengthE net_scalaredE net_దత్తాంశం_Type_E
                        { VARDTYPE_NDECL($5);
                          GRAMMARP->సమితి_Net_Strength($3); }
        |       net_declRESET yINTERCONNECT signingE rangeListE
                        { BBUNSUP($<fl>2, "Unsupported: interconnect");
                          VARDECL(WIRE);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        ;

net_declRESET:
                /*ఖాళీలు*/                             { VARRESET_NONLIST(UNKNOWN); }
        ;

net_scalaredE:
                /*ఖాళీలు*/                             { }
        //                      //UNSUP: ySCALARED/yVECTORED ignored
        |       ySCALARED                               { }
        |       yVECTORED                               { }
        ;

net_దత్తాంశం_Type_E<కణుపు_DTypep>:
        //                      // If there's a SV data type there shouldn't be a delay on this wire
        //                      // Otherwise #(...) can't be determined to be a delay or parameters
        //                      // Submit this as a footnote to the committee
                var_దత్తాంశం_type                           { $$ = $1; }
        |       signingE rangeList delay_controlE
                        { $$ = GRAMMARP->కూడు_Range(new AstBasicDType{$2->fileline(), LOGIC, $1},
                                                                    $2, true);
                          GRAMMARP->సమితి_Net_Delay($3); }  // not implicit
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC, $1}; }  // not implicit
        |       /*implicit*/ delay_controlE
                        { $$ = new AstBasicDType{CRELINE(), LOGIC};
                          GRAMMARP->సమితి_Net_Delay($1); }  // not implicit
        ;

net_type:                       // ==IEEE: net_type
                ySUPPLY0                                { VARDECL(SUPPLY0); }
        |       ySUPPLY1                                { VARDECL(SUPPLY1); }
        |       yTRI                                    { VARDECL(TRIWIRE); }
        |       yTRI0                                   { VARDECL(TRI0); }
        |       yTRI1                                   { VARDECL(TRI1); }
        |       yTRIAND                                 { VARDECL(TRIAND); }
        |       yTRIOR                                  { VARDECL(TRIOR); }
        |       yTRIREG                                 { VARDECL(WIRE); BBUNSUP($1, "Unsupported: trireg"); }
        |       yWAND                                   { VARDECL(TRIAND); }
        |       yWIRE                                   { VARDECL(WIRE); }
        |       yWOR                                    { VARDECL(TRIOR); }
        //                      // VAMS - somewhat hackish
        |       yWREAL                                  { VARDECL(WREAL); }
        ;

varParamReset:
                yPARAMETER                              { VARRESET_NONLIST(GPARAM); }
        |       yLOCALPARAM                             { VARRESET_NONLIST(LPARAM); }
        // Note that the type of these params could be modified later according to context (see V3LinkParse)
        ;

పోర్టు_direction:                 // ==IEEE: port_direction + tf_port_direction
        //                      // IEEE 19.8 just "input" FIRST forces type to wire - we'll ignore that here
        //                      // Only used for ANSI declarations
                y_ఆదానం                                  { VARIO(ఆదానం); }
        |       yOUTPUT                                 { VARIO(OUTPUT); }
        |       yINOUT                                  { VARIO(INOUT); }
        |       yREF                                    { VARIO(REF); }
        //                      // IEEE 1800-2023: Added 'ref static' and 'const ref static'
        |       yREF ySTATIC__ETC                       { VARIO(REF);
                                                          BBUNSUP($1, "Unsupported: 'ref static' ports"); }
        |       yCONST__REF yREF                        { VARIO(CONSTREF); }
        |       yCONST__REF yREF ySTATIC__ETC           { VARIO(CONSTREF);
                                                          BBUNSUP($1, "Unsupported: 'ref static' ports"); }
        ;

పోర్టు_direction_Reset:            // IEEE: port_direction that starts a port_declaraiton
        //                      // Used only for declarations outside the port list
                y_ఆదానం                                  { VARRESET_NONLIST(UNKNOWN); VARIO(ఆదానం); }
        |       yOUTPUT                                 { VARRESET_NONLIST(UNKNOWN); VARIO(OUTPUT); }
        |       yINOUT                                  { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT); }
        |       yREF                                    { VARRESET_NONLIST(UNKNOWN); VARIO(REF); }
        |       yCONST__REF yREF                        { VARRESET_NONLIST(UNKNOWN); VARIO(CONSTREF); }
        ;

పోర్టు_declaration<nodep>:        // ==IEEE: port_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port, tf_port_declaration, data_declarationVarFront
        //
        //                      // IEEE: inout_declaration
        //                      // IEEE: input_declaration
        //                      // IEEE: output_declaration
        //                      // IEEE: ref_declaration
                పోర్టు_direction_Reset పోర్టు_decl_Net_E దత్తాంశం_type
        /*mid*/         { VARDTYPE($3); }
        /*cont*/    list_యొక్క_variable_decl_assignments                   { $$ = $5; }
        |       పోర్టు_direction_Reset పోర్టు_decl_Net_E yVAR దత్తాంశం_type
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_యొక్క_variable_decl_assignments                   { $$ = $6; }
        |       పోర్టు_direction_Reset పోర్టు_decl_Net_E yVAR implicit_typeE
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_యొక్క_variable_decl_assignments                   { $$ = $6; }
        |       పోర్టు_direction_Reset పోర్టు_decl_Net_E signingE rangeList
        /*mid*/         { Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                              new AstBasicDType{$4->fileline(), LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_యొక్క_variable_decl_assignments                   { $$ = $6; }
        |       పోర్టు_direction_Reset పోర్టు_decl_Net_E signing
        /*mid*/         { Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $3};
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_యొక్క_variable_decl_assignments                   { $$ = $5; }
        |       పోర్టు_direction_Reset పోర్టు_decl_Net_E /*implicit*/
        /*mid*/         { VARDTYPE_NDECL(nullptr); /*default_nettype*/ }
        /*cont*/    list_యొక్క_variable_decl_assignments                   { $$ = $4; }
        //
        //                      // IEEE: interface_port_declaration
        //                      // IEEE: interface_identifier list_of_interface_identifiers
        //
        //                      // Identical to variable_declaration, resolve in V3LinkDot when id known
        //                      // NO:  id/*interface*/  mpInstnameList
        //
        //                      // IEEE: interface_port_declaration
        //                      // IEEE: interface_identifier '.' modport_identifier list_of_interface_identifiers
        |       id/*interface*/ '.' idAny/*modport*/
        /*mid*/         { VARRESET_NONLIST(VVarType::IFACEREF);
                          AstIfaceRefDType* స్థిరరాశి dtp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          VARDTYPE(dtp); }
        /*cont*/    mpInstnameList
                        { $$ = VARDONEP($5, nullptr, nullptr); DEL($5); }
        //UNSUP: strengthSpecE for udp_instantiations
        ;

tf_పోర్టు_declaration<nodep>:     // ==IEEE: tf_port_declaration
        //                      // Used inside function; followed by ';'
        //                      // SEE ALSO port_declaration, port, data_declarationVarFront
        //
                పోర్టు_direction_Reset      దత్తాంశం_type      { VARDTYPE($2); }  list_యొక్క_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       పోర్టు_direction_Reset      implicit_typeE { VARDTYPE_NDECL($2); }  list_యొక్క_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       పోర్టు_direction_Reset yVAR దత్తాంశం_type      { VARDTYPE($3); }  list_యొక్క_tf_variable_identifiers ';'
                        { $$ = $5; }
        |       పోర్టు_direction_Reset yVAR implicit_typeE { VARDTYPE($3); }  list_యొక్క_tf_variable_identifiers ';'
                        { $$ = $5; }
        ;

integer_atom_type<basicDTypep>: // ==IEEE: integer_atom_type
                yBYTE                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::BYTE}; }
        |       ySHORTINT                               { $$ = new AstBasicDType{$1, VBasicDTypeKwd::SHORTINT}; }
        |       yINT                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INT}; }
        |       yLONGINT                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::LONGINT}; }
        |       yINTEGER                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INTEGER}; }
        |       y_సమయం                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::సమయం}; }
        ;

integer_సదిశ_type<basicDTypep>:       // ==IEEE: integer_atom_type
                y_బిట్టు                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::బిట్టు}; }
        |       yLOGIC                                  { $$ = new AstBasicDType{$1, VBasicDTypeKwd::LOGIC}; }
        |       yREG                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::LOGIC}; } // logic==reg
        ;

non_integer_type<basicDTypep>:  // ==IEEE: non_integer_type
                yREAL                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE}; }
        |       yREALTIME                               { $$ = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE}; }
        |       ySHORTREAL                              { $$ = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE}; UNSUPREAL($1); }
        ;

signingE<signstate>:            // IEEE: signing - plus empty
                /*empty*/                               { $$ = VSigning::NOSIGN; }
        |       signing                                 { $$ = $1; }
        ;

signing<signstate>:             // ==IEEE: signing
                ySIGNED                                 { $<fl>$ = $<fl>1; $$ = VSigning::SIGNED; }
        |       yUNSIGNED                               { $<fl>$ = $<fl>1; $$ = VSigning::UNSIGNED; }
        ;

//************************************************
// Data Types

simple_typeNoRef<కణుపు_DTypep>:   // IEEE: simple_type without idType
        //                      // IEEE: integer_type
                integer_atom_type                       { $$ = $1; }
        |       integer_సదిశ_type                     { $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        //
        //                      // { generate_block_identifer ... } '.'
        //                      // Need to determine if generate_block_identifier can be lex-detected
        ;

దత్తాంశం_type<కణుపు_DTypep>:          // ==IEEE: data_type
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                దత్తాంశం_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idType packed_dimensionListE
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idType parameter_విలువ_assignment_Class packed_dimensionListE
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

దత్తాంశం_type_Any<కణుపు_DTypep>:       // ==IEEE: data_type (accepting idAny)
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                దత్తాంశం_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idAny packed_dimensionListE
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idAny parameter_విలువ_assignment_Class packed_dimensionListE
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

దత్తాంశం_type_Basic<కణుపు_DTypep>:             // IEEE: part of data_type
                integer_సదిశ_type signingE rangeListE { $1->సమితి_Signed_State($2); $$ = GRAMMARP->కూడు_Range($1, $3, true); }
        //                      // IEEE: packed_dimension: unsized_dimension, legal only in DPI import
        |       integer_సదిశ_type signingE '[' ']'
                        { $1->సమితి_Signed_State($2);
                          $$ = new AstUnsizedArrayDType{$3, VFlag_బిడ్డ_DType{}, $1}; }
        |       integer_atom_type signingE              { $1->సమితి_Signed_State($2); $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        ;

దత్తాంశం_type_No_Ref<కణుపు_DTypep>:             // ==IEEE: data_type, excluding class_type etc references
                దత్తాంశం_type_Basic                          { $$ = $1; }
        |       struct_unionDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpsu" + cvt_గమ్యం_Str(GRAMMARP->s_type_Imp_అంకె++),
                                                           VFlag_బిడ్డ_DType{}, $1}, $2, true); }
        |       enumDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpenum" + cvt_గమ్యం_Str(GRAMMARP->s_type_Imp_అంకె++),
                                                           VFlag_బిడ్డ_DType{}, $1}, $2, true); }
        |       ySTRING
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::STRING}; }
        |       yCHANDLE
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CHANDLE}; }
        |       yEVENT
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::EVENT}; v3Global.సమితి_Has_Events(); }
        |       type_referenceDecl                      { $$ = $1; }
        //                      // IEEE: class_scope: see data_type above
        //                      // IEEE: class_type: see data_type above
        //                      // IEEE: ps_covergroup: see data_type above
        //                      // Rules overlap virtual_interface_declaration
        |       yVIRTUAL__INTERFACE yINTERFACE దత్తాంశం_type_Virtual
                        { $$ = $3; }
        |       yVIRTUAL__anyID                దత్తాంశం_type_Virtual
                        { $$ = $2; }
        ;

దత్తాంశం_type_Virtual<కణుపు_DTypep>:           // ==IEEE: data_type after yVIRTUAL [ yINTERFACE ]
        //                      // Parameters here are SV2009
                idAny/*interface*/
                        { AstIfaceRefDType* స్థిరరాశి ifrefp = new AstIfaceRefDType{$<fl>1, "", *$1};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ '.' idAny/*modport*/
                        { AstIfaceRefDType* స్థిరరాశి ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_విలువ_assignment_Class
                        { AstIfaceRefDType* స్థిరరాశి ifrefp = new AstIfaceRefDType{$<fl>1, nullptr, "", *$1, "", $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_విలువ_assignment_Class '.' idAny/*modport*/
                        { AstIfaceRefDType* స్థిరరాశి ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>4, "", *$1, *$4, $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        ;

దత్తాంశం_type_or_void<కణుపు_DTypep>:  // ==IEEE: data_type_or_void
                దత్తాంశం_type_Any                            { $$ = $1; }
        |       yVOID
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CVOID}; }
        ;

var_దత్తాంశం_type<కణుపు_DTypep>:              // ==IEEE: var_data_type
                దత్తాంశం_type                               { $$ = $1; }
        |       yVAR దత్తాంశం_type                          { $$ = $2; }
        |       yVAR implicit_typeE                     { $$ = $2; }
        ;

type_referenceBoth<కణుపు_Exprp>:          // IEEE: type_reference
                yTYPE__ETC '(' సమాసం_Or_దత్తాంశం_Type ')'
                        { $$ = new Ast_Attr_యొక్క{$1, VAttrType::TYPEID, $3}; }
        ;

type_referenceDecl<కణుపు_DTypep>:         // IEEE: type_reference (as a data type for declaration)
                yTYPE__ETC '(' సమాసం_Or_దత్తాంశం_Type ')'
                        { $$ = new AstRefDType{$1, AstRefDType::Flag_Type_యొక్క_సమాసం{}, $3}; }
        ;

type_referenceEq<కణుపు_Exprp>:            // IEEE: type_reference (as an ==/!== expression)
                yTYPE__EQ '(' సమాసం_Or_దత్తాంశం_Type ')'
                        { $$ = new Ast_Attr_యొక్క{$1, VAttrType::TYPEID, $3}; }
        ;

struct_unionDecl<కణుపు_UOr_Struct_DTypep>:  // IEEE: part of data_type
        //                      // packedSigningE is NOP for unpacked
                ySTRUCT        packedSigningE '{'
        /*mid*/         { $<కణుపు_UOr_Struct_DTypep>$ = new AstStructDType{$1, $2}; }
        /*cont*/    struct_union_member_List_చివర
                        { $$ = $<కణుపు_UOr_Struct_DTypep>4; $$->కూడు_Membersp($5); }
        |       yUNION taggedSoftE packedSigningE '{'
        /*mid*/         { $<కణుపు_UOr_Struct_DTypep>$ = new AstUnionDType{$1, $2 == tagged_SOFT, $2 == tagged_TAGGED, $3}; }
        /*cont*/    struct_union_member_List_చివర
                        { $$ = $<కణుపు_UOr_Struct_DTypep>5; $$->కూడు_Membersp($6); }
        ;

struct_union_member_List_చివర<memberDTypep>: // IEEE: { struct_union_member } '}'
                struct_union_memberList '}'                     { $$ = $1; }
        //
        |       struct_union_memberList error '}'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error '}'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_memberList<memberDTypep>: // IEEE: { struct_union_member }
                struct_union_member                             { $$ = $1; }

        |       struct_union_memberList struct_union_member     { $$ = కూడు_తదుపరి_Null($1, $2); }
        //
        |       struct_union_memberList error ';'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error ';'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_member<memberDTypep>:     // ==IEEE: struct_union_member
                random_qualifierE దత్తాంశం_type_or_void
        /*mid*/         { GRAMMARP->m_memDTypep = $2;  // As a list follows, need to attach this dtype to each member.
                          GRAMMARP->m_memRand = $1.randAttr(); }
        /*cont*/    list_యొక్క_member_decl_assignments ';'
                        { $$ = $4;
                          DEL(GRAMMARP->m_memDTypep); GRAMMARP->m_memDTypep = nullptr;
                          GRAMMARP->m_memRand = VRandAttr::NONE; }
        |       vl_ట్యాగ్                                   { $$ = nullptr; }
        ;

list_యొక్క_member_decl_assignments<memberDTypep>: // Derived from IEEE: list_of_variable_decl_assignments
                member_decl_assignment                  { $$ = $1; }
        |       list_యొక్క_member_decl_assignments ',' member_decl_assignment      { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

member_decl_assignment<memberDTypep>:   // Derived from IEEE: variable_decl_assignment
        //                      // At present we allow only packed structures/unions.
        //                      // So this is different from variable_decl_assignment
                idAny variable_dimensionListE
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlag_బిడ్డ_DType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_వృక్షం(true) : nullptr),
                                                                        $2, false),
                                                  nullptr};
                          $$->rand(GRAMMARP->m_memRand);
                          PARSEP->ట్యాగ్_Nodep($$);
                        }
        |       idAny variable_dimensionListE '=' variable_decl_సమాసం
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlag_బిడ్డ_DType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_వృక్షం(true) : nullptr),
                                                                        $2, false),
                                                  $4};
                          $$->rand(GRAMMARP->m_memRand);
                          PARSEP->ట్యాగ్_Nodep($$);
                        }
        |       idSVKwd                                 { $$ = nullptr; }
        //
        //                      // IEEE: "dynamic_array_variable_identifier '[' ']' [ '=' dynamic_array_new ]"
        //                      // Matches above with variable_dimensionE = "[]"
        //                      // IEEE: "class_variable_identifier [ '=' class_new ]"
        //                      // variable_dimensionE must be empty
        //                      // Pushed into variable_declExpr:dynamic_array_new
        ;

list_యొక్క_variable_decl_assignments<varp>:        // ==IEEE: list_of_variable_decl_assignments
                variable_decl_assignment                { $$ = $1; }
        |       list_యొక్క_variable_decl_assignments ',' variable_decl_assignment  { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

variable_decl_assignment<varp>: // ==IEEE: variable_decl_assignment
                id variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       id variable_dimensionListE sigAttrListE '=' variable_decl_సమాసం
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); $$->valuep($5); }
        |       idSVKwd                                 { $$ = nullptr; }
        //
        //                      // IEEE: "dynamic_array_variable_identifier '[' ']' [ '=' dynamic_array_new ]"
        |       id variable_dimensionListE sigAttrListE yP_EQ__NEW dynamic_array_new
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); $$->valuep($5); }
        //                      // IEEE: "class_variable_identifier [ '=' class_new ]"
        //                      // variable_dimensionE must be empty
        |       id variable_dimensionListE sigAttrListE yP_EQ__NEW class_new
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); $$->valuep($5); }
        ;

list_యొక్క_tf_variable_identifiers<nodep>: // ==IEEE: list_of_tf_variable_identifiers
                tf_variable_identifier                  { $$ = $1; }
        |       list_యొక్క_tf_variable_identifiers ',' tf_variable_identifier      { $$ = $1->కూడు_తదుపరి($3); }
        ;

tf_variable_identifier<varp>:           // IEEE: part of list_of_tf_variable_identifiers
                id variable_dimensionListE sigAttrListE సమాసం_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          ఒకవేళ ($4) Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(
                                      $$, new Ast_కేటాయించు{$4->fileline(), new AstParseRef{$<fl>1, *$1}, $4}); }
        ;

variable_decl_సమాసం<nodep>:               // IEEE: part of variable_decl_assignment - rhs of expr
                సమాసం                                    { $$ = $1; }
        |       dynamic_array_new                       { $$ = $1; }
        |       class_newNoScope                        { $$ = $1; }
        ;

variable_dimensionListE<కణుపు_Rangep>:    // IEEE: variable_dimension + empty
                /*empty*/                               { $$ = nullptr; }
        |       variable_dimensionList                  { $$ = $1; }
        ;

variable_dimensionList<కణుపు_Rangep>:     // IEEE: variable_dimension + empty
                variable_dimension                      { $$ = $1; }
        |       variable_dimensionList variable_dimension       { $$ = $1->కూడు_తదుపరి($2); }
        ;

variable_dimension<కణుపు_Rangep>: // ==IEEE: variable_dimension
        //                      // IEEE: unsized_dimension
                '[' ']'                                 { $$ = new AstUnsizedRange{$1}; }
        //                      // IEEE: unpacked_dimension
        |       anyrange                                { $$ = $1; }
        //                      // IEEE: unpacked_dimension (if const_expr)
        //                      // IEEE: associative_dimension (if data_type)
        //                      // Can't tell which until see if expr is data type or not
        |       '[' సమాసం_Or_దత్తాంశం_Type ']'                  { $$ = new AstBracketRange{$1, $2}; }
        |       yP_BRASTAR ']'                          { $$ = new AstWildcardRange{$1}; }
        |       '[' '*' ']'                             { $$ = new AstWildcardRange{$1}; }
        //                      // IEEE: queue_dimension
        //                      // '[' '$' ']' -- $ is part of expr, see '[' constExpr ']'
        //                      // '[' '$' ':' expr ']' -- anyrange:expr:$
        ;

random_qualifierE<qualifiers>:  // IEEE: random_qualifier + empty
                /*empty*/                               { $$ = VMemberQualifiers::none(); }
        |       random_qualifier                        { $$ = $1; }
        ;

random_qualifier<qualifiers>:   // ==IEEE: random_qualifier
                yRAND                                   { $$ = VMemberQualifiers::none(); $$.m_rand = true; }
        |       yRANDC                                  { $$ = VMemberQualifiers::none(); $$.m_randc = true; }
        ;

taggedSoftE<taggedstate>:
                /*empty*/                               { $$ = tagged_NONE; }
        |       ySOFT                                   { $$ = tagged_SOFT; }
        |       yTAGGED                                 { $$ = tagged_TAGGED; }
        ;

packedSigningE<signstate>:
        //                      // VSigning::NOSIGN overloaded to indicate not packed
                /*empty*/                               { $$ = VSigning::NOSIGN; }
        |       yPACKED signingE                        { $$ = $2; ఒకవేళ ($$ == VSigning::NOSIGN) $$ = VSigning::UNSIGNED; }
        ;

//************************************************
// enum

// IEEE: part of data_type
enumDecl<కణుపు_DTypep>:
                yENUM enum_పునాది_type_E '{' enum_nameList '}'
                        { $$ = new AstEnumDType{$1, VFlag_బిడ్డ_DType{}, $2, $4}; }
        ;

enum_పునాది_type_E<కణుపు_DTypep>:    // IEEE: enum_base_type
                /*ఖాళీలు*/
                        { $$ = new AstBasicDType{CRELINE(), VBasicDTypeKwd::INT}; }
        //                      // Not in spec, but obviously "enum [1:0]" should work
        //                      // implicit_type expanded, without empty
        //                      // Note enum base types are always packed data types
        |       signingE rangeList
                        { $$ = GRAMMARP->కూడు_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        //
        |       integer_atom_type signingE
                        { $1->సమితి_Signed_State($2); $$ = $1; }
        |       integer_సదిశ_type signingE rangeListE
                        { $1->సమితి_Signed_State($2); $$ = GRAMMARP->కూడు_Range($1, $3, true); }
        //                      // below can be idAny or yaID__aTYPE
        //                      // IEEE requires a type, though no shift conflict if idAny
        //                      // IEEE: type_identifier [ packed_dimension ]
        //                      // however other simulators allow [ class_scope | package_scope ] type_identifier
        |       idAny rangeListE
                        { $$ = GRAMMARP->createArray(new AstRefDType{$<fl>1, *$1}, $2, true); }
        |       packageClassScope idAny rangeListE
                        { AstRefDType* refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        ;

enum_nameList<enumItemp>:
                enum_name_declaration                   { $$ = $1; }
        |       enum_nameList ',' enum_name_declaration { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

enum_name_declaration<enumItemp>:   // ==IEEE: enum_name_declaration
                idAny/*enum_identifier*/ enumNameRangeE enum_Name_ప్రారంభం_E
                        { $$ = new Ast_Enum_అంశం{$<fl>1, *$1, $2, $3}; }
        ;

enumNameRangeE<rangep>:          // IEEE: second part of enum_name_declaration
                /*ఖాళీలు*/
                        { $$ = nullptr; }
        |       '[' intnumAsConst ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstConst($1, $2->గమ్యం_SInt() - 1)}; DEL($2); }
        |       '[' intnumAsConst ':' intnumAsConst ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

enum_Name_ప్రారంభం_E<కణుపు_Exprp>:      // IEEE: third part of enum_name_declaration
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       '=' const_సమాసం                           { $$ = $2; }
        ;

intnumAsConst<constp>:
                yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        ;

//************************************************
// Typedef

దత్తాంశం_declaration<nodep>:        // ==IEEE: data_declaration
        //                      // VARRESET can't be called here - conflicts
                దత్తాంశం_declaration_Var                     { $$ = $1; }
        |       type_declaration                        { $$ = $1; }
        |       package_import_declaration              { $$ = $1; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        //                      // 1800-2009:
        |       nettype_declaration                     { $$ = $1; }
        |       vl_ట్యాగ్                                   { $$ = nullptr; }
        ;

class_property<nodep>:          // ==IEEE: class_property, which is {property_qualifier} data_declaration
                memberQualListE దత్తాంశం_declaration_Var_Class
                        { $$ = $2; $1.apply_గమ్యం_Nodes($2); }
        |       memberQualListE type_declaration
                        { $$ = $2; ఒకవేళ (VN_IS($2, Typedef)) $1.apply_గమ్యం_Nodes(VN_AS($2, Typedef)); }
        //                      // UNSUP: Import needs to apply local/protected from memberQualList, and error on others
        |       memberQualListE package_import_declaration
                        { $$ = $2; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        ;

దత్తాంశం_declaration_Var<varp>:      // IEEE: part of data_declaration
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                దత్తాంశం_declaration_Var_Front list_యొక్క_variable_decl_assignments ';'  { $$ = $2; }
        ;

దత్తాంశం_declaration_Var_Class<varp>: // IEEE: part of data_declaration (for class_property)
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                దత్తాంశం_declaration_Var_Front_Class list_యొక్క_variable_decl_assignments ';'     { $$ = $2; }
        ;

దత్తాంశం_declaration_Var_Front:       // IEEE: part of data_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port_declaration, tf_port_declaration, port
        //
        //                      // Expanded: "constE yVAR lifetimeE data_type"
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE దత్తాంశం_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT};
                          VARDTYPE(dtp); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/ VARLIFE($2);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                              new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); }
        //
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
        |       yCONST__ETC yVAR lifetimeE దత్తాంశం_type
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstConstDType{$<fl>2, VFlag_బిడ్డ_DType{}, $4};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstConstDType{$<fl>2, VFlag_బిడ్డ_DType{},
                                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE signingE rangeList
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstConstDType{$<fl>2, VFlag_బిడ్డ_DType{},
                                  GRAMMARP->కూడు_Range(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $4}, $5, true)};
                          VARDTYPE(dtp); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       /**/                  దత్తాంశం_type
                        { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        |       /**/        lifetime  దత్తాంశం_type
                        { VARRESET_NONLIST(VAR); VARLIFE($1); VARDTYPE($2); }
        |       yCONST__ETC lifetimeE దత్తాంశం_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = new AstConstDType{$<fl>1, VFlag_బిడ్డ_DType{}, $3};
                          VARDTYPE(dtp); }
        //                      // = class_new is in variable_decl_assignment
        ;

దత్తాంశం_declaration_Var_Front_Class:  // IEEE: part of data_declaration (for class_property)
        //                      // VARRESET called before this rule
        //                      // yCONST is removed, added to memberQual rules
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE దత్తాంశం_type        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE                  { VARRESET_NONLIST(VAR); VARLIFE($2); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp);
                          VARLIFE($2); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       దత్తాంశం_type                       { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        //                      // lifetime is removed, added to memberQual rules to avoid conflict
        //                      // yCONST is removed, added to memberQual rules to avoid conflict
        //                      // = class_new is in variable_decl_assignment
        ;

nettype_declaration<nodep>:  // IEEE: nettype_declaration/net_type_declaration
        //                      // Union of data_typeAny and nettype_identifier matching
                yNETTYPE దత్తాంశం_type_No_Ref
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>3, *$3); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE దత్తాంశం_type_No_Ref
        /*cont*/   idAny/*nettype_identifier*/
        /*cont*/   yWITH__ETC packageClassScopeE id/*tf_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>3, *$3); BBUNSUP($<fl>1, "Unsupported: nettype with"); }
        |       yNETTYPE packageClassScopeE idAny packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>5, *$5); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE packageClassScopeE idAny packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/
        /*cont*/   yWITH__ETC packageClassScopeE id/*tf_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>5, *$5); BBUNSUP($<fl>1, "Unsupported: nettype with"); }
        |       yNETTYPE packageClassScopeE idAny parameter_విలువ_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE packageClassScopeE idAny parameter_విలువ_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/
        /*cont*/   yWITH__ETC packageClassScopeE id/*tf_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype with"); }
        ;

implicit_typeE<కణుపు_DTypep>:             // IEEE: part of *data_type_or_implicit
        //                      // Also expanded in data_declaration
                /*ఖాళీలు*/
                        { $$ = nullptr; }
        |       signingE rangeList
                        { $$ = GRAMMARP->కూడు_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        ;

assertion_variable_declaration<nodep>:  // IEEE: assertion_variable_declaration
        //                      // IEEE: var_data_type expanded
                var_దత్తాంశం_type                        { VARRESET_NONLIST(VAR); VARDTYPE_NDECL($1); }
        /*cont*/    list_యొక్క_variable_decl_assignments ';'     { $$ = $3; }
        ;

type_declaration<nodep>:        // ==IEEE: type_declaration
                                // Data_type expanded
                yTYPEDEF దత్తాంశం_type_No_Ref
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { Ast_కణుపు_DType* స్థిరరాశి dtp = $2;
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, dtp, $4); }

        // IEEE 1800-2023 6.18 typedef: dotted or arrayed type identifier
        // Handles interface typedef references like if0.rq_t and if0[0].rq_t (arrays allowed after first component)
        |       yTYPEDEF idDottedOrArrayed
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* స్థిరరాశి ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* స్థిరరాశి varp = VARDONEA($<fl>3, *$3, $4, $5);
                // idDottedOrArrayed produces Dot/SelBit tree for hierarchical refs like if0[0].rq_t
                varp->valuep($2);
                $$ = varp; }

        // IEEE 1800-2023 6.18 typedef with hierarchical type identifier
        // Special-case array on first component requiring a '.' after ']' to disambiguate from packed dims
        // Examples: typedef if0[0].rq_t my_t; typedef if0[0].x_if.rq_t my_t;
        |       yTYPEDEF id '[' సమాసం ']' '.' id_Dotted_Sel_మరిన్ని
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* స్థిరరాశి ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* స్థిరరాశి varp = VARDONEA($<fl>8, *$8, $9, $10);
                Ast_కణుపు_సమాసం* స్థిరరాశి arrp = new Ast_Sel_బిట్టు{$3, new AstParseRef{$<fl>2, *$2, nullptr, nullptr}, $4};
                varp->valuep(new AstDot{$6, false, arrp, $7});
                $$ = varp; }

        |       yTYPEDEF packageClassScope idAny packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>3, *$3, $2, nullptr};
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->createArray(refp, $4, true);
                          $$ = GRAMMARP->createTypedef($<fl>5, *$5, $7, dtp, $6); }
        |       yTYPEDEF packageClassScope idAny parameter_విలువ_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>3, *$3, $2, $4};
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->createArray(refp, $5, true);
                          $$ = GRAMMARP->createTypedef($<fl>6, *$6, $8, dtp, $7); }

        // Type alias without packed dimensions: typedef existing_t new_t;
        |       yTYPEDEF idAny idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, refp, $4); }

        // IEEE 1800-2023 6.18.2 typedef with packed dimensions on an existing type identifier
        // Disambiguated from interface array access by requiring ':' inside the brackets
        // (applies to both plain identifiers and type identifiers)
        |       yTYPEDEF id '[' const_సమాసం ':' const_సమాసం ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_కణుపు_Range* స్థిరరాశి rangep = new AstRange{$3, $4, $6};
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->createArray(refp, కూడు_తదుపరి_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        // Same as above but for type identifiers (parameter types, etc.)
        |       yTYPEDEF idType '[' const_సమాసం ':' const_సమాసం ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_కణుపు_Range* స్థిరరాశి rangep = new AstRange{$3, $4, $6};
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->createArray(refp, కూడు_తదుపరి_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        |       yTYPEDEF idAny parameter_విలువ_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, nullptr, $3};
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->createArray(refp, $4, true);
                          $$ = GRAMMARP->createTypedef($<fl>5, *$5, $7, dtp, $6); }

        //                      // idAny as also allows redeclaring same typedef again
        |       yTYPEDEF idAny ';'                      { $$ = GRAMMARP->createTypedefFwd($<fl>2, *$2, VFwdType::NONE); }
        //                      // IEEE: expanded forward_type to prevent conflict
        |       yTYPEDEF yENUM idAny ';'                { $$ = GRAMMARP->createTypedefFwd($<fl>3, *$3, VFwdType::ENUM); }
        |       yTYPEDEF ySTRUCT idAny ';'              { $$ = GRAMMARP->createTypedefFwd($<fl>3, *$3, VFwdType::STRUCT); }
        |       yTYPEDEF yUNION idAny ';'               { $$ = GRAMMARP->createTypedefFwd($<fl>3, *$3, VFwdType::UNION); }
        |       yTYPEDEF yCLASS idAny ';'               { $$ = GRAMMARP->createTypedefFwd($<fl>3, *$3, VFwdType::CLASS); }
        |       yTYPEDEF yINTERFACE yCLASS idAny ';'    { $$ = GRAMMARP->createTypedefFwd($<fl>4, *$4, VFwdType::INTERFACE_CLASS); }
        //
        |       yTYPEDEF error idAny ';'                { $$ = GRAMMARP->createTypedefFwd($<fl>3, *$3, VFwdType::NONE); }  // LCOV_EXCL_LINE
        ;

dtypeAttrListE<nodep>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       dtypeAttrList                           { $$ = $1; }
        ;

dtypeAttrList<nodep>:
                dtypeAttr                               { $$ = $1; }
        |       dtypeAttrList dtypeAttr                 { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

dtypeAttr<nodep>:
                y_VL_ప్రజా                              { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DT_ప్రజా}; }
        ;

vl_ట్యాగ్:                          // verilator tag handling
                y_VL_ట్యాగ్                                 { ఒకవేళ (PARSEP->ట్యాగ్_Nodep()) PARSEP->ట్యాగ్_Nodep()->ట్యాగ్(*$1); }
        ;

//************************************************
// Module Items

module_అంశం_List_E<nodep>:        // IEEE: Part of module_declaration
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       module_అంశం_List                         { $$ = $1; }
        ;

module_అంశం_List<nodep>:         // IEEE: Part of module_declaration
                module_అంశం                             { $$ = $1; }
        |       module_అంశం_List module_అంశం             { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

module_అంశం<nodep>:             // ==IEEE: module_item
                పోర్టు_declaration ';'                    { $$ = $1; }
        |       non_పోర్టు_module_అంశం                    { $$ = $1; }
        ;

non_పోర్టు_module_అంశం<nodep>:    // ==IEEE: non_port_module_item
                generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       gen_అంశం_Begin                            { $$ = $1; }
        |       module_or_generate_అంశం                 { $$ = $1; }
        |       specify_ఖండం                           { $$ = $1; }
        |       specparam_declaration                   { $$ = $1; }
        |       కార్యక్రమం_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within module decls"); }
        |       module_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: module decls within module decls"); }
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within module decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // Verilator specific
        |       vl_Sc_ఖండం                               { $$ = $1; }
        |       y_VL_HIER_ఖండం                          { $$ = new AstPragma{$1, VPragmaType::HIER_ఖండం}; }
        |       yVL_INLINE_MODULE                       { $$ = new AstPragma{$1, VPragmaType::INLINE_MODULE}; }
        |       yVL_NO_INLINE_MODULE                    { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_MODULE}; }
        |       y_VL_ప్రజా_MODULE                       { $$ = new AstPragma{$1, VPragmaType::ప్రజా_MODULE}; v3Global.dpi(true); }
        ;

vl_Sc_ఖండం<nodep>:  // Verilator-specific `systemc_* blocks
                yaSCHDR     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR, *$1};  }
        |       yaSCHDRP    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR_POST, *$1}; }
        |       yaSCINT     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::INT, *$1}; }
        |       yaSCIMP     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP, *$1}; }
        |       yaSCIMPH    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP_HDR, *$1}; }
        |       yaSCCTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::CTOR, *$1}; }
        |       yaSCDTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::DTOR, *$1}; }
        ;


module_or_generate_అంశం<nodep>: // ==IEEE: module_or_generate_item
        //                      // IEEE: parameter_override
                yDEFPARAM list_యొక్క_defparam_assignments ';'      { $$ = $2; }
        //                      // IEEE: gate_instantiation + udp_instantiation + module_instantiation
        //                      // not here, see etcInst in module_common_item
        //                      // We joined udp & module definitions, so this goes here
        |       combinational_body                      { $$ = $1; }
        //                      // This module_common_item shared with
        //                      // interface_or_generate_item:module_common_item
        |       module_common_అంశం                      { $$ = $1; }
        ;

module_common_అంశం<nodep>:      // ==IEEE: module_common_item
                module_or_generate_అంశం_declaration     { $$ = $1; }
        //                      // IEEE: interface_instantiation
        //                      // + IEEE: program_instantiation
        //                      // + module_instantiation from module_or_generate_item
        |       etcInst                                 { $$ = $1; }
        |       assertion_అంశం                          { $$ = $1; }
        |       bind_directive                          { $$ = $1; }
        |       continuous_కేటాయించు                       { $$ = $1; }
        |       net_alias                               { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       always_construct                        { $$ = $1; }
        |       loop_generate_construct                 { $$ = $1; }
        |       conditional_generate_construct          { $$ = $1; }
        |       severity_system_task                    { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        //
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

always_construct<nodep>:        // IEEE: == always_construct
                yALWAYS       stmt                 { $$ = new AstAlways{$1, VAlwaysKwd::ALWAYS, nullptr, $2}; }
        |       yALWAYS_FF    stmt                 { $$ = new AstAlways{$1, VAlwaysKwd::ALWAYS_FF, nullptr, $2}; }
        |       yALWAYS_LATCH stmt                 { $$ = new AstAlways{$1, VAlwaysKwd::ALWAYS_LATCH, nullptr, $2}; }
        |       yALWAYS_COMB  stmt                 { $$ = new AstAlways{$1, VAlwaysKwd::ALWAYS_COMB, nullptr, $2}; }
        ;

continuous_కేటాయించు<nodep>:       // IEEE: continuous_assign
                y_కేటాయించు driveStrengthE delay_controlE కేటాయించు_List ';'
                        { $$ = $4;
                          STRENGTH_LIST($4, $2);
                          DELAY_LIST($4, $3); }
        ;

initial_construct<nodep>:       // IEEE: initial_construct
                yINITIAL stmt                      { $$ = new AstInitial{$1, $2}; }
        ;


net_alias<nodep>:               // IEEE: net_alias
                yALIAS variable_lvalue aliasEqList ';'
                        { $2->కూడు_తదుపరి($3);
                          $$ = new AstAlias{$1, $2}; }
        ;

aliasEqList<కణుపు_Exprp>:                    // IEEE: part of net_alias
                '=' variable_lvalue                     { $$ = $2; }
        |       aliasEqList '=' variable_lvalue         { $$ = $1->కూడు_తదుపరి($3); }
        ;

final_construct<nodep>:         // IEEE: final_construct
                yFINAL stmt                        { $$ = new AstFinal{$1, $2}; }
        ;

module_or_generate_అంశం_declaration<nodep>:     // ==IEEE: module_or_generate_item_declaration
                package_or_generate_అంశం_declaration    { $$ = $1; }
        |       genvar_declaration                      { $$ = $1; }
        |       clocking_declaration                    { $$ = $1; }
        |       modDefaultClocking                      { $$ = $1; }
        |       defaultDisable                          { $$ = $1; }
        ;

modDefaultClocking<nodep>:  // IEEE: part of module_or_generate_item_declaration/checker_or_...
                yDEFAULT yCLOCKING idAny/*new-clocking_identifier*/ ';'
                        { $$ = new AstDefaultClocking{$<fl>2, *$3}; }
        ;

defaultDisable<nodep>:  // IEEE: part of module_/checker_or_generate_item_declaration
                yDEFAULT yDISABLE yIFF సమాసం/*expression_or_dist*/ ';'
                        { $$ = new AstDefaultDisable{$1, $4}; }
        ;

bind_directive<nodep>:          // ==IEEE: bind_directive + bind_target_scope
        //                      // ';' - Note IEEE grammar is wrong, includes extra ';'
        //                      // - it's already in module_instantiation
        //                      // We merged the rules - id may be a bind_target_instance or
        //                      // module_identifier or interface_identifier
                yBIND bind_లక్ష్యం_instance bind_instantiation   { $$ = new AstBind{$<fl>2, *$2, $3}; }
        |       yBIND bind_లక్ష్యం_instance ':' bind_లక్ష్యం_instance_list bind_instantiation
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: Bind with instance list"); DEL($5); }
        ;

bind_లక్ష్యం_instance_list:      // ==IEEE: bind_target_instance_list
                bind_లక్ష్యం_instance                    { }
        |       bind_లక్ష్యం_instance_list ',' bind_లక్ష్యం_instance      { }
        ;

bind_లక్ష్యం_instance<strp>:     // ==IEEE: bind_target_instance
        //UNSUP hierarchical_identifierBit              { }
                idAny                                   { $$ = $1; }
        ;

bind_instantiation<nodep>:      // ==IEEE: bind_instantiation
        //                      // IEEE: program_instantiation
        //                      // IEEE: + module_instantiation
        //                      // IEEE: + interface_instantiation
        //                      // Need to get an AstBind instead of AstCell, so have special rules
                instDecl                                { $$ = $1; }
        ;

//************************************************
// Generates
//
// Way down in generate_item is speced a difference between module,
// interface and checker generates.  modules and interfaces are almost
// identical (minus DEFPARAMs) so we overlap them.  Checkers are too
// different, so we copy all rules for checkers.

generate_region<nodep>:         // ==IEEE: generate_region
                yGENERATE ~c~gen_అంశం_List yENDGENERATE   { $$ = $2; }
        |       yGENERATE yENDGENERATE                  { $$ = nullptr; }
        ;

c_generate_region<nodep>:  // IEEE: generate_region (for checkers)
                BISONPRE_COPY(generate_region,{s/~c~/c_/g})     // {copied}
        ;

i_generate_region<nodep>:  // IEEE: generate_region (for interface)
                BISONPRE_COPY(generate_region,{s/~c~/i_/g})     // {copied}
        ;

generate_ఖండం_or_null<nodep>:  // IEEE: generate_block_or_null (called from gencase/genif/genfor)
        //      ';'             // is included in
        //                      // IEEE: generate_block
        //                      // Must always return a BEGIN node, or nullptr - see GenFor construction
                ~c~generate_అంశం
                        { $$ = $1 ? (new Ast_Gen_ఖండం{$1->fileline(), "", $1, true}) : nullptr; }
        |       ~c~gen_అంశం_Begin                         { $$ = $1; }
        ;

c_generate_ఖండం_or_null<nodep>:  // IEEE: generate_block_or_null (for checkers)
                BISONPRE_COPY(generate_ఖండం_or_null,{s/~c~/c_/g})      // {copied}
        ;

gen_అంశం_Begin<nodep>:            // IEEE: part of generate_block
                yBEGIN ~c~gen_అంశం_List y_చివర              { $$ = new Ast_Gen_ఖండం{$1, "", $2, false}; }
        |       yBEGIN y_చివర                             { $$ = nullptr; }
        |       id yP_COLON__BEGIN yBEGIN ~c~gen_అంశం_List y_చివర చివర_Label_E
                        { $$ = new Ast_Gen_ఖండం{$<fl>1, *$1, $4, false};
                          GRAMMARP->చివర_Label($<fl>6, *$1, $6); }
        |       id yP_COLON__BEGIN yBEGIN y_చివర చివర_Label_E
                        { $$ = new Ast_Gen_ఖండం{$<fl>1, *$1, nullptr, false};
                          GRAMMARP->చివర_Label($<fl>5, *$1, $5); }
        |       yBEGIN ':' idAny ~c~gen_అంశం_List y_చివర చివర_Label_E
                        { $$ = new Ast_Gen_ఖండం{$<fl>3, *$3, $4, false};
                          GRAMMARP->చివర_Label($<fl>6, *$3, $6); }
        |       yBEGIN ':' idAny y_చివర చివర_Label_E
                        { $$ = new Ast_Gen_ఖండం{$<fl>3, *$3, nullptr, false};
                          GRAMMARP->చివర_Label($<fl>5, *$3, $5); }
        ;

c_gen_అంశం_Begin<nodep>:  // IEEE: part of generate_block (for checkers)
                BISONPRE_COPY(gen_అంశం_Begin,{s/~c~/c_/g})                // {copied}
        ;

i_gen_అంశం_Begin<nodep>:  // IEEE: part of generate_block (for interfaces)
                BISONPRE_COPY(gen_అంశం_Begin,{s/~c~/i_/g})                // {copied}
        ;

gen_అంశం_Or_Begin<nodep>:          // Not in IEEE, but our begin isn't under generate_item
                ~c~generate_అంశం                        { $$ = $1; }
        |       ~c~gen_అంశం_Begin                         { $$ = $1; }
        ;

c_gen_అంశం_Or_Begin<nodep>:  // (for checkers)
                BISONPRE_COPY(gen_అంశం_Or_Begin,{s/~c~/c_/g})      // {copied}
        ;

i_gen_అంశం_Or_Begin<nodep>:  // (for interfaces)
                interface_అంశం                          { $$ = $1; }
        |       i_gen_అంశం_Begin                          { $$ = $1; }
        ;

gen_అంశం_List<nodep>:
                ~c~gen_అంశం_Or_Begin                       { $$ = $1; }
        |       ~c~gen_అంశం_List ~c~gen_అంశం_Or_Begin        { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

c_gen_అంశం_List<nodep>:  // (for checkers)
                BISONPRE_COPY(gen_అంశం_List,{s/~c~/c_/g})         // {copied}
        ;

i_gen_అంశం_List<nodep>:  // (for interfaces)
                BISONPRE_COPY(gen_అంశం_List,{s/~c~/i_/g})         // {copied}
        ;

generate_అంశం<nodep>:           // IEEE: module_or_interface_or_generate_item
        //                      // Only legal when in a generate under a module (or interface under a module)
                module_or_generate_అంశం                 { $$ = $1; }
        //                      // Only legal when in a generate under an interface
        //UNSUP interface_or_generate_item              { $$ = $1; }
        //                      // IEEE: checker_or_generate_item
        //                      // Only legal when in a generate under a checker
        //                      // so below in c_generate_item
        ;

c_generate_అంశం<nodep>:  // IEEE: generate_item (for checkers)
                checker_or_generate_అంశం                { $$ = $1; }
        ;

conditional_generate_construct<nodep>:  // ==IEEE: conditional_generate_construct
                yCASE '(' సమాసం_Type_Compare ')' ~c~case_generate_అంశం_List yENDCASE
                        { $$ = new AstGenCase{$1, $3, $5}; }
        |       yIF '(' సమాసం_Type_Compare ')' ~c~generate_ఖండం_or_null %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstGenIf{$1, $3, $5, nullptr}; }
        |       yIF '(' సమాసం_Type_Compare ')' ~c~generate_ఖండం_or_null yELSE ~c~generate_ఖండం_or_null
                        { $$ = new AstGenIf{$1, $3, $5, $7}; }
        ;

c_conditional_generate_construct<nodep>:  // IEEE: conditional_generate_construct (for checkers)
                BISONPRE_COPY(conditional_generate_construct,{s/~c~/c_/g})      // {copied}
        ;

loop_generate_construct<nodep>: // ==IEEE: loop_generate_construct
                yFOR '(' genvar_initialization ';' సమాసం ';' genvar_iteration ')' ~c~generate_ఖండం_or_null
                        { // Convert BEGIN(...) to BEGIN(GENFOR(...)), as we need the BEGIN to hide the local genvar
                          Ast_Gen_ఖండం* lowerp = VN_CAST($9, Gen_ఖండం);
                          UASSERT_OBJ(!$9 || lowerp, $9, "Child of GENFOR should have been begin");
                          Ast_కణుపు* స్థిరరాశి itemsp = lowerp && lowerp->itemsp() ? lowerp->itemsp()->unlink_Fr_వెనుక_With_తదుపరి() : nullptr;
                          Ast_Gen_ఖండం* స్థిరరాశి blkp = new Ast_Gen_ఖండం{$1, lowerp ? lowerp->name() : "", nullptr, true};
                          // V3LinkDot detects BEGIN(GENFOR(...)) as a special case
                          Ast_కణుపు* initp = $3;
                          Ast_కణుపు* స్థిరరాశి varp = $3;
                          ఒకవేళ (VN_IS(varp, Var)) {  // Genvar
                                initp = varp->nextp();
                                initp->unlink_Fr_వెనుక_With_తదుపరి();  // Detach 2nd from varp, make 1st init
                                blkp->కూడు_Itemsp(varp);
                          }
                          // Statements are under 'genforp' as instances under this
                          // for loop won't get an extra layer of hierarchy tacked on
                          blkp->genforp(new AstGenFor{$1, initp, $5, $7, itemsp});
                          $$ = blkp;
                          DEL(lowerp);
                        }
        ;

c_loop_generate_construct<nodep>:  // IEEE: loop_generate_construct (for checkers)
                BISONPRE_COPY(loop_generate_construct,{s/~c~/c_/g})     // {copied}
        ;

genvar_initialization<nodep>:   // ==IEEE: genvar_initialization
                parse_Ref_పునాది '=' సమాసం                     { $$ = new Ast_కేటాయించు{$2, $1, $3}; }
        |       yGENVAR genvar_identifierDecl '=' const_సమాసం
                        { $$ = $2; Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>($$,
                                       new Ast_కేటాయించు{$3, new AstVarRef{$2->fileline(), $2, VAccess::WRITE}, $4}); }
        ;

genvar_iteration<nodep>:        // ==IEEE: genvar_iteration
                parse_Ref_పునాది '='          సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, $3}; }
        |       parse_Ref_పునాది yP_PLUSEQ    సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new Ast_కూడు{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_MINUSEQ   సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstSub{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_TIMESEQ   సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstMul{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_DIVEQ     సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstDiv{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_MODEQ     సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstModDiv{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_ANDEQ     సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstAnd{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_OREQ      సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstOr{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_XOREQ     సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstXor{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_SLEFTEQ   సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new Ast_జరుపు_L{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_SRIGHTEQ  సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new Ast_జరుపు_R{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        |       parse_Ref_పునాది yP_SSRIGHTEQ సమాసం
                        { $$ = new Ast_కేటాయించు{$2, $1, new Ast_జరుపు_RS{$2, $1->clone_వృక్షం_శుద్ధ(true), $3}}; }
        //                      // inc_or_dec_operator
        |       yP_PLUSPLUS   parse_Ref_పునాది
                        { $$ = new Ast_కేటాయించు{$1, $2, new Ast_కూడు{$1, $2->clone_వృక్షం_శుద్ధ(true),
                                                                new AstConst{$1, AstConst::String_గమ్యం_Parse{}, "'b1"}}}; }
        |       yP_MINUSMINUS parse_Ref_పునాది
                        { $$ = new Ast_కేటాయించు{$1, $2, new AstSub{$1, $2->clone_వృక్షం_శుద్ధ(true),
                                                                new AstConst{$1, AstConst::String_గమ్యం_Parse{}, "'b1"}}}; }
        |       parse_Ref_పునాది yP_PLUSPLUS
                        { $$ = new Ast_కేటాయించు{$2, $1, new Ast_కూడు{$2, $1->clone_వృక్షం_శుద్ధ(true),
                                                                new AstConst{$2, AstConst::String_గమ్యం_Parse{}, "'b1"}}}; }
        |       parse_Ref_పునాది yP_MINUSMINUS
                        { $$ = new Ast_కేటాయించు{$2, $1, new AstSub{$2, $1->clone_వృక్షం_శుద్ధ(true),
                                                                new AstConst{$2, AstConst::String_గమ్యం_Parse{}, "'b1"}}}; }
        ;

case_generate_అంశం_List<genCaseItemp>:  // IEEE: { case_generate_itemList }
                ~c~case_generate_అంశం                   { $$ = $1; }
        |       ~c~case_generate_అంశం_List ~c~case_generate_అంశం         { $$ = $1; $1->కూడు_తదుపరి($2); }
        ;

c_case_generate_అంశం_List<genCaseItemp>:  // IEEE: { case_generate_item } (for checkers)
                BISONPRE_COPY(case_generate_అంశం_List,{s/~c~/c_/g})      // {copied}
        ;

case_generate_అంశం<genCaseItemp>:      // ==IEEE: case_generate_item
                caseCondList colon ~c~generate_ఖండం_or_null    { $$ = new Ast_Gen_Case_అంశం{$2, $1, $3}; }
        |       yDEFAULT colon ~c~generate_ఖండం_or_null        { $$ = new Ast_Gen_Case_అంశం{$1, nullptr, $3}; }
        |       yDEFAULT ~c~generate_ఖండం_or_null              { $$ = new Ast_Gen_Case_అంశం{$1, nullptr, $2}; }
        ;

c_case_generate_అంశం<genCaseItemp>:  // IEEE: case_generate_item (for checkers)
                BISONPRE_COPY(case_generate_అంశం,{s/~c~/c_/g})  // {copied}
        ;

//************************************************
// Assignments and register declarations

కేటాయించు_List<alwaysp>:
                కేటాయించు_One                               { $$ = $1; }
        |       కేటాయించు_List ',' కేటాయించు_One                { $$ = $1->కూడు_తదుపరి($3); }
        ;

కేటాయించు_One<alwaysp>:
                variable_lvalue '=' సమాసం                { Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$2, $1, $3};
                                                          $$ = new AstAlways{ap}; }
        ;

delay_or_event_controlE<nodep>:  // IEEE: delay_or_event_control plus empty
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        |       event_control                           { $$ = $1; }
        |       yREPEAT '(' సమాసం ')' event_control
                        { $$ = $5; BBUNSUP($1, "Unsupported: repeat event control"); }
        ;

delay_controlE<delayp>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        ;

delay_control<delayp>:   //== IEEE: delay_control
                '#' delay_విలువ
                        { $$ = new AstDelay{$<fl>1, $2, false}; }
        |       '#' '(' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; }
        |       '#' '(' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false};
                          $$->fallDelay($5); }
        |       '#' '(' minTypMax ',' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; $$->fallDelay($5); RISEFALLDLYUNSUP($7); DEL($7); }
        ;

delay_విలువ<కణుపు_Exprp>:         // ==IEEE:delay_value
        //                      // IEEE: ps_identifier
                idClass                                 { $$ = $1; }
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       సమయం_అంకె_Adjusted                         { $$ = $1; }
        |       y1_అడుగు                                  { $$ = new AstConst{$<fl>1, AstConst::One_అడుగు{}}; }
        ;

minTypMax<కణుపు_Exprp>:           // IEEE: mintypmax_expression and constant_mintypmax_expression
                సమాసం                               { $$ = $1; }
        |       సమాసం ':' సమాసం ':' సమాసం { $$ = $3; MINTYPMAXDLYUNSUP($3); DEL($1); DEL($5); }
        ;

minTypMaxE<కణుపు_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       minTypMax                               { $$ = $1; }
        ;

netSigList<varp>:               // IEEE: list_of_port_identifiers
                netSig                                  { $$ = $1; }
        |       netSigList ',' netSig                   { $$ = $1; $1->కూడు_తదుపరి($3); }
        ;

netSig<varp>:                   // IEEE: net_decl_assignment -  one element from list_of_port_identifiers
                netId variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       netId variable_dimensionListE sigAttrListE '=' సమాసం
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          AstDelay* స్థిరరాశి delayp = $$->delayp() ? $$->delayp()->unlink_Fr_వెనుక() : nullptr;
                          Ast_కేటాయించు_W* స్థిరరాశి assignp = new Ast_కేటాయించు_W{$4, new AstParseRef{$<fl>1, *$1}, $5, delayp};
                          ఒకవేళ (GRAMMARP->m_netStrengthp) assignp->strengthSpecp(GRAMMARP->m_netStrengthp->clone_వృక్షం(false));
                          Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>($$, new AstAlways{assignp}); }
        ;

netId<strp>:
                id/*new-net*/                           { $$ = $1; $<fl>$ = $<fl>1; }
        |       idSVKwd                                 { $$ = $1; $<fl>$ = $<fl>1; }
        ;

sigAttrScope:
                y_VL_ప్రజా_FLAT_RW_ON_SNS attr_event_control
                                                        { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ప్రజా_FLAT_RW);
                                                          v3Global.dpi(true); DEL($2); }
        |       y_VL_ప్రజా_ON                           { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ప్రజా); }
        |       y_VL_ప్రజా_FLAT_ON                      { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ప్రజా_FLAT); }
        |       y_VL_ప్రజా_FLAT_RD_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ప్రజా_FLAT_RD); }
        |       y_VL_ప్రజా_FLAT_RW_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ప్రజా_FLAT_RW); }
        |       y_VL_ప్రజా_OFF                          { GRAMMARP->సమితి_Scoped_Sig_Attr(nullptr); }
        ;

sigAttrListE<nodep>: // Scoped Attributes are added to explicit attributes
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       sigAttrList                             { $$ = $1; }
        ;

sigAttrList<nodep>:
                sigAttr                                 { $$ = $1; }
        |       sigAttrList sigAttr                     { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

sigAttr<nodep>:
                yVL_CLOCKER                             { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_NO_CLOCKER                          { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_CLOCK_ENABLE                        { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_FORCEABLE                           { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FORCEABLE}; }
        |       y_VL_ప్రజా                              { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_ప్రజా}; v3Global.dpi(true); }
        |       y_VL_ప్రజా_FLAT                         { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_ప్రజా_FLAT}; v3Global.dpi(true); }
        |       y_VL_ప్రజా_FLAT_RD                      { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_ప్రజా_FLAT_RD}; v3Global.dpi(true); }
        |       y_VL_ప్రజా_FLAT_RW attr_event_controlE  { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_ప్రజా_FLAT_RW}; v3Global.dpi(true); DEL($2); }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_SC_BIGUINT                          { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_SC_BIGUINT}; }
        |       yVL_SC_BV                               { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_SC_BV}; }
        |       yVL_SFORMAT                             { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_SFORMAT}; }
        |       y_VL_విభజించు_VAR                           { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_విభజించు_VAR}; }
        |       yVL_FSM_ARC_INCL_COND                   { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO              { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO_EXPAND       { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND}; }
        |       y_VL_FSM_ARC_INCL_COND_నిండిన              { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_నిండిన}; }
        |       yVL_FSM_STATE                           { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_STATE}; }
        |       yVL_FSM_STATE_AUTO                      { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_STATE_AUTO}; }
        |       yVL_FSM_STATE_AUTO_EXPAND               { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_STATE_AUTO_EXPAND}; }
        |       y_VL_FSM_STATE_నిండిన                      { $$ = new Ast_Attr_యొక్క{$1, VAttrType::VAR_FSM_STATE_నిండిన}; }
        ;

rangeListE<కణుపు_Rangep>:         // IEEE: [{packed_dimension}]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       rangeList                               { $$ = $1; }
        ;

rangeList<కణుపు_Rangep>:          // IEEE: {packed_dimension}
                anyrange                                { $$ = $1; }
        |       rangeList anyrange                      { $$ = $1->కూడు_తదుపరి($2); }
        ;

anyrange<కణుపు_Rangep>:
                '[' const_సమాసం ':' const_సమాసం ']'         { $$ = new AstRange{$1, $2, $4}; }
        ;

part_select_rangeList<కణుపు_Pre_Selp>:  // IEEE: part_select_range (as used after function calls)
                part_select_range                        { $$ = $1; }
        |       part_select_rangeList part_select_range  { $$ = GRAMMARP->scrubSel($1, $2); }
        ;

part_select_range<కణుపు_Pre_Selp>:
                '[' సమాసం ']'
                        { $$ = new Ast_Sel_బిట్టు{$1, new AstParseHolder{$1}, $2}; }
        |       '[' const_సమాసం ':' const_సమాసం ']'
                        { $$ = new Ast_Sel_వెలికితీయి{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' సమాసం yP_PLUSCOLON const_సమాసం ']'
                        { $$ = new AstSelPlus{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' సమాసం yP_MINUSCOLON const_సమాసం ']'
                        { $$ = new AstSelMinus{$1, new AstParseHolder{$1}, $2, $4}; }
        ;

packed_dimensionListE<కణుపు_Rangep>:      // IEEE: [{ packed_dimension }]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       packed_dimensionList                    { $$ = $1; }
        ;

packed_dimensionList<కణుపు_Rangep>:       // IEEE: { packed_dimension }
                packed_dimension                        { $$ = $1; }
        |       packed_dimensionList packed_dimension   { $$ = $1->కూడు_తదుపరి($2); }
        ;

packed_dimension<కణుపు_Rangep>:   // ==IEEE: packed_dimension
                anyrange                                { $$ = $1; }
        |       '[' ']'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: [] dimensions"); }
        ;

//************************************************
// Parameters

param_assignment<varp>:         // ==IEEE: param_assignment
        //                      // IEEE: constant_param_expression
        //                      // constant_param_expression: '$' is in expr
                id/*new-parameter*/ variable_dimensionListE sigAttrListE సమాసం_Or_దత్తాంశం_Type_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          ఒకవేళ ($4) $$->valuep($4);
                          లేకపోతే ఒకవేళ (!GRAMMARP->m_pinAnsi)
                              $$->v3warn(PARAMNODEFAULT, "Parameter without default requires"
                                         " ANSI-style parameter list (IEEE 1800-2023 6.20.1): "
                                         << $$->prettyNameQ()); }
        ;

list_యొక్క_param_assignments<varp>:        // ==IEEE: list_of_param_assignments
                param_assignment                        { $$ = $1; }
        |       list_యొక్క_param_assignments ',' param_assignment  { $$ = $1->కూడు_తదుపరి($3); }
        ;

type_assignment<varp>:          // ==IEEE: type_assignment
        //                      // note exprOrDataType being a data_type is only for yPARAMETER yTYPE
        //                      // Using exprOrDataType allows hierarchical refs like if0.rq_t
        //                      // which get resolved to types during linking
                idAny/*new-parameter*/ sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        |       idAny/*new-parameter*/ sigAttrListE '=' సమాసం_Or_దత్తాంశం_Type
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          // V3LinkParse will wrap this in RequireDType when creating ParamTypeDType
                          $$->valuep($4); }
        ;

list_యొక్క_type_assignments<varp>:         // ==IEEE: list_of_type_assignments
                type_assignment                         { $$ = $1; }
        |       list_యొక్క_type_assignments ',' type_assignment
                        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

list_యొక్క_defparam_assignments<nodep>:    //== IEEE: list_of_defparam_assignments
                defparam_assignment                     { $$ = $1; }
        |       list_యొక్క_defparam_assignments ',' defparam_assignment
                        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

defparam_assignment<nodep>:     // ==IEEE: defparam_assignment
                defparamIdRangeList '.' defparamIdRange '=' సమాసం
                        { $$ = new AstDefParam{$4, $1, *$3, $5}; }
        |       defparamIdRange '=' సమాసం
                        { $$ = nullptr; BBUNSUP($2, "Unsupported: defparam with no dot");
                          DEL($3); }
        ;

defparamIdRangeList<కణుపు_Exprp>:  // IEEE: part of defparam_assignment
                defparam_Id_Range_సమాసం                     { $$ = $1; }
        |       defparamIdRangeList '.' defparam_Id_Range_సమాసం
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

defparam_Id_Range_సమాసం<కణుపు_Exprp>:  // IEEE: part of defparam_assignment
                idAny
                        { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        |       idAny part_select_rangeList
                        { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr};
                          BBUNSUP($2, "Unsupported: defparam with arrayed instance");
                          DEL($2); }
        ;

defparamIdRange<strp>:  // IEEE: part of defparam_assignment
                idAny
                        { $$ = $1; }
        |       idAny part_select_rangeList
                        { $$ = $1; BBUNSUP($2, "Unsupported: defparam with arrayed instance");
                          DEL($2); }
        ;

//************************************************
// Instances
// We don't know identifier types, so this matches all module,udp,etc instantiation
//   module_id      [#(params)]   name  (pins) [, name ...] ;   // module_instantiation
//   gate (strong0) [#(delay)]   [name] (pins) [, (pins)...] ;  // gate_instantiation
//   program_id     [#(params}]   name ;                        // program_instantiation
//   interface_id   [#(params}]   name ;                        // interface_instantiation
//   checker_id                   name  (pins) ;                // checker_instantiation

etcInst<nodep>:                 // IEEE: module_instantiation + gate_instantiation + udp_instantiation
                instDecl                                { $$ = $1; }
        |       gateDecl                                { $$ = $1; }
        ;

instDecl<nodep>:
        //                      // Disambigurated from data_declaration based on
        //                      // idInst which is found as IEEE requires a later '('
                idInst parameter_విలువ_assignment_Inst_E
        /*mid*/         { INSTPREP($<fl>1, *$1, $2); }
        /*cont*/    instnameList ';'
                        { $$ = $4;
                          GRAMMARP->m_impliedDecl = false;
                          ఒకవేళ (GRAMMARP->m_instParamp) {
                              VL_DO_CLEAR(GRAMMARP->m_instParamp->delete_వృక్షం(),
                                          GRAMMARP->m_instParamp = nullptr);
                          } }
        //
        //                      // IEEE: part of udp_instance when no name_of_instance
        //                      // Note no unpacked dimension nor list of instances
        |       id
        /*mid*/         { INSTPREP($<fl>1, *$1, nullptr); }
        /*cont*/    instnameParenUdpn ';'
                        { $$ = $3; GRAMMARP->m_impliedDecl = false; }
        ;

mpInstnameList<nodep>:          // Similar to instnameList, but for modport instantiations which have no parenthesis
                mpInstnameParen                         { $$ = $1; }
        |       mpInstnameList ',' mpInstnameParen      { $$ = $1->కూడు_తదుపరి($3); }
        ;

mpInstnameParen<nodep>:         // Similar to instnameParen, but for modport instantiations which have no parenthesis
                id instRangeListE sigAttrListE          { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        ;

instnameList<nodep>:
                instnameParen                           { $$ = $1; }
        |       instnameList ',' instnameParen          { $$ = $1->కూడు_తదుపరి($3); }
        ;

instnameParen<nodep>:
                id instRangeListE '(' instPinListE ')'
                        { $$ = GRAMMARP->createCell($<fl>1, *$1, $4, $2); }
        ;

instnameParenUdpn<nodep>:  // IEEE: part of udp_instance when no name_of_instance
                '(' instPinListE ')'  // When UDP has empty name, unpacked dimensions must not be used
                        { $$ = GRAMMARP->createCell($<fl>1, "", $2, nullptr); }
        ;

instRangeListE<కణుపు_Rangep>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       instRangeList                           { $$ = $1; }
        ;

instRangeList<కణుపు_Rangep>:
                instRange                               { $$ = $1; }
        |       instRangeList instRange                 { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

instRange<కణుపు_Rangep>:
                '[' const_సమాసం ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstSub{$1, $2, new AstConst{$1, 1}}, true}; }
        |       '[' const_సమాసం ':' const_సమాసం ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

instParamListE<pinp>:
                { GRAMMARP->pin_నెట్టు(); } instParamItListE   { $$ = $2; GRAMMARP->pin_తీయి(CRELINE()); }
        ;

instPinListE<pinp>:
                { VARRESET_LIST(UNKNOWN); } instPinItListE   { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

instParamItListE<pinp>:         // IEEE: list_of_parameter_value_assignments/list_of_parameter_assignments
        //                      // Empty gets a node, to track class reference of #()
                /*empty*/                               { $$ = new AstPin{CRELINE(), PINNUMINC(), "", nullptr}; }
        |       instParamItList                         { $$ = $1; }
        ;

instParamItList<pinp>:          // IEEE: list_of_parameter_value_assignments/list_of_parameter_assignments
                inst_Param_అంశం                           { $$ = $1; }
        |       instParamItList ',' inst_Param_అంశం       { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

instPinItListE<pinp>:           // IEEE: list_of_port_connections
                inst_Pin_అంశం_E                            { $$ = $1; }
        |       instPinItListE ',' inst_Pin_అంశం_E         { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

inst_Param_అంశం<pinp>:            // IEEE: named_parameter_assignment + empty
        //                      // Note empty is not allowed in parameter lists
                yP_DOTSTAR                              { $$ = new AstPin{$1, PINNUMINC(), ".*", nullptr}; }
        |       '.' idAny '(' ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, nullptr};
                          $$->svDotName(true); }
        |       '.' idSVKwd
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2,
                                          new AstParseRef{$<fl>2, *$2, nullptr, nullptr}};
                          $$->svDotName(true); $$->svImplicit(true); }
        |       '.' idAny
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2,
                                          new AstParseRef{$<fl>2, *$2, nullptr, nullptr}};
                          $$->svDotName(true); $$->svImplicit(true); }
        //                      // mintypmax is expanded here, as it might be a UDP or gate primitive
        //                      // data_type for 'parameter type' hookups
        |       '.' idAny '(' సమాసం_Or_దత్తాంశం_Type ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $4};
                          $$->svDotName(true); }
        //UNSUP | '.' idAny '(' exprOrDataType/*expr*/ ':' expr ')'
        //UNSUP          { MINTYPMAXDLYUNSUP($4); DEL($4);
        //UNSUP            $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $6};
        //UNSUP            $$->svDotName(true); }
        //UNSUP | '.' idAny '(' exprOrDataType/*expr*/ ':' expr ':' expr ')'
        //UNSUP          { MINTYPMAXDLYUNSUP($4); DEL($4); DEL($8);
        //UNSUP            $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $6};
        //UNSUP            $$->svDotName(true); }
        //                      // data_type for 'parameter type' hookups
        |       సమాసం_Or_దత్తాంశం_Type
                        { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1); DEL($5);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        ;

inst_Pin_అంశం_E<pinp>:             // IEEE: named_port_connection + empty
        //                      // Note empty can match either () or (,); V3LinkCells cleans up ()
                /* empty: ',,' is legal */              { $$ = new AstPin{CRELINE(), PINNUMINC(), "", nullptr}; }
        |       yP_DOTSTAR                              { $$ = new AstPin{$1, PINNUMINC(), ".*", nullptr}; }
        |       '.' idAny '(' ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, nullptr};
                          $$->svDotName(true); }
        |       '.' idSVKwd
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2,
                                          new AstParseRef{$<fl>2, *$2, nullptr, nullptr}};
                          $$->svDotName(true); $$->svImplicit(true); }
        |       '.' idAny
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2,
                                          new AstParseRef{$<fl>2, *$2, nullptr, nullptr}};
                          $$->svDotName(true); $$->svImplicit(true); }
        //                      // mintypmax is expanded here, as it might be a UDP or gate primitive
        //UNSUP               pev_expr below
        |       '.' idAny '(' సమాసం ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $4};
                          $$->svDotName(true); }
        //UNSUP '.' idAny '(' pev_expr ':' expr ')'     { }
        //UNSUP '.' idAny '(' pev_expr ':' expr ':' expr ')' { }
        //
        |       సమాసం                                    { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP expr ':' expr                           { }
        //UNSUP expr ':' expr ':' expr                  { }
        ;

//************************************************
// EventControl lists

attr_event_controlE<senTreep>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       attr_event_control                      { $$ = $1; }
        ;

attr_event_control<senTreep>:   // ==IEEE: event_control
                '@' '(' event_వ్యక్తీకరణ ')'            { $$ = new Ast_Sen_వృక్షం{$1, $3}; }
        |       '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_వృక్షం_Dot_నక్షత్రం($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_వృక్షం_Dot_నక్షత్రం($1); }
        ;

event_control<senTreep>:        // ==IEEE: event_control
        // UNSUP: Needs alignment with IEEE event_control and clocking_event
                '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_వృక్షం_Dot_నక్షత్రం($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_వృక్షం_Dot_నక్షత్రం($1); }
        //                      // IEEE: clocking_event
        |       '@' '(' event_వ్యక్తీకరణ ')'            { $$ = new Ast_Sen_వృక్షం{$1, $3}; }
        //                      // IEEE: hierarchical_event_identifier
        //                      // UNSUP below should be idClassSel
        |       '@' senitemVar                          { $$ = new Ast_Sen_వృక్షం{$1, $2}; } /* For events only */
        //                      // IEEE: sequence_instance
        //                      // sequence_instance without parens matches idClassSel above.
        //                      // Ambiguity: "'@' sequence (-for-sequence" versus
        //                      // expr:delay_or_event_controlE "'@' id (-for-expr
        //                      // For now we avoid this, as it's very unlikely someone would mix
        //                      // 1995 delay with a sequence with parameters.
        //                      // Alternatively split this out of event_control, and delay_or_event_controlE
        //                      // and anywhere delay_or_event_controlE is called allow two expressions
        //UNSUP '@' idClassSel '(' list_of_argumentsE ')'       { }
        ;

event_వ్యక్తీకరణ<senItemp>:     // IEEE: event_expression - split over several
        //UNSUP                 // Below are all removed
                senitem                                 { $$ = $1; }
        |       event_వ్యక్తీకరణ yOR senitem            { $$ = కూడు_తదుపరి_Null($1, $3); }
        |       event_వ్యక్తీకరణ ',' senitem            { $$ = కూడు_తదుపరి_Null($1, $3); } /* Verilog 2001 */
        //UNSUP                 // Above are all removed, replace with:
        //UNSUP ev_expr                                 { $$ = $1; }
        //UNSUP event_expression ',' ev_expr %prec yOR  { $$ = addNextNull($1, $3); }
        ;

senitem<senItemp>:              // IEEE: part of event_expression, non-'OR' ',' terms
                senitem_అంచు                             { $$ = $1; }
        |       సమాసం
                        { $$ = new Ast_Sen_అంశం{$<fl>1, VEdgeType::ET_మారింది, $1}; }
        |       సమాసం yIFF సమాసం
                        { $$ = new Ast_Sen_అంశం{$<fl>1, VEdgeType::ET_మారింది, $1, $3}; }
        ;

senitemVar<senItemp>:
                idClassSel
                        { $$ = new Ast_Sen_అంశం{$1->fileline(), VEdgeType::ET_మారింది, $1}; }
        ;

senitem_అంచు<senItemp>:          // IEEE: part of event_expression
                yPOSEDGE సమాసం
                        { $$ = new Ast_Sen_అంశం{$1, VEdgeType::ET_POSEDGE, $2}; }
        |       yNEGEDGE సమాసం
                        { $$ = new Ast_Sen_అంశం{$1, VEdgeType::ET_NEGEDGE, $2}; }
        |       y_అంచు సమాసం
                        { $$ = new Ast_Sen_అంశం{$1, VEdgeType::ET_BOTHEDGE, $2}; }
        |       yPOSEDGE సమాసం yIFF సమాసం
                        { $$ = new Ast_Sen_అంశం{$1, VEdgeType::ET_POSEDGE, $2, $4}; }
        |       yNEGEDGE సమాసం yIFF సమాసం
                        { $$ = new Ast_Sen_అంశం{$1, VEdgeType::ET_NEGEDGE, $2, $4}; }
        |       y_అంచు సమాసం yIFF సమాసం
                        { $$ = new Ast_Sen_అంశం{$1, VEdgeType::ET_BOTHEDGE, $2, $4}; }
        ;

//************************************************
// Statements

seq_ఖండం<beginp>:               // ==IEEE: seq_block
        //                      // IEEE doesn't allow declarations in unnamed blocks, but several simulators do.
        //                      // So need AstBegin's even if unnamed to scope variables down
                yBEGIN ప్రారంభం_Label_E ఖండం_Decl_List_E stmtListE y_చివర చివర_Label_E
                        {
                            $$ = new AstBegin{$1, $2 ? *$2 : "", nullptr, false};
                            GRAMMARP->చివర_Label($<fl>6, $$, $6);
                            $$->కూడు_Declsp($3);
                            $$->కూడు_Stmtsp($4);
                        }
        ;

seq_ఖండం_Pre_Id<beginp>:          // IEEE: seq_block, but called with leading ID
                id yP_COLON__BEGIN yBEGIN ఖండం_Decl_List_E stmtListE y_చివర చివర_Label_E
                        {
                            $$ = new AstBegin{$3, *$1, nullptr, false};
                            GRAMMARP->చివర_Label($<fl>7, $$, $7);
                            $$->కూడు_Declsp($4);
                            $$->కూడు_Stmtsp($5);
                        }
        ;

par_ఖండం_Join<joinType>:
                yJOIN       { $$ = VJoinType::JOIN; }
        |       yJOIN_ANY   { $$ = VJoinType::JOIN_ANY; }
        |       yJOIN_NONE  { $$ = VJoinType::JOIN_NONE; }
        ;

par_ఖండం<forkp>:               // ==IEEE: par_block
                yFORK ప్రారంభం_Label_E ఖండం_Decl_List_E stmtListE par_ఖండం_Join చివర_Label_E
                        {
                            $$ = new AstFork{$1, $5, $2 ? *$2 : ""};
                            GRAMMARP->చివర_Label($<fl>6, $$, $6);
                            $$->కూడు_Declsp($3);
                            $$->కూడు_Forksp(V3ParseGrammar::wrap_లోపల_Begin($4));
                        }
        ;

par_ఖండం_Pre_Id<forkp>:          // ==IEEE: par_block but called with leading ID
                id yP_COLON__FORK yFORK ఖండం_Decl_List_E stmtListE par_ఖండం_Join చివర_Label_E
                        {
                            $$ = new AstFork{$3, $6, *$1};
                            GRAMMARP->చివర_Label($<fl>7, $$, $7);
                            $$->కూడు_Declsp($4);
                            $$->కూడు_Forksp(V3ParseGrammar::wrap_లోపల_Begin($5));
                        }
            ;

ఖండం_Decl_List_E<nodep>:      // IEEE: [ block_item_declaration ]
                /*empty*/                               { $$ = nullptr; }
        |       ఖండం_Decl_List_E ఖండం_అంశం_declaration   { $$ = కూడు_తదుపరి_Null($1, $2); }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

ఖండం_అంశం_declaration<nodep>:  // ==IEEE: block_item_declaration
                దత్తాంశం_declaration                        { $$ = $1; }
        |       parameter_declaration ';'               { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

stmtListE<కణుపు_Stmtp>:
                /*empty*/                               { $$ = nullptr; }
        |       stmtList                                { $$ = $1; }
        ;

stmtList<కణుపు_Stmtp>:
                stmt                               { $$ = $1; }
        |       stmtList stmt                      { $$ = కూడు_తదుపరి_Null($1, $2); }
        //
        |       stmtList error ';'                      { $$ = $1; }  // LCOV_EXCL_LINE
        ;

stmt<కణుపు_Stmtp>:                    // IEEE: statement + statement_or_null + seq_block + par_block
                statement_అంశం                          { $$ = $1; }
        //                      // S05 block creation rule
        |       id/*block_identifier*/ ':' statement_అంశం       { $$ = new AstBegin{$<fl>1, *$1, $3, false}; }
        //                      // from _or_null
        |       ';'                                     { $$ = nullptr; }
        //                      // labeled par_block/seq_block with leading ':'
        |       seq_ఖండం_Pre_Id                          { $$ = $1; }
        |       par_ఖండం_Pre_Id                          { $$ = $1; }
        ;

statement_అంశం<కణుపు_Stmtp>:          // IEEE: statement_item
        //                      // IEEE: operator_assignment
                foperator_assignment ';'                { $$ = $1; }
        //
        //                      // IEEE: blocking_assignment
        //                      // 1800-2009 restricts LHS of assignment to new to not have a range
        //                      // This is ignored to avoid conflicts
        |       fexprLvalue yP_EQ__NEW dynamic_array_new ';'   { $$ = new Ast_కేటాయించు{$2, $1, $3}; }
        |       fexprLvalue yP_EQ__NEW class_new ';'    { $$ = new Ast_కేటాయించు{$2, $1, $3}; }
        //                      // IEEE: inc_or_dec_expression
        |       finc_or_dec_వ్యక్తీకరణ ';'              { $$ = new Ast_Stmt_సమాసం{$<fl>1, $1}; }
        //
        //                      // IEEE: nonblocking_assignment
        |       fexprLvalue yP_LTE delay_or_event_controlE సమాసం ';'
                        { $$ = new Ast_కేటాయించు_Dly{$2, $1, $4, $3}; }
        //                      // IEEE: clocking_drive ';'
        |       fexprLvalue yP_LTE చక్రం_delay సమాసం ';'
                        { $$ = new Ast_కేటాయించు_Dly{$2, $1, $4, $3}; }
        //UNSUP cycle_delay fexprLvalue yP_LTE ';'      { UNSUP }
        |       y_కేటాయించు variable_lvalue '=' delay_or_event_controlE సమాసం ';'
                        { $$ = new Ast_కేటాయించు_Cont{$1, $2, $5, $4};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.సమితి_Has_కేటాయించు_Deassign(); }
        |       yDEASSIGN variable_lvalue ';'
                        { $$ = new AstDeassign{$1, $2};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.సమితి_Has_కేటాయించు_Deassign(); }
        |       yFORCE variable_lvalue '=' సమాసం ';'
                        { $$ = new Ast_కేటాయించు_Force{$1, $2, $4}; v3Global.సమితి_Has_Forceable_సంకేతాలు(); }
        |       yRELEASE variable_lvalue ';'
                        { $$ = new AstRelease{$1, $2}; v3Global.సమితి_Has_Forceable_సంకేతాలు(); }
        //
        //                      // IEEE: case_statement
        |       unique_ప్రాధాన్యత_E case_ప్రారంభం caseAttrE case_అంశం_List yENDCASE
                        { $$ = $2; ఒకవేళ ($4) $2->కూడు_Itemsp($4);
                          ఒకవేళ ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          ఒకవేళ ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          ఒకవేళ ($1 == uniq_ప్రాధాన్యత) $2->ప్రాధాన్యత_Pragma(true); }
        // case matches uses patterns, not expressions
        |       unique_ప్రాధాన్యత_E case_ప్రారంభం caseAttrE yMATCHES case_matches_అంశం_List yENDCASE
                        { $$ = $2; ఒకవేళ ($5) $2->కూడు_Itemsp($5);
                          $2->case_Matches_సమితి();
                          ఒకవేళ ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          ఒకవేళ ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          ఒకవేళ ($1 == uniq_ప్రాధాన్యత) $2->ప్రాధాన్యత_Pragma(true); }
        |       unique_ప్రాధాన్యత_E case_ప్రారంభం caseAttrE yINSIDE case_inside_అంశం_List yENDCASE
                        { $$ = $2; ఒకవేళ ($5) $2->కూడు_Itemsp($5);
                          ఒకవేళ (!$2->caseSimple()) $4->v3error("Illegal to have inside on a casex/casez");
                          $2->case_Inside_సమితి();
                          ఒకవేళ ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          ఒకవేళ ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          ఒకవేళ ($1 == uniq_ప్రాధాన్యత) $2->ప్రాధాన్యత_Pragma(true); }
        //
        //                      // IEEE: conditional_statement
        |       unique_ప్రాధాన్యత_E yIF '(' సమాసం ')' stmt     %prec pr_క్రింది_THAN_ELSE
                        { AstIf* స్థిరరాశి newp = new AstIf{$2, $4,
                              PARSEP->new_ఖండం($2, $6)};
                          $$ = newp;
                          ఒకవేళ ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          ఒకవేళ ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          ఒకవేళ ($1 == uniq_ప్రాధాన్యత) newp->ప్రాధాన్యత_Pragma(true); }
        |       unique_ప్రాధాన్యత_E yIF '(' సమాసం ')' stmt yELSE stmt
                        { AstIf* స్థిరరాశి newp = new AstIf{$2, $4,
                              PARSEP->new_ఖండం($2, $6),
                              PARSEP->new_ఖండం($2, $8)};
                          $$ = newp;
                          ఒకవేళ ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          ఒకవేళ ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          ఒకవేళ ($1 == uniq_ప్రాధాన్యత) newp->ప్రాధాన్యత_Pragma(true); }
        //
        //                      // IEEE: subroutine_call_statement
        //                      // IEEE says we then expect a function call
        //                      // (function_subroutine_callNoMethod), but rest of
        //                      // the code expects an AstTask when used as a statement,
        //                      // so parse as if task
        //                      // Alternative would be shim with new AstVoidStmt.
        |       yVOID yP_TICK '(' task_subroutine_call_No_పద్ధతి ')' ';'
                        { Ast_కణుపు_సమాసం* స్థిరరాశి exprp = $4;
                          Ast_కణుపు* callp = exprp;
                          అయితే (AstDot* స్థిరరాశి dotp = VN_CAST(callp, Dot)) callp = dotp->rhsp();
                          File_పంక్తి* స్థిరరాశి newfl = new File_పంక్తి{callp->fileline()};
                          newfl->warnOff(V3_Error_సంకేతం::IGNOREDRETURN, true);
                          callp->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' సమాసం '.' task_subroutine_call_No_పద్ధతి ')' ';'
                        { Ast_కణుపు_సమాసం* స్థిరరాశి exprp = new AstDot{$5, false, $4, $6};
                          File_పంక్తి* స్థిరరాశి newfl = new File_పంక్తి{$6->fileline()};
                          newfl->warnOff(V3_Error_సంకేతం::IGNOREDRETURN, true);
                          $6->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' system_f_only_సమాసం_call ')' ';'
                        { $$ = new Ast_Stmt_సమాసం{$<fl>4, $4};
                          File_పంక్తి* స్థిరరాశి newfl = new File_పంక్తి{$$->fileline()};
                          newfl->warnOff(V3_Error_సంకేతం::IGNOREDRETURN, true);
                          $$->fileline(newfl); }
        //                      // Any system function as a task
        |       yVOID yP_TICK '(' system_f_or_t_సమాసం_call ')' ';'
                        { $$ = new Ast_Stmt_సమాసం{$<fl>4, $4};
                          File_పంక్తి* స్థిరరాశి newfl = new File_పంక్తి{$$->fileline()};
                          newfl->warnOff(V3_Error_సంకేతం::IGNOREDRETURN, true);
                          $$->fileline(newfl); }
        //
        |       task_subroutine_callNoSemi ';'          { $$ = $1; }
        //
        |       statementVerilatorPragmas               { $$ = new AstStmtPragma{$<fl>1, $1}; }
        //
        //                      // IEEE: disable_statement
        |       yDISABLE yFORK ';'                      { $$ = new AstDisableFork{$1}; }
        |       yDISABLE idDottedSel ';'
                        { $$ = new AstDisable{$1, $2};
                          PARSEP->import_If_లోపల_Std($1, "process", true);
                        }
        //                      // IEEE: event_trigger
        |       yP_MINUSGT సమాసం ';'
                        { $$ = new AstFireEvent{$1, $2, false}; }
        |       yP_MINUSGTGT delay_or_event_controlE సమాసం ';'
                        { $$ = new AstFireEvent{$1, $3, true}; }
        //
        // do/for/forever/while loops all modelled as AstLoop
        |       yDO stmt yWHILE '(' సమాసం ')' ';'
                        { AstLoop* స్థిరరాశి loopp = new AstLoop{$1, $2};
                          loopp->కూడు_Contsp(new AstLoopTest{$<fl>5, loopp, $5});
                          $$ = loopp; }
        |       yFOR  '(' { VARRESET_NONLIST(UNKNOWN); } for_initializationE ';' సమాసం_E ';' for_అడుగు_E ')' stmt
                        { AstBegin* స్థిరరాశి blockp = new AstBegin{$1, "", $4, true};
                          AstLoop* స్థిరరాశి loopp = new AstLoop{$1};
                          ఒకవేళ ($6) loopp->కూడు_Stmtsp(new AstLoopTest{$<fl>6, loopp, $6});
                          loopp->కూడు_Stmtsp($10);
                          loopp->కూడు_Contsp($8);
                          blockp->కూడు_Stmtsp(loopp);
                          $$ = blockp; }
        |       yFOREVER stmt
                        { AstLoop* స్థిరరాశి loopp = new AstLoop{$1, $2};
                          $$ = loopp; }
        |       yWHILE '(' సమాసం ')' stmt
                        { AstLoop* స్థిరరాశి loopp = new AstLoop{$1};
                          loopp->కూడు_Stmtsp(new AstLoopTest{$<fl>3, loopp, $3});
                          loopp->కూడు_Stmtsp($5);
                          $$ = loopp; }
        // Other loop statements
        |       yREPEAT '(' సమాసం ')' stmt          { $$ = new AstRepeat{$1, $3, $5}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM and 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' stmt
                        { $$ = new AstBegin{$1, "", new AstForeach{$1, $3, $5}, true}; }
        //
        //                      // IEEE: jump_statement
        |       yRETURN ';'                             { $$ = new AstReturn{$1}; }
        |       yRETURN సమాసం ';'                        { $$ = new AstReturn{$1, $2}; }
        |       yBREAK ';'                              { $$ = new AstBreak{$1}; }
        |       yCONTINUE ';'                           { $$ = new AstContinue{$1}; }
        //
        |       par_ఖండం                               { $$ = $1; }
        //                      // IEEE: procedural_timing_control_statement + procedural_timing_control
        |       delay_control stmt
                        { Ast_కణుపు_Stmt* nextp = nullptr;
                          ఒకవేళ ($2) {
                              ఒకవేళ ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_వెనుక_With_తదుపరి(), కణుపు_Stmt);
                              $1->కూడు_Stmtsp($2);
                          }
                          $$ = $1;
                          కూడు_తదుపరి_Null($$, nextp); }
        |       event_control stmt
                        { Ast_కణుపు_Stmt* nextp = nullptr;
                          ఒకవేళ ($2 && $2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_వెనుక_With_తదుపరి(), కణుపు_Stmt);
                          $$ = new AstEventControl{FILELINE_OR_CRE($1), $1, $2};
                          కూడు_తదుపరి_Null($$, nextp); }
        |       చక్రం_delay stmt
                        { Ast_కణుపు_Stmt* nextp = nullptr;
                          ఒకవేళ ($2) {
                              ఒకవేళ ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_వెనుక_With_తదుపరి(), కణుపు_Stmt);
                              $1->కూడు_Stmtsp($2);
                          }
                          $$ = $1;
                          కూడు_తదుపరి_Null($$, nextp); }
        |       seq_ఖండం                               { $$ = $1; }
        //
        //                      // IEEE: wait_statement
        |       yWAIT '(' సమాసం ')' stmt            { $$ = new AstWait{$1, $3, $5}; }
        |       yWAIT yFORK ';'                         { $$ = new AstWaitFork{$1}; }
        //                      // action_block expanded here
        |       y_WAIT_క్రమం '(' vrdList ')' stmt %prec pr_క్రింది_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5); }
        |       y_WAIT_క్రమం '(' vrdList ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5, $7);}
        |       y_WAIT_క్రమం '(' vrdList ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $6); }
        //
        //                      // IEEE: procedural_assertion_statement
        |       procedural_assertion_statement          { $$ = $1; }
        //
        |       randsequence_statement                  { $$ = $1; }
        //
        //                      // IEEE: randcase_statement
        |       yRANDCASE rand_case_అంశం_List yENDCASE   { $$ = new AstRandCase{$1, $2}; }
        //
        //                      // IEEE: expect_property_statement
        //                      // action_block expanded here
        |       yEXPECT '(' property_spec ')' stmt %prec pr_క్రింది_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5); }
        |       yEXPECT '(' property_spec ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5, $7); }
        |       yEXPECT '(' property_spec ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $6); }
        ;

statementVerilatorPragmas<pragmap>:
                y_VL_COVERAGE_ఖండం_OFF
                        { $$ = new AstPragma{$1, VPragmaType::COVERAGE_ఖండం_OFF}; }
        |       yVL_UNROLL_DISABLE
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_DISABLE}; }
        |       y_VL_UNROLL_నిండిన
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_నిండిన}; }
        ;

foperator_assignment<కణుపు_Stmtp>:    // IEEE: operator_assignment (for first part of expression)
                fexprLvalue '=' delay_or_event_controlE సమాసం    { $$ = new Ast_కేటాయించు{$2, $1, $4, $3}; }
        //
        |       fexprLvalue yP_PLUSEQ    సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::కూడు_2, $2, $1, $3}; }
        |       fexprLvalue yP_MINUSEQ   సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::Sub, $2, $1, $3}; }
        |       fexprLvalue yP_TIMESEQ   సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::Mul, $2, $1, $3}; }
        |       fexprLvalue yP_DIVEQ     సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::Div, $2, $1, $3}; }
        |       fexprLvalue yP_MODEQ     సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::ModDiv, $2, $1, $3}; }
        |       fexprLvalue yP_ANDEQ     సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::And, $2, $1, $3}; }
        |       fexprLvalue yP_OREQ      సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::Or, $2, $1, $3}; }
        |       fexprLvalue yP_XOREQ     సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::Xor, $2, $1, $3}; }
        |       fexprLvalue yP_SLEFTEQ   సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::జరుపు_L, $2, $1, $3}; }
        |       fexprLvalue yP_SRIGHTEQ  సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::జరుపు_R, $2, $1, $3}; }
        |       fexprLvalue yP_SSRIGHTEQ సమాసం
                        { $$ = new Ast_కేటాయించు_Compound{Ast_కేటాయించు_Compound::operation::జరుపు_RS, $2, $1, $3}; }
        ;

inc_or_dec_వ్యక్తీకరణ<కణుపు_Exprp>:   // ==IEEE: inc_or_dec_expression
        //                      // Need fexprScope instead of variable_lvalue to prevent conflict
                ~l~సమాసం_Scope yP_PLUSPLUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostInc{$2, $1}; }
        |       ~l~సమాసం_Scope yP_MINUSMINUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostDec{$2, $1}; }
        //                      // Need expr instead of variable_lvalue to prevent conflict
        |       yP_PLUSPLUS     సమాసం
                        { $<fl>$ = $<fl>1; $$ = new AstPreInc{$1, $2}; }
        |       yP_MINUSMINUS   సమాసం
                        { $<fl>$ = $<fl>1; $$ = new AstPreDec{$1, $2}; }
        ;

finc_or_dec_వ్యక్తీకరణ<కణుపు_Exprp>:  // ==IEEE: inc_or_dec_expression
                BISONPRE_COPY(inc_or_dec_వ్యక్తీకరణ,{s/~l~/f/g})        // {copied}
        ;

sinc_or_dec_వ్యక్తీకరణ<కణుపు_Exprp>:  // IEEE: inc_or_dec_expression (for sequence_expression)
                BISONPRE_COPY(inc_or_dec_వ్యక్తీకరణ,{s/~l~/s/g})        // {copied}
        ;

pinc_or_dec_వ్యక్తీకరణ<కణుపు_Exprp>:  // IEEE: inc_or_dec_expression (for property_expression)
                BISONPRE_COPY(inc_or_dec_వ్యక్తీకరణ,{s/~l~/p/g})        // {copied}
        ;

//UNSUPev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for ev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/ev_/g})      // {copied}
//UNSUP ;

//UNSUPpev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for pev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/pev_/g})     // {copied}
//UNSUP ;

class_new<కణుపు_Exprp>:    // IEEE: class_new
        //                      // See V3ParseImp::tokenPipeScanEqNew that searches for '=' ... yNEW__LEX
                class_newNoScope
                        { $$ = $1; }
        //                      // Special precedence so (...) doesn't match expr
        //                      // A scope is not legal in front of a AstNewCopy
        |       packageClassScopeNoId yNEW__ETC
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2,  nullptr, true}); }
        |       packageClassScopeNoId yNEW__PAREN '(' list_యొక్క_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2, $4, true}); }
        ;

class_newNoScope<కణుపు_Exprp>:    // IEEE: class_new but no packageClassScope
        //                      // Special precedence so (...) doesn't match expr
                yNEW__ETC                               { $$ = new AstNew{$1}; }
        |       yNEW__ETC సమాసం                          { $$ = new AstNewCopy{$1, $2}; }
        |       yNEW__PAREN '(' list_యొక్క_arguments_E ')'  { $$ = new AstNew{$1, $3}; }
        ;

dynamic_array_new<కణుపు_Exprp>:   // ==IEEE: dynamic_array_new
                yNEW__ETC '[' సమాసం ']'                  { $$ = new AstNewDynamic{$1, $3, nullptr}; }
        |       yNEW__ETC '[' సమాసం ']' '(' సమాసం ')'     { $$ = new AstNewDynamic{$1, $3, $6}; }
        ;

//************************************************
// Case/If

unique_ప్రాధాన్యత_E<uniqstate>:    // IEEE: unique_priority + empty
                /*empty*/                               { $$ = uniq_NONE; }
        |       y_ప్రాధాన్యత                               { $$ = uniq_ప్రాధాన్యత; }
        |       yUNIQUE                                 { $$ = uniq_UNIQUE; }
        |       yUNIQUE0                                { $$ = uniq_UNIQUE0; }
        ;

case_ప్రారంభం<casep>:               // IEEE: part of case_statement
                yCASE  '(' సమాసం_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASE, $3, nullptr}; }
        |       yCASEX '(' సమాసం_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEX, $3, nullptr}; }
        |       yCASEZ '(' సమాసం_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEZ, $3, nullptr}; }
        ;

caseAttrE:
                /*empty*/                               { }
        |       caseAttrE y_VL_నిండిన_CASE                 { GRAMMARP->m_caseAttrp->నిండిన_Pragma(true); }
        |       caseAttrE yVL_PARALLEL_CASE             { GRAMMARP->m_caseAttrp->parallelPragma(true); }
        ;

case_అంశం_List<caseItemp>:       // IEEE: { case_item + ... }
                caseCondList colon stmt                    { $$ = new Ast_Case_అంశం{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_అంశం{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_అంశం{$1, nullptr, $2}; }
        |       case_అంశం_List caseCondList colon stmt      { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$3, $2, $4}); }
        |       case_అంశం_List yDEFAULT stmt                { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$2, nullptr, $3}); }
        |       case_అంశం_List yDEFAULT colon stmt          { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$2, nullptr, $4}); }
        ;

case_inside_అంశం_List<caseItemp>:        // IEEE: { case_inside_item + range_list ... }
                range_list colon stmt                      { $$ = new Ast_Case_అంశం{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_అంశం{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_అంశం{$1, nullptr, $2}; }
        |       case_inside_అంశం_List range_list colon stmt { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$3, $2, $4}); }
        |       case_inside_అంశం_List yDEFAULT stmt         { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$2, nullptr, $3}); }
        |       case_inside_అంశం_List yDEFAULT colon stmt   { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$2, nullptr, $4}); }
        ;

case_matches_అంశం_List<caseItemp>:    // IEEE: { case_pattern_item + ... }
        //                      // IEEE: case_pattern_item ::= pattern [&&& expr] : stmt
        //                      // pattern includes expr for tagged void members (tagged id)
                నమూనా_No_సమాసం colon stmt                   { $$ = new Ast_Case_అంశం{$2, $1, $3}; }
        |       సమాసం colon stmt                            { $$ = new Ast_Case_అంశం{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_అంశం{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_అంశం{$1, nullptr, $2}; }
        |       case_matches_అంశం_List నమూనా_No_సమాసం colon stmt
                        { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$3, $2, $4}); }
        |       case_matches_అంశం_List సమాసం colon stmt
                        { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$3, $2, $4}); }
        |       case_matches_అంశం_List yDEFAULT stmt        { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$2, nullptr, $3}); }
        |       case_matches_అంశం_List yDEFAULT colon stmt  { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$2, nullptr, $4}); }
        ;

rand_case_అంశం_List<caseItemp>:       // IEEE: { rand_case_item + ... }
        //                      // Randcase syntax doesn't have default, or expression lists
                సమాసం colon stmt                            { $$ = new Ast_Case_అంశం{$2, $1, $3}; }
        |       rand_case_అంశం_List సమాసం colon stmt         { $$ = $1->కూడు_తదుపరి(new Ast_Case_అంశం{$3, $2, $4}); }
        ;

range_list<కణుపు_Exprp>:     // ==IEEE: range_list/open_range_list + value_range/open_value_range
                విలువ_range                             { $$ = $1; }
        |       range_list ',' విలువ_range              { $$ = $1->కూడు_తదుపరి($3); }
        ;

విలువ_range<కణుపు_Exprp>:         // ==IEEE: value_range/open_value_range
                సమాసం                                    { $$ = $1; }
        |       '[' సమాసం ':' సమాసం ']'                   { $$ = new AstInsideRange{$1, $2, $4}; }
        //                      // IEEE-2023: added all four:
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' '$' ':' expr ']'
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' expr ':' '$' ']'
        |       '[' సమాసం yP_PLUSSLASHMINUS సమాసం ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +/- range"); DEL($2, $4); }
        |       '[' సమాసం yP_PLUSPCTMINUS సమాసం ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +%- range"); DEL($2, $4); }
        ;

covergroup_విలువ_range<కణుపు_Exprp>:  // ==IEEE-2012: covergroup_value_range
                cgexpr                                  { $$ = $1; }
        |       '[' cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: covergroup value range '[...]'"); DEL($2, $4); }
        //                      // IEEE-2023: added all four:
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' '$' ':' cgexpr ']'
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' cgexpr ':' '$' ']'
        |       '[' cgexpr yP_PLUSSLASHMINUS cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: covergroup value range '[...]'"); DEL($2, $4); }
        |       '[' cgexpr yP_PLUSPCTMINUS cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: covergroup value range '[...]'"); DEL($2, $4); }
        ;

caseCondList<కణుపు_Exprp>:        // IEEE: part of case_item
                సమాసం_Type_Compare                         { $$ = $1; }
        |       caseCondList ',' సమాసం_Type_Compare        { $$ = $1->కూడు_తదుపరి($3); }
        ;

నమూనా_No_సమాసం<కణుపు_Exprp>:       // IEEE: pattern **Excluding Expr*
                '.' idAny/*variable*/
                        { $$ = new Ast_నమూనా_Var{$1, *$2}; }
        |       yP_DOTSTAR
                        { $$ = new Ast_నమూనా_నక్షత్రం{$1}; }
        //                      // IEEE: "expr" excluded; expand in callers
        //                      // IEEE: tagged member_identifier [ pattern ]
        //                      // Standalone "yTAGGED__NONPRIMARY idAny" is handled via expr in patternOne
        //                      // Here, we need to treat yTAGGED and yTAGGED__NONPRIMARY identically.
        |       yTAGGED idAny/*member_identifier*/ నమూనా_No_సమాసం
                        { $$ = new Ast_Tagged_నమూనా{$1, *$2, $3}; }
        |       yTAGGED__NONPRIMARY idAny/*member_identifier*/ నమూనా_No_సమాసం
                        { $$ = new Ast_Tagged_నమూనా{$1, *$2, $3}; }
        //                      // "yP_TICKBRA patternList '}'" part of expr under assignment_pattern
        ;

నమూనా_List<nodep>:             // IEEE: part of pattern
                నమూనా_One                              { $$ = $1; }
        |       నమూనా_List ',' నమూనా_One              { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

నమూనా_One<nodep>:              // IEEE: part of pattern
                సమాసం
                        { ఒకవేళ ($1) $$ = new AstPatMember{$1->fileline(), $1, nullptr, nullptr}; లేకపోతే $$ = nullptr; }
        |       సమాసం '{' వాదనలు_సమాసం_List '}'               { $$ = new AstPatMember{$2, $3, nullptr, $1}; }
        |       నమూనా_No_సమాసం                           { $$ = $1; }
        ;

నమూనా_Member_List<nodep>:       // IEEE: part of pattern and assignment_pattern
                నమూనా_Member_One                        { $$ = $1; }
        |       నమూనా_Member_List ',' నమూనా_Member_One  { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

నమూనా_Member_One<patMemberp>:   // IEEE: part of pattern and assignment_pattern
                నమూనా_కీలకం ':' సమాసం                     { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        |       నమూనా_కీలకం ':' నమూనా_No_సమాసం            { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        //                      // From assignment_pattern_key
        |       yDEFAULT ':' సమాసం                       { $$ = new AstPatMember{$1, $3, nullptr, nullptr}; $$->isDefault(true); }
        |       yDEFAULT ':' నమూనా_No_సమాసం              { AstPatMember* స్థిరరాశి patp = new AstPatMember{$1, $3, nullptr, nullptr}; patp->isDefault(true); $$ = patp; }
        ;

నమూనా_కీలకం<nodep>:              // IEEE: merge structure_pattern_key, array_pattern_key, assignment_pattern_key
        //                      // IEEE: structure_pattern_key: member_identifier | assignment_pattern_key
        //                      // IEEE: array_pattern_key: constant_expression | assignment_pattern_key
        //                      // Verilator:
        //                      //   A bare "foo" is ambiguous here, as it may be a constant
        //                      //   identifier (if array) or a reference to the "foo" member
        //                      //   (if structure).  Both spell the same in expr, so the
        //                      //   bare-identifier case becomes a Text node and V3LinkDot
        //                      //   resolves which one it is.
                సమాసం
                        { $$ = GRAMMARP->create_నమూనా_కీలకం($1); }
        //                      // IEEE: assignment_pattern_key
        |       simple_typeNoRef
                        { $$ = $1; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // (simple_type ps_parameter_identifier is part of expr above)
        |       packageClassScopeE idType
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = refp; }
        ;

assignment_నమూనా<patternp>:   // ==IEEE: assignment_pattern
        // This doesn't match the text of the spec.  I think a : is missing, or example code needed
        // yP_TICKBRA constExpr exprList '}'    { $$="'{"+$2+" "+$3"}"; }
        //                      // "'{ const_expression }" is same as patternList with one entry
        //                      // From patternNoExpr
        //                      // also IEEE: "''{' expression { ',' expression } '}'"
        //                      //      matches since patternList includes expr
                yP_TICKBRA నమూనా_List '}'              { $$ = new Ast_నమూనా{$1, $2}; }
        //                      // From patternNoExpr
        //                      // also IEEE "''{' structure_pattern_key ':' ...
        //                      // also IEEE "''{' array_pattern_key ':' ...
        |       yP_TICKBRA నమూనా_Member_List '}'        { $$ = new Ast_నమూనా{$1, $2}; }
        //                      // IEEE: Not in grammar, but in VMM
        |       yP_TICKBRA '}'                          { $$ = new Ast_నమూనా{$1, nullptr}; }
        ;

// "datatype id = x {, id = x }"  |  "yaId = x {, id=x}" is legal
for_initializationE<nodep>:      // ==IEEE: for_initialization + for_variable_declaration
                /*ఖాళీలు*/                     { $$ = nullptr; }
        |       for_initialization_అంశం_List      { $$ = $1; }
        ;

for_initialization_అంశం_List<nodep>:      // IEEE: [for_variable_declaration...]
                for_initialization_అంశం                  { $$ = $1; }
        |       for_initialization_అంశం_List ',' for_initialization_అంశం   { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

for_initialization_అంశం<nodep>:          // IEEE: variable_assignment + for_variable_declaration
        //                      // IEEE: for_variable_declaration
                దత్తాంశం_type idAny/*new*/ '=' సమాసం
                        { VARRESET_NONLIST(VAR); VARDTYPE($1);
                          AstVar* స్థిరరాశి varp = VARDONEA($<fl>2, *$2, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->కూడు_తదుపరి(new Ast_కేటాయించు{$3, new AstParseRef{$<fl>2, *$2}, $4}); }
        //                      // IEEE-2012:
        |       yVAR దత్తాంశం_type idAny/*new*/ '=' సమాసం
                        { VARRESET_NONLIST(VAR); VARDTYPE($2);
                          AstVar* స్థిరరాశి varp = VARDONEA($<fl>3, *$3, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->కూడు_తదుపరి(new Ast_కేటాయించు{$4, new AstParseRef{$<fl>3, *$3}, $5}); }
        //                      // IEEE: variable_assignment
        //                      // UNSUP variable_lvalue below
        |       id/*newOrExisting*/ '=' సమాసం
                        { ఒకవేళ (GRAMMARP->m_varDecl) {
                              AstVar* స్థిరరాశి varp = VARDONEA($<fl>1, *$1, nullptr, nullptr);
                              varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                              $$ = varp;
                              $$->కూడు_తదుపరి(new Ast_కేటాయించు{$2, new AstParseRef{$<fl>1, *$1}, $3});
                          } లేకపోతే {
                              $$ = new Ast_కేటాయించు{$2, new AstParseRef{$<fl>1, *$1}, $3};
                          }
                        }
        ;

for_అడుగు_E<nodep>:               // IEEE: for_step + empty
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       for_అడుగు                                { $$ = $1; }
        ;

for_అడుగు<nodep>:                // IEEE: for_step
                for_అడుగు_assignment                     { $$ = $1; }
        |       for_అడుగు ',' for_అడుగు_assignment        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

for_అడుగు_assignment<nodep>:     // ==IEEE: for_step_assignment
                foperator_assignment                    { $$ = $1; }
        |       finc_or_dec_వ్యక్తీకరణ                  { $$ = new Ast_Stmt_సమాసం{$<fl>1, $1}; }
        //                      // IEEE: function_subroutine_call
        |       task_subroutine_callNoSemi              { $$ = $1; }
        ;

loop_variables<nodep>:          // IEEE: loop_variables
                loop_variableE                          { $$ = $1; }
        |       loop_variables ',' loop_variableE       { $$ = $1->కూడు_తదుపరి($3); }
        ;

loop_variableE<nodep>:          // IEEE: part of loop_variables
                /*ఖాళీలు*/                             { $$ = new Ast_ఖాళీలు{CRELINE()}; }
        |       parse_Ref_పునాది                            { $$ = $1; }
        ;

//************************************************
// Functions/tasks

taskRef<కణుపు_Exprp>:            // IEEE: part of tf_call
                id                                      { $$ = new AstTaskRef{$<fl>1, *$1}; }
        |       id '(' list_యొక్క_arguments_E ')'           { $$ = new AstTaskRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_యొక్క_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstTaskRef{$<fl>2, *$2, $4}); }
        ;

funcRef<కణుపు_Exprp>:             // IEEE: part of tf_call
        //                      // package_scope/hierarchical_... is part of expr, so just need ID
        //                      //      making-a                id-is-a
        //                      //      -----------------       ------------------
        //                      //      tf_call                 tf_identifier           expr (list_of_arguments)
        //                      //      method_call(post .)     function_identifier     expr (list_of_arguments)
        //                      //      property_instance       property_identifier     property_actual_arg
        //                      //      sequence_instance       sequence_identifier     sequence_actual_arg
        //                      //      let_expression          let_identifier          let_actual_arg
        //
                id '(' list_యొక్క_arguments_E ')'
                        { $$ = new AstFuncRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_యొక్క_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstFuncRef{$<fl>2, *$2, $4}); }
        //UNSUP list_of_argumentE should be pev_list_of_argumentE
        //UNSUP: idDottedSel is really just id to allow dotted method calls
        ;

task_subroutine_callNoSemi<కణుపు_Stmtp>:  // similar to IEEE task_subroutine_call but without ';'
        //                      // Expr included here to resolve our not knowing what is a method call
        //                      // Expr here must result in a subroutine_call
                task_subroutine_call_No_పద్ధతి            { $$ = $1->makeStmt(); }
        |       fexpr '.' task_subroutine_call_No_పద్ధతి  { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        |       system_t_stmt_call                      { $$ = $1; }
        //                      // Any system function as a task
        |       system_f_or_t_సమాసం_call                 { $$ = new Ast_Stmt_సమాసం{$<fl>1, $1}; }
        //                      // Not here in IEEE; from class_constructor_declaration
        //                      // Because we've joined class_constructor_declaration into generic functions
        //                      // Way over-permissive;
        //                      // IEEE: [ ySUPER '.' yNEW [ '(' list_of_arguments ')' ] ';' ]
        |       fexpr '.' class_newNoScope              { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        ;

task_subroutine_call_No_పద్ధతి<కణుపు_Exprp>:    // function_subroutine_callNoMethod (as task)
        //                      // IEEE: tf_call
                taskRef                                 { $$ = $1; }
        //                      // funcref below not task ref to avoid conflict, must later handle either
        |       funcRef yWITH__PAREN '(' సమాసం ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' సమాసం ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_ఖండం     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_ఖండం
                                                        { AstWithParse* స్థిరరాశి withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

function_subroutine_call_No_పద్ధతి<కణుపు_Exprp>:        // IEEE: function_subroutine_call (as function)
        //                      // IEEE: tf_call
                funcRef                                 { $$ = $1; }
        |       funcRef yWITH__PAREN '(' సమాసం ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' సమాసం ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        |       system_f_only_సమాసం_call                 { $$ = $1; }
        |       system_f_or_t_సమాసం_call                 { $$ = $1; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_ఖండం     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_ఖండం
                                                        { AstWithParse* స్థిరరాశి withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

system_t_stmt_call<కణుపు_Stmtp>:  // IEEE: part of system_tf_call (as task returning statement)
        //
                yaD_PLI system_Dpi_వాదనలు_E                  { AstTaskRef* స్థిరరాశి refp = new AstTaskRef{$<fl>1, *$1, $2};
                                                          refp->pli(true);
                                                          $$ = refp->makeStmt(); }
        //
        |       yD_DUMPPORTS '(' idDottedSel ',' సమాసం ')'  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $5}; DEL($3);
                                                          $$->కూడు_తదుపరి(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPPORTS '(' ',' సమాసం ')'           { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $4};
                                                          $$->కూడు_తదుపరి(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPFILE '(' సమాసం ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $3}; }
        |       yD_DUMPVARS parenE                      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                              new AstConst{$<fl>1, 0}}; }
        |       yD_DUMPVARS '(' సమాసం ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; }
        |       yD_DUMPVARS '(' సమాసం ',' సమాసం_List ')'   { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; DEL($5); }
        |       yD_DUMPALL parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; }
        |       yD_DUMPALL '(' సమాసం ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; DEL($3); }
        |       yD_DUMPFLUSH parenE                     { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; }
        |       yD_DUMPFLUSH '(' సమాసం ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; DEL($3); }
        |       yD_DUMPLIMIT '(' సమాసం ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::హద్దు_2, $3}; }
        |       yD_DUMPLIMIT '(' సమాసం ',' సమాసం ')'      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::హద్దు_2, $3}; DEL($5); }
        |       yD_DUMPOFF parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; }
        |       yD_DUMPOFF '(' సమాసం ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; DEL($3); }
        |       yD_DUMPON parenE                        { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; }
        |       yD_DUMPON '(' సమాసం ')'                  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; DEL($3); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCStmtUser* cstmtp = nullptr;
                    ఒకవేళ (!v3Global.opt.ignc()) {
                        cstmtp = new AstCStmtUser{$1, true};
                        cstmtp->కూడు($3);
                    }
                    $$ = cstmtp;
                }
        |       yD_SDF_ANNOTATE '(' సమాసం_EList_E ')'      { $$ = nullptr; $1->v3warn(SPECIFYIGN, "Ignoring unsupported: $sdf_annotate"); DEL($3); }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_దొంతర_Trace_T{$1}; }
        |       yD_SYSTEM '(' సమాసం ')'                  { $$ = new AstSystemT{$1, $3}; }
        //
        |       yD_EXIT parenE                          { $$ = new Ast_ముగింపు{$1}; }
        //
        |       yD_FCLOSE '(' సమాసం ')'                  { $$ = new AstFClose{$1, $3}; }
        |       yD_FFLUSH parenE                        { $$ = new AstFFlush{$1, nullptr}; }
        |       yD_FFLUSH '(' సమాసం ')'                  { $$ = new AstFFlush{$1, $3}; }
        |       y_D_ముగింపు parenE                        { $$ = new Ast_ముగింపు{$1}; }
        |       y_D_ముగింపు '(' సమాసం ')'                  { $$ = new Ast_ముగింపు{$1}; DEL($3); }
        |       yD_STOP parenE                          { $$ = new AstStop{$1, false}; }
        |       yD_STOP '(' సమాసం ')'                    { $$ = new AstStop{$1, false}; DEL($3); }
        //
        |       yD_SFORMAT '(' సమాసం ',' సమాసం_Disp_List ')'        { $$ = new AstSFormat{$1, false, $3, $5}; }
        |       yD_SWRITE  '(' సమాసం ',' సమాసం_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'd'}; }
        |       yD_SWRITEB '(' సమాసం ',' సమాసం_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'b'}; }
        |       yD_SWRITEH '(' సమాసం ',' సమాసం_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'h'}; }
        |       yD_SWRITEO '(' సమాసం ',' సమాసం_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'o'}; }
        //
        |       yD_DISPLAY parenE                               { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr}; }
        |       yD_DISPLAY '(' commasE సమాసం_Disp_List ')'         { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4}; }
        |       yD_DISPLAYB parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'b'}; }
        |       yD_DISPLAYB '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'b'}; }
        |       yD_DISPLAYH parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'h'}; }
        |       yD_DISPLAYH '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'h'}; }
        |       yD_DISPLAYO parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'o'}; }
        |       yD_DISPLAYO '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'o'}; }
        |       yD_MONITOR  '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4}; }
        |       yD_MONITORB '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'b'}; }
        |       yD_MONITORH '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'h'}; }
        |       yD_MONITORO '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'o'}; }
        |       yD_STROBE   '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4}; }
        |       yD_STROBEB  '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'b'}; }
        |       yD_STROBEH  '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'h'}; }
        |       yD_STROBEO  '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'o'}; }
        |       yD_WRITE    parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITE    '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4}; }
        |       yD_WRITEB   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEB   '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'b'}; }
        |       yD_WRITEH   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEH   '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'h'}; }
        |       yD_WRITEO   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEO   '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'o'}; }
        |       yD_FDISPLAY '(' సమాసం ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr}; }
        |       yD_FDISPLAY '(' సమాసం ',' సమాసం_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5}; }
        |       yD_FDISPLAYB '(' సమాసం ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'b'}; }
        |       yD_FDISPLAYB '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'b'}; }
        |       yD_FDISPLAYH '(' సమాసం ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'h'}; }
        |       yD_FDISPLAYH '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'h'}; }
        |       yD_FDISPLAYO '(' సమాసం ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'o'}; }
        |       yD_FDISPLAYO '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'o'}; }
        |       yD_FMONITOR  '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5}; }
        |       yD_FMONITORB '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'b'}; }
        |       yD_FMONITORH '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'h'}; }
        |       yD_FMONITORO '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'o'}; }
        |       yD_FSTROBE   '(' సమాసం ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, nullptr}; }
        |       yD_FSTROBE   '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5}; }
        |       yD_FSTROBEB  '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'b'}; }
        |       yD_FSTROBEH  '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'h'}; }
        |       yD_FSTROBEO  '(' సమాసం ',' సమాసం_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'o'}; }
        |       yD_FWRITE   '(' సమాసం ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, nullptr}; }
        |       yD_FWRITE   '(' సమాసం ',' సమాసం_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5}; }
        |       yD_FWRITEB  '(' సమాసం ',' సమాసం_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'b'}; }
        |       yD_FWRITEH  '(' సమాసం ',' సమాసం_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'h'}; }
        |       yD_FWRITEO  '(' సమాసం ',' సమాసం_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'o'}; }
        |       yD_INFO     parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, nullptr}; }
        |       yD_INFO     '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, $4}; }
        |       yD_WARNING  parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, nullptr}; }
        |       yD_WARNING  '(' commasE సమాసం_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, $4}; }
        |       yD_ERROR    parenE                              { $$ = GRAMMARP->createDisplayError($1); }
        |       yD_ERROR    '(' commasE సమాసం_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_ERROR, nullptr, $4};
                          $$->కూడు_తదుపరి(new AstStop{$1, false}); }
        |       yD_FATAL    parenE
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->కూడు_తదుపరి(new AstStop{$1, true}); }
        |       yD_FATAL    '(' సమాసం ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->కూడు_తదుపరి(new AstStop{$1, true}); DEL($3); }
        |       yD_FATAL    '(' సమాసం ',' సమాసం_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, $5};
                          $$->కూడు_తదుపరి(new AstStop{$1, true}); DEL($3); }
        //
        |       yD_ASSERTCTL '(' సమాసం ')'                                            { $$ = new AstAssertCtl{$1, $3}; }
        |       yD_ASSERTCTL '(' సమాసం ',' సమాసం_E ')'                                  { $$ = new AstAssertCtl{$1, $3, $5}; }
        |       yD_ASSERTCTL '(' సమాసం ',' సమాసం_E ',' సమాసం_E ')'                        { $$ = new AstAssertCtl{$1, $3, $5, $7}; }
        |       yD_ASSERTCTL '(' సమాసం ',' సమాసం_E ',' సమాసం_E ',' సమాసం_E ')'              { $$ = new AstAssertCtl{$1, $3, $5, $7, $9}; }
        |       yD_ASSERTCTL '(' సమాసం ',' సమాసం_E ',' సమాసం_E ',' సమాసం_E ',' సమాసం_List ')' { $$ = new AstAssertCtl{$1, $3, $5, $7, $9, $11}; }
        |       yD_ASSERTFAILOFF '(' సమాసం ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3}; }
        |       yD_ASSERTFAILOFF '(' సమాసం_E ',' సమాసం_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7}; }
        |       yD_ASSERTFAILON '(' సమాసం ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3}; }
        |       yD_ASSERTFAILON '(' సమాసం_E ',' సమాసం_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7}; }
        |       yD_ASSERTKILL '(' సమాసం ')'                      { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3}; }
        |       yD_ASSERTKILL '(' సమాసం_E ',' సమాసం_List ')'        { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3, $5}; }
        |       yD_ASSERTKILL parenE                            { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7}; }
        |       yD_ASSERTNONVACUOUSON '(' సమాసం ')'                 { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3}; }
        |       yD_ASSERTNONVACUOUSON '(' సమాసం_E ',' సమాసం_List ')'   { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTNONVACUOUSON parenE                       { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7}; }
        |       yD_ASSERTOFF '(' సమాసం ')'                       { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3}; }
        |       yD_ASSERTOFF '(' సమాసం_E ',' సమాసం_List ')'         { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3, $5}; }
        |       yD_ASSERTOFF parenE                             { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7}; }
        |       yD_ASSERTON '(' సమాసం ')'                        { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3}; }
        |       yD_ASSERTON '(' సమాసం_E ',' సమాసం_List ')'          { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3, $5}; }
        |       yD_ASSERTON parenE                              { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7}; }
        |       yD_ASSERTPASSOFF '(' సమాసం ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3}; }
        |       yD_ASSERTPASSOFF '(' సమాసం_E ',' సమాసం_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7}; }
        |       yD_ASSERTPASSON '(' సమాసం ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3}; }
        |       yD_ASSERTPASSON '(' సమాసం_E ',' సమాసం_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7}; }
        |       yD_ASSERTVACUOUSOFF '(' సమాసం ')'                { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3}; }
        |       yD_ASSERTVACUOUSOFF '(' సమాసం_E ',' సమాసం_List ')'  { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTVACUOUSOFF parenE                      { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7}; }
        //
        |       yD_MONITOROFF parenE                    { $$ = new AstMonitorOff{$1, true}; }
        |       yD_MONITORON parenE                     { $$ = new AstMonitorOff{$1, false}; }
        //
        |       yD_PRINTTIMESCALE                       { $$ = new Ast_Print_సమయం_Scale{$1}; }
        |       yD_PRINTTIMESCALE '(' ')'               { $$ = new Ast_Print_సమయం_Scale{$1}; }
        |       yD_PRINTTIMESCALE '(' idClassSel ')'    { $$ = new Ast_Print_సమయం_Scale{$1}; DEL($3); }
        |       yD_TIMEFORMAT '(' సమాసం_E ',' సమాసం_E ',' సమాసం_E ',' సమాసం_E ')'
                        { $$ = new Ast_సమయం_Format{$1, $3, $5, $7, $9}; }
        |       yD_TIMEFORMAT '(' సమాసం_E ',' సమాసం_E ',' సమాసం_E ')'
                        { $$ = new Ast_సమయం_Format{$1, $3, $5, $7, nullptr}; }
        |       yD_TIMEFORMAT '(' సమాసం_E ',' సమాసం_E ')'
                        { $$ = new Ast_సమయం_Format{$1, $3, $5, nullptr, nullptr}; }
        |       yD_TIMEFORMAT '(' సమాసం_E ')'
                        { $$ = new Ast_సమయం_Format{$1, $3, nullptr, nullptr, nullptr}; }
        //
        |       yD_READMEMB '(' సమాసం ',' idClassSel ')'                         { $$ = new AstReadMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_READMEMB '(' సమాసం ',' idClassSel ',' సమాసం ')'                { $$ = new AstReadMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_READMEMB '(' సమాసం ',' idClassSel ',' సమాసం ',' సమాసం ')'       { $$ = new AstReadMem{$1, false, $3, $5, $7, $9}; }
        |       yD_READMEMH '(' సమాసం ',' idClassSel ')'                         { $$ = new AstReadMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_READMEMH '(' సమాసం ',' idClassSel ',' సమాసం ')'                { $$ = new AstReadMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_READMEMH '(' సమాసం ',' idClassSel ',' సమాసం ',' సమాసం ')'       { $$ = new AstReadMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_WRITEMEMB '(' సమాసం ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMB '(' సమాసం ',' idClassSel ',' సమాసం ')'               { $$ = new AstWriteMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMB '(' సమాసం ',' idClassSel ',' సమాసం ',' సమాసం ')'      { $$ = new AstWriteMem{$1, false, $3, $5, $7, $9}; }
        |       yD_WRITEMEMH '(' సమాసం ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMH '(' సమాసం ',' idClassSel ',' సమాసం ')'               { $$ = new AstWriteMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMH '(' సమాసం ',' idClassSel ',' సమాసం ',' సమాసం ')'      { $$ = new AstWriteMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_CAST '(' సమాసం ',' సమాసం ')'
                        { File_పంక్తి* స్థిరరాశి fl_nowarn = new File_పంక్తి{$1};
                          fl_nowarn->warnOff(V3_Error_సంకేతం::వెడల్పు_2, true);
                          $$ = new AstAssertIntrinsic{fl_nowarn, new AstCastDynamic{fl_nowarn, $5, $3},
                                                      nullptr, nullptr}; }
        ;

system_f_only_సమాసం_call<కణుపు_Exprp>:  // IEEE: part of system_tf_call (for functions returning expressions)
                yaD_PLI system_Dpi_వాదనలు_E                  { $$ = new AstFuncRef{$<fl>1, *$1, $2}; VN_CAST($$, FuncRef)->pli(true); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    ఒకవేళ (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1};
                        cexprp->కూడు($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CPURE '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    ఒకవేళ (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1, AstCExprUser::శుద్ధ{}};
                        cexprp->కూడు($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CAST '(' సమాసం ',' సమాసం ')'           { $$ = new AstCastDynamic{$1, $5, $3}; }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_దొంతర_Trace_F{$1}; }
        |       yD_SYSTEM  '(' సమాసం ')'                 { $$ = new AstSystemF{$1, $3}; }
        ;

system_f_or_t_సమాసం_call<కణుపు_Exprp>:  // IEEE: part of system_tf_call (can be task or func)
                yD_ACOS '(' సమాసం ')'                    { $$ = new AstAcosD{$1, $3}; }
        |       yD_ACOSH '(' సమాసం ')'                   { $$ = new AstAcoshD{$1, $3}; }
        |       yD_ASIN '(' సమాసం ')'                    { $$ = new AstAsinD{$1, $3}; }
        |       yD_ASINH '(' సమాసం ')'                   { $$ = new AstAsinhD{$1, $3}; }
        |       yD_ATAN '(' సమాసం ')'                    { $$ = new AstAtanD{$1, $3}; }
        |       yD_ATAN2 '(' సమాసం ',' సమాసం ')'          { $$ = new AstAtan2D{$1, $3, $5}; }
        |       yD_ATANH '(' సమాసం ')'                   { $$ = new AstAtanhD{$1, $3}; }
        |       y_D_బిట్లు '(' సమాసం_Or_దత్తాంశం_Type ')'          { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_బిట్లు, $3}; }
        |       y_D_బిట్లు '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')' { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_బిట్లు, $3, $5}; }
        |       yD_BITSTOREAL '(' సమాసం ')'              { $$ = new Ast_బిట్లు_గమ్యం_Real_D{$1, $3}; }
        |       yD_BITSTOSHORTREAL '(' సమాసం ')'         { $$ = new Ast_బిట్లు_గమ్యం_Real_D{$1, $3}; UNSUPREAL($1); }
        |       yD_CEIL '(' సమాసం ')'                    { $$ = new AstCeilD{$1, $3}; }
        |       y_D_మారింది '(' సమాసం ')'                 { $$ = new AstLogNot{$1, new AstStable{$1, $3, nullptr}}; }
        |       y_D_మారింది '(' సమాసం ',' సమాసం ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Sen_వృక్షం_మారింది($1, $5)}}; }
        |       y_D_మారింది_GCLK '(' సమాసం ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_వృక్షం($1)}}; }
        |       yD_CHANGING_GCLK '(' సమాసం ')'
                        { $$ = new AstLogNot{$1, new AstSteady{$1, $3}}; }
        |       yD_CLOG2 '(' సమాసం ')'                   { $$ = new AstCLog2{$1, $3}; }
        |       yD_COS '(' సమాసం ')'                     { $$ = new AstCosD{$1, $3}; }
        |       yD_COSH '(' సమాసం ')'                    { $$ = new AstCoshD{$1, $3}; }
        |       yD_COUNTBITS '(' సమాసం ',' సమాసం ')'              { $$ = new Ast_లెక్క_బిట్లు{$1, $3, $5}; }
        |       yD_COUNTBITS '(' సమాసం ',' సమాసం ',' సమాసం ')'             { $$ = new Ast_లెక్క_బిట్లు{$1, $3, $5, $7}; }
        |       yD_COUNTBITS '(' సమాసం ',' సమాసం ',' సమాసం ',' సమాసం ')'    { $$ = new Ast_లెక్క_బిట్లు{$1, $3, $5, $7, $9}; }
        |       yD_COUNTBITS '(' సమాసం ',' సమాసం ',' సమాసం ',' సమాసం ',' సమాసం_List ')'
                        { $$ = new Ast_లెక్క_బిట్లు{$1, $3, $5, $7, $9};
                          BBUNSUP($11, "Unsupported: $countbits with more than 3 control fields"); }
        |       yD_COUNTONES '(' సమాసం ')'               { $$ = new Ast_లెక్క_Ones{$1, $3}; }
        |       yD_DIMENSIONS '(' సమాసం_Or_దత్తాంశం_Type ')'    { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_DIMENSIONS, $3}; }
        |       y_D_DIST_CHI_వర్గం '(' సమాసం ',' సమాసం ')'        { $$ = new Ast_Dist_Chi_వర్గం{$1, $3, $5}; }
        |       yD_DIST_ERLANG '(' సమాసం ',' సమాసం ',' సమాసం ')'   { $$ = new AstDistErlang{$1, $3, $5, $7}; }
        |       y_D_DIST_ఘాతాంక '(' సమాసం ',' సమాసం ')'       { $$ = new Ast_Dist_ఘాతాంక{$1, $3, $5}; }
        |       y_D_DIST_సాధారణ '(' సమాసం ',' సమాసం ',' సమాసం ')'   { $$ = new Ast_Dist_సాధారణ{$1, $3, $5, $7}; }
        |       y_D_DIST_పాయిసాన్ '(' సమాసం ',' సమాసం ')'   { $$ = new Ast_Dist_పాయిసాన్{$1, $3, $5}; }
        |       yD_DIST_T '(' సమాసం ',' సమాసం ')'         { $$ = new AstDistT{$1, $3, $5}; }
        |       y_D_DIST_ఏకరీతి '(' సమాసం ',' సమాసం ',' సమాసం ')'  { $$ = new Ast_Dist_ఏకరీతి{$1, $3, $5, $7}; }
        |       yD_EXP '(' సమాసం ')'                     { $$ = new AstExpD{$1, $3}; }
        |       yD_FALLING_GCLK '(' సమాసం ')'            { $$ = new AstFalling{$1, $3}; }
        |       yD_FELL '(' సమాసం ')'                    { $$ = new AstFell{$1, $3, nullptr}; }
        |       yD_FELL '(' సమాసం ',' సమాసం ')'           { $$ = new AstFell{$1, $3, GRAMMARP->create_Sen_వృక్షం_మారింది($1, $5)}; }
        |       yD_FELL_GCLK '(' సమాసం ')'               { $$ = new AstFell{$1, $3, GRAMMARP->create_Global_Clock_Sen_వృక్షం($1)}; }
        |       yD_FEOF '(' సమాసం ')'                    { $$ = new AstFEof{$1, $3}; }
        |       yD_FERROR '(' సమాసం ',' idClassSel ')'   { $$ = new AstFError{$1, $3, $5}; }
        |       yD_FGETC '(' సమాసం ')'                   { $$ = new AstFGetC{$1, $3}; }
        |       yD_FGETS '(' సమాసం ',' సమాసం ')'          { $$ = new AstFGetS{$1, $3, $5}; }
        |       yD_FOPEN '(' సమాసం ')'                   { $$ = new AstFOpenMcd{$1, $3}; }
        |       yD_FOPEN '(' సమాసం ',' సమాసం ')'          { $$ = new AstFOpen{$1, $3, $5}; }
        |       yD_FREAD '(' సమాసం ',' సమాసం ')'          { $$ = new AstFRead{$1, $3, $5, nullptr, nullptr}; }
        |       yD_FREAD '(' సమాసం ',' సమాసం ',' సమాసం ')'  { $$ = new AstFRead{$1, $3, $5, $7, nullptr}; }
        |       yD_FREAD '(' సమాసం ',' సమాసం ',' సమాసం ',' సమాసం ')'  { $$ = new AstFRead{$1, $3, $5, $7, $9}; }
        |       yD_FREAD '(' సమాసం ',' సమాసం ',' ',' సమాసం ')'  { $$ = new AstFRead{$1, $3, $5, nullptr, $8}; }
        |       yD_FREWIND '(' సమాసం ')'                 { $$ = new AstFRewind{$1, $3}; }
        |       yD_FUTURE_GCLK '(' సమాసం ')'             { $$ = new AstFuture{$1, $3, nullptr}; }
        |       yD_FLOOR '(' సమాసం ')'                   { $$ = new AstFloorD{$1, $3}; }
        |       yD_FSCANF '(' సమాసం ',' str commaVRDListE ')'    { $$ = new AstFScanF{$1, *$5, $3, $6}; }
        |       yD_FSEEK '(' సమాసం ',' సమాసం ',' సమాసం ')' { $$ = new AstFSeek{$1, $3, $5, $7}; }
        |       yD_FTELL '(' సమాసం ')'                   { $$ = new AstFTell{$1, $3}; }
        |       y_D_GET_INITIAL_RANDOM_విత్తనం parenE      { $$ = new Ast_Get_Initial_Random_విత్తనం{$1}; }
        |       yD_GLOBAL_CLOCK parenE                  { $$ = GRAMMARP->createGlobalClockParseRef($1); }
        |       y_D_ఎగువ '(' సమాసం_Or_దత్తాంశం_Type ')'          { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_ఎగువ, $3, nullptr}; }
        |       y_D_ఎగువ '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')' { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_ఎగువ, $3, $5}; }
        |       yD_HYPOT '(' సమాసం ',' సమాసం ')'          { $$ = new AstHypotD{$1, $3, $5}; }
        |       yD_INCREMENT '(' సమాసం_Or_దత్తాంశం_Type ')'     { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_INCREMENT, $3, nullptr}; }
        |       yD_INCREMENT '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')'    { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_INCREMENT, $3, $5}; }
        |       yD_INFERRED_DISABLE parenE              { $$ = new AstInferredDisable{$1}; }
        |       yD_ISUNBOUNDED '(' సమాసం ')'             { $$ = new AstIsUnbounded{$1, $3}; }
        |       yD_ISUNKNOWN '(' సమాసం ')'               { $$ = new AstIsUnknown{$1, $3}; }
        |       yD_ITOR '(' సమాసం ')'                    { $$ = new AstIToRD{$1, $3}; }
        |       y_D_ఎడమ '(' సమాసం_Or_దత్తాంశం_Type ')'          { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_ఎడమ, $3, nullptr}; }
        |       y_D_ఎడమ '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')' { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_ఎడమ, $3, $5}; }
        |       yD_LN '(' సమాసం ')'                      { $$ = new AstLogD{$1, $3}; }
        |       yD_LOG10 '(' సమాసం ')'                   { $$ = new AstLog10D{$1, $3}; }
        |       y_D_దిగువ '(' సమాసం_Or_దత్తాంశం_Type ')'           { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_దిగువ, $3, nullptr}; }
        |       y_D_దిగువ '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')'  { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_దిగువ, $3, $5}; }
        |       yD_ONEHOT '(' సమాసం ')'                  { $$ = new AstOneHot{$1, $3}; }
        |       yD_ONEHOT0 '(' సమాసం ')'                 { $$ = new AstOneHot0{$1, $3}; }
        |       yD_PAST '(' సమాసం ')'                    { $$ = new AstPast{$1, $3}; }
        |       yD_PAST '(' సమాసం ',' సమాసం_E ')'          { $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' సమాసం ',' సమాసం_E ',' సమాసం_E ')'
                        { ఒకవేళ ($7) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' సమాసం ',' సమాసం_E ',' సమాసం_E ',' clocking_eventE ')'
                        { ఒకవేళ ($7 || $9) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7, $9);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST_GCLK '(' సమాసం ')'               { $$ = new AstPast{$1, $3, nullptr, GRAMMARP->create_Global_Clock_Sen_వృక్షం($1)}; }
        |       yD_POW '(' సమాసం ',' సమాసం ')'            { $$ = new AstPowD{$1, $3, $5}; }
        |       yD_RANDOM '(' సమాసం ')'                  { $$ = new AstRand{$1, $3, false}; }
        |       yD_RANDOM parenE                        { $$ = new AstRand{$1, nullptr, false}; }
        |       yD_REALTIME parenE                      { $$ = new Ast_సమయం_D{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_REALTOBITS '(' సమాసం ')'              { $$ = new Ast_Real_గమ్యం_బిట్లు{$1, $3}; }
        |       yD_REWIND '(' సమాసం ')'                  { $$ = new AstFSeek{$1, $3, new AstConst{$1, 0}, new AstConst{$1, 0}}; }
        |       y_D_కుడి '(' సమాసం_Or_దత్తాంశం_Type ')'         { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_కుడి, $3, nullptr}; }
        |       y_D_కుడి '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')'        { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_కుడి, $3, $5}; }
        |       yD_RISING_GCLK '(' సమాసం ')'             { $$ = new AstRising{$1, $3}; }
        |       yD_ROSE '(' సమాసం ')'                    { $$ = new AstRose{$1, $3, nullptr}; }
        |       yD_ROSE '(' సమాసం ',' సమాసం ')'           { $$ = new AstRose{$1, $3, GRAMMARP->create_Sen_వృక్షం_మారింది($1, $5)}; }
        |       yD_ROSE_GCLK '(' సమాసం ')'               { $$ = new AstRose{$1, $3, GRAMMARP->create_Global_Clock_Sen_వృక్షం($1)}; }
        |       yD_RTOI '(' సమాసం ')'                    { $$ = new AstRToIS{$1, $3}; }
        |       yD_SAMPLED '(' సమాసం ')'                 { $$ = new AstSampled{$1, $3, $3->dtypep()}; }
        |       yD_SFORMATF '(' సమాసం_Disp_List ')'        { $$ = new AstSFormatF{$1, AstSFormatF::సమాసం_Format{}, $3, 'd', false}; }
        |       yD_SHORTREALTOBITS '(' సమాసం ')'         { $$ = new Ast_Real_గమ్యం_బిట్లు{$1, $3}; UNSUPREAL($1); }
        |       yD_SIGNED '(' సమాసం ')'                  { $$ = new AstSigned{$1, $3}; }
        |       yD_SIN '(' సమాసం ')'                     { $$ = new AstSinD{$1, $3}; }
        |       yD_SINH '(' సమాసం ')'                    { $$ = new AstSinhD{$1, $3}; }
        |       y_D_కొలత '(' సమాసం_Or_దత్తాంశం_Type ')'          { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_కొలత, $3, nullptr}; }
        |       y_D_కొలత '(' సమాసం_Or_దత్తాంశం_Type ',' సమాసం ')' { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_కొలత, $3, $5}; }
        |       yD_SQRT '(' సమాసం ')'                    { $$ = new AstSqrtD{$1, $3}; }
        |       yD_SSCANF '(' సమాసం ',' str commaVRDListE ')'    { $$ = new AstSScanF{$1, *$5, $3, $6}; }
        |       yD_STABLE '(' సమాసం ')'                  { $$ = new AstStable{$1, $3, nullptr}; }
        |       yD_STABLE '(' సమాసం ',' సమాసం ')'         { $$ = new AstStable{$1, $3, GRAMMARP->create_Sen_వృక్షం_మారింది($1, $5)}; }
        |       yD_STABLE_GCLK '(' సమాసం ')'             { $$ = new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_వృక్షం($1)}; }
        |       yD_STEADY_GCLK '(' సమాసం ')'             { $$ = new AstSteady{$1, $3}; }
        |       yD_STIME parenE
                        { $$ = new AstSel{$1, new Ast_సమయం{$1, VTimescale{VTimescale::NONE}}, 0, 32}; }
        |       yD_TAN '(' సమాసం ')'                     { $$ = new AstTanD{$1, $3}; }
        |       yD_TANH '(' సమాసం ')'                    { $$ = new AstTanhD{$1, $3}; }
        |       yD_TESTPLUSARGS '(' సమాసం ')'            { $$ = new Ast_Test_Plus_వాదనలు{$1, $3}; }
        |       y_D_సమయం parenE                          { $$ = new Ast_సమయం{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_TIMEPRECISION                        { $$ = new Ast_సమయం_Precision{$1}; }
        |       yD_TIMEPRECISION '(' ')'                { $$ = new Ast_సమయం_Precision{$1}; }
        |       yD_TIMEPRECISION '(' idClassSel ')'     { $$ = new Ast_సమయం_Precision{$1}; DEL($3); }
        |       yD_TIMEUNIT                             { $$ = new Ast_సమయం_Unit{$1}; }
        |       yD_TIMEUNIT '(' ')'                     { $$ = new Ast_సమయం_Unit{$1}; }
        |       yD_TIMEUNIT '(' idClassSel ')'          { $$ = new Ast_సమయం_Unit{$1}; DEL($3); }
        |       yD_TYPENAME '(' సమాసం_Or_దత్తాంశం_Type ')'      { $$ = new Ast_Attr_యొక్క{$1, VAttrType::TYPENAME, $3}; }
        |       yD_UNGETC '(' సమాసం ',' సమాసం ')'         { $$ = new AstFUngetC{$1, $5, $3}; }  // Arg swap to file first
        |       yD_UNPACKED_DIMENSIONS '(' సమాసం_Or_దత్తాంశం_Type ')'   { $$ = new Ast_Attr_యొక్క{$1, VAttrType::DIM_UNPK_DIMENSIONS, $3}; }
        |       yD_UNSIGNED '(' సమాసం ')'                { $$ = new AstUnsigned{$1, $3}; }
        |       yD_URANDOM '(' సమాసం ')'                 { $$ = new AstRand{$1, $3, true}; }
        |       yD_URANDOM parenE                       { $$ = new AstRand{$1, nullptr, true}; }
        |       yD_URANDOM_RANGE '(' సమాసం ')'           { $$ = new AstURandomRange{$1, $3, new AstConst{$1, 0}}; }
        |       yD_URANDOM_RANGE '(' సమాసం ',' సమాసం ')'  { $$ = new AstURandomRange{$1, $3, $5}; }
        |       yD_VALUEPLUSARGS '(' సమాసం ',' సమాసం ')'  { $$ = new Ast_విలువ_Plus_వాదనలు{$1, $3, $5}; }
        ;

severity_system_task<nodep>: // IEEE: severity_system_task/elaboration_severity_system_task (1800-2009)
        //                      // Elaboration-time task; V3Width evaluates and removes it
                severity_system_task_guts ';'           { $$ = $1; }
        ;

severity_system_task_guts<nodep>:    // IEEE: part of severity_system_task (1800-2009)
        //                      // $fatal first argument is exit number, must be constant
                yD_INFO parenE                          { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, nullptr}; }
        |       yD_INFO '(' సమాసం_List ')'                { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, $3}; }
        |       yD_WARNING parenE                       { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, nullptr}; }
        |       yD_WARNING '(' సమాసం_List ')'             { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, $3}; }
        |       yD_ERROR parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, nullptr}; }
        |       yD_ERROR '(' సమాసం_List ')'               { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, $3}; }
        |       yD_FATAL parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; }
        |       yD_FATAL '(' సమాసం ')'                   { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; DEL($3); }
        |       yD_FATAL '(' సమాసం ',' సమాసం_List_E ')'     { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, $5}; DEL($3); }
        ;

system_Dpi_వాదనలు_E<argp>:           // IEEE: part of system_if_call for arguments of $dpi call
                parenE                                  { $$ = nullptr; }
        |       '(' సమాసం_List ')'                        { $$ = GRAMMARP->argWrapList($2); }
        ;

property_actual_arg<కణుపు_Exprp>:  // ==IEEE: property_actual_arg
        //                      // IEEE: property_expr
        //                      // IEEE: sequence_actual_arg
        //UNSUP pev_expr                                { $$ = $1; }
        //UNSUP remove below:
                pexpr                                   { $$ = $1; }
        //                      // IEEE: sequence_expr
        //                      // property_expr already includes sequence_expr
        ;

సమాసం_Or_దత్తాంశం_Type<nodep>:          // expr | data_type: combined to prevent conflicts
                సమాసం                                    { $$ = $1; }
        //                      // data_type includes id that overlaps expr, so special flavor
        //                      // data_type expanded:
        |       దత్తాంశం_type_No_Ref                          { $$ = $1; }
        //
        //                      // Conflicts with non-type id, resolved in V3LinkDot
        //                      // NO: packageClassScopeE idType packed_dimensionListE
        //
        |       packageClassScopeE idType parameter_విలువ_assignment_Class packed_dimensionListE
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        //                      // not in spec, but needed for $past(sig,1,,@(posedge clk))
        //UNSUP event_control                           { }
        ;

//UNSUPexprOrDataTypeOrMinTypMax<nodep>:  // exprOrDataType or mintypmax_expression
//UNSUP         expr                                    { $$ = $1; }
//UNSUP |       expr ':' expr ':' expr                  { $$ = $3; }
//UNSUP //                      // data_type includes id that overlaps expr, so special flavor
//UNSUP |       data_type                               { $$ = $1; }
//UNSUP //                      // not in spec, but needed for $past(sig,1,,@(posedge clk))
//UNSUP |       event_control                           { $$ = $1; }
//UNSUP ;

//UNSUPexprOrDataTypeList<nodep>:
//UNSUP         exprOrDataType                          { $$ = $1; }
//UNSUP |       exprOrDataTypeList ',' exprOrDataType   { $$ = addNextNull($1, $3); }
//UNSUP ;

list_యొక్క_arguments_E<argp>:  // IEEE: [list_of_arguments]
                వాదనలు_Dotted_List                          { $$ = $1; }
        |       వాదనలు_సమాసం_List_E
                        { ఒకవేళ ($1->ఖాళీలు_Connect_No_తదుపరి()) {
                              $1->delete_వృక్షం(); $$ = nullptr;  // Mis-created when have 'func()'
                          } లేకపోతే { $$ = $1; } }
        |       వాదనలు_సమాసం_List_E ',' వాదనలు_Dotted_List        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

task_declaration<కణుపు_FTaskp>:   // ==IEEE: task_declaration
                yTASK dynamic_override_specifiersE lifetimeE taskId tfGuts yENDTASK చివర_Label_E
                        { $$ = $4; $$->కూడు_Stmtsp($5);
                          $$->పునాది_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->చివర_Label($<fl>7, $$, $7); }
        ;

task_prototype<కణుపు_FTaskp>:             // ==IEEE: task_prototype
                yTASK dynamic_override_specifiersE taskId '(' tf_పోర్టు_list_E ')'
                        { $$ = $3; $$->కూడు_Stmtsp($5); $$->prototype(true); }
        |       yTASK dynamic_override_specifiersE taskId
                        { $$ = $3; $$->prototype(true); }
        ;

function_declaration<కణుపు_FTaskp>:       // IEEE: function_declaration + function_body_declaration
                yFUNCTION dynamic_override_specifiersE lifetimeE funcId funcIsolateE tfGuts yENDFUNCTION చివర_Label_E
                        { $$ = $4; $$->కూడు_Stmtsp($6);
                          $$->పునాది_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->చివర_Label($<fl>8, $$, $8); }
        |       yFUNCTION dynamic_override_specifiersE lifetimeE funcIdNew funcIsolateE tfNewGuts yENDFUNCTION చివర_Label_E
                        { $$ = $4; $$->కూడు_Stmtsp($6);
                          $$->పునాది_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->చివర_Label($<fl>8, $$, $8); }
        ;

function_prototype<కణుపు_FTaskp>: // IEEE: function_prototype
                yFUNCTION dynamic_override_specifiersE funcId '(' tf_పోర్టు_list_E ')'
                        { $$ = $3; $$->కూడు_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcId
                        { $$ = $3; $$->prototype(true); }
        ;

class_constructor_prototype<కణుపు_FTaskp>:        // ==IEEE: class_constructor_prototype
        //                      // IEEE has no dynamic_override_specifiersE,
        //                      // but required to avoid conflicts, so we must check after parsing
                yFUNCTION dynamic_override_specifiersE funcIdNew '(' class_constructor_arg_listE ')' ';'
                        { $$ = $3; $$->కూడు_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcIdNew ';'
                        { $$ = $3; $$->prototype(true); }
        ;

funcIsolateE<cint>:
                /*ఖాళీలు*/                             { $$ = 0; }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = 1; }
        ;

పద్ధతి_prototype<కణుపు_FTaskp>:
                task_prototype                          { $$ = $1; }
        |       function_prototype                      { $$ = $1; }
        ;

lifetimeE<lifetime>:            // IEEE: [lifetime]
                /*ఖాళీలు*/                             { $$ = VLifetime::NONE; }
        |       lifetime                                { $$ = $1; }
        ;

lifetime<lifetime>:             // ==IEEE: lifetime
        //                      // Note lifetime used by members is instead under memberQual
                ySTATIC__ETC                            { $$ = VLifetime::STATIC_EXPLICIT; }
        |       yAUTOMATIC                              { $$ = VLifetime::AUTOMATIC_EXPLICIT; }
        ;

taskId<కణుపు_FTaskp>:
                id
                        { $$ = new AstTask{$<fl>$, *$1, nullptr};
                          $$->verilogTask(true); }
        //
        |       id/*interface_identifier*/ '.' idAny
                        { $$ = new AstTask{$<fl>$, *$3, nullptr};
                          $$->verilogTask(true);
                          $$->iface_పోర్టు_Name(*$1); }
        //
        |       packageClassScope id
                        { $$ = new AstTask{$<fl>$, *$2, nullptr};
                          $$->verilogTask(true);
                          $$->classOrPackagep($1);
                          $$->class_పద్ధతి(true); }
        ;

funcId<కణుపు_FTaskp>:             // IEEE: function_data_type_or_implicit + part of function_body_declaration
        //                      // IEEE: function_data_type_or_implicit must be expanded here to prevent conflict
        //                      // function_data_type expanded here to prevent conflicts with
        //                      // implicit_type:empty vs data_type:ID
                /**/ fIdScoped
                        { $$ = $1;
                          $$->fvarp(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT}); }
        |       signingE rangeList fIdScoped
                        { $$ = $3;
                          $$->fvarp(GRAMMARP->కూడు_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)); }
        |       signing fIdScoped
                        { $$ = $2;
                          $$->fvarp(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}); }
        |       దత్తాంశం_type_No_Ref fIdScoped
                        { $$ = $2;
                          $$->fvarp($1); }
        |       packageClassScopeE idInstType packed_dimensionListE fIdScoped
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = $4;
                          $$->fvarp(GRAMMARP->createArray(refp, $3, true)); }
        |       packageClassScopeE idInstType parameter_విలువ_assignment_Class packed_dimensionListE fIdScoped
                        { AstRefDType* స్థిరరాశి refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = $5;
                          $$->fvarp(GRAMMARP->createArray(refp, $4, true)); }
        //                      // To verilator tasks are the same as void functions (we separately detect time passing)
        |       yVOID taskId
                        { $$ = $2;  // Internals represent it as a task, not a function (TODO cleanup)
                          $$->verilogTask(false);
                          $$->verilogFunction(true); }
        ;

funcIdNew<కణుపు_FTaskp>:          // IEEE: from class_constructor_declaration
                yNEW__ETC
                        { $$ = new AstFunc{$<fl>1, "new", nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->isConstructor(true); }
        |       yNEW__PAREN
                        { $$ = new AstFunc{$<fl>1, "new", nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->isConstructor(true); }
        |       packageClassScopeNoId yNEW__PAREN
                        { $$ = new AstFunc{$<fl>2, "new", nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->classOrPackagep($1);
                          $$->isConstructor(true);
                          $$->class_పద్ధతి(true); }
        ;

fIdScoped<funcp>:               // IEEE: part of function_body_declaration/task_body_declaration
        //                      // IEEE: [ interface_identifier '.' | class_scope ] function_identifier
                id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstFunc{$<fl>$, *$1, nullptr, nullptr};
                          $$->verilogFunction(true); }
        //
        |       id/*interface_identifier*/ '.' idAny
                        { $<fl>$ = $<fl>1;
                          $$ = new AstFunc{$<fl>$, *$3, nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->iface_పోర్టు_Name(*$1); }
        //
        |       packageClassScope id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstFunc{$<fl>$, *$2, nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->class_పద్ధతి(true);
                          $$->classOrPackagep($1); }
        ;

tfGuts<nodep>:
                '(' tf_పోర్టు_list_E ')' ';' tfBodyE       { $$ = కూడు_తదుపరి_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfNewGuts<nodep>:
                '(' class_constructor_arg_listE ')' ';' tfBodyE       { $$ = కూడు_తదుపరి_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfBodyE<nodep>:                 // IEEE: part of function_body_declaration/task_body_declaration
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       tf_అంశం_declaration_List                 { $$ = $1; }
        |       tf_అంశం_declaration_List stmtList        { $$ = కూడు_తదుపరి_Null($1, $2); }
        |       stmtList                                { $$ = $1; }
        ;

tf_అంశం_declaration_List<nodep>:
                tf_అంశం_declaration                     { $$ = $1; }
        |       tf_అంశం_declaration_List tf_అంశం_declaration     { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

tf_అంశం_declaration<nodep>:     // ==IEEE: tf_item_declaration
                ఖండం_అంశం_declaration                  { $$ = $1; }
        |       tf_పోర్టు_declaration                     { $$ = $1; }
        |       tf_అంశం_declaration_Verilator            { $$ = $1; }
        ;

tf_అంశం_declaration_Verilator<nodep>:    // Verilator extensions
                y_VL_ప్రజా                              { $$ = new AstPragma{$1, VPragmaType::ప్రజా_TASK}; v3Global.dpi(true); }
        |       yVL_NO_INLINE_TASK                      { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_TASK}; }
        ;

tf_పోర్టు_list_E<nodep>:           // IEEE: tf_port_list + empty
        //                      // Empty covered by tf_port_item
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(ఆదానం); }
        /*cont*/    tf_పోర్టు_list_List                    { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

tf_పోర్టు_list_List<nodep>:        // IEEE: part of tf_port_list
                tf_పోర్టు_అంశం                            { $$ = $1; }
        |       tf_పోర్టు_list_List ',' tf_పోర్టు_అంశం       { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

class_constructor_arg_listE<nodep>:  // IEEE: class_constructor_arg_list or empty
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(ఆదానం); }
        /*cont*/    class_constructor_arg_listList      { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

class_constructor_arg_listList<nodep>:  // IEEE: part of class_constructor_arg_list
                class_constructor_arg                   { $$ = $1; }
        |       class_constructor_arg_listList ',' class_constructor_arg       { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

class_constructor_arg<nodep>:  // ==IEEE: class_constructor_arg
                tf_పోర్టు_అంశం                            { $$ = $1; }
        |       yDEFAULT                                { $$ = nullptr; BBUNSUP($1, "Unsupported: new constructor 'default' argument"); }
        ;

tf_పోర్టు_అంశం<nodep>:            // ==IEEE: tf_port_item
        //                      // We split tf_port_item into the type and assignment as don't know what follows a comma
                /*ఖాళీలు*/                             { $$ = nullptr; PINNUMINC(); }  // For example a ",," port
        |       tf_పోర్టు_అంశం_Front tf_పోర్టు_అంశం_Assignment { $$ = $2; }
        |       tf_పోర్టు_అంశం_Assignment                  { $$ = $1; }
        ;

tf_పోర్టు_అంశం_Front:              // IEEE: part of tf_port_item, which has the data type
                దత్తాంశం_type                               { VARDTYPE($1); }
        |       signingE rangeList
                        { Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                              new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true);
                          VARDTYPE(dtp); }
        |       signing
                        { Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1};
                          VARDTYPE(dtp); }
        |       yVAR దత్తాంశం_type                          { VARDTYPE($2); }
        |       yVAR implicit_typeE                     { VARDTYPE($2); }
        //
        |       tf_పోర్టు_అంశం_Dir /*implicit*/            { VARDTYPE(nullptr); /*default_nettype-see spec*/ }
        |       tf_పోర్టు_అంశం_Dir దత్తాంశం_type               { VARDTYPE($2); }
        |       tf_పోర్టు_అంశం_Dir signingE rangeList
                        { Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                              new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE(dtp); }
        |       tf_పోర్టు_అంశం_Dir signing
                        { Ast_కణుపు_DType* స్థిరరాశి dtp = new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $2};
                          VARDTYPE(dtp); }
        |       tf_పోర్టు_అంశం_Dir yVAR దత్తాంశం_type          { VARDTYPE($3); }
        |       tf_పోర్టు_అంశం_Dir yVAR implicit_typeE     { VARDTYPE($3); }
        ;

tf_పోర్టు_అంశం_Dir:                // IEEE: part of tf_port_item, direction
                పోర్టు_direction                          { }  // port_direction sets VARIO
        ;

tf_పోర్టు_అంశం_Assignment<varp>:   // IEEE: part of tf_port_item, which has assignment
                id variable_dimensionListE sigAttrListE సమాసం_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); ఒకవేళ ($4) $$->valuep($4); }
        ;

parenE:
                /*ఖాళీలు*/                             { }
        |       '(' ')'                                 { }
        ;

//      method_call:            // ==IEEE: method_call + method_call_body
//                              // IEEE: method_call_root '.' method_identifier [ '(' list_of_arguments ')' ]
//                              //   "method_call_root '.' method_identifier" looks just like "expr '.' id"
//                              //   "method_call_root '.' method_identifier (...)" looks just like "expr '.' tf_call"
//                              // IEEE: built_in_method_call
//                              //   method_call_root not needed, part of expr resolution
//                              // What's left is below array_methodNoRoot
array_పద్ధతి_No_మూలం<కణుపు_FTask_Refp>:
                yOR                                     { $$ = new AstFuncRef{$1, "or"}; }
        |       yAND                                    { $$ = new AstFuncRef{$1, "and"}; }
        |       yXOR                                    { $$ = new AstFuncRef{$1, "xor"}; }
        |       yUNIQUE                                 { $$ = new AstFuncRef{$1, "unique"}; }
        ;

array_పద్ధతి_With<కణుపు_Exprp>:
                array_పద్ధతి_No_మూలం parenE               { $$ = $1; }
        |       array_పద్ధతి_No_మూలం parenE yWITH__PAREN '(' సమాసం ')'
                        { $$ = new AstWithParse{$3, $1, $5}; }
        |       array_పద్ధతి_No_మూలం '(' సమాసం ')' yWITH__PAREN '(' సమాసం ')'
                        { $$ = new AstWithParse{$5, $1, $7}; $1->కూడు_Argsp(new AstArg{$<fl>3, "", $3}); }
        ;

dpi_import_export<nodep>:       // ==IEEE: dpi_import_export
                yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype ';'
                        { $$ = $5;
                          ఒకవేళ (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_శుద్ధ($3 == iprop_శుద్ధ);
                          $5->dpiImport(true);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype yVL_DPI_C_DECL yaSTRING ';'
                        { $$ = $5;
                          ఒకవేళ (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_శుద్ధ($3 == iprop_శుద్ధ);
                          $5->dpiImport(true);
                          ఒకవేళ (*$7 != "") $5->dpiCDecl(*$7);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE task_prototype ';'
                        { $$ = $5;
                          ఒకవేళ (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_శుద్ధ($3 == iprop_శుద్ధ);
                          $5->dpiImport(true);
                          $5->dpiTask(true);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yEXPORT yaSTRING dpi_importLabelE yFUNCTION idAny ';'
                        { $$ = new AstDpiExport{$<fl>5, *$5, *$3};
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yEXPORT yaSTRING dpi_importLabelE yTASK     idAny ';'
                        { $$ = new AstDpiExport{$<fl>5, *$5, *$3};
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        ;

dpi_importLabelE<strp>:         // IEEE: part of dpi_import_export
                /*ఖాళీలు*/                             { స్థిర string s; $$ = &s; }
        |       idAny/*c_identifier*/ '='               { $$ = $1; $<fl>$ = $<fl>1; }
        ;

dpi_tf_import_propertyE<iprop>: // IEEE: [ dpi_function_import_property + dpi_task_import_property ]
                /*ఖాళీలు*/                             { $$ = iprop_NONE; }
        |       yCONTEXT                                { $$ = iprop_CONTEXT; }
        |       y_శుద్ధ                                   { $$ = iprop_శుద్ధ; }
        ;


//************************************************
// Expressions
//
// ~l~ means this is the (l)eft hand side of any operator
//     it will get replaced by "", "f" or "s"equence
// ~r~ means this is a (r)ight hand later expansion in the same statement,
//     not under parenthesis for <= disambiguation
//     it will get replaced by "", or "f"
// ~p~ means this is a (p)arenthetized expression
//     it will get replaced by "", or "s"equence

సమాసం_Eq_E<కణుపు_Exprp>:             // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' సమాసం                                { $$ = $2; }
        ;

సమాసం_Or_దత్తాంశం_Type_Eq_E<nodep>:       // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' సమాసం_Or_దత్తాంశం_Type                      { $$ = $2; }
        ;

const_సమాసం<కణుపు_Exprp>:
                సమాసం                                    { $$ = $1; }
        ;

సమాసం_E<కణుపు_Exprp>:               // IEEE: optional expression
                /*empty*/                               { $$ = nullptr; }
        |       సమాసం                                    { $$ = $1; }
        ;

సమాసం<కణుపు_Exprp>:                // IEEE: part of expression/constant_expression/primary
        // *SEE BELOW*          // IEEE: primary/constant_primary
        //
        //                      // IEEE: unary_operator primary
                '+' ~r~సమాసం     %prec prUNARYARITH      { $$ = $2; }
        |       '-' ~r~సమాసం     %prec prUNARYARITH      { $$ = new AstNegate{$1, $2}; }
        |       '!' ~r~సమాసం     %prec prNEGATION        { $$ = new AstLogNot{$1, $2}; }
        |       '&' ~r~సమాసం     %prec prREDUCTION       { $$ = new AstRedAnd{$1, $2}; }
        |       '~' ~r~సమాసం     %prec prNEGATION        { $$ = new AstNot{$1, $2}; }
        |       '|' ~r~సమాసం     %prec prREDUCTION       { $$ = new AstRedOr{$1, $2}; }
        |       '^' ~r~సమాసం     %prec prREDUCTION       { $$ = new AstRedXor{$1, $2}; }
        |       yP_NAND ~r~సమాసం %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedAnd{$1, $2}}; }
        |       yP_NOR  ~r~సమాసం %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedOr{$1, $2}}; }
        |       yP_XNOR ~r~సమాసం %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedXor{$1, $2}}; }
        //
        //                      // IEEE: inc_or_dec_expression
        |       ~l~inc_or_dec_వ్యక్తీకరణ                { $<fl>$ = $<fl>1; $$ = $1; }
        //
        //                      // IEEE: '(' operator_assignment ')'
        //                      // Need exprScope of variable_lvalue to prevent conflict
        |       '(' ~p~సమాసం_Scope '='          సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, $4},
                                               $2->clone_వృక్షం_శుద్ధ(true)};
                          ASSIGNEQEXPR($<fl>3); }
        |       '(' ~p~సమాసం_Scope yP_PLUSEQ    సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new Ast_కూడు{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_MINUSEQ   సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstSub{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_TIMESEQ   సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstMul{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_DIVEQ     సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstDiv{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_MODEQ     సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstModDiv{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_ANDEQ     సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstAnd{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_OREQ      సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstOr{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_XOREQ     సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new AstXor{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_SLEFTEQ   సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new Ast_జరుపు_L{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_SRIGHTEQ  సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new Ast_జరుపు_R{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        |       '(' ~p~సమాసం_Scope yP_SSRIGHTEQ సమాసం ')'
                        { $$ = new Ast_సమాసం_Stmt{$1, new Ast_కేటాయించు{$3, $2, new Ast_జరుపు_RS{$3, $2->clone_వృక్షం_శుద్ధ(true), $4}},
                                               $2->clone_వృక్షం_శుద్ధ(true)}; }
        //
        //                      // IEEE: expression binary_operator expression
        |       ~l~సమాసం '+' ~r~సమాసం                     { $$ = new Ast_కూడు{$2, $1, $3}; }
        |       ~l~సమాసం '-' ~r~సమాసం                     { $$ = new AstSub{$2, $1, $3}; }
        |       ~l~సమాసం '*' ~r~సమాసం                     { $$ = new AstMul{$2, $1, $3}; }
        |       ~l~సమాసం '/' ~r~సమాసం                     { $$ = new AstDiv{$2, $1, $3}; }
        |       ~l~సమాసం '%' ~r~సమాసం                     { $$ = new AstModDiv{$2, $1, $3}; }
        |       ~l~సమాసం yP_EQUAL ~r~సమాసం                { $$ = new AstEq{$2, $1, $3}; }
        |       ~l~సమాసం yP_NOTEQUAL ~r~సమాసం             { $$ = new AstNeq{$2, $1, $3}; }
        |       ~l~సమాసం yP_CASEEQUAL ~r~సమాసం            { $$ = new AstEqCase{$2, $1, $3}; }
        |       ~l~సమాసం yP_CASENOTEQUAL ~r~సమాసం         { $$ = new AstNeqCase{$2, $1, $3}; }
        |       ~l~సమాసం yP_WILDEQUAL ~r~సమాసం            { $$ = new AstEqWild{$2, $1, $3}; }
        |       ~l~సమాసం yP_WILDNOTEQUAL ~r~సమాసం         { $$ = new AstNeqWild{$2, $1, $3}; }
        |       ~l~సమాసం yP_ANDAND ~r~సమాసం               { $$ = new AstLogAnd{$2, $1, $3}; }
        |       ~l~సమాసం yP_OROR ~r~సమాసం                 { $$ = new AstLogOr{$2, $1, $3}; }
        |       ~l~సమాసం yP_POW ~r~సమాసం                  { $$ = new AstPow{$2, $1, $3}; }
        |       ~l~సమాసం '<' ~r~సమాసం                     { $$ = new AstLt{$2, $1, $3}; }
        |       ~l~సమాసం '>' ~r~సమాసం                     { $$ = new AstGt{$2, $1, $3}; }
        |       ~l~సమాసం yP_GTE ~r~సమాసం                  { $$ = new AstGte{$2, $1, $3}; }
        |       ~l~సమాసం '&' ~r~సమాసం                     { $$ = new AstAnd{$2, $1, $3}; }
        |       ~l~సమాసం '|' ~r~సమాసం                     { $$ = new AstOr{$2, $1, $3}; }
        |       ~l~సమాసం '^' ~r~సమాసం                     { $$ = new AstXor{$2, $1, $3}; }
        |       ~l~సమాసం yP_XNOR ~r~సమాసం                 { $$ = new AstNot{$2, new AstXor{$2, $1, $3}}; }
        |       ~l~సమాసం yP_NOR ~r~సమాసం
                        { $$ = new AstNot{$2, new AstOr{$2, $1, $3}};
                          $2->v3error("Syntax error: '~|' is not a 'nor' binary operator, but a unary '~ |'"
                                      << YYNEWLINE << $<fl>1->warn_మరిన్ని() << "... Suggest '~ (... | ...)'"); }
        |       ~l~సమాసం yP_NAND ~r~సమాసం
                        { $$ = new AstNot{$2, new AstAnd{$2, $1, $3}};
                          $2->v3error("Syntax error: '~&' is not a 'nand' binary operator, but a unary '~ &'"
                                      << YYNEWLINE << $<fl>1->warn_మరిన్ని() << "... Suggest '~ (... & ...)'"); }
        |       ~l~సమాసం yP_SLEFT ~r~సమాసం                { $$ = new Ast_జరుపు_L{$2, $1, $3}; }
        |       ~l~సమాసం yP_SRIGHT ~r~సమాసం               { $$ = new Ast_జరుపు_R{$2, $1, $3}; }
        |       ~l~సమాసం yP_SSRIGHT ~r~సమాసం              { $$ = new Ast_జరుపు_RS{$2, $1, $3}; }
        |       ~l~సమాసం yP_LTMINUSGT ~r~సమాసం            { $$ = new AstLogEq{$2, $1, $3}; }
        //
        //                      // IEEE: expression binary_operator expression (type compare see IEEE footnote)
        |       type_referenceEq yP_CASEEQUAL type_referenceBoth     { $$ = new AstEqT{$2, $1, $3}; }
        |       type_referenceEq yP_CASENOTEQUAL type_referenceBoth  { $$ = new AstNeqT{$2, $1, $3}; }
        |       type_referenceEq yP_EQUAL type_referenceBoth         { $$ = new AstEqT{$2, $1, $3}; }
        |       type_referenceEq yP_NOTEQUAL type_referenceBoth      { $$ = new AstNeqT{$2, $1, $3}; }
        //                      // IEEE: expr yP_MINUSGT expr  (1800-2009)
        //                      // Full IEEE 1800-2023 18.7.2 "expr ->
        //                      // constraint_set" is split: this rule handles
        //                      // the bare-expression RHS as a boolean
        //                      // (AstLogIf), constraint_expression has one
        //                      // production per non-expression RHS shape.
        |       ~l~సమాసం yP_MINUSGT ~r~సమాసం              { $$ = new AstLogIf{$2, $1, $3}; }
        //
        //                      // <= is special, as we need to disambiguate it with <= assignment
        //                      // We copy all of expr to fexpr and rename this token to a fake one.
        |       ~l~సమాసం yP_LTE~f__IGNORE~ ~r~సమాసం       { $$ = new AstLte{$2, $1, $3}; }
        //
        //                      // IEEE: conditional_expression
        |       ~l~సమాసం '?' ~r~సమాసం ':' ~r~సమాసం         { $$ = new AstCond{$2, $1, $3, $5}; }
        //
        //                      // IEEE: inside_expression
        |       ~l~సమాసం yINSIDE '{' range_list '}'      { $$ = new AstInside{$2, $1, $4}; }
        //
        //                      // IEEE: tagged_union_expression
        //                      // yTAGGED__NONPRIMARY = tokenPipeline determined no primary follows
        |       yTAGGED__NONPRIMARY idAny/*member*/ %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, nullptr}; }
        //                      // yTAGGED = primary follows; handle specific primary types
        //                      // Parenthesized expression
        |       yTAGGED idAny/*member*/ '(' సమాసం ')' %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, $4}; }
        //                      // Assignment patterns like tagged Add '{a, b, c}
        |       yTAGGED idAny/*member*/ assignment_నమూనా %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, $3}; }
        //                      // Integer literal
        |       yTAGGED idAny/*member*/ yaINTNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, new AstConst{$<fl>3, *$3}}; }
        //                      // Float literal
        |       yTAGGED idAny/*member*/ yaFLOATNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, new AstConst{$<fl>3, AstConst::RealDouble{}, $3}}; }
        //                      // String literal
        |       yTAGGED idAny/*member*/ yaSTRING %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, new AstConst{$<fl>3, AstConst::VerilogStringLiteral{}, *$3}}; }
        //                      // null literal
        |       yTAGGED idAny/*member*/ yNULL %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, new AstConst{$3, AstConst::Null{}}}; }
        //                      // Identifier as value
        |       yTAGGED idAny/*member*/ idAny %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, new AstParseRef{$<fl>3, *$3, nullptr, nullptr}}; }
        //                      // Concatenation
        |       yTAGGED idAny/*member*/ '{' cateList '}' %prec prTAGGED
                        { $$ = new Ast_Tagged_సమాసం{$1, *$2, $4}; }
        //
        //======================// IEEE: primary/constant_primary
        //
        //                      // IEEE: primary_literal (minus string, which is handled specially)
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       సమయం_అంకె_Adjusted                         { $$ = $1; }
        |       strAsInt~noStr__IGNORE~                 { $$ = $1; }
        //
        //                      // IEEE: "... hierarchical_identifier select"  see below
        //
        //                      // IEEE: empty_unpacked_array_concatenation
        //                      // IEEE: (aka empty_queue, empty_unpacked_array_concatenation)
        |       '{' '}'                                 { $$ = new Ast_ఖాళీలు_వరుస{$1}; }
        //
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Part of exprOkLvalue below
        //
        //                      // IEEE: multiple_concatenation/constant_multiple_concatenation
        |       '{' const_సమాసం '{' cateList '}' '}'
                        { $$ = new AstReplicate{$3, $4, $2}; }
        |       '{' const_సమాసం '{' cateList '}' '}' '[' సమాసం ']'
                        { $$ = new Ast_Sel_బిట్టు{$7, new AstReplicate{$3, $4, $2}, $8}; }
        |       '{' const_సమాసం '{' cateList '}' '}' '[' const_సమాసం ':' const_సమాసం ']'
                        { $$ = new Ast_Sel_వెలికితీయి{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_సమాసం '{' cateList '}' '}' '[' సమాసం yP_PLUSCOLON const_సమాసం ']'
                        { $$ = new AstSelPlus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_సమాసం '{' cateList '}' '}' '[' సమాసం yP_MINUSCOLON const_సమాసం ']'
                        { $$ = new AstSelMinus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        //                      // UNSUP some other rules above
        //
        //                      // IEEE grammar error: function_subroutine_call [ range_expression ]
        //                      // should be instead:  function_subroutine_call [ part_select_range ]
        |       function_subroutine_call_No_పద్ధతి
                        { $$ = $1; }
        |       function_subroutine_call_No_పద్ధతి part_select_rangeList
                        { $$ = GRAMMARP->scrubSel($1, $2); }
        //                      // method_call
        |       ~l~సమాసం '.' function_subroutine_call_No_పద్ధతి
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~సమాసం '.' function_subroutine_call_No_పద్ధతి part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //                      // method_call:array_method requires a '.'
        |       ~l~సమాసం '.' array_పద్ధతి_With
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~సమాసం '.' array_పద్ధతి_With part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //
        //                      // IEEE: let_expression
        //                      // see funcRef
        //
        //                      // IEEE: '(' mintypmax_expression ')'
        |       ~noPar__IGNORE~'(' సమాసం ')'             { $$ = $2; }
        |       ~noPar__IGNORE~'(' సమాసం ':' సమాసం ':' సమాసం ')'
                        { $$ = $4; MINTYPMAXDLYUNSUP($4); DEL($2); DEL($6); }
        //                      // PSL rule
        |       '_' '(' సమాసం ')'                        { $$ = $3; }    // Arbitrary Verilog inside PSL
        //
        //                      // IEEE: cast/constant_cast
        //                      // expanded from casting_type
        |       simple_typeNoRef yP_TICK '(' సమాసం ')'
                        { $$ = new AstCast{$2, $4, VFlag_బిడ్డ_DType{}, $1}; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // Causes conflict, so handled post-parse
        //                      // NO: packageClassScopeE idType yP_TICK '(' expr ')'
        //
        |       yTYPE__ETC '(' సమాసం_Or_దత్తాంశం_Type ')' yP_TICK '(' సమాసం ')'
                        { $$ = new AstCast{$1, $7, VFlag_బిడ్డ_DType{},
                                           new AstRefDType{$1, AstRefDType::Flag_Type_యొక్క_సమాసం{}, $3}}; }
        |       ySIGNED yP_TICK '(' సమాసం ')'            { $$ = new AstSigned{$1, $4}; }
        |       yUNSIGNED yP_TICK '(' సమాసం ')'          { $$ = new AstUnsigned{$1, $4}; }
        |       ySTRING yP_TICK '(' సమాసం ')'            { $$ = new AstCvtPackString{$1, $4}; }
        |       yCONST__ETC yP_TICK '(' సమాసం ')'        { $$ = $4; }  // Not linting const presently
        //                      // Spec only allows primary with addition of a type reference
        //                      // We'll be more general, and later assert LHS was a type.
        |       ~l~సమాసం yP_TICK '(' సమాసం ')'            { $$ = new AstCastParse{$2, $4, $1}; }
        //
        //                      // IEEE: assignment_pattern_expression
        //                      // IEEE: streaming_concatenation
        //                      // See exprOkLvalue
        //
        //                      // IEEE: sequence_method_call
        //                      // Indistinguishable from function_subroutine_call:method_call
        //
        |       '$'                                     { $$ = new AstUnbounded{$<fl>1}; }
        |       yNULL                                   { $$ = new AstConst{$1, AstConst::Null{}}; }
        //                      // IEEE: yTHIS
        //                      // See exprScope
        //
        //----------------------
        //
        //                      // Part of expr that may also be used as lvalue
        |       ~l~సమాసం_Ok_Lvalue                         { $$ = $1; }
        //
        //----------------------
        //
        //                      // IEEE: cond_predicate - here to avoid reduce problems
        //                      // Note expr includes cond_pattern
        |       ~l~సమాసం yP_ANDANDAND ~r~సమాసం            { $$ = new AstConst{$2, AstConst::బిట్టు_False{}};
                                                          BBUNSUP($<fl>2, "Unsupported: &&& expression"); }
        //
        //                      // IEEE: cond_pattern - here to avoid reduce problems
        //                      // "expr yMATCHES pattern"
        //                      // IEEE: pattern - expanded here to avoid conflicts
        |       ~l~సమాసం yMATCHES నమూనా_No_సమాసం          { $$ = new AstMatches{$2, $1, $3}; }
        |       ~l~సమాసం yMATCHES ~r~సమాసం                { $$ = new AstMatches{$2, $1, $3}; }
        //
        //                      // IEEE: expression_or_dist - here to avoid reduce problems
        //                      // "expr yDIST '{' dist_list '}'"
        |       ~l~సమాసం yDIST '{' dist_list '}'         { $$ = new AstDist{$2, $1, $4}; }
        ;

fexpr<కణుపు_Exprp>:                   // For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(సమాసం,{s/~l~/f/g; s/~r~/f/g; s/~f__IGNORE~/__IGNORE/g;})   // {copied}
        ;

//UNSUPpev_expr<nodeExprp>:  // IEEE: event_expression
//UNSUP //                      // for yOR/, see event_expression
//UNSUP //
//UNSUP //                      // IEEE: [ edge_identifier ] expression [ yIFF expression ]
//UNSUP //                      // expr alone see below
//UNSUP         senitemEdge                             { $$ = $1; }
//UNSUP |       ev_expr yIFF expr                       { }
//UNSUP //
//UNSUP //                      // IEEE: sequence_instance [ yIFF expression ]
//UNSUP //                      // seq_inst is in expr, so matches senitem rule above
//UNSUP //
//UNSUP //                      // IEEE: event_expression yOR event_expression
//UNSUP |       ev_expr yOR ev_expr                     { }
//UNSUP //                      // IEEE: event_expression ',' event_expression
//UNSUP //                      // See real event_expression rule
//UNSUP //
//UNSUP //---------------------
//UNSUP //                      // IEEE: expr
//UNSUP |       BISONPRE_COPY(expr,{s/~l~/ev_/g; s/~r~/ev_/g; s/~p~/ev_/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g;})  // {copied}
//UNSUP //
//UNSUP //                      // IEEE: '(' event_expression ')'
//UNSUP //                      // expr:'(' x ')' conflicts with event_expression:'(' event_expression ')'
//UNSUP //                      // so we use a special expression class
//UNSUP |       '(' event_expression ')'                { $<fl>$ = $<fl>1; $$ = "(...)"; }
//UNSUP //                      // IEEE: From normal expr: '(' expr ':' expr ':' expr ')'
//UNSUP //                      // But must avoid conflict
//UNSUP |       '(' event_expression ':' expr ':' expr ')'      { $<fl>$ = $<fl>1; $$ = "(...)"; }
//UNSUP ;

సమాసం_No_Str<కణుపు_Exprp>:               // expression with string removed
                BISONPRE_COPY(సమాసం,{s/~noStr__IGNORE~/Ignore/g;})       // {copied}
        ;

సమాసం_Ok_Lvalue<కణుపు_Exprp>:            // expression that's also OK to use as a variable_lvalue
                ~l~సమాసం_Scope                            { $$ = $1; }
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Replicate(1) required as otherwise "{a}" would not be self-determined
        |       '{' cateList '}'                        { $$ = new AstReplicate{$1, $2, 1}; }
        |       '{' cateList '}' '[' సమాసం ']'           { $$ = new Ast_Sel_బిట్టు{$4, new AstReplicate{$1, $2, 1}, $5}; }
        |       '{' cateList '}' '[' const_సమాసం ':' const_సమాసం ']'
                                                        { $$ = new Ast_Sel_వెలికితీయి{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' సమాసం yP_PLUSCOLON const_సమాసం ']'
                                                        { $$ = new AstSelPlus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' సమాసం yP_MINUSCOLON const_సమాసం ']'
                                                        { $$ = new AstSelMinus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        //                      // IEEE: assignment_pattern_expression
        //                      // IEEE: [ assignment_pattern_expression_type ] == [ ps_type_id /ps_paremeter_id/data_type]
        //                      // We allow more here than the spec requires
        //UNSUP ~l~exprScope assignment_pattern         { UNSUP }
        |       దత్తాంశం_type assignment_నమూనా            { $$ = $2; ఒకవేళ ($2) $2->బిడ్డ_DTypep($1); }
        |       assignment_నమూనా                      { $$ = $1; }
        //
        |       streaming_concatenation                 { $$ = $1; }
        ;

fexprOkLvalue<కణుపు_Exprp>:           // exprOkLValue, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(సమాసం_Ok_Lvalue,{s/~l~/f/g}) // {copied}
        ;

sexprOkLvalue<కణుపు_Exprp>:  // exprOkLValue, For use by sequence_expr
                BISONPRE_COPY(సమాసం_Ok_Lvalue,{s/~l~/s/g}) // {copied}
        ;

pexprOkLvalue<కణుపు_Exprp>:  // exprOkLValue, For use by property_expr
                BISONPRE_COPY(సమాసం_Ok_Lvalue,{s/~l~/p/g}) // {copied}
        ;

//UNSUPev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/ev_/g})       // {copied}
//UNSUP ;

//UNSUPpev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/pev_/g})      // {copied}
//UNSUP ;

fexprLvalue<కణుపు_Exprp>:             // For use as first part of statement (disambiguates <=)
                fexprOkLvalue                           { $<fl>$ = $<fl>1; $$ = $1; }
        ;

సమాసం_Scope<కణుపు_Exprp>:               // scope and variable for use to inside an expression
        //                      // Here we've split method_call_root | implicit_class_handle | class_scope | package_scope
        //                      // from the object being called and let expr's "." deal with resolving it.
        //                      // (note method_call_root was simplified to require a primary in 1800-2009)
        //
        //                      // IEEE: [ implicit_class_handle . | class_scope | package_scope ] hierarchical_identifier select
        //                      // Or method_call_body without parenthesis
        //                      // See also varRefClassBit, which is the non-expr version of most of this
                yTHIS                                   { $$ = new AstParseRef{$<fl>1, "this"}; }
        |       y_D_మూలం                                 { $$ = new AstParseRef{$<fl>1, "$root"}; }
        |       idArrayed                               { $$ = $1; }
        |       packageClassScope idArrayed             { $$ = AstDot::newIfPkg($2->fileline(), $1, $2); }
        |       ~l~సమాసం '.' idArrayed                   { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        //                      // expr below must be a "yTHIS"
        |       ~l~సమాసం '.' ySUPER
                        { AstParseRef* స్థిరరాశి anodep = VN_CAST($1, ParseRef);
                          ఒకవేళ (anodep && anodep->name() == "this") {
                              $$ = new AstParseRef{$<fl>1, "super"};
                              $1->delete_వృక్షం();
                          } లేకపోతే {
                              $$ = $1;  $$->v3error("Syntax error: 'super' must be first name component, or after 'this.'");
                          }
                        }
        //                      // Part of implicit_class_handle
        |       ySUPER                                  { $$ = new AstParseRef{$<fl>1, "super"}; }
        ;

fexprScope<కణుపు_Exprp>:              // exprScope, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(సమాసం_Scope,{s/~l~/f/g})    // {copied}
        ;

sexprScope<కణుపు_Exprp>:  // exprScope, For use by sequence_expr
                BISONPRE_COPY(సమాసం_Scope,{s/~l~/s/g})    // {copied}
        ;

pexprScope<కణుపు_Exprp>:  // exprScope, For use by property_expr
                BISONPRE_COPY(సమాసం_Scope,{s/~l~/p/g})    // {copied}
        ;

//UNSUPev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/ev_/g})  // {copied}
//UNSUP ;

//UNSUPpev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/pev_/g}) // {copied}
//UNSUP ;

// PLI calls exclude "" as integers, they're strings
// For $c("foo","bar") we want "bar" as a string, not a Verilog integer.
సమాసం_Str_పాఠ్యం<nodep>:
                సమాసం_No_Str                               { $$ = $1; }
        |       yaSTRING                                { $$ = new Ast_పాఠ్యం{$<fl>1, GRAMMARP->పాఠ్యం_Quoted($<fl>1, *$1)}; }
        ;

సమాసం_Type_Compare<కణుపు_Exprp>:
                సమాసం                                    { $$ = $1; }
        |       type_referenceBoth                      { $$ = $1; }
        ;

cStrList<nodep>:
                సమాసం_Str_పాఠ్యం                             { $$ = $1; }
        |       సమాసం_Str_పాఠ్యం ',' cStrList                { $$ = $1->కూడు_తదుపరి($3); }
        ;

cateList<కణుపు_Exprp>:
        //                      // Not just 'expr' to prevent conflict via stream_concOrExprOrType
                ప్రవాహం_వ్యక్తీకరణ                       { $$ = $1; }
        |       cateList ',' ప్రవాహం_వ్యక్తీకరణ          { $$ = new AstConcat{$2, $1, $3}; }
        ;

సమాసం_List_E<కణుపు_Exprp>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       సమాసం_List                                { $$ = $1; }
        ;

సమాసం_List<కణుపు_Exprp>:
                సమాసం                                    { $$ = $1; }
        |       సమాసం_List ',' సమాసం                       { $$ = $1->కూడు_తదుపరి($3); }
        ;

// identifier_list for 'with' in inline randomize constraints (IEEE 1800-2023 18.7).
// Only simple identifiers; non-identifier expressions are parse errors.
inlineConstraintIdList<కణుపు_Exprp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        |       inlineConstraintIdList ',' id           { $$ = $1->కూడు_తదుపరి(
                                                             new AstParseRef{$<fl>3, *$3, nullptr, nullptr}); }
        ;

// Optional identifier_list. Empty 'with () {...}' differs from bare 'with {...}'
// via AstWithParse::restricted().
inlineConstraintIdListE<కణుపు_Exprp>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       inlineConstraintIdList                  { $$ = $1; }
        ;

సమాసం_EList_E<nodep>:  // expression list with empty commas allowed
                సమాసం_E                                   { $$ = $1; }
        |       సమాసం_EList_E ',' సమాసం_E                    { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

సమాసం_Disp_List<కణుపు_Exprp>:            // exprList for within $display
                సమాసం                                    { $$ = $1; }
        |       సమాసం_Disp_List ',' సమాసం                   { $$ = $1->కూడు_తదుపరి($3); }
        //                      // ,, creates a space in $display
        |       సమాసం_Disp_List ',' /*empty*/
                        { $$ = $1->కూడు_తదుపరి(new AstConst{$<fl>2, AstConst::VerilogStringLiteral{}, " "}); }
        ;

vrdList<కణుపు_Exprp>:
                idClassSel                              { $$ = $1; }
        |       vrdList ',' idClassSel                  { $$ = $1->కూడు_తదుపరి($3); }
        ;

commasE:
                /*ఖాళీలు*/                             { } /* ignored */
        |       ',' commasE                             { } /* ignored */
        ;

commaVRDListE<కణుపు_Exprp>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       ',' vrdList                             { $$ = $2; }
        ;

వాదనలు_సమాసం_List<కణుపు_Exprp>:        // IEEE: part of list_of_arguments (used where ,, isn't legal)
                సమాసం                                    { $$ = $1; }
        |       వాదనలు_సమాసం_List ',' సమాసం                   { $$ = $1->కూడు_తదుపరి($3); }
        ;

వాదనలు_సమాసం_List_E<argp>:       // IEEE: part of list_of_arguments
                వాదనలు_సమాసం_One_E                            { $$ = $1; }
        |       వాదనలు_సమాసం_List_E ',' వాదనలు_సమాసం_One_E          { $$ = $1->కూడు_తదుపరి($3); }
        ;

//UNSUPpev_argsExprListE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsExprOneE                        { $$ = $1; }
//UNSUP |       pev_argsExprListE ',' pev_argsExprOneE  { $$ = addNextNull($1, $3); }
//UNSUP ;

వాదనలు_సమాసం_One_E<argp>:        // IEEE: part of list_of_arguments
                /*empty*/                               { $$ = new AstArg{CRELINE(), "", nullptr}; }
        |       సమాసం                                    { $$ = new AstArg{$1->fileline(), "", $1}; }
        ;

//UNSUPpev_argsExprOneE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         /*empty*/                               { $$ = nullptr; }       // ,, is legal in list_of_arguments
//UNSUP |       pev_expr                                { $$ = $1; }
//UNSUP ;

వాదనలు_Dotted_List<argp>:      // IEEE: part of list_of_arguments
                వాదనలు_Dotted                              { $$ = $1; }
        |       వాదనలు_Dotted_List ',' వాదనలు_Dotted           { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

//UNSUPpev_argsDottedList<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsDotted                          { $$ = $1; }
//UNSUP |       pev_argsDottedList ',' pev_argsDotted   { $$ = addNextNull($1, $3); }
//UNSUP ;

వాదనలు_Dotted<argp>:          // IEEE: part of list_of_arguments
                '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
        |       '.' idAny '(' సమాసం ')'                  { $$ = new AstArg{$<fl>2, *$2, $4}; }
        ;

//UNSUPpev_argsDotted<argp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
//UNSUP |       '.' idAny '(' pev_expr ')'              { $$ = new AstArg{$<fl>2, *$2, $4}; }
//UNSUP ;

streaming_concatenation<కణుపు_Streamp>: // ==IEEE: streaming_concatenation
        //                      // Need to disambiguate {<< expr-{ ... expr-} stream_concat }
        //                      // From                 {<< stream-{ ... stream-} }
        //                      // Likewise simple_type's idScoped from constExpr's idScope
        //                      // Thus we allow always any two operations.  Sorry
        //                      // IEEE: "'{' yP_SL/R             stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R simple_type stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R constExpr   stream_concatenation '}'"
                '{' yP_SLEFT  ప్రవాహం_concatenation '}'
                        { $$ = new Ast_ప్రవాహం_L{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SRIGHT ప్రవాహం_concatenation '}'
                        { $$ = new Ast_ప్రవాహం_R{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SLEFT  ప్రవాహం_వ్యక్తీకరణ_Or_దత్తాంశం_Type ప్రవాహం_concatenation '}'
                        { $$ = new Ast_ప్రవాహం_L{$2, $4, new Ast_Attr_యొక్క{$1, VAttrType::DIM_బిట్లు_OR_సంఖ్య, $3}}; }
        |       '{' yP_SRIGHT ప్రవాహం_వ్యక్తీకరణ_Or_దత్తాంశం_Type ప్రవాహం_concatenation '}'
                        { $$ = new Ast_ప్రవాహం_R{$2, $4, new Ast_Attr_యొక్క{$1, VAttrType::DIM_బిట్లు_OR_సంఖ్య, $3}}; }
        ;

ప్రవాహం_concatenation<కణుపు_Exprp>:    // ==IEEE: stream_concatenation
        //                      // '{' { stream_expression } '}'
                '{' cateList '}'                        { $$ = $2; }
        ;

ప్రవాహం_వ్యక్తీకరణ<కణుపు_Exprp>:   // ==IEEE: stream_expression
        //                      // IEEE: array_range_expression expanded below
                సమాసం                                    { $$ = $1; }
        |       సమాసం yWITH__BRA '[' సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       సమాసం yWITH__BRA '[' సమాసం ':' సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       సమాసం yWITH__BRA '[' సమాసం yP_PLUSCOLON  సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       సమాసం yWITH__BRA '[' సమాసం yP_MINUSCOLON సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        ;

ప్రవాహం_వ్యక్తీకరణ_Or_దత్తాంశం_Type<nodep>:     // IEEE: from streaming_concatenation
                సమాసం_Or_దత్తాంశం_Type                          { $$ = $1; }
        |       సమాసం yWITH__BRA '[' సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       సమాసం yWITH__BRA '[' సమాసం ':' సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       సమాసం yWITH__BRA '[' సమాసం yP_PLUSCOLON  సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       సమాసం yWITH__BRA '[' సమాసం yP_MINUSCOLON సమాసం ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        ;

//************************************************
// Let

letId<letp>:  // IEEE: pert of let_declaration
                idAny/*let_identifieer*/
                        { $<fl>$ = $<fl>1;
                          $$ = new AstLet{$<fl>$, *$1}; }
        ;

let_declaration<letp>:  // IEEE: let_declaration
                yLET letId '=' సమాసం ';'
                        { $$ = $2;
                          $$->కూడు_Stmtsp(new Ast_Stmt_సమాసం{$1, $4}); }
        |       yLET letId '(' let_పోర్టు_list_E ')' '=' సమాసం ';'
                        { $$ = $2;
                          $$->కూడు_Stmtsp(new Ast_Stmt_సమాసం{$1, $7});
                          $$->కూడు_Stmtsp($4); }
        ;

let_పోర్టు_list_E<nodep>:   // IEEE: [ let_port_list ]
                /*empty*/                               { $$ = nullptr; }
        |       /*emptyStart*/
        /*mid*/         { VARRESET_LIST(VAR); VARIO(INOUT); }
        /*cont*/  let_పోర్టు_list                         { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

let_పోర్టు_list<nodep>:   // IEEE: let_port_list
                let_పోర్టు_అంశం                           { $$ = $1; }
        |       let_పోర్టు_list ',' let_పోర్టు_అంశం         { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

let_పోర్టు_అంశం<varp>:  // IEEE: let_port_Item
        //                      // IEEE: Expanded let_formal_type
                yUNTYPED idAny/*formal_port_identifier*/ variable_dimensionListE సమాసం_Eq_E
                        { $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_బిడ్డ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          ఒకవేళ ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       దత్తాంశం_type idAny/*formal_port_identifier*/ variable_dimensionListE సమాసం_Eq_E
                        { BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_బిడ్డ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          ఒకవేళ ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       implicit_typeE id/*formal_port_identifier*/ variable_dimensionListE సమాసం_Eq_E
                        { ఒకవేళ ($1) BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_బిడ్డ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          ఒకవేళ ($4) $$->valuep($4);
                          PINNUMINC(); }
        ;

//************************************************
// Gate declarations

gateDecl<nodep>:
                yBUF    driveStrengthE delay_controlE gateBufList ';'     { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yBUFIF0 driveStrengthE delay_controlE gateBufif0List ';'  { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yBUFIF1 driveStrengthE delay_controlE gateBufif1List ';'  { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yNOT    driveStrengthE delay_controlE gateNotList ';'     { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yNOTIF0 driveStrengthE delay_controlE gateNotif0List ';'  { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yNOTIF1 driveStrengthE delay_controlE gateNotif1List ';'  { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yAND    driveStrengthE delay_controlE gateAndList ';'     { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yNAND   driveStrengthE delay_controlE gateNandList ';'    { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yOR     driveStrengthE delay_controlE gateOrList ';'      { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yNOR    driveStrengthE delay_controlE gateNorList ';'     { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yXOR    driveStrengthE delay_controlE gateXorList ';'     { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yXNOR   driveStrengthE delay_controlE gateXnorList ';'    { $$ = $4; STRENGTH_LIST($4, $2); DELAY_LIST($4, $3); }
        |       yPULLDOWN pulldown_strengthE delay_controlE gatePulldownList ';'   { $$ = $4; DELAY_LIST($4, $3); }
        |       yPULLUP   pullup_strengthE   delay_controlE gatePullupList ';'     { $$ = $4; DELAY_LIST($4, $3); }
        |       yNMOS     delay_controlE gateBufif1List ';'     { $$ = $3; DELAY_LIST($3, $2); }
        |       yPMOS     delay_controlE gateBufif0List ';'     { $$ = $3; DELAY_LIST($3, $2); }
        //
        |       yTRAN delay_controlE gateUnsupList ';'          { $$ = $3; GATEUNSUP($3, "tran"); }
        |       yRCMOS delay_controlE gateUnsupList ';'         { $$ = $3; GATEUNSUP($3, "rcmos"); }
        |       yCMOS delay_controlE gateUnsupList ';'          { $$ = $3; GATEUNSUP($3, "cmos"); }
        |       yRNMOS delay_controlE gateUnsupList ';'         { $$ = $3; GATEUNSUP($3, "rmos"); }
        |       yRPMOS delay_controlE gateUnsupList ';'         { $$ = $3; GATEUNSUP($3, "pmos"); }
        |       yRTRAN delay_controlE gateUnsupList ';'         { $$ = $3; GATEUNSUP($3, "rtran"); }
        |       yRTRANIF0 delay_controlE gateUnsupList ';'      { $$ = $3; GATEUNSUP($3, "rtranif0"); }
        |       yRTRANIF1 delay_controlE gateUnsupList ';'      { $$ = $3; GATEUNSUP($3, "rtranif1"); }
        |       yTRANIF0 delay_controlE gateUnsupList ';'       { $$ = $3; GATEUNSUP($3, "tranif0"); }
        |       yTRANIF1 delay_controlE gateUnsupList ';'       { $$ = $3; GATEUNSUP($3, "tranif1"); }
        ;

gateBufList<nodep>:
                gateBuf                                 { $$ = $1; }
        |       gateBufList ',' gateBuf                 { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateBufif0List<nodep>:
                gateBufif0                              { $$ = $1; }
        |       gateBufif0List ',' gateBufif0           { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateBufif1List<nodep>:
                gateBufif1                              { $$ = $1; }
        |       gateBufif1List ',' gateBufif1           { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateNotList<nodep>:
                gateNot                                 { $$ = $1; }
        |       gateNotList ',' gateNot                 { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateNotif0List<nodep>:
                gateNotif0                              { $$ = $1; }
        |       gateNotif0List ',' gateNotif0           { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateNotif1List<nodep>:
                gateNotif1                              { $$ = $1; }
        |       gateNotif1List ',' gateNotif1           { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateAndList<nodep>:
                gateAnd                                 { $$ = $1; }
        |       gateAndList ',' gateAnd                 { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateNandList<nodep>:
                gateNand                                { $$ = $1; }
        |       gateNandList ',' gateNand               { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateOrList<nodep>:
                gateOr                                  { $$ = $1; }
        |       gateOrList ',' gateOr                   { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateNorList<nodep>:
                gateNor                                 { $$ = $1; }
        |       gateNorList ',' gateNor                 { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateXorList<nodep>:
                gateXor                                 { $$ = $1; }
        |       gateXorList ',' gateXor                 { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateXnorList<nodep>:
                gateXnor                                { $$ = $1; }
        |       gateXnorList ',' gateXnor               { $$ = $1->కూడు_తదుపరి($3); }
        ;
gatePullupList<nodep>:
                gatePullup                              { $$ = $1; }
        |       gatePullupList ',' gatePullup           { $$ = $1->కూడు_తదుపరి($3); }
        ;
gatePulldownList<nodep>:
                gatePulldown                            { $$ = $1; }
        |       gatePulldownList ',' gatePulldown       { $$ = $1->కూడు_తదుపరి($3); }
        ;
gateUnsupList<nodep>:
                gateUnsup                               { $$ = $1; }
        |       gateUnsupList ',' gateUnsup             { $$ = $1->కూడు_తదుపరి($3); }
        ;

gateRangeE<nodep>:
                instRangeListE                          { $$ = $1; GATERANGE(GRAMMARP->scrubRange($1)); }
        ;

gateBuf<nodep>:
                gateFront variable_lvalue ',' సమాసం_List ')'
                        { Ast_కణుపు_సమాసం* inp = $4;
                          అయితే (inp->nextp()) inp = VN_AS(inp->nextp(), కణుపు_సమాసం);
                          $$ = new AstImplicit{$<fl>1, inp->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = GRAMMARP->createGatePin(inp->clone_వృక్షం(false));
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          కొరకు (Ast_కణుపు_సమాసం* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), కణుపు_సమాసం)) {
                              Ast_కణుపు_సమాసం* స్థిరరాశి pinRhsp = GRAMMARP->createGatePin(inp->clone_వృక్షం(false));
                              Ast_కేటాయించు_W* స్థిరరాశి pinAssp = new Ast_కేటాయించు_W{$<fl>1, outp->clone_వృక్షం(false), pinRhsp};
                              $$->కూడు_తదుపరి(new AstAlways{pinAssp});
                          }
                          DEL($1); DEL($4); }
        ;
gateNot<nodep>:
                gateFront variable_lvalue ',' సమాసం_List ')'
                        { Ast_కణుపు_సమాసం* inp = $4;
                          అయితే (inp->nextp()) inp = VN_AS(inp->nextp(), కణుపు_సమాసం);
                          $$ = new AstImplicit{$<fl>1, inp->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_వృక్షం(false))};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          కొరకు (Ast_కణుపు_సమాసం* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), కణుపు_సమాసం)) {
                              Ast_కణుపు_సమాసం* స్థిరరాశి pinRhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_వృక్షం(false))};
                              Ast_కేటాయించు_W* స్థిరరాశి pinAssp = new Ast_కేటాయించు_W{$<fl>1, outp->clone_వృక్షం(false), pinRhsp};
                              $$->కూడు_తదుపరి(new AstAlways{pinAssp});
                          }
                          DEL($1, $4); }
        ;
gateBufif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_సమాసం ',' gate_Pin_సమాసం ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_వృక్షం(false)};
                          $<implicitp>$->కూడు_Exprsp($4->clone_వృక్షం(false));
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, $4};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateBufif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_సమాసం ',' gate_Pin_సమాసం ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_వృక్షం(false)};
                          $<implicitp>$->కూడు_Exprsp($4->clone_వృక్షం(false));
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstBufIf1{$<fl>1, $6, $4};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_సమాసం ',' gate_Pin_సమాసం ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_వృక్షం(false)};
                          $<implicitp>$->కూడు_Exprsp($4->clone_వృక్షం(false));
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, new AstNot{$<fl>1, $4}};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_సమాసం ',' gate_Pin_సమాసం ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_వృక్షం(false)};
                          $<implicitp>$->కూడు_Exprsp($4->clone_వృక్షం(false));
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstBufIf1{$<fl>1, $6, new AstNot{$<fl>1, $4}};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateAnd<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = $4;
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateNand<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstNot{$<fl>1, $4};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateOr<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = $4;
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateNor<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstNot{$<fl>1, $4};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateXor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = $4;
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gateXnor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_వృక్షం(false)};
                          Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = new AstNot{$<fl>1, $4};
                          Ast_కేటాయించు_W* స్థిరరాశి ap = new Ast_కేటాయించు_W{$<fl>1, $2, rhsp};
                          $$->కూడు_తదుపరి(new AstAlways{ap});
                          DEL($1); }
        ;
gatePullup<nodep>:
                gateFront variable_lvalue ')'           { $$ = new AstPull{$<fl>1, $2, true}; DEL($1); }
        ;
gatePulldown<nodep>:
                gateFront variable_lvalue ')'           { $$ = new AstPull{$<fl>1, $2, false}; DEL($1); }
        ;
gateUnsup<nodep>:
                gateFront gateUnsupPinList ')'          { $$ = new AstImplicit{$<fl>1, $2}; DEL($1); }
        ;

gateFront<nodep>:
                id/*gate*/ gateRangeE '('               { $$ = $2; $<fl>$ = $<fl>1; }
        |       gateRangeE '('                          { $$ = $1; $<fl>$ = $<fl>2; }
        ;

gateAndPinList<కణుపు_Exprp>:
                gate_Pin_సమాసం                             { $$ = $1; }
        |       gateAndPinList ',' gate_Pin_సమాసం          { $$ = new AstAnd{$2, $1, $3}; }
        ;
gateOrPinList<కణుపు_Exprp>:
                gate_Pin_సమాసం                             { $$ = $1; }
        |       gateOrPinList ',' gate_Pin_సమాసం           { $$ = new AstOr{$2, $1, $3}; }
        ;
gateXorPinList<కణుపు_Exprp>:
                gate_Pin_సమాసం                             { $$ = $1; }
        |       gateXorPinList ',' gate_Pin_సమాసం          { $$ = new AstXor{$2, $1, $3}; }
        ;
gateUnsupPinList<కణుపు_Exprp>:
                gate_Pin_సమాసం                             { $$ = $1; }
        |       gateUnsupPinList ',' gate_Pin_సమాసం        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

gate_Pin_సమాసం<కణుపు_Exprp>:
                సమాసం                                    { $$ = GRAMMARP->createGatePin($1); }
        ;

strength0<strength>:
                ySUPPLY0                                { $$ = VStrength::SUPPLY; }
        |       ySTRONG0                                { $$ = VStrength::STRONG; }
        |       yPULL0                                  { $$ = VStrength::PULL; }
        |       yWEAK0                                  { $$ = VStrength::WEAK; }
        ;

strength1<strength>:
                ySUPPLY1                                { $$ = VStrength::SUPPLY; }
        |       ySTRONG1                                { $$ = VStrength::STRONG; }
        |       yPULL1                                  { $$ = VStrength::PULL; }
        |       yWEAK1                                  { $$ = VStrength::WEAK; }
        ;

driveStrengthE<strengthSpecp>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       driveStrength                           { $$ = $1; }
        ;

driveStrength<strengthSpecp>:
                yP_PAR__STRENGTH strength0 ',' strength1 ')' { $$ = new AstStrengthSpec{$1, $2, $4}; }
        |       yP_PAR__STRENGTH strength1 ',' strength0 ')' { $$ = new AstStrengthSpec{$1, $4, $2}; }
        |       yP_PAR__STRENGTH strength0 ',' yHIGHZ1 ')' { $$ = nullptr; BBUNSUP($<fl>4, "Unsupported: highz strength"); }
        |       yP_PAR__STRENGTH strength1 ',' yHIGHZ0 ')' { $$ = nullptr; BBUNSUP($<fl>4, "Unsupported: highz strength"); }
        |       yP_PAR__STRENGTH yHIGHZ0 ',' strength1 ')' { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: highz strength"); }
        |       yP_PAR__STRENGTH yHIGHZ1 ',' strength0 ')' { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: highz strength"); }
        ;

pulldown_strengthE<nodep>:  // IEEE: [ pulldown_strength ]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       pulldown_strength                       { $$ = $1; }
        ;

pulldown_strength<nodep>:  // IEEE: pulldown_strength
                yP_PAR__STRENGTH strength0 ',' strength1 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength1 ',' strength0 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength0 ')'                { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        ;

pullup_strengthE<nodep>:  // IEEE: [ pullup_strength ]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       pullup_strength                         { $$ = $1; }
        ;

pullup_strength<nodep>:  // IEEE: pullup_strength
                yP_PAR__STRENGTH strength0 ',' strength1 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pullup strength"); }
        |       yP_PAR__STRENGTH strength1 ',' strength0 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pullup strength"); }
        |       yP_PAR__STRENGTH strength1 ')'                { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pullup strength"); }
        ;

//************************************************
// Tables

combinational_body<nodep>:      // IEEE: combinational_body + sequential_body
                y_పట్టిక పట్టిక_నమోదు_List yENDTABLE         { $$ = new Ast_Udp_పట్టిక{$1, $2}; }
        ;

పట్టిక_నమోదు_List<udp_పట్టిక_Linep>:  // IEEE: { combinational_entry + sequential_entry }
                పట్టిక_పంక్తి                              { $$ = $1; }
        |       పట్టిక_నమోదు_List పట్టిక_పంక్తి               { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

పట్టిక_పంక్తి<udp_పట్టిక_Linep>:
                పట్టిక_ఆదానం_List ya_పట్టిక_LRSEP tablelVal ya_పట్టిక_LINEEND
                        { $$ = new Ast_Udp_పట్టిక_పంక్తి{Ast_Udp_పట్టిక_పంక్తి::UdpCombo{}, $<fl>1, $1, $3}; }
        |       పట్టిక_ఆదానం_List ya_పట్టిక_LRSEP tablelVal ya_పట్టిక_LRSEP tablelVal ya_పట్టిక_LINEEND
                        { $$ = new Ast_Udp_పట్టిక_పంక్తి{Ast_Udp_పట్టిక_పంక్తి::UdpSequential{}, $<fl>1, $1, $3, $5}; }
        ;

పట్టిక_ఆదానం_List<udp_పట్టిక_పంక్తి_Valp>:
                tablelVal                            { $$ = $1; }
        |       పట్టిక_ఆదానం_List tablelVal             { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

tablelVal<udp_పట్టిక_పంక్తి_Valp>:
                ya_పట్టిక_క్షేత్రం                          { $$ = new Ast_Udp_పట్టిక_పంక్తి_Val{$<fl>1, *$1}; }
        |       '(' ya_పట్టిక_క్షేత్రం ya_పట్టిక_క్షేత్రం ')'    { $$ = new Ast_Udp_పట్టిక_పంక్తి_Val{$<fl>2, *$2 + *$3}; }
        ;

//************************************************
// Specify

specify_ఖండం<nodep>:               // ==IEEE: specify_block
                specifyFront specify_అంశం_List yENDSPECIFY   { $$ = $2; }
        |       specifyFront yENDSPECIFY                { $$ = nullptr; }
        ;

specifyFront:  // IEEE: specify_block front
                ySPECIFY                                { GRAMMARP->m_specifyignWarned = false; }
        ;

specify_అంశం_List<nodep>:            // IEEE: { specify_item }
                specify_అంశం                            { $$ = $1; }
        |       specify_అంశం_List specify_అంశం           { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

specify_అంశం<nodep>:                // ==IEEE: specify_item
                specparam_declaration                   { $$ = $1; }
        |       system_timing_check                     { $$ = $1; }
        |       junk_గమ్యం_Semi_List ';'
                        { $$ = nullptr;
                          ఒకవేళ (!GRAMMARP->m_specifyignWarned) {
                              GRAMMARP->m_specifyignWarned = true;
                              $1->v3warn(SPECIFYIGN, "Ignoring unsupported: specify block construct");
                          }
                        }
        ;

specparam_declaration<nodep>:       // ==IEEE: specparam_declaration
                specparam_declarationFront list_యొక్క_specparam_assignments ';'
                        { $$ = $2; }
        ;

specparam_declarationFront:     // IEEE: part of specparam_declaration
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                ySPECPARAM
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_కణుపు_DType* dtp = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE};
                          VARDTYPE(dtp); }
        |       ySPECPARAM packed_dimension
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_కణుపు_DType* స్థిరరాశి dtp = GRAMMARP->కూడు_Range(
                                    new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT}, $2, true);
                          VARDTYPE(dtp); }
        ;

list_యొక్క_specparam_assignments<varp>:  // ==IEEE: list_of_specparam_assignments
                specparam_assignment                    { $$ = $1; }
        |       list_యొక్క_specparam_assignments ',' specparam_assignment  { $$ = $1->కూడు_తదుపరి($3); }
        ;

specparam_assignment<varp>:     // ==IEEE: specparam_assignment
                idNotPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          ఒకవేళ ($4) $$->valuep($4); }
        //                      // IEEE: pulse_control_specparam
        //                      // LRM grammar requires '(' as the first token after assignment,
        //                      // but IEEE provides an example in 30.7.1 where it is omitted.
        //                      // Other simulators also support it.
        |       idPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          ఒకవేళ ($4) $$->valuep($4); }
        |       idPathpulse sigAttrListE '=' '(' minTypMax ',' minTypMax ')'
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          ఒకవేళ ($5) $$->valuep($5);
                          DEL($7); }
        ;

system_timing_check<nodep>:         // ==IEEE: system_timing_check
                setuphold_timing_check                      { $$ = $1; }
        ;

setuphold_timing_check<nodep>:      // ==IEEE: $setuphold_timing_check
                yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_హద్దు ',' timing_check_హద్దు ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_హద్దు ',' timing_check_హద్దు ',' idAnyE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_హద్దు ',' timing_check_హద్దు ',' idAnyE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_హద్దు ',' timing_check_హద్దు ',' idAnyE ',' minTypMaxE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13, $15); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_హద్దు ',' timing_check_హద్దు ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17}; DEL($7, $9, $13, $15);  }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_హద్దు ',' timing_check_హద్దు ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17, $19}; DEL($7, $9, $13, $15); }
        ;

timing_check_event<కణుపు_Exprp>:      // ==IEEE: $timing_check_event
                terminal_identifier                                                         { $$ = $1; }
        |       yPOSEDGE terminal_identifier                                                { $$ = $2; }
        |       yNEGEDGE terminal_identifier                                                { $$ = $2; }
        |       y_అంచు terminal_identifier                                                   { $$ = $2; }
        |       y_అంచు '[' అంచు_descriptor_list ']' terminal_identifier                      { $$ = $5; }
        |       terminal_identifier yP_ANDANDAND సమాసం                                       { $$ = $1; DEL($3); }
        |       yPOSEDGE terminal_identifier yP_ANDANDAND సమాసం                              { $$ = $2; DEL($4); }
        |       yNEGEDGE terminal_identifier yP_ANDANDAND సమాసం                              { $$ = $2; DEL($4); }
        |       y_అంచు terminal_identifier yP_ANDANDAND సమాసం                                 { $$ = $2; DEL($4); }
        |       y_అంచు '[' అంచు_descriptor_list ']' terminal_identifier yP_ANDANDAND సమాసం    { $$ = $5; DEL($7); }
        ;

అంచు_descriptor_list:
                yaEDGEDESC                            {  }
        |       అంచు_descriptor_list ',' yaEDGEDESC   {  }
        ;

timing_check_హద్దు<కణుపు_Exprp>:
                సమాసం                      { $$ = $1; }
        |       సమాసం ':' సమాసం ':' సమాసం    { $$ = $3; DEL($1, $5); }
        ;

delayed_referenceE<కణుపు_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       terminal_identifier                     { $$ = $1; }
        ;

terminal_identifier<కణుపు_Exprp>:
                idArrayed     { $$ = $1; }
        ;

idAnyE<strp>:
                /*empty*/                               { $$ = nullptr; }
        |       idAny                                   { $$ = $1; }
        ;

junk_గమ్యం_Semi_List<fl>:
                junk_గమ్యం_Semi                              { $$ = CRELINE(); }
        |       junk_గమ్యం_Semi_List junk_గమ్యం_Semi               { $$ = CRELINE(); }
        ;

junk_గమ్యం_Semi:
                BISONPRE_NOT(';',yD_SETUPHOLD,yENDMODULE,yENDSPECIFY,ySPECPARAM)        { }
        |       error                                   { }  // LCOV_EXCL_LINE
        ;

//************************************************
// IDs

id<strp>:
                yaID__ETC                               { $$ = $1; $<fl>$ = $<fl>1; }
        |       yaID__PATHPULSE                         { $$ = $1; $<fl>$ = $<fl>1; }
        |       idRandomize                             { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idAny<strp>:  // Any kind of identifier
                yaID__ETC                               { $$ = $1; $<fl>$ = $<fl>1; }
        |       yaID__PATHPULSE                         { $$ = $1; $<fl>$ = $<fl>1; }
        |       yaID__aINST                             { $$ = $1; $<fl>$ = $<fl>1; }
        |       yaID__aTYPE                             { $$ = $1; $<fl>$ = $<fl>1; }
        |       idRandomize                             { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idNotPathpulse<strp>:  // Id excluding specparam PATHPULSE$, IEEE: part of specparam_assignment
                yaID__ETC                               { $$ = $1; $<fl>$ = $<fl>1; }
        |       idRandomize                             { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idPathpulse<strp>:  // Id for specparam PATHPULSE$, IEEE: part of pulse_control_specparam
                yaID__PATHPULSE                         { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idAnyAsParseRef<parseRefp>:  // Any kind of identifier as a ParseRef
                idAny
                        { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;


idInst<strp>:                   // IEEE: instance_identifier or similar with another id then '('
        //                      // See V3ParseImp::tokenPipeScanIdInst
        //                      //   [^': '@' '.'] yaID/*module_id*/ [ '#' '('...')' ] yaID/*name_of_instance*/ [ '['...']' ] '(' ...
        //                      //   [^':' @' '.'] yaID/*module_id*/ [ '#' id|etc ] yaID/*name_of_instance*/ [ '['...']' ] '(' ...
                yaID__aINST                             { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idType<strp>:                   // IEEE: class_identifier or other type identifier
        //                      // Used where reference is needed
                yaID__aTYPE                             { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idInstType<strp>:               // type_identifier for functions which have a following id then '('
                yaID__aINST                             { $$ = $1; $<fl>$ = $<fl>1; }
        |       yaID__aTYPE                             { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idCC<strp>:                     // IEEE: class/package then ::
                                // lexer matches this:  yaID_LEX [ '#' '(' ... ')' ] yP_COLONCOLON
                yaID__CC                                { $$ = $1; $<fl>$ = $<fl>1; }
        ;

idRandomize<strp>:              // Keyword as an identifier
                yRANDOMIZE                              { స్థిర string s = "randomize"; $$ = &s; $<fl>$ = $<fl>1; }
        ;

idSVKwd<strp>:                  // Warn about non-forward compatible Verilog 2001 code
        //                      // yBIT, yBYTE won't work here as causes conflicts
                yDO
                        { స్థిర string s = "do"   ; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        |       yFINAL
                        { స్థిర string s = "final"; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        ;

variable_lvalue<కణుపు_Exprp>:     // IEEE: variable_lvalue or net_lvalue
        //                      // Note many variable_lvalue's must use exprOkLvalue when arbitrary expressions may also exist
                idClassSel                              { $$ = $1; }
        |       '{' variable_lvalueConcList '}'         { $$ = $2; }
        //                      // IEEE: [ assignment_pattern_expression_type ] assignment_pattern_variable_lvalue
        //                      // We allow more assignment_pattern_expression_types then strictly required
        //UNSUP data_type  yP_TICKBRA variable_lvalueList '}'   { UNSUP }
        //UNSUP idClassSel yP_TICKBRA variable_lvalueList '}'   { UNSUP }
        //UNSUP /**/       yP_TICKBRA variable_lvalueList '}'   { UNSUP }
        |       streaming_concatenation                 { $$ = $1; }
        ;

variable_lvalueConcList<కణుపు_Exprp>:  // IEEE: part of variable_lvalue: '{' variable_lvalue { ',' variable_lvalue } '}'
                variable_lvalue                                 { $$ = $1; }
        |       variable_lvalueConcList ',' variable_lvalue     { $$ = new AstConcat{$2, $1, $3}; }
        ;

//UNSUPvariable_lvalueList<nodeExprp>:  // IEEE: part of variable_lvalue: variable_lvalue { ',' variable_lvalue }
//UNSUP         variable_lvalue                         { $$ = $1; }
//UNSUP |       variable_lvalueList ',' variable_lvalue { $$ = addNextNull($1, $3); }
//UNSUP ;

idClass<కణుపు_Exprp>:             // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
                idDotted                                { $$ = $1; }
        //                      // IEEE: [ implicit_class_handle . | package_scope ] hierarchical_variable_identifier select
        |       yTHIS '.' idDotted
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "this"}, $3}; }
        |       ySUPER '.' idDotted
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "super"}, $3}; }
        |       yTHIS '.' ySUPER '.' idDotted
                        { $$ = new AstDot{$4, false, new AstParseRef{$<fl>3, "super"}, $5}; }
        //                      // Expanded: package_scope idDottedSel
        |       packageClassScope idDotted              { $$ = new AstDot{$<fl>2, true, $1, $2}; }
        ;

idClassSel<కణుపు_Exprp>:          // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
                idDottedSel                             { $$ = $1; }
        //                      // IEEE: [ implicit_class_handle . | package_scope ] hierarchical_variable_identifier select
        |       yTHIS '.' idDottedSel
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "this"}, $3}; }
        |       ySUPER '.' idDottedSel
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "super"}, $3}; }
        |       yTHIS '.' ySUPER '.' idDottedSel
                        { $$ = new AstDot{$4, false, new AstParseRef{$<fl>3, "super"}, $5}; }
        //                      // Expanded: package_scope idDottedSel
        |       packageClassScope idDottedSel           { $$ = new AstDot{$<fl>2, true, $1, $2}; }
        ;

idClassSelForeach<foreachHeaderp>:
                idDottedForeach                         { $$ = $1; }
        //                      // IEEE: [ implicit_class_handle . | package_scope ] hierarchical_variable_identifier select
        |       yTHIS '.' idDottedForeach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "this"}, $3->fromp()->unlink_Fr_వెనుక()}); $$ = $3; }
        |       ySUPER '.' idDottedForeach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "super"}, $3->fromp()->unlink_Fr_వెనుక()}); $$ = $3; }
        |       yTHIS '.' ySUPER '.' idDottedForeach
                        { $5->fromp(new AstDot{$4, false, new AstParseRef{$<fl>3, "super"}, $5->fromp()->unlink_Fr_వెనుక()}); $$ = $5; }
        //                      // Expanded: package_scope idForeach
        |       packageClassScope idDottedForeach
                       { $2->fromp(new AstDot{$<fl>2, true, $1, $2->fromp()->unlink_Fr_వెనుక()}); $$ = $2; }
        ;


// Dotted identifier for typedef - must have at least one '.'
// First component is plain id or id with array index, subsequent components can have arrays
idDottedOrArrayed<కణుపు_Exprp>:
                id '.' idArrayed
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, *$1, nullptr, nullptr}, $3}; }
        |       idDottedOrArrayed '.' idArrayed
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

idDotted<కణుపు_Exprp>:
                y_D_మూలం '.' id_Dotted_మరిన్ని
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_మరిన్ని                            { $$ = $1; }
        ;

idDottedSel<కణుపు_Exprp>:
                y_D_మూలం '.' id_Dotted_Sel_మరిన్ని
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_Sel_మరిన్ని                         { $$ = $1; }
        ;

idDottedForeach<foreachHeaderp>:
                y_D_మూలం '.' id_Dotted_మరిన్ని_Foreach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3->fromp()->unlink_Fr_వెనుక()}); $$ = $3; }
        |       id_Dotted_మరిన్ని_Foreach                     { $$ = $1; }
        ;

id_Dotted_మరిన్ని<కణుపు_Exprp>:
                parse_Ref_పునాది                            { $$ = $1; }
        |       id_Dotted_మరిన్ని '.' parse_Ref_పునాది           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_Sel_మరిన్ని<కణుపు_Exprp>:
                idArrayed                               { $$ = $1; }
        |       id_Dotted_Sel_మరిన్ని '.' idArrayed           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_మరిన్ని_Foreach<foreachHeaderp>:
                idArrayedForeach                        { $$ = $1; }
        |       id_Dotted_Sel_మరిన్ని '.' idArrayedForeach
                        { $3->fromp(new AstDot{$2, false, $1, $3->fromp()->unlink_Fr_వెనుక()}); $$ = $3; }
        ;

// Single component of dotted path, maybe [#].
// Due to lookahead constraints, we can't know if [:] or [+:] are valid (last dotted part),
// we'll assume so and cleanup later.
// id below includes:
//       enum_identifier
idArrayed<కణుపు_Exprp>:               // IEEE: id + select
                id
                        { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        //                      // IEEE: id + part_select_range/constant_part_select_range
        |       idArrayed '[' సమాసం ']'                          { $$ = new Ast_Sel_బిట్టు{$2, $1, $3}; }  // Or AstArraySel, don't know yet.
        |       idArrayed '[' const_సమాసం ':' const_సమాసం ']'       { $$ = new Ast_Sel_వెలికితీయి{$2, $1, $3, $5}; }
        //                      // IEEE: id + indexed_range/constant_indexed_range
        |       idArrayed '[' సమాసం yP_PLUSCOLON  const_సమాసం ']'  { $$ = new AstSelPlus{$2, $1, $3, $5}; }
        |       idArrayed '[' సమాసం yP_MINUSCOLON const_సమాసం ']'  { $$ = new AstSelMinus{$2, $1, $3, $5}; }
        ;

idArrayedForeach<foreachHeaderp>:    // IEEE: id + select (under foreach expression)
                parse_Ref_పునాది  // Malformed, but accept for better error reporting
                        { $$ = new Ast_Foreach_శీర్షిక{$<fl>1, $1, nullptr}; }
        |       idArrayed '[' సమాసం ']'
                        { $$ = new Ast_Foreach_శీర్షిక{$2, $1, $3}; }
        |       idArrayed '[' ']'
                        { $$ = new Ast_Foreach_శీర్షిక{$2, $1, new Ast_ఖాళీలు{$3}}; }
        |       idArrayed '[' parse_Ref_పునాది ',' loop_variables ']'
                        { $$ = new Ast_Foreach_శీర్షిక{$2, $1, కూడు_తదుపరి_Null(static_cast<Ast_కణుపు*>($3), $5)}; }
        |       idArrayed '[' ',' loop_variables ']'
                        { $$ = new Ast_Foreach_శీర్షిక{$2, $1, కూడు_తదుపరి_Null(static_cast<Ast_కణుపు*>(new Ast_ఖాళీలు{$3}), $4)}; }
        ;

// ParseRef without any dots or vectorizaion
parse_Ref_పునాది<parseRefp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

// yaSTRING shouldn't be used directly, instead via an abstraction below
str<strp>:                      // yaSTRING but with \{escapes} need decoded
                yaSTRING                                { $$ = PARSEP->newString(GRAMMARP->unquoteString($<fl>1, *$1)); }
        ;

strAsInt<కణుపు_Exprp>:
                yaSTRING
                        { // Numeric context, so IEEE 1800-2017 11.10.3 "" is a "\000"
                          $$ = new AstConst{$<fl>1, AstConst::VerilogStringLiteral{}, GRAMMARP->unquoteString($<fl>1, *$1)}; }
        ;

strAsIntIgnore<కణుపు_Exprp>:          // strAsInt, but never matches for when expr shouldn't parse strings
                yaSTRING__IGNORE                        { $$ = nullptr; yyerror("Impossible token"); }
        ;

ప్రారంభం_Label_E<strp>:
                /*ఖాళీలు*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        ;

చివర_Label_E<strp>:
                /*ఖాళీలు*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        |       ':' yNEW__ETC                           { స్థిర string n = "new"; $$ = &n; $<fl>$ = $<fl>2; }
        ;

//************************************************
// Clocking

clocking_declaration<nodep>:            // IEEE: clocking_declaration
                yCLOCKING idAny clocking_event ';' clocking_అంశం_List_E yENDCLOCKING చివర_Label_E
                        { $$ = new AstClocking{$<fl>2, *$2, new Ast_Sen_వృక్షం{$<fl>2, $3}, $5, false, false}; }
        |       yDEFAULT yCLOCKING clocking_event ';' clocking_అంశం_List_E yENDCLOCKING చివర_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_వృక్షం{$<fl>2, $3}, $5, true, false}; }
        |       yDEFAULT yCLOCKING idAny clocking_event ';' clocking_అంశం_List_E yENDCLOCKING చివర_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_వృక్షం{$<fl>3, $4}, $6, true, false}; }
        |       yGLOBAL__CLOCKING yCLOCKING clocking_event ';' clocking_అంశం_List_E yENDCLOCKING చివర_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_వృక్షం{$<fl>2, $3}, $5, false, true}; }
        |       yGLOBAL__CLOCKING yCLOCKING idAny clocking_event ';' clocking_అంశం_List_E yENDCLOCKING చివర_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_వృక్షం{$<fl>3, $4}, $6, false, true}; }
        ;

clocking_eventE<senItemp>:      // IEEE: optional clocking_event
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       clocking_event                          { $$ = $1; }
        ;

clocking_event<senItemp>:       // IEEE: clocking_event
        //                      // IEEE: '@' ps_identifier
        //                      // IEEE: '@' hierarchical_identifier
        //UNSUP: '@' idClassSel/*ps_identifier*/
                '@' id
                        { $$ = new Ast_Sen_అంశం{$<fl>2, VEdgeType::ET_మారింది,
                                              new AstParseRef{$<fl>2, *$2, nullptr, nullptr}}; }
        |       '@' '(' event_వ్యక్తీకరణ ')'            { $$ = $3; }
        ;

clocking_అంశం_List_E<nodep>:
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       clocking_అంశం_List                       { $$ = $1; }
        ;

clocking_అంశం_List<nodep>:  // IEEE: [ clocking_item ]
                clocking_అంశం                           { $$ = $1; }
        |       clocking_అంశం_List clocking_అంశం         { ఒకవేళ ($1) $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

clocking_అంశం<nodep>:   // IEEE: clocking_item
                yDEFAULT y_ఆదానం clocking_skew ';'       { $$ = new Ast_Clocking_అంశం{$<fl>1, VDirection::ఆదానం, $3, nullptr}; }
        |       yDEFAULT yOUTPUT clocking_skew ';'      { $$ = new Ast_Clocking_అంశం{$<fl>1, VDirection::OUTPUT, $3, nullptr}; }
        |       yDEFAULT y_ఆదానం clocking_skew yOUTPUT clocking_skew ';'
                        { $$ = new Ast_Clocking_అంశం{$<fl>1, VDirection::ఆదానం, $3, nullptr};
                          $$->కూడు_తదుపరి(new Ast_Clocking_అంశం{$<fl>4, VDirection::OUTPUT, $5, nullptr}); }
        |       y_ఆదానం clocking_skewE list_యొక్క_clocking_decl_కేటాయించు ';'
                        { $$ = GRAMMARP->make_Clocking_అంశం_List($<fl>1, VDirection::ఆదానం, $2, $3); }
        |       yOUTPUT clocking_skewE list_యొక్క_clocking_decl_కేటాయించు ';'
                        { $$ = GRAMMARP->make_Clocking_అంశం_List($<fl>1, VDirection::OUTPUT, $2, $3); }
        |       y_ఆదానం clocking_skewE yOUTPUT clocking_skewE list_యొక్క_clocking_decl_కేటాయించు ';'
                        { $$ = GRAMMARP->make_Clocking_అంశం_List($<fl>1, VDirection::ఆదానం, $2, $5->clone_వృక్షం(true));
                          $$->కూడు_తదుపరి(GRAMMARP->make_Clocking_అంశం_List($<fl>3, VDirection::OUTPUT, $4, $5)); }
        |       yINOUT list_యొక్క_clocking_decl_కేటాయించు ';'
                        { $$ = GRAMMARP->make_Clocking_అంశం_List($<fl>1, VDirection::ఆదానం, nullptr, $2->clone_వృక్షం(true));
                          $$->కూడు_తదుపరి(GRAMMARP->make_Clocking_అంశం_List($<fl>1, VDirection::OUTPUT, nullptr, $2)); }
        |       assertion_అంశం_declaration
                        { $$ = $1;
                          కొరకు (Ast_కణుపు* nodep = $1; nodep; nodep = nodep->nextp()) {
                              ఒకవేళ (!VN_IS(nodep, Sequence)) {
                                  $$ = nullptr;
                                  BBUNSUP(nodep, "Unsupported: assertion items in clocking blocks");
                                  DEL($1);
                                  ఆపు;
                              }
                          }
                        }
        ;

list_యొక్క_clocking_decl_కేటాయించు<nodep>:  // IEEE: list_of_clocking_decl_assign
                clocking_decl_కేటాయించు                    { $$ = $1; }
        |       list_యొక్క_clocking_decl_కేటాయించు ',' clocking_decl_కేటాయించు
                        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

clocking_decl_కేటాయించు<nodep>:    // IEEE: clocking_decl_assign
                idAnyAsParseRef/*new-signal_identifier*/ సమాసం_Eq_E
                        { AstParseRef* స్థిరరాశి refp = $1;
                          $$ = refp;
                          ఒకవేళ ($2) $$ = new Ast_కేటాయించు{$<fl>2, refp, $2}; }
        ;

clocking_skewE<కణుపు_Exprp>:          // IEEE: [clocking_skew]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       clocking_skew                           { $$ = $1; }
        ;

clocking_skew<కణుపు_Exprp>:           // IEEE: clocking_skew
                delay_control                           { $$ = $1->lhsp()->unlink_Fr_వెనుక(); $1->delete_వృక్షం(); }
        |      yPOSEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      yNEGEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      y_అంచు delay_controlE                     { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        ;

చక్రం_delay<delayp>:  // IEEE: cycle_delay
               yP_POUNDPOUND yaINTNUM
                        { $$ = new AstDelay{$<fl>1, new AstConst{$<fl>2, *$2}, true}; }
        |      yP_POUNDPOUND idAnyAsParseRef
                        { $$ = new AstDelay{$<fl>1, $2, true}; }
        |      yP_POUNDPOUND '(' సమాసం ')'
                        { $$ = new AstDelay{$<fl>1, $3, true}; }
        ;

//************************************************
// Asserts

assertion_అంశం_declaration<nodep>:  // ==IEEE: assertion_item_declaration
                property_declaration                    { $$ = $1; }
        |       sequence_declaration                    { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

assertion_అంశం<nodep>:          // ==IEEE: assertion_item
                concurrent_assertion_అంశం               { $$ = $1; }
        |       deferred_తక్షణ_assertion_అంశం
                        { $$ = $1 ? new AstAlways{$1->fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr, $1} : nullptr; }
        ;

deferred_తక్షణ_assertion_అంశం<nodep>:       // ==IEEE: deferred_immediate_assertion_item
                deferred_తక్షణ_assertion_statement  { $$ = $1; }
        |       id/*block_identifier*/ ':' deferred_తక్షణ_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        ;

procedural_assertion_statement<కణుపు_Stmtp>:  // ==IEEE: procedural_assertion_statement
                concurrent_assertion_statement          { $$ = $1; }
        |       తక్షణ_assertion_statement           { $$ = $1; }
        //                      // IEEE: checker_instantiation
        //                      // Unlike modules, checkers are the only "id id (...)" form in statements.
        //UNSUP checker_instantiation                   { $$ = $1; }
        ;

తక్షణ_assertion_statement<కణుపు_Stmtp>:   // ==IEEE: immediate_assertion_statement
                simple_తక్షణ_assertion_statement    { $$ = $1; }
        |       deferred_తక్షణ_assertion_statement  { $$ = $1; }
        ;

simple_తక్షణ_assertion_statement<కణుపు_Stmtp>:    // ==IEEE: simple_immediate_assertion_statement
        //                      // action_block expanded here, for compatibility with AstAssert
                assertOrAssume '(' సమాసం ')' stmt %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $3, $5, nullptr, VAssertType::SIMPLE_తక్షణ, $1}; }
        |       assertOrAssume '(' సమాసం ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, nullptr, $6, VAssertType::SIMPLE_తక్షణ, $1}; }
        |       assertOrAssume '(' సమాసం ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, $5, $7, VAssertType::SIMPLE_తక్షణ, $1}; }
        //                      // IEEE: simple_immediate_cover_statement
        |       yCOVER '(' సమాసం ')' stmt                { $$ = new AstCover{$1, $3, $5, VAssertType::SIMPLE_తక్షణ}; }
        ;

assertOrAssume<assertdirectivetypeen>:
                yASSERT                                 { $$ = VAssertDirectiveType::ASSERT; }
        |       yASSUME                                 { $$ = VAssertDirectiveType::ASSUME; }
        ;

final_zero<asserttypeen>:                     // IEEE: part of deferred_immediate_assertion_statement
                '#' yaINTNUM
                        { ఒకవేళ ($2->isNeqZero()) { $<fl>2->v3error("Deferred assertions must use '#0' (IEEE 1800-2023 16.4)"); }
                          $$ = VAssertType::OBSERVED_DEFERRED_తక్షణ; }
        //                      // 1800-2012:
        |       yFINAL                                                  { $$ = VAssertType::FINAL_DEFERRED_తక్షణ; }
        ;

deferred_తక్షణ_assertion_statement<కణుపు_Stmtp>:  // ==IEEE: deferred_immediate_assertion_statement
        //                      // IEEE: deferred_immediate_assert_statement
                assertOrAssume final_zero '(' సమాసం ')' stmt %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $4, $6, nullptr, $2, $1}; }
        |       assertOrAssume final_zero '(' సమాసం ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, nullptr, $7, $2, $1}; }
        |       assertOrAssume final_zero '(' సమాసం ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, $6, $8, $2, $1}; }
        //                      // IEEE: deferred_immediate_cover_statement
        |       yCOVER final_zero '(' సమాసం ')' stmt     { $$ = new AstCover{$1, $4, $6, $2}; }
        ;

concurrent_assertion_అంశం<nodep>:       // IEEE: concurrent_assertion_item
                concurrent_assertion_statement          { $$ = $1; }
        |       id/*block_identifier*/ ':' concurrent_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        //                      // IEEE: checker_instantiation
        //                      // identical to module_instantiation; see etcInst
        ;

concurrent_assertion_statement<కణుపు_Stmtp>:  // ==IEEE: concurrent_assertion_statement
        //                      // IEEE: assert_property_statement
        //                      // IEEE: assume_property_statement
        //                      // action_block expanded here
                assertOrAssume yPROPERTY '(' property_spec ')' stmt %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $4, $6, nullptr, VAssertType::CONCURRENT, $1}; }
        |       assertOrAssume yPROPERTY '(' property_spec ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, $6, $8, VAssertType::CONCURRENT, $1}; }
        |       assertOrAssume yPROPERTY '(' property_spec ')' yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, nullptr, $7, VAssertType::CONCURRENT, $1}; }
        //                      // IEEE: cover_property_statement
        |       yCOVER yPROPERTY '(' property_spec ')' stmt
                        { $$ = new AstCover{$1, $4, $6, VAssertType::CONCURRENT}; }
        //                      // IEEE: cover_sequence_statement
        //                      // Reuses AstCover + AstPropSpec (same wrapper as
        //                      // cover_property_statement above) and the isCoverSeq
        //                      // flag drives V3AssertNfa to fire stmt per end-of-match
        //                      // (IEEE 1800-2023 16.14.3), not per property success.
        |       yCOVER ySEQUENCE '(' sexpr ')' stmt
                        { AstCover* స్థిరరాశి coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), nullptr, nullptr, $4},
                              $6, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event sexpr ')' stmt
                        { AstCover* స్థిరరాశి coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, nullptr, $5},
                              $7, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event yDISABLE yIFF '(' సమాసం/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* స్థిరరాశి coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, $8, $10},
                              $12, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' yDISABLE yIFF '(' సమాసం/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* స్థిరరాశి coverp = new AstCover{$1,
                              new AstPropSpec{$7->fileline(), nullptr, $7, $9},
                              $11, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        //                      // IEEE: restrict_property_statement
        |       yRESTRICT yPROPERTY '(' property_spec ')' ';'
                        { $$ = new AstRestrict{$1, $4}; }
        ;

property_declaration<కణుపు_FTaskp>:  // ==IEEE: property_declaration
                property_declarationFront property_పోర్టు_list_E ';' property_declarationBody
                        yENDPROPERTY చివర_Label_E
                        { $$ = $1;
                          $$->కూడు_Stmtsp($2);
                          $$->కూడు_Stmtsp($4);
                          GRAMMARP->చివర_Label($<fl>6, $$, $6);
                          GRAMMARP->m_insideProperty = false;
                          GRAMMARP->m_typed_Property_పోర్టు = false; }
        ;

property_declarationFront<కణుపు_FTaskp>:  // IEEE: part of property_declaration
                yPROPERTY idAny/*property_identifier*/
                        { $$ = new AstProperty{$<fl>2, *$2, nullptr};
                          GRAMMARP->m_insideProperty = true; }
        ;

property_పోర్టు_list_E<nodep>:  // IEEE: [ ( [ property_port_list ] ) ]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '(' property_పోర్టు_list ')'              { $$ = $2; }
        ;

property_పోర్టు_list<nodep>:  // ==IEEE: property_port_list
                property_పోర్టు_అంశం                              { $$ = $1; }
        |       property_పోర్టు_list ',' property_పోర్టు_అంశం       { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

property_పోర్టు_అంశం<nodep>:  // IEEE: property_port_item/sequence_port_item
        //                      // Merged in sequence_port_item
        //                      // IEEE: property_lvar_port_direction ::= yINPUT
        //                      // prop IEEE: [ yLOCAL [ yINPUT ] ] property_formal_type
        //                      //           id {variable_dimension} [ '=' property_actual_arg ]
        //                      // seq IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
                property_పోర్టు_అంశం_Front property_పోర్టు_అంశం_Assignment  { $$ = $2; }
        ;

property_పోర్టు_అంశం_Front:  // IEEE: part of property_port_item/sequence_port_item
                property_పోర్టు_అంశం_Dir_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       property_పోర్టు_అంశం_Dir_E దత్తాంశం_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_పోర్టు = true; }
        |       property_పోర్టు_అంశం_Dir_E yVAR దత్తాంశం_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_పోర్టు = true; }
        |       property_పోర్టు_అంశం_Dir_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       property_పోర్టు_అంశం_Dir_E implicit_typeE
                        { VARDTYPE($2); }
        ;

property_పోర్టు_అంశం_Assignment<nodep>:  // IEEE: part of property_port_item/sequence_port_item
                id variable_dimensionListE
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, nullptr); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, $4);
                          BBUNSUP($3, "Unsupported: property variable default value"); }
        ;

property_పోర్టు_అంశం_Dir_E:
                /*ఖాళీలు*/
                        { VARIO(ఆదానం); }
        |       yLOCAL__ETC
                        { VARIO(ఆదానం);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        |       yLOCAL__ETC y_ఆదానం
                        { VARIO(ఆదానం);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        ;

property_declarationBody<nodep>:  // IEEE: part of property_declaration
                assertion_variable_declarationList property_spec
                        { $$ = కూడు_తదుపరి_Null($1, $2); }
        |       assertion_variable_declarationList property_spec ';'
                        { $$ = కూడు_తదుపరి_Null($1, $2); }
        //                      // IEEE-2012: Incorrectly has yCOVER ySEQUENCE then property_spec here.
        //                      // Fixed in IEEE 1800-2017
        |       property_spec                           { $$ = $1; }
        |       property_spec ';'                       { $$ = $1; }
        ;

assertion_variable_declarationList<nodep>:  // IEEE: part of assertion_variable_declaration
                assertion_variable_declaration          { $$ = $1; }
        |       assertion_variable_declarationList assertion_variable_declaration
                        { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

sequence_declaration<కణుపు_FTaskp>:  // ==IEEE: sequence_declaration
                sequence_declarationFront sequence_పోర్టు_list_E ';' sequence_declarationBody
        /*cont*/    yENDSEQUENCE చివర_Label_E
                        { $$ = $1;
                          $$->కూడు_Stmtsp($2);
                          $$->కూడు_Stmtsp($4);
                          GRAMMARP->చివర_Label($<fl>6, $$, $6);
                          // Sequence declarations are allowed; they are inlined by V3AssertPre.
                          // Sequences in unsupported contexts are reported by assertPreAll
                          // after inlining.
                        }
        ;

sequence_declarationFront<కణుపు_FTaskp>:  // IEEE: part of sequence_declaration
                ySEQUENCE idAny/*new_sequence*/
                        { $$ = new AstSequence{$<fl>2, *$2, nullptr}; }
        ;

sequence_పోర్టు_list_E<nodep>:  // IEEE: [ ( [ sequence_port_list ] ) ]
        //                      // IEEE: sequence_lvar_port_direction ::= yINPUT | yINOUT | yOUTPUT
        //                      // IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
        //                      // All this is almost identically the same as a property.
        //                      // Difference is only yINOUT/yOUTPUT (which might be added to 1800-2012)
        //                      // and yPROPERTY.  So save some work.
                property_పోర్టు_list_E                     { $$ = $1; }
        ;

property_formal_typeNoDt<కణుపు_DTypep>:  // IEEE: property_formal_type (w/o implicit)
                sequence_formal_typeNoDt                { $$ = $1; }
        |       yPROPERTY
                        { $$ = nullptr; GRAMMARP->m_typed_Property_పోర్టు = false;
                          BBUNSUP($1, "Unsupported: property argument data type"); }
        ;

sequence_formal_typeNoDt<కణుపు_DTypep>:  // ==IEEE: sequence_formal_type (w/o data_type_or_implicit)
                        // IEEE: data_type_or_implicit
                        // implicit expanded where used
                ySEQUENCE
                        { $$ = nullptr; GRAMMARP->m_typed_Property_పోర్టు = false;
                          BBUNSUP($1, "Unsupported: sequence argument data type"); }
                        // IEEE-2009: yEVENT
                        // already part of data_type.  Removed in 1800-2012.
        |       yUNTYPED
                        { $$ = nullptr; GRAMMARP->m_typed_Property_పోర్టు = false; }
        ;

sequence_declarationBody<nodep>:  // IEEE: part of sequence_declaration
        //                      // 1800-2012 makes ';' optional
                assertion_variable_declarationList sexpr        { $$ = కూడు_తదుపరి_Null($1, $2); }
        |       assertion_variable_declarationList sexpr ';'    { $$ = కూడు_తదుపరి_Null($1, $2); }
        |       sexpr                                   { $$ = $1; }
        |       sexpr ';'                               { $$ = $1; }
        //                      // IEEE: clocking_event sequence_expr (16.7)
        //                      // A leading clocking event on a named sequence body.
        |       '@' '(' event_వ్యక్తీకరణ ')' sexpr      { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' '(' event_వ్యక్తీకరణ ')' sexpr ';'  { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' senitemVar sexpr                    { $$ = new AstSClocked{$1, $2, $3}; }
        |       '@' senitemVar sexpr ';'                { $$ = new AstSClocked{$1, $2, $3}; }
        ;

property_spec<propSpecp>:               // IEEE: property_spec
        //UNSUP: This rule has been super-specialized to what is supported now
        //UNSUP remove below
                '@' '(' senitem ')' yDISABLE yIFF '(' సమాసం ')' property_సమాసం_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($3); $$->disablep($8); }
        |       '@' '(' senitem ')' property_సమాసం_Spec
                        { $$ = $5; $$->fileline($1); $$->sensesp($3); }
        |       '@' senitemVar property_సమాసం_Spec
                        { $$ = $3; $$->fileline($1); $$->sensesp($2); }
        |       yDISABLE yIFF '(' సమాసం ')' '@' '(' senitem ')' property_సమాసం_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($8); $$->disablep($4); }
        //UNSUP remove above
        |       yDISABLE yIFF '(' సమాసం ')' property_సమాసం_Spec
                        { $$ = $6; $$->fileline($4->fileline()); $$->disablep($4); }
        |       property_సమాసం_Spec                       { $$ = $1; }
        ;

property_సమాసం_Spec<propSpecp>:  // A property expression plus explicit weak/strong strength
                pexpr
                        { $$ = new AstPropSpec{$1->fileline(), nullptr, nullptr, $1}; }
        |       ySTRONG '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::STRONG}; }
        |       yWEAK '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::WEAK}; }
        ;

property_సమాసం_Case_If<కణుపు_Exprp>:  // IEEE: part of property_expr for if/case
                yCASE '(' సమాసం/*expression_or_dist*/ ')' property_case_అంశం_List yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, $5); }
        |       yCASE '(' సమాసం/*expression_or_dist*/ ')' yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, nullptr); }
        |       yIF '(' సమాసం/*expression_or_dist*/ ')' pexpr  %prec prIF
                        { $$ = new AstImplication{$1, $3, $5, true}; }
        |       yIF '(' సమాసం/*expression_or_dist*/ ')' pexpr yELSE pexpr
                        { Ast_కణుపు_సమాసం* స్థిరరాశి elseCondp = new AstLogNot{$1, $3->clone_వృక్షం_శుద్ధ(false)};
                          $$ = new AstSAnd{$1, new AstImplication{$1, $3, $5, true},
                                           new AstImplication{$1, elseCondp, $7, true},
                                           /*propertyControl=*/true}; }
        ;

property_case_అంశం_List<caseItemp>:  // IEEE: {property_case_item}
                property_case_అంశం                      { $$ = $1; }
        |       property_case_అంశం_List property_case_అంశం   { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

property_case_అంశం<caseItemp>:  // ==IEEE: property_case_item
        //                      // IEEE: expression_or_dist { ',' expression_or_dist } ':' property_expr
        //                      // IEEE 1800-2012 changed from property_statement to property_expr
        //                      // IEEE 1800-2017 changed to require the semicolon
                caseCondList ':' pexpr ';'              { $$ = new Ast_Case_అంశం{$2, $1, $3}; }
        |       yDEFAULT pexpr ';'                      { $$ = new Ast_Case_అంశం{$1, nullptr, $2}; }
        |       yDEFAULT ':' pexpr ';'                  { $$ = new Ast_Case_అంశం{$1, nullptr, $3}; }
        ;

//UNSUPpev_expr<nodep>:  // IEEE: property_actual_arg | expr
//UNSUP //                      //       which expands to pexpr | event_expression
//UNSUP //                      // Used in port and function calls, when we can't know yet if something
//UNSUP //                      // is a function/sequence/property or instance/checker pin.
//UNSUP //
//UNSUP //                      // '(' pev_expr ')'
//UNSUP //                      // Already in pexpr
//UNSUP //                      // IEEE: event_expression ',' event_expression
//UNSUP //                      // ','s are legal in event_expressions, but parens required to avoid conflict with port-sep-,
//UNSUP //                      // IEEE: event_expression yOR event_expression
//UNSUP //                      // Already in pexpr - needs removal there
//UNSUP //                      // IEEE: event_expression yIFF expr
//UNSUP //                      // Already in pexpr - needs removal there
//UNSUP //
//UNSUP         senitemEdge                             { $$ = $1; }
//UNSUP //
//UNSUP //============= pexpr rules copied for pev_expr
//UNSUP |       BISONPRE_COPY_ONCE(pexpr,{s/~o~p/pev_/g; })     // {copied}
//UNSUP //
//UNSUP //============= sexpr rules copied for pev_expr
//UNSUP |       BISONPRE_COPY_ONCE(sexpr,{s/~p~s/pev_/g; })     // {copied}
//UNSUP //
//UNSUP //============= expr rules copied for pev_expr
//UNSUP |       BISONPRE_COPY_ONCE(expr,{s/~l~/pev_/g; s/~p~/pev_/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })    // {copied}
//UNSUP ;

pexpr<కణుపు_Exprp>:  // IEEE: property_expr  (The name pexpr is important as regexps just add an "p" to expr.)
        //
        //                      // IEEE: sequence_expr
        //                      // Expanded below
        //
        //                      // IEEE: '(' pexpr ')'
        //                      // Expanded below
        //
                yNOT pexpr
                        { $$ = new AstLogNot{$1, $2, /*fromProperty=*/true}; }
        //                      // IEEE: pexpr yOR pexpr
        //                      // IEEE: pexpr yAND pexpr
        //                      // Under ~p~sexpr and/or ~p~sexpr
        //
        //                      // IEEE: "sequence_expr yP_ORMINUSGT pexpr"
        //                      // Instead we use pexpr to prevent conflicts
        |       ~o~pexpr yP_ORMINUSGT pexpr             { $$ = new AstImplication{$2, $1, $3, true}; }
        |       ~o~pexpr yP_OREQGT pexpr                { $$ = new AstImplication{$2, $1, $3, false}; }
        //
        //                      // IEEE-2009: property_statement
        //                      // IEEE-2012: yIF and yCASE
        |       property_సమాసం_Case_If                     { $$ = $1; }
        //
        //                      // IEEE: "sequence_expr yP_POUNDMINUSPD pexpr" (followed-by #-#/#=#)
        //                      // Reuses AstImplication with m_isFollowedBy to carry non-vacuous-fail polarity
        |       ~o~pexpr/*sexpr*/ yP_POUNDMINUSPD pexpr
                        { $$ = new AstImplication{$2, $1, $3, true, true}; }
        |       ~o~pexpr/*sexpr*/ yP_POUNDEQPD pexpr
                        { $$ = new AstImplication{$2, $1, $3, false, true}; }
        |       yNEXTTIME pexpr
                        { $$ = $2; BBUNSUP($1, "Unsupported: nexttime (in property expression)"); }
        |       yS_NEXTTIME pexpr
                        { $$ = $2; BBUNSUP($1, "Unsupported: s_nexttime (in property expression)"); }
        |       yNEXTTIME '[' const_సమాసం ']' pexpr %prec yNEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: nexttime[] (in property expression)"); DEL($3); }
        |       yS_NEXTTIME '[' const_సమాసం ']' pexpr %prec yS_NEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: s_nexttime[] (in property expression)"); DEL($3); }
        |       yALWAYS pexpr  %prec prALWAYS
                        { $$ = $2; }
        |       yALWAYS '[' const_సమాసం ':' const_సమాసం ']' pexpr  %prec prALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, false}; }
        |       yS_ALWAYS '[' const_సమాసం ':' const_సమాసం ']' pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, true}; }
        |       yS_ALWAYS pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $2, new AstUnbounded{$1}, new AstUnbounded{$1}, true}; }
        |       yS_EVENTUALLY pexpr  %prec prS_EVENTUALLY
                        {
                            $$ = new AstSEventually{$1, $2};
                            PARSEP->import_If_లోపల_Std($1, "process", true);
                        }
        |       yS_EVENTUALLY anyrange pexpr  %prec prS_EVENTUALLY
                        { $$ = $3; BBUNSUP($1, "Unsupported: s_eventually[] (in property expression)"); DEL($2); }
        |       yEVENTUALLY anyrange pexpr  %prec prEVENTUALLY
                        { $$ = $3; BBUNSUP($1, "Unsupported: eventually[] (in property expression)"); DEL($2); }
        |       ~o~pexpr yUNTIL pexpr
                        { $$ = new AstUntil{$2, $1, $3, false, false}; }
        |       ~o~pexpr yS_UNTIL pexpr
                        { $$ = new AstUntil{$2, $1, $3, true, false}; }
        |       ~o~pexpr yUNTIL_WITH pexpr
                        { $$ = new AstUntil{$2, $1, $3, false, true}; }
        |       ~o~pexpr yS_UNTIL_WITH pexpr
                        { $$ = new AstUntil{$2, $1, $3, true, true}; }
        |       ~o~pexpr y_సూచిస్తుంది pexpr
                        { $$ = new AstLogOr{$2, new AstLogNot{$2, $1}, $3}; }
        //                      // yIFF also used by event_expression
        |       ~o~pexpr yIFF pexpr
                        { $$ = new AstLogEq{$2, $1, $3}; }
        |       yACCEPT_ON '(' సమాసం/*expression_or_dist*/ ')' pexpr  %prec prACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_రకం::ACCEPT_ON, $3, $5}; }
        |       yREJECT_ON '(' సమాసం/*expression_or_dist*/ ')' pexpr  %prec prREJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_రకం::REJECT_ON, $3, $5}; }
        |       ySYNC_ACCEPT_ON '(' సమాసం/*expression_or_dist*/ ')' pexpr %prec prSYNC_ACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_రకం::SYNC_ACCEPT_ON, $3, $5}; }
        |       ySYNC_REJECT_ON '(' సమాసం/*expression_or_dist*/ ')' pexpr %prec prSYNC_REJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_రకం::SYNC_REJECT_ON, $3, $5}; }
        //
        //                      // IEEE: "property_instance"
        //                      // Looks just like a function/method call
        //
        //                      // Note "clocking_event pexpr" overlaps
        //                      // property_statement_spec: clocking_event property_statement
        //
        //                      // Include property_specDisable to match property_spec rule
        //UNSUP clocking_event ~p~sexpr %prec prSEQ_CLOCKING
        //UNSUP         { $$ = $2; BBUNSUP($2, "Unsupported: clocking event (in sequence expression)"); DEL($1); }
        //
        //============= sexpr rules copied for property_expr
        |       BISONPRE_COPY_ONCE(sexpr,{s/~p~s/p/g; })        // {copied}
        //
        //============= expr rules copied for property_expr
        |       BISONPRE_COPY_ONCE(సమాసం,{s/~l~/p/g; s/~p~/p/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

sexpr<కణుపు_Exprp>:  // ==IEEE: sequence_expr  (The name sexpr is important as regexps just add an "s" to expr.)
        //                      // ********* RULES COPIED IN sequence_exprProp
        //                      // For precedence, see IEEE 17.7.1
        //
        //                      // IEEE: "cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // IEEE: "sequence_expr cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // Both rules basically mean we can repeat sequences, so make it simpler:
                చక్రం_delay_range ~p~sexpr  %prec yP_POUNDPOUND
                        { $$ = new AstSExpr{$<fl>1, $1, $2}; }
        |       ~p~sexpr చక్రం_delay_range sexpr %prec prPOUNDPOUND_MULTI
                        { $$ = new AstSExpr{$<fl>2, $1, $2, $3}; }
        //
        //                      // IEEE: expression_or_dist [ boolean_abbrev ]
        //                      // Note expression_or_dist includes "expr"!
        //                      // sexpr/*sexpression_or_dist*/  --- Hardcoded below
        //                      // Consecutive repetition (IEEE 1800-2023 16.9.2)
        //                      // [*N] exact count
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_సమాసం ']'
                        { $$ = new AstSConsRep{$<fl>2, $1, $3}; }
        //                      // [*N:M] bounded range or [*N:$] unbounded range
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_సమాసం ':' const_సమాసం ']'
                        {
                            ఒకవేళ (VN_IS($5, Unbounded)) {
                                DEL($5);
                                $$ = new AstSConsRep{$<fl>2, $1, $3, nullptr, true};
                            } లేకపోతే {
                                $$ = new AstSConsRep{$<fl>2, $1, $3, $5, false};
                            }
                        }
        //                      // [+] = [*1:$]
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAPLUSKET
                        { $$ = new AstSConsRep{$<fl>2, $1,
                              new AstConst{$<fl>2, 1u}, nullptr, true}; }
        //                      // [*] = [*0:$]
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR ']'
                        { $$ = new AstSConsRep{$<fl>2, $1,
                              new AstConst{$<fl>2, 0u}, nullptr, true}; }
        //                      // IEEE: goto_repetition (single count form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_సమాసం ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3}; }
        //                      // IEEE: goto_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_సమాసం ':' const_సమాసం ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3, $5}; }
        //                      // IEEE: nonconsecutive_repetition (single count form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_సమాసం ']'
                        { $$ = new AstSNonConsRep{$<fl>2, $1, $3}; }
        //                      // IEEE: nonconsecutive_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_సమాసం ':' const_సమాసం ']'
                        { $$ = new AstSNonConsRep{$<fl>2, $1, $3, $5}; }
        //                      // All boolean_abbrev forms are now handled above:
        //                      // [*N], [*N:M], [+], [*] via AstSConsRep
        //                      // [->N], [->M:N] via AstSGotoRep
        //                      // [=N], [=M:N] via AstSNonConsRep
        //
        //                      // IEEE: "sequence_instance [ sequence_abbrev ]"
        //                      // version without sequence_abbrev looks just like normal function call
        //                      // version w/sequence_abbrev matches above;
        //                      // expression_or_dist:expr:func boolean_abbrev:sequence_abbrev
        //
        //                      // IEEE: '(' expression_or_dist {',' sequence_match_item } ')' [ boolean_abbrev ]
        //                      // IEEE: '(' sexpr {',' sequence_match_item } ')' [ sequence_abbrev ]
        //                      // As sequence_expr includes expression_or_dist, and boolean_abbrev includes sequence_abbrev:
        //                      // '(' sequence_expr {',' sequence_match_item } ')' [ boolean_abbrev ]
        //                      // "'(' sexpr ')' boolean_abbrev" matches "[sexpr:'(' expr ')'] boolean_abbrev" so we can drop it
        |       '(' ~p~sexpr ')'                        { $$ = $2; }
        |       '(' ~p~sexpr ',' sequence_సరిపోలిక_అంశం_List ')'
                        { $$ = new Ast_సమాసం_Stmt{$3, $4, $2}; }
        //
        //                      // AND/OR are between pexprs OR sexprs (IEEE 1800-2023 16.9.2, 16.11.2)
        //                      // For expression-level (boolean) operands, AstLogAnd/AstLogOr is correct.
        //                      // For temporal sequence operands (containing ##), the downstream
        //                      // V3Width "Implication with sequence expression" check will catch them.
        |       ~p~sexpr yAND ~p~sexpr
                        { $$ = new AstSAnd{$2, $1, $3}; }
        |       ~p~sexpr yOR ~p~sexpr
                        { $$ = new AstSOr{$2, $1, $3}; }
        //                      // Intersect always has an sexpr rhs
        |       ~p~sexpr yINTERSECT sexpr
                        { $$ = new AstSIntersect{$2, $1, $3}; }
        //
        |       y_మొదటి_సరిపోలిక '(' sexpr ')'
                        { $$ = $3; }  // IEEE 16.9.8: no-op in deterministic DFA model
        |       y_మొదటి_సరిపోలిక '(' sexpr ',' sequence_సరిపోలిక_అంశం_List ')'
                        { $$ = $3; BBUNSUP($1, "Unsupported: first_match with sequence_match_items"); DEL($5); }
        |       ~p~sexpr/*sexpression_or_dist*/ yTHROUGHOUT sexpr
                        { $$ = new AstSThroughout{$2, $1, $3}; }
        //                      // Below pexpr's are really sequence_expr, but avoid conflict
        //                      // IEEE: sexpr yWITHIN sexpr
        |       ~p~sexpr yWITHIN sexpr
                        { $$ = new AstSWithin{$2, $1, $3}; }
        //                      // Note concurrent_assertion had duplicate rule for below
        //UNSUP clocking_event ~p~sexpr %prec prSEQ_CLOCKING    { }
        //
        //============= expr rules copied for sequence_expr
        |       BISONPRE_COPY_ONCE(సమాసం,{s/~l~/s/g; s/~p~/s/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

చక్రం_delay_range<delayp>:  // IEEE: ==cycle_delay_range
        //                      // These three terms in 1800-2005 ONLY
                yP_POUNDPOUND intnumAsConst
                        { $$ = new AstDelay{$1, $2, true}; }
        |       yP_POUNDPOUND idAny
                        { $$ = new AstDelay{$1, new AstParseRef{$<fl>2, *$2}, true}; }
        |       yP_POUNDPOUND '(' const_సమాసం ')'
                        { $$ = new AstDelay{$1, $3, true}; }
        //                      // In 1800-2009 ONLY:
        //                      // IEEE: yP_POUNDPOUND constant_primary
        //                      // UNSUP: This causes a big grammar ambiguity
        //                      // as ()'s mismatch between primary and the following statement
        //                      // the sv-ac committee has been asked to clarify  (Mantis 1901)
        |       yP_POUNDPOUND '[' const_సమాసం ':' const_సమాసం ']'
                        { $$ = new AstDelay{$1, $3, true};
                                                                                                        $$->rhsp($5); }
        |       yP_POUNDPOUND yP_BRASTAR ']'
                        { $$ = new AstDelay{$1, new AstConst{$1, 0}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        |       yP_POUNDPOUND yP_BRAPLUSKET
                        { $$ = new AstDelay{$1, new AstConst{$1, 1}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        ;

sequence_సరిపోలిక_అంశం_List<nodep>:  // IEEE: [sequence_match_item] part of sequence_expr
                sequence_సరిపోలిక_అంశం                     { $$ = $1; }
        |       sequence_సరిపోలిక_అంశం_List ',' sequence_సరిపోలిక_అంశం { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

sequence_సరిపోలిక_అంశం<nodep>:  // ==IEEE: sequence_match_item
        //                      // IEEE says: operator_assignment
        //                      // IEEE says: inc_or_dec_expression
        //                      // IEEE says: subroutine_call
        //                      // This is the same list as...
                for_అడుగు_assignment                     { $$ = $1; }
        ;

//      boolean_abbrev -- all forms now handled directly in sexpr rule:
//                      // IEEE: consecutive_repetition -- [*N], [*N:M], [+], [*] via AstSConsRep
//                      // IEEE: goto_repetition -- [->N], [->M:N] via AstSGotoRep
//                      // IEEE: nonconsecutive_repetition -- [=N], [=M:N] via AstSNonConsRep

//************************************************
// Covergroup

covergroup_declaration<nodep>:  // ==IEEE: covergroup_declaration
                 yCOVERGROUP idAny cg_పోర్టు_List_E coverage_eventE ';'
        /*cont*/    coverage_spec_or_optionListE
        /*cont*/ yENDGROUP చివర_Label_E
                        { Ast_Sen_వృక్షం* clockp = nullptr;
                          Ast_కణుపు* sampleArgsp = nullptr;
                          ఒకవేళ ($4) {
                              ఒకవేళ (VN_IS($4, Sen_అంశం)) {
                                  clockp = new Ast_Sen_వృక్షం{$<fl>1, VN_AS($4, Sen_అంశం)};
                              } లేకపోతే {
                                  sampleArgsp = $4;
                              }
                          }
                          $$ = new AstCovergroup{$<fl>1, *$2, static_cast<AstVar*>($3),
                                                 static_cast<AstVar*>(sampleArgsp), $6, clockp};
                          // Every covergroup has option/type_option members (added by V3LinkParse)
                          // referencing std:: types, so mark std as needed at parse time.
                          v3Global.సమితి_ఉపయోగిస్తుంది_Std_Package();
                          GRAMMARP->చివర_Label($<fl>8, $$, $8); }
        |        yCOVERGROUP yEXTENDS idAny ';'
        /*cont*/     coverage_spec_or_optionListE
        /*cont*/ yENDGROUP చివర_Label_E
                        { $$ = nullptr;
                          BBUNSUP($1, "Unsupported: covergroup inheritance (extends)");
                          DEL($5); }
        ;

cg_పోర్టు_List_E<nodep>:
                /*empty*/                               { $$ = nullptr; }
        |       '(' tf_పోర్టు_list_E ')'                   { $$ = $2; }
        ;

cgexpr<కణుపు_Exprp>:  // IEEE-2012: covergroup_expression, before that just expression
                సమాసం                                    { $$ = $1; }
        ;

coverage_spec_or_optionListE<nodep>:  // IEEE: [{coverage_spec_or_option}]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       coverage_spec_or_optionList             { $$ = $1; }
        ;

coverage_spec_or_optionList<nodep>:  // IEEE: {coverage_spec_or_option}
                coverage_spec_or_option                 { $$ = $1; }
        |       coverage_spec_or_optionList coverage_spec_or_option     { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

coverage_spec_or_option<nodep>:  // ==IEEE: coverage_spec_or_option
        //                      // IEEE: coverage_spec
                cover_బిందువు                             { $$ = $1; }
        |       cover_cross                             { $$ = $1; }
        |       coverage_option ';'                     { $$ = $1; }
        |       error                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

coverage_option<nodep>:  // ==IEEE: coverage_option
        //                      // option/type_option aren't really keywords
                id/*yOPTION | yTYPE_OPTION*/ '.' idAny/*member_identifier*/ '=' సమాసం
                        { $$ = nullptr;
                          ఒకవేళ (*$1 == "option" || *$1 == "type_option") {
                              స్థిరరాశి bool typeOpt = (*$1 == "type_option");
                              VCoverOptionType optType = VCoverOptionType::UNKNOWN;
                              bool valid = true;
                              ఒకవేళ (!typeOpt) {
                                  // IEEE 1800-2023 Table 19-1 option.* names and ignored legacy names
                                  ఒకవేళ      (*$3 == "at_least")                optType = VCoverOptionType::AT_కనిష్ఠ;
                                  లేకపోతే ఒకవేళ (*$3 == "auto_bin_max")            optType = VCoverOptionType::AUTO_BIN_MAX;
                                  లేకపోతే ఒకవేళ (*$3 == "comment")                 optType = VCoverOptionType::COMMENT;
                                  లేకపోతే ఒకవేళ (*$3 == "cross_auto_bin_max")      optType = VCoverOptionType::CROSS_AUTO_BIN_MAX;
                                  లేకపోతే ఒకవేళ (*$3 == "cross_num_print_missing") optType = VCoverOptionType::CROSS_అంకె_PRINT_MISSING;
                                  లేకపోతే ఒకవేళ (*$3 == "cross_retain_auto_bins")  optType = VCoverOptionType::CROSS_RETAIN_AUTO_BINS;
                                  లేకపోతే ఒకవేళ (*$3 == "detect_overlap")          optType = VCoverOptionType::DETECT_OVERLAP;
                                  లేకపోతే ఒకవేళ (*$3 == "get_inst_coverage")       optType = VCoverOptionType::GET_INST_COVERAGE;
                                  లేకపోతే ఒకవేళ (*$3 == "goal")                    optType = VCoverOptionType::GOAL;
                                  లేకపోతే ఒకవేళ (*$3 == "name")                    optType = VCoverOptionType::NAME;
                                  లేకపోతే ఒకవేళ (*$3 == "per_instance")            optType = VCoverOptionType::చొప్పున_INSTANCE;
                                  లేకపోతే ఒకవేళ (*$3 == "weight")                  optType = VCoverOptionType::బరువు;
                                  లేకపోతే {
                                      $<fl>1->v3error("Unknown coverage option name 'option."
                                                      << *$3 << "'"
                                                      << "; not a valid option (IEEE 1800-2023 Table 19-1)");
                                      valid = false;
                                  }
                              } లేకపోతే {
                                  // IEEE 1800-2023 Table 19-3: type_option.* names
                                  ఒకవేళ      (*$3 == "comment")          optType = VCoverOptionType::COMMENT;
                                  లేకపోతే ఒకవేళ (*$3 == "distribute_first") optType = VCoverOptionType::DISTRIBUTE_మొదటి;
                                  లేకపోతే ఒకవేళ (*$3 == "goal")             optType = VCoverOptionType::GOAL;
                                  లేకపోతే ఒకవేళ (*$3 == "merge_instances")  optType = VCoverOptionType::విలీనం_INSTANCES;
                                  లేకపోతే ఒకవేళ (*$3 == "real_interval")    optType = VCoverOptionType::REAL_INTERVAL;
                                  లేకపోతే ఒకవేళ (*$3 == "strobe")           optType = VCoverOptionType::STROBE;
                                  లేకపోతే ఒకవేళ (*$3 == "weight")           optType = VCoverOptionType::బరువు;
                                  లేకపోతే {
                                      // Specific message for option.* names used under type_option.*
                                      ఒకవేళ (*$3 == "at_least" || *$3 == "auto_bin_max"
                                          || *$3 == "cross_num_print_missing" || *$3 == "cross_retain_auto_bins"
                                          || *$3 == "detect_overlap" || *$3 == "get_inst_coverage"
                                          || *$3 == "name" || *$3 == "per_instance") {
                                          $<fl>1->v3error("'type_option." << *$3
                                              << "' is not valid; use 'option." << *$3 << "' instead");
                                      } లేకపోతే {
                                          $<fl>1->v3error("Unknown coverage type option name 'type_option."
                                              << *$3 << "'"
                                              << "; not a valid type option (IEEE 1800-2023 Table 19-3)");
                                      }
                                      valid = false;
                                  }
                              }
                              ఒకవేళ (valid) {
                                  $$ = new Ast_Cg_Option_కేటాయించు{$<fl>1, typeOpt, optType, *$3, $5};
                              } లేకపోతే {
                                  DEL($5);
                              }
                          } లేకపోతే {
                              $<fl>1->v3error("Syntax error; expected 'option' or 'type_option': '" << *$1 << "'");
                              DEL($5);
                          } }
        ;

cover_బిందువు<nodep>:  // ==IEEE: cover_point
        //              // [ [ data_type_or_implicit ] cover_point_identifier ':' ] yCOVERPOINT
                yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>1, "", $2, $3, $4}; }
        //                      // IEEE-2012: class_scope before an ID
        |       id/*cover_point_id*/ ':' yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>3, *$1, $4, $5, $6}; }
        //                      // data_type_or_implicit expansion
        |       దత్తాంశం_type id/*cover_point_id*/ ':' yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7};
                          DEL($1); }
        |       yVAR దత్తాంశం_type id/*cover_point_id*/ ':' yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       yVAR implicit_typeE id/*cover_point_id*/ ':' yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signingE rangeList id/*cover_point_id*/ ':' yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signing id/*cover_point_id*/ ':' yCOVERPOINT సమాసం iffE bins_or_ఖాళీలు
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7}; }
        //                      // IEEE-2012:
        |       bins_or_ఖాళీలు                           { $$ = $1; }
        ;

iffE<కణుపు_Exprp>:  // IEEE: part of cover_point, others
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       yIFF '(' సమాసం ')'
                        { $$ = $3; /* Keep iff condition for coverpoint */ }
        ;

bins_or_ఖాళీలు<nodep>:  // ==IEEE: bins_or_empty
                '{' bins_or_optionsList '}'             { $$ = $2; }
        |       '{' '}'                                 { $$ = nullptr; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' bins_or_optionsList error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

bins_or_optionsList<nodep>:  // IEEE: { bins_or_options ';' }
                bins_or_options ';'                     { $$ = $1; }
        |       bins_or_optionsList bins_or_options ';' { $$ = కూడు_తదుపరి_Null($1, $2); }
        //
        |       bins_or_optionsList error ';'           { $$ = $1; }  // LCOV_EXCL_LINE
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

bins_or_options<nodep>:  // ==IEEE: bins_or_options
        //                      // Superset of IEEE - we allow []'s in more places
                coverage_option                         { $$ = $1; }
        //                      // Can't use wildcardE as results in conflicts
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, $6, false, false, false, $8}, $3); }
        |       yBINS idAny/*bin_identifier*/ '[' cgexpr ']' iffE
                        { // Check for automatic bins: bins auto[N]
                          ఒకవేళ (*$2 == "auto") {
                              $$ = new AstCoverBin{$<fl>2, *$2, $4};
                              DEL($6);
                          } లేకపోతే {
                              $$ = nullptr;
                              BBCOVERIGN($<fl>2, "Unsupported: 'bins' array (non-auto)");
                              DEL($4, $6);
                          }
                        }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, $6, true, false, false, $8}, $3); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, $6, false, true, false, $8}, $3); }
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, new AstCoverWith{$8, $6, $10}, false, false, false, $12}, $3); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, new AstCoverWith{$8, $6, $10}, true, false, false, $12}, $3); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, new AstCoverWith{$8, $6, $10}, false, true, false, $12}, $3); }
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' id/*cover_point_id*/ yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, new AstCoverWith{$6, new AstCoverpointRef{$<fl>5, *$5}, $8}, false, false, false, $10}, $3); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' id/*cover_point_id*/ yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, new AstCoverWith{$6, new AstCoverpointRef{$<fl>5, *$5}, $8}, true, false, false, $10}, $3); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' id/*cover_point_id*/ yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>2, *$2, new AstCoverWith{$6, new AstCoverpointRef{$<fl>5, *$5}, $8}, false, true, false, $10}, $3); }
        |       yWILDCARD yBINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, $7, false, false, true, $9}, $4); }
        |       yWILDCARD yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, $7, true, false, true, $9}, $4); }
        |       yWILDCARD yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, $7, false, true, true, $9}, $4); }
        |       yWILDCARD yBINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, new AstCoverWith{$9, $7, $11}, false, false, true, $13}, $4); }
        |       yWILDCARD yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, new AstCoverWith{$9, $7, $11}, true, false, true, $13}, $4); }
        |       yWILDCARD yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' '{' range_list '}' yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, new AstCoverWith{$9, $7, $11}, false, true, true, $13}, $4); }
        |       yWILDCARD yBINS idAny/*bin_identifier*/ bins_orBraE '=' id/*cover_point_id*/ yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, new AstCoverWith{$7, new AstCoverpointRef{$<fl>6, *$6}, $9}, false, false, true, $11}, $4); }
        |       yWILDCARD yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' id/*cover_point_id*/ yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, new AstCoverWith{$7, new AstCoverpointRef{$<fl>6, *$6}, $9}, true, false, true, $11}, $4); }
        |       yWILDCARD yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' id/*cover_point_id*/ yWITH__PAREN '(' cgexpr ')' iffE
                        { $$ = GRAMMARP->coverBinArray(new AstCoverBin{$<fl>3, *$3, new AstCoverWith{$7, new AstCoverpointRef{$<fl>6, *$6}, $9}, false, true, true, $11}, $4); }
        //
        //                      // cgexpr part of trans_list
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { స్థిరరాశి bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_సమితి*>($5), VCoverBinsType{VCoverBinsType::BINS_TRANSITION}, isArray};
                          ఒకవేళ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { స్థిరరాశి bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_సమితి*>($5), VCoverBinsType{VCoverBinsType::BINS_IGNORE}, isArray};
                          ఒకవేళ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { స్థిరరాశి bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_సమితి*>($5), VCoverBinsType{VCoverBinsType::BINS_ILLEGAL}, isArray};
                          ఒకవేళ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yWILDCARD yBINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($4); BBCOVERIGN($<fl>1, "Unsupported: 'wildcard' transition list in cover bin"); DEL($6, $7);}
        |       yWILDCARD yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($4); BBCOVERIGN($<fl>1, "Unsupported: 'wildcard' transition list in cover bin"); DEL($6, $7);}
        |       yWILDCARD yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($4); BBCOVERIGN($<fl>1, "Unsupported: 'wildcard' transition list in cover bin"); DEL($6, $7);}
        //
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_DEFAULT};
                          ఒకవేళ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_IGNORE};
                          ఒకవేళ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_ILLEGAL};
                          ఒకవేళ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        ;

bins_orBraE<కణుపు_Exprp>:  // IEEE: part of bins_or_options: AstUnbounded for '[]', else the size
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       '[' ']'                                 { $$ = new AstUnbounded{$<fl>1}; }
        |       '[' cgexpr ']'
                        { $$ = $2;
                          ఒకవేళ (VN_IS($2, Unbounded)) $2->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)"); }
        ;

trans_list<nodep>:  // ==IEEE: trans_list
                '(' trans_సమితి ')'                       { $$ = $2; }
        |       trans_list ',' '(' trans_సమితి ')'        { $$ = కూడు_తదుపరి_Null($1, $4); }
        ;

trans_సమితి<nodep>:  // ==IEEE: trans_set (returns AstCoverTransSet)
                trans_range_list                        {
                        Ast_Cover_Trans_అంశం* స్థిరరాశి itemp = static_cast<Ast_Cover_Trans_అంశం*>($1);
                        $$ = new Ast_Cover_Trans_సమితి{$<fl>1, itemp};
                }
        |       trans_సమితి yP_EQGT trans_range_list
                {
                        Ast_Cover_Trans_సమితి* స్థిరరాశి setp = static_cast<Ast_Cover_Trans_సమితి*>($1);
                        Ast_Cover_Trans_అంశం* స్థిరరాశి itemp = static_cast<Ast_Cover_Trans_అంశం*>($3);
                        setp->కూడు_Itemsp(itemp);
                        $$ = setp;
                }
        ;

trans_range_list<nodep>:  // ==IEEE: trans_range_list (returns AstCoverTransItem)
                trans_అంశం                              {
                        // Simple transition item without repetition
                        $$ = new Ast_Cover_Trans_అంశం{$<fl>1, $1, VTransRepType::NONE};
                }
        |       trans_అంశం yP_BRASTAR cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3); }
        |       trans_అంశం yP_BRASTAR cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3, $5); }
        |       trans_అంశం yP_BRAMINUSGT cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($3);
                          $$ = new Ast_Cover_Trans_అంశం{$<fl>1, $1, VTransRepType::GOTO}; }
        |       trans_అంశం yP_BRAMINUSGT cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($1, $3, $5); }
        |       trans_అంశం yP_BRAEQ cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($3);
                          $$ = new Ast_Cover_Trans_అంశం{$<fl>1, $1, VTransRepType::NONCONS}; }
        |       trans_అంశం yP_BRAEQ cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($1, $3, $5); }
        ;

trans_అంశం<nodep>:  // ==IEEE: range_list (returns range list node)
                covergroup_range_list                   { $$ = $1; }
        ;

covergroup_range_list<nodep>:  // ==IEEE: covergroup_range_list
                covergroup_విలువ_range                  { $$ = $1; }
        |       covergroup_range_list ',' covergroup_విలువ_range        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;


cover_cross<nodep>:  // ==IEEE: cover_cross
                id/*cover_point_identifier*/ ':' yCROSS list_యొక్క_cross_వస్తువులు iffE cross_body
                        {
                          AstCoverCross* స్థిరరాశి nodep = new AstCoverCross{$<fl>3, *$1,
                                                          VN_AS($4, CoverpointRef), $5};
                          ఒకవేళ ($6) nodep->కూడు_Binsp($6);
                          $$ = nodep;
                        }
        |       yCROSS list_యొక్క_cross_వస్తువులు iffE cross_body
                        {
                          AstCoverCross* స్థిరరాశి nodep = new AstCoverCross{$<fl>1,
                                                          "__cross" + cvt_గమ్యం_Str(GRAMMARP->s_type_Imp_అంకె++),
                                                          VN_AS($2, CoverpointRef), $3};
                          ఒకవేళ ($4) nodep->కూడు_Binsp($4);
                          $$ = nodep;
                        }
        ;

list_యొక్క_cross_వస్తువులు<nodep>:  // ==IEEE: list_of_cross_items
                cross_అంశం ',' cross_అంశం               { $$ = కూడు_తదుపరి_Null($1, $3); }
        |       cross_అంశం ',' cross_అంశం ',' cross_అంశం_List
                        { $$ = కూడు_తదుపరి_Null(కూడు_తదుపరి_Null($1, $3), $5); }
        ;

cross_అంశం_List<nodep>:  // IEEE: part of list_of_cross_items
                cross_అంశం                              { $$ = $1; }
        |       cross_అంశం_List ',' cross_అంశం           { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

cross_అంశం<nodep>:  // ==IEEE: cross_item
        //                      // IEEE: cover_point_identifier | variable_identifier - both are a
        //                      // simple identifier.  We parse idDotted (a plain hierarchical
        //                      // reference a.b.c, with no bit/array selects) to also accept the
        //                      // non-standard dotted form (e.g. 'cross a.b') that several
        //                      // simulators support; the common simple-identifier case is detected
        //                      // and handled exactly as before.
                idDotted
                        {
                          ఒకవేళ (AstParseRef* స్థిరరాశి refp = VN_CAST($1, ParseRef)) {
                              // Standard: simple cover_point_identifier / variable_identifier
                              $$ = new AstCoverpointRef{refp->fileline(), refp->name()};
                              VL_DO_DANGLING(refp->delete_వృక్షం(), refp);
                          } లేకపోతే {
                              // Verilator extension beyond strict IEEE (cross_item is a simple
                              // identifier): some tools accept a hierarchical/dotted reference.
                              // Carry the reference expression (still an AstDot here) out of the
                              // parser unchanged; later stages resolve and, eventually, implement
                              // it as an implicit coverpoint.
                              $1->v3warn(NONSTD, "Non-standard hierarchical reference as a coverage "
                                                 "cross item (an implicit coverpoint)");
                              $$ = new AstCoverpointRef{$1->fileline(), $1};
                          }
                        }
        ;

cross_body<nodep>:  // ==IEEE: cross_body
                '{' '}'                                 { $$ = nullptr; }
        //                      // IEEE-2012: No semicolon here, mistake in spec
        |       '{' cross_body_అంశం_List '}'             { $$ = $2; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' cross_body_అంశం_List error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

cross_body_అంశం_List<nodep>:  // IEEE: part of cross_body
                cross_body_అంశం                         { $$ = $1; }
        |       cross_body_అంశం_List cross_body_అంశం     { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

cross_body_అంశం<nodep>:  // ==IEEE: cross_body_item
                function_declaration
                        { $$ = nullptr; BBCOVERIGN($1->fileline(), "Unsupported: 'function' in coverage cross body"); DEL($1); }
        //                      // IEEE: bins_selection_or_option
        |       coverage_option ';'                     { $$ = $1; }
        //                      // IEEE: bins_selection
        |       yBINS idAny/*new-bin_identifier*/ '=' select_వ్యక్తీకరణ iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5}; }
        |       yIGNORE_BINS idAny/*new-bin_identifier*/ '=' select_వ్యక్తీకరణ iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_IGNORE}; }
        |       yILLEGAL_BINS idAny/*new-bin_identifier*/ '=' select_వ్యక్తీకరణ iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_ILLEGAL}; }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

select_వ్యక్తీకరణ<nodep>:  // ==IEEE: select_expression
                select_వ్యక్తీకరణ_r
                        { $$ = $1; }
        //                      // IEEE: integer_covergroup_expression ::= covergroup_expression | '$'
        //                      // '$' is part of our expr.  'matches' has higher precedence than
        //                      // '&&' and '||', which end the count (IEEE 1800-2023 footnote 30).
        //                      // A count could end in a function call followed by its 'with', so
        //                      // a selection with a count is filtered further only in parentheses.
        |       select_వ్యక్తీకరణ_r yWITH__PAREN '(' cgexpr ')' yMATCHES సమాసం %prec yP_ANDAND
                        { $$ = new AstCoverCrossWith{$2, $1, $4, $7}; }
        //                      // IEEE: cross_set_expression yMATCHES integer_covergroup_expression
        |       idAny '(' list_యొక్క_arguments_E ')' yMATCHES సమాసం %prec yP_ANDAND
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3, $6); }
        |       select_వ్యక్తీకరణ yP_ANDAND select_వ్యక్తీకరణ
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, false}; }
        |       select_వ్యక్తీకరణ yP_OROR   select_వ్యక్తీకరణ
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, true}; }
        ;

// This non-terminal exists to disambiguate select_expression and make "with" bind tighter
select_వ్యక్తీకరణ_r<nodep>:
        //                      // IEEE: select_condition expanded here
                yBINSOF '(' bins_వ్యక్తీకరణ ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}}; }
        |       '!' yBINSOF '(' bins_వ్యక్తీకరణ ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true}; }
        //                      // IEEE: covergroup_range_list has the same syntax as range_list
        |       yBINSOF '(' bins_వ్యక్తీకరణ ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}, false, $7}; }
        |       '!' yBINSOF '(' bins_వ్యక్తీకరణ ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true, $8}; }
        |       yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($3); }
        |       '!' yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($4); }
        |       select_వ్యక్తీకరణ_r yWITH__PAREN '(' cgexpr ')'
                        { $$ = new AstCoverCrossWith{$2, $1, $4, nullptr}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr    { }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP '!' yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr { }
        //
        |       '(' select_వ్యక్తీకరణ ')'                       { $$ = $2; }
        |       idAny/*cross_identifier*/                        { $$ = new AstCoverCrossRef{$<fl>1, *$1}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP  cgexpr  { $$ = nullptr; BBCOVERIGN($1, "Ignoring unsupported: coverage select expression"); }
        //
        //                      // IEEE: cross_set_expression [ yMATCHES integer_covergroup_expression ]
        //UNSUP  cgexpr    {..}
        //UNSUP                 // Below are all removed
        |       idAny '(' list_యొక్క_arguments_E ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3); }
        //UNSUP                 // Above are all removed, replace with:
        ;

bins_వ్యక్తీకరణ<కణుపు_Exprp>:  // ==IEEE: bins_expression
        //                      // "cover_point_identifier" and "variable_identifier" look identical
        // IEEE specifies:
        // bins_expression ::=
        //    variable_identifier
        //    | cover_point_identifier [ . bin_identifier ]
        // Verilator supports hierarchical reference in a place of variable identifier.
        // This is an extension based on other simulators.
               idDotted
                        { $$ = $1; }
        ;

coverage_eventE<nodep>:  // IEEE: [ coverage_event ]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       clocking_event
                        { $$ = $1; }  // Keep the clocking event for automatic sampling
        |       yWITH__ETC yFUNCTION idAny/*"sample"*/ '(' tf_పోర్టు_list_E ')'
                        { ఒకవేళ (*$3 != "sample") {
                            $<fl>3->v3error("Coverage sampling function must be named 'sample'"
                                            " (IEEE 1800-2023 19.8.1)");
                            $$ = nullptr;
                            DEL($5);
                          } లేకపోతే {
                            $$ = $5;
                          }
                        }
        |       yP_ATAT '(' ఖండం_event_వ్యక్తీకరణ ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: '@@' coverage event"); DEL($3); }
        ;

ఖండం_event_వ్యక్తీకరణ<nodep>:  // ==IEEE: block_event_expression
                ఖండం_event_వ్యక్తీకరణ_పదం              { $$ = nullptr; /*UNSUP @@*/ DEL($1); }
        |       ఖండం_event_వ్యక్తీకరణ yOR ఖండం_event_వ్యక్తీకరణ_పదం   { $$ = nullptr; /*UNSUP @@*/ DEL($1, $3);  }
        ;

ఖండం_event_వ్యక్తీకరణ_పదం<nodep>:  // IEEE: part of block_event_expression
                yBEGIN క్రమానుగత_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        |       y_చివర   క్రమానుగత_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        ;

క్రమానుగత_btf_identifier<nodep>:  // ==IEEE: hierarchical_btf_identifier
        //                      // hierarchical_tf_identifier + hierarchical_block_identifier
        //                      // method_identifier
                packageClassScopeE idAny                { $$ = nullptr; /*UNSUP*/ DEL($1); }
        ;

//**********************************************************************
// Randsequence

randsequence_statement<కణుపు_Stmtp>:  // ==IEEE: randsequence_statement
                yRANDSEQUENCE '(' ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, "", $4};
                          v3Global.useRandSequence(true); }
        |       yRANDSEQUENCE '(' idAny/*rs_production_identifier*/ ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, *$3, $5};
                          v3Global.useRandSequence(true); }
        ;

rs_productionList<rSProdp>:  // IEEE: rs_production+
                rs_production                           { $$ = $1; }
        |       rs_productionList rs_production         { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

rs_production<rSProdp>:  // ==IEEE: rs_production
                rs_productionFront ':' rs_నియమం_List ';'
                        { $$ = $1; $1->కూడు_Rulesp($3); }
        ;

rs_productionFront<rSProdp>:  // IEEE: part of rs_production
                rs_funcId/*rs_production_identifier*/   { $$ = $1; }
        |       rs_funcId '(' tf_పోర్టు_list_E ')'         { $$ = $1; $$->కూడు_Portsp($3); }
        ;

rs_funcId<rSProdp>:  // IEEE: part of rs_production
                /**/ rs_fId
                        { $$ = $1; }  // Note is void as default, not logic as default like functions
        |       signingE rangeList rs_fId
                        { $$ = $3;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::పోర్టు, $3->name(), VFlag_బిడ్డ_DType{},
                                               GRAMMARP->కూడు_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)}); }
        |       signing rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::పోర్టు, $2->name(), VFlag_బిడ్డ_DType{},
                                               new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}}); }
        |       దత్తాంశం_type rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::పోర్టు, $2->name(), VFlag_బిడ్డ_DType{},
                                               $1}); }
        |       yVOID rs_fId
                        { $$ = $2; }
        ;

rs_fId<rSProdp>:  // IEEE: part of rs_production
                id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstRSProd{$<fl>$, *$1, nullptr, nullptr}; }
        ;

rs_నియమం_List<rSRulep>:  // IEEE: rs_rule+ part of rs_production
                rs_నియమం                                 { $$ = $1; }
        |       rs_నియమం_List '|' rs_నియమం                 { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

rs_నియమం<rSRulep>:  // ==IEEE: rs_rule
                rs_production_list
                        { $$ = new AstRSRule{$1->fileline(), nullptr, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_బరువు_specification
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_బరువు_specification rs_సంకేతం_ఖండం
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, $4}; }
        ;

rs_production_list<rSProdListp>:  // ==IEEE: rs_production_list
                rs_prodList
                        { $$ = new AstRSProdList{CRELINE(), nullptr, $1}; }
        |       yRAND yJOIN rs_production_అంశం rs_production_అంశం_List
                        { $$ = new AstRSProdList{$1, new AstConst{$2, AstConst::RealDouble{}, 0.5}, $3};
                          $$->randJoin(true);
                          $$->కూడు_Prodsp($4); }
        |       yRAND yJOIN '(' సమాసం ')' rs_production_అంశం rs_production_అంశం_List
                        { $$ = new AstRSProdList{$1, $4, $6};
                          $$->randJoin(true);
                          $$->కూడు_Prodsp($7); }
        ;

rs_బరువు_specification<కణుపు_Exprp>:  // ==IEEE: rs_weight_specification
                intnumAsConst                           { $$ = $1; }
        |       idClassSel/*ps_identifier*/             { $$ = $1; }
        |       '(' సమాసం ')'                            { $$ = $2; }
        ;

rs_సంకేతం_ఖండం<nodep>:  // ==IEEE: rs_code_block
                '{' '}'                                 { $$ = nullptr; }
        |       '{' rs_సంకేతం_ఖండం_అంశం_List '}'           { $$ = $2; }
        ;

rs_సంకేతం_ఖండం_అంశం_List<nodep>:  // IEEE: part of rs_code_block
                rs_సంకేతం_ఖండం_అంశం                       { $$ = $1; }
        |       rs_సంకేతం_ఖండం_అంశం_List rs_సంకేతం_ఖండం_అంశం         { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

rs_సంకేతం_ఖండం_అంశం<nodep>:  // IEEE: part of rs_code_block
                దత్తాంశం_declaration                        { $$ = $1; }
        |       stmt                                    { $$ = $1; }
        ;

rs_prodList<nodep>:  // IEEE: rs_prod+
                rs_prod                                 { $$ = $1; }
        |       rs_prodList rs_prod                     { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

rs_prod<nodep>:  // ==IEEE: rs_prod
                rs_production_అంశం                      { $$ = $1; }
        |       rs_సంకేతం_ఖండం                           { $$ = $1; }
        //                      // IEEE: rs_if_else
        |       yIF '(' సమాసం ')' rs_production_అంశం %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstIf{$<fl>1, $3, $5, nullptr}; }
        |       yIF '(' సమాసం ')' rs_production_అంశం yELSE rs_production_అంశం
                        { $$ = new AstIf{$<fl>1, $3, $5, $7}; }
        //                      // IEEE: rs_repeat
        |       yREPEAT '(' సమాసం ')' rs_production_అంశం
                               { $$ = new AstRepeat{$<fl>1, $3, $5}; }
        //                      // IEEE: rs_case
        |       yCASE '(' సమాసం ')' rs_case_అంశం_List yENDCASE
                        { $$ = new AstCase{$<fl>1, VCaseType::CT_RANDSEQUENCE, $3, $5}; }
        ;

rs_production_అంశం_List<nodep>:  // IEEE: rs_production_item+
                rs_production_అంశం                         { $$ = $1; }
        |       rs_production_అంశం_List rs_production_అంశం  { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

rs_production_అంశం<nodep>:  // ==IEEE: rs_production_item
                idAny/*production_identifier*/
                        { $$ = new Ast_RSProd_అంశం{$<fl>1, *$1, nullptr}; }
        |       idAny/*production_identifier*/ '(' list_యొక్క_arguments_E ')'
                        { $$ = new Ast_RSProd_అంశం{$<fl>1, *$1, $3}; }
        ;

rs_case_అంశం_List<caseItemp>:  // IEEE: rs_case_item+
                rs_case_అంశం                            { $$ = $1; }
        |       rs_case_అంశం_List rs_case_అంశం           { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

rs_case_అంశం<caseItemp>:  // ==IEEE: rs_case_item
                caseCondList ':' rs_production_అంశం ';'
                        { $$ = new Ast_Case_అంశం{$<fl>1, $1, $3}; }
        |       yDEFAULT rs_production_అంశం ';'
                        { $$ = new Ast_Case_అంశం{$<fl>1, nullptr, $2}; }
        |       yDEFAULT ':' rs_production_అంశం ';'
                        { $$ = new Ast_Case_అంశం{$<fl>1, nullptr, $3}; }
        ;

//**********************************************************************
// Checker

checker_declaration<కణుపు_Modulep>:  // ==IEEE: part of checker_declaration
                checkerFront checker_పోర్టు_list_E ';'
                        checker_or_generate_అంశం_List_E yENDCHECKER చివర_Label_E
                        { $$ = $1;
                          $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));
                          ఒకవేళ ($2) $1->కూడు_Stmtsp($2);
                          ఒకవేళ ($4) $1->కూడు_Stmtsp($4);
                          GRAMMARP->చివర_Label($<fl>6, $1, $6); }
        ;

checkerFront<కణుపు_Modulep>:  // IEEE: part of checker_declaration
                yCHECKER idAny/*checker_identifier*/
                        { $$ = new AstModule{$<fl>2, *$2, PARSEP->libname(), AstModule::Checker{}};
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->సమయం_చివరి_Unit());
                          PARSEP->rootp()->timeprecision_విలీనం($$->fileline(),
                                                              PARSEP->సమయం_చివరి_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive()); }
        |       checkerFront sigAttrScope               { $$ = $1; }
        ;

checker_పోర్టు_list_E<nodep>:  // IEEE: [ ( [ checker_port_list ] ) ]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '('
        /*mid*/         { VARRESET_LIST(పోర్టు); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    checker_పోర్టు_list ')'
                        { $$ = $3; }
        ;

checker_పోర్టు_list<nodep>:  // ==IEEE: checker_port_list
                checker_పోర్టు_అంశం                             { $$ = $1; }
        |       checker_పోర్టు_list ',' checker_పోర్టు_అంశం       { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

checker_పోర్టు_అంశం<nodep>:  // IEEE: checker_port_item
                checker_పోర్టు_అంశం_Front checker_పోర్టు_అంశం_Assignment  { $$ = $2; }
        ;

checker_పోర్టు_అంశం_Front:  // IEEE: part of checker_port_item
                checker_పోర్టు_direction_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       checker_పోర్టు_direction_E దత్తాంశం_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_పోర్టు = true; }
        |       checker_పోర్టు_direction_E yVAR దత్తాంశం_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_పోర్టు = true; }
        |       checker_పోర్టు_direction_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       checker_పోర్టు_direction_E implicit_typeE
                        { VARDTYPE($2); }
        ;

checker_పోర్టు_direction_E:  // IEEE: [ checker_port_direction ]
                /*ఖాళీలు*/                             { VARIO(ఆదానం); }
        |       y_ఆదానం                                  { VARIO(ఆదానం); }
        |       yOUTPUT                                 { VARIO(OUTPUT); }
        ;

checker_పోర్టు_అంశం_Assignment<nodep>:  // IEEE: part of checker_port_direction
                id variable_dimensionListE
                        { $$ = new Ast_పోర్టు{CRELINE(), PINNUMINC(), *$1};
                          $$->కూడు_తదుపరి(VARDONEA($<fl>1, *$1, $2, nullptr)); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { $$ = new Ast_పోర్టు{CRELINE(), PINNUMINC(), *$1};
                          $$->కూడు_తదుపరి(VARDONEA($<fl>1, *$1, $2, $4));
                          BBUNSUP($3, "Unsupported: checker port variable default value"); }
        ;

checker_or_generate_అంశం_List_E<nodep>:  // IEEE: [{ checker_or_generate_itemList }]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       checker_or_generate_అంశం_List            { $$ = $1; }
        ;

checker_or_generate_అంశం_List<nodep>:  // IEEE: { checker_or_generate_itemList }
                checker_or_generate_అంశం                { $$ = $1; }
        |       checker_or_generate_అంశం_List checker_or_generate_అంశం   { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

checker_or_generate_అంశం<nodep>:  // ==IEEE: checker_or_generate_item
                checker_or_generate_అంశం_declaration    { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        //                      // IEEE: checker_construct
        |       always_construct                        { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       assertion_అంశం                          { $$ = $1; }
        |       continuous_కేటాయించు                       { $$ = $1; }
        |       checker_generate_అంశం                   { $$ = $1; }
        ;

checker_or_generate_అంశం_declaration<nodep>:  // ==IEEE: checker_or_generate_item_declaration
                దత్తాంశం_declaration                        { $$ = $1; }
        |       yRAND దత్తాంశం_declaration
                        { $$ = $2; BBUNSUP($1, "Unsupported: checker rand"); }
        |       function_declaration                    { $$ = $1; }
        |       checker_declaration
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: recursive 'checker'"); DEL($1); }
        |       assertion_అంశం_declaration              { $$ = $1; }
        |       covergroup_declaration                  { $$ = $1; }
        //      // IEEE deprecated: overload_declaration
        |       genvar_declaration                      { $$ = $1; }
        |       clocking_declaration                    { $$ = $1; }
        |       modDefaultClocking                      { $$ = $1; }
        |       defaultDisable                          { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

checker_generate_అంశం<nodep>:  // ==IEEE: checker_generate_item
        //                      // Specialized for checker so need "c_" prefixes here
                c_loop_generate_construct               { $$ = $1; }
        |       c_conditional_generate_construct        { $$ = $1; }
        |       c_generate_region                       { $$ = $1; }
        //
        |       severity_system_task                    { $$ = $1; }
        ;

//UNSUPchecker_instantiation<nodep>:
//UNSUP //                      // Only used for procedural_assertion_item's
//UNSUP //                      // Version in concurrent_assertion_item looks like etcInst
//UNSUP //                      // Thus instead of *_checker_port_connection we can use etcInst's instPinListE
//UNSUP         id/*checker_identifier*/ id '(' instPinListE ')' ';'     { }
//UNSUP ;

//**********************************************************************
// Class

class_declaration<nodep>:       // ==IEEE: part of class_declaration
        //                      // IEEE-2012: using this also for interface_class_declaration
        //                      // The classExtendsE rule relies on classFront having the
        //                      // new class scope correct via classFront
                classFront parameter_పోర్టు_list_E classExtendsE classImplementsE ';'
        /*mid*/         { // Allow resolving types declared in base extends class
                          $1->hasParameterList($<flag>2);
                        }
        /*cont*/    class_అంశం_List_చివర చివర_Label_E
                        { $$ = $1; $1->కూడు_Membersp($2);
                          $1->కూడు_Extendsp($3);
                          $1->కూడు_Extendsp($4);
                          $1->కూడు_Membersp($7);
                          GRAMMARP->చివర_Label($<fl>8, $1, $8); }
        ;

classFront<classp>:             // IEEE: part of class_declaration
        //                      // IEEE 1800-2023: lifetimeE replaced with final_specifierE
                classVirtualE yCLASS final_specifierE lifetimeE idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$5, PARSEP->libname()};
                          $$->పునాది_Override($3);
                          $$->isVirtual($1);
                          v3Global.సమితి_Has_Classes(); }
        //                      // IEEE: part of interface_class_declaration
        //                      // IEEE 1800-2023: lifetimeE removed
        |       yINTERFACE yCLASS idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$3, PARSEP->libname()};
                          $$->isInterfaceClass(true);
                          v3Global.సమితి_Has_Classes(); }
        ;

classVirtualE<cbool>:
                /*ఖాళీలు*/                             { $$ = false; }
        |       yVIRTUAL__CLASS                         { $$ = true; }
        ;

classExtendsE<classExtendsp>:           // IEEE: part of class_declaration
        //                      // The classExtendsE rule relies on classFront having the
        //                      // new class scope correct via classFront
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       yEXTENDS classExtendsList               { $$ = $2; }
        ;

classExtendsList<classExtendsp>:        // IEEE: part of class_declaration
                classExtendsOne                         { $$ = $1; }
        |       classExtendsList ',' classExtendsOne    { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

classExtendsOne<classExtendsp>:         // IEEE: part of class_declaration
                class_typeExtImpList
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_లోపల_Implements}; }
        |       class_typeExtImpList '(' list_యొక్క_arguments_E ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_లోపల_Implements};
                          $$->కూడు_Argsp($3); }
        //                      // IEEE-2023: Added: yEXTENDS class_type '(' yDEFAULT ')'
        |       class_typeExtImpList '(' yDEFAULT ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_లోపల_Implements};
                          BBUNSUP($<fl>2, "Unsupported: 'extends' with 'default'"); }
        ;

classImplementsE<classExtendsp>:        // IEEE: part of class_declaration
        //                      // All 1800-2012
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       yIMPLEMENTS
        /*mid*/         { GRAMMARP->m_లోపల_Implements = true; }
        /*cont*/    classImplementsList
                        { $$ = $3;
                          GRAMMARP->m_లోపల_Implements = false; }
        ;

classImplementsList<classExtendsp>:     // IEEE: part of class_declaration
        //                      // All 1800-2012
                classExtendsOne                         { $$ = $1; }
        |       classImplementsList ',' classExtendsOne
                        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

class_typeExtImpList<కణుపు_Exprp>:  // IEEE: class_type: "[package_scope] id [ parameter_value_assignment ]"
        //                      // but allow yaID__aTYPE for extends/implements
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                class_typeExtImpOne                     { $$ = $1; }
        |       class_typeExtImpList yP_COLONCOLON class_typeExtImpOne
                        { $$ = new AstDot{$<fl>1, true, $1, $3}; }
        ;

class_typeExtImpOne<కణుపు_Exprp>:  // part of IEEE: class_type, where we either get a package_scope component or class
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
        //                      // Not listed in IEEE, but see bug627 any parameter type maybe a class
        //                      // If idAny below is a class, parameter_value is legal
        //                      // If idAny below is a package, parameter_value is not legal
        //                      // If idAny below is otherwise, not legal
                idAny
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_విలువ_assignment_Class_E
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $3}; }
        |       idCC
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_విలువ_assignment_Class_E
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $3}; }
        //
        //                      // package_sopeIdFollows expanded
        |       yD_UNIT yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, "$unit", nullptr, nullptr}; }
        ;

//=========
// Package scoping - to traverse the symbol table properly, the final identifer
// must be included in the rules below.
// Each of these must end with {symsPackageDone | symsClassDone}

//=== Below rules assume special scoping per above

packageClassScopeNoId<కణుపు_Exprp>:  // IEEE: [package_scope] not followed by yaID
                packageClassScope                       { $$ = $1; }
        ;

packageClassScopeE<కణుపు_Exprp>:  // IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // TODO: To support classes should return generic type, not packagep
        //                      // class_qualifier := [ yLOCAL '::'  ] [ implicit_class_handle '.'  class_scope ]
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       packageClassScope                       { $$ = $1; }
        ;

packageClassScope<కణుపు_Exprp>:   // IEEE: class_scope
        //                      // IEEE: "class_type yP_COLONCOLON"
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
                packageClassScopeList                   { $$ = $1; }
        |       local_తదుపరి_Id yP_COLONCOLON               { $$ = $1; }
        |       dollar_Unit_తదుపరి_Id yP_COLONCOLON          { $$ = $1; }
        |       dollar_Unit_తదుపరి_Id yP_COLONCOLON packageClassScopeList
                        { $$ = new AstDot{$2, true, $1, $3}; }
        ;

packageClassScopeList<కణుపు_Exprp>:   // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      // Or IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                package_Class_Scope_అంశం                   { $$ = $1; }
        |       packageClassScopeList package_Class_Scope_అంశం
                        { $$ = new AstDot{$<fl>2, true, $1, $2}; }
        ;

package_Class_Scope_అంశం<కణుపు_Exprp>:   // IEEE: package_scope or [package_scope]::[class_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      //vv mid rule action needed otherwise we might not have NextId in time to parse the id token
                idCC
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, nullptr}; }
        //
        |       idCC parameter_విలువ_assignment_Class
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $2}; }
        ;

dollar_Unit_తదుపరి_Id<కణుపు_Exprp>:    // $unit
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yD_UNIT
                        { $$ = new AstClassOrPackageRef{$1, "$unit", nullptr, nullptr}; }
        ;

local_తదుపరి_Id<కణుపు_Exprp>:         // local::
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yLOCAL__COLONCOLON
                        { $$ = new AstClassOrPackageRef{$1, "local::", nullptr, nullptr}; }
        ;

//^^^=========

class_అంశం_List_చివర<nodep>:
                yENDCLASS                               { $$ = nullptr; }
        |       class_అంశం_List yENDCLASS                { $$ = $1; }
        |       error yENDCLASS                         { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       class_అంశం_List error yENDCLASS          { $$ = $1; }  // LCOV_EXCL_LINE
        ;

class_అంశం_List<nodep>:
                class_అంశం                              { $$ = $1; }
        |       class_అంశం_List class_అంశం               { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

class_అంశం<nodep>:                      // ==IEEE: class_item
                class_property                          { $$ = $1; }
        |       class_పద్ధతి_2                            { $$ = $1; }
        |       class_constraint                        { $$ = $1; }
        //
        |       class_declaration                       { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       covergroup_declaration
                        {
                          ఒకవేళ ($1) {
                              // Also declares an instance variable, of the covergroup's type
                              AstVar* స్థిరరాశి newp = new AstVar{$1->fileline(), VVarType::VAR,
                                  $1->name(), VFlag_బిడ్డ_DType{},
                                  new AstRefDType($1->fileline(), $1->name())};
                              newp->embeddedCovergroup(true);
                              $$ = కూడు_తదుపరి_Null($1, newp);
                          } లేకపోతే {
                              $$ = nullptr;
                          }
                        }
        //                      // local_parameter_declaration under parameter_declaration
        |       parameter_declaration ';'               { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        //                      // Verilator specific
        |       vl_Sc_ఖండం                               { $$ = $1; }
        //
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

class_పద్ధతి_2<nodep>:            // ==IEEE: class_method
                memberQualListE task_declaration        { $$ = $2; $1.apply_గమ్యం_Nodes($2); }
        |       memberQualListE function_declaration    { $$ = $2; $1.apply_గమ్యం_Nodes($2); }
        |       y_శుద్ధ yVIRTUAL__ETC memberQualListE పద్ధతి_prototype ';'
                        { $$ = $4; $3.apply_గమ్యం_Nodes($4); $4->శుద్ధ_Virtual(true); $4->isVirtual(true); }
        |       yEXTERN memberQualListE పద్ధతి_prototype ';'
                        { $$ = $3; $2.apply_గమ్యం_Nodes($3); $3->isExternProto(true); }
        //                      // IEEE: "method_qualifierE class_constructor_declaration"
        //                      // part of function_declaration
        |       yEXTERN memberQualListE class_constructor_prototype
                        { $$ = $3; $2.apply_గమ్యం_Nodes($3); $3->isExternProto(true); }
        ;

dynamic_override_specifiersE<పునాది_Override>:  // IEEE: dynamic_override_specifiers or empty
                /*ఖాళీలు*/                             { $$ = VBaseOverride{}; }
        //                      // IEEE: Expanded [ initial_or_extends_specifier ] [ final_specifier ]
        //                      // Note lifetime used by members is instead under memberQual
        |       ':' yINITIAL                            { $$ = VBaseOverride{VBaseOverride::Initial{}}; }
        |       ':' yEXTENDS                            { $$ = VBaseOverride{VBaseOverride::Extends{}}; }
        |       ':' yINITIAL ':' yFINAL                 { $$ = VBaseOverride{VBaseOverride::Initial{}};
                                                          $$.combine(VBaseOverride{VBaseOverride::Final{}}); }
        |       ':' yEXTENDS ':' yFINAL                 { $$ = VBaseOverride{VBaseOverride::Extends{}};
                                                          $$.combine(VBaseOverride{VBaseOverride::Final{}}); }
        |       ':' yFINAL                              { $$ = VBaseOverride{VBaseOverride::Final{}}; }
        ;

final_specifierE<పునాది_Override>:  // ==IEEE: final_specifier (similar to dynamic_override_specifiers)
                /*ఖాళీలు*/                             { $$ = VBaseOverride{}; }
        |       ':' yFINAL                              { $$ = VBaseOverride{VBaseOverride::Final{}}; }
        ;

// IEEE: class_constructor_prototype
// See function_declaration

memberQualListE<qualifiers>:    // Called from class_property for all qualifiers before yVAR
        //                      // Also before method declarations, to prevent grammar conflict
        //                      // Thus both types of qualifiers (method/property) are here
                /*empty*/                               { $$ = VMemberQualifiers::none(); }
        |       memberQualList                          { $$ = $1; }
        ;

memberQualList<qualifiers>:
                memberQualOne                           { $$ = $1; }
        |       memberQualList memberQualOne            { $$ = VMemberQualifiers::combine($1, $2); }
        ;

memberQualOne<qualifiers>:                      // IEEE: property_qualifier + method_qualifier
        //                      // Part of method_qualifier and property_qualifier
        //                      // IMPORTANT: yPROTECTED | yLOCAL is in a lex rule
                yPROTECTED                              { $$ = VMemberQualifiers::none(); $$.m_protected = true; }
        |       yLOCAL__ETC                             { $$ = VMemberQualifiers::none(); $$.m_local = true; }
        |       ySTATIC__ETC                            { $$ = VMemberQualifiers::none(); $$.m_static = true; }
        //                      // Part of method_qualifier only
        |       yVIRTUAL__ETC                           { $$ = VMemberQualifiers::none(); $$.m_virtual = true; }
        //                      // Part of property_qualifier only
        |       random_qualifier                        { $$ = $1; }
        //                      // Part of data_declaration, but not in data_declarationVarFrontClass
        |       yCONST__ETC                             { $$ = VMemberQualifiers::none(); $$.m_const = true; }
        ;

//**********************************************************************
// Constraints

class_constraint<constraintp>:  // ==IEEE: class_constraint
        //                      // IEEE: constraint_declaration
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew constraint_ఖండం
                        { $$ = $4; $$->isStatic($1); $$->పునాది_Override($3); $$->కూడు_Itemsp($5); }
        //                      // IEEE: constraint_prototype + constraint_prototype_qualifier
        |       constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $4; $$->isStatic($1); $$->పునాది_Override($3);
                          $$->isExternProto(true); }
        |       yEXTERN constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $5; $$->isStatic($2); $$->పునాది_Override($4);
                          $$->isExternProto(true); $$->isExternExplicit(true); }
        |       y_శుద్ధ constraintStaticE yCONSTRAINT constraintIdNew ';'
                        { $$ = $4; $$->is_Kwd_శుద్ధ($1); $$->isStatic($2); }
        ;

constraintIdNew<constraintp>:  // IEEE: id part of class_constraint
                idAny/*constraint_identifier*/
                        { $$ = new AstConstraint{$<fl>1, *$1, nullptr}; }
        ;

constraint_ఖండం<nodep>:  // ==IEEE: constraint_block
                '{' '}'                                         { $$ = nullptr; }
        |       '{' constraint_ఖండం_అంశం_List '}'               { $$ = $2; }
        //
        |       '{' error '}'                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_ఖండం_అంశం_List error '}'         { $$ = $2; }  // LCOV_EXCL_LINE
        ;

constraint_ఖండం_అంశం_List<nodep>:  // IEEE: { constraint_block_item }
                constraint_ఖండం_అంశం                           { $$ = $1; }
        |       constraint_ఖండం_అంశం_List constraint_ఖండం_అంశం { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

constraint_ఖండం_అంశం<nodep>:  // ==IEEE: constraint_block_item
                constraint_వ్యక్తీకరణ                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        ;

solve_before_list<కణుపు_Exprp>:  // ==IEEE: solve_before_list
                constraint_primary                              { $$ = $1; }
        |       solve_before_list ',' constraint_primary        { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

constraint_primary<కణుపు_Exprp>:  // ==IEEE: constraint_primary
        //                      // exprScope more general than:
        //                      // [ implicit_class_handle '.' | class_scope ] hierarchical_identifier select [ '(' ')' ]
                సమాసం_Scope                                       { $$ = $1; }
        ;

constraint_వ్యక్తీకరణ_List<nodep>:  // ==IEEE: { constraint_expression }
                constraint_వ్యక్తీకరణ                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        |       constraint_వ్యక్తీకరణ_List constraint_వ్యక్తీకరణ { $$ = కూడు_తదుపరి_Null($1, $2); }
        |       constraint_వ్యక్తీకరణ_List ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = కూడు_తదుపరి_Null($1, new AstConstraintBefore{$<fl>2, $3, $5}); }
        ;

constraint_వ్యక్తీకరణ<nodep>:  // ==IEEE: constraint_expression
                సమాసం/*expression_or_dist*/ ';'
                        { $$ = new Ast_Constraint_సమాసం{$1->fileline(), $1}; }
        //                      // 1800-2012:
        |       ySOFT సమాసం/*expression_or_dist*/ ';'
                        { Ast_Constraint_సమాసం* స్థిరరాశి newp = new Ast_Constraint_సమాసం{$1, $2};
                          newp->isSoft(true);
                          $$ = newp; }
        //                      // 1800-2012:
        //                      // IEEE: uniqueness_constraint ';'
        |       yUNIQUE '{' range_list '}' ';'
                        { $$ = new AstConstraintUnique{$1, $3}; }
        //                      // IEEE: expr yP_MINUSGT constraint_set
        //                      // The bare-expression case "expr -> expr ;"
        //                      // arrives via the general expr rule above
        //                      // (AstLogIf) and matches "expr ';'".  Each
        //                      // production below covers one constraint_set
        //                      // shape that is not a plain expression and
        //                      // builds an outer AstConstraintIf wrapping the
        //                      // statement V3Randomize already knows how to
        //                      // lower (with disable-soft hoisted by the
        //                      // ConstraintExprVisitor pre-pass).
        |       సమాసం yP_MINUSGT '{' constraint_వ్యక్తీకరణ_List '}'
                        { $$ = new AstConstraintIf{$2, $1, $4, nullptr}; }
        |       సమాసం yP_MINUSGT yIF '(' సమాసం ')' constraint_సమితి %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, nullptr}, nullptr}; }
        |       సమాసం yP_MINUSGT yIF '(' సమాసం ')' constraint_సమితి yELSE constraint_సమితి
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, $9}, nullptr}; }
        |       సమాసం yP_MINUSGT yFOREACH '(' idClassSelForeach ')' constraint_సమితి
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintForeach{$3, $5, $7}, nullptr}; }
        |       సమాసం yP_MINUSGT yUNIQUE '{' range_list '}' ';'
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintUnique{$3, $5}, nullptr}; }
        |       సమాసం yP_MINUSGT ySOFT సమాసం ';'
                        { Ast_Constraint_సమాసం* స్థిరరాశి innerp = new Ast_Constraint_సమాసం{$3, $4};
                          innerp->isSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       సమాసం yP_MINUSGT yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_సమాసం* స్థిరరాశి innerp = new Ast_Constraint_సమాసం{$3, $5};
                          innerp->isDisableSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       yIF '(' సమాసం ')' constraint_సమితి %prec pr_క్రింది_THAN_ELSE
                        { $$ = new AstConstraintIf{$1, $3, $5, nullptr}; }
        |       yIF '(' సమాసం ')' constraint_సమితి yELSE constraint_సమితి
                        { $$ = new AstConstraintIf{$1, $3, $5, $7}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM + 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' constraint_సమితి
                        { $$ = new AstConstraintForeach{$1, $3, $5}; }
        //                      // Non-IEEE extension, soft foreach
        |       ySOFT yFOREACH '(' idClassSelForeach ')' constraint_సమితి
                        { $$ = new AstConstraintForeach{$2, $4, $6, true}; }
        //                      // soft is 1800-2012
        |       yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_సమాసం* స్థిరరాశి newp = new Ast_Constraint_సమాసం{$1, $3};
                          newp->isDisableSoft(true);
                          $$ = newp; }
        //
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

constraint_సమితి<nodep>:  // ==IEEE: constraint_set
                constraint_వ్యక్తీకరణ                   { $$ = $1; }
        |       '{' constraint_వ్యక్తీకరణ_List '}'       { $$ = $2; }
        //
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_వ్యక్తీకరణ_List error '}' { $$ = $2; }  // LCOV_EXCL_LINE
        ;

dist_list<distItemp>:  // ==IEEE: dist_list
                dist_అంశం                               { $$ = $1; }
        |       dist_list ',' dist_అంశం                 { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

dist_అంశం<distItemp>:  // ==IEEE: dist_item + dist_weight
                విలువ_range
                        { $$ = new Ast_Dist_అంశం{$1->fileline(), $1, new AstConst{$1->fileline(), 1}}; }
        |       విలువ_range yP_COLONEQ  సమాసం
                        { $$ = new Ast_Dist_అంశం{$2, $1, $3}; }
        |       విలువ_range yP_COLONDIV సమాసం
                        { $$ = new Ast_Dist_అంశం{$2, $1, $3}; $$->is_సంపూర్ణ(true); }
        //                      // IEEE 1800-2023 added:
        |       yDEFAULT yP_COLONDIV సమాసం
                        { BBUNSUP($<fl>2, "Unsupported: 'default :/' constraint");
                          $$ = nullptr; DEL($3); }
        ;

extern_constraint_declaration<constraintp>:  // ==IEEE: extern_constraint_declaration
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE packageClassScopeE constraintIdNew constraint_ఖండం
                        { $$ = $5; $$->isStatic($1); $$->isExternDef(true);
                          $$->పునాది_Override($3); $$->classOrPackagep($4); $$->కూడు_Itemsp($6); }
        ;

constraintStaticE<cbool>:  // IEEE: part of extern_constraint_declaration
                /*ఖాళీలు*/                             { $$ = false; }
        |       ySTATIC__CONSTRAINT                     { $$ = true; }
        ;

//**********************************************************************
// Constants

సమయం_అంకె_Adjusted<కణుపు_Exprp>:         // Time constant, adjusted to module's time units/precision
                yaTIMENUM
                        { $$ = new Ast_సమయం_Import{$<fl>1, new AstConst{$<fl>1, AstConst::RealDouble{}, $1}}; }
        ;

//**********************************************************************
// Generic tokens

colon<fl>:                      // Generic colon that isn't making a label (e.g. in a case_item)
                ':'                                     { $$ = $1; }
        |       yP_COLON__BEGIN                         { $$ = $1; }
        |       yP_COLON__FORK                          { $$ = $1; }
        ;

//**********************************************************************
// Config - config...endconfig

config_declaration:  // == IEEE: config_declaration
                yCONFIG idAny/*config_identifier*/ ';'
        /*cont*/    configParameterListE design_statement config_నియమం_statement_List_E
        /*cont*/    yENDCONFIG చివర_Label_E
                { AstConfig* స్థిరరాశి newp = new AstConfig{$1, PARSEP->libname(), *$2};
                  newp->కూడు_Designp($5);
                  newp->కూడు_Itemsp($4);
                  newp->కూడు_Itemsp($6);
                  GRAMMARP->చివర_Label($<fl>7, *$2, $8);
                  PARSEP->rootp()->కూడు_Miscsp(newp); }
        ;

configParameterListE<nodep>:  // IEEE: { local_parameter_declaration ';' }
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       configParameterList                     { $$ = $1; }
        ;

configParameterList<nodep>:  // IEEE: part of config_declaration
                configParameter                         { $$ = nullptr; DEL($1); }
        |       configParameterList configParameter     { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

configParameter<nodep>:  // IEEE: part of config_declaration
                parameter_declaration ';'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config localparam declaration"); DEL($1); }
        ;

design_statement<configCellp>:  // == IEEE: design_statement
                yDESIGN configCellList ';'              { $$ = $2; }
        ;

configCellList<configCellp>:  // IEEE: part of design_statement
                configCell                              { $$ = $1; }
        |       configCellList configCell               { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

configCell<configCellp>:  // IEEE: part of design_statement, part of cell_clause
                idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, PARSEP->libname(), *$1}; }
        |       idAny/*library_identifier*/ '.' idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, *$1, *$3}; }
        ;

config_నియమం_statement_List_E<nodep>:  // IEEE: { config_rule }
                /*ఖాళీలు*/                             { $$ = nullptr; }
        |       config_నియమం_statement_List               { $$ = $1; }
        ;

config_నియమం_statement_List<nodep>:  // IEEE: { config_rule }
                config_నియమం_statement                   { $$ = $1; }
        |       config_నియమం_statement_List config_నియమం_statement   { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

config_నియమం_statement<nodep>:  // == IEEE: config_rule_statement
        //                      // IEEE: default_clause
                yDEFAULT liblist_clause ';'
                        { $$ = new Ast_Config_నియమం{$1, nullptr, $2, false}; }
        //                      // IEEE: inst_clause
        |       yINSTANCE inst_name liblist_clause ';'
                        { $$ = new Ast_Config_నియమం{$1, $2, $3, false}; }
        |       yINSTANCE inst_name use_clause ';'
                        { $$ = new Ast_Config_నియమం{$1, $2, $3, false}; }
        //                      // IEEE: cell_clause
        |       yCELL configCell liblist_clause ';'
                        { $$ = new Ast_Config_నియమం{$1, $2, $3, true}; }
        |       yCELL configCell use_clause ';'
                        { $$ = new Ast_Config_నియమం{$1, $2, $3, true}; }
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

inst_name<కణుపు_Exprp>:  // == IEEE: inst_name
                idAnyAsParseRef/*topmodule_identifier*/
                        { $$ = $1; }
        |       idAnyAsParseRef/*topmodule_identifier*/ inst_nameInstanceList
                        { $$ = new AstDot{$<fl>1, false, $1, $2}; }
        ;

inst_nameInstanceList<కణుపు_Exprp>:  // IEEE: part of inst_name
                '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = $2; }
        |       inst_nameInstanceList '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        ;

liblist_clause<nodep>:  // == IEEE: liblist_clause
                yLIBLIST                                { $$ = nullptr; }
        |       yLIBLIST liblistLibraryList             { $$ = $2; }
        ;

liblistLibraryList<కణుపు_Exprp>:  // IEEE: part of liblist_clause
                idAnyAsParseRef/*library_identifier*/
                        { $$ = $1; }
        |       liblistLibraryList idAnyAsParseRef/*library_identifier*/
                        { $$ = కూడు_తదుపరి_Null($1, $2); }
        ;

use_clause<nodep>:  // == IEEE: use_clause
                yUSE idAny/*cell_identifier*/ useAssignmentListE colonConfigE
                        { $$ = new AstConfigUse{$1, "", *$2, $3, $4}; }
        |       yUSE idAny/*library_identifier*/ '.' idAny/*cell_identifier*/ useAssignmentListE colonConfigE
                        { $$ = new AstConfigUse{$1, *$2, *$4, $5, $6}; }
        |       yUSE useAssignmentListE colonConfigE
                        { $$ = new AstConfigUse{$1, "", "", $2, $3}; }
        ;

useAssignmentListE<pinp>:  // IEEE: part of use clause
                /*ఖాళీలు*/                             { $$ = nullptr; }
        //                      // IEEE is missing the '#' '(', but examples need it
        |       '#' '(' ')'                             { $$ = nullptr; }
        |       '#' '(' useAssignmentList ')'           { $$ = $3; }
        ;

useAssignmentList<pinp>:  // IEEE: part of use_clause
                useAssignment                           { $$ = $1; }
        |       useAssignmentList ',' useAssignment     { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;

useAssignment<pinp>:  // IEEE: part of use_clause
        //                      // IEEE: named_parameter_assignment
                '.' idAny/*parameter_identifier*/ '(' ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); }
        |       '.' idAny/*parameter_identifier*/ '(' సమాసం_Or_దత్తాంశం_Type ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); DEL($4); }
        ;

colonConfigE<cbool>:  // IEEE: [ ':' yCONFIG]
                /*ఖాళీలు*/                             { $$ = false; }
        |       ':' yCONFIG                             { $$ = true; }
        ;

//**********************************************************************
// Config - lib.map
//

library_declaration<nodep>:  // IEEE: library_declaration
                yLIBRARY yaSTRING file_మార్గం_spec_List incdirE ';'
                        { $$ = new AstLibrary{$<fl>1, *$2, $3, $4}; }
        ;
incdirE<కణుపు_Exprp>:  // IEEE: [ '-' yINCDIR file_path_specList ';']
                /*ఖాళీలు*/                             { $$ = nullptr; }
                                // https://accellera.mantishub.io/view.php?id=1166
        |       yINCDIR file_మార్గం_spec_List              { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config incdir"); }
        ;
include_statement<nodep>:  // IEEE: include_statement
                yINCLUDE file_మార్గం_spec ';'             { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config include"); }
        ;
file_మార్గం_spec_List<కణుపు_Exprp>:  // IEEE: file_path_spec { ',' file_path_spec }
                file_మార్గం_spec                          { $$ = $1; }
        |       file_మార్గం_spec_List ',' file_మార్గం_spec   { $$ = కూడు_తదుపరి_Null($1, $3); }
        ;
file_మార్గం_spec<కణుపు_Exprp>:  // IEEE: file_path_spec
                yaSTRING { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

//**********************************************************************
// VLT Files

vlt_అంశం:
        //                      // TODO support arbitrary order of arguments
                vltOffFront
                        { V3Control::కూడు_Ignore($1, false, "*", 0, 0); }
        |       vltOffFront vltDFile
                        { V3Control::కూడు_Ignore($1, false, *$2, 0, 0); }
        |       vltOffFront vltDFile y_VLT_D_పంక్తులు yaINTNUM
                        { V3Control::కూడు_Ignore($1, false, *$2, $4->గమ్యం_UInt(), $4->గమ్యం_UInt()); }
        |       vltOffFront vltDFile y_VLT_D_పంక్తులు yaINTNUM '-' yaINTNUM
                        { V3Control::కూడు_Ignore($1, false, *$2, $4->గమ్యం_UInt(), $6->గమ్యం_UInt()); }
        |       vltOffFront vltDFile vltDMatch
                        { ఒకవేళ (($1 == V3_Error_సంకేతం::I_COVERAGE) || ($1 == V3_Error_సంకేతం::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } లేకపోతే {
                              V3Control::కూడు_Ignore_సరిపోలిక($1, *$2, "", *$3);
                          }}
        |       vltOffFront vltDFile vltDContents
                        { ఒకవేళ (($1 == V3_Error_సంకేతం::I_COVERAGE) || ($1 == V3_Error_సంకేతం::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } లేకపోతే {
                              V3Control::కూడు_Ignore_సరిపోలిక($1, *$2, *$3, "*");
                          }}
        |       vltOffFront vltDFile vltDContents vltDMatch
                        { ఒకవేళ (($1 == V3_Error_సంకేతం::I_COVERAGE) || ($1 == V3_Error_సంకేతం::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } లేకపోతే {
                              V3Control::కూడు_Ignore_సరిపోలిక($1, *$2, *$3, *$4);
                          }}
        |       vltOffFront vltDScope
                        { ఒకవేళ ($1 != V3_Error_సంకేతం::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } లేకపోతే {
                              V3Control::కూడు_Scope_Trace_On(false, *$2, 0);
                          }}
        |       vltOffFront vltDScope vltDLevels
                        { ఒకవేళ ($1 != V3_Error_సంకేతం::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } లేకపోతే {
                              V3Control::కూడు_Scope_Trace_On(false, *$2, $3->గమ్యం_UInt());
                          }}
        |       vltOnFront
                        { V3Control::కూడు_Ignore($1, true, "*", 0, 0); }
        |       vltOnFront vltDFile
                        { V3Control::కూడు_Ignore($1, true, *$2, 0, 0); }
        |       vltOnFront vltDFile y_VLT_D_పంక్తులు yaINTNUM
                        { V3Control::కూడు_Ignore($1, true, *$2, $4->గమ్యం_UInt(), $4->గమ్యం_UInt()); }
        |       vltOnFront vltDFile y_VLT_D_పంక్తులు yaINTNUM '-' yaINTNUM
                        { V3Control::కూడు_Ignore($1, true, *$2, $4->గమ్యం_UInt(), $6->గమ్యం_UInt()); }
        |       vltOnFront vltDScope
                        { ఒకవేళ ($1 != V3_Error_సంకేతం::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } లేకపోతే {
                              V3Control::కూడు_Scope_Trace_On(true, *$2, 0);
                          }}
        |       vltOnFront vltDScope vltDLevels
                        { ఒకవేళ ($1 != V3_Error_సంకేతం::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } లేకపోతే {
                              V3Control::కూడు_Scope_Trace_On(true, *$2, $3->గమ్యం_UInt());
                          }}
        |       vltVarAttrFront vltDModuleE vltDFTaskE vltVarAttrSpecE attr_event_controlE
                        { V3Control::కూడు_Var_Attr($<fl>1, *$2, *$3, GRAMMARP->m_vlt_Var_Spec_రకం, *$4, $1, $5); }
        |       vltVarAttrFrontDeprecated vltDModuleE vltDFTaskE vltVarAttrSpecE
                        { /* Historical, now has no effect */ }
        |       vltInlineFront vltDModuleE vltDFTaskE
                        { V3Control::కూడు_Inline($<fl>1, *$2, *$3, $1); }
        |       y_VLT_COVERAGE_ఖండం_OFF vltDFile
                        { V3Control::కూడు_Coverage_ఖండం_Off(*$2, 0); }
        |       y_VLT_COVERAGE_ఖండం_OFF vltDFile y_VLT_D_పంక్తులు yaINTNUM
                        { V3Control::కూడు_Coverage_ఖండం_Off(*$2, $4->గమ్యం_UInt()); }
        |       y_VLT_COVERAGE_ఖండం_OFF vltDModule vltDBlock
                        { V3Control::కూడు_Coverage_ఖండం_Off(*$2, *$3); }
        |       y_VLT_నిండిన_CASE vltDFile
                        { V3Control::కూడు_Case_నిండిన(*$2, 0); }
        |       y_VLT_నిండిన_CASE vltDFile y_VLT_D_పంక్తులు yaINTNUM
                        { V3Control::కూడు_Case_నిండిన(*$2, $4->గమ్యం_UInt()); }
        |       y_VLT_HIER_ఖండం vltDModuleE
                        { V3Control::కూడు_Module_Pragma(*$2, VPragmaType::HIER_ఖండం); }
        |       yVLT_HIER_PARAMS vltDModuleE
                        { V3Control::కూడు_Module_Pragma(*$2, VPragmaType::HIER_PARAMS); }
        |       yVLT_HIER_WORKERS vltDModuleE vltDWorkers
                        { V3Control::కూడు_Hier_Workers($<fl>1, *$2, $3->గమ్యం_SInt()); }
        |       yVLT_HIER_WORKERS vltDHierDpi vltDWorkers
                        { V3Control::కూడు_Hier_Workers($<fl>1, *$2, $3->గమ్యం_SInt()); }
        |       yVLT_PARALLEL_CASE vltDFile
                        { V3Control::కూడు_Case_Parallel(*$2, 0); }
        |       yVLT_PARALLEL_CASE vltDFile y_VLT_D_పంక్తులు yaINTNUM
                        { V3Control::కూడు_Case_Parallel(*$2, $4->గమ్యం_UInt()); }
        |       y_VLT_PROFILE_దత్తాంశం vltDHierDpi vltDCost
                        { V3Control::కూడు_Profile_దత్తాంశం($<fl>1, *$2, $3->గమ్యం_UQuad()); }
        |       y_VLT_PROFILE_దత్తాంశం vltDModel vltDMtask vltDCost
                        { V3Control::కూడు_Profile_దత్తాంశం($<fl>1, *$2, *$3, $4->గమ్యం_UQuad()); }
        |       y_VLT_FSM_రిజిస్టరు_WRAPPER vltDModule vltDFsmD vltDFsmQ vltDFsmClock vltDFsmResetE vlt_DFsm_Reset_విలువ_E
                        { V3Control::కూడు_Fsm_రిజిస్టరు_Wrapper($<fl>1, *$2, *$3, *$4, *$5, *$6, *$7); }
        |       yVLT_VERILATOR_LIB vltDModule
                        { V3Control::కూడు_Module_Pragma(*$2, VPragmaType::VERILATOR_LIB); }
        ;

vltOffFront<errcodeen>:
                yVLT_COVERAGE_OFF                       { $$ = V3_Error_సంకేతం::I_COVERAGE; }
        |       yVLT_TIMING_OFF                         { $$ = V3_Error_సంకేతం::I_TIMING; }
        |       yVLT_TRACING_OFF                        { $$ = V3_Error_సంకేతం::I_TRACING; }
        |       yVLT_LINT_OFF                           { $$ = V3_Error_సంకేతం::I_LINT; }
        |       yVLT_LINT_OFF y_VLT_D_నియమం idAny
                        { $$ = V3_Error_సంకేతం{*$3};
                          ఒకవేళ ($$ == V3_Error_సంకేతం::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltOnFront<errcodeen>:
                yVLT_COVERAGE_ON                        { $$ = V3_Error_సంకేతం::I_COVERAGE; }
        |       yVLT_TIMING_ON                          { $$ = V3_Error_సంకేతం::I_TIMING; }
        |       yVLT_TRACING_ON                         { $$ = V3_Error_సంకేతం::I_TRACING; }
        |       yVLT_LINT_ON                            { $$ = V3_Error_సంకేతం::I_LINT; }
        |       yVLT_LINT_ON y_VLT_D_నియమం idAny
                        { $$ = V3_Error_సంకేతం{*$3};
                          ఒకవేళ ($$ == V3_Error_సంకేతం::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltDBlock<strp>:  // --block <arg>
                y_VLT_D_ఖండం str                        { $$ = $2; }
        ;

vltDContents<strp>:
                yVLT_D_CONTENTS str                     { $$ = $2; }
        ;

vltDCost<nump>:  // --cost <arg>
                y_VLT_D_ఖర్చు yaINTNUM                    { $$ = $2; }
        ;

vltDFile<strp>:  // --file <arg>
                yVLT_D_FILE str                         { $$ = $2; }
        ;

vltDFsmClock<strp>:  // --clock <arg>
                yVLT_D_CLOCK str                        { $$ = $2; }
        ;

vltDFsmD<strp>:  // --d <arg>
                yVLT_D_D str                            { $$ = $2; }
        ;

vltDFsmQ<strp>:  // --q <arg>
                yVLT_D_Q str                            { $$ = $2; }
        ;

vltDFsmResetE<strp>:  // [--reset <arg>]
                /*ఖాళీలు*/                             { స్థిర string ఖాళీలు; $$ = &ఖాళీలు; }
        |       yVLT_D_RESET str                        { $$ = $2; }
        ;

vlt_DFsm_Reset_విలువ_E<strp>:  // [--reset_value <arg>]
                /*ఖాళీలు*/                             { స్థిర string ఖాళీలు; $$ = &ఖాళీలు; }
        |       y_VLT_D_RESET_విలువ str                  { $$ = $2; }
        ;

vltDHierDpi<strp>:  // --hier-dpi <arg>
                yVLT_D_HIER_DPI str                     { $$ = $2; }
        ;

vltDLevels<nump>:  // --levels <arg>
                y_VLT_D_స్థాయిలు yaINTNUM                  { $$ = $2; }
        ;

vltDMatch<strp>:  // --match <arg>
                y_VLT_D_సరిపోలిక str                        { $$ = $2; }
        ;

vltDModel<strp>:  // --model <arg>
                yVLT_D_MODEL str                        { $$ = $2; }
        ;

vltDMtask<strp>:  // --mtask <arg>
                yVLT_D_MTASK str                        { $$ = $2; }
        ;

vltDModule<strp>:  // --module <arg>
                yVLT_D_MODULE str                       { $$ = $2; }
        ;

vltDModuleE<strp>:  // [--module <arg>]
                /*ఖాళీలు*/
                        { స్థిర string unit = "$unit"; $$ = &unit; }  // .vlt uses prettyName
        |       vltDModule
                        { $$ = $1; }
        ;

vltDScope<strp>:  // --scope <arg>
                yVLT_D_SCOPE str                        { $$ = $2; }
        ;

vltDFTaskE<strp>:
                /*ఖాళీలు*/                             { స్థిర string ఖాళీలు; $$ = &ఖాళీలు; }
        |       yVLT_D_FUNCTION str                     { $$ = $2; }
        |       yVLT_D_TASK str                         { $$ = $2; }
        ;

vltDWorkers<nump>:  // --workers <arg>
                yVLT_D_WORKERS yaINTNUM                  { $$ = $2; }
        ;

vltInlineFront<cbool>:
                yVLT_INLINE                             { $$ = true; }
        |       yVLT_NO_INLINE                          { $$ = false; }
        ;

vltVarAttrSpecE<strp>:
                /*ఖాళీలు*/
                        { GRAMMARP->m_vlt_Var_Spec_రకం = V3Control::Var_Spec_రకం::VAR; స్థిర std::string ఖాళీలు; $$ = &ఖాళీలు; }
        |       yVLT_D_PARAM str
                        { GRAMMARP->m_vlt_Var_Spec_రకం = V3Control::Var_Spec_రకం::PARAM; $$ = $2; }
        |       y_VLT_D_పోర్టు str
                        { GRAMMARP->m_vlt_Var_Spec_రకం = V3Control::Var_Spec_రకం::పోర్టు; $$ = $2; }
        |       yVLT_D_VAR str
                        { GRAMMARP->m_vlt_Var_Spec_రకం = V3Control::Var_Spec_రకం::VAR; $$ = $2; }
        ;

vltVarAttrFront<attrtypeen>:
                yVLT_FORCEABLE              { $$ = VAttrType::VAR_FORCEABLE; }
        |       y_VLT_ప్రజా                 { $$ = VAttrType::VAR_ప్రజా; v3Global.dpi(true); }
        |       y_VLT_ప్రజా_FLAT            { $$ = VAttrType::VAR_ప్రజా_FLAT; v3Global.dpi(true); }
        |       y_VLT_ప్రజా_FLAT_RD         { $$ = VAttrType::VAR_ప్రజా_FLAT_RD; v3Global.dpi(true); }
        |       y_VLT_ప్రజా_FLAT_RW         { $$ = VAttrType::VAR_ప్రజా_FLAT_RW; v3Global.dpi(true); }
        |       yVLT_SC_BIGUINT             { $$ = VAttrType::VAR_SC_BIGUINT; }
        |       yVLT_SC_BV                  { $$ = VAttrType::VAR_SC_BV; }
        |       yVLT_SFORMAT                { $$ = VAttrType::VAR_SFORMAT; }
        |       y_VLT_విభజించు_VAR              { $$ = VAttrType::VAR_విభజించు_VAR; }
        ;

vltVarAttrFrontDeprecated:
                yVLT_CLOCK_ENABLE           { }
        |       yVLT_CLOCKER                { }
        |       yVLT_ISOLATE_ASSIGNMENTS    { }
        |       yVLT_NO_CLOCKER             { }
        ;

//**********************************************************************
%%
// For implementation functions see V3ParseGrammar.cpp

//YACC = /kits/sources/bison-2.4.1/src/bison --report=lookahead
// --report=lookahead
// --report=itemset
// --graph
