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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Global.h"

#अन्तर्भाव <deque>

class V3_Pre_अभिव्यक्ति_Token final {
सार्वजनिक:
    // TYPES
    // Order of enum must match token table
    गणना token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

गुप्त:
    // MEMBERS
    File_पङ्क्ति* अचर m_fileline;  // Token fileline
    token_t अचर m_token;  // Token value
सार्वजनिक:
    // METHODS
    V3_Pre_अभिव्यक्ति_Token(File_पङ्क्ति* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_अभिव्यक्ति_Token(File_पङ्क्ति* fileline, bool मान)
        : m_fileline{fileline}
        , m_token{मान ? ONE : ZERO} {}
    ~V3_Pre_अभिव्यक्ति_Token() = स्वभाव;
    अचर वर्ण* ascii() अचर {
        स्थिर अचर वर्ण* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        फर्काऊ names[m_token];
    }
    File_पङ्क्ति* fileline() अचर { फर्काऊ m_fileline; }
    token_t token() अचर { फर्काऊ m_token; }
    bool is_मान() अचर { फर्काऊ m_token == ZERO || m_token == ONE; }
    bool मान() अचर {
        UASSERT(is_मान(), "preproc expr fetch of non-value");
        फर्काऊ m_token == ONE;
    }
};

class V3_Pre_अभिव्यक्ति final {
    // MEMBERS
    std::deque<V3_Pre_अभिव्यक्ति_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_अभिव्यक्ति_Token> m_मानहरू;  // Value stack
    std::deque<V3_Pre_अभिव्यक्ति_Token> m_ops;  // Operator stack
    File_पङ्क्ति* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    गणना action_t : uint8_t {
        VV,  //मान
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    स्थिर अचर वर्ण* actionAscii(action_t en) {
        स्थिर constexpr अचर वर्ण* names[] = {"VV", "AA", "RR", "SS", "EE"};
        फर्काऊ names[en];
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

    action_t parseTable[V3_Pre_अभिव्यक्ति_Token::MAX][V3_Pre_अभिव्यक्ति_Token::MAX] = {
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

    स्थिर शून्य selfTestImp() {
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

    स्थिर शून्य selfTestOne(अचर string& अभिव्यक्ति, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << अभिव्यक्ति);
        File_पङ्क्ति* अचर flp = nullptr;
        V3_Pre_अभिव्यक्ति parser;
        parser.reset(flp);
        bool tstart = true;
        लागि (अचर वर्ण* cp = अभिव्यक्ति.c_str(); *cp; ++cp) {
            यदि (tstart) {
                tstart = false;
                विकल्प (*cp) {
                पक्ष '0': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::ZERO}); रोक;
                पक्ष '1': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::ONE}); रोक;
                पक्ष '!': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::LNOT}); रोक;
                पक्ष '|': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::LOR}); रोक;
                पक्ष '&': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::LAND}); रोक;
                पक्ष '-': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::IMP}); रोक;
                पक्ष '<': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::EQV}); रोक;
                पक्ष '(': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::BRA}); रोक;
                पक्ष ')': parser.थप_आगत(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::KET}); रोक;
                स्वभाव: रोक;
                }
            } अन्यथा यदि (*cp == ' ') {
                tstart = true;
            }
        }
        अचर bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    शून्य थप_Op(अचर V3_Pre_अभिव्यक्ति_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.थप_back(token);
    }
    शून्य थप_मान(अचर V3_Pre_अभिव्यक्ति_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_मानहरू.थप_back(token);
    }
    V3_Pre_अभिव्यक्ति_Token झिक_मान() {
        यदि (m_मानहरू.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            फर्काऊ V3_Pre_अभिव्यक्ति_Token{m_firstFileline, false};
        }
        अचर V3_Pre_अभिव्यक्ति_Token tok = m_मानहरू.back();
        m_मानहरू.झिक_back();
        // UINFO(9, "  popValue " << tok.ascii());
        फर्काऊ tok;
    }
    शून्य reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        अचर V3_Pre_अभिव्यक्ति_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.झिक_back();
        विकल्प (tok.token()) {
        पक्ष V3_Pre_अभिव्यक्ति_Token::KET: {
            जबसम्म (m_ops.back().token() != V3_Pre_अभिव्यक्ति_Token::END
                   && m_ops.back().token() != V3_Pre_अभिव्यक्ति_Token::BRA)
                reduce();
            यदि (m_ops.back().token() == V3_Pre_अभिव्यक्ति_Token::BRA) {
                m_ops.झिक_back();
            } अन्यथा {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                फर्काऊ;
            }
            रोक;
        }
        पक्ष V3_Pre_अभिव्यक्ति_Token::LNOT: {
            अचर V3_Pre_अभिव्यक्ति_Token lhs = झिक_मान();
            थप_मान(V3_Pre_अभिव्यक्ति_Token{tok.fileline(), !lhs.मान()});
            रोक;
        }
        पक्ष V3_Pre_अभिव्यक्ति_Token::LAND: {
            अचर V3_Pre_अभिव्यक्ति_Token rhs = झिक_मान();
            अचर V3_Pre_अभिव्यक्ति_Token lhs = झिक_मान();
            थप_मान(V3_Pre_अभिव्यक्ति_Token{tok.fileline(), lhs.मान() && rhs.मान()});
            रोक;
        }
        पक्ष V3_Pre_अभिव्यक्ति_Token::LOR: {
            अचर V3_Pre_अभिव्यक्ति_Token rhs = झिक_मान();
            अचर V3_Pre_अभिव्यक्ति_Token lhs = झिक_मान();
            थप_मान(V3_Pre_अभिव्यक्ति_Token{tok.fileline(), lhs.मान() || rhs.मान()});
            रोक;
        }
        पक्ष V3_Pre_अभिव्यक्ति_Token::IMP: {
            अचर V3_Pre_अभिव्यक्ति_Token rhs = झिक_मान();
            अचर V3_Pre_अभिव्यक्ति_Token lhs = झिक_मान();
            थप_मान(V3_Pre_अभिव्यक्ति_Token{tok.fileline(), !lhs.मान() || rhs.मान()});
            रोक;
        }
        पक्ष V3_Pre_अभिव्यक्ति_Token::EQV: {
            अचर V3_Pre_अभिव्यक्ति_Token rhs = झिक_मान();
            अचर V3_Pre_अभिव्यक्ति_Token lhs = झिक_मान();
            थप_मान(V3_Pre_अभिव्यक्ति_Token{tok.fileline(), lhs.मान() == rhs.मान()});
            रोक;
        }
        स्वभाव: {
            v3fatalSrc("bad case on operand stack");
            रोक;
        }
        }
    }
    शून्य parse() {
        जबसम्म (!m_inputs.empty()) {
            अचर V3_Pre_अभिव्यक्ति_Token tok = m_inputs.front();
            m_inputs.झिक_front();
            UINFO(9, "input read " << tok.ascii());
            यदि (tok.is_मान()) {
                थप_मान(tok);
                जारी;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            अचर V3_Pre_अभिव्यक्ति_Token शीर्ष_Tok = m_ops.back();
            स्वतः action = parseTable[शीर्ष_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << शीर्ष_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            विकल्प (action) {
            पक्ष RR:  // Reduce
                reduce();
                रोक;
            पक्ष SS:  // Shift
                m_ops.थप_back(tok);
                रोक;
            पक्ष AA:  // Accept
                रोक;
            स्वभाव: tok.fileline()->v3error("Syntax error in `ifdef () expression"); फर्काऊ;
            }
        }
    }

सार्वजनिक:
    // METHODS
    V3_Pre_अभिव्यक्ति() = स्वभाव;
    ~V3_Pre_अभिव्यक्ति() = स्वभाव;
    शून्य reset(File_पङ्क्ति* flp) {
        m_inputs.clear();
        m_मानहरू.clear();
        m_ops.clear();
        m_firstFileline = flp;
        थप_Op(V3_Pre_अभिव्यक्ति_Token{flp, V3_Pre_अभिव्यक्ति_Token::END});
    }
    शून्य थप_आगत(अचर V3_Pre_अभिव्यक्ति_Token& token) {
        यदि (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.थप_back(token);
    }
    bool result() {
        थप_आगत(V3_Pre_अभिव्यक्ति_Token{m_firstFileline, V3_Pre_अभिव्यक्ति_Token::END});
        parse();
        फर्काऊ झिक_मान().मान();
    }
    स्थिर शून्य selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
