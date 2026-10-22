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

#ضمن "V3PchAstMT.h"

#ضمن "V3Number.h"

#ضمن "V3File.h"

#ضمن <algorithm>
#ضمن <cerrno>
#ضمن <cmath>
#ضمن <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr صحيح MAX_SPRINTF_DOUBLE_ناپ
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#تعریف_کرو عدد_ASSERT_OP_ARGS1(arg1) \
    UASSERT((هذا != &(arg1)), "Number operation called with same source and dest")
#تعریف_کرو عدد_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2)), \
            "Number operation called with same source and dest")
#تعریف_کرو عدد_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3)), \
            "Number operation called with same source and dest")
#تعریف_کرو عدد_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3) && هذا != &(arg4)), \
            "Number operation called with same source and dest")

#تعریف_کرو عدد_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).انکڑے_Type() == V3_نمبر_انکڑے::V3_نمبر_انکڑے_Type::LOGIC), \
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

فراغ V3_نمبر::v3error_سرا(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (m_nodep) {
        m_nodep->v3error_سرا(nsstr);
    } وإلا إذا (m_fileline) {
        m_fileline->v3error_سرا(nsstr);
    } وإلا {
        V3Error::v3error_سرا(nsstr, "", خالی_سوچک);
    }
}

فراغ V3_نمبر::v3error_سرا_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_سرا(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_نمبر::V3_نمبر(Ast_گرہ* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    إذا (str.خالی_تھاواں()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } وإلا {
        init(nodep, std::max<صحيح>(str.لمائی() * 8, 1));
        لكل (بنانشانی pos = 0; pos < str.لمائی(); ++pos) {
            ثابت صحيح topos = str.لمائی() - 1 - pos;
            قدر_And_X& v = m_انکڑے.عدد()[topos / 4];
            لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < 8; ++ثنائی_ہندسہ_2) {
                إذا (str[pos] & (1UL << ثنائی_ہندسہ_2)) v.m_قدر |= (1UL << (ثنائی_ہندسہ_2 + (topos % 4) * 8));
            }
        }
    }
    m_انکڑے.m_fromString = سچ;
    opCleanThis(سچ);
}

V3_نمبر::V3_نمبر(Ast_گرہ* nodep, ثابت Ast_گرہ_DType* nodedtypep) {
    إذا (nodedtypep->isCHandle()) {
        init(nodep);
        سیٹ_Null();
    } وإلا إذا (nodedtypep->isDouble()) {
        init(nodep, 64);
        سیٹ_Double(0.0);
    } وإلا إذا (nodedtypep->isString()) {
        init(nodep);
        سیٹ_String("");
    } وإلا {
        init(nodep, nodedtypep->چوڑائی(), nodedtypep->چوڑائی_Sized());
    }
}

فراغ V3_نمبر::create(ثابت حرف* sourcep) {
    m_انکڑے.سیٹ_Logic();
    ثابت حرف* قدر_startp = sourcep;
    لكل (ثابت حرف* cp = sourcep; *cp; ++cp) {
        إذا (*cp == '\'') {
            قدر_startp = cp + 1;
            اكسر;
        }
    }

    منطقي userSized = خطأ;
    منطقي unbased = خطأ;
    حرف ادھار = '\0';
    إذا (قدر_startp != sourcep) {  // Has a '
        string widthn;
        ثابت حرف* cp = sourcep;
        لكل (; *cp; ++cp) {
            إذا (*cp == '\'') {
                ++cp;
                اكسر;
            }
            إذا (*cp != '_') widthn += *cp;
        }
        طالما (*cp == '_') cp++;
        إذا (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(سچ);
        }
        إذا (*cp) {
            ادھار = *cp;
            ++cp;
        }
        قدر_startp = cp;

        userSized = widthn.لمائی() != 0;
        إذا (std::atoi(widthn.c_str())) {
            إذا (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_عدد_چوڑائی()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                چوڑائی(v3Global.opt.max_عدد_چوڑائی(), سچ);
            } وإلا {
                چوڑائی(std::atoi(widthn.c_str()), سچ);
            }
        }
    } وإلا {
        unbased = سچ;
        ادھار = 'd';
    }

    لكل (صحيح i = 0; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    إذا (ادھار == '0') {
        چوڑائی(1, خطأ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 0);
        m_انکڑے.m_autoExtend = سچ;
    } وإلا إذا (ادھار == '1') {
        چوڑائی(1, خطأ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 1);
        m_انکڑے.m_autoExtend = سچ;
    } وإلا إذا (std::tolower(ادھار) == 'z') {
        چوڑائی(1, خطأ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 'z');
        m_انکڑے.m_autoExtend = سچ;
    } وإلا إذا (std::tolower(ادھار) == 'x') {
        چوڑائی(1, خطأ);  // So we extend it
        سیٹ_ثنائی_ہندسہ(0, 'x');
        m_انکڑے.m_autoExtend = سچ;
    }
    // Otherwise...
    وإلا إذا (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحيح چوڑائی_Bound = std::max<صحيح>(32, std::لڑی_لمبائی(قدر_startp) * 4);
        چوڑائی(چوڑائی_Bound, خطأ);  // Will change width below
        إذا (unbased) isSigned(سچ);  // Also says the spec.
    }

    // Ignore leading blanks
    طالما (*قدر_startp == '_' || std::خلا_اے(*قدر_startp)) ++قدر_startp;
    إذا (!*قدر_startp && !m_انکڑے.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    إذا (userSized && m_انکڑے.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحيح obit = 0;  // Start at LSB
    صحيح ادھار_align = 1;
    إذا (std::tolower(ادھار) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        طالما (*قدر_startp == '_' || *قدر_startp == '0') ++قدر_startp;
        // Convert decimal number to hex
        صحيح olen = 0;
        uint32_t val = 0;
        صحيح got_x = 0;
        صحيح got_z = 0;
        صحيح got_01 = 0;
        منطقي warnedTooMany = خطأ;
        لكل (ثابت حرف* cp = قدر_startp; *cp; ++cp) {
            بدل (std::tolower(*cp)) {
            حالة '0':  // FALLTHRU
            حالة '1':  // FALLTHRU
            حالة '2':  // FALLTHRU
            حالة '3':  // FALLTHRU
            حالة '4':  // FALLTHRU
            حالة '5':  // FALLTHRU
            حالة '6':  // FALLTHRU
            حالة '7':  // FALLTHRU
            حالة '8':  // FALLTHRU
            حالة '9': {
                إذا (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_انکڑے.عدد()[0].m_قدر = val;
                } وإلا {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_نمبر ضرب_حاصل(هذا, چوڑائی() + 4);  // +4 for overflow detection
                    ثابت V3_نمبر ten(هذا, چوڑائی() + 4, 10);
                    ثابت V3_نمبر addend(هذا, چوڑائی(), (*cp - '0'));
                    ضرب_حاصل.opMul(*هذا, ten);
                    op_جوڑو(ضرب_حاصل, addend);
                    إذا (!warnedTooMany && ضرب_حاصل.بٹ_قدر(چوڑائی(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = سچ;
                    }
                }
                ++olen;
                got_01 = 1;
                اكسر;
            }
            حالة 'z':
            حالة '?': {
                got_z = 1;
                إذا (!userSized) چوڑائی(32, خطأ);
                سیٹ_All_بٹ_Z();
                اكسر;
            }
            حالة 'x': {
                got_x = 1;
                إذا (!userSized) چوڑائی(32, خطأ);
                سیٹ_All_بٹ_X();
                اكسر;
            }
            حالة '_': اكسر;
            افتراضي: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                اكسر;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = چوڑائی();
        إذا ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << قدر_startp);
        }
    } وإلا {
        // Convert bin/octal number to hex
        لكل (ثابت حرف* cp = قدر_startp + std::لڑی_لمبائی(قدر_startp) - 1; cp >= قدر_startp;
             cp--) {
            إذا (*cp != '_' && *cp != '0' && obit >= چوڑائی()) {
                warnTooMany(sourcep);
                اكسر;
            }
            بدل (std::tolower(ادھار)) {
            حالة 'b': {
                ادھار_align = 1;
                بدل (std::tolower(*cp)) {
                حالة '0': سیٹ_ثنائی_ہندسہ(obit++, 0); اكسر;
                حالة '1': سیٹ_ثنائی_ہندسہ(obit++, 1); اكسر;
                حالة 'z':
                حالة '?': سیٹ_ثنائی_ہندسہ(obit++, 'z'); اكسر;
                حالة 'x': سیٹ_ثنائی_ہندسہ(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                افتراضي: v3error("Illegal character in binary constant: " << *cp);
                }
                اكسر;
            }

            حالة 'o':
            حالة 'c': {
                ادھار_align = 3;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 0);  اكسر;
                حالة '1': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 0);  اكسر;
                حالة '2': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 0);  اكسر;
                حالة '3': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 0);  اكسر;
                حالة '4': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 1);  اكسر;
                حالة '5': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 0);  سیٹ_ثنائی_ہندسہ(obit++, 1);  اكسر;
                حالة '6': سیٹ_ثنائی_ہندسہ(obit++, 0); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 1);  اكسر;
                حالة '7': سیٹ_ثنائی_ہندسہ(obit++, 1); سیٹ_ثنائی_ہندسہ(obit++, 1);  سیٹ_ثنائی_ہندسہ(obit++, 1);  اكسر;
                حالة 'z':
                حالة '?': سیٹ_ثنائی_ہندسہ(obit++, 'z'); سیٹ_ثنائی_ہندسہ(obit++, 'z'); سیٹ_ثنائی_ہندسہ(obit++, 'z'); اكسر;
                حالة 'x': سیٹ_ثنائی_ہندسہ(obit++, 'x'); سیٹ_ثنائی_ہندسہ(obit++, 'x'); سیٹ_ثنائی_ہندسہ(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                // clang-format on
                افتراضي: v3error("Illegal character in octal constant");
                }
                اكسر;
            }

            حالة 'h':  // FALLTHRU
            حالة 'x': {
                ادھار_align = 4;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '1': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '2': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '3': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '4': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '5': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '6': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '7': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); اكسر;
                حالة '8': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة '9': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'a': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'b': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'c': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'd': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'e': سیٹ_ثنائی_ہندسہ(obit++,0); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'f': سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); سیٹ_ثنائی_ہندسہ(obit++,1); اكسر;
                حالة 'z':
                حالة '?': سیٹ_ثنائی_ہندسہ(obit++,'z'); سیٹ_ثنائی_ہندسہ(obit++,'z'); سیٹ_ثنائی_ہندسہ(obit++,'z'); سیٹ_ثنائی_ہندسہ(obit++,'z'); اكسر;
                حالة 'x': سیٹ_ثنائی_ہندسہ(obit++,'x'); سیٹ_ثنائی_ہندسہ(obit++,'x'); سیٹ_ثنائی_ہندسہ(obit++,'x'); سیٹ_ثنائی_ہندسہ(obit++,'x'); اكسر;
                    // clang-format on
                حالة '_': اكسر;
                افتراضي:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    اكسر;
                }  // LCOV_EXCL_STOP
                اكسر;
            }
            افتراضي:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << ادھار);
                اكسر;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    إذا (!userSized && !m_انکڑے.m_autoExtend) {
        چوڑائی(std::max(32, ادھار_align * ((چوڑائی_منزل_Fit() + ادھار_align - 1) / ادھار_align)), خطأ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    طالما (obit <= چوڑائی() && obit && ثنائی_ہندسہ_Is_XZ(obit - 1)) {
        سیٹ_ثنائی_ہندسہ(obit, ثنائی_ہندسہ_Is(obit - 1));
        ++obit;
    }
    opCleanThis(سچ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

فراغ V3_نمبر::warnTooMany(ثابت string& قدر) {
    v3warn(WIDTHDIGITS, "Too many digits for " << چوڑائی() << " bit number: '" << قدر << "'");
}

فراغ V3_نمبر::nodep(Ast_گرہ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    إذا (!nodep) أرجع;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

منطقي V3_نمبر::epsilonEqual(مزدوج a, مزدوج b) {
    أرجع std::fabs(a - b)
           <= (std::numeric_limits<مزدوج>::epsilon() * std::max(1.0, std::max(a, b)));
}

منطقي V3_نمبر::epsilonIntegral(مزدوج a) {
    ثابت مزدوج dint = static_cast<مزدوج>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    أرجع epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

صحيح V3_نمبر::log2b(uint32_t عدد) {
    // See also opCLog2
    لكل (صحيح ثنائی_ہندسہ_2 = 31; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2--) {
        إذا (عدد & (1ULL << ثنائی_ہندسہ_2)) أرجع ثنائی_ہندسہ_2;
    }
    أرجع 0;
}

صحيح V3_نمبر::log2bQuad(uint64_t عدد) {
    إذا (عدد >> 32ULL) أرجع 32 + log2b(عدد >> 32ULL);
    أرجع log2b(عدد);
}

//======================================================================
// Setters

V3_نمبر& V3_نمبر::سیٹ_Zero() {
    لكل (صحيح i = 0; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, 0};
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_Quad(uint64_t قدر) {
    سیٹ_Zero();
    m_انکڑے.عدد()[0].m_قدر = قدر & 0xffffffffULL;
    إذا (چوڑائی() > 32) m_انکڑے.عدد()[1].m_قدر = (قدر >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_Long(uint32_t قدر) {
    سیٹ_Zero();
    m_انکڑے.عدد()[0].m_قدر = قدر;
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_Long_S(int32_t قدر) {
    سیٹ_Zero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = قدر;
    (فراغ)u.s;
    m_انکڑے.عدد()[0].m_قدر = u.u;
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_Double(مزدوج قدر) {
    UASSERT(چوڑائی() == 64, "Real operation on wrong sized number");
    m_انکڑے.سیٹ_Double();
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.d = قدر;
    (فراغ)u.d;
    لكل (صحيح i = 2; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, 0};
    m_انکڑے.عدد()[0].m_قدر = u.u[0];
    m_انکڑے.عدد()[1].m_قدر = u.u[1];
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_Single_بٹ(حرف قدر) {
    لكل (صحيح i = 1 /*اتلا*/; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, 0};
    m_انکڑے.عدد()[0] = {(قدر == '1' || قدر == 'x' || قدر == 1 || قدر == 3),
                       (قدر == 'z' || قدر == 'x' || قدر == 2 || قدر == 3)};
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::سیٹ_All_Bits0() {
    سیٹ_Zero();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_All_Bits1() {
    لكل (صحيح i = 0; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {~0U, 0};
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_All_بٹ_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    لكل (صحيح i = 0; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {~0U, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_All_بٹ_Z() {
    لكل (صحيح i = 0; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::سیٹ_All_بٹ_XRemoved() {
    إذا (!v3Global.constRemoveXs()) {
        أرجع سیٹ_All_بٹ_X();
    } وإلا {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        إذا (v3Global.opt.x_سونپو() == "1") {
            أرجع سیٹ_All_Bits1();
        } وإلا {
            أرجع سیٹ_All_Bits0();
        }
    }
}
V3_نمبر& V3_نمبر::سیٹ_Value1() {
    m_انکڑے.عدد()[0] = {1, 0};
    لكل (صحيح i = 1; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, 0};
    أرجع *هذا;
}

فراغ V3_نمبر::سیٹ_ثنائی_ہندسہ_X0(صحيح ثنائی_ہندسہ_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_نمبر& V3_نمبر::سیٹ_نقاب(صحيح nbits, صحيح lsb) {
    سیٹ_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_بٹ(lsb, std::max(0, std::min(nbits, چوڑائی() - lsb)), 1);
    أرجع *هذا;
}

//======================================================================
// ACCESSORS - as strings

string V3_نمبر::ascii(منطقي prefixed, منطقي cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (is1_قدم()) أرجع "1step";
    إذا (isNull()) أرجع "null";
    std::ostringstream باہرلا;
    إذا (isDouble()) {
        باہرلا.precision(17);
        إذا (VL_UNCOVERABLE(چوڑائی() != 64)) {
            باہرلا << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } وإلا {
            باہرلا << منزل_Double();
            إذا (منزل_Double() == floor(منزل_Double())) باہرلا << ".0";
        }
        أرجع باہرلا.str();
    } وإلا إذا (isString()) {
        أرجع '"' + منزل_String() + '"';
    } وإلا {
        إذا (VL_UNCOVERABLE((m_انکڑے.عدد()[الفاظ() - 1].m_قدر | m_انکڑے.عدد()[الفاظ() - 1].m_قدر_X)
                           & ~hi_لفظ_نقاب())) {
            باہرلا << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    إذا (prefixed) {
        إذا (sized()) {
            باہرلا << چوڑائی() << "'";
        } وإلا إذا (autoExtend() && چوڑائی() == 1) {
            باہرلا << "'";
            إذا (ثنائی_ہندسہ_Is0(0)) {
                باہرلا << '0';
            } وإلا إذا (ثنائی_ہندسہ_Is1(0)) {
                باہرلا << '1';
            } وإلا إذا (ثنائی_ہندسہ_Is_Z(0)) {
                باہرلا << 'z';
            } وإلا {
                باہرلا << 'x';
            }
            أرجع باہرلا.str();
        } وإلا {
            إذا (cleanVerilog) {
                باہرلا << "'";
            } وإلا {
                باہرلا << "?" << چوڑائی() << "?";
            }
        }
        إذا (isSigned()) باہرلا << "s";
    }

    ثابت منطقي ثنائی = (isFourState()
#جے_تعریف_شدہ V3_نمبر_ASCII_ثنائی
                         // cppcheck-suppress konwnConditionTrueFalse
                         || سچ
#جے_انت
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    إذا (ثنائی) {
        باہرلا << "b";
        باہرلا << displayed("%0b");
    } وإلا {
        إذا (prefixed) باہرلا << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        باہرلا << displayed("%0h");
    }
    إذا (isNull()) {
        إذا (VL_UNCOVERABLE(!isEqZero())) {
            باہرلا << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } وإلا {
            باہرلا << " [null]";
        }
    }
    أرجع باہرلا.str();
}

منطقي V3_نمبر::displayedFmtHasArg(حرف format, منطقي is_جانچ) {
    // Is this a format letter that takes an argument in runtime?
    (فراغ)is_جانچ;
    بدل (std::tolower(format)) {
    حالة 'b': أرجع سچ;  //ثنائی
    حالة 'c': أرجع سچ;  // Character
    حالة 'd': أرجع سچ;  // Decimal
    حالة 'e': أرجع سچ;  // Floating
    حالة 'f': أرجع سچ;  // Floating
    حالة 'g': أرجع سچ;  // Floating
    حالة 'h': أرجع سچ;  // Hex
    حالة 'o': أرجع سچ;  // Octal
    حالة 'p': أرجع سچ;  //نمونہ
    حالة 's': أرجع سچ;  // String
    حالة 't': أرجع سچ;  //ویلا
    حالة 'u': أرجع سچ;  // Packed 2-state
    حالة 'v': أرجع سچ;  // Strength
    حالة 'x': أرجع سچ;  // Hex
    حالة 'z': أرجع سچ;  // Packed 4-state
    حالة '?': أرجع سچ;  // Internal: Compute format in V3Width based on data type
    افتراضي: أرجع خطأ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_نمبر::displayPad(ناپ_t fmtsize, حرف pad, منطقي کھبے, ثابت string& وچ) VL_خالص {
    string padding;
    إذا (وچ.لمائی() < fmtsize) padding = string(fmtsize - وچ.لمائی(), pad);
    أرجع کھبے ? (وچ + padding) : (padding + وچ);
}

string V3_نمبر::displayedEnumName(ثابت Ast_Enum_شے* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    أرجع VString::isIdentifier(name) && !std::ہندسہ_اے(static_cast<بنانشانی حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_نمبر::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3_نمبر قدر{هذا, dtypep->چوڑائی()};
    قدر.op_سونپو(*هذا);
    لكل (ثابت Ast_Enum_شے* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_شے)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        إذا (!constp->عدد().isAnyXZ() && قدر.isCaseEq(constp->عدد()))
            أرجع displayedEnumName(itemp);
    }
    أرجع "";
}

string V3_نمبر::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
    V3_نمبر قدر{هذا, dtypep->چوڑائی()};
    قدر.op_سونپو(*هذا);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.پٹھ());
    إذا (fmt != 'p' && fmt != 's') أرجع قدر.displayed(argp, vformat, numericAttr);
    string name = قدر.displayedEnumName(dtypep);
    إذا (name.خالی_تھاواں()) name = قدر.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_نمبر name_عدد{هذا};
    name_عدد.سیٹ_String(name);
    أرجع name_عدد.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_نمبر::displayed(ثابت Ast_گرہ* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    أرجع displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_نمبر::displayed(File_سطر* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    خودکار pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *هذا);
    ++pos;
    منطقي کھبے = خطأ;
    إذا (pos[0] == '-') {
        کھبے = سچ;
        ++pos;
    }
    string fmtsize;
    لكل (; pos != vformat.cend() && (std::ہندسہ_اے(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    حرف fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    إذا (formatAttr.isComplex()) {
        إذا (fmt != 'p') fmt = 's';  // override
    } وإلا إذا (formatAttr.isString()) {
        إذا (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_نمبر strwide{fl, static_cast<صحيح>(منزل_String().لمائی()) * 8, 0};
            strwide.opNToI(*هذا);
            أرجع strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        إذا (fmt != 'p') fmt = 's';  // override
    } وإلا إذا (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        إذا (fmt == 'p') {
            إذا (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } وإلا {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } وإلا إذا (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        إذا (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_نمبر nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*هذا);
            أرجع nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    بدل (fmt) {
    حالة 'c': {
        إذا (چوڑائی() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت بنانشانی صحيح v = بٹ_قدر(0, 8);
        str.ڈالو_پٹھ(static_cast<حرف>(v));
        أرجع str;
    }
    حالة 'e':  // FALLTHRU
    حالة 'f':  // FALLTHRU
    حالة 'g': {
        ثابت مزدوج n = formatAttr.isDouble()   ? منزل_Double()
                         : formatAttr.isSigned() ? منزل_SQuad()
                                                 : منزل_UQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_ناپ];
        (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_ناپ, vformat.c_str(), n);
        أرجع tmp;
    }
    حالة 's': {
        إذا (formatAttr.isString() || formatAttr.isComplex()) {
            str = منزل_String();
        } وإلا {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحيح ثنائی_ہندسہ_2 = چوڑائی() - 1;
            منطقي شروع = سچ;
            طالما ((ثنائی_ہندسہ_2 % 8) != 7) ++ثنائی_ہندسہ_2;
            لكل (; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2 -= 8) {
                ثابت صحيح v = بٹ_قدر(ثنائی_ہندسہ_2 - 7, 8);
                إذا (!شروع || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    شروع = خطأ;  // Drop leading 0s
                } وإلا {
                    إذا (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت ناپ_t fmtsizen = static_cast<ناپ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', کھبے, str);
        أرجع str;
    }
    حالة 'p': {  //نمونہ
        // 'p' with NUMBER was earlier converted to 'd'
        إذا (formatAttr.isDouble()) {
            ثابت مزدوج n = formatAttr.isDouble()   ? منزل_Double()
                             : formatAttr.isSigned() ? منزل_SQuad()
                                                     : منزل_UQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_ناپ];
            (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_ناپ, "%g", n);
            أرجع tmp;
        }
        إذا (formatAttr.isString())
            أرجع '"' + V3_باہرلا_Formatter::quoteNameControls(منزل_String()) + '"';
        إذا (formatAttr.isComplex()) أرجع منزل_String();
        أرجع "%p";
    }
    حالة 'b':  // FALLTHRU
    حالة 'o':  // FALLTHRU
    حالة 'h':  // FALLTHRU
    حالة 'x': {
        صحيح ثنائی_ہندسہ_2 = چوڑائی() - 1;
        إذا (کھبے || !fmtsize.خالی_تھاواں()) {
            طالما (ثنائی_ہندسہ_2 && ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) --ثنائی_ہندسہ_2;
        }
        بدل (fmt) {
        حالة 'b': {
            لكل (; ثنائی_ہندسہ_2 >= 0; --ثنائی_ہندسہ_2) {
                إذا (ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
                    str += '0';
                } وإلا إذا (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
                    str += '1';
                } وإلا إذا (ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) {
                    str += 'z';
                } وإلا {
                    str += 'x';
                }
            }
            اكسر;
        }
        حالة 'o': {
            طالما ((ثنائی_ہندسہ_2 % 3) != 2) ++ثنائی_ہندسہ_2;
            لكل (; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2 -= 3) {
                ثابت صحيح عدد_X = گنتی_X(ثنائی_ہندسہ_2 - 2, 3);
                ثابت صحيح عدد_Z = گنتی_Z(ثنائی_ہندسہ_2 - 2, 3);
                إذا (عدد_X == 3 || عدد_X == چوڑائی() - (ثنائی_ہندسہ_2 - 2)) {
                    str += 'x';
                    استمر;
                }
                إذا (عدد_Z == 3 || عدد_Z == چوڑائی() - (ثنائی_ہندسہ_2 - 2)) {
                    str += 'z';
                    استمر;
                }
                إذا (عدد_X > 0) {
                    str += 'X';
                    استمر;
                }
                إذا (عدد_Z > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = بٹ_قدر(ثنائی_ہندسہ_2 - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            اكسر;
        }
        افتراضي: {  // h/x
            طالما ((ثنائی_ہندسہ_2 % 4) != 3) ++ثنائی_ہندسہ_2;
            لكل (; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2 -= 4) {
                ثابت صحيح عدد_X = گنتی_X(ثنائی_ہندسہ_2 - 3, 4);
                ثابت صحيح عدد_Z = گنتی_Z(ثنائی_ہندسہ_2 - 3, 4);
                إذا (عدد_X == 4 || عدد_X == چوڑائی() - (ثنائی_ہندسہ_2 - 3)) {
                    str += 'x';
                    استمر;
                }
                إذا (عدد_Z == 4 || عدد_Z == چوڑائی() - (ثنائی_ہندسہ_2 - 3)) {
                    str += 'z';
                    استمر;
                }
                إذا (عدد_X > 0) {
                    str += 'X';
                    استمر;
                }
                إذا (عدد_Z > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = بٹ_قدر(ثنائی_ہندسہ_2 - 3, 4);
                إذا (v >= 10) {
                    str += static_cast<حرف>('a' + v - 10);
                } وإلا {
                    str += static_cast<حرف>('0' + v);
                }
            }
            اكسر;
        }
        }  // switch
        ثابت ناپ_t fmtsizen = static_cast<ناپ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (کھبے ? ' ' : '0'), کھبے, str);
        أرجع str;
    }  // case b/d/x/o
    حالة 't':  //ویلا
    حالة 'd': {  // Unsigned decimal
        ثابت منطقي issigned = formatAttr.isSigned();
        إذا (fmtsize == "" && !کھبے) {
            ثابت مزدوج mantissabits = چوڑائی() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            مزدوج dchars = mantissabits / 3.321928094887362 + 1.0;
            إذا (issigned) ++dchars;  // space for sign
            fmtsize = cvt_منزل_Str(static_cast<صحيح>(dchars));
        }
        منطقي hasXZ = خطأ;
        إذا (isAllX()) {
            str = "x";
            hasXZ = سچ;
        } وإلا إذا (isAllZ()) {
            str = "z";
            hasXZ = سچ;
        } وإلا إذا (isAnyX()) {
            str = "X";
            hasXZ = سچ;
        } وإلا إذا (isAnyZ()) {
            str = "Z";
            hasXZ = سچ;
        }
        إذا (!hasXZ) {
            إذا (issigned) {
                إذا (چوڑائی() > 64) {
                    str = منزل_Decimal_S();
                } وإلا {
                    str = cvt_منزل_Str(منزل_SQuad());
                }
            } وإلا {
                إذا (چوڑائی() > 64) {
                    str = منزل_Decimal_U();
                } وإلا {
                    str = cvt_منزل_Str(منزل_UQuad());
                }
            }
        }
        ثابت منطقي zeropad = fmtsize.لمائی() > 0 && fmtsize[0] == '0' && !کھبے;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت ناپ_t fmtsizen = static_cast<ناپ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), کھبے, str);
        أرجع str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالة 'u': {  // Packed 2-state
        لكل (صحيح i = 0; i < الفاظ(); ++i) {
            ثابت uint32_t v = m_انکڑے.عدد()[i].m_قدر;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'z': {  // Packed 4-state
        لكل (صحيح i = 0; i < الفاظ(); ++i) {
            ثابت قدر_And_X v = m_انکڑے.عدد()[i];
            str += static_cast<حرف>((v.m_قدر >> 0) & 0xff);
            str += static_cast<حرف>((v.m_قدر >> 8) & 0xff);
            str += static_cast<حرف>((v.m_قدر >> 16) & 0xff);
            str += static_cast<حرف>((v.m_قدر >> 24) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 0) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 8) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 16) & 0xff);
            str += static_cast<حرف>((v.m_قدر_X >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'v': {  // Strength
        صحيح ثنائی_ہندسہ_2 = چوڑائی() - 1;
        لكل (; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
            إذا (ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            وإلا إذا (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
                str += "St1 ";
            } وإلا إذا (ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) {
                str += "StZ ";
            } وإلا {
                str += "StX";
            }
        }
        أرجع str;
    }
    افتراضي: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_نمبر::emitC() ثابت VL_MT_STABLE {
    constexpr ناپ_t bufsize = 1000;
    حرف sbuf[bufsize];
    string نتیجہ;
    إذا (isNull()) {
        أرجع "VlNull{}";
    } وإلا إذا (isDouble()) {
        ثابت مزدوج dnum = منزل_Double();
        إذا (std::isinf(dnum)) {
            إذا (std::signbit(dnum)) نتیجہ += '-';
            نتیجہ += "std::numeric_limits<double>::infinity()";
        } وإلا إذا (std::isnan(dnum)) {
            إذا (std::signbit(dnum)) نتیجہ += '-';
            نتیجہ += "std::numeric_limits<double>::quiet_NaN()";
        } وإلا {
            ثابت حرف* ثابت fmt = (static_cast<صحيح>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            أرجع sbuf;
        }
    } وإلا إذا (isString()) {
        ثابت string strg = منزل_String();
        ثابت string quoted
            = V3_باہرلا_Formatter::quoteNameControls(strg, V3_باہرلا_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        أرجع '"' + quoted + "\"s";
    } وإلا إذا (الفاظ() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        نتیجہ = "::VlWide<" + std::منزل_string(الفاظ()) + ">{{";
        إذا (الفاظ() > 4) نتیجہ += '\n';
        لكل (صحيح n = 0; n < الفاظ(); ++n) {
            إذا (n) نتیجہ += ((n % 4) ? ", " : ",\n");
            (فراغ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_لفظ(n));
            نتیجہ += sbuf;
        }
        إذا (الفاظ() > 4) نتیجہ += '\n';
        نتیجہ += "}}";
    } وإلا إذا (الفاظ() == 2) {  // Quad
        ثابت uint64_t qnum = static_cast<uint64_t>(منزل_UQuad());
        ثابت حرف* ثابت fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        أرجع sbuf;
    } وإلا {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ثابت uint32_t unum = منزل_UInt();
        ثابت حرف* ثابت fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (چوڑائی() > 16) ? ("0x%08" PRIx32 "U")
                                : (چوڑائی() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        أرجع sbuf;
    }
    أرجع نتیجہ;
}

string V3_نمبر::منزل_Decimal_S() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (isNegative()) {
        V3_نمبر lhsNoSign = *هذا;
        lhsNoSign.opNegate(*هذا);
        أرجع "-"s + lhsNoSign.منزل_Decimal_U();
    } وإلا {
        أرجع منزل_Decimal_U();
    }
}

string V3_نمبر::منزل_Decimal_U() ثابت VL_MT_STABLE {
    ثابت صحيح maxdecwidth = (چوڑائی() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_نمبر bcd(هذا, maxdecwidth + 4);
    V3_نمبر tmp(هذا, maxdecwidth + 4);
    V3_نمبر tmp2(هذا, maxdecwidth + 4);

    صحيح from_ثنائی_ہندسہ = چوڑائی() - 1;
    // Skip all leading zeros
    لكل (; from_ثنائی_ہندسہ >= 0 && ثنائی_ہندسہ_Is0(from_ثنائی_ہندسہ); --from_ثنائی_ہندسہ) {}
    // Double-dabble algorithm
    لكل (; from_ثنائی_ہندسہ >= 0; --from_ثنائی_ہندسہ) {
        // Any digits >= 5 need an add 3 (via tmp)
        لكل (صحيح nibble_ثنائی_ہندسہ = 0; nibble_ثنائی_ہندسہ < maxdecwidth; nibble_ثنائی_ہندسہ += 4) {
            إذا (bcd.بٹ_قدر(nibble_ثنائی_ہندسہ, 4) >= 5) {
                tmp2.سیٹ_All_Bits0();
                tmp2.سیٹ_ثنائی_ہندسہ(nibble_ثنائی_ہندسہ, 1);  // Add 3, decomposed as two bits
                tmp2.سیٹ_ثنائی_ہندسہ(nibble_ثنائی_ہندسہ + 1, 1);
                tmp.op_سونپو(bcd);
                bcd.op_جوڑو(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_سونپو(bcd);
        bcd.op_کھسکاؤ_L(tmp, V3_نمبر(هذا, 32, 1));
        // bcd[0] = this[from_bit]
        إذا (ثنائی_ہندسہ_Is1(from_ثنائی_ہندسہ)) bcd.سیٹ_ثنائی_ہندسہ(0, 1);
    }

    string output;
    صحيح lsb = (maxdecwidth - 1) & ~3;
    لكل (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        إذا (bcd.بٹ_قدر(lsb, 4)) اكسر;
    }
    لكل (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.بٹ_قدر(lsb, 4));  // 0..9
    }
    أرجع output;
}

string V3_نمبر::منزل_Real_String() ثابت VL_MT_STABLE {
    ثابت مزدوج قدر = منزل_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream باہرلا;
    باہرلا.precision(15);
    باہرلا << قدر;
    string نتیجہ = باہرلا.str();
    إذا (std::strtod(نتیجہ.c_str(), خالی_سوچک) != قدر) {
        باہرلا.str("");
        باہرلا.precision(17);
        باہرلا << قدر;
        نتیجہ = باہرلا.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    إذا (نتیجہ.لبھو_پہلا_دا(".en") == string::npos) نتیجہ += ".0";
    أرجع نتیجہ;
}

//======================================================================
// ACCESSORS - as numbers

منطقي V3_نمبر::سماندا_وچ_UInt() ثابت VL_MT_محفوظ {
    إذا (isFourState()) أرجع خطأ;
    // We allow wide numbers that represent values <= 32 bits
    لكل (صحيح i = 1; i < الفاظ(); ++i) {
        إذا (m_انکڑے.عدد()[i].m_قدر) أرجع خطأ;
    }
    أرجع سچ;
}
uint32_t V3_نمبر::منزل_UInt() ثابت VL_MT_محفوظ {
    UASSERT(!isFourState(), "toUInt with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 32 bits
    إذا (VL_UNCOVERABLE(!سماندا_وچ_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *هذا);
    }
    أرجع m_انکڑے.عدد()[0].m_قدر;
}

مزدوج V3_نمبر::منزل_Double() ثابت VL_MT_محفوظ {
    UASSERT(isDouble() && چوڑائی() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.u[0] = m_انکڑے.عدد()[0].m_قدر;
    u.u[1] = m_انکڑے.عدد()[1].m_قدر;
    أرجع u.d;
}

int32_t V3_نمبر::منزل_SInt() ثابت VL_MT_محفوظ {
    إذا (isSigned()) {
        ثابت uint32_t v = منزل_UInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (چوڑائی() - 1))));
        ثابت uint32_t extended = v | signExtend;
        أرجع static_cast<int32_t>(extended);
    } وإلا {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        أرجع static_cast<int32_t>(منزل_UInt());
    }
}

uint64_t V3_نمبر::منزل_UQuad() ثابت VL_MT_محفوظ {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 64 bits
    إذا (isDouble()) أرجع static_cast<uint64_t>(منزل_Double());
    لكل (صحيح i = 2; i < الفاظ(); ++i) {
        إذا (m_انکڑے.عدد()[i].m_قدر) {
            v3error("Value too wide for 64-bits expected in this context " << *هذا);
            اكسر;
        }
    }
    إذا (چوڑائی() <= 32) أرجع (static_cast<uint64_t>(منزل_UInt()));
    أرجع ((static_cast<uint64_t>(m_انکڑے.عدد()[1].m_قدر) << 32ULL)
            | (static_cast<uint64_t>(m_انکڑے.عدد()[0].m_قدر)));
}

int64_t V3_نمبر::منزل_SQuad() ثابت {
    إذا (isDouble()) أرجع static_cast<int64_t>(منزل_Double());
    ثابت uint64_t v = منزل_UQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (چوڑائی() - 1))));
    ثابت uint64_t extended = v | signExtend;
    أرجع static_cast<int64_t>(extended);
}

string V3_نمبر::منزل_String() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *هذا);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    إذا (isString()) أرجع m_انکڑے.str();
    صحيح ثنائی_ہندسہ_2 = چوڑائی() - 1;
    طالما ((ثنائی_ہندسہ_2 % 8) != 7) ++ثنائی_ہندسہ_2;
    std::string str;
    لكل (; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2 -= 8) {
        ثابت صحيح v = بٹ_قدر(ثنائی_ہندسہ_2 - 7, 8);
        إذا (v) str += static_cast<حرف>(v);
    }
    أرجع str;
}

uint32_t V3_نمبر::edata_لفظ(صحيح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *هذا);
    أرجع m_انکڑے.عدد()[eword].m_قدر;
}

uint8_t V3_نمبر::انکڑے_Byte(صحيح byte) ثابت {
    أرجع (edata_لفظ(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقي V3_نمبر::isAllZ() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        ثابت uint32_t نقاب = i == الفاظ() - 1 ? hi_لفظ_نقاب() : ~0U;
        إذا (((~v.m_قدر & v.m_قدر_X) & نقاب) != نقاب) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_نمبر::isAllX() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    uint32_t نقاب = hi_لفظ_نقاب();
    لكل (صحيح i = الفاظ() - 1; i >= 0; --i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        إذا ((v.m_قدر & v.m_قدر_X) ^ نقاب) أرجع خطأ;
        نقاب = ~0U;
    }
    أرجع سچ;
}
منطقي V3_نمبر::isEqZero() ثابت VL_MT_محفوظ {
    إذا (isString()) أرجع m_انکڑے.str().خالی_تھاواں();
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        إذا (v.m_قدر || v.m_قدر_X) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_نمبر::isNeqZero() ثابت {
    إذا (isString()) أرجع !m_انکڑے.str().خالی_تھاواں();
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        إذا (v.m_قدر & ~v.m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_نمبر::is_بٹ_Zero(صحيح msb, صحيح lsb) ثابت {
    لكل (صحيح i = lsb; i <= msb; ++i) {
        إذا (VL_UNLIKELY(!ثنائی_ہندسہ_Is0(i))) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_نمبر::isEqOne() ثابت {
    إذا (m_انکڑے.عدد()[0].m_قدر != 1 || m_انکڑے.عدد()[0].m_قدر_X) أرجع خطأ;
    لكل (صحيح i = 1; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        إذا (v.m_قدر || v.m_قدر_X) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_نمبر::isEqAllOnes(صحيح optwidth) ثابت {
    // Correct number of zero bits/width matters
    إذا (!optwidth) optwidth = چوڑائی();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < optwidth; ++ثنائی_ہندسہ_2) {
        إذا (!ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_نمبر::isFourState() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        إذا (m_انکڑے.عدد()[i].m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_نمبر::isAnyX() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        إذا (v.m_قدر & v.m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_نمبر::isAnyXZ() ثابت {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        إذا (m_انکڑے.عدد()[i].m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_نمبر::isAnyZ() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        ثابت قدر_And_X v = m_انکڑے.عدد()[i];
        إذا (~v.m_قدر & v.m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_نمبر::isLtXZ(ثابت V3_نمبر& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    إذا (isString()) أرجع منزل_String() < rhs.منزل_String();
    إذا (isDouble()) أرجع منزل_Double() < rhs.منزل_Double();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) أرجع سچ;
        إذا (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) أرجع خطأ;
        إذا (ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) أرجع سچ;
        إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) أرجع خطأ;
    }
    أرجع خطأ;
}
صحيح V3_نمبر::گنتی_X(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ {
    صحيح گنتی = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= چوڑائی()) أرجع گنتی;
        إذا (ثنائی_ہندسہ_Is_X(lsb + bitn)) ++گنتی;
    }
    أرجع گنتی;
}
صحيح V3_نمبر::گنتی_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ {
    صحيح گنتی = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= چوڑائی()) أرجع گنتی;
        إذا (ثنائی_ہندسہ_Is_Z(lsb + bitn)) ++گنتی;
    }
    أرجع گنتی;
}

صحيح V3_نمبر::چوڑائی_منزل_Fit() ثابت {
    لكل (صحيح ثنائی_ہندسہ_2 = چوڑائی() - 1; ثنائی_ہندسہ_2 > 0; ثنائی_ہندسہ_2--) {
        إذا (!ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) أرجع ثنائی_ہندسہ_2 + 1;
    }
    أرجع 1;  // one bit even if number is == 0
}

uint32_t V3_نمبر::گنتی_بٹ(ثابت V3_نمبر& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        بدل (ctrl.ثنائی_ہندسہ_Is(0)) {
        حالة '0':
            إذا (ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) ++n;
            اكسر;
        حالة '1':
            إذا (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) ++n;
            اكسر;
        حالة 'x':
            إذا (ثنائی_ہندسہ_Is_X(ثنائی_ہندسہ_2)) ++n;
            اكسر;
        حالة 'z':
            إذا (ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) ++n;
            اكسر;
        }
    }
    أرجع n;
}

uint32_t V3_نمبر::گنتی_بٹ(ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2,
                             ثابت V3_نمبر& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = گنتی_بٹ(ctrl1);
    إذا (ctrl2.ثنائی_ہندسہ_Is(0) != ctrl1.ثنائی_ہندسہ_Is(0)) n += گنتی_بٹ(ctrl2);
    إذا ((ctrl3.ثنائی_ہندسہ_Is(0) != ctrl1.ثنائی_ہندسہ_Is(0)) && (ctrl3.ثنائی_ہندسہ_Is(0) != ctrl2.ثنائی_ہندسہ_Is(0))) {
        n += گنتی_بٹ(ctrl3);
    }
    أرجع n;
}

uint32_t V3_نمبر::گنتی_Ones() ثابت {
    صحيح n = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) ++n;
    }
    أرجع n;
}

uint32_t V3_نمبر::most_سیٹ_ثنائی_ہندسہ_P1() ثابت {
    لكل (صحيح ثنائی_ہندسہ_2 = چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        إذا (!ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) أرجع ثنائی_ہندسہ_2 + 1;
    }
    أرجع 0;
}

uint32_t V3_نمبر::گھٹ_توں_گھٹ_سیٹ_ثنائی_ہندسہ_P1() ثابت {
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (!ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) أرجع ثنائی_ہندسہ_2 + 1;
    }
    أرجع 0;
}

//======================================================================

V3_نمبر& V3_نمبر::op_بٹ_Non_XZ(ثابت V3_نمبر& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) || lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_بٹ_One(ثابت V3_نمبر& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_بٹ_XZ(ثابت V3_نمبر& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_بٹ_Z(ثابت V3_نمبر& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is_Z(ثنائی_ہندسہ_2)) سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
    }
    أرجع *هذا;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_نمبر& V3_نمبر::opRedOr(ثابت V3_نمبر& lhs) {
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            أرجع سیٹ_Single_بٹ(1);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opRedAnd(ثابت V3_نمبر& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            أرجع سیٹ_Single_بٹ(0);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opRedXor(ثابت V3_نمبر& lhs) {
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            إذا (outc == 1) {
                outc = 0;
            } وإلا إذا (outc == 0) {
                outc = 1;
            }
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::op_گنتی_بٹ(ثابت V3_نمبر& اظہاریہ, ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2,
                                ثابت V3_نمبر& ctrl3) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS4(اظہاریہ, ctrl1, ctrl2, ctrl3);
    عدد_ASSERT_LOGIC_ARGS4(اظہاریہ, ctrl1, ctrl2, ctrl3);
    سیٹ_Zero();
    m_انکڑے.عدد()[0].m_قدر = اظہاریہ.گنتی_بٹ(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_گنتی_Ones(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    سیٹ_Zero();
    m_انکڑے.عدد()[0].m_قدر = lhs.گنتی_Ones();
    opCleanThis();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opIsUnknown(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    أرجع سیٹ_Single_بٹ(lhs.isAnyXZ());
}
V3_نمبر& V3_نمبر::opOneHot(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    أرجع سیٹ_Single_بٹ(lhs.گنتی_Ones() == 1);
}
V3_نمبر& V3_نمبر::opOneHot0(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    أرجع سیٹ_Single_بٹ(lhs.گنتی_Ones() <= 1);
}
V3_نمبر& V3_نمبر::opCLog2(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحيح adjust = (lhs.گنتی_Ones() == 1) ? 0 : 1;
    لكل (صحيح ثنائی_ہندسہ_2 = lhs.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_Long(ثنائی_ہندسہ_2 + adjust);
            أرجع *هذا;
        }
    }
    سیٹ_Zero();
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_Most_سیٹ_ثنائی_ہندسہ_P1(ثابت V3_نمبر& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    لكل (صحيح ثنائی_ہندسہ_2 = lhs.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_Long(ثنائی_ہندسہ_2 + 1);
            أرجع *هذا;
        }
    }
    سیٹ_Zero();
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opLogNot(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 0;
            اذهب آخری;
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opNot(ثابت V3_نمبر& lhs) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opAnd(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) || rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {  // 0
        } وإلا {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opOr(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) || rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            // 0
        } وإلا {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opXor(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 1);
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) || rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'x');
        }
        // else zero
    }
    أرجع *هذا;
}

فراغ V3_نمبر::fill_بٹ(صحيح destLsb, صحيح چوڑائی, حرف قدر) {
    UASSERT(destLsb >= 0 && چوڑائی >= 0, "Negative bit range");
    إذا (!چوڑائی) أرجع;
    UASSERT(چوڑائی <= هذا->چوڑائی() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t قدر_لفظ = قدر == '1' || قدر == 'x' || قدر == 1 || قدر == 3 ? ~0U : 0;
    ثابت uint32_t x_لفظ = قدر == 'z' || قدر == 'x' || قدر == 2 || قدر == 3 ? ~0U : 0;
    ثابت صحيح پہلا_لفظ = VL_BITWORD_E(destLsb);
    ثابت صحيح آخری_لفظ = VL_BITWORD_E(destLsb + چوڑائی - 1);
    لكل (صحيح لفظ = پہلا_لفظ; لفظ <= آخری_لفظ; ++لفظ) {
        ثابت صحيح لفظ_Lsb = لفظ == پہلا_لفظ ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحيح لفظ_Msb = لفظ == آخری_لفظ ? VL_BITBIT_E(destLsb + چوڑائی - 1) : VL_SIZEBITS_E;
        ثابت uint32_t نقاب = VL_نقاب_E(لفظ_Msb - لفظ_Lsb + 1) << لفظ_Lsb;
        قدر_And_X& dest = m_انکڑے.عدد()[لفظ];
        dest.m_قدر = (dest.m_قدر & ~نقاب) | (قدر_لفظ & نقاب);
        dest.m_قدر_X = (dest.m_قدر_X & ~نقاب) | (x_لفظ & نقاب);
    }
}

فراغ V3_نمبر::copy_بٹ(صحيح destLsb, ثابت V3_نمبر& سروت, صحيح سروت_Lsb, صحيح چوڑائی) {
    UASSERT(destLsb >= 0 && سروت_Lsb >= 0 && چوڑائی >= 0, "Negative bit range");
    إذا (!چوڑائی) أرجع;
    UASSERT(چوڑائی <= هذا->چوڑائی() - destLsb, "Destination bit range exceeds number width");
    UASSERT(چوڑائی <= سروت.چوڑائی() - سروت_Lsb, "Source bit range exceeds number width");

    ثابت صحيح پہلا_لفظ = VL_BITWORD_E(destLsb);
    ثابت صحيح آخری_لفظ = VL_BITWORD_E(destLsb + چوڑائی - 1);
    لكل (صحيح dest_لفظ = پہلا_لفظ; dest_لفظ <= آخری_لفظ; ++dest_لفظ) {
        ثابت صحيح dest_ثنائی_ہندسہ = std::max(destLsb, dest_لفظ * VL_EDATASIZE);
        ثابت صحيح dest_وتھ = VL_BITBIT_E(dest_ثنائی_ہندسہ);
        ثابت صحيح سروت_ثنائی_ہندسہ = سروت_Lsb + dest_ثنائی_ہندسہ - destLsb;
        ثابت صحيح سروت_لفظ = VL_BITWORD_E(سروت_ثنائی_ہندسہ);
        ثابت صحيح سروت_وتھ = VL_BITBIT_E(سروت_ثنائی_ہندسہ);
        ثابت صحيح copy_چوڑائی = std::min(چوڑائی - (dest_ثنائی_ہندسہ - destLsb), VL_EDATASIZE - dest_وتھ);
        ثابت uint32_t نقاب = VL_نقاب_E(copy_چوڑائی) << dest_وتھ;

        ثابت قدر_And_X& سروت_Lo = سروت.m_انکڑے.عدد()[سروت_لفظ];
        uint32_t قدر = سروت_Lo.m_قدر >> سروت_وتھ;
        uint32_t قدر_X = سروت_Lo.m_قدر_X >> سروت_وتھ;
        إذا (سروت_وتھ && copy_چوڑائی > VL_EDATASIZE - سروت_وتھ) {
            ثابت قدر_And_X& سروت_Hi = سروت.m_انکڑے.عدد()[سروت_لفظ + 1];
            قدر |= سروت_Hi.m_قدر << (VL_EDATASIZE - سروت_وتھ);
            قدر_X |= سروت_Hi.m_قدر_X << (VL_EDATASIZE - سروت_وتھ);
        }

        قدر_And_X& dest = m_انکڑے.عدد()[dest_لفظ];
        dest.m_قدر = (dest.m_قدر & ~نقاب) | ((قدر << dest_وتھ) & نقاب);
        dest.m_قدر_X = (dest.m_قدر_X & ~نقاب) | ((قدر_X << dest_وتھ) & نقاب);
    }
}

V3_نمبر& V3_نمبر::opConcat(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    // See also error in V3Width
    إذا (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_بٹ(0, rhs, 0, rhs.چوڑائی());
    copy_بٹ(rhs.چوڑائی(), lhs, 0, lhs.چوڑائی());
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opLenN(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    سیٹ_Quad(lhs.منزل_String().لمائی());
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opRepl(ثابت V3_نمبر& lhs,
                           ثابت V3_نمبر& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    أرجع opRepl(lhs, rhs.منزل_UInt());
}

V3_نمبر& V3_نمبر::opRepl(ثابت V3_نمبر& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (v3Global.opt.replication_حد_بندی()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_حد_بندی())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_حد_بندی() << " is suspect: " << rhsval);
    }
    سیٹ_Zero();
    لكل (بنانشانی times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.چوڑائی();
        إذا (destLsb >= static_cast<uint32_t>(چوڑائی())) اكسر;
        copy_بٹ(static_cast<صحيح>(destLsb), lhs, 0,
                 std::min(lhs.چوڑائی(), چوڑائی() - static_cast<صحيح>(destLsb)));
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::op_دھارا_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    // See also error in V3Width
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحيح ssize = std::min(rhs.منزل_UInt(), static_cast<بنانشانی>(lhs.چوڑائی()));
    لكل (صحيح istart = 0; istart < lhs.چوڑائی(); istart += ssize) {
        ثابت صحيح ostart = std::max(0, lhs.چوڑائی() - ssize - istart);
        copy_بٹ(ostart, lhs, istart, std::min(ssize, lhs.چوڑائی() - istart));
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opLogAnd(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            loutc = 1;
            اكسر;
        }
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && loutc == 0) loutc = 'x';
    }
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < rhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            routc = 1;
            اكسر;
        }
        إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    إذا (routc == 1 && loutc == 1) outc = 1;
    إذا (routc == 0 || loutc == 0) outc = 0;
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opLogOr(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < lhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 1;
            اذهب آخری;
        }
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && outc == 0) outc = 'x';
    }
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < rhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 1;
            اذهب آخری;
        }
        إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2) && outc == 0) outc = 'x';
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opLogIf(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر lnot = lhs;
    lnot.opLogNot(lhs);
    أرجع opLogOr(lnot, rhs);
}

V3_نمبر& V3_نمبر::opLogEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_نمبر ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    أرجع opLogAnd(ifa, ifb);
}

V3_نمبر& V3_نمبر::opAtoN(ثابت V3_نمبر& lhs, صحيح ادھار) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    UASSERT(ادھار == AstAtoN::ATOREAL || ادھار == 2 || ادھار == 8 || ادھار == 10 || ادھار == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.منزل_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.سرا(), '_'), str.سرا());

    إذا (ادھار == AstAtoN::ATOREAL) أرجع سیٹ_Double(std::atof(str.c_str()));

    errno = 0;
    خودکار v = std::strtol(str.c_str(), خالی_سوچک, ادھار);
    إذا (errno != 0) v = 0;
    أرجع سیٹ_Long_S(static_cast<int32_t>(v));
}

V3_نمبر& V3_نمبر::opPutcN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, ثابت V3_نمبر& ths) {
    عدد_ASSERT_OP_ARGS3(lhs, rhs, ths);
    عدد_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    ثابت int32_t c = ths.منزل_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.لمائی() && c != 0) lstring[i] = c;
    أرجع سیٹ_String(lstring);
}

V3_نمبر& V3_نمبر::opGetcN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.لمائی()) v = lstring[i];
    أرجع سیٹ_Long(v);
}

V3_نمبر& V3_نمبر::opSubstrN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, ثابت V3_نمبر& ths) {
    عدد_ASSERT_OP_ARGS3(lhs, rhs, ths);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    ثابت int32_t j = ths.منزل_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    إذا (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.لمائی()) أرجع سیٹ_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    أرجع سیٹ_String(lstring.substr(i, j - i + 1));
}

V3_نمبر& V3_نمبر::opCompareNN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, منطقي ignoreCase) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحيح نتیجہ;
    ثابت string lstring = lhs.منزل_String();
    ثابت string rstring = rhs.منزل_String();
    إذا (ignoreCase) {
        نتیجہ = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } وإلا {
        نتیجہ = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    أرجع سیٹ_Long_S(نتیجہ);
}

V3_نمبر& V3_نمبر::opEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opEqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opEqD(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            outc = 0;
            اذهب آخری;
        }
        إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 0;
            اذهب آخری;
        }
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
        إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opNeqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opNeqD(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) {
            outc = 1;
            اذهب آخری;
        }
        إذا (lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            outc = 1;
            اذهب آخری;
        }
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
        إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3Hash V3_نمبر::منزل_Hash() ثابت {
    // Shouldmatch isCaseEq
    إذا (isString()) أرجع V3Hash{منزل_String()};
    إذا (isDouble()) أرجع V3Hash{منزل_Double()};
    V3Hash hash{چوڑائی()};
    لكل (صحيح i = 0; i < الفاظ(); ++i) hash += m_انکڑے.عدد()[i].m_قدر;
    أرجع hash;
}

منطقي V3_نمبر::isCaseEq(ثابت V3_نمبر& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    إذا (isString()) أرجع منزل_String() == rhs.منزل_String();
    إذا (isDouble()) أرجع منزل_Double() == rhs.منزل_Double();
    إذا (چوڑائی() != rhs.چوڑائی()) أرجع خطأ;
    لكل (صحيح i = 0; i < الفاظ(); ++i) {
        إذا (!(m_انکڑے.عدد()[i] == rhs.m_انکڑے.عدد()[i])) أرجع خطأ;
    }
    أرجع سچ;
}

V3_نمبر& V3_نمبر::opCaseEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_نمبر& V3_نمبر::opCaseNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    إذا (lhs.isString()) {
        أرجع opNeqN(lhs, rhs);
    } وإلا إذا (lhs.isDouble()) {
        أرجع opNeqD(lhs, rhs);
    }
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2) != rhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2)) {
            outc = 1;
            اذهب آخری;
        }
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opWildEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (!rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
                outc = 'x';
                اذهب آخری;
            }
            إذا (lhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2) != rhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2)) {
                outc = 0;
                اذهب آخری;
            }
        }
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opWildNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (!rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
            إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) {
                outc = 'x';
                اذهب آخری;
            }
            إذا (lhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2) != rhs.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2)) {
                outc = 1;
                اذهب آخری;
            }
        }
    }
آخری:
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opGt(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی(), rhs.چوڑائی()); ++ثنائی_ہندسہ_2) {
        إذا (lhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 1;
        إذا (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2) && lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 0;
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
        إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
    }
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opGtS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحيح mbit = std::max(lhs.چوڑائی() - 1, rhs.چوڑائی() - 1);
        إذا (lhs.ثنائی_ہندسہ_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (rhs.ثنائی_ہندسہ_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is0(mbit) && rhs.ثنائی_ہندسہ_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } وإلا إذا (lhs.ثنائی_ہندسہ_Is1_Extend(mbit) && rhs.ثنائی_ہندسہ_Is0(mbit)) {
            outc = 0;  // - !> +
        } وإلا {
            // both positive or negative, normal >
            لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < std::max(lhs.چوڑائی() - 1, rhs.چوڑائی() - 1); ++ثنائی_ہندسہ_2) {
                إذا (lhs.ثنائی_ہندسہ_Is1_Extend(ثنائی_ہندسہ_2) && rhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 1;
                إذا (rhs.ثنائی_ہندسہ_Is1_Extend(ثنائی_ہندسہ_2) && lhs.ثنائی_ہندسہ_Is0(ثنائی_ہندسہ_2)) outc = 0;
                إذا (lhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
                إذا (rhs.ثنائی_ہندسہ_Is_XZ(ثنائی_ہندسہ_2)) outc = 'x';
            }
        }
    }
    أرجع سیٹ_Single_بٹ(outc);
}

V3_نمبر& V3_نمبر::opGte(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGt(lhs, rhs);
}

V3_نمبر& V3_نمبر::opGteS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_نمبر& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGtS(lhs, rhs);
}

V3_نمبر& V3_نمبر::opLt(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { أرجع opGt(rhs, lhs); }
V3_نمبر& V3_نمبر::opLte(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { أرجع opGte(rhs, lhs); }
V3_نمبر& V3_نمبر::opLtS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { أرجع opGtS(rhs, lhs); }
V3_نمبر& V3_نمبر::opLteS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) { أرجع opGteS(rhs, lhs); }

V3_نمبر& V3_نمبر::op_کھسکاؤ_R(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // L(lhs) bit return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    سیٹ_Zero();
    إذا (rhs.چوڑائی() > 32 && !rhs.is_بٹ_Zero(rhs.چوڑائی() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.منزل_UInt();
    إذا (rhsval < static_cast<uint32_t>(lhs.چوڑائی())) {
        copy_بٹ(0, lhs, static_cast<صحيح>(rhsval),
                 std::min(چوڑائی(), lhs.چوڑائی() - static_cast<صحيح>(rhsval)));
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::op_کھسکاؤ_RS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    سیٹ_Zero();
    ثابت منطقي حد_توں_ودھ = rhs.چوڑائی() > 32 && !rhs.is_بٹ_Zero(rhs.چوڑائی() - 1, 32);
    إذا (!حد_توں_ودھ) {
        ثابت uint32_t rhsval = rhs.منزل_UInt();
        إذا (rhsval < lbits) {
            ثابت صحيح copy_چوڑائی = std::min(چوڑائی(), static_cast<صحيح>(lbits - rhsval));
            copy_بٹ(0, lhs, static_cast<صحيح>(rhsval), copy_چوڑائی);
            fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, lhs.ثنائی_ہندسہ_Is(lbits - 1));
            أرجع *هذا;
        }
    }
    fill_بٹ(0, چوڑائی(), lhs.ثنائی_ہندسہ_Is(lbits - 1));  // '0/'1/'x/'z
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::op_کھسکاؤ_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // L(lhs) bit return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    سیٹ_Zero();
    إذا (rhs.چوڑائی() > 32 && !rhs.is_بٹ_Zero(rhs.چوڑائی() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.منزل_UInt();
    إذا (rhsval < static_cast<uint32_t>(چوڑائی())) {
        copy_بٹ(static_cast<صحيح>(rhsval), lhs, 0,
                 std::min(چوڑائی() - static_cast<صحيح>(rhsval), lhs.چوڑائی()));
    }
    أرجع *هذا;
}

//======================================================================
// Ops - Arithmetic

V3_نمبر& V3_نمبر::opNegate(ثابت V3_نمبر& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    V3_نمبر notlhs(&lhs, چوڑائی());
    notlhs.opNot(lhs);
    ثابت V3_نمبر one(&lhs, چوڑائی(), 1);
    op_جوڑو(notlhs, one);
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_جوڑو(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    سیٹ_Zero();
    // Addem
    uint64_t حاصل = 0;
    لكل (صحيح لفظ = 0; لفظ < الفاظ(); ++لفظ) {
        ثابت uint64_t lwordval = lhs.m_انکڑے.عدد()[لفظ].m_قدر;
        ثابت uint64_t rwordval = rhs.m_انکڑے.عدد()[لفظ].m_قدر;
        ثابت uint64_t جوڑ = lwordval + rwordval + حاصل;
        m_انکڑے.عدد()[لفظ].m_قدر = جوڑ & 0xffffffffULL;
        حاصل = جوڑ > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opSub(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    V3_نمبر negrhs(&rhs, چوڑائی());
    negrhs.opNegate(rhs);
    أرجع op_جوڑو(lhs, negrhs);
}
V3_نمبر& V3_نمبر::opMul(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    سیٹ_Zero();
    إذا (چوڑائی() <= 64) {
        سیٹ_Quad(lhs.منزل_UQuad() * rhs.منزل_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } وإلا {
        لكل (صحيح lword = 0; lword < lhs.الفاظ(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_انکڑے.عدد()[lword].m_قدر);
            إذا (lwordval == 0) استمر;
            لكل (صحيح rword = 0; rword < rhs.الفاظ(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_انکڑے.عدد()[rword].m_قدر);
                إذا (rwordval == 0) استمر;
                uint64_t mul = lwordval * rwordval;
                لكل (صحيح qword = lword + rword; qword < الفاظ(); ++qword) {
                    mul += static_cast<uint64_t>(m_انکڑے.عدد()[qword].m_قدر);
                    m_انکڑے.عدد()[qword].m_قدر = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    إذا (mul == 0) اكسر;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opMulS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    V3_نمبر lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_نمبر rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_نمبر qNoSign = opMul(lhsNoSign, rhsNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_سونپو(qNoSign);
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opDiv(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    إذا (rhs.isEqZero()) أرجع سیٹ_All_بٹ_XRemoved();
    إذا (lhs.چوڑائی() <= 64) {
        سیٹ_Quad(lhs.منزل_UQuad() / rhs.منزل_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide division
        أرجع opModDivGuts(lhs, rhs, خطأ);
    }
}
V3_نمبر& V3_نمبر::opDivS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    إذا (rhs.isEqZero()) أرجع سیٹ_All_بٹ_XRemoved();
    V3_نمبر lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_نمبر rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_نمبر qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_سونپو(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *هذا);
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opModDiv(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    إذا (rhs.isEqZero()) أرجع سیٹ_All_بٹ_XRemoved();
    إذا (lhs.چوڑائی() <= 64) {
        سیٹ_Quad(lhs.منزل_UQuad() % rhs.منزل_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide modulus
        أرجع opModDivGuts(lhs, rhs, سچ);
    }
}
V3_نمبر& V3_نمبر::opModDivS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    إذا (rhs.isEqZero()) أرجع سیٹ_All_بٹ_XRemoved();
    V3_نمبر lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_نمبر rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_نمبر qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    إذا (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } وإلا {
        op_سونپو(qNoSign);
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opModDivGuts(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, منطقي is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سیٹ_Zero();
    // Find MSB and check for zero.
    ثابت صحيح l_الفاظ = lhs.الفاظ();
    ثابت صحيح umsbp1 = lhs.most_سیٹ_ثنائی_ہندسہ_P1();  // dividend
    ثابت صحيح vmsbp1 = rhs.most_سیٹ_ثنائی_ہندسہ_P1();  //مقسوم علیہ
    إذا (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    }

    ثابت صحيح uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ثابت صحيح vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    إذا (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        لكل (صحيح j = uw - 1; j >= 0; j--) {
            ثابت uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_انکڑے.عدد()[j].m_قدر));
            m_انکڑے.عدد()[j].m_قدر = unw64 / static_cast<uint64_t>(rhs.m_انکڑے.عدد()[0].m_قدر);
            k = unw64
                - (static_cast<uint64_t>(m_انکڑے.عدد()[j].m_قدر)
                   * static_cast<uint64_t>(rhs.m_انکڑے.عدد()[0].m_قدر));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *هذا << " rem=0x" << std::hex
                                   << k << std::dec);
        إذا (is_modulus) {
            سیٹ_Zero();
            m_انکڑے.عدد()[0].m_قدر = k;
        }
        opCleanThis();
        أرجع *هذا;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_الفاظ + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_الفاظ + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    لكل (صحيح i = 0; i < l_الفاظ; ++i) m_انکڑے.عدد()[i].m_قدر = 0;
    لكل (صحيح i = 0; i < l_الفاظ + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحيح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t کھسکاؤ_نقاب = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    لكل (صحيح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_انکڑے.عدد()[i].m_قدر << s)
                | (کھسکاؤ_نقاب & (rhs.m_انکڑے.عدد()[i - 1].m_قدر >> (32 - s)));
    }
    vn[0] = rhs.m_انکڑے.عدد()[0].m_قدر << s;

    // Copy and shift dividend by same amount; may set new upper word
    إذا (s) {
        un[uw] = lhs.m_انکڑے.عدد()[uw - 1].m_قدر >> (32 - s);
    } وإلا {
        un[uw] = 0;
    }
    لكل (صحيح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_انکڑے.عدد()[i].m_قدر << s)
                | (کھسکاؤ_نقاب & (lhs.m_انکڑے.عدد()[i - 1].m_قدر >> (32 - s)));
    }
    un[0] = lhs.m_انکڑے.عدد()[0].m_قدر << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    لكل (صحيح j = uw - vw; j >= 0; j--) {
        // Estimate
        ثابت uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        إذا (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            إذا (rhat < 0x100000000ULL) اذهب again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        لكل (صحيح i = 0; i < vw; ++i) {
            ثابت uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //گھٹاؤ
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_انکڑے.عدد()[j].m_قدر = qhat;  // Save quotient digit

        إذا (t < 0) {
            // Over subtracted; correct by adding back
            m_انکڑے.عدد()[j].m_قدر--;
            k = 0;
            لكل (صحيح i = 0; i < vw; ++i) {
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

    إذا (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        لكل (صحيح i = 0; i < vw; ++i) {
            m_انکڑے.عدد()[i].m_قدر = (un[i] >> s) | (کھسکاؤ_نقاب & (un[i + 1] << (32 - s)));
        }
        لكل (صحيح i = vw; i < l_الفاظ; ++i) m_انکڑے.عدد()[i].m_قدر = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    } وإلا {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    }
}

V3_نمبر& V3_نمبر::opPow(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, منطقي lsign, منطقي rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سیٹ_All_بٹ_X();
    إذا (rhs.isEqZero()) أرجع سیٹ_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    إذا (rsign && rhs.isNegative()) {
        إذا (lhs.isEqZero()) {
            أرجع سیٹ_All_بٹ_XRemoved();
        } وإلا إذا (lhs.isEqOne()) {
            أرجع سیٹ_Value1();
        } وإلا إذا (lsign && lhs.isEqAllOnes()) {
            إذا (rhs.ثنائی_ہندسہ_Is1(0)) {
                أرجع سیٹ_All_Bits1();  // -1^odd=-1
            } وإلا {
                أرجع سیٹ_Value1();  // -1^even=1
            }
        }
        أرجع سیٹ_Zero();
    }
    إذا (lhs.isEqZero()) أرجع سیٹ_Zero();
    سیٹ_Zero();
    m_انکڑے.عدد()[0].m_قدر = 1;
    V3_نمبر power(&lhs, چوڑائی());
    power.op_سونپو(lhs);
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < rhs.چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (ثنائی_ہندسہ_2 > 0) {  // power = power*power
            V3_نمبر آخری_Power(&lhs, چوڑائی());
            آخری_Power.op_سونپو(power);
            power.opMul(آخری_Power, آخری_Power);
        }
        إذا (rhs.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {  // out *= power
            V3_نمبر آخری_باہرلا(&lhs, چوڑائی());
            آخری_باہرلا.op_سونپو(*هذا);
            opMul(آخری_باہرلا, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::opPowSU(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, سچ, خطأ);
}
V3_نمبر& V3_نمبر::opPowSS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, سچ, سچ);
}
V3_نمبر& V3_نمبر::opPowUS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, خطأ, سچ);
}

V3_نمبر& V3_نمبر::opBufIf1(ثابت V3_نمبر& ens, ثابت V3_نمبر& if1s) {
    عدد_ASSERT_OP_ARGS2(ens, if1s);
    عدد_ASSERT_LOGIC_ARGS2(ens, if1s);
    سیٹ_Zero();
    لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 < چوڑائی(); ++ثنائی_ہندسہ_2) {
        إذا (ens.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, if1s.ثنائی_ہندسہ_Is(ثنائی_ہندسہ_2));
        } وإلا {
            سیٹ_ثنائی_ہندسہ(ثنائی_ہندسہ_2, 'z');
        }
    }
    أرجع *هذا;
}

// Sets all bits in range to the given value
V3_نمبر& V3_نمبر::op_سیٹ_Range(uint32_t lsb, uint32_t چوڑائی, حرف ثنائی_ہندسہ_قدر) {
    إذا (lsb < static_cast<uint32_t>(هذا->چوڑائی())) {
        fill_بٹ(static_cast<صحيح>(lsb),
                 static_cast<صحيح>(std::min(چوڑائی, static_cast<uint32_t>(هذا->چوڑائی()) - lsb)),
                 ثنائی_ہندسہ_قدر);
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::op_سونپو(ثابت V3_نمبر& lhs) { أرجع op_سونپو_Non_XZ(lhs, خطأ); }
V3_نمبر& V3_نمبر::op_سونپو_Non_XZ(ثابت V3_نمبر& lhs, منطقي ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    إذا (هذا != &lhs) {
        إذا (VL_UNLIKELY(lhs.isNull())) {
            m_انکڑے.m_isNull = سچ;
        } وإلا إذا (isString()) {
            إذا (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_انکڑے.str() = lhs.منزل_String();
            } وإلا {
                m_انکڑے.str() = lhs.m_انکڑے.str();
            }
        } وإلا إذا (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            سیٹ_Zero();
        } وإلا إذا (lhs.isDouble()) {
            سیٹ_Double(lhs.منزل_Double());
        } وإلا إذا (!ignoreXZ) {
            سیٹ_Zero();
            copy_بٹ(0, lhs, 0, std::min(چوڑائی(), lhs.چوڑائی()));
        } وإلا {
            لكل (صحيح لفظ = 0; لفظ < الفاظ(); ++لفظ) {
                ثابت uint32_t قدر = لفظ < lhs.الفاظ() ? lhs.m_انکڑے.عدد()[لفظ].m_قدر
                                                                & ~lhs.m_انکڑے.عدد()[لفظ].m_قدر_X
                                                          : 0;
                m_انکڑے.عدد()[لفظ] = {قدر, 0};
            }
            opCleanThis();
        }
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opNToI(ثابت V3_نمبر& lhs) {
    // String to packed number
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    سیٹ_Zero();
    ثابت string& str = lhs.منزل_String();
    لكل (ناپ_t n = 0; n < str.لمائی(); ++n) {
        ثابت حرف c = str[str.لمائی() - 1 - n];
        لكل (ناپ_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<بنانشانی لما لما>(c)};
            سیٹ_ثنائی_ہندسہ(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opExtendS(ثابت V3_نمبر& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    ثابت صحيح copy_چوڑائی = std::min(چوڑائی(), static_cast<صحيح>(lbits));
    copy_بٹ(0, lhs, 0, copy_چوڑائی);
    fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, lhs.ثنائی_ہندسہ_Is(lbits - 1));
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opExtendXZ(ثابت V3_نمبر& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    ثابت صحيح copy_چوڑائی = std::min(چوڑائی(), static_cast<صحيح>(lbits));
    copy_بٹ(0, lhs, 0, copy_چوڑائی);
    fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, lhs.ثنائی_ہندسہ_Is(lbits - 1));
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opClean(ثابت V3_نمبر& lhs, uint32_t بٹ) { أرجع opSel(lhs, بٹ - 1, 0); }

فراغ V3_نمبر::opCleanThis(منطقي warnOnTruncation) {
    // Clean MSB of number
    عدد_ASSERT_LOGIC_ARGS1(*هذا);
    ثابت قدر_And_X v = m_انکڑے.عدد()[الفاظ() - 1];
    ثابت uint32_t new_قدر_Msb = v.m_قدر & hi_لفظ_نقاب();
    ثابت uint32_t new_قدر_XMsb = v.m_قدر_X & hi_لفظ_نقاب();
    إذا (warnOnTruncation && (new_قدر_Msb != v.m_قدر || new_قدر_XMsb != v.m_قدر_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << چوڑائی() << " bit number: " << displayed("%d"));
    }
    m_انکڑے.عدد()[الفاظ() - 1] = {new_قدر_Msb, new_قدر_XMsb};
}

V3_نمبر& V3_نمبر::opSel(ثابت V3_نمبر& lhs, ثابت V3_نمبر& msb, ثابت V3_نمبر& lsb) {
    عدد_ASSERT_OP_ARGS3(lhs, msb, lsb);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS2(msb, lsb);
    إذا (lsb.isFourState() || msb.isFourState()) أرجع سیٹ_All_بٹ_X();
    أرجع opSel(lhs, msb.منزل_UInt(), lsb.منزل_UInt());
}

V3_نمبر& V3_نمبر::opSel(ثابت V3_نمبر& lhs, uint32_t msbval, uint32_t lsbval) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سیٹ_Zero();
    صحيح copy_چوڑائی = 0;
    إذا (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.چوڑائی())) {
        ثابت uint64_t selected_چوڑائی = uint64_t{msbval} - lsbval + 1;
        copy_چوڑائی = static_cast<صحيح>(
            std::min({static_cast<uint64_t>(چوڑائی()), static_cast<uint64_t>(lhs.چوڑائی()) - lsbval,
                      selected_چوڑائی}));
    }
    copy_بٹ(0, lhs, copy_چوڑائی ? static_cast<صحيح>(lsbval) : 0, copy_چوڑائی);
    fill_بٹ(copy_چوڑائی, چوڑائی() - copy_چوڑائی, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    أرجع *هذا;
}

V3_نمبر& V3_نمبر::opSelInto(ثابت V3_نمبر& lhs, ثابت V3_نمبر& lsb, صحيح چوڑائی) {
    أرجع opSelInto(lhs, lsb.منزل_SInt(), چوڑائی);
}

V3_نمبر& V3_نمبر::opSelInto(ثابت V3_نمبر& lhs, صحيح lsbval, صحيح چوڑائی) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحيح copy_چوڑائی = std::max(0, std::min({چوڑائی, lhs.چوڑائی(), هذا->چوڑائی() - lsbval}));
    copy_بٹ(lsbval, lhs, 0, copy_چوڑائی);
    ثابت صحيح fill_چوڑائی
        = std::max(0, std::min(چوڑائی - copy_چوڑائی, هذا->چوڑائی() - lsbval - copy_چوڑائی));
    fill_بٹ(lsbval + copy_چوڑائی, fill_چوڑائی, v3Global.constRemoveXs() ? 0 : 'x');
    أرجع *هذا;
}

//======================================================================
// Ops - Floating point

V3_نمبر& V3_نمبر::opIToRD(ثابت V3_نمبر& lhs, منطقي isSigned) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_نمبر noxz(lhs);
    noxz.op_سونپو_Non_XZ(lhs);
    مزدوج d = 0;
    ثابت منطقي negate = isSigned && noxz.isNegative();
    إذا (negate) {
        ثابت V3_نمبر noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    لكل (صحيح ثنائی_ہندسہ_2 = noxz.چوڑائی() - 1; ثنائی_ہندسہ_2 >= 0; ثنائی_ہندسہ_2--) {
        // Some precision might be lost in this add, that's what we want
        إذا (noxz.ثنائی_ہندسہ_Is1(ثنائی_ہندسہ_2)) d += exp2(ثنائی_ہندسہ_2);
    }
    إذا (negate) d = -d;
    أرجع سیٹ_Double(d);
}
V3_نمبر& V3_نمبر::opRToIS(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_TRUNC(lhs.منزل_Double());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    أرجع سیٹ_Long_S(i);
}
V3_نمبر& V3_نمبر::opRToIRoundS(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_گیڑا(lhs.منزل_Double());
    سیٹ_Zero();
    اتحاد {
        مزدوج d;
        uint64_t q;
    } u;
    u.d = v;
    (فراغ)u.d;

    ثابت صحيح قوت_نما = static_cast<صحيح>((u.q >> 52ULL) & VL_نقاب_Q(11)) - 1023;
    ثابت صحيح lsb = قوت_نما - 52;
    ثابت uint64_t mantissa = (u.q & VL_نقاب_Q(52)) | (1ULL << 52);
    إذا (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        لكل (صحيح ثنائی_ہندسہ_2 = 0; ثنائی_ہندسہ_2 <= 52; ++ثنائی_ہندسہ_2) {
            إذا (mantissa & (1ULL << ثنائی_ہندسہ_2)) {
                ثابت صحيح outbit = ثنائی_ہندسہ_2 + lsb;
                إذا (outbit >= 0) سیٹ_ثنائی_ہندسہ(outbit, 1);
            }
        }
        إذا (v < 0) {
            ثابت V3_نمبر noSign = *هذا;
            opNegate(noSign);
        }
    }
    أرجع *هذا;
}
V3_نمبر& V3_نمبر::op_Real_منزل_بٹ(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.چوڑائی() == 64 && چوڑائی() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.منزل_Double();
    أرجع سیٹ_Quad(u.m_v);
}
V3_نمبر& V3_نمبر::op_بٹ_منزل_Real_D(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.چوڑائی() == 64 && چوڑائی() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.منزل_UQuad();
    أرجع سیٹ_Double(u.m_d);
}
V3_نمبر& V3_نمبر::opNegateD(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    أرجع سیٹ_Double(-lhs.منزل_Double());
}
V3_نمبر& V3_نمبر::op_جوڑو_D(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Double(lhs.منزل_Double() + rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opSubD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Double(lhs.منزل_Double() - rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opMulD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Double(lhs.منزل_Double() * rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opDivD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Double(lhs.منزل_Double() / rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opPowD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Double(قوت(lhs.منزل_Double(), rhs.منزل_Double()));
}
V3_نمبر& V3_نمبر::opEqD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_Double() == rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opNeqD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_Double() != rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opGtD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_Double() > rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opGteD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_Double() >= rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opLtD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_Double() < rhs.منزل_Double());
}
V3_نمبر& V3_نمبر::opLteD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_Double() <= rhs.منزل_Double());
}

//======================================================================
// Ops - String

V3_نمبر& V3_نمبر::opConcatN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_String(lhs.منزل_String() + rhs.منزل_String());
}
V3_نمبر& V3_نمبر::opReplN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_STRING_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(rhs);
    أرجع opReplN(lhs, rhs.منزل_UInt());
}
V3_نمبر& V3_نمبر::opReplN(ثابت V3_نمبر& lhs, uint32_t rhsval) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    string باہرلا;
    باہرلا.reserve(lhs.منزل_String().لمائی() * rhsval);
    لكل (بنانشانی times = 0; times < rhsval; ++times) باہرلا += lhs.منزل_String();
    أرجع سیٹ_String(باہرلا);
}
V3_نمبر& V3_نمبر::opCvtPackString(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    أرجع سیٹ_String(lhs.منزل_String());
}
V3_نمبر& V3_نمبر::op_منزل_ہیٹھلا_N(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string باہرلا = VString::downcase(lhs.منزل_String());
    أرجع سیٹ_String(باہرلا);
}
V3_نمبر& V3_نمبر::op_منزل_اتلا_N(ثابت V3_نمبر& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string باہرلا = VString::upcase(lhs.منزل_String());
    أرجع سیٹ_String(باہرلا);
}

V3_نمبر& V3_نمبر::opEqN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_String() == rhs.منزل_String());
}
V3_نمبر& V3_نمبر::opNeqN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_String() != rhs.منزل_String());
}
V3_نمبر& V3_نمبر::opGtN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_String() > rhs.منزل_String());
}
V3_نمبر& V3_نمبر::opGteN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_String() >= rhs.منزل_String());
}
V3_نمبر& V3_نمبر::opLtN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_String() < rhs.منزل_String());
}
V3_نمبر& V3_نمبر::opLteN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سیٹ_Single_بٹ(lhs.منزل_String() <= rhs.منزل_String());
}

//======================================================================

فراغ V3_نمبر::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_سطر* ثابت fl = جديد File_سطر{File_سطر::built_وچ_Filename()};
    V3_نمبر عدد{fl, 32, 0};
    عدد.selfTestThis();
}

فراغ V3_نمبر::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(0, 0), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(1e19, 1e19), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(9, 0.0001), خطأ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(1, 1 + std::numeric_limits<مزدوج>::epsilon()), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonEqual(0.009, 0.00899999999999999931998839741709), سچ);

    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(0), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(1), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(-1), سچ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(1.0001), خطأ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(0.9999), خطأ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(-1.0001), خطأ);
    UASSERT_SELFTEST(V3_نمبر::epsilonIntegral(-0.9999), خطأ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
