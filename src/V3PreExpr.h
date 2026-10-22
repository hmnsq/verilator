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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <deque>

class V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // Order of enum must match token table
    ಗಣನಾ token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

ಗುಪ್ತ:
    // MEMBERS
    File_ಸಾಲು* ಧ್ರುವ m_fileline;  // Token fileline
    token_t ಧ್ರುವ m_token;  // Token value
ಸಾರ್ವಜನಿಕ:
    // METHODS
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token(File_ಸಾಲು* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token(File_ಸಾಲು* fileline, bool ಮೌಲ್ಯ)
        : m_fileline{fileline}
        , m_token{ಮೌಲ್ಯ ? ONE : ZERO} {}
    ~V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token() = ಸ್ವಭಾವ;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        ಪ್ರತ್ಯಾಗಮ names[m_token];
    }
    File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fileline; }
    token_t token() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_token; }
    bool is_ಮೌಲ್ಯ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_token == ZERO || m_token == ONE; }
    bool ಮೌಲ್ಯ() ಧ್ರುವ {
        UASSERT(is_ಮೌಲ್ಯ(), "preproc expr fetch of non-value");
        ಪ್ರತ್ಯಾಗಮ m_token == ONE;
    }
};

class V3_Pre_ಅಭಿವ್ಯಕ್ತಿ final {
    // MEMBERS
    std::deque<V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token> m_ಮೌಲ್ಯಗಳು;  // Value stack
    std::deque<V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token> m_ops;  // Operator stack
    File_ಸಾಲು* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    ಗಣನಾ action_t : uint8_t {
        VV,  //ಮೌಲ್ಯ
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* actionAscii(action_t en) {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* names[] = {"VV", "AA", "RR", "SS", "EE"};
        ಪ್ರತ್ಯಾಗಮ names[en];
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

    action_t parseTable[V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::MAX][V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::MAX] = {
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

    ಸ್ಥಿರ ಶೂನ್ಯ selfTestImp() {
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

    ಸ್ಥಿರ ಶೂನ್ಯ selfTestOne(ಧ್ರುವ string& ಅಭಿವ್ಯಕ್ತಿ, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << ಅಭಿವ್ಯಕ್ತಿ);
        File_ಸಾಲು* ಧ್ರುವ flp = nullptr;
        V3_Pre_ಅಭಿವ್ಯಕ್ತಿ parser;
        parser.reset(flp);
        bool tstart = true;
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = ಅಭಿವ್ಯಕ್ತಿ.c_str(); *cp; ++cp) {
            ಯದಿ (tstart) {
                tstart = false;
                ವಿಕಲ್ಪ (*cp) {
                ಪಕ್ಷ '0': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ZERO}); ಭಙ್ಗ;
                ಪಕ್ಷ '1': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ONE}); ಭಙ್ಗ;
                ಪಕ್ಷ '!': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LNOT}); ಭಙ್ಗ;
                ಪಕ್ಷ '|': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LOR}); ಭಙ್ಗ;
                ಪಕ್ಷ '&': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LAND}); ಭಙ್ಗ;
                ಪಕ್ಷ '-': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::IMP}); ಭಙ್ಗ;
                ಪಕ್ಷ '<': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::EQV}); ಭಙ್ಗ;
                ಪಕ್ಷ '(': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA}); ಭಙ್ಗ;
                ಪಕ್ಷ ')': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::KET}); ಭಙ್ಗ;
                ಸ್ವಭಾವ: ಭಙ್ಗ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (*cp == ' ') {
                tstart = true;
            }
        }
        ಧ್ರುವ bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    ಶೂನ್ಯ ತಳ್ಳು_Op(ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ತಳ್ಳು_back(token);
    }
    ಶೂನ್ಯ ತಳ್ಳು_ಮೌಲ್ಯ(ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_ಮೌಲ್ಯಗಳು.ತಳ್ಳು_back(token);
    }
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token ತೆಗೆ_ಮೌಲ್ಯ() {
        ಯದಿ (m_ಮೌಲ್ಯಗಳು.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            ಪ್ರತ್ಯಾಗಮ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{m_firstFileline, false};
        }
        ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token tok = m_ಮೌಲ್ಯಗಳು.back();
        m_ಮೌಲ್ಯಗಳು.ತೆಗೆ_back();
        // UINFO(9, "  popValue " << tok.ascii());
        ಪ್ರತ್ಯಾಗಮ tok;
    }
    ಶೂನ್ಯ reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.ತೆಗೆ_back();
        ವಿಕಲ್ಪ (tok.token()) {
        ಪಕ್ಷ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::KET: {
            ಯಾವತ್ (m_ops.back().token() != V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::END
                   && m_ops.back().token() != V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA)
                reduce();
            ಯದಿ (m_ops.back().token() == V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA) {
                m_ops.ತೆಗೆ_back();
            } ಅನ್ಯಥಾ {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LNOT: {
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), !lhs.ಮೌಲ್ಯ()});
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LAND: {
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), lhs.ಮೌಲ್ಯ() && rhs.ಮೌಲ್ಯ()});
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LOR: {
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), lhs.ಮೌಲ್ಯ() || rhs.ಮೌಲ್ಯ()});
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::IMP: {
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), !lhs.ಮೌಲ್ಯ() || rhs.ಮೌಲ್ಯ()});
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::EQV: {
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), lhs.ಮೌಲ್ಯ() == rhs.ಮೌಲ್ಯ()});
            ಭಙ್ಗ;
        }
        ಸ್ವಭಾವ: {
            v3fatalSrc("bad case on operand stack");
            ಭಙ್ಗ;
        }
        }
    }
    ಶೂನ್ಯ parse() {
        ಯಾವತ್ (!m_inputs.empty()) {
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token tok = m_inputs.front();
            m_inputs.ತೆಗೆ_front();
            UINFO(9, "input read " << tok.ascii());
            ಯದಿ (tok.is_ಮೌಲ್ಯ()) {
                ತಳ್ಳು_ಮೌಲ್ಯ(tok);
                ಅನುವರ್ತ;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token ತುದಿ_Tok = m_ops.back();
            auto action = parseTable[ತುದಿ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << ತುದಿ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            ವಿಕಲ್ಪ (action) {
            ಪಕ್ಷ RR:  // Reduce
                reduce();
                ಭಙ್ಗ;
            ಪಕ್ಷ SS:  // Shift
                m_ops.ತಳ್ಳು_back(tok);
                ಭಙ್ಗ;
            ಪಕ್ಷ AA:  // Accept
                ಭಙ್ಗ;
            ಸ್ವಭಾವ: tok.fileline()->v3error("Syntax error in `ifdef () expression"); ಪ್ರತ್ಯಾಗಮ;
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ() = ಸ್ವಭಾವ;
    ~V3_Pre_ಅಭಿವ್ಯಕ್ತಿ() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ reset(File_ಸಾಲು* flp) {
        m_inputs.clear();
        m_ಮೌಲ್ಯಗಳು.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ತಳ್ಳು_Op(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::END});
    }
    ಶೂನ್ಯ ತಳ್ಳು_ಒಳಹರಿವು(ಧ್ರುವ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token& token) {
        ಯದಿ (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ತಳ್ಳು_back(token);
    }
    bool result() {
        ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{m_firstFileline, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::END});
        parse();
        ಪ್ರತ್ಯಾಗಮ ತೆಗೆ_ಮೌಲ್ಯ().ಮೌಲ್ಯ();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
