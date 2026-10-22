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

#include "V3PchAstMT.h"

#include "V3Number.h"

#include "V3File.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr số_nguyên MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define NUM_ASSERT_OP_ARGS1(arg1) \
    UASSERT((this != &(arg1)), "Number operation called with same source and dest")
#define NUM_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((this != &(arg1) && this != &(arg2)), \
            "Number operation called with same source and dest")
#define NUM_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((this != &(arg1) && this != &(arg2) && this != &(arg3)), \
            "Number operation called with same source and dest")
#define NUM_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((this != &(arg1) && this != &(arg2) && this != &(arg3) && this != &(arg4)), \
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

void V3Number::v3errorEnd(const std::ostringstream& str) const VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    nếu (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } không_thì nếu (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } không_thì {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

void V3Number::v3errorEndFatal(const std::ostringstream& str) const
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    assert(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3Number::V3Number(AstNode* nodep, VerilogStringLiteral, const string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    nếu (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } không_thì {
        init(nodep, std::max<số_nguyên>(str.length() * 8, 1));
        cho (unsigned pos = 0; pos < str.length(); ++pos) {
            const số_nguyên topos = str.length() - 1 - pos;
            ValueAndX& v = m_data.num()[topos / 4];
            cho (số_nguyên bit = 0; bit < 8; ++bit) {
                nếu (str[pos] & (1UL << bit)) v.m_value |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = true;
    opCleanThis(true);
}

V3Number::V3Number(AstNode* nodep, const AstNodeDType* nodedtypep) {
    nếu (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } không_thì nếu (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } không_thì nếu (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } không_thì {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

void V3Number::create(const char* sourcep) {
    m_data.setLogic();
    const char* value_startp = sourcep;
    cho (const char* cp = sourcep; *cp; ++cp) {
        nếu (*cp == '\'') {
            value_startp = cp + 1;
            break;
        }
    }

    bool userSized = false;
    bool unbased = false;
    char base = '\0';
    nếu (value_startp != sourcep) {  // Has a '
        string widthn;
        const char* cp = sourcep;
        cho (; *cp; ++cp) {
            nếu (*cp == '\'') {
                ++cp;
                break;
            }
            nếu (*cp != '_') widthn += *cp;
        }
        trong_khi (*cp == '_') cp++;
        nếu (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(true);
        }
        nếu (*cp) {
            base = *cp;
            ++cp;
        }
        value_startp = cp;

        userSized = widthn.length() != 0;
        nếu (std::atoi(widthn.c_str())) {
            nếu (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.maxNumWidth()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.maxNumWidth(), true);
            } không_thì {
                width(std::atoi(widthn.c_str()), true);
            }
        }
    } không_thì {
        unbased = true;
        base = 'd';
    }

    cho (số_nguyên i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    nếu (base == '0') {
        width(1, false);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = true;
    } không_thì nếu (base == '1') {
        width(1, false);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = true;
    } không_thì nếu (std::tolower(base) == 'z') {
        width(1, false);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = true;
    } không_thì nếu (std::tolower(base) == 'x') {
        width(1, false);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = true;
    }
    // Otherwise...
    không_thì nếu (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        const số_nguyên widthBound = std::max<số_nguyên>(32, std::strlen(value_startp) * 4);
        width(widthBound, false);  // Will change width below
        nếu (unbased) isSigned(true);  // Also says the spec.
    }

    // Ignore leading blanks
    trong_khi (*value_startp == '_' || std::isspace(*value_startp)) ++value_startp;
    nếu (!*value_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    nếu (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    số_nguyên obit = 0;  // Start at LSB
    số_nguyên base_align = 1;
    nếu (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        trong_khi (*value_startp == '_' || *value_startp == '0') ++value_startp;
        // Convert decimal number to hex
        số_nguyên olen = 0;
        uint32_t val = 0;
        số_nguyên got_x = 0;
        số_nguyên got_z = 0;
        số_nguyên got_01 = 0;
        bool warnedTooMany = false;
        cho (const char* cp = value_startp; *cp; ++cp) {
            switch (std::tolower(*cp)) {
            case '0':  // FALLTHRU
            case '1':  // FALLTHRU
            case '2':  // FALLTHRU
            case '3':  // FALLTHRU
            case '4':  // FALLTHRU
            case '5':  // FALLTHRU
            case '6':  // FALLTHRU
            case '7':  // FALLTHRU
            case '8':  // FALLTHRU
            case '9': {
                nếu (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.num()[0].m_value = val;
                } không_thì {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3Number product(this, width() + 4);  // +4 for overflow detection
                    const V3Number ten(this, width() + 4, 10);
                    const V3Number addend(this, width(), (*cp - '0'));
                    product.opMul(*this, ten);
                    opAdd(product, addend);
                    nếu (!warnedTooMany && product.bitsValue(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = true;
                    }
                }
                ++olen;
                got_01 = 1;
                break;
            }
            case 'z':
            case '?': {
                got_z = 1;
                nếu (!userSized) width(32, false);
                setAllBitsZ();
                break;
            }
            case 'x': {
                got_x = 1;
                nếu (!userSized) width(32, false);
                setAllBitsX();
                break;
            }
            case '_': break;
            default: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                break;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        nếu ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << value_startp);
        }
    } không_thì {
        // Convert bin/octal number to hex
        cho (const char* cp = value_startp + std::strlen(value_startp) - 1; cp >= value_startp;
             cp--) {
            nếu (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                break;
            }
            switch (std::tolower(base)) {
            case 'b': {
                base_align = 1;
                switch (std::tolower(*cp)) {
                case '0': setBit(obit++, 0); break;
                case '1': setBit(obit++, 1); break;
                case 'z':
                case '?': setBit(obit++, 'z'); break;
                case 'x': setBit(obit++, 'x'); break;
                case '_': break;
                default: v3error("Illegal character in binary constant: " << *cp);
                }
                break;
            }

            case 'o':
            case 'c': {
                base_align = 3;
                switch (std::tolower(*cp)) {  // clang-format off
                case '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  break;
                case '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  break;
                case '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  break;
                case '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  break;
                case '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  break;
                case '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  break;
                case '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  break;
                case '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  break;
                case 'z':
                case '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); break;
                case 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); break;
                case '_': break;
                // clang-format on
                default: v3error("Illegal character in octal constant");
                }
                break;
            }

            case 'h':  // FALLTHRU
            case 'x': {
                base_align = 4;
                switch (std::tolower(*cp)) {  // clang-format off
                case '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); break;
                case '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); break;
                case '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); break;
                case '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); break;
                case '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); break;
                case '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); break;
                case '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); break;
                case '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); break;
                case '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); break;
                case '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); break;
                case 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); break;
                case 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); break;
                case 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); break;
                case 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); break;
                case 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); break;
                case 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); break;
                case 'z':
                case '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); break;
                case 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); break;
                    // clang-format on
                case '_': break;
                default:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    break;
                }  // LCOV_EXCL_STOP
                break;
            }
            default:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                break;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    nếu (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), false);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    trong_khi (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(true);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

void V3Number::warnTooMany(const string& value) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << value << "'");
}

void V3Number::nodep(AstNode* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    nếu (!nodep) trả_về;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

bool V3Number::epsilonEqual(double a, double b) {
    trả_về std::fabs(a - b)
           <= (std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(a, b)));
}

bool V3Number::epsilonIntegral(double a) {
    const double dint = static_cast<double>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    trả_về epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

số_nguyên V3Number::log2b(uint32_t num) {
    // See also opCLog2
    cho (số_nguyên bit = 31; bit > 0; bit--) {
        nếu (num & (1ULL << bit)) trả_về bit;
    }
    trả_về 0;
}

số_nguyên V3Number::log2bQuad(uint64_t num) {
    nếu (num >> 32ULL) trả_về 32 + log2b(num >> 32ULL);
    trả_về log2b(num);
}

//======================================================================
// Setters

V3Number& V3Number::setZero() {
    cho (số_nguyên i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};
    trả_về *this;
}
V3Number& V3Number::setQuad(uint64_t value) {
    setZero();
    m_data.num()[0].m_value = value & 0xffffffffULL;
    nếu (width() > 32) m_data.num()[1].m_value = (value >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::setLong(uint32_t value) {
    setZero();
    m_data.num()[0].m_value = value;
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::setLongS(int32_t value) {
    setZero();
    union {
        uint32_t u;
        int32_t s;
    } u;
    u.s = value;
    (void)u.s;
    m_data.num()[0].m_value = u.u;
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::setDouble(double value) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    union {
        double d;
        uint32_t u[2];
    } u;
    u.d = value;
    (void)u.d;
    cho (số_nguyên i = 2; i < words(); ++i) m_data.num()[i] = {0, 0};
    m_data.num()[0].m_value = u.u[0];
    m_data.num()[1].m_value = u.u[1];
    trả_về *this;
}
V3Number& V3Number::setSingleBits(char value) {
    cho (số_nguyên i = 1 /*upper*/; i < words(); ++i) m_data.num()[i] = {0, 0};
    m_data.num()[0] = {(value == '1' || value == 'x' || value == 1 || value == 3),
                       (value == 'z' || value == 'x' || value == 2 || value == 3)};
    trả_về *this;
}

V3Number& V3Number::setAllBits0() {
    setZero();
    trả_về *this;
}
V3Number& V3Number::setAllBits1() {
    cho (số_nguyên i = 0; i < words(); ++i) m_data.num()[i] = {~0U, 0};
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    cho (số_nguyên i = 0; i < words(); ++i) m_data.num()[i] = {~0U, ~0U};
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::setAllBitsZ() {
    cho (số_nguyên i = 0; i < words(); ++i) m_data.num()[i] = {0, ~0U};
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::setAllBitsXRemoved() {
    nếu (!v3Global.constRemoveXs()) {
        trả_về setAllBitsX();
    } không_thì {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        nếu (v3Global.opt.xAssign() == "1") {
            trả_về setAllBits1();
        } không_thì {
            trả_về setAllBits0();
        }
    }
}
V3Number& V3Number::setValue1() {
    m_data.num()[0] = {1, 0};
    cho (số_nguyên i = 1; i < words(); ++i) m_data.num()[i] = {0, 0};
    trả_về *this;
}

void V3Number::setBitX0(số_nguyên bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3Number& V3Number::setMask(số_nguyên nbits, số_nguyên lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    trả_về *this;
}

//======================================================================
// ACCESSORS - as strings

string V3Number::ascii(bool prefixed, bool cleanVerilog) const VL_MT_STABLE {
    // Correct number of zero bits/width matters
    nếu (is1Step()) trả_về "1step";
    nếu (isNull()) trả_về "null";
    std::ostringstream out;
    nếu (isDouble()) {
        out.precision(17);
        nếu (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } không_thì {
            out << toDouble();
            nếu (toDouble() == floor(toDouble())) out << ".0";
        }
        trả_về out.str();
    } không_thì nếu (isString()) {
        trả_về '"' + toString() + '"';
    } không_thì {
        nếu (VL_UNCOVERABLE((m_data.num()[words() - 1].m_value | m_data.num()[words() - 1].m_valueX)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    nếu (prefixed) {
        nếu (sized()) {
            out << width() << "'";
        } không_thì nếu (autoExtend() && width() == 1) {
            out << "'";
            nếu (bitIs0(0)) {
                out << '0';
            } không_thì nếu (bitIs1(0)) {
                out << '1';
            } không_thì nếu (bitIsZ(0)) {
                out << 'z';
            } không_thì {
                out << 'x';
            }
            trả_về out.str();
        } không_thì {
            nếu (cleanVerilog) {
                out << "'";
            } không_thì {
                out << "?" << width() << "?";
            }
        }
        nếu (isSigned()) out << "s";
    }

    const bool binary = (isFourState()
#ifdef V3NUMBER_ASCII_BINARY
                         // cppcheck-suppress konwnConditionTrueFalse
                         || true
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    nếu (binary) {
        out << "b";
        out << displayed("%0b");
    } không_thì {
        nếu (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    nếu (isNull()) {
        nếu (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } không_thì {
            out << " [null]";
        }
    }
    trả_về out.str();
}

bool V3Number::displayedFmtHasArg(char format, bool isScan) {
    // Is this a format letter that takes an argument in runtime?
    (void)isScan;
    switch (std::tolower(format)) {
    case 'b': trả_về true;  // Binary
    case 'c': trả_về true;  // Character
    case 'd': trả_về true;  // Decimal
    case 'e': trả_về true;  // Floating
    case 'f': trả_về true;  // Floating
    case 'g': trả_về true;  // Floating
    case 'h': trả_về true;  // Hex
    case 'o': trả_về true;  // Octal
    case 'p': trả_về true;  // Pattern
    case 's': trả_về true;  // String
    case 't': trả_về true;  // Time
    case 'u': trả_về true;  // Packed 2-state
    case 'v': trả_về true;  // Strength
    case 'x': trả_về true;  // Hex
    case 'z': trả_về true;  // Packed 4-state
    case '?': trả_về true;  // Internal: Compute format in V3Width based on data type
    default: trả_về false;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3Number::displayPad(size_t fmtsize, char pad, bool left, const string& in) VL_PURE {
    string padding;
    nếu (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    trả_về left ? (in + padding) : (padding + in);
}

string V3Number::displayedEnumName(const AstEnumItem* itemp) {
    const string name = VIdProtect::protect(itemp->prettyName());
    trả_về VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned char>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3Number::displayedEnumName(const AstEnumDType* dtypep) const VL_MT_STABLE {
    V3Number value{this, dtypep->width()};
    value.opAssign(*this);
    cho (const AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        const AstConst* const constp = VN_AS(itemp->valuep(), Const);
        nếu (!constp->num().isAnyXZ() && value.isCaseEq(constp->num()))
            trả_về displayedEnumName(itemp);
    }
    trả_về "";
}

string V3Number::displayedEnum(const AstSFormatArg* argp,
                               const string& vformat) const VL_MT_STABLE {
    const AstEnumDType* const dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3Number value{this, dtypep->width()};
    value.opAssign(*this);
    const VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    const char fmt = std::tolower(vformat.back());
    nếu (fmt != 'p' && fmt != 's') trả_về value.displayed(argp, vformat, numericAttr);
    string name = value.displayedEnumName(dtypep);
    nếu (name.empty()) name = value.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3Number nameNum{this};
    nameNum.setString(name);
    trả_về nameNum.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3Number::displayed(const AstNode* nodep, const string& vformat,
                           const VFormatAttr& formatAttr) const VL_MT_STABLE {
    trả_về displayed(nodep->fileline(), vformat, formatAttr);
}

string V3Number::displayed(FileLine* fl, const string& vformat,
                           const VFormatAttr& formatAttr) const VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *this);
    ++pos;
    bool left = false;
    nếu (pos[0] == '-') {
        left = true;
        ++pos;
    }
    string fmtsize;
    cho (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    char fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    nếu (formatAttr.isComplex()) {
        nếu (fmt != 'p') fmt = 's';  // override
    } không_thì nếu (formatAttr.isString()) {
        nếu (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3Number strwide{fl, static_cast<số_nguyên>(toString().length()) * 8, 0};
            strwide.opNToI(*this);
            trả_về strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        nếu (fmt != 'p') fmt = 's';  // override
    } không_thì nếu (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        nếu (fmt == 'p') {
            nếu (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } không_thì {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } không_thì nếu (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        nếu (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3Number nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*this);
            trả_về nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    switch (fmt) {
    case 'c': {
        nếu (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        const unsigned số_nguyên v = bitsValue(0, 8);
        str.push_back(static_cast<char>(v));
        trả_về str;
    }
    case 'e':  // FALLTHRU
    case 'f':  // FALLTHRU
    case 'g': {
        const double n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        char tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (void)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        trả_về tmp;
    }
    case 's': {
        nếu (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } không_thì {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            số_nguyên bit = width() - 1;
            bool start = true;
            trong_khi ((bit % 8) != 7) ++bit;
            cho (; bit >= 0; bit -= 8) {
                const số_nguyên v = bitsValue(bit - 7, 8);
                nếu (!start || v) {
                    str += static_cast<char>((v == 0) ? ' ' : v);
                    start = false;  // Drop leading 0s
                } không_thì {
                    nếu (fmtsize != "0") str += ' ';
                }
            }
        }
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', left, str);
        trả_về str;
    }
    case 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        nếu (formatAttr.isDouble()) {
            const double n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            char tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (void)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            trả_về tmp;
        }
        nếu (formatAttr.isString())
            trả_về '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        nếu (formatAttr.isComplex()) trả_về toString();
        trả_về "%p";
    }
    case 'b':  // FALLTHRU
    case 'o':  // FALLTHRU
    case 'h':  // FALLTHRU
    case 'x': {
        số_nguyên bit = width() - 1;
        nếu (left || !fmtsize.empty()) {
            trong_khi (bit && bitIs0(bit)) --bit;
        }
        switch (fmt) {
        case 'b': {
            cho (; bit >= 0; --bit) {
                nếu (bitIs0(bit)) {
                    str += '0';
                } không_thì nếu (bitIs1(bit)) {
                    str += '1';
                } không_thì nếu (bitIsZ(bit)) {
                    str += 'z';
                } không_thì {
                    str += 'x';
                }
            }
            break;
        }
        case 'o': {
            trong_khi ((bit % 3) != 2) ++bit;
            cho (; bit > 0; bit -= 3) {
                const số_nguyên numX = countX(bit - 2, 3);
                const số_nguyên numZ = countZ(bit - 2, 3);
                nếu (numX == 3 || numX == width() - (bit - 2)) {
                    str += 'x';
                    continue;
                }
                nếu (numZ == 3 || numZ == width() - (bit - 2)) {
                    str += 'z';
                    continue;
                }
                nếu (numX > 0) {
                    str += 'X';
                    continue;
                }
                nếu (numZ > 0) {
                    str += 'Z';
                    continue;
                }
                const số_nguyên v = bitsValue(bit - 2, 3);
                str += static_cast<char>('0' + v);
            }
            break;
        }
        default: {  // h/x
            trong_khi ((bit % 4) != 3) ++bit;
            cho (; bit > 0; bit -= 4) {
                const số_nguyên numX = countX(bit - 3, 4);
                const số_nguyên numZ = countZ(bit - 3, 4);
                nếu (numX == 4 || numX == width() - (bit - 3)) {
                    str += 'x';
                    continue;
                }
                nếu (numZ == 4 || numZ == width() - (bit - 3)) {
                    str += 'z';
                    continue;
                }
                nếu (numX > 0) {
                    str += 'X';
                    continue;
                }
                nếu (numZ > 0) {
                    str += 'Z';
                    continue;
                }
                const số_nguyên v = bitsValue(bit - 3, 4);
                nếu (v >= 10) {
                    str += static_cast<char>('a' + v - 10);
                } không_thì {
                    str += static_cast<char>('0' + v);
                }
            }
            break;
        }
        }  // switch
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (left ? ' ' : '0'), left, str);
        trả_về str;
    }  // case b/d/x/o
    case 't':  // Time
    case 'd': {  // Unsigned decimal
        const bool issigned = formatAttr.isSigned();
        nếu (fmtsize == "" && !left) {
            const double mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            double dchars = mantissabits / 3.321928094887362 + 1.0;
            nếu (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<số_nguyên>(dchars));
        }
        bool hasXZ = false;
        nếu (isAllX()) {
            str = "x";
            hasXZ = true;
        } không_thì nếu (isAllZ()) {
            str = "z";
            hasXZ = true;
        } không_thì nếu (isAnyX()) {
            str = "X";
            hasXZ = true;
        } không_thì nếu (isAnyZ()) {
            str = "Z";
            hasXZ = true;
        }
        nếu (!hasXZ) {
            nếu (issigned) {
                nếu (width() > 64) {
                    str = toDecimalS();
                } không_thì {
                    str = cvtToStr(toSQuad());
                }
            } không_thì {
                nếu (width() > 64) {
                    str = toDecimalU();
                } không_thì {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        const bool zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !left;
        // fmtsize might have changed since we parsed the %fmtsize
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), left, str);
        trả_về str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    case 'u': {  // Packed 2-state
        cho (số_nguyên i = 0; i < words(); ++i) {
            const uint32_t v = m_data.num()[i].m_value;
            str += static_cast<char>((v >> 0) & 0xff);
            str += static_cast<char>((v >> 8) & 0xff);
            str += static_cast<char>((v >> 16) & 0xff);
            str += static_cast<char>((v >> 24) & 0xff);
        }
        trả_về str;
    }
    case 'z': {  // Packed 4-state
        cho (số_nguyên i = 0; i < words(); ++i) {
            const ValueAndX v = m_data.num()[i];
            str += static_cast<char>((v.m_value >> 0) & 0xff);
            str += static_cast<char>((v.m_value >> 8) & 0xff);
            str += static_cast<char>((v.m_value >> 16) & 0xff);
            str += static_cast<char>((v.m_value >> 24) & 0xff);
            str += static_cast<char>((v.m_valueX >> 0) & 0xff);
            str += static_cast<char>((v.m_valueX >> 8) & 0xff);
            str += static_cast<char>((v.m_valueX >> 16) & 0xff);
            str += static_cast<char>((v.m_valueX >> 24) & 0xff);
        }
        trả_về str;
    }
    case 'v': {  // Strength
        số_nguyên bit = width() - 1;
        cho (; bit >= 0; bit--) {
            nếu (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            không_thì nếu (bitIs1(bit)) {
                str += "St1 ";
            } không_thì nếu (bitIsZ(bit)) {
                str += "StZ ";
            } không_thì {
                str += "StX";
            }
        }
        trả_về str;
    }
    default: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3Number::emitC() const VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    char sbuf[bufsize];
    string result;
    nếu (isNull()) {
        trả_về "VlNull{}";
    } không_thì nếu (isDouble()) {
        const double dnum = toDouble();
        nếu (std::isinf(dnum)) {
            nếu (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } không_thì nếu (std::isnan(dnum)) {
            nếu (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } không_thì {
            const char* const fmt = (static_cast<số_nguyên>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (void)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            trả_về sbuf;
        }
    } không_thì nếu (isString()) {
        const string strg = toString();
        const string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        trả_về '"' + quoted + "\"s";
    } không_thì nếu (words() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(words()) + ">{{";
        nếu (words() > 4) result += '\n';
        cho (số_nguyên n = 0; n < words(); ++n) {
            nếu (n) result += ((n % 4) ? ", " : ",\n");
            (void)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        nếu (words() > 4) result += '\n';
        result += "}}";
    } không_thì nếu (words() == 2) {  // Quad
        const uint64_t qnum = static_cast<uint64_t>(toUQuad());
        const char* const fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (void)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        trả_về sbuf;
    } không_thì {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        const uint32_t unum = toUInt();
        const char* const fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (void)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        trả_về sbuf;
    }
    trả_về result;
}

string V3Number::toDecimalS() const VL_MT_STABLE {
    // Correct number of zero bits/width matters
    nếu (isNegative()) {
        V3Number lhsNoSign = *this;
        lhsNoSign.opNegate(*this);
        trả_về "-"s + lhsNoSign.toDecimalU();
    } không_thì {
        trả_về toDecimalU();
    }
}

string V3Number::toDecimalU() const VL_MT_STABLE {
    const số_nguyên maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3Number bcd(this, maxdecwidth + 4);
    V3Number tmp(this, maxdecwidth + 4);
    V3Number tmp2(this, maxdecwidth + 4);

    số_nguyên from_bit = width() - 1;
    // Skip all leading zeros
    cho (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    cho (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        cho (số_nguyên nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            nếu (bcd.bitsValue(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3Number(this, 32, 1));
        // bcd[0] = this[from_bit]
        nếu (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    số_nguyên lsb = (maxdecwidth - 1) & ~3;
    cho (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        nếu (bcd.bitsValue(lsb, 4)) break;
    }
    cho (; lsb >= 0; lsb -= 4) {
        output += static_cast<char>('0' + bcd.bitsValue(lsb, 4));  // 0..9
    }
    trả_về output;
}

string V3Number::toRealString() const VL_MT_STABLE {
    const double value = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << value;
    string result = out.str();
    nếu (std::strtod(result.c_str(), nullptr) != value) {
        out.str("");
        out.precision(17);
        out << value;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    nếu (result.find_first_of(".en") == string::npos) result += ".0";
    trả_về result;
}

//======================================================================
// ACCESSORS - as numbers

bool V3Number::fitsInUInt() const VL_MT_SAFE {
    nếu (isFourState()) trả_về false;
    // We allow wide numbers that represent values <= 32 bits
    cho (số_nguyên i = 1; i < words(); ++i) {
        nếu (m_data.num()[i].m_value) trả_về false;
    }
    trả_về true;
}
uint32_t V3Number::toUInt() const VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *this);
    // We allow wide numbers that represent values <= 32 bits
    nếu (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *this);
    }
    trả_về m_data.num()[0].m_value;
}

double V3Number::toDouble() const VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    union {
        double d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.num()[0].m_value;
    u.u[1] = m_data.num()[1].m_value;
    trả_về u.d;
}

int32_t V3Number::toSInt() const VL_MT_SAFE {
    nếu (isSigned()) {
        const uint32_t v = toUInt();
        const uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        const uint32_t extended = v | signExtend;
        trả_về static_cast<int32_t>(extended);
    } không_thì {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        trả_về static_cast<int32_t>(toUInt());
    }
}

uint64_t V3Number::toUQuad() const VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *this);
    // We allow wide numbers that represent values <= 64 bits
    nếu (isDouble()) trả_về static_cast<uint64_t>(toDouble());
    cho (số_nguyên i = 2; i < words(); ++i) {
        nếu (m_data.num()[i].m_value) {
            v3error("Value too wide for 64-bits expected in this context " << *this);
            break;
        }
    }
    nếu (width() <= 32) trả_về (static_cast<uint64_t>(toUInt()));
    trả_về ((static_cast<uint64_t>(m_data.num()[1].m_value) << 32ULL)
            | (static_cast<uint64_t>(m_data.num()[0].m_value)));
}

int64_t V3Number::toSQuad() const {
    nếu (isDouble()) trả_về static_cast<int64_t>(toDouble());
    const uint64_t v = toUQuad();
    const uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    const uint64_t extended = v | signExtend;
    trả_về static_cast<int64_t>(extended);
}

string V3Number::toString() const {
    UASSERT(!isFourState(), "toString with 4-state " << *this);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    nếu (isString()) trả_về m_data.str();
    số_nguyên bit = width() - 1;
    trong_khi ((bit % 8) != 7) ++bit;
    std::string str;
    cho (; bit >= 0; bit -= 8) {
        const số_nguyên v = bitsValue(bit - 7, 8);
        nếu (v) str += static_cast<char>(v);
    }
    trả_về str;
}

uint32_t V3Number::edataWord(số_nguyên eword) const {
    UASSERT(!isFourState(), "edataWord with 4-state " << *this);
    trả_về m_data.num()[eword].m_value;
}

uint8_t V3Number::dataByte(số_nguyên byte) const {
    trả_về (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

bool V3Number::isAllZ() const VL_MT_SAFE {
    nếu (isDouble() || isString()) trả_về false;
    cho (số_nguyên i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        const uint32_t mask = i == words() - 1 ? hiWordMask() : ~0U;
        nếu (((~v.m_value & v.m_valueX) & mask) != mask) trả_về false;
    }
    trả_về true;
}
bool V3Number::isAllX() const VL_MT_SAFE {
    nếu (isDouble() || isString()) trả_về false;
    uint32_t mask = hiWordMask();
    cho (số_nguyên i = words() - 1; i >= 0; --i) {
        const ValueAndX v = m_data.num()[i];
        nếu ((v.m_value & v.m_valueX) ^ mask) trả_về false;
        mask = ~0U;
    }
    trả_về true;
}
bool V3Number::isEqZero() const VL_MT_SAFE {
    nếu (isString()) trả_về m_data.str().empty();
    cho (số_nguyên i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        nếu (v.m_value || v.m_valueX) trả_về false;
    }
    trả_về true;
}
bool V3Number::isNeqZero() const {
    nếu (isString()) trả_về !m_data.str().empty();
    cho (số_nguyên i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        nếu (v.m_value & ~v.m_valueX) trả_về true;
    }
    trả_về false;
}
bool V3Number::isBitsZero(số_nguyên msb, số_nguyên lsb) const {
    cho (số_nguyên i = lsb; i <= msb; ++i) {
        nếu (VL_UNLIKELY(!bitIs0(i))) trả_về false;
    }
    trả_về true;
}
bool V3Number::isEqOne() const {
    nếu (m_data.num()[0].m_value != 1 || m_data.num()[0].m_valueX) trả_về false;
    cho (số_nguyên i = 1; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        nếu (v.m_value || v.m_valueX) trả_về false;
    }
    trả_về true;
}
bool V3Number::isEqAllOnes(số_nguyên optwidth) const {
    // Correct number of zero bits/width matters
    nếu (!optwidth) optwidth = width();
    cho (số_nguyên bit = 0; bit < optwidth; ++bit) {
        nếu (!bitIs1(bit)) trả_về false;
    }
    trả_về true;
}
bool V3Number::isFourState() const VL_MT_SAFE {
    nếu (isDouble() || isString()) trả_về false;
    cho (số_nguyên i = 0; i < words(); ++i) {
        nếu (m_data.num()[i].m_valueX) trả_về true;
    }
    trả_về false;
}
bool V3Number::isAnyX() const VL_MT_SAFE {
    nếu (isDouble() || isString()) trả_về false;
    cho (số_nguyên i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        nếu (v.m_value & v.m_valueX) trả_về true;
    }
    trả_về false;
}
bool V3Number::isAnyXZ() const {
    nếu (isDouble() || isString()) trả_về false;
    cho (số_nguyên i = 0; i < words(); ++i) {
        nếu (m_data.num()[i].m_valueX) trả_về true;
    }
    trả_về false;
}
bool V3Number::isAnyZ() const VL_MT_SAFE {
    nếu (isDouble() || isString()) trả_về false;
    cho (số_nguyên i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        nếu (~v.m_value & v.m_valueX) trả_về true;
    }
    trả_về false;
}
bool V3Number::isLtXZ(const V3Number& rhs) const {
    // Include X/Z in comparisons for sort ordering
    nếu (isString()) trả_về toString() < rhs.toString();
    nếu (isDouble()) trả_về toDouble() < rhs.toDouble();
    cho (số_nguyên bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        nếu (bitIs1(bit) && rhs.bitIs0(bit)) trả_về true;
        nếu (rhs.bitIs1(bit) && bitIs0(bit)) trả_về false;
        nếu (bitIsXZ(bit)) trả_về true;
        nếu (rhs.bitIsXZ(bit)) trả_về false;
    }
    trả_về false;
}
số_nguyên V3Number::countX(số_nguyên lsb, số_nguyên nbits) const VL_MT_SAFE {
    số_nguyên count = 0;
    cho (số_nguyên bitn = 0; bitn < nbits; ++bitn) {
        nếu (lsb + bitn >= width()) trả_về count;
        nếu (bitIsX(lsb + bitn)) ++count;
    }
    trả_về count;
}
số_nguyên V3Number::countZ(số_nguyên lsb, số_nguyên nbits) const VL_MT_SAFE {
    số_nguyên count = 0;
    cho (số_nguyên bitn = 0; bitn < nbits; ++bitn) {
        nếu (lsb + bitn >= width()) trả_về count;
        nếu (bitIsZ(lsb + bitn)) ++count;
    }
    trả_về count;
}

số_nguyên V3Number::widthToFit() const {
    cho (số_nguyên bit = width() - 1; bit > 0; bit--) {
        nếu (!bitIs0(bit)) trả_về bit + 1;
    }
    trả_về 1;  // one bit even if number is == 0
}

uint32_t V3Number::countBits(const V3Number& ctrl) const {
    // Correct number of zero bits/width matters
    số_nguyên n = 0;
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        switch (ctrl.bitIs(0)) {
        case '0':
            nếu (bitIs0(bit)) ++n;
            break;
        case '1':
            nếu (bitIs1(bit)) ++n;
            break;
        case 'x':
            nếu (bitIsX(bit)) ++n;
            break;
        case 'z':
            nếu (bitIsZ(bit)) ++n;
            break;
        }
    }
    trả_về n;
}

uint32_t V3Number::countBits(const V3Number& ctrl1, const V3Number& ctrl2,
                             const V3Number& ctrl3) const {
    // Correct number of zero bits/width matters
    số_nguyên n = countBits(ctrl1);
    nếu (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += countBits(ctrl2);
    nếu ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += countBits(ctrl3);
    }
    trả_về n;
}

uint32_t V3Number::countOnes() const {
    số_nguyên n = 0;
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (bitIs1(bit)) ++n;
    }
    trả_về n;
}

uint32_t V3Number::mostSetBitP1() const {
    cho (số_nguyên bit = width() - 1; bit >= 0; bit--) {
        nếu (!bitIs0(bit)) trả_về bit + 1;
    }
    trả_về 0;
}

uint32_t V3Number::leastSetBitP1() const {
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (!bitIs0(bit)) trả_về bit + 1;
    }
    trả_về 0;
}

//======================================================================

V3Number& V3Number::opBitsNonXZ(const V3Number& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    trả_về *this;
}
V3Number& V3Number::opBitsOne(const V3Number& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    trả_về *this;
}
V3Number& V3Number::opBitsXZ(const V3Number& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    trả_về *this;
}
V3Number& V3Number::opBitsZ(const V3Number& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    trả_về *this;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3Number& V3Number::opRedOr(const V3Number& lhs) {
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    char outc = 0;
    cho (số_nguyên bit = 0; bit < lhs.width(); ++bit) {
        nếu (lhs.bitIs1(bit)) {
            trả_về setSingleBits(1);
        } không_thì nếu (lhs.bitIs0(bit)) {
        } không_thì {
            outc = 'x';
        }
    }
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opRedAnd(const V3Number& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    char outc = 1;
    cho (số_nguyên bit = 0; bit < lhs.width(); ++bit) {
        nếu (lhs.bitIs0(bit)) {
            trả_về setSingleBits(0);
        } không_thì nếu (lhs.bitIs1(bit)) {
        } không_thì {
            outc = 'x';
        }
    }
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opRedXor(const V3Number& lhs) {
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    char outc = 0;
    cho (số_nguyên bit = 0; bit < lhs.width(); ++bit) {
        nếu (lhs.bitIs1(bit)) {
            nếu (outc == 1) {
                outc = 0;
            } không_thì nếu (outc == 0) {
                outc = 1;
            }
        } không_thì nếu (lhs.bitIs0(bit)) {
        } không_thì {
            outc = 'x';
        }
    }
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opCountBits(const V3Number& expr, const V3Number& ctrl1, const V3Number& ctrl2,
                                const V3Number& ctrl3) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS4(expr, ctrl1, ctrl2, ctrl3);
    NUM_ASSERT_LOGIC_ARGS4(expr, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.num()[0].m_value = expr.countBits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::opCountOnes(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    nếu (lhs.isFourState()) trả_về setAllBitsX();
    setZero();
    m_data.num()[0].m_value = lhs.countOnes();
    opCleanThis();
    trả_về *this;
}
V3Number& V3Number::opIsUnknown(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    trả_về setSingleBits(lhs.isAnyXZ());
}
V3Number& V3Number::opOneHot(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    nếu (lhs.isFourState()) trả_về setAllBitsX();
    trả_về setSingleBits(lhs.countOnes() == 1);
}
V3Number& V3Number::opOneHot0(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    nếu (lhs.isFourState()) trả_về setAllBitsX();
    trả_về setSingleBits(lhs.countOnes() <= 1);
}
V3Number& V3Number::opCLog2(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    nếu (lhs.isFourState()) trả_về setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    const số_nguyên adjust = (lhs.countOnes() == 1) ? 0 : 1;
    cho (số_nguyên bit = lhs.width() - 1; bit >= 0; bit--) {
        nếu (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            trả_về *this;
        }
    }
    setZero();
    trả_về *this;
}
V3Number& V3Number::opMostSetBitP1(const V3Number& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    nếu (lhs.isFourState()) trả_về setAllBitsX();
    cho (số_nguyên bit = lhs.width() - 1; bit >= 0; bit--) {
        nếu (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            trả_về *this;
        }
    }
    setZero();
    trả_về *this;
}

V3Number& V3Number::opLogNot(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    char outc = 1;
    cho (số_nguyên bit = 0; bit < lhs.width(); ++bit) {
        nếu (lhs.bitIs1(bit)) {
            outc = 0;
            goto last;
        } không_thì nếu (lhs.bitIs0(bit)) {
        } không_thì {
            outc = 'x';
        }
    }
last:
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opNot(const V3Number& lhs) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } không_thì nếu (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    trả_về *this;
}

V3Number& V3Number::opAnd(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } không_thì nếu (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } không_thì {
            setBit(bit, 'x');
        }
    }
    trả_về *this;
}

V3Number& V3Number::opOr(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } không_thì nếu (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } không_thì {
            setBit(bit, 'x');
        }
    }
    trả_về *this;
}

V3Number& V3Number::opXor(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } không_thì nếu (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } không_thì nếu (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    trả_về *this;
}

void V3Number::fillBits(số_nguyên destLsb, số_nguyên width, char value) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    nếu (!width) trả_về;
    UASSERT(width <= this->width() - destLsb, "Destination bit range exceeds number width");

    const uint32_t valueWord = value == '1' || value == 'x' || value == 1 || value == 3 ? ~0U : 0;
    const uint32_t xWord = value == 'z' || value == 'x' || value == 2 || value == 3 ? ~0U : 0;
    const số_nguyên firstWord = VL_BITWORD_E(destLsb);
    const số_nguyên lastWord = VL_BITWORD_E(destLsb + width - 1);
    cho (số_nguyên word = firstWord; word <= lastWord; ++word) {
        const số_nguyên wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        const số_nguyên wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        const uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        ValueAndX& dest = m_data.num()[word];
        dest.m_value = (dest.m_value & ~mask) | (valueWord & mask);
        dest.m_valueX = (dest.m_valueX & ~mask) | (xWord & mask);
    }
}

void V3Number::copyBits(số_nguyên destLsb, const V3Number& source, số_nguyên sourceLsb, số_nguyên width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    nếu (!width) trả_về;
    UASSERT(width <= this->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    const số_nguyên firstWord = VL_BITWORD_E(destLsb);
    const số_nguyên lastWord = VL_BITWORD_E(destLsb + width - 1);
    cho (số_nguyên destWord = firstWord; destWord <= lastWord; ++destWord) {
        const số_nguyên destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        const số_nguyên destOffset = VL_BITBIT_E(destBit);
        const số_nguyên sourceBit = sourceLsb + destBit - destLsb;
        const số_nguyên sourceWord = VL_BITWORD_E(sourceBit);
        const số_nguyên sourceOffset = VL_BITBIT_E(sourceBit);
        const số_nguyên copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        const uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        const ValueAndX& sourceLo = source.m_data.num()[sourceWord];
        uint32_t value = sourceLo.m_value >> sourceOffset;
        uint32_t valueX = sourceLo.m_valueX >> sourceOffset;
        nếu (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            const ValueAndX& sourceHi = source.m_data.num()[sourceWord + 1];
            value |= sourceHi.m_value << (VL_EDATASIZE - sourceOffset);
            valueX |= sourceHi.m_valueX << (VL_EDATASIZE - sourceOffset);
        }

        ValueAndX& dest = m_data.num()[destWord];
        dest.m_value = (dest.m_value & ~mask) | ((value << destOffset) & mask);
        dest.m_valueX = (dest.m_valueX & ~mask) | ((valueX << destOffset) & mask);
    }
}

V3Number& V3Number::opConcat(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    nếu (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    trả_về *this;
}

V3Number& V3Number::opLenN(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    trả_về *this;
}

V3Number& V3Number::opRepl(const V3Number& lhs,
                           const V3Number& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    trả_về opRepl(lhs, rhs.toUInt());
}

V3Number& V3Number::opRepl(const V3Number& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    nếu (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    cho (unsigned times = 0; times < rhsval; ++times) {
        const uint64_t destLsb = uint64_t{times} * lhs.width();
        nếu (destLsb >= static_cast<uint32_t>(width())) break;
        copyBits(static_cast<số_nguyên>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<số_nguyên>(destLsb)));
    }
    trả_về *this;
}

V3Number& V3Number::opStreamL(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    nếu (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    const số_nguyên ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    cho (số_nguyên istart = 0; istart < lhs.width(); istart += ssize) {
        const số_nguyên ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    trả_về *this;
}

V3Number& V3Number::opLogAnd(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char loutc = 0;
    char routc = 0;
    cho (số_nguyên bit = 0; bit < lhs.width(); ++bit) {
        nếu (lhs.bitIs1(bit)) {
            loutc = 1;
            break;
        }
        nếu (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    cho (số_nguyên bit = 0; bit < rhs.width(); ++bit) {
        nếu (rhs.bitIs1(bit)) {
            routc = 1;
            break;
        }
        nếu (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    char outc = 'x';
    nếu (routc == 1 && loutc == 1) outc = 1;
    nếu (routc == 0 || loutc == 0) outc = 0;
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opLogOr(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    cho (số_nguyên bit = 0; bit < lhs.width(); ++bit) {
        nếu (lhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        nếu (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    cho (số_nguyên bit = 0; bit < rhs.width(); ++bit) {
        nếu (rhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        nếu (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opLogIf(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number lnot = lhs;
    lnot.opLogNot(lhs);
    trả_về opLogOr(lnot, rhs);
}

V3Number& V3Number::opLogEq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3Number ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    trả_về opLogAnd(ifa, ifb);
}

V3Number& V3Number::opAtoN(const V3Number& lhs, số_nguyên base) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    nếu (base == AstAtoN::ATOREAL) trả_về setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    nếu (errno != 0) v = 0;
    trả_về setLongS(static_cast<int32_t>(v));
}

V3Number& V3Number::opPutcN(const V3Number& lhs, const V3Number& rhs, const V3Number& ths) {
    NUM_ASSERT_OP_ARGS3(lhs, rhs, ths);
    NUM_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    const int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    nếu (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    trả_về setString(lstring);
}

V3Number& V3Number::opGetcN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    nếu (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    trả_về setLong(v);
}

V3Number& V3Number::opSubstrN(const V3Number& lhs, const V3Number& rhs, const V3Number& ths) {
    NUM_ASSERT_OP_ARGS3(lhs, rhs, ths);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    const int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    nếu (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) trả_về setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    trả_về setString(lstring.substr(i, j - i + 1));
}

V3Number& V3Number::opCompareNN(const V3Number& lhs, const V3Number& rhs, bool ignoreCase) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    số_nguyên result;
    const string lstring = lhs.toString();
    const string rstring = rhs.toString();
    nếu (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } không_thì {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    trả_về setLongS(result);
}

V3Number& V3Number::opEq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    nếu (lhs.isString()) trả_về opEqN(lhs, rhs);
    nếu (lhs.isDouble()) trả_về opEqD(lhs, rhs);
    char outc = 1;
    cho (số_nguyên bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        nếu (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            goto last;
        }
        nếu (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            goto last;
        }
        nếu (lhs.bitIsXZ(bit)) outc = 'x';
        nếu (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opNeq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    nếu (lhs.isString()) trả_về opNeqN(lhs, rhs);
    nếu (lhs.isDouble()) trả_về opNeqD(lhs, rhs);
    char outc = 0;
    cho (số_nguyên bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        nếu (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            goto last;
        }
        nếu (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        nếu (lhs.bitIsXZ(bit)) outc = 'x';
        nếu (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    trả_về setSingleBits(outc);
}

V3Hash V3Number::toHash() const {
    // Shouldmatch isCaseEq
    nếu (isString()) trả_về V3Hash{toString()};
    nếu (isDouble()) trả_về V3Hash{toDouble()};
    V3Hash hash{width()};
    cho (số_nguyên i = 0; i < words(); ++i) hash += m_data.num()[i].m_value;
    trả_về hash;
}

bool V3Number::isCaseEq(const V3Number& rhs) const {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    nếu (isString()) trả_về toString() == rhs.toString();
    nếu (isDouble()) trả_về toDouble() == rhs.toDouble();
    nếu (width() != rhs.width()) trả_về false;
    cho (số_nguyên i = 0; i < words(); ++i) {
        nếu (!(m_data.num()[i] == rhs.m_data.num()[i])) trả_về false;
    }
    trả_về true;
}

V3Number& V3Number::opCaseEq(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3Number& V3Number::opCaseNeq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    char outc = 0;
    nếu (lhs.isString()) {
        trả_về opNeqN(lhs, rhs);
    } không_thì nếu (lhs.isDouble()) {
        trả_về opNeqD(lhs, rhs);
    }
    cho (số_nguyên bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        nếu (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            goto last;
        }
    }
last:
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opWildEq(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 1;
    cho (số_nguyên bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        nếu (!rhs.bitIsXZ(bit)) {
            nếu (lhs.bitIsXZ(bit)) {
                outc = 'x';
                goto last;
            }
            nếu (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                goto last;
            }
        }
    }
last:
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opWildNeq(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    cho (số_nguyên bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        nếu (!rhs.bitIsXZ(bit)) {
            nếu (lhs.bitIsXZ(bit)) {
                outc = 'x';
                goto last;
            }
            nếu (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                goto last;
            }
        }
    }
last:
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opGt(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    cho (số_nguyên bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        nếu (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        nếu (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        nếu (lhs.bitIsXZ(bit)) outc = 'x';
        nếu (rhs.bitIsXZ(bit)) outc = 'x';
    }
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opGtS(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    {
        const số_nguyên mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        nếu (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } không_thì nếu (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } không_thì nếu (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } không_thì nếu (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } không_thì {
            // both positive or negative, normal >
            cho (số_nguyên bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                nếu (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                nếu (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                nếu (lhs.bitIsXZ(bit)) outc = 'x';
                nếu (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    trả_về setSingleBits(outc);
}

V3Number& V3Number::opGte(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number& eq = opEq(lhs, rhs);
    nếu (eq.isNeqZero()) trả_về eq;  // Return true
    trả_về opGt(lhs, rhs);
}

V3Number& V3Number::opGteS(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number& eq = opEq(lhs, rhs);
    nếu (eq.isNeqZero()) trả_về eq;  // Return true
    trả_về opGtS(lhs, rhs);
}

V3Number& V3Number::opLt(const V3Number& lhs, const V3Number& rhs) { trả_về opGt(rhs, lhs); }
V3Number& V3Number::opLte(const V3Number& lhs, const V3Number& rhs) { trả_về opGte(rhs, lhs); }
V3Number& V3Number::opLtS(const V3Number& lhs, const V3Number& rhs) { trả_về opGtS(rhs, lhs); }
V3Number& V3Number::opLteS(const V3Number& lhs, const V3Number& rhs) { trả_về opGteS(rhs, lhs); }

V3Number& V3Number::opShiftR(const V3Number& lhs, const V3Number& rhs) {
    // L(lhs) bit return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (rhs.isFourState()) trả_về setAllBitsX();
    setZero();
    nếu (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) trả_về *this;
    const uint32_t rhsval = rhs.toUInt();
    nếu (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<số_nguyên>(rhsval),
                 std::min(width(), lhs.width() - static_cast<số_nguyên>(rhsval)));
    }
    trả_về *this;
}

V3Number& V3Number::opShiftRS(const V3Number& lhs, const V3Number& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (rhs.isFourState()) trả_về setAllBitsX();
    setZero();
    const bool overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    nếu (!overflow) {
        const uint32_t rhsval = rhs.toUInt();
        nếu (rhsval < lbits) {
            const số_nguyên copyWidth = std::min(width(), static_cast<số_nguyên>(lbits - rhsval));
            copyBits(0, lhs, static_cast<số_nguyên>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            trả_về *this;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    trả_về *this;
}

V3Number& V3Number::opShiftL(const V3Number& lhs, const V3Number& rhs) {
    // L(lhs) bit return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (rhs.isFourState()) trả_về setAllBitsX();
    setZero();
    nếu (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) trả_về *this;
    const uint32_t rhsval = rhs.toUInt();
    nếu (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<số_nguyên>(rhsval), lhs, 0,
                 std::min(width() - static_cast<số_nguyên>(rhsval), lhs.width()));
    }
    trả_về *this;
}

//======================================================================
// Ops - Arithmetic

V3Number& V3Number::opNegate(const V3Number& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    nếu (lhs.isFourState()) trả_về setAllBitsX();
    V3Number notlhs(&lhs, width());
    notlhs.opNot(lhs);
    const V3Number one(&lhs, width(), 1);
    opAdd(notlhs, one);
    trả_về *this;
}
V3Number& V3Number::opAdd(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    cho (số_nguyên word = 0; word < words(); ++word) {
        const uint64_t lwordval = lhs.m_data.num()[word].m_value;
        const uint64_t rwordval = rhs.m_data.num()[word].m_value;
        const uint64_t sum = lwordval + rwordval + carry;
        m_data.num()[word].m_value = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    trả_về *this;
}
V3Number& V3Number::opSub(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    V3Number negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    trả_về opAdd(lhs, negrhs);
}
V3Number& V3Number::opMul(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    setZero();
    nếu (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } không_thì {
        cho (số_nguyên lword = 0; lword < lhs.words(); ++lword) {
            const uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.num()[lword].m_value);
            nếu (lwordval == 0) continue;
            cho (số_nguyên rword = 0; rword < rhs.words(); ++rword) {
                const uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.num()[rword].m_value);
                nếu (rwordval == 0) continue;
                uint64_t mul = lwordval * rwordval;
                cho (số_nguyên qword = lword + rword; qword < words(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.num()[qword].m_value);
                    m_data.num()[qword].m_value = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    nếu (mul == 0) break;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    trả_về *this;
}
V3Number& V3Number::opMulS(const V3Number& lhs, const V3Number& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    V3Number lhsNoSign = lhs;
    nếu (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    nếu (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3Number qNoSign = opMul(lhsNoSign, rhsNoSign);
    nếu ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } không_thì {
        opAssign(qNoSign);
    }
    trả_về *this;
}
V3Number& V3Number::opDiv(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    nếu (rhs.isEqZero()) trả_về setAllBitsXRemoved();
    nếu (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        trả_về *this;
    } không_thì {
        // Wide division
        trả_về opModDivGuts(lhs, rhs, false);
    }
}
V3Number& V3Number::opDivS(const V3Number& lhs, const V3Number& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    nếu (rhs.isEqZero()) trả_về setAllBitsXRemoved();
    V3Number lhsNoSign = lhs;
    nếu (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    nếu (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3Number qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    nếu ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } không_thì {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *this);
    trả_về *this;
}
V3Number& V3Number::opModDiv(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    nếu (rhs.isEqZero()) trả_về setAllBitsXRemoved();
    nếu (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        trả_về *this;
    } không_thì {
        // Wide modulus
        trả_về opModDivGuts(lhs, rhs, true);
    }
}
V3Number& V3Number::opModDivS(const V3Number& lhs, const V3Number& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    nếu (rhs.isEqZero()) trả_về setAllBitsXRemoved();
    V3Number lhsNoSign = lhs;
    nếu (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    nếu (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3Number qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    nếu (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } không_thì {
        opAssign(qNoSign);
    }
    trả_về *this;
}
V3Number& V3Number::opModDivGuts(const V3Number& lhs, const V3Number& rhs, bool is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    const số_nguyên lWords = lhs.words();
    const số_nguyên umsbp1 = lhs.mostSetBitP1();  // dividend
    const số_nguyên vmsbp1 = rhs.mostSetBitP1();  // divisor
    nếu (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *this);
        trả_về *this;
    }

    const số_nguyên uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    const số_nguyên vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    nếu (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        cho (số_nguyên j = uw - 1; j >= 0; j--) {
            const uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.num()[j].m_value));
            m_data.num()[j].m_value = unw64 / static_cast<uint64_t>(rhs.m_data.num()[0].m_value);
            k = unw64
                - (static_cast<uint64_t>(m_data.num()[j].m_value)
                   * static_cast<uint64_t>(rhs.m_data.num()[0].m_value));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *this << " rem=0x" << std::hex
                                   << k << std::dec);
        nếu (is_modulus) {
            setZero();
            m_data.num()[0].m_value = k;
        }
        opCleanThis();
        trả_về *this;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_WORDS + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_WORDS + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    cho (số_nguyên i = 0; i < lWords; ++i) m_data.num()[i].m_value = 0;
    cho (số_nguyên i = 0; i < lWords + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    const số_nguyên s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    const uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    cho (số_nguyên i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.num()[i].m_value << s)
                | (shift_mask & (rhs.m_data.num()[i - 1].m_value >> (32 - s)));
    }
    vn[0] = rhs.m_data.num()[0].m_value << s;

    // Copy and shift dividend by same amount; may set new upper word
    nếu (s) {
        un[uw] = lhs.m_data.num()[uw - 1].m_value >> (32 - s);
    } không_thì {
        un[uw] = 0;
    }
    cho (số_nguyên i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.num()[i].m_value << s)
                | (shift_mask & (lhs.m_data.num()[i - 1].m_value >> (32 - s)));
    }
    un[0] = lhs.m_data.num()[0].m_value << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    cho (số_nguyên j = uw - vw; j >= 0; j--) {
        // Estimate
        const uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        nếu (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            nếu (rhat < 0x100000000ULL) goto again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        cho (số_nguyên i = 0; i < vw; ++i) {
            const uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.num()[j].m_value = qhat;  // Save quotient digit

        nếu (t < 0) {
            // Over subtracted; correct by adding back
            m_data.num()[j].m_value--;
            k = 0;
            cho (số_nguyên i = 0; i < vw; ++i) {
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

    nếu (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        cho (số_nguyên i = 0; i < vw; ++i) {
            m_data.num()[i].m_value = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        cho (số_nguyên i = vw; i < lWords; ++i) m_data.num()[i].m_value = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *this);
        trả_về *this;
    } không_thì {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *this);
        trả_về *this;
    }
}

V3Number& V3Number::opPow(const V3Number& lhs, const V3Number& rhs, bool lsign, bool rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    nếu (lhs.isFourState() || rhs.isFourState()) trả_về setAllBitsX();
    nếu (rhs.isEqZero()) trả_về setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    nếu (rsign && rhs.isNegative()) {
        nếu (lhs.isEqZero()) {
            trả_về setAllBitsXRemoved();
        } không_thì nếu (lhs.isEqOne()) {
            trả_về setValue1();
        } không_thì nếu (lsign && lhs.isEqAllOnes()) {
            nếu (rhs.bitIs1(0)) {
                trả_về setAllBits1();  // -1^odd=-1
            } không_thì {
                trả_về setValue1();  // -1^even=1
            }
        }
        trả_về setZero();
    }
    nếu (lhs.isEqZero()) trả_về setZero();
    setZero();
    m_data.num()[0].m_value = 1;
    V3Number power(&lhs, width());
    power.opAssign(lhs);
    cho (số_nguyên bit = 0; bit < rhs.width(); ++bit) {
        nếu (bit > 0) {  // power = power*power
            V3Number lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        nếu (rhs.bitIs1(bit)) {  // out *= power
            V3Number lastOut(&lhs, width());
            lastOut.opAssign(*this);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    trả_về *this;
}
V3Number& V3Number::opPowSU(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    trả_về opPow(lhs, rhs, true, false);
}
V3Number& V3Number::opPowSS(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    trả_về opPow(lhs, rhs, true, true);
}
V3Number& V3Number::opPowUS(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    trả_về opPow(lhs, rhs, false, true);
}

V3Number& V3Number::opBufIf1(const V3Number& ens, const V3Number& if1s) {
    NUM_ASSERT_OP_ARGS2(ens, if1s);
    NUM_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    cho (số_nguyên bit = 0; bit < width(); ++bit) {
        nếu (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } không_thì {
            setBit(bit, 'z');
        }
    }
    trả_về *this;
}

// Sets all bits in range to the given value
V3Number& V3Number::opSetRange(uint32_t lsb, uint32_t width, char bitValue) {
    nếu (lsb < static_cast<uint32_t>(this->width())) {
        fillBits(static_cast<số_nguyên>(lsb),
                 static_cast<số_nguyên>(std::min(width, static_cast<uint32_t>(this->width()) - lsb)),
                 bitValue);
    }
    trả_về *this;
}

V3Number& V3Number::opAssign(const V3Number& lhs) { trả_về opAssignNonXZ(lhs, false); }
V3Number& V3Number::opAssignNonXZ(const V3Number& lhs, bool ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    nếu (this != &lhs) {
        nếu (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = true;
        } không_thì nếu (isString()) {
            nếu (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } không_thì {
                m_data.str() = lhs.m_data.str();
            }
        } không_thì nếu (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } không_thì nếu (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } không_thì nếu (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } không_thì {
            cho (số_nguyên word = 0; word < words(); ++word) {
                const uint32_t value = word < lhs.words() ? lhs.m_data.num()[word].m_value
                                                                & ~lhs.m_data.num()[word].m_valueX
                                                          : 0;
                m_data.num()[word] = {value, 0};
            }
            opCleanThis();
        }
    }
    trả_về *this;
}

V3Number& V3Number::opNToI(const V3Number& lhs) {
    // String to packed number
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    setZero();
    const string& str = lhs.toString();
    cho (size_t n = 0; n < str.length(); ++n) {
        const char c = str[str.length() - 1 - n];
        cho (size_t cbit = 0; cbit < 8; ++cbit) {
            const std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    trả_về *this;
}

V3Number& V3Number::opExtendS(const V3Number& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    const số_nguyên copyWidth = std::min(width(), static_cast<số_nguyên>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    trả_về *this;
}

V3Number& V3Number::opExtendXZ(const V3Number& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    const số_nguyên copyWidth = std::min(width(), static_cast<số_nguyên>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    trả_về *this;
}

V3Number& V3Number::opClean(const V3Number& lhs, uint32_t bits) { trả_về opSel(lhs, bits - 1, 0); }

void V3Number::opCleanThis(bool warnOnTruncation) {
    // Clean MSB of number
    NUM_ASSERT_LOGIC_ARGS1(*this);
    const ValueAndX v = m_data.num()[words() - 1];
    const uint32_t newValueMsb = v.m_value & hiWordMask();
    const uint32_t newValueXMsb = v.m_valueX & hiWordMask();
    nếu (warnOnTruncation && (newValueMsb != v.m_value || newValueXMsb != v.m_valueX)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.num()[words() - 1] = {newValueMsb, newValueXMsb};
}

V3Number& V3Number::opSel(const V3Number& lhs, const V3Number& msb, const V3Number& lsb) {
    NUM_ASSERT_OP_ARGS3(lhs, msb, lsb);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS2(msb, lsb);
    nếu (lsb.isFourState() || msb.isFourState()) trả_về setAllBitsX();
    trả_về opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3Number& V3Number::opSel(const V3Number& lhs, uint32_t msbval, uint32_t lsbval) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    số_nguyên copyWidth = 0;
    nếu (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        const uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<số_nguyên>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<số_nguyên>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    trả_về *this;
}

V3Number& V3Number::opSelInto(const V3Number& lhs, const V3Number& lsb, số_nguyên width) {
    trả_về opSelInto(lhs, lsb.toSInt(), width);
}

V3Number& V3Number::opSelInto(const V3Number& lhs, số_nguyên lsbval, số_nguyên width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    const số_nguyên copyWidth = std::max(0, std::min({width, lhs.width(), this->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    const số_nguyên fillWidth
        = std::max(0, std::min(width - copyWidth, this->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    trả_về *this;
}

//======================================================================
// Ops - Floating point

V3Number& V3Number::opIToRD(const V3Number& lhs, bool isSigned) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3Number noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    double d = 0;
    const bool negate = isSigned && noxz.isNegative();
    nếu (negate) {
        const V3Number noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    cho (số_nguyên bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        nếu (noxz.bitIs1(bit)) d += exp2(bit);
    }
    nếu (negate) d = -d;
    trả_về setDouble(d);
}
V3Number& V3Number::opRToIS(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    const double v = VL_TRUNC(lhs.toDouble());
    const int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    trả_về setLongS(i);
}
V3Number& V3Number::opRToIRoundS(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    const double v = VL_ROUND(lhs.toDouble());
    setZero();
    union {
        double d;
        uint64_t q;
    } u;
    u.d = v;
    (void)u.d;

    const số_nguyên exp = static_cast<số_nguyên>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    const số_nguyên lsb = exp - 52;
    const uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    nếu (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        cho (số_nguyên bit = 0; bit <= 52; ++bit) {
            nếu (mantissa & (1ULL << bit)) {
                const số_nguyên outbit = bit + lsb;
                nếu (outbit >= 0) setBit(outbit, 1);
            }
        }
        nếu (v < 0) {
            const V3Number noSign = *this;
            opNegate(noSign);
        }
    }
    trả_về *this;
}
V3Number& V3Number::opRealToBits(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    union {
        double m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    trả_về setQuad(u.m_v);
}
V3Number& V3Number::opBitsToRealD(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    union {
        double m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    trả_về setDouble(u.m_d);
}
V3Number& V3Number::opNegateD(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    trả_về setDouble(-lhs.toDouble());
}
V3Number& V3Number::opAddD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setDouble(lhs.toDouble() + rhs.toDouble());
}
V3Number& V3Number::opSubD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setDouble(lhs.toDouble() - rhs.toDouble());
}
V3Number& V3Number::opMulD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setDouble(lhs.toDouble() * rhs.toDouble());
}
V3Number& V3Number::opDivD(const V3Number& lhs, const V3Number& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setDouble(lhs.toDouble() / rhs.toDouble());
}
V3Number& V3Number::opPowD(const V3Number& lhs, const V3Number& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3Number& V3Number::opEqD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3Number& V3Number::opNeqD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3Number& V3Number::opGtD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3Number& V3Number::opGteD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3Number& V3Number::opLtD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3Number& V3Number::opLteD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3Number& V3Number::opConcatN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setString(lhs.toString() + rhs.toString());
}
V3Number& V3Number::opReplN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_STRING_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(rhs);
    trả_về opReplN(lhs, rhs.toUInt());
}
V3Number& V3Number::opReplN(const V3Number& lhs, uint32_t rhsval) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    cho (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    trả_về setString(out);
}
V3Number& V3Number::opCvtPackString(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    trả_về setString(lhs.toString());
}
V3Number& V3Number::opToLowerN(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const std::string out = VString::downcase(lhs.toString());
    trả_về setString(out);
}
V3Number& V3Number::opToUpperN(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const std::string out = VString::upcase(lhs.toString());
    trả_về setString(out);
}

V3Number& V3Number::opEqN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toString() == rhs.toString());
}
V3Number& V3Number::opNeqN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toString() != rhs.toString());
}
V3Number& V3Number::opGtN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toString() > rhs.toString());
}
V3Number& V3Number::opGteN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toString() >= rhs.toString());
}
V3Number& V3Number::opLtN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toString() < rhs.toString());
}
V3Number& V3Number::opLteN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    trả_về setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

void V3Number::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    FileLine* const fl = new FileLine{FileLine::builtInFilename()};
    V3Number num{fl, 32, 0};
    num.selfTestThis();
}

void V3Number::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3Number::epsilonEqual(0, 0), true);
    UASSERT_SELFTEST(V3Number::epsilonEqual(1e19, 1e19), true);
    UASSERT_SELFTEST(V3Number::epsilonEqual(9, 0.0001), false);
    UASSERT_SELFTEST(V3Number::epsilonEqual(1, 1 + std::numeric_limits<double>::epsilon()), true);
    UASSERT_SELFTEST(V3Number::epsilonEqual(0.009, 0.00899999999999999931998839741709), true);

    UASSERT_SELFTEST(V3Number::epsilonIntegral(0), true);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(1), true);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(-1), true);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(1.0001), false);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(0.9999), false);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(-1.0001), false);
    UASSERT_SELFTEST(V3Number::epsilonIntegral(-0.9999), false);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
