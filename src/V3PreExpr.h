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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3PREEXPR_H_
#وصف_ڪريو VERILATOR_V3PREEXPR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3FileLine.h"
#ضمن "V3Global.h"

#ضمن <deque>

class V3_Pre_اظهار_Token final {
عام:
    // TYPES
    // Order of enum must match token table
    تعداد token_t : uint8_t { ZERO, ONE, پڇاڙي_2, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خاص:
    // MEMBERS
    File_سٽ* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عام:
    // METHODS
    V3_Pre_اظهار_Token(File_سٽ* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_اظهار_Token(File_سٽ* fileline, bool قدر)
        : m_fileline{fileline}
        , m_token{قدر ? ONE : ZERO} {}
    ~V3_Pre_اظهار_Token() = افتراضي;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* نالا[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        أرجع نالا[m_token];
    }
    File_سٽ* fileline() ثابت { أرجع m_fileline; }
    token_t token() ثابت { أرجع m_token; }
    bool is_قدر() ثابت { أرجع m_token == ZERO || m_token == ONE; }
    bool قدر() ثابت {
        UASSERT(is_قدر(), "preproc expr fetch of non-value");
        أرجع m_token == ONE;
    }
};

class V3_Pre_اظهار final {
    // MEMBERS
    std::deque<V3_Pre_اظهار_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_اظهار_Token> m_قدرون;  // Value stack
    std::deque<V3_Pre_اظهار_Token> m_ops;  // Operator stack
    File_سٽ* m_پهريون_Fileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    تعداد action_t : uint8_t {
        VV,  //قدر
        AA,  // Accept
        RR,  // Reduce
        SS,  //سرڪايو
        EE  // Error
    };
    ساكن ثابت حرف* actionAscii(action_t en) {
        ساكن constexpr ثابت حرف* نالا[] = {"VV", "AA", "RR", "SS", "EE"};
        أرجع نالا[en];
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

    action_t parse_جدول[V3_Pre_اظهار_Token::MAX][V3_Pre_اظهار_Token::MAX] = {
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

    ساكن فراغ selfTestOne(ثابت string& اظهار, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << اظهار);
        File_سٽ* ثابت flp = nullptr;
        V3_Pre_اظهار parser;
        parser.reset(flp);
        bool tstart = true;
        لكل (ثابت حرف* cp = اظهار.c_str(); *cp; ++cp) {
            إذا (tstart) {
                tstart = false;
                بدل (*cp) {
                حالة '0': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::ZERO}); اكسر;
                حالة '1': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::ONE}); اكسر;
                حالة '!': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::LNOT}); اكسر;
                حالة '|': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::LOR}); اكسر;
                حالة '&': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::LAND}); اكسر;
                حالة '-': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::IMP}); اكسر;
                حالة '<': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::EQV}); اكسر;
                حالة '(': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::BRA}); اكسر;
                حالة ')': parser.ڌڪو_داخلا(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::KET}); اكسر;
                افتراضي: اكسر;
                }
            } وإلا إذا (*cp == ' ') {
                tstart = true;
            }
        }
        ثابت bool got = parser.نتيجو();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    فراغ ڌڪو_Op(ثابت V3_Pre_اظهار_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ڌڪو_پٺ(token);
    }
    فراغ ڌڪو_قدر(ثابت V3_Pre_اظهار_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_قدرون.ڌڪو_پٺ(token);
    }
    V3_Pre_اظهار_Token ڪڍو_قدر() {
        إذا (m_قدرون.خالي_جايون()) {
            m_پهريون_Fileline->v3error("Syntax error in `ifdef () expression");
            أرجع V3_Pre_اظهار_Token{m_پهريون_Fileline, false};
        }
        ثابت V3_Pre_اظهار_Token tok = m_قدرون.پٺ();
        m_قدرون.ڪڍو_پٺ();
        // UINFO(9, "  popValue " << tok.ascii());
        أرجع tok;
    }
    فراغ reduce() {
        UASSERT(!m_ops.خالي_جايون(), "lost op stack beginning END");
        ثابت V3_Pre_اظهار_Token tok = m_ops.پٺ();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.ڪڍو_پٺ();
        بدل (tok.token()) {
        حالة V3_Pre_اظهار_Token::KET: {
            طالما (m_ops.پٺ().token() != V3_Pre_اظهار_Token::پڇاڙي_2
                   && m_ops.پٺ().token() != V3_Pre_اظهار_Token::BRA)
                reduce();
            إذا (m_ops.پٺ().token() == V3_Pre_اظهار_Token::BRA) {
                m_ops.ڪڍو_پٺ();
            } وإلا {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                أرجع;
            }
            اكسر;
        }
        حالة V3_Pre_اظهار_Token::LNOT: {
            ثابت V3_Pre_اظهار_Token lhs = ڪڍو_قدر();
            ڌڪو_قدر(V3_Pre_اظهار_Token{tok.fileline(), !lhs.قدر()});
            اكسر;
        }
        حالة V3_Pre_اظهار_Token::LAND: {
            ثابت V3_Pre_اظهار_Token rhs = ڪڍو_قدر();
            ثابت V3_Pre_اظهار_Token lhs = ڪڍو_قدر();
            ڌڪو_قدر(V3_Pre_اظهار_Token{tok.fileline(), lhs.قدر() && rhs.قدر()});
            اكسر;
        }
        حالة V3_Pre_اظهار_Token::LOR: {
            ثابت V3_Pre_اظهار_Token rhs = ڪڍو_قدر();
            ثابت V3_Pre_اظهار_Token lhs = ڪڍو_قدر();
            ڌڪو_قدر(V3_Pre_اظهار_Token{tok.fileline(), lhs.قدر() || rhs.قدر()});
            اكسر;
        }
        حالة V3_Pre_اظهار_Token::IMP: {
            ثابت V3_Pre_اظهار_Token rhs = ڪڍو_قدر();
            ثابت V3_Pre_اظهار_Token lhs = ڪڍو_قدر();
            ڌڪو_قدر(V3_Pre_اظهار_Token{tok.fileline(), !lhs.قدر() || rhs.قدر()});
            اكسر;
        }
        حالة V3_Pre_اظهار_Token::EQV: {
            ثابت V3_Pre_اظهار_Token rhs = ڪڍو_قدر();
            ثابت V3_Pre_اظهار_Token lhs = ڪڍو_قدر();
            ڌڪو_قدر(V3_Pre_اظهار_Token{tok.fileline(), lhs.قدر() == rhs.قدر()});
            اكسر;
        }
        افتراضي: {
            v3fatalSrc("bad case on operand stack");
            اكسر;
        }
        }
    }
    فراغ parse() {
        طالما (!m_inputs.خالي_جايون()) {
            ثابت V3_Pre_اظهار_Token tok = m_inputs.front();
            m_inputs.ڪڍو_front();
            UINFO(9, "input read " << tok.ascii());
            إذا (tok.is_قدر()) {
                ڌڪو_قدر(tok);
                استمر;
            }

            UASSERT(!m_ops.خالي_جايون(), "lost op stack beginning END");
            ثابت V3_Pre_اظهار_Token چوٽي_Tok = m_ops.پٺ();
            خودڪار action = parse_جدول[چوٽي_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << چوٽي_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            بدل (action) {
            حالة RR:  // Reduce
                reduce();
                اكسر;
            حالة SS:  //سرڪايو
                m_ops.ڌڪو_پٺ(tok);
                اكسر;
            حالة AA:  // Accept
                اكسر;
            افتراضي: tok.fileline()->v3error("Syntax error in `ifdef () expression"); أرجع;
            }
        }
    }

عام:
    // METHODS
    V3_Pre_اظهار() = افتراضي;
    ~V3_Pre_اظهار() = افتراضي;
    فراغ reset(File_سٽ* flp) {
        m_inputs.clear();
        m_قدرون.clear();
        m_ops.clear();
        m_پهريون_Fileline = flp;
        ڌڪو_Op(V3_Pre_اظهار_Token{flp, V3_Pre_اظهار_Token::پڇاڙي_2});
    }
    فراغ ڌڪو_داخلا(ثابت V3_Pre_اظهار_Token& token) {
        إذا (!m_پهريون_Fileline) m_پهريون_Fileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ڌڪو_پٺ(token);
    }
    bool نتيجو() {
        ڌڪو_داخلا(V3_Pre_اظهار_Token{m_پهريون_Fileline, V3_Pre_اظهار_Token::پڇاڙي_2});
        parse();
        أرجع ڪڍو_قدر().قدر();
    }
    ساكن فراغ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#جيڪڏهن_پورو  // Guard
