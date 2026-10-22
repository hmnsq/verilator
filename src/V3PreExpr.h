// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilog::Preproc: Preprocess verilog code
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2000-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3PREEXPR_H_
#define VERILATOR_V3PREEXPR_H_

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Global.h"

#അന്തര്ഭാവ <deque>

class V3_Pre_വ്യഞ്ജകം_Token final {
പൊതു:
    // TYPES
    // Order of enum must match token table
    എണ്ണൽ token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

സ്വകാര്യ:
    // MEMBERS
    File_വരി* സ്ഥിരം m_fileline;  // Token fileline
    token_t സ്ഥിരം m_token;  // Token value
പൊതു:
    // METHODS
    V3_Pre_വ്യഞ്ജകം_Token(File_വരി* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_വ്യഞ്ജകം_Token(File_വരി* fileline, bool മൂല്യം)
        : m_fileline{fileline}
        , m_token{മൂല്യം ? ONE : ZERO} {}
    ~V3_Pre_വ്യഞ്ജകം_Token() = സ്ഥിരസ്ഥിതി;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        തിരികെ names[m_token];
    }
    File_വരി* fileline() സ്ഥിരം { തിരികെ m_fileline; }
    token_t token() സ്ഥിരം { തിരികെ m_token; }
    bool is_മൂല്യം() സ്ഥിരം { തിരികെ m_token == ZERO || m_token == ONE; }
    bool മൂല്യം() സ്ഥിരം {
        UASSERT(is_മൂല്യം(), "preproc expr fetch of non-value");
        തിരികെ m_token == ONE;
    }
};

class V3_Pre_വ്യഞ്ജകം final {
    // MEMBERS
    std::deque<V3_Pre_വ്യഞ്ജകം_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_വ്യഞ്ജകം_Token> m_മൂല്യങ്ങൾ;  // Value stack
    std::deque<V3_Pre_വ്യഞ്ജകം_Token> m_ops;  // Operator stack
    File_വരി* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    എണ്ണൽ action_t : uint8_t {
        VV,  //മൂല്യം
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    സ്ഥിതം സ്ഥിരം അക്ഷരം* actionAscii(action_t en) {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* names[] = {"VV", "AA", "RR", "SS", "EE"};
        തിരികെ names[en];
    }

    //  Operators  Associativity  Precedence
    //  ---------  -------------  ----------
    //  ()         Left           Highest
    //  !          (unary)
    //  &&         Left
    //  ||         Left
    //  -> <->     Right          Lowest
    //
    //  If different op precedence, shift for higher to input, else reduce for lower
    //  If same op precedence, shift for right assoc, reduce for left assoc

    action_t parseTable[V3_Pre_വ്യഞ്ജകം_Token::MAX][V3_Pre_വ്യഞ്ജകം_Token::MAX] = {
        // stack   ------------- inputs ------------------
        //         0   1   $   (   )   !   &&  ||  ->  <->
        /* 0   */ {EE, EE, EE, EE, EE, EE, EE, EE, EE, EE},  // 0 never on op stack
        /* 1   */ {EE, EE, EE, EE, EE, EE, EE, EE, EE, EE},  // 1 never on op stack
        /* $   */ {VV, VV, AA, SS, EE, SS, SS, SS, SS, SS},
        /* (   */ {VV, VV, EE, SS, RR, SS, SS, SS, SS, SS},
        /* )   */ {VV, VV, RR, EE, RR, RR, RR, RR, RR, RR},
        /* !   */ {VV, VV, RR, SS, RR, SS, RR, RR, RR, RR},
        /* &&  */ {VV, VV, RR, SS, RR, SS, SS, RR, RR, RR},
        /* ||  */ {VV, VV, RR, SS, RR, SS, SS, SS, RR, RR},
        /* ->  */ {VV, VV, RR, SS, RR, SS, SS, SS, RR, RR},
        /* <-> */ {VV, VV, RR, SS, RR, SS, SS, SS, RR, RR}};

    സ്ഥിതം ശൂന്യം selfTestImp() {
        selfTestOne("0", false);
        selfTestOne("1", true);
        selfTestOne("! 0", true);
        selfTestOne("! 1", false);
        selfTestOne("0 || 0", false);
        selfTestOne("0 || 1", true);
        selfTestOne("1 || 0", true);
        selfTestOne("1 || 1", true);
        selfTestOne("0 && 0", false);
        selfTestOne("0 && 1", false);
        selfTestOne("1 && 0", false);
        selfTestOne("1 && 1", true);
        selfTestOne("0 -> 0", true);
        selfTestOne("0 -> 1", true);
        selfTestOne("1 -> 0", false);
        selfTestOne("1 -> 1", true);
        selfTestOne("0 <-> 0", true);
        selfTestOne("0 <-> 1", false);
        selfTestOne("1 <-> 0", false);
        selfTestOne("1 <-> 1", true);
        selfTestOne("1 || 0 && 1", false);
        selfTestOne("( 1 || 0 ) && 1", true);
        selfTestOne("! 1 || ! 1", false);
        selfTestOne("! 0 && ! 0", true);
    }

    സ്ഥിതം ശൂന്യം selfTestOne(സ്ഥിരം string& വ്യഞ്ജകം, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << വ്യഞ്ജകം);
        File_വരി* സ്ഥിരം flp = nullptr;
        V3_Pre_വ്യഞ്ജകം parser;
        parser.reset(flp);
        bool tstart = true;
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = വ്യഞ്ജകം.c_str(); *cp; ++cp) {
            എങ്കിൽ (tstart) {
                tstart = false;
                തിരഞ്ഞെടുപ്പ് (*cp) {
                അവസ്ഥ '0': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::ZERO}); നിർത്തുക;
                അവസ്ഥ '1': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::ONE}); നിർത്തുക;
                അവസ്ഥ '!': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::LNOT}); നിർത്തുക;
                അവസ്ഥ '|': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::LOR}); നിർത്തുക;
                അവസ്ഥ '&': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::LAND}); നിർത്തുക;
                അവസ്ഥ '-': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::IMP}); നിർത്തുക;
                അവസ്ഥ '<': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::EQV}); നിർത്തുക;
                അവസ്ഥ '(': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::BRA}); നിർത്തുക;
                അവസ്ഥ ')': parser.തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::KET}); നിർത്തുക;
                സ്ഥിരസ്ഥിതി: നിർത്തുക;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (*cp == ' ') {
                tstart = true;
            }
        }
        സ്ഥിരം bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    ശൂന്യം തള്ളുക_Op(സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.തള്ളുക_back(token);
    }
    ശൂന്യം തള്ളുക_മൂല്യം(സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_മൂല്യങ്ങൾ.തള്ളുക_back(token);
    }
    V3_Pre_വ്യഞ്ജകം_Token എടുക്കുക_മൂല്യം() {
        എങ്കിൽ (m_മൂല്യങ്ങൾ.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            തിരികെ V3_Pre_വ്യഞ്ജകം_Token{m_firstFileline, false};
        }
        സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token tok = m_മൂല്യങ്ങൾ.back();
        m_മൂല്യങ്ങൾ.എടുക്കുക_back();
        // UINFO(9, "  popValue " << tok.ascii());
        തിരികെ tok;
    }
    ശൂന്യം reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.എടുക്കുക_back();
        തിരഞ്ഞെടുപ്പ് (tok.token()) {
        അവസ്ഥ V3_Pre_വ്യഞ്ജകം_Token::KET: {
            ആകുമ്പോൾ (m_ops.back().token() != V3_Pre_വ്യഞ്ജകം_Token::END
                   && m_ops.back().token() != V3_Pre_വ്യഞ്ജകം_Token::BRA)
                reduce();
            എങ്കിൽ (m_ops.back().token() == V3_Pre_വ്യഞ്ജകം_Token::BRA) {
                m_ops.എടുക്കുക_back();
            } അല്ലെങ്കിൽ {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                തിരികെ;
            }
            നിർത്തുക;
        }
        അവസ്ഥ V3_Pre_വ്യഞ്ജകം_Token::LNOT: {
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token lhs = എടുക്കുക_മൂല്യം();
            തള്ളുക_മൂല്യം(V3_Pre_വ്യഞ്ജകം_Token{tok.fileline(), !lhs.മൂല്യം()});
            നിർത്തുക;
        }
        അവസ്ഥ V3_Pre_വ്യഞ്ജകം_Token::LAND: {
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token rhs = എടുക്കുക_മൂല്യം();
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token lhs = എടുക്കുക_മൂല്യം();
            തള്ളുക_മൂല്യം(V3_Pre_വ്യഞ്ജകം_Token{tok.fileline(), lhs.മൂല്യം() && rhs.മൂല്യം()});
            നിർത്തുക;
        }
        അവസ്ഥ V3_Pre_വ്യഞ്ജകം_Token::LOR: {
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token rhs = എടുക്കുക_മൂല്യം();
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token lhs = എടുക്കുക_മൂല്യം();
            തള്ളുക_മൂല്യം(V3_Pre_വ്യഞ്ജകം_Token{tok.fileline(), lhs.മൂല്യം() || rhs.മൂല്യം()});
            നിർത്തുക;
        }
        അവസ്ഥ V3_Pre_വ്യഞ്ജകം_Token::IMP: {
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token rhs = എടുക്കുക_മൂല്യം();
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token lhs = എടുക്കുക_മൂല്യം();
            തള്ളുക_മൂല്യം(V3_Pre_വ്യഞ്ജകം_Token{tok.fileline(), !lhs.മൂല്യം() || rhs.മൂല്യം()});
            നിർത്തുക;
        }
        അവസ്ഥ V3_Pre_വ്യഞ്ജകം_Token::EQV: {
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token rhs = എടുക്കുക_മൂല്യം();
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token lhs = എടുക്കുക_മൂല്യം();
            തള്ളുക_മൂല്യം(V3_Pre_വ്യഞ്ജകം_Token{tok.fileline(), lhs.മൂല്യം() == rhs.മൂല്യം()});
            നിർത്തുക;
        }
        സ്ഥിരസ്ഥിതി: {
            v3fatalSrc("bad case on operand stack");
            നിർത്തുക;
        }
        }
    }
    ശൂന്യം parse() {
        ആകുമ്പോൾ (!m_inputs.empty()) {
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token tok = m_inputs.front();
            m_inputs.എടുക്കുക_front();
            UINFO(9, "input read " << tok.ascii());
            എങ്കിൽ (tok.is_മൂല്യം()) {
                തള്ളുക_മൂല്യം(tok);
                തുടരുക;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token മുകൾ_Tok = m_ops.back();
            auto action = parseTable[മുകൾ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << മുകൾ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            തിരഞ്ഞെടുപ്പ് (action) {
            അവസ്ഥ RR:  // Reduce
                reduce();
                നിർത്തുക;
            അവസ്ഥ SS:  // Shift
                m_ops.തള്ളുക_back(tok);
                നിർത്തുക;
            അവസ്ഥ AA:  // Accept
                നിർത്തുക;
            സ്ഥിരസ്ഥിതി: tok.fileline()->v3error("Syntax error in `ifdef () expression"); തിരികെ;
            }
        }
    }

പൊതു:
    // METHODS
    V3_Pre_വ്യഞ്ജകം() = സ്ഥിരസ്ഥിതി;
    ~V3_Pre_വ്യഞ്ജകം() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം reset(File_വരി* flp) {
        m_inputs.clear();
        m_മൂല്യങ്ങൾ.clear();
        m_ops.clear();
        m_firstFileline = flp;
        തള്ളുക_Op(V3_Pre_വ്യഞ്ജകം_Token{flp, V3_Pre_വ്യഞ്ജകം_Token::END});
    }
    ശൂന്യം തള്ളുക_നിവേശം(സ്ഥിരം V3_Pre_വ്യഞ്ജകം_Token& token) {
        എങ്കിൽ (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.തള്ളുക_back(token);
    }
    bool result() {
        തള്ളുക_നിവേശം(V3_Pre_വ്യഞ്ജകം_Token{m_firstFileline, V3_Pre_വ്യഞ്ജകം_Token::END});
        parse();
        തിരികെ എടുക്കുക_മൂല്യം().മൂല്യം();
    }
    സ്ഥിതം ശൂന്യം selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
