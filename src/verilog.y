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
#यदि_परिभाषितम् NEVER_JUST_FOR_CLANG_FORMAT
 }
#यदि_अन्तः
// clang-format on
#अन्तर्भाव "V3ParseGrammar.h"  // Defines YYTYPE; before including bison header

#परिभाषय YYERROR_VERBOSE 1  // For prior to Bison 3.6
#परिभाषय YYINITDEPTH 10000  // Older bisons ignore YYMAXDEPTH
#परिभाषय YYMAXDEPTH 10000
#परिभाषय YYNEWLINE "\n"

// Pick up new lexer
#परिभाषय yylex PARSEP->token_गन्तव्यम्_Bison

#परिभाषय BBCOVERIGN(fl, सन्देशः_2) (fl)->v3warn(COVERIGN, सन्देशः_2)
#परिभाषय BBUNSUP(fl, सन्देशः_2) (fl)->v3warn(E_UNSUPPORTED, सन्देशः_2)
#परिभाषय GATEUNSUP(fl, tok) \
    { BBUNSUP((fl), "Unsupported: Verilog 1995 gate primitive: " << (tok)); }
#परिभाषय RISEFALLDLYUNSUP(nodep) \
    यदि (nodep->fileline()->timingOn() && v3Global.opt.timing().is_समुच्चयः_True()) { \
        nodep->v3warn(RISEFALLDLY, "Unsupported: turn-off delays. Ignoring the third delay"); \
    }
#परिभाषय MINTYPMAXDLYUNSUP(nodep) \
    यदि (nodep->fileline()->timingOn() && v3Global.opt.timing().is_समुच्चयः_True()) { \
        nodep->v3warn( \
            MINTYPMAXDLY, \
            "Unsupported: minimum/typical/maximum delay expressions. Using the typical delay"); \
    }
// Apply a delay to all continuous assignments under listp
स्थिर शून्य DELAY_LIST(Ast_ग्रन्थिः* listp, AstDelay* delayp) {
    यदि (!delayp) प्रत्यागम;
    प्रति (Ast_ग्रन्थिः* nodep = listp; nodep; nodep = nodep->nextp()) {
        यदि (VN_IS(nodep, Implicit)) अनुवर्त;
        AstAlways* ध्रुव alwaysp = VN_AS(nodep, Always);
        Ast_नियोजय_W* ध्रुव assignp = VN_AS(alwaysp->stmtsp(), नियोजय_W);
        assignp->timingControlp(delayp->backp() ? delayp->clone_वृक्षः(false) : delayp);
    }
}
// Apply a strength to all continuous assignments under listp
स्थिर शून्य STRENGTH_LIST(Ast_ग्रन्थिः* listp, AstStrengthSpec* specp) {
    यदि (!specp) प्रत्यागम;
    प्रति (Ast_ग्रन्थिः* nodep = listp; nodep; nodep = nodep->nextp()) {
        यदि (VN_IS(nodep, Implicit)) अनुवर्त;
        AstAlways* ध्रुव alwaysp = VN_AS(nodep, Always);
        Ast_नियोजय_W* ध्रुव assignp = VN_AS(alwaysp->stmtsp(), नियोजय_W);
        assignp->strengthSpecp(specp->backp() ? specp->clone_वृक्षः(false) : specp);
    }
}
//======================================================================
// Statics (for here only)

#परिभाषय PARSEP V3ParseImp::parsep()
#परिभाषय GRAMMARP V3ParseGrammar::singletonp()

ध्रुव VBasicDTypeKwd LOGIC = VBasicDTypeKwd::LOGIC;  // Shorthand "LOGIC"
ध्रुव VBasicDTypeKwd LOGIC_IMPLICIT = VBasicDTypeKwd::LOGIC_IMPLICIT;

//======================================================================
// Macro functions

// Only use in empty rules, so lines point at beginnings
#परिभाषय CRELINE() (PARSEP->bison_चरमः_Fileline()->copy_Or_Same_File_पङ्क्तिः_Applied())
#परिभाषय FILELINE_OR_CRE(nodep) ((nodep) ? (nodep)->fileline() : CRELINE())

#परिभाषय VARRESET_LIST(decl) VARRESET__PVT(decl, 1)  // Start of pinlist
#परिभाषय VARRESET_NONLIST(decl) VARRESET__PVT(decl, 0);  // Not in a pinlist
#परिभाषय VARRESET__PVT(decl, pin_अङ्कः_Start) \
    { \
        VARDECL(decl); \
        VARIO(NONE); \
        VARDTYPE_NDECL(nullptr); \
        GRAMMARP->m_pin_अङ्कः = (pin_अङ्कः_Start); \
        GRAMMARP->m_varLifetime = VLifetime::NONE; \
        GRAMMARP->m_varDeclTyped = false; \
    }
#परिभाषय VARDECL(प्रकार) \
    { GRAMMARP->m_varDecl = VVarType::प्रकार; }
#परिभाषय VARIO(प्रकार) \
    { GRAMMARP->m_varIO = VDirection::प्रकार; }
// Set direction to default-input when detect inside an ANSI port list
#परिभाषय VARIOANSI() \
    { \
        यदि (GRAMMARP->m_varIO == VDirection::NONE) VARIO(निवेशः); \
    }
#परिभाषय VARLIFE(flag) \
    { GRAMMARP->m_varLifetime = flag; }
#परिभाषय VARDTYPE(dtypep) \
    { \
        GRAMMARP->समुच्चयः_DType(dtypep); \
        GRAMMARP->m_varDeclTyped = true; \
    }
#परिभाषय VARDTYPE_NDECL(dtypep) \
    { GRAMMARP->समुच्चयः_DType(dtypep); }  // Port that is range or signed only (not a decl)

#परिभाषय VARDONEA(fl, name, array, attrs) GRAMMARP->createVariable((fl), (name), (array), (attrs))
#परिभाषय VARDONEP(portp, array, attrs) \
    GRAMMARP->createVariable((portp)->fileline(), (portp)->name(), (array), (attrs))
#परिभाषय PINNUMINC() (GRAMMARP->m_pin_अङ्कः++)

#परिभाषय GATERANGE(rangep) \
    { GRAMMARP->m_gateRangep = rangep; }

#परिभाषय INSTPREP(modfl, modname, paramsp) \
    { \
        GRAMMARP->m_impliedDecl = true; \
        GRAMMARP->m_instModuleFl = modfl; \
        GRAMMARP->m_instModule = modname; \
        GRAMMARP->m_instParamp = paramsp; \
    }

#परिभाषय DEL(...) \
    { \
        /* cppcheck-suppress constVariable */ \
        Ast_ग्रन्थिः* ध्रुव nodeps[] = {__VA_ARGS__}; \
        प्रति (Ast_ग्रन्थिः* ध्रुव nodep : nodeps) \
            यदि (nodep) nodep->delete_वृक्षः(); \
    }

स्थिर शून्य ERRSVKWD(File_पङ्क्तिः* fileline, ध्रुव string& tokname) {
    स्थिर पूर्णाङ्क s_toldonce = 0;
    fileline->v3error(
        "Unexpected '"s + tokname + "': '" + tokname
        + "' is a SystemVerilog keyword misused as an identifier."
        + (!s_toldonce++ ? "\n" + fileline->warn_अधिकम्()
                               + "... Suggest modify the Verilog-2001 code to avoid SV keywords,"
                               + " or use `begin_keywords or --language."
                         : ""));
}

स्थिर शून्य ASSIGNEQEXPR(File_पङ्क्तिः* fileline) {
    fileline->v3warn(ASSIGNEQEXPR,
                     "Assignment '=' inside expression\n"
                         << fileline->warn_अधिकम्()
                         << "... Was a '==' intended, or suggest use a separate statement");
}

स्थिर शून्य UNSUPREAL(File_पङ्क्तिः* fileline) {
    fileline->v3warn(SHORTREAL,
                     "Unsupported: shortreal being promoted to real (suggest use real instead)");
}

//======================================================================

शून्य yyerror(ध्रुव वर्ण* errmsg) { PARSEP->bison_चरमः_Fileline()->v3error(errmsg); }

प्रतिरूपम् <typename T_ग्रन्थिः, typename T_अग्र>
स्थिर T_ग्रन्थिः* सङ्कलय_अग्र_Null(T_ग्रन्थिः* nodep, T_अग्र* nextp) {
    प्रत्यागम Ast_ग्रन्थिः::सङ्कलय_अग्र_Null<T_ग्रन्थिः, T_अग्र>(nodep, nextp);
}

//======================================================================

class Ast_Sen_वृक्षः;
// clang-format off
%}

// Bison 3.0 and newer
BISONPRE_संस्करणम्(3.0,%define parse.error verbose)

// We run bison with the -d argument. This tells it to generate a
// header file with token names. Old versions of bison pasted the
// contents of that file into the generated source as well; newer
// versions just include it.
//
// Since we run bison through ../bisonpre, it doesn't know the correct
// header file name, so we need to tell it.
BISONPRE_संस्करणम्(3.7,%define api.शीर्षकम्.include {"V3ParseBison.h"})

// When writing Bison patterns we use yTOKEN instead of "token",
// so Bison will error out on unknown "token"s.

// Generic lexer tokens, for example a number
// IEEE: real_number
%शब्दांशः<cdouble>         yaFLOATNUM      "FLOATING-POINT NUMBER"

// IEEE: identifier, class_identifier, class_variable_identifier,
// covergroup_variable_identifier, dynamic_array_variable_identifier,
// enum_identifier, interface_identifier, interface_instance_identifier,
// package_identifier, type_identifier, variable_identifier,
%शब्दांशः<strp>            yaID__ETC       "IDENTIFIER"
%शब्दांशः<strp>            yaID__CC        "IDENTIFIER-::"
%शब्दांशः<strp>            yaID__LEX       "IDENTIFIER-in-lex"
%शब्दांशः<strp>            yaID__PATHPULSE "IDENTIFIER-for-pathpulse"
%शब्दांशः<strp>            yaID__aINST     "IDENTIFIER-for-instance"
%शब्दांशः<strp>            yaID__aTYPE     "IDENTIFIER-for-type"
//                      Can't predecode aFUNCTION, can declare after use
//                      Can't predecode aINTERFACE, can declare after use
//                      Can't predecode aTASK, can declare after use

// IEEE: integral_number
%शब्दांशः<nump>            yaINTNUM        "INTEGER NUMBER"
// IEEE: time_literal + time_unit
%शब्दांशः<cdouble>         yaTIMENUM       "TIME NUMBER"
// IEEE: string_literal
%शब्दांशः<strp>            yaSTRING        "STRING"
%शब्दांशः<strp>            yaSTRING__IGNORE "STRING-ignored"       // Used when expr:string not allowed
// IEEE: edge_descriptor
%शब्दांशः<nump>            yaEDGEDESC      "EDGE DESCRIPTOR"

%शब्दांशः<fl>              yaTIMINGSPEC    "TIMING SPEC ELEMENT"

%शब्दांशः<fl>              ygenSTRENGTH    "STRENGTH keyword (strong1/etc)"

%शब्दांशः<strp>            ya_सारणी_क्षेत्रम्   "UDP table field"
%शब्दांशः<fl>              ya_सारणी_LRSEP   ":"
%शब्दांशः<fl>              ya_सारणी_LINEEND "UDP table line end"

%शब्दांशः<strp>            yaSCCTOR        "`systemc_ctor block"
%शब्दांशः<strp>            yaSCDTOR        "`systemc_dtor block"
%शब्दांशः<strp>            yaSCHDR         "`systemc_header block"
%शब्दांशः<strp>            yaSCHDRP        "`systemc_header_post block"
%शब्दांशः<strp>            yaSCIMP         "`systemc_implementation block"
%शब्दांशः<strp>            yaSCIMPH        "`systemc_imp_header block"
%शब्दांशः<strp>            yaSCINT         "`systemc_interface block"

%शब्दांशः<fl>              yVLT_CLOCKER                "clocker"
%शब्दांशः<fl>              yVLT_CLOCK_ENABLE           "clock_enable"
%शब्दांशः<fl>              y_VLT_COVERAGE_खण्डम्_OFF     "coverage_block_off"
%शब्दांशः<fl>              yVLT_COVERAGE_OFF           "coverage_off"
%शब्दांशः<fl>              yVLT_COVERAGE_ON            "coverage_on"
%शब्दांशः<fl>              yVLT_FORCEABLE              "forceable"
%शब्दांशः<fl>              y_VLT_FSM_पञ्जिकाकोशः_WRAPPER   "fsm_register_wrapper"
%शब्दांशः<fl>              y_VLT_पूर्णम्_CASE              "full_case"
%शब्दांशः<fl>              y_VLT_HIER_खण्डम्             "hier_block"
%शब्दांशः<fl>              yVLT_HIER_PARAMS            "hier_params"
%शब्दांशः<fl>              yVLT_HIER_WORKERS           "hier_workers"
%शब्दांशः<fl>              yVLT_INLINE                 "inline"
%शब्दांशः<fl>              yVLT_ISOLATE_ASSIGNMENTS    "isolate_assignments"
%शब्दांशः<fl>              yVLT_LINT_OFF               "lint_off"
%शब्दांशः<fl>              yVLT_LINT_ON                "lint_on"
%शब्दांशः<fl>              yVLT_NO_CLOCKER             "no_clocker"
%शब्दांशः<fl>              yVLT_NO_INLINE              "no_inline"
%शब्दांशः<fl>              yVLT_PARALLEL_CASE          "parallel_case"
%शब्दांशः<fl>              y_VLT_PROFILE_दत्त           "profile_data"
%शब्दांशः<fl>              y_VLT_लौकिकम्                 "public"
%शब्दांशः<fl>              y_VLT_लौकिकम्_FLAT            "public_flat"
%शब्दांशः<fl>              y_VLT_लौकिकम्_FLAT_RD         "public_flat_rd"
%शब्दांशः<fl>              y_VLT_लौकिकम्_FLAT_RW         "public_flat_rw"
%शब्दांशः<fl>              y_VLT_लौकिकम्_MODULE          "public_module"
%शब्दांशः<fl>              yVLT_SC_BIGUINT             "sc_biguint"
%शब्दांशः<fl>              yVLT_SC_BV                  "sc_bv"
%शब्दांशः<fl>              yVLT_SFORMAT                "sformat"
%शब्दांशः<fl>              y_VLT_विभज_VAR              "split_var"
%शब्दांशः<fl>              yVLT_TIMING_OFF             "timing_off"
%शब्दांशः<fl>              yVLT_TIMING_ON              "timing_on"
%शब्दांशः<fl>              yVLT_TRACING_OFF            "tracing_off"
%शब्दांशः<fl>              yVLT_TRACING_ON             "tracing_on"
%शब्दांशः<fl>              yVLT_VERILATOR_LIB          "verilator_lib"

%शब्दांशः<fl>              y_VLT_D_खण्डम्    "--block"
%शब्दांशः<fl>              yVLT_D_CONTENTS "--contents"
%शब्दांशः<fl>              yVLT_D_COST     "--cost"
%शब्दांशः<fl>              yVLT_D_CLOCK    "--clock"
%शब्दांशः<fl>              yVLT_D_D        "--d"
%शब्दांशः<fl>              yVLT_D_FILE     "--file"
%शब्दांशः<fl>              yVLT_D_FUNCTION "--function"
%शब्दांशः<fl>              yVLT_D_HIER_DPI "--hier-dpi"
%शब्दांशः<fl>              y_VLT_D_स्तराः   "--levels"
%शब्दांशः<fl>              y_VLT_D_पङ्क्तयः    "--lines"
%शब्दांशः<fl>              y_VLT_D_मेलनम्    "--match"
%शब्दांशः<fl>              yVLT_D_MODEL    "--model"
%शब्दांशः<fl>              yVLT_D_MODULE   "--module"
%शब्दांशः<fl>              yVLT_D_MTASK    "--mtask"
%शब्दांशः<fl>              yVLT_D_PARAM    "--param"
%शब्दांशः<fl>              y_VLT_D_पत्तनम्     "--port"
%शब्दांशः<fl>              y_VLT_D_नियमः     "--rule"
%शब्दांशः<fl>              yVLT_D_Q        "--q"
%शब्दांशः<fl>              yVLT_D_RESET    "--reset"
%शब्दांशः<fl>              y_VLT_D_RESET_मानम् "--reset_value"
%शब्दांशः<fl>              yVLT_D_SCOPE    "--scope"
%शब्दांशः<fl>              yVLT_D_TASK     "--task"
%शब्दांशः<fl>              yVLT_D_VAR      "--var"
%शब्दांशः<fl>              yVLT_D_WORKERS  "--workers"

%शब्दांशः<strp>            yaD_PLI         "${pli-system}"

%शब्दांशः<fl>              yaT_NOUNCONNECTED  "`nounconnecteddrive"
%शब्दांशः<fl>              yaT_RESETALL    "`resetall"
%शब्दांशः<fl>              yaT_UNCONNECTED_PULL0  "`unconnected_drive pull0"
%शब्दांशः<fl>              yaT_UNCONNECTED_PULL1  "`unconnected_drive pull1"

// <fl> is the fileline, abbreviated to shorten "$<fl>1" references
%शब्दांशः<fl>              '!'
%शब्दांशः<fl>              '#'
%शब्दांशः<fl>              '%'
%शब्दांशः<fl>              '&'
%शब्दांशः<fl>              '('  // See also yP_PAR__STRENGTH
%शब्दांशः<fl>              ')'
%शब्दांशः<fl>              '*'
%शब्दांशः<fl>              '+'
%शब्दांशः<fl>              ','
%शब्दांशः<fl>              '-'
%शब्दांशः<fl>              '.'
%शब्दांशः<fl>              '/'
%शब्दांशः<fl>              ':'  // See also yP_COLON__BEGIN or yP_COLON__FORK
%शब्दांशः<fl>              ';'
%शब्दांशः<fl>              '<'
%शब्दांशः<fl>              '='  // See also yP_EQ__NEW
%शब्दांशः<fl>              '>'
%शब्दांशः<fl>              '?'
%शब्दांशः<fl>              '@'
%शब्दांशः<fl>              '['
%शब्दांशः<fl>              ']'
%शब्दांशः<fl>              '^'
%शब्दांशः<fl>              '{'
%शब्दांशः<fl>              '|'
%शब्दांशः<fl>              '}'
%शब्दांशः<fl>              '~'

// Specific keywords
// yKEYWORD means match "keyword"
// Other cases are yXX_KEYWORD where XX makes it unique,
// for example yP_ for punctuation based operators.
// Double underscores "yX__Y" means token X followed by Y,
// and "yX__ETC" means X folled by everything but Y(s).
%शब्दांशः<fl>              y1_चरणः          "1step"
%शब्दांशः<fl>              yACCEPT_ON      "accept_on"
%शब्दांशः<fl>              yALIAS          "alias"
%शब्दांशः<fl>              yALWAYS         "always"
%शब्दांशः<fl>              yALWAYS_COMB    "always_comb"
%शब्दांशः<fl>              yALWAYS_FF      "always_ff"
%शब्दांशः<fl>              yALWAYS_LATCH   "always_latch"
%शब्दांशः<fl>              yAND            "and"
%शब्दांशः<fl>              yASSERT         "assert"
%शब्दांशः<fl>              y_नियोजय         "assign"
%शब्दांशः<fl>              yASSUME         "assume"
%शब्दांशः<fl>              yAUTOMATIC      "automatic"
%शब्दांशः<fl>              yBEFORE         "before"
%शब्दांशः<fl>              yBEGIN          "begin"
%शब्दांशः<fl>              yBIND           "bind"
%शब्दांशः<fl>              yBINS           "bins"
%शब्दांशः<fl>              yBINSOF         "binsof"
%शब्दांशः<fl>              yBIT            "bit"
%शब्दांशः<fl>              yBREAK          "break"
%शब्दांशः<fl>              yBUF            "buf"
%शब्दांशः<fl>              yBUFIF0         "bufif0"
%शब्दांशः<fl>              yBUFIF1         "bufif1"
%शब्दांशः<fl>              yBYTE           "byte"
%शब्दांशः<fl>              yCASE           "case"
%शब्दांशः<fl>              yCASEX          "casex"
%शब्दांशः<fl>              yCASEZ          "casez"
%शब्दांशः<fl>              yCELL           "cell"
%शब्दांशः<fl>              yCHANDLE        "chandle"
%शब्दांशः<fl>              yCHECKER        "checker"
%शब्दांशः<fl>              yCLASS          "class"
%शब्दांशः<fl>              yCLOCKING       "clocking"
%शब्दांशः<fl>              yCMOS           "cmos"
%शब्दांशः<fl>              yCONFIG         "config"
%शब्दांशः<fl>              yCONSTRAINT     "constraint"
%शब्दांशः<fl>              yCONST__ETC     "const"
%शब्दांशः<fl>              yCONST__LEX     "const-in-lex"
%शब्दांशः<fl>              yCONST__REF     "const-then-ref"
%शब्दांशः<fl>              yCONTEXT        "context"
%शब्दांशः<fl>              yCONTINUE       "continue"
%शब्दांशः<fl>              yCOVER          "cover"
%शब्दांशः<fl>              yCOVERGROUP     "covergroup"
%शब्दांशः<fl>              yCOVERPOINT     "coverpoint"
%शब्दांशः<fl>              yCROSS          "cross"
%शब्दांशः<fl>              yDEASSIGN       "deassign"
%शब्दांशः<fl>              yDEFAULT        "default"
%शब्दांशः<fl>              yDEFPARAM       "defparam"
%शब्दांशः<fl>              yDESIGN         "design"
%शब्दांशः<fl>              yDISABLE        "disable"
%शब्दांशः<fl>              yDIST           "dist"
%शब्दांशः<fl>              yDO             "do"
%शब्दांशः<fl>              yEDGE           "edge"
%शब्दांशः<fl>              yELSE           "else"
%शब्दांशः<fl>              y_अन्तः            "end"
%शब्दांशः<fl>              yENDCASE        "endcase"
%शब्दांशः<fl>              yENDCHECKER     "endchecker"
%शब्दांशः<fl>              yENDCLASS       "endclass"
%शब्दांशः<fl>              yENDCLOCKING    "endclocking"
%शब्दांशः<fl>              yENDCONFIG      "endconfig"
%शब्दांशः<fl>              yENDFUNCTION    "endfunction"
%शब्दांशः<fl>              yENDGENERATE    "endgenerate"
%शब्दांशः<fl>              yENDGROUP       "endgroup"
%शब्दांशः<fl>              yENDINTERFACE   "endinterface"
%शब्दांशः<fl>              yENDMODULE      "endmodule"
%शब्दांशः<fl>              yENDPACKAGE     "endpackage"
%शब्दांशः<fl>              yENDPRIMITIVE   "endprimitive"
%शब्दांशः<fl>              yENDPROGRAM     "endprogram"
%शब्दांशः<fl>              yENDPROPERTY    "endproperty"
%शब्दांशः<fl>              yENDSEQUENCE    "endsequence"
%शब्दांशः<fl>              yENDSPECIFY     "endspecify"
%शब्दांशः<fl>              yENDTABLE       "endtable"
%शब्दांशः<fl>              yENDTASK        "endtask"
%शब्दांशः<fl>              yENUM           "enum"
%शब्दांशः<fl>              yEVENT          "event"
%शब्दांशः<fl>              yEVENTUALLY     "eventually"
%शब्दांशः<fl>              yEXPECT         "expect"
%शब्दांशः<fl>              yEXPORT         "export"
%शब्दांशः<fl>              yEXTENDS        "extends"
%शब्दांशः<fl>              yEXTERN         "extern"
%शब्दांशः<fl>              yFINAL          "final"
%शब्दांशः<fl>              y_FIRST_मेलनम्    "first_match"
%शब्दांशः<fl>              yFOR            "for"
%शब्दांशः<fl>              yFORCE          "force"
%शब्दांशः<fl>              yFOREACH        "foreach"
%शब्दांशः<fl>              yFOREVER        "forever"
%शब्दांशः<fl>              yFORK           "fork"
%शब्दांशः<fl>              yFORKJOIN       "forkjoin"
%शब्दांशः<fl>              yFUNCTION       "function"
%शब्दांशः<fl>              yGENERATE       "generate"
%शब्दांशः<fl>              yGENVAR         "genvar"
%शब्दांशः<fl>              yGLOBAL__CLOCKING "global-then-clocking"
%शब्दांशः<fl>              yGLOBAL__ETC    "global"
%शब्दांशः<fl>              yGLOBAL__LEX    "global-in-lex"
%शब्दांशः<fl>              yHIGHZ0         "highz0"
%शब्दांशः<fl>              yHIGHZ1         "highz1"
%शब्दांशः<fl>              yIF             "if"
%शब्दांशः<fl>              yIFF            "iff"
%शब्दांशः<fl>              yIGNORE_BINS    "ignore_bins"
%शब्दांशः<fl>              yILLEGAL_BINS   "illegal_bins"
%शब्दांशः<fl>              yIMPLEMENTS     "implements"
%शब्दांशः<fl>              y_उपपादयति        "implies"
%शब्दांशः<fl>              yIMPORT         "import"
%शब्दांशः<fl>              yINCDIR         "incdir"
%शब्दांशः<fl>              yINCLUDE        "include"
%शब्दांशः<fl>              yINITIAL        "initial"
%शब्दांशः<fl>              yINOUT          "inout"
%शब्दांशः<fl>              y_निवेशः          "input"
%शब्दांशः<fl>              yINSIDE         "inside"
%शब्दांशः<fl>              yINSTANCE       "instance"
%शब्दांशः<fl>              yINT            "int"
%शब्दांशः<fl>              yINTEGER        "integer"
%शब्दांशः<fl>              yINTERCONNECT   "interconnect"
%शब्दांशः<fl>              yINTERFACE      "interface"
%शब्दांशः<fl>              yINTERSECT      "intersect"
%शब्दांशः<fl>              yJOIN           "join"
%शब्दांशः<fl>              yJOIN_ANY       "join_any"
%शब्दांशः<fl>              yJOIN_NONE      "join_none"
%शब्दांशः<fl>              yLET            "let"
%शब्दांशः<fl>              yLIBLIST        "liblist"
%शब्दांशः<fl>              yLIBRARY        "library"
%शब्दांशः<fl>              yLOCALPARAM     "localparam"
%शब्दांशः<fl>              yLOCAL__COLONCOLON "local-then-::"
%शब्दांशः<fl>              yLOCAL__ETC     "local"
%शब्दांशः<fl>              yLOCAL__LEX     "local-in-lex"
%शब्दांशः<fl>              yLOGIC          "logic"
%शब्दांशः<fl>              yLONGINT        "longint"
%शब्दांशः<fl>              yMATCHES        "matches"
%शब्दांशः<fl>              yMODPORT        "modport"
%शब्दांशः<fl>              yMODULE         "module"
%शब्दांशः<fl>              yNAND           "nand"
%शब्दांशः<fl>              yNEGEDGE        "negedge"
%शब्दांशः<fl>              yNETTYPE        "nettype"
%शब्दांशः<fl>              yNEW__ETC       "new"
%शब्दांशः<fl>              yNEW__LEX       "new-in-lex"
%शब्दांशः<fl>              yNEW__PAREN     "new-then-paren"
%शब्दांशः<fl>              yNEXTTIME       "nexttime"
%शब्दांशः<fl>              yNMOS           "nmos"
%शब्दांशः<fl>              yNOR            "nor"
%शब्दांशः<fl>              yNOT            "not"
%शब्दांशः<fl>              yNOTIF0         "notif0"
%शब्दांशः<fl>              yNOTIF1         "notif1"
%शब्दांशः<fl>              yNULL           "null"
%शब्दांशः<fl>              yOR             "or"
%शब्दांशः<fl>              yOUTPUT         "output"
%शब्दांशः<fl>              yPACKAGE        "package"
%शब्दांशः<fl>              yPACKED         "packed"
%शब्दांशः<fl>              yPARAMETER      "parameter"
%शब्दांशः<fl>              yPMOS           "pmos"
%शब्दांशः<fl>              yPOSEDGE        "posedge"
%शब्दांशः<fl>              yPRIMITIVE      "primitive"
%शब्दांशः<fl>              y_प्राथमिकता       "priority"
%शब्दांशः<fl>              y_क्रमादेशः        "program"
%शब्दांशः<fl>              yPROPERTY       "property"
%शब्दांशः<fl>              yPROTECTED      "protected"
%शब्दांशः<fl>              yPULL0          "pull0"
%शब्दांशः<fl>              yPULL1          "pull1"
%शब्दांशः<fl>              yPULLDOWN       "pulldown"
%शब्दांशः<fl>              yPULLUP         "pullup"
%शब्दांशः<fl>              y_विशुद्धम्           "pure"
%शब्दांशः<fl>              yRAND           "rand"
%शब्दांशः<fl>              yRANDC          "randc"
%शब्दांशः<fl>              yRANDCASE       "randcase"
%शब्दांशः<fl>              yRANDOMIZE      "randomize"
%शब्दांशः<fl>              yRANDSEQUENCE   "randsequence"
%शब्दांशः<fl>              yRCMOS          "rcmos"
%शब्दांशः<fl>              yREAL           "real"
%शब्दांशः<fl>              yREALTIME       "realtime"
%शब्दांशः<fl>              yREF            "ref"
%शब्दांशः<fl>              yREG            "reg"
%शब्दांशः<fl>              yREJECT_ON      "reject_on"
%शब्दांशः<fl>              yRELEASE        "release"
%शब्दांशः<fl>              yREPEAT         "repeat"
%शब्दांशः<fl>              yRESTRICT       "restrict"
%शब्दांशः<fl>              yRETURN         "return"
%शब्दांशः<fl>              yRNMOS          "rnmos"
%शब्दांशः<fl>              yRPMOS          "rpmos"
%शब्दांशः<fl>              yRTRAN          "rtran"
%शब्दांशः<fl>              yRTRANIF0       "rtranif0"
%शब्दांशः<fl>              yRTRANIF1       "rtranif1"
%शब्दांशः<fl>              ySCALARED       "scalared"
%शब्दांशः<fl>              ySEQUENCE       "sequence"
%शब्दांशः<fl>              ySHORTINT       "shortint"
%शब्दांशः<fl>              ySHORTREAL      "shortreal"
%शब्दांशः<fl>              ySIGNED         "signed"
%शब्दांशः<fl>              ySOFT           "soft"
%शब्दांशः<fl>              ySOLVE          "solve"
%शब्दांशः<fl>              ySPECIFY        "specify"
%शब्दांशः<fl>              ySPECPARAM      "specparam"
%शब्दांशः<fl>              ySTATIC__CONSTRAINT "static-then-constraint"
%शब्दांशः<fl>              ySTATIC__ETC    "static"
%शब्दांशः<fl>              ySTATIC__LEX    "static-in-lex"
%शब्दांशः<fl>              ySTRING         "string"
%शब्दांशः<fl>              ySTRONG         "strong"
%शब्दांशः<fl>              ySTRONG0        "strong0"
%शब्दांशः<fl>              ySTRONG1        "strong1"
%शब्दांशः<fl>              ySTRUCT         "struct"
%शब्दांशः<fl>              ySUPER          "super"
%शब्दांशः<fl>              ySUPPLY0        "supply0"
%शब्दांशः<fl>              ySUPPLY1        "supply1"
%शब्दांशः<fl>              ySYNC_ACCEPT_ON "sync_accept_on"
%शब्दांशः<fl>              ySYNC_REJECT_ON "sync_reject_on"
%शब्दांशः<fl>              yS_ALWAYS       "s_always"
%शब्दांशः<fl>              yS_EVENTUALLY   "s_eventually"
%शब्दांशः<fl>              yS_NEXTTIME     "s_nexttime"
%शब्दांशः<fl>              yS_UNTIL        "s_until"
%शब्दांशः<fl>              yS_UNTIL_WITH   "s_until_with"
%शब्दांशः<fl>              y_सारणी          "table"
%शब्दांशः<fl>              yTAGGED         "tagged"
%शब्दांशः<fl>              yTAGGED__LEX    "tagged-in-lex"
%शब्दांशः<fl>              yTAGGED__NONPRIMARY   "tagged-nonprimary"
%शब्दांशः<fl>              yTASK           "task"
%शब्दांशः<fl>              yTHIS           "this"
%शब्दांशः<fl>              yTHROUGHOUT     "throughout"
%शब्दांशः<fl>              yTIME           "time"
%शब्दांशः<fl>              yTIMEPRECISION  "timeprecision"
%शब्दांशः<fl>              yTIMEUNIT       "timeunit"
%शब्दांशः<fl>              yTRAN           "tran"
%शब्दांशः<fl>              yTRANIF0        "tranif0"
%शब्दांशः<fl>              yTRANIF1        "tranif1"
%शब्दांशः<fl>              yTRI            "tri"
%शब्दांशः<fl>              yTRI0           "tri0"
%शब्दांशः<fl>              yTRI1           "tri1"
%शब्दांशः<fl>              yTRIAND         "triand"
%शब्दांशः<fl>              yTRIOR          "trior"
%शब्दांशः<fl>              yTRIREG         "trireg"
%शब्दांशः<fl>              yTRUE           "true"
%शब्दांशः<fl>              yTYPEDEF        "typedef"
%शब्दांशः<fl>              yTYPE__EQ       "type-then-eqneq"
%शब्दांशः<fl>              yTYPE__ETC      "type"
%शब्दांशः<fl>              yTYPE__LEX      "type-in-lex"
%शब्दांशः<fl>              yUNION          "union"
%शब्दांशः<fl>              yUNIQUE         "unique"
%शब्दांशः<fl>              yUNIQUE0        "unique0"
%शब्दांशः<fl>              yUNSIGNED       "unsigned"
%शब्दांशः<fl>              yUNTIL          "until"
%शब्दांशः<fl>              yUNTIL_WITH     "until_with"
%शब्दांशः<fl>              yUNTYPED        "untyped"
%शब्दांशः<fl>              yUSE            "use"
%शब्दांशः<fl>              yVAR            "var"
%शब्दांशः<fl>              yVECTORED       "vectored"
%शब्दांशः<fl>              yVIRTUAL__CLASS "virtual-then-class"
%शब्दांशः<fl>              yVIRTUAL__ETC   "virtual"
%शब्दांशः<fl>              yVIRTUAL__INTERFACE     "virtual-then-interface"
%शब्दांशः<fl>              yVIRTUAL__LEX   "virtual-in-lex"
%शब्दांशः<fl>              yVIRTUAL__anyID "virtual-then-identifier"
%शब्दांशः<fl>              yVOID           "void"
%शब्दांशः<fl>              yWAIT           "wait"
%शब्दांशः<fl>              y_WAIT_क्रमः     "wait_order"
%शब्दांशः<fl>              yWAND           "wand"
%शब्दांशः<fl>              yWEAK           "weak"
%शब्दांशः<fl>              yWEAK0          "weak0"
%शब्दांशः<fl>              yWEAK1          "weak1"
%शब्दांशः<fl>              yWHILE          "while"
%शब्दांशः<fl>              yWILDCARD       "wildcard"
%शब्दांशः<fl>              yWIRE           "wire"
%शब्दांशः<fl>              yWITHIN         "within"
%शब्दांशः<fl>              yWITH__BRA      "with-then-["
%शब्दांशः<fl>              yWITH__CUR      "with-then-{"
%शब्दांशः<fl>              yWITH__ETC      "with"
%शब्दांशः<fl>              yWITH__LEX      "with-in-lex"
%शब्दांशः<fl>              yWITH__PAREN    "with-then-("
%शब्दांशः<fl>              yWITH__PAREN_CUR "with-then-(-then-{"
%शब्दांशः<fl>              yWOR            "wor"
%शब्दांशः<fl>              yWREAL          "wreal"
%शब्दांशः<fl>              yXNOR           "xnor"
%शब्दांशः<fl>              yXOR            "xor"

%शब्दांशः<fl>              yD_ACOS         "$acos"
%शब्दांशः<fl>              yD_ACOSH        "$acosh"
%शब्दांशः<fl>              yD_ASIN         "$asin"
%शब्दांशः<fl>              yD_ASINH        "$asinh"
%शब्दांशः<fl>              yD_ASSERTCTL    "$assertcontrol"
%शब्दांशः<fl>              yD_ASSERTFAILOFF "$assertfailoff"
%शब्दांशः<fl>              yD_ASSERTFAILON "$assertfailon"
%शब्दांशः<fl>              yD_ASSERTKILL   "$assertkill"
%शब्दांशः<fl>              yD_ASSERTNONVACUOUSON  "$assertnonvacuouson"
%शब्दांशः<fl>              yD_ASSERTOFF    "$assertoff"
%शब्दांशः<fl>              yD_ASSERTON     "$asserton"
%शब्दांशः<fl>              yD_ASSERTPASSOFF "$assertpassoff"
%शब्दांशः<fl>              yD_ASSERTPASSON  "$assertpasson"
%शब्दांशः<fl>              yD_ASSERTVACUOUSOFF  "$assertvacuousoff"
%शब्दांशः<fl>              yD_ATAN         "$atan"
%शब्दांशः<fl>              yD_ATAN2        "$atan2"
%शब्दांशः<fl>              yD_ATANH        "$atanh"
%शब्दांशः<fl>              y_D_द्वयङ्काः         "$bits"
%शब्दांशः<fl>              yD_BITSTOREAL   "$bitstoreal"
%शब्दांशः<fl>              yD_BITSTOSHORTREAL "$bitstoshortreal"
%शब्दांशः<fl>              yD_C            "$c"
%शब्दांशः<fl>              yD_CPURE        "$cpure"
%शब्दांशः<fl>              yD_CAST         "$cast"
%शब्दांशः<fl>              yD_CEIL         "$ceil"
%शब्दांशः<fl>              y_D_परिवर्तितम्      "$changed"
%शब्दांशः<fl>              y_D_परिवर्तितम्_GCLK "$changed_gclk"
%शब्दांशः<fl>              yD_CHANGING_GCLK  "$changing_gclk"
%शब्दांशः<fl>              yD_CLOG2        "$clog2"
%शब्दांशः<fl>              yD_COS          "$cos"
%शब्दांशः<fl>              yD_COSH         "$cosh"
%शब्दांशः<fl>              yD_COUNTBITS    "$countbits"
%शब्दांशः<fl>              yD_COUNTONES    "$countones"
%शब्दांशः<fl>              yD_DIMENSIONS   "$dimensions"
%शब्दांशः<fl>              yD_DISPLAY      "$display"
%शब्दांशः<fl>              yD_DISPLAYB     "$displayb"
%शब्दांशः<fl>              yD_DISPLAYH     "$displayh"
%शब्दांशः<fl>              yD_DISPLAYO     "$displayo"
%शब्दांशः<fl>              y_D_DIST_CHI_वर्गफलम् "$dist_chi_square"
%शब्दांशः<fl>              yD_DIST_ERLANG  "$dist_erlang"
%शब्दांशः<fl>              y_D_DIST_घातीयम् "$dist_exponential"
%शब्दांशः<fl>              y_D_DIST_प्रसामान्यम्  "$dist_normal"
%शब्दांशः<fl>              y_D_DIST_पोयसन "$dist_poisson"
%शब्दांशः<fl>              yD_DIST_T       "$dist_t"
%शब्दांशः<fl>              y_D_DIST_एकरूपम् "$dist_uniform"
%शब्दांशः<fl>              yD_DUMPALL      "$dumpall"
%शब्दांशः<fl>              yD_DUMPFILE     "$dumpfile"
%शब्दांशः<fl>              yD_DUMPFLUSH    "$dumpflush"
%शब्दांशः<fl>              yD_DUMPLIMIT    "$dumplimit"
%शब्दांशः<fl>              yD_DUMPOFF      "$dumpoff"
%शब्दांशः<fl>              yD_DUMPON       "$dumpon"
%शब्दांशः<fl>              yD_DUMPPORTS    "$dumpports"
%शब्दांशः<fl>              yD_DUMPVARS     "$dumpvars"
%शब्दांशः<fl>              yD_ERROR        "$error"
%शब्दांशः<fl>              yD_EXIT         "$exit"
%शब्दांशः<fl>              yD_EXP          "$exp"
%शब्दांशः<fl>              yD_FALLING_GCLK "$falling_gclk"
%शब्दांशः<fl>              yD_FATAL        "$fatal"
%शब्दांशः<fl>              yD_FCLOSE       "$fclose"
%शब्दांशः<fl>              yD_FDISPLAY     "$fdisplay"
%शब्दांशः<fl>              yD_FDISPLAYB    "$fdisplayb"
%शब्दांशः<fl>              yD_FDISPLAYH    "$fdisplayh"
%शब्दांशः<fl>              yD_FDISPLAYO    "$fdisplayo"
%शब्दांशः<fl>              yD_FELL         "$fell"
%शब्दांशः<fl>              yD_FELL_GCLK    "$fell_gclk"
%शब्दांशः<fl>              yD_FEOF         "$feof"
%शब्दांशः<fl>              yD_FERROR       "$ferror"
%शब्दांशः<fl>              yD_FFLUSH       "$fflush"
%शब्दांशः<fl>              yD_FGETC        "$fgetc"
%शब्दांशः<fl>              yD_FGETS        "$fgets"
%शब्दांशः<fl>              yD_FINISH       "$finish"
%शब्दांशः<fl>              yD_FLOOR        "$floor"
%शब्दांशः<fl>              yD_FMONITOR     "$fmonitor"
%शब्दांशः<fl>              yD_FMONITORB    "$fmonitorb"
%शब्दांशः<fl>              yD_FMONITORH    "$fmonitorh"
%शब्दांशः<fl>              yD_FMONITORO    "$fmonitoro"
%शब्दांशः<fl>              yD_FOPEN        "$fopen"
%शब्दांशः<fl>              yD_FREAD        "$fread"
%शब्दांशः<fl>              yD_FREWIND      "$frewind"
%शब्दांशः<fl>              yD_FSCANF       "$fscanf"
%शब्दांशः<fl>              yD_FSEEK        "$fseek"
%शब्दांशः<fl>              yD_FSTROBE      "$fstrobe"
%शब्दांशः<fl>              yD_FSTROBEB     "$fstrobeb"
%शब्दांशः<fl>              yD_FSTROBEH     "$fstrobeh"
%शब्दांशः<fl>              yD_FSTROBEO     "$fstrobeo"
%शब्दांशः<fl>              yD_FTELL        "$ftell"
%शब्दांशः<fl>              yD_FUTURE_GCLK  "$future_gclk"
%शब्दांशः<fl>              yD_FWRITE       "$fwrite"
%शब्दांशः<fl>              yD_FWRITEB      "$fwriteb"
%शब्दांशः<fl>              yD_FWRITEH      "$fwriteh"
%शब्दांशः<fl>              yD_FWRITEO      "$fwriteo"
%शब्दांशः<fl>              y_D_GET_INITIAL_RANDOM_बीजम् "$get_initial_random_seed"
%शब्दांशः<fl>              yD_GLOBAL_CLOCK "$global_clock"
%शब्दांशः<fl>              y_D_उच्चः         "$high"
%शब्दांशः<fl>              yD_HYPOT        "$hypot"
%शब्दांशः<fl>              yD_INCREMENT    "$increment"
%शब्दांशः<fl>              yD_INFERRED_DISABLE "$inferred_disable"
%शब्दांशः<fl>              yD_INFO         "$info"
%शब्दांशः<fl>              yD_ISUNBOUNDED  "$isunbounded"
%शब्दांशः<fl>              yD_ISUNKNOWN    "$isunknown"
%शब्दांशः<fl>              yD_ITOR         "$itor"
%शब्दांशः<fl>              y_D_वामः         "$left"
%शब्दांशः<fl>              yD_LN           "$ln"
%शब्दांशः<fl>              yD_LOG10        "$log10"
%शब्दांशः<fl>              y_D_नीचः          "$low"
%शब्दांशः<fl>              yD_MONITOR      "$monitor"
%शब्दांशः<fl>              yD_MONITORB     "$monitorb"
%शब्दांशः<fl>              yD_MONITORH     "$monitorh"
%शब्दांशः<fl>              yD_MONITORO     "$monitoro"
%शब्दांशः<fl>              yD_MONITOROFF   "$monitoroff"
%शब्दांशः<fl>              yD_MONITORON    "$monitoron"
%शब्दांशः<fl>              yD_ONEHOT       "$onehot"
%शब्दांशः<fl>              yD_ONEHOT0      "$onehot0"
%शब्दांशः<fl>              yD_PAST         "$past"
%शब्दांशः<fl>              yD_PAST_GCLK    "$past_gclk"
%शब्दांशः<fl>              yD_POW          "$pow"
%शब्दांशः<fl>              yD_PRINTTIMESCALE "$printtimescale"
%शब्दांशः<fl>              yD_RANDOM       "$random"
%शब्दांशः<fl>              yD_READMEMB     "$readmemb"
%शब्दांशः<fl>              yD_READMEMH     "$readmemh"
%शब्दांशः<fl>              yD_REALTIME     "$realtime"
%शब्दांशः<fl>              yD_REALTOBITS   "$realtobits"
%शब्दांशः<fl>              yD_REWIND       "$rewind"
%शब्दांशः<fl>              y_D_दक्षिणः        "$right"
%शब्दांशः<fl>              yD_RISING_GCLK  "$rising_gclk"
%शब्दांशः<fl>              y_D_मूलम्         "$root"
%शब्दांशः<fl>              yD_ROSE         "$rose"
%शब्दांशः<fl>              yD_ROSE_GCLK    "$rose_gclk"
%शब्दांशः<fl>              yD_RTOI         "$rtoi"
%शब्दांशः<fl>              yD_SAMPLED      "$sampled"
%शब्दांशः<fl>              yD_SDF_ANNOTATE "$sdf_annotate"
%शब्दांशः<fl>              yD_SETUPHOLD    "$setuphold"
%शब्दांशः<fl>              yD_SFORMAT      "$sformat"
%शब्दांशः<fl>              yD_SFORMATF     "$sformatf"
%शब्दांशः<fl>              yD_SHORTREALTOBITS "$shortrealtobits"
%शब्दांशः<fl>              yD_SIGNED       "$signed"
%शब्दांशः<fl>              yD_SIN          "$sin"
%शब्दांशः<fl>              yD_SINH         "$sinh"
%शब्दांशः<fl>              y_D_आकार         "$size"
%शब्दांशः<fl>              yD_SQRT         "$sqrt"
%शब्दांशः<fl>              yD_SSCANF       "$sscanf"
%शब्दांशः<fl>              yD_STABLE       "$stable"
%शब्दांशः<fl>              yD_STABLE_GCLK  "$stable_gclk"
%शब्दांशः<fl>              yD_STACKTRACE   "$stacktrace"
%शब्दांशः<fl>              yD_STEADY_GCLK  "$steady_gclk"
%शब्दांशः<fl>              yD_STIME        "$stime"
%शब्दांशः<fl>              yD_STOP         "$stop"
%शब्दांशः<fl>              yD_STROBE       "$strobe"
%शब्दांशः<fl>              yD_STROBEB      "$strobeb"
%शब्दांशः<fl>              yD_STROBEH      "$strobeh"
%शब्दांशः<fl>              yD_STROBEO      "$strobeo"
%शब्दांशः<fl>              yD_SWRITE       "$swrite"
%शब्दांशः<fl>              yD_SWRITEB      "$swriteb"
%शब्दांशः<fl>              yD_SWRITEH      "$swriteh"
%शब्दांशः<fl>              yD_SWRITEO      "$swriteo"
%शब्दांशः<fl>              yD_SYSTEM       "$system"
%शब्दांशः<fl>              yD_TAN          "$tan"
%शब्दांशः<fl>              yD_TANH         "$tanh"
%शब्दांशः<fl>              yD_TESTPLUSARGS "$test$plusargs"
%शब्दांशः<fl>              yD_TIME         "$time"
%शब्दांशः<fl>              yD_TIMEFORMAT   "$timeformat"
%शब्दांशः<fl>              yD_TIMEPRECISION "$timeprecision"
%शब्दांशः<fl>              yD_TIMEUNIT     "$timeunit"
%शब्दांशः<fl>              yD_TYPENAME     "$typename"
%शब्दांशः<fl>              yD_UNGETC       "$ungetc"
%शब्दांशः<fl>              yD_UNIT         "$unit"
%शब्दांशः<fl>              yD_UNPACKED_DIMENSIONS "$unpacked_dimensions"
%शब्दांशः<fl>              yD_UNSIGNED     "$unsigned"
%शब्दांशः<fl>              yD_URANDOM      "$urandom"
%शब्दांशः<fl>              yD_URANDOM_RANGE "$urandom_range"
%शब्दांशः<fl>              yD_VALUEPLUSARGS "$value$plusargs"
%शब्दांशः<fl>              yD_WARNING      "$warning"
%शब्दांशः<fl>              yD_WRITE        "$write"
%शब्दांशः<fl>              yD_WRITEB       "$writeb"
%शब्दांशः<fl>              yD_WRITEH       "$writeh"
%शब्दांशः<fl>              yD_WRITEMEMB    "$writememb"
%शब्दांशः<fl>              yD_WRITEMEMH    "$writememh"
%शब्दांशः<fl>              yD_WRITEO       "$writeo"

%शब्दांशः<fl>              yVL_CLOCKER               "/*verilator clocker*/"
%शब्दांशः<fl>              yVL_CLOCK_ENABLE          "/*verilator clock_enable*/"
%शब्दांशः<fl>              y_VL_COVERAGE_खण्डम्_OFF    "/*verilator coverage_block_off*/"
%शब्दांशः<fl>              yVL_DPI_C_DECL            "/*verilator dpi_c_decl*/"
%शब्दांशः<fl>              yVL_FORCEABLE             "/*verilator forceable*/"
%शब्दांशः<fl>              y_VL_पूर्णम्_CASE             "/*verilator full_case*/"
%शब्दांशः<fl>              y_VL_HIER_खण्डम्            "/*verilator hier_block*/"
%शब्दांशः<fl>              yVL_INLINE_MODULE         "/*verilator inline_module*/"
%शब्दांशः<fl>              yVL_ISOLATE_ASSIGNMENTS   "/*verilator isolate_assignments*/"
%शब्दांशः<fl>              yVL_NO_CLOCKER            "/*verilator no_clocker*/"
%शब्दांशः<fl>              yVL_NO_INLINE_MODULE      "/*verilator no_inline_module*/"
%शब्दांशः<fl>              yVL_NO_INLINE_TASK        "/*verilator no_inline_task*/"
%शब्दांशः<fl>              yVL_PARALLEL_CASE         "/*verilator parallel_case*/"
%शब्दांशः<fl>              y_VL_लौकिकम्                "/*verilator public*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT           "/*verilator public_flat*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT_ON        "/*verilator public_flat_on*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT_RD        "/*verilator public_flat_rd*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT_RD_ON     "/*verilator public_flat_rd_on*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT_RW        "/*verilator public_flat_rw*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT_RW_ON     "/*verilator public_flat_rw_on*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_FLAT_RW_ON_SNS "/*verilator public_flat_rw_on_sns*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_ON             "/*verilator public_on*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_OFF            "/*verilator public_off*/"
%शब्दांशः<fl>              y_VL_लौकिकम्_MODULE         "/*verilator public_module*/"
%शब्दांशः<fl>              yVL_SC_BIGUINT            "/*verilator sc_biguint*/"
%शब्दांशः<fl>              yVL_SC_BV                 "/*verilator sc_bv*/"
%शब्दांशः<fl>              yVL_SFORMAT               "/*verilator sformat*/"
%शब्दांशः<fl>              y_VL_विभज_VAR             "/*verilator split_var*/"
%शब्दांशः<fl>              yVL_FSM_ARC_INCL_COND     "/*verilator fsm_arc_include_cond*/"
%शब्दांशः<fl>              yVL_FSM_ARC_INCL_COND_AUTO        "/*verilator fsm_arc_include_cond_auto*/"
%शब्दांशः<fl>              yVL_FSM_ARC_INCL_COND_AUTO_EXPAND "/*verilator fsm_arc_include_cond_auto_expand*/"
%शब्दांशः<fl>              y_VL_FSM_ARC_INCL_COND_पूर्णम्        "/*verilator fsm_arc_include_cond_full*/"
%शब्दांशः<fl>              yVL_FSM_STATE             "/*verilator fsm_state*/"
%शब्दांशः<fl>              yVL_FSM_STATE_AUTO        "/*verilator fsm_state_auto*/"
%शब्दांशः<fl>              yVL_FSM_STATE_AUTO_EXPAND "/*verilator fsm_state_auto_expand*/"
%शब्दांशः<fl>              y_VL_FSM_STATE_पूर्णम्        "/*verilator fsm_state_full*/"
%शब्दांशः<strp>            y_VL_चिह्नकम्                   "/*verilator tag*/"
%शब्दांशः<fl>              yVL_UNROLL_DISABLE        "/*verilator unroll_disable*/"
%शब्दांशः<fl>              y_VL_UNROLL_पूर्णम्           "/*verilator unroll_full*/"

%शब्दांशः<fl>              yP_TICK         "'"
%शब्दांशः<fl>              yP_TICKBRA      "'{"
%शब्दांशः<fl>              yP_OROR         "||"
%शब्दांशः<fl>              yP_ANDAND       "&&"
%शब्दांशः<fl>              yP_NOR          "~|"
%शब्दांशः<fl>              yP_XNOR         "^~"
%शब्दांशः<fl>              yP_NAND         "~&"
%शब्दांशः<fl>              yP_EQUAL        "=="
%शब्दांशः<fl>              yP_NOTEQUAL     "!="
%शब्दांशः<fl>              yP_CASEEQUAL    "==="
%शब्दांशः<fl>              yP_CASENOTEQUAL "!=="
%शब्दांशः<fl>              yP_WILDEQUAL    "==?"
%शब्दांशः<fl>              yP_WILDNOTEQUAL "!=?"
%शब्दांशः<fl>              yP_GTE          ">="
%शब्दांशः<fl>              yP_LTE          "<="
%शब्दांशः<fl>              yP_LTE__IGNORE  "<=-ignored"    // Used when expr:<= means assignment
%शब्दांशः<fl>              yP_SLEFT        "<<"
%शब्दांशः<fl>              yP_SRIGHT       ">>"
%शब्दांशः<fl>              yP_SSRIGHT      ">>>"
%शब्दांशः<fl>              yP_POW          "**"

%शब्दांशः<fl>              yP_COLON__BEGIN ":-then-begin"
%शब्दांशः<fl>              yP_COLON__FORK  ":-then-fork"
%शब्दांशः<fl>              yP_EQ__NEW      "=-then-new"
%शब्दांशः<fl>              yP_PAR__IGNORE  "(-ignored"     // Used when sequence_expr:expr:( is ignored
%शब्दांशः<fl>              yP_PAR__STRENGTH "(-for-strength"

%शब्दांशः<fl>              yP_LTMINUSGT    "<->"
%शब्दांशः<fl>              yP_PLUSCOLON    "+:"
%शब्दांशः<fl>              yP_MINUSCOLON   "-:"
%शब्दांशः<fl>              yP_MINUSGT      "->"
%शब्दांशः<fl>              yP_MINUSGTGT    "->>"
%शब्दांशः<fl>              yP_EQGT         "=>"
%शब्दांशः<fl>              yP_ASTGT        "*>"
%शब्दांशः<fl>              yP_ANDANDAND    "&&&"
%शब्दांशः<fl>              yP_POUNDPOUND   "##"
%शब्दांशः<fl>              yP_POUNDMINUSPD "#-#"
%शब्दांशः<fl>              yP_POUNDEQPD    "#=#"
%शब्दांशः<fl>              yP_DOTSTAR      ".*"

%शब्दांशः<fl>              yP_ATAT         "@@"
%शब्दांशः<fl>              yP_COLONCOLON   "::"
%शब्दांशः<fl>              yP_COLONEQ      ":="
%शब्दांशः<fl>              yP_COLONDIV     ":/"
%शब्दांशः<fl>              yP_ORMINUSGT    "|->"
%शब्दांशः<fl>              yP_OREQGT       "|=>"
%शब्दांशः<fl>              yP_BRASTAR      "[*"
%शब्दांशः<fl>              yP_BRAEQ        "[="
%शब्दांशः<fl>              yP_BRAMINUSGT   "[->"
%शब्दांशः<fl>              yP_BRAPLUSKET   "[+]"

%शब्दांशः<fl>              yP_PLUSPLUS     "++"
%शब्दांशः<fl>              yP_MINUSMINUS   "--"
%शब्दांशः<fl>              yP_PLUSEQ       "+="
%शब्दांशः<fl>              yP_MINUSEQ      "-="
%शब्दांशः<fl>              yP_TIMESEQ      "*="
%शब्दांशः<fl>              yP_DIVEQ        "/="
%शब्दांशः<fl>              yP_MODEQ        "%="
%शब्दांशः<fl>              yP_ANDEQ        "&="
%शब्दांशः<fl>              yP_OREQ         "|="
%शब्दांशः<fl>              yP_XOREQ        "^="
%शब्दांशः<fl>              yP_SLEFTEQ      "<<="
%शब्दांशः<fl>              yP_SRIGHTEQ     ">>="
%शब्दांशः<fl>              yP_SSRIGHTEQ    ">>>="

%शब्दांशः<fl>              yP_PLUSSLASHMINUS "+/-"
%शब्दांशः<fl>              yP_PLUSPCTMINUS "+%-"

// [* is not an operator, as "[ * ]" is legal
// [= and [-> could be repetition operators, but to match [* we don't add them.
// '( is not an operator, as "' (" is legal

//********************
// Verilog op precedence
//UNSUP %token<fl>      prUNARYARITH
//UNSUP %token<fl>      prREDUCTION
//UNSUP %token<fl>      prNEGATION
//UNSUP %token<fl>      prEVENTBEGIN
%शब्दांशः<fl>      prTAGGED

// These prevent other conflicts
%वामतः           yP_ANDANDAND
%वामतः           yMATCHES
%वामतः           prTAGGED
//UNSUP %left   prSEQ_CLOCKING

// PSL op precedence

// Lowest precedence
// These are in IEEE 17.7.1
%असाहचर्य       prALWAYS prS_ALWAYS prEVENTUALLY prS_EVENTUALLY prIF prACCEPT_ON prREJECT_ON prSYNC_ACCEPT_ON prSYNC_REJECT_ON

%दक्षिणतः          yP_ORMINUSGT yP_OREQGT yP_POUNDMINUSPD yP_POUNDEQPD
%दक्षिणतः          yUNTIL yS_UNTIL yUNTIL_WITH yS_UNTIL_WITH y_उपपादयति
%दक्षिणतः          yIFF
%वामतः           yOR
%वामतः           yAND
%असाहचर्य       yNOT yNEXTTIME yS_NEXTTIME
%वामतः           yINTERSECT
%वामतः           yWITHIN
%दक्षिणतः          yTHROUGHOUT
%वामतः           prPOUNDPOUND_MULTI
%वामतः           yP_POUNDPOUND
%वामतः           yP_BRASTAR yP_BRAEQ yP_BRAMINUSGT yP_BRAPLUSKET

// Not specified, but needed higher than yOR, lower than normal non-pexpr expressions
//UNSUP %left           yPOSEDGE yNEGEDGE yEDGE

//UNSUP %left           '{' '}'

// Verilog op precedence
%दक्षिणतः          yP_MINUSGT yP_LTMINUSGT
%दक्षिणतः          '?' ':' yP_COLON__BEGIN yP_COLON__FORK
%वामतः           yP_OROR
%वामतः           yP_ANDAND
%वामतः           '|' yP_NOR
%वामतः           '^' yP_XNOR
%वामतः           '&' yP_NAND
%वामतः           yP_EQUAL yP_NOTEQUAL yP_CASEEQUAL yP_CASENOTEQUAL yP_WILDEQUAL yP_WILDNOTEQUAL
%वामतः           '>' '<' yP_GTE yP_LTE yP_LTE__IGNORE yINSIDE yDIST
%वामतः           yP_SLEFT yP_SRIGHT yP_SSRIGHT
%वामतः           '+' '-'
%वामतः           '*' '/' '%'
%वामतः           yP_POW
%वामतः           prUNARYARITH yP_MINUSMINUS yP_PLUSPLUS prREDUCTION prNEGATION
%वामतः           '.'
// Not in IEEE, but need to avoid conflicts; TICK should bind tightly just lower than COLONCOLON
%वामतः           yP_TICK
//%left         '(' ')' '[' ']' yP_COLONCOLON '.'

%असाहचर्य pr_अधस्तनम्_THAN_ELSE
%असाहचर्य yELSE

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

%आरम्भः source_text

%%
//**********************************************************************
// Files

source_text:                    // ==IEEE: source_text
                /*रिक्तानि*/                             { }
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
        |       library_declaration                     { PARSEP->rootp()->सङ्कलय_Miscsp($1); }
        |       include_statement                       { }
        |       interface_declaration                   { }
        |       क्रमादेशः_declaration                     { }
        |       package_declaration                     { }
        |       package_item_शिखरम्                         { यदि ($1) PARSEP->unitPackage($1->fileline())->सङ्कलय_Stmtsp($1); }
        |       bind_directive                          { यदि ($1) PARSEP->unitPackage($1->fileline())->सङ्कलय_Stmtsp($1); }
        |       config_declaration                      { }
        //                      // Verilator only
        |       yaT_RESETALL                            { }  // Else, under design, and illegal based on IEEE 22.3
        |       yaT_NOUNCONNECTED                       { PARSEP->unconnectedDrive(VOptionBool::OPT_DEFAULT_FALSE); }
        |       yaT_UNCONNECTED_PULL0                   { PARSEP->unconnectedDrive(VOptionBool::OPT_FALSE); }
        |       yaT_UNCONNECTED_PULL1                   { PARSEP->unconnectedDrive(VOptionBool::OPT_TRUE); }
        |       vltItem                                 { }
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
                packageFront package_itemListE yENDPACKAGE अन्तः_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          यदि ($2) $1->सङ्कलय_Stmtsp($2);
                          GRAMMARP->अन्तः_Label($<fl>4, $1, $4); }
        ;

packageFront<ग्रन्थिः_Modulep>:
                yPACKAGE lifetimeE idAny ';'
                        { $$ = new AstPackage{$<fl>3, *$3, PARSEP->libname()};
                          यदि ($$->name() == "std") {
                              यदि ($$->fileline()->filename() != V3Options::get_Std_Package_मार्गः()) {
                                  $$->v3error("Redeclaring the 'std' package is not allowed");
                              } अन्यथा {
                                  PARSEP->rootp()->stdPackagep(VN_AS($$, Package));
                              }
                          }
                          $$->अन्तर्_Library(true);  // packages are always libraries; don't want to make them a "top"
                          $$->lifetime($2);
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->time_चरमः_Unit());
                          PARSEP->rootp()->timeprecisionMerge($$->fileline(),
                                                              PARSEP->time_चरमः_Prec());
                          PARSEP->rootp()->सङ्कलय_Modulesp($$); }
        ;

package_itemListE<nodep>:       // IEEE: [{ package_item }]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       package_itemList                        { $$ = $1; }
        ;

package_itemList<nodep>:        // IEEE: { package_item }
                package_item                            { $$ = $1; }
        |       package_itemList package_item           { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

package_item<nodep>:            // ==IEEE: package_item
                package_or_generate_item_declaration    { $$ = $1; }
        |       anonymous_क्रमादेशः                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_item_शिखरम्<nodep>:         // ==IEEE: package_item

                package_or_generate_item_declNoChecker  { $$ = $1; }
        |       checker_declaration
                       { PARSEP->rootp()->सङ्कलय_Modulesp($1);
                         $$ = nullptr; }
        |       anonymous_क्रमादेशः                       { $$ = $1; }
        |       package_export_declaration              { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       sigAttrScope                            { $$ = nullptr; }
        ;

package_or_generate_item_declaration<nodep>:    // ==IEEE: package_or_generate_item_declaration
                package_or_generate_item_declNoChecker  { $$ = $1; }
        |       checker_declaration
                        { $1->v3warn(E_UNSUPPORTED, "Unsupported: 'checker' below unit-level");
                          PARSEP->rootp()->सङ्कलय_Modulesp($1);
                          $$ = nullptr; }
        ;

package_or_generate_item_declNoChecker<nodep>:
                net_declaration                         { $$ = $1; }
        |       दत्त_declaration                        { $$ = $1; }
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
        |       assertion_item_declaration              { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

package_import_declarationList<nodep>:
                package_import_declaration              { $$ = $1; }
        |       package_import_declarationList package_import_declaration
                        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

package_import_declaration<nodep>:      // ==IEEE: package_import_declaration
                yIMPORT package_import_itemList ';'     { $$ = $2; }
        ;

package_import_itemList<nodep>:
                package_import_item                     { $$ = $1; }
        |       package_import_itemList ',' package_import_item { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

package_import_item<nodep>:     // ==IEEE: package_import_item
                idCC/*package_identifier*/ yP_COLONCOLON package_import_itemObj
                        { $$ = new AstPackageImport{$<fl>1, *$<strp>1, *$3}; }
        ;

package_import_itemObj<strp>:   // IEEE: part of package_import_item
                idAny/*package_identifier*/             { $<fl>$ = $<fl>1; $$ = $1; }
        |       '*'                                     { $<fl>$ = $<fl>1; स्थिर string तारकम् = "*"; $$ = &तारकम्; }
        ;

package_export_declaration<nodep>: // IEEE: package_export_declaration
                yEXPORT '*' yP_COLONCOLON '*' ';'
                        { $$ = new Ast_Package_Export_तारकम्_तारकम्{$<fl>2}; }
        |       yEXPORT package_export_itemList ';'     { $$ = $2; }
        ;

package_export_itemList<nodep>:
                package_export_item                     { $$ = $1; }
        |       package_export_itemList ',' package_export_item { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

package_export_item<nodep>:     // ==IEEE: package_export_item
                idCC yP_COLONCOLON package_import_itemObj
                        { $$ = new AstPackageExport{$<fl>3, *$<strp>1, *$3}; }
        ;

//**********************************************************************
// Module headers

module_declaration:             // ==IEEE: module_declaration
        //                      // timeunits_declaration instead in module_item
        //                      // IEEE: module_nonansi_header + module_ansi_header
                modFront importsAndParametersE ports_तारकम्_E ';'
        /*cont*/    module_itemListE yENDMODULE अन्तः_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          यदि ($2) $1->सङ्कलय_Stmtsp($2);
                          यदि ($3) $1->सङ्कलय_Stmtsp($3);
                          यदि ($5) $1->सङ्कलय_Stmtsp($5);
                          GRAMMARP->अन्तः_Label($<fl>7, $1, $7); }
        |       udpFront ports_तारकम्_E ';'
        /*cont*/    module_itemListE yENDPRIMITIVE अन्तः_Label_E
                        { $1->modTrace(false);  // Stash for implicit wires, etc
                          यदि ($2) $1->सङ्कलय_Stmtsp($2);
                          यदि ($4) $1->सङ्कलय_Stmtsp($4);
                          GRAMMARP->m_tracingParse = true;
                          GRAMMARP->अन्तः_Label($<fl>6, $1, $6); }
        //
        |       yEXTERN modFront parameter_पत्तनम्_list_E ports_तारकम्_E ';'
                        { DEL($2->unlink_Fr_पृष्ठभागः()); }
                        // We allow modules to be declared after instantiations, so harmless
        ;

modFront<ग्रन्थिः_Modulep>:
        //                      // General note: all *Front functions must call symPushNew before
        //                      // any formal arguments, as the arguments must land in the new scope.
                yMODULE lifetimeE idAny
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname()};
                          $$->lifetime($2);
                          $$->अन्तर्_Library(PARSEP->अन्तर्_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->time_चरमः_Unit());
                          PARSEP->rootp()->timeprecisionMerge($$->fileline(),
                                                              PARSEP->time_चरमः_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive());
                          PARSEP->rootp()->सङ्कलय_Modulesp($$); }
        |       modFront sigAttrScope                   { $$ = $1; }
        ;

importsAndParametersE<nodep>:   // IEEE: common part of module_declaration, interface_declaration, program_declaration
        //                      // { package_import_declaration } [ parameter_port_list ]
                parameter_पत्तनम्_list_E
                        { $$ = $1; $<flag>$ = $<flag>1; }  // hasParameterList
        |       package_import_declarationList parameter_पत्तनम्_list_E
                        { $$ = सङ्कलय_अग्र_Null($1, $2);
                          $<flag>$ = $<flag>2; }  // hasParameterList
        ;

udpFront<ग्रन्थिः_Modulep>:
                yPRIMITIVE lifetimeE idAny
                        { $$ = new AstPrimitive{$<fl>3, *$3, PARSEP->libname()};
                          $$->अन्तर्_Library(true);
                          $$->lifetime($2);
                          $$->modTrace(false);
                          $$->सङ्कलय_Stmtsp(new AstPragma{$<fl>3, VPragmaType::INLINE_MODULE});
                          GRAMMARP->m_tracingParse = false;
                          PARSEP->rootp()->सङ्कलय_Modulesp($$); }
        ;

parameter_मानम्_assignment_Inst_E<pinp>:      // IEEE: [ parameter_value_assignment ] for instance
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       parameter_मानम्_assignment_Inst          { $$ = $1; }
        ;

parameter_मानम्_assignment_Class_E<pinp>:      // IEEE: [ parameter_value_assignment ] for classes
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       parameter_मानम्_assignment_Class         { $$ = $1; }
        ;

parameter_मानम्_assignment_Inst<pinp>:       // IEEE: parameter_value_assignment for instance
                '#' '(' instParamListE ')'              { $$ = $3; }
        //                      // Parentheses are optional around a single parameter
        //                      // IMPORTANT: Below hardcoded in tokenPipeScanParam
        |       '#' yaINTNUM                            { $$ = new AstPin{$<fl>2, 1, "", new AstConst{$<fl>2, *$2}}; }
        |       '#' yaFLOATNUM                          { $$ = new AstPin{$<fl>2, 1, "",
                                                                          new AstConst{$<fl>2, AstConst::RealDouble{}, $2}}; }
        |       '#' time_अङ्कः_Adjusted                     { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        |       '#' idClassSel                          { $$ = new AstPin{$<fl>2, 1, "", $2}; }
        //                      // Not needed in Verilator:
        //                      // Side effect of combining *_instantiations
        //                      // '#' delay_value      { UNSUP }
        ;

parameter_मानम्_assignment_Class<pinp>:  // IEEE: parameter_value_assignment (for classes)
        //                      // Like parameter_value_assignment, but for classes only, which always have #()
                '#' '(' instParamListE ')'              { $$ = $3; }
        ;

parameter_पत्तनम्_list_E<nodep>:    // IEEE: parameter_port_list + empty == parameter_value_assignment
                /*रिक्तानि*/                             { $$ = nullptr; $<flag>$ = false; }  // hasParameterList
        |       '#' '(' ')'                             { $$ = nullptr;
                                                          $<flag>$ = true; }  // hasParameterList
        //                      // IEEE: '#' '(' list_of_param_assignments { ',' parameter_port_declaration } ')'
        //                      // IEEE: '#' '(' parameter_port_declaration { ',' parameter_port_declaration } ')'
        //                      // Can't just do that as "," conflicts with between vars and between stmts, so
        //                      // split into pre-comma and post-comma parts
        |       '#' '('                                 { VARRESET_LIST(GPARAM);
                                                          GRAMMARP->m_pinAnsi = true; }
        /*cont*/    param_पत्तनम्_Decl_Or_Arg_List ')'          { $$ = $4;
                                                          $<flag>$ = true;  // hasParameterList
                                                          VARRESET_NONLIST(UNKNOWN);
                                                          GRAMMARP->m_pinAnsi = false; }
        //                      // Note legal to start with "a=b" with no parameter statement
        ;

param_पत्तनम्_Decl_Or_Arg_List<nodep>:  // IEEE: list_of_param_assignments + { parameter_port_declaration }
                param_पत्तनम्_Decl_Or_Arg                              { $$ = $1; }
        |       param_पत्तनम्_Decl_Or_Arg_List ',' param_पत्तनम्_Decl_Or_Arg   { $$ = $1->सङ्कलय_अग्र($3); }
        |       param_पत्तनम्_Decl_Or_Arg_List sigAttrScope             { $$ = $1; }
        ;

param_पत्तनम्_Decl_Or_Arg<nodep>:      // IEEE: param_assignment + parameter_port_declaration
        //                      // We combine the two as we can't tell which follows a comma
                param_पत्तनम्_Decl_Or_Arg_Sub                   { $$ = $1; }
        |       vl_चिह्नकम्                                   { $$ = nullptr; }
        ;

param_पत्तनम्_Decl_Or_Arg_Sub<nodep>:
                parameter_पत्तनम्_declaration_Front_E param_assignment       { $$ = $2; }
        |       parameter_पत्तनम्_declaration_Type_Front_E type_assignment    { $$ = $2; }
        |       sigAttrScope param_पत्तनम्_Decl_Or_Arg_Sub                      { $$ = $2; }
        ;

ports_तारकम्_E<nodep>:              // IEEE: .* + list_of_ports + list_of_port_declarations + empty
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        //                      // .* expanded from module_declaration
        //UNSUP '(' yP_DOTSTAR ')'                              { UNSUP }
        |       '('
        /*mid*/         { VARRESET_LIST(पत्तनम्); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    list_सम्बन्धि_ports ')'
                       { $$ = $3; VARRESET_NONLIST(UNKNOWN); GRAMMARP->m_pinAnsi = false; }
        ;

list_सम्बन्धि_ports_E<nodep>:          // IEEE: [ list_of_ports + list_of_port_declarations ]
                पत्तनम्_And_चिह्नकम्_E                             { $$ = $1; }
        |       list_सम्बन्धि_ports_E ',' पत्तनम्_And_चिह्नकम्_E          { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

list_सम्बन्धि_ports<nodep>:           // IEEE: list_of_ports + list_of_port_declarations
                पत्तनम्_And_चिह्नकम्                              { $$ = $1; }
        |       list_सम्बन्धि_ports_E ',' पत्तनम्_And_चिह्नकम्_E          { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

पत्तनम्_And_चिह्नकम्_E<nodep>:
                /*रिक्तानि*/
                        { ध्रुव पूर्णाङ्क p = PINNUMINC();
                          ध्रुव string name = "__pinNumber" + cvt_गन्तव्यम्_Str(p);
                          $$ = new Ast_पत्तनम्{CRELINE(), p, name};
                          AstVar* varp = new AstVar{CRELINE(), VVarType::WIRE, name, VFlagChildDType{},
                                                    new AstBasicDType{CRELINE(), LOGIC_IMPLICIT}};
                          varp->declDirection(VDirection::निवेशः);
                          varp->direction(VDirection::निवेशः);
                          varp->ansi(false);
                          varp->declTyped(true);
                          varp->trace(false);
                          $$ = $$->सङ्कलय_अग्र(varp);
                          $$->v3warn(NULLPORT, "Null port on module (perhaps extraneous comma)"); }
        |       पत्तनम्_And_चिह्नकम्                              { $$ = $1; }
        |       पत्तनम्_And_चिह्नकम् sigAttrScope                 { $$ = $1; }
        ;

पत्तनम्_And_चिह्नकम्<nodep>:
                पत्तनम्_3                                    { $$ = $1; }
        |       vl_चिह्नकम् पत्तनम्_3                              { $$ = $2; }  // Tag will associate with previous port
        |       sigAttrScope पत्तनम्_And_चिह्नकम्                 { $$ = $2; }
        ;

पत्तनम्_3<nodep>:                    // ==IEEE: port
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
                पत्तनम्_Dir_Net_E id/*interface*/ '.' idAny/*modport*/ पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { // VAR for now, but V3LinkCells may call setIfcaeRef on it later
                          $$ = $5; VARDECL(VAR); VARIO(NONE);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstIfaceRefDType{$<fl>2, $<fl>4, "", *$2, *$4};
                          VARDTYPE(dtp); VARIOANSI();
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $6, $7)); }
        |       पत्तनम्_Dir_Net_E yINTERFACE                           पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; GRAMMARP->createGenericIface($3, $4, $5); }
        |       पत्तनम्_Dir_Net_E yINTERFACE      '.' idAny/*modport*/ पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5; GRAMMARP->createGenericIface($5, $6, $7, $<fl>4, *$4); }
        //
        |       पत्तनम्_Dir_Net_E yINTERCONNECT signingE rangeListE पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $5;
                          BBUNSUP($<fl>2, "Unsupported: interconnect");
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); VARIOANSI();
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $6, $7)); }
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
        |       पत्तनम्_Dir_Net_E दत्त_type           पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $4, $5)); }
        |       पत्तनम्_Dir_Net_E दत्त_type           पत्तनम्_Sig variable_dimensionListE sigAttrListE '=' const_व्यञ्जकम्
                        { $$ = $3; VARDTYPE($2); VARIOANSI();
                          यदि (AstVar* vp = VARDONEP($$, $4, $5)) { सङ्कलय_अग्र_Null($$, vp); vp->valuep($7); } }
        |       पत्तनम्_Dir_Net_E yVAR दत्त_type      पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $5, $6)); }
        |       पत्तनम्_Dir_Net_E yVAR दत्त_type      पत्तनम्_Sig variable_dimensionListE sigAttrListE '=' const_व्यञ्जकम्
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          यदि (AstVar* vp = VARDONEP($$, $5, $6)) { सङ्कलय_अग्र_Null($$, vp); vp->valuep($8); } }
        |       पत्तनम्_Dir_Net_E yVAR implicit_typeE पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $5, $6)); }
        |       पत्तनम्_Dir_Net_E yVAR implicit_typeE पत्तनम्_Sig variable_dimensionListE sigAttrListE '=' const_व्यञ्जकम्
                        { $$ = $4; VARDECL(VAR); VARDTYPE($3); VARIOANSI();
                          यदि (AstVar* vp = VARDONEP($$, $5, $6)) { सङ्कलय_अग्र_Null($$, vp); vp->valuep($8); } }
        |       पत्तनम्_Dir_Net_E signing             पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $3;
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $4, $5)); }
        |       पत्तनम्_Dir_Net_E signing             पत्तनम्_Sig variable_dimensionListE sigAttrListE '=' const_व्यञ्जकम्
                        { $$ = $3;
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2};
                          VARDTYPE_NDECL(dtp); VARIOANSI();
                          यदि (AstVar* vp = VARDONEP($$, $4, $5)) { सङ्कलय_अग्र_Null($$, vp); vp->valuep($7); } }
        |       पत्तनम्_Dir_Net_E signingE rangeList  पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $4;
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          सङ्कलय_अग्र_Null($$, VARDONEP($$, $5, $6)); }
        |       पत्तनम्_Dir_Net_E signingE rangeList  पत्तनम्_Sig variable_dimensionListE sigAttrListE '=' const_व्यञ्जकम्
                        { $$ = $4;
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                                    new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE_NDECL(dtp);
                          यदि (AstVar* vp = VARDONEP($$, $5, $6)) { सङ्कलय_अग्र_Null($$, vp); vp->valuep($8); } }
        |       पत्तनम्_Dir_Net_E /*implicit*/        पत्तनम्_Sig variable_dimensionListE sigAttrListE
                        { $$ = $2; /*VARDTYPE-same*/ सङ्कलय_अग्र_Null($$, VARDONEP($$, $3, $4)); }
        |       पत्तनम्_Dir_Net_E /*implicit*/        पत्तनम्_Sig variable_dimensionListE sigAttrListE '=' const_व्यञ्जकम्
                        { $$ = $2; /*VARDTYPE-same*/
                          यदि (AstVar* vp = VARDONEP($$, $3, $4)) { सङ्कलय_अग्र_Null($$, vp); vp->valuep($6); } }
        //                      //   IEEE: '.' port_identifier '(' [ expression ] ')'
        |       पत्तनम्_Dir_Net_E /*implicit*/ '.' पत्तनम्_Sig '(' व्यञ्जकम् ')'
                        { $$ = $3; DEL($5);
                          BBUNSUP($<fl>2, "Unsupported: complex ports (IEEE 1800-2023 23.2.2.1/2)"); }
        //                      // IEEE: part of (non-ansi) port_reference
        |       '{' पत्तनम्_expression_List '}'
                        { $$ = $2; }
        ;

पत्तनम्_Dir_Net_E:                    // IEEE: part of port, optional net type and/or direction
                /*रिक्तानि*/                             { }
        //                      // Per spec, if direction given default the nettype.
        //                      // The higher level rule may override this VARDTYPE with one later in the parse.
        |       पत्तनम्_direction                                  { VARDECL(पत्तनम्); VARDTYPE_NDECL(nullptr); }
        |       पत्तनम्_direction { VARDECL(पत्तनम्); } net_type      { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        |       net_type                                        { VARDTYPE_NDECL(nullptr); }  // net_type calls VARDECL
        ;

पत्तनम्_decl_Net_E:                  // IEEE: part of port_declaration, optional net type
                /*रिक्तानि*/                             { }
        |       net_type                                { } // net_type calls VARDECL
        ;

पत्तनम्_Sig<nodep>:
                id/*पत्तनम्*/
                        { $$ = new Ast_पत्तनम्{$<fl>1, PINNUMINC(), *$1}; }
        |       idSVKwd
                        { $$ = new Ast_पत्तनम्{$<fl>1, PINNUMINC(), *$1}; }
        ;

पत्तनम्_expression_List<nodep>:  // IEEE: part of (non-ansi) port_reference
                पत्तनम्_reference                          { $$ = $1; }
        |       पत्तनम्_expression_List ',' पत्तनम्_reference  { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

पत्तनम्_reference<nodep>:  // IEEE: (non-ansi) port-reference
        //                      // IEEE: port_identifier constant_select
        //                      // constant_select ::= [ '[' constant_part_select_range ']' ]
                id/*port_identifier*/                   { $$ = nullptr; }  // UNSUP above here
        |       id/*port_identifier*/ part_select_range  { $$ = nullptr; DEL($2); }  // UNSUP above here
        ;

//**********************************************************************
// Interface headers

interface_declaration:          // IEEE: interface_declaration + interface_nonansi_header + interface_ansi_header:
        //                      // timeunits_declarationE is instead in interface_item
                intFront importsAndParametersE ports_तारकम्_E ';'
                        interface_itemListE yENDINTERFACE अन्तः_Label_E
                        { यदि ($2) $1->सङ्कलय_Stmtsp($2);
                          यदि ($3) $1->सङ्कलय_Stmtsp($3);
                          यदि ($5) $1->सङ्कलय_Stmtsp($5);
                          $1->hasParameterList($<flag>2); }
        |       yEXTERN intFront parameter_पत्तनम्_list_E ports_तारकम्_E ';'
                        { DEL($2->unlink_Fr_पृष्ठभागः()); }
                        // We allow interfaces to be declared after instantiations, so harmless
        ;

intFront<ग्रन्थिः_Modulep>:
                yINTERFACE lifetimeE idAny/*new_interface*/
                        { $$ = new AstIface{$<fl>3, *$3, PARSEP->libname()};
                          $$->अन्तर्_Library(true);
                          $$->lifetime($2);
                          PARSEP->rootp()->सङ्कलय_Modulesp($$); }
        |       intFront sigAttrScope                   { $$ = $1; }
        ;

interface_itemListE<nodep>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       interface_itemList                      { $$ = $1; }
        ;

interface_itemList<nodep>:
                interface_item                          { $$ = $1; }
        |       interface_itemList interface_item       { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

interface_item<nodep>:          // IEEE: interface_item + non_port_interface_item
                पत्तनम्_declaration ';'                    { $$ = $1; }
        //                      // IEEE: non_port_interface_item
        //                      // IEEE: generate_region
        |       i_generate_region                       { $$ = $1; }
        |       interface_or_generate_item              { $$ = $1; }
        |       क्रमादेशः_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within interface decls"); }
        //                      // IEEE 1800-2017: modport_item
        //                      // See instead old 2012 position in interface_or_generate_item
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within interface decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // See note in interface_or_generate item
        |       module_common_item                      { $$ = $1; }
        ;

interface_or_generate_item<nodep>:  // ==IEEE: interface_or_generate_item
        //                      // module_common_item in interface_item, as otherwise duplicated
        //                      // with module_or_generate_item's module_common_item
                modport_declaration                     { $$ = $1; }
        |       extern_tf_declaration                   { $$ = $1; }
        ;

//**********************************************************************
// Program headers

anonymous_क्रमादेशः<nodep>:       // ==IEEE: anonymous_program
        //                      // See the spec - this doesn't change the scope, items still go up "top"
                y_क्रमादेशः ';' anonymous_क्रमादेशः_item_List_E yENDPROGRAM
                        { $$ = nullptr;
                         BBUNSUP($<fl>1, "Unsupported: Anonymous programs");
                         VARDTYPE_NDECL(nullptr);
                         DEL($3); }
        ;

anonymous_क्रमादेशः_item_List_E<nodep>:     // IEEE: { anonymous_program_item }
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       anonymous_क्रमादेशः_item_List              { $$ = $1; }
        ;

anonymous_क्रमादेशः_item_List<nodep>:      // IEEE: { anonymous_program_item }
                anonymous_क्रमादेशः_item                  { $$ = $1; }
        |       anonymous_क्रमादेशः_item_List anonymous_क्रमादेशः_item       { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

anonymous_क्रमादेशः_item<nodep>:  // ==IEEE: anonymous_program_item
                task_declaration                        { $$ = $1; }
        |       function_declaration                    { $$ = $1; }
        |       class_declaration                       { $$ = $1; }
        //                      // IEEE-2023: Added: interface_class_declaration
        //                      // interface_class_declaration is part of class_declaration
        |       covergroup_declaration                  { $$ = $1; }
        //                      // class_constructor_declaration is part of function_declaration
        |       ';'                                     { $$ = nullptr; }
        ;

क्रमादेशः_declaration:            // IEEE: program_declaration + program_nonansi_header + program_ansi_header:
        //                      // timeunits_declarationE is instead in program_item
                pgmFront parameter_पत्तनम्_list_E ports_तारकम्_E ';'
        /*cont*/    क्रमादेशः_item_List_E yENDPROGRAM अन्तः_Label_E
                        { $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));  // Stash for implicit wires, etc
                          $1->hasParameterList($<flag>2);
                          यदि ($2) $1->सङ्कलय_Stmtsp($2);
                          यदि ($3) $1->सङ्कलय_Stmtsp($3);
                          यदि ($5) $1->सङ्कलय_Stmtsp($5);
                          GRAMMARP->अन्तः_Label($<fl>7, $1, $7); }
        |       yEXTERN pgmFront parameter_पत्तनम्_list_E ports_तारकम्_E ';'
                        { DEL($2->unlink_Fr_पृष्ठभागः()); }
                        // We allow programs to be declared after instantiations, so harmless
        ;

pgmFront<ग्रन्थिः_Modulep>:
                y_क्रमादेशः lifetimeE idAny/*new_program*/
                        { $$ = new AstModule{$<fl>3, *$3, PARSEP->libname(), AstModule::क्रमादेशः{}};
                          $$->lifetime($2);
                          $$->अन्तर्_Library(PARSEP->अन्तर्_Library() || $$->fileline()->celldefineOn());
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->time_चरमः_Unit());
                          PARSEP->rootp()->timeprecisionMerge($$->fileline(),
                                                              PARSEP->time_चरमः_Prec());
                          PARSEP->rootp()->सङ्कलय_Modulesp($$); }
        ;

क्रमादेशः_item_List_E<nodep>:       // ==IEEE: [{ program_item }]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       क्रमादेशः_item_List                        { $$ = $1; }
        ;

क्रमादेशः_item_List<nodep>:        // ==IEEE: { program_item }
                क्रमादेशः_item                            { $$ = $1; }
        |       क्रमादेशः_item_List क्रमादेशः_item           { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

क्रमादेशः_item<nodep>:            // ==IEEE: program_item
                पत्तनम्_declaration ';'                    { $$ = $1; }
        |       non_पत्तनम्_क्रमादेशः_item                   { $$ = $1; }
        ;

non_पत्तनम्_क्रमादेशः_item<nodep>:   // ==IEEE: non_port_program_item
                continuous_नियोजय                       { $$ = $1; }
        |       module_or_generate_item_declaration     { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       concurrent_assertion_item               { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       क्रमादेशः_generate_item                   { $$ = $1; }
        ;

क्रमादेशः_generate_item<nodep>:           // ==IEEE: program_generate_item
                loop_generate_construct                 { $$ = $1; }
        |       conditional_generate_construct          { $$ = $1; }
        |       generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       genItemBegin                            { $$ = $1; }
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
                yMODPORT modport_itemList ';'           { $$ = $2; }
        ;

modport_itemList<nodep>:                // IEEE: part of modport_declaration
                modport_item                            { $$ = $1; }
        |       modport_itemList ',' modport_item       { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

modport_item<nodep>:                    // ==IEEE: modport_item
                idAny/*new-modport*/ '('
        /*mid*/         { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT);
                          GRAMMARP->m_modportProtoTasksp = nullptr; }
        /*cont*/    modportPortsDeclList ')'
                        { $$ = new AstModport{$<fl>1, *$1, $4};
                          // Append any prototype tasks from import/export as interface siblings
                          यदि (GRAMMARP->m_modportProtoTasksp) {
                              $$ = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(
                                  GRAMMARP->m_modportProtoTasksp, $$);
                              GRAMMARP->m_modportProtoTasksp = nullptr;
                          } }
        ;

modportPortsDeclList<nodep>:
                modportPortsDecl                            { $$ = $1; }
        |       modportPortsDeclList ',' modportPortsDecl   { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

// IEEE: modport_ports_declaration  + modport_simple_ports_declaration
//      + (modport_tf_ports_declaration+import_export) + modport_clocking_declaration
// We've expanded the lists each take to instead just have standalone ID ports.
// We track the type as with the V2k series of defines, then create as each ID is seen.
modportPortsDecl<nodep>:
        //                      // IEEE: modport_simple_ports_declaration
                पत्तनम्_direction { GRAMMARP->m_modport_Imp_Exp_सक्रिय = false; } modport_Simple_पत्तनम्_Or_TFPort { $$ = $3; }
        //                      // IEEE: modport_clocking_declaration
        |       yCLOCKING idAny/*clocking_identifier*/ { $$ = new AstModportClockingRef{$1, *$2}; }
        //                      // IEEE: yIMPORT modport_tf_port
        //                      // IEEE: yEXPORT modport_tf_port
        //                      // modport_tf_port expanded here
        |       yIMPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, false};
                          GRAMMARP->m_modport_Imp_Exp_सक्रिय = true;
                          GRAMMARP->m_modport_Imp_Exp_चरमः_Is_Export = false; }
        |       yEXPORT idAny/*tf_identifier*/
                        { $$ = new AstModportFTaskRef{$<fl>2, *$2, true};
                          GRAMMARP->m_modport_Imp_Exp_सक्रिय = true;
                          GRAMMARP->m_modport_Imp_Exp_चरमः_Is_Export = true; }
        |       yIMPORT विधानम्_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), false};
                          GRAMMARP->m_modport_Imp_Exp_सक्रिय = true;
                          GRAMMARP->m_modport_Imp_Exp_चरमः_Is_Export = false;
                          // Import prototype: task should already exist in interface
                          // (from export or direct declaration). Delete the prototype.
                          DEL($2); }
        |       yEXPORT विधानम्_prototype
                        { $$ = new AstModportFTaskRef{$<fl>2, $2->name(), true};
                          GRAMMARP->m_modport_Imp_Exp_सक्रिय = true;
                          GRAMMARP->m_modport_Imp_Exp_चरमः_Is_Export = true;
                          // Export: add prototype task to interface (as sibling of modport)
                          GRAMMARP->m_modportProtoTasksp
                              = Ast_ग्रन्थिः::सङ्कलय_अग्र_Null(GRAMMARP->m_modportProtoTasksp, $2); }
        // Continuations of above after a comma.
        //                      // IEEE: modport_simple_ports_declaration
        |       modport_Simple_पत्तनम्_Or_TFPort                { $$ = $1; }
        ;

modport_Simple_पत्तनम्_Or_TFPort<nodep>:// IEEE: modport_simple_port or modport_tf_port, depending what keyword was earlier
                idAny                                   { $$ = GRAMMARP->m_modport_Imp_Exp_सक्रिय ?
                                                                static_cast<Ast_ग्रन्थिः*>(
                                                                  new AstModportFTaskRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_modport_Imp_Exp_चरमः_Is_Export} ) :
                                                                static_cast<Ast_ग्रन्थिः*>(
                                                                  new AstModportVarRef{
                                                                    $<fl>1, *$1, GRAMMARP->m_varIO} ); }
        |       '.' idAny '(' ')'                       { $$ = new AstModportVarRef{$<fl>2, *$2, GRAMMARP->m_varIO};
                                                          BBUNSUP($<fl>4, "Unsupported: Modport empty expression"); }
        |       '.' idAny '(' व्यञ्जकम् ')'                  { $$ = new AstModportVarRef{$<fl>2, *$2, $4, GRAMMARP->m_varIO}; }
        ;

//************************************************
// Variable Declarations

genvar_declaration<nodep>:      // ==IEEE: genvar_declaration
                yGENVAR list_सम्बन्धि_genvar_identifiers ';'  { $$ = $2; }
        ;

list_सम्बन्धि_genvar_identifiers<nodep>:      // IEEE: list_of_genvar_identifiers (for declaration)
                genvar_identifierDecl                   { $$ = $1; }
        |       list_सम्बन्धि_genvar_identifiers ',' genvar_identifierDecl    { $$ = $1->सङ्कलय_अग्र($3); }
        ;

genvar_identifierDecl<varp>:            // IEEE: genvar_identifier (for declaration)
                idAny/*new-genvar_identifier*/ sigAttrListE
                        { VARRESET_NONLIST(GENVAR);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$<fl>1, VBasicDTypeKwd::INTEGER};
                          VARDTYPE(dtp);
                          $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        ;

parameter_declaration<nodep>:   // IEEE: local_ or parameter_declaration and type_parameter_declaration
        //                      // IEEE: yPARAMETER yTYPE list_of_type_assignments ';'
        //                      // Instead of list_of_type_assignments
        //                      // we use list_of_param_assignments because for port handling
        //                      // it already must accept types, so simpler to have code only one place
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                parameter_declarationFront list_सम्बन्धि_param_assignments            { $$ = $2; }
        |       parameter_declarationTypeFront list_सम्बन्धि_type_assignments         { $$ = $2; }
        ;

parameter_declarationFront:     // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset दत्त_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        ;

parameter_declarationTypeFront: // IEEE: local_ or parameter_declaration w/o assignment
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/ VARDTYPE(new AstParseTypeDType{$2}); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        ;

parameter_पत्तनम्_declaration_Front_E: // IEEE: local_ or parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset implicit_typeE            { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       varParamReset दत्त_type                 { /*VARRESET-in-varParam*/ VARDTYPE($2); }
        |       implicit_typeE
                        { /*VARRESET-in-varParam*/
                          // Keep previous type to handle subsequent declarations.
                          // This rule is also used when the previous parameter is a type parameter
                          यदि ($1) $1->v3error("parameter port declarations require 'parameter'"
                                              " keyword before implicit data types"
                                              " (IEEE 1800-2023 6.20.1/A.2.1.1)\n"
                                              + $1->warn_अधिकम्()
                                              + "... Suggest add 'parameter' before here");
                       }
        |       दत्त_type                               { /*VARRESET-in-varParam*/ VARDTYPE($1); }
        ;

parameter_पत्तनम्_declaration_Type_Front_E: // IEEE: parameter_port_declaration w/o assignment
        //                      // IEEE: parameter_declaration (minus assignment)
        //                      // IEEE: local_parameter_declaration (minus assignment)
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                varParamReset yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstParseTypeDType{$2}; VARDTYPE(dtp); }
        |       varParamReset yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstParseTypeDType{$2, $3}; VARDTYPE(dtp); }
        |       yTYPE__ETC
                        { /*VARRESET-in-varParam*/
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstParseTypeDType{$1}; VARDTYPE(dtp); }
        |       yTYPE__ETC forward_type
                        { /*VARRESET-in-varParam*/
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstParseTypeDType{$1, $2}; VARDTYPE(dtp); }
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
                          यदि (GRAMMARP->m_netStrengthp) {
                              VL_DO_CLEAR(delete GRAMMARP->m_netStrengthp, GRAMMARP->m_netStrengthp = nullptr);
                          }
                          GRAMMARP->समुच्चयः_Net_Delay(nullptr); }
        ;

net_declarationFront:           // IEEE: beginning of net_declaration
                net_declRESET net_type driveStrengthE net_scalaredE net_दत्त_Type_E
                        { VARDTYPE_NDECL($5);
                          GRAMMARP->समुच्चयः_Net_Strength($3); }
        |       net_declRESET yINTERCONNECT signingE rangeListE
                        { BBUNSUP($<fl>2, "Unsupported: interconnect");
                          VARDECL(WIRE);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                                    new AstBasicDType{$2, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        ;

net_declRESET:
                /*रिक्तानि*/                             { VARRESET_NONLIST(UNKNOWN); }
        ;

net_scalaredE:
                /*रिक्तानि*/                             { }
        //                      //UNSUP: ySCALARED/yVECTORED ignored
        |       ySCALARED                               { }
        |       yVECTORED                               { }
        ;

net_दत्त_Type_E<ग्रन्थिः_DTypep>:
        //                      // If there's a SV data type there shouldn't be a delay on this wire
        //                      // Otherwise #(...) can't be determined to be a delay or parameters
        //                      // Submit this as a footnote to the committee
                var_दत्त_type                           { $$ = $1; }
        |       signingE rangeList delay_controlE
                        { $$ = GRAMMARP->सङ्कलय_Range(new AstBasicDType{$2->fileline(), LOGIC, $1},
                                                                    $2, true);
                          GRAMMARP->समुच्चयः_Net_Delay($3); }  // not implicit
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC, $1}; }  // not implicit
        |       /*implicit*/ delay_controlE
                        { $$ = new AstBasicDType{CRELINE(), LOGIC};
                          GRAMMARP->समुच्चयः_Net_Delay($1); }  // not implicit
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

पत्तनम्_direction:                 // ==IEEE: port_direction + tf_port_direction
        //                      // IEEE 19.8 just "input" FIRST forces type to wire - we'll ignore that here
        //                      // Only used for ANSI declarations
                y_निवेशः                                  { VARIO(निवेशः); }
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

पत्तनम्_direction_Reset:            // IEEE: port_direction that starts a port_declaraiton
        //                      // Used only for declarations outside the port list
                y_निवेशः                                  { VARRESET_NONLIST(UNKNOWN); VARIO(निवेशः); }
        |       yOUTPUT                                 { VARRESET_NONLIST(UNKNOWN); VARIO(OUTPUT); }
        |       yINOUT                                  { VARRESET_NONLIST(UNKNOWN); VARIO(INOUT); }
        |       yREF                                    { VARRESET_NONLIST(UNKNOWN); VARIO(REF); }
        |       yCONST__REF yREF                        { VARRESET_NONLIST(UNKNOWN); VARIO(CONSTREF); }
        ;

पत्तनम्_declaration<nodep>:        // ==IEEE: port_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port, tf_port_declaration, data_declarationVarFront
        //
        //                      // IEEE: inout_declaration
        //                      // IEEE: input_declaration
        //                      // IEEE: output_declaration
        //                      // IEEE: ref_declaration
                पत्तनम्_direction_Reset पत्तनम्_decl_Net_E दत्त_type
        /*mid*/         { VARDTYPE($3); }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments                   { $$ = $5; }
        |       पत्तनम्_direction_Reset पत्तनम्_decl_Net_E yVAR दत्त_type
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments                   { $$ = $6; }
        |       पत्तनम्_direction_Reset पत्तनम्_decl_Net_E yVAR implicit_typeE
        /*mid*/         { VARDECL(VAR); VARDTYPE($4); }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments                   { $$ = $6; }
        |       पत्तनम्_direction_Reset पत्तनम्_decl_Net_E signingE rangeList
        /*mid*/         { Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                              new AstBasicDType{$4->fileline(), LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments                   { $$ = $6; }
        |       पत्तनम्_direction_Reset पत्तनम्_decl_Net_E signing
        /*mid*/         { Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $3};
                          VARDTYPE_NDECL(dtp); }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments                   { $$ = $5; }
        |       पत्तनम्_direction_Reset पत्तनम्_decl_Net_E /*implicit*/
        /*mid*/         { VARDTYPE_NDECL(nullptr); /*default_nettype*/ }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments                   { $$ = $4; }
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
                          AstIfaceRefDType* ध्रुव dtp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          VARDTYPE(dtp); }
        /*cont*/    mpInstnameList
                        { $$ = VARDONEP($5, nullptr, nullptr); DEL($5); }
        //UNSUP: strengthSpecE for udp_instantiations
        ;

tf_पत्तनम्_declaration<nodep>:     // ==IEEE: tf_port_declaration
        //                      // Used inside function; followed by ';'
        //                      // SEE ALSO port_declaration, port, data_declarationVarFront
        //
                पत्तनम्_direction_Reset      दत्त_type      { VARDTYPE($2); }  list_सम्बन्धि_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       पत्तनम्_direction_Reset      implicit_typeE { VARDTYPE_NDECL($2); }  list_सम्बन्धि_tf_variable_identifiers ';'
                        { $$ = $4; }
        |       पत्तनम्_direction_Reset yVAR दत्त_type      { VARDTYPE($3); }  list_सम्बन्धि_tf_variable_identifiers ';'
                        { $$ = $5; }
        |       पत्तनम्_direction_Reset yVAR implicit_typeE { VARDTYPE($3); }  list_सम्बन्धि_tf_variable_identifiers ';'
                        { $$ = $5; }
        ;

integer_atom_type<basicDTypep>: // ==IEEE: integer_atom_type
                yBYTE                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::BYTE}; }
        |       ySHORTINT                               { $$ = new AstBasicDType{$1, VBasicDTypeKwd::SHORTINT}; }
        |       yINT                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INT}; }
        |       yLONGINT                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::LONGINT}; }
        |       yINTEGER                                { $$ = new AstBasicDType{$1, VBasicDTypeKwd::INTEGER}; }
        |       yTIME                                   { $$ = new AstBasicDType{$1, VBasicDTypeKwd::TIME}; }
        ;

integer_सदिशम्_type<basicDTypep>:       // ==IEEE: integer_atom_type
                yBIT                                    { $$ = new AstBasicDType{$1, VBasicDTypeKwd::BIT}; }
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

simple_typeNoRef<ग्रन्थिः_DTypep>:   // IEEE: simple_type without idType
        //                      // IEEE: integer_type
                integer_atom_type                       { $$ = $1; }
        |       integer_सदिशम्_type                     { $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        //
        //                      // { generate_block_identifer ... } '.'
        //                      // Need to determine if generate_block_identifier can be lex-detected
        ;

दत्त_type<ग्रन्थिः_DTypep>:          // ==IEEE: data_type
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                दत्त_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idType packed_dimensionListE
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idType parameter_मानम्_assignment_Class packed_dimensionListE
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

दत्त_type_Any<ग्रन्थिः_DTypep>:       // ==IEEE: data_type (accepting idAny)
        //                      // IEEE: data_type_or_incomplete_class_scoped_type parses same as this
        //                      // as can't tell them apart until link
        //                      // This expansion also replicated elsewhere, IE data_type__AndID
                दत्त_type_No_Ref                          { $$ = $1; }
        //
        //                      // REFERENCES
        //
        //                      // IEEE: [ class_scope | package_scope ] type_identifier { packed_dimension }
        //                      // IEEE: class_type
        //                      // IEEE: ps_covergroup_identifier
        //                      // Don't distinguish between types and classes so all these combined
        |       packageClassScopeE idAny packed_dimensionListE
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = GRAMMARP->createArray(refp, $3, true); }
        |       packageClassScopeE idAny parameter_मानम्_assignment_Class packed_dimensionListE
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = GRAMMARP->createArray(refp, $4, true); }
        ;

दत्त_type_Basic<ग्रन्थिः_DTypep>:             // IEEE: part of data_type
                integer_सदिशम्_type signingE rangeListE { $1->समुच्चयः_Signed_State($2); $$ = GRAMMARP->सङ्कलय_Range($1, $3, true); }
        //                      // IEEE: packed_dimension: unsized_dimension, legal only in DPI import
        |       integer_सदिशम्_type signingE '[' ']'
                        { $1->समुच्चयः_Signed_State($2);
                          $$ = new AstUnsizedArrayDType{$3, VFlagChildDType{}, $1}; }
        |       integer_atom_type signingE              { $1->समुच्चयः_Signed_State($2); $$ = $1; }
        |       non_integer_type                        { $$ = $1; }
        ;

दत्त_type_No_Ref<ग्रन्थिः_DTypep>:             // ==IEEE: data_type, excluding class_type etc references
                दत्त_type_Basic                          { $$ = $1; }
        |       struct_unionDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpsu" + cvt_गन्तव्यम्_Str(GRAMMARP->s_type_Imp_अङ्कः++),
                                                           VFlagChildDType{}, $1}, $2, true); }
        |       enumDecl packed_dimensionListE
                        { $$ = GRAMMARP->createArray(
                                   new AstDefImplicitDType{$1->fileline(),
                                                           "__typeimpenum" + cvt_गन्तव्यम्_Str(GRAMMARP->s_type_Imp_अङ्कः++),
                                                           VFlagChildDType{}, $1}, $2, true); }
        |       ySTRING
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::STRING}; }
        |       yCHANDLE
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CHANDLE}; }
        |       yEVENT
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::EVENT}; v3Global.समुच्चयः_Has_Events(); }
        |       type_referenceDecl                      { $$ = $1; }
        //                      // IEEE: class_scope: see data_type above
        //                      // IEEE: class_type: see data_type above
        //                      // IEEE: ps_covergroup: see data_type above
        //                      // Rules overlap virtual_interface_declaration
        |       yVIRTUAL__INTERFACE yINTERFACE दत्त_type_Virtual
                        { $$ = $3; }
        |       yVIRTUAL__anyID                दत्त_type_Virtual
                        { $$ = $2; }
        ;

दत्त_type_Virtual<ग्रन्थिः_DTypep>:           // ==IEEE: data_type after yVIRTUAL [ yINTERFACE ]
        //                      // Parameters here are SV2009
                idAny/*interface*/
                        { AstIfaceRefDType* ध्रुव ifrefp = new AstIfaceRefDType{$<fl>1, "", *$1};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ '.' idAny/*modport*/
                        { AstIfaceRefDType* ध्रुव ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>3, "", *$1, *$3};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_मानम्_assignment_Class
                        { AstIfaceRefDType* ध्रुव ifrefp = new AstIfaceRefDType{$<fl>1, nullptr, "", *$1, "", $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        |       idAny/*interface*/ parameter_मानम्_assignment_Class '.' idAny/*modport*/
                        { AstIfaceRefDType* ध्रुव ifrefp = new AstIfaceRefDType{$<fl>1, $<fl>4, "", *$1, *$4, $2};
                          ifrefp->isVirtual(true);
                          $$ = ifrefp; }
        ;

दत्त_type_or_void<ग्रन्थिः_DTypep>:  // ==IEEE: data_type_or_void
                दत्त_type_Any                            { $$ = $1; }
        |       yVOID
                        { $$ = new AstBasicDType{$1, VBasicDTypeKwd::CVOID}; }
        ;

var_दत्त_type<ग्रन्थिः_DTypep>:              // ==IEEE: var_data_type
                दत्त_type                               { $$ = $1; }
        |       yVAR दत्त_type                          { $$ = $2; }
        |       yVAR implicit_typeE                     { $$ = $2; }
        ;

type_referenceBoth<ग्रन्थिः_Exprp>:          // IEEE: type_reference
                yTYPE__ETC '(' व्यञ्जकम्_Or_दत्त_Type ')'
                        { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::TYPEID, $3}; }
        ;

type_referenceDecl<ग्रन्थिः_DTypep>:         // IEEE: type_reference (as a data type for declaration)
                yTYPE__ETC '(' व्यञ्जकम्_Or_दत्त_Type ')'
                        { $$ = new AstRefDType{$1, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{}, $3}; }
        ;

type_referenceEq<ग्रन्थिः_Exprp>:            // IEEE: type_reference (as an ==/!== expression)
                yTYPE__EQ '(' व्यञ्जकम्_Or_दत्त_Type ')'
                        { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::TYPEID, $3}; }
        ;

struct_unionDecl<ग्रन्थिः_UOr_Struct_DTypep>:  // IEEE: part of data_type
        //                      // packedSigningE is NOP for unpacked
                ySTRUCT        packedSigningE '{'
        /*mid*/         { $<ग्रन्थिः_UOr_Struct_DTypep>$ = new AstStructDType{$1, $2}; }
        /*cont*/    struct_union_member_List_अन्तः
                        { $$ = $<ग्रन्थिः_UOr_Struct_DTypep>4; $$->सङ्कलय_Membersp($5); }
        |       yUNION taggedSoftE packedSigningE '{'
        /*mid*/         { $<ग्रन्थिः_UOr_Struct_DTypep>$ = new AstUnionDType{$1, $2 == tagged_SOFT, $2 == tagged_TAGGED, $3}; }
        /*cont*/    struct_union_member_List_अन्तः
                        { $$ = $<ग्रन्थिः_UOr_Struct_DTypep>5; $$->सङ्कलय_Membersp($6); }
        ;

struct_union_member_List_अन्तः<memberDTypep>: // IEEE: { struct_union_member } '}'
                struct_union_memberList '}'                     { $$ = $1; }
        //
        |       struct_union_memberList error '}'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error '}'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_memberList<memberDTypep>: // IEEE: { struct_union_member }
                struct_union_member                             { $$ = $1; }

        |       struct_union_memberList struct_union_member     { $$ = सङ्कलय_अग्र_Null($1, $2); }
        //
        |       struct_union_memberList error ';'               { $$ = $1; }  // LCOV_EXCL_LINE
        |       error ';'                                       { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

struct_union_member<memberDTypep>:     // ==IEEE: struct_union_member
                random_qualifierE दत्त_type_or_void
        /*mid*/         { GRAMMARP->m_memDTypep = $2;  // As a list follows, need to attach this dtype to each member.
                          GRAMMARP->m_memRand = $1.randAttr(); }
        /*cont*/    list_सम्बन्धि_member_decl_assignments ';'
                        { $$ = $4;
                          DEL(GRAMMARP->m_memDTypep); GRAMMARP->m_memDTypep = nullptr;
                          GRAMMARP->m_memRand = VRandAttr::NONE; }
        |       vl_चिह्नकम्                                   { $$ = nullptr; }
        ;

list_सम्बन्धि_member_decl_assignments<memberDTypep>: // Derived from IEEE: list_of_variable_decl_assignments
                member_decl_assignment                  { $$ = $1; }
        |       list_सम्बन्धि_member_decl_assignments ',' member_decl_assignment      { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

member_decl_assignment<memberDTypep>:   // Derived from IEEE: variable_decl_assignment
        //                      // At present we allow only packed structures/unions.
        //                      // So this is different from variable_decl_assignment
                idAny variable_dimensionListE
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlagChildDType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_वृक्षः(true) : nullptr),
                                                                        $2, false),
                                                  nullptr};
                          $$->यादृच्छिकम्(GRAMMARP->m_memRand);
                          PARSEP->चिह्नकम्_Nodep($$);
                        }
        |       idAny variable_dimensionListE '=' variable_decl_व्यञ्जकम्
                        { $$ = new AstMemberDType{$<fl>1, *$1, VFlagChildDType{},
                                                  GRAMMARP->createArray((GRAMMARP->m_memDTypep
                                                                         ? GRAMMARP->m_memDTypep->clone_वृक्षः(true) : nullptr),
                                                                        $2, false),
                                                  $4};
                          $$->यादृच्छिकम्(GRAMMARP->m_memRand);
                          PARSEP->चिह्नकम्_Nodep($$);
                        }
        |       idSVKwd                                 { $$ = nullptr; }
        //
        //                      // IEEE: "dynamic_array_variable_identifier '[' ']' [ '=' dynamic_array_new ]"
        //                      // Matches above with variable_dimensionE = "[]"
        //                      // IEEE: "class_variable_identifier [ '=' class_new ]"
        //                      // variable_dimensionE must be empty
        //                      // Pushed into variable_declExpr:dynamic_array_new
        ;

list_सम्बन्धि_variable_decl_assignments<varp>:        // ==IEEE: list_of_variable_decl_assignments
                variable_decl_assignment                { $$ = $1; }
        |       list_सम्बन्धि_variable_decl_assignments ',' variable_decl_assignment  { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

variable_decl_assignment<varp>: // ==IEEE: variable_decl_assignment
                id variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       id variable_dimensionListE sigAttrListE '=' variable_decl_व्यञ्जकम्
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

list_सम्बन्धि_tf_variable_identifiers<nodep>: // ==IEEE: list_of_tf_variable_identifiers
                tf_variable_identifier                  { $$ = $1; }
        |       list_सम्बन्धि_tf_variable_identifiers ',' tf_variable_identifier      { $$ = $1->सङ्कलय_अग्र($3); }
        ;

tf_variable_identifier<varp>:           // IEEE: part of list_of_tf_variable_identifiers
                id variable_dimensionListE sigAttrListE व्यञ्जकम्_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          यदि ($4) Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(
                                      $$, new Ast_नियोजय{$4->fileline(), new AstParseRef{$<fl>1, *$1}, $4}); }
        ;

variable_decl_व्यञ्जकम्<nodep>:               // IEEE: part of variable_decl_assignment - rhs of expr
                व्यञ्जकम्                                    { $$ = $1; }
        |       dynamic_array_new                       { $$ = $1; }
        |       class_newNoScope                        { $$ = $1; }
        ;

variable_dimensionListE<ग्रन्थिः_Rangep>:    // IEEE: variable_dimension + empty
                /*empty*/                               { $$ = nullptr; }
        |       variable_dimensionList                  { $$ = $1; }
        ;

variable_dimensionList<ग्रन्थिः_Rangep>:     // IEEE: variable_dimension + empty
                variable_dimension                      { $$ = $1; }
        |       variable_dimensionList variable_dimension       { $$ = $1->सङ्कलय_अग्र($2); }
        ;

variable_dimension<ग्रन्थिः_Rangep>: // ==IEEE: variable_dimension
        //                      // IEEE: unsized_dimension
                '[' ']'                                 { $$ = new AstUnsizedRange{$1}; }
        //                      // IEEE: unpacked_dimension
        |       anyrange                                { $$ = $1; }
        //                      // IEEE: unpacked_dimension (if const_expr)
        //                      // IEEE: associative_dimension (if data_type)
        //                      // Can't tell which until see if expr is data type or not
        |       '[' व्यञ्जकम्_Or_दत्त_Type ']'                  { $$ = new AstBracketRange{$1, $2}; }
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
        |       yPACKED signingE                        { $$ = $2; यदि ($$ == VSigning::NOSIGN) $$ = VSigning::UNSIGNED; }
        ;

//************************************************
// enum

// IEEE: part of data_type
enumDecl<ग्रन्थिः_DTypep>:
                yENUM enum_आधारः_type_E '{' enum_nameList '}'
                        { $$ = new AstEnumDType{$1, VFlagChildDType{}, $2, $4}; }
        ;

enum_आधारः_type_E<ग्रन्थिः_DTypep>:    // IEEE: enum_base_type
                /*रिक्तानि*/
                        { $$ = new AstBasicDType{CRELINE(), VBasicDTypeKwd::INT}; }
        //                      // Not in spec, but obviously "enum [1:0]" should work
        //                      // implicit_type expanded, without empty
        //                      // Note enum base types are always packed data types
        |       signingE rangeList
                        { $$ = GRAMMARP->सङ्कलय_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        //
        |       integer_atom_type signingE
                        { $1->समुच्चयः_Signed_State($2); $$ = $1; }
        |       integer_सदिशम्_type signingE rangeListE
                        { $1->समुच्चयः_Signed_State($2); $$ = GRAMMARP->सङ्कलय_Range($1, $3, true); }
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
        |       enum_nameList ',' enum_name_declaration { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

enum_name_declaration<enumItemp>:   // ==IEEE: enum_name_declaration
                idAny/*enum_identifier*/ enumNameRangeE enumNameStartE
                        { $$ = new AstEnumItem{$<fl>1, *$1, $2, $3}; }
        ;

enumNameRangeE<rangep>:          // IEEE: second part of enum_name_declaration
                /*रिक्तानि*/
                        { $$ = nullptr; }
        |       '[' intnumAsConst ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstConst($1, $2->गन्तव्यम्_SInt() - 1)}; DEL($2); }
        |       '[' intnumAsConst ':' intnumAsConst ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

enumNameStartE<ग्रन्थिः_Exprp>:      // IEEE: third part of enum_name_declaration
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       '=' const_व्यञ्जकम्                           { $$ = $2; }
        ;

intnumAsConst<constp>:
                yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        ;

//************************************************
// Typedef

दत्त_declaration<nodep>:        // ==IEEE: data_declaration
        //                      // VARRESET can't be called here - conflicts
                दत्त_declaration_Var                     { $$ = $1; }
        |       type_declaration                        { $$ = $1; }
        |       package_import_declaration              { $$ = $1; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        //                      // 1800-2009:
        |       nettype_declaration                     { $$ = $1; }
        |       vl_चिह्नकम्                                   { $$ = nullptr; }
        ;

class_property<nodep>:          // ==IEEE: class_property, which is {property_qualifier} data_declaration
                memberQualListE दत्त_declaration_Var_Class
                        { $$ = $2; $1.apply_गन्तव्यम्_Nodes($2); }
        |       memberQualListE type_declaration
                        { $$ = $2; यदि (VN_IS($2, Typedef)) $1.apply_गन्तव्यम्_Nodes(VN_AS($2, Typedef)); }
        //                      // UNSUP: Import needs to apply local/protected from memberQualList, and error on others
        |       memberQualListE package_import_declaration
                        { $$ = $2; }
        //                      // IEEE: virtual_interface_declaration
        //                      // "yVIRTUAL yID yID" looks just like a data_declaration
        //                      // Therefore the virtual_interface_declaration term isn't used
        ;

दत्त_declaration_Var<varp>:      // IEEE: part of data_declaration
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                दत्त_declaration_Var_Front list_सम्बन्धि_variable_decl_assignments ';'  { $$ = $2; }
        ;

दत्त_declaration_Var_Class<varp>: // IEEE: part of data_declaration (for class_property)
        //                      // The first declaration has complications between assuming what's
        //                      // the type vs ID declaring
                दत्त_declaration_Var_Front_Class list_सम्बन्धि_variable_decl_assignments ';'     { $$ = $2; }
        ;

दत्त_declaration_Var_Front:       // IEEE: part of data_declaration
        //                      // Non-ANSI; used inside block followed by ';'
        //                      // SEE ALSO port_declaration, tf_port_declaration, port
        //
        //                      // Expanded: "constE yVAR lifetimeE data_type"
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE दत्त_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT};
                          VARDTYPE(dtp); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/ VARLIFE($2);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                              new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp); }
        //
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
        |       yCONST__ETC yVAR lifetimeE दत्त_type
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstConstDType{$<fl>2, VFlagChildDType{}, $4};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstConstDType{$<fl>2, VFlagChildDType{},
                                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          VARDTYPE(dtp); }
        |       yCONST__ETC yVAR lifetimeE signingE rangeList
                        { VARRESET_NONLIST(VAR); VARLIFE($3);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstConstDType{$<fl>2, VFlagChildDType{},
                                  GRAMMARP->सङ्कलय_Range(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $4}, $5, true)};
                          VARDTYPE(dtp); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       /**/                  दत्त_type
                        { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        |       /**/        lifetime  दत्त_type
                        { VARRESET_NONLIST(VAR); VARLIFE($1); VARDTYPE($2); }
        |       yCONST__ETC lifetimeE दत्त_type
                        { VARRESET_NONLIST(VAR); VARLIFE($2);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstConstDType{$<fl>1, VFlagChildDType{}, $3};
                          VARDTYPE(dtp); }
        //                      // = class_new is in variable_decl_assignment
        ;

दत्त_declaration_Var_Front_Class:  // IEEE: part of data_declaration (for class_property)
        //                      // VARRESET called before this rule
        //                      // yCONST is removed, added to memberQual rules
        //                      // implicit_type expanded into /*empty*/ or "signingE rangeList"
                yVAR lifetimeE दत्त_type        { VARRESET_NONLIST(VAR); VARLIFE($2); VARDTYPE($3); }
        |       yVAR lifetimeE                  { VARRESET_NONLIST(VAR); VARLIFE($2); }
        |       yVAR lifetimeE signingE rangeList
                        { /*VARRESET-in-ddVar*/
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $3}, $4, true);
                          VARDTYPE(dtp);
                          VARLIFE($2); }
        //
        //                      // Expanded: "constE lifetimeE data_type"
        |       दत्त_type                       { VARRESET_NONLIST(VAR); VARDTYPE($1); }
        //                      // lifetime is removed, added to memberQual rules to avoid conflict
        //                      // yCONST is removed, added to memberQual rules to avoid conflict
        //                      // = class_new is in variable_decl_assignment
        ;

nettype_declaration<nodep>:  // IEEE: nettype_declaration/net_type_declaration
        //                      // Union of data_typeAny and nettype_identifier matching
                yNETTYPE दत्त_type_No_Ref
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>3, *$3); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE दत्त_type_No_Ref
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
        |       yNETTYPE packageClassScopeE idAny parameter_मानम्_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype"); }
        |       yNETTYPE packageClassScopeE idAny parameter_मानम्_assignment_Class packed_dimensionListE
        /*cont*/   idAny/*nettype_identifier*/
        /*cont*/   yWITH__ETC packageClassScopeE id/*tf_identifier*/ ';'
                        { $$ = GRAMMARP->createNettype($<fl>6, *$6); BBUNSUP($<fl>1, "Unsupported: nettype with"); }
        ;

implicit_typeE<ग्रन्थिः_DTypep>:             // IEEE: part of *data_type_or_implicit
        //                      // Also expanded in data_declaration
                /*रिक्तानि*/
                        { $$ = nullptr; }
        |       signingE rangeList
                        { $$ = GRAMMARP->सङ्कलय_Range(new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true); }
        |       signing
                        { $$ = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1}; }
        ;

assertion_variable_declaration<nodep>:  // IEEE: assertion_variable_declaration
        //                      // IEEE: var_data_type expanded
                var_दत्त_type                        { VARRESET_NONLIST(VAR); VARDTYPE_NDECL($1); }
        /*cont*/    list_सम्बन्धि_variable_decl_assignments ';'     { $$ = $3; }
        ;

type_declaration<nodep>:        // ==IEEE: type_declaration
                                // Data_type expanded
                yTYPEDEF दत्त_type_No_Ref
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { Ast_ग्रन्थिः_DType* ध्रुव dtp = $2;
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, dtp, $4); }

        // IEEE 1800-2023 6.18 typedef: dotted or arrayed type identifier
        // Handles interface typedef references like if0.rq_t and if0[0].rq_t (arrays allowed after first component)
        |       yTYPEDEF idDottedOrArrayed
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* ध्रुव ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* ध्रुव varp = VARDONEA($<fl>3, *$3, $4, $5);
                // idDottedOrArrayed produces Dot/SelBit tree for hierarchical refs like if0[0].rq_t
                varp->valuep($2);
                $$ = varp; }

        // IEEE 1800-2023 6.18 typedef with hierarchical type identifier
        // Special-case array on first component requiring a '.' after ']' to disambiguate from packed dims
        // Examples: typedef if0[0].rq_t my_t; typedef if0[0].x_if.rq_t my_t;
        |       yTYPEDEF id '[' व्यञ्जकम् ']' '.' id_Dotted_Sel_अधिकम्
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
              { VARRESET_NONLIST(LPARAM);
                AstParseTypeDType* ध्रुव ptypep = new AstParseTypeDType{$<fl>2, VFwdType::NONE};
                VARDTYPE(ptypep);
                AstVar* ध्रुव varp = VARDONEA($<fl>8, *$8, $9, $10);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrp = new AstSelBit{$3, new AstParseRef{$<fl>2, *$2, nullptr, nullptr}, $4};
                varp->valuep(new AstDot{$6, false, arrp, $7});
                $$ = varp; }

        |       yTYPEDEF packageClassScope idAny packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>3, *$3, $2, nullptr};
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->createArray(refp, $4, true);
                          $$ = GRAMMARP->createTypedef($<fl>5, *$5, $7, dtp, $6); }
        |       yTYPEDEF packageClassScope idAny parameter_मानम्_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>3, *$3, $2, $4};
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->createArray(refp, $5, true);
                          $$ = GRAMMARP->createTypedef($<fl>6, *$6, $8, dtp, $7); }

        // Type alias without packed dimensions: typedef existing_t new_t;
        |       yTYPEDEF idAny idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          $$ = GRAMMARP->createTypedef($<fl>3, *$3, $5, refp, $4); }

        // IEEE 1800-2023 6.18.2 typedef with packed dimensions on an existing type identifier
        // Disambiguated from interface array access by requiring ':' inside the brackets
        // (applies to both plain identifiers and type identifiers)
        |       yTYPEDEF id '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_ग्रन्थिः_Range* ध्रुव rangep = new AstRange{$3, $4, $6};
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->createArray(refp, सङ्कलय_अग्र_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        // Same as above but for type identifiers (parameter types, etc.)
        |       yTYPEDEF idType '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']' packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, nullptr, nullptr};
                          Ast_ग्रन्थिः_Range* ध्रुव rangep = new AstRange{$3, $4, $6};
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->createArray(refp, सङ्कलय_अग्र_Null(rangep, $8), true);
                          $$ = GRAMMARP->createTypedef($<fl>9, *$9, $11, dtp, $10); }

        |       yTYPEDEF idAny parameter_मानम्_assignment_Class packed_dimensionListE
        /*cont*/    idAny variable_dimensionListE dtypeAttrListE ';'
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, nullptr, $3};
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->createArray(refp, $4, true);
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
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       dtypeAttrList                           { $$ = $1; }
        ;

dtypeAttrList<nodep>:
                dtypeAttr                               { $$ = $1; }
        |       dtypeAttrList dtypeAttr                 { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

dtypeAttr<nodep>:
                y_VL_लौकिकम्                              { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DT_लौकिकम्}; }
        ;

vl_चिह्नकम्:                          // verilator tag handling
                y_VL_चिह्नकम्                                 { यदि (PARSEP->चिह्नकम्_Nodep()) PARSEP->चिह्नकम्_Nodep()->चिह्नकम्(*$1); }
        ;

//************************************************
// Module Items

module_itemListE<nodep>:        // IEEE: Part of module_declaration
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       module_itemList                         { $$ = $1; }
        ;

module_itemList<nodep>:         // IEEE: Part of module_declaration
                module_item                             { $$ = $1; }
        |       module_itemList module_item             { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

module_item<nodep>:             // ==IEEE: module_item
                पत्तनम्_declaration ';'                    { $$ = $1; }
        |       non_पत्तनम्_module_item                    { $$ = $1; }
        ;

non_पत्तनम्_module_item<nodep>:    // ==IEEE: non_port_module_item
                generate_region                         { $$ = $1; }
                                // not in IEEE, but presumed so can do yBEGIN ... yEND
        |       genItemBegin                            { $$ = $1; }
        |       module_or_generate_item                 { $$ = $1; }
        |       specify_खण्डम्                           { $$ = $1; }
        |       specparam_declaration                   { $$ = $1; }
        |       क्रमादेशः_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: program decls within module decls"); }
        |       module_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: module decls within module decls"); }
        |       interface_declaration
                        { $$ = nullptr; BBUNSUP(CRELINE(), "Unsupported: interface decls within module decls"); }
        |       timeunits_declaration                   { $$ = $1; }
        //                      // Verilator specific
        |       vl_Sc_खण्डम्                               { $$ = $1; }
        |       y_VL_HIER_खण्डम्                          { $$ = new AstPragma{$1, VPragmaType::HIER_खण्डम्}; }
        |       yVL_INLINE_MODULE                       { $$ = new AstPragma{$1, VPragmaType::INLINE_MODULE}; }
        |       yVL_NO_INLINE_MODULE                    { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_MODULE}; }
        |       y_VL_लौकिकम्_MODULE                       { $$ = new AstPragma{$1, VPragmaType::लौकिकम्_MODULE}; v3Global.dpi(true); }
        ;

vl_Sc_खण्डम्<nodep>:  // Verilator-specific `systemc_* blocks
                yaSCHDR     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR, *$1};  }
        |       yaSCHDRP    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::HDR_POST, *$1}; }
        |       yaSCINT     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::INT, *$1}; }
        |       yaSCIMP     { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP, *$1}; }
        |       yaSCIMPH    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::IMP_HDR, *$1}; }
        |       yaSCCTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::CTOR, *$1}; }
        |       yaSCDTOR    { $$ = new AstSystemCSection{$<fl>1, VSystemCSectionType::DTOR, *$1}; }
        ;


module_or_generate_item<nodep>: // ==IEEE: module_or_generate_item
        //                      // IEEE: parameter_override
                yDEFPARAM list_सम्बन्धि_defparam_assignments ';'      { $$ = $2; }
        //                      // IEEE: gate_instantiation + udp_instantiation + module_instantiation
        //                      // not here, see etcInst in module_common_item
        //                      // We joined udp & module definitions, so this goes here
        |       combinational_body                      { $$ = $1; }
        //                      // This module_common_item shared with
        //                      // interface_or_generate_item:module_common_item
        |       module_common_item                      { $$ = $1; }
        ;

module_common_item<nodep>:      // ==IEEE: module_common_item
                module_or_generate_item_declaration     { $$ = $1; }
        //                      // IEEE: interface_instantiation
        //                      // + IEEE: program_instantiation
        //                      // + module_instantiation from module_or_generate_item
        |       etcInst                                 { $$ = $1; }
        |       assertion_item                          { $$ = $1; }
        |       bind_directive                          { $$ = $1; }
        |       continuous_नियोजय                       { $$ = $1; }
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

continuous_नियोजय<nodep>:       // IEEE: continuous_assign
                y_नियोजय driveStrengthE delay_controlE नियोजय_List ';'
                        { $$ = $4;
                          STRENGTH_LIST($4, $2);
                          DELAY_LIST($4, $3); }
        ;

initial_construct<nodep>:       // IEEE: initial_construct
                yINITIAL stmt                      { $$ = new AstInitial{$1, $2}; }
        ;


net_alias<nodep>:               // IEEE: net_alias
                yALIAS variable_lvalue aliasEqList ';'
                        { $2->सङ्कलय_अग्र($3);
                          $$ = new AstAlias{$1, $2}; }
        ;

aliasEqList<ग्रन्थिः_Exprp>:                    // IEEE: part of net_alias
                '=' variable_lvalue                     { $$ = $2; }
        |       aliasEqList '=' variable_lvalue         { $$ = $1->सङ्कलय_अग्र($3); }
        ;

final_construct<nodep>:         // IEEE: final_construct
                yFINAL stmt                        { $$ = new AstFinal{$1, $2}; }
        ;

module_or_generate_item_declaration<nodep>:     // ==IEEE: module_or_generate_item_declaration
                package_or_generate_item_declaration    { $$ = $1; }
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
                yDEFAULT yDISABLE yIFF व्यञ्जकम्/*expression_or_dist*/ ';'
                        { $$ = new AstDefaultDisable{$1, $4}; }
        ;

bind_directive<nodep>:          // ==IEEE: bind_directive + bind_target_scope
        //                      // ';' - Note IEEE grammar is wrong, includes extra ';'
        //                      // - it's already in module_instantiation
        //                      // We merged the rules - id may be a bind_target_instance or
        //                      // module_identifier or interface_identifier
                yBIND bind_target_instance bind_instantiation   { $$ = new AstBind{$<fl>2, *$2, $3}; }
        |       yBIND bind_target_instance ':' bind_target_instance_list bind_instantiation
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: Bind with instance list"); DEL($5); }
        ;

bind_target_instance_list:      // ==IEEE: bind_target_instance_list
                bind_target_instance                    { }
        |       bind_target_instance_list ',' bind_target_instance      { }
        ;

bind_target_instance<strp>:     // ==IEEE: bind_target_instance
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
                yGENERATE ~c~genItemList yENDGENERATE   { $$ = $2; }
        |       yGENERATE yENDGENERATE                  { $$ = nullptr; }
        ;

c_generate_region<nodep>:  // IEEE: generate_region (for checkers)
                BISONPRE_COPY(generate_region,{s/~c~/c_/g})     // {copied}
        ;

i_generate_region<nodep>:  // IEEE: generate_region (for interface)
                BISONPRE_COPY(generate_region,{s/~c~/i_/g})     // {copied}
        ;

generate_खण्डम्_or_null<nodep>:  // IEEE: generate_block_or_null (called from gencase/genif/genfor)
        //      ';'             // is included in
        //                      // IEEE: generate_block
        //                      // Must always return a BEGIN node, or nullptr - see GenFor construction
                ~c~generate_item
                        { $$ = $1 ? (new Ast_Gen_खण्डम्{$1->fileline(), "", $1, true}) : nullptr; }
        |       ~c~genItemBegin                         { $$ = $1; }
        ;

c_generate_खण्डम्_or_null<nodep>:  // IEEE: generate_block_or_null (for checkers)
                BISONPRE_COPY(generate_खण्डम्_or_null,{s/~c~/c_/g})      // {copied}
        ;

genItemBegin<nodep>:            // IEEE: part of generate_block
                yBEGIN ~c~genItemList y_अन्तः              { $$ = new Ast_Gen_खण्डम्{$1, "", $2, false}; }
        |       yBEGIN y_अन्तः                             { $$ = nullptr; }
        |       id yP_COLON__BEGIN yBEGIN ~c~genItemList y_अन्तः अन्तः_Label_E
                        { $$ = new Ast_Gen_खण्डम्{$<fl>1, *$1, $4, false};
                          GRAMMARP->अन्तः_Label($<fl>6, *$1, $6); }
        |       id yP_COLON__BEGIN yBEGIN y_अन्तः अन्तः_Label_E
                        { $$ = new Ast_Gen_खण्डम्{$<fl>1, *$1, nullptr, false};
                          GRAMMARP->अन्तः_Label($<fl>5, *$1, $5); }
        |       yBEGIN ':' idAny ~c~genItemList y_अन्तः अन्तः_Label_E
                        { $$ = new Ast_Gen_खण्डम्{$<fl>3, *$3, $4, false};
                          GRAMMARP->अन्तः_Label($<fl>6, *$3, $6); }
        |       yBEGIN ':' idAny y_अन्तः अन्तः_Label_E
                        { $$ = new Ast_Gen_खण्डम्{$<fl>3, *$3, nullptr, false};
                          GRAMMARP->अन्तः_Label($<fl>5, *$3, $5); }
        ;

c_genItemBegin<nodep>:  // IEEE: part of generate_block (for checkers)
                BISONPRE_COPY(genItemBegin,{s/~c~/c_/g})                // {copied}
        ;

i_genItemBegin<nodep>:  // IEEE: part of generate_block (for interfaces)
                BISONPRE_COPY(genItemBegin,{s/~c~/i_/g})                // {copied}
        ;

genItemOrBegin<nodep>:          // Not in IEEE, but our begin isn't under generate_item
                ~c~generate_item                        { $$ = $1; }
        |       ~c~genItemBegin                         { $$ = $1; }
        ;

c_genItemOrBegin<nodep>:  // (for checkers)
                BISONPRE_COPY(genItemOrBegin,{s/~c~/c_/g})      // {copied}
        ;

i_genItemOrBegin<nodep>:  // (for interfaces)
                interface_item                          { $$ = $1; }
        |       i_genItemBegin                          { $$ = $1; }
        ;

genItemList<nodep>:
                ~c~genItemOrBegin                       { $$ = $1; }
        |       ~c~genItemList ~c~genItemOrBegin        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

c_genItemList<nodep>:  // (for checkers)
                BISONPRE_COPY(genItemList,{s/~c~/c_/g})         // {copied}
        ;

i_genItemList<nodep>:  // (for interfaces)
                BISONPRE_COPY(genItemList,{s/~c~/i_/g})         // {copied}
        ;

generate_item<nodep>:           // IEEE: module_or_interface_or_generate_item
        //                      // Only legal when in a generate under a module (or interface under a module)
                module_or_generate_item                 { $$ = $1; }
        //                      // Only legal when in a generate under an interface
        //UNSUP interface_or_generate_item              { $$ = $1; }
        //                      // IEEE: checker_or_generate_item
        //                      // Only legal when in a generate under a checker
        //                      // so below in c_generate_item
        ;

c_generate_item<nodep>:  // IEEE: generate_item (for checkers)
                checker_or_generate_item                { $$ = $1; }
        ;

conditional_generate_construct<nodep>:  // ==IEEE: conditional_generate_construct
                yCASE '(' व्यञ्जकम्_Type_Compare ')' ~c~case_generate_itemList yENDCASE
                        { $$ = new AstGenCase{$1, $3, $5}; }
        |       yIF '(' व्यञ्जकम्_Type_Compare ')' ~c~generate_खण्डम्_or_null %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = new AstGenIf{$1, $3, $5, nullptr}; }
        |       yIF '(' व्यञ्जकम्_Type_Compare ')' ~c~generate_खण्डम्_or_null yELSE ~c~generate_खण्डम्_or_null
                        { $$ = new AstGenIf{$1, $3, $5, $7}; }
        ;

c_conditional_generate_construct<nodep>:  // IEEE: conditional_generate_construct (for checkers)
                BISONPRE_COPY(conditional_generate_construct,{s/~c~/c_/g})      // {copied}
        ;

loop_generate_construct<nodep>: // ==IEEE: loop_generate_construct
                yFOR '(' genvar_initialization ';' व्यञ्जकम् ';' genvar_iteration ')' ~c~generate_खण्डम्_or_null
                        { // Convert BEGIN(...) to BEGIN(GENFOR(...)), as we need the BEGIN to hide the local genvar
                          Ast_Gen_खण्डम्* lowerp = VN_CAST($9, Gen_खण्डम्);
                          UASSERT_OBJ(!$9 || lowerp, $9, "Child of GENFOR should have been begin");
                          Ast_ग्रन्थिः* ध्रुव itemsp = lowerp && lowerp->itemsp() ? lowerp->itemsp()->unlink_Fr_पृष्ठभागः_With_अग्र() : nullptr;
                          Ast_Gen_खण्डम्* ध्रुव blkp = new Ast_Gen_खण्डम्{$1, lowerp ? lowerp->name() : "", nullptr, true};
                          // V3LinkDot detects BEGIN(GENFOR(...)) as a special case
                          Ast_ग्रन्थिः* initp = $3;
                          Ast_ग्रन्थिः* ध्रुव varp = $3;
                          यदि (VN_IS(varp, Var)) {  // Genvar
                                initp = varp->nextp();
                                initp->unlink_Fr_पृष्ठभागः_With_अग्र();  // Detach 2nd from varp, make 1st init
                                blkp->सङ्कलय_Itemsp(varp);
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
                parse_Ref_आधारः '=' व्यञ्जकम्                     { $$ = new Ast_नियोजय{$2, $1, $3}; }
        |       yGENVAR genvar_identifierDecl '=' const_व्यञ्जकम्
                        { $$ = $2; Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>($$,
                                       new Ast_नियोजय{$3, new AstVarRef{$2->fileline(), $2, VAccess::WRITE}, $4}); }
        ;

genvar_iteration<nodep>:        // ==IEEE: genvar_iteration
                parse_Ref_आधारः '='          व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, $3}; }
        |       parse_Ref_आधारः yP_PLUSEQ    व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new Ast_सङ्कलय{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_MINUSEQ   व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstSub{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_TIMESEQ   व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstMul{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_DIVEQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstDiv{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_MODEQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstModDiv{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_ANDEQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstAnd{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_OREQ      व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstOr{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_XOREQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new AstXor{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_SLEFTEQ   व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new Ast_सरणम्_L{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_SRIGHTEQ  व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new Ast_सरणम्_R{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        |       parse_Ref_आधारः yP_SSRIGHTEQ व्यञ्जकम्
                        { $$ = new Ast_नियोजय{$2, $1, new Ast_सरणम्_RS{$2, $1->clone_वृक्षः_विशुद्धम्(true), $3}}; }
        //                      // inc_or_dec_operator
        |       yP_PLUSPLUS   parse_Ref_आधारः
                        { $$ = new Ast_नियोजय{$1, $2, new Ast_सङ्कलय{$1, $2->clone_वृक्षः_विशुद्धम्(true),
                                                                new AstConst{$1, AstConst::String_गन्तव्यम्_Parse{}, "'b1"}}}; }
        |       yP_MINUSMINUS parse_Ref_आधारः
                        { $$ = new Ast_नियोजय{$1, $2, new AstSub{$1, $2->clone_वृक्षः_विशुद्धम्(true),
                                                                new AstConst{$1, AstConst::String_गन्तव्यम्_Parse{}, "'b1"}}}; }
        |       parse_Ref_आधारः yP_PLUSPLUS
                        { $$ = new Ast_नियोजय{$2, $1, new Ast_सङ्कलय{$2, $1->clone_वृक्षः_विशुद्धम्(true),
                                                                new AstConst{$2, AstConst::String_गन्तव्यम्_Parse{}, "'b1"}}}; }
        |       parse_Ref_आधारः yP_MINUSMINUS
                        { $$ = new Ast_नियोजय{$2, $1, new AstSub{$2, $1->clone_वृक्षः_विशुद्धम्(true),
                                                                new AstConst{$2, AstConst::String_गन्तव्यम्_Parse{}, "'b1"}}}; }
        ;

case_generate_itemList<genCaseItemp>:  // IEEE: { case_generate_itemList }
                ~c~case_generate_item                   { $$ = $1; }
        |       ~c~case_generate_itemList ~c~case_generate_item         { $$ = $1; $1->सङ्कलय_अग्र($2); }
        ;

c_case_generate_itemList<genCaseItemp>:  // IEEE: { case_generate_item } (for checkers)
                BISONPRE_COPY(case_generate_itemList,{s/~c~/c_/g})      // {copied}
        ;

case_generate_item<genCaseItemp>:      // ==IEEE: case_generate_item
                caseCondList colon ~c~generate_खण्डम्_or_null    { $$ = new AstGenCaseItem{$2, $1, $3}; }
        |       yDEFAULT colon ~c~generate_खण्डम्_or_null        { $$ = new AstGenCaseItem{$1, nullptr, $3}; }
        |       yDEFAULT ~c~generate_खण्डम्_or_null              { $$ = new AstGenCaseItem{$1, nullptr, $2}; }
        ;

c_case_generate_item<genCaseItemp>:  // IEEE: case_generate_item (for checkers)
                BISONPRE_COPY(case_generate_item,{s/~c~/c_/g})  // {copied}
        ;

//************************************************
// Assignments and register declarations

नियोजय_List<alwaysp>:
                नियोजय_One                               { $$ = $1; }
        |       नियोजय_List ',' नियोजय_One                { $$ = $1->सङ्कलय_अग्र($3); }
        ;

नियोजय_One<alwaysp>:
                variable_lvalue '=' व्यञ्जकम्                { Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$2, $1, $3};
                                                          $$ = new AstAlways{ap}; }
        ;

delay_or_event_controlE<nodep>:  // IEEE: delay_or_event_control plus empty
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        |       event_control                           { $$ = $1; }
        |       yREPEAT '(' व्यञ्जकम् ')' event_control
                        { $$ = $5; BBUNSUP($1, "Unsupported: repeat event control"); }
        ;

delay_controlE<delayp>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       delay_control                           { $$ = $1; }
        ;

delay_control<delayp>:   //== IEEE: delay_control
                '#' delay_मानम्
                        { $$ = new AstDelay{$<fl>1, $2, false}; }
        |       '#' '(' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; }
        |       '#' '(' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false};
                          $$->fallDelay($5); }
        |       '#' '(' minTypMax ',' minTypMax ',' minTypMax ')'
                        { $$ = new AstDelay{$<fl>1, $3, false}; $$->fallDelay($5); RISEFALLDLYUNSUP($7); DEL($7); }
        ;

delay_मानम्<ग्रन्थिः_Exprp>:         // ==IEEE:delay_value
        //                      // IEEE: ps_identifier
                idClass                                 { $$ = $1; }
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       time_अङ्कः_Adjusted                         { $$ = $1; }
        |       y1_चरणः                                  { $$ = new AstConst{$<fl>1, AstConst::One_चरणः{}}; }
        ;

minTypMax<ग्रन्थिः_Exprp>:           // IEEE: mintypmax_expression and constant_mintypmax_expression
                व्यञ्जकम्                               { $$ = $1; }
        |       व्यञ्जकम् ':' व्यञ्जकम् ':' व्यञ्जकम् { $$ = $3; MINTYPMAXDLYUNSUP($3); DEL($1); DEL($5); }
        ;

minTypMaxE<ग्रन्थिः_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       minTypMax                               { $$ = $1; }
        ;

netSigList<varp>:               // IEEE: list_of_port_identifiers
                netSig                                  { $$ = $1; }
        |       netSigList ',' netSig                   { $$ = $1; $1->सङ्कलय_अग्र($3); }
        ;

netSig<varp>:                   // IEEE: net_decl_assignment -  one element from list_of_port_identifiers
                netId variable_dimensionListE sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        |       netId variable_dimensionListE sigAttrListE '=' व्यञ्जकम्
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          AstDelay* ध्रुव delayp = $$->delayp() ? $$->delayp()->unlink_Fr_पृष्ठभागः() : nullptr;
                          Ast_नियोजय_W* ध्रुव assignp = new Ast_नियोजय_W{$4, new AstParseRef{$<fl>1, *$1}, $5, delayp};
                          यदि (GRAMMARP->m_netStrengthp) assignp->strengthSpecp(GRAMMARP->m_netStrengthp->clone_वृक्षः(false));
                          Ast_ग्रन्थिः::सङ्कलय_अग्र<Ast_ग्रन्थिः, Ast_ग्रन्थिः>($$, new AstAlways{assignp}); }
        ;

netId<strp>:
                id/*new-net*/                           { $$ = $1; $<fl>$ = $<fl>1; }
        |       idSVKwd                                 { $$ = $1; $<fl>$ = $<fl>1; }
        ;

sigAttrScope:
                y_VL_लौकिकम्_FLAT_RW_ON_SNS attr_event_control
                                                        { GRAMMARP->createScopedSigAttr(VAttrType::VAR_लौकिकम्_FLAT_RW);
                                                          v3Global.dpi(true); DEL($2); }
        |       y_VL_लौकिकम्_ON                           { GRAMMARP->createScopedSigAttr(VAttrType::VAR_लौकिकम्); }
        |       y_VL_लौकिकम्_FLAT_ON                      { GRAMMARP->createScopedSigAttr(VAttrType::VAR_लौकिकम्_FLAT); }
        |       y_VL_लौकिकम्_FLAT_RD_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_लौकिकम्_FLAT_RD); }
        |       y_VL_लौकिकम्_FLAT_RW_ON                   { GRAMMARP->createScopedSigAttr(VAttrType::VAR_लौकिकम्_FLAT_RW); }
        |       y_VL_लौकिकम्_OFF                          { GRAMMARP->समुच्चयः_Scoped_Sig_Attr(nullptr); }
        ;

sigAttrListE<nodep>: // Scoped Attributes are added to explicit attributes
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       sigAttrList                             { $$ = $1; }
        ;

sigAttrList<nodep>:
                sigAttr                                 { $$ = $1; }
        |       sigAttrList sigAttr                     { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

sigAttr<nodep>:
                yVL_CLOCKER                             { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_NO_CLOCKER                          { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_CLOCK_ENABLE                        { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_FORCEABLE                           { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FORCEABLE}; }
        |       y_VL_लौकिकम्                              { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_लौकिकम्}; v3Global.dpi(true); }
        |       y_VL_लौकिकम्_FLAT                         { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_लौकिकम्_FLAT}; v3Global.dpi(true); }
        |       y_VL_लौकिकम्_FLAT_RD                      { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_लौकिकम्_FLAT_RD}; v3Global.dpi(true); }
        |       y_VL_लौकिकम्_FLAT_RW attr_event_controlE  { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_लौकिकम्_FLAT_RW}; v3Global.dpi(true); DEL($2); }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = nullptr; /* Historical, now has no effect */ }
        |       yVL_SC_BIGUINT                          { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_SC_BIGUINT}; }
        |       yVL_SC_BV                               { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_SC_BV}; }
        |       yVL_SFORMAT                             { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_SFORMAT}; }
        |       y_VL_विभज_VAR                           { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_विभज_VAR}; }
        |       yVL_FSM_ARC_INCL_COND                   { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO              { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO}; }
        |       yVL_FSM_ARC_INCL_COND_AUTO_EXPAND       { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND}; }
        |       y_VL_FSM_ARC_INCL_COND_पूर्णम्              { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_ARC_INCLUDE_COND_पूर्णम्}; }
        |       yVL_FSM_STATE                           { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_STATE}; }
        |       yVL_FSM_STATE_AUTO                      { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_STATE_AUTO}; }
        |       yVL_FSM_STATE_AUTO_EXPAND               { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_STATE_AUTO_EXPAND}; }
        |       y_VL_FSM_STATE_पूर्णम्                      { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::VAR_FSM_STATE_पूर्णम्}; }
        ;

rangeListE<ग्रन्थिः_Rangep>:         // IEEE: [{packed_dimension}]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       rangeList                               { $$ = $1; }
        ;

rangeList<ग्रन्थिः_Rangep>:          // IEEE: {packed_dimension}
                anyrange                                { $$ = $1; }
        |       rangeList anyrange                      { $$ = $1->सङ्कलय_अग्र($2); }
        ;

anyrange<ग्रन्थिः_Rangep>:
                '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'         { $$ = new AstRange{$1, $2, $4}; }
        ;

part_select_rangeList<ग्रन्थिः_Pre_Selp>:  // IEEE: part_select_range (as used after function calls)
                part_select_range                        { $$ = $1; }
        |       part_select_rangeList part_select_range  { $$ = GRAMMARP->scrubSel($1, $2); }
        ;

part_select_range<ग्रन्थिः_Pre_Selp>:
                '[' व्यञ्जकम् ']'
                        { $$ = new AstSelBit{$1, new AstParseHolder{$1}, $2}; }
        |       '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                        { $$ = new AstSelExtract{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' व्यञ्जकम् yP_PLUSCOLON const_व्यञ्जकम् ']'
                        { $$ = new AstSelPlus{$1, new AstParseHolder{$1}, $2, $4}; }
        |       '[' व्यञ्जकम् yP_MINUSCOLON const_व्यञ्जकम् ']'
                        { $$ = new AstSelMinus{$1, new AstParseHolder{$1}, $2, $4}; }
        ;

packed_dimensionListE<ग्रन्थिः_Rangep>:      // IEEE: [{ packed_dimension }]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       packed_dimensionList                    { $$ = $1; }
        ;

packed_dimensionList<ग्रन्थिः_Rangep>:       // IEEE: { packed_dimension }
                packed_dimension                        { $$ = $1; }
        |       packed_dimensionList packed_dimension   { $$ = $1->सङ्कलय_अग्र($2); }
        ;

packed_dimension<ग्रन्थिः_Rangep>:   // ==IEEE: packed_dimension
                anyrange                                { $$ = $1; }
        |       '[' ']'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: [] dimensions"); }
        ;

//************************************************
// Parameters

param_assignment<varp>:         // ==IEEE: param_assignment
        //                      // IEEE: constant_param_expression
        //                      // constant_param_expression: '$' is in expr
                id/*new-parameter*/ variable_dimensionListE sigAttrListE व्यञ्जकम्_Or_दत्त_Type_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3);
                          यदि ($4) $$->valuep($4);
                          अन्यथा यदि (!GRAMMARP->m_pinAnsi)
                              $$->v3warn(PARAMNODEFAULT, "Parameter without default requires"
                                         " ANSI-style parameter list (IEEE 1800-2023 6.20.1): "
                                         << $$->prettyNameQ()); }
        ;

list_सम्बन्धि_param_assignments<varp>:        // ==IEEE: list_of_param_assignments
                param_assignment                        { $$ = $1; }
        |       list_सम्बन्धि_param_assignments ',' param_assignment  { $$ = $1->सङ्कलय_अग्र($3); }
        ;

type_assignment<varp>:          // ==IEEE: type_assignment
        //                      // note exprOrDataType being a data_type is only for yPARAMETER yTYPE
        //                      // Using exprOrDataType allows hierarchical refs like if0.rq_t
        //                      // which get resolved to types during linking
                idAny/*new-parameter*/ sigAttrListE
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2); }
        |       idAny/*new-parameter*/ sigAttrListE '=' व्यञ्जकम्_Or_दत्त_Type
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          // V3LinkParse will wrap this in RequireDType when creating ParamTypeDType
                          $$->valuep($4); }
        ;

list_सम्बन्धि_type_assignments<varp>:         // ==IEEE: list_of_type_assignments
                type_assignment                         { $$ = $1; }
        |       list_सम्बन्धि_type_assignments ',' type_assignment
                        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

list_सम्बन्धि_defparam_assignments<nodep>:    //== IEEE: list_of_defparam_assignments
                defparam_assignment                     { $$ = $1; }
        |       list_सम्बन्धि_defparam_assignments ',' defparam_assignment
                        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

defparam_assignment<nodep>:     // ==IEEE: defparam_assignment
                defparamIdRangeList '.' defparamIdRange '=' व्यञ्जकम्
                        { $$ = new AstDefParam{$4, $1, *$3, $5}; }
        |       defparamIdRange '=' व्यञ्जकम्
                        { $$ = nullptr; BBUNSUP($2, "Unsupported: defparam with no dot");
                          DEL($3); }
        ;

defparamIdRangeList<ग्रन्थिः_Exprp>:  // IEEE: part of defparam_assignment
                defparam_Id_Range_व्यञ्जकम्                     { $$ = $1; }
        |       defparamIdRangeList '.' defparam_Id_Range_व्यञ्जकम्
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

defparam_Id_Range_व्यञ्जकम्<ग्रन्थिः_Exprp>:  // IEEE: part of defparam_assignment
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
                idInst parameter_मानम्_assignment_Inst_E
        /*mid*/         { INSTPREP($<fl>1, *$1, $2); }
        /*cont*/    instnameList ';'
                        { $$ = $4;
                          GRAMMARP->m_impliedDecl = false;
                          यदि (GRAMMARP->m_instParamp) {
                              VL_DO_CLEAR(GRAMMARP->m_instParamp->delete_वृक्षः(),
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
        |       mpInstnameList ',' mpInstnameParen      { $$ = $1->सङ्कलय_अग्र($3); }
        ;

mpInstnameParen<nodep>:         // Similar to instnameParen, but for modport instantiations which have no parenthesis
                id instRangeListE sigAttrListE          { $$ = VARDONEA($<fl>1, *$1, $2, $3); }
        ;

instnameList<nodep>:
                instnameParen                           { $$ = $1; }
        |       instnameList ',' instnameParen          { $$ = $1->सङ्कलय_अग्र($3); }
        ;

instnameParen<nodep>:
                id instRangeListE '(' instPinListE ')'
                        { $$ = GRAMMARP->createCell($<fl>1, *$1, $4, $2); }
        ;

instnameParenUdpn<nodep>:  // IEEE: part of udp_instance when no name_of_instance
                '(' instPinListE ')'  // When UDP has empty name, unpacked dimensions must not be used
                        { $$ = GRAMMARP->createCell($<fl>1, "", $2, nullptr); }
        ;

instRangeListE<ग्रन्थिः_Rangep>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       instRangeList                           { $$ = $1; }
        ;

instRangeList<ग्रन्थिः_Rangep>:
                instRange                               { $$ = $1; }
        |       instRangeList instRange                 { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

instRange<ग्रन्थिः_Rangep>:
                '[' const_व्यञ्जकम् ']'
                        { $$ = new AstRange{$1, new AstConst{$1, 0}, new AstSub{$1, $2, new AstConst{$1, 1}}, true}; }
        |       '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                        { $$ = new AstRange{$1, $2, $4}; }
        ;

instParamListE<pinp>:
                { GRAMMARP->pin_प्रक्षिप(); } instParamItListE   { $$ = $2; GRAMMARP->pin_निष्कासय(CRELINE()); }
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
                instParamItem                           { $$ = $1; }
        |       instParamItList ',' instParamItem       { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

instPinItListE<pinp>:           // IEEE: list_of_port_connections
                instPinItemE                            { $$ = $1; }
        |       instPinItListE ',' instPinItemE         { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

instParamItem<pinp>:            // IEEE: named_parameter_assignment + empty
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
        |       '.' idAny '(' व्यञ्जकम्_Or_दत्त_Type ')'
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
        |       व्यञ्जकम्_Or_दत्त_Type
                        { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        //UNSUP | exprOrDataType/*expr*/ ':' expr ':' expr
        //UNSUP          { MINTYPMAXDLYUNSUP($1); DEL($1); DEL($5);
        //UNSUP            $$ = new AstPin{FILELINE_OR_CRE($3), PINNUMINC(), "", $3}; }
        ;

instPinItemE<pinp>:             // IEEE: named_port_connection + empty
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
        |       '.' idAny '(' व्यञ्जकम् ')'
                        { $$ = new AstPin{$<fl>2, PINNUMINC(), *$2, $4};
                          $$->svDotName(true); }
        //UNSUP '.' idAny '(' pev_expr ':' expr ')'     { }
        //UNSUP '.' idAny '(' pev_expr ':' expr ':' expr ')' { }
        //
        |       व्यञ्जकम्                                    { $$ = new AstPin{FILELINE_OR_CRE($1), PINNUMINC(), "", $1}; }
        //UNSUP expr ':' expr                           { }
        //UNSUP expr ':' expr ':' expr                  { }
        ;

//************************************************
// EventControl lists

attr_event_controlE<senTreep>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       attr_event_control                      { $$ = $1; }
        ;

attr_event_control<senTreep>:   // ==IEEE: event_control
                '@' '(' event_expression ')'            { $$ = new Ast_Sen_वृक्षः{$1, $3}; }
        |       '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_वृक्षः_Dot_तारकम्($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_वृक्षः_Dot_तारकम्($1); }
        ;

event_control<senTreep>:        // ==IEEE: event_control
        // UNSUP: Needs alignment with IEEE event_control and clocking_event
                '@' '(' '*' ')'                         { $$ = GRAMMARP->create_Sen_वृक्षः_Dot_तारकम्($1); }
        |       '@' '*'                                 { $$ = GRAMMARP->create_Sen_वृक्षः_Dot_तारकम्($1); }
        //                      // IEEE: clocking_event
        |       '@' '(' event_expression ')'            { $$ = new Ast_Sen_वृक्षः{$1, $3}; }
        //                      // IEEE: hierarchical_event_identifier
        //                      // UNSUP below should be idClassSel
        |       '@' senitemVar                          { $$ = new Ast_Sen_वृक्षः{$1, $2}; } /* For events only */
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

event_expression<senItemp>:     // IEEE: event_expression - split over several
        //UNSUP                 // Below are all removed
                senitem                                 { $$ = $1; }
        |       event_expression yOR senitem            { $$ = सङ्कलय_अग्र_Null($1, $3); }
        |       event_expression ',' senitem            { $$ = सङ्कलय_अग्र_Null($1, $3); } /* Verilog 2001 */
        //UNSUP                 // Above are all removed, replace with:
        //UNSUP ev_expr                                 { $$ = $1; }
        //UNSUP event_expression ',' ev_expr %prec yOR  { $$ = addNextNull($1, $3); }
        ;

senitem<senItemp>:              // IEEE: part of event_expression, non-'OR' ',' terms
                senitemEdge                             { $$ = $1; }
        |       व्यञ्जकम्
                        { $$ = new AstSenItem{$<fl>1, VEdgeType::ET_परिवर्तितम्, $1}; }
        |       व्यञ्जकम् yIFF व्यञ्जकम्
                        { $$ = new AstSenItem{$<fl>1, VEdgeType::ET_परिवर्तितम्, $1, $3}; }
        ;

senitemVar<senItemp>:
                idClassSel
                        { $$ = new AstSenItem{$1->fileline(), VEdgeType::ET_परिवर्तितम्, $1}; }
        ;

senitemEdge<senItemp>:          // IEEE: part of event_expression
                yPOSEDGE व्यञ्जकम्
                        { $$ = new AstSenItem{$1, VEdgeType::ET_POSEDGE, $2}; }
        |       yNEGEDGE व्यञ्जकम्
                        { $$ = new AstSenItem{$1, VEdgeType::ET_NEGEDGE, $2}; }
        |       yEDGE व्यञ्जकम्
                        { $$ = new AstSenItem{$1, VEdgeType::ET_BOTHEDGE, $2}; }
        |       yPOSEDGE व्यञ्जकम् yIFF व्यञ्जकम्
                        { $$ = new AstSenItem{$1, VEdgeType::ET_POSEDGE, $2, $4}; }
        |       yNEGEDGE व्यञ्जकम् yIFF व्यञ्जकम्
                        { $$ = new AstSenItem{$1, VEdgeType::ET_NEGEDGE, $2, $4}; }
        |       yEDGE व्यञ्जकम् yIFF व्यञ्जकम्
                        { $$ = new AstSenItem{$1, VEdgeType::ET_BOTHEDGE, $2, $4}; }
        ;

//************************************************
// Statements

seq_खण्डम्<beginp>:               // ==IEEE: seq_block
        //                      // IEEE doesn't allow declarations in unnamed blocks, but several simulators do.
        //                      // So need AstBegin's even if unnamed to scope variables down
                yBEGIN startLabelE खण्डम्_Decl_List_E stmtListE y_अन्तः अन्तः_Label_E
                        {
                            $$ = new AstBegin{$1, $2 ? *$2 : "", nullptr, false};
                            GRAMMARP->अन्तः_Label($<fl>6, $$, $6);
                            $$->सङ्कलय_Declsp($3);
                            $$->सङ्कलय_Stmtsp($4);
                        }
        ;

seq_खण्डम्_Pre_Id<beginp>:          // IEEE: seq_block, but called with leading ID
                id yP_COLON__BEGIN yBEGIN खण्डम्_Decl_List_E stmtListE y_अन्तः अन्तः_Label_E
                        {
                            $$ = new AstBegin{$3, *$1, nullptr, false};
                            GRAMMARP->अन्तः_Label($<fl>7, $$, $7);
                            $$->सङ्कलय_Declsp($4);
                            $$->सङ्कलय_Stmtsp($5);
                        }
        ;

par_खण्डम्_Join<joinType>:
                yJOIN       { $$ = VJoinType::JOIN; }
        |       yJOIN_ANY   { $$ = VJoinType::JOIN_ANY; }
        |       yJOIN_NONE  { $$ = VJoinType::JOIN_NONE; }
        ;

par_खण्डम्<forkp>:               // ==IEEE: par_block
                yFORK startLabelE खण्डम्_Decl_List_E stmtListE par_खण्डम्_Join अन्तः_Label_E
                        {
                            $$ = new AstFork{$1, $5, $2 ? *$2 : ""};
                            GRAMMARP->अन्तः_Label($<fl>6, $$, $6);
                            $$->सङ्कलय_Declsp($3);
                            $$->सङ्कलय_Forksp(V3ParseGrammar::wrap_अन्तर्_Begin($4));
                        }
        ;

par_खण्डम्_Pre_Id<forkp>:          // ==IEEE: par_block but called with leading ID
                id yP_COLON__FORK yFORK खण्डम्_Decl_List_E stmtListE par_खण्डम्_Join अन्तः_Label_E
                        {
                            $$ = new AstFork{$3, $6, *$1};
                            GRAMMARP->अन्तः_Label($<fl>7, $$, $7);
                            $$->सङ्कलय_Declsp($4);
                            $$->सङ्कलय_Forksp(V3ParseGrammar::wrap_अन्तर्_Begin($5));
                        }
            ;

खण्डम्_Decl_List_E<nodep>:      // IEEE: [ block_item_declaration ]
                /*empty*/                               { $$ = nullptr; }
        |       खण्डम्_Decl_List_E खण्डम्_item_declaration   { $$ = सङ्कलय_अग्र_Null($1, $2); }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

खण्डम्_item_declaration<nodep>:  // ==IEEE: block_item_declaration
                दत्त_declaration                        { $$ = $1; }
        |       parameter_declaration ';'               { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

stmtListE<ग्रन्थिः_Stmtp>:
                /*empty*/                               { $$ = nullptr; }
        |       stmtList                                { $$ = $1; }
        ;

stmtList<ग्रन्थिः_Stmtp>:
                stmt                               { $$ = $1; }
        |       stmtList stmt                      { $$ = सङ्कलय_अग्र_Null($1, $2); }
        //
        |       stmtList error ';'                      { $$ = $1; }  // LCOV_EXCL_LINE
        ;

stmt<ग्रन्थिः_Stmtp>:                    // IEEE: statement + statement_or_null + seq_block + par_block
                statement_item                          { $$ = $1; }
        //                      // S05 block creation rule
        |       id/*block_identifier*/ ':' statement_item       { $$ = new AstBegin{$<fl>1, *$1, $3, false}; }
        //                      // from _or_null
        |       ';'                                     { $$ = nullptr; }
        //                      // labeled par_block/seq_block with leading ':'
        |       seq_खण्डम्_Pre_Id                          { $$ = $1; }
        |       par_खण्डम्_Pre_Id                          { $$ = $1; }
        ;

statement_item<ग्रन्थिः_Stmtp>:          // IEEE: statement_item
        //                      // IEEE: operator_assignment
                foperator_assignment ';'                { $$ = $1; }
        //
        //                      // IEEE: blocking_assignment
        //                      // 1800-2009 restricts LHS of assignment to new to not have a range
        //                      // This is ignored to avoid conflicts
        |       fexprLvalue yP_EQ__NEW dynamic_array_new ';'   { $$ = new Ast_नियोजय{$2, $1, $3}; }
        |       fexprLvalue yP_EQ__NEW class_new ';'    { $$ = new Ast_नियोजय{$2, $1, $3}; }
        //                      // IEEE: inc_or_dec_expression
        |       finc_or_dec_expression ';'              { $$ = new Ast_Stmt_व्यञ्जकम्{$<fl>1, $1}; }
        //
        //                      // IEEE: nonblocking_assignment
        |       fexprLvalue yP_LTE delay_or_event_controlE व्यञ्जकम् ';'
                        { $$ = new Ast_नियोजय_Dly{$2, $1, $4, $3}; }
        //                      // IEEE: clocking_drive ';'
        |       fexprLvalue yP_LTE आवृत्तिः_delay व्यञ्जकम् ';'
                        { $$ = new Ast_नियोजय_Dly{$2, $1, $4, $3}; }
        //UNSUP cycle_delay fexprLvalue yP_LTE ';'      { UNSUP }
        |       y_नियोजय variable_lvalue '=' delay_or_event_controlE व्यञ्जकम् ';'
                        { $$ = new Ast_नियोजय_Cont{$1, $2, $5, $4};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.समुच्चयः_Has_नियोजय_Deassign(); }
        |       yDEASSIGN variable_lvalue ';'
                        { $$ = new AstDeassign{$1, $2};
                          $1->v3warn(IEEEMAYDEPRECATE, "Feature may be deprecated in future IEEE standard"); v3Global.समुच्चयः_Has_नियोजय_Deassign(); }
        |       yFORCE variable_lvalue '=' व्यञ्जकम् ';'
                        { $$ = new Ast_नियोजय_Force{$1, $2, $4}; v3Global.समुच्चयः_Has_Forceable_सङ्केताः(); }
        |       yRELEASE variable_lvalue ';'
                        { $$ = new AstRelease{$1, $2}; v3Global.समुच्चयः_Has_Forceable_सङ्केताः(); }
        //
        //                      // IEEE: case_statement
        |       unique_प्राथमिकता_E caseStart caseAttrE case_itemList yENDCASE
                        { $$ = $2; यदि ($4) $2->सङ्कलय_Itemsp($4);
                          यदि ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          यदि ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          यदि ($1 == uniq_प्राथमिकता) $2->प्राथमिकता_Pragma(true); }
        // case matches uses patterns, not expressions
        |       unique_प्राथमिकता_E caseStart caseAttrE yMATCHES case_matches_itemList yENDCASE
                        { $$ = $2; यदि ($5) $2->सङ्कलय_Itemsp($5);
                          $2->case_Matches_समुच्चयः();
                          यदि ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          यदि ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          यदि ($1 == uniq_प्राथमिकता) $2->प्राथमिकता_Pragma(true); }
        |       unique_प्राथमिकता_E caseStart caseAttrE yINSIDE case_inside_itemList yENDCASE
                        { $$ = $2; यदि ($5) $2->सङ्कलय_Itemsp($5);
                          यदि (!$2->caseSimple()) $4->v3error("Illegal to have inside on a casex/casez");
                          $2->case_Inside_समुच्चयः();
                          यदि ($1 == uniq_UNIQUE) $2->uniquePragma(true);
                          यदि ($1 == uniq_UNIQUE0) $2->unique0Pragma(true);
                          यदि ($1 == uniq_प्राथमिकता) $2->प्राथमिकता_Pragma(true); }
        //
        //                      // IEEE: conditional_statement
        |       unique_प्राथमिकता_E yIF '(' व्यञ्जकम् ')' stmt     %prec pr_अधस्तनम्_THAN_ELSE
                        { AstIf* ध्रुव newp = new AstIf{$2, $4,
                              PARSEP->new_खण्डम्($2, $6)};
                          $$ = newp;
                          यदि ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          यदि ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          यदि ($1 == uniq_प्राथमिकता) newp->प्राथमिकता_Pragma(true); }
        |       unique_प्राथमिकता_E yIF '(' व्यञ्जकम् ')' stmt yELSE stmt
                        { AstIf* ध्रुव newp = new AstIf{$2, $4,
                              PARSEP->new_खण्डम्($2, $6),
                              PARSEP->new_खण्डम्($2, $8)};
                          $$ = newp;
                          यदि ($1 == uniq_UNIQUE) newp->uniquePragma(true);
                          यदि ($1 == uniq_UNIQUE0) newp->unique0Pragma(true);
                          यदि ($1 == uniq_प्राथमिकता) newp->प्राथमिकता_Pragma(true); }
        //
        //                      // IEEE: subroutine_call_statement
        //                      // IEEE says we then expect a function call
        //                      // (function_subroutine_callNoMethod), but rest of
        //                      // the code expects an AstTask when used as a statement,
        //                      // so parse as if task
        //                      // Alternative would be shim with new AstVoidStmt.
        |       yVOID yP_TICK '(' task_subroutine_call_No_विधानम् ')' ';'
                        { Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = $4;
                          Ast_ग्रन्थिः* callp = exprp;
                          यावत् (AstDot* ध्रुव dotp = VN_CAST(callp, Dot)) callp = dotp->rhsp();
                          File_पङ्क्तिः* ध्रुव newfl = new File_पङ्क्तिः{callp->fileline()};
                          newfl->warnOff(V3_Error_संहिता::IGNOREDRETURN, true);
                          callp->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' व्यञ्जकम् '.' task_subroutine_call_No_विधानम् ')' ';'
                        { Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = new AstDot{$5, false, $4, $6};
                          File_पङ्क्तिः* ध्रुव newfl = new File_पङ्क्तिः{$6->fileline()};
                          newfl->warnOff(V3_Error_संहिता::IGNOREDRETURN, true);
                          $6->fileline(newfl);
                          $$ = exprp->makeStmt(); }
        |       yVOID yP_TICK '(' system_f_only_व्यञ्जकम्_call ')' ';'
                        { $$ = new Ast_Stmt_व्यञ्जकम्{$<fl>4, $4};
                          File_पङ्क्तिः* ध्रुव newfl = new File_पङ्क्तिः{$$->fileline()};
                          newfl->warnOff(V3_Error_संहिता::IGNOREDRETURN, true);
                          $$->fileline(newfl); }
        //                      // Any system function as a task
        |       yVOID yP_TICK '(' system_f_or_t_व्यञ्जकम्_call ')' ';'
                        { $$ = new Ast_Stmt_व्यञ्जकम्{$<fl>4, $4};
                          File_पङ्क्तिः* ध्रुव newfl = new File_पङ्क्तिः{$$->fileline()};
                          newfl->warnOff(V3_Error_संहिता::IGNOREDRETURN, true);
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
                          PARSEP->import_If_अन्तर्_Std($1, "process", true);
                        }
        //                      // IEEE: event_trigger
        |       yP_MINUSGT व्यञ्जकम् ';'
                        { $$ = new AstFireEvent{$1, $2, false}; }
        |       yP_MINUSGTGT delay_or_event_controlE व्यञ्जकम् ';'
                        { $$ = new AstFireEvent{$1, $3, true}; }
        //
        // do/for/forever/while loops all modelled as AstLoop
        |       yDO stmt yWHILE '(' व्यञ्जकम् ')' ';'
                        { AstLoop* ध्रुव loopp = new AstLoop{$1, $2};
                          loopp->सङ्कलय_Contsp(new AstLoopTest{$<fl>5, loopp, $5});
                          $$ = loopp; }
        |       yFOR  '(' { VARRESET_NONLIST(UNKNOWN); } for_initializationE ';' व्यञ्जकम्_E ';' for_चरणः_E ')' stmt
                        { AstBegin* ध्रुव blockp = new AstBegin{$1, "", $4, true};
                          AstLoop* ध्रुव loopp = new AstLoop{$1};
                          यदि ($6) loopp->सङ्कलय_Stmtsp(new AstLoopTest{$<fl>6, loopp, $6});
                          loopp->सङ्कलय_Stmtsp($10);
                          loopp->सङ्कलय_Contsp($8);
                          blockp->सङ्कलय_Stmtsp(loopp);
                          $$ = blockp; }
        |       yFOREVER stmt
                        { AstLoop* ध्रुव loopp = new AstLoop{$1, $2};
                          $$ = loopp; }
        |       yWHILE '(' व्यञ्जकम् ')' stmt
                        { AstLoop* ध्रुव loopp = new AstLoop{$1};
                          loopp->सङ्कलय_Stmtsp(new AstLoopTest{$<fl>3, loopp, $3});
                          loopp->सङ्कलय_Stmtsp($5);
                          $$ = loopp; }
        // Other loop statements
        |       yREPEAT '(' व्यञ्जकम् ')' stmt          { $$ = new AstRepeat{$1, $3, $5}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM and 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' stmt
                        { $$ = new AstBegin{$1, "", new AstForeach{$1, $3, $5}, true}; }
        //
        //                      // IEEE: jump_statement
        |       yRETURN ';'                             { $$ = new AstReturn{$1}; }
        |       yRETURN व्यञ्जकम् ';'                        { $$ = new AstReturn{$1, $2}; }
        |       yBREAK ';'                              { $$ = new AstBreak{$1}; }
        |       yCONTINUE ';'                           { $$ = new AstContinue{$1}; }
        //
        |       par_खण्डम्                               { $$ = $1; }
        //                      // IEEE: procedural_timing_control_statement + procedural_timing_control
        |       delay_control stmt
                        { Ast_ग्रन्थिः_Stmt* nextp = nullptr;
                          यदि ($2) {
                              यदि ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_पृष्ठभागः_With_अग्र(), ग्रन्थिः_Stmt);
                              $1->सङ्कलय_Stmtsp($2);
                          }
                          $$ = $1;
                          सङ्कलय_अग्र_Null($$, nextp); }
        |       event_control stmt
                        { Ast_ग्रन्थिः_Stmt* nextp = nullptr;
                          यदि ($2 && $2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_पृष्ठभागः_With_अग्र(), ग्रन्थिः_Stmt);
                          $$ = new AstEventControl{FILELINE_OR_CRE($1), $1, $2};
                          सङ्कलय_अग्र_Null($$, nextp); }
        |       आवृत्तिः_delay stmt
                        { Ast_ग्रन्थिः_Stmt* nextp = nullptr;
                          यदि ($2) {
                              यदि ($2->nextp()) nextp = VN_AS($2->nextp()->unlink_Fr_पृष्ठभागः_With_अग्र(), ग्रन्थिः_Stmt);
                              $1->सङ्कलय_Stmtsp($2);
                          }
                          $$ = $1;
                          सङ्कलय_अग्र_Null($$, nextp); }
        |       seq_खण्डम्                               { $$ = $1; }
        //
        //                      // IEEE: wait_statement
        |       yWAIT '(' व्यञ्जकम् ')' stmt            { $$ = new AstWait{$1, $3, $5}; }
        |       yWAIT yFORK ';'                         { $$ = new AstWaitFork{$1}; }
        //                      // action_block expanded here
        |       y_WAIT_क्रमः '(' vrdList ')' stmt %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5); }
        |       y_WAIT_क्रमः '(' vrdList ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $5, $7);}
        |       y_WAIT_क्रमः '(' vrdList ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($4, "Unsupported: wait_order"); DEL($3, $6); }
        //
        //                      // IEEE: procedural_assertion_statement
        |       procedural_assertion_statement          { $$ = $1; }
        //
        |       randsequence_statement                  { $$ = $1; }
        //
        //                      // IEEE: randcase_statement
        |       yRANDCASE rand_case_itemList yENDCASE   { $$ = new AstRandCase{$1, $2}; }
        //
        //                      // IEEE: expect_property_statement
        //                      // action_block expanded here
        |       yEXPECT '(' property_spec ')' stmt %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5); }
        |       yEXPECT '(' property_spec ')' stmt yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $5, $7); }
        |       yEXPECT '(' property_spec ')' yELSE stmt
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: expect"); DEL($3, $6); }
        ;

statementVerilatorPragmas<pragmap>:
                y_VL_COVERAGE_खण्डम्_OFF
                        { $$ = new AstPragma{$1, VPragmaType::COVERAGE_खण्डम्_OFF}; }
        |       yVL_UNROLL_DISABLE
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_DISABLE}; }
        |       y_VL_UNROLL_पूर्णम्
                        { $$ = new AstPragma{$1, VPragmaType::UNROLL_पूर्णम्}; }
        ;

foperator_assignment<ग्रन्थिः_Stmtp>:    // IEEE: operator_assignment (for first part of expression)
                fexprLvalue '=' delay_or_event_controlE व्यञ्जकम्    { $$ = new Ast_नियोजय{$2, $1, $4, $3}; }
        //
        |       fexprLvalue yP_PLUSEQ    व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::सङ्कलय_2, $2, $1, $3}; }
        |       fexprLvalue yP_MINUSEQ   व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::Sub, $2, $1, $3}; }
        |       fexprLvalue yP_TIMESEQ   व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::Mul, $2, $1, $3}; }
        |       fexprLvalue yP_DIVEQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::Div, $2, $1, $3}; }
        |       fexprLvalue yP_MODEQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::ModDiv, $2, $1, $3}; }
        |       fexprLvalue yP_ANDEQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::And, $2, $1, $3}; }
        |       fexprLvalue yP_OREQ      व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::Or, $2, $1, $3}; }
        |       fexprLvalue yP_XOREQ     व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::Xor, $2, $1, $3}; }
        |       fexprLvalue yP_SLEFTEQ   व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::सरणम्_L, $2, $1, $3}; }
        |       fexprLvalue yP_SRIGHTEQ  व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::सरणम्_R, $2, $1, $3}; }
        |       fexprLvalue yP_SSRIGHTEQ व्यञ्जकम्
                        { $$ = new Ast_नियोजय_Compound{Ast_नियोजय_Compound::operation::सरणम्_RS, $2, $1, $3}; }
        ;

inc_or_dec_expression<ग्रन्थिः_Exprp>:   // ==IEEE: inc_or_dec_expression
        //                      // Need fexprScope instead of variable_lvalue to prevent conflict
                ~l~व्यञ्जकम्_Scope yP_PLUSPLUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostInc{$2, $1}; }
        |       ~l~व्यञ्जकम्_Scope yP_MINUSMINUS
                        { $<fl>$ = $<fl>1; $$ = new AstPostDec{$2, $1}; }
        //                      // Need expr instead of variable_lvalue to prevent conflict
        |       yP_PLUSPLUS     व्यञ्जकम्
                        { $<fl>$ = $<fl>1; $$ = new AstPreInc{$1, $2}; }
        |       yP_MINUSMINUS   व्यञ्जकम्
                        { $<fl>$ = $<fl>1; $$ = new AstPreDec{$1, $2}; }
        ;

finc_or_dec_expression<ग्रन्थिः_Exprp>:  // ==IEEE: inc_or_dec_expression
                BISONPRE_COPY(inc_or_dec_expression,{s/~l~/f/g})        // {copied}
        ;

sinc_or_dec_expression<ग्रन्थिः_Exprp>:  // IEEE: inc_or_dec_expression (for sequence_expression)
                BISONPRE_COPY(inc_or_dec_expression,{s/~l~/s/g})        // {copied}
        ;

pinc_or_dec_expression<ग्रन्थिः_Exprp>:  // IEEE: inc_or_dec_expression (for property_expression)
                BISONPRE_COPY(inc_or_dec_expression,{s/~l~/p/g})        // {copied}
        ;

//UNSUPev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for ev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/ev_/g})      // {copied}
//UNSUP ;

//UNSUPpev_inc_or_dec_expression<nodeExprp>:  // IEEE: inc_or_dec_expression (for pev_expr)
//UNSUP         BISONPRE_COPY(inc_or_dec_expression,{s/~l~/pev_/g})     // {copied}
//UNSUP ;

class_new<ग्रन्थिः_Exprp>:    // IEEE: class_new
        //                      // See V3ParseImp::tokenPipeScanEqNew that searches for '=' ... yNEW__LEX
                class_newNoScope
                        { $$ = $1; }
        //                      // Special precedence so (...) doesn't match expr
        //                      // A scope is not legal in front of a AstNewCopy
        |       packageClassScopeNoId yNEW__ETC
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2,  nullptr, true}); }
        |       packageClassScopeNoId yNEW__PAREN '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstNew{$2, $4, true}); }
        ;

class_newNoScope<ग्रन्थिः_Exprp>:    // IEEE: class_new but no packageClassScope
        //                      // Special precedence so (...) doesn't match expr
                yNEW__ETC                               { $$ = new AstNew{$1}; }
        |       yNEW__ETC व्यञ्जकम्                          { $$ = new AstNewCopy{$1, $2}; }
        |       yNEW__PAREN '(' list_सम्बन्धि_arguments_E ')'  { $$ = new AstNew{$1, $3}; }
        ;

dynamic_array_new<ग्रन्थिः_Exprp>:   // ==IEEE: dynamic_array_new
                yNEW__ETC '[' व्यञ्जकम् ']'                  { $$ = new AstNewDynamic{$1, $3, nullptr}; }
        |       yNEW__ETC '[' व्यञ्जकम् ']' '(' व्यञ्जकम् ')'     { $$ = new AstNewDynamic{$1, $3, $6}; }
        ;

//************************************************
// Case/If

unique_प्राथमिकता_E<uniqstate>:    // IEEE: unique_priority + empty
                /*empty*/                               { $$ = uniq_NONE; }
        |       y_प्राथमिकता                               { $$ = uniq_प्राथमिकता; }
        |       yUNIQUE                                 { $$ = uniq_UNIQUE; }
        |       yUNIQUE0                                { $$ = uniq_UNIQUE0; }
        ;

caseStart<casep>:               // IEEE: part of case_statement
                yCASE  '(' व्यञ्जकम्_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASE, $3, nullptr}; }
        |       yCASEX '(' व्यञ्जकम्_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEX, $3, nullptr}; }
        |       yCASEZ '(' व्यञ्जकम्_Type_Compare ')'
                        { $$ = GRAMMARP->m_caseAttrp = new AstCase{$1, VCaseType::CT_CASEZ, $3, nullptr}; }
        ;

caseAttrE:
                /*empty*/                               { }
        |       caseAttrE y_VL_पूर्णम्_CASE                 { GRAMMARP->m_caseAttrp->पूर्णम्_Pragma(true); }
        |       caseAttrE yVL_PARALLEL_CASE             { GRAMMARP->m_caseAttrp->parallelPragma(true); }
        ;

case_itemList<caseItemp>:       // IEEE: { case_item + ... }
                caseCondList colon stmt                    { $$ = new AstCaseItem{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new AstCaseItem{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new AstCaseItem{$1, nullptr, $2}; }
        |       case_itemList caseCondList colon stmt      { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$3, $2, $4}); }
        |       case_itemList yDEFAULT stmt                { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$2, nullptr, $3}); }
        |       case_itemList yDEFAULT colon stmt          { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$2, nullptr, $4}); }
        ;

case_inside_itemList<caseItemp>:        // IEEE: { case_inside_item + range_list ... }
                range_list colon stmt                      { $$ = new AstCaseItem{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new AstCaseItem{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new AstCaseItem{$1, nullptr, $2}; }
        |       case_inside_itemList range_list colon stmt { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$3, $2, $4}); }
        |       case_inside_itemList yDEFAULT stmt         { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$2, nullptr, $3}); }
        |       case_inside_itemList yDEFAULT colon stmt   { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$2, nullptr, $4}); }
        ;

case_matches_itemList<caseItemp>:    // IEEE: { case_pattern_item + ... }
        //                      // IEEE: case_pattern_item ::= pattern [&&& expr] : stmt
        //                      // pattern includes expr for tagged void members (tagged id)
                आदर्शः_No_व्यञ्जकम् colon stmt                   { $$ = new AstCaseItem{$2, $1, $3}; }
        |       व्यञ्जकम् colon stmt                            { $$ = new AstCaseItem{$2, $1, $3}; }
        |       yDEFAULT colon stmt                        { $$ = new AstCaseItem{$1, nullptr, $3}; }
        |       yDEFAULT stmt                              { $$ = new AstCaseItem{$1, nullptr, $2}; }
        |       case_matches_itemList आदर्शः_No_व्यञ्जकम् colon stmt
                        { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$3, $2, $4}); }
        |       case_matches_itemList व्यञ्जकम् colon stmt
                        { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$3, $2, $4}); }
        |       case_matches_itemList yDEFAULT stmt        { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$2, nullptr, $3}); }
        |       case_matches_itemList yDEFAULT colon stmt  { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$2, nullptr, $4}); }
        ;

rand_case_itemList<caseItemp>:       // IEEE: { rand_case_item + ... }
        //                      // Randcase syntax doesn't have default, or expression lists
                व्यञ्जकम् colon stmt                            { $$ = new AstCaseItem{$2, $1, $3}; }
        |       rand_case_itemList व्यञ्जकम् colon stmt         { $$ = $1->सङ्कलय_अग्र(new AstCaseItem{$3, $2, $4}); }
        ;

range_list<ग्रन्थिः_Exprp>:     // ==IEEE: range_list/open_range_list + value_range/open_value_range
                मानम्_range                             { $$ = $1; }
        |       range_list ',' मानम्_range              { $$ = $1->सङ्कलय_अग्र($3); }
        ;

मानम्_range<ग्रन्थिः_Exprp>:         // ==IEEE: value_range/open_value_range
                व्यञ्जकम्                                    { $$ = $1; }
        |       '[' व्यञ्जकम् ':' व्यञ्जकम् ']'                   { $$ = new AstInsideRange{$1, $2, $4}; }
        //                      // IEEE-2023: added all four:
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' '$' ':' expr ']'
        //                      // Skipped as '$' is part of our expr
        //                      // IEEE-2023: '[' expr ':' '$' ']'
        |       '[' व्यञ्जकम् yP_PLUSSLASHMINUS व्यञ्जकम् ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +/- range"); DEL($2, $4); }
        |       '[' व्यञ्जकम् yP_PLUSPCTMINUS व्यञ्जकम् ']'
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: +%- range"); DEL($2, $4); }
        ;

covergroup_मानम्_range<ग्रन्थिः_Exprp>:  // ==IEEE-2012: covergroup_value_range
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

caseCondList<ग्रन्थिः_Exprp>:        // IEEE: part of case_item
                व्यञ्जकम्_Type_Compare                         { $$ = $1; }
        |       caseCondList ',' व्यञ्जकम्_Type_Compare        { $$ = $1->सङ्कलय_अग्र($3); }
        ;

आदर्शः_No_व्यञ्जकम्<ग्रन्थिः_Exprp>:       // IEEE: pattern **Excluding Expr*
                '.' idAny/*variable*/
                        { $$ = new Ast_आदर्शः_Var{$1, *$2}; }
        |       yP_DOTSTAR
                        { $$ = new Ast_आदर्शः_तारकम्{$1}; }
        //                      // IEEE: "expr" excluded; expand in callers
        //                      // IEEE: tagged member_identifier [ pattern ]
        //                      // Standalone "yTAGGED__NONPRIMARY idAny" is handled via expr in patternOne
        //                      // Here, we need to treat yTAGGED and yTAGGED__NONPRIMARY identically.
        |       yTAGGED idAny/*member_identifier*/ आदर्शः_No_व्यञ्जकम्
                        { $$ = new Ast_Tagged_आदर्शः{$1, *$2, $3}; }
        |       yTAGGED__NONPRIMARY idAny/*member_identifier*/ आदर्शः_No_व्यञ्जकम्
                        { $$ = new Ast_Tagged_आदर्शः{$1, *$2, $3}; }
        //                      // "yP_TICKBRA patternList '}'" part of expr under assignment_pattern
        ;

आदर्शः_List<nodep>:             // IEEE: part of pattern
                आदर्शः_One                              { $$ = $1; }
        |       आदर्शः_List ',' आदर्शः_One              { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

आदर्शः_One<nodep>:              // IEEE: part of pattern
                व्यञ्जकम्
                        { यदि ($1) $$ = new AstPatMember{$1->fileline(), $1, nullptr, nullptr}; अन्यथा $$ = nullptr; }
        |       व्यञ्जकम् '{' तर्काः_व्यञ्जकम्_List '}'               { $$ = new AstPatMember{$2, $3, nullptr, $1}; }
        |       आदर्शः_No_व्यञ्जकम्                           { $$ = $1; }
        ;

आदर्शः_Member_List<nodep>:       // IEEE: part of pattern and assignment_pattern
                आदर्शः_Member_One                        { $$ = $1; }
        |       आदर्शः_Member_List ',' आदर्शः_Member_One  { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

आदर्शः_Member_One<patMemberp>:   // IEEE: part of pattern and assignment_pattern
                आदर्शः_कुञ्चिका ':' व्यञ्जकम्                     { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        |       आदर्शः_कुञ्चिका ':' आदर्शः_No_व्यञ्जकम्            { $$ = new AstPatMember{$1->fileline(), $3, $1, nullptr}; }
        //                      // From assignment_pattern_key
        |       yDEFAULT ':' व्यञ्जकम्                       { $$ = new AstPatMember{$1, $3, nullptr, nullptr}; $$->isDefault(true); }
        |       yDEFAULT ':' आदर्शः_No_व्यञ्जकम्              { AstPatMember* ध्रुव patp = new AstPatMember{$1, $3, nullptr, nullptr}; patp->isDefault(true); $$ = patp; }
        ;

आदर्शः_कुञ्चिका<nodep>:              // IEEE: merge structure_pattern_key, array_pattern_key, assignment_pattern_key
        //                      // IEEE: structure_pattern_key: member_identifier | assignment_pattern_key
        //                      // IEEE: array_pattern_key: constant_expression | assignment_pattern_key
        //                      // Verilator:
        //                      //   A bare "foo" is ambiguous here, as it may be a constant
        //                      //   identifier (if array) or a reference to the "foo" member
        //                      //   (if structure).  Both spell the same in expr, so the
        //                      //   bare-identifier case becomes a Text node and V3LinkDot
        //                      //   resolves which one it is.
                व्यञ्जकम्
                        { $$ = GRAMMARP->create_आदर्शः_कुञ्चिका($1); }
        //                      // IEEE: assignment_pattern_key
        |       simple_typeNoRef
                        { $$ = $1; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // (simple_type ps_parameter_identifier is part of expr above)
        |       packageClassScopeE idType
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = refp; }
        ;

assignment_आदर्शः<patternp>:   // ==IEEE: assignment_pattern
        // This doesn't match the text of the spec.  I think a : is missing, or example code needed
        // yP_TICKBRA constExpr exprList '}'    { $$="'{"+$2+" "+$3"}"; }
        //                      // "'{ const_expression }" is same as patternList with one entry
        //                      // From patternNoExpr
        //                      // also IEEE: "''{' expression { ',' expression } '}'"
        //                      //      matches since patternList includes expr
                yP_TICKBRA आदर्शः_List '}'              { $$ = new Ast_आदर्शः{$1, $2}; }
        //                      // From patternNoExpr
        //                      // also IEEE "''{' structure_pattern_key ':' ...
        //                      // also IEEE "''{' array_pattern_key ':' ...
        |       yP_TICKBRA आदर्शः_Member_List '}'        { $$ = new Ast_आदर्शः{$1, $2}; }
        //                      // IEEE: Not in grammar, but in VMM
        |       yP_TICKBRA '}'                          { $$ = new Ast_आदर्शः{$1, nullptr}; }
        ;

// "datatype id = x {, id = x }"  |  "yaId = x {, id=x}" is legal
for_initializationE<nodep>:      // ==IEEE: for_initialization + for_variable_declaration
                /*रिक्तानि*/                     { $$ = nullptr; }
        |       for_initializationItemList      { $$ = $1; }
        ;

for_initializationItemList<nodep>:      // IEEE: [for_variable_declaration...]
                for_initializationItem                  { $$ = $1; }
        |       for_initializationItemList ',' for_initializationItem   { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

for_initializationItem<nodep>:          // IEEE: variable_assignment + for_variable_declaration
        //                      // IEEE: for_variable_declaration
                दत्त_type idAny/*new*/ '=' व्यञ्जकम्
                        { VARRESET_NONLIST(VAR); VARDTYPE($1);
                          AstVar* ध्रुव varp = VARDONEA($<fl>2, *$2, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->सङ्कलय_अग्र(new Ast_नियोजय{$3, new AstParseRef{$<fl>2, *$2}, $4}); }
        //                      // IEEE-2012:
        |       yVAR दत्त_type idAny/*new*/ '=' व्यञ्जकम्
                        { VARRESET_NONLIST(VAR); VARDTYPE($2);
                          AstVar* ध्रुव varp = VARDONEA($<fl>3, *$3, nullptr, nullptr);
                          varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          $$ = varp;
                          $$->सङ्कलय_अग्र(new Ast_नियोजय{$4, new AstParseRef{$<fl>3, *$3}, $5}); }
        //                      // IEEE: variable_assignment
        //                      // UNSUP variable_lvalue below
        |       id/*newOrExisting*/ '=' व्यञ्जकम्
                        { यदि (GRAMMARP->m_varDecl) {
                              AstVar* ध्रुव varp = VARDONEA($<fl>1, *$1, nullptr, nullptr);
                              varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                              $$ = varp;
                              $$->सङ्कलय_अग्र(new Ast_नियोजय{$2, new AstParseRef{$<fl>1, *$1}, $3});
                          } अन्यथा {
                              $$ = new Ast_नियोजय{$2, new AstParseRef{$<fl>1, *$1}, $3};
                          }
                        }
        ;

for_चरणः_E<nodep>:               // IEEE: for_step + empty
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       for_चरणः                                { $$ = $1; }
        ;

for_चरणः<nodep>:                // IEEE: for_step
                for_चरणः_assignment                     { $$ = $1; }
        |       for_चरणः ',' for_चरणः_assignment        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

for_चरणः_assignment<nodep>:     // ==IEEE: for_step_assignment
                foperator_assignment                    { $$ = $1; }
        |       finc_or_dec_expression                  { $$ = new Ast_Stmt_व्यञ्जकम्{$<fl>1, $1}; }
        //                      // IEEE: function_subroutine_call
        |       task_subroutine_callNoSemi              { $$ = $1; }
        ;

loop_variables<nodep>:          // IEEE: loop_variables
                loop_variableE                          { $$ = $1; }
        |       loop_variables ',' loop_variableE       { $$ = $1->सङ्कलय_अग्र($3); }
        ;

loop_variableE<nodep>:          // IEEE: part of loop_variables
                /*रिक्तानि*/                             { $$ = new Ast_रिक्तानि{CRELINE()}; }
        |       parse_Ref_आधारः                            { $$ = $1; }
        ;

//************************************************
// Functions/tasks

taskRef<ग्रन्थिः_Exprp>:            // IEEE: part of tf_call
                id                                      { $$ = new AstTaskRef{$<fl>1, *$1}; }
        |       id '(' list_सम्बन्धि_arguments_E ')'           { $$ = new AstTaskRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstTaskRef{$<fl>2, *$2, $4}); }
        ;

funcRef<ग्रन्थिः_Exprp>:             // IEEE: part of tf_call
        //                      // package_scope/hierarchical_... is part of expr, so just need ID
        //                      //      making-a                id-is-a
        //                      //      -----------------       ------------------
        //                      //      tf_call                 tf_identifier           expr (list_of_arguments)
        //                      //      method_call(post .)     function_identifier     expr (list_of_arguments)
        //                      //      property_instance       property_identifier     property_actual_arg
        //                      //      sequence_instance       sequence_identifier     sequence_actual_arg
        //                      //      let_expression          let_identifier          let_actual_arg
        //
                id '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = new AstFuncRef{$<fl>1, *$1, $3}; }
        |       packageClassScope id '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = AstDot::newIfPkg($<fl>2, $1, new AstFuncRef{$<fl>2, *$2, $4}); }
        //UNSUP list_of_argumentE should be pev_list_of_argumentE
        //UNSUP: idDottedSel is really just id to allow dotted method calls
        ;

task_subroutine_callNoSemi<ग्रन्थिः_Stmtp>:  // similar to IEEE task_subroutine_call but without ';'
        //                      // Expr included here to resolve our not knowing what is a method call
        //                      // Expr here must result in a subroutine_call
                task_subroutine_call_No_विधानम्            { $$ = $1->makeStmt(); }
        |       fexpr '.' task_subroutine_call_No_विधानम्  { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        |       system_t_stmt_call                      { $$ = $1; }
        //                      // Any system function as a task
        |       system_f_or_t_व्यञ्जकम्_call                 { $$ = new Ast_Stmt_व्यञ्जकम्{$<fl>1, $1}; }
        //                      // Not here in IEEE; from class_constructor_declaration
        //                      // Because we've joined class_constructor_declaration into generic functions
        //                      // Way over-permissive;
        //                      // IEEE: [ ySUPER '.' yNEW [ '(' list_of_arguments ')' ] ';' ]
        |       fexpr '.' class_newNoScope              { $$ = (new AstDot{$<fl>2, false, $1, $3})->makeStmt(); }
        ;

task_subroutine_call_No_विधानम्<ग्रन्थिः_Exprp>:    // function_subroutine_callNoMethod (as task)
        //                      // IEEE: tf_call
                taskRef                                 { $$ = $1; }
        //                      // funcref below not task ref to avoid conflict, must later handle either
        |       funcRef yWITH__PAREN '(' व्यञ्जकम् ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' व्यञ्जकम् ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_खण्डम्     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_खण्डम्
                                                        { AstWithParse* ध्रुव withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

function_subroutine_call_No_विधानम्<ग्रन्थिः_Exprp>:        // IEEE: function_subroutine_call (as function)
        //                      // IEEE: tf_call
                funcRef                                 { $$ = $1; }
        |       funcRef yWITH__PAREN '(' व्यञ्जकम् ')'       { $$ = new AstWithParse{$2, $1, $4}; }
        //                      // can call as method and yWITH without parenthesis
        |       id yWITH__PAREN '(' व्यञ्जकम् ')'            { $$ = new AstWithParse{$2, new AstFuncRef{$<fl>1, *$1}, $4}; }
        |       system_f_only_व्यञ्जकम्_call                 { $$ = $1; }
        |       system_f_or_t_व्यञ्जकम्_call                 { $$ = $1; }
        //                      // IEEE: method_call requires a "." so is in expr
        //                      // IEEE: ['std::'] not needed, as normal std package resolution will find it
        //                      // IEEE: randomize_call
        //                      // We implement randomize as a normal funcRef, since randomize isn't a keyword
        //                      // Note yNULL is already part of expressions, so they come for free
        |       funcRef yWITH__CUR constraint_खण्डम्     { $$ = new AstWithParse{$2, $1, nullptr, $3}; }
        |       funcRef yWITH__PAREN_CUR '(' inlineConstraintIdListE ')' constraint_खण्डम्
                                                        { AstWithParse* ध्रुव withParsep
                                                              = new AstWithParse{$2, $1, $4, $6};
                                                          withParsep->restricted(true);
                                                          $$ = withParsep; }
        ;

system_t_stmt_call<ग्रन्थिः_Stmtp>:  // IEEE: part of system_tf_call (as task returning statement)
        //
                yaD_PLI system_Dpi_तर्काः_E                  { AstTaskRef* ध्रुव refp = new AstTaskRef{$<fl>1, *$1, $2};
                                                          refp->pli(true);
                                                          $$ = refp->makeStmt(); }
        //
        |       yD_DUMPPORTS '(' idDottedSel ',' व्यञ्जकम् ')'  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $5}; DEL($3);
                                                          $$->सङ्कलय_अग्र(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPPORTS '(' ',' व्यञ्जकम् ')'           { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $4};
                                                          $$->सङ्कलय_अग्र(new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                                     new AstConst{$<fl>1, 1}}); }
        |       yD_DUMPFILE '(' व्यञ्जकम् ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FILE, $3}; }
        |       yD_DUMPVARS parenE                      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS,
                                                                              new AstConst{$<fl>1, 0}}; }
        |       yD_DUMPVARS '(' व्यञ्जकम् ')'                { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; }
        |       yD_DUMPVARS '(' व्यञ्जकम् ',' व्यञ्जकम्_List ')'   { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::VARS, $3}; DEL($5); }
        |       yD_DUMPALL parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; }
        |       yD_DUMPALL '(' व्यञ्जकम् ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ALL}; DEL($3); }
        |       yD_DUMPFLUSH parenE                     { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; }
        |       yD_DUMPFLUSH '(' व्यञ्जकम् ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::FLUSH}; DEL($3); }
        |       yD_DUMPLIMIT '(' व्यञ्जकम् ')'               { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::सीमा_2, $3}; }
        |       yD_DUMPLIMIT '(' व्यञ्जकम् ',' व्यञ्जकम् ')'      { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::सीमा_2, $3}; DEL($5); }
        |       yD_DUMPOFF parenE                       { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; }
        |       yD_DUMPOFF '(' व्यञ्जकम् ')'                 { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::OFF}; DEL($3); }
        |       yD_DUMPON parenE                        { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; }
        |       yD_DUMPON '(' व्यञ्जकम् ')'                  { $$ = new AstDumpCtl{$<fl>1, VDumpCtlType::ON}; DEL($3); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCStmtUser* cstmtp = nullptr;
                    यदि (!v3Global.opt.ignc()) {
                        cstmtp = new AstCStmtUser{$1, true};
                        cstmtp->सङ्कलय($3);
                    }
                    $$ = cstmtp;
                }
        |       yD_SDF_ANNOTATE '(' व्यञ्जकम्_EList_E ')'      { $$ = nullptr; $1->v3warn(SPECIFYIGN, "Ignoring unsupported: $sdf_annotate"); DEL($3); }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_राशिः_Trace_T{$1}; }
        |       yD_SYSTEM '(' व्यञ्जकम् ')'                  { $$ = new AstSystemT{$1, $3}; }
        //
        |       yD_EXIT parenE                          { $$ = new AstFinish{$1}; }
        //
        |       yD_FCLOSE '(' व्यञ्जकम् ')'                  { $$ = new AstFClose{$1, $3}; }
        |       yD_FFLUSH parenE                        { $$ = new AstFFlush{$1, nullptr}; }
        |       yD_FFLUSH '(' व्यञ्जकम् ')'                  { $$ = new AstFFlush{$1, $3}; }
        |       yD_FINISH parenE                        { $$ = new AstFinish{$1}; }
        |       yD_FINISH '(' व्यञ्जकम् ')'                  { $$ = new AstFinish{$1}; DEL($3); }
        |       yD_STOP parenE                          { $$ = new AstStop{$1, false}; }
        |       yD_STOP '(' व्यञ्जकम् ')'                    { $$ = new AstStop{$1, false}; DEL($3); }
        //
        |       yD_SFORMAT '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'        { $$ = new AstSFormat{$1, false, $3, $5}; }
        |       yD_SWRITE  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'd'}; }
        |       yD_SWRITEB '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'b'}; }
        |       yD_SWRITEH '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'h'}; }
        |       yD_SWRITEO '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'        { $$ = new AstSFormat{$1, true, $3, $5, 'o'}; }
        //
        |       yD_DISPLAY parenE                               { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr}; }
        |       yD_DISPLAY '(' commasE व्यञ्जकम्_Disp_List ')'         { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4}; }
        |       yD_DISPLAYB parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'b'}; }
        |       yD_DISPLAYB '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'b'}; }
        |       yD_DISPLAYH parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'h'}; }
        |       yD_DISPLAYH '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'h'}; }
        |       yD_DISPLAYO parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, nullptr, 'o'}; }
        |       yD_DISPLAYO '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, nullptr, $4, 'o'}; }
        |       yD_MONITOR  '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4}; }
        |       yD_MONITORB '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'b'}; }
        |       yD_MONITORH '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'h'}; }
        |       yD_MONITORO '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, nullptr, $4, 'o'}; }
        |       yD_STROBE   '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4}; }
        |       yD_STROBEB  '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'b'}; }
        |       yD_STROBEH  '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'h'}; }
        |       yD_STROBEO  '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, nullptr, $4, 'o'}; }
        |       yD_WRITE    parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITE    '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4}; }
        |       yD_WRITEB   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEB   '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'b'}; }
        |       yD_WRITEH   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEH   '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'h'}; }
        |       yD_WRITEO   parenE                              { $$ = nullptr; }  // NOP
        |       yD_WRITEO   '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, nullptr, $4, 'o'}; }
        |       yD_FDISPLAY '(' व्यञ्जकम् ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr}; }
        |       yD_FDISPLAY '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5}; }
        |       yD_FDISPLAYB '(' व्यञ्जकम् ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'b'}; }
        |       yD_FDISPLAYB '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'b'}; }
        |       yD_FDISPLAYH '(' व्यञ्जकम् ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'h'}; }
        |       yD_FDISPLAYH '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'h'}; }
        |       yD_FDISPLAYO '(' व्यञ्जकम् ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, nullptr, 'o'}; }
        |       yD_FDISPLAYO '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_DISPLAY, $3, $5, 'o'}; }
        |       yD_FMONITOR  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5}; }
        |       yD_FMONITORB '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'b'}; }
        |       yD_FMONITORH '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'h'}; }
        |       yD_FMONITORO '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_MONITOR, $3, $5, 'o'}; }
        |       yD_FSTROBE   '(' व्यञ्जकम् ')'                       { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, nullptr}; }
        |       yD_FSTROBE   '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5}; }
        |       yD_FSTROBEB  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'b'}; }
        |       yD_FSTROBEH  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'h'}; }
        |       yD_FSTROBEO  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'      { $$ = new AstDisplay{$1, VDisplayType::DT_STROBE, $3, $5, 'o'}; }
        |       yD_FWRITE   '(' व्यञ्जकम् ')'                        { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, nullptr}; }
        |       yD_FWRITE   '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5}; }
        |       yD_FWRITEB  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'b'}; }
        |       yD_FWRITEH  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'h'}; }
        |       yD_FWRITEO  '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'       { $$ = new AstDisplay{$1, VDisplayType::DT_WRITE, $3, $5, 'o'}; }
        |       yD_INFO     parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, nullptr}; }
        |       yD_INFO     '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_INFO, nullptr, $4}; }
        |       yD_WARNING  parenE                              { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, nullptr}; }
        |       yD_WARNING  '(' commasE व्यञ्जकम्_Disp_List ')'        { $$ = new AstDisplay{$1, VDisplayType::DT_WARNING, nullptr, $4}; }
        |       yD_ERROR    parenE                              { $$ = GRAMMARP->createDisplayError($1); }
        |       yD_ERROR    '(' commasE व्यञ्जकम्_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_ERROR, nullptr, $4};
                          $$->सङ्कलय_अग्र(new AstStop{$1, false}); }
        |       yD_FATAL    parenE
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->सङ्कलय_अग्र(new AstStop{$1, true}); }
        |       yD_FATAL    '(' व्यञ्जकम् ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, nullptr};
                          $$->सङ्कलय_अग्र(new AstStop{$1, true}); DEL($3); }
        |       yD_FATAL    '(' व्यञ्जकम् ',' व्यञ्जकम्_Disp_List ')'
                        { $$ = new AstDisplay{$1, VDisplayType::DT_FATAL, nullptr, $5};
                          $$->सङ्कलय_अग्र(new AstStop{$1, true}); DEL($3); }
        //
        |       yD_ASSERTCTL '(' व्यञ्जकम् ')'                                            { $$ = new AstAssertCtl{$1, $3}; }
        |       yD_ASSERTCTL '(' व्यञ्जकम् ',' व्यञ्जकम्_E ')'                                  { $$ = new AstAssertCtl{$1, $3, $5}; }
        |       yD_ASSERTCTL '(' व्यञ्जकम् ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ')'                        { $$ = new AstAssertCtl{$1, $3, $5, $7}; }
        |       yD_ASSERTCTL '(' व्यञ्जकम् ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ')'              { $$ = new AstAssertCtl{$1, $3, $5, $7, $9}; }
        |       yD_ASSERTCTL '(' व्यञ्जकम् ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')' { $$ = new AstAssertCtl{$1, $3, $5, $7, $9, $11}; }
        |       yD_ASSERTFAILOFF '(' व्यञ्जकम् ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3}; }
        |       yD_ASSERTFAILOFF '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_OFF, 31, 7}; }
        |       yD_ASSERTFAILON '(' व्यञ्जकम् ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3}; }
        |       yD_ASSERTFAILON '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTFAILON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::FAIL_ON, 31, 7}; }
        |       yD_ASSERTKILL '(' व्यञ्जकम् ')'                      { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3}; }
        |       yD_ASSERTKILL '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'        { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7, $3, $5}; }
        |       yD_ASSERTKILL parenE                            { $$ = new AstAssertCtl{$1, VAssertCtlType::KILL, 15, 7}; }
        |       yD_ASSERTNONVACUOUSON '(' व्यञ्जकम् ')'                 { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3}; }
        |       yD_ASSERTNONVACUOUSON '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'   { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTNONVACUOUSON parenE                       { $$ = new AstAssertCtl{$1, VAssertCtlType::NONVACUOUS_ON, 31, 7}; }
        |       yD_ASSERTOFF '(' व्यञ्जकम् ')'                       { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3}; }
        |       yD_ASSERTOFF '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'         { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7, $3, $5}; }
        |       yD_ASSERTOFF parenE                             { $$ = new AstAssertCtl{$1, VAssertCtlType::OFF, 15, 7}; }
        |       yD_ASSERTON '(' व्यञ्जकम् ')'                        { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3}; }
        |       yD_ASSERTON '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'          { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7, $3, $5}; }
        |       yD_ASSERTON parenE                              { $$ = new AstAssertCtl{$1, VAssertCtlType::ON, 15, 7}; }
        |       yD_ASSERTPASSOFF '(' व्यञ्जकम् ')'                   { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3}; }
        |       yD_ASSERTPASSOFF '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'     { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSOFF parenE                         { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_OFF, 31, 7}; }
        |       yD_ASSERTPASSON '(' व्यञ्जकम् ')'                    { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3}; }
        |       yD_ASSERTPASSON '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'      { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7, $3, $5}; }
        |       yD_ASSERTPASSON parenE                          { $$ = new AstAssertCtl{$1, VAssertCtlType::PASS_ON, 31, 7}; }
        |       yD_ASSERTVACUOUSOFF '(' व्यञ्जकम् ')'                { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3}; }
        |       yD_ASSERTVACUOUSOFF '(' व्यञ्जकम्_E ',' व्यञ्जकम्_List ')'  { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7, $3, $5}; }
        |       yD_ASSERTVACUOUSOFF parenE                      { $$ = new AstAssertCtl{$1, VAssertCtlType::VACUOUS_OFF, 31, 7}; }
        //
        |       yD_MONITOROFF parenE                    { $$ = new AstMonitorOff{$1, true}; }
        |       yD_MONITORON parenE                     { $$ = new AstMonitorOff{$1, false}; }
        //
        |       yD_PRINTTIMESCALE                       { $$ = new AstPrintTimeScale{$1}; }
        |       yD_PRINTTIMESCALE '(' ')'               { $$ = new AstPrintTimeScale{$1}; }
        |       yD_PRINTTIMESCALE '(' idClassSel ')'    { $$ = new AstPrintTimeScale{$1}; DEL($3); }
        |       yD_TIMEFORMAT '(' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ')'
                        { $$ = new AstTimeFormat{$1, $3, $5, $7, $9}; }
        |       yD_TIMEFORMAT '(' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ')'
                        { $$ = new AstTimeFormat{$1, $3, $5, $7, nullptr}; }
        |       yD_TIMEFORMAT '(' व्यञ्जकम्_E ',' व्यञ्जकम्_E ')'
                        { $$ = new AstTimeFormat{$1, $3, $5, nullptr, nullptr}; }
        |       yD_TIMEFORMAT '(' व्यञ्जकम्_E ')'
                        { $$ = new AstTimeFormat{$1, $3, nullptr, nullptr, nullptr}; }
        //
        |       yD_READMEMB '(' व्यञ्जकम् ',' idClassSel ')'                         { $$ = new AstReadMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_READMEMB '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ')'                { $$ = new AstReadMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_READMEMB '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ',' व्यञ्जकम् ')'       { $$ = new AstReadMem{$1, false, $3, $5, $7, $9}; }
        |       yD_READMEMH '(' व्यञ्जकम् ',' idClassSel ')'                         { $$ = new AstReadMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_READMEMH '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ')'                { $$ = new AstReadMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_READMEMH '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ',' व्यञ्जकम् ')'       { $$ = new AstReadMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_WRITEMEMB '(' व्यञ्जकम् ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, false, $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMB '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ')'               { $$ = new AstWriteMem{$1, false, $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMB '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ',' व्यञ्जकम् ')'      { $$ = new AstWriteMem{$1, false, $3, $5, $7, $9}; }
        |       yD_WRITEMEMH '(' व्यञ्जकम् ',' idClassSel ')'                        { $$ = new AstWriteMem{$1, true,  $3, $5, nullptr, nullptr}; }
        |       yD_WRITEMEMH '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ')'               { $$ = new AstWriteMem{$1, true,  $3, $5, $7, nullptr}; }
        |       yD_WRITEMEMH '(' व्यञ्जकम् ',' idClassSel ',' व्यञ्जकम् ',' व्यञ्जकम् ')'      { $$ = new AstWriteMem{$1, true,  $3, $5, $7, $9}; }
        //
        |       yD_CAST '(' व्यञ्जकम् ',' व्यञ्जकम् ')'
                        { File_पङ्क्तिः* ध्रुव fl_nowarn = new File_पङ्क्तिः{$1};
                          fl_nowarn->warnOff(V3_Error_संहिता::विस्तारः_2, true);
                          $$ = new AstAssertIntrinsic{fl_nowarn, new AstCastDynamic{fl_nowarn, $5, $3},
                                                      nullptr, nullptr}; }
        ;

system_f_only_व्यञ्जकम्_call<ग्रन्थिः_Exprp>:  // IEEE: part of system_tf_call (for functions returning expressions)
                yaD_PLI system_Dpi_तर्काः_E                  { $$ = new AstFuncRef{$<fl>1, *$1, $2}; VN_CAST($$, FuncRef)->pli(true); }
        //
        |       yD_C '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    यदि (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1};
                        cexprp->सङ्कलय($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CPURE '(' cStrList ')' {
                    AstCExprUser* cexprp = nullptr;
                    यदि (!v3Global.opt.ignc()) {
                        cexprp = new AstCExprUser{$1, AstCExprUser::विशुद्धम्{}};
                        cexprp->सङ्कलय($3);
                    }
                    $$ = cexprp;
                }
        |       yD_CAST '(' व्यञ्जकम् ',' व्यञ्जकम् ')'           { $$ = new AstCastDynamic{$1, $5, $3}; }
        |       yD_STACKTRACE parenE                    { $$ = new Ast_राशिः_Trace_F{$1}; }
        |       yD_SYSTEM  '(' व्यञ्जकम् ')'                 { $$ = new AstSystemF{$1, $3}; }
        ;

system_f_or_t_व्यञ्जकम्_call<ग्रन्थिः_Exprp>:  // IEEE: part of system_tf_call (can be task or func)
                yD_ACOS '(' व्यञ्जकम् ')'                    { $$ = new AstAcosD{$1, $3}; }
        |       yD_ACOSH '(' व्यञ्जकम् ')'                   { $$ = new AstAcoshD{$1, $3}; }
        |       yD_ASIN '(' व्यञ्जकम् ')'                    { $$ = new AstAsinD{$1, $3}; }
        |       yD_ASINH '(' व्यञ्जकम् ')'                   { $$ = new AstAsinhD{$1, $3}; }
        |       yD_ATAN '(' व्यञ्जकम् ')'                    { $$ = new AstAtanD{$1, $3}; }
        |       yD_ATAN2 '(' व्यञ्जकम् ',' व्यञ्जकम् ')'          { $$ = new AstAtan2D{$1, $3, $5}; }
        |       yD_ATANH '(' व्यञ्जकम् ')'                   { $$ = new AstAtanhD{$1, $3}; }
        |       y_D_द्वयङ्काः '(' व्यञ्जकम्_Or_दत्त_Type ')'          { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_द्वयङ्काः, $3}; }
        |       y_D_द्वयङ्काः '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')' { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_द्वयङ्काः, $3, $5}; }
        |       yD_BITSTOREAL '(' व्यञ्जकम् ')'              { $$ = new Ast_द्वयङ्काः_गन्तव्यम्_Real_D{$1, $3}; }
        |       yD_BITSTOSHORTREAL '(' व्यञ्जकम् ')'         { $$ = new Ast_द्वयङ्काः_गन्तव्यम्_Real_D{$1, $3}; UNSUPREAL($1); }
        |       yD_CEIL '(' व्यञ्जकम् ')'                    { $$ = new AstCeilD{$1, $3}; }
        |       y_D_परिवर्तितम् '(' व्यञ्जकम् ')'                 { $$ = new AstLogNot{$1, new AstStable{$1, $3, nullptr}}; }
        |       y_D_परिवर्तितम् '(' व्यञ्जकम् ',' व्यञ्जकम् ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Sen_वृक्षः_परिवर्तितम्($1, $5)}}; }
        |       y_D_परिवर्तितम्_GCLK '(' व्यञ्जकम् ')'
                        { $$ = new AstLogNot{$1, new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_वृक्षः($1)}}; }
        |       yD_CHANGING_GCLK '(' व्यञ्जकम् ')'
                        { $$ = new AstLogNot{$1, new AstSteady{$1, $3}}; }
        |       yD_CLOG2 '(' व्यञ्जकम् ')'                   { $$ = new AstCLog2{$1, $3}; }
        |       yD_COS '(' व्यञ्जकम् ')'                     { $$ = new AstCosD{$1, $3}; }
        |       yD_COSH '(' व्यञ्जकम् ')'                    { $$ = new AstCoshD{$1, $3}; }
        |       yD_COUNTBITS '(' व्यञ्जकम् ',' व्यञ्जकम् ')'              { $$ = new Ast_गणनम्_द्वयङ्काः{$1, $3, $5}; }
        |       yD_COUNTBITS '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'             { $$ = new Ast_गणनम्_द्वयङ्काः{$1, $3, $5, $7}; }
        |       yD_COUNTBITS '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'    { $$ = new Ast_गणनम्_द्वयङ्काः{$1, $3, $5, $7, $9}; }
        |       yD_COUNTBITS '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम्_List ')'
                        { $$ = new Ast_गणनम्_द्वयङ्काः{$1, $3, $5, $7, $9};
                          BBUNSUP($11, "Unsupported: $countbits with more than 3 control fields"); }
        |       yD_COUNTONES '(' व्यञ्जकम् ')'               { $$ = new Ast_गणनम्_Ones{$1, $3}; }
        |       yD_DIMENSIONS '(' व्यञ्जकम्_Or_दत्त_Type ')'    { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_DIMENSIONS, $3}; }
        |       y_D_DIST_CHI_वर्गफलम् '(' व्यञ्जकम् ',' व्यञ्जकम् ')'        { $$ = new Ast_Dist_Chi_वर्गफलम्{$1, $3, $5}; }
        |       yD_DIST_ERLANG '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'   { $$ = new AstDistErlang{$1, $3, $5, $7}; }
        |       y_D_DIST_घातीयम् '(' व्यञ्जकम् ',' व्यञ्जकम् ')'       { $$ = new Ast_Dist_घातीयम्{$1, $3, $5}; }
        |       y_D_DIST_प्रसामान्यम् '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'   { $$ = new Ast_Dist_प्रसामान्यम्{$1, $3, $5, $7}; }
        |       y_D_DIST_पोयसन '(' व्यञ्जकम् ',' व्यञ्जकम् ')'   { $$ = new Ast_Dist_पोयसन{$1, $3, $5}; }
        |       yD_DIST_T '(' व्यञ्जकम् ',' व्यञ्जकम् ')'         { $$ = new AstDistT{$1, $3, $5}; }
        |       y_D_DIST_एकरूपम् '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'  { $$ = new Ast_Dist_एकरूपम्{$1, $3, $5, $7}; }
        |       yD_EXP '(' व्यञ्जकम् ')'                     { $$ = new AstExpD{$1, $3}; }
        |       yD_FALLING_GCLK '(' व्यञ्जकम् ')'            { $$ = new AstFalling{$1, $3}; }
        |       yD_FELL '(' व्यञ्जकम् ')'                    { $$ = new AstFell{$1, $3, nullptr}; }
        |       yD_FELL '(' व्यञ्जकम् ',' व्यञ्जकम् ')'           { $$ = new AstFell{$1, $3, GRAMMARP->create_Sen_वृक्षः_परिवर्तितम्($1, $5)}; }
        |       yD_FELL_GCLK '(' व्यञ्जकम् ')'               { $$ = new AstFell{$1, $3, GRAMMARP->create_Global_Clock_Sen_वृक्षः($1)}; }
        |       yD_FEOF '(' व्यञ्जकम् ')'                    { $$ = new AstFEof{$1, $3}; }
        |       yD_FERROR '(' व्यञ्जकम् ',' idClassSel ')'   { $$ = new AstFError{$1, $3, $5}; }
        |       yD_FGETC '(' व्यञ्जकम् ')'                   { $$ = new AstFGetC{$1, $3}; }
        |       yD_FGETS '(' व्यञ्जकम् ',' व्यञ्जकम् ')'          { $$ = new AstFGetS{$1, $3, $5}; }
        |       yD_FOPEN '(' व्यञ्जकम् ')'                   { $$ = new AstFOpenMcd{$1, $3}; }
        |       yD_FOPEN '(' व्यञ्जकम् ',' व्यञ्जकम् ')'          { $$ = new AstFOpen{$1, $3, $5}; }
        |       yD_FREAD '(' व्यञ्जकम् ',' व्यञ्जकम् ')'          { $$ = new AstFRead{$1, $3, $5, nullptr, nullptr}; }
        |       yD_FREAD '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'  { $$ = new AstFRead{$1, $3, $5, $7, nullptr}; }
        |       yD_FREAD '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')'  { $$ = new AstFRead{$1, $3, $5, $7, $9}; }
        |       yD_FREAD '(' व्यञ्जकम् ',' व्यञ्जकम् ',' ',' व्यञ्जकम् ')'  { $$ = new AstFRead{$1, $3, $5, nullptr, $8}; }
        |       yD_FREWIND '(' व्यञ्जकम् ')'                 { $$ = new AstFRewind{$1, $3}; }
        |       yD_FUTURE_GCLK '(' व्यञ्जकम् ')'             { $$ = new AstFuture{$1, $3, nullptr}; }
        |       yD_FLOOR '(' व्यञ्जकम् ')'                   { $$ = new AstFloorD{$1, $3}; }
        |       yD_FSCANF '(' व्यञ्जकम् ',' str commaVRDListE ')'    { $$ = new AstFScanF{$1, *$5, $3, $6}; }
        |       yD_FSEEK '(' व्यञ्जकम् ',' व्यञ्जकम् ',' व्यञ्जकम् ')' { $$ = new AstFSeek{$1, $3, $5, $7}; }
        |       yD_FTELL '(' व्यञ्जकम् ')'                   { $$ = new AstFTell{$1, $3}; }
        |       y_D_GET_INITIAL_RANDOM_बीजम् parenE      { $$ = new Ast_Get_Initial_Random_बीजम्{$1}; }
        |       yD_GLOBAL_CLOCK parenE                  { $$ = GRAMMARP->createGlobalClockParseRef($1); }
        |       y_D_उच्चः '(' व्यञ्जकम्_Or_दत्त_Type ')'          { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_उच्चः, $3, nullptr}; }
        |       y_D_उच्चः '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')' { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_उच्चः, $3, $5}; }
        |       yD_HYPOT '(' व्यञ्जकम् ',' व्यञ्जकम् ')'          { $$ = new AstHypotD{$1, $3, $5}; }
        |       yD_INCREMENT '(' व्यञ्जकम्_Or_दत्त_Type ')'     { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_INCREMENT, $3, nullptr}; }
        |       yD_INCREMENT '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')'    { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_INCREMENT, $3, $5}; }
        |       yD_INFERRED_DISABLE parenE              { $$ = new AstInferredDisable{$1}; }
        |       yD_ISUNBOUNDED '(' व्यञ्जकम् ')'             { $$ = new AstIsUnbounded{$1, $3}; }
        |       yD_ISUNKNOWN '(' व्यञ्जकम् ')'               { $$ = new AstIsUnknown{$1, $3}; }
        |       yD_ITOR '(' व्यञ्जकम् ')'                    { $$ = new AstIToRD{$1, $3}; }
        |       y_D_वामः '(' व्यञ्जकम्_Or_दत्त_Type ')'          { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_वामः, $3, nullptr}; }
        |       y_D_वामः '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')' { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_वामः, $3, $5}; }
        |       yD_LN '(' व्यञ्जकम् ')'                      { $$ = new AstLogD{$1, $3}; }
        |       yD_LOG10 '(' व्यञ्जकम् ')'                   { $$ = new AstLog10D{$1, $3}; }
        |       y_D_नीचः '(' व्यञ्जकम्_Or_दत्त_Type ')'           { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_नीचः, $3, nullptr}; }
        |       y_D_नीचः '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')'  { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_नीचः, $3, $5}; }
        |       yD_ONEHOT '(' व्यञ्जकम् ')'                  { $$ = new AstOneHot{$1, $3}; }
        |       yD_ONEHOT0 '(' व्यञ्जकम् ')'                 { $$ = new AstOneHot0{$1, $3}; }
        |       yD_PAST '(' व्यञ्जकम् ')'                    { $$ = new AstPast{$1, $3}; }
        |       yD_PAST '(' व्यञ्जकम् ',' व्यञ्जकम्_E ')'          { $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' व्यञ्जकम् ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ')'
                        { यदि ($7) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST '(' व्यञ्जकम् ',' व्यञ्जकम्_E ',' व्यञ्जकम्_E ',' clocking_eventE ')'
                        { यदि ($7 || $9) BBUNSUP($1, "Unsupported: $past expr2 and/or clock arguments");
                          DEL($7, $9);
                          $$ = new AstPast{$1, $3, $5}; }
        |       yD_PAST_GCLK '(' व्यञ्जकम् ')'               { $$ = new AstPast{$1, $3, nullptr, GRAMMARP->create_Global_Clock_Sen_वृक्षः($1)}; }
        |       yD_POW '(' व्यञ्जकम् ',' व्यञ्जकम् ')'            { $$ = new AstPowD{$1, $3, $5}; }
        |       yD_RANDOM '(' व्यञ्जकम् ')'                  { $$ = new AstRand{$1, $3, false}; }
        |       yD_RANDOM parenE                        { $$ = new AstRand{$1, nullptr, false}; }
        |       yD_REALTIME parenE                      { $$ = new AstTimeD{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_REALTOBITS '(' व्यञ्जकम् ')'              { $$ = new Ast_Real_गन्तव्यम्_द्वयङ्काः{$1, $3}; }
        |       yD_REWIND '(' व्यञ्जकम् ')'                  { $$ = new AstFSeek{$1, $3, new AstConst{$1, 0}, new AstConst{$1, 0}}; }
        |       y_D_दक्षिणः '(' व्यञ्जकम्_Or_दत्त_Type ')'         { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_दक्षिणः, $3, nullptr}; }
        |       y_D_दक्षिणः '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')'        { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_दक्षिणः, $3, $5}; }
        |       yD_RISING_GCLK '(' व्यञ्जकम् ')'             { $$ = new AstRising{$1, $3}; }
        |       yD_ROSE '(' व्यञ्जकम् ')'                    { $$ = new AstRose{$1, $3, nullptr}; }
        |       yD_ROSE '(' व्यञ्जकम् ',' व्यञ्जकम् ')'           { $$ = new AstRose{$1, $3, GRAMMARP->create_Sen_वृक्षः_परिवर्तितम्($1, $5)}; }
        |       yD_ROSE_GCLK '(' व्यञ्जकम् ')'               { $$ = new AstRose{$1, $3, GRAMMARP->create_Global_Clock_Sen_वृक्षः($1)}; }
        |       yD_RTOI '(' व्यञ्जकम् ')'                    { $$ = new AstRToIS{$1, $3}; }
        |       yD_SAMPLED '(' व्यञ्जकम् ')'                 { $$ = new AstSampled{$1, $3, $3->dtypep()}; }
        |       yD_SFORMATF '(' व्यञ्जकम्_Disp_List ')'        { $$ = new AstSFormatF{$1, AstSFormatF::व्यञ्जकम्_Format{}, $3, 'd', false}; }
        |       yD_SHORTREALTOBITS '(' व्यञ्जकम् ')'         { $$ = new Ast_Real_गन्तव्यम्_द्वयङ्काः{$1, $3}; UNSUPREAL($1); }
        |       yD_SIGNED '(' व्यञ्जकम् ')'                  { $$ = new AstSigned{$1, $3}; }
        |       yD_SIN '(' व्यञ्जकम् ')'                     { $$ = new AstSinD{$1, $3}; }
        |       yD_SINH '(' व्यञ्जकम् ')'                    { $$ = new AstSinhD{$1, $3}; }
        |       y_D_आकार '(' व्यञ्जकम्_Or_दत्त_Type ')'          { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_आकार, $3, nullptr}; }
        |       y_D_आकार '(' व्यञ्जकम्_Or_दत्त_Type ',' व्यञ्जकम् ')' { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_आकार, $3, $5}; }
        |       yD_SQRT '(' व्यञ्जकम् ')'                    { $$ = new AstSqrtD{$1, $3}; }
        |       yD_SSCANF '(' व्यञ्जकम् ',' str commaVRDListE ')'    { $$ = new AstSScanF{$1, *$5, $3, $6}; }
        |       yD_STABLE '(' व्यञ्जकम् ')'                  { $$ = new AstStable{$1, $3, nullptr}; }
        |       yD_STABLE '(' व्यञ्जकम् ',' व्यञ्जकम् ')'         { $$ = new AstStable{$1, $3, GRAMMARP->create_Sen_वृक्षः_परिवर्तितम्($1, $5)}; }
        |       yD_STABLE_GCLK '(' व्यञ्जकम् ')'             { $$ = new AstStable{$1, $3, GRAMMARP->create_Global_Clock_Sen_वृक्षः($1)}; }
        |       yD_STEADY_GCLK '(' व्यञ्जकम् ')'             { $$ = new AstSteady{$1, $3}; }
        |       yD_STIME parenE
                        { $$ = new AstSel{$1, new AstTime{$1, VTimescale{VTimescale::NONE}}, 0, 32}; }
        |       yD_TAN '(' व्यञ्जकम् ')'                     { $$ = new AstTanD{$1, $3}; }
        |       yD_TANH '(' व्यञ्जकम् ')'                    { $$ = new AstTanhD{$1, $3}; }
        |       yD_TESTPLUSARGS '(' व्यञ्जकम् ')'            { $$ = new Ast_Test_Plus_तर्काः{$1, $3}; }
        |       yD_TIME parenE                          { $$ = new AstTime{$1, VTimescale{VTimescale::NONE}}; }
        |       yD_TIMEPRECISION                        { $$ = new AstTimePrecision{$1}; }
        |       yD_TIMEPRECISION '(' ')'                { $$ = new AstTimePrecision{$1}; }
        |       yD_TIMEPRECISION '(' idClassSel ')'     { $$ = new AstTimePrecision{$1}; DEL($3); }
        |       yD_TIMEUNIT                             { $$ = new AstTimeUnit{$1}; }
        |       yD_TIMEUNIT '(' ')'                     { $$ = new AstTimeUnit{$1}; }
        |       yD_TIMEUNIT '(' idClassSel ')'          { $$ = new AstTimeUnit{$1}; DEL($3); }
        |       yD_TYPENAME '(' व्यञ्जकम्_Or_दत्त_Type ')'      { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::TYPENAME, $3}; }
        |       yD_UNGETC '(' व्यञ्जकम् ',' व्यञ्जकम् ')'         { $$ = new AstFUngetC{$1, $5, $3}; }  // Arg swap to file first
        |       yD_UNPACKED_DIMENSIONS '(' व्यञ्जकम्_Or_दत्त_Type ')'   { $$ = new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_UNPK_DIMENSIONS, $3}; }
        |       yD_UNSIGNED '(' व्यञ्जकम् ')'                { $$ = new AstUnsigned{$1, $3}; }
        |       yD_URANDOM '(' व्यञ्जकम् ')'                 { $$ = new AstRand{$1, $3, true}; }
        |       yD_URANDOM parenE                       { $$ = new AstRand{$1, nullptr, true}; }
        |       yD_URANDOM_RANGE '(' व्यञ्जकम् ')'           { $$ = new AstURandomRange{$1, $3, new AstConst{$1, 0}}; }
        |       yD_URANDOM_RANGE '(' व्यञ्जकम् ',' व्यञ्जकम् ')'  { $$ = new AstURandomRange{$1, $3, $5}; }
        |       yD_VALUEPLUSARGS '(' व्यञ्जकम् ',' व्यञ्जकम् ')'  { $$ = new Ast_मानम्_Plus_तर्काः{$1, $3, $5}; }
        ;

severity_system_task<nodep>: // IEEE: severity_system_task/elaboration_severity_system_task (1800-2009)
        //                      // Elaboration-time task; V3Width evaluates and removes it
                severity_system_task_guts ';'           { $$ = $1; }
        ;

severity_system_task_guts<nodep>:    // IEEE: part of severity_system_task (1800-2009)
        //                      // $fatal first argument is exit number, must be constant
                yD_INFO parenE                          { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, nullptr}; }
        |       yD_INFO '(' व्यञ्जकम्_List ')'                { $$ = new AstElabDisplay{$1, VDisplayType::DT_INFO, $3}; }
        |       yD_WARNING parenE                       { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, nullptr}; }
        |       yD_WARNING '(' व्यञ्जकम्_List ')'             { $$ = new AstElabDisplay{$1, VDisplayType::DT_WARNING, $3}; }
        |       yD_ERROR parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, nullptr}; }
        |       yD_ERROR '(' व्यञ्जकम्_List ')'               { $$ = new AstElabDisplay{$1, VDisplayType::DT_ERROR, $3}; }
        |       yD_FATAL parenE                         { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; }
        |       yD_FATAL '(' व्यञ्जकम् ')'                   { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, nullptr}; DEL($3); }
        |       yD_FATAL '(' व्यञ्जकम् ',' व्यञ्जकम्_List_E ')'     { $$ = new AstElabDisplay{$1, VDisplayType::DT_FATAL, $5}; DEL($3); }
        ;

system_Dpi_तर्काः_E<argp>:           // IEEE: part of system_if_call for arguments of $dpi call
                parenE                                  { $$ = nullptr; }
        |       '(' व्यञ्जकम्_List ')'                        { $$ = GRAMMARP->argWrapList($2); }
        ;

property_actual_arg<ग्रन्थिः_Exprp>:  // ==IEEE: property_actual_arg
        //                      // IEEE: property_expr
        //                      // IEEE: sequence_actual_arg
        //UNSUP pev_expr                                { $$ = $1; }
        //UNSUP remove below:
                pexpr                                   { $$ = $1; }
        //                      // IEEE: sequence_expr
        //                      // property_expr already includes sequence_expr
        ;

व्यञ्जकम्_Or_दत्त_Type<nodep>:          // expr | data_type: combined to prevent conflicts
                व्यञ्जकम्                                    { $$ = $1; }
        //                      // data_type includes id that overlaps expr, so special flavor
        //                      // data_type expanded:
        |       दत्त_type_No_Ref                          { $$ = $1; }
        //
        //                      // Conflicts with non-type id, resolved in V3LinkDot
        //                      // NO: packageClassScopeE idType packed_dimensionListE
        //
        |       packageClassScopeE idType parameter_मानम्_assignment_Class packed_dimensionListE
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, $3};
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

list_सम्बन्धि_arguments_E<argp>:  // IEEE: [list_of_arguments]
                तर्काः_Dotted_List                          { $$ = $1; }
        |       तर्काः_व्यञ्जकम्_List_E
                        { यदि ($1->रिक्तानि_Connect_No_अग्र()) {
                              $1->delete_वृक्षः(); $$ = nullptr;  // Mis-created when have 'func()'
                          } अन्यथा { $$ = $1; } }
        |       तर्काः_व्यञ्जकम्_List_E ',' तर्काः_Dotted_List        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

task_declaration<ग्रन्थिः_FTaskp>:   // ==IEEE: task_declaration
                yTASK dynamic_override_specifiersE lifetimeE taskId tfGuts yENDTASK अन्तः_Label_E
                        { $$ = $4; $$->सङ्कलय_Stmtsp($5);
                          $$->आधारः_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->अन्तः_Label($<fl>7, $$, $7); }
        ;

task_prototype<ग्रन्थिः_FTaskp>:             // ==IEEE: task_prototype
                yTASK dynamic_override_specifiersE taskId '(' tf_पत्तनम्_list_E ')'
                        { $$ = $3; $$->सङ्कलय_Stmtsp($5); $$->prototype(true); }
        |       yTASK dynamic_override_specifiersE taskId
                        { $$ = $3; $$->prototype(true); }
        ;

function_declaration<ग्रन्थिः_FTaskp>:       // IEEE: function_declaration + function_body_declaration
                yFUNCTION dynamic_override_specifiersE lifetimeE funcId funcIsolateE tfGuts yENDFUNCTION अन्तः_Label_E
                        { $$ = $4; $$->सङ्कलय_Stmtsp($6);
                          $$->आधारः_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->अन्तः_Label($<fl>8, $$, $8); }
        |       yFUNCTION dynamic_override_specifiersE lifetimeE funcIdNew funcIsolateE tfNewGuts yENDFUNCTION अन्तः_Label_E
                        { $$ = $4; $$->सङ्कलय_Stmtsp($6);
                          $$->आधारः_Override($2);
                          $$->lifetime($3);
                          GRAMMARP->अन्तः_Label($<fl>8, $$, $8); }
        ;

function_prototype<ग्रन्थिः_FTaskp>: // IEEE: function_prototype
                yFUNCTION dynamic_override_specifiersE funcId '(' tf_पत्तनम्_list_E ')'
                        { $$ = $3; $$->सङ्कलय_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcId
                        { $$ = $3; $$->prototype(true); }
        ;

class_constructor_prototype<ग्रन्थिः_FTaskp>:        // ==IEEE: class_constructor_prototype
        //                      // IEEE has no dynamic_override_specifiersE,
        //                      // but required to avoid conflicts, so we must check after parsing
                yFUNCTION dynamic_override_specifiersE funcIdNew '(' class_constructor_arg_listE ')' ';'
                        { $$ = $3; $$->सङ्कलय_Stmtsp($5); $$->prototype(true); }
        |       yFUNCTION dynamic_override_specifiersE funcIdNew ';'
                        { $$ = $3; $$->prototype(true); }
        ;

funcIsolateE<cint>:
                /*रिक्तानि*/                             { $$ = 0; }
        |       yVL_ISOLATE_ASSIGNMENTS                 { $$ = 1; }
        ;

विधानम्_prototype<ग्रन्थिः_FTaskp>:
                task_prototype                          { $$ = $1; }
        |       function_prototype                      { $$ = $1; }
        ;

lifetimeE<lifetime>:            // IEEE: [lifetime]
                /*रिक्तानि*/                             { $$ = VLifetime::NONE; }
        |       lifetime                                { $$ = $1; }
        ;

lifetime<lifetime>:             // ==IEEE: lifetime
        //                      // Note lifetime used by members is instead under memberQual
                ySTATIC__ETC                            { $$ = VLifetime::STATIC_EXPLICIT; }
        |       yAUTOMATIC                              { $$ = VLifetime::AUTOMATIC_EXPLICIT; }
        ;

taskId<ग्रन्थिः_FTaskp>:
                id
                        { $$ = new AstTask{$<fl>$, *$1, nullptr};
                          $$->verilogTask(true); }
        //
        |       id/*interface_identifier*/ '.' idAny
                        { $$ = new AstTask{$<fl>$, *$3, nullptr};
                          $$->verilogTask(true);
                          $$->iface_पत्तनम्_Name(*$1); }
        //
        |       packageClassScope id
                        { $$ = new AstTask{$<fl>$, *$2, nullptr};
                          $$->verilogTask(true);
                          $$->classOrPackagep($1);
                          $$->class_विधानम्(true); }
        ;

funcId<ग्रन्थिः_FTaskp>:             // IEEE: function_data_type_or_implicit + part of function_body_declaration
        //                      // IEEE: function_data_type_or_implicit must be expanded here to prevent conflict
        //                      // function_data_type expanded here to prevent conflicts with
        //                      // implicit_type:empty vs data_type:ID
                /**/ fIdScoped
                        { $$ = $1;
                          $$->fvarp(new AstBasicDType{$<fl>1, LOGIC_IMPLICIT}); }
        |       signingE rangeList fIdScoped
                        { $$ = $3;
                          $$->fvarp(GRAMMARP->सङ्कलय_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)); }
        |       signing fIdScoped
                        { $$ = $2;
                          $$->fvarp(new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}); }
        |       दत्त_type_No_Ref fIdScoped
                        { $$ = $2;
                          $$->fvarp($1); }
        |       packageClassScopeE idInstType packed_dimensionListE fIdScoped
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, nullptr};
                          $$ = $4;
                          $$->fvarp(GRAMMARP->createArray(refp, $3, true)); }
        |       packageClassScopeE idInstType parameter_मानम्_assignment_Class packed_dimensionListE fIdScoped
                        { AstRefDType* ध्रुव refp = new AstRefDType{$<fl>2, *$2, $1, $3};
                          $$ = $5;
                          $$->fvarp(GRAMMARP->createArray(refp, $4, true)); }
        //                      // To verilator tasks are the same as void functions (we separately detect time passing)
        |       yVOID taskId
                        { $$ = $2;  // Internals represent it as a task, not a function (TODO cleanup)
                          $$->verilogTask(false);
                          $$->verilogFunction(true); }
        ;

funcIdNew<ग्रन्थिः_FTaskp>:          // IEEE: from class_constructor_declaration
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
                          $$->class_विधानम्(true); }
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
                          $$->iface_पत्तनम्_Name(*$1); }
        //
        |       packageClassScope id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstFunc{$<fl>$, *$2, nullptr, nullptr};
                          $$->verilogFunction(true);
                          $$->class_विधानम्(true);
                          $$->classOrPackagep($1); }
        ;

tfGuts<nodep>:
                '(' tf_पत्तनम्_list_E ')' ';' tfBodyE       { $$ = सङ्कलय_अग्र_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfNewGuts<nodep>:
                '(' class_constructor_arg_listE ')' ';' tfBodyE       { $$ = सङ्कलय_अग्र_Null($2, $5); }
        |       ';' tfBodyE                             { $$ = $2; }
        ;

tfBodyE<nodep>:                 // IEEE: part of function_body_declaration/task_body_declaration
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       tf_item_declarationList                 { $$ = $1; }
        |       tf_item_declarationList stmtList        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        |       stmtList                                { $$ = $1; }
        ;

tf_item_declarationList<nodep>:
                tf_item_declaration                     { $$ = $1; }
        |       tf_item_declarationList tf_item_declaration     { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

tf_item_declaration<nodep>:     // ==IEEE: tf_item_declaration
                खण्डम्_item_declaration                  { $$ = $1; }
        |       tf_पत्तनम्_declaration                     { $$ = $1; }
        |       tf_item_declarationVerilator            { $$ = $1; }
        ;

tf_item_declarationVerilator<nodep>:    // Verilator extensions
                y_VL_लौकिकम्                              { $$ = new AstPragma{$1, VPragmaType::लौकिकम्_TASK}; v3Global.dpi(true); }
        |       yVL_NO_INLINE_TASK                      { $$ = new AstPragma{$1, VPragmaType::NO_INLINE_TASK}; }
        ;

tf_पत्तनम्_list_E<nodep>:           // IEEE: tf_port_list + empty
        //                      // Empty covered by tf_port_item
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(निवेशः); }
        /*cont*/    tf_पत्तनम्_list_List                    { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

tf_पत्तनम्_list_List<nodep>:        // IEEE: part of tf_port_list
                tf_पत्तनम्_item                            { $$ = $1; }
        |       tf_पत्तनम्_list_List ',' tf_पत्तनम्_item       { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

class_constructor_arg_listE<nodep>:  // IEEE: class_constructor_arg_list or empty
                /*empty*/
        /*mid*/         { VARRESET_LIST(UNKNOWN); VARIO(निवेशः); }
        /*cont*/    class_constructor_arg_listList      { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

class_constructor_arg_listList<nodep>:  // IEEE: part of class_constructor_arg_list
                class_constructor_arg                   { $$ = $1; }
        |       class_constructor_arg_listList ',' class_constructor_arg       { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

class_constructor_arg<nodep>:  // ==IEEE: class_constructor_arg
                tf_पत्तनम्_item                            { $$ = $1; }
        |       yDEFAULT                                { $$ = nullptr; BBUNSUP($1, "Unsupported: new constructor 'default' argument"); }
        ;

tf_पत्तनम्_item<nodep>:            // ==IEEE: tf_port_item
        //                      // We split tf_port_item into the type and assignment as don't know what follows a comma
                /*रिक्तानि*/                             { $$ = nullptr; PINNUMINC(); }  // For example a ",," port
        |       tf_पत्तनम्_item_Front tf_पत्तनम्_item_Assignment { $$ = $2; }
        |       tf_पत्तनम्_item_Assignment                  { $$ = $1; }
        ;

tf_पत्तनम्_item_Front:              // IEEE: part of tf_port_item, which has the data type
                दत्त_type                               { VARDTYPE($1); }
        |       signingE rangeList
                        { Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                              new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT, $1}, $2, true);
                          VARDTYPE(dtp); }
        |       signing
                        { Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$<fl>1, LOGIC_IMPLICIT, $1};
                          VARDTYPE(dtp); }
        |       yVAR दत्त_type                          { VARDTYPE($2); }
        |       yVAR implicit_typeE                     { VARDTYPE($2); }
        //
        |       tf_पत्तनम्_item_Dir /*implicit*/            { VARDTYPE(nullptr); /*default_nettype-see spec*/ }
        |       tf_पत्तनम्_item_Dir दत्त_type               { VARDTYPE($2); }
        |       tf_पत्तनम्_item_Dir signingE rangeList
                        { Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                              new AstBasicDType{$3->fileline(), LOGIC_IMPLICIT, $2}, $3, true);
                          VARDTYPE(dtp); }
        |       tf_पत्तनम्_item_Dir signing
                        { Ast_ग्रन्थिः_DType* ध्रुव dtp = new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $2};
                          VARDTYPE(dtp); }
        |       tf_पत्तनम्_item_Dir yVAR दत्त_type          { VARDTYPE($3); }
        |       tf_पत्तनम्_item_Dir yVAR implicit_typeE     { VARDTYPE($3); }
        ;

tf_पत्तनम्_item_Dir:                // IEEE: part of tf_port_item, direction
                पत्तनम्_direction                          { }  // port_direction sets VARIO
        ;

tf_पत्तनम्_item_Assignment<varp>:   // IEEE: part of tf_port_item, which has assignment
                id variable_dimensionListE sigAttrListE व्यञ्जकम्_Eq_E
                        { $$ = VARDONEA($<fl>1, *$1, $2, $3); यदि ($4) $$->valuep($4); }
        ;

parenE:
                /*रिक्तानि*/                             { }
        |       '(' ')'                                 { }
        ;

//      method_call:            // ==IEEE: method_call + method_call_body
//                              // IEEE: method_call_root '.' method_identifier [ '(' list_of_arguments ')' ]
//                              //   "method_call_root '.' method_identifier" looks just like "expr '.' id"
//                              //   "method_call_root '.' method_identifier (...)" looks just like "expr '.' tf_call"
//                              // IEEE: built_in_method_call
//                              //   method_call_root not needed, part of expr resolution
//                              // What's left is below array_methodNoRoot
array_विधानम्_No_मूलम्<ग्रन्थिः_FTask_Refp>:
                yOR                                     { $$ = new AstFuncRef{$1, "or"}; }
        |       yAND                                    { $$ = new AstFuncRef{$1, "and"}; }
        |       yXOR                                    { $$ = new AstFuncRef{$1, "xor"}; }
        |       yUNIQUE                                 { $$ = new AstFuncRef{$1, "unique"}; }
        ;

array_विधानम्_With<ग्रन्थिः_Exprp>:
                array_विधानम्_No_मूलम् parenE               { $$ = $1; }
        |       array_विधानम्_No_मूलम् parenE yWITH__PAREN '(' व्यञ्जकम् ')'
                        { $$ = new AstWithParse{$3, $1, $5}; }
        |       array_विधानम्_No_मूलम् '(' व्यञ्जकम् ')' yWITH__PAREN '(' व्यञ्जकम् ')'
                        { $$ = new AstWithParse{$5, $1, $7}; $1->सङ्कलय_Argsp(new AstArg{$<fl>3, "", $3}); }
        ;

dpi_import_export<nodep>:       // ==IEEE: dpi_import_export
                yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype ';'
                        { $$ = $5;
                          यदि (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_विशुद्धम्($3 == iprop_विशुद्धम्);
                          $5->dpiImport(true);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE function_prototype yVL_DPI_C_DECL yaSTRING ';'
                        { $$ = $5;
                          यदि (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_विशुद्धम्($3 == iprop_विशुद्धम्);
                          $5->dpiImport(true);
                          यदि (*$7 != "") $5->dpiCDecl(*$7);
                          GRAMMARP->checkDpiVer($1, *$2); v3Global.dpi(true); }
        |       yIMPORT yaSTRING dpi_tf_import_propertyE dpi_importLabelE task_prototype ';'
                        { $$ = $5;
                          यदि (*$4 != "") $5->cname(*$4);
                          $5->dpiContext($3 == iprop_CONTEXT);
                          $5->dpi_विशुद्धम्($3 == iprop_विशुद्धम्);
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
                /*रिक्तानि*/                             { स्थिर string s; $$ = &s; }
        |       idAny/*c_identifier*/ '='               { $$ = $1; $<fl>$ = $<fl>1; }
        ;

dpi_tf_import_propertyE<iprop>: // IEEE: [ dpi_function_import_property + dpi_task_import_property ]
                /*रिक्तानि*/                             { $$ = iprop_NONE; }
        |       yCONTEXT                                { $$ = iprop_CONTEXT; }
        |       y_विशुद्धम्                                   { $$ = iprop_विशुद्धम्; }
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

व्यञ्जकम्_Eq_E<ग्रन्थिः_Exprp>:             // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' व्यञ्जकम्                                { $$ = $2; }
        ;

व्यञ्जकम्_Or_दत्त_Type_Eq_E<nodep>:       // IEEE: optional '=' expression (part of param_assignment)
        //                      // constant_param_expression: '$' is in expr
                /*empty*/                               { $$ = nullptr; }
        |       '=' व्यञ्जकम्_Or_दत्त_Type                      { $$ = $2; }
        ;

const_व्यञ्जकम्<ग्रन्थिः_Exprp>:
                व्यञ्जकम्                                    { $$ = $1; }
        ;

व्यञ्जकम्_E<ग्रन्थिः_Exprp>:               // IEEE: optional expression
                /*empty*/                               { $$ = nullptr; }
        |       व्यञ्जकम्                                    { $$ = $1; }
        ;

व्यञ्जकम्<ग्रन्थिः_Exprp>:                // IEEE: part of expression/constant_expression/primary
        // *SEE BELOW*          // IEEE: primary/constant_primary
        //
        //                      // IEEE: unary_operator primary
                '+' ~r~व्यञ्जकम्     %prec prUNARYARITH      { $$ = $2; }
        |       '-' ~r~व्यञ्जकम्     %prec prUNARYARITH      { $$ = new AstNegate{$1, $2}; }
        |       '!' ~r~व्यञ्जकम्     %prec prNEGATION        { $$ = new AstLogNot{$1, $2}; }
        |       '&' ~r~व्यञ्जकम्     %prec prREDUCTION       { $$ = new AstRedAnd{$1, $2}; }
        |       '~' ~r~व्यञ्जकम्     %prec prNEGATION        { $$ = new AstNot{$1, $2}; }
        |       '|' ~r~व्यञ्जकम्     %prec prREDUCTION       { $$ = new AstRedOr{$1, $2}; }
        |       '^' ~r~व्यञ्जकम्     %prec prREDUCTION       { $$ = new AstRedXor{$1, $2}; }
        |       yP_NAND ~r~व्यञ्जकम् %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedAnd{$1, $2}}; }
        |       yP_NOR  ~r~व्यञ्जकम् %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedOr{$1, $2}}; }
        |       yP_XNOR ~r~व्यञ्जकम् %prec prREDUCTION       { $$ = new AstLogNot{$1, new AstRedXor{$1, $2}}; }
        //
        //                      // IEEE: inc_or_dec_expression
        |       ~l~inc_or_dec_expression                { $<fl>$ = $<fl>1; $$ = $1; }
        //
        //                      // IEEE: '(' operator_assignment ')'
        //                      // Need exprScope of variable_lvalue to prevent conflict
        |       '(' ~p~व्यञ्जकम्_Scope '='          व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, $4},
                                               $2->clone_वृक्षः_विशुद्धम्(true)};
                          ASSIGNEQEXPR($<fl>3); }
        |       '(' ~p~व्यञ्जकम्_Scope yP_PLUSEQ    व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new Ast_सङ्कलय{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_MINUSEQ   व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstSub{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_TIMESEQ   व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstMul{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_DIVEQ     व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstDiv{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_MODEQ     व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstModDiv{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_ANDEQ     व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstAnd{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_OREQ      व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstOr{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_XOREQ     व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new AstXor{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_SLEFTEQ   व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new Ast_सरणम्_L{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_SRIGHTEQ  व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new Ast_सरणम्_R{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        |       '(' ~p~व्यञ्जकम्_Scope yP_SSRIGHTEQ व्यञ्जकम् ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$1, new Ast_नियोजय{$3, $2, new Ast_सरणम्_RS{$3, $2->clone_वृक्षः_विशुद्धम्(true), $4}},
                                               $2->clone_वृक्षः_विशुद्धम्(true)}; }
        //
        //                      // IEEE: expression binary_operator expression
        |       ~l~व्यञ्जकम् '+' ~r~व्यञ्जकम्                     { $$ = new Ast_सङ्कलय{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '-' ~r~व्यञ्जकम्                     { $$ = new AstSub{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '*' ~r~व्यञ्जकम्                     { $$ = new AstMul{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '/' ~r~व्यञ्जकम्                     { $$ = new AstDiv{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '%' ~r~व्यञ्जकम्                     { $$ = new AstModDiv{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_EQUAL ~r~व्यञ्जकम्                { $$ = new AstEq{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_NOTEQUAL ~r~व्यञ्जकम्             { $$ = new AstNeq{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_CASEEQUAL ~r~व्यञ्जकम्            { $$ = new AstEqCase{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_CASENOTEQUAL ~r~व्यञ्जकम्         { $$ = new AstNeqCase{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_WILDEQUAL ~r~व्यञ्जकम्            { $$ = new AstEqWild{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_WILDNOTEQUAL ~r~व्यञ्जकम्         { $$ = new AstNeqWild{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_ANDAND ~r~व्यञ्जकम्               { $$ = new AstLogAnd{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_OROR ~r~व्यञ्जकम्                 { $$ = new AstLogOr{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_POW ~r~व्यञ्जकम्                  { $$ = new AstPow{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '<' ~r~व्यञ्जकम्                     { $$ = new AstLt{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '>' ~r~व्यञ्जकम्                     { $$ = new AstGt{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_GTE ~r~व्यञ्जकम्                  { $$ = new AstGte{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '&' ~r~व्यञ्जकम्                     { $$ = new AstAnd{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '|' ~r~व्यञ्जकम्                     { $$ = new AstOr{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् '^' ~r~व्यञ्जकम्                     { $$ = new AstXor{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_XNOR ~r~व्यञ्जकम्                 { $$ = new AstNot{$2, new AstXor{$2, $1, $3}}; }
        |       ~l~व्यञ्जकम् yP_NOR ~r~व्यञ्जकम्
                        { $$ = new AstNot{$2, new AstOr{$2, $1, $3}};
                          $2->v3error("Syntax error: '~|' is not a 'nor' binary operator, but a unary '~ |'"
                                      << YYNEWLINE << $<fl>1->warn_अधिकम्() << "... Suggest '~ (... | ...)'"); }
        |       ~l~व्यञ्जकम् yP_NAND ~r~व्यञ्जकम्
                        { $$ = new AstNot{$2, new AstAnd{$2, $1, $3}};
                          $2->v3error("Syntax error: '~&' is not a 'nand' binary operator, but a unary '~ &'"
                                      << YYNEWLINE << $<fl>1->warn_अधिकम्() << "... Suggest '~ (... & ...)'"); }
        |       ~l~व्यञ्जकम् yP_SLEFT ~r~व्यञ्जकम्                { $$ = new Ast_सरणम्_L{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_SRIGHT ~r~व्यञ्जकम्               { $$ = new Ast_सरणम्_R{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_SSRIGHT ~r~व्यञ्जकम्              { $$ = new Ast_सरणम्_RS{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yP_LTMINUSGT ~r~व्यञ्जकम्            { $$ = new AstLogEq{$2, $1, $3}; }
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
        |       ~l~व्यञ्जकम् yP_MINUSGT ~r~व्यञ्जकम्              { $$ = new AstLogIf{$2, $1, $3}; }
        //
        //                      // <= is special, as we need to disambiguate it with <= assignment
        //                      // We copy all of expr to fexpr and rename this token to a fake one.
        |       ~l~व्यञ्जकम् yP_LTE~f__IGNORE~ ~r~व्यञ्जकम्       { $$ = new AstLte{$2, $1, $3}; }
        //
        //                      // IEEE: conditional_expression
        |       ~l~व्यञ्जकम् '?' ~r~व्यञ्जकम् ':' ~r~व्यञ्जकम्         { $$ = new AstCond{$2, $1, $3, $5}; }
        //
        //                      // IEEE: inside_expression
        |       ~l~व्यञ्जकम् yINSIDE '{' range_list '}'      { $$ = new AstInside{$2, $1, $4}; }
        //
        //                      // IEEE: tagged_union_expression
        //                      // yTAGGED__NONPRIMARY = tokenPipeline determined no primary follows
        |       yTAGGED__NONPRIMARY idAny/*member*/ %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, nullptr}; }
        //                      // yTAGGED = primary follows; handle specific primary types
        //                      // Parenthesized expression
        |       yTAGGED idAny/*member*/ '(' व्यञ्जकम् ')' %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, $4}; }
        //                      // Assignment patterns like tagged Add '{a, b, c}
        |       yTAGGED idAny/*member*/ assignment_आदर्शः %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, $3}; }
        //                      // Integer literal
        |       yTAGGED idAny/*member*/ yaINTNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, new AstConst{$<fl>3, *$3}}; }
        //                      // Float literal
        |       yTAGGED idAny/*member*/ yaFLOATNUM %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, new AstConst{$<fl>3, AstConst::RealDouble{}, $3}}; }
        //                      // String literal
        |       yTAGGED idAny/*member*/ yaSTRING %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, new AstConst{$<fl>3, AstConst::VerilogStringLiteral{}, *$3}}; }
        //                      // null literal
        |       yTAGGED idAny/*member*/ yNULL %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, new AstConst{$3, AstConst::Null{}}}; }
        //                      // Identifier as value
        |       yTAGGED idAny/*member*/ idAny %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, new AstParseRef{$<fl>3, *$3, nullptr, nullptr}}; }
        //                      // Concatenation
        |       yTAGGED idAny/*member*/ '{' cateList '}' %prec prTAGGED
                        { $$ = new Ast_Tagged_व्यञ्जकम्{$1, *$2, $4}; }
        //
        //======================// IEEE: primary/constant_primary
        //
        //                      // IEEE: primary_literal (minus string, which is handled specially)
        |       yaINTNUM                                { $$ = new AstConst{$<fl>1, *$1}; }
        |       yaFLOATNUM                              { $$ = new AstConst{$<fl>1, AstConst::RealDouble{}, $1}; }
        |       time_अङ्कः_Adjusted                         { $$ = $1; }
        |       strAsInt~noStr__IGNORE~                 { $$ = $1; }
        //
        //                      // IEEE: "... hierarchical_identifier select"  see below
        //
        //                      // IEEE: empty_unpacked_array_concatenation
        //                      // IEEE: (aka empty_queue, empty_unpacked_array_concatenation)
        |       '{' '}'                                 { $$ = new Ast_रिक्तानि_Queue{$1}; }
        //
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Part of exprOkLvalue below
        //
        //                      // IEEE: multiple_concatenation/constant_multiple_concatenation
        |       '{' const_व्यञ्जकम् '{' cateList '}' '}'
                        { $$ = new AstReplicate{$3, $4, $2}; }
        |       '{' const_व्यञ्जकम् '{' cateList '}' '}' '[' व्यञ्जकम् ']'
                        { $$ = new AstSelBit{$7, new AstReplicate{$3, $4, $2}, $8}; }
        |       '{' const_व्यञ्जकम् '{' cateList '}' '}' '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                        { $$ = new AstSelExtract{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_व्यञ्जकम् '{' cateList '}' '}' '[' व्यञ्जकम् yP_PLUSCOLON const_व्यञ्जकम् ']'
                        { $$ = new AstSelPlus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        |       '{' const_व्यञ्जकम् '{' cateList '}' '}' '[' व्यञ्जकम् yP_MINUSCOLON const_व्यञ्जकम् ']'
                        { $$ = new AstSelMinus{$7, new AstReplicate{$3, $4, $2}, $8, $10}; }
        //                      // UNSUP some other rules above
        //
        //                      // IEEE grammar error: function_subroutine_call [ range_expression ]
        //                      // should be instead:  function_subroutine_call [ part_select_range ]
        |       function_subroutine_call_No_विधानम्
                        { $$ = $1; }
        |       function_subroutine_call_No_विधानम् part_select_rangeList
                        { $$ = GRAMMARP->scrubSel($1, $2); }
        //                      // method_call
        |       ~l~व्यञ्जकम् '.' function_subroutine_call_No_विधानम्
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~व्यञ्जकम् '.' function_subroutine_call_No_विधानम् part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //                      // method_call:array_method requires a '.'
        |       ~l~व्यञ्जकम् '.' array_विधानम्_With
                        { $$ = new AstDot{$2, false, $1, $3}; }
        |       ~l~व्यञ्जकम् '.' array_विधानम्_With part_select_rangeList
                        { $$ = GRAMMARP->scrubSel(new AstDot{$2, false, $1, $3}, $4); }
        //
        //                      // IEEE: let_expression
        //                      // see funcRef
        //
        //                      // IEEE: '(' mintypmax_expression ')'
        |       ~noPar__IGNORE~'(' व्यञ्जकम् ')'             { $$ = $2; }
        |       ~noPar__IGNORE~'(' व्यञ्जकम् ':' व्यञ्जकम् ':' व्यञ्जकम् ')'
                        { $$ = $4; MINTYPMAXDLYUNSUP($4); DEL($2); DEL($6); }
        //                      // PSL rule
        |       '_' '(' व्यञ्जकम् ')'                        { $$ = $3; }    // Arbitrary Verilog inside PSL
        //
        //                      // IEEE: cast/constant_cast
        //                      // expanded from casting_type
        |       simple_typeNoRef yP_TICK '(' व्यञ्जकम् ')'
                        { $$ = new AstCast{$2, $4, VFlagChildDType{}, $1}; }
        //                      // expanded from simple_type ps_type_identifier (part of simple_type)
        //                      // expanded from simple_type ps_parameter_identifier (part of simple_type)
        //                      // Causes conflict, so handled post-parse
        //                      // NO: packageClassScopeE idType yP_TICK '(' expr ')'
        //
        |       yTYPE__ETC '(' व्यञ्जकम्_Or_दत्त_Type ')' yP_TICK '(' व्यञ्जकम् ')'
                        { $$ = new AstCast{$1, $7, VFlagChildDType{},
                                           new AstRefDType{$1, AstRefDType::Flag_Type_सम्बन्धि_व्यञ्जकम्{}, $3}}; }
        |       ySIGNED yP_TICK '(' व्यञ्जकम् ')'            { $$ = new AstSigned{$1, $4}; }
        |       yUNSIGNED yP_TICK '(' व्यञ्जकम् ')'          { $$ = new AstUnsigned{$1, $4}; }
        |       ySTRING yP_TICK '(' व्यञ्जकम् ')'            { $$ = new AstCvtPackString{$1, $4}; }
        |       yCONST__ETC yP_TICK '(' व्यञ्जकम् ')'        { $$ = $4; }  // Not linting const presently
        //                      // Spec only allows primary with addition of a type reference
        //                      // We'll be more general, and later assert LHS was a type.
        |       ~l~व्यञ्जकम् yP_TICK '(' व्यञ्जकम् ')'            { $$ = new AstCastParse{$2, $4, $1}; }
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
        |       ~l~व्यञ्जकम्_Ok_Lvalue                         { $$ = $1; }
        //
        //----------------------
        //
        //                      // IEEE: cond_predicate - here to avoid reduce problems
        //                      // Note expr includes cond_pattern
        |       ~l~व्यञ्जकम् yP_ANDANDAND ~r~व्यञ्जकम्            { $$ = new AstConst{$2, AstConst::BitFalse{}};
                                                          BBUNSUP($<fl>2, "Unsupported: &&& expression"); }
        //
        //                      // IEEE: cond_pattern - here to avoid reduce problems
        //                      // "expr yMATCHES pattern"
        //                      // IEEE: pattern - expanded here to avoid conflicts
        |       ~l~व्यञ्जकम् yMATCHES आदर्शः_No_व्यञ्जकम्          { $$ = new AstMatches{$2, $1, $3}; }
        |       ~l~व्यञ्जकम् yMATCHES ~r~व्यञ्जकम्                { $$ = new AstMatches{$2, $1, $3}; }
        //
        //                      // IEEE: expression_or_dist - here to avoid reduce problems
        //                      // "expr yDIST '{' dist_list '}'"
        |       ~l~व्यञ्जकम् yDIST '{' dist_list '}'         { $$ = new AstDist{$2, $1, $4}; }
        ;

fexpr<ग्रन्थिः_Exprp>:                   // For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(व्यञ्जकम्,{s/~l~/f/g; s/~r~/f/g; s/~f__IGNORE~/__IGNORE/g;})   // {copied}
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

व्यञ्जकम्_No_Str<ग्रन्थिः_Exprp>:               // expression with string removed
                BISONPRE_COPY(व्यञ्जकम्,{s/~noStr__IGNORE~/Ignore/g;})       // {copied}
        ;

व्यञ्जकम्_Ok_Lvalue<ग्रन्थिः_Exprp>:            // expression that's also OK to use as a variable_lvalue
                ~l~व्यञ्जकम्_Scope                            { $$ = $1; }
        //                      // IEEE: concatenation/constant_concatenation
        //                      // Replicate(1) required as otherwise "{a}" would not be self-determined
        |       '{' cateList '}'                        { $$ = new AstReplicate{$1, $2, 1}; }
        |       '{' cateList '}' '[' व्यञ्जकम् ']'           { $$ = new AstSelBit{$4, new AstReplicate{$1, $2, 1}, $5}; }
        |       '{' cateList '}' '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                                                        { $$ = new AstSelExtract{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' व्यञ्जकम् yP_PLUSCOLON const_व्यञ्जकम् ']'
                                                        { $$ = new AstSelPlus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        |       '{' cateList '}' '[' व्यञ्जकम् yP_MINUSCOLON const_व्यञ्जकम् ']'
                                                        { $$ = new AstSelMinus{$4, new AstReplicate{$1, $2, 1}, $5, $7}; }
        //                      // IEEE: assignment_pattern_expression
        //                      // IEEE: [ assignment_pattern_expression_type ] == [ ps_type_id /ps_paremeter_id/data_type]
        //                      // We allow more here than the spec requires
        //UNSUP ~l~exprScope assignment_pattern         { UNSUP }
        |       दत्त_type assignment_आदर्शः            { $$ = $2; यदि ($2) $2->childDTypep($1); }
        |       assignment_आदर्शः                      { $$ = $1; }
        //
        |       streaming_concatenation                 { $$ = $1; }
        ;

fexprOkLvalue<ग्रन्थिः_Exprp>:           // exprOkLValue, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(व्यञ्जकम्_Ok_Lvalue,{s/~l~/f/g}) // {copied}
        ;

sexprOkLvalue<ग्रन्थिः_Exprp>:  // exprOkLValue, For use by sequence_expr
                BISONPRE_COPY(व्यञ्जकम्_Ok_Lvalue,{s/~l~/s/g}) // {copied}
        ;

pexprOkLvalue<ग्रन्थिः_Exprp>:  // exprOkLValue, For use by property_expr
                BISONPRE_COPY(व्यञ्जकम्_Ok_Lvalue,{s/~l~/p/g}) // {copied}
        ;

//UNSUPev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/ev_/g})       // {copied}
//UNSUP ;

//UNSUPpev_exprOkLvalue<nodeExprp>:  // exprOkLValue, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprOkLvalue,{s/~l~/pev_/g})      // {copied}
//UNSUP ;

fexprLvalue<ग्रन्थिः_Exprp>:             // For use as first part of statement (disambiguates <=)
                fexprOkLvalue                           { $<fl>$ = $<fl>1; $$ = $1; }
        ;

व्यञ्जकम्_Scope<ग्रन्थिः_Exprp>:               // scope and variable for use to inside an expression
        //                      // Here we've split method_call_root | implicit_class_handle | class_scope | package_scope
        //                      // from the object being called and let expr's "." deal with resolving it.
        //                      // (note method_call_root was simplified to require a primary in 1800-2009)
        //
        //                      // IEEE: [ implicit_class_handle . | class_scope | package_scope ] hierarchical_identifier select
        //                      // Or method_call_body without parenthesis
        //                      // See also varRefClassBit, which is the non-expr version of most of this
                yTHIS                                   { $$ = new AstParseRef{$<fl>1, "this"}; }
        |       y_D_मूलम्                                 { $$ = new AstParseRef{$<fl>1, "$root"}; }
        |       idArrayed                               { $$ = $1; }
        |       packageClassScope idArrayed             { $$ = AstDot::newIfPkg($2->fileline(), $1, $2); }
        |       ~l~व्यञ्जकम् '.' idArrayed                   { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        //                      // expr below must be a "yTHIS"
        |       ~l~व्यञ्जकम् '.' ySUPER
                        { AstParseRef* ध्रुव anodep = VN_CAST($1, ParseRef);
                          यदि (anodep && anodep->name() == "this") {
                              $$ = new AstParseRef{$<fl>1, "super"};
                              $1->delete_वृक्षः();
                          } अन्यथा {
                              $$ = $1;  $$->v3error("Syntax error: 'super' must be first name component, or after 'this.'");
                          }
                        }
        //                      // Part of implicit_class_handle
        |       ySUPER                                  { $$ = new AstParseRef{$<fl>1, "super"}; }
        ;

fexprScope<ग्रन्थिः_Exprp>:              // exprScope, For use as first part of statement (disambiguates <=)
                BISONPRE_COPY(व्यञ्जकम्_Scope,{s/~l~/f/g})    // {copied}
        ;

sexprScope<ग्रन्थिः_Exprp>:  // exprScope, For use by sequence_expr
                BISONPRE_COPY(व्यञ्जकम्_Scope,{s/~l~/s/g})    // {copied}
        ;

pexprScope<ग्रन्थिः_Exprp>:  // exprScope, For use by property_expr
                BISONPRE_COPY(व्यञ्जकम्_Scope,{s/~l~/p/g})    // {copied}
        ;

//UNSUPev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/ev_/g})  // {copied}
//UNSUP ;

//UNSUPpev_exprScope<nodeExprp>:  // exprScope, For use by ev_expr
//UNSUP         BISONPRE_COPY(exprScope,{s/~l~/pev_/g}) // {copied}
//UNSUP ;

// PLI calls exclude "" as integers, they're strings
// For $c("foo","bar") we want "bar" as a string, not a Verilog integer.
व्यञ्जकम्_Str_Text<nodep>:
                व्यञ्जकम्_No_Str                               { $$ = $1; }
        |       yaSTRING                                { $$ = new AstText{$<fl>1, GRAMMARP->textQuoted($<fl>1, *$1)}; }
        ;

व्यञ्जकम्_Type_Compare<ग्रन्थिः_Exprp>:
                व्यञ्जकम्                                    { $$ = $1; }
        |       type_referenceBoth                      { $$ = $1; }
        ;

cStrList<nodep>:
                व्यञ्जकम्_Str_Text                             { $$ = $1; }
        |       व्यञ्जकम्_Str_Text ',' cStrList                { $$ = $1->सङ्कलय_अग्र($3); }
        ;

cateList<ग्रन्थिः_Exprp>:
        //                      // Not just 'expr' to prevent conflict via stream_concOrExprOrType
                प्रवाहः_expression                       { $$ = $1; }
        |       cateList ',' प्रवाहः_expression          { $$ = new AstConcat{$2, $1, $3}; }
        ;

व्यञ्जकम्_List_E<ग्रन्थिः_Exprp>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       व्यञ्जकम्_List                                { $$ = $1; }
        ;

व्यञ्जकम्_List<ग्रन्थिः_Exprp>:
                व्यञ्जकम्                                    { $$ = $1; }
        |       व्यञ्जकम्_List ',' व्यञ्जकम्                       { $$ = $1->सङ्कलय_अग्र($3); }
        ;

// identifier_list for 'with' in inline randomize constraints (IEEE 1800-2023 18.7).
// Only simple identifiers; non-identifier expressions are parse errors.
inlineConstraintIdList<ग्रन्थिः_Exprp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        |       inlineConstraintIdList ',' id           { $$ = $1->सङ्कलय_अग्र(
                                                             new AstParseRef{$<fl>3, *$3, nullptr, nullptr}); }
        ;

// Optional identifier_list. Empty 'with () {...}' differs from bare 'with {...}'
// via AstWithParse::restricted().
inlineConstraintIdListE<ग्रन्थिः_Exprp>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       inlineConstraintIdList                  { $$ = $1; }
        ;

व्यञ्जकम्_EList_E<nodep>:  // expression list with empty commas allowed
                व्यञ्जकम्_E                                   { $$ = $1; }
        |       व्यञ्जकम्_EList_E ',' व्यञ्जकम्_E                    { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

व्यञ्जकम्_Disp_List<ग्रन्थिः_Exprp>:            // exprList for within $display
                व्यञ्जकम्                                    { $$ = $1; }
        |       व्यञ्जकम्_Disp_List ',' व्यञ्जकम्                   { $$ = $1->सङ्कलय_अग्र($3); }
        //                      // ,, creates a space in $display
        |       व्यञ्जकम्_Disp_List ',' /*empty*/
                        { $$ = $1->सङ्कलय_अग्र(new AstConst{$<fl>2, AstConst::VerilogStringLiteral{}, " "}); }
        ;

vrdList<ग्रन्थिः_Exprp>:
                idClassSel                              { $$ = $1; }
        |       vrdList ',' idClassSel                  { $$ = $1->सङ्कलय_अग्र($3); }
        ;

commasE:
                /*रिक्तानि*/                             { } /* ignored */
        |       ',' commasE                             { } /* ignored */
        ;

commaVRDListE<ग्रन्थिः_Exprp>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       ',' vrdList                             { $$ = $2; }
        ;

तर्काः_व्यञ्जकम्_List<ग्रन्थिः_Exprp>:        // IEEE: part of list_of_arguments (used where ,, isn't legal)
                व्यञ्जकम्                                    { $$ = $1; }
        |       तर्काः_व्यञ्जकम्_List ',' व्यञ्जकम्                   { $$ = $1->सङ्कलय_अग्र($3); }
        ;

तर्काः_व्यञ्जकम्_List_E<argp>:       // IEEE: part of list_of_arguments
                तर्काः_व्यञ्जकम्_One_E                            { $$ = $1; }
        |       तर्काः_व्यञ्जकम्_List_E ',' तर्काः_व्यञ्जकम्_One_E          { $$ = $1->सङ्कलय_अग्र($3); }
        ;

//UNSUPpev_argsExprListE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsExprOneE                        { $$ = $1; }
//UNSUP |       pev_argsExprListE ',' pev_argsExprOneE  { $$ = addNextNull($1, $3); }
//UNSUP ;

तर्काः_व्यञ्जकम्_One_E<argp>:        // IEEE: part of list_of_arguments
                /*empty*/                               { $$ = new AstArg{CRELINE(), "", nullptr}; }
        |       व्यञ्जकम्                                    { $$ = new AstArg{$1->fileline(), "", $1}; }
        ;

//UNSUPpev_argsExprOneE<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         /*empty*/                               { $$ = nullptr; }       // ,, is legal in list_of_arguments
//UNSUP |       pev_expr                                { $$ = $1; }
//UNSUP ;

तर्काः_Dotted_List<argp>:      // IEEE: part of list_of_arguments
                तर्काः_Dotted                              { $$ = $1; }
        |       तर्काः_Dotted_List ',' तर्काः_Dotted           { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

//UNSUPpev_argsDottedList<nodeExprp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         pev_argsDotted                          { $$ = $1; }
//UNSUP |       pev_argsDottedList ',' pev_argsDotted   { $$ = addNextNull($1, $3); }
//UNSUP ;

तर्काः_Dotted<argp>:          // IEEE: part of list_of_arguments
                '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
        |       '.' idAny '(' व्यञ्जकम् ')'                  { $$ = new AstArg{$<fl>2, *$2, $4}; }
        ;

//UNSUPpev_argsDotted<argp>:  // IEEE: part of list_of_arguments - pev_expr at bottom
//UNSUP         '.' idAny '(' ')'                       { $$ = new AstArg{$<fl>2, *$2, nullptr}; }
//UNSUP |       '.' idAny '(' pev_expr ')'              { $$ = new AstArg{$<fl>2, *$2, $4}; }
//UNSUP ;

streaming_concatenation<ग्रन्थिः_Streamp>: // ==IEEE: streaming_concatenation
        //                      // Need to disambiguate {<< expr-{ ... expr-} stream_concat }
        //                      // From                 {<< stream-{ ... stream-} }
        //                      // Likewise simple_type's idScoped from constExpr's idScope
        //                      // Thus we allow always any two operations.  Sorry
        //                      // IEEE: "'{' yP_SL/R             stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R simple_type stream_concatenation '}'"
        //                      // IEEE: "'{' yP_SL/R constExpr   stream_concatenation '}'"
                '{' yP_SLEFT  प्रवाहः_concatenation '}'
                        { $$ = new Ast_प्रवाहः_L{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SRIGHT प्रवाहः_concatenation '}'
                        { $$ = new Ast_प्रवाहः_R{$2, $3, new AstConst{$2, 1}}; }
        |       '{' yP_SLEFT  प्रवाहः_expression_Or_दत्त_Type प्रवाहः_concatenation '}'
                        { $$ = new Ast_प्रवाहः_L{$2, $4, new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_द्वयङ्काः_OR_सङ्ख्या, $3}}; }
        |       '{' yP_SRIGHT प्रवाहः_expression_Or_दत्त_Type प्रवाहः_concatenation '}'
                        { $$ = new Ast_प्रवाहः_R{$2, $4, new Ast_Attr_सम्बन्धि{$1, VAttrType::DIM_द्वयङ्काः_OR_सङ्ख्या, $3}}; }
        ;

प्रवाहः_concatenation<ग्रन्थिः_Exprp>:    // ==IEEE: stream_concatenation
        //                      // '{' { stream_expression } '}'
                '{' cateList '}'                        { $$ = $2; }
        ;

प्रवाहः_expression<ग्रन्थिः_Exprp>:   // ==IEEE: stream_expression
        //                      // IEEE: array_range_expression expanded below
                व्यञ्जकम्                                    { $$ = $1; }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् ':' व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् yP_PLUSCOLON  व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् yP_MINUSCOLON व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        ;

प्रवाहः_expression_Or_दत्त_Type<nodep>:     // IEEE: from streaming_concatenation
                व्यञ्जकम्_Or_दत्त_Type                          { $$ = $1; }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् ':' व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् yP_PLUSCOLON  व्यञ्जकम् ']'
                        { $$ = $1; BBUNSUP($2, "Unsupported: with[] stream expression"); }
        |       व्यञ्जकम् yWITH__BRA '[' व्यञ्जकम् yP_MINUSCOLON व्यञ्जकम् ']'
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
                yLET letId '=' व्यञ्जकम् ';'
                        { $$ = $2;
                          $$->सङ्कलय_Stmtsp(new Ast_Stmt_व्यञ्जकम्{$1, $4}); }
        |       yLET letId '(' let_पत्तनम्_list_E ')' '=' व्यञ्जकम् ';'
                        { $$ = $2;
                          $$->सङ्कलय_Stmtsp(new Ast_Stmt_व्यञ्जकम्{$1, $7});
                          $$->सङ्कलय_Stmtsp($4); }
        ;

let_पत्तनम्_list_E<nodep>:   // IEEE: [ let_port_list ]
                /*empty*/                               { $$ = nullptr; }
        |       /*emptyStart*/
        /*mid*/         { VARRESET_LIST(VAR); VARIO(INOUT); }
        /*cont*/  let_पत्तनम्_list                         { $$ = $2; VARRESET_NONLIST(UNKNOWN); }
        ;

let_पत्तनम्_list<nodep>:   // IEEE: let_port_list
                let_पत्तनम्_item                           { $$ = $1; }
        |       let_पत्तनम्_list ',' let_पत्तनम्_item         { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

let_पत्तनम्_item<varp>:  // IEEE: let_port_Item
        //                      // IEEE: Expanded let_formal_type
                yUNTYPED idAny/*formal_port_identifier*/ variable_dimensionListE व्यञ्जकम्_Eq_E
                        { $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlagChildDType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          यदि ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       दत्त_type idAny/*formal_port_identifier*/ variable_dimensionListE व्यञ्जकम्_Eq_E
                        { BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlagChildDType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          यदि ($4) $$->valuep($4);
                          PINNUMINC(); }
        |       implicit_typeE id/*formal_port_identifier*/ variable_dimensionListE व्यञ्जकम्_Eq_E
                        { यदि ($1) BBUNSUP($<fl>1, "Unsupported: let typed ports");
                          $$ = new AstVar{$<fl>2, VVarType::VAR, *$2, VFlagChildDType{},
                                          new AstBasicDType{$<fl>2, LOGIC_IMPLICIT}};
                          $$->direction(VDirection::INOUT);
                          $$->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                          यदि ($4) $$->valuep($4);
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
        |       gateBufList ',' gateBuf                 { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateBufif0List<nodep>:
                gateBufif0                              { $$ = $1; }
        |       gateBufif0List ',' gateBufif0           { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateBufif1List<nodep>:
                gateBufif1                              { $$ = $1; }
        |       gateBufif1List ',' gateBufif1           { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateNotList<nodep>:
                gateNot                                 { $$ = $1; }
        |       gateNotList ',' gateNot                 { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateNotif0List<nodep>:
                gateNotif0                              { $$ = $1; }
        |       gateNotif0List ',' gateNotif0           { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateNotif1List<nodep>:
                gateNotif1                              { $$ = $1; }
        |       gateNotif1List ',' gateNotif1           { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateAndList<nodep>:
                gateAnd                                 { $$ = $1; }
        |       gateAndList ',' gateAnd                 { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateNandList<nodep>:
                gateNand                                { $$ = $1; }
        |       gateNandList ',' gateNand               { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateOrList<nodep>:
                gateOr                                  { $$ = $1; }
        |       gateOrList ',' gateOr                   { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateNorList<nodep>:
                gateNor                                 { $$ = $1; }
        |       gateNorList ',' gateNor                 { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateXorList<nodep>:
                gateXor                                 { $$ = $1; }
        |       gateXorList ',' gateXor                 { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateXnorList<nodep>:
                gateXnor                                { $$ = $1; }
        |       gateXnorList ',' gateXnor               { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gatePullupList<nodep>:
                gatePullup                              { $$ = $1; }
        |       gatePullupList ',' gatePullup           { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gatePulldownList<nodep>:
                gatePulldown                            { $$ = $1; }
        |       gatePulldownList ',' gatePulldown       { $$ = $1->सङ्कलय_अग्र($3); }
        ;
gateUnsupList<nodep>:
                gateUnsup                               { $$ = $1; }
        |       gateUnsupList ',' gateUnsup             { $$ = $1->सङ्कलय_अग्र($3); }
        ;

gateRangeE<nodep>:
                instRangeListE                          { $$ = $1; GATERANGE(GRAMMARP->scrubRange($1)); }
        ;

gateBuf<nodep>:
                gateFront variable_lvalue ',' व्यञ्जकम्_List ')'
                        { Ast_ग्रन्थिः_व्यञ्जकम्* inp = $4;
                          यावत् (inp->nextp()) inp = VN_AS(inp->nextp(), ग्रन्थिः_व्यञ्जकम्);
                          $$ = new AstImplicit{$<fl>1, inp->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = GRAMMARP->createGatePin(inp->clone_वृक्षः(false));
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                              Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinRhsp = GRAMMARP->createGatePin(inp->clone_वृक्षः(false));
                              Ast_नियोजय_W* ध्रुव pinAssp = new Ast_नियोजय_W{$<fl>1, outp->clone_वृक्षः(false), pinRhsp};
                              $$->सङ्कलय_अग्र(new AstAlways{pinAssp});
                          }
                          DEL($1); DEL($4); }
        ;
gateNot<nodep>:
                gateFront variable_lvalue ',' व्यञ्जकम्_List ')'
                        { Ast_ग्रन्थिः_व्यञ्जकम्* inp = $4;
                          यावत् (inp->nextp()) inp = VN_AS(inp->nextp(), ग्रन्थिः_व्यञ्जकम्);
                          $$ = new AstImplicit{$<fl>1, inp->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_वृक्षः(false))};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* outp = $4; outp->nextp(); outp = VN_CAST(outp->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                              Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinRhsp = new AstNot{$<fl>1, GRAMMARP->createGatePin(inp->clone_वृक्षः(false))};
                              Ast_नियोजय_W* ध्रुव pinAssp = new Ast_नियोजय_W{$<fl>1, outp->clone_वृक्षः(false), pinRhsp};
                              $$->सङ्कलय_अग्र(new AstAlways{pinAssp});
                          }
                          DEL($1, $4); }
        ;
gateBufif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_व्यञ्जकम् ',' gate_Pin_व्यञ्जकम् ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_वृक्षः(false)};
                          $<implicitp>$->सङ्कलय_Exprsp($4->clone_वृक्षः(false));
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, $4};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateBufif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_व्यञ्जकम् ',' gate_Pin_व्यञ्जकम् ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_वृक्षः(false)};
                          $<implicitp>$->सङ्कलय_Exprsp($4->clone_वृक्षः(false));
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstBufIf1{$<fl>1, $6, $4};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif0<nodep>:
                gateFront variable_lvalue ',' gate_Pin_व्यञ्जकम् ',' gate_Pin_व्यञ्जकम् ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_वृक्षः(false)};
                          $<implicitp>$->सङ्कलय_Exprsp($4->clone_वृक्षः(false));
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstBufIf1{$<fl>1, new AstNot{$<fl>1, $6}, new AstNot{$<fl>1, $4}};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateNotif1<nodep>:
                gateFront variable_lvalue ',' gate_Pin_व्यञ्जकम् ',' gate_Pin_व्यञ्जकम् ')'
                        { $$ = new AstImplicit{$<fl>1, $6->clone_वृक्षः(false)};
                          $<implicitp>$->सङ्कलय_Exprsp($4->clone_वृक्षः(false));
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstBufIf1{$<fl>1, $6, new AstNot{$<fl>1, $4}};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateAnd<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = $4;
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateNand<nodep>:
                gateFront variable_lvalue ',' gateAndPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstNot{$<fl>1, $4};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateOr<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = $4;
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateNor<nodep>:
                gateFront variable_lvalue ',' gateOrPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstNot{$<fl>1, $4};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateXor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = $4;
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
                          DEL($1); }
        ;
gateXnor<nodep>:
                gateFront variable_lvalue ',' gateXorPinList ')'
                        { $$ = new AstImplicit{$<fl>1, $4->clone_वृक्षः(false)};
                          Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = new AstNot{$<fl>1, $4};
                          Ast_नियोजय_W* ध्रुव ap = new Ast_नियोजय_W{$<fl>1, $2, rhsp};
                          $$->सङ्कलय_अग्र(new AstAlways{ap});
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

gateAndPinList<ग्रन्थिः_Exprp>:
                gate_Pin_व्यञ्जकम्                             { $$ = $1; }
        |       gateAndPinList ',' gate_Pin_व्यञ्जकम्          { $$ = new AstAnd{$2, $1, $3}; }
        ;
gateOrPinList<ग्रन्थिः_Exprp>:
                gate_Pin_व्यञ्जकम्                             { $$ = $1; }
        |       gateOrPinList ',' gate_Pin_व्यञ्जकम्           { $$ = new AstOr{$2, $1, $3}; }
        ;
gateXorPinList<ग्रन्थिः_Exprp>:
                gate_Pin_व्यञ्जकम्                             { $$ = $1; }
        |       gateXorPinList ',' gate_Pin_व्यञ्जकम्          { $$ = new AstXor{$2, $1, $3}; }
        ;
gateUnsupPinList<ग्रन्थिः_Exprp>:
                gate_Pin_व्यञ्जकम्                             { $$ = $1; }
        |       gateUnsupPinList ',' gate_Pin_व्यञ्जकम्        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

gate_Pin_व्यञ्जकम्<ग्रन्थिः_Exprp>:
                व्यञ्जकम्                                    { $$ = GRAMMARP->createGatePin($1); }
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
                /*रिक्तानि*/                             { $$ = nullptr; }
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
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       pulldown_strength                       { $$ = $1; }
        ;

pulldown_strength<nodep>:  // IEEE: pulldown_strength
                yP_PAR__STRENGTH strength0 ',' strength1 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength1 ',' strength0 ')'  { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        |       yP_PAR__STRENGTH strength0 ')'                { $$ = nullptr; BBUNSUP($<fl>2, "Unsupported: pulldown strength"); }
        ;

pullup_strengthE<nodep>:  // IEEE: [ pullup_strength ]
                /*रिक्तानि*/                             { $$ = nullptr; }
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
                y_सारणी सारणी_प्रविष्टिः_List yENDTABLE         { $$ = new Ast_Udp_सारणी{$1, $2}; }
        ;

सारणी_प्रविष्टिः_List<udp_सारणी_Linep>:  // IEEE: { combinational_entry + sequential_entry }
                सारणी_पङ्क्तिः                              { $$ = $1; }
        |       सारणी_प्रविष्टिः_List सारणी_पङ्क्तिः               { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

सारणी_पङ्क्तिः<udp_सारणी_Linep>:
                सारणी_निवेशः_List ya_सारणी_LRSEP tablelVal ya_सारणी_LINEEND
                        { $$ = new Ast_Udp_सारणी_पङ्क्तिः{Ast_Udp_सारणी_पङ्क्तिः::UdpCombo{}, $<fl>1, $1, $3}; }
        |       सारणी_निवेशः_List ya_सारणी_LRSEP tablelVal ya_सारणी_LRSEP tablelVal ya_सारणी_LINEEND
                        { $$ = new Ast_Udp_सारणी_पङ्क्तिः{Ast_Udp_सारणी_पङ्क्तिः::UdpSequential{}, $<fl>1, $1, $3, $5}; }
        ;

सारणी_निवेशः_List<udp_सारणी_पङ्क्तिः_Valp>:
                tablelVal                            { $$ = $1; }
        |       सारणी_निवेशः_List tablelVal             { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

tablelVal<udp_सारणी_पङ्क्तिः_Valp>:
                ya_सारणी_क्षेत्रम्                          { $$ = new Ast_Udp_सारणी_पङ्क्तिः_Val{$<fl>1, *$1}; }
        |       '(' ya_सारणी_क्षेत्रम् ya_सारणी_क्षेत्रम् ')'    { $$ = new Ast_Udp_सारणी_पङ्क्तिः_Val{$<fl>2, *$2 + *$3}; }
        ;

//************************************************
// Specify

specify_खण्डम्<nodep>:               // ==IEEE: specify_block
                specifyFront specify_itemList yENDSPECIFY   { $$ = $2; }
        |       specifyFront yENDSPECIFY                { $$ = nullptr; }
        ;

specifyFront:  // IEEE: specify_block front
                ySPECIFY                                { GRAMMARP->m_specifyignWarned = false; }
        ;

specify_itemList<nodep>:            // IEEE: { specify_item }
                specify_item                            { $$ = $1; }
        |       specify_itemList specify_item           { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

specify_item<nodep>:                // ==IEEE: specify_item
                specparam_declaration                   { $$ = $1; }
        |       system_timing_check                     { $$ = $1; }
        |       junk_गन्तव्यम्_Semi_List ';'
                        { $$ = nullptr;
                          यदि (!GRAMMARP->m_specifyignWarned) {
                              GRAMMARP->m_specifyignWarned = true;
                              $1->v3warn(SPECIFYIGN, "Ignoring unsupported: specify block construct");
                          }
                        }
        ;

specparam_declaration<nodep>:       // ==IEEE: specparam_declaration
                specparam_declarationFront list_सम्बन्धि_specparam_assignments ';'
                        { $$ = $2; }
        ;

specparam_declarationFront:     // IEEE: part of specparam_declaration
        //                      // Front must execute first so VARDTYPE is ready before list of vars
                ySPECPARAM
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_ग्रन्थिः_DType* dtp = new AstBasicDType{$1, VBasicDTypeKwd::DOUBLE};
                          VARDTYPE(dtp); }
        |       ySPECPARAM packed_dimension
                        { VARRESET_NONLIST(SPECPARAM);
                          Ast_ग्रन्थिः_DType* ध्रुव dtp = GRAMMARP->सङ्कलय_Range(
                                    new AstBasicDType{$2->fileline(), LOGIC_IMPLICIT}, $2, true);
                          VARDTYPE(dtp); }
        ;

list_सम्बन्धि_specparam_assignments<varp>:  // ==IEEE: list_of_specparam_assignments
                specparam_assignment                    { $$ = $1; }
        |       list_सम्बन्धि_specparam_assignments ',' specparam_assignment  { $$ = $1->सङ्कलय_अग्र($3); }
        ;

specparam_assignment<varp>:     // ==IEEE: specparam_assignment
                idNotPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          यदि ($4) $$->valuep($4); }
        //                      // IEEE: pulse_control_specparam
        //                      // LRM grammar requires '(' as the first token after assignment,
        //                      // but IEEE provides an example in 30.7.1 where it is omitted.
        //                      // Other simulators also support it.
        |       idPathpulse sigAttrListE '=' minTypMax
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          यदि ($4) $$->valuep($4); }
        |       idPathpulse sigAttrListE '=' '(' minTypMax ',' minTypMax ')'
                        { $$ = VARDONEA($<fl>1, *$1, nullptr, $2);
                          यदि ($5) $$->valuep($5);
                          DEL($7); }
        ;

system_timing_check<nodep>:         // ==IEEE: system_timing_check
                setuphold_timing_check                      { $$ = $1; }
        ;

setuphold_timing_check<nodep>:      // ==IEEE: $setuphold_timing_check
                yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_सीमा ',' timing_check_सीमा ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_सीमा ',' timing_check_सीमा ',' idAnyE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_सीमा ',' timing_check_सीमा ',' idAnyE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_सीमा ',' timing_check_सीमा ',' idAnyE ',' minTypMaxE ',' minTypMaxE ')' ';'
                        { $$ = nullptr; DEL($3, $5, $7, $9, $13, $15); }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_सीमा ',' timing_check_सीमा ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17}; DEL($7, $9, $13, $15);  }
        |       yD_SETUPHOLD '(' timing_check_event ',' timing_check_event ',' timing_check_सीमा ',' timing_check_सीमा ',' idAnyE ',' minTypMaxE ',' minTypMaxE ',' delayed_referenceE ',' delayed_referenceE ')' ';'
                        { $$ = new AstSetuphold{$1, $3, $5, $17, $19}; DEL($7, $9, $13, $15); }
        ;

timing_check_event<ग्रन्थिः_Exprp>:      // ==IEEE: $timing_check_event
                terminal_identifier                                                         { $$ = $1; }
        |       yPOSEDGE terminal_identifier                                                { $$ = $2; }
        |       yNEGEDGE terminal_identifier                                                { $$ = $2; }
        |       yEDGE terminal_identifier                                                   { $$ = $2; }
        |       yEDGE '[' edge_descriptor_list ']' terminal_identifier                      { $$ = $5; }
        |       terminal_identifier yP_ANDANDAND व्यञ्जकम्                                       { $$ = $1; DEL($3); }
        |       yPOSEDGE terminal_identifier yP_ANDANDAND व्यञ्जकम्                              { $$ = $2; DEL($4); }
        |       yNEGEDGE terminal_identifier yP_ANDANDAND व्यञ्जकम्                              { $$ = $2; DEL($4); }
        |       yEDGE terminal_identifier yP_ANDANDAND व्यञ्जकम्                                 { $$ = $2; DEL($4); }
        |       yEDGE '[' edge_descriptor_list ']' terminal_identifier yP_ANDANDAND व्यञ्जकम्    { $$ = $5; DEL($7); }
        ;

edge_descriptor_list:
                yaEDGEDESC                            {  }
        |       edge_descriptor_list ',' yaEDGEDESC   {  }
        ;

timing_check_सीमा<ग्रन्थिः_Exprp>:
                व्यञ्जकम्                      { $$ = $1; }
        |       व्यञ्जकम् ':' व्यञ्जकम् ':' व्यञ्जकम्    { $$ = $3; DEL($1, $5); }
        ;

delayed_referenceE<ग्रन्थिः_Exprp>:
                /*empty*/                               { $$ = nullptr; }
        |       terminal_identifier                     { $$ = $1; }
        ;

terminal_identifier<ग्रन्थिः_Exprp>:
                idArrayed     { $$ = $1; }
        ;

idAnyE<strp>:
                /*empty*/                               { $$ = nullptr; }
        |       idAny                                   { $$ = $1; }
        ;

junk_गन्तव्यम्_Semi_List<fl>:
                junk_गन्तव्यम्_Semi                              { $$ = CRELINE(); }
        |       junk_गन्तव्यम्_Semi_List junk_गन्तव्यम्_Semi               { $$ = CRELINE(); }
        ;

junk_गन्तव्यम्_Semi:
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
                yRANDOMIZE                              { स्थिर string s = "randomize"; $$ = &s; $<fl>$ = $<fl>1; }
        ;

idSVKwd<strp>:                  // Warn about non-forward compatible Verilog 2001 code
        //                      // yBIT, yBYTE won't work here as causes conflicts
                yDO
                        { स्थिर string s = "do"   ; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        |       yFINAL
                        { स्थिर string s = "final"; $$ = &s; ERRSVKWD($1, *$$); $<fl>$ = $<fl>1; }
        ;

variable_lvalue<ग्रन्थिः_Exprp>:     // IEEE: variable_lvalue or net_lvalue
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

variable_lvalueConcList<ग्रन्थिः_Exprp>:  // IEEE: part of variable_lvalue: '{' variable_lvalue { ',' variable_lvalue } '}'
                variable_lvalue                                 { $$ = $1; }
        |       variable_lvalueConcList ',' variable_lvalue     { $$ = new AstConcat{$2, $1, $3}; }
        ;

//UNSUPvariable_lvalueList<nodeExprp>:  // IEEE: part of variable_lvalue: variable_lvalue { ',' variable_lvalue }
//UNSUP         variable_lvalue                         { $$ = $1; }
//UNSUP |       variable_lvalueList ',' variable_lvalue { $$ = addNextNull($1, $3); }
//UNSUP ;

idClass<ग्रन्थिः_Exprp>:             // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
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

idClassSel<ग्रन्थिः_Exprp>:          // Misc Ref to dotted, and/or arrayed, and/or bit-ranged variable
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
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "this"}, $3->fromp()->unlink_Fr_पृष्ठभागः()}); $$ = $3; }
        |       ySUPER '.' idDottedForeach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "super"}, $3->fromp()->unlink_Fr_पृष्ठभागः()}); $$ = $3; }
        |       yTHIS '.' ySUPER '.' idDottedForeach
                        { $5->fromp(new AstDot{$4, false, new AstParseRef{$<fl>3, "super"}, $5->fromp()->unlink_Fr_पृष्ठभागः()}); $$ = $5; }
        //                      // Expanded: package_scope idForeach
        |       packageClassScope idDottedForeach
                       { $2->fromp(new AstDot{$<fl>2, true, $1, $2->fromp()->unlink_Fr_पृष्ठभागः()}); $$ = $2; }
        ;


// Dotted identifier for typedef - must have at least one '.'
// First component is plain id or id with array index, subsequent components can have arrays
idDottedOrArrayed<ग्रन्थिः_Exprp>:
                id '.' idArrayed
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, *$1, nullptr, nullptr}, $3}; }
        |       idDottedOrArrayed '.' idArrayed
                        { $$ = new AstDot{$2, false, $1, $3}; }
        ;

idDotted<ग्रन्थिः_Exprp>:
                y_D_मूलम् '.' id_Dotted_अधिकम्
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_अधिकम्                            { $$ = $1; }
        ;

idDottedSel<ग्रन्थिः_Exprp>:
                y_D_मूलम् '.' id_Dotted_Sel_अधिकम्
                        { $$ = new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3}; }
        |       id_Dotted_Sel_अधिकम्                         { $$ = $1; }
        ;

idDottedForeach<foreachHeaderp>:
                y_D_मूलम् '.' id_Dotted_अधिकम्_Foreach
                        { $3->fromp(new AstDot{$2, false, new AstParseRef{$<fl>1, "$root"}, $3->fromp()->unlink_Fr_पृष्ठभागः()}); $$ = $3; }
        |       id_Dotted_अधिकम्_Foreach                     { $$ = $1; }
        ;

id_Dotted_अधिकम्<ग्रन्थिः_Exprp>:
                parse_Ref_आधारः                            { $$ = $1; }
        |       id_Dotted_अधिकम् '.' parse_Ref_आधारः           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_Sel_अधिकम्<ग्रन्थिः_Exprp>:
                idArrayed                               { $$ = $1; }
        |       id_Dotted_Sel_अधिकम् '.' idArrayed           { $$ = new AstDot{$2, false, $1, $3}; }
        ;

id_Dotted_अधिकम्_Foreach<foreachHeaderp>:
                idArrayedForeach                        { $$ = $1; }
        |       id_Dotted_Sel_अधिकम् '.' idArrayedForeach
                        { $3->fromp(new AstDot{$2, false, $1, $3->fromp()->unlink_Fr_पृष्ठभागः()}); $$ = $3; }
        ;

// Single component of dotted path, maybe [#].
// Due to lookahead constraints, we can't know if [:] or [+:] are valid (last dotted part),
// we'll assume so and cleanup later.
// id below includes:
//       enum_identifier
idArrayed<ग्रन्थिः_Exprp>:               // IEEE: id + select
                id
                        { $$ = new AstParseRef{$<fl>1, *$1, nullptr, nullptr}; }
        //                      // IEEE: id + part_select_range/constant_part_select_range
        |       idArrayed '[' व्यञ्जकम् ']'                          { $$ = new AstSelBit{$2, $1, $3}; }  // Or AstArraySel, don't know yet.
        |       idArrayed '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'       { $$ = new AstSelExtract{$2, $1, $3, $5}; }
        //                      // IEEE: id + indexed_range/constant_indexed_range
        |       idArrayed '[' व्यञ्जकम् yP_PLUSCOLON  const_व्यञ्जकम् ']'  { $$ = new AstSelPlus{$2, $1, $3, $5}; }
        |       idArrayed '[' व्यञ्जकम् yP_MINUSCOLON const_व्यञ्जकम् ']'  { $$ = new AstSelMinus{$2, $1, $3, $5}; }
        ;

idArrayedForeach<foreachHeaderp>:    // IEEE: id + select (under foreach expression)
                parse_Ref_आधारः  // Malformed, but accept for better error reporting
                        { $$ = new Ast_Foreach_शीर्षकम्{$<fl>1, $1, nullptr}; }
        |       idArrayed '[' व्यञ्जकम् ']'
                        { $$ = new Ast_Foreach_शीर्षकम्{$2, $1, $3}; }
        |       idArrayed '[' ']'
                        { $$ = new Ast_Foreach_शीर्षकम्{$2, $1, new Ast_रिक्तानि{$3}}; }
        |       idArrayed '[' parse_Ref_आधारः ',' loop_variables ']'
                        { $$ = new Ast_Foreach_शीर्षकम्{$2, $1, सङ्कलय_अग्र_Null(static_cast<Ast_ग्रन्थिः*>($3), $5)}; }
        |       idArrayed '[' ',' loop_variables ']'
                        { $$ = new Ast_Foreach_शीर्षकम्{$2, $1, सङ्कलय_अग्र_Null(static_cast<Ast_ग्रन्थिः*>(new Ast_रिक्तानि{$3}), $4)}; }
        ;

// ParseRef without any dots or vectorizaion
parse_Ref_आधारः<parseRefp>:
                id                                      { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

// yaSTRING shouldn't be used directly, instead via an abstraction below
str<strp>:                      // yaSTRING but with \{escapes} need decoded
                yaSTRING                                { $$ = PARSEP->newString(GRAMMARP->unquoteString($<fl>1, *$1)); }
        ;

strAsInt<ग्रन्थिः_Exprp>:
                yaSTRING
                        { // Numeric context, so IEEE 1800-2017 11.10.3 "" is a "\000"
                          $$ = new AstConst{$<fl>1, AstConst::VerilogStringLiteral{}, GRAMMARP->unquoteString($<fl>1, *$1)}; }
        ;

strAsIntIgnore<ग्रन्थिः_Exprp>:          // strAsInt, but never matches for when expr shouldn't parse strings
                yaSTRING__IGNORE                        { $$ = nullptr; yyerror("Impossible token"); }
        ;

startLabelE<strp>:
                /*रिक्तानि*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        ;

अन्तः_Label_E<strp>:
                /*रिक्तानि*/                             { $$ = nullptr; $<fl>$ = nullptr; }
        |       ':' idAny                               { $$ = $2; $<fl>$ = $<fl>2; }
        |       ':' yNEW__ETC                           { स्थिर string n = "new"; $$ = &n; $<fl>$ = $<fl>2; }
        ;

//************************************************
// Clocking

clocking_declaration<nodep>:            // IEEE: clocking_declaration
                yCLOCKING idAny clocking_event ';' clocking_itemListE yENDCLOCKING अन्तः_Label_E
                        { $$ = new AstClocking{$<fl>2, *$2, new Ast_Sen_वृक्षः{$<fl>2, $3}, $5, false, false}; }
        |       yDEFAULT yCLOCKING clocking_event ';' clocking_itemListE yENDCLOCKING अन्तः_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_वृक्षः{$<fl>2, $3}, $5, true, false}; }
        |       yDEFAULT yCLOCKING idAny clocking_event ';' clocking_itemListE yENDCLOCKING अन्तः_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_वृक्षः{$<fl>3, $4}, $6, true, false}; }
        |       yGLOBAL__CLOCKING yCLOCKING clocking_event ';' clocking_itemListE yENDCLOCKING अन्तः_Label_E
                        { $$ = new AstClocking{$<fl>2, "", new Ast_Sen_वृक्षः{$<fl>2, $3}, $5, false, true}; }
        |       yGLOBAL__CLOCKING yCLOCKING idAny clocking_event ';' clocking_itemListE yENDCLOCKING अन्तः_Label_E
                        { $$ = new AstClocking{$<fl>3, *$3, new Ast_Sen_वृक्षः{$<fl>3, $4}, $6, false, true}; }
        ;

clocking_eventE<senItemp>:      // IEEE: optional clocking_event
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       clocking_event                          { $$ = $1; }
        ;

clocking_event<senItemp>:       // IEEE: clocking_event
        //                      // IEEE: '@' ps_identifier
        //                      // IEEE: '@' hierarchical_identifier
        //UNSUP: '@' idClassSel/*ps_identifier*/
                '@' id
                        { $$ = new AstSenItem{$<fl>2, VEdgeType::ET_परिवर्तितम्,
                                              new AstParseRef{$<fl>2, *$2, nullptr, nullptr}}; }
        |       '@' '(' event_expression ')'            { $$ = $3; }
        ;

clocking_itemListE<nodep>:
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       clocking_itemList                       { $$ = $1; }
        ;

clocking_itemList<nodep>:  // IEEE: [ clocking_item ]
                clocking_item                           { $$ = $1; }
        |       clocking_itemList clocking_item         { यदि ($1) $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

clocking_item<nodep>:   // IEEE: clocking_item
                yDEFAULT y_निवेशः clocking_skew ';'       { $$ = new AstClockingItem{$<fl>1, VDirection::निवेशः, $3, nullptr}; }
        |       yDEFAULT yOUTPUT clocking_skew ';'      { $$ = new AstClockingItem{$<fl>1, VDirection::OUTPUT, $3, nullptr}; }
        |       yDEFAULT y_निवेशः clocking_skew yOUTPUT clocking_skew ';'
                        { $$ = new AstClockingItem{$<fl>1, VDirection::निवेशः, $3, nullptr};
                          $$->सङ्कलय_अग्र(new AstClockingItem{$<fl>4, VDirection::OUTPUT, $5, nullptr}); }
        |       y_निवेशः clocking_skewE list_सम्बन्धि_clocking_decl_नियोजय ';'
                        { $$ = GRAMMARP->makeClockingItemList($<fl>1, VDirection::निवेशः, $2, $3); }
        |       yOUTPUT clocking_skewE list_सम्बन्धि_clocking_decl_नियोजय ';'
                        { $$ = GRAMMARP->makeClockingItemList($<fl>1, VDirection::OUTPUT, $2, $3); }
        |       y_निवेशः clocking_skewE yOUTPUT clocking_skewE list_सम्बन्धि_clocking_decl_नियोजय ';'
                        { $$ = GRAMMARP->makeClockingItemList($<fl>1, VDirection::निवेशः, $2, $5->clone_वृक्षः(true));
                          $$->सङ्कलय_अग्र(GRAMMARP->makeClockingItemList($<fl>3, VDirection::OUTPUT, $4, $5)); }
        |       yINOUT list_सम्बन्धि_clocking_decl_नियोजय ';'
                        { $$ = GRAMMARP->makeClockingItemList($<fl>1, VDirection::निवेशः, nullptr, $2->clone_वृक्षः(true));
                          $$->सङ्कलय_अग्र(GRAMMARP->makeClockingItemList($<fl>1, VDirection::OUTPUT, nullptr, $2)); }
        |       assertion_item_declaration
                        { $$ = $1;
                          प्रति (Ast_ग्रन्थिः* nodep = $1; nodep; nodep = nodep->nextp()) {
                              यदि (!VN_IS(nodep, Sequence)) {
                                  $$ = nullptr;
                                  BBUNSUP(nodep, "Unsupported: assertion items in clocking blocks");
                                  DEL($1);
                                  भङ्ग;
                              }
                          }
                        }
        ;

list_सम्बन्धि_clocking_decl_नियोजय<nodep>:  // IEEE: list_of_clocking_decl_assign
                clocking_decl_नियोजय                    { $$ = $1; }
        |       list_सम्बन्धि_clocking_decl_नियोजय ',' clocking_decl_नियोजय
                        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

clocking_decl_नियोजय<nodep>:    // IEEE: clocking_decl_assign
                idAnyAsParseRef/*new-signal_identifier*/ व्यञ्जकम्_Eq_E
                        { AstParseRef* ध्रुव refp = $1;
                          $$ = refp;
                          यदि ($2) $$ = new Ast_नियोजय{$<fl>2, refp, $2}; }
        ;

clocking_skewE<ग्रन्थिः_Exprp>:          // IEEE: [clocking_skew]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       clocking_skew                           { $$ = $1; }
        ;

clocking_skew<ग्रन्थिः_Exprp>:           // IEEE: clocking_skew
                delay_control                           { $$ = $1->lhsp()->unlink_Fr_पृष्ठभागः(); $1->delete_वृक्षः(); }
        |      yPOSEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      yNEGEDGE delay_controlE                  { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        |      yEDGE delay_controlE                     { $$ = nullptr;
                                                          BBUNSUP($1, "Unsupported: clocking event edge override"); DEL($2); }
        ;

आवृत्तिः_delay<delayp>:  // IEEE: cycle_delay
               yP_POUNDPOUND yaINTNUM
                        { $$ = new AstDelay{$<fl>1, new AstConst{$<fl>2, *$2}, true}; }
        |      yP_POUNDPOUND idAnyAsParseRef
                        { $$ = new AstDelay{$<fl>1, $2, true}; }
        |      yP_POUNDPOUND '(' व्यञ्जकम् ')'
                        { $$ = new AstDelay{$<fl>1, $3, true}; }
        ;

//************************************************
// Asserts

assertion_item_declaration<nodep>:  // ==IEEE: assertion_item_declaration
                property_declaration                    { $$ = $1; }
        |       sequence_declaration                    { $$ = $1; }
        |       let_declaration                         { $$ = $1; }
        ;

assertion_item<nodep>:          // ==IEEE: assertion_item
                concurrent_assertion_item               { $$ = $1; }
        |       deferred_तात्कालिकम्_assertion_item
                        { $$ = $1 ? new AstAlways{$1->fileline(), VAlwaysKwd::ALWAYS_COMB, nullptr, $1} : nullptr; }
        ;

deferred_तात्कालिकम्_assertion_item<nodep>:       // ==IEEE: deferred_immediate_assertion_item
                deferred_तात्कालिकम्_assertion_statement  { $$ = $1; }
        |       id/*block_identifier*/ ':' deferred_तात्कालिकम्_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        ;

procedural_assertion_statement<ग्रन्थिः_Stmtp>:  // ==IEEE: procedural_assertion_statement
                concurrent_assertion_statement          { $$ = $1; }
        |       तात्कालिकम्_assertion_statement           { $$ = $1; }
        //                      // IEEE: checker_instantiation
        //                      // Unlike modules, checkers are the only "id id (...)" form in statements.
        //UNSUP checker_instantiation                   { $$ = $1; }
        ;

तात्कालिकम्_assertion_statement<ग्रन्थिः_Stmtp>:   // ==IEEE: immediate_assertion_statement
                simple_तात्कालिकम्_assertion_statement    { $$ = $1; }
        |       deferred_तात्कालिकम्_assertion_statement  { $$ = $1; }
        ;

simple_तात्कालिकम्_assertion_statement<ग्रन्थिः_Stmtp>:    // ==IEEE: simple_immediate_assertion_statement
        //                      // action_block expanded here, for compatibility with AstAssert
                assertOrAssume '(' व्यञ्जकम् ')' stmt %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $3, $5, nullptr, VAssertType::SIMPLE_तात्कालिकम्, $1}; }
        |       assertOrAssume '(' व्यञ्जकम् ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, nullptr, $6, VAssertType::SIMPLE_तात्कालिकम्, $1}; }
        |       assertOrAssume '(' व्यञ्जकम् ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $3, $5, $7, VAssertType::SIMPLE_तात्कालिकम्, $1}; }
        //                      // IEEE: simple_immediate_cover_statement
        |       yCOVER '(' व्यञ्जकम् ')' stmt                { $$ = new AstCover{$1, $3, $5, VAssertType::SIMPLE_तात्कालिकम्}; }
        ;

assertOrAssume<assertdirectivetypeen>:
                yASSERT                                 { $$ = VAssertDirectiveType::ASSERT; }
        |       yASSUME                                 { $$ = VAssertDirectiveType::ASSUME; }
        ;

final_zero<asserttypeen>:                     // IEEE: part of deferred_immediate_assertion_statement
                '#' yaINTNUM
                        { यदि ($2->isNeqZero()) { $<fl>2->v3error("Deferred assertions must use '#0' (IEEE 1800-2023 16.4)"); }
                          $$ = VAssertType::OBSERVED_DEFERRED_तात्कालिकम्; }
        //                      // 1800-2012:
        |       yFINAL                                                  { $$ = VAssertType::FINAL_DEFERRED_तात्कालिकम्; }
        ;

deferred_तात्कालिकम्_assertion_statement<ग्रन्थिः_Stmtp>:  // ==IEEE: deferred_immediate_assertion_statement
        //                      // IEEE: deferred_immediate_assert_statement
                assertOrAssume final_zero '(' व्यञ्जकम् ')' stmt %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = new AstAssert{$<fl>1, $4, $6, nullptr, $2, $1}; }
        |       assertOrAssume final_zero '(' व्यञ्जकम् ')'           yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, nullptr, $7, $2, $1}; }
        |       assertOrAssume final_zero '(' व्यञ्जकम् ')' stmt yELSE stmt
                        { $$ = new AstAssert{$<fl>1, $4, $6, $8, $2, $1}; }
        //                      // IEEE: deferred_immediate_cover_statement
        |       yCOVER final_zero '(' व्यञ्जकम् ')' stmt     { $$ = new AstCover{$1, $4, $6, $2}; }
        ;

concurrent_assertion_item<nodep>:       // IEEE: concurrent_assertion_item
                concurrent_assertion_statement          { $$ = $1; }
        |       id/*block_identifier*/ ':' concurrent_assertion_statement
                        { $$ = new AstBegin{$<fl>1, *$1, $3, true}; }
        //                      // IEEE: checker_instantiation
        //                      // identical to module_instantiation; see etcInst
        ;

concurrent_assertion_statement<ग्रन्थिः_Stmtp>:  // ==IEEE: concurrent_assertion_statement
        //                      // IEEE: assert_property_statement
        //                      // IEEE: assume_property_statement
        //                      // action_block expanded here
                assertOrAssume yPROPERTY '(' property_spec ')' stmt %prec pr_अधस्तनम्_THAN_ELSE
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
                        { AstCover* ध्रुव coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), nullptr, nullptr, $4},
                              $6, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event sexpr ')' stmt
                        { AstCover* ध्रुव coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, nullptr, $5},
                              $7, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' clocking_event yDISABLE yIFF '(' व्यञ्जकम्/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* ध्रुव coverp = new AstCover{$1,
                              new AstPropSpec{$4->fileline(), $4, $8, $10},
                              $12, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        |       yCOVER ySEQUENCE '(' yDISABLE yIFF '(' व्यञ्जकम्/*expression_or_dist*/ ')' sexpr ')' stmt
                        { AstCover* ध्रुव coverp = new AstCover{$1,
                              new AstPropSpec{$7->fileline(), nullptr, $7, $9},
                              $11, VAssertType::CONCURRENT};
                          coverp->isCoverSeq(true);
                          $$ = coverp; }
        //                      // IEEE: restrict_property_statement
        |       yRESTRICT yPROPERTY '(' property_spec ')' ';'
                        { $$ = new AstRestrict{$1, $4}; }
        ;

property_declaration<ग्रन्थिः_FTaskp>:  // ==IEEE: property_declaration
                property_declarationFront property_पत्तनम्_list_E ';' property_declarationBody
                        yENDPROPERTY अन्तः_Label_E
                        { $$ = $1;
                          $$->सङ्कलय_Stmtsp($2);
                          $$->सङ्कलय_Stmtsp($4);
                          GRAMMARP->अन्तः_Label($<fl>6, $$, $6);
                          GRAMMARP->m_insideProperty = false;
                          GRAMMARP->m_typed_Property_पत्तनम् = false; }
        ;

property_declarationFront<ग्रन्थिः_FTaskp>:  // IEEE: part of property_declaration
                yPROPERTY idAny/*property_identifier*/
                        { $$ = new AstProperty{$<fl>2, *$2, nullptr};
                          GRAMMARP->m_insideProperty = true; }
        ;

property_पत्तनम्_list_E<nodep>:  // IEEE: [ ( [ property_port_list ] ) ]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '(' property_पत्तनम्_list ')'              { $$ = $2; }
        ;

property_पत्तनम्_list<nodep>:  // ==IEEE: property_port_list
                property_पत्तनम्_item                              { $$ = $1; }
        |       property_पत्तनम्_list ',' property_पत्तनम्_item       { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

property_पत्तनम्_item<nodep>:  // IEEE: property_port_item/sequence_port_item
        //                      // Merged in sequence_port_item
        //                      // IEEE: property_lvar_port_direction ::= yINPUT
        //                      // prop IEEE: [ yLOCAL [ yINPUT ] ] property_formal_type
        //                      //           id {variable_dimension} [ '=' property_actual_arg ]
        //                      // seq IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
                property_पत्तनम्_item_Front property_पत्तनम्_item_Assignment  { $$ = $2; }
        ;

property_पत्तनम्_item_Front:  // IEEE: part of property_port_item/sequence_port_item
                property_पत्तनम्_item_Dir_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       property_पत्तनम्_item_Dir_E दत्त_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_पत्तनम् = true; }
        |       property_पत्तनम्_item_Dir_E yVAR दत्त_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_पत्तनम् = true; }
        |       property_पत्तनम्_item_Dir_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       property_पत्तनम्_item_Dir_E implicit_typeE
                        { VARDTYPE($2); }
        ;

property_पत्तनम्_item_Assignment<nodep>:  // IEEE: part of property_port_item/sequence_port_item
                id variable_dimensionListE
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, nullptr); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { VARDECL(VAR);
                          $$ = VARDONEA($<fl>1, *$1, $2, $4);
                          BBUNSUP($3, "Unsupported: property variable default value"); }
        ;

property_पत्तनम्_item_Dir_E:
                /*रिक्तानि*/
                        { VARIO(निवेशः); }
        |       yLOCAL__ETC
                        { VARIO(निवेशः);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        |       yLOCAL__ETC y_निवेशः
                        { VARIO(निवेशः);
                          BBUNSUP($1, "Unsupported: property port 'local'"); }
        ;

property_declarationBody<nodep>:  // IEEE: part of property_declaration
                assertion_variable_declarationList property_spec
                        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        |       assertion_variable_declarationList property_spec ';'
                        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        //                      // IEEE-2012: Incorrectly has yCOVER ySEQUENCE then property_spec here.
        //                      // Fixed in IEEE 1800-2017
        |       property_spec                           { $$ = $1; }
        |       property_spec ';'                       { $$ = $1; }
        ;

assertion_variable_declarationList<nodep>:  // IEEE: part of assertion_variable_declaration
                assertion_variable_declaration          { $$ = $1; }
        |       assertion_variable_declarationList assertion_variable_declaration
                        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

sequence_declaration<ग्रन्थिः_FTaskp>:  // ==IEEE: sequence_declaration
                sequence_declarationFront sequence_पत्तनम्_list_E ';' sequence_declarationBody
        /*cont*/    yENDSEQUENCE अन्तः_Label_E
                        { $$ = $1;
                          $$->सङ्कलय_Stmtsp($2);
                          $$->सङ्कलय_Stmtsp($4);
                          GRAMMARP->अन्तः_Label($<fl>6, $$, $6);
                          // Sequence declarations are allowed; they are inlined by V3AssertPre.
                          // Sequences in unsupported contexts are reported by assertPreAll
                          // after inlining.
                        }
        ;

sequence_declarationFront<ग्रन्थिः_FTaskp>:  // IEEE: part of sequence_declaration
                ySEQUENCE idAny/*new_sequence*/
                        { $$ = new AstSequence{$<fl>2, *$2, nullptr}; }
        ;

sequence_पत्तनम्_list_E<nodep>:  // IEEE: [ ( [ sequence_port_list ] ) ]
        //                      // IEEE: sequence_lvar_port_direction ::= yINPUT | yINOUT | yOUTPUT
        //                      // IEEE: [ yLOCAL [ sequence_lvar_port_direction ] ] sequence_formal_type
        //                      //           id {variable_dimension} [ '=' sequence_actual_arg ]
        //                      // All this is almost identically the same as a property.
        //                      // Difference is only yINOUT/yOUTPUT (which might be added to 1800-2012)
        //                      // and yPROPERTY.  So save some work.
                property_पत्तनम्_list_E                     { $$ = $1; }
        ;

property_formal_typeNoDt<ग्रन्थिः_DTypep>:  // IEEE: property_formal_type (w/o implicit)
                sequence_formal_typeNoDt                { $$ = $1; }
        |       yPROPERTY
                        { $$ = nullptr; GRAMMARP->m_typed_Property_पत्तनम् = false;
                          BBUNSUP($1, "Unsupported: property argument data type"); }
        ;

sequence_formal_typeNoDt<ग्रन्थिः_DTypep>:  // ==IEEE: sequence_formal_type (w/o data_type_or_implicit)
                        // IEEE: data_type_or_implicit
                        // implicit expanded where used
                ySEQUENCE
                        { $$ = nullptr; GRAMMARP->m_typed_Property_पत्तनम् = false;
                          BBUNSUP($1, "Unsupported: sequence argument data type"); }
                        // IEEE-2009: yEVENT
                        // already part of data_type.  Removed in 1800-2012.
        |       yUNTYPED
                        { $$ = nullptr; GRAMMARP->m_typed_Property_पत्तनम् = false; }
        ;

sequence_declarationBody<nodep>:  // IEEE: part of sequence_declaration
        //                      // 1800-2012 makes ';' optional
                assertion_variable_declarationList sexpr        { $$ = सङ्कलय_अग्र_Null($1, $2); }
        |       assertion_variable_declarationList sexpr ';'    { $$ = सङ्कलय_अग्र_Null($1, $2); }
        |       sexpr                                   { $$ = $1; }
        |       sexpr ';'                               { $$ = $1; }
        //                      // IEEE: clocking_event sequence_expr (16.7)
        //                      // A leading clocking event on a named sequence body.
        |       '@' '(' event_expression ')' sexpr      { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' '(' event_expression ')' sexpr ';'  { $$ = new AstSClocked{$1, $3, $5}; }
        |       '@' senitemVar sexpr                    { $$ = new AstSClocked{$1, $2, $3}; }
        |       '@' senitemVar sexpr ';'                { $$ = new AstSClocked{$1, $2, $3}; }
        ;

property_spec<propSpecp>:               // IEEE: property_spec
        //UNSUP: This rule has been super-specialized to what is supported now
        //UNSUP remove below
                '@' '(' senitem ')' yDISABLE yIFF '(' व्यञ्जकम् ')' property_व्यञ्जकम्_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($3); $$->disablep($8); }
        |       '@' '(' senitem ')' property_व्यञ्जकम्_Spec
                        { $$ = $5; $$->fileline($1); $$->sensesp($3); }
        |       '@' senitemVar property_व्यञ्जकम्_Spec
                        { $$ = $3; $$->fileline($1); $$->sensesp($2); }
        |       yDISABLE yIFF '(' व्यञ्जकम् ')' '@' '(' senitem ')' property_व्यञ्जकम्_Spec
                        { $$ = $10; $$->fileline($1); $$->sensesp($8); $$->disablep($4); }
        //UNSUP remove above
        |       yDISABLE yIFF '(' व्यञ्जकम् ')' property_व्यञ्जकम्_Spec
                        { $$ = $6; $$->fileline($4->fileline()); $$->disablep($4); }
        |       property_व्यञ्जकम्_Spec                       { $$ = $1; }
        ;

property_व्यञ्जकम्_Spec<propSpecp>:  // A property expression plus explicit weak/strong strength
                pexpr
                        { $$ = new AstPropSpec{$1->fileline(), nullptr, nullptr, $1}; }
        |       ySTRONG '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::STRONG}; }
        |       yWEAK '(' sexpr ')'
                        { $$ = new AstPropSpec{$1, nullptr, nullptr, $3, VPropStrength::WEAK}; }
        ;

property_व्यञ्जकम्_Case_If<ग्रन्थिः_Exprp>:  // IEEE: part of property_expr for if/case
                yCASE '(' व्यञ्जकम्/*expression_or_dist*/ ')' property_case_itemList yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, $5); }
        |       yCASE '(' व्यञ्जकम्/*expression_or_dist*/ ')' yENDCASE
                        { $$ = PARSEP->makePropertyCase($1, $3, nullptr); }
        |       yIF '(' व्यञ्जकम्/*expression_or_dist*/ ')' pexpr  %prec prIF
                        { $$ = new AstImplication{$1, $3, $5, true}; }
        |       yIF '(' व्यञ्जकम्/*expression_or_dist*/ ')' pexpr yELSE pexpr
                        { Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elseCondp = new AstLogNot{$1, $3->clone_वृक्षः_विशुद्धम्(false)};
                          $$ = new AstSAnd{$1, new AstImplication{$1, $3, $5, true},
                                           new AstImplication{$1, elseCondp, $7, true},
                                           /*propertyControl=*/true}; }
        ;

property_case_itemList<caseItemp>:  // IEEE: {property_case_item}
                property_case_item                      { $$ = $1; }
        |       property_case_itemList property_case_item   { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

property_case_item<caseItemp>:  // ==IEEE: property_case_item
        //                      // IEEE: expression_or_dist { ',' expression_or_dist } ':' property_expr
        //                      // IEEE 1800-2012 changed from property_statement to property_expr
        //                      // IEEE 1800-2017 changed to require the semicolon
                caseCondList ':' pexpr ';'              { $$ = new AstCaseItem{$2, $1, $3}; }
        |       yDEFAULT pexpr ';'                      { $$ = new AstCaseItem{$1, nullptr, $2}; }
        |       yDEFAULT ':' pexpr ';'                  { $$ = new AstCaseItem{$1, nullptr, $3}; }
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

pexpr<ग्रन्थिः_Exprp>:  // IEEE: property_expr  (The name pexpr is important as regexps just add an "p" to expr.)
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
        |       property_व्यञ्जकम्_Case_If                     { $$ = $1; }
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
        |       yNEXTTIME '[' const_व्यञ्जकम् ']' pexpr %prec yNEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: nexttime[] (in property expression)"); DEL($3); }
        |       yS_NEXTTIME '[' const_व्यञ्जकम् ']' pexpr %prec yS_NEXTTIME
                        { $$ = $5; BBUNSUP($1, "Unsupported: s_nexttime[] (in property expression)"); DEL($3); }
        |       yALWAYS pexpr  %prec prALWAYS
                        { $$ = $2; }
        |       yALWAYS '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']' pexpr  %prec prALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, false}; }
        |       yS_ALWAYS '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']' pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $7, $3, $5, true}; }
        |       yS_ALWAYS pexpr  %prec prS_ALWAYS
                        { $$ = new AstPropAlways{$1, $2, new AstUnbounded{$1}, new AstUnbounded{$1}, true}; }
        |       yS_EVENTUALLY pexpr  %prec prS_EVENTUALLY
                        {
                            $$ = new AstSEventually{$1, $2};
                            PARSEP->import_If_अन्तर्_Std($1, "process", true);
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
        |       ~o~pexpr y_उपपादयति pexpr
                        { $$ = new AstLogOr{$2, new AstLogNot{$2, $1}, $3}; }
        //                      // yIFF also used by event_expression
        |       ~o~pexpr yIFF pexpr
                        { $$ = new AstLogEq{$2, $1, $3}; }
        |       yACCEPT_ON '(' व्यञ्जकम्/*expression_or_dist*/ ')' pexpr  %prec prACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_विधा::ACCEPT_ON, $3, $5}; }
        |       yREJECT_ON '(' व्यञ्जकम्/*expression_or_dist*/ ')' pexpr  %prec prREJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_विधा::REJECT_ON, $3, $5}; }
        |       ySYNC_ACCEPT_ON '(' व्यञ्जकम्/*expression_or_dist*/ ')' pexpr %prec prSYNC_ACCEPT_ON
                        { $$ = new AstAbortOn{$1, VAbort_विधा::SYNC_ACCEPT_ON, $3, $5}; }
        |       ySYNC_REJECT_ON '(' व्यञ्जकम्/*expression_or_dist*/ ')' pexpr %prec prSYNC_REJECT_ON
                        { $$ = new AstAbortOn{$1, VAbort_विधा::SYNC_REJECT_ON, $3, $5}; }
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
        |       BISONPRE_COPY_ONCE(व्यञ्जकम्,{s/~l~/p/g; s/~p~/p/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

sexpr<ग्रन्थिः_Exprp>:  // ==IEEE: sequence_expr  (The name sexpr is important as regexps just add an "s" to expr.)
        //                      // ********* RULES COPIED IN sequence_exprProp
        //                      // For precedence, see IEEE 17.7.1
        //
        //                      // IEEE: "cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // IEEE: "sequence_expr cycle_delay_range sequence_expr { cycle_delay_range sequence_expr }"
        //                      // Both rules basically mean we can repeat sequences, so make it simpler:
                आवृत्तिः_delay_range ~p~sexpr  %prec yP_POUNDPOUND
                        { $$ = new AstSExpr{$<fl>1, $1, $2}; }
        |       ~p~sexpr आवृत्तिः_delay_range sexpr %prec prPOUNDPOUND_MULTI
                        { $$ = new AstSExpr{$<fl>2, $1, $2, $3}; }
        //
        //                      // IEEE: expression_or_dist [ boolean_abbrev ]
        //                      // Note expression_or_dist includes "expr"!
        //                      // sexpr/*sexpression_or_dist*/  --- Hardcoded below
        //                      // Consecutive repetition (IEEE 1800-2023 16.9.2)
        //                      // [*N] exact count
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_व्यञ्जकम् ']'
                        { $$ = new AstSConsRep{$<fl>2, $1, $3}; }
        //                      // [*N:M] bounded range or [*N:$] unbounded range
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRASTAR const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                        {
                            यदि (VN_IS($5, Unbounded)) {
                                DEL($5);
                                $$ = new AstSConsRep{$<fl>2, $1, $3, nullptr, true};
                            } अन्यथा {
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
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_व्यञ्जकम् ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3}; }
        //                      // IEEE: goto_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAMINUSGT const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                        { $$ = new AstSGotoRep{$<fl>2, $1, $3, $5}; }
        //                      // IEEE: nonconsecutive_repetition (single count form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_व्यञ्जकम् ']'
                        { $$ = new AstSNonConsRep{$<fl>2, $1, $3}; }
        //                      // IEEE: nonconsecutive_repetition (range form)
        |       ~p~sexpr/*sexpression_or_dist*/ yP_BRAEQ const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
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
        |       '(' ~p~sexpr ',' sequence_मेलनम्_item_List ')'
                        { $$ = new Ast_व्यञ्जकम्_Stmt{$3, $4, $2}; }
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
        |       y_FIRST_मेलनम् '(' sexpr ')'
                        { $$ = $3; }  // IEEE 16.9.8: no-op in deterministic DFA model
        |       y_FIRST_मेलनम् '(' sexpr ',' sequence_मेलनम्_item_List ')'
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
        |       BISONPRE_COPY_ONCE(व्यञ्जकम्,{s/~l~/s/g; s/~p~/s/g; s/~noPar__IGNORE~'.'/yP_PAR__IGNORE /g; })  // {copied}
        ;

आवृत्तिः_delay_range<delayp>:  // IEEE: ==cycle_delay_range
        //                      // These three terms in 1800-2005 ONLY
                yP_POUNDPOUND intnumAsConst
                        { $$ = new AstDelay{$1, $2, true}; }
        |       yP_POUNDPOUND idAny
                        { $$ = new AstDelay{$1, new AstParseRef{$<fl>2, *$2}, true}; }
        |       yP_POUNDPOUND '(' const_व्यञ्जकम् ')'
                        { $$ = new AstDelay{$1, $3, true}; }
        //                      // In 1800-2009 ONLY:
        //                      // IEEE: yP_POUNDPOUND constant_primary
        //                      // UNSUP: This causes a big grammar ambiguity
        //                      // as ()'s mismatch between primary and the following statement
        //                      // the sv-ac committee has been asked to clarify  (Mantis 1901)
        |       yP_POUNDPOUND '[' const_व्यञ्जकम् ':' const_व्यञ्जकम् ']'
                        { $$ = new AstDelay{$1, $3, true};
                                                                                                        $$->rhsp($5); }
        |       yP_POUNDPOUND yP_BRASTAR ']'
                        { $$ = new AstDelay{$1, new AstConst{$1, 0}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        |       yP_POUNDPOUND yP_BRAPLUSKET
                        { $$ = new AstDelay{$1, new AstConst{$1, 1}, true};
                          $$->rhsp(new AstUnbounded{$1}); }
        ;

sequence_मेलनम्_item_List<nodep>:  // IEEE: [sequence_match_item] part of sequence_expr
                sequence_मेलनम्_item                     { $$ = $1; }
        |       sequence_मेलनम्_item_List ',' sequence_मेलनम्_item { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

sequence_मेलनम्_item<nodep>:  // ==IEEE: sequence_match_item
        //                      // IEEE says: operator_assignment
        //                      // IEEE says: inc_or_dec_expression
        //                      // IEEE says: subroutine_call
        //                      // This is the same list as...
                for_चरणः_assignment                     { $$ = $1; }
        ;

//      boolean_abbrev -- all forms now handled directly in sexpr rule:
//                      // IEEE: consecutive_repetition -- [*N], [*N:M], [+], [*] via AstSConsRep
//                      // IEEE: goto_repetition -- [->N], [->M:N] via AstSGotoRep
//                      // IEEE: nonconsecutive_repetition -- [=N], [=M:N] via AstSNonConsRep

//************************************************
// Covergroup

covergroup_declaration<nodep>:  // ==IEEE: covergroup_declaration
                 yCOVERGROUP idAny cg_पत्तनम्_List_E coverage_eventE ';'
        /*cont*/    coverage_spec_or_optionListE
        /*cont*/ yENDGROUP अन्तः_Label_E
                        { Ast_Sen_वृक्षः* clockp = nullptr;
                          Ast_ग्रन्थिः* sampleArgsp = nullptr;
                          यदि ($4) {
                              यदि (VN_IS($4, SenItem)) {
                                  clockp = new Ast_Sen_वृक्षः{$<fl>1, VN_AS($4, SenItem)};
                              } अन्यथा {
                                  sampleArgsp = $4;
                              }
                          }
                          $$ = new AstCovergroup{$<fl>1, *$2, static_cast<AstVar*>($3),
                                                 static_cast<AstVar*>(sampleArgsp), $6, clockp};
                          // Every covergroup has option/type_option members (added by V3LinkParse)
                          // referencing std:: types, so mark std as needed at parse time.
                          v3Global.समुच्चयः_उपयुङ्क्ते_Std_Package();
                          GRAMMARP->अन्तः_Label($<fl>8, $$, $8); }
        |        yCOVERGROUP yEXTENDS idAny ';'
        /*cont*/     coverage_spec_or_optionListE
        /*cont*/ yENDGROUP अन्तः_Label_E
                        { $$ = nullptr;
                          BBUNSUP($1, "Unsupported: covergroup inheritance (extends)");
                          DEL($5); }
        ;

cg_पत्तनम्_List_E<nodep>:
                /*empty*/                               { $$ = nullptr; }
        |       '(' tf_पत्तनम्_list_E ')'                   { $$ = $2; }
        ;

cgexpr<ग्रन्थिः_Exprp>:  // IEEE-2012: covergroup_expression, before that just expression
                व्यञ्जकम्                                    { $$ = $1; }
        ;

coverage_spec_or_optionListE<nodep>:  // IEEE: [{coverage_spec_or_option}]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       coverage_spec_or_optionList             { $$ = $1; }
        ;

coverage_spec_or_optionList<nodep>:  // IEEE: {coverage_spec_or_option}
                coverage_spec_or_option                 { $$ = $1; }
        |       coverage_spec_or_optionList coverage_spec_or_option     { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

coverage_spec_or_option<nodep>:  // ==IEEE: coverage_spec_or_option
        //                      // IEEE: coverage_spec
                cover_point                             { $$ = $1; }
        |       cover_cross                             { $$ = $1; }
        |       coverage_option ';'                     { $$ = $1; }
        |       error                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

coverage_option<nodep>:  // ==IEEE: coverage_option
        //                      // option/type_option aren't really keywords
                id/*yOPTION | yTYPE_OPTION*/ '.' idAny/*member_identifier*/ '=' व्यञ्जकम्
                        { $$ = nullptr;
                          यदि (*$1 == "option" || *$1 == "type_option") {
                              ध्रुव bool typeOpt = (*$1 == "type_option");
                              VCoverOptionType optType = VCoverOptionType::UNKNOWN;
                              bool valid = true;
                              यदि (!typeOpt) {
                                  // IEEE 1800-2023 Table 19-1 option.* names and ignored legacy names
                                  यदि      (*$3 == "at_least")                optType = VCoverOptionType::AT_न्यूनतमम्;
                                  अन्यथा यदि (*$3 == "auto_bin_max")            optType = VCoverOptionType::AUTO_BIN_MAX;
                                  अन्यथा यदि (*$3 == "comment")                 optType = VCoverOptionType::COMMENT;
                                  अन्यथा यदि (*$3 == "cross_auto_bin_max")      optType = VCoverOptionType::CROSS_AUTO_BIN_MAX;
                                  अन्यथा यदि (*$3 == "cross_num_print_missing") optType = VCoverOptionType::CROSS_अङ्कः_PRINT_MISSING;
                                  अन्यथा यदि (*$3 == "cross_retain_auto_bins")  optType = VCoverOptionType::CROSS_RETAIN_AUTO_BINS;
                                  अन्यथा यदि (*$3 == "detect_overlap")          optType = VCoverOptionType::DETECT_OVERLAP;
                                  अन्यथा यदि (*$3 == "get_inst_coverage")       optType = VCoverOptionType::GET_INST_COVERAGE;
                                  अन्यथा यदि (*$3 == "goal")                    optType = VCoverOptionType::GOAL;
                                  अन्यथा यदि (*$3 == "name")                    optType = VCoverOptionType::NAME;
                                  अन्यथा यदि (*$3 == "per_instance")            optType = VCoverOptionType::प्रत्येकम्_INSTANCE;
                                  अन्यथा यदि (*$3 == "weight")                  optType = VCoverOptionType::WEIGHT;
                                  अन्यथा {
                                      $<fl>1->v3error("Unknown coverage option name 'option."
                                                      << *$3 << "'"
                                                      << "; not a valid option (IEEE 1800-2023 Table 19-1)");
                                      valid = false;
                                  }
                              } अन्यथा {
                                  // IEEE 1800-2023 Table 19-3: type_option.* names
                                  यदि      (*$3 == "comment")          optType = VCoverOptionType::COMMENT;
                                  अन्यथा यदि (*$3 == "distribute_first") optType = VCoverOptionType::DISTRIBUTE_FIRST;
                                  अन्यथा यदि (*$3 == "goal")             optType = VCoverOptionType::GOAL;
                                  अन्यथा यदि (*$3 == "merge_instances")  optType = VCoverOptionType::MERGE_INSTANCES;
                                  अन्यथा यदि (*$3 == "real_interval")    optType = VCoverOptionType::REAL_INTERVAL;
                                  अन्यथा यदि (*$3 == "strobe")           optType = VCoverOptionType::STROBE;
                                  अन्यथा यदि (*$3 == "weight")           optType = VCoverOptionType::WEIGHT;
                                  अन्यथा {
                                      // Specific message for option.* names used under type_option.*
                                      यदि (*$3 == "at_least" || *$3 == "auto_bin_max"
                                          || *$3 == "cross_num_print_missing" || *$3 == "cross_retain_auto_bins"
                                          || *$3 == "detect_overlap" || *$3 == "get_inst_coverage"
                                          || *$3 == "name" || *$3 == "per_instance") {
                                          $<fl>1->v3error("'type_option." << *$3
                                              << "' is not valid; use 'option." << *$3 << "' instead");
                                      } अन्यथा {
                                          $<fl>1->v3error("Unknown coverage type option name 'type_option."
                                              << *$3 << "'"
                                              << "; not a valid type option (IEEE 1800-2023 Table 19-3)");
                                      }
                                      valid = false;
                                  }
                              }
                              यदि (valid) {
                                  $$ = new Ast_Cg_Option_नियोजय{$<fl>1, typeOpt, optType, *$3, $5};
                              } अन्यथा {
                                  DEL($5);
                              }
                          } अन्यथा {
                              $<fl>1->v3error("Syntax error; expected 'option' or 'type_option': '" << *$1 << "'");
                              DEL($5);
                          } }
        ;

cover_point<nodep>:  // ==IEEE: cover_point
        //              // [ [ data_type_or_implicit ] cover_point_identifier ':' ] yCOVERPOINT
                yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>1, "", $2, $3, $4}; }
        //                      // IEEE-2012: class_scope before an ID
        |       id/*cover_point_id*/ ':' yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>3, *$1, $4, $5, $6}; }
        //                      // data_type_or_implicit expansion
        |       दत्त_type id/*cover_point_id*/ ':' yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7};
                          DEL($1); }
        |       yVAR दत्त_type id/*cover_point_id*/ ':' yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       yVAR implicit_typeE id/*cover_point_id*/ ':' yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signingE rangeList id/*cover_point_id*/ ':' yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>5, *$3, $6, $7, $8};
                          DEL($2); }
        |       signing id/*cover_point_id*/ ':' yCOVERPOINT व्यञ्जकम् iffE bins_or_रिक्तानि
                        { $$ = new AstCoverpoint{$<fl>4, *$2, $5, $6, $7}; }
        //                      // IEEE-2012:
        |       bins_or_रिक्तानि                           { $$ = $1; }
        ;

iffE<ग्रन्थिः_Exprp>:  // IEEE: part of cover_point, others
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       yIFF '(' व्यञ्जकम् ')'
                        { $$ = $3; /* Keep iff condition for coverpoint */ }
        ;

bins_or_रिक्तानि<nodep>:  // ==IEEE: bins_or_empty
                '{' bins_or_optionsList '}'             { $$ = $2; }
        |       '{' '}'                                 { $$ = nullptr; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' bins_or_optionsList error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

bins_or_optionsList<nodep>:  // IEEE: { bins_or_options ';' }
                bins_or_options ';'                     { $$ = $1; }
        |       bins_or_optionsList bins_or_options ';' { $$ = सङ्कलय_अग्र_Null($1, $2); }
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
                          यदि (*$2 == "auto") {
                              $$ = new AstCoverBin{$<fl>2, *$2, $4};
                              DEL($6);
                          } अन्यथा {
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
                        { ध्रुव bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_समुच्चयः*>($5), VCoverBinsType{VCoverBinsType::BINS_TRANSITION}, isArray};
                          यदि ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { ध्रुव bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_समुच्चयः*>($5), VCoverBinsType{VCoverBinsType::BINS_IGNORE}, isArray};
                          यदि ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' trans_list iffE
                        { ध्रुव bool isArray = GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, static_cast<Ast_Cover_Trans_समुच्चयः*>($5), VCoverBinsType{VCoverBinsType::BINS_ILLEGAL}, isArray};
                          यदि ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in transition cover bin (guard ignored)");
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
                          यदि ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_IGNORE};
                          यदि ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT iffE
                        { GRAMMARP->coverBinArrayUnsized($3);
                          $$ = new AstCoverBin{$<fl>2, *$2, VCoverBinsType::BINS_ILLEGAL};
                          यदि ($6) BBCOVERIGN($6->fileline(), "Unsupported: 'iff' in default cover bin (guard ignored)");
                          DEL($6); }
        |       yBINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yIGNORE_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        |       yILLEGAL_BINS idAny/*bin_identifier*/ bins_orBraE '=' yDEFAULT ySEQUENCE iffE
                        { $$ = nullptr; GRAMMARP->coverBinArrayUnsized($3); BBCOVERIGN($<fl>6, "Unsupported: 'sequence' in default cover bin"); DEL($7); }
        ;

bins_orBraE<ग्रन्थिः_Exprp>:  // IEEE: part of bins_or_options: AstUnbounded for '[]', else the size
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       '[' ']'                                 { $$ = new AstUnbounded{$<fl>1}; }
        |       '[' cgexpr ']'
                        { $$ = $2;
                          यदि (VN_IS($2, Unbounded)) $2->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)"); }
        ;

trans_list<nodep>:  // ==IEEE: trans_list
                '(' trans_समुच्चयः ')'                       { $$ = $2; }
        |       trans_list ',' '(' trans_समुच्चयः ')'        { $$ = सङ्कलय_अग्र_Null($1, $4); }
        ;

trans_समुच्चयः<nodep>:  // ==IEEE: trans_set (returns AstCoverTransSet)
                trans_range_list                        {
                        AstCoverTransItem* ध्रुव itemp = static_cast<AstCoverTransItem*>($1);
                        $$ = new Ast_Cover_Trans_समुच्चयः{$<fl>1, itemp};
                }
        |       trans_समुच्चयः yP_EQGT trans_range_list
                {
                        Ast_Cover_Trans_समुच्चयः* ध्रुव setp = static_cast<Ast_Cover_Trans_समुच्चयः*>($1);
                        AstCoverTransItem* ध्रुव itemp = static_cast<AstCoverTransItem*>($3);
                        setp->सङ्कलय_Itemsp(itemp);
                        $$ = setp;
                }
        ;

trans_range_list<nodep>:  // ==IEEE: trans_range_list (returns AstCoverTransItem)
                trans_item                              {
                        // Simple transition item without repetition
                        $$ = new AstCoverTransItem{$<fl>1, $1, VTransRepType::NONE};
                }
        |       trans_item yP_BRASTAR cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3); }
        |       trans_item yP_BRASTAR cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[*]' in cover transition"); DEL($1, $3, $5); }
        |       trans_item yP_BRAMINUSGT cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($3);
                          $$ = new AstCoverTransItem{$<fl>1, $1, VTransRepType::GOTO}; }
        |       trans_item yP_BRAMINUSGT cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[->' in cover transition"); DEL($1, $3, $5); }
        |       trans_item yP_BRAEQ cgexpr ']'
                        { BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($3);
                          $$ = new AstCoverTransItem{$<fl>1, $1, VTransRepType::NONCONS}; }
        |       trans_item yP_BRAEQ cgexpr ':' cgexpr ']'
                        { $$ = nullptr; BBCOVERIGN($<fl>2, "Unsupported: '[=]' in cover transition"); DEL($1, $3, $5); }
        ;

trans_item<nodep>:  // ==IEEE: range_list (returns range list node)
                covergroup_range_list                   { $$ = $1; }
        ;

covergroup_range_list<nodep>:  // ==IEEE: covergroup_range_list
                covergroup_मानम्_range                  { $$ = $1; }
        |       covergroup_range_list ',' covergroup_मानम्_range        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;


cover_cross<nodep>:  // ==IEEE: cover_cross
                id/*cover_point_identifier*/ ':' yCROSS list_सम्बन्धि_cross_वस्तूनि iffE cross_body
                        {
                          AstCoverCross* ध्रुव nodep = new AstCoverCross{$<fl>3, *$1,
                                                          VN_AS($4, CoverpointRef), $5};
                          यदि ($6) nodep->सङ्कलय_Binsp($6);
                          $$ = nodep;
                        }
        |       yCROSS list_सम्बन्धि_cross_वस्तूनि iffE cross_body
                        {
                          AstCoverCross* ध्रुव nodep = new AstCoverCross{$<fl>1,
                                                          "__cross" + cvt_गन्तव्यम्_Str(GRAMMARP->s_type_Imp_अङ्कः++),
                                                          VN_AS($2, CoverpointRef), $3};
                          यदि ($4) nodep->सङ्कलय_Binsp($4);
                          $$ = nodep;
                        }
        ;

list_सम्बन्धि_cross_वस्तूनि<nodep>:  // ==IEEE: list_of_cross_items
                cross_item ',' cross_item               { $$ = सङ्कलय_अग्र_Null($1, $3); }
        |       cross_item ',' cross_item ',' cross_itemList
                        { $$ = सङ्कलय_अग्र_Null(सङ्कलय_अग्र_Null($1, $3), $5); }
        ;

cross_itemList<nodep>:  // IEEE: part of list_of_cross_items
                cross_item                              { $$ = $1; }
        |       cross_itemList ',' cross_item           { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

cross_item<nodep>:  // ==IEEE: cross_item
        //                      // IEEE: cover_point_identifier | variable_identifier - both are a
        //                      // simple identifier.  We parse idDotted (a plain hierarchical
        //                      // reference a.b.c, with no bit/array selects) to also accept the
        //                      // non-standard dotted form (e.g. 'cross a.b') that several
        //                      // simulators support; the common simple-identifier case is detected
        //                      // and handled exactly as before.
                idDotted
                        {
                          यदि (AstParseRef* ध्रुव refp = VN_CAST($1, ParseRef)) {
                              // Standard: simple cover_point_identifier / variable_identifier
                              $$ = new AstCoverpointRef{refp->fileline(), refp->name()};
                              VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
                          } अन्यथा {
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
        |       '{' cross_body_itemList '}'             { $$ = $2; }
        |       ';'                                     { $$ = nullptr; }
        //
        |       '{' cross_body_itemList error '}'       { $$ = $2; }  // LCOV_EXCL_LINE
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

cross_body_itemList<nodep>:  // IEEE: part of cross_body
                cross_body_item                         { $$ = $1; }
        |       cross_body_itemList cross_body_item     { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

cross_body_item<nodep>:  // ==IEEE: cross_body_item
                function_declaration
                        { $$ = nullptr; BBCOVERIGN($1->fileline(), "Unsupported: 'function' in coverage cross body"); DEL($1); }
        //                      // IEEE: bins_selection_or_option
        |       coverage_option ';'                     { $$ = $1; }
        //                      // IEEE: bins_selection
        |       yBINS idAny/*new-bin_identifier*/ '=' select_expression iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5}; }
        |       yIGNORE_BINS idAny/*new-bin_identifier*/ '=' select_expression iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_IGNORE}; }
        |       yILLEGAL_BINS idAny/*new-bin_identifier*/ '=' select_expression iffE ';'
                        { $$ = new AstCoverCrossBin{$1, *$2, $4, $5, VCoverBinsType::BINS_ILLEGAL}; }
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

select_expression<nodep>:  // ==IEEE: select_expression
                select_expression_r
                        { $$ = $1; }
        //                      // IEEE: integer_covergroup_expression ::= covergroup_expression | '$'
        //                      // '$' is part of our expr.  'matches' has higher precedence than
        //                      // '&&' and '||', which end the count (IEEE 1800-2023 footnote 30).
        //                      // A count could end in a function call followed by its 'with', so
        //                      // a selection with a count is filtered further only in parentheses.
        |       select_expression_r yWITH__PAREN '(' cgexpr ')' yMATCHES व्यञ्जकम् %prec yP_ANDAND
                        { $$ = new AstCoverCrossWith{$2, $1, $4, $7}; }
        //                      // IEEE: cross_set_expression yMATCHES integer_covergroup_expression
        |       idAny '(' list_सम्बन्धि_arguments_E ')' yMATCHES व्यञ्जकम् %prec yP_ANDAND
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3, $6); }
        |       select_expression yP_ANDAND select_expression
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, false}; }
        |       select_expression yP_OROR   select_expression
                        { $$ = new AstCoverCrossSelect{$2, $1, $3, true}; }
        ;

// This non-terminal exists to disambiguate select_expression and make "with" bind tighter
select_expression_r<nodep>:
        //                      // IEEE: select_condition expanded here
                yBINSOF '(' bins_expression ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}}; }
        |       '!' yBINSOF '(' bins_expression ')'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true}; }
        //                      // IEEE: covergroup_range_list has the same syntax as range_list
        |       yBINSOF '(' bins_expression ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$3->fileline(), $3}, false, $7}; }
        |       '!' yBINSOF '(' bins_expression ')' yINTERSECT '{' range_list '}'
                        { $$ = new AstCoverBinsof{$1, new AstCoverpointRef{$4->fileline(), $4}, true, $8}; }
        |       yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($3); }
        |       '!' yWITH__PAREN '(' cgexpr ')'
                        { $$ = nullptr; BBCOVERIGN($1, "Unsupported: 'with' in coverage select expression"); DEL($4); }
        |       select_expression_r yWITH__PAREN '(' cgexpr ')'
                        { $$ = new AstCoverCrossWith{$2, $1, $4, nullptr}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr    { }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP '!' yWITH__PAREN '(' cgexpr ')' yMATCHES cgexpr { }
        //
        |       '(' select_expression ')'                       { $$ = $2; }
        |       idAny/*cross_identifier*/                        { $$ = new AstCoverCrossRef{$<fl>1, *$1}; }
        //                      // IEEE-2012: Need clarification as to precedence
        //UNSUP  cgexpr  { $$ = nullptr; BBCOVERIGN($1, "Ignoring unsupported: coverage select expression"); }
        //
        //                      // IEEE: cross_set_expression [ yMATCHES integer_covergroup_expression ]
        //UNSUP  cgexpr    {..}
        //UNSUP                 // Below are all removed
        |       idAny '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: function call in coverage select expression"); DEL($3); }
        //UNSUP                 // Above are all removed, replace with:
        ;

bins_expression<ग्रन्थिः_Exprp>:  // ==IEEE: bins_expression
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
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       clocking_event
                        { $$ = $1; }  // Keep the clocking event for automatic sampling
        |       yWITH__ETC yFUNCTION idAny/*"sample"*/ '(' tf_पत्तनम्_list_E ')'
                        { यदि (*$3 != "sample") {
                            $<fl>3->v3error("Coverage sampling function must be named 'sample'"
                                            " (IEEE 1800-2023 19.8.1)");
                            $$ = nullptr;
                            DEL($5);
                          } अन्यथा {
                            $$ = $5;
                          }
                        }
        |       yP_ATAT '(' खण्डम्_event_expression ')'
                        { $$ = nullptr; BBCOVERIGN($<fl>1, "Unsupported: '@@' coverage event"); DEL($3); }
        ;

खण्डम्_event_expression<nodep>:  // ==IEEE: block_event_expression
                खण्डम्_event_expression_पदांशः              { $$ = nullptr; /*UNSUP @@*/ DEL($1); }
        |       खण्डम्_event_expression yOR खण्डम्_event_expression_पदांशः   { $$ = nullptr; /*UNSUP @@*/ DEL($1, $3);  }
        ;

खण्डम्_event_expression_पदांशः<nodep>:  // IEEE: part of block_event_expression
                yBEGIN श्रेणीबद्धम्_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        |       y_अन्तः   श्रेणीबद्धम्_btf_identifier      { $$ = nullptr; /*UNSUP @@*/ DEL($2); }
        ;

श्रेणीबद्धम्_btf_identifier<nodep>:  // ==IEEE: hierarchical_btf_identifier
        //                      // hierarchical_tf_identifier + hierarchical_block_identifier
        //                      // method_identifier
                packageClassScopeE idAny                { $$ = nullptr; /*UNSUP*/ DEL($1); }
        ;

//**********************************************************************
// Randsequence

randsequence_statement<ग्रन्थिः_Stmtp>:  // ==IEEE: randsequence_statement
                yRANDSEQUENCE '(' ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, "", $4};
                          v3Global.useRandSequence(true); }
        |       yRANDSEQUENCE '(' idAny/*rs_production_identifier*/ ')' rs_productionList yENDSEQUENCE
                        { $$ = new AstRandSequence{$1, *$3, $5};
                          v3Global.useRandSequence(true); }
        ;

rs_productionList<rSProdp>:  // IEEE: rs_production+
                rs_production                           { $$ = $1; }
        |       rs_productionList rs_production         { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

rs_production<rSProdp>:  // ==IEEE: rs_production
                rs_productionFront ':' rs_नियमः_List ';'
                        { $$ = $1; $1->सङ्कलय_Rulesp($3); }
        ;

rs_productionFront<rSProdp>:  // IEEE: part of rs_production
                rs_funcId/*rs_production_identifier*/   { $$ = $1; }
        |       rs_funcId '(' tf_पत्तनम्_list_E ')'         { $$ = $1; $$->सङ्कलय_Portsp($3); }
        ;

rs_funcId<rSProdp>:  // IEEE: part of rs_production
                /**/ rs_fId
                        { $$ = $1; }  // Note is void as default, not logic as default like functions
        |       signingE rangeList rs_fId
                        { $$ = $3;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::पत्तनम्, $3->name(), VFlagChildDType{},
                                               GRAMMARP->सङ्कलय_Range(new AstBasicDType{$<fl>3, LOGIC_IMPLICIT, $1}, $2, true)}); }
        |       signing rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::पत्तनम्, $2->name(), VFlagChildDType{},
                                               new AstBasicDType{$<fl>2, LOGIC_IMPLICIT, $1}}); }
        |       दत्त_type rs_fId
                        { $$ = $2;
                          $$->fvarp(new AstVar{$<fl>1, VVarType::पत्तनम्, $2->name(), VFlagChildDType{},
                                               $1}); }
        |       yVOID rs_fId
                        { $$ = $2; }
        ;

rs_fId<rSProdp>:  // IEEE: part of rs_production
                id
                        { $<fl>$ = $<fl>1;
                          $$ = new AstRSProd{$<fl>$, *$1, nullptr, nullptr}; }
        ;

rs_नियमः_List<rSRulep>:  // IEEE: rs_rule+ part of rs_production
                rs_नियमः                                 { $$ = $1; }
        |       rs_नियमः_List '|' rs_नियमः                 { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

rs_नियमः<rSRulep>:  // ==IEEE: rs_rule
                rs_production_list
                        { $$ = new AstRSRule{$1->fileline(), nullptr, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_weight_specification
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, nullptr}; }
        |       rs_production_list yP_COLONEQ rs_weight_specification rs_संहिता_खण्डम्
                        { $$ = new AstRSRule{$1->fileline(), $3, $1, $4}; }
        ;

rs_production_list<rSProdListp>:  // ==IEEE: rs_production_list
                rs_prodList
                        { $$ = new AstRSProdList{CRELINE(), nullptr, $1}; }
        |       yRAND yJOIN rs_production_item rs_production_itemList
                        { $$ = new AstRSProdList{$1, new AstConst{$2, AstConst::RealDouble{}, 0.5}, $3};
                          $$->randJoin(true);
                          $$->सङ्कलय_Prodsp($4); }
        |       yRAND yJOIN '(' व्यञ्जकम् ')' rs_production_item rs_production_itemList
                        { $$ = new AstRSProdList{$1, $4, $6};
                          $$->randJoin(true);
                          $$->सङ्कलय_Prodsp($7); }
        ;

rs_weight_specification<ग्रन्थिः_Exprp>:  // ==IEEE: rs_weight_specification
                intnumAsConst                           { $$ = $1; }
        |       idClassSel/*ps_identifier*/             { $$ = $1; }
        |       '(' व्यञ्जकम् ')'                            { $$ = $2; }
        ;

rs_संहिता_खण्डम्<nodep>:  // ==IEEE: rs_code_block
                '{' '}'                                 { $$ = nullptr; }
        |       '{' rs_संहिता_खण्डम्_Item_List '}'           { $$ = $2; }
        ;

rs_संहिता_खण्डम्_Item_List<nodep>:  // IEEE: part of rs_code_block
                rs_संहिता_खण्डम्_Item                       { $$ = $1; }
        |       rs_संहिता_खण्डम्_Item_List rs_संहिता_खण्डम्_Item         { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

rs_संहिता_खण्डम्_Item<nodep>:  // IEEE: part of rs_code_block
                दत्त_declaration                        { $$ = $1; }
        |       stmt                                    { $$ = $1; }
        ;

rs_prodList<nodep>:  // IEEE: rs_prod+
                rs_prod                                 { $$ = $1; }
        |       rs_prodList rs_prod                     { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

rs_prod<nodep>:  // ==IEEE: rs_prod
                rs_production_item                      { $$ = $1; }
        |       rs_संहिता_खण्डम्                           { $$ = $1; }
        //                      // IEEE: rs_if_else
        |       yIF '(' व्यञ्जकम् ')' rs_production_item %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = new AstIf{$<fl>1, $3, $5, nullptr}; }
        |       yIF '(' व्यञ्जकम् ')' rs_production_item yELSE rs_production_item
                        { $$ = new AstIf{$<fl>1, $3, $5, $7}; }
        //                      // IEEE: rs_repeat
        |       yREPEAT '(' व्यञ्जकम् ')' rs_production_item
                               { $$ = new AstRepeat{$<fl>1, $3, $5}; }
        //                      // IEEE: rs_case
        |       yCASE '(' व्यञ्जकम् ')' rs_case_itemList yENDCASE
                        { $$ = new AstCase{$<fl>1, VCaseType::CT_RANDSEQUENCE, $3, $5}; }
        ;

rs_production_itemList<nodep>:  // IEEE: rs_production_item+
                rs_production_item                         { $$ = $1; }
        |       rs_production_itemList rs_production_item  { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

rs_production_item<nodep>:  // ==IEEE: rs_production_item
                idAny/*production_identifier*/
                        { $$ = new AstRSProdItem{$<fl>1, *$1, nullptr}; }
        |       idAny/*production_identifier*/ '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = new AstRSProdItem{$<fl>1, *$1, $3}; }
        ;

rs_case_itemList<caseItemp>:  // IEEE: rs_case_item+
                rs_case_item                            { $$ = $1; }
        |       rs_case_itemList rs_case_item           { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

rs_case_item<caseItemp>:  // ==IEEE: rs_case_item
                caseCondList ':' rs_production_item ';'
                        { $$ = new AstCaseItem{$<fl>1, $1, $3}; }
        |       yDEFAULT rs_production_item ';'
                        { $$ = new AstCaseItem{$<fl>1, nullptr, $2}; }
        |       yDEFAULT ':' rs_production_item ';'
                        { $$ = new AstCaseItem{$<fl>1, nullptr, $3}; }
        ;

//**********************************************************************
// Checker

checker_declaration<ग्रन्थिः_Modulep>:  // ==IEEE: part of checker_declaration
                checkerFront checker_पत्तनम्_list_E ';'
                        checker_or_generate_itemListE yENDCHECKER अन्तः_Label_E
                        { $$ = $1;
                          $1->modTrace(GRAMMARP->allTracingOn($1->fileline()));
                          यदि ($2) $1->सङ्कलय_Stmtsp($2);
                          यदि ($4) $1->सङ्कलय_Stmtsp($4);
                          GRAMMARP->अन्तः_Label($<fl>6, $1, $6); }
        ;

checkerFront<ग्रन्थिः_Modulep>:  // IEEE: part of checker_declaration
                yCHECKER idAny/*checker_identifier*/
                        { $$ = new AstModule{$<fl>2, *$2, PARSEP->libname(), AstModule::Checker{}};
                          $$->modTrace(GRAMMARP->allTracingOn($$->fileline()));
                          $$->timeunit(PARSEP->time_चरमः_Unit());
                          PARSEP->rootp()->timeprecisionMerge($$->fileline(),
                                                              PARSEP->time_चरमः_Prec());
                          $$->unconnectedDrive(PARSEP->unconnectedDrive()); }
        |       checkerFront sigAttrScope               { $$ = $1; }
        ;

checker_पत्तनम्_list_E<nodep>:  // IEEE: [ ( [ checker_port_list ] ) ]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       '(' ')'                                 { $$ = nullptr; }
        |       '('
        /*mid*/         { VARRESET_LIST(पत्तनम्); GRAMMARP->m_pinAnsi = true; }
        /*cont*/    checker_पत्तनम्_list ')'
                        { $$ = $3; }
        ;

checker_पत्तनम्_list<nodep>:  // ==IEEE: checker_port_list
                checker_पत्तनम्_item                             { $$ = $1; }
        |       checker_पत्तनम्_list ',' checker_पत्तनम्_item       { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

checker_पत्तनम्_item<nodep>:  // IEEE: checker_port_item
                checker_पत्तनम्_item_Front checker_पत्तनम्_item_Assignment  { $$ = $2; }
        ;

checker_पत्तनम्_item_Front:  // IEEE: part of checker_port_item
                checker_पत्तनम्_direction_E property_formal_typeNoDt
                        { VARDTYPE($2); }
        //                      // data_type_or_implicit
        |       checker_पत्तनम्_direction_E दत्त_type
                        { VARDTYPE($2); GRAMMARP->m_typed_Property_पत्तनम् = true; }
        |       checker_पत्तनम्_direction_E yVAR दत्त_type
                        { VARDTYPE($3); GRAMMARP->m_typed_Property_पत्तनम् = true; }
        |       checker_पत्तनम्_direction_E yVAR implicit_typeE
                        { VARDTYPE($3); }
        |       checker_पत्तनम्_direction_E implicit_typeE
                        { VARDTYPE($2); }
        ;

checker_पत्तनम्_direction_E:  // IEEE: [ checker_port_direction ]
                /*रिक्तानि*/                             { VARIO(निवेशः); }
        |       y_निवेशः                                  { VARIO(निवेशः); }
        |       yOUTPUT                                 { VARIO(OUTPUT); }
        ;

checker_पत्तनम्_item_Assignment<nodep>:  // IEEE: part of checker_port_direction
                id variable_dimensionListE
                        { $$ = new Ast_पत्तनम्{CRELINE(), PINNUMINC(), *$1};
                          $$->सङ्कलय_अग्र(VARDONEA($<fl>1, *$1, $2, nullptr)); }
        |       id variable_dimensionListE '=' property_actual_arg
                        { $$ = new Ast_पत्तनम्{CRELINE(), PINNUMINC(), *$1};
                          $$->सङ्कलय_अग्र(VARDONEA($<fl>1, *$1, $2, $4));
                          BBUNSUP($3, "Unsupported: checker port variable default value"); }
        ;

checker_or_generate_itemListE<nodep>:  // IEEE: [{ checker_or_generate_itemList }]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       checker_or_generate_itemList            { $$ = $1; }
        ;

checker_or_generate_itemList<nodep>:  // IEEE: { checker_or_generate_itemList }
                checker_or_generate_item                { $$ = $1; }
        |       checker_or_generate_itemList checker_or_generate_item   { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

checker_or_generate_item<nodep>:  // ==IEEE: checker_or_generate_item
                checker_or_generate_item_declaration    { $$ = $1; }
        |       initial_construct                       { $$ = $1; }
        //                      // IEEE: checker_construct
        |       always_construct                        { $$ = $1; }
        |       final_construct                         { $$ = $1; }
        |       assertion_item                          { $$ = $1; }
        |       continuous_नियोजय                       { $$ = $1; }
        |       checker_generate_item                   { $$ = $1; }
        ;

checker_or_generate_item_declaration<nodep>:  // ==IEEE: checker_or_generate_item_declaration
                दत्त_declaration                        { $$ = $1; }
        |       yRAND दत्त_declaration
                        { $$ = $2; BBUNSUP($1, "Unsupported: checker rand"); }
        |       function_declaration                    { $$ = $1; }
        |       checker_declaration
                        { $$ = nullptr; BBUNSUP($1, "Unsupported: recursive 'checker'"); DEL($1); }
        |       assertion_item_declaration              { $$ = $1; }
        |       covergroup_declaration                  { $$ = $1; }
        //      // IEEE deprecated: overload_declaration
        |       genvar_declaration                      { $$ = $1; }
        |       clocking_declaration                    { $$ = $1; }
        |       modDefaultClocking                      { $$ = $1; }
        |       defaultDisable                          { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        ;

checker_generate_item<nodep>:  // ==IEEE: checker_generate_item
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
                classFront parameter_पत्तनम्_list_E classExtendsE classImplementsE ';'
        /*mid*/         { // Allow resolving types declared in base extends class
                          $1->hasParameterList($<flag>2);
                        }
        /*cont*/    class_item_List_अन्तः अन्तः_Label_E
                        { $$ = $1; $1->सङ्कलय_Membersp($2);
                          $1->सङ्कलय_Extendsp($3);
                          $1->सङ्कलय_Extendsp($4);
                          $1->सङ्कलय_Membersp($7);
                          GRAMMARP->अन्तः_Label($<fl>8, $1, $8); }
        ;

classFront<classp>:             // IEEE: part of class_declaration
        //                      // IEEE 1800-2023: lifetimeE replaced with final_specifierE
                classVirtualE yCLASS final_specifierE lifetimeE idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$5, PARSEP->libname()};
                          $$->आधारः_Override($3);
                          $$->isVirtual($1);
                          v3Global.समुच्चयः_Has_Classes(); }
        //                      // IEEE: part of interface_class_declaration
        //                      // IEEE 1800-2023: lifetimeE removed
        |       yINTERFACE yCLASS idAny/*class_identifier*/
                        { $$ = new AstClass{$2, *$3, PARSEP->libname()};
                          $$->isInterfaceClass(true);
                          v3Global.समुच्चयः_Has_Classes(); }
        ;

classVirtualE<cbool>:
                /*रिक्तानि*/                             { $$ = false; }
        |       yVIRTUAL__CLASS                         { $$ = true; }
        ;

classExtendsE<classExtendsp>:           // IEEE: part of class_declaration
        //                      // The classExtendsE rule relies on classFront having the
        //                      // new class scope correct via classFront
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       yEXTENDS classExtendsList               { $$ = $2; }
        ;

classExtendsList<classExtendsp>:        // IEEE: part of class_declaration
                classExtendsOne                         { $$ = $1; }
        |       classExtendsList ',' classExtendsOne    { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

classExtendsOne<classExtendsp>:         // IEEE: part of class_declaration
                class_typeExtImpList
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_अन्तर्_Implements}; }
        |       class_typeExtImpList '(' list_सम्बन्धि_arguments_E ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_अन्तर्_Implements};
                          $$->सङ्कलय_Argsp($3); }
        //                      // IEEE-2023: Added: yEXTENDS class_type '(' yDEFAULT ')'
        |       class_typeExtImpList '(' yDEFAULT ')'
                        { $$ = new AstClassExtends{$1->fileline(), $1, GRAMMARP->m_अन्तर्_Implements};
                          BBUNSUP($<fl>2, "Unsupported: 'extends' with 'default'"); }
        ;

classImplementsE<classExtendsp>:        // IEEE: part of class_declaration
        //                      // All 1800-2012
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       yIMPLEMENTS
        /*mid*/         { GRAMMARP->m_अन्तर्_Implements = true; }
        /*cont*/    classImplementsList
                        { $$ = $3;
                          GRAMMARP->m_अन्तर्_Implements = false; }
        ;

classImplementsList<classExtendsp>:     // IEEE: part of class_declaration
        //                      // All 1800-2012
                classExtendsOne                         { $$ = $1; }
        |       classImplementsList ',' classExtendsOne
                        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

class_typeExtImpList<ग्रन्थिः_Exprp>:  // IEEE: class_type: "[package_scope] id [ parameter_value_assignment ]"
        //                      // but allow yaID__aTYPE for extends/implements
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                class_typeExtImpOne                     { $$ = $1; }
        |       class_typeExtImpList yP_COLONCOLON class_typeExtImpOne
                        { $$ = new AstDot{$<fl>1, true, $1, $3}; }
        ;

class_typeExtImpOne<ग्रन्थिः_Exprp>:  // part of IEEE: class_type, where we either get a package_scope component or class
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
        //                      // Not listed in IEEE, but see bug627 any parameter type maybe a class
        //                      // If idAny below is a class, parameter_value is legal
        //                      // If idAny below is a package, parameter_value is not legal
        //                      // If idAny below is otherwise, not legal
                idAny
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_मानम्_assignment_Class_E
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $3}; }
        |       idCC
        /*mid*/         { /* no nextId as not refing it above this*/ }
        /*cont*/    parameter_मानम्_assignment_Class_E
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

packageClassScopeNoId<ग्रन्थिः_Exprp>:  // IEEE: [package_scope] not followed by yaID
                packageClassScope                       { $$ = $1; }
        ;

packageClassScopeE<ग्रन्थिः_Exprp>:  // IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // TODO: To support classes should return generic type, not packagep
        //                      // class_qualifier := [ yLOCAL '::'  ] [ implicit_class_handle '.'  class_scope ]
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       packageClassScope                       { $$ = $1; }
        ;

packageClassScope<ग्रन्थिः_Exprp>:   // IEEE: class_scope
        //                      // IEEE: "class_type yP_COLONCOLON"
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
                packageClassScopeList                   { $$ = $1; }
        |       local_अग्र_Id yP_COLONCOLON               { $$ = $1; }
        |       dollar_Unit_अग्र_Id yP_COLONCOLON          { $$ = $1; }
        |       dollar_Unit_अग्र_Id yP_COLONCOLON packageClassScopeList
                        { $$ = new AstDot{$2, true, $1, $3}; }
        ;

packageClassScopeList<ग्रन्थिः_Exprp>:   // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      // Or IEEE: [package_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // In this parser <package_identifier>:: and <class_identifier>:: are indistinguishible
        //                      // If you follow the rules down, class_type is really a list via ps_class_identifier
                packageClassScopeItem                   { $$ = $1; }
        |       packageClassScopeList packageClassScopeItem
                        { $$ = new AstDot{$<fl>2, true, $1, $2}; }
        ;

packageClassScopeItem<ग्रन्थिः_Exprp>:   // IEEE: package_scope or [package_scope]::[class_scope]
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // IEEE: class_type: "id [ parameter_value_assignment ]" but allow yaID__aTYPE
        //                      //vv mid rule action needed otherwise we might not have NextId in time to parse the id token
                idCC
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, nullptr}; }
        //
        |       idCC parameter_मानम्_assignment_Class
        /*mid*/         { }
        /*cont*/    yP_COLONCOLON
                        { $$ = new AstClassOrPackageRef{$<fl>1, *$1, nullptr, $2}; }
        ;

dollar_Unit_अग्र_Id<ग्रन्थिः_Exprp>:    // $unit
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yD_UNIT
                        { $$ = new AstClassOrPackageRef{$1, "$unit", nullptr, nullptr}; }
        ;

local_अग्र_Id<ग्रन्थिः_Exprp>:         // local::
        //                      // IMPORTANT: The lexer will parse the following ID to be in the found package
        //                      //     if not needed must use packageClassScopeNoId
        //                      // Must call nextId without any additional tokens following
                yLOCAL__COLONCOLON
                        { $$ = new AstClassOrPackageRef{$1, "local::", nullptr, nullptr}; }
        ;

//^^^=========

class_item_List_अन्तः<nodep>:
                yENDCLASS                               { $$ = nullptr; }
        |       class_itemList yENDCLASS                { $$ = $1; }
        |       error yENDCLASS                         { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       class_itemList error yENDCLASS          { $$ = $1; }  // LCOV_EXCL_LINE
        ;

class_itemList<nodep>:
                class_item                              { $$ = $1; }
        |       class_itemList class_item               { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

class_item<nodep>:                      // ==IEEE: class_item
                class_property                          { $$ = $1; }
        |       class_विधानम्_2                            { $$ = $1; }
        |       class_constraint                        { $$ = $1; }
        //
        |       class_declaration                       { $$ = $1; }
        |       timeunits_declaration                   { $$ = $1; }
        |       covergroup_declaration
                        {
                          यदि ($1) {
                              // Also declares an instance variable, of the covergroup's type
                              AstVar* ध्रुव newp = new AstVar{$1->fileline(), VVarType::VAR,
                                  $1->name(), VFlagChildDType{},
                                  new AstRefDType($1->fileline(), $1->name())};
                              newp->embeddedCovergroup(true);
                              $$ = सङ्कलय_अग्र_Null($1, newp);
                          } अन्यथा {
                              $$ = nullptr;
                          }
                        }
        //                      // local_parameter_declaration under parameter_declaration
        |       parameter_declaration ';'               { $$ = $1; }
        |       ';'                                     { $$ = nullptr; }
        //                      // Verilator specific
        |       vl_Sc_खण्डम्                               { $$ = $1; }
        //
        |       error ';'                               { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

class_विधानम्_2<nodep>:            // ==IEEE: class_method
                memberQualListE task_declaration        { $$ = $2; $1.apply_गन्तव्यम्_Nodes($2); }
        |       memberQualListE function_declaration    { $$ = $2; $1.apply_गन्तव्यम्_Nodes($2); }
        |       y_विशुद्धम् yVIRTUAL__ETC memberQualListE विधानम्_prototype ';'
                        { $$ = $4; $3.apply_गन्तव्यम्_Nodes($4); $4->विशुद्धम्_Virtual(true); $4->isVirtual(true); }
        |       yEXTERN memberQualListE विधानम्_prototype ';'
                        { $$ = $3; $2.apply_गन्तव्यम्_Nodes($3); $3->isExternProto(true); }
        //                      // IEEE: "method_qualifierE class_constructor_declaration"
        //                      // part of function_declaration
        |       yEXTERN memberQualListE class_constructor_prototype
                        { $$ = $3; $2.apply_गन्तव्यम्_Nodes($3); $3->isExternProto(true); }
        ;

dynamic_override_specifiersE<आधारः_Override>:  // IEEE: dynamic_override_specifiers or empty
                /*रिक्तानि*/                             { $$ = VBaseOverride{}; }
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

final_specifierE<आधारः_Override>:  // ==IEEE: final_specifier (similar to dynamic_override_specifiers)
                /*रिक्तानि*/                             { $$ = VBaseOverride{}; }
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
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew constraint_खण्डम्
                        { $$ = $4; $$->isStatic($1); $$->आधारः_Override($3); $$->सङ्कलय_Itemsp($5); }
        //                      // IEEE: constraint_prototype + constraint_prototype_qualifier
        |       constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $4; $$->isStatic($1); $$->आधारः_Override($3);
                          $$->isExternProto(true); }
        |       yEXTERN constraintStaticE yCONSTRAINT dynamic_override_specifiersE constraintIdNew ';'
                        { $$ = $5; $$->isStatic($2); $$->आधारः_Override($4);
                          $$->isExternProto(true); $$->isExternExplicit(true); }
        |       y_विशुद्धम् constraintStaticE yCONSTRAINT constraintIdNew ';'
                        { $$ = $4; $$->is_Kwd_विशुद्धम्($1); $$->isStatic($2); }
        ;

constraintIdNew<constraintp>:  // IEEE: id part of class_constraint
                idAny/*constraint_identifier*/
                        { $$ = new AstConstraint{$<fl>1, *$1, nullptr}; }
        ;

constraint_खण्डम्<nodep>:  // ==IEEE: constraint_block
                '{' '}'                                         { $$ = nullptr; }
        |       '{' constraint_खण्डम्_item_List '}'               { $$ = $2; }
        //
        |       '{' error '}'                                   { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_खण्डम्_item_List error '}'         { $$ = $2; }  // LCOV_EXCL_LINE
        ;

constraint_खण्डम्_item_List<nodep>:  // IEEE: { constraint_block_item }
                constraint_खण्डम्_item                           { $$ = $1; }
        |       constraint_खण्डम्_item_List constraint_खण्डम्_item { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

constraint_खण्डम्_item<nodep>:  // ==IEEE: constraint_block_item
                constraint_expression                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        ;

solve_before_list<ग्रन्थिः_Exprp>:  // ==IEEE: solve_before_list
                constraint_primary                              { $$ = $1; }
        |       solve_before_list ',' constraint_primary        { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

constraint_primary<ग्रन्थिः_Exprp>:  // ==IEEE: constraint_primary
        //                      // exprScope more general than:
        //                      // [ implicit_class_handle '.' | class_scope ] hierarchical_identifier select [ '(' ')' ]
                व्यञ्जकम्_Scope                                       { $$ = $1; }
        ;

constraint_expressionList<nodep>:  // ==IEEE: { constraint_expression }
                constraint_expression                           { $$ = $1; }
        |       ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = new AstConstraintBefore{$1, $2, $4}; }
        |       constraint_expressionList constraint_expression { $$ = सङ्कलय_अग्र_Null($1, $2); }
        |       constraint_expressionList ySOLVE solve_before_list yBEFORE solve_before_list ';'
                        { $$ = सङ्कलय_अग्र_Null($1, new AstConstraintBefore{$<fl>2, $3, $5}); }
        ;

constraint_expression<nodep>:  // ==IEEE: constraint_expression
                व्यञ्जकम्/*expression_or_dist*/ ';'
                        { $$ = new Ast_Constraint_व्यञ्जकम्{$1->fileline(), $1}; }
        //                      // 1800-2012:
        |       ySOFT व्यञ्जकम्/*expression_or_dist*/ ';'
                        { Ast_Constraint_व्यञ्जकम्* ध्रुव newp = new Ast_Constraint_व्यञ्जकम्{$1, $2};
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
        |       व्यञ्जकम् yP_MINUSGT '{' constraint_expressionList '}'
                        { $$ = new AstConstraintIf{$2, $1, $4, nullptr}; }
        |       व्यञ्जकम् yP_MINUSGT yIF '(' व्यञ्जकम् ')' constraint_समुच्चयः %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, nullptr}, nullptr}; }
        |       व्यञ्जकम् yP_MINUSGT yIF '(' व्यञ्जकम् ')' constraint_समुच्चयः yELSE constraint_समुच्चयः
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintIf{$3, $5, $7, $9}, nullptr}; }
        |       व्यञ्जकम् yP_MINUSGT yFOREACH '(' idClassSelForeach ')' constraint_समुच्चयः
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintForeach{$3, $5, $7}, nullptr}; }
        |       व्यञ्जकम् yP_MINUSGT yUNIQUE '{' range_list '}' ';'
                        { $$ = new AstConstraintIf{$2, $1,
                                   new AstConstraintUnique{$3, $5}, nullptr}; }
        |       व्यञ्जकम् yP_MINUSGT ySOFT व्यञ्जकम् ';'
                        { Ast_Constraint_व्यञ्जकम्* ध्रुव innerp = new Ast_Constraint_व्यञ्जकम्{$3, $4};
                          innerp->isSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       व्यञ्जकम् yP_MINUSGT yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_व्यञ्जकम्* ध्रुव innerp = new Ast_Constraint_व्यञ्जकम्{$3, $5};
                          innerp->isDisableSoft(true);
                          $$ = new AstConstraintIf{$2, $1, innerp, nullptr}; }
        |       yIF '(' व्यञ्जकम् ')' constraint_समुच्चयः %prec pr_अधस्तनम्_THAN_ELSE
                        { $$ = new AstConstraintIf{$1, $3, $5, nullptr}; }
        |       yIF '(' व्यञ्जकम् ')' constraint_समुच्चयः yELSE constraint_समुच्चयः
                        { $$ = new AstConstraintIf{$1, $3, $5, $7}; }
        //                      // IEEE says array_identifier here, but dotted accepted in VMM + 1800-2009
        |       yFOREACH '(' idClassSelForeach ')' constraint_समुच्चयः
                        { $$ = new AstConstraintForeach{$1, $3, $5}; }
        //                      // Non-IEEE extension, soft foreach
        |       ySOFT yFOREACH '(' idClassSelForeach ')' constraint_समुच्चयः
                        { $$ = new AstConstraintForeach{$2, $4, $6, true}; }
        //                      // soft is 1800-2012
        |       yDISABLE ySOFT constraint_primary ';'
                        { Ast_Constraint_व्यञ्जकम्* ध्रुव newp = new Ast_Constraint_व्यञ्जकम्{$1, $3};
                          newp->isDisableSoft(true);
                          $$ = newp; }
        //
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

constraint_समुच्चयः<nodep>:  // ==IEEE: constraint_set
                constraint_expression                   { $$ = $1; }
        |       '{' constraint_expressionList '}'       { $$ = $2; }
        //
        |       '{' error '}'                           { $$ = nullptr; }  // LCOV_EXCL_LINE
        |       '{' constraint_expressionList error '}' { $$ = $2; }  // LCOV_EXCL_LINE
        ;

dist_list<distItemp>:  // ==IEEE: dist_list
                dist_item                               { $$ = $1; }
        |       dist_list ',' dist_item                 { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

dist_item<distItemp>:  // ==IEEE: dist_item + dist_weight
                मानम्_range
                        { $$ = new AstDistItem{$1->fileline(), $1, new AstConst{$1->fileline(), 1}}; }
        |       मानम्_range yP_COLONEQ  व्यञ्जकम्
                        { $$ = new AstDistItem{$2, $1, $3}; }
        |       मानम्_range yP_COLONDIV व्यञ्जकम्
                        { $$ = new AstDistItem{$2, $1, $3}; $$->is_सम्पूर्णम्(true); }
        //                      // IEEE 1800-2023 added:
        |       yDEFAULT yP_COLONDIV व्यञ्जकम्
                        { BBUNSUP($<fl>2, "Unsupported: 'default :/' constraint");
                          $$ = nullptr; DEL($3); }
        ;

extern_constraint_declaration<constraintp>:  // ==IEEE: extern_constraint_declaration
                constraintStaticE yCONSTRAINT dynamic_override_specifiersE packageClassScopeE constraintIdNew constraint_खण्डम्
                        { $$ = $5; $$->isStatic($1); $$->isExternDef(true);
                          $$->आधारः_Override($3); $$->classOrPackagep($4); $$->सङ्कलय_Itemsp($6); }
        ;

constraintStaticE<cbool>:  // IEEE: part of extern_constraint_declaration
                /*रिक्तानि*/                             { $$ = false; }
        |       ySTATIC__CONSTRAINT                     { $$ = true; }
        ;

//**********************************************************************
// Constants

time_अङ्कः_Adjusted<ग्रन्थिः_Exprp>:         // Time constant, adjusted to module's time units/precision
                yaTIMENUM
                        { $$ = new AstTimeImport{$<fl>1, new AstConst{$<fl>1, AstConst::RealDouble{}, $1}}; }
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
        /*cont*/    configParameterListE design_statement config_नियमः_statement_List_E
        /*cont*/    yENDCONFIG अन्तः_Label_E
                { AstConfig* ध्रुव newp = new AstConfig{$1, PARSEP->libname(), *$2};
                  newp->सङ्कलय_Designp($5);
                  newp->सङ्कलय_Itemsp($4);
                  newp->सङ्कलय_Itemsp($6);
                  GRAMMARP->अन्तः_Label($<fl>7, *$2, $8);
                  PARSEP->rootp()->सङ्कलय_Miscsp(newp); }
        ;

configParameterListE<nodep>:  // IEEE: { local_parameter_declaration ';' }
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       configParameterList                     { $$ = $1; }
        ;

configParameterList<nodep>:  // IEEE: part of config_declaration
                configParameter                         { $$ = nullptr; DEL($1); }
        |       configParameterList configParameter     { $$ = सङ्कलय_अग्र_Null($1, $2); }
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
        |       configCellList configCell               { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

configCell<configCellp>:  // IEEE: part of design_statement, part of cell_clause
                idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, PARSEP->libname(), *$1}; }
        |       idAny/*library_identifier*/ '.' idAny/*cell_identifier*/
                        { $$ = new AstConfigCell{$<fl>1, *$1, *$3}; }
        ;

config_नियमः_statement_List_E<nodep>:  // IEEE: { config_rule }
                /*रिक्तानि*/                             { $$ = nullptr; }
        |       config_नियमः_statement_List               { $$ = $1; }
        ;

config_नियमः_statement_List<nodep>:  // IEEE: { config_rule }
                config_नियमः_statement                   { $$ = $1; }
        |       config_नियमः_statement_List config_नियमः_statement   { $$ = सङ्कलय_अग्र_Null($1, $2); }
        ;

config_नियमः_statement<nodep>:  // == IEEE: config_rule_statement
        //                      // IEEE: default_clause
                yDEFAULT liblist_clause ';'
                        { $$ = new Ast_Config_नियमः{$1, nullptr, $2, false}; }
        //                      // IEEE: inst_clause
        |       yINSTANCE inst_name liblist_clause ';'
                        { $$ = new Ast_Config_नियमः{$1, $2, $3, false}; }
        |       yINSTANCE inst_name use_clause ';'
                        { $$ = new Ast_Config_नियमः{$1, $2, $3, false}; }
        //                      // IEEE: cell_clause
        |       yCELL configCell liblist_clause ';'
                        { $$ = new Ast_Config_नियमः{$1, $2, $3, true}; }
        |       yCELL configCell use_clause ';'
                        { $$ = new Ast_Config_नियमः{$1, $2, $3, true}; }
        |       error ';'
                        { $$ = nullptr; }  // LCOV_EXCL_LINE
        ;

inst_name<ग्रन्थिः_Exprp>:  // == IEEE: inst_name
                idAnyAsParseRef/*topmodule_identifier*/
                        { $$ = $1; }
        |       idAnyAsParseRef/*topmodule_identifier*/ inst_nameInstanceList
                        { $$ = new AstDot{$<fl>1, false, $1, $2}; }
        ;

inst_nameInstanceList<ग्रन्थिः_Exprp>:  // IEEE: part of inst_name
                '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = $2; }
        |       inst_nameInstanceList '.' idAnyAsParseRef/*instance_identifier*/
                        { $$ = new AstDot{$<fl>2, false, $1, $3}; }
        ;

liblist_clause<nodep>:  // == IEEE: liblist_clause
                yLIBLIST                                { $$ = nullptr; }
        |       yLIBLIST liblistLibraryList             { $$ = $2; }
        ;

liblistLibraryList<ग्रन्थिः_Exprp>:  // IEEE: part of liblist_clause
                idAnyAsParseRef/*library_identifier*/
                        { $$ = $1; }
        |       liblistLibraryList idAnyAsParseRef/*library_identifier*/
                        { $$ = सङ्कलय_अग्र_Null($1, $2); }
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
                /*रिक्तानि*/                             { $$ = nullptr; }
        //                      // IEEE is missing the '#' '(', but examples need it
        |       '#' '(' ')'                             { $$ = nullptr; }
        |       '#' '(' useAssignmentList ')'           { $$ = $3; }
        ;

useAssignmentList<pinp>:  // IEEE: part of use_clause
                useAssignment                           { $$ = $1; }
        |       useAssignmentList ',' useAssignment     { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;

useAssignment<pinp>:  // IEEE: part of use_clause
        //                      // IEEE: named_parameter_assignment
                '.' idAny/*parameter_identifier*/ '(' ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); }
        |       '.' idAny/*parameter_identifier*/ '(' व्यञ्जकम्_Or_दत्त_Type ')'
                        { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: 'config use' parameter assignment"); DEL($4); }
        ;

colonConfigE<cbool>:  // IEEE: [ ':' yCONFIG]
                /*रिक्तानि*/                             { $$ = false; }
        |       ':' yCONFIG                             { $$ = true; }
        ;

//**********************************************************************
// Config - lib.map
//

library_declaration<nodep>:  // IEEE: library_declaration
                yLIBRARY yaSTRING file_मार्गः_spec_List incdirE ';'
                        { $$ = new AstLibrary{$<fl>1, *$2, $3, $4}; }
        ;
incdirE<ग्रन्थिः_Exprp>:  // IEEE: [ '-' yINCDIR file_path_specList ';']
                /*रिक्तानि*/                             { $$ = nullptr; }
                                // https://accellera.mantishub.io/view.php?id=1166
        |       yINCDIR file_मार्गः_spec_List              { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config incdir"); }
        ;
include_statement<nodep>:  // IEEE: include_statement
                yINCLUDE file_मार्गः_spec ';'             { $$ = nullptr; BBUNSUP($<fl>1, "Unsupported: config include"); }
        ;
file_मार्गः_spec_List<ग्रन्थिः_Exprp>:  // IEEE: file_path_spec { ',' file_path_spec }
                file_मार्गः_spec                          { $$ = $1; }
        |       file_मार्गः_spec_List ',' file_मार्गः_spec   { $$ = सङ्कलय_अग्र_Null($1, $3); }
        ;
file_मार्गः_spec<ग्रन्थिः_Exprp>:  // IEEE: file_path_spec
                yaSTRING { $$ = new AstParseRef{$<fl>1, *$1}; }
        ;

//**********************************************************************
// VLT Files

vltItem:
        //                      // TODO support arbitrary order of arguments
                vltOffFront
                        { V3Control::सङ्कलय_Ignore($1, false, "*", 0, 0); }
        |       vltOffFront vltDFile
                        { V3Control::सङ्कलय_Ignore($1, false, *$2, 0, 0); }
        |       vltOffFront vltDFile y_VLT_D_पङ्क्तयः yaINTNUM
                        { V3Control::सङ्कलय_Ignore($1, false, *$2, $4->गन्तव्यम्_UInt(), $4->गन्तव्यम्_UInt()); }
        |       vltOffFront vltDFile y_VLT_D_पङ्क्तयः yaINTNUM '-' yaINTNUM
                        { V3Control::सङ्कलय_Ignore($1, false, *$2, $4->गन्तव्यम्_UInt(), $6->गन्तव्यम्_UInt()); }
        |       vltOffFront vltDFile vltDMatch
                        { यदि (($1 == V3_Error_संहिता::I_COVERAGE) || ($1 == V3_Error_संहिता::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Ignore_मेलनम्($1, *$2, "", *$3);
                          }}
        |       vltOffFront vltDFile vltDContents
                        { यदि (($1 == V3_Error_संहिता::I_COVERAGE) || ($1 == V3_Error_संहिता::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Ignore_मेलनम्($1, *$2, *$3, "*");
                          }}
        |       vltOffFront vltDFile vltDContents vltDMatch
                        { यदि (($1 == V3_Error_संहिता::I_COVERAGE) || ($1 == V3_Error_संहिता::I_TRACING)) {
                              $<fl>1->v3error("Argument -match only supported for lint_off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Ignore_मेलनम्($1, *$2, *$3, *$4);
                          }}
        |       vltOffFront vltDScope
                        { यदि ($1 != V3_Error_संहिता::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Scope_Trace_On(false, *$2, 0);
                          }}
        |       vltOffFront vltDScope vltDLevels
                        { यदि ($1 != V3_Error_संहिता::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Scope_Trace_On(false, *$2, $3->गन्तव्यम्_UInt());
                          }}
        |       vltOnFront
                        { V3Control::सङ्कलय_Ignore($1, true, "*", 0, 0); }
        |       vltOnFront vltDFile
                        { V3Control::सङ्कलय_Ignore($1, true, *$2, 0, 0); }
        |       vltOnFront vltDFile y_VLT_D_पङ्क्तयः yaINTNUM
                        { V3Control::सङ्कलय_Ignore($1, true, *$2, $4->गन्तव्यम्_UInt(), $4->गन्तव्यम्_UInt()); }
        |       vltOnFront vltDFile y_VLT_D_पङ्क्तयः yaINTNUM '-' yaINTNUM
                        { V3Control::सङ्कलय_Ignore($1, true, *$2, $4->गन्तव्यम्_UInt(), $6->गन्तव्यम्_UInt()); }
        |       vltOnFront vltDScope
                        { यदि ($1 != V3_Error_संहिता::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Scope_Trace_On(true, *$2, 0);
                          }}
        |       vltOnFront vltDScope vltDLevels
                        { यदि ($1 != V3_Error_संहिता::I_TRACING) {
                              $<fl>1->v3error("Argument -scope only supported for tracing_on/off_off");
                          } अन्यथा {
                              V3Control::सङ्कलय_Scope_Trace_On(true, *$2, $3->गन्तव्यम्_UInt());
                          }}
        |       vltVarAttrFront vltDModuleE vltDFTaskE vltVarAttrSpecE attr_event_controlE
                        { V3Control::सङ्कलय_Var_Attr($<fl>1, *$2, *$3, GRAMMARP->m_vlt_Var_Spec_विधा, *$4, $1, $5); }
        |       vltVarAttrFrontDeprecated vltDModuleE vltDFTaskE vltVarAttrSpecE
                        { /* Historical, now has no effect */ }
        |       vltInlineFront vltDModuleE vltDFTaskE
                        { V3Control::सङ्कलय_Inline($<fl>1, *$2, *$3, $1); }
        |       y_VLT_COVERAGE_खण्डम्_OFF vltDFile
                        { V3Control::सङ्कलय_Coverage_खण्डम्_Off(*$2, 0); }
        |       y_VLT_COVERAGE_खण्डम्_OFF vltDFile y_VLT_D_पङ्क्तयः yaINTNUM
                        { V3Control::सङ्कलय_Coverage_खण्डम्_Off(*$2, $4->गन्तव्यम्_UInt()); }
        |       y_VLT_COVERAGE_खण्डम्_OFF vltDModule vltDBlock
                        { V3Control::सङ्कलय_Coverage_खण्डम्_Off(*$2, *$3); }
        |       y_VLT_पूर्णम्_CASE vltDFile
                        { V3Control::सङ्कलय_Case_पूर्णम्(*$2, 0); }
        |       y_VLT_पूर्णम्_CASE vltDFile y_VLT_D_पङ्क्तयः yaINTNUM
                        { V3Control::सङ्कलय_Case_पूर्णम्(*$2, $4->गन्तव्यम्_UInt()); }
        |       y_VLT_HIER_खण्डम् vltDModuleE
                        { V3Control::सङ्कलय_Module_Pragma(*$2, VPragmaType::HIER_खण्डम्); }
        |       yVLT_HIER_PARAMS vltDModuleE
                        { V3Control::सङ्कलय_Module_Pragma(*$2, VPragmaType::HIER_PARAMS); }
        |       yVLT_HIER_WORKERS vltDModuleE vltDWorkers
                        { V3Control::सङ्कलय_Hier_Workers($<fl>1, *$2, $3->गन्तव्यम्_SInt()); }
        |       yVLT_HIER_WORKERS vltDHierDpi vltDWorkers
                        { V3Control::सङ्कलय_Hier_Workers($<fl>1, *$2, $3->गन्तव्यम्_SInt()); }
        |       yVLT_PARALLEL_CASE vltDFile
                        { V3Control::सङ्कलय_Case_Parallel(*$2, 0); }
        |       yVLT_PARALLEL_CASE vltDFile y_VLT_D_पङ्क्तयः yaINTNUM
                        { V3Control::सङ्कलय_Case_Parallel(*$2, $4->गन्तव्यम्_UInt()); }
        |       y_VLT_PROFILE_दत्त vltDHierDpi vltDCost
                        { V3Control::सङ्कलय_Profile_दत्त($<fl>1, *$2, $3->गन्तव्यम्_UQuad()); }
        |       y_VLT_PROFILE_दत्त vltDModel vltDMtask vltDCost
                        { V3Control::सङ्कलय_Profile_दत्त($<fl>1, *$2, *$3, $4->गन्तव्यम्_UQuad()); }
        |       y_VLT_FSM_पञ्जिकाकोशः_WRAPPER vltDModule vltDFsmD vltDFsmQ vltDFsmClock vltDFsmResetE vlt_DFsm_Reset_मानम्_E
                        { V3Control::सङ्कलय_Fsm_पञ्जिकाकोशः_Wrapper($<fl>1, *$2, *$3, *$4, *$5, *$6, *$7); }
        |       yVLT_VERILATOR_LIB vltDModule
                        { V3Control::सङ्कलय_Module_Pragma(*$2, VPragmaType::VERILATOR_LIB); }
        ;

vltOffFront<errcodeen>:
                yVLT_COVERAGE_OFF                       { $$ = V3_Error_संहिता::I_COVERAGE; }
        |       yVLT_TIMING_OFF                         { $$ = V3_Error_संहिता::I_TIMING; }
        |       yVLT_TRACING_OFF                        { $$ = V3_Error_संहिता::I_TRACING; }
        |       yVLT_LINT_OFF                           { $$ = V3_Error_संहिता::I_LINT; }
        |       yVLT_LINT_OFF y_VLT_D_नियमः idAny
                        { $$ = V3_Error_संहिता{*$3};
                          यदि ($$ == V3_Error_संहिता::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltOnFront<errcodeen>:
                yVLT_COVERAGE_ON                        { $$ = V3_Error_संहिता::I_COVERAGE; }
        |       yVLT_TIMING_ON                          { $$ = V3_Error_संहिता::I_TIMING; }
        |       yVLT_TRACING_ON                         { $$ = V3_Error_संहिता::I_TRACING; }
        |       yVLT_LINT_ON                            { $$ = V3_Error_संहिता::I_LINT; }
        |       yVLT_LINT_ON y_VLT_D_नियमः idAny
                        { $$ = V3_Error_संहिता{*$3};
                          यदि ($$ == V3_Error_संहिता::EC_ERROR) $1->v3error("Unknown error code: '" << *$3 << "'"); }
        ;

vltDBlock<strp>:  // --block <arg>
                y_VLT_D_खण्डम् str                        { $$ = $2; }
        ;

vltDContents<strp>:
                yVLT_D_CONTENTS str                     { $$ = $2; }
        ;

vltDCost<nump>:  // --cost <arg>
                yVLT_D_COST yaINTNUM                    { $$ = $2; }
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
                /*रिक्तानि*/                             { स्थिर string रिक्तानि; $$ = &रिक्तानि; }
        |       yVLT_D_RESET str                        { $$ = $2; }
        ;

vlt_DFsm_Reset_मानम्_E<strp>:  // [--reset_value <arg>]
                /*रिक्तानि*/                             { स्थिर string रिक्तानि; $$ = &रिक्तानि; }
        |       y_VLT_D_RESET_मानम् str                  { $$ = $2; }
        ;

vltDHierDpi<strp>:  // --hier-dpi <arg>
                yVLT_D_HIER_DPI str                     { $$ = $2; }
        ;

vltDLevels<nump>:  // --levels <arg>
                y_VLT_D_स्तराः yaINTNUM                  { $$ = $2; }
        ;

vltDMatch<strp>:  // --match <arg>
                y_VLT_D_मेलनम् str                        { $$ = $2; }
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
                /*रिक्तानि*/
                        { स्थिर string unit = "$unit"; $$ = &unit; }  // .vlt uses prettyName
        |       vltDModule
                        { $$ = $1; }
        ;

vltDScope<strp>:  // --scope <arg>
                yVLT_D_SCOPE str                        { $$ = $2; }
        ;

vltDFTaskE<strp>:
                /*रिक्तानि*/                             { स्थिर string रिक्तानि; $$ = &रिक्तानि; }
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
                /*रिक्तानि*/
                        { GRAMMARP->m_vlt_Var_Spec_विधा = V3Control::Var_Spec_विधा::VAR; स्थिर std::string रिक्तानि; $$ = &रिक्तानि; }
        |       yVLT_D_PARAM str
                        { GRAMMARP->m_vlt_Var_Spec_विधा = V3Control::Var_Spec_विधा::PARAM; $$ = $2; }
        |       y_VLT_D_पत्तनम् str
                        { GRAMMARP->m_vlt_Var_Spec_विधा = V3Control::Var_Spec_विधा::पत्तनम्; $$ = $2; }
        |       yVLT_D_VAR str
                        { GRAMMARP->m_vlt_Var_Spec_विधा = V3Control::Var_Spec_विधा::VAR; $$ = $2; }
        ;

vltVarAttrFront<attrtypeen>:
                yVLT_FORCEABLE              { $$ = VAttrType::VAR_FORCEABLE; }
        |       y_VLT_लौकिकम्                 { $$ = VAttrType::VAR_लौकिकम्; v3Global.dpi(true); }
        |       y_VLT_लौकिकम्_FLAT            { $$ = VAttrType::VAR_लौकिकम्_FLAT; v3Global.dpi(true); }
        |       y_VLT_लौकिकम्_FLAT_RD         { $$ = VAttrType::VAR_लौकिकम्_FLAT_RD; v3Global.dpi(true); }
        |       y_VLT_लौकिकम्_FLAT_RW         { $$ = VAttrType::VAR_लौकिकम्_FLAT_RW; v3Global.dpi(true); }
        |       yVLT_SC_BIGUINT             { $$ = VAttrType::VAR_SC_BIGUINT; }
        |       yVLT_SC_BV                  { $$ = VAttrType::VAR_SC_BV; }
        |       yVLT_SFORMAT                { $$ = VAttrType::VAR_SFORMAT; }
        |       y_VLT_विभज_VAR              { $$ = VAttrType::VAR_विभज_VAR; }
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
