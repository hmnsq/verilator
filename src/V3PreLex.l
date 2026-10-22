/**************************************************************************
 * DESCRIPTION: Verilator: Flex verilog preprocessor
 *
 * Code available from: https://verilator.org
 *
 **************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of either the GNU Lesser General Public License Version 3
 * or the Perl Artistic License Version 2.0.
 * SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
 * SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
 *
 **************************************************************************
 * Do not use Flex in C++ mode.  It has bugs with yyunput() which result in
 * lost characters.
 **************************************************************************/
/* clang-format off */

%option noyywrap align interactive
%option ರಾಶಿ
%option noc++
%option ಪೂರ್ವಪ್ರತ್ಯಯ="V3PreLex"
%{
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ NEVER_JUST_FOR_CLANG_FORMAT
 }
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "V3PreProc.h"
#ಅನ್ತರ್ಭಾವ "V3PreLex.h"
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ _WIN32
# include <io.h> // for isatty
#ಆದರೆ_ಅಂತ್ಯ

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#ವ್ಯಾಖ್ಯಾನಿಸು LEXP V3PreLex::s_currentLexp

#ವ್ಯಾಖ್ಯಾನಿಸು YY_ಒಳಹರಿವು(buf, ಫಲಿತಾಂಶ, max_ಗಾತ್ರ_2) \
    ಮಾಡು { ಫಲಿತಾಂಶ = LEXP->ಒಳಹರಿವು_ಗಮ್ಯ_Lex(buf, max_ಗಾತ್ರ_2); } ಆಗ (false)

// Accessors, because flex keeps changing the type of yyleng
ಅಕ್ಷರ* yyourtext() { ಹಿಂತಿರುಗಿ yytext; }
ಗಾತ್ರ_t yyourleng() { ಹಿಂತಿರುಗಿ yyleng; }
ಶೂನ್ಯ yyourtext(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಗಾತ್ರ_2) {
    yytext = (ಅಕ್ಷರ*)textp;
    yyleng = ಗಾತ್ರ_2;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#ವ್ಯಾಖ್ಯಾನಿಸು FL_FWDC (LEXP->curFilelinep()->forwardToken(yytext, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#ವ್ಯಾಖ್ಯಾನಿಸು FL_BRK (LEXP->curFilelinep()->ಆರಂಭ_Token())

ಸ್ಥಿರ ಶೂನ್ಯ linenoInc() { LEXP->linenoInc(); }
ಸ್ಥಿರ bool pedantic() { ಹಿಂತಿರುಗಿ LEXP->m_pedantic; }
ಸ್ಥಿರ ಶೂನ್ಯ yyerror(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸಂದೇಶ_2) { LEXP->curFilelinep()->v3error(ಸಂದೇಶ_2); }
ಸ್ಥಿರ ಶೂನ್ಯ yyerrorf(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸಂದೇಶ_2) { LEXP->curFilelinep()->v3error(ಸಂದೇಶ_2); }
ಸ್ಥಿರ ಶೂನ್ಯ append_Def_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* t, ಗಾತ್ರ_t l) { LEXP->append_Def_ಮೌಲ್ಯ(t, l); }

/* clang-format off */
/**********************************************************************/
%}

%x ARGMODE
%x CMTMODE
%x CMTONEM
%x DEFCMT
%x DEFFORM
%x DEFFPAR
%x DEFVAL
%x ENCBASE64
%x ಅಭಿವ್ಯಕ್ತಿ_2
%x INCMODE
%x PASSTHRU
%x PRAGMA
%x PRAGMAERR
%x PRAGMAPRT
%x PRAGMAPRTERR
%x PRTMODE
%x QQQMODE
%x STRIFY
%x STRMODE
%x LIBMODE

/* drop: Drop Ctrl-Z - can't pass thru or may EOF the output too soon */

ws              [ \t\f\r]
wsn             [ \t\f]
crnl            [\r]*[\n]
quote           [\"]
tickquote       [`][\"]
/* Where we use symb/symbdef, we must also look for a `` join */
/* Note in the preprocessor \ESCaped is *not* always special; mantis1537/bug441 */
symb            ([a-zA-Z_][a-zA-Z0-9_$]*|\\[^ \t\f\r\n]+)
symbdef         ([a-zA-Z_][a-zA-Z0-9_$]*|\\[^ \t\f\r\n`]+)
ಶಬ್ದ            [a-zA-Z0-9_]+
drop            [\032]
bom             [\357\273\277]


        /**************************************************************/
%%
        /* Passthru, to support dev-coverage of some main verilog.l rules */
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<PASSTHRU>{ಶಬ್ದ}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->ಸಾಲು_Directive(yytext);
                                                  ಹಿಂತಿರುಗಿ VP_ಸಾಲು; }
<INITIAL>"`define"      { FL_FWDC; ಹಿಂತಿರುಗಿ VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; ಹಿಂತಿರುಗಿ VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; ಹಿಂತಿರುಗಿ VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; ಹಿಂತಿರುಗಿ VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_ತಳ್ಳು_state(LIBMODE); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<INITIAL>"library"      { FL_FWDC; yy_ತಳ್ಳು_state(LIBMODE); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<INITIAL>"`undef"       { FL_FWDC; ಹಿಂತಿರುಗಿ VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; ಹಿಂತಿರುಗಿ VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; ಆದರೆ (!pedantic()) ಹಿಂತಿರುಗಿ VP_ERROR; ಇಲ್ಲದಿದ್ದರೆ ಹಿಂತಿರುಗಿ VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          ಪೂರ್ಣಾಂಕ res;
                          ಅಕ್ಷರ enctype[16]; // long enough to hold "quote-printable"
                          ಆದರೆ (LEXP->m_prot_ಬೈಟುಗಳು > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = ಅಕ್ಷರಸರಣಿ_ಓದು(yytext + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_prot_ಉದ್ದಳತೆ, &LEXP->m_prot_ಬೈಟುಗಳು);
                          ಆದರೆ (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          ಆದರೆ (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          ಆದರೆ (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          ಆದರೆ (res == 3) {
                              ಆದರೆ ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_prot_ಉದ್ದಳತೆ & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } ಇಲ್ಲದಿದ್ದರೆ {
                              // default values
                              LEXP->m_prot_ಬೈಟುಗಳು = 0;
                              LEXP->m_prot_ಉದ್ದಳತೆ = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          ಆದರೆ ((yyourleng()-1) <= ಗಾತ್ರ_t(LEXP->m_prot_ಉದ್ದಳತೆ) && ((yyleng & 3) == 1)) {
                              LEXP->m_prot_ಬೈಟುಗಳು -= (yyleng-1)/4*3;
                          } ಇಲ್ಲದಿದ್ದರೆ {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          ಆದರೆ (yytext[yyleng-3] == '=')
                              LEXP->m_prot_ಬೈಟುಗಳು++;
                          ಆದರೆ (yytext[yyleng-2] == '=')
                              LEXP->m_prot_ಬೈಟುಗಳು++;
                          ಆದರೆ (LEXP->m_prot_ಬೈಟುಗಳು == 0) {
                              BEGIN(INITIAL);
                          } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (LEXP->m_prot_ಬೈಟುಗಳು < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          ಆದರೆ (LEXP->m_prot_ಬೈಟುಗಳು != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          ಆದರೆ (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          ಆದರೆ (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          ಸ್ಥಿರ string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; yytext = (ಅಕ್ಷರ*)s_rtnfile.c_str(); yyleng = s_rtnfile.ಉದ್ದಳತೆ();
                          ಹಿಂತಿರುಗಿ VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          ಸ್ಥಿರ ಅಕ್ಷರ s_buf[25];
                          (ಶೂನ್ಯ)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->ಕೊನೆಯ_Lineno());
                          yytext = s_buf; yyleng = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(yytext);
                          ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_ತಳ್ಳು_state(STRMODE); yymore(); }
<STRMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{ಶಬ್ದ}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_ತೆಗೆ_state();
                          ಆದರೆ (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_ಮೌಲ್ಯ(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } ಇಲ್ಲದಿದ್ದರೆ ಹಿಂತಿರುಗಿ VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_ತಳ್ಳು_state(QQQMODE); yymore(); }
<QQQMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{ಶಬ್ದ}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_ತೆಗೆ_state();
                          ಆದರೆ (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_ಮೌಲ್ಯ(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } ಇಲ್ಲದಿದ್ದರೆ ಹಿಂತಿರುಗಿ VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_ತಳ್ಳು_state(STRIFY); ಹಿಂತಿರುಗಿ VP_STRIFY; }
<STRIFY><<EOF>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; ಹಿಂತಿರುಗಿ VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_ತಳ್ಳು_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಚಿಹ್ನೆ; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; ಹಿಂತಿರುಗಿ VP_ಚಿಹ್ನೆ_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; ಹಿಂತಿರುಗಿ VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; ಹಿಂತಿರುಗಿ VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; ಹಿಂತಿರುಗಿ VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); yytext = (ಅಕ್ಷರ*)"\n"; yyleng = 1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; ಹಿಂತಿರುಗಿ VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_ತಳ್ಳು_state(PRTMODE); yymore(); }
<PRTMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

        /* Pass-through include <> filenames */
<INCMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; ಹಿಂತಿರುಗಿ VP_WHITE; }
<LIBMODE>{quote}        { yy_ತಳ್ಳು_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_ತಳ್ಳು_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<LIBMODE>.              { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_ಮೌಲ್ಯ("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_ತೆಗೆ_state(); unput('\n'); yyleng=0; ಹಿಂತಿರುಗಿ VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<EOF>>        { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_ತೆಗೆ_state(); unput(yytext[yyleng-1]); yyleng=0; ಹಿಂತಿರುಗಿ VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); yyleng=0;
                          ಆದರೆ ((--LEXP->m_formalLevel)==0) { yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_ತಳ್ಳು_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; ಹಿಂತಿರುಗಿ VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<EOF>>        { FL_FWDC; linenoInc(); yy_ತೆಗೆ_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; ಹಿಂತಿರುಗಿ VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_ಮೌಲ್ಯ((ಅಕ್ಷರ*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_ಮೌಲ್ಯ((ಅಕ್ಷರ*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_ತಳ್ಳು_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_ತಳ್ಳು_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_ತಳ್ಳು_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_ಮೌಲ್ಯ((ಅಕ್ಷರ*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_ತೆಗೆ_state(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<EOF>>         { FL_FWDC; linenoInc(); yy_ತೆಗೆ_state(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_ತೆಗೆ_state(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_ಮೌಲ್ಯ((ಅಕ್ಷರ*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_ತಳ್ಳು_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_ತಳ್ಳು_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_ತೆಗೆ_state(); append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_ಮೌಲ್ಯ(yytext, yyleng-2); append_Def_ಮೌಲ್ಯ((ಅಕ್ಷರ*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); ಆದರೆ (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{ಶಬ್ದ}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<EOF>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }

        /* Preprocessor expression */
<ಅಭಿವ್ಯಕ್ತಿ_2><<EOF>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<ಅಭಿವ್ಯಕ್ತಿ_2>"/*"              { yy_ತಳ್ಳು_state(CMTMODE); yymore(); }
<ಅಭಿವ್ಯಕ್ತಿ_2>"//"[^\n\r]*      { FL_FWDC; ಹಿಂತಿರುಗಿ VP_COMMENT;}
<ಅಭಿವ್ಯಕ್ತಿ_2>"("               { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }  /* V3PreProc will push another EXPR state to stack */
<ಅಭಿವ್ಯಕ್ತಿ_2>")"               { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>"!"               { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>"&&"              { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>"||"              { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>"->"              { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>"<->"             { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>{symb}            { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಚಿಹ್ನೆ; }
<ಅಭಿವ್ಯಕ್ತಿ_2>{crnl}            { FL_FWDC; linenoInc(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<ಅಭಿವ್ಯಕ್ತಿ_2>{wsn}+            { FL_FWDC; ಹಿಂತಿರುಗಿ VP_WHITE; }
<ಅಭಿವ್ಯಕ್ತಿ_2>.                 { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_ತಳ್ಳು_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; ಹಿಂತಿರುಗಿ VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<ARGMODE>{quote}        { yy_ತಳ್ಳು_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_ತಳ್ಳು_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_ತಳ್ಳು_state(STRIFY); ಹಿಂತಿರುಗಿ VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          ಆದರೆ (LEXP->m_parenLevel == 1) {  // Starting (
                              ಆದರೆ (!VString::isWhitespace(LEXP->m_def_ಮೌಲ್ಯ)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          ಆದರೆ (LEXP->m_parenLevel>1) {
                              append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK;
                          } ಇಲ್ಲದಿದ್ದರೆ {
                              ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          ಆದರೆ (LEXP->m_parenLevel>0) {
                              append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK;
                          } ಇಲ್ಲದಿದ್ದರೆ {
                              yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; ಆದರೆ (LEXP->m_parenLevel>1) {
                              append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK;
                          } ಇಲ್ಲದಿದ್ದರೆ {
                              yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_ಮೌಲ್ಯ(yytext, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); ಹಿಂತಿರುಗಿ VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<INITIAL>"//"           { yy_ತಳ್ಳು_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_ತಳ್ಳು_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_ತೆಗೆ_state(); ಹಿಂತಿರುಗಿ VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; ಹಿಂತಿರುಗಿ VP_EOF_ERROR; }
<CMTMODE>{ಶಬ್ದ}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; ಹಿಂತಿರುಗಿ VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; ಹಿಂತಿರುಗಿ VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); yytext=(ಅಕ್ಷರ*)"\n"; yyleng=1; ಹಿಂತಿರುಗಿ VP_WHITE; }
<INITIAL><<EOF>>        { FL_FWDC; ಹಿಂತಿರುಗಿ VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಚಿಹ್ನೆ; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; ಹಿಂತಿರುಗಿ VP_ಚಿಹ್ನೆ_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; ಹಿಂತಿರುಗಿ VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; ಹಿಂತಿರುಗಿ VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; ಹಿಂತಿರುಗಿ VP_ಪಠ್ಯ; }
%%
// clang-format on

ಶೂನ್ಯ V3PreLex::ತಳ್ಳು_State_Def_Arg(ಪೂರ್ಣಾಂಕ level) {
    // Enter define substitution argument state
    yy_ತಳ್ಳು_state(ARGMODE);
    m_parenLevel = level;
    m_def_ಮೌಲ್ಯ = "";
}

ಶೂನ್ಯ V3PreLex::ತಳ್ಳು_State_Def_Form() {
    // Enter define formal arguments state
    yy_ತಳ್ಳು_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_ಮೌಲ್ಯ = "";
}

ಶೂನ್ಯ V3PreLex::ತಳ್ಳು_State_Def_ಮೌಲ್ಯ() {
    // Enter define value state
    yy_ತಳ್ಳು_state(DEFVAL);
    m_parenLevel = 0;
    m_def_ಮೌಲ್ಯ = "";
}

ಶೂನ್ಯ V3PreLex::ತಳ್ಳು_State_ಅಭಿವ್ಯಕ್ತಿ() {
    // Enter preprocessor expression state
    yy_ತಳ್ಳು_state(ಅಭಿವ್ಯಕ್ತಿ_2);
}

ಶೂನ್ಯ V3PreLex::ತಳ್ಳು_State_Inc_Filename() {
    // Enter include <> filename state
    yy_ತಳ್ಳು_state(INCMODE);
    yymore();
}

ಶೂನ್ಯ V3PreLex::ತಳ್ಳು_State_Passthru() {
    yy_ತಳ್ಳು_state(PASSTHRU);
    yymore();
}

ಶೂನ್ಯ V3PreLex::ಗಣ_YYDebug(bool on) {
    yy_flex_debug = static_cast<ಪೂರ್ಣಾಂಕ>(on); }

VPre_ಹರಿವು* V3PreLex::new_ಹರಿವು(File_ಸಾಲು* fl, V3PreLex* lexp) {
    VPre_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = new VPre_ಹರಿವು{fl, lexp};
    ಆದರೆ (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->ತಳ್ಳು_State_Passthru();
    ಹಿಂತಿರುಗಿ streamp;
}

ಪೂರ್ಣಾಂಕ V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    ಹಿಂತಿರುಗಿ yylex();
}

ಗಾತ್ರ_t V3PreLex::ಒಳಹರಿವು_ಗಮ್ಯ_Lex(ಅಕ್ಷರ* buf, ಗಾತ್ರ_t max_ಗಾತ್ರ_2) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPre_ಹರಿವು* streamp = curStreamp();
    ಆದರೆ (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_ಗಾತ್ರ_2 << " bs=" << streamp->m_buffers.ಗಾತ್ರ_2()
             << endl;
        dump_ರಾಶಿ();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    ಗಾತ್ರ_t got = 0;
    // Get from this stream
    ಆಗ (got < max_ಗಾತ್ರ_2  // Haven't got enough
           && !streamp->m_buffers.ಖಾಲಿಗಳು()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.ತೆಗೆ_front();
        ಗಾತ್ರ_t len = front.ಉದ್ದಳತೆ();
        ಆದರೆ (len > (max_ಗಾತ್ರ_2 - got)) {  // Front string too big
            len = (max_ಗಾತ್ರ_2 - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.ತಳ್ಳು_front(remainder);  // Put back remainder for next time
        }
        strncpy(buf + got, front.c_str(), len);
        got += len;
    }
    ಆದರೆ (!got) {  // end of stream; try "above" file
        bool again = false;
        string force_ಹೊರಹೋಗು = ಕೊನೆ_ದ_ಹರಿವು(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        ಆದರೆ (force_ಹೊರಹೋಗು != "") {
            ಆದರೆ (force_ಹೊರಹೋಗು.ಉದ್ದಳತೆ() > max_ಗಾತ್ರ_2) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } ಇಲ್ಲದಿದ್ದರೆ {
                got = force_ಹೊರಹೋಗು.ಉದ್ದಳತೆ();
                strncpy(buf, force_ಹೊರಹೋಗು.c_str(), got);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (streamp->m_eof) {
                ಆದರೆ (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            ಆದರೆ (again) ಹೋಗು again;
        }
    }
    ಆದರೆ (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    ಹಿಂತಿರುಗಿ got;
}

string V3PreLex::ಕೊನೆ_ದ_ಹರಿವು(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    ಆದರೆ (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_ಪದ_State << " at " << curFilelinep() << endl;
    }
    ಆದರೆ (curStreamp()->m_eof) ಹಿಂತಿರುಗಿ "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    ಆದರೆ (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_ರಾಶಿ.ತೆಗೆ_2();  // Must work as size>1; EOF is entry 0
        againr = true;
        ಹಿಂತಿರುಗಿ "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!curStreamp()->m_ಪದ_State) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_ಪದ_State = 1;
        ಹಿಂತಿರುಗಿ "\n";  // Exit old file
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (curStreamp()->m_ಪದ_State == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_ಪದ_State = 2;
        ಹಿಂತಿರುಗಿ "";  // End of file
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (curStreamp()->m_ಪದ_State == 2) {
        // Now ending `line
        curStreamp()->m_ಪದ_State = 3;
        ಹಿಂತಿರುಗಿ curFilelinep()->ಸಾಲು_Directive_Strg(0);  // THe "2" exit is below
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_ಪದ_State = 0;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_ರಾಶಿ.ತೆಗೆ_2();  // Must work as size>1; EOF is entry 0
        ಆದರೆ (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        ಆದರೆ (curStreamp()->m_eof) {
            ಹಿಂತಿರುಗಿ "";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ curFilelinep()->ಸಾಲು_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

ಶೂನ್ಯ V3PreLex::init_ಮೊದಲ_ಬಫರ್(File_ಸಾಲು* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPre_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = new_ಹರಿವು(filelinep, this);
    streamp->m_eof = true;
    m_streamp_ರಾಶಿ.ತಳ್ಳು(streamp);
    //
    m_ಬಫರ್_State = yy_create_ಬಫರ್(nullptr, YY_BUF_ಗಾತ್ರ);
    yy_switch_ಗಮ್ಯ_ಬಫರ್(m_ಬಫರ್_State);
    yyrestart(nullptr);
}

ಶೂನ್ಯ V3PreLex::ಪರಿಶೀಲನೆ_New_File(File_ಸಾಲು* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    ಆದರೆ (ಹರಿವು_ಆಳ() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } ಇಲ್ಲದಿದ್ದರೆ {
        VPre_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = new_ಹರಿವು(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        ಪರಿಶೀಲನೆ_Switch_ಹರಿವು(streamp);
    }
}

ಶೂನ್ಯ V3PreLex::ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು(ಸ್ಥಿರಾಂಕ string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    ಆದರೆ (ಹರಿವು_ಆಳ() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } ಇಲ್ಲದಿದ್ದರೆ {
        VPre_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = new_ಹರಿವು(curFilelinep(), this);
        streamp->m_buffers.ತಳ್ಳು_front(str);
        ಪರಿಶೀಲನೆ_Switch_ಹರಿವು(streamp);
    }
}

ಶೂನ್ಯ V3PreLex::ಪರಿಶೀಲನೆ_Switch_ಹರಿವು(VPre_ಹರಿವು* streamp) {
    curStreamp()->m_buffers.ತಳ್ಳು_front(currentUnreadChars());
    m_streamp_ರಾಶಿ.ತಳ್ಳು(streamp);
    yyrestart(nullptr);
}

ಶೂನ್ಯ V3PreLex::ಪರಿಶೀಲನೆ_ಬೈಟುಗಳು_ಹಿಂಭಾಗ(ಸ್ಥಿರಾಂಕ string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    ಆದರೆ (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.ತಳ್ಳು_ಹಿಂಭಾಗ(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t ಎಡ_2 = (yy_n_chars - (yy_c_buf_p - current_ಬಫರ್()->yy_ch_buf));
    ಆದರೆ (ಎಡ_2 > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        ಹಿಂತಿರುಗಿ std::string(yy_c_buf_p, ಎಡ_2);  // () narrowing conversion
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ "";
    }
}

YY_ಬಫರ್_STATE V3PreLex::current_ಬಫರ್() {
    ಹಿಂತಿರುಗಿ YY_CURRENT_ಬಫರ್;
}

ಪೂರ್ಣಾಂಕ V3PreLex::current_ಆರಂಭ_State() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ YY_ಆರಂಭ;
}

ಶೂನ್ಯ V3PreLex::ಸಾಲು_Directive(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) {
    curFilelinep()->ಸಾಲು_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::ಕೂಡಿಸು_Src_Depend(curFilelinep()->filename());
}

ಶೂನ್ಯ V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

ಶೂನ್ಯ V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvt_ಗಮ್ಯ_Hex(current_ಬಫರ್());
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t ಎಡ_2 = (yy_n_chars - (yy_c_buf_p - current_ಬಫರ್()->yy_ch_buf));
    cout << " left=" << std::dec << ಎಡ_2;
#ಆದರೆ_ಅಂತ್ಯ
    cout << endl;
}  // LCOV_EXCL_STOP

ಶೂನ್ಯ V3PreLex::dump_ರಾಶಿ() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::ರಾಶಿ<VPre_ಹರಿವು*> tmpstack = LEXP->m_streamp_ರಾಶಿ;
    ಆಗ (!tmpstack.ಖಾಲಿಗಳು()) {
        ಸ್ಥಿರಾಂಕ VPre_ಹರಿವು* ಸ್ಥಿರಾಂಕ streamp = tmpstack.ತುದಿ();
        cout << "-    bufferStack[" << cvt_ಗಮ್ಯ_Hex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.ಗಾತ್ರ_2()
             << " size0=" << (streamp->m_buffers.ಖಾಲಿಗಳು() ? 0 : streamp->m_buffers.front().ಉದ್ದಳತೆ())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.ತೆಗೆ_2();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(ಸ್ಥಿರಾಂಕ string& in) {
    string ಫಲಿತಾಂಶ = in;
    string::ಗಾತ್ರ_type pos;
    ಆಗ ((pos = ಫಲಿತಾಂಶ.ಕಂಡುಹಿಡಿ('\n')) != string::npos) ಫಲಿತಾಂಶ.replace(pos, 1, "\\n");
    ಆಗ ((pos = ಫಲಿತಾಂಶ.ಕಂಡುಹಿಡಿ('\r')) != string::npos) ಫಲಿತಾಂಶ.replace(pos, 1, "\\r");
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಶೂನ್ಯ V3PreLex::ಬಳಸದ() {
    ಆದರೆ (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_ತುದಿ_state();
        yyerror((ಅಕ್ಷರ*)"");
    }  // LCOV_EXCL_STOP
}

ಶೂನ್ಯ V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.ತಳ್ಳು_ಹಿಂಭಾಗ(*curFilelinep());
}
ಶೂನ್ಯ V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    ಆದರೆ (m_lexLintState.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
    curFilelinep()->warnStateFrom(m_lexLintState.ಹಿಂಭಾಗ());
    m_lexLintState.ತೆಗೆ_ಹಿಂಭಾಗ();
}
ಶೂನ್ಯ V3PreLex::verilatorCmtLint(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, bool turnOff) {
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sp = textp;
    ಆಗ (*sp && std::ಖಾಲಿಯೇ(*sp)) ++sp;
    ಆಗ (*sp && !std::ಖಾಲಿಯೇ(*sp)) ++sp;  // "verilator"
    ಆಗ (*sp && std::ಖಾಲಿಯೇ(*sp)) ++sp;
    ಆಗ (*sp && !std::ಖಾಲಿಯೇ(*sp)) ++sp;  // "lint_on/lint_off"
    ಆಗ (*sp && std::ಖಾಲಿಯೇ(*sp)) ++sp;
    string ಸಂದೇಶ_2 = sp;
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ pos = ಸಂದೇಶ_2.begin(); pos != ಸಂದೇಶ_2.ಕೊನೆ(); ++pos) {
        ಆದರೆ (std::ಖಾಲಿಯೇ(*pos) || *pos == '*') {
            ಸಂದೇಶ_2.erase(pos, ಸಂದೇಶ_2.ಕೊನೆ());
            ನಿಲ್ಲಿಸು;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(ಸಂದೇಶ_2, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
