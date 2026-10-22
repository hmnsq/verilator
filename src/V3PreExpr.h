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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3FileLine.h"
#شامل "V3Global.h"

#شامل <deque>

class V3_Pre_عبارت_Token final {
عمومی:
    // TYPES
    // Order of enum must match token table
    شمارش token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خصوصی:
    // MEMBERS
    File_سطر* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عمومی:
    // METHODS
    V3_Pre_عبارت_Token(File_سطر* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_عبارت_Token(File_سطر* fileline, bool مقدار)
        : m_fileline{fileline}
        , m_token{مقدار ? ONE : ZERO} {}
    ~V3_Pre_عبارت_Token() = پیشفرض;
    ثابت نویسه* ascii() ثابت {
        ایستا ثابت نویسه* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        بازگشت names[m_token];
    }
    File_سطر* fileline() ثابت { بازگشت m_fileline; }
    token_t token() ثابت { بازگشت m_token; }
    bool is_مقدار() ثابت { بازگشت m_token == ZERO || m_token == ONE; }
    bool مقدار() ثابت {
        UASSERT(is_مقدار(), "preproc expr fetch of non-value");
        بازگشت m_token == ONE;
    }
};

class V3_Pre_عبارت final {
    // MEMBERS
    std::deque<V3_Pre_عبارت_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_عبارت_Token> m_مقادیر;  // Value stack
    std::deque<V3_Pre_عبارت_Token> m_ops;  // Operator stack
    File_سطر* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    شمارش action_t : uint8_t {
        VV,  //مقدار
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    ایستا ثابت نویسه* actionAscii(action_t en) {
        ایستا constexpr ثابت نویسه* names[] = {"VV", "AA", "RR", "SS", "EE"};
        بازگشت names[en];
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

    action_t parseTable[V3_Pre_عبارت_Token::MAX][V3_Pre_عبارت_Token::MAX] = {
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

    ایستا تهی selfTestImp() {
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

    ایستا تهی selfTestOne(ثابت string& عبارت, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << عبارت);
        File_سطر* ثابت flp = nullptr;
        V3_Pre_عبارت parser;
        parser.reset(flp);
        bool tstart = true;
        برای (ثابت نویسه* cp = عبارت.c_str(); *cp; ++cp) {
            اگر (tstart) {
                tstart = false;
                انتخاب (*cp) {
                حالت '0': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::ZERO}); بشکن;
                حالت '1': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::ONE}); بشکن;
                حالت '!': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LNOT}); بشکن;
                حالت '|': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LOR}); بشکن;
                حالت '&': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::LAND}); بشکن;
                حالت '-': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::IMP}); بشکن;
                حالت '<': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::EQV}); بشکن;
                حالت '(': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::BRA}); بشکن;
                حالت ')': parser.افزودن_درون_داد(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::KET}); بشکن;
                پیشفرض: بشکن;
                }
            } وگرنه اگر (*cp == ' ') {
                tstart = true;
            }
        }
        ثابت bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    تهی افزودن_Op(ثابت V3_Pre_عبارت_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.افزودن_back(token);
    }
    تهی افزودن_مقدار(ثابت V3_Pre_عبارت_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_مقادیر.افزودن_back(token);
    }
    V3_Pre_عبارت_Token برداشت_مقدار() {
        اگر (m_مقادیر.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            بازگشت V3_Pre_عبارت_Token{m_firstFileline, false};
        }
        ثابت V3_Pre_عبارت_Token tok = m_مقادیر.back();
        m_مقادیر.برداشت_back();
        // UINFO(9, "  popValue " << tok.ascii());
        بازگشت tok;
    }
    تهی reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ثابت V3_Pre_عبارت_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.برداشت_back();
        انتخاب (tok.token()) {
        حالت V3_Pre_عبارت_Token::KET: {
            وقتی (m_ops.back().token() != V3_Pre_عبارت_Token::END
                   && m_ops.back().token() != V3_Pre_عبارت_Token::BRA)
                reduce();
            اگر (m_ops.back().token() == V3_Pre_عبارت_Token::BRA) {
                m_ops.برداشت_back();
            } وگرنه {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                بازگشت;
            }
            بشکن;
        }
        حالت V3_Pre_عبارت_Token::LNOT: {
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), !lhs.مقدار()});
            بشکن;
        }
        حالت V3_Pre_عبارت_Token::LAND: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), lhs.مقدار() && rhs.مقدار()});
            بشکن;
        }
        حالت V3_Pre_عبارت_Token::LOR: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), lhs.مقدار() || rhs.مقدار()});
            بشکن;
        }
        حالت V3_Pre_عبارت_Token::IMP: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), !lhs.مقدار() || rhs.مقدار()});
            بشکن;
        }
        حالت V3_Pre_عبارت_Token::EQV: {
            ثابت V3_Pre_عبارت_Token rhs = برداشت_مقدار();
            ثابت V3_Pre_عبارت_Token lhs = برداشت_مقدار();
            افزودن_مقدار(V3_Pre_عبارت_Token{tok.fileline(), lhs.مقدار() == rhs.مقدار()});
            بشکن;
        }
        پیشفرض: {
            v3fatalSrc("bad case on operand stack");
            بشکن;
        }
        }
    }
    تهی parse() {
        وقتی (!m_inputs.empty()) {
            ثابت V3_Pre_عبارت_Token tok = m_inputs.front();
            m_inputs.برداشت_front();
            UINFO(9, "input read " << tok.ascii());
            اگر (tok.is_مقدار()) {
                افزودن_مقدار(tok);
                ادامه;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ثابت V3_Pre_عبارت_Token بالاترین_Tok = m_ops.back();
            auto action = parseTable[بالاترین_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << بالاترین_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            انتخاب (action) {
            حالت RR:  // Reduce
                reduce();
                بشکن;
            حالت SS:  // Shift
                m_ops.افزودن_back(tok);
                بشکن;
            حالت AA:  // Accept
                بشکن;
            پیشفرض: tok.fileline()->v3error("Syntax error in `ifdef () expression"); بازگشت;
            }
        }
    }

عمومی:
    // METHODS
    V3_Pre_عبارت() = پیشفرض;
    ~V3_Pre_عبارت() = پیشفرض;
    تهی reset(File_سطر* flp) {
        m_inputs.clear();
        m_مقادیر.clear();
        m_ops.clear();
        m_firstFileline = flp;
        افزودن_Op(V3_Pre_عبارت_Token{flp, V3_Pre_عبارت_Token::END});
    }
    تهی افزودن_درون_داد(ثابت V3_Pre_عبارت_Token& token) {
        اگر (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.افزودن_back(token);
    }
    bool result() {
        افزودن_درون_داد(V3_Pre_عبارت_Token{m_firstFileline, V3_Pre_عبارت_Token::END});
        parse();
        بازگشت برداشت_مقدار().مقدار();
    }
    ایستا تهی selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
