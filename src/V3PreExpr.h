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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3PREEXPR_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3PREEXPR_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>

class V3_Pre_ਵਿਅੰਜਕ_Token final {
ਜਨਤਕ:
    // TYPES
    // Order of enum must match token table
    ਗਿਣਤੀ token_t : uint8_t { ZERO, ONE, ਸਿਰਾ_2, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

ਨਿਜੀ:
    // MEMBERS
    File_ਸਤਰ* ਅਚਲ m_fileline;  // Token fileline
    token_t ਅਚਲ m_token;  // Token value
ਜਨਤਕ:
    // METHODS
    V3_Pre_ਵਿਅੰਜਕ_Token(File_ਸਤਰ* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_ਵਿਅੰਜਕ_Token(File_ਸਤਰ* fileline, bool ਮੁੱਲ)
        : m_fileline{fileline}
        , m_token{ਮੁੱਲ ? ONE : ZERO} {}
    ~V3_Pre_ਵਿਅੰਜਕ_Token() = ਮੂਲ;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਨਾਂ[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        ਵਾਪਸ ਨਾਂ[m_token];
    }
    File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ m_fileline; }
    token_t token() ਅਚਲ { ਵਾਪਸ m_token; }
    bool is_ਮੁੱਲ() ਅਚਲ { ਵਾਪਸ m_token == ZERO || m_token == ONE; }
    bool ਮੁੱਲ() ਅਚਲ {
        UASSERT(is_ਮੁੱਲ(), "preproc expr fetch of non-value");
        ਵਾਪਸ m_token == ONE;
    }
};

class V3_Pre_ਵਿਅੰਜਕ final {
    // MEMBERS
    std::deque<V3_Pre_ਵਿਅੰਜਕ_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_ਵਿਅੰਜਕ_Token> m_ਮੁੱਲਾਂ;  // Value stack
    std::deque<V3_Pre_ਵਿਅੰਜਕ_Token> m_ops;  // Operator stack
    File_ਸਤਰ* m_ਪਹਿਲਾ_Fileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    ਗਿਣਤੀ action_t : uint8_t {
        VV,  //ਮੁੱਲ
        AA,  // Accept
        RR,  // Reduce
        SS,  //ਖਿਸਕਾਓ
        EE  // Error
    };
    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* actionAscii(action_t en) {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਨਾਂ[] = {"VV", "AA", "RR", "SS", "EE"};
        ਵਾਪਸ ਨਾਂ[en];
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

    action_t parse_ਸਾਰਣੀ[V3_Pre_ਵਿਅੰਜਕ_Token::MAX][V3_Pre_ਵਿਅੰਜਕ_Token::MAX] = {
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

    ਸਥਿਰ ਖਾਲੀ selfTestImp() {
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

    ਸਥਿਰ ਖਾਲੀ selfTestOne(ਅਚਲ string& ਵਿਅੰਜਕ, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << ਵਿਅੰਜਕ);
        File_ਸਤਰ* ਅਚਲ flp = nullptr;
        V3_Pre_ਵਿਅੰਜਕ parser;
        parser.reset(flp);
        bool tstart = true;
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = ਵਿਅੰਜਕ.c_str(); *cp; ++cp) {
            ਜੇ (tstart) {
                tstart = false;
                ਚੋਣ (*cp) {
                ਹਾਲਤ '0': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::ZERO}); ਰੋਕੋ;
                ਹਾਲਤ '1': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::ONE}); ਰੋਕੋ;
                ਹਾਲਤ '!': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::LNOT}); ਰੋਕੋ;
                ਹਾਲਤ '|': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::LOR}); ਰੋਕੋ;
                ਹਾਲਤ '&': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::LAND}); ਰੋਕੋ;
                ਹਾਲਤ '-': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::IMP}); ਰੋਕੋ;
                ਹਾਲਤ '<': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::EQV}); ਰੋਕੋ;
                ਹਾਲਤ '(': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::BRA}); ਰੋਕੋ;
                ਹਾਲਤ ')': parser.ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::KET}); ਰੋਕੋ;
                ਮੂਲ: ਰੋਕੋ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (*cp == ' ') {
                tstart = true;
            }
        }
        ਅਚਲ bool got = parser.ਨਤੀਜਾ();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    ਖਾਲੀ ਧੱਕੋ_Op(ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ਧੱਕੋ_ਪਿੱਠ(token);
    }
    ਖਾਲੀ ਧੱਕੋ_ਮੁੱਲ(ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_ਮੁੱਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(token);
    }
    V3_Pre_ਵਿਅੰਜਕ_Token ਕੱਢੋ_ਮੁੱਲ() {
        ਜੇ (m_ਮੁੱਲਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            m_ਪਹਿਲਾ_Fileline->v3error("Syntax error in `ifdef () expression");
            ਵਾਪਸ V3_Pre_ਵਿਅੰਜਕ_Token{m_ਪਹਿਲਾ_Fileline, false};
        }
        ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token tok = m_ਮੁੱਲਾਂ.ਪਿੱਠ();
        m_ਮੁੱਲਾਂ.ਕੱਢੋ_ਪਿੱਠ();
        // UINFO(9, "  popValue " << tok.ascii());
        ਵਾਪਸ tok;
    }
    ਖਾਲੀ reduce() {
        UASSERT(!m_ops.ਖਾਲੀ_ਥਾਵਾਂ(), "lost op stack beginning END");
        ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token tok = m_ops.ਪਿੱਠ();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.ਕੱਢੋ_ਪਿੱਠ();
        ਚੋਣ (tok.token()) {
        ਹਾਲਤ V3_Pre_ਵਿਅੰਜਕ_Token::KET: {
            ਜਦੋਂ (m_ops.ਪਿੱਠ().token() != V3_Pre_ਵਿਅੰਜਕ_Token::ਸਿਰਾ_2
                   && m_ops.ਪਿੱਠ().token() != V3_Pre_ਵਿਅੰਜਕ_Token::BRA)
                reduce();
            ਜੇ (m_ops.ਪਿੱਠ().token() == V3_Pre_ਵਿਅੰਜਕ_Token::BRA) {
                m_ops.ਕੱਢੋ_ਪਿੱਠ();
            } ਨਹੀਂਤਾਂ {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                ਵਾਪਸ;
            }
            ਰੋਕੋ;
        }
        ਹਾਲਤ V3_Pre_ਵਿਅੰਜਕ_Token::LNOT: {
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token lhs = ਕੱਢੋ_ਮੁੱਲ();
            ਧੱਕੋ_ਮੁੱਲ(V3_Pre_ਵਿਅੰਜਕ_Token{tok.fileline(), !lhs.ਮੁੱਲ()});
            ਰੋਕੋ;
        }
        ਹਾਲਤ V3_Pre_ਵਿਅੰਜਕ_Token::LAND: {
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token rhs = ਕੱਢੋ_ਮੁੱਲ();
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token lhs = ਕੱਢੋ_ਮੁੱਲ();
            ਧੱਕੋ_ਮੁੱਲ(V3_Pre_ਵਿਅੰਜਕ_Token{tok.fileline(), lhs.ਮੁੱਲ() && rhs.ਮੁੱਲ()});
            ਰੋਕੋ;
        }
        ਹਾਲਤ V3_Pre_ਵਿਅੰਜਕ_Token::LOR: {
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token rhs = ਕੱਢੋ_ਮੁੱਲ();
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token lhs = ਕੱਢੋ_ਮੁੱਲ();
            ਧੱਕੋ_ਮੁੱਲ(V3_Pre_ਵਿਅੰਜਕ_Token{tok.fileline(), lhs.ਮੁੱਲ() || rhs.ਮੁੱਲ()});
            ਰੋਕੋ;
        }
        ਹਾਲਤ V3_Pre_ਵਿਅੰਜਕ_Token::IMP: {
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token rhs = ਕੱਢੋ_ਮੁੱਲ();
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token lhs = ਕੱਢੋ_ਮੁੱਲ();
            ਧੱਕੋ_ਮੁੱਲ(V3_Pre_ਵਿਅੰਜਕ_Token{tok.fileline(), !lhs.ਮੁੱਲ() || rhs.ਮੁੱਲ()});
            ਰੋਕੋ;
        }
        ਹਾਲਤ V3_Pre_ਵਿਅੰਜਕ_Token::EQV: {
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token rhs = ਕੱਢੋ_ਮੁੱਲ();
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token lhs = ਕੱਢੋ_ਮੁੱਲ();
            ਧੱਕੋ_ਮੁੱਲ(V3_Pre_ਵਿਅੰਜਕ_Token{tok.fileline(), lhs.ਮੁੱਲ() == rhs.ਮੁੱਲ()});
            ਰੋਕੋ;
        }
        ਮੂਲ: {
            v3fatalSrc("bad case on operand stack");
            ਰੋਕੋ;
        }
        }
    }
    ਖਾਲੀ parse() {
        ਜਦੋਂ (!m_inputs.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token tok = m_inputs.front();
            m_inputs.ਕੱਢੋ_front();
            UINFO(9, "input read " << tok.ascii());
            ਜੇ (tok.is_ਮੁੱਲ()) {
                ਧੱਕੋ_ਮੁੱਲ(tok);
                ਜਾਰੀ;
            }

            UASSERT(!m_ops.ਖਾਲੀ_ਥਾਵਾਂ(), "lost op stack beginning END");
            ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token ਸਿਖਰ_Tok = m_ops.ਪਿੱਠ();
            ਸਵੈਚਾਲਿਤ action = parse_ਸਾਰਣੀ[ਸਿਖਰ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << ਸਿਖਰ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            ਚੋਣ (action) {
            ਹਾਲਤ RR:  // Reduce
                reduce();
                ਰੋਕੋ;
            ਹਾਲਤ SS:  //ਖਿਸਕਾਓ
                m_ops.ਧੱਕੋ_ਪਿੱਠ(tok);
                ਰੋਕੋ;
            ਹਾਲਤ AA:  // Accept
                ਰੋਕੋ;
            ਮੂਲ: tok.fileline()->v3error("Syntax error in `ifdef () expression"); ਵਾਪਸ;
            }
        }
    }

ਜਨਤਕ:
    // METHODS
    V3_Pre_ਵਿਅੰਜਕ() = ਮੂਲ;
    ~V3_Pre_ਵਿਅੰਜਕ() = ਮੂਲ;
    ਖਾਲੀ reset(File_ਸਤਰ* flp) {
        m_inputs.clear();
        m_ਮੁੱਲਾਂ.clear();
        m_ops.clear();
        m_ਪਹਿਲਾ_Fileline = flp;
        ਧੱਕੋ_Op(V3_Pre_ਵਿਅੰਜਕ_Token{flp, V3_Pre_ਵਿਅੰਜਕ_Token::ਸਿਰਾ_2});
    }
    ਖਾਲੀ ਧੱਕੋ_ਨਿਵੇਸ਼(ਅਚਲ V3_Pre_ਵਿਅੰਜਕ_Token& token) {
        ਜੇ (!m_ਪਹਿਲਾ_Fileline) m_ਪਹਿਲਾ_Fileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ਧੱਕੋ_ਪਿੱਠ(token);
    }
    bool ਨਤੀਜਾ() {
        ਧੱਕੋ_ਨਿਵੇਸ਼(V3_Pre_ਵਿਅੰਜਕ_Token{m_ਪਹਿਲਾ_Fileline, V3_Pre_ਵਿਅੰਜਕ_Token::ਸਿਰਾ_2});
        parse();
        ਵਾਪਸ ਕੱਢੋ_ਮੁੱਲ().ਮੁੱਲ();
    }
    ਸਥਿਰ ਖਾਲੀ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#ਜੇ_ਅੰਤ  // Guard
