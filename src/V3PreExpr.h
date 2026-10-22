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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"

#ଅନ୍ତର୍ଭାଵ <deque>

class V3_Pre_ପଦସମଷ୍ଟି_Token final {
ସର୍ବସାଧାରଣ:
    // TYPES
    // Order of enum must match token table
    ଗଣନା token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    File_ଧାଡ଼ି* ଅଚଳ m_fileline;  // Token fileline
    token_t ଅଚଳ m_token;  // Token value
ସର୍ବସାଧାରଣ:
    // METHODS
    V3_Pre_ପଦସମଷ୍ଟି_Token(File_ଧାଡ଼ି* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_ପଦସମଷ୍ଟି_Token(File_ଧାଡ଼ି* fileline, bool ମୂଲ୍ୟ)
        : m_fileline{fileline}
        , m_token{ମୂଲ୍ୟ ? ONE : ZERO} {}
    ~V3_Pre_ପଦସମଷ୍ଟି_Token() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        ଫେରନ୍ତୁ names[m_token];
    }
    File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ m_fileline; }
    token_t token() ଅଚଳ { ଫେରନ୍ତୁ m_token; }
    bool is_ମୂଲ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ m_token == ZERO || m_token == ONE; }
    bool ମୂଲ୍ୟ() ଅଚଳ {
        UASSERT(is_ମୂଲ୍ୟ(), "preproc expr fetch of non-value");
        ଫେରନ୍ତୁ m_token == ONE;
    }
};

class V3_Pre_ପଦସମଷ୍ଟି final {
    // MEMBERS
    std::deque<V3_Pre_ପଦସମଷ୍ଟି_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_ପଦସମଷ୍ଟି_Token> m_ମୂଲ୍ୟଗୁଡ଼ିକ;  // Value stack
    std::deque<V3_Pre_ପଦସମଷ୍ଟି_Token> m_ops;  // Operator stack
    File_ଧାଡ଼ି* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    ଗଣନା action_t : uint8_t {
        VV,  //ମୂଲ୍ୟ
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* actionAscii(action_t en) {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* names[] = {"VV", "AA", "RR", "SS", "EE"};
        ଫେରନ୍ତୁ names[en];
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

    action_t parseTable[V3_Pre_ପଦସମଷ୍ଟି_Token::MAX][V3_Pre_ପଦସମଷ୍ଟି_Token::MAX] = {
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

    ସ୍ଥିର ଶୂନ୍ୟ selfTestImp() {
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

    ସ୍ଥିର ଶୂନ୍ୟ selfTestOne(ଅଚଳ string& ପଦସମଷ୍ଟି, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << ପଦସମଷ୍ଟି);
        File_ଧାଡ଼ି* ଅଚଳ flp = nullptr;
        V3_Pre_ପଦସମଷ୍ଟି parser;
        parser.reset(flp);
        bool tstart = true;
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = ପଦସମଷ୍ଟି.c_str(); *cp; ++cp) {
            ଯଦି (tstart) {
                tstart = false;
                ବାଛ (*cp) {
                ସ୍ଥିତି '0': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::ZERO}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '1': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::ONE}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '!': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::LNOT}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '|': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::LOR}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '&': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::LAND}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '-': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::IMP}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '<': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::EQV}); ଭାଙ୍ଗ;
                ସ୍ଥିତି '(': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::BRA}); ଭାଙ୍ଗ;
                ସ୍ଥିତି ')': parser.ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::KET}); ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଭାଙ୍ଗ;
                }
            } ଅନ୍ୟଥା ଯଦି (*cp == ' ') {
                tstart = true;
            }
        }
        ଅଚଳ bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    ଶୂନ୍ୟ ଠେଲ_Op(ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.ଠେଲ_back(token);
    }
    ଶୂନ୍ୟ ଠେଲ_ମୂଲ୍ୟ(ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_ମୂଲ୍ୟଗୁଡ଼ିକ.ଠେଲ_back(token);
    }
    V3_Pre_ପଦସମଷ୍ଟି_Token କାଢ଼_ମୂଲ୍ୟ() {
        ଯଦି (m_ମୂଲ୍ୟଗୁଡ଼ିକ.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            ଫେରନ୍ତୁ V3_Pre_ପଦସମଷ୍ଟି_Token{m_firstFileline, false};
        }
        ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token tok = m_ମୂଲ୍ୟଗୁଡ଼ିକ.back();
        m_ମୂଲ୍ୟଗୁଡ଼ିକ.କାଢ଼_back();
        // UINFO(9, "  popValue " << tok.ascii());
        ଫେରନ୍ତୁ tok;
    }
    ଶୂନ୍ୟ reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.କାଢ଼_back();
        ବାଛ (tok.token()) {
        ସ୍ଥିତି V3_Pre_ପଦସମଷ୍ଟି_Token::KET: {
            ଯେତେବେଳେ (m_ops.back().token() != V3_Pre_ପଦସମଷ୍ଟି_Token::END
                   && m_ops.back().token() != V3_Pre_ପଦସମଷ୍ଟି_Token::BRA)
                reduce();
            ଯଦି (m_ops.back().token() == V3_Pre_ପଦସମଷ୍ଟି_Token::BRA) {
                m_ops.କାଢ଼_back();
            } ଅନ୍ୟଥା {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                ଫେରନ୍ତୁ;
            }
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି V3_Pre_ପଦସମଷ୍ଟି_Token::LNOT: {
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token lhs = କାଢ଼_ମୂଲ୍ୟ();
            ଠେଲ_ମୂଲ୍ୟ(V3_Pre_ପଦସମଷ୍ଟି_Token{tok.fileline(), !lhs.ମୂଲ୍ୟ()});
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି V3_Pre_ପଦସମଷ୍ଟି_Token::LAND: {
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token rhs = କାଢ଼_ମୂଲ୍ୟ();
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token lhs = କାଢ଼_ମୂଲ୍ୟ();
            ଠେଲ_ମୂଲ୍ୟ(V3_Pre_ପଦସମଷ୍ଟି_Token{tok.fileline(), lhs.ମୂଲ୍ୟ() && rhs.ମୂଲ୍ୟ()});
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି V3_Pre_ପଦସମଷ୍ଟି_Token::LOR: {
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token rhs = କାଢ଼_ମୂଲ୍ୟ();
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token lhs = କାଢ଼_ମୂଲ୍ୟ();
            ଠେଲ_ମୂଲ୍ୟ(V3_Pre_ପଦସମଷ୍ଟି_Token{tok.fileline(), lhs.ମୂଲ୍ୟ() || rhs.ମୂଲ୍ୟ()});
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି V3_Pre_ପଦସମଷ୍ଟି_Token::IMP: {
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token rhs = କାଢ଼_ମୂଲ୍ୟ();
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token lhs = କାଢ଼_ମୂଲ୍ୟ();
            ଠେଲ_ମୂଲ୍ୟ(V3_Pre_ପଦସମଷ୍ଟି_Token{tok.fileline(), !lhs.ମୂଲ୍ୟ() || rhs.ମୂଲ୍ୟ()});
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି V3_Pre_ପଦସମଷ୍ଟି_Token::EQV: {
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token rhs = କାଢ଼_ମୂଲ୍ୟ();
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token lhs = କାଢ଼_ମୂଲ୍ୟ();
            ଠେଲ_ମୂଲ୍ୟ(V3_Pre_ପଦସମଷ୍ଟି_Token{tok.fileline(), lhs.ମୂଲ୍ୟ() == rhs.ମୂଲ୍ୟ()});
            ଭାଙ୍ଗ;
        }
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: {
            v3fatalSrc("bad case on operand stack");
            ଭାଙ୍ଗ;
        }
        }
    }
    ଶୂନ୍ୟ parse() {
        ଯେତେବେଳେ (!m_inputs.empty()) {
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token tok = m_inputs.front();
            m_inputs.କାଢ଼_front();
            UINFO(9, "input read " << tok.ascii());
            ଯଦି (tok.is_ମୂଲ୍ୟ()) {
                ଠେଲ_ମୂଲ୍ୟ(tok);
                ଜାରି;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token ଶୀର୍ଷ_Tok = m_ops.back();
            auto action = parseTable[ଶୀର୍ଷ_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << ଶୀର୍ଷ_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            ବାଛ (action) {
            ସ୍ଥିତି RR:  // Reduce
                reduce();
                ଭାଙ୍ଗ;
            ସ୍ଥିତି SS:  // Shift
                m_ops.ଠେଲ_back(tok);
                ଭାଙ୍ଗ;
            ସ୍ଥିତି AA:  // Accept
                ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: tok.fileline()->v3error("Syntax error in `ifdef () expression"); ଫେରନ୍ତୁ;
            }
        }
    }

ସର୍ବସାଧାରଣ:
    // METHODS
    V3_Pre_ପଦସମଷ୍ଟି() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3_Pre_ପଦସମଷ୍ଟି() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ reset(File_ଧାଡ଼ି* flp) {
        m_inputs.clear();
        m_ମୂଲ୍ୟଗୁଡ଼ିକ.clear();
        m_ops.clear();
        m_firstFileline = flp;
        ଠେଲ_Op(V3_Pre_ପଦସମଷ୍ଟି_Token{flp, V3_Pre_ପଦସମଷ୍ଟି_Token::END});
    }
    ଶୂନ୍ୟ ଠେଲ_ନିବେଶ(ଅଚଳ V3_Pre_ପଦସମଷ୍ଟି_Token& token) {
        ଯଦି (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.ଠେଲ_back(token);
    }
    bool result() {
        ଠେଲ_ନିବେଶ(V3_Pre_ପଦସମଷ୍ଟି_Token{m_firstFileline, V3_Pre_ପଦସମଷ୍ଟି_Token::END});
        parse();
        ଫେରନ୍ତୁ କାଢ଼_ମୂଲ୍ୟ().ମୂଲ୍ୟ();
    }
    ସ୍ଥିର ଶୂନ୍ୟ selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
