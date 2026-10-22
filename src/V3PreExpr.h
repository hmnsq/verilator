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

class V3_Pre_اوڠکاڤن_Token final {
عام:
    // TYPES
    // Order of enum must match token table
    تعداد token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خاص:
    // MEMBERS
    File_باريس* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عام:
    // METHODS
    V3_Pre_اوڠکاڤن_Token(File_باريس* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_اوڠکاڤن_Token(File_باريس* fileline, bool نيلاي)
        : m_fileline{fileline}
        , m_token{نيلاي ? ONE : ZERO} {}
    ~V3_Pre_اوڠکاڤن_Token() = افتراضي;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        أرجع names[m_token];
    }
    File_باريس* fileline() ثابت { أرجع m_fileline; }
    token_t token() ثابت { أرجع m_token; }
    bool is_نيلاي() ثابت { أرجع m_token == ZERO || m_token == ONE; }
    bool نيلاي() ثابت {
        UASSERT(is_نيلاي(), "preproc expr fetch of non-value");
        أرجع m_token == ONE;
    }
};

class V3_Pre_اوڠکاڤن final {
    // MEMBERS
    std::deque<V3_Pre_اوڠکاڤن_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_اوڠکاڤن_Token> m_نيلاي_سموا;  // Value stack
    std::deque<V3_Pre_اوڠکاڤن_Token> m_ops;  // Operator stack
    File_باريس* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    تعداد action_t : uint8_t {
        VV,  //نيلاي
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

    action_t parseTable[V3_Pre_اوڠکاڤن_Token::MAX][V3_Pre_اوڠکاڤن_Token::MAX] = {
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

    ساكن فراغ selfTestOne(ثابت string& اوڠکاڤن, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << اوڠکاڤن);
        File_باريس* ثابت flp = nullptr;
        V3_Pre_اوڠکاڤن parser;
        parser.reset(flp);
        bool tstart = true;
        لكل (ثابت حرف* cp = اوڠکاڤن.c_str(); *cp; ++cp) {
            إذا (tstart) {
                tstart = false;
                بدل (*cp) {
                حالة '0': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::ZERO}); اكسر;
                حالة '1': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::ONE}); اكسر;
                حالة '!': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::LNOT}); اكسر;
                حالة '|': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::LOR}); اكسر;
                حالة '&': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::LAND}); اكسر;
                حالة '-': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::IMP}); اكسر;
                حالة '<': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::EQV}); اكسر;
                حالة '(': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::BRA}); اكسر;
                حالة ')': parser.تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::KET}); اكسر;
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
    فراغ تولق_Op(ثابت V3_Pre_اوڠکاڤن_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.تولق_back(token);
    }
    فراغ تولق_نيلاي(ثابت V3_Pre_اوڠکاڤن_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_نيلاي_سموا.تولق_back(token);
    }
    V3_Pre_اوڠکاڤن_Token کلوار_نيلاي() {
        إذا (m_نيلاي_سموا.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            أرجع V3_Pre_اوڠکاڤن_Token{m_firstFileline, false};
        }
        ثابت V3_Pre_اوڠکاڤن_Token tok = m_نيلاي_سموا.back();
        m_نيلاي_سموا.کلوار_back();
        // UINFO(9, "  popValue " << tok.ascii());
        أرجع tok;
    }
    فراغ reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ثابت V3_Pre_اوڠکاڤن_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.کلوار_back();
        بدل (tok.token()) {
        حالة V3_Pre_اوڠکاڤن_Token::KET: {
            طالما (m_ops.back().token() != V3_Pre_اوڠکاڤن_Token::END
                   && m_ops.back().token() != V3_Pre_اوڠکاڤن_Token::BRA)
                reduce();
            إذا (m_ops.back().token() == V3_Pre_اوڠکاڤن_Token::BRA) {
                m_ops.کلوار_back();
            } وإلا {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                أرجع;
            }
            اكسر;
        }
        حالة V3_Pre_اوڠکاڤن_Token::LNOT: {
            ثابت V3_Pre_اوڠکاڤن_Token lhs = کلوار_نيلاي();
            تولق_نيلاي(V3_Pre_اوڠکاڤن_Token{tok.fileline(), !lhs.نيلاي()});
            اكسر;
        }
        حالة V3_Pre_اوڠکاڤن_Token::LAND: {
            ثابت V3_Pre_اوڠکاڤن_Token rhs = کلوار_نيلاي();
            ثابت V3_Pre_اوڠکاڤن_Token lhs = کلوار_نيلاي();
            تولق_نيلاي(V3_Pre_اوڠکاڤن_Token{tok.fileline(), lhs.نيلاي() && rhs.نيلاي()});
            اكسر;
        }
        حالة V3_Pre_اوڠکاڤن_Token::LOR: {
            ثابت V3_Pre_اوڠکاڤن_Token rhs = کلوار_نيلاي();
            ثابت V3_Pre_اوڠکاڤن_Token lhs = کلوار_نيلاي();
            تولق_نيلاي(V3_Pre_اوڠکاڤن_Token{tok.fileline(), lhs.نيلاي() || rhs.نيلاي()});
            اكسر;
        }
        حالة V3_Pre_اوڠکاڤن_Token::IMP: {
            ثابت V3_Pre_اوڠکاڤن_Token rhs = کلوار_نيلاي();
            ثابت V3_Pre_اوڠکاڤن_Token lhs = کلوار_نيلاي();
            تولق_نيلاي(V3_Pre_اوڠکاڤن_Token{tok.fileline(), !lhs.نيلاي() || rhs.نيلاي()});
            اكسر;
        }
        حالة V3_Pre_اوڠکاڤن_Token::EQV: {
            ثابت V3_Pre_اوڠکاڤن_Token rhs = کلوار_نيلاي();
            ثابت V3_Pre_اوڠکاڤن_Token lhs = کلوار_نيلاي();
            تولق_نيلاي(V3_Pre_اوڠکاڤن_Token{tok.fileline(), lhs.نيلاي() == rhs.نيلاي()});
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
            ثابت V3_Pre_اوڠکاڤن_Token tok = m_inputs.front();
            m_inputs.کلوار_front();
            UINFO(9, "input read " << tok.ascii());
            إذا (tok.is_نيلاي()) {
                تولق_نيلاي(tok);
                استمر;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ثابت V3_Pre_اوڠکاڤن_Token اتس_Tok = m_ops.back();
            auto action = parseTable[اتس_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << اتس_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            بدل (action) {
            حالة RR:  // Reduce
                reduce();
                اكسر;
            حالة SS:  // Shift
                m_ops.تولق_back(tok);
                اكسر;
            حالة AA:  // Accept
                اكسر;
            افتراضي: tok.fileline()->v3error("Syntax error in `ifdef () expression"); أرجع;
            }
        }
    }

عام:
    // METHODS
    V3_Pre_اوڠکاڤن() = افتراضي;
    ~V3_Pre_اوڠکاڤن() = افتراضي;
    فراغ reset(File_باريس* flp) {
        m_inputs.clear();
        m_نيلاي_سموا.clear();
        m_ops.clear();
        m_firstFileline = flp;
        تولق_Op(V3_Pre_اوڠکاڤن_Token{flp, V3_Pre_اوڠکاڤن_Token::END});
    }
    فراغ تولق_ماسوقن(ثابت V3_Pre_اوڠکاڤن_Token& token) {
        إذا (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.تولق_back(token);
    }
    bool result() {
        تولق_ماسوقن(V3_Pre_اوڠکاڤن_Token{m_firstFileline, V3_Pre_اوڠکاڤن_Token::END});
        parse();
        أرجع کلوار_نيلاي().نيلاي();
    }
    ساكن فراغ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
