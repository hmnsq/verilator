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
%option ਢੇਰ
%option noc++
%option ਅਗੇਤਰ="V3PreLex"
%{
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ NEVER_JUST_FOR_CLANG_FORMAT
 }
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreProc.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreLex.h"
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ _WIN32
# include <io.h> // for isatty
#ਜੇ_ਅੰਤ

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ LEXP V3PreLex::s_currentLexp

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ YY_ਨਿਵੇਸ਼(buf, ਨਤੀਜਾ, max_ਮਾਪ_2) \
    ਕਰੋ { ਨਤੀਜਾ = LEXP->ਨਿਵੇਸ਼_ਮੰਜ਼ਿਲ_Lex(buf, max_ਮਾਪ_2); } ਜਦੋਂ (false)

// Accessors, because flex keeps changing the type of yyleng
ਅੱਖਰ* yyourtext() { ਵਾਪਸ yytext; }
ਮਾਪ_t yyourleng() { ਵਾਪਸ yyleng; }
ਖਾਲੀ yyourtext(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਮਾਪ) {
    yytext = (ਅੱਖਰ*)textp;
    yyleng = ਮਾਪ;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FL_FWDC (LEXP->curFilelinep()->forwardToken(yytext, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FL_BRK (LEXP->curFilelinep()->ਸ਼ੁਰੂ_Token())

ਸਥਿਰ ਖਾਲੀ linenoInc() { LEXP->linenoInc(); }
ਸਥਿਰ bool pedantic() { ਵਾਪਸ LEXP->m_pedantic; }
ਸਥਿਰ ਖਾਲੀ yyerror(ਅਚਲ ਅੱਖਰ* ਸੁਨੇਹਾ_2) { LEXP->curFilelinep()->v3error(ਸੁਨੇਹਾ_2); }
ਸਥਿਰ ਖਾਲੀ yyerrorf(ਅਚਲ ਅੱਖਰ* ਸੁਨੇਹਾ_2) { LEXP->curFilelinep()->v3error(ਸੁਨੇਹਾ_2); }
ਸਥਿਰ ਖਾਲੀ append_Def_ਮੁੱਲ(ਅਚਲ ਅੱਖਰ* t, ਮਾਪ_t l) { LEXP->append_Def_ਮੁੱਲ(t, l); }

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
%x ਵਿਅੰਜਕ_2
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
ਸ਼ਬਦ            [a-zA-Z0-9_]+
drop            [\032]
bom             [\357\273\277]


        /**************************************************************/
%%
        /* Passthru, to support dev-coverage of some main verilog.l rules */
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_WHITE; }
<PASSTHRU>{ਸ਼ਬਦ}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->ਸਤਰ_Directive(yytext);
                                                  ਵਾਪਸ VP_ਸਤਰ; }
<INITIAL>"`define"      { FL_FWDC; ਵਾਪਸ VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; ਵਾਪਸ VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; ਵਾਪਸ VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; ਵਾਪਸ VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; ਵਾਪਸ VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; ਵਾਪਸ VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; ਵਾਪਸ VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_ਧੱਕੋ_state(LIBMODE); ਵਾਪਸ VP_ਪਾਠ; }
<INITIAL>"library"      { FL_FWDC; yy_ਧੱਕੋ_state(LIBMODE); ਵਾਪਸ VP_ਪਾਠ; }
<INITIAL>"`undef"       { FL_FWDC; ਵਾਪਸ VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; ਵਾਪਸ VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; ਜੇ (!pedantic()) ਵਾਪਸ VP_ERROR; ਨਹੀਂਤਾਂ ਵਾਪਸ VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          ਪੂਰਨ res;
                          ਅੱਖਰ enctype[16]; // long enough to hold "quote-printable"
                          ਜੇ (LEXP->m_prot_ਬਾਈਟਾਂ > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = ਲੜੀ_ਤੋਂ_ਪੜ੍ਹੋ(yytext + std::ਲੜੀ_ਲੰਬਾਈ("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_prot_ਲੰਮਾਈ, &LEXP->m_prot_ਬਾਈਟਾਂ);
                          ਜੇ (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          ਜੇ (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          ਜੇ (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          ਜੇ (res == 3) {
                              ਜੇ ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_prot_ਲੰਮਾਈ & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } ਨਹੀਂਤਾਂ {
                              // default values
                              LEXP->m_prot_ਬਾਈਟਾਂ = 0;
                              LEXP->m_prot_ਲੰਮਾਈ = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); ਵਾਪਸ VP_ਪਾਠ; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          ਵਾਪਸ VP_ਪਾਠ; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          ਜੇ ((yyourleng()-1) <= ਮਾਪ_t(LEXP->m_prot_ਲੰਮਾਈ) && ((yyleng & 3) == 1)) {
                              LEXP->m_prot_ਬਾਈਟਾਂ -= (yyleng-1)/4*3;
                          } ਨਹੀਂਤਾਂ {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          ਜੇ (yytext[yyleng-3] == '=')
                              LEXP->m_prot_ਬਾਈਟਾਂ++;
                          ਜੇ (yytext[yyleng-2] == '=')
                              LEXP->m_prot_ਬਾਈਟਾਂ++;
                          ਜੇ (LEXP->m_prot_ਬਾਈਟਾਂ == 0) {
                              BEGIN(INITIAL);
                          } ਨਹੀਂਤਾਂ ਜੇ (LEXP->m_prot_ਬਾਈਟਾਂ < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          ਜੇ (LEXP->m_prot_ਬਾਈਟਾਂ != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          ਜੇ (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          ਵਾਪਸ VP_ਪਾਠ; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          ਜੇ (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); ਵਾਪਸ VP_ਪਾਠ;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          ਵਾਪਸ VP_ਪਾਠ; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          ਵਾਪਸ VP_ਪਾਠ; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          ਸਥਿਰ string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; yytext = (ਅੱਖਰ*)s_rtnfile.c_str(); yyleng = s_rtnfile.ਲੰਮਾਈ();
                          ਵਾਪਸ VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          ਸਥਿਰ ਅੱਖਰ s_buf[25];
                          (ਖਾਲੀ)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->ਆਖਰੀ_Lineno());
                          yytext = s_buf; yyleng = std::ਲੜੀ_ਲੰਬਾਈ(yytext);
                          ਵਾਪਸ VP_ਪਾਠ; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_ਧੱਕੋ_state(STRMODE); yymore(); }
<STRMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; ਵਾਪਸ VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{ਸ਼ਬਦ}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_ਕੱਢੋ_state();
                          ਜੇ (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_ਮੁੱਲ(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } ਨਹੀਂਤਾਂ ਵਾਪਸ VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_ਧੱਕੋ_state(QQQMODE); yymore(); }
<QQQMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; ਵਾਪਸ VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{ਸ਼ਬਦ}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_ਕੱਢੋ_state();
                          ਜੇ (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_ਮੁੱਲ(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } ਨਹੀਂਤਾਂ ਵਾਪਸ VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_ਧੱਕੋ_state(STRIFY); ਵਾਪਸ VP_STRIFY; }
<STRIFY><<EOF>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; ਵਾਪਸ VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; ਵਾਪਸ VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_ਧੱਕੋ_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; ਵਾਪਸ VP_ਚਿੰਨ੍ਹ; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; ਵਾਪਸ VP_ਚਿੰਨ੍ਹ_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; ਵਾਪਸ VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; ਵਾਪਸ VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; ਵਾਪਸ VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); yytext = (ਅੱਖਰ*)"\n"; yyleng = 1; ਵਾਪਸ VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; ਵਾਪਸ VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_ਧੱਕੋ_state(PRTMODE); yymore(); }
<PRTMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; ਵਾਪਸ VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); ਵਾਪਸ VP_ਪਾਠ; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_ਪਾਠ; }

        /* Pass-through include <> filenames */
<INCMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; ਵਾਪਸ VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; ਵਾਪਸ VP_WHITE; }
<LIBMODE>{quote}        { yy_ਧੱਕੋ_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_ਧੱਕੋ_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_ਪਾਠ; }
<LIBMODE>.              { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_ਮੁੱਲ("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_ਕੱਢੋ_state(); unput('\n'); yyleng=0; ਵਾਪਸ VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<EOF>>        { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_ਕੱਢੋ_state(); unput(yytext[yyleng-1]); yyleng=0; ਵਾਪਸ VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); yyleng=0;
                          ਜੇ ((--LEXP->m_formalLevel)==0) { yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_ਧੱਕੋ_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; ਵਾਪਸ VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<EOF>>        { FL_FWDC; linenoInc(); yy_ਕੱਢੋ_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; ਵਾਪਸ VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_ਮੁੱਲ((ਅੱਖਰ*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_ਮੁੱਲ((ਅੱਖਰ*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_ਧੱਕੋ_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_ਧੱਕੋ_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_ਧੱਕੋ_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_ਮੁੱਲ((ਅੱਖਰ*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_ਕੱਢੋ_state(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<EOF>>         { FL_FWDC; linenoInc(); yy_ਕੱਢੋ_state(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_ਕੱਢੋ_state(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_ਮੁੱਲ((ਅੱਖਰ*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_ਧੱਕੋ_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_ਧੱਕੋ_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_ਕੱਢੋ_state(); append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_ਮੁੱਲ(yytext, yyleng-2); append_Def_ਮੁੱਲ((ਅੱਖਰ*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); ਜੇ (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{ਸ਼ਬਦ}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<EOF>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; ਵਾਪਸ VP_EOF_ERROR; }

        /* Preprocessor expression */
<ਵਿਅੰਜਕ_2><<EOF>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; ਵਾਪਸ VP_EOF_ERROR; }
<ਵਿਅੰਜਕ_2>"/*"              { yy_ਧੱਕੋ_state(CMTMODE); yymore(); }
<ਵਿਅੰਜਕ_2>"//"[^\n\r]*      { FL_FWDC; ਵਾਪਸ VP_COMMENT;}
<ਵਿਅੰਜਕ_2>"("               { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }  /* V3PreProc will push another EXPR state to stack */
<ਵਿਅੰਜਕ_2>")"               { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_ਪਾਠ; }
<ਵਿਅੰਜਕ_2>"!"               { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
<ਵਿਅੰਜਕ_2>"&&"              { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
<ਵਿਅੰਜਕ_2>"||"              { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
<ਵਿਅੰਜਕ_2>"->"              { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
<ਵਿਅੰਜਕ_2>"<->"             { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
<ਵਿਅੰਜਕ_2>{symb}            { FL_FWDC; ਵਾਪਸ VP_ਚਿੰਨ੍ਹ; }
<ਵਿਅੰਜਕ_2>{crnl}            { FL_FWDC; linenoInc(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_WHITE; }
<ਵਿਅੰਜਕ_2>{wsn}+            { FL_FWDC; ਵਾਪਸ VP_WHITE; }
<ਵਿਅੰਜਕ_2>.                 { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_ਧੱਕੋ_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; ਵਾਪਸ VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; ਵਾਪਸ VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_WHITE; }
<ARGMODE>{quote}        { yy_ਧੱਕੋ_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_ਧੱਕੋ_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_ਧੱਕੋ_state(STRIFY); ਵਾਪਸ VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          ਜੇ (LEXP->m_parenLevel == 1) {  // Starting (
                              ਜੇ (!VString::isWhitespace(LEXP->m_def_ਮੁੱਲ)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          ਜੇ (LEXP->m_parenLevel>1) {
                              append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK;
                          } ਨਹੀਂਤਾਂ {
                              ਵਾਪਸ VP_ਪਾਠ;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          ਜੇ (LEXP->m_parenLevel>0) {
                              append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK;
                          } ਨਹੀਂਤਾਂ {
                              yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; ਜੇ (LEXP->m_parenLevel>1) {
                              append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK;
                          } ਨਹੀਂਤਾਂ {
                              yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_ਮੁੱਲ(yytext, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); ਵਾਪਸ VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); ਵਾਪਸ VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_WHITE; }
<INITIAL>"//"           { yy_ਧੱਕੋ_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_ਧੱਕੋ_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_ਕੱਢੋ_state(); ਵਾਪਸ VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; ਵਾਪਸ VP_EOF_ERROR; }
<CMTMODE>{ਸ਼ਬਦ}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; ਵਾਪਸ VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; ਵਾਪਸ VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); yytext=(ਅੱਖਰ*)"\n"; yyleng=1; ਵਾਪਸ VP_WHITE; }
<INITIAL><<EOF>>        { FL_FWDC; ਵਾਪਸ VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; ਵਾਪਸ VP_ਚਿੰਨ੍ਹ; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; ਵਾਪਸ VP_ਚਿੰਨ੍ਹ_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; ਵਾਪਸ VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; ਵਾਪਸ VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; ਵਾਪਸ VP_ਪਾਠ; }
%%
// clang-format on

ਖਾਲੀ V3PreLex::ਧੱਕੋ_State_Def_Arg(ਪੂਰਨ level) {
    // Enter define substitution argument state
    yy_ਧੱਕੋ_state(ARGMODE);
    m_parenLevel = level;
    m_def_ਮੁੱਲ = "";
}

ਖਾਲੀ V3PreLex::ਧੱਕੋ_State_Def_Form() {
    // Enter define formal arguments state
    yy_ਧੱਕੋ_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_ਮੁੱਲ = "";
}

ਖਾਲੀ V3PreLex::ਧੱਕੋ_State_Def_ਮੁੱਲ() {
    // Enter define value state
    yy_ਧੱਕੋ_state(DEFVAL);
    m_parenLevel = 0;
    m_def_ਮੁੱਲ = "";
}

ਖਾਲੀ V3PreLex::ਧੱਕੋ_State_ਵਿਅੰਜਕ() {
    // Enter preprocessor expression state
    yy_ਧੱਕੋ_state(ਵਿਅੰਜਕ_2);
}

ਖਾਲੀ V3PreLex::ਧੱਕੋ_State_Inc_Filename() {
    // Enter include <> filename state
    yy_ਧੱਕੋ_state(INCMODE);
    yymore();
}

ਖਾਲੀ V3PreLex::ਧੱਕੋ_State_Passthru() {
    yy_ਧੱਕੋ_state(PASSTHRU);
    yymore();
}

ਖਾਲੀ V3PreLex::ਸੈੱਟ_YYDebug(bool on) {
    yy_flex_debug = static_cast<ਪੂਰਨ>(on); }

VPre_ਧਾਰਾ* V3PreLex::new_ਧਾਰਾ(File_ਸਤਰ* fl, V3PreLex* lexp) {
    VPre_ਧਾਰਾ* ਅਚਲ streamp = new VPre_ਧਾਰਾ{fl, lexp};
    ਜੇ (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->ਧੱਕੋ_State_Passthru();
    ਵਾਪਸ streamp;
}

ਪੂਰਨ V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    ਵਾਪਸ yylex();
}

ਮਾਪ_t V3PreLex::ਨਿਵੇਸ਼_ਮੰਜ਼ਿਲ_Lex(ਅੱਖਰ* buf, ਮਾਪ_t max_ਮਾਪ_2) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPre_ਧਾਰਾ* streamp = curStreamp();
    ਜੇ (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_ਮਾਪ_2 << " bs=" << streamp->m_buffers.ਮਾਪ()
             << endl;
        dump_ਢੇਰ();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    ਮਾਪ_t got = 0;
    // Get from this stream
    ਜਦੋਂ (got < max_ਮਾਪ_2  // Haven't got enough
           && !streamp->m_buffers.ਖਾਲੀ_ਥਾਵਾਂ()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.ਕੱਢੋ_front();
        ਮਾਪ_t len = front.ਲੰਮਾਈ();
        ਜੇ (len > (max_ਮਾਪ_2 - got)) {  // Front string too big
            len = (max_ਮਾਪ_2 - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.ਧੱਕੋ_front(remainder);  // Put back remainder for next time
        }
        strncpy(buf + got, front.c_str(), len);
        got += len;
    }
    ਜੇ (!got) {  // end of stream; try "above" file
        bool again = false;
        string force_ਬਾਹਰਲਾ = ਸਿਰਾ_ਦਾ_ਧਾਰਾ(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        ਜੇ (force_ਬਾਹਰਲਾ != "") {
            ਜੇ (force_ਬਾਹਰਲਾ.ਲੰਮਾਈ() > max_ਮਾਪ_2) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } ਨਹੀਂਤਾਂ {
                got = force_ਬਾਹਰਲਾ.ਲੰਮਾਈ();
                strncpy(buf, force_ਬਾਹਰਲਾ.c_str(), got);
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (streamp->m_eof) {
                ਜੇ (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            ਜੇ (again) ਜਾਓ again;
        }
    }
    ਜੇ (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    ਵਾਪਸ got;
}

string V3PreLex::ਸਿਰਾ_ਦਾ_ਧਾਰਾ(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    ਜੇ (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_ਪਦ_State << " at " << curFilelinep() << endl;
    }
    ਜੇ (curStreamp()->m_eof) ਵਾਪਸ "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    ਜੇ (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_ਢੇਰ.ਕੱਢੋ();  // Must work as size>1; EOF is entry 0
        againr = true;
        ਵਾਪਸ "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    ਨਹੀਂਤਾਂ ਜੇ (!curStreamp()->m_ਪਦ_State) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_ਪਦ_State = 1;
        ਵਾਪਸ "\n";  // Exit old file
    } ਨਹੀਂਤਾਂ ਜੇ (curStreamp()->m_ਪਦ_State == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_ਪਦ_State = 2;
        ਵਾਪਸ "";  // End of file
    } ਨਹੀਂਤਾਂ ਜੇ (curStreamp()->m_ਪਦ_State == 2) {
        // Now ending `line
        curStreamp()->m_ਪਦ_State = 3;
        ਵਾਪਸ curFilelinep()->ਸਤਰ_Directive_Strg(0);  // THe "2" exit is below
    } ਨਹੀਂਤਾਂ {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_ਪਦ_State = 0;
        File_ਸਤਰ* ਅਚਲ filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_ਢੇਰ.ਕੱਢੋ();  // Must work as size>1; EOF is entry 0
        ਜੇ (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        ਜੇ (curStreamp()->m_eof) {
            ਵਾਪਸ "";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ curFilelinep()->ਸਤਰ_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

ਖਾਲੀ V3PreLex::init_ਪਹਿਲਾ_ਬਫਰ(File_ਸਤਰ* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPre_ਧਾਰਾ* ਅਚਲ streamp = new_ਧਾਰਾ(filelinep, this);
    streamp->m_eof = true;
    m_streamp_ਢੇਰ.ਧੱਕੋ(streamp);
    //
    m_ਬਫਰ_State = yy_create_ਬਫਰ(nullptr, YY_BUF_ਮਾਪ);
    yy_switch_ਮੰਜ਼ਿਲ_ਬਫਰ(m_ਬਫਰ_State);
    yyrestart(nullptr);
}

ਖਾਲੀ V3PreLex::ਜਾਂਚ_New_File(File_ਸਤਰ* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    ਜੇ (ਧਾਰਾ_ਡੂੰਘਾਈ() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } ਨਹੀਂਤਾਂ {
        VPre_ਧਾਰਾ* ਅਚਲ streamp = new_ਧਾਰਾ(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        ਜਾਂਚ_Switch_ਧਾਰਾ(streamp);
    }
}

ਖਾਲੀ V3PreLex::ਜਾਂਚ_ਬਾਈਟਾਂ(ਅਚਲ string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    ਜੇ (ਧਾਰਾ_ਡੂੰਘਾਈ() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } ਨਹੀਂਤਾਂ {
        VPre_ਧਾਰਾ* ਅਚਲ streamp = new_ਧਾਰਾ(curFilelinep(), this);
        streamp->m_buffers.ਧੱਕੋ_front(str);
        ਜਾਂਚ_Switch_ਧਾਰਾ(streamp);
    }
}

ਖਾਲੀ V3PreLex::ਜਾਂਚ_Switch_ਧਾਰਾ(VPre_ਧਾਰਾ* streamp) {
    curStreamp()->m_buffers.ਧੱਕੋ_front(currentUnreadChars());
    m_streamp_ਢੇਰ.ਧੱਕੋ(streamp);
    yyrestart(nullptr);
}

ਖਾਲੀ V3PreLex::ਜਾਂਚ_ਬਾਈਟਾਂ_ਪਿੱਠ(ਅਚਲ string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    ਜੇ (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.ਧੱਕੋ_ਪਿੱਠ(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t ਖੱਬਾ = (yy_n_chars - (yy_c_buf_p - current_ਬਫਰ()->yy_ch_buf));
    ਜੇ (ਖੱਬਾ > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        ਵਾਪਸ std::string(yy_c_buf_p, ਖੱਬਾ);  // () narrowing conversion
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ "";
    }
}

YY_ਬਫਰ_STATE V3PreLex::current_ਬਫਰ() {
    ਵਾਪਸ YY_CURRENT_ਬਫਰ;
}

ਪੂਰਨ V3PreLex::current_ਸ਼ੁਰੂ_State() ਅਚਲ {
    ਵਾਪਸ YY_ਸ਼ੁਰੂ;
}

ਖਾਲੀ V3PreLex::ਸਤਰ_Directive(ਅਚਲ ਅੱਖਰ* textp) {
    curFilelinep()->ਸਤਰ_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::ਜੋੜੋ_Src_Depend(curFilelinep()->filename());
}

ਖਾਲੀ V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

ਖਾਲੀ V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvt_ਮੰਜ਼ਿਲ_Hex(current_ਬਫਰ());
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t ਖੱਬਾ = (yy_n_chars - (yy_c_buf_p - current_ਬਫਰ()->yy_ch_buf));
    cout << " left=" << std::dec << ਖੱਬਾ;
#ਜੇ_ਅੰਤ
    cout << endl;
}  // LCOV_EXCL_STOP

ਖਾਲੀ V3PreLex::dump_ਢੇਰ() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::ਢੇਰ<VPre_ਧਾਰਾ*> tmpstack = LEXP->m_streamp_ਢੇਰ;
    ਜਦੋਂ (!tmpstack.ਖਾਲੀ_ਥਾਵਾਂ()) {
        ਅਚਲ VPre_ਧਾਰਾ* ਅਚਲ streamp = tmpstack.ਸਿਖਰ();
        cout << "-    bufferStack[" << cvt_ਮੰਜ਼ਿਲ_Hex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.ਮਾਪ()
             << " size0=" << (streamp->m_buffers.ਖਾਲੀ_ਥਾਵਾਂ() ? 0 : streamp->m_buffers.front().ਲੰਮਾਈ())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.ਕੱਢੋ();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(ਅਚਲ string& in) {
    string ਨਤੀਜਾ = in;
    string::ਮਾਪ_type pos;
    ਜਦੋਂ ((pos = ਨਤੀਜਾ.ਲੱਭੋ('\n')) != string::npos) ਨਤੀਜਾ.replace(pos, 1, "\\n");
    ਜਦੋਂ ((pos = ਨਤੀਜਾ.ਲੱਭੋ('\r')) != string::npos) ਨਤੀਜਾ.replace(pos, 1, "\\r");
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਖਾਲੀ V3PreLex::ਅਣਵਰਤਿਆ() {
    ਜੇ (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_ਸਿਖਰ_state();
        yyerror((ਅੱਖਰ*)"");
    }  // LCOV_EXCL_STOP
}

ਖਾਲੀ V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.ਧੱਕੋ_ਪਿੱਠ(*curFilelinep());
}
ਖਾਲੀ V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    ਜੇ (m_lexLintState.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
    curFilelinep()->warnStateFrom(m_lexLintState.ਪਿੱਠ());
    m_lexLintState.ਕੱਢੋ_ਪਿੱਠ();
}
ਖਾਲੀ V3PreLex::verilatorCmtLint(ਅਚਲ ਅੱਖਰ* textp, bool turnOff) {
    ਅਚਲ ਅੱਖਰ* sp = textp;
    ਜਦੋਂ (*sp && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*sp)) ++sp;
    ਜਦੋਂ (*sp && !std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*sp)) ++sp;  // "verilator"
    ਜਦੋਂ (*sp && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*sp)) ++sp;
    ਜਦੋਂ (*sp && !std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*sp)) ++sp;  // "lint_on/lint_off"
    ਜਦੋਂ (*sp && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*sp)) ++sp;
    string ਸੁਨੇਹਾ_2 = sp;
    ਲਈ (ਸਵੈਚਾਲਿਤ pos = ਸੁਨੇਹਾ_2.begin(); pos != ਸੁਨੇਹਾ_2.ਸਿਰਾ(); ++pos) {
        ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*pos) || *pos == '*') {
            ਸੁਨੇਹਾ_2.erase(pos, ਸੁਨੇਹਾ_2.ਸਿਰਾ());
            ਰੋਕੋ;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(ਸੁਨੇਹਾ_2, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
