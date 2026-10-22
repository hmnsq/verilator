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

constexpr صحيح MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define رقم_ASSERT_OP_ARGS1(arg1) \
    UASSERT((هذا != &(arg1)), "Number operation called with same source and dest")
#define رقم_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2)), \
            "Number operation called with same source and dest")
#define رقم_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3)), \
            "Number operation called with same source and dest")
#define رقم_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3) && هذا != &(arg4)), \
            "Number operation called with same source and dest")

#define رقم_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_العدد_Data::V3_العدد_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define رقم_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    رقم_ASSERT_LOGIC_ARGS1(arg1); \
    رقم_ASSERT_LOGIC_ARGS1(arg2)

#define رقم_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    رقم_ASSERT_LOGIC_ARGS1(arg1); \
    رقم_ASSERT_LOGIC_ARGS1(arg2); \
    رقم_ASSERT_LOGIC_ARGS1(arg3); \
    رقم_ASSERT_LOGIC_ARGS1(arg4)

#define رقم_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define رقم_ASSERT_STRING_ARGS2(arg1, arg2) \
    رقم_ASSERT_STRING_ARGS1(arg1); \
    رقم_ASSERT_STRING_ARGS1(arg2)

#define رقم_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define رقم_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    رقم_ASSERT_DOUBLE_ARGS1(arg1); \
    رقم_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

فراغ V3_العدد::v3errorEnd(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    إذا (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } وإلا إذا (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } وإلا {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

فراغ V3_العدد::v3errorEndFatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_العدد::V3_العدد(Ast_عقدة* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    إذا (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } وإلا {
        init(nodep, std::max<صحيح>(str.length() * 8, 1));
        لكل (unsigned pos = 0; pos < str.length(); ++pos) {
            ثابت صحيح topos = str.length() - 1 - pos;
            قيمة_And_X& v = m_data.رقم()[topos / 4];
            لكل (صحيح bit = 0; bit < 8; ++bit) {
                إذا (str[pos] & (1UL << bit)) v.m_قيمة |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = true;
    opCleanThis(true);
}

V3_العدد::V3_العدد(Ast_عقدة* nodep, ثابت Ast_عقدة_DType* nodedtypep) {
    إذا (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } وإلا إذا (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } وإلا إذا (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } وإلا {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

فراغ V3_العدد::create(ثابت حرف* sourcep) {
    m_data.setLogic();
    ثابت حرف* قيمة_startp = sourcep;
    لكل (ثابت حرف* cp = sourcep; *cp; ++cp) {
        إذا (*cp == '\'') {
            قيمة_startp = cp + 1;
            اكسر;
        }
    }

    منطقي userSized = خطأ;
    منطقي unbased = خطأ;
    حرف base = '\0';
    إذا (قيمة_startp != sourcep) {  // Has a '
        string widthn;
        ثابت حرف* cp = sourcep;
        لكل (; *cp; ++cp) {
            إذا (*cp == '\'') {
                ++cp;
                اكسر;
            }
            إذا (*cp != '_') widthn += *cp;
        }
        بينما (*cp == '_') cp++;
        إذا (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(true);
        }
        إذا (*cp) {
            base = *cp;
            ++cp;
        }
        قيمة_startp = cp;

        userSized = widthn.length() != 0;
        إذا (std::atoi(widthn.c_str())) {
            إذا (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_رقم_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_رقم_Width(), true);
            } وإلا {
                width(std::atoi(widthn.c_str()), true);
            }
        }
    } وإلا {
        unbased = true;
        base = 'd';
    }

    لكل (صحيح i = 0; i < كلمات(); ++i) m_data.رقم()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    إذا (base == '0') {
        width(1, خطأ);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = true;
    } وإلا إذا (base == '1') {
        width(1, خطأ);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = true;
    } وإلا إذا (std::tolower(base) == 'z') {
        width(1, خطأ);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = true;
    } وإلا إذا (std::tolower(base) == 'x') {
        width(1, خطأ);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = true;
    }
    // Otherwise...
    وإلا إذا (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحيح widthBound = std::max<صحيح>(32, std::strlen(قيمة_startp) * 4);
        width(widthBound, خطأ);  // Will change width below
        إذا (unbased) isSigned(true);  // Also says the spec.
    }

    // Ignore leading blanks
    بينما (*قيمة_startp == '_' || std::isspace(*قيمة_startp)) ++قيمة_startp;
    إذا (!*قيمة_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    إذا (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحيح obit = 0;  // Start at LSB
    صحيح base_align = 1;
    إذا (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        بينما (*قيمة_startp == '_' || *قيمة_startp == '0') ++قيمة_startp;
        // Convert decimal number to hex
        صحيح olen = 0;
        uint32_t val = 0;
        صحيح got_x = 0;
        صحيح got_z = 0;
        صحيح got_01 = 0;
        منطقي warnedTooMany = خطأ;
        لكل (ثابت حرف* cp = قيمة_startp; *cp; ++cp) {
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
                    m_data.رقم()[0].m_قيمة = val;
                } وإلا {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_العدد product(هذا, width() + 4);  // +4 for overflow detection
                    ثابت V3_العدد ten(هذا, width() + 4, 10);
                    ثابت V3_العدد addend(هذا, width(), (*cp - '0'));
                    product.opMul(*هذا, ten);
                    opAdd(product, addend);
                    إذا (!warnedTooMany && product.bits_قيمة(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = true;
                    }
                }
                ++olen;
                got_01 = 1;
                اكسر;
            }
            حالة 'z':
            حالة '?': {
                got_z = 1;
                إذا (!userSized) width(32, خطأ);
                setAllBitsZ();
                اكسر;
            }
            حالة 'x': {
                got_x = 1;
                إذا (!userSized) width(32, خطأ);
                setAllBitsX();
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
        obit = width();
        إذا ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << قيمة_startp);
        }
    } وإلا {
        // Convert bin/octal number to hex
        لكل (ثابت حرف* cp = قيمة_startp + std::strlen(قيمة_startp) - 1; cp >= قيمة_startp;
             cp--) {
            إذا (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                اكسر;
            }
            بدل (std::tolower(base)) {
            حالة 'b': {
                base_align = 1;
                بدل (std::tolower(*cp)) {
                حالة '0': setBit(obit++, 0); اكسر;
                حالة '1': setBit(obit++, 1); اكسر;
                حالة 'z':
                حالة '?': setBit(obit++, 'z'); اكسر;
                حالة 'x': setBit(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                افتراضي: v3error("Illegal character in binary constant: " << *cp);
                }
                اكسر;
            }

            حالة 'o':
            حالة 'c': {
                base_align = 3;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  اكسر;
                حالة '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  اكسر;
                حالة '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  اكسر;
                حالة '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  اكسر;
                حالة '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  اكسر;
                حالة '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  اكسر;
                حالة '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  اكسر;
                حالة '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  اكسر;
                حالة 'z':
                حالة '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); اكسر;
                حالة 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); اكسر;
                حالة '_': اكسر;
                // clang-format on
                افتراضي: v3error("Illegal character in octal constant");
                }
                اكسر;
            }

            حالة 'h':  // FALLTHRU
            حالة 'x': {
                base_align = 4;
                بدل (std::tolower(*cp)) {  // clang-format off
                حالة '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); اكسر;
                حالة '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); اكسر;
                حالة '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); اكسر;
                حالة '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); اكسر;
                حالة '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); اكسر;
                حالة '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); اكسر;
                حالة '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); اكسر;
                حالة '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); اكسر;
                حالة '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); اكسر;
                حالة '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); اكسر;
                حالة 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); اكسر;
                حالة 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); اكسر;
                حالة 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); اكسر;
                حالة 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); اكسر;
                حالة 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); اكسر;
                حالة 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); اكسر;
                حالة 'z':
                حالة '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); اكسر;
                حالة 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); اكسر;
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
                v3error("Illegal base character: " << base);
                اكسر;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    إذا (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), خطأ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    بينما (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(true);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

فراغ V3_العدد::warnTooMany(ثابت string& قيمة) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << قيمة << "'");
}

فراغ V3_العدد::nodep(Ast_عقدة* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    إذا (!nodep) إرجاع;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

منطقي V3_العدد::epsilonEqual(مزدوج a, مزدوج b) {
    إرجاع std::fabs(a - b)
           <= (std::numeric_limits<مزدوج>::epsilon() * std::max(1.0, std::max(a, b)));
}

منطقي V3_العدد::epsilonIntegral(مزدوج a) {
    ثابت مزدوج dint = static_cast<مزدوج>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    إرجاع epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

صحيح V3_العدد::log2b(uint32_t رقم) {
    // See also opCLog2
    لكل (صحيح bit = 31; bit > 0; bit--) {
        إذا (رقم & (1ULL << bit)) إرجاع bit;
    }
    إرجاع 0;
}

صحيح V3_العدد::log2bQuad(uint64_t رقم) {
    إذا (رقم >> 32ULL) إرجاع 32 + log2b(رقم >> 32ULL);
    إرجاع log2b(رقم);
}

//======================================================================
// Setters

V3_العدد& V3_العدد::setZero() {
    لكل (صحيح i = 0; i < كلمات(); ++i) m_data.رقم()[i] = {0, 0};
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setQuad(uint64_t قيمة) {
    setZero();
    m_data.رقم()[0].m_قيمة = قيمة & 0xffffffffULL;
    إذا (width() > 32) m_data.رقم()[1].m_قيمة = (قيمة >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setLong(uint32_t قيمة) {
    setZero();
    m_data.رقم()[0].m_قيمة = قيمة;
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setLongS(int32_t قيمة) {
    setZero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = قيمة;
    (فراغ)u.s;
    m_data.رقم()[0].m_قيمة = u.u;
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setDouble(مزدوج قيمة) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.d = قيمة;
    (فراغ)u.d;
    لكل (صحيح i = 2; i < كلمات(); ++i) m_data.رقم()[i] = {0, 0};
    m_data.رقم()[0].m_قيمة = u.u[0];
    m_data.رقم()[1].m_قيمة = u.u[1];
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setSingleBits(حرف قيمة) {
    لكل (صحيح i = 1 /*upper*/; i < كلمات(); ++i) m_data.رقم()[i] = {0, 0};
    m_data.رقم()[0] = {(قيمة == '1' || قيمة == 'x' || قيمة == 1 || قيمة == 3),
                       (قيمة == 'z' || قيمة == 'x' || قيمة == 2 || قيمة == 3)};
    إرجاع *هذا;
}

V3_العدد& V3_العدد::setAllBits0() {
    setZero();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setAllBits1() {
    لكل (صحيح i = 0; i < كلمات(); ++i) m_data.رقم()[i] = {~0U, 0};
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    لكل (صحيح i = 0; i < كلمات(); ++i) m_data.رقم()[i] = {~0U, ~0U};
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setAllBitsZ() {
    لكل (صحيح i = 0; i < كلمات(); ++i) m_data.رقم()[i] = {0, ~0U};
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::setAllBitsXRemoved() {
    إذا (!v3Global.constRemoveXs()) {
        إرجاع setAllBitsX();
    } وإلا {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        إذا (v3Global.opt.xAssign() == "1") {
            إرجاع setAllBits1();
        } وإلا {
            إرجاع setAllBits0();
        }
    }
}
V3_العدد& V3_العدد::setValue1() {
    m_data.رقم()[0] = {1, 0};
    لكل (صحيح i = 1; i < كلمات(); ++i) m_data.رقم()[i] = {0, 0};
    إرجاع *هذا;
}

فراغ V3_العدد::setBitX0(صحيح bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_العدد& V3_العدد::setMask(صحيح nbits, صحيح lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    إرجاع *هذا;
}

//======================================================================
// ACCESSORS - as strings

string V3_العدد::ascii(منطقي prefixed, منطقي cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (is1Step()) إرجاع "1step";
    إذا (isNull()) إرجاع "null";
    std::ostringstream out;
    إذا (isDouble()) {
        out.precision(17);
        إذا (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } وإلا {
            out << toDouble();
            إذا (toDouble() == floor(toDouble())) out << ".0";
        }
        إرجاع out.str();
    } وإلا إذا (isString()) {
        إرجاع '"' + toString() + '"';
    } وإلا {
        إذا (VL_UNCOVERABLE((m_data.رقم()[كلمات() - 1].m_قيمة | m_data.رقم()[كلمات() - 1].m_قيمة_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    إذا (prefixed) {
        إذا (sized()) {
            out << width() << "'";
        } وإلا إذا (autoExtend() && width() == 1) {
            out << "'";
            إذا (bitIs0(0)) {
                out << '0';
            } وإلا إذا (bitIs1(0)) {
                out << '1';
            } وإلا إذا (bitIsZ(0)) {
                out << 'z';
            } وإلا {
                out << 'x';
            }
            إرجاع out.str();
        } وإلا {
            إذا (cleanVerilog) {
                out << "'";
            } وإلا {
                out << "?" << width() << "?";
            }
        }
        إذا (isSigned()) out << "s";
    }

    ثابت منطقي ثنائي = (isFourState()
#ifdef V3_العدد_ASCII_ثنائي
                         // cppcheck-suppress konwnConditionTrueFalse
                         || true
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    إذا (ثنائي) {
        out << "b";
        out << displayed("%0b");
    } وإلا {
        إذا (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    إذا (isNull()) {
        إذا (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } وإلا {
            out << " [null]";
        }
    }
    إرجاع out.str();
}

منطقي V3_العدد::displayedFmtHasArg(حرف format, منطقي isScan) {
    // Is this a format letter that takes an argument in runtime?
    (فراغ)isScan;
    بدل (std::tolower(format)) {
    حالة 'b': إرجاع true;  //ثنائي
    حالة 'c': إرجاع true;  // Character
    حالة 'd': إرجاع true;  // Decimal
    حالة 'e': إرجاع true;  // Floating
    حالة 'f': إرجاع true;  // Floating
    حالة 'g': إرجاع true;  // Floating
    حالة 'h': إرجاع true;  // Hex
    حالة 'o': إرجاع true;  // Octal
    حالة 'p': إرجاع true;  // Pattern
    حالة 's': إرجاع true;  // String
    حالة 't': إرجاع true;  // Time
    حالة 'u': إرجاع true;  // Packed 2-state
    حالة 'v': إرجاع true;  // Strength
    حالة 'x': إرجاع true;  // Hex
    حالة 'z': إرجاع true;  // Packed 4-state
    حالة '?': إرجاع true;  // Internal: Compute format in V3Width based on data type
    افتراضي: إرجاع خطأ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_العدد::displayPad(size_t fmtsize, حرف pad, منطقي يسار, ثابت string& in) VL_PURE {
    string padding;
    إذا (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    إرجاع يسار ? (in + padding) : (padding + in);
}

string V3_العدد::displayedEnumName(ثابت AstEnumItem* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    إرجاع VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_العدد::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3_العدد قيمة{هذا, dtypep->width()};
    قيمة.opAssign(*هذا);
    لكل (ثابت AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        إذا (!constp->رقم().isAnyXZ() && قيمة.isCaseEq(constp->رقم()))
            إرجاع displayedEnumName(itemp);
    }
    إرجاع "";
}

string V3_العدد::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_العدد قيمة{هذا, dtypep->width()};
    قيمة.opAssign(*هذا);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.back());
    إذا (fmt != 'p' && fmt != 's') إرجاع قيمة.displayed(argp, vformat, numericAttr);
    string name = قيمة.displayedEnumName(dtypep);
    إذا (name.empty()) name = قيمة.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_العدد name_رقم{هذا};
    name_رقم.setString(name);
    إرجاع name_رقم.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_العدد::displayed(ثابت Ast_عقدة* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    إرجاع displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_العدد::displayed(File_سطر* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *هذا);
    ++pos;
    منطقي يسار = خطأ;
    إذا (pos[0] == '-') {
        يسار = true;
        ++pos;
    }
    string fmtsize;
    لكل (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
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
            V3_العدد strwide{fl, static_cast<صحيح>(toString().length()) * 8, 0};
            strwide.opNToI(*هذا);
            إرجاع strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
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
            V3_العدد nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*هذا);
            إرجاع nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    بدل (fmt) {
    حالة 'c': {
        إذا (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت unsigned صحيح v = bits_قيمة(0, 8);
        str.ادفع_back(static_cast<حرف>(v));
        إرجاع str;
    }
    حالة 'e':  // FALLTHRU
    حالة 'f':  // FALLTHRU
    حالة 'g': {
        ثابت مزدوج n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        إرجاع tmp;
    }
    حالة 's': {
        إذا (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } وإلا {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحيح bit = width() - 1;
            منطقي start = true;
            بينما ((bit % 8) != 7) ++bit;
            لكل (; bit >= 0; bit -= 8) {
                ثابت صحيح v = bits_قيمة(bit - 7, 8);
                إذا (!start || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    start = خطأ;  // Drop leading 0s
                } وإلا {
                    إذا (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', يسار, str);
        إرجاع str;
    }
    حالة 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        إذا (formatAttr.isDouble()) {
            ثابت مزدوج n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            إرجاع tmp;
        }
        إذا (formatAttr.isString())
            إرجاع '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        إذا (formatAttr.isComplex()) إرجاع toString();
        إرجاع "%p";
    }
    حالة 'b':  // FALLTHRU
    حالة 'o':  // FALLTHRU
    حالة 'h':  // FALLTHRU
    حالة 'x': {
        صحيح bit = width() - 1;
        إذا (يسار || !fmtsize.empty()) {
            بينما (bit && bitIs0(bit)) --bit;
        }
        بدل (fmt) {
        حالة 'b': {
            لكل (; bit >= 0; --bit) {
                إذا (bitIs0(bit)) {
                    str += '0';
                } وإلا إذا (bitIs1(bit)) {
                    str += '1';
                } وإلا إذا (bitIsZ(bit)) {
                    str += 'z';
                } وإلا {
                    str += 'x';
                }
            }
            اكسر;
        }
        حالة 'o': {
            بينما ((bit % 3) != 2) ++bit;
            لكل (; bit > 0; bit -= 3) {
                ثابت صحيح رقم_X = تعداد_X(bit - 2, 3);
                ثابت صحيح رقم_Z = تعداد_Z(bit - 2, 3);
                إذا (رقم_X == 3 || رقم_X == width() - (bit - 2)) {
                    str += 'x';
                    استمر;
                }
                إذا (رقم_Z == 3 || رقم_Z == width() - (bit - 2)) {
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
                ثابت صحيح v = bits_قيمة(bit - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            اكسر;
        }
        افتراضي: {  // h/x
            بينما ((bit % 4) != 3) ++bit;
            لكل (; bit > 0; bit -= 4) {
                ثابت صحيح رقم_X = تعداد_X(bit - 3, 4);
                ثابت صحيح رقم_Z = تعداد_Z(bit - 3, 4);
                إذا (رقم_X == 4 || رقم_X == width() - (bit - 3)) {
                    str += 'x';
                    استمر;
                }
                إذا (رقم_Z == 4 || رقم_Z == width() - (bit - 3)) {
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
                ثابت صحيح v = bits_قيمة(bit - 3, 4);
                إذا (v >= 10) {
                    str += static_cast<حرف>('a' + v - 10);
                } وإلا {
                    str += static_cast<حرف>('0' + v);
                }
            }
            اكسر;
        }
        }  // switch
        ثابت size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (يسار ? ' ' : '0'), يسار, str);
        إرجاع str;
    }  // case b/d/x/o
    حالة 't':  // Time
    حالة 'd': {  // Unsigned decimal
        ثابت منطقي issigned = formatAttr.isSigned();
        إذا (fmtsize == "" && !يسار) {
            ثابت مزدوج mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            مزدوج dchars = mantissabits / 3.321928094887362 + 1.0;
            إذا (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<صحيح>(dchars));
        }
        منطقي hasXZ = خطأ;
        إذا (isAllX()) {
            str = "x";
            hasXZ = true;
        } وإلا إذا (isAllZ()) {
            str = "z";
            hasXZ = true;
        } وإلا إذا (isAnyX()) {
            str = "X";
            hasXZ = true;
        } وإلا إذا (isAnyZ()) {
            str = "Z";
            hasXZ = true;
        }
        إذا (!hasXZ) {
            إذا (issigned) {
                إذا (width() > 64) {
                    str = toDecimalS();
                } وإلا {
                    str = cvtToStr(toSQuad());
                }
            } وإلا {
                إذا (width() > 64) {
                    str = toDecimalU();
                } وإلا {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        ثابت منطقي zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !يسار;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), يسار, str);
        إرجاع str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالة 'u': {  // Packed 2-state
        لكل (صحيح i = 0; i < كلمات(); ++i) {
            ثابت uint32_t v = m_data.رقم()[i].m_قيمة;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        إرجاع str;
    }
    حالة 'z': {  // Packed 4-state
        لكل (صحيح i = 0; i < كلمات(); ++i) {
            ثابت قيمة_And_X v = m_data.رقم()[i];
            str += static_cast<حرف>((v.m_قيمة >> 0) & 0xff);
            str += static_cast<حرف>((v.m_قيمة >> 8) & 0xff);
            str += static_cast<حرف>((v.m_قيمة >> 16) & 0xff);
            str += static_cast<حرف>((v.m_قيمة >> 24) & 0xff);
            str += static_cast<حرف>((v.m_قيمة_X >> 0) & 0xff);
            str += static_cast<حرف>((v.m_قيمة_X >> 8) & 0xff);
            str += static_cast<حرف>((v.m_قيمة_X >> 16) & 0xff);
            str += static_cast<حرف>((v.m_قيمة_X >> 24) & 0xff);
        }
        إرجاع str;
    }
    حالة 'v': {  // Strength
        صحيح bit = width() - 1;
        لكل (; bit >= 0; bit--) {
            إذا (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            وإلا إذا (bitIs1(bit)) {
                str += "St1 ";
            } وإلا إذا (bitIsZ(bit)) {
                str += "StZ ";
            } وإلا {
                str += "StX";
            }
        }
        إرجاع str;
    }
    افتراضي: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_العدد::emitC() ثابت VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    حرف sbuf[bufsize];
    string result;
    إذا (isNull()) {
        إرجاع "VlNull{}";
    } وإلا إذا (isDouble()) {
        ثابت مزدوج dnum = toDouble();
        إذا (std::isinf(dnum)) {
            إذا (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } وإلا إذا (std::isnan(dnum)) {
            إذا (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } وإلا {
            ثابت حرف* ثابت fmt = (static_cast<صحيح>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            إرجاع sbuf;
        }
    } وإلا إذا (isString()) {
        ثابت string strg = toString();
        ثابت string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        إرجاع '"' + quoted + "\"s";
    } وإلا إذا (كلمات() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(كلمات()) + ">{{";
        إذا (كلمات() > 4) result += '\n';
        لكل (صحيح n = 0; n < كلمات(); ++n) {
            إذا (n) result += ((n % 4) ? ", " : ",\n");
            (فراغ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        إذا (كلمات() > 4) result += '\n';
        result += "}}";
    } وإلا إذا (كلمات() == 2) {  // Quad
        ثابت uint64_t qnum = static_cast<uint64_t>(toUQuad());
        ثابت حرف* ثابت fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        إرجاع sbuf;
    } وإلا {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ثابت uint32_t unum = toUInt();
        ثابت حرف* ثابت fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        إرجاع sbuf;
    }
    إرجاع result;
}

string V3_العدد::toDecimalS() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    إذا (isNegative()) {
        V3_العدد lhsNoSign = *هذا;
        lhsNoSign.opNegate(*هذا);
        إرجاع "-"s + lhsNoSign.toDecimalU();
    } وإلا {
        إرجاع toDecimalU();
    }
}

string V3_العدد::toDecimalU() ثابت VL_MT_STABLE {
    ثابت صحيح maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_العدد bcd(هذا, maxdecwidth + 4);
    V3_العدد tmp(هذا, maxdecwidth + 4);
    V3_العدد tmp2(هذا, maxdecwidth + 4);

    صحيح from_bit = width() - 1;
    // Skip all leading zeros
    لكل (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    لكل (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        لكل (صحيح nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            إذا (bcd.bits_قيمة(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_العدد(هذا, 32, 1));
        // bcd[0] = this[from_bit]
        إذا (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    صحيح lsb = (maxdecwidth - 1) & ~3;
    لكل (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        إذا (bcd.bits_قيمة(lsb, 4)) اكسر;
    }
    لكل (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.bits_قيمة(lsb, 4));  // 0..9
    }
    إرجاع output;
}

string V3_العدد::toRealString() ثابت VL_MT_STABLE {
    ثابت مزدوج قيمة = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << قيمة;
    string result = out.str();
    إذا (std::strtod(result.c_str(), nullptr) != قيمة) {
        out.str("");
        out.precision(17);
        out << قيمة;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    إذا (result.find_first_of(".en") == string::npos) result += ".0";
    إرجاع result;
}

//======================================================================
// ACCESSORS - as numbers

منطقي V3_العدد::fitsInUInt() ثابت VL_MT_SAFE {
    إذا (isFourState()) إرجاع خطأ;
    // We allow wide numbers that represent values <= 32 bits
    لكل (صحيح i = 1; i < كلمات(); ++i) {
        إذا (m_data.رقم()[i].m_قيمة) إرجاع خطأ;
    }
    إرجاع true;
}
uint32_t V3_العدد::toUInt() ثابت VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 32 bits
    إذا (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *هذا);
    }
    إرجاع m_data.رقم()[0].m_قيمة;
}

مزدوج V3_العدد::toDouble() ثابت VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.رقم()[0].m_قيمة;
    u.u[1] = m_data.رقم()[1].m_قيمة;
    إرجاع u.d;
}

int32_t V3_العدد::toSInt() ثابت VL_MT_SAFE {
    إذا (isSigned()) {
        ثابت uint32_t v = toUInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        ثابت uint32_t extended = v | signExtend;
        إرجاع static_cast<int32_t>(extended);
    } وإلا {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        إرجاع static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_العدد::toUQuad() ثابت VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 64 bits
    إذا (isDouble()) إرجاع static_cast<uint64_t>(toDouble());
    لكل (صحيح i = 2; i < كلمات(); ++i) {
        إذا (m_data.رقم()[i].m_قيمة) {
            v3error("Value too wide for 64-bits expected in this context " << *هذا);
            اكسر;
        }
    }
    إذا (width() <= 32) إرجاع (static_cast<uint64_t>(toUInt()));
    إرجاع ((static_cast<uint64_t>(m_data.رقم()[1].m_قيمة) << 32ULL)
            | (static_cast<uint64_t>(m_data.رقم()[0].m_قيمة)));
}

int64_t V3_العدد::toSQuad() ثابت {
    إذا (isDouble()) إرجاع static_cast<int64_t>(toDouble());
    ثابت uint64_t v = toUQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    ثابت uint64_t extended = v | signExtend;
    إرجاع static_cast<int64_t>(extended);
}

string V3_العدد::toString() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *هذا);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    إذا (isString()) إرجاع m_data.str();
    صحيح bit = width() - 1;
    بينما ((bit % 8) != 7) ++bit;
    std::string str;
    لكل (; bit >= 0; bit -= 8) {
        ثابت صحيح v = bits_قيمة(bit - 7, 8);
        إذا (v) str += static_cast<حرف>(v);
    }
    إرجاع str;
}

uint32_t V3_العدد::edataWord(صحيح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *هذا);
    إرجاع m_data.رقم()[eword].m_قيمة;
}

uint8_t V3_العدد::dataByte(صحيح byte) ثابت {
    إرجاع (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقي V3_العدد::isAllZ() ثابت VL_MT_SAFE {
    إذا (isDouble() || isString()) إرجاع خطأ;
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        ثابت uint32_t mask = i == كلمات() - 1 ? hiWordMask() : ~0U;
        إذا (((~v.m_قيمة & v.m_قيمة_X) & mask) != mask) إرجاع خطأ;
    }
    إرجاع true;
}
منطقي V3_العدد::isAllX() ثابت VL_MT_SAFE {
    إذا (isDouble() || isString()) إرجاع خطأ;
    uint32_t mask = hiWordMask();
    لكل (صحيح i = كلمات() - 1; i >= 0; --i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        إذا ((v.m_قيمة & v.m_قيمة_X) ^ mask) إرجاع خطأ;
        mask = ~0U;
    }
    إرجاع true;
}
منطقي V3_العدد::isEqZero() ثابت VL_MT_SAFE {
    إذا (isString()) إرجاع m_data.str().empty();
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        إذا (v.m_قيمة || v.m_قيمة_X) إرجاع خطأ;
    }
    إرجاع true;
}
منطقي V3_العدد::isNeqZero() ثابت {
    إذا (isString()) إرجاع !m_data.str().empty();
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        إذا (v.m_قيمة & ~v.m_قيمة_X) إرجاع true;
    }
    إرجاع خطأ;
}
منطقي V3_العدد::isBitsZero(صحيح msb, صحيح lsb) ثابت {
    لكل (صحيح i = lsb; i <= msb; ++i) {
        إذا (VL_UNLIKELY(!bitIs0(i))) إرجاع خطأ;
    }
    إرجاع true;
}
منطقي V3_العدد::isEqOne() ثابت {
    إذا (m_data.رقم()[0].m_قيمة != 1 || m_data.رقم()[0].m_قيمة_X) إرجاع خطأ;
    لكل (صحيح i = 1; i < كلمات(); ++i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        إذا (v.m_قيمة || v.m_قيمة_X) إرجاع خطأ;
    }
    إرجاع true;
}
منطقي V3_العدد::isEqAllOnes(صحيح optwidth) ثابت {
    // Correct number of zero bits/width matters
    إذا (!optwidth) optwidth = width();
    لكل (صحيح bit = 0; bit < optwidth; ++bit) {
        إذا (!bitIs1(bit)) إرجاع خطأ;
    }
    إرجاع true;
}
منطقي V3_العدد::isFourState() ثابت VL_MT_SAFE {
    إذا (isDouble() || isString()) إرجاع خطأ;
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        إذا (m_data.رقم()[i].m_قيمة_X) إرجاع true;
    }
    إرجاع خطأ;
}
منطقي V3_العدد::isAnyX() ثابت VL_MT_SAFE {
    إذا (isDouble() || isString()) إرجاع خطأ;
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        إذا (v.m_قيمة & v.m_قيمة_X) إرجاع true;
    }
    إرجاع خطأ;
}
منطقي V3_العدد::isAnyXZ() ثابت {
    إذا (isDouble() || isString()) إرجاع خطأ;
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        إذا (m_data.رقم()[i].m_قيمة_X) إرجاع true;
    }
    إرجاع خطأ;
}
منطقي V3_العدد::isAnyZ() ثابت VL_MT_SAFE {
    إذا (isDouble() || isString()) إرجاع خطأ;
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        ثابت قيمة_And_X v = m_data.رقم()[i];
        إذا (~v.m_قيمة & v.m_قيمة_X) إرجاع true;
    }
    إرجاع خطأ;
}
منطقي V3_العدد::isLtXZ(ثابت V3_العدد& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    إذا (isString()) إرجاع toString() < rhs.toString();
    إذا (isDouble()) إرجاع toDouble() < rhs.toDouble();
    لكل (صحيح bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        إذا (bitIs1(bit) && rhs.bitIs0(bit)) إرجاع true;
        إذا (rhs.bitIs1(bit) && bitIs0(bit)) إرجاع خطأ;
        إذا (bitIsXZ(bit)) إرجاع true;
        إذا (rhs.bitIsXZ(bit)) إرجاع خطأ;
    }
    إرجاع خطأ;
}
صحيح V3_العدد::تعداد_X(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
    صحيح تعداد_2 = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= width()) إرجاع تعداد_2;
        إذا (bitIsX(lsb + bitn)) ++تعداد_2;
    }
    إرجاع تعداد_2;
}
صحيح V3_العدد::تعداد_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
    صحيح تعداد_2 = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        إذا (lsb + bitn >= width()) إرجاع تعداد_2;
        إذا (bitIsZ(lsb + bitn)) ++تعداد_2;
    }
    إرجاع تعداد_2;
}

صحيح V3_العدد::widthToFit() ثابت {
    لكل (صحيح bit = width() - 1; bit > 0; bit--) {
        إذا (!bitIs0(bit)) إرجاع bit + 1;
    }
    إرجاع 1;  // one bit even if number is == 0
}

uint32_t V3_العدد::تعداد_Bits(ثابت V3_العدد& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = 0;
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        بدل (ctrl.bitIs(0)) {
        حالة '0':
            إذا (bitIs0(bit)) ++n;
            اكسر;
        حالة '1':
            إذا (bitIs1(bit)) ++n;
            اكسر;
        حالة 'x':
            إذا (bitIsX(bit)) ++n;
            اكسر;
        حالة 'z':
            إذا (bitIsZ(bit)) ++n;
            اكسر;
        }
    }
    إرجاع n;
}

uint32_t V3_العدد::تعداد_Bits(ثابت V3_العدد& ctrl1, ثابت V3_العدد& ctrl2,
                             ثابت V3_العدد& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = تعداد_Bits(ctrl1);
    إذا (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += تعداد_Bits(ctrl2);
    إذا ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += تعداد_Bits(ctrl3);
    }
    إرجاع n;
}

uint32_t V3_العدد::تعداد_Ones() ثابت {
    صحيح n = 0;
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (bitIs1(bit)) ++n;
    }
    إرجاع n;
}

uint32_t V3_العدد::mostSetBitP1() ثابت {
    لكل (صحيح bit = width() - 1; bit >= 0; bit--) {
        إذا (!bitIs0(bit)) إرجاع bit + 1;
    }
    إرجاع 0;
}

uint32_t V3_العدد::leastSetBitP1() ثابت {
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (!bitIs0(bit)) إرجاع bit + 1;
    }
    إرجاع 0;
}

//======================================================================

V3_العدد& V3_العدد::opBitsNonXZ(ثابت V3_العدد& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opBitsOne(ثابت V3_العدد& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opBitsXZ(ثابت V3_العدد& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opBitsZ(ثابت V3_العدد& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    إرجاع *هذا;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_العدد& V3_العدد::opRedOr(ثابت V3_العدد& lhs) {
    // op i, 1 bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        إذا (lhs.bitIs1(bit)) {
            إرجاع setSingleBits(1);
        } وإلا إذا (lhs.bitIs0(bit)) {
        } وإلا {
            outc = 'x';
        }
    }
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opRedAnd(ثابت V3_العدد& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        إذا (lhs.bitIs0(bit)) {
            إرجاع setSingleBits(0);
        } وإلا إذا (lhs.bitIs1(bit)) {
        } وإلا {
            outc = 'x';
        }
    }
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opRedXor(ثابت V3_العدد& lhs) {
    // op i, 1 bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        إذا (lhs.bitIs1(bit)) {
            إذا (outc == 1) {
                outc = 0;
            } وإلا إذا (outc == 0) {
                outc = 1;
            }
        } وإلا إذا (lhs.bitIs0(bit)) {
        } وإلا {
            outc = 'x';
        }
    }
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::op_تعداد_Bits(ثابت V3_العدد& تعبير, ثابت V3_العدد& ctrl1, ثابت V3_العدد& ctrl2,
                                ثابت V3_العدد& ctrl3) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS4(تعبير, ctrl1, ctrl2, ctrl3);
    رقم_ASSERT_LOGIC_ARGS4(تعبير, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.رقم()[0].m_قيمة = تعبير.تعداد_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::op_تعداد_Ones(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) إرجاع setAllBitsX();
    setZero();
    m_data.رقم()[0].m_قيمة = lhs.تعداد_Ones();
    opCleanThis();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opIsUnknown(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    إرجاع setSingleBits(lhs.isAnyXZ());
}
V3_العدد& V3_العدد::opOneHot(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) إرجاع setAllBitsX();
    إرجاع setSingleBits(lhs.تعداد_Ones() == 1);
}
V3_العدد& V3_العدد::opOneHot0(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    إذا (lhs.isFourState()) إرجاع setAllBitsX();
    إرجاع setSingleBits(lhs.تعداد_Ones() <= 1);
}
V3_العدد& V3_العدد::opCLog2(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) إرجاع setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحيح adjust = (lhs.تعداد_Ones() == 1) ? 0 : 1;
    لكل (صحيح bit = lhs.width() - 1; bit >= 0; bit--) {
        إذا (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            إرجاع *هذا;
        }
    }
    setZero();
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opMostSetBitP1(ثابت V3_العدد& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) إرجاع setAllBitsX();
    لكل (صحيح bit = lhs.width() - 1; bit >= 0; bit--) {
        إذا (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            إرجاع *هذا;
        }
    }
    setZero();
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opLogNot(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        إذا (lhs.bitIs1(bit)) {
            outc = 0;
            اذهب last;
        } وإلا إذا (lhs.bitIs0(bit)) {
        } وإلا {
            outc = 'x';
        }
    }
last:
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opNot(ثابت V3_العدد& lhs) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } وإلا إذا (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opAnd(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } وإلا إذا (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } وإلا {
            setBit(bit, 'x');
        }
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opOr(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } وإلا إذا (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } وإلا {
            setBit(bit, 'x');
        }
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opXor(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } وإلا إذا (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } وإلا إذا (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    إرجاع *هذا;
}

فراغ V3_العدد::fillBits(صحيح destLsb, صحيح width, حرف قيمة) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    إذا (!width) إرجاع;
    UASSERT(width <= هذا->width() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t قيمة_Word = قيمة == '1' || قيمة == 'x' || قيمة == 1 || قيمة == 3 ? ~0U : 0;
    ثابت uint32_t xWord = قيمة == 'z' || قيمة == 'x' || قيمة == 2 || قيمة == 3 ? ~0U : 0;
    ثابت صحيح firstWord = VL_BITWORD_E(destLsb);
    ثابت صحيح lastWord = VL_BITWORD_E(destLsb + width - 1);
    لكل (صحيح word = firstWord; word <= lastWord; ++word) {
        ثابت صحيح wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحيح wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        ثابت uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        قيمة_And_X& dest = m_data.رقم()[word];
        dest.m_قيمة = (dest.m_قيمة & ~mask) | (قيمة_Word & mask);
        dest.m_قيمة_X = (dest.m_قيمة_X & ~mask) | (xWord & mask);
    }
}

فراغ V3_العدد::copyBits(صحيح destLsb, ثابت V3_العدد& source, صحيح sourceLsb, صحيح width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    إذا (!width) إرجاع;
    UASSERT(width <= هذا->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    ثابت صحيح firstWord = VL_BITWORD_E(destLsb);
    ثابت صحيح lastWord = VL_BITWORD_E(destLsb + width - 1);
    لكل (صحيح destWord = firstWord; destWord <= lastWord; ++destWord) {
        ثابت صحيح destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        ثابت صحيح destOffset = VL_BITBIT_E(destBit);
        ثابت صحيح sourceBit = sourceLsb + destBit - destLsb;
        ثابت صحيح sourceWord = VL_BITWORD_E(sourceBit);
        ثابت صحيح sourceOffset = VL_BITBIT_E(sourceBit);
        ثابت صحيح copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        ثابت uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        ثابت قيمة_And_X& sourceLo = source.m_data.رقم()[sourceWord];
        uint32_t قيمة = sourceLo.m_قيمة >> sourceOffset;
        uint32_t قيمة_X = sourceLo.m_قيمة_X >> sourceOffset;
        إذا (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            ثابت قيمة_And_X& sourceHi = source.m_data.رقم()[sourceWord + 1];
            قيمة |= sourceHi.m_قيمة << (VL_EDATASIZE - sourceOffset);
            قيمة_X |= sourceHi.m_قيمة_X << (VL_EDATASIZE - sourceOffset);
        }

        قيمة_And_X& dest = m_data.رقم()[destWord];
        dest.m_قيمة = (dest.m_قيمة & ~mask) | ((قيمة << destOffset) & mask);
        dest.m_قيمة_X = (dest.m_قيمة_X & ~mask) | ((قيمة_X << destOffset) & mask);
    }
}

V3_العدد& V3_العدد::opConcat(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    إذا (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opLenN(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opRepl(ثابت V3_العدد& lhs,
                           ثابت V3_العدد& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    إرجاع opRepl(lhs, rhs.toUInt());
}

V3_العدد& V3_العدد::opRepl(ثابت V3_العدد& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    لكل (unsigned times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.width();
        إذا (destLsb >= static_cast<uint32_t>(width())) اكسر;
        copyBits(static_cast<صحيح>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<صحيح>(destLsb)));
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opStreamL(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    إذا (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحيح ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    لكل (صحيح istart = 0; istart < lhs.width(); istart += ssize) {
        ثابت صحيح ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opLogAnd(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        إذا (lhs.bitIs1(bit)) {
            loutc = 1;
            اكسر;
        }
        إذا (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    لكل (صحيح bit = 0; bit < rhs.width(); ++bit) {
        إذا (rhs.bitIs1(bit)) {
            routc = 1;
            اكسر;
        }
        إذا (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    إذا (routc == 1 && loutc == 1) outc = 1;
    إذا (routc == 0 || loutc == 0) outc = 0;
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opLogOr(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        إذا (lhs.bitIs1(bit)) {
            outc = 1;
            اذهب last;
        }
        إذا (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    لكل (صحيح bit = 0; bit < rhs.width(); ++bit) {
        إذا (rhs.bitIs1(bit)) {
            outc = 1;
            اذهب last;
        }
        إذا (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opLogIf(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_العدد lnot = lhs;
    lnot.opLogNot(lhs);
    إرجاع opLogOr(lnot, rhs);
}

V3_العدد& V3_العدد::opLogEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_العدد ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_العدد ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    إرجاع opLogAnd(ifa, ifb);
}

V3_العدد& V3_العدد::opAtoN(ثابت V3_العدد& lhs, صحيح base) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    إذا (base == AstAtoN::ATOREAL) إرجاع setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    إذا (errno != 0) v = 0;
    إرجاع setLongS(static_cast<int32_t>(v));
}

V3_العدد& V3_العدد::opPutcN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, ثابت V3_العدد& ths) {
    رقم_ASSERT_OP_ARGS3(lhs, rhs, ths);
    رقم_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    ثابت int32_t i = rhs.toSInt();
    ثابت int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    إرجاع setString(lstring);
}

V3_العدد& V3_العدد::opGetcN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.toString();
    ثابت int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    إذا (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    إرجاع setLong(v);
}

V3_العدد& V3_العدد::opSubstrN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, ثابت V3_العدد& ths) {
    رقم_ASSERT_OP_ARGS3(lhs, rhs, ths);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.toString();
    ثابت int32_t i = rhs.toSInt();
    ثابت int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    إذا (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) إرجاع setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    إرجاع setString(lstring.substr(i, j - i + 1));
}

V3_العدد& V3_العدد::opCompareNN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, منطقي ignoreCase) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحيح result;
    ثابت string lstring = lhs.toString();
    ثابت string rstring = rhs.toString();
    إذا (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } وإلا {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    إرجاع setLongS(result);
}

V3_العدد& V3_العدد::opEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) إرجاع opEqN(lhs, rhs);
    إذا (lhs.isDouble()) إرجاع opEqD(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        إذا (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            اذهب last;
        }
        إذا (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            اذهب last;
        }
        إذا (lhs.bitIsXZ(bit)) outc = 'x';
        إذا (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opNeq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    إذا (lhs.isString()) إرجاع opNeqN(lhs, rhs);
    إذا (lhs.isDouble()) إرجاع opNeqD(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        إذا (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            اذهب last;
        }
        إذا (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            اذهب last;
        }
        إذا (lhs.bitIsXZ(bit)) outc = 'x';
        إذا (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    إرجاع setSingleBits(outc);
}

V3Hash V3_العدد::toHash() ثابت {
    // Shouldmatch isCaseEq
    إذا (isString()) إرجاع V3Hash{toString()};
    إذا (isDouble()) إرجاع V3Hash{toDouble()};
    V3Hash hash{width()};
    لكل (صحيح i = 0; i < كلمات(); ++i) hash += m_data.رقم()[i].m_قيمة;
    إرجاع hash;
}

منطقي V3_العدد::isCaseEq(ثابت V3_العدد& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    إذا (isString()) إرجاع toString() == rhs.toString();
    إذا (isDouble()) إرجاع toDouble() == rhs.toDouble();
    إذا (width() != rhs.width()) إرجاع خطأ;
    لكل (صحيح i = 0; i < كلمات(); ++i) {
        إذا (!(m_data.رقم()[i] == rhs.m_data.رقم()[i])) إرجاع خطأ;
    }
    إرجاع true;
}

V3_العدد& V3_العدد::opCaseEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_العدد& V3_العدد::opCaseNeq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    إذا (lhs.isString()) {
        إرجاع opNeqN(lhs, rhs);
    } وإلا إذا (lhs.isDouble()) {
        إرجاع opNeqD(lhs, rhs);
    }
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        إذا (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            اذهب last;
        }
    }
last:
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opWildEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        إذا (!rhs.bitIsXZ(bit)) {
            إذا (lhs.bitIsXZ(bit)) {
                outc = 'x';
                اذهب last;
            }
            إذا (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                اذهب last;
            }
        }
    }
last:
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opWildNeq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        إذا (!rhs.bitIsXZ(bit)) {
            إذا (lhs.bitIsXZ(bit)) {
                outc = 'x';
                اذهب last;
            }
            إذا (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                اذهب last;
            }
        }
    }
last:
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opGt(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        إذا (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        إذا (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        إذا (lhs.bitIsXZ(bit)) outc = 'x';
        إذا (rhs.bitIsXZ(bit)) outc = 'x';
    }
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opGtS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحيح mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        إذا (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } وإلا إذا (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } وإلا إذا (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } وإلا إذا (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } وإلا {
            // both positive or negative, normal >
            لكل (صحيح bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                إذا (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                إذا (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                إذا (lhs.bitIsXZ(bit)) outc = 'x';
                إذا (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    إرجاع setSingleBits(outc);
}

V3_العدد& V3_العدد::opGte(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_العدد& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) إرجاع eq;  // Return true
    إرجاع opGt(lhs, rhs);
}

V3_العدد& V3_العدد::opGteS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    V3_العدد& eq = opEq(lhs, rhs);
    إذا (eq.isNeqZero()) إرجاع eq;  // Return true
    إرجاع opGtS(lhs, rhs);
}

V3_العدد& V3_العدد::opLt(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) { إرجاع opGt(rhs, lhs); }
V3_العدد& V3_العدد::opLte(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) { إرجاع opGte(rhs, lhs); }
V3_العدد& V3_العدد::opLtS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) { إرجاع opGtS(rhs, lhs); }
V3_العدد& V3_العدد::opLteS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) { إرجاع opGteS(rhs, lhs); }

V3_العدد& V3_العدد::opShiftR(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // L(lhs) bit return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) إرجاع setAllBitsX();
    setZero();
    إذا (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) إرجاع *هذا;
    ثابت uint32_t rhsval = rhs.toUInt();
    إذا (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<صحيح>(rhsval),
                 std::min(width(), lhs.width() - static_cast<صحيح>(rhsval)));
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opShiftRS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) إرجاع setAllBitsX();
    setZero();
    ثابت منطقي overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    إذا (!overflow) {
        ثابت uint32_t rhsval = rhs.toUInt();
        إذا (rhsval < lbits) {
            ثابت صحيح copyWidth = std::min(width(), static_cast<صحيح>(lbits - rhsval));
            copyBits(0, lhs, static_cast<صحيح>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            إرجاع *هذا;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opShiftL(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // L(lhs) bit return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (rhs.isFourState()) إرجاع setAllBitsX();
    setZero();
    إذا (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) إرجاع *هذا;
    ثابت uint32_t rhsval = rhs.toUInt();
    إذا (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<صحيح>(rhsval), lhs, 0,
                 std::min(width() - static_cast<صحيح>(rhsval), lhs.width()));
    }
    إرجاع *هذا;
}

//======================================================================
// Ops - Arithmetic

V3_العدد& V3_العدد::opNegate(ثابت V3_العدد& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    إذا (lhs.isFourState()) إرجاع setAllBitsX();
    V3_العدد notlhs(&lhs, width());
    notlhs.opNot(lhs);
    ثابت V3_العدد one(&lhs, width(), 1);
    opAdd(notlhs, one);
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opAdd(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    لكل (صحيح word = 0; word < كلمات(); ++word) {
        ثابت uint64_t lwordval = lhs.m_data.رقم()[word].m_قيمة;
        ثابت uint64_t rwordval = rhs.m_data.رقم()[word].m_قيمة;
        ثابت uint64_t sum = lwordval + rwordval + carry;
        m_data.رقم()[word].m_قيمة = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opSub(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    V3_العدد negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    إرجاع opAdd(lhs, negrhs);
}
V3_العدد& V3_العدد::opMul(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    setZero();
    إذا (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } وإلا {
        لكل (صحيح lword = 0; lword < lhs.كلمات(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.رقم()[lword].m_قيمة);
            إذا (lwordval == 0) استمر;
            لكل (صحيح rword = 0; rword < rhs.كلمات(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.رقم()[rword].m_قيمة);
                إذا (rwordval == 0) استمر;
                uint64_t mul = lwordval * rwordval;
                لكل (صحيح qword = lword + rword; qword < كلمات(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.رقم()[qword].m_قيمة);
                    m_data.رقم()[qword].m_قيمة = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    إذا (mul == 0) اكسر;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opMulS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    V3_العدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_العدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_العدد qNoSign = opMul(lhsNoSign, rhsNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        opAssign(qNoSign);
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opDiv(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    إذا (rhs.isEqZero()) إرجاع setAllBitsXRemoved();
    إذا (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        إرجاع *هذا;
    } وإلا {
        // Wide division
        إرجاع opModDivGuts(lhs, rhs, خطأ);
    }
}
V3_العدد& V3_العدد::opDivS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    إذا (rhs.isEqZero()) إرجاع setAllBitsXRemoved();
    V3_العدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_العدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_العدد qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    إذا ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } وإلا {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *هذا);
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opModDiv(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    إذا (rhs.isEqZero()) إرجاع setAllBitsXRemoved();
    إذا (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        إرجاع *هذا;
    } وإلا {
        // Wide modulus
        إرجاع opModDivGuts(lhs, rhs, true);
    }
}
V3_العدد& V3_العدد::opModDivS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    إذا (rhs.isEqZero()) إرجاع setAllBitsXRemoved();
    V3_العدد lhsNoSign = lhs;
    إذا (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_العدد rhsNoSign = rhs;
    إذا (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3_العدد qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    إذا (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } وإلا {
        opAssign(qNoSign);
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opModDivGuts(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, منطقي is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    ثابت صحيح l_كلمات = lhs.كلمات();
    ثابت صحيح umsbp1 = lhs.mostSetBitP1();  // dividend
    ثابت صحيح vmsbp1 = rhs.mostSetBitP1();  //قاسم
    إذا (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *هذا);
        إرجاع *هذا;
    }

    ثابت صحيح uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ثابت صحيح vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    إذا (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        لكل (صحيح j = uw - 1; j >= 0; j--) {
            ثابت uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.رقم()[j].m_قيمة));
            m_data.رقم()[j].m_قيمة = unw64 / static_cast<uint64_t>(rhs.m_data.رقم()[0].m_قيمة);
            k = unw64
                - (static_cast<uint64_t>(m_data.رقم()[j].m_قيمة)
                   * static_cast<uint64_t>(rhs.m_data.رقم()[0].m_قيمة));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *هذا << " rem=0x" << std::hex
                                   << k << std::dec);
        إذا (is_modulus) {
            setZero();
            m_data.رقم()[0].m_قيمة = k;
        }
        opCleanThis();
        إرجاع *هذا;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_كلمات + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_كلمات + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    لكل (صحيح i = 0; i < l_كلمات; ++i) m_data.رقم()[i].m_قيمة = 0;
    لكل (صحيح i = 0; i < l_كلمات + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحيح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    لكل (صحيح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.رقم()[i].m_قيمة << s)
                | (shift_mask & (rhs.m_data.رقم()[i - 1].m_قيمة >> (32 - s)));
    }
    vn[0] = rhs.m_data.رقم()[0].m_قيمة << s;

    // Copy and shift dividend by same amount; may set new upper word
    إذا (s) {
        un[uw] = lhs.m_data.رقم()[uw - 1].m_قيمة >> (32 - s);
    } وإلا {
        un[uw] = 0;
    }
    لكل (صحيح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.رقم()[i].m_قيمة << s)
                | (shift_mask & (lhs.m_data.رقم()[i - 1].m_قيمة >> (32 - s)));
    }
    un[0] = lhs.m_data.رقم()[0].m_قيمة << s;

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
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.رقم()[j].m_قيمة = qhat;  // Save quotient digit

        إذا (t < 0) {
            // Over subtracted; correct by adding back
            m_data.رقم()[j].m_قيمة--;
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
            m_data.رقم()[i].m_قيمة = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        لكل (صحيح i = vw; i < l_كلمات; ++i) m_data.رقم()[i].m_قيمة = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *هذا);
        إرجاع *هذا;
    } وإلا {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *هذا);
        إرجاع *هذا;
    }
}

V3_العدد& V3_العدد::opPow(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, منطقي lsign, منطقي rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_LOGIC_ARGS2(lhs, rhs);
    إذا (lhs.isFourState() || rhs.isFourState()) إرجاع setAllBitsX();
    إذا (rhs.isEqZero()) إرجاع setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    إذا (rsign && rhs.isNegative()) {
        إذا (lhs.isEqZero()) {
            إرجاع setAllBitsXRemoved();
        } وإلا إذا (lhs.isEqOne()) {
            إرجاع setValue1();
        } وإلا إذا (lsign && lhs.isEqAllOnes()) {
            إذا (rhs.bitIs1(0)) {
                إرجاع setAllBits1();  // -1^odd=-1
            } وإلا {
                إرجاع setValue1();  // -1^even=1
            }
        }
        إرجاع setZero();
    }
    إذا (lhs.isEqZero()) إرجاع setZero();
    setZero();
    m_data.رقم()[0].m_قيمة = 1;
    V3_العدد power(&lhs, width());
    power.opAssign(lhs);
    لكل (صحيح bit = 0; bit < rhs.width(); ++bit) {
        إذا (bit > 0) {  // power = power*power
            V3_العدد lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        إذا (rhs.bitIs1(bit)) {  // out *= power
            V3_العدد lastOut(&lhs, width());
            lastOut.opAssign(*هذا);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opPowSU(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Correct number of zero bits/width matters
    إرجاع opPow(lhs, rhs, true, خطأ);
}
V3_العدد& V3_العدد::opPowSS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Correct number of zero bits/width matters
    إرجاع opPow(lhs, rhs, true, true);
}
V3_العدد& V3_العدد::opPowUS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // Correct number of zero bits/width matters
    إرجاع opPow(lhs, rhs, خطأ, true);
}

V3_العدد& V3_العدد::opBufIf1(ثابت V3_العدد& ens, ثابت V3_العدد& if1s) {
    رقم_ASSERT_OP_ARGS2(ens, if1s);
    رقم_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        إذا (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } وإلا {
            setBit(bit, 'z');
        }
    }
    إرجاع *هذا;
}

// Sets all bits in range to the given value
V3_العدد& V3_العدد::opSetRange(uint32_t lsb, uint32_t width, حرف bit_قيمة) {
    إذا (lsb < static_cast<uint32_t>(هذا->width())) {
        fillBits(static_cast<صحيح>(lsb),
                 static_cast<صحيح>(std::min(width, static_cast<uint32_t>(هذا->width()) - lsb)),
                 bit_قيمة);
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opAssign(ثابت V3_العدد& lhs) { إرجاع opAssignNonXZ(lhs, خطأ); }
V3_العدد& V3_العدد::opAssignNonXZ(ثابت V3_العدد& lhs, منطقي ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    إذا (هذا != &lhs) {
        إذا (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = true;
        } وإلا إذا (isString()) {
            إذا (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } وإلا {
                m_data.str() = lhs.m_data.str();
            }
        } وإلا إذا (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } وإلا إذا (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } وإلا إذا (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } وإلا {
            لكل (صحيح word = 0; word < كلمات(); ++word) {
                ثابت uint32_t قيمة = word < lhs.كلمات() ? lhs.m_data.رقم()[word].m_قيمة
                                                                & ~lhs.m_data.رقم()[word].m_قيمة_X
                                                          : 0;
                m_data.رقم()[word] = {قيمة, 0};
            }
            opCleanThis();
        }
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opNToI(ثابت V3_العدد& lhs) {
    // String to packed number
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    setZero();
    ثابت string& str = lhs.toString();
    لكل (size_t n = 0; n < str.length(); ++n) {
        ثابت حرف c = str[str.length() - 1 - n];
        لكل (size_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opExtendS(ثابت V3_العدد& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ثابت صحيح copyWidth = std::min(width(), static_cast<صحيح>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opExtendXZ(ثابت V3_العدد& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ثابت صحيح copyWidth = std::min(width(), static_cast<صحيح>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opClean(ثابت V3_العدد& lhs, uint32_t bits) { إرجاع opSel(lhs, bits - 1, 0); }

فراغ V3_العدد::opCleanThis(منطقي warnOnTruncation) {
    // Clean MSB of number
    رقم_ASSERT_LOGIC_ARGS1(*هذا);
    ثابت قيمة_And_X v = m_data.رقم()[كلمات() - 1];
    ثابت uint32_t new_قيمة_Msb = v.m_قيمة & hiWordMask();
    ثابت uint32_t new_قيمة_XMsb = v.m_قيمة_X & hiWordMask();
    إذا (warnOnTruncation && (new_قيمة_Msb != v.m_قيمة || new_قيمة_XMsb != v.m_قيمة_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.رقم()[كلمات() - 1] = {new_قيمة_Msb, new_قيمة_XMsb};
}

V3_العدد& V3_العدد::opSel(ثابت V3_العدد& lhs, ثابت V3_العدد& msb, ثابت V3_العدد& lsb) {
    رقم_ASSERT_OP_ARGS3(lhs, msb, lsb);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS2(msb, lsb);
    إذا (lsb.isFourState() || msb.isFourState()) إرجاع setAllBitsX();
    إرجاع opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_العدد& V3_العدد::opSel(ثابت V3_العدد& lhs, uint32_t msbval, uint32_t lsbval) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    صحيح copyWidth = 0;
    إذا (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        ثابت uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<صحيح>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<صحيح>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    إرجاع *هذا;
}

V3_العدد& V3_العدد::opSelInto(ثابت V3_العدد& lhs, ثابت V3_العدد& lsb, صحيح width) {
    إرجاع opSelInto(lhs, lsb.toSInt(), width);
}

V3_العدد& V3_العدد::opSelInto(ثابت V3_العدد& lhs, صحيح lsbval, صحيح width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحيح copyWidth = std::max(0, std::min({width, lhs.width(), هذا->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    ثابت صحيح fillWidth
        = std::max(0, std::min(width - copyWidth, هذا->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    إرجاع *هذا;
}

//======================================================================
// Ops - Floating point

V3_العدد& V3_العدد::opIToRD(ثابت V3_العدد& lhs, منطقي isSigned) {
    // Correct number of zero bits/width matters
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_العدد noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    مزدوج d = 0;
    ثابت منطقي negate = isSigned && noxz.isNegative();
    إذا (negate) {
        ثابت V3_العدد noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    لكل (صحيح bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        إذا (noxz.bitIs1(bit)) d += exp2(bit);
    }
    إذا (negate) d = -d;
    إرجاع setDouble(d);
}
V3_العدد& V3_العدد::opRToIS(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_TRUNC(lhs.toDouble());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    إرجاع setLongS(i);
}
V3_العدد& V3_العدد::opRToIRoundS(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_ROUND(lhs.toDouble());
    setZero();
    اتحاد {
        مزدوج d;
        uint64_t q;
    } u;
    u.d = v;
    (فراغ)u.d;

    ثابت صحيح exp = static_cast<صحيح>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    ثابت صحيح lsb = exp - 52;
    ثابت uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    إذا (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        لكل (صحيح bit = 0; bit <= 52; ++bit) {
            إذا (mantissa & (1ULL << bit)) {
                ثابت صحيح outbit = bit + lsb;
                إذا (outbit >= 0) setBit(outbit, 1);
            }
        }
        إذا (v < 0) {
            ثابت V3_العدد noSign = *هذا;
            opNegate(noSign);
        }
    }
    إرجاع *هذا;
}
V3_العدد& V3_العدد::opRealToBits(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    إرجاع setQuad(u.m_v);
}
V3_العدد& V3_العدد::opBitsToRealD(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    إرجاع setDouble(u.m_d);
}
V3_العدد& V3_العدد::opNegateD(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_DOUBLE_ARGS1(lhs);
    إرجاع setDouble(-lhs.toDouble());
}
V3_العدد& V3_العدد::opAddD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_العدد& V3_العدد::opSubD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_العدد& V3_العدد::opMulD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_العدد& V3_العدد::opDivD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_العدد& V3_العدد::opPowD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_العدد& V3_العدد::opEqD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_العدد& V3_العدد::opNeqD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_العدد& V3_العدد::opGtD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_العدد& V3_العدد::opGteD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_العدد& V3_العدد::opLtD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_العدد& V3_العدد::opLteD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_العدد& V3_العدد::opConcatN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setString(lhs.toString() + rhs.toString());
}
V3_العدد& V3_العدد::opReplN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_STRING_ARGS1(lhs);
    رقم_ASSERT_LOGIC_ARGS1(rhs);
    إرجاع opReplN(lhs, rhs.toUInt());
}
V3_العدد& V3_العدد::opReplN(ثابت V3_العدد& lhs, uint32_t rhsval) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    لكل (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    إرجاع setString(out);
}
V3_العدد& V3_العدد::opCvtPackString(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    إرجاع setString(lhs.toString());
}
V3_العدد& V3_العدد::opToLowerN(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string out = VString::downcase(lhs.toString());
    إرجاع setString(out);
}
V3_العدد& V3_العدد::opToUpperN(ثابت V3_العدد& lhs) {
    رقم_ASSERT_OP_ARGS1(lhs);
    رقم_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string out = VString::upcase(lhs.toString());
    إرجاع setString(out);
}

V3_العدد& V3_العدد::opEqN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toString() == rhs.toString());
}
V3_العدد& V3_العدد::opNeqN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toString() != rhs.toString());
}
V3_العدد& V3_العدد::opGtN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toString() > rhs.toString());
}
V3_العدد& V3_العدد::opGteN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toString() >= rhs.toString());
}
V3_العدد& V3_العدد::opLtN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toString() < rhs.toString());
}
V3_العدد& V3_العدد::opLteN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs) {
    رقم_ASSERT_OP_ARGS2(lhs, rhs);
    رقم_ASSERT_STRING_ARGS2(lhs, rhs);
    إرجاع setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

فراغ V3_العدد::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_سطر* ثابت fl = جديد File_سطر{File_سطر::builtInFilename()};
    V3_العدد رقم{fl, 32, 0};
    رقم.selfTestThis();
}

فراغ V3_العدد::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_العدد::epsilonEqual(0, 0), true);
    UASSERT_SELFTEST(V3_العدد::epsilonEqual(1e19, 1e19), true);
    UASSERT_SELFTEST(V3_العدد::epsilonEqual(9, 0.0001), خطأ);
    UASSERT_SELFTEST(V3_العدد::epsilonEqual(1, 1 + std::numeric_limits<مزدوج>::epsilon()), true);
    UASSERT_SELFTEST(V3_العدد::epsilonEqual(0.009, 0.00899999999999999931998839741709), true);

    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(0), true);
    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(1), true);
    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(-1), true);
    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(1.0001), خطأ);
    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(0.9999), خطأ);
    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(-1.0001), خطأ);
    UASSERT_SELFTEST(V3_العدد::epsilonIntegral(-0.9999), خطأ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
