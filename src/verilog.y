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
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ NEVER_JUST_FOR_CLANG_FORMAT
 }
#ਜੇ_ਅੰਤ
// clang-format on
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ParseGrammar.h"  // Defines YYTYPE; before including bison header

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YYERROR_VERBOSE 1  // For prior to Bison 3.6
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YYINITDEPTH 10000  // Older bisons ignore YYMAXDEPTH
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YYMAXDEPTH 10000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YYNEWLINE "\n"

// Pick up new lexer
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ yylex PARSEP->token_ਮੰਜ਼ਿਲ_Bison

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ BBCOVERIGN(fl, ਸੁਨੇਹਾ_2) (fl)->v3warn(COVERIGN, ਸੁਨੇਹਾ_2)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ BBUNSUP(fl, ਸੁਨੇਹਾ_2) (fl)->v3warn(E_UNSUPPORTED, ਸੁਨੇਹਾ_2)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ GATEUNSUP(fl, tok) \
    { BBUNSUP((fl), "Unsupported: Verilog 1995 gate primitive: " << (tok)); }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ RISEFALLDLYUNSUP(nodep) \
    ਜੇ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_ਸੈੱਟ_True()) { \
        nodep->v3warn(RISEFALLDLY, "Unsupported: turn-off delays. Ignoring the third delay"); \
    }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ MINTYPMAXDLYUNSUP(nodep) \
    ਜੇ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_ਸੈੱਟ_True()) { \
        nodep->v3warn( \
            MINTYPMAXDLY, \
            "Unsupported: minimum/typical/maximum delay expressions. Using the typical delay"); \
    }
// Apply a delay to all continuous assignments under listp
ਸਥਿਰ ਖਾਲੀ DELAY_LIST(Ast_ਗੰਢ* listp, AstDelay* delayp) {
    ਜੇ (!delayp) ਵਾਪਸ;
    ਲਈ (Ast_ਗੰਢ* nodep = listp; nodep; nodep = nodep->nextp()) {
        ਜੇ (VN_IS(nodep, Implicit)) ਜਾਰੀ;
        AstAlways* ਅਚਲ alwaysp = VN_AS(nodep, Always);
        Ast_ਸੌਂਪੋ_W* ਅਚਲ assignp = VN_AS(alwaysp->stmtsp(), ਸੌਂਪੋ_W);
        assignp->timingControlp(delayp->backp() ? delayp->clone_ਰੁੱਖ(false) : delayp);
    }
}
// Apply a strength to all continuous assignments under listp
ਸਥਿਰ ਖਾਲੀ STRENGTH_LIST(Ast_ਗੰਢ* listp, AstStrengthSpec* specp) {
    ਜੇ (!specp) ਵਾਪਸ;
    ਲਈ (Ast_ਗੰਢ* nodep = listp; nodep; nodep = nodep->nextp()) {
        ਜੇ (VN_IS(nodep, Implicit)) ਜਾਰੀ;
        AstAlways* ਅਚਲ alwaysp = VN_AS(nodep, Always);
        Ast_ਸੌਂਪੋ_W* ਅਚਲ assignp = VN_AS(alwaysp->stmtsp(), ਸੌਂਪੋ_W);
        assignp->strengthSpecp(specp->backp() ? specp->clone_ਰੁੱਖ(false) : specp);
    }
}
//======================================================================
// Statics (for here only)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ PARSEP V3ParseImp::parsep()
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ GRAMMARP V3ParseGrammar::singletonp()

ਅਚਲ VBasicDTypeKwd LOGIC = VBasicDTypeKwd::LOGIC;  // Shorthand "LOGIC"
ਅਚਲ VBasicDTypeKwd LOGIC_IMPLICIT = VBasicDTypeKwd::LOGIC_IMPLICIT;

//======================================================================
// Macro functions

// Only use in empty rules, so lines point at beginnings
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ CRELINE() (PARSEP->bison_ਆਖਰੀ_Fileline()->copy_Or_Same_File_ਸਤਰ_Applied())
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FILELINE_OR_CRE(nodep) ((nodep) ? (nodep)->fileline() : CRELINE())

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARRESET_LIST(decl) VARRESET__PVT(decl, 1)  // Start of pinlist
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARRESET_NONLIST(decl) VARRESET__PVT(decl, 0);  // Not in a pinlist
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARRESET__PVT(decl, pin_ਅੰਕ_ਸ਼ੁਰੂ) \
    { \
        VARDECL(decl); \
        VARIO(NONE); \
        VARDTYPE_NDECL(nullptr); \
        GRAMMARP->m_pin_ਅੰਕ = (pin_ਅੰਕ_ਸ਼ੁਰੂ); \
        GRAMMARP->m_varLifetime = VLifetime::NONE; \
        GRAMMARP->m_varDeclTyped = false; \
    }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARDECL(ਕਿਸਮ) \
    { GRAMMARP->m_varDecl = VVarType::ਕਿਸਮ; }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARIO(ਕਿਸਮ) \
    { GRAMMARP->m_varIO = VDirection::ਕਿਸਮ; }
// Set direction to default-input when detect inside an ANSI port list
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARIOANSI() \
    { \
        ਜੇ (GRAMMARP->m_varIO == VDirection::NONE) VARIO(ਨਿਵੇਸ਼); \
    }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARLIFE(flag) \
    { GRAMMARP->m_varLifetime = flag; }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARDTYPE(dtypep) \
    { \
        GRAMMARP->ਸੈੱਟ_DType(dtypep); \
        GRAMMARP->m_varDeclTyped = true; \
    }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARDTYPE_NDECL(dtypep) \
    { GRAMMARP->ਸੈੱਟ_DType(dtypep); }  // Port that is range or signed only (not a decl)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARDONEA(fl, name, array, attrs) GRAMMARP->createVariable((fl), (name), (array), (attrs))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VARDONEP(portp, array, attrs) \
    GRAMMARP->createVariable((portp)->fileline(), (portp)->name(), (array), (attrs))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ PINNUMINC() (GRAMMARP->m_pin_ਅੰਕ++)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ GATERANGE(rangep) \
    { GRAMMARP->m_gateRangep = rangep; }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ INSTPREP(modfl, modname, paramsp) \
    { \
        GRAMMARP->m_impliedDecl = true; \
        GRAMMARP->m_instModuleFl = modfl; \
        GRAMMARP->m_instModule = modname; \
        GRAMMARP->m_instParamp = paramsp; \
    }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ DEL(...) \
    { \
        /* cppcheck-suppress constVariable */ \
        Ast_ਗੰਢ* ਅਚਲ nodeps[] = {__VA_ARGS__}; \
        ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : nodeps) \
            ਜੇ (nodep) nodep->delete_ਰੁੱਖ(); \
    }

ਸਥਿਰ ਖਾਲੀ ERRSVKWD(File_ਸਤਰ* fileline, ਅਚਲ string& tokname) {
    ਸਥਿਰ ਪੂਰਨ s_toldonce = 0;
    fileline->v3error(
        "Unexpected '"s + tokname + "': '" + tokname
        + "' is a SystemVerilog keyword misused as an identifier."
        + (!s_toldonce++ ? "\n" + fileline->warn_ਹੋਰ()
                               + "... Suggest modify the Verilog-2001 code to avoid SV keywords,"
                               + " or use `begin_keywords or --language."
                         : ""));
}

ਸਥਿਰ ਖਾਲੀ ASSIGNEQEXPR(File_ਸਤਰ* fileline) {
    fileline->v3warn(ASSIGNEQEXPR,
                     "Assignment '=' inside expression\n"
                         << fileline->warn_ਹੋਰ()
                         << "... Was a '==' intended, or suggest use a separate statement");
}

ਸਥਿਰ ਖਾਲੀ UNSUPREAL(File_ਸਤਰ* fileline) {
    fileline->v3warn(SHORTREAL,
                     "Unsupported: shortreal being promoted to real (suggest use real instead)");
}

//======================================================================

ਖਾਲੀ yyerror(ਅਚਲ ਅੱਖਰ* errmsg) { PARSEP->bison_ਆਖਰੀ_Fileline()->v3error(errmsg); }

ਸਾਂਚਾ <typename T_ਗੰਢ, typename T_ਅਗਲਾ>
ਸਥਿਰ T_ਗੰਢ* ਜੋੜੋ_ਅਗਲਾ_Null(T_ਗੰਢ* nodep, T_ਅਗਲਾ* nextp) {
    ਵਾਪਸ Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null<T_ਗੰਢ, T_ਅਗਲਾ>(nodep, nextp);
}

//======================================================================

class Ast_Sen_ਰੁੱਖ;
// clang-format off
%}

// Bison 3.0 and newer
BISONPRE_ਸੰਸਕਰਣ(3.0,%define parse.error verbose)

// We run bison with the -d argument. This tells it to generate a
// header file with token names. Old versions of bison pasted the
// contents of that file into the generated source as well; newer
// versions just include it.
//
// Since we run bison through ../bisonpre, it doesn't know the correct
// header file name, so we need to tell it.
BISONPRE_ਸੰਸਕਰਣ(3.7,%define api.ਸਿਰਲੇਖ.include {"V3ParseBison.h"})

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

%token<strp>            ya_ਸਾਰਣੀ_ਖੇਤਰ   "UDP table field"
%token<fl>              ya_ਸਾਰਣੀ_LRSEP   ":"
%token<fl>              ya_ਸਾਰਣੀ_LINEEND "UDP table line end"

%token<strp>            yaSCCTOR        "`systemc_ctor block"
%token<strp>            yaSCDTOR        "`systemc_dtor block"
%token<strp>            yaSCHDR         "`systemc_header block"
%token<strp>            yaSCHDRP        "`systemc_header_post block"
%token<strp>            yaSCIMP         "`systemc_implementation block"
%token<strp>            yaSCIMPH        "`systemc_imp_header block"
%token<strp>            yaSCINT         "`systemc_interface block"

%token<fl>              yVLT_CLOCKER                "clocker"
%token<fl>              yVLT_CLOCK_ENABLE           "clock_enable"
%token<fl>              y_VLT_COVERAGE_ਖੰਡ_OFF     "coverage_block_off"
%token<fl>              yVLT_COVERAGE_OFF           "coverage_off"
%token<fl>              yVLT_COVERAGE_ON            "coverage_on"
%token<fl>              yVLT_FORCEABLE              "forceable"
%token<fl>              y_VLT_FSM_ਰਜਿਸਟਰ_WRAPPER   "fsm_register_wrapper"
%token<fl>              y_VLT_ਭਰਿਆ_CASE              "full_case"
%token<fl>              y_VLT_HIER_ਖੰਡ             "hier_block"
%token<fl>              yVLT_HIER_PARAMS            "hier_params"
%token<fl>              yVLT_HIER_WORKERS           "hier_workers"
%token<fl>              yVLT_INLINE                 "inline"
%token<fl>              yVLT_ISOLATE_ASSIGNMENTS    "isolate_assignments"
%token<fl>              yVLT_LINT_OFF               "lint_off"
%token<fl>              yVLT_LINT_ON                "lint_on"
%token<fl>              yVLT_NO_CLOCKER             "no_clocker"
%token<fl>              yVLT_NO_INLINE              "no_inline"
%token<fl>              yVLT_PARALLEL_CASE          "parallel_case"
%token<fl>              y_VLT_PROFILE_ਅੰਕੜੇ           "profile_data"
%token<fl>              y_VLT_ਜਨਤਕ                 "public"
%token<fl>              y_VLT_ਜਨਤਕ_FLAT            "public_flat"
%token<fl>              y_VLT_ਜਨਤਕ_FLAT_RD         "public_flat_rd"
%token<fl>              y_VLT_ਜਨਤਕ_FLAT_RW         "public_flat_rw"
%token<fl>              y_VLT_ਜਨਤਕ_MODULE          "public_module"
%token<fl>              yVLT_SC_BIGUINT             "sc_biguint"
%token<fl>              yVLT_SC_BV                  "sc_bv"
%token<fl>              yVLT_SFORMAT                "sformat"
%token<fl>              y_VLT_ਵੰਡੋ_VAR              "split_var"
%token<fl>              yVLT_TIMING_OFF             "timing_off"
%token<fl>              yVLT_TIMING_ON              "timing_on"
%token<fl>              yVLT_TRACING_OFF            "tracing_off"
%token<fl>              yVLT_TRACING_ON             "tracing_on"
%token<fl>              yVLT_VERILATOR_LIB          "verilator_lib"

%token<fl>              y_VLT_D_ਖੰਡ    "--block"
%token<fl>              yVLT_D_CONTENTS "--contents"
%token<fl>              y_VLT_D_ਲਾਗਤ     "--cost"
%token<fl>              yVLT_D_CLOCK    "--clock"
%token<fl>              yVLT_D_D        "--d"
%token<fl>              yVLT_D_FILE     "--file"
%token<fl>              yVLT_D_FUNCTION "--function"
%token<fl>              yVLT_D_HIER_DPI "--hier-dpi"
%token<fl>              y_VLT_D_ਪੱਧਰ   "--levels"
%token<fl>              y_VLT_D_ਸਤਰਾਂ    "--lines"
%token<fl>              y_VLT_D_ਮੇਲ    "--match"
%token<fl>              yVLT_D_MODEL    "--model"
%token<fl>              yVLT_D_MODULE   "--module"
%token<fl>              yVLT_D_MTASK    "--mtask"
%token<fl>              yVLT_D_PARAM    "--param"
%token<fl>              y_VLT_D_ਪੋਰਟ     "--port"
%token<fl>              y_VLT_D_ਨਿਯਮ     "--rule"
%token<fl>              yVLT_D_Q        "--q"
%token<fl>              yVLT_D_RESET    "--reset"
%token<fl>              y_VLT_D_RESET_ਮੁੱਲ "--reset_value"
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
%token<fl>              y1_ਕਦਮ          "1step"
%token<fl>              yACCEPT_ON      "accept_on"
%token<fl>              yALIAS          "alias"
%token<fl>              yALWAYS         "always"
%token<fl>              yALWAYS_COMB    "always_comb"
%token<fl>              yALWAYS_FF      "always_ff"
%token<fl>              yALWAYS_LATCH   "always_latch"
%token<fl>              yAND            "and"
%token<fl>              yASSERT         "assert"
%token<fl>              y_ਸੌਂਪੋ         "assign"
%token<fl>              yASSUME         "assume"
%token<fl>              yAUTOMATIC      "automatic"
%token<fl>              yBEFORE         "before"
%token<fl>              yBEGIN          "begin"
%token<fl>              yBIND           "bind"
%token<fl>              yBINS           "bins"
%token<fl>              yBINSOF         "binsof"
%token<fl>              y_ਦੋਆਧਾਰੀ_ਅੰਕ            "bit"
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
%token<fl>              y_ਕਿਨਾਰਾ           "edge"
%token<fl>              yELSE           "else"
%token<fl>              y_ਸਿਰਾ            "end"
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
%token<fl>              y_ਪਹਿਲਾ_ਮੇਲ    "first_match"
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
%token<fl>              y_ਸਿੱਟਾ_ਦਿੰਦਾ        "implies"
%token<fl>              yIMPORT         "import"
%token<fl>              yINCDIR         "incdir"
%token<fl>              yINCLUDE        "include"
%token<fl>              yINITIAL        "initial"
%token<fl>              yINOUT          "inout"
%token<fl>              y_ਨਿਵੇਸ਼          "input"
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
%token<fl>              y_ਤਰਜੀਹ       "priority"
%token<fl>              y_ਪ੍ਰੋਗਰਾਮ        "program"
%token<fl>              yPROPERTY       "property"
%token<fl>              yPROTECTED      "protected"
%token<fl>              yPULL0          "pull0"
%token<fl>              yPULL1          "pull1"
%token<fl>              yPULLDOWN       "pulldown"
%token<fl>              yPULLUP         "pullup"
%token<fl>              y_ਸ਼ੁੱਧ           "pure"
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
%token<fl>              y_ਸਾਰਣੀ          "table"
%token<fl>              yTAGGED         "tagged"
%token<fl>              yTAGGED__LEX    "tagged-in-lex"
%token<fl>              yTAGGED__NONPRIMARY   "tagged-nonprimary"
%token<fl>              yTASK           "task"
%token<fl>              yTHIS           "this"
%token<fl>              yTHROUGHOUT     "throughout"
%token<fl>              y_ਸਮਾਂ           "time"
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
%token<fl>              y_WAIT_ਕ੍ਰਮ     "wait_order"
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
%token<fl>              y_D_ਬਿੱਟ         "$bits"
%token<fl>              yD_BITSTOREAL   "$bitstoreal"
%token<fl>              yD_BITSTOSHORTREAL "$bitstoshortreal"
%token<fl>              yD_C            "$c"
%token<fl>              yD_CPURE        "$cpure"
%token<fl>              yD_CAST         "$cast"
%token<fl>              yD_CEIL         "$ceil"
%token<fl>              y_D_ਬਦਲਿਆ      "$changed"
%token<fl>              y_D_ਬਦਲਿਆ_GCLK "$changed_gclk"
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
%token<fl>              y_D_DIST_CHI_ਵਰਗਫਲ "$dist_chi_square"
%token<fl>              yD_DIST_ERLANG  "$dist_erlang"
%token<fl>              y_D_DIST_ਘਾਤਾਂਕੀ "$dist_exponential"
%token<fl>              y_D_DIST_ਸਾਧਾਰਨ  "$dist_normal"
%token<fl>              y_D_DIST_ਪੁਆਸੋਂ "$dist_poisson"
%token<fl>              yD_DIST_T       "$dist_t"
%token<fl>              y_D_DIST_ਇਕਸਾਰ "$dist_uniform"
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
%token<fl>              y_D_ਅੰਤ       "$finish"
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
%token<fl>              y_D_GET_INITIAL_RANDOM_ਬੀਜ "$get_initial_random_seed"
%token<fl>              yD_GLOBAL_CLOCK "$global_clock"
%token<fl>              y_D_ਉੱਚਾ         "$high"
%token<fl>              yD_HYPOT        "$hypot"
%token<fl>              yD_INCREMENT    "$increment"
%token<fl>              yD_INFERRED_DISABLE "$inferred_disable"
%token<fl>              yD_INFO         "$info"
%token<fl>              yD_ISUNBOUNDED  "$isunbounded"
%token<fl>              yD_ISUNKNOWN    "$isunknown"
%token<fl>              yD_ITOR         "$itor"
%token<fl>              y_D_ਖੱਬਾ         "$left"
%token<fl>              yD_LN           "$ln"
%token<fl>              yD_LOG10        "$log10"
%token<fl>              y_D_ਨੀਵਾਂ          "$low"
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
%token<fl>              y_D_ਸੱਜਾ        "$right"
%token<fl>              yD_RISING_GCLK  "$rising_gclk"
%token<fl>              y_D_ਜੜ੍ਹ         "$root"
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
%token<fl>              y_D_ਮਾਪ         "$size"
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
%token<fl>              y_D_ਸਮਾਂ         "$time"
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
%token<fl>              y_VL_COVERAGE_ਖੰਡ_OFF    "/*verilator coverage_block_off*/"
%token<fl>              yVL_DPI_C_DECL            "/*verilator dpi_c_decl*/"
%token<fl>              yVL_FORCEABLE             "/*verilator forceable*/"
%token<fl>              y_VL_ਭਰਿਆ_CASE             "/*verilator full_case*/"
%token<fl>              y_VL_HIER_ਖੰਡ            "/*verilator hier_block*/"
%token<fl>              yVL_INLINE_MODULE         "/*verilator inline_module*/"
%token<fl>              yVL_ISOLATE_ASSIGNMENTS   "/*verilator isolate_assignments*/"
%token<fl>              yVL_NO_CLOCKER            "/*verilator no_clocker*/"
%token<fl>              yVL_NO_INLINE_MODULE      "/*verilator no_inline_module*/"
%token<fl>              yVL_NO_INLINE_TASK        "/*verilator no_inline_task*/"
%token<fl>              yVL_PARALLEL_CASE         "/*verilator parallel_case*/"
%token<fl>              y_VL_ਜਨਤਕ                "/*verilator public*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT           "/*verilator public_flat*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT_ON        "/*verilator public_flat_on*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT_RD        "/*verilator public_flat_rd*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT_RD_ON     "/*verilator public_flat_rd_on*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT_RW        "/*verilator public_flat_rw*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT_RW_ON     "/*verilator public_flat_rw_on*/"
%token<fl>              y_VL_ਜਨਤਕ_FLAT_RW_ON_SNS "/*verilator public_flat_rw_on_sns*/"
%token<fl>              y_VL_ਜਨਤਕ_ON             "/*verilator public_on*/"
%token<fl>              y_VL_ਜਨਤਕ_OFF            "/*verilator public_off*/"
%token<fl>              y_VL_ਜਨਤਕ_MODULE         "/*verilator public_module*/"
%token<fl>              yVL_SC_BIGUINT            "/*verilator sc_biguint*/"
%token<fl>              yVL_SC_BV                 "/*verilator sc_bv*/"
%token<fl>              yVL_SFORMAT               "/*verilator sformat*/"
%token<fl>              y_VL_ਵੰਡੋ_VAR             "/*verilator split_var*/"
%token<fl>              yVL_FSM_ARC_INCL_COND     "/*verilator fsm_arc_include_cond*/"
%token<fl>              yVL_FSM_ARC_INCL_COND_AUTO        "/*verilator fsm_arc_include_cond_auto*/"
%token<fl>              yVL_FSM_ARC_INCL_COND_AUTO_EXPAND "/*verilator fsm_arc_include_cond_auto_expand*/"
%token<fl>              y_VL_FSM_ARC_INCL_COND_ਭਰਿਆ        "/*verilator fsm_arc_include_cond_full*/"
%token<fl>              yVL_FSM_STATE             "/*verilator fsm_state*/"
%token<fl>              yVL_FSM_STATE_AUTO        "/*verilator fsm_state_auto*/"
%token<fl>              yVL_FSM_STATE_AUTO_EXPAND "/*verilator fsm_state_auto_expand*/"
%token<fl>              y_VL_FSM_STATE_ਭਰਿਆ        "/*verilator fsm_state_full*/"
%token<strp>            y_VL_ਟੈਗ                   "/*verilator tag*/"
%token<fl>              yVL_UNROLL_DISABLE        "/*verilator unroll_disable*/"
%token<fl>              y_VL_UNROLL_ਭਰਿਆ           "/*verilator unroll_full*/"

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
%ਖੱਬੇ           yP_ANDANDAND
%ਖੱਬੇ           yMATCHES
%ਖੱਬੇ           prTAGGED
//UNSUP %left   prSEQ_CLOCKING

// PSL op precedence

// Lowest precedence
// These are in IEEE 17.7.1
%nonassoc       prALWAYS prS_ALWAYS prEVENTUALLY prS_EVENTUALLY prIF prACCEPT_ON prREJECT_ON prSYNC_ACCEPT_ON prSYNC_REJECT_ON

%ਸੱਜੇ          yP_ORMINUSGT yP_OREQGT yP_POUNDMINUSPD yP_POUNDEQPD
%ਸੱਜੇ          yUNTIL yS_UNTIL yUNTIL_WITH yS_UNTIL_WITH y_ਸਿੱਟਾ_ਦਿੰਦਾ
%ਸੱਜੇ          yIFF
%ਖੱਬੇ           yOR
%ਖੱਬੇ           yAND
%nonassoc       yNOT yNEXTTIME yS_NEXTTIME
%ਖੱਬੇ           yINTERSECT
%ਖੱਬੇ           yWITHIN
%ਸੱਜੇ          yTHROUGHOUT
%ਖੱਬੇ           prPOUNDPOUND_MULTI
%ਖੱਬੇ           yP_POUNDPOUND
%ਖੱਬੇ           yP_BRASTAR yP_BRAEQ yP_BRAMINUSGT yP_BRAPLUSKET

// Not specified, but needed higher than yOR, lower than normal non-pexpr expressions
//UNSUP %left           yPOSEDGE yNEGEDGE yEDGE

//UNSUP %left           '{' '}'

// Verilog op precedence
%ਸੱਜੇ          yP_MINUSGT yP_LTMINUSGT
%ਸੱਜੇ          '?' ':' yP_COLON__BEGIN yP_COLON__FORK
%ਖੱਬੇ           yP_OROR
%ਖੱਬੇ           yP_ANDAND
%ਖੱਬੇ           '|' yP_NOR
%ਖੱਬੇ           '^' yP_XNOR
%ਖੱਬੇ           '&' yP_NAND
%ਖੱਬੇ           yP_EQUAL yP_NOTEQUAL yP_CASEEQUAL yP_CASENOTEQUAL yP_WILDEQUAL yP_WILDNOTEQUAL
%ਖੱਬੇ           '>' '<' yP_GTE yP_LTE yP_LTE__IGNORE yINSIDE yDIST
%ਖੱਬੇ           yP_SLEFT yP_SRIGHT yP_SSRIGHT
%ਖੱਬੇ           '+' '-'
%ਖੱਬੇ           '*' '/' '%'
%ਖੱਬੇ           yP_POW
%ਖੱਬੇ           prUNARYARITH yP_MINUSMINUS yP_PLUSPLUS prREDUCTION prNEGATION
%ਖੱਬੇ           '.'
// Not in IEEE, but need to avoid conflicts; TICK should bind tightly just lower than COLONCOLON
%ਖੱਬੇ           yP_TICK
//%left         '(' ')' '[' ']' yP_COLONCOLON '.'

%nonassoc pr_ਹੇਠਲਾ_THAN_ELSE
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

%start ਸਰੋਤ_ਪਾਠ

%%
//**********************************************************************
// Files

ਸਰੋਤ_ਪਾਠ:                    // ==IEEE: source_text
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { }
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
        |       library_declaration                     { PARSEP->rootp()->ਜੋੜੋ_Miscsp($1); }
        |       include_statement                       { }
        |       interface_declaration                   { }
        |       ਪ੍ਰੋਗਰਾਮ_declaration                     { }
        |       package_declaration                     { }
        |       package_ਵਸਤੂ_ਸਿਖਰ                         { ਜੇ ($1) PARSEP->unitPackage($1->fileline())->ਜੋੜੋ_Stmtsp($1); }
        |       bind_directive                          { ਜੇ ($1) PARSEP->unitPackage($1->fileline())->ਜੋੜੋ_Stmtsp($1); }
        |       config_declaration                      { }
        //                      // Verilator only
        |       yaT_RESETALL                            { }  // Else, under design, and illegal based on IEEE 22.3
        |       yaT_NOUNCONNECTED                       { PARSEP->unconnectedDrive(VOptionBool::OPT_DEFAULT_FALSE); }
        |       yaT_UNCONNECTED_PULL0                   { PARSEP->unconnectedDrive(VOptionBool::OPT_FALSE); }
        |       yaT_UNCONNECTED_PULL1                   { PARSEP->unconnectedDrive(VOptionBool::OPT_TRUE); }
        |       vlt_ਵਸਤੂ                                 { }
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
                packageFront package_ਵਸਤੂ_List_E yENDPACKAGE ਸਿਰਾ_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          ਜੇ ($2) $1->ਜੋੜੋ_Stmtsp($2);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>4, $1, $4); }
        ;

packageFront<ਗੰਢ_Modulep>:
                yPACKAGE lifetimeE idAny ';'
                        { $$ = new AstPackage{$<fl>3, *$3, PARSEP->libname()};
                          ਜੇ ($$->name() == "std") {
                              ਜੇ ($$->fileline()->filename() != V3Options::get_Std_Package_ਰਸਤਾ()) {
                                  $$->v3error("Redeclaring the 'std' package is not allowed");
                              } ਨਹੀਂਤਾਂ {
                                  PARSEP->rootp()->stdPackagep(VN_AS($$, Package));
                              }
                          }
                          $$->ਵਿੱਚ_Library(true);  // packages are always libraries; don't want to make them a "top"
                          $$->lifetime($2);
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ਸਮਾਂ_ਆਖਰੀ_Unit());
                          PARSEP->rootp()->timeprecision_ਮਿਲਾਓ($$->fileline(),
                                                              PARSEP->ਸਮਾਂ_ਆਖਰੀ_Prec());
                          PARSEP->rootp()->ਜੋੜੋ_Modulesp($$); }
        ;

package_ਵਸਤੂ_List_E<nodep>:       // IEEE: [{ package_item }]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       package_ਵਸਤੂ_List                        { $$ = $1; }
        ;

package_ਵਸਤੂ_List<nodep>:        // IEEE: { package_item }
                package_ਵਸਤੂ                            { $$ = $1; }
        |       package_ਵਸਤੂ_List package_ਵਸਤੂ           { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

package_ਵਸਤੂ<nodep>:            // ==IEEE: package_item
                package_or_generate_ਵਸਤੂ_declaration    { $$ = $1; }
        |       anonymous_ਪ੍ਰੋਗਰਾਮ                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_ਵਸਤੂ_ਸਿਖਰ<nodep>:         // ==IEEE: package_item

                package_or_generate_ਵਸਤੂ_decl_No_Checker  { $$ = $1; }
        |       checker_declaration
                       { PARSEP->rootp()->ਜੋੜੋ_Modulesp($1);
                         $$ = nullptr; }
        |       anonymous_ਪ੍ਰੋਗਰਾਮ                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_or_generate_ਵਸਤੂ_declaration<nodep>:    // ==IEEE: package_or_generate_item_declaration
                package_or_generate_ਵਸਤੂ_decl_No_Checker  { $$ = $1; }
        |       checker_declaration
                        { $1->v3warn(E_UNSUPPORTED, "Unsupported: 'checker' below unit-level");
                          PARSEP->rootp()->ਜੋੜੋ_Modulesp($1);
                          $$ = nullptr; }
        ;

package_or_generate_ਵਸਤੂ_decl_No_Checker<nodep>:
                net_declaration                         { $$ = $1; }
        |       ਅੰਕੜੇ_declaration                        { $$ = $1; }
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
        |       assertion_ਵਸਤੂ_declaration              { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

package_import_declarationList<nodep>:
                package_import_declaration              { $$ = $1; }
        |       package_import_declarationList package_import_declaration
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

package_import_declaration<nodep>:      // ==IEEE: package_import_declaration
                yIMPORT package_import_ਵਸਤੂ_List ';'     { $$ = $2; }
        ;

package_import_ਵਸਤੂ_List<nodep>:
                package_import_ਵਸਤੂ                     { $$ = $1; }
        |       package_import_ਵਸਤੂ_List ',' package_import_ਵਸਤੂ { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

package_import_ਵਸਤੂ<nodep>:     // ==IEEE: package_import_item
                idCC/*package_identifier*/ yP_COLONCOLON package_import_ਵਸਤੂ_Obj
                        { $$ = new AstPackageImport{$<fl>1, *$<strp>1, *$3}; }
        ;

package_import_ਵਸਤੂ_Obj<strp>:   // IEEE: part of package_import_item
                idAny/*package_identifier*/             { $<fl>$ = $<fl>1; $$ = $1; }
        |       '*'                                     { $<fl>$ = $<fl>1; ਸਥਿਰ string ਤਾਰਾ = "*"; $$ = &ਤਾਰਾ; }
        ;

package_export_declaration<nodep>: // IEEE: package_export_declaration
                yEXPORT '*' yP_COLONCOLON '*' ';'
                        { $$ = new Ast_Package_Export_ਤਾਰਾ_ਤਾਰਾ{$<fl>2}; }
        |       yEXPORT package_export_ਵਸਤੂ_List ';'     { $$ = $2; }
        ;

package_export_ਵਸਤੂ_List<nodep>:
                package_export_ਵਸਤੂ                     { $$ = $1; }
        |       package_export_ਵਸਤੂ_List ',' package_export_ਵਸਤੂ { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

package_export_ਵਸਤੂ<nodep>:     // ==IEEE: package_export_item
                idCC yP_COLONCOLON package_import_ਵਸਤੂ_Obj
                        { $$ = new AstPackageExport{$<fl>3, *$<strp>1, *$3}; }
        ;

//**********************************************************************
// Module headers

module_declaration:             // ==IEEE: module_declaration
        //                      // timeunits_declaration instead in module_item
        //                      // IEEE: module_nonansi_header + module_ansi_header
                modFront importsAndParametersE ports_ਤਾਰਾ_E ';'
        /*cont*/    module_ਵਸਤੂ_List_E yENDMODULE ਸਿਰਾ_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          ਜੇ ($2) $1->ਜੋੜੋ_Stmtsp($2);
                          ਜੇ ($3) $1->ਜੋੜੋ_Stmtsp($3);
                          ਜੇ ($5) $1->ਜੋੜੋ_Stmtsp($5);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>7, $1, $7); }
        |       udpFront ports_ਤਾਰਾ_E ';'
        /*cont*/    module_ਵਸਤੂ_List_E yENDPRIMITIVE ਸਿਰਾ_Label_E
                        { $1->modTrace(false);  // Stash for implicit wires, etc
                          ਜੇ ($2) $1->ਜੋੜੋ_Stmtsp($2);
                          ਜੇ ($4) $1->ਜੋੜੋ_Stmtsp($4);
                          GRAMMARP->m_tracingParse = true;
                          GRAMMARP->ਸਿਰਾ_Label($<fl>6, $1, $6); }
        //
        |       yEXTERN modFront parameter_ਪੋਰਟ_list_E ports_ਤਾਰਾ_E ';'
                        { DEL($2->unlink_Fr_ਪਿੱਠ()); }
                        // We allow modules to be declared after instantiations, so harmless
        ;

modFront<ਗੰਢ_Modulep>:
        //                      // General note: all *Front functions must call symPushNew before
        //                      // any formal arguments, as the arguments must land in the new scope.
                yMODULE lifetimeE idAny
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname()};
                          $$->lifetime($2);
                          $$->ਵਿੱਚ_Library(PARSEP->ਵਿੱਚ_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ਸਮਾਂ_ਆਖਰੀ_Unit());
                          PARSEP->rootp()->timeprecision_ਮਿਲਾਓ($$->fileline(),
                                                              PARSEP->ਸਮਾਂ_ਆਖਰੀ_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive());
                          PARSEP->rootp()->ਜੋੜੋ_Modulesp($$); }
        |       modFront sigAttrScope                   { $$ = $1; }
        ;

importsAndParametersE<nodep>:   // IEEE: common part of module_declaration, interface_declaration, program_declaration
        //                      // { package_import_declaration } [ parameter_port_list ]
                parameter_ਪੋਰਟ_list_E
                        { $$ = $1; $<flag>$ = $<flag>1; }  // hasParameterList
        |       package_import_declarationList parameter_ਪੋਰਟ_list_E
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2);
                          $<flag>$ = $<flag>2; }  // hasParameterList
        ;

udpFront<ਗੰਢ_Modulep>:
                yPRIMITIVE lifetimeE idAny
                        { $$ = new AstPrimitive{$<fl>3, *$3, PARSEP->libname()};
                          $$->ਵਿੱਚ_Library(true);
                          $$->lifetime($2);
                          $$->modTrace(false);
                          $$->ਜੋੜੋ_Stmtsp(new AstPragma{$<fl>3, VPragmaType::INLINE_MODULE});
                          GRAMMARP->m_tracingParse = false;
                          PARSEP->rootp()->ਜੋੜੋ_Modulesp($$); }
        ;

parameter_ਮੁੱਲ_assignment_Inst_E<pinp>:      // IEEE: [ parameter_value_assignment ] for instance
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       parameter_ਮੁੱਲ_assignment_Inst          { $$ = $1; }
        ;

parameter_ਮੁੱਲ_assignment_Class_E<pinp>:      // IEEE: [ parameter_value_assignment ] for classes
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       parameter_ਮੁੱਲ_assignment_Class         { $$ = $1; }
        ;

parameter_ਮੁੱਲ_assignment_Inst<pinp>:       // IEEE: parameter_value_assignment for instance
                '#' '(' instParamListE ')'              { $$ = $3; }
        //                      // Parentheses are optional around a single parameter
        //                      // IMPORTANT: Below hardcoded in tokenPipeScanParam
        |       '#' yaINTNUM                            { $$ = new AstPin{$<fl>2, 1, "", new AstConst{$<fl>2, *$2}}; }
        |       '#' yaFLOATNUM                          { $$ = new AstPin{$<fl>2, 1, "",
                                                                          new AstConst{$<fl>2, AstConst::RealDouble{}, $2}}; }
        |       '#' ਸਮਾਂ_ਅੰਕ_Adjusted                     { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        |       '#' idClassSel                          { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        //                      // Not needed in Verilator:
        //                      // Side effect of combining *_instantiations
        //                      // '#' delay_value      { UNSUP }
        ;

parameter_ਮੁੱਲ_assignment_Class<pinp>:  // IEEE: parameter_value_assignment (for classes)
        //                      // Like parameter_value_assignment, but for classes only, which always have #()
                '#' '(' instParamListE ')'              { $$ = $3; }
        ;

parameter_ਪੋਰਟ_list_E<nodep>:    // IEEE: parameter_port_list + empty == parameter_value_assignment
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; $<flag>$ = false; }  // hasParameterList
        |       '#' '(' ')'                             { $$ = nullptr;
                                                          $<flag>$ = true; }  // hasParameterList
        //                      // IEEE: '#' '(' list_of_param_assignments { ',' parameter_port_declaration } ')'
        //                      // IEEE: '#' '(' parameter_port_declaration { ',' parameter_port_declaration } ')'
        //                      // Can't just do that as "," conflicts with between vars and between stmts, so
        //                      // split into pre-comma and post-comma parts
        |       '#' '('                                 { VARRESET_LIST(GPARAM);
                                                          GRAMMARP->m_pinAnsi = true; }
        /*cont*/    param_ਪੋਰਟ_Decl_Or_Arg_List ')'          { $$ = $4;
                                                          $<flag>$ = true;  // hasParameterList
                                                          VARRESET_NONLIST(UNKNOWN);
                                                          GRAMMARP->m_pinAnsi = false; }
        //                      // Note legal to start with "a=b" with no parameter statement
        ;

param_ਪੋਰਟ_Decl_Or_Arg_List<nodep>:  // IEEE: list_of_param_assignments + { parameter_port_declaration }
                param_ਪੋਰਟ_Decl_Or_Arg                              { $$ = $1; }
        |       param_ਪੋਰਟ_Decl_Or_Arg_List ',' param_ਪੋਰਟ_Decl_Or_Arg   { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        |       param_ਪੋਰਟ_Decl_Or_Arg_List sigAttrScope             { $$ = $1; }
        ;

param_ਪੋਰਟ_Decl_Or_Arg<nodep>:      // IEEE: param_assignment + parameter_port_declaration
        //                      // We combine the two as we can't tell which follows a comma
                param_ਪੋਰਟ_Decl_Or_Arg_Sub                   { $$ = $1; }
        |       vl_ਟੈਗ                                   { $$ = nullptr; }
        ;

param_ਪੋਰਟ_Decl_Or_Arg_Sub<nodep>:
                parameter_ਪੋਰਟ_declaration_Front_E param_assignment       { $$ = $2; }
        |       parameter_ਪੋਰਟ_declaration_Type_Front_E type_assignment    { $$ = $2; }
        |       sigAttrScope param_ਪੋਰਟ_Decl_Or_Arg_Sub                      { $$ = $2; }
        ;

ports_ਤਾਰਾ_E<nodep>:              // IEEE: .* + list_of_ports + list_of_port_declarations + empty
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        //                      // .* expanded from module_declaration
        //UNSUP '(' yP_DOTSTAR ')'                              { UNSUP }
        |       '('
        /*mid*/         { VARRESET_LIST(ਪੋਰਟ); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    list_ਦਾ_ports ')'
                       { $$ = $3; VARRESET_NONLIST(UNKNOWN); GRAMMARP->m_pinAnsi = false; }
        ;

list_ਦਾ_ports_E<nodep>:          // IEEE: [ list_of_ports + list_of_port_declarations ]
                ਪੋਰਟ_And_ਟੈਗ_E                             { $$ = $1; }
        |       list_ਦਾ_ports_E ',' ਪੋਰਟ_And_ਟੈਗ_E          { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

list_ਦਾ_ports<nodep>:           // IEEE: list_of_ports + list_of_port_declarations
                ਪੋਰਟ_And_ਟੈਗ                              { $$ = $1; }
        |       list_ਦਾ_ports_E ',' ਪੋਰਟ_And_ਟੈਗ_E          { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

ਪੋਰਟ_And_ਟੈਗ_E<nodep>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { ਅਚਲ ਪੂਰਨ p = PINNUMINC();
                          ਅਚਲ string name = "__pinNumber" + cvt_ਮੰਜ਼ਿਲ_Str(p);
                          $$ = new Ast_ਪੋਰਟ{CRELINE(), p, name};
                          AstVar* varp = new AstVar{CRELINE(), VVarType::WIRE, name, VFlag_ਬੱਚਾ_DType{},
                                                    new AstBasicDType{CRELINE(), LOGIC_IMPLICIT}};
                          varp->declDirection(VDirection::ਨਿਵੇਸ਼);
                          varp->direction(VDirection::ਨਿਵੇਸ਼);
                          varp->ansi(false);
                          varp->declTyped(true);
                          varp->trace(false);
                          $$ = $$->ਜੋੜੋ_ਅਗਲਾ(varp);
                          $$->v3warn(NULLPORT, "Null port on module (perhaps extraneous comma)"); }
        |       ਪੋਰਟ_And_ਟੈਗ                              { $$ = $1; }
        |       ਪੋਰਟ_And_ਟੈਗ sigAttrScope                 { $$ = $1; }
        ;

ਪੋਰਟ_And_ਟੈਗ<nodep>:
                ਪੋਰਟ_3                                    { $$ = $1; }
        |       vl_ਟੈਗ ਪੋਰਟ_3                              { $$ = $2; }  // Tag will associate with previous port
        |       sigAttrScope ਪੋਰਟ_And_ਟੈਗ                 { $$ = $2; }
        ;

ਪੋਰਟ_3<nodep>:                    // ==IEEE: port
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
                ਪੋਰਟ_Dir_Net_E id/*interface*/ '.' idAny/*modport*/ ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { // VAR for now, but V3LinkCells may call setIfcaeRef on it later
                          $$ = $5; VARDECL(VAR); VARIO(NONE);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstIfaceRefDType{$<fl>2, $<fl>4, "", *$2, *$4};
                          VARDTYPE(dtp); VARIOANSI();
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $6, $7)); }
        |       ਪੋਰਟ_Dir_Net_E yINTERFACE                           ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; GRAMMARP->createGenericIface($3, $4, $5); }
        |       ਪੋਰਟ_Dir_Net_E yINTERFACE      '.' idAny/*modport*/ ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5; GRAMMARP->createGenericIface($5, $6, $7, $<fl>4, *$4); }
        //
        |       ਪੋਰਟ_Dir_Net_E yINTERCONNECT signingE rangeListE ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5;
                          BBUNSUP($<fl>2, "Unsupported: interconnect");
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); VARIOANSI();
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $6, $7)); }
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
        |       ਪੋਰਟ_Dir_Net_E ਅੰਕੜੇ_type           ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $4, $5)); }
        |       ਪੋਰਟ_Dir_Net_E ਅੰਕੜੇ_type           ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE '=' const_ਵਿਅੰਜਕ
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          ਜੇ (AstVar* vp = VARDONEP($$, $4, $5)) { ਜੋੜੋ_ਅਗਲਾ_Null($$, vp); vp->valuep($7); } }
        |       ਪੋਰਟ_Dir_Net_E yVAR ਅੰਕੜੇ_type      ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $5, $6)); }
        |       ਪੋਰਟ_Dir_Net_E yVAR ਅੰਕੜੇ_type      ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE '=' const_ਵਿਅੰਜਕ
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          ਜੇ (AstVar* vp = VARDONEP($$, $5, $6)) { ਜੋੜੋ_ਅਗਲਾ_Null($$, vp); vp->valuep($8); } }
        |       ਪੋਰਟ_Dir_Net_E yVAR implicit_typeE ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $5, $6)); }
        |       ਪੋਰਟ_Dir_Net_E yVAR implicit_typeE ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE '=' const_ਵਿਅੰਜਕ
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          ਜੇ (AstVar* vp = VARDONEP($$, $5, $6)) { ਜੋੜੋ_ਅਗਲਾ_Null($$, vp); vp->valuep($8); } }
        |       ਪੋਰਟ_Dir_Net_E signing             ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3;
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $4, $5)); }
        |       ਪੋਰਟ_Dir_Net_E signing             ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE '=' const_ਵਿਅੰਜਕ
                        { $$ = $3;
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          ਜੇ (AstVar* vp = VARDONEP($$, $4, $5)) { ਜੋੜੋ_ਅਗਲਾ_Null($$, vp); vp->valuep($7); } }
        |       ਪੋਰਟ_Dir_Net_E signingE rangeList  ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4;
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $5, $6)); }
        |       ਪੋਰਟ_Dir_Net_E signingE rangeList  ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE '=' const_ਵਿਅੰਜਕ
                        { $$ = $4;
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          ਜੇ (AstVar* vp = VARDONEP($$, $5, $6)) { ਜੋੜੋ_ਅਗਲਾ_Null($$, vp); vp->valuep($8); } }
        |       ਪੋਰਟ_Dir_Net_E /*implicit*/        ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE
                        { $$ = $2; /*VARDTYPE-same*/ ਜੋੜੋ_ਅਗਲਾ_Null($$, VARDONEP($$, $3, $4)); }
        |       ਪੋਰਟ_Dir_Net_E /*implicit*/        ਪੋਰਟ_Sig variable_dimensionListE sigAttrListE '=' const_ਵਿਅੰਜਕ
                        { $$ = $2; /*VARDTYPE-same*/
                          ਜੇ (AstVar* vp = VARDONEP($$, $3, $4)) { ਜੋੜੋ_ਅਗਲਾ_Null($$, vp); vp->valuep($6); } }
        //                      //   IEEE: '.' port_identifier '(' [ expression ] ')'
        |       ਪੋਰਟ_Dir_Net_E /*implicit*/ '.' ਪੋਰਟ_Sig '(' ਵਿਅੰਜਕ ')'
                        { $$ = $3; DEL($5);
                          BBUNSUP($<fl>2, "Unsupported: complex ports (IEEE 1800-2023 23.2.2.1/2)"); }
        //                      // IEEE: part of (non-ansi) port_reference
        |       '{' ਪੋਰਟ_ਵਿਅੰਜਕ_List '}'
                        { $$ = $2; }
        ;

ਪੋਰਟ_Dir_Net_E:                    // IEEE: part of port, optional net type and/or direction
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { }
        //                      // Per spec, if direction given default the nettype.
        //                      // The higher level rule may override this VARDTYPE with one later in the parse.
        |       ਪੋਰਟ_direction                                  { VARDECL(ਪੋਰਟ); VARDTYPE_NDECL(nullptr); }
        |       ਪੋਰਟ_direction { VARDECL(ਪੋਰਟ); } net_type      { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        |       net_type                                        { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        ;

ਪੋਰਟ_decl_Net_E:                  // IEEE: part of port_declaration, optional net type
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { }
        |       net_type                                { } // net_type calls VARDECL
        ;

ਪੋਰਟ_Sig<nodep>:
                id/*ਪੋਰਟ*/
                        { $$ = new Ast_ਪੋਰਟ{$<fl>1, PINNUMINC(), *$1}; }
        |       idSVKwd
                        { $$ = new Ast_ਪੋਰਟ{$<fl>1, PINNUMINC(), *$1}; }
        ;

ਪੋਰਟ_ਵਿਅੰਜਕ_List<nodep>:  // IEEE: part of (non-ansi) port_reference
                ਪੋਰਟ_reference                          { $$ = $1; }
        |       ਪੋਰਟ_ਵਿਅੰਜਕ_List ',' ਪੋਰਟ_reference  { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

ਪੋਰਟ_reference<nodep>:  // IEEE: (non-ansi) port-reference
        //                      // IEEE: port_identifier constant_select
        //                      // constant_select ::= [ '[' constant_part_select_range ']' ]
                id/*port_identifier*/                   { $$ = nullptr; }  // UNSUP above here
        |       id/*port_identifier*/ part_select_range  { $$ = nullptr; DEL($2); }  // UNSUP above here
        ;

//**********************************************************************
// Interface headers

interface_declaration:          // IEEE: interface_declaration + interface_nonansi_header + interface_ansi_header:
        //                      // timeunits_declarationE is instead in interface_item
                intFront importsAndParametersE ports_ਤਾਰਾ_E ';'
                        interface_ਵਸਤੂ_List_E yENDINTERFACE ਸਿਰਾ_Label_E
                        { ਜੇ ($2) $1->ਜੋੜੋ_Stmtsp($2);
                          ਜੇ ($3) $1->ਜੋੜੋ_Stmtsp($3);
                          ਜੇ ($5) $1->ਜੋੜੋ_Stmtsp($5);
                          $1->hasParameterList($<flag>2); }
        |       yEXTERN intFront parameter_ਪੋਰਟ_list_E ports_ਤਾਰਾ_E ';'
                        { DEL($2->unlink_Fr_ਪਿੱਠ()); }
                        // We allow interfaces to be declared after instantiations, so harmless
        ;

intFront<ਗੰਢ_Modulep>:
                yINTERFACE lifetimeE idAny/*new_interface*/
                        { $$ = new AstIface{$<fl>3, *$3, PARSEP->libname()};
                          $$->ਵਿੱਚ_Library(true);
                          $$->lifetime($2);
                          PARSEP->rootp()->ਜੋੜੋ_Modulesp($$); }
        |       intFront sigAttrScope                   { $$ = $1; }
        ;

interface_ਵਸਤੂ_List_E<nodep>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       interface_ਵਸਤੂ_List                      { $$ = $1; }
        ;

interface_ਵਸਤੂ_List<nodep>:
                interface_ਵਸਤੂ                          { $$ = $1; }
        |       interface_ਵਸਤੂ_List interface_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

interface_ਵਸਤੂ<nodep>:          // IEEE: interface_item + non_port_interface_item
                ਪੋਰਟ_declaration ';'                    { $$ = $1; }
        //                      // IEEE: non_port_interface_item
        //                      // IEEE: generate_region
        |       i_generate_region                       { $$ = $1; }
        |       interface_or_generate_ਵਸਤੂ              { $$ = $1; }
        |       ਪ੍ਰੋਗਰਾਮ_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within interface decls"); }
        //                      // IEEE 1800-2017: modport_item
        //                      // See instead old 2012 position in interface_or_generate_item
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within interface decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // See note in interface_or_generate item
        |       module_common_ਵਸਤੂ                      { $$ = $1; }
        ;

interface_or_generate_ਵਸਤੂ<nodep>:  // ==IEEE: interface_or_generate_item
        //                      // module_common_item in interface_item, as otherwise duplicated
        //                      // with module_or_generate_item's module_common_item
                modport_declaration                     { $$ = $1; }
        |       extern_tf_declaration                   { $$ = $1; }
        ;

//**********************************************************************
// Program headers

anonymous_ਪ੍ਰੋਗਰਾਮ<nodep>:       // ==IEEE: anonymous_program
        //                      // See the spec - this doesn't change the scope, items still go up "top"
                y_ਪ੍ਰੋਗਰਾਮ ';' anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List_E yENDPROGRAM
                        { $$ = nullptr;
                         BBUNSUP($<fl>1, "Unsupported: Anonymous programs");
                         VARDTYPE_NDECL(nullptr);
                         DEL($3); }
        ;

anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List_E<nodep>:     // IEEE: { anonymous_program_item }
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List              { $$ = $1; }
        ;

anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List<nodep>:      // IEEE: { anonymous_program_item }
                anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ                  { $$ = $1; }
        |       anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

anonymous_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ<nodep>:  // ==IEEE: anonymous_program_item
                task_declaration                        { $$ = $1; }
        |       function_declaration                    { $$ = $1; }
        |       class_declaration                       { $$ = $1; }
        //                      // IEEE-2023: Added: interface_class_declaration
        //                      // interface_class_declaration is part of class_declaration
        |       covergroup_declaration                  { $$ = $1; }
        //                      // class_constructor_declaration is part of function_declaration
        |       ';'                                     { $$ = nullptr; }
        ;

ਪ੍ਰੋਗਰਾਮ_declaration:            // IEEE: program_declaration + program_nonansi_header + program_ansi_header:
        //                      // timeunits_declarationE is instead in program_item
                pgmFront parameter_ਪੋਰਟ_list_E ports_ਤਾਰਾ_E ';'
        /*cont*/    ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List_E yENDPROGRAM ਸਿਰਾ_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          ਜੇ ($2) $1->ਜੋੜੋ_Stmtsp($2);
                          ਜੇ ($3) $1->ਜੋੜੋ_Stmtsp($3);
                          ਜੇ ($5) $1->ਜੋੜੋ_Stmtsp($5);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>7, $1, $7); }
        |       yEXTERN pgmFront parameter_ਪੋਰਟ_list_E ports_ਤਾਰਾ_E ';'
                        { DEL($2->unlink_Fr_ਪਿੱਠ()); }
                        // We allow programs to be declared after instantiations, so harmless
        ;

pgmFront<ਗੰਢ_Modulep>:
                y_ਪ੍ਰੋਗਰਾਮ lifetimeE idAny/*new_program*/
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname(), AstModule::ਪ੍ਰੋਗਰਾਮ{}};
                          $$->lifetime($2);
                          $$->ਵਿੱਚ_Library(PARSEP->ਵਿੱਚ_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ਸਮਾਂ_ਆਖਰੀ_Unit());
                          PARSEP->rootp()->timeprecision_ਮਿਲਾਓ($$->fileline(),
                                                              PARSEP->ਸਮਾਂ_ਆਖਰੀ_Prec());
                          PARSEP->rootp()->ਜੋੜੋ_Modulesp($$); }
        ;

ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List_E<nodep>:       // ==IEEE: [{ program_item }]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List                        { $$ = $1; }
        ;

ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List<nodep>:        // ==IEEE: { program_item }
                ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ                            { $$ = $1; }
        |       ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ_List ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ           { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ<nodep>:            // ==IEEE: program_item
                ਪੋਰਟ_declaration ';'                    { $$ = $1; }
        |       non_ਪੋਰਟ_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ                   { $$ = $1; }
        ;

non_ਪੋਰਟ_ਪ੍ਰੋਗਰਾਮ_ਵਸਤੂ<nodep>:   // ==IEEE: non_port_program_item
                continuous_ਸੌਂਪੋ                       { $$ = $1; }
        |       module_or_generate_ਵਸਤੂ_declaration     { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       concurrent_assertion_ਵਸਤੂ               { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       ਪ੍ਰੋਗਰਾਮ_generate_ਵਸਤੂ                   { $$ = $1; }
        ;

ਪ੍ਰੋਗਰਾਮ_generate_ਵਸਤੂ<nodep>:           // ==IEEE: program_generate_item
                loop_generate_construct                 { $$ = $1; }
        |       conditional_generate_construct          { $$ = $1; }
        |       generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       gen_ਵਸਤੂ_Begin                            { $$ = $1; }
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
                yMODPORT modport_ਵਸਤੂ_List ';'           { $$ = $2; }
        ;

modport_ਵਸਤੂ_List<nodep>:                // IEEE: part of modport_declaration
                modport_ਵਸਤੂ                            { $$ = $1; }
        |       modport_ਵਸਤੂ_List ',' modport_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

modport_ਵਸਤੂ<nodep>:                    // ==IEEE: modport_item
                idAny/*new-modport*/ '('
        /*mid*/         { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT);
                          GRAMMARP->m_modportProtoTasksp = nullptr; }
        /*cont*/    modportPortsDeclList ')'
                        { $$ = new AstModport{$<fl>1, *$1, $4};
                          // Append any prototype tasks from import/export as interface siblings
                          ਜੇ (GRAMMARP->m_modportProtoTasksp) {
                              $$ = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(
                                  GRAMMARP->m_modportProtoTasksp, $$);
                              GRAMMARP->m_modportProtoTasksp = nullptr;
                          } }
        ;

modportPortsDeclList<nodep>:
                modportPortsDecl                            { $$ = $1; }
        |       modportPortsDeclList ',' modportPortsDecl   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

// IEEE: modport_ports_declaration  + modport_simple_ports_declaration
//      + (modport_tf_ports_declaration+import_export) + modport_clocking_declaration
// We've expanded the lists each take to instead just have standalone ID ports.
// We track the type as with the V2k series of defines, then create as each ID is seen.
modportPortsDecl<nodep>:
        //                      // IEEE: modport_simple_ports_declaration
                ਪੋਰਟ_direction { GRAMMARP->m_modport_Imp_Exp_ਸਰਗਰਮ = false; } modport_Simple_ਪੋਰਟ_Or_TFPort { $$ = $3; }
        //                      // IEEE: modport_clocking_declaration
        |       yCLOCKING idAny/*clocking_identifier*/ { $$ = new AstModportClockingRef{$1, *$2}; }
        //                      // IEEE: yIMPORT modport_tf_port
        //                      // IEEE: yEXPORT modport_tf_port
        //                      // modport_tf_port expanded here
        |       yIMPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, false};
                          GRAMMARP->m_modport_Imp_Exp_ਸਰਗਰਮ = true;
                          GRAMMARP->m_modport_Imp_Exp_ਆਖਰੀ_Is_Export = false; }
        |       yEXPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, true};
                          GRAMMARP->m_modport_Imp_Exp_ਸਰਗਰਮ = true;
                          GRAMMARP->m_modport_Imp_Exp_ਆਖਰੀ_Is_Export = true; }
        |       yIMPORT ਵਿਧੀ_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), false};
                          GRAMMARP->m_modport_Imp_Exp_ਸਰਗਰਮ = true;
                          GRAMMARP->m_modport_Imp_Exp_ਆਖਰੀ_Is_Export = false;
                          // Import prototype: task should already exist in interface
                          // (from export or direct declaration). Delete the prototype.
                          DEL($2); }
        |       yEXPORT ਵਿਧੀ_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), true};
                          GRAMMARP->m_modport_Imp_Exp_ਸਰਗਰਮ = true;
                          GRAMMARP->m_modport_Imp_Exp_ਆਖਰੀ_Is_Export = true;
                          // Export: add prototype task to interface (as sibling of modport)
                          GRAMMARP->m_modportProtoTasksp
                              = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(GRAMMARP->m_modportProtoTasksp, $2); }
        // Continuations of above after a comma.
        //                      // IEEE: modport_simple_ports_declaration
        |       modport_Simple_ਪੋਰਟ_Or_TFPort                { $$ = $1; }
        ;

modport_Simple_ਪੋਰਟ_Or_TFPort<nodep>:// IEEE: modport_simple_port or modport_tf_port, depending what keyword was earlier
                idAny                                   { $$ = GRAMMARP->m_modport_Imp_Exp_ਸਰਗਰਮ ?
                                                                static_cast<Ast_ਗੰਢ*>(
                                                                  new AstModportFTaskRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_modport_Imp_Exp_ਆਖਰੀ_Is_Export} ) :
                                                                static_cast<Ast_ਗੰਢ*>(
                                                                  new AstModportVarRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_varIO} ); }
        |       '.' idAny '(' ')'                       { $$ = new AstModportVarRef{$<fl>2, *$2, GRAMMARP->m_varIO};
                                                          BBUNSUP($<fl>4, "Unsupported: Modport empty expression"); }
        |       '.' idAny '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstModportVarRef{$<fl>2, *$2, $4, GRAMMARP->m_varIO}; }
        ;

//************************************************
// Variable Declarations

genvar_declaration<nodep>:      // ==IEEE: genvar_declaration
                yGENVAR list_ਦਾ_genvar_identifiers ';'  { $$ = $2; }
        ;

list_ਦਾ_genvar_identifiers<nodep>:      // IEEE: list_of_genvar_identifiers (for declaration)
                genvar_identifierDecl                   { $$ = $1; }
        |       list_ਦਾ_genvar_identifiers ',' genvar_identifierDecl    { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

genvar_identifierDecl<varp>:            // IEEE: genvar_identifier (for declaration)
                idAny/*new-genvar_identifier*/ sigAttrListE
                        { VARRESET_NONLIST(GENVAR);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$<fl>1, VBasicDTypeKwd::INTEGER};
                          VARDTYPE(dtp);
                          $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        ;

parameter_declaration<nodep>:   // IEEE: local_ or parameter_declaration and type_parameter_declaration
        //                      // IEEE: yPARAMETER yTYPE list_of_type_assignments ';'
        //                      // Instead of list_of_type_assignments
        //                      // we use list_of_param_assignments because for port handling
        //                      // it already must accept types, so simpler to have code only one place
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                parameter_declarationFront list_ਦਾ_param_assignments            { $$ = $2; }
        |       parameter_declarationTypeFront list_ਦਾ_type_assignments         { $$ = $2; }
        ;

parameter_declarationFront:     // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset ਅੰਕੜੇ_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        ;

parameter_declarationTypeFront: // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/ VARDTYPE(new AstParseTypeDType{$2}); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        ;

parameter_ਪੋਰਟ_declaration_Front_E: // IEEE: local_ or parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset ਅੰਕੜੇ_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       implicit_typeE
                        { /*VARRESET-in-varParam*/
                          // Keep previous type to handle subsequent declarations.
                          // This rule is also used when the previous parameter is a type parameter
                          ਜੇ ($1) $1->v3error("parameter port declarations require 'parameter'"
                                              " keyword before implicit data types"
                                              " (IEEE 1800-2023 6.20.1/A.2.1.1)\n"
                                              + $1->warn_ਹੋਰ()
                                              + "... Suggest add 'parameter' before here");
                       }
        |       ਅੰਕੜੇ_type                               { /*VARRESET-in-varParam*/ VARDTYPE($1); }
        ;

parameter_ਪੋਰਟ_declaration_Type_Front_E: // IEEE: parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstParseTypeDType{$2}; VARDTYPE(dtp); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        |       yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstParseTypeDType{$1}; VARDTYPE(dtp); }
        |       yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstParseTypeDType{$1, $2}; VARDTYPE(dtp); }
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
                          ਜੇ (GRAMMARP->m_netStrengthp) {
                              VL_DO_CLEAR(delete GRAMMARP->m_netStrengthp, GRAMMARP->m_netStrengthp = nullptr);
                          }
                          GRAMMARP->ਸੈੱਟ_Net_Delay(nullptr); }
        ;

net_declarationFront:           // IEEE: beginning of net_declaration
                net_declRESET net_type driveStrengthE net_scalaredE net_ਅੰਕੜੇ_Type_E
                        { VARDTYPE_NDECL($5);
                          GRAMMARP->ਸੈੱਟ_Net_Strength($3); }
        |       net_declRESET yINTERCONNECT signingE rangeListE
                        { BBUNSUP($<fl>2, "Unsupported: interconnect");
                          VARDECL(WIRE);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        ;

net_declRESET:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { VARRESET_NONLIST(UNKNOWN); }
        ;

net_scalaredE:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { }
        //                      //UNSUP: ySCALARED/yVECTORED ignored
        |       ySCALARED                               { }
        |       yVECTORED                               { }
        ;

net_ਅੰਕੜੇ_Type_E<ਗੰਢ_DTypep>:
        //                      // If there's a SV data type there shouldn't be a delay on this wire
        //                      // Otherwise #(...) can't be determined to be a delay or parameters
        //                      // Submit this as a footnote to the committee
                var_ਅੰਕੜੇ_type                           { $$ = $1; }
        |       signingE rangeList delay_controlE
                        { $$ = GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$2->fileline(), LOGIC, $1},
                                                                    $2, true);
                          GRAMMARP->ਸੈੱਟ_Net_Delay($3); }  // not implicit
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC, $1}; }  // not implicit
        |       /*implicit*/ delay_controlE
                        { $$ = new AstBasicDType{CRELINE(), LOGIC};
                          GRAMMARP->ਸੈੱਟ_Net_Delay($1); }  // not implicit
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

ਪੋਰਟ_direction:                 // ==IEEE: port_direction + tf_port_direction
        //                      // IEEE 19.8 just "input" FIRST forces type to wire - we'll ignore that here
        //                      // Only used for ANSI declarations
                y_ਨਿਵੇਸ਼                                  { VARIO(ਨਿਵੇਸ਼); }
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

ਪੋਰਟ_direction_Reset:            // IEEE: port_direction that starts a port_declaraiton
        //                      // Used only for declarations outside the port list
                y_ਨਿਵੇਸ਼                                  { VARRESET_NONLIST(UNKNOWN); VARIO(ਨਿਵੇਸ਼); }
        |       yOUTPUT                                 { VARRESET_NONLIST(UNKNOWN); VARIO(OUTPUT); }
        |       yINOUT                                  { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT); }
        |       yREF                                    { VARRESET_NONLIST(UNKNOWN); VARIO(REF); }
        |       yCONST__REF yREF                        { VARRESET_NONLIST(UNKNOWN); VARIO(CONSTREF); }
        ;

ਪੋਰਟ_declaration<nodep>:        // ==IEEE: port_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port, tf_port_declaration, data_declarationVarFront
        //
        //                      // IEEE: inout_declaration
        //                      // IEEE: input_declaration
        //                      // IEEE: output_declaration
        //                      // IEEE: ref_declaration
                ਪੋਰਟ_direction_Reset ਪੋਰਟ_decl_Net_E ਅੰਕੜੇ_type
        /*mid*/         { VARDTYPE($3); }
        /*cont*/    list_ਦਾ_variable_decl_assignments                   { $$ = $5; }
        |       ਪੋਰਟ_direction_Reset ਪੋਰਟ_decl_Net_E yVAR ਅੰਕੜੇ_type
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_ਦਾ_variable_decl_assignments                   { $$ = $6; }
        |       ਪੋਰਟ_direction_Reset ਪੋਰਟ_decl_Net_E yVAR implicit_typeE
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_ਦਾ_variable_decl_assignments                   { $$ = $6; }
        |       ਪੋਰਟ_direction_Reset ਪੋਰਟ_decl_Net_E signingE rangeList
        /*mid*/         { Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                              new AstBasicDType{$4->fileline(), LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_ਦਾ_variable_decl_assignments                   { $$ = $6; }
        |       ਪੋਰਟ_direction_Reset ਪੋਰਟ_decl_Net_E signing
        /*mid*/         { Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $3};
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_ਦਾ_variable_decl_assignments                   { $$ = $5; }
        |       ਪੋਰਟ_direction_Reset ਪੋਰਟ_decl_Net_E /*implicit*/
        /*mid*/         { VARDTYPE_NDECL(nullptr); /*default_nettype*/ }
        /*cont*/    list_ਦਾ_variable_decl_assignments                   { $$ = $4; }
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
                          AstIfaceRefDType* ਅਚਲ dtp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          VARDTYPE(dtp); }
        /*cont*/    mpInstnameList
                        { $$ = VARDONEP($5, nullptr, nullptr); DEL($5); }
        //UNSUP: strengthSpecE for udp_instantiations
        ;

tf_ਪੋਰਟ_declaration<nodep>:     // ==IEEE: tf_port_declaration
        //                      // Used inside function; followed by ';'
        //                      // SEE ALSO port_declaration, port, data_declarationVarFront
        //
                ਪੋਰਟ_direction_Reset      ਅੰਕੜੇ_type      { VARDTYPE($2); }  list_ਦਾ_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       ਪੋਰਟ_direction_Reset      implicit_typeE { VARDTYPE_NDECL($2); }  list_ਦਾ_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       ਪੋਰਟ_direction_Reset yVAR ਅੰਕੜੇ_type      { VARDTYPE($3); }  list_ਦਾ_tf_variable_identifiers ';'
                        { $$ = $5; }
        |       ਪੋਰਟ_direction_Reset yVAR implicit_typeE { VARDTYPE($3); }  list_ਦਾ_tf_variable_identifiers ';'
                        { $$ = $5; }
        ;

integer_atom_type<basicDTypep>: // ==IEEE: integer_atom_type
                yBYTE                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::BYTE}; }
        |       ySHORTINT                               { $$ = new AstBasicDType{$1, VBasicDTypeKwd::SHORTINT}; }
        |       yINT                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INT}; }
        |       yLONGINT                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::LONGINT}; }
        |       yINTEGER                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INTEGER}; }
        |       y_ਸਮਾਂ                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::ਸਮਾਂ}; }
        ;

integer_ਸਦਿਸ਼_type<basicDTypep>:       // ==IEEE: integer_atom_type
                y_ਦੋਆਧਾਰੀ_ਅੰਕ                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ}; }
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

simple_typeNoRef<ਗੰਢ_DTypep>:   // IEEE: simple_type without idType
        //                      // IEEE: integer_type
                integer_atom_type                       { $$ = $1; }
        |       integer_ਸਦਿਸ਼_type                     { $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        //
        //                      // { generate_block_identifer ... } '.'
        //                      // Need to determine if generate_block_identifier can be lex-detected
        ;

ਅੰਕੜੇ_type<ਗੰਢ_DTypep>:          // ==IEEE: data_type
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                ਅੰਕੜੇ_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idType packed_dimensionListE
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idType parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

ਅੰਕੜੇ_type_Any<ਗੰਢ_DTypep>:       // ==IEEE: data_type (accepting idAny)
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                ਅੰਕੜੇ_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idAny packed_dimensionListE
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idAny parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

ਅੰਕੜੇ_type_Basic<ਗੰਢ_DTypep>:             // IEEE: part of data_type
                integer_ਸਦਿਸ਼_type signingE rangeListE { $1->ਸੈੱਟ_Signed_State($2); $$ = GRAMMARP->ਜੋੜੋ_Range($1, $3, true); }
        //                      // IEEE: packed_dimension: unsized_dimension, legal only in DPI import
        |       integer_ਸਦਿਸ਼_type signingE '[' ']'
                        { $1->ਸੈੱਟ_Signed_State($2);
                          $$ = new AstUnsizedArrayDType{$3, VFlag_ਬੱਚਾ_DType{}, $1}; }
        |       integer_atom_type signingE              { $1->ਸੈੱਟ_Signed_State($2); $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        ;

ਅੰਕੜੇ_type_No_Ref<ਗੰਢ_DTypep>:             // ==IEEE: data_type, excluding class_type etc references
                ਅੰਕੜੇ_type_Basic                          { $$ = $1; }
        |       struct_unionDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpsu" + cvt_ਮੰਜ਼ਿਲ_Str(GRAMMARP->s_type_Imp_ਅੰਕ++),
                                                           VFlag_ਬੱਚਾ_DType{}, $1}, $2, true); }
        |       enumDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpenum" + cvt_ਮੰਜ਼ਿਲ_Str(GRAMMARP->s_type_Imp_ਅੰਕ++),
                                                           VFlag_ਬੱਚਾ_DType{}, $1}, $2, true); }
        |       ySTRING
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::STRING}; }
        |       yCHANDLE
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CHANDLE}; }
        |       yEVENT
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::EVENT}; v3Global.ਸੈੱਟ_Has_Events(); }
        |       type_referenceDecl                      { $$ = $1; }
        //                      // IEEE: class_scope: see data_type above
        //                      // IEEE: class_type: see data_type above
        //                      // IEEE: ps_covergroup: see data_type above
        //                      // Rules overlap virtual_interface_declaration
        |       yVIRTUAL__INTERFACE yINTERFACE ਅੰਕੜੇ_type_Virtual
                        { $$ = $3; }
        |       yVIRTUAL__anyID                ਅੰਕੜੇ_type_Virtual
                        { $$ = $2; }
        ;

ਅੰਕੜੇ_type_Virtual<ਗੰਢ_DTypep>:           // ==IEEE: data_type after yVIRTUAL [ yINTERFACE ]
        //                      // Parameters here are SV2009
                idAny/*interface*/
                        { AstIfaceRefDType* ਅਚਲ ifrefp = new AstIfaceRefDType{$<fl>1, "", *$1};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ '.' idAny/*modport*/
                        { AstIfaceRefDType* ਅਚਲ ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_ਮੁੱਲ_assignment_Class
                        { AstIfaceRefDType* ਅਚਲ ifrefp = new AstIfaceRefDType{$<fl>1, nullptr, "", *$1, "", $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_ਮੁੱਲ_assignment_Class '.' idAny/*modport*/
                        { AstIfaceRefDType* ਅਚਲ ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>4, "", *$1, *$4, $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        ;

ਅੰਕੜੇ_type_or_void<ਗੰਢ_DTypep>:  // ==IEEE: data_type_or_void
                ਅੰਕੜੇ_type_Any                            { $$ = $1; }
        |       yVOID
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CVOID}; }
        ;

var_ਅੰਕੜੇ_type<ਗੰਢ_DTypep>:              // ==IEEE: var_data_type
                ਅੰਕੜੇ_type                               { $$ = $1; }
        |       yVAR ਅੰਕੜੇ_type                          { $$ = $2; }
        |       yVAR implicit_typeE                     { $$ = $2; }
        ;

type_referenceBoth<ਗੰਢ_Exprp>:          // IEEE: type_reference
                yTYPE__ETC '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'
                        { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::TYPEID, $3}; }
        ;

type_referenceDecl<ਗੰਢ_DTypep>:         // IEEE: type_reference (as a data type for declaration)
                yTYPE__ETC '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'
                        { $$ = new AstRefDType{$1, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{}, $3}; }
        ;

type_referenceEq<ਗੰਢ_Exprp>:            // IEEE: type_reference (as an ==/!== expression)
                yTYPE__EQ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'
                        { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::TYPEID, $3}; }
        ;

struct_unionDecl<ਗੰਢ_UOr_Struct_DTypep>:  // IEEE: part of data_type
        //                      // packedSigningE is NOP for unpacked
                ySTRUCT        packedSigningE '{'
        /*mid*/         { $<ਗੰਢ_UOr_Struct_DTypep>$ = new AstStructDType{$1, $2}; }
        /*cont*/    struct_union_member_List_ਸਿਰਾ
                        { $$ = $<ਗੰਢ_UOr_Struct_DTypep>4; $$->ਜੋੜੋ_Membersp($5); }
        |       yUNION taggedSoftE packedSigningE '{'
        /*mid*/         { $<ਗੰਢ_UOr_Struct_DTypep>$ = new AstUnionDType{$1, $2 == tagged_SOFT, $2 == tagged_TAGGED, $3}; }
        /*cont*/    struct_union_member_List_ਸਿਰਾ
                        { $$ = $<ਗੰਢ_UOr_Struct_DTypep>5; $$->ਜੋੜੋ_Membersp($6); }
        ;

struct_union_member_List_ਸਿਰਾ<memberDTypep>: // IEEE: { struct_union_member } '}'
                struct_union_memberList '}'                     { $$ = $1; }
        //
        |       struct_union_memberList error '}'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error '}'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_memberList<memberDTypep>: // IEEE: { struct_union_member }
                struct_union_member                             { $$ = $1; }

        |       struct_union_memberList struct_union_member     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        //
        |       struct_union_memberList error ';'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error ';'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_member<memberDTypep>:     // ==IEEE: struct_union_member
                random_qualifierE ਅੰਕੜੇ_type_or_void
        /*mid*/         { GRAMMARP->m_memDTypep = $2;  // As a list follows, need to attach this dtype to each member.
                          GRAMMARP->m_memRand = $1.randAttr(); }
        /*cont*/    list_ਦਾ_member_decl_assignments ';'
                        { $$ = $4;
                          DEL(GRAMMARP->m_memDTypep); GRAMMARP->m_memDTypep = nullptr;
                          GRAMMARP->m_memRand = VRandAttr::NONE; }
        |       vl_ਟੈਗ                                   { $$ = nullptr; }
        ;

list_ਦਾ_member_decl_assignments<memberDTypep>: // Derived from IEEE: list_of_variable_decl_assignments
                member_decl_assignment                  { $$ = $1; }
        |       list_ਦਾ_member_decl_assignments ',' member_decl_assignment      { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

member_decl_assignment<memberDTypep>:   // Derived from IEEE: variable_decl_assignment
        //                      // At present we allow only packed structures/unions.
        //                      // So this is different from variable_decl_assignment
                idAny variable_dimensionListE
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlag_ਬੱਚਾ_DType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_ਰੁੱਖ(true) : nullptr),
                                                                        $2, false),
                                                  nullptr};
                          $$->rand(GRAMMARP->m_memRand);
                          PARSEP->ਟੈਗ_Nodep($$);
                        }
        |       idAny variable_dimensionListE '=' variable_decl_ਵਿਅੰਜਕ
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlag_ਬੱਚਾ_DType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_ਰੁੱਖ(true) : nullptr),
                                                                        $2, false),
                                                  $4};
                          $$->rand(GRAMMARP->m_memRand);
                          PARSEP->ਟੈਗ_Nodep($$);
                        }
        |       idSVKwd                                 { $$ = nullptr; }
        //
        //                      // IEEE: "dynamic_array_variable_identifier '[' ']' [ '=' dynamic_array_new ]"
        //                      // Matches above with variable_dimensionE = "[]"
        //                      // IEEE: "class_variable_identifier [ '=' class_new ]"
        //                      // variable_dimensionE must be empty
        //                      // Pushed into variable_declExpr:dynamic_array_new
        ;

list_ਦਾ_variable_decl_assignments<varp>:        // ==IEEE: list_of_variable_decl_assignments
                variable_decl_assignment                { $$ = $1; }
        |       list_ਦਾ_variable_decl_assignments ',' variable_decl_assignment  { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

variable_decl_assignment<varp>: // ==IEEE: variable_decl_assignment
                id variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       id variable_dimensionListE sigAttrListE '=' variable_decl_ਵਿਅੰਜਕ
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

list_ਦਾ_tf_variable_identifiers<nodep>: // ==IEEE: list_of_tf_variable_identifiers
                tf_variable_identifier                  { $$ = $1; }
        |       list_ਦਾ_tf_variable_identifiers ',' tf_variable_identifier      { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

tf_variable_identifier<varp>:           // IEEE: part of list_of_tf_variable_identifiers
                id variable_dimensionListE sigAttrListE ਵਿਅੰਜਕ_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          ਜੇ ($4) Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(
                                      $$, new Ast_ਸੌਂਪੋ{$4->fileline(), new AstParseRef{$<fl>1, *$1}, $4}); }
        ;

variable_decl_ਵਿਅੰਜਕ<nodep>:               // IEEE: part of variable_decl_assignment - rhs of expr
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       dynamic_array_new                       { $$ = $1; }
        |       class_newNoScope                        { $$ = $1; }
        ;

variable_dimensionListE<ਗੰਢ_Rangep>:    // IEEE: variable_dimension + empty
                /*empty*/                               { $$ = nullptr; }
        |       variable_dimensionList                  { $$ = $1; }
        ;

variable_dimensionList<ਗੰਢ_Rangep>:     // IEEE: variable_dimension + empty
                variable_dimension                      { $$ = $1; }
        |       variable_dimensionList variable_dimension       { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($2); }
        ;

variable_dimension<ਗੰਢ_Rangep>: // ==IEEE: variable_dimension
        //                      // IEEE: unsized_dimension
                '[' ']'                                 { $$ = new AstUnsizedRange{$1}; }
        //                      // IEEE: unpacked_dimension
        |       anyrange                                { $$ = $1; }
        //                      // IEEE: unpacked_dimension (if const_expr)
        //                      // IEEE: associative_dimension (if data_type)
        //                      // Can't tell which until see if expr is data type or not
        |       '[' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ']'                  { $$ = new AstBracketRange{$1, $2}; }
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
        |       yPACKED signingE                        { $$ = $2; ਜੇ ($$ == VSigning::NOSIGN) $$ = VSigning::UNSIGNED; }
        ;

//************************************************
// enum

// IEEE: part of data_type
enumDecl<ਗੰਢ_DTypep>:
                yENUM enum_ਅਧਾਰ_type_E '{' enum_nameList '}'
                        { $$ = new AstEnumDType{$1, VFlag_ਬੱਚਾ_DType{}, $2, $4}; }
        ;

enum_ਅਧਾਰ_type_E<ਗੰਢ_DTypep>:    // IEEE: enum_base_type
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { $$ = new AstBasicDType{CRELINE(), VBasicDTypeKwd::INT}; }
        //                      // Not in spec, but obviously "enum [1:0]" should work
        //                      // implicit_type expanded, without empty
        //                      // Note enum base types are always packed data types
        |       signingE rangeList
                        { $$ = GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        //
        |       integer_atom_type signingE
                        { $1->ਸੈੱਟ_Signed_State($2); $$ = $1; }
        |       integer_ਸਦਿਸ਼_type signingE rangeListE
                        { $1->ਸੈੱਟ_Signed_State($2); $$ = GRAMMARP->ਜੋੜੋ_Range($1, $3, true); }
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
        |       enum_nameList ',' enum_name_declaration { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

enum_name_declaration<enumItemp>:   // ==IEEE: enum_name_declaration
                idAny/*enum_identifier*/ enumNameRangeE enum_Name_ਸ਼ੁਰੂ_E
                        { $$ = new Ast_Enum_ਵਸਤੂ{$<fl>1, *$1, $2, $3}; }
        ;

enumNameRangeE<rangep>:          // IEEE: second part of enum_name_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { $$ = nullptr; }
        |       '[' intnumAsConst ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstConst($1, $2->ਮੰਜ਼ਿਲ_SInt() - 1)}; DEL($2); }
        |       '[' intnumAsConst ':' intnumAsConst ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

enum_Name_ਸ਼ੁਰੂ_E<ਗੰਢ_Exprp>:      // IEEE: third part of enum_name_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       '=' const_ਵਿਅੰਜਕ                           { $$ = $2; }
        ;

intnumAsConst<constp>:
                yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        ;

//************************************************
// Typedef

ਅੰਕੜੇ_declaration<nodep>:        // ==IEEE: data_declaration
        //                      // VARRESET can't be called here - conflicts
                ਅੰਕੜੇ_declaration_Var                     { $$ = $1; }
        |       type_declaration                        { $$ = $1; }
        |       package_import_declaration              { $$ = $1; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        //                      // 1800-2009:
        |       nettype_declaration                     { $$ = $1; }
        |       vl_ਟੈਗ                                   { $$ = nullptr; }
        ;

class_property<nodep>:          // ==IEEE: class_property, which is {property_qualifier} data_declaration
                memberQualListE ਅੰਕੜੇ_declaration_Var_Class
                        { $$ = $2; $1.apply_ਮੰਜ਼ਿਲ_Nodes($2); }
        |       memberQualListE type_declaration
                        { $$ = $2; ਜੇ (VN_IS($2, Typedef)) $1.apply_ਮੰਜ਼ਿਲ_Nodes(VN_AS($2, Typedef)); }
        //                      // UNSUP: Import needs to apply local/protected from memberQualList, and error on others
        |       memberQualListE package_import_declaration
                        { $$ = $2; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        ;

ਅੰਕੜੇ_declaration_Var<varp>:      // IEEE: part of data_declaration
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                ਅੰਕੜੇ_declaration_Var_Front list_ਦਾ_variable_decl_assignments ';'  { $$ = $2; }
        ;

ਅੰਕੜੇ_declaration_Var_Class<varp>: // IEEE: part of data_declaration (for class_property)
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                ਅੰਕੜੇ_declaration_Var_Front_Class list_ਦਾ_variable_decl_assignments ';'     { $$ = $2; }
        ;

ਅੰਕੜੇ_declaration_Var_Front:       // IEEE: part of data_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port_declaration, tf_port_declaration, port
        //
        //                      // Expanded: "constE yVAR lifetimeE data_type"
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE ਅੰਕੜੇ_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT};
                          VARDTYPE(dtp); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/ VARLIFE($2);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                              new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); }
        //
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
        |       yCONST__ETC yVAR lifetimeE ਅੰਕੜੇ_type
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstConstDType{$<fl>2, VFlag_ਬੱਚਾ_DType{}, $4};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstConstDType{$<fl>2, VFlag_ਬੱਚਾ_DType{},
                                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE signingE rangeList
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstConstDType{$<fl>2, VFlag_ਬੱਚਾ_DType{},
                                  GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $4}, $5, true)};
                          VARDTYPE(dtp); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       /**/                  ਅੰਕੜੇ_type
                        { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        |       /**/        lifetime  ਅੰਕੜੇ_type
                        { VARRESET_NONLIST(VAR); VARLIFE($1); VARDTYPE($2); }
        |       yCONST__ETC lifetimeE ਅੰਕੜੇ_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstConstDType{$<fl>1, VFlag_ਬੱਚਾ_DType{}, $3};
                          VARDTYPE(dtp); }
        //                      // = class_new is in variable_decl_assignment
        ;

ਅੰਕੜੇ_declaration_Var_Front_Class:  // IEEE: part of data_declaration (for class_property)
        //                      // VARRESET called before this rule
        //                      // yCONST is removed, added to memberQual rules
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE ਅੰਕੜੇ_type        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE                  { VARRESET_NONLIST(VAR); VARLIFE($2); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp);
                          VARLIFE($2); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       ਅੰਕੜੇ_type                       { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        //                      // lifetime is removed, added to memberQual rules to avoid conflict
        //                      // yCONST is removed, added to memberQual rules to avoid conflict
        //                      // = class_new is in variable_decl_assignment
        ;

nettype_declaration<nodep>:  // IEEE: nettype_declaration/net_type_declaration
        //                      // Union of data_typeAny and nettype_identifier matching
                yNETTYPE ਅੰਕੜੇ_type_No_Ref
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>3, *$3); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE ਅੰਕੜੇ_type_No_Ref
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
        |       yNETTYPE packageClassScopeE idAny parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE packageClassScopeE idAny parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/
        /*cont*/   yWITH__ETC packageClassScopeE id/*tf_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype with"); }
        ;

implicit_typeE<ਗੰਢ_DTypep>:             // IEEE: part of *data_type_or_implicit
        //                      // Also expanded in data_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { $$ = nullptr; }
        |       signingE rangeList
                        { $$ = GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        ;

assertion_variable_declaration<nodep>:  // IEEE: assertion_variable_declaration
        //                      // IEEE: var_data_type expanded
                var_ਅੰਕੜੇ_type                        { VARRESET_NONLIST(VAR); VARDTYPE_NDECL($1); }
        /*cont*/    list_ਦਾ_variable_decl_assignments ';'     { $$ = $3; }
        ;

type_declaration<nodep>:        // ==IEEE: type_declaration
                                // Data_type expanded
                yTYPEDEF ਅੰਕੜੇ_type_No_Ref
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { Ast_ਗੰਢ_DType* ਅਚਲ dtp = $2;
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, dtp, $4); }

        // IEEE 1800-2023 6.18 typedef: dotted or arrayed type identifier
        // Handles interface typedef references like if0.rq_t and if0[0].rq_t (arrays allowed after first component)
        |       yTYPEDEF idDottedOrArrayed
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* ਅਚਲ ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* ਅਚਲ varp = VARDONEA($<fl>3, *$3, $4, $5);
                // idDottedOrArrayed produces Dot/SelBit tree for hierarchical refs like if0[0].rq_t
                varp->valuep($2);
                $$ = varp; }

        // IEEE 1800-2023 6.18 typedef with hierarchical type identifier
        // Special-case array on first component requiring a '.' after ']' to disambiguate from packed dims
        // Examples: typedef if0[0].rq_t my_t; typedef if0[0].x_if.rq_t my_t;
        |       yTYPEDEF id '[' ਵਿਅੰਜਕ ']' '.' id_Dotted_Sel_ਹੋਰ
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* ਅਚਲ ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* ਅਚਲ varp = VARDONEA($<fl>8, *$8, $9, $10);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ arrp = new Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{$3, new AstParseRef{$<fl>2, *$2, nullptr, nullptr}, $4};
                varp->valuep(new AstDot{$6, false, arrp, $7});
                $$ = varp; }

        |       yTYPEDEF packageClassScope idAny packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>3, *$3, $2, nullptr};
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->createArray(refp, $4, true);
                          $$ = GRAMMARP->createTypedef($<fl>5, *$5, $7, dtp, $6); }
        |       yTYPEDEF packageClassScope idAny parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>3, *$3, $2, $4};
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->createArray(refp, $5, true);
                          $$ = GRAMMARP->createTypedef($<fl>6, *$6, $8, dtp, $7); }

        // Type alias without packed dimensions: typedef existing_t new_t;
        |       yTYPEDEF idAny idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, refp, $4); }

        // IEEE 1800-2023 6.18.2 typedef with packed dimensions on an existing type identifier
        // Disambiguated from interface array access by requiring ':' inside the brackets
        // (applies to both plain identifiers and type identifiers)
        |       yTYPEDEF id '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_ਗੰਢ_Range* ਅਚਲ rangep = new AstRange{$3, $4, $6};
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->createArray(refp, ਜੋੜੋ_ਅਗਲਾ_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        // Same as above but for type identifiers (parameter types, etc.)
        |       yTYPEDEF idType '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_ਗੰਢ_Range* ਅਚਲ rangep = new AstRange{$3, $4, $6};
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->createArray(refp, ਜੋੜੋ_ਅਗਲਾ_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        |       yTYPEDEF idAny parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, nullptr, $3};
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->createArray(refp, $4, true);
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       dtypeAttrList                           { $$ = $1; }
        ;

dtypeAttrList<nodep>:
                dtypeAttr                               { $$ = $1; }
        |       dtypeAttrList dtypeAttr                 { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

dtypeAttr<nodep>:
                y_VL_ਜਨਤਕ                              { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DT_ਜਨਤਕ}; }
        ;

vl_ਟੈਗ:                          // verilator tag handling
                y_VL_ਟੈਗ                                 { ਜੇ (PARSEP->ਟੈਗ_Nodep()) PARSEP->ਟੈਗ_Nodep()->ਟੈਗ(*$1); }
        ;

//************************************************
// Module Items

module_ਵਸਤੂ_List_E<nodep>:        // IEEE: Part of module_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       module_ਵਸਤੂ_List                         { $$ = $1; }
        ;

module_ਵਸਤੂ_List<nodep>:         // IEEE: Part of module_declaration
                module_ਵਸਤੂ                             { $$ = $1; }
        |       module_ਵਸਤੂ_List module_ਵਸਤੂ             { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

module_ਵਸਤੂ<nodep>:             // ==IEEE: module_item
                ਪੋਰਟ_declaration ';'                    { $$ = $1; }
        |       non_ਪੋਰਟ_module_ਵਸਤੂ                    { $$ = $1; }
        ;

non_ਪੋਰਟ_module_ਵਸਤੂ<nodep>:    // ==IEEE: non_port_module_item
                generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       gen_ਵਸਤੂ_Begin                            { $$ = $1; }
        |       module_or_generate_ਵਸਤੂ                 { $$ = $1; }
        |       specify_ਖੰਡ                           { $$ = $1; }
        |       specparam_declaration                   { $$ = $1; }
        |       ਪ੍ਰੋਗਰਾਮ_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within module decls"); }
        |       module_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: module decls within module decls"); }
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within module decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // Verilator specific
        |       vl_Sc_ਖੰਡ                               { $$ = $1; }
        |       y_VL_HIER_ਖੰਡ                          { $$ = new AstPragma{$1, VPragmaType::HIER_ਖੰਡ}; }
        |       yVL_INLINE_MODULE                       { $$ = new AstPragma{$1, VPragmaType::INLINE_MODULE}; }
        |       yVL_NO_INLINE_MODULE                    { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_MODULE}; }
        |       y_VL_ਜਨਤਕ_MODULE                       { $$ = new AstPragma{$1, VPragmaType::ਜਨਤਕ_MODULE}; v3Global.dpi(true); }
        ;

vl_Sc_ਖੰਡ<nodep>:  // Verilator-specific `systemc_* blocks
                yaSCHDR     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR, *$1};  }
        |       yaSCHDRP    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR_POST, *$1}; }
        |       yaSCINT     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::INT, *$1}; }
        |       yaSCIMP     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP, *$1}; }
        |       yaSCIMPH    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP_HDR, *$1}; }
        |       yaSCCTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::CTOR, *$1}; }
        |       yaSCDTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::DTOR, *$1}; }
        ;


module_or_generate_ਵਸਤੂ<nodep>: // ==IEEE: module_or_generate_item
        //                      // IEEE: parameter_override
                yDEFPARAM list_ਦਾ_defparam_assignments ';'      { $$ = $2; }
        //                      // IEEE: gate_instantiation + udp_instantiation + module_instantiation
        //                      // not here, see etcInst in module_common_item
        //                      // We joined udp & module definitions, so this goes here
        |       combinational_body                      { $$ = $1; }
        //                      // This module_common_item shared with
        //                      // interface_or_generate_item:module_common_item
        |       module_common_ਵਸਤੂ                      { $$ = $1; }
        ;

module_common_ਵਸਤੂ<nodep>:      // ==IEEE: module_common_item
                module_or_generate_ਵਸਤੂ_declaration     { $$ = $1; }
        //                      // IEEE: interface_instantiation
        //                      // + IEEE: program_instantiation
        //                      // + module_instantiation from module_or_generate_item
        |       etcInst                                 { $$ = $1; }
        |       assertion_ਵਸਤੂ                          { $$ = $1; }
        |       bind_directive                          { $$ = $1; }
        |       continuous_ਸੌਂਪੋ                       { $$ = $1; }
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

continuous_ਸੌਂਪੋ<nodep>:       // IEEE: continuous_assign
                y_ਸੌਂਪੋ driveStrengthE delay_controlE ਸੌਂਪੋ_List ';'
                        { $$ = $4;
                          STRENGTH_LIST($4, $2);
                          DELAY_LIST($4, $3); }
        ;

initial_construct<nodep>:       // IEEE: initial_construct
                yINITIAL stmt                      { $$ = new AstInitial{$1, $2}; }
        ;


net_alias<nodep>:               // IEEE: net_alias
                yALIAS variable_lvalue aliasEqList ';'
                        { $2->ਜੋੜੋ_ਅਗਲਾ($3);
                          $$ = new AstAlias{$1, $2}; }
        ;

aliasEqList<ਗੰਢ_Exprp>:                    // IEEE: part of net_alias
                '=' variable_lvalue                     { $$ = $2; }
        |       aliasEqList '=' variable_lvalue         { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

final_construct<nodep>:         // IEEE: final_construct
                yFINAL stmt                        { $$ = new AstFinal{$1, $2}; }
        ;

module_or_generate_ਵਸਤੂ_declaration<nodep>:     // ==IEEE: module_or_generate_item_declaration
                package_or_generate_ਵਸਤੂ_declaration    { $$ = $1; }
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
                yDEFAULT yDISABLE yIFF ਵਿਅੰਜਕ/*expression_or_dist*/ ';'
                        { $$ = new AstDefaultDisable{$1, $4}; }
        ;

bind_directive<nodep>:          // ==IEEE: bind_directive + bind_target_scope
        //                      // ';' - Note IEEE grammar is wrong, includes extra ';'
        //                      // - it's already in module_instantiation
        //                      // We merged the rules - id may be a bind_target_instance or
        //                      // module_identifier or interface_identifier
                yBIND bind_ਨਿਸ਼ਾਨਾ_instance bind_instantiation   { $$ = new AstBind{$<fl>2, *$2, $3}; }
        |       yBIND bind_ਨਿਸ਼ਾਨਾ_instance ':' bind_ਨਿਸ਼ਾਨਾ_instance_list bind_instantiation
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: Bind with instance list"); DEL($5); }
        ;

bind_ਨਿਸ਼ਾਨਾ_instance_list:      // ==IEEE: bind_target_instance_list
                bind_ਨਿਸ਼ਾਨਾ_instance                    { }
        |       bind_ਨਿਸ਼ਾਨਾ_instance_list ',' bind_ਨਿਸ਼ਾਨਾ_instance      { }
        ;

bind_ਨਿਸ਼ਾਨਾ_instance<strp>:     // ==IEEE: bind_target_instance
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
                yGENERATE ~c~gen_ਵਸਤੂ_List yENDGENERATE   { $$ = $2; }
        |       yGENERATE yENDGENERATE                  { $$ = nullptr; }
        ;

c_generate_region<nodep>:  // IEEE: generate_region (for checkers)
                BISONPRE_COPY(generate_region,{s/~c~/c_/g})     // {copied}
        ;

i_generate_region<nodep>:  // IEEE: generate_region (for interface)
                BISONPRE_COPY(generate_region,{s/~c~/i_/g})     // {copied}
        ;

generate_ਖੰਡ_or_null<nodep>:  // IEEE: generate_block_or_null (called from gencase/genif/genfor)
        //      ';'             // is included in
        //                      // IEEE: generate_block
        //                      // Must always return a BEGIN node, or nullptr - see GenFor construction
                ~c~generate_ਵਸਤੂ
                        { $$ = $1 ? (new Ast_Gen_ਖੰਡ{$1->fileline(), "", $1, true}) : nullptr; }
        |       ~c~gen_ਵਸਤੂ_Begin                         { $$ = $1; }
        ;

c_generate_ਖੰਡ_or_null<nodep>:  // IEEE: generate_block_or_null (for checkers)
                BISONPRE_COPY(generate_ਖੰਡ_or_null,{s/~c~/c_/g})      // {copied}
        ;

gen_ਵਸਤੂ_Begin<nodep>:            // IEEE: part of generate_block
                yBEGIN ~c~gen_ਵਸਤੂ_List y_ਸਿਰਾ              { $$ = new Ast_Gen_ਖੰਡ{$1, "", $2, false}; }
        |       yBEGIN y_ਸਿਰਾ                             { $$ = nullptr; }
        |       id yP_COLON__BEGIN yBEGIN ~c~gen_ਵਸਤੂ_List y_ਸਿਰਾ ਸਿਰਾ_Label_E
                        { $$ = new Ast_Gen_ਖੰਡ{$<fl>1, *$1, $4, false};
                          GRAMMARP->ਸਿਰਾ_Label($<fl>6, *$1, $6); }
        |       id yP_COLON__BEGIN yBEGIN y_ਸਿਰਾ ਸਿਰਾ_Label_E
                        { $$ = new Ast_Gen_ਖੰਡ{$<fl>1, *$1, nullptr, false};
                          GRAMMARP->ਸਿਰਾ_Label($<fl>5, *$1, $5); }
        |       yBEGIN ':' idAny ~c~gen_ਵਸਤੂ_List y_ਸਿਰਾ ਸਿਰਾ_Label_E
                        { $$ = new Ast_Gen_ਖੰਡ{$<fl>3, *$3, $4, false};
                          GRAMMARP->ਸਿਰਾ_Label($<fl>6, *$3, $6); }
        |       yBEGIN ':' idAny y_ਸਿਰਾ ਸਿਰਾ_Label_E
                        { $$ = new Ast_Gen_ਖੰਡ{$<fl>3, *$3, nullptr, false};
                          GRAMMARP->ਸਿਰਾ_Label($<fl>5, *$3, $5); }
        ;

c_gen_ਵਸਤੂ_Begin<nodep>:  // IEEE: part of generate_block (for checkers)
                BISONPRE_COPY(gen_ਵਸਤੂ_Begin,{s/~c~/c_/g})                // {copied}
        ;

i_gen_ਵਸਤੂ_Begin<nodep>:  // IEEE: part of generate_block (for interfaces)
                BISONPRE_COPY(gen_ਵਸਤੂ_Begin,{s/~c~/i_/g})                // {copied}
        ;

gen_ਵਸਤੂ_Or_Begin<nodep>:          // Not in IEEE, but our begin isn't under generate_item
                ~c~generate_ਵਸਤੂ                        { $$ = $1; }
        |       ~c~gen_ਵਸਤੂ_Begin                         { $$ = $1; }
        ;

c_gen_ਵਸਤੂ_Or_Begin<nodep>:  // (for checkers)
                BISONPRE_COPY(gen_ਵਸਤੂ_Or_Begin,{s/~c~/c_/g})      // {copied}
        ;

i_gen_ਵਸਤੂ_Or_Begin<nodep>:  // (for interfaces)
                interface_ਵਸਤੂ                          { $$ = $1; }
        |       i_gen_ਵਸਤੂ_Begin                          { $$ = $1; }
        ;

gen_ਵਸਤੂ_List<nodep>:
                ~c~gen_ਵਸਤੂ_Or_Begin                       { $$ = $1; }
        |       ~c~gen_ਵਸਤੂ_List ~c~gen_ਵਸਤੂ_Or_Begin        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

c_gen_ਵਸਤੂ_List<nodep>:  // (for checkers)
                BISONPRE_COPY(gen_ਵਸਤੂ_List,{s/~c~/c_/g})         // {copied}
        ;

i_gen_ਵਸਤੂ_List<nodep>:  // (for interfaces)
                BISONPRE_COPY(gen_ਵਸਤੂ_List,{s/~c~/i_/g})         // {copied}
        ;

generate_ਵਸਤੂ<nodep>:           // IEEE: module_or_interface_or_generate_item
        //                      // Only legal when in a generate under a module (or interface under a module)
                module_or_generate_ਵਸਤੂ                 { $$ = $1; }
        //                      // Only legal when in a generate under an interface
        //UNSUP interface_or_generate_item              { $$ = $1; }
        //                      // IEEE: checker_or_generate_item
        //                      // Only legal when in a generate under a checker
        //                      // so below in c_generate_item
        ;

c_generate_ਵਸਤੂ<nodep>:  // IEEE: generate_item (for checkers)
                checker_or_generate_ਵਸਤੂ                { $$ = $1; }
        ;

conditional_generate_construct<nodep>:  // ==IEEE: conditional_generate_construct
                yCASE '(' ਵਿਅੰਜਕ_Type_Compare ')' ~c~case_generate_ਵਸਤੂ_List yENDCASE
                        { $$ = new AstGenCase{$1, $3, $5}; }
        |       yIF '(' ਵਿਅੰਜਕ_Type_Compare ')' ~c~generate_ਖੰਡ_or_null %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = new AstGenIf{$1, $3, $5, nullptr}; }
        |       yIF '(' ਵਿਅੰਜਕ_Type_Compare ')' ~c~generate_ਖੰਡ_or_null yELSE ~c~generate_ਖੰਡ_or_null
                        { $$ = new AstGenIf{$1, $3, $5, $7}; }
        ;

c_conditional_generate_construct<nodep>:  // IEEE: conditional_generate_construct (for checkers)
                BISONPRE_COPY(conditional_generate_construct,{s/~c~/c_/g})      // {copied}
        ;

loop_generate_construct<nodep>: // ==IEEE: loop_generate_construct
                yFOR '(' genvar_initialization ';' ਵਿਅੰਜਕ ';' genvar_iteration ')' ~c~generate_ਖੰਡ_or_null
                        { // Convert BEGIN(...) to BEGIN(GENFOR(...)), as we need the BEGIN to hide the local genvar
                          Ast_Gen_ਖੰਡ* lowerp = VN_CAST($9, Gen_ਖੰਡ);
                          UASSERT_OBJ(!$9 || lowerp, $9, "Child of GENFOR should have been begin");
                          Ast_ਗੰਢ* ਅਚਲ itemsp = lowerp && lowerp->itemsp() ? lowerp->itemsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ() : nullptr;
                          Ast_Gen_ਖੰਡ* ਅਚਲ blkp = new Ast_Gen_ਖੰਡ{$1, lowerp ? lowerp->name() : "", nullptr, true};
                          // V3LinkDot detects BEGIN(GENFOR(...)) as a special case
                          Ast_ਗੰਢ* initp = $3;
                          Ast_ਗੰਢ* ਅਚਲ varp = $3;
                          ਜੇ (VN_IS(varp, Var)) {  // Genvar
                                initp = varp->nextp();
                                initp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();  // Detach 2nd from varp, make 1st init
                                blkp->ਜੋੜੋ_Itemsp(varp);
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
                parse_Ref_ਅਧਾਰ '=' ਵਿਅੰਜਕ                     { $$ = new Ast_ਸੌਂਪੋ{$2, $1, $3}; }
        |       yGENVAR genvar_identifierDecl '=' const_ਵਿਅੰਜਕ
                        { $$ = $2; Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>($$,
                                       new Ast_ਸੌਂਪੋ{$3, new AstVarRef{$2->fileline(), $2, VAccess::WRITE}, $4}); }
        ;

genvar_iteration<nodep>:        // ==IEEE: genvar_iteration
                parse_Ref_ਅਧਾਰ '='          ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, $3}; }
        |       parse_Ref_ਅਧਾਰ yP_PLUSEQ    ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new Ast_ਜੋੜੋ{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_MINUSEQ   ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstSub{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_TIMESEQ   ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstMul{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_DIVEQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstDiv{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_MODEQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstModDiv{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_ANDEQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstAnd{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_OREQ      ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstOr{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_XOREQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstXor{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_SLEFTEQ   ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new Ast_ਖਿਸਕਾਓ_L{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_SRIGHTEQ  ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new Ast_ਖਿਸਕਾਓ_R{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        |       parse_Ref_ਅਧਾਰ yP_SSRIGHTEQ ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new Ast_ਖਿਸਕਾਓ_RS{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $3}}; }
        //                      // inc_or_dec_operator
        |       yP_PLUSPLUS   parse_Ref_ਅਧਾਰ
                        { $$ = new Ast_ਸੌਂਪੋ{$1, $2, new Ast_ਜੋੜੋ{$1, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true),
                                                                new AstConst{$1, AstConst::String_ਮੰਜ਼ਿਲ_Parse{}, "'b1"}}}; }
        |       yP_MINUSMINUS parse_Ref_ਅਧਾਰ
                        { $$ = new Ast_ਸੌਂਪੋ{$1, $2, new AstSub{$1, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true),
                                                                new AstConst{$1, AstConst::String_ਮੰਜ਼ਿਲ_Parse{}, "'b1"}}}; }
        |       parse_Ref_ਅਧਾਰ yP_PLUSPLUS
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new Ast_ਜੋੜੋ{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true),
                                                                new AstConst{$2, AstConst::String_ਮੰਜ਼ਿਲ_Parse{}, "'b1"}}}; }
        |       parse_Ref_ਅਧਾਰ yP_MINUSMINUS
                        { $$ = new Ast_ਸੌਂਪੋ{$2, $1, new AstSub{$2, $1->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true),
                                                                new AstConst{$2, AstConst::String_ਮੰਜ਼ਿਲ_Parse{}, "'b1"}}}; }
        ;

case_generate_ਵਸਤੂ_List<genCaseItemp>:  // IEEE: { case_generate_itemList }
                ~c~case_generate_ਵਸਤੂ                   { $$ = $1; }
        |       ~c~case_generate_ਵਸਤੂ_List ~c~case_generate_ਵਸਤੂ         { $$ = $1; $1->ਜੋੜੋ_ਅਗਲਾ($2); }
        ;

c_case_generate_ਵਸਤੂ_List<genCaseItemp>:  // IEEE: { case_generate_item } (for checkers)
                BISONPRE_COPY(case_generate_ਵਸਤੂ_List,{s/~c~/c_/g})      // {copied}
        ;

case_generate_ਵਸਤੂ<genCaseItemp>:      // ==IEEE: case_generate_item
                caseCondList colon ~c~generate_ਖੰਡ_or_null    { $$ = new Ast_Gen_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       yDEFAULT colon ~c~generate_ਖੰਡ_or_null        { $$ = new Ast_Gen_Case_ਵਸਤੂ{$1, nullptr, $3}; }
        |       yDEFAULT ~c~generate_ਖੰਡ_or_null              { $$ = new Ast_Gen_Case_ਵਸਤੂ{$1, nullptr, $2}; }
        ;

c_case_generate_ਵਸਤੂ<genCaseItemp>:  // IEEE: case_generate_item (for checkers)
                BISONPRE_COPY(case_generate_ਵਸਤੂ,{s/~c~/c_/g})  // {copied}
        ;

//************************************************
// Assignments and register declarations

ਸੌਂਪੋ_List<alwaysp>:
                ਸੌਂਪੋ_One                               { $$ = $1; }
        |       ਸੌਂਪੋ_List ',' ਸੌਂਪੋ_One                { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

ਸੌਂਪੋ_One<alwaysp>:
                variable_lvalue '=' ਵਿਅੰਜਕ                { Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$2, $1, $3};
                                                          $$ = new AstAlways{ap}; }
        ;

delay_or_event_controlE<nodep>:  // IEEE: delay_or_event_control plus empty
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        |       event_control                           { $$ = $1; }
        |       yREPEAT '(' ਵਿਅੰਜਕ ')' event_control
                        { $$ = $5; BBUNSUP($1, "Unsupported: repeat event control"); }
        ;

delay_controlE<delayp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        ;

delay_control<delayp>:   //== IEEE: delay_control
                '#' delay_ਮੁੱਲ
                        { $$ = new AstDelay{$<fl>1, $2, false}; }
        |       '#' '(' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; }
        |       '#' '(' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false};
                          $$->fallDelay($5); }
        |       '#' '(' minTypMax ',' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; $$->fallDelay($5); RISEFALLDLYUNSUP($7); DEL($7); }
        ;

delay_ਮੁੱਲ<ਗੰਢ_Exprp>:         // ==IEEE:delay_value
        //                      // IEEE: ps_identifier
                idClass                                 { $$ = $1; }
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       ਸਮਾਂ_ਅੰਕ_Adjusted                         { $$ = $1; }
        |       y1_ਕਦਮ                                  { $$ = new AstConst{$<fl>1, AstConst::One_ਕਦਮ{}}; }
        ;

minTypMax<ਗੰਢ_Exprp>:           // IEEE: mintypmax_expression and constant_mintypmax_expression
                ਵਿਅੰਜਕ                               { $$ = $1; }
        |       ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ { $$ = $3; MINTYPMAXDLYUNSUP($3); DEL($1); DEL($5); }
        ;

minTypMaxE<ਗੰਢ_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       minTypMax                               { $$ = $1; }
        ;

netSigList<varp>:               // IEEE: list_of_port_identifiers
                netSig                                  { $$ = $1; }
        |       netSigList ',' netSig                   { $$ = $1; $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

netSig<varp>:                   // IEEE: net_decl_assignment -  one element from list_of_port_identifiers
                netId variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       netId variable_dimensionListE sigAttrListE '=' ਵਿਅੰਜਕ
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          AstDelay* ਅਚਲ delayp = $$->delayp() ? $$->delayp()->unlink_Fr_ਪਿੱਠ() : nullptr;
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ assignp = new Ast_ਸੌਂਪੋ_W{$4, new AstParseRef{$<fl>1, *$1}, $5, delayp};
                          ਜੇ (GRAMMARP->m_netStrengthp) assignp->strengthSpecp(GRAMMARP->m_netStrengthp->clone_ਰੁੱਖ(false));
                          Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>($$, new AstAlways{assignp}); }
        ;

netId<strp>:
                id/*new-net*/                           { $$ = $1; $<fl>$ = $<fl>1; }
        |       idSVKwd                                 { $$ = $1; $<fl>$ = $<fl>1; }
        ;

sigAttrScope:
                y_VL_ਜਨਤਕ_FLAT_RW_ON_SNS attr_event_control
                                                        { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ਜਨਤਕ_FLAT_RW);
                                                          v3Global.dpi(true); DEL($2); }
        |       y_VL_ਜਨਤਕ_ON                           { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ਜਨਤਕ); }
        |       y_VL_ਜਨਤਕ_FLAT_ON                      { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ਜਨਤਕ_FLAT); }
        |       y_VL_ਜਨਤਕ_FLAT_RD_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ਜਨਤਕ_FLAT_RD); }
        |       y_VL_ਜਨਤਕ_FLAT_RW_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_ਜਨਤਕ_FLAT_RW); }
        |       y_VL_ਜਨਤਕ_OFF                          { GRAMMARP->ਸੈੱਟ_Scoped_Sig_Attr(nullptr); }
        ;

sigAttrListE<nodep>: // Scoped Attributes are added to explicit attributes
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       sigAttrList                             { $$ = $1; }
        ;

sigAttrList<nodep>:
                sigAttr                                 { $$ = $1; }
        |       sigAttrList sigAttr                     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

sigAttr<nodep>:
                yVL_CLOCKER                             { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_NO_CLOCKER                          { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_CLOCK_ENABLE                        { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_FORCEABLE                           { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FORCEABLE}; }
        |       y_VL_ਜਨਤਕ                              { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_ਜਨਤਕ}; v3Global.dpi(true); }
        |       y_VL_ਜਨਤਕ_FLAT                         { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_ਜਨਤਕ_FLAT}; v3Global.dpi(true); }
        |       y_VL_ਜਨਤਕ_FLAT_RD                      { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_ਜਨਤਕ_FLAT_RD}; v3Global.dpi(true); }
        |       y_VL_ਜਨਤਕ_FLAT_RW attr_event_controlE  { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_ਜਨਤਕ_FLAT_RW}; v3Global.dpi(true); DEL($2); }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_SC_BIGUINT                          { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_SC_BIGUINT}; }
        |       yVL_SC_BV                               { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_SC_BV}; }
        |       yVL_SFORMAT                             { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_SFORMAT}; }
        |       y_VL_ਵੰਡੋ_VAR                           { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_ਵੰਡੋ_VAR}; }
        |       yVL_FSM_ARC_INCL_COND                   { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO              { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO_EXPAND       { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND}; }
        |       y_VL_FSM_ARC_INCL_COND_ਭਰਿਆ              { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_ਭਰਿਆ}; }
        |       yVL_FSM_STATE                           { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_STATE}; }
        |       yVL_FSM_STATE_AUTO                      { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_STATE_AUTO}; }
        |       yVL_FSM_STATE_AUTO_EXPAND               { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_STATE_AUTO_EXPAND}; }
        |       y_VL_FSM_STATE_ਭਰਿਆ                      { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::VAR_FSM_STATE_ਭਰਿਆ}; }
        ;

rangeListE<ਗੰਢ_Rangep>:         // IEEE: [{packed_dimension}]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       rangeList                               { $$ = $1; }
        ;

rangeList<ਗੰਢ_Rangep>:          // IEEE: {packed_dimension}
                anyrange                                { $$ = $1; }
        |       rangeList anyrange                      { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($2); }
        ;

anyrange<ਗੰਢ_Rangep>:
                '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'         { $$ = new AstRange{$1, $2, $4}; }
        ;

part_select_rangeList<ਗੰਢ_Pre_Selp>:  // IEEE: part_select_range (as used after function calls)
                part_select_range                        { $$ = $1; }
        |       part_select_rangeList part_select_range  { $$ = GRAMMARP->scrubSel($1, $2); }
        ;

part_select_range<ਗੰਢ_Pre_Selp>:
                '[' ਵਿਅੰਜਕ ']'
                        { $$ = new Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{$1, new AstParseHolder{$1}, $2}; }
        |       '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                        { $$ = new Ast_Sel_ਕੱਢੋ{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' ਵਿਅੰਜਕ yP_PLUSCOLON const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSelPlus{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' ਵਿਅੰਜਕ yP_MINUSCOLON const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSelMinus{$1, new AstParseHolder{$1}, $2, $4}; }
        ;

packed_dimensionListE<ਗੰਢ_Rangep>:      // IEEE: [{ packed_dimension }]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       packed_dimensionList                    { $$ = $1; }
        ;

packed_dimensionList<ਗੰਢ_Rangep>:       // IEEE: { packed_dimension }
                packed_dimension                        { $$ = $1; }
        |       packed_dimensionList packed_dimension   { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($2); }
        ;

packed_dimension<ਗੰਢ_Rangep>:   // ==IEEE: packed_dimension
                anyrange                                { $$ = $1; }
        |       '[' ']'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: [] dimensions"); }
        ;

//************************************************
// Parameters

param_assignment<varp>:         // ==IEEE: param_assignment
        //                      // IEEE: constant_param_expression
        //                      // constant_param_expression: '$' is in expr
                id/*new-parameter*/ variable_dimensionListE sigAttrListE ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          ਜੇ ($4) $$->valuep($4);
                          ਨਹੀਂਤਾਂ ਜੇ (!GRAMMARP->m_pinAnsi)
                              $$->v3warn(PARAMNODEFAULT, "Parameter without default requires"
                                         " ANSI-style parameter list (IEEE 1800-2023 6.20.1): "
                                         << $$->prettyNameQ()); }
        ;

list_ਦਾ_param_assignments<varp>:        // ==IEEE: list_of_param_assignments
                param_assignment                        { $$ = $1; }
        |       list_ਦਾ_param_assignments ',' param_assignment  { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

type_assignment<varp>:          // ==IEEE: type_assignment
        //                      // note exprOrDataType being a data_type is only for yPARAMETER yTYPE
        //                      // Using exprOrDataType allows hierarchical refs like if0.rq_t
        //                      // which get resolved to types during linking
                idAny/*new-parameter*/ sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        |       idAny/*new-parameter*/ sigAttrListE '=' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          // V3LinkParse will wrap this in RequireDType when creating ParamTypeDType
                          $$->valuep($4); }
        ;

list_ਦਾ_type_assignments<varp>:         // ==IEEE: list_of_type_assignments
                type_assignment                         { $$ = $1; }
        |       list_ਦਾ_type_assignments ',' type_assignment
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

list_ਦਾ_defparam_assignments<nodep>:    //== IEEE: list_of_defparam_assignments
                defparam_assignment                     { $$ = $1; }
        |       list_ਦਾ_defparam_assignments ',' defparam_assignment
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

defparam_assignment<nodep>:     // ==IEEE: defparam_assignment
                defparamIdRangeList '.' defparamIdRange '=' ਵਿਅੰਜਕ
                        { $$ = new AstDefParam{$4, $1, *$3, $5}; }
        |       defparamIdRange '=' ਵਿਅੰਜਕ
                        { $$ = nullptr; BBUNSUP($2, "Unsupported: defparam with no dot");
                          DEL($3); }
        ;

defparamIdRangeList<ਗੰਢ_Exprp>:  // IEEE: part of defparam_assignment
                defparam_Id_Range_ਵਿਅੰਜਕ                     { $$ = $1; }
        |       defparamIdRangeList '.' defparam_Id_Range_ਵਿਅੰਜਕ
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

defparam_Id_Range_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:  // IEEE: part of defparam_assignment
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
                idInst parameter_ਮੁੱਲ_assignment_Inst_E
        /*mid*/         { INSTPREP($<fl>1, *$1, $2); }
        /*cont*/    instnameList ';'
                        { $$ = $4;
                          GRAMMARP->m_impliedDecl = false;
                          ਜੇ (GRAMMARP->m_instParamp) {
                              VL_DO_CLEAR(GRAMMARP->m_instParamp->delete_ਰੁੱਖ(),
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
        |       mpInstnameList ',' mpInstnameParen      { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

mpInstnameParen<nodep>:         // Similar to instnameParen, but for modport instantiations which have no parenthesis
                id instRangeListE sigAttrListE          { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        ;

instnameList<nodep>:
                instnameParen                           { $$ = $1; }
        |       instnameList ',' instnameParen          { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

instnameParen<nodep>:
                id instRangeListE '(' instPinListE ')'
                        { $$ = GRAMMARP->createCell($<fl>1, *$1, $4, $2); }
        ;

instnameParenUdpn<nodep>:  // IEEE: part of udp_instance when no name_of_instance
                '(' instPinListE ')'  // When UDP has empty name, unpacked dimensions must not be used
                        { $$ = GRAMMARP->createCell($<fl>1, "", $2, nullptr); }
        ;

instRangeListE<ਗੰਢ_Rangep>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       instRangeList                           { $$ = $1; }
        ;

instRangeList<ਗੰਢ_Rangep>:
                instRange                               { $$ = $1; }
        |       instRangeList instRange                 { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

instRange<ਗੰਢ_Rangep>:
                '[' const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstSub{$1, $2, new AstConst{$1, 1}}, true}; }
        |       '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

instParamListE<pinp>:
                { GRAMMARP->pin_ਧੱਕੋ(); } instParamItListE   { $$ = $2; GRAMMARP->pin_ਕੱਢੋ(CRELINE()); }
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
                inst_Param_ਵਸਤੂ                           { $$ = $1; }
        |       instParamItList ',' inst_Param_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

instPinItListE<pinp>:           // IEEE: list_of_port_connections
                inst_Pin_ਵਸਤੂ_E                            { $$ = $1; }
        |       instPinItListE ',' inst_Pin_ਵਸਤੂ_E         { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

inst_Param_ਵਸਤੂ<pinp>:            // IEEE: named_parameter_assignment + empty
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
        |       '.' idAny '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'
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
        |       ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type
                        { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1); DEL($5);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        ;

inst_Pin_ਵਸਤੂ_E<pinp>:             // IEEE: named_port_connection + empty
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
        |       '.' idAny '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $4};
                          $$->svDotName(true); }
        //UNSUP '.' idAny '(' pev_expr ':' expr ')'     { }
        //UNSUP '.' idAny '(' pev_expr ':' expr ':' expr ')' { }
        //
        |       ਵਿਅੰਜਕ                                    { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP expr ':' expr                           { }
        //UNSUP expr ':' expr ':' expr                  { }
        ;

//************************************************
// EventControl lists

attr_event_controlE<senTreep>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       attr_event_control                      { $$ = $1; }
        ;

attr_event_control<senTreep>:   // ==IEEE: event_control
                '@' '(' event_ਵਿਅੰਜਕ ')'            { $$ = new Ast_Sen_ਰੁੱਖ{$1, $3}; }
        |       '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_ਰੁੱਖ_Dot_ਤਾਰਾ($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_ਰੁੱਖ_Dot_ਤਾਰਾ($1); }
        ;

event_control<senTreep>:        // ==IEEE: event_control
        // UNSUP: Needs alignment with IEEE event_control and clocking_event
                '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_ਰੁੱਖ_Dot_ਤਾਰਾ($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_ਰੁੱਖ_Dot_ਤਾਰਾ($1); }
        //                      // IEEE: clocking_event
        |       '@' '(' event_ਵਿਅੰਜਕ ')'            { $$ = new Ast_Sen_ਰੁੱਖ{$1, $3}; }
        //                      // IEEE: hierarchical_event_identifier
        //                      // UNSUP below should be idClassSel
        |       '@' senitemVar                          { $$ = new Ast_Sen_ਰੁੱਖ{$1, $2}; } /* For events only */
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

event_ਵਿਅੰਜਕ<senItemp>:     // IEEE: event_expression - split over several
        //UNSUP                 // Below are all removed
                senitem                                 { $$ = $1; }
        |       event_ਵਿਅੰਜਕ yOR senitem            { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        |       event_ਵਿਅੰਜਕ ',' senitem            { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); } /* Verilog 2001 */
        //UNSUP                 // Above are all removed, replace with:
        //UNSUP ev_expr                                 { $$ = $1; }
        //UNSUP event_expression ',' ev_expr %prec yOR  { $$ = addNextNull($1, $3); }
        ;

senitem<senItemp>:              // IEEE: part of event_expression, non-'OR' ',' terms
                senitem_ਕਿਨਾਰਾ                             { $$ = $1; }
        |       ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$<fl>1, VEdgeType::ET_ਬਦਲਿਆ, $1}; }
        |       ਵਿਅੰਜਕ yIFF ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$<fl>1, VEdgeType::ET_ਬਦਲਿਆ, $1, $3}; }
        ;

senitemVar<senItemp>:
                idClassSel
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1->fileline(), VEdgeType::ET_ਬਦਲਿਆ, $1}; }
        ;

senitem_ਕਿਨਾਰਾ<senItemp>:          // IEEE: part of event_expression
                yPOSEDGE ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1, VEdgeType::ET_POSEDGE, $2}; }
        |       yNEGEDGE ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1, VEdgeType::ET_NEGEDGE, $2}; }
        |       y_ਕਿਨਾਰਾ ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1, VEdgeType::ET_BOTHEDGE, $2}; }
        |       yPOSEDGE ਵਿਅੰਜਕ yIFF ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1, VEdgeType::ET_POSEDGE, $2, $4}; }
        |       yNEGEDGE ਵਿਅੰਜਕ yIFF ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1, VEdgeType::ET_NEGEDGE, $2, $4}; }
        |       y_ਕਿਨਾਰਾ ਵਿਅੰਜਕ yIFF ਵਿਅੰਜਕ
                        { $$ = new Ast_Sen_ਵਸਤੂ{$1, VEdgeType::ET_BOTHEDGE, $2, $4}; }
        ;

//************************************************
// Statements

seq_ਖੰਡ<beginp>:               // ==IEEE: seq_block
        //                      // IEEE doesn't allow declarations in unnamed blocks, but several simulators do.
        //                      // So need AstBegin's even if unnamed to scope variables down
                yBEGIN ਸ਼ੁਰੂ_Label_E ਖੰਡ_Decl_List_E stmtListE y_ਸਿਰਾ ਸਿਰਾ_Label_E
                        {
                            $$ = new AstBegin{$1, $2 ? *$2 : "", nullptr, false};
                            GRAMMARP->ਸਿਰਾ_Label($<fl>6, $$, $6);
                            $$->ਜੋੜੋ_Declsp($3);
                            $$->ਜੋੜੋ_Stmtsp($4);
                        }
        ;

seq_ਖੰਡ_Pre_Id<beginp>:          // IEEE: seq_block, but called with leading ID
                id yP_COLON__BEGIN yBEGIN ਖੰਡ_Decl_List_E stmtListE y_ਸਿਰਾ ਸਿਰਾ_Label_E
                        {
                            $$ = new AstBegin{$3, *$1, nullptr, false};
                            GRAMMARP->ਸਿਰਾ_Label($<fl>7, $$, $7);
                            $$->ਜੋੜੋ_Declsp($4);
                            $$->ਜੋੜੋ_Stmtsp($5);
                        }
        ;

par_ਖੰਡ_Join<joinType>:
                yJOIN       { $$ = VJoinType::JOIN; }
        |       yJOIN_ANY   { $$ = VJoinType::JOIN_ANY; }
        |       yJOIN_NONE  { $$ = VJoinType::JOIN_NONE; }
        ;

par_ਖੰਡ<forkp>:               // ==IEEE: par_block
                yFORK ਸ਼ੁਰੂ_Label_E ਖੰਡ_Decl_List_E stmtListE par_ਖੰਡ_Join ਸਿਰਾ_Label_E
                        {
                            $$ = new AstFork{$1, $5, $2 ? *$2 : ""};
                            GRAMMARP->ਸਿਰਾ_Label($<fl>6, $$, $6);
                            $$->ਜੋੜੋ_Declsp($3);
                            $$->ਜੋੜੋ_Forksp(V3ParseGrammar::wrap_ਵਿੱਚ_Begin($4));
                        }
        ;

par_ਖੰਡ_Pre_Id<forkp>:          // ==IEEE: par_block but called with leading ID
                id yP_COLON__FORK yFORK ਖੰਡ_Decl_List_E stmtListE par_ਖੰਡ_Join ਸਿਰਾ_Label_E
                        {
                            $$ = new AstFork{$3, $6, *$1};
                            GRAMMARP->ਸਿਰਾ_Label($<fl>7, $$, $7);
                            $$->ਜੋੜੋ_Declsp($4);
                            $$->ਜੋੜੋ_Forksp(V3ParseGrammar::wrap_ਵਿੱਚ_Begin($5));
                        }
            ;

ਖੰਡ_Decl_List_E<nodep>:      // IEEE: [ block_item_declaration ]
                /*empty*/                               { $$ = nullptr; }
        |       ਖੰਡ_Decl_List_E ਖੰਡ_ਵਸਤੂ_declaration   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

ਖੰਡ_ਵਸਤੂ_declaration<nodep>:  // ==IEEE: block_item_declaration
                ਅੰਕੜੇ_declaration                        { $$ = $1; }
        |       parameter_declaration ';'               { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

stmtListE<ਗੰਢ_Stmtp>:
                /*empty*/                               { $$ = nullptr; }
        |       stmtList                                { $$ = $1; }
        ;

stmtList<ਗੰਢ_Stmtp>:
                stmt                               { $$ = $1; }
        |       stmtList stmt                      { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        //
        |       stmtList error ';'                      { $$ = $1; }  // LCOV_EXCL_LINE
        ;

stmt<ਗੰਢ_Stmtp>:                    // IEEE: statement + statement_or_null + seq_block + par_block
                statement_ਵਸਤੂ                          { $$ = $1; }
        //                      // S05 block creation rule
        |       id/*block_identifier*/ ':' statement_ਵਸਤੂ       { $$ = new AstBegin{$<fl>1, *$1, $3, false}; }
        //                      // from _or_null
        |       ';'                                     { $$ = nullptr; }
        //                      // labeled par_block/seq_block with leading ':'
        |       seq_ਖੰਡ_Pre_Id                          { $$ = $1; }
        |       par_ਖੰਡ_Pre_Id                          { $$ = $1; }
        ;

statement_ਵਸਤੂ<ਗੰਢ_Stmtp>:          // IEEE: statement_item
        //                      // IEEE: operator_assignment
                foperator_assignment ';'                { $$ = $1; }
        //
        //                      // IEEE: blocking_assignment
        //                      // 1800-2009 restricts LHS of assignment to new to not have a range
        //                      // This is ignored to avoid conflicts
        |       fexprLvalue yP_EQ__NEW dynamic_array_new ';'   { $$ = new Ast_ਸੌਂਪੋ{$2, $1, $3}; }
        |       fexprLvalue yP_EQ__NEW class_new ';'    { $$ = new Ast_ਸੌਂਪੋ{$2, $1, $3}; }
        //                      // IEEE: inc_or_dec_expression
        |       finc_or_dec_ਵਿਅੰਜਕ ';'              { $$ = new Ast_Stmt_ਵਿਅੰਜਕ{$<fl>1, $1}; }
        //
        //                      // IEEE: nonblocking_assignment
        |       fexprLvalue yP_LTE delay_or_event_controlE ਵਿਅੰਜਕ ';'
                        { $$ = new Ast_ਸੌਂਪੋ_Dly{$2, $1, $4, $3}; }
        //                      // IEEE: clocking_drive ';'
        |       fexprLvalue yP_LTE ਚੱਕਰ_delay ਵਿਅੰਜਕ ';'
                        { $$ = new Ast_ਸੌਂਪੋ_Dly{$2, $1, $4, $3}; }
        //UNSUP cycle_delay fexprLvalue yP_LTE ';'      { UNSUP }
        |       y_ਸੌਂਪੋ variable_lvalue '=' delay_or_event_controlE ਵਿਅੰਜਕ ';'
                        { $$ = new Ast_ਸੌਂਪੋ_Cont{$1, $2, $5, $4};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.ਸੈੱਟ_Has_ਸੌਂਪੋ_Deassign(); }
        |       yDEASSIGN variable_lvalue ';'
                        { $$ = new AstDeassign{$1, $2};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.ਸੈੱਟ_Has_ਸੌਂਪੋ_Deassign(); }
        |       yFORCE variable_lvalue '=' ਵਿਅੰਜਕ ';'
                        { $$ = new Ast_ਸੌਂਪੋ_Force{$1, $2, $4}; v3Global.ਸੈੱਟ_Has_Forceable_ਸੰਕੇਤ(); }
        |       yRELEASE variable_lvalue ';'
                        { $$ = new AstRelease{$1, $2}; v3Global.ਸੈੱਟ_Has_Forceable_ਸੰਕੇਤ(); }
        //
        //                      // IEEE: case_statement
        |       unique_ਤਰਜੀਹ_E case_ਸ਼ੁਰੂ caseAttrE case_ਵਸਤੂ_List yENDCASE
                        { $$ = $2; ਜੇ ($4) $2->ਜੋੜੋ_Itemsp($4);
                          ਜੇ ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          ਜੇ ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          ਜੇ ($1 == uniq_ਤਰਜੀਹ) $2->ਤਰਜੀਹ_Pragma(true); }
        // case matches uses patterns, not expressions
        |       unique_ਤਰਜੀਹ_E case_ਸ਼ੁਰੂ caseAttrE yMATCHES case_matches_ਵਸਤੂ_List yENDCASE
                        { $$ = $2; ਜੇ ($5) $2->ਜੋੜੋ_Itemsp($5);
                          $2->case_Matches_ਸੈੱਟ();
                          ਜੇ ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          ਜੇ ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          ਜੇ ($1 == uniq_ਤਰਜੀਹ) $2->ਤਰਜੀਹ_Pragma(true); }
        |       unique_ਤਰਜੀਹ_E case_ਸ਼ੁਰੂ caseAttrE yINSIDE case_inside_ਵਸਤੂ_List yENDCASE
                        { $$ = $2; ਜੇ ($5) $2->ਜੋੜੋ_Itemsp($5);
                          ਜੇ (!$2->caseSimple()) $4->v3error("Illegal to have inside on a casex/casez");
                          $2->case_Inside_ਸੈੱਟ();
                          ਜੇ ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          ਜੇ ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          ਜੇ ($1 == uniq_ਤਰਜੀਹ) $2->ਤਰਜੀਹ_Pragma(true); }
        //
        //                      // IEEE: conditional_statement
        |       unique_ਤਰਜੀਹ_E yIF '(' ਵਿਅੰਜਕ ')' stmt     %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { AstIf* ਅਚਲ newp = new AstIf{$2, $4,
                              PARSEP->new_ਖੰਡ($2, $6)};
                          $$ = newp;
                          ਜੇ ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          ਜੇ ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          ਜੇ ($1 == uniq_ਤਰਜੀਹ) newp->ਤਰਜੀਹ_Pragma(true); }
        |       unique_ਤਰਜੀਹ_E yIF '(' ਵਿਅੰਜਕ ')' stmt yELSE stmt
                        { AstIf* ਅਚਲ newp = new AstIf{$2, $4,
                              PARSEP->new_ਖੰਡ($2, $6),
                              PARSEP->new_ਖੰਡ($2, $8)};
                          $$ = newp;
                          ਜੇ ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          ਜੇ ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          ਜੇ ($1 == uniq_ਤਰਜੀਹ) newp->ਤਰਜੀਹ_Pragma(true); }
        //
        //                      // IEEE: subroutine_call_statement
        //                      // IEEE says we then expect a function call
        //                      // (function_subroutine_callNoMethod), but rest of
        //                      // the code expects an AstTask when used as a statement,
        //                      // so parse as if task
        //                      // Alternative would be shim with new AstVoidStmt.
        |       yVOID yP_TICK '(' task_subroutine_call_No_ਵਿਧੀ ')' ';'
                        { Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = $4;
                          Ast_ਗੰਢ* callp = exprp;
                          ਜਦੋਂ (AstDot* ਅਚਲ dotp = VN_CAST(callp, Dot)) callp = dotp->rhsp();
                          File_ਸਤਰ* ਅਚਲ newfl = new File_ਸਤਰ{callp->fileline()};
                          newfl->warnOff(V3_Error_ਕੋਡ::IGNOREDRETURN, true);
                          callp->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' ਵਿਅੰਜਕ '.' task_subroutine_call_No_ਵਿਧੀ ')' ';'
                        { Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = new AstDot{$5, false, $4, $6};
                          File_ਸਤਰ* ਅਚਲ newfl = new File_ਸਤਰ{$6->fileline()};
                          newfl->warnOff(V3_Error_ਕੋਡ::IGNOREDRETURN, true);
                          $6->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' system_f_only_ਵਿਅੰਜਕ_call ')' ';'
                        { $$ = new Ast_Stmt_ਵਿਅੰਜਕ{$<fl>4, $4};
                          File_ਸਤਰ* ਅਚਲ newfl = new File_ਸਤਰ{$$->fileline()};
                          newfl->warnOff(V3_Error_ਕੋਡ::IGNOREDRETURN, true);
                          $$->fileline(newfl); }
        //                      // Any system function as a task
        |       yVOID yP_TICK '(' system_f_or_t_ਵਿਅੰਜਕ_call ')' ';'
                        { $$ = new Ast_Stmt_ਵਿਅੰਜਕ{$<fl>4, $4};
                          File_ਸਤਰ* ਅਚਲ newfl = new File_ਸਤਰ{$$->fileline()};
                          newfl->warnOff(V3_Error_ਕੋਡ::IGNOREDRETURN, true);
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
                          PARSEP->import_If_ਵਿੱਚ_Std($1, "process", true);
                        }
        //                      // IEEE: event_trigger
        |       yP_MINUSGT ਵਿਅੰਜਕ ';'
                        { $$ = new AstFireEvent{$1, $2, false}; }
        |       yP_MINUSGTGT delay_or_event_controlE ਵਿਅੰਜਕ ';'
                        { $$ = new AstFireEvent{$1, $3, true}; }
        //
        // do/for/forever/while loops all modelled as AstLoop
        |       yDO stmt yWHILE '(' ਵਿਅੰਜਕ ')' ';'
                        { AstLoop* ਅਚਲ loopp = new AstLoop{$1, $2};
                          loopp->ਜੋੜੋ_Contsp(new AstLoopTest{$<fl>5, loopp, $5});
                          $$ = loopp; }
        |       yFOR  '(' { VARRESET_NONLIST(UNKNOWN); } for_initializationE ';' ਵਿਅੰਜਕ_E ';' for_ਕਦਮ_E ')' stmt
                        { AstBegin* ਅਚਲ blockp = new AstBegin{$1, "", $4, true};
                          AstLoop* ਅਚਲ loopp = new AstLoop{$1};
                          ਜੇ ($6) loopp->ਜੋੜੋ_Stmtsp(new AstLoopTest{$<fl>6, loopp, $6});
                          loopp->ਜੋੜੋ_Stmtsp($10);
                          loopp->ਜੋੜੋ_Contsp($8);
                          blockp->ਜੋੜੋ_Stmtsp(loopp);
                          $$ = blockp; }
        |       yFOREVER stmt
                        { AstLoop* ਅਚਲ loopp = new AstLoop{$1, $2};
                          $$ = loopp; }
        |       yWHILE '(' ਵਿਅੰਜਕ ')' stmt
                        { AstLoop* ਅਚਲ loopp = new AstLoop{$1};
                          loopp->ਜੋੜੋ_Stmtsp(new AstLoopTest{$<fl>3, loopp, $3});
                          loopp->ਜੋੜੋ_Stmtsp($5);
                          $$ = loopp; }
        // Other loop statements
        |       yREPEAT '(' ਵਿਅੰਜਕ ')' stmt          { $$ = new AstRepeat{$1, $3, $5}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM and 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' stmt
                        { $$ = new AstBegin{$1, "", new AstForeach{$1, $3, $5}, true}; }
        //
        //                      // IEEE: jump_statement
        |       yRETURN ';'                             { $$ = new AstReturn{$1}; }
        |       yRETURN ਵਿਅੰਜਕ ';'                        { $$ = new AstReturn{$1, $2}; }
        |       yBREAK ';'                              { $$ = new AstBreak{$1}; }
        |       yCONTINUE ';'                           { $$ = new AstContinue{$1}; }
        //
        |       par_ਖੰਡ                               { $$ = $1; }
        //                      // IEEE: procedural_timing_control_statement + procedural_timing_control
        |       delay_control stmt
                        { Ast_ਗੰਢ_Stmt* nextp = nullptr;
                          ਜੇ ($2) {
                              ਜੇ ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(), ਗੰਢ_Stmt);
                              $1->ਜੋੜੋ_Stmtsp($2);
                          }
                          $$ = $1;
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, nextp); }
        |       event_control stmt
                        { Ast_ਗੰਢ_Stmt* nextp = nullptr;
                          ਜੇ ($2 && $2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(), ਗੰਢ_Stmt);
                          $$ = new AstEventControl{FILELINE_OR_CRE($1), $1, $2};
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, nextp); }
        |       ਚੱਕਰ_delay stmt
                        { Ast_ਗੰਢ_Stmt* nextp = nullptr;
                          ਜੇ ($2) {
                              ਜੇ ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(), ਗੰਢ_Stmt);
                              $1->ਜੋੜੋ_Stmtsp($2);
                          }
                          $$ = $1;
                          ਜੋੜੋ_ਅਗਲਾ_Null($$, nextp); }
        |       seq_ਖੰਡ                               { $$ = $1; }
        //
        //                      // IEEE: wait_statement
        |       yWAIT '(' ਵਿਅੰਜਕ ')' stmt            { $$ = new AstWait{$1, $3, $5}; }
        |       yWAIT yFORK ';'                         { $$ = new AstWaitFork{$1}; }
        //                      // action_block expanded here
        |       y_WAIT_ਕ੍ਰਮ '(' vrdList ')' stmt %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5); }
        |       y_WAIT_ਕ੍ਰਮ '(' vrdList ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5, $7);}
        |       y_WAIT_ਕ੍ਰਮ '(' vrdList ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $6); }
        //
        //                      // IEEE: procedural_assertion_statement
        |       procedural_assertion_statement          { $$ = $1; }
        //
        |       randsequence_statement                  { $$ = $1; }
        //
        //                      // IEEE: randcase_statement
        |       yRANDCASE rand_case_ਵਸਤੂ_List yENDCASE   { $$ = new AstRandCase{$1, $2}; }
        //
        //                      // IEEE: expect_property_statement
        //                      // action_block expanded here
        |       yEXPECT '(' property_spec ')' stmt %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5); }
        |       yEXPECT '(' property_spec ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5, $7); }
        |       yEXPECT '(' property_spec ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $6); }
        ;

statementVerilatorPragmas<pragmap>:
                y_VL_COVERAGE_ਖੰਡ_OFF
                        { $$ = new AstPragma{$1, VPragmaType::COVERAGE_ਖੰਡ_OFF}; }
        |       yVL_UNROLL_DISABLE
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_DISABLE}; }
        |       y_VL_UNROLL_ਭਰਿਆ
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_ਭਰਿਆ}; }
        ;

foperator_assignment<ਗੰਢ_Stmtp>:    // IEEE: operator_assignment (for first part of expression)
                fexprLvalue '=' delay_or_event_controlE ਵਿਅੰਜਕ    { $$ = new Ast_ਸੌਂਪੋ{$2, $1, $4, $3}; }
        //
        |       fexprLvalue yP_PLUSEQ    ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::ਜੋੜੋ_2, $2, $1, $3}; }
        |       fexprLvalue yP_MINUSEQ   ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::Sub, $2, $1, $3}; }
        |       fexprLvalue yP_TIMESEQ   ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::Mul, $2, $1, $3}; }
        |       fexprLvalue yP_DIVEQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::Div, $2, $1, $3}; }
        |       fexprLvalue yP_MODEQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::ModDiv, $2, $1, $3}; }
        |       fexprLvalue yP_ANDEQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::And, $2, $1, $3}; }
        |       fexprLvalue yP_OREQ      ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::Or, $2, $1, $3}; }
        |       fexprLvalue yP_XOREQ     ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::Xor, $2, $1, $3}; }
        |       fexprLvalue yP_SLEFTEQ   ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::ਖਿਸਕਾਓ_L, $2, $1, $3}; }
        |       fexprLvalue yP_SRIGHTEQ  ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::ਖਿਸਕਾਓ_R, $2, $1, $3}; }
        |       fexprLvalue yP_SSRIGHTEQ ਵਿਅੰਜਕ
                        { $$ = new Ast_ਸੌਂਪੋ_Compound{Ast_ਸੌਂਪੋ_Compound::operation::ਖਿਸਕਾਓ_RS, $2, $1, $3}; }
        ;

inc_or_dec_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:   // ==IEEE: inc_or_dec_expression
        //                      // Need fexprScope instead of variable_lvalue to prevent conflict
                ~l~ਵਿਅੰਜਕ_Scope yP_PLUSPLUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostInc{$2, $1}; }
        |       ~l~ਵਿਅੰਜਕ_Scope yP_MINUSMINUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostDec{$2, $1}; }
        //                      // Need expr instead of variable_lvalue to prevent conflict
        |       yP_PLUSPLUS     ਵਿਅੰਜਕ
                        { $<fl>$ = $<fl>1; $$ = new AstPreInc{$1, $2}; }
        |       yP_MINUSMINUS   ਵਿਅੰਜਕ
                        { $<fl>$ = $<fl>1; $$ = new AstPreDec{$1, $2}; }
        ;

finc_or_dec_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:  // ==IEEE: inc_or_dec_expression
                BISONPRE_COPY(inc_or_dec_ਵਿਅੰਜਕ,{s/~l~/f/g})        // {copied}
        ;

sinc_or_dec_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:  // IEEE: inc_or_dec_expression (for sequence_expression)
                BISONPRE_COPY(inc_or_dec_ਵਿਅੰਜਕ,{s/~l~/s/g})        // {copied}
        ;

pinc_or_dec_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:  // IEEE: inc_or_dec_expression (for property_expression)
                BISONPRE_COPY(inc_or_dec_ਵਿਅੰਜਕ,{s/~l~/p/g})        // {copied}
        ;

//UNSUPev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for ev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/ev_/g})      // {copied}
//UNSUP ;

//UNSUPpev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for pev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/pev_/g})     // {copied}
//UNSUP ;

class_new<ਗੰਢ_Exprp>:    // IEEE: class_new
        //                      // See V3ParseImp::tokenPipeScanEqNew that searches for '=' ... yNEW__LEX
                class_newNoScope
                        { $$ = $1; }
        //                      // Special precedence so (...) doesn't match expr
        //                      // A scope is not legal in front of a AstNewCopy
        |       packageClassScopeNoId yNEW__ETC
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2,  nullptr, true}); }
        |       packageClassScopeNoId yNEW__PAREN '(' list_ਦਾ_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2, $4, true}); }
        ;

class_newNoScope<ਗੰਢ_Exprp>:    // IEEE: class_new but no packageClassScope
        //                      // Special precedence so (...) doesn't match expr
                yNEW__ETC                               { $$ = new AstNew{$1}; }
        |       yNEW__ETC ਵਿਅੰਜਕ                          { $$ = new AstNewCopy{$1, $2}; }
        |       yNEW__PAREN '(' list_ਦਾ_arguments_E ')'  { $$ = new AstNew{$1, $3}; }
        ;

dynamic_array_new<ਗੰਢ_Exprp>:   // ==IEEE: dynamic_array_new
                yNEW__ETC '[' ਵਿਅੰਜਕ ']'                  { $$ = new AstNewDynamic{$1, $3, nullptr}; }
        |       yNEW__ETC '[' ਵਿਅੰਜਕ ']' '(' ਵਿਅੰਜਕ ')'     { $$ = new AstNewDynamic{$1, $3, $6}; }
        ;

//************************************************
// Case/If

unique_ਤਰਜੀਹ_E<uniqstate>:    // IEEE: unique_priority + empty
                /*empty*/                               { $$ = uniq_NONE; }
        |       y_ਤਰਜੀਹ                               { $$ = uniq_ਤਰਜੀਹ; }
        |       yUNIQUE                                 { $$ = uniq_UNIQUE; }
        |       yUNIQUE0                                { $$ = uniq_UNIQUE0; }
        ;

case_ਸ਼ੁਰੂ<casep>:               // IEEE: part of case_statement
                yCASE  '(' ਵਿਅੰਜਕ_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASE, $3, nullptr}; }
        |       yCASEX '(' ਵਿਅੰਜਕ_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEX, $3, nullptr}; }
        |       yCASEZ '(' ਵਿਅੰਜਕ_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEZ, $3, nullptr}; }
        ;

caseAttrE:
                /*empty*/                               { }
        |       caseAttrE y_VL_ਭਰਿਆ_CASE                 { GRAMMARP->m_caseAttrp->ਭਰਿਆ_Pragma(true); }
        |       caseAttrE yVL_PARALLEL_CASE             { GRAMMARP->m_caseAttrp->parallelPragma(true); }
        ;

case_ਵਸਤੂ_List<caseItemp>:       // IEEE: { case_item + ... }
                caseCondList colon stmt                    { $$ = new Ast_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $2}; }
        |       case_ਵਸਤੂ_List caseCondList colon stmt      { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$3, $2, $4}); }
        |       case_ਵਸਤੂ_List yDEFAULT stmt                { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$2, nullptr, $3}); }
        |       case_ਵਸਤੂ_List yDEFAULT colon stmt          { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$2, nullptr, $4}); }
        ;

case_inside_ਵਸਤੂ_List<caseItemp>:        // IEEE: { case_inside_item + range_list ... }
                range_list colon stmt                      { $$ = new Ast_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $2}; }
        |       case_inside_ਵਸਤੂ_List range_list colon stmt { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$3, $2, $4}); }
        |       case_inside_ਵਸਤੂ_List yDEFAULT stmt         { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$2, nullptr, $3}); }
        |       case_inside_ਵਸਤੂ_List yDEFAULT colon stmt   { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$2, nullptr, $4}); }
        ;

case_matches_ਵਸਤੂ_List<caseItemp>:    // IEEE: { case_pattern_item + ... }
        //                      // IEEE: case_pattern_item ::= pattern [&&& expr] : stmt
        //                      // pattern includes expr for tagged void members (tagged id)
                ਨਮੂਨਾ_No_ਵਿਅੰਜਕ colon stmt                   { $$ = new Ast_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       ਵਿਅੰਜਕ colon stmt                            { $$ = new Ast_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $2}; }
        |       case_matches_ਵਸਤੂ_List ਨਮੂਨਾ_No_ਵਿਅੰਜਕ colon stmt
                        { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$3, $2, $4}); }
        |       case_matches_ਵਸਤੂ_List ਵਿਅੰਜਕ colon stmt
                        { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$3, $2, $4}); }
        |       case_matches_ਵਸਤੂ_List yDEFAULT stmt        { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$2, nullptr, $3}); }
        |       case_matches_ਵਸਤੂ_List yDEFAULT colon stmt  { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$2, nullptr, $4}); }
        ;

rand_case_ਵਸਤੂ_List<caseItemp>:       // IEEE: { rand_case_item + ... }
        //                      // Randcase syntax doesn't have default, or expression lists
                ਵਿਅੰਜਕ colon stmt                            { $$ = new Ast_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       rand_case_ਵਸਤੂ_List ਵਿਅੰਜਕ colon stmt         { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new Ast_Case_ਵਸਤੂ{$3, $2, $4}); }
        ;

range_list<ਗੰਢ_Exprp>:     // ==IEEE: range_list/open_range_list + value_range/open_value_range
                ਮੁੱਲ_range                             { $$ = $1; }
        |       range_list ',' ਮੁੱਲ_range              { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

ਮੁੱਲ_range<ਗੰਢ_Exprp>:         // ==IEEE: value_range/open_value_range
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       '[' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ']'                   { $$ = new AstInsideRange{$1, $2, $4}; }
        //                      // IEEE-2023: added all four:
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' '$' ':' expr ']'
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' expr ':' '$' ']'
        |       '[' ਵਿਅੰਜਕ yP_PLUSSLASHMINUS ਵਿਅੰਜਕ ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +/- range"); DEL($2, $4); }
        |       '[' ਵਿਅੰਜਕ yP_PLUSPCTMINUS ਵਿਅੰਜਕ ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +%- range"); DEL($2, $4); }
        ;

covergroup_ਮੁੱਲ_range<ਗੰਢ_Exprp>:  // ==IEEE-2012: covergroup_value_range
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

caseCondList<ਗੰਢ_Exprp>:        // IEEE: part of case_item
                ਵਿਅੰਜਕ_Type_Compare                         { $$ = $1; }
        |       caseCondList ',' ਵਿਅੰਜਕ_Type_Compare        { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

ਨਮੂਨਾ_No_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:       // IEEE: pattern **Excluding Expr*
                '.' idAny/*variable*/
                        { $$ = new Ast_ਨਮੂਨਾ_Var{$1, *$2}; }
        |       yP_DOTSTAR
                        { $$ = new Ast_ਨਮੂਨਾ_ਤਾਰਾ{$1}; }
        //                      // IEEE: "expr" excluded; expand in callers
        //                      // IEEE: tagged member_identifier [ pattern ]
        //                      // Standalone "yTAGGED__NONPRIMARY idAny" is handled via expr in patternOne
        //                      // Here, we need to treat yTAGGED and yTAGGED__NONPRIMARY identically.
        |       yTAGGED idAny/*member_identifier*/ ਨਮੂਨਾ_No_ਵਿਅੰਜਕ
                        { $$ = new Ast_Tagged_ਨਮੂਨਾ{$1, *$2, $3}; }
        |       yTAGGED__NONPRIMARY idAny/*member_identifier*/ ਨਮੂਨਾ_No_ਵਿਅੰਜਕ
                        { $$ = new Ast_Tagged_ਨਮੂਨਾ{$1, *$2, $3}; }
        //                      // "yP_TICKBRA patternList '}'" part of expr under assignment_pattern
        ;

ਨਮੂਨਾ_List<nodep>:             // IEEE: part of pattern
                ਨਮੂਨਾ_One                              { $$ = $1; }
        |       ਨਮੂਨਾ_List ',' ਨਮੂਨਾ_One              { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

ਨਮੂਨਾ_One<nodep>:              // IEEE: part of pattern
                ਵਿਅੰਜਕ
                        { ਜੇ ($1) $$ = new AstPatMember{$1->fileline(), $1, nullptr, nullptr}; ਨਹੀਂਤਾਂ $$ = nullptr; }
        |       ਵਿਅੰਜਕ '{' ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List '}'               { $$ = new AstPatMember{$2, $3, nullptr, $1}; }
        |       ਨਮੂਨਾ_No_ਵਿਅੰਜਕ                           { $$ = $1; }
        ;

ਨਮੂਨਾ_Member_List<nodep>:       // IEEE: part of pattern and assignment_pattern
                ਨਮੂਨਾ_Member_One                        { $$ = $1; }
        |       ਨਮੂਨਾ_Member_List ',' ਨਮੂਨਾ_Member_One  { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

ਨਮੂਨਾ_Member_One<patMemberp>:   // IEEE: part of pattern and assignment_pattern
                ਨਮੂਨਾ_ਕੁੰਜੀ ':' ਵਿਅੰਜਕ                     { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        |       ਨਮੂਨਾ_ਕੁੰਜੀ ':' ਨਮੂਨਾ_No_ਵਿਅੰਜਕ            { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        //                      // From assignment_pattern_key
        |       yDEFAULT ':' ਵਿਅੰਜਕ                       { $$ = new AstPatMember{$1, $3, nullptr, nullptr}; $$->isDefault(true); }
        |       yDEFAULT ':' ਨਮੂਨਾ_No_ਵਿਅੰਜਕ              { AstPatMember* ਅਚਲ patp = new AstPatMember{$1, $3, nullptr, nullptr}; patp->isDefault(true); $$ = patp; }
        ;

ਨਮੂਨਾ_ਕੁੰਜੀ<nodep>:              // IEEE: merge structure_pattern_key, array_pattern_key, assignment_pattern_key
        //                      // IEEE: structure_pattern_key: member_identifier | assignment_pattern_key
        //                      // IEEE: array_pattern_key: constant_expression | assignment_pattern_key
        //                      // Verilator:
        //                      //   A bare "foo" is ambiguous here, as it may be a constant
        //                      //   identifier (if array) or a reference to the "foo" member
        //                      //   (if structure).  Both spell the same in expr, so the
        //                      //   bare-identifier case becomes a Text node and V3LinkDot
        //                      //   resolves which one it is.
                ਵਿਅੰਜਕ
                        { $$ = GRAMMARP->create_ਨਮੂਨਾ_ਕੁੰਜੀ($1); }
        //                      // IEEE: assignment_pattern_key
        |       simple_typeNoRef
                        { $$ = $1; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // (simple_type ps_parameter_identifier is part of expr above)
        |       packageClassScopeE idType
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = refp; }
        ;

assignment_ਨਮੂਨਾ<patternp>:   // ==IEEE: assignment_pattern
        // This doesn't match the text of the spec.  I think a : is missing, or example code needed
        // yP_TICKBRA constExpr exprList '}'    { $$="'{"+$2+" "+$3"}"; }
        //                      // "'{ const_expression }" is same as patternList with one entry
        //                      // From patternNoExpr
        //                      // also IEEE: "''{' expression { ',' expression } '}'"
        //                      //      matches since patternList includes expr
                yP_TICKBRA ਨਮੂਨਾ_List '}'              { $$ = new Ast_ਨਮੂਨਾ{$1, $2}; }
        //                      // From patternNoExpr
        //                      // also IEEE "''{' structure_pattern_key ':' ...
        //                      // also IEEE "''{' array_pattern_key ':' ...
        |       yP_TICKBRA ਨਮੂਨਾ_Member_List '}'        { $$ = new Ast_ਨਮੂਨਾ{$1, $2}; }
        //                      // IEEE: Not in grammar, but in VMM
        |       yP_TICKBRA '}'                          { $$ = new Ast_ਨਮੂਨਾ{$1, nullptr}; }
        ;

// "datatype id = x {, id = x }"  |  "yaId = x {, id=x}" is legal
for_initializationE<nodep>:      // ==IEEE: for_initialization + for_variable_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                     { $$ = nullptr; }
        |       for_initialization_ਵਸਤੂ_List      { $$ = $1; }
        ;

for_initialization_ਵਸਤੂ_List<nodep>:      // IEEE: [for_variable_declaration...]
                for_initialization_ਵਸਤੂ                  { $$ = $1; }
        |       for_initialization_ਵਸਤੂ_List ',' for_initialization_ਵਸਤੂ   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

for_initialization_ਵਸਤੂ<nodep>:          // IEEE: variable_assignment + for_variable_declaration
        //                      // IEEE: for_variable_declaration
                ਅੰਕੜੇ_type idAny/*new*/ '=' ਵਿਅੰਜਕ
                        { VARRESET_NONLIST(VAR); VARDTYPE($1);
                          AstVar* ਅਚਲ varp = VARDONEA($<fl>2, *$2, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->ਜੋੜੋ_ਅਗਲਾ(new Ast_ਸੌਂਪੋ{$3, new AstParseRef{$<fl>2, *$2}, $4}); }
        //                      // IEEE-2012:
        |       yVAR ਅੰਕੜੇ_type idAny/*new*/ '=' ਵਿਅੰਜਕ
                        { VARRESET_NONLIST(VAR); VARDTYPE($2);
                          AstVar* ਅਚਲ varp = VARDONEA($<fl>3, *$3, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->ਜੋੜੋ_ਅਗਲਾ(new Ast_ਸੌਂਪੋ{$4, new AstParseRef{$<fl>3, *$3}, $5}); }
        //                      // IEEE: variable_assignment
        //                      // UNSUP variable_lvalue below
        |       id/*newOrExisting*/ '=' ਵਿਅੰਜਕ
                        { ਜੇ (GRAMMARP->m_varDecl) {
                              AstVar* ਅਚਲ varp = VARDONEA($<fl>1, *$1, nullptr, nullptr);
                              varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                              $$ = varp;
                              $$->ਜੋੜੋ_ਅਗਲਾ(new Ast_ਸੌਂਪੋ{$2, new AstParseRef{$<fl>1, *$1}, $3});
                          } ਨਹੀਂਤਾਂ {
                              $$ = new Ast_ਸੌਂਪੋ{$2, new AstParseRef{$<fl>1, *$1}, $3};
                          }
                        }
        ;

for_ਕਦਮ_E<nodep>:               // IEEE: for_step + empty
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       for_ਕਦਮ                                { $$ = $1; }
        ;

for_ਕਦਮ<nodep>:                // IEEE: for_step
                for_ਕਦਮ_assignment                     { $$ = $1; }
        |       for_ਕਦਮ ',' for_ਕਦਮ_assignment        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

for_ਕਦਮ_assignment<nodep>:     // ==IEEE: for_step_assignment
                foperator_assignment                    { $$ = $1; }
        |       finc_or_dec_ਵਿਅੰਜਕ                  { $$ = new Ast_Stmt_ਵਿਅੰਜਕ{$<fl>1, $1}; }
        //                      // IEEE: function_subroutine_call
        |       task_subroutine_callNoSemi              { $$ = $1; }
        ;

loop_variables<nodep>:          // IEEE: loop_variables
                loop_variableE                          { $$ = $1; }
        |       loop_variables ',' loop_variableE       { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

loop_variableE<nodep>:          // IEEE: part of loop_variables
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = new Ast_ਖਾਲੀ_ਥਾਵਾਂ{CRELINE()}; }
        |       parse_Ref_ਅਧਾਰ                            { $$ = $1; }
        ;

//************************************************
// Functions/tasks

taskRef<ਗੰਢ_Exprp>:            // IEEE: part of tf_call
                id                                      { $$ = new AstTaskRef{$<fl>1, *$1}; }
        |       id '(' list_ਦਾ_arguments_E ')'           { $$ = new AstTaskRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_ਦਾ_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstTaskRef{$<fl>2, *$2, $4}); }
        ;

funcRef<ਗੰਢ_Exprp>:             // IEEE: part of tf_call
        //                      // package_scope/hierarchical_... is part of expr, so just need ID
        //                      //      making-a                id-is-a
        //                      //      -----------------       ------------------
        //                      //      tf_call                 tf_identifier           expr (list_of_arguments)
        //                      //      method_call(post .)     function_identifier     expr (list_of_arguments)
        //                      //      property_instance       property_identifier     property_actual_arg
        //                      //      sequence_instance       sequence_identifier     sequence_actual_arg
        //                      //      let_expression          let_identifier          let_actual_arg
        //
                id '(' list_ਦਾ_arguments_E ')'
                        { $$ = new AstFuncRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_ਦਾ_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstFuncRef{$<fl>2, *$2, $4}); }
        //UNSUP list_of_argumentE should be pev_list_of_argumentE
        //UNSUP: idDottedSel is really just id to allow dotted method calls
        ;

task_subroutine_callNoSemi<ਗੰਢ_Stmtp>:  // similar to IEEE task_subroutine_call but without ';'
        //                      // Expr included here to resolve our not knowing what is a method call
        //                      // Expr here must result in a subroutine_call
                task_subroutine_call_No_ਵਿਧੀ            { $$ = $1->makeStmt(); }
        |       fexpr '.' task_subroutine_call_No_ਵਿਧੀ  { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        |       system_t_stmt_call                      { $$ = $1; }
        //                      // Any system function as a task
        |       system_f_or_t_ਵਿਅੰਜਕ_call                 { $$ = new Ast_Stmt_ਵਿਅੰਜਕ{$<fl>1, $1}; }
        //                      // Not here in IEEE; from class_constructor_declaration
        //                      // Because we've joined class_constructor_declaration into generic functions
        //                      // Way over-permissive;
        //                      // IEEE: [ ySUPER '.' yNEW [ '(' list_of_arguments ')' ] ';' ]
        |       fexpr '.' class_newNoScope              { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        ;

task_subroutine_call_No_ਵਿਧੀ<ਗੰਢ_Exprp>:    // function_subroutine_callNoMethod (as task)
        //                      // IEEE: tf_call
                taskRef                                 { $$ = $1; }
        //                      // funcref below not task ref to avoid conflict, must later handle either
        |       funcRef yWITH__PAREN '(' ਵਿਅੰਜਕ ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' ਵਿਅੰਜਕ ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_ਖੰਡ     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_ਖੰਡ
                                                        { AstWithParse* ਅਚਲ withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

function_subroutine_call_No_ਵਿਧੀ<ਗੰਢ_Exprp>:        // IEEE: function_subroutine_call (as function)
        //                      // IEEE: tf_call
                funcRef                                 { $$ = $1; }
        |       funcRef yWITH__PAREN '(' ਵਿਅੰਜਕ ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' ਵਿਅੰਜਕ ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        |       system_f_only_ਵਿਅੰਜਕ_call                 { $$ = $1; }
        |       system_f_or_t_ਵਿਅੰਜਕ_call                 { $$ = $1; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_ਖੰਡ     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_ਖੰਡ
                                                        { AstWithParse* ਅਚਲ withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

system_t_stmt_call<ਗੰਢ_Stmtp>:  // IEEE: part of system_tf_call (as task returning statement)
        //
                yaD_PLI system_Dpi_ਦਲੀਲਾਂ_E                  { AstTaskRef* ਅਚਲ refp = new AstTaskRef{$<fl>1, *$1, $2};
                                                          refp->pli(true);
                                                          $$ = refp->makeStmt(); }
        //
        |       yD_DUMPPORTS '(' idDottedSel ',' ਵਿਅੰਜਕ ')'  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $5}; DEL($3);
                                                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPPORTS '(' ',' ਵਿਅੰਜਕ ')'           { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $4};
                                                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPFILE '(' ਵਿਅੰਜਕ ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $3}; }
        |       yD_DUMPVARS parenE                      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                              new AstConst{$<fl>1, 0}}; }
        |       yD_DUMPVARS '(' ਵਿਅੰਜਕ ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; }
        |       yD_DUMPVARS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_List ')'   { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; DEL($5); }
        |       yD_DUMPALL parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; }
        |       yD_DUMPALL '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; DEL($3); }
        |       yD_DUMPFLUSH parenE                     { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; }
        |       yD_DUMPFLUSH '(' ਵਿਅੰਜਕ ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; DEL($3); }
        |       yD_DUMPLIMIT '(' ਵਿਅੰਜਕ ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ਹੱਦਬੰਦੀ_2, $3}; }
        |       yD_DUMPLIMIT '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ਹੱਦਬੰਦੀ_2, $3}; DEL($5); }
        |       yD_DUMPOFF parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; }
        |       yD_DUMPOFF '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; DEL($3); }
        |       yD_DUMPON parenE                        { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; }
        |       yD_DUMPON '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; DEL($3); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCStmtUser* cstmtp = nullptr;
                    ਜੇ (!v3Global.opt.ignc()) {
                        cstmtp = new AstCStmtUser{$1, true};
                        cstmtp->ਜੋੜੋ($3);
                    }
                    $$ = cstmtp;
                }
        |       yD_SDF_ANNOTATE '(' ਵਿਅੰਜਕ_EList_E ')'      { $$ = nullptr; $1->v3warn(SPECIFYIGN, "Ignoring unsupported: $sdf_annotate"); DEL($3); }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_ਢੇਰ_Trace_T{$1}; }
        |       yD_SYSTEM '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstSystemT{$1, $3}; }
        //
        |       yD_EXIT parenE                          { $$ = new Ast_ਅੰਤ{$1}; }
        //
        |       yD_FCLOSE '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstFClose{$1, $3}; }
        |       yD_FFLUSH parenE                        { $$ = new AstFFlush{$1, nullptr}; }
        |       yD_FFLUSH '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstFFlush{$1, $3}; }
        |       y_D_ਅੰਤ parenE                        { $$ = new Ast_ਅੰਤ{$1}; }
        |       y_D_ਅੰਤ '(' ਵਿਅੰਜਕ ')'                  { $$ = new Ast_ਅੰਤ{$1}; DEL($3); }
        |       yD_STOP parenE                          { $$ = new AstStop{$1, false}; }
        |       yD_STOP '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstStop{$1, false}; DEL($3); }
        //
        |       yD_SFORMAT '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstSFormat{$1, false, $3, $5}; }
        |       yD_SWRITE  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'd'}; }
        |       yD_SWRITEB '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'b'}; }
        |       yD_SWRITEH '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'h'}; }
        |       yD_SWRITEO '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'o'}; }
        //
        |       yD_DISPLAY parenE                               { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr}; }
        |       yD_DISPLAY '(' commasE ਵਿਅੰਜਕ_Disp_List ')'         { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4}; }
        |       yD_DISPLAYB parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'b'}; }
        |       yD_DISPLAYB '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'b'}; }
        |       yD_DISPLAYH parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'h'}; }
        |       yD_DISPLAYH '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'h'}; }
        |       yD_DISPLAYO parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'o'}; }
        |       yD_DISPLAYO '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'o'}; }
        |       yD_MONITOR  '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4}; }
        |       yD_MONITORB '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'b'}; }
        |       yD_MONITORH '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'h'}; }
        |       yD_MONITORO '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'o'}; }
        |       yD_STROBE   '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4}; }
        |       yD_STROBEB  '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'b'}; }
        |       yD_STROBEH  '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'h'}; }
        |       yD_STROBEO  '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'o'}; }
        |       yD_WRITE    parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITE    '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4}; }
        |       yD_WRITEB   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEB   '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'b'}; }
        |       yD_WRITEH   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEH   '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'h'}; }
        |       yD_WRITEO   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEO   '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'o'}; }
        |       yD_FDISPLAY '(' ਵਿਅੰਜਕ ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr}; }
        |       yD_FDISPLAY '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5}; }
        |       yD_FDISPLAYB '(' ਵਿਅੰਜਕ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'b'}; }
        |       yD_FDISPLAYB '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'b'}; }
        |       yD_FDISPLAYH '(' ਵਿਅੰਜਕ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'h'}; }
        |       yD_FDISPLAYH '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'h'}; }
        |       yD_FDISPLAYO '(' ਵਿਅੰਜਕ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'o'}; }
        |       yD_FDISPLAYO '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'o'}; }
        |       yD_FMONITOR  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5}; }
        |       yD_FMONITORB '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'b'}; }
        |       yD_FMONITORH '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'h'}; }
        |       yD_FMONITORO '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'o'}; }
        |       yD_FSTROBE   '(' ਵਿਅੰਜਕ ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, nullptr}; }
        |       yD_FSTROBE   '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5}; }
        |       yD_FSTROBEB  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'b'}; }
        |       yD_FSTROBEH  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'h'}; }
        |       yD_FSTROBEO  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'o'}; }
        |       yD_FWRITE   '(' ਵਿਅੰਜਕ ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, nullptr}; }
        |       yD_FWRITE   '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5}; }
        |       yD_FWRITEB  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'b'}; }
        |       yD_FWRITEH  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'h'}; }
        |       yD_FWRITEO  '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'o'}; }
        |       yD_INFO     parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, nullptr}; }
        |       yD_INFO     '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, $4}; }
        |       yD_WARNING  parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, nullptr}; }
        |       yD_WARNING  '(' commasE ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, $4}; }
        |       yD_ERROR    parenE                              { $$ = GRAMMARP->createDisplayError($1); }
        |       yD_ERROR    '(' commasE ਵਿਅੰਜਕ_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_ERROR, nullptr, $4};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstStop{$1, false}); }
        |       yD_FATAL    parenE
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstStop{$1, true}); }
        |       yD_FATAL    '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstStop{$1, true}); DEL($3); }
        |       yD_FATAL    '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, $5};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstStop{$1, true}); DEL($3); }
        //
        |       yD_ASSERTCTL '(' ਵਿਅੰਜਕ ')'                                            { $$ = new AstAssertCtl{$1, $3}; }
        |       yD_ASSERTCTL '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ')'                                  { $$ = new AstAssertCtl{$1, $3, $5}; }
        |       yD_ASSERTCTL '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ')'                        { $$ = new AstAssertCtl{$1, $3, $5, $7}; }
        |       yD_ASSERTCTL '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ')'              { $$ = new AstAssertCtl{$1, $3, $5, $7, $9}; }
        |       yD_ASSERTCTL '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')' { $$ = new AstAssertCtl{$1, $3, $5, $7, $9, $11}; }
        |       yD_ASSERTFAILOFF '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3}; }
        |       yD_ASSERTFAILOFF '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7}; }
        |       yD_ASSERTFAILON '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3}; }
        |       yD_ASSERTFAILON '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7}; }
        |       yD_ASSERTKILL '(' ਵਿਅੰਜਕ ')'                      { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3}; }
        |       yD_ASSERTKILL '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'        { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3, $5}; }
        |       yD_ASSERTKILL parenE                            { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7}; }
        |       yD_ASSERTNONVACUOUSON '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3}; }
        |       yD_ASSERTNONVACUOUSON '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'   { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTNONVACUOUSON parenE                       { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7}; }
        |       yD_ASSERTOFF '(' ਵਿਅੰਜਕ ')'                       { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3}; }
        |       yD_ASSERTOFF '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'         { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3, $5}; }
        |       yD_ASSERTOFF parenE                             { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7}; }
        |       yD_ASSERTON '(' ਵਿਅੰਜਕ ')'                        { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3}; }
        |       yD_ASSERTON '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'          { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3, $5}; }
        |       yD_ASSERTON parenE                              { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7}; }
        |       yD_ASSERTPASSOFF '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3}; }
        |       yD_ASSERTPASSOFF '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7}; }
        |       yD_ASSERTPASSON '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3}; }
        |       yD_ASSERTPASSON '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7}; }
        |       yD_ASSERTVACUOUSOFF '(' ਵਿਅੰਜਕ ')'                { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3}; }
        |       yD_ASSERTVACUOUSOFF '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_List ')'  { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTVACUOUSOFF parenE                      { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7}; }
        //
        |       yD_MONITOROFF parenE                    { $$ = new AstMonitorOff{$1, true}; }
        |       yD_MONITORON parenE                     { $$ = new AstMonitorOff{$1, false}; }
        //
        |       yD_PRINTTIMESCALE                       { $$ = new Ast_Print_ਸਮਾਂ_Scale{$1}; }
        |       yD_PRINTTIMESCALE '(' ')'               { $$ = new Ast_Print_ਸਮਾਂ_Scale{$1}; }
        |       yD_PRINTTIMESCALE '(' idClassSel ')'    { $$ = new Ast_Print_ਸਮਾਂ_Scale{$1}; DEL($3); }
        |       yD_TIMEFORMAT '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ')'
                        { $$ = new Ast_ਸਮਾਂ_Format{$1, $3, $5, $7, $9}; }
        |       yD_TIMEFORMAT '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ')'
                        { $$ = new Ast_ਸਮਾਂ_Format{$1, $3, $5, $7, nullptr}; }
        |       yD_TIMEFORMAT '(' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ')'
                        { $$ = new Ast_ਸਮਾਂ_Format{$1, $3, $5, nullptr, nullptr}; }
        |       yD_TIMEFORMAT '(' ਵਿਅੰਜਕ_E ')'
                        { $$ = new Ast_ਸਮਾਂ_Format{$1, $3, nullptr, nullptr, nullptr}; }
        //
        |       yD_READMEMB '(' ਵਿਅੰਜਕ ',' idClassSel ')'                         { $$ = new AstReadMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_READMEMB '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ')'                { $$ = new AstReadMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_READMEMB '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'       { $$ = new AstReadMem{$1, false, $3, $5, $7, $9}; }
        |       yD_READMEMH '(' ਵਿਅੰਜਕ ',' idClassSel ')'                         { $$ = new AstReadMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_READMEMH '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ')'                { $$ = new AstReadMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_READMEMH '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'       { $$ = new AstReadMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_WRITEMEMB '(' ਵਿਅੰਜਕ ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMB '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ')'               { $$ = new AstWriteMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMB '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'      { $$ = new AstWriteMem{$1, false, $3, $5, $7, $9}; }
        |       yD_WRITEMEMH '(' ਵਿਅੰਜਕ ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMH '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ')'               { $$ = new AstWriteMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMH '(' ਵਿਅੰਜਕ ',' idClassSel ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'      { $$ = new AstWriteMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_CAST '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'
                        { File_ਸਤਰ* ਅਚਲ fl_nowarn = new File_ਸਤਰ{$1};
                          fl_nowarn->warnOff(V3_Error_ਕੋਡ::ਚੌੜਾਈ_2, true);
                          $$ = new AstAssertIntrinsic{fl_nowarn, new AstCastDynamic{fl_nowarn, $5, $3},
                                                      nullptr, nullptr}; }
        ;

system_f_only_ਵਿਅੰਜਕ_call<ਗੰਢ_Exprp>:  // IEEE: part of system_tf_call (for functions returning expressions)
                yaD_PLI system_Dpi_ਦਲੀਲਾਂ_E                  { $$ = new AstFuncRef{$<fl>1, *$1, $2}; VN_CAST($$, FuncRef)->pli(true); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    ਜੇ (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1};
                        cexprp->ਜੋੜੋ($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CPURE '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    ਜੇ (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1, AstCExprUser::ਸ਼ੁੱਧ{}};
                        cexprp->ਜੋੜੋ($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CAST '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'           { $$ = new AstCastDynamic{$1, $5, $3}; }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_ਢੇਰ_Trace_F{$1}; }
        |       yD_SYSTEM  '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstSystemF{$1, $3}; }
        ;

system_f_or_t_ਵਿਅੰਜਕ_call<ਗੰਢ_Exprp>:  // IEEE: part of system_tf_call (can be task or func)
                yD_ACOS '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstAcosD{$1, $3}; }
        |       yD_ACOSH '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstAcoshD{$1, $3}; }
        |       yD_ASIN '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstAsinD{$1, $3}; }
        |       yD_ASINH '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstAsinhD{$1, $3}; }
        |       yD_ATAN '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstAtanD{$1, $3}; }
        |       yD_ATAN2 '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'          { $$ = new AstAtan2D{$1, $3, $5}; }
        |       yD_ATANH '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstAtanhD{$1, $3}; }
        |       y_D_ਬਿੱਟ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'          { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਬਿੱਟ, $3}; }
        |       y_D_ਬਿੱਟ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')' { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਬਿੱਟ, $3, $5}; }
        |       yD_BITSTOREAL '(' ਵਿਅੰਜਕ ')'              { $$ = new Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D{$1, $3}; }
        |       yD_BITSTOSHORTREAL '(' ਵਿਅੰਜਕ ')'         { $$ = new Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D{$1, $3}; UNSUPREAL($1); }
        |       yD_CEIL '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstCeilD{$1, $3}; }
        |       y_D_ਬਦਲਿਆ '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstLogNot{$1, new AstStable{$1, $3, nullptr}}; }
        |       y_D_ਬਦਲਿਆ '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Sen_ਰੁੱਖ_ਬਦਲਿਆ($1, $5)}}; }
        |       y_D_ਬਦਲਿਆ_GCLK '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_ਰੁੱਖ($1)}}; }
        |       yD_CHANGING_GCLK '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstLogNot{$1, new AstSteady{$1, $3}}; }
        |       yD_CLOG2 '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstCLog2{$1, $3}; }
        |       yD_COS '(' ਵਿਅੰਜਕ ')'                     { $$ = new AstCosD{$1, $3}; }
        |       yD_COSH '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstCoshD{$1, $3}; }
        |       yD_COUNTBITS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'              { $$ = new Ast_ਗਿਣਤੀ_ਬਿੱਟ{$1, $3, $5}; }
        |       yD_COUNTBITS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'             { $$ = new Ast_ਗਿਣਤੀ_ਬਿੱਟ{$1, $3, $5, $7}; }
        |       yD_COUNTBITS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'    { $$ = new Ast_ਗਿਣਤੀ_ਬਿੱਟ{$1, $3, $5, $7, $9}; }
        |       yD_COUNTBITS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_List ')'
                        { $$ = new Ast_ਗਿਣਤੀ_ਬਿੱਟ{$1, $3, $5, $7, $9};
                          BBUNSUP($11, "Unsupported: $countbits with more than 3 control fields"); }
        |       yD_COUNTONES '(' ਵਿਅੰਜਕ ')'               { $$ = new Ast_ਗਿਣਤੀ_Ones{$1, $3}; }
        |       yD_DIMENSIONS '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'    { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_DIMENSIONS, $3}; }
        |       y_D_DIST_CHI_ਵਰਗਫਲ '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'        { $$ = new Ast_Dist_Chi_ਵਰਗਫਲ{$1, $3, $5}; }
        |       yD_DIST_ERLANG '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'   { $$ = new AstDistErlang{$1, $3, $5, $7}; }
        |       y_D_DIST_ਘਾਤਾਂਕੀ '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'       { $$ = new Ast_Dist_ਘਾਤਾਂਕੀ{$1, $3, $5}; }
        |       y_D_DIST_ਸਾਧਾਰਨ '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'   { $$ = new Ast_Dist_ਸਾਧਾਰਨ{$1, $3, $5, $7}; }
        |       y_D_DIST_ਪੁਆਸੋਂ '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'   { $$ = new Ast_Dist_ਪੁਆਸੋਂ{$1, $3, $5}; }
        |       yD_DIST_T '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'         { $$ = new AstDistT{$1, $3, $5}; }
        |       y_D_DIST_ਇਕਸਾਰ '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'  { $$ = new Ast_Dist_ਇਕਸਾਰ{$1, $3, $5, $7}; }
        |       yD_EXP '(' ਵਿਅੰਜਕ ')'                     { $$ = new AstExpD{$1, $3}; }
        |       yD_FALLING_GCLK '(' ਵਿਅੰਜਕ ')'            { $$ = new AstFalling{$1, $3}; }
        |       yD_FELL '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstFell{$1, $3, nullptr}; }
        |       yD_FELL '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'           { $$ = new AstFell{$1, $3, GRAMMARP->create_Sen_ਰੁੱਖ_ਬਦਲਿਆ($1, $5)}; }
        |       yD_FELL_GCLK '(' ਵਿਅੰਜਕ ')'               { $$ = new AstFell{$1, $3, GRAMMARP->create_Global_Clock_Sen_ਰੁੱਖ($1)}; }
        |       yD_FEOF '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstFEof{$1, $3}; }
        |       yD_FERROR '(' ਵਿਅੰਜਕ ',' idClassSel ')'   { $$ = new AstFError{$1, $3, $5}; }
        |       yD_FGETC '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstFGetC{$1, $3}; }
        |       yD_FGETS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'          { $$ = new AstFGetS{$1, $3, $5}; }
        |       yD_FOPEN '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstFOpenMcd{$1, $3}; }
        |       yD_FOPEN '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'          { $$ = new AstFOpen{$1, $3, $5}; }
        |       yD_FREAD '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'          { $$ = new AstFRead{$1, $3, $5, nullptr, nullptr}; }
        |       yD_FREAD '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'  { $$ = new AstFRead{$1, $3, $5, $7, nullptr}; }
        |       yD_FREAD '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'  { $$ = new AstFRead{$1, $3, $5, $7, $9}; }
        |       yD_FREAD '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ',' ਵਿਅੰਜਕ ')'  { $$ = new AstFRead{$1, $3, $5, nullptr, $8}; }
        |       yD_FREWIND '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstFRewind{$1, $3}; }
        |       yD_FUTURE_GCLK '(' ਵਿਅੰਜਕ ')'             { $$ = new AstFuture{$1, $3, nullptr}; }
        |       yD_FLOOR '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstFloorD{$1, $3}; }
        |       yD_FSCANF '(' ਵਿਅੰਜਕ ',' str commaVRDListE ')'    { $$ = new AstFScanF{$1, *$5, $3, $6}; }
        |       yD_FSEEK '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')' { $$ = new AstFSeek{$1, $3, $5, $7}; }
        |       yD_FTELL '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstFTell{$1, $3}; }
        |       y_D_GET_INITIAL_RANDOM_ਬੀਜ parenE      { $$ = new Ast_Get_Initial_Random_ਬੀਜ{$1}; }
        |       yD_GLOBAL_CLOCK parenE                  { $$ = GRAMMARP->createGlobalClockParseRef($1); }
        |       y_D_ਉੱਚਾ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'          { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਉੱਚਾ, $3, nullptr}; }
        |       y_D_ਉੱਚਾ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')' { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਉੱਚਾ, $3, $5}; }
        |       yD_HYPOT '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'          { $$ = new AstHypotD{$1, $3, $5}; }
        |       yD_INCREMENT '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'     { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_INCREMENT, $3, nullptr}; }
        |       yD_INCREMENT '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')'    { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_INCREMENT, $3, $5}; }
        |       yD_INFERRED_DISABLE parenE              { $$ = new AstInferredDisable{$1}; }
        |       yD_ISUNBOUNDED '(' ਵਿਅੰਜਕ ')'             { $$ = new AstIsUnbounded{$1, $3}; }
        |       yD_ISUNKNOWN '(' ਵਿਅੰਜਕ ')'               { $$ = new AstIsUnknown{$1, $3}; }
        |       yD_ITOR '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstIToRD{$1, $3}; }
        |       y_D_ਖੱਬਾ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'          { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਖੱਬਾ, $3, nullptr}; }
        |       y_D_ਖੱਬਾ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')' { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਖੱਬਾ, $3, $5}; }
        |       yD_LN '(' ਵਿਅੰਜਕ ')'                      { $$ = new AstLogD{$1, $3}; }
        |       yD_LOG10 '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstLog10D{$1, $3}; }
        |       y_D_ਨੀਵਾਂ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'           { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਨੀਵਾਂ, $3, nullptr}; }
        |       y_D_ਨੀਵਾਂ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')'  { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਨੀਵਾਂ, $3, $5}; }
        |       yD_ONEHOT '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstOneHot{$1, $3}; }
        |       yD_ONEHOT0 '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstOneHot0{$1, $3}; }
        |       yD_PAST '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstPast{$1, $3}; }
        |       yD_PAST '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ')'          { $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ')'
                        { ਜੇ ($7) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_E ',' ਵਿਅੰਜਕ_E ',' clocking_eventE ')'
                        { ਜੇ ($7 || $9) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7, $9);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST_GCLK '(' ਵਿਅੰਜਕ ')'               { $$ = new AstPast{$1, $3, nullptr, GRAMMARP->create_Global_Clock_Sen_ਰੁੱਖ($1)}; }
        |       yD_POW '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'            { $$ = new AstPowD{$1, $3, $5}; }
        |       yD_RANDOM '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstRand{$1, $3, false}; }
        |       yD_RANDOM parenE                        { $$ = new AstRand{$1, nullptr, false}; }
        |       yD_REALTIME parenE                      { $$ = new Ast_ਸਮਾਂ_D{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_REALTOBITS '(' ਵਿਅੰਜਕ ')'              { $$ = new Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ{$1, $3}; }
        |       yD_REWIND '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstFSeek{$1, $3, new AstConst{$1, 0}, new AstConst{$1, 0}}; }
        |       y_D_ਸੱਜਾ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'         { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਸੱਜਾ, $3, nullptr}; }
        |       y_D_ਸੱਜਾ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')'        { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਸੱਜਾ, $3, $5}; }
        |       yD_RISING_GCLK '(' ਵਿਅੰਜਕ ')'             { $$ = new AstRising{$1, $3}; }
        |       yD_ROSE '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstRose{$1, $3, nullptr}; }
        |       yD_ROSE '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'           { $$ = new AstRose{$1, $3, GRAMMARP->create_Sen_ਰੁੱਖ_ਬਦਲਿਆ($1, $5)}; }
        |       yD_ROSE_GCLK '(' ਵਿਅੰਜਕ ')'               { $$ = new AstRose{$1, $3, GRAMMARP->create_Global_Clock_Sen_ਰੁੱਖ($1)}; }
        |       yD_RTOI '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstRToIS{$1, $3}; }
        |       yD_SAMPLED '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstSampled{$1, $3, $3->dtypep()}; }
        |       yD_SFORMATF '(' ਵਿਅੰਜਕ_Disp_List ')'        { $$ = new AstSFormatF{$1, AstSFormatF::ਵਿਅੰਜਕ_Format{}, $3, 'd', false}; }
        |       yD_SHORTREALTOBITS '(' ਵਿਅੰਜਕ ')'         { $$ = new Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ{$1, $3}; UNSUPREAL($1); }
        |       yD_SIGNED '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstSigned{$1, $3}; }
        |       yD_SIN '(' ਵਿਅੰਜਕ ')'                     { $$ = new AstSinD{$1, $3}; }
        |       yD_SINH '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstSinhD{$1, $3}; }
        |       y_D_ਮਾਪ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'          { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਮਾਪ, $3, nullptr}; }
        |       y_D_ਮਾਪ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ',' ਵਿਅੰਜਕ ')' { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਮਾਪ, $3, $5}; }
        |       yD_SQRT '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstSqrtD{$1, $3}; }
        |       yD_SSCANF '(' ਵਿਅੰਜਕ ',' str commaVRDListE ')'    { $$ = new AstSScanF{$1, *$5, $3, $6}; }
        |       yD_STABLE '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstStable{$1, $3, nullptr}; }
        |       yD_STABLE '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'         { $$ = new AstStable{$1, $3, GRAMMARP->create_Sen_ਰੁੱਖ_ਬਦਲਿਆ($1, $5)}; }
        |       yD_STABLE_GCLK '(' ਵਿਅੰਜਕ ')'             { $$ = new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_ਰੁੱਖ($1)}; }
        |       yD_STEADY_GCLK '(' ਵਿਅੰਜਕ ')'             { $$ = new AstSteady{$1, $3}; }
        |       yD_STIME parenE
                        { $$ = new AstSel{$1, new Ast_ਸਮਾਂ{$1, VTimescale{VTimescale::NONE}}, 0, 32}; }
        |       yD_TAN '(' ਵਿਅੰਜਕ ')'                     { $$ = new AstTanD{$1, $3}; }
        |       yD_TANH '(' ਵਿਅੰਜਕ ')'                    { $$ = new AstTanhD{$1, $3}; }
        |       yD_TESTPLUSARGS '(' ਵਿਅੰਜਕ ')'            { $$ = new Ast_Test_Plus_ਦਲੀਲਾਂ{$1, $3}; }
        |       y_D_ਸਮਾਂ parenE                          { $$ = new Ast_ਸਮਾਂ{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_TIMEPRECISION                        { $$ = new Ast_ਸਮਾਂ_Precision{$1}; }
        |       yD_TIMEPRECISION '(' ')'                { $$ = new Ast_ਸਮਾਂ_Precision{$1}; }
        |       yD_TIMEPRECISION '(' idClassSel ')'     { $$ = new Ast_ਸਮਾਂ_Precision{$1}; DEL($3); }
        |       yD_TIMEUNIT                             { $$ = new Ast_ਸਮਾਂ_Unit{$1}; }
        |       yD_TIMEUNIT '(' ')'                     { $$ = new Ast_ਸਮਾਂ_Unit{$1}; }
        |       yD_TIMEUNIT '(' idClassSel ')'          { $$ = new Ast_ਸਮਾਂ_Unit{$1}; DEL($3); }
        |       yD_TYPENAME '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'      { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::TYPENAME, $3}; }
        |       yD_UNGETC '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'         { $$ = new AstFUngetC{$1, $5, $3}; }  // Arg swap to file first
        |       yD_UNPACKED_DIMENSIONS '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'   { $$ = new Ast_Attr_ਦਾ{$1, VAttrType::DIM_UNPK_DIMENSIONS, $3}; }
        |       yD_UNSIGNED '(' ਵਿਅੰਜਕ ')'                { $$ = new AstUnsigned{$1, $3}; }
        |       yD_URANDOM '(' ਵਿਅੰਜਕ ')'                 { $$ = new AstRand{$1, $3, true}; }
        |       yD_URANDOM parenE                       { $$ = new AstRand{$1, nullptr, true}; }
        |       yD_URANDOM_RANGE '(' ਵਿਅੰਜਕ ')'           { $$ = new AstURandomRange{$1, $3, new AstConst{$1, 0}}; }
        |       yD_URANDOM_RANGE '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'  { $$ = new AstURandomRange{$1, $3, $5}; }
        |       yD_VALUEPLUSARGS '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ ')'  { $$ = new Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ{$1, $3, $5}; }
        ;

severity_system_task<nodep>: // IEEE: severity_system_task/elaboration_severity_system_task (1800-2009)
        //                      // Elaboration-time task; V3Width evaluates and removes it
                severity_system_task_guts ';'           { $$ = $1; }
        ;

severity_system_task_guts<nodep>:    // IEEE: part of severity_system_task (1800-2009)
        //                      // $fatal first argument is exit number, must be constant
                yD_INFO parenE                          { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, nullptr}; }
        |       yD_INFO '(' ਵਿਅੰਜਕ_List ')'                { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, $3}; }
        |       yD_WARNING parenE                       { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, nullptr}; }
        |       yD_WARNING '(' ਵਿਅੰਜਕ_List ')'             { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, $3}; }
        |       yD_ERROR parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, nullptr}; }
        |       yD_ERROR '(' ਵਿਅੰਜਕ_List ')'               { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, $3}; }
        |       yD_FATAL parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; }
        |       yD_FATAL '(' ਵਿਅੰਜਕ ')'                   { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; DEL($3); }
        |       yD_FATAL '(' ਵਿਅੰਜਕ ',' ਵਿਅੰਜਕ_List_E ')'     { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, $5}; DEL($3); }
        ;

system_Dpi_ਦਲੀਲਾਂ_E<argp>:           // IEEE: part of system_if_call for arguments of $dpi call
                parenE                                  { $$ = nullptr; }
        |       '(' ਵਿਅੰਜਕ_List ')'                        { $$ = GRAMMARP->argWrapList($2); }
        ;

property_actual_arg<ਗੰਢ_Exprp>:  // ==IEEE: property_actual_arg
        //                      // IEEE: property_expr
        //                      // IEEE: sequence_actual_arg
        //UNSUP pev_expr                                { $$ = $1; }
        //UNSUP remove below:
                pexpr                                   { $$ = $1; }
        //                      // IEEE: sequence_expr
        //                      // property_expr already includes sequence_expr
        ;

ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type<nodep>:          // expr | data_type: combined to prevent conflicts
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        //                      // data_type includes id that overlaps expr, so special flavor
        //                      // data_type expanded:
        |       ਅੰਕੜੇ_type_No_Ref                          { $$ = $1; }
        //
        //                      // Conflicts with non-type id, resolved in V3LinkDot
        //                      // NO: packageClassScopeE idType packed_dimensionListE
        //
        |       packageClassScopeE idType parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, $3};
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

list_ਦਾ_arguments_E<argp>:  // IEEE: [list_of_arguments]
                ਦਲੀਲਾਂ_Dotted_List                          { $$ = $1; }
        |       ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List_E
                        { ਜੇ ($1->ਖਾਲੀ_ਥਾਵਾਂ_Connect_No_ਅਗਲਾ()) {
                              $1->delete_ਰੁੱਖ(); $$ = nullptr;  // Mis-created when have 'func()'
                          } ਨਹੀਂਤਾਂ { $$ = $1; } }
        |       ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List_E ',' ਦਲੀਲਾਂ_Dotted_List        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

task_declaration<ਗੰਢ_FTaskp>:   // ==IEEE: task_declaration
                yTASK dynamic_override_specifiersE lifetimeE taskId tfGuts yENDTASK ਸਿਰਾ_Label_E
                        { $$ = $4; $$->ਜੋੜੋ_Stmtsp($5);
                          $$->ਅਧਾਰ_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>7, $$, $7); }
        ;

task_prototype<ਗੰਢ_FTaskp>:             // ==IEEE: task_prototype
                yTASK dynamic_override_specifiersE taskId '(' tf_ਪੋਰਟ_list_E ')'
                        { $$ = $3; $$->ਜੋੜੋ_Stmtsp($5); $$->prototype(true); }
        |       yTASK dynamic_override_specifiersE taskId
                        { $$ = $3; $$->prototype(true); }
        ;

function_declaration<ਗੰਢ_FTaskp>:       // IEEE: function_declaration + function_body_declaration
                yFUNCTION dynamic_override_specifiersE lifetimeE funcId funcIsolateE tfGuts yENDFUNCTION ਸਿਰਾ_Label_E
                        { $$ = $4; $$->ਜੋੜੋ_Stmtsp($6);
                          $$->ਅਧਾਰ_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>8, $$, $8); }
        |       yFUNCTION dynamic_override_specifiersE lifetimeE funcIdNew funcIsolateE tfNewGuts yENDFUNCTION ਸਿਰਾ_Label_E
                        { $$ = $4; $$->ਜੋੜੋ_Stmtsp($6);
                          $$->ਅਧਾਰ_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>8, $$, $8); }
        ;

function_prototype<ਗੰਢ_FTaskp>: // IEEE: function_prototype
                yFUNCTION dynamic_override_specifiersE funcId '(' tf_ਪੋਰਟ_list_E ')'
                        { $$ = $3; $$->ਜੋੜੋ_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcId
                        { $$ = $3; $$->prototype(true); }
        ;

class_constructor_prototype<ਗੰਢ_FTaskp>:        // ==IEEE: class_constructor_prototype
        //                      // IEEE has no dynamic_override_specifiersE,
        //                      // but required to avoid conflicts, so we must check after parsing
                yFUNCTION dynamic_override_specifiersE funcIdNew '(' class_constructor_arg_listE ')' ';'
                        { $$ = $3; $$->ਜੋੜੋ_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcIdNew ';'
                        { $$ = $3; $$->prototype(true); }
        ;

funcIsolateE<cint>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = 0; }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = 1; }
        ;

ਵਿਧੀ_prototype<ਗੰਢ_FTaskp>:
                task_prototype                          { $$ = $1; }
        |       function_prototype                      { $$ = $1; }
        ;

lifetimeE<lifetime>:            // IEEE: [lifetime]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = VLifetime::NONE; }
        |       lifetime                                { $$ = $1; }
        ;

lifetime<lifetime>:             // ==IEEE: lifetime
        //                      // Note lifetime used by members is instead under memberQual
                ySTATIC__ETC                            { $$ = VLifetime::STATIC_EXPLICIT; }
        |       yAUTOMATIC                              { $$ = VLifetime::AUTOMATIC_EXPLICIT; }
        ;

taskId<ਗੰਢ_FTaskp>:
                id
                        { $$ = new AstTask{$<fl>$, *$1, nullptr};
                          $$->verilogTask(true); }
        //
        |       id/*interface_identifier*/ '.' idAny
                        { $$ = new AstTask{$<fl>$, *$3, nullptr};
                          $$->verilogTask(true);
                          $$->iface_ਪੋਰਟ_Name(*$1); }
        //
        |       packageClassScope id
                        { $$ = new AstTask{$<fl>$, *$2, nullptr};
                          $$->verilogTask(true);
                          $$->classOrPackagep($1);
                          $$->class_ਵਿਧੀ(true); }
        ;

funcId<ਗੰਢ_FTaskp>:             // IEEE: function_data_type_or_implicit + part of function_body_declaration
        //                      // IEEE: function_data_type_or_implicit must be expanded here to prevent conflict
        //                      // function_data_type expanded here to prevent conflicts with
        //                      // implicit_type:empty vs data_type:ID
                /**/ fIdScoped
                        { $$ = $1;
                          $$->fvarp(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT}); }
        |       signingE rangeList fIdScoped
                        { $$ = $3;
                          $$->fvarp(GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)); }
        |       signing fIdScoped
                        { $$ = $2;
                          $$->fvarp(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}); }
        |       ਅੰਕੜੇ_type_No_Ref fIdScoped
                        { $$ = $2;
                          $$->fvarp($1); }
        |       packageClassScopeE idInstType packed_dimensionListE fIdScoped
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = $4;
                          $$->fvarp(GRAMMARP->createArray(refp, $3, true)); }
        |       packageClassScopeE idInstType parameter_ਮੁੱਲ_assignment_Class packed_dimensionListE fIdScoped
                        { AstRefDType* ਅਚਲ refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = $5;
                          $$->fvarp(GRAMMARP->createArray(refp, $4, true)); }
        //                      // To verilator tasks are the same as void functions (we separately detect time passing)
        |       yVOID taskId
                        { $$ = $2;  // Internals represent it as a task, not a function (TODO cleanup)
                          $$->verilogTask(false);
                          $$->verilogFunction(true); }
        ;

funcIdNew<ਗੰਢ_FTaskp>:          // IEEE: from class_constructor_declaration
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
                          $$->class_ਵਿਧੀ(true); }
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
                          $$->iface_ਪੋਰਟ_Name(*$1); }
        //
        |       packageClassScope id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstFunc{$<fl>$, *$2, nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->class_ਵਿਧੀ(true);
                          $$->classOrPackagep($1); }
        ;

tfGuts<nodep>:
                '(' tf_ਪੋਰਟ_list_E ')' ';' tfBodyE       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfNewGuts<nodep>:
                '(' class_constructor_arg_listE ')' ';' tfBodyE       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfBodyE<nodep>:                 // IEEE: part of function_body_declaration/task_body_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       tf_ਵਸਤੂ_declaration_List                 { $$ = $1; }
        |       tf_ਵਸਤੂ_declaration_List stmtList        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        |       stmtList                                { $$ = $1; }
        ;

tf_ਵਸਤੂ_declaration_List<nodep>:
                tf_ਵਸਤੂ_declaration                     { $$ = $1; }
        |       tf_ਵਸਤੂ_declaration_List tf_ਵਸਤੂ_declaration     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

tf_ਵਸਤੂ_declaration<nodep>:     // ==IEEE: tf_item_declaration
                ਖੰਡ_ਵਸਤੂ_declaration                  { $$ = $1; }
        |       tf_ਪੋਰਟ_declaration                     { $$ = $1; }
        |       tf_ਵਸਤੂ_declaration_Verilator            { $$ = $1; }
        ;

tf_ਵਸਤੂ_declaration_Verilator<nodep>:    // Verilator extensions
                y_VL_ਜਨਤਕ                              { $$ = new AstPragma{$1, VPragmaType::ਜਨਤਕ_TASK}; v3Global.dpi(true); }
        |       yVL_NO_INLINE_TASK                      { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_TASK}; }
        ;

tf_ਪੋਰਟ_list_E<nodep>:           // IEEE: tf_port_list + empty
        //                      // Empty covered by tf_port_item
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(ਨਿਵੇਸ਼); }
        /*cont*/    tf_ਪੋਰਟ_list_List                    { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

tf_ਪੋਰਟ_list_List<nodep>:        // IEEE: part of tf_port_list
                tf_ਪੋਰਟ_ਵਸਤੂ                            { $$ = $1; }
        |       tf_ਪੋਰਟ_list_List ',' tf_ਪੋਰਟ_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

class_constructor_arg_listE<nodep>:  // IEEE: class_constructor_arg_list or empty
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(ਨਿਵੇਸ਼); }
        /*cont*/    class_constructor_arg_listList      { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

class_constructor_arg_listList<nodep>:  // IEEE: part of class_constructor_arg_list
                class_constructor_arg                   { $$ = $1; }
        |       class_constructor_arg_listList ',' class_constructor_arg       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

class_constructor_arg<nodep>:  // ==IEEE: class_constructor_arg
                tf_ਪੋਰਟ_ਵਸਤੂ                            { $$ = $1; }
        |       yDEFAULT                                { $$ = nullptr; BBUNSUP($1, "Unsupported: new constructor 'default' argument"); }
        ;

tf_ਪੋਰਟ_ਵਸਤੂ<nodep>:            // ==IEEE: tf_port_item
        //                      // We split tf_port_item into the type and assignment as don't know what follows a comma
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; PINNUMINC(); }  // For example a ",," port
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Front tf_ਪੋਰਟ_ਵਸਤੂ_Assignment { $$ = $2; }
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Assignment                  { $$ = $1; }
        ;

tf_ਪੋਰਟ_ਵਸਤੂ_Front:              // IEEE: part of tf_port_item, which has the data type
                ਅੰਕੜੇ_type                               { VARDTYPE($1); }
        |       signingE rangeList
                        { Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                              new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true);
                          VARDTYPE(dtp); }
        |       signing
                        { Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1};
                          VARDTYPE(dtp); }
        |       yVAR ਅੰਕੜੇ_type                          { VARDTYPE($2); }
        |       yVAR implicit_typeE                     { VARDTYPE($2); }
        //
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Dir /*implicit*/            { VARDTYPE(nullptr); /*default_nettype-see spec*/ }
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Dir ਅੰਕੜੇ_type               { VARDTYPE($2); }
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Dir signingE rangeList
                        { Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                              new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE(dtp); }
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Dir signing
                        { Ast_ਗੰਢ_DType* ਅਚਲ dtp = new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $2};
                          VARDTYPE(dtp); }
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Dir yVAR ਅੰਕੜੇ_type          { VARDTYPE($3); }
        |       tf_ਪੋਰਟ_ਵਸਤੂ_Dir yVAR implicit_typeE     { VARDTYPE($3); }
        ;

tf_ਪੋਰਟ_ਵਸਤੂ_Dir:                // IEEE: part of tf_port_item, direction
                ਪੋਰਟ_direction                          { }  // port_direction sets VARIO
        ;

tf_ਪੋਰਟ_ਵਸਤੂ_Assignment<varp>:   // IEEE: part of tf_port_item, which has assignment
                id variable_dimensionListE sigAttrListE ਵਿਅੰਜਕ_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); ਜੇ ($4) $$->valuep($4); }
        ;

parenE:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { }
        |       '(' ')'                                 { }
        ;

//      method_call:            // ==IEEE: method_call + method_call_body
//                              // IEEE: method_call_root '.' method_identifier [ '(' list_of_arguments ')' ]
//                              //   "method_call_root '.' method_identifier" looks just like "expr '.' id"
//                              //   "method_call_root '.' method_identifier (...)" looks just like "expr '.' tf_call"
//                              // IEEE: built_in_method_call
//                              //   method_call_root not needed, part of expr resolution
//                              // What's left is below array_methodNoRoot
array_ਵਿਧੀ_No_ਜੜ੍ਹ<ਗੰਢ_FTask_Refp>:
                yOR                                     { $$ = new AstFuncRef{$1, "or"}; }
        |       yAND                                    { $$ = new AstFuncRef{$1, "and"}; }
        |       yXOR                                    { $$ = new AstFuncRef{$1, "xor"}; }
        |       yUNIQUE                                 { $$ = new AstFuncRef{$1, "unique"}; }
        ;

array_ਵਿਧੀ_With<ਗੰਢ_Exprp>:
                array_ਵਿਧੀ_No_ਜੜ੍ਹ parenE               { $$ = $1; }
        |       array_ਵਿਧੀ_No_ਜੜ੍ਹ parenE yWITH__PAREN '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstWithParse{$3, $1, $5}; }
        |       array_ਵਿਧੀ_No_ਜੜ੍ਹ '(' ਵਿਅੰਜਕ ')' yWITH__PAREN '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstWithParse{$5, $1, $7}; $1->ਜੋੜੋ_Argsp(new AstArg{$<fl>3, "", $3}); }
        ;

dpi_import_export<nodep>:       // ==IEEE: dpi_import_export
                yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype ';'
                        { $$ = $5;
                          ਜੇ (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_ਸ਼ੁੱਧ($3 == iprop_ਸ਼ੁੱਧ);
                          $5->dpiImport(true);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype yVL_DPI_C_DECL yaSTRING ';'
                        { $$ = $5;
                          ਜੇ (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_ਸ਼ੁੱਧ($3 == iprop_ਸ਼ੁੱਧ);
                          $5->dpiImport(true);
                          ਜੇ (*$7 != "") $5->dpiCDecl(*$7);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE task_prototype ';'
                        { $$ = $5;
                          ਜੇ (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_ਸ਼ੁੱਧ($3 == iprop_ਸ਼ੁੱਧ);
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { ਸਥਿਰ string s; $$ = &s; }
        |       idAny/*c_identifier*/ '='               { $$ = $1; $<fl>$ = $<fl>1; }
        ;

dpi_tf_import_propertyE<iprop>: // IEEE: [ dpi_function_import_property + dpi_task_import_property ]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = iprop_NONE; }
        |       yCONTEXT                                { $$ = iprop_CONTEXT; }
        |       y_ਸ਼ੁੱਧ                                   { $$ = iprop_ਸ਼ੁੱਧ; }
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

ਵਿਅੰਜਕ_Eq_E<ਗੰਢ_Exprp>:             // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' ਵਿਅੰਜਕ                                { $$ = $2; }
        ;

ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type_Eq_E<nodep>:       // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type                      { $$ = $2; }
        ;

const_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        ;

ਵਿਅੰਜਕ_E<ਗੰਢ_Exprp>:               // IEEE: optional expression
                /*empty*/                               { $$ = nullptr; }
        |       ਵਿਅੰਜਕ                                    { $$ = $1; }
        ;

ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:                // IEEE: part of expression/constant_expression/primary
        // *SEE BELOW*          // IEEE: primary/constant_primary
        //
        //                      // IEEE: unary_operator primary
                '+' ~r~ਵਿਅੰਜਕ     %prec prUNARYARITH      { $$ = $2; }
        |       '-' ~r~ਵਿਅੰਜਕ     %prec prUNARYARITH      { $$ = new AstNegate{$1, $2}; }
        |       '!' ~r~ਵਿਅੰਜਕ     %prec prNEGATION        { $$ = new AstLogNot{$1, $2}; }
        |       '&' ~r~ਵਿਅੰਜਕ     %prec prREDUCTION       { $$ = new AstRedAnd{$1, $2}; }
        |       '~' ~r~ਵਿਅੰਜਕ     %prec prNEGATION        { $$ = new AstNot{$1, $2}; }
        |       '|' ~r~ਵਿਅੰਜਕ     %prec prREDUCTION       { $$ = new AstRedOr{$1, $2}; }
        |       '^' ~r~ਵਿਅੰਜਕ     %prec prREDUCTION       { $$ = new AstRedXor{$1, $2}; }
        |       yP_NAND ~r~ਵਿਅੰਜਕ %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedAnd{$1, $2}}; }
        |       yP_NOR  ~r~ਵਿਅੰਜਕ %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedOr{$1, $2}}; }
        |       yP_XNOR ~r~ਵਿਅੰਜਕ %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedXor{$1, $2}}; }
        //
        //                      // IEEE: inc_or_dec_expression
        |       ~l~inc_or_dec_ਵਿਅੰਜਕ                { $<fl>$ = $<fl>1; $$ = $1; }
        //
        //                      // IEEE: '(' operator_assignment ')'
        //                      // Need exprScope of variable_lvalue to prevent conflict
        |       '(' ~p~ਵਿਅੰਜਕ_Scope '='          ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, $4},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)};
                          ASSIGNEQEXPR($<fl>3); }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_PLUSEQ    ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new Ast_ਜੋੜੋ{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_MINUSEQ   ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstSub{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_TIMESEQ   ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstMul{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_DIVEQ     ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstDiv{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_MODEQ     ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstModDiv{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_ANDEQ     ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstAnd{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_OREQ      ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstOr{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_XOREQ     ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new AstXor{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_SLEFTEQ   ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new Ast_ਖਿਸਕਾਓ_L{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_SRIGHTEQ  ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new Ast_ਖਿਸਕਾਓ_R{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        |       '(' ~p~ਵਿਅੰਜਕ_Scope yP_SSRIGHTEQ ਵਿਅੰਜਕ ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$1, new Ast_ਸੌਂਪੋ{$3, $2, new Ast_ਖਿਸਕਾਓ_RS{$3, $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true), $4}},
                                               $2->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(true)}; }
        //
        //                      // IEEE: expression binary_operator expression
        |       ~l~ਵਿਅੰਜਕ '+' ~r~ਵਿਅੰਜਕ                     { $$ = new Ast_ਜੋੜੋ{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '-' ~r~ਵਿਅੰਜਕ                     { $$ = new AstSub{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '*' ~r~ਵਿਅੰਜਕ                     { $$ = new AstMul{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '/' ~r~ਵਿਅੰਜਕ                     { $$ = new AstDiv{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '%' ~r~ਵਿਅੰਜਕ                     { $$ = new AstModDiv{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_EQUAL ~r~ਵਿਅੰਜਕ                { $$ = new AstEq{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_NOTEQUAL ~r~ਵਿਅੰਜਕ             { $$ = new AstNeq{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_CASEEQUAL ~r~ਵਿਅੰਜਕ            { $$ = new AstEqCase{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_CASENOTEQUAL ~r~ਵਿਅੰਜਕ         { $$ = new AstNeqCase{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_WILDEQUAL ~r~ਵਿਅੰਜਕ            { $$ = new AstEqWild{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_WILDNOTEQUAL ~r~ਵਿਅੰਜਕ         { $$ = new AstNeqWild{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_ANDAND ~r~ਵਿਅੰਜਕ               { $$ = new AstLogAnd{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_OROR ~r~ਵਿਅੰਜਕ                 { $$ = new AstLogOr{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_POW ~r~ਵਿਅੰਜਕ                  { $$ = new AstPow{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '<' ~r~ਵਿਅੰਜਕ                     { $$ = new AstLt{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '>' ~r~ਵਿਅੰਜਕ                     { $$ = new AstGt{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_GTE ~r~ਵਿਅੰਜਕ                  { $$ = new AstGte{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '&' ~r~ਵਿਅੰਜਕ                     { $$ = new AstAnd{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '|' ~r~ਵਿਅੰਜਕ                     { $$ = new AstOr{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '^' ~r~ਵਿਅੰਜਕ                     { $$ = new AstXor{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_XNOR ~r~ਵਿਅੰਜਕ                 { $$ = new AstNot{$2, new AstXor{$2, $1, $3}}; }
        |       ~l~ਵਿਅੰਜਕ yP_NOR ~r~ਵਿਅੰਜਕ
                        { $$ = new AstNot{$2, new AstOr{$2, $1, $3}};
                          $2->v3error("Syntax error: '~|' is not a 'nor' binary operator, but a unary '~ |'"
                                      << YYNEWLINE << $<fl>1->warn_ਹੋਰ() << "... Suggest '~ (... | ...)'"); }
        |       ~l~ਵਿਅੰਜਕ yP_NAND ~r~ਵਿਅੰਜਕ
                        { $$ = new AstNot{$2, new AstAnd{$2, $1, $3}};
                          $2->v3error("Syntax error: '~&' is not a 'nand' binary operator, but a unary '~ &'"
                                      << YYNEWLINE << $<fl>1->warn_ਹੋਰ() << "... Suggest '~ (... & ...)'"); }
        |       ~l~ਵਿਅੰਜਕ yP_SLEFT ~r~ਵਿਅੰਜਕ                { $$ = new Ast_ਖਿਸਕਾਓ_L{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_SRIGHT ~r~ਵਿਅੰਜਕ               { $$ = new Ast_ਖਿਸਕਾਓ_R{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_SSRIGHT ~r~ਵਿਅੰਜਕ              { $$ = new Ast_ਖਿਸਕਾਓ_RS{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yP_LTMINUSGT ~r~ਵਿਅੰਜਕ            { $$ = new AstLogEq{$2, $1, $3}; }
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
        |       ~l~ਵਿਅੰਜਕ yP_MINUSGT ~r~ਵਿਅੰਜਕ              { $$ = new AstLogIf{$2, $1, $3}; }
        //
        //                      // <= is special, as we need to disambiguate it with <= assignment
        //                      // We copy all of expr to fexpr and rename this token to a fake one.
        |       ~l~ਵਿਅੰਜਕ yP_LTE~f__IGNORE~ ~r~ਵਿਅੰਜਕ       { $$ = new AstLte{$2, $1, $3}; }
        //
        //                      // IEEE: conditional_expression
        |       ~l~ਵਿਅੰਜਕ '?' ~r~ਵਿਅੰਜਕ ':' ~r~ਵਿਅੰਜਕ         { $$ = new AstCond{$2, $1, $3, $5}; }
        //
        //                      // IEEE: inside_expression
        |       ~l~ਵਿਅੰਜਕ yINSIDE '{' range_list '}'      { $$ = new AstInside{$2, $1, $4}; }
        //
        //                      // IEEE: tagged_union_expression
        //                      // yTAGGED__NONPRIMARY = tokenPipeline determined no primary follows
        |       yTAGGED__NONPRIMARY idAny/*member*/ %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, nullptr}; }
        //                      // yTAGGED = primary follows; handle specific primary types
        //                      // Parenthesized expression
        |       yTAGGED idAny/*member*/ '(' ਵਿਅੰਜਕ ')' %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, $4}; }
        //                      // Assignment patterns like tagged Add '{a, b, c}
        |       yTAGGED idAny/*member*/ assignment_ਨਮੂਨਾ %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, $3}; }
        //                      // Integer literal
        |       yTAGGED idAny/*member*/ yaINTNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, new AstConst{$<fl>3, *$3}}; }
        //                      // Float literal
        |       yTAGGED idAny/*member*/ yaFLOATNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, new AstConst{$<fl>3, AstConst::RealDouble{}, $3}}; }
        //                      // String literal
        |       yTAGGED idAny/*member*/ yaSTRING %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, new AstConst{$<fl>3, AstConst::VerilogStringLiteral{}, *$3}}; }
        //                      // null literal
        |       yTAGGED idAny/*member*/ yNULL %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, new AstConst{$3, AstConst::Null{}}}; }
        //                      // Identifier as value
        |       yTAGGED idAny/*member*/ idAny %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, new AstParseRef{$<fl>3, *$3, nullptr, nullptr}}; }
        //                      // Concatenation
        |       yTAGGED idAny/*member*/ '{' cateList '}' %prec prTAGGED
                        { $$ = new Ast_Tagged_ਵਿਅੰਜਕ{$1, *$2, $4}; }
        //
        //======================// IEEE: primary/constant_primary
        //
        //                      // IEEE: primary_literal (minus string, which is handled specially)
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       ਸਮਾਂ_ਅੰਕ_Adjusted                         { $$ = $1; }
        |       strAsInt~noStr__IGNORE~                 { $$ = $1; }
        //
        //                      // IEEE: "... hierarchical_identifier select"  see below
        //
        //                      // IEEE: empty_unpacked_array_concatenation
        //                      // IEEE: (aka empty_queue, empty_unpacked_array_concatenation)
        |       '{' '}'                                 { $$ = new Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ{$1}; }
        //
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Part of exprOkLvalue below
        //
        //                      // IEEE: multiple_concatenation/constant_multiple_concatenation
        |       '{' const_ਵਿਅੰਜਕ '{' cateList '}' '}'
                        { $$ = new AstReplicate{$3, $4, $2}; }
        |       '{' const_ਵਿਅੰਜਕ '{' cateList '}' '}' '[' ਵਿਅੰਜਕ ']'
                        { $$ = new Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{$7, new AstReplicate{$3, $4, $2}, $8}; }
        |       '{' const_ਵਿਅੰਜਕ '{' cateList '}' '}' '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                        { $$ = new Ast_Sel_ਕੱਢੋ{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_ਵਿਅੰਜਕ '{' cateList '}' '}' '[' ਵਿਅੰਜਕ yP_PLUSCOLON const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSelPlus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_ਵਿਅੰਜਕ '{' cateList '}' '}' '[' ਵਿਅੰਜਕ yP_MINUSCOLON const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSelMinus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        //                      // UNSUP some other rules above
        //
        //                      // IEEE grammar error: function_subroutine_call [ range_expression ]
        //                      // should be instead:  function_subroutine_call [ part_select_range ]
        |       function_subroutine_call_No_ਵਿਧੀ
                        { $$ = $1; }
        |       function_subroutine_call_No_ਵਿਧੀ part_select_rangeList
                        { $$ = GRAMMARP->scrubSel($1, $2); }
        //                      // method_call
        |       ~l~ਵਿਅੰਜਕ '.' function_subroutine_call_No_ਵਿਧੀ
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '.' function_subroutine_call_No_ਵਿਧੀ part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //                      // method_call:array_method requires a '.'
        |       ~l~ਵਿਅੰਜਕ '.' array_ਵਿਧੀ_With
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ '.' array_ਵਿਧੀ_With part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //
        //                      // IEEE: let_expression
        //                      // see funcRef
        //
        //                      // IEEE: '(' mintypmax_expression ')'
        |       ~noPar__IGNORE~'(' ਵਿਅੰਜਕ ')'             { $$ = $2; }
        |       ~noPar__IGNORE~'(' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ')'
                        { $$ = $4; MINTYPMAXDLYUNSUP($4); DEL($2); DEL($6); }
        //                      // PSL rule
        |       '_' '(' ਵਿਅੰਜਕ ')'                        { $$ = $3; }    // Arbitrary Verilog inside PSL
        //
        //                      // IEEE: cast/constant_cast
        //                      // expanded from casting_type
        |       simple_typeNoRef yP_TICK '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstCast{$2, $4, VFlag_ਬੱਚਾ_DType{}, $1}; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // Causes conflict, so handled post-parse
        //                      // NO: packageClassScopeE idType yP_TICK '(' expr ')'
        //
        |       yTYPE__ETC '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')' yP_TICK '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstCast{$1, $7, VFlag_ਬੱਚਾ_DType{},
                                           new AstRefDType{$1, AstRefDType::Flag_Type_ਦਾ_ਵਿਅੰਜਕ{}, $3}}; }
        |       ySIGNED yP_TICK '(' ਵਿਅੰਜਕ ')'            { $$ = new AstSigned{$1, $4}; }
        |       yUNSIGNED yP_TICK '(' ਵਿਅੰਜਕ ')'          { $$ = new AstUnsigned{$1, $4}; }
        |       ySTRING yP_TICK '(' ਵਿਅੰਜਕ ')'            { $$ = new AstCvtPackString{$1, $4}; }
        |       yCONST__ETC yP_TICK '(' ਵਿਅੰਜਕ ')'        { $$ = $4; }  // Not linting const presently
        //                      // Spec only allows primary with addition of a type reference
        //                      // We'll be more general, and later assert LHS was a type.
        |       ~l~ਵਿਅੰਜਕ yP_TICK '(' ਵਿਅੰਜਕ ')'            { $$ = new AstCastParse{$2, $4, $1}; }
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
        |       ~l~ਵਿਅੰਜਕ_Ok_Lvalue                         { $$ = $1; }
        //
        //----------------------
        //
        //                      // IEEE: cond_predicate - here to avoid reduce problems
        //                      // Note expr includes cond_pattern
        |       ~l~ਵਿਅੰਜਕ yP_ANDANDAND ~r~ਵਿਅੰਜਕ            { $$ = new AstConst{$2, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
                                                          BBUNSUP($<fl>2, "Unsupported: &&& expression"); }
        //
        //                      // IEEE: cond_pattern - here to avoid reduce problems
        //                      // "expr yMATCHES pattern"
        //                      // IEEE: pattern - expanded here to avoid conflicts
        |       ~l~ਵਿਅੰਜਕ yMATCHES ਨਮੂਨਾ_No_ਵਿਅੰਜਕ          { $$ = new AstMatches{$2, $1, $3}; }
        |       ~l~ਵਿਅੰਜਕ yMATCHES ~r~ਵਿਅੰਜਕ                { $$ = new AstMatches{$2, $1, $3}; }
        //
        //                      // IEEE: expression_or_dist - here to avoid reduce problems
        //                      // "expr yDIST '{' dist_list '}'"
        |       ~l~ਵਿਅੰਜਕ yDIST '{' dist_list '}'         { $$ = new AstDist{$2, $1, $4}; }
        ;

fexpr<ਗੰਢ_Exprp>:                   // For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(ਵਿਅੰਜਕ,{s/~l~/f/g; s/~r~/f/g; s/~f__IGNORE~/__IGNORE/g;})   // {copied}
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

ਵਿਅੰਜਕ_No_Str<ਗੰਢ_Exprp>:               // expression with string removed
                BISONPRE_COPY(ਵਿਅੰਜਕ,{s/~noStr__IGNORE~/Ignore/g;})       // {copied}
        ;

ਵਿਅੰਜਕ_Ok_Lvalue<ਗੰਢ_Exprp>:            // expression that's also OK to use as a variable_lvalue
                ~l~ਵਿਅੰਜਕ_Scope                            { $$ = $1; }
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Replicate(1) required as otherwise "{a}" would not be self-determined
        |       '{' cateList '}'                        { $$ = new AstReplicate{$1, $2, 1}; }
        |       '{' cateList '}' '[' ਵਿਅੰਜਕ ']'           { $$ = new Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{$4, new AstReplicate{$1, $2, 1}, $5}; }
        |       '{' cateList '}' '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                                                        { $$ = new Ast_Sel_ਕੱਢੋ{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' ਵਿਅੰਜਕ yP_PLUSCOLON const_ਵਿਅੰਜਕ ']'
                                                        { $$ = new AstSelPlus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' ਵਿਅੰਜਕ yP_MINUSCOLON const_ਵਿਅੰਜਕ ']'
                                                        { $$ = new AstSelMinus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        //                      // IEEE: assignment_pattern_expression
        //                      // IEEE: [ assignment_pattern_expression_type ] == [ ps_type_id /ps_paremeter_id/data_type]
        //                      // We allow more here than the spec requires
        //UNSUP ~l~exprScope assignment_pattern         { UNSUP }
        |       ਅੰਕੜੇ_type assignment_ਨਮੂਨਾ            { $$ = $2; ਜੇ ($2) $2->ਬੱਚਾ_DTypep($1); }
        |       assignment_ਨਮੂਨਾ                      { $$ = $1; }
        //
        |       streaming_concatenation                 { $$ = $1; }
        ;

fexprOkLvalue<ਗੰਢ_Exprp>:           // exprOkLValue, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(ਵਿਅੰਜਕ_Ok_Lvalue,{s/~l~/f/g}) // {copied}
        ;

sexprOkLvalue<ਗੰਢ_Exprp>:  // exprOkLValue, For use by sequence_expr
                BISONPRE_COPY(ਵਿਅੰਜਕ_Ok_Lvalue,{s/~l~/s/g}) // {copied}
        ;

pexprOkLvalue<ਗੰਢ_Exprp>:  // exprOkLValue, For use by property_expr
                BISONPRE_COPY(ਵਿਅੰਜਕ_Ok_Lvalue,{s/~l~/p/g}) // {copied}
        ;

//UNSUPev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/ev_/g})       // {copied}
//UNSUP ;

//UNSUPpev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/pev_/g})      // {copied}
//UNSUP ;

fexprLvalue<ਗੰਢ_Exprp>:             // For use as first part of statement (disambiguates <=)
                fexprOkLvalue                           { $<fl>$ = $<fl>1; $$ = $1; }
        ;

ਵਿਅੰਜਕ_Scope<ਗੰਢ_Exprp>:               // scope and variable for use to inside an expression
        //                      // Here we've split method_call_root | implicit_class_handle | class_scope | package_scope
        //                      // from the object being called and let expr's "." deal with resolving it.
        //                      // (note method_call_root was simplified to require a primary in 1800-2009)
        //
        //                      // IEEE: [ implicit_class_handle . | class_scope | package_scope ] hierarchical_identifier select
        //                      // Or method_call_body without parenthesis
        //                      // See also varRefClassBit, which is the non-expr version of most of this
                yTHIS                                   { $$ = new AstParseRef{$<fl>1, "this"}; }
        |       y_D_ਜੜ੍ਹ                                 { $$ = new AstParseRef{$<fl>1, "$root"}; }
        |       idArrayed                               { $$ = $1; }
        |       packageClassScope idArrayed             { $$ = AstDot::newIfPkg($2->fileline(), $1, $2); }
        |       ~l~ਵਿਅੰਜਕ '.' idArrayed                   { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        //                      // expr below must be a "yTHIS"
        |       ~l~ਵਿਅੰਜਕ '.' ySUPER
                        { AstParseRef* ਅਚਲ anodep = VN_CAST($1, ParseRef);
                          ਜੇ (anodep && anodep->name() == "this") {
                              $$ = new AstParseRef{$<fl>1, "super"};
                              $1->delete_ਰੁੱਖ();
                          } ਨਹੀਂਤਾਂ {
                              $$ = $1;  $$->v3error("Syntax error: 'super' must be first name component, or after 'this.'");
                          }
                        }
        //                      // Part of implicit_class_handle
        |       ySUPER                                  { $$ = new AstParseRef{$<fl>1, "super"}; }
        ;

fexprScope<ਗੰਢ_Exprp>:              // exprScope, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(ਵਿਅੰਜਕ_Scope,{s/~l~/f/g})    // {copied}
        ;

sexprScope<ਗੰਢ_Exprp>:  // exprScope, For use by sequence_expr
                BISONPRE_COPY(ਵਿਅੰਜਕ_Scope,{s/~l~/s/g})    // {copied}
        ;

pexprScope<ਗੰਢ_Exprp>:  // exprScope, For use by property_expr
                BISONPRE_COPY(ਵਿਅੰਜਕ_Scope,{s/~l~/p/g})    // {copied}
        ;

//UNSUPev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/ev_/g})  // {copied}
//UNSUP ;

//UNSUPpev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/pev_/g}) // {copied}
//UNSUP ;

// PLI calls exclude "" as integers, they're strings
// For $c("foo","bar") we want "bar" as a string, not a Verilog integer.
ਵਿਅੰਜਕ_Str_ਪਾਠ<nodep>:
                ਵਿਅੰਜਕ_No_Str                               { $$ = $1; }
        |       yaSTRING                                { $$ = new Ast_ਪਾਠ{$<fl>1, GRAMMARP->ਪਾਠ_Quoted($<fl>1, *$1)}; }
        ;

ਵਿਅੰਜਕ_Type_Compare<ਗੰਢ_Exprp>:
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       type_referenceBoth                      { $$ = $1; }
        ;

cStrList<nodep>:
                ਵਿਅੰਜਕ_Str_ਪਾਠ                             { $$ = $1; }
        |       ਵਿਅੰਜਕ_Str_ਪਾਠ ',' cStrList                { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

cateList<ਗੰਢ_Exprp>:
        //                      // Not just 'expr' to prevent conflict via stream_concOrExprOrType
                ਧਾਰਾ_ਵਿਅੰਜਕ                       { $$ = $1; }
        |       cateList ',' ਧਾਰਾ_ਵਿਅੰਜਕ          { $$ = new AstConcat{$2, $1, $3}; }
        ;

ਵਿਅੰਜਕ_List_E<ਗੰਢ_Exprp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       ਵਿਅੰਜਕ_List                                { $$ = $1; }
        ;

ਵਿਅੰਜਕ_List<ਗੰਢ_Exprp>:
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       ਵਿਅੰਜਕ_List ',' ਵਿਅੰਜਕ                       { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

// identifier_list for 'with' in inline randomize constraints (IEEE 1800-2023 18.7).
// Only simple identifiers; non-identifier expressions are parse errors.
inlineConstraintIdList<ਗੰਢ_Exprp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        |       inlineConstraintIdList ',' id           { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(
                                                             new AstParseRef{$<fl>3, *$3, nullptr, nullptr}); }
        ;

// Optional identifier_list. Empty 'with () {...}' differs from bare 'with {...}'
// via AstWithParse::restricted().
inlineConstraintIdListE<ਗੰਢ_Exprp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       inlineConstraintIdList                  { $$ = $1; }
        ;

ਵਿਅੰਜਕ_EList_E<nodep>:  // expression list with empty commas allowed
                ਵਿਅੰਜਕ_E                                   { $$ = $1; }
        |       ਵਿਅੰਜਕ_EList_E ',' ਵਿਅੰਜਕ_E                    { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

ਵਿਅੰਜਕ_Disp_List<ਗੰਢ_Exprp>:            // exprList for within $display
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       ਵਿਅੰਜਕ_Disp_List ',' ਵਿਅੰਜਕ                   { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        //                      // ,, creates a space in $display
        |       ਵਿਅੰਜਕ_Disp_List ',' /*empty*/
                        { $$ = $1->ਜੋੜੋ_ਅਗਲਾ(new AstConst{$<fl>2, AstConst::VerilogStringLiteral{}, " "}); }
        ;

vrdList<ਗੰਢ_Exprp>:
                idClassSel                              { $$ = $1; }
        |       vrdList ',' idClassSel                  { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

commasE:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { } /* ignored */
        |       ',' commasE                             { } /* ignored */
        ;

commaVRDListE<ਗੰਢ_Exprp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       ',' vrdList                             { $$ = $2; }
        ;

ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List<ਗੰਢ_Exprp>:        // IEEE: part of list_of_arguments (used where ,, isn't legal)
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List ',' ਵਿਅੰਜਕ                   { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List_E<argp>:       // IEEE: part of list_of_arguments
                ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_One_E                            { $$ = $1; }
        |       ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_List_E ',' ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_One_E          { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

//UNSUPpev_argsExprListE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsExprOneE                        { $$ = $1; }
//UNSUP |       pev_argsExprListE ',' pev_argsExprOneE  { $$ = addNextNull($1, $3); }
//UNSUP ;

ਦਲੀਲਾਂ_ਵਿਅੰਜਕ_One_E<argp>:        // IEEE: part of list_of_arguments
                /*empty*/                               { $$ = new AstArg{CRELINE(), "", nullptr}; }
        |       ਵਿਅੰਜਕ                                    { $$ = new AstArg{$1->fileline(), "", $1}; }
        ;

//UNSUPpev_argsExprOneE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         /*empty*/                               { $$ = nullptr; }       // ,, is legal in list_of_arguments
//UNSUP |       pev_expr                                { $$ = $1; }
//UNSUP ;

ਦਲੀਲਾਂ_Dotted_List<argp>:      // IEEE: part of list_of_arguments
                ਦਲੀਲਾਂ_Dotted                              { $$ = $1; }
        |       ਦਲੀਲਾਂ_Dotted_List ',' ਦਲੀਲਾਂ_Dotted           { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

//UNSUPpev_argsDottedList<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsDotted                          { $$ = $1; }
//UNSUP |       pev_argsDottedList ',' pev_argsDotted   { $$ = addNextNull($1, $3); }
//UNSUP ;

ਦਲੀਲਾਂ_Dotted<argp>:          // IEEE: part of list_of_arguments
                '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
        |       '.' idAny '(' ਵਿਅੰਜਕ ')'                  { $$ = new AstArg{$<fl>2, *$2, $4}; }
        ;

//UNSUPpev_argsDotted<argp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
//UNSUP |       '.' idAny '(' pev_expr ')'              { $$ = new AstArg{$<fl>2, *$2, $4}; }
//UNSUP ;

streaming_concatenation<ਗੰਢ_Streamp>: // ==IEEE: streaming_concatenation
        //                      // Need to disambiguate {<< expr-{ ... expr-} stream_concat }
        //                      // From                 {<< stream-{ ... stream-} }
        //                      // Likewise simple_type's idScoped from constExpr's idScope
        //                      // Thus we allow always any two operations.  Sorry
        //                      // IEEE: "'{' yP_SL/R             stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R simple_type stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R constExpr   stream_concatenation '}'"
                '{' yP_SLEFT  ਧਾਰਾ_concatenation '}'
                        { $$ = new Ast_ਧਾਰਾ_L{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SRIGHT ਧਾਰਾ_concatenation '}'
                        { $$ = new Ast_ਧਾਰਾ_R{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SLEFT  ਧਾਰਾ_ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ਧਾਰਾ_concatenation '}'
                        { $$ = new Ast_ਧਾਰਾ_L{$2, $4, new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਬਿੱਟ_OR_ਸੰਖਿਆ, $3}}; }
        |       '{' yP_SRIGHT ਧਾਰਾ_ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ਧਾਰਾ_concatenation '}'
                        { $$ = new Ast_ਧਾਰਾ_R{$2, $4, new Ast_Attr_ਦਾ{$1, VAttrType::DIM_ਬਿੱਟ_OR_ਸੰਖਿਆ, $3}}; }
        ;

ਧਾਰਾ_concatenation<ਗੰਢ_Exprp>:    // ==IEEE: stream_concatenation
        //                      // '{' { stream_expression } '}'
                '{' cateList '}'                        { $$ = $2; }
        ;

ਧਾਰਾ_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:   // ==IEEE: stream_expression
        //                      // IEEE: array_range_expression expanded below
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ yP_PLUSCOLON  ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ yP_MINUSCOLON ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        ;

ਧਾਰਾ_ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type<nodep>:     // IEEE: from streaming_concatenation
                ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type                          { $$ = $1; }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ yP_PLUSCOLON  ਵਿਅੰਜਕ ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       ਵਿਅੰਜਕ yWITH__BRA '[' ਵਿਅੰਜਕ yP_MINUSCOLON ਵਿਅੰਜਕ ']'
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
                yLET letId '=' ਵਿਅੰਜਕ ';'
                        { $$ = $2;
                          $$->ਜੋੜੋ_Stmtsp(new Ast_Stmt_ਵਿਅੰਜਕ{$1, $4}); }
        |       yLET letId '(' let_ਪੋਰਟ_list_E ')' '=' ਵਿਅੰਜਕ ';'
                        { $$ = $2;
                          $$->ਜੋੜੋ_Stmtsp(new Ast_Stmt_ਵਿਅੰਜਕ{$1, $7});
                          $$->ਜੋੜੋ_Stmtsp($4); }
        ;

let_ਪੋਰਟ_list_E<nodep>:   // IEEE: [ let_port_list ]
                /*empty*/                               { $$ = nullptr; }
        |       /*emptyStart*/
        /*mid*/         { VARRESET_LIST(VAR); VARIO(INOUT); }
        /*cont*/  let_ਪੋਰਟ_list                         { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

let_ਪੋਰਟ_list<nodep>:   // IEEE: let_port_list
                let_ਪੋਰਟ_ਵਸਤੂ                           { $$ = $1; }
        |       let_ਪੋਰਟ_list ',' let_ਪੋਰਟ_ਵਸਤੂ         { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

let_ਪੋਰਟ_ਵਸਤੂ<varp>:  // IEEE: let_port_Item
        //                      // IEEE: Expanded let_formal_type
                yUNTYPED idAny/*formal_port_identifier*/ variable_dimensionListE ਵਿਅੰਜਕ_Eq_E
                        { $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_ਬੱਚਾ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          ਜੇ ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       ਅੰਕੜੇ_type idAny/*formal_port_identifier*/ variable_dimensionListE ਵਿਅੰਜਕ_Eq_E
                        { BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_ਬੱਚਾ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          ਜੇ ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       implicit_typeE id/*formal_port_identifier*/ variable_dimensionListE ਵਿਅੰਜਕ_Eq_E
                        { ਜੇ ($1) BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlag_ਬੱਚਾ_DType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          ਜੇ ($4) $$->valuep($4);
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
        |       gateBufList ',' gateBuf                 { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateBufif0List<nodep>:
                gateBufif0                              { $$ = $1; }
        |       gateBufif0List ',' gateBufif0           { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateBufif1List<nodep>:
                gateBufif1                              { $$ = $1; }
        |       gateBufif1List ',' gateBufif1           { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateNotList<nodep>:
                gateNot                                 { $$ = $1; }
        |       gateNotList ',' gateNot                 { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateNotif0List<nodep>:
                gateNotif0                              { $$ = $1; }
        |       gateNotif0List ',' gateNotif0           { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateNotif1List<nodep>:
                gateNotif1                              { $$ = $1; }
        |       gateNotif1List ',' gateNotif1           { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateAndList<nodep>:
                gateAnd                                 { $$ = $1; }
        |       gateAndList ',' gateAnd                 { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateNandList<nodep>:
                gateNand                                { $$ = $1; }
        |       gateNandList ',' gateNand               { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateOrList<nodep>:
                gateOr                                  { $$ = $1; }
        |       gateOrList ',' gateOr                   { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateNorList<nodep>:
                gateNor                                 { $$ = $1; }
        |       gateNorList ',' gateNor                 { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateXorList<nodep>:
                gateXor                                 { $$ = $1; }
        |       gateXorList ',' gateXor                 { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateXnorList<nodep>:
                gateXnor                                { $$ = $1; }
        |       gateXnorList ',' gateXnor               { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gatePullupList<nodep>:
                gatePullup                              { $$ = $1; }
        |       gatePullupList ',' gatePullup           { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gatePulldownList<nodep>:
                gatePulldown                            { $$ = $1; }
        |       gatePulldownList ',' gatePulldown       { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;
gateUnsupList<nodep>:
                gateUnsup                               { $$ = $1; }
        |       gateUnsupList ',' gateUnsup             { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

gateRangeE<nodep>:
                instRangeListE                          { $$ = $1; GATERANGE(GRAMMARP->scrubRange($1)); }
        ;

gateBuf<nodep>:
                gateFront variable_lvalue ',' ਵਿਅੰਜਕ_List ')'
                        { Ast_ਗੰਢ_ਵਿਅੰਜਕ* inp = $4;
                          ਜਦੋਂ (inp->nextp()) inp = VN_AS(inp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
                          $$ = new AstImplicit{$<fl>1, inp->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = GRAMMARP->createGatePin(inp->clone_ਰੁੱਖ(false));
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinRhsp = GRAMMARP->createGatePin(inp->clone_ਰੁੱਖ(false));
                              Ast_ਸੌਂਪੋ_W* ਅਚਲ pinAssp = new Ast_ਸੌਂਪੋ_W{$<fl>1, outp->clone_ਰੁੱਖ(false), pinRhsp};
                              $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{pinAssp});
                          }
                          DEL($1); DEL($4); }
        ;
gateNot<nodep>:
                gateFront variable_lvalue ',' ਵਿਅੰਜਕ_List ')'
                        { Ast_ਗੰਢ_ਵਿਅੰਜਕ* inp = $4;
                          ਜਦੋਂ (inp->nextp()) inp = VN_AS(inp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
                          $$ = new AstImplicit{$<fl>1, inp->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_ਰੁੱਖ(false))};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                              Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinRhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_ਰੁੱਖ(false))};
                              Ast_ਸੌਂਪੋ_W* ਅਚਲ pinAssp = new Ast_ਸੌਂਪੋ_W{$<fl>1, outp->clone_ਰੁੱਖ(false), pinRhsp};
                              $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{pinAssp});
                          }
                          DEL($1, $4); }
        ;
gateBufif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_ਵਿਅੰਜਕ ',' gate_Pin_ਵਿਅੰਜਕ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_ਰੁੱਖ(false)};
                          $<implicitp>$->ਜੋੜੋ_Exprsp($4->clone_ਰੁੱਖ(false));
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, $4};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateBufif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_ਵਿਅੰਜਕ ',' gate_Pin_ਵਿਅੰਜਕ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_ਰੁੱਖ(false)};
                          $<implicitp>$->ਜੋੜੋ_Exprsp($4->clone_ਰੁੱਖ(false));
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstBufIf1{$<fl>1, $6, $4};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_ਵਿਅੰਜਕ ',' gate_Pin_ਵਿਅੰਜਕ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_ਰੁੱਖ(false)};
                          $<implicitp>$->ਜੋੜੋ_Exprsp($4->clone_ਰੁੱਖ(false));
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, new AstNot{$<fl>1, $4}};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_ਵਿਅੰਜਕ ',' gate_Pin_ਵਿਅੰਜਕ ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_ਰੁੱਖ(false)};
                          $<implicitp>$->ਜੋੜੋ_Exprsp($4->clone_ਰੁੱਖ(false));
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstBufIf1{$<fl>1, $6, new AstNot{$<fl>1, $4}};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateAnd<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = $4;
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateNand<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstNot{$<fl>1, $4};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateOr<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = $4;
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateNor<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstNot{$<fl>1, $4};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateXor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = $4;
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
                          DEL($1); }
        ;
gateXnor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_ਰੁੱਖ(false)};
                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = new AstNot{$<fl>1, $4};
                          Ast_ਸੌਂਪੋ_W* ਅਚਲ ap = new Ast_ਸੌਂਪੋ_W{$<fl>1, $2, rhsp};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new AstAlways{ap});
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

gateAndPinList<ਗੰਢ_Exprp>:
                gate_Pin_ਵਿਅੰਜਕ                             { $$ = $1; }
        |       gateAndPinList ',' gate_Pin_ਵਿਅੰਜਕ          { $$ = new AstAnd{$2, $1, $3}; }
        ;
gateOrPinList<ਗੰਢ_Exprp>:
                gate_Pin_ਵਿਅੰਜਕ                             { $$ = $1; }
        |       gateOrPinList ',' gate_Pin_ਵਿਅੰਜਕ           { $$ = new AstOr{$2, $1, $3}; }
        ;
gateXorPinList<ਗੰਢ_Exprp>:
                gate_Pin_ਵਿਅੰਜਕ                             { $$ = $1; }
        |       gateXorPinList ',' gate_Pin_ਵਿਅੰਜਕ          { $$ = new AstXor{$2, $1, $3}; }
        ;
gateUnsupPinList<ਗੰਢ_Exprp>:
                gate_Pin_ਵਿਅੰਜਕ                             { $$ = $1; }
        |       gateUnsupPinList ',' gate_Pin_ਵਿਅੰਜਕ        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

gate_Pin_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:
                ਵਿਅੰਜਕ                                    { $$ = GRAMMARP->createGatePin($1); }
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       pulldown_strength                       { $$ = $1; }
        ;

pulldown_strength<nodep>:  // IEEE: pulldown_strength
                yP_PAR__STRENGTH strength0 ',' strength1 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength1 ',' strength0 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength0 ')'                { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        ;

pullup_strengthE<nodep>:  // IEEE: [ pullup_strength ]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
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
                y_ਸਾਰਣੀ ਸਾਰਣੀ_ਇੰਦਰਾਜ_List yENDTABLE         { $$ = new Ast_Udp_ਸਾਰਣੀ{$1, $2}; }
        ;

ਸਾਰਣੀ_ਇੰਦਰਾਜ_List<udp_ਸਾਰਣੀ_Linep>:  // IEEE: { combinational_entry + sequential_entry }
                ਸਾਰਣੀ_ਸਤਰ                              { $$ = $1; }
        |       ਸਾਰਣੀ_ਇੰਦਰਾਜ_List ਸਾਰਣੀ_ਸਤਰ               { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

ਸਾਰਣੀ_ਸਤਰ<udp_ਸਾਰਣੀ_Linep>:
                ਸਾਰਣੀ_ਨਿਵੇਸ਼_List ya_ਸਾਰਣੀ_LRSEP tablelVal ya_ਸਾਰਣੀ_LINEEND
                        { $$ = new Ast_Udp_ਸਾਰਣੀ_ਸਤਰ{Ast_Udp_ਸਾਰਣੀ_ਸਤਰ::UdpCombo{}, $<fl>1, $1, $3}; }
        |       ਸਾਰਣੀ_ਨਿਵੇਸ਼_List ya_ਸਾਰਣੀ_LRSEP tablelVal ya_ਸਾਰਣੀ_LRSEP tablelVal ya_ਸਾਰਣੀ_LINEEND
                        { $$ = new Ast_Udp_ਸਾਰਣੀ_ਸਤਰ{Ast_Udp_ਸਾਰਣੀ_ਸਤਰ::UdpSequential{}, $<fl>1, $1, $3, $5}; }
        ;

ਸਾਰਣੀ_ਨਿਵੇਸ਼_List<udp_ਸਾਰਣੀ_ਸਤਰ_Valp>:
                tablelVal                            { $$ = $1; }
        |       ਸਾਰਣੀ_ਨਿਵੇਸ਼_List tablelVal             { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

tablelVal<udp_ਸਾਰਣੀ_ਸਤਰ_Valp>:
                ya_ਸਾਰਣੀ_ਖੇਤਰ                          { $$ = new Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val{$<fl>1, *$1}; }
        |       '(' ya_ਸਾਰਣੀ_ਖੇਤਰ ya_ਸਾਰਣੀ_ਖੇਤਰ ')'    { $$ = new Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val{$<fl>2, *$2 + *$3}; }
        ;

//************************************************
// Specify

specify_ਖੰਡ<nodep>:               // ==IEEE: specify_block
                specifyFront specify_ਵਸਤੂ_List yENDSPECIFY   { $$ = $2; }
        |       specifyFront yENDSPECIFY                { $$ = nullptr; }
        ;

specifyFront:  // IEEE: specify_block front
                ySPECIFY                                { GRAMMARP->m_specifyignWarned = false; }
        ;

specify_ਵਸਤੂ_List<nodep>:            // IEEE: { specify_item }
                specify_ਵਸਤੂ                            { $$ = $1; }
        |       specify_ਵਸਤੂ_List specify_ਵਸਤੂ           { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

specify_ਵਸਤੂ<nodep>:                // ==IEEE: specify_item
                specparam_declaration                   { $$ = $1; }
        |       system_timing_check                     { $$ = $1; }
        |       junk_ਮੰਜ਼ਿਲ_Semi_List ';'
                        { $$ = nullptr;
                          ਜੇ (!GRAMMARP->m_specifyignWarned) {
                              GRAMMARP->m_specifyignWarned = true;
                              $1->v3warn(SPECIFYIGN, "Ignoring unsupported: specify block construct");
                          }
                        }
        ;

specparam_declaration<nodep>:       // ==IEEE: specparam_declaration
                specparam_declarationFront list_ਦਾ_specparam_assignments ';'
                        { $$ = $2; }
        ;

specparam_declarationFront:     // IEEE: part of specparam_declaration
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                ySPECPARAM
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_ਗੰਢ_DType* dtp = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE};
                          VARDTYPE(dtp); }
        |       ySPECPARAM packed_dimension
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_ਗੰਢ_DType* ਅਚਲ dtp = GRAMMARP->ਜੋੜੋ_Range(
                                    new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT}, $2, true);
                          VARDTYPE(dtp); }
        ;

list_ਦਾ_specparam_assignments<varp>:  // ==IEEE: list_of_specparam_assignments
                specparam_assignment                    { $$ = $1; }
        |       list_ਦਾ_specparam_assignments ',' specparam_assignment  { $$ = $1->ਜੋੜੋ_ਅਗਲਾ($3); }
        ;

specparam_assignment<varp>:     // ==IEEE: specparam_assignment
                idNotPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          ਜੇ ($4) $$->valuep($4); }
        //                      // IEEE: pulse_control_specparam
        //                      // LRM grammar requires '(' as the first token after assignment,
        //                      // but IEEE provides an example in 30.7.1 where it is omitted.
        //                      // Other simulators also support it.
        |       idPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          ਜੇ ($4) $$->valuep($4); }
        |       idPathpulse sigAttrListE '=' '(' minTypMax ',' minTypMax ')'
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          ਜੇ ($5) $$->valuep($5);
                          DEL($7); }
        ;

system_timing_check<nodep>:         // ==IEEE: system_timing_check
                setuphold_timing_check                      { $$ = $1; }
        ;

setuphold_timing_check<nodep>:      // ==IEEE: $setuphold_timing_check
                yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_ਹੱਦਬੰਦੀ ',' timing_check_ਹੱਦਬੰਦੀ ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_ਹੱਦਬੰਦੀ ',' timing_check_ਹੱਦਬੰਦੀ ',' idAnyE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_ਹੱਦਬੰਦੀ ',' timing_check_ਹੱਦਬੰਦੀ ',' idAnyE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_ਹੱਦਬੰਦੀ ',' timing_check_ਹੱਦਬੰਦੀ ',' idAnyE ',' minTypMaxE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13, $15); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_ਹੱਦਬੰਦੀ ',' timing_check_ਹੱਦਬੰਦੀ ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17}; DEL($7, $9, $13, $15);  }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_ਹੱਦਬੰਦੀ ',' timing_check_ਹੱਦਬੰਦੀ ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17, $19}; DEL($7, $9, $13, $15); }
        ;

timing_check_event<ਗੰਢ_Exprp>:      // ==IEEE: $timing_check_event
                terminal_identifier                                                         { $$ = $1; }
        |       yPOSEDGE terminal_identifier                                                { $$ = $2; }
        |       yNEGEDGE terminal_identifier                                                { $$ = $2; }
        |       y_ਕਿਨਾਰਾ terminal_identifier                                                   { $$ = $2; }
        |       y_ਕਿਨਾਰਾ '[' ਕਿਨਾਰਾ_descriptor_list ']' terminal_identifier                      { $$ = $5; }
        |       terminal_identifier yP_ANDANDAND ਵਿਅੰਜਕ                                       { $$ = $1; DEL($3); }
        |       yPOSEDGE terminal_identifier yP_ANDANDAND ਵਿਅੰਜਕ                              { $$ = $2; DEL($4); }
        |       yNEGEDGE terminal_identifier yP_ANDANDAND ਵਿਅੰਜਕ                              { $$ = $2; DEL($4); }
        |       y_ਕਿਨਾਰਾ terminal_identifier yP_ANDANDAND ਵਿਅੰਜਕ                                 { $$ = $2; DEL($4); }
        |       y_ਕਿਨਾਰਾ '[' ਕਿਨਾਰਾ_descriptor_list ']' terminal_identifier yP_ANDANDAND ਵਿਅੰਜਕ    { $$ = $5; DEL($7); }
        ;

ਕਿਨਾਰਾ_descriptor_list:
                yaEDGEDESC                            {  }
        |       ਕਿਨਾਰਾ_descriptor_list ',' yaEDGEDESC   {  }
        ;

timing_check_ਹੱਦਬੰਦੀ<ਗੰਢ_Exprp>:
                ਵਿਅੰਜਕ                      { $$ = $1; }
        |       ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ ':' ਵਿਅੰਜਕ    { $$ = $3; DEL($1, $5); }
        ;

delayed_referenceE<ਗੰਢ_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       terminal_identifier                     { $$ = $1; }
        ;

terminal_identifier<ਗੰਢ_Exprp>:
                idArrayed     { $$ = $1; }
        ;

idAnyE<strp>:
                /*empty*/                               { $$ = nullptr; }
        |       idAny                                   { $$ = $1; }
        ;

junk_ਮੰਜ਼ਿਲ_Semi_List<fl>:
                junk_ਮੰਜ਼ਿਲ_Semi                              { $$ = CRELINE(); }
        |       junk_ਮੰਜ਼ਿਲ_Semi_List junk_ਮੰਜ਼ਿਲ_Semi               { $$ = CRELINE(); }
        ;

junk_ਮੰਜ਼ਿਲ_Semi:
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
                yRANDOMIZE                              { ਸਥਿਰ string s = "randomize"; $$ = &s; $<fl>$ = $<fl>1; }
        ;

idSVKwd<strp>:                  // Warn about non-forward compatible Verilog 2001 code
        //                      // yBIT, yBYTE won't work here as causes conflicts
                yDO
                        { ਸਥਿਰ string s = "do"   ; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        |       yFINAL
                        { ਸਥਿਰ string s = "final"; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        ;

variable_lvalue<ਗੰਢ_Exprp>:     // IEEE: variable_lvalue or net_lvalue
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

variable_lvalueConcList<ਗੰਢ_Exprp>:  // IEEE: part of variable_lvalue: '{' variable_lvalue { ',' variable_lvalue } '}'
                variable_lvalue                                 { $$ = $1; }
        |       variable_lvalueConcList ',' variable_lvalue     { $$ = new AstConcat{$2, $1, $3}; }
        ;

//UNSUPvariable_lvalueList<nodeExprp>:  // IEEE: part of variable_lvalue: variable_lvalue { ',' variable_lvalue }
//UNSUP         variable_lvalue                         { $$ = $1; }
//UNSUP |       variable_lvalueList ',' variable_lvalue { $$ = addNextNull($1, $3); }
//UNSUP ;

idClass<ਗੰਢ_Exprp>:             // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
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

idClassSel<ਗੰਢ_Exprp>:          // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
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
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "this"}, $3->fromp()->unlink_Fr_ਪਿੱਠ()}); $$ = $3; }
        |       ySUPER '.' idDottedForeach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "super"}, $3->fromp()->unlink_Fr_ਪਿੱਠ()}); $$ = $3; }
        |       yTHIS '.' ySUPER '.' idDottedForeach
                        { $5->fromp(new AstDot{$4, false, new AstParseRef{$<fl>3, "super"}, $5->fromp()->unlink_Fr_ਪਿੱਠ()}); $$ = $5; }
        //                      // Expanded: package_scope idForeach
        |       packageClassScope idDottedForeach
                       { $2->fromp(new AstDot{$<fl>2, true, $1, $2->fromp()->unlink_Fr_ਪਿੱਠ()}); $$ = $2; }
        ;


// Dotted identifier for typedef - must have at least one '.'
// First component is plain id or id with array index, subsequent components can have arrays
idDottedOrArrayed<ਗੰਢ_Exprp>:
                id '.' idArrayed
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, *$1, nullptr, nullptr}, $3}; }
        |       idDottedOrArrayed '.' idArrayed
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

idDotted<ਗੰਢ_Exprp>:
                y_D_ਜੜ੍ਹ '.' id_Dotted_ਹੋਰ
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_ਹੋਰ                            { $$ = $1; }
        ;

idDottedSel<ਗੰਢ_Exprp>:
                y_D_ਜੜ੍ਹ '.' id_Dotted_Sel_ਹੋਰ
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_Sel_ਹੋਰ                         { $$ = $1; }
        ;

idDottedForeach<foreachHeaderp>:
                y_D_ਜੜ੍ਹ '.' id_Dotted_ਹੋਰ_Foreach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3->fromp()->unlink_Fr_ਪਿੱਠ()}); $$ = $3; }
        |       id_Dotted_ਹੋਰ_Foreach                     { $$ = $1; }
        ;

id_Dotted_ਹੋਰ<ਗੰਢ_Exprp>:
                parse_Ref_ਅਧਾਰ                            { $$ = $1; }
        |       id_Dotted_ਹੋਰ '.' parse_Ref_ਅਧਾਰ           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_Sel_ਹੋਰ<ਗੰਢ_Exprp>:
                idArrayed                               { $$ = $1; }
        |       id_Dotted_Sel_ਹੋਰ '.' idArrayed           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_ਹੋਰ_Foreach<foreachHeaderp>:
                idArrayedForeach                        { $$ = $1; }
        |       id_Dotted_Sel_ਹੋਰ '.' idArrayedForeach
                        { $3->fromp(new AstDot{$2, false, $1, $3->fromp()->unlink_Fr_ਪਿੱਠ()}); $$ = $3; }
        ;

// Single component of dotted path, maybe [#].
// Due to lookahead constraints, we can't know if [:] or [+:] are valid (last dotted part),
// we'll assume so and cleanup later.
// id below includes:
//       enum_identifier
idArrayed<ਗੰਢ_Exprp>:               // IEEE: id + select
                id
                        { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        //                      // IEEE: id + part_select_range/constant_part_select_range
        |       idArrayed '[' ਵਿਅੰਜਕ ']'                          { $$ = new Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ{$2, $1, $3}; }  // Or AstArraySel, don't know yet.
        |       idArrayed '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'       { $$ = new Ast_Sel_ਕੱਢੋ{$2, $1, $3, $5}; }
        //                      // IEEE: id + indexed_range/constant_indexed_range
        |       idArrayed '[' ਵਿਅੰਜਕ yP_PLUSCOLON  const_ਵਿਅੰਜਕ ']'  { $$ = new AstSelPlus{$2, $1, $3, $5}; }
        |       idArrayed '[' ਵਿਅੰਜਕ yP_MINUSCOLON const_ਵਿਅੰਜਕ ']'  { $$ = new AstSelMinus{$2, $1, $3, $5}; }
        ;

idArrayedForeach<foreachHeaderp>:    // IEEE: id + select (under foreach expression)
                parse_Ref_ਅਧਾਰ  // Malformed, but accept for better error reporting
                        { $$ = new Ast_Foreach_ਸਿਰਲੇਖ{$<fl>1, $1, nullptr}; }
        |       idArrayed '[' ਵਿਅੰਜਕ ']'
                        { $$ = new Ast_Foreach_ਸਿਰਲੇਖ{$2, $1, $3}; }
        |       idArrayed '[' ']'
                        { $$ = new Ast_Foreach_ਸਿਰਲੇਖ{$2, $1, new Ast_ਖਾਲੀ_ਥਾਵਾਂ{$3}}; }
        |       idArrayed '[' parse_Ref_ਅਧਾਰ ',' loop_variables ']'
                        { $$ = new Ast_Foreach_ਸਿਰਲੇਖ{$2, $1, ਜੋੜੋ_ਅਗਲਾ_Null(static_cast<Ast_ਗੰਢ*>($3), $5)}; }
        |       idArrayed '[' ',' loop_variables ']'
                        { $$ = new Ast_Foreach_ਸਿਰਲੇਖ{$2, $1, ਜੋੜੋ_ਅਗਲਾ_Null(static_cast<Ast_ਗੰਢ*>(new Ast_ਖਾਲੀ_ਥਾਵਾਂ{$3}), $4)}; }
        ;

// ParseRef without any dots or vectorizaion
parse_Ref_ਅਧਾਰ<parseRefp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

// yaSTRING shouldn't be used directly, instead via an abstraction below
str<strp>:                      // yaSTRING but with \{escapes} need decoded
                yaSTRING                                { $$ = PARSEP->newString(GRAMMARP->unquoteString($<fl>1, *$1)); }
        ;

strAsInt<ਗੰਢ_Exprp>:
                yaSTRING
                        { // Numeric context, so IEEE 1800-2017 11.10.3 "" is a "\000"
                          $$ = new AstConst{$<fl>1, AstConst::VerilogStringLiteral{}, GRAMMARP->unquoteString($<fl>1, *$1)}; }
        ;

strAsIntIgnore<ਗੰਢ_Exprp>:          // strAsInt, but never matches for when expr shouldn't parse strings
                yaSTRING__IGNORE                        { $$ = nullptr; yyerror("Impossible token"); }
        ;

ਸ਼ੁਰੂ_Label_E<strp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        ;

ਸਿਰਾ_Label_E<strp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        |       ':' yNEW__ETC                           { ਸਥਿਰ string n = "new"; $$ = &n; $<fl>$ = $<fl>2; }
        ;

//************************************************
// Clocking

clocking_declaration<nodep>:            // IEEE: clocking_declaration
                yCLOCKING idAny clocking_event ';' clocking_ਵਸਤੂ_List_E yENDCLOCKING ਸਿਰਾ_Label_E
                        { $$ = new AstClocking{$<fl>2, *$2, new Ast_Sen_ਰੁੱਖ{$<fl>2, $3}, $5, false, false}; }
        |       yDEFAULT yCLOCKING clocking_event ';' clocking_ਵਸਤੂ_List_E yENDCLOCKING ਸਿਰਾ_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_ਰੁੱਖ{$<fl>2, $3}, $5, true, false}; }
        |       yDEFAULT yCLOCKING idAny clocking_event ';' clocking_ਵਸਤੂ_List_E yENDCLOCKING ਸਿਰਾ_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_ਰੁੱਖ{$<fl>3, $4}, $6, true, false}; }
        |       yGLOBAL__CLOCKING yCLOCKING clocking_event ';' clocking_ਵਸਤੂ_List_E yENDCLOCKING ਸਿਰਾ_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_ਰੁੱਖ{$<fl>2, $3}, $5, false, true}; }
        |       yGLOBAL__CLOCKING yCLOCKING idAny clocking_event ';' clocking_ਵਸਤੂ_List_E yENDCLOCKING ਸਿਰਾ_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_ਰੁੱਖ{$<fl>3, $4}, $6, false, true}; }
        ;

clocking_eventE<senItemp>:      // IEEE: optional clocking_event
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       clocking_event                          { $$ = $1; }
        ;

clocking_event<senItemp>:       // IEEE: clocking_event
        //                      // IEEE: '@' ps_identifier
        //                      // IEEE: '@' hierarchical_identifier
        //UNSUP: '@' idClassSel/*ps_identifier*/
                '@' id
                        { $$ = new Ast_Sen_ਵਸਤੂ{$<fl>2, VEdgeType::ET_ਬਦਲਿਆ,
                                              new AstParseRef{$<fl>2, *$2, nullptr, nullptr}}; }
        |       '@' '(' event_ਵਿਅੰਜਕ ')'            { $$ = $3; }
        ;

clocking_ਵਸਤੂ_List_E<nodep>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       clocking_ਵਸਤੂ_List                       { $$ = $1; }
        ;

clocking_ਵਸਤੂ_List<nodep>:  // IEEE: [ clocking_item ]
                clocking_ਵਸਤੂ                           { $$ = $1; }
        |       clocking_ਵਸਤੂ_List clocking_ਵਸਤੂ         { ਜੇ ($1) $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

clocking_ਵਸਤੂ<nodep>:   // IEEE: clocking_item
                yDEFAULT y_ਨਿਵੇਸ਼ clocking_skew ';'       { $$ = new Ast_Clocking_ਵਸਤੂ{$<fl>1, VDirection::ਨਿਵੇਸ਼, $3, nullptr}; }
        |       yDEFAULT yOUTPUT clocking_skew ';'      { $$ = new Ast_Clocking_ਵਸਤੂ{$<fl>1, VDirection::OUTPUT, $3, nullptr}; }
        |       yDEFAULT y_ਨਿਵੇਸ਼ clocking_skew yOUTPUT clocking_skew ';'
                        { $$ = new Ast_Clocking_ਵਸਤੂ{$<fl>1, VDirection::ਨਿਵੇਸ਼, $3, nullptr};
                          $$->ਜੋੜੋ_ਅਗਲਾ(new Ast_Clocking_ਵਸਤੂ{$<fl>4, VDirection::OUTPUT, $5, nullptr}); }
        |       y_ਨਿਵੇਸ਼ clocking_skewE list_ਦਾ_clocking_decl_ਸੌਂਪੋ ';'
                        { $$ = GRAMMARP->make_Clocking_ਵਸਤੂ_List($<fl>1, VDirection::ਨਿਵੇਸ਼, $2, $3); }
        |       yOUTPUT clocking_skewE list_ਦਾ_clocking_decl_ਸੌਂਪੋ ';'
                        { $$ = GRAMMARP->make_Clocking_ਵਸਤੂ_List($<fl>1, VDirection::OUTPUT, $2, $3); }
        |       y_ਨਿਵੇਸ਼ clocking_skewE yOUTPUT clocking_skewE list_ਦਾ_clocking_decl_ਸੌਂਪੋ ';'
                        { $$ = GRAMMARP->make_Clocking_ਵਸਤੂ_List($<fl>1, VDirection::ਨਿਵੇਸ਼, $2, $5->clone_ਰੁੱਖ(true));
                          $$->ਜੋੜੋ_ਅਗਲਾ(GRAMMARP->make_Clocking_ਵਸਤੂ_List($<fl>3, VDirection::OUTPUT, $4, $5)); }
        |       yINOUT list_ਦਾ_clocking_decl_ਸੌਂਪੋ ';'
                        { $$ = GRAMMARP->make_Clocking_ਵਸਤੂ_List($<fl>1, VDirection::ਨਿਵੇਸ਼, nullptr, $2->clone_ਰੁੱਖ(true));
                          $$->ਜੋੜੋ_ਅਗਲਾ(GRAMMARP->make_Clocking_ਵਸਤੂ_List($<fl>1, VDirection::OUTPUT, nullptr, $2)); }
        |       assertion_ਵਸਤੂ_declaration
                        { $$ = $1;
                          ਲਈ (Ast_ਗੰਢ* nodep = $1; nodep; nodep = nodep->nextp()) {
                              ਜੇ (!VN_IS(nodep, Sequence)) {
                                  $$ = nullptr;
                                  BBUNSUP(nodep, "Unsupported: assertion items in clocking blocks");
                                  DEL($1);
                                  ਰੋਕੋ;
                              }
                          }
                        }
        ;

list_ਦਾ_clocking_decl_ਸੌਂਪੋ<nodep>:  // IEEE: list_of_clocking_decl_assign
                clocking_decl_ਸੌਂਪੋ                    { $$ = $1; }
        |       list_ਦਾ_clocking_decl_ਸੌਂਪੋ ',' clocking_decl_ਸੌਂਪੋ
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

clocking_decl_ਸੌਂਪੋ<nodep>:    // IEEE: clocking_decl_assign
                idAnyAsParseRef/*new-signal_identifier*/ ਵਿਅੰਜਕ_Eq_E
                        { AstParseRef* ਅਚਲ refp = $1;
                          $$ = refp;
                          ਜੇ ($2) $$ = new Ast_ਸੌਂਪੋ{$<fl>2, refp, $2}; }
        ;

clocking_skewE<ਗੰਢ_Exprp>:          // IEEE: [clocking_skew]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       clocking_skew                           { $$ = $1; }
        ;

clocking_skew<ਗੰਢ_Exprp>:           // IEEE: clocking_skew
                delay_control                           { $$ = $1->lhsp()->unlink_Fr_ਪਿੱਠ(); $1->delete_ਰੁੱਖ(); }
        |      yPOSEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      yNEGEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      y_ਕਿਨਾਰਾ delay_controlE                     { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        ;

ਚੱਕਰ_delay<delayp>:  // IEEE: cycle_delay
               yP_POUNDPOUND yaINTNUM
                        { $$ = new AstDelay{$<fl>1, new AstConst{$<fl>2, *$2}, true}; }
        |      yP_POUNDPOUND idAnyAsParseRef
                        { $$ = new AstDelay{$<fl>1, $2, true}; }
        |      yP_POUNDPOUND '(' ਵਿਅੰਜਕ ')'
                        { $$ = new AstDelay{$<fl>1, $3, true}; }
        ;

//************************************************
// Asserts

assertion_ਵਸਤੂ_declaration<nodep>:  // ==IEEE: assertion_item_declaration
                property_declaration                    { $$ = $1; }
        |       sequence_declaration                    { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

assertion_ਵਸਤੂ<nodep>:          // ==IEEE: assertion_item
                concurrent_assertion_ਵਸਤੂ               { $$ = $1; }
        |       deferred_ਤੁਰੰਤ_assertion_ਵਸਤੂ
                        { $$ = $1 ? new AstAlways{$1->fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr, $1} : nullptr; }
        ;

deferred_ਤੁਰੰਤ_assertion_ਵਸਤੂ<nodep>:       // ==IEEE: deferred_immediate_assertion_item
                deferred_ਤੁਰੰਤ_assertion_statement  { $$ = $1; }
        |       id/*block_identifier*/ ':' deferred_ਤੁਰੰਤ_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        ;

procedural_assertion_statement<ਗੰਢ_Stmtp>:  // ==IEEE: procedural_assertion_statement
                concurrent_assertion_statement          { $$ = $1; }
        |       ਤੁਰੰਤ_assertion_statement           { $$ = $1; }
        //                      // IEEE: checker_instantiation
        //                      // Unlike modules, checkers are the only "id id (...)" form in statements.
        //UNSUP checker_instantiation                   { $$ = $1; }
        ;

ਤੁਰੰਤ_assertion_statement<ਗੰਢ_Stmtp>:   // ==IEEE: immediate_assertion_statement
                simple_ਤੁਰੰਤ_assertion_statement    { $$ = $1; }
        |       deferred_ਤੁਰੰਤ_assertion_statement  { $$ = $1; }
        ;

simple_ਤੁਰੰਤ_assertion_statement<ਗੰਢ_Stmtp>:    // ==IEEE: simple_immediate_assertion_statement
        //                      // action_block expanded here, for compatibility with AstAssert
                assertOrAssume '(' ਵਿਅੰਜਕ ')' stmt %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $3, $5, nullptr, VAssertType::SIMPLE_ਤੁਰੰਤ, $1}; }
        |       assertOrAssume '(' ਵਿਅੰਜਕ ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, nullptr, $6, VAssertType::SIMPLE_ਤੁਰੰਤ, $1}; }
        |       assertOrAssume '(' ਵਿਅੰਜਕ ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, $5, $7, VAssertType::SIMPLE_ਤੁਰੰਤ, $1}; }
        //                      // IEEE: simple_immediate_cover_statement
        |       yCOVER '(' ਵਿਅੰਜਕ ')' stmt                { $$ = new AstCover{$1, $3, $5, VAssertType::SIMPLE_ਤੁਰੰਤ}; }
        ;

assertOrAssume<assertdirectivetypeen>:
                yASSERT                                 { $$ = VAssertDirectiveType::ASSERT; }
        |       yASSUME                                 { $$ = VAssertDirectiveType::ASSUME; }
        ;

final_zero<asserttypeen>:                     // IEEE: part of deferred_immediate_assertion_statement
                '#' yaINTNUM
                        { ਜੇ ($2->isNeqZero()) { $<fl>2->v3error("Deferred assertions must use '#0' (IEEE 1800-2023 16.4)"); }
                          $$ = VAssertType::OBSERVED_DEFERRED_ਤੁਰੰਤ; }
        //                      // 1800-2012:
        |       yFINAL                                                  { $$ = VAssertType::FINAL_DEFERRED_ਤੁਰੰਤ; }
        ;

deferred_ਤੁਰੰਤ_assertion_statement<ਗੰਢ_Stmtp>:  // ==IEEE: deferred_immediate_assertion_statement
        //                      // IEEE: deferred_immediate_assert_statement
                assertOrAssume final_zero '(' ਵਿਅੰਜਕ ')' stmt %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $4, $6, nullptr, $2, $1}; }
        |       assertOrAssume final_zero '(' ਵਿਅੰਜਕ ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, nullptr, $7, $2, $1}; }
        |       assertOrAssume final_zero '(' ਵਿਅੰਜਕ ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, $6, $8, $2, $1}; }
        //                      // IEEE: deferred_immediate_cover_statement
        |       yCOVER final_zero '(' ਵਿਅੰਜਕ ')' stmt     { $$ = new AstCover{$1, $4, $6, $2}; }
        ;

concurrent_assertion_ਵਸਤੂ<nodep>:       // IEEE: concurrent_assertion_item
                concurrent_assertion_statement          { $$ = $1; }
        |       id/*block_identifier*/ ':' concurrent_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        //                      // IEEE: checker_instantiation
        //                      // identical to module_instantiation; see etcInst
        ;

concurrent_assertion_statement<ਗੰਢ_Stmtp>:  // ==IEEE: concurrent_assertion_statement
        //                      // IEEE: assert_property_statement
        //                      // IEEE: assume_property_statement
        //                      // action_block expanded here
                assertOrAssume yPROPERTY '(' property_spec ')' stmt %prec pr_ਹੇਠਲਾ_THAN_ELSE
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
                        { AstCover* ਅਚਲ coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), nullptr, nullptr, $4},
                              $6, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event sexpr ')' stmt
                        { AstCover* ਅਚਲ coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, nullptr, $5},
                              $7, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event yDISABLE yIFF '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* ਅਚਲ coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, $8, $10},
                              $12, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' yDISABLE yIFF '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* ਅਚਲ coverp = new AstCover{$1,
                              new AstPropSpec{$7->fileline(), nullptr, $7, $9},
                              $11, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        //                      // IEEE: restrict_property_statement
        |       yRESTRICT yPROPERTY '(' property_spec ')' ';'
                        { $$ = new AstRestrict{$1, $4}; }
        ;

property_declaration<ਗੰਢ_FTaskp>:  // ==IEEE: property_declaration
                property_declarationFront property_ਪੋਰਟ_list_E ';' property_declarationBody
                        yENDPROPERTY ਸਿਰਾ_Label_E
                        { $$ = $1;
                          $$->ਜੋੜੋ_Stmtsp($2);
                          $$->ਜੋੜੋ_Stmtsp($4);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>6, $$, $6);
                          GRAMMARP->m_insideProperty = false;
                          GRAMMARP->m_typed_Property_ਪੋਰਟ = false; }
        ;

property_declarationFront<ਗੰਢ_FTaskp>:  // IEEE: part of property_declaration
                yPROPERTY idAny/*property_identifier*/
                        { $$ = new AstProperty{$<fl>2, *$2, nullptr};
                          GRAMMARP->m_insideProperty = true; }
        ;

property_ਪੋਰਟ_list_E<nodep>:  // IEEE: [ ( [ property_port_list ] ) ]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '(' property_ਪੋਰਟ_list ')'              { $$ = $2; }
        ;

property_ਪੋਰਟ_list<nodep>:  // ==IEEE: property_port_list
                property_ਪੋਰਟ_ਵਸਤੂ                              { $$ = $1; }
        |       property_ਪੋਰਟ_list ',' property_ਪੋਰਟ_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

property_ਪੋਰਟ_ਵਸਤੂ<nodep>:  // IEEE: property_port_item/sequence_port_item
        //                      // Merged in sequence_port_item
        //                      // IEEE: property_lvar_port_direction ::= yINPUT
        //                      // prop IEEE: [ yLOCAL [ yINPUT ] ] property_formal_type
        //                      //           id {variable_dimension} [ '=' property_actual_arg ]
        //                      // seq IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
                property_ਪੋਰਟ_ਵਸਤੂ_Front property_ਪੋਰਟ_ਵਸਤੂ_Assignment  { $$ = $2; }
        ;

property_ਪੋਰਟ_ਵਸਤੂ_Front:  // IEEE: part of property_port_item/sequence_port_item
                property_ਪੋਰਟ_ਵਸਤੂ_Dir_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       property_ਪੋਰਟ_ਵਸਤੂ_Dir_E ਅੰਕੜੇ_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_ਪੋਰਟ = true; }
        |       property_ਪੋਰਟ_ਵਸਤੂ_Dir_E yVAR ਅੰਕੜੇ_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_ਪੋਰਟ = true; }
        |       property_ਪੋਰਟ_ਵਸਤੂ_Dir_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       property_ਪੋਰਟ_ਵਸਤੂ_Dir_E implicit_typeE
                        { VARDTYPE($2); }
        ;

property_ਪੋਰਟ_ਵਸਤੂ_Assignment<nodep>:  // IEEE: part of property_port_item/sequence_port_item
                id variable_dimensionListE
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, nullptr); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, $4);
                          BBUNSUP($3, "Unsupported: property variable default value"); }
        ;

property_ਪੋਰਟ_ਵਸਤੂ_Dir_E:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { VARIO(ਨਿਵੇਸ਼); }
        |       yLOCAL__ETC
                        { VARIO(ਨਿਵੇਸ਼);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        |       yLOCAL__ETC y_ਨਿਵੇਸ਼
                        { VARIO(ਨਿਵੇਸ਼);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        ;

property_declarationBody<nodep>:  // IEEE: part of property_declaration
                assertion_variable_declarationList property_spec
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        |       assertion_variable_declarationList property_spec ';'
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        //                      // IEEE-2012: Incorrectly has yCOVER ySEQUENCE then property_spec here.
        //                      // Fixed in IEEE 1800-2017
        |       property_spec                           { $$ = $1; }
        |       property_spec ';'                       { $$ = $1; }
        ;

assertion_variable_declarationList<nodep>:  // IEEE: part of assertion_variable_declaration
                assertion_variable_declaration          { $$ = $1; }
        |       assertion_variable_declarationList assertion_variable_declaration
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

sequence_declaration<ਗੰਢ_FTaskp>:  // ==IEEE: sequence_declaration
                sequence_declarationFront sequence_ਪੋਰਟ_list_E ';' sequence_declarationBody
        /*cont*/    yENDSEQUENCE ਸਿਰਾ_Label_E
                        { $$ = $1;
                          $$->ਜੋੜੋ_Stmtsp($2);
                          $$->ਜੋੜੋ_Stmtsp($4);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>6, $$, $6);
                          // Sequence declarations are allowed; they are inlined by V3AssertPre.
                          // Sequences in unsupported contexts are reported by assertPreAll
                          // after inlining.
                        }
        ;

sequence_declarationFront<ਗੰਢ_FTaskp>:  // IEEE: part of sequence_declaration
                ySEQUENCE idAny/*new_sequence*/
                        { $$ = new AstSequence{$<fl>2, *$2, nullptr}; }
        ;

sequence_ਪੋਰਟ_list_E<nodep>:  // IEEE: [ ( [ sequence_port_list ] ) ]
        //                      // IEEE: sequence_lvar_port_direction ::= yINPUT | yINOUT | yOUTPUT
        //                      // IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
        //                      // All this is almost identically the same as a property.
        //                      // Difference is only yINOUT/yOUTPUT (which might be added to 1800-2012)
        //                      // and yPROPERTY.  So save some work.
                property_ਪੋਰਟ_list_E                     { $$ = $1; }
        ;

property_formal_typeNoDt<ਗੰਢ_DTypep>:  // IEEE: property_formal_type (w/o implicit)
                sequence_formal_typeNoDt                { $$ = $1; }
        |       yPROPERTY
                        { $$ = nullptr; GRAMMARP->m_typed_Property_ਪੋਰਟ = false;
                          BBUNSUP($1, "Unsupported: property argument data type"); }
        ;

sequence_formal_typeNoDt<ਗੰਢ_DTypep>:  // ==IEEE: sequence_formal_type (w/o data_type_or_implicit)
                        // IEEE: data_type_or_implicit
                        // implicit expanded where used
                ySEQUENCE
                        { $$ = nullptr; GRAMMARP->m_typed_Property_ਪੋਰਟ = false;
                          BBUNSUP($1, "Unsupported: sequence argument data type"); }
                        // IEEE-2009: yEVENT
                        // already part of data_type.  Removed in 1800-2012.
        |       yUNTYPED
                        { $$ = nullptr; GRAMMARP->m_typed_Property_ਪੋਰਟ = false; }
        ;

sequence_declarationBody<nodep>:  // IEEE: part of sequence_declaration
        //                      // 1800-2012 makes ';' optional
                assertion_variable_declarationList sexpr        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        |       assertion_variable_declarationList sexpr ';'    { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        |       sexpr                                   { $$ = $1; }
        |       sexpr ';'                               { $$ = $1; }
        //                      // IEEE: clocking_event sequence_expr (16.7)
        //                      // A leading clocking event on a named sequence body.
        |       '@' '(' event_ਵਿਅੰਜਕ ')' sexpr      { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' '(' event_ਵਿਅੰਜਕ ')' sexpr ';'  { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' senitemVar sexpr                    { $$ = new AstSClocked{$1, $2, $3}; }
        |       '@' senitemVar sexpr ';'                { $$ = new AstSClocked{$1, $2, $3}; }
        ;

property_spec<propSpecp>:               // IEEE: property_spec
        //UNSUP: This rule has been super-specialized to what is supported now
        //UNSUP remove below
                '@' '(' senitem ')' yDISABLE yIFF '(' ਵਿਅੰਜਕ ')' property_ਵਿਅੰਜਕ_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($3); $$->disablep($8); }
        |       '@' '(' senitem ')' property_ਵਿਅੰਜਕ_Spec
                        { $$ = $5; $$->fileline($1); $$->sensesp($3); }
        |       '@' senitemVar property_ਵਿਅੰਜਕ_Spec
                        { $$ = $3; $$->fileline($1); $$->sensesp($2); }
        |       yDISABLE yIFF '(' ਵਿਅੰਜਕ ')' '@' '(' senitem ')' property_ਵਿਅੰਜਕ_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($8); $$->disablep($4); }
        //UNSUP remove above
        |       yDISABLE yIFF '(' ਵਿਅੰਜਕ ')' property_ਵਿਅੰਜਕ_Spec
                        { $$ = $6; $$->fileline($4->fileline()); $$->disablep($4); }
        |       property_ਵਿਅੰਜਕ_Spec                       { $$ = $1; }
        ;

property_ਵਿਅੰਜਕ_Spec<propSpecp>:  // A property expression plus explicit weak/strong strength
                pexpr
                        { $$ = new AstPropSpec{$1->fileline(), nullptr, nullptr, $1}; }
        |       ySTRONG '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::STRONG}; }
        |       yWEAK '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::WEAK}; }
        ;

property_ਵਿਅੰਜਕ_Case_If<ਗੰਢ_Exprp>:  // IEEE: part of property_expr for if/case
                yCASE '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' property_case_ਵਸਤੂ_List yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, $5); }
        |       yCASE '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, nullptr); }
        |       yIF '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' pexpr  %prec prIF
                        { $$ = new AstImplication{$1, $3, $5, true}; }
        |       yIF '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' pexpr yELSE pexpr
                        { Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elseCondp = new AstLogNot{$1, $3->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(false)};
                          $$ = new AstSAnd{$1, new AstImplication{$1, $3, $5, true},
                                           new AstImplication{$1, elseCondp, $7, true},
                                           /*propertyControl=*/true}; }
        ;

property_case_ਵਸਤੂ_List<caseItemp>:  // IEEE: {property_case_item}
                property_case_ਵਸਤੂ                      { $$ = $1; }
        |       property_case_ਵਸਤੂ_List property_case_ਵਸਤੂ   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

property_case_ਵਸਤੂ<caseItemp>:  // ==IEEE: property_case_item
        //                      // IEEE: expression_or_dist { ',' expression_or_dist } ':' property_expr
        //                      // IEEE 1800-2012 changed from property_statement to property_expr
        //                      // IEEE 1800-2017 changed to require the semicolon
                caseCondList ':' pexpr ';'              { $$ = new Ast_Case_ਵਸਤੂ{$2, $1, $3}; }
        |       yDEFAULT pexpr ';'                      { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $2}; }
        |       yDEFAULT ':' pexpr ';'                  { $$ = new Ast_Case_ਵਸਤੂ{$1, nullptr, $3}; }
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

pexpr<ਗੰਢ_Exprp>:  // IEEE: property_expr  (The name pexpr is important as regexps just add an "p" to expr.)
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
        |       property_ਵਿਅੰਜਕ_Case_If                     { $$ = $1; }
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
        |       yNEXTTIME '[' const_ਵਿਅੰਜਕ ']' pexpr %prec yNEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: nexttime[] (in property expression)"); DEL($3); }
        |       yS_NEXTTIME '[' const_ਵਿਅੰਜਕ ']' pexpr %prec yS_NEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: s_nexttime[] (in property expression)"); DEL($3); }
        |       yALWAYS pexpr  %prec prALWAYS
                        { $$ = $2; }
        |       yALWAYS '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']' pexpr  %prec prALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, false}; }
        |       yS_ALWAYS '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']' pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, true}; }
        |       yS_ALWAYS pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $2, new AstUnbounded{$1}, new AstUnbounded{$1}, true}; }
        |       yS_EVENTUALLY pexpr  %prec prS_EVENTUALLY
                        {
                            $$ = new AstSEventually{$1, $2};
                            PARSEP->import_If_ਵਿੱਚ_Std($1, "process", true);
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
        |       ~o~pexpr y_ਸਿੱਟਾ_ਦਿੰਦਾ pexpr
                        { $$ = new AstLogOr{$2, new AstLogNot{$2, $1}, $3}; }
        //                      // yIFF also used by event_expression
        |       ~o~pexpr yIFF pexpr
                        { $$ = new AstLogEq{$2, $1, $3}; }
        |       yACCEPT_ON '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' pexpr  %prec prACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_ਕਿਸਮ::ACCEPT_ON, $3, $5}; }
        |       yREJECT_ON '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' pexpr  %prec prREJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_ਕਿਸਮ::REJECT_ON, $3, $5}; }
        |       ySYNC_ACCEPT_ON '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' pexpr %prec prSYNC_ACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_ਕਿਸਮ::SYNC_ACCEPT_ON, $3, $5}; }
        |       ySYNC_REJECT_ON '(' ਵਿਅੰਜਕ/*expression_or_dist*/ ')' pexpr %prec prSYNC_REJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_ਕਿਸਮ::SYNC_REJECT_ON, $3, $5}; }
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
        |       BISONPRE_COPY_ONCE(ਵਿਅੰਜਕ,{s/~l~/p/g; s/~p~/p/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

sexpr<ਗੰਢ_Exprp>:  // ==IEEE: sequence_expr  (The name sexpr is important as regexps just add an "s" to expr.)
        //                      // ********* RULES COPIED IN sequence_exprProp
        //                      // For precedence, see IEEE 17.7.1
        //
        //                      // IEEE: "cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // IEEE: "sequence_expr cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // Both rules basically mean we can repeat sequences, so make it simpler:
                ਚੱਕਰ_delay_range ~p~sexpr  %prec yP_POUNDPOUND
                        { $$ = new AstSExpr{$<fl>1, $1, $2}; }
        |       ~p~sexpr ਚੱਕਰ_delay_range sexpr %prec prPOUNDPOUND_MULTI
                        { $$ = new AstSExpr{$<fl>2, $1, $2, $3}; }
        //
        //                      // IEEE: expression_or_dist [ boolean_abbrev ]
        //                      // Note expression_or_dist includes "expr"!
        //                      // sexpr/*sexpression_or_dist*/  --- Hardcoded below
        //                      // Consecutive repetition (IEEE 1800-2023 16.9.2)
        //                      // [*N] exact count
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSConsRep{$<fl>2, $1, $3}; }
        //                      // [*N:M] bounded range or [*N:$] unbounded range
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                        {
                            ਜੇ (VN_IS($5, Unbounded)) {
                                DEL($5);
                                $$ = new AstSConsRep{$<fl>2, $1, $3, nullptr, true};
                            } ਨਹੀਂਤਾਂ {
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
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3}; }
        //                      // IEEE: goto_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3, $5}; }
        //                      // IEEE: nonconsecutive_repetition (single count form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstSNonConsRep{$<fl>2, $1, $3}; }
        //                      // IEEE: nonconsecutive_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
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
        |       '(' ~p~sexpr ',' sequence_ਮੇਲ_ਵਸਤੂ_List ')'
                        { $$ = new Ast_ਵਿਅੰਜਕ_Stmt{$3, $4, $2}; }
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
        |       y_ਪਹਿਲਾ_ਮੇਲ '(' sexpr ')'
                        { $$ = $3; }  // IEEE 16.9.8: no-op in deterministic DFA model
        |       y_ਪਹਿਲਾ_ਮੇਲ '(' sexpr ',' sequence_ਮੇਲ_ਵਸਤੂ_List ')'
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
        |       BISONPRE_COPY_ONCE(ਵਿਅੰਜਕ,{s/~l~/s/g; s/~p~/s/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

ਚੱਕਰ_delay_range<delayp>:  // IEEE: ==cycle_delay_range
        //                      // These three terms in 1800-2005 ONLY
                yP_POUNDPOUND intnumAsConst
                        { $$ = new AstDelay{$1, $2, true}; }
        |       yP_POUNDPOUND idAny
                        { $$ = new AstDelay{$1, new AstParseRef{$<fl>2, *$2}, true}; }
        |       yP_POUNDPOUND '(' const_ਵਿਅੰਜਕ ')'
                        { $$ = new AstDelay{$1, $3, true}; }
        //                      // In 1800-2009 ONLY:
        //                      // IEEE: yP_POUNDPOUND constant_primary
        //                      // UNSUP: This causes a big grammar ambiguity
        //                      // as ()'s mismatch between primary and the following statement
        //                      // the sv-ac committee has been asked to clarify  (Mantis 1901)
        |       yP_POUNDPOUND '[' const_ਵਿਅੰਜਕ ':' const_ਵਿਅੰਜਕ ']'
                        { $$ = new AstDelay{$1, $3, true};
                                                                                                        $$->rhsp($5); }
        |       yP_POUNDPOUND yP_BRASTAR ']'
                        { $$ = new AstDelay{$1, new AstConst{$1, 0}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        |       yP_POUNDPOUND yP_BRAPLUSKET
                        { $$ = new AstDelay{$1, new AstConst{$1, 1}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        ;

sequence_ਮੇਲ_ਵਸਤੂ_List<nodep>:  // IEEE: [sequence_match_item] part of sequence_expr
                sequence_ਮੇਲ_ਵਸਤੂ                     { $$ = $1; }
        |       sequence_ਮੇਲ_ਵਸਤੂ_List ',' sequence_ਮੇਲ_ਵਸਤੂ { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

sequence_ਮੇਲ_ਵਸਤੂ<nodep>:  // ==IEEE: sequence_match_item
        //                      // IEEE says: operator_assignment
        //                      // IEEE says: inc_or_dec_expression
        //                      // IEEE says: subroutine_call
        //                      // This is the same list as...
                for_ਕਦਮ_assignment                     { $$ = $1; }
        ;

//      boolean_abbrev -- all forms now handled directly in sexpr rule:
//                      // IEEE: consecutive_repetition -- [*N], [*N:M], [+], [*] via AstSConsRep
//                      // IEEE: goto_repetition -- [->N], [->M:N] via AstSGotoRep
//                      // IEEE: nonconsecutive_repetition -- [=N], [=M:N] via AstSNonConsRep

//************************************************
// Covergroup

covergroup_declaration<nodep>:  // ==IEEE: covergroup_declaration
                 yCOVERGROUP idAny cg_ਪੋਰਟ_List_E coverage_eventE ';'
        /*cont*/    coverage_spec_or_optionListE
        /*cont*/ yENDGROUP ਸਿਰਾ_Label_E
                        { Ast_Sen_ਰੁੱਖ* clockp = nullptr;
                          Ast_ਗੰਢ* sampleArgsp = nullptr;
                          ਜੇ ($4) {
                              ਜੇ (VN_IS($4, Sen_ਵਸਤੂ)) {
                                  clockp = new Ast_Sen_ਰੁੱਖ{$<fl>1, VN_AS($4, Sen_ਵਸਤੂ)};
                              } ਨਹੀਂਤਾਂ {
                                  sampleArgsp = $4;
                              }
                          }
                          $$ = new AstCovergroup{$<fl>1, *$2, static_cast<AstVar*>($3),
                                                 static_cast<AstVar*>(sampleArgsp), $6, clockp};
                          // Every covergroup has option/type_option members (added by V3LinkParse)
                          // referencing std:: types, so mark std as needed at parse time.
                          v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Std_Package();
                          GRAMMARP->ਸਿਰਾ_Label($<fl>8, $$, $8); }
        |        yCOVERGROUP yEXTENDS idAny ';'
        /*cont*/     coverage_spec_or_optionListE
        /*cont*/ yENDGROUP ਸਿਰਾ_Label_E
                        { $$ = nullptr;
                          BBUNSUP($1, "Unsupported: covergroup inheritance (extends)");
                          DEL($5); }
        ;

cg_ਪੋਰਟ_List_E<nodep>:
                /*empty*/                               { $$ = nullptr; }
        |       '(' tf_ਪੋਰਟ_list_E ')'                   { $$ = $2; }
        ;

cgexpr<ਗੰਢ_Exprp>:  // IEEE-2012: covergroup_expression, before that just expression
                ਵਿਅੰਜਕ                                    { $$ = $1; }
        ;

coverage_spec_or_optionListE<nodep>:  // IEEE: [{coverage_spec_or_option}]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       coverage_spec_or_optionList             { $$ = $1; }
        ;

coverage_spec_or_optionList<nodep>:  // IEEE: {coverage_spec_or_option}
                coverage_spec_or_option                 { $$ = $1; }
        |       coverage_spec_or_optionList coverage_spec_or_option     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

coverage_spec_or_option<nodep>:  // ==IEEE: coverage_spec_or_option
        //                      // IEEE: coverage_spec
                cover_ਬਿੰਦੂ                             { $$ = $1; }
        |       cover_cross                             { $$ = $1; }
        |       coverage_option ';'                     { $$ = $1; }
        |       error                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

coverage_option<nodep>:  // ==IEEE: coverage_option
        //                      // option/type_option aren't really keywords
                id/*yOPTION | yTYPE_OPTION*/ '.' idAny/*member_identifier*/ '=' ਵਿਅੰਜਕ
                        { $$ = nullptr;
                          ਜੇ (*$1 == "option" || *$1 == "type_option") {
                              ਅਚਲ bool typeOpt = (*$1 == "type_option");
                              VCoverOptionType optType = VCoverOptionType::UNKNOWN;
                              bool valid = true;
                              ਜੇ (!typeOpt) {
                                  // IEEE 1800-2023 Table 19-1 option.* names and ignored legacy names
                                  ਜੇ      (*$3 == "at_least")                optType = VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "auto_bin_max")            optType = VCoverOptionType::AUTO_BIN_MAX;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "comment")                 optType = VCoverOptionType::COMMENT;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "cross_auto_bin_max")      optType = VCoverOptionType::CROSS_AUTO_BIN_MAX;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "cross_num_print_missing") optType = VCoverOptionType::CROSS_ਅੰਕ_PRINT_MISSING;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "cross_retain_auto_bins")  optType = VCoverOptionType::CROSS_RETAIN_AUTO_BINS;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "detect_overlap")          optType = VCoverOptionType::DETECT_OVERLAP;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "get_inst_coverage")       optType = VCoverOptionType::GET_INST_COVERAGE;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "goal")                    optType = VCoverOptionType::GOAL;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "name")                    optType = VCoverOptionType::NAME;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "per_instance")            optType = VCoverOptionType::ਪ੍ਰਤੀ_INSTANCE;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "weight")                  optType = VCoverOptionType::ਭਾਰ;
                                  ਨਹੀਂਤਾਂ {
                                      $<fl>1->v3error("Unknown coverage option name 'option."
                                                      << *$3 << "'"
                                                      << "; not a valid option (IEEE 1800-2023 Table 19-1)");
                                      valid = false;
                                  }
                              } ਨਹੀਂਤਾਂ {
                                  // IEEE 1800-2023 Table 19-3: type_option.* names
                                  ਜੇ      (*$3 == "comment")          optType = VCoverOptionType::COMMENT;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "distribute_first") optType = VCoverOptionType::DISTRIBUTE_ਪਹਿਲਾ;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "goal")             optType = VCoverOptionType::GOAL;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "merge_instances")  optType = VCoverOptionType::ਮਿਲਾਓ_INSTANCES;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "real_interval")    optType = VCoverOptionType::REAL_INTERVAL;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "strobe")           optType = VCoverOptionType::STROBE;
                                  ਨਹੀਂਤਾਂ ਜੇ (*$3 == "weight")           optType = VCoverOptionType::ਭਾਰ;
                                  ਨਹੀਂਤਾਂ {
                                      // Specific message for option.* names used under type_option.*
                                      ਜੇ (*$3 == "at_least" || *$3 == "auto_bin_max"
                                          || *$3 == "cross_num_print_missing" || *$3 == "cross_retain_auto_bins"
                                          || *$3 == "detect_overlap" || *$3 == "get_inst_coverage"
                                          || *$3 == "name" || *$3 == "per_instance") {
                                          $<fl>1->v3error("'type_option." << *$3
                                              << "' is not valid; use 'option." << *$3 << "' instead");
                                      } ਨਹੀਂਤਾਂ {
                                          $<fl>1->v3error("Unknown coverage type option name 'type_option."
                                              << *$3 << "'"
                                              << "; not a valid type option (IEEE 1800-2023 Table 19-3)");
                                      }
                                      valid = false;
                                  }
                              }
                              ਜੇ (valid) {
                                  $$ = new Ast_Cg_Option_ਸੌਂਪੋ{$<fl>1, typeOpt, optType, *$3, $5};
                              } ਨਹੀਂਤਾਂ {
                                  DEL($5);
                              }
                          } ਨਹੀਂਤਾਂ {
                              $<fl>1->v3error("Syntax error; expected 'option' or 'type_option': '" << *$1 << "'");
                              DEL($5);
                          } }
        ;

cover_ਬਿੰਦੂ<nodep>:  // ==IEEE: cover_point
        //              // [ [ data_type_or_implicit ] cover_point_identifier ':' ] yCOVERPOINT
                yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>1, "", $2, $3, $4}; }
        //                      // IEEE-2012: class_scope before an ID
        |       id/*cover_point_id*/ ':' yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>3, *$1, $4, $5, $6}; }
        //                      // data_type_or_implicit expansion
        |       ਅੰਕੜੇ_type id/*cover_point_id*/ ':' yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7};
                          DEL($1); }
        |       yVAR ਅੰਕੜੇ_type id/*cover_point_id*/ ':' yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       yVAR implicit_typeE id/*cover_point_id*/ ':' yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signingE rangeList id/*cover_point_id*/ ':' yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signing id/*cover_point_id*/ ':' yCOVERPOINT ਵਿਅੰਜਕ iffE bins_or_ਖਾਲੀ_ਥਾਵਾਂ
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7}; }
        //                      // IEEE-2012:
        |       bins_or_ਖਾਲੀ_ਥਾਵਾਂ                           { $$ = $1; }
        ;

iffE<ਗੰਢ_Exprp>:  // IEEE: part of cover_point, others
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       yIFF '(' ਵਿਅੰਜਕ ')'
                        { $$ = $3; /* Keep iff condition for coverpoint */ }
        ;

bins_or_ਖਾਲੀ_ਥਾਵਾਂ<nodep>:  // ==IEEE: bins_or_empty
                '{' bins_or_optionsList '}'             { $$ = $2; }
        |       '{' '}'                                 { $$ = nullptr; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' bins_or_optionsList error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

bins_or_optionsList<nodep>:  // IEEE: { bins_or_options ';' }
                bins_or_options ';'                     { $$ = $1; }
        |       bins_or_optionsList bins_or_options ';' { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
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
                          ਜੇ (*$2 == "auto") {
                              $$ = new AstCoverBin{$<fl>2, *$2, $4};
                              DEL($6);
                          } ਨਹੀਂਤਾਂ {
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
                        { ਅਚਲ bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_ਸੈੱਟ*>($5), VCoverBinsType{VCoverBinsType::BINS_TRANSITION}, isArray};
                          ਜੇ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { ਅਚਲ bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_ਸੈੱਟ*>($5), VCoverBinsType{VCoverBinsType::BINS_IGNORE}, isArray};
                          ਜੇ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { ਅਚਲ bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_ਸੈੱਟ*>($5), VCoverBinsType{VCoverBinsType::BINS_ILLEGAL}, isArray};
                          ਜੇ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
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
                          ਜੇ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_IGNORE};
                          ਜੇ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_ILLEGAL};
                          ਜੇ ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        ;

bins_orBraE<ਗੰਢ_Exprp>:  // IEEE: part of bins_or_options: AstUnbounded for '[]', else the size
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       '[' ']'                                 { $$ = new AstUnbounded{$<fl>1}; }
        |       '[' cgexpr ']'
                        { $$ = $2;
                          ਜੇ (VN_IS($2, Unbounded)) $2->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)"); }
        ;

trans_list<nodep>:  // ==IEEE: trans_list
                '(' trans_ਸੈੱਟ ')'                       { $$ = $2; }
        |       trans_list ',' '(' trans_ਸੈੱਟ ')'        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $4); }
        ;

trans_ਸੈੱਟ<nodep>:  // ==IEEE: trans_set (returns AstCoverTransSet)
                trans_range_list                        {
                        Ast_Cover_Trans_ਵਸਤੂ* ਅਚਲ itemp = static_cast<Ast_Cover_Trans_ਵਸਤੂ*>($1);
                        $$ = new Ast_Cover_Trans_ਸੈੱਟ{$<fl>1, itemp};
                }
        |       trans_ਸੈੱਟ yP_EQGT trans_range_list
                {
                        Ast_Cover_Trans_ਸੈੱਟ* ਅਚਲ setp = static_cast<Ast_Cover_Trans_ਸੈੱਟ*>($1);
                        Ast_Cover_Trans_ਵਸਤੂ* ਅਚਲ itemp = static_cast<Ast_Cover_Trans_ਵਸਤੂ*>($3);
                        setp->ਜੋੜੋ_Itemsp(itemp);
                        $$ = setp;
                }
        ;

trans_range_list<nodep>:  // ==IEEE: trans_range_list (returns AstCoverTransItem)
                trans_ਵਸਤੂ                              {
                        // Simple transition item without repetition
                        $$ = new Ast_Cover_Trans_ਵਸਤੂ{$<fl>1, $1, VTransRepType::NONE};
                }
        |       trans_ਵਸਤੂ yP_BRASTAR cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3); }
        |       trans_ਵਸਤੂ yP_BRASTAR cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3, $5); }
        |       trans_ਵਸਤੂ yP_BRAMINUSGT cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($3);
                          $$ = new Ast_Cover_Trans_ਵਸਤੂ{$<fl>1, $1, VTransRepType::GOTO}; }
        |       trans_ਵਸਤੂ yP_BRAMINUSGT cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($1, $3, $5); }
        |       trans_ਵਸਤੂ yP_BRAEQ cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($3);
                          $$ = new Ast_Cover_Trans_ਵਸਤੂ{$<fl>1, $1, VTransRepType::NONCONS}; }
        |       trans_ਵਸਤੂ yP_BRAEQ cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($1, $3, $5); }
        ;

trans_ਵਸਤੂ<nodep>:  // ==IEEE: range_list (returns range list node)
                covergroup_range_list                   { $$ = $1; }
        ;

covergroup_range_list<nodep>:  // ==IEEE: covergroup_range_list
                covergroup_ਮੁੱਲ_range                  { $$ = $1; }
        |       covergroup_range_list ',' covergroup_ਮੁੱਲ_range        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;


cover_cross<nodep>:  // ==IEEE: cover_cross
                id/*cover_point_identifier*/ ':' yCROSS list_ਦਾ_cross_ਚੀਜ਼ਾਂ iffE cross_body
                        {
                          AstCoverCross* ਅਚਲ nodep = new AstCoverCross{$<fl>3, *$1,
                                                          VN_AS($4, CoverpointRef), $5};
                          ਜੇ ($6) nodep->ਜੋੜੋ_Binsp($6);
                          $$ = nodep;
                        }
        |       yCROSS list_ਦਾ_cross_ਚੀਜ਼ਾਂ iffE cross_body
                        {
                          AstCoverCross* ਅਚਲ nodep = new AstCoverCross{$<fl>1,
                                                          "__cross" + cvt_ਮੰਜ਼ਿਲ_Str(GRAMMARP->s_type_Imp_ਅੰਕ++),
                                                          VN_AS($2, CoverpointRef), $3};
                          ਜੇ ($4) nodep->ਜੋੜੋ_Binsp($4);
                          $$ = nodep;
                        }
        ;

list_ਦਾ_cross_ਚੀਜ਼ਾਂ<nodep>:  // ==IEEE: list_of_cross_items
                cross_ਵਸਤੂ ',' cross_ਵਸਤੂ               { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        |       cross_ਵਸਤੂ ',' cross_ਵਸਤੂ ',' cross_ਵਸਤੂ_List
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null(ਜੋੜੋ_ਅਗਲਾ_Null($1, $3), $5); }
        ;

cross_ਵਸਤੂ_List<nodep>:  // IEEE: part of list_of_cross_items
                cross_ਵਸਤੂ                              { $$ = $1; }
        |       cross_ਵਸਤੂ_List ',' cross_ਵਸਤੂ           { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

cross_ਵਸਤੂ<nodep>:  // ==IEEE: cross_item
        //                      // IEEE: cover_point_identifier | variable_identifier - both are a
        //                      // simple identifier.  We parse idDotted (a plain hierarchical
        //                      // reference a.b.c, with no bit/array selects) to also accept the
        //                      // non-standard dotted form (e.g. 'cross a.b') that several
        //                      // simulators support; the common simple-identifier case is detected
        //                      // and handled exactly as before.
                idDotted
                        {
                          ਜੇ (AstParseRef* ਅਚਲ refp = VN_CAST($1, ParseRef)) {
                              // Standard: simple cover_point_identifier / variable_identifier
                              $$ = new AstCoverpointRef{refp->fileline(), refp->name()};
                              VL_DO_DANGLING(refp->delete_ਰੁੱਖ(), refp);
                          } ਨਹੀਂਤਾਂ {
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
        |       '{' cross_body_ਵਸਤੂ_List '}'             { $$ = $2; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' cross_body_ਵਸਤੂ_List error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

cross_body_ਵਸਤੂ_List<nodep>:  // IEEE: part of cross_body
                cross_body_ਵਸਤੂ                         { $$ = $1; }
        |       cross_body_ਵਸਤੂ_List cross_body_ਵਸਤੂ     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

cross_body_ਵਸਤੂ<nodep>:  // ==IEEE: cross_body_item
                function_declaration
                        { $$ = nullptr; BBCOVERIGN($1->fileline(), "Unsupported: 'function' in coverage cross body"); DEL($1); }
        //                      // IEEE: bins_selection_or_option
        |       coverage_option ';'                     { $$ = $1; }
        //                      // IEEE: bins_selection
        |       yBINS idAny/*new-bin_identifier*/ '=' select_ਵਿਅੰਜਕ iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5}; }
        |       yIGNORE_BINS idAny/*new-bin_identifier*/ '=' select_ਵਿਅੰਜਕ iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_IGNORE}; }
        |       yILLEGAL_BINS idAny/*new-bin_identifier*/ '=' select_ਵਿਅੰਜਕ iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_ILLEGAL}; }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

select_ਵਿਅੰਜਕ<nodep>:  // ==IEEE: select_expression
                select_ਵਿਅੰਜਕ_r
                        { $$ = $1; }
        //                      // IEEE: integer_covergroup_expression ::= covergroup_expression | '$'
        //                      // '$' is part of our expr.  'matches' has higher precedence than
        //                      // '&&' and '||', which end the count (IEEE 1800-2023 footnote 30).
        //                      // A count could end in a function call followed by its 'with', so
        //                      // a selection with a count is filtered further only in parentheses.
        |       select_ਵਿਅੰਜਕ_r yWITH__PAREN '(' cgexpr ')' yMATCHES ਵਿਅੰਜਕ %prec yP_ANDAND
                        { $$ = new AstCoverCrossWith{$2, $1, $4, $7}; }
        //                      // IEEE: cross_set_expression yMATCHES integer_covergroup_expression
        |       idAny '(' list_ਦਾ_arguments_E ')' yMATCHES ਵਿਅੰਜਕ %prec yP_ANDAND
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3, $6); }
        |       select_ਵਿਅੰਜਕ yP_ANDAND select_ਵਿਅੰਜਕ
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, false}; }
        |       select_ਵਿਅੰਜਕ yP_OROR   select_ਵਿਅੰਜਕ
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, true}; }
        ;

// This non-terminal exists to disambiguate select_expression and make "with" bind tighter
select_ਵਿਅੰਜਕ_r<nodep>:
        //                      // IEEE: select_condition expanded here
                yBINSOF '(' bins_ਵਿਅੰਜਕ ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}}; }
        |       '!' yBINSOF '(' bins_ਵਿਅੰਜਕ ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true}; }
        //                      // IEEE: covergroup_range_list has the same syntax as range_list
        |       yBINSOF '(' bins_ਵਿਅੰਜਕ ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}, false, $7}; }
        |       '!' yBINSOF '(' bins_ਵਿਅੰਜਕ ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true, $8}; }
        |       yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($3); }
        |       '!' yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($4); }
        |       select_ਵਿਅੰਜਕ_r yWITH__PAREN '(' cgexpr ')'
                        { $$ = new AstCoverCrossWith{$2, $1, $4, nullptr}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr    { }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP '!' yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr { }
        //
        |       '(' select_ਵਿਅੰਜਕ ')'                       { $$ = $2; }
        |       idAny/*cross_identifier*/                        { $$ = new AstCoverCrossRef{$<fl>1, *$1}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP  cgexpr  { $$ = nullptr; BBCOVERIGN($1, "Ignoring unsupported: coverage select expression"); }
        //
        //                      // IEEE: cross_set_expression [ yMATCHES integer_covergroup_expression ]
        //UNSUP  cgexpr    {..}
        //UNSUP                 // Below are all removed
        |       idAny '(' list_ਦਾ_arguments_E ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3); }
        //UNSUP                 // Above are all removed, replace with:
        ;

bins_ਵਿਅੰਜਕ<ਗੰਢ_Exprp>:  // ==IEEE: bins_expression
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       clocking_event
                        { $$ = $1; }  // Keep the clocking event for automatic sampling
        |       yWITH__ETC yFUNCTION idAny/*"sample"*/ '(' tf_ਪੋਰਟ_list_E ')'
                        { ਜੇ (*$3 != "sample") {
                            $<fl>3->v3error("Coverage sampling function must be named 'sample'"
                                            " (IEEE 1800-2023 19.8.1)");
                            $$ = nullptr;
                            DEL($5);
                          } ਨਹੀਂਤਾਂ {
                            $$ = $5;
                          }
                        }
        |       yP_ATAT '(' ਖੰਡ_event_ਵਿਅੰਜਕ ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: '@@' coverage event"); DEL($3); }
        ;

ਖੰਡ_event_ਵਿਅੰਜਕ<nodep>:  // ==IEEE: block_event_expression
                ਖੰਡ_event_ਵਿਅੰਜਕ_ਪਦ              { $$ = nullptr; /*UNSUP @@*/ DEL($1); }
        |       ਖੰਡ_event_ਵਿਅੰਜਕ yOR ਖੰਡ_event_ਵਿਅੰਜਕ_ਪਦ   { $$ = nullptr; /*UNSUP @@*/ DEL($1, $3);  }
        ;

ਖੰਡ_event_ਵਿਅੰਜਕ_ਪਦ<nodep>:  // IEEE: part of block_event_expression
                yBEGIN ਦਰਜਾਵਾਰ_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        |       y_ਸਿਰਾ   ਦਰਜਾਵਾਰ_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        ;

ਦਰਜਾਵਾਰ_btf_identifier<nodep>:  // ==IEEE: hierarchical_btf_identifier
        //                      // hierarchical_tf_identifier + hierarchical_block_identifier
        //                      // method_identifier
                packageClassScopeE idAny                { $$ = nullptr; /*UNSUP*/ DEL($1); }
        ;

//**********************************************************************
// Randsequence

randsequence_statement<ਗੰਢ_Stmtp>:  // ==IEEE: randsequence_statement
                yRANDSEQUENCE '(' ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, "", $4};
                          v3Global.useRandSequence(true); }
        |       yRANDSEQUENCE '(' idAny/*rs_production_identifier*/ ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, *$3, $5};
                          v3Global.useRandSequence(true); }
        ;

rs_productionList<rSProdp>:  // IEEE: rs_production+
                rs_production                           { $$ = $1; }
        |       rs_productionList rs_production         { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

rs_production<rSProdp>:  // ==IEEE: rs_production
                rs_productionFront ':' rs_ਨਿਯਮ_List ';'
                        { $$ = $1; $1->ਜੋੜੋ_Rulesp($3); }
        ;

rs_productionFront<rSProdp>:  // IEEE: part of rs_production
                rs_funcId/*rs_production_identifier*/   { $$ = $1; }
        |       rs_funcId '(' tf_ਪੋਰਟ_list_E ')'         { $$ = $1; $$->ਜੋੜੋ_Portsp($3); }
        ;

rs_funcId<rSProdp>:  // IEEE: part of rs_production
                /**/ rs_fId
                        { $$ = $1; }  // Note is void as default, not logic as default like functions
        |       signingE rangeList rs_fId
                        { $$ = $3;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::ਪੋਰਟ, $3->name(), VFlag_ਬੱਚਾ_DType{},
                                               GRAMMARP->ਜੋੜੋ_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)}); }
        |       signing rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::ਪੋਰਟ, $2->name(), VFlag_ਬੱਚਾ_DType{},
                                               new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}}); }
        |       ਅੰਕੜੇ_type rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::ਪੋਰਟ, $2->name(), VFlag_ਬੱਚਾ_DType{},
                                               $1}); }
        |       yVOID rs_fId
                        { $$ = $2; }
        ;

rs_fId<rSProdp>:  // IEEE: part of rs_production
                id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstRSProd{$<fl>$, *$1, nullptr, nullptr}; }
        ;

rs_ਨਿਯਮ_List<rSRulep>:  // IEEE: rs_rule+ part of rs_production
                rs_ਨਿਯਮ                                 { $$ = $1; }
        |       rs_ਨਿਯਮ_List '|' rs_ਨਿਯਮ                 { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

rs_ਨਿਯਮ<rSRulep>:  // ==IEEE: rs_rule
                rs_production_list
                        { $$ = new AstRSRule{$1->fileline(), nullptr, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_ਭਾਰ_specification
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_ਭਾਰ_specification rs_ਕੋਡ_ਖੰਡ
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, $4}; }
        ;

rs_production_list<rSProdListp>:  // ==IEEE: rs_production_list
                rs_prodList
                        { $$ = new AstRSProdList{CRELINE(), nullptr, $1}; }
        |       yRAND yJOIN rs_production_ਵਸਤੂ rs_production_ਵਸਤੂ_List
                        { $$ = new AstRSProdList{$1, new AstConst{$2, AstConst::RealDouble{}, 0.5}, $3};
                          $$->randJoin(true);
                          $$->ਜੋੜੋ_Prodsp($4); }
        |       yRAND yJOIN '(' ਵਿਅੰਜਕ ')' rs_production_ਵਸਤੂ rs_production_ਵਸਤੂ_List
                        { $$ = new AstRSProdList{$1, $4, $6};
                          $$->randJoin(true);
                          $$->ਜੋੜੋ_Prodsp($7); }
        ;

rs_ਭਾਰ_specification<ਗੰਢ_Exprp>:  // ==IEEE: rs_weight_specification
                intnumAsConst                           { $$ = $1; }
        |       idClassSel/*ps_identifier*/             { $$ = $1; }
        |       '(' ਵਿਅੰਜਕ ')'                            { $$ = $2; }
        ;

rs_ਕੋਡ_ਖੰਡ<nodep>:  // ==IEEE: rs_code_block
                '{' '}'                                 { $$ = nullptr; }
        |       '{' rs_ਕੋਡ_ਖੰਡ_ਵਸਤੂ_List '}'           { $$ = $2; }
        ;

rs_ਕੋਡ_ਖੰਡ_ਵਸਤੂ_List<nodep>:  // IEEE: part of rs_code_block
                rs_ਕੋਡ_ਖੰਡ_ਵਸਤੂ                       { $$ = $1; }
        |       rs_ਕੋਡ_ਖੰਡ_ਵਸਤੂ_List rs_ਕੋਡ_ਖੰਡ_ਵਸਤੂ         { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

rs_ਕੋਡ_ਖੰਡ_ਵਸਤੂ<nodep>:  // IEEE: part of rs_code_block
                ਅੰਕੜੇ_declaration                        { $$ = $1; }
        |       stmt                                    { $$ = $1; }
        ;

rs_prodList<nodep>:  // IEEE: rs_prod+
                rs_prod                                 { $$ = $1; }
        |       rs_prodList rs_prod                     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

rs_prod<nodep>:  // ==IEEE: rs_prod
                rs_production_ਵਸਤੂ                      { $$ = $1; }
        |       rs_ਕੋਡ_ਖੰਡ                           { $$ = $1; }
        //                      // IEEE: rs_if_else
        |       yIF '(' ਵਿਅੰਜਕ ')' rs_production_ਵਸਤੂ %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = new AstIf{$<fl>1, $3, $5, nullptr}; }
        |       yIF '(' ਵਿਅੰਜਕ ')' rs_production_ਵਸਤੂ yELSE rs_production_ਵਸਤੂ
                        { $$ = new AstIf{$<fl>1, $3, $5, $7}; }
        //                      // IEEE: rs_repeat
        |       yREPEAT '(' ਵਿਅੰਜਕ ')' rs_production_ਵਸਤੂ
                               { $$ = new AstRepeat{$<fl>1, $3, $5}; }
        //                      // IEEE: rs_case
        |       yCASE '(' ਵਿਅੰਜਕ ')' rs_case_ਵਸਤੂ_List yENDCASE
                        { $$ = new AstCase{$<fl>1, VCaseType::CT_RANDSEQUENCE, $3, $5}; }
        ;

rs_production_ਵਸਤੂ_List<nodep>:  // IEEE: rs_production_item+
                rs_production_ਵਸਤੂ                         { $$ = $1; }
        |       rs_production_ਵਸਤੂ_List rs_production_ਵਸਤੂ  { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

rs_production_ਵਸਤੂ<nodep>:  // ==IEEE: rs_production_item
                idAny/*production_identifier*/
                        { $$ = new Ast_RSProd_ਵਸਤੂ{$<fl>1, *$1, nullptr}; }
        |       idAny/*production_identifier*/ '(' list_ਦਾ_arguments_E ')'
                        { $$ = new Ast_RSProd_ਵਸਤੂ{$<fl>1, *$1, $3}; }
        ;

rs_case_ਵਸਤੂ_List<caseItemp>:  // IEEE: rs_case_item+
                rs_case_ਵਸਤੂ                            { $$ = $1; }
        |       rs_case_ਵਸਤੂ_List rs_case_ਵਸਤੂ           { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

rs_case_ਵਸਤੂ<caseItemp>:  // ==IEEE: rs_case_item
                caseCondList ':' rs_production_ਵਸਤੂ ';'
                        { $$ = new Ast_Case_ਵਸਤੂ{$<fl>1, $1, $3}; }
        |       yDEFAULT rs_production_ਵਸਤੂ ';'
                        { $$ = new Ast_Case_ਵਸਤੂ{$<fl>1, nullptr, $2}; }
        |       yDEFAULT ':' rs_production_ਵਸਤੂ ';'
                        { $$ = new Ast_Case_ਵਸਤੂ{$<fl>1, nullptr, $3}; }
        ;

//**********************************************************************
// Checker

checker_declaration<ਗੰਢ_Modulep>:  // ==IEEE: part of checker_declaration
                checkerFront checker_ਪੋਰਟ_list_E ';'
                        checker_or_generate_ਵਸਤੂ_List_E yENDCHECKER ਸਿਰਾ_Label_E
                        { $$ = $1;
                          $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));
                          ਜੇ ($2) $1->ਜੋੜੋ_Stmtsp($2);
                          ਜੇ ($4) $1->ਜੋੜੋ_Stmtsp($4);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>6, $1, $6); }
        ;

checkerFront<ਗੰਢ_Modulep>:  // IEEE: part of checker_declaration
                yCHECKER idAny/*checker_identifier*/
                        { $$ = new AstModule{$<fl>2, *$2, PARSEP->libname(), AstModule::Checker{}};
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->ਸਮਾਂ_ਆਖਰੀ_Unit());
                          PARSEP->rootp()->timeprecision_ਮਿਲਾਓ($$->fileline(),
                                                              PARSEP->ਸਮਾਂ_ਆਖਰੀ_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive()); }
        |       checkerFront sigAttrScope               { $$ = $1; }
        ;

checker_ਪੋਰਟ_list_E<nodep>:  // IEEE: [ ( [ checker_port_list ] ) ]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '('
        /*mid*/         { VARRESET_LIST(ਪੋਰਟ); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    checker_ਪੋਰਟ_list ')'
                        { $$ = $3; }
        ;

checker_ਪੋਰਟ_list<nodep>:  // ==IEEE: checker_port_list
                checker_ਪੋਰਟ_ਵਸਤੂ                             { $$ = $1; }
        |       checker_ਪੋਰਟ_list ',' checker_ਪੋਰਟ_ਵਸਤੂ       { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

checker_ਪੋਰਟ_ਵਸਤੂ<nodep>:  // IEEE: checker_port_item
                checker_ਪੋਰਟ_ਵਸਤੂ_Front checker_ਪੋਰਟ_ਵਸਤੂ_Assignment  { $$ = $2; }
        ;

checker_ਪੋਰਟ_ਵਸਤੂ_Front:  // IEEE: part of checker_port_item
                checker_ਪੋਰਟ_direction_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       checker_ਪੋਰਟ_direction_E ਅੰਕੜੇ_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_ਪੋਰਟ = true; }
        |       checker_ਪੋਰਟ_direction_E yVAR ਅੰਕੜੇ_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_ਪੋਰਟ = true; }
        |       checker_ਪੋਰਟ_direction_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       checker_ਪੋਰਟ_direction_E implicit_typeE
                        { VARDTYPE($2); }
        ;

checker_ਪੋਰਟ_direction_E:  // IEEE: [ checker_port_direction ]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { VARIO(ਨਿਵੇਸ਼); }
        |       y_ਨਿਵੇਸ਼                                  { VARIO(ਨਿਵੇਸ਼); }
        |       yOUTPUT                                 { VARIO(OUTPUT); }
        ;

checker_ਪੋਰਟ_ਵਸਤੂ_Assignment<nodep>:  // IEEE: part of checker_port_direction
                id variable_dimensionListE
                        { $$ = new Ast_ਪੋਰਟ{CRELINE(), PINNUMINC(), *$1};
                          $$->ਜੋੜੋ_ਅਗਲਾ(VARDONEA($<fl>1, *$1, $2, nullptr)); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { $$ = new Ast_ਪੋਰਟ{CRELINE(), PINNUMINC(), *$1};
                          $$->ਜੋੜੋ_ਅਗਲਾ(VARDONEA($<fl>1, *$1, $2, $4));
                          BBUNSUP($3, "Unsupported: checker port variable default value"); }
        ;

checker_or_generate_ਵਸਤੂ_List_E<nodep>:  // IEEE: [{ checker_or_generate_itemList }]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       checker_or_generate_ਵਸਤੂ_List            { $$ = $1; }
        ;

checker_or_generate_ਵਸਤੂ_List<nodep>:  // IEEE: { checker_or_generate_itemList }
                checker_or_generate_ਵਸਤੂ                { $$ = $1; }
        |       checker_or_generate_ਵਸਤੂ_List checker_or_generate_ਵਸਤੂ   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

checker_or_generate_ਵਸਤੂ<nodep>:  // ==IEEE: checker_or_generate_item
                checker_or_generate_ਵਸਤੂ_declaration    { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        //                      // IEEE: checker_construct
        |       always_construct                        { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       assertion_ਵਸਤੂ                          { $$ = $1; }
        |       continuous_ਸੌਂਪੋ                       { $$ = $1; }
        |       checker_generate_ਵਸਤੂ                   { $$ = $1; }
        ;

checker_or_generate_ਵਸਤੂ_declaration<nodep>:  // ==IEEE: checker_or_generate_item_declaration
                ਅੰਕੜੇ_declaration                        { $$ = $1; }
        |       yRAND ਅੰਕੜੇ_declaration
                        { $$ = $2; BBUNSUP($1, "Unsupported: checker rand"); }
        |       function_declaration                    { $$ = $1; }
        |       checker_declaration
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: recursive 'checker'"); DEL($1); }
        |       assertion_ਵਸਤੂ_declaration              { $$ = $1; }
        |       covergroup_declaration                  { $$ = $1; }
        //      // IEEE deprecated: overload_declaration
        |       genvar_declaration                      { $$ = $1; }
        |       clocking_declaration                    { $$ = $1; }
        |       modDefaultClocking                      { $$ = $1; }
        |       defaultDisable                          { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

checker_generate_ਵਸਤੂ<nodep>:  // ==IEEE: checker_generate_item
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
                classFront parameter_ਪੋਰਟ_list_E classExtendsE classImplementsE ';'
        /*mid*/         { // Allow resolving types declared in base extends class
                          $1->hasParameterList($<flag>2);
                        }
        /*cont*/    class_ਵਸਤੂ_List_ਸਿਰਾ ਸਿਰਾ_Label_E
                        { $$ = $1; $1->ਜੋੜੋ_Membersp($2);
                          $1->ਜੋੜੋ_Extendsp($3);
                          $1->ਜੋੜੋ_Extendsp($4);
                          $1->ਜੋੜੋ_Membersp($7);
                          GRAMMARP->ਸਿਰਾ_Label($<fl>8, $1, $8); }
        ;

classFront<classp>:             // IEEE: part of class_declaration
        //                      // IEEE 1800-2023: lifetimeE replaced with final_specifierE
                classVirtualE yCLASS final_specifierE lifetimeE idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$5, PARSEP->libname()};
                          $$->ਅਧਾਰ_Override($3);
                          $$->isVirtual($1);
                          v3Global.ਸੈੱਟ_Has_Classes(); }
        //                      // IEEE: part of interface_class_declaration
        //                      // IEEE 1800-2023: lifetimeE removed
        |       yINTERFACE yCLASS idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$3, PARSEP->libname()};
                          $$->isInterfaceClass(true);
                          v3Global.ਸੈੱਟ_Has_Classes(); }
        ;

classVirtualE<cbool>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = false; }
        |       yVIRTUAL__CLASS                         { $$ = true; }
        ;

classExtendsE<classExtendsp>:           // IEEE: part of class_declaration
        //                      // The classExtendsE rule relies on classFront having the
        //                      // new class scope correct via classFront
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       yEXTENDS classExtendsList               { $$ = $2; }
        ;

classExtendsList<classExtendsp>:        // IEEE: part of class_declaration
                classExtendsOne                         { $$ = $1; }
        |       classExtendsList ',' classExtendsOne    { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

classExtendsOne<classExtendsp>:         // IEEE: part of class_declaration
                class_typeExtImpList
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_ਵਿੱਚ_Implements}; }
        |       class_typeExtImpList '(' list_ਦਾ_arguments_E ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_ਵਿੱਚ_Implements};
                          $$->ਜੋੜੋ_Argsp($3); }
        //                      // IEEE-2023: Added: yEXTENDS class_type '(' yDEFAULT ')'
        |       class_typeExtImpList '(' yDEFAULT ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_ਵਿੱਚ_Implements};
                          BBUNSUP($<fl>2, "Unsupported: 'extends' with 'default'"); }
        ;

classImplementsE<classExtendsp>:        // IEEE: part of class_declaration
        //                      // All 1800-2012
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       yIMPLEMENTS
        /*mid*/         { GRAMMARP->m_ਵਿੱਚ_Implements = true; }
        /*cont*/    classImplementsList
                        { $$ = $3;
                          GRAMMARP->m_ਵਿੱਚ_Implements = false; }
        ;

classImplementsList<classExtendsp>:     // IEEE: part of class_declaration
        //                      // All 1800-2012
                classExtendsOne                         { $$ = $1; }
        |       classImplementsList ',' classExtendsOne
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

class_typeExtImpList<ਗੰਢ_Exprp>:  // IEEE: class_type: "[package_scope] id [ parameter_value_assignment ]"
        //                      // but allow yaID__aTYPE for extends/implements
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                class_typeExtImpOne                     { $$ = $1; }
        |       class_typeExtImpList yP_COLONCOLON class_typeExtImpOne
                        { $$ = new AstDot{$<fl>1, true, $1, $3}; }
        ;

class_typeExtImpOne<ਗੰਢ_Exprp>:  // part of IEEE: class_type, where we either get a package_scope component or class
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
        //                      // Not listed in IEEE, but see bug627 any parameter type maybe a class
        //                      // If idAny below is a class, parameter_value is legal
        //                      // If idAny below is a package, parameter_value is not legal
        //                      // If idAny below is otherwise, not legal
                idAny
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_ਮੁੱਲ_assignment_Class_E
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $3}; }
        |       idCC
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_ਮੁੱਲ_assignment_Class_E
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

packageClassScopeNoId<ਗੰਢ_Exprp>:  // IEEE: [package_scope] not followed by yaID
                packageClassScope                       { $$ = $1; }
        ;

packageClassScopeE<ਗੰਢ_Exprp>:  // IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // TODO: To support classes should return generic type, not packagep
        //                      // class_qualifier := [ yLOCAL '::'  ] [ implicit_class_handle '.'  class_scope ]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       packageClassScope                       { $$ = $1; }
        ;

packageClassScope<ਗੰਢ_Exprp>:   // IEEE: class_scope
        //                      // IEEE: "class_type yP_COLONCOLON"
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
                packageClassScopeList                   { $$ = $1; }
        |       local_ਅਗਲਾ_Id yP_COLONCOLON               { $$ = $1; }
        |       dollar_Unit_ਅਗਲਾ_Id yP_COLONCOLON          { $$ = $1; }
        |       dollar_Unit_ਅਗਲਾ_Id yP_COLONCOLON packageClassScopeList
                        { $$ = new AstDot{$2, true, $1, $3}; }
        ;

packageClassScopeList<ਗੰਢ_Exprp>:   // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      // Or IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                package_Class_Scope_ਵਸਤੂ                   { $$ = $1; }
        |       packageClassScopeList package_Class_Scope_ਵਸਤੂ
                        { $$ = new AstDot{$<fl>2, true, $1, $2}; }
        ;

package_Class_Scope_ਵਸਤੂ<ਗੰਢ_Exprp>:   // IEEE: package_scope or [package_scope]::[class_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      //vv mid rule action needed otherwise we might not have NextId in time to parse the id token
                idCC
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, nullptr}; }
        //
        |       idCC parameter_ਮੁੱਲ_assignment_Class
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $2}; }
        ;

dollar_Unit_ਅਗਲਾ_Id<ਗੰਢ_Exprp>:    // $unit
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yD_UNIT
                        { $$ = new AstClassOrPackageRef{$1, "$unit", nullptr, nullptr}; }
        ;

local_ਅਗਲਾ_Id<ਗੰਢ_Exprp>:         // local::
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yLOCAL__COLONCOLON
                        { $$ = new AstClassOrPackageRef{$1, "local::", nullptr, nullptr}; }
        ;

//^^^=========

class_ਵਸਤੂ_List_ਸਿਰਾ<nodep>:
                yENDCLASS                               { $$ = nullptr; }
        |       class_ਵਸਤੂ_List yENDCLASS                { $$ = $1; }
        |       error yENDCLASS                         { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       class_ਵਸਤੂ_List error yENDCLASS          { $$ = $1; }  // LCOV_EXCL_LINE
        ;

class_ਵਸਤੂ_List<nodep>:
                class_ਵਸਤੂ                              { $$ = $1; }
        |       class_ਵਸਤੂ_List class_ਵਸਤੂ               { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

class_ਵਸਤੂ<nodep>:                      // ==IEEE: class_item
                class_property                          { $$ = $1; }
        |       class_ਵਿਧੀ_2                            { $$ = $1; }
        |       class_constraint                        { $$ = $1; }
        //
        |       class_declaration                       { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       covergroup_declaration
                        {
                          ਜੇ ($1) {
                              // Also declares an instance variable, of the covergroup's type
                              AstVar* ਅਚਲ newp = new AstVar{$1->fileline(), VVarType::VAR,
                                  $1->name(), VFlag_ਬੱਚਾ_DType{},
                                  new AstRefDType($1->fileline(), $1->name())};
                              newp->embeddedCovergroup(true);
                              $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, newp);
                          } ਨਹੀਂਤਾਂ {
                              $$ = nullptr;
                          }
                        }
        //                      // local_parameter_declaration under parameter_declaration
        |       parameter_declaration ';'               { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        //                      // Verilator specific
        |       vl_Sc_ਖੰਡ                               { $$ = $1; }
        //
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

class_ਵਿਧੀ_2<nodep>:            // ==IEEE: class_method
                memberQualListE task_declaration        { $$ = $2; $1.apply_ਮੰਜ਼ਿਲ_Nodes($2); }
        |       memberQualListE function_declaration    { $$ = $2; $1.apply_ਮੰਜ਼ਿਲ_Nodes($2); }
        |       y_ਸ਼ੁੱਧ yVIRTUAL__ETC memberQualListE ਵਿਧੀ_prototype ';'
                        { $$ = $4; $3.apply_ਮੰਜ਼ਿਲ_Nodes($4); $4->ਸ਼ੁੱਧ_Virtual(true); $4->isVirtual(true); }
        |       yEXTERN memberQualListE ਵਿਧੀ_prototype ';'
                        { $$ = $3; $2.apply_ਮੰਜ਼ਿਲ_Nodes($3); $3->isExternProto(true); }
        //                      // IEEE: "method_qualifierE class_constructor_declaration"
        //                      // part of function_declaration
        |       yEXTERN memberQualListE class_constructor_prototype
                        { $$ = $3; $2.apply_ਮੰਜ਼ਿਲ_Nodes($3); $3->isExternProto(true); }
        ;

dynamic_override_specifiersE<ਅਧਾਰ_Override>:  // IEEE: dynamic_override_specifiers or empty
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = VBaseOverride{}; }
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

final_specifierE<ਅਧਾਰ_Override>:  // ==IEEE: final_specifier (similar to dynamic_override_specifiers)
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = VBaseOverride{}; }
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
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew constraint_ਖੰਡ
                        { $$ = $4; $$->isStatic($1); $$->ਅਧਾਰ_Override($3); $$->ਜੋੜੋ_Itemsp($5); }
        //                      // IEEE: constraint_prototype + constraint_prototype_qualifier
        |       constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $4; $$->isStatic($1); $$->ਅਧਾਰ_Override($3);
                          $$->isExternProto(true); }
        |       yEXTERN constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $5; $$->isStatic($2); $$->ਅਧਾਰ_Override($4);
                          $$->isExternProto(true); $$->isExternExplicit(true); }
        |       y_ਸ਼ੁੱਧ constraintStaticE yCONSTRAINT constraintIdNew ';'
                        { $$ = $4; $$->is_Kwd_ਸ਼ੁੱਧ($1); $$->isStatic($2); }
        ;

constraintIdNew<constraintp>:  // IEEE: id part of class_constraint
                idAny/*constraint_identifier*/
                        { $$ = new AstConstraint{$<fl>1, *$1, nullptr}; }
        ;

constraint_ਖੰਡ<nodep>:  // ==IEEE: constraint_block
                '{' '}'                                         { $$ = nullptr; }
        |       '{' constraint_ਖੰਡ_ਵਸਤੂ_List '}'               { $$ = $2; }
        //
        |       '{' error '}'                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_ਖੰਡ_ਵਸਤੂ_List error '}'         { $$ = $2; }  // LCOV_EXCL_LINE
        ;

constraint_ਖੰਡ_ਵਸਤੂ_List<nodep>:  // IEEE: { constraint_block_item }
                constraint_ਖੰਡ_ਵਸਤੂ                           { $$ = $1; }
        |       constraint_ਖੰਡ_ਵਸਤੂ_List constraint_ਖੰਡ_ਵਸਤੂ { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

constraint_ਖੰਡ_ਵਸਤੂ<nodep>:  // ==IEEE: constraint_block_item
                constraint_ਵਿਅੰਜਕ                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        ;

solve_before_list<ਗੰਢ_Exprp>:  // ==IEEE: solve_before_list
                constraint_primary                              { $$ = $1; }
        |       solve_before_list ',' constraint_primary        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

constraint_primary<ਗੰਢ_Exprp>:  // ==IEEE: constraint_primary
        //                      // exprScope more general than:
        //                      // [ implicit_class_handle '.' | class_scope ] hierarchical_identifier select [ '(' ')' ]
                ਵਿਅੰਜਕ_Scope                                       { $$ = $1; }
        ;

constraint_ਵਿਅੰਜਕ_List<nodep>:  // ==IEEE: { constraint_expression }
                constraint_ਵਿਅੰਜਕ                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        |       constraint_ਵਿਅੰਜਕ_List constraint_ਵਿਅੰਜਕ { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        |       constraint_ਵਿਅੰਜਕ_List ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, new AstConstraintBefore{$<fl>2, $3, $5}); }
        ;

constraint_ਵਿਅੰਜਕ<nodep>:  // ==IEEE: constraint_expression
                ਵਿਅੰਜਕ/*expression_or_dist*/ ';'
                        { $$ = new Ast_Constraint_ਵਿਅੰਜਕ{$1->fileline(), $1}; }
        //                      // 1800-2012:
        |       ySOFT ਵਿਅੰਜਕ/*expression_or_dist*/ ';'
                        { Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ newp = new Ast_Constraint_ਵਿਅੰਜਕ{$1, $2};
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
        |       ਵਿਅੰਜਕ yP_MINUSGT '{' constraint_ਵਿਅੰਜਕ_List '}'
                        { $$ = new AstConstraintIf{$2, $1, $4, nullptr}; }
        |       ਵਿਅੰਜਕ yP_MINUSGT yIF '(' ਵਿਅੰਜਕ ')' constraint_ਸੈੱਟ %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, nullptr}, nullptr}; }
        |       ਵਿਅੰਜਕ yP_MINUSGT yIF '(' ਵਿਅੰਜਕ ')' constraint_ਸੈੱਟ yELSE constraint_ਸੈੱਟ
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, $9}, nullptr}; }
        |       ਵਿਅੰਜਕ yP_MINUSGT yFOREACH '(' idClassSelForeach ')' constraint_ਸੈੱਟ
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintForeach{$3, $5, $7}, nullptr}; }
        |       ਵਿਅੰਜਕ yP_MINUSGT yUNIQUE '{' range_list '}' ';'
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintUnique{$3, $5}, nullptr}; }
        |       ਵਿਅੰਜਕ yP_MINUSGT ySOFT ਵਿਅੰਜਕ ';'
                        { Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ innerp = new Ast_Constraint_ਵਿਅੰਜਕ{$3, $4};
                          innerp->isSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       ਵਿਅੰਜਕ yP_MINUSGT yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ innerp = new Ast_Constraint_ਵਿਅੰਜਕ{$3, $5};
                          innerp->isDisableSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       yIF '(' ਵਿਅੰਜਕ ')' constraint_ਸੈੱਟ %prec pr_ਹੇਠਲਾ_THAN_ELSE
                        { $$ = new AstConstraintIf{$1, $3, $5, nullptr}; }
        |       yIF '(' ਵਿਅੰਜਕ ')' constraint_ਸੈੱਟ yELSE constraint_ਸੈੱਟ
                        { $$ = new AstConstraintIf{$1, $3, $5, $7}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM + 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' constraint_ਸੈੱਟ
                        { $$ = new AstConstraintForeach{$1, $3, $5}; }
        //                      // Non-IEEE extension, soft foreach
        |       ySOFT yFOREACH '(' idClassSelForeach ')' constraint_ਸੈੱਟ
                        { $$ = new AstConstraintForeach{$2, $4, $6, true}; }
        //                      // soft is 1800-2012
        |       yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_ਵਿਅੰਜਕ* ਅਚਲ newp = new Ast_Constraint_ਵਿਅੰਜਕ{$1, $3};
                          newp->isDisableSoft(true);
                          $$ = newp; }
        //
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

constraint_ਸੈੱਟ<nodep>:  // ==IEEE: constraint_set
                constraint_ਵਿਅੰਜਕ                   { $$ = $1; }
        |       '{' constraint_ਵਿਅੰਜਕ_List '}'       { $$ = $2; }
        //
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_ਵਿਅੰਜਕ_List error '}' { $$ = $2; }  // LCOV_EXCL_LINE
        ;

dist_list<distItemp>:  // ==IEEE: dist_list
                dist_ਵਸਤੂ                               { $$ = $1; }
        |       dist_list ',' dist_ਵਸਤੂ                 { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

dist_ਵਸਤੂ<distItemp>:  // ==IEEE: dist_item + dist_weight
                ਮੁੱਲ_range
                        { $$ = new Ast_Dist_ਵਸਤੂ{$1->fileline(), $1, new AstConst{$1->fileline(), 1}}; }
        |       ਮੁੱਲ_range yP_COLONEQ  ਵਿਅੰਜਕ
                        { $$ = new Ast_Dist_ਵਸਤੂ{$2, $1, $3}; }
        |       ਮੁੱਲ_range yP_COLONDIV ਵਿਅੰਜਕ
                        { $$ = new Ast_Dist_ਵਸਤੂ{$2, $1, $3}; $$->is_ਸਾਰਾ(true); }
        //                      // IEEE 1800-2023 added:
        |       yDEFAULT yP_COLONDIV ਵਿਅੰਜਕ
                        { BBUNSUP($<fl>2, "Unsupported: 'default :/' constraint");
                          $$ = nullptr; DEL($3); }
        ;

extern_constraint_declaration<constraintp>:  // ==IEEE: extern_constraint_declaration
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE packageClassScopeE constraintIdNew constraint_ਖੰਡ
                        { $$ = $5; $$->isStatic($1); $$->isExternDef(true);
                          $$->ਅਧਾਰ_Override($3); $$->classOrPackagep($4); $$->ਜੋੜੋ_Itemsp($6); }
        ;

constraintStaticE<cbool>:  // IEEE: part of extern_constraint_declaration
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = false; }
        |       ySTATIC__CONSTRAINT                     { $$ = true; }
        ;

//**********************************************************************
// Constants

ਸਮਾਂ_ਅੰਕ_Adjusted<ਗੰਢ_Exprp>:         // Time constant, adjusted to module's time units/precision
                yaTIMENUM
                        { $$ = new Ast_ਸਮਾਂ_Import{$<fl>1, new AstConst{$<fl>1, AstConst::RealDouble{}, $1}}; }
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
        /*cont*/    configParameterListE design_statement config_ਨਿਯਮ_statement_List_E
        /*cont*/    yENDCONFIG ਸਿਰਾ_Label_E
                { AstConfig* ਅਚਲ newp = new AstConfig{$1, PARSEP->libname(), *$2};
                  newp->ਜੋੜੋ_Designp($5);
                  newp->ਜੋੜੋ_Itemsp($4);
                  newp->ਜੋੜੋ_Itemsp($6);
                  GRAMMARP->ਸਿਰਾ_Label($<fl>7, *$2, $8);
                  PARSEP->rootp()->ਜੋੜੋ_Miscsp(newp); }
        ;

configParameterListE<nodep>:  // IEEE: { local_parameter_declaration ';' }
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       configParameterList                     { $$ = $1; }
        ;

configParameterList<nodep>:  // IEEE: part of config_declaration
                configParameter                         { $$ = nullptr; DEL($1); }
        |       configParameterList configParameter     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
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
        |       configCellList configCell               { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

configCell<configCellp>:  // IEEE: part of design_statement, part of cell_clause
                idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, PARSEP->libname(), *$1}; }
        |       idAny/*library_identifier*/ '.' idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, *$1, *$3}; }
        ;

config_ਨਿਯਮ_statement_List_E<nodep>:  // IEEE: { config_rule }
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        |       config_ਨਿਯਮ_statement_List               { $$ = $1; }
        ;

config_ਨਿਯਮ_statement_List<nodep>:  // IEEE: { config_rule }
                config_ਨਿਯਮ_statement                   { $$ = $1; }
        |       config_ਨਿਯਮ_statement_List config_ਨਿਯਮ_statement   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
        ;

config_ਨਿਯਮ_statement<nodep>:  // == IEEE: config_rule_statement
        //                      // IEEE: default_clause
                yDEFAULT liblist_clause ';'
                        { $$ = new Ast_Config_ਨਿਯਮ{$1, nullptr, $2, false}; }
        //                      // IEEE: inst_clause
        |       yINSTANCE inst_name liblist_clause ';'
                        { $$ = new Ast_Config_ਨਿਯਮ{$1, $2, $3, false}; }
        |       yINSTANCE inst_name use_clause ';'
                        { $$ = new Ast_Config_ਨਿਯਮ{$1, $2, $3, false}; }
        //                      // IEEE: cell_clause
        |       yCELL configCell liblist_clause ';'
                        { $$ = new Ast_Config_ਨਿਯਮ{$1, $2, $3, true}; }
        |       yCELL configCell use_clause ';'
                        { $$ = new Ast_Config_ਨਿਯਮ{$1, $2, $3, true}; }
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

inst_name<ਗੰਢ_Exprp>:  // == IEEE: inst_name
                idAnyAsParseRef/*topmodule_identifier*/
                        { $$ = $1; }
        |       idAnyAsParseRef/*topmodule_identifier*/ inst_nameInstanceList
                        { $$ = new AstDot{$<fl>1, false, $1, $2}; }
        ;

inst_nameInstanceList<ਗੰਢ_Exprp>:  // IEEE: part of inst_name
                '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = $2; }
        |       inst_nameInstanceList '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        ;

liblist_clause<nodep>:  // == IEEE: liblist_clause
                yLIBLIST                                { $$ = nullptr; }
        |       yLIBLIST liblistLibraryList             { $$ = $2; }
        ;

liblistLibraryList<ਗੰਢ_Exprp>:  // IEEE: part of liblist_clause
                idAnyAsParseRef/*library_identifier*/
                        { $$ = $1; }
        |       liblistLibraryList idAnyAsParseRef/*library_identifier*/
                        { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $2); }
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
        //                      // IEEE is missing the '#' '(', but examples need it
        |       '#' '(' ')'                             { $$ = nullptr; }
        |       '#' '(' useAssignmentList ')'           { $$ = $3; }
        ;

useAssignmentList<pinp>:  // IEEE: part of use_clause
                useAssignment                           { $$ = $1; }
        |       useAssignmentList ',' useAssignment     { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;

useAssignment<pinp>:  // IEEE: part of use_clause
        //                      // IEEE: named_parameter_assignment
                '.' idAny/*parameter_identifier*/ '(' ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); }
        |       '.' idAny/*parameter_identifier*/ '(' ਵਿਅੰਜਕ_Or_ਅੰਕੜੇ_Type ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); DEL($4); }
        ;

colonConfigE<cbool>:  // IEEE: [ ':' yCONFIG]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = false; }
        |       ':' yCONFIG                             { $$ = true; }
        ;

//**********************************************************************
// Config - lib.map
//

library_declaration<nodep>:  // IEEE: library_declaration
                yLIBRARY yaSTRING file_ਰਸਤਾ_spec_List incdirE ';'
                        { $$ = new AstLibrary{$<fl>1, *$2, $3, $4}; }
        ;
incdirE<ਗੰਢ_Exprp>:  // IEEE: [ '-' yINCDIR file_path_specList ';']
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { $$ = nullptr; }
                                // https://accellera.mantishub.io/view.php?id=1166
        |       yINCDIR file_ਰਸਤਾ_spec_List              { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config incdir"); }
        ;
include_statement<nodep>:  // IEEE: include_statement
                yINCLUDE file_ਰਸਤਾ_spec ';'             { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config include"); }
        ;
file_ਰਸਤਾ_spec_List<ਗੰਢ_Exprp>:  // IEEE: file_path_spec { ',' file_path_spec }
                file_ਰਸਤਾ_spec                          { $$ = $1; }
        |       file_ਰਸਤਾ_spec_List ',' file_ਰਸਤਾ_spec   { $$ = ਜੋੜੋ_ਅਗਲਾ_Null($1, $3); }
        ;
file_ਰਸਤਾ_spec<ਗੰਢ_Exprp>:  // IEEE: file_path_spec
                yaSTRING { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

//**********************************************************************
// VLT Files

vlt_ਵਸਤੂ:
        //                      // TODO support arbitrary order of arguments
                vltOffFront
                        { V3Control::ਜੋੜੋ_Ignore($1, false, "*", 0, 0); }
        |       vltOffFront vltDFile
                        { V3Control::ਜੋੜੋ_Ignore($1, false, *$2, 0, 0); }
        |       vltOffFront vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM
                        { V3Control::ਜੋੜੋ_Ignore($1, false, *$2, $4->ਮੰਜ਼ਿਲ_UInt(), $4->ਮੰਜ਼ਿਲ_UInt()); }
        |       vltOffFront vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM '-' yaINTNUM
                        { V3Control::ਜੋੜੋ_Ignore($1, false, *$2, $4->ਮੰਜ਼ਿਲ_UInt(), $6->ਮੰਜ਼ਿਲ_UInt()); }
        |       vltOffFront vltDFile vltDMatch
                        { ਜੇ (($1 == V3_Error_ਕੋਡ::I_COVERAGE) || ($1 == V3_Error_ਕੋਡ::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Ignore_ਮੇਲ($1, *$2, "", *$3);
                          }}
        |       vltOffFront vltDFile vltDContents
                        { ਜੇ (($1 == V3_Error_ਕੋਡ::I_COVERAGE) || ($1 == V3_Error_ਕੋਡ::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Ignore_ਮੇਲ($1, *$2, *$3, "*");
                          }}
        |       vltOffFront vltDFile vltDContents vltDMatch
                        { ਜੇ (($1 == V3_Error_ਕੋਡ::I_COVERAGE) || ($1 == V3_Error_ਕੋਡ::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Ignore_ਮੇਲ($1, *$2, *$3, *$4);
                          }}
        |       vltOffFront vltDScope
                        { ਜੇ ($1 != V3_Error_ਕੋਡ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Scope_Trace_On(false, *$2, 0);
                          }}
        |       vltOffFront vltDScope vltDLevels
                        { ਜੇ ($1 != V3_Error_ਕੋਡ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Scope_Trace_On(false, *$2, $3->ਮੰਜ਼ਿਲ_UInt());
                          }}
        |       vltOnFront
                        { V3Control::ਜੋੜੋ_Ignore($1, true, "*", 0, 0); }
        |       vltOnFront vltDFile
                        { V3Control::ਜੋੜੋ_Ignore($1, true, *$2, 0, 0); }
        |       vltOnFront vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM
                        { V3Control::ਜੋੜੋ_Ignore($1, true, *$2, $4->ਮੰਜ਼ਿਲ_UInt(), $4->ਮੰਜ਼ਿਲ_UInt()); }
        |       vltOnFront vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM '-' yaINTNUM
                        { V3Control::ਜੋੜੋ_Ignore($1, true, *$2, $4->ਮੰਜ਼ਿਲ_UInt(), $6->ਮੰਜ਼ਿਲ_UInt()); }
        |       vltOnFront vltDScope
                        { ਜੇ ($1 != V3_Error_ਕੋਡ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Scope_Trace_On(true, *$2, 0);
                          }}
        |       vltOnFront vltDScope vltDLevels
                        { ਜੇ ($1 != V3_Error_ਕੋਡ::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } ਨਹੀਂਤਾਂ {
                              V3Control::ਜੋੜੋ_Scope_Trace_On(true, *$2, $3->ਮੰਜ਼ਿਲ_UInt());
                          }}
        |       vltVarAttrFront vltDModuleE vltDFTaskE vltVarAttrSpecE attr_event_controlE
                        { V3Control::ਜੋੜੋ_Var_Attr($<fl>1, *$2, *$3, GRAMMARP->m_vlt_Var_Spec_ਕਿਸਮ, *$4, $1, $5); }
        |       vltVarAttrFrontDeprecated vltDModuleE vltDFTaskE vltVarAttrSpecE
                        { /* Historical, now has no effect */ }
        |       vltInlineFront vltDModuleE vltDFTaskE
                        { V3Control::ਜੋੜੋ_Inline($<fl>1, *$2, *$3, $1); }
        |       y_VLT_COVERAGE_ਖੰਡ_OFF vltDFile
                        { V3Control::ਜੋੜੋ_Coverage_ਖੰਡ_Off(*$2, 0); }
        |       y_VLT_COVERAGE_ਖੰਡ_OFF vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM
                        { V3Control::ਜੋੜੋ_Coverage_ਖੰਡ_Off(*$2, $4->ਮੰਜ਼ਿਲ_UInt()); }
        |       y_VLT_COVERAGE_ਖੰਡ_OFF vltDModule vltDBlock
                        { V3Control::ਜੋੜੋ_Coverage_ਖੰਡ_Off(*$2, *$3); }
        |       y_VLT_ਭਰਿਆ_CASE vltDFile
                        { V3Control::ਜੋੜੋ_Case_ਭਰਿਆ(*$2, 0); }
        |       y_VLT_ਭਰਿਆ_CASE vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM
                        { V3Control::ਜੋੜੋ_Case_ਭਰਿਆ(*$2, $4->ਮੰਜ਼ਿਲ_UInt()); }
        |       y_VLT_HIER_ਖੰਡ vltDModuleE
                        { V3Control::ਜੋੜੋ_Module_Pragma(*$2, VPragmaType::HIER_ਖੰਡ); }
        |       yVLT_HIER_PARAMS vltDModuleE
                        { V3Control::ਜੋੜੋ_Module_Pragma(*$2, VPragmaType::HIER_PARAMS); }
        |       yVLT_HIER_WORKERS vltDModuleE vltDWorkers
                        { V3Control::ਜੋੜੋ_Hier_Workers($<fl>1, *$2, $3->ਮੰਜ਼ਿਲ_SInt()); }
        |       yVLT_HIER_WORKERS vltDHierDpi vltDWorkers
                        { V3Control::ਜੋੜੋ_Hier_Workers($<fl>1, *$2, $3->ਮੰਜ਼ਿਲ_SInt()); }
        |       yVLT_PARALLEL_CASE vltDFile
                        { V3Control::ਜੋੜੋ_Case_Parallel(*$2, 0); }
        |       yVLT_PARALLEL_CASE vltDFile y_VLT_D_ਸਤਰਾਂ yaINTNUM
                        { V3Control::ਜੋੜੋ_Case_Parallel(*$2, $4->ਮੰਜ਼ਿਲ_UInt()); }
        |       y_VLT_PROFILE_ਅੰਕੜੇ vltDHierDpi vltDCost
                        { V3Control::ਜੋੜੋ_Profile_ਅੰਕੜੇ($<fl>1, *$2, $3->ਮੰਜ਼ਿਲ_UQuad()); }
        |       y_VLT_PROFILE_ਅੰਕੜੇ vltDModel vltDMtask vltDCost
                        { V3Control::ਜੋੜੋ_Profile_ਅੰਕੜੇ($<fl>1, *$2, *$3, $4->ਮੰਜ਼ਿਲ_UQuad()); }
        |       y_VLT_FSM_ਰਜਿਸਟਰ_WRAPPER vltDModule vltDFsmD vltDFsmQ vltDFsmClock vltDFsmResetE vlt_DFsm_Reset_ਮੁੱਲ_E
                        { V3Control::ਜੋੜੋ_Fsm_ਰਜਿਸਟਰ_Wrapper($<fl>1, *$2, *$3, *$4, *$5, *$6, *$7); }
        |       yVLT_VERILATOR_LIB vltDModule
                        { V3Control::ਜੋੜੋ_Module_Pragma(*$2, VPragmaType::VERILATOR_LIB); }
        ;

vltOffFront<errcodeen>:
                yVLT_COVERAGE_OFF                       { $$ = V3_Error_ਕੋਡ::I_COVERAGE; }
        |       yVLT_TIMING_OFF                         { $$ = V3_Error_ਕੋਡ::I_TIMING; }
        |       yVLT_TRACING_OFF                        { $$ = V3_Error_ਕੋਡ::I_TRACING; }
        |       yVLT_LINT_OFF                           { $$ = V3_Error_ਕੋਡ::I_LINT; }
        |       yVLT_LINT_OFF y_VLT_D_ਨਿਯਮ idAny
                        { $$ = V3_Error_ਕੋਡ{*$3};
                          ਜੇ ($$ == V3_Error_ਕੋਡ::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltOnFront<errcodeen>:
                yVLT_COVERAGE_ON                        { $$ = V3_Error_ਕੋਡ::I_COVERAGE; }
        |       yVLT_TIMING_ON                          { $$ = V3_Error_ਕੋਡ::I_TIMING; }
        |       yVLT_TRACING_ON                         { $$ = V3_Error_ਕੋਡ::I_TRACING; }
        |       yVLT_LINT_ON                            { $$ = V3_Error_ਕੋਡ::I_LINT; }
        |       yVLT_LINT_ON y_VLT_D_ਨਿਯਮ idAny
                        { $$ = V3_Error_ਕੋਡ{*$3};
                          ਜੇ ($$ == V3_Error_ਕੋਡ::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltDBlock<strp>:  // --block <arg>
                y_VLT_D_ਖੰਡ str                        { $$ = $2; }
        ;

vltDContents<strp>:
                yVLT_D_CONTENTS str                     { $$ = $2; }
        ;

vltDCost<nump>:  // --cost <arg>
                y_VLT_D_ਲਾਗਤ yaINTNUM                    { $$ = $2; }
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { ਸਥਿਰ string ਖਾਲੀ_ਥਾਵਾਂ; $$ = &ਖਾਲੀ_ਥਾਵਾਂ; }
        |       yVLT_D_RESET str                        { $$ = $2; }
        ;

vlt_DFsm_Reset_ਮੁੱਲ_E<strp>:  // [--reset_value <arg>]
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { ਸਥਿਰ string ਖਾਲੀ_ਥਾਵਾਂ; $$ = &ਖਾਲੀ_ਥਾਵਾਂ; }
        |       y_VLT_D_RESET_ਮੁੱਲ str                  { $$ = $2; }
        ;

vltDHierDpi<strp>:  // --hier-dpi <arg>
                yVLT_D_HIER_DPI str                     { $$ = $2; }
        ;

vltDLevels<nump>:  // --levels <arg>
                y_VLT_D_ਪੱਧਰ yaINTNUM                  { $$ = $2; }
        ;

vltDMatch<strp>:  // --match <arg>
                y_VLT_D_ਮੇਲ str                        { $$ = $2; }
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { ਸਥਿਰ string unit = "$unit"; $$ = &unit; }  // .vlt uses prettyName
        |       vltDModule
                        { $$ = $1; }
        ;

vltDScope<strp>:  // --scope <arg>
                yVLT_D_SCOPE str                        { $$ = $2; }
        ;

vltDFTaskE<strp>:
                /*ਖਾਲੀ_ਥਾਵਾਂ*/                             { ਸਥਿਰ string ਖਾਲੀ_ਥਾਵਾਂ; $$ = &ਖਾਲੀ_ਥਾਵਾਂ; }
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
                /*ਖਾਲੀ_ਥਾਵਾਂ*/
                        { GRAMMARP->m_vlt_Var_Spec_ਕਿਸਮ = V3Control::Var_Spec_ਕਿਸਮ::VAR; ਸਥਿਰ std::string ਖਾਲੀ_ਥਾਵਾਂ; $$ = &ਖਾਲੀ_ਥਾਵਾਂ; }
        |       yVLT_D_PARAM str
                        { GRAMMARP->m_vlt_Var_Spec_ਕਿਸਮ = V3Control::Var_Spec_ਕਿਸਮ::PARAM; $$ = $2; }
        |       y_VLT_D_ਪੋਰਟ str
                        { GRAMMARP->m_vlt_Var_Spec_ਕਿਸਮ = V3Control::Var_Spec_ਕਿਸਮ::ਪੋਰਟ; $$ = $2; }
        |       yVLT_D_VAR str
                        { GRAMMARP->m_vlt_Var_Spec_ਕਿਸਮ = V3Control::Var_Spec_ਕਿਸਮ::VAR; $$ = $2; }
        ;

vltVarAttrFront<attrtypeen>:
                yVLT_FORCEABLE              { $$ = VAttrType::VAR_FORCEABLE; }
        |       y_VLT_ਜਨਤਕ                 { $$ = VAttrType::VAR_ਜਨਤਕ; v3Global.dpi(true); }
        |       y_VLT_ਜਨਤਕ_FLAT            { $$ = VAttrType::VAR_ਜਨਤਕ_FLAT; v3Global.dpi(true); }
        |       y_VLT_ਜਨਤਕ_FLAT_RD         { $$ = VAttrType::VAR_ਜਨਤਕ_FLAT_RD; v3Global.dpi(true); }
        |       y_VLT_ਜਨਤਕ_FLAT_RW         { $$ = VAttrType::VAR_ਜਨਤਕ_FLAT_RW; v3Global.dpi(true); }
        |       yVLT_SC_BIGUINT             { $$ = VAttrType::VAR_SC_BIGUINT; }
        |       yVLT_SC_BV                  { $$ = VAttrType::VAR_SC_BV; }
        |       yVLT_SFORMAT                { $$ = VAttrType::VAR_SFORMAT; }
        |       y_VLT_ਵੰਡੋ_VAR              { $$ = VAttrType::VAR_ਵੰਡੋ_VAR; }
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
