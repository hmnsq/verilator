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

class V3_Pre_تعبير_Token final {
عام:
    // TYPES
    // Order of enum must match token table
    تعداد token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خاص:
    // MEMBERS
    File_سطر* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عام:
    // METHODS
    V3_Pre_تعبير_Token(File_سطر* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_تعبير_Token(File_سطر* fileline, bool قيمة)
        : m_fileline{fileline}
        , m_token{قيمة ? ONE : ZERO} {}
    ~V3_Pre_تعبير_Token() = افتراضي;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        إرجاع names[m_token];
    }
    File_سطر* fileline() ثابت { إرجاع m_fileline; }
    token_t token() ثابت { إرجاع m_token; }
    bool is_قيمة() ثابت { إرجاع m_token == ZERO || m_token == ONE; }
    bool قيمة() ثابت {
        UASSERT(is_قيمة(), "preproc expr fetch of non-value");
        إرجاع m_token == ONE;
    }
};

class V3_Pre_تعبير final {
    // MEMBERS
    std::deque<V3_Pre_تعبير_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_تعبير_Token> m_قيم;  // Value stack
    std::deque<V3_Pre_تعبير_Token> m_ops;  // Operator stack
    File_سطر* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    تعداد action_t : uint8_t {
        VV,  //قيمة
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    ساكن ثابت حرف* actionAscii(action_t en) {
        ساكن constexpr ثابت حرف* names[] = {"VV", "AA", "RR", "SS", "EE"};
        إرجاع names[en];
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

    action_t parseTable[V3_Pre_تعبير_Token::MAX][V3_Pre_تعبير_Token::MAX] = {
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

    ساكن فراغ selfTestOne(ثابت string& تعبير, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << تعبير);
        File_سطر* ثابت flp = nullptr;
        V3_Pre_تعبير parser;
        parser.reset(flp);
        bool tstart = true;
        لكل (ثابت حرف* cp = تعبير.c_str(); *cp; ++cp) {
            إذا (tstart) {
                tstart = false;
                بدل (*cp) {
                حالة '0': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::ZERO}); اكسر;
                حالة '1': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::ONE}); اكسر;
                حالة '!': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::LNOT}); اكسر;
                حالة '|': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::LOR}); اكسر;
                حالة '&': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::LAND}); اكسر;
                حالة '-': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::IMP}); اكسر;
                حالة '<': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::EQV}); اكسر;
                حالة '(': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::BRA}); اكسر;
                حالة ')': parser.ادفع_إدخال(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::KET}); اكسر;
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
    فراغ ادفع_Op(ثابت V3_Pre_تعبير_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ادفع_back(token);
    }
    فراغ ادفع_قيمة(ثابت V3_Pre_تعبير_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_قيم.ادفع_back(token);
    }
    V3_Pre_تعبير_Token اسحب_قيمة() {
        إذا (m_قيم.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            إرجاع V3_Pre_تعبير_Token{m_firstFileline, false};
        }
        ثابت V3_Pre_تعبير_Token tok = m_قيم.back();
        m_قيم.اسحب_back();
        // UINFO(9, "  popValue " << tok.ascii());
        إرجاع tok;
    }
    فراغ reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ثابت V3_Pre_تعبير_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.اسحب_back();
        بدل (tok.token()) {
        حالة V3_Pre_تعبير_Token::KET: {
            بينما (m_ops.back().token() != V3_Pre_تعبير_Token::END
                   && m_ops.back().token() != V3_Pre_تعبير_Token::BRA)
                reduce();
            إذا (m_ops.back().token() == V3_Pre_تعبير_Token::BRA) {
                m_ops.اسحب_back();
            } وإلا {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                إرجاع;
            }
            اكسر;
        }
        حالة V3_Pre_تعبير_Token::LNOT: {
            ثابت V3_Pre_تعبير_Token lhs = اسحب_قيمة();
            ادفع_قيمة(V3_Pre_تعبير_Token{tok.fileline(), !lhs.قيمة()});
            اكسر;
        }
        حالة V3_Pre_تعبير_Token::LAND: {
            ثابت V3_Pre_تعبير_Token rhs = اسحب_قيمة();
            ثابت V3_Pre_تعبير_Token lhs = اسحب_قيمة();
            ادفع_قيمة(V3_Pre_تعبير_Token{tok.fileline(), lhs.قيمة() && rhs.قيمة()});
            اكسر;
        }
        حالة V3_Pre_تعبير_Token::LOR: {
            ثابت V3_Pre_تعبير_Token rhs = اسحب_قيمة();
            ثابت V3_Pre_تعبير_Token lhs = اسحب_قيمة();
            ادفع_قيمة(V3_Pre_تعبير_Token{tok.fileline(), lhs.قيمة() || rhs.قيمة()});
            اكسر;
        }
        حالة V3_Pre_تعبير_Token::IMP: {
            ثابت V3_Pre_تعبير_Token rhs = اسحب_قيمة();
            ثابت V3_Pre_تعبير_Token lhs = اسحب_قيمة();
            ادفع_قيمة(V3_Pre_تعبير_Token{tok.fileline(), !lhs.قيمة() || rhs.قيمة()});
            اكسر;
        }
        حالة V3_Pre_تعبير_Token::EQV: {
            ثابت V3_Pre_تعبير_Token rhs = اسحب_قيمة();
            ثابت V3_Pre_تعبير_Token lhs = اسحب_قيمة();
            ادفع_قيمة(V3_Pre_تعبير_Token{tok.fileline(), lhs.قيمة() == rhs.قيمة()});
            اكسر;
        }
        افتراضي: {
            v3fatalSrc("bad case on operand stack");
            اكسر;
        }
        }
    }
    فراغ parse() {
        بينما (!m_inputs.empty()) {
            ثابت V3_Pre_تعبير_Token tok = m_inputs.front();
            m_inputs.اسحب_front();
            UINFO(9, "input read " << tok.ascii());
            إذا (tok.is_قيمة()) {
                ادفع_قيمة(tok);
                استمر;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ثابت V3_Pre_تعبير_Token قمة_Tok = m_ops.back();
            auto action = parseTable[قمة_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << قمة_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            بدل (action) {
            حالة RR:  // Reduce
                reduce();
                اكسر;
            حالة SS:  // Shift
                m_ops.ادفع_back(tok);
                اكسر;
            حالة AA:  // Accept
                اكسر;
            افتراضي: tok.fileline()->v3error("Syntax error in `ifdef () expression"); إرجاع;
            }
        }
    }

عام:
    // METHODS
    V3_Pre_تعبير() = افتراضي;
    ~V3_Pre_تعبير() = افتراضي;
    فراغ reset(File_سطر* flp) {
        m_inputs.clear();
        m_قيم.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ادفع_Op(V3_Pre_تعبير_Token{flp, V3_Pre_تعبير_Token::END});
    }
    فراغ ادفع_إدخال(ثابت V3_Pre_تعبير_Token& token) {
        إذا (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ادفع_back(token);
    }
    bool result() {
        ادفع_إدخال(V3_Pre_تعبير_Token{m_firstFileline, V3_Pre_تعبير_Token::END});
        parse();
        إرجاع اسحب_قيمة().قيمة();
    }
    ساكن فراغ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
