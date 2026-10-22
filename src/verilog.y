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
#جے_تعریف_شدہ NEVER_JUST_FOR_CLANG_FORMAT
 }
#جے_انت
// clang-format on
#ضمن "V3ParseGrammar.h"  // Defines YYTYPE; before including bison header

#تعریف_کرو YYERROR_VERBOSE 1  // For prior to Bison 3.6
#تعریف_کرو YYINITDEPTH 10000  // Older bisons ignore YYMAXDEPTH
#تعریف_کرو YYMAXDEPTH 10000
#تعریف_کرو YYNEWLINE "\n"

// Pick up new lexer
#تعریف_کرو yylex PARSEP->token_منزل_Bison

#تعریف_کرو BBCOVERIGN(fl, پیغام) (fl)->v3warn(COVERIGN, پیغام)
#تعریف_کرو BBUNSUP(fl, پیغام) (fl)->v3warn(E_UNSUPPORTED, پیغام)
#تعریف_کرو GATEUNSUP(fl, tok) \
    { BBUNSUP((fl), "Unsupported: Verilog 1995 gate primitive: " << (tok)); }
#تعریف_کرو RISEFALLDLYUNSUP(nodep) \
    إذا (nodep->fileline()->timingOn() && v3Global.opt.timing().is_سیٹ_True()) { \
        nodep->v3warn(RISEFALLDLY, "Unsupported: turn-off delays. Ignoring the third delay"); \
    }
#تعریف_کرو MINTYPMAXDLYUNSUP(nodep) \
    إذا (nodep->fileline()->timingOn() && v3Global.opt.timing().is_سیٹ_True()) { \
        nodep->v3warn( \
            MINTYPMAXDLY, \
            "Unsupported: minimum/typical/maximum delay expressions. Using the typical delay"); \
    }
// Apply a delay to all continuous assignments under listp
ساكن فراغ DELAY_LIST(Ast_گرہ* listp, AstDelay* delayp) {
    إذا (!delayp) أرجع;
    لكل (Ast_گرہ* nodep = listp; nodep; nodep = nodep->nextp()) {
        إذا (VN_IS(nodep, Implicit)) استمر;
        AstAlways* ثابت alwaysp = VN_AS(nodep, Always);
        Ast_سونپو_W* ثابت assignp = VN_AS(alwaysp->stmtsp(), سونپو_W);
        assignp->timingControlp(delayp->backp() ? delayp->clone_درخت(false) : delayp);
    }
}
// Apply a strength to all continuous assignments under listp
ساكن فراغ STRENGTH_LIST(Ast_گرہ* listp, AstStrengthSpec* specp) {
    إذا (!specp) أرجع;
    لكل (Ast_گرہ* nodep = listp; nodep; nodep = nodep->nextp()) {
        إذا (VN_IS(nodep, Implicit)) استمر;
        AstAlways* ثابت alwaysp = VN_AS(nodep, Always);
        Ast_سونپو_W* ثابت assignp = VN_AS(alwaysp->stmtsp(), سونپو_W);
        assignp->strengthSpecp(specp->backp() ? specp->clone_درخت(false) : specp);
    }
}
//======================================================================
// Statics (for here only)

#تعریف_کرو PARSEP V3ParseImp::parsep()
#تعریف_کرو GRAMMARP V3ParseGrammar::singletonp()

ثابت VBasicDTypeKwd LOGIC = VBasicDTypeKwd::LOGIC;  // Shorthand "LOGIC"
ثابت VBasicDTypeKwd LOGIC_IMPLICIT = VBasicDTypeKwd::LOGIC_IMPLICIT;

//======================================================================
// Macro functions

// Only use in empty rules, so lines point at beginnings
#تعریف_کرو CRELINE() (PARSEP->bison_آخری_Fileline()->copy_Or_Same_File_سطر_Applied())
#تعریف_کرو FILELINE_OR_CRE(nodep) ((nodep) ? (nodep)->fileline() : CRELINE())

#تعریف_کرو VARRESET_LIST(decl) VARRESET__PVT(decl, 1)  // Start of pinlist
#تعریف_کرو VARRESET_NONLIST(decl) VARRESET__PVT(decl, 0);  // Not in a pinlist
#تعریف_کرو VARRESET__PVT(decl, pin_عدد_شروع) \
    { \
        VARDECL(decl); \
        VARIO(NONE); \
        VARDTYPE_NDECL(nullptr); \
        GRAMMARP->m_pin_عدد = (pin_عدد_شروع); \
        GRAMMARP->m_varLifetime = VLifetime::NONE; \
        GRAMMARP->m_varDeclTyped = false; \
    }
#تعریف_کرو VARDECL(نوع) \
    { GRAMMARP->m_varDecl = VVarType::نوع; }
#تعریف_کرو VARIO(نوع) \
    { GRAMMARP->m_varIO = VDirection::نوع; }
// Set direction to default-input when detect inside an ANSI port list
#تعریف_کرو VARIOANSI() \
    { \
        إذا (GRAMMARP->m_varIO == VDirection::NONE) VARIO(داخلہ); \
    }
#تعریف_کرو VARLIFE(flag) \
    { GRAMMARP->m_varLifetime = flag; }
#تعریف_کرو VARDTYPE(dtypep) \
    { \
        GRAMMARP->سیٹ_DType(dtypep); \
        GRAMMARP->m_varDeclTyped = true; \
    }
#تعریف_کرو VARDTYPE_NDECL(dtypep) \
    { GRAMMARP->سیٹ_DType(dtypep); }  // Port that is range or signed only (not a decl)

#تعریف_کرو VARDONEA(fl, name, array, attrs) GRAMMARP->createVariable((fl), (name), (array), (attrs))
#تعریف_کرو VARDONEP(portp, array, attrs) \
    GRAMMARP->createVariable((portp)->fileline(), (portp)->name(), (array), (attrs))
#تعریف_کرو PINNUMINC() (GRAMMARP->m_pin_عدد++)

#تعریف_کرو GATERANGE(rangep) \
    { GRAMMARP->m_gateRangep = rangep; }

#تعریف_کرو INSTPREP(modfl, modname, paramsp) \
    { \
        GRAMMARP->m_impliedDecl = true; \
        GRAMMARP->m_instModuleFl = modfl; \
        GRAMMARP->m_instModule = modname; \
        GRAMMARP->m_instParamp = paramsp; \
    }

#تعریف_کرو DEL(...) \
    { \
        /* cppcheck-suppress constVariable */ \
        Ast_گرہ* ثابت nodeps[] = {__VA_ARGS__}; \
        لكل (Ast_گرہ* ثابت nodep : nodeps) \
            إذا (nodep) nodep->delete_درخت(); \
    }

ساكن فراغ ERRSVKWD(File_سطر* fileline, ثابت string& tokname) {
    ساكن صحيح s_toldonce = 0;
    fileline->v3error(
        "Unexpected '"s + tokname + "': '" + tokname
        + "' is a SystemVerilog keyword misused as an identifier."
        + (!s_toldonce++ ? "\n" + fileline->warn_ہور()
                               + "... Suggest modify the Verilog-2001 code to avoid SV keywords,"
                               + " or use `begin_keywords or --language."
                         : ""));
}

ساكن فراغ ASSIGNEQEXPR(File_سطر* fileline) {
    fileline->v3warn(ASSIGNEQEXPR,
                     "Assignment '=' inside expression\n"
                         << fileline->warn_ہور()
                         << "... Was a '==' intended, or suggest use a separate statement");
}

ساكن فراغ UNSUPREAL(File_سطر* fileline) {
    fileline->v3warn(SHORTREAL,
                     "Unsupported: shortreal being promoted to real (suggest use real instead)");
}

//======================================================================

فراغ yyerror(ثابت حرف* errmsg) { PARSEP->bison_آخری_Fileline()->v3error(errmsg); }

سانچا <typename T_گرہ, typename T_اگلا>
ساكن T_گرہ* جوڑو_اگلا_Null(T_گرہ* nodep, T_اگلا* nextp) {
    أرجع Ast_گرہ::جوڑو_اگلا_Null<T_گرہ, T_اگلا>(nodep, nextp);
}

//======================================================================

class Ast_Sen_درخت;
// clang-format off
%}

// Bison 3.0 and newer
BISONPRE_نسخہ(3.0,%define parse.error verbose)

// We run bison with the -d argument. This tells it to generate a
// header file with token names. Old versions of bison pasted the
// contents of that file into the generated source as well; newer
// versions just include it.
//
// Since we run bison through ../bisonpre, it doesn't know the correct
// header file name, so we need to tell it.
BISONPRE_نسخہ(3.7,%define api.سرخی.include {"V3ParseBison.h"})

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

%token<strp>            ya_جدول_میدان   "UDP table field"
%token<fl>              ya_جدول_LRSEP   ":"
%token<fl>              ya_جدول_LINEEND "UDP table line end"

%token<strp>            yaSCCTOR        "`systemc_ctor block"
%token<strp>            yaSCDTOR        "`systemc_dtor block"
%token<strp>            yaSCHDR         "`systemc_header block"
%token<strp>            yaSCHDRP        "`systemc_header_post block"
%token<strp>            yaSCIMP         "`systemc_implementation block"
%token<strp>            yaSCIMPH        "`systemc_imp_header block"
%token<strp>            yaSCINT         "`systemc_interface block"

%token<fl>              yVLT_CLOCKER                "clocker"
%token<fl>              yVLT_CLOCK_ENABLE           "clock_enable"
%token<fl>              y_VLT_COVERAGE_بلاک_OFF     "coverage_block_off"
%token<fl>              yVLT_COVERAGE_OFF           "coverage_off"
%token<fl>              yVLT_COVERAGE_ON            "coverage_on"
%token<fl>              yVLT_FORCEABLE              "forceable"
%token<fl>              y_VLT_FSM_رجسٹر_WRAPPER   "fsm_register_wrapper"
%token<fl>              y_VLT_بھریا_CASE              "full_case"
%token<fl>              y_VLT_HIER_بلاک             "hier_block"
%token<fl>              yVLT_HIER_PARAMS            "hier_params"
%token<fl>              yVLT_HIER_WORKERS           "hier_workers"
%token<fl>              yVLT_INLINE                 "inline"
%token<fl>              yVLT_ISOLATE_ASSIGNMENTS    "isolate_assignments"
%token<fl>              yVLT_LINT_OFF               "lint_off"
%token<fl>              yVLT_LINT_ON                "lint_on"
%token<fl>              yVLT_NO_CLOCKER             "no_clocker"
%token<fl>              yVLT_NO_INLINE              "no_inline"
%token<fl>              yVLT_PARALLEL_CASE          "parallel_case"
%token<fl>              y_VLT_PROFILE_انکڑے           "profile_data"
%token<fl>              y_VLT_عوامی                 "public"
%token<fl>              y_VLT_عوامی_FLAT            "public_flat"
%token<fl>              y_VLT_عوامی_FLAT_RD         "public_flat_rd"
%token<fl>              y_VLT_عوامی_FLAT_RW         "public_flat_rw"
%token<fl>              y_VLT_عوامی_MODULE          "public_module"
%token<fl>              yVLT_SC_BIGUINT             "sc_biguint"
%token<fl>              yVLT_SC_BV                  "sc_bv"
%token<fl>              yVLT_SFORMAT                "sformat"
%token<fl>              y_VLT_ونڈو_VAR              "split_var"
%token<fl>              yVLT_TIMING_OFF             "timing_off"
%token<fl>              yVLT_TIMING_ON              "timing_on"
%token<fl>              yVLT_TRACING_OFF            "tracing_off"
%token<fl>              yVLT_TRACING_ON             "tracing_on"
%token<fl>              yVLT_VERILATOR_LIB          "verilator_lib"

%token<fl>              y_VLT_D_بلاک    "--block"
%token<fl>              yVLT_D_CONTENTS "--contents"
%token<fl>              y_VLT_D_لاگت     "--cost"
%token<fl>              yVLT_D_CLOCK    "--clock"
%token<fl>              yVLT_D_D        "--d"
%token<fl>              yVLT_D_FILE     "--file"
%token<fl>              yVLT_D_FUNCTION "--function"
%token<fl>              yVLT_D_HIER_DPI "--hier-dpi"
%token<fl>              y_VLT_D_پدھراں   "--levels"
%token<fl>              y_VLT_D_سطریں    "--lines"
%token<fl>              y_VLT_D_میل    "--match"
%token<fl>              yVLT_D_MODEL    "--model"
%token<fl>              yVLT_D_MODULE   "--module"
%token<fl>              yVLT_D_MTASK    "--mtask"
%token<fl>              yVLT_D_PARAM    "--param"
%token<fl>              y_VLT_D_پورٹ     "--port"
%token<fl>              y_VLT_D_قاعدہ     "--rule"
%token<fl>              yVLT_D_Q        "--q"
%token<fl>              yVLT_D_RESET    "--reset"
%token<fl>              y_VLT_D_RESET_قدر "--reset_value"
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
%token<fl>              y1_قدم          "1step"
%token<fl>              yACCEPT_ON      "accept_on"
%token<fl>              yALIAS          "alias"
%token<fl>              yALWAYS         "always"
%token<fl>              yALWAYS_COMB    "always_comb"
%token<fl>              yALWAYS_FF      "always_ff"
%token<fl>              yALWAYS_LATCH   "always_latch"
%token<fl>              yAND            "and"
%token<fl>              yASSERT         "assert"
%token<fl>              y_سونپو         "assign"
%token<fl>              yASSUME         "assume"
%token<fl>              yAUTOMATIC      "automatic"
%token<fl>              yBEFORE         "before"
%token<fl>              yBEGIN          "begin"
%token<fl>              yBIND           "bind"
%token<fl>              yBINS           "bins"
%token<fl>              yBINSOF         "binsof"
%token<fl>              y_ثنائی_ہندسہ            "bit"
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
%token<fl>              y_کنارا           "edge"
%token<fl>              yELSE           "else"
%token<fl>              y_سرا            "end"
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
%token<fl>              y_پہلا_میل    "first_match"
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
%token<fl>              y_سٹا_دیندا        "implies"
%token<fl>              yIMPORT         "import"
%token<fl>              yINCDIR         "incdir"
%token<fl>              yINCLUDE        "include"
%token<fl>              yINITIAL        "initial"
%token<fl>              yINOUT          "inout"
%token<fl>              y_داخلہ          "input"
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
%token<fl>              y_ترجیح       "priority"
%token<fl>              y_پروگرام        "program"
%token<fl>              yPROPERTY       "property"
%token<fl>              yPROTECTED      "protected"
%token<fl>              yPULL0          "pull0"
%token<fl>              yPULL1          "pull1"
%token<fl>              yPULLDOWN       "pulldown"
%token<fl>              yPULLUP         "pullup"
%token<fl>              y_خالص           "pure"
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
%token<fl>              y_جدول          "table"
%token<fl>              yTAGGED         "tagged"
%token<fl>              yTAGGED__LEX    "tagged-in-lex"
%token<fl>              yTAGGED__NONPRIMARY   "tagged-nonprimary"
%token<fl>              yTASK           "task"
%token<fl>              yTHIS           "this"
%token<fl>              yTHROUGHOUT     "throughout"
%token<fl>              y_ویلا           "time"
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
%token<fl>              y_WAIT_ترتیب     "wait_order"
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
%token<fl>              y_D_بٹ         "$bits"
%token<fl>              yD_BITSTOREAL   "$bitstoreal"
%token<fl>              yD_BITSTOSHORTREAL "$bitstoshortreal"
%token<fl>              yD_C            "$c"
%token<fl>              yD_CPURE        "$cpure"
%token<fl>              yD_CAST         "$cast"
%token<fl>              yD_CEIL         "$ceil"
%token<fl>              y_D_بدلیا      "$changed"
%token<fl>              y_D_بدلیا_GCLK "$changed_gclk"
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
%token<fl>              y_D_DIST_CHI_مربع "$dist_chi_square"
%token<fl>              yD_DIST_ERLANG  "$dist_erlang"
%token<fl>              y_D_DIST_قوت_نمائی "$dist_exponential"
%token<fl>              y_D_DIST_عام  "$dist_normal"
%token<fl>              y_D_DIST_پواسوں "$dist_poisson"
%token<fl>              yD_DIST_T       "$dist_t"
%token<fl>              y_D_DIST_اکسار "$dist_uniform"
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
%token<fl>              y_D_انت       "$finish"
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
%token<fl>              y_D_GET_INITIAL_RANDOM_بیج "$get_initial_random_seed"
%token<fl>              yD_GLOBAL_CLOCK "$global_clock"
%token<fl>              y_D_بلند         "$high"
%token<fl>              yD_HYPOT        "$hypot"
%token<fl>              yD_INCREMENT    "$increment"
%token<fl>              yD_INFERRED_DISABLE "$inferred_disable"
%token<fl>              yD_INFO         "$info"
%token<fl>              yD_ISUNBOUNDED  "$isunbounded"
%token<fl>              yD_ISUNKNOWN    "$isunknown"
%token<fl>              yD_ITOR         "$itor"
%token<fl>              y_D_بایاں         "$left"
%token<fl>              yD_LN           "$ln"
%token<fl>              yD_LOG10        "$log10"
%token<fl>              y_D_کم          "$low"
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
%token<fl>              y_D_دایاں        "$right"
%token<fl>              yD_RISING_GCLK  "$rising_gclk"
%token<fl>              y_D_جڑ         "$root"
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
%token<fl>              y_D_ناپ         "$size"
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
%token<fl>              y_D_ویلا         "$time"
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
%token<fl>              y_VL_COVERAGE_بلاک_OFF    "/*verilator coverage_block_off*/"
%token<fl>              yVL_DPI_C_DECL            "/*verilator dpi_c_decl*/"
%token<fl>              yVL_FORCEABLE             "/*verilator forceable*/"
%token<fl>              y_VL_بھریا_CASE             "/*verilator full_case*/"
%token<fl>              y_VL_HIER_بلاک            "/*verilator hier_block*/"
%token<fl>              yVL_INLINE_MODULE         "/*verilator inline_module*/"
%token<fl>              yVL_ISOLATE_ASSIGNMENTS   "/*verilator isolate_assignments*/"
%token<fl>              yVL_NO_CLOCKER            "/*verilator no_clocker*/"
%token<fl>              yVL_NO_INLINE_MODULE      "/*verilator no_inline_module*/"
%token<fl>              yVL_NO_INLINE_TASK        "/*verilator no_inline_task*/"
%token<fl>              yVL_PARALLEL_CASE         "/*verilator parallel_case*/"
%token<fl>              y_VL_عوامی                "/*verilator public*/"
%token<fl>              y_VL_عوامی_FLAT           "/*verilator public_flat*/"
%token<fl>              y_VL_عوامی_FLAT_ON        "/*verilator public_flat_on*/"
%token<fl>              y_VL_عوامی_FLAT_RD        "/*verilator public_flat_rd*/"
%token<fl>              y_VL_عوامی_FLAT_RD_ON     "/*verilator public_flat_rd_on*/"
%token<fl>              y_VL_عوامی_FLAT_RW        "/*verilator public_flat_rw*/"
%token<fl>              y_VL_عوامی_FLAT_RW_ON     "/*verilator public_flat_rw_on*/"
%token<fl>              y_VL_عوامی_FLAT_RW_ON_SNS "/*verilator public_flat_rw_on_sns*/"
%token<fl>              y_VL_عوامی_ON             "/*verilator public_on*/"
%token<fl>              y_VL_عوامی_OFF            "/*verilator public_off*/"
%token<fl>              y_VL_عوامی_MODULE         "/*verilator public_module*/"
%token<fl>              yVL_SC_BIGUINT            "/*verilator sc_biguint*/"
%token<fl>              yVL_SC_BV                 "/*verilator sc_bv*/"
%token<fl>              yVL_SFORMAT               "/*verilator sformat*/"
%token<fl>              y_VL_ونڈو_VAR             "/*verilator split_var*/"
%token<fl>              yVL_FSM_ARC_INCL_COND     "/*verilator fsm_arc_include_cond*/"
%token<fl>              yVL_FSM_ARC_INCL_COND_AUTO        "/*verilator fsm_arc_include_cond_auto*/"
%token<fl>              yVL_FSM_ARC_INCL_COND_AUTO_EXPAND "/*verilator fsm_arc_include_cond_auto_expand*/"
%token<fl>              y_VL_FSM_ARC_INCL_COND_بھریا        "/*verilator fsm_arc_include_cond_full*/"
%token<fl>              yVL_FSM_STATE             "/*verilator fsm_state*/"
%token<fl>              yVL_FSM_STATE_AUTO        "/*verilator fsm_state_auto*/"
%token<fl>              yVL_FSM_STATE_AUTO_EXPAND "/*verilator fsm_state_auto_expand*/"
%token<fl>              y_VL_FSM_STATE_بھریا        "/*verilator fsm_state_full*/"
%token<strp>            y_VL_ٹیگ                   "/*verilator tag*/"
%token<fl>              yVL_UNROLL_DISABLE        "/*verilator unroll_disable*/"
%token<fl>              y_VL_UNROLL_بھریا           "/*verilator unroll_full*/"

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
%کھبے           yP_ANDANDAND
%کھبے           yMATCHES
%کھبے           prTAGGED
//UNSUP %left   prSEQ_CLOCKING

// PSL op precedence

// Lowest precedence
// These are in IEEE 17.7.1
%nonassoc       prALWAYS prS_ALWAYS prEVENTUALLY prS_EVENTUALLY prIF prACCEPT_ON prREJECT_ON prSYNC_ACCEPT_ON prSYNC_REJECT_ON

%سجے          yP_ORMINUSGT yP_OREQGT yP_POUNDMINUSPD yP_POUNDEQPD
%سجے          yUNTIL yS_UNTIL yUNTIL_WITH yS_UNTIL_WITH y_سٹا_دیندا
%سجے          yIFF
%کھبے           yOR
%کھبے           yAND
%nonassoc       yNOT yNEXTTIME yS_NEXTTIME
%کھبے           yINTERSECT
%کھبے           yWITHIN
%سجے          yTHROUGHOUT
%کھبے           prPOUNDPOUND_MULTI
%کھبے           yP_POUNDPOUND
%کھبے           yP_BRASTAR yP_BRAEQ yP_BRAMINUSGT yP_BRAPLUSKET

// Not specified, but needed higher than yOR, lower than normal non-pexpr expressions
//UNSUP %left           yPOSEDGE yNEGEDGE yEDGE

//UNSUP %left           '{' '}'

// Verilog op precedence
%سجے          yP_MINUSGT yP_LTMINUSGT
%سجے          '?' ':' yP_COLON__BEGIN yP_COLON__FORK
%کھبے           yP_OROR
%کھبے           yP_ANDAND
%کھبے           '|' yP_NOR
%کھبے           '^' yP_XNOR
%کھبے           '&' yP_NAND
%کھبے           yP_EQUAL yP_NOTEQUAL yP_CASEEQUAL yP_CASENOTEQUAL yP_WILDEQUAL yP_WILDNOTEQUAL
%کھبے           '>' '<' yP_GTE yP_LTE yP_LTE__IGNORE yINSIDE yDIST
%کھبے           yP_SLEFT yP_SRIGHT yP_SSRIGHT
%کھبے           '+' '-'
%کھبے           '*' '/' '%'
%کھبے           yP_POW
%کھبے           prUNARYARITH yP_MINUSMINUS yP_PLUSPLUS prREDUCTION prNEGATION
%کھبے           '.'
// Not in IEEE, but need to avoid conflicts; TICK should bind tightly just lower than COLONCOLON
%کھبے           yP_TICK
//%left         '(' ')' '[' ']' yP_COLONCOLON '.'

%nonassoc pr_ہیٹھلا_THAN_ELSE
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

%start سروت_متن

%%
//**********************************************************************
// Files

سروت_متن:                    // ==IEEE: source_text
                /*خالی_تھاواں*/                             { }
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
        |       library_declaration                     { PARSEP->rootp()->جوڑو_Miscsp($1); }
        |       include_statement                       { }
        |       interface_declaration                   { }
        |       پروگرام_declaration                     { }
        |       package_declaration                     { }
        |       package_شے_چوٹی                         { إذا ($1) PARSEP->unitPackage($1->fileline())->جوڑو_Stmtsp($1); }
        |       bind_directive                          { إذا ($1) PARSEP->unitPackage($1->fileline())->جوڑو_Stmtsp($1); }
        |       config_declaration                      { }
        //                      // Verilator only
        |       yaT_RESETALL                            { }  // Else, under design, and illegal based on IEEE 22.3
        |       yaT_NOUNCONNECTED                       { PARSEP->unconnectedDrive(VOptionBool::OPT_DEFAULT_FALSE); }
        |       yaT_UNCONNECTED_PULL0                   { PARSEP->unconnectedDrive(VOptionBool::OPT_FALSE); }
        |       yaT_UNCONNECTED_PULL1                   { PARSEP->unconnectedDrive(VOptionBool::OPT_TRUE); }
        |       vlt_شے                                 { }
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
                packageFront package_شے_List_E yENDPACKAGE سرا_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          إذا ($2) $1->جوڑو_Stmtsp($2);
                          GRAMMARP->سرا_Label($<fl>4, $1, $4); }
        ;

packageFront<گرہ_Modulep>:
                yPACKAGE lifetimeE idAny ';'
                        { $$ = new AstPackage{$<fl>3, *$3, PARSEP->libname()};
                          إذا ($$->name() == "std") {
                              إذا ($$->fileline()->filename() != V3Options::get_Std_Package_رستہ()) {
                                  $$->v3error("Redeclaring the 'std' package is not allowed");
                              } وإلا {
                                  PARSEP->rootp()->stdPackagep(VN_AS($$, Package));
                              }
                          }
                          $$->وچ_Library(true);  // packages are always libraries; don't want to make them a "top"
                          $$->lifetime($2);
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ویلا_آخری_Unit());
                          PARSEP->rootp()->timeprecision_ملاؤ($$->fileline(),
                                                              PARSEP->ویلا_آخری_Prec());
                          PARSEP->rootp()->جوڑو_Modulesp($$); }
        ;

package_شے_List_E<nodep>:       // IEEE: [{ package_item }]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       package_شے_List                        { $$ = $1; }
        ;

package_شے_List<nodep>:        // IEEE: { package_item }
                package_شے                            { $$ = $1; }
        |       package_شے_List package_شے           { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

package_شے<nodep>:            // ==IEEE: package_item
                package_or_generate_شے_declaration    { $$ = $1; }
        |       anonymous_پروگرام                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_شے_چوٹی<nodep>:         // ==IEEE: package_item

                package_or_generate_شے_decl_No_Checker  { $$ = $1; }
        |       checker_declaration
                       { PARSEP->rootp()->جوڑو_Modulesp($1);
                         $$ = nullptr; }
        |       anonymous_پروگرام                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_or_generate_شے_declaration<nodep>:    // ==IEEE: package_or_generate_item_declaration
                package_or_generate_شے_decl_No_Checker  { $$ = $1; }
        |       checker_declaration
                        { $1->v3warn(E_UNSUPPORTED, "Unsupported: 'checker' below unit-level");
                          PARSEP->rootp()->جوڑو_Modulesp($1);
                          $$ = nullptr; }
        ;

package_or_generate_شے_decl_No_Checker<nodep>:
                net_declaration                         { $$ = $1; }
        |       انکڑے_declaration                        { $$ = $1; }
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
        |       assertion_شے_declaration              { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

package_import_declarationList<nodep>:
                package_import_declaration              { $$ = $1; }
        |       package_import_declarationList package_import_declaration
                        { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

package_import_declaration<nodep>:      // ==IEEE: package_import_declaration
                yIMPORT package_import_شے_List ';'     { $$ = $2; }
        ;

package_import_شے_List<nodep>:
                package_import_شے                     { $$ = $1; }
        |       package_import_شے_List ',' package_import_شے { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

package_import_شے<nodep>:     // ==IEEE: package_import_item
                idCC/*package_identifier*/ yP_COLONCOLON package_import_شے_Obj
                        { $$ = new AstPackageImport{$<fl>1, *$<strp>1, *$3}; }
        ;

package_import_شے_Obj<strp>:   // IEEE: part of package_import_item
                idAny/*package_identifier*/             { $<fl>$ = $<fl>1; $$ = $1; }
        |       '*'                                     { $<fl>$ = $<fl>1; ساكن string تارا = "*"; $$ = &تارا; }
        ;

package_export_declaration<nodep>: // IEEE: package_export_declaration
                yEXPORT '*' yP_COLONCOLON '*' ';'
                        { $$ = new Ast_Package_Export_تارا_تارا{$<fl>2}; }
        |       yEXPORT package_export_شے_List ';'     { $$ = $2; }
        ;

package_export_شے_List<nodep>:
                package_export_شے                     { $$ = $1; }
        |       package_export_شے_List ',' package_export_شے { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

package_export_شے<nodep>:     // ==IEEE: package_export_item
                idCC yP_COLONCOLON package_import_شے_Obj
                        { $$ = new AstPackageExport{$<fl>3, *$<strp>1, *$3}; }
        ;

//**********************************************************************
// Module headers

module_declaration:             // ==IEEE: module_declaration
        //                      // timeunits_declaration instead in module_item
        //                      // IEEE: module_nonansi_header + module_ansi_header
                modFront importsAndParametersE ports_تارا_E ';'
        /*cont*/    module_شے_List_E yENDMODULE سرا_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          إذا ($2) $1->جوڑو_Stmtsp($2);
                          إذا ($3) $1->جوڑو_Stmtsp($3);
                          إذا ($5) $1->جوڑو_Stmtsp($5);
                          GRAMMARP->سرا_Label($<fl>7, $1, $7); }
        |       udpFront ports_تارا_E ';'
        /*cont*/    module_شے_List_E yENDPRIMITIVE سرا_Label_E
                        { $1->modTrace(false);  // Stash for implicit wires, etc
                          إذا ($2) $1->جوڑو_Stmtsp($2);
                          إذا ($4) $1->جوڑو_Stmtsp($4);
                          GRAMMARP->m_tracingParse = true;
                          GRAMMARP->سرا_Label($<fl>6, $1, $6); }
        //
        |       yEXTERN modFront parameter_پورٹ_list_E ports_تارا_E ';'
                        { DEL($2->unlink_Fr_پٹھ()); }
                        // We allow modules to be declared after instantiations, so harmless
        ;

modFront<گرہ_Modulep>:
        //                      // General note: all *Front functions must call symPushNew before
        //                      // any formal arguments, as the arguments must land in the new scope.
                yMODULE lifetimeE idAny
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname()};
                          $$->lifetime($2);
                          $$->وچ_Library(PARSEP->وچ_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ویلا_آخری_Unit());
                          PARSEP->rootp()->timeprecision_ملاؤ($$->fileline(),
                                                              PARSEP->ویلا_آخری_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive());
                          PARSEP->rootp()->جوڑو_Modulesp($$); }
        |       modFront sigAttrScope                   { $$ = $1; }
        ;

importsAndParametersE<nodep>:   // IEEE: common part of module_declaration, interface_declaration, program_declaration
        //                      // { package_import_declaration } [ parameter_port_list ]
                parameter_پورٹ_list_E
                        { $$ = $1; $<flag>$ = $<flag>1; }  // hasParameterList
        |       package_import_declarationList parameter_پورٹ_list_E
                        { $$ = جوڑو_اگلا_Null($1, $2);
                          $<flag>$ = $<flag>2; }  // hasParameterList
        ;

udpFront<گرہ_Modulep>:
                yPRIMITIVE lifetimeE idAny
                        { $$ = new AstPrimitive{$<fl>3, *$3, PARSEP->libname()};
                          $$->وچ_Library(true);
                          $$->lifetime($2);
                          $$->modTrace(false);
                          $$->جوڑو_Stmtsp(new AstPragma{$<fl>3, VPragmaType::INLINE_MODULE});
                          GRAMMARP->m_tracingParse = false;
                          PARSEP->rootp()->جوڑو_Modulesp($$); }
        ;

parameter_قدر_assignment_Inst_E<pinp>:      // IEEE: [ parameter_value_assignment ] for instance
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       parameter_قدر_assignment_Inst          { $$ = $1; }
        ;

parameter_قدر_assignment_Class_E<pinp>:      // IEEE: [ parameter_value_assignment ] for classes
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       parameter_قدر_assignment_Class         { $$ = $1; }
        ;

parameter_قدر_assignment_Inst<pinp>:       // IEEE: parameter_value_assignment for instance
                '#' '(' instParamListE ')'              { $$ = $3; }
        //                      // Parentheses are optional around a single parameter
        //                      // IMPORTANT: Below hardcoded in tokenPipeScanParam
        |       '#' yaINTNUM                            { $$ = new AstPin{$<fl>2, 1, "", new AstConst{$<fl>2, *$2}}; }
        |       '#' yaFLOATNUM                          { $$ = new AstPin{$<fl>2, 1, "",
                                                                          new AstConst{$<fl>2, AstConst::RealDouble{}, $2}}; }
        |       '#' ویلا_عدد_Adjusted                     { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        |       '#' idClassSel                          { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        //                      // Not needed in Verilator:
        //                      // Side effect of combining *_instantiations
        //                      // '#' delay_value      { UNSUP }
        ;

parameter_قدر_assignment_Class<pinp>:  // IEEE: parameter_value_assignment (for classes)
        //                      // Like parameter_value_assignment, but for classes only, which always have #()
                '#' '(' instParamListE ')'              { $$ = $3; }
        ;

parameter_پورٹ_list_E<nodep>:    // IEEE: parameter_port_list + empty == parameter_value_assignment
                /*خالی_تھاواں*/                             { $$ = nullptr; $<flag>$ = false; }  // hasParameterList
        |       '#' '(' ')'                             { $$ = nullptr;
                                                          $<flag>$ = true; }  // hasParameterList
        //                      // IEEE: '#' '(' list_of_param_assignments { ',' parameter_port_declaration } ')'
        //                      // IEEE: '#' '(' parameter_port_declaration { ',' parameter_port_declaration } ')'
        //                      // Can't just do that as "," conflicts with between vars and between stmts, so
        //                      // split into pre-comma and post-comma parts
        |       '#' '('                                 { VARRESET_LIST(GPARAM);
                                                          GRAMMARP->m_pinAnsi = true; }
        /*cont*/    param_پورٹ_Decl_Or_Arg_List ')'          { $$ = $4;
                                                          $<flag>$ = true;  // hasParameterList
                                                          VARRESET_NONLIST(UNKNOWN);
                                                          GRAMMARP->m_pinAnsi = false; }
        //                      // Note legal to start with "a=b" with no parameter statement
        ;

param_پورٹ_Decl_Or_Arg_List<nodep>:  // IEEE: list_of_param_assignments + { parameter_port_declaration }
                param_پورٹ_Decl_Or_Arg                              { $$ = $1; }
        |       param_پورٹ_Decl_Or_Arg_List ',' param_پورٹ_Decl_Or_Arg   { $$ = $1->جوڑو_اگلا($3); }
        |       param_پورٹ_Decl_Or_Arg_List sigAttrScope             { $$ = $1; }
        ;

param_پورٹ_Decl_Or_Arg<nodep>:      // IEEE: param_assignment + parameter_port_declaration
        //                      // We combine the two as we can't tell which follows a comma
                param_پورٹ_Decl_Or_Arg_Sub                   { $$ = $1; }
        |       vl_ٹیگ                                   { $$ = nullptr; }
        ;

param_پورٹ_Decl_Or_Arg_Sub<nodep>:
                parameter_پورٹ_declaration_Front_E param_assignment       { $$ = $2; }
        |       parameter_پورٹ_declaration_Type_Front_E type_assignment    { $$ = $2; }
        |       sigAttrScope param_پورٹ_Decl_Or_Arg_Sub                      { $$ = $2; }
        ;

ports_تارا_E<nodep>:              // IEEE: .* + list_of_ports + list_of_port_declarations + empty
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        //                      // .* expanded from module_declaration
        //UNSUP '(' yP_DOTSTAR ')'                              { UNSUP }
        |       '('
        /*mid*/         { VARRESET_LIST(پورٹ); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    list_دا_ports ')'
                       { $$ = $3; VARRESET_NONLIST(UNKNOWN); GRAMMARP->m_pinAnsi = false; }
        ;

list_دا_ports_E<nodep>:          // IEEE: [ list_of_ports + list_of_port_declarations ]
                پورٹ_And_ٹیگ_E                             { $$ = $1; }
        |       list_دا_ports_E ',' پورٹ_And_ٹیگ_E          { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

list_دا_ports<nodep>:           // IEEE: list_of_ports + list_of_port_declarations
                پورٹ_And_ٹیگ                              { $$ = $1; }
        |       list_دا_ports_E ',' پورٹ_And_ٹیگ_E          { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

پورٹ_And_ٹیگ_E<nodep>:
                /*خالی_تھاواں*/
                        { ثابت صحيح p = PINNUMINC();
                          ثابت string name = "__pinNumber" + cvt_منزل_Str(p);
                          $$ = new Ast_پورٹ{CRELINE(), p, name};
                          AstVar* varp = new AstVar{CRELINE(), VVarType::WIRE, name, VFlag_بچہ_DType{},
                                                    new AstBasicDType{CRELINE(), LOGIC_IMPLICIT}};
                          varp->declDirection(VDirection::داخلہ);
                          varp->direction(VDirection::داخلہ);
                          varp->ansi(false);
                          varp->declTyped(true);
                          varp->trace(false);
                          $$ = $$->جوڑو_اگلا(varp);
                          $$->v3warn(NULLPORT, "Null port on module (perhaps extraneous comma)"); }
        |       پورٹ_And_ٹیگ                              { $$ = $1; }
        |       پورٹ_And_ٹیگ sigAttrScope                 { $$ = $1; }
        ;

پورٹ_And_ٹیگ<nodep>:
                پورٹ_3                                    { $$ = $1; }
        |       vl_ٹیگ پورٹ_3                              { $$ = $2; }  // Tag will associate with previous port
        |       sigAttrScope پورٹ_And_ٹیگ                 { $$ = $2; }
        ;

پورٹ_3<nodep>:                    // ==IEEE: port
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
                پورٹ_Dir_Net_E id/*interface*/ '.' idAny/*modport*/ پورٹ_Sig variable_dimensionListE sigAttrListE
                        { // VAR for now, but V3LinkCells may call setIfcaeRef on it later
                          $$ = $5; VARDECL(VAR); VARIO(NONE);
                          Ast_گرہ_DType* ثابت dtp = new AstIfaceRefDType{$<fl>2, $<fl>4, "", *$2, *$4};
                          VARDTYPE(dtp); VARIOANSI();
                          جوڑو_اگلا_Null($$, VARDONEP($$, $6, $7)); }
        |       پورٹ_Dir_Net_E yINTERFACE                           پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; GRAMMARP->createGenericIface($3, $4, $5); }
        |       پورٹ_Dir_Net_E yINTERFACE      '.' idAny/*modport*/ پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5; GRAMMARP->createGenericIface($5, $6, $7, $<fl>4, *$4); }
        //
        |       پورٹ_Dir_Net_E yINTERCONNECT signingE rangeListE پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5;
                          BBUNSUP($<fl>2, "Unsupported: interconnect");
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); VARIOANSI();
                          جوڑو_اگلا_Null($$, VARDONEP($$, $6, $7)); }
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
        |       پورٹ_Dir_Net_E انکڑے_type           پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          جوڑو_اگلا_Null($$, VARDONEP($$, $4, $5)); }
        |       پورٹ_Dir_Net_E انکڑے_type           پورٹ_Sig variable_dimensionListE sigAttrListE '=' const_اظہاریہ
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          إذا (AstVar* vp = VARDONEP($$, $4, $5)) { جوڑو_اگلا_Null($$, vp); vp->valuep($7); } }
        |       پورٹ_Dir_Net_E yVAR انکڑے_type      پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          جوڑو_اگلا_Null($$, VARDONEP($$, $5, $6)); }
        |       پورٹ_Dir_Net_E yVAR انکڑے_type      پورٹ_Sig variable_dimensionListE sigAttrListE '=' const_اظہاریہ
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          إذا (AstVar* vp = VARDONEP($$, $5, $6)) { جوڑو_اگلا_Null($$, vp); vp->valuep($8); } }
        |       پورٹ_Dir_Net_E yVAR implicit_typeE پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          جوڑو_اگلا_Null($$, VARDONEP($$, $5, $6)); }
        |       پورٹ_Dir_Net_E yVAR implicit_typeE پورٹ_Sig variable_dimensionListE sigAttrListE '=' const_اظہاریہ
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          إذا (AstVar* vp = VARDONEP($$, $5, $6)) { جوڑو_اگلا_Null($$, vp); vp->valuep($8); } }
        |       پورٹ_Dir_Net_E signing             پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3;
                          Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          جوڑو_اگلا_Null($$, VARDONEP($$, $4, $5)); }
        |       پورٹ_Dir_Net_E signing             پورٹ_Sig variable_dimensionListE sigAttrListE '=' const_اظہاریہ
                        { $$ = $3;
                          Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          إذا (AstVar* vp = VARDONEP($$, $4, $5)) { جوڑو_اگلا_Null($$, vp); vp->valuep($7); } }
        |       پورٹ_Dir_Net_E signingE rangeList  پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4;
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          جوڑو_اگلا_Null($$, VARDONEP($$, $5, $6)); }
        |       پورٹ_Dir_Net_E signingE rangeList  پورٹ_Sig variable_dimensionListE sigAttrListE '=' const_اظہاریہ
                        { $$ = $4;
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          إذا (AstVar* vp = VARDONEP($$, $5, $6)) { جوڑو_اگلا_Null($$, vp); vp->valuep($8); } }
        |       پورٹ_Dir_Net_E /*implicit*/        پورٹ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $2; /*VARDTYPE-same*/ جوڑو_اگلا_Null($$, VARDONEP($$, $3, $4)); }
        |       پورٹ_Dir_Net_E /*implicit*/        پورٹ_Sig variable_dimensionListE sigAttrListE '=' const_اظہاریہ
                        { $$ = $2; /*VARDTYPE-same*/
                          إذا (AstVar* vp = VARDONEP($$, $3, $4)) { جوڑو_اگلا_Null($$, vp); vp->valuep($6); } }
        //                      //   IEEE: '.' port_identifier '(' [ expression ] ')'
        |       پورٹ_Dir_Net_E /*implicit*/ '.' پورٹ_Sig '(' اظہاریہ ')'
                        { $$ = $3; DEL($5);
                          BBUNSUP($<fl>2, "Unsupported: complex ports (IEEE 1800-2023 23.2.2.1/2)"); }
        //                      // IEEE: part of (non-ansi) port_reference
        |       '{' پورٹ_عبارت_List '}'
                        { $$ = $2; }
        ;

پورٹ_Dir_Net_E:                    // IEEE: part of port, optional net type and/or direction
                /*خالی_تھاواں*/                             { }
        //                      // Per spec, if direction given default the nettype.
        //                      // The higher level rule may override this VARDTYPE with one later in the parse.
        |       پورٹ_direction                                  { VARDECL(پورٹ); VARDTYPE_NDECL(nullptr); }
        |       پورٹ_direction { VARDECL(پورٹ); } net_type      { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        |       net_type                                        { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        ;

پورٹ_decl_Net_E:                  // IEEE: part of port_declaration, optional net type
                /*خالی_تھاواں*/                             { }
        |       net_type                                { } // net_type calls VARDECL
        ;

پورٹ_Sig<nodep>:
                id/*پورٹ*/
                        { $$ = new Ast_پورٹ{$<fl>1, PINNUMINC(), *$1}; }
        |       idSVKwd
                        { $$ = new Ast_پورٹ{$<fl>1, PINNUMINC(), *$1}; }
        ;

پورٹ_عبارت_List<nodep>:  // IEEE: part of (non-ansi) port_reference
                پورٹ_reference                          { $$ = $1; }
        |       پورٹ_عبارت_List ',' پورٹ_reference  { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

پورٹ_reference<nodep>:  // IEEE: (non-ansi) port-reference
        //                      // IEEE: port_identifier constant_select
        //                      // constant_select ::= [ '[' constant_part_select_range ']' ]
                id/*port_identifier*/                   { $$ = nullptr; }  // UNSUP above here
        |       id/*port_identifier*/ part_select_range  { $$ = nullptr; DEL($2); }  // UNSUP above here
        ;

//**********************************************************************
// Interface headers

interface_declaration:          // IEEE: interface_declaration + interface_nonansi_header + interface_ansi_header:
        //                      // timeunits_declarationE is instead in interface_item
                intFront importsAndParametersE ports_تارا_E ';'
                        interface_شے_List_E yENDINTERFACE سرا_Label_E
                        { إذا ($2) $1->جوڑو_Stmtsp($2);
                          إذا ($3) $1->جوڑو_Stmtsp($3);
                          إذا ($5) $1->جوڑو_Stmtsp($5);
                          $1->hasParameterList($<flag>2); }
        |       yEXTERN intFront parameter_پورٹ_list_E ports_تارا_E ';'
                        { DEL($2->unlink_Fr_پٹھ()); }
                        // We allow interfaces to be declared after instantiations, so harmless
        ;

intFront<گرہ_Modulep>:
                yINTERFACE lifetimeE idAny/*new_interface*/
                        { $$ = new AstIface{$<fl>3, *$3, PARSEP->libname()};
                          $$->وچ_Library(true);
                          $$->lifetime($2);
                          PARSEP->rootp()->جوڑو_Modulesp($$); }
        |       intFront sigAttrScope                   { $$ = $1; }
        ;

interface_شے_List_E<nodep>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       interface_شے_List                      { $$ = $1; }
        ;

interface_شے_List<nodep>:
                interface_شے                          { $$ = $1; }
        |       interface_شے_List interface_شے       { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

interface_شے<nodep>:          // IEEE: interface_item + non_port_interface_item
                پورٹ_declaration ';'                    { $$ = $1; }
        //                      // IEEE: non_port_interface_item
        //                      // IEEE: generate_region
        |       i_generate_region                       { $$ = $1; }
        |       interface_or_generate_شے              { $$ = $1; }
        |       پروگرام_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within interface decls"); }
        //                      // IEEE 1800-2017: modport_item
        //                      // See instead old 2012 position in interface_or_generate_item
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within interface decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // See note in interface_or_generate item
        |       module_common_شے                      { $$ = $1; }
        ;

interface_or_generate_شے<nodep>:  // ==IEEE: interface_or_generate_item
        //                      // module_common_item in interface_item, as otherwise duplicated
        //                      // with module_or_generate_item's module_common_item
                modport_declaration                     { $$ = $1; }
        |       extern_tf_declaration                   { $$ = $1; }
        ;

//**********************************************************************
// Program headers

anonymous_پروگرام<nodep>:       // ==IEEE: anonymous_program
        //                      // See the spec - this doesn't change the scope, items still go up "top"
                y_پروگرام ';' anonymous_پروگرام_شے_List_E yENDPROGRAM
                        { $$ = nullptr;
                         BBUNSUP($<fl>1, "Unsupported: Anonymous programs");
                         VARDTYPE_NDECL(nullptr);
                         DEL($3); }
        ;

anonymous_پروگرام_شے_List_E<nodep>:     // IEEE: { anonymous_program_item }
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       anonymous_پروگرام_شے_List              { $$ = $1; }
        ;

anonymous_پروگرام_شے_List<nodep>:      // IEEE: { anonymous_program_item }
                anonymous_پروگرام_شے                  { $$ = $1; }
        |       anonymous_پروگرام_شے_List anonymous_پروگرام_شے       { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

anonymous_پروگرام_شے<nodep>:  // ==IEEE: anonymous_program_item
                task_declaration                        { $$ = $1; }
        |       function_declaration                    { $$ = $1; }
        |       class_declaration                       { $$ = $1; }
        //                      // IEEE-2023: Added: interface_class_declaration
        //                      // interface_class_declaration is part of class_declaration
        |       covergroup_declaration                  { $$ = $1; }
        //                      // class_constructor_declaration is part of function_declaration
        |       ';'                                     { $$ = nullptr; }
        ;

پروگرام_declaration:            // IEEE: program_declaration + program_nonansi_header + program_ansi_header:
        //                      // timeunits_declarationE is instead in program_item
                pgmFront parameter_پورٹ_list_E ports_تارا_E ';'
        /*cont*/    پروگرام_شے_List_E yENDPROGRAM سرا_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          إذا ($2) $1->جوڑو_Stmtsp($2);
                          إذا ($3) $1->جوڑو_Stmtsp($3);
                          إذا ($5) $1->جوڑو_Stmtsp($5);
                          GRAMMARP->سرا_Label($<fl>7, $1, $7); }
        |       yEXTERN pgmFront parameter_پورٹ_list_E ports_تارا_E ';'
                        { DEL($2->unlink_Fr_پٹھ()); }
                        // We allow programs to be declared after instantiations, so harmless
        ;

pgmFront<گرہ_Modulep>:
                y_پروگرام lifetimeE idAny/*new_program*/
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname(), AstModule::پروگرام{}};
                          $$->lifetime($2);
                          $$->وچ_Library(PARSEP->وچ_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ویلا_آخری_Unit());
                          PARSEP->rootp()->timeprecision_ملاؤ($$->fileline(),
                                                              PARSEP->ویلا_آخری_Prec());
                          PARSEP->rootp()->جوڑو_Modulesp($$); }
        ;

پروگرام_شے_List_E<nodep>:       // ==IEEE: [{ program_item }]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       پروگرام_شے_List                        { $$ = $1; }
        ;

پروگرام_شے_List<nodep>:        // ==IEEE: { program_item }
                پروگرام_شے                            { $$ = $1; }
        |       پروگرام_شے_List پروگرام_شے           { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

پروگرام_شے<nodep>:            // ==IEEE: program_item
                پورٹ_declaration ';'                    { $$ = $1; }
        |       non_پورٹ_پروگرام_شے                   { $$ = $1; }
        ;

non_پورٹ_پروگرام_شے<nodep>:   // ==IEEE: non_port_program_item
                continuous_سونپو                       { $$ = $1; }
        |       module_or_generate_شے_declaration     { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       concurrent_assertion_شے               { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       پروگرام_generate_شے                   { $$ = $1; }
        ;

پروگرام_generate_شے<nodep>:           // ==IEEE: program_generate_item
                loop_generate_construct                 { $$ = $1; }
        |       conditional_generate_construct          { $$ = $1; }
        |       generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       gen_شے_Begin                            { $$ = $1; }
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
                yMODPORT modport_شے_List ';'           { $$ = $2; }
        ;

modport_شے_List<nodep>:                // IEEE: part of modport_declaration
                modport_شے                            { $$ = $1; }
        |       modport_شے_List ',' modport_شے       { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

modport_شے<nodep>:                    // ==IEEE: modport_item
                idAny/*new-modport*/ '('
        /*mid*/         { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT);
                          GRAMMARP->m_modportProtoTasksp = nullptr; }
        /*cont*/    modportPortsDeclList ')'
                        { $$ = new AstModport{$<fl>1, *$1, $4};
                          // Append any prototype tasks from import/export as interface siblings
                          إذا (GRAMMARP->m_modportProtoTasksp) {
                              $$ = Ast_گرہ::جوڑو_اگلا_Null(
                                  GRAMMARP->m_modportProtoTasksp, $$);
                              GRAMMARP->m_modportProtoTasksp = nullptr;
                          } }
        ;

modportPortsDeclList<nodep>:
                modportPortsDecl                            { $$ = $1; }
        |       modportPortsDeclList ',' modportPortsDecl   { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

// IEEE: modport_ports_declaration  + modport_simple_ports_declaration
//      + (modport_tf_ports_declaration+import_export) + modport_clocking_declaration
// We've expanded the lists each take to instead just have standalone ID ports.
// We track the type as with the V2k series of defines, then create as each ID is seen.
modportPortsDecl<nodep>:
        //                      // IEEE: modport_simple_ports_declaration
                پورٹ_direction { GRAMMARP->m_modport_Imp_Exp_سرگرم = false; } modport_Simple_پورٹ_Or_TFPort { $$ = $3; }
        //                      // IEEE: modport_clocking_declaration
        |       yCLOCKING idAny/*clocking_identifier*/ { $$ = new AstModportClockingRef{$1, *$2}; }
        //                      // IEEE: yIMPORT modport_tf_port
        //                      // IEEE: yEXPORT modport_tf_port
        //                      // modport_tf_port expanded here
        |       yIMPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, false};
                          GRAMMARP->m_modport_Imp_Exp_سرگرم = true;
                          GRAMMARP->m_modport_Imp_Exp_آخری_Is_Export = false; }
        |       yEXPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, true};
                          GRAMMARP->m_modport_Imp_Exp_سرگرم = true;
                          GRAMMARP->m_modport_Imp_Exp_آخری_Is_Export = true; }
        |       yIMPORT طریقہ_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), false};
                          GRAMMARP->m_modport_Imp_Exp_سرگرم = true;
                          GRAMMARP->m_modport_Imp_Exp_آخری_Is_Export = false;
                          // Import prototype: task should already exist in interface
                          // (from export or direct declaration). Delete the prototype.
                          DEL($2); }
        |       yEXPORT طریقہ_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), true};
                          GRAMMARP->m_modport_Imp_Exp_سرگرم = true;
                          GRAMMARP->m_modport_Imp_Exp_آخری_Is_Export = true;
                          // Export: add prototype task to interface (as sibling of modport)
                          GRAMMARP->m_modportProtoTasksp
                              = Ast_گرہ::جوڑو_اگلا_Null(GRAMMARP->m_modportProtoTasksp, $2); }
        // Continuations of above after a comma.
        //                      // IEEE: modport_simple_ports_declaration
        |       modport_Simple_پورٹ_Or_TFPort                { $$ = $1; }
        ;

modport_Simple_پورٹ_Or_TFPort<nodep>:// IEEE: modport_simple_port or modport_tf_port, depending what keyword was earlier
                idAny                                   { $$ = GRAMMARP->m_modport_Imp_Exp_سرگرم ?
                                                                static_cast<Ast_گرہ*>(
                                                                  new AstModportFTaskRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_modport_Imp_Exp_آخری_Is_Export} ) :
                                                                static_cast<Ast_گرہ*>(
                                                                  new AstModportVarRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_varIO} ); }
        |       '.' idAny '(' ')'                       { $$ = new AstModportVarRef{$<fl>2, *$2, GRAMMARP->m_varIO};
                                                          BBUNSUP($<fl>4, "Unsupported: Modport empty expression"); }
        |       '.' idAny '(' اظہاریہ ')'                  { $$ = new AstModportVarRef{$<fl>2, *$2, $4, GRAMMARP->m_varIO}; }
        ;

//************************************************
// Variable Declarations

genvar_declaration<nodep>:      // ==IEEE: genvar_declaration
                yGENVAR list_دا_genvar_identifiers ';'  { $$ = $2; }
        ;

list_دا_genvar_identifiers<nodep>:      // IEEE: list_of_genvar_identifiers (for declaration)
                genvar_identifierDecl                   { $$ = $1; }
        |       list_دا_genvar_identifiers ',' genvar_identifierDecl    { $$ = $1->جوڑو_اگلا($3); }
        ;

genvar_identifierDecl<varp>:            // IEEE: genvar_identifier (for declaration)
                idAny/*new-genvar_identifier*/ sigAttrListE
                        { VARRESET_NONLIST(GENVAR);
                          Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$<fl>1, VBasicDTypeKwd::INTEGER};
                          VARDTYPE(dtp);
                          $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        ;

parameter_declaration<nodep>:   // IEEE: local_ or parameter_declaration and type_parameter_declaration
        //                      // IEEE: yPARAMETER yTYPE list_of_type_assignments ';'
        //                      // Instead of list_of_type_assignments
        //                      // we use list_of_param_assignments because for port handling
        //                      // it already must accept types, so simpler to have code only one place
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                parameter_declarationFront list_دا_param_assignments            { $$ = $2; }
        |       parameter_declarationTypeFront list_دا_type_assignments         { $$ = $2; }
        ;

parameter_declarationFront:     // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset انکڑے_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        ;

parameter_declarationTypeFront: // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/ VARDTYPE(new AstParseTypeDType{$2}); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_گرہ_DType* ثابت dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        ;

parameter_پورٹ_declaration_Front_E: // IEEE: local_ or parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset انکڑے_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       implicit_typeE
                        { /*VARRESET-in-varParam*/
                          // Keep previous type to handle subsequent declarations.
                          // This rule is also used when the previous parameter is a type parameter
                          إذا ($1) $1->v3error("parameter port declarations require 'parameter'"
                                              " keyword before implicit data types"
                                              " (IEEE 1800-2023 6.20.1/A.2.1.1)\n"
                                              + $1->warn_ہور()
                                              + "... Suggest add 'parameter' before here");
                       }
        |       انکڑے_type                               { /*VARRESET-in-varParam*/ VARDTYPE($1); }
        ;

parameter_پورٹ_declaration_Type_Front_E: // IEEE: parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_گرہ_DType* ثابت dtp = new AstParseTypeDType{$2}; VARDTYPE(dtp); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_گرہ_DType* ثابت dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        |       yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_گرہ_DType* ثابت dtp = new AstParseTypeDType{$1}; VARDTYPE(dtp); }
        |       yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_گرہ_DType* ثابت dtp = new AstParseTypeDType{$1, $2}; VARDTYPE(dtp); }
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
                          إذا (GRAMMARP->m_netStrengthp) {
                              VL_DO_CLEAR(delete GRAMMARP->m_netStrengthp, GRAMMARP->m_netStrengthp = nullptr);
                          }
                          GRAMMARP->سیٹ_Net_Delay(nullptr); }
        ;

net_declarationFront:           // IEEE: beginning of net_declaration
                net_declRESET net_type driveStrengthE net_scalaredE net_انکڑے_Type_E
                        { VARDTYPE_NDECL($5);
                          GRAMMARP->سیٹ_Net_Strength($3); }
        |       net_declRESET yINTERCONNECT signingE rangeListE
                        { BBUNSUP($<fl>2, "Unsupported: interconnect");
                          VARDECL(WIRE);
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        ;

net_declRESET:
                /*خالی_تھاواں*/                             { VARRESET_NONLIST(UNKNOWN); }
        ;

net_scalaredE:
                /*خالی_تھاواں*/                             { }
        //                      //UNSUP: ySCALARED/yVECTORED ignored
        |       ySCALARED                               { }
        |       yVECTORED                               { }
        ;

net_انکڑے_Type_E<گرہ_DTypep>:
        //                      // If there's a SV data type there shouldn't be a delay on this wire
        //                      // Otherwise #(...) can't be determined to be a delay or parameters
        //                      // Submit this as a footnote to the committee
                var_انکڑے_type                           { $$ = $1; }
        |       signingE rangeList delay_controlE
                        { $$ = GRAMMARP->جوڑو_Range(new AstBasicDType{$2->fileline(), LOGIC, $1},
                                                                    $2, true);
                          GRAMMARP->سیٹ_Net_Delay($3); }  // not implicit
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC, $1}; }  // not implicit
        |       /*implicit*/ delay_controlE
                        { $$ = new AstBasicDType{CRELINE(), LOGIC};
                          GRAMMARP->سیٹ_Net_Delay($1); }  // not implicit
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

پورٹ_direction:                 // ==IEEE: port_direction + tf_port_direction
        //                      // IEEE 19.8 just "input" FIRST forces type to wire - we'll ignore that here
        //                      // Only used for ANSI declarations
                y_داخلہ                                  { VARIO(داخلہ); }
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

پورٹ_direction_Reset:            // IEEE: port_direction that starts a port_declaraiton
        //                      // Used only for declarations outside the port list
                y_داخلہ                                  { VARRESET_NONLIST(UNKNOWN); VARIO(داخلہ); }
        |       yOUTPUT                                 { VARRESET_NONLIST(UNKNOWN); VARIO(OUTPUT); }
        |       yINOUT                                  { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT); }
        |       yREF                                    { VARRESET_NONLIST(UNKNOWN); VARIO(REF); }
        |       yCONST__REF yREF                        { VARRESET_NONLIST(UNKNOWN); VARIO(CONSTREF); }
        ;

پورٹ_declaration<nodep>:        // ==IEEE: port_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port, tf_port_declaration, data_declarationVarFront
        //
        //                      // IEEE: inout_declaration
        //                      // IEEE: input_declaration
        //                      // IEEE: output_declaration
        //                      // IEEE: ref_declaration
                پورٹ_direction_Reset پورٹ_decl_Net_E انکڑے_type
        /*mid*/         { VARDTYPE($3); }
        /*cont*/    list_دا_variable_decl_assignments                   { $$ = $5; }
        |       پورٹ_direction_Reset پورٹ_decl_Net_E yVAR انکڑے_type
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_دا_variable_decl_assignments                   { $$ = $6; }
        |       پورٹ_direction_Reset پورٹ_decl_Net_E yVAR implicit_typeE
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_دا_variable_decl_assignments                   { $$ = $6; }
        |       پورٹ_direction_Reset پورٹ_decl_Net_E signingE rangeList
        /*mid*/         { Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                              new AstBasicDType{$4->fileline(), LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_دا_variable_decl_assignments                   { $$ = $6; }
        |       پورٹ_direction_Reset پورٹ_decl_Net_E signing
        /*mid*/         { Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $3};
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_دا_variable_decl_assignments                   { $$ = $5; }
        |       پورٹ_direction_Reset پورٹ_decl_Net_E /*implicit*/
        /*mid*/         { VARDTYPE_NDECL(nullptr); /*default_nettype*/ }
        /*cont*/    list_دا_variable_decl_assignments                   { $$ = $4; }
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
                          AstIfaceRefDType* ثابت dtp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          VARDTYPE(dtp); }
        /*cont*/    mpInstnameList
                        { $$ = VARDONEP($5, nullptr, nullptr); DEL($5); }
        //UNSUP: strengthSpecE for udp_instantiations
        ;

tf_پورٹ_declaration<nodep>:     // ==IEEE: tf_port_declaration
        //                      // Used inside function; followed by ';'
        //                      // SEE ALSO port_declaration, port, data_declarationVarFront
        //
                پورٹ_direction_Reset      انکڑے_type      { VARDTYPE($2); }  list_دا_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       پورٹ_direction_Reset      implicit_typeE { VARDTYPE_NDECL($2); }  list_دا_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       پورٹ_direction_Reset yVAR انکڑے_type      { VARDTYPE($3); }  list_دا_tf_variable_identifiers ';'
                        { $$ = $5; }
        |       پورٹ_direction_Reset yVAR implicit_typeE { VARDTYPE($3); }  list_دا_tf_variable_identifiers ';'
                        { $$ = $5; }
        ;

integer_atom_type<basicDTypep>: // ==IEEE: integer_atom_type
                yBYTE                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::BYTE}; }
        |       ySHORTINT                               { $$ = new AstBasicDType{$1, VBasicDTypeKwd::SHORTINT}; }
        |       yINT                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INT}; }
        |       yLONGINT                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::LONGINT}; }
        |       yINTEGER                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INTEGER}; }
        |       y_ویلا                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::ویلا}; }
        ;

integer_سمتیہ_type<basicDTypep>:       // ==IEEE: integer_atom_type
                y_ثنائی_ہندسہ                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::ثنائی_ہندسہ}; }
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

simple_typeNoRef<گرہ_DTypep>:   // IEEE: simple_type without idType
        //                      // IEEE: integer_type
                integer_atom_type                       { $$ = $1; }
        |       integer_سمتیہ_type                     { $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        //
        //                      // { generate_block_identifer ... } '.'
        //                      // Need to determine if generate_block_identifier can be lex-detected
        ;

انکڑے_type<گرہ_DTypep>:          // ==IEEE: data_type
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                انکڑے_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idType packed_dimensionListE
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idType parameter_قدر_assignment_Class packed_dimensionListE
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

انکڑے_type_Any<گرہ_DTypep>:       // ==IEEE: data_type (accepting idAny)
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                انکڑے_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idAny packed_dimensionListE
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idAny parameter_قدر_assignment_Class packed_dimensionListE
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

انکڑے_type_Basic<گرہ_DTypep>:             // IEEE: part of data_type
                integer_سمتیہ_type signingE rangeListE { $1->سیٹ_Signed_State($2); $$ = GRAMMARP->جوڑو_Range($1, $3, true); }
        //                      // IEEE: packed_dimension: unsized_dimension, legal only in DPI import
        |       integer_سمتیہ_type signingE '[' ']'
                        { $1->سیٹ_Signed_State($2);
                          $$ = new AstUnsizedArrayDType{$3, VFlag_بچہ_DType{}, $1}; }
        |       integer_atom_type signingE              { $1->سیٹ_Signed_State($2); $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        ;

انکڑے_type_No_Ref<گرہ_DTypep>:             // ==IEEE: data_type, excluding class_type etc references
                انکڑے_type_Basic                          { $$ = $1; }
        |       struct_unionDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpsu" + cvt_منزل_Str(GRAMMARP->s_type_Imp_عدد++),
                                                           VFlag_بچہ_DType{}, $1}, $2, true); }
        |       enumDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpenum" + cvt_منزل_Str(GRAMMARP->s_type_Imp_عدد++),
                                                           VFlag_بچہ_DType{}, $1}, $2, true); }
        |       ySTRING
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::STRING}; }
        |       yCHANDLE
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CHANDLE}; }
        |       yEVENT
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::EVENT}; v3Global.سیٹ_Has_Events(); }
        |       type_referenceDecl                      { $$ = $1; }
        //                      // IEEE: class_scope: see data_type above
        //                      // IEEE: class_type: see data_type above
        //                      // IEEE: ps_covergroup: see data_type above
        //                      // Rules overlap virtual_interface_declaration
        |       yVIRTUAL__INTERFACE yINTERFACE انکڑے_type_Virtual
                        { $$ = $3; }
        |       yVIRTUAL__anyID                انکڑے_type_Virtual
                        { $$ = $2; }
        ;

انکڑے_type_Virtual<گرہ_DTypep>:           // ==IEEE: data_type after yVIRTUAL [ yINTERFACE ]
        //                      // Parameters here are SV2009
                idAny/*interface*/
                        { AstIfaceRefDType* ثابت ifrefp = new AstIfaceRefDType{$<fl>1, "", *$1};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ '.' idAny/*modport*/
                        { AstIfaceRefDType* ثابت ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_قدر_assignment_Class
                        { AstIfaceRefDType* ثابت ifrefp = new AstIfaceRefDType{$<fl>1, nullptr, "", *$1, "", $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_قدر_assignment_Class '.' idAny/*modport*/
                        { AstIfaceRefDType* ثابت ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>4, "", *$1, *$4, $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        ;

انکڑے_type_or_void<گرہ_DTypep>:  // ==IEEE: data_type_or_void
                انکڑے_type_Any                            { $$ = $1; }
        |       yVOID
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CVOID}; }
        ;

var_انکڑے_type<گرہ_DTypep>:              // ==IEEE: var_data_type
                انکڑے_type                               { $$ = $1; }
        |       yVAR انکڑے_type                          { $$ = $2; }
        |       yVAR implicit_typeE                     { $$ = $2; }
        ;

type_referenceBoth<گرہ_Exprp>:          // IEEE: type_reference
                yTYPE__ETC '(' اظہاریہ_Or_انکڑے_Type ')'
                        { $$ = new Ast_Attr_دا{$1, VAttrType::TYPEID, $3}; }
        ;

type_referenceDecl<گرہ_DTypep>:         // IEEE: type_reference (as a data type for declaration)
                yTYPE__ETC '(' اظہاریہ_Or_انکڑے_Type ')'
                        { $$ = new AstRefDType{$1, AstRefDType::Flag_Type_دا_اظہاریہ{}, $3}; }
        ;

type_referenceEq<گرہ_Exprp>:            // IEEE: type_reference (as an ==/!== expression)
                yTYPE__EQ '(' اظہاریہ_Or_انکڑے_Type ')'
                        { $$ = new Ast_Attr_دا{$1, VAttrType::TYPEID, $3}; }
        ;

struct_unionDecl<گرہ_UOr_Struct_DTypep>:  // IEEE: part of data_type
        //                      // packedSigningE is NOP for unpacked
                ySTRUCT        packedSigningE '{'
        /*mid*/         { $<گرہ_UOr_Struct_DTypep>$ = new AstStructDType{$1, $2}; }
        /*cont*/    struct_union_member_List_سرا
                        { $$ = $<گرہ_UOr_Struct_DTypep>4; $$->جوڑو_Membersp($5); }
        |       yUNION taggedSoftE packedSigningE '{'
        /*mid*/         { $<گرہ_UOr_Struct_DTypep>$ = new AstUnionDType{$1, $2 == tagged_SOFT, $2 == tagged_TAGGED, $3}; }
        /*cont*/    struct_union_member_List_سرا
                        { $$ = $<گرہ_UOr_Struct_DTypep>5; $$->جوڑو_Membersp($6); }
        ;

struct_union_member_List_سرا<memberDTypep>: // IEEE: { struct_union_member } '}'
                struct_union_memberList '}'                     { $$ = $1; }
        //
        |       struct_union_memberList error '}'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error '}'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_memberList<memberDTypep>: // IEEE: { struct_union_member }
                struct_union_member                             { $$ = $1; }

        |       struct_union_memberList struct_union_member     { $$ = جوڑو_اگلا_Null($1, $2); }
        //
        |       struct_union_memberList error ';'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error ';'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_member<memberDTypep>:     // ==IEEE: struct_union_member
                random_qualifierE انکڑے_type_or_void
        /*mid*/         { GRAMMARP->m_memDTypep = $2;  // As a list follows, need to attach this dtype to each member.
                          GRAMMARP->m_memRand = $1.randAttr(); }
        /*cont*/    list_دا_member_decl_assignments ';'
                        { $$ = $4;
                          DEL(GRAMMARP->m_memDTypep); GRAMMARP->m_memDTypep = nullptr;
                          GRAMMARP->m_memRand = VRandAttr::NONE; }
        |       vl_ٹیگ                                   { $$ = nullptr; }
        ;

list_دا_member_decl_assignments<memberDTypep>: // Derived from IEEE: list_of_variable_decl_assignments
                member_decl_assignment                  { $$ = $1; }
        |       list_دا_member_decl_assignments ',' member_decl_assignment      { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

member_decl_assignment<memberDTypep>:   // Derived from IEEE: variable_decl_assignment
        //                      // At present we allow only packed structures/unions.
        //                      // So this is different from variable_decl_assignment
                idAny variable_dimensionListE
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlag_بچہ_DType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_درخت(true) : nullptr),
                                                                        $2, false),
                                                  nullptr};
                          $$->rand(GRAMMARP->m_memRand);
                          PARSEP->ٹیگ_Nodep($$);
                        }
        |       idAny variable_dimensionListE '=' variable_decl_اظہاریہ
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlag_بچہ_DType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_درخت(true) : nullptr),
                                                                        $2, false),
                                                  $4};
                          $$->rand(GRAMMARP->m_memRand);
                          PARSEP->ٹیگ_Nodep($$);
                        }
        |       idSVKwd                                 { $$ = nullptr; }
        //
        //                      // IEEE: "dynamic_array_variable_identifier '[' ']' [ '=' dynamic_array_new ]"
        //                      // Matches above with variable_dimensionE = "[]"
        //                      // IEEE: "class_variable_identifier [ '=' class_new ]"
        //                      // variable_dimensionE must be empty
        //                      // Pushed into variable_declExpr:dynamic_array_new
        ;

list_دا_variable_decl_assignments<varp>:        // ==IEEE: list_of_variable_decl_assignments
                variable_decl_assignment                { $$ = $1; }
        |       list_دا_variable_decl_assignments ',' variable_decl_assignment  { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

variable_decl_assignment<varp>: // ==IEEE: variable_decl_assignment
                id variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       id variable_dimensionListE sigAttrListE '=' variable_decl_اظہاریہ
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

list_دا_tf_variable_identifiers<nodep>: // ==IEEE: list_of_tf_variable_identifiers
                tf_variable_identifier                  { $$ = $1; }
        |       list_دا_tf_variable_identifiers ',' tf_variable_identifier      { $$ = $1->جوڑو_اگلا($3); }
        ;

tf_variable_identifier<varp>:           // IEEE: part of list_of_tf_variable_identifiers
                id variable_dimensionListE sigAttrListE اظہاریہ_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          إذا ($4) Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(
                                      $$, new Ast_سونپو{$4->fileline(), new AstParseRef{$<fl>1, *$1}, $4}); }
        ;

variable_decl_اظہاریہ<nodep>:               // IEEE: part of variable_decl_assignment - rhs of expr
                اظہاریہ                                    { $$ = $1; }
        |       dynamic_array_new                       { $$ = $1; }
        |       class_newNoScope                        { $$ = $1; }
        ;

variable_dimensionListE<گرہ_Rangep>:    // IEEE: variable_dimension + empty
                /*empty*/                               { $$ = nullptr; }
        |       variable_dimensionList                  { $$ = $1; }
        ;

variable_dimensionList<گرہ_Rangep>:     // IEEE: variable_dimension + empty
                variable_dimension                      { $$ = $1; }
        |       variable_dimensionList variable_dimension       { $$ = $1->جوڑو_اگلا($2); }
        ;

variable_dimension<گرہ_Rangep>: // ==IEEE: variable_dimension
        //                      // IEEE: unsized_dimension
                '[' ']'                                 { $$ = new AstUnsizedRange{$1}; }
        //                      // IEEE: unpacked_dimension
        |       anyrange                                { $$ = $1; }
        //                      // IEEE: unpacked_dimension (if const_expr)
        //                      // IEEE: associative_dimension (if data_type)
        //                      // Can't tell which until see if expr is data type or not
        |       '[' اظہاریہ_Or_انکڑے_Type ']'                  { $$ = new AstBracketRange{$1, $2}; }
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
        |       yPACKED signingE                        { $$ = $2; إذا ($$ == VSigning::NOSIGN) $$ = VSigning::UNSIGNED; }
        ;

//************************************************
// enum

// IEEE: part of data_type
enumDecl<گرہ_DTypep>:
                yENUM enum_ادھار_type_E '{' enum_nameList '}'
                        { $$ = new AstEnumDType{$1, VFlag_بچہ_DType{}, $2, $4}; }
        ;

enum_ادھار_type_E<گرہ_DTypep>:    // IEEE: enum_base_type
                /*خالی_تھاواں*/
                        { $$ = new AstBasicDType{CRELINE(), VBasicDTypeKwd::INT}; }
        //                      // Not in spec, but obviously "enum [1:0]" should work
        //                      // implicit_type expanded, without empty
        //                      // Note enum base types are always packed data types
        |       signingE rangeList
                        { $$ = GRAMMARP->جوڑو_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        //
        |       integer_atom_type signingE
                        { $1->سیٹ_Signed_State($2); $$ = $1; }
        |       integer_سمتیہ_type signingE rangeListE
                        { $1->سیٹ_Signed_State($2); $$ = GRAMMARP->جوڑو_Range($1, $3, true); }
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
        |       enum_nameList ',' enum_name_declaration { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

enum_name_declaration<enumItemp>:   // ==IEEE: enum_name_declaration
                idAny/*enum_identifier*/ enumNameRangeE enum_Name_شروع_E
                        { $$ = new Ast_Enum_شے{$<fl>1, *$1, $2, $3}; }
        ;

enumNameRangeE<rangep>:          // IEEE: second part of enum_name_declaration
                /*خالی_تھاواں*/
                        { $$ = nullptr; }
        |       '[' intnumAsConst ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstConst($1, $2->منزل_SInt() - 1)}; DEL($2); }
        |       '[' intnumAsConst ':' intnumAsConst ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

enum_Name_شروع_E<گرہ_Exprp>:      // IEEE: third part of enum_name_declaration
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       '=' const_اظہاریہ                           { $$ = $2; }
        ;

intnumAsConst<constp>:
                yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        ;

//************************************************
// Typedef

انکڑے_declaration<nodep>:        // ==IEEE: data_declaration
        //                      // VARRESET can't be called here - conflicts
                انکڑے_declaration_Var                     { $$ = $1; }
        |       type_declaration                        { $$ = $1; }
        |       package_import_declaration              { $$ = $1; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        //                      // 1800-2009:
        |       nettype_declaration                     { $$ = $1; }
        |       vl_ٹیگ                                   { $$ = nullptr; }
        ;

class_property<nodep>:          // ==IEEE: class_property, which is {property_qualifier} data_declaration
                memberQualListE انکڑے_declaration_Var_Class
                        { $$ = $2; $1.apply_منزل_Nodes($2); }
        |       memberQualListE type_declaration
                        { $$ = $2; إذا (VN_IS($2, Typedef)) $1.apply_منزل_Nodes(VN_AS($2, Typedef)); }
        //                      // UNSUP: Import needs to apply local/protected from memberQualList, and error on others
        |       memberQualListE package_import_declaration
                        { $$ = $2; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        ;

انکڑے_declaration_Var<varp>:      // IEEE: part of data_declaration
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                انکڑے_declaration_Var_Front list_دا_variable_decl_assignments ';'  { $$ = $2; }
        ;

انکڑے_declaration_Var_Class<varp>: // IEEE: part of data_declaration (for class_property)
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                انکڑے_declaration_Var_Front_Class list_دا_variable_decl_assignments ';'     { $$ = $2; }
        ;

انکڑے_declaration_Var_Front:       // IEEE: part of data_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port_declaration, tf_port_declaration, port
        //
        //                      // Expanded: "constE yVAR lifetimeE data_type"
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE انکڑے_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT};
                          VARDTYPE(dtp); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/ VARLIFE($2);
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                              new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); }
        //
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
        |       yCONST__ETC yVAR lifetimeE انکڑے_type
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_گرہ_DType* ثابت dtp = new AstConstDType{$<fl>2, VFlag_بچہ_DType{}, $4};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_گرہ_DType* ثابت dtp = new AstConstDType{$<fl>2, VFlag_بچہ_DType{},
                                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE signingE rangeList
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_گرہ_DType* ثابت dtp = new AstConstDType{$<fl>2, VFlag_بچہ_DType{},
                                  GRAMMARP->جوڑو_Range(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $4}, $5, true)};
                          VARDTYPE(dtp); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       /**/                  انکڑے_type
                        { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        |       /**/        lifetime  انکڑے_type
                        { VARRESET_NONLIST(VAR); VARLIFE($1); VARDTYPE($2); }
        |       yCONST__ETC lifetimeE انکڑے_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_گرہ_DType* ثابت dtp = new AstConstDType{$<fl>1, VFlag_بچہ_DType{}, $3};
                          VARDTYPE(dtp); }
        //                      // = class_new is in variable_decl_assignment
        ;

انکڑے_declaration_Var_Front_Class:  // IEEE: part of data_declaration (for class_property)
        //                      // VARRESET called before this rule
        //                      // yCONST is removed, added to memberQual rules
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE انکڑے_type        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE                  { VARRESET_NONLIST(VAR); VARLIFE($2); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp);
                          VARLIFE($2); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       انکڑے_type                       { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        //                      // lifetime is removed, added to memberQual rules to avoid conflict
        //                      // yCONST is removed, added to memberQual rules to avoid conflict
        //                      // = class_new is in variable_decl_assignment
        ;

nettype_declaration<nodep>:  // IEEE: nettype_declaration/net_type_declaration
        //                      // Union of data_typeAny and nettype_identifier matching
                yNETTYPE انکڑے_type_No_Ref
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>3, *$3); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE انکڑے_type_No_Ref
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
        |       yNETTYPE packageClassScopeE idAny parameter_قدر_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE packageClassScopeE idAny parameter_قدر_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/
        /*cont*/   yWITH__ETC packageClassScopeE id/*tf_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype with"); }
        ;

implicit_typeE<گرہ_DTypep>:             // IEEE: part of *data_type_or_implicit
        //                      // Also expanded in data_declaration
                /*خالی_تھاواں*/
                        { $$ = nullptr; }
        |       signingE rangeList
                        { $$ = GRAMMARP->جوڑو_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        ;

assertion_variable_declaration<nodep>:  // IEEE: assertion_variable_declaration
        //                      // IEEE: var_data_type expanded
                var_انکڑے_type                        { VARRESET_NONLIST(VAR); VARDTYPE_NDECL($1); }
        /*cont*/    list_دا_variable_decl_assignments ';'     { $$ = $3; }
        ;

type_declaration<nodep>:        // ==IEEE: type_declaration
                                // Data_type expanded
                yTYPEDEF انکڑے_type_No_Ref
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { Ast_گرہ_DType* ثابت dtp = $2;
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, dtp, $4); }

        // IEEE 1800-2023 6.18 typedef: dotted or arrayed type identifier
        // Handles interface typedef references like if0.rq_t and if0[0].rq_t (arrays allowed after first component)
        |       yTYPEDEF idDottedOrArrayed
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* ثابت ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* ثابت varp = VARDONEA($<fl>3, *$3, $4, $5);
                // idDottedOrArrayed produces Dot/SelBit tree for hierarchical refs like if0[0].rq_t
                varp->valuep($2);
                $$ = varp; }

        // IEEE 1800-2023 6.18 typedef with hierarchical type identifier
        // Special-case array on first component requiring a '.' after ']' to disambiguate from packed dims
        // Examples: typedef if0[0].rq_t my_t; typedef if0[0].x_if.rq_t my_t;
        |       yTYPEDEF id '[' اظہاریہ ']' '.' id_Dotted_Sel_ہور
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* ثابت ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* ثابت varp = VARDONEA($<fl>8, *$8, $9, $10);
                Ast_گرہ_اظہاریہ* ثابت arrp = new Ast_Sel_ثنائی_ہندسہ{$3, new AstParseRef{$<fl>2, *$2, nullptr, nullptr}, $4};
                varp->valuep(new AstDot{$6, false, arrp, $7});
                $$ = varp; }

        |       yTYPEDEF packageClassScope idAny packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>3, *$3, $2, nullptr};
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->createArray(refp, $4, true);
                          $$ = GRAMMARP->createTypedef($<fl>5, *$5, $7, dtp, $6); }
        |       yTYPEDEF packageClassScope idAny parameter_قدر_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>3, *$3, $2, $4};
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->createArray(refp, $5, true);
                          $$ = GRAMMARP->createTypedef($<fl>6, *$6, $8, dtp, $7); }

        // Type alias without packed dimensions: typedef existing_t new_t;
        |       yTYPEDEF idAny idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, refp, $4); }

        // IEEE 1800-2023 6.18.2 typedef with packed dimensions on an existing type identifier
        // Disambiguated from interface array access by requiring ':' inside the brackets
        // (applies to both plain identifiers and type identifiers)
        |       yTYPEDEF id '[' const_اظہاریہ ':' const_اظہاریہ ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_گرہ_Range* ثابت rangep = new AstRange{$3, $4, $6};
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->createArray(refp, جوڑو_اگلا_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        // Same as above but for type identifiers (parameter types, etc.)
        |       yTYPEDEF idType '[' const_اظہاریہ ':' const_اظہاریہ ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_گرہ_Range* ثابت rangep = new AstRange{$3, $4, $6};
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->createArray(refp, جوڑو_اگلا_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        |       yTYPEDEF idAny parameter_قدر_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, nullptr, $3};
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->createArray(refp, $4, true);
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
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       dtypeAttrList                           { $$ = $1; }
        ;

dtypeAttrList<nodep>:
                dtypeAttr                               { $$ = $1; }
        |       dtypeAttrList dtypeAttr                 { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

dtypeAttr<nodep>:
                y_VL_عوامی                              { $$ = new Ast_Attr_دا{$1, VAttrType::DT_عوامی}; }
        ;

vl_ٹیگ:                          // verilator tag handling
                y_VL_ٹیگ                                 { إذا (PARSEP->ٹیگ_Nodep()) PARSEP->ٹیگ_Nodep()->ٹیگ(*$1); }
        ;

//************************************************
// Module Items

module_شے_List_E<nodep>:        // IEEE: Part of module_declaration
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       module_شے_List                         { $$ = $1; }
        ;

module_شے_List<nodep>:         // IEEE: Part of module_declaration
                module_شے                             { $$ = $1; }
        |       module_شے_List module_شے             { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

module_شے<nodep>:             // ==IEEE: module_item
                پورٹ_declaration ';'                    { $$ = $1; }
        |       non_پورٹ_module_شے                    { $$ = $1; }
        ;

non_پورٹ_module_شے<nodep>:    // ==IEEE: non_port_module_item
                generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       gen_شے_Begin                            { $$ = $1; }
        |       module_or_generate_شے                 { $$ = $1; }
        |       specify_بلاک                           { $$ = $1; }
        |       specparam_declaration                   { $$ = $1; }
        |       پروگرام_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within module decls"); }
        |       module_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: module decls within module decls"); }
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within module decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // Verilator specific
        |       vl_Sc_بلاک                               { $$ = $1; }
        |       y_VL_HIER_بلاک                          { $$ = new AstPragma{$1, VPragmaType::HIER_بلاک}; }
        |       yVL_INLINE_MODULE                       { $$ = new AstPragma{$1, VPragmaType::INLINE_MODULE}; }
        |       yVL_NO_INLINE_MODULE                    { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_MODULE}; }
        |       y_VL_عوامی_MODULE                       { $$ = new AstPragma{$1, VPragmaType::عوامی_MODULE}; v3Global.dpi(true); }
        ;

vl_Sc_بلاک<nodep>:  // Verilator-specific `systemc_* blocks
                yaSCHDR     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR, *$1};  }
        |       yaSCHDRP    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR_POST, *$1}; }
        |       yaSCINT     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::INT, *$1}; }
        |       yaSCIMP     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP, *$1}; }
        |       yaSCIMPH    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP_HDR, *$1}; }
        |       yaSCCTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::CTOR, *$1}; }
        |       yaSCDTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::DTOR, *$1}; }
        ;


module_or_generate_شے<nodep>: // ==IEEE: module_or_generate_item
        //                      // IEEE: parameter_override
                yDEFPARAM list_دا_defparam_assignments ';'      { $$ = $2; }
        //                      // IEEE: gate_instantiation + udp_instantiation + module_instantiation
        //                      // not here, see etcInst in module_common_item
        //                      // We joined udp & module definitions, so this goes here
        |       combinational_body                      { $$ = $1; }
        //                      // This module_common_item shared with
        //                      // interface_or_generate_item:module_common_item
        |       module_common_شے                      { $$ = $1; }
        ;

module_common_شے<nodep>:      // ==IEEE: module_common_item
                module_or_generate_شے_declaration     { $$ = $1; }
        //                      // IEEE: interface_instantiation
        //                      // + IEEE: program_instantiation
        //                      // + module_instantiation from module_or_generate_item
        |       etcInst                                 { $$ = $1; }
        |       assertion_شے                          { $$ = $1; }
        |       bind_directive                          { $$ = $1; }
        |       continuous_سونپو                       { $$ = $1; }
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

continuous_سونپو<nodep>:       // IEEE: continuous_assign
                y_سونپو driveStrengthE delay_controlE سونپو_List ';'
                        { $$ = $4;
                          STRENGTH_LIST($4, $2);
                          DELAY_LIST($4, $3); }
        ;

initial_construct<nodep>:       // IEEE: initial_construct
                yINITIAL stmt                      { $$ = new AstInitial{$1, $2}; }
        ;


net_alias<nodep>:               // IEEE: net_alias
                yALIAS variable_lvalue aliasEqList ';'
                        { $2->جوڑو_اگلا($3);
                          $$ = new AstAlias{$1, $2}; }
        ;

aliasEqList<گرہ_Exprp>:                    // IEEE: part of net_alias
                '=' variable_lvalue                     { $$ = $2; }
        |       aliasEqList '=' variable_lvalue         { $$ = $1->جوڑو_اگلا($3); }
        ;

final_construct<nodep>:         // IEEE: final_construct
                yFINAL stmt                        { $$ = new AstFinal{$1, $2}; }
        ;

module_or_generate_شے_declaration<nodep>:     // ==IEEE: module_or_generate_item_declaration
                package_or_generate_شے_declaration    { $$ = $1; }
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
                yDEFAULT yDISABLE yIFF اظہاریہ/*expression_or_dist*/ ';'
                        { $$ = new AstDefaultDisable{$1, $4}; }
        ;

bind_directive<nodep>:          // ==IEEE: bind_directive + bind_target_scope
        //                      // ';' - Note IEEE grammar is wrong, includes extra ';'
        //                      // - it's already in module_instantiation
        //                      // We merged the rules - id may be a bind_target_instance or
        //                      // module_identifier or interface_identifier
                yBIND bind_نشانہ_instance bind_instantiation   { $$ = new AstBind{$<fl>2, *$2, $3}; }
        |       yBIND bind_نشانہ_instance ':' bind_نشانہ_instance_list bind_instantiation
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: Bind with instance list"); DEL($5); }
        ;

bind_نشانہ_instance_list:      // ==IEEE: bind_target_instance_list
                bind_نشانہ_instance                    { }
        |       bind_نشانہ_instance_list ',' bind_نشانہ_instance      { }
        ;

bind_نشانہ_instance<strp>:     // ==IEEE: bind_target_instance
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
                yGENERATE ~c~gen_شے_List yENDGENERATE   { $$ = $2; }
        |       yGENERATE yENDGENERATE                  { $$ = nullptr; }
        ;

c_generate_region<nodep>:  // IEEE: generate_region (for checkers)
                BISONPRE_COPY(generate_region,{s/~c~/c_/g})     // {copied}
        ;

i_generate_region<nodep>:  // IEEE: generate_region (for interface)
                BISONPRE_COPY(generate_region,{s/~c~/i_/g})     // {copied}
        ;

generate_بلاک_or_null<nodep>:  // IEEE: generate_block_or_null (called from gencase/genif/genfor)
        //      ';'             // is included in
        //                      // IEEE: generate_block
        //                      // Must always return a BEGIN node, or nullptr - see GenFor construction
                ~c~generate_شے
                        { $$ = $1 ? (new Ast_Gen_بلاک{$1->fileline(), "", $1, true}) : nullptr; }
        |       ~c~gen_شے_Begin                         { $$ = $1; }
        ;

c_generate_بلاک_or_null<nodep>:  // IEEE: generate_block_or_null (for checkers)
                BISONPRE_COPY(generate_بلاک_or_null,{s/~c~/c_/g})      // {copied}
        ;

gen_شے_Begin<nodep>:            // IEEE: part of generate_block
                yBEGIN ~c~gen_شے_List y_سرا              { $$ = new Ast_Gen_بلاک{$1, "", $2, false}; }
        |       yBEGIN y_سرا                             { $$ = nullptr; }
        |       id yP_COLON__BEGIN yBEGIN ~c~gen_شے_List y_سرا سرا_Label_E
                        { $$ = new Ast_Gen_بلاک{$<fl>1, *$1, $4, false};
                          GRAMMARP->سرا_Label($<fl>6, *$1, $6); }
        |       id yP_COLON__BEGIN yBEGIN y_سرا سرا_Label_E
                        { $$ = new Ast_Gen_بلاک{$<fl>1, *$1, nullptr, false};
                          GRAMMARP->سرا_Label($<fl>5, *$1, $5); }
        |       yBEGIN ':' idAny ~c~gen_شے_List y_سرا سرا_Label_E
                        { $$ = new Ast_Gen_بلاک{$<fl>3, *$3, $4, false};
                          GRAMMARP->سرا_Label($<fl>6, *$3, $6); }
        |       yBEGIN ':' idAny y_سرا سرا_Label_E
                        { $$ = new Ast_Gen_بلاک{$<fl>3, *$3, nullptr, false};
                          GRAMMARP->سرا_Label($<fl>5, *$3, $5); }
        ;

c_gen_شے_Begin<nodep>:  // IEEE: part of generate_block (for checkers)
                BISONPRE_COPY(gen_شے_Begin,{s/~c~/c_/g})                // {copied}
        ;

i_gen_شے_Begin<nodep>:  // IEEE: part of generate_block (for interfaces)
                BISONPRE_COPY(gen_شے_Begin,{s/~c~/i_/g})                // {copied}
        ;

gen_شے_Or_Begin<nodep>:          // Not in IEEE, but our begin isn't under generate_item
                ~c~generate_شے                        { $$ = $1; }
        |       ~c~gen_شے_Begin                         { $$ = $1; }
        ;

c_gen_شے_Or_Begin<nodep>:  // (for checkers)
                BISONPRE_COPY(gen_شے_Or_Begin,{s/~c~/c_/g})      // {copied}
        ;

i_gen_شے_Or_Begin<nodep>:  // (for interfaces)
                interface_شے                          { $$ = $1; }
        |       i_gen_شے_Begin                          { $$ = $1; }
        ;

gen_شے_List<nodep>:
                ~c~gen_شے_Or_Begin                       { $$ = $1; }
        |       ~c~gen_شے_List ~c~gen_شے_Or_Begin        { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

c_gen_شے_List<nodep>:  // (for checkers)
                BISONPRE_COPY(gen_شے_List,{s/~c~/c_/g})         // {copied}
        ;

i_gen_شے_List<nodep>:  // (for interfaces)
                BISONPRE_COPY(gen_شے_List,{s/~c~/i_/g})         // {copied}
        ;

generate_شے<nodep>:           // IEEE: module_or_interface_or_generate_item
        //                      // Only legal when in a generate under a module (or interface under a module)
                module_or_generate_شے                 { $$ = $1; }
        //                      // Only legal when in a generate under an interface
        //UNSUP interface_or_generate_item              { $$ = $1; }
        //                      // IEEE: checker_or_generate_item
        //                      // Only legal when in a generate under a checker
        //                      // so below in c_generate_item
        ;

c_generate_شے<nodep>:  // IEEE: generate_item (for checkers)
                checker_or_generate_شے                { $$ = $1; }
        ;

conditional_generate_construct<nodep>:  // ==IEEE: conditional_generate_construct
                yCASE '(' اظہاریہ_Type_Compare ')' ~c~case_generate_شے_List yENDCASE
                        { $$ = new AstGenCase{$1, $3, $5}; }
        |       yIF '(' اظہاریہ_Type_Compare ')' ~c~generate_بلاک_or_null %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = new AstGenIf{$1, $3, $5, nullptr}; }
        |       yIF '(' اظہاریہ_Type_Compare ')' ~c~generate_بلاک_or_null yELSE ~c~generate_بلاک_or_null
                        { $$ = new AstGenIf{$1, $3, $5, $7}; }
        ;

c_conditional_generate_construct<nodep>:  // IEEE: conditional_generate_construct (for checkers)
                BISONPRE_COPY(conditional_generate_construct,{s/~c~/c_/g})      // {copied}
        ;

loop_generate_construct<nodep>: // ==IEEE: loop_generate_construct
                yFOR '(' genvar_initialization ';' اظہاریہ ';' genvar_iteration ')' ~c~generate_بلاک_or_null
                        { // Convert BEGIN(...) to BEGIN(GENFOR(...)), as we need the BEGIN to hide the local genvar
                          Ast_Gen_بلاک* lowerp = VN_CAST($9, Gen_بلاک);
                          UASSERT_OBJ(!$9 || lowerp, $9, "Child of GENFOR should have been begin");
                          Ast_گرہ* ثابت itemsp = lowerp && lowerp->itemsp() ? lowerp->itemsp()->unlink_Fr_پٹھ_With_اگلا() : nullptr;
                          Ast_Gen_بلاک* ثابت blkp = new Ast_Gen_بلاک{$1, lowerp ? lowerp->name() : "", nullptr, true};
                          // V3LinkDot detects BEGIN(GENFOR(...)) as a special case
                          Ast_گرہ* initp = $3;
                          Ast_گرہ* ثابت varp = $3;
                          إذا (VN_IS(varp, Var)) {  // Genvar
                                initp = varp->nextp();
                                initp->unlink_Fr_پٹھ_With_اگلا();  // Detach 2nd from varp, make 1st init
                                blkp->جوڑو_Itemsp(varp);
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
                parse_Ref_ادھار '=' اظہاریہ                     { $$ = new Ast_سونپو{$2, $1, $3}; }
        |       yGENVAR genvar_identifierDecl '=' const_اظہاریہ
                        { $$ = $2; Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>($$,
                                       new Ast_سونپو{$3, new AstVarRef{$2->fileline(), $2, VAccess::WRITE}, $4}); }
        ;

genvar_iteration<nodep>:        // ==IEEE: genvar_iteration
                parse_Ref_ادھار '='          اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, $3}; }
        |       parse_Ref_ادھار yP_PLUSEQ    اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new Ast_جوڑو{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_MINUSEQ   اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstSub{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_TIMESEQ   اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstMul{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_DIVEQ     اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstDiv{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_MODEQ     اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstModDiv{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_ANDEQ     اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstAnd{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_OREQ      اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstOr{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_XOREQ     اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new AstXor{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_SLEFTEQ   اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new Ast_کھسکاؤ_L{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_SRIGHTEQ  اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new Ast_کھسکاؤ_R{$2, $1->clone_درخت_خالص(true), $3}}; }
        |       parse_Ref_ادھار yP_SSRIGHTEQ اظہاریہ
                        { $$ = new Ast_سونپو{$2, $1, new Ast_کھسکاؤ_RS{$2, $1->clone_درخت_خالص(true), $3}}; }
        //                      // inc_or_dec_operator
        |       yP_PLUSPLUS   parse_Ref_ادھار
                        { $$ = new Ast_سونپو{$1, $2, new Ast_جوڑو{$1, $2->clone_درخت_خالص(true),
                                                                new AstConst{$1, AstConst::String_منزل_Parse{}, "'b1"}}}; }
        |       yP_MINUSMINUS parse_Ref_ادھار
                        { $$ = new Ast_سونپو{$1, $2, new AstSub{$1, $2->clone_درخت_خالص(true),
                                                                new AstConst{$1, AstConst::String_منزل_Parse{}, "'b1"}}}; }
        |       parse_Ref_ادھار yP_PLUSPLUS
                        { $$ = new Ast_سونپو{$2, $1, new Ast_جوڑو{$2, $1->clone_درخت_خالص(true),
                                                                new AstConst{$2, AstConst::String_منزل_Parse{}, "'b1"}}}; }
        |       parse_Ref_ادھار yP_MINUSMINUS
                        { $$ = new Ast_سونپو{$2, $1, new AstSub{$2, $1->clone_درخت_خالص(true),
                                                                new AstConst{$2, AstConst::String_منزل_Parse{}, "'b1"}}}; }
        ;

case_generate_شے_List<genCaseItemp>:  // IEEE: { case_generate_itemList }
                ~c~case_generate_شے                   { $$ = $1; }
        |       ~c~case_generate_شے_List ~c~case_generate_شے         { $$ = $1; $1->جوڑو_اگلا($2); }
        ;

c_case_generate_شے_List<genCaseItemp>:  // IEEE: { case_generate_item } (for checkers)
                BISONPRE_COPY(case_generate_شے_List,{s/~c~/c_/g})      // {copied}
        ;

case_generate_شے<genCaseItemp>:      // ==IEEE: case_generate_item
                caseCondList colon ~c~generate_بلاک_or_null    { $$ = new Ast_Gen_Case_شے{$2, $1, $3}; }
        |       yDEFAULT colon ~c~generate_بلاک_or_null        { $$ = new Ast_Gen_Case_شے{$1, nullptr, $3}; }
        |       yDEFAULT ~c~generate_بلاک_or_null              { $$ = new Ast_Gen_Case_شے{$1, nullptr, $2}; }
        ;

c_case_generate_شے<genCaseItemp>:  // IEEE: case_generate_item (for checkers)
                BISONPRE_COPY(case_generate_شے,{s/~c~/c_/g})  // {copied}
        ;

//************************************************
// Assignments and register declarations

سونپو_List<alwaysp>:
                سونپو_One                               { $$ = $1; }
        |       سونپو_List ',' سونپو_One                { $$ = $1->جوڑو_اگلا($3); }
        ;

سونپو_One<alwaysp>:
                variable_lvalue '=' اظہاریہ                { Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$2, $1, $3};
                                                          $$ = new AstAlways{ap}; }
        ;

delay_or_event_controlE<nodep>:  // IEEE: delay_or_event_control plus empty
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        |       event_control                           { $$ = $1; }
        |       yREPEAT '(' اظہاریہ ')' event_control
                        { $$ = $5; BBUNSUP($1, "Unsupported: repeat event control"); }
        ;

delay_controlE<delayp>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        ;

delay_control<delayp>:   //== IEEE: delay_control
                '#' delay_قدر
                        { $$ = new AstDelay{$<fl>1, $2, false}; }
        |       '#' '(' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; }
        |       '#' '(' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false};
                          $$->fallDelay($5); }
        |       '#' '(' minTypMax ',' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; $$->fallDelay($5); RISEFALLDLYUNSUP($7); DEL($7); }
        ;

delay_قدر<گرہ_Exprp>:         // ==IEEE:delay_value
        //                      // IEEE: ps_identifier
                idClass                                 { $$ = $1; }
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       ویلا_عدد_Adjusted                         { $$ = $1; }
        |       y1_قدم                                  { $$ = new AstConst{$<fl>1, AstConst::One_قدم{}}; }
        ;

minTypMax<گرہ_Exprp>:           // IEEE: mintypmax_expression and constant_mintypmax_expression
                اظہاریہ                               { $$ = $1; }
        |       اظہاریہ ':' اظہاریہ ':' اظہاریہ { $$ = $3; MINTYPMAXDLYUNSUP($3); DEL($1); DEL($5); }
        ;

minTypMaxE<گرہ_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       minTypMax                               { $$ = $1; }
        ;

netSigList<varp>:               // IEEE: list_of_port_identifiers
                netSig                                  { $$ = $1; }
        |       netSigList ',' netSig                   { $$ = $1; $1->جوڑو_اگلا($3); }
        ;

netSig<varp>:                   // IEEE: net_decl_assignment -  one element from list_of_port_identifiers
                netId variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       netId variable_dimensionListE sigAttrListE '=' اظہاریہ
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          AstDelay* ثابت delayp = $$->delayp() ? $$->delayp()->unlink_Fr_پٹھ() : nullptr;
                          Ast_سونپو_W* ثابت assignp = new Ast_سونپو_W{$4, new AstParseRef{$<fl>1, *$1}, $5, delayp};
                          إذا (GRAMMARP->m_netStrengthp) assignp->strengthSpecp(GRAMMARP->m_netStrengthp->clone_درخت(false));
                          Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>($$, new AstAlways{assignp}); }
        ;

netId<strp>:
                id/*new-net*/                           { $$ = $1; $<fl>$ = $<fl>1; }
        |       idSVKwd                                 { $$ = $1; $<fl>$ = $<fl>1; }
        ;

sigAttrScope:
                y_VL_عوامی_FLAT_RW_ON_SNS attr_event_control
                                                        { GRAMMARP->createScopedSigAttr(VAttrType::VAR_عوامی_FLAT_RW);
                                                          v3Global.dpi(true); DEL($2); }
        |       y_VL_عوامی_ON                           { GRAMMARP->createScopedSigAttr(VAttrType::VAR_عوامی); }
        |       y_VL_عوامی_FLAT_ON                      { GRAMMARP->createScopedSigAttr(VAttrType::VAR_عوامی_FLAT); }
        |       y_VL_عوامی_FLAT_RD_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_عوامی_FLAT_RD); }
        |       y_VL_عوامی_FLAT_RW_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_عوامی_FLAT_RW); }
        |       y_VL_عوامی_OFF                          { GRAMMARP->سیٹ_Scoped_Sig_Attr(nullptr); }
        ;

sigAttrListE<nodep>: // Scoped Attributes are added to explicit attributes
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       sigAttrList                             { $$ = $1; }
        ;

sigAttrList<nodep>:
                sigAttr                                 { $$ = $1; }
        |       sigAttrList sigAttr                     { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

sigAttr<nodep>:
                yVL_CLOCKER                             { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_NO_CLOCKER                          { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_CLOCK_ENABLE                        { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_FORCEABLE                           { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FORCEABLE}; }
        |       y_VL_عوامی                              { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_عوامی}; v3Global.dpi(true); }
        |       y_VL_عوامی_FLAT                         { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_عوامی_FLAT}; v3Global.dpi(true); }
        |       y_VL_عوامی_FLAT_RD                      { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_عوامی_FLAT_RD}; v3Global.dpi(true); }
        |       y_VL_عوامی_FLAT_RW attr_event_controlE  { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_عوامی_FLAT_RW}; v3Global.dpi(true); DEL($2); }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_SC_BIGUINT                          { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_SC_BIGUINT}; }
        |       yVL_SC_BV                               { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_SC_BV}; }
        |       yVL_SFORMAT                             { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_SFORMAT}; }
        |       y_VL_ونڈو_VAR                           { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_ونڈو_VAR}; }
        |       yVL_FSM_ARC_INCL_COND                   { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO              { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO_EXPAND       { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND}; }
        |       y_VL_FSM_ARC_INCL_COND_بھریا              { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_بھریا}; }
        |       yVL_FSM_STATE                           { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_STATE}; }
        |       yVL_FSM_STATE_AUTO                      { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_STATE_AUTO}; }
        |       yVL_FSM_STATE_AUTO_EXPAND               { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_STATE_AUTO_EXPAND}; }
        |       y_VL_FSM_STATE_بھریا                      { $$ = new Ast_Attr_دا{$1, VAttrType::VAR_FSM_STATE_بھریا}; }
        ;

rangeListE<گرہ_Rangep>:         // IEEE: [{packed_dimension}]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       rangeList                               { $$ = $1; }
        ;

rangeList<گرہ_Rangep>:          // IEEE: {packed_dimension}
                anyrange                                { $$ = $1; }
        |       rangeList anyrange                      { $$ = $1->جوڑو_اگلا($2); }
        ;

anyrange<گرہ_Rangep>:
                '[' const_اظہاریہ ':' const_اظہاریہ ']'         { $$ = new AstRange{$1, $2, $4}; }
        ;

part_select_rangeList<گرہ_Pre_Selp>:  // IEEE: part_select_range (as used after function calls)
                part_select_range                        { $$ = $1; }
        |       part_select_rangeList part_select_range  { $$ = GRAMMARP->scrubSel($1, $2); }
        ;

part_select_range<گرہ_Pre_Selp>:
                '[' اظہاریہ ']'
                        { $$ = new Ast_Sel_ثنائی_ہندسہ{$1, new AstParseHolder{$1}, $2}; }
        |       '[' const_اظہاریہ ':' const_اظہاریہ ']'
                        { $$ = new Ast_Sel_کڈھو{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' اظہاریہ yP_PLUSCOLON const_اظہاریہ ']'
                        { $$ = new AstSelPlus{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' اظہاریہ yP_MINUSCOLON const_اظہاریہ ']'
                        { $$ = new AstSelMinus{$1, new AstParseHolder{$1}, $2, $4}; }
        ;

packed_dimensionListE<گرہ_Rangep>:      // IEEE: [{ packed_dimension }]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       packed_dimensionList                    { $$ = $1; }
        ;

packed_dimensionList<گرہ_Rangep>:       // IEEE: { packed_dimension }
                packed_dimension                        { $$ = $1; }
        |       packed_dimensionList packed_dimension   { $$ = $1->جوڑو_اگلا($2); }
        ;

packed_dimension<گرہ_Rangep>:   // ==IEEE: packed_dimension
                anyrange                                { $$ = $1; }
        |       '[' ']'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: [] dimensions"); }
        ;

//************************************************
// Parameters

param_assignment<varp>:         // ==IEEE: param_assignment
        //                      // IEEE: constant_param_expression
        //                      // constant_param_expression: '$' is in expr
                id/*new-parameter*/ variable_dimensionListE sigAttrListE اظہاریہ_Or_انکڑے_Type_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          إذا ($4) $$->valuep($4);
                          وإلا إذا (!GRAMMARP->m_pinAnsi)
                              $$->v3warn(PARAMNODEFAULT, "Parameter without default requires"
                                         " ANSI-style parameter list (IEEE 1800-2023 6.20.1): "
                                         << $$->prettyNameQ()); }
        ;

list_دا_param_assignments<varp>:        // ==IEEE: list_of_param_assignments
                param_assignment                        { $$ = $1; }
        |       list_دا_param_assignments ',' param_assignment  { $$ = $1->جوڑو_اگلا($3); }
        ;

type_assignment<varp>:          // ==IEEE: type_assignment
        //                      // note exprOrDataType being a data_type is only for yPARAMETER yTYPE
        //                      // Using exprOrDataType allows hierarchical refs like if0.rq_t
        //                      // which get resolved to types during linking
                idAny/*new-parameter*/ sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        |       idAny/*new-parameter*/ sigAttrListE '=' اظہاریہ_Or_انکڑے_Type
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          // V3LinkParse will wrap this in RequireDType when creating ParamTypeDType
                          $$->valuep($4); }
        ;

list_دا_type_assignments<varp>:         // ==IEEE: list_of_type_assignments
                type_assignment                         { $$ = $1; }
        |       list_دا_type_assignments ',' type_assignment
                        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

list_دا_defparam_assignments<nodep>:    //== IEEE: list_of_defparam_assignments
                defparam_assignment                     { $$ = $1; }
        |       list_دا_defparam_assignments ',' defparam_assignment
                        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

defparam_assignment<nodep>:     // ==IEEE: defparam_assignment
                defparamIdRangeList '.' defparamIdRange '=' اظہاریہ
                        { $$ = new AstDefParam{$4, $1, *$3, $5}; }
        |       defparamIdRange '=' اظہاریہ
                        { $$ = nullptr; BBUNSUP($2, "Unsupported: defparam with no dot");
                          DEL($3); }
        ;

defparamIdRangeList<گرہ_Exprp>:  // IEEE: part of defparam_assignment
                defparam_Id_Range_اظہاریہ                     { $$ = $1; }
        |       defparamIdRangeList '.' defparam_Id_Range_اظہاریہ
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

defparam_Id_Range_اظہاریہ<گرہ_Exprp>:  // IEEE: part of defparam_assignment
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
                idInst parameter_قدر_assignment_Inst_E
        /*mid*/         { INSTPREP($<fl>1, *$1, $2); }
        /*cont*/    instnameList ';'
                        { $$ = $4;
                          GRAMMARP->m_impliedDecl = false;
                          إذا (GRAMMARP->m_instParamp) {
                              VL_DO_CLEAR(GRAMMARP->m_instParamp->delete_درخت(),
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
        |       mpInstnameList ',' mpInstnameParen      { $$ = $1->جوڑو_اگلا($3); }
        ;

mpInstnameParen<nodep>:         // Similar to instnameParen, but for modport instantiations which have no parenthesis
                id instRangeListE sigAttrListE          { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        ;

instnameList<nodep>:
                instnameParen                           { $$ = $1; }
        |       instnameList ',' instnameParen          { $$ = $1->جوڑو_اگلا($3); }
        ;

instnameParen<nodep>:
                id instRangeListE '(' instPinListE ')'
                        { $$ = GRAMMARP->createCell($<fl>1, *$1, $4, $2); }
        ;

instnameParenUdpn<nodep>:  // IEEE: part of udp_instance when no name_of_instance
                '(' instPinListE ')'  // When UDP has empty name, unpacked dimensions must not be used
                        { $$ = GRAMMARP->createCell($<fl>1, "", $2, nullptr); }
        ;

instRangeListE<گرہ_Rangep>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       instRangeList                           { $$ = $1; }
        ;

instRangeList<گرہ_Rangep>:
                instRange                               { $$ = $1; }
        |       instRangeList instRange                 { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

instRange<گرہ_Rangep>:
                '[' const_اظہاریہ ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstSub{$1, $2, new AstConst{$1, 1}}, true}; }
        |       '[' const_اظہاریہ ':' const_اظہاریہ ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

instParamListE<pinp>:
                { GRAMMARP->pin_ڈالو(); } instParamItListE   { $$ = $2; GRAMMARP->pin_نکالو(CRELINE()); }
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
                inst_Param_شے                           { $$ = $1; }
        |       instParamItList ',' inst_Param_شے       { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

instPinItListE<pinp>:           // IEEE: list_of_port_connections
                inst_Pin_شے_E                            { $$ = $1; }
        |       instPinItListE ',' inst_Pin_شے_E         { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

inst_Param_شے<pinp>:            // IEEE: named_parameter_assignment + empty
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
        |       '.' idAny '(' اظہاریہ_Or_انکڑے_Type ')'
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
        |       اظہاریہ_Or_انکڑے_Type
                        { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1); DEL($5);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        ;

inst_Pin_شے_E<pinp>:             // IEEE: named_port_connection + empty
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
        |       '.' idAny '(' اظہاریہ ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $4};
                          $$->svDotName(true); }
        //UNSUP '.' idAny '(' pev_expr ':' expr ')'     { }
        //UNSUP '.' idAny '(' pev_expr ':' expr ':' expr ')' { }
        //
        |       اظہاریہ                                    { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP expr ':' expr                           { }
        //UNSUP expr ':' expr ':' expr                  { }
        ;

//************************************************
// EventControl lists

attr_event_controlE<senTreep>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       attr_event_control                      { $$ = $1; }
        ;

attr_event_control<senTreep>:   // ==IEEE: event_control
                '@' '(' event_عبارت ')'            { $$ = new Ast_Sen_درخت{$1, $3}; }
        |       '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_درخت_Dot_تارا($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_درخت_Dot_تارا($1); }
        ;

event_control<senTreep>:        // ==IEEE: event_control
        // UNSUP: Needs alignment with IEEE event_control and clocking_event
                '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_درخت_Dot_تارا($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_درخت_Dot_تارا($1); }
        //                      // IEEE: clocking_event
        |       '@' '(' event_عبارت ')'            { $$ = new Ast_Sen_درخت{$1, $3}; }
        //                      // IEEE: hierarchical_event_identifier
        //                      // UNSUP below should be idClassSel
        |       '@' senitemVar                          { $$ = new Ast_Sen_درخت{$1, $2}; } /* For events only */
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

event_عبارت<senItemp>:     // IEEE: event_expression - split over several
        //UNSUP                 // Below are all removed
                senitem                                 { $$ = $1; }
        |       event_عبارت yOR senitem            { $$ = جوڑو_اگلا_Null($1, $3); }
        |       event_عبارت ',' senitem            { $$ = جوڑو_اگلا_Null($1, $3); } /* Verilog 2001 */
        //UNSUP                 // Above are all removed, replace with:
        //UNSUP ev_expr                                 { $$ = $1; }
        //UNSUP event_expression ',' ev_expr %prec yOR  { $$ = addNextNull($1, $3); }
        ;

senitem<senItemp>:              // IEEE: part of event_expression, non-'OR' ',' terms
                senitem_کنارا                             { $$ = $1; }
        |       اظہاریہ
                        { $$ = new Ast_Sen_شے{$<fl>1, VEdgeType::ET_بدلیا, $1}; }
        |       اظہاریہ yIFF اظہاریہ
                        { $$ = new Ast_Sen_شے{$<fl>1, VEdgeType::ET_بدلیا, $1, $3}; }
        ;

senitemVar<senItemp>:
                idClassSel
                        { $$ = new Ast_Sen_شے{$1->fileline(), VEdgeType::ET_بدلیا, $1}; }
        ;

senitem_کنارا<senItemp>:          // IEEE: part of event_expression
                yPOSEDGE اظہاریہ
                        { $$ = new Ast_Sen_شے{$1, VEdgeType::ET_POSEDGE, $2}; }
        |       yNEGEDGE اظہاریہ
                        { $$ = new Ast_Sen_شے{$1, VEdgeType::ET_NEGEDGE, $2}; }
        |       y_کنارا اظہاریہ
                        { $$ = new Ast_Sen_شے{$1, VEdgeType::ET_BOTHEDGE, $2}; }
        |       yPOSEDGE اظہاریہ yIFF اظہاریہ
                        { $$ = new Ast_Sen_شے{$1, VEdgeType::ET_POSEDGE, $2, $4}; }
        |       yNEGEDGE اظہاریہ yIFF اظہاریہ
                        { $$ = new Ast_Sen_شے{$1, VEdgeType::ET_NEGEDGE, $2, $4}; }
        |       y_کنارا اظہاریہ yIFF اظہاریہ
                        { $$ = new Ast_Sen_شے{$1, VEdgeType::ET_BOTHEDGE, $2, $4}; }
        ;

//************************************************
// Statements

seq_بلاک<beginp>:               // ==IEEE: seq_block
        //                      // IEEE doesn't allow declarations in unnamed blocks, but several simulators do.
        //                      // So need AstBegin's even if unnamed to scope variables down
                yBEGIN شروع_Label_E بلاک_Decl_List_E stmtListE y_سرا سرا_Label_E
                        {
                            $$ = new AstBegin{$1, $2 ? *$2 : "", nullptr, false};
                            GRAMMARP->سرا_Label($<fl>6, $$, $6);
                            $$->جوڑو_Declsp($3);
                            $$->جوڑو_Stmtsp($4);
                        }
        ;

seq_بلاک_Pre_Id<beginp>:          // IEEE: seq_block, but called with leading ID
                id yP_COLON__BEGIN yBEGIN بلاک_Decl_List_E stmtListE y_سرا سرا_Label_E
                        {
                            $$ = new AstBegin{$3, *$1, nullptr, false};
                            GRAMMARP->سرا_Label($<fl>7, $$, $7);
                            $$->جوڑو_Declsp($4);
                            $$->جوڑو_Stmtsp($5);
                        }
        ;

par_بلاک_Join<joinType>:
                yJOIN       { $$ = VJoinType::JOIN; }
        |       yJOIN_ANY   { $$ = VJoinType::JOIN_ANY; }
        |       yJOIN_NONE  { $$ = VJoinType::JOIN_NONE; }
        ;

par_بلاک<forkp>:               // ==IEEE: par_block
                yFORK شروع_Label_E بلاک_Decl_List_E stmtListE par_بلاک_Join سرا_Label_E
                        {
                            $$ = new AstFork{$1, $5, $2 ? *$2 : ""};
                            GRAMMARP->سرا_Label($<fl>6, $$, $6);
                            $$->جوڑو_Declsp($3);
                            $$->جوڑو_Forksp(V3ParseGrammar::wrap_وچ_Begin($4));
                        }
        ;

par_بلاک_Pre_Id<forkp>:          // ==IEEE: par_block but called with leading ID
                id yP_COLON__FORK yFORK بلاک_Decl_List_E stmtListE par_بلاک_Join سرا_Label_E
                        {
                            $$ = new AstFork{$3, $6, *$1};
                            GRAMMARP->سرا_Label($<fl>7, $$, $7);
                            $$->جوڑو_Declsp($4);
                            $$->جوڑو_Forksp(V3ParseGrammar::wrap_وچ_Begin($5));
                        }
            ;

بلاک_Decl_List_E<nodep>:      // IEEE: [ block_item_declaration ]
                /*empty*/                               { $$ = nullptr; }
        |       بلاک_Decl_List_E بلاک_شے_declaration   { $$ = جوڑو_اگلا_Null($1, $2); }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

بلاک_شے_declaration<nodep>:  // ==IEEE: block_item_declaration
                انکڑے_declaration                        { $$ = $1; }
        |       parameter_declaration ';'               { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

stmtListE<گرہ_Stmtp>:
                /*empty*/                               { $$ = nullptr; }
        |       stmtList                                { $$ = $1; }
        ;

stmtList<گرہ_Stmtp>:
                stmt                               { $$ = $1; }
        |       stmtList stmt                      { $$ = جوڑو_اگلا_Null($1, $2); }
        //
        |       stmtList error ';'                      { $$ = $1; }  // LCOV_EXCL_LINE
        ;

stmt<گرہ_Stmtp>:                    // IEEE: statement + statement_or_null + seq_block + par_block
                statement_شے                          { $$ = $1; }
        //                      // S05 block creation rule
        |       id/*block_identifier*/ ':' statement_شے       { $$ = new AstBegin{$<fl>1, *$1, $3, false}; }
        //                      // from _or_null
        |       ';'                                     { $$ = nullptr; }
        //                      // labeled par_block/seq_block with leading ':'
        |       seq_بلاک_Pre_Id                          { $$ = $1; }
        |       par_بلاک_Pre_Id                          { $$ = $1; }
        ;

statement_شے<گرہ_Stmtp>:          // IEEE: statement_item
        //                      // IEEE: operator_assignment
                foperator_assignment ';'                { $$ = $1; }
        //
        //                      // IEEE: blocking_assignment
        //                      // 1800-2009 restricts LHS of assignment to new to not have a range
        //                      // This is ignored to avoid conflicts
        |       fexprLvalue yP_EQ__NEW dynamic_array_new ';'   { $$ = new Ast_سونپو{$2, $1, $3}; }
        |       fexprLvalue yP_EQ__NEW class_new ';'    { $$ = new Ast_سونپو{$2, $1, $3}; }
        //                      // IEEE: inc_or_dec_expression
        |       finc_or_dec_عبارت ';'              { $$ = new Ast_Stmt_اظہاریہ{$<fl>1, $1}; }
        //
        //                      // IEEE: nonblocking_assignment
        |       fexprLvalue yP_LTE delay_or_event_controlE اظہاریہ ';'
                        { $$ = new Ast_سونپو_Dly{$2, $1, $4, $3}; }
        //                      // IEEE: clocking_drive ';'
        |       fexprLvalue yP_LTE چکر_delay اظہاریہ ';'
                        { $$ = new Ast_سونپو_Dly{$2, $1, $4, $3}; }
        //UNSUP cycle_delay fexprLvalue yP_LTE ';'      { UNSUP }
        |       y_سونپو variable_lvalue '=' delay_or_event_controlE اظہاریہ ';'
                        { $$ = new Ast_سونپو_Cont{$1, $2, $5, $4};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.سیٹ_Has_سونپو_Deassign(); }
        |       yDEASSIGN variable_lvalue ';'
                        { $$ = new AstDeassign{$1, $2};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.سیٹ_Has_سونپو_Deassign(); }
        |       yFORCE variable_lvalue '=' اظہاریہ ';'
                        { $$ = new Ast_سونپو_Force{$1, $2, $4}; v3Global.سیٹ_Has_Forceable_اشارے(); }
        |       yRELEASE variable_lvalue ';'
                        { $$ = new AstRelease{$1, $2}; v3Global.سیٹ_Has_Forceable_اشارے(); }
        //
        //                      // IEEE: case_statement
        |       unique_ترجیح_E case_شروع caseAttrE case_شے_List yENDCASE
                        { $$ = $2; إذا ($4) $2->جوڑو_Itemsp($4);
                          إذا ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          إذا ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          إذا ($1 == uniq_ترجیح) $2->ترجیح_Pragma(true); }
        // case matches uses patterns, not expressions
        |       unique_ترجیح_E case_شروع caseAttrE yMATCHES case_matches_شے_List yENDCASE
                        { $$ = $2; إذا ($5) $2->جوڑو_Itemsp($5);
                          $2->case_Matches_سیٹ();
                          إذا ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          إذا ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          إذا ($1 == uniq_ترجیح) $2->ترجیح_Pragma(true); }
        |       unique_ترجیح_E case_شروع caseAttrE yINSIDE case_inside_شے_List yENDCASE
                        { $$ = $2; إذا ($5) $2->جوڑو_Itemsp($5);
                          إذا (!$2->caseSimple()) $4->v3error("Illegal to have inside on a casex/casez");
                          $2->case_Inside_سیٹ();
                          إذا ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          إذا ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          إذا ($1 == uniq_ترجیح) $2->ترجیح_Pragma(true); }
        //
        //                      // IEEE: conditional_statement
        |       unique_ترجیح_E yIF '(' اظہاریہ ')' stmt     %prec pr_ہیٹھلا_THAN_ELSE
                        { AstIf* ثابت newp = new AstIf{$2, $4,
                              PARSEP->new_بلاک($2, $6)};
                          $$ = newp;
                          إذا ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          إذا ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          إذا ($1 == uniq_ترجیح) newp->ترجیح_Pragma(true); }
        |       unique_ترجیح_E yIF '(' اظہاریہ ')' stmt yELSE stmt
                        { AstIf* ثابت newp = new AstIf{$2, $4,
                              PARSEP->new_بلاک($2, $6),
                              PARSEP->new_بلاک($2, $8)};
                          $$ = newp;
                          إذا ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          إذا ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          إذا ($1 == uniq_ترجیح) newp->ترجیح_Pragma(true); }
        //
        //                      // IEEE: subroutine_call_statement
        //                      // IEEE says we then expect a function call
        //                      // (function_subroutine_callNoMethod), but rest of
        //                      // the code expects an AstTask when used as a statement,
        //                      // so parse as if task
        //                      // Alternative would be shim with new AstVoidStmt.
        |       yVOID yP_TICK '(' task_subroutine_call_No_طریقہ ')' ';'
                        { Ast_گرہ_اظہاریہ* ثابت exprp = $4;
                          Ast_گرہ* callp = exprp;
                          طالما (AstDot* ثابت dotp = VN_CAST(callp, Dot)) callp = dotp->rhsp();
                          File_سطر* ثابت newfl = new File_سطر{callp->fileline()};
                          newfl->warnOff(V3_Error_کوڈ::IGNOREDRETURN, true);
                          callp->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' اظہاریہ '.' task_subroutine_call_No_طریقہ ')' ';'
                        { Ast_گرہ_اظہاریہ* ثابت exprp = new AstDot{$5, false, $4, $6};
                          File_سطر* ثابت newfl = new File_سطر{$6->fileline()};
                          newfl->warnOff(V3_Error_کوڈ::IGNOREDRETURN, true);
                          $6->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' system_f_only_اظہاریہ_call ')' ';'
                        { $$ = new Ast_Stmt_اظہاریہ{$<fl>4, $4};
                          File_سطر* ثابت newfl = new File_سطر{$$->fileline()};
                          newfl->warnOff(V3_Error_کوڈ::IGNOREDRETURN, true);
                          $$->fileline(newfl); }
        //                      // Any system function as a task
        |       yVOID yP_TICK '(' system_f_or_t_اظہاریہ_call ')' ';'
                        { $$ = new Ast_Stmt_اظہاریہ{$<fl>4, $4};
                          File_سطر* ثابت newfl = new File_سطر{$$->fileline()};
                          newfl->warnOff(V3_Error_کوڈ::IGNOREDRETURN, true);
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
                          PARSEP->import_If_وچ_Std($1, "process", true);
                        }
        //                      // IEEE: event_trigger
        |       yP_MINUSGT اظہاریہ ';'
                        { $$ = new AstFireEvent{$1, $2, false}; }
        |       yP_MINUSGTGT delay_or_event_controlE اظہاریہ ';'
                        { $$ = new AstFireEvent{$1, $3, true}; }
        //
        // do/for/forever/while loops all modelled as AstLoop
        |       yDO stmt yWHILE '(' اظہاریہ ')' ';'
                        { AstLoop* ثابت loopp = new AstLoop{$1, $2};
                          loopp->جوڑو_Contsp(new AstLoopTest{$<fl>5, loopp, $5});
                          $$ = loopp; }
        |       yFOR  '(' { VARRESET_NONLIST(UNKNOWN); } for_initializationE ';' اظہاریہ_E ';' for_قدم_E ')' stmt
                        { AstBegin* ثابت blockp = new AstBegin{$1, "", $4, true};
                          AstLoop* ثابت loopp = new AstLoop{$1};
                          إذا ($6) loopp->جوڑو_Stmtsp(new AstLoopTest{$<fl>6, loopp, $6});
                          loopp->جوڑو_Stmtsp($10);
                          loopp->جوڑو_Contsp($8);
                          blockp->جوڑو_Stmtsp(loopp);
                          $$ = blockp; }
        |       yFOREVER stmt
                        { AstLoop* ثابت loopp = new AstLoop{$1, $2};
                          $$ = loopp; }
        |       yWHILE '(' اظہاریہ ')' stmt
                        { AstLoop* ثابت loopp = new AstLoop{$1};
                          loopp->جوڑو_Stmtsp(new AstLoopTest{$<fl>3, loopp, $3});
                          loopp->جوڑو_Stmtsp($5);
                          $$ = loopp; }
        // Other loop statements
        |       yREPEAT '(' اظہاریہ ')' stmt          { $$ = new AstRepeat{$1, $3, $5}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM and 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' stmt
                        { $$ = new AstBegin{$1, "", new AstForeach{$1, $3, $5}, true}; }
        //
        //                      // IEEE: jump_statement
        |       yRETURN ';'                             { $$ = new AstReturn{$1}; }
        |       yRETURN اظہاریہ ';'                        { $$ = new AstReturn{$1, $2}; }
        |       yBREAK ';'                              { $$ = new AstBreak{$1}; }
        |       yCONTINUE ';'                           { $$ = new AstContinue{$1}; }
        //
        |       par_بلاک                               { $$ = $1; }
        //                      // IEEE: procedural_timing_control_statement + procedural_timing_control
        |       delay_control stmt
                        { Ast_گرہ_Stmt* nextp = nullptr;
                          إذا ($2) {
                              إذا ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_پٹھ_With_اگلا(), گرہ_Stmt);
                              $1->جوڑو_Stmtsp($2);
                          }
                          $$ = $1;
                          جوڑو_اگلا_Null($$, nextp); }
        |       event_control stmt
                        { Ast_گرہ_Stmt* nextp = nullptr;
                          إذا ($2 && $2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_پٹھ_With_اگلا(), گرہ_Stmt);
                          $$ = new AstEventControl{FILELINE_OR_CRE($1), $1, $2};
                          جوڑو_اگلا_Null($$, nextp); }
        |       چکر_delay stmt
                        { Ast_گرہ_Stmt* nextp = nullptr;
                          إذا ($2) {
                              إذا ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_پٹھ_With_اگلا(), گرہ_Stmt);
                              $1->جوڑو_Stmtsp($2);
                          }
                          $$ = $1;
                          جوڑو_اگلا_Null($$, nextp); }
        |       seq_بلاک                               { $$ = $1; }
        //
        //                      // IEEE: wait_statement
        |       yWAIT '(' اظہاریہ ')' stmt            { $$ = new AstWait{$1, $3, $5}; }
        |       yWAIT yFORK ';'                         { $$ = new AstWaitFork{$1}; }
        //                      // action_block expanded here
        |       y_WAIT_ترتیب '(' vrdList ')' stmt %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5); }
        |       y_WAIT_ترتیب '(' vrdList ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5, $7);}
        |       y_WAIT_ترتیب '(' vrdList ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $6); }
        //
        //                      // IEEE: procedural_assertion_statement
        |       procedural_assertion_statement          { $$ = $1; }
        //
        |       randsequence_statement                  { $$ = $1; }
        //
        //                      // IEEE: randcase_statement
        |       yRANDCASE rand_case_شے_List yENDCASE   { $$ = new AstRandCase{$1, $2}; }
        //
        //                      // IEEE: expect_property_statement
        //                      // action_block expanded here
        |       yEXPECT '(' property_spec ')' stmt %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5); }
        |       yEXPECT '(' property_spec ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5, $7); }
        |       yEXPECT '(' property_spec ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $6); }
        ;

statementVerilatorPragmas<pragmap>:
                y_VL_COVERAGE_بلاک_OFF
                        { $$ = new AstPragma{$1, VPragmaType::COVERAGE_بلاک_OFF}; }
        |       yVL_UNROLL_DISABLE
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_DISABLE}; }
        |       y_VL_UNROLL_بھریا
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_بھریا}; }
        ;

foperator_assignment<گرہ_Stmtp>:    // IEEE: operator_assignment (for first part of expression)
                fexprLvalue '=' delay_or_event_controlE اظہاریہ    { $$ = new Ast_سونپو{$2, $1, $4, $3}; }
        //
        |       fexprLvalue yP_PLUSEQ    اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::جوڑو_2, $2, $1, $3}; }
        |       fexprLvalue yP_MINUSEQ   اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::Sub, $2, $1, $3}; }
        |       fexprLvalue yP_TIMESEQ   اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::Mul, $2, $1, $3}; }
        |       fexprLvalue yP_DIVEQ     اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::Div, $2, $1, $3}; }
        |       fexprLvalue yP_MODEQ     اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::ModDiv, $2, $1, $3}; }
        |       fexprLvalue yP_ANDEQ     اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::And, $2, $1, $3}; }
        |       fexprLvalue yP_OREQ      اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::Or, $2, $1, $3}; }
        |       fexprLvalue yP_XOREQ     اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::Xor, $2, $1, $3}; }
        |       fexprLvalue yP_SLEFTEQ   اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::کھسکاؤ_L, $2, $1, $3}; }
        |       fexprLvalue yP_SRIGHTEQ  اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::کھسکاؤ_R, $2, $1, $3}; }
        |       fexprLvalue yP_SSRIGHTEQ اظہاریہ
                        { $$ = new Ast_سونپو_Compound{Ast_سونپو_Compound::operation::کھسکاؤ_RS, $2, $1, $3}; }
        ;

inc_or_dec_عبارت<گرہ_Exprp>:   // ==IEEE: inc_or_dec_expression
        //                      // Need fexprScope instead of variable_lvalue to prevent conflict
                ~l~اظہاریہ_Scope yP_PLUSPLUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostInc{$2, $1}; }
        |       ~l~اظہاریہ_Scope yP_MINUSMINUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostDec{$2, $1}; }
        //                      // Need expr instead of variable_lvalue to prevent conflict
        |       yP_PLUSPLUS     اظہاریہ
                        { $<fl>$ = $<fl>1; $$ = new AstPreInc{$1, $2}; }
        |       yP_MINUSMINUS   اظہاریہ
                        { $<fl>$ = $<fl>1; $$ = new AstPreDec{$1, $2}; }
        ;

finc_or_dec_عبارت<گرہ_Exprp>:  // ==IEEE: inc_or_dec_expression
                BISONPRE_COPY(inc_or_dec_عبارت,{s/~l~/f/g})        // {copied}
        ;

sinc_or_dec_عبارت<گرہ_Exprp>:  // IEEE: inc_or_dec_expression (for sequence_expression)
                BISONPRE_COPY(inc_or_dec_عبارت,{s/~l~/s/g})        // {copied}
        ;

pinc_or_dec_عبارت<گرہ_Exprp>:  // IEEE: inc_or_dec_expression (for property_expression)
                BISONPRE_COPY(inc_or_dec_عبارت,{s/~l~/p/g})        // {copied}
        ;

//UNSUPev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for ev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/ev_/g})      // {copied}
//UNSUP ;

//UNSUPpev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for pev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/pev_/g})     // {copied}
//UNSUP ;

class_new<گرہ_Exprp>:    // IEEE: class_new
        //                      // See V3ParseImp::tokenPipeScanEqNew that searches for '=' ... yNEW__LEX
                class_newNoScope
                        { $$ = $1; }
        //                      // Special precedence so (...) doesn't match expr
        //                      // A scope is not legal in front of a AstNewCopy
        |       packageClassScopeNoId yNEW__ETC
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2,  nullptr, true}); }
        |       packageClassScopeNoId yNEW__PAREN '(' list_دا_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2, $4, true}); }
        ;

class_newNoScope<گرہ_Exprp>:    // IEEE: class_new but no packageClassScope
        //                      // Special precedence so (...) doesn't match expr
                yNEW__ETC                               { $$ = new AstNew{$1}; }
        |       yNEW__ETC اظہاریہ                          { $$ = new AstNewCopy{$1, $2}; }
        |       yNEW__PAREN '(' list_دا_arguments_E ')'  { $$ = new AstNew{$1, $3}; }
        ;

dynamic_array_new<گرہ_Exprp>:   // ==IEEE: dynamic_array_new
                yNEW__ETC '[' اظہاریہ ']'                  { $$ = new AstNewDynamic{$1, $3, nullptr}; }
        |       yNEW__ETC '[' اظہاریہ ']' '(' اظہاریہ ')'     { $$ = new AstNewDynamic{$1, $3, $6}; }
        ;

//************************************************
// Case/If

unique_ترجیح_E<uniqstate>:    // IEEE: unique_priority + empty
                /*empty*/                               { $$ = uniq_NONE; }
        |       y_ترجیح                               { $$ = uniq_ترجیح; }
        |       yUNIQUE                                 { $$ = uniq_UNIQUE; }
        |       yUNIQUE0                                { $$ = uniq_UNIQUE0; }
        ;

case_شروع<casep>:               // IEEE: part of case_statement
                yCASE  '(' اظہاریہ_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASE, $3, nullptr}; }
        |       yCASEX '(' اظہاریہ_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEX, $3, nullptr}; }
        |       yCASEZ '(' اظہاریہ_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEZ, $3, nullptr}; }
        ;

caseAttrE:
                /*empty*/                               { }
        |       caseAttrE y_VL_بھریا_CASE                 { GRAMMARP->m_caseAttrp->بھریا_Pragma(true); }
        |       caseAttrE yVL_PARALLEL_CASE             { GRAMMARP->m_caseAttrp->parallelPragma(true); }
        ;

case_شے_List<caseItemp>:       // IEEE: { case_item + ... }
                caseCondList colon stmt                    { $$ = new Ast_Case_شے{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_شے{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_شے{$1, nullptr, $2}; }
        |       case_شے_List caseCondList colon stmt      { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$3, $2, $4}); }
        |       case_شے_List yDEFAULT stmt                { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$2, nullptr, $3}); }
        |       case_شے_List yDEFAULT colon stmt          { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$2, nullptr, $4}); }
        ;

case_inside_شے_List<caseItemp>:        // IEEE: { case_inside_item + range_list ... }
                range_list colon stmt                      { $$ = new Ast_Case_شے{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_شے{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_شے{$1, nullptr, $2}; }
        |       case_inside_شے_List range_list colon stmt { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$3, $2, $4}); }
        |       case_inside_شے_List yDEFAULT stmt         { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$2, nullptr, $3}); }
        |       case_inside_شے_List yDEFAULT colon stmt   { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$2, nullptr, $4}); }
        ;

case_matches_شے_List<caseItemp>:    // IEEE: { case_pattern_item + ... }
        //                      // IEEE: case_pattern_item ::= pattern [&&& expr] : stmt
        //                      // pattern includes expr for tagged void members (tagged id)
                نمونہ_No_اظہاریہ colon stmt                   { $$ = new Ast_Case_شے{$2, $1, $3}; }
        |       اظہاریہ colon stmt                            { $$ = new Ast_Case_شے{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_شے{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_شے{$1, nullptr, $2}; }
        |       case_matches_شے_List نمونہ_No_اظہاریہ colon stmt
                        { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$3, $2, $4}); }
        |       case_matches_شے_List اظہاریہ colon stmt
                        { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$3, $2, $4}); }
        |       case_matches_شے_List yDEFAULT stmt        { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$2, nullptr, $3}); }
        |       case_matches_شے_List yDEFAULT colon stmt  { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$2, nullptr, $4}); }
        ;

rand_case_شے_List<caseItemp>:       // IEEE: { rand_case_item + ... }
        //                      // Randcase syntax doesn't have default, or expression lists
                اظہاریہ colon stmt                            { $$ = new Ast_Case_شے{$2, $1, $3}; }
        |       rand_case_شے_List اظہاریہ colon stmt         { $$ = $1->جوڑو_اگلا(new Ast_Case_شے{$3, $2, $4}); }
        ;

range_list<گرہ_Exprp>:     // ==IEEE: range_list/open_range_list + value_range/open_value_range
                قدر_range                             { $$ = $1; }
        |       range_list ',' قدر_range              { $$ = $1->جوڑو_اگلا($3); }
        ;

قدر_range<گرہ_Exprp>:         // ==IEEE: value_range/open_value_range
                اظہاریہ                                    { $$ = $1; }
        |       '[' اظہاریہ ':' اظہاریہ ']'                   { $$ = new AstInsideRange{$1, $2, $4}; }
        //                      // IEEE-2023: added all four:
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' '$' ':' expr ']'
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' expr ':' '$' ']'
        |       '[' اظہاریہ yP_PLUSSLASHMINUS اظہاریہ ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +/- range"); DEL($2, $4); }
        |       '[' اظہاریہ yP_PLUSPCTMINUS اظہاریہ ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +%- range"); DEL($2, $4); }
        ;

covergroup_قدر_range<گرہ_Exprp>:  // ==IEEE-2012: covergroup_value_range
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

caseCondList<گرہ_Exprp>:        // IEEE: part of case_item
                اظہاریہ_Type_Compare                         { $$ = $1; }
        |       caseCondList ',' اظہاریہ_Type_Compare        { $$ = $1->جوڑو_اگلا($3); }
        ;

نمونہ_No_اظہاریہ<گرہ_Exprp>:       // IEEE: pattern **Excluding Expr*
                '.' idAny/*variable*/
                        { $$ = new Ast_نمونہ_Var{$1, *$2}; }
        |       yP_DOTSTAR
                        { $$ = new Ast_نمونہ_تارا{$1}; }
        //                      // IEEE: "expr" excluded; expand in callers
        //                      // IEEE: tagged member_identifier [ pattern ]
        //                      // Standalone "yTAGGED__NONPRIMARY idAny" is handled via expr in patternOne
        //                      // Here, we need to treat yTAGGED and yTAGGED__NONPRIMARY identically.
        |       yTAGGED idAny/*member_identifier*/ نمونہ_No_اظہاریہ
                        { $$ = new Ast_Tagged_نمونہ{$1, *$2, $3}; }
        |       yTAGGED__NONPRIMARY idAny/*member_identifier*/ نمونہ_No_اظہاریہ
                        { $$ = new Ast_Tagged_نمونہ{$1, *$2, $3}; }
        //                      // "yP_TICKBRA patternList '}'" part of expr under assignment_pattern
        ;

نمونہ_List<nodep>:             // IEEE: part of pattern
                نمونہ_One                              { $$ = $1; }
        |       نمونہ_List ',' نمونہ_One              { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

نمونہ_One<nodep>:              // IEEE: part of pattern
                اظہاریہ
                        { إذا ($1) $$ = new AstPatMember{$1->fileline(), $1, nullptr, nullptr}; وإلا $$ = nullptr; }
        |       اظہاریہ '{' دلیلاں_اظہاریہ_List '}'               { $$ = new AstPatMember{$2, $3, nullptr, $1}; }
        |       نمونہ_No_اظہاریہ                           { $$ = $1; }
        ;

نمونہ_Member_List<nodep>:       // IEEE: part of pattern and assignment_pattern
                نمونہ_Member_One                        { $$ = $1; }
        |       نمونہ_Member_List ',' نمونہ_Member_One  { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

نمونہ_Member_One<patMemberp>:   // IEEE: part of pattern and assignment_pattern
                نمونہ_کلید ':' اظہاریہ                     { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        |       نمونہ_کلید ':' نمونہ_No_اظہاریہ            { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        //                      // From assignment_pattern_key
        |       yDEFAULT ':' اظہاریہ                       { $$ = new AstPatMember{$1, $3, nullptr, nullptr}; $$->isDefault(true); }
        |       yDEFAULT ':' نمونہ_No_اظہاریہ              { AstPatMember* ثابت patp = new AstPatMember{$1, $3, nullptr, nullptr}; patp->isDefault(true); $$ = patp; }
        ;

نمونہ_کلید<nodep>:              // IEEE: merge structure_pattern_key, array_pattern_key, assignment_pattern_key
        //                      // IEEE: structure_pattern_key: member_identifier | assignment_pattern_key
        //                      // IEEE: array_pattern_key: constant_expression | assignment_pattern_key
        //                      // Verilator:
        //                      //   A bare "foo" is ambiguous here, as it may be a constant
        //                      //   identifier (if array) or a reference to the "foo" member
        //                      //   (if structure).  Both spell the same in expr, so the
        //                      //   bare-identifier case becomes a Text node and V3LinkDot
        //                      //   resolves which one it is.
                اظہاریہ
                        { $$ = GRAMMARP->create_نمونہ_کلید($1); }
        //                      // IEEE: assignment_pattern_key
        |       simple_typeNoRef
                        { $$ = $1; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // (simple_type ps_parameter_identifier is part of expr above)
        |       packageClassScopeE idType
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = refp; }
        ;

assignment_نمونہ<patternp>:   // ==IEEE: assignment_pattern
        // This doesn't match the text of the spec.  I think a : is missing, or example code needed
        // yP_TICKBRA constExpr exprList '}'    { $$="'{"+$2+" "+$3"}"; }
        //                      // "'{ const_expression }" is same as patternList with one entry
        //                      // From patternNoExpr
        //                      // also IEEE: "''{' expression { ',' expression } '}'"
        //                      //      matches since patternList includes expr
                yP_TICKBRA نمونہ_List '}'              { $$ = new Ast_نمونہ{$1, $2}; }
        //                      // From patternNoExpr
        //                      // also IEEE "''{' structure_pattern_key ':' ...
        //                      // also IEEE "''{' array_pattern_key ':' ...
        |       yP_TICKBRA نمونہ_Member_List '}'        { $$ = new Ast_نمونہ{$1, $2}; }
        //                      // IEEE: Not in grammar, but in VMM
        |       yP_TICKBRA '}'                          { $$ = new Ast_نمونہ{$1, nullptr}; }
        ;

// "datatype id = x {, id = x }"  |  "yaId = x {, id=x}" is legal
for_initializationE<nodep>:      // ==IEEE: for_initialization + for_variable_declaration
                /*خالی_تھاواں*/                     { $$ = nullptr; }
        |       for_initialization_شے_List      { $$ = $1; }
        ;

for_initialization_شے_List<nodep>:      // IEEE: [for_variable_declaration...]
                for_initialization_شے                  { $$ = $1; }
        |       for_initialization_شے_List ',' for_initialization_شے   { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

for_initialization_شے<nodep>:          // IEEE: variable_assignment + for_variable_declaration
        //                      // IEEE: for_variable_declaration
                انکڑے_type idAny/*new*/ '=' اظہاریہ
                        { VARRESET_NONLIST(VAR); VARDTYPE($1);
                          AstVar* ثابت varp = VARDONEA($<fl>2, *$2, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->جوڑو_اگلا(new Ast_سونپو{$3, new AstParseRef{$<fl>2, *$2}, $4}); }
        //                      // IEEE-2012:
        |       yVAR انکڑے_type idAny/*new*/ '=' اظہاریہ
                        { VARRESET_NONLIST(VAR); VARDTYPE($2);
                          AstVar* ثابت varp = VARDONEA($<fl>3, *$3, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->جوڑو_اگلا(new Ast_سونپو{$4, new AstParseRef{$<fl>3, *$3}, $5}); }
        //                      // IEEE: variable_assignment
        //                      // UNSUP variable_lvalue below
        |       id/*newOrExisting*/ '=' اظہاریہ
                        { إذا (GRAMMARP->m_varDecl) {
                              AstVar* ثابت varp = VARDONEA($<fl>1, *$1, nullptr, nullptr);
                              varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                              $$ = varp;
                              $$->جوڑو_اگلا(new Ast_سونپو{$2, new AstParseRef{$<fl>1, *$1}, $3});
                          } وإلا {
                              $$ = new Ast_سونپو{$2, new AstParseRef{$<fl>1, *$1}, $3};
                          }
                        }
        ;

for_قدم_E<nodep>:               // IEEE: for_step + empty
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       for_قدم                                { $$ = $1; }
        ;

for_قدم<nodep>:                // IEEE: for_step
                for_قدم_assignment                     { $$ = $1; }
        |       for_قدم ',' for_قدم_assignment        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

for_قدم_assignment<nodep>:     // ==IEEE: for_step_assignment
                foperator_assignment                    { $$ = $1; }
        |       finc_or_dec_عبارت                  { $$ = new Ast_Stmt_اظہاریہ{$<fl>1, $1}; }
        //                      // IEEE: function_subroutine_call
        |       task_subroutine_callNoSemi              { $$ = $1; }
        ;

loop_variables<nodep>:          // IEEE: loop_variables
                loop_variableE                          { $$ = $1; }
        |       loop_variables ',' loop_variableE       { $$ = $1->جوڑو_اگلا($3); }
        ;

loop_variableE<nodep>:          // IEEE: part of loop_variables
                /*خالی_تھاواں*/                             { $$ = new Ast_خالی_تھاواں{CRELINE()}; }
        |       parse_Ref_ادھار                            { $$ = $1; }
        ;

//************************************************
// Functions/tasks

taskRef<گرہ_Exprp>:            // IEEE: part of tf_call
                id                                      { $$ = new AstTaskRef{$<fl>1, *$1}; }
        |       id '(' list_دا_arguments_E ')'           { $$ = new AstTaskRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_دا_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstTaskRef{$<fl>2, *$2, $4}); }
        ;

funcRef<گرہ_Exprp>:             // IEEE: part of tf_call
        //                      // package_scope/hierarchical_... is part of expr, so just need ID
        //                      //      making-a                id-is-a
        //                      //      -----------------       ------------------
        //                      //      tf_call                 tf_identifier           expr (list_of_arguments)
        //                      //      method_call(post .)     function_identifier     expr (list_of_arguments)
        //                      //      property_instance       property_identifier     property_actual_arg
        //                      //      sequence_instance       sequence_identifier     sequence_actual_arg
        //                      //      let_expression          let_identifier          let_actual_arg
        //
                id '(' list_دا_arguments_E ')'
                        { $$ = new AstFuncRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_دا_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstFuncRef{$<fl>2, *$2, $4}); }
        //UNSUP list_of_argumentE should be pev_list_of_argumentE
        //UNSUP: idDottedSel is really just id to allow dotted method calls
        ;

task_subroutine_callNoSemi<گرہ_Stmtp>:  // similar to IEEE task_subroutine_call but without ';'
        //                      // Expr included here to resolve our not knowing what is a method call
        //                      // Expr here must result in a subroutine_call
                task_subroutine_call_No_طریقہ            { $$ = $1->makeStmt(); }
        |       fexpr '.' task_subroutine_call_No_طریقہ  { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        |       system_t_stmt_call                      { $$ = $1; }
        //                      // Any system function as a task
        |       system_f_or_t_اظہاریہ_call                 { $$ = new Ast_Stmt_اظہاریہ{$<fl>1, $1}; }
        //                      // Not here in IEEE; from class_constructor_declaration
        //                      // Because we've joined class_constructor_declaration into generic functions
        //                      // Way over-permissive;
        //                      // IEEE: [ ySUPER '.' yNEW [ '(' list_of_arguments ')' ] ';' ]
        |       fexpr '.' class_newNoScope              { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        ;

task_subroutine_call_No_طریقہ<گرہ_Exprp>:    // function_subroutine_callNoMethod (as task)
        //                      // IEEE: tf_call
                taskRef                                 { $$ = $1; }
        //                      // funcref below not task ref to avoid conflict, must later handle either
        |       funcRef yWITH__PAREN '(' اظہاریہ ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' اظہاریہ ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_بلاک     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_بلاک
                                                        { AstWithParse* ثابت withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

function_subroutine_call_No_طریقہ<گرہ_Exprp>:        // IEEE: function_subroutine_call (as function)
        //                      // IEEE: tf_call
                funcRef                                 { $$ = $1; }
        |       funcRef yWITH__PAREN '(' اظہاریہ ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' اظہاریہ ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        |       system_f_only_اظہاریہ_call                 { $$ = $1; }
        |       system_f_or_t_اظہاریہ_call                 { $$ = $1; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_بلاک     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_بلاک
                                                        { AstWithParse* ثابت withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

system_t_stmt_call<گرہ_Stmtp>:  // IEEE: part of system_tf_call (as task returning statement)
        //
                yaD_PLI system_Dpi_دلیلاں_E                  { AstTaskRef* ثابت refp = new AstTaskRef{$<fl>1, *$1, $2};
                                                          refp->pli(true);
                                                          $$ = refp->makeStmt(); }
        //
        |       yD_DUMPPORTS '(' idDottedSel ',' اظہاریہ ')'  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $5}; DEL($3);
                                                          $$->جوڑو_اگلا(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPPORTS '(' ',' اظہاریہ ')'           { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $4};
                                                          $$->جوڑو_اگلا(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPFILE '(' اظہاریہ ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $3}; }
        |       yD_DUMPVARS parenE                      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                              new AstConst{$<fl>1, 0}}; }
        |       yD_DUMPVARS '(' اظہاریہ ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; }
        |       yD_DUMPVARS '(' اظہاریہ ',' اظہاریہ_List ')'   { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; DEL($5); }
        |       yD_DUMPALL parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; }
        |       yD_DUMPALL '(' اظہاریہ ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; DEL($3); }
        |       yD_DUMPFLUSH parenE                     { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; }
        |       yD_DUMPFLUSH '(' اظہاریہ ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; DEL($3); }
        |       yD_DUMPLIMIT '(' اظہاریہ ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::حد_بندی_2, $3}; }
        |       yD_DUMPLIMIT '(' اظہاریہ ',' اظہاریہ ')'      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::حد_بندی_2, $3}; DEL($5); }
        |       yD_DUMPOFF parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; }
        |       yD_DUMPOFF '(' اظہاریہ ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; DEL($3); }
        |       yD_DUMPON parenE                        { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; }
        |       yD_DUMPON '(' اظہاریہ ')'                  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; DEL($3); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCStmtUser* cstmtp = nullptr;
                    إذا (!v3Global.opt.ignc()) {
                        cstmtp = new AstCStmtUser{$1, true};
                        cstmtp->جوڑو($3);
                    }
                    $$ = cstmtp;
                }
        |       yD_SDF_ANNOTATE '(' اظہاریہ_EList_E ')'      { $$ = nullptr; $1->v3warn(SPECIFYIGN, "Ignoring unsupported: $sdf_annotate"); DEL($3); }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_ڈھیر_Trace_T{$1}; }
        |       yD_SYSTEM '(' اظہاریہ ')'                  { $$ = new AstSystemT{$1, $3}; }
        //
        |       yD_EXIT parenE                          { $$ = new Ast_انت{$1}; }
        //
        |       yD_FCLOSE '(' اظہاریہ ')'                  { $$ = new AstFClose{$1, $3}; }
        |       yD_FFLUSH parenE                        { $$ = new AstFFlush{$1, nullptr}; }
        |       yD_FFLUSH '(' اظہاریہ ')'                  { $$ = new AstFFlush{$1, $3}; }
        |       y_D_انت parenE                        { $$ = new Ast_انت{$1}; }
        |       y_D_انت '(' اظہاریہ ')'                  { $$ = new Ast_انت{$1}; DEL($3); }
        |       yD_STOP parenE                          { $$ = new AstStop{$1, false}; }
        |       yD_STOP '(' اظہاریہ ')'                    { $$ = new AstStop{$1, false}; DEL($3); }
        //
        |       yD_SFORMAT '(' اظہاریہ ',' اظہاریہ_Disp_List ')'        { $$ = new AstSFormat{$1, false, $3, $5}; }
        |       yD_SWRITE  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'd'}; }
        |       yD_SWRITEB '(' اظہاریہ ',' اظہاریہ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'b'}; }
        |       yD_SWRITEH '(' اظہاریہ ',' اظہاریہ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'h'}; }
        |       yD_SWRITEO '(' اظہاریہ ',' اظہاریہ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'o'}; }
        //
        |       yD_DISPLAY parenE                               { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr}; }
        |       yD_DISPLAY '(' commasE اظہاریہ_Disp_List ')'         { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4}; }
        |       yD_DISPLAYB parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'b'}; }
        |       yD_DISPLAYB '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'b'}; }
        |       yD_DISPLAYH parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'h'}; }
        |       yD_DISPLAYH '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'h'}; }
        |       yD_DISPLAYO parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'o'}; }
        |       yD_DISPLAYO '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'o'}; }
        |       yD_MONITOR  '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4}; }
        |       yD_MONITORB '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'b'}; }
        |       yD_MONITORH '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'h'}; }
        |       yD_MONITORO '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'o'}; }
        |       yD_STROBE   '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4}; }
        |       yD_STROBEB  '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'b'}; }
        |       yD_STROBEH  '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'h'}; }
        |       yD_STROBEO  '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'o'}; }
        |       yD_WRITE    parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITE    '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4}; }
        |       yD_WRITEB   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEB   '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'b'}; }
        |       yD_WRITEH   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEH   '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'h'}; }
        |       yD_WRITEO   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEO   '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'o'}; }
        |       yD_FDISPLAY '(' اظہاریہ ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr}; }
        |       yD_FDISPLAY '(' اظہاریہ ',' اظہاریہ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5}; }
        |       yD_FDISPLAYB '(' اظہاریہ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'b'}; }
        |       yD_FDISPLAYB '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'b'}; }
        |       yD_FDISPLAYH '(' اظہاریہ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'h'}; }
        |       yD_FDISPLAYH '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'h'}; }
        |       yD_FDISPLAYO '(' اظہاریہ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'o'}; }
        |       yD_FDISPLAYO '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'o'}; }
        |       yD_FMONITOR  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5}; }
        |       yD_FMONITORB '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'b'}; }
        |       yD_FMONITORH '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'h'}; }
        |       yD_FMONITORO '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'o'}; }
        |       yD_FSTROBE   '(' اظہاریہ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, nullptr}; }
        |       yD_FSTROBE   '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5}; }
        |       yD_FSTROBEB  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'b'}; }
        |       yD_FSTROBEH  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'h'}; }
        |       yD_FSTROBEO  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'o'}; }
        |       yD_FWRITE   '(' اظہاریہ ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, nullptr}; }
        |       yD_FWRITE   '(' اظہاریہ ',' اظہاریہ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5}; }
        |       yD_FWRITEB  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'b'}; }
        |       yD_FWRITEH  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'h'}; }
        |       yD_FWRITEO  '(' اظہاریہ ',' اظہاریہ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'o'}; }
        |       yD_INFO     parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, nullptr}; }
        |       yD_INFO     '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, $4}; }
        |       yD_WARNING  parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, nullptr}; }
        |       yD_WARNING  '(' commasE اظہاریہ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, $4}; }
        |       yD_ERROR    parenE                              { $$ = GRAMMARP->createDisplayError($1); }
        |       yD_ERROR    '(' commasE اظہاریہ_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_ERROR, nullptr, $4};
                          $$->جوڑو_اگلا(new AstStop{$1, false}); }
        |       yD_FATAL    parenE
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->جوڑو_اگلا(new AstStop{$1, true}); }
        |       yD_FATAL    '(' اظہاریہ ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->جوڑو_اگلا(new AstStop{$1, true}); DEL($3); }
        |       yD_FATAL    '(' اظہاریہ ',' اظہاریہ_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, $5};
                          $$->جوڑو_اگلا(new AstStop{$1, true}); DEL($3); }
        //
        |       yD_ASSERTCTL '(' اظہاریہ ')'                                            { $$ = new AstAssertCtl{$1, $3}; }
        |       yD_ASSERTCTL '(' اظہاریہ ',' اظہاریہ_E ')'                                  { $$ = new AstAssertCtl{$1, $3, $5}; }
        |       yD_ASSERTCTL '(' اظہاریہ ',' اظہاریہ_E ',' اظہاریہ_E ')'                        { $$ = new AstAssertCtl{$1, $3, $5, $7}; }
        |       yD_ASSERTCTL '(' اظہاریہ ',' اظہاریہ_E ',' اظہاریہ_E ',' اظہاریہ_E ')'              { $$ = new AstAssertCtl{$1, $3, $5, $7, $9}; }
        |       yD_ASSERTCTL '(' اظہاریہ ',' اظہاریہ_E ',' اظہاریہ_E ',' اظہاریہ_E ',' اظہاریہ_List ')' { $$ = new AstAssertCtl{$1, $3, $5, $7, $9, $11}; }
        |       yD_ASSERTFAILOFF '(' اظہاریہ ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3}; }
        |       yD_ASSERTFAILOFF '(' اظہاریہ_E ',' اظہاریہ_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7}; }
        |       yD_ASSERTFAILON '(' اظہاریہ ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3}; }
        |       yD_ASSERTFAILON '(' اظہاریہ_E ',' اظہاریہ_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7}; }
        |       yD_ASSERTKILL '(' اظہاریہ ')'                      { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3}; }
        |       yD_ASSERTKILL '(' اظہاریہ_E ',' اظہاریہ_List ')'        { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3, $5}; }
        |       yD_ASSERTKILL parenE                            { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7}; }
        |       yD_ASSERTNONVACUOUSON '(' اظہاریہ ')'                 { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3}; }
        |       yD_ASSERTNONVACUOUSON '(' اظہاریہ_E ',' اظہاریہ_List ')'   { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTNONVACUOUSON parenE                       { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7}; }
        |       yD_ASSERTOFF '(' اظہاریہ ')'                       { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3}; }
        |       yD_ASSERTOFF '(' اظہاریہ_E ',' اظہاریہ_List ')'         { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3, $5}; }
        |       yD_ASSERTOFF parenE                             { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7}; }
        |       yD_ASSERTON '(' اظہاریہ ')'                        { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3}; }
        |       yD_ASSERTON '(' اظہاریہ_E ',' اظہاریہ_List ')'          { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3, $5}; }
        |       yD_ASSERTON parenE                              { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7}; }
        |       yD_ASSERTPASSOFF '(' اظہاریہ ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3}; }
        |       yD_ASSERTPASSOFF '(' اظہاریہ_E ',' اظہاریہ_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7}; }
        |       yD_ASSERTPASSON '(' اظہاریہ ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3}; }
        |       yD_ASSERTPASSON '(' اظہاریہ_E ',' اظہاریہ_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7}; }
        |       yD_ASSERTVACUOUSOFF '(' اظہاریہ ')'                { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3}; }
        |       yD_ASSERTVACUOUSOFF '(' اظہاریہ_E ',' اظہاریہ_List ')'  { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTVACUOUSOFF parenE                      { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7}; }
        //
        |       yD_MONITOROFF parenE                    { $$ = new AstMonitorOff{$1, true}; }
        |       yD_MONITORON parenE                     { $$ = new AstMonitorOff{$1, false}; }
        //
        |       yD_PRINTTIMESCALE                       { $$ = new Ast_Print_ویلا_Scale{$1}; }
        |       yD_PRINTTIMESCALE '(' ')'               { $$ = new Ast_Print_ویلا_Scale{$1}; }
        |       yD_PRINTTIMESCALE '(' idClassSel ')'    { $$ = new Ast_Print_ویلا_Scale{$1}; DEL($3); }
        |       yD_TIMEFORMAT '(' اظہاریہ_E ',' اظہاریہ_E ',' اظہاریہ_E ',' اظہاریہ_E ')'
                        { $$ = new Ast_ویلا_Format{$1, $3, $5, $7, $9}; }
        |       yD_TIMEFORMAT '(' اظہاریہ_E ',' اظہاریہ_E ',' اظہاریہ_E ')'
                        { $$ = new Ast_ویلا_Format{$1, $3, $5, $7, nullptr}; }
        |       yD_TIMEFORMAT '(' اظہاریہ_E ',' اظہاریہ_E ')'
                        { $$ = new Ast_ویلا_Format{$1, $3, $5, nullptr, nullptr}; }
        |       yD_TIMEFORMAT '(' اظہاریہ_E ')'
                        { $$ = new Ast_ویلا_Format{$1, $3, nullptr, nullptr, nullptr}; }
        //
        |       yD_READMEMB '(' اظہاریہ ',' idClassSel ')'                         { $$ = new AstReadMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_READMEMB '(' اظہاریہ ',' idClassSel ',' اظہاریہ ')'                { $$ = new AstReadMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_READMEMB '(' اظہاریہ ',' idClassSel ',' اظہاریہ ',' اظہاریہ ')'       { $$ = new AstReadMem{$1, false, $3, $5, $7, $9}; }
        |       yD_READMEMH '(' اظہاریہ ',' idClassSel ')'                         { $$ = new AstReadMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_READMEMH '(' اظہاریہ ',' idClassSel ',' اظہاریہ ')'                { $$ = new AstReadMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_READMEMH '(' اظہاریہ ',' idClassSel ',' اظہاریہ ',' اظہاریہ ')'       { $$ = new AstReadMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_WRITEMEMB '(' اظہاریہ ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMB '(' اظہاریہ ',' idClassSel ',' اظہاریہ ')'               { $$ = new AstWriteMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMB '(' اظہاریہ ',' idClassSel ',' اظہاریہ ',' اظہاریہ ')'      { $$ = new AstWriteMem{$1, false, $3, $5, $7, $9}; }
        |       yD_WRITEMEMH '(' اظہاریہ ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMH '(' اظہاریہ ',' idClassSel ',' اظہاریہ ')'               { $$ = new AstWriteMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMH '(' اظہاریہ ',' idClassSel ',' اظہاریہ ',' اظہاریہ ')'      { $$ = new AstWriteMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_CAST '(' اظہاریہ ',' اظہاریہ ')'
                        { File_سطر* ثابت fl_nowarn = new File_سطر{$1};
                          fl_nowarn->warnOff(V3_Error_کوڈ::چوڑائی_2, true);
                          $$ = new AstAssertIntrinsic{fl_nowarn, new AstCastDynamic{fl_nowarn, $5, $3},
                                                      nullptr, nullptr}; }
        ;

system_f_only_اظہاریہ_call<گرہ_Exprp>:  // IEEE: part of system_tf_call (for functions returning expressions)
                yaD_PLI system_Dpi_دلیلاں_E                  { $$ = new AstFuncRef{$<fl>1, *$1, $2}; VN_CAST($$, FuncRef)->pli(true); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    إذا (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1};
                        cexprp->جوڑو($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CPURE '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    إذا (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1, AstCExprUser::خالص{}};
                        cexprp->جوڑو($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CAST '(' اظہاریہ ',' اظہاریہ ')'           { $$ = new AstCastDynamic{$1, $5, $3}; }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_ڈھیر_Trace_F{$1}; }
        |       yD_SYSTEM  '(' اظہاریہ ')'                 { $$ = new AstSystemF{$1, $3}; }
        ;

system_f_or_t_اظہاریہ_call<گرہ_Exprp>:  // IEEE: part of system_tf_call (can be task or func)
                yD_ACOS '(' اظہاریہ ')'                    { $$ = new AstAcosD{$1, $3}; }
        |       yD_ACOSH '(' اظہاریہ ')'                   { $$ = new AstAcoshD{$1, $3}; }
        |       yD_ASIN '(' اظہاریہ ')'                    { $$ = new AstAsinD{$1, $3}; }
        |       yD_ASINH '(' اظہاریہ ')'                   { $$ = new AstAsinhD{$1, $3}; }
        |       yD_ATAN '(' اظہاریہ ')'                    { $$ = new AstAtanD{$1, $3}; }
        |       yD_ATAN2 '(' اظہاریہ ',' اظہاریہ ')'          { $$ = new AstAtan2D{$1, $3, $5}; }
        |       yD_ATANH '(' اظہاریہ ')'                   { $$ = new AstAtanhD{$1, $3}; }
        |       y_D_بٹ '(' اظہاریہ_Or_انکڑے_Type ')'          { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_بٹ, $3}; }
        |       y_D_بٹ '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')' { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_بٹ, $3, $5}; }
        |       yD_BITSTOREAL '(' اظہاریہ ')'              { $$ = new Ast_بٹ_منزل_Real_D{$1, $3}; }
        |       yD_BITSTOSHORTREAL '(' اظہاریہ ')'         { $$ = new Ast_بٹ_منزل_Real_D{$1, $3}; UNSUPREAL($1); }
        |       yD_CEIL '(' اظہاریہ ')'                    { $$ = new AstCeilD{$1, $3}; }
        |       y_D_بدلیا '(' اظہاریہ ')'                 { $$ = new AstLogNot{$1, new AstStable{$1, $3, nullptr}}; }
        |       y_D_بدلیا '(' اظہاریہ ',' اظہاریہ ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Sen_درخت_بدلیا($1, $5)}}; }
        |       y_D_بدلیا_GCLK '(' اظہاریہ ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_درخت($1)}}; }
        |       yD_CHANGING_GCLK '(' اظہاریہ ')'
                        { $$ = new AstLogNot{$1, new AstSteady{$1, $3}}; }
        |       yD_CLOG2 '(' اظہاریہ ')'                   { $$ = new AstCLog2{$1, $3}; }
        |       yD_COS '(' اظہاریہ ')'                     { $$ = new AstCosD{$1, $3}; }
        |       yD_COSH '(' اظہاریہ ')'                    { $$ = new AstCoshD{$1, $3}; }
        |       yD_COUNTBITS '(' اظہاریہ ',' اظہاریہ ')'              { $$ = new Ast_گنتی_بٹ{$1, $3, $5}; }
        |       yD_COUNTBITS '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'             { $$ = new Ast_گنتی_بٹ{$1, $3, $5, $7}; }
        |       yD_COUNTBITS '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'    { $$ = new Ast_گنتی_بٹ{$1, $3, $5, $7, $9}; }
        |       yD_COUNTBITS '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ',' اظہاریہ ',' اظہاریہ_List ')'
                        { $$ = new Ast_گنتی_بٹ{$1, $3, $5, $7, $9};
                          BBUNSUP($11, "Unsupported: $countbits with more than 3 control fields"); }
        |       yD_COUNTONES '(' اظہاریہ ')'               { $$ = new Ast_گنتی_Ones{$1, $3}; }
        |       yD_DIMENSIONS '(' اظہاریہ_Or_انکڑے_Type ')'    { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_DIMENSIONS, $3}; }
        |       y_D_DIST_CHI_مربع '(' اظہاریہ ',' اظہاریہ ')'        { $$ = new Ast_Dist_Chi_مربع{$1, $3, $5}; }
        |       yD_DIST_ERLANG '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'   { $$ = new AstDistErlang{$1, $3, $5, $7}; }
        |       y_D_DIST_قوت_نمائی '(' اظہاریہ ',' اظہاریہ ')'       { $$ = new Ast_Dist_قوت_نمائی{$1, $3, $5}; }
        |       y_D_DIST_عام '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'   { $$ = new Ast_Dist_عام{$1, $3, $5, $7}; }
        |       y_D_DIST_پواسوں '(' اظہاریہ ',' اظہاریہ ')'   { $$ = new Ast_Dist_پواسوں{$1, $3, $5}; }
        |       yD_DIST_T '(' اظہاریہ ',' اظہاریہ ')'         { $$ = new AstDistT{$1, $3, $5}; }
        |       y_D_DIST_اکسار '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'  { $$ = new Ast_Dist_اکسار{$1, $3, $5, $7}; }
        |       yD_EXP '(' اظہاریہ ')'                     { $$ = new AstExpD{$1, $3}; }
        |       yD_FALLING_GCLK '(' اظہاریہ ')'            { $$ = new AstFalling{$1, $3}; }
        |       yD_FELL '(' اظہاریہ ')'                    { $$ = new AstFell{$1, $3, nullptr}; }
        |       yD_FELL '(' اظہاریہ ',' اظہاریہ ')'           { $$ = new AstFell{$1, $3, GRAMMARP->create_Sen_درخت_بدلیا($1, $5)}; }
        |       yD_FELL_GCLK '(' اظہاریہ ')'               { $$ = new AstFell{$1, $3, GRAMMARP->create_Global_Clock_Sen_درخت($1)}; }
        |       yD_FEOF '(' اظہاریہ ')'                    { $$ = new AstFEof{$1, $3}; }
        |       yD_FERROR '(' اظہاریہ ',' idClassSel ')'   { $$ = new AstFError{$1, $3, $5}; }
        |       yD_FGETC '(' اظہاریہ ')'                   { $$ = new AstFGetC{$1, $3}; }
        |       yD_FGETS '(' اظہاریہ ',' اظہاریہ ')'          { $$ = new AstFGetS{$1, $3, $5}; }
        |       yD_FOPEN '(' اظہاریہ ')'                   { $$ = new AstFOpenMcd{$1, $3}; }
        |       yD_FOPEN '(' اظہاریہ ',' اظہاریہ ')'          { $$ = new AstFOpen{$1, $3, $5}; }
        |       yD_FREAD '(' اظہاریہ ',' اظہاریہ ')'          { $$ = new AstFRead{$1, $3, $5, nullptr, nullptr}; }
        |       yD_FREAD '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'  { $$ = new AstFRead{$1, $3, $5, $7, nullptr}; }
        |       yD_FREAD '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')'  { $$ = new AstFRead{$1, $3, $5, $7, $9}; }
        |       yD_FREAD '(' اظہاریہ ',' اظہاریہ ',' ',' اظہاریہ ')'  { $$ = new AstFRead{$1, $3, $5, nullptr, $8}; }
        |       yD_FREWIND '(' اظہاریہ ')'                 { $$ = new AstFRewind{$1, $3}; }
        |       yD_FUTURE_GCLK '(' اظہاریہ ')'             { $$ = new AstFuture{$1, $3, nullptr}; }
        |       yD_FLOOR '(' اظہاریہ ')'                   { $$ = new AstFloorD{$1, $3}; }
        |       yD_FSCANF '(' اظہاریہ ',' str commaVRDListE ')'    { $$ = new AstFScanF{$1, *$5, $3, $6}; }
        |       yD_FSEEK '(' اظہاریہ ',' اظہاریہ ',' اظہاریہ ')' { $$ = new AstFSeek{$1, $3, $5, $7}; }
        |       yD_FTELL '(' اظہاریہ ')'                   { $$ = new AstFTell{$1, $3}; }
        |       y_D_GET_INITIAL_RANDOM_بیج parenE      { $$ = new Ast_Get_Initial_Random_بیج{$1}; }
        |       yD_GLOBAL_CLOCK parenE                  { $$ = GRAMMARP->createGlobalClockParseRef($1); }
        |       y_D_بلند '(' اظہاریہ_Or_انکڑے_Type ')'          { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_بلند, $3, nullptr}; }
        |       y_D_بلند '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')' { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_بلند, $3, $5}; }
        |       yD_HYPOT '(' اظہاریہ ',' اظہاریہ ')'          { $$ = new AstHypotD{$1, $3, $5}; }
        |       yD_INCREMENT '(' اظہاریہ_Or_انکڑے_Type ')'     { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_INCREMENT, $3, nullptr}; }
        |       yD_INCREMENT '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')'    { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_INCREMENT, $3, $5}; }
        |       yD_INFERRED_DISABLE parenE              { $$ = new AstInferredDisable{$1}; }
        |       yD_ISUNBOUNDED '(' اظہاریہ ')'             { $$ = new AstIsUnbounded{$1, $3}; }
        |       yD_ISUNKNOWN '(' اظہاریہ ')'               { $$ = new AstIsUnknown{$1, $3}; }
        |       yD_ITOR '(' اظہاریہ ')'                    { $$ = new AstIToRD{$1, $3}; }
        |       y_D_بایاں '(' اظہاریہ_Or_انکڑے_Type ')'          { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_بایاں, $3, nullptr}; }
        |       y_D_بایاں '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')' { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_بایاں, $3, $5}; }
        |       yD_LN '(' اظہاریہ ')'                      { $$ = new AstLogD{$1, $3}; }
        |       yD_LOG10 '(' اظہاریہ ')'                   { $$ = new AstLog10D{$1, $3}; }
        |       y_D_کم '(' اظہاریہ_Or_انکڑے_Type ')'           { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_کم, $3, nullptr}; }
        |       y_D_کم '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')'  { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_کم, $3, $5}; }
        |       yD_ONEHOT '(' اظہاریہ ')'                  { $$ = new AstOneHot{$1, $3}; }
        |       yD_ONEHOT0 '(' اظہاریہ ')'                 { $$ = new AstOneHot0{$1, $3}; }
        |       yD_PAST '(' اظہاریہ ')'                    { $$ = new AstPast{$1, $3}; }
        |       yD_PAST '(' اظہاریہ ',' اظہاریہ_E ')'          { $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' اظہاریہ ',' اظہاریہ_E ',' اظہاریہ_E ')'
                        { إذا ($7) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' اظہاریہ ',' اظہاریہ_E ',' اظہاریہ_E ',' clocking_eventE ')'
                        { إذا ($7 || $9) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7, $9);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST_GCLK '(' اظہاریہ ')'               { $$ = new AstPast{$1, $3, nullptr, GRAMMARP->create_Global_Clock_Sen_درخت($1)}; }
        |       yD_POW '(' اظہاریہ ',' اظہاریہ ')'            { $$ = new AstPowD{$1, $3, $5}; }
        |       yD_RANDOM '(' اظہاریہ ')'                  { $$ = new AstRand{$1, $3, false}; }
        |       yD_RANDOM parenE                        { $$ = new AstRand{$1, nullptr, false}; }
        |       yD_REALTIME parenE                      { $$ = new Ast_ویلا_D{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_REALTOBITS '(' اظہاریہ ')'              { $$ = new Ast_Real_منزل_بٹ{$1, $3}; }
        |       yD_REWIND '(' اظہاریہ ')'                  { $$ = new AstFSeek{$1, $3, new AstConst{$1, 0}, new AstConst{$1, 0}}; }
        |       y_D_دایاں '(' اظہاریہ_Or_انکڑے_Type ')'         { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_دایاں, $3, nullptr}; }
        |       y_D_دایاں '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')'        { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_دایاں, $3, $5}; }
        |       yD_RISING_GCLK '(' اظہاریہ ')'             { $$ = new AstRising{$1, $3}; }
        |       yD_ROSE '(' اظہاریہ ')'                    { $$ = new AstRose{$1, $3, nullptr}; }
        |       yD_ROSE '(' اظہاریہ ',' اظہاریہ ')'           { $$ = new AstRose{$1, $3, GRAMMARP->create_Sen_درخت_بدلیا($1, $5)}; }
        |       yD_ROSE_GCLK '(' اظہاریہ ')'               { $$ = new AstRose{$1, $3, GRAMMARP->create_Global_Clock_Sen_درخت($1)}; }
        |       yD_RTOI '(' اظہاریہ ')'                    { $$ = new AstRToIS{$1, $3}; }
        |       yD_SAMPLED '(' اظہاریہ ')'                 { $$ = new AstSampled{$1, $3, $3->dtypep()}; }
        |       yD_SFORMATF '(' اظہاریہ_Disp_List ')'        { $$ = new AstSFormatF{$1, AstSFormatF::اظہاریہ_Format{}, $3, 'd', false}; }
        |       yD_SHORTREALTOBITS '(' اظہاریہ ')'         { $$ = new Ast_Real_منزل_بٹ{$1, $3}; UNSUPREAL($1); }
        |       yD_SIGNED '(' اظہاریہ ')'                  { $$ = new AstSigned{$1, $3}; }
        |       yD_SIN '(' اظہاریہ ')'                     { $$ = new AstSinD{$1, $3}; }
        |       yD_SINH '(' اظہاریہ ')'                    { $$ = new AstSinhD{$1, $3}; }
        |       y_D_ناپ '(' اظہاریہ_Or_انکڑے_Type ')'          { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_ناپ, $3, nullptr}; }
        |       y_D_ناپ '(' اظہاریہ_Or_انکڑے_Type ',' اظہاریہ ')' { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_ناپ, $3, $5}; }
        |       yD_SQRT '(' اظہاریہ ')'                    { $$ = new AstSqrtD{$1, $3}; }
        |       yD_SSCANF '(' اظہاریہ ',' str commaVRDListE ')'    { $$ = new AstSScanF{$1, *$5, $3, $6}; }
        |       yD_STABLE '(' اظہاریہ ')'                  { $$ = new AstStable{$1, $3, nullptr}; }
        |       yD_STABLE '(' اظہاریہ ',' اظہاریہ ')'         { $$ = new AstStable{$1, $3, GRAMMARP->create_Sen_درخت_بدلیا($1, $5)}; }
        |       yD_STABLE_GCLK '(' اظہاریہ ')'             { $$ = new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_درخت($1)}; }
        |       yD_STEADY_GCLK '(' اظہاریہ ')'             { $$ = new AstSteady{$1, $3}; }
        |       yD_STIME parenE
                        { $$ = new AstSel{$1, new Ast_ویلا{$1, VTimescale{VTimescale::NONE}}, 0, 32}; }
        |       yD_TAN '(' اظہاریہ ')'                     { $$ = new AstTanD{$1, $3}; }
        |       yD_TANH '(' اظہاریہ ')'                    { $$ = new AstTanhD{$1, $3}; }
        |       yD_TESTPLUSARGS '(' اظہاریہ ')'            { $$ = new Ast_Test_Plus_دلیلاں{$1, $3}; }
        |       y_D_ویلا parenE                          { $$ = new Ast_ویلا{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_TIMEPRECISION                        { $$ = new Ast_ویلا_Precision{$1}; }
        |       yD_TIMEPRECISION '(' ')'                { $$ = new Ast_ویلا_Precision{$1}; }
        |       yD_TIMEPRECISION '(' idClassSel ')'     { $$ = new Ast_ویلا_Precision{$1}; DEL($3); }
        |       yD_TIMEUNIT                             { $$ = new Ast_ویلا_Unit{$1}; }
        |       yD_TIMEUNIT '(' ')'                     { $$ = new Ast_ویلا_Unit{$1}; }
        |       yD_TIMEUNIT '(' idClassSel ')'          { $$ = new Ast_ویلا_Unit{$1}; DEL($3); }
        |       yD_TYPENAME '(' اظہاریہ_Or_انکڑے_Type ')'      { $$ = new Ast_Attr_دا{$1, VAttrType::TYPENAME, $3}; }
        |       yD_UNGETC '(' اظہاریہ ',' اظہاریہ ')'         { $$ = new AstFUngetC{$1, $5, $3}; }  // Arg swap to file first
        |       yD_UNPACKED_DIMENSIONS '(' اظہاریہ_Or_انکڑے_Type ')'   { $$ = new Ast_Attr_دا{$1, VAttrType::DIM_UNPK_DIMENSIONS, $3}; }
        |       yD_UNSIGNED '(' اظہاریہ ')'                { $$ = new AstUnsigned{$1, $3}; }
        |       yD_URANDOM '(' اظہاریہ ')'                 { $$ = new AstRand{$1, $3, true}; }
        |       yD_URANDOM parenE                       { $$ = new AstRand{$1, nullptr, true}; }
        |       yD_URANDOM_RANGE '(' اظہاریہ ')'           { $$ = new AstURandomRange{$1, $3, new AstConst{$1, 0}}; }
        |       yD_URANDOM_RANGE '(' اظہاریہ ',' اظہاریہ ')'  { $$ = new AstURandomRange{$1, $3, $5}; }
        |       yD_VALUEPLUSARGS '(' اظہاریہ ',' اظہاریہ ')'  { $$ = new Ast_قدر_Plus_دلیلاں{$1, $3, $5}; }
        ;

severity_system_task<nodep>: // IEEE: severity_system_task/elaboration_severity_system_task (1800-2009)
        //                      // Elaboration-time task; V3Width evaluates and removes it
                severity_system_task_guts ';'           { $$ = $1; }
        ;

severity_system_task_guts<nodep>:    // IEEE: part of severity_system_task (1800-2009)
        //                      // $fatal first argument is exit number, must be constant
                yD_INFO parenE                          { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, nullptr}; }
        |       yD_INFO '(' اظہاریہ_List ')'                { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, $3}; }
        |       yD_WARNING parenE                       { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, nullptr}; }
        |       yD_WARNING '(' اظہاریہ_List ')'             { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, $3}; }
        |       yD_ERROR parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, nullptr}; }
        |       yD_ERROR '(' اظہاریہ_List ')'               { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, $3}; }
        |       yD_FATAL parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; }
        |       yD_FATAL '(' اظہاریہ ')'                   { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; DEL($3); }
        |       yD_FATAL '(' اظہاریہ ',' اظہاریہ_List_E ')'     { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, $5}; DEL($3); }
        ;

system_Dpi_دلیلاں_E<argp>:           // IEEE: part of system_if_call for arguments of $dpi call
                parenE                                  { $$ = nullptr; }
        |       '(' اظہاریہ_List ')'                        { $$ = GRAMMARP->argWrapList($2); }
        ;

property_actual_arg<گرہ_Exprp>:  // ==IEEE: property_actual_arg
        //                      // IEEE: property_expr
        //                      // IEEE: sequence_actual_arg
        //UNSUP pev_expr                                { $$ = $1; }
        //UNSUP remove below:
                pexpr                                   { $$ = $1; }
        //                      // IEEE: sequence_expr
        //                      // property_expr already includes sequence_expr
        ;

اظہاریہ_Or_انکڑے_Type<nodep>:          // expr | data_type: combined to prevent conflicts
                اظہاریہ                                    { $$ = $1; }
        //                      // data_type includes id that overlaps expr, so special flavor
        //                      // data_type expanded:
        |       انکڑے_type_No_Ref                          { $$ = $1; }
        //
        //                      // Conflicts with non-type id, resolved in V3LinkDot
        //                      // NO: packageClassScopeE idType packed_dimensionListE
        //
        |       packageClassScopeE idType parameter_قدر_assignment_Class packed_dimensionListE
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, $3};
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

list_دا_arguments_E<argp>:  // IEEE: [list_of_arguments]
                دلیلاں_Dotted_List                          { $$ = $1; }
        |       دلیلاں_اظہاریہ_List_E
                        { إذا ($1->خالی_تھاواں_Connect_No_اگلا()) {
                              $1->delete_درخت(); $$ = nullptr;  // Mis-created when have 'func()'
                          } وإلا { $$ = $1; } }
        |       دلیلاں_اظہاریہ_List_E ',' دلیلاں_Dotted_List        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

task_declaration<گرہ_FTaskp>:   // ==IEEE: task_declaration
                yTASK dynamic_override_specifiersE lifetimeE taskId tfGuts yENDTASK سرا_Label_E
                        { $$ = $4; $$->جوڑو_Stmtsp($5);
                          $$->ادھار_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->سرا_Label($<fl>7, $$, $7); }
        ;

task_prototype<گرہ_FTaskp>:             // ==IEEE: task_prototype
                yTASK dynamic_override_specifiersE taskId '(' tf_پورٹ_list_E ')'
                        { $$ = $3; $$->جوڑو_Stmtsp($5); $$->prototype(true); }
        |       yTASK dynamic_override_specifiersE taskId
                        { $$ = $3; $$->prototype(true); }
        ;

function_declaration<گرہ_FTaskp>:       // IEEE: function_declaration + function_body_declaration
                yFUNCTION dynamic_override_specifiersE lifetimeE funcId funcIsolateE tfGuts yENDFUNCTION سرا_Label_E
                        { $$ = $4; $$->جوڑو_Stmtsp($6);
                          $$->ادھار_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->سرا_Label($<fl>8, $$, $8); }
        |       yFUNCTION dynamic_override_specifiersE lifetimeE funcIdNew funcIsolateE tfNewGuts yENDFUNCTION سرا_Label_E
                        { $$ = $4; $$->جوڑو_Stmtsp($6);
                          $$->ادھار_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->سرا_Label($<fl>8, $$, $8); }
        ;

function_prototype<گرہ_FTaskp>: // IEEE: function_prototype
                yFUNCTION dynamic_override_specifiersE funcId '(' tf_پورٹ_list_E ')'
                        { $$ = $3; $$->جوڑو_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcId
                        { $$ = $3; $$->prototype(true); }
        ;

class_constructor_prototype<گرہ_FTaskp>:        // ==IEEE: class_constructor_prototype
        //                      // IEEE has no dynamic_override_specifiersE,
        //                      // but required to avoid conflicts, so we must check after parsing
                yFUNCTION dynamic_override_specifiersE funcIdNew '(' class_constructor_arg_listE ')' ';'
                        { $$ = $3; $$->جوڑو_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcIdNew ';'
                        { $$ = $3; $$->prototype(true); }
        ;

funcIsolateE<cint>:
                /*خالی_تھاواں*/                             { $$ = 0; }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = 1; }
        ;

طریقہ_prototype<گرہ_FTaskp>:
                task_prototype                          { $$ = $1; }
        |       function_prototype                      { $$ = $1; }
        ;

lifetimeE<lifetime>:            // IEEE: [lifetime]
                /*خالی_تھاواں*/                             { $$ = VLifetime::NONE; }
        |       lifetime                                { $$ = $1; }
        ;

lifetime<lifetime>:             // ==IEEE: lifetime
        //                      // Note lifetime used by members is instead under memberQual
                ySTATIC__ETC                            { $$ = VLifetime::STATIC_EXPLICIT; }
        |       yAUTOMATIC                              { $$ = VLifetime::AUTOMATIC_EXPLICIT; }
        ;

taskId<گرہ_FTaskp>:
                id
                        { $$ = new AstTask{$<fl>$, *$1, nullptr};
                          $$->verilogTask(true); }
        //
        |       id/*interface_identifier*/ '.' idAny
                        { $$ = new AstTask{$<fl>$, *$3, nullptr};
                          $$->verilogTask(true);
                          $$->iface_پورٹ_Name(*$1); }
        //
        |       packageClassScope id
                        { $$ = new AstTask{$<fl>$, *$2, nullptr};
                          $$->verilogTask(true);
                          $$->classOrPackagep($1);
                          $$->class_طریقہ(true); }
        ;

funcId<گرہ_FTaskp>:             // IEEE: function_data_type_or_implicit + part of function_body_declaration
        //                      // IEEE: function_data_type_or_implicit must be expanded here to prevent conflict
        //                      // function_data_type expanded here to prevent conflicts with
        //                      // implicit_type:empty vs data_type:ID
                /**/ fIdScoped
                        { $$ = $1;
                          $$->fvarp(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT}); }
        |       signingE rangeList fIdScoped
                        { $$ = $3;
                          $$->fvarp(GRAMMARP->جوڑو_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)); }
        |       signing fIdScoped
                        { $$ = $2;
                          $$->fvarp(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}); }
        |       انکڑے_type_No_Ref fIdScoped
                        { $$ = $2;
                          $$->fvarp($1); }
        |       packageClassScopeE idInstType packed_dimensionListE fIdScoped
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = $4;
                          $$->fvarp(GRAMMARP->createArray(refp, $3, true)); }
        |       packageClassScopeE idInstType parameter_قدر_assignment_Class packed_dimensionListE fIdScoped
                        { AstRefDType* ثابت refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = $5;
                          $$->fvarp(GRAMMARP->createArray(refp, $4, true)); }
        //                      // To verilator tasks are the same as void functions (we separately detect time passing)
        |       yVOID taskId
                        { $$ = $2;  // Internals represent it as a task, not a function (TODO cleanup)
                          $$->verilogTask(false);
                          $$->verilogFunction(true); }
        ;

funcIdNew<گرہ_FTaskp>:          // IEEE: from class_constructor_declaration
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
                          $$->class_طریقہ(true); }
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
                          $$->iface_پورٹ_Name(*$1); }
        //
        |       packageClassScope id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstFunc{$<fl>$, *$2, nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->class_طریقہ(true);
                          $$->classOrPackagep($1); }
        ;

tfGuts<nodep>:
                '(' tf_پورٹ_list_E ')' ';' tfBodyE       { $$ = جوڑو_اگلا_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfNewGuts<nodep>:
                '(' class_constructor_arg_listE ')' ';' tfBodyE       { $$ = جوڑو_اگلا_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfBodyE<nodep>:                 // IEEE: part of function_body_declaration/task_body_declaration
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       tf_شے_declaration_List                 { $$ = $1; }
        |       tf_شے_declaration_List stmtList        { $$ = جوڑو_اگلا_Null($1, $2); }
        |       stmtList                                { $$ = $1; }
        ;

tf_شے_declaration_List<nodep>:
                tf_شے_declaration                     { $$ = $1; }
        |       tf_شے_declaration_List tf_شے_declaration     { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

tf_شے_declaration<nodep>:     // ==IEEE: tf_item_declaration
                بلاک_شے_declaration                  { $$ = $1; }
        |       tf_پورٹ_declaration                     { $$ = $1; }
        |       tf_شے_declaration_Verilator            { $$ = $1; }
        ;

tf_شے_declaration_Verilator<nodep>:    // Verilator extensions
                y_VL_عوامی                              { $$ = new AstPragma{$1, VPragmaType::عوامی_TASK}; v3Global.dpi(true); }
        |       yVL_NO_INLINE_TASK                      { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_TASK}; }
        ;

tf_پورٹ_list_E<nodep>:           // IEEE: tf_port_list + empty
        //                      // Empty covered by tf_port_item
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(داخلہ); }
        /*cont*/    tf_پورٹ_list_List                    { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

tf_پورٹ_list_List<nodep>:        // IEEE: part of tf_port_list
                tf_پورٹ_شے                            { $$ = $1; }
        |       tf_پورٹ_list_List ',' tf_پورٹ_شے       { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

class_constructor_arg_listE<nodep>:  // IEEE: class_constructor_arg_list or empty
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(داخلہ); }
        /*cont*/    class_constructor_arg_listList      { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

class_constructor_arg_listList<nodep>:  // IEEE: part of class_constructor_arg_list
                class_constructor_arg                   { $$ = $1; }
        |       class_constructor_arg_listList ',' class_constructor_arg       { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

class_constructor_arg<nodep>:  // ==IEEE: class_constructor_arg
                tf_پورٹ_شے                            { $$ = $1; }
        |       yDEFAULT                                { $$ = nullptr; BBUNSUP($1, "Unsupported: new constructor 'default' argument"); }
        ;

tf_پورٹ_شے<nodep>:            // ==IEEE: tf_port_item
        //                      // We split tf_port_item into the type and assignment as don't know what follows a comma
                /*خالی_تھاواں*/                             { $$ = nullptr; PINNUMINC(); }  // For example a ",," port
        |       tf_پورٹ_شے_Front tf_پورٹ_شے_Assignment { $$ = $2; }
        |       tf_پورٹ_شے_Assignment                  { $$ = $1; }
        ;

tf_پورٹ_شے_Front:              // IEEE: part of tf_port_item, which has the data type
                انکڑے_type                               { VARDTYPE($1); }
        |       signingE rangeList
                        { Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                              new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true);
                          VARDTYPE(dtp); }
        |       signing
                        { Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1};
                          VARDTYPE(dtp); }
        |       yVAR انکڑے_type                          { VARDTYPE($2); }
        |       yVAR implicit_typeE                     { VARDTYPE($2); }
        //
        |       tf_پورٹ_شے_Dir /*implicit*/            { VARDTYPE(nullptr); /*default_nettype-see spec*/ }
        |       tf_پورٹ_شے_Dir انکڑے_type               { VARDTYPE($2); }
        |       tf_پورٹ_شے_Dir signingE rangeList
                        { Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                              new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE(dtp); }
        |       tf_پورٹ_شے_Dir signing
                        { Ast_گرہ_DType* ثابت dtp = new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $2};
                          VARDTYPE(dtp); }
        |       tf_پورٹ_شے_Dir yVAR انکڑے_type          { VARDTYPE($3); }
        |       tf_پورٹ_شے_Dir yVAR implicit_typeE     { VARDTYPE($3); }
        ;

tf_پورٹ_شے_Dir:                // IEEE: part of tf_port_item, direction
                پورٹ_direction                          { }  // port_direction sets VARIO
        ;

tf_پورٹ_شے_Assignment<varp>:   // IEEE: part of tf_port_item, which has assignment
                id variable_dimensionListE sigAttrListE اظہاریہ_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); إذا ($4) $$->valuep($4); }
        ;

parenE:
                /*خالی_تھاواں*/                             { }
        |       '(' ')'                                 { }
        ;

//      method_call:            // ==IEEE: method_call + method_call_body
//                              // IEEE: method_call_root '.' method_identifier [ '(' list_of_arguments ')' ]
//                              //   "method_call_root '.' method_identifier" looks just like "expr '.' id"
//                              //   "method_call_root '.' method_identifier (...)" looks just like "expr '.' tf_call"
//                              // IEEE: built_in_method_call
//                              //   method_call_root not needed, part of expr resolution
//                              // What's left is below array_methodNoRoot
array_طریقہ_No_جڑ<گرہ_FTask_Refp>:
                yOR                                     { $$ = new AstFuncRef{$1, "or"}; }
        |       yAND                                    { $$ = new AstFuncRef{$1, "and"}; }
        |       yXOR                                    { $$ = new AstFuncRef{$1, "xor"}; }
        |       yUNIQUE                                 { $$ = new AstFuncRef{$1, "unique"}; }
        ;

array_طریقہ_With<گرہ_Exprp>:
                array_طریقہ_No_جڑ parenE               { $$ = $1; }
        |       array_طریقہ_No_جڑ parenE yWITH__PAREN '(' اظہاریہ ')'
                        { $$ = new AstWithParse{$3, $1, $5}; }
        |       array_طریقہ_No_جڑ '(' اظہاریہ ')' yWITH__PAREN '(' اظہاریہ ')'
                        { $$ = new AstWithParse{$5, $1, $7}; $1->جوڑو_Argsp(new AstArg{$<fl>3, "", $3}); }
        ;

dpi_import_export<nodep>:       // ==IEEE: dpi_import_export
                yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype ';'
                        { $$ = $5;
                          إذا (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_خالص($3 == iprop_خالص);
                          $5->dpiImport(true);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype yVL_DPI_C_DECL yaSTRING ';'
                        { $$ = $5;
                          إذا (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_خالص($3 == iprop_خالص);
                          $5->dpiImport(true);
                          إذا (*$7 != "") $5->dpiCDecl(*$7);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE task_prototype ';'
                        { $$ = $5;
                          إذا (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_خالص($3 == iprop_خالص);
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
                /*خالی_تھاواں*/                             { ساكن string s; $$ = &s; }
        |       idAny/*c_identifier*/ '='               { $$ = $1; $<fl>$ = $<fl>1; }
        ;

dpi_tf_import_propertyE<iprop>: // IEEE: [ dpi_function_import_property + dpi_task_import_property ]
                /*خالی_تھاواں*/                             { $$ = iprop_NONE; }
        |       yCONTEXT                                { $$ = iprop_CONTEXT; }
        |       y_خالص                                   { $$ = iprop_خالص; }
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

اظہاریہ_Eq_E<گرہ_Exprp>:             // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' اظہاریہ                                { $$ = $2; }
        ;

اظہاریہ_Or_انکڑے_Type_Eq_E<nodep>:       // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' اظہاریہ_Or_انکڑے_Type                      { $$ = $2; }
        ;

const_اظہاریہ<گرہ_Exprp>:
                اظہاریہ                                    { $$ = $1; }
        ;

اظہاریہ_E<گرہ_Exprp>:               // IEEE: optional expression
                /*empty*/                               { $$ = nullptr; }
        |       اظہاریہ                                    { $$ = $1; }
        ;

اظہاریہ<گرہ_Exprp>:                // IEEE: part of expression/constant_expression/primary
        // *SEE BELOW*          // IEEE: primary/constant_primary
        //
        //                      // IEEE: unary_operator primary
                '+' ~r~اظہاریہ     %prec prUNARYARITH      { $$ = $2; }
        |       '-' ~r~اظہاریہ     %prec prUNARYARITH      { $$ = new AstNegate{$1, $2}; }
        |       '!' ~r~اظہاریہ     %prec prNEGATION        { $$ = new AstLogNot{$1, $2}; }
        |       '&' ~r~اظہاریہ     %prec prREDUCTION       { $$ = new AstRedAnd{$1, $2}; }
        |       '~' ~r~اظہاریہ     %prec prNEGATION        { $$ = new AstNot{$1, $2}; }
        |       '|' ~r~اظہاریہ     %prec prREDUCTION       { $$ = new AstRedOr{$1, $2}; }
        |       '^' ~r~اظہاریہ     %prec prREDUCTION       { $$ = new AstRedXor{$1, $2}; }
        |       yP_NAND ~r~اظہاریہ %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedAnd{$1, $2}}; }
        |       yP_NOR  ~r~اظہاریہ %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedOr{$1, $2}}; }
        |       yP_XNOR ~r~اظہاریہ %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedXor{$1, $2}}; }
        //
        //                      // IEEE: inc_or_dec_expression
        |       ~l~inc_or_dec_عبارت                { $<fl>$ = $<fl>1; $$ = $1; }
        //
        //                      // IEEE: '(' operator_assignment ')'
        //                      // Need exprScope of variable_lvalue to prevent conflict
        |       '(' ~p~اظہاریہ_Scope '='          اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, $4},
                                               $2->clone_درخت_خالص(true)};
                          ASSIGNEQEXPR($<fl>3); }
        |       '(' ~p~اظہاریہ_Scope yP_PLUSEQ    اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new Ast_جوڑو{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_MINUSEQ   اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstSub{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_TIMESEQ   اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstMul{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_DIVEQ     اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstDiv{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_MODEQ     اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstModDiv{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_ANDEQ     اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstAnd{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_OREQ      اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstOr{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_XOREQ     اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new AstXor{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_SLEFTEQ   اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new Ast_کھسکاؤ_L{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_SRIGHTEQ  اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new Ast_کھسکاؤ_R{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        |       '(' ~p~اظہاریہ_Scope yP_SSRIGHTEQ اظہاریہ ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$1, new Ast_سونپو{$3, $2, new Ast_کھسکاؤ_RS{$3, $2->clone_درخت_خالص(true), $4}},
                                               $2->clone_درخت_خالص(true)}; }
        //
        //                      // IEEE: expression binary_operator expression
        |       ~l~اظہاریہ '+' ~r~اظہاریہ                     { $$ = new Ast_جوڑو{$2, $1, $3}; }
        |       ~l~اظہاریہ '-' ~r~اظہاریہ                     { $$ = new AstSub{$2, $1, $3}; }
        |       ~l~اظہاریہ '*' ~r~اظہاریہ                     { $$ = new AstMul{$2, $1, $3}; }
        |       ~l~اظہاریہ '/' ~r~اظہاریہ                     { $$ = new AstDiv{$2, $1, $3}; }
        |       ~l~اظہاریہ '%' ~r~اظہاریہ                     { $$ = new AstModDiv{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_EQUAL ~r~اظہاریہ                { $$ = new AstEq{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_NOTEQUAL ~r~اظہاریہ             { $$ = new AstNeq{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_CASEEQUAL ~r~اظہاریہ            { $$ = new AstEqCase{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_CASENOTEQUAL ~r~اظہاریہ         { $$ = new AstNeqCase{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_WILDEQUAL ~r~اظہاریہ            { $$ = new AstEqWild{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_WILDNOTEQUAL ~r~اظہاریہ         { $$ = new AstNeqWild{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_ANDAND ~r~اظہاریہ               { $$ = new AstLogAnd{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_OROR ~r~اظہاریہ                 { $$ = new AstLogOr{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_POW ~r~اظہاریہ                  { $$ = new AstPow{$2, $1, $3}; }
        |       ~l~اظہاریہ '<' ~r~اظہاریہ                     { $$ = new AstLt{$2, $1, $3}; }
        |       ~l~اظہاریہ '>' ~r~اظہاریہ                     { $$ = new AstGt{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_GTE ~r~اظہاریہ                  { $$ = new AstGte{$2, $1, $3}; }
        |       ~l~اظہاریہ '&' ~r~اظہاریہ                     { $$ = new AstAnd{$2, $1, $3}; }
        |       ~l~اظہاریہ '|' ~r~اظہاریہ                     { $$ = new AstOr{$2, $1, $3}; }
        |       ~l~اظہاریہ '^' ~r~اظہاریہ                     { $$ = new AstXor{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_XNOR ~r~اظہاریہ                 { $$ = new AstNot{$2, new AstXor{$2, $1, $3}}; }
        |       ~l~اظہاریہ yP_NOR ~r~اظہاریہ
                        { $$ = new AstNot{$2, new AstOr{$2, $1, $3}};
                          $2->v3error("Syntax error: '~|' is not a 'nor' binary operator, but a unary '~ |'"
                                      << YYNEWLINE << $<fl>1->warn_ہور() << "... Suggest '~ (... | ...)'"); }
        |       ~l~اظہاریہ yP_NAND ~r~اظہاریہ
                        { $$ = new AstNot{$2, new AstAnd{$2, $1, $3}};
                          $2->v3error("Syntax error: '~&' is not a 'nand' binary operator, but a unary '~ &'"
                                      << YYNEWLINE << $<fl>1->warn_ہور() << "... Suggest '~ (... & ...)'"); }
        |       ~l~اظہاریہ yP_SLEFT ~r~اظہاریہ                { $$ = new Ast_کھسکاؤ_L{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_SRIGHT ~r~اظہاریہ               { $$ = new Ast_کھسکاؤ_R{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_SSRIGHT ~r~اظہاریہ              { $$ = new Ast_کھسکاؤ_RS{$2, $1, $3}; }
        |       ~l~اظہاریہ yP_LTMINUSGT ~r~اظہاریہ            { $$ = new AstLogEq{$2, $1, $3}; }
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
        |       ~l~اظہاریہ yP_MINUSGT ~r~اظہاریہ              { $$ = new AstLogIf{$2, $1, $3}; }
        //
        //                      // <= is special, as we need to disambiguate it with <= assignment
        //                      // We copy all of expr to fexpr and rename this token to a fake one.
        |       ~l~اظہاریہ yP_LTE~f__IGNORE~ ~r~اظہاریہ       { $$ = new AstLte{$2, $1, $3}; }
        //
        //                      // IEEE: conditional_expression
        |       ~l~اظہاریہ '?' ~r~اظہاریہ ':' ~r~اظہاریہ         { $$ = new AstCond{$2, $1, $3, $5}; }
        //
        //                      // IEEE: inside_expression
        |       ~l~اظہاریہ yINSIDE '{' range_list '}'      { $$ = new AstInside{$2, $1, $4}; }
        //
        //                      // IEEE: tagged_union_expression
        //                      // yTAGGED__NONPRIMARY = tokenPipeline determined no primary follows
        |       yTAGGED__NONPRIMARY idAny/*member*/ %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, nullptr}; }
        //                      // yTAGGED = primary follows; handle specific primary types
        //                      // Parenthesized expression
        |       yTAGGED idAny/*member*/ '(' اظہاریہ ')' %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, $4}; }
        //                      // Assignment patterns like tagged Add '{a, b, c}
        |       yTAGGED idAny/*member*/ assignment_نمونہ %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, $3}; }
        //                      // Integer literal
        |       yTAGGED idAny/*member*/ yaINTNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, new AstConst{$<fl>3, *$3}}; }
        //                      // Float literal
        |       yTAGGED idAny/*member*/ yaFLOATNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, new AstConst{$<fl>3, AstConst::RealDouble{}, $3}}; }
        //                      // String literal
        |       yTAGGED idAny/*member*/ yaSTRING %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, new AstConst{$<fl>3, AstConst::VerilogStringLiteral{}, *$3}}; }
        //                      // null literal
        |       yTAGGED idAny/*member*/ yNULL %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, new AstConst{$3, AstConst::Null{}}}; }
        //                      // Identifier as value
        |       yTAGGED idAny/*member*/ idAny %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, new AstParseRef{$<fl>3, *$3, nullptr, nullptr}}; }
        //                      // Concatenation
        |       yTAGGED idAny/*member*/ '{' cateList '}' %prec prTAGGED
                        { $$ = new Ast_Tagged_اظہاریہ{$1, *$2, $4}; }
        //
        //======================// IEEE: primary/constant_primary
        //
        //                      // IEEE: primary_literal (minus string, which is handled specially)
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       ویلا_عدد_Adjusted                         { $$ = $1; }
        |       strAsInt~noStr__IGNORE~                 { $$ = $1; }
        //
        //                      // IEEE: "... hierarchical_identifier select"  see below
        //
        //                      // IEEE: empty_unpacked_array_concatenation
        //                      // IEEE: (aka empty_queue, empty_unpacked_array_concatenation)
        |       '{' '}'                                 { $$ = new Ast_خالی_تھاواں_قطار{$1}; }
        //
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Part of exprOkLvalue below
        //
        //                      // IEEE: multiple_concatenation/constant_multiple_concatenation
        |       '{' const_اظہاریہ '{' cateList '}' '}'
                        { $$ = new AstReplicate{$3, $4, $2}; }
        |       '{' const_اظہاریہ '{' cateList '}' '}' '[' اظہاریہ ']'
                        { $$ = new Ast_Sel_ثنائی_ہندسہ{$7, new AstReplicate{$3, $4, $2}, $8}; }
        |       '{' const_اظہاریہ '{' cateList '}' '}' '[' const_اظہاریہ ':' const_اظہاریہ ']'
                        { $$ = new Ast_Sel_کڈھو{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_اظہاریہ '{' cateList '}' '}' '[' اظہاریہ yP_PLUSCOLON const_اظہاریہ ']'
                        { $$ = new AstSelPlus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_اظہاریہ '{' cateList '}' '}' '[' اظہاریہ yP_MINUSCOLON const_اظہاریہ ']'
                        { $$ = new AstSelMinus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        //                      // UNSUP some other rules above
        //
        //                      // IEEE grammar error: function_subroutine_call [ range_expression ]
        //                      // should be instead:  function_subroutine_call [ part_select_range ]
        |       function_subroutine_call_No_طریقہ
                        { $$ = $1; }
        |       function_subroutine_call_No_طریقہ part_select_rangeList
                        { $$ = GRAMMARP->scrubSel($1, $2); }
        //                      // method_call
        |       ~l~اظہاریہ '.' function_subroutine_call_No_طریقہ
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~اظہاریہ '.' function_subroutine_call_No_طریقہ part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //                      // method_call:array_method requires a '.'
        |       ~l~اظہاریہ '.' array_طریقہ_With
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~اظہاریہ '.' array_طریقہ_With part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //
        //                      // IEEE: let_expression
        //                      // see funcRef
        //
        //                      // IEEE: '(' mintypmax_expression ')'
        |       ~noPar__IGNORE~'(' اظہاریہ ')'             { $$ = $2; }
        |       ~noPar__IGNORE~'(' اظہاریہ ':' اظہاریہ ':' اظہاریہ ')'
                        { $$ = $4; MINTYPMAXDLYUNSUP($4); DEL($2); DEL($6); }
        //                      // PSL rule
        |       '_' '(' اظہاریہ ')'                        { $$ = $3; }    // Arbitrary Verilog inside PSL
        //
        //                      // IEEE: cast/constant_cast
        //                      // expanded from casting_type
        |       simple_typeNoRef yP_TICK '(' اظہاریہ ')'
                        { $$ = new AstCast{$2, $4, VFlag_بچہ_DType{}, $1}; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // Causes conflict, so handled post-parse
        //                      // NO: packageClassScopeE idType yP_TICK '(' expr ')'
        //
        |       yTYPE__ETC '(' اظہاریہ_Or_انکڑے_Type ')' yP_TICK '(' اظہاریہ ')'
                        { $$ = new AstCast{$1, $7, VFlag_بچہ_DType{},
                                           new AstRefDType{$1, AstRefDType::Flag_Type_دا_اظہاریہ{}, $3}}; }
        |       ySIGNED yP_TICK '(' اظہاریہ ')'            { $$ = new AstSigned{$1, $4}; }
        |       yUNSIGNED yP_TICK '(' اظہاریہ ')'          { $$ = new AstUnsigned{$1, $4}; }
        |       ySTRING yP_TICK '(' اظہاریہ ')'            { $$ = new AstCvtPackString{$1, $4}; }
        |       yCONST__ETC yP_TICK '(' اظہاریہ ')'        { $$ = $4; }  // Not linting const presently
        //                      // Spec only allows primary with addition of a type reference
        //                      // We'll be more general, and later assert LHS was a type.
        |       ~l~اظہاریہ yP_TICK '(' اظہاریہ ')'            { $$ = new AstCastParse{$2, $4, $1}; }
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
        |       ~l~اظہاریہ_Ok_Lvalue                         { $$ = $1; }
        //
        //----------------------
        //
        //                      // IEEE: cond_predicate - here to avoid reduce problems
        //                      // Note expr includes cond_pattern
        |       ~l~اظہاریہ yP_ANDANDAND ~r~اظہاریہ            { $$ = new AstConst{$2, AstConst::ثنائی_ہندسہ_False{}};
                                                          BBUNSUP($<fl>2, "Unsupported: &&& expression"); }
        //
        //                      // IEEE: cond_pattern - here to avoid reduce problems
        //                      // "expr yMATCHES pattern"
        //                      // IEEE: pattern - expanded here to avoid conflicts
        |       ~l~اظہاریہ yMATCHES نمونہ_No_اظہاریہ          { $$ = new AstMatches{$2, $1, $3}; }
        |       ~l~اظہاریہ yMATCHES ~r~اظہاریہ                { $$ = new AstMatches{$2, $1, $3}; }
        //
        //                      // IEEE: expression_or_dist - here to avoid reduce problems
        //                      // "expr yDIST '{' dist_list '}'"
        |       ~l~اظہاریہ yDIST '{' dist_list '}'         { $$ = new AstDist{$2, $1, $4}; }
        ;

fexpr<گرہ_Exprp>:                   // For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(اظہاریہ,{s/~l~/f/g; s/~r~/f/g; s/~f__IGNORE~/__IGNORE/g;})   // {copied}
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

اظہاریہ_No_Str<گرہ_Exprp>:               // expression with string removed
                BISONPRE_COPY(اظہاریہ,{s/~noStr__IGNORE~/Ignore/g;})       // {copied}
        ;

اظہاریہ_Ok_Lvalue<گرہ_Exprp>:            // expression that's also OK to use as a variable_lvalue
                ~l~اظہاریہ_Scope                            { $$ = $1; }
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Replicate(1) required as otherwise "{a}" would not be self-determined
        |       '{' cateList '}'                        { $$ = new AstReplicate{$1, $2, 1}; }
        |       '{' cateList '}' '[' اظہاریہ ']'           { $$ = new Ast_Sel_ثنائی_ہندسہ{$4, new AstReplicate{$1, $2, 1}, $5}; }
        |       '{' cateList '}' '[' const_اظہاریہ ':' const_اظہاریہ ']'
                                                        { $$ = new Ast_Sel_کڈھو{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' اظہاریہ yP_PLUSCOLON const_اظہاریہ ']'
                                                        { $$ = new AstSelPlus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' اظہاریہ yP_MINUSCOLON const_اظہاریہ ']'
                                                        { $$ = new AstSelMinus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        //                      // IEEE: assignment_pattern_expression
        //                      // IEEE: [ assignment_pattern_expression_type ] == [ ps_type_id /ps_paremeter_id/data_type]
        //                      // We allow more here than the spec requires
        //UNSUP ~l~exprScope assignment_pattern         { UNSUP }
        |       انکڑے_type assignment_نمونہ            { $$ = $2; إذا ($2) $2->بچہ_DTypep($1); }
        |       assignment_نمونہ                      { $$ = $1; }
        //
        |       streaming_concatenation                 { $$ = $1; }
        ;

fexprOkLvalue<گرہ_Exprp>:           // exprOkLValue, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(اظہاریہ_Ok_Lvalue,{s/~l~/f/g}) // {copied}
        ;

sexprOkLvalue<گرہ_Exprp>:  // exprOkLValue, For use by sequence_expr
                BISONPRE_COPY(اظہاریہ_Ok_Lvalue,{s/~l~/s/g}) // {copied}
        ;

pexprOkLvalue<گرہ_Exprp>:  // exprOkLValue, For use by property_expr
                BISONPRE_COPY(اظہاریہ_Ok_Lvalue,{s/~l~/p/g}) // {copied}
        ;

//UNSUPev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/ev_/g})       // {copied}
//UNSUP ;

//UNSUPpev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/pev_/g})      // {copied}
//UNSUP ;

fexprLvalue<گرہ_Exprp>:             // For use as first part of statement (disambiguates <=)
                fexprOkLvalue                           { $<fl>$ = $<fl>1; $$ = $1; }
        ;

اظہاریہ_Scope<گرہ_Exprp>:               // scope and variable for use to inside an expression
        //                      // Here we've split method_call_root | implicit_class_handle | class_scope | package_scope
        //                      // from the object being called and let expr's "." deal with resolving it.
        //                      // (note method_call_root was simplified to require a primary in 1800-2009)
        //
        //                      // IEEE: [ implicit_class_handle . | class_scope | package_scope ] hierarchical_identifier select
        //                      // Or method_call_body without parenthesis
        //                      // See also varRefClassBit, which is the non-expr version of most of this
                yTHIS                                   { $$ = new AstParseRef{$<fl>1, "this"}; }
        |       y_D_جڑ                                 { $$ = new AstParseRef{$<fl>1, "$root"}; }
        |       idArrayed                               { $$ = $1; }
        |       packageClassScope idArrayed             { $$ = AstDot::newIfPkg($2->fileline(), $1, $2); }
        |       ~l~اظہاریہ '.' idArrayed                   { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        //                      // expr below must be a "yTHIS"
        |       ~l~اظہاریہ '.' ySUPER
                        { AstParseRef* ثابت anodep = VN_CAST($1, ParseRef);
                          إذا (anodep && anodep->name() == "this") {
                              $$ = new AstParseRef{$<fl>1, "super"};
                              $1->delete_درخت();
                          } وإلا {
                              $$ = $1;  $$->v3error("Syntax error: 'super' must be first name component, or after 'this.'");
                          }
                        }
        //                      // Part of implicit_class_handle
        |       ySUPER                                  { $$ = new AstParseRef{$<fl>1, "super"}; }
        ;

fexprScope<گرہ_Exprp>:              // exprScope, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(اظہاریہ_Scope,{s/~l~/f/g})    // {copied}
        ;

sexprScope<گرہ_Exprp>:  // exprScope, For use by sequence_expr
                BISONPRE_COPY(اظہاریہ_Scope,{s/~l~/s/g})    // {copied}
        ;

pexprScope<گرہ_Exprp>:  // exprScope, For use by property_expr
                BISONPRE_COPY(اظہاریہ_Scope,{s/~l~/p/g})    // {copied}
        ;

//UNSUPev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/ev_/g})  // {copied}
//UNSUP ;

//UNSUPpev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/pev_/g}) // {copied}
//UNSUP ;

// PLI calls exclude "" as integers, they're strings
// For $c("foo","bar") we want "bar" as a string, not a Verilog integer.
اظہاریہ_Str_متن<nodep>:
                اظہاریہ_No_Str                               { $$ = $1; }
        |       yaSTRING                                { $$ = new Ast_متن{$<fl>1, GRAMMARP->متن_Quoted($<fl>1, *$1)}; }
        ;

اظہاریہ_Type_Compare<گرہ_Exprp>:
                اظہاریہ                                    { $$ = $1; }
        |       type_referenceBoth                      { $$ = $1; }
        ;

cStrList<nodep>:
                اظہاریہ_Str_متن                             { $$ = $1; }
        |       اظہاریہ_Str_متن ',' cStrList                { $$ = $1->جوڑو_اگلا($3); }
        ;

cateList<گرہ_Exprp>:
        //                      // Not just 'expr' to prevent conflict via stream_concOrExprOrType
                دھارا_عبارت                       { $$ = $1; }
        |       cateList ',' دھارا_عبارت          { $$ = new AstConcat{$2, $1, $3}; }
        ;

اظہاریہ_List_E<گرہ_Exprp>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       اظہاریہ_List                                { $$ = $1; }
        ;

اظہاریہ_List<گرہ_Exprp>:
                اظہاریہ                                    { $$ = $1; }
        |       اظہاریہ_List ',' اظہاریہ                       { $$ = $1->جوڑو_اگلا($3); }
        ;

// identifier_list for 'with' in inline randomize constraints (IEEE 1800-2023 18.7).
// Only simple identifiers; non-identifier expressions are parse errors.
inlineConstraintIdList<گرہ_Exprp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        |       inlineConstraintIdList ',' id           { $$ = $1->جوڑو_اگلا(
                                                             new AstParseRef{$<fl>3, *$3, nullptr, nullptr}); }
        ;

// Optional identifier_list. Empty 'with () {...}' differs from bare 'with {...}'
// via AstWithParse::restricted().
inlineConstraintIdListE<گرہ_Exprp>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       inlineConstraintIdList                  { $$ = $1; }
        ;

اظہاریہ_EList_E<nodep>:  // expression list with empty commas allowed
                اظہاریہ_E                                   { $$ = $1; }
        |       اظہاریہ_EList_E ',' اظہاریہ_E                    { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

اظہاریہ_Disp_List<گرہ_Exprp>:            // exprList for within $display
                اظہاریہ                                    { $$ = $1; }
        |       اظہاریہ_Disp_List ',' اظہاریہ                   { $$ = $1->جوڑو_اگلا($3); }
        //                      // ,, creates a space in $display
        |       اظہاریہ_Disp_List ',' /*empty*/
                        { $$ = $1->جوڑو_اگلا(new AstConst{$<fl>2, AstConst::VerilogStringLiteral{}, " "}); }
        ;

vrdList<گرہ_Exprp>:
                idClassSel                              { $$ = $1; }
        |       vrdList ',' idClassSel                  { $$ = $1->جوڑو_اگلا($3); }
        ;

commasE:
                /*خالی_تھاواں*/                             { } /* ignored */
        |       ',' commasE                             { } /* ignored */
        ;

commaVRDListE<گرہ_Exprp>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       ',' vrdList                             { $$ = $2; }
        ;

دلیلاں_اظہاریہ_List<گرہ_Exprp>:        // IEEE: part of list_of_arguments (used where ,, isn't legal)
                اظہاریہ                                    { $$ = $1; }
        |       دلیلاں_اظہاریہ_List ',' اظہاریہ                   { $$ = $1->جوڑو_اگلا($3); }
        ;

دلیلاں_اظہاریہ_List_E<argp>:       // IEEE: part of list_of_arguments
                دلیلاں_اظہاریہ_One_E                            { $$ = $1; }
        |       دلیلاں_اظہاریہ_List_E ',' دلیلاں_اظہاریہ_One_E          { $$ = $1->جوڑو_اگلا($3); }
        ;

//UNSUPpev_argsExprListE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsExprOneE                        { $$ = $1; }
//UNSUP |       pev_argsExprListE ',' pev_argsExprOneE  { $$ = addNextNull($1, $3); }
//UNSUP ;

دلیلاں_اظہاریہ_One_E<argp>:        // IEEE: part of list_of_arguments
                /*empty*/                               { $$ = new AstArg{CRELINE(), "", nullptr}; }
        |       اظہاریہ                                    { $$ = new AstArg{$1->fileline(), "", $1}; }
        ;

//UNSUPpev_argsExprOneE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         /*empty*/                               { $$ = nullptr; }       // ,, is legal in list_of_arguments
//UNSUP |       pev_expr                                { $$ = $1; }
//UNSUP ;

دلیلاں_Dotted_List<argp>:      // IEEE: part of list_of_arguments
                دلیلاں_Dotted                              { $$ = $1; }
        |       دلیلاں_Dotted_List ',' دلیلاں_Dotted           { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

//UNSUPpev_argsDottedList<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsDotted                          { $$ = $1; }
//UNSUP |       pev_argsDottedList ',' pev_argsDotted   { $$ = addNextNull($1, $3); }
//UNSUP ;

دلیلاں_Dotted<argp>:          // IEEE: part of list_of_arguments
                '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
        |       '.' idAny '(' اظہاریہ ')'                  { $$ = new AstArg{$<fl>2, *$2, $4}; }
        ;

//UNSUPpev_argsDotted<argp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
//UNSUP |       '.' idAny '(' pev_expr ')'              { $$ = new AstArg{$<fl>2, *$2, $4}; }
//UNSUP ;

streaming_concatenation<گرہ_Streamp>: // ==IEEE: streaming_concatenation
        //                      // Need to disambiguate {<< expr-{ ... expr-} stream_concat }
        //                      // From                 {<< stream-{ ... stream-} }
        //                      // Likewise simple_type's idScoped from constExpr's idScope
        //                      // Thus we allow always any two operations.  Sorry
        //                      // IEEE: "'{' yP_SL/R             stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R simple_type stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R constExpr   stream_concatenation '}'"
                '{' yP_SLEFT  دھارا_concatenation '}'
                        { $$ = new Ast_دھارا_L{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SRIGHT دھارا_concatenation '}'
                        { $$ = new Ast_دھارا_R{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SLEFT  دھارا_عبارت_Or_انکڑے_Type دھارا_concatenation '}'
                        { $$ = new Ast_دھارا_L{$2, $4, new Ast_Attr_دا{$1, VAttrType::DIM_بٹ_OR_نمبر, $3}}; }
        |       '{' yP_SRIGHT دھارا_عبارت_Or_انکڑے_Type دھارا_concatenation '}'
                        { $$ = new Ast_دھارا_R{$2, $4, new Ast_Attr_دا{$1, VAttrType::DIM_بٹ_OR_نمبر, $3}}; }
        ;

دھارا_concatenation<گرہ_Exprp>:    // ==IEEE: stream_concatenation
        //                      // '{' { stream_expression } '}'
                '{' cateList '}'                        { $$ = $2; }
        ;

دھارا_عبارت<گرہ_Exprp>:   // ==IEEE: stream_expression
        //                      // IEEE: array_range_expression expanded below
                اظہاریہ                                    { $$ = $1; }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ ':' اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ yP_PLUSCOLON  اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ yP_MINUSCOLON اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        ;

دھارا_عبارت_Or_انکڑے_Type<nodep>:     // IEEE: from streaming_concatenation
                اظہاریہ_Or_انکڑے_Type                          { $$ = $1; }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ ':' اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ yP_PLUSCOLON  اظہاریہ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       اظہاریہ yWITH__BRA '[' اظہاریہ yP_MINUSCOLON اظہاریہ ']'
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
                yLET letId '=' اظہاریہ ';'
                        { $$ = $2;
                          $$->جوڑو_Stmtsp(new Ast_Stmt_اظہاریہ{$1, $4}); }
        |       yLET letId '(' let_پورٹ_list_E ')' '=' اظہاریہ ';'
                        { $$ = $2;
                          $$->جوڑو_Stmtsp(new Ast_Stmt_اظہاریہ{$1, $7});
                          $$->جوڑو_Stmtsp($4); }
        ;

let_پورٹ_list_E<nodep>:   // IEEE: [ let_port_list ]
                /*empty*/                               { $$ = nullptr; }
        |       /*emptyStart*/
        /*mid*/         { VARRESET_LIST(VAR); VARIO(INOUT); }
        /*cont*/  let_پورٹ_list                         { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

let_پورٹ_list<nodep>:   // IEEE: let_port_list
                let_پورٹ_شے                           { $$ = $1; }
        |       let_پورٹ_list ',' let_پورٹ_شے         { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

let_پورٹ_شے<varp>:  // IEEE: let_port_Item
        //                      // IEEE: Expanded let_formal_type
                yUNTYPED idAny/*formal_port_identifier*/ variable_dimensionListE اظہاریہ_Eq_E
                        { $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_بچہ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          إذا ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       انکڑے_type idAny/*formal_port_identifier*/ variable_dimensionListE اظہاریہ_Eq_E
                        { BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_بچہ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          إذا ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       implicit_typeE id/*formal_port_identifier*/ variable_dimensionListE اظہاریہ_Eq_E
                        { إذا ($1) BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_بچہ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          إذا ($4) $$->valuep($4);
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
        |       gateBufList ',' gateBuf                 { $$ = $1->جوڑو_اگلا($3); }
        ;
gateBufif0List<nodep>:
                gateBufif0                              { $$ = $1; }
        |       gateBufif0List ',' gateBufif0           { $$ = $1->جوڑو_اگلا($3); }
        ;
gateBufif1List<nodep>:
                gateBufif1                              { $$ = $1; }
        |       gateBufif1List ',' gateBufif1           { $$ = $1->جوڑو_اگلا($3); }
        ;
gateNotList<nodep>:
                gateNot                                 { $$ = $1; }
        |       gateNotList ',' gateNot                 { $$ = $1->جوڑو_اگلا($3); }
        ;
gateNotif0List<nodep>:
                gateNotif0                              { $$ = $1; }
        |       gateNotif0List ',' gateNotif0           { $$ = $1->جوڑو_اگلا($3); }
        ;
gateNotif1List<nodep>:
                gateNotif1                              { $$ = $1; }
        |       gateNotif1List ',' gateNotif1           { $$ = $1->جوڑو_اگلا($3); }
        ;
gateAndList<nodep>:
                gateAnd                                 { $$ = $1; }
        |       gateAndList ',' gateAnd                 { $$ = $1->جوڑو_اگلا($3); }
        ;
gateNandList<nodep>:
                gateNand                                { $$ = $1; }
        |       gateNandList ',' gateNand               { $$ = $1->جوڑو_اگلا($3); }
        ;
gateOrList<nodep>:
                gateOr                                  { $$ = $1; }
        |       gateOrList ',' gateOr                   { $$ = $1->جوڑو_اگلا($3); }
        ;
gateNorList<nodep>:
                gateNor                                 { $$ = $1; }
        |       gateNorList ',' gateNor                 { $$ = $1->جوڑو_اگلا($3); }
        ;
gateXorList<nodep>:
                gateXor                                 { $$ = $1; }
        |       gateXorList ',' gateXor                 { $$ = $1->جوڑو_اگلا($3); }
        ;
gateXnorList<nodep>:
                gateXnor                                { $$ = $1; }
        |       gateXnorList ',' gateXnor               { $$ = $1->جوڑو_اگلا($3); }
        ;
gatePullupList<nodep>:
                gatePullup                              { $$ = $1; }
        |       gatePullupList ',' gatePullup           { $$ = $1->جوڑو_اگلا($3); }
        ;
gatePulldownList<nodep>:
                gatePulldown                            { $$ = $1; }
        |       gatePulldownList ',' gatePulldown       { $$ = $1->جوڑو_اگلا($3); }
        ;
gateUnsupList<nodep>:
                gateUnsup                               { $$ = $1; }
        |       gateUnsupList ',' gateUnsup             { $$ = $1->جوڑو_اگلا($3); }
        ;

gateRangeE<nodep>:
                instRangeListE                          { $$ = $1; GATERANGE(GRAMMARP->scrubRange($1)); }
        ;

gateBuf<nodep>:
                gateFront variable_lvalue ',' اظہاریہ_List ')'
                        { Ast_گرہ_اظہاریہ* inp = $4;
                          طالما (inp->nextp()) inp = VN_AS(inp->nextp(), گرہ_اظہاریہ);
                          $$ = new AstImplicit{$<fl>1, inp->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = GRAMMARP->createGatePin(inp->clone_درخت(false));
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          لكل (Ast_گرہ_اظہاریہ* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), گرہ_اظہاریہ)) {
                              Ast_گرہ_اظہاریہ* ثابت pinRhsp = GRAMMARP->createGatePin(inp->clone_درخت(false));
                              Ast_سونپو_W* ثابت pinAssp = new Ast_سونپو_W{$<fl>1, outp->clone_درخت(false), pinRhsp};
                              $$->جوڑو_اگلا(new AstAlways{pinAssp});
                          }
                          DEL($1); DEL($4); }
        ;
gateNot<nodep>:
                gateFront variable_lvalue ',' اظہاریہ_List ')'
                        { Ast_گرہ_اظہاریہ* inp = $4;
                          طالما (inp->nextp()) inp = VN_AS(inp->nextp(), گرہ_اظہاریہ);
                          $$ = new AstImplicit{$<fl>1, inp->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_درخت(false))};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          لكل (Ast_گرہ_اظہاریہ* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), گرہ_اظہاریہ)) {
                              Ast_گرہ_اظہاریہ* ثابت pinRhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_درخت(false))};
                              Ast_سونپو_W* ثابت pinAssp = new Ast_سونپو_W{$<fl>1, outp->clone_درخت(false), pinRhsp};
                              $$->جوڑو_اگلا(new AstAlways{pinAssp});
                          }
                          DEL($1, $4); }
        ;
gateBufif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_اظہاریہ ',' gate_Pin_اظہاریہ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_درخت(false)};
                          $<implicitp>$->جوڑو_Exprsp($4->clone_درخت(false));
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, $4};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateBufif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_اظہاریہ ',' gate_Pin_اظہاریہ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_درخت(false)};
                          $<implicitp>$->جوڑو_Exprsp($4->clone_درخت(false));
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstBufIf1{$<fl>1, $6, $4};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_اظہاریہ ',' gate_Pin_اظہاریہ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_درخت(false)};
                          $<implicitp>$->جوڑو_Exprsp($4->clone_درخت(false));
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, new AstNot{$<fl>1, $4}};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_اظہاریہ ',' gate_Pin_اظہاریہ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_درخت(false)};
                          $<implicitp>$->جوڑو_Exprsp($4->clone_درخت(false));
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstBufIf1{$<fl>1, $6, new AstNot{$<fl>1, $4}};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateAnd<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = $4;
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateNand<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstNot{$<fl>1, $4};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateOr<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = $4;
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateNor<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstNot{$<fl>1, $4};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateXor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = $4;
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
                          DEL($1); }
        ;
gateXnor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_درخت(false)};
                          Ast_گرہ_اظہاریہ* ثابت rhsp = new AstNot{$<fl>1, $4};
                          Ast_سونپو_W* ثابت ap = new Ast_سونپو_W{$<fl>1, $2, rhsp};
                          $$->جوڑو_اگلا(new AstAlways{ap});
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

gateAndPinList<گرہ_Exprp>:
                gate_Pin_اظہاریہ                             { $$ = $1; }
        |       gateAndPinList ',' gate_Pin_اظہاریہ          { $$ = new AstAnd{$2, $1, $3}; }
        ;
gateOrPinList<گرہ_Exprp>:
                gate_Pin_اظہاریہ                             { $$ = $1; }
        |       gateOrPinList ',' gate_Pin_اظہاریہ           { $$ = new AstOr{$2, $1, $3}; }
        ;
gateXorPinList<گرہ_Exprp>:
                gate_Pin_اظہاریہ                             { $$ = $1; }
        |       gateXorPinList ',' gate_Pin_اظہاریہ          { $$ = new AstXor{$2, $1, $3}; }
        ;
gateUnsupPinList<گرہ_Exprp>:
                gate_Pin_اظہاریہ                             { $$ = $1; }
        |       gateUnsupPinList ',' gate_Pin_اظہاریہ        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

gate_Pin_اظہاریہ<گرہ_Exprp>:
                اظہاریہ                                    { $$ = GRAMMARP->createGatePin($1); }
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
                /*خالی_تھاواں*/                             { $$ = nullptr; }
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
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       pulldown_strength                       { $$ = $1; }
        ;

pulldown_strength<nodep>:  // IEEE: pulldown_strength
                yP_PAR__STRENGTH strength0 ',' strength1 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength1 ',' strength0 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength0 ')'                { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        ;

pullup_strengthE<nodep>:  // IEEE: [ pullup_strength ]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
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
                y_جدول جدول_اندراج_List yENDTABLE         { $$ = new Ast_Udp_جدول{$1, $2}; }
        ;

جدول_اندراج_List<udp_جدول_Linep>:  // IEEE: { combinational_entry + sequential_entry }
                جدول_سطر                              { $$ = $1; }
        |       جدول_اندراج_List جدول_سطر               { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

جدول_سطر<udp_جدول_Linep>:
                جدول_داخلہ_List ya_جدول_LRSEP tablelVal ya_جدول_LINEEND
                        { $$ = new Ast_Udp_جدول_سطر{Ast_Udp_جدول_سطر::UdpCombo{}, $<fl>1, $1, $3}; }
        |       جدول_داخلہ_List ya_جدول_LRSEP tablelVal ya_جدول_LRSEP tablelVal ya_جدول_LINEEND
                        { $$ = new Ast_Udp_جدول_سطر{Ast_Udp_جدول_سطر::UdpSequential{}, $<fl>1, $1, $3, $5}; }
        ;

جدول_داخلہ_List<udp_جدول_سطر_Valp>:
                tablelVal                            { $$ = $1; }
        |       جدول_داخلہ_List tablelVal             { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

tablelVal<udp_جدول_سطر_Valp>:
                ya_جدول_میدان                          { $$ = new Ast_Udp_جدول_سطر_Val{$<fl>1, *$1}; }
        |       '(' ya_جدول_میدان ya_جدول_میدان ')'    { $$ = new Ast_Udp_جدول_سطر_Val{$<fl>2, *$2 + *$3}; }
        ;

//************************************************
// Specify

specify_بلاک<nodep>:               // ==IEEE: specify_block
                specifyFront specify_شے_List yENDSPECIFY   { $$ = $2; }
        |       specifyFront yENDSPECIFY                { $$ = nullptr; }
        ;

specifyFront:  // IEEE: specify_block front
                ySPECIFY                                { GRAMMARP->m_specifyignWarned = false; }
        ;

specify_شے_List<nodep>:            // IEEE: { specify_item }
                specify_شے                            { $$ = $1; }
        |       specify_شے_List specify_شے           { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

specify_شے<nodep>:                // ==IEEE: specify_item
                specparam_declaration                   { $$ = $1; }
        |       system_timing_check                     { $$ = $1; }
        |       junk_منزل_Semi_List ';'
                        { $$ = nullptr;
                          إذا (!GRAMMARP->m_specifyignWarned) {
                              GRAMMARP->m_specifyignWarned = true;
                              $1->v3warn(SPECIFYIGN, "Ignoring unsupported: specify block construct");
                          }
                        }
        ;

specparam_declaration<nodep>:       // ==IEEE: specparam_declaration
                specparam_declarationFront list_دا_specparam_assignments ';'
                        { $$ = $2; }
        ;

specparam_declarationFront:     // IEEE: part of specparam_declaration
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                ySPECPARAM
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_گرہ_DType* dtp = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE};
                          VARDTYPE(dtp); }
        |       ySPECPARAM packed_dimension
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_گرہ_DType* ثابت dtp = GRAMMARP->جوڑو_Range(
                                    new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT}, $2, true);
                          VARDTYPE(dtp); }
        ;

list_دا_specparam_assignments<varp>:  // ==IEEE: list_of_specparam_assignments
                specparam_assignment                    { $$ = $1; }
        |       list_دا_specparam_assignments ',' specparam_assignment  { $$ = $1->جوڑو_اگلا($3); }
        ;

specparam_assignment<varp>:     // ==IEEE: specparam_assignment
                idNotPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          إذا ($4) $$->valuep($4); }
        //                      // IEEE: pulse_control_specparam
        //                      // LRM grammar requires '(' as the first token after assignment,
        //                      // but IEEE provides an example in 30.7.1 where it is omitted.
        //                      // Other simulators also support it.
        |       idPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          إذا ($4) $$->valuep($4); }
        |       idPathpulse sigAttrListE '=' '(' minTypMax ',' minTypMax ')'
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          إذا ($5) $$->valuep($5);
                          DEL($7); }
        ;

system_timing_check<nodep>:         // ==IEEE: system_timing_check
                setuphold_timing_check                      { $$ = $1; }
        ;

setuphold_timing_check<nodep>:      // ==IEEE: $setuphold_timing_check
                yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_حد_بندی ',' timing_check_حد_بندی ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_حد_بندی ',' timing_check_حد_بندی ',' idAnyE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_حد_بندی ',' timing_check_حد_بندی ',' idAnyE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_حد_بندی ',' timing_check_حد_بندی ',' idAnyE ',' minTypMaxE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13, $15); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_حد_بندی ',' timing_check_حد_بندی ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17}; DEL($7, $9, $13, $15);  }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_حد_بندی ',' timing_check_حد_بندی ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17, $19}; DEL($7, $9, $13, $15); }
        ;

timing_check_event<گرہ_Exprp>:      // ==IEEE: $timing_check_event
                terminal_identifier                                                         { $$ = $1; }
        |       yPOSEDGE terminal_identifier                                                { $$ = $2; }
        |       yNEGEDGE terminal_identifier                                                { $$ = $2; }
        |       y_کنارا terminal_identifier                                                   { $$ = $2; }
        |       y_کنارا '[' کنارا_descriptor_list ']' terminal_identifier                      { $$ = $5; }
        |       terminal_identifier yP_ANDANDAND اظہاریہ                                       { $$ = $1; DEL($3); }
        |       yPOSEDGE terminal_identifier yP_ANDANDAND اظہاریہ                              { $$ = $2; DEL($4); }
        |       yNEGEDGE terminal_identifier yP_ANDANDAND اظہاریہ                              { $$ = $2; DEL($4); }
        |       y_کنارا terminal_identifier yP_ANDANDAND اظہاریہ                                 { $$ = $2; DEL($4); }
        |       y_کنارا '[' کنارا_descriptor_list ']' terminal_identifier yP_ANDANDAND اظہاریہ    { $$ = $5; DEL($7); }
        ;

کنارا_descriptor_list:
                yaEDGEDESC                            {  }
        |       کنارا_descriptor_list ',' yaEDGEDESC   {  }
        ;

timing_check_حد_بندی<گرہ_Exprp>:
                اظہاریہ                      { $$ = $1; }
        |       اظہاریہ ':' اظہاریہ ':' اظہاریہ    { $$ = $3; DEL($1, $5); }
        ;

delayed_referenceE<گرہ_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       terminal_identifier                     { $$ = $1; }
        ;

terminal_identifier<گرہ_Exprp>:
                idArrayed     { $$ = $1; }
        ;

idAnyE<strp>:
                /*empty*/                               { $$ = nullptr; }
        |       idAny                                   { $$ = $1; }
        ;

junk_منزل_Semi_List<fl>:
                junk_منزل_Semi                              { $$ = CRELINE(); }
        |       junk_منزل_Semi_List junk_منزل_Semi               { $$ = CRELINE(); }
        ;

junk_منزل_Semi:
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
                yRANDOMIZE                              { ساكن string s = "randomize"; $$ = &s; $<fl>$ = $<fl>1; }
        ;

idSVKwd<strp>:                  // Warn about non-forward compatible Verilog 2001 code
        //                      // yBIT, yBYTE won't work here as causes conflicts
                yDO
                        { ساكن string s = "do"   ; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        |       yFINAL
                        { ساكن string s = "final"; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        ;

variable_lvalue<گرہ_Exprp>:     // IEEE: variable_lvalue or net_lvalue
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

variable_lvalueConcList<گرہ_Exprp>:  // IEEE: part of variable_lvalue: '{' variable_lvalue { ',' variable_lvalue } '}'
                variable_lvalue                                 { $$ = $1; }
        |       variable_lvalueConcList ',' variable_lvalue     { $$ = new AstConcat{$2, $1, $3}; }
        ;

//UNSUPvariable_lvalueList<nodeExprp>:  // IEEE: part of variable_lvalue: variable_lvalue { ',' variable_lvalue }
//UNSUP         variable_lvalue                         { $$ = $1; }
//UNSUP |       variable_lvalueList ',' variable_lvalue { $$ = addNextNull($1, $3); }
//UNSUP ;

idClass<گرہ_Exprp>:             // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
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

idClassSel<گرہ_Exprp>:          // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
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
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "this"}, $3->fromp()->unlink_Fr_پٹھ()}); $$ = $3; }
        |       ySUPER '.' idDottedForeach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "super"}, $3->fromp()->unlink_Fr_پٹھ()}); $$ = $3; }
        |       yTHIS '.' ySUPER '.' idDottedForeach
                        { $5->fromp(new AstDot{$4, false, new AstParseRef{$<fl>3, "super"}, $5->fromp()->unlink_Fr_پٹھ()}); $$ = $5; }
        //                      // Expanded: package_scope idForeach
        |       packageClassScope idDottedForeach
                       { $2->fromp(new AstDot{$<fl>2, true, $1, $2->fromp()->unlink_Fr_پٹھ()}); $$ = $2; }
        ;


// Dotted identifier for typedef - must have at least one '.'
// First component is plain id or id with array index, subsequent components can have arrays
idDottedOrArrayed<گرہ_Exprp>:
                id '.' idArrayed
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, *$1, nullptr, nullptr}, $3}; }
        |       idDottedOrArrayed '.' idArrayed
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

idDotted<گرہ_Exprp>:
                y_D_جڑ '.' id_Dotted_ہور
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_ہور                            { $$ = $1; }
        ;

idDottedSel<گرہ_Exprp>:
                y_D_جڑ '.' id_Dotted_Sel_ہور
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_Sel_ہور                         { $$ = $1; }
        ;

idDottedForeach<foreachHeaderp>:
                y_D_جڑ '.' id_Dotted_ہور_Foreach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3->fromp()->unlink_Fr_پٹھ()}); $$ = $3; }
        |       id_Dotted_ہور_Foreach                     { $$ = $1; }
        ;

id_Dotted_ہور<گرہ_Exprp>:
                parse_Ref_ادھار                            { $$ = $1; }
        |       id_Dotted_ہور '.' parse_Ref_ادھار           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_Sel_ہور<گرہ_Exprp>:
                idArrayed                               { $$ = $1; }
        |       id_Dotted_Sel_ہور '.' idArrayed           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_ہور_Foreach<foreachHeaderp>:
                idArrayedForeach                        { $$ = $1; }
        |       id_Dotted_Sel_ہور '.' idArrayedForeach
                        { $3->fromp(new AstDot{$2, false, $1, $3->fromp()->unlink_Fr_پٹھ()}); $$ = $3; }
        ;

// Single component of dotted path, maybe [#].
// Due to lookahead constraints, we can't know if [:] or [+:] are valid (last dotted part),
// we'll assume so and cleanup later.
// id below includes:
//       enum_identifier
idArrayed<گرہ_Exprp>:               // IEEE: id + select
                id
                        { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        //                      // IEEE: id + part_select_range/constant_part_select_range
        |       idArrayed '[' اظہاریہ ']'                          { $$ = new Ast_Sel_ثنائی_ہندسہ{$2, $1, $3}; }  // Or AstArraySel, don't know yet.
        |       idArrayed '[' const_اظہاریہ ':' const_اظہاریہ ']'       { $$ = new Ast_Sel_کڈھو{$2, $1, $3, $5}; }
        //                      // IEEE: id + indexed_range/constant_indexed_range
        |       idArrayed '[' اظہاریہ yP_PLUSCOLON  const_اظہاریہ ']'  { $$ = new AstSelPlus{$2, $1, $3, $5}; }
        |       idArrayed '[' اظہاریہ yP_MINUSCOLON const_اظہاریہ ']'  { $$ = new AstSelMinus{$2, $1, $3, $5}; }
        ;

idArrayedForeach<foreachHeaderp>:    // IEEE: id + select (under foreach expression)
                parse_Ref_ادھار  // Malformed, but accept for better error reporting
                        { $$ = new Ast_Foreach_سرخی{$<fl>1, $1, nullptr}; }
        |       idArrayed '[' اظہاریہ ']'
                        { $$ = new Ast_Foreach_سرخی{$2, $1, $3}; }
        |       idArrayed '[' ']'
                        { $$ = new Ast_Foreach_سرخی{$2, $1, new Ast_خالی_تھاواں{$3}}; }
        |       idArrayed '[' parse_Ref_ادھار ',' loop_variables ']'
                        { $$ = new Ast_Foreach_سرخی{$2, $1, جوڑو_اگلا_Null(static_cast<Ast_گرہ*>($3), $5)}; }
        |       idArrayed '[' ',' loop_variables ']'
                        { $$ = new Ast_Foreach_سرخی{$2, $1, جوڑو_اگلا_Null(static_cast<Ast_گرہ*>(new Ast_خالی_تھاواں{$3}), $4)}; }
        ;

// ParseRef without any dots or vectorizaion
parse_Ref_ادھار<parseRefp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

// yaSTRING shouldn't be used directly, instead via an abstraction below
str<strp>:                      // yaSTRING but with \{escapes} need decoded
                yaSTRING                                { $$ = PARSEP->newString(GRAMMARP->unquoteString($<fl>1, *$1)); }
        ;

strAsInt<گرہ_Exprp>:
                yaSTRING
                        { // Numeric context, so IEEE 1800-2017 11.10.3 "" is a "\000"
                          $$ = new AstConst{$<fl>1, AstConst::VerilogStringLiteral{}, GRAMMARP->unquoteString($<fl>1, *$1)}; }
        ;

strAsIntIgnore<گرہ_Exprp>:          // strAsInt, but never matches for when expr shouldn't parse strings
                yaSTRING__IGNORE                        { $$ = nullptr; yyerror("Impossible token"); }
        ;

شروع_Label_E<strp>:
                /*خالی_تھاواں*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        ;

سرا_Label_E<strp>:
                /*خالی_تھاواں*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        |       ':' yNEW__ETC                           { ساكن string n = "new"; $$ = &n; $<fl>$ = $<fl>2; }
        ;

//************************************************
// Clocking

clocking_declaration<nodep>:            // IEEE: clocking_declaration
                yCLOCKING idAny clocking_event ';' clocking_شے_List_E yENDCLOCKING سرا_Label_E
                        { $$ = new AstClocking{$<fl>2, *$2, new Ast_Sen_درخت{$<fl>2, $3}, $5, false, false}; }
        |       yDEFAULT yCLOCKING clocking_event ';' clocking_شے_List_E yENDCLOCKING سرا_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_درخت{$<fl>2, $3}, $5, true, false}; }
        |       yDEFAULT yCLOCKING idAny clocking_event ';' clocking_شے_List_E yENDCLOCKING سرا_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_درخت{$<fl>3, $4}, $6, true, false}; }
        |       yGLOBAL__CLOCKING yCLOCKING clocking_event ';' clocking_شے_List_E yENDCLOCKING سرا_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_درخت{$<fl>2, $3}, $5, false, true}; }
        |       yGLOBAL__CLOCKING yCLOCKING idAny clocking_event ';' clocking_شے_List_E yENDCLOCKING سرا_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_درخت{$<fl>3, $4}, $6, false, true}; }
        ;

clocking_eventE<senItemp>:      // IEEE: optional clocking_event
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       clocking_event                          { $$ = $1; }
        ;

clocking_event<senItemp>:       // IEEE: clocking_event
        //                      // IEEE: '@' ps_identifier
        //                      // IEEE: '@' hierarchical_identifier
        //UNSUP: '@' idClassSel/*ps_identifier*/
                '@' id
                        { $$ = new Ast_Sen_شے{$<fl>2, VEdgeType::ET_بدلیا,
                                              new AstParseRef{$<fl>2, *$2, nullptr, nullptr}}; }
        |       '@' '(' event_عبارت ')'            { $$ = $3; }
        ;

clocking_شے_List_E<nodep>:
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       clocking_شے_List                       { $$ = $1; }
        ;

clocking_شے_List<nodep>:  // IEEE: [ clocking_item ]
                clocking_شے                           { $$ = $1; }
        |       clocking_شے_List clocking_شے         { إذا ($1) $$ = جوڑو_اگلا_Null($1, $2); }
        ;

clocking_شے<nodep>:   // IEEE: clocking_item
                yDEFAULT y_داخلہ clocking_skew ';'       { $$ = new Ast_Clocking_شے{$<fl>1, VDirection::داخلہ, $3, nullptr}; }
        |       yDEFAULT yOUTPUT clocking_skew ';'      { $$ = new Ast_Clocking_شے{$<fl>1, VDirection::OUTPUT, $3, nullptr}; }
        |       yDEFAULT y_داخلہ clocking_skew yOUTPUT clocking_skew ';'
                        { $$ = new Ast_Clocking_شے{$<fl>1, VDirection::داخلہ, $3, nullptr};
                          $$->جوڑو_اگلا(new Ast_Clocking_شے{$<fl>4, VDirection::OUTPUT, $5, nullptr}); }
        |       y_داخلہ clocking_skewE list_دا_clocking_decl_سونپو ';'
                        { $$ = GRAMMARP->make_Clocking_شے_List($<fl>1, VDirection::داخلہ, $2, $3); }
        |       yOUTPUT clocking_skewE list_دا_clocking_decl_سونپو ';'
                        { $$ = GRAMMARP->make_Clocking_شے_List($<fl>1, VDirection::OUTPUT, $2, $3); }
        |       y_داخلہ clocking_skewE yOUTPUT clocking_skewE list_دا_clocking_decl_سونپو ';'
                        { $$ = GRAMMARP->make_Clocking_شے_List($<fl>1, VDirection::داخلہ, $2, $5->clone_درخت(true));
                          $$->جوڑو_اگلا(GRAMMARP->make_Clocking_شے_List($<fl>3, VDirection::OUTPUT, $4, $5)); }
        |       yINOUT list_دا_clocking_decl_سونپو ';'
                        { $$ = GRAMMARP->make_Clocking_شے_List($<fl>1, VDirection::داخلہ, nullptr, $2->clone_درخت(true));
                          $$->جوڑو_اگلا(GRAMMARP->make_Clocking_شے_List($<fl>1, VDirection::OUTPUT, nullptr, $2)); }
        |       assertion_شے_declaration
                        { $$ = $1;
                          لكل (Ast_گرہ* nodep = $1; nodep; nodep = nodep->nextp()) {
                              إذا (!VN_IS(nodep, Sequence)) {
                                  $$ = nullptr;
                                  BBUNSUP(nodep, "Unsupported: assertion items in clocking blocks");
                                  DEL($1);
                                  اكسر;
                              }
                          }
                        }
        ;

list_دا_clocking_decl_سونپو<nodep>:  // IEEE: list_of_clocking_decl_assign
                clocking_decl_سونپو                    { $$ = $1; }
        |       list_دا_clocking_decl_سونپو ',' clocking_decl_سونپو
                        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

clocking_decl_سونپو<nodep>:    // IEEE: clocking_decl_assign
                idAnyAsParseRef/*new-signal_identifier*/ اظہاریہ_Eq_E
                        { AstParseRef* ثابت refp = $1;
                          $$ = refp;
                          إذا ($2) $$ = new Ast_سونپو{$<fl>2, refp, $2}; }
        ;

clocking_skewE<گرہ_Exprp>:          // IEEE: [clocking_skew]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       clocking_skew                           { $$ = $1; }
        ;

clocking_skew<گرہ_Exprp>:           // IEEE: clocking_skew
                delay_control                           { $$ = $1->lhsp()->unlink_Fr_پٹھ(); $1->delete_درخت(); }
        |      yPOSEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      yNEGEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      y_کنارا delay_controlE                     { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        ;

چکر_delay<delayp>:  // IEEE: cycle_delay
               yP_POUNDPOUND yaINTNUM
                        { $$ = new AstDelay{$<fl>1, new AstConst{$<fl>2, *$2}, true}; }
        |      yP_POUNDPOUND idAnyAsParseRef
                        { $$ = new AstDelay{$<fl>1, $2, true}; }
        |      yP_POUNDPOUND '(' اظہاریہ ')'
                        { $$ = new AstDelay{$<fl>1, $3, true}; }
        ;

//************************************************
// Asserts

assertion_شے_declaration<nodep>:  // ==IEEE: assertion_item_declaration
                property_declaration                    { $$ = $1; }
        |       sequence_declaration                    { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

assertion_شے<nodep>:          // ==IEEE: assertion_item
                concurrent_assertion_شے               { $$ = $1; }
        |       deferred_فوری_assertion_شے
                        { $$ = $1 ? new AstAlways{$1->fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr, $1} : nullptr; }
        ;

deferred_فوری_assertion_شے<nodep>:       // ==IEEE: deferred_immediate_assertion_item
                deferred_فوری_assertion_statement  { $$ = $1; }
        |       id/*block_identifier*/ ':' deferred_فوری_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        ;

procedural_assertion_statement<گرہ_Stmtp>:  // ==IEEE: procedural_assertion_statement
                concurrent_assertion_statement          { $$ = $1; }
        |       فوری_assertion_statement           { $$ = $1; }
        //                      // IEEE: checker_instantiation
        //                      // Unlike modules, checkers are the only "id id (...)" form in statements.
        //UNSUP checker_instantiation                   { $$ = $1; }
        ;

فوری_assertion_statement<گرہ_Stmtp>:   // ==IEEE: immediate_assertion_statement
                simple_فوری_assertion_statement    { $$ = $1; }
        |       deferred_فوری_assertion_statement  { $$ = $1; }
        ;

simple_فوری_assertion_statement<گرہ_Stmtp>:    // ==IEEE: simple_immediate_assertion_statement
        //                      // action_block expanded here, for compatibility with AstAssert
                assertOrAssume '(' اظہاریہ ')' stmt %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $3, $5, nullptr, VAssertType::SIMPLE_فوری, $1}; }
        |       assertOrAssume '(' اظہاریہ ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, nullptr, $6, VAssertType::SIMPLE_فوری, $1}; }
        |       assertOrAssume '(' اظہاریہ ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, $5, $7, VAssertType::SIMPLE_فوری, $1}; }
        //                      // IEEE: simple_immediate_cover_statement
        |       yCOVER '(' اظہاریہ ')' stmt                { $$ = new AstCover{$1, $3, $5, VAssertType::SIMPLE_فوری}; }
        ;

assertOrAssume<assertdirectivetypeen>:
                yASSERT                                 { $$ = VAssertDirectiveType::ASSERT; }
        |       yASSUME                                 { $$ = VAssertDirectiveType::ASSUME; }
        ;

final_zero<asserttypeen>:                     // IEEE: part of deferred_immediate_assertion_statement
                '#' yaINTNUM
                        { إذا ($2->isNeqZero()) { $<fl>2->v3error("Deferred assertions must use '#0' (IEEE 1800-2023 16.4)"); }
                          $$ = VAssertType::OBSERVED_DEFERRED_فوری; }
        //                      // 1800-2012:
        |       yFINAL                                                  { $$ = VAssertType::FINAL_DEFERRED_فوری; }
        ;

deferred_فوری_assertion_statement<گرہ_Stmtp>:  // ==IEEE: deferred_immediate_assertion_statement
        //                      // IEEE: deferred_immediate_assert_statement
                assertOrAssume final_zero '(' اظہاریہ ')' stmt %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $4, $6, nullptr, $2, $1}; }
        |       assertOrAssume final_zero '(' اظہاریہ ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, nullptr, $7, $2, $1}; }
        |       assertOrAssume final_zero '(' اظہاریہ ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, $6, $8, $2, $1}; }
        //                      // IEEE: deferred_immediate_cover_statement
        |       yCOVER final_zero '(' اظہاریہ ')' stmt     { $$ = new AstCover{$1, $4, $6, $2}; }
        ;

concurrent_assertion_شے<nodep>:       // IEEE: concurrent_assertion_item
                concurrent_assertion_statement          { $$ = $1; }
        |       id/*block_identifier*/ ':' concurrent_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        //                      // IEEE: checker_instantiation
        //                      // identical to module_instantiation; see etcInst
        ;

concurrent_assertion_statement<گرہ_Stmtp>:  // ==IEEE: concurrent_assertion_statement
        //                      // IEEE: assert_property_statement
        //                      // IEEE: assume_property_statement
        //                      // action_block expanded here
                assertOrAssume yPROPERTY '(' property_spec ')' stmt %prec pr_ہیٹھلا_THAN_ELSE
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
                        { AstCover* ثابت coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), nullptr, nullptr, $4},
                              $6, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event sexpr ')' stmt
                        { AstCover* ثابت coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, nullptr, $5},
                              $7, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event yDISABLE yIFF '(' اظہاریہ/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* ثابت coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, $8, $10},
                              $12, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' yDISABLE yIFF '(' اظہاریہ/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* ثابت coverp = new AstCover{$1,
                              new AstPropSpec{$7->fileline(), nullptr, $7, $9},
                              $11, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        //                      // IEEE: restrict_property_statement
        |       yRESTRICT yPROPERTY '(' property_spec ')' ';'
                        { $$ = new AstRestrict{$1, $4}; }
        ;

property_declaration<گرہ_FTaskp>:  // ==IEEE: property_declaration
                property_declarationFront property_پورٹ_list_E ';' property_declarationBody
                        yENDPROPERTY سرا_Label_E
                        { $$ = $1;
                          $$->جوڑو_Stmtsp($2);
                          $$->جوڑو_Stmtsp($4);
                          GRAMMARP->سرا_Label($<fl>6, $$, $6);
                          GRAMMARP->m_insideProperty = false;
                          GRAMMARP->m_typed_Property_پورٹ = false; }
        ;

property_declarationFront<گرہ_FTaskp>:  // IEEE: part of property_declaration
                yPROPERTY idAny/*property_identifier*/
                        { $$ = new AstProperty{$<fl>2, *$2, nullptr};
                          GRAMMARP->m_insideProperty = true; }
        ;

property_پورٹ_list_E<nodep>:  // IEEE: [ ( [ property_port_list ] ) ]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '(' property_پورٹ_list ')'              { $$ = $2; }
        ;

property_پورٹ_list<nodep>:  // ==IEEE: property_port_list
                property_پورٹ_شے                              { $$ = $1; }
        |       property_پورٹ_list ',' property_پورٹ_شے       { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

property_پورٹ_شے<nodep>:  // IEEE: property_port_item/sequence_port_item
        //                      // Merged in sequence_port_item
        //                      // IEEE: property_lvar_port_direction ::= yINPUT
        //                      // prop IEEE: [ yLOCAL [ yINPUT ] ] property_formal_type
        //                      //           id {variable_dimension} [ '=' property_actual_arg ]
        //                      // seq IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
                property_پورٹ_شے_Front property_پورٹ_شے_Assignment  { $$ = $2; }
        ;

property_پورٹ_شے_Front:  // IEEE: part of property_port_item/sequence_port_item
                property_پورٹ_شے_Dir_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       property_پورٹ_شے_Dir_E انکڑے_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_پورٹ = true; }
        |       property_پورٹ_شے_Dir_E yVAR انکڑے_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_پورٹ = true; }
        |       property_پورٹ_شے_Dir_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       property_پورٹ_شے_Dir_E implicit_typeE
                        { VARDTYPE($2); }
        ;

property_پورٹ_شے_Assignment<nodep>:  // IEEE: part of property_port_item/sequence_port_item
                id variable_dimensionListE
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, nullptr); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, $4);
                          BBUNSUP($3, "Unsupported: property variable default value"); }
        ;

property_پورٹ_شے_Dir_E:
                /*خالی_تھاواں*/
                        { VARIO(داخلہ); }
        |       yLOCAL__ETC
                        { VARIO(داخلہ);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        |       yLOCAL__ETC y_داخلہ
                        { VARIO(داخلہ);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        ;

property_declarationBody<nodep>:  // IEEE: part of property_declaration
                assertion_variable_declarationList property_spec
                        { $$ = جوڑو_اگلا_Null($1, $2); }
        |       assertion_variable_declarationList property_spec ';'
                        { $$ = جوڑو_اگلا_Null($1, $2); }
        //                      // IEEE-2012: Incorrectly has yCOVER ySEQUENCE then property_spec here.
        //                      // Fixed in IEEE 1800-2017
        |       property_spec                           { $$ = $1; }
        |       property_spec ';'                       { $$ = $1; }
        ;

assertion_variable_declarationList<nodep>:  // IEEE: part of assertion_variable_declaration
                assertion_variable_declaration          { $$ = $1; }
        |       assertion_variable_declarationList assertion_variable_declaration
                        { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

sequence_declaration<گرہ_FTaskp>:  // ==IEEE: sequence_declaration
                sequence_declarationFront sequence_پورٹ_list_E ';' sequence_declarationBody
        /*cont*/    yENDSEQUENCE سرا_Label_E
                        { $$ = $1;
                          $$->جوڑو_Stmtsp($2);
                          $$->جوڑو_Stmtsp($4);
                          GRAMMARP->سرا_Label($<fl>6, $$, $6);
                          // Sequence declarations are allowed; they are inlined by V3AssertPre.
                          // Sequences in unsupported contexts are reported by assertPreAll
                          // after inlining.
                        }
        ;

sequence_declarationFront<گرہ_FTaskp>:  // IEEE: part of sequence_declaration
                ySEQUENCE idAny/*new_sequence*/
                        { $$ = new AstSequence{$<fl>2, *$2, nullptr}; }
        ;

sequence_پورٹ_list_E<nodep>:  // IEEE: [ ( [ sequence_port_list ] ) ]
        //                      // IEEE: sequence_lvar_port_direction ::= yINPUT | yINOUT | yOUTPUT
        //                      // IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
        //                      // All this is almost identically the same as a property.
        //                      // Difference is only yINOUT/yOUTPUT (which might be added to 1800-2012)
        //                      // and yPROPERTY.  So save some work.
                property_پورٹ_list_E                     { $$ = $1; }
        ;

property_formal_typeNoDt<گرہ_DTypep>:  // IEEE: property_formal_type (w/o implicit)
                sequence_formal_typeNoDt                { $$ = $1; }
        |       yPROPERTY
                        { $$ = nullptr; GRAMMARP->m_typed_Property_پورٹ = false;
                          BBUNSUP($1, "Unsupported: property argument data type"); }
        ;

sequence_formal_typeNoDt<گرہ_DTypep>:  // ==IEEE: sequence_formal_type (w/o data_type_or_implicit)
                        // IEEE: data_type_or_implicit
                        // implicit expanded where used
                ySEQUENCE
                        { $$ = nullptr; GRAMMARP->m_typed_Property_پورٹ = false;
                          BBUNSUP($1, "Unsupported: sequence argument data type"); }
                        // IEEE-2009: yEVENT
                        // already part of data_type.  Removed in 1800-2012.
        |       yUNTYPED
                        { $$ = nullptr; GRAMMARP->m_typed_Property_پورٹ = false; }
        ;

sequence_declarationBody<nodep>:  // IEEE: part of sequence_declaration
        //                      // 1800-2012 makes ';' optional
                assertion_variable_declarationList sexpr        { $$ = جوڑو_اگلا_Null($1, $2); }
        |       assertion_variable_declarationList sexpr ';'    { $$ = جوڑو_اگلا_Null($1, $2); }
        |       sexpr                                   { $$ = $1; }
        |       sexpr ';'                               { $$ = $1; }
        //                      // IEEE: clocking_event sequence_expr (16.7)
        //                      // A leading clocking event on a named sequence body.
        |       '@' '(' event_عبارت ')' sexpr      { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' '(' event_عبارت ')' sexpr ';'  { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' senitemVar sexpr                    { $$ = new AstSClocked{$1, $2, $3}; }
        |       '@' senitemVar sexpr ';'                { $$ = new AstSClocked{$1, $2, $3}; }
        ;

property_spec<propSpecp>:               // IEEE: property_spec
        //UNSUP: This rule has been super-specialized to what is supported now
        //UNSUP remove below
                '@' '(' senitem ')' yDISABLE yIFF '(' اظہاریہ ')' property_اظہاریہ_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($3); $$->disablep($8); }
        |       '@' '(' senitem ')' property_اظہاریہ_Spec
                        { $$ = $5; $$->fileline($1); $$->sensesp($3); }
        |       '@' senitemVar property_اظہاریہ_Spec
                        { $$ = $3; $$->fileline($1); $$->sensesp($2); }
        |       yDISABLE yIFF '(' اظہاریہ ')' '@' '(' senitem ')' property_اظہاریہ_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($8); $$->disablep($4); }
        //UNSUP remove above
        |       yDISABLE yIFF '(' اظہاریہ ')' property_اظہاریہ_Spec
                        { $$ = $6; $$->fileline($4->fileline()); $$->disablep($4); }
        |       property_اظہاریہ_Spec                       { $$ = $1; }
        ;

property_اظہاریہ_Spec<propSpecp>:  // A property expression plus explicit weak/strong strength
                pexpr
                        { $$ = new AstPropSpec{$1->fileline(), nullptr, nullptr, $1}; }
        |       ySTRONG '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::STRONG}; }
        |       yWEAK '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::WEAK}; }
        ;

property_اظہاریہ_Case_If<گرہ_Exprp>:  // IEEE: part of property_expr for if/case
                yCASE '(' اظہاریہ/*expression_or_dist*/ ')' property_case_شے_List yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, $5); }
        |       yCASE '(' اظہاریہ/*expression_or_dist*/ ')' yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, nullptr); }
        |       yIF '(' اظہاریہ/*expression_or_dist*/ ')' pexpr  %prec prIF
                        { $$ = new AstImplication{$1, $3, $5, true}; }
        |       yIF '(' اظہاریہ/*expression_or_dist*/ ')' pexpr yELSE pexpr
                        { Ast_گرہ_اظہاریہ* ثابت elseCondp = new AstLogNot{$1, $3->clone_درخت_خالص(false)};
                          $$ = new AstSAnd{$1, new AstImplication{$1, $3, $5, true},
                                           new AstImplication{$1, elseCondp, $7, true},
                                           /*propertyControl=*/true}; }
        ;

property_case_شے_List<caseItemp>:  // IEEE: {property_case_item}
                property_case_شے                      { $$ = $1; }
        |       property_case_شے_List property_case_شے   { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

property_case_شے<caseItemp>:  // ==IEEE: property_case_item
        //                      // IEEE: expression_or_dist { ',' expression_or_dist } ':' property_expr
        //                      // IEEE 1800-2012 changed from property_statement to property_expr
        //                      // IEEE 1800-2017 changed to require the semicolon
                caseCondList ':' pexpr ';'              { $$ = new Ast_Case_شے{$2, $1, $3}; }
        |       yDEFAULT pexpr ';'                      { $$ = new Ast_Case_شے{$1, nullptr, $2}; }
        |       yDEFAULT ':' pexpr ';'                  { $$ = new Ast_Case_شے{$1, nullptr, $3}; }
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

pexpr<گرہ_Exprp>:  // IEEE: property_expr  (The name pexpr is important as regexps just add an "p" to expr.)
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
        |       property_اظہاریہ_Case_If                     { $$ = $1; }
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
        |       yNEXTTIME '[' const_اظہاریہ ']' pexpr %prec yNEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: nexttime[] (in property expression)"); DEL($3); }
        |       yS_NEXTTIME '[' const_اظہاریہ ']' pexpr %prec yS_NEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: s_nexttime[] (in property expression)"); DEL($3); }
        |       yALWAYS pexpr  %prec prALWAYS
                        { $$ = $2; }
        |       yALWAYS '[' const_اظہاریہ ':' const_اظہاریہ ']' pexpr  %prec prALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, false}; }
        |       yS_ALWAYS '[' const_اظہاریہ ':' const_اظہاریہ ']' pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, true}; }
        |       yS_ALWAYS pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $2, new AstUnbounded{$1}, new AstUnbounded{$1}, true}; }
        |       yS_EVENTUALLY pexpr  %prec prS_EVENTUALLY
                        {
                            $$ = new AstSEventually{$1, $2};
                            PARSEP->import_If_وچ_Std($1, "process", true);
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
        |       ~o~pexpr y_سٹا_دیندا pexpr
                        { $$ = new AstLogOr{$2, new AstLogNot{$2, $1}, $3}; }
        //                      // yIFF also used by event_expression
        |       ~o~pexpr yIFF pexpr
                        { $$ = new AstLogEq{$2, $1, $3}; }
        |       yACCEPT_ON '(' اظہاریہ/*expression_or_dist*/ ')' pexpr  %prec prACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_قسم::ACCEPT_ON, $3, $5}; }
        |       yREJECT_ON '(' اظہاریہ/*expression_or_dist*/ ')' pexpr  %prec prREJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_قسم::REJECT_ON, $3, $5}; }
        |       ySYNC_ACCEPT_ON '(' اظہاریہ/*expression_or_dist*/ ')' pexpr %prec prSYNC_ACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_قسم::SYNC_ACCEPT_ON, $3, $5}; }
        |       ySYNC_REJECT_ON '(' اظہاریہ/*expression_or_dist*/ ')' pexpr %prec prSYNC_REJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_قسم::SYNC_REJECT_ON, $3, $5}; }
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
        |       BISONPRE_COPY_ONCE(اظہاریہ,{s/~l~/p/g; s/~p~/p/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

sexpr<گرہ_Exprp>:  // ==IEEE: sequence_expr  (The name sexpr is important as regexps just add an "s" to expr.)
        //                      // ********* RULES COPIED IN sequence_exprProp
        //                      // For precedence, see IEEE 17.7.1
        //
        //                      // IEEE: "cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // IEEE: "sequence_expr cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // Both rules basically mean we can repeat sequences, so make it simpler:
                چکر_delay_range ~p~sexpr  %prec yP_POUNDPOUND
                        { $$ = new AstSExpr{$<fl>1, $1, $2}; }
        |       ~p~sexpr چکر_delay_range sexpr %prec prPOUNDPOUND_MULTI
                        { $$ = new AstSExpr{$<fl>2, $1, $2, $3}; }
        //
        //                      // IEEE: expression_or_dist [ boolean_abbrev ]
        //                      // Note expression_or_dist includes "expr"!
        //                      // sexpr/*sexpression_or_dist*/  --- Hardcoded below
        //                      // Consecutive repetition (IEEE 1800-2023 16.9.2)
        //                      // [*N] exact count
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_اظہاریہ ']'
                        { $$ = new AstSConsRep{$<fl>2, $1, $3}; }
        //                      // [*N:M] bounded range or [*N:$] unbounded range
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_اظہاریہ ':' const_اظہاریہ ']'
                        {
                            إذا (VN_IS($5, Unbounded)) {
                                DEL($5);
                                $$ = new AstSConsRep{$<fl>2, $1, $3, nullptr, true};
                            } وإلا {
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
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_اظہاریہ ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3}; }
        //                      // IEEE: goto_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_اظہاریہ ':' const_اظہاریہ ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3, $5}; }
        //                      // IEEE: nonconsecutive_repetition (single count form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_اظہاریہ ']'
                        { $$ = new AstSNonConsRep{$<fl>2, $1, $3}; }
        //                      // IEEE: nonconsecutive_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_اظہاریہ ':' const_اظہاریہ ']'
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
        |       '(' ~p~sexpr ',' sequence_میل_شے_List ')'
                        { $$ = new Ast_اظہاریہ_Stmt{$3, $4, $2}; }
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
        |       y_پہلا_میل '(' sexpr ')'
                        { $$ = $3; }  // IEEE 16.9.8: no-op in deterministic DFA model
        |       y_پہلا_میل '(' sexpr ',' sequence_میل_شے_List ')'
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
        |       BISONPRE_COPY_ONCE(اظہاریہ,{s/~l~/s/g; s/~p~/s/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

چکر_delay_range<delayp>:  // IEEE: ==cycle_delay_range
        //                      // These three terms in 1800-2005 ONLY
                yP_POUNDPOUND intnumAsConst
                        { $$ = new AstDelay{$1, $2, true}; }
        |       yP_POUNDPOUND idAny
                        { $$ = new AstDelay{$1, new AstParseRef{$<fl>2, *$2}, true}; }
        |       yP_POUNDPOUND '(' const_اظہاریہ ')'
                        { $$ = new AstDelay{$1, $3, true}; }
        //                      // In 1800-2009 ONLY:
        //                      // IEEE: yP_POUNDPOUND constant_primary
        //                      // UNSUP: This causes a big grammar ambiguity
        //                      // as ()'s mismatch between primary and the following statement
        //                      // the sv-ac committee has been asked to clarify  (Mantis 1901)
        |       yP_POUNDPOUND '[' const_اظہاریہ ':' const_اظہاریہ ']'
                        { $$ = new AstDelay{$1, $3, true};
                                                                                                        $$->rhsp($5); }
        |       yP_POUNDPOUND yP_BRASTAR ']'
                        { $$ = new AstDelay{$1, new AstConst{$1, 0}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        |       yP_POUNDPOUND yP_BRAPLUSKET
                        { $$ = new AstDelay{$1, new AstConst{$1, 1}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        ;

sequence_میل_شے_List<nodep>:  // IEEE: [sequence_match_item] part of sequence_expr
                sequence_میل_شے                     { $$ = $1; }
        |       sequence_میل_شے_List ',' sequence_میل_شے { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

sequence_میل_شے<nodep>:  // ==IEEE: sequence_match_item
        //                      // IEEE says: operator_assignment
        //                      // IEEE says: inc_or_dec_expression
        //                      // IEEE says: subroutine_call
        //                      // This is the same list as...
                for_قدم_assignment                     { $$ = $1; }
        ;

//      boolean_abbrev -- all forms now handled directly in sexpr rule:
//                      // IEEE: consecutive_repetition -- [*N], [*N:M], [+], [*] via AstSConsRep
//                      // IEEE: goto_repetition -- [->N], [->M:N] via AstSGotoRep
//                      // IEEE: nonconsecutive_repetition -- [=N], [=M:N] via AstSNonConsRep

//************************************************
// Covergroup

covergroup_declaration<nodep>:  // ==IEEE: covergroup_declaration
                 yCOVERGROUP idAny cg_پورٹ_List_E coverage_eventE ';'
        /*cont*/    coverage_spec_or_optionListE
        /*cont*/ yENDGROUP سرا_Label_E
                        { Ast_Sen_درخت* clockp = nullptr;
                          Ast_گرہ* sampleArgsp = nullptr;
                          إذا ($4) {
                              إذا (VN_IS($4, Sen_شے)) {
                                  clockp = new Ast_Sen_درخت{$<fl>1, VN_AS($4, Sen_شے)};
                              } وإلا {
                                  sampleArgsp = $4;
                              }
                          }
                          $$ = new AstCovergroup{$<fl>1, *$2, static_cast<AstVar*>($3),
                                                 static_cast<AstVar*>(sampleArgsp), $6, clockp};
                          // Every covergroup has option/type_option members (added by V3LinkParse)
                          // referencing std:: types, so mark std as needed at parse time.
                          v3Global.سیٹ_ورتدا_Std_Package();
                          GRAMMARP->سرا_Label($<fl>8, $$, $8); }
        |        yCOVERGROUP yEXTENDS idAny ';'
        /*cont*/     coverage_spec_or_optionListE
        /*cont*/ yENDGROUP سرا_Label_E
                        { $$ = nullptr;
                          BBUNSUP($1, "Unsupported: covergroup inheritance (extends)");
                          DEL($5); }
        ;

cg_پورٹ_List_E<nodep>:
                /*empty*/                               { $$ = nullptr; }
        |       '(' tf_پورٹ_list_E ')'                   { $$ = $2; }
        ;

cgexpr<گرہ_Exprp>:  // IEEE-2012: covergroup_expression, before that just expression
                اظہاریہ                                    { $$ = $1; }
        ;

coverage_spec_or_optionListE<nodep>:  // IEEE: [{coverage_spec_or_option}]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       coverage_spec_or_optionList             { $$ = $1; }
        ;

coverage_spec_or_optionList<nodep>:  // IEEE: {coverage_spec_or_option}
                coverage_spec_or_option                 { $$ = $1; }
        |       coverage_spec_or_optionList coverage_spec_or_option     { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

coverage_spec_or_option<nodep>:  // ==IEEE: coverage_spec_or_option
        //                      // IEEE: coverage_spec
                cover_نقطہ                             { $$ = $1; }
        |       cover_cross                             { $$ = $1; }
        |       coverage_option ';'                     { $$ = $1; }
        |       error                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

coverage_option<nodep>:  // ==IEEE: coverage_option
        //                      // option/type_option aren't really keywords
                id/*yOPTION | yTYPE_OPTION*/ '.' idAny/*member_identifier*/ '=' اظہاریہ
                        { $$ = nullptr;
                          إذا (*$1 == "option" || *$1 == "type_option") {
                              ثابت bool typeOpt = (*$1 == "type_option");
                              VCoverOptionType optType = VCoverOptionType::UNKNOWN;
                              bool valid = true;
                              إذا (!typeOpt) {
                                  // IEEE 1800-2023 Table 19-1 option.* names and ignored legacy names
                                  إذا      (*$3 == "at_least")                optType = VCoverOptionType::AT_گھٹ_توں_گھٹ;
                                  وإلا إذا (*$3 == "auto_bin_max")            optType = VCoverOptionType::AUTO_BIN_MAX;
                                  وإلا إذا (*$3 == "comment")                 optType = VCoverOptionType::COMMENT;
                                  وإلا إذا (*$3 == "cross_auto_bin_max")      optType = VCoverOptionType::CROSS_AUTO_BIN_MAX;
                                  وإلا إذا (*$3 == "cross_num_print_missing") optType = VCoverOptionType::CROSS_عدد_PRINT_MISSING;
                                  وإلا إذا (*$3 == "cross_retain_auto_bins")  optType = VCoverOptionType::CROSS_RETAIN_AUTO_BINS;
                                  وإلا إذا (*$3 == "detect_overlap")          optType = VCoverOptionType::DETECT_OVERLAP;
                                  وإلا إذا (*$3 == "get_inst_coverage")       optType = VCoverOptionType::GET_INST_COVERAGE;
                                  وإلا إذا (*$3 == "goal")                    optType = VCoverOptionType::GOAL;
                                  وإلا إذا (*$3 == "name")                    optType = VCoverOptionType::NAME;
                                  وإلا إذا (*$3 == "per_instance")            optType = VCoverOptionType::فی_INSTANCE;
                                  وإلا إذا (*$3 == "weight")                  optType = VCoverOptionType::بھار;
                                  وإلا {
                                      $<fl>1->v3error("Unknown coverage option name 'option."
                                                      << *$3 << "'"
                                                      << "; not a valid option (IEEE 1800-2023 Table 19-1)");
                                      valid = false;
                                  }
                              } وإلا {
                                  // IEEE 1800-2023 Table 19-3: type_option.* names
                                  إذا      (*$3 == "comment")          optType = VCoverOptionType::COMMENT;
                                  وإلا إذا (*$3 == "distribute_first") optType = VCoverOptionType::DISTRIBUTE_پہلا;
                                  وإلا إذا (*$3 == "goal")             optType = VCoverOptionType::GOAL;
                                  وإلا إذا (*$3 == "merge_instances")  optType = VCoverOptionType::ملاؤ_INSTANCES;
                                  وإلا إذا (*$3 == "real_interval")    optType = VCoverOptionType::REAL_INTERVAL;
                                  وإلا إذا (*$3 == "strobe")           optType = VCoverOptionType::STROBE;
                                  وإلا إذا (*$3 == "weight")           optType = VCoverOptionType::بھار;
                                  وإلا {
                                      // Specific message for option.* names used under type_option.*
                                      إذا (*$3 == "at_least" || *$3 == "auto_bin_max"
                                          || *$3 == "cross_num_print_missing" || *$3 == "cross_retain_auto_bins"
                                          || *$3 == "detect_overlap" || *$3 == "get_inst_coverage"
                                          || *$3 == "name" || *$3 == "per_instance") {
                                          $<fl>1->v3error("'type_option." << *$3
                                              << "' is not valid; use 'option." << *$3 << "' instead");
                                      } وإلا {
                                          $<fl>1->v3error("Unknown coverage type option name 'type_option."
                                              << *$3 << "'"
                                              << "; not a valid type option (IEEE 1800-2023 Table 19-3)");
                                      }
                                      valid = false;
                                  }
                              }
                              إذا (valid) {
                                  $$ = new Ast_Cg_Option_سونپو{$<fl>1, typeOpt, optType, *$3, $5};
                              } وإلا {
                                  DEL($5);
                              }
                          } وإلا {
                              $<fl>1->v3error("Syntax error; expected 'option' or 'type_option': '" << *$1 << "'");
                              DEL($5);
                          } }
        ;

cover_نقطہ<nodep>:  // ==IEEE: cover_point
        //              // [ [ data_type_or_implicit ] cover_point_identifier ':' ] yCOVERPOINT
                yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>1, "", $2, $3, $4}; }
        //                      // IEEE-2012: class_scope before an ID
        |       id/*cover_point_id*/ ':' yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>3, *$1, $4, $5, $6}; }
        //                      // data_type_or_implicit expansion
        |       انکڑے_type id/*cover_point_id*/ ':' yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7};
                          DEL($1); }
        |       yVAR انکڑے_type id/*cover_point_id*/ ':' yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       yVAR implicit_typeE id/*cover_point_id*/ ':' yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signingE rangeList id/*cover_point_id*/ ':' yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signing id/*cover_point_id*/ ':' yCOVERPOINT اظہاریہ iffE bins_or_خالی_تھاواں
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7}; }
        //                      // IEEE-2012:
        |       bins_or_خالی_تھاواں                           { $$ = $1; }
        ;

iffE<گرہ_Exprp>:  // IEEE: part of cover_point, others
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       yIFF '(' اظہاریہ ')'
                        { $$ = $3; /* Keep iff condition for coverpoint */ }
        ;

bins_or_خالی_تھاواں<nodep>:  // ==IEEE: bins_or_empty
                '{' bins_or_optionsList '}'             { $$ = $2; }
        |       '{' '}'                                 { $$ = nullptr; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' bins_or_optionsList error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

bins_or_optionsList<nodep>:  // IEEE: { bins_or_options ';' }
                bins_or_options ';'                     { $$ = $1; }
        |       bins_or_optionsList bins_or_options ';' { $$ = جوڑو_اگلا_Null($1, $2); }
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
                          إذا (*$2 == "auto") {
                              $$ = new AstCoverBin{$<fl>2, *$2, $4};
                              DEL($6);
                          } وإلا {
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
                        { ثابت bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_سیٹ*>($5), VCoverBinsType{VCoverBinsType::BINS_TRANSITION}, isArray};
                          إذا ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { ثابت bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_سیٹ*>($5), VCoverBinsType{VCoverBinsType::BINS_IGNORE}, isArray};
                          إذا ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { ثابت bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_سیٹ*>($5), VCoverBinsType{VCoverBinsType::BINS_ILLEGAL}, isArray};
                          إذا ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
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
                          إذا ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_IGNORE};
                          إذا ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_ILLEGAL};
                          إذا ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        ;

bins_orBraE<گرہ_Exprp>:  // IEEE: part of bins_or_options: AstUnbounded for '[]', else the size
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       '[' ']'                                 { $$ = new AstUnbounded{$<fl>1}; }
        |       '[' cgexpr ']'
                        { $$ = $2;
                          إذا (VN_IS($2, Unbounded)) $2->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)"); }
        ;

trans_list<nodep>:  // ==IEEE: trans_list
                '(' trans_سیٹ ')'                       { $$ = $2; }
        |       trans_list ',' '(' trans_سیٹ ')'        { $$ = جوڑو_اگلا_Null($1, $4); }
        ;

trans_سیٹ<nodep>:  // ==IEEE: trans_set (returns AstCoverTransSet)
                trans_range_list                        {
                        Ast_Cover_Trans_شے* ثابت itemp = static_cast<Ast_Cover_Trans_شے*>($1);
                        $$ = new Ast_Cover_Trans_سیٹ{$<fl>1, itemp};
                }
        |       trans_سیٹ yP_EQGT trans_range_list
                {
                        Ast_Cover_Trans_سیٹ* ثابت setp = static_cast<Ast_Cover_Trans_سیٹ*>($1);
                        Ast_Cover_Trans_شے* ثابت itemp = static_cast<Ast_Cover_Trans_شے*>($3);
                        setp->جوڑو_Itemsp(itemp);
                        $$ = setp;
                }
        ;

trans_range_list<nodep>:  // ==IEEE: trans_range_list (returns AstCoverTransItem)
                trans_شے                              {
                        // Simple transition item without repetition
                        $$ = new Ast_Cover_Trans_شے{$<fl>1, $1, VTransRepType::NONE};
                }
        |       trans_شے yP_BRASTAR cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3); }
        |       trans_شے yP_BRASTAR cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3, $5); }
        |       trans_شے yP_BRAMINUSGT cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($3);
                          $$ = new Ast_Cover_Trans_شے{$<fl>1, $1, VTransRepType::GOTO}; }
        |       trans_شے yP_BRAMINUSGT cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($1, $3, $5); }
        |       trans_شے yP_BRAEQ cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($3);
                          $$ = new Ast_Cover_Trans_شے{$<fl>1, $1, VTransRepType::NONCONS}; }
        |       trans_شے yP_BRAEQ cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($1, $3, $5); }
        ;

trans_شے<nodep>:  // ==IEEE: range_list (returns range list node)
                covergroup_range_list                   { $$ = $1; }
        ;

covergroup_range_list<nodep>:  // ==IEEE: covergroup_range_list
                covergroup_قدر_range                  { $$ = $1; }
        |       covergroup_range_list ',' covergroup_قدر_range        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;


cover_cross<nodep>:  // ==IEEE: cover_cross
                id/*cover_point_identifier*/ ':' yCROSS list_دا_cross_اشیاء iffE cross_body
                        {
                          AstCoverCross* ثابت nodep = new AstCoverCross{$<fl>3, *$1,
                                                          VN_AS($4, CoverpointRef), $5};
                          إذا ($6) nodep->جوڑو_Binsp($6);
                          $$ = nodep;
                        }
        |       yCROSS list_دا_cross_اشیاء iffE cross_body
                        {
                          AstCoverCross* ثابت nodep = new AstCoverCross{$<fl>1,
                                                          "__cross" + cvt_منزل_Str(GRAMMARP->s_type_Imp_عدد++),
                                                          VN_AS($2, CoverpointRef), $3};
                          إذا ($4) nodep->جوڑو_Binsp($4);
                          $$ = nodep;
                        }
        ;

list_دا_cross_اشیاء<nodep>:  // ==IEEE: list_of_cross_items
                cross_شے ',' cross_شے               { $$ = جوڑو_اگلا_Null($1, $3); }
        |       cross_شے ',' cross_شے ',' cross_شے_List
                        { $$ = جوڑو_اگلا_Null(جوڑو_اگلا_Null($1, $3), $5); }
        ;

cross_شے_List<nodep>:  // IEEE: part of list_of_cross_items
                cross_شے                              { $$ = $1; }
        |       cross_شے_List ',' cross_شے           { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

cross_شے<nodep>:  // ==IEEE: cross_item
        //                      // IEEE: cover_point_identifier | variable_identifier - both are a
        //                      // simple identifier.  We parse idDotted (a plain hierarchical
        //                      // reference a.b.c, with no bit/array selects) to also accept the
        //                      // non-standard dotted form (e.g. 'cross a.b') that several
        //                      // simulators support; the common simple-identifier case is detected
        //                      // and handled exactly as before.
                idDotted
                        {
                          إذا (AstParseRef* ثابت refp = VN_CAST($1, ParseRef)) {
                              // Standard: simple cover_point_identifier / variable_identifier
                              $$ = new AstCoverpointRef{refp->fileline(), refp->name()};
                              VL_DO_DANGLING(refp->delete_درخت(), refp);
                          } وإلا {
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
        |       '{' cross_body_شے_List '}'             { $$ = $2; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' cross_body_شے_List error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

cross_body_شے_List<nodep>:  // IEEE: part of cross_body
                cross_body_شے                         { $$ = $1; }
        |       cross_body_شے_List cross_body_شے     { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

cross_body_شے<nodep>:  // ==IEEE: cross_body_item
                function_declaration
                        { $$ = nullptr; BBCOVERIGN($1->fileline(), "Unsupported: 'function' in coverage cross body"); DEL($1); }
        //                      // IEEE: bins_selection_or_option
        |       coverage_option ';'                     { $$ = $1; }
        //                      // IEEE: bins_selection
        |       yBINS idAny/*new-bin_identifier*/ '=' select_عبارت iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5}; }
        |       yIGNORE_BINS idAny/*new-bin_identifier*/ '=' select_عبارت iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_IGNORE}; }
        |       yILLEGAL_BINS idAny/*new-bin_identifier*/ '=' select_عبارت iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_ILLEGAL}; }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

select_عبارت<nodep>:  // ==IEEE: select_expression
                select_عبارت_r
                        { $$ = $1; }
        //                      // IEEE: integer_covergroup_expression ::= covergroup_expression | '$'
        //                      // '$' is part of our expr.  'matches' has higher precedence than
        //                      // '&&' and '||', which end the count (IEEE 1800-2023 footnote 30).
        //                      // A count could end in a function call followed by its 'with', so
        //                      // a selection with a count is filtered further only in parentheses.
        |       select_عبارت_r yWITH__PAREN '(' cgexpr ')' yMATCHES اظہاریہ %prec yP_ANDAND
                        { $$ = new AstCoverCrossWith{$2, $1, $4, $7}; }
        //                      // IEEE: cross_set_expression yMATCHES integer_covergroup_expression
        |       idAny '(' list_دا_arguments_E ')' yMATCHES اظہاریہ %prec yP_ANDAND
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3, $6); }
        |       select_عبارت yP_ANDAND select_عبارت
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, false}; }
        |       select_عبارت yP_OROR   select_عبارت
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, true}; }
        ;

// This non-terminal exists to disambiguate select_expression and make "with" bind tighter
select_عبارت_r<nodep>:
        //                      // IEEE: select_condition expanded here
                yBINSOF '(' bins_عبارت ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}}; }
        |       '!' yBINSOF '(' bins_عبارت ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true}; }
        //                      // IEEE: covergroup_range_list has the same syntax as range_list
        |       yBINSOF '(' bins_عبارت ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}, false, $7}; }
        |       '!' yBINSOF '(' bins_عبارت ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true, $8}; }
        |       yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($3); }
        |       '!' yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($4); }
        |       select_عبارت_r yWITH__PAREN '(' cgexpr ')'
                        { $$ = new AstCoverCrossWith{$2, $1, $4, nullptr}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr    { }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP '!' yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr { }
        //
        |       '(' select_عبارت ')'                       { $$ = $2; }
        |       idAny/*cross_identifier*/                        { $$ = new AstCoverCrossRef{$<fl>1, *$1}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP  cgexpr  { $$ = nullptr; BBCOVERIGN($1, "Ignoring unsupported: coverage select expression"); }
        //
        //                      // IEEE: cross_set_expression [ yMATCHES integer_covergroup_expression ]
        //UNSUP  cgexpr    {..}
        //UNSUP                 // Below are all removed
        |       idAny '(' list_دا_arguments_E ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3); }
        //UNSUP                 // Above are all removed, replace with:
        ;

bins_عبارت<گرہ_Exprp>:  // ==IEEE: bins_expression
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
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       clocking_event
                        { $$ = $1; }  // Keep the clocking event for automatic sampling
        |       yWITH__ETC yFUNCTION idAny/*"sample"*/ '(' tf_پورٹ_list_E ')'
                        { إذا (*$3 != "sample") {
                            $<fl>3->v3error("Coverage sampling function must be named 'sample'"
                                            " (IEEE 1800-2023 19.8.1)");
                            $$ = nullptr;
                            DEL($5);
                          } وإلا {
                            $$ = $5;
                          }
                        }
        |       yP_ATAT '(' بلاک_event_عبارت ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: '@@' coverage event"); DEL($3); }
        ;

بلاک_event_عبارت<nodep>:  // ==IEEE: block_event_expression
                بلاک_event_عبارت_رکن              { $$ = nullptr; /*UNSUP @@*/ DEL($1); }
        |       بلاک_event_عبارت yOR بلاک_event_عبارت_رکن   { $$ = nullptr; /*UNSUP @@*/ DEL($1, $3);  }
        ;

بلاک_event_عبارت_رکن<nodep>:  // IEEE: part of block_event_expression
                yBEGIN درجہ_وار_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        |       y_سرا   درجہ_وار_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        ;

درجہ_وار_btf_identifier<nodep>:  // ==IEEE: hierarchical_btf_identifier
        //                      // hierarchical_tf_identifier + hierarchical_block_identifier
        //                      // method_identifier
                packageClassScopeE idAny                { $$ = nullptr; /*UNSUP*/ DEL($1); }
        ;

//**********************************************************************
// Randsequence

randsequence_statement<گرہ_Stmtp>:  // ==IEEE: randsequence_statement
                yRANDSEQUENCE '(' ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, "", $4};
                          v3Global.useRandSequence(true); }
        |       yRANDSEQUENCE '(' idAny/*rs_production_identifier*/ ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, *$3, $5};
                          v3Global.useRandSequence(true); }
        ;

rs_productionList<rSProdp>:  // IEEE: rs_production+
                rs_production                           { $$ = $1; }
        |       rs_productionList rs_production         { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

rs_production<rSProdp>:  // ==IEEE: rs_production
                rs_productionFront ':' rs_قاعدہ_List ';'
                        { $$ = $1; $1->جوڑو_Rulesp($3); }
        ;

rs_productionFront<rSProdp>:  // IEEE: part of rs_production
                rs_funcId/*rs_production_identifier*/   { $$ = $1; }
        |       rs_funcId '(' tf_پورٹ_list_E ')'         { $$ = $1; $$->جوڑو_Portsp($3); }
        ;

rs_funcId<rSProdp>:  // IEEE: part of rs_production
                /**/ rs_fId
                        { $$ = $1; }  // Note is void as default, not logic as default like functions
        |       signingE rangeList rs_fId
                        { $$ = $3;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::پورٹ, $3->name(), VFlag_بچہ_DType{},
                                               GRAMMARP->جوڑو_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)}); }
        |       signing rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::پورٹ, $2->name(), VFlag_بچہ_DType{},
                                               new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}}); }
        |       انکڑے_type rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::پورٹ, $2->name(), VFlag_بچہ_DType{},
                                               $1}); }
        |       yVOID rs_fId
                        { $$ = $2; }
        ;

rs_fId<rSProdp>:  // IEEE: part of rs_production
                id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstRSProd{$<fl>$, *$1, nullptr, nullptr}; }
        ;

rs_قاعدہ_List<rSRulep>:  // IEEE: rs_rule+ part of rs_production
                rs_قاعدہ                                 { $$ = $1; }
        |       rs_قاعدہ_List '|' rs_قاعدہ                 { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

rs_قاعدہ<rSRulep>:  // ==IEEE: rs_rule
                rs_production_list
                        { $$ = new AstRSRule{$1->fileline(), nullptr, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_بھار_specification
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_بھار_specification rs_کوڈ_بلاک
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, $4}; }
        ;

rs_production_list<rSProdListp>:  // ==IEEE: rs_production_list
                rs_prodList
                        { $$ = new AstRSProdList{CRELINE(), nullptr, $1}; }
        |       yRAND yJOIN rs_production_شے rs_production_شے_List
                        { $$ = new AstRSProdList{$1, new AstConst{$2, AstConst::RealDouble{}, 0.5}, $3};
                          $$->randJoin(true);
                          $$->جوڑو_Prodsp($4); }
        |       yRAND yJOIN '(' اظہاریہ ')' rs_production_شے rs_production_شے_List
                        { $$ = new AstRSProdList{$1, $4, $6};
                          $$->randJoin(true);
                          $$->جوڑو_Prodsp($7); }
        ;

rs_بھار_specification<گرہ_Exprp>:  // ==IEEE: rs_weight_specification
                intnumAsConst                           { $$ = $1; }
        |       idClassSel/*ps_identifier*/             { $$ = $1; }
        |       '(' اظہاریہ ')'                            { $$ = $2; }
        ;

rs_کوڈ_بلاک<nodep>:  // ==IEEE: rs_code_block
                '{' '}'                                 { $$ = nullptr; }
        |       '{' rs_کوڈ_بلاک_شے_List '}'           { $$ = $2; }
        ;

rs_کوڈ_بلاک_شے_List<nodep>:  // IEEE: part of rs_code_block
                rs_کوڈ_بلاک_شے                       { $$ = $1; }
        |       rs_کوڈ_بلاک_شے_List rs_کوڈ_بلاک_شے         { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

rs_کوڈ_بلاک_شے<nodep>:  // IEEE: part of rs_code_block
                انکڑے_declaration                        { $$ = $1; }
        |       stmt                                    { $$ = $1; }
        ;

rs_prodList<nodep>:  // IEEE: rs_prod+
                rs_prod                                 { $$ = $1; }
        |       rs_prodList rs_prod                     { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

rs_prod<nodep>:  // ==IEEE: rs_prod
                rs_production_شے                      { $$ = $1; }
        |       rs_کوڈ_بلاک                           { $$ = $1; }
        //                      // IEEE: rs_if_else
        |       yIF '(' اظہاریہ ')' rs_production_شے %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = new AstIf{$<fl>1, $3, $5, nullptr}; }
        |       yIF '(' اظہاریہ ')' rs_production_شے yELSE rs_production_شے
                        { $$ = new AstIf{$<fl>1, $3, $5, $7}; }
        //                      // IEEE: rs_repeat
        |       yREPEAT '(' اظہاریہ ')' rs_production_شے
                               { $$ = new AstRepeat{$<fl>1, $3, $5}; }
        //                      // IEEE: rs_case
        |       yCASE '(' اظہاریہ ')' rs_case_شے_List yENDCASE
                        { $$ = new AstCase{$<fl>1, VCaseType::CT_RANDSEQUENCE, $3, $5}; }
        ;

rs_production_شے_List<nodep>:  // IEEE: rs_production_item+
                rs_production_شے                         { $$ = $1; }
        |       rs_production_شے_List rs_production_شے  { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

rs_production_شے<nodep>:  // ==IEEE: rs_production_item
                idAny/*production_identifier*/
                        { $$ = new Ast_RSProd_شے{$<fl>1, *$1, nullptr}; }
        |       idAny/*production_identifier*/ '(' list_دا_arguments_E ')'
                        { $$ = new Ast_RSProd_شے{$<fl>1, *$1, $3}; }
        ;

rs_case_شے_List<caseItemp>:  // IEEE: rs_case_item+
                rs_case_شے                            { $$ = $1; }
        |       rs_case_شے_List rs_case_شے           { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

rs_case_شے<caseItemp>:  // ==IEEE: rs_case_item
                caseCondList ':' rs_production_شے ';'
                        { $$ = new Ast_Case_شے{$<fl>1, $1, $3}; }
        |       yDEFAULT rs_production_شے ';'
                        { $$ = new Ast_Case_شے{$<fl>1, nullptr, $2}; }
        |       yDEFAULT ':' rs_production_شے ';'
                        { $$ = new Ast_Case_شے{$<fl>1, nullptr, $3}; }
        ;

//**********************************************************************
// Checker

checker_declaration<گرہ_Modulep>:  // ==IEEE: part of checker_declaration
                checkerFront checker_پورٹ_list_E ';'
                        checker_or_generate_شے_List_E yENDCHECKER سرا_Label_E
                        { $$ = $1;
                          $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));
                          إذا ($2) $1->جوڑو_Stmtsp($2);
                          إذا ($4) $1->جوڑو_Stmtsp($4);
                          GRAMMARP->سرا_Label($<fl>6, $1, $6); }
        ;

checkerFront<گرہ_Modulep>:  // IEEE: part of checker_declaration
                yCHECKER idAny/*checker_identifier*/
                        { $$ = new AstModule{$<fl>2, *$2, PARSEP->libname(), AstModule::Checker{}};
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ویلا_آخری_Unit());
                          PARSEP->rootp()->timeprecision_ملاؤ($$->fileline(),
                                                              PARSEP->ویلا_آخری_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive()); }
        |       checkerFront sigAttrScope               { $$ = $1; }
        ;

checker_پورٹ_list_E<nodep>:  // IEEE: [ ( [ checker_port_list ] ) ]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '('
        /*mid*/         { VARRESET_LIST(پورٹ); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    checker_پورٹ_list ')'
                        { $$ = $3; }
        ;

checker_پورٹ_list<nodep>:  // ==IEEE: checker_port_list
                checker_پورٹ_شے                             { $$ = $1; }
        |       checker_پورٹ_list ',' checker_پورٹ_شے       { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

checker_پورٹ_شے<nodep>:  // IEEE: checker_port_item
                checker_پورٹ_شے_Front checker_پورٹ_شے_Assignment  { $$ = $2; }
        ;

checker_پورٹ_شے_Front:  // IEEE: part of checker_port_item
                checker_پورٹ_direction_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       checker_پورٹ_direction_E انکڑے_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_پورٹ = true; }
        |       checker_پورٹ_direction_E yVAR انکڑے_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_پورٹ = true; }
        |       checker_پورٹ_direction_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       checker_پورٹ_direction_E implicit_typeE
                        { VARDTYPE($2); }
        ;

checker_پورٹ_direction_E:  // IEEE: [ checker_port_direction ]
                /*خالی_تھاواں*/                             { VARIO(داخلہ); }
        |       y_داخلہ                                  { VARIO(داخلہ); }
        |       yOUTPUT                                 { VARIO(OUTPUT); }
        ;

checker_پورٹ_شے_Assignment<nodep>:  // IEEE: part of checker_port_direction
                id variable_dimensionListE
                        { $$ = new Ast_پورٹ{CRELINE(), PINNUMINC(), *$1};
                          $$->جوڑو_اگلا(VARDONEA($<fl>1, *$1, $2, nullptr)); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { $$ = new Ast_پورٹ{CRELINE(), PINNUMINC(), *$1};
                          $$->جوڑو_اگلا(VARDONEA($<fl>1, *$1, $2, $4));
                          BBUNSUP($3, "Unsupported: checker port variable default value"); }
        ;

checker_or_generate_شے_List_E<nodep>:  // IEEE: [{ checker_or_generate_itemList }]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       checker_or_generate_شے_List            { $$ = $1; }
        ;

checker_or_generate_شے_List<nodep>:  // IEEE: { checker_or_generate_itemList }
                checker_or_generate_شے                { $$ = $1; }
        |       checker_or_generate_شے_List checker_or_generate_شے   { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

checker_or_generate_شے<nodep>:  // ==IEEE: checker_or_generate_item
                checker_or_generate_شے_declaration    { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        //                      // IEEE: checker_construct
        |       always_construct                        { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       assertion_شے                          { $$ = $1; }
        |       continuous_سونپو                       { $$ = $1; }
        |       checker_generate_شے                   { $$ = $1; }
        ;

checker_or_generate_شے_declaration<nodep>:  // ==IEEE: checker_or_generate_item_declaration
                انکڑے_declaration                        { $$ = $1; }
        |       yRAND انکڑے_declaration
                        { $$ = $2; BBUNSUP($1, "Unsupported: checker rand"); }
        |       function_declaration                    { $$ = $1; }
        |       checker_declaration
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: recursive 'checker'"); DEL($1); }
        |       assertion_شے_declaration              { $$ = $1; }
        |       covergroup_declaration                  { $$ = $1; }
        //      // IEEE deprecated: overload_declaration
        |       genvar_declaration                      { $$ = $1; }
        |       clocking_declaration                    { $$ = $1; }
        |       modDefaultClocking                      { $$ = $1; }
        |       defaultDisable                          { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

checker_generate_شے<nodep>:  // ==IEEE: checker_generate_item
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
                classFront parameter_پورٹ_list_E classExtendsE classImplementsE ';'
        /*mid*/         { // Allow resolving types declared in base extends class
                          $1->hasParameterList($<flag>2);
                        }
        /*cont*/    class_شے_List_سرا سرا_Label_E
                        { $$ = $1; $1->جوڑو_Membersp($2);
                          $1->جوڑو_Extendsp($3);
                          $1->جوڑو_Extendsp($4);
                          $1->جوڑو_Membersp($7);
                          GRAMMARP->سرا_Label($<fl>8, $1, $8); }
        ;

classFront<classp>:             // IEEE: part of class_declaration
        //                      // IEEE 1800-2023: lifetimeE replaced with final_specifierE
                classVirtualE yCLASS final_specifierE lifetimeE idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$5, PARSEP->libname()};
                          $$->ادھار_Override($3);
                          $$->isVirtual($1);
                          v3Global.سیٹ_Has_Classes(); }
        //                      // IEEE: part of interface_class_declaration
        //                      // IEEE 1800-2023: lifetimeE removed
        |       yINTERFACE yCLASS idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$3, PARSEP->libname()};
                          $$->isInterfaceClass(true);
                          v3Global.سیٹ_Has_Classes(); }
        ;

classVirtualE<cbool>:
                /*خالی_تھاواں*/                             { $$ = false; }
        |       yVIRTUAL__CLASS                         { $$ = true; }
        ;

classExtendsE<classExtendsp>:           // IEEE: part of class_declaration
        //                      // The classExtendsE rule relies on classFront having the
        //                      // new class scope correct via classFront
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       yEXTENDS classExtendsList               { $$ = $2; }
        ;

classExtendsList<classExtendsp>:        // IEEE: part of class_declaration
                classExtendsOne                         { $$ = $1; }
        |       classExtendsList ',' classExtendsOne    { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

classExtendsOne<classExtendsp>:         // IEEE: part of class_declaration
                class_typeExtImpList
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_وچ_Implements}; }
        |       class_typeExtImpList '(' list_دا_arguments_E ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_وچ_Implements};
                          $$->جوڑو_Argsp($3); }
        //                      // IEEE-2023: Added: yEXTENDS class_type '(' yDEFAULT ')'
        |       class_typeExtImpList '(' yDEFAULT ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_وچ_Implements};
                          BBUNSUP($<fl>2, "Unsupported: 'extends' with 'default'"); }
        ;

classImplementsE<classExtendsp>:        // IEEE: part of class_declaration
        //                      // All 1800-2012
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       yIMPLEMENTS
        /*mid*/         { GRAMMARP->m_وچ_Implements = true; }
        /*cont*/    classImplementsList
                        { $$ = $3;
                          GRAMMARP->m_وچ_Implements = false; }
        ;

classImplementsList<classExtendsp>:     // IEEE: part of class_declaration
        //                      // All 1800-2012
                classExtendsOne                         { $$ = $1; }
        |       classImplementsList ',' classExtendsOne
                        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

class_typeExtImpList<گرہ_Exprp>:  // IEEE: class_type: "[package_scope] id [ parameter_value_assignment ]"
        //                      // but allow yaID__aTYPE for extends/implements
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                class_typeExtImpOne                     { $$ = $1; }
        |       class_typeExtImpList yP_COLONCOLON class_typeExtImpOne
                        { $$ = new AstDot{$<fl>1, true, $1, $3}; }
        ;

class_typeExtImpOne<گرہ_Exprp>:  // part of IEEE: class_type, where we either get a package_scope component or class
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
        //                      // Not listed in IEEE, but see bug627 any parameter type maybe a class
        //                      // If idAny below is a class, parameter_value is legal
        //                      // If idAny below is a package, parameter_value is not legal
        //                      // If idAny below is otherwise, not legal
                idAny
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_قدر_assignment_Class_E
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $3}; }
        |       idCC
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_قدر_assignment_Class_E
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

packageClassScopeNoId<گرہ_Exprp>:  // IEEE: [package_scope] not followed by yaID
                packageClassScope                       { $$ = $1; }
        ;

packageClassScopeE<گرہ_Exprp>:  // IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // TODO: To support classes should return generic type, not packagep
        //                      // class_qualifier := [ yLOCAL '::'  ] [ implicit_class_handle '.'  class_scope ]
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       packageClassScope                       { $$ = $1; }
        ;

packageClassScope<گرہ_Exprp>:   // IEEE: class_scope
        //                      // IEEE: "class_type yP_COLONCOLON"
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
                packageClassScopeList                   { $$ = $1; }
        |       local_اگلا_Id yP_COLONCOLON               { $$ = $1; }
        |       dollar_Unit_اگلا_Id yP_COLONCOLON          { $$ = $1; }
        |       dollar_Unit_اگلا_Id yP_COLONCOLON packageClassScopeList
                        { $$ = new AstDot{$2, true, $1, $3}; }
        ;

packageClassScopeList<گرہ_Exprp>:   // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      // Or IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                package_Class_Scope_شے                   { $$ = $1; }
        |       packageClassScopeList package_Class_Scope_شے
                        { $$ = new AstDot{$<fl>2, true, $1, $2}; }
        ;

package_Class_Scope_شے<گرہ_Exprp>:   // IEEE: package_scope or [package_scope]::[class_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      //vv mid rule action needed otherwise we might not have NextId in time to parse the id token
                idCC
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, nullptr}; }
        //
        |       idCC parameter_قدر_assignment_Class
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $2}; }
        ;

dollar_Unit_اگلا_Id<گرہ_Exprp>:    // $unit
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yD_UNIT
                        { $$ = new AstClassOrPackageRef{$1, "$unit", nullptr, nullptr}; }
        ;

local_اگلا_Id<گرہ_Exprp>:         // local::
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yLOCAL__COLONCOLON
                        { $$ = new AstClassOrPackageRef{$1, "local::", nullptr, nullptr}; }
        ;

//^^^=========

class_شے_List_سرا<nodep>:
                yENDCLASS                               { $$ = nullptr; }
        |       class_شے_List yENDCLASS                { $$ = $1; }
        |       error yENDCLASS                         { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       class_شے_List error yENDCLASS          { $$ = $1; }  // LCOV_EXCL_LINE
        ;

class_شے_List<nodep>:
                class_شے                              { $$ = $1; }
        |       class_شے_List class_شے               { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

class_شے<nodep>:                      // ==IEEE: class_item
                class_property                          { $$ = $1; }
        |       class_طریقہ_2                            { $$ = $1; }
        |       class_constraint                        { $$ = $1; }
        //
        |       class_declaration                       { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       covergroup_declaration
                        {
                          إذا ($1) {
                              // Also declares an instance variable, of the covergroup's type
                              AstVar* ثابت newp = new AstVar{$1->fileline(), VVarType::VAR,
                                  $1->name(), VFlag_بچہ_DType{},
                                  new AstRefDType($1->fileline(), $1->name())};
                              newp->embeddedCovergroup(true);
                              $$ = جوڑو_اگلا_Null($1, newp);
                          } وإلا {
                              $$ = nullptr;
                          }
                        }
        //                      // local_parameter_declaration under parameter_declaration
        |       parameter_declaration ';'               { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        //                      // Verilator specific
        |       vl_Sc_بلاک                               { $$ = $1; }
        //
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

class_طریقہ_2<nodep>:            // ==IEEE: class_method
                memberQualListE task_declaration        { $$ = $2; $1.apply_منزل_Nodes($2); }
        |       memberQualListE function_declaration    { $$ = $2; $1.apply_منزل_Nodes($2); }
        |       y_خالص yVIRTUAL__ETC memberQualListE طریقہ_prototype ';'
                        { $$ = $4; $3.apply_منزل_Nodes($4); $4->خالص_Virtual(true); $4->isVirtual(true); }
        |       yEXTERN memberQualListE طریقہ_prototype ';'
                        { $$ = $3; $2.apply_منزل_Nodes($3); $3->isExternProto(true); }
        //                      // IEEE: "method_qualifierE class_constructor_declaration"
        //                      // part of function_declaration
        |       yEXTERN memberQualListE class_constructor_prototype
                        { $$ = $3; $2.apply_منزل_Nodes($3); $3->isExternProto(true); }
        ;

dynamic_override_specifiersE<ادھار_Override>:  // IEEE: dynamic_override_specifiers or empty
                /*خالی_تھاواں*/                             { $$ = VBaseOverride{}; }
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

final_specifierE<ادھار_Override>:  // ==IEEE: final_specifier (similar to dynamic_override_specifiers)
                /*خالی_تھاواں*/                             { $$ = VBaseOverride{}; }
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
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew constraint_بلاک
                        { $$ = $4; $$->isStatic($1); $$->ادھار_Override($3); $$->جوڑو_Itemsp($5); }
        //                      // IEEE: constraint_prototype + constraint_prototype_qualifier
        |       constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $4; $$->isStatic($1); $$->ادھار_Override($3);
                          $$->isExternProto(true); }
        |       yEXTERN constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $5; $$->isStatic($2); $$->ادھار_Override($4);
                          $$->isExternProto(true); $$->isExternExplicit(true); }
        |       y_خالص constraintStaticE yCONSTRAINT constraintIdNew ';'
                        { $$ = $4; $$->is_Kwd_خالص($1); $$->isStatic($2); }
        ;

constraintIdNew<constraintp>:  // IEEE: id part of class_constraint
                idAny/*constraint_identifier*/
                        { $$ = new AstConstraint{$<fl>1, *$1, nullptr}; }
        ;

constraint_بلاک<nodep>:  // ==IEEE: constraint_block
                '{' '}'                                         { $$ = nullptr; }
        |       '{' constraint_بلاک_شے_List '}'               { $$ = $2; }
        //
        |       '{' error '}'                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_بلاک_شے_List error '}'         { $$ = $2; }  // LCOV_EXCL_LINE
        ;

constraint_بلاک_شے_List<nodep>:  // IEEE: { constraint_block_item }
                constraint_بلاک_شے                           { $$ = $1; }
        |       constraint_بلاک_شے_List constraint_بلاک_شے { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

constraint_بلاک_شے<nodep>:  // ==IEEE: constraint_block_item
                constraint_عبارت                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        ;

solve_before_list<گرہ_Exprp>:  // ==IEEE: solve_before_list
                constraint_primary                              { $$ = $1; }
        |       solve_before_list ',' constraint_primary        { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

constraint_primary<گرہ_Exprp>:  // ==IEEE: constraint_primary
        //                      // exprScope more general than:
        //                      // [ implicit_class_handle '.' | class_scope ] hierarchical_identifier select [ '(' ')' ]
                اظہاریہ_Scope                                       { $$ = $1; }
        ;

constraint_عبارت_List<nodep>:  // ==IEEE: { constraint_expression }
                constraint_عبارت                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        |       constraint_عبارت_List constraint_عبارت { $$ = جوڑو_اگلا_Null($1, $2); }
        |       constraint_عبارت_List ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = جوڑو_اگلا_Null($1, new AstConstraintBefore{$<fl>2, $3, $5}); }
        ;

constraint_عبارت<nodep>:  // ==IEEE: constraint_expression
                اظہاریہ/*expression_or_dist*/ ';'
                        { $$ = new Ast_Constraint_اظہاریہ{$1->fileline(), $1}; }
        //                      // 1800-2012:
        |       ySOFT اظہاریہ/*expression_or_dist*/ ';'
                        { Ast_Constraint_اظہاریہ* ثابت newp = new Ast_Constraint_اظہاریہ{$1, $2};
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
        |       اظہاریہ yP_MINUSGT '{' constraint_عبارت_List '}'
                        { $$ = new AstConstraintIf{$2, $1, $4, nullptr}; }
        |       اظہاریہ yP_MINUSGT yIF '(' اظہاریہ ')' constraint_سیٹ %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, nullptr}, nullptr}; }
        |       اظہاریہ yP_MINUSGT yIF '(' اظہاریہ ')' constraint_سیٹ yELSE constraint_سیٹ
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, $9}, nullptr}; }
        |       اظہاریہ yP_MINUSGT yFOREACH '(' idClassSelForeach ')' constraint_سیٹ
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintForeach{$3, $5, $7}, nullptr}; }
        |       اظہاریہ yP_MINUSGT yUNIQUE '{' range_list '}' ';'
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintUnique{$3, $5}, nullptr}; }
        |       اظہاریہ yP_MINUSGT ySOFT اظہاریہ ';'
                        { Ast_Constraint_اظہاریہ* ثابت innerp = new Ast_Constraint_اظہاریہ{$3, $4};
                          innerp->isSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       اظہاریہ yP_MINUSGT yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_اظہاریہ* ثابت innerp = new Ast_Constraint_اظہاریہ{$3, $5};
                          innerp->isDisableSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       yIF '(' اظہاریہ ')' constraint_سیٹ %prec pr_ہیٹھلا_THAN_ELSE
                        { $$ = new AstConstraintIf{$1, $3, $5, nullptr}; }
        |       yIF '(' اظہاریہ ')' constraint_سیٹ yELSE constraint_سیٹ
                        { $$ = new AstConstraintIf{$1, $3, $5, $7}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM + 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' constraint_سیٹ
                        { $$ = new AstConstraintForeach{$1, $3, $5}; }
        //                      // Non-IEEE extension, soft foreach
        |       ySOFT yFOREACH '(' idClassSelForeach ')' constraint_سیٹ
                        { $$ = new AstConstraintForeach{$2, $4, $6, true}; }
        //                      // soft is 1800-2012
        |       yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_اظہاریہ* ثابت newp = new Ast_Constraint_اظہاریہ{$1, $3};
                          newp->isDisableSoft(true);
                          $$ = newp; }
        //
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

constraint_سیٹ<nodep>:  // ==IEEE: constraint_set
                constraint_عبارت                   { $$ = $1; }
        |       '{' constraint_عبارت_List '}'       { $$ = $2; }
        //
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_عبارت_List error '}' { $$ = $2; }  // LCOV_EXCL_LINE
        ;

dist_list<distItemp>:  // ==IEEE: dist_list
                dist_شے                               { $$ = $1; }
        |       dist_list ',' dist_شے                 { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

dist_شے<distItemp>:  // ==IEEE: dist_item + dist_weight
                قدر_range
                        { $$ = new Ast_Dist_شے{$1->fileline(), $1, new AstConst{$1->fileline(), 1}}; }
        |       قدر_range yP_COLONEQ  اظہاریہ
                        { $$ = new Ast_Dist_شے{$2, $1, $3}; }
        |       قدر_range yP_COLONDIV اظہاریہ
                        { $$ = new Ast_Dist_شے{$2, $1, $3}; $$->is_سارا(true); }
        //                      // IEEE 1800-2023 added:
        |       yDEFAULT yP_COLONDIV اظہاریہ
                        { BBUNSUP($<fl>2, "Unsupported: 'default :/' constraint");
                          $$ = nullptr; DEL($3); }
        ;

extern_constraint_declaration<constraintp>:  // ==IEEE: extern_constraint_declaration
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE packageClassScopeE constraintIdNew constraint_بلاک
                        { $$ = $5; $$->isStatic($1); $$->isExternDef(true);
                          $$->ادھار_Override($3); $$->classOrPackagep($4); $$->جوڑو_Itemsp($6); }
        ;

constraintStaticE<cbool>:  // IEEE: part of extern_constraint_declaration
                /*خالی_تھاواں*/                             { $$ = false; }
        |       ySTATIC__CONSTRAINT                     { $$ = true; }
        ;

//**********************************************************************
// Constants

ویلا_عدد_Adjusted<گرہ_Exprp>:         // Time constant, adjusted to module's time units/precision
                yaTIMENUM
                        { $$ = new Ast_ویلا_Import{$<fl>1, new AstConst{$<fl>1, AstConst::RealDouble{}, $1}}; }
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
        /*cont*/    configParameterListE design_statement config_قاعدہ_statement_List_E
        /*cont*/    yENDCONFIG سرا_Label_E
                { AstConfig* ثابت newp = new AstConfig{$1, PARSEP->libname(), *$2};
                  newp->جوڑو_Designp($5);
                  newp->جوڑو_Itemsp($4);
                  newp->جوڑو_Itemsp($6);
                  GRAMMARP->سرا_Label($<fl>7, *$2, $8);
                  PARSEP->rootp()->جوڑو_Miscsp(newp); }
        ;

configParameterListE<nodep>:  // IEEE: { local_parameter_declaration ';' }
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       configParameterList                     { $$ = $1; }
        ;

configParameterList<nodep>:  // IEEE: part of config_declaration
                configParameter                         { $$ = nullptr; DEL($1); }
        |       configParameterList configParameter     { $$ = جوڑو_اگلا_Null($1, $2); }
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
        |       configCellList configCell               { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

configCell<configCellp>:  // IEEE: part of design_statement, part of cell_clause
                idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, PARSEP->libname(), *$1}; }
        |       idAny/*library_identifier*/ '.' idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, *$1, *$3}; }
        ;

config_قاعدہ_statement_List_E<nodep>:  // IEEE: { config_rule }
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        |       config_قاعدہ_statement_List               { $$ = $1; }
        ;

config_قاعدہ_statement_List<nodep>:  // IEEE: { config_rule }
                config_قاعدہ_statement                   { $$ = $1; }
        |       config_قاعدہ_statement_List config_قاعدہ_statement   { $$ = جوڑو_اگلا_Null($1, $2); }
        ;

config_قاعدہ_statement<nodep>:  // == IEEE: config_rule_statement
        //                      // IEEE: default_clause
                yDEFAULT liblist_clause ';'
                        { $$ = new Ast_Config_قاعدہ{$1, nullptr, $2, false}; }
        //                      // IEEE: inst_clause
        |       yINSTANCE inst_name liblist_clause ';'
                        { $$ = new Ast_Config_قاعدہ{$1, $2, $3, false}; }
        |       yINSTANCE inst_name use_clause ';'
                        { $$ = new Ast_Config_قاعدہ{$1, $2, $3, false}; }
        //                      // IEEE: cell_clause
        |       yCELL configCell liblist_clause ';'
                        { $$ = new Ast_Config_قاعدہ{$1, $2, $3, true}; }
        |       yCELL configCell use_clause ';'
                        { $$ = new Ast_Config_قاعدہ{$1, $2, $3, true}; }
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

inst_name<گرہ_Exprp>:  // == IEEE: inst_name
                idAnyAsParseRef/*topmodule_identifier*/
                        { $$ = $1; }
        |       idAnyAsParseRef/*topmodule_identifier*/ inst_nameInstanceList
                        { $$ = new AstDot{$<fl>1, false, $1, $2}; }
        ;

inst_nameInstanceList<گرہ_Exprp>:  // IEEE: part of inst_name
                '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = $2; }
        |       inst_nameInstanceList '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        ;

liblist_clause<nodep>:  // == IEEE: liblist_clause
                yLIBLIST                                { $$ = nullptr; }
        |       yLIBLIST liblistLibraryList             { $$ = $2; }
        ;

liblistLibraryList<گرہ_Exprp>:  // IEEE: part of liblist_clause
                idAnyAsParseRef/*library_identifier*/
                        { $$ = $1; }
        |       liblistLibraryList idAnyAsParseRef/*library_identifier*/
                        { $$ = جوڑو_اگلا_Null($1, $2); }
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
                /*خالی_تھاواں*/                             { $$ = nullptr; }
        //                      // IEEE is missing the '#' '(', but examples need it
        |       '#' '(' ')'                             { $$ = nullptr; }
        |       '#' '(' useAssignmentList ')'           { $$ = $3; }
        ;

useAssignmentList<pinp>:  // IEEE: part of use_clause
                useAssignment                           { $$ = $1; }
        |       useAssignmentList ',' useAssignment     { $$ = جوڑو_اگلا_Null($1, $3); }
        ;

useAssignment<pinp>:  // IEEE: part of use_clause
        //                      // IEEE: named_parameter_assignment
                '.' idAny/*parameter_identifier*/ '(' ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); }
        |       '.' idAny/*parameter_identifier*/ '(' اظہاریہ_Or_انکڑے_Type ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); DEL($4); }
        ;

colonConfigE<cbool>:  // IEEE: [ ':' yCONFIG]
                /*خالی_تھاواں*/                             { $$ = false; }
        |       ':' yCONFIG                             { $$ = true; }
        ;

//**********************************************************************
// Config - lib.map
//

library_declaration<nodep>:  // IEEE: library_declaration
                yLIBRARY yaSTRING file_رستہ_spec_List incdirE ';'
                        { $$ = new AstLibrary{$<fl>1, *$2, $3, $4}; }
        ;
incdirE<گرہ_Exprp>:  // IEEE: [ '-' yINCDIR file_path_specList ';']
                /*خالی_تھاواں*/                             { $$ = nullptr; }
                                // https://accellera.mantishub.io/view.php?id=1166
        |       yINCDIR file_رستہ_spec_List              { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config incdir"); }
        ;
include_statement<nodep>:  // IEEE: include_statement
                yINCLUDE file_رستہ_spec ';'             { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config include"); }
        ;
file_رستہ_spec_List<گرہ_Exprp>:  // IEEE: file_path_spec { ',' file_path_spec }
                file_رستہ_spec                          { $$ = $1; }
        |       file_رستہ_spec_List ',' file_رستہ_spec   { $$ = جوڑو_اگلا_Null($1, $3); }
        ;
file_رستہ_spec<گرہ_Exprp>:  // IEEE: file_path_spec
                yaSTRING { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

//**********************************************************************
// VLT Files

vlt_شے:
        //                      // TODO support arbitrary order of arguments
                vltOffFront
                        { V3Control::جوڑو_Ignore($1, false, "*", 0, 0); }
        |       vltOffFront vltDFile
                        { V3Control::جوڑو_Ignore($1, false, *$2, 0, 0); }
        |       vltOffFront vltDFile y_VLT_D_سطریں yaINTNUM
                        { V3Control::جوڑو_Ignore($1, false, *$2, $4->منزل_UInt(), $4->منزل_UInt()); }
        |       vltOffFront vltDFile y_VLT_D_سطریں yaINTNUM '-' yaINTNUM
                        { V3Control::جوڑو_Ignore($1, false, *$2, $4->منزل_UInt(), $6->منزل_UInt()); }
        |       vltOffFront vltDFile vltDMatch
                        { إذا (($1 == V3_Error_کوڈ::I_COVERAGE) || ($1 == V3_Error_کوڈ::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } وإلا {
                              V3Control::جوڑو_Ignore_میل($1, *$2, "", *$3);
                          }}
        |       vltOffFront vltDFile vltDContents
                        { إذا (($1 == V3_Error_کوڈ::I_COVERAGE) || ($1 == V3_Error_کوڈ::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } وإلا {
                              V3Control::جوڑو_Ignore_میل($1, *$2, *$3, "*");
                          }}
        |       vltOffFront vltDFile vltDContents vltDMatch
                        { إذا (($1 == V3_Error_کوڈ::I_COVERAGE) || ($1 == V3_Error_کوڈ::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } وإلا {
                              V3Control::جوڑو_Ignore_میل($1, *$2, *$3, *$4);
                          }}
        |       vltOffFront vltDScope
                        { إذا ($1 != V3_Error_کوڈ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } وإلا {
                              V3Control::جوڑو_Scope_Trace_On(false, *$2, 0);
                          }}
        |       vltOffFront vltDScope vltDLevels
                        { إذا ($1 != V3_Error_کوڈ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } وإلا {
                              V3Control::جوڑو_Scope_Trace_On(false, *$2, $3->منزل_UInt());
                          }}
        |       vltOnFront
                        { V3Control::جوڑو_Ignore($1, true, "*", 0, 0); }
        |       vltOnFront vltDFile
                        { V3Control::جوڑو_Ignore($1, true, *$2, 0, 0); }
        |       vltOnFront vltDFile y_VLT_D_سطریں yaINTNUM
                        { V3Control::جوڑو_Ignore($1, true, *$2, $4->منزل_UInt(), $4->منزل_UInt()); }
        |       vltOnFront vltDFile y_VLT_D_سطریں yaINTNUM '-' yaINTNUM
                        { V3Control::جوڑو_Ignore($1, true, *$2, $4->منزل_UInt(), $6->منزل_UInt()); }
        |       vltOnFront vltDScope
                        { إذا ($1 != V3_Error_کوڈ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } وإلا {
                              V3Control::جوڑو_Scope_Trace_On(true, *$2, 0);
                          }}
        |       vltOnFront vltDScope vltDLevels
                        { إذا ($1 != V3_Error_کوڈ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } وإلا {
                              V3Control::جوڑو_Scope_Trace_On(true, *$2, $3->منزل_UInt());
                          }}
        |       vltVarAttrFront vltDModuleE vltDFTaskE vltVarAttrSpecE attr_event_controlE
                        { V3Control::جوڑو_Var_Attr($<fl>1, *$2, *$3, GRAMMARP->m_vlt_Var_Spec_قسم, *$4, $1, $5); }
        |       vltVarAttrFrontDeprecated vltDModuleE vltDFTaskE vltVarAttrSpecE
                        { /* Historical, now has no effect */ }
        |       vltInlineFront vltDModuleE vltDFTaskE
                        { V3Control::جوڑو_Inline($<fl>1, *$2, *$3, $1); }
        |       y_VLT_COVERAGE_بلاک_OFF vltDFile
                        { V3Control::جوڑو_Coverage_بلاک_Off(*$2, 0); }
        |       y_VLT_COVERAGE_بلاک_OFF vltDFile y_VLT_D_سطریں yaINTNUM
                        { V3Control::جوڑو_Coverage_بلاک_Off(*$2, $4->منزل_UInt()); }
        |       y_VLT_COVERAGE_بلاک_OFF vltDModule vltDBlock
                        { V3Control::جوڑو_Coverage_بلاک_Off(*$2, *$3); }
        |       y_VLT_بھریا_CASE vltDFile
                        { V3Control::جوڑو_Case_بھریا(*$2, 0); }
        |       y_VLT_بھریا_CASE vltDFile y_VLT_D_سطریں yaINTNUM
                        { V3Control::جوڑو_Case_بھریا(*$2, $4->منزل_UInt()); }
        |       y_VLT_HIER_بلاک vltDModuleE
                        { V3Control::جوڑو_Module_Pragma(*$2, VPragmaType::HIER_بلاک); }
        |       yVLT_HIER_PARAMS vltDModuleE
                        { V3Control::جوڑو_Module_Pragma(*$2, VPragmaType::HIER_PARAMS); }
        |       yVLT_HIER_WORKERS vltDModuleE vltDWorkers
                        { V3Control::جوڑو_Hier_Workers($<fl>1, *$2, $3->منزل_SInt()); }
        |       yVLT_HIER_WORKERS vltDHierDpi vltDWorkers
                        { V3Control::جوڑو_Hier_Workers($<fl>1, *$2, $3->منزل_SInt()); }
        |       yVLT_PARALLEL_CASE vltDFile
                        { V3Control::جوڑو_Case_Parallel(*$2, 0); }
        |       yVLT_PARALLEL_CASE vltDFile y_VLT_D_سطریں yaINTNUM
                        { V3Control::جوڑو_Case_Parallel(*$2, $4->منزل_UInt()); }
        |       y_VLT_PROFILE_انکڑے vltDHierDpi vltDCost
                        { V3Control::جوڑو_Profile_انکڑے($<fl>1, *$2, $3->منزل_UQuad()); }
        |       y_VLT_PROFILE_انکڑے vltDModel vltDMtask vltDCost
                        { V3Control::جوڑو_Profile_انکڑے($<fl>1, *$2, *$3, $4->منزل_UQuad()); }
        |       y_VLT_FSM_رجسٹر_WRAPPER vltDModule vltDFsmD vltDFsmQ vltDFsmClock vltDFsmResetE vlt_DFsm_Reset_قدر_E
                        { V3Control::جوڑو_Fsm_رجسٹر_Wrapper($<fl>1, *$2, *$3, *$4, *$5, *$6, *$7); }
        |       yVLT_VERILATOR_LIB vltDModule
                        { V3Control::جوڑو_Module_Pragma(*$2, VPragmaType::VERILATOR_LIB); }
        ;

vltOffFront<errcodeen>:
                yVLT_COVERAGE_OFF                       { $$ = V3_Error_کوڈ::I_COVERAGE; }
        |       yVLT_TIMING_OFF                         { $$ = V3_Error_کوڈ::I_TIMING; }
        |       yVLT_TRACING_OFF                        { $$ = V3_Error_کوڈ::I_TRACING; }
        |       yVLT_LINT_OFF                           { $$ = V3_Error_کوڈ::I_LINT; }
        |       yVLT_LINT_OFF y_VLT_D_قاعدہ idAny
                        { $$ = V3_Error_کوڈ{*$3};
                          إذا ($$ == V3_Error_کوڈ::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltOnFront<errcodeen>:
                yVLT_COVERAGE_ON                        { $$ = V3_Error_کوڈ::I_COVERAGE; }
        |       yVLT_TIMING_ON                          { $$ = V3_Error_کوڈ::I_TIMING; }
        |       yVLT_TRACING_ON                         { $$ = V3_Error_کوڈ::I_TRACING; }
        |       yVLT_LINT_ON                            { $$ = V3_Error_کوڈ::I_LINT; }
        |       yVLT_LINT_ON y_VLT_D_قاعدہ idAny
                        { $$ = V3_Error_کوڈ{*$3};
                          إذا ($$ == V3_Error_کوڈ::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltDBlock<strp>:  // --block <arg>
                y_VLT_D_بلاک str                        { $$ = $2; }
        ;

vltDContents<strp>:
                yVLT_D_CONTENTS str                     { $$ = $2; }
        ;

vltDCost<nump>:  // --cost <arg>
                y_VLT_D_لاگت yaINTNUM                    { $$ = $2; }
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
                /*خالی_تھاواں*/                             { ساكن string خالی_تھاواں; $$ = &خالی_تھاواں; }
        |       yVLT_D_RESET str                        { $$ = $2; }
        ;

vlt_DFsm_Reset_قدر_E<strp>:  // [--reset_value <arg>]
                /*خالی_تھاواں*/                             { ساكن string خالی_تھاواں; $$ = &خالی_تھاواں; }
        |       y_VLT_D_RESET_قدر str                  { $$ = $2; }
        ;

vltDHierDpi<strp>:  // --hier-dpi <arg>
                yVLT_D_HIER_DPI str                     { $$ = $2; }
        ;

vltDLevels<nump>:  // --levels <arg>
                y_VLT_D_پدھراں yaINTNUM                  { $$ = $2; }
        ;

vltDMatch<strp>:  // --match <arg>
                y_VLT_D_میل str                        { $$ = $2; }
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
                /*خالی_تھاواں*/
                        { ساكن string unit = "$unit"; $$ = &unit; }  // .vlt uses prettyName
        |       vltDModule
                        { $$ = $1; }
        ;

vltDScope<strp>:  // --scope <arg>
                yVLT_D_SCOPE str                        { $$ = $2; }
        ;

vltDFTaskE<strp>:
                /*خالی_تھاواں*/                             { ساكن string خالی_تھاواں; $$ = &خالی_تھاواں; }
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
                /*خالی_تھاواں*/
                        { GRAMMARP->m_vlt_Var_Spec_قسم = V3Control::Var_Spec_قسم::VAR; ساكن std::string خالی_تھاواں; $$ = &خالی_تھاواں; }
        |       yVLT_D_PARAM str
                        { GRAMMARP->m_vlt_Var_Spec_قسم = V3Control::Var_Spec_قسم::PARAM; $$ = $2; }
        |       y_VLT_D_پورٹ str
                        { GRAMMARP->m_vlt_Var_Spec_قسم = V3Control::Var_Spec_قسم::پورٹ; $$ = $2; }
        |       yVLT_D_VAR str
                        { GRAMMARP->m_vlt_Var_Spec_قسم = V3Control::Var_Spec_قسم::VAR; $$ = $2; }
        ;

vltVarAttrFront<attrtypeen>:
                yVLT_FORCEABLE              { $$ = VAttrType::VAR_FORCEABLE; }
        |       y_VLT_عوامی                 { $$ = VAttrType::VAR_عوامی; v3Global.dpi(true); }
        |       y_VLT_عوامی_FLAT            { $$ = VAttrType::VAR_عوامی_FLAT; v3Global.dpi(true); }
        |       y_VLT_عوامی_FLAT_RD         { $$ = VAttrType::VAR_عوامی_FLAT_RD; v3Global.dpi(true); }
        |       y_VLT_عوامی_FLAT_RW         { $$ = VAttrType::VAR_عوامی_FLAT_RW; v3Global.dpi(true); }
        |       yVLT_SC_BIGUINT             { $$ = VAttrType::VAR_SC_BIGUINT; }
        |       yVLT_SC_BV                  { $$ = VAttrType::VAR_SC_BV; }
        |       yVLT_SFORMAT                { $$ = VAttrType::VAR_SFORMAT; }
        |       y_VLT_ونڈو_VAR              { $$ = VAttrType::VAR_ونڈو_VAR; }
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
