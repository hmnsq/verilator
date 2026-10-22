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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3FileLine.h"
#ضمن "V3Global.h"

#ضمن <deque>

class V3_Pre_دەربڕین_Token final {
عام:
    // TYPES
    // Order of enum must match token table
    تعداد token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خاص:
    // MEMBERS
    File_هێڵ* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عام:
    // METHODS
    V3_Pre_دەربڕین_Token(File_هێڵ* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_دەربڕین_Token(File_هێڵ* fileline, bool بەها)
        : m_fileline{fileline}
        , m_token{بەها ? ONE : ZERO} {}
    ~V3_Pre_دەربڕین_Token() = افتراضي;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        أرجع names[m_token];
    }
    File_هێڵ* fileline() ثابت { أرجع m_fileline; }
    token_t token() ثابت { أرجع m_token; }
    bool is_بەها() ثابت { أرجع m_token == ZERO || m_token == ONE; }
    bool بەها() ثابت {
        UASSERT(is_بەها(), "preproc expr fetch of non-value");
        أرجع m_token == ONE;
    }
};

class V3_Pre_دەربڕین final {
    // MEMBERS
    std::deque<V3_Pre_دەربڕین_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_دەربڕین_Token> m_بەهاکان;  // Value stack
    std::deque<V3_Pre_دەربڕین_Token> m_ops;  // Operator stack
    File_هێڵ* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    تعداد action_t : uint8_t {
        VV,  //بەها
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    ساكن ثابت حرف* actionAscii(action_t en) {
        ساكن constexpr ثابت حرف* names[] = {"VV", "AA", "RR", "SS", "EE"};
        أرجع names[en];
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

    action_t parseTable[V3_Pre_دەربڕین_Token::MAX][V3_Pre_دەربڕین_Token::MAX] = {
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

    ساكن فراغ selfTestImp() {
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

    ساكن فراغ selfTestOne(ثابت string& دەربڕین, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << دەربڕین);
        File_هێڵ* ثابت flp = nullptr;
        V3_Pre_دەربڕین parser;
        parser.reset(flp);
        bool tstart = true;
        لكل (ثابت حرف* cp = دەربڕین.c_str(); *cp; ++cp) {
            إذا (tstart) {
                tstart = false;
                بدل (*cp) {
                حالة '0': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::ZERO}); اكسر;
                حالة '1': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::ONE}); اكسر;
                حالة '!': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::LNOT}); اكسر;
                حالة '|': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::LOR}); اكسر;
                حالة '&': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::LAND}); اكسر;
                حالة '-': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::IMP}); اكسر;
                حالة '<': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::EQV}); اكسر;
                حالة '(': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::BRA}); اكسر;
                حالة ')': parser.پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::KET}); اكسر;
                افتراضي: اكسر;
                }
            } وإلا إذا (*cp == ' ') {
                tstart = true;
            }
        }
        ثابت bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    فراغ پاڵنان_Op(ثابت V3_Pre_دەربڕین_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.پاڵنان_back(token);
    }
    فراغ پاڵنان_بەها(ثابت V3_Pre_دەربڕین_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_بەهاکان.پاڵنان_back(token);
    }
    V3_Pre_دەربڕین_Token دەرهێنان_بەها() {
        إذا (m_بەهاکان.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            أرجع V3_Pre_دەربڕین_Token{m_firstFileline, false};
        }
        ثابت V3_Pre_دەربڕین_Token tok = m_بەهاکان.back();
        m_بەهاکان.دەرهێنان_back();
        // UINFO(9, "  popValue " << tok.ascii());
        أرجع tok;
    }
    فراغ reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ثابت V3_Pre_دەربڕین_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.دەرهێنان_back();
        بدل (tok.token()) {
        حالة V3_Pre_دەربڕین_Token::KET: {
            طالما (m_ops.back().token() != V3_Pre_دەربڕین_Token::END
                   && m_ops.back().token() != V3_Pre_دەربڕین_Token::BRA)
                reduce();
            إذا (m_ops.back().token() == V3_Pre_دەربڕین_Token::BRA) {
                m_ops.دەرهێنان_back();
            } وإلا {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                أرجع;
            }
            اكسر;
        }
        حالة V3_Pre_دەربڕین_Token::LNOT: {
            ثابت V3_Pre_دەربڕین_Token lhs = دەرهێنان_بەها();
            پاڵنان_بەها(V3_Pre_دەربڕین_Token{tok.fileline(), !lhs.بەها()});
            اكسر;
        }
        حالة V3_Pre_دەربڕین_Token::LAND: {
            ثابت V3_Pre_دەربڕین_Token rhs = دەرهێنان_بەها();
            ثابت V3_Pre_دەربڕین_Token lhs = دەرهێنان_بەها();
            پاڵنان_بەها(V3_Pre_دەربڕین_Token{tok.fileline(), lhs.بەها() && rhs.بەها()});
            اكسر;
        }
        حالة V3_Pre_دەربڕین_Token::LOR: {
            ثابت V3_Pre_دەربڕین_Token rhs = دەرهێنان_بەها();
            ثابت V3_Pre_دەربڕین_Token lhs = دەرهێنان_بەها();
            پاڵنان_بەها(V3_Pre_دەربڕین_Token{tok.fileline(), lhs.بەها() || rhs.بەها()});
            اكسر;
        }
        حالة V3_Pre_دەربڕین_Token::IMP: {
            ثابت V3_Pre_دەربڕین_Token rhs = دەرهێنان_بەها();
            ثابت V3_Pre_دەربڕین_Token lhs = دەرهێنان_بەها();
            پاڵنان_بەها(V3_Pre_دەربڕین_Token{tok.fileline(), !lhs.بەها() || rhs.بەها()});
            اكسر;
        }
        حالة V3_Pre_دەربڕین_Token::EQV: {
            ثابت V3_Pre_دەربڕین_Token rhs = دەرهێنان_بەها();
            ثابت V3_Pre_دەربڕین_Token lhs = دەرهێنان_بەها();
            پاڵنان_بەها(V3_Pre_دەربڕین_Token{tok.fileline(), lhs.بەها() == rhs.بەها()});
            اكسر;
        }
        افتراضي: {
            v3fatalSrc("bad case on operand stack");
            اكسر;
        }
        }
    }
    فراغ parse() {
        طالما (!m_inputs.empty()) {
            ثابت V3_Pre_دەربڕین_Token tok = m_inputs.front();
            m_inputs.دەرهێنان_front();
            UINFO(9, "input read " << tok.ascii());
            إذا (tok.is_بەها()) {
                پاڵنان_بەها(tok);
                استمر;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ثابت V3_Pre_دەربڕین_Token لووتکە_Tok = m_ops.back();
            auto action = parseTable[لووتکە_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << لووتکە_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            بدل (action) {
            حالة RR:  // Reduce
                reduce();
                اكسر;
            حالة SS:  // Shift
                m_ops.پاڵنان_back(tok);
                اكسر;
            حالة AA:  // Accept
                اكسر;
            افتراضي: tok.fileline()->v3error("Syntax error in `ifdef () expression"); أرجع;
            }
        }
    }

عام:
    // METHODS
    V3_Pre_دەربڕین() = افتراضي;
    ~V3_Pre_دەربڕین() = افتراضي;
    فراغ reset(File_هێڵ* flp) {
        m_inputs.clear();
        m_بەهاکان.clear();
        m_ops.clear();
        m_firstFileline = flp;
        پاڵنان_Op(V3_Pre_دەربڕین_Token{flp, V3_Pre_دەربڕین_Token::END});
    }
    فراغ پاڵنان_تێخستن(ثابت V3_Pre_دەربڕین_Token& token) {
        إذا (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.پاڵنان_back(token);
    }
    bool result() {
        پاڵنان_تێخستن(V3_Pre_دەربڕین_Token{m_firstFileline, V3_Pre_دەربڕین_Token::END});
        parse();
        أرجع دەرهێنان_بەها().بەها();
    }
    ساكن فراغ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
