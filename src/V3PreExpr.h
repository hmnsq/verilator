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

#اگر_غیر_تعریف_شدہ VERILATOR_V3PREEXPR_H_
#تعریف_کرو VERILATOR_V3PREEXPR_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3FileLine.h"
#شامل "V3Global.h"

#شامل <deque>

class V3_Pre_اظہاریہ_Token final {
عوامی:
    // TYPES
    // Order of enum must match token table
    شمار token_t : uint8_t { ZERO, ONE, انتہا_2, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

نجی:
    // MEMBERS
    File_سطر* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عوامی:
    // METHODS
    V3_Pre_اظہاریہ_Token(File_سطر* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_اظہاریہ_Token(File_سطر* fileline, bool قدر)
        : m_fileline{fileline}
        , m_token{قدر ? ONE : ZERO} {}
    ~V3_Pre_اظہاریہ_Token() = طےشدہ;
    ثابت حرف* ascii() ثابت {
        ساکن ثابت حرف* نام[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        واپس نام[m_token];
    }
    File_سطر* fileline() ثابت { واپس m_fileline; }
    token_t token() ثابت { واپس m_token; }
    bool is_قدر() ثابت { واپس m_token == ZERO || m_token == ONE; }
    bool قدر() ثابت {
        UASSERT(is_قدر(), "preproc expr fetch of non-value");
        واپس m_token == ONE;
    }
};

class V3_Pre_اظہاریہ final {
    // MEMBERS
    std::deque<V3_Pre_اظہاریہ_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_اظہاریہ_Token> m_اقدار;  // Value stack
    std::deque<V3_Pre_اظہاریہ_Token> m_ops;  // Operator stack
    File_سطر* m_پہلا_Fileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    شمار action_t : uint8_t {
        VV,  //قدر
        AA,  // Accept
        RR,  // Reduce
        SS,  //سرکاؤ
        EE  // Error
    };
    ساکن ثابت حرف* actionAscii(action_t en) {
        ساکن constexpr ثابت حرف* نام[] = {"VV", "AA", "RR", "SS", "EE"};
        واپس نام[en];
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

    action_t parse_جدول[V3_Pre_اظہاریہ_Token::MAX][V3_Pre_اظہاریہ_Token::MAX] = {
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

    ساکن خالی selfTestImp() {
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

    ساکن خالی selfTestOne(ثابت string& اظہاریہ, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << اظہاریہ);
        File_سطر* ثابت flp = nullptr;
        V3_Pre_اظہاریہ parser;
        parser.reset(flp);
        bool tstart = true;
        برائے (ثابت حرف* cp = اظہاریہ.c_str(); *cp; ++cp) {
            اگر (tstart) {
                tstart = false;
                انتخاب (*cp) {
                حالت '0': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::ZERO}); توڑو;
                حالت '1': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::ONE}); توڑو;
                حالت '!': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LNOT}); توڑو;
                حالت '|': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LOR}); توڑو;
                حالت '&': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::LAND}); توڑو;
                حالت '-': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::IMP}); توڑو;
                حالت '<': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::EQV}); توڑو;
                حالت '(': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::BRA}); توڑو;
                حالت ')': parser.ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::KET}); توڑو;
                طےشدہ: توڑو;
                }
            } ورنہ اگر (*cp == ' ') {
                tstart = true;
            }
        }
        ثابت bool got = parser.نتیجہ();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    خالی ڈالو_Op(ثابت V3_Pre_اظہاریہ_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ڈالو_عقب(token);
    }
    خالی ڈالو_قدر(ثابت V3_Pre_اظہاریہ_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_اقدار.ڈالو_عقب(token);
    }
    V3_Pre_اظہاریہ_Token نکالو_قدر() {
        اگر (m_اقدار.خالی_خانے()) {
            m_پہلا_Fileline->v3error("Syntax error in `ifdef () expression");
            واپس V3_Pre_اظہاریہ_Token{m_پہلا_Fileline, false};
        }
        ثابت V3_Pre_اظہاریہ_Token tok = m_اقدار.عقب();
        m_اقدار.نکالو_عقب();
        // UINFO(9, "  popValue " << tok.ascii());
        واپس tok;
    }
    خالی reduce() {
        UASSERT(!m_ops.خالی_خانے(), "lost op stack beginning END");
        ثابت V3_Pre_اظہاریہ_Token tok = m_ops.عقب();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.نکالو_عقب();
        انتخاب (tok.token()) {
        حالت V3_Pre_اظہاریہ_Token::KET: {
            جب (m_ops.عقب().token() != V3_Pre_اظہاریہ_Token::انتہا_2
                   && m_ops.عقب().token() != V3_Pre_اظہاریہ_Token::BRA)
                reduce();
            اگر (m_ops.عقب().token() == V3_Pre_اظہاریہ_Token::BRA) {
                m_ops.نکالو_عقب();
            } ورنہ {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                واپس;
            }
            توڑو;
        }
        حالت V3_Pre_اظہاریہ_Token::LNOT: {
            ثابت V3_Pre_اظہاریہ_Token lhs = نکالو_قدر();
            ڈالو_قدر(V3_Pre_اظہاریہ_Token{tok.fileline(), !lhs.قدر()});
            توڑو;
        }
        حالت V3_Pre_اظہاریہ_Token::LAND: {
            ثابت V3_Pre_اظہاریہ_Token rhs = نکالو_قدر();
            ثابت V3_Pre_اظہاریہ_Token lhs = نکالو_قدر();
            ڈالو_قدر(V3_Pre_اظہاریہ_Token{tok.fileline(), lhs.قدر() && rhs.قدر()});
            توڑو;
        }
        حالت V3_Pre_اظہاریہ_Token::LOR: {
            ثابت V3_Pre_اظہاریہ_Token rhs = نکالو_قدر();
            ثابت V3_Pre_اظہاریہ_Token lhs = نکالو_قدر();
            ڈالو_قدر(V3_Pre_اظہاریہ_Token{tok.fileline(), lhs.قدر() || rhs.قدر()});
            توڑو;
        }
        حالت V3_Pre_اظہاریہ_Token::IMP: {
            ثابت V3_Pre_اظہاریہ_Token rhs = نکالو_قدر();
            ثابت V3_Pre_اظہاریہ_Token lhs = نکالو_قدر();
            ڈالو_قدر(V3_Pre_اظہاریہ_Token{tok.fileline(), !lhs.قدر() || rhs.قدر()});
            توڑو;
        }
        حالت V3_Pre_اظہاریہ_Token::EQV: {
            ثابت V3_Pre_اظہاریہ_Token rhs = نکالو_قدر();
            ثابت V3_Pre_اظہاریہ_Token lhs = نکالو_قدر();
            ڈالو_قدر(V3_Pre_اظہاریہ_Token{tok.fileline(), lhs.قدر() == rhs.قدر()});
            توڑو;
        }
        طےشدہ: {
            v3fatalSrc("bad case on operand stack");
            توڑو;
        }
        }
    }
    خالی parse() {
        جب (!m_inputs.خالی_خانے()) {
            ثابت V3_Pre_اظہاریہ_Token tok = m_inputs.front();
            m_inputs.نکالو_front();
            UINFO(9, "input read " << tok.ascii());
            اگر (tok.is_قدر()) {
                ڈالو_قدر(tok);
                جاری;
            }

            UASSERT(!m_ops.خالی_خانے(), "lost op stack beginning END");
            ثابت V3_Pre_اظہاریہ_Token چوٹی_Tok = m_ops.عقب();
            خودکار action = parse_جدول[چوٹی_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << چوٹی_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            انتخاب (action) {
            حالت RR:  // Reduce
                reduce();
                توڑو;
            حالت SS:  //سرکاؤ
                m_ops.ڈالو_عقب(tok);
                توڑو;
            حالت AA:  // Accept
                توڑو;
            طےشدہ: tok.fileline()->v3error("Syntax error in `ifdef () expression"); واپس;
            }
        }
    }

عوامی:
    // METHODS
    V3_Pre_اظہاریہ() = طےشدہ;
    ~V3_Pre_اظہاریہ() = طےشدہ;
    خالی reset(File_سطر* flp) {
        m_inputs.clear();
        m_اقدار.clear();
        m_ops.clear();
        m_پہلا_Fileline = flp;
        ڈالو_Op(V3_Pre_اظہاریہ_Token{flp, V3_Pre_اظہاریہ_Token::انتہا_2});
    }
    خالی ڈالو_داخلہ(ثابت V3_Pre_اظہاریہ_Token& token) {
        اگر (!m_پہلا_Fileline) m_پہلا_Fileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ڈالو_عقب(token);
    }
    bool نتیجہ() {
        ڈالو_داخلہ(V3_Pre_اظہاریہ_Token{m_پہلا_Fileline, V3_Pre_اظہاریہ_Token::انتہا_2});
        parse();
        واپس نکالو_قدر().قدر();
    }
    ساکن خالی selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#اگر_ختم  // Guard
