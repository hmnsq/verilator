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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3PREEXPR_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3PREEXPR_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"

#ಅನ್ತರ್ಭಾವ <deque>

class V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // Order of enum must match token table
    ಎಣಿಕೆ token_t : uint8_t { ZERO, ONE, ಕೊನೆ_2, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

ಖಾಸಗಿ:
    // MEMBERS
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ m_fileline;  // Token fileline
    token_t ಸ್ಥಿರಾಂಕ m_token;  // Token value
ಸಾರ್ವಜನಿಕ:
    // METHODS
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token(File_ಸಾಲು* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token(File_ಸಾಲು* fileline, bool ಮೌಲ್ಯ)
        : m_fileline{fileline}
        , m_token{ಮೌಲ್ಯ ? ONE : ZERO} {}
    ~V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಹೆಸರುಗಳು[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_token];
    }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fileline; }
    token_t token() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_token; }
    bool is_ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_token == ZERO || m_token == ONE; }
    bool ಮೌಲ್ಯ() ಸ್ಥಿರಾಂಕ {
        UASSERT(is_ಮೌಲ್ಯ(), "preproc expr fetch of non-value");
        ಹಿಂತಿರುಗಿ m_token == ONE;
    }
};

class V3_Pre_ಅಭಿವ್ಯಕ್ತಿ final {
    // MEMBERS
    std::deque<V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token> m_ಮೌಲ್ಯಗಳು;  // Value stack
    std::deque<V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token> m_ops;  // Operator stack
    File_ಸಾಲು* m_ಮೊದಲ_Fileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    ಎಣಿಕೆ action_t : uint8_t {
        VV,  //ಮೌಲ್ಯ
        AA,  // Accept
        RR,  // Reduce
        SS,  //ಸರಿಸು
        EE  // Error
    };
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* actionAscii(action_t en) {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಹೆಸರುಗಳು[] = {"VV", "AA", "RR", "SS", "EE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[en];
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

    action_t parse_ಕೋಷ್ಟಕ[V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::MAX][V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::MAX] = {
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

    ಸ್ಥಿರ ಶೂನ್ಯ selfTestOne(ಸ್ಥಿರಾಂಕ string& ಅಭಿವ್ಯಕ್ತಿ, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << ಅಭಿವ್ಯಕ್ತಿ);
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nullptr;
        V3_Pre_ಅಭಿವ್ಯಕ್ತಿ parser;
        parser.reset(flp);
        bool tstart = true;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = ಅಭಿವ್ಯಕ್ತಿ.c_str(); *cp; ++cp) {
            ಆದರೆ (tstart) {
                tstart = false;
                ಆಯ್ಕೆ (*cp) {
                ಸ್ಥಿತಿ '0': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ZERO}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '1': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ONE}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '!': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LNOT}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '|': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LOR}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '&': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LAND}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '-': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::IMP}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '<': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::EQV}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '(': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA}); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ ')': parser.ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::KET}); ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == ' ') {
                tstart = true;
            }
        }
        ಸ್ಥಿರಾಂಕ bool got = parser.ಫಲಿತಾಂಶ();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    ಶೂನ್ಯ ತಳ್ಳು_Op(ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ತಳ್ಳು_ಹಿಂಭಾಗ(token);
    }
    ಶೂನ್ಯ ತಳ್ಳು_ಮೌಲ್ಯ(ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_ಮೌಲ್ಯಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(token);
    }
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token ತೆಗೆ_ಮೌಲ್ಯ() {
        ಆದರೆ (m_ಮೌಲ್ಯಗಳು.ಖಾಲಿಗಳು()) {
            m_ಮೊದಲ_Fileline->v3error("Syntax error in `ifdef () expression");
            ಹಿಂತಿರುಗಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{m_ಮೊದಲ_Fileline, false};
        }
        ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token tok = m_ಮೌಲ್ಯಗಳು.ಹಿಂಭಾಗ();
        m_ಮೌಲ್ಯಗಳು.ತೆಗೆ_ಹಿಂಭಾಗ();
        // UINFO(9, "  popValue " << tok.ascii());
        ಹಿಂತಿರುಗಿ tok;
    }
    ಶೂನ್ಯ reduce() {
        UASSERT(!m_ops.ಖಾಲಿಗಳು(), "lost op stack beginning END");
        ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token tok = m_ops.ಹಿಂಭಾಗ();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.ತೆಗೆ_ಹಿಂಭಾಗ();
        ಆಯ್ಕೆ (tok.token()) {
        ಸ್ಥಿತಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::KET: {
            ಆಗ (m_ops.ಹಿಂಭಾಗ().token() != V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ಕೊನೆ_2
                   && m_ops.ಹಿಂಭಾಗ().token() != V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA)
                reduce();
            ಆದರೆ (m_ops.ಹಿಂಭಾಗ().token() == V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::BRA) {
                m_ops.ತೆಗೆ_ಹಿಂಭಾಗ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                ಹಿಂತಿರುಗಿ;
            }
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LNOT: {
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), !lhs.ಮೌಲ್ಯ()});
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LAND: {
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), lhs.ಮೌಲ್ಯ() && rhs.ಮೌಲ್ಯ()});
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::LOR: {
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), lhs.ಮೌಲ್ಯ() || rhs.ಮೌಲ್ಯ()});
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::IMP: {
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), !lhs.ಮೌಲ್ಯ() || rhs.ಮೌಲ್ಯ()});
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::EQV: {
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token rhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token lhs = ತೆಗೆ_ಮೌಲ್ಯ();
            ತಳ್ಳು_ಮೌಲ್ಯ(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{tok.fileline(), lhs.ಮೌಲ್ಯ() == rhs.ಮೌಲ್ಯ()});
            ನಿಲ್ಲಿಸು;
        }
        ಪೂರ್ವನಿಯೋಜಿತ: {
            v3fatalSrc("bad case on operand stack");
            ನಿಲ್ಲಿಸು;
        }
        }
    }
    ಶೂನ್ಯ parse() {
        ಆಗ (!m_inputs.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token tok = m_inputs.front();
            m_inputs.ತೆಗೆ_front();
            UINFO(9, "input read " << tok.ascii());
            ಆದರೆ (tok.is_ಮೌಲ್ಯ()) {
                ತಳ್ಳು_ಮೌಲ್ಯ(tok);
                ಮುಂದುವರಿ;
            }

            UASSERT(!m_ops.ಖಾಲಿಗಳು(), "lost op stack beginning END");
            ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token ತುದಿ_Tok = m_ops.ಹಿಂಭಾಗ();
            ಸ್ವಯಂಚಾಲಿತ action = parse_ಕೋಷ್ಟಕ[ತುದಿ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << ತುದಿ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            ಆಯ್ಕೆ (action) {
            ಸ್ಥಿತಿ RR:  // Reduce
                reduce();
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ SS:  //ಸರಿಸು
                m_ops.ತಳ್ಳು_ಹಿಂಭಾಗ(tok);
                ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ AA:  // Accept
                ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: tok.fileline()->v3error("Syntax error in `ifdef () expression"); ಹಿಂತಿರುಗಿ;
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    // METHODS
    V3_Pre_ಅಭಿವ್ಯಕ್ತಿ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3_Pre_ಅಭಿವ್ಯಕ್ತಿ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ reset(File_ಸಾಲು* flp) {
        m_inputs.clear();
        m_ಮೌಲ್ಯಗಳು.clear();
        m_ops.clear();
        m_ಮೊದಲ_Fileline = flp;
        ತಳ್ಳು_Op(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{flp, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ಕೊನೆ_2});
    }
    ಶೂನ್ಯ ತಳ್ಳು_ಒಳಹರಿವು(ಸ್ಥಿರಾಂಕ V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token& token) {
        ಆದರೆ (!m_ಮೊದಲ_Fileline) m_ಮೊದಲ_Fileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ತಳ್ಳು_ಹಿಂಭಾಗ(token);
    }
    bool ಫಲಿತಾಂಶ() {
        ತಳ್ಳು_ಒಳಹರಿವು(V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token{m_ಮೊದಲ_Fileline, V3_Pre_ಅಭಿವ್ಯಕ್ತಿ_Token::ಕೊನೆ_2});
        parse();
        ಹಿಂತಿರುಗಿ ತೆಗೆ_ಮೌಲ್ಯ().ಮೌಲ್ಯ();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
