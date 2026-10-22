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
#تعریف_کړه عدد_ASSERT_OP_ARGS1(arg1) \
    UASSERT((هذا != &(arg1)), "Number operation called with same source and dest")
#تعریف_کړه عدد_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2)), \
            "Number operation called with same source and dest")
#تعریف_کړه عدد_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3)), \
            "Number operation called with same source and dest")
#تعریف_کړه عدد_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3) && هذا != &(arg4)), \
            "Number operation called with same source and dest")

#تعریف_کړه عدد_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).معلومات_Type() == V3_شمېره_معلومات::V3_شمېره_معلومات_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#تعریف_کړه عدد_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    عدد_ASSERT_LOGIC_ARGS1(arg1); \
    عدد_ASSERT_LOGIC_ARGS1(arg2)

#تعریف_کړه عدد_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    عدد_ASSERT_LOGIC_ARGS1(arg1); \
    عدد_ASSERT_LOGIC_ARGS1(arg2); \
    عدد_ASSERT_LOGIC_ARGS1(arg3); \
    عدد_ASSERT_LOGIC_ARGS1(arg4)

#تعریف_کړه عدد_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#تعریف_کړه عدد_ASSERT_STRING_ARGS2(arg1, arg2) \
    عدد_ASSERT_STRING_ARGS1(arg1); \
    عدد_ASSERT_STRING_ARGS1(arg2)

#تعریف_کړه عدد_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#تعریف_کړه عدد_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    عدد_ASSERT_DOUBLE_ARGS1(arg1); \
    عدد_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

فراغ V3_شمېره::v3error_پای_ټکی(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (m_nodep) {
        m_nodep->v3error_پای_ټکی(nsstr);
    } وإلا إذا (m_fileline) {
        m_fileline->v3error_پای_ټکی(nsstr);
    } وإلا {
        V3Error::v3error_پای_ټکی(nsstr, "", تش_اشاره);
    }
}

فراغ V3_شمېره::v3error_پای_ټکی_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_پای_ټکی(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_شمېره::V3_شمېره(Ast_غوټه* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    إذا (str.تشه()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } وإلا {
        init(nodep, std::max<صحيح>(str.اوږدتیا() * 8, 1));
        لكل (بینښه pos = 0; pos < str.اوږدتیا(); ++pos) {
            ثابت صحيح topos = str.اوږدتیا() - 1 - pos;
            ارزښت_And_X& v = m_معلومات.عدد()[topos / 4];
            لكل (صحيح بټ_2 = 0; بټ_2 < 8; ++بټ_2) {
                إذا (str[pos] & (1UL << بټ_2)) v.m_ارزښت |= (1UL << (بټ_2 + (topos % 4) * 8));
            }
        }
    }
    m_معلومات.m_fromString = ریښتیا;
    opCleanThis(ریښتیا);
}

V3_شمېره::V3_شمېره(Ast_غوټه* nodep, ثابت Ast_غوټه_DType* nodedtypep) {
    إذا (nodedtypep->isCHandle()) {
        init(nodep);
        سېټ_Null();
    } وإلا إذا (nodedtypep->isDouble()) {
        init(nodep, 64);
        سېټ_Double(0.0);
    } وإلا إذا (nodedtypep->isString()) {
        init(nodep);
        سېټ_String("");
    } وإلا {
        init(nodep, nodedtypep->پلنوالی(), nodedtypep->پلنوالی_Sized());
    }
}

فراغ V3_شمېره::create(ثابت حرف* sourcep) {
    m_معلومات.سېټ_Logic();
    ثابت حرف* ارزښت_startp = sourcep;
    لكل (ثابت حرف* cp = sourcep; *cp; ++cp) {
        إذا (*cp == '\'') {
            ارزښت_startp = cp + 1;
            اكسر;
        }
    }

    منطقي userSized = خطأ;
    منطقي unbased = خطأ;
    حرف بنسټ = '\0';
    إذا (ارزښت_startp != sourcep) {  // Has a '
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
            isSigned(ریښتیا);
        }
        إذا (*cp) {
            بنسټ = *cp;
            ++cp;
        }
        ارزښت_startp = cp;

        userSized = widthn.اوږدتیا() != 0;
        إذا (std::atoi(widthn.c_str())) {
            إذا (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_عدد_پلنوالی()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                پلنوالی(v3Global.opt.max_عدد_پلنوالی(), ریښتیا);
            } وإلا {
                پلنوالی(std::atoi(widthn.c_str()), ریښتیا);
            }
        }
    } وإلا {
        unbased = ریښتیا;
        بنسټ = 'd';
    }

    لكل (صحيح i = 0; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    إذا (بنسټ == '0') {
        پلنوالی(1, خطأ);  // So we extend it
        سېټ_بټ(0, 0);
        m_معلومات.m_autoExtend = ریښتیا;
    } وإلا إذا (بنسټ == '1') {
        پلنوالی(1, خطأ);  // So we extend it
        سېټ_بټ(0, 1);
        m_معلومات.m_autoExtend = ریښتیا;
    } وإلا إذا (std::tolower(بنسټ) == 'z') {
        پلنوالی(1, خطأ);  // So we extend it
        سېټ_بټ(0, 'z');
        m_معلومات.m_autoExtend = ریښتیا;
    } وإلا إذا (std::tolower(بنسټ) == 'x') {
        پلنوالی(1, خطأ);  // So we extend it
        سېټ_بټ(0, 'x');
        m_معلومات.m_autoExtend = ریښتیا;
    }
    // Otherwise...
    وإلا إذا (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحيح پلنوالی_Bound = std::max<صحيح>(32, std::مزي_اوږدوالی(ارزښت_startp) * 4);
        پلنوالی(پلنوالی_Bound, خطأ);  // Will change width below
        إذا (unbased) isSigned(ریښتیا);  // Also says the spec.
    }

    // Ignore leading blanks
    طالما (*ارزښت_startp == '_' || std::که_تشه(*ارزښت_startp)) ++ارزښت_startp;
    إذا (!*ارزښت_startp && !m_معلومات.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    إذا (userSized && m_معلومات.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحيح obit = 0;  // Start at LSB
    صحيح بنسټ_align = 1;
    إذا (std::tolower(بنسټ) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        طالما (*ارزښت_startp == '_' || *ارزښت_startp == '0') ++ارزښت_startp;
        // Convert decimal number to hex
        صحيح olen = 0;
        uint32_t val = 0;
        صحيح got_x = 0;
        صحيح got_z = 0;
        صحيح got_01 = 0;
        منطقي warnedTooMany = خطأ;
        لكل (ثابت حرف* cp = ارزښت_startp; *cp; ++cp) {
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
                    m_معلومات.عدد()[0].m_ارزښت = val;
                } وإلا {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_شمېره حاصل_ضرب(هذا, پلنوالی() + 4);  // +4 for overflow detection
                    ثابت V3_شمېره ten(هذا, پلنوالی() + 4, 10);
                    ثابت V3_شمېره addend(هذا, پلنوالی(), (*cp - '0'));
                    حاصل_ضرب.opMul(*هذا, ten);
                    op_جمع_کړه(حاصل_ضرب, addend);
                    إذا (!warnedTooMany && حاصل_ضرب.بټونه_ارزښت(پلنوالی(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = ریښتیا;
                    }
                }
                ++olen;
                got_01 = 1;
                اكسر;
            }
            حالة 'z':
            حالة '?': {
                got_z = 1;
                إذا (!userSized) پلنوالی(32, خطأ);
                سېټ_All_بټونه_Z();
                اكسر;
            }
            حالة 'x': {
                got_x = 1;
                إذا (!userSized) پلنوالی(32, خطأ);
                سېټ_All_بټونه_X();
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
        obit = پلنوالی();
        إذا ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << ارزښت_startp);
        }
    } وإلا {
        // Convert bin/octal number to hex
        لكل (ثابت حرف* cp = ارزښت_startp + std::مزي_اوږدوالی(ارزښت_startp) - 1; cp >= ارزښت_startp;
             cp--) {
            إذا (*cp != '_' && *cp != '0' && obit >= پلنوالی()) {
                warnTooMany(sourcep);
                اكسر;
            }
            بدل (std::tolower(بنسټ)) {
            حالة 'b': {
                بنسټ_align = 1;
                بدل (std::tolower(*cp)) {
                حالة '0': سېټ_بټ(obit++, 0); اكسر;
                حالة '1': سېټ_بټ(obit++, 1); اكسر;
                حالة 'z':
                حالة '?': سېټ_بټ(obit++, 'z'); اكسر;
                حالة 'x': سېټ_بټ(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                افتراضي: v3error("Illegal character in binary constant: " << *cp);
                }
                اكسر;
            }

            حالة 'o':
            حالة 'c': {
                بنسټ_align = 3;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': سېټ_بټ(obit++, 0); سېټ_بټ(obit++, 0);  سېټ_بټ(obit++, 0);  اكسر;
                حالة '1': سېټ_بټ(obit++, 1); سېټ_بټ(obit++, 0);  سېټ_بټ(obit++, 0);  اكسر;
                حالة '2': سېټ_بټ(obit++, 0); سېټ_بټ(obit++, 1);  سېټ_بټ(obit++, 0);  اكسر;
                حالة '3': سېټ_بټ(obit++, 1); سېټ_بټ(obit++, 1);  سېټ_بټ(obit++, 0);  اكسر;
                حالة '4': سېټ_بټ(obit++, 0); سېټ_بټ(obit++, 0);  سېټ_بټ(obit++, 1);  اكسر;
                حالة '5': سېټ_بټ(obit++, 1); سېټ_بټ(obit++, 0);  سېټ_بټ(obit++, 1);  اكسر;
                حالة '6': سېټ_بټ(obit++, 0); سېټ_بټ(obit++, 1);  سېټ_بټ(obit++, 1);  اكسر;
                حالة '7': سېټ_بټ(obit++, 1); سېټ_بټ(obit++, 1);  سېټ_بټ(obit++, 1);  اكسر;
                حالة 'z':
                حالة '?': سېټ_بټ(obit++, 'z'); سېټ_بټ(obit++, 'z'); سېټ_بټ(obit++, 'z'); اكسر;
                حالة 'x': سېټ_بټ(obit++, 'x'); سېټ_بټ(obit++, 'x'); سېټ_بټ(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                // clang-format on
                افتراضي: v3error("Illegal character in octal constant");
                }
                اكسر;
            }

            حالة 'h':  // FALLTHRU
            حالة 'x': {
                بنسټ_align = 4;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); اكسر;
                حالة '1': سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); اكسر;
                حالة '2': سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); اكسر;
                حالة '3': سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); اكسر;
                حالة '4': سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); اكسر;
                حالة '5': سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); اكسر;
                حالة '6': سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); اكسر;
                حالة '7': سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); اكسر;
                حالة '8': سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); اكسر;
                حالة '9': سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); اكسر;
                حالة 'a': سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); اكسر;
                حالة 'b': سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); اكسر;
                حالة 'c': سېټ_بټ(obit++,0); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); اكسر;
                حالة 'd': سېټ_بټ(obit++,1); سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); اكسر;
                حالة 'e': سېټ_بټ(obit++,0); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); اكسر;
                حالة 'f': سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); سېټ_بټ(obit++,1); اكسر;
                حالة 'z':
                حالة '?': سېټ_بټ(obit++,'z'); سېټ_بټ(obit++,'z'); سېټ_بټ(obit++,'z'); سېټ_بټ(obit++,'z'); اكسر;
                حالة 'x': سېټ_بټ(obit++,'x'); سېټ_بټ(obit++,'x'); سېټ_بټ(obit++,'x'); سېټ_بټ(obit++,'x'); اكسر;
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
                v3error("Illegal base character: " << بنسټ);
                اكسر;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    إذا (!userSized && !m_معلومات.m_autoExtend) {
        پلنوالی(std::max(32, بنسټ_align * ((پلنوالی_منزل_Fit() + بنسټ_align - 1) / بنسټ_align)), خطأ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    طالما (obit <= پلنوالی() && obit && بټ_Is_XZ(obit - 1)) {
        سېټ_بټ(obit, بټ_Is(obit - 1));
        ++obit;
    }
    opCleanThis(ریښتیا);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

فراغ V3_شمېره::warnTooMany(ثابت string& ارزښت) {
    v3warn(WIDTHDIGITS, "Too many digits for " << پلنوالی() << " bit number: '" << ارزښت << "'");
}

فراغ V3_شمېره::nodep(Ast_غوټه* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    إذا (!nodep) أرجع;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

منطقي V3_شمېره::epsilonEqual(مزدوج a, مزدوج b) {
    أرجع std::fabs(a - b)
           <= (std::numeric_limits<مزدوج>::epsilon() * std::max(1.0, std::max(a, b)));
}

منطقي V3_شمېره::epsilonIntegral(مزدوج a) {
    ثابت مزدوج dint = static_cast<مزدوج>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    أرجع epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

صحيح V3_شمېره::log2b(uint32_t عدد) {
    // See also opCLog2
    لكل (صحيح بټ_2 = 31; بټ_2 > 0; بټ_2--) {
        إذا (عدد & (1ULL << بټ_2)) أرجع بټ_2;
    }
    أرجع 0;
}

صحيح V3_شمېره::log2bQuad(uint64_t عدد) {
    إذا (عدد >> 32ULL) أرجع 32 + log2b(عدد >> 32ULL);
    أرجع log2b(عدد);
}

//======================================================================
// Setters

V3_شمېره& V3_شمېره::سېټ_Zero() {
    لكل (صحيح i = 0; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, 0};
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_Quad(uint64_t ارزښت) {
    سېټ_Zero();
    m_معلومات.عدد()[0].m_ارزښت = ارزښت & 0xffffffffULL;
    إذا (پلنوالی() > 32) m_معلومات.عدد()[1].m_ارزښت = (ارزښت >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_Long(uint32_t ارزښت) {
    سېټ_Zero();
    m_معلومات.عدد()[0].m_ارزښت = ارزښت;
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_Long_S(int32_t ارزښت) {
    سېټ_Zero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = ارزښت;
    (فراغ)u.s;
    m_معلومات.عدد()[0].m_ارزښت = u.u;
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_Double(مزدوج ارزښت) {
    UASSERT(پلنوالی() == 64, "Real operation on wrong sized number");
    m_معلومات.سېټ_Double();
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.d = ارزښت;
    (فراغ)u.d;
    لكل (صحيح i = 2; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, 0};
    m_معلومات.عدد()[0].m_ارزښت = u.u[0];
    m_معلومات.عدد()[1].m_ارزښت = u.u[1];
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_Single_بټونه(حرف ارزښت) {
    لكل (صحيح i = 1 /*پاسنی*/; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, 0};
    m_معلومات.عدد()[0] = {(ارزښت == '1' || ارزښت == 'x' || ارزښت == 1 || ارزښت == 3),
                       (ارزښت == 'z' || ارزښت == 'x' || ارزښت == 2 || ارزښت == 3)};
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::سېټ_All_Bits0() {
    سېټ_Zero();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_All_Bits1() {
    لكل (صحيح i = 0; i < ټکي(); ++i) m_معلومات.عدد()[i] = {~0U, 0};
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_All_بټونه_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    لكل (صحيح i = 0; i < ټکي(); ++i) m_معلومات.عدد()[i] = {~0U, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_All_بټونه_Z() {
    لكل (صحيح i = 0; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, ~0U};
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::سېټ_All_بټونه_XRemoved() {
    إذا (!v3Global.constRemoveXs()) {
        أرجع سېټ_All_بټونه_X();
    } وإلا {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        إذا (v3Global.opt.x_ټاکل() == "1") {
            أرجع سېټ_All_Bits1();
        } وإلا {
            أرجع سېټ_All_Bits0();
        }
    }
}
V3_شمېره& V3_شمېره::سېټ_Value1() {
    m_معلومات.عدد()[0] = {1, 0};
    لكل (صحيح i = 1; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, 0};
    أرجع *هذا;
}

فراغ V3_شمېره::سېټ_بټ_X0(صحيح بټ_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    سېټ_بټ(بټ_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_شمېره& V3_شمېره::سېټ_ماسک(صحيح nbits, صحيح lsb) {
    سېټ_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_بټونه(lsb, std::max(0, std::min(nbits, پلنوالی() - lsb)), 1);
    أرجع *هذا;
}

//======================================================================
// ACCESSORS - as strings

string V3_شمېره::ascii(منطقي prefixed, منطقي cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (is1_ګام()) أرجع "1step";
    إذا (isNull()) أرجع "null";
    std::ostringstream بهر;
    إذا (isDouble()) {
        بهر.precision(17);
        إذا (VL_UNCOVERABLE(پلنوالی() != 64)) {
            بهر << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } وإلا {
            بهر << منزل_Double();
            إذا (منزل_Double() == floor(منزل_Double())) بهر << ".0";
        }
        أرجع بهر.str();
    } وإلا إذا (isString()) {
        أرجع '"' + منزل_String() + '"';
    } وإلا {
        إذا (VL_UNCOVERABLE((m_معلومات.عدد()[ټکي() - 1].m_ارزښت | m_معلومات.عدد()[ټکي() - 1].m_ارزښت_X)
                           & ~hi_کلمه_ماسک())) {
            بهر << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    إذا (prefixed) {
        إذا (sized()) {
            بهر << پلنوالی() << "'";
        } وإلا إذا (autoExtend() && پلنوالی() == 1) {
            بهر << "'";
            إذا (بټ_Is0(0)) {
                بهر << '0';
            } وإلا إذا (بټ_Is1(0)) {
                بهر << '1';
            } وإلا إذا (بټ_Is_Z(0)) {
                بهر << 'z';
            } وإلا {
                بهر << 'x';
            }
            أرجع بهر.str();
        } وإلا {
            إذا (cleanVerilog) {
                بهر << "'";
            } وإلا {
                بهر << "?" << پلنوالی() << "?";
            }
        }
        إذا (isSigned()) بهر << "s";
    }

    ثابت منطقي دوګونی = (isFourState()
#که_تعریف_شوی V3_شمېره_ASCII_دوګونی
                         // cppcheck-suppress konwnConditionTrueFalse
                         || ریښتیا
#د_که_پای
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    إذا (دوګونی) {
        بهر << "b";
        بهر << displayed("%0b");
    } وإلا {
        إذا (prefixed) بهر << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        بهر << displayed("%0h");
    }
    إذا (isNull()) {
        إذا (VL_UNCOVERABLE(!isEqZero())) {
            بهر << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } وإلا {
            بهر << " [null]";
        }
    }
    أرجع بهر.str();
}

منطقي V3_شمېره::displayedFmtHasArg(حرف format, منطقي is_لټون) {
    // Is this a format letter that takes an argument in runtime?
    (فراغ)is_لټون;
    بدل (std::tolower(format)) {
    حالة 'b': أرجع ریښتیا;  //دوګونی
    حالة 'c': أرجع ریښتیا;  // Character
    حالة 'd': أرجع ریښتیا;  // Decimal
    حالة 'e': أرجع ریښتیا;  // Floating
    حالة 'f': أرجع ریښتیا;  // Floating
    حالة 'g': أرجع ریښتیا;  // Floating
    حالة 'h': أرجع ریښتیا;  // Hex
    حالة 'o': أرجع ریښتیا;  // Octal
    حالة 'p': أرجع ریښتیا;  //نمونه
    حالة 's': أرجع ریښتیا;  // String
    حالة 't': أرجع ریښتیا;  //وخت
    حالة 'u': أرجع ریښتیا;  // Packed 2-state
    حالة 'v': أرجع ریښتیا;  // Strength
    حالة 'x': أرجع ریښتیا;  // Hex
    حالة 'z': أرجع ریښتیا;  // Packed 4-state
    حالة '?': أرجع ریښتیا;  // Internal: Compute format in V3Width based on data type
    افتراضي: أرجع خطأ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_شمېره::displayPad(اندازه_t fmtsize, حرف pad, منطقي کیڼ, ثابت string& دننه) VL_خالص {
    string padding;
    إذا (دننه.اوږدتیا() < fmtsize) padding = string(fmtsize - دننه.اوږدتیا(), pad);
    أرجع کیڼ ? (دننه + padding) : (padding + دننه);
}

string V3_شمېره::displayedEnumName(ثابت Ast_Enum_توکی* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    أرجع VString::isIdentifier(name) && !std::که_عدد(static_cast<بینښه حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_شمېره::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3_شمېره ارزښت{هذا, dtypep->پلنوالی()};
    ارزښت.op_ټاکل(*هذا);
    لكل (ثابت Ast_Enum_توکی* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        إذا (!constp->عدد().isAnyXZ() && ارزښت.isCaseEq(constp->عدد()))
            أرجع displayedEnumName(itemp);
    }
    أرجع "";
}

string V3_شمېره::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
    V3_شمېره ارزښت{هذا, dtypep->پلنوالی()};
    ارزښت.op_ټاکل(*هذا);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.شا());
    إذا (fmt != 'p' && fmt != 's') أرجع ارزښت.displayed(argp, vformat, numericAttr);
    string name = ارزښت.displayedEnumName(dtypep);
    إذا (name.تشه()) name = ارزښت.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_شمېره name_عدد{هذا};
    name_عدد.سېټ_String(name);
    أرجع name_عدد.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_شمېره::displayed(ثابت Ast_غوټه* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    أرجع displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_شمېره::displayed(File_کرښه* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    خپلکاری pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *هذا);
    ++pos;
    منطقي کیڼ = خطأ;
    إذا (pos[0] == '-') {
        کیڼ = ریښتیا;
        ++pos;
    }
    string fmtsize;
    لكل (; pos != vformat.cend() && (std::که_عدد(pos[0]) || pos[0] == '.'); ++pos) {
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
            V3_شمېره strwide{fl, static_cast<صحيح>(منزل_String().اوږدتیا()) * 8, 0};
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
            V3_شمېره nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*هذا);
            أرجع nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    بدل (fmt) {
    حالة 'c': {
        إذا (پلنوالی() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت بینښه صحيح v = بټونه_ارزښت(0, 8);
        str.ټېل_شا(static_cast<حرف>(v));
        أرجع str;
    }
    حالة 'e':  // FALLTHRU
    حالة 'f':  // FALLTHRU
    حالة 'g': {
        ثابت مزدوج n = formatAttr.isDouble()   ? منزل_Double()
                         : formatAttr.isSigned() ? منزل_SQuad()
                                                 : منزل_UQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_اندازه];
        (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_اندازه, vformat.c_str(), n);
        أرجع tmp;
    }
    حالة 's': {
        إذا (formatAttr.isString() || formatAttr.isComplex()) {
            str = منزل_String();
        } وإلا {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحيح بټ_2 = پلنوالی() - 1;
            منطقي پیل = ریښتیا;
            طالما ((بټ_2 % 8) != 7) ++بټ_2;
            لكل (; بټ_2 >= 0; بټ_2 -= 8) {
                ثابت صحيح v = بټونه_ارزښت(بټ_2 - 7, 8);
                إذا (!پیل || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    پیل = خطأ;  // Drop leading 0s
                } وإلا {
                    إذا (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت اندازه_t fmtsizen = static_cast<اندازه_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', کیڼ, str);
        أرجع str;
    }
    حالة 'p': {  //نمونه
        // 'p' with NUMBER was earlier converted to 'd'
        إذا (formatAttr.isDouble()) {
            ثابت مزدوج n = formatAttr.isDouble()   ? منزل_Double()
                             : formatAttr.isSigned() ? منزل_SQuad()
                                                     : منزل_UQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_اندازه];
            (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_اندازه, "%g", n);
            أرجع tmp;
        }
        إذا (formatAttr.isString())
            أرجع '"' + V3_بهر_Formatter::quoteNameControls(منزل_String()) + '"';
        إذا (formatAttr.isComplex()) أرجع منزل_String();
        أرجع "%p";
    }
    حالة 'b':  // FALLTHRU
    حالة 'o':  // FALLTHRU
    حالة 'h':  // FALLTHRU
    حالة 'x': {
        صحيح بټ_2 = پلنوالی() - 1;
        إذا (کیڼ || !fmtsize.تشه()) {
            طالما (بټ_2 && بټ_Is0(بټ_2)) --بټ_2;
        }
        بدل (fmt) {
        حالة 'b': {
            لكل (; بټ_2 >= 0; --بټ_2) {
                إذا (بټ_Is0(بټ_2)) {
                    str += '0';
                } وإلا إذا (بټ_Is1(بټ_2)) {
                    str += '1';
                } وإلا إذا (بټ_Is_Z(بټ_2)) {
                    str += 'z';
                } وإلا {
                    str += 'x';
                }
            }
            اكسر;
        }
        حالة 'o': {
            طالما ((بټ_2 % 3) != 2) ++بټ_2;
            لكل (; بټ_2 > 0; بټ_2 -= 3) {
                ثابت صحيح عدد_X = شمېر_X(بټ_2 - 2, 3);
                ثابت صحيح عدد_Z = شمېر_Z(بټ_2 - 2, 3);
                إذا (عدد_X == 3 || عدد_X == پلنوالی() - (بټ_2 - 2)) {
                    str += 'x';
                    استمر;
                }
                إذا (عدد_Z == 3 || عدد_Z == پلنوالی() - (بټ_2 - 2)) {
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
                ثابت صحيح v = بټونه_ارزښت(بټ_2 - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            اكسر;
        }
        افتراضي: {  // h/x
            طالما ((بټ_2 % 4) != 3) ++بټ_2;
            لكل (; بټ_2 > 0; بټ_2 -= 4) {
                ثابت صحيح عدد_X = شمېر_X(بټ_2 - 3, 4);
                ثابت صحيح عدد_Z = شمېر_Z(بټ_2 - 3, 4);
                إذا (عدد_X == 4 || عدد_X == پلنوالی() - (بټ_2 - 3)) {
                    str += 'x';
                    استمر;
                }
                إذا (عدد_Z == 4 || عدد_Z == پلنوالی() - (بټ_2 - 3)) {
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
                ثابت صحيح v = بټونه_ارزښت(بټ_2 - 3, 4);
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
        str = displayPad(fmtsizen, (کیڼ ? ' ' : '0'), کیڼ, str);
        أرجع str;
    }  // case b/d/x/o
    حالة 't':  //وخت
    حالة 'd': {  // Unsigned decimal
        ثابت منطقي issigned = formatAttr.isSigned();
        إذا (fmtsize == "" && !کیڼ) {
            ثابت مزدوج mantissabits = پلنوالی() - (issigned ? 1 : 0);
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
            hasXZ = ریښتیا;
        } وإلا إذا (isAllZ()) {
            str = "z";
            hasXZ = ریښتیا;
        } وإلا إذا (isAnyX()) {
            str = "X";
            hasXZ = ریښتیا;
        } وإلا إذا (isAnyZ()) {
            str = "Z";
            hasXZ = ریښتیا;
        }
        إذا (!hasXZ) {
            إذا (issigned) {
                إذا (پلنوالی() > 64) {
                    str = منزل_Decimal_S();
                } وإلا {
                    str = cvt_منزل_Str(منزل_SQuad());
                }
            } وإلا {
                إذا (پلنوالی() > 64) {
                    str = منزل_Decimal_U();
                } وإلا {
                    str = cvt_منزل_Str(منزل_UQuad());
                }
            }
        }
        ثابت منطقي zeropad = fmtsize.اوږدتیا() > 0 && fmtsize[0] == '0' && !کیڼ;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت اندازه_t fmtsizen = static_cast<اندازه_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), کیڼ, str);
        أرجع str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالة 'u': {  // Packed 2-state
        لكل (صحيح i = 0; i < ټکي(); ++i) {
            ثابت uint32_t v = m_معلومات.عدد()[i].m_ارزښت;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'z': {  // Packed 4-state
        لكل (صحيح i = 0; i < ټکي(); ++i) {
            ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
            str += static_cast<حرف>((v.m_ارزښت >> 0) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت >> 8) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت >> 16) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت >> 24) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت_X >> 0) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت_X >> 8) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت_X >> 16) & 0xff);
            str += static_cast<حرف>((v.m_ارزښت_X >> 24) & 0xff);
        }
        أرجع str;
    }
    حالة 'v': {  // Strength
        صحيح بټ_2 = پلنوالی() - 1;
        لكل (; بټ_2 >= 0; بټ_2--) {
            إذا (بټ_Is0(بټ_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            وإلا إذا (بټ_Is1(بټ_2)) {
                str += "St1 ";
            } وإلا إذا (بټ_Is_Z(بټ_2)) {
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

string V3_شمېره::emitC() ثابت VL_MT_STABLE {
    constexpr اندازه_t bufsize = 1000;
    حرف sbuf[bufsize];
    string پایله;
    إذا (isNull()) {
        أرجع "VlNull{}";
    } وإلا إذا (isDouble()) {
        ثابت مزدوج dnum = منزل_Double();
        إذا (std::isinf(dnum)) {
            إذا (std::signbit(dnum)) پایله += '-';
            پایله += "std::numeric_limits<double>::infinity()";
        } وإلا إذا (std::isnan(dnum)) {
            إذا (std::signbit(dnum)) پایله += '-';
            پایله += "std::numeric_limits<double>::quiet_NaN()";
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
            = V3_بهر_Formatter::quoteNameControls(strg, V3_بهر_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        أرجع '"' + quoted + "\"s";
    } وإلا إذا (ټکي() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        پایله = "::VlWide<" + std::منزل_string(ټکي()) + ">{{";
        إذا (ټکي() > 4) پایله += '\n';
        لكل (صحيح n = 0; n < ټکي(); ++n) {
            إذا (n) پایله += ((n % 4) ? ", " : ",\n");
            (فراغ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_کلمه(n));
            پایله += sbuf;
        }
        إذا (ټکي() > 4) پایله += '\n';
        پایله += "}}";
    } وإلا إذا (ټکي() == 2) {  // Quad
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
                                : (پلنوالی() > 16) ? ("0x%08" PRIx32 "U")
                                : (پلنوالی() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        أرجع sbuf;
    }
    أرجع پایله;
}

string V3_شمېره::منزل_Decimal_S() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (isNegative()) {
        V3_شمېره lhsNoSign = *هذا;
        lhsNoSign.opNegate(*هذا);
        أرجع "-"s + lhsNoSign.منزل_Decimal_U();
    } وإلا {
        أرجع منزل_Decimal_U();
    }
}

string V3_شمېره::منزل_Decimal_U() ثابت VL_MT_STABLE {
    ثابت صحيح maxdecwidth = (پلنوالی() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_شمېره bcd(هذا, maxdecwidth + 4);
    V3_شمېره tmp(هذا, maxdecwidth + 4);
    V3_شمېره tmp2(هذا, maxdecwidth + 4);

    صحيح from_بټ = پلنوالی() - 1;
    // Skip all leading zeros
    لكل (; from_بټ >= 0 && بټ_Is0(from_بټ); --from_بټ) {}
    // Double-dabble algorithm
    لكل (; from_بټ >= 0; --from_بټ) {
        // Any digits >= 5 need an add 3 (via tmp)
        لكل (صحيح nibble_بټ = 0; nibble_بټ < maxdecwidth; nibble_بټ += 4) {
            إذا (bcd.بټونه_ارزښت(nibble_بټ, 4) >= 5) {
                tmp2.سېټ_All_Bits0();
                tmp2.سېټ_بټ(nibble_بټ, 1);  // Add 3, decomposed as two bits
                tmp2.سېټ_بټ(nibble_بټ + 1, 1);
                tmp.op_ټاکل(bcd);
                bcd.op_جمع_کړه(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_ټاکل(bcd);
        bcd.op_ښويدنه_L(tmp, V3_شمېره(هذا, 32, 1));
        // bcd[0] = this[from_bit]
        إذا (بټ_Is1(from_بټ)) bcd.سېټ_بټ(0, 1);
    }

    string output;
    صحيح lsb = (maxdecwidth - 1) & ~3;
    لكل (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        إذا (bcd.بټونه_ارزښت(lsb, 4)) اكسر;
    }
    لكل (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.بټونه_ارزښت(lsb, 4));  // 0..9
    }
    أرجع output;
}

string V3_شمېره::منزل_Real_String() ثابت VL_MT_STABLE {
    ثابت مزدوج ارزښت = منزل_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream بهر;
    بهر.precision(15);
    بهر << ارزښت;
    string پایله = بهر.str();
    إذا (std::strtod(پایله.c_str(), تش_اشاره) != ارزښت) {
        بهر.str("");
        بهر.precision(17);
        بهر << ارزښت;
        پایله = بهر.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    إذا (پایله.پیدا_کړه_لومړی_د(".en") == string::npos) پایله += ".0";
    أرجع پایله;
}

//======================================================================
// ACCESSORS - as numbers

منطقي V3_شمېره::ځاییږي_دننه_UInt() ثابت VL_MT_خوندي {
    إذا (isFourState()) أرجع خطأ;
    // We allow wide numbers that represent values <= 32 bits
    لكل (صحيح i = 1; i < ټکي(); ++i) {
        إذا (m_معلومات.عدد()[i].m_ارزښت) أرجع خطأ;
    }
    أرجع ریښتیا;
}
uint32_t V3_شمېره::منزل_UInt() ثابت VL_MT_خوندي {
    UASSERT(!isFourState(), "toUInt with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 32 bits
    إذا (VL_UNCOVERABLE(!ځاییږي_دننه_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *هذا);
    }
    أرجع m_معلومات.عدد()[0].m_ارزښت;
}

مزدوج V3_شمېره::منزل_Double() ثابت VL_MT_خوندي {
    UASSERT(isDouble() && پلنوالی() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.u[0] = m_معلومات.عدد()[0].m_ارزښت;
    u.u[1] = m_معلومات.عدد()[1].m_ارزښت;
    أرجع u.d;
}

int32_t V3_شمېره::منزل_SInt() ثابت VL_MT_خوندي {
    إذا (isSigned()) {
        ثابت uint32_t v = منزل_UInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (پلنوالی() - 1))));
        ثابت uint32_t extended = v | signExtend;
        أرجع static_cast<int32_t>(extended);
    } وإلا {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        أرجع static_cast<int32_t>(منزل_UInt());
    }
}

uint64_t V3_شمېره::منزل_UQuad() ثابت VL_MT_خوندي {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 64 bits
    إذا (isDouble()) أرجع static_cast<uint64_t>(منزل_Double());
    لكل (صحيح i = 2; i < ټکي(); ++i) {
        إذا (m_معلومات.عدد()[i].m_ارزښت) {
            v3error("Value too wide for 64-bits expected in this context " << *هذا);
            اكسر;
        }
    }
    إذا (پلنوالی() <= 32) أرجع (static_cast<uint64_t>(منزل_UInt()));
    أرجع ((static_cast<uint64_t>(m_معلومات.عدد()[1].m_ارزښت) << 32ULL)
            | (static_cast<uint64_t>(m_معلومات.عدد()[0].m_ارزښت)));
}

int64_t V3_شمېره::منزل_SQuad() ثابت {
    إذا (isDouble()) أرجع static_cast<int64_t>(منزل_Double());
    ثابت uint64_t v = منزل_UQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (پلنوالی() - 1))));
    ثابت uint64_t extended = v | signExtend;
    أرجع static_cast<int64_t>(extended);
}

string V3_شمېره::منزل_String() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *هذا);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    إذا (isString()) أرجع m_معلومات.str();
    صحيح بټ_2 = پلنوالی() - 1;
    طالما ((بټ_2 % 8) != 7) ++بټ_2;
    std::string str;
    لكل (; بټ_2 >= 0; بټ_2 -= 8) {
        ثابت صحيح v = بټونه_ارزښت(بټ_2 - 7, 8);
        إذا (v) str += static_cast<حرف>(v);
    }
    أرجع str;
}

uint32_t V3_شمېره::edata_کلمه(صحيح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *هذا);
    أرجع m_معلومات.عدد()[eword].m_ارزښت;
}

uint8_t V3_شمېره::معلومات_Byte(صحيح byte) ثابت {
    أرجع (edata_کلمه(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقي V3_شمېره::isAllZ() ثابت VL_MT_خوندي {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        ثابت uint32_t ماسک = i == ټکي() - 1 ? hi_کلمه_ماسک() : ~0U;
        إذا (((~v.m_ارزښت & v.m_ارزښت_X) & ماسک) != ماسک) أرجع خطأ;
    }
    أرجع ریښتیا;
}
منطقي V3_شمېره::isAllX() ثابت VL_MT_خوندي {
    إذا (isDouble() || isString()) أرجع خطأ;
    uint32_t ماسک = hi_کلمه_ماسک();
    لكل (صحيح i = ټکي() - 1; i >= 0; --i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        إذا ((v.m_ارزښت & v.m_ارزښت_X) ^ ماسک) أرجع خطأ;
        ماسک = ~0U;
    }
    أرجع ریښتیا;
}
منطقي V3_شمېره::isEqZero() ثابت VL_MT_خوندي {
    إذا (isString()) أرجع m_معلومات.str().تشه();
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        إذا (v.m_ارزښت || v.m_ارزښت_X) أرجع خطأ;
    }
    أرجع ریښتیا;
}
منطقي V3_شمېره::isNeqZero() ثابت {
    إذا (isString()) أرجع !m_معلومات.str().تشه();
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        إذا (v.m_ارزښت & ~v.m_ارزښت_X) أرجع ریښتیا;
    }
    أرجع خطأ;
}
منطقي V3_شمېره::is_بټونه_Zero(صحيح msb, صحيح lsb) ثابت {
    لكل (صحيح i = lsb; i <= msb; ++i) {
        إذا (VL_UNLIKELY(!بټ_Is0(i))) أرجع خطأ;
    }
    أرجع ریښتیا;
}
منطقي V3_شمېره::isEqOne() ثابت {
    إذا (m_معلومات.عدد()[0].m_ارزښت != 1 || m_معلومات.عدد()[0].m_ارزښت_X) أرجع خطأ;
    لكل (صحيح i = 1; i < ټکي(); ++i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        إذا (v.m_ارزښت || v.m_ارزښت_X) أرجع خطأ;
    }
    أرجع ریښتیا;
}
منطقي V3_شمېره::isEqAllOnes(صحيح optwidth) ثابت {
    // Correct number of zero bits/width matters
    إذا (!optwidth) optwidth = پلنوالی();
    لكل (صحيح بټ_2 = 0; بټ_2 < optwidth; ++بټ_2) {
        إذا (!بټ_Is1(بټ_2)) أرجع خطأ;
    }
    أرجع ریښتیا;
}
منطقي V3_شمېره::isFourState() ثابت VL_MT_خوندي {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        إذا (m_معلومات.عدد()[i].m_ارزښت_X) أرجع ریښتیا;
    }
    أرجع خطأ;
}
منطقي V3_شمېره::isAnyX() ثابت VL_MT_خوندي {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        إذا (v.m_ارزښت & v.m_ارزښت_X) أرجع ریښتیا;
    }
    أرجع خطأ;
}
منطقي V3_شمېره::isAnyXZ() ثابت {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        إذا (m_معلومات.عدد()[i].m_ارزښت_X) أرجع ریښتیا;
    }
    أرجع خطأ;
}
منطقي V3_شمېره::isAnyZ() ثابت VL_MT_خوندي {
    إذا (isDouble() || isString()) أرجع خطأ;
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
        إذا (~v.m_ارزښت & v.m_ارزښت_X) أرجع ریښتیا;
    }
    أرجع خطأ;
}
منطقي V3_شمېره::isLtXZ(ثابت V3_شمېره& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    إذا (isString()) أرجع منزل_String() < rhs.منزل_String();
    إذا (isDouble()) أرجع منزل_Double() < rhs.منزل_Double();
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (بټ_Is1(بټ_2) && rhs.بټ_Is0(بټ_2)) أرجع ریښتیا;
        إذا (rhs.بټ_Is1(بټ_2) && بټ_Is0(بټ_2)) أرجع خطأ;
        إذا (بټ_Is_XZ(بټ_2)) أرجع ریښتیا;
        إذا (rhs.بټ_Is_XZ(بټ_2)) أرجع خطأ;
    }
    أرجع خطأ;
}
صحيح V3_شمېره::شمېر_X(صحيح lsb, صحيح nbits) ثابت VL_MT_خوندي {
    صحيح شمېر = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= پلنوالی()) أرجع شمېر;
        إذا (بټ_Is_X(lsb + bitn)) ++شمېر;
    }
    أرجع شمېر;
}
صحيح V3_شمېره::شمېر_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_خوندي {
    صحيح شمېر = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= پلنوالی()) أرجع شمېر;
        إذا (بټ_Is_Z(lsb + bitn)) ++شمېر;
    }
    أرجع شمېر;
}

صحيح V3_شمېره::پلنوالی_منزل_Fit() ثابت {
    لكل (صحيح بټ_2 = پلنوالی() - 1; بټ_2 > 0; بټ_2--) {
        إذا (!بټ_Is0(بټ_2)) أرجع بټ_2 + 1;
    }
    أرجع 1;  // one bit even if number is == 0
}

uint32_t V3_شمېره::شمېر_بټونه(ثابت V3_شمېره& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        بدل (ctrl.بټ_Is(0)) {
        حالة '0':
            إذا (بټ_Is0(بټ_2)) ++n;
            اكسر;
        حالة '1':
            إذا (بټ_Is1(بټ_2)) ++n;
            اكسر;
        حالة 'x':
            إذا (بټ_Is_X(بټ_2)) ++n;
            اكسر;
        حالة 'z':
            إذا (بټ_Is_Z(بټ_2)) ++n;
            اكسر;
        }
    }
    أرجع n;
}

uint32_t V3_شمېره::شمېر_بټونه(ثابت V3_شمېره& ctrl1, ثابت V3_شمېره& ctrl2,
                             ثابت V3_شمېره& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = شمېر_بټونه(ctrl1);
    إذا (ctrl2.بټ_Is(0) != ctrl1.بټ_Is(0)) n += شمېر_بټونه(ctrl2);
    إذا ((ctrl3.بټ_Is(0) != ctrl1.بټ_Is(0)) && (ctrl3.بټ_Is(0) != ctrl2.بټ_Is(0))) {
        n += شمېر_بټونه(ctrl3);
    }
    أرجع n;
}

uint32_t V3_شمېره::شمېر_Ones() ثابت {
    صحيح n = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (بټ_Is1(بټ_2)) ++n;
    }
    أرجع n;
}

uint32_t V3_شمېره::most_سېټ_بټ_P1() ثابت {
    لكل (صحيح بټ_2 = پلنوالی() - 1; بټ_2 >= 0; بټ_2--) {
        إذا (!بټ_Is0(بټ_2)) أرجع بټ_2 + 1;
    }
    أرجع 0;
}

uint32_t V3_شمېره::لږترلږه_سېټ_بټ_P1() ثابت {
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (!بټ_Is0(بټ_2)) أرجع بټ_2 + 1;
    }
    أرجع 0;
}

//======================================================================

V3_شمېره& V3_شمېره::op_بټونه_Non_XZ(ثابت V3_شمېره& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is0(بټ_2) || lhs.بټ_Is1(بټ_2)) سېټ_بټ(بټ_2, 1);
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_بټونه_One(ثابت V3_شمېره& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2)) سېټ_بټ(بټ_2, 1);
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_بټونه_XZ(ثابت V3_شمېره& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is_XZ(بټ_2)) سېټ_بټ(بټ_2, 1);
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_بټونه_Z(ثابت V3_شمېره& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is_Z(بټ_2)) سېټ_بټ(بټ_2, 1);
    }
    أرجع *هذا;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_شمېره& V3_شمېره::opRedOr(ثابت V3_شمېره& lhs) {
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < lhs.پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            أرجع سېټ_Single_بټونه(1);
        } وإلا إذا (lhs.بټ_Is0(بټ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opRedAnd(ثابت V3_شمېره& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    لكل (صحيح بټ_2 = 0; بټ_2 < lhs.پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is0(بټ_2)) {
            أرجع سېټ_Single_بټونه(0);
        } وإلا إذا (lhs.بټ_Is1(بټ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opRedXor(ثابت V3_شمېره& lhs) {
    // op i, 1 bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < lhs.پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            إذا (outc == 1) {
                outc = 0;
            } وإلا إذا (outc == 0) {
                outc = 1;
            }
        } وإلا إذا (lhs.بټ_Is0(بټ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::op_شمېر_بټونه(ثابت V3_شمېره& بیان, ثابت V3_شمېره& ctrl1, ثابت V3_شمېره& ctrl2,
                                ثابت V3_شمېره& ctrl3) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS4(بیان, ctrl1, ctrl2, ctrl3);
    عدد_ASSERT_LOGIC_ARGS4(بیان, ctrl1, ctrl2, ctrl3);
    سېټ_Zero();
    m_معلومات.عدد()[0].m_ارزښت = بیان.شمېر_بټونه(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_شمېر_Ones(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سېټ_All_بټونه_X();
    سېټ_Zero();
    m_معلومات.عدد()[0].m_ارزښت = lhs.شمېر_Ones();
    opCleanThis();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opIsUnknown(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    أرجع سېټ_Single_بټونه(lhs.isAnyXZ());
}
V3_شمېره& V3_شمېره::opOneHot(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سېټ_All_بټونه_X();
    أرجع سېټ_Single_بټونه(lhs.شمېر_Ones() == 1);
}
V3_شمېره& V3_شمېره::opOneHot0(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سېټ_All_بټونه_X();
    أرجع سېټ_Single_بټونه(lhs.شمېر_Ones() <= 1);
}
V3_شمېره& V3_شمېره::opCLog2(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سېټ_All_بټونه_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحيح adjust = (lhs.شمېر_Ones() == 1) ? 0 : 1;
    لكل (صحيح بټ_2 = lhs.پلنوالی() - 1; بټ_2 >= 0; بټ_2--) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            سېټ_Long(بټ_2 + adjust);
            أرجع *هذا;
        }
    }
    سېټ_Zero();
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_Most_سېټ_بټ_P1(ثابت V3_شمېره& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سېټ_All_بټونه_X();
    لكل (صحيح بټ_2 = lhs.پلنوالی() - 1; بټ_2 >= 0; بټ_2--) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            سېټ_Long(بټ_2 + 1);
            أرجع *هذا;
        }
    }
    سېټ_Zero();
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opLogNot(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    لكل (صحيح بټ_2 = 0; بټ_2 < lhs.پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            outc = 0;
            اذهب وروستی;
        } وإلا إذا (lhs.بټ_Is0(بټ_2)) {
        } وإلا {
            outc = 'x';
        }
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opNot(ثابت V3_شمېره& lhs) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is0(بټ_2)) {
            سېټ_بټ(بټ_2, 1);
        } وإلا إذا (lhs.بټ_Is_XZ(بټ_2)) {
            سېټ_بټ(بټ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opAnd(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2) && rhs.بټ_Is1(بټ_2)) {
            سېټ_بټ(بټ_2, 1);
        } وإلا إذا (lhs.بټ_Is0(بټ_2) || rhs.بټ_Is0(بټ_2)) {  // 0
        } وإلا {
            سېټ_بټ(بټ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opOr(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2) || rhs.بټ_Is1(بټ_2)) {
            سېټ_بټ(بټ_2, 1);
        } وإلا إذا (lhs.بټ_Is0(بټ_2) && rhs.بټ_Is0(بټ_2)) {
            // 0
        } وإلا {
            سېټ_بټ(بټ_2, 'x');
        }
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opXor(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2) && rhs.بټ_Is0(بټ_2)) {
            سېټ_بټ(بټ_2, 1);
        } وإلا إذا (lhs.بټ_Is0(بټ_2) && rhs.بټ_Is1(بټ_2)) {
            سېټ_بټ(بټ_2, 1);
        } وإلا إذا (lhs.بټ_Is_XZ(بټ_2) || rhs.بټ_Is_XZ(بټ_2)) {
            سېټ_بټ(بټ_2, 'x');
        }
        // else zero
    }
    أرجع *هذا;
}

فراغ V3_شمېره::fill_بټونه(صحيح destLsb, صحيح پلنوالی, حرف ارزښت) {
    UASSERT(destLsb >= 0 && پلنوالی >= 0, "Negative bit range");
    إذا (!پلنوالی) أرجع;
    UASSERT(پلنوالی <= هذا->پلنوالی() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t ارزښت_کلمه = ارزښت == '1' || ارزښت == 'x' || ارزښت == 1 || ارزښت == 3 ? ~0U : 0;
    ثابت uint32_t x_کلمه = ارزښت == 'z' || ارزښت == 'x' || ارزښت == 2 || ارزښت == 3 ? ~0U : 0;
    ثابت صحيح لومړی_کلمه = VL_BITWORD_E(destLsb);
    ثابت صحيح وروستی_کلمه = VL_BITWORD_E(destLsb + پلنوالی - 1);
    لكل (صحيح کلمه = لومړی_کلمه; کلمه <= وروستی_کلمه; ++کلمه) {
        ثابت صحيح کلمه_Lsb = کلمه == لومړی_کلمه ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحيح کلمه_Msb = کلمه == وروستی_کلمه ? VL_BITBIT_E(destLsb + پلنوالی - 1) : VL_SIZEBITS_E;
        ثابت uint32_t ماسک = VL_ماسک_E(کلمه_Msb - کلمه_Lsb + 1) << کلمه_Lsb;
        ارزښت_And_X& dest = m_معلومات.عدد()[کلمه];
        dest.m_ارزښت = (dest.m_ارزښت & ~ماسک) | (ارزښت_کلمه & ماسک);
        dest.m_ارزښت_X = (dest.m_ارزښت_X & ~ماسک) | (x_کلمه & ماسک);
    }
}

فراغ V3_شمېره::copy_بټونه(صحيح destLsb, ثابت V3_شمېره& سرچینه, صحيح سرچینه_Lsb, صحيح پلنوالی) {
    UASSERT(destLsb >= 0 && سرچینه_Lsb >= 0 && پلنوالی >= 0, "Negative bit range");
    إذا (!پلنوالی) أرجع;
    UASSERT(پلنوالی <= هذا->پلنوالی() - destLsb, "Destination bit range exceeds number width");
    UASSERT(پلنوالی <= سرچینه.پلنوالی() - سرچینه_Lsb, "Source bit range exceeds number width");

    ثابت صحيح لومړی_کلمه = VL_BITWORD_E(destLsb);
    ثابت صحيح وروستی_کلمه = VL_BITWORD_E(destLsb + پلنوالی - 1);
    لكل (صحيح dest_کلمه = لومړی_کلمه; dest_کلمه <= وروستی_کلمه; ++dest_کلمه) {
        ثابت صحيح dest_بټ = std::max(destLsb, dest_کلمه * VL_EDATASIZE);
        ثابت صحيح dest_دننه_واټن = VL_BITBIT_E(dest_بټ);
        ثابت صحيح سرچینه_بټ = سرچینه_Lsb + dest_بټ - destLsb;
        ثابت صحيح سرچینه_کلمه = VL_BITWORD_E(سرچینه_بټ);
        ثابت صحيح سرچینه_دننه_واټن = VL_BITBIT_E(سرچینه_بټ);
        ثابت صحيح copy_پلنوالی = std::min(پلنوالی - (dest_بټ - destLsb), VL_EDATASIZE - dest_دننه_واټن);
        ثابت uint32_t ماسک = VL_ماسک_E(copy_پلنوالی) << dest_دننه_واټن;

        ثابت ارزښت_And_X& سرچینه_Lo = سرچینه.m_معلومات.عدد()[سرچینه_کلمه];
        uint32_t ارزښت = سرچینه_Lo.m_ارزښت >> سرچینه_دننه_واټن;
        uint32_t ارزښت_X = سرچینه_Lo.m_ارزښت_X >> سرچینه_دننه_واټن;
        إذا (سرچینه_دننه_واټن && copy_پلنوالی > VL_EDATASIZE - سرچینه_دننه_واټن) {
            ثابت ارزښت_And_X& سرچینه_Hi = سرچینه.m_معلومات.عدد()[سرچینه_کلمه + 1];
            ارزښت |= سرچینه_Hi.m_ارزښت << (VL_EDATASIZE - سرچینه_دننه_واټن);
            ارزښت_X |= سرچینه_Hi.m_ارزښت_X << (VL_EDATASIZE - سرچینه_دننه_واټن);
        }

        ارزښت_And_X& dest = m_معلومات.عدد()[dest_کلمه];
        dest.m_ارزښت = (dest.m_ارزښت & ~ماسک) | ((ارزښت << dest_دننه_واټن) & ماسک);
        dest.m_ارزښت_X = (dest.m_ارزښت_X & ~ماسک) | ((ارزښت_X << dest_دننه_واټن) & ماسک);
    }
}

V3_شمېره& V3_شمېره::opConcat(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سېټ_Zero();
    // See also error in V3Width
    إذا (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_بټونه(0, rhs, 0, rhs.پلنوالی());
    copy_بټونه(rhs.پلنوالی(), lhs, 0, lhs.پلنوالی());
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opLenN(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    سېټ_Quad(lhs.منزل_String().اوږدتیا());
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opRepl(ثابت V3_شمېره& lhs,
                           ثابت V3_شمېره& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    أرجع opRepl(lhs, rhs.منزل_UInt());
}

V3_شمېره& V3_شمېره::opRepl(ثابت V3_شمېره& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (v3Global.opt.replication_حد()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_حد())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_حد() << " is suspect: " << rhsval);
    }
    سېټ_Zero();
    لكل (بینښه times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.پلنوالی();
        إذا (destLsb >= static_cast<uint32_t>(پلنوالی())) اكسر;
        copy_بټونه(static_cast<صحيح>(destLsb), lhs, 0,
                 std::min(lhs.پلنوالی(), پلنوالی() - static_cast<صحيح>(destLsb)));
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::op_بهیر_L(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سېټ_Zero();
    // See also error in V3Width
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحيح ssize = std::min(rhs.منزل_UInt(), static_cast<بینښه>(lhs.پلنوالی()));
    لكل (صحيح istart = 0; istart < lhs.پلنوالی(); istart += ssize) {
        ثابت صحيح ostart = std::max(0, lhs.پلنوالی() - ssize - istart);
        copy_بټونه(ostart, lhs, istart, std::min(ssize, lhs.پلنوالی() - istart));
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opLogAnd(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < lhs.پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            loutc = 1;
            اكسر;
        }
        إذا (lhs.بټ_Is_XZ(بټ_2) && loutc == 0) loutc = 'x';
    }
    لكل (صحيح بټ_2 = 0; بټ_2 < rhs.پلنوالی(); ++بټ_2) {
        إذا (rhs.بټ_Is1(بټ_2)) {
            routc = 1;
            اكسر;
        }
        إذا (rhs.بټ_Is_XZ(بټ_2) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    إذا (routc == 1 && loutc == 1) outc = 1;
    إذا (routc == 0 || loutc == 0) outc = 0;
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opLogOr(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < lhs.پلنوالی(); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2)) {
            outc = 1;
            اذهب وروستی;
        }
        إذا (lhs.بټ_Is_XZ(بټ_2) && outc == 0) outc = 'x';
    }
    لكل (صحيح بټ_2 = 0; بټ_2 < rhs.پلنوالی(); ++بټ_2) {
        إذا (rhs.بټ_Is1(بټ_2)) {
            outc = 1;
            اذهب وروستی;
        }
        إذا (rhs.بټ_Is_XZ(بټ_2) && outc == 0) outc = 'x';
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opLogIf(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_شمېره lnot = lhs;
    lnot.opLogNot(lhs);
    أرجع opLogOr(lnot, rhs);
}

V3_شمېره& V3_شمېره::opLogEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_شمېره ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_شمېره ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    أرجع opLogAnd(ifa, ifb);
}

V3_شمېره& V3_شمېره::opAtoN(ثابت V3_شمېره& lhs, صحيح بنسټ) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    UASSERT(بنسټ == AstAtoN::ATOREAL || بنسټ == 2 || بنسټ == 8 || بنسټ == 10 || بنسټ == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.منزل_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.پای_ټکی(), '_'), str.پای_ټکی());

    إذا (بنسټ == AstAtoN::ATOREAL) أرجع سېټ_Double(std::atof(str.c_str()));

    errno = 0;
    خپلکاری v = std::strtol(str.c_str(), تش_اشاره, بنسټ);
    إذا (errno != 0) v = 0;
    أرجع سېټ_Long_S(static_cast<int32_t>(v));
}

V3_شمېره& V3_شمېره::opPutcN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, ثابت V3_شمېره& ths) {
    عدد_ASSERT_OP_ARGS3(lhs, rhs, ths);
    عدد_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    ثابت int32_t c = ths.منزل_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.اوږدتیا() && c != 0) lstring[i] = c;
    أرجع سېټ_String(lstring);
}

V3_شمېره& V3_شمېره::opGetcN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.اوږدتیا()) v = lstring[i];
    أرجع سېټ_Long(v);
}

V3_شمېره& V3_شمېره::opSubstrN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, ثابت V3_شمېره& ths) {
    عدد_ASSERT_OP_ARGS3(lhs, rhs, ths);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.منزل_String();
    ثابت int32_t i = rhs.منزل_SInt();
    ثابت int32_t j = ths.منزل_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    إذا (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.اوږدتیا()) أرجع سېټ_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    أرجع سېټ_String(lstring.substr(i, j - i + 1));
}

V3_شمېره& V3_شمېره::opCompareNN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, منطقي ignoreCase) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحيح پایله;
    ثابت string lstring = lhs.منزل_String();
    ثابت string rstring = rhs.منزل_String();
    إذا (ignoreCase) {
        پایله = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } وإلا {
        پایله = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    أرجع سېټ_Long_S(پایله);
}

V3_شمېره& V3_شمېره::opEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opEqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opEqD(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2) && rhs.بټ_Is0(بټ_2)) {
            outc = 0;
            اذهب وروستی;
        }
        إذا (lhs.بټ_Is0(بټ_2) && rhs.بټ_Is1(بټ_2)) {
            outc = 0;
            اذهب وروستی;
        }
        إذا (lhs.بټ_Is_XZ(بټ_2)) outc = 'x';
        إذا (rhs.بټ_Is_XZ(بټ_2)) outc = 'x';
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opNeq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) أرجع opNeqN(lhs, rhs);
    إذا (lhs.isDouble()) أرجع opNeqD(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2) && rhs.بټ_Is0(بټ_2)) {
            outc = 1;
            اذهب وروستی;
        }
        إذا (lhs.بټ_Is0(بټ_2) && rhs.بټ_Is1(بټ_2)) {
            outc = 1;
            اذهب وروستی;
        }
        إذا (lhs.بټ_Is_XZ(بټ_2)) outc = 'x';
        إذا (rhs.بټ_Is_XZ(بټ_2)) outc = 'x';
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3Hash V3_شمېره::منزل_Hash() ثابت {
    // Shouldmatch isCaseEq
    إذا (isString()) أرجع V3Hash{منزل_String()};
    إذا (isDouble()) أرجع V3Hash{منزل_Double()};
    V3Hash hash{پلنوالی()};
    لكل (صحيح i = 0; i < ټکي(); ++i) hash += m_معلومات.عدد()[i].m_ارزښت;
    أرجع hash;
}

منطقي V3_شمېره::isCaseEq(ثابت V3_شمېره& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    إذا (isString()) أرجع منزل_String() == rhs.منزل_String();
    إذا (isDouble()) أرجع منزل_Double() == rhs.منزل_Double();
    إذا (پلنوالی() != rhs.پلنوالی()) أرجع خطأ;
    لكل (صحيح i = 0; i < ټکي(); ++i) {
        إذا (!(m_معلومات.عدد()[i] == rhs.m_معلومات.عدد()[i])) أرجع خطأ;
    }
    أرجع ریښتیا;
}

V3_شمېره& V3_شمېره::opCaseEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_شمېره& V3_شمېره::opCaseNeq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    إذا (lhs.isString()) {
        أرجع opNeqN(lhs, rhs);
    } وإلا إذا (lhs.isDouble()) {
        أرجع opNeqD(lhs, rhs);
    }
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (lhs.بټ_Is(بټ_2) != rhs.بټ_Is(بټ_2)) {
            outc = 1;
            اذهب وروستی;
        }
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opWildEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (!rhs.بټ_Is_XZ(بټ_2)) {
            إذا (lhs.بټ_Is_XZ(بټ_2)) {
                outc = 'x';
                اذهب وروستی;
            }
            إذا (lhs.بټ_Is(بټ_2) != rhs.بټ_Is(بټ_2)) {
                outc = 0;
                اذهب وروستی;
            }
        }
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opWildNeq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (!rhs.بټ_Is_XZ(بټ_2)) {
            إذا (lhs.بټ_Is_XZ(بټ_2)) {
                outc = 'x';
                اذهب وروستی;
            }
            إذا (lhs.بټ_Is(بټ_2) != rhs.بټ_Is(بټ_2)) {
                outc = 1;
                اذهب وروستی;
            }
        }
    }
وروستی:
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opGt(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی(), rhs.پلنوالی()); ++بټ_2) {
        إذا (lhs.بټ_Is1(بټ_2) && rhs.بټ_Is0(بټ_2)) outc = 1;
        إذا (rhs.بټ_Is1(بټ_2) && lhs.بټ_Is0(بټ_2)) outc = 0;
        إذا (lhs.بټ_Is_XZ(بټ_2)) outc = 'x';
        إذا (rhs.بټ_Is_XZ(بټ_2)) outc = 'x';
    }
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opGtS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحيح mbit = std::max(lhs.پلنوالی() - 1, rhs.پلنوالی() - 1);
        إذا (lhs.بټ_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (rhs.بټ_Is_XZ(mbit)) {
            outc = 'x';
        } وإلا إذا (lhs.بټ_Is0(mbit) && rhs.بټ_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } وإلا إذا (lhs.بټ_Is1_Extend(mbit) && rhs.بټ_Is0(mbit)) {
            outc = 0;  // - !> +
        } وإلا {
            // both positive or negative, normal >
            لكل (صحيح بټ_2 = 0; بټ_2 < std::max(lhs.پلنوالی() - 1, rhs.پلنوالی() - 1); ++بټ_2) {
                إذا (lhs.بټ_Is1_Extend(بټ_2) && rhs.بټ_Is0(بټ_2)) outc = 1;
                إذا (rhs.بټ_Is1_Extend(بټ_2) && lhs.بټ_Is0(بټ_2)) outc = 0;
                إذا (lhs.بټ_Is_XZ(بټ_2)) outc = 'x';
                إذا (rhs.بټ_Is_XZ(بټ_2)) outc = 'x';
            }
        }
    }
    أرجع سېټ_Single_بټونه(outc);
}

V3_شمېره& V3_شمېره::opGte(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_شمېره& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGt(lhs, rhs);
}

V3_شمېره& V3_شمېره::opGteS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    V3_شمېره& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) أرجع eq;  // Return true
    أرجع opGtS(lhs, rhs);
}

V3_شمېره& V3_شمېره::opLt(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) { أرجع opGt(rhs, lhs); }
V3_شمېره& V3_شمېره::opLte(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) { أرجع opGte(rhs, lhs); }
V3_شمېره& V3_شمېره::opLtS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) { أرجع opGtS(rhs, lhs); }
V3_شمېره& V3_شمېره::opLteS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) { أرجع opGteS(rhs, lhs); }

V3_شمېره& V3_شمېره::op_ښويدنه_R(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // L(lhs) bit return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    سېټ_Zero();
    إذا (rhs.پلنوالی() > 32 && !rhs.is_بټونه_Zero(rhs.پلنوالی() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.منزل_UInt();
    إذا (rhsval < static_cast<uint32_t>(lhs.پلنوالی())) {
        copy_بټونه(0, lhs, static_cast<صحيح>(rhsval),
                 std::min(پلنوالی(), lhs.پلنوالی() - static_cast<صحيح>(rhsval)));
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::op_ښويدنه_RS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    سېټ_Zero();
    ثابت منطقي ډکېدنه = rhs.پلنوالی() > 32 && !rhs.is_بټونه_Zero(rhs.پلنوالی() - 1, 32);
    إذا (!ډکېدنه) {
        ثابت uint32_t rhsval = rhs.منزل_UInt();
        إذا (rhsval < lbits) {
            ثابت صحيح copy_پلنوالی = std::min(پلنوالی(), static_cast<صحيح>(lbits - rhsval));
            copy_بټونه(0, lhs, static_cast<صحيح>(rhsval), copy_پلنوالی);
            fill_بټونه(copy_پلنوالی, پلنوالی() - copy_پلنوالی, lhs.بټ_Is(lbits - 1));
            أرجع *هذا;
        }
    }
    fill_بټونه(0, پلنوالی(), lhs.بټ_Is(lbits - 1));  // '0/'1/'x/'z
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::op_ښويدنه_L(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // L(lhs) bit return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    سېټ_Zero();
    إذا (rhs.پلنوالی() > 32 && !rhs.is_بټونه_Zero(rhs.پلنوالی() - 1, 32)) أرجع *هذا;
    ثابت uint32_t rhsval = rhs.منزل_UInt();
    إذا (rhsval < static_cast<uint32_t>(پلنوالی())) {
        copy_بټونه(static_cast<صحيح>(rhsval), lhs, 0,
                 std::min(پلنوالی() - static_cast<صحيح>(rhsval), lhs.پلنوالی()));
    }
    أرجع *هذا;
}

//======================================================================
// Ops - Arithmetic

V3_شمېره& V3_شمېره::opNegate(ثابت V3_شمېره& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) أرجع سېټ_All_بټونه_X();
    V3_شمېره notlhs(&lhs, پلنوالی());
    notlhs.opNot(lhs);
    ثابت V3_شمېره one(&lhs, پلنوالی(), 1);
    op_جمع_کړه(notlhs, one);
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_جمع_کړه(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    سېټ_Zero();
    // Addem
    uint64_t لېږد = 0;
    لكل (صحيح کلمه = 0; کلمه < ټکي(); ++کلمه) {
        ثابت uint64_t lwordval = lhs.m_معلومات.عدد()[کلمه].m_ارزښت;
        ثابت uint64_t rwordval = rhs.m_معلومات.عدد()[کلمه].m_ارزښت;
        ثابت uint64_t مجموعه = lwordval + rwordval + لېږد;
        m_معلومات.عدد()[کلمه].m_ارزښت = مجموعه & 0xffffffffULL;
        لېږد = مجموعه > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opSub(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    V3_شمېره negrhs(&rhs, پلنوالی());
    negrhs.opNegate(rhs);
    أرجع op_جمع_کړه(lhs, negrhs);
}
V3_شمېره& V3_شمېره::opMul(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    سېټ_Zero();
    إذا (پلنوالی() <= 64) {
        سېټ_Quad(lhs.منزل_UQuad() * rhs.منزل_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } وإلا {
        لكل (صحيح lword = 0; lword < lhs.ټکي(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_معلومات.عدد()[lword].m_ارزښت);
            إذا (lwordval == 0) استمر;
            لكل (صحيح rword = 0; rword < rhs.ټکي(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_معلومات.عدد()[rword].m_ارزښت);
                إذا (rwordval == 0) استمر;
                uint64_t mul = lwordval * rwordval;
                لكل (صحيح qword = lword + rword; qword < ټکي(); ++qword) {
                    mul += static_cast<uint64_t>(m_معلومات.عدد()[qword].m_ارزښت);
                    m_معلومات.عدد()[qword].m_ارزښت = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    إذا (mul == 0) اكسر;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opMulS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    V3_شمېره lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_شمېره rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_شمېره qNoSign = opMul(lhsNoSign, rhsNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_ټاکل(qNoSign);
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opDiv(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    إذا (rhs.isEqZero()) أرجع سېټ_All_بټونه_XRemoved();
    إذا (lhs.پلنوالی() <= 64) {
        سېټ_Quad(lhs.منزل_UQuad() / rhs.منزل_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide division
        أرجع opModDivGuts(lhs, rhs, خطأ);
    }
}
V3_شمېره& V3_شمېره::opDivS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    إذا (rhs.isEqZero()) أرجع سېټ_All_بټونه_XRemoved();
    V3_شمېره lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_شمېره rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_شمېره qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        op_ټاکل(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *هذا);
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opModDiv(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    إذا (rhs.isEqZero()) أرجع سېټ_All_بټونه_XRemoved();
    إذا (lhs.پلنوالی() <= 64) {
        سېټ_Quad(lhs.منزل_UQuad() % rhs.منزل_UQuad());
        أرجع *هذا;
    } وإلا {
        // Wide modulus
        أرجع opModDivGuts(lhs, rhs, ریښتیا);
    }
}
V3_شمېره& V3_شمېره::opModDivS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    إذا (rhs.isEqZero()) أرجع سېټ_All_بټونه_XRemoved();
    V3_شمېره lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_شمېره rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_شمېره qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    إذا (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } وإلا {
        op_ټاکل(qNoSign);
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opModDivGuts(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, منطقي is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    سېټ_Zero();
    // Find MSB and check for zero.
    ثابت صحيح l_ټکي = lhs.ټکي();
    ثابت صحيح umsbp1 = lhs.most_سېټ_بټ_P1();  // dividend
    ثابت صحيح vmsbp1 = rhs.most_سېټ_بټ_P1();  //وېشونکی
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
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_معلومات.عدد()[j].m_ارزښت));
            m_معلومات.عدد()[j].m_ارزښت = unw64 / static_cast<uint64_t>(rhs.m_معلومات.عدد()[0].m_ارزښت);
            k = unw64
                - (static_cast<uint64_t>(m_معلومات.عدد()[j].m_ارزښت)
                   * static_cast<uint64_t>(rhs.m_معلومات.عدد()[0].m_ارزښت));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *هذا << " rem=0x" << std::hex
                                   << k << std::dec);
        إذا (is_modulus) {
            سېټ_Zero();
            m_معلومات.عدد()[0].m_ارزښت = k;
        }
        opCleanThis();
        أرجع *هذا;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_ټکي + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_ټکي + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    لكل (صحيح i = 0; i < l_ټکي; ++i) m_معلومات.عدد()[i].m_ارزښت = 0;
    لكل (صحيح i = 0; i < l_ټکي + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحيح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t ښويدنه_ماسک = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    لكل (صحيح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_معلومات.عدد()[i].m_ارزښت << s)
                | (ښويدنه_ماسک & (rhs.m_معلومات.عدد()[i - 1].m_ارزښت >> (32 - s)));
    }
    vn[0] = rhs.m_معلومات.عدد()[0].m_ارزښت << s;

    // Copy and shift dividend by same amount; may set new upper word
    إذا (s) {
        un[uw] = lhs.m_معلومات.عدد()[uw - 1].m_ارزښت >> (32 - s);
    } وإلا {
        un[uw] = 0;
    }
    لكل (صحيح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_معلومات.عدد()[i].m_ارزښت << s)
                | (ښويدنه_ماسک & (lhs.m_معلومات.عدد()[i - 1].m_ارزښت >> (32 - s)));
    }
    un[0] = lhs.m_معلومات.عدد()[0].m_ارزښت << s;

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
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //کمول
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_معلومات.عدد()[j].m_ارزښت = qhat;  // Save quotient digit

        إذا (t < 0) {
            // Over subtracted; correct by adding back
            m_معلومات.عدد()[j].m_ارزښت--;
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
            m_معلومات.عدد()[i].m_ارزښت = (un[i] >> s) | (ښويدنه_ماسک & (un[i + 1] << (32 - s)));
        }
        لكل (صحيح i = vw; i < l_ټکي; ++i) m_معلومات.عدد()[i].m_ارزښت = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    } وإلا {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *هذا);
        أرجع *هذا;
    }
}

V3_شمېره& V3_شمېره::opPow(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, منطقي lsign, منطقي rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) أرجع سېټ_All_بټونه_X();
    إذا (rhs.isEqZero()) أرجع سېټ_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    إذا (rsign && rhs.isNegative()) {
        إذا (lhs.isEqZero()) {
            أرجع سېټ_All_بټونه_XRemoved();
        } وإلا إذا (lhs.isEqOne()) {
            أرجع سېټ_Value1();
        } وإلا إذا (lsign && lhs.isEqAllOnes()) {
            إذا (rhs.بټ_Is1(0)) {
                أرجع سېټ_All_Bits1();  // -1^odd=-1
            } وإلا {
                أرجع سېټ_Value1();  // -1^even=1
            }
        }
        أرجع سېټ_Zero();
    }
    إذا (lhs.isEqZero()) أرجع سېټ_Zero();
    سېټ_Zero();
    m_معلومات.عدد()[0].m_ارزښت = 1;
    V3_شمېره power(&lhs, پلنوالی());
    power.op_ټاکل(lhs);
    لكل (صحيح بټ_2 = 0; بټ_2 < rhs.پلنوالی(); ++بټ_2) {
        إذا (بټ_2 > 0) {  // power = power*power
            V3_شمېره وروستی_Power(&lhs, پلنوالی());
            وروستی_Power.op_ټاکل(power);
            power.opMul(وروستی_Power, وروستی_Power);
        }
        إذا (rhs.بټ_Is1(بټ_2)) {  // out *= power
            V3_شمېره وروستی_بهر(&lhs, پلنوالی());
            وروستی_بهر.op_ټاکل(*هذا);
            opMul(وروستی_بهر, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::opPowSU(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, ریښتیا, خطأ);
}
V3_شمېره& V3_شمېره::opPowSS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, ریښتیا, ریښتیا);
}
V3_شمېره& V3_شمېره::opPowUS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // Correct number of zero bits/width matters
    أرجع opPow(lhs, rhs, خطأ, ریښتیا);
}

V3_شمېره& V3_شمېره::opBufIf1(ثابت V3_شمېره& ens, ثابت V3_شمېره& if1s) {
    عدد_ASSERT_OP_ARGS2(ens, if1s);
    عدد_ASSERT_LOGIC_ARGS2(ens, if1s);
    سېټ_Zero();
    لكل (صحيح بټ_2 = 0; بټ_2 < پلنوالی(); ++بټ_2) {
        إذا (ens.بټ_Is1(بټ_2)) {
            سېټ_بټ(بټ_2, if1s.بټ_Is(بټ_2));
        } وإلا {
            سېټ_بټ(بټ_2, 'z');
        }
    }
    أرجع *هذا;
}

// Sets all bits in range to the given value
V3_شمېره& V3_شمېره::op_سېټ_Range(uint32_t lsb, uint32_t پلنوالی, حرف بټ_ارزښت) {
    إذا (lsb < static_cast<uint32_t>(هذا->پلنوالی())) {
        fill_بټونه(static_cast<صحيح>(lsb),
                 static_cast<صحيح>(std::min(پلنوالی, static_cast<uint32_t>(هذا->پلنوالی()) - lsb)),
                 بټ_ارزښت);
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::op_ټاکل(ثابت V3_شمېره& lhs) { أرجع op_ټاکل_Non_XZ(lhs, خطأ); }
V3_شمېره& V3_شمېره::op_ټاکل_Non_XZ(ثابت V3_شمېره& lhs, منطقي ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    إذا (هذا != &lhs) {
        إذا (VL_UNLIKELY(lhs.isNull())) {
            m_معلومات.m_isNull = ریښتیا;
        } وإلا إذا (isString()) {
            إذا (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_معلومات.str() = lhs.منزل_String();
            } وإلا {
                m_معلومات.str() = lhs.m_معلومات.str();
            }
        } وإلا إذا (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            سېټ_Zero();
        } وإلا إذا (lhs.isDouble()) {
            سېټ_Double(lhs.منزل_Double());
        } وإلا إذا (!ignoreXZ) {
            سېټ_Zero();
            copy_بټونه(0, lhs, 0, std::min(پلنوالی(), lhs.پلنوالی()));
        } وإلا {
            لكل (صحيح کلمه = 0; کلمه < ټکي(); ++کلمه) {
                ثابت uint32_t ارزښت = کلمه < lhs.ټکي() ? lhs.m_معلومات.عدد()[کلمه].m_ارزښت
                                                                & ~lhs.m_معلومات.عدد()[کلمه].m_ارزښت_X
                                                          : 0;
                m_معلومات.عدد()[کلمه] = {ارزښت, 0};
            }
            opCleanThis();
        }
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opNToI(ثابت V3_شمېره& lhs) {
    // String to packed number
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    سېټ_Zero();
    ثابت string& str = lhs.منزل_String();
    لكل (اندازه_t n = 0; n < str.اوږدتیا(); ++n) {
        ثابت حرف c = str[str.اوږدتیا() - 1 - n];
        لكل (اندازه_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<بینښه اوږد اوږد>(c)};
            سېټ_بټ(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opExtendS(ثابت V3_شمېره& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    ثابت صحيح copy_پلنوالی = std::min(پلنوالی(), static_cast<صحيح>(lbits));
    copy_بټونه(0, lhs, 0, copy_پلنوالی);
    fill_بټونه(copy_پلنوالی, پلنوالی() - copy_پلنوالی, lhs.بټ_Is(lbits - 1));
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opExtendXZ(ثابت V3_شمېره& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    ثابت صحيح copy_پلنوالی = std::min(پلنوالی(), static_cast<صحيح>(lbits));
    copy_بټونه(0, lhs, 0, copy_پلنوالی);
    fill_بټونه(copy_پلنوالی, پلنوالی() - copy_پلنوالی, lhs.بټ_Is(lbits - 1));
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opClean(ثابت V3_شمېره& lhs, uint32_t بټونه) { أرجع opSel(lhs, بټونه - 1, 0); }

فراغ V3_شمېره::opCleanThis(منطقي warnOnTruncation) {
    // Clean MSB of number
    عدد_ASSERT_LOGIC_ARGS1(*هذا);
    ثابت ارزښت_And_X v = m_معلومات.عدد()[ټکي() - 1];
    ثابت uint32_t new_ارزښت_Msb = v.m_ارزښت & hi_کلمه_ماسک();
    ثابت uint32_t new_ارزښت_XMsb = v.m_ارزښت_X & hi_کلمه_ماسک();
    إذا (warnOnTruncation && (new_ارزښت_Msb != v.m_ارزښت || new_ارزښت_XMsb != v.m_ارزښت_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << پلنوالی() << " bit number: " << displayed("%d"));
    }
    m_معلومات.عدد()[ټکي() - 1] = {new_ارزښت_Msb, new_ارزښت_XMsb};
}

V3_شمېره& V3_شمېره::opSel(ثابت V3_شمېره& lhs, ثابت V3_شمېره& msb, ثابت V3_شمېره& lsb) {
    عدد_ASSERT_OP_ARGS3(lhs, msb, lsb);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS2(msb, lsb);
    إذا (lsb.isFourState() || msb.isFourState()) أرجع سېټ_All_بټونه_X();
    أرجع opSel(lhs, msb.منزل_UInt(), lsb.منزل_UInt());
}

V3_شمېره& V3_شمېره::opSel(ثابت V3_شمېره& lhs, uint32_t msbval, uint32_t lsbval) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    سېټ_Zero();
    صحيح copy_پلنوالی = 0;
    إذا (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.پلنوالی())) {
        ثابت uint64_t selected_پلنوالی = uint64_t{msbval} - lsbval + 1;
        copy_پلنوالی = static_cast<صحيح>(
            std::min({static_cast<uint64_t>(پلنوالی()), static_cast<uint64_t>(lhs.پلنوالی()) - lsbval,
                      selected_پلنوالی}));
    }
    copy_بټونه(0, lhs, copy_پلنوالی ? static_cast<صحيح>(lsbval) : 0, copy_پلنوالی);
    fill_بټونه(copy_پلنوالی, پلنوالی() - copy_پلنوالی, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    أرجع *هذا;
}

V3_شمېره& V3_شمېره::opSelInto(ثابت V3_شمېره& lhs, ثابت V3_شمېره& lsb, صحيح پلنوالی) {
    أرجع opSelInto(lhs, lsb.منزل_SInt(), پلنوالی);
}

V3_شمېره& V3_شمېره::opSelInto(ثابت V3_شمېره& lhs, صحيح lsbval, صحيح پلنوالی) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحيح copy_پلنوالی = std::max(0, std::min({پلنوالی, lhs.پلنوالی(), هذا->پلنوالی() - lsbval}));
    copy_بټونه(lsbval, lhs, 0, copy_پلنوالی);
    ثابت صحيح fill_پلنوالی
        = std::max(0, std::min(پلنوالی - copy_پلنوالی, هذا->پلنوالی() - lsbval - copy_پلنوالی));
    fill_بټونه(lsbval + copy_پلنوالی, fill_پلنوالی, v3Global.constRemoveXs() ? 0 : 'x');
    أرجع *هذا;
}

//======================================================================
// Ops - Floating point

V3_شمېره& V3_شمېره::opIToRD(ثابت V3_شمېره& lhs, منطقي isSigned) {
    // Correct number of zero bits/width matters
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_شمېره noxz(lhs);
    noxz.op_ټاکل_Non_XZ(lhs);
    مزدوج d = 0;
    ثابت منطقي negate = isSigned && noxz.isNegative();
    إذا (negate) {
        ثابت V3_شمېره noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    لكل (صحيح بټ_2 = noxz.پلنوالی() - 1; بټ_2 >= 0; بټ_2--) {
        // Some precision might be lost in this add, that's what we want
        إذا (noxz.بټ_Is1(بټ_2)) d += exp2(بټ_2);
    }
    إذا (negate) d = -d;
    أرجع سېټ_Double(d);
}
V3_شمېره& V3_شمېره::opRToIS(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_TRUNC(lhs.منزل_Double());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    أرجع سېټ_Long_S(i);
}
V3_شمېره& V3_شمېره::opRToIRoundS(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_پړاو(lhs.منزل_Double());
    سېټ_Zero();
    اتحاد {
        مزدوج d;
        uint64_t q;
    } u;
    u.d = v;
    (فراغ)u.d;

    ثابت صحيح توانیز_تابع = static_cast<صحيح>((u.q >> 52ULL) & VL_ماسک_Q(11)) - 1023;
    ثابت صحيح lsb = توانیز_تابع - 52;
    ثابت uint64_t mantissa = (u.q & VL_ماسک_Q(52)) | (1ULL << 52);
    إذا (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        لكل (صحيح بټ_2 = 0; بټ_2 <= 52; ++بټ_2) {
            إذا (mantissa & (1ULL << بټ_2)) {
                ثابت صحيح outbit = بټ_2 + lsb;
                إذا (outbit >= 0) سېټ_بټ(outbit, 1);
            }
        }
        إذا (v < 0) {
            ثابت V3_شمېره noSign = *هذا;
            opNegate(noSign);
        }
    }
    أرجع *هذا;
}
V3_شمېره& V3_شمېره::op_Real_منزل_بټونه(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.پلنوالی() == 64 && پلنوالی() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.منزل_Double();
    أرجع سېټ_Quad(u.m_v);
}
V3_شمېره& V3_شمېره::op_بټونه_منزل_Real_D(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.پلنوالی() == 64 && پلنوالی() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.منزل_UQuad();
    أرجع سېټ_Double(u.m_d);
}
V3_شمېره& V3_شمېره::opNegateD(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_DOUBLE_ARGS1(lhs);
    أرجع سېټ_Double(-lhs.منزل_Double());
}
V3_شمېره& V3_شمېره::op_جمع_کړه_D(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Double(lhs.منزل_Double() + rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opSubD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Double(lhs.منزل_Double() - rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opMulD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Double(lhs.منزل_Double() * rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opDivD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Double(lhs.منزل_Double() / rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opPowD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Double(توان(lhs.منزل_Double(), rhs.منزل_Double()));
}
V3_شمېره& V3_شمېره::opEqD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_Double() == rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opNeqD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_Double() != rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opGtD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_Double() > rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opGteD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_Double() >= rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opLtD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_Double() < rhs.منزل_Double());
}
V3_شمېره& V3_شمېره::opLteD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_Double() <= rhs.منزل_Double());
}

//======================================================================
// Ops - String

V3_شمېره& V3_شمېره::opConcatN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_String(lhs.منزل_String() + rhs.منزل_String());
}
V3_شمېره& V3_شمېره::opReplN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_STRING_ARGS1(lhs);
    عدد_ASSERT_LOGIC_ARGS1(rhs);
    أرجع opReplN(lhs, rhs.منزل_UInt());
}
V3_شمېره& V3_شمېره::opReplN(ثابت V3_شمېره& lhs, uint32_t rhsval) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    string بهر;
    بهر.reserve(lhs.منزل_String().اوږدتیا() * rhsval);
    لكل (بینښه times = 0; times < rhsval; ++times) بهر += lhs.منزل_String();
    أرجع سېټ_String(بهر);
}
V3_شمېره& V3_شمېره::opCvtPackString(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    أرجع سېټ_String(lhs.منزل_String());
}
V3_شمېره& V3_شمېره::op_منزل_ښکتنی_N(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string بهر = VString::downcase(lhs.منزل_String());
    أرجع سېټ_String(بهر);
}
V3_شمېره& V3_شمېره::op_منزل_پاسنی_N(ثابت V3_شمېره& lhs) {
    عدد_ASSERT_OP_ARGS1(lhs);
    عدد_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string بهر = VString::upcase(lhs.منزل_String());
    أرجع سېټ_String(بهر);
}

V3_شمېره& V3_شمېره::opEqN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_String() == rhs.منزل_String());
}
V3_شمېره& V3_شمېره::opNeqN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_String() != rhs.منزل_String());
}
V3_شمېره& V3_شمېره::opGtN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_String() > rhs.منزل_String());
}
V3_شمېره& V3_شمېره::opGteN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_String() >= rhs.منزل_String());
}
V3_شمېره& V3_شمېره::opLtN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_String() < rhs.منزل_String());
}
V3_شمېره& V3_شمېره::opLteN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs) {
    عدد_ASSERT_OP_ARGS2(lhs, rhs);
    عدد_ASSERT_STRING_ARGS2(lhs, rhs);
    أرجع سېټ_Single_بټونه(lhs.منزل_String() <= rhs.منزل_String());
}

//======================================================================

فراغ V3_شمېره::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_کرښه* ثابت fl = جديد File_کرښه{File_کرښه::built_دننه_Filename()};
    V3_شمېره عدد{fl, 32, 0};
    عدد.selfTestThis();
}

فراغ V3_شمېره::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_شمېره::epsilonEqual(0, 0), ریښتیا);
    UASSERT_SELFTEST(V3_شمېره::epsilonEqual(1e19, 1e19), ریښتیا);
    UASSERT_SELFTEST(V3_شمېره::epsilonEqual(9, 0.0001), خطأ);
    UASSERT_SELFTEST(V3_شمېره::epsilonEqual(1, 1 + std::numeric_limits<مزدوج>::epsilon()), ریښتیا);
    UASSERT_SELFTEST(V3_شمېره::epsilonEqual(0.009, 0.00899999999999999931998839741709), ریښتیا);

    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(0), ریښتیا);
    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(1), ریښتیا);
    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(-1), ریښتیا);
    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(1.0001), خطأ);
    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(0.9999), خطأ);
    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(-1.0001), خطأ);
    UASSERT_SELFTEST(V3_شمېره::epsilonIntegral(-0.9999), خطأ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
