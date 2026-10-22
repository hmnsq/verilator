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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Global.h"

#অন্তর্ভাব <deque>

class V3_Pre_রাশিমালা_Token final {
সর্বজনীন:
    // TYPES
    // Order of enum must match token table
    গণনা token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

ব্যক্তিগত:
    // MEMBERS
    File_সারি* ধ্রুবক m_fileline;  // Token fileline
    token_t ধ্রুবক m_token;  // Token value
সর্বজনীন:
    // METHODS
    V3_Pre_রাশিমালা_Token(File_সারি* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_রাশিমালা_Token(File_সারি* fileline, bool মান)
        : m_fileline{fileline}
        , m_token{মান ? ONE : ZERO} {}
    ~V3_Pre_রাশিমালা_Token() = পূর্বনির্ধারিত;
    ধ্রুবক অক্ষর* ascii() ধ্রুবক {
        স্থির ধ্রুবক অক্ষর* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        ফেরত names[m_token];
    }
    File_সারি* fileline() ধ্রুবক { ফেরত m_fileline; }
    token_t token() ধ্রুবক { ফেরত m_token; }
    bool is_মান() ধ্রুবক { ফেরত m_token == ZERO || m_token == ONE; }
    bool মান() ধ্রুবক {
        UASSERT(is_মান(), "preproc expr fetch of non-value");
        ফেরত m_token == ONE;
    }
};

class V3_Pre_রাশিমালা final {
    // MEMBERS
    std::deque<V3_Pre_রাশিমালা_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_রাশিমালা_Token> m_মানসমূহ;  // Value stack
    std::deque<V3_Pre_রাশিমালা_Token> m_ops;  // Operator stack
    File_সারি* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    গণনা action_t : uint8_t {
        VV,  //মান
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    স্থির ধ্রুবক অক্ষর* actionAscii(action_t en) {
        স্থির constexpr ধ্রুবক অক্ষর* names[] = {"VV", "AA", "RR", "SS", "EE"};
        ফেরত names[en];
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

    action_t parseTable[V3_Pre_রাশিমালা_Token::MAX][V3_Pre_রাশিমালা_Token::MAX] = {
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

    স্থির শূন্য selfTestOne(ধ্রুবক string& রাশিমালা, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << রাশিমালা);
        File_সারি* ধ্রুবক flp = nullptr;
        V3_Pre_রাশিমালা parser;
        parser.reset(flp);
        bool tstart = true;
        জন্য (ধ্রুবক অক্ষর* cp = রাশিমালা.c_str(); *cp; ++cp) {
            যদি (tstart) {
                tstart = false;
                নির্বাচন (*cp) {
                ক্ষেত্র '0': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::ZERO}); বিরতি;
                ক্ষেত্র '1': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::ONE}); বিরতি;
                ক্ষেত্র '!': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::LNOT}); বিরতি;
                ক্ষেত্র '|': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::LOR}); বিরতি;
                ক্ষেত্র '&': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::LAND}); বিরতি;
                ক্ষেত্র '-': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::IMP}); বিরতি;
                ক্ষেত্র '<': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::EQV}); বিরতি;
                ক্ষেত্র '(': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::BRA}); বিরতি;
                ক্ষেত্র ')': parser.ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::KET}); বিরতি;
                পূর্বনির্ধারিত: বিরতি;
                }
            } অন্যথা যদি (*cp == ' ') {
                tstart = true;
            }
        }
        ধ্রুবক bool got = parser.ফলাফল();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    শূন্য ঠেলো_Op(ধ্রুবক V3_Pre_রাশিমালা_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ঠেলো_back(token);
    }
    শূন্য ঠেলো_মান(ধ্রুবক V3_Pre_রাশিমালা_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_মানসমূহ.ঠেলো_back(token);
    }
    V3_Pre_রাশিমালা_Token বের_করো_মান() {
        যদি (m_মানসমূহ.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            ফেরত V3_Pre_রাশিমালা_Token{m_firstFileline, false};
        }
        ধ্রুবক V3_Pre_রাশিমালা_Token tok = m_মানসমূহ.back();
        m_মানসমূহ.বের_করো_back();
        // UINFO(9, "  popValue " << tok.ascii());
        ফেরত tok;
    }
    শূন্য reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ধ্রুবক V3_Pre_রাশিমালা_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.বের_করো_back();
        নির্বাচন (tok.token()) {
        ক্ষেত্র V3_Pre_রাশিমালা_Token::KET: {
            যখন (m_ops.back().token() != V3_Pre_রাশিমালা_Token::END
                   && m_ops.back().token() != V3_Pre_রাশিমালা_Token::BRA)
                reduce();
            যদি (m_ops.back().token() == V3_Pre_রাশিমালা_Token::BRA) {
                m_ops.বের_করো_back();
            } অন্যথা {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                ফেরত;
            }
            বিরতি;
        }
        ক্ষেত্র V3_Pre_রাশিমালা_Token::LNOT: {
            ধ্রুবক V3_Pre_রাশিমালা_Token lhs = বের_করো_মান();
            ঠেলো_মান(V3_Pre_রাশিমালা_Token{tok.fileline(), !lhs.মান()});
            বিরতি;
        }
        ক্ষেত্র V3_Pre_রাশিমালা_Token::LAND: {
            ধ্রুবক V3_Pre_রাশিমালা_Token rhs = বের_করো_মান();
            ধ্রুবক V3_Pre_রাশিমালা_Token lhs = বের_করো_মান();
            ঠেলো_মান(V3_Pre_রাশিমালা_Token{tok.fileline(), lhs.মান() && rhs.মান()});
            বিরতি;
        }
        ক্ষেত্র V3_Pre_রাশিমালা_Token::LOR: {
            ধ্রুবক V3_Pre_রাশিমালা_Token rhs = বের_করো_মান();
            ধ্রুবক V3_Pre_রাশিমালা_Token lhs = বের_করো_মান();
            ঠেলো_মান(V3_Pre_রাশিমালা_Token{tok.fileline(), lhs.মান() || rhs.মান()});
            বিরতি;
        }
        ক্ষেত্র V3_Pre_রাশিমালা_Token::IMP: {
            ধ্রুবক V3_Pre_রাশিমালা_Token rhs = বের_করো_মান();
            ধ্রুবক V3_Pre_রাশিমালা_Token lhs = বের_করো_মান();
            ঠেলো_মান(V3_Pre_রাশিমালা_Token{tok.fileline(), !lhs.মান() || rhs.মান()});
            বিরতি;
        }
        ক্ষেত্র V3_Pre_রাশিমালা_Token::EQV: {
            ধ্রুবক V3_Pre_রাশিমালা_Token rhs = বের_করো_মান();
            ধ্রুবক V3_Pre_রাশিমালা_Token lhs = বের_করো_মান();
            ঠেলো_মান(V3_Pre_রাশিমালা_Token{tok.fileline(), lhs.মান() == rhs.মান()});
            বিরতি;
        }
        পূর্বনির্ধারিত: {
            v3fatalSrc("bad case on operand stack");
            বিরতি;
        }
        }
    }
    শূন্য parse() {
        যখন (!m_inputs.empty()) {
            ধ্রুবক V3_Pre_রাশিমালা_Token tok = m_inputs.front();
            m_inputs.বের_করো_front();
            UINFO(9, "input read " << tok.ascii());
            যদি (tok.is_মান()) {
                ঠেলো_মান(tok);
                চালু;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ধ্রুবক V3_Pre_রাশিমালা_Token শীর্ষ_Tok = m_ops.back();
            auto action = parseTable[শীর্ষ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << শীর্ষ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            নির্বাচন (action) {
            ক্ষেত্র RR:  // Reduce
                reduce();
                বিরতি;
            ক্ষেত্র SS:  // Shift
                m_ops.ঠেলো_back(tok);
                বিরতি;
            ক্ষেত্র AA:  // Accept
                বিরতি;
            পূর্বনির্ধারিত: tok.fileline()->v3error("Syntax error in `ifdef () expression"); ফেরত;
            }
        }
    }

সর্বজনীন:
    // METHODS
    V3_Pre_রাশিমালা() = পূর্বনির্ধারিত;
    ~V3_Pre_রাশিমালা() = পূর্বনির্ধারিত;
    শূন্য reset(File_সারি* flp) {
        m_inputs.clear();
        m_মানসমূহ.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ঠেলো_Op(V3_Pre_রাশিমালা_Token{flp, V3_Pre_রাশিমালা_Token::END});
    }
    শূন্য ঠেলো_প্রবেশ(ধ্রুবক V3_Pre_রাশিমালা_Token& token) {
        যদি (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ঠেলো_back(token);
    }
    bool ফলাফল() {
        ঠেলো_প্রবেশ(V3_Pre_রাশিমালা_Token{m_firstFileline, V3_Pre_রাশিমালা_Token::END});
        parse();
        ফেরত বের_করো_মান().মান();
    }
    স্থির শূন্য selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
