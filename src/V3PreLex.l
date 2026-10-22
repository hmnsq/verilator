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

%विकल्पः noyywrap align interactive
%विकल्पः राशिः
%विकल्पः noc++
%विकल्पः उपसर्गः="V3PreLex"
%{
#यदि_परिभाषितम् NEVER_JUST_FOR_CLANG_FORMAT
 }
#यदि_अन्तः

#अन्तर्भाव "V3PreProc.h"
#अन्तर्भाव "V3PreLex.h"
#यदि_परिभाषितम् _WIN32
# include <io.h> // for isatty
#यदि_अन्तः

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#परिभाषय LEXP V3PreLex::s_currentLexp

#परिभाषय YY_निवेशः(buf, परिणाम, max_आकार_2) \
    कुरु { परिणाम = LEXP->निवेशः_गन्तव्यम्_Lex(buf, max_आकार_2); } यावत् (false)

// Accessors, because flex keeps changing the type of yyleng
वर्ण* yyourtext() { प्रत्यागम yytext; }
आकार_t yyourleng() { प्रत्यागम yyleng; }
शून्य yyourtext(ध्रुव वर्ण* textp, आकार_t आकार) {
    yytext = (वर्ण*)textp;
    yyleng = आकार;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#परिभाषय FL_FWDC (LEXP->curFilelinep()->forwardToken(yytext, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#परिभाषय FL_BRK (LEXP->curFilelinep()->startToken())

स्थिर शून्य linenoInc() { LEXP->linenoInc(); }
स्थिर bool pedantic() { प्रत्यागम LEXP->m_pedantic; }
स्थिर शून्य yyerror(ध्रुव वर्ण* सन्देशः_2) { LEXP->curFilelinep()->v3error(सन्देशः_2); }
स्थिर शून्य yyerrorf(ध्रुव वर्ण* सन्देशः_2) { LEXP->curFilelinep()->v3error(सन्देशः_2); }
स्थिर शून्य append_Def_मानम्(ध्रुव वर्ण* t, आकार_t l) { LEXP->append_Def_मानम्(t, l); }

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
%x व्यञ्जकम्_2
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
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_WHITE; }
<PASSTHRU>{word}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; प्रत्यागम VP_TEXT; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->पङ्क्तिः_Directive(yytext);
                                                  प्रत्यागम VP_पङ्क्तिः; }
<INITIAL>"`define"      { FL_FWDC; प्रत्यागम VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; प्रत्यागम VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; प्रत्यागम VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; प्रत्यागम VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; प्रत्यागम VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; प्रत्यागम VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; प्रत्यागम VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_प्रक्षिप_state(LIBMODE); प्रत्यागम VP_TEXT; }
<INITIAL>"library"      { FL_FWDC; yy_प्रक्षिप_state(LIBMODE); प्रत्यागम VP_TEXT; }
<INITIAL>"`undef"       { FL_FWDC; प्रत्यागम VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; प्रत्यागम VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; यदि (!pedantic()) प्रत्यागम VP_ERROR; अन्यथा प्रत्यागम VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          पूर्णाङ्क res;
                          वर्ण enctype[16]; // long enough to hold "quote-printable"
                          यदि (LEXP->m_prot_अष्टकानि > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = वाक्यात्_पठ(yytext + std::वाक्यदैर्घ्यम्("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_prot_दीर्घता, &LEXP->m_prot_अष्टकानि);
                          यदि (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          यदि (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          यदि (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          यदि (res == 3) {
                              यदि ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_prot_दीर्घता & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } अन्यथा {
                              // default values
                              LEXP->m_prot_अष्टकानि = 0;
                              LEXP->m_prot_दीर्घता = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          प्रत्यागम VP_TEXT; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          प्रत्यागम VP_TEXT; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); प्रत्यागम VP_TEXT; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; प्रत्यागम VP_TEXT; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); प्रत्यागम VP_TEXT; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          प्रत्यागम VP_TEXT; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          यदि ((yyourleng()-1) <= आकार_t(LEXP->m_prot_दीर्घता) && ((yyleng & 3) == 1)) {
                              LEXP->m_prot_अष्टकानि -= (yyleng-1)/4*3;
                          } अन्यथा {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          यदि (yytext[yyleng-3] == '=')
                              LEXP->m_prot_अष्टकानि++;
                          यदि (yytext[yyleng-2] == '=')
                              LEXP->m_prot_अष्टकानि++;
                          यदि (LEXP->m_prot_अष्टकानि == 0) {
                              BEGIN(INITIAL);
                          } अन्यथा यदि (LEXP->m_prot_अष्टकानि < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          यदि (LEXP->m_prot_अष्टकानि != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          यदि (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          प्रत्यागम VP_TEXT; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          यदि (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          प्रत्यागम VP_TEXT; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); प्रत्यागम VP_TEXT;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          प्रत्यागम VP_TEXT; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          प्रत्यागम VP_TEXT; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          स्थिर string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; yytext = (वर्ण*)s_rtnfile.c_str(); yyleng = s_rtnfile.दीर्घता_2();
                          प्रत्यागम VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          स्थिर वर्ण s_buf[25];
                          (शून्य)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->चरमः_Lineno());
                          yytext = s_buf; yyleng = std::वाक्यदैर्घ्यम्(yytext);
                          प्रत्यागम VP_TEXT; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_प्रक्षिप_state(STRMODE); yymore(); }
<STRMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; प्रत्यागम VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{word}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_निष्कासय_state();
                          यदि (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_मानम्(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } अन्यथा प्रत्यागम VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_प्रक्षिप_state(QQQMODE); yymore(); }
<QQQMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; प्रत्यागम VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{word}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_निष्कासय_state();
                          यदि (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_मानम्(yytext, yyleng);
                              yyleng=0; FL_BRK;
                          } अन्यथा प्रत्यागम VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_प्रक्षिप_state(STRIFY); प्रत्यागम VP_STRIFY; }
<STRIFY><<EOF>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; प्रत्यागम VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; प्रत्यागम VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_प्रक्षिप_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; प्रत्यागम VP_चिह्नम्; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; प्रत्यागम VP_चिह्नम्_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; प्रत्यागम VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; प्रत्यागम VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; प्रत्यागम VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); yytext = (वर्ण*)"\n"; yyleng = 1; प्रत्यागम VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; प्रत्यागम VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; प्रत्यागम VP_TEXT; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_प्रक्षिप_state(PRTMODE); yymore(); }
<PRTMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; प्रत्यागम VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); प्रत्यागम VP_TEXT; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_TEXT; }

        /* Pass-through include <> filenames */
<INCMODE><<EOF>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; प्रत्यागम VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; प्रत्यागम VP_WHITE; }
<LIBMODE>{quote}        { yy_प्रक्षिप_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_प्रक्षिप_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_TEXT; }
<LIBMODE>.              { FL_FWDC; प्रत्यागम VP_TEXT; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_मानम्("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_निष्कासय_state(); unput('\n'); yyleng=0; प्रत्यागम VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<EOF>>        { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_निष्कासय_state(); unput(yytext[yyleng-1]); yyleng=0; प्रत्यागम VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_मानम्(yytext, yyleng); yyleng=0;
                          यदि ((--LEXP->m_formalLevel)==0) { yy_निष्कासय_state(); प्रत्यागम VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_प्रक्षिप_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; प्रत्यागम VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<EOF>>        { FL_FWDC; linenoInc(); yy_निष्कासय_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; प्रत्यागम VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_मानम्((वर्ण*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_मानम्((वर्ण*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_प्रक्षिप_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_प्रक्षिप_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_मानम्(yytext, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_मानम्(yytext, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_प्रक्षिप_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_मानम्((वर्ण*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_निष्कासय_state(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<EOF>>         { FL_FWDC; linenoInc(); yy_निष्कासय_state(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_निष्कासय_state(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_मानम्((वर्ण*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_प्रक्षिप_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_प्रक्षिप_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_निष्कासय_state(); append_Def_मानम्(yytext, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_मानम्(yytext, yyleng-2); append_Def_मानम्((वर्ण*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); यदि (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{word}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<EOF>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; प्रत्यागम VP_EOF_ERROR; }

        /* Preprocessor expression */
<व्यञ्जकम्_2><<EOF>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; प्रत्यागम VP_EOF_ERROR; }
<व्यञ्जकम्_2>"/*"              { yy_प्रक्षिप_state(CMTMODE); yymore(); }
<व्यञ्जकम्_2>"//"[^\n\r]*      { FL_FWDC; प्रत्यागम VP_COMMENT;}
<व्यञ्जकम्_2>"("               { FL_FWDC; प्रत्यागम VP_TEXT; }  /* V3PreProc will push another EXPR state to stack */
<व्यञ्जकम्_2>")"               { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_TEXT; }
<व्यञ्जकम्_2>"!"               { FL_FWDC; प्रत्यागम VP_TEXT; }
<व्यञ्जकम्_2>"&&"              { FL_FWDC; प्रत्यागम VP_TEXT; }
<व्यञ्जकम्_2>"||"              { FL_FWDC; प्रत्यागम VP_TEXT; }
<व्यञ्जकम्_2>"->"              { FL_FWDC; प्रत्यागम VP_TEXT; }
<व्यञ्जकम्_2>"<->"             { FL_FWDC; प्रत्यागम VP_TEXT; }
<व्यञ्जकम्_2>{symb}            { FL_FWDC; प्रत्यागम VP_चिह्नम्; }
<व्यञ्जकम्_2>{crnl}            { FL_FWDC; linenoInc(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_WHITE; }
<व्यञ्जकम्_2>{wsn}+            { FL_FWDC; प्रत्यागम VP_WHITE; }
<व्यञ्जकम्_2>.                 { FL_FWDC; प्रत्यागम VP_TEXT; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_प्रक्षिप_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; प्रत्यागम VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; प्रत्यागम VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_WHITE; }
<ARGMODE>{quote}        { yy_प्रक्षिप_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_प्रक्षिप_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_प्रक्षिप_state(STRIFY); प्रत्यागम VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_मानम्(yytext, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_मानम्(yytext, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          यदि (LEXP->m_parenLevel == 1) {  // Starting (
                              यदि (!VString::isWhitespace(LEXP->m_def_मानम्)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          यदि (LEXP->m_parenLevel>1) {
                              append_Def_मानम्(yytext, yyleng); FL_BRK;
                          } अन्यथा {
                              प्रत्यागम VP_TEXT;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          यदि (LEXP->m_parenLevel>0) {
                              append_Def_मानम्(yytext, yyleng); FL_BRK;
                          } अन्यथा {
                              yy_निष्कासय_state(); प्रत्यागम VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; यदि (LEXP->m_parenLevel>1) {
                              append_Def_मानम्(yytext, yyleng); FL_BRK;
                          } अन्यथा {
                              yy_निष्कासय_state(); प्रत्यागम VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_मानम्(yytext, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); प्रत्यागम VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); प्रत्यागम VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); प्रत्यागम VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); प्रत्यागम VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, true); प्रत्यागम VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(yytext + 2, false); प्रत्यागम VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); प्रत्यागम VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); प्रत्यागम VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_WHITE; }
<INITIAL>"//"           { yy_प्रक्षिप_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_प्रक्षिप_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_निष्कासय_state(); प्रत्यागम VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<EOF>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; प्रत्यागम VP_EOF_ERROR; }
<CMTMODE>{word}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; प्रत्यागम VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; प्रत्यागम VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); yytext=(वर्ण*)"\n"; yyleng=1; प्रत्यागम VP_WHITE; }
<INITIAL><<EOF>>        { FL_FWDC; प्रत्यागम VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; प्रत्यागम VP_चिह्नम्; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; प्रत्यागम VP_चिह्नम्_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; प्रत्यागम VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; प्रत्यागम VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; प्रत्यागम VP_TEXT; }
%%
// clang-format on

शून्य V3PreLex::प्रक्षिप_State_Def_Arg(पूर्णाङ्क level) {
    // Enter define substitution argument state
    yy_प्रक्षिप_state(ARGMODE);
    m_parenLevel = level;
    m_def_मानम् = "";
}

शून्य V3PreLex::प्रक्षिप_State_Def_Form() {
    // Enter define formal arguments state
    yy_प्रक्षिप_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_मानम् = "";
}

शून्य V3PreLex::प्रक्षिप_State_Def_मानम्() {
    // Enter define value state
    yy_प्रक्षिप_state(DEFVAL);
    m_parenLevel = 0;
    m_def_मानम् = "";
}

शून्य V3PreLex::प्रक्षिप_State_व्यञ्जकम्() {
    // Enter preprocessor expression state
    yy_प्रक्षिप_state(व्यञ्जकम्_2);
}

शून्य V3PreLex::प्रक्षिप_State_Inc_Filename() {
    // Enter include <> filename state
    yy_प्रक्षिप_state(INCMODE);
    yymore();
}

शून्य V3PreLex::प्रक्षिप_State_Passthru() {
    yy_प्रक्षिप_state(PASSTHRU);
    yymore();
}

शून्य V3PreLex::समुच्चयः_YYDebug(bool on) {
    yy_flex_debug = static_cast<पूर्णाङ्क>(on); }

VPre_प्रवाहः* V3PreLex::new_प्रवाहः(File_पङ्क्तिः* fl, V3PreLex* lexp) {
    VPre_प्रवाहः* ध्रुव streamp = new VPre_प्रवाहः{fl, lexp};
    यदि (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->प्रक्षिप_State_Passthru();
    प्रत्यागम streamp;
}

पूर्णाङ्क V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    प्रत्यागम yylex();
}

आकार_t V3PreLex::निवेशः_गन्तव्यम्_Lex(वर्ण* buf, आकार_t max_आकार_2) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPre_प्रवाहः* streamp = curStreamp();
    यदि (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_आकार_2 << " bs=" << streamp->m_buffers.आकार()
             << endl;
        dump_राशिः();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    आकार_t got = 0;
    // Get from this stream
    यावत् (got < max_आकार_2  // Haven't got enough
           && !streamp->m_buffers.रिक्तानि()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.निष्कासय_front();
        आकार_t len = front.दीर्घता_2();
        यदि (len > (max_आकार_2 - got)) {  // Front string too big
            len = (max_आकार_2 - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.प्रक्षिप_front(remainder);  // Put back remainder for next time
        }
        strncpy(buf + got, front.c_str(), len);
        got += len;
    }
    यदि (!got) {  // end of stream; try "above" file
        bool again = false;
        string force_बहिर्गमनम् = अन्तः_सम्बन्धि_प्रवाहः(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        यदि (force_बहिर्गमनम् != "") {
            यदि (force_बहिर्गमनम्.दीर्घता_2() > max_आकार_2) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } अन्यथा {
                got = force_बहिर्गमनम्.दीर्घता_2();
                strncpy(buf, force_बहिर्गमनम्.c_str(), got);
            }
        } अन्यथा {
            यदि (streamp->m_eof) {
                यदि (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            यदि (again) गच्छ again;
        }
    }
    यदि (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    प्रत्यागम got;
}

string V3PreLex::अन्तः_सम्बन्धि_प्रवाहः(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    यदि (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_पदांशः_State << " at " << curFilelinep() << endl;
    }
    यदि (curStreamp()->m_eof) प्रत्यागम "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    यदि (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_राशिः.निष्कासय();  // Must work as size>1; EOF is entry 0
        againr = true;
        प्रत्यागम "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    अन्यथा यदि (!curStreamp()->m_पदांशः_State) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_पदांशः_State = 1;
        प्रत्यागम "\n";  // Exit old file
    } अन्यथा यदि (curStreamp()->m_पदांशः_State == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_पदांशः_State = 2;
        प्रत्यागम "";  // End of file
    } अन्यथा यदि (curStreamp()->m_पदांशः_State == 2) {
        // Now ending `line
        curStreamp()->m_पदांशः_State = 3;
        प्रत्यागम curFilelinep()->पङ्क्तिः_Directive_Strg(0);  // THe "2" exit is below
    } अन्यथा {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_पदांशः_State = 0;
        File_पङ्क्तिः* ध्रुव filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_राशिः.निष्कासय();  // Must work as size>1; EOF is entry 0
        यदि (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        यदि (curStreamp()->m_eof) {
            प्रत्यागम "";
        } अन्यथा {
            प्रत्यागम curFilelinep()->पङ्क्तिः_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

शून्य V3PreLex::init_First_बफर(File_पङ्क्तिः* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPre_प्रवाहः* ध्रुव streamp = new_प्रवाहः(filelinep, this);
    streamp->m_eof = true;
    m_streamp_राशिः.प्रक्षिप(streamp);
    //
    m_बफर_State = yy_create_बफर(nullptr, YY_BUF_आकार);
    yy_switch_गन्तव्यम्_बफर(m_बफर_State);
    yyrestart(nullptr);
}

शून्य V3PreLex::अवलोकनम्_New_File(File_पङ्क्तिः* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    यदि (प्रवाहः_गभीरता() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } अन्यथा {
        VPre_प्रवाहः* ध्रुव streamp = new_प्रवाहः(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        अवलोकनम्_Switch_प्रवाहः(streamp);
    }
}

शून्य V3PreLex::अवलोकनम्_अष्टकानि(ध्रुव string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    यदि (प्रवाहः_गभीरता() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } अन्यथा {
        VPre_प्रवाहः* ध्रुव streamp = new_प्रवाहः(curFilelinep(), this);
        streamp->m_buffers.प्रक्षिप_front(str);
        अवलोकनम्_Switch_प्रवाहः(streamp);
    }
}

शून्य V3PreLex::अवलोकनम्_Switch_प्रवाहः(VPre_प्रवाहः* streamp) {
    curStreamp()->m_buffers.प्रक्षिप_front(currentUnreadChars());
    m_streamp_राशिः.प्रक्षिप(streamp);
    yyrestart(nullptr);
}

शून्य V3PreLex::अवलोकनम्_अष्टकानि_पृष्ठभागः(ध्रुव string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    यदि (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.प्रक्षिप_पृष्ठभागः(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t वामः = (yy_n_chars - (yy_c_buf_p - current_बफर()->yy_ch_buf));
    यदि (वामः > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        प्रत्यागम std::string(yy_c_buf_p, वामः);  // () narrowing conversion
    } अन्यथा {
        प्रत्यागम "";
    }
}

YY_बफर_STATE V3PreLex::current_बफर() {
    प्रत्यागम YY_CURRENT_बफर;
}

पूर्णाङ्क V3PreLex::currentStartState() ध्रुव {
    प्रत्यागम YY_START;
}

शून्य V3PreLex::पङ्क्तिः_Directive(ध्रुव वर्ण* textp) {
    curFilelinep()->पङ्क्तिः_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::सङ्कलय_Src_Depend(curFilelinep()->filename());
}

शून्य V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

शून्य V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvt_गन्तव्यम्_Hex(current_बफर());
#यदि_परिभाषितम् FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t वामः = (yy_n_chars - (yy_c_buf_p - current_बफर()->yy_ch_buf));
    cout << " left=" << std::dec << वामः;
#यदि_अन्तः
    cout << endl;
}  // LCOV_EXCL_STOP

शून्य V3PreLex::dump_राशिः() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::राशिः<VPre_प्रवाहः*> tmpstack = LEXP->m_streamp_राशिः;
    यावत् (!tmpstack.रिक्तानि()) {
        ध्रुव VPre_प्रवाहः* ध्रुव streamp = tmpstack.शिखरम्();
        cout << "-    bufferStack[" << cvt_गन्तव्यम्_Hex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.आकार()
             << " size0=" << (streamp->m_buffers.रिक्तानि() ? 0 : streamp->m_buffers.front().दीर्घता_2())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.निष्कासय();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(ध्रुव string& in) {
    string परिणाम = in;
    string::आकार_type pos;
    यावत् ((pos = परिणाम.find('\n')) != string::npos) परिणाम.replace(pos, 1, "\\n");
    यावत् ((pos = परिणाम.find('\r')) != string::npos) परिणाम.replace(pos, 1, "\\r");
    प्रत्यागम परिणाम;
}

शून्य V3PreLex::अप्रयुक्तम्() {
    यदि (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_शिखरम्_state();
        yyerror((वर्ण*)"");
    }  // LCOV_EXCL_STOP
}

शून्य V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.प्रक्षिप_पृष्ठभागः(*curFilelinep());
}
शून्य V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    यदि (m_lexLintState.रिक्तानि()) प्रत्यागम;
    curFilelinep()->warnStateFrom(m_lexLintState.पृष्ठभागः());
    m_lexLintState.निष्कासय_पृष्ठभागः();
}
शून्य V3PreLex::verilatorCmtLint(ध्रुव वर्ण* textp, bool turnOff) {
    ध्रुव वर्ण* sp = textp;
    यावत् (*sp && std::रिक्तम्_वा(*sp)) ++sp;
    यावत् (*sp && !std::रिक्तम्_वा(*sp)) ++sp;  // "verilator"
    यावत् (*sp && std::रिक्तम्_वा(*sp)) ++sp;
    यावत् (*sp && !std::रिक्तम्_वा(*sp)) ++sp;  // "lint_on/lint_off"
    यावत् (*sp && std::रिक्तम्_वा(*sp)) ++sp;
    string सन्देशः_2 = sp;
    प्रति (स्वचालित pos = सन्देशः_2.begin(); pos != सन्देशः_2.अन्तः_2(); ++pos) {
        यदि (std::रिक्तम्_वा(*pos) || *pos == '*') {
            सन्देशः_2.erase(pos, सन्देशः_2.अन्तः_2());
            भङ्ग;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(सन्देशः_2, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
