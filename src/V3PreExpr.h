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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Global.h"

#அந்தர்பாவ <deque>

class V3_Pre_கோவை_Token final {
பொது:
    // TYPES
    // Order of enum must match token table
    எண்ணிக்கை token_t : uint8_t { ZERO, ONE, END, BRA, KET, LNOT, LAND, LOR, IMP, EQV, MAX };

தனிப்பட்ட:
    // MEMBERS
    File_வரி* மாறிலி m_fileline;  // Token fileline
    token_t மாறிலி m_token;  // Token value
பொது:
    // METHODS
    V3_Pre_கோவை_Token(File_வரி* fileline, token_t token)
        : m_fileline{fileline}
        , m_token{token} {}
    V3_Pre_கோவை_Token(File_வரி* fileline, bool மதிப்பு)
        : m_fileline{fileline}
        , m_token{மதிப்பு ? ONE : ZERO} {}
    ~V3_Pre_கோவை_Token() = இயல்பு;
    மாறிலி எழுத்து* ascii() மாறிலி {
        நிலையான மாறிலி எழுத்து* names[] = {"0", "1", "$", "(", ")", "!", "&&", "||", "->", "<->"};
        திரும்பு names[m_token];
    }
    File_வரி* fileline() மாறிலி { திரும்பு m_fileline; }
    token_t token() மாறிலி { திரும்பு m_token; }
    bool is_மதிப்பு() மாறிலி { திரும்பு m_token == ZERO || m_token == ONE; }
    bool மதிப்பு() மாறிலி {
        UASSERT(is_மதிப்பு(), "preproc expr fetch of non-value");
        திரும்பு m_token == ONE;
    }
};

class V3_Pre_கோவை final {
    // MEMBERS
    std::deque<V3_Pre_கோவை_Token> m_inputs;  // Input stack
    std::deque<V3_Pre_கோவை_Token> m_மதிப்புகள்;  // Value stack
    std::deque<V3_Pre_கோவை_Token> m_ops;  // Operator stack
    File_வரி* m_firstFileline = nullptr;

    VL_DEFINE_DEBUG_FUNCTIONS;

    // PARSER DEFINITION
    எண்ணிக்கை action_t : uint8_t {
        VV,  //மதிப்பு
        AA,  // Accept
        RR,  // Reduce
        SS,  // Shift
        EE  // Error
    };
    நிலையான மாறிலி எழுத்து* actionAscii(action_t en) {
        நிலையான constexpr மாறிலி எழுத்து* names[] = {"VV", "AA", "RR", "SS", "EE"};
        திரும்பு names[en];
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

    action_t parseTable[V3_Pre_கோவை_Token::MAX][V3_Pre_கோவை_Token::MAX] = {
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

    நிலையான வெறுமை selfTestImp() {
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

    நிலையான வெறுமை selfTestOne(மாறிலி string& கோவை, bool expect) {
        // This hacky self-test parser just looks at first character of
        // operator, and requires space separation of operators/values
        UINFO(9, "V3PreExpr selfTestOne " << கோவை);
        File_வரி* மாறிலி flp = nullptr;
        V3_Pre_கோவை parser;
        parser.reset(flp);
        bool tstart = true;
        க்கு (மாறிலி எழுத்து* cp = கோவை.c_str(); *cp; ++cp) {
            எனில் (tstart) {
                tstart = false;
                தேர்வு (*cp) {
                நிலை '0': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::ZERO}); நிறுத்து;
                நிலை '1': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::ONE}); நிறுத்து;
                நிலை '!': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::LNOT}); நிறுத்து;
                நிலை '|': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::LOR}); நிறுத்து;
                நிலை '&': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::LAND}); நிறுத்து;
                நிலை '-': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::IMP}); நிறுத்து;
                நிலை '<': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::EQV}); நிறுத்து;
                நிலை '(': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::BRA}); நிறுத்து;
                நிலை ')': parser.தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::KET}); நிறுத்து;
                இயல்பு: நிறுத்து;
                }
            } இல்லையெனில் எனில் (*cp == ' ') {
                tstart = true;
            }
        }
        மாறிலி bool got = parser.result();
        UASSERT_SELFTEST(got, expect);
    }

    // METHODS
    வெறுமை தள்ளு_Op(மாறிலி V3_Pre_கோவை_Token& token) {
        // UINFO(9, "  pushOp " << token.ascii());
        m_ops.தள்ளு_back(token);
    }
    வெறுமை தள்ளு_மதிப்பு(மாறிலி V3_Pre_கோவை_Token& token) {
        // UINFO(9, "  pushValue " << token.ascii());
        m_மதிப்புகள்.தள்ளு_back(token);
    }
    V3_Pre_கோவை_Token எடு_மதிப்பு() {
        எனில் (m_மதிப்புகள்.empty()) {
            m_firstFileline->v3error("Syntax error in `ifdef () expression");
            திரும்பு V3_Pre_கோவை_Token{m_firstFileline, false};
        }
        மாறிலி V3_Pre_கோவை_Token tok = m_மதிப்புகள்.back();
        m_மதிப்புகள்.எடு_back();
        // UINFO(9, "  popValue " << tok.ascii());
        திரும்பு tok;
    }
    வெறுமை reduce() {
        UASSERT(!m_ops.empty(), "lost op stack beginning END");
        மாறிலி V3_Pre_கோவை_Token tok = m_ops.back();
        // UINFO(9, "Reduce " << tok.ascii());
        m_ops.எடு_back();
        தேர்வு (tok.token()) {
        நிலை V3_Pre_கோவை_Token::KET: {
            போது (m_ops.back().token() != V3_Pre_கோவை_Token::END
                   && m_ops.back().token() != V3_Pre_கோவை_Token::BRA)
                reduce();
            எனில் (m_ops.back().token() == V3_Pre_கோவை_Token::BRA) {
                m_ops.எடு_back();
            } இல்லையெனில் {
                tok.fileline()->v3error("Syntax error in `ifdef () expression:"  // LCOV_EXCL_LINE
                                        " ) without matching )");
                திரும்பு;
            }
            நிறுத்து;
        }
        நிலை V3_Pre_கோவை_Token::LNOT: {
            மாறிலி V3_Pre_கோவை_Token lhs = எடு_மதிப்பு();
            தள்ளு_மதிப்பு(V3_Pre_கோவை_Token{tok.fileline(), !lhs.மதிப்பு()});
            நிறுத்து;
        }
        நிலை V3_Pre_கோவை_Token::LAND: {
            மாறிலி V3_Pre_கோவை_Token rhs = எடு_மதிப்பு();
            மாறிலி V3_Pre_கோவை_Token lhs = எடு_மதிப்பு();
            தள்ளு_மதிப்பு(V3_Pre_கோவை_Token{tok.fileline(), lhs.மதிப்பு() && rhs.மதிப்பு()});
            நிறுத்து;
        }
        நிலை V3_Pre_கோவை_Token::LOR: {
            மாறிலி V3_Pre_கோவை_Token rhs = எடு_மதிப்பு();
            மாறிலி V3_Pre_கோவை_Token lhs = எடு_மதிப்பு();
            தள்ளு_மதிப்பு(V3_Pre_கோவை_Token{tok.fileline(), lhs.மதிப்பு() || rhs.மதிப்பு()});
            நிறுத்து;
        }
        நிலை V3_Pre_கோவை_Token::IMP: {
            மாறிலி V3_Pre_கோவை_Token rhs = எடு_மதிப்பு();
            மாறிலி V3_Pre_கோவை_Token lhs = எடு_மதிப்பு();
            தள்ளு_மதிப்பு(V3_Pre_கோவை_Token{tok.fileline(), !lhs.மதிப்பு() || rhs.மதிப்பு()});
            நிறுத்து;
        }
        நிலை V3_Pre_கோவை_Token::EQV: {
            மாறிலி V3_Pre_கோவை_Token rhs = எடு_மதிப்பு();
            மாறிலி V3_Pre_கோவை_Token lhs = எடு_மதிப்பு();
            தள்ளு_மதிப்பு(V3_Pre_கோவை_Token{tok.fileline(), lhs.மதிப்பு() == rhs.மதிப்பு()});
            நிறுத்து;
        }
        இயல்பு: {
            v3fatalSrc("bad case on operand stack");
            நிறுத்து;
        }
        }
    }
    வெறுமை parse() {
        போது (!m_inputs.empty()) {
            மாறிலி V3_Pre_கோவை_Token tok = m_inputs.front();
            m_inputs.எடு_front();
            UINFO(9, "input read " << tok.ascii());
            எனில் (tok.is_மதிப்பு()) {
                தள்ளு_மதிப்பு(tok);
                தொடர்;
            }

            UASSERT(!m_ops.empty(), "lost op stack beginning END");
            மாறிலி V3_Pre_கோவை_Token உச்சி_Tok = m_ops.back();
            auto action = parseTable[உச்சி_Tok.token()][tok.token()];
            UINFO(9, "pop action " << actionAscii(action) << " from parseTable[" << உச்சி_Tok.ascii()
                                   << "][" << tok.ascii() << "]");
            தேர்வு (action) {
            நிலை RR:  // Reduce
                reduce();
                நிறுத்து;
            நிலை SS:  // Shift
                m_ops.தள்ளு_back(tok);
                நிறுத்து;
            நிலை AA:  // Accept
                நிறுத்து;
            இயல்பு: tok.fileline()->v3error("Syntax error in `ifdef () expression"); திரும்பு;
            }
        }
    }

பொது:
    // METHODS
    V3_Pre_கோவை() = இயல்பு;
    ~V3_Pre_கோவை() = இயல்பு;
    வெறுமை reset(File_வரி* flp) {
        m_inputs.clear();
        m_மதிப்புகள்.clear();
        m_ops.clear();
        m_firstFileline = flp;
        தள்ளு_Op(V3_Pre_கோவை_Token{flp, V3_Pre_கோவை_Token::END});
    }
    வெறுமை தள்ளு_உள்ளீடு(மாறிலி V3_Pre_கோவை_Token& token) {
        எனில் (!m_firstFileline) m_firstFileline = token.fileline();
        UINFO(9, "pushInput " << token.ascii());
        m_inputs.தள்ளு_back(token);
    }
    bool result() {
        தள்ளு_உள்ளீடு(V3_Pre_கோவை_Token{m_firstFileline, V3_Pre_கோவை_Token::END});
        parse();
        திரும்பு எடு_மதிப்பு().மதிப்பு();
    }
    நிலையான வெறுமை selfTest() VL_MT_DISABLED { selfTestImp(); }
};

#endif  // Guard
