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
%option ڈھیر
%option noc++
%option اگیتر="V3PreLex"
%{
#جے_تعریف_شدہ NEVER_JUST_FOR_CLANG_FORMAT
 }
#جے_انت

#ضمن "V3PreProc.h"
#ضمن "V3PreLex.h"
#جے_تعریف_شدہ _WIN32
# include <io.h> // for isatty
#جے_انت

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#تعریف_کرو LEXP V3PreLex::s_currentLexp

#تعریف_کرو YY_داخلہ(buf, نتیجہ, max_ناپ_2) \
    افعل { نتیجہ = LEXP->داخلہ_منزل_Lex(buf, max_ناپ_2); } طالما (false)

// Accessors, because flex keeps changing the type of yyleng
حرف* yyourtext() { أرجع yytext; }
ناپ_t yyourleng() { أرجع yyleng; }
فراغ yyourtext(ثابت حرف* textp, ناپ_t ناپ) {
    yytext = (حرف*)textp;
    yyleng = ناپ;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#تعریف_کرو FL_FWDC (LEXP->curFilelinep()->forwardToken(yytext, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#تعریف_کرو FL_BRK (LEXP->curFilelinep()->شروع_Token())

ساكن فراغ linenoInc() { LEXP->linenoInc(); }
ساكن bool pedantic() { أرجع LEXP->m_pedantic; }
ساكن فراغ yyerror(ثابت حرف* پیغام) { LEXP->curFilelinep()->v3error(پیغام); }
ساكن فراغ yyerrorf(ثابت حرف* پیغام) { LEXP->curFilelinep()->v3error(پیغام); }
ساكن فراغ append_Def_قدر(ثابت حرف* t, ناپ_t l) { LEXP->append_Def_قدر(t, l); }

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
%x اظہاریہ_2
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
لفظ            [a-zA-Z0-9_]+
drop            [\032]
bom             [\357\273\277]


        /**************************************************************/
%%
        /* Passthru, to support dev-coverage of some main verilog.l rules */
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_WHITE; }
<PASSTHRU>{لفظ}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; أرجع VP_متن; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->سطر_Directive(yytext);
                                                  أرجع VP_سطر; }
<INITIAL>"`define"      { FL_FWDC; أرجع VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; أرجع VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; أرجع VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; أرجع VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; أرجع VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; أرجع VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; أرجع VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_ڈالو_state(LIBMODE); أرجع VP_متن; }
<INITIAL>"library"      { FL_FWDC; yy_ڈالو_state(LIBMODE); أرجع VP_متن; }
<INITIAL>"`undef"       { FL_FWDC; أرجع VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; أرجع VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; إذا (!pedantic()) أرجع VP_ERROR; وإلا أرجع VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          صحيح res;
                          حرف enctype[16]; // long enough to hold "quote-printable"
                          إذا (LEXP->m_prot_بائٹاں > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = لڑی_توں_پڑھو(yytext + std::لڑی_لمبائی("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_prot_لمائی, &LEXP->m_prot_بائٹاں);
                          إذا (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          إذا (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          إذا (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          إذا (res == 3) {
                              إذا ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_prot_لمائی & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } وإلا {
                              // default values
                              LEXP->m_prot_بائٹاں = 0;
                              LEXP->m_prot_لمائی = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          أرجع VP_متن;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          أرجع VP_متن; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          أرجع VP_متن; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); أرجع VP_متن; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          أرجع VP_متن; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          أرجع VP_متن; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          أرجع VP_متن; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; أرجع VP_متن; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); أرجع VP_متن; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          أرجع VP_متن; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          إذا ((yyourleng()-1) <= ناپ_t(LEXP->m_prot_لمائی) && ((yyleng & 3) == 1)) {
                              LEXP->m_prot_بائٹاں -= (yyleng-1)/4*3;
                          } وإلا {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          إذا (yytext[yyleng-3] == '=')
                              LEXP->m_prot_بائٹاں++;
                          إذا (yytext[yyleng-2] == '=')
                              LEXP->m_prot_بائٹاں++;
                          إذا (LEXP->m_prot_بائٹاں == 0) {
                              BEGIN(INITIAL);
                          } وإلا إذا (LEXP->m_prot_بائٹاں < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          إذا (LEXP->m_prot_بائٹاں != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          أرجع VP_متن; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          إذا (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          أرجع VP_متن; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          إذا (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          أرجع VP_متن; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); أرجع VP_متن;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          أرجع VP_متن; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          أرجع VP_متن; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          أرجع VP_متن; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          ساكن string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; yytext = (حرف*)s_rtnfile.c_str(); yyleng = s_rtnfile.لمائی();
                          أرجع VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          ساكن حرف s_buf[25];
                          (فراغ)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->آخری_Lineno());
                          yytext = s_buf; yyleng = std::لڑی_لمبائی(yytext);
                          أرجع VP_متن; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_ڈالو_state(STRMODE); yymore(); }
<STRMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; أرجع VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{لفظ}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_نکالو_state();
                          إذا (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_قدر(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } وإلا أرجع VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_ڈالو_state(QQQMODE); yymore(); }
<QQQMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; أرجع VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{لفظ}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_نکالو_state();
                          إذا (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_قدر(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } وإلا أرجع VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_ڈالو_state(STRIFY); أرجع VP_STRIFY; }
<STRIFY><<EOF>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; أرجع VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; أرجع VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_ڈالو_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_نکالو_state(); أرجع VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; أرجع VP_علامت; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; أرجع VP_علامت_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; أرجع VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; أرجع VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; أرجع VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); yytext = (حرف*)"\n"; yyleng = 1; أرجع VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; أرجع VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; أرجع VP_متن; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_ڈالو_state(PRTMODE); yymore(); }
<PRTMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; أرجع VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); أرجع VP_متن; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_نکالو_state(); أرجع VP_متن; }

        /* Pass-through include <> filenames */
<INCMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; أرجع VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_نکالو_state(); أرجع VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; أرجع VP_WHITE; }
<LIBMODE>{quote}        { yy_ڈالو_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_ڈالو_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_نکالو_state(); أرجع VP_متن; }
<LIBMODE>.              { FL_FWDC; أرجع VP_متن; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_قدر("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_نکالو_state(); unput('\n'); yyleng=0; أرجع VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<EOF>>        { FL_FWDC; yy_نکالو_state(); أرجع VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_نکالو_state(); unput(yytext[yyleng-1]); yyleng=0; أرجع VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_قدر(yytext, yyleng); yyleng=0;
                          إذا ((--LEXP->m_formalLevel)==0) { yy_نکالو_state(); أرجع VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_ڈالو_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; أرجع VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<EOF>>        { FL_FWDC; linenoInc(); yy_نکالو_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; أرجع VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_قدر((حرف*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_قدر((حرف*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_ڈالو_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_ڈالو_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_قدر(yytext, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_قدر(yytext, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_ڈالو_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_قدر((حرف*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_نکالو_state(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<EOF>>         { FL_FWDC; linenoInc(); yy_نکالو_state(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_نکالو_state(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_قدر((حرف*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_ڈالو_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_ڈالو_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_نکالو_state(); append_Def_قدر(yytext, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_قدر(yytext, yyleng-2); append_Def_قدر((حرف*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); إذا (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{لفظ}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<EOF>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; أرجع VP_EOF_ERROR; }

        /* Preprocessor expression */
<اظہاریہ_2><<EOF>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; أرجع VP_EOF_ERROR; }
<اظہاریہ_2>"/*"              { yy_ڈالو_state(CMTMODE); yymore(); }
<اظہاریہ_2>"//"[^\n\r]*      { FL_FWDC; أرجع VP_COMMENT;}
<اظہاریہ_2>"("               { FL_FWDC; أرجع VP_متن; }  /* V3PreProc will push another EXPR state to stack */
<اظہاریہ_2>")"               { FL_FWDC; yy_نکالو_state(); أرجع VP_متن; }
<اظہاریہ_2>"!"               { FL_FWDC; أرجع VP_متن; }
<اظہاریہ_2>"&&"              { FL_FWDC; أرجع VP_متن; }
<اظہاریہ_2>"||"              { FL_FWDC; أرجع VP_متن; }
<اظہاریہ_2>"->"              { FL_FWDC; أرجع VP_متن; }
<اظہاریہ_2>"<->"             { FL_FWDC; أرجع VP_متن; }
<اظہاریہ_2>{symb}            { FL_FWDC; أرجع VP_علامت; }
<اظہاریہ_2>{crnl}            { FL_FWDC; linenoInc(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_WHITE; }
<اظہاریہ_2>{wsn}+            { FL_FWDC; أرجع VP_WHITE; }
<اظہاریہ_2>.                 { FL_FWDC; أرجع VP_متن; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_ڈالو_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; أرجع VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; أرجع VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_WHITE; }
<ARGMODE>{quote}        { yy_ڈالو_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_ڈالو_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_ڈالو_state(STRIFY); أرجع VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_قدر(yytext, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_قدر(yytext, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          إذا (LEXP->m_parenLevel == 1) {  // Starting (
                              إذا (!VString::isWhitespace(LEXP->m_def_قدر)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          إذا (LEXP->m_parenLevel>1) {
                              append_Def_قدر(yytext, yyleng); FL_BRK;
                          } وإلا {
                              أرجع VP_متن;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          إذا (LEXP->m_parenLevel>0) {
                              append_Def_قدر(yytext, yyleng); FL_BRK;
                          } وإلا {
                              yy_نکالو_state(); أرجع VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; إذا (LEXP->m_parenLevel>1) {
                              append_Def_قدر(yytext, yyleng); FL_BRK;
                          } وإلا {
                              yy_نکالو_state(); أرجع VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_قدر(yytext, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); أرجع VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); أرجع VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); أرجع VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); أرجع VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); أرجع VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); أرجع VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); أرجع VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); أرجع VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_WHITE; }
<INITIAL>"//"           { yy_ڈالو_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_نکالو_state(); أرجع VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_ڈالو_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_نکالو_state(); أرجع VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; أرجع VP_EOF_ERROR; }
<CMTMODE>{لفظ}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; أرجع VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; أرجع VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); yytext=(حرف*)"\n"; yyleng=1; أرجع VP_WHITE; }
<INITIAL><<EOF>>        { FL_FWDC; أرجع VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; أرجع VP_علامت; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; أرجع VP_علامت_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; أرجع VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; أرجع VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; أرجع VP_متن; }
%%
// clang-format on

فراغ V3PreLex::ڈالو_State_Def_Arg(صحيح level) {
    // Enter define substitution argument state
    yy_ڈالو_state(ARGMODE);
    m_parenLevel = level;
    m_def_قدر = "";
}

فراغ V3PreLex::ڈالو_State_Def_Form() {
    // Enter define formal arguments state
    yy_ڈالو_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_قدر = "";
}

فراغ V3PreLex::ڈالو_State_Def_قدر() {
    // Enter define value state
    yy_ڈالو_state(DEFVAL);
    m_parenLevel = 0;
    m_def_قدر = "";
}

فراغ V3PreLex::ڈالو_State_اظہاریہ() {
    // Enter preprocessor expression state
    yy_ڈالو_state(اظہاریہ_2);
}

فراغ V3PreLex::ڈالو_State_Inc_Filename() {
    // Enter include <> filename state
    yy_ڈالو_state(INCMODE);
    yymore();
}

فراغ V3PreLex::ڈالو_State_Passthru() {
    yy_ڈالو_state(PASSTHRU);
    yymore();
}

فراغ V3PreLex::سیٹ_YYDebug(bool on) {
    yy_flex_debug = static_cast<صحيح>(on); }

VPre_دھارا* V3PreLex::new_دھارا(File_سطر* fl, V3PreLex* lexp) {
    VPre_دھارا* ثابت streamp = new VPre_دھارا{fl, lexp};
    إذا (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->ڈالو_State_Passthru();
    أرجع streamp;
}

صحيح V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    أرجع yylex();
}

ناپ_t V3PreLex::داخلہ_منزل_Lex(حرف* buf, ناپ_t max_ناپ_2) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPre_دھارا* streamp = curStreamp();
    إذا (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_ناپ_2 << " bs=" << streamp->m_buffers.ناپ()
             << endl;
        dump_ڈھیر();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    ناپ_t got = 0;
    // Get from this stream
    طالما (got < max_ناپ_2  // Haven't got enough
           && !streamp->m_buffers.خالی_تھاواں()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.نکالو_front();
        ناپ_t len = front.لمائی();
        إذا (len > (max_ناپ_2 - got)) {  // Front string too big
            len = (max_ناپ_2 - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.ڈالو_front(remainder);  // Put back remainder for next time
        }
        strncpy(buf + got, front.c_str(), len);
        got += len;
    }
    إذا (!got) {  // end of stream; try "above" file
        bool again = false;
        string force_باہرلا = سرا_دا_دھارا(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        إذا (force_باہرلا != "") {
            إذا (force_باہرلا.لمائی() > max_ناپ_2) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } وإلا {
                got = force_باہرلا.لمائی();
                strncpy(buf, force_باہرلا.c_str(), got);
            }
        } وإلا {
            إذا (streamp->m_eof) {
                إذا (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            إذا (again) اذهب again;
        }
    }
    إذا (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    أرجع got;
}

string V3PreLex::سرا_دا_دھارا(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    إذا (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_رکن_State << " at " << curFilelinep() << endl;
    }
    إذا (curStreamp()->m_eof) أرجع "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    إذا (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_ڈھیر.نکالو();  // Must work as size>1; EOF is entry 0
        againr = true;
        أرجع "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    وإلا إذا (!curStreamp()->m_رکن_State) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_رکن_State = 1;
        أرجع "\n";  // Exit old file
    } وإلا إذا (curStreamp()->m_رکن_State == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_رکن_State = 2;
        أرجع "";  // End of file
    } وإلا إذا (curStreamp()->m_رکن_State == 2) {
        // Now ending `line
        curStreamp()->m_رکن_State = 3;
        أرجع curFilelinep()->سطر_Directive_Strg(0);  // THe "2" exit is below
    } وإلا {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_رکن_State = 0;
        File_سطر* ثابت filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_ڈھیر.نکالو();  // Must work as size>1; EOF is entry 0
        إذا (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        إذا (curStreamp()->m_eof) {
            أرجع "";
        } وإلا {
            أرجع curFilelinep()->سطر_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

فراغ V3PreLex::init_پہلا_بفر(File_سطر* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPre_دھارا* ثابت streamp = new_دھارا(filelinep, this);
    streamp->m_eof = true;
    m_streamp_ڈھیر.ڈالو(streamp);
    //
    m_بفر_State = yy_create_بفر(nullptr, YY_BUF_ناپ);
    yy_switch_منزل_بفر(m_بفر_State);
    yyrestart(nullptr);
}

فراغ V3PreLex::جانچ_New_File(File_سطر* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    إذا (دھارا_ڈونگھائی() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } وإلا {
        VPre_دھارا* ثابت streamp = new_دھارا(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        جانچ_Switch_دھارا(streamp);
    }
}

فراغ V3PreLex::جانچ_بائٹاں(ثابت string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    إذا (دھارا_ڈونگھائی() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } وإلا {
        VPre_دھارا* ثابت streamp = new_دھارا(curFilelinep(), this);
        streamp->m_buffers.ڈالو_front(str);
        جانچ_Switch_دھارا(streamp);
    }
}

فراغ V3PreLex::جانچ_Switch_دھارا(VPre_دھارا* streamp) {
    curStreamp()->m_buffers.ڈالو_front(currentUnreadChars());
    m_streamp_ڈھیر.ڈالو(streamp);
    yyrestart(nullptr);
}

فراغ V3PreLex::جانچ_بائٹاں_پٹھ(ثابت string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    إذا (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.ڈالو_پٹھ(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t بایاں = (yy_n_chars - (yy_c_buf_p - current_بفر()->yy_ch_buf));
    إذا (بایاں > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        أرجع std::string(yy_c_buf_p, بایاں);  // () narrowing conversion
    } وإلا {
        أرجع "";
    }
}

YY_بفر_STATE V3PreLex::current_بفر() {
    أرجع YY_CURRENT_بفر;
}

صحيح V3PreLex::current_شروع_State() ثابت {
    أرجع YY_شروع;
}

فراغ V3PreLex::سطر_Directive(ثابت حرف* textp) {
    curFilelinep()->سطر_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::جوڑو_Src_Depend(curFilelinep()->filename());
}

فراغ V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

فراغ V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvt_منزل_Hex(current_بفر());
#جے_تعریف_شدہ FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t بایاں = (yy_n_chars - (yy_c_buf_p - current_بفر()->yy_ch_buf));
    cout << " left=" << std::dec << بایاں;
#جے_انت
    cout << endl;
}  // LCOV_EXCL_STOP

فراغ V3PreLex::dump_ڈھیر() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::ڈھیر<VPre_دھارا*> tmpstack = LEXP->m_streamp_ڈھیر;
    طالما (!tmpstack.خالی_تھاواں()) {
        ثابت VPre_دھارا* ثابت streamp = tmpstack.چوٹی();
        cout << "-    bufferStack[" << cvt_منزل_Hex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.ناپ()
             << " size0=" << (streamp->m_buffers.خالی_تھاواں() ? 0 : streamp->m_buffers.front().لمائی())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.نکالو();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(ثابت string& in) {
    string نتیجہ = in;
    string::ناپ_type pos;
    طالما ((pos = نتیجہ.لبھو('\n')) != string::npos) نتیجہ.replace(pos, 1, "\\n");
    طالما ((pos = نتیجہ.لبھو('\r')) != string::npos) نتیجہ.replace(pos, 1, "\\r");
    أرجع نتیجہ;
}

فراغ V3PreLex::بن_ورتیا() {
    إذا (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_چوٹی_state();
        yyerror((حرف*)"");
    }  // LCOV_EXCL_STOP
}

فراغ V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.ڈالو_پٹھ(*curFilelinep());
}
فراغ V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    إذا (m_lexLintState.خالی_تھاواں()) أرجع;
    curFilelinep()->warnStateFrom(m_lexLintState.پٹھ());
    m_lexLintState.نکالو_پٹھ();
}
فراغ V3PreLex::verilatorCmtLint(ثابت حرف* textp, bool turnOff) {
    ثابت حرف* sp = textp;
    طالما (*sp && std::خلا_اے(*sp)) ++sp;
    طالما (*sp && !std::خلا_اے(*sp)) ++sp;  // "verilator"
    طالما (*sp && std::خلا_اے(*sp)) ++sp;
    طالما (*sp && !std::خلا_اے(*sp)) ++sp;  // "lint_on/lint_off"
    طالما (*sp && std::خلا_اے(*sp)) ++sp;
    string پیغام = sp;
    لكل (خودکار pos = پیغام.begin(); pos != پیغام.سرا(); ++pos) {
        إذا (std::خلا_اے(*pos) || *pos == '*') {
            پیغام.erase(pos, پیغام.سرا());
            اكسر;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(پیغام, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
