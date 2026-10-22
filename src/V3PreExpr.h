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

class V3_Pre_अभिव्यक्ती_Token final {
सार्वजनिक:
    // TYPES
    // Order of enum must match token table
    गणना token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

खाजगी:
    // MEMBERS
    File_ओळ* स्थिरांक m_fileline;  // Token fileline
    token_t स्थिरांक m_token;  // Token value
सार्वजनिक:
    // METHODS
    V3_Pre_अभिव्यक्ती_Token(File_ओळ* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_अभिव्यक्ती_Token(File_ओळ* fileline, bool मूल्य)
        : m_fileline{fileline}
        , m_token{मूल्य ? ONE : ZERO} {}
    ~V3_Pre_अभिव्यक्ती_Token() = पूर्वनिर्धारित;
    स्थिरांक अक्षर* ascii() स्थिरांक {
        स्थिर स्थिरांक अक्षर* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        परत names[m_token];
    }
    File_ओळ* fileline() स्थिरांक { परत m_fileline; }
    token_t token() स्थिरांक { परत m_token; }
    bool is_मूल्य() स्थिरांक { परत m_token == ZERO || m_token == ONE; }
    bool मूल्य() स्थिरांक {
        UASSERT(is_मूल्य(), "preproc expr fetch of non-value");
        परत m_token == ONE;
    }
};

class V3_Pre_अभिव्यक्ती final {
    // MEMBERS
    std::deque<V3_Pre_अभिव्यक्ती_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_अभिव्यक्ती_Token> m_मूल्ये;  // Value stack
    std::deque<V3_Pre_अभिव्यक्ती_Token> m_ops;  // Operator stack
    File_ओळ* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    गणना action_t : uint8_t {
        VV,  //मूल्य
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    स्थिर स्थिरांक अक्षर* actionAscii(action_t en) {
        स्थिर constexpr स्थिरांक अक्षर* names[] = {"VV", "AA", "RR", "SS", "EE"};
        परत names[en];
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

    action_t parseTable[V3_Pre_अभिव्यक्ती_Token::MAX][V3_Pre_अभिव्यक्ती_Token::MAX] = {
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

    स्थिर शून्य selfTestOne(स्थिरांक string& अभिव्यक्ती, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << अभिव्यक्ती);
        File_ओळ* स्थिरांक flp = nullptr;
        V3_Pre_अभिव्यक्ती parser;
        parser.reset(flp);
        bool tstart = true;
        साठी (स्थिरांक अक्षर* cp = अभिव्यक्ती.c_str(); *cp; ++cp) {
            जर (tstart) {
                tstart = false;
                निवड (*cp) {
                स्थिती '0': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::ZERO}); थांबा;
                स्थिती '1': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::ONE}); थांबा;
                स्थिती '!': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::LNOT}); थांबा;
                स्थिती '|': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::LOR}); थांबा;
                स्थिती '&': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::LAND}); थांबा;
                स्थिती '-': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::IMP}); थांबा;
                स्थिती '<': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::EQV}); थांबा;
                स्थिती '(': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::BRA}); थांबा;
                स्थिती ')': parser.ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::KET}); थांबा;
                पूर्वनिर्धारित: थांबा;
                }
            } अन्यथा जर (*cp == ' ') {
                tstart = true;
            }
        }
        स्थिरांक bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    शून्य ढकला_Op(स्थिरांक V3_Pre_अभिव्यक्ती_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ढकला_back(token);
    }
    शून्य ढकला_मूल्य(स्थिरांक V3_Pre_अभिव्यक्ती_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_मूल्ये.ढकला_back(token);
    }
    V3_Pre_अभिव्यक्ती_Token काढा_मूल्य() {
        जर (m_मूल्ये.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            परत V3_Pre_अभिव्यक्ती_Token{m_firstFileline, false};
        }
        स्थिरांक V3_Pre_अभिव्यक्ती_Token tok = m_मूल्ये.back();
        m_मूल्ये.काढा_back();
        // UINFO(9, "  popValue " << tok.ascii());
        परत tok;
    }
    शून्य reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        स्थिरांक V3_Pre_अभिव्यक्ती_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.काढा_back();
        निवड (tok.token()) {
        स्थिती V3_Pre_अभिव्यक्ती_Token::KET: {
            पर्यंत (m_ops.back().token() != V3_Pre_अभिव्यक्ती_Token::END
                   && m_ops.back().token() != V3_Pre_अभिव्यक्ती_Token::BRA)
                reduce();
            जर (m_ops.back().token() == V3_Pre_अभिव्यक्ती_Token::BRA) {
                m_ops.काढा_back();
            } अन्यथा {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                परत;
            }
            थांबा;
        }
        स्थिती V3_Pre_अभिव्यक्ती_Token::LNOT: {
            स्थिरांक V3_Pre_अभिव्यक्ती_Token lhs = काढा_मूल्य();
            ढकला_मूल्य(V3_Pre_अभिव्यक्ती_Token{tok.fileline(), !lhs.मूल्य()});
            थांबा;
        }
        स्थिती V3_Pre_अभिव्यक्ती_Token::LAND: {
            स्थिरांक V3_Pre_अभिव्यक्ती_Token rhs = काढा_मूल्य();
            स्थिरांक V3_Pre_अभिव्यक्ती_Token lhs = काढा_मूल्य();
            ढकला_मूल्य(V3_Pre_अभिव्यक्ती_Token{tok.fileline(), lhs.मूल्य() && rhs.मूल्य()});
            थांबा;
        }
        स्थिती V3_Pre_अभिव्यक्ती_Token::LOR: {
            स्थिरांक V3_Pre_अभिव्यक्ती_Token rhs = काढा_मूल्य();
            स्थिरांक V3_Pre_अभिव्यक्ती_Token lhs = काढा_मूल्य();
            ढकला_मूल्य(V3_Pre_अभिव्यक्ती_Token{tok.fileline(), lhs.मूल्य() || rhs.मूल्य()});
            थांबा;
        }
        स्थिती V3_Pre_अभिव्यक्ती_Token::IMP: {
            स्थिरांक V3_Pre_अभिव्यक्ती_Token rhs = काढा_मूल्य();
            स्थिरांक V3_Pre_अभिव्यक्ती_Token lhs = काढा_मूल्य();
            ढकला_मूल्य(V3_Pre_अभिव्यक्ती_Token{tok.fileline(), !lhs.मूल्य() || rhs.मूल्य()});
            थांबा;
        }
        स्थिती V3_Pre_अभिव्यक्ती_Token::EQV: {
            स्थिरांक V3_Pre_अभिव्यक्ती_Token rhs = काढा_मूल्य();
            स्थिरांक V3_Pre_अभिव्यक्ती_Token lhs = काढा_मूल्य();
            ढकला_मूल्य(V3_Pre_अभिव्यक्ती_Token{tok.fileline(), lhs.मूल्य() == rhs.मूल्य()});
            थांबा;
        }
        पूर्वनिर्धारित: {
            v3fatalSrc("bad case on operand stack");
            थांबा;
        }
        }
    }
    शून्य parse() {
        पर्यंत (!m_inputs.empty()) {
            स्थिरांक V3_Pre_अभिव्यक्ती_Token tok = m_inputs.front();
            m_inputs.काढा_front();
            UINFO(9, "input read " << tok.ascii());
            जर (tok.is_मूल्य()) {
                ढकला_मूल्य(tok);
                सुरूठेवा;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            स्थिरांक V3_Pre_अभिव्यक्ती_Token शीर्ष_Tok = m_ops.back();
            स्वतः action = parseTable[शीर्ष_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << शीर्ष_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            निवड (action) {
            स्थिती RR:  // Reduce
                reduce();
                थांबा;
            स्थिती SS:  // Shift
                m_ops.ढकला_back(tok);
                थांबा;
            स्थिती AA:  // Accept
                थांबा;
            पूर्वनिर्धारित: tok.fileline()->v3error("Syntax error in `ifdef () expression"); परत;
            }
        }
    }

सार्वजनिक:
    // METHODS
    V3_Pre_अभिव्यक्ती() = पूर्वनिर्धारित;
    ~V3_Pre_अभिव्यक्ती() = पूर्वनिर्धारित;
    शून्य reset(File_ओळ* flp) {
        m_inputs.clear();
        m_मूल्ये.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ढकला_Op(V3_Pre_अभिव्यक्ती_Token{flp, V3_Pre_अभिव्यक्ती_Token::END});
    }
    शून्य ढकला_आदान(स्थिरांक V3_Pre_अभिव्यक्ती_Token& token) {
        जर (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ढकला_back(token);
    }
    bool result() {
        ढकला_आदान(V3_Pre_अभिव्यक्ती_Token{m_firstFileline, V3_Pre_अभिव्यक्ती_Token::END});
        parse();
        परत काढा_मूल्य().मूल्य();
    }
    स्थिर शून्य selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
