// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Large 4-state numbers
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Number.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cerrno>
#ਅਨ੍ਤਰ੍ਭਾਵ <cmath>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ਪੂਰਨ MAX_SPRINTF_DOUBLE_ਮਾਪ
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_OP_ARGS1(arg1) \
    UASSERT((ਇਹ != &(arg1)), "Number operation called with same source and dest")
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((ਇਹ != &(arg1) && ਇਹ != &(arg2)), \
            "Number operation called with same source and dest")
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((ਇਹ != &(arg1) && ਇਹ != &(arg2) && ਇਹ != &(arg3)), \
            "Number operation called with same source and dest")
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((ਇਹ != &(arg1) && ਇਹ != &(arg2) && ਇਹ != &(arg3) && ਇਹ != &(arg4)), \
            "Number operation called with same source and dest")

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).ਅੰਕੜੇ_Type() == V3_ਸੰਖਿਆ_ਅੰਕੜੇ::V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    ਅੰਕ_ASSERT_LOGIC_ARGS1(arg1); \
    ਅੰਕ_ASSERT_LOGIC_ARGS1(arg2)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    ਅੰਕ_ASSERT_LOGIC_ARGS1(arg1); \
    ਅੰਕ_ASSERT_LOGIC_ARGS1(arg2); \
    ਅੰਕ_ASSERT_LOGIC_ARGS1(arg3); \
    ਅੰਕ_ASSERT_LOGIC_ARGS1(arg4)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_STRING_ARGS2(arg1, arg2) \
    ਅੰਕ_ASSERT_STRING_ARGS1(arg1); \
    ਅੰਕ_ASSERT_STRING_ARGS1(arg2)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਅੰਕ_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    ਅੰਕ_ASSERT_DOUBLE_ARGS1(arg1); \
    ਅੰਕ_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

ਖਾਲੀ V3_ਸੰਖਿਆ::v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& str) ਅਚਲ VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ਜੇ (m_nodep) {
        m_nodep->v3error_ਸਿਰਾ(nsstr);
    } ਨਹੀਂਤਾਂ ਜੇ (m_fileline) {
        m_fileline->v3error_ਸਿਰਾ(nsstr);
    } ਨਹੀਂਤਾਂ {
        V3Error::v3error_ਸਿਰਾ(nsstr, "", ਖਾਲੀ_ਸੂਚਕ);
    }
}

ਖਾਲੀ V3_ਸੰਖਿਆ::v3error_ਸਿਰਾ_Fatal(ਅਚਲ std::ostringstream& str) ਅਚਲ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_ਸਿਰਾ(str);
    ਪ੍ਰਤਿਜ੍ਞਾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_ਸੰਖਿਆ::V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, VerilogStringLiteral, ਅਚਲ string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    ਜੇ (str.ਖਾਲੀ_ਥਾਵਾਂ()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } ਨਹੀਂਤਾਂ {
        init(nodep, std::max<ਪੂਰਨ>(str.ਲੰਮਾਈ() * 8, 1));
        ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ pos = 0; pos < str.ਲੰਮਾਈ(); ++pos) {
            ਅਚਲ ਪੂਰਨ topos = str.ਲੰਮਾਈ() - 1 - pos;
            ਮੁੱਲ_And_X& v = m_ਅੰਕੜੇ.ਅੰਕ()[topos / 4];
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 8; ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                ਜੇ (str[pos] & (1UL << ਦੋਆਧਾਰੀ_ਅੰਕ_2)) v.m_ਮੁੱਲ |= (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 + (topos % 4) * 8));
            }
        }
    }
    m_ਅੰਕੜੇ.m_fromString = ਸੱਚ;
    opCleanThis(ਸੱਚ);
}

V3_ਸੰਖਿਆ::V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ_DType* nodedtypep) {
    ਜੇ (nodedtypep->isCHandle()) {
        init(nodep);
        ਸੈੱਟ_Null();
    } ਨਹੀਂਤਾਂ ਜੇ (nodedtypep->isDouble()) {
        init(nodep, 64);
        ਸੈੱਟ_Double(0.0);
    } ਨਹੀਂਤਾਂ ਜੇ (nodedtypep->isString()) {
        init(nodep);
        ਸੈੱਟ_String("");
    } ਨਹੀਂਤਾਂ {
        init(nodep, nodedtypep->ਚੌੜਾਈ(), nodedtypep->ਚੌੜਾਈ_Sized());
    }
}

ਖਾਲੀ V3_ਸੰਖਿਆ::create(ਅਚਲ ਅੱਖਰ* sourcep) {
    m_ਅੰਕੜੇ.ਸੈੱਟ_Logic();
    ਅਚਲ ਅੱਖਰ* ਮੁੱਲ_startp = sourcep;
    ਲਈ (ਅਚਲ ਅੱਖਰ* cp = sourcep; *cp; ++cp) {
        ਜੇ (*cp == '\'') {
            ਮੁੱਲ_startp = cp + 1;
            ਰੋਕੋ;
        }
    }

    ਤਾਰਕਿਕ userSized = ਝੂਠ;
    ਤਾਰਕਿਕ unbased = ਝੂਠ;
    ਅੱਖਰ ਅਧਾਰ = '\0';
    ਜੇ (ਮੁੱਲ_startp != sourcep) {  // Has a '
        string widthn;
        ਅਚਲ ਅੱਖਰ* cp = sourcep;
        ਲਈ (; *cp; ++cp) {
            ਜੇ (*cp == '\'') {
                ++cp;
                ਰੋਕੋ;
            }
            ਜੇ (*cp != '_') widthn += *cp;
        }
        ਜਦੋਂ (*cp == '_') cp++;
        ਜੇ (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(ਸੱਚ);
        }
        ਜੇ (*cp) {
            ਅਧਾਰ = *cp;
            ++cp;
        }
        ਮੁੱਲ_startp = cp;

        userSized = widthn.ਲੰਮਾਈ() != 0;
        ਜੇ (std::atoi(widthn.c_str())) {
            ਜੇ (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_ਅੰਕ_ਚੌੜਾਈ()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                ਚੌੜਾਈ(v3Global.opt.max_ਅੰਕ_ਚੌੜਾਈ(), ਸੱਚ);
            } ਨਹੀਂਤਾਂ {
                ਚੌੜਾਈ(std::atoi(widthn.c_str()), ਸੱਚ);
            }
        }
    } ਨਹੀਂਤਾਂ {
        unbased = ਸੱਚ;
        ਅਧਾਰ = 'd';
    }

    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    ਜੇ (ਅਧਾਰ == '0') {
        ਚੌੜਾਈ(1, ਝੂਠ);  // So we extend it
        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 0);
        m_ਅੰਕੜੇ.m_autoExtend = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ == '1') {
        ਚੌੜਾਈ(1, ਝੂਠ);  // So we extend it
        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 1);
        m_ਅੰਕੜੇ.m_autoExtend = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (std::tolower(ਅਧਾਰ) == 'z') {
        ਚੌੜਾਈ(1, ਝੂਠ);  // So we extend it
        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 'z');
        m_ਅੰਕੜੇ.m_autoExtend = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (std::tolower(ਅਧਾਰ) == 'x') {
        ਚੌੜਾਈ(1, ਝੂਠ);  // So we extend it
        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 'x');
        m_ਅੰਕੜੇ.m_autoExtend = ਸੱਚ;
    }
    // Otherwise...
    ਨਹੀਂਤਾਂ ਜੇ (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Bound = std::max<ਪੂਰਨ>(32, std::ਲੜੀ_ਲੰਬਾਈ(ਮੁੱਲ_startp) * 4);
        ਚੌੜਾਈ(ਚੌੜਾਈ_Bound, ਝੂਠ);  // Will change width below
        ਜੇ (unbased) isSigned(ਸੱਚ);  // Also says the spec.
    }

    // Ignore leading blanks
    ਜਦੋਂ (*ਮੁੱਲ_startp == '_' || std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*ਮੁੱਲ_startp)) ++ਮੁੱਲ_startp;
    ਜੇ (!*ਮੁੱਲ_startp && !m_ਅੰਕੜੇ.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    ਜੇ (userSized && m_ਅੰਕੜੇ.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    ਪੂਰਨ obit = 0;  // Start at LSB
    ਪੂਰਨ ਅਧਾਰ_align = 1;
    ਜੇ (std::tolower(ਅਧਾਰ) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        ਜਦੋਂ (*ਮੁੱਲ_startp == '_' || *ਮੁੱਲ_startp == '0') ++ਮੁੱਲ_startp;
        // Convert decimal number to hex
        ਪੂਰਨ olen = 0;
        uint32_t val = 0;
        ਪੂਰਨ got_x = 0;
        ਪੂਰਨ got_z = 0;
        ਪੂਰਨ got_01 = 0;
        ਤਾਰਕਿਕ warnedTooMany = ਝੂਠ;
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = ਮੁੱਲ_startp; *cp; ++cp) {
            ਚੋਣ (std::tolower(*cp)) {
            ਹਾਲਤ '0':  // FALLTHRU
            ਹਾਲਤ '1':  // FALLTHRU
            ਹਾਲਤ '2':  // FALLTHRU
            ਹਾਲਤ '3':  // FALLTHRU
            ਹਾਲਤ '4':  // FALLTHRU
            ਹਾਲਤ '5':  // FALLTHRU
            ਹਾਲਤ '6':  // FALLTHRU
            ਹਾਲਤ '7':  // FALLTHRU
            ਹਾਲਤ '8':  // FALLTHRU
            ਹਾਲਤ '9': {
                ਜੇ (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = val;
                } ਨਹੀਂਤਾਂ {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_ਸੰਖਿਆ ਗੁਣਨਫਲ(ਇਹ, ਚੌੜਾਈ() + 4);  // +4 for overflow detection
                    ਅਚਲ V3_ਸੰਖਿਆ ten(ਇਹ, ਚੌੜਾਈ() + 4, 10);
                    ਅਚਲ V3_ਸੰਖਿਆ addend(ਇਹ, ਚੌੜਾਈ(), (*cp - '0'));
                    ਗੁਣਨਫਲ.opMul(*ਇਹ, ten);
                    op_ਜੋੜੋ(ਗੁਣਨਫਲ, addend);
                    ਜੇ (!warnedTooMany && ਗੁਣਨਫਲ.ਬਿੱਟ_ਮੁੱਲ(ਚੌੜਾਈ(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = ਸੱਚ;
                    }
                }
                ++olen;
                got_01 = 1;
                ਰੋਕੋ;
            }
            ਹਾਲਤ 'z':
            ਹਾਲਤ '?': {
                got_z = 1;
                ਜੇ (!userSized) ਚੌੜਾਈ(32, ਝੂਠ);
                ਸੈੱਟ_All_ਬਿੱਟ_Z();
                ਰੋਕੋ;
            }
            ਹਾਲਤ 'x': {
                got_x = 1;
                ਜੇ (!userSized) ਚੌੜਾਈ(32, ਝੂਠ);
                ਸੈੱਟ_All_ਬਿੱਟ_X();
                ਰੋਕੋ;
            }
            ਹਾਲਤ '_': ਰੋਕੋ;
            ਮੂਲ: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                ਰੋਕੋ;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = ਚੌੜਾਈ();
        ਜੇ ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << ਮੁੱਲ_startp);
        }
    } ਨਹੀਂਤਾਂ {
        // Convert bin/octal number to hex
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = ਮੁੱਲ_startp + std::ਲੜੀ_ਲੰਬਾਈ(ਮੁੱਲ_startp) - 1; cp >= ਮੁੱਲ_startp;
             cp--) {
            ਜੇ (*cp != '_' && *cp != '0' && obit >= ਚੌੜਾਈ()) {
                warnTooMany(sourcep);
                ਰੋਕੋ;
            }
            ਚੋਣ (std::tolower(ਅਧਾਰ)) {
            ਹਾਲਤ 'b': {
                ਅਧਾਰ_align = 1;
                ਚੋਣ (std::tolower(*cp)) {
                ਹਾਲਤ '0': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0); ਰੋਕੋ;
                ਹਾਲਤ '1': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1); ਰੋਕੋ;
                ਹਾਲਤ 'z':
                ਹਾਲਤ '?': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'z'); ਰੋਕੋ;
                ਹਾਲਤ 'x': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'x'); ਰੋਕੋ;
                ਹਾਲਤ '_': ਰੋਕੋ;
                ਮੂਲ: v3error("Illegal character in binary constant: " << *cp);
                }
                ਰੋਕੋ;
            }

            ਹਾਲਤ 'o':
            ਹਾਲਤ 'c': {
                ਅਧਾਰ_align = 3;
                ਚੋਣ (std::tolower(*cp)) {  // clang-format off
                ਹਾਲਤ '0': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਰੋਕੋ;
                ਹਾਲਤ '1': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਰੋਕੋ;
                ਹਾਲਤ '2': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਰੋਕੋ;
                ਹਾਲਤ '3': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਰੋਕੋ;
                ਹਾਲਤ '4': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਰੋਕੋ;
                ਹਾਲਤ '5': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਰੋਕੋ;
                ਹਾਲਤ '6': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਰੋਕੋ;
                ਹਾਲਤ '7': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 1);  ਰੋਕੋ;
                ਹਾਲਤ 'z':
                ਹਾਲਤ '?': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'z'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'z'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'z'); ਰੋਕੋ;
                ਹਾਲਤ 'x': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'x'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'x'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++, 'x'); ਰੋਕੋ;
                ਹਾਲਤ '_': ਰੋਕੋ;
                // clang-format on
                ਮੂਲ: v3error("Illegal character in octal constant");
                }
                ਰੋਕੋ;
            }

            ਹਾਲਤ 'h':  // FALLTHRU
            ਹਾਲਤ 'x': {
                ਅਧਾਰ_align = 4;
                ਚੋਣ (std::tolower(*cp)) {  // clang-format off
                ਹਾਲਤ '0': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '1': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '2': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '3': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '4': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '5': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '6': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '7': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਰੋਕੋ;
                ਹਾਲਤ '8': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ '9': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'a': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'b': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'c': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'd': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'e': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,0); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'f': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,1); ਰੋਕੋ;
                ਹਾਲਤ 'z':
                ਹਾਲਤ '?': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'z'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'z'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'z'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'z'); ਰੋਕੋ;
                ਹਾਲਤ 'x': ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'x'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'x'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'x'); ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit++,'x'); ਰੋਕੋ;
                    // clang-format on
                ਹਾਲਤ '_': ਰੋਕੋ;
                ਮੂਲ:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    ਰੋਕੋ;
                }  // LCOV_EXCL_STOP
                ਰੋਕੋ;
            }
            ਮੂਲ:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << ਅਧਾਰ);
                ਰੋਕੋ;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    ਜੇ (!userSized && !m_ਅੰਕੜੇ.m_autoExtend) {
        ਚੌੜਾਈ(std::max(32, ਅਧਾਰ_align * ((ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit() + ਅਧਾਰ_align - 1) / ਅਧਾਰ_align)), ਝੂਠ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    ਜਦੋਂ (obit <= ਚੌੜਾਈ() && obit && ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(obit - 1)) {
        ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(obit, ਦੋਆਧਾਰੀ_ਅੰਕ_Is(obit - 1));
        ++obit;
    }
    opCleanThis(ਸੱਚ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

ਖਾਲੀ V3_ਸੰਖਿਆ::warnTooMany(ਅਚਲ string& ਮੁੱਲ) {
    v3warn(WIDTHDIGITS, "Too many digits for " << ਚੌੜਾਈ() << " bit number: '" << ਮੁੱਲ << "'");
}

ਖਾਲੀ V3_ਸੰਖਿਆ::nodep(Ast_ਗੰਢ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    ਜੇ (!nodep) ਵਾਪਸ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::epsilonEqual(ਦੁੱਗਣਾ a, ਦੁੱਗਣਾ b) {
    ਵਾਪਸ std::fabs(a - b)
           <= (std::numeric_limits<ਦੁੱਗਣਾ>::epsilon() * std::max(1.0, std::max(a, b)));
}

ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::epsilonIntegral(ਦੁੱਗਣਾ a) {
    ਅਚਲ ਦੁੱਗਣਾ dint = static_cast<ਦੁੱਗਣਾ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ਵਾਪਸ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

ਪੂਰਨ V3_ਸੰਖਿਆ::log2b(uint32_t ਅੰਕ) {
    // See also opCLog2
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 31; ਦੋਆਧਾਰੀ_ਅੰਕ_2 > 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
        ਜੇ (ਅੰਕ & (1ULL << ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_2;
    }
    ਵਾਪਸ 0;
}

ਪੂਰਨ V3_ਸੰਖਿਆ::log2bQuad(uint64_t ਅੰਕ) {
    ਜੇ (ਅੰਕ >> 32ULL) ਵਾਪਸ 32 + log2b(ਅੰਕ >> 32ULL);
    ਵਾਪਸ log2b(ਅੰਕ);
}

//======================================================================
// Setters

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Zero() {
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, 0};
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Quad(uint64_t ਮੁੱਲ) {
    ਸੈੱਟ_Zero();
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = ਮੁੱਲ & 0xffffffffULL;
    ਜੇ (ਚੌੜਾਈ() > 32) m_ਅੰਕੜੇ.ਅੰਕ()[1].m_ਮੁੱਲ = (ਮੁੱਲ >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Long(uint32_t ਮੁੱਲ) {
    ਸੈੱਟ_Zero();
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = ਮੁੱਲ;
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Long_S(int32_t ਮੁੱਲ) {
    ਸੈੱਟ_Zero();
    ਸੰਘ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = ਮੁੱਲ;
    (ਖਾਲੀ)u.s;
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = u.u;
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Double(ਦੁੱਗਣਾ ਮੁੱਲ) {
    UASSERT(ਚੌੜਾਈ() == 64, "Real operation on wrong sized number");
    m_ਅੰਕੜੇ.ਸੈੱਟ_Double();
    ਸੰਘ {
        ਦੁੱਗਣਾ d;
        uint32_t u[2];
    } u;
    u.d = ਮੁੱਲ;
    (ਖਾਲੀ)u.d;
    ਲਈ (ਪੂਰਨ i = 2; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, 0};
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = u.u[0];
    m_ਅੰਕੜੇ.ਅੰਕ()[1].m_ਮੁੱਲ = u.u[1];
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Single_ਬਿੱਟ(ਅੱਖਰ ਮੁੱਲ) {
    ਲਈ (ਪੂਰਨ i = 1 /*ਉੱਤਲਾ*/; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, 0};
    m_ਅੰਕੜੇ.ਅੰਕ()[0] = {(ਮੁੱਲ == '1' || ਮੁੱਲ == 'x' || ਮੁੱਲ == 1 || ਮੁੱਲ == 3),
                       (ਮੁੱਲ == 'z' || ਮੁੱਲ == 'x' || ਮੁੱਲ == 2 || ਮੁੱਲ == 3)};
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_All_Bits0() {
    ਸੈੱਟ_Zero();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_All_Bits1() {
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {~0U, 0};
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_All_ਬਿੱਟ_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {~0U, ~0U};
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_All_ਬਿੱਟ_Z() {
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, ~0U};
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_All_ਬਿੱਟ_XRemoved() {
    ਜੇ (!v3Global.constRemoveXs()) {
        ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    } ਨਹੀਂਤਾਂ {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        ਜੇ (v3Global.opt.x_ਸੌਂਪੋ() == "1") {
            ਵਾਪਸ ਸੈੱਟ_All_Bits1();
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ ਸੈੱਟ_All_Bits0();
        }
    }
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_Value1() {
    m_ਅੰਕੜੇ.ਅੰਕ()[0] = {1, 0};
    ਲਈ (ਪੂਰਨ i = 1; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, 0};
    ਵਾਪਸ *ਇਹ;
}

ਖਾਲੀ V3_ਸੰਖਿਆ::ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_X0(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::ਸੈੱਟ_ਨਕਾਬ(ਪੂਰਨ nbits, ਪੂਰਨ lsb) {
    ਸੈੱਟ_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_ਬਿੱਟ(lsb, std::max(0, std::min(nbits, ਚੌੜਾਈ() - lsb)), 1);
    ਵਾਪਸ *ਇਹ;
}

//======================================================================
// ACCESSORS - as strings

string V3_ਸੰਖਿਆ::ascii(ਤਾਰਕਿਕ prefixed, ਤਾਰਕਿਕ cleanVerilog) ਅਚਲ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ਜੇ (is1_ਕਦਮ()) ਵਾਪਸ "1step";
    ਜੇ (isNull()) ਵਾਪਸ "null";
    std::ostringstream ਬਾਹਰਲਾ;
    ਜੇ (isDouble()) {
        ਬਾਹਰਲਾ.precision(17);
        ਜੇ (VL_UNCOVERABLE(ਚੌੜਾਈ() != 64)) {
            ਬਾਹਰਲਾ << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } ਨਹੀਂਤਾਂ {
            ਬਾਹਰਲਾ << ਮੰਜ਼ਿਲ_Double();
            ਜੇ (ਮੰਜ਼ਿਲ_Double() == floor(ਮੰਜ਼ਿਲ_Double())) ਬਾਹਰਲਾ << ".0";
        }
        ਵਾਪਸ ਬਾਹਰਲਾ.str();
    } ਨਹੀਂਤਾਂ ਜੇ (isString()) {
        ਵਾਪਸ '"' + ਮੰਜ਼ਿਲ_String() + '"';
    } ਨਹੀਂਤਾਂ {
        ਜੇ (VL_UNCOVERABLE((m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ_2() - 1].m_ਮੁੱਲ | m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ_2() - 1].m_ਮੁੱਲ_X)
                           & ~hi_ਸ਼ਬਦ_ਨਕਾਬ())) {
            ਬਾਹਰਲਾ << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    ਜੇ (prefixed) {
        ਜੇ (sized()) {
            ਬਾਹਰਲਾ << ਚੌੜਾਈ() << "'";
        } ਨਹੀਂਤਾਂ ਜੇ (autoExtend() && ਚੌੜਾਈ() == 1) {
            ਬਾਹਰਲਾ << "'";
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(0)) {
                ਬਾਹਰਲਾ << '0';
            } ਨਹੀਂਤਾਂ ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(0)) {
                ਬਾਹਰਲਾ << '1';
            } ਨਹੀਂਤਾਂ ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(0)) {
                ਬਾਹਰਲਾ << 'z';
            } ਨਹੀਂਤਾਂ {
                ਬਾਹਰਲਾ << 'x';
            }
            ਵਾਪਸ ਬਾਹਰਲਾ.str();
        } ਨਹੀਂਤਾਂ {
            ਜੇ (cleanVerilog) {
                ਬਾਹਰਲਾ << "'";
            } ਨਹੀਂਤਾਂ {
                ਬਾਹਰਲਾ << "?" << ਚੌੜਾਈ() << "?";
            }
        }
        ਜੇ (isSigned()) ਬਾਹਰਲਾ << "s";
    }

    ਅਚਲ ਤਾਰਕਿਕ ਦੁਆਧਾਰੀ = (isFourState()
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ V3_ਸੰਖਿਆ_ASCII_ਦੁਆਧਾਰੀ
                         // cppcheck-suppress konwnConditionTrueFalse
                         || ਸੱਚ
#ਜੇ_ਅੰਤ
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    ਜੇ (ਦੁਆਧਾਰੀ) {
        ਬਾਹਰਲਾ << "b";
        ਬਾਹਰਲਾ << displayed("%0b");
    } ਨਹੀਂਤਾਂ {
        ਜੇ (prefixed) ਬਾਹਰਲਾ << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        ਬਾਹਰਲਾ << displayed("%0h");
    }
    ਜੇ (isNull()) {
        ਜੇ (VL_UNCOVERABLE(!isEqZero())) {
            ਬਾਹਰਲਾ << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } ਨਹੀਂਤਾਂ {
            ਬਾਹਰਲਾ << " [null]";
        }
    }
    ਵਾਪਸ ਬਾਹਰਲਾ.str();
}

ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::displayedFmtHasArg(ਅੱਖਰ format, ਤਾਰਕਿਕ is_ਜਾਂਚ) {
    // Is this a format letter that takes an argument in runtime?
    (ਖਾਲੀ)is_ਜਾਂਚ;
    ਚੋਣ (std::tolower(format)) {
    ਹਾਲਤ 'b': ਵਾਪਸ ਸੱਚ;  //ਦੁਆਧਾਰੀ
    ਹਾਲਤ 'c': ਵਾਪਸ ਸੱਚ;  // Character
    ਹਾਲਤ 'd': ਵਾਪਸ ਸੱਚ;  // Decimal
    ਹਾਲਤ 'e': ਵਾਪਸ ਸੱਚ;  // Floating
    ਹਾਲਤ 'f': ਵਾਪਸ ਸੱਚ;  // Floating
    ਹਾਲਤ 'g': ਵਾਪਸ ਸੱਚ;  // Floating
    ਹਾਲਤ 'h': ਵਾਪਸ ਸੱਚ;  // Hex
    ਹਾਲਤ 'o': ਵਾਪਸ ਸੱਚ;  // Octal
    ਹਾਲਤ 'p': ਵਾਪਸ ਸੱਚ;  //ਨਮੂਨਾ
    ਹਾਲਤ 's': ਵਾਪਸ ਸੱਚ;  // String
    ਹਾਲਤ 't': ਵਾਪਸ ਸੱਚ;  //ਸਮਾਂ
    ਹਾਲਤ 'u': ਵਾਪਸ ਸੱਚ;  // Packed 2-state
    ਹਾਲਤ 'v': ਵਾਪਸ ਸੱਚ;  // Strength
    ਹਾਲਤ 'x': ਵਾਪਸ ਸੱਚ;  // Hex
    ਹਾਲਤ 'z': ਵਾਪਸ ਸੱਚ;  // Packed 4-state
    ਹਾਲਤ '?': ਵਾਪਸ ਸੱਚ;  // Internal: Compute format in V3Width based on data type
    ਮੂਲ: ਵਾਪਸ ਝੂਠ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_ਸੰਖਿਆ::displayPad(ਮਾਪ_t fmtsize, ਅੱਖਰ pad, ਤਾਰਕਿਕ ਖੱਬੇ, ਅਚਲ string& ਵਿੱਚ_2) VL_ਸ਼ੁੱਧ {
    string padding;
    ਜੇ (ਵਿੱਚ_2.ਲੰਮਾਈ() < fmtsize) padding = string(fmtsize - ਵਿੱਚ_2.ਲੰਮਾਈ(), pad);
    ਵਾਪਸ ਖੱਬੇ ? (ਵਿੱਚ_2 + padding) : (padding + ਵਿੱਚ_2);
}

string V3_ਸੰਖਿਆ::displayedEnumName(ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp) {
    ਅਚਲ string name = VIdProtect::protect(itemp->prettyName());
    ਵਾਪਸ VString::isIdentifier(name) && !std::ਅੰਕ_ਹੈ(static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_ਸੰਖਿਆ::displayedEnumName(ਅਚਲ AstEnumDType* dtypep) ਅਚਲ VL_MT_STABLE {
    V3_ਸੰਖਿਆ ਮੁੱਲ{ਇਹ, dtypep->ਚੌੜਾਈ()};
    ਮੁੱਲ.op_ਸੌਂਪੋ(*ਇਹ);
    ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(itemp->valuep(), Const);
        ਜੇ (!constp->ਅੰਕ().isAnyXZ() && ਮੁੱਲ.isCaseEq(constp->ਅੰਕ()))
            ਵਾਪਸ displayedEnumName(itemp);
    }
    ਵਾਪਸ "";
}

string V3_ਸੰਖਿਆ::displayedEnum(ਅਚਲ AstSFormatArg* argp,
                               ਅਚਲ string& vformat) ਅਚਲ VL_MT_STABLE {
    ਅਚਲ AstEnumDType* ਅਚਲ dtypep = VN_AS(argp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
    V3_ਸੰਖਿਆ ਮੁੱਲ{ਇਹ, dtypep->ਚੌੜਾਈ()};
    ਮੁੱਲ.op_ਸੌਂਪੋ(*ਇਹ);
    ਅਚਲ VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ਅਚਲ ਅੱਖਰ fmt = std::tolower(vformat.ਪਿੱਠ());
    ਜੇ (fmt != 'p' && fmt != 's') ਵਾਪਸ ਮੁੱਲ.displayed(argp, vformat, numericAttr);
    string name = ਮੁੱਲ.displayedEnumName(dtypep);
    ਜੇ (name.ਖਾਲੀ_ਥਾਵਾਂ()) name = ਮੁੱਲ.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_ਸੰਖਿਆ name_ਅੰਕ{ਇਹ};
    name_ਅੰਕ.ਸੈੱਟ_String(name);
    ਵਾਪਸ name_ਅੰਕ.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_ਸੰਖਿਆ::displayed(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& vformat,
                           ਅਚਲ VFormatAttr& formatAttr) ਅਚਲ VL_MT_STABLE {
    ਵਾਪਸ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_ਸੰਖਿਆ::displayed(File_ਸਤਰ* fl, ਅਚਲ string& vformat,
                           ਅਚਲ VFormatAttr& formatAttr) ਅਚਲ VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    ਸਵੈਚਾਲਿਤ pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *ਇਹ);
    ++pos;
    ਤਾਰਕਿਕ ਖੱਬੇ = ਝੂਠ;
    ਜੇ (pos[0] == '-') {
        ਖੱਬੇ = ਸੱਚ;
        ++pos;
    }
    string fmtsize;
    ਲਈ (; pos != vformat.cend() && (std::ਅੰਕ_ਹੈ(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    ਅੱਖਰ fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    ਜੇ (formatAttr.isComplex()) {
        ਜੇ (fmt != 'p') fmt = 's';  // override
    } ਨਹੀਂਤਾਂ ਜੇ (formatAttr.isString()) {
        ਜੇ (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_ਸੰਖਿਆ strwide{fl, static_cast<ਪੂਰਨ>(ਮੰਜ਼ਿਲ_String().ਲੰਮਾਈ()) * 8, 0};
            strwide.opNToI(*ਇਹ);
            ਵਾਪਸ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        ਜੇ (fmt != 'p') fmt = 's';  // override
    } ਨਹੀਂਤਾਂ ਜੇ (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        ਜੇ (fmt == 'p') {
            ਜੇ (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } ਨਹੀਂਤਾਂ {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } ਨਹੀਂਤਾਂ ਜੇ (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        ਜੇ (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_ਸੰਖਿਆ nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*ਇਹ);
            ਵਾਪਸ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    ਚੋਣ (fmt) {
    ਹਾਲਤ 'c': {
        ਜੇ (ਚੌੜਾਈ() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਪੂਰਨ v = ਬਿੱਟ_ਮੁੱਲ(0, 8);
        str.ਧੱਕੋ_ਪਿੱਠ(static_cast<ਅੱਖਰ>(v));
        ਵਾਪਸ str;
    }
    ਹਾਲਤ 'e':  // FALLTHRU
    ਹਾਲਤ 'f':  // FALLTHRU
    ਹਾਲਤ 'g': {
        ਅਚਲ ਦੁੱਗਣਾ n = formatAttr.isDouble()   ? ਮੰਜ਼ਿਲ_Double()
                         : formatAttr.isSigned() ? ਮੰਜ਼ਿਲ_SQuad()
                                                 : ਮੰਜ਼ਿਲ_UQuad();
        ਅੱਖਰ tmp[MAX_SPRINTF_DOUBLE_ਮਾਪ];
        (ਖਾਲੀ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_ਮਾਪ, vformat.c_str(), n);
        ਵਾਪਸ tmp;
    }
    ਹਾਲਤ 's': {
        ਜੇ (formatAttr.isString() || formatAttr.isComplex()) {
            str = ਮੰਜ਼ਿਲ_String();
        } ਨਹੀਂਤਾਂ {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ() - 1;
            ਤਾਰਕਿਕ ਸ਼ੁਰੂ = ਸੱਚ;
            ਜਦੋਂ ((ਦੋਆਧਾਰੀ_ਅੰਕ_2 % 8) != 7) ++ਦੋਆਧਾਰੀ_ਅੰਕ_2;
            ਲਈ (; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 -= 8) {
                ਅਚਲ ਪੂਰਨ v = ਬਿੱਟ_ਮੁੱਲ(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 7, 8);
                ਜੇ (!ਸ਼ੁਰੂ || v) {
                    str += static_cast<ਅੱਖਰ>((v == 0) ? ' ' : v);
                    ਸ਼ੁਰੂ = ਝੂਠ;  // Drop leading 0s
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (fmtsize != "0") str += ' ';
                }
            }
        }
        ਅਚਲ ਮਾਪ_t fmtsizen = static_cast<ਮਾਪ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ਖੱਬੇ, str);
        ਵਾਪਸ str;
    }
    ਹਾਲਤ 'p': {  //ਨਮੂਨਾ
        // 'p' with NUMBER was earlier converted to 'd'
        ਜੇ (formatAttr.isDouble()) {
            ਅਚਲ ਦੁੱਗਣਾ n = formatAttr.isDouble()   ? ਮੰਜ਼ਿਲ_Double()
                             : formatAttr.isSigned() ? ਮੰਜ਼ਿਲ_SQuad()
                                                     : ਮੰਜ਼ਿਲ_UQuad();
            ਅੱਖਰ tmp[MAX_SPRINTF_DOUBLE_ਮਾਪ];
            (ਖਾਲੀ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_ਮਾਪ, "%g", n);
            ਵਾਪਸ tmp;
        }
        ਜੇ (formatAttr.isString())
            ਵਾਪਸ '"' + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(ਮੰਜ਼ਿਲ_String()) + '"';
        ਜੇ (formatAttr.isComplex()) ਵਾਪਸ ਮੰਜ਼ਿਲ_String();
        ਵਾਪਸ "%p";
    }
    ਹਾਲਤ 'b':  // FALLTHRU
    ਹਾਲਤ 'o':  // FALLTHRU
    ਹਾਲਤ 'h':  // FALLTHRU
    ਹਾਲਤ 'x': {
        ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ() - 1;
        ਜੇ (ਖੱਬੇ || !fmtsize.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਜਦੋਂ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 && ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) --ਦੋਆਧਾਰੀ_ਅੰਕ_2;
        }
        ਚੋਣ (fmt) {
        ਹਾਲਤ 'b': {
            ਲਈ (; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; --ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                    str += '0';
                } ਨਹੀਂਤਾਂ ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                    str += '1';
                } ਨਹੀਂਤਾਂ ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                    str += 'z';
                } ਨਹੀਂਤਾਂ {
                    str += 'x';
                }
            }
            ਰੋਕੋ;
        }
        ਹਾਲਤ 'o': {
            ਜਦੋਂ ((ਦੋਆਧਾਰੀ_ਅੰਕ_2 % 3) != 2) ++ਦੋਆਧਾਰੀ_ਅੰਕ_2;
            ਲਈ (; ਦੋਆਧਾਰੀ_ਅੰਕ_2 > 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 -= 3) {
                ਅਚਲ ਪੂਰਨ ਅੰਕ_X = ਗਿਣਤੀ_X(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 2, 3);
                ਅਚਲ ਪੂਰਨ ਅੰਕ_Z = ਗਿਣਤੀ_Z(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 2, 3);
                ਜੇ (ਅੰਕ_X == 3 || ਅੰਕ_X == ਚੌੜਾਈ() - (ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 2)) {
                    str += 'x';
                    ਜਾਰੀ;
                }
                ਜੇ (ਅੰਕ_Z == 3 || ਅੰਕ_Z == ਚੌੜਾਈ() - (ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 2)) {
                    str += 'z';
                    ਜਾਰੀ;
                }
                ਜੇ (ਅੰਕ_X > 0) {
                    str += 'X';
                    ਜਾਰੀ;
                }
                ਜੇ (ਅੰਕ_Z > 0) {
                    str += 'Z';
                    ਜਾਰੀ;
                }
                ਅਚਲ ਪੂਰਨ v = ਬਿੱਟ_ਮੁੱਲ(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 2, 3);
                str += static_cast<ਅੱਖਰ>('0' + v);
            }
            ਰੋਕੋ;
        }
        ਮੂਲ: {  // h/x
            ਜਦੋਂ ((ਦੋਆਧਾਰੀ_ਅੰਕ_2 % 4) != 3) ++ਦੋਆਧਾਰੀ_ਅੰਕ_2;
            ਲਈ (; ਦੋਆਧਾਰੀ_ਅੰਕ_2 > 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 -= 4) {
                ਅਚਲ ਪੂਰਨ ਅੰਕ_X = ਗਿਣਤੀ_X(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 3, 4);
                ਅਚਲ ਪੂਰਨ ਅੰਕ_Z = ਗਿਣਤੀ_Z(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 3, 4);
                ਜੇ (ਅੰਕ_X == 4 || ਅੰਕ_X == ਚੌੜਾਈ() - (ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 3)) {
                    str += 'x';
                    ਜਾਰੀ;
                }
                ਜੇ (ਅੰਕ_Z == 4 || ਅੰਕ_Z == ਚੌੜਾਈ() - (ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 3)) {
                    str += 'z';
                    ਜਾਰੀ;
                }
                ਜੇ (ਅੰਕ_X > 0) {
                    str += 'X';
                    ਜਾਰੀ;
                }
                ਜੇ (ਅੰਕ_Z > 0) {
                    str += 'Z';
                    ਜਾਰੀ;
                }
                ਅਚਲ ਪੂਰਨ v = ਬਿੱਟ_ਮੁੱਲ(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 3, 4);
                ਜੇ (v >= 10) {
                    str += static_cast<ਅੱਖਰ>('a' + v - 10);
                } ਨਹੀਂਤਾਂ {
                    str += static_cast<ਅੱਖਰ>('0' + v);
                }
            }
            ਰੋਕੋ;
        }
        }  // switch
        ਅਚਲ ਮਾਪ_t fmtsizen = static_cast<ਮਾਪ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ਖੱਬੇ ? ' ' : '0'), ਖੱਬੇ, str);
        ਵਾਪਸ str;
    }  // case b/d/x/o
    ਹਾਲਤ 't':  //ਸਮਾਂ
    ਹਾਲਤ 'd': {  // Unsigned decimal
        ਅਚਲ ਤਾਰਕਿਕ issigned = formatAttr.isSigned();
        ਜੇ (fmtsize == "" && !ਖੱਬੇ) {
            ਅਚਲ ਦੁੱਗਣਾ mantissabits = ਚੌੜਾਈ() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            ਦੁੱਗਣਾ dchars = mantissabits / 3.321928094887362 + 1.0;
            ਜੇ (issigned) ++dchars;  // space for sign
            fmtsize = cvt_ਮੰਜ਼ਿਲ_Str(static_cast<ਪੂਰਨ>(dchars));
        }
        ਤਾਰਕਿਕ hasXZ = ਝੂਠ;
        ਜੇ (isAllX()) {
            str = "x";
            hasXZ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (isAllZ()) {
            str = "z";
            hasXZ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (isAnyX()) {
            str = "X";
            hasXZ = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (isAnyZ()) {
            str = "Z";
            hasXZ = ਸੱਚ;
        }
        ਜੇ (!hasXZ) {
            ਜੇ (issigned) {
                ਜੇ (ਚੌੜਾਈ() > 64) {
                    str = ਮੰਜ਼ਿਲ_Decimal_S();
                } ਨਹੀਂਤਾਂ {
                    str = cvt_ਮੰਜ਼ਿਲ_Str(ਮੰਜ਼ਿਲ_SQuad());
                }
            } ਨਹੀਂਤਾਂ {
                ਜੇ (ਚੌੜਾਈ() > 64) {
                    str = ਮੰਜ਼ਿਲ_Decimal_U();
                } ਨਹੀਂਤਾਂ {
                    str = cvt_ਮੰਜ਼ਿਲ_Str(ਮੰਜ਼ਿਲ_UQuad());
                }
            }
        }
        ਅਚਲ ਤਾਰਕਿਕ zeropad = fmtsize.ਲੰਮਾਈ() > 0 && fmtsize[0] == '0' && !ਖੱਬੇ;
        // fmtsize might have changed since we parsed the %fmtsize
        ਅਚਲ ਮਾਪ_t fmtsizen = static_cast<ਮਾਪ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ਖੱਬੇ, str);
        ਵਾਪਸ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    ਹਾਲਤ 'u': {  // Packed 2-state
        ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
            ਅਚਲ uint32_t v = m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ;
            str += static_cast<ਅੱਖਰ>((v >> 0) & 0xff);
            str += static_cast<ਅੱਖਰ>((v >> 8) & 0xff);
            str += static_cast<ਅੱਖਰ>((v >> 16) & 0xff);
            str += static_cast<ਅੱਖਰ>((v >> 24) & 0xff);
        }
        ਵਾਪਸ str;
    }
    ਹਾਲਤ 'z': {  // Packed 4-state
        ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
            ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ >> 0) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ >> 8) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ >> 16) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ >> 24) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ_X >> 0) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ_X >> 8) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ_X >> 16) & 0xff);
            str += static_cast<ਅੱਖਰ>((v.m_ਮੁੱਲ_X >> 24) & 0xff);
        }
        ਵਾਪਸ str;
    }
    ਹਾਲਤ 'v': {  // Strength
        ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ() - 1;
        ਲਈ (; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            ਨਹੀਂਤਾਂ ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                str += "St1 ";
            } ਨਹੀਂਤਾਂ ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                str += "StZ ";
            } ਨਹੀਂਤਾਂ {
                str += "StX";
            }
        }
        ਵਾਪਸ str;
    }
    ਮੂਲ: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_ਸੰਖਿਆ::emitC() ਅਚਲ VL_MT_STABLE {
    constexpr ਮਾਪ_t bufsize = 1000;
    ਅੱਖਰ sbuf[bufsize];
    string ਨਤੀਜਾ;
    ਜੇ (isNull()) {
        ਵਾਪਸ "VlNull{}";
    } ਨਹੀਂਤਾਂ ਜੇ (isDouble()) {
        ਅਚਲ ਦੁੱਗਣਾ dnum = ਮੰਜ਼ਿਲ_Double();
        ਜੇ (std::isinf(dnum)) {
            ਜੇ (std::signbit(dnum)) ਨਤੀਜਾ += '-';
            ਨਤੀਜਾ += "std::numeric_limits<double>::infinity()";
        } ਨਹੀਂਤਾਂ ਜੇ (std::isnan(dnum)) {
            ਜੇ (std::signbit(dnum)) ਨਤੀਜਾ += '-';
            ਨਤੀਜਾ += "std::numeric_limits<double>::quiet_NaN()";
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਅੱਖਰ* ਅਚਲ fmt = (static_cast<ਪੂਰਨ>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (ਖਾਲੀ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ਵਾਪਸ sbuf;
        }
    } ਨਹੀਂਤਾਂ ਜੇ (isString()) {
        ਅਚਲ string strg = ਮੰਜ਼ਿਲ_String();
        ਅਚਲ string quoted
            = V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(strg, V3_ਬਾਹਰਲਾ_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ਵਾਪਸ '"' + quoted + "\"s";
    } ਨਹੀਂਤਾਂ ਜੇ (ਸ਼ਬਦ_2() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        ਨਤੀਜਾ = "::VlWide<" + std::ਮੰਜ਼ਿਲ_string(ਸ਼ਬਦ_2()) + ">{{";
        ਜੇ (ਸ਼ਬਦ_2() > 4) ਨਤੀਜਾ += '\n';
        ਲਈ (ਪੂਰਨ n = 0; n < ਸ਼ਬਦ_2(); ++n) {
            ਜੇ (n) ਨਤੀਜਾ += ((n % 4) ? ", " : ",\n");
            (ਖਾਲੀ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_ਸ਼ਬਦ(n));
            ਨਤੀਜਾ += sbuf;
        }
        ਜੇ (ਸ਼ਬਦ_2() > 4) ਨਤੀਜਾ += '\n';
        ਨਤੀਜਾ += "}}";
    } ਨਹੀਂਤਾਂ ਜੇ (ਸ਼ਬਦ_2() == 2) {  // Quad
        ਅਚਲ uint64_t qnum = static_cast<uint64_t>(ਮੰਜ਼ਿਲ_UQuad());
        ਅਚਲ ਅੱਖਰ* ਅਚਲ fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ਖਾਲੀ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ਵਾਪਸ sbuf;
    } ਨਹੀਂਤਾਂ {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ਅਚਲ uint32_t unum = ਮੰਜ਼ਿਲ_UInt();
        ਅਚਲ ਅੱਖਰ* ਅਚਲ fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (ਚੌੜਾਈ() > 16) ? ("0x%08" PRIx32 "U")
                                : (ਚੌੜਾਈ() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ਖਾਲੀ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ਵਾਪਸ sbuf;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_Decimal_S() ਅਚਲ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ਜੇ (isNegative()) {
        V3_ਸੰਖਿਆ lhsNoSign = *ਇਹ;
        lhsNoSign.opNegate(*ਇਹ);
        ਵਾਪਸ "-"s + lhsNoSign.ਮੰਜ਼ਿਲ_Decimal_U();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ ਮੰਜ਼ਿਲ_Decimal_U();
    }
}

string V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_Decimal_U() ਅਚਲ VL_MT_STABLE {
    ਅਚਲ ਪੂਰਨ maxdecwidth = (ਚੌੜਾਈ() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_ਸੰਖਿਆ bcd(ਇਹ, maxdecwidth + 4);
    V3_ਸੰਖਿਆ tmp(ਇਹ, maxdecwidth + 4);
    V3_ਸੰਖਿਆ tmp2(ਇਹ, maxdecwidth + 4);

    ਪੂਰਨ from_ਦੋਆਧਾਰੀ_ਅੰਕ = ਚੌੜਾਈ() - 1;
    // Skip all leading zeros
    ਲਈ (; from_ਦੋਆਧਾਰੀ_ਅੰਕ >= 0 && ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(from_ਦੋਆਧਾਰੀ_ਅੰਕ); --from_ਦੋਆਧਾਰੀ_ਅੰਕ) {}
    // Double-dabble algorithm
    ਲਈ (; from_ਦੋਆਧਾਰੀ_ਅੰਕ >= 0; --from_ਦੋਆਧਾਰੀ_ਅੰਕ) {
        // Any digits >= 5 need an add 3 (via tmp)
        ਲਈ (ਪੂਰਨ nibble_ਦੋਆਧਾਰੀ_ਅੰਕ = 0; nibble_ਦੋਆਧਾਰੀ_ਅੰਕ < maxdecwidth; nibble_ਦੋਆਧਾਰੀ_ਅੰਕ += 4) {
            ਜੇ (bcd.ਬਿੱਟ_ਮੁੱਲ(nibble_ਦੋਆਧਾਰੀ_ਅੰਕ, 4) >= 5) {
                tmp2.ਸੈੱਟ_All_Bits0();
                tmp2.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(nibble_ਦੋਆਧਾਰੀ_ਅੰਕ, 1);  // Add 3, decomposed as two bits
                tmp2.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(nibble_ਦੋਆਧਾਰੀ_ਅੰਕ + 1, 1);
                tmp.op_ਸੌਂਪੋ(bcd);
                bcd.op_ਜੋੜੋ(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_ਸੌਂਪੋ(bcd);
        bcd.op_ਖਿਸਕਾਓ_L(tmp, V3_ਸੰਖਿਆ(ਇਹ, 32, 1));
        // bcd[0] = this[from_bit]
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(from_ਦੋਆਧਾਰੀ_ਅੰਕ)) bcd.ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(0, 1);
    }

    string output;
    ਪੂਰਨ lsb = (maxdecwidth - 1) & ~3;
    ਲਈ (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        ਜੇ (bcd.ਬਿੱਟ_ਮੁੱਲ(lsb, 4)) ਰੋਕੋ;
    }
    ਲਈ (; lsb >= 0; lsb -= 4) {
        output += static_cast<ਅੱਖਰ>('0' + bcd.ਬਿੱਟ_ਮੁੱਲ(lsb, 4));  // 0..9
    }
    ਵਾਪਸ output;
}

string V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_Real_String() ਅਚਲ VL_MT_STABLE {
    ਅਚਲ ਦੁੱਗਣਾ ਮੁੱਲ = ਮੰਜ਼ਿਲ_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream ਬਾਹਰਲਾ;
    ਬਾਹਰਲਾ.precision(15);
    ਬਾਹਰਲਾ << ਮੁੱਲ;
    string ਨਤੀਜਾ = ਬਾਹਰਲਾ.str();
    ਜੇ (std::strtod(ਨਤੀਜਾ.c_str(), ਖਾਲੀ_ਸੂਚਕ) != ਮੁੱਲ) {
        ਬਾਹਰਲਾ.str("");
        ਬਾਹਰਲਾ.precision(17);
        ਬਾਹਰਲਾ << ਮੁੱਲ;
        ਨਤੀਜਾ = ਬਾਹਰਲਾ.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    ਜੇ (ਨਤੀਜਾ.ਲੱਭੋ_ਪਹਿਲਾ_ਦਾ(".en") == string::npos) ਨਤੀਜਾ += ".0";
    ਵਾਪਸ ਨਤੀਜਾ;
}

//======================================================================
// ACCESSORS - as numbers

ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isFourState()) ਵਾਪਸ ਝੂਠ;
    // We allow wide numbers that represent values <= 32 bits
    ਲਈ (ਪੂਰਨ i = 1; i < ਸ਼ਬਦ_2(); ++i) {
        ਜੇ (m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}
uint32_t V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_UInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    UASSERT(!isFourState(), "toUInt with 4-state " << *ਇਹ);
    // We allow wide numbers that represent values <= 32 bits
    ਜੇ (VL_UNCOVERABLE(!ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *ਇਹ);
    }
    ਵਾਪਸ m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ;
}

ਦੁੱਗਣਾ V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_Double() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    UASSERT(isDouble() && ਚੌੜਾਈ() == 64, "Real operation on wrong sized/non-real number");
    ਸੰਘ {
        ਦੁੱਗਣਾ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ;
    u.u[1] = m_ਅੰਕੜੇ.ਅੰਕ()[1].m_ਮੁੱਲ;
    ਵਾਪਸ u.d;
}

int32_t V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_SInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isSigned()) {
        ਅਚਲ uint32_t v = ਮੰਜ਼ਿਲ_UInt();
        ਅਚਲ uint32_t signExtend = (-(v & (1UL << (ਚੌੜਾਈ() - 1))));
        ਅਚਲ uint32_t extended = v | signExtend;
        ਵਾਪਸ static_cast<int32_t>(extended);
    } ਨਹੀਂਤਾਂ {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ਵਾਪਸ static_cast<int32_t>(ਮੰਜ਼ਿਲ_UInt());
    }
}

uint64_t V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_UQuad() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *ਇਹ);
    // We allow wide numbers that represent values <= 64 bits
    ਜੇ (isDouble()) ਵਾਪਸ static_cast<uint64_t>(ਮੰਜ਼ਿਲ_Double());
    ਲਈ (ਪੂਰਨ i = 2; i < ਸ਼ਬਦ_2(); ++i) {
        ਜੇ (m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ) {
            v3error("Value too wide for 64-bits expected in this context " << *ਇਹ);
            ਰੋਕੋ;
        }
    }
    ਜੇ (ਚੌੜਾਈ() <= 32) ਵਾਪਸ (static_cast<uint64_t>(ਮੰਜ਼ਿਲ_UInt()));
    ਵਾਪਸ ((static_cast<uint64_t>(m_ਅੰਕੜੇ.ਅੰਕ()[1].m_ਮੁੱਲ) << 32ULL)
            | (static_cast<uint64_t>(m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ)));
}

int64_t V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_SQuad() ਅਚਲ {
    ਜੇ (isDouble()) ਵਾਪਸ static_cast<int64_t>(ਮੰਜ਼ਿਲ_Double());
    ਅਚਲ uint64_t v = ਮੰਜ਼ਿਲ_UQuad();
    ਅਚਲ uint64_t signExtend = (-(v & (1ULL << (ਚੌੜਾਈ() - 1))));
    ਅਚਲ uint64_t extended = v | signExtend;
    ਵਾਪਸ static_cast<int64_t>(extended);
}

string V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_String() ਅਚਲ {
    UASSERT(!isFourState(), "toString with 4-state " << *ਇਹ);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    ਜੇ (isString()) ਵਾਪਸ m_ਅੰਕੜੇ.str();
    ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ() - 1;
    ਜਦੋਂ ((ਦੋਆਧਾਰੀ_ਅੰਕ_2 % 8) != 7) ++ਦੋਆਧਾਰੀ_ਅੰਕ_2;
    std::string str;
    ਲਈ (; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 -= 8) {
        ਅਚਲ ਪੂਰਨ v = ਬਿੱਟ_ਮੁੱਲ(ਦੋਆਧਾਰੀ_ਅੰਕ_2 - 7, 8);
        ਜੇ (v) str += static_cast<ਅੱਖਰ>(v);
    }
    ਵਾਪਸ str;
}

uint32_t V3_ਸੰਖਿਆ::edata_ਸ਼ਬਦ(ਪੂਰਨ eword) ਅਚਲ {
    UASSERT(!isFourState(), "edataWord with 4-state " << *ਇਹ);
    ਵਾਪਸ m_ਅੰਕੜੇ.ਅੰਕ()[eword].m_ਮੁੱਲ;
}

uint8_t V3_ਸੰਖਿਆ::ਅੰਕੜੇ_Byte(ਪੂਰਨ byte) ਅਚਲ {
    ਵਾਪਸ (edata_ਸ਼ਬਦ(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isAllZ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isDouble() || isString()) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਅਚਲ uint32_t ਨਕਾਬ = i == ਸ਼ਬਦ_2() - 1 ? hi_ਸ਼ਬਦ_ਨਕਾਬ() : ~0U;
        ਜੇ (((~v.m_ਮੁੱਲ & v.m_ਮੁੱਲ_X) & ਨਕਾਬ) != ਨਕਾਬ) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isAllX() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isDouble() || isString()) ਵਾਪਸ ਝੂਠ;
    uint32_t ਨਕਾਬ = hi_ਸ਼ਬਦ_ਨਕਾਬ();
    ਲਈ (ਪੂਰਨ i = ਸ਼ਬਦ_2() - 1; i >= 0; --i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਜੇ ((v.m_ਮੁੱਲ & v.m_ਮੁੱਲ_X) ^ ਨਕਾਬ) ਵਾਪਸ ਝੂਠ;
        ਨਕਾਬ = ~0U;
    }
    ਵਾਪਸ ਸੱਚ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isEqZero() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isString()) ਵਾਪਸ m_ਅੰਕੜੇ.str().ਖਾਲੀ_ਥਾਵਾਂ();
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਜੇ (v.m_ਮੁੱਲ || v.m_ਮੁੱਲ_X) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isNeqZero() ਅਚਲ {
    ਜੇ (isString()) ਵਾਪਸ !m_ਅੰਕੜੇ.str().ਖਾਲੀ_ਥਾਵਾਂ();
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਜੇ (v.m_ਮੁੱਲ & ~v.m_ਮੁੱਲ_X) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::is_ਬਿੱਟ_Zero(ਪੂਰਨ msb, ਪੂਰਨ lsb) ਅਚਲ {
    ਲਈ (ਪੂਰਨ i = lsb; i <= msb; ++i) {
        ਜੇ (VL_UNLIKELY(!ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(i))) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isEqOne() ਅਚਲ {
    ਜੇ (m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ != 1 || m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ_X) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 1; i < ਸ਼ਬਦ_2(); ++i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਜੇ (v.m_ਮੁੱਲ || v.m_ਮੁੱਲ_X) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isEqAllOnes(ਪੂਰਨ optwidth) ਅਚਲ {
    // Correct number of zero bits/width matters
    ਜੇ (!optwidth) optwidth = ਚੌੜਾਈ();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < optwidth; ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (!ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isFourState() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isDouble() || isString()) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਜੇ (m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ_X) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isAnyX() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isDouble() || isString()) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਜੇ (v.m_ਮੁੱਲ & v.m_ਮੁੱਲ_X) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isAnyXZ() ਅਚਲ {
    ਜੇ (isDouble() || isString()) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਜੇ (m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ_X) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isAnyZ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (isDouble() || isString()) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
        ਜੇ (~v.m_ਮੁੱਲ & v.m_ਮੁੱਲ_X) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}
ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isLtXZ(ਅਚਲ V3_ਸੰਖਿਆ& rhs) ਅਚਲ {
    // Include X/Z in comparisons for sort ordering
    ਜੇ (isString()) ਵਾਪਸ ਮੰਜ਼ਿਲ_String() < rhs.ਮੰਜ਼ਿਲ_String();
    ਜੇ (isDouble()) ਵਾਪਸ ਮੰਜ਼ਿਲ_Double() < rhs.ਮੰਜ਼ਿਲ_Double();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਝੂਠ;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਝੂਠ;
}
ਪੂਰਨ V3_ਸੰਖਿਆ::ਗਿਣਤੀ_X(ਪੂਰਨ lsb, ਪੂਰਨ nbits) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਪੂਰਨ ਗਿਣਤੀ_2 = 0;
    ਲਈ (ਪੂਰਨ bitn = 0; bitn < nbits; ++bitn) {
        ਜੇ (lsb + bitn >= ਚੌੜਾਈ()) ਵਾਪਸ ਗਿਣਤੀ_2;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_X(lsb + bitn)) ++ਗਿਣਤੀ_2;
    }
    ਵਾਪਸ ਗਿਣਤੀ_2;
}
ਪੂਰਨ V3_ਸੰਖਿਆ::ਗਿਣਤੀ_Z(ਪੂਰਨ lsb, ਪੂਰਨ nbits) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਪੂਰਨ ਗਿਣਤੀ_2 = 0;
    ਲਈ (ਪੂਰਨ bitn = 0; bitn < nbits; ++bitn) {
        ਜੇ (lsb + bitn >= ਚੌੜਾਈ()) ਵਾਪਸ ਗਿਣਤੀ_2;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(lsb + bitn)) ++ਗਿਣਤੀ_2;
    }
    ਵਾਪਸ ਗਿਣਤੀ_2;
}

ਪੂਰਨ V3_ਸੰਖਿਆ::ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit() ਅਚਲ {
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 > 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
        ਜੇ (!ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_2 + 1;
    }
    ਵਾਪਸ 1;  // one bit even if number is == 0
}

uint32_t V3_ਸੰਖਿਆ::ਗਿਣਤੀ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& ctrl) ਅਚਲ {
    // Correct number of zero bits/width matters
    ਪੂਰਨ n = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਚੋਣ (ctrl.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0)) {
        ਹਾਲਤ '0':
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ++n;
            ਰੋਕੋ;
        ਹਾਲਤ '1':
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ++n;
            ਰੋਕੋ;
        ਹਾਲਤ 'x':
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_X(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ++n;
            ਰੋਕੋ;
        ਹਾਲਤ 'z':
            ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ++n;
            ਰੋਕੋ;
        }
    }
    ਵਾਪਸ n;
}

uint32_t V3_ਸੰਖਿਆ::ਗਿਣਤੀ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& ctrl1, ਅਚਲ V3_ਸੰਖਿਆ& ctrl2,
                             ਅਚਲ V3_ਸੰਖਿਆ& ctrl3) ਅਚਲ {
    // Correct number of zero bits/width matters
    ਪੂਰਨ n = ਗਿਣਤੀ_ਬਿੱਟ(ctrl1);
    ਜੇ (ctrl2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0) != ctrl1.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0)) n += ਗਿਣਤੀ_ਬਿੱਟ(ctrl2);
    ਜੇ ((ctrl3.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0) != ctrl1.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0)) && (ctrl3.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0) != ctrl2.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(0))) {
        n += ਗਿਣਤੀ_ਬਿੱਟ(ctrl3);
    }
    ਵਾਪਸ n;
}

uint32_t V3_ਸੰਖਿਆ::ਗਿਣਤੀ_Ones() ਅਚਲ {
    ਪੂਰਨ n = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ++n;
    }
    ਵਾਪਸ n;
}

uint32_t V3_ਸੰਖਿਆ::most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() ਅਚਲ {
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
        ਜੇ (!ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_2 + 1;
    }
    ਵਾਪਸ 0;
}

uint32_t V3_ਸੰਖਿਆ::ਘੱਟ_ਤੋਂ_ਘੱਟ_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() ਅਚਲ {
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (!ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_2 + 1;
    }
    ਵਾਪਸ 0;
}

//======================================================================

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਬਿੱਟ_Non_XZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2) || lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਬਿੱਟ_One(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਬਿੱਟ_XZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਬਿੱਟ_Z(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
    }
    ਵਾਪਸ *ਇਹ;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRedOr(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // op i, 1 bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਅੱਖਰ outc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < lhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(1);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
        } ਨਹੀਂਤਾਂ {
            outc = 'x';
        }
    }
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRedAnd(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਅੱਖਰ outc = 1;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < lhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(0);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
        } ਨਹੀਂਤਾਂ {
            outc = 'x';
        }
    }
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRedXor(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // op i, 1 bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਅੱਖਰ outc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < lhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਜੇ (outc == 1) {
                outc = 0;
            } ਨਹੀਂਤਾਂ ਜੇ (outc == 0) {
                outc = 1;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
        } ਨਹੀਂਤਾਂ {
            outc = 'x';
        }
    }
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਗਿਣਤੀ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& ਵਿਅੰਜਕ, ਅਚਲ V3_ਸੰਖਿਆ& ctrl1, ਅਚਲ V3_ਸੰਖਿਆ& ctrl2,
                                ਅਚਲ V3_ਸੰਖਿਆ& ctrl3) {
    // Correct number of zero bits/width matters
    ਅੰਕ_ASSERT_OP_ARGS4(ਵਿਅੰਜਕ, ctrl1, ctrl2, ctrl3);
    ਅੰਕ_ASSERT_LOGIC_ARGS4(ਵਿਅੰਜਕ, ctrl1, ctrl2, ctrl3);
    ਸੈੱਟ_Zero();
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = ਵਿਅੰਜਕ.ਗਿਣਤੀ_ਬਿੱਟ(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਗਿਣਤੀ_Ones(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਜੇ (lhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਸੈੱਟ_Zero();
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = lhs.ਗਿਣਤੀ_Ones();
    opCleanThis();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opIsUnknown(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.isAnyXZ());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opOneHot(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਜੇ (lhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਗਿਣਤੀ_Ones() == 1);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opOneHot0(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਜੇ (lhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਗਿਣਤੀ_Ones() <= 1);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opCLog2(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਜੇ (lhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ਅਚਲ ਪੂਰਨ adjust = (lhs.ਗਿਣਤੀ_Ones() == 1) ? 0 : 1;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = lhs.ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_Long(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + adjust);
            ਵਾਪਸ *ਇਹ;
        }
    }
    ਸੈੱਟ_Zero();
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਜੇ (lhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = lhs.ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_Long(ਦੋਆਧਾਰੀ_ਅੰਕ_2 + 1);
            ਵਾਪਸ *ਇਹ;
        }
    }
    ਸੈੱਟ_Zero();
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLogNot(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    ਅੱਖਰ outc = 1;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < lhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 0;
            ਜਾਓ ਆਖਰੀ;
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
        } ਨਹੀਂਤਾਂ {
            outc = 'x';
        }
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNot(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // Correct number of zero bits/width matters
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'x');
        }
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opAnd(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2) || rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {  // 0
        } ਨਹੀਂਤਾਂ {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'x');
        }
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opOr(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) || rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            // 0
        } ਨਹੀਂਤਾਂ {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'x');
        }
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opXor(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 1);
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2) || rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'x');
        }
        // else zero
    }
    ਵਾਪਸ *ਇਹ;
}

ਖਾਲੀ V3_ਸੰਖਿਆ::fill_ਬਿੱਟ(ਪੂਰਨ destLsb, ਪੂਰਨ ਚੌੜਾਈ, ਅੱਖਰ ਮੁੱਲ) {
    UASSERT(destLsb >= 0 && ਚੌੜਾਈ >= 0, "Negative bit range");
    ਜੇ (!ਚੌੜਾਈ) ਵਾਪਸ;
    UASSERT(ਚੌੜਾਈ <= ਇਹ->ਚੌੜਾਈ() - destLsb, "Destination bit range exceeds number width");

    ਅਚਲ uint32_t ਮੁੱਲ_ਸ਼ਬਦ = ਮੁੱਲ == '1' || ਮੁੱਲ == 'x' || ਮੁੱਲ == 1 || ਮੁੱਲ == 3 ? ~0U : 0;
    ਅਚਲ uint32_t x_ਸ਼ਬਦ = ਮੁੱਲ == 'z' || ਮੁੱਲ == 'x' || ਮੁੱਲ == 2 || ਮੁੱਲ == 3 ? ~0U : 0;
    ਅਚਲ ਪੂਰਨ ਪਹਿਲਾ_ਸ਼ਬਦ = VL_BITWORD_E(destLsb);
    ਅਚਲ ਪੂਰਨ ਆਖਰੀ_ਸ਼ਬਦ = VL_BITWORD_E(destLsb + ਚੌੜਾਈ - 1);
    ਲਈ (ਪੂਰਨ ਸ਼ਬਦ = ਪਹਿਲਾ_ਸ਼ਬਦ; ਸ਼ਬਦ <= ਆਖਰੀ_ਸ਼ਬਦ; ++ਸ਼ਬਦ) {
        ਅਚਲ ਪੂਰਨ ਸ਼ਬਦ_Lsb = ਸ਼ਬਦ == ਪਹਿਲਾ_ਸ਼ਬਦ ? VL_BITBIT_E(destLsb) : 0;
        ਅਚਲ ਪੂਰਨ ਸ਼ਬਦ_Msb = ਸ਼ਬਦ == ਆਖਰੀ_ਸ਼ਬਦ ? VL_BITBIT_E(destLsb + ਚੌੜਾਈ - 1) : VL_SIZEBITS_E;
        ਅਚਲ uint32_t ਨਕਾਬ = VL_ਨਕਾਬ_E(ਸ਼ਬਦ_Msb - ਸ਼ਬਦ_Lsb + 1) << ਸ਼ਬਦ_Lsb;
        ਮੁੱਲ_And_X& dest = m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ];
        dest.m_ਮੁੱਲ = (dest.m_ਮੁੱਲ & ~ਨਕਾਬ) | (ਮੁੱਲ_ਸ਼ਬਦ & ਨਕਾਬ);
        dest.m_ਮੁੱਲ_X = (dest.m_ਮੁੱਲ_X & ~ਨਕਾਬ) | (x_ਸ਼ਬਦ & ਨਕਾਬ);
    }
}

ਖਾਲੀ V3_ਸੰਖਿਆ::copy_ਬਿੱਟ(ਪੂਰਨ destLsb, ਅਚਲ V3_ਸੰਖਿਆ& ਸਰੋਤ, ਪੂਰਨ ਸਰੋਤ_Lsb, ਪੂਰਨ ਚੌੜਾਈ) {
    UASSERT(destLsb >= 0 && ਸਰੋਤ_Lsb >= 0 && ਚੌੜਾਈ >= 0, "Negative bit range");
    ਜੇ (!ਚੌੜਾਈ) ਵਾਪਸ;
    UASSERT(ਚੌੜਾਈ <= ਇਹ->ਚੌੜਾਈ() - destLsb, "Destination bit range exceeds number width");
    UASSERT(ਚੌੜਾਈ <= ਸਰੋਤ.ਚੌੜਾਈ() - ਸਰੋਤ_Lsb, "Source bit range exceeds number width");

    ਅਚਲ ਪੂਰਨ ਪਹਿਲਾ_ਸ਼ਬਦ = VL_BITWORD_E(destLsb);
    ਅਚਲ ਪੂਰਨ ਆਖਰੀ_ਸ਼ਬਦ = VL_BITWORD_E(destLsb + ਚੌੜਾਈ - 1);
    ਲਈ (ਪੂਰਨ dest_ਸ਼ਬਦ = ਪਹਿਲਾ_ਸ਼ਬਦ; dest_ਸ਼ਬਦ <= ਆਖਰੀ_ਸ਼ਬਦ; ++dest_ਸ਼ਬਦ) {
        ਅਚਲ ਪੂਰਨ dest_ਦੋਆਧਾਰੀ_ਅੰਕ = std::max(destLsb, dest_ਸ਼ਬਦ * VL_EDATASIZE);
        ਅਚਲ ਪੂਰਨ dest_ਵਿੱਥ = VL_BITBIT_E(dest_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਅਚਲ ਪੂਰਨ ਸਰੋਤ_ਦੋਆਧਾਰੀ_ਅੰਕ = ਸਰੋਤ_Lsb + dest_ਦੋਆਧਾਰੀ_ਅੰਕ - destLsb;
        ਅਚਲ ਪੂਰਨ ਸਰੋਤ_ਸ਼ਬਦ = VL_BITWORD_E(ਸਰੋਤ_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਅਚਲ ਪੂਰਨ ਸਰੋਤ_ਵਿੱਥ = VL_BITBIT_E(ਸਰੋਤ_ਦੋਆਧਾਰੀ_ਅੰਕ);
        ਅਚਲ ਪੂਰਨ copy_ਚੌੜਾਈ = std::min(ਚੌੜਾਈ - (dest_ਦੋਆਧਾਰੀ_ਅੰਕ - destLsb), VL_EDATASIZE - dest_ਵਿੱਥ);
        ਅਚਲ uint32_t ਨਕਾਬ = VL_ਨਕਾਬ_E(copy_ਚੌੜਾਈ) << dest_ਵਿੱਥ;

        ਅਚਲ ਮੁੱਲ_And_X& ਸਰੋਤ_Lo = ਸਰੋਤ.m_ਅੰਕੜੇ.ਅੰਕ()[ਸਰੋਤ_ਸ਼ਬਦ];
        uint32_t ਮੁੱਲ = ਸਰੋਤ_Lo.m_ਮੁੱਲ >> ਸਰੋਤ_ਵਿੱਥ;
        uint32_t ਮੁੱਲ_X = ਸਰੋਤ_Lo.m_ਮੁੱਲ_X >> ਸਰੋਤ_ਵਿੱਥ;
        ਜੇ (ਸਰੋਤ_ਵਿੱਥ && copy_ਚੌੜਾਈ > VL_EDATASIZE - ਸਰੋਤ_ਵਿੱਥ) {
            ਅਚਲ ਮੁੱਲ_And_X& ਸਰੋਤ_Hi = ਸਰੋਤ.m_ਅੰਕੜੇ.ਅੰਕ()[ਸਰੋਤ_ਸ਼ਬਦ + 1];
            ਮੁੱਲ |= ਸਰੋਤ_Hi.m_ਮੁੱਲ << (VL_EDATASIZE - ਸਰੋਤ_ਵਿੱਥ);
            ਮੁੱਲ_X |= ਸਰੋਤ_Hi.m_ਮੁੱਲ_X << (VL_EDATASIZE - ਸਰੋਤ_ਵਿੱਥ);
        }

        ਮੁੱਲ_And_X& dest = m_ਅੰਕੜੇ.ਅੰਕ()[dest_ਸ਼ਬਦ];
        dest.m_ਮੁੱਲ = (dest.m_ਮੁੱਲ & ~ਨਕਾਬ) | ((ਮੁੱਲ << dest_ਵਿੱਥ) & ਨਕਾਬ);
        dest.m_ਮੁੱਲ_X = (dest.m_ਮੁੱਲ_X & ~ਨਕਾਬ) | ((ਮੁੱਲ_X << dest_ਵਿੱਥ) & ਨਕਾਬ);
    }
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opConcat(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Correct number of zero bits/width matters
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਸੈੱਟ_Zero();
    // See also error in V3Width
    ਜੇ (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_ਬਿੱਟ(0, rhs, 0, rhs.ਚੌੜਾਈ());
    copy_ਬਿੱਟ(rhs.ਚੌੜਾਈ(), lhs, 0, lhs.ਚੌੜਾਈ());
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLenN(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਸੈੱਟ_Quad(lhs.ਮੰਜ਼ਿਲ_String().ਲੰਮਾਈ());
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRepl(ਅਚਲ V3_ਸੰਖਿਆ& lhs,
                           ਅਚਲ V3_ਸੰਖਿਆ& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ਵਾਪਸ opRepl(lhs, rhs.ਮੰਜ਼ਿਲ_UInt());
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRepl(ਅਚਲ V3_ਸੰਖਿਆ& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਜੇ (v3Global.opt.replication_ਹੱਦਬੰਦੀ()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_ਹੱਦਬੰਦੀ())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_ਹੱਦਬੰਦੀ() << " is suspect: " << rhsval);
    }
    ਸੈੱਟ_Zero();
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ times = 0; times < rhsval; ++times) {
        ਅਚਲ uint64_t destLsb = uint64_t{times} * lhs.ਚੌੜਾਈ();
        ਜੇ (destLsb >= static_cast<uint32_t>(ਚੌੜਾਈ())) ਰੋਕੋ;
        copy_ਬਿੱਟ(static_cast<ਪੂਰਨ>(destLsb), lhs, 0,
                 std::min(lhs.ਚੌੜਾਈ(), ਚੌੜਾਈ() - static_cast<ਪੂਰਨ>(destLsb)));
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਧਾਰਾ_L(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਸੈੱਟ_Zero();
    // See also error in V3Width
    ਜੇ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ਅਚਲ ਪੂਰਨ ssize = std::min(rhs.ਮੰਜ਼ਿਲ_UInt(), static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(lhs.ਚੌੜਾਈ()));
    ਲਈ (ਪੂਰਨ istart = 0; istart < lhs.ਚੌੜਾਈ(); istart += ssize) {
        ਅਚਲ ਪੂਰਨ ostart = std::max(0, lhs.ਚੌੜਾਈ() - ssize - istart);
        copy_ਬਿੱਟ(ostart, lhs, istart, std::min(ssize, lhs.ਚੌੜਾਈ() - istart));
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLogAnd(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਅੱਖਰ loutc = 0;
    ਅੱਖਰ routc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < lhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            loutc = 1;
            ਰੋਕੋ;
        }
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && loutc == 0) loutc = 'x';
    }
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < rhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            routc = 1;
            ਰੋਕੋ;
        }
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && routc == 0) routc = 'x';
    }
    ਅੱਖਰ outc = 'x';
    ਜੇ (routc == 1 && loutc == 1) outc = 1;
    ਜੇ (routc == 0 || loutc == 0) outc = 0;
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLogOr(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਅੱਖਰ outc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < lhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 1;
            ਜਾਓ ਆਖਰੀ;
        }
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && outc == 0) outc = 'x';
    }
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < rhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 1;
            ਜਾਓ ਆਖਰੀ;
        }
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && outc == 0) outc = 'x';
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLogIf(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ਸੰਖਿਆ lnot = lhs;
    lnot.opLogNot(lhs);
    ਵਾਪਸ opLogOr(lnot, rhs);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLogEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ਸੰਖਿਆ ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_ਸੰਖਿਆ ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ਵਾਪਸ opLogAnd(ifa, ifb);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opAtoN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਪੂਰਨ ਅਧਾਰ) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    UASSERT(ਅਧਾਰ == AstAtoN::ATOREAL || ਅਧਾਰ == 2 || ਅਧਾਰ == 8 || ਅਧਾਰ == 10 || ਅਧਾਰ == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.ਮੰਜ਼ਿਲ_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.ਸਿਰਾ(), '_'), str.ਸਿਰਾ());

    ਜੇ (ਅਧਾਰ == AstAtoN::ATOREAL) ਵਾਪਸ ਸੈੱਟ_Double(std::atof(str.c_str()));

    errno = 0;
    ਸਵੈਚਾਲਿਤ v = std::strtol(str.c_str(), ਖਾਲੀ_ਸੂਚਕ, ਅਧਾਰ);
    ਜੇ (errno != 0) v = 0;
    ਵਾਪਸ ਸੈੱਟ_Long_S(static_cast<int32_t>(v));
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opPutcN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਅਚਲ V3_ਸੰਖਿਆ& ths) {
    ਅੰਕ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.ਮੰਜ਼ਿਲ_String();
    ਅਚਲ int32_t i = rhs.ਮੰਜ਼ਿਲ_SInt();
    ਅਚਲ int32_t c = ths.ਮੰਜ਼ਿਲ_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    ਜੇ (0 <= i && static_cast<uint32_t>(i) < lstring.ਲੰਮਾਈ() && c != 0) lstring[i] = c;
    ਵਾਪਸ ਸੈੱਟ_String(lstring);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGetcN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਅਚਲ string lstring = lhs.ਮੰਜ਼ਿਲ_String();
    ਅਚਲ int32_t i = rhs.ਮੰਜ਼ਿਲ_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    ਜੇ (0 <= i && static_cast<uint32_t>(i) < lstring.ਲੰਮਾਈ()) v = lstring[i];
    ਵਾਪਸ ਸੈੱਟ_Long(v);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSubstrN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਅਚਲ V3_ਸੰਖਿਆ& ths) {
    ਅੰਕ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਅਚਲ string lstring = lhs.ਮੰਜ਼ਿਲ_String();
    ਅਚਲ int32_t i = rhs.ਮੰਜ਼ਿਲ_SInt();
    ਅਚਲ int32_t j = ths.ਮੰਜ਼ਿਲ_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    ਜੇ (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.ਲੰਮਾਈ()) ਵਾਪਸ ਸੈੱਟ_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ਵਾਪਸ ਸੈੱਟ_String(lstring.substr(i, j - i + 1));
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opCompareNN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਤਾਰਕਿਕ ignoreCase) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    ਪੂਰਨ ਨਤੀਜਾ;
    ਅਚਲ string lstring = lhs.ਮੰਜ਼ਿਲ_String();
    ਅਚਲ string rstring = rhs.ਮੰਜ਼ਿਲ_String();
    ਜੇ (ignoreCase) {
        ਨਤੀਜਾ = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } ਨਹੀਂਤਾਂ {
        ਨਤੀਜਾ = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ਵਾਪਸ ਸੈੱਟ_Long_S(ਨਤੀਜਾ);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਜੇ (lhs.isString()) ਵਾਪਸ opEqN(lhs, rhs);
    ਜੇ (lhs.isDouble()) ਵਾਪਸ opEqD(lhs, rhs);
    ਅੱਖਰ outc = 1;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 0;
            ਜਾਓ ਆਖਰੀ;
        }
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 0;
            ਜਾਓ ਆਖਰੀ;
        }
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNeq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਜੇ (lhs.isString()) ਵਾਪਸ opNeqN(lhs, rhs);
    ਜੇ (lhs.isDouble()) ਵਾਪਸ opNeqD(lhs, rhs);
    ਅੱਖਰ outc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 1;
            ਜਾਓ ਆਖਰੀ;
        }
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 1;
            ਜਾਓ ਆਖਰੀ;
        }
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3Hash V3_ਸੰਖਿਆ::ਮੰਜ਼ਿਲ_Hash() ਅਚਲ {
    // Shouldmatch isCaseEq
    ਜੇ (isString()) ਵਾਪਸ V3Hash{ਮੰਜ਼ਿਲ_String()};
    ਜੇ (isDouble()) ਵਾਪਸ V3Hash{ਮੰਜ਼ਿਲ_Double()};
    V3Hash hash{ਚੌੜਾਈ()};
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) hash += m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ;
    ਵਾਪਸ hash;
}

ਤਾਰਕਿਕ V3_ਸੰਖਿਆ::isCaseEq(ਅਚਲ V3_ਸੰਖਿਆ& rhs) ਅਚਲ {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਜੇ (isString()) ਵਾਪਸ ਮੰਜ਼ਿਲ_String() == rhs.ਮੰਜ਼ਿਲ_String();
    ਜੇ (isDouble()) ਵਾਪਸ ਮੰਜ਼ਿਲ_Double() == rhs.ਮੰਜ਼ਿਲ_Double();
    ਜੇ (ਚੌੜਾਈ() != rhs.ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
    ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
        ਜੇ (!(m_ਅੰਕੜੇ.ਅੰਕ()[i] == rhs.m_ਅੰਕੜੇ.ਅੰਕ()[i])) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opCaseEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opCaseNeq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੱਖਰ outc = 0;
    ਜੇ (lhs.isString()) {
        ਵਾਪਸ opNeqN(lhs, rhs);
    } ਨਹੀਂਤਾਂ ਜੇ (lhs.isDouble()) {
        ਵਾਪਸ opNeqD(lhs, rhs);
    }
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2) != rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            outc = 1;
            ਜਾਓ ਆਖਰੀ;
        }
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opWildEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਅੱਖਰ outc = 1;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (!rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                outc = 'x';
                ਜਾਓ ਆਖਰੀ;
            }
            ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2) != rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                outc = 0;
                ਜਾਓ ਆਖਰੀ;
            }
        }
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opWildNeq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਅੱਖਰ outc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (!rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                outc = 'x';
                ਜਾਓ ਆਖਰੀ;
            }
            ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2) != rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                outc = 1;
                ਜਾਓ ਆਖਰੀ;
            }
        }
    }
ਆਖਰੀ:
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGt(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਅੱਖਰ outc = 0;
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ(), rhs.ਚੌੜਾਈ()); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 1;
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 0;
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
    }
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGtS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਅੱਖਰ outc = 0;
    {
        ਅਚਲ ਪੂਰਨ mbit = std::max(lhs.ਚੌੜਾਈ() - 1, rhs.ਚੌੜਾਈ() - 1);
        ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(mbit)) {
            outc = 'x';
        } ਨਹੀਂਤਾਂ ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(mbit)) {
            outc = 'x';
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(mbit) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1_Extend(mbit) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(mbit)) {
            outc = 0;  // - !> +
        } ਨਹੀਂਤਾਂ {
            // both positive or negative, normal >
            ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < std::max(lhs.ਚੌੜਾਈ() - 1, rhs.ਚੌੜਾਈ() - 1); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
                ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1_Extend(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 1;
                ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1_Extend(ਦੋਆਧਾਰੀ_ਅੰਕ_2) && lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 0;
                ਜੇ (lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
                ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) outc = 'x';
            }
        }
    }
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(outc);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGte(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ਸੰਖਿਆ& eq = opEq(lhs, rhs);
    ਜੇ (eq.isNeqZero()) ਵਾਪਸ eq;  // Return true
    ਵਾਪਸ opGt(lhs, rhs);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGteS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ਸੰਖਿਆ& eq = opEq(lhs, rhs);
    ਜੇ (eq.isNeqZero()) ਵਾਪਸ eq;  // Return true
    ਵਾਪਸ opGtS(lhs, rhs);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLt(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਵਾਪਸ opGt(rhs, lhs); }
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLte(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਵਾਪਸ opGte(rhs, lhs); }
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLtS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਵਾਪਸ opGtS(rhs, lhs); }
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLteS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) { ਵਾਪਸ opGteS(rhs, lhs); }

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਖਿਸਕਾਓ_R(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਸੈੱਟ_Zero();
    ਜੇ (rhs.ਚੌੜਾਈ() > 32 && !rhs.is_ਬਿੱਟ_Zero(rhs.ਚੌੜਾਈ() - 1, 32)) ਵਾਪਸ *ਇਹ;
    ਅਚਲ uint32_t rhsval = rhs.ਮੰਜ਼ਿਲ_UInt();
    ਜੇ (rhsval < static_cast<uint32_t>(lhs.ਚੌੜਾਈ())) {
        copy_ਬਿੱਟ(0, lhs, static_cast<ਪੂਰਨ>(rhsval),
                 std::min(ਚੌੜਾਈ(), lhs.ਚੌੜਾਈ() - static_cast<ਪੂਰਨ>(rhsval)));
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਖਿਸਕਾਓ_RS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਸੈੱਟ_Zero();
    ਅਚਲ ਤਾਰਕਿਕ ਉਛਾਲ = rhs.ਚੌੜਾਈ() > 32 && !rhs.is_ਬਿੱਟ_Zero(rhs.ਚੌੜਾਈ() - 1, 32);
    ਜੇ (!ਉਛਾਲ) {
        ਅਚਲ uint32_t rhsval = rhs.ਮੰਜ਼ਿਲ_UInt();
        ਜੇ (rhsval < lbits) {
            ਅਚਲ ਪੂਰਨ copy_ਚੌੜਾਈ = std::min(ਚੌੜਾਈ(), static_cast<ਪੂਰਨ>(lbits - rhsval));
            copy_ਬਿੱਟ(0, lhs, static_cast<ਪੂਰਨ>(rhsval), copy_ਚੌੜਾਈ);
            fill_ਬਿੱਟ(copy_ਚੌੜਾਈ, ਚੌੜਾਈ() - copy_ਚੌੜਾਈ, lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(lbits - 1));
            ਵਾਪਸ *ਇਹ;
        }
    }
    fill_ਬਿੱਟ(0, ਚੌੜਾਈ(), lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(lbits - 1));  // '0/'1/'x/'z
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਖਿਸਕਾਓ_L(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਸੈੱਟ_Zero();
    ਜੇ (rhs.ਚੌੜਾਈ() > 32 && !rhs.is_ਬਿੱਟ_Zero(rhs.ਚੌੜਾਈ() - 1, 32)) ਵਾਪਸ *ਇਹ;
    ਅਚਲ uint32_t rhsval = rhs.ਮੰਜ਼ਿਲ_UInt();
    ਜੇ (rhsval < static_cast<uint32_t>(ਚੌੜਾਈ())) {
        copy_ਬਿੱਟ(static_cast<ਪੂਰਨ>(rhsval), lhs, 0,
                 std::min(ਚੌੜਾਈ() - static_cast<ਪੂਰਨ>(rhsval), lhs.ਚੌੜਾਈ()));
    }
    ਵਾਪਸ *ਇਹ;
}

//======================================================================
// Ops - Arithmetic

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNegate(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਜੇ (lhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    V3_ਸੰਖਿਆ notlhs(&lhs, ਚੌੜਾਈ());
    notlhs.opNot(lhs);
    ਅਚਲ V3_ਸੰਖਿਆ one(&lhs, ਚੌੜਾਈ(), 1);
    op_ਜੋੜੋ(notlhs, one);
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਜੋੜੋ(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਸੈੱਟ_Zero();
    // Addem
    uint64_t ਹਾਸਲ = 0;
    ਲਈ (ਪੂਰਨ ਸ਼ਬਦ = 0; ਸ਼ਬਦ < ਸ਼ਬਦ_2(); ++ਸ਼ਬਦ) {
        ਅਚਲ uint64_t lwordval = lhs.m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ].m_ਮੁੱਲ;
        ਅਚਲ uint64_t rwordval = rhs.m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ].m_ਮੁੱਲ;
        ਅਚਲ uint64_t ਜੋੜ = lwordval + rwordval + ਹਾਸਲ;
        m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ].m_ਮੁੱਲ = ਜੋੜ & 0xffffffffULL;
        ਹਾਸਲ = ਜੋੜ > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSub(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    V3_ਸੰਖਿਆ negrhs(&rhs, ਚੌੜਾਈ());
    negrhs.opNegate(rhs);
    ਵਾਪਸ op_ਜੋੜੋ(lhs, negrhs);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opMul(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਸੈੱਟ_Zero();
    ਜੇ (ਚੌੜਾਈ() <= 64) {
        ਸੈੱਟ_Quad(lhs.ਮੰਜ਼ਿਲ_UQuad() * rhs.ਮੰਜ਼ਿਲ_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } ਨਹੀਂਤਾਂ {
        ਲਈ (ਪੂਰਨ lword = 0; lword < lhs.ਸ਼ਬਦ_2(); ++lword) {
            ਅਚਲ uint64_t lwordval = static_cast<uint64_t>(lhs.m_ਅੰਕੜੇ.ਅੰਕ()[lword].m_ਮੁੱਲ);
            ਜੇ (lwordval == 0) ਜਾਰੀ;
            ਲਈ (ਪੂਰਨ rword = 0; rword < rhs.ਸ਼ਬਦ_2(); ++rword) {
                ਅਚਲ uint64_t rwordval = static_cast<uint64_t>(rhs.m_ਅੰਕੜੇ.ਅੰਕ()[rword].m_ਮੁੱਲ);
                ਜੇ (rwordval == 0) ਜਾਰੀ;
                uint64_t mul = lwordval * rwordval;
                ਲਈ (ਪੂਰਨ qword = lword + rword; qword < ਸ਼ਬਦ_2(); ++qword) {
                    mul += static_cast<uint64_t>(m_ਅੰਕੜੇ.ਅੰਕ()[qword].m_ਮੁੱਲ);
                    m_ਅੰਕੜੇ.ਅੰਕ()[qword].m_ਮੁੱਲ = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    ਜੇ (mul == 0) ਰੋਕੋ;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opMulS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    V3_ਸੰਖਿਆ lhsNoSign = lhs;
    ਜੇ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ਸੰਖਿਆ rhsNoSign = rhs;
    ਜੇ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ਅਚਲ V3_ਸੰਖਿਆ qNoSign = opMul(lhsNoSign, rhsNoSign);
    ਜੇ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ਨਹੀਂਤਾਂ {
        op_ਸੌਂਪੋ(qNoSign);
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opDiv(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਜੇ (rhs.isEqZero()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_XRemoved();
    ਜੇ (lhs.ਚੌੜਾਈ() <= 64) {
        ਸੈੱਟ_Quad(lhs.ਮੰਜ਼ਿਲ_UQuad() / rhs.ਮੰਜ਼ਿਲ_UQuad());
        ਵਾਪਸ *ਇਹ;
    } ਨਹੀਂਤਾਂ {
        // Wide division
        ਵਾਪਸ opModDivGuts(lhs, rhs, ਝੂਠ);
    }
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opDivS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਜੇ (rhs.isEqZero()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_XRemoved();
    V3_ਸੰਖਿਆ lhsNoSign = lhs;
    ਜੇ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ਸੰਖਿਆ rhsNoSign = rhs;
    ਜੇ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ਅਚਲ V3_ਸੰਖਿਆ qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    ਜੇ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ਨਹੀਂਤਾਂ {
        op_ਸੌਂਪੋ(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *ਇਹ);
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opModDiv(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਜੇ (rhs.isEqZero()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_XRemoved();
    ਜੇ (lhs.ਚੌੜਾਈ() <= 64) {
        ਸੈੱਟ_Quad(lhs.ਮੰਜ਼ਿਲ_UQuad() % rhs.ਮੰਜ਼ਿਲ_UQuad());
        ਵਾਪਸ *ਇਹ;
    } ਨਹੀਂਤਾਂ {
        // Wide modulus
        ਵਾਪਸ opModDivGuts(lhs, rhs, ਸੱਚ);
    }
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opModDivS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਜੇ (rhs.isEqZero()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_XRemoved();
    V3_ਸੰਖਿਆ lhsNoSign = lhs;
    ਜੇ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ਸੰਖਿਆ rhsNoSign = rhs;
    ਜੇ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ਅਚਲ V3_ਸੰਖਿਆ qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    ਜੇ (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } ਨਹੀਂਤਾਂ {
        op_ਸੌਂਪੋ(qNoSign);
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opModDivGuts(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਤਾਰਕਿਕ is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਸੈੱਟ_Zero();
    // Find MSB and check for zero.
    ਅਚਲ ਪੂਰਨ l_ਸ਼ਬਦ = lhs.ਸ਼ਬਦ_2();
    ਅਚਲ ਪੂਰਨ umsbp1 = lhs.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1();  // dividend
    ਅਚਲ ਪੂਰਨ vmsbp1 = rhs.most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1();  //ਭਾਜਕ
    ਜੇ (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *ਇਹ);
        ਵਾਪਸ *ਇਹ;
    }

    ਅਚਲ ਪੂਰਨ uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ਅਚਲ ਪੂਰਨ vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    ਜੇ (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        ਲਈ (ਪੂਰਨ j = uw - 1; j >= 0; j--) {
            ਅਚਲ uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_ਅੰਕੜੇ.ਅੰਕ()[j].m_ਮੁੱਲ));
            m_ਅੰਕੜੇ.ਅੰਕ()[j].m_ਮੁੱਲ = unw64 / static_cast<uint64_t>(rhs.m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ);
            k = unw64
                - (static_cast<uint64_t>(m_ਅੰਕੜੇ.ਅੰਕ()[j].m_ਮੁੱਲ)
                   * static_cast<uint64_t>(rhs.m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *ਇਹ << " rem=0x" << std::hex
                                   << k << std::dec);
        ਜੇ (is_modulus) {
            ਸੈੱਟ_Zero();
            m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = k;
        }
        opCleanThis();
        ਵਾਪਸ *ਇਹ;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_ਸ਼ਬਦ + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_ਸ਼ਬਦ + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    ਲਈ (ਪੂਰਨ i = 0; i < l_ਸ਼ਬਦ; ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ = 0;
    ਲਈ (ਪੂਰਨ i = 0; i < l_ਸ਼ਬਦ + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ਅਚਲ ਪੂਰਨ s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ਅਚਲ uint32_t ਖਿਸਕਾਓ_ਨਕਾਬ = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    ਲਈ (ਪੂਰਨ i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ << s)
                | (ਖਿਸਕਾਓ_ਨਕਾਬ & (rhs.m_ਅੰਕੜੇ.ਅੰਕ()[i - 1].m_ਮੁੱਲ >> (32 - s)));
    }
    vn[0] = rhs.m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ << s;

    // Copy and shift dividend by same amount; may set new upper word
    ਜੇ (s) {
        un[uw] = lhs.m_ਅੰਕੜੇ.ਅੰਕ()[uw - 1].m_ਮੁੱਲ >> (32 - s);
    } ਨਹੀਂਤਾਂ {
        un[uw] = 0;
    }
    ਲਈ (ਪੂਰਨ i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ << s)
                | (ਖਿਸਕਾਓ_ਨਕਾਬ & (lhs.m_ਅੰਕੜੇ.ਅੰਕ()[i - 1].m_ਮੁੱਲ >> (32 - s)));
    }
    un[0] = lhs.m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    ਲਈ (ਪੂਰਨ j = uw - vw; j >= 0; j--) {
        // Estimate
        ਅਚਲ uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        ਜੇ (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            ਜੇ (rhat < 0x100000000ULL) ਜਾਓ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        ਲਈ (ਪੂਰਨ i = 0; i < vw; ++i) {
            ਅਚਲ uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //ਘਟਾਓ
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_ਅੰਕੜੇ.ਅੰਕ()[j].m_ਮੁੱਲ = qhat;  // Save quotient digit

        ਜੇ (t < 0) {
            // Over subtracted; correct by adding back
            m_ਅੰਕੜੇ.ਅੰਕ()[j].m_ਮੁੱਲ--;
            k = 0;
            ਲਈ (ਪੂਰਨ i = 0; i < vw; ++i) {
                t = static_cast<uint64_t>(un[i + j]) + static_cast<uint64_t>(vn[i]) + k;
                un[i + j] = t;
                k = t >> 32ULL;
            }
            un[j + vw] = un[j + vw] + k;
        }
    }

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    ਜੇ (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        ਲਈ (ਪੂਰਨ i = 0; i < vw; ++i) {
            m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ = (un[i] >> s) | (ਖਿਸਕਾਓ_ਨਕਾਬ & (un[i + 1] << (32 - s)));
        }
        ਲਈ (ਪੂਰਨ i = vw; i < l_ਸ਼ਬਦ; ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i].m_ਮੁੱਲ = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *ਇਹ);
        ਵਾਪਸ *ਇਹ;
    } ਨਹੀਂਤਾਂ {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *ਇਹ);
        ਵਾਪਸ *ਇਹ;
    }
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opPow(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਤਾਰਕਿਕ lsign, ਤਾਰਕਿਕ rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ਜੇ (lhs.isFourState() || rhs.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਜੇ (rhs.isEqZero()) ਵਾਪਸ ਸੈੱਟ_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    ਜੇ (rsign && rhs.isNegative()) {
        ਜੇ (lhs.isEqZero()) {
            ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_XRemoved();
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.isEqOne()) {
            ਵਾਪਸ ਸੈੱਟ_Value1();
        } ਨਹੀਂਤਾਂ ਜੇ (lsign && lhs.isEqAllOnes()) {
            ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(0)) {
                ਵਾਪਸ ਸੈੱਟ_All_Bits1();  // -1^odd=-1
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਸੈੱਟ_Value1();  // -1^even=1
            }
        }
        ਵਾਪਸ ਸੈੱਟ_Zero();
    }
    ਜੇ (lhs.isEqZero()) ਵਾਪਸ ਸੈੱਟ_Zero();
    ਸੈੱਟ_Zero();
    m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = 1;
    V3_ਸੰਖਿਆ power(&lhs, ਚੌੜਾਈ());
    power.op_ਸੌਂਪੋ(lhs);
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < rhs.ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 > 0) {  // power = power*power
            V3_ਸੰਖਿਆ ਆਖਰੀ_Power(&lhs, ਚੌੜਾਈ());
            ਆਖਰੀ_Power.op_ਸੌਂਪੋ(power);
            power.opMul(ਆਖਰੀ_Power, ਆਖਰੀ_Power);
        }
        ਜੇ (rhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {  // out *= power
            V3_ਸੰਖਿਆ ਆਖਰੀ_ਬਾਹਰਲਾ(&lhs, ਚੌੜਾਈ());
            ਆਖਰੀ_ਬਾਹਰਲਾ.op_ਸੌਂਪੋ(*ਇਹ);
            opMul(ਆਖਰੀ_ਬਾਹਰਲਾ, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opPowSU(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Correct number of zero bits/width matters
    ਵਾਪਸ opPow(lhs, rhs, ਸੱਚ, ਝੂਠ);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opPowSS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Correct number of zero bits/width matters
    ਵਾਪਸ opPow(lhs, rhs, ਸੱਚ, ਸੱਚ);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opPowUS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // Correct number of zero bits/width matters
    ਵਾਪਸ opPow(lhs, rhs, ਝੂਠ, ਸੱਚ);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opBufIf1(ਅਚਲ V3_ਸੰਖਿਆ& ens, ਅਚਲ V3_ਸੰਖਿਆ& if1s) {
    ਅੰਕ_ASSERT_OP_ARGS2(ens, if1s);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(ens, if1s);
    ਸੈੱਟ_Zero();
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 < ਚੌੜਾਈ(); ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        ਜੇ (ens.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, if1s.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਦੋਆਧਾਰੀ_ਅੰਕ_2));
        } ਨਹੀਂਤਾਂ {
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਦੋਆਧਾਰੀ_ਅੰਕ_2, 'z');
        }
    }
    ਵਾਪਸ *ਇਹ;
}

// Sets all bits in range to the given value
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਸੈੱਟ_Range(uint32_t lsb, uint32_t ਚੌੜਾਈ, ਅੱਖਰ ਦੋਆਧਾਰੀ_ਅੰਕ_ਮੁੱਲ) {
    ਜੇ (lsb < static_cast<uint32_t>(ਇਹ->ਚੌੜਾਈ())) {
        fill_ਬਿੱਟ(static_cast<ਪੂਰਨ>(lsb),
                 static_cast<ਪੂਰਨ>(std::min(ਚੌੜਾਈ, static_cast<uint32_t>(ਇਹ->ਚੌੜਾਈ()) - lsb)),
                 ਦੋਆਧਾਰੀ_ਅੰਕ_ਮੁੱਲ);
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਸੌਂਪੋ(ਅਚਲ V3_ਸੰਖਿਆ& lhs) { ਵਾਪਸ op_ਸੌਂਪੋ_Non_XZ(lhs, ਝੂਠ); }
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਸੌਂਪੋ_Non_XZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਤਾਰਕਿਕ ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    ਜੇ (ਇਹ != &lhs) {
        ਜੇ (VL_UNLIKELY(lhs.isNull())) {
            m_ਅੰਕੜੇ.m_isNull = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (isString()) {
            ਜੇ (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_ਅੰਕੜੇ.str() = lhs.ਮੰਜ਼ਿਲ_String();
            } ਨਹੀਂਤਾਂ {
                m_ਅੰਕੜੇ.str() = lhs.m_ਅੰਕੜੇ.str();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            ਸੈੱਟ_Zero();
        } ਨਹੀਂਤਾਂ ਜੇ (lhs.isDouble()) {
            ਸੈੱਟ_Double(lhs.ਮੰਜ਼ਿਲ_Double());
        } ਨਹੀਂਤਾਂ ਜੇ (!ignoreXZ) {
            ਸੈੱਟ_Zero();
            copy_ਬਿੱਟ(0, lhs, 0, std::min(ਚੌੜਾਈ(), lhs.ਚੌੜਾਈ()));
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਪੂਰਨ ਸ਼ਬਦ = 0; ਸ਼ਬਦ < ਸ਼ਬਦ_2(); ++ਸ਼ਬਦ) {
                ਅਚਲ uint32_t ਮੁੱਲ = ਸ਼ਬਦ < lhs.ਸ਼ਬਦ_2() ? lhs.m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ].m_ਮੁੱਲ
                                                                & ~lhs.m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ].m_ਮੁੱਲ_X
                                                          : 0;
                m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ] = {ਮੁੱਲ, 0};
            }
            opCleanThis();
        }
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNToI(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    // String to packed number
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਅਚਲ string& str = lhs.ਮੰਜ਼ਿਲ_String();
    ਲਈ (ਮਾਪ_t n = 0; n < str.ਲੰਮਾਈ(); ++n) {
        ਅਚਲ ਅੱਖਰ c = str[str.ਲੰਮਾਈ() - 1 - n];
        ਲਈ (ਮਾਪ_t cbit = 0; cbit < 8; ++cbit) {
            ਅਚਲ std::bitset<8> sbit{static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ>(c)};
            ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opExtendS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਅਚਲ ਪੂਰਨ copy_ਚੌੜਾਈ = std::min(ਚੌੜਾਈ(), static_cast<ਪੂਰਨ>(lbits));
    copy_ਬਿੱਟ(0, lhs, 0, copy_ਚੌੜਾਈ);
    fill_ਬਿੱਟ(copy_ਚੌੜਾਈ, ਚੌੜਾਈ() - copy_ਚੌੜਾਈ, lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(lbits - 1));
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opExtendXZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਅਚਲ ਪੂਰਨ copy_ਚੌੜਾਈ = std::min(ਚੌੜਾਈ(), static_cast<ਪੂਰਨ>(lbits));
    copy_ਬਿੱਟ(0, lhs, 0, copy_ਚੌੜਾਈ);
    fill_ਬਿੱਟ(copy_ਚੌੜਾਈ, ਚੌੜਾਈ() - copy_ਚੌੜਾਈ, lhs.ਦੋਆਧਾਰੀ_ਅੰਕ_Is(lbits - 1));
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opClean(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t ਬਿੱਟ) { ਵਾਪਸ opSel(lhs, ਬਿੱਟ - 1, 0); }

ਖਾਲੀ V3_ਸੰਖਿਆ::opCleanThis(ਤਾਰਕਿਕ warnOnTruncation) {
    // Clean MSB of number
    ਅੰਕ_ASSERT_LOGIC_ARGS1(*ਇਹ);
    ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ_2() - 1];
    ਅਚਲ uint32_t new_ਮੁੱਲ_Msb = v.m_ਮੁੱਲ & hi_ਸ਼ਬਦ_ਨਕਾਬ();
    ਅਚਲ uint32_t new_ਮੁੱਲ_XMsb = v.m_ਮੁੱਲ_X & hi_ਸ਼ਬਦ_ਨਕਾਬ();
    ਜੇ (warnOnTruncation && (new_ਮੁੱਲ_Msb != v.m_ਮੁੱਲ || new_ਮੁੱਲ_XMsb != v.m_ਮੁੱਲ_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << ਚੌੜਾਈ() << " bit number: " << displayed("%d"));
    }
    m_ਅੰਕੜੇ.ਅੰਕ()[ਸ਼ਬਦ_2() - 1] = {new_ਮੁੱਲ_Msb, new_ਮੁੱਲ_XMsb};
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSel(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& msb, ਅਚਲ V3_ਸੰਖਿਆ& lsb) {
    ਅੰਕ_ASSERT_OP_ARGS3(lhs, msb, lsb);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS2(msb, lsb);
    ਜੇ (lsb.isFourState() || msb.isFourState()) ਵਾਪਸ ਸੈੱਟ_All_ਬਿੱਟ_X();
    ਵਾਪਸ opSel(lhs, msb.ਮੰਜ਼ਿਲ_UInt(), lsb.ਮੰਜ਼ਿਲ_UInt());
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSel(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t msbval, uint32_t lsbval) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    ਸੈੱਟ_Zero();
    ਪੂਰਨ copy_ਚੌੜਾਈ = 0;
    ਜੇ (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.ਚੌੜਾਈ())) {
        ਅਚਲ uint64_t selected_ਚੌੜਾਈ = uint64_t{msbval} - lsbval + 1;
        copy_ਚੌੜਾਈ = static_cast<ਪੂਰਨ>(
            std::min({static_cast<uint64_t>(ਚੌੜਾਈ()), static_cast<uint64_t>(lhs.ਚੌੜਾਈ()) - lsbval,
                      selected_ਚੌੜਾਈ}));
    }
    copy_ਬਿੱਟ(0, lhs, copy_ਚੌੜਾਈ ? static_cast<ਪੂਰਨ>(lsbval) : 0, copy_ਚੌੜਾਈ);
    fill_ਬਿੱਟ(copy_ਚੌੜਾਈ, ਚੌੜਾਈ() - copy_ਚੌੜਾਈ, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ਵਾਪਸ *ਇਹ;
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSelInto(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& lsb, ਪੂਰਨ ਚੌੜਾਈ) {
    ਵਾਪਸ opSelInto(lhs, lsb.ਮੰਜ਼ਿਲ_SInt(), ਚੌੜਾਈ);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSelInto(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਪੂਰਨ lsbval, ਪੂਰਨ ਚੌੜਾਈ) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ਅਚਲ ਪੂਰਨ copy_ਚੌੜਾਈ = std::max(0, std::min({ਚੌੜਾਈ, lhs.ਚੌੜਾਈ(), ਇਹ->ਚੌੜਾਈ() - lsbval}));
    copy_ਬਿੱਟ(lsbval, lhs, 0, copy_ਚੌੜਾਈ);
    ਅਚਲ ਪੂਰਨ fill_ਚੌੜਾਈ
        = std::max(0, std::min(ਚੌੜਾਈ - copy_ਚੌੜਾਈ, ਇਹ->ਚੌੜਾਈ() - lsbval - copy_ਚੌੜਾਈ));
    fill_ਬਿੱਟ(lsbval + copy_ਚੌੜਾਈ, fill_ਚੌੜਾਈ, v3Global.constRemoveXs() ? 0 : 'x');
    ਵਾਪਸ *ਇਹ;
}

//======================================================================
// Ops - Floating point

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opIToRD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਤਾਰਕਿਕ isSigned) {
    // Correct number of zero bits/width matters
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_ਸੰਖਿਆ noxz(lhs);
    noxz.op_ਸੌਂਪੋ_Non_XZ(lhs);
    ਦੁੱਗਣਾ d = 0;
    ਅਚਲ ਤਾਰਕਿਕ negate = isSigned && noxz.isNegative();
    ਜੇ (negate) {
        ਅਚਲ V3_ਸੰਖਿਆ noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = noxz.ਚੌੜਾਈ() - 1; ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2--) {
        // Some precision might be lost in this add, that's what we want
        ਜੇ (noxz.ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਦੋਆਧਾਰੀ_ਅੰਕ_2)) d += exp2(ਦੋਆਧਾਰੀ_ਅੰਕ_2);
    }
    ਜੇ (negate) d = -d;
    ਵਾਪਸ ਸੈੱਟ_Double(d);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRToIS(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS1(lhs);
    ਅਚਲ ਦੁੱਗਣਾ v = VL_TRUNC(lhs.ਮੰਜ਼ਿਲ_Double());
    ਅਚਲ int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ਵਾਪਸ ਸੈੱਟ_Long_S(i);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opRToIRoundS(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS1(lhs);
    ਅਚਲ ਦੁੱਗਣਾ v = VL_ਗੇੜਾ(lhs.ਮੰਜ਼ਿਲ_Double());
    ਸੈੱਟ_Zero();
    ਸੰਘ {
        ਦੁੱਗਣਾ d;
        uint64_t q;
    } u;
    u.d = v;
    (ਖਾਲੀ)u.d;

    ਅਚਲ ਪੂਰਨ ਘਾਤਾਂਕ = static_cast<ਪੂਰਨ>((u.q >> 52ULL) & VL_ਨਕਾਬ_Q(11)) - 1023;
    ਅਚਲ ਪੂਰਨ lsb = ਘਾਤਾਂਕ - 52;
    ਅਚਲ uint64_t mantissa = (u.q & VL_ਨਕਾਬ_Q(52)) | (1ULL << 52);
    ਜੇ (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        ਲਈ (ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = 0; ਦੋਆਧਾਰੀ_ਅੰਕ_2 <= 52; ++ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
            ਜੇ (mantissa & (1ULL << ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                ਅਚਲ ਪੂਰਨ outbit = ਦੋਆਧਾਰੀ_ਅੰਕ_2 + lsb;
                ਜੇ (outbit >= 0) ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(outbit, 1);
            }
        }
        ਜੇ (v < 0) {
            ਅਚਲ V3_ਸੰਖਿਆ noSign = *ਇਹ;
            opNegate(noSign);
        }
    }
    ਵਾਪਸ *ਇਹ;
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.ਚੌੜਾਈ() == 64 && ਚੌੜਾਈ() == 64, "Real operation on wrong sized number");
    ਸੰਘ {
        ਦੁੱਗਣਾ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.ਮੰਜ਼ਿਲ_Double();
    ਵਾਪਸ ਸੈੱਟ_Quad(u.m_v);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.ਚੌੜਾਈ() == 64 && ਚੌੜਾਈ() == 64, "Real operation on wrong sized number");
    ਸੰਘ {
        ਦੁੱਗਣਾ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.ਮੰਜ਼ਿਲ_UQuad();
    ਵਾਪਸ ਸੈੱਟ_Double(u.m_d);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNegateD(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS1(lhs);
    ਵਾਪਸ ਸੈੱਟ_Double(-lhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਜੋੜੋ_D(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Double(lhs.ਮੰਜ਼ਿਲ_Double() + rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opSubD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Double(lhs.ਮੰਜ਼ਿਲ_Double() - rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opMulD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Double(lhs.ਮੰਜ਼ਿਲ_Double() * rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opDivD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Double(lhs.ਮੰਜ਼ਿਲ_Double() / rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opPowD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Double(ਘਾਤ(lhs.ਮੰਜ਼ਿਲ_Double(), rhs.ਮੰਜ਼ਿਲ_Double()));
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opEqD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_Double() == rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNeqD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_Double() != rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGtD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_Double() > rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGteD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_Double() >= rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLtD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_Double() < rhs.ਮੰਜ਼ਿਲ_Double());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLteD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_Double() <= rhs.ਮੰਜ਼ਿਲ_Double());
}

//======================================================================
// Ops - String

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opConcatN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_String(lhs.ਮੰਜ਼ਿਲ_String() + rhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opReplN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਅੰਕ_ASSERT_LOGIC_ARGS1(rhs);
    ਵਾਪਸ opReplN(lhs, rhs.ਮੰਜ਼ਿਲ_UInt());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opReplN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t rhsval) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    string ਬਾਹਰਲਾ;
    ਬਾਹਰਲਾ.reserve(lhs.ਮੰਜ਼ਿਲ_String().ਲੰਮਾਈ() * rhsval);
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ times = 0; times < rhsval; ++times) ਬਾਹਰਲਾ += lhs.ਮੰਜ਼ਿਲ_String();
    ਵਾਪਸ ਸੈੱਟ_String(ਬਾਹਰਲਾ);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opCvtPackString(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਵਾਪਸ ਸੈੱਟ_String(lhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਅਚਲ std::string ਬਾਹਰਲਾ = VString::downcase(lhs.ਮੰਜ਼ਿਲ_String());
    ਵਾਪਸ ਸੈੱਟ_String(ਬਾਹਰਲਾ);
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::op_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N(ਅਚਲ V3_ਸੰਖਿਆ& lhs) {
    ਅੰਕ_ASSERT_OP_ARGS1(lhs);
    ਅੰਕ_ASSERT_STRING_ARGS1(lhs);
    ਅਚਲ std::string ਬਾਹਰਲਾ = VString::upcase(lhs.ਮੰਜ਼ਿਲ_String());
    ਵਾਪਸ ਸੈੱਟ_String(ਬਾਹਰਲਾ);
}

V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opEqN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_String() == rhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opNeqN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_String() != rhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGtN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_String() > rhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opGteN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_String() >= rhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLtN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_String() < rhs.ਮੰਜ਼ਿਲ_String());
}
V3_ਸੰਖਿਆ& V3_ਸੰਖਿਆ::opLteN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs) {
    ਅੰਕ_ASSERT_OP_ARGS2(lhs, rhs);
    ਅੰਕ_ASSERT_STRING_ARGS2(lhs, rhs);
    ਵਾਪਸ ਸੈੱਟ_Single_ਬਿੱਟ(lhs.ਮੰਜ਼ਿਲ_String() <= rhs.ਮੰਜ਼ਿਲ_String());
}

//======================================================================

ਖਾਲੀ V3_ਸੰਖਿਆ::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_ਸਤਰ* ਅਚਲ fl = ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::built_ਵਿੱਚ_Filename()};
    V3_ਸੰਖਿਆ ਅੰਕ{fl, 32, 0};
    ਅੰਕ.selfTestThis();
}

ਖਾਲੀ V3_ਸੰਖਿਆ::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonEqual(0, 0), ਸੱਚ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonEqual(1e19, 1e19), ਸੱਚ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonEqual(9, 0.0001), ਝੂਠ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonEqual(1, 1 + std::numeric_limits<ਦੁੱਗਣਾ>::epsilon()), ਸੱਚ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonEqual(0.009, 0.00899999999999999931998839741709), ਸੱਚ);

    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(0), ਸੱਚ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(1), ਸੱਚ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(-1), ਸੱਚ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(1.0001), ਝੂਠ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(0.9999), ਝੂਠ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(-1.0001), ਝੂਠ);
    UASSERT_SELFTEST(V3_ਸੰਖਿਆ::epsilonIntegral(-0.9999), ਝੂਠ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
