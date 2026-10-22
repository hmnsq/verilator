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

constexpr صحيح MAX_SPRINTF_DOUBLE_اندازه
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#تعریف_کن رقم_ASSERT_OP_ARGS1(arg1) \
    UASSERT((هذا != &(arg1)), "Number operation called with same source and dest")
#تعریف_کن رقم_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2)), \
            "Number operation called with same source and dest")
#تعریف_کن رقم_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3)), \
            "Number operation called with same source and dest")
#تعریف_کن رقم_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3) && هذا != &(arg4)), \
            "Number operation called with same source and dest")

#تعریف_کن رقم_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).داده_Type() == V3_عدد_داده::V3_عدد_داده_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#تعریف_کن رقم_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    رقم_ASSERT_LOGIC_ARGS1(arg1); \
    رقم_ASSERT_LOGIC_ARGS1(arg2)

#تعریف_کن رقم_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    رقم_ASSERT_LOGIC_ARGS1(arg1); \
    رقم_ASSERT_LOGIC_ARGS1(arg2); \
    رقم_ASSERT_LOGIC_ARGS1(arg3); \
    رقم_ASSERT_LOGIC_ARGS1(arg4)

#تعریف_کن رقم_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#تعریف_کن رقم_ASSERT_STRING_ARGS2(arg1, arg2) \
    رقم_ASSERT_STRING_ARGS1(arg1); \
    رقم_ASSERT_STRING_ARGS1(arg2)

#تعریف_کن رقم_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#تعریف_کن رقم_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    رقم_ASSERT_DOUBLE_ARGS1(arg1); \
    رقم_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

فراغ V3_عدد::v3error_انتها(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (m_nodep) {
        m_nodep->v3error_انتها(nsstr);
    } وإلا إذا (m_fileline) {
        m_fileline->v3error_انتها(nsstr);
    } وإلا {
        V3Error::v3error_انتها(nsstr, "", اشاره_تهی);
    }
}

فراغ V3_عدد::v3error_انتها_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_انتها(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_عدد::V3_عدد(Ast_گره* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    إذا (str.خالی()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } وإلا {
        init(nodep, std::max<صحيح>(str.درازی() * 8, 1));
        لكل (بیعلامت pos = 0; pos < str.درازی(); ++pos) {
            ثابت صحيح topos = str.درازی() - 1 - pos;
            مقدار_And_X& v = m_داده.رقم()[topos / 4];
            لكل (صحيح بیت_2 = 0; بیت_2 < 8; ++بیت_2) {
                إذا (str[pos] & (1UL << بیت_2)) v.m_مقدار |= (1UL << (بیت_2 + (topos % 4) * 8));
            }
        }
    }
    m_داده.m_fromString = درست;
    opCleanThis(درست);
}

V3_عدد::V3_عدد(Ast_گره* nodep, ثابت Ast_گره_DType* nodedtypep) {
    إذا (nodedtypep->isCHandle()) {
        init(nodep);
        مجموعه_Null();
    } وإلا إذا (nodedtypep->isDouble()) {
        init(nodep, 64);
        مجموعه_Double(0.0);
    } وإلا إذا (nodedtypep->isString()) {
        init(nodep);
        مجموعه_String("");
    } وإلا {
        init(nodep, nodedtypep->عرض(), nodedtypep->عرض_Sized());
    }
}

فراغ V3_عدد::create(ثابت حرف* sourcep) {
    m_داده.مجموعه_Logic();
    ثابت حرف* مقدار_startp = sourcep;
    لكل (ثابت حرف* cp = sourcep; *cp; ++cp) {
        إذا (*cp == '\'') {
            مقدار_startp = cp + 1;
            اكسر;
        }
    }

    منطقي userSized = خطأ;
    منطقي unbased = خطأ;
    حرف پایه = '\0';
    إذا (مقدار_startp != sourcep) {  // Has a '
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
            isSigned(درست);
        }
        إذا (*cp) {
            پایه = *cp;
            ++cp;
        }
        مقدار_startp = cp;

        userSized = widthn.درازی() != 0;
        إذا (std::atoi(widthn.c_str())) {
            إذا (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_رقم_عرض()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                عرض(v3Global.opt.max_رقم_عرض(), درست);
            } وإلا {
                عرض(std::atoi(widthn.c_str()), درست);
            }
        }
    } وإلا {
        unbased = درست;
        پایه = 'd';
    }

    لكل (صحيح i = 0; i < واژگان(); ++i) m_داده.رقم()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    إذا (پایه == '0') {
        عرض(1, خطأ);  // So we extend it
        مجموعه_بیت(0, 0);
        m_داده.m_autoExtend = درست;
    } وإلا إذا (پایه == '1') {
        عرض(1, خطأ);  // So we extend it
        مجموعه_بیت(0, 1);
        m_داده.m_autoExtend = درست;
    } وإلا إذا (std::tolower(پایه) == 'z') {
        عرض(1, خطأ);  // So we extend it
        مجموعه_بیت(0, 'z');
        m_داده.m_autoExtend = درست;
    } وإلا إذا (std::tolower(پایه) == 'x') {
        عرض(1, خطأ);  // So we extend it
        مجموعه_بیت(0, 'x');
        m_داده.m_autoExtend = درست;
    }
    // Otherwise...
    وإلا إذا (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحيح عرض_Bound = std::max<صحيح>(32, std::طول_رشته(مقدار_startp) * 4);
        عرض(عرض_Bound, خطأ);  // Will change width below
        إذا (unbased) isSigned(درست);  // Also says the spec.
    }

    // Ignore leading blanks
    طالما (*مقدار_startp == '_' || std::آیا_فاصله(*مقدار_startp)) ++مقدار_startp;
    إذا (!*مقدار_startp && !m_داده.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    إذا (userSized && m_داده.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحيح obit = 0;  // Start at LSB
    صحيح پایه_align = 1;
    إذا (std::tolower(پایه) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        طالما (*مقدار_startp == '_' || *مقدار_startp == '0') ++مقدار_startp;
        // Convert decimal number to hex
        صحيح olen = 0;
        uint32_t val = 0;
        صحيح got_x = 0;
        صحيح got_z = 0;
        صحيح got_01 = 0;
        منطقي warnedTooMany = خطأ;
        لكل (ثابت حرف* cp = مقدار_startp; *cp; ++cp) {
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
                    m_داده.رقم()[0].m_مقدار = val;
                } وإلا {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_عدد حاصلضرب(هذا, عرض() + 4);  // +4 for overflow detection
                    ثابت V3_عدد ten(هذا, عرض() + 4, 10);
                    ثابت V3_عدد addend(هذا, عرض(), (*cp - '0'));
                    حاصلضرب.opMul(*هذا, ten);
                    op_جمع_کن(حاصلضرب, addend);
                    إذا (!warnedTooMany && حاصلضرب.بیتها_مقدار(عرض(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = درست;
                    }
                }
                ++olen;
                got_01 = 1;
                اكسر;
            }
            حالة 'z':
            حالة '?': {
                got_z = 1;
                إذا (!userSized) عرض(32, خطأ);
                مجموعه_All_بیتها_Z();
                اكسر;
            }
            حالة 'x': {
                got_x = 1;
                إذا (!userSized) عرض(32, خطأ);
                مجموعه_All_بیتها_X();
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
        obit = عرض();
        إذا ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << مقدار_startp);
        }
    } وإلا {
        // Convert bin/octal number to hex
        لكل (ثابت حرف* cp = مقدار_startp + std::طول_رشته(مقدار_startp) - 1; cp >= مقدار_startp;
             cp--) {
            إذا (*cp != '_' && *cp != '0' && obit >= عرض()) {
                warnTooMany(sourcep);
                اكسر;
            }
            بدل (std::tolower(پایه)) {
            حالة 'b': {
                پایه_align = 1;
                بدل (std::tolower(*cp)) {
                حالة '0': مجموعه_بیت(obit++, 0); اكسر;
                حالة '1': مجموعه_بیت(obit++, 1); اكسر;
                حالة 'z':
                حالة '?': مجموعه_بیت(obit++, 'z'); اكسر;
                حالة 'x': مجموعه_بیت(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                افتراضي: v3error("Illegal character in binary constant: " << *cp);
                }
                اكسر;
            }

            حالة 'o':
            حالة 'c': {
                پایه_align = 3;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': مجموعه_بیت(obit++, 0); مجموعه_بیت(obit++, 0);  مجموعه_بیت(obit++, 0);  اكسر;
                حالة '1': مجموعه_بیت(obit++, 1); مجموعه_بیت(obit++, 0);  مجموعه_بیت(obit++, 0);  اكسر;
                حالة '2': مجموعه_بیت(obit++, 0); مجموعه_بیت(obit++, 1);  مجموعه_بیت(obit++, 0);  اكسر;
                حالة '3': مجموعه_بیت(obit++, 1); مجموعه_بیت(obit++, 1);  مجموعه_بیت(obit++, 0);  اكسر;
                حالة '4': مجموعه_بیت(obit++, 0); مجموعه_بیت(obit++, 0);  مجموعه_بیت(obit++, 1);  اكسر;
                حالة '5': مجموعه_بیت(obit++, 1); مجموعه_بیت(obit++, 0);  مجموعه_بیت(obit++, 1);  اكسر;
                حالة '6': مجموعه_بیت(obit++, 0); مجموعه_بیت(obit++, 1);  مجموعه_بیت(obit++, 1);  اكسر;
                حالة '7': مجموعه_بیت(obit++, 1); مجموعه_بیت(obit++, 1);  مجموعه_بیت(obit++, 1);  اكسر;
                حالة 'z':
                حالة '?': مجموعه_بیت(obit++, 'z'); مجموعه_بیت(obit++, 'z'); مجموعه_بیت(obit++, 'z'); اكسر;
                حالة 'x': مجموعه_بیت(obit++, 'x'); مجموعه_بیت(obit++, 'x'); مجموعه_بیت(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                // clang-format on
                افتراضي: v3error("Illegal character in octal constant");
                }
                اكسر;
            }

            حالة 'h':  // FALLTHRU
            حالة 'x': {
                پایه_align = 4;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); اكسر;
                حالة '1': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); اكسر;
                حالة '2': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); اكسر;
                حالة '3': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); اكسر;
                حالة '4': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); اكسر;
                حالة '5': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); اكسر;
                حالة '6': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); اكسر;
                حالة '7': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); اكسر;
                حالة '8': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); اكسر;
                حالة '9': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); اكسر;
                حالة 'a': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); اكسر;
                حالة 'b': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); اكسر;
                حالة 'c': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); اكسر;
                حالة 'd': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); اكسر;
                حالة 'e': مجموعه_بیت(obit++,0); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); اكسر;
                حالة 'f': مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); مجموعه_بیت(obit++,1); اكسر;
                حالة 'z':
                حالة '?': مجموعه_بیت(obit++,'z'); مجموعه_بیت(obit++,'z'); مجموعه_بیت(obit++,'z'); مجموعه_بیت(obit++,'z'); اكسر;
                حالة 'x': مجموعه_بیت(obit++,'x'); مجموعه_بیت(obit++,'x'); مجموعه_بیت(obit++,'x'); مجموعه_بیت(obit++,'x'); اكسر;
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
                v3error("Illegal base character: " << پایه);
                اكسر;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    إذا (!userSized && !m_داده.m_autoExtend) {
        عرض(std::max(32, پایه_align * ((عرض_مقصد_Fit() + پایه_align - 1) / پایه_align)), خطأ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    طالما (obit <= عرض() && obit && بیت_Is_XZ(obit - 1)) {
        مجموعه_بیت(obit, بیت_Is(obit - 1));
        ++obit;
    }
    opCleanThis(درست);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

فراغ V3_عدد::warnTooMany(ثابت string& مقدار) {
    v3warn(WIDTHDIGITS, "Too many digits for " << عرض() << " bit number: '" << مقدار << "'");
}

فراغ V3_عدد::nodep(Ast_گره* nodep) VL_MT_STABLE {
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

صحيح V3_عدد::log2b(uint32_t رقم) {
    // See also opCLog2
    لكل (صحيح بیت_2 = 31; بیت_2 > 0; بیت_2--) {
        إذا (رقم & (1ULL << بیت_2)) أرجع بیت_2;
    }
    أرجع 0;
}

صحيح V3_عدد::log2bQuad(uint64_t رقم) {
    إذا (رقم >> 32ULL) أرجع 32 + log2b(رقم >> 32ULL);
    أرجع log2b(رقم);
}

//======================================================================
// Setters

V3_عدد& V3_عدد::مجموعه_Zero() {
    لكل (صحيح i = 0; i < واژگان(); ++i) m_داده.رقم()[i] = {0, 0};
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_Quad(uint64_t مقدار) {
    مجموعه_Zero();
    m_داده.رقم()[0].m_مقدار = مقدار & 0xffffffffULL;
    إذا (عرض() > 32) m_داده.رقم()[1].m_مقدار = (مقدار >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_Long(uint32_t مقدار) {
    مجموعه_Zero();
    m_داده.رقم()[0].m_مقدار = مقدار;
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_Long_S(int32_t مقدار) {
    مجموعه_Zero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = مقدار;
    (فراغ)u.s;
    m_داده.رقم()[0].m_مقدار = u.u;
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_Double(مزدوج مقدار) {
    UASSERT(عرض() == 64, "Real operation on wrong sized number");
    m_داده.مجموعه_Double();
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.d = مقدار;
    (فراغ)u.d;
    لكل (صحيح i = 2; i < واژگان(); ++i) m_داده.رقم()[i] = {0, 0};
    m_داده.رقم()[0].m_مقدار = u.u[0];
    m_داده.رقم()[1].m_مقدار = u.u[1];
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_Single_بیتها(حرف مقدار) {
    لكل (صحيح i = 1 /*فوقانی*/; i < واژگان(); ++i) m_داده.رقم()[i] = {0, 0};
    m_داده.رقم()[0] = {(مقدار == '1' || مقدار == 'x' || مقدار == 1 || مقدار == 3),
                       (مقدار == 'z' || مقدار == 'x' || مقدار == 2 || مقدار == 3)};
    أرجع *هذا;
}

V3_عدد& V3_عدد::مجموعه_All_Bits0() {
    مجموعه_Zero();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_All_Bits1() {
    لكل (صحيح i = 0; i < واژگان(); ++i) m_داده.رقم()[i] = {~0U, 0};
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_All_بیتها_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    لكل (صحيح i = 0; i < واژگان(); ++i) m_داده.رقم()[i] = {~0U, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_All_بیتها_Z() {
    لكل (صحيح i = 0; i < واژگان(); ++i) m_داده.رقم()[i] = {0, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::مجموعه_All_بیتها_XRemoved() {
    إذا (!v3Global.constRemoveXs()) {
        أرجع مجموعه_All_بیتها_X();
    } وإلا {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        إذا (v3Global.opt.x_تخصیص_بده() == "1") {
            أرجع مجموعه_All_Bits1();
        } وإلا {
            أرجع مجموعه_All_Bits0();
        }
    }
}
V3_عدد& V3_عدد::مجموعه_Value1() {
    m_داده.رقم()[0] = {1, 0};
    لكل (صحيح i = 1; i < واژگان(); ++i) m_داده.رقم()[i] = {0, 0};
    أرجع *هذا;
}

فراغ V3_عدد::مجموعه_بیت_X0(صحيح بیت_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    مجموعه_بیت(بیت_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_عدد& V3_عدد::مجموعه_ماسک(صحيح nbits, صحيح lsb) {
    مجموعه_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_بیتها(lsb, std::max(0, std::min(nbits, عرض() - lsb)), 1);
    أرجع *هذا;
}

//======================================================================
// ACCESSORS - as strings

string V3_عدد::ascii(منطقي prefixed, منطقي cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (is1_گام()) أرجع "1step";
    إذا (isNull()) أرجع "null";
    std::ostringstream بیرون;
    إذا (isDouble()) {
        بیرون.precision(17);
        إذا (VL_UNCOVERABLE(عرض() != 64)) {
            بیرون << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } وإلا {
            بیرون << مقصد_Double();
            إذا (مقصد_Double() == floor(مقصد_Double())) بیرون << ".0";
        }
        أرجع بیرون.str();
    } وإلا إذا (isString()) {
        أرجع '"' + مقصد_String() + '"';
    } وإلا {
        إذا (VL_UNCOVERABLE((m_داده.رقم()[واژگان() - 1].m_مقدار | m_داده.رقم()[واژگان() - 1].m_مقدار_X)
                           & ~hi_کلمه_ماسک())) {
            بیرون << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    إذا (prefixed) {
        إذا (sized()) {
            بیرون << عرض() << "'";
        } وإلا إذا (autoExtend() && عرض() == 1) {
            بیرون << "'";
            إذا (بیت_Is0(0)) {
                بیرون << '0';
            } وإلا إذا (بیت_Is1(0)) {
                بیرون << '1';
            } وإلا إذا (بیت_Is_Z(0)) {
                بیرون << 'z';
            } وإلا {
                بیرون << 'x';
            }
            أرجع بیرون.str();
        } وإلا {
            إذا (cleanVerilog) {
                بیرون << "'";
            } وإلا {
                بیرون << "?" << عرض() << "?";
            }
        }
        إذا (isSigned()) بیرون << "s";
    }

    ثابت منطقي دودویی = (isFourState()
#اگر_تعریف_شده V3_عدد_ASCII_دودویی
                         // cppcheck-suppress konwnConditionTrueFalse
                         || درست
#پایان_اگر
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    إذا (دودویی) {
        بیرون << "b";
        بیرون << displayed("%0b");
    } وإلا {
        إذا (prefixed) بیرون << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        بیرون << displayed("%0h");
    }
    إذا (isNull()) {
        إذا (VL_UNCOVERABLE(!isEqZero())) {
            بیرون << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } وإلا {
            بیرون << " [null]";
        }
    }
    أرجع بیرون.str();
}

منطقي V3_عدد::displayedFmtHasArg(حرف format, منطقي is_پیمایش) {
    // Is this a format letter that takes an argument in runtime?
    (فراغ)is_پیمایش;
    بدل (std::tolower(format)) {
    حالة 'b': أرجع درست;  //دودویی
    حالة 'c': أرجع درست;  // Character
    حالة 'd': أرجع درست;  // Decimal
    حالة 'e': أرجع درست;  // Floating
    حالة 'f': أرجع درست;  // Floating
    حالة 'g': أرجع درست;  // Floating
    حالة 'h': أرجع درست;  // Hex
    حالة 'o': أرجع درست;  // Octal
    حالة 'p': أرجع درست;  //الگو
    حالة 's': أرجع درست;  // String
    حالة 't': أرجع درست;  //زمان
    حالة 'u': أرجع درست;  // Packed 2-state
    حالة 'v': أرجع درست;  // Strength
    حالة 'x': أرجع درست;  // Hex
    حالة 'z': أرجع درست;  // Packed 4-state
    حالة '?': أرجع درست;  // Internal: Compute format in V3Width based on data type
    افتراضي: أرجع خطأ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_عدد::displayPad(اندازه_t fmtsize, حرف pad, منطقي چپ, ثابت string& درون) VL_خالص {
    string padding;
    إذا (درون.درازی() < fmtsize) padding = string(fmtsize - درون.درازی(), pad);
    أرجع چپ ? (درون + padding) : (padding + درون);
}

string V3_عدد::displayedEnumName(ثابت Ast_Enum_قلم* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    أرجع VString::isIdentifier(name) && !std::آیا_رقم(static_cast<بیعلامت حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_عدد::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3_عدد مقدار{هذا, dtypep->عرض()};
    مقدار.op_تخصیص_بده(*هذا);
    لكل (ثابت Ast_Enum_قلم* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        إذا (!constp->رقم().isAnyXZ() && مقدار.isCaseEq(constp->رقم()))
            أرجع displayedEnumName(itemp);
    }
    أرجع "";
}

string V3_عدد::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
    V3_عدد مقدار{هذا, dtypep->عرض()};
    مقدار.op_تخصیص_بده(*هذا);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.پشت());
    إذا (fmt != 'p' && fmt != 's') أرجع مقدار.displayed(argp, vformat, numericAttr);
    string name = مقدار.displayedEnumName(dtypep);
    إذا (name.خالی()) name = مقدار.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_عدد name_رقم{هذا};
    name_رقم.مجموعه_String(name);
    أرجع name_رقم.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_عدد::displayed(ثابت Ast_گره* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    أرجع displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_عدد::displayed(File_سطر* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    خودکار pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *هذا);
    ++pos;
    منطقي چپ = خطأ;
    إذا (pos[0] == '-') {
        چپ = درست;
        ++pos;
    }
    string fmtsize;
    لكل (; pos != vformat.cend() && (std::آیا_رقم(pos[0]) || pos[0] == '.'); ++pos) {
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
            V3_عدد strwide{fl, static_cast<صحيح>(مقصد_String().درازی()) * 8, 0};
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
        إذا (عرض() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت بیعلامت صحيح v = بیتها_مقدار(0, 8);
        str.افزودن_پشت(static_cast<حرف>(v));
        أرجع str;
    }
    حالة 'e':  // FALLTHRU
    حالة 'f':  // FALLTHRU
    حالة 'g': {
        ثابت مزدوج n = formatAttr.isDouble()   ? مقصد_Double()
                         : formatAttr.isSigned() ? مقصد_SQuad()
                                                 : مقصد_UQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_اندازه];
        (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_اندازه, vformat.c_str(), n);
        أرجع tmp;
    }
    حالة 's': {
        إذا (formatAttr.isString() || formatAttr.isComplex()) {
            str = مقصد_String();
        } وإلا {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحيح بیت_2 = عرض() - 1;
            منطقي شروع = درست;
            طالما ((بیت_2 % 8) != 7) ++بیت_2;
            لكل (; بیت_2 >= 0; بیت_2 -= 8) {
                ثابت صحيح v = بیتها_مقدار(بیت_2 - 7, 8);
                إذا (!شروع || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    شروع = خطأ;  // Drop leading 0s
                } وإلا {
                    إذا (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت اندازه_t fmtsizen = static_cast<اندازه_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', چپ, str);
        أرجع str;
    }
    حالة 'p': {  //الگو
        // 'p' with NUMBER was earlier converted to 'd'
        إذا (formatAttr.isDouble()) {
            ثابت مزدوج n = formatAttr.isDouble()   ? مقصد_Double()
                             : formatAttr.isSigned() ? مقصد_SQuad()
                                                     : مقصد_UQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_اندازه];
            (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_اندازه, "%g", n);
            أرجع tmp;
        }
        إذا (formatAttr.isString())
            أرجع '"' + V3_بیرون_Formatter::quoteNameControls(مقصد_String()) + '"';
        إذا (formatAttr.isComplex()) أرجع مقصد_String();
        أرجع "%p";
    }
    حالة 'b':  // FALLTHRU
    حالة 'o':  // FALLTHRU
    حالة 'h':  // FALLTHRU
    حالة 'x': {
        صحيح بیت_2 = عرض() - 1;
        إذا (چپ || !fmtsize.خالی()) {
            طالما (بیت_2 && بیت_Is0(بیت_2)) --بیت_2;
        }
        بدل (fmt) {
        حالة 'b': {
            لكل (; بیت_2 >= 0; --بیت_2) {
                إذا (بیت_Is0(بیت_2)) {
                    str += '0';
                } وإلا إذا (بیت_Is1(بیت_2)) {
                    str += '1';
                } وإلا إذا (بیت_Is_Z(بیت_2)) {
                    str += 'z';
                } وإلا {
                    str += 'x';
                }
            }
            اكسر;
        }
        حالة 'o': {
            طالما ((بیت_2 % 3) != 2) ++بیت_2;
            لكل (; بیت_2 > 0; بیت_2 -= 3) {
                ثابت صحيح رقم_X = شمار_X(بیت_2 - 2, 3);
                ثابت صحيح رقم_Z = شمار_Z(بیت_2 - 2, 3);
                إذا (رقم_X == 3 || رقم_X == عرض() - (بیت_2 - 2)) {
                    str += 'x';
                    استمر;
                }
                إذا (رقم_Z == 3 || رقم_Z == عرض() - (بیت_2 - 2)) {
                    str += 'z';
                    استمر;
                }
                إذا (رقم_X > 0) {
                    str += 'X';
                    استمر;
                }
                إذا (رقم_Z > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = بیتها_مقدار(بیت_2 - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            اكسر;
        }
        افتراضي: {  // h/x
            طالما ((بیت_2 % 4) != 3) ++بیت_2;
            لكل (; بیت_2 > 0; بیت_2 -= 4) {
                ثابت صحيح رقم_X = شمار_X(بیت_2 - 3, 4);
                ثابت صحيح رقم_Z = شمار_Z(بیت_2 - 3, 4);
                إذا (رقم_X == 4 || رقم_X == عرض() - (بیت_2 - 3)) {
                    str += 'x';
                    استمر;
                }
                إذا (رقم_Z == 4 || رقم_Z == عرض() - (بیت_2 - 3)) {
                    str += 'z';
                    استمر;
                }
                إذا (رقم_X > 0) {
                    str += 'X';
                    استمر;
                }
                إذا (رقم_Z > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = بیتها_مقدار(بیت_2 - 3, 4);
                إذا (v >= 10) {
                    str += static_cast<حرف>('a' + v - 10);
                } وإلا {
                    str += static_cast<حرف>('0' + v);
                }
            }
            اكسر;
        }
        }  // switch
        ثابت اندازه_t fmtsizen = static_cast<اندازه_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (چپ ? ' ' : '0'), چپ, str);
        أرجع str;
    }  // case b/d/x/o
    حالة 't':  //زمان
    حالة 'd': {  // Unsigned decimal
        ثابت منطقي issigned = formatAttr.isSigned();
        إذا (fmtsize == "" && !چپ) {
            ثابت مزدوج mantissabits = عرض() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            مزدوج dchars = mantissabits / 3.321928094887362 + 1.0;
            إذا (issigned) ++dchars;  // space for sign
            fmtsize = cvt_مقصد_Str(static_cast<صحيح>(dchars));
        }
        منطقي hasXZ = خطأ;
        إذا (isAllX()) {
            str = "x";
            hasXZ = درست;
        } وإلا إذا (isAllZ()) {
            str = "z";
            hasXZ = درست;
        } وإلا إذا (isAnyX()) {
            str = "X";
            hasXZ = درست;
        } وإلا إذا (isAnyZ()) {
            str = "Z";
            hasXZ = درست;
        }
        إذا (!hasXZ) {
            إذا (issigned) {
                إذا (عرض() > 64) {
                    str = مقصد_Decimal_S();
                } وإلا {
                    str = cvt_مقصد_Str(مقصد_SQuad());
                }
            } وإلا {
                إذا (عرض() > 64) {
                    str = مقصد_Decimal_U();
                } وإلا {
                    str = cvt_مقصد_Str(مقصد_UQuad());
                }
            }
        }
        ثابت منطقي zeropad = fmtsize.درازی() > 0 && fmtsize[0] == '0' && !چپ;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت اندازه_t fmtsizen = static_cast<اندازه_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), چپ, str);
        أرجع str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالة 'u': {  // Packed 2-state
        لكل (صحيح i = 0; i < واژگان(); ++i) {
            ثابت uint32_t v = m_داده.رقم()[i].m_مقدار;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'z': {  // Packed 4-state
        لكل (صحيح i = 0; i < واژگان(); ++i) {
            ثابت مقدار_And_X v = m_داده.رقم()[i];
            str += static_cast<حرف>((v.m_مقدار >> 0) & 0xff);
            str += static_cast<حرف>((v.m_مقدار >> 8) & 0xff);
            str += static_cast<حرف>((v.m_مقدار >> 16) & 0xff);
            str += static_cast<حرف>((v.m_مقدار >> 24) & 0xff);
            str += static_cast<حرف>((v.m_مقدار_X >> 0) & 0xff);
            str += static_cast<حرف>((v.m_مقدار_X >> 8) & 0xff);
            str += static_cast<حرف>((v.m_مقدار_X >> 16) & 0xff);
            str += static_cast<حرف>((v.m_مقدار_X >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'v': {  // Strength
        صحيح بیت_2 = عرض() - 1;
        لكل (; بیت_2 >= 0; بیت_2--) {
            إذا (بیت_Is0(بیت_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            وإلا إذا (بیت_Is1(بیت_2)) {
                str += "St1 ";
            } وإلا إذا (بیت_Is_Z(بیت_2)) {
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
    constexpr اندازه_t bufsize = 1000;
    حرف sbuf[bufsize];
    string نتیجه;
    إذا (isNull()) {
        أرجع "VlNull{}";
    } وإلا إذا (isDouble()) {
        ثابت مزدوج dnum = مقصد_Double();
        إذا (std::isinf(dnum)) {
            إذا (std::signbit(dnum)) نتیجه += '-';
            نتیجه += "std::numeric_limits<double>::infinity()";
        } وإلا إذا (std::isnan(dnum)) {
            إذا (std::signbit(dnum)) نتیجه += '-';
            نتیجه += "std::numeric_limits<double>::quiet_NaN()";
        } وإلا {
            ثابت حرف* ثابت fmt = (static_cast<صحيح>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            أرجع sbuf;
        }
    } وإلا إذا (isString()) {
        ثابت string strg = مقصد_String();
        ثابت string quoted
            = V3_بیرون_Formatter::quoteNameControls(strg, V3_بیرون_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        أرجع '"' + quoted + "\"s";
    } وإلا إذا (واژگان() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        نتیجه = "::VlWide<" + std::مقصد_string(واژگان()) + ">{{";
        إذا (واژگان() > 4) نتیجه += '\n';
        لكل (صحيح n = 0; n < واژگان(); ++n) {
            إذا (n) نتیجه += ((n % 4) ? ", " : ",\n");
            (فراغ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_کلمه(n));
            نتیجه += sbuf;
        }
        إذا (واژگان() > 4) نتیجه += '\n';
        نتیجه += "}}";
    } وإلا إذا (واژگان() == 2) {  // Quad
        ثابت uint64_t qnum = static_cast<uint64_t>(مقصد_UQuad());
        ثابت حرف* ثابت fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        أرجع sbuf;
    } وإلا {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ثابت uint32_t unum = مقصد_UInt();
        ثابت حرف* ثابت fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (عرض() > 16) ? ("0x%08" PRIx32 "U")
                                : (عرض() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        أرجع sbuf;
    }
    أرجع نتیجه;
}

string V3_عدد::مقصد_Decimal_S() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (isNegative()) {
        V3_عدد lhsNoSign = *هذا;
        lhsNoSign.opNegate(*هذا);
        أرجع "-"s + lhsNoSign.مقصد_Decimal_U();
    } وإلا {
        أرجع مقصد_Decimal_U();
    }
}

string V3_عدد::مقصد_Decimal_U() ثابت VL_MT_STABLE {
    ثابت صحيح maxdecwidth = (عرض() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_عدد bcd(هذا, maxdecwidth + 4);
    V3_عدد tmp(هذا, maxdecwidth + 4);
    V3_عدد tmp2(هذا, maxdecwidth + 4);

    صحيح from_بیت = عرض() - 1;
    // Skip all leading zeros
    لكل (; from_بیت >= 0 && بیت_Is0(from_بیت); --from_بیت) {}
    // Double-dabble algorithm
    لكل (; from_بیت >= 0; --from_بیت) {
        // Any digits >= 5 need an add 3 (via tmp)
        لكل (صحيح nibble_بیت = 0; nibble_بیت < maxdecwidth; nibble_بیت += 4) {
            إذا (bcd.بیتها_مقدار(nibble_بیت, 4) >= 5) {
                tmp2.مجموعه_All_Bits0();
                tmp2.مجموعه_بیت(nibble_بیت, 1);  // Add 3, decomposed as two bits
                tmp2.مجموعه_بیت(nibble_بیت + 1, 1);
                tmp.op_تخصیص_بده(bcd);
                bcd.op_جمع_کن(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_تخصیص_بده(bcd);
        bcd.op_لغزش_L(tmp, V3_عدد(هذا, 32, 1));
        // bcd[0] = this[from_bit]
        إذا (بیت_Is1(from_بیت)) bcd.مجموعه_بیت(0, 1);
    }

    string output;
    صحيح lsb = (maxdecwidth - 1) & ~3;
    لكل (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        إذا (bcd.بیتها_مقدار(lsb, 4)) اكسر;
    }
    لكل (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.بیتها_مقدار(lsb, 4));  // 0..9
    }
    أرجع output;
}

string V3_عدد::مقصد_Real_String() ثابت VL_MT_STABLE {
    ثابت مزدوج مقدار = مقصد_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream بیرون;
    بیرون.precision(15);
    بیرون << مقدار;
    string نتیجه = بیرون.str();
    إذا (std::strtod(نتیجه.c_str(), اشاره_تهی) != مقدار) {
        بیرون.str("");
        بیرون.precision(17);
        بیرون << مقدار;
        نتیجه = بیرون.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    إذا (نتیجه.پیدا_کن_اولی_از(".en") == string::npos) نتیجه += ".0";
    أرجع نتیجه;
}

//======================================================================
// ACCESSORS - as numbers

منطقي V3_عدد::میگنجد_درون_UInt() ثابت VL_MT_امن {
    إذا (isFourState()) أرجع خطأ;
    // We allow wide numbers that represent values <= 32 bits
    لكل (صحيح i = 1; i < واژگان(); ++i) {
        إذا (m_داده.رقم()[i].m_مقدار) أرجع خطأ;
    }
    أرجع درست;
}
uint32_t V3_عدد::مقصد_UInt() ثابت VL_MT_امن {
    UASSERT(!isFourState(), "toUInt with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 32 bits
    إذا (VL_UNCOVERABLE(!میگنجد_درون_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *هذا);
    }
    أرجع m_داده.رقم()[0].m_مقدار;
}

مزدوج V3_عدد::مقصد_Double() ثابت VL_MT_امن {
    UASSERT(isDouble() && عرض() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.u[0] = m_داده.رقم()[0].m_مقدار;
    u.u[1] = m_داده.رقم()[1].m_مقدار;
    أرجع u.d;
}

int32_t V3_عدد::مقصد_SInt() ثابت VL_MT_امن {
    إذا (isSigned()) {
        ثابت uint32_t v = مقصد_UInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (عرض() - 1))));
        ثابت uint32_t extended = v | signExtend;
        أرجع static_cast<int32_t>(extended);
    } وإلا {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        أرجع static_cast<int32_t>(مقصد_UInt());
    }
}

uint64_t V3_عدد::مقصد_UQuad() ثابت VL_MT_امن {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 64 bits
    إذا (isDouble()) أرجع static_cast<uint64_t>(مقصد_Double());
    لكل (صحيح i = 2; i < واژگان(); ++i) {
        إذا (m_داده.رقم()[i].m_مقدار) {
            v3error("Value too wide for 64-bits expected in this context " << *هذا);
            اكسر;
        }
    }
    إذا (عرض() <= 32) أرجع (static_cast<uint64_t>(مقصد_UInt()));
    أرجع ((static_cast<uint64_t>(m_داده.رقم()[1].m_مقدار) << 32ULL)
            | (static_cast<uint64_t>(m_داده.رقم()[0].m_مقدار)));
}

int64_t V3_عدد::مقصد_SQuad() ثابت {
    إذا (isDouble()) أرجع static_cast<int64_t>(مقصد_Double());
    ثابت uint64_t v = مقصد_UQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (عرض() - 1))));
    ثابت uint64_t extended = v | signExtend;
    أرجع static_cast<int64_t>(extended);
}

string V3_عدد::مقصد_String() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *هذا);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    إذا (isString()) أرجع m_داده.str();
    صحيح بیت_2 = عرض() - 1;
    طالما ((بیت_2 % 8) != 7) ++بیت_2;
    std::string str;
    لكل (; بیت_2 >= 0; بیت_2 -= 8) {
        ثابت صحيح v = بیتها_مقدار(بیت_2 - 7, 8);
        إذا (v) str += static_cast<حرف>(v);
    }
    أرجع str;
}

uint32_t V3_عدد::edata_کلمه(صحيح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *هذا);
    أرجع m_داده.رقم()[eword].m_مقدار;
}

uint8_t V3_عدد::داده_Byte(صحيح byte) ثابت {
    أرجع (edata_کلمه(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقي V3_عدد::isAllZ() ثابت VL_MT_امن {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        ثابت uint32_t ماسک = i == واژگان() - 1 ? hi_کلمه_ماسک() : ~0U;
        إذا (((~v.m_مقدار & v.m_مقدار_X) & ماسک) != ماسک) أرجع خطأ;
    }
    أرجع درست;
}
منطقي V3_عدد::isAllX() ثابت VL_MT_امن {
    إذا (isDouble() || isString()) أرجع خطأ;
    uint32_t ماسک = hi_کلمه_ماسک();
    لكل (صحيح i = واژگان() - 1; i >= 0; --i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        إذا ((v.m_مقدار & v.m_مقدار_X) ^ ماسک) أرجع خطأ;
        ماسک = ~0U;
    }
    أرجع درست;
}
منطقي V3_عدد::isEqZero() ثابت VL_MT_امن {
    إذا (isString()) أرجع m_داده.str().خالی();
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        إذا (v.m_مقدار || v.m_مقدار_X) أرجع خطأ;
    }
    أرجع درست;
}
منطقي V3_عدد::isNeqZero() ثابت {
    إذا (isString()) أرجع !m_داده.str().خالی();
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        إذا (v.m_مقدار & ~v.m_مقدار_X) أرجع درست;
    }
    أرجع خطأ;
}
منطقي V3_عدد::is_بیتها_Zero(صحيح msb, صحيح lsb) ثابت {
    لكل (صحيح i = lsb; i <= msb; ++i) {
        إذا (VL_UNLIKELY(!بیت_Is0(i))) أرجع خطأ;
    }
    أرجع درست;
}
منطقي V3_عدد::isEqOne() ثابت {
    إذا (m_داده.رقم()[0].m_مقدار != 1 || m_داده.رقم()[0].m_مقدار_X) أرجع خطأ;
    لكل (صحيح i = 1; i < واژگان(); ++i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        إذا (v.m_مقدار || v.m_مقدار_X) أرجع خطأ;
    }
    أرجع درست;
}
منطقي V3_عدد::isEqAllOnes(صحيح optwidth) ثابت {
    // Correct number of zero bits/width matters
    إذا (!optwidth) optwidth = عرض();
    لكل (صحيح بیت_2 = 0; بیت_2 < optwidth; ++بیت_2) {
        إذا (!بیت_Is1(بیت_2)) أرجع خطأ;
    }
    أرجع درست;
}
منطقي V3_عدد::isFourState() ثابت VL_MT_امن {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        إذا (m_داده.رقم()[i].m_مقدار_X) أرجع درست;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isAnyX() ثابت VL_MT_امن {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        إذا (v.m_مقدار & v.m_مقدار_X) أرجع درست;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isAnyXZ() ثابت {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        إذا (m_داده.رقم()[i].m_مقدار_X) أرجع درست;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isAnyZ() ثابت VL_MT_امن {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        ثابت مقدار_And_X v = m_داده.رقم()[i];
        إذا (~v.m_مقدار & v.m_مقدار_X) أرجع درست;
    }
    أرجع خطأ;
}
منطقي V3_عدد::isLtXZ(ثابت V3_عدد& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    إذا (isString()) أرجع مقصد_String() < rhs.مقصد_String();
    إذا (isDouble()) أرجع مقصد_Double() < rhs.مقصد_Double();
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(عرض(), rhs.عرض()); ++بیت_2) {
        إذا (بیت_Is1(بیت_2) && rhs.بیت_Is0(بیت_2)) أرجع درست;
        إذا (rhs.بیت_Is1(بیت_2) && بیت_Is0(بیت_2)) أرجع خطأ;
        إذا (بیت_Is_XZ(بیت_2)) أرجع درست;
        إذا (rhs.بیت_Is_XZ(بیت_2)) أرجع خطأ;
    }
    أرجع خطأ;
}
صحيح V3_عدد::شمار_X(صحيح lsb, صحيح nbits) ثابت VL_MT_امن {
    صحيح شمار = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= عرض()) أرجع شمار;
        إذا (بیت_Is_X(lsb + bitn)) ++شمار;
    }
    أرجع شمار;
}
صحيح V3_عدد::شمار_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_امن {
    صحيح شمار = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= عرض()) أرجع شمار;
        إذا (بیت_Is_Z(lsb + bitn)) ++شمار;
    }
    أرجع شمار;
}

صحيح V3_عدد::عرض_مقصد_Fit() ثابت {
    لكل (صحيح بیت_2 = عرض() - 1; بیت_2 > 0; بیت_2--) {
        إذا (!بیت_Is0(بیت_2)) أرجع بیت_2 + 1;
    }
    أرجع 1;  // one bit even if number is == 0
}

uint32_t V3_عدد::شمار_بیتها(ثابت V3_عدد& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        بدل (ctrl.بیت_Is(0)) {
        حالة '0':
            إذا (بیت_Is0(بیت_2)) ++n;
            اكسر;
        حالة '1':
            إذا (بیت_Is1(بیت_2)) ++n;
            اكسر;
        حالة 'x':
            إذا (بیت_Is_X(بیت_2)) ++n;
            اكسر;
        حالة 'z':
            إذا (بیت_Is_Z(بیت_2)) ++n;
            اكسر;
        }
    }
    أرجع n;
}

uint32_t V3_عدد::شمار_بیتها(ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                             ثابت V3_عدد& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = شمار_بیتها(ctrl1);
    إذا (ctrl2.بیت_Is(0) != ctrl1.بیت_Is(0)) n += شمار_بیتها(ctrl2);
    إذا ((ctrl3.بیت_Is(0) != ctrl1.بیت_Is(0)) && (ctrl3.بیت_Is(0) != ctrl2.بیت_Is(0))) {
        n += شمار_بیتها(ctrl3);
    }
    أرجع n;
}

uint32_t V3_عدد::شمار_Ones() ثابت {
    صحيح n = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (بیت_Is1(بیت_2)) ++n;
    }
    أرجع n;
}

uint32_t V3_عدد::most_مجموعه_بیت_P1() ثابت {
    لكل (صحيح بیت_2 = عرض() - 1; بیت_2 >= 0; بیت_2--) {
        إذا (!بیت_Is0(بیت_2)) أرجع بیت_2 + 1;
    }
    أرجع 0;
}

uint32_t V3_عدد::کمترین_مجموعه_بیت_P1() ثابت {
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (!بیت_Is0(بیت_2)) أرجع بیت_2 + 1;
    }
    أرجع 0;
}

//======================================================================

V3_عدد& V3_عدد::op_بیتها_Non_XZ(ثابت V3_عدد& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is0(بیت_2) || lhs.بیت_Is1(بیت_2)) مجموعه_بیت(بیت_2, 1);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_بیتها_One(ثابت V3_عدد& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2)) مجموعه_بیت(بیت_2, 1);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_بیتها_XZ(ثابت V3_عدد& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is_XZ(بیت_2)) مجموعه_بیت(بیت_2, 1);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_بیتها_Z(ثابت V3_عدد& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is_Z(بیت_2)) مجموعه_بیت(بیت_2, 1);
    }
    أرجع *هذا;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_عدد& V3_عدد::opRedOr(ثابت V3_عدد& lhs) {
    // op i, 1 bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < lhs.عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            أرجع مجموعه_Single_بیتها(1);
        } وإلا إذا (lhs.بیت_Is0(بیت_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opRedAnd(ثابت V3_عدد& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    لكل (صحيح بیت_2 = 0; بیت_2 < lhs.عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is0(بیت_2)) {
            أرجع مجموعه_Single_بیتها(0);
        } وإلا إذا (lhs.بیت_Is1(بیت_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opRedXor(ثابت V3_عدد& lhs) {
    // op i, 1 bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < lhs.عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            إذا (outc == 1) {
                outc = 0;
            } وإلا إذا (outc == 0) {
                outc = 1;
            }
        } وإلا إذا (lhs.بیت_Is0(بیت_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::op_شمار_بیتها(ثابت V3_عدد& عبارت, ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                                ثابت V3_عدد& ctrl3) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS4(عبارت, ctrl1, ctrl2, ctrl3);
    رقم_ASSERT_LOGIC_ARGS4(عبارت, ctrl1, ctrl2, ctrl3);
    مجموعه_Zero();
    m_داده.رقم()[0].m_مقدار = عبارت.شمار_بیتها(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_شمار_Ones(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    مجموعه_Zero();
    m_داده.رقم()[0].m_مقدار = lhs.شمار_Ones();
    opCleanThis();
    أرجع *هذا;
}
V3_عدد& V3_عدد::opIsUnknown(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    أرجع مجموعه_Single_بیتها(lhs.isAnyXZ());
}
V3_عدد& V3_عدد::opOneHot(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    أرجع مجموعه_Single_بیتها(lhs.شمار_Ones() == 1);
}
V3_عدد& V3_عدد::opOneHot0(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    أرجع مجموعه_Single_بیتها(lhs.شمار_Ones() <= 1);
}
V3_عدد& V3_عدد::opCLog2(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحيح adjust = (lhs.شمار_Ones() == 1) ? 0 : 1;
    لكل (صحيح بیت_2 = lhs.عرض() - 1; بیت_2 >= 0; بیت_2--) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            مجموعه_Long(بیت_2 + adjust);
            أرجع *هذا;
        }
    }
    مجموعه_Zero();
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_Most_مجموعه_بیت_P1(ثابت V3_عدد& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    لكل (صحيح بیت_2 = lhs.عرض() - 1; بیت_2 >= 0; بیت_2--) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            مجموعه_Long(بیت_2 + 1);
            أرجع *هذا;
        }
    }
    مجموعه_Zero();
    أرجع *هذا;
}

V3_عدد& V3_عدد::opLogNot(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    لكل (صحيح بیت_2 = 0; بیت_2 < lhs.عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            outc = 0;
            اذهب آخرین;
        } وإلا إذا (lhs.بیت_Is0(بیت_2)) {
        } وإلا {
            outc = 'x';
        }
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opNot(ثابت V3_عدد& lhs) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is0(بیت_2)) {
            مجموعه_بیت(بیت_2, 1);
        } وإلا إذا (lhs.بیت_Is_XZ(بیت_2)) {
            مجموعه_بیت(بیت_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2) && rhs.بیت_Is1(بیت_2)) {
            مجموعه_بیت(بیت_2, 1);
        } وإلا إذا (lhs.بیت_Is0(بیت_2) || rhs.بیت_Is0(بیت_2)) {  // 0
        } وإلا {
            مجموعه_بیت(بیت_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2) || rhs.بیت_Is1(بیت_2)) {
            مجموعه_بیت(بیت_2, 1);
        } وإلا إذا (lhs.بیت_Is0(بیت_2) && rhs.بیت_Is0(بیت_2)) {
            // 0
        } وإلا {
            مجموعه_بیت(بیت_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opXor(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2) && rhs.بیت_Is0(بیت_2)) {
            مجموعه_بیت(بیت_2, 1);
        } وإلا إذا (lhs.بیت_Is0(بیت_2) && rhs.بیت_Is1(بیت_2)) {
            مجموعه_بیت(بیت_2, 1);
        } وإلا إذا (lhs.بیت_Is_XZ(بیت_2) || rhs.بیت_Is_XZ(بیت_2)) {
            مجموعه_بیت(بیت_2, 'x');
        }
        // else zero
    }
    أرجع *هذا;
}

فراغ V3_عدد::fill_بیتها(صحيح destLsb, صحيح عرض, حرف مقدار) {
    UASSERT(destLsb >= 0 && عرض >= 0, "Negative bit range");
    إذا (!عرض) أرجع;
    UASSERT(عرض <= هذا->عرض() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t مقدار_کلمه = مقدار == '1' || مقدار == 'x' || مقدار == 1 || مقدار == 3 ? ~0U : 0;
    ثابت uint32_t x_کلمه = مقدار == 'z' || مقدار == 'x' || مقدار == 2 || مقدار == 3 ? ~0U : 0;
    ثابت صحيح اولی_کلمه = VL_BITWORD_E(destLsb);
    ثابت صحيح آخرین_کلمه = VL_BITWORD_E(destLsb + عرض - 1);
    لكل (صحيح کلمه = اولی_کلمه; کلمه <= آخرین_کلمه; ++کلمه) {
        ثابت صحيح کلمه_Lsb = کلمه == اولی_کلمه ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحيح کلمه_Msb = کلمه == آخرین_کلمه ? VL_BITBIT_E(destLsb + عرض - 1) : VL_SIZEBITS_E;
        ثابت uint32_t ماسک = VL_ماسک_E(کلمه_Msb - کلمه_Lsb + 1) << کلمه_Lsb;
        مقدار_And_X& dest = m_داده.رقم()[کلمه];
        dest.m_مقدار = (dest.m_مقدار & ~ماسک) | (مقدار_کلمه & ماسک);
        dest.m_مقدار_X = (dest.m_مقدار_X & ~ماسک) | (x_کلمه & ماسک);
    }
}

فراغ V3_عدد::copy_بیتها(صحيح destLsb, ثابت V3_عدد& منبع, صحيح منبع_Lsb, صحيح عرض) {
    UASSERT(destLsb >= 0 && منبع_Lsb >= 0 && عرض >= 0, "Negative bit range");
    إذا (!عرض) أرجع;
    UASSERT(عرض <= هذا->عرض() - destLsb, "Destination bit range exceeds number width");
    UASSERT(عرض <= منبع.عرض() - منبع_Lsb, "Source bit range exceeds number width");

    ثابت صحيح اولی_کلمه = VL_BITWORD_E(destLsb);
    ثابت صحيح آخرین_کلمه = VL_BITWORD_E(destLsb + عرض - 1);
    لكل (صحيح dest_کلمه = اولی_کلمه; dest_کلمه <= آخرین_کلمه; ++dest_کلمه) {
        ثابت صحيح dest_بیت = std::max(destLsb, dest_کلمه * VL_EDATASIZE);
        ثابت صحيح dest_جابه_جایی = VL_BITBIT_E(dest_بیت);
        ثابت صحيح منبع_بیت = منبع_Lsb + dest_بیت - destLsb;
        ثابت صحيح منبع_کلمه = VL_BITWORD_E(منبع_بیت);
        ثابت صحيح منبع_جابه_جایی = VL_BITBIT_E(منبع_بیت);
        ثابت صحيح copy_عرض = std::min(عرض - (dest_بیت - destLsb), VL_EDATASIZE - dest_جابه_جایی);
        ثابت uint32_t ماسک = VL_ماسک_E(copy_عرض) << dest_جابه_جایی;

        ثابت مقدار_And_X& منبع_Lo = منبع.m_داده.رقم()[منبع_کلمه];
        uint32_t مقدار = منبع_Lo.m_مقدار >> منبع_جابه_جایی;
        uint32_t مقدار_X = منبع_Lo.m_مقدار_X >> منبع_جابه_جایی;
        إذا (منبع_جابه_جایی && copy_عرض > VL_EDATASIZE - منبع_جابه_جایی) {
            ثابت مقدار_And_X& منبع_Hi = منبع.m_داده.رقم()[منبع_کلمه + 1];
            مقدار |= منبع_Hi.m_مقدار << (VL_EDATASIZE - منبع_جابه_جایی);
            مقدار_X |= منبع_Hi.m_مقدار_X << (VL_EDATASIZE - منبع_جابه_جایی);
        }

        مقدار_And_X& dest = m_داده.رقم()[dest_کلمه];
        dest.m_مقدار = (dest.m_مقدار & ~ماسک) | ((مقدار << dest_جابه_جایی) & ماسک);
        dest.m_مقدار_X = (dest.m_مقدار_X & ~ماسک) | ((مقدار_X << dest_جابه_جایی) & ماسک);
    }
}

V3_عدد& V3_عدد::opConcat(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    مجموعه_Zero();
    // See also error in V3Width
    إذا (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_بیتها(0, rhs, 0, rhs.عرض());
    copy_بیتها(rhs.عرض(), lhs, 0, lhs.عرض());
    أرجع *هذا;
}

V3_عدد& V3_عدد::opLenN(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    مجموعه_Quad(lhs.مقصد_String().درازی());
    أرجع *هذا;
}

V3_عدد& V3_عدد::opRepl(ثابت V3_عدد& lhs,
                           ثابت V3_عدد& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    أرجع opRepl(lhs, rhs.مقصد_UInt());
}

V3_عدد& V3_عدد::opRepl(ثابت V3_عدد& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (v3Global.opt.replication_حد()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_حد())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_حد() << " is suspect: " << rhsval);
    }
    مجموعه_Zero();
    لكل (بیعلامت times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.عرض();
        إذا (destLsb >= static_cast<uint32_t>(عرض())) اكسر;
        copy_بیتها(static_cast<صحيح>(destLsb), lhs, 0,
                 std::min(lhs.عرض(), عرض() - static_cast<صحيح>(destLsb)));
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_جریان_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    مجموعه_Zero();
    // See also error in V3Width
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحيح ssize = std::min(rhs.مقصد_UInt(), static_cast<بیعلامت>(lhs.عرض()));
    لكل (صحيح istart = 0; istart < lhs.عرض(); istart += ssize) {
        ثابت صحيح ostart = std::max(0, lhs.عرض() - ssize - istart);
        copy_بیتها(ostart, lhs, istart, std::min(ssize, lhs.عرض() - istart));
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opLogAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < lhs.عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            loutc = 1;
            اكسر;
        }
        إذا (lhs.بیت_Is_XZ(بیت_2) && loutc == 0) loutc = 'x';
    }
    لكل (صحيح بیت_2 = 0; بیت_2 < rhs.عرض(); ++بیت_2) {
        إذا (rhs.بیت_Is1(بیت_2)) {
            routc = 1;
            اكسر;
        }
        إذا (rhs.بیت_Is_XZ(بیت_2) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    إذا (routc == 1 && loutc == 1) outc = 1;
    إذا (routc == 0 || loutc == 0) outc = 0;
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opLogOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < lhs.عرض(); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2)) {
            outc = 1;
            اذهب آخرین;
        }
        إذا (lhs.بیت_Is_XZ(بیت_2) && outc == 0) outc = 'x';
    }
    لكل (صحيح بیت_2 = 0; بیت_2 < rhs.عرض(); ++بیت_2) {
        إذا (rhs.بیت_Is1(بیت_2)) {
            outc = 1;
            اذهب آخرین;
        }
        إذا (rhs.بیت_Is_XZ(بیت_2) && outc == 0) outc = 'x';
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opLogIf(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد lnot = lhs;
    lnot.opLogNot(lhs);
    أرجع opLogOr(lnot, rhs);
}

V3_عدد& V3_عدد::opLogEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_عدد ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    أرجع opLogAnd(ifa, ifb);
}

V3_عدد& V3_عدد::opAtoN(ثابت V3_عدد& lhs, صحيح پایه) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    UASSERT(پایه == AstAtoN::ATOREAL || پایه == 2 || پایه == 8 || پایه == 10 || پایه == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.مقصد_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.انتها(), '_'), str.انتها());

    إذا (پایه == AstAtoN::ATOREAL) أرجع مجموعه_Double(std::atof(str.c_str()));

    errno = 0;
    خودکار v = std::strtol(str.c_str(), اشاره_تهی, پایه);
    إذا (errno != 0) v = 0;
    أرجع مجموعه_Long_S(static_cast<int32_t>(v));
}

V3_عدد& V3_عدد::opPutcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths) {
    رقم_ASSERT_OP_ARGS3(lhs, rhs, ths);
    رقم_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.مقصد_String();
    ثابت int32_t i = rhs.مقصد_SInt();
    ثابت int32_t c = ths.مقصد_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.درازی() && c != 0) lstring[i] = c;
    أرجع مجموعه_String(lstring);
}

V3_عدد& V3_عدد::opGetcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.مقصد_String();
    ثابت int32_t i = rhs.مقصد_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.درازی()) v = lstring[i];
    أرجع مجموعه_Long(v);
}

V3_عدد& V3_عدد::opSubstrN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths) {
    رقم_ASSERT_OP_ARGS3(lhs, rhs, ths);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.مقصد_String();
    ثابت int32_t i = rhs.مقصد_SInt();
    ثابت int32_t j = ths.مقصد_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    إذا (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.درازی()) أرجع مجموعه_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    أرجع مجموعه_String(lstring.substr(i, j - i + 1));
}

V3_عدد& V3_عدد::opCompareNN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, منطقي ignoreCase) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحيح نتیجه;
    ثابت string lstring = lhs.مقصد_String();
    ثابت string rstring = rhs.مقصد_String();
    إذا (ignoreCase) {
        نتیجه = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } وإلا {
        نتیجه = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    أرجع مجموعه_Long_S(نتیجه);
}

V3_عدد& V3_عدد::opEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opEqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opEqD(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض(), rhs.عرض()); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2) && rhs.بیت_Is0(بیت_2)) {
            outc = 0;
            اذهب آخرین;
        }
        إذا (lhs.بیت_Is0(بیت_2) && rhs.بیت_Is1(بیت_2)) {
            outc = 0;
            اذهب آخرین;
        }
        إذا (lhs.بیت_Is_XZ(بیت_2)) outc = 'x';
        إذا (rhs.بیت_Is_XZ(بیت_2)) outc = 'x';
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opNeqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opNeqD(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض(), rhs.عرض()); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2) && rhs.بیت_Is0(بیت_2)) {
            outc = 1;
            اذهب آخرین;
        }
        إذا (lhs.بیت_Is0(بیت_2) && rhs.بیت_Is1(بیت_2)) {
            outc = 1;
            اذهب آخرین;
        }
        إذا (lhs.بیت_Is_XZ(بیت_2)) outc = 'x';
        إذا (rhs.بیت_Is_XZ(بیت_2)) outc = 'x';
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3Hash V3_عدد::مقصد_Hash() ثابت {
    // Shouldmatch isCaseEq
    إذا (isString()) أرجع V3Hash{مقصد_String()};
    إذا (isDouble()) أرجع V3Hash{مقصد_Double()};
    V3Hash hash{عرض()};
    لكل (صحيح i = 0; i < واژگان(); ++i) hash += m_داده.رقم()[i].m_مقدار;
    أرجع hash;
}

منطقي V3_عدد::isCaseEq(ثابت V3_عدد& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    إذا (isString()) أرجع مقصد_String() == rhs.مقصد_String();
    إذا (isDouble()) أرجع مقصد_Double() == rhs.مقصد_Double();
    إذا (عرض() != rhs.عرض()) أرجع خطأ;
    لكل (صحيح i = 0; i < واژگان(); ++i) {
        إذا (!(m_داده.رقم()[i] == rhs.m_داده.رقم()[i])) أرجع خطأ;
    }
    أرجع درست;
}

V3_عدد& V3_عدد::opCaseEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_عدد& V3_عدد::opCaseNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    إذا (lhs.isString()) {
        أرجع opNeqN(lhs, rhs);
    } وإلا إذا (lhs.isDouble()) {
        أرجع opNeqD(lhs, rhs);
    }
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض(), rhs.عرض()); ++بیت_2) {
        إذا (lhs.بیت_Is(بیت_2) != rhs.بیت_Is(بیت_2)) {
            outc = 1;
            اذهب آخرین;
        }
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opWildEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض(), rhs.عرض()); ++بیت_2) {
        إذا (!rhs.بیت_Is_XZ(بیت_2)) {
            إذا (lhs.بیت_Is_XZ(بیت_2)) {
                outc = 'x';
                اذهب آخرین;
            }
            إذا (lhs.بیت_Is(بیت_2) != rhs.بیت_Is(بیت_2)) {
                outc = 0;
                اذهب آخرین;
            }
        }
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opWildNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض(), rhs.عرض()); ++بیت_2) {
        إذا (!rhs.بیت_Is_XZ(بیت_2)) {
            إذا (lhs.بیت_Is_XZ(بیت_2)) {
                outc = 'x';
                اذهب آخرین;
            }
            إذا (lhs.بیت_Is(بیت_2) != rhs.بیت_Is(بیت_2)) {
                outc = 1;
                اذهب آخرین;
            }
        }
    }
آخرین:
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opGt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض(), rhs.عرض()); ++بیت_2) {
        إذا (lhs.بیت_Is1(بیت_2) && rhs.بیت_Is0(بیت_2)) outc = 1;
        إذا (rhs.بیت_Is1(بیت_2) && lhs.بیت_Is0(بیت_2)) outc = 0;
        إذا (lhs.بیت_Is_XZ(بیت_2)) outc = 'x';
        إذا (rhs.بیت_Is_XZ(بیت_2)) outc = 'x';
    }
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opGtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحيح mbit = std::max(lhs.عرض() - 1, rhs.عرض() - 1);
        إذا (lhs.بیت_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (rhs.بیت_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (lhs.بیت_Is0(mbit) && rhs.بیت_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } وإلا إذا (lhs.بیت_Is1_Extend(mbit) && rhs.بیت_Is0(mbit)) {
            outc = 0;  // - !> +
        } وإلا {
            // both positive or negative, normal >
            لكل (صحيح بیت_2 = 0; بیت_2 < std::max(lhs.عرض() - 1, rhs.عرض() - 1); ++بیت_2) {
                إذا (lhs.بیت_Is1_Extend(بیت_2) && rhs.بیت_Is0(بیت_2)) outc = 1;
                إذا (rhs.بیت_Is1_Extend(بیت_2) && lhs.بیت_Is0(بیت_2)) outc = 0;
                إذا (lhs.بیت_Is_XZ(بیت_2)) outc = 'x';
                إذا (rhs.بیت_Is_XZ(بیت_2)) outc = 'x';
            }
        }
    }
    أرجع مجموعه_Single_بیتها(outc);
}

V3_عدد& V3_عدد::opGte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGt(lhs, rhs);
}

V3_عدد& V3_عدد::opGteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_عدد& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGtS(lhs, rhs);
}

V3_عدد& V3_عدد::opLt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGt(rhs, lhs); }
V3_عدد& V3_عدد::opLte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGte(rhs, lhs); }
V3_عدد& V3_عدد::opLtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGtS(rhs, lhs); }
V3_عدد& V3_عدد::opLteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) { أرجع opGteS(rhs, lhs); }

V3_عدد& V3_عدد::op_لغزش_R(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // L(lhs) bit return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    مجموعه_Zero();
    إذا (rhs.عرض() > 32 && !rhs.is_بیتها_Zero(rhs.عرض() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.مقصد_UInt();
    إذا (rhsval < static_cast<uint32_t>(lhs.عرض())) {
        copy_بیتها(0, lhs, static_cast<صحيح>(rhsval),
                 std::min(عرض(), lhs.عرض() - static_cast<صحيح>(rhsval)));
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_لغزش_RS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    مجموعه_Zero();
    ثابت منطقي سرریز = rhs.عرض() > 32 && !rhs.is_بیتها_Zero(rhs.عرض() - 1, 32);
    إذا (!سرریز) {
        ثابت uint32_t rhsval = rhs.مقصد_UInt();
        إذا (rhsval < lbits) {
            ثابت صحيح copy_عرض = std::min(عرض(), static_cast<صحيح>(lbits - rhsval));
            copy_بیتها(0, lhs, static_cast<صحيح>(rhsval), copy_عرض);
            fill_بیتها(copy_عرض, عرض() - copy_عرض, lhs.بیت_Is(lbits - 1));
            أرجع *هذا;
        }
    }
    fill_بیتها(0, عرض(), lhs.بیت_Is(lbits - 1));  // '0/'1/'x/'z
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_لغزش_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // L(lhs) bit return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    مجموعه_Zero();
    إذا (rhs.عرض() > 32 && !rhs.is_بیتها_Zero(rhs.عرض() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.مقصد_UInt();
    إذا (rhsval < static_cast<uint32_t>(عرض())) {
        copy_بیتها(static_cast<صحيح>(rhsval), lhs, 0,
                 std::min(عرض() - static_cast<صحيح>(rhsval), lhs.عرض()));
    }
    أرجع *هذا;
}

//======================================================================
// Ops - Arithmetic

V3_عدد& V3_عدد::opNegate(ثابت V3_عدد& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    V3_عدد notlhs(&lhs, عرض());
    notlhs.opNot(lhs);
    ثابت V3_عدد one(&lhs, عرض(), 1);
    op_جمع_کن(notlhs, one);
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_جمع_کن(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    مجموعه_Zero();
    // Addem
    uint64_t انتقال = 0;
    لكل (صحيح کلمه = 0; کلمه < واژگان(); ++کلمه) {
        ثابت uint64_t lwordval = lhs.m_داده.رقم()[کلمه].m_مقدار;
        ثابت uint64_t rwordval = rhs.m_داده.رقم()[کلمه].m_مقدار;
        ثابت uint64_t جمع = lwordval + rwordval + انتقال;
        m_داده.رقم()[کلمه].m_مقدار = جمع & 0xffffffffULL;
        انتقال = جمع > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    أرجع *هذا;
}
V3_عدد& V3_عدد::opSub(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    V3_عدد negrhs(&rhs, عرض());
    negrhs.opNegate(rhs);
    أرجع op_جمع_کن(lhs, negrhs);
}
V3_عدد& V3_عدد::opMul(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    مجموعه_Zero();
    إذا (عرض() <= 64) {
        مجموعه_Quad(lhs.مقصد_UQuad() * rhs.مقصد_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } وإلا {
        لكل (صحيح lword = 0; lword < lhs.واژگان(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_داده.رقم()[lword].m_مقدار);
            إذا (lwordval == 0) استمر;
            لكل (صحيح rword = 0; rword < rhs.واژگان(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_داده.رقم()[rword].m_مقدار);
                إذا (rwordval == 0) استمر;
                uint64_t mul = lwordval * rwordval;
                لكل (صحيح qword = lword + rword; qword < واژگان(); ++qword) {
                    mul += static_cast<uint64_t>(m_داده.رقم()[qword].m_مقدار);
                    m_داده.رقم()[qword].m_مقدار = (mul & 0xffffffffULL);
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
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    V3_عدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_عدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_عدد qNoSign = opMul(lhsNoSign, rhsNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_تخصیص_بده(qNoSign);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    إذا (rhs.isEqZero()) أرجع مجموعه_All_بیتها_XRemoved();
    إذا (lhs.عرض() <= 64) {
        مجموعه_Quad(lhs.مقصد_UQuad() / rhs.مقصد_UQuad());
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
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    إذا (rhs.isEqZero()) أرجع مجموعه_All_بیتها_XRemoved();
    V3_عدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_عدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_عدد qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_تخصیص_بده(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *هذا);
    أرجع *هذا;
}
V3_عدد& V3_عدد::opModDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    إذا (rhs.isEqZero()) أرجع مجموعه_All_بیتها_XRemoved();
    إذا (lhs.عرض() <= 64) {
        مجموعه_Quad(lhs.مقصد_UQuad() % rhs.مقصد_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide modulus
        أرجع opModDivGuts(lhs, rhs, درست);
    }
}
V3_عدد& V3_عدد::opModDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    إذا (rhs.isEqZero()) أرجع مجموعه_All_بیتها_XRemoved();
    V3_عدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_عدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_عدد qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    إذا (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } وإلا {
        op_تخصیص_بده(qNoSign);
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opModDivGuts(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, منطقي is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    مجموعه_Zero();
    // Find MSB and check for zero.
    ثابت صحيح l_واژگان = lhs.واژگان();
    ثابت صحيح umsbp1 = lhs.most_مجموعه_بیت_P1();  // dividend
    ثابت صحيح vmsbp1 = rhs.most_مجموعه_بیت_P1();  //مقسوم علیه
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
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_داده.رقم()[j].m_مقدار));
            m_داده.رقم()[j].m_مقدار = unw64 / static_cast<uint64_t>(rhs.m_داده.رقم()[0].m_مقدار);
            k = unw64
                - (static_cast<uint64_t>(m_داده.رقم()[j].m_مقدار)
                   * static_cast<uint64_t>(rhs.m_داده.رقم()[0].m_مقدار));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *هذا << " rem=0x" << std::hex
                                   << k << std::dec);
        إذا (is_modulus) {
            مجموعه_Zero();
            m_داده.رقم()[0].m_مقدار = k;
        }
        opCleanThis();
        أرجع *هذا;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_واژگان + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_واژگان + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    لكل (صحيح i = 0; i < l_واژگان; ++i) m_داده.رقم()[i].m_مقدار = 0;
    لكل (صحيح i = 0; i < l_واژگان + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحيح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t لغزش_ماسک = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    لكل (صحيح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_داده.رقم()[i].m_مقدار << s)
                | (لغزش_ماسک & (rhs.m_داده.رقم()[i - 1].m_مقدار >> (32 - s)));
    }
    vn[0] = rhs.m_داده.رقم()[0].m_مقدار << s;

    // Copy and shift dividend by same amount; may set new upper word
    إذا (s) {
        un[uw] = lhs.m_داده.رقم()[uw - 1].m_مقدار >> (32 - s);
    } وإلا {
        un[uw] = 0;
    }
    لكل (صحيح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_داده.رقم()[i].m_مقدار << s)
                | (لغزش_ماسک & (lhs.m_داده.رقم()[i - 1].m_مقدار >> (32 - s)));
    }
    un[0] = lhs.m_داده.رقم()[0].m_مقدار << s;

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
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //تفریق
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_داده.رقم()[j].m_مقدار = qhat;  // Save quotient digit

        إذا (t < 0) {
            // Over subtracted; correct by adding back
            m_داده.رقم()[j].m_مقدار--;
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
            m_داده.رقم()[i].m_مقدار = (un[i] >> s) | (لغزش_ماسک & (un[i + 1] << (32 - s)));
        }
        لكل (صحيح i = vw; i < l_واژگان; ++i) m_داده.رقم()[i].m_مقدار = 0;
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
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع مجموعه_All_بیتها_X();
    إذا (rhs.isEqZero()) أرجع مجموعه_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    إذا (rsign && rhs.isNegative()) {
        إذا (lhs.isEqZero()) {
            أرجع مجموعه_All_بیتها_XRemoved();
        } وإلا إذا (lhs.isEqOne()) {
            أرجع مجموعه_Value1();
        } وإلا إذا (lsign && lhs.isEqAllOnes()) {
            إذا (rhs.بیت_Is1(0)) {
                أرجع مجموعه_All_Bits1();  // -1^odd=-1
            } وإلا {
                أرجع مجموعه_Value1();  // -1^even=1
            }
        }
        أرجع مجموعه_Zero();
    }
    إذا (lhs.isEqZero()) أرجع مجموعه_Zero();
    مجموعه_Zero();
    m_داده.رقم()[0].m_مقدار = 1;
    V3_عدد power(&lhs, عرض());
    power.op_تخصیص_بده(lhs);
    لكل (صحيح بیت_2 = 0; بیت_2 < rhs.عرض(); ++بیت_2) {
        إذا (بیت_2 > 0) {  // power = power*power
            V3_عدد آخرین_Power(&lhs, عرض());
            آخرین_Power.op_تخصیص_بده(power);
            power.opMul(آخرین_Power, آخرین_Power);
        }
        إذا (rhs.بیت_Is1(بیت_2)) {  // out *= power
            V3_عدد آخرین_بیرون(&lhs, عرض());
            آخرین_بیرون.op_تخصیص_بده(*هذا);
            opMul(آخرین_بیرون, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::opPowSU(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, درست, خطأ);
}
V3_عدد& V3_عدد::opPowSS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, درست, درست);
}
V3_عدد& V3_عدد::opPowUS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, خطأ, درست);
}

V3_عدد& V3_عدد::opBufIf1(ثابت V3_عدد& ens, ثابت V3_عدد& if1s) {
    رقم_ASSERT_OP_ARGS2(ens, if1s);
    رقم_ASSERT_LOGIC_ARGS2(ens, if1s);
    مجموعه_Zero();
    لكل (صحيح بیت_2 = 0; بیت_2 < عرض(); ++بیت_2) {
        إذا (ens.بیت_Is1(بیت_2)) {
            مجموعه_بیت(بیت_2, if1s.بیت_Is(بیت_2));
        } وإلا {
            مجموعه_بیت(بیت_2, 'z');
        }
    }
    أرجع *هذا;
}

// Sets all bits in range to the given value
V3_عدد& V3_عدد::op_مجموعه_Range(uint32_t lsb, uint32_t عرض, حرف بیت_مقدار) {
    إذا (lsb < static_cast<uint32_t>(هذا->عرض())) {
        fill_بیتها(static_cast<صحيح>(lsb),
                 static_cast<صحيح>(std::min(عرض, static_cast<uint32_t>(هذا->عرض()) - lsb)),
                 بیت_مقدار);
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::op_تخصیص_بده(ثابت V3_عدد& lhs) { أرجع op_تخصیص_بده_Non_XZ(lhs, خطأ); }
V3_عدد& V3_عدد::op_تخصیص_بده_Non_XZ(ثابت V3_عدد& lhs, منطقي ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    إذا (هذا != &lhs) {
        إذا (VL_UNLIKELY(lhs.isNull())) {
            m_داده.m_isNull = درست;
        } وإلا إذا (isString()) {
            إذا (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_داده.str() = lhs.مقصد_String();
            } وإلا {
                m_داده.str() = lhs.m_داده.str();
            }
        } وإلا إذا (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            مجموعه_Zero();
        } وإلا إذا (lhs.isDouble()) {
            مجموعه_Double(lhs.مقصد_Double());
        } وإلا إذا (!ignoreXZ) {
            مجموعه_Zero();
            copy_بیتها(0, lhs, 0, std::min(عرض(), lhs.عرض()));
        } وإلا {
            لكل (صحيح کلمه = 0; کلمه < واژگان(); ++کلمه) {
                ثابت uint32_t مقدار = کلمه < lhs.واژگان() ? lhs.m_داده.رقم()[کلمه].m_مقدار
                                                                & ~lhs.m_داده.رقم()[کلمه].m_مقدار_X
                                                          : 0;
                m_داده.رقم()[کلمه] = {مقدار, 0};
            }
            opCleanThis();
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opNToI(ثابت V3_عدد& lhs) {
    // String to packed number
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    مجموعه_Zero();
    ثابت string& str = lhs.مقصد_String();
    لكل (اندازه_t n = 0; n < str.درازی(); ++n) {
        ثابت حرف c = str[str.درازی() - 1 - n];
        لكل (اندازه_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<بیعلامت دراز دراز>(c)};
            مجموعه_بیت(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    أرجع *هذا;
}

V3_عدد& V3_عدد::opExtendS(ثابت V3_عدد& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    ثابت صحيح copy_عرض = std::min(عرض(), static_cast<صحيح>(lbits));
    copy_بیتها(0, lhs, 0, copy_عرض);
    fill_بیتها(copy_عرض, عرض() - copy_عرض, lhs.بیت_Is(lbits - 1));
    أرجع *هذا;
}

V3_عدد& V3_عدد::opExtendXZ(ثابت V3_عدد& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    ثابت صحيح copy_عرض = std::min(عرض(), static_cast<صحيح>(lbits));
    copy_بیتها(0, lhs, 0, copy_عرض);
    fill_بیتها(copy_عرض, عرض() - copy_عرض, lhs.بیت_Is(lbits - 1));
    أرجع *هذا;
}

V3_عدد& V3_عدد::opClean(ثابت V3_عدد& lhs, uint32_t بیتها) { أرجع opSel(lhs, بیتها - 1, 0); }

فراغ V3_عدد::opCleanThis(منطقي warnOnTruncation) {
    // Clean MSB of number
    رقم_ASSERT_LOGIC_ARGS1(*هذا);
    ثابت مقدار_And_X v = m_داده.رقم()[واژگان() - 1];
    ثابت uint32_t new_مقدار_Msb = v.m_مقدار & hi_کلمه_ماسک();
    ثابت uint32_t new_مقدار_XMsb = v.m_مقدار_X & hi_کلمه_ماسک();
    إذا (warnOnTruncation && (new_مقدار_Msb != v.m_مقدار || new_مقدار_XMsb != v.m_مقدار_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << عرض() << " bit number: " << displayed("%d"));
    }
    m_داده.رقم()[واژگان() - 1] = {new_مقدار_Msb, new_مقدار_XMsb};
}

V3_عدد& V3_عدد::opSel(ثابت V3_عدد& lhs, ثابت V3_عدد& msb, ثابت V3_عدد& lsb) {
    رقم_ASSERT_OP_ARGS3(lhs, msb, lsb);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS2(msb, lsb);
    إذا (lsb.isFourState() || msb.isFourState()) أرجع مجموعه_All_بیتها_X();
    أرجع opSel(lhs, msb.مقصد_UInt(), lsb.مقصد_UInt());
}

V3_عدد& V3_عدد::opSel(ثابت V3_عدد& lhs, uint32_t msbval, uint32_t lsbval) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    مجموعه_Zero();
    صحيح copy_عرض = 0;
    إذا (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.عرض())) {
        ثابت uint64_t selected_عرض = uint64_t{msbval} - lsbval + 1;
        copy_عرض = static_cast<صحيح>(
            std::min({static_cast<uint64_t>(عرض()), static_cast<uint64_t>(lhs.عرض()) - lsbval,
                      selected_عرض}));
    }
    copy_بیتها(0, lhs, copy_عرض ? static_cast<صحيح>(lsbval) : 0, copy_عرض);
    fill_بیتها(copy_عرض, عرض() - copy_عرض, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    أرجع *هذا;
}

V3_عدد& V3_عدد::opSelInto(ثابت V3_عدد& lhs, ثابت V3_عدد& lsb, صحيح عرض) {
    أرجع opSelInto(lhs, lsb.مقصد_SInt(), عرض);
}

V3_عدد& V3_عدد::opSelInto(ثابت V3_عدد& lhs, صحيح lsbval, صحيح عرض) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحيح copy_عرض = std::max(0, std::min({عرض, lhs.عرض(), هذا->عرض() - lsbval}));
    copy_بیتها(lsbval, lhs, 0, copy_عرض);
    ثابت صحيح fill_عرض
        = std::max(0, std::min(عرض - copy_عرض, هذا->عرض() - lsbval - copy_عرض));
    fill_بیتها(lsbval + copy_عرض, fill_عرض, v3Global.constRemoveXs() ? 0 : 'x');
    أرجع *هذا;
}

//======================================================================
// Ops - Floating point

V3_عدد& V3_عدد::opIToRD(ثابت V3_عدد& lhs, منطقي isSigned) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_عدد noxz(lhs);
    noxz.op_تخصیص_بده_Non_XZ(lhs);
    مزدوج d = 0;
    ثابت منطقي negate = isSigned && noxz.isNegative();
    إذا (negate) {
        ثابت V3_عدد noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    لكل (صحيح بیت_2 = noxz.عرض() - 1; بیت_2 >= 0; بیت_2--) {
        // Some precision might be lost in this add, that's what we want
        إذا (noxz.بیت_Is1(بیت_2)) d += exp2(بیت_2);
    }
    إذا (negate) d = -d;
    أرجع مجموعه_Double(d);
}
V3_عدد& V3_عدد::opRToIS(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_TRUNC(lhs.مقصد_Double());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    أرجع مجموعه_Long_S(i);
}
V3_عدد& V3_عدد::opRToIRoundS(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_دور(lhs.مقصد_Double());
    مجموعه_Zero();
    اتحاد {
        مزدوج d;
        uint64_t q;
    } u;
    u.d = v;
    (فراغ)u.d;

    ثابت صحيح نمایی_تابع = static_cast<صحيح>((u.q >> 52ULL) & VL_ماسک_Q(11)) - 1023;
    ثابت صحيح lsb = نمایی_تابع - 52;
    ثابت uint64_t mantissa = (u.q & VL_ماسک_Q(52)) | (1ULL << 52);
    إذا (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        لكل (صحيح بیت_2 = 0; بیت_2 <= 52; ++بیت_2) {
            إذا (mantissa & (1ULL << بیت_2)) {
                ثابت صحيح outbit = بیت_2 + lsb;
                إذا (outbit >= 0) مجموعه_بیت(outbit, 1);
            }
        }
        إذا (v < 0) {
            ثابت V3_عدد noSign = *هذا;
            opNegate(noSign);
        }
    }
    أرجع *هذا;
}
V3_عدد& V3_عدد::op_Real_مقصد_بیتها(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.عرض() == 64 && عرض() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.مقصد_Double();
    أرجع مجموعه_Quad(u.m_v);
}
V3_عدد& V3_عدد::op_بیتها_مقصد_Real_D(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.عرض() == 64 && عرض() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.مقصد_UQuad();
    أرجع مجموعه_Double(u.m_d);
}
V3_عدد& V3_عدد::opNegateD(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    أرجع مجموعه_Double(-lhs.مقصد_Double());
}
V3_عدد& V3_عدد::op_جمع_کن_D(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Double(lhs.مقصد_Double() + rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opSubD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Double(lhs.مقصد_Double() - rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opMulD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Double(lhs.مقصد_Double() * rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opDivD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Double(lhs.مقصد_Double() / rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opPowD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Double(توان(lhs.مقصد_Double(), rhs.مقصد_Double()));
}
V3_عدد& V3_عدد::opEqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_Double() == rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opNeqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_Double() != rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opGtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_Double() > rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opGteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_Double() >= rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opLtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_Double() < rhs.مقصد_Double());
}
V3_عدد& V3_عدد::opLteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_Double() <= rhs.مقصد_Double());
}

//======================================================================
// Ops - String

V3_عدد& V3_عدد::opConcatN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_String(lhs.مقصد_String() + rhs.مقصد_String());
}
V3_عدد& V3_عدد::opReplN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_STRING_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(rhs);
    أرجع opReplN(lhs, rhs.مقصد_UInt());
}
V3_عدد& V3_عدد::opReplN(ثابت V3_عدد& lhs, uint32_t rhsval) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    string بیرون;
    بیرون.reserve(lhs.مقصد_String().درازی() * rhsval);
    لكل (بیعلامت times = 0; times < rhsval; ++times) بیرون += lhs.مقصد_String();
    أرجع مجموعه_String(بیرون);
}
V3_عدد& V3_عدد::opCvtPackString(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    أرجع مجموعه_String(lhs.مقصد_String());
}
V3_عدد& V3_عدد::op_مقصد_پایینی_N(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string بیرون = VString::downcase(lhs.مقصد_String());
    أرجع مجموعه_String(بیرون);
}
V3_عدد& V3_عدد::op_مقصد_فوقانی_N(ثابت V3_عدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string بیرون = VString::upcase(lhs.مقصد_String());
    أرجع مجموعه_String(بیرون);
}

V3_عدد& V3_عدد::opEqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_String() == rhs.مقصد_String());
}
V3_عدد& V3_عدد::opNeqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_String() != rhs.مقصد_String());
}
V3_عدد& V3_عدد::opGtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_String() > rhs.مقصد_String());
}
V3_عدد& V3_عدد::opGteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_String() >= rhs.مقصد_String());
}
V3_عدد& V3_عدد::opLtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_String() < rhs.مقصد_String());
}
V3_عدد& V3_عدد::opLteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع مجموعه_Single_بیتها(lhs.مقصد_String() <= rhs.مقصد_String());
}

//======================================================================

فراغ V3_عدد::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_سطر* ثابت fl = جديد File_سطر{File_سطر::built_درون_Filename()};
    V3_عدد رقم{fl, 32, 0};
    رقم.selfTestThis();
}

فراغ V3_عدد::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_عدد::epsilonEqual(0, 0), درست);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(1e19, 1e19), درست);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(9, 0.0001), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(1, 1 + std::numeric_limits<مزدوج>::epsilon()), درست);
    UASSERT_SELFTEST(V3_عدد::epsilonEqual(0.009, 0.00899999999999999931998839741709), درست);

    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(0), درست);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(1), درست);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(-1), درست);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(1.0001), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(0.9999), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(-1.0001), خطأ);
    UASSERT_SELFTEST(V3_عدد::epsilonIntegral(-0.9999), خطأ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
