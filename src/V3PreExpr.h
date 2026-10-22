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

#اگر_تعریف_نشده VERILATOR_V3PREEXPR_H_
#تعریف_کن VERILATOR_V3PREEXPR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3FileLine.h"
#ضمن "V3Global.h"

#ضمن <deque>

class V3_Pre_عبارت_Token final {
عام:
    // TYPES
    // Order of enum must match token table
    تعداد token_t : uint8_t { ZERO, ONE, انتها_2, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خاص:
    // MEMBERS
    File_سطر* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عام:
    // METHODS
    V3_Pre_عبارت_Token(File_سطر* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_عبارت_Token(File_سطر* fileline, bool مقدار)
        : m_fileline{fileline}
        , m_token{مقدار ? ONE : ZERO} {}
    ~V3_Pre_عبارت_Token() = افتراضي;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* نامها[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        أرجع نامها[m_token];
    }
    File_سطر* fileline() ثابت { أرجع m_fileline; }
    token_t token() ثابت { أرجع m_token; }
    bool is_مقدار() ثابت { أرجع m_token == ZERO || m_token == ONE; }
    bool مقدار() ثابت {
        UASSERT(is_مقدار(), "preproc expr fetch of non-value");
        أرجع m_token == ONE;
    }
};

class V3_Pre_عبارت final {
    // MEMBERS
    std::deque<V3_Pre_عبارت_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_عبارت_Token> m_مقادیر;  // Value stack
    std::deque<V3_Pre_عبارت_Token> m_ops;  // Operator stack
    File_سطر* m_اولی_Fileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    تعداد action_t : uint8_t {
        VV,  //مقدار
        AA,  // Accept
        RR,  // Reduce
        SS,  //لغزش
        EE  // Error
    };
    ساكن ثابت حرف* actionAscii(action_t en) {
        ساكن constexpr ثابت حرف* نامها[] = {"VV", "AA", "RR", "SS", "EE"};
        أرجع نامها[en];
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

    action_t parse_جدول[V3_Pre_عبارت_Token::MAX][V3_Pre_عبارت_Token::MAX] = {
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

    ساكن فراغ selfTestOne(ثابت string& عبارت, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << عبارت);
        File_سطر* ثابت flp = nullptr;
        V3_Pre_عبارت parser;
        parser.reset(flp);
        bool tstart = true;
        لكل (ثابت حرف* cp = عبارت.c_str(); *cp; ++cp) {
            إذا (tstart) {
                tstart = false;
                بدل (*cp) {
                حالة '0': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::ZERO}); اكسر;
                حالة '1': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::ONE}); اكسر;
                حالة '!': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LNOT}); اكسر;
                حالة '|': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LOR}); اكسر;
                حالة '&': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LAND}); اكسر;
                حالة '-': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::IMP}); اكسر;
                حالة '<': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::EQV}); اكسر;
                حالة '(': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::BRA}); اكسر;
                حالة ')': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::KET}); اكسر;
                افتراضي: اكسر;
                }
            } وإلا إذا (*cp == ' ') {
                tstart = true;
            }
        }
        ثابت bool got = parser.نتیجه();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    فراغ افزودن_Op(ثابت V3_Pre_عبارت_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.افزودن_پشت(token);
    }
    فراغ افزودن_مقدار(ثابت V3_Pre_عبارت_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_مقادیر.افزودن_پشت(token);
    }
    V3_Pre_عبارت_Token برداشت_مقدار() {
        إذا (m_مقادیر.خالی()) {
            m_اولی_Fileline->v3error("Syntax error in `ifdef () expression");
            أرجع V3_Pre_عبارت_Token{m_اولی_Fileline, false};
        }
        ثابت V3_Pre_عبارت_Token tok = m_مقادیر.پشت();
        m_مقادیر.برداشت_پشت();
        // UINFO(9, "  popValue " << tok.ascii());
        أرجع tok;
    }
    فراغ reduce() {
        UASSERT(!m_ops.خالی(), "lost op stack beginning END");
        ثابت V3_Pre_عبارت_Token tok = m_ops.پشت();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.برداشت_پشت();
        بدل (tok.token()) {
        حالة V3_Pre_عبارت_Token::KET: {
            طالما (m_ops.پشت().token() != V3_Pre_عبارت_Token::انتها_2
                   && m_ops.پشت().token() != V3_Pre_عبارت_Token::BRA)
                reduce();
            إذا (m_ops.پشت().token() == V3_Pre_عبارت_Token::BRA) {
                m_ops.برداشت_پشت();
            } وإلا {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                أرجع;
            }
            اكسر;
        }
        حالة V3_Pre_عبارت_Token::LNOT: {
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), !lhs.مقدار()});
            اكسر;
        }
        حالة V3_Pre_عبارت_Token::LAND: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), lhs.مقدار() && rhs.مقدار()});
            اكسر;
        }
        حالة V3_Pre_عبارت_Token::LOR: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), lhs.مقدار() || rhs.مقدار()});
            اكسر;
        }
        حالة V3_Pre_عبارت_Token::IMP: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), !lhs.مقدار() || rhs.مقدار()});
            اكسر;
        }
        حالة V3_Pre_عبارت_Token::EQV: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), lhs.مقدار() == rhs.مقدار()});
            اكسر;
        }
        افتراضي: {
            v3fatalSrc("bad case on operand stack");
            اكسر;
        }
        }
    }
    فراغ parse() {
        طالما (!m_inputs.خالی()) {
            ثابت V3_Pre_عبارت_Token tok = m_inputs.front();
            m_inputs.برداشت_front();
            UINFO(9, "input read " << tok.ascii());
            إذا (tok.is_مقدار()) {
                افزودن_مقدار(tok);
                استمر;
            }

            UASSERT(!m_ops.خالی(), "lost op stack beginning END");
            ثابت V3_Pre_عبارت_Token بالاترین_Tok = m_ops.پشت();
            خودکار action = parse_جدول[بالاترین_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << بالاترین_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            بدل (action) {
            حالة RR:  // Reduce
                reduce();
                اكسر;
            حالة SS:  //لغزش
                m_ops.افزودن_پشت(tok);
                اكسر;
            حالة AA:  // Accept
                اكسر;
            افتراضي: tok.fileline()->v3error("Syntax error in `ifdef () expression"); أرجع;
            }
        }
    }

عام:
    // METHODS
    V3_Pre_عبارت() = افتراضي;
    ~V3_Pre_عبارت() = افتراضي;
    فراغ reset(File_سطر* flp) {
        m_inputs.clear();
        m_مقادیر.clear();
        m_ops.clear();
        m_اولی_Fileline = flp;
        افزودن_Op(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::انتها_2});
    }
    فراغ افزودن_درون_داد(ثابت V3_Pre_عبارت_Token& token) {
        إذا (!m_اولی_Fileline) m_اولی_Fileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.افزودن_پشت(token);
    }
    bool نتیجه() {
        افزودن_درون_داد(V3_Pre_عبارت_Token{m_اولی_Fileline, V3_Pre_عبارت_Token::انتها_2});
        parse();
        أرجع برداشت_مقدار().مقدار();
    }
    ساكن فراغ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#پایان_اگر  // Guard
