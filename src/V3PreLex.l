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
%option ढेर
%option noc++
%option prefix="V3PreLex"
%{
#अगर_घोषित NEVER_JUST_FOR_CLANG_FORMAT
 }
#पूर्ण_अगर

#समावेश "V3PreProc.h"
#समावेश "V3PreLex.h"
#अगर_घोषित _WIN32
# include <io.h> // for isatty
#पूर्ण_अगर

/* clang-format on */

V3PreLex* V3PreLex::s_currentLexp = nullptr;  // Current lexing point

#घोषणा LEXP V3PreLex::s_currentLexp

#घोषणा YY_निवेश(buf, परिणाम, max_आकार_2) \
    करो { परिणाम = LEXP->निवेश_To_Lex(buf, max_आकार_2); } जबतक (false)

// Accessors, because flex keeps changing the type of yyleng
अक्षर* yyourtext() { वापस श_माला; }
माप_प्रकार yyourleng() { वापस yyleng; }
व्योम yyourtext(स्थिर अक्षर* textp, माप_प्रकार आकार) {
    श_माला = (अक्षर*)textp;
    yyleng = आकार;
}

// FL_FWD only tracks columns; preproc uses linenoInc() to track lines, so
// insertion of a \n does not mess up line count
#घोषणा FL_FWDC (LEXP->curFilelinep()->forwardToken(श_माला, yyleng, false))
// Use this to break between tokens whereever not return'ing a token (e.g. skipping inside lexer)
#घोषणा FL_BRK (LEXP->curFilelinep()->startToken())

अटल व्योम linenoInc() { LEXP->linenoInc(); }
अटल bool pedantic() { वापस LEXP->m_pedantic; }
अटल व्योम yyerror(स्थिर अक्षर* संदेश) { LEXP->curFilelinep()->v3error(संदेश); }
अटल व्योम yyerrorf(स्थिर अक्षर* संदेश) { LEXP->curFilelinep()->v3error(संदेश); }
अटल व्योम append_Def_मूल्य(स्थिर अक्षर* t, माप_प्रकार l) { LEXP->append_Def_मूल्य(t, l); }

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
%x व्यंजक_2
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
<PASSTHRU>{crnl}        { FL_FWDC; linenoInc(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_WHITE; }
<PASSTHRU>{word}        { yymore(); }
<PASSTHRU>.             { FL_FWDC; वापस VP_TEXT; }

        /* Special directives we recognize */
<INITIAL>{bom}          { }
<INITIAL,STRIFY>^{ws}*"`line"{ws}+.*{crnl}      { FL_FWDC; LEXP->पंक्ति_Directive(श_माला);
                                                  वापस VP_पंक्ति; }
<INITIAL>"`define"      { FL_FWDC; वापस VP_DEFINE; }
<INITIAL>"`else"        { FL_FWDC; वापस VP_ELSE; }
<INITIAL>"`elsif"       { FL_FWDC; वापस VP_ELSIF; }
<INITIAL>"`endif"       { FL_FWDC; वापस VP_ENDIF; }
<INITIAL>"`ifdef"       { FL_FWDC; वापस VP_IFDEF; }
<INITIAL>"`ifndef"      { FL_FWDC; वापस VP_IFNDEF; }
<INITIAL>"`include"     { FL_FWDC; वापस VP_INCLUDE; }
<INITIAL>"include"      { FL_FWDC; yy_धकेलो_state(LIBMODE); वापस VP_TEXT; }
<INITIAL>"library"      { FL_FWDC; yy_धकेलो_state(LIBMODE); वापस VP_TEXT; }
<INITIAL>"`undef"       { FL_FWDC; वापस VP_UNDEF; }
<INITIAL>"`undefineall" { FL_FWDC; वापस VP_UNDEFINEALL; }
<INITIAL>"`error"       { FL_FWDC; अगर (!pedantic()) वापस VP_ERROR; अन्यथा वापस VP_DEFREF; }
    /* We wanted this to be next to `protect But it must be before `pramga */
    /* we win only because we both match to the end of the line so the length */
    /* is equal and we are first*/
<PRAGMAPRT>"encoding"{wsn}*[^\n\r]* { FL_FWDC;
                          पूर्णांक res;
                          अक्षर enctype[16]; // long enough to hold "quote-printable"
                          अगर (LEXP->m_protBytes > 0) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Multiple `pragma protected encoding sections");
                          }
                          res = माला_पूछो(श_माला + std::म_माप("encoding"), " = (enctype = \"%15[A-Za-z0-9]\", line_length = %d, bytes = %d)", &enctype[0], &LEXP->m_prot_लंबाई, &LEXP->m_protBytes);
                          अगर (res == 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma protected encoding must have an \"enctype\" field");
                          LEXP->m_encType = !VL_STRCASECMP(enctype, "uuencode") ? Enctype::UUENCODE :
                                            !VL_STRCASECMP(enctype, "base64") ? Enctype::BASE64 :
                                            !VL_STRCASECMP(enctype, "quoted-printable") ? Enctype::QUOTE_PRINTABLE :
                                            !VL_STRCASECMP(enctype, "raw") ? Enctype::RAW : Enctype::ERR;
                          अगर (LEXP->m_encType == Enctype::ERR)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Illegal encoding type for `pragma protected encoding");
                          अगर (LEXP->m_encType != Enctype::BASE64)
                              LEXP->curFilelinep()->v3warn(E_UNSUPPORTED, "Unsupported: only BASE64 is recognized for `pragma protected encoding");
                          अगर (res == 3) {
                              अगर ((LEXP->m_encType == Enctype::BASE64) && (LEXP->m_prot_लंबाई & 3))
                                  LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "line_length must be multiple of 4 for BASE64");
                          } अन्यथा {
                              // default values
                              LEXP->m_protBytes = 0;
                              LEXP->m_prot_लंबाई = 76; // ?? default value not mentioned in IEEE spec
                          }
                          BEGIN(INITIAL);
                          वापस VP_TEXT;
                        }
<PRAGMAPRT>"key_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(ENCBASE64);
                          वापस VP_TEXT; }
<PRAGMAPRT>"data_block"{wsn}*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          LEXP->curFilelinep()->v3warn(PROTECTED, "A '`pragma protected data_block' encrypted section was detected and will be skipped.");
                          BEGIN(ENCBASE64);
                          वापस VP_TEXT; }
<PRAGMAPRT>("begin_protected"|"end_protected"|"end")[\n\r]  { FL_FWDC; linenoInc(); BEGIN(INITIAL); वापस VP_TEXT; }
<PRAGMAPRT>"version"{wsn}*={wsn}*[^\n\r]*[\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          वापस VP_TEXT; }
<PRAGMAPRT>("encrypt_agent"|"encrypt_agent_info"|"key_keyowner"|"key_keyname"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          वापस VP_TEXT; }
<PRAGMAPRT>("data_method"|"key_method"){wsn}*={wsn}*[\"][^\n\r]*[\"][\n\r] {
                          FL_FWDC;
                          linenoInc();
                          BEGIN(INITIAL);
                          वापस VP_TEXT; }
    /* end of `pragma protect */
<PRAGMAPRT>{wsn}+        { FL_FWDC; वापस VP_TEXT; }
<PRAGMAPRT>[\n\r]        { FL_FWDC; linenoInc(); BEGIN(INITIAL); वापस VP_TEXT; }

    /* catch-all for unknown '`pragma protect' rules */
<PRAGMAPRT>.            { yyless(0);
                          BEGIN(PRAGMAPRTERR);
                          वापस VP_TEXT; }
<ENCBASE64>[A-Za-z0-9+/]+[=]{0,2}[\n\r] { FL_FWDC; linenoInc(); FL_BRK;
                          अगर ((yyourleng()-1) <= माप_प्रकार(LEXP->m_prot_लंबाई) && ((yyleng & 3) == 1)) {
                              LEXP->m_protBytes -= (yyleng-1)/4*3;
                          } अन्यथा {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 line too long in `pragma protect key_block/data_block");
                          }
                          अगर (श_माला[yyleng-3] == '=')
                              LEXP->m_protBytes++;
                          अगर (श_माला[yyleng-2] == '=')
                              LEXP->m_protBytes++;
                          अगर (LEXP->m_protBytes == 0) {
                              BEGIN(INITIAL);
                          } अन्यथा अगर (LEXP->m_protBytes < 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding (too short) in `pragma protect key_block/data_block");
                          /*return VP_TEXT;*/ }
<ENCBASE64>{wsn}*[\n\r] { FL_FWDC;
                          अगर (LEXP->m_protBytes != 0)
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "BASE64 encoding length mismatch in `pragma protect key_block/data_block");
                          linenoInc(); BEGIN(INITIAL);
                          वापस VP_TEXT; }

    /* Catch only empty `pragma lines */
<INITIAL>"`pragma"{wsn}*[\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          अगर (v3Global.opt.pedantic()) {
                              LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "`pragma is missing a pragma_expression.");
                          }
                          वापस VP_TEXT; }

    /* catch all other nonempty `pragma */
<INITIAL>"`pragma"{wsn}*[^\n\r]  {
                          yyless(yyleng - 1); FL_FWDC;
                          अगर (!v3Global.opt.preprocOnly())
                              BEGIN(PRAGMA);
                          वापस VP_TEXT; }
<PRAGMA>"protect"{wsn}* { FL_FWDC; BEGIN(PRAGMAPRT); वापस VP_TEXT;}
    /* catch-all for unknown `pragma rules */
<PRAGMA>.               { yyless(0);
                          BEGIN(PRAGMAERR);
                          वापस VP_TEXT; }

    /* the catch-all rule only got 1 char, lets get all line */
<PRAGMAERR>[^\n\r]*     { FL_FWDC;
                          /* Add a warning here for unknown pragmas if desired, at the moment , we don't */
                          /* LEXP->curFilelinep()->v3warn(BADPRAGMA, "Unknown `pragma"); */
                          BEGIN(INITIAL);
                          वापस VP_TEXT; }
<PRAGMAPRTERR>[^\n\r]*  { FL_FWDC;
                          LEXP->curFilelinep()->v3warn(BADSTDPRAGMA, "Unknown '`pragma protect' error");
                          BEGIN(INITIAL);
                          वापस VP_TEXT; }
<INITIAL,STRIFY>"`__FILE__"     { FL_FWDC;
                          अटल string s_rtnfile;
                          s_rtnfile = '"'; s_rtnfile += LEXP->curFilelinep()->filenameEsc();
                          s_rtnfile += '"'; श_माला = (अक्षर*)s_rtnfile.c_str(); yyleng = s_rtnfile.लंबाई();
                          वापस VP_STRING; }
<INITIAL,STRIFY>"`__LINE__"     { FL_FWDC;
                          अटल अक्षर s_buf[25];
                          (व्योम)VL_SNPRINTF(s_buf, 25, "%d", LEXP->curFilelinep()->lastLineno());
                          श_माला = s_buf; yyleng = std::म_माप(श_माला);
                          वापस VP_TEXT; }

        /* Pass-through strings */
<INITIAL>{quote}        { yy_धकेलो_state(STRMODE); yymore(); }
<STRMODE><<खातापूर्ण>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated string");
                          yyleng=0; वापस VP_EOF_ERROR; }
<STRMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated string");
                          FL_BRK; BEGIN(INITIAL); }
<STRMODE>{word}         { yymore(); }
<STRMODE>[^\"\\]        { yymore(); }
<STRMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<STRMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<STRMODE>[\\].          { yymore(); }
<STRMODE>{quote}        { FL_FWDC; yy_निकालो_state();
                          अगर (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_मूल्य(श_माला, yyleng);
                              yyleng=0; FL_BRK;
                          } अन्यथा वापस VP_STRING; }

        /* Pass-through quote-quote-quote */
<INITIAL>{quote}{quote}{quote}        { yy_धकेलो_state(QQQMODE); yymore(); }
<QQQMODE><<खातापूर्ण>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated \"\"\" string");
                          yyleng=0; वापस VP_EOF_ERROR; }
<QQQMODE>{crnl}         { FL_FWDC; linenoInc(); yymore(); }
<QQQMODE>{word}         { yymore(); }
<QQQMODE>[^\"\\]        { yymore(); }
<QQQMODE>[\\]{crnl}     { linenoInc(); yymore(); }
<QQQMODE>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<QQQMODE>[\\].          { yymore(); }
<QQQMODE>.              { yymore(); }
<QQQMODE>{quote}{quote}{quote}        { FL_FWDC; yy_निकालो_state();
                          अगर (LEXP->m_parenLevel || LEXP->m_defQuote) {
                              LEXP->m_defQuote=false; append_Def_मूल्य(श_माला, yyleng);
                              yyleng=0; FL_BRK;
                          } अन्यथा वापस VP_STRING; }

        /* Stringification */
<INITIAL>{tickquote}    { FL_FWDC; yy_धकेलो_state(STRIFY); वापस VP_STRIFY; }
<STRIFY><<खातापूर्ण>>         { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated '\"");
                          yyleng=0; वापस VP_EOF_ERROR; }
<STRIFY>"`\\`\""        { FL_FWDC; वापस VP_BACKQUOTE; }
<STRIFY>{quote}         { yy_धकेलो_state(STRMODE); yymore(); }
<STRIFY>{tickquote}     { FL_FWDC; yy_निकालो_state(); वापस VP_STRIFY; }
<STRIFY>{symbdef}       { FL_FWDC; वापस VP_प्रतीक; }
<STRIFY>{symbdef}``     { FL_FWDC; yyleng-=2; वापस VP_प्रतीक_JOIN; }
<STRIFY>"`"{symbdef}    { FL_FWDC; वापस VP_DEFREF; }
<STRIFY>"`"{symbdef}``  { FL_FWDC; yyleng-=2; वापस VP_DEFREF_JOIN; }
<STRIFY>``              { FL_FWDC; yyleng-=2; वापस VP_JOIN; }
<STRIFY>{crnl}          { FL_FWDC; linenoInc(); श_माला = (अक्षर*)"\n"; yyleng = 1; वापस VP_WHITE; }
<STRIFY>{wsn}+          { FL_FWDC; वापस VP_WHITE; }
<STRIFY>{drop}          { FL_FWDC; FL_BRK; }
<STRIFY>[\r]            { FL_FWDC; FL_BRK; }
<STRIFY>.               { FL_FWDC; वापस VP_TEXT; }

        /* Protected blocks */
<INITIAL>"`protected"   { yy_धकेलो_state(PRTMODE); yymore(); }
<PRTMODE><<खातापूर्ण>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in `protected");
                          yyleng = 0; वापस VP_EOF_ERROR; }
<PRTMODE>{crnl}         { FL_FWDC; linenoInc(); वापस VP_TEXT; }
<PRTMODE>.              { yymore(); }
<PRTMODE>"`endprotected" { FL_FWDC; yy_निकालो_state(); वापस VP_TEXT; }

        /* Pass-through include <> filenames */
<INCMODE><<खातापूर्ण>>        { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated include filename");
                          yyleng = 0; वापस VP_EOF_ERROR; }
<INCMODE>{crnl}         { FL_FWDC; linenoInc(); yyerrorf("Unterminated include filename");
                          FL_BRK; BEGIN(INITIAL); }
<INCMODE>[^\>\\]        { yymore(); }
<INCMODE>[\\].          { yymore(); }
<INCMODE>[\>]           { FL_FWDC; yy_निकालो_state(); वापस VP_STRING; }

        /* Library/config file paths */
<LIBMODE>{crnl}         { FL_FWDC; linenoInc(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_WHITE; }
<LIBMODE>{wsn}+         { FL_FWDC; वापस VP_WHITE; }
<LIBMODE>{quote}        { yy_धकेलो_state(STRMODE); yymore(); }
<LIBMODE>"//"           { yy_धकेलो_state(CMTONEM); yymore(); }
<LIBMODE>";"            { FL_FWDC; yy_निकालो_state(); वापस VP_TEXT; }
<LIBMODE>.              { FL_FWDC; वापस VP_TEXT; }

        /* Reading definition formal parenthesis (or not) to begin formal arguments */
        /* Note '(' must IMMEDIATELY follow definition name */
<DEFFPAR>[(]            { FL_FWDC; append_Def_मूल्य("(", 1); LEXP->m_formalLevel=1;
                          FL_BRK; BEGIN(DEFFORM); }
<DEFFPAR>{crnl}         { FL_FWDC; yy_निकालो_state(); unput('\n'); yyleng=0; वापस VP_DEFFORM; }  /* DEFVAL will later grab the return */
<DEFFPAR><<खातापूर्ण>>        { FL_FWDC; yy_निकालो_state(); वापस VP_DEFFORM; }  /* empty formals */
<DEFFPAR>.              { FL_FWDC; yy_निकालो_state(); unput(श_माला[yyleng-1]); yyleng=0; वापस VP_DEFFORM; }  /* empty formals */

        /* Reading definition formals (declaration of a define) */
<DEFFORM>[(]            { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; yyleng=0; ++LEXP->m_formalLevel; }
<DEFFORM>[)]            { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); yyleng=0;
                          अगर ((--LEXP->m_formalLevel)==0) { yy_निकालो_state(); वापस VP_DEFFORM; }
                          FL_BRK; }
<DEFFORM>"/*"           { yy_धकेलो_state(CMTMODE); yymore(); }
<DEFFORM>"//"[^\n\r]*   { FL_FWDC; वापस VP_COMMENT;}
<DEFFORM>{drop}         { FL_FWDC; FL_BRK; }
<DEFFORM><<खातापूर्ण>>        { FL_FWDC; linenoInc(); yy_निकालो_state(); yyerrorf("Unterminated ( in define formal arguments.");
                          yyleng=0; वापस VP_DEFFORM; }
<DEFFORM>{crnl}         { FL_FWDC; linenoInc(); append_Def_मूल्य((अक्षर*)"\n", 1); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFFORM>[\\]{crnl}     { FL_FWDC; linenoInc(); append_Def_मूल्य((अक्षर*)"\\\n", 2); FL_BRK; }  /* Include return so can maintain output line count */
<DEFFORM>{quote}        { LEXP->m_defQuote=true; yy_धकेलो_state(STRMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>{quote}{quote}{quote}   { LEXP->m_defQuote=true; yy_धकेलो_state(QQQMODE); yymore(); }  /* Legal only in default values */
<DEFFORM>"`\\`\""       { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>{tickquote}    { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }  /* Maybe illegal, otherwise in default value */
<DEFFORM>[{\[]          { FL_FWDC; LEXP->m_formalLevel++; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }
<DEFFORM>[}\]]          { FL_FWDC; LEXP->m_formalLevel--; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }
<DEFFORM>[^\/\*\n\r\\(){}\[\]\"]+       |
<DEFFORM>[\\][^\n\r]    |
<DEFFORM>.              { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }

        /* Reading definition value (declaration of a define's text) */
<DEFVAL>"/*"            { LEXP->m_defCmtSlash=false; yy_धकेलो_state(DEFCMT); yymore(); }  /* Special comment parser */
<DEFVAL>"//"[^\n\r]*[\\]{crnl}  { FL_FWDC; linenoInc(); append_Def_मूल्य((अक्षर*)"\n", 1); FL_BRK; }  /* Spec says // not part of define value */
<DEFVAL>"//"            { FL_FWDC; yyless(0); yy_निकालो_state(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_DEFVALUE; }  /* Spec says // not part of define value; comment is lexed as normal text after it */
<DEFVAL>{drop}          { FL_FWDC; FL_BRK; }
<DEFVAL><<खातापूर्ण>>         { FL_FWDC; linenoInc(); yy_निकालो_state(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_DEFVALUE; }  /* Technically illegal, but people complained */
<DEFVAL>{crnl}          { FL_FWDC; linenoInc(); yy_निकालो_state(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_DEFVALUE; }
<DEFVAL>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFVAL>[\\]{crnl}      { FL_FWDC; linenoInc(); append_Def_मूल्य((अक्षर*)"\\\n", 2); FL_BRK; }  /* Return, AND \ is part of define value */
<DEFVAL>{quote}         { LEXP->m_defQuote = true; yy_धकेलो_state(STRMODE); yymore(); }
<DEFVAL>{quote}{quote}{quote}   { LEXP->m_defQuote = true; yy_धकेलो_state(QQQMODE); yymore(); }
<DEFVAL>[^\/\*\n\r\\\"]+        |
<DEFVAL>[\\][^\n\r]     |
<DEFVAL>.               { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }

        /* Comments inside define values - if embedded get added to define value per spec */
        /* - if no \{crnl} ending then the comment belongs to the next line, as a non-embedded comment */
        /* - if all but (say) 3rd line is missing \ then it's indeterminate */
<DEFCMT>"*/"            { FL_FWDC; yy_निकालो_state(); append_Def_मूल्य(श_माला, yyleng); FL_BRK; }
<DEFCMT>[\\]{wsn}+{crnl}  { LEXP->warnBackslashSpace(); yyless(1); }
<DEFCMT>[\\]{crnl}      { FL_FWDC; linenoInc(); LEXP->m_defCmtSlash=true;
                          append_Def_मूल्य(श_माला, yyleng-2); append_Def_मूल्य((अक्षर*)"\n", 1);  /* Return but not \ */
                          FL_BRK; }
<DEFCMT>{crnl}          { linenoInc(); yymore(); अगर (LEXP->m_defCmtSlash) yyerrorf("One line of /* ... */ is missing \\ before newline");
                          BEGIN(CMTMODE); }
<DEFCMT>{word}          { yymore(); }
<DEFCMT>.               { yymore(); }
<DEFCMT><<खातापूर्ण>>         { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                          yyleng=0; वापस VP_EOF_ERROR; }

        /* Preprocessor expression */
<व्यंजक_2><<खातापूर्ण>>           { FL_FWDC; linenoInc(); yyerrorf("EOF in unterminated preprocessor expression");
                          yyleng = 0; वापस VP_EOF_ERROR; }
<व्यंजक_2>"/*"              { yy_धकेलो_state(CMTMODE); yymore(); }
<व्यंजक_2>"//"[^\n\r]*      { FL_FWDC; वापस VP_COMMENT;}
<व्यंजक_2>"("               { FL_FWDC; वापस VP_TEXT; }  /* V3PreProc will push another EXPR state to stack */
<व्यंजक_2>")"               { FL_FWDC; yy_निकालो_state(); वापस VP_TEXT; }
<व्यंजक_2>"!"               { FL_FWDC; वापस VP_TEXT; }
<व्यंजक_2>"&&"              { FL_FWDC; वापस VP_TEXT; }
<व्यंजक_2>"||"              { FL_FWDC; वापस VP_TEXT; }
<व्यंजक_2>"->"              { FL_FWDC; वापस VP_TEXT; }
<व्यंजक_2>"<->"             { FL_FWDC; वापस VP_TEXT; }
<व्यंजक_2>{symb}            { FL_FWDC; वापस VP_प्रतीक; }
<व्यंजक_2>{crnl}            { FL_FWDC; linenoInc(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_WHITE; }
<व्यंजक_2>{wsn}+            { FL_FWDC; वापस VP_WHITE; }
<व्यंजक_2>.                 { FL_FWDC; वापस VP_TEXT; }

        /* Define arguments (use of a define) */
<ARGMODE>"/*"           { yy_धकेलो_state(CMTMODE); yymore(); }
<ARGMODE>"//"[^\n\r]*   { FL_FWDC; वापस VP_COMMENT; }
<ARGMODE>{drop}         { FL_FWDC; FL_BRK; }
<ARGMODE><<खातापूर्ण>>        { FL_FWDC; yyerrorf("EOF in define argument list\n");
                          yyleng = 0; वापस VP_EOF_ERROR; }
<ARGMODE>{crnl}         { FL_FWDC; linenoInc(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_WHITE; }
<ARGMODE>{quote}        { yy_धकेलो_state(STRMODE); yymore(); }
<ARGMODE>{quote}{quote}{quote}    { yy_धकेलो_state(QQQMODE); yymore(); }
<ARGMODE>"`\\`\""       { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }  /* Literal text */
<ARGMODE>{tickquote}    { FL_FWDC; yy_धकेलो_state(STRIFY); वापस VP_STRIFY; }
<ARGMODE>[{\[]          { FL_FWDC; LEXP->m_parenLevel++; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }
<ARGMODE>[}\]]          { FL_FWDC; LEXP->m_parenLevel--; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }
<ARGMODE>[(]            { FL_FWDC; LEXP->m_parenLevel++;
                          // Note paren level 0 means before "(" of starting args
                          // Level 1 means "," between arguments
                          // Level 2+ means one inside the () of an argument
                          अगर (LEXP->m_parenLevel == 1) {  // Starting (
                              अगर (!VString::isWhitespace(LEXP->m_def_मूल्य)) {
                                  yyerrorf("Illegal text before '(' that starts define arguments");
                              }
                          }
                          अगर (LEXP->m_parenLevel>1) {
                              append_Def_मूल्य(श_माला, yyleng); FL_BRK;
                          } अन्यथा {
                              वापस VP_TEXT;
                        }}
<ARGMODE>[)]            { FL_FWDC; LEXP->m_parenLevel--;
                          अगर (LEXP->m_parenLevel>0) {
                              append_Def_मूल्य(श_माला, yyleng); FL_BRK;
                          } अन्यथा {
                              yy_निकालो_state(); वापस VP_DEFARG;
                        }}
<ARGMODE>[,]            { FL_FWDC; अगर (LEXP->m_parenLevel>1) {
                              append_Def_मूल्य(श_माला, yyleng); FL_BRK;
                          } अन्यथा {
                              yy_निकालो_state(); वापस VP_DEFARG;
                        }}
<ARGMODE>"`"{symbdef}   { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>"`"{symbdef}`` { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>``             { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }  /* defref in defref - outer macro expands first */
<ARGMODE>[^\/\*\n\r\\(,){}\[\]\"`]+     |
<ARGMODE>.              { FL_FWDC; append_Def_मूल्य(श_माला, yyleng); FL_BRK; }

        /* Pragma comments. */
<INITIAL>"//"{ws}*"verilator lint_off"[^\n\r/]*      { FL_FWDC; LEXP->verilatorCmtLint(श_माला + 2, true); वापस VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_on"[^\n\r/]*       { FL_FWDC; LEXP->verilatorCmtLint(श_माला + 2, false); वापस VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_restore"[^\n\r/]*  { FL_FWDC; LEXP->verilatorCmtLintRestore(); वापस VP_COMMENT; }
<INITIAL>"//"{ws}*"verilator lint_save"[^\n\r/]*     { FL_FWDC; LEXP->verilatorCmtLintSave(); वापस VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_off"[^*]*"*/"      { FL_FWDC; LEXP->verilatorCmtLint(श_माला + 2, true); वापस VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_on"[^*]*"*/"       { FL_FWDC; LEXP->verilatorCmtLint(श_माला + 2, false); वापस VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_restore"[^*]*"*/"  { FL_FWDC; LEXP->verilatorCmtLintRestore(); वापस VP_COMMENT; }
<INITIAL>"/*"{ws}*"verilator lint_save"[^*]*"*/"     { FL_FWDC; LEXP->verilatorCmtLintSave(); वापस VP_COMMENT; }

        /* One line comments. */
<INITIAL>"//"{ws}*{crnl} { FL_FWDC; linenoInc(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_WHITE; }
<INITIAL>"//"           { yy_धकेलो_state(CMTONEM); yymore(); }
<CMTONEM>[^\n\r]*       { FL_FWDC; yy_निकालो_state(); वापस VP_COMMENT; }

        /* C-style comments. */
        /**** See also DEFCMT */
<INITIAL>"/*"           { yy_धकेलो_state(CMTMODE); yymore(); }
<CMTMODE>"*/"           { FL_FWDC; yy_निकालो_state(); वापस VP_COMMENT; }
<CMTMODE>{crnl}         { linenoInc(); yymore(); }
<CMTMODE><<खातापूर्ण>>        { FL_FWDC; yyerrorf("EOF in '/* ... */' block comment\n");
                                  yyleng=0; वापस VP_EOF_ERROR; }
<CMTMODE>{word}         { yymore(); }
<CMTMODE>.              { yymore(); }

        /* Define calls */
        /* symbdef prevents normal lex rules from making `\`"foo a symbol {`"foo} instead of a BACKQUOTE */
<INITIAL>"`"{symbdef}   { FL_FWDC; वापस VP_DEFREF; }
<INITIAL>"`"{symbdef}`` { FL_FWDC; yyleng-=2; वापस VP_DEFREF_JOIN; }

        /* Generics */
<INITIAL>{crnl}         { FL_FWDC; linenoInc(); श_माला=(अक्षर*)"\n"; yyleng=1; वापस VP_WHITE; }
<INITIAL><<खातापूर्ण>>        { FL_FWDC; वापस VP_EOF; }  /* A "normal" EOF */
<INITIAL>{symb}         { FL_FWDC; वापस VP_प्रतीक; }
<INITIAL>{symb}``       { FL_FWDC; yyleng-=2; वापस VP_प्रतीक_JOIN; }
<INITIAL>``             { FL_FWDC; yyleng-=2; वापस VP_JOIN; }
<INITIAL>{wsn}+         { FL_FWDC; वापस VP_WHITE; }
<INITIAL>{drop}         { FL_FWDC; FL_BRK; }
<INITIAL>[\r]           { FL_FWDC; FL_BRK; }
<INITIAL>.              { FL_FWDC; वापस VP_TEXT; }
%%
// clang-format on

व्योम V3PreLex::धकेलो_State_Def_Arg(पूर्णांक level) {
    // Enter define substitution argument state
    yy_धकेलो_state(ARGMODE);
    m_parenLevel = level;
    m_def_मूल्य = "";
}

व्योम V3PreLex::धकेलो_State_Def_Form() {
    // Enter define formal arguments state
    yy_धकेलो_state(DEFFPAR);  // First is an optional ( to begin args
    m_parenLevel = 0;
    m_def_मूल्य = "";
}

व्योम V3PreLex::धकेलो_State_Def_मूल्य() {
    // Enter define value state
    yy_धकेलो_state(DEFVAL);
    m_parenLevel = 0;
    m_def_मूल्य = "";
}

व्योम V3PreLex::धकेलो_State_व्यंजक() {
    // Enter preprocessor expression state
    yy_धकेलो_state(व्यंजक_2);
}

व्योम V3PreLex::धकेलो_State_Inc_Filename() {
    // Enter include <> filename state
    yy_धकेलो_state(INCMODE);
    yymore();
}

व्योम V3PreLex::धकेलो_State_Passthru() {
    yy_धकेलो_state(PASSTHRU);
    yymore();
}

व्योम V3PreLex::setYYDebug(bool on) {
    yy_flex_debug = static_cast<पूर्णांक>(on); }

VPreStream* V3PreLex::newStream(File_पंक्ति* fl, V3PreLex* lexp) {
    VPreStream* स्थिर streamp = new VPreStream{fl, lexp};
    अगर (v3Global.opt.debugPreprocPassthru()) streamp->m_lexp->धकेलो_State_Passthru();
    वापस streamp;
}

पूर्णांक V3PreLex::lex() {
    V3PreLex::s_currentLexp = this;  // Tell parser where to get/put data
    // Remember token start location, may be updated by the lexer later
    m_tokFilelinep = curFilelinep();
    वापस श_शब्द();
}

माप_प्रकार V3PreLex::निवेश_To_Lex(अक्षर* buf, माप_प्रकार max_आकार_2) {
    // We need a custom YY_INPUT because we can't use flex buffers.
    // Flex buffers are limited to 2GB, and we can't chop into 2G pieces
    // because buffers can't end in the middle of tokens.
    // Note if we switched streams here (which we don't) "buf" would be
    // become a stale invalid pointer.
    //
    VPreStream* streamp = curStreamp();
    अगर (debug() >= 10) {  // LCOV_EXCL_START
        cout << "-  pp:inputToLex ITL s=" << max_आकार_2 << " bs=" << streamp->m_buffers.आकार()
             << endl;
        dump_ढेर();
    }  // LCOV_EXCL_STOP
    // For testing, use really small chunks
    // if (max_size > 13) max_size=13;
again:
    माप_प्रकार got = 0;
    // Get from this stream
    जबतक (got < max_आकार_2  // Haven't got enough
           && !streamp->m_buffers.empty()) {  // And something buffered
        string front = curStreamp()->m_buffers.front();
        streamp->m_buffers.निकालो_front();
        माप_प्रकार len = front.लंबाई();
        अगर (len > (max_आकार_2 - got)) {  // Front string too big
            len = (max_आकार_2 - got);
            string remainder = front.substr(len);
            front.resize(len);
            streamp->m_buffers.धकेलो_front(remainder);  // Put back remainder for next time
        }
        म_नकलन(buf + got, front.c_str(), len);
        got += len;
    }
    अगर (!got) {  // end of stream; try "above" file
        bool again = false;
        string forceOut = endOfStream(again /*ref*/);
        streamp = curStreamp();  // May have been updated
        अगर (forceOut != "") {
            अगर (forceOut.लंबाई() > max_आकार_2) {  // LCOV_EXCL_LINE
                yyerrorf("Output buffer too small for a `line");  // LCOV_EXCL_LINE
            } अन्यथा {
                got = forceOut.लंबाई();
                म_नकलन(buf, forceOut.c_str(), got);
            }
        } अन्यथा {
            अगर (streamp->m_eof) {
                अगर (yy_flex_debug) cout << "-  EOF\n";
            }
            got = 0;  // 0=EOF/EOS - although got was already 0.
            अगर (again) जाओ again;
        }
    }
    अगर (debug() >= 10) {
        cout << "-  pp::inputToLex  got=" << got << " '" << std::string{buf, got} << "'" << endl;
    }
    वापस got;
}

string V3PreLex::endOfStream(bool& againr) {
    // Switch to file or next unputString
    againr = false;
    अगर (yy_flex_debug) {
        cout << "-EOS state=" << curStreamp()->m_termState << " at " << curFilelinep() << endl;
    }
    अगर (curStreamp()->m_eof) वापस "";  // Don't delete the final "EOF" stream
    bool exited_file = curStreamp()->m_file;
    अगर (!exited_file) {
        // Midpoint of stream, just change buffers
        delete curStreamp();
        m_streamp_ढेर.निकालो();  // Must work as size>1; EOF is entry 0
        againr = true;
        वापस "";
    }
    // Multiple steps because we need FLEX to see ending \n and EOS to end
    // any illegal states, like an unterminated `protected region
    अन्यथा अगर (!curStreamp()->m_termState) {
        // First shutdown phase for a file
        // Terminate all files with a newline.  This prevents problems if
        // the user had a define without a terminating newline,
        // otherwise the resumed file's next line would get tacked on.
        // Also makes it likely the `line that changes files comes out
        // immediately.
        curStreamp()->m_termState = 1;
        वापस "\n";  // Exit old file
    } अन्यथा अगर (curStreamp()->m_termState == 1) {
        // Now the EOF - can't be sent with other characters
        curStreamp()->m_termState = 2;
        वापस "";  // End of file
    } अन्यथा अगर (curStreamp()->m_termState == 2) {
        // Now ending `line
        curStreamp()->m_termState = 3;
        वापस curFilelinep()->पंक्ति_Directive_Strg(0);  // THe "2" exit is below
    } अन्यथा {
        // Final shutdown phase for a stream, we can finally change the
        // current fileline to the new stream
        curStreamp()->m_termState = 0;
        File_पंक्ति* स्थिर filelinep = curFilelinep();
        delete curStreamp();
        m_streamp_ढेर.निकालो();  // Must work as size>1; EOF is entry 0
        अगर (curStreamp()->m_eof) {
            // EOF doesn't have a "real" fileline, but a linenumber of 0 from init time
            // Inherit whatever we last parsed so it's more obvious.
            curFilelinep(filelinep);
        }
        // The caller parser remembered the start location for the text we are parsing,
        // but we've discovered there was a file switch along the way, so update it.
        m_tokFilelinep = curFilelinep();
        //
        अगर (curStreamp()->m_eof) {
            वापस "";
        } अन्यथा {
            वापस curFilelinep()->पंक्ति_Directive_Strg(2);  // Reenter resumed file
        }
    }
}

व्योम V3PreLex::init_First_बफर(File_पंक्ति* filelinep) {
    // Called from constructor to make first buffer
    // yy_create_buffer also sets yy_fill_buffer=1 so reads from YY_INPUT
    VPreStream* स्थिर streamp = newStream(filelinep, this);
    streamp->m_eof = true;
    m_streamp_ढेर.धकेलो(streamp);
    //
    m_बफर_State = yy_create_बफर(nullptr, YY_BUF_आकार);
    yy_switch_to_बफर(m_बफर_State);
    yyrestart(nullptr);
}

व्योम V3PreLex::scanNewFile(File_पंक्ति* filelinep) {
    // Called on new open file.  scanBytesBack will be called next.
    अगर (streamDepth() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // The recursive `include in VPreProcImp should trigger first
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } अन्यथा {
        VPreStream* स्थिर streamp = newStream(filelinep, this);
        m_tokFilelinep = curFilelinep();
        streamp->m_file = true;
        scanSwitchStream(streamp);
    }
}

व्योम V3PreLex::scanBytes(स्थिर string& str) {
    // Note buffers also appended in ::scanBytesBack
    // Not "m_buffers.push_front(std::string{strp,len})" as we need a `define
    // to take effect immediately, in the middle of the current buffer
    // Also we don't use scan_bytes that would set yy_fill_buffer
    // which would force Flex to bypass our YY_INPUT routine.
    अगर (streamDepth() > V3PreProc::DEFINE_RECURSION_LEVEL_MAX) {
        // More streams if recursive `define with complex insertion
        // More buffers mostly if something internal goes funky
        yyerrorf("Recursive `define or other nested inclusion");
        curStreamp()->m_eof = true;  // Fake it to stop recursion
    } अन्यथा {
        VPreStream* स्थिर streamp = newStream(curFilelinep(), this);
        streamp->m_buffers.धकेलो_front(str);
        scanSwitchStream(streamp);
    }
}

व्योम V3PreLex::scanSwitchStream(VPreStream* streamp) {
    curStreamp()->m_buffers.धकेलो_front(currentUnreadChars());
    m_streamp_ढेर.धकेलो(streamp);
    yyrestart(nullptr);
}

व्योम V3PreLex::scanBytesBack(स्थिर string& str) {
    // Initial creation, that will pull from YY_INPUT==inputToLex
    // Note buffers also appended in ::scanBytes
    अगर (VL_UNCOVERABLE(curStreamp()->m_eof)) yyerrorf("scanBytesBack not under scanNewFile");
    curStreamp()->m_buffers.धकेलो_back(str);
}

string V3PreLex::currentUnreadChars() {
    // WARNING - Peeking at internals
    ssize_t बायाँ = (yy_n_chars - (yy_c_buf_p - current_बफर()->yy_ch_buf));
    अगर (बायाँ > 0) {  // left may be -1 at EOS
        *(yy_c_buf_p) = (yy_hold_char);
        वापस std::string(yy_c_buf_p, बायाँ);  // () narrowing conversion
    } अन्यथा {
        वापस "";
    }
}

YY_बफर_STATE V3PreLex::current_बफर() {
    वापस YY_CURRENT_बफर;
}

पूर्णांक V3PreLex::currentStartState() स्थिर {
    वापस YY_START;
}

व्योम V3PreLex::पंक्ति_Directive(स्थिर अक्षर* textp) {
    curFilelinep()->पंक्ति_Directive(textp, m_enterExit /*ref*/);
    // Make sure we have a dependency on whatever file was specified
    V3File::addSrcDepend(curFilelinep()->filename());
}

व्योम V3PreLex::warnBackslashSpace() {
    // Make fileline highlight the specific backslash and space
    curFilelinep()->v3warn(
        BSSPACE, "Backslash followed by whitespace, perhaps the whitespace is accidental?");
}

व्योम V3PreLex::dumpSummary() {  // LCOV_EXCL_START
    cout << "-  pp::dumpSummary  curBuf=" << cvtToHex(current_बफर());
#अगर_घोषित FLEX_DEBUG  // Else peeking at internals may cause portability issues
    ssize_t बायाँ = (yy_n_chars - (yy_c_buf_p - current_बफर()->yy_ch_buf));
    cout << " left=" << std::dec << बायाँ;
#पूर्ण_अगर
    cout << endl;
}  // LCOV_EXCL_STOP

व्योम V3PreLex::dump_ढेर() {  // LCOV_EXCL_START
    // For debug use
    dumpSummary();
    std::ढेर<VPreStream*> tmpstack = LEXP->m_streamp_ढेर;
    जबतक (!tmpstack.empty()) {
        स्थिर VPreStream* स्थिर streamp = tmpstack.शीर्ष();
        cout << "-    bufferStack[" << cvtToHex(streamp)
             << "]: " << " at=" << streamp->m_curFilelinep << " nBuf=" << streamp->m_buffers.आकार()
             << " size0=" << (streamp->m_buffers.empty() ? 0 : streamp->m_buffers.front().लंबाई())
             << (streamp->m_eof ? " [EOF]" : "") << (streamp->m_file ? " [FILE]" : "") << endl;
        tmpstack.निकालो();
    }
}  // LCOV_EXCL_STOP

string V3PreLex::cleanDbgStrg(स्थिर string& in) {
    string परिणाम = in;
    string::आकार_type pos;
    जबतक ((pos = परिणाम.find('\n')) != string::npos) परिणाम.replace(pos, 1, "\\n");
    जबतक ((pos = परिणाम.find('\r')) != string::npos) परिणाम.replace(pos, 1, "\\r");
    वापस परिणाम;
}

व्योम V3PreLex::unused() {
    अगर (VL_UNCOVERABLE(false)) {  // LCOV_EXCL_START
        // Prevent unused warnings
        yy_शीर्ष_state();
        yyerror((अक्षर*)"");
    }  // LCOV_EXCL_STOP
}

व्योम V3PreLex::verilatorCmtLintSave() {
    m_lexLintState.धकेलो_back(*curFilelinep());
}
व्योम V3PreLex::verilatorCmtLintRestore() {
    // No error here on restore without save - the verilog.y parse will report as appropriate
    अगर (m_lexLintState.empty()) वापस;
    curFilelinep()->warnStateFrom(m_lexLintState.back());
    m_lexLintState.निकालो_back();
}
व्योम V3PreLex::verilatorCmtLint(स्थिर अक्षर* textp, bool turnOff) {
    स्थिर अक्षर* sp = textp;
    जबतक (*sp && std::है_खाली(*sp)) ++sp;
    जबतक (*sp && !std::है_खाली(*sp)) ++sp;  // "verilator"
    जबतक (*sp && std::है_खाली(*sp)) ++sp;
    जबतक (*sp && !std::है_खाली(*sp)) ++sp;  // "lint_on/lint_off"
    जबतक (*sp && std::है_खाली(*sp)) ++sp;
    string संदेश = sp;
    क्रम (स्वतः pos = संदेश.begin(); pos != संदेश.end(); ++pos) {
        अगर (std::है_खाली(*pos) || *pos == '*') {
            संदेश.erase(pos, संदेश.end());
            अवरोध;
        }
    }
    // No warnings on bad warning codes - the verilog.y parse will report as appropriate
    curFilelinep()->warnOffParse(संदेश, turnOff);
}

/*###################################################################
 * Local Variables:
 * mode: C++
 * End:
 */
