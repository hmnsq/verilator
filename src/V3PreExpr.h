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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3FileLine.h"
#অন্তর্ভাৱ "V3Global.h"

#অন্তর্ভাৱ <deque>

class V3_Pre_ৰাশিমালা_Token final {
সার্ৱজনিক:
    // TYPES
    // Order of enum must match token table
    গণনা token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

গুপ্ত:
    // MEMBERS
    File_সাৰি* ধ্রুৱ m_fileline;  // Token fileline
    token_t ধ্রুৱ m_token;  // Token value
সার্ৱজনিক:
    // METHODS
    V3_Pre_ৰাশিমালা_Token(File_সাৰি* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_ৰাশিমালা_Token(File_সাৰি* fileline, bool মান)
        : m_fileline{fileline}
        , m_token{মান ? ONE : ZERO} {}
    ~V3_Pre_ৰাশিমালা_Token() = স্ৱভাৱ;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির ধ্রুৱ ৱর্ণ* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        ঘূৰাই names[m_token];
    }
    File_সাৰি* fileline() ধ্রুৱ { ঘূৰাই m_fileline; }
    token_t token() ধ্রুৱ { ঘূৰাই m_token; }
    bool is_মান() ধ্রুৱ { ঘূৰাই m_token == ZERO || m_token == ONE; }
    bool মান() ধ্রুৱ {
        UASSERT(is_মান(), "preproc expr fetch of non-value");
        ঘূৰাই m_token == ONE;
    }
};

class V3_Pre_ৰাশিমালা final {
    // MEMBERS
    std::deque<V3_Pre_ৰাশিমালা_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_ৰাশিমালা_Token> m_মানসমূহ;  // Value stack
    std::deque<V3_Pre_ৰাশিমালা_Token> m_ops;  // Operator stack
    File_সাৰি* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    গণনা action_t : uint8_t {
        VV,  //মান
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    স্থির ধ্রুৱ ৱর্ণ* actionAscii(action_t en) {
        স্থির constexpr ধ্রুৱ ৱর্ণ* names[] = {"VV", "AA", "RR", "SS", "EE"};
        ঘূৰাই names[en];
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

    action_t parseTable[V3_Pre_ৰাশিমালা_Token::MAX][V3_Pre_ৰাশিমালা_Token::MAX] = {
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

    স্থির শূন্য selfTestImp() {
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

    স্থির শূন্য selfTestOne(ধ্রুৱ string& ৰাশিমালা, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << ৰাশিমালা);
        File_সাৰি* ধ্রুৱ flp = nullptr;
        V3_Pre_ৰাশিমালা parser;
        parser.reset(flp);
        bool tstart = true;
        বাবে (ধ্রুৱ ৱর্ণ* cp = ৰাশিমালা.c_str(); *cp; ++cp) {
            যদি (tstart) {
                tstart = false;
                ৱিকল্প (*cp) {
                পক্ষ '0': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::ZERO}); ভঙ্গ;
                পক্ষ '1': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::ONE}); ভঙ্গ;
                পক্ষ '!': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::LNOT}); ভঙ্গ;
                পক্ষ '|': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::LOR}); ভঙ্গ;
                পক্ষ '&': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::LAND}); ভঙ্গ;
                পক্ষ '-': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::IMP}); ভঙ্গ;
                পক্ষ '<': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::EQV}); ভঙ্গ;
                পক্ষ '(': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::BRA}); ভঙ্গ;
                পক্ষ ')': parser.ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::KET}); ভঙ্গ;
                স্ৱভাৱ: ভঙ্গ;
                }
            } অন্যথা যদি (*cp == ' ') {
                tstart = true;
            }
        }
        ধ্রুৱ bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    শূন্য ঠেলো_Op(ধ্রুৱ V3_Pre_ৰাশিমালা_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ঠেলো_back(token);
    }
    শূন্য ঠেলো_মান(ধ্রুৱ V3_Pre_ৰাশিমালা_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_মানসমূহ.ঠেলো_back(token);
    }
    V3_Pre_ৰাশিমালা_Token বেৰ_কৰো_মান() {
        যদি (m_মানসমূহ.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            ঘূৰাই V3_Pre_ৰাশিমালা_Token{m_firstFileline, false};
        }
        ধ্রুৱ V3_Pre_ৰাশিমালা_Token tok = m_মানসমূহ.back();
        m_মানসমূহ.বেৰ_কৰো_back();
        // UINFO(9, "  popValue " << tok.ascii());
        ঘূৰাই tok;
    }
    শূন্য reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ধ্রুৱ V3_Pre_ৰাশিমালা_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.বেৰ_কৰো_back();
        ৱিকল্প (tok.token()) {
        পক্ষ V3_Pre_ৰাশিমালা_Token::KET: {
            যেতিয়ালৈ (m_ops.back().token() != V3_Pre_ৰাশিমালা_Token::END
                   && m_ops.back().token() != V3_Pre_ৰাশিমালা_Token::BRA)
                reduce();
            যদি (m_ops.back().token() == V3_Pre_ৰাশিমালা_Token::BRA) {
                m_ops.বেৰ_কৰো_back();
            } অন্যথা {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                ঘূৰাই;
            }
            ভঙ্গ;
        }
        পক্ষ V3_Pre_ৰাশিমালা_Token::LNOT: {
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token lhs = বেৰ_কৰো_মান();
            ঠেলো_মান(V3_Pre_ৰাশিমালা_Token{tok.fileline(), !lhs.মান()});
            ভঙ্গ;
        }
        পক্ষ V3_Pre_ৰাশিমালা_Token::LAND: {
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token rhs = বেৰ_কৰো_মান();
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token lhs = বেৰ_কৰো_মান();
            ঠেলো_মান(V3_Pre_ৰাশিমালা_Token{tok.fileline(), lhs.মান() && rhs.মান()});
            ভঙ্গ;
        }
        পক্ষ V3_Pre_ৰাশিমালা_Token::LOR: {
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token rhs = বেৰ_কৰো_মান();
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token lhs = বেৰ_কৰো_মান();
            ঠেলো_মান(V3_Pre_ৰাশিমালা_Token{tok.fileline(), lhs.মান() || rhs.মান()});
            ভঙ্গ;
        }
        পক্ষ V3_Pre_ৰাশিমালা_Token::IMP: {
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token rhs = বেৰ_কৰো_মান();
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token lhs = বেৰ_কৰো_মান();
            ঠেলো_মান(V3_Pre_ৰাশিমালা_Token{tok.fileline(), !lhs.মান() || rhs.মান()});
            ভঙ্গ;
        }
        পক্ষ V3_Pre_ৰাশিমালা_Token::EQV: {
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token rhs = বেৰ_কৰো_মান();
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token lhs = বেৰ_কৰো_মান();
            ঠেলো_মান(V3_Pre_ৰাশিমালা_Token{tok.fileline(), lhs.মান() == rhs.মান()});
            ভঙ্গ;
        }
        স্ৱভাৱ: {
            v3fatalSrc("bad case on operand stack");
            ভঙ্গ;
        }
        }
    }
    শূন্য parse() {
        যেতিয়ালৈ (!m_inputs.empty()) {
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token tok = m_inputs.front();
            m_inputs.বেৰ_কৰো_front();
            UINFO(9, "input read " << tok.ascii());
            যদি (tok.is_মান()) {
                ঠেলো_মান(tok);
                অনুৱর্ত;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ধ্রুৱ V3_Pre_ৰাশিমালা_Token শীৰ্ষ_Tok = m_ops.back();
            auto action = parseTable[শীৰ্ষ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << শীৰ্ষ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            ৱিকল্প (action) {
            পক্ষ RR:  // Reduce
                reduce();
                ভঙ্গ;
            পক্ষ SS:  // Shift
                m_ops.ঠেলো_back(tok);
                ভঙ্গ;
            পক্ষ AA:  // Accept
                ভঙ্গ;
            স্ৱভাৱ: tok.fileline()->v3error("Syntax error in `ifdef () expression"); ঘূৰাই;
            }
        }
    }

সার্ৱজনিক:
    // METHODS
    V3_Pre_ৰাশিমালা() = স্ৱভাৱ;
    ~V3_Pre_ৰাশিমালা() = স্ৱভাৱ;
    শূন্য reset(File_সাৰি* flp) {
        m_inputs.clear();
        m_মানসমূহ.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ঠেলো_Op(V3_Pre_ৰাশিমালা_Token{flp, V3_Pre_ৰাশিমালা_Token::END});
    }
    শূন্য ঠেলো_প্ৰবেশ(ধ্রুৱ V3_Pre_ৰাশিমালা_Token& token) {
        যদি (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ঠেলো_back(token);
    }
    bool result() {
        ঠেলো_প্ৰবেশ(V3_Pre_ৰাশিমালা_Token{m_firstFileline, V3_Pre_ৰাশিমালা_Token::END});
        parse();
        ঘূৰাই বেৰ_কৰো_মান().মান();
    }
    স্থির শূন্য selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
