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

constexpr صحيح MAX_SPRINTF_DOUBLE_سائيز
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#وصف_ڪريو انگ_ASSERT_OP_ARGS1(arg1) \
    UASSERT((هذا != &(arg1)), "Number operation called with same source and dest")
#وصف_ڪريو انگ_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2)), \
            "Number operation called with same source and dest")
#وصف_ڪريو انگ_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3)), \
            "Number operation called with same source and dest")
#وصف_ڪريو انگ_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3) && هذا != &(arg4)), \
            "Number operation called with same source and dest")

#وصف_ڪريو انگ_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).معلومات_Type() == V3_عدد_معلومات::V3_عدد_معلومات_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#وصف_ڪريو انگ_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    انگ_ASSERT_LOGIC_ARGS1(arg1); \
    انگ_ASSERT_LOGIC_ARGS1(arg2)

#وصف_ڪريو انگ_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    انگ_ASSERT_LOGIC_ARGS1(arg1); \
    انگ_ASSERT_LOGIC_ARGS1(arg2); \
    انگ_ASSERT_LOGIC_ARGS1(arg3); \
    انگ_ASSERT_LOGIC_ARGS1(arg4)

#وصف_ڪريو انگ_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#وصف_ڪريو انگ_ASSERT_STRING_ARGS2(arg1, arg2) \
    انگ_ASSERT_STRING_ARGS1(arg1); \
    انگ_ASSERT_STRING_ARGS1(arg2)

#وصف_ڪريو انگ_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#وصف_ڪريو انگ_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    انگ_ASSERT_DOUBLE_ARGS1(arg1); \
    انگ_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

فراغ V3_عدد::v3error_پڇاڙي(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (m_nodep) {
        m_nodep->v3error_پڇاڙي(nsstr);
    } وإلا إذا (m_fileline) {
        m_fileline->v3error_پڇاڙي(nsstr);
    } وإلا {
        V3Error::v3error_پڇاڙي(nsstr, "", خالي_اشارو);
    }
}

فراغ V3_عدد::v3error_پڇاڙي_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_پڇاڙي(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_عدد::V3_عدد(Ast_ڳنڍ* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    إذا (str.خالي_جايون()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } وإلا {
        init(nodep, std::max<صحيح>(str.ڊيگهائي() * 8, 1));
        لكل (بنانشان pos = 0; pos < str.ڊيگهائي(); ++pos) {
            ثابت صحيح topos = str.ڊيگهائي() - 1 - pos;
            قدر_And_X& v = m_معلومات.انگ()[topos / 4];
            لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < 8; ++ٻٽو_انگ_2) {
                إذا (str[pos] & (1UL << ٻٽو_انگ_2)) v.m_قدر |= (1UL << (ٻٽو_انگ_2 + (topos % 4) * 8));
            }
        }
    }
    m_معلومات.m_fromString = سچ;
    opCleanThis(سچ);
}

V3_عدد::V3_عدد(Ast_ڳنڍ* nodep, ثابت Ast_ڳنڍ_DType* nodedtypep) {
    إذا (nodedtypep->isCHandle()) {
        init(nodep);
        سيٽ_Null();
    } وإلا إذا (nodedtypep->isDouble()) {
        init(nodep, 64);
        سيٽ_Double(0.0);
    } وإلا إذا (nodedtypep->isString()) {
        init(nodep);
        سيٽ_String("");
    } وإلا {
        init(nodep, nodedtypep->ويڪر(), nodedtypep->ويڪر_Sized());
    }
}

فراغ V3_عدد::create(ثابت حرف* sourcep) {
    m_معلومات.سيٽ_Logic();
    ثابت حرف* قدر_startp = sourcep;
    لكل (ثابت حرف* cp = sourcep; *cp; ++cp) {
        إذا (*cp == '\'') {
            قدر_startp = cp + 1;
            اكسر;
        }
    }

    منطقي userSized = خطأ;
    منطقي unbased = خطأ;
    حرف بنياد = '\0';
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
            بنياد = *cp;
            ++cp;
        }
        قدر_startp = cp;

        userSized = widthn.ڊيگهائي() != 0;
        إذا (std::atoi(widthn.c_str())) {
            إذا (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_انگ_ويڪر()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                ويڪر(v3Global.opt.max_انگ_ويڪر(), سچ);
            } وإلا {
                ويڪر(std::atoi(widthn.c_str()), سچ);
            }
        }
    } وإلا {
        unbased = سچ;
        بنياد = 'd';
    }

    لكل (صحيح i = 0; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    إذا (بنياد == '0') {
        ويڪر(1, خطأ);  // So we extend it
        سيٽ_ٻٽو_انگ(0, 0);
        m_معلومات.m_autoExtend = سچ;
    } وإلا إذا (بنياد == '1') {
        ويڪر(1, خطأ);  // So we extend it
        سيٽ_ٻٽو_انگ(0, 1);
        m_معلومات.m_autoExtend = سچ;
    } وإلا إذا (std::tolower(بنياد) == 'z') {
        ويڪر(1, خطأ);  // So we extend it
        سيٽ_ٻٽو_انگ(0, 'z');
        m_معلومات.m_autoExtend = سچ;
    } وإلا إذا (std::tolower(بنياد) == 'x') {
        ويڪر(1, خطأ);  // So we extend it
        سيٽ_ٻٽو_انگ(0, 'x');
        m_معلومات.m_autoExtend = سچ;
    }
    // Otherwise...
    وإلا إذا (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحيح ويڪر_Bound = std::max<صحيح>(32, std::لڙي_ڊيگهه(قدر_startp) * 4);
        ويڪر(ويڪر_Bound, خطأ);  // Will change width below
        إذا (unbased) isSigned(سچ);  // Also says the spec.
    }

    // Ignore leading blanks
    طالما (*قدر_startp == '_' || std::خال_آهي(*قدر_startp)) ++قدر_startp;
    إذا (!*قدر_startp && !m_معلومات.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    إذا (userSized && m_معلومات.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحيح obit = 0;  // Start at LSB
    صحيح بنياد_align = 1;
    إذا (std::tolower(بنياد) == 'd') {
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
                    m_معلومات.انگ()[0].m_قدر = val;
                } وإلا {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_عدد حاصل_ضرب(هذا, ويڪر() + 4);  // +4 for overflow detection
                    ثابت V3_عدد ten(هذا, ويڪر() + 4, 10);
                    ثابت V3_عدد addend(هذا, ويڪر(), (*cp - '0'));
                    حاصل_ضرب.opMul(*هذا, ten);
                    op_جوڙيو(حاصل_ضرب, addend);
                    إذا (!warnedTooMany && حاصل_ضرب.بٽ_قدر(ويڪر(), 4)) {  // Overflowed
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
                إذا (!userSized) ويڪر(32, خطأ);
                سيٽ_All_بٽ_Z();
                اكسر;
            }
            حالة 'x': {
                got_x = 1;
                إذا (!userSized) ويڪر(32, خطأ);
                سيٽ_All_بٽ_X();
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
        obit = ويڪر();
        إذا ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << قدر_startp);
        }
    } وإلا {
        // Convert bin/octal number to hex
        لكل (ثابت حرف* cp = قدر_startp + std::لڙي_ڊيگهه(قدر_startp) - 1; cp >= قدر_startp;
             cp--) {
            إذا (*cp != '_' && *cp != '0' && obit >= ويڪر()) {
                warnTooMany(sourcep);
                اكسر;
            }
            بدل (std::tolower(بنياد)) {
            حالة 'b': {
                بنياد_align = 1;
                بدل (std::tolower(*cp)) {
                حالة '0': سيٽ_ٻٽو_انگ(obit++, 0); اكسر;
                حالة '1': سيٽ_ٻٽو_انگ(obit++, 1); اكسر;
                حالة 'z':
                حالة '?': سيٽ_ٻٽو_انگ(obit++, 'z'); اكسر;
                حالة 'x': سيٽ_ٻٽو_انگ(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                افتراضي: v3error("Illegal character in binary constant: " << *cp);
                }
                اكسر;
            }

            حالة 'o':
            حالة 'c': {
                بنياد_align = 3;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': سيٽ_ٻٽو_انگ(obit++, 0); سيٽ_ٻٽو_انگ(obit++, 0);  سيٽ_ٻٽو_انگ(obit++, 0);  اكسر;
                حالة '1': سيٽ_ٻٽو_انگ(obit++, 1); سيٽ_ٻٽو_انگ(obit++, 0);  سيٽ_ٻٽو_انگ(obit++, 0);  اكسر;
                حالة '2': سيٽ_ٻٽو_انگ(obit++, 0); سيٽ_ٻٽو_انگ(obit++, 1);  سيٽ_ٻٽو_انگ(obit++, 0);  اكسر;
                حالة '3': سيٽ_ٻٽو_انگ(obit++, 1); سيٽ_ٻٽو_انگ(obit++, 1);  سيٽ_ٻٽو_انگ(obit++, 0);  اكسر;
                حالة '4': سيٽ_ٻٽو_انگ(obit++, 0); سيٽ_ٻٽو_انگ(obit++, 0);  سيٽ_ٻٽو_انگ(obit++, 1);  اكسر;
                حالة '5': سيٽ_ٻٽو_انگ(obit++, 1); سيٽ_ٻٽو_انگ(obit++, 0);  سيٽ_ٻٽو_انگ(obit++, 1);  اكسر;
                حالة '6': سيٽ_ٻٽو_انگ(obit++, 0); سيٽ_ٻٽو_انگ(obit++, 1);  سيٽ_ٻٽو_انگ(obit++, 1);  اكسر;
                حالة '7': سيٽ_ٻٽو_انگ(obit++, 1); سيٽ_ٻٽو_انگ(obit++, 1);  سيٽ_ٻٽو_انگ(obit++, 1);  اكسر;
                حالة 'z':
                حالة '?': سيٽ_ٻٽو_انگ(obit++, 'z'); سيٽ_ٻٽو_انگ(obit++, 'z'); سيٽ_ٻٽو_انگ(obit++, 'z'); اكسر;
                حالة 'x': سيٽ_ٻٽو_انگ(obit++, 'x'); سيٽ_ٻٽو_انگ(obit++, 'x'); سيٽ_ٻٽو_انگ(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                // clang-format on
                افتراضي: v3error("Illegal character in octal constant");
                }
                اكسر;
            }

            حالة 'h':  // FALLTHRU
            حالة 'x': {
                بنياد_align = 4;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '1': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '2': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '3': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '4': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '5': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '6': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '7': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); اكسر;
                حالة '8': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة '9': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'a': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'b': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'c': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'd': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'e': سيٽ_ٻٽو_انگ(obit++,0); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'f': سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); سيٽ_ٻٽو_انگ(obit++,1); اكسر;
                حالة 'z':
                حالة '?': سيٽ_ٻٽو_انگ(obit++,'z'); سيٽ_ٻٽو_انگ(obit++,'z'); سيٽ_ٻٽو_انگ(obit++,'z'); سيٽ_ٻٽو_انگ(obit++,'z'); اكسر;
                حالة 'x': سيٽ_ٻٽو_انگ(obit++,'x'); سيٽ_ٻٽو_انگ(obit++,'x'); سيٽ_ٻٽو_انگ(obit++,'x'); سيٽ_ٻٽو_انگ(obit++,'x'); اكسر;
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
                v3error("Illegal base character: " << بنياد);
                اكسر;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    إذا (!userSized && !m_معلومات.m_autoExtend) {
        ويڪر(std::max(32, بنياد_align * ((ويڪر_منزل_Fit() + بنياد_align - 1) / بنياد_align)), خطأ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    طالما (obit <= ويڪر() && obit && ٻٽو_انگ_Is_XZ(obit - 1)) {
        سيٽ_ٻٽو_انگ(obit, ٻٽو_انگ_Is(obit - 1));
        ++obit;
    }
    opCleanThis(سچ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

فراغ V3_عدد::warnTooMany(ثابت string& قدر) {
    v3warn(WIDTHDIGITS, "Too many digits for " << ويڪر() << " bit number: '" << قدر << "'");
}

فراغ V3_عدد::nodep(Ast_ڳنڍ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    إذا (!nodep) أرجع;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

منطقي V3_عدد::epsilonEqual(مزدوج a, مزدوج b) {
    أرجع std::fabs(a - b)
           <= (std::numeric_limits<مزدوج>::epsilon() * std::max(1.0, std::max(a, b)));
}

منطقي V3_عدد::epsilonIntegral(مزدوج a) {
    ثابت مزدوج dint = static_cast<مزدوج>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    أرجع epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

صحيح V3_عدد::log2b(uint32_t انگ) {
    // See also opCLog2
    لكل (صحيح ٻٽو_انگ_2 = 31; ٻٽو_انگ_2 > 0; ٻٽو_انگ_2--) {
        إذا (انگ & (1ULL << ٻٽو_انگ_2)) أرجع ٻٽو_انگ_2;
    }
    أرجع 0;
}

صحيح V3_عدد::log2bQuad(uint64_t انگ) {
    إذا (انگ >> 32ULL) أرجع 32 + log2b(انگ >> 32ULL);
    أرجع log2b(انگ);
}

//======================================================================
// Setters

V3_عدد& V3_عدد::سيٽ_Zero() {
    لكل (صحيح i = 0; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, 0};
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_Quad(uint64_t قدر) {
    سيٽ_Zero();
    m_معلومات.انگ()[0].m_قدر = قدر & 0xffffffffULL;
    إذا (ويڪر() > 32) m_معلومات.انگ()[1].m_قدر = (قدر >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_Long(uint32_t قدر) {
    سيٽ_Zero();
    m_معلومات.انگ()[0].m_قدر = قدر;
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_Long_S(int32_t قدر) {
    سيٽ_Zero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = قدر;
    (فراغ)u.s;
    m_معلومات.انگ()[0].m_قدر = u.u;
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_Double(مزدوج قدر) {
    UASSERT(ويڪر() == 64, "Real operation on wrong sized number");
    m_معلومات.سيٽ_Double();
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.d = قدر;
    (فراغ)u.d;
    لكل (صحيح i = 2; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, 0};
    m_معلومات.انگ()[0].m_قدر = u.u[0];
    m_معلومات.انگ()[1].m_قدر = u.u[1];
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_Single_بٽ(حرف قدر) {
    لكل (صحيح i = 1 /*مٿانهون*/; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, 0};
    m_معلومات.انگ()[0] = {(قدر == '1' || قدر == 'x' || قدر == 1 || قدر == 3),
                       (قدر == 'z' || قدر == 'x' || قدر == 2 || قدر == 3)};
    أرجع *هذا;
}

V3_عدد& V3_عدد::سيٽ_All_Bits0() {
    سيٽ_Zero();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_All_Bits1() {
    لكل (صحيح i = 0; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {~0U, 0};
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_All_بٽ_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    لكل (صحيح i = 0; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {~0U, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_All_بٽ_Z() {
    لكل (صحيح i = 0; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::سيٽ_All_بٽ_XRemoved() {
    إذا (!v3Global.constRemoveXs()) {
        أرجع سيٽ_All_بٽ_X();
    } وإلا {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        إذا (v3Global.opt.x_سونپيو() == "1") {
            أرجع سيٽ_All_Bits1();
        } وإلا {
            أرجع سيٽ_All_Bits0();
        }
    }
}
V3_عدد& V3_عدد::سيٽ_Value1() {
    m_معلومات.انگ()[0] = {1, 0};
    لكل (صحيح i = 1; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, 0};
    أرجع *هذا;
}

فراغ V3_عدد::سيٽ_ٻٽو_انگ_X0(صحيح ٻٽو_انگ_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_عدد& V3_عدد::سيٽ_نقاب(صحيح nbits, صحيح lsb) {
    سيٽ_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_بٽ(lsb, std::max(0, std::min(nbits, ويڪر() - lsb)), 1);
    أرجع *هذا;
}

//======================================================================
// ACCESSORS - as strings

string V3_عدد::ascii(منطقي prefixed, منطقي cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (is1_قدم()) أرجع "1step";
    إذا (isNull()) أرجع "null";
    std::ostringstream ٻاهريون;
    إذا (isDouble()) {
        ٻاهريون.precision(17);
        إذا (VL_UNCOVERABLE(ويڪر() != 64)) {
            ٻاهريون << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } وإلا {
            ٻاهريون << منزل_Double();
            إذا (منزل_Double() == floor(منزل_Double())) ٻاهريون << ".0";
        }
        أرجع ٻاهريون.str();
    } وإلا إذا (isString()) {
        أرجع '"' + منزل_String() + '"';
    } وإلا {
        إذا (VL_UNCOVERABLE((m_معلومات.انگ()[لفظ_2() - 1].m_قدر | m_معلومات.انگ()[لفظ_2() - 1].m_قدر_X)
                           & ~hi_لفظ_نقاب())) {
            ٻاهريون << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    إذا (prefixed) {
        إذا (sized()) {
            ٻاهريون << ويڪر() << "'";
        } وإلا إذا (autoExtend() && ويڪر() == 1) {
            ٻاهريون << "'";
            إذا (ٻٽو_انگ_Is0(0)) {
                ٻاهريون << '0';
            } وإلا إذا (ٻٽو_انگ_Is1(0)) {
                ٻاهريون << '1';
            } وإلا إذا (ٻٽو_انگ_Is_Z(0)) {
                ٻاهريون << 'z';
            } وإلا {
                ٻاهريون << 'x';
            }
            أرجع ٻاهريون.str();
        } وإلا {
            إذا (cleanVerilog) {
                ٻاهريون << "'";
            } وإلا {
                ٻاهريون << "?" << ويڪر() << "?";
            }
        }
        إذا (isSigned()) ٻاهريون << "s";
    }

    ثابت منطقي ٻٽو = (isFourState()
#جيڪڏهن_وصف_ٿيل V3_عدد_ASCII_ٻٽو
                         // cppcheck-suppress konwnConditionTrueFalse
                         || سچ
#جيڪڏهن_پورو
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    إذا (ٻٽو) {
        ٻاهريون << "b";
        ٻاهريون << displayed("%0b");
    } وإلا {
        إذا (prefixed) ٻاهريون << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        ٻاهريون << displayed("%0h");
    }
    إذا (isNull()) {
        إذا (VL_UNCOVERABLE(!isEqZero())) {
            ٻاهريون << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } وإلا {
            ٻاهريون << " [null]";
        }
    }
    أرجع ٻاهريون.str();
}

منطقي V3_عدد::displayedFmtHasArg(حرف format, منطقي is_جاچ) {
    // Is this a format letter that takes an argument in runtime?
    (فراغ)is_جاچ;
    بدل (std::tolower(format)) {
    حالة 'b': أرجع سچ;  //ٻٽو
    حالة 'c': أرجع سچ;  // Character
    حالة 'd': أرجع سچ;  // Decimal
    حالة 'e': أرجع سچ;  // Floating
    حالة 'f': أرجع سچ;  // Floating
    حالة 'g': أرجع سچ;  // Floating
    حالة 'h': أرجع سچ;  // Hex
    حالة 'o': أرجع سچ;  // Octal
    حالة 'p': أرجع سچ;  //نمونو
    حالة 's': أرجع سچ;  // String
    حالة 't': أرجع سچ;  //وقت
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

string V3_عدد::displayPad(سائيز_t fmtsize, حرف pad, منطقي کاٻي, ثابت string& اندريون) VL_خالص {
    string padding;
    إذا (اندريون.ڊيگهائي() < fmtsize) padding = string(fmtsize - اندريون.ڊيگهائي(), pad);
    أرجع کاٻي ? (اندريون + padding) : (padding + اندريون);
}

string V3_عدد::displayedEnumName(ثابت Ast_Enum_شي* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    أرجع VString::isIdentifier(name) && !std::انگ_آهي(static_cast<بنانشان حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_عدد::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3_عدد قدر{هذا, dtypep->ويڪر()};
    قدر.op_سونپيو(*هذا);
    لكل (ثابت Ast_Enum_شي* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_شي)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        إذا (!constp->انگ().isAnyXZ() && قدر.isCaseEq(constp->انگ()))
            أرجع displayedEnumName(itemp);
    }
    أرجع "";
}

string V3_عدد::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
    V3_عدد قدر{هذا, dtypep->ويڪر()};
    قدر.op_سونپيو(*هذا);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.پٺ());
    إذا (fmt != 'p' && fmt != 's') أرجع قدر.displayed(argp, vformat, numericAttr);
    string name = قدر.displayedEnumName(dtypep);
    إذا (name.خالي_جايون()) name = قدر.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_عدد name_انگ{هذا};
    name_انگ.سيٽ_String(name);
    أرجع name_انگ.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_عدد::displayed(ثابت Ast_ڳنڍ* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    أرجع displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_عدد::displayed(File_سٽ* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    خودڪار pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *هذا);
    ++pos;
    منطقي کاٻي = خطأ;
    إذا (pos[0] == '-') {
        کاٻي = سچ;
        ++pos;
    }
    string fmtsize;
    لكل (; pos != vformat.cend() && (std::انگ_آهي(pos[0]) || pos[0] == '.'); ++pos) {
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
            V3_عدد strwide{fl, static_cast<صحيح>(منزل_String().ڊيگهائي()) * 8, 0};
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
            V3_عدد nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*هذا);
            أرجع nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    بدل (fmt) {
    حالة 'c': {
        إذا (ويڪر() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت بنانشان صحيح v = بٽ_قدر(0, 8);
        str.ڌڪو_پٺ(static_cast<حرف>(v));
        أرجع str;
    }
    حالة 'e':  // FALLTHRU
    حالة 'f':  // FALLTHRU
    حالة 'g': {
        ثابت مزدوج n = formatAttr.isDouble()   ? منزل_Double()
                         : formatAttr.isSigned() ? منزل_SQuad()
                                                 : منزل_UQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_سائيز];
        (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_سائيز, vformat.c_str(), n);
        أرجع tmp;
    }
    حالة 's': {
        إذا (formatAttr.isString() || formatAttr.isComplex()) {
            str = منزل_String();
        } وإلا {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحيح ٻٽو_انگ_2 = ويڪر() - 1;
            منطقي شروعات = سچ;
            طالما ((ٻٽو_انگ_2 % 8) != 7) ++ٻٽو_انگ_2;
            لكل (; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2 -= 8) {
                ثابت صحيح v = بٽ_قدر(ٻٽو_انگ_2 - 7, 8);
                إذا (!شروعات || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    شروعات = خطأ;  // Drop leading 0s
                } وإلا {
                    إذا (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت سائيز_t fmtsizen = static_cast<سائيز_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', کاٻي, str);
        أرجع str;
    }
    حالة 'p': {  //نمونو
        // 'p' with NUMBER was earlier converted to 'd'
        إذا (formatAttr.isDouble()) {
            ثابت مزدوج n = formatAttr.isDouble()   ? منزل_Double()
                             : formatAttr.isSigned() ? منزل_SQuad()
                                                     : منزل_UQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_سائيز];
            (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_سائيز, "%g", n);
            أرجع tmp;
        }
        إذا (formatAttr.isString())
            أرجع '"' + V3_ٻاهريون_Formatter::quoteNameControls(منزل_String()) + '"';
        إذا (formatAttr.isComplex()) أرجع منزل_String();
        أرجع "%p";
    }
    حالة 'b':  // FALLTHRU
    حالة 'o':  // FALLTHRU
    حالة 'h':  // FALLTHRU
    حالة 'x': {
        صحيح ٻٽو_انگ_2 = ويڪر() - 1;
        إذا (کاٻي || !fmtsize.خالي_جايون()) {
            طالما (ٻٽو_انگ_2 && ٻٽو_انگ_Is0(ٻٽو_انگ_2)) --ٻٽو_انگ_2;
        }
        بدل (fmt) {
        حالة 'b': {
            لكل (; ٻٽو_انگ_2 >= 0; --ٻٽو_انگ_2) {
                إذا (ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
                    str += '0';
                } وإلا إذا (ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
                    str += '1';
                } وإلا إذا (ٻٽو_انگ_Is_Z(ٻٽو_انگ_2)) {
                    str += 'z';
                } وإلا {
                    str += 'x';
                }
            }
            اكسر;
        }
        حالة 'o': {
            طالما ((ٻٽو_انگ_2 % 3) != 2) ++ٻٽو_انگ_2;
            لكل (; ٻٽو_انگ_2 > 0; ٻٽو_انگ_2 -= 3) {
                ثابت صحيح انگ_X = ڳڻپ_X(ٻٽو_انگ_2 - 2, 3);
                ثابت صحيح انگ_Z = ڳڻپ_Z(ٻٽو_انگ_2 - 2, 3);
                إذا (انگ_X == 3 || انگ_X == ويڪر() - (ٻٽو_انگ_2 - 2)) {
                    str += 'x';
                    استمر;
                }
                إذا (انگ_Z == 3 || انگ_Z == ويڪر() - (ٻٽو_انگ_2 - 2)) {
                    str += 'z';
                    استمر;
                }
                إذا (انگ_X > 0) {
                    str += 'X';
                    استمر;
                }
                إذا (انگ_Z > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = بٽ_قدر(ٻٽو_انگ_2 - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            اكسر;
        }
        افتراضي: {  // h/x
            طالما ((ٻٽو_انگ_2 % 4) != 3) ++ٻٽو_انگ_2;
            لكل (; ٻٽو_انگ_2 > 0; ٻٽو_انگ_2 -= 4) {
                ثابت صحيح انگ_X = ڳڻپ_X(ٻٽو_انگ_2 - 3, 4);
                ثابت صحيح انگ_Z = ڳڻپ_Z(ٻٽو_انگ_2 - 3, 4);
                إذا (انگ_X == 4 || انگ_X == ويڪر() - (ٻٽو_انگ_2 - 3)) {
                    str += 'x';
                    استمر;
                }
                إذا (انگ_Z == 4 || انگ_Z == ويڪر() - (ٻٽو_انگ_2 - 3)) {
                    str += 'z';
                    استمر;
                }
                إذا (انگ_X > 0) {
                    str += 'X';
                    استمر;
                }
                إذا (انگ_Z > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = بٽ_قدر(ٻٽو_انگ_2 - 3, 4);
                إذا (v >= 10) {
                    str += static_cast<حرف>('a' + v - 10);
                } وإلا {
                    str += static_cast<حرف>('0' + v);
                }
            }
            اكسر;
        }
        }  // switch
        ثابت سائيز_t fmtsizen = static_cast<سائيز_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (کاٻي ? ' ' : '0'), کاٻي, str);
        أرجع str;
    }  // case b/d/x/o
    حالة 't':  //وقت
    حالة 'd': {  // Unsigned decimal
        ثابت منطقي issigned = formatAttr.isSigned();
        إذا (fmtsize == "" && !کاٻي) {
            ثابت مزدوج mantissabits = ويڪر() - (issigned ? 1 : 0);
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
                إذا (ويڪر() > 64) {
                    str = منزل_Decimal_S();
                } وإلا {
                    str = cvt_منزل_Str(منزل_SQuad());
                }
            } وإلا {
                إذا (ويڪر() > 64) {
                    str = منزل_Decimal_U();
                } وإلا {
                    str = cvt_منزل_Str(منزل_UQuad());
                }
            }
        }
        ثابت منطقي zeropad = fmtsize.ڊيگهائي() > 0 && fmtsize[0] == '0' && !کاٻي;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت سائيز_t fmtsizen = static_cast<سائيز_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), کاٻي, str);
        أرجع str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالة 'u': {  // Packed 2-state
        لكل (صحيح i = 0; i < لفظ_2(); ++i) {
            ثابت uint32_t v = m_معلومات.انگ()[i].m_قدر;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'z': {  // Packed 4-state
        لكل (صحيح i = 0; i < لفظ_2(); ++i) {
            ثابت قدر_And_X v = m_معلومات.انگ()[i];
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
        صحيح ٻٽو_انگ_2 = ويڪر() - 1;
        لكل (; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2--) {
            إذا (ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            وإلا إذا (ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
                str += "St1 ";
            } وإلا إذا (ٻٽو_انگ_Is_Z(ٻٽو_انگ_2)) {
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

string V3_عدد::emitC() ثابت VL_MT_STABLE {
    constexpr سائيز_t bufsize = 1000;
    حرف sbuf[bufsize];
    string نتيجو;
    إذا (isNull()) {
        أرجع "VlNull{}";
    } وإلا إذا (isDouble()) {
        ثابت مزدوج dnum = منزل_Double();
        إذا (std::isinf(dnum)) {
            إذا (std::signbit(dnum)) نتيجو += '-';
            نتيجو += "std::numeric_limits<double>::infinity()";
        } وإلا إذا (std::isnan(dnum)) {
            إذا (std::signbit(dnum)) نتيجو += '-';
            نتيجو += "std::numeric_limits<double>::quiet_NaN()";
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
            = V3_ٻاهريون_Formatter::quoteNameControls(strg, V3_ٻاهريون_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        أرجع '"' + quoted + "\"s";
    } وإلا إذا (لفظ_2() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        نتيجو = "::VlWide<" + std::منزل_string(لفظ_2()) + ">{{";
        إذا (لفظ_2() > 4) نتيجو += '\n';
        لكل (صحيح n = 0; n < لفظ_2(); ++n) {
            إذا (n) نتيجو += ((n % 4) ? ", " : ",\n");
            (فراغ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_لفظ(n));
            نتيجو += sbuf;
        }
        إذا (لفظ_2() > 4) نتيجو += '\n';
        نتيجو += "}}";
    } وإلا إذا (لفظ_2() == 2) {  // Quad
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
                                : (ويڪر() > 16) ? ("0x%08" PRIx32 "U")
                                : (ويڪر() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        أرجع sbuf;
    }
    أرجع نتيجو;
}

string V3_عدد::منزل_Decimal_S() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (isNegative()) {
        V3_عدد lhsNoSign = *هذا;
        lhsNoSign.opNegate(*هذا);
        أرجع "-"s + lhsNoSign.منزل_Decimal_U();
    } وإلا {
        أرجع منزل_Decimal_U();
    }
}

string V3_عدد::منزل_Decimal_U() ثابت VL_MT_STABLE {
    ثابت صحيح maxdecwidth = (ويڪر() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_عدد bcd(هذا, maxdecwidth + 4);
    V3_عدد tmp(هذا, maxdecwidth + 4);
    V3_عدد tmp2(هذا, maxdecwidth + 4);

    صحيح from_ٻٽو_انگ = ويڪر() - 1;
    // Skip all leading zeros
    لكل (; from_ٻٽو_انگ >= 0 && ٻٽو_انگ_Is0(from_ٻٽو_انگ); --from_ٻٽو_انگ) {}
    // Double-dabble algorithm
    لكل (; from_ٻٽو_انگ >= 0; --from_ٻٽو_انگ) {
        // Any digits >= 5 need an add 3 (via tmp)
        لكل (صحيح nibble_ٻٽو_انگ = 0; nibble_ٻٽو_انگ < maxdecwidth; nibble_ٻٽو_انگ += 4) {
            إذا (bcd.بٽ_قدر(nibble_ٻٽو_انگ, 4) >= 5) {
                tmp2.سيٽ_All_Bits0();
                tmp2.سيٽ_ٻٽو_انگ(nibble_ٻٽو_انگ, 1);  // Add 3, decomposed as two bits
                tmp2.سيٽ_ٻٽو_انگ(nibble_ٻٽو_انگ + 1, 1);
                tmp.op_سونپيو(bcd);
                bcd.op_جوڙيو(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_سونپيو(bcd);
        bcd.op_سرڪايو_L(tmp, V3_عدد(هذا, 32, 1));
        // bcd[0] = this[from_bit]
        إذا (ٻٽو_انگ_Is1(from_ٻٽو_انگ)) bcd.سيٽ_ٻٽو_انگ(0, 1);
    }

    string output;
    صحيح lsb = (maxdecwidth - 1) & ~3;
    لكل (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        إذا (bcd.بٽ_قدر(lsb, 4)) اكسر;
    }
    لكل (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.بٽ_قدر(lsb, 4));  // 0..9
    }
    أرجع output;
}

string V3_عدد::منزل_Real_String() ثابت VL_MT_STABLE {
    ثابت مزدوج قدر = منزل_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream ٻاهريون;
    ٻاهريون.precision(15);
    ٻاهريون << قدر;
    string نتيجو = ٻاهريون.str();
    إذا (std::strtod(نتيجو.c_str(), خالي_اشارو) != قدر) {
        ٻاهريون.str("");
        ٻاهريون.precision(17);
        ٻاهريون << قدر;
        نتيجو = ٻاهريون.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    إذا (نتيجو.ڳوليو_پهريون_جو(".en") == string::npos) نتيجو += ".0";
    أرجع نتيجو;
}

//======================================================================
// ACCESSORS - as numbers

منطقي V3_عدد::سمائي_اندريون_UInt() ثابت VL_MT_محفوظ {
    إذا (isFourState()) أرجع خطأ;
    // We allow wide numbers that represent values <= 32 bits
    لكل (صحيح i = 1; i < لفظ_2(); ++i) {
        إذا (m_معلومات.انگ()[i].m_قدر) أرجع خطأ;
    }
    أرجع سچ;
}
uint32_t V3_عدد::منزل_UInt() ثابت VL_MT_محفوظ {
    UASSERT(!isFourState(), "toUInt with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 32 bits
    إذا (VL_UNCOVERABLE(!سمائي_اندريون_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *هذا);
    }
    أرجع m_معلومات.انگ()[0].m_قدر;
}

مزدوج V3_عدد::منزل_Double() ثابت VL_MT_محفوظ {
    UASSERT(isDouble() && ويڪر() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.u[0] = m_معلومات.انگ()[0].m_قدر;
    u.u[1] = m_معلومات.انگ()[1].m_قدر;
    أرجع u.d;
}

int32_t V3_عدد::منزل_SInt() ثابت VL_MT_محفوظ {
    إذا (isSigned()) {
        ثابت uint32_t v = منزل_UInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (ويڪر() - 1))));
        ثابت uint32_t extended = v | signExtend;
        أرجع static_cast<int32_t>(extended);
    } وإلا {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        أرجع static_cast<int32_t>(منزل_UInt());
    }
}

uint64_t V3_عدد::منزل_UQuad() ثابت VL_MT_محفوظ {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 64 bits
    إذا (isDouble()) أرجع static_cast<uint64_t>(منزل_Double());
    لكل (صحيح i = 2; i < لفظ_2(); ++i) {
        إذا (m_معلومات.انگ()[i].m_قدر) {
            v3error("Value too wide for 64-bits expected in this context " << *هذا);
            اكسر;
        }
    }
    إذا (ويڪر() <= 32) أرجع (static_cast<uint64_t>(منزل_UInt()));
    أرجع ((static_cast<uint64_t>(m_معلومات.انگ()[1].m_قدر) << 32ULL)
            | (static_cast<uint64_t>(m_معلومات.انگ()[0].m_قدر)));
}

int64_t V3_عدد::منزل_SQuad() ثابت {
    إذا (isDouble()) أرجع static_cast<int64_t>(منزل_Double());
    ثابت uint64_t v = منزل_UQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (ويڪر() - 1))));
    ثابت uint64_t extended = v | signExtend;
    أرجع static_cast<int64_t>(extended);
}

string V3_عدد::منزل_String() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *هذا);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    إذا (isString()) أرجع m_معلومات.str();
    صحيح ٻٽو_انگ_2 = ويڪر() - 1;
    طالما ((ٻٽو_انگ_2 % 8) != 7) ++ٻٽو_انگ_2;
    std::string str;
    لكل (; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2 -= 8) {
        ثابت صحيح v = بٽ_قدر(ٻٽو_انگ_2 - 7, 8);
        إذا (v) str += static_cast<حرف>(v);
    }
    أرجع str;
}

uint32_t V3_عدد::edata_لفظ(صحيح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *هذا);
    أرجع m_معلومات.انگ()[eword].m_قدر;
}

uint8_t V3_عدد::معلومات_Byte(صحيح byte) ثابت {
    أرجع (edata_لفظ(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقي V3_عدد::isAllZ() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        ثابت uint32_t نقاب = i == لفظ_2() - 1 ? hi_لفظ_نقاب() : ~0U;
        إذا (((~v.m_قدر & v.m_قدر_X) & نقاب) != نقاب) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_عدد::isAllX() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    uint32_t نقاب = hi_لفظ_نقاب();
    لكل (صحيح i = لفظ_2() - 1; i >= 0; --i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        إذا ((v.m_قدر & v.m_قدر_X) ^ نقاب) أرجع خطأ;
        نقاب = ~0U;
    }
    أرجع سچ;
}
منطقي V3_عدد::isEqZero() ثابت VL_MT_محفوظ {
    إذا (isString()) أرجع m_معلومات.str().خالي_جايون();
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        إذا (v.m_قدر || v.m_قدر_X) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_عدد::isNeqZero() ثابت {
    إذا (isString()) أرجع !m_معلومات.str().خالي_جايون();
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        إذا (v.m_قدر & ~v.m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_عدد::is_بٽ_Zero(صحيح msb, صحيح lsb) ثابت {
    لكل (صحيح i = lsb; i <= msb; ++i) {
        إذا (VL_UNLIKELY(!ٻٽو_انگ_Is0(i))) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_عدد::isEqOne() ثابت {
    إذا (m_معلومات.انگ()[0].m_قدر != 1 || m_معلومات.انگ()[0].m_قدر_X) أرجع خطأ;
    لكل (صحيح i = 1; i < لفظ_2(); ++i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        إذا (v.m_قدر || v.m_قدر_X) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_عدد::isEqAllOnes(صحيح optwidth) ثابت {
    // Correct number of zero bits/width matters
    إذا (!optwidth) optwidth = ويڪر();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < optwidth; ++ٻٽو_انگ_2) {
        إذا (!ٻٽو_انگ_Is1(ٻٽو_انگ_2)) أرجع خطأ;
    }
    أرجع سچ;
}
منطقي V3_عدد::isFourState() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        إذا (m_معلومات.انگ()[i].m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isAnyX() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        إذا (v.m_قدر & v.m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isAnyXZ() ثابت {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        إذا (m_معلومات.انگ()[i].m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isAnyZ() ثابت VL_MT_محفوظ {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        ثابت قدر_And_X v = m_معلومات.انگ()[i];
        إذا (~v.m_قدر & v.m_قدر_X) أرجع سچ;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isLtXZ(ثابت V3_عدد& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    إذا (isString()) أرجع منزل_String() < rhs.منزل_String();
    إذا (isDouble()) أرجع منزل_Double() < rhs.منزل_Double();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (ٻٽو_انگ_Is1(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) أرجع سچ;
        إذا (rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && ٻٽو_انگ_Is0(ٻٽو_انگ_2)) أرجع خطأ;
        إذا (ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) أرجع سچ;
        إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) أرجع خطأ;
    }
    أرجع خطأ;
}
صحيح V3_عدد::ڳڻپ_X(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ {
    صحيح ڳڻپ = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= ويڪر()) أرجع ڳڻپ;
        إذا (ٻٽو_انگ_Is_X(lsb + bitn)) ++ڳڻپ;
    }
    أرجع ڳڻپ;
}
صحيح V3_عدد::ڳڻپ_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ {
    صحيح ڳڻپ = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= ويڪر()) أرجع ڳڻپ;
        إذا (ٻٽو_انگ_Is_Z(lsb + bitn)) ++ڳڻپ;
    }
    أرجع ڳڻپ;
}

صحيح V3_عدد::ويڪر_منزل_Fit() ثابت {
    لكل (صحيح ٻٽو_انگ_2 = ويڪر() - 1; ٻٽو_انگ_2 > 0; ٻٽو_انگ_2--) {
        إذا (!ٻٽو_انگ_Is0(ٻٽو_انگ_2)) أرجع ٻٽو_انگ_2 + 1;
    }
    أرجع 1;  // one bit even if number is == 0
}

uint32_t V3_عدد::ڳڻپ_بٽ(ثابت V3_عدد& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        بدل (ctrl.ٻٽو_انگ_Is(0)) {
        حالة '0':
            إذا (ٻٽو_انگ_Is0(ٻٽو_انگ_2)) ++n;
            اكسر;
        حالة '1':
            إذا (ٻٽو_انگ_Is1(ٻٽو_انگ_2)) ++n;
            اكسر;
        حالة 'x':
            إذا (ٻٽو_انگ_Is_X(ٻٽو_انگ_2)) ++n;
            اكسر;
        حالة 'z':
            إذا (ٻٽو_انگ_Is_Z(ٻٽو_انگ_2)) ++n;
            اكسر;
        }
    }
    أرجع n;
}

uint32_t V3_عدد::ڳڻپ_بٽ(ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                             ثابت V3_عدد& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = ڳڻپ_بٽ(ctrl1);
    إذا (ctrl2.ٻٽو_انگ_Is(0) != ctrl1.ٻٽو_انگ_Is(0)) n += ڳڻپ_بٽ(ctrl2);
    إذا ((ctrl3.ٻٽو_انگ_Is(0) != ctrl1.ٻٽو_انگ_Is(0)) && (ctrl3.ٻٽو_انگ_Is(0) != ctrl2.ٻٽو_انگ_Is(0))) {
        n += ڳڻپ_بٽ(ctrl3);
    }
    أرجع n;
}

uint32_t V3_عدد::ڳڻپ_Ones() ثابت {
    صحيح n = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (ٻٽو_انگ_Is1(ٻٽو_انگ_2)) ++n;
    }
    أرجع n;
}

uint32_t V3_عدد::most_سيٽ_ٻٽو_انگ_P1() ثابت {
    لكل (صحيح ٻٽو_انگ_2 = ويڪر() - 1; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2--) {
        إذا (!ٻٽو_انگ_Is0(ٻٽو_انگ_2)) أرجع ٻٽو_انگ_2 + 1;
    }
    أرجع 0;
}

uint32_t V3_عدد::گهٽ_کان_گهٽ_سيٽ_ٻٽو_انگ_P1() ثابت {
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (!ٻٽو_انگ_Is0(ٻٽو_انگ_2)) أرجع ٻٽو_انگ_2 + 1;
    }
    أرجع 0;
}

//======================================================================

V3_عدد& V3_عدد::op_بٽ_Non_XZ(ثابت V3_عدد& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2) || lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_بٽ_One(ثابت V3_عدد& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_بٽ_XZ(ثابت V3_عدد& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_بٽ_Z(ثابت V3_عدد& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is_Z(ٻٽو_انگ_2)) سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
    }
    أرجع *هذا;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_عدد& V3_عدد::opRedOr(ثابت V3_عدد& lhs) {
    // op i, 1 bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < lhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            أرجع سيٽ_Single_بٽ(1);
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opRedAnd(ثابت V3_عدد& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < lhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
            أرجع سيٽ_Single_بٽ(0);
        } وإلا إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opRedXor(ثابت V3_عدد& lhs) {
    // op i, 1 bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < lhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            إذا (outc == 1) {
                outc = 0;
            } وإلا إذا (outc == 0) {
                outc = 1;
            }
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::op_ڳڻپ_بٽ(ثابت V3_عدد& اظهار, ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                                ثابت V3_عدد& ctrl3) {
    // Correct number of zero bits/width matters
    انگ_ASSERT_OP_ARGS4(اظهار, ctrl1, ctrl2, ctrl3);
    انگ_ASSERT_LOGIC_ARGS4(اظهار, ctrl1, ctrl2, ctrl3);
    سيٽ_Zero();
    m_معلومات.انگ()[0].m_قدر = اظهار.ڳڻپ_بٽ(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_ڳڻپ_Ones(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    سيٽ_Zero();
    m_معلومات.انگ()[0].m_قدر = lhs.ڳڻپ_Ones();
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::opIsUnknown(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    أرجع سيٽ_Single_بٽ(lhs.isAnyXZ());
}
V3_عدد& V3_عدد::opOneHot(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    أرجع سيٽ_Single_بٽ(lhs.ڳڻپ_Ones() == 1);
}
V3_عدد& V3_عدد::opOneHot0(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    أرجع سيٽ_Single_بٽ(lhs.ڳڻپ_Ones() <= 1);
}
V3_عدد& V3_عدد::opCLog2(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحيح adjust = (lhs.ڳڻپ_Ones() == 1) ? 0 : 1;
    لكل (صحيح ٻٽو_انگ_2 = lhs.ويڪر() - 1; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2--) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            سيٽ_Long(ٻٽو_انگ_2 + adjust);
            أرجع *هذا;
        }
    }
    سيٽ_Zero();
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_Most_سيٽ_ٻٽو_انگ_P1(ثابت V3_عدد& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    لكل (صحيح ٻٽو_انگ_2 = lhs.ويڪر() - 1; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2--) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            سيٽ_Long(ٻٽو_انگ_2 + 1);
            أرجع *هذا;
        }
    }
    سيٽ_Zero();
    أرجع *هذا;
}

V3_عدد& V3_عدد::opLogNot(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < lhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            outc = 0;
            اذهب آخري;
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opNot(ثابت V3_عدد& lhs) {
    // Correct number of zero bits/width matters
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        } وإلا إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2) || rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {  // 0
        } وإلا {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) || rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
            // 0
        } وإلا {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opXor(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        } وإلا إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2) || rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'x');
        }
        // else zero
    }
    أرجع *هذا;
}

فراغ V3_عدد::fill_بٽ(صحيح destLsb, صحيح ويڪر, حرف قدر) {
    UASSERT(destLsb >= 0 && ويڪر >= 0, "Negative bit range");
    إذا (!ويڪر) أرجع;
    UASSERT(ويڪر <= هذا->ويڪر() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t قدر_لفظ = قدر == '1' || قدر == 'x' || قدر == 1 || قدر == 3 ? ~0U : 0;
    ثابت uint32_t x_لفظ = قدر == 'z' || قدر == 'x' || قدر == 2 || قدر == 3 ? ~0U : 0;
    ثابت صحيح پهريون_لفظ = VL_BITWORD_E(destLsb);
    ثابت صحيح آخري_لفظ = VL_BITWORD_E(destLsb + ويڪر - 1);
    لكل (صحيح لفظ = پهريون_لفظ; لفظ <= آخري_لفظ; ++لفظ) {
        ثابت صحيح لفظ_Lsb = لفظ == پهريون_لفظ ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحيح لفظ_Msb = لفظ == آخري_لفظ ? VL_BITBIT_E(destLsb + ويڪر - 1) : VL_SIZEBITS_E;
        ثابت uint32_t نقاب = VL_نقاب_E(لفظ_Msb - لفظ_Lsb + 1) << لفظ_Lsb;
        قدر_And_X& dest = m_معلومات.انگ()[لفظ];
        dest.m_قدر = (dest.m_قدر & ~نقاب) | (قدر_لفظ & نقاب);
        dest.m_قدر_X = (dest.m_قدر_X & ~نقاب) | (x_لفظ & نقاب);
    }
}

فراغ V3_عدد::copy_بٽ(صحيح destLsb, ثابت V3_عدد& ذريعو, صحيح ذريعو_Lsb, صحيح ويڪر) {
    UASSERT(destLsb >= 0 && ذريعو_Lsb >= 0 && ويڪر >= 0, "Negative bit range");
    إذا (!ويڪر) أرجع;
    UASSERT(ويڪر <= هذا->ويڪر() - destLsb, "Destination bit range exceeds number width");
    UASSERT(ويڪر <= ذريعو.ويڪر() - ذريعو_Lsb, "Source bit range exceeds number width");

    ثابت صحيح پهريون_لفظ = VL_BITWORD_E(destLsb);
    ثابت صحيح آخري_لفظ = VL_BITWORD_E(destLsb + ويڪر - 1);
    لكل (صحيح dest_لفظ = پهريون_لفظ; dest_لفظ <= آخري_لفظ; ++dest_لفظ) {
        ثابت صحيح dest_ٻٽو_انگ = std::max(destLsb, dest_لفظ * VL_EDATASIZE);
        ثابت صحيح dest_وٿي = VL_BITBIT_E(dest_ٻٽو_انگ);
        ثابت صحيح ذريعو_ٻٽو_انگ = ذريعو_Lsb + dest_ٻٽو_انگ - destLsb;
        ثابت صحيح ذريعو_لفظ = VL_BITWORD_E(ذريعو_ٻٽو_انگ);
        ثابت صحيح ذريعو_وٿي = VL_BITBIT_E(ذريعو_ٻٽو_انگ);
        ثابت صحيح copy_ويڪر = std::min(ويڪر - (dest_ٻٽو_انگ - destLsb), VL_EDATASIZE - dest_وٿي);
        ثابت uint32_t نقاب = VL_نقاب_E(copy_ويڪر) << dest_وٿي;

        ثابت قدر_And_X& ذريعو_Lo = ذريعو.m_معلومات.انگ()[ذريعو_لفظ];
        uint32_t قدر = ذريعو_Lo.m_قدر >> ذريعو_وٿي;
        uint32_t قدر_X = ذريعو_Lo.m_قدر_X >> ذريعو_وٿي;
        إذا (ذريعو_وٿي && copy_ويڪر > VL_EDATASIZE - ذريعو_وٿي) {
            ثابت قدر_And_X& ذريعو_Hi = ذريعو.m_معلومات.انگ()[ذريعو_لفظ + 1];
            قدر |= ذريعو_Hi.m_قدر << (VL_EDATASIZE - ذريعو_وٿي);
            قدر_X |= ذريعو_Hi.m_قدر_X << (VL_EDATASIZE - ذريعو_وٿي);
        }

        قدر_And_X& dest = m_معلومات.انگ()[dest_لفظ];
        dest.m_قدر = (dest.m_قدر & ~نقاب) | ((قدر << dest_وٿي) & نقاب);
        dest.m_قدر_X = (dest.m_قدر_X & ~نقاب) | ((قدر_X << dest_وٿي) & نقاب);
    }
}

V3_عدد& V3_عدد::opConcat(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سيٽ_Zero();
    // See also error in V3Width
    إذا (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_بٽ(0, rhs, 0, rhs.ويڪر());
    copy_بٽ(rhs.ويڪر(), lhs, 0, lhs.ويڪر());
    أرجع *هذا;
}

V3_عدد& V3_عدد::opLenN(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    سيٽ_Quad(lhs.منزل_String().ڊيگهائي());
    أرجع *هذا;
}

V3_عدد& V3_عدد::opRepl(ثابت V3_عدد& lhs,
                           ثابت V3_عدد& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    أرجع opRepl(lhs, rhs.منزل_UInt());
}

V3_عدد& V3_عدد::opRepl(ثابت V3_عدد& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    إذا (v3Global.opt.replication_حد_بندي()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_حد_بندي())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_حد_بندي() << " is suspect: " << rhsval);
    }
    سيٽ_Zero();
    لكل (بنانشان times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.ويڪر();
        إذا (destLsb >= static_cast<uint32_t>(ويڪر())) اكسر;
        copy_بٽ(static_cast<صحيح>(destLsb), lhs, 0,
                 std::min(lhs.ويڪر(), ويڪر() - static_cast<صحيح>(destLsb)));
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_وهڪرو_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سيٽ_Zero();
    // See also error in V3Width
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحيح ssize = std::min(rhs.منزل_UInt(), static_cast<بنانشان>(lhs.ويڪر()));
    لكل (صحيح istart = 0; istart < lhs.ويڪر(); istart += ssize) {
        ثابت صحيح ostart = std::max(0, lhs.ويڪر() - ssize - istart);
        copy_بٽ(ostart, lhs, istart, std::min(ssize, lhs.ويڪر() - istart));
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opLogAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < lhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            loutc = 1;
            اكسر;
        }
        إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2) && loutc == 0) loutc = 'x';
    }
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < rhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            routc = 1;
            اكسر;
        }
        إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    إذا (routc == 1 && loutc == 1) outc = 1;
    إذا (routc == 0 || loutc == 0) outc = 0;
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opLogOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < lhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            outc = 1;
            اذهب آخري;
        }
        إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2) && outc == 0) outc = 'x';
    }
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < rhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            outc = 1;
            اذهب آخري;
        }
        إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2) && outc == 0) outc = 'x';
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opLogIf(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد lnot = lhs;
    lnot.opLogNot(lhs);
    أرجع opLogOr(lnot, rhs);
}

V3_عدد& V3_عدد::opLogEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_عدد ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    أرجع opLogAnd(ifa, ifb);
}

V3_عدد& V3_عدد::opAtoN(ثابت V3_عدد& lhs, صحيح بنياد) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    UASSERT(بنياد == AstAtoN::ATOREAL || بنياد == 2 || بنياد == 8 || بنياد == 10 || بنياد == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.منزل_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.پڇاڙي(), '_'), str.پڇاڙي());

    إذا (بنياد == AstAtoN::ATOREAL) أرجع سيٽ_Double(std::atof(str.c_str()));

    errno = 0;
    خودڪار v = std::strtol(str.c_str(), خالي_اشارو, بنياد);
    إذا (errno != 0) v = 0;
    أرجع سيٽ_Long_S(static_cast<int32_t>(v));
}

V3_عدد& V3_عدد::opPutcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths) {
    انگ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    انگ_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    ثابت int32_t c = ths.منزل_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.ڊيگهائي() && c != 0) lstring[i] = c;
    أرجع سيٽ_String(lstring);
}

V3_عدد& V3_عدد::opGetcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.ڊيگهائي()) v = lstring[i];
    أرجع سيٽ_Long(v);
}

V3_عدد& V3_عدد::opSubstrN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths) {
    انگ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    انگ_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    ثابت int32_t j = ths.منزل_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    إذا (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.ڊيگهائي()) أرجع سيٽ_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    أرجع سيٽ_String(lstring.substr(i, j - i + 1));
}

V3_عدد& V3_عدد::opCompareNN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, منطقي ignoreCase) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحيح نتيجو;
    ثابت string lstring = lhs.منزل_String();
    ثابت string rstring = rhs.منزل_String();
    إذا (ignoreCase) {
        نتيجو = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } وإلا {
        نتيجو = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    أرجع سيٽ_Long_S(نتيجو);
}

V3_عدد& V3_عدد::opEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opEqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opEqD(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
            outc = 0;
            اذهب آخري;
        }
        إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            outc = 0;
            اذهب آخري;
        }
        إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
        إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opNeqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opNeqD(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) {
            outc = 1;
            اذهب آخري;
        }
        إذا (lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            outc = 1;
            اذهب آخري;
        }
        إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
        إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3Hash V3_عدد::منزل_Hash() ثابت {
    // Shouldmatch isCaseEq
    إذا (isString()) أرجع V3Hash{منزل_String()};
    إذا (isDouble()) أرجع V3Hash{منزل_Double()};
    V3Hash hash{ويڪر()};
    لكل (صحيح i = 0; i < لفظ_2(); ++i) hash += m_معلومات.انگ()[i].m_قدر;
    أرجع hash;
}

منطقي V3_عدد::isCaseEq(ثابت V3_عدد& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    إذا (isString()) أرجع منزل_String() == rhs.منزل_String();
    إذا (isDouble()) أرجع منزل_Double() == rhs.منزل_Double();
    إذا (ويڪر() != rhs.ويڪر()) أرجع خطأ;
    لكل (صحيح i = 0; i < لفظ_2(); ++i) {
        إذا (!(m_معلومات.انگ()[i] == rhs.m_معلومات.انگ()[i])) أرجع خطأ;
    }
    أرجع سچ;
}

V3_عدد& V3_عدد::opCaseEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_عدد& V3_عدد::opCaseNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    إذا (lhs.isString()) {
        أرجع opNeqN(lhs, rhs);
    } وإلا إذا (lhs.isDouble()) {
        أرجع opNeqD(lhs, rhs);
    }
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is(ٻٽو_انگ_2) != rhs.ٻٽو_انگ_Is(ٻٽو_انگ_2)) {
            outc = 1;
            اذهب آخري;
        }
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opWildEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (!rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
            إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
                outc = 'x';
                اذهب آخري;
            }
            إذا (lhs.ٻٽو_انگ_Is(ٻٽو_انگ_2) != rhs.ٻٽو_انگ_Is(ٻٽو_انگ_2)) {
                outc = 0;
                اذهب آخري;
            }
        }
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opWildNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (!rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
            إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
                outc = 'x';
                اذهب آخري;
            }
            إذا (lhs.ٻٽو_انگ_Is(ٻٽو_انگ_2) != rhs.ٻٽو_انگ_Is(ٻٽو_انگ_2)) {
                outc = 1;
                اذهب آخري;
            }
        }
    }
آخري:
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opGt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر(), rhs.ويڪر()); ++ٻٽو_انگ_2) {
        إذا (lhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) outc = 1;
        إذا (rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2) && lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) outc = 0;
        إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
        إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
    }
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opGtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحيح mbit = std::max(lhs.ويڪر() - 1, rhs.ويڪر() - 1);
        إذا (lhs.ٻٽو_انگ_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (rhs.ٻٽو_انگ_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (lhs.ٻٽو_انگ_Is0(mbit) && rhs.ٻٽو_انگ_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } وإلا إذا (lhs.ٻٽو_انگ_Is1_Extend(mbit) && rhs.ٻٽو_انگ_Is0(mbit)) {
            outc = 0;  // - !> +
        } وإلا {
            // both positive or negative, normal >
            لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < std::max(lhs.ويڪر() - 1, rhs.ويڪر() - 1); ++ٻٽو_انگ_2) {
                إذا (lhs.ٻٽو_انگ_Is1_Extend(ٻٽو_انگ_2) && rhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) outc = 1;
                إذا (rhs.ٻٽو_انگ_Is1_Extend(ٻٽو_انگ_2) && lhs.ٻٽو_انگ_Is0(ٻٽو_انگ_2)) outc = 0;
                إذا (lhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
                إذا (rhs.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) outc = 'x';
            }
        }
    }
    أرجع سيٽ_Single_بٽ(outc);
}

V3_عدد& V3_عدد::opGte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGt(lhs, rhs);
}

V3_عدد& V3_عدد::opGteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGtS(lhs, rhs);
}

V3_عدد& V3_عدد::opLt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGt(rhs, lhs); }
V3_عدد& V3_عدد::opLte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGte(rhs, lhs); }
V3_عدد& V3_عدد::opLtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGtS(rhs, lhs); }
V3_عدد& V3_عدد::opLteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGteS(rhs, lhs); }

V3_عدد& V3_عدد::op_سرڪايو_R(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // L(lhs) bit return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    سيٽ_Zero();
    إذا (rhs.ويڪر() > 32 && !rhs.is_بٽ_Zero(rhs.ويڪر() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.منزل_UInt();
    إذا (rhsval < static_cast<uint32_t>(lhs.ويڪر())) {
        copy_بٽ(0, lhs, static_cast<صحيح>(rhsval),
                 std::min(ويڪر(), lhs.ويڪر() - static_cast<صحيح>(rhsval)));
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_سرڪايو_RS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    سيٽ_Zero();
    ثابت منطقي حد_کان_مٿي = rhs.ويڪر() > 32 && !rhs.is_بٽ_Zero(rhs.ويڪر() - 1, 32);
    إذا (!حد_کان_مٿي) {
        ثابت uint32_t rhsval = rhs.منزل_UInt();
        إذا (rhsval < lbits) {
            ثابت صحيح copy_ويڪر = std::min(ويڪر(), static_cast<صحيح>(lbits - rhsval));
            copy_بٽ(0, lhs, static_cast<صحيح>(rhsval), copy_ويڪر);
            fill_بٽ(copy_ويڪر, ويڪر() - copy_ويڪر, lhs.ٻٽو_انگ_Is(lbits - 1));
            أرجع *هذا;
        }
    }
    fill_بٽ(0, ويڪر(), lhs.ٻٽو_انگ_Is(lbits - 1));  // '0/'1/'x/'z
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_سرڪايو_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // L(lhs) bit return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    سيٽ_Zero();
    إذا (rhs.ويڪر() > 32 && !rhs.is_بٽ_Zero(rhs.ويڪر() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.منزل_UInt();
    إذا (rhsval < static_cast<uint32_t>(ويڪر())) {
        copy_بٽ(static_cast<صحيح>(rhsval), lhs, 0,
                 std::min(ويڪر() - static_cast<صحيح>(rhsval), lhs.ويڪر()));
    }
    أرجع *هذا;
}

//======================================================================
// Ops - Arithmetic

V3_عدد& V3_عدد::opNegate(ثابت V3_عدد& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    V3_عدد notlhs(&lhs, ويڪر());
    notlhs.opNot(lhs);
    ثابت V3_عدد one(&lhs, ويڪر(), 1);
    op_جوڙيو(notlhs, one);
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_جوڙيو(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    سيٽ_Zero();
    // Addem
    uint64_t حاصل = 0;
    لكل (صحيح لفظ = 0; لفظ < لفظ_2(); ++لفظ) {
        ثابت uint64_t lwordval = lhs.m_معلومات.انگ()[لفظ].m_قدر;
        ثابت uint64_t rwordval = rhs.m_معلومات.انگ()[لفظ].m_قدر;
        ثابت uint64_t جوڙ = lwordval + rwordval + حاصل;
        m_معلومات.انگ()[لفظ].m_قدر = جوڙ & 0xffffffffULL;
        حاصل = جوڙ > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    أرجع *هذا;
}
V3_عدد& V3_عدد::opSub(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    V3_عدد negrhs(&rhs, ويڪر());
    negrhs.opNegate(rhs);
    أرجع op_جوڙيو(lhs, negrhs);
}
V3_عدد& V3_عدد::opMul(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    سيٽ_Zero();
    إذا (ويڪر() <= 64) {
        سيٽ_Quad(lhs.منزل_UQuad() * rhs.منزل_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } وإلا {
        لكل (صحيح lword = 0; lword < lhs.لفظ_2(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_معلومات.انگ()[lword].m_قدر);
            إذا (lwordval == 0) استمر;
            لكل (صحيح rword = 0; rword < rhs.لفظ_2(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_معلومات.انگ()[rword].m_قدر);
                إذا (rwordval == 0) استمر;
                uint64_t mul = lwordval * rwordval;
                لكل (صحيح qword = lword + rword; qword < لفظ_2(); ++qword) {
                    mul += static_cast<uint64_t>(m_معلومات.انگ()[qword].m_قدر);
                    m_معلومات.انگ()[qword].m_قدر = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    إذا (mul == 0) اكسر;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opMulS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    V3_عدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_عدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_عدد qNoSign = opMul(lhsNoSign, rhsNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_سونپيو(qNoSign);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    إذا (rhs.isEqZero()) أرجع سيٽ_All_بٽ_XRemoved();
    إذا (lhs.ويڪر() <= 64) {
        سيٽ_Quad(lhs.منزل_UQuad() / rhs.منزل_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide division
        أرجع opModDivGuts(lhs, rhs, خطأ);
    }
}
V3_عدد& V3_عدد::opDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    إذا (rhs.isEqZero()) أرجع سيٽ_All_بٽ_XRemoved();
    V3_عدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_عدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_عدد qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_سونپيو(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *هذا);
    أرجع *هذا;
}
V3_عدد& V3_عدد::opModDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    إذا (rhs.isEqZero()) أرجع سيٽ_All_بٽ_XRemoved();
    إذا (lhs.ويڪر() <= 64) {
        سيٽ_Quad(lhs.منزل_UQuad() % rhs.منزل_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide modulus
        أرجع opModDivGuts(lhs, rhs, سچ);
    }
}
V3_عدد& V3_عدد::opModDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    إذا (rhs.isEqZero()) أرجع سيٽ_All_بٽ_XRemoved();
    V3_عدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_عدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_عدد qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    إذا (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } وإلا {
        op_سونپيو(qNoSign);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opModDivGuts(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, منطقي is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سيٽ_Zero();
    // Find MSB and check for zero.
    ثابت صحيح l_لفظ = lhs.لفظ_2();
    ثابت صحيح umsbp1 = lhs.most_سيٽ_ٻٽو_انگ_P1();  // dividend
    ثابت صحيح vmsbp1 = rhs.most_سيٽ_ٻٽو_انگ_P1();  //ورهائيندڙ
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
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_معلومات.انگ()[j].m_قدر));
            m_معلومات.انگ()[j].m_قدر = unw64 / static_cast<uint64_t>(rhs.m_معلومات.انگ()[0].m_قدر);
            k = unw64
                - (static_cast<uint64_t>(m_معلومات.انگ()[j].m_قدر)
                   * static_cast<uint64_t>(rhs.m_معلومات.انگ()[0].m_قدر));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *هذا << " rem=0x" << std::hex
                                   << k << std::dec);
        إذا (is_modulus) {
            سيٽ_Zero();
            m_معلومات.انگ()[0].m_قدر = k;
        }
        opCleanThis();
        أرجع *هذا;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_لفظ + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_لفظ + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    لكل (صحيح i = 0; i < l_لفظ; ++i) m_معلومات.انگ()[i].m_قدر = 0;
    لكل (صحيح i = 0; i < l_لفظ + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحيح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t سرڪايو_نقاب = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    لكل (صحيح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_معلومات.انگ()[i].m_قدر << s)
                | (سرڪايو_نقاب & (rhs.m_معلومات.انگ()[i - 1].m_قدر >> (32 - s)));
    }
    vn[0] = rhs.m_معلومات.انگ()[0].m_قدر << s;

    // Copy and shift dividend by same amount; may set new upper word
    إذا (s) {
        un[uw] = lhs.m_معلومات.انگ()[uw - 1].m_قدر >> (32 - s);
    } وإلا {
        un[uw] = 0;
    }
    لكل (صحيح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_معلومات.انگ()[i].m_قدر << s)
                | (سرڪايو_نقاب & (lhs.m_معلومات.انگ()[i - 1].m_قدر >> (32 - s)));
    }
    un[0] = lhs.m_معلومات.انگ()[0].m_قدر << s;

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
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //ڪٽيو
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_معلومات.انگ()[j].m_قدر = qhat;  // Save quotient digit

        إذا (t < 0) {
            // Over subtracted; correct by adding back
            m_معلومات.انگ()[j].m_قدر--;
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
            m_معلومات.انگ()[i].m_قدر = (un[i] >> s) | (سرڪايو_نقاب & (un[i + 1] << (32 - s)));
        }
        لكل (صحيح i = vw; i < l_لفظ; ++i) m_معلومات.انگ()[i].m_قدر = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    } وإلا {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    }
}

V3_عدد& V3_عدد::opPow(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, منطقي lsign, منطقي rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سيٽ_All_بٽ_X();
    إذا (rhs.isEqZero()) أرجع سيٽ_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    إذا (rsign && rhs.isNegative()) {
        إذا (lhs.isEqZero()) {
            أرجع سيٽ_All_بٽ_XRemoved();
        } وإلا إذا (lhs.isEqOne()) {
            أرجع سيٽ_Value1();
        } وإلا إذا (lsign && lhs.isEqAllOnes()) {
            إذا (rhs.ٻٽو_انگ_Is1(0)) {
                أرجع سيٽ_All_Bits1();  // -1^odd=-1
            } وإلا {
                أرجع سيٽ_Value1();  // -1^even=1
            }
        }
        أرجع سيٽ_Zero();
    }
    إذا (lhs.isEqZero()) أرجع سيٽ_Zero();
    سيٽ_Zero();
    m_معلومات.انگ()[0].m_قدر = 1;
    V3_عدد power(&lhs, ويڪر());
    power.op_سونپيو(lhs);
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < rhs.ويڪر(); ++ٻٽو_انگ_2) {
        إذا (ٻٽو_انگ_2 > 0) {  // power = power*power
            V3_عدد آخري_Power(&lhs, ويڪر());
            آخري_Power.op_سونپيو(power);
            power.opMul(آخري_Power, آخري_Power);
        }
        إذا (rhs.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {  // out *= power
            V3_عدد آخري_ٻاهريون(&lhs, ويڪر());
            آخري_ٻاهريون.op_سونپيو(*هذا);
            opMul(آخري_ٻاهريون, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opPowSU(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, سچ, خطأ);
}
V3_عدد& V3_عدد::opPowSS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, سچ, سچ);
}
V3_عدد& V3_عدد::opPowUS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, خطأ, سچ);
}

V3_عدد& V3_عدد::opBufIf1(ثابت V3_عدد& ens, ثابت V3_عدد& if1s) {
    انگ_ASSERT_OP_ARGS2(ens, if1s);
    انگ_ASSERT_LOGIC_ARGS2(ens, if1s);
    سيٽ_Zero();
    لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 < ويڪر(); ++ٻٽو_انگ_2) {
        إذا (ens.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, if1s.ٻٽو_انگ_Is(ٻٽو_انگ_2));
        } وإلا {
            سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'z');
        }
    }
    أرجع *هذا;
}

// Sets all bits in range to the given value
V3_عدد& V3_عدد::op_سيٽ_Range(uint32_t lsb, uint32_t ويڪر, حرف ٻٽو_انگ_قدر) {
    إذا (lsb < static_cast<uint32_t>(هذا->ويڪر())) {
        fill_بٽ(static_cast<صحيح>(lsb),
                 static_cast<صحيح>(std::min(ويڪر, static_cast<uint32_t>(هذا->ويڪر()) - lsb)),
                 ٻٽو_انگ_قدر);
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_سونپيو(ثابت V3_عدد& lhs) { أرجع op_سونپيو_Non_XZ(lhs, خطأ); }
V3_عدد& V3_عدد::op_سونپيو_Non_XZ(ثابت V3_عدد& lhs, منطقي ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    إذا (هذا != &lhs) {
        إذا (VL_UNLIKELY(lhs.isNull())) {
            m_معلومات.m_isNull = سچ;
        } وإلا إذا (isString()) {
            إذا (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_معلومات.str() = lhs.منزل_String();
            } وإلا {
                m_معلومات.str() = lhs.m_معلومات.str();
            }
        } وإلا إذا (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            سيٽ_Zero();
        } وإلا إذا (lhs.isDouble()) {
            سيٽ_Double(lhs.منزل_Double());
        } وإلا إذا (!ignoreXZ) {
            سيٽ_Zero();
            copy_بٽ(0, lhs, 0, std::min(ويڪر(), lhs.ويڪر()));
        } وإلا {
            لكل (صحيح لفظ = 0; لفظ < لفظ_2(); ++لفظ) {
                ثابت uint32_t قدر = لفظ < lhs.لفظ_2() ? lhs.m_معلومات.انگ()[لفظ].m_قدر
                                                                & ~lhs.m_معلومات.انگ()[لفظ].m_قدر_X
                                                          : 0;
                m_معلومات.انگ()[لفظ] = {قدر, 0};
            }
            opCleanThis();
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opNToI(ثابت V3_عدد& lhs) {
    // String to packed number
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    سيٽ_Zero();
    ثابت string& str = lhs.منزل_String();
    لكل (سائيز_t n = 0; n < str.ڊيگهائي(); ++n) {
        ثابت حرف c = str[str.ڊيگهائي() - 1 - n];
        لكل (سائيز_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<بنانشان ڊگهو ڊگهو>(c)};
            سيٽ_ٻٽو_انگ(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opExtendS(ثابت V3_عدد& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    ثابت صحيح copy_ويڪر = std::min(ويڪر(), static_cast<صحيح>(lbits));
    copy_بٽ(0, lhs, 0, copy_ويڪر);
    fill_بٽ(copy_ويڪر, ويڪر() - copy_ويڪر, lhs.ٻٽو_انگ_Is(lbits - 1));
    أرجع *هذا;
}

V3_عدد& V3_عدد::opExtendXZ(ثابت V3_عدد& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    ثابت صحيح copy_ويڪر = std::min(ويڪر(), static_cast<صحيح>(lbits));
    copy_بٽ(0, lhs, 0, copy_ويڪر);
    fill_بٽ(copy_ويڪر, ويڪر() - copy_ويڪر, lhs.ٻٽو_انگ_Is(lbits - 1));
    أرجع *هذا;
}

V3_عدد& V3_عدد::opClean(ثابت V3_عدد& lhs, uint32_t بٽ) { أرجع opSel(lhs, بٽ - 1, 0); }

فراغ V3_عدد::opCleanThis(منطقي warnOnTruncation) {
    // Clean MSB of number
    انگ_ASSERT_LOGIC_ARGS1(*هذا);
    ثابت قدر_And_X v = m_معلومات.انگ()[لفظ_2() - 1];
    ثابت uint32_t new_قدر_Msb = v.m_قدر & hi_لفظ_نقاب();
    ثابت uint32_t new_قدر_XMsb = v.m_قدر_X & hi_لفظ_نقاب();
    إذا (warnOnTruncation && (new_قدر_Msb != v.m_قدر || new_قدر_XMsb != v.m_قدر_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << ويڪر() << " bit number: " << displayed("%d"));
    }
    m_معلومات.انگ()[لفظ_2() - 1] = {new_قدر_Msb, new_قدر_XMsb};
}

V3_عدد& V3_عدد::opSel(ثابت V3_عدد& lhs, ثابت V3_عدد& msb, ثابت V3_عدد& lsb) {
    انگ_ASSERT_OP_ARGS3(lhs, msb, lsb);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS2(msb, lsb);
    إذا (lsb.isFourState() || msb.isFourState()) أرجع سيٽ_All_بٽ_X();
    أرجع opSel(lhs, msb.منزل_UInt(), lsb.منزل_UInt());
}

V3_عدد& V3_عدد::opSel(ثابت V3_عدد& lhs, uint32_t msbval, uint32_t lsbval) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    سيٽ_Zero();
    صحيح copy_ويڪر = 0;
    إذا (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.ويڪر())) {
        ثابت uint64_t selected_ويڪر = uint64_t{msbval} - lsbval + 1;
        copy_ويڪر = static_cast<صحيح>(
            std::min({static_cast<uint64_t>(ويڪر()), static_cast<uint64_t>(lhs.ويڪر()) - lsbval,
                      selected_ويڪر}));
    }
    copy_بٽ(0, lhs, copy_ويڪر ? static_cast<صحيح>(lsbval) : 0, copy_ويڪر);
    fill_بٽ(copy_ويڪر, ويڪر() - copy_ويڪر, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    أرجع *هذا;
}

V3_عدد& V3_عدد::opSelInto(ثابت V3_عدد& lhs, ثابت V3_عدد& lsb, صحيح ويڪر) {
    أرجع opSelInto(lhs, lsb.منزل_SInt(), ويڪر);
}

V3_عدد& V3_عدد::opSelInto(ثابت V3_عدد& lhs, صحيح lsbval, صحيح ويڪر) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحيح copy_ويڪر = std::max(0, std::min({ويڪر, lhs.ويڪر(), هذا->ويڪر() - lsbval}));
    copy_بٽ(lsbval, lhs, 0, copy_ويڪر);
    ثابت صحيح fill_ويڪر
        = std::max(0, std::min(ويڪر - copy_ويڪر, هذا->ويڪر() - lsbval - copy_ويڪر));
    fill_بٽ(lsbval + copy_ويڪر, fill_ويڪر, v3Global.constRemoveXs() ? 0 : 'x');
    أرجع *هذا;
}

//======================================================================
// Ops - Floating point

V3_عدد& V3_عدد::opIToRD(ثابت V3_عدد& lhs, منطقي isSigned) {
    // Correct number of zero bits/width matters
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_عدد noxz(lhs);
    noxz.op_سونپيو_Non_XZ(lhs);
    مزدوج d = 0;
    ثابت منطقي negate = isSigned && noxz.isNegative();
    إذا (negate) {
        ثابت V3_عدد noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    لكل (صحيح ٻٽو_انگ_2 = noxz.ويڪر() - 1; ٻٽو_انگ_2 >= 0; ٻٽو_انگ_2--) {
        // Some precision might be lost in this add, that's what we want
        إذا (noxz.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) d += exp2(ٻٽو_انگ_2);
    }
    إذا (negate) d = -d;
    أرجع سيٽ_Double(d);
}
V3_عدد& V3_عدد::opRToIS(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_TRUNC(lhs.منزل_Double());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    أرجع سيٽ_Long_S(i);
}
V3_عدد& V3_عدد::opRToIRoundS(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_ڦيرو(lhs.منزل_Double());
    سيٽ_Zero();
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
        لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 <= 52; ++ٻٽو_انگ_2) {
            إذا (mantissa & (1ULL << ٻٽو_انگ_2)) {
                ثابت صحيح outbit = ٻٽو_انگ_2 + lsb;
                إذا (outbit >= 0) سيٽ_ٻٽو_انگ(outbit, 1);
            }
        }
        إذا (v < 0) {
            ثابت V3_عدد noSign = *هذا;
            opNegate(noSign);
        }
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_Real_منزل_بٽ(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.ويڪر() == 64 && ويڪر() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.منزل_Double();
    أرجع سيٽ_Quad(u.m_v);
}
V3_عدد& V3_عدد::op_بٽ_منزل_Real_D(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.ويڪر() == 64 && ويڪر() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.منزل_UQuad();
    أرجع سيٽ_Double(u.m_d);
}
V3_عدد& V3_عدد::opNegateD(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_DOUBLE_ARGS1(lhs);
    أرجع سيٽ_Double(-lhs.منزل_Double());
}
V3_عدد& V3_عدد::op_جوڙيو_D(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Double(lhs.منزل_Double() + rhs.منزل_Double());
}
V3_عدد& V3_عدد::opSubD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Double(lhs.منزل_Double() - rhs.منزل_Double());
}
V3_عدد& V3_عدد::opMulD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Double(lhs.منزل_Double() * rhs.منزل_Double());
}
V3_عدد& V3_عدد::opDivD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Double(lhs.منزل_Double() / rhs.منزل_Double());
}
V3_عدد& V3_عدد::opPowD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Double(قوت(lhs.منزل_Double(), rhs.منزل_Double()));
}
V3_عدد& V3_عدد::opEqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_Double() == rhs.منزل_Double());
}
V3_عدد& V3_عدد::opNeqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_Double() != rhs.منزل_Double());
}
V3_عدد& V3_عدد::opGtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_Double() > rhs.منزل_Double());
}
V3_عدد& V3_عدد::opGteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_Double() >= rhs.منزل_Double());
}
V3_عدد& V3_عدد::opLtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_Double() < rhs.منزل_Double());
}
V3_عدد& V3_عدد::opLteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_Double() <= rhs.منزل_Double());
}

//======================================================================
// Ops - String

V3_عدد& V3_عدد::opConcatN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_String(lhs.منزل_String() + rhs.منزل_String());
}
V3_عدد& V3_عدد::opReplN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_STRING_ARGS1(lhs);
    انگ_ASSERT_LOGIC_ARGS1(rhs);
    أرجع opReplN(lhs, rhs.منزل_UInt());
}
V3_عدد& V3_عدد::opReplN(ثابت V3_عدد& lhs, uint32_t rhsval) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    string ٻاهريون;
    ٻاهريون.reserve(lhs.منزل_String().ڊيگهائي() * rhsval);
    لكل (بنانشان times = 0; times < rhsval; ++times) ٻاهريون += lhs.منزل_String();
    أرجع سيٽ_String(ٻاهريون);
}
V3_عدد& V3_عدد::opCvtPackString(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    أرجع سيٽ_String(lhs.منزل_String());
}
V3_عدد& V3_عدد::op_منزل_هيٺاهون_N(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string ٻاهريون = VString::downcase(lhs.منزل_String());
    أرجع سيٽ_String(ٻاهريون);
}
V3_عدد& V3_عدد::op_منزل_مٿانهون_N(ثابت V3_عدد& lhs) {
    انگ_ASSERT_OP_ARGS1(lhs);
    انگ_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string ٻاهريون = VString::upcase(lhs.منزل_String());
    أرجع سيٽ_String(ٻاهريون);
}

V3_عدد& V3_عدد::opEqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_String() == rhs.منزل_String());
}
V3_عدد& V3_عدد::opNeqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_String() != rhs.منزل_String());
}
V3_عدد& V3_عدد::opGtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_String() > rhs.منزل_String());
}
V3_عدد& V3_عدد::opGteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_String() >= rhs.منزل_String());
}
V3_عدد& V3_عدد::opLtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_String() < rhs.منزل_String());
}
V3_عدد& V3_عدد::opLteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    انگ_ASSERT_OP_ARGS2(lhs, rhs);
    انگ_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سيٽ_Single_بٽ(lhs.منزل_String() <= rhs.منزل_String());
}

//======================================================================

فراغ V3_عدد::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_سٽ* ثابت fl = جديد File_سٽ{File_سٽ::built_اندريون_Filename()};
    V3_عدد انگ{fl, 32, 0};
    انگ.selfTestThis();
}

فراغ V3_عدد::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_عدد::epsilonEqual(0, 0), سچ);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(1e19, 1e19), سچ);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(9, 0.0001), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(1, 1 + std::numeric_limits<مزدوج>::epsilon()), سچ);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(0.009, 0.00899999999999999931998839741709), سچ);

    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(0), سچ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(1), سچ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(-1), سچ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(1.0001), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(0.9999), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(-1.0001), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(-0.9999), خطأ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
