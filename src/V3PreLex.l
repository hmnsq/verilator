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
%option স্তূপ
%option noc++
%option prefix="V3PreLex"
%{
#ifdef NEVER_JUST_FOR_CLANG_FORMAT
 }
#endif

#অন্তর্ভাব "V3PreProc.h"
#অন্তর্ভাব "V3PreLex.h"
#ifdef _WIN32
# include <io.h> // for isatty
#endif

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#define LEXP V3PreLex::s_currentLexp

#define YY_প্রবেশ(buf, result, max_size) \
    কুরু { result = LEXP->প্রবেশ_To_Lex(buf, max_size); } যাবত্ (false)

// Accessors, because flex keeps changing the type of yyleng
বর্ণ* yyourtext() { প্রত্যাগম yytext; }
size_t yyourleng() { প্রত্যাগম yyleng; }
শূন্য yyourtext(ধ্রুব বর্ণ* textp, size_t size) {
    yytext = (বর্ণ*)textp;
    yyleng = size;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#define FL_FWDC (LEXP->curFilelinep()->forwardToken(yytext, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#define FL_BRK (LEXP->curFilelinep()->startToken())

স্থির শূন্য linenoInc() { LEXP->linenoInc(); }
স্থির bool pedantic() { প্রত্যাগম LEXP->m_pedantic; }
স্থির শূন্য yyerror(ধ্রুব বর্ণ* বার্তা) { LEXP->curFilelinep()->v3error(বার্তা); }
স্থির শূন্য yyerrorf(ধ্রুব বর্ণ* বার্তা) { LEXP->curFilelinep()->v3error(বার্তা); }
স্থির শূন্য append_Def_মান(ধ্রুব বর্ণ* t, size_t l) { LEXP->append_Def_মান(t, l); }

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
%x রাশিমালা_2
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
word            [a-zA-Z0-9_]+
drop            [\032]
bom             [\357\273\277]


        /**************************************************************/
%%
        /* Passthru, to support dev-coverage of some main verilog.l rules */
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_WHITE; }
<PASSTHRU>{word}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; প্রত্যাগম VP_TEXT; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->সারি_Directive(yytext);
                                                  প্রত্যাগম VP_সারি; }
<INITIAL>"`define"      { FL_FWDC; প্রত্যাগম VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; প্রত্যাগম VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; প্রত্যাগম VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; প্রত্যাগম VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; প্রত্যাগম VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; প্রত্যাগম VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; প্রত্যাগম VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_ঠেলো_state(LIBMODE); প্রত্যাগম VP_TEXT; }
<INITIAL>"library"      { FL_FWDC; yy_ঠেলো_state(LIBMODE); প্রত্যাগম VP_TEXT; }
<INITIAL>"`undef"       { FL_FWDC; প্রত্যাগম VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; প্রত্যাগম VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; যদি (!pedantic()) প্রত্যাগম VP_ERROR; অন্যথা প্রত্যাগম VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          পূর্ণাঙ্ক res;
                          বর্ণ enctype[16]; // long enough to hold "quote-printable"
                          যদি (LEXP->m_protBytes > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = sscanf(yytext + std::strlen("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_protLength, &LEXP->m_protBytes);
                          যদি (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          যদি (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          যদি (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          যদি (res == 3) {
                              যদি ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_protLength & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } অন্যথা {
                              // default values
                              LEXP->m_protBytes = 0;
                              LEXP->m_protLength = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          প্রত্যাগম VP_TEXT; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          প্রত্যাগম VP_TEXT; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); প্রত্যাগম VP_TEXT; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; প্রত্যাগম VP_TEXT; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); প্রত্যাগম VP_TEXT; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          প্রত্যাগম VP_TEXT; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          যদি ((yyourleng()-1) <= size_t(LEXP->m_protLength) && ((yyleng & 3) == 1)) {
                              LEXP->m_protBytes -= (yyleng-1)/4*3;
                          } অন্যথা {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          যদি (yytext[yyleng-3] == '=')
                              LEXP->m_protBytes++;
                          যদি (yytext[yyleng-2] == '=')
                              LEXP->m_protBytes++;
                          যদি (LEXP->m_protBytes == 0) {
                              BEGIN(INITIAL);
                          } অন্যথা যদি (LEXP->m_protBytes < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          যদি (LEXP->m_protBytes != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          যদি (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          প্রত্যাগম VP_TEXT; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          যদি (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          প্রত্যাগম VP_TEXT; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); প্রত্যাগম VP_TEXT;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          প্রত্যাগম VP_TEXT; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          প্রত্যাগম VP_TEXT; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          স্থির string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; yytext = (বর্ণ*)s_rtnfile.c_str(); yyleng = s_rtnfile.length();
                          প্রত্যাগম VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          স্থির বর্ণ s_buf[25];
                          (শূন্য)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->lastLineno());
                          yytext = s_buf; yyleng = std::strlen(yytext);
                          প্রত্যাগম VP_TEXT; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_ঠেলো_state(STRMODE); yymore(); }
<STRMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; প্রত্যাগম VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{word}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_বের_করো_state();
                          যদি (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_মান(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } অন্যথা প্রত্যাগম VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_ঠেলো_state(QQQMODE); yymore(); }
<QQQMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; প্রত্যাগম VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{word}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_বের_করো_state();
                          যদি (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_মান(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } অন্যথা প্রত্যাগম VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_ঠেলো_state(STRIFY); প্রত্যাগম VP_STRIFY; }
<STRIFY><<EOF>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; প্রত্যাগম VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; প্রত্যাগম VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_ঠেলো_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; প্রত্যাগম VP_প্রতীক; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; প্রত্যাগম VP_প্রতীক_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; প্রত্যাগম VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; প্রত্যাগম VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; প্রত্যাগম VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); yytext = (বর্ণ*)"\n"; yyleng = 1; প্রত্যাগম VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; প্রত্যাগম VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; প্রত্যাগম VP_TEXT; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_ঠেলো_state(PRTMODE); yymore(); }
<PRTMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; প্রত্যাগম VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); প্রত্যাগম VP_TEXT; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_TEXT; }

        /* Pass-through include <> filenames */
<INCMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; প্রত্যাগম VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; প্রত্যাগম VP_WHITE; }
<LIBMODE>{quote}        { yy_ঠেলো_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_ঠেলো_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_TEXT; }
<LIBMODE>.              { FL_FWDC; প্রত্যাগম VP_TEXT; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_মান("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_বের_করো_state(); unput('\n'); yyleng=0; প্রত্যাগম VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<EOF>>        { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_বের_করো_state(); unput(yytext[yyleng-1]); yyleng=0; প্রত্যাগম VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_মান(yytext, yyleng); yyleng=0;
                          যদি ((--LEXP->m_formalLevel)==0) { yy_বের_করো_state(); প্রত্যাগম VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_ঠেলো_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; প্রত্যাগম VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<EOF>>        { FL_FWDC; linenoInc(); yy_বের_করো_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; প্রত্যাগম VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_মান((বর্ণ*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_মান((বর্ণ*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_ঠেলো_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_ঠেলো_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_মান(yytext, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_মান(yytext, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_ঠেলো_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_মান((বর্ণ*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_বের_করো_state(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<EOF>>         { FL_FWDC; linenoInc(); yy_বের_করো_state(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_বের_করো_state(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_মান((বর্ণ*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_ঠেলো_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_ঠেলো_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_বের_করো_state(); append_Def_মান(yytext, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_মান(yytext, yyleng-2); append_Def_মান((বর্ণ*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); যদি (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{word}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<EOF>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; প্রত্যাগম VP_EOF_ERROR; }

        /* Preprocessor expression */
<রাশিমালা_2><<EOF>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; প্রত্যাগম VP_EOF_ERROR; }
<রাশিমালা_2>"/*"              { yy_ঠেলো_state(CMTMODE); yymore(); }
<রাশিমালা_2>"//"[^\n\r]*      { FL_FWDC; প্রত্যাগম VP_COMMENT;}
<রাশিমালা_2>"("               { FL_FWDC; প্রত্যাগম VP_TEXT; }  /* V3PreProc will push another EXPR state to stack */
<রাশিমালা_2>")"               { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_TEXT; }
<রাশিমালা_2>"!"               { FL_FWDC; প্রত্যাগম VP_TEXT; }
<রাশিমালা_2>"&&"              { FL_FWDC; প্রত্যাগম VP_TEXT; }
<রাশিমালা_2>"||"              { FL_FWDC; প্রত্যাগম VP_TEXT; }
<রাশিমালা_2>"->"              { FL_FWDC; প্রত্যাগম VP_TEXT; }
<রাশিমালা_2>"<->"             { FL_FWDC; প্রত্যাগম VP_TEXT; }
<রাশিমালা_2>{symb}            { FL_FWDC; প্রত্যাগম VP_প্রতীক; }
<রাশিমালা_2>{crnl}            { FL_FWDC; linenoInc(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_WHITE; }
<রাশিমালা_2>{wsn}+            { FL_FWDC; প্রত্যাগম VP_WHITE; }
<রাশিমালা_2>.                 { FL_FWDC; প্রত্যাগম VP_TEXT; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_ঠেলো_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; প্রত্যাগম VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; প্রত্যাগম VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_WHITE; }
<ARGMODE>{quote}        { yy_ঠেলো_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_ঠেলো_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_ঠেলো_state(STRIFY); প্রত্যাগম VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_মান(yytext, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_মান(yytext, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          যদি (LEXP->m_parenLevel == 1) {  // Starting (
                              যদি (!VString::isWhitespace(LEXP->m_def_মান)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          যদি (LEXP->m_parenLevel>1) {
                              append_Def_মান(yytext, yyleng); FL_BRK;
                          } অন্যথা {
                              প্রত্যাগম VP_TEXT;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          যদি (LEXP->m_parenLevel>0) {
                              append_Def_মান(yytext, yyleng); FL_BRK;
                          } অন্যথা {
                              yy_বের_করো_state(); প্রত্যাগম VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; যদি (LEXP->m_parenLevel>1) {
                              append_Def_মান(yytext, yyleng); FL_BRK;
                          } অন্যথা {
                              yy_বের_করো_state(); প্রত্যাগম VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_মান(yytext, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); প্রত্যাগম VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); প্রত্যাগম VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); প্রত্যাগম VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); প্রত্যাগম VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); প্রত্যাগম VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); প্রত্যাগম VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); প্রত্যাগম VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); প্রত্যাগম VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_WHITE; }
<INITIAL>"//"           { yy_ঠেলো_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_ঠেলো_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_বের_করো_state(); প্রত্যাগম VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; প্রত্যাগম VP_EOF_ERROR; }
<CMTMODE>{word}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; প্রত্যাগম VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; প্রত্যাগম VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); yytext=(বর্ণ*)"\n"; yyleng=1; প্রত্যাগম VP_WHITE; }
<INITIAL><<EOF>>        { FL_FWDC; প্রত্যাগম VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; প্রত্যাগম VP_প্রতীক; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; প্রত্যাগম VP_প্রতীক_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; প্রত্যাগম VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; প্রত্যাগম VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; প্রত্যাগম VP_TEXT; }
%%
// clang-format on

শূন্য V3PreLex::ঠেলো_State_Def_Arg(পূর্ণাঙ্ক level) {
    // Enter define substitution argument state
    yy_ঠেলো_state(ARGMODE);
    m_parenLevel = level;
    m_def_মান = "";
}

শূন্য V3PreLex::ঠেলো_State_Def_Form() {
    // Enter define formal arguments state
    yy_ঠেলো_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_মান = "";
}

শূন্য V3PreLex::ঠেলো_State_Def_মান() {
    // Enter define value state
    yy_ঠেলো_state(DEFVAL);
    m_parenLevel = 0;
    m_def_মান = "";
}

শূন্য V3PreLex::ঠেলো_State_রাশিমালা() {
    // Enter preprocessor expression state
    yy_ঠেলো_state(রাশিমালা_2);
}

শূন্য V3PreLex::ঠেলো_State_Inc_Filename() {
    // Enter include <> filename state
    yy_ঠেলো_state(INCMODE);
    yymore();
}

শূন্য V3PreLex::ঠেলো_State_Passthru() {
    yy_ঠেলো_state(PASSTHRU);
    yymore();
}

শূন্য V3PreLex::setYYDebug(bool on) {
    yy_flex_debug = static_cast<পূর্ণাঙ্ক>(on); }

VPreStream* V3PreLex::newStream(File_সারি* fl, V3PreLex* lexp) {
    VPreStream* ধ্রুব streamp = new VPreStream{fl, lexp};
    যদি (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->ঠেলো_State_Passthru();
    প্রত্যাগম streamp;
}

পূর্ণাঙ্ক V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    প্রত্যাগম yylex();
}

size_t V3PreLex::প্রবেশ_To_Lex(বর্ণ* buf, size_t max_size) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPreStream* streamp = curStreamp();
    যদি (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_size << " bs=" << streamp->m_buffers.size()
             << endl;
        dump_স্তূপ();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    size_t got = 0;
    // Get from this stream
    যাবত্ (got < max_size  // Haven't got enough
           && !streamp->m_buffers.empty()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.বের_করো_front();
        size_t len = front.length();
        যদি (len > (max_size - got)) {  // Front string too big
            len = (max_size - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.ঠেলো_front(remainder);  // Put back remainder for next time
        }
        strncpy(buf + got, front.c_str(), len);
        got += len;
    }
    যদি (!got) {  // end of stream; try "above" file
        bool again = false;
        string forceOut = endOfStream(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        যদি (forceOut != "") {
            যদি (forceOut.length() > max_size) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } অন্যথা {
                got = forceOut.length();
                strncpy(buf, forceOut.c_str(), got);
            }
        } অন্যথা {
            যদি (streamp->m_eof) {
                যদি (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            যদি (again) গচ্ছ again;
        }
    }
    যদি (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    প্রত্যাগম got;
}

string V3PreLex::endOfStream(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    যদি (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_termState << " at " << curFilelinep() << endl;
    }
    যদি (curStreamp()->m_eof) প্রত্যাগম "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    যদি (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_স্তূপ.বের_করো();  // Must work as size>1; EOF is entry 0
        againr = true;
        প্রত্যাগম "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    অন্যথা যদি (!curStreamp()->m_termState) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_termState = 1;
        প্রত্যাগম "\n";  // Exit old file
    } অন্যথা যদি (curStreamp()->m_termState == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_termState = 2;
        প্রত্যাগম "";  // End of file
    } অন্যথা যদি (curStreamp()->m_termState == 2) {
        // Now ending `line
        curStreamp()->m_termState = 3;
        প্রত্যাগম curFilelinep()->সারি_Directive_Strg(0);  // THe "2" exit is below
    } অন্যথা {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_termState = 0;
        File_সারি* ধ্রুব filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_স্তূপ.বের_করো();  // Must work as size>1; EOF is entry 0
        যদি (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        যদি (curStreamp()->m_eof) {
            প্রত্যাগম "";
        } অন্যথা {
            প্রত্যাগম curFilelinep()->সারি_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

শূন্য V3PreLex::initFirstBuffer(File_সারি* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPreStream* ধ্রুব streamp = newStream(filelinep, this);
    streamp->m_eof = true;
    m_streamp_স্তূপ.ঠেলো(streamp);
    //
    m_bufferState = yy_create_buffer(nullptr, YY_BUF_SIZE);
    yy_switch_to_buffer(m_bufferState);
    yyrestart(nullptr);
}

শূন্য V3PreLex::scanNewFile(File_সারি* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    যদি (streamDepth() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } অন্যথা {
        VPreStream* ধ্রুব streamp = newStream(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        scanSwitchStream(streamp);
    }
}

শূন্য V3PreLex::scanBytes(ধ্রুব string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    যদি (streamDepth() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } অন্যথা {
        VPreStream* ধ্রুব streamp = newStream(curFilelinep(), this);
        streamp->m_buffers.ঠেলো_front(str);
        scanSwitchStream(streamp);
    }
}

শূন্য V3PreLex::scanSwitchStream(VPreStream* streamp) {
    curStreamp()->m_buffers.ঠেলো_front(currentUnreadChars());
    m_streamp_স্তূপ.ঠেলো(streamp);
    yyrestart(nullptr);
}

শূন্য V3PreLex::scanBytesBack(ধ্রুব string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    যদি (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.ঠেলো_back(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t বাম = (yy_n_chars - (yy_c_buf_p - currentBuffer()->yy_ch_buf));
    যদি (বাম > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        প্রত্যাগম std::string(yy_c_buf_p, বাম);  // () narrowing conversion
    } অন্যথা {
        প্রত্যাগম "";
    }
}

YY_BUFFER_STATE V3PreLex::currentBuffer() {
    প্রত্যাগম YY_CURRENT_BUFFER;
}

পূর্ণাঙ্ক V3PreLex::currentStartState() ধ্রুব {
    প্রত্যাগম YY_START;
}

শূন্য V3PreLex::সারি_Directive(ধ্রুব বর্ণ* textp) {
    curFilelinep()->সারি_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::addSrcDepend(curFilelinep()->filename());
}

শূন্য V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

শূন্য V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvtToHex(currentBuffer());
#ifdef FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t বাম = (yy_n_chars - (yy_c_buf_p - currentBuffer()->yy_ch_buf));
    cout << " left=" << std::dec << বাম;
#endif
    cout << endl;
}  // LCOV_EXCL_STOP

শূন্য V3PreLex::dump_স্তূপ() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::স্তূপ<VPreStream*> tmpstack = LEXP->m_streamp_স্তূপ;
    যাবত্ (!tmpstack.empty()) {
        ধ্রুব VPreStream* ধ্রুব streamp = tmpstack.শীর্ষ();
        cout << "-    bufferStack[" << cvtToHex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.size()
             << " size0=" << (streamp->m_buffers.empty() ? 0 : streamp->m_buffers.front().length())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.বের_করো();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(ধ্রুব string& in) {
    string result = in;
    string::size_type pos;
    যাবত্ ((pos = result.find('\n')) != string::npos) result.replace(pos, 1, "\\n");
    যাবত্ ((pos = result.find('\r')) != string::npos) result.replace(pos, 1, "\\r");
    প্রত্যাগম result;
}

শূন্য V3PreLex::unused() {
    যদি (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_শীর্ষ_state();
        yyerror((বর্ণ*)"");
    }  // LCOV_EXCL_STOP
}

শূন্য V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.ঠেলো_back(*curFilelinep());
}
শূন্য V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    যদি (m_lexLintState.empty()) প্রত্যাগম;
    curFilelinep()->warnStateFrom(m_lexLintState.back());
    m_lexLintState.বের_করো_back();
}
শূন্য V3PreLex::verilatorCmtLint(ধ্রুব বর্ণ* textp, bool turnOff) {
    ধ্রুব বর্ণ* sp = textp;
    যাবত্ (*sp && std::isspace(*sp)) ++sp;
    যাবত্ (*sp && !std::isspace(*sp)) ++sp;  // "verilator"
    যাবত্ (*sp && std::isspace(*sp)) ++sp;
    যাবত্ (*sp && !std::isspace(*sp)) ++sp;  // "lint_on/lint_off"
    যাবত্ (*sp && std::isspace(*sp)) ++sp;
    string বার্তা = sp;
    প্রতি (auto pos = বার্তা.begin(); pos != বার্তা.end(); ++pos) {
        যদি (std::isspace(*pos) || *pos == '*') {
            বার্তা.erase(pos, বার্তা.end());
            ভঙ্গ;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(বার্তা, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
