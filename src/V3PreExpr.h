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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Global.h"

#अन्तर्भाव <deque>

class V3_Pre_व्यंजक_Token final {
सार्वजनिक:
    // TYPES
    // Order of enum must match token table
    गणना token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

गुप्त:
    // MEMBERS
    File_पंक्ति* ध्रुव m_fileline;  // Token fileline
    token_t ध्रुव m_token;  // Token value
सार्वजनिक:
    // METHODS
    V3_Pre_व्यंजक_Token(File_पंक्ति* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_व्यंजक_Token(File_पंक्ति* fileline, bool मूल्य)
        : m_fileline{fileline}
        , m_token{मूल्य ? ONE : ZERO} {}
    ~V3_Pre_व्यंजक_Token() = स्वभाव;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर ध्रुव वर्ण* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        प्रत्यागम names[m_token];
    }
    File_पंक्ति* fileline() ध्रुव { प्रत्यागम m_fileline; }
    token_t token() ध्रुव { प्रत्यागम m_token; }
    bool is_मूल्य() ध्रुव { प्रत्यागम m_token == ZERO || m_token == ONE; }
    bool मूल्य() ध्रुव {
        UASSERT(is_मूल्य(), "preproc expr fetch of non-value");
        प्रत्यागम m_token == ONE;
    }
};

class V3_Pre_व्यंजक final {
    // MEMBERS
    std::deque<V3_Pre_व्यंजक_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_व्यंजक_Token> m_मूल्यावली;  // Value stack
    std::deque<V3_Pre_व्यंजक_Token> m_ops;  // Operator stack
    File_पंक्ति* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    गणना action_t : uint8_t {
        VV,  //मूल्य
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    स्थिर ध्रुव वर्ण* actionAscii(action_t en) {
        स्थिर constexpr ध्रुव वर्ण* names[] = {"VV", "AA", "RR", "SS", "EE"};
        प्रत्यागम names[en];
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

    action_t parseTable[V3_Pre_व्यंजक_Token::MAX][V3_Pre_व्यंजक_Token::MAX] = {
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

    स्थिर शून्य selfTestImp() {
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

    स्थिर शून्य selfTestOne(ध्रुव string& व्यंजक, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << व्यंजक);
        File_पंक्ति* ध्रुव flp = nullptr;
        V3_Pre_व्यंजक parser;
        parser.reset(flp);
        bool tstart = true;
        प्रति (ध्रुव वर्ण* cp = व्यंजक.c_str(); *cp; ++cp) {
            यदि (tstart) {
                tstart = false;
                विकल्प (*cp) {
                पक्ष '0': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::ZERO}); भङ्ग;
                पक्ष '1': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::ONE}); भङ्ग;
                पक्ष '!': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LNOT}); भङ्ग;
                पक्ष '|': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LOR}); भङ्ग;
                पक्ष '&': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::LAND}); भङ्ग;
                पक्ष '-': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::IMP}); भङ्ग;
                पक्ष '<': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::EQV}); भङ्ग;
                पक्ष '(': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::BRA}); भङ्ग;
                पक्ष ')': parser.धकेलो_निवेश(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::KET}); भङ्ग;
                स्वभाव: भङ्ग;
                }
            } अन्यथा यदि (*cp == ' ') {
                tstart = true;
            }
        }
        ध्रुव bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    शून्य धकेलो_Op(ध्रुव V3_Pre_व्यंजक_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.धकेलो_back(token);
    }
    शून्य धकेलो_मूल्य(ध्रुव V3_Pre_व्यंजक_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_मूल्यावली.धकेलो_back(token);
    }
    V3_Pre_व्यंजक_Token निकालो_मूल्य() {
        यदि (m_मूल्यावली.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            प्रत्यागम V3_Pre_व्यंजक_Token{m_firstFileline, false};
        }
        ध्रुव V3_Pre_व्यंजक_Token tok = m_मूल्यावली.back();
        m_मूल्यावली.निकालो_back();
        // UINFO(9, "  popValue " << tok.ascii());
        प्रत्यागम tok;
    }
    शून्य reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        ध्रुव V3_Pre_व्यंजक_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.निकालो_back();
        विकल्प (tok.token()) {
        पक्ष V3_Pre_व्यंजक_Token::KET: {
            यावत् (m_ops.back().token() != V3_Pre_व्यंजक_Token::END
                   && m_ops.back().token() != V3_Pre_व्यंजक_Token::BRA)
                reduce();
            यदि (m_ops.back().token() == V3_Pre_व्यंजक_Token::BRA) {
                m_ops.निकालो_back();
            } अन्यथा {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                प्रत्यागम;
            }
            भङ्ग;
        }
        पक्ष V3_Pre_व्यंजक_Token::LNOT: {
            ध्रुव V3_Pre_व्यंजक_Token lhs = निकालो_मूल्य();
            धकेलो_मूल्य(V3_Pre_व्यंजक_Token{tok.fileline(), !lhs.मूल्य()});
            भङ्ग;
        }
        पक्ष V3_Pre_व्यंजक_Token::LAND: {
            ध्रुव V3_Pre_व्यंजक_Token rhs = निकालो_मूल्य();
            ध्रुव V3_Pre_व्यंजक_Token lhs = निकालो_मूल्य();
            धकेलो_मूल्य(V3_Pre_व्यंजक_Token{tok.fileline(), lhs.मूल्य() && rhs.मूल्य()});
            भङ्ग;
        }
        पक्ष V3_Pre_व्यंजक_Token::LOR: {
            ध्रुव V3_Pre_व्यंजक_Token rhs = निकालो_मूल्य();
            ध्रुव V3_Pre_व्यंजक_Token lhs = निकालो_मूल्य();
            धकेलो_मूल्य(V3_Pre_व्यंजक_Token{tok.fileline(), lhs.मूल्य() || rhs.मूल्य()});
            भङ्ग;
        }
        पक्ष V3_Pre_व्यंजक_Token::IMP: {
            ध्रुव V3_Pre_व्यंजक_Token rhs = निकालो_मूल्य();
            ध्रुव V3_Pre_व्यंजक_Token lhs = निकालो_मूल्य();
            धकेलो_मूल्य(V3_Pre_व्यंजक_Token{tok.fileline(), !lhs.मूल्य() || rhs.मूल्य()});
            भङ्ग;
        }
        पक्ष V3_Pre_व्यंजक_Token::EQV: {
            ध्रुव V3_Pre_व्यंजक_Token rhs = निकालो_मूल्य();
            ध्रुव V3_Pre_व्यंजक_Token lhs = निकालो_मूल्य();
            धकेलो_मूल्य(V3_Pre_व्यंजक_Token{tok.fileline(), lhs.मूल्य() == rhs.मूल्य()});
            भङ्ग;
        }
        स्वभाव: {
            v3fatalSrc("bad case on operand stack");
            भङ्ग;
        }
        }
    }
    शून्य parse() {
        यावत् (!m_inputs.empty()) {
            ध्रुव V3_Pre_व्यंजक_Token tok = m_inputs.front();
            m_inputs.निकालो_front();
            UINFO(9, "input read " << tok.ascii());
            यदि (tok.is_मूल्य()) {
                धकेलो_मूल्य(tok);
                अनुवर्त;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            ध्रुव V3_Pre_व्यंजक_Token शीर्ष_Tok = m_ops.back();
            auto action = parseTable[शीर्ष_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << शीर्ष_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            विकल्प (action) {
            पक्ष RR:  // Reduce
                reduce();
                भङ्ग;
            पक्ष SS:  // Shift
                m_ops.धकेलो_back(tok);
                भङ्ग;
            पक्ष AA:  // Accept
                भङ्ग;
            स्वभाव: tok.fileline()->v3error("Syntax error in `ifdef () expression"); प्रत्यागम;
            }
        }
    }

सार्वजनिक:
    // METHODS
    V3_Pre_व्यंजक() = स्वभाव;
    ~V3_Pre_व्यंजक() = स्वभाव;
    शून्य reset(File_पंक्ति* flp) {
        m_inputs.clear();
        m_मूल्यावली.clear();
        m_ops.clear();
        m_firstFileline = flp;
        धकेलो_Op(V3_Pre_व्यंजक_Token{flp, V3_Pre_व्यंजक_Token::END});
    }
    शून्य धकेलो_निवेश(ध्रुव V3_Pre_व्यंजक_Token& token) {
        यदि (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.धकेलो_back(token);
    }
    bool result() {
        धकेलो_निवेश(V3_Pre_व्यंजक_Token{m_firstFileline, V3_Pre_व्यंजक_Token::END});
        parse();
        प्रत्यागम निकालो_मूल्य().मूल्य();
    }
    स्थिर शून्य selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
