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

#که_نه_تعریف_شوی VERILATOR_V3PREEXPR_H_
#تعریف_کړه VERILATOR_V3PREEXPR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3FileLine.h"
#ضمن "V3Global.h"

#ضمن <deque>

class V3_Pre_بیان_Token final {
عام:
    // TYPES
    // Order of enum must match token table
    تعداد token_t : uint8_t { ZERO, ONE, پای_ټکی_2, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

خاص:
    // MEMBERS
    File_کرښه* ثابت m_fileline;  // Token fileline
    token_t ثابت m_token;  // Token value
عام:
    // METHODS
    V3_Pre_بیان_Token(File_کرښه* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_بیان_Token(File_کرښه* fileline, bool ارزښت)
        : m_fileline{fileline}
        , m_token{ارزښت ? ONE : ZERO} {}
    ~V3_Pre_بیان_Token() = افتراضي;
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* نومونه[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        أرجع نومونه[m_token];
    }
    File_کرښه* fileline() ثابت { أرجع m_fileline; }
    token_t token() ثابت { أرجع m_token; }
    bool is_ارزښت() ثابت { أرجع m_token == ZERO || m_token == ONE; }
    bool ارزښت() ثابت {
        UASSERT(is_ارزښت(), "preproc expr fetch of non-value");
        أرجع m_token == ONE;
    }
};

class V3_Pre_بیان final {
    // MEMBERS
    std::deque<V3_Pre_بیان_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_بیان_Token> m_ارزښتونه;  // Value stack
    std::deque<V3_Pre_بیان_Token> m_ops;  // Operator stack
    File_کرښه* m_لومړی_Fileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    تعداد action_t : uint8_t {
        VV,  //ارزښت
        AA,  // Accept
        RR,  // Reduce
        SS,  //ښويدنه
        EE  // Error
    };
    ساكن ثابت حرف* actionAscii(action_t en) {
        ساكن constexpr ثابت حرف* نومونه[] = {"VV", "AA", "RR", "SS", "EE"};
        أرجع نومونه[en];
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

    action_t parse_جدول[V3_Pre_بیان_Token::MAX][V3_Pre_بیان_Token::MAX] = {
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

    ساكن فراغ selfTestOne(ثابت string& بیان, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << بیان);
        File_کرښه* ثابت flp = nullptr;
        V3_Pre_بیان parser;
        parser.reset(flp);
        bool tstart = true;
        لكل (ثابت حرف* cp = بیان.c_str(); *cp; ++cp) {
            إذا (tstart) {
                tstart = false;
                بدل (*cp) {
                حالة '0': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::ZERO}); اكسر;
                حالة '1': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::ONE}); اكسر;
                حالة '!': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::LNOT}); اكسر;
                حالة '|': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::LOR}); اكسر;
                حالة '&': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::LAND}); اكسر;
                حالة '-': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::IMP}); اكسر;
                حالة '<': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::EQV}); اكسر;
                حالة '(': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::BRA}); اكسر;
                حالة ')': parser.ټېل_ننوت(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::KET}); اكسر;
                افتراضي: اكسر;
                }
            } وإلا إذا (*cp == ' ') {
                tstart = true;
            }
        }
        ثابت bool got = parser.پایله();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    فراغ ټېل_Op(ثابت V3_Pre_بیان_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ټېل_شا(token);
    }
    فراغ ټېل_ارزښت(ثابت V3_Pre_بیان_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_ارزښتونه.ټېل_شا(token);
    }
    V3_Pre_بیان_Token راوباسه_ارزښت() {
        إذا (m_ارزښتونه.تشه()) {
            m_لومړی_Fileline->v3error("Syntax error in `ifdef () expression");
            أرجع V3_Pre_بیان_Token{m_لومړی_Fileline, false};
        }
        ثابت V3_Pre_بیان_Token tok = m_ارزښتونه.شا();
        m_ارزښتونه.راوباسه_شا();
        // UINFO(9, "  popValue " << tok.ascii());
        أرجع tok;
    }
    فراغ reduce() {
        UASSERT(!m_ops.تشه(), "lost op stack beginning END");
        ثابت V3_Pre_بیان_Token tok = m_ops.شا();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.راوباسه_شا();
        بدل (tok.token()) {
        حالة V3_Pre_بیان_Token::KET: {
            طالما (m_ops.شا().token() != V3_Pre_بیان_Token::پای_ټکی_2
                   && m_ops.شا().token() != V3_Pre_بیان_Token::BRA)
                reduce();
            إذا (m_ops.شا().token() == V3_Pre_بیان_Token::BRA) {
                m_ops.راوباسه_شا();
            } وإلا {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                أرجع;
            }
            اكسر;
        }
        حالة V3_Pre_بیان_Token::LNOT: {
            ثابت V3_Pre_بیان_Token lhs = راوباسه_ارزښت();
            ټېل_ارزښت(V3_Pre_بیان_Token{tok.fileline(), !lhs.ارزښت()});
            اكسر;
        }
        حالة V3_Pre_بیان_Token::LAND: {
            ثابت V3_Pre_بیان_Token rhs = راوباسه_ارزښت();
            ثابت V3_Pre_بیان_Token lhs = راوباسه_ارزښت();
            ټېل_ارزښت(V3_Pre_بیان_Token{tok.fileline(), lhs.ارزښت() && rhs.ارزښت()});
            اكسر;
        }
        حالة V3_Pre_بیان_Token::LOR: {
            ثابت V3_Pre_بیان_Token rhs = راوباسه_ارزښت();
            ثابت V3_Pre_بیان_Token lhs = راوباسه_ارزښت();
            ټېل_ارزښت(V3_Pre_بیان_Token{tok.fileline(), lhs.ارزښت() || rhs.ارزښت()});
            اكسر;
        }
        حالة V3_Pre_بیان_Token::IMP: {
            ثابت V3_Pre_بیان_Token rhs = راوباسه_ارزښت();
            ثابت V3_Pre_بیان_Token lhs = راوباسه_ارزښت();
            ټېل_ارزښت(V3_Pre_بیان_Token{tok.fileline(), !lhs.ارزښت() || rhs.ارزښت()});
            اكسر;
        }
        حالة V3_Pre_بیان_Token::EQV: {
            ثابت V3_Pre_بیان_Token rhs = راوباسه_ارزښت();
            ثابت V3_Pre_بیان_Token lhs = راوباسه_ارزښت();
            ټېل_ارزښت(V3_Pre_بیان_Token{tok.fileline(), lhs.ارزښت() == rhs.ارزښت()});
            اكسر;
        }
        افتراضي: {
            v3fatalSrc("bad case on operand stack");
            اكسر;
        }
        }
    }
    فراغ parse() {
        طالما (!m_inputs.تشه()) {
            ثابت V3_Pre_بیان_Token tok = m_inputs.front();
            m_inputs.راوباسه_front();
            UINFO(9, "input read " << tok.ascii());
            إذا (tok.is_ارزښت()) {
                ټېل_ارزښت(tok);
                استمر;
            }

            UASSERT(!m_ops.تشه(), "lost op stack beginning END");
            ثابت V3_Pre_بیان_Token سر_Tok = m_ops.شا();
            خپلکاری action = parse_جدول[سر_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << سر_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            بدل (action) {
            حالة RR:  // Reduce
                reduce();
                اكسر;
            حالة SS:  //ښويدنه
                m_ops.ټېل_شا(tok);
                اكسر;
            حالة AA:  // Accept
                اكسر;
            افتراضي: tok.fileline()->v3error("Syntax error in `ifdef () expression"); أرجع;
            }
        }
    }

عام:
    // METHODS
    V3_Pre_بیان() = افتراضي;
    ~V3_Pre_بیان() = افتراضي;
    فراغ reset(File_کرښه* flp) {
        m_inputs.clear();
        m_ارزښتونه.clear();
        m_ops.clear();
        m_لومړی_Fileline = flp;
        ټېل_Op(V3_Pre_بیان_Token{flp, V3_Pre_بیان_Token::پای_ټکی_2});
    }
    فراغ ټېل_ننوت(ثابت V3_Pre_بیان_Token& token) {
        إذا (!m_لومړی_Fileline) m_لومړی_Fileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ټېل_شا(token);
    }
    bool پایله() {
        ټېل_ننوت(V3_Pre_بیان_Token{m_لومړی_Fileline, V3_Pre_بیان_Token::پای_ټکی_2});
        parse();
        أرجع راوباسه_ارزښت().ارزښت();
    }
    ساكن فراغ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#د_که_پای  // Guard
