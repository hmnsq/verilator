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
%option దొంతర
%option noc++
%option ఉపసర్గ="V3PreLex"
%{
#నిర్వచితమైతే NEVER_JUST_FOR_CLANG_FORMAT
 }
#అయితే_ముగింపు

#అన్తర్భావ "V3PreProc.h"
#అన్తర్భావ "V3PreLex.h"
#నిర్వచితమైతే _WIN32
# include <io.h> // for isatty
#అయితే_ముగింపు

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#నిర్వచించు LEXP V3PreLex::s_currentLexp

#నిర్వచించు YY_ఆదానం(buf, ఫలితం, max_కొలత_2) \
    చేయి { ఫలితం = LEXP->ఆదానం_గమ్యం_Lex(buf, max_కొలత_2); } అయితే (false)

// Accessors, because flex keeps changing the type of yyleng
అక్షరం* yyourtext() { తిరిగి yytext; }
కొలత_t yyourleng() { తిరిగి yyleng; }
శూన్య yyourtext(స్థిరరాశి అక్షరం* textp, కొలత_t కొలత) {
    yytext = (అక్షరం*)textp;
    yyleng = కొలత;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#నిర్వచించు FL_FWDC (LEXP->curFilelinep()->forwardToken(yytext, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#నిర్వచించు FL_BRK (LEXP->curFilelinep()->ప్రారంభం_Token())

స్థిర శూన్య linenoInc() { LEXP->linenoInc(); }
స్థిర bool pedantic() { తిరిగి LEXP->m_pedantic; }
స్థిర శూన్య yyerror(స్థిరరాశి అక్షరం* సందేశం_2) { LEXP->curFilelinep()->v3error(సందేశం_2); }
స్థిర శూన్య yyerrorf(స్థిరరాశి అక్షరం* సందేశం_2) { LEXP->curFilelinep()->v3error(సందేశం_2); }
స్థిర శూన్య append_Def_విలువ(స్థిరరాశి అక్షరం* t, కొలత_t l) { LEXP->append_Def_విలువ(t, l); }

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
%x సమాసం_2
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
మాట            [a-zA-Z0-9_]+
drop            [\032]
bom             [\357\273\277]


        /**************************************************************/
%%
        /* Passthru, to support dev-coverage of some main verilog.l rules */
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_WHITE; }
<PASSTHRU>{మాట}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; తిరిగి VP_పాఠ్యం; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->పంక్తి_Directive(yytext);
                                                  తిరిగి VP_పంక్తి; }
<INITIAL>"`define"      { FL_FWDC; తిరిగి VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; తిరిగి VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; తిరిగి VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; తిరిగి VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; తిరిగి VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; తిరిగి VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; తిరిగి VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_నెట్టు_state(LIBMODE); తిరిగి VP_పాఠ్యం; }
<INITIAL>"library"      { FL_FWDC; yy_నెట్టు_state(LIBMODE); తిరిగి VP_పాఠ్యం; }
<INITIAL>"`undef"       { FL_FWDC; తిరిగి VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; తిరిగి VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; ఒకవేళ (!pedantic()) తిరిగి VP_ERROR; లేకపోతే తిరిగి VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          పూర్ణాంకం res;
                          అక్షరం enctype[16]; // long enough to hold "quote-printable"
                          ఒకవేళ (LEXP->m_prot_బైట్లు > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = అక్షరశ్రేణి_చదువు(yytext + std::అక్షరశ్రేణి_పొడవు("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_prot_నిడివి, &LEXP->m_prot_బైట్లు);
                          ఒకవేళ (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          ఒకవేళ (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          ఒకవేళ (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          ఒకవేళ (res == 3) {
                              ఒకవేళ ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_prot_నిడివి & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } లేకపోతే {
                              // default values
                              LEXP->m_prot_బైట్లు = 0;
                              LEXP->m_prot_నిడివి = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          తిరిగి VP_పాఠ్యం; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          తిరిగి VP_పాఠ్యం; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); తిరిగి VP_పాఠ్యం; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; తిరిగి VP_పాఠ్యం; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); తిరిగి VP_పాఠ్యం; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          తిరిగి VP_పాఠ్యం; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          ఒకవేళ ((yyourleng()-1) <= కొలత_t(LEXP->m_prot_నిడివి) && ((yyleng & 3) == 1)) {
                              LEXP->m_prot_బైట్లు -= (yyleng-1)/4*3;
                          } లేకపోతే {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          ఒకవేళ (yytext[yyleng-3] == '=')
                              LEXP->m_prot_బైట్లు++;
                          ఒకవేళ (yytext[yyleng-2] == '=')
                              LEXP->m_prot_బైట్లు++;
                          ఒకవేళ (LEXP->m_prot_బైట్లు == 0) {
                              BEGIN(INITIAL);
                          } లేకపోతే ఒకవేళ (LEXP->m_prot_బైట్లు < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          ఒకవేళ (LEXP->m_prot_బైట్లు != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          ఒకవేళ (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          తిరిగి VP_పాఠ్యం; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          ఒకవేళ (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          తిరిగి VP_పాఠ్యం; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); తిరిగి VP_పాఠ్యం;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          తిరిగి VP_పాఠ్యం; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          తిరిగి VP_పాఠ్యం; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          స్థిర string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; yytext = (అక్షరం*)s_rtnfile.c_str(); yyleng = s_rtnfile.నిడివి();
                          తిరిగి VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          స్థిర అక్షరం s_buf[25];
                          (శూన్య)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->చివరి_Lineno());
                          yytext = s_buf; yyleng = std::అక్షరశ్రేణి_పొడవు(yytext);
                          తిరిగి VP_పాఠ్యం; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_నెట్టు_state(STRMODE); yymore(); }
<STRMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; తిరిగి VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{మాట}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_తీయి_state();
                          ఒకవేళ (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_విలువ(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } లేకపోతే తిరిగి VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_నెట్టు_state(QQQMODE); yymore(); }
<QQQMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; తిరిగి VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{మాట}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_తీయి_state();
                          ఒకవేళ (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_విలువ(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } లేకపోతే తిరిగి VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_నెట్టు_state(STRIFY); తిరిగి VP_STRIFY; }
<STRIFY><<EOF>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; తిరిగి VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; తిరిగి VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_నెట్టు_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_తీయి_state(); తిరిగి VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; తిరిగి VP_చిహ్నం; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; తిరిగి VP_చిహ్నం_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; తిరిగి VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; తిరిగి VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; తిరిగి VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); yytext = (అక్షరం*)"\n"; yyleng = 1; తిరిగి VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; తిరిగి VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; తిరిగి VP_పాఠ్యం; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_నెట్టు_state(PRTMODE); yymore(); }
<PRTMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; తిరిగి VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); తిరిగి VP_పాఠ్యం; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_తీయి_state(); తిరిగి VP_పాఠ్యం; }

        /* Pass-through include <> filenames */
<INCMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; తిరిగి VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_తీయి_state(); తిరిగి VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; తిరిగి VP_WHITE; }
<LIBMODE>{quote}        { yy_నెట్టు_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_నెట్టు_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_తీయి_state(); తిరిగి VP_పాఠ్యం; }
<LIBMODE>.              { FL_FWDC; తిరిగి VP_పాఠ్యం; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_విలువ("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_తీయి_state(); unput('\n'); yyleng=0; తిరిగి VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<EOF>>        { FL_FWDC; yy_తీయి_state(); తిరిగి VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_తీయి_state(); unput(yytext[yyleng-1]); yyleng=0; తిరిగి VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_విలువ(yytext, yyleng); yyleng=0;
                          ఒకవేళ ((--LEXP->m_formalLevel)==0) { yy_తీయి_state(); తిరిగి VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_నెట్టు_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; తిరిగి VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<EOF>>        { FL_FWDC; linenoInc(); yy_తీయి_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; తిరిగి VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_విలువ((అక్షరం*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_విలువ((అక్షరం*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_నెట్టు_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_నెట్టు_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_విలువ(yytext, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_విలువ(yytext, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_నెట్టు_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_విలువ((అక్షరం*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_తీయి_state(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<EOF>>         { FL_FWDC; linenoInc(); yy_తీయి_state(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_తీయి_state(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_విలువ((అక్షరం*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_నెట్టు_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_నెట్టు_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_తీయి_state(); append_Def_విలువ(yytext, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_విలువ(yytext, yyleng-2); append_Def_విలువ((అక్షరం*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); ఒకవేళ (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{మాట}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<EOF>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; తిరిగి VP_EOF_ERROR; }

        /* Preprocessor expression */
<సమాసం_2><<EOF>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; తిరిగి VP_EOF_ERROR; }
<సమాసం_2>"/*"              { yy_నెట్టు_state(CMTMODE); yymore(); }
<సమాసం_2>"//"[^\n\r]*      { FL_FWDC; తిరిగి VP_COMMENT;}
<సమాసం_2>"("               { FL_FWDC; తిరిగి VP_పాఠ్యం; }  /* V3PreProc will push another EXPR state to stack */
<సమాసం_2>")"               { FL_FWDC; yy_తీయి_state(); తిరిగి VP_పాఠ్యం; }
<సమాసం_2>"!"               { FL_FWDC; తిరిగి VP_పాఠ్యం; }
<సమాసం_2>"&&"              { FL_FWDC; తిరిగి VP_పాఠ్యం; }
<సమాసం_2>"||"              { FL_FWDC; తిరిగి VP_పాఠ్యం; }
<సమాసం_2>"->"              { FL_FWDC; తిరిగి VP_పాఠ్యం; }
<సమాసం_2>"<->"             { FL_FWDC; తిరిగి VP_పాఠ్యం; }
<సమాసం_2>{symb}            { FL_FWDC; తిరిగి VP_చిహ్నం; }
<సమాసం_2>{crnl}            { FL_FWDC; linenoInc(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_WHITE; }
<సమాసం_2>{wsn}+            { FL_FWDC; తిరిగి VP_WHITE; }
<సమాసం_2>.                 { FL_FWDC; తిరిగి VP_పాఠ్యం; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_నెట్టు_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; తిరిగి VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; తిరిగి VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_WHITE; }
<ARGMODE>{quote}        { yy_నెట్టు_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_నెట్టు_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_నెట్టు_state(STRIFY); తిరిగి VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_విలువ(yytext, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_విలువ(yytext, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          ఒకవేళ (LEXP->m_parenLevel == 1) {  // Starting (
                              ఒకవేళ (!VString::isWhitespace(LEXP->m_def_విలువ)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          ఒకవేళ (LEXP->m_parenLevel>1) {
                              append_Def_విలువ(yytext, yyleng); FL_BRK;
                          } లేకపోతే {
                              తిరిగి VP_పాఠ్యం;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          ఒకవేళ (LEXP->m_parenLevel>0) {
                              append_Def_విలువ(yytext, yyleng); FL_BRK;
                          } లేకపోతే {
                              yy_తీయి_state(); తిరిగి VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; ఒకవేళ (LEXP->m_parenLevel>1) {
                              append_Def_విలువ(yytext, yyleng); FL_BRK;
                          } లేకపోతే {
                              yy_తీయి_state(); తిరిగి VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_విలువ(yytext, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); తిరిగి VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); తిరిగి VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); తిరిగి VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); తిరిగి VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); తిరిగి VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); తిరిగి VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); తిరిగి VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); తిరిగి VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_WHITE; }
<INITIAL>"//"           { yy_నెట్టు_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_తీయి_state(); తిరిగి VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_నెట్టు_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_తీయి_state(); తిరిగి VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; తిరిగి VP_EOF_ERROR; }
<CMTMODE>{మాట}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; తిరిగి VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; తిరిగి VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); yytext=(అక్షరం*)"\n"; yyleng=1; తిరిగి VP_WHITE; }
<INITIAL><<EOF>>        { FL_FWDC; తిరిగి VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; తిరిగి VP_చిహ్నం; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; తిరిగి VP_చిహ్నం_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; తిరిగి VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; తిరిగి VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; తిరిగి VP_పాఠ్యం; }
%%
// clang-format on

శూన్య V3PreLex::నెట్టు_State_Def_Arg(పూర్ణాంకం level) {
    // Enter define substitution argument state
    yy_నెట్టు_state(ARGMODE);
    m_parenLevel = level;
    m_def_విలువ = "";
}

శూన్య V3PreLex::నెట్టు_State_Def_Form() {
    // Enter define formal arguments state
    yy_నెట్టు_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_విలువ = "";
}

శూన్య V3PreLex::నెట్టు_State_Def_విలువ() {
    // Enter define value state
    yy_నెట్టు_state(DEFVAL);
    m_parenLevel = 0;
    m_def_విలువ = "";
}

శూన్య V3PreLex::నెట్టు_State_సమాసం() {
    // Enter preprocessor expression state
    yy_నెట్టు_state(సమాసం_2);
}

శూన్య V3PreLex::నెట్టు_State_Inc_Filename() {
    // Enter include <> filename state
    yy_నెట్టు_state(INCMODE);
    yymore();
}

శూన్య V3PreLex::నెట్టు_State_Passthru() {
    yy_నెట్టు_state(PASSTHRU);
    yymore();
}

శూన్య V3PreLex::సమితి_YYDebug(bool on) {
    yy_flex_debug = static_cast<పూర్ణాంకం>(on); }

VPre_ప్రవాహం* V3PreLex::new_ప్రవాహం(File_పంక్తి* fl, V3PreLex* lexp) {
    VPre_ప్రవాహం* స్థిరరాశి streamp = new VPre_ప్రవాహం{fl, lexp};
    ఒకవేళ (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->నెట్టు_State_Passthru();
    తిరిగి streamp;
}

పూర్ణాంకం V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    తిరిగి yylex();
}

కొలత_t V3PreLex::ఆదానం_గమ్యం_Lex(అక్షరం* buf, కొలత_t max_కొలత_2) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPre_ప్రవాహం* streamp = curStreamp();
    ఒకవేళ (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_కొలత_2 << " bs=" << streamp->m_buffers.కొలత()
             << endl;
        dump_దొంతర();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    కొలత_t got = 0;
    // Get from this stream
    అయితే (got < max_కొలత_2  // Haven't got enough
           && !streamp->m_buffers.ఖాళీలు()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.తీయి_front();
        కొలత_t len = front.నిడివి();
        ఒకవేళ (len > (max_కొలత_2 - got)) {  // Front string too big
            len = (max_కొలత_2 - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.నెట్టు_front(remainder);  // Put back remainder for next time
        }
        strncpy(buf + got, front.c_str(), len);
        got += len;
    }
    ఒకవేళ (!got) {  // end of stream; try "above" file
        bool again = false;
        string force_బహిర్గతం = చివర_యొక్క_ప్రవాహం(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        ఒకవేళ (force_బహిర్గతం != "") {
            ఒకవేళ (force_బహిర్గతం.నిడివి() > max_కొలత_2) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } లేకపోతే {
                got = force_బహిర్గతం.నిడివి();
                strncpy(buf, force_బహిర్గతం.c_str(), got);
            }
        } లేకపోతే {
            ఒకవేళ (streamp->m_eof) {
                ఒకవేళ (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            ఒకవేళ (again) వెళ్ళు again;
        }
    }
    ఒకవేళ (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    తిరిగి got;
}

string V3PreLex::చివర_యొక్క_ప్రవాహం(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    ఒకవేళ (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_పదం_State << " at " << curFilelinep() << endl;
    }
    ఒకవేళ (curStreamp()->m_eof) తిరిగి "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    ఒకవేళ (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_దొంతర.తీయి();  // Must work as size>1; EOF is entry 0
        againr = true;
        తిరిగి "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    లేకపోతే ఒకవేళ (!curStreamp()->m_పదం_State) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_పదం_State = 1;
        తిరిగి "\n";  // Exit old file
    } లేకపోతే ఒకవేళ (curStreamp()->m_పదం_State == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_పదం_State = 2;
        తిరిగి "";  // End of file
    } లేకపోతే ఒకవేళ (curStreamp()->m_పదం_State == 2) {
        // Now ending `line
        curStreamp()->m_పదం_State = 3;
        తిరిగి curFilelinep()->పంక్తి_Directive_Strg(0);  // THe "2" exit is below
    } లేకపోతే {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_పదం_State = 0;
        File_పంక్తి* స్థిరరాశి filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_దొంతర.తీయి();  // Must work as size>1; EOF is entry 0
        ఒకవేళ (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        ఒకవేళ (curStreamp()->m_eof) {
            తిరిగి "";
        } లేకపోతే {
            తిరిగి curFilelinep()->పంక్తి_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

శూన్య V3PreLex::init_మొదటి_బఫరు(File_పంక్తి* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPre_ప్రవాహం* స్థిరరాశి streamp = new_ప్రవాహం(filelinep, this);
    streamp->m_eof = true;
    m_streamp_దొంతర.నెట్టు(streamp);
    //
    m_బఫరు_State = yy_create_బఫరు(nullptr, YY_BUF_కొలత);
    yy_switch_గమ్యం_బఫరు(m_బఫరు_State);
    yyrestart(nullptr);
}

శూన్య V3PreLex::పరిశీలన_New_File(File_పంక్తి* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    ఒకవేళ (ప్రవాహం_లోతు() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } లేకపోతే {
        VPre_ప్రవాహం* స్థిరరాశి streamp = new_ప్రవాహం(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        పరిశీలన_Switch_ప్రవాహం(streamp);
    }
}

శూన్య V3PreLex::పరిశీలన_బైట్లు(స్థిరరాశి string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    ఒకవేళ (ప్రవాహం_లోతు() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } లేకపోతే {
        VPre_ప్రవాహం* స్థిరరాశి streamp = new_ప్రవాహం(curFilelinep(), this);
        streamp->m_buffers.నెట్టు_front(str);
        పరిశీలన_Switch_ప్రవాహం(streamp);
    }
}

శూన్య V3PreLex::పరిశీలన_Switch_ప్రవాహం(VPre_ప్రవాహం* streamp) {
    curStreamp()->m_buffers.నెట్టు_front(currentUnreadChars());
    m_streamp_దొంతర.నెట్టు(streamp);
    yyrestart(nullptr);
}

శూన్య V3PreLex::పరిశీలన_బైట్లు_వెనుక(స్థిరరాశి string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    ఒకవేళ (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.నెట్టు_వెనుక(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t ఎడమ_2 = (yy_n_chars - (yy_c_buf_p - current_బఫరు()->yy_ch_buf));
    ఒకవేళ (ఎడమ_2 > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        తిరిగి std::string(yy_c_buf_p, ఎడమ_2);  // () narrowing conversion
    } లేకపోతే {
        తిరిగి "";
    }
}

YY_బఫరు_STATE V3PreLex::current_బఫరు() {
    తిరిగి YY_CURRENT_బఫరు;
}

పూర్ణాంకం V3PreLex::current_ప్రారంభం_State() స్థిరరాశి {
    తిరిగి YY_ప్రారంభం;
}

శూన్య V3PreLex::పంక్తి_Directive(స్థిరరాశి అక్షరం* textp) {
    curFilelinep()->పంక్తి_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::కూడు_Src_Depend(curFilelinep()->filename());
}

శూన్య V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

శూన్య V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvt_గమ్యం_Hex(current_బఫరు());
#నిర్వచితమైతే FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t ఎడమ_2 = (yy_n_chars - (yy_c_buf_p - current_బఫరు()->yy_ch_buf));
    cout << " left=" << std::dec << ఎడమ_2;
#అయితే_ముగింపు
    cout << endl;
}  // LCOV_EXCL_STOP

శూన్య V3PreLex::dump_దొంతర() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::దొంతర<VPre_ప్రవాహం*> tmpstack = LEXP->m_streamp_దొంతర;
    అయితే (!tmpstack.ఖాళీలు()) {
        స్థిరరాశి VPre_ప్రవాహం* స్థిరరాశి streamp = tmpstack.శిఖరం();
        cout << "-    bufferStack[" << cvt_గమ్యం_Hex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.కొలత()
             << " size0=" << (streamp->m_buffers.ఖాళీలు() ? 0 : streamp->m_buffers.front().నిడివి())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.తీయి();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(స్థిరరాశి string& in) {
    string ఫలితం = in;
    string::కొలత_type pos;
    అయితే ((pos = ఫలితం.కనుగొను('\n')) != string::npos) ఫలితం.replace(pos, 1, "\\n");
    అయితే ((pos = ఫలితం.కనుగొను('\r')) != string::npos) ఫలితం.replace(pos, 1, "\\r");
    తిరిగి ఫలితం;
}

శూన్య V3PreLex::వాడని() {
    ఒకవేళ (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_శిఖరం_state();
        yyerror((అక్షరం*)"");
    }  // LCOV_EXCL_STOP
}

శూన్య V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.నెట్టు_వెనుక(*curFilelinep());
}
శూన్య V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    ఒకవేళ (m_lexLintState.ఖాళీలు()) తిరిగి;
    curFilelinep()->warnStateFrom(m_lexLintState.వెనుక());
    m_lexLintState.తీయి_వెనుక();
}
శూన్య V3PreLex::verilatorCmtLint(స్థిరరాశి అక్షరం* textp, bool turnOff) {
    స్థిరరాశి అక్షరం* sp = textp;
    అయితే (*sp && std::ఖాళీనా(*sp)) ++sp;
    అయితే (*sp && !std::ఖాళీనా(*sp)) ++sp;  // "verilator"
    అయితే (*sp && std::ఖాళీనా(*sp)) ++sp;
    అయితే (*sp && !std::ఖాళీనా(*sp)) ++sp;  // "lint_on/lint_off"
    అయితే (*sp && std::ఖాళీనా(*sp)) ++sp;
    string సందేశం_2 = sp;
    కొరకు (స్వయంచాలక pos = సందేశం_2.begin(); pos != సందేశం_2.చివర(); ++pos) {
        ఒకవేళ (std::ఖాళీనా(*pos) || *pos == '*') {
            సందేశం_2.erase(pos, సందేశం_2.చివర());
            ఆపు;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(సందేశం_2, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
