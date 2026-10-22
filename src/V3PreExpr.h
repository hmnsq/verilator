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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Global.h"

#અન્તર્ભાવ <deque>

class V3_Pre_પદાવલિ_Token final {
જાહેર:
    // TYPES
    // Order of enum must match token table
    ગણના token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

ખાનગી:
    // MEMBERS
    File_લીટી* અચલ m_fileline;  // Token fileline
    token_t અચલ m_token;  // Token value
જાહેર:
    // METHODS
    V3_Pre_પદાવલિ_Token(File_લીટી* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_પદાવલિ_Token(File_લીટી* fileline, bool કિંમત)
        : m_fileline{fileline}
        , m_token{કિંમત ? ONE : ZERO} {}
    ~V3_Pre_પદાવલિ_Token() = મૂળભૂત;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર અચલ અક્ષર* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        પાછા names[m_token];
    }
    File_લીટી* fileline() અચલ { પાછા m_fileline; }
    token_t token() અચલ { પાછા m_token; }
    bool is_કિંમત() અચલ { પાછા m_token == ZERO || m_token == ONE; }
    bool કિંમત() અચલ {
        UASSERT(is_કિંમત(), "preproc expr fetch of non-value");
        પાછા m_token == ONE;
    }
};

class V3_Pre_પદાવલિ final {
    // MEMBERS
    std::deque<V3_Pre_પદાવલિ_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_પદાવલિ_Token> m_કિંમતો;  // Value stack
    std::deque<V3_Pre_પદાવલિ_Token> m_ops;  // Operator stack
    File_લીટી* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    ગણના action_t : uint8_t {
        VV,  //કિંમત
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    સ્થિર અચલ અક્ષર* actionAscii(action_t en) {
        સ્થિર constexpr અચલ અક્ષર* names[] = {"VV", "AA", "RR", "SS", "EE"};
        પાછા names[en];
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

    action_t parseTable[V3_Pre_પદાવલિ_Token::MAX][V3_Pre_પદાવલિ_Token::MAX] = {
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

    સ્થિર શૂન્ય selfTestImp() {
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

    સ્થિર શૂન્ય selfTestOne(અચલ string& પદાવલિ, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << પદાવલિ);
        File_લીટી* અચલ flp = nullptr;
        V3_Pre_પદાવલિ parser;
        parser.reset(flp);
        bool tstart = true;
        માટે (અચલ અક્ષર* cp = પદાવલિ.c_str(); *cp; ++cp) {
            જો (tstart) {
                tstart = false;
                પસંદગી (*cp) {
                સ્થિતિ '0': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::ZERO}); તોડો;
                સ્થિતિ '1': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::ONE}); તોડો;
                સ્થિતિ '!': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::LNOT}); તોડો;
                સ્થિતિ '|': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::LOR}); તોડો;
                સ્થિતિ '&': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::LAND}); તોડો;
                સ્થિતિ '-': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::IMP}); તોડો;
                સ્થિતિ '<': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::EQV}); તોડો;
                સ્થિતિ '(': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::BRA}); તોડો;
                સ્થિતિ ')': parser.ધકેલો_આગત(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::KET}); તોડો;
                મૂળભૂત: તોડો;
                }
            } નહીંતર જો (*cp == ' ') {
                tstart = true;
            }
        }
        અચલ bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    શૂન્ય ધકેલો_Op(અચલ V3_Pre_પદાવલિ_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ધકેલો_back(token);
    }
    શૂન્ય ધકેલો_કિંમત(અચલ V3_Pre_પદાવલિ_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_કિંમતો.ધકેલો_back(token);
    }
    V3_Pre_પદાવલિ_Token કાઢો_કિંમત() {
        જો (m_કિંમતો.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            પાછા V3_Pre_પદાવલિ_Token{m_firstFileline, false};
        }
        અચલ V3_Pre_પદાવલિ_Token tok = m_કિંમતો.back();
        m_કિંમતો.કાઢો_back();
        // UINFO(9, "  popValue " << tok.ascii());
        પાછા tok;
    }
    શૂન્ય reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        અચલ V3_Pre_પદાવલિ_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.કાઢો_back();
        પસંદગી (tok.token()) {
        સ્થિતિ V3_Pre_પદાવલિ_Token::KET: {
            જ્યારે (m_ops.back().token() != V3_Pre_પદાવલિ_Token::END
                   && m_ops.back().token() != V3_Pre_પદાવલિ_Token::BRA)
                reduce();
            જો (m_ops.back().token() == V3_Pre_પદાવલિ_Token::BRA) {
                m_ops.કાઢો_back();
            } નહીંતર {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                પાછા;
            }
            તોડો;
        }
        સ્થિતિ V3_Pre_પદાવલિ_Token::LNOT: {
            અચલ V3_Pre_પદાવલિ_Token lhs = કાઢો_કિંમત();
            ધકેલો_કિંમત(V3_Pre_પદાવલિ_Token{tok.fileline(), !lhs.કિંમત()});
            તોડો;
        }
        સ્થિતિ V3_Pre_પદાવલિ_Token::LAND: {
            અચલ V3_Pre_પદાવલિ_Token rhs = કાઢો_કિંમત();
            અચલ V3_Pre_પદાવલિ_Token lhs = કાઢો_કિંમત();
            ધકેલો_કિંમત(V3_Pre_પદાવલિ_Token{tok.fileline(), lhs.કિંમત() && rhs.કિંમત()});
            તોડો;
        }
        સ્થિતિ V3_Pre_પદાવલિ_Token::LOR: {
            અચલ V3_Pre_પદાવલિ_Token rhs = કાઢો_કિંમત();
            અચલ V3_Pre_પદાવલિ_Token lhs = કાઢો_કિંમત();
            ધકેલો_કિંમત(V3_Pre_પદાવલિ_Token{tok.fileline(), lhs.કિંમત() || rhs.કિંમત()});
            તોડો;
        }
        સ્થિતિ V3_Pre_પદાવલિ_Token::IMP: {
            અચલ V3_Pre_પદાવલિ_Token rhs = કાઢો_કિંમત();
            અચલ V3_Pre_પદાવલિ_Token lhs = કાઢો_કિંમત();
            ધકેલો_કિંમત(V3_Pre_પદાવલિ_Token{tok.fileline(), !lhs.કિંમત() || rhs.કિંમત()});
            તોડો;
        }
        સ્થિતિ V3_Pre_પદાવલિ_Token::EQV: {
            અચલ V3_Pre_પદાવલિ_Token rhs = કાઢો_કિંમત();
            અચલ V3_Pre_પદાવલિ_Token lhs = કાઢો_કિંમત();
            ધકેલો_કિંમત(V3_Pre_પદાવલિ_Token{tok.fileline(), lhs.કિંમત() == rhs.કિંમત()});
            તોડો;
        }
        મૂળભૂત: {
            v3fatalSrc("bad case on operand stack");
            તોડો;
        }
        }
    }
    શૂન્ય parse() {
        જ્યારે (!m_inputs.empty()) {
            અચલ V3_Pre_પદાવલિ_Token tok = m_inputs.front();
            m_inputs.કાઢો_front();
            UINFO(9, "input read " << tok.ascii());
            જો (tok.is_કિંમત()) {
                ધકેલો_કિંમત(tok);
                ચાલુ;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            અચલ V3_Pre_પદાવલિ_Token ટોચ_Tok = m_ops.back();
            auto action = parseTable[ટોચ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << ટોચ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            પસંદગી (action) {
            સ્થિતિ RR:  // Reduce
                reduce();
                તોડો;
            સ્થિતિ SS:  // Shift
                m_ops.ધકેલો_back(tok);
                તોડો;
            સ્થિતિ AA:  // Accept
                તોડો;
            મૂળભૂત: tok.fileline()->v3error("Syntax error in `ifdef () expression"); પાછા;
            }
        }
    }

જાહેર:
    // METHODS
    V3_Pre_પદાવલિ() = મૂળભૂત;
    ~V3_Pre_પદાવલિ() = મૂળભૂત;
    શૂન્ય reset(File_લીટી* flp) {
        m_inputs.clear();
        m_કિંમતો.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ધકેલો_Op(V3_Pre_પદાવલિ_Token{flp, V3_Pre_પદાવલિ_Token::END});
    }
    શૂન્ય ધકેલો_આગત(અચલ V3_Pre_પદાવલિ_Token& token) {
        જો (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ધકેલો_back(token);
    }
    bool result() {
        ધકેલો_આગત(V3_Pre_પદાવલિ_Token{m_firstFileline, V3_Pre_પદાવલિ_Token::END});
        parse();
        પાછા કાઢો_કિંમત().કિંમત();
    }
    સ્થિર શૂન્ય selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
