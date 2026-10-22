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

#شامل "V3PchAstMT.h"

#شامل "V3Number.h"

#شامل "V3File.h"

#شامل <algorithm>
#شامل <cerrno>
#شامل <cmath>
#شامل <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr صحیح MAX_SPRINTF_DOUBLE_سائز
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#تعریف_کرو عدد_ASSERT_OP_ARGS1(arg1) \
    UASSERT((یہ != &(arg1)), "Number operation called with same source and dest")
#تعریف_کرو عدد_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((یہ != &(arg1) && یہ != &(arg2)), \
            "Number operation called with same source and dest")
#تعریف_کرو عدد_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((یہ != &(arg1) && یہ != &(arg2) && یہ != &(arg3)), \
            "Number operation called with same source and dest")
#تعریف_کرو عدد_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((یہ != &(arg1) && یہ != &(arg2) && یہ != &(arg3) && یہ != &(arg4)), \
            "Number operation called with same source and dest")

#تعریف_کرو عدد_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).ڈیٹا_Type() == V3_نمبر_ڈیٹا::V3_نمبر_ڈیٹا_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#تعریف_کرو عدد_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    عدد_ASSERT_LOGIC_ARGS1(arg1); \
    عدد_ASSERT_LOGIC_ARGS1(arg2)

#تعریف_کرو عدد_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    عدد_ASSERT_LOGIC_ARGS1(arg1); \
    عدد_ASSERT_LOGIC_ARGS1(arg2); \
    عدد_ASSERT_LOGIC_ARGS1(arg3); \
    عدد_ASSERT_LOGIC_ARGS1(arg4)

#تعریف_کرو عدد_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#تعریف_کرو عدد_ASSERT_STRING_ARGS2(arg1, arg2) \
    عدد_ASSERT_STRING_ARGS1(arg1); \
    عدد_ASSERT_STRING_ARGS1(arg2)

#تعریف_کرو عدد_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#تعریف_کرو عدد_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    عدد_ASSERT_DOUBLE_ARGS1(arg1); \
    عدد_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

خالی V3_نمبر::v3error_انتہا(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    اگر (m_nodep) {
        m_nodep->v3error_انتہا(nsstr);
    } ورنہ اگر (m_fileline) {
        m_fileline->v3error_انتہا(nsstr);
    } ورنہ {
        V3Error::v3error_انتہا(nsstr, "", خالی_اشاریہ);
    }
}

خالی V3_نمبر::v3error_انتہا_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_انتہا(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_نمبر::V3_نمبر(Ast_گرہ* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    اگر (str.خالی_خانے()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } ورنہ {
        init(nodep, std::max<صحیح>(str.لمبائی_2() * 8, 1));
        برائے (غیرعلامتی pos = 0; pos < str.لمبائی_2(); ++pos) {
            ثابت صحیح topos = str.لمبائی_2() - 1 - pos;
            قدر_And_X& v = m_ڈیٹا.عدد()[topos / 4];
            برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < 8; ++ثنائی_ہندسہ_2) {
                اگر (str[pos] & (1UL << ثنائی_ہندسہ_2)) v.m_قدر |= (1UL << (ثنائی_ہندسہ_2 + (topos % 4) * 8));
            }
        }
    }
    m_ڈیٹا.m_fromString = سچ;
    opCleanThis(سچ);
}

V3_نمبر::V3_نمبر(Ast_گرہ* nodep, ثابت Ast_گرہ_DType* nodedtypep) {
    اگر (nodedtypep->isCHandle()) {
        init(nodep);
        سیٹ_Null();
    } ورنہ اگر (nodedtypep->isDouble()) {
        init(nodep, 64);
        سیٹ_Double(0.0);
    } ورنہ اگر (nodedtypep->isString()) {
        init(nodep);
        سیٹ_String("");
    } ورنہ {
        init(nodep, nodedtypep->چوڑائی(), nodedtypep->چوڑائی_Sized());
    }
}

خالی V3_نمبر::create(ثابت حرف* sourcep) {
    m_ڈیٹا.سیٹ_Logic();
    ثابت حرف* قدر_startp = sourcep;
    برائے (ثابت حرف* cp = sourcep; *cp; ++cp) {
        اگر (*cp == '\'') {
            قدر_startp = cp + 1;
            توڑو;
        }
    }

    منطقی userSized = جھوٹ;
    منطقی unbased = جھوٹ;
    حرف بنیاد = '\0';
    اگر (قدر_startp != sourcep) {  // Has a '
        string widthn;
        ثابت حرف* cp = sourcep;
        برائے (; *cp; ++cp) {
            اگر (*cp == '\'') {
                ++cp;
                توڑو;
            }
            اگر (*cp != '_') widthn += *cp;
        }
        جب (*cp == '_') cp++;
        اگر (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(سچ);
        }
        اگر (*cp) {
            بنیاد = *cp;
            ++cp;
        }
        قدر_startp = cp;

        userSized = widthn.لمبائی_2() != 0;
        اگر (std::atoi(widthn.c_str())) {
            اگر (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_عدد_چوڑائی()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                چوڑائی(v3Global.opt.max_عدد_چوڑائی(), سچ);
            } ورنہ {
                چوڑائی(std::atoi(widthn.c_str()), سچ);
            }
        }
    } ورنہ {
        unbased = سچ;
        بنیاد = 'd';
    }

    برائے (صحیح i = 0; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    اگر (بنیاد == '0') {
        چوڑائی(1, جھوٹ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 0);
        m_ڈیٹا.m_autoExtend = سچ;
    } ورنہ اگر (بنیاد == '1') {
        چوڑائی(1, جھوٹ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 1);
        m_ڈیٹا.m_autoExtend = سچ;
    } ورنہ اگر (std::tolower(بنیاد) == 'z') {
        چوڑائی(1, جھوٹ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 'z');
        m_ڈیٹا.m_autoExtend = سچ;
    } ورنہ اگر (std::tolower(بنیاد) == 'x') {
        چوڑائی(1, جھوٹ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 'x');
        m_ڈیٹا.m_autoExtend = سچ;
    }
    // Otherwise...
    ورنہ اگر (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحیح چوڑائی_Bound = std::max<صحیح>(32, std::لڑی_لمبائی(قدر_startp) * 4);
        چوڑائی(چوڑائی_Bound, جھوٹ);  // Will change width below
        اگر (unbased) isSigned(سچ);  // Also says the spec.
    }

    // Ignore leading blanks
    جب (*قدر_startp == '_' || std::خلا_ہے(*قدر_startp)) ++قدر_startp;
    اگر (!*قدر_startp && !m_ڈیٹا.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    اگر (userSized && m_ڈیٹا.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحیح obit = 0;  // Start at LSB
    صحیح بنیاد_align = 1;
    اگر (std::tolower(بنیاد) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        جب (*قدر_startp == '_' || *قدر_startp == '0') ++قدر_startp;
        // Convert decimal number to hex
        صحیح olen = 0;
        uint32_t val = 0;
        صحیح got_x = 0;
        صحیح got_z = 0;
        صحیح got_01 = 0;
        منطقی warnedTooMany = جھوٹ;
        برائے (ثابت حرف* cp = قدر_startp; *cp; ++cp) {
            انتخاب (std::tolower(*cp)) {
            حالت '0':  // FALLTHRU
            حالت '1':  // FALLTHRU
            حالت '2':  // FALLTHRU
            حالت '3':  // FALLTHRU
            حالت '4':  // FALLTHRU
            حالت '5':  // FALLTHRU
            حالت '6':  // FALLTHRU
            حالت '7':  // FALLTHRU
            حالت '8':  // FALLTHRU
            حالت '9': {
                اگر (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_ڈیٹا.عدد()[0].m_قدر = val;
                } ورنہ {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_نمبر حاصل_ضرب(یہ, چوڑائی() + 4);  // +4 for overflow detection
                    ثابت V3_نمبر ten(یہ, چوڑائی() + 4, 10);
                    ثابت V3_نمبر addend(یہ, چوڑائی(), (*cp - '0'));
                    حاصل_ضرب.opMul(*یہ, ten);
                    op_جوڑو(حاصل_ضرب, addend);
                    اگر (!warnedTooMany && حاصل_ضرب.بٹ_قدر(چوڑائی(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = سچ;
                    }
                }
                ++olen;
                got_01 = 1;
                توڑو;
            }
            حالت 'z':
            حالت '?': {
                got_z = 1;
                اگر (!userSized) چوڑائی(32, جھوٹ);
                سیٹ_All_بٹ_Z();
                توڑو;
            }
            حالت 'x': {
                got_x = 1;
                اگر (!userSized) چوڑائی(32, جھوٹ);
                سیٹ_All_بٹ_X();
                توڑو;
            }
            حالت '_': توڑو;
            طےشدہ: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                توڑو;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = چوڑائی();
        اگر ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << قدر_startp);
        }
    } ورنہ {
        // Convert bin/octal number to hex
        برائے (ثابت حرف* cp = قدر_startp + std::لڑی_لمبائی(قدر_startp) - 1; cp >= قدر_startp;
             cp--) {
            اگر (*cp != '_' && *cp != '0' && obit >= چوڑائی()) {
                warnTooMany(sourcep);
                توڑو;
            }
            انتخاب (std::tolower(بنیاد)) {
            حالت 'b': {
                بنیاد_align = 1;
                انتخاب (std::tolower(*cp)) {
                حالت '0': سیٹ_ثنائی_ہندسہ(obit++, 0); توڑو;
                حالت '1': سیٹ_ثنائی_ہندسہ(obit++, 1); توڑو;
                حالت 'z':
                حالت '?': سیٹ_ثنائی_ہندسہ(obit++, 'z'); توڑو;
                حالت 'x': سیٹ_ثنائی_ہندسہ(obit++, 'x'); توڑو;
                حالت '_': توڑو;
                طےشدہ: v3error("Illegal character in binary constant: " << *cp);
                }
                توڑو;
            }

            حالت 'o':
            حالت 'c': {
                بنیاد_align = 3;
                انتخاب (std::tolower(*cp)) {  // clang-format off
                حالت '0': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 0);  توڑو;
                حالت '1': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 0);  توڑو;
                حالت '2': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 0);  توڑو;
                حالت '3': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 0);  توڑو;
                حالت '4': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 1);  توڑو;
                حالت '5': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 1);  توڑو;
                حالت '6': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 1);  توڑو;
                حالت '7': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 1);  توڑو;
                حالت 'z':
                حالت '?': سیٹ_ثنائی_ہندسہ(obit++, 'z'); سیٹ_ثنائی_ہندسہ(obit++, 'z'); سیٹ_ثنائی_ہندسہ(obit++, 'z'); توڑو;
                حالت 'x': سیٹ_ثنائی_ہندسہ(obit++, 'x'); سیٹ_ثنائی_ہندسہ(obit++, 'x'); سیٹ_ثنائی_ہندسہ(obit++, 'x'); توڑو;
                حالت '_': توڑو;
                // clang-format on
                طےشدہ: v3error("Illegal character in octal constant");
                }
                توڑو;
            }

            حالت 'h':  // FALLTHRU
            حالت 'x': {
                بنیاد_align = 4;
                انتخاب (std::tolower(*cp)) {  // clang-format off
                حالت '0': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '1': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '2': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '3': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '4': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '5': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '6': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '7': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); توڑو;
                حالت '8': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت '9': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'a': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'b': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'c': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'd': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'e': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'f': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); توڑو;
                حالت 'z':
                حالت '?': سیٹ_ثنائی_ہندسہ(obit++,'z'); سیٹ_ثنائی_ہندسہ(obit++,'z'); سیٹ_ثنائی_ہندسہ(obit++,'z'); سیٹ_ثنائی_ہندسہ(obit++,'z'); توڑو;
                حالت 'x': سیٹ_ثنائی_ہندسہ(obit++,'x'); سیٹ_ثنائی_ہندسہ(obit++,'x'); سیٹ_ثنائی_ہندسہ(obit++,'x'); سیٹ_ثنائی_ہندسہ(obit++,'x'); توڑو;
                    // clang-format on
                حالت '_': توڑو;
                طےشدہ:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    توڑو;
                }  // LCOV_EXCL_STOP
                توڑو;
            }
            طےشدہ:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << بنیاد);
                توڑو;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    اگر (!userSized && !m_ڈیٹا.m_autoExtend) {
        چوڑائی(std::max(32, بنیاد_align * ((چوڑائی_مقصد_Fit() + بنیاد_align - 1) / بنیاد_align)), جھوٹ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    جب (obit <= چوڑائی() && obit && ثنائی_ہندسہ_Is_XZ(obit - 1)) {
        سیٹ_ثنائی_ہندسہ(obit, ثنائی_ہندسہ_Is(obit - 1));
        ++obit;
    }
    opCleanThis(سچ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

خالی V3_نمبر::warnTooMany(ثابت string& قدر) {
    v3warn(WIDTHDIGITS, "Too many digits for " << چوڑائی() << " bit number: '" << قدر << "'");
}

خالی V3_نمبر::nodep(Ast_گرہ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    اگر (!nodep) واپس;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

منطقی V3_نمبر::epsilonEqual(دوہرا a, دوہرا b) {
    واپس std::fabs(a - b)
           <= (std::numeric_limits<دوہرا>::epsilon() * std::max(1.0, std::max(a, b)));
}

منطقی V3_نمبر::epsilonIntegral(دوہرا a) {
    ثابت دوہرا dint = static_cast<دوہرا>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    واپس epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

صحیح V3_نمبر::log2b(uint32_t عدد) {
    // See also opCLog2
    برائے (صحیح ثنائی_ہندسہ_2 = 31; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2--) {
        اگر (عدد & (1ULL << ثنائی_ہندسہ_2)) واپس ثنائی_ہندسہ_2;
    }
    واپس 0;
}

صحیح V3_نمبر::log2bQuad(uint64_t عدد) {
    اگر (عدد >> 32ULL) واپس 32 + log2b(عدد >> 32ULL);
    واپس log2b(عدد);
}

//======================================================================
// Setters

V3_نمبر& V3_نمبر::سیٹ_Zero() {
    برائے (صحیح i = 0; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, 0};
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_Quad(uint64_t قدر) {
    سیٹ_Zero();
    m_ڈیٹا.عدد()[0].m_قدر = قدر & 0xffffffffULL;
    اگر (چوڑائی() > 32) m_ڈیٹا.عدد()[1].m_قدر = (قدر >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_Long(uint32_t قدر) {
    سیٹ_Zero();
    m_ڈیٹا.عدد()[0].m_قدر = قدر;
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_Long_S(int32_t قدر) {
    سیٹ_Zero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = قدر;
    (خالی)u.s;
    m_ڈیٹا.عدد()[0].m_قدر = u.u;
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_Double(دوہرا قدر) {
    UASSERT(چوڑائی() == 64, "Real operation on wrong sized number");
    m_ڈیٹا.سیٹ_Double();
    اتحاد {
        دوہرا d;
        uint32_t u[2];
    } u;
    u.d = قدر;
    (خالی)u.d;
    برائے (صحیح i = 2; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, 0};
    m_ڈیٹا.عدد()[0].m_قدر = u.u[0];
    m_ڈیٹا.عدد()[1].m_قدر = u.u[1];
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_Single_بٹ(حرف قدر) {
    برائے (صحیح i = 1 /*بالائی*/; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, 0};
    m_ڈیٹا.عدد()[0] = {(قدر == '1' || قدر == 'x' || قدر == 1 || قدر == 3),
                       (قدر == 'z' || قدر == 'x' || قدر == 2 || قدر == 3)};
    واپس *یہ;
}

V3_نمبر& V3_نمبر::سیٹ_All_Bits0() {
    سیٹ_Zero();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_All_Bits1() {
    برائے (صحیح i = 0; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {~0U, 0};
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_All_بٹ_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    برائے (صحیح i = 0; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {~0U, ~0U};
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_All_بٹ_Z() {
    برائے (صحیح i = 0; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, ~0U};
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::سیٹ_All_بٹ_XRemoved() {
    اگر (!v3Global.constRemoveXs()) {
        واپس سیٹ_All_بٹ_X();
    } ورنہ {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        اگر (v3Global.opt.x_تفویض() == "1") {
            واپس سیٹ_All_Bits1();
        } ورنہ {
            واپس سیٹ_All_Bits0();
        }
    }
}
V3_نمبر& V3_نمبر::سیٹ_Value1() {
    m_ڈیٹا.عدد()[0] = {1, 0};
    برائے (صحیح i = 1; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, 0};
    واپس *یہ;
}

خالی V3_نمبر::سیٹ_ثنائی_ہندسہ_X0(صحیح ثنائی_ہندسہ_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_نمبر& V3_نمبر::سیٹ_نقاب(صحیح nbits, صحیح lsb) {
    سیٹ_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_بٹ(lsb, std::max(0, std::min(nbits, چوڑائی() - lsb)), 1);
    واپس *یہ;
}

//======================================================================
// ACCESSORS - as strings

string V3_نمبر::ascii(منطقی prefixed, منطقی cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    اگر (is1_قدم()) واپس "1step";
    اگر (isNull()) واپس "null";
    std::ostringstream برآمد;
    اگر (isDouble()) {
        برآمد.precision(17);
        اگر (VL_UNCOVERABLE(چوڑائی() != 64)) {
            برآمد << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } ورنہ {
            برآمد << مقصد_Double();
            اگر (مقصد_Double() == floor(مقصد_Double())) برآمد << ".0";
        }
        واپس برآمد.str();
    } ورنہ اگر (isString()) {
        واپس '"' + مقصد_String() + '"';
    } ورنہ {
        اگر (VL_UNCOVERABLE((m_ڈیٹا.عدد()[الفاظ() - 1].m_قدر | m_ڈیٹا.عدد()[الفاظ() - 1].m_قدر_X)
                           & ~hi_لفظ_نقاب())) {
            برآمد << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    اگر (prefixed) {
        اگر (sized()) {
            برآمد << چوڑائی() << "'";
        } ورنہ اگر (autoExtend() && چوڑائی() == 1) {
            برآمد << "'";
            اگر (ثنائی_ہندسہ_Is0(0)) {
                برآمد << '0';
            } ورنہ اگر (ثنائی_ہندسہ_Is1(0)) {
                برآمد << '1';
            } ورنہ اگر (ثنائی_ہندسہ_Is_Z(0)) {
                برآمد << 'z';
            } ورنہ {
                برآمد << 'x';
            }
            واپس برآمد.str();
        } ورنہ {
            اگر (cleanVerilog) {
                برآمد << "'";
            } ورنہ {
                برآمد << "?" << چوڑائی() << "?";
            }
        }
        اگر (isSigned()) برآمد << "s";
    }

    ثابت منطقی ثنائی = (isFourState()
#اگر_تعریف_شدہ V3_نمبر_ASCII_ثنائی
                         // cppcheck-suppress konwnConditionTrueFalse
                         || سچ
#اگر_ختم
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    اگر (ثنائی) {
        برآمد << "b";
        برآمد << displayed("%0b");
    } ورنہ {
        اگر (prefixed) برآمد << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        برآمد << displayed("%0h");
    }
    اگر (isNull()) {
        اگر (VL_UNCOVERABLE(!isEqZero())) {
            برآمد << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } ورنہ {
            برآمد << " [null]";
        }
    }
    واپس برآمد.str();
}

منطقی V3_نمبر::displayedFmtHasArg(حرف format, منطقی is_جائزہ) {
    // Is this a format letter that takes an argument in runtime?
    (خالی)is_جائزہ;
    انتخاب (std::tolower(format)) {
    حالت 'b': واپس سچ;  //ثنائی
    حالت 'c': واپس سچ;  // Character
    حالت 'd': واپس سچ;  // Decimal
    حالت 'e': واپس سچ;  // Floating
    حالت 'f': واپس سچ;  // Floating
    حالت 'g': واپس سچ;  // Floating
    حالت 'h': واپس سچ;  // Hex
    حالت 'o': واپس سچ;  // Octal
    حالت 'p': واپس سچ;  //نمونہ
    حالت 's': واپس سچ;  // String
    حالت 't': واپس سچ;  //وقت
    حالت 'u': واپس سچ;  // Packed 2-state
    حالت 'v': واپس سچ;  // Strength
    حالت 'x': واپس سچ;  // Hex
    حالت 'z': واپس سچ;  // Packed 4-state
    حالت '?': واپس سچ;  // Internal: Compute format in V3Width based on data type
    طےشدہ: واپس جھوٹ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_نمبر::displayPad(سائز_t fmtsize, حرف pad, منطقی بائیں, ثابت string& اندر) VL_خالص {
    string padding;
    اگر (اندر.لمبائی_2() < fmtsize) padding = string(fmtsize - اندر.لمبائی_2(), pad);
    واپس بائیں ? (اندر + padding) : (padding + اندر);
}

string V3_نمبر::displayedEnumName(ثابت Ast_Enum_شے* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    واپس VString::isIdentifier(name) && !std::ہندسہ_ہے(static_cast<غیرعلامتی حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_نمبر::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3_نمبر قدر{یہ, dtypep->چوڑائی()};
    قدر.op_تفویض(*یہ);
    برائے (ثابت Ast_Enum_شے* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_شے)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        اگر (!constp->عدد().isAnyXZ() && قدر.isCaseEq(constp->عدد()))
            واپس displayedEnumName(itemp);
    }
    واپس "";
}

string V3_نمبر::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
    V3_نمبر قدر{یہ, dtypep->چوڑائی()};
    قدر.op_تفویض(*یہ);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.عقب());
    اگر (fmt != 'p' && fmt != 's') واپس قدر.displayed(argp, vformat, numericAttr);
    string name = قدر.displayedEnumName(dtypep);
    اگر (name.خالی_خانے()) name = قدر.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_نمبر name_عدد{یہ};
    name_عدد.سیٹ_String(name);
    واپس name_عدد.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_نمبر::displayed(ثابت Ast_گرہ* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    واپس displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_نمبر::displayed(File_سطر* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    خودکار pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *یہ);
    ++pos;
    منطقی بائیں = جھوٹ;
    اگر (pos[0] == '-') {
        بائیں = سچ;
        ++pos;
    }
    string fmtsize;
    برائے (; pos != vformat.cend() && (std::ہندسہ_ہے(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    حرف fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    اگر (formatAttr.isComplex()) {
        اگر (fmt != 'p') fmt = 's';  // override
    } ورنہ اگر (formatAttr.isString()) {
        اگر (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_نمبر strwide{fl, static_cast<صحیح>(مقصد_String().لمبائی_2()) * 8, 0};
            strwide.opNToI(*یہ);
            واپس strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        اگر (fmt != 'p') fmt = 's';  // override
    } ورنہ اگر (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        اگر (fmt == 'p') {
            اگر (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } ورنہ {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } ورنہ اگر (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        اگر (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_نمبر nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*یہ);
            واپس nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    انتخاب (fmt) {
    حالت 'c': {
        اگر (چوڑائی() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت غیرعلامتی صحیح v = بٹ_قدر(0, 8);
        str.ڈالو_عقب(static_cast<حرف>(v));
        واپس str;
    }
    حالت 'e':  // FALLTHRU
    حالت 'f':  // FALLTHRU
    حالت 'g': {
        ثابت دوہرا n = formatAttr.isDouble()   ? مقصد_Double()
                         : formatAttr.isSigned() ? مقصد_SQuad()
                                                 : مقصد_UQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_سائز];
        (خالی)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_سائز, vformat.c_str(), n);
        واپس tmp;
    }
    حالت 's': {
        اگر (formatAttr.isString() || formatAttr.isComplex()) {
            str = مقصد_String();
        } ورنہ {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحیح ثنائی_ہندسہ_2 = چوڑائی() - 1;
            منطقی آغاز = سچ;
            جب ((ثنائی_ہندسہ_2 % 8) != 7) ++ثنائی_ہندسہ_2;
            برائے (; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2 -= 8) {
                ثابت صحیح v = بٹ_قدر(ثنائی_ہندسہ_2 - 7, 8);
                اگر (!آغاز || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    آغاز = جھوٹ;  // Drop leading 0s
                } ورنہ {
                    اگر (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت سائز_t fmtsizen = static_cast<سائز_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', بائیں, str);
        واپس str;
    }
    حالت 'p': {  //نمونہ
        // 'p' with NUMBER was earlier converted to 'd'
        اگر (formatAttr.isDouble()) {
            ثابت دوہرا n = formatAttr.isDouble()   ? مقصد_Double()
                             : formatAttr.isSigned() ? مقصد_SQuad()
                                                     : مقصد_UQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_سائز];
            (خالی)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_سائز, "%g", n);
            واپس tmp;
        }
        اگر (formatAttr.isString())
            واپس '"' + V3_برآمد_Formatter::quoteNameControls(مقصد_String()) + '"';
        اگر (formatAttr.isComplex()) واپس مقصد_String();
        واپس "%p";
    }
    حالت 'b':  // FALLTHRU
    حالت 'o':  // FALLTHRU
    حالت 'h':  // FALLTHRU
    حالت 'x': {
        صحیح ثنائی_ہندسہ_2 = چوڑائی() - 1;
        اگر (بائیں || !fmtsize.خالی_خانے()) {
            جب (ثنائی_ہندسہ_2 && ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) --ثنائی_ہندسہ_2;
        }
        انتخاب (fmt) {
        حالت 'b': {
            برائے (; ثنائی_ہندسہ_2 >= 0; --ثنائی_ہندسہ_2) {
                اگر (ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
                    str += '0';
                } ورنہ اگر (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
                    str += '1';
                } ورنہ اگر (ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) {
                    str += 'z';
                } ورنہ {
                    str += 'x';
                }
            }
            توڑو;
        }
        حالت 'o': {
            جب ((ثنائی_ہندسہ_2 % 3) != 2) ++ثنائی_ہندسہ_2;
            برائے (; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2 -= 3) {
                ثابت صحیح عدد_X = گنتی_X(ثنائی_ہندسہ_2 - 2, 3);
                ثابت صحیح عدد_Z = گنتی_Z(ثنائی_ہندسہ_2 - 2, 3);
                اگر (عدد_X == 3 || عدد_X == چوڑائی() - (ثنائی_ہندسہ_2 - 2)) {
                    str += 'x';
                    جاری;
                }
                اگر (عدد_Z == 3 || عدد_Z == چوڑائی() - (ثنائی_ہندسہ_2 - 2)) {
                    str += 'z';
                    جاری;
                }
                اگر (عدد_X > 0) {
                    str += 'X';
                    جاری;
                }
                اگر (عدد_Z > 0) {
                    str += 'Z';
                    جاری;
                }
                ثابت صحیح v = بٹ_قدر(ثنائی_ہندسہ_2 - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            توڑو;
        }
        طےشدہ: {  // h/x
            جب ((ثنائی_ہندسہ_2 % 4) != 3) ++ثنائی_ہندسہ_2;
            برائے (; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2 -= 4) {
                ثابت صحیح عدد_X = گنتی_X(ثنائی_ہندسہ_2 - 3, 4);
                ثابت صحیح عدد_Z = گنتی_Z(ثنائی_ہندسہ_2 - 3, 4);
                اگر (عدد_X == 4 || عدد_X == چوڑائی() - (ثنائی_ہندسہ_2 - 3)) {
                    str += 'x';
                    جاری;
                }
                اگر (عدد_Z == 4 || عدد_Z == چوڑائی() - (ثنائی_ہندسہ_2 - 3)) {
                    str += 'z';
                    جاری;
                }
                اگر (عدد_X > 0) {
                    str += 'X';
                    جاری;
                }
                اگر (عدد_Z > 0) {
                    str += 'Z';
                    جاری;
                }
                ثابت صحیح v = بٹ_قدر(ثنائی_ہندسہ_2 - 3, 4);
                اگر (v >= 10) {
                    str += static_cast<حرف>('a' + v - 10);
                } ورنہ {
                    str += static_cast<حرف>('0' + v);
                }
            }
            توڑو;
        }
        }  // switch
        ثابت سائز_t fmtsizen = static_cast<سائز_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (بائیں ? ' ' : '0'), بائیں, str);
        واپس str;
    }  // case b/d/x/o
    حالت 't':  //وقت
    حالت 'd': {  // Unsigned decimal
        ثابت منطقی issigned = formatAttr.isSigned();
        اگر (fmtsize == "" && !بائیں) {
            ثابت دوہرا mantissabits = چوڑائی() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            دوہرا dchars = mantissabits / 3.321928094887362 + 1.0;
            اگر (issigned) ++dchars;  // space for sign
            fmtsize = cvt_مقصد_Str(static_cast<صحیح>(dchars));
        }
        منطقی hasXZ = جھوٹ;
        اگر (isAllX()) {
            str = "x";
            hasXZ = سچ;
        } ورنہ اگر (isAllZ()) {
            str = "z";
            hasXZ = سچ;
        } ورنہ اگر (isAnyX()) {
            str = "X";
            hasXZ = سچ;
        } ورنہ اگر (isAnyZ()) {
            str = "Z";
            hasXZ = سچ;
        }
        اگر (!hasXZ) {
            اگر (issigned) {
                اگر (چوڑائی() > 64) {
                    str = مقصد_Decimal_S();
                } ورنہ {
                    str = cvt_مقصد_Str(مقصد_SQuad());
                }
            } ورنہ {
                اگر (چوڑائی() > 64) {
                    str = مقصد_Decimal_U();
                } ورنہ {
                    str = cvt_مقصد_Str(مقصد_UQuad());
                }
            }
        }
        ثابت منطقی zeropad = fmtsize.لمبائی_2() > 0 && fmtsize[0] == '0' && !بائیں;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت سائز_t fmtsizen = static_cast<سائز_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), بائیں, str);
        واپس str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالت 'u': {  // Packed 2-state
        برائے (صحیح i = 0; i < الفاظ(); ++i) {
            ثابت uint32_t v = m_ڈیٹا.عدد()[i].m_قدر;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        واپس str;
    }
    حالت 'z': {  // Packed 4-state
        برائے (صحیح i = 0; i < الفاظ(); ++i) {
            ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
            str += static_cast<حرف>((v.m_قدر >> 0) & 0xff);
            str += static_cast<حرف>((v.m_قدر >> 8) & 0xff);
            str += static_cast<حرف>((v.m_قدر >> 16) & 0xff);
            str += static_cast<حرف>((v.m_قدر >> 24) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 0) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 8) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 16) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 24) & 0xff);
        }
        واپس str;
    }
    حالت 'v': {  // Strength
        صحیح ثنائی_ہندسہ_2 = چوڑائی() - 1;
        برائے (; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
            اگر (ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            ورنہ اگر (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
                str += "St1 ";
            } ورنہ اگر (ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) {
                str += "StZ ";
            } ورنہ {
                str += "StX";
            }
        }
        واپس str;
    }
    طےشدہ: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_نمبر::emitC() ثابت VL_MT_STABLE {
    constexpr سائز_t bufsize = 1000;
    حرف sbuf[bufsize];
    string نتیجہ;
    اگر (isNull()) {
        واپس "VlNull{}";
    } ورنہ اگر (isDouble()) {
        ثابت دوہرا dnum = مقصد_Double();
        اگر (std::isinf(dnum)) {
            اگر (std::signbit(dnum)) نتیجہ += '-';
            نتیجہ += "std::numeric_limits<double>::infinity()";
        } ورنہ اگر (std::isnan(dnum)) {
            اگر (std::signbit(dnum)) نتیجہ += '-';
            نتیجہ += "std::numeric_limits<double>::quiet_NaN()";
        } ورنہ {
            ثابت حرف* ثابت fmt = (static_cast<صحیح>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (خالی)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            واپس sbuf;
        }
    } ورنہ اگر (isString()) {
        ثابت string strg = مقصد_String();
        ثابت string quoted
            = V3_برآمد_Formatter::quoteNameControls(strg, V3_برآمد_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        واپس '"' + quoted + "\"s";
    } ورنہ اگر (الفاظ() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        نتیجہ = "::VlWide<" + std::مقصد_string(الفاظ()) + ">{{";
        اگر (الفاظ() > 4) نتیجہ += '\n';
        برائے (صحیح n = 0; n < الفاظ(); ++n) {
            اگر (n) نتیجہ += ((n % 4) ? ", " : ",\n");
            (خالی)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_لفظ(n));
            نتیجہ += sbuf;
        }
        اگر (الفاظ() > 4) نتیجہ += '\n';
        نتیجہ += "}}";
    } ورنہ اگر (الفاظ() == 2) {  // Quad
        ثابت uint64_t qnum = static_cast<uint64_t>(مقصد_UQuad());
        ثابت حرف* ثابت fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (خالی)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        واپس sbuf;
    } ورنہ {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ثابت uint32_t unum = مقصد_UInt();
        ثابت حرف* ثابت fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (چوڑائی() > 16) ? ("0x%08" PRIx32 "U")
                                : (چوڑائی() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (خالی)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        واپس sbuf;
    }
    واپس نتیجہ;
}

string V3_نمبر::مقصد_Decimal_S() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    اگر (isNegative()) {
        V3_نمبر lhsNoSign = *یہ;
        lhsNoSign.opNegate(*یہ);
        واپس "-"s + lhsNoSign.مقصد_Decimal_U();
    } ورنہ {
        واپس مقصد_Decimal_U();
    }
}

string V3_نمبر::مقصد_Decimal_U() ثابت VL_MT_STABLE {
    ثابت صحیح maxdecwidth = (چوڑائی() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_نمبر bcd(یہ, maxdecwidth + 4);
    V3_نمبر tmp(یہ, maxdecwidth + 4);
    V3_نمبر tmp2(یہ, maxdecwidth + 4);

    صحیح from_ثنائی_ہندسہ = چوڑائی() - 1;
    // Skip all leading zeros
    برائے (; from_ثنائی_ہندسہ >= 0 && ثنائی_ہندسہ_Is0(from_ثنائی_ہندسہ); --from_ثنائی_ہندسہ) {}
    // Double-dabble algorithm
    برائے (; from_ثنائی_ہندسہ >= 0; --from_ثنائی_ہندسہ) {
        // Any digits >= 5 need an add 3 (via tmp)
        برائے (صحیح nibble_ثنائی_ہندسہ = 0; nibble_ثنائی_ہندسہ < maxdecwidth; nibble_ثنائی_ہندسہ += 4) {
            اگر (bcd.بٹ_قدر(nibble_ثنائی_ہندسہ, 4) >= 5) {
                tmp2.سیٹ_All_Bits0();
                tmp2.سیٹ_ثنائی_ہندسہ(nibble_ثنائی_ہندسہ, 1);  // Add 3, decomposed as two bits
                tmp2.سیٹ_ثنائی_ہندسہ(nibble_ثنائی_ہندسہ + 1, 1);
                tmp.op_تفویض(bcd);
                bcd.op_جوڑو(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_تفویض(bcd);
        bcd.op_سرکاؤ_L(tmp, V3_نمبر(یہ, 32, 1));
        // bcd[0] = this[from_bit]
        اگر (ثنائی_ہندسہ_Is1(from_ثنائی_ہندسہ)) bcd.سیٹ_ثنائی_ہندسہ(0, 1);
    }

    string output;
    صحیح lsb = (maxdecwidth - 1) & ~3;
    برائے (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        اگر (bcd.بٹ_قدر(lsb, 4)) توڑو;
    }
    برائے (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.بٹ_قدر(lsb, 4));  // 0..9
    }
    واپس output;
}

string V3_نمبر::مقصد_Real_String() ثابت VL_MT_STABLE {
    ثابت دوہرا قدر = مقصد_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream برآمد;
    برآمد.precision(15);
    برآمد << قدر;
    string نتیجہ = برآمد.str();
    اگر (std::strtod(نتیجہ.c_str(), خالی_اشاریہ) != قدر) {
        برآمد.str("");
        برآمد.precision(17);
        برآمد << قدر;
        نتیجہ = برآمد.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    اگر (نتیجہ.ڈھونڈو_پہلا_کا(".en") == string::npos) نتیجہ += ".0";
    واپس نتیجہ;
}

//======================================================================
// ACCESSORS - as numbers

منطقی V3_نمبر::سماتا_اندر_UInt() ثابت VL_MT_محفوظ {
    اگر (isFourState()) واپس جھوٹ;
    // We allow wide numbers that represent values <= 32 bits
    برائے (صحیح i = 1; i < الفاظ(); ++i) {
        اگر (m_ڈیٹا.عدد()[i].m_قدر) واپس جھوٹ;
    }
    واپس سچ;
}
uint32_t V3_نمبر::مقصد_UInt() ثابت VL_MT_محفوظ {
    UASSERT(!isFourState(), "toUInt with 4-state " << *یہ);
    // We allow wide numbers that represent values <= 32 bits
    اگر (VL_UNCOVERABLE(!سماتا_اندر_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *یہ);
    }
    واپس m_ڈیٹا.عدد()[0].m_قدر;
}

دوہرا V3_نمبر::مقصد_Double() ثابت VL_MT_محفوظ {
    UASSERT(isDouble() && چوڑائی() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        دوہرا d;
        uint32_t u[2];
    } u;
    u.u[0] = m_ڈیٹا.عدد()[0].m_قدر;
    u.u[1] = m_ڈیٹا.عدد()[1].m_قدر;
    واپس u.d;
}

int32_t V3_نمبر::مقصد_SInt() ثابت VL_MT_محفوظ {
    اگر (isSigned()) {
        ثابت uint32_t v = مقصد_UInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (چوڑائی() - 1))));
        ثابت uint32_t extended = v | signExtend;
        واپس static_cast<int32_t>(extended);
    } ورنہ {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        واپس static_cast<int32_t>(مقصد_UInt());
    }
}

uint64_t V3_نمبر::مقصد_UQuad() ثابت VL_MT_محفوظ {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *یہ);
    // We allow wide numbers that represent values <= 64 bits
    اگر (isDouble()) واپس static_cast<uint64_t>(مقصد_Double());
    برائے (صحیح i = 2; i < الفاظ(); ++i) {
        اگر (m_ڈیٹا.عدد()[i].m_قدر) {
            v3error("Value too wide for 64-bits expected in this context " << *یہ);
            توڑو;
        }
    }
    اگر (چوڑائی() <= 32) واپس (static_cast<uint64_t>(مقصد_UInt()));
    واپس ((static_cast<uint64_t>(m_ڈیٹا.عدد()[1].m_قدر) << 32ULL)
            | (static_cast<uint64_t>(m_ڈیٹا.عدد()[0].m_قدر)));
}

int64_t V3_نمبر::مقصد_SQuad() ثابت {
    اگر (isDouble()) واپس static_cast<int64_t>(مقصد_Double());
    ثابت uint64_t v = مقصد_UQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (چوڑائی() - 1))));
    ثابت uint64_t extended = v | signExtend;
    واپس static_cast<int64_t>(extended);
}

string V3_نمبر::مقصد_String() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *یہ);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    اگر (isString()) واپس m_ڈیٹا.str();
    صحیح ثنائی_ہندسہ_2 = چوڑائی() - 1;
    جب ((ثنائی_ہندسہ_2 % 8) != 7) ++ثنائی_ہندسہ_2;
    std::string str;
    برائے (; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2 -= 8) {
        ثابت صحیح v = بٹ_قدر(ثنائی_ہندسہ_2 - 7, 8);
        اگر (v) str += static_cast<حرف>(v);
    }
    واپس str;
}

uint32_t V3_نمبر::edata_لفظ(صحیح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *یہ);
    واپس m_ڈیٹا.عدد()[eword].m_قدر;
}

uint8_t V3_نمبر::ڈیٹا_Byte(صحیح byte) ثابت {
    واپس (edata_لفظ(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقی V3_نمبر::isAllZ() ثابت VL_MT_محفوظ {
    اگر (isDouble() || isString()) واپس جھوٹ;
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        ثابت uint32_t نقاب = i == الفاظ() - 1 ? hi_لفظ_نقاب() : ~0U;
        اگر (((~v.m_قدر & v.m_قدر_X) & نقاب) != نقاب) واپس جھوٹ;
    }
    واپس سچ;
}
منطقی V3_نمبر::isAllX() ثابت VL_MT_محفوظ {
    اگر (isDouble() || isString()) واپس جھوٹ;
    uint32_t نقاب = hi_لفظ_نقاب();
    برائے (صحیح i = الفاظ() - 1; i >= 0; --i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        اگر ((v.m_قدر & v.m_قدر_X) ^ نقاب) واپس جھوٹ;
        نقاب = ~0U;
    }
    واپس سچ;
}
منطقی V3_نمبر::isEqZero() ثابت VL_MT_محفوظ {
    اگر (isString()) واپس m_ڈیٹا.str().خالی_خانے();
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        اگر (v.m_قدر || v.m_قدر_X) واپس جھوٹ;
    }
    واپس سچ;
}
منطقی V3_نمبر::isNeqZero() ثابت {
    اگر (isString()) واپس !m_ڈیٹا.str().خالی_خانے();
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        اگر (v.m_قدر & ~v.m_قدر_X) واپس سچ;
    }
    واپس جھوٹ;
}
منطقی V3_نمبر::is_بٹ_Zero(صحیح msb, صحیح lsb) ثابت {
    برائے (صحیح i = lsb; i <= msb; ++i) {
        اگر (VL_UNLIKELY(!ثنائی_ہندسہ_Is0(i))) واپس جھوٹ;
    }
    واپس سچ;
}
منطقی V3_نمبر::isEqOne() ثابت {
    اگر (m_ڈیٹا.عدد()[0].m_قدر != 1 || m_ڈیٹا.عدد()[0].m_قدر_X) واپس جھوٹ;
    برائے (صحیح i = 1; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        اگر (v.m_قدر || v.m_قدر_X) واپس جھوٹ;
    }
    واپس سچ;
}
منطقی V3_نمبر::isEqAllOnes(صحیح optwidth) ثابت {
    // Correct number of zero bits/width matters
    اگر (!optwidth) optwidth = چوڑائی();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < optwidth; ++ثنائی_ہندسہ_2) {
        اگر (!ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) واپس جھوٹ;
    }
    واپس سچ;
}
منطقی V3_نمبر::isFourState() ثابت VL_MT_محفوظ {
    اگر (isDouble() || isString()) واپس جھوٹ;
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        اگر (m_ڈیٹا.عدد()[i].m_قدر_X) واپس سچ;
    }
    واپس جھوٹ;
}
منطقی V3_نمبر::isAnyX() ثابت VL_MT_محفوظ {
    اگر (isDouble() || isString()) واپس جھوٹ;
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        اگر (v.m_قدر & v.m_قدر_X) واپس سچ;
    }
    واپس جھوٹ;
}
منطقی V3_نمبر::isAnyXZ() ثابت {
    اگر (isDouble() || isString()) واپس جھوٹ;
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        اگر (m_ڈیٹا.عدد()[i].m_قدر_X) واپس سچ;
    }
    واپس جھوٹ;
}
منطقی V3_نمبر::isAnyZ() ثابت VL_MT_محفوظ {
    اگر (isDouble() || isString()) واپس جھوٹ;
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
        اگر (~v.m_قدر & v.m_قدر_X) واپس سچ;
    }
    واپس جھوٹ;
}
منطقی V3_نمبر::isLtXZ(ثابت V3_نمبر& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    اگر (isString()) واپس مقصد_String() < rhs.مقصد_String();
    اگر (isDouble()) واپس مقصد_Double() < rhs.مقصد_Double();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) واپس سچ;
        اگر (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) واپس جھوٹ;
        اگر (ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) واپس سچ;
        اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) واپس جھوٹ;
    }
    واپس جھوٹ;
}
صحیح V3_نمبر::گنتی_X(صحیح lsb, صحیح nbits) ثابت VL_MT_محفوظ {
    صحیح گنتی = 0;
    برائے (صحیح bitn = 0; bitn < nbits; ++bitn) {
        اگر (lsb + bitn >= چوڑائی()) واپس گنتی;
        اگر (ثنائی_ہندسہ_Is_X(lsb + bitn)) ++گنتی;
    }
    واپس گنتی;
}
صحیح V3_نمبر::گنتی_Z(صحیح lsb, صحیح nbits) ثابت VL_MT_محفوظ {
    صحیح گنتی = 0;
    برائے (صحیح bitn = 0; bitn < nbits; ++bitn) {
        اگر (lsb + bitn >= چوڑائی()) واپس گنتی;
        اگر (ثنائی_ہندسہ_Is_Z(lsb + bitn)) ++گنتی;
    }
    واپس گنتی;
}

صحیح V3_نمبر::چوڑائی_مقصد_Fit() ثابت {
    برائے (صحیح ثنائی_ہندسہ_2 = چوڑائی() - 1; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2--) {
        اگر (!ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) واپس ثنائی_ہندسہ_2 + 1;
    }
    واپس 1;  // one bit even if number is == 0
}

uint32_t V3_نمبر::گنتی_بٹ(ثابت V3_نمبر& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحیح n = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        انتخاب (ctrl.ثنائی_ہندسہ_Is(0)) {
        حالت '0':
            اگر (ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) ++n;
            توڑو;
        حالت '1':
            اگر (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) ++n;
            توڑو;
        حالت 'x':
            اگر (ثنائی_ہندسہ_Is_X(ثنائی_ہندسہ_2)) ++n;
            توڑو;
        حالت 'z':
            اگر (ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) ++n;
            توڑو;
        }
    }
    واپس n;
}

uint32_t V3_نمبر::گنتی_بٹ(ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2,
                             ثابت V3_نمبر& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحیح n = گنتی_بٹ(ctrl1);
    اگر (ctrl2.ثنائی_ہندسہ_Is(0) != ctrl1.ثنائی_ہندسہ_Is(0)) n += گنتی_بٹ(ctrl2);
    اگر ((ctrl3.ثنائی_ہندسہ_Is(0) != ctrl1.ثنائی_ہندسہ_Is(0)) && (ctrl3.ثنائی_ہندسہ_Is(0) != ctrl2.ثنائی_ہندسہ_Is(0))) {
        n += گنتی_بٹ(ctrl3);
    }
    واپس n;
}

uint32_t V3_نمبر::گنتی_Ones() ثابت {
    صحیح n = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) ++n;
    }
    واپس n;
}

uint32_t V3_نمبر::most_سیٹ_ثنائی_ہندسہ_P1() ثابت {
    برائے (صحیح ثنائی_ہندسہ_2 = چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        اگر (!ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) واپس ثنائی_ہندسہ_2 + 1;
    }
    واپس 0;
}

uint32_t V3_نمبر::کم_سے_کم_سیٹ_ثنائی_ہندسہ_P1() ثابت {
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (!ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) واپس ثنائی_ہندسہ_2 + 1;
    }
    واپس 0;
}

//======================================================================

V3_نمبر& V3_نمبر::op_بٹ_Non_XZ(ثابت V3_نمبر& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) || lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_بٹ_One(ثابت V3_نمبر& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_بٹ_XZ(ثابت V3_نمبر& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_بٹ_Z(ثابت V3_نمبر& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    واپس *یہ;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_نمبر& V3_نمبر::opRedOr(ثابت V3_نمبر& lhs) {
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            واپس سیٹ_Single_بٹ(1);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
        } ورنہ {
            outc = 'x';
        }
    }
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opRedAnd(ثابت V3_نمبر& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            واپس سیٹ_Single_بٹ(0);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
        } ورنہ {
            outc = 'x';
        }
    }
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opRedXor(ثابت V3_نمبر& lhs) {
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            اگر (outc == 1) {
                outc = 0;
            } ورنہ اگر (outc == 0) {
                outc = 1;
            }
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
        } ورنہ {
            outc = 'x';
        }
    }
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::op_گنتی_بٹ(ثابت V3_نمبر& اظہاریہ, ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2,
                                ثابت V3_نمبر& ctrl3) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS4(اظہاریہ, ctrl1, ctrl2, ctrl3);
    عدد_ASSERT_LOGIC_ARGS4(اظہاریہ, ctrl1, ctrl2, ctrl3);
    سیٹ_Zero();
    m_ڈیٹا.عدد()[0].m_قدر = اظہاریہ.گنتی_بٹ(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_گنتی_Ones(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    اگر (lhs.isFourState()) واپس سیٹ_All_بٹ_X();
    سیٹ_Zero();
    m_ڈیٹا.عدد()[0].m_قدر = lhs.گنتی_Ones();
    opCleanThis();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opIsUnknown(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    واپس سیٹ_Single_بٹ(lhs.isAnyXZ());
}
V3_نمبر& V3_نمبر::opOneHot(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    اگر (lhs.isFourState()) واپس سیٹ_All_بٹ_X();
    واپس سیٹ_Single_بٹ(lhs.گنتی_Ones() == 1);
}
V3_نمبر& V3_نمبر::opOneHot0(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    اگر (lhs.isFourState()) واپس سیٹ_All_بٹ_X();
    واپس سیٹ_Single_بٹ(lhs.گنتی_Ones() <= 1);
}
V3_نمبر& V3_نمبر::opCLog2(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    اگر (lhs.isFourState()) واپس سیٹ_All_بٹ_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحیح adjust = (lhs.گنتی_Ones() == 1) ? 0 : 1;
    برائے (صحیح ثنائی_ہندسہ_2 = lhs.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_Long(ثنائی_ہندسہ_2 + adjust);
            واپس *یہ;
        }
    }
    سیٹ_Zero();
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_Most_سیٹ_ثنائی_ہندسہ_P1(ثابت V3_نمبر& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    اگر (lhs.isFourState()) واپس سیٹ_All_بٹ_X();
    برائے (صحیح ثنائی_ہندسہ_2 = lhs.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_Long(ثنائی_ہندسہ_2 + 1);
            واپس *یہ;
        }
    }
    سیٹ_Zero();
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opLogNot(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 0;
            جاؤ آخری;
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
        } ورنہ {
            outc = 'x';
        }
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opNot(ثابت V3_نمبر& lhs) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opAnd(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) || rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {  // 0
        } ورنہ {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opOr(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) || rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            // 0
        } ورنہ {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opXor(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) || rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
        // else zero
    }
    واپس *یہ;
}

خالی V3_نمبر::fill_بٹ(صحیح destLsb, صحیح چوڑائی, حرف قدر) {
    UASSERT(destLsb >= 0 && چوڑائی >= 0, "Negative bit range");
    اگر (!چوڑائی) واپس;
    UASSERT(چوڑائی <= یہ->چوڑائی() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t قدر_لفظ = قدر == '1' || قدر == 'x' || قدر == 1 || قدر == 3 ? ~0U : 0;
    ثابت uint32_t x_لفظ = قدر == 'z' || قدر == 'x' || قدر == 2 || قدر == 3 ? ~0U : 0;
    ثابت صحیح پہلا_لفظ = VL_BITWORD_E(destLsb);
    ثابت صحیح آخری_لفظ = VL_BITWORD_E(destLsb + چوڑائی - 1);
    برائے (صحیح لفظ = پہلا_لفظ; لفظ <= آخری_لفظ; ++لفظ) {
        ثابت صحیح لفظ_Lsb = لفظ == پہلا_لفظ ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحیح لفظ_Msb = لفظ == آخری_لفظ ? VL_BITBIT_E(destLsb + چوڑائی - 1) : VL_SIZEBITS_E;
        ثابت uint32_t نقاب = VL_نقاب_E(لفظ_Msb - لفظ_Lsb + 1) << لفظ_Lsb;
        قدر_And_X& dest = m_ڈیٹا.عدد()[لفظ];
        dest.m_قدر = (dest.m_قدر & ~نقاب) | (قدر_لفظ & نقاب);
        dest.m_قدر_X = (dest.m_قدر_X & ~نقاب) | (x_لفظ & نقاب);
    }
}

خالی V3_نمبر::copy_بٹ(صحیح destLsb, ثابت V3_نمبر& ماخذ, صحیح ماخذ_Lsb, صحیح چوڑائی) {
    UASSERT(destLsb >= 0 && ماخذ_Lsb >= 0 && چوڑائی >= 0, "Negative bit range");
    اگر (!چوڑائی) واپس;
    UASSERT(چوڑائی <= یہ->چوڑائی() - destLsb, "Destination bit range exceeds number width");
    UASSERT(چوڑائی <= ماخذ.چوڑائی() - ماخذ_Lsb, "Source bit range exceeds number width");

    ثابت صحیح پہلا_لفظ = VL_BITWORD_E(destLsb);
    ثابت صحیح آخری_لفظ = VL_BITWORD_E(destLsb + چوڑائی - 1);
    برائے (صحیح dest_لفظ = پہلا_لفظ; dest_لفظ <= آخری_لفظ; ++dest_لفظ) {
        ثابت صحیح dest_ثنائی_ہندسہ = std::max(destLsb, dest_لفظ * VL_EDATASIZE);
        ثابت صحیح dest_انحراف = VL_BITBIT_E(dest_ثنائی_ہندسہ);
        ثابت صحیح ماخذ_ثنائی_ہندسہ = ماخذ_Lsb + dest_ثنائی_ہندسہ - destLsb;
        ثابت صحیح ماخذ_لفظ = VL_BITWORD_E(ماخذ_ثنائی_ہندسہ);
        ثابت صحیح ماخذ_انحراف = VL_BITBIT_E(ماخذ_ثنائی_ہندسہ);
        ثابت صحیح copy_چوڑائی = std::min(چوڑائی - (dest_ثنائی_ہندسہ - destLsb), VL_EDATASIZE - dest_انحراف);
        ثابت uint32_t نقاب = VL_نقاب_E(copy_چوڑائی) << dest_انحراف;

        ثابت قدر_And_X& ماخذ_Lo = ماخذ.m_ڈیٹا.عدد()[ماخذ_لفظ];
        uint32_t قدر = ماخذ_Lo.m_قدر >> ماخذ_انحراف;
        uint32_t قدر_X = ماخذ_Lo.m_قدر_X >> ماخذ_انحراف;
        اگر (ماخذ_انحراف && copy_چوڑائی > VL_EDATASIZE - ماخذ_انحراف) {
            ثابت قدر_And_X& ماخذ_Hi = ماخذ.m_ڈیٹا.عدد()[ماخذ_لفظ + 1];
            قدر |= ماخذ_Hi.m_قدر << (VL_EDATASIZE - ماخذ_انحراف);
            قدر_X |= ماخذ_Hi.m_قدر_X << (VL_EDATASIZE - ماخذ_انحراف);
        }

        قدر_And_X& dest = m_ڈیٹا.عدد()[dest_لفظ];
        dest.m_قدر = (dest.m_قدر & ~نقاب) | ((قدر << dest_انحراف) & نقاب);
        dest.m_قدر_X = (dest.m_قدر_X & ~نقاب) | ((قدر_X << dest_انحراف) & نقاب);
    }
}

V3_نمبر& V3_نمبر::opConcat(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    // See also error in V3Width
    اگر (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_بٹ(0, rhs, 0, rhs.چوڑائی());
    copy_بٹ(rhs.چوڑائی(), lhs, 0, lhs.چوڑائی());
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opLenN(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    سیٹ_Quad(lhs.مقصد_String().لمبائی_2());
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opRepl(ثابت V3_نمبر& lhs,
                           ثابت V3_نمبر& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    واپس opRepl(lhs, rhs.مقصد_UInt());
}

V3_نمبر& V3_نمبر::opRepl(ثابت V3_نمبر& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    اگر (v3Global.opt.replication_حد_بندی()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_حد_بندی())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_حد_بندی() << " is suspect: " << rhsval);
    }
    سیٹ_Zero();
    برائے (غیرعلامتی times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.چوڑائی();
        اگر (destLsb >= static_cast<uint32_t>(چوڑائی())) توڑو;
        copy_بٹ(static_cast<صحیح>(destLsb), lhs, 0,
                 std::min(lhs.چوڑائی(), چوڑائی() - static_cast<صحیح>(destLsb)));
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::op_دھارا_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    // See also error in V3Width
    اگر (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحیح ssize = std::min(rhs.مقصد_UInt(), static_cast<غیرعلامتی>(lhs.چوڑائی()));
    برائے (صحیح istart = 0; istart < lhs.چوڑائی(); istart += ssize) {
        ثابت صحیح ostart = std::max(0, lhs.چوڑائی() - ssize - istart);
        copy_بٹ(ostart, lhs, istart, std::min(ssize, lhs.چوڑائی() - istart));
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opLogAnd(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            loutc = 1;
            توڑو;
        }
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && loutc == 0) loutc = 'x';
    }
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < rhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            routc = 1;
            توڑو;
        }
        اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    اگر (routc == 1 && loutc == 1) outc = 1;
    اگر (routc == 0 || loutc == 0) outc = 0;
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opLogOr(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 1;
            جاؤ آخری;
        }
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && outc == 0) outc = 'x';
    }
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < rhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 1;
            جاؤ آخری;
        }
        اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && outc == 0) outc = 'x';
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opLogIf(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر lnot = lhs;
    lnot.opLogNot(lhs);
    واپس opLogOr(lnot, rhs);
}

V3_نمبر& V3_نمبر::opLogEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_نمبر ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    واپس opLogAnd(ifa, ifb);
}

V3_نمبر& V3_نمبر::opAtoN(ثابت V3_نمبر& lhs, صحیح بنیاد) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    UASSERT(بنیاد == AstAtoN::ATOREAL || بنیاد == 2 || بنیاد == 8 || بنیاد == 10 || بنیاد == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.مقصد_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.انتہا(), '_'), str.انتہا());

    اگر (بنیاد == AstAtoN::ATOREAL) واپس سیٹ_Double(std::atof(str.c_str()));

    errno = 0;
    خودکار v = std::strtol(str.c_str(), خالی_اشاریہ, بنیاد);
    اگر (errno != 0) v = 0;
    واپس سیٹ_Long_S(static_cast<int32_t>(v));
}

V3_نمبر& V3_نمبر::opPutcN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, ثابت V3_نمبر& ths) {
    عدد_ASSERT_OP_ARGS3(lhs, rhs, ths);
    عدد_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.مقصد_String();
    ثابت int32_t i = rhs.مقصد_SInt();
    ثابت int32_t c = ths.مقصد_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    اگر (0 <= i && static_cast<uint32_t>(i) < lstring.لمبائی_2() && c != 0) lstring[i] = c;
    واپس سیٹ_String(lstring);
}

V3_نمبر& V3_نمبر::opGetcN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.مقصد_String();
    ثابت int32_t i = rhs.مقصد_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    اگر (0 <= i && static_cast<uint32_t>(i) < lstring.لمبائی_2()) v = lstring[i];
    واپس سیٹ_Long(v);
}

V3_نمبر& V3_نمبر::opSubstrN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, ثابت V3_نمبر& ths) {
    عدد_ASSERT_OP_ARGS3(lhs, rhs, ths);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.مقصد_String();
    ثابت int32_t i = rhs.مقصد_SInt();
    ثابت int32_t j = ths.مقصد_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    اگر (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.لمبائی_2()) واپس سیٹ_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    واپس سیٹ_String(lstring.substr(i, j - i + 1));
}

V3_نمبر& V3_نمبر::opCompareNN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, منطقی ignoreCase) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحیح نتیجہ;
    ثابت string lstring = lhs.مقصد_String();
    ثابت string rstring = rhs.مقصد_String();
    اگر (ignoreCase) {
        نتیجہ = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } ورنہ {
        نتیجہ = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    واپس سیٹ_Long_S(نتیجہ);
}

V3_نمبر& V3_نمبر::opEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    اگر (lhs.isString()) واپس opEqN(lhs, rhs);
    اگر (lhs.isDouble()) واپس opEqD(lhs, rhs);
    حرف outc = 1;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            outc = 0;
            جاؤ آخری;
        }
        اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 0;
            جاؤ آخری;
        }
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
        اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    اگر (lhs.isString()) واپس opNeqN(lhs, rhs);
    اگر (lhs.isDouble()) واپس opNeqD(lhs, rhs);
    حرف outc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            outc = 1;
            جاؤ آخری;
        }
        اگر (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 1;
            جاؤ آخری;
        }
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
        اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3Hash V3_نمبر::مقصد_Hash() ثابت {
    // Shouldmatch isCaseEq
    اگر (isString()) واپس V3Hash{مقصد_String()};
    اگر (isDouble()) واپس V3Hash{مقصد_Double()};
    V3Hash hash{چوڑائی()};
    برائے (صحیح i = 0; i < الفاظ(); ++i) hash += m_ڈیٹا.عدد()[i].m_قدر;
    واپس hash;
}

منطقی V3_نمبر::isCaseEq(ثابت V3_نمبر& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    اگر (isString()) واپس مقصد_String() == rhs.مقصد_String();
    اگر (isDouble()) واپس مقصد_Double() == rhs.مقصد_Double();
    اگر (چوڑائی() != rhs.چوڑائی()) واپس جھوٹ;
    برائے (صحیح i = 0; i < الفاظ(); ++i) {
        اگر (!(m_ڈیٹا.عدد()[i] == rhs.m_ڈیٹا.عدد()[i])) واپس جھوٹ;
    }
    واپس سچ;
}

V3_نمبر& V3_نمبر::opCaseEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_نمبر& V3_نمبر::opCaseNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    اگر (lhs.isString()) {
        واپس opNeqN(lhs, rhs);
    } ورنہ اگر (lhs.isDouble()) {
        واپس opNeqD(lhs, rhs);
    }
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2) != rhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2)) {
            outc = 1;
            جاؤ آخری;
        }
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opWildEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (!rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
                outc = 'x';
                جاؤ آخری;
            }
            اگر (lhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2) != rhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2)) {
                outc = 0;
                جاؤ آخری;
            }
        }
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opWildNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (!rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
                outc = 'x';
                جاؤ آخری;
            }
            اگر (lhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2) != rhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2)) {
                outc = 1;
                جاؤ آخری;
            }
        }
    }
آخری:
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opGt(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        اگر (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 1;
        اگر (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 0;
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
        اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
    }
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opGtS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحیح mbit = std::max(lhs.چوڑائی() - 1, rhs.چوڑائی() - 1);
        اگر (lhs.ثنائی_ہندسہ_Is_XZ(mbit)) {
            outc = 'x';
        } ورنہ اگر (rhs.ثنائی_ہندسہ_Is_XZ(mbit)) {
            outc = 'x';
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is0(mbit) && rhs.ثنائی_ہندسہ_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } ورنہ اگر (lhs.ثنائی_ہندسہ_Is1_Extend(mbit) && rhs.ثنائی_ہندسہ_Is0(mbit)) {
            outc = 0;  // - !> +
        } ورنہ {
            // both positive or negative, normal >
            برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی() - 1, rhs.چوڑائی() - 1); ++ثنائی_ہندسہ_2) {
                اگر (lhs.ثنائی_ہندسہ_Is1_Extend(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 1;
                اگر (rhs.ثنائی_ہندسہ_Is1_Extend(ثنائی_ہندسہ_2) && lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 0;
                اگر (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
                اگر (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
            }
        }
    }
    واپس سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opGte(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر& eq = opEq(lhs, rhs);
    اگر (eq.isNeqZero()) واپس eq;  // Return true
    واپس opGt(lhs, rhs);
}

V3_نمبر& V3_نمبر::opGteS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر& eq = opEq(lhs, rhs);
    اگر (eq.isNeqZero()) واپس eq;  // Return true
    واپس opGtS(lhs, rhs);
}

V3_نمبر& V3_نمبر::opLt(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { واپس opGt(rhs, lhs); }
V3_نمبر& V3_نمبر::opLte(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { واپس opGte(rhs, lhs); }
V3_نمبر& V3_نمبر::opLtS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { واپس opGtS(rhs, lhs); }
V3_نمبر& V3_نمبر::opLteS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { واپس opGteS(rhs, lhs); }

V3_نمبر& V3_نمبر::op_سرکاؤ_R(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // L(lhs) bit return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    سیٹ_Zero();
    اگر (rhs.چوڑائی() > 32 && !rhs.is_بٹ_Zero(rhs.چوڑائی() - 1, 32)) واپس *یہ;
    ثابت uint32_t rhsval = rhs.مقصد_UInt();
    اگر (rhsval < static_cast<uint32_t>(lhs.چوڑائی())) {
        copy_بٹ(0, lhs, static_cast<صحیح>(rhsval),
                 std::min(چوڑائی(), lhs.چوڑائی() - static_cast<صحیح>(rhsval)));
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::op_سرکاؤ_RS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    سیٹ_Zero();
    ثابت منطقی حد_سے_تجاوز = rhs.چوڑائی() > 32 && !rhs.is_بٹ_Zero(rhs.چوڑائی() - 1, 32);
    اگر (!حد_سے_تجاوز) {
        ثابت uint32_t rhsval = rhs.مقصد_UInt();
        اگر (rhsval < lbits) {
            ثابت صحیح copy_چوڑائی = std::min(چوڑائی(), static_cast<صحیح>(lbits - rhsval));
            copy_بٹ(0, lhs, static_cast<صحیح>(rhsval), copy_چوڑائی);
            fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, lhs.ثنائی_ہندسہ_Is(lbits - 1));
            واپس *یہ;
        }
    }
    fill_بٹ(0, چوڑائی(), lhs.ثنائی_ہندسہ_Is(lbits - 1));  // '0/'1/'x/'z
    واپس *یہ;
}

V3_نمبر& V3_نمبر::op_سرکاؤ_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // L(lhs) bit return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    سیٹ_Zero();
    اگر (rhs.چوڑائی() > 32 && !rhs.is_بٹ_Zero(rhs.چوڑائی() - 1, 32)) واپس *یہ;
    ثابت uint32_t rhsval = rhs.مقصد_UInt();
    اگر (rhsval < static_cast<uint32_t>(چوڑائی())) {
        copy_بٹ(static_cast<صحیح>(rhsval), lhs, 0,
                 std::min(چوڑائی() - static_cast<صحیح>(rhsval), lhs.چوڑائی()));
    }
    واپس *یہ;
}

//======================================================================
// Ops - Arithmetic

V3_نمبر& V3_نمبر::opNegate(ثابت V3_نمبر& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    اگر (lhs.isFourState()) واپس سیٹ_All_بٹ_X();
    V3_نمبر notlhs(&lhs, چوڑائی());
    notlhs.opNot(lhs);
    ثابت V3_نمبر one(&lhs, چوڑائی(), 1);
    op_جوڑو(notlhs, one);
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_جوڑو(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    سیٹ_Zero();
    // Addem
    uint64_t حاصل = 0;
    برائے (صحیح لفظ = 0; لفظ < الفاظ(); ++لفظ) {
        ثابت uint64_t lwordval = lhs.m_ڈیٹا.عدد()[لفظ].m_قدر;
        ثابت uint64_t rwordval = rhs.m_ڈیٹا.عدد()[لفظ].m_قدر;
        ثابت uint64_t مجموع = lwordval + rwordval + حاصل;
        m_ڈیٹا.عدد()[لفظ].m_قدر = مجموع & 0xffffffffULL;
        حاصل = مجموع > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opSub(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    V3_نمبر negrhs(&rhs, چوڑائی());
    negrhs.opNegate(rhs);
    واپس op_جوڑو(lhs, negrhs);
}
V3_نمبر& V3_نمبر::opMul(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    سیٹ_Zero();
    اگر (چوڑائی() <= 64) {
        سیٹ_Quad(lhs.مقصد_UQuad() * rhs.مقصد_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } ورنہ {
        برائے (صحیح lword = 0; lword < lhs.الفاظ(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_ڈیٹا.عدد()[lword].m_قدر);
            اگر (lwordval == 0) جاری;
            برائے (صحیح rword = 0; rword < rhs.الفاظ(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_ڈیٹا.عدد()[rword].m_قدر);
                اگر (rwordval == 0) جاری;
                uint64_t mul = lwordval * rwordval;
                برائے (صحیح qword = lword + rword; qword < الفاظ(); ++qword) {
                    mul += static_cast<uint64_t>(m_ڈیٹا.عدد()[qword].m_قدر);
                    m_ڈیٹا.عدد()[qword].m_قدر = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    اگر (mul == 0) توڑو;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opMulS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    V3_نمبر lhsNoSign = lhs;
    اگر (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_نمبر rhsNoSign = rhs;
    اگر (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_نمبر qNoSign = opMul(lhsNoSign, rhsNoSign);
    اگر ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ورنہ {
        op_تفویض(qNoSign);
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opDiv(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    اگر (rhs.isEqZero()) واپس سیٹ_All_بٹ_XRemoved();
    اگر (lhs.چوڑائی() <= 64) {
        سیٹ_Quad(lhs.مقصد_UQuad() / rhs.مقصد_UQuad());
        واپس *یہ;
    } ورنہ {
        // Wide division
        واپس opModDivGuts(lhs, rhs, جھوٹ);
    }
}
V3_نمبر& V3_نمبر::opDivS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    اگر (rhs.isEqZero()) واپس سیٹ_All_بٹ_XRemoved();
    V3_نمبر lhsNoSign = lhs;
    اگر (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_نمبر rhsNoSign = rhs;
    اگر (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_نمبر qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    اگر ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ورنہ {
        op_تفویض(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *یہ);
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opModDiv(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    اگر (rhs.isEqZero()) واپس سیٹ_All_بٹ_XRemoved();
    اگر (lhs.چوڑائی() <= 64) {
        سیٹ_Quad(lhs.مقصد_UQuad() % rhs.مقصد_UQuad());
        واپس *یہ;
    } ورنہ {
        // Wide modulus
        واپس opModDivGuts(lhs, rhs, سچ);
    }
}
V3_نمبر& V3_نمبر::opModDivS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    اگر (rhs.isEqZero()) واپس سیٹ_All_بٹ_XRemoved();
    V3_نمبر lhsNoSign = lhs;
    اگر (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_نمبر rhsNoSign = rhs;
    اگر (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_نمبر qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    اگر (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } ورنہ {
        op_تفویض(qNoSign);
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opModDivGuts(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, منطقی is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    // Find MSB and check for zero.
    ثابت صحیح l_الفاظ = lhs.الفاظ();
    ثابت صحیح umsbp1 = lhs.most_سیٹ_ثنائی_ہندسہ_P1();  // dividend
    ثابت صحیح vmsbp1 = rhs.most_سیٹ_ثنائی_ہندسہ_P1();  //مقسوم علیہ
    اگر (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *یہ);
        واپس *یہ;
    }

    ثابت صحیح uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ثابت صحیح vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    اگر (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        برائے (صحیح j = uw - 1; j >= 0; j--) {
            ثابت uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_ڈیٹا.عدد()[j].m_قدر));
            m_ڈیٹا.عدد()[j].m_قدر = unw64 / static_cast<uint64_t>(rhs.m_ڈیٹا.عدد()[0].m_قدر);
            k = unw64
                - (static_cast<uint64_t>(m_ڈیٹا.عدد()[j].m_قدر)
                   * static_cast<uint64_t>(rhs.m_ڈیٹا.عدد()[0].m_قدر));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *یہ << " rem=0x" << std::hex
                                   << k << std::dec);
        اگر (is_modulus) {
            سیٹ_Zero();
            m_ڈیٹا.عدد()[0].m_قدر = k;
        }
        opCleanThis();
        واپس *یہ;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_الفاظ + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_الفاظ + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    برائے (صحیح i = 0; i < l_الفاظ; ++i) m_ڈیٹا.عدد()[i].m_قدر = 0;
    برائے (صحیح i = 0; i < l_الفاظ + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحیح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t سرکاؤ_نقاب = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    برائے (صحیح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_ڈیٹا.عدد()[i].m_قدر << s)
                | (سرکاؤ_نقاب & (rhs.m_ڈیٹا.عدد()[i - 1].m_قدر >> (32 - s)));
    }
    vn[0] = rhs.m_ڈیٹا.عدد()[0].m_قدر << s;

    // Copy and shift dividend by same amount; may set new upper word
    اگر (s) {
        un[uw] = lhs.m_ڈیٹا.عدد()[uw - 1].m_قدر >> (32 - s);
    } ورنہ {
        un[uw] = 0;
    }
    برائے (صحیح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_ڈیٹا.عدد()[i].m_قدر << s)
                | (سرکاؤ_نقاب & (lhs.m_ڈیٹا.عدد()[i - 1].m_قدر >> (32 - s)));
    }
    un[0] = lhs.m_ڈیٹا.عدد()[0].m_قدر << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    برائے (صحیح j = uw - vw; j >= 0; j--) {
        // Estimate
        ثابت uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        اگر (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            اگر (rhat < 0x100000000ULL) جاؤ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        برائے (صحیح i = 0; i < vw; ++i) {
            ثابت uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //تفریق
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_ڈیٹا.عدد()[j].m_قدر = qhat;  // Save quotient digit

        اگر (t < 0) {
            // Over subtracted; correct by adding back
            m_ڈیٹا.عدد()[j].m_قدر--;
            k = 0;
            برائے (صحیح i = 0; i < vw; ++i) {
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

    اگر (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        برائے (صحیح i = 0; i < vw; ++i) {
            m_ڈیٹا.عدد()[i].m_قدر = (un[i] >> s) | (سرکاؤ_نقاب & (un[i + 1] << (32 - s)));
        }
        برائے (صحیح i = vw; i < l_الفاظ; ++i) m_ڈیٹا.عدد()[i].m_قدر = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *یہ);
        واپس *یہ;
    } ورنہ {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *یہ);
        واپس *یہ;
    }
}

V3_نمبر& V3_نمبر::opPow(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, منطقی lsign, منطقی rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    اگر (lhs.isFourState() || rhs.isFourState()) واپس سیٹ_All_بٹ_X();
    اگر (rhs.isEqZero()) واپس سیٹ_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    اگر (rsign && rhs.isNegative()) {
        اگر (lhs.isEqZero()) {
            واپس سیٹ_All_بٹ_XRemoved();
        } ورنہ اگر (lhs.isEqOne()) {
            واپس سیٹ_Value1();
        } ورنہ اگر (lsign && lhs.isEqAllOnes()) {
            اگر (rhs.ثنائی_ہندسہ_Is1(0)) {
                واپس سیٹ_All_Bits1();  // -1^odd=-1
            } ورنہ {
                واپس سیٹ_Value1();  // -1^even=1
            }
        }
        واپس سیٹ_Zero();
    }
    اگر (lhs.isEqZero()) واپس سیٹ_Zero();
    سیٹ_Zero();
    m_ڈیٹا.عدد()[0].m_قدر = 1;
    V3_نمبر power(&lhs, چوڑائی());
    power.op_تفویض(lhs);
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < rhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (ثنائی_ہندسہ_2 > 0) {  // power = power*power
            V3_نمبر آخری_Power(&lhs, چوڑائی());
            آخری_Power.op_تفویض(power);
            power.opMul(آخری_Power, آخری_Power);
        }
        اگر (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {  // out *= power
            V3_نمبر آخری_برآمد(&lhs, چوڑائی());
            آخری_برآمد.op_تفویض(*یہ);
            opMul(آخری_برآمد, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::opPowSU(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    واپس opPow(lhs, rhs, سچ, جھوٹ);
}
V3_نمبر& V3_نمبر::opPowSS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    واپس opPow(lhs, rhs, سچ, سچ);
}
V3_نمبر& V3_نمبر::opPowUS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    واپس opPow(lhs, rhs, جھوٹ, سچ);
}

V3_نمبر& V3_نمبر::opBufIf1(ثابت V3_نمبر& ens, ثابت V3_نمبر& if1s) {
    عدد_ASSERT_OP_ARGS2(ens, if1s);
    عدد_ASSERT_LOGIC_ARGS2(ens, if1s);
    سیٹ_Zero();
    برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        اگر (ens.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, if1s.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2));
        } ورنہ {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'z');
        }
    }
    واپس *یہ;
}

// Sets all bits in range to the given value
V3_نمبر& V3_نمبر::op_سیٹ_Range(uint32_t lsb, uint32_t چوڑائی, حرف ثنائی_ہندسہ_قدر) {
    اگر (lsb < static_cast<uint32_t>(یہ->چوڑائی())) {
        fill_بٹ(static_cast<صحیح>(lsb),
                 static_cast<صحیح>(std::min(چوڑائی, static_cast<uint32_t>(یہ->چوڑائی()) - lsb)),
                 ثنائی_ہندسہ_قدر);
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::op_تفویض(ثابت V3_نمبر& lhs) { واپس op_تفویض_Non_XZ(lhs, جھوٹ); }
V3_نمبر& V3_نمبر::op_تفویض_Non_XZ(ثابت V3_نمبر& lhs, منطقی ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    اگر (یہ != &lhs) {
        اگر (VL_UNLIKELY(lhs.isNull())) {
            m_ڈیٹا.m_isNull = سچ;
        } ورنہ اگر (isString()) {
            اگر (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_ڈیٹا.str() = lhs.مقصد_String();
            } ورنہ {
                m_ڈیٹا.str() = lhs.m_ڈیٹا.str();
            }
        } ورنہ اگر (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            سیٹ_Zero();
        } ورنہ اگر (lhs.isDouble()) {
            سیٹ_Double(lhs.مقصد_Double());
        } ورنہ اگر (!ignoreXZ) {
            سیٹ_Zero();
            copy_بٹ(0, lhs, 0, std::min(چوڑائی(), lhs.چوڑائی()));
        } ورنہ {
            برائے (صحیح لفظ = 0; لفظ < الفاظ(); ++لفظ) {
                ثابت uint32_t قدر = لفظ < lhs.الفاظ() ? lhs.m_ڈیٹا.عدد()[لفظ].m_قدر
                                                                & ~lhs.m_ڈیٹا.عدد()[لفظ].m_قدر_X
                                                          : 0;
                m_ڈیٹا.عدد()[لفظ] = {قدر, 0};
            }
            opCleanThis();
        }
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opNToI(ثابت V3_نمبر& lhs) {
    // String to packed number
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    سیٹ_Zero();
    ثابت string& str = lhs.مقصد_String();
    برائے (سائز_t n = 0; n < str.لمبائی_2(); ++n) {
        ثابت حرف c = str[str.لمبائی_2() - 1 - n];
        برائے (سائز_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<غیرعلامتی طویل طویل>(c)};
            سیٹ_ثنائی_ہندسہ(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opExtendS(ثابت V3_نمبر& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    ثابت صحیح copy_چوڑائی = std::min(چوڑائی(), static_cast<صحیح>(lbits));
    copy_بٹ(0, lhs, 0, copy_چوڑائی);
    fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, lhs.ثنائی_ہندسہ_Is(lbits - 1));
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opExtendXZ(ثابت V3_نمبر& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    ثابت صحیح copy_چوڑائی = std::min(چوڑائی(), static_cast<صحیح>(lbits));
    copy_بٹ(0, lhs, 0, copy_چوڑائی);
    fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, lhs.ثنائی_ہندسہ_Is(lbits - 1));
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opClean(ثابت V3_نمبر& lhs, uint32_t بٹ) { واپس opSel(lhs, بٹ - 1, 0); }

خالی V3_نمبر::opCleanThis(منطقی warnOnTruncation) {
    // Clean MSB of number
    عدد_ASSERT_LOGIC_ARGS1(*یہ);
    ثابت قدر_And_X v = m_ڈیٹا.عدد()[الفاظ() - 1];
    ثابت uint32_t new_قدر_Msb = v.m_قدر & hi_لفظ_نقاب();
    ثابت uint32_t new_قدر_XMsb = v.m_قدر_X & hi_لفظ_نقاب();
    اگر (warnOnTruncation && (new_قدر_Msb != v.m_قدر || new_قدر_XMsb != v.m_قدر_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << چوڑائی() << " bit number: " << displayed("%d"));
    }
    m_ڈیٹا.عدد()[الفاظ() - 1] = {new_قدر_Msb, new_قدر_XMsb};
}

V3_نمبر& V3_نمبر::opSel(ثابت V3_نمبر& lhs, ثابت V3_نمبر& msb, ثابت V3_نمبر& lsb) {
    عدد_ASSERT_OP_ARGS3(lhs, msb, lsb);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS2(msb, lsb);
    اگر (lsb.isFourState() || msb.isFourState()) واپس سیٹ_All_بٹ_X();
    واپس opSel(lhs, msb.مقصد_UInt(), lsb.مقصد_UInt());
}

V3_نمبر& V3_نمبر::opSel(ثابت V3_نمبر& lhs, uint32_t msbval, uint32_t lsbval) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    صحیح copy_چوڑائی = 0;
    اگر (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.چوڑائی())) {
        ثابت uint64_t selected_چوڑائی = uint64_t{msbval} - lsbval + 1;
        copy_چوڑائی = static_cast<صحیح>(
            std::min({static_cast<uint64_t>(چوڑائی()), static_cast<uint64_t>(lhs.چوڑائی()) - lsbval,
                      selected_چوڑائی}));
    }
    copy_بٹ(0, lhs, copy_چوڑائی ? static_cast<صحیح>(lsbval) : 0, copy_چوڑائی);
    fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    واپس *یہ;
}

V3_نمبر& V3_نمبر::opSelInto(ثابت V3_نمبر& lhs, ثابت V3_نمبر& lsb, صحیح چوڑائی) {
    واپس opSelInto(lhs, lsb.مقصد_SInt(), چوڑائی);
}

V3_نمبر& V3_نمبر::opSelInto(ثابت V3_نمبر& lhs, صحیح lsbval, صحیح چوڑائی) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحیح copy_چوڑائی = std::max(0, std::min({چوڑائی, lhs.چوڑائی(), یہ->چوڑائی() - lsbval}));
    copy_بٹ(lsbval, lhs, 0, copy_چوڑائی);
    ثابت صحیح fill_چوڑائی
        = std::max(0, std::min(چوڑائی - copy_چوڑائی, یہ->چوڑائی() - lsbval - copy_چوڑائی));
    fill_بٹ(lsbval + copy_چوڑائی, fill_چوڑائی, v3Global.constRemoveXs() ? 0 : 'x');
    واپس *یہ;
}

//======================================================================
// Ops - Floating point

V3_نمبر& V3_نمبر::opIToRD(ثابت V3_نمبر& lhs, منطقی isSigned) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_نمبر noxz(lhs);
    noxz.op_تفویض_Non_XZ(lhs);
    دوہرا d = 0;
    ثابت منطقی negate = isSigned && noxz.isNegative();
    اگر (negate) {
        ثابت V3_نمبر noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    برائے (صحیح ثنائی_ہندسہ_2 = noxz.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        // Some precision might be lost in this add, that's what we want
        اگر (noxz.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) d += exp2(ثنائی_ہندسہ_2);
    }
    اگر (negate) d = -d;
    واپس سیٹ_Double(d);
}
V3_نمبر& V3_نمبر::opRToIS(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت دوہرا v = VL_TRUNC(lhs.مقصد_Double());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    واپس سیٹ_Long_S(i);
}
V3_نمبر& V3_نمبر::opRToIRoundS(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت دوہرا v = VL_باری(lhs.مقصد_Double());
    سیٹ_Zero();
    اتحاد {
        دوہرا d;
        uint64_t q;
    } u;
    u.d = v;
    (خالی)u.d;

    ثابت صحیح قوت_نما = static_cast<صحیح>((u.q >> 52ULL) & VL_نقاب_Q(11)) - 1023;
    ثابت صحیح lsb = قوت_نما - 52;
    ثابت uint64_t mantissa = (u.q & VL_نقاب_Q(52)) | (1ULL << 52);
    اگر (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        برائے (صحیح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 <= 52; ++ثنائی_ہندسہ_2) {
            اگر (mantissa & (1ULL << ثنائی_ہندسہ_2)) {
                ثابت صحیح outbit = ثنائی_ہندسہ_2 + lsb;
                اگر (outbit >= 0) سیٹ_ثنائی_ہندسہ(outbit, 1);
            }
        }
        اگر (v < 0) {
            ثابت V3_نمبر noSign = *یہ;
            opNegate(noSign);
        }
    }
    واپس *یہ;
}
V3_نمبر& V3_نمبر::op_Real_مقصد_بٹ(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.چوڑائی() == 64 && چوڑائی() == 64, "Real operation on wrong sized number");
    اتحاد {
        دوہرا m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.مقصد_Double();
    واپس سیٹ_Quad(u.m_v);
}
V3_نمبر& V3_نمبر::op_بٹ_مقصد_Real_D(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.چوڑائی() == 64 && چوڑائی() == 64, "Real operation on wrong sized number");
    اتحاد {
        دوہرا m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.مقصد_UQuad();
    واپس سیٹ_Double(u.m_d);
}
V3_نمبر& V3_نمبر::opNegateD(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    واپس سیٹ_Double(-lhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::op_جوڑو_D(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Double(lhs.مقصد_Double() + rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opSubD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Double(lhs.مقصد_Double() - rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opMulD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Double(lhs.مقصد_Double() * rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opDivD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Double(lhs.مقصد_Double() / rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opPowD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Double(قوت(lhs.مقصد_Double(), rhs.مقصد_Double()));
}
V3_نمبر& V3_نمبر::opEqD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_Double() == rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opNeqD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_Double() != rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opGtD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_Double() > rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opGteD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_Double() >= rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opLtD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_Double() < rhs.مقصد_Double());
}
V3_نمبر& V3_نمبر::opLteD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_Double() <= rhs.مقصد_Double());
}

//======================================================================
// Ops - String

V3_نمبر& V3_نمبر::opConcatN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_String(lhs.مقصد_String() + rhs.مقصد_String());
}
V3_نمبر& V3_نمبر::opReplN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_STRING_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(rhs);
    واپس opReplN(lhs, rhs.مقصد_UInt());
}
V3_نمبر& V3_نمبر::opReplN(ثابت V3_نمبر& lhs, uint32_t rhsval) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    string برآمد;
    برآمد.reserve(lhs.مقصد_String().لمبائی_2() * rhsval);
    برائے (غیرعلامتی times = 0; times < rhsval; ++times) برآمد += lhs.مقصد_String();
    واپس سیٹ_String(برآمد);
}
V3_نمبر& V3_نمبر::opCvtPackString(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    واپس سیٹ_String(lhs.مقصد_String());
}
V3_نمبر& V3_نمبر::op_مقصد_زیریں_N(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string برآمد = VString::downcase(lhs.مقصد_String());
    واپس سیٹ_String(برآمد);
}
V3_نمبر& V3_نمبر::op_مقصد_بالائی_N(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string برآمد = VString::upcase(lhs.مقصد_String());
    واپس سیٹ_String(برآمد);
}

V3_نمبر& V3_نمبر::opEqN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_String() == rhs.مقصد_String());
}
V3_نمبر& V3_نمبر::opNeqN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_String() != rhs.مقصد_String());
}
V3_نمبر& V3_نمبر::opGtN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_String() > rhs.مقصد_String());
}
V3_نمبر& V3_نمبر::opGteN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_String() >= rhs.مقصد_String());
}
V3_نمبر& V3_نمبر::opLtN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_String() < rhs.مقصد_String());
}
V3_نمبر& V3_نمبر::opLteN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    واپس سیٹ_Single_بٹ(lhs.مقصد_String() <= rhs.مقصد_String());
}

//======================================================================

خالی V3_نمبر::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_سطر* ثابت fl = نیا File_سطر{File_سطر::built_اندر_Filename()};
    V3_نمبر عدد{fl, 32, 0};
    عدد.selfTestThis();
}

خالی V3_نمبر::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(0, 0), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(1e19, 1e19), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(9, 0.0001), جھوٹ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(1, 1 + std::numeric_limits<دوہرا>::epsilon()), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(0.009, 0.00899999999999999931998839741709), سچ);

    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(0), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(1), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(-1), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(1.0001), جھوٹ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(0.9999), جھوٹ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(-1.0001), جھوٹ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(-0.9999), جھوٹ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
