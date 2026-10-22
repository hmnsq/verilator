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
#define NUM_ASSERT_OP_ARGS1(arg1) \
    UASSERT((هذا != &(arg1)), "Number operation called with same source and dest")
#define NUM_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2)), \
            "Number operation called with same source and dest")
#define NUM_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3)), \
            "Number operation called with same source and dest")
#define NUM_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((هذا != &(arg1) && هذا != &(arg2) && هذا != &(arg3) && هذا != &(arg4)), \
            "Number operation called with same source and dest")

#define NUM_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3NumberData::V3NumberDataType::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define NUM_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    NUM_ASSERT_LOGIC_ARGS1(arg1); \
    NUM_ASSERT_LOGIC_ARGS1(arg2)

#define NUM_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    NUM_ASSERT_LOGIC_ARGS1(arg1); \
    NUM_ASSERT_LOGIC_ARGS1(arg2); \
    NUM_ASSERT_LOGIC_ARGS1(arg3); \
    NUM_ASSERT_LOGIC_ARGS1(arg4)

#define NUM_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define NUM_ASSERT_STRING_ARGS2(arg1, arg2) \
    NUM_ASSERT_STRING_ARGS1(arg1); \
    NUM_ASSERT_STRING_ARGS1(arg2)

#define NUM_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define NUM_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    NUM_ASSERT_DOUBLE_ARGS1(arg1); \
    NUM_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

فراغ V3Number::v3errorEnd(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ܐܢ (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } ܐܚܪܢܐ ܐܢ (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } ܐܚܪܢܐ {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

فراغ V3Number::v3errorEndFatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3Number::V3Number(AstNode* nodep, VerilogStringLiteral, ثابت string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    ܐܢ (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } ܐܚܪܢܐ {
        init(nodep, std::max<صحيح>(str.length() * 8, 1));
        لكل (unsigned pos = 0; pos < str.length(); ++pos) {
            ثابت صحيح topos = str.length() - 1 - pos;
            ValueAndX& v = m_data.num()[topos / 4];
            لكل (صحيح bit = 0; bit < 8; ++bit) {
                ܐܢ (str[pos] & (1UL << bit)) v.m_value |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = ܫܪܪܐ;
    opCleanThis(ܫܪܪܐ);
}

V3Number::V3Number(AstNode* nodep, ثابت AstNodeDType* nodedtypep) {
    ܐܢ (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } ܐܚܪܢܐ ܐܢ (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } ܐܚܪܢܐ ܐܢ (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } ܐܚܪܢܐ {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

فراغ V3Number::create(ثابت حرف* sourcep) {
    m_data.setLogic();
    ثابت حرف* value_startp = sourcep;
    لكل (ثابت حرف* cp = sourcep; *cp; ++cp) {
        ܐܢ (*cp == '\'') {
            value_startp = cp + 1;
            اكسر;
        }
    }

    منطقي userSized = ܕܓܠܐ;
    منطقي unbased = ܕܓܠܐ;
    حرف base = '\0';
    ܐܢ (value_startp != sourcep) {  // Has a '
        string widthn;
        ثابت حرف* cp = sourcep;
        لكل (; *cp; ++cp) {
            ܐܢ (*cp == '\'') {
                ++cp;
                اكسر;
            }
            ܐܢ (*cp != '_') widthn += *cp;
        }
        ܟܡܐ (*cp == '_') cp++;
        ܐܢ (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(ܫܪܪܐ);
        }
        ܐܢ (*cp) {
            base = *cp;
            ++cp;
        }
        value_startp = cp;

        userSized = widthn.length() != 0;
        ܐܢ (std::atoi(widthn.c_str())) {
            ܐܢ (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.maxNumWidth()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.maxNumWidth(), ܫܪܪܐ);
            } ܐܚܪܢܐ {
                width(std::atoi(widthn.c_str()), ܫܪܪܐ);
            }
        }
    } ܐܚܪܢܐ {
        unbased = ܫܪܪܐ;
        base = 'd';
    }

    لكل (صحيح i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    ܐܢ (base == '0') {
        width(1, ܕܓܠܐ);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (base == '1') {
        width(1, ܕܓܠܐ);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (std::tolower(base) == 'z') {
        width(1, ܕܓܠܐ);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = ܫܪܪܐ;
    } ܐܚܪܢܐ ܐܢ (std::tolower(base) == 'x') {
        width(1, ܕܓܠܐ);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = ܫܪܪܐ;
    }
    // Otherwise...
    ܐܚܪܢܐ ܐܢ (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ثابت صحيح widthBound = std::max<صحيح>(32, std::strlen(value_startp) * 4);
        width(widthBound, ܕܓܠܐ);  // Will change width below
        ܐܢ (unbased) isSigned(ܫܪܪܐ);  // Also says the spec.
    }

    // Ignore leading blanks
    ܟܡܐ (*value_startp == '_' || std::isspace(*value_startp)) ++value_startp;
    ܐܢ (!*value_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    ܐܢ (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    صحيح obit = 0;  // Start at LSB
    صحيح base_align = 1;
    ܐܢ (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        ܟܡܐ (*value_startp == '_' || *value_startp == '0') ++value_startp;
        // Convert decimal number to hex
        صحيح olen = 0;
        uint32_t val = 0;
        صحيح got_x = 0;
        صحيح got_z = 0;
        صحيح got_01 = 0;
        منطقي warnedTooMany = ܕܓܠܐ;
        لكل (ثابت حرف* cp = value_startp; *cp; ++cp) {
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
                ܐܢ (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.num()[0].m_value = val;
                } ܐܚܪܢܐ {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3Number product(هذا, width() + 4);  // +4 for overflow detection
                    ثابت V3Number ten(هذا, width() + 4, 10);
                    ثابت V3Number addend(هذا, width(), (*cp - '0'));
                    product.opMul(*هذا, ten);
                    opAdd(product, addend);
                    ܐܢ (!warnedTooMany && product.bitsValue(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = ܫܪܪܐ;
                    }
                }
                ++olen;
                got_01 = 1;
                اكسر;
            }
            حالة 'z':
            حالة '?': {
                got_z = 1;
                ܐܢ (!userSized) width(32, ܕܓܠܐ);
                setAllBitsZ();
                اكسر;
            }
            حالة 'x': {
                got_x = 1;
                ܐܢ (!userSized) width(32, ܕܓܠܐ);
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
        ܐܢ ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << value_startp);
        }
    } ܐܚܪܢܐ {
        // Convert bin/octal number to hex
        لكل (ثابت حرف* cp = value_startp + std::strlen(value_startp) - 1; cp >= value_startp;
             cp--) {
            ܐܢ (*cp != '_' && *cp != '0' && obit >= width()) {
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
    ܐܢ (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), ܕܓܠܐ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    ܟܡܐ (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(ܫܪܪܐ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

فراغ V3Number::warnTooMany(ثابت string& value) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << value << "'");
}

فراغ V3Number::nodep(AstNode* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    ܐܢ (!nodep) ܦܢܐ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

منطقي V3Number::epsilonEqual(مزدوج a, مزدوج b) {
    ܦܢܐ std::fabs(a - b)
           <= (std::numeric_limits<مزدوج>::epsilon() * std::max(1.0, std::max(a, b)));
}

منطقي V3Number::epsilonIntegral(مزدوج a) {
    ثابت مزدوج dint = static_cast<مزدوج>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ܦܢܐ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

صحيح V3Number::log2b(uint32_t num) {
    // See also opCLog2
    لكل (صحيح bit = 31; bit > 0; bit--) {
        ܐܢ (num & (1ULL << bit)) ܦܢܐ bit;
    }
    ܦܢܐ 0;
}

صحيح V3Number::log2bQuad(uint64_t num) {
    ܐܢ (num >> 32ULL) ܦܢܐ 32 + log2b(num >> 32ULL);
    ܦܢܐ log2b(num);
}

//======================================================================
// Setters

V3Number& V3Number::setZero() {
    لكل (صحيح i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};
    ܦܢܐ *هذا;
}
V3Number& V3Number::setQuad(uint64_t value) {
    setZero();
    m_data.num()[0].m_value = value & 0xffffffffULL;
    ܐܢ (width() > 32) m_data.num()[1].m_value = (value >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setLong(uint32_t value) {
    setZero();
    m_data.num()[0].m_value = value;
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setLongS(int32_t value) {
    setZero();
    اتحاد {
        uint32_t u;
        int32_t s;
    } u;
    u.s = value;
    (فراغ)u.s;
    m_data.num()[0].m_value = u.u;
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setDouble(مزدوج value) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.d = value;
    (فراغ)u.d;
    لكل (صحيح i = 2; i < words(); ++i) m_data.num()[i] = {0, 0};
    m_data.num()[0].m_value = u.u[0];
    m_data.num()[1].m_value = u.u[1];
    ܦܢܐ *هذا;
}
V3Number& V3Number::setSingleBits(حرف value) {
    لكل (صحيح i = 1 /*upper*/; i < words(); ++i) m_data.num()[i] = {0, 0};
    m_data.num()[0] = {(value == '1' || value == 'x' || value == 1 || value == 3),
                       (value == 'z' || value == 'x' || value == 2 || value == 3)};
    ܦܢܐ *هذا;
}

V3Number& V3Number::setAllBits0() {
    setZero();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setAllBits1() {
    لكل (صحيح i = 0; i < words(); ++i) m_data.num()[i] = {~0U, 0};
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    لكل (صحيح i = 0; i < words(); ++i) m_data.num()[i] = {~0U, ~0U};
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setAllBitsZ() {
    لكل (صحيح i = 0; i < words(); ++i) m_data.num()[i] = {0, ~0U};
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::setAllBitsXRemoved() {
    ܐܢ (!v3Global.constRemoveXs()) {
        ܦܢܐ setAllBitsX();
    } ܐܚܪܢܐ {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        ܐܢ (v3Global.opt.xAssign() == "1") {
            ܦܢܐ setAllBits1();
        } ܐܚܪܢܐ {
            ܦܢܐ setAllBits0();
        }
    }
}
V3Number& V3Number::setValue1() {
    m_data.num()[0] = {1, 0};
    لكل (صحيح i = 1; i < words(); ++i) m_data.num()[i] = {0, 0};
    ܦܢܐ *هذا;
}

فراغ V3Number::setBitX0(صحيح bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3Number& V3Number::setMask(صحيح nbits, صحيح lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    ܦܢܐ *هذا;
}

//======================================================================
// ACCESSORS - as strings

string V3Number::ascii(منطقي prefixed, منطقي cleanVerilog) ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ܐܢ (is1Step()) ܦܢܐ "1step";
    ܐܢ (isNull()) ܦܢܐ "null";
    std::ostringstream out;
    ܐܢ (isDouble()) {
        out.precision(17);
        ܐܢ (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } ܐܚܪܢܐ {
            out << toDouble();
            ܐܢ (toDouble() == floor(toDouble())) out << ".0";
        }
        ܦܢܐ out.str();
    } ܐܚܪܢܐ ܐܢ (isString()) {
        ܦܢܐ '"' + toString() + '"';
    } ܐܚܪܢܐ {
        ܐܢ (VL_UNCOVERABLE((m_data.num()[words() - 1].m_value | m_data.num()[words() - 1].m_valueX)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    ܐܢ (prefixed) {
        ܐܢ (sized()) {
            out << width() << "'";
        } ܐܚܪܢܐ ܐܢ (autoExtend() && width() == 1) {
            out << "'";
            ܐܢ (bitIs0(0)) {
                out << '0';
            } ܐܚܪܢܐ ܐܢ (bitIs1(0)) {
                out << '1';
            } ܐܚܪܢܐ ܐܢ (bitIsZ(0)) {
                out << 'z';
            } ܐܚܪܢܐ {
                out << 'x';
            }
            ܦܢܐ out.str();
        } ܐܚܪܢܐ {
            ܐܢ (cleanVerilog) {
                out << "'";
            } ܐܚܪܢܐ {
                out << "?" << width() << "?";
            }
        }
        ܐܢ (isSigned()) out << "s";
    }

    ثابت منطقي binary = (isFourState()
#ifdef V3NUMBER_ASCII_BINARY
                         // cppcheck-suppress konwnConditionTrueFalse
                         || ܫܪܪܐ
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    ܐܢ (binary) {
        out << "b";
        out << displayed("%0b");
    } ܐܚܪܢܐ {
        ܐܢ (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    ܐܢ (isNull()) {
        ܐܢ (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } ܐܚܪܢܐ {
            out << " [null]";
        }
    }
    ܦܢܐ out.str();
}

منطقي V3Number::displayedFmtHasArg(حرف format, منطقي isScan) {
    // Is this a format letter that takes an argument in runtime?
    (فراغ)isScan;
    بدل (std::tolower(format)) {
    حالة 'b': ܦܢܐ ܫܪܪܐ;  // Binary
    حالة 'c': ܦܢܐ ܫܪܪܐ;  // Character
    حالة 'd': ܦܢܐ ܫܪܪܐ;  // Decimal
    حالة 'e': ܦܢܐ ܫܪܪܐ;  // Floating
    حالة 'f': ܦܢܐ ܫܪܪܐ;  // Floating
    حالة 'g': ܦܢܐ ܫܪܪܐ;  // Floating
    حالة 'h': ܦܢܐ ܫܪܪܐ;  // Hex
    حالة 'o': ܦܢܐ ܫܪܪܐ;  // Octal
    حالة 'p': ܦܢܐ ܫܪܪܐ;  // Pattern
    حالة 's': ܦܢܐ ܫܪܪܐ;  // String
    حالة 't': ܦܢܐ ܫܪܪܐ;  // Time
    حالة 'u': ܦܢܐ ܫܪܪܐ;  // Packed 2-state
    حالة 'v': ܦܢܐ ܫܪܪܐ;  // Strength
    حالة 'x': ܦܢܐ ܫܪܪܐ;  // Hex
    حالة 'z': ܦܢܐ ܫܪܪܐ;  // Packed 4-state
    حالة '?': ܦܢܐ ܫܪܪܐ;  // Internal: Compute format in V3Width based on data type
    افتراضي: ܦܢܐ ܕܓܠܐ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3Number::displayPad(size_t fmtsize, حرف pad, منطقي left, ثابت string& in) VL_PURE {
    string padding;
    ܐܢ (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    ܦܢܐ left ? (in + padding) : (padding + in);
}

string V3Number::displayedEnumName(ثابت AstEnumItem* itemp) {
    ثابت string name = VIdProtect::protect(itemp->prettyName());
    ܦܢܐ VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned حرف>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3Number::displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE {
    V3Number value{هذا, dtypep->width()};
    value.opAssign(*هذا);
    لكل (ثابت AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
        ܐܢ (!constp->num().isAnyXZ() && value.isCaseEq(constp->num()))
            ܦܢܐ displayedEnumName(itemp);
    }
    ܦܢܐ "";
}

string V3Number::displayedEnum(ثابت AstSFormatArg* argp,
                               ثابت string& vformat) ثابت VL_MT_STABLE {
    ثابت AstEnumDType* ثابت dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3Number value{هذا, dtypep->width()};
    value.opAssign(*هذا);
    ثابت VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ثابت حرف fmt = std::tolower(vformat.back());
    ܐܢ (fmt != 'p' && fmt != 's') ܦܢܐ value.displayed(argp, vformat, numericAttr);
    string name = value.displayedEnumName(dtypep);
    ܐܢ (name.empty()) name = value.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3Number nameNum{هذا};
    nameNum.setString(name);
    ܦܢܐ nameNum.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3Number::displayed(ثابت AstNode* nodep, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    ܦܢܐ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3Number::displayed(FileLine* fl, ثابت string& vformat,
                           ثابت VFormatAttr& formatAttr) ثابت VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *هذا);
    ++pos;
    منطقي left = ܕܓܠܐ;
    ܐܢ (pos[0] == '-') {
        left = ܫܪܪܐ;
        ++pos;
    }
    string fmtsize;
    لكل (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    حرف fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    ܐܢ (formatAttr.isComplex()) {
        ܐܢ (fmt != 'p') fmt = 's';  // override
    } ܐܚܪܢܐ ܐܢ (formatAttr.isString()) {
        ܐܢ (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3Number strwide{fl, static_cast<صحيح>(toString().length()) * 8, 0};
            strwide.opNToI(*هذا);
            ܦܢܐ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        ܐܢ (fmt != 'p') fmt = 's';  // override
    } ܐܚܪܢܐ ܐܢ (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        ܐܢ (fmt == 'p') {
            ܐܢ (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } ܐܚܪܢܐ {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } ܐܚܪܢܐ ܐܢ (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        ܐܢ (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3Number nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*هذا);
            ܦܢܐ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    بدل (fmt) {
    حالة 'c': {
        ܐܢ (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ثابت unsigned صحيح v = bitsValue(0, 8);
        str.push_back(static_cast<حرف>(v));
        ܦܢܐ str;
    }
    حالة 'e':  // FALLTHRU
    حالة 'f':  // FALLTHRU
    حالة 'g': {
        ثابت مزدوج n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        حرف tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        ܦܢܐ tmp;
    }
    حالة 's': {
        ܐܢ (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } ܐܚܪܢܐ {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            صحيح bit = width() - 1;
            منطقي start = ܫܪܪܐ;
            ܟܡܐ ((bit % 8) != 7) ++bit;
            لكل (; bit >= 0; bit -= 8) {
                ثابت صحيح v = bitsValue(bit - 7, 8);
                ܐܢ (!start || v) {
                    str += static_cast<حرف>((v == 0) ? ' ' : v);
                    start = ܕܓܠܐ;  // Drop leading 0s
                } ܐܚܪܢܐ {
                    ܐܢ (fmtsize != "0") str += ' ';
                }
            }
        }
        ثابت size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', left, str);
        ܦܢܐ str;
    }
    حالة 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        ܐܢ (formatAttr.isDouble()) {
            ثابت مزدوج n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            حرف tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (فراغ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            ܦܢܐ tmp;
        }
        ܐܢ (formatAttr.isString())
            ܦܢܐ '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        ܐܢ (formatAttr.isComplex()) ܦܢܐ toString();
        ܦܢܐ "%p";
    }
    حالة 'b':  // FALLTHRU
    حالة 'o':  // FALLTHRU
    حالة 'h':  // FALLTHRU
    حالة 'x': {
        صحيح bit = width() - 1;
        ܐܢ (left || !fmtsize.empty()) {
            ܟܡܐ (bit && bitIs0(bit)) --bit;
        }
        بدل (fmt) {
        حالة 'b': {
            لكل (; bit >= 0; --bit) {
                ܐܢ (bitIs0(bit)) {
                    str += '0';
                } ܐܚܪܢܐ ܐܢ (bitIs1(bit)) {
                    str += '1';
                } ܐܚܪܢܐ ܐܢ (bitIsZ(bit)) {
                    str += 'z';
                } ܐܚܪܢܐ {
                    str += 'x';
                }
            }
            اكسر;
        }
        حالة 'o': {
            ܟܡܐ ((bit % 3) != 2) ++bit;
            لكل (; bit > 0; bit -= 3) {
                ثابت صحيح numX = countX(bit - 2, 3);
                ثابت صحيح numZ = countZ(bit - 2, 3);
                ܐܢ (numX == 3 || numX == width() - (bit - 2)) {
                    str += 'x';
                    استمر;
                }
                ܐܢ (numZ == 3 || numZ == width() - (bit - 2)) {
                    str += 'z';
                    استمر;
                }
                ܐܢ (numX > 0) {
                    str += 'X';
                    استمر;
                }
                ܐܢ (numZ > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = bitsValue(bit - 2, 3);
                str += static_cast<حرف>('0' + v);
            }
            اكسر;
        }
        افتراضي: {  // h/x
            ܟܡܐ ((bit % 4) != 3) ++bit;
            لكل (; bit > 0; bit -= 4) {
                ثابت صحيح numX = countX(bit - 3, 4);
                ثابت صحيح numZ = countZ(bit - 3, 4);
                ܐܢ (numX == 4 || numX == width() - (bit - 3)) {
                    str += 'x';
                    استمر;
                }
                ܐܢ (numZ == 4 || numZ == width() - (bit - 3)) {
                    str += 'z';
                    استمر;
                }
                ܐܢ (numX > 0) {
                    str += 'X';
                    استمر;
                }
                ܐܢ (numZ > 0) {
                    str += 'Z';
                    استمر;
                }
                ثابت صحيح v = bitsValue(bit - 3, 4);
                ܐܢ (v >= 10) {
                    str += static_cast<حرف>('a' + v - 10);
                } ܐܚܪܢܐ {
                    str += static_cast<حرف>('0' + v);
                }
            }
            اكسر;
        }
        }  // switch
        ثابت size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (left ? ' ' : '0'), left, str);
        ܦܢܐ str;
    }  // case b/d/x/o
    حالة 't':  // Time
    حالة 'd': {  // Unsigned decimal
        ثابت منطقي issigned = formatAttr.isSigned();
        ܐܢ (fmtsize == "" && !left) {
            ثابت مزدوج mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            مزدوج dchars = mantissabits / 3.321928094887362 + 1.0;
            ܐܢ (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<صحيح>(dchars));
        }
        منطقي hasXZ = ܕܓܠܐ;
        ܐܢ (isAllX()) {
            str = "x";
            hasXZ = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (isAllZ()) {
            str = "z";
            hasXZ = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (isAnyX()) {
            str = "X";
            hasXZ = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (isAnyZ()) {
            str = "Z";
            hasXZ = ܫܪܪܐ;
        }
        ܐܢ (!hasXZ) {
            ܐܢ (issigned) {
                ܐܢ (width() > 64) {
                    str = toDecimalS();
                } ܐܚܪܢܐ {
                    str = cvtToStr(toSQuad());
                }
            } ܐܚܪܢܐ {
                ܐܢ (width() > 64) {
                    str = toDecimalU();
                } ܐܚܪܢܐ {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        ثابت منطقي zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !left;
        // fmtsize might have changed since we parsed the %fmtsize
        ثابت size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), left, str);
        ܦܢܐ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    حالة 'u': {  // Packed 2-state
        لكل (صحيح i = 0; i < words(); ++i) {
            ثابت uint32_t v = m_data.num()[i].m_value;
            str += static_cast<حرف>((v >> 0) & 0xff);
            str += static_cast<حرف>((v >> 8) & 0xff);
            str += static_cast<حرف>((v >> 16) & 0xff);
            str += static_cast<حرف>((v >> 24) & 0xff);
        }
        ܦܢܐ str;
    }
    حالة 'z': {  // Packed 4-state
        لكل (صحيح i = 0; i < words(); ++i) {
            ثابت ValueAndX v = m_data.num()[i];
            str += static_cast<حرف>((v.m_value >> 0) & 0xff);
            str += static_cast<حرف>((v.m_value >> 8) & 0xff);
            str += static_cast<حرف>((v.m_value >> 16) & 0xff);
            str += static_cast<حرف>((v.m_value >> 24) & 0xff);
            str += static_cast<حرف>((v.m_valueX >> 0) & 0xff);
            str += static_cast<حرف>((v.m_valueX >> 8) & 0xff);
            str += static_cast<حرف>((v.m_valueX >> 16) & 0xff);
            str += static_cast<حرف>((v.m_valueX >> 24) & 0xff);
        }
        ܦܢܐ str;
    }
    حالة 'v': {  // Strength
        صحيح bit = width() - 1;
        لكل (; bit >= 0; bit--) {
            ܐܢ (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            ܐܚܪܢܐ ܐܢ (bitIs1(bit)) {
                str += "St1 ";
            } ܐܚܪܢܐ ܐܢ (bitIsZ(bit)) {
                str += "StZ ";
            } ܐܚܪܢܐ {
                str += "StX";
            }
        }
        ܦܢܐ str;
    }
    افتراضي: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3Number::emitC() ثابت VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    حرف sbuf[bufsize];
    string result;
    ܐܢ (isNull()) {
        ܦܢܐ "VlNull{}";
    } ܐܚܪܢܐ ܐܢ (isDouble()) {
        ثابت مزدوج dnum = toDouble();
        ܐܢ (std::isinf(dnum)) {
            ܐܢ (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } ܐܚܪܢܐ ܐܢ (std::isnan(dnum)) {
            ܐܢ (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } ܐܚܪܢܐ {
            ثابت حرف* ثابت fmt = (static_cast<صحيح>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ܦܢܐ sbuf;
        }
    } ܐܚܪܢܐ ܐܢ (isString()) {
        ثابت string strg = toString();
        ثابت string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ܦܢܐ '"' + quoted + "\"s";
    } ܐܚܪܢܐ ܐܢ (words() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(words()) + ">{{";
        ܐܢ (words() > 4) result += '\n';
        لكل (صحيح n = 0; n < words(); ++n) {
            ܐܢ (n) result += ((n % 4) ? ", " : ",\n");
            (فراغ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        ܐܢ (words() > 4) result += '\n';
        result += "}}";
    } ܐܚܪܢܐ ܐܢ (words() == 2) {  // Quad
        ثابت uint64_t qnum = static_cast<uint64_t>(toUQuad());
        ثابت حرف* ثابت fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ܦܢܐ sbuf;
    } ܐܚܪܢܐ {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ثابت uint32_t unum = toUInt();
        ثابت حرف* ثابت fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (فراغ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ܦܢܐ sbuf;
    }
    ܦܢܐ result;
}

string V3Number::toDecimalS() ثابت VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ܐܢ (isNegative()) {
        V3Number lhsNoSign = *هذا;
        lhsNoSign.opNegate(*هذا);
        ܦܢܐ "-"s + lhsNoSign.toDecimalU();
    } ܐܚܪܢܐ {
        ܦܢܐ toDecimalU();
    }
}

string V3Number::toDecimalU() ثابت VL_MT_STABLE {
    ثابت صحيح maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3Number bcd(هذا, maxdecwidth + 4);
    V3Number tmp(هذا, maxdecwidth + 4);
    V3Number tmp2(هذا, maxdecwidth + 4);

    صحيح from_bit = width() - 1;
    // Skip all leading zeros
    لكل (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    لكل (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        لكل (صحيح nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            ܐܢ (bcd.bitsValue(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3Number(هذا, 32, 1));
        // bcd[0] = this[from_bit]
        ܐܢ (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    صحيح lsb = (maxdecwidth - 1) & ~3;
    لكل (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        ܐܢ (bcd.bitsValue(lsb, 4)) اكسر;
    }
    لكل (; lsb >= 0; lsb -= 4) {
        output += static_cast<حرف>('0' + bcd.bitsValue(lsb, 4));  // 0..9
    }
    ܦܢܐ output;
}

string V3Number::toRealString() ثابت VL_MT_STABLE {
    ثابت مزدوج value = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << value;
    string result = out.str();
    ܐܢ (std::strtod(result.c_str(), nullptr) != value) {
        out.str("");
        out.precision(17);
        out << value;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    ܐܢ (result.find_first_of(".en") == string::npos) result += ".0";
    ܦܢܐ result;
}

//======================================================================
// ACCESSORS - as numbers

منطقي V3Number::fitsInUInt() ثابت VL_MT_SAFE {
    ܐܢ (isFourState()) ܦܢܐ ܕܓܠܐ;
    // We allow wide numbers that represent values <= 32 bits
    لكل (صحيح i = 1; i < words(); ++i) {
        ܐܢ (m_data.num()[i].m_value) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}
uint32_t V3Number::toUInt() ثابت VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 32 bits
    ܐܢ (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *هذا);
    }
    ܦܢܐ m_data.num()[0].m_value;
}

مزدوج V3Number::toDouble() ثابت VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    اتحاد {
        مزدوج d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.num()[0].m_value;
    u.u[1] = m_data.num()[1].m_value;
    ܦܢܐ u.d;
}

int32_t V3Number::toSInt() ثابت VL_MT_SAFE {
    ܐܢ (isSigned()) {
        ثابت uint32_t v = toUInt();
        ثابت uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        ثابت uint32_t extended = v | signExtend;
        ܦܢܐ static_cast<int32_t>(extended);
    } ܐܚܪܢܐ {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ܦܢܐ static_cast<int32_t>(toUInt());
    }
}

uint64_t V3Number::toUQuad() ثابت VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *هذا);
    // We allow wide numbers that represent values <= 64 bits
    ܐܢ (isDouble()) ܦܢܐ static_cast<uint64_t>(toDouble());
    لكل (صحيح i = 2; i < words(); ++i) {
        ܐܢ (m_data.num()[i].m_value) {
            v3error("Value too wide for 64-bits expected in this context " << *هذا);
            اكسر;
        }
    }
    ܐܢ (width() <= 32) ܦܢܐ (static_cast<uint64_t>(toUInt()));
    ܦܢܐ ((static_cast<uint64_t>(m_data.num()[1].m_value) << 32ULL)
            | (static_cast<uint64_t>(m_data.num()[0].m_value)));
}

int64_t V3Number::toSQuad() ثابت {
    ܐܢ (isDouble()) ܦܢܐ static_cast<int64_t>(toDouble());
    ثابت uint64_t v = toUQuad();
    ثابت uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    ثابت uint64_t extended = v | signExtend;
    ܦܢܐ static_cast<int64_t>(extended);
}

string V3Number::toString() ثابت {
    UASSERT(!isFourState(), "toString with 4-state " << *هذا);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    ܐܢ (isString()) ܦܢܐ m_data.str();
    صحيح bit = width() - 1;
    ܟܡܐ ((bit % 8) != 7) ++bit;
    std::string str;
    لكل (; bit >= 0; bit -= 8) {
        ثابت صحيح v = bitsValue(bit - 7, 8);
        ܐܢ (v) str += static_cast<حرف>(v);
    }
    ܦܢܐ str;
}

uint32_t V3Number::edataWord(صحيح eword) ثابت {
    UASSERT(!isFourState(), "edataWord with 4-state " << *هذا);
    ܦܢܐ m_data.num()[eword].m_value;
}

uint8_t V3Number::dataByte(صحيح byte) ثابت {
    ܦܢܐ (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

منطقي V3Number::isAllZ() ثابت VL_MT_SAFE {
    ܐܢ (isDouble() || isString()) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 0; i < words(); ++i) {
        ثابت ValueAndX v = m_data.num()[i];
        ثابت uint32_t mask = i == words() - 1 ? hiWordMask() : ~0U;
        ܐܢ (((~v.m_value & v.m_valueX) & mask) != mask) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}
منطقي V3Number::isAllX() ثابت VL_MT_SAFE {
    ܐܢ (isDouble() || isString()) ܦܢܐ ܕܓܠܐ;
    uint32_t mask = hiWordMask();
    لكل (صحيح i = words() - 1; i >= 0; --i) {
        ثابت ValueAndX v = m_data.num()[i];
        ܐܢ ((v.m_value & v.m_valueX) ^ mask) ܦܢܐ ܕܓܠܐ;
        mask = ~0U;
    }
    ܦܢܐ ܫܪܪܐ;
}
منطقي V3Number::isEqZero() ثابت VL_MT_SAFE {
    ܐܢ (isString()) ܦܢܐ m_data.str().empty();
    لكل (صحيح i = 0; i < words(); ++i) {
        ثابت ValueAndX v = m_data.num()[i];
        ܐܢ (v.m_value || v.m_valueX) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}
منطقي V3Number::isNeqZero() ثابت {
    ܐܢ (isString()) ܦܢܐ !m_data.str().empty();
    لكل (صحيح i = 0; i < words(); ++i) {
        ثابت ValueAndX v = m_data.num()[i];
        ܐܢ (v.m_value & ~v.m_valueX) ܦܢܐ ܫܪܪܐ;
    }
    ܦܢܐ ܕܓܠܐ;
}
منطقي V3Number::isBitsZero(صحيح msb, صحيح lsb) ثابت {
    لكل (صحيح i = lsb; i <= msb; ++i) {
        ܐܢ (VL_UNLIKELY(!bitIs0(i))) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}
منطقي V3Number::isEqOne() ثابت {
    ܐܢ (m_data.num()[0].m_value != 1 || m_data.num()[0].m_valueX) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 1; i < words(); ++i) {
        ثابت ValueAndX v = m_data.num()[i];
        ܐܢ (v.m_value || v.m_valueX) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}
منطقي V3Number::isEqAllOnes(صحيح optwidth) ثابت {
    // Correct number of zero bits/width matters
    ܐܢ (!optwidth) optwidth = width();
    لكل (صحيح bit = 0; bit < optwidth; ++bit) {
        ܐܢ (!bitIs1(bit)) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}
منطقي V3Number::isFourState() ثابت VL_MT_SAFE {
    ܐܢ (isDouble() || isString()) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 0; i < words(); ++i) {
        ܐܢ (m_data.num()[i].m_valueX) ܦܢܐ ܫܪܪܐ;
    }
    ܦܢܐ ܕܓܠܐ;
}
منطقي V3Number::isAnyX() ثابت VL_MT_SAFE {
    ܐܢ (isDouble() || isString()) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 0; i < words(); ++i) {
        ثابت ValueAndX v = m_data.num()[i];
        ܐܢ (v.m_value & v.m_valueX) ܦܢܐ ܫܪܪܐ;
    }
    ܦܢܐ ܕܓܠܐ;
}
منطقي V3Number::isAnyXZ() ثابت {
    ܐܢ (isDouble() || isString()) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 0; i < words(); ++i) {
        ܐܢ (m_data.num()[i].m_valueX) ܦܢܐ ܫܪܪܐ;
    }
    ܦܢܐ ܕܓܠܐ;
}
منطقي V3Number::isAnyZ() ثابت VL_MT_SAFE {
    ܐܢ (isDouble() || isString()) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 0; i < words(); ++i) {
        ثابت ValueAndX v = m_data.num()[i];
        ܐܢ (~v.m_value & v.m_valueX) ܦܢܐ ܫܪܪܐ;
    }
    ܦܢܐ ܕܓܠܐ;
}
منطقي V3Number::isLtXZ(ثابت V3Number& rhs) ثابت {
    // Include X/Z in comparisons for sort ordering
    ܐܢ (isString()) ܦܢܐ toString() < rhs.toString();
    ܐܢ (isDouble()) ܦܢܐ toDouble() < rhs.toDouble();
    لكل (صحيح bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        ܐܢ (bitIs1(bit) && rhs.bitIs0(bit)) ܦܢܐ ܫܪܪܐ;
        ܐܢ (rhs.bitIs1(bit) && bitIs0(bit)) ܦܢܐ ܕܓܠܐ;
        ܐܢ (bitIsXZ(bit)) ܦܢܐ ܫܪܪܐ;
        ܐܢ (rhs.bitIsXZ(bit)) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܕܓܠܐ;
}
صحيح V3Number::countX(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
    صحيح count = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        ܐܢ (lsb + bitn >= width()) ܦܢܐ count;
        ܐܢ (bitIsX(lsb + bitn)) ++count;
    }
    ܦܢܐ count;
}
صحيح V3Number::countZ(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
    صحيح count = 0;
    لكل (صحيح bitn = 0; bitn < nbits; ++bitn) {
        ܐܢ (lsb + bitn >= width()) ܦܢܐ count;
        ܐܢ (bitIsZ(lsb + bitn)) ++count;
    }
    ܦܢܐ count;
}

صحيح V3Number::widthToFit() ثابت {
    لكل (صحيح bit = width() - 1; bit > 0; bit--) {
        ܐܢ (!bitIs0(bit)) ܦܢܐ bit + 1;
    }
    ܦܢܐ 1;  // one bit even if number is == 0
}

uint32_t V3Number::countBits(ثابت V3Number& ctrl) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = 0;
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        بدل (ctrl.bitIs(0)) {
        حالة '0':
            ܐܢ (bitIs0(bit)) ++n;
            اكسر;
        حالة '1':
            ܐܢ (bitIs1(bit)) ++n;
            اكسر;
        حالة 'x':
            ܐܢ (bitIsX(bit)) ++n;
            اكسر;
        حالة 'z':
            ܐܢ (bitIsZ(bit)) ++n;
            اكسر;
        }
    }
    ܦܢܐ n;
}

uint32_t V3Number::countBits(ثابت V3Number& ctrl1, ثابت V3Number& ctrl2,
                             ثابت V3Number& ctrl3) ثابت {
    // Correct number of zero bits/width matters
    صحيح n = countBits(ctrl1);
    ܐܢ (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += countBits(ctrl2);
    ܐܢ ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += countBits(ctrl3);
    }
    ܦܢܐ n;
}

uint32_t V3Number::countOnes() ثابت {
    صحيح n = 0;
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (bitIs1(bit)) ++n;
    }
    ܦܢܐ n;
}

uint32_t V3Number::mostSetBitP1() ثابت {
    لكل (صحيح bit = width() - 1; bit >= 0; bit--) {
        ܐܢ (!bitIs0(bit)) ܦܢܐ bit + 1;
    }
    ܦܢܐ 0;
}

uint32_t V3Number::leastSetBitP1() ثابت {
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (!bitIs0(bit)) ܦܢܐ bit + 1;
    }
    ܦܢܐ 0;
}

//======================================================================

V3Number& V3Number::opBitsNonXZ(ثابت V3Number& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opBitsOne(ثابت V3Number& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opBitsXZ(ثابت V3Number& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opBitsZ(ثابت V3Number& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    ܦܢܐ *هذا;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3Number& V3Number::opRedOr(ثابت V3Number& lhs) {
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit)) {
            ܦܢܐ setSingleBits(1);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(bit)) {
        } ܐܚܪܢܐ {
            outc = 'x';
        }
    }
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opRedAnd(ثابت V3Number& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        ܐܢ (lhs.bitIs0(bit)) {
            ܦܢܐ setSingleBits(0);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs1(bit)) {
        } ܐܚܪܢܐ {
            outc = 'x';
        }
    }
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opRedXor(ثابت V3Number& lhs) {
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit)) {
            ܐܢ (outc == 1) {
                outc = 0;
            } ܐܚܪܢܐ ܐܢ (outc == 0) {
                outc = 1;
            }
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(bit)) {
        } ܐܚܪܢܐ {
            outc = 'x';
        }
    }
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opCountBits(ثابت V3Number& expr, ثابت V3Number& ctrl1, ثابت V3Number& ctrl2,
                                ثابت V3Number& ctrl3) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS4(expr, ctrl1, ctrl2, ctrl3);
    NUM_ASSERT_LOGIC_ARGS4(expr, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.num()[0].m_value = expr.countBits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::opCountOnes(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    ܐܢ (lhs.isFourState()) ܦܢܐ setAllBitsX();
    setZero();
    m_data.num()[0].m_value = lhs.countOnes();
    opCleanThis();
    ܦܢܐ *هذا;
}
V3Number& V3Number::opIsUnknown(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    ܦܢܐ setSingleBits(lhs.isAnyXZ());
}
V3Number& V3Number::opOneHot(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    ܐܢ (lhs.isFourState()) ܦܢܐ setAllBitsX();
    ܦܢܐ setSingleBits(lhs.countOnes() == 1);
}
V3Number& V3Number::opOneHot0(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    ܐܢ (lhs.isFourState()) ܦܢܐ setAllBitsX();
    ܦܢܐ setSingleBits(lhs.countOnes() <= 1);
}
V3Number& V3Number::opCLog2(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    ܐܢ (lhs.isFourState()) ܦܢܐ setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ثابت صحيح adjust = (lhs.countOnes() == 1) ? 0 : 1;
    لكل (صحيح bit = lhs.width() - 1; bit >= 0; bit--) {
        ܐܢ (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            ܦܢܐ *هذا;
        }
    }
    setZero();
    ܦܢܐ *هذا;
}
V3Number& V3Number::opMostSetBitP1(ثابت V3Number& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    ܐܢ (lhs.isFourState()) ܦܢܐ setAllBitsX();
    لكل (صحيح bit = lhs.width() - 1; bit >= 0; bit--) {
        ܐܢ (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            ܦܢܐ *هذا;
        }
    }
    setZero();
    ܦܢܐ *هذا;
}

V3Number& V3Number::opLogNot(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit)) {
            outc = 0;
            اذهب last;
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(bit)) {
        } ܐܚܪܢܐ {
            outc = 'x';
        }
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opNot(ثابت V3Number& lhs) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opAnd(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } ܐܚܪܢܐ {
            setBit(bit, 'x');
        }
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opOr(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } ܐܚܪܢܐ {
            setBit(bit, 'x');
        }
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opXor(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ܐܚܪܢܐ ܐܢ (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    ܦܢܐ *هذا;
}

فراغ V3Number::fillBits(صحيح destLsb, صحيح width, حرف value) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    ܐܢ (!width) ܦܢܐ;
    UASSERT(width <= هذا->width() - destLsb, "Destination bit range exceeds number width");

    ثابت uint32_t valueWord = value == '1' || value == 'x' || value == 1 || value == 3 ? ~0U : 0;
    ثابت uint32_t xWord = value == 'z' || value == 'x' || value == 2 || value == 3 ? ~0U : 0;
    ثابت صحيح firstWord = VL_BITWORD_E(destLsb);
    ثابت صحيح lastWord = VL_BITWORD_E(destLsb + width - 1);
    لكل (صحيح word = firstWord; word <= lastWord; ++word) {
        ثابت صحيح wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ثابت صحيح wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        ثابت uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        ValueAndX& dest = m_data.num()[word];
        dest.m_value = (dest.m_value & ~mask) | (valueWord & mask);
        dest.m_valueX = (dest.m_valueX & ~mask) | (xWord & mask);
    }
}

فراغ V3Number::copyBits(صحيح destLsb, ثابت V3Number& source, صحيح sourceLsb, صحيح width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    ܐܢ (!width) ܦܢܐ;
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

        ثابت ValueAndX& sourceLo = source.m_data.num()[sourceWord];
        uint32_t value = sourceLo.m_value >> sourceOffset;
        uint32_t valueX = sourceLo.m_valueX >> sourceOffset;
        ܐܢ (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            ثابت ValueAndX& sourceHi = source.m_data.num()[sourceWord + 1];
            value |= sourceHi.m_value << (VL_EDATASIZE - sourceOffset);
            valueX |= sourceHi.m_valueX << (VL_EDATASIZE - sourceOffset);
        }

        ValueAndX& dest = m_data.num()[destWord];
        dest.m_value = (dest.m_value & ~mask) | ((value << destOffset) & mask);
        dest.m_valueX = (dest.m_valueX & ~mask) | ((valueX << destOffset) & mask);
    }
}

V3Number& V3Number::opConcat(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    ܐܢ (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    ܦܢܐ *هذا;
}

V3Number& V3Number::opLenN(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    ܦܢܐ *هذا;
}

V3Number& V3Number::opRepl(ثابت V3Number& lhs,
                           ثابت V3Number& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ܦܢܐ opRepl(lhs, rhs.toUInt());
}

V3Number& V3Number::opRepl(ثابت V3Number& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    ܐܢ (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    لكل (unsigned times = 0; times < rhsval; ++times) {
        ثابت uint64_t destLsb = uint64_t{times} * lhs.width();
        ܐܢ (destLsb >= static_cast<uint32_t>(width())) اكسر;
        copyBits(static_cast<صحيح>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<صحيح>(destLsb)));
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opStreamL(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    ܐܢ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ثابت صحيح ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    لكل (صحيح istart = 0; istart < lhs.width(); istart += ssize) {
        ثابت صحيح ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opLogAnd(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف loutc = 0;
    حرف routc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit)) {
            loutc = 1;
            اكسر;
        }
        ܐܢ (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    لكل (صحيح bit = 0; bit < rhs.width(); ++bit) {
        ܐܢ (rhs.bitIs1(bit)) {
            routc = 1;
            اكسر;
        }
        ܐܢ (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    حرف outc = 'x';
    ܐܢ (routc == 1 && loutc == 1) outc = 1;
    ܐܢ (routc == 0 || loutc == 0) outc = 0;
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opLogOr(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < lhs.width(); ++bit) {
        ܐܢ (lhs.bitIs1(bit)) {
            outc = 1;
            اذهب last;
        }
        ܐܢ (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    لكل (صحيح bit = 0; bit < rhs.width(); ++bit) {
        ܐܢ (rhs.bitIs1(bit)) {
            outc = 1;
            اذهب last;
        }
        ܐܢ (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opLogIf(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number lnot = lhs;
    lnot.opLogNot(lhs);
    ܦܢܐ opLogOr(lnot, rhs);
}

V3Number& V3Number::opLogEq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3Number ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ܦܢܐ opLogAnd(ifa, ifb);
}

V3Number& V3Number::opAtoN(ثابت V3Number& lhs, صحيح base) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    ܐܢ (base == AstAtoN::ATOREAL) ܦܢܐ setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    ܐܢ (errno != 0) v = 0;
    ܦܢܐ setLongS(static_cast<int32_t>(v));
}

V3Number& V3Number::opPutcN(ثابت V3Number& lhs, ثابت V3Number& rhs, ثابت V3Number& ths) {
    NUM_ASSERT_OP_ARGS3(lhs, rhs, ths);
    NUM_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    ثابت int32_t i = rhs.toSInt();
    ثابت int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    ܐܢ (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    ܦܢܐ setString(lstring);
}

V3Number& V3Number::opGetcN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.toString();
    ثابت int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    ܐܢ (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    ܦܢܐ setLong(v);
}

V3Number& V3Number::opSubstrN(ثابت V3Number& lhs, ثابت V3Number& rhs, ثابت V3Number& ths) {
    NUM_ASSERT_OP_ARGS3(lhs, rhs, ths);
    NUM_ASSERT_STRING_ARGS1(lhs);
    ثابت string lstring = lhs.toString();
    ثابت int32_t i = rhs.toSInt();
    ثابت int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    ܐܢ (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) ܦܢܐ setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ܦܢܐ setString(lstring.substr(i, j - i + 1));
}

V3Number& V3Number::opCompareNN(ثابت V3Number& lhs, ثابت V3Number& rhs, منطقي ignoreCase) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    صحيح result;
    ثابت string lstring = lhs.toString();
    ثابت string rstring = rhs.toString();
    ܐܢ (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } ܐܚܪܢܐ {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ܦܢܐ setLongS(result);
}

V3Number& V3Number::opEq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    ܐܢ (lhs.isString()) ܦܢܐ opEqN(lhs, rhs);
    ܐܢ (lhs.isDouble()) ܦܢܐ opEqD(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ܐܢ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            اذهب last;
        }
        ܐܢ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            اذهب last;
        }
        ܐܢ (lhs.bitIsXZ(bit)) outc = 'x';
        ܐܢ (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opNeq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    ܐܢ (lhs.isString()) ܦܢܐ opNeqN(lhs, rhs);
    ܐܢ (lhs.isDouble()) ܦܢܐ opNeqD(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ܐܢ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            اذهب last;
        }
        ܐܢ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            اذهب last;
        }
        ܐܢ (lhs.bitIsXZ(bit)) outc = 'x';
        ܐܢ (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Hash V3Number::toHash() ثابت {
    // Shouldmatch isCaseEq
    ܐܢ (isString()) ܦܢܐ V3Hash{toString()};
    ܐܢ (isDouble()) ܦܢܐ V3Hash{toDouble()};
    V3Hash hash{width()};
    لكل (صحيح i = 0; i < words(); ++i) hash += m_data.num()[i].m_value;
    ܦܢܐ hash;
}

منطقي V3Number::isCaseEq(ثابت V3Number& rhs) ثابت {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ܐܢ (isString()) ܦܢܐ toString() == rhs.toString();
    ܐܢ (isDouble()) ܦܢܐ toDouble() == rhs.toDouble();
    ܐܢ (width() != rhs.width()) ܦܢܐ ܕܓܠܐ;
    لكل (صحيح i = 0; i < words(); ++i) {
        ܐܢ (!(m_data.num()[i] == rhs.m_data.num()[i])) ܦܢܐ ܕܓܠܐ;
    }
    ܦܢܐ ܫܪܪܐ;
}

V3Number& V3Number::opCaseEq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3Number& V3Number::opCaseNeq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    حرف outc = 0;
    ܐܢ (lhs.isString()) {
        ܦܢܐ opNeqN(lhs, rhs);
    } ܐܚܪܢܐ ܐܢ (lhs.isDouble()) {
        ܦܢܐ opNeqD(lhs, rhs);
    }
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ܐܢ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            اذهب last;
        }
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opWildEq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 1;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ܐܢ (!rhs.bitIsXZ(bit)) {
            ܐܢ (lhs.bitIsXZ(bit)) {
                outc = 'x';
                اذهب last;
            }
            ܐܢ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                اذهب last;
            }
        }
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opWildNeq(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ܐܢ (!rhs.bitIsXZ(bit)) {
            ܐܢ (lhs.bitIsXZ(bit)) {
                outc = 'x';
                اذهب last;
            }
            ܐܢ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                اذهب last;
            }
        }
    }
last:
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opGt(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    لكل (صحيح bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ܐܢ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        ܐܢ (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        ܐܢ (lhs.bitIsXZ(bit)) outc = 'x';
        ܐܢ (rhs.bitIsXZ(bit)) outc = 'x';
    }
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opGtS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    حرف outc = 0;
    {
        ثابت صحيح mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        ܐܢ (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } ܐܚܪܢܐ ܐܢ (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } ܐܚܪܢܐ ܐܢ (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } ܐܚܪܢܐ {
            // both positive or negative, normal >
            لكل (صحيح bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                ܐܢ (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                ܐܢ (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                ܐܢ (lhs.bitIsXZ(bit)) outc = 'x';
                ܐܢ (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    ܦܢܐ setSingleBits(outc);
}

V3Number& V3Number::opGte(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number& eq = opEq(lhs, rhs);
    ܐܢ (eq.isNeqZero()) ܦܢܐ eq;  // Return true
    ܦܢܐ opGt(lhs, rhs);
}

V3Number& V3Number::opGteS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number& eq = opEq(lhs, rhs);
    ܐܢ (eq.isNeqZero()) ܦܢܐ eq;  // Return true
    ܦܢܐ opGtS(lhs, rhs);
}

V3Number& V3Number::opLt(ثابت V3Number& lhs, ثابت V3Number& rhs) { ܦܢܐ opGt(rhs, lhs); }
V3Number& V3Number::opLte(ثابت V3Number& lhs, ثابت V3Number& rhs) { ܦܢܐ opGte(rhs, lhs); }
V3Number& V3Number::opLtS(ثابت V3Number& lhs, ثابت V3Number& rhs) { ܦܢܐ opGtS(rhs, lhs); }
V3Number& V3Number::opLteS(ثابت V3Number& lhs, ثابت V3Number& rhs) { ܦܢܐ opGteS(rhs, lhs); }

V3Number& V3Number::opShiftR(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // L(lhs) bit return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (rhs.isFourState()) ܦܢܐ setAllBitsX();
    setZero();
    ܐܢ (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ܦܢܐ *هذا;
    ثابت uint32_t rhsval = rhs.toUInt();
    ܐܢ (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<صحيح>(rhsval),
                 std::min(width(), lhs.width() - static_cast<صحيح>(rhsval)));
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opShiftRS(ثابت V3Number& lhs, ثابت V3Number& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (rhs.isFourState()) ܦܢܐ setAllBitsX();
    setZero();
    ثابت منطقي overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    ܐܢ (!overflow) {
        ثابت uint32_t rhsval = rhs.toUInt();
        ܐܢ (rhsval < lbits) {
            ثابت صحيح copyWidth = std::min(width(), static_cast<صحيح>(lbits - rhsval));
            copyBits(0, lhs, static_cast<صحيح>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            ܦܢܐ *هذا;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    ܦܢܐ *هذا;
}

V3Number& V3Number::opShiftL(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // L(lhs) bit return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (rhs.isFourState()) ܦܢܐ setAllBitsX();
    setZero();
    ܐܢ (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ܦܢܐ *هذا;
    ثابت uint32_t rhsval = rhs.toUInt();
    ܐܢ (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<صحيح>(rhsval), lhs, 0,
                 std::min(width() - static_cast<صحيح>(rhsval), lhs.width()));
    }
    ܦܢܐ *هذا;
}

//======================================================================
// Ops - Arithmetic

V3Number& V3Number::opNegate(ثابت V3Number& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    ܐܢ (lhs.isFourState()) ܦܢܐ setAllBitsX();
    V3Number notlhs(&lhs, width());
    notlhs.opNot(lhs);
    ثابت V3Number one(&lhs, width(), 1);
    opAdd(notlhs, one);
    ܦܢܐ *هذا;
}
V3Number& V3Number::opAdd(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    لكل (صحيح word = 0; word < words(); ++word) {
        ثابت uint64_t lwordval = lhs.m_data.num()[word].m_value;
        ثابت uint64_t rwordval = rhs.m_data.num()[word].m_value;
        ثابت uint64_t sum = lwordval + rwordval + carry;
        m_data.num()[word].m_value = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ܦܢܐ *هذا;
}
V3Number& V3Number::opSub(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    V3Number negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    ܦܢܐ opAdd(lhs, negrhs);
}
V3Number& V3Number::opMul(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    setZero();
    ܐܢ (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } ܐܚܪܢܐ {
        لكل (صحيح lword = 0; lword < lhs.words(); ++lword) {
            ثابت uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.num()[lword].m_value);
            ܐܢ (lwordval == 0) استمر;
            لكل (صحيح rword = 0; rword < rhs.words(); ++rword) {
                ثابت uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.num()[rword].m_value);
                ܐܢ (rwordval == 0) استمر;
                uint64_t mul = lwordval * rwordval;
                لكل (صحيح qword = lword + rword; qword < words(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.num()[qword].m_value);
                    m_data.num()[qword].m_value = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    ܐܢ (mul == 0) اكسر;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opMulS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    V3Number lhsNoSign = lhs;
    ܐܢ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    ܐܢ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3Number qNoSign = opMul(lhsNoSign, rhsNoSign);
    ܐܢ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ܐܚܪܢܐ {
        opAssign(qNoSign);
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opDiv(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    ܐܢ (rhs.isEqZero()) ܦܢܐ setAllBitsXRemoved();
    ܐܢ (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        ܦܢܐ *هذا;
    } ܐܚܪܢܐ {
        // Wide division
        ܦܢܐ opModDivGuts(lhs, rhs, ܕܓܠܐ);
    }
}
V3Number& V3Number::opDivS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    ܐܢ (rhs.isEqZero()) ܦܢܐ setAllBitsXRemoved();
    V3Number lhsNoSign = lhs;
    ܐܢ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    ܐܢ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3Number qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    ܐܢ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ܐܚܪܢܐ {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *هذا);
    ܦܢܐ *هذا;
}
V3Number& V3Number::opModDiv(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    ܐܢ (rhs.isEqZero()) ܦܢܐ setAllBitsXRemoved();
    ܐܢ (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        ܦܢܐ *هذا;
    } ܐܚܪܢܐ {
        // Wide modulus
        ܦܢܐ opModDivGuts(lhs, rhs, ܫܪܪܐ);
    }
}
V3Number& V3Number::opModDivS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    ܐܢ (rhs.isEqZero()) ܦܢܐ setAllBitsXRemoved();
    V3Number lhsNoSign = lhs;
    ܐܢ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    ܐܢ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ثابت V3Number qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    ܐܢ (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } ܐܚܪܢܐ {
        opAssign(qNoSign);
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opModDivGuts(ثابت V3Number& lhs, ثابت V3Number& rhs, منطقي is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    ثابت صحيح lWords = lhs.words();
    ثابت صحيح umsbp1 = lhs.mostSetBitP1();  // dividend
    ثابت صحيح vmsbp1 = rhs.mostSetBitP1();  // divisor
    ܐܢ (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *هذا);
        ܦܢܐ *هذا;
    }

    ثابت صحيح uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ثابت صحيح vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    ܐܢ (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        لكل (صحيح j = uw - 1; j >= 0; j--) {
            ثابت uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.num()[j].m_value));
            m_data.num()[j].m_value = unw64 / static_cast<uint64_t>(rhs.m_data.num()[0].m_value);
            k = unw64
                - (static_cast<uint64_t>(m_data.num()[j].m_value)
                   * static_cast<uint64_t>(rhs.m_data.num()[0].m_value));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *هذا << " rem=0x" << std::hex
                                   << k << std::dec);
        ܐܢ (is_modulus) {
            setZero();
            m_data.num()[0].m_value = k;
        }
        opCleanThis();
        ܦܢܐ *هذا;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_WORDS + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_WORDS + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    لكل (صحيح i = 0; i < lWords; ++i) m_data.num()[i].m_value = 0;
    لكل (صحيح i = 0; i < lWords + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ثابت صحيح s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ثابت uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    لكل (صحيح i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.num()[i].m_value << s)
                | (shift_mask & (rhs.m_data.num()[i - 1].m_value >> (32 - s)));
    }
    vn[0] = rhs.m_data.num()[0].m_value << s;

    // Copy and shift dividend by same amount; may set new upper word
    ܐܢ (s) {
        un[uw] = lhs.m_data.num()[uw - 1].m_value >> (32 - s);
    } ܐܚܪܢܐ {
        un[uw] = 0;
    }
    لكل (صحيح i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.num()[i].m_value << s)
                | (shift_mask & (lhs.m_data.num()[i - 1].m_value >> (32 - s)));
    }
    un[0] = lhs.m_data.num()[0].m_value << s;

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
        ܐܢ (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            ܐܢ (rhat < 0x100000000ULL) اذهب again;
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
        m_data.num()[j].m_value = qhat;  // Save quotient digit

        ܐܢ (t < 0) {
            // Over subtracted; correct by adding back
            m_data.num()[j].m_value--;
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

    ܐܢ (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        لكل (صحيح i = 0; i < vw; ++i) {
            m_data.num()[i].m_value = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        لكل (صحيح i = vw; i < lWords; ++i) m_data.num()[i].m_value = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *هذا);
        ܦܢܐ *هذا;
    } ܐܚܪܢܐ {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *هذا);
        ܦܢܐ *هذا;
    }
}

V3Number& V3Number::opPow(ثابت V3Number& lhs, ثابت V3Number& rhs, منطقي lsign, منطقي rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ܐܢ (lhs.isFourState() || rhs.isFourState()) ܦܢܐ setAllBitsX();
    ܐܢ (rhs.isEqZero()) ܦܢܐ setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    ܐܢ (rsign && rhs.isNegative()) {
        ܐܢ (lhs.isEqZero()) {
            ܦܢܐ setAllBitsXRemoved();
        } ܐܚܪܢܐ ܐܢ (lhs.isEqOne()) {
            ܦܢܐ setValue1();
        } ܐܚܪܢܐ ܐܢ (lsign && lhs.isEqAllOnes()) {
            ܐܢ (rhs.bitIs1(0)) {
                ܦܢܐ setAllBits1();  // -1^odd=-1
            } ܐܚܪܢܐ {
                ܦܢܐ setValue1();  // -1^even=1
            }
        }
        ܦܢܐ setZero();
    }
    ܐܢ (lhs.isEqZero()) ܦܢܐ setZero();
    setZero();
    m_data.num()[0].m_value = 1;
    V3Number power(&lhs, width());
    power.opAssign(lhs);
    لكل (صحيح bit = 0; bit < rhs.width(); ++bit) {
        ܐܢ (bit > 0) {  // power = power*power
            V3Number lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        ܐܢ (rhs.bitIs1(bit)) {  // out *= power
            V3Number lastOut(&lhs, width());
            lastOut.opAssign(*هذا);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opPowSU(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Correct number of zero bits/width matters
    ܦܢܐ opPow(lhs, rhs, ܫܪܪܐ, ܕܓܠܐ);
}
V3Number& V3Number::opPowSS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Correct number of zero bits/width matters
    ܦܢܐ opPow(lhs, rhs, ܫܪܪܐ, ܫܪܪܐ);
}
V3Number& V3Number::opPowUS(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // Correct number of zero bits/width matters
    ܦܢܐ opPow(lhs, rhs, ܕܓܠܐ, ܫܪܪܐ);
}

V3Number& V3Number::opBufIf1(ثابت V3Number& ens, ثابت V3Number& if1s) {
    NUM_ASSERT_OP_ARGS2(ens, if1s);
    NUM_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    لكل (صحيح bit = 0; bit < width(); ++bit) {
        ܐܢ (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } ܐܚܪܢܐ {
            setBit(bit, 'z');
        }
    }
    ܦܢܐ *هذا;
}

// Sets all bits in range to the given value
V3Number& V3Number::opSetRange(uint32_t lsb, uint32_t width, حرف bitValue) {
    ܐܢ (lsb < static_cast<uint32_t>(هذا->width())) {
        fillBits(static_cast<صحيح>(lsb),
                 static_cast<صحيح>(std::min(width, static_cast<uint32_t>(هذا->width()) - lsb)),
                 bitValue);
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opAssign(ثابت V3Number& lhs) { ܦܢܐ opAssignNonXZ(lhs, ܕܓܠܐ); }
V3Number& V3Number::opAssignNonXZ(ثابت V3Number& lhs, منطقي ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    ܐܢ (هذا != &lhs) {
        ܐܢ (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = ܫܪܪܐ;
        } ܐܚܪܢܐ ܐܢ (isString()) {
            ܐܢ (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } ܐܚܪܢܐ {
                m_data.str() = lhs.m_data.str();
            }
        } ܐܚܪܢܐ ܐܢ (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } ܐܚܪܢܐ ܐܢ (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } ܐܚܪܢܐ ܐܢ (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } ܐܚܪܢܐ {
            لكل (صحيح word = 0; word < words(); ++word) {
                ثابت uint32_t value = word < lhs.words() ? lhs.m_data.num()[word].m_value
                                                                & ~lhs.m_data.num()[word].m_valueX
                                                          : 0;
                m_data.num()[word] = {value, 0};
            }
            opCleanThis();
        }
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opNToI(ثابت V3Number& lhs) {
    // String to packed number
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    setZero();
    ثابت string& str = lhs.toString();
    لكل (size_t n = 0; n < str.length(); ++n) {
        ثابت حرف c = str[str.length() - 1 - n];
        لكل (size_t cbit = 0; cbit < 8; ++cbit) {
            ثابت std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ܦܢܐ *هذا;
}

V3Number& V3Number::opExtendS(ثابت V3Number& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ثابت صحيح copyWidth = std::min(width(), static_cast<صحيح>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ܦܢܐ *هذا;
}

V3Number& V3Number::opExtendXZ(ثابت V3Number& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ثابت صحيح copyWidth = std::min(width(), static_cast<صحيح>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ܦܢܐ *هذا;
}

V3Number& V3Number::opClean(ثابت V3Number& lhs, uint32_t bits) { ܦܢܐ opSel(lhs, bits - 1, 0); }

فراغ V3Number::opCleanThis(منطقي warnOnTruncation) {
    // Clean MSB of number
    NUM_ASSERT_LOGIC_ARGS1(*هذا);
    ثابت ValueAndX v = m_data.num()[words() - 1];
    ثابت uint32_t newValueMsb = v.m_value & hiWordMask();
    ثابت uint32_t newValueXMsb = v.m_valueX & hiWordMask();
    ܐܢ (warnOnTruncation && (newValueMsb != v.m_value || newValueXMsb != v.m_valueX)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.num()[words() - 1] = {newValueMsb, newValueXMsb};
}

V3Number& V3Number::opSel(ثابت V3Number& lhs, ثابت V3Number& msb, ثابت V3Number& lsb) {
    NUM_ASSERT_OP_ARGS3(lhs, msb, lsb);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS2(msb, lsb);
    ܐܢ (lsb.isFourState() || msb.isFourState()) ܦܢܐ setAllBitsX();
    ܦܢܐ opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3Number& V3Number::opSel(ثابت V3Number& lhs, uint32_t msbval, uint32_t lsbval) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    صحيح copyWidth = 0;
    ܐܢ (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        ثابت uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<صحيح>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<صحيح>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ܦܢܐ *هذا;
}

V3Number& V3Number::opSelInto(ثابت V3Number& lhs, ثابت V3Number& lsb, صحيح width) {
    ܦܢܐ opSelInto(lhs, lsb.toSInt(), width);
}

V3Number& V3Number::opSelInto(ثابت V3Number& lhs, صحيح lsbval, صحيح width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ثابت صحيح copyWidth = std::max(0, std::min({width, lhs.width(), هذا->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    ثابت صحيح fillWidth
        = std::max(0, std::min(width - copyWidth, هذا->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    ܦܢܐ *هذا;
}

//======================================================================
// Ops - Floating point

V3Number& V3Number::opIToRD(ثابت V3Number& lhs, منطقي isSigned) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3Number noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    مزدوج d = 0;
    ثابت منطقي negate = isSigned && noxz.isNegative();
    ܐܢ (negate) {
        ثابت V3Number noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    لكل (صحيح bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        ܐܢ (noxz.bitIs1(bit)) d += exp2(bit);
    }
    ܐܢ (negate) d = -d;
    ܦܢܐ setDouble(d);
}
V3Number& V3Number::opRToIS(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    ثابت مزدوج v = VL_TRUNC(lhs.toDouble());
    ثابت int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ܦܢܐ setLongS(i);
}
V3Number& V3Number::opRToIRoundS(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
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
    ܐܢ (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        لكل (صحيح bit = 0; bit <= 52; ++bit) {
            ܐܢ (mantissa & (1ULL << bit)) {
                ثابت صحيح outbit = bit + lsb;
                ܐܢ (outbit >= 0) setBit(outbit, 1);
            }
        }
        ܐܢ (v < 0) {
            ثابت V3Number noSign = *هذا;
            opNegate(noSign);
        }
    }
    ܦܢܐ *هذا;
}
V3Number& V3Number::opRealToBits(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    ܦܢܐ setQuad(u.m_v);
}
V3Number& V3Number::opBitsToRealD(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    اتحاد {
        مزدوج m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    ܦܢܐ setDouble(u.m_d);
}
V3Number& V3Number::opNegateD(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    ܦܢܐ setDouble(-lhs.toDouble());
}
V3Number& V3Number::opAddD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setDouble(lhs.toDouble() + rhs.toDouble());
}
V3Number& V3Number::opSubD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setDouble(lhs.toDouble() - rhs.toDouble());
}
V3Number& V3Number::opMulD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setDouble(lhs.toDouble() * rhs.toDouble());
}
V3Number& V3Number::opDivD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setDouble(lhs.toDouble() / rhs.toDouble());
}
V3Number& V3Number::opPowD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3Number& V3Number::opEqD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3Number& V3Number::opNeqD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3Number& V3Number::opGtD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3Number& V3Number::opGteD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3Number& V3Number::opLtD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3Number& V3Number::opLteD(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3Number& V3Number::opConcatN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setString(lhs.toString() + rhs.toString());
}
V3Number& V3Number::opReplN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_STRING_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(rhs);
    ܦܢܐ opReplN(lhs, rhs.toUInt());
}
V3Number& V3Number::opReplN(ثابت V3Number& lhs, uint32_t rhsval) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    لكل (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    ܦܢܐ setString(out);
}
V3Number& V3Number::opCvtPackString(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    ܦܢܐ setString(lhs.toString());
}
V3Number& V3Number::opToLowerN(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string out = VString::downcase(lhs.toString());
    ܦܢܐ setString(out);
}
V3Number& V3Number::opToUpperN(ثابت V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    ثابت std::string out = VString::upcase(lhs.toString());
    ܦܢܐ setString(out);
}

V3Number& V3Number::opEqN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toString() == rhs.toString());
}
V3Number& V3Number::opNeqN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toString() != rhs.toString());
}
V3Number& V3Number::opGtN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toString() > rhs.toString());
}
V3Number& V3Number::opGteN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toString() >= rhs.toString());
}
V3Number& V3Number::opLtN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toString() < rhs.toString());
}
V3Number& V3Number::opLteN(ثابت V3Number& lhs, ثابت V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ܦܢܐ setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

فراغ V3Number::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    FileLine* ثابت fl = جديد FileLine{FileLine::builtInFilename()};
    V3Number num{fl, 32, 0};
    num.selfTestThis();
}

فراغ V3Number::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3Number::epsilonEqual(0, 0), ܫܪܪܐ);
    UASSERT_SELFTEST(V3Number::epsilonEqual(1e19, 1e19), ܫܪܪܐ);
    UASSERT_SELFTEST(V3Number::epsilonEqual(9, 0.0001), ܕܓܠܐ);
    UASSERT_SELFTEST(V3Number::epsilonEqual(1, 1 + std::numeric_limits<مزدوج>::epsilon()), ܫܪܪܐ);
    UASSERT_SELFTEST(V3Number::epsilonEqual(0.009, 0.00899999999999999931998839741709), ܫܪܪܐ);

    UASSERT_SELFTEST(V3Number::epsilonIntegral(0), ܫܪܪܐ);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(1), ܫܪܪܐ);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(-1), ܫܪܪܐ);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(1.0001), ܕܓܠܐ);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(0.9999), ܕܓܠܐ);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(-1.0001), ܕܓܠܐ);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(-0.9999), ܕܓܠܐ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
