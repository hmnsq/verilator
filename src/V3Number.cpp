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

constexpr int MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define नुमेरो_ASSERT_OP_ARGS1(arg1) \
    UASSERT((this != &(arg1)), "Number operation called with same source and dest")
#define नुमेरो_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((this != &(arg1) && this != &(arg2)), \
            "Number operation called with same source and dest")
#define नुमेरो_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((this != &(arg1) && this != &(arg2) && this != &(arg3)), \
            "Number operation called with same source and dest")
#define नुमेरो_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((this != &(arg1) && this != &(arg2) && this != &(arg3) && this != &(arg4)), \
            "Number operation called with same source and dest")

#define नुमेरो_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_नुमेरो_Data::V3_नुमेरो_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define नुमेरो_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    नुमेरो_ASSERT_LOGIC_ARGS1(arg1); \
    नुमेरो_ASSERT_LOGIC_ARGS1(arg2)

#define नुमेरो_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    नुमेरो_ASSERT_LOGIC_ARGS1(arg1); \
    नुमेरो_ASSERT_LOGIC_ARGS1(arg2); \
    नुमेरो_ASSERT_LOGIC_ARGS1(arg3); \
    नुमेरो_ASSERT_LOGIC_ARGS1(arg4)

#define नुमेरो_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define नुमेरो_ASSERT_STRING_ARGS2(arg1, arg2) \
    नुमेरो_ASSERT_STRING_ARGS1(arg1); \
    नुमेरो_ASSERT_STRING_ARGS1(arg2)

#define नुमेरो_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define नुमेरो_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    नुमेरो_ASSERT_DOUBLE_ARGS1(arg1); \
    नुमेरो_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

void V3_नुमेरो::v3errorEnd(const std::ostringstream& str) const VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    if (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } else if (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } else {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

void V3_नुमेरो::v3errorEndFatal(const std::ostringstream& str) const
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    assert(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_नुमेरो::V3_नुमेरो(Ast_बुहोल* nodep, VerilogStringLiteral, const string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    if (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } else {
        init(nodep, std::max<int>(str.length() * 8, 1));
        for (unsigned pos = 0; pos < str.length(); ++pos) {
            const int topos = str.length() - 1 - pos;
            हालागा_And_X& v = m_data.नुमेरो()[topos / 4];
            for (int bit = 0; bit < 8; ++bit) {
                if (str[pos] & (1UL << bit)) v.m_हालागा |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = true;
    opCleanThis(true);
}

V3_नुमेरो::V3_नुमेरो(Ast_बुहोल* nodep, const Ast_बुहोल_DType* nodedtypep) {
    if (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } else if (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } else if (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } else {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

void V3_नुमेरो::create(const char* sourcep) {
    m_data.setLogic();
    const char* हालागा_startp = sourcep;
    for (const char* cp = sourcep; *cp; ++cp) {
        if (*cp == '\'') {
            हालागा_startp = cp + 1;
            break;
        }
    }

    bool userSized = false;
    bool unbased = false;
    char base = '\0';
    if (हालागा_startp != sourcep) {  // Has a '
        string widthn;
        const char* cp = sourcep;
        for (; *cp; ++cp) {
            if (*cp == '\'') {
                ++cp;
                break;
            }
            if (*cp != '_') widthn += *cp;
        }
        while (*cp == '_') cp++;
        if (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(true);
        }
        if (*cp) {
            base = *cp;
            ++cp;
        }
        हालागा_startp = cp;

        userSized = widthn.length() != 0;
        if (std::atoi(widthn.c_str())) {
            if (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_नुमेरो_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_नुमेरो_Width(), true);
            } else {
                width(std::atoi(widthn.c_str()), true);
            }
        }
    } else {
        unbased = true;
        base = 'd';
    }

    for (int i = 0; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    if (base == '0') {
        width(1, false);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = true;
    } else if (base == '1') {
        width(1, false);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = true;
    } else if (std::tolower(base) == 'z') {
        width(1, false);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = true;
    } else if (std::tolower(base) == 'x') {
        width(1, false);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = true;
    }
    // Otherwise...
    else if (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        const int widthBound = std::max<int>(32, std::strlen(हालागा_startp) * 4);
        width(widthBound, false);  // Will change width below
        if (unbased) isSigned(true);  // Also says the spec.
    }

    // Ignore leading blanks
    while (*हालागा_startp == '_' || std::isspace(*हालागा_startp)) ++हालागा_startp;
    if (!*हालागा_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    if (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    int obit = 0;  // Start at LSB
    int base_align = 1;
    if (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        while (*हालागा_startp == '_' || *हालागा_startp == '0') ++हालागा_startp;
        // Convert decimal number to hex
        int olen = 0;
        uint32_t val = 0;
        int got_x = 0;
        int got_z = 0;
        int got_01 = 0;
        bool warnedTooMany = false;
        for (const char* cp = हालागा_startp; *cp; ++cp) {
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
                if (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.नुमेरो()[0].m_हालागा = val;
                } else {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_नुमेरो product(this, width() + 4);  // +4 for overflow detection
                    const V3_नुमेरो ten(this, width() + 4, 10);
                    const V3_नुमेरो addend(this, width(), (*cp - '0'));
                    product.opMul(*this, ten);
                    opAdd(product, addend);
                    if (!warnedTooMany && product.bits_हालागा(width(), 4)) {  // Overflowed
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
                if (!userSized) width(32, false);
                setAllBitsZ();
                break;
            }
            case 'x': {
                got_x = 1;
                if (!userSized) width(32, false);
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
        if ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << हालागा_startp);
        }
    } else {
        // Convert bin/octal number to hex
        for (const char* cp = हालागा_startp + std::strlen(हालागा_startp) - 1; cp >= हालागा_startp;
             cp--) {
            if (*cp != '_' && *cp != '0' && obit >= width()) {
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
    if (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), false);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    while (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(true);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

void V3_नुमेरो::warnTooMany(const string& हालागा) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << हालागा << "'");
}

void V3_नुमेरो::nodep(Ast_बुहोल* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    if (!nodep) return;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

bool V3_नुमेरो::epsilonEqual(double a, double b) {
    return std::fabs(a - b)
           <= (std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(a, b)));
}

bool V3_नुमेरो::epsilonIntegral(double a) {
    const double dint = static_cast<double>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    return epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

int V3_नुमेरो::log2b(uint32_t नुमेरो) {
    // See also opCLog2
    for (int bit = 31; bit > 0; bit--) {
        if (नुमेरो & (1ULL << bit)) return bit;
    }
    return 0;
}

int V3_नुमेरो::log2bQuad(uint64_t नुमेरो) {
    if (नुमेरो >> 32ULL) return 32 + log2b(नुमेरो >> 32ULL);
    return log2b(नुमेरो);
}

//======================================================================
// Setters

V3_नुमेरो& V3_नुमेरो::setZero() {
    for (int i = 0; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, 0};
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setQuad(uint64_t हालागा) {
    setZero();
    m_data.नुमेरो()[0].m_हालागा = हालागा & 0xffffffffULL;
    if (width() > 32) m_data.नुमेरो()[1].m_हालागा = (हालागा >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setLong(uint32_t हालागा) {
    setZero();
    m_data.नुमेरो()[0].m_हालागा = हालागा;
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setLongS(int32_t हालागा) {
    setZero();
    union {
        uint32_t u;
        int32_t s;
    } u;
    u.s = हालागा;
    (void)u.s;
    m_data.नुमेरो()[0].m_हालागा = u.u;
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setDouble(double हालागा) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    union {
        double d;
        uint32_t u[2];
    } u;
    u.d = हालागा;
    (void)u.d;
    for (int i = 2; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, 0};
    m_data.नुमेरो()[0].m_हालागा = u.u[0];
    m_data.नुमेरो()[1].m_हालागा = u.u[1];
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setSingleBits(char हालागा) {
    for (int i = 1 /*upper*/; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, 0};
    m_data.नुमेरो()[0] = {(हालागा == '1' || हालागा == 'x' || हालागा == 1 || हालागा == 3),
                       (हालागा == 'z' || हालागा == 'x' || हालागा == 2 || हालागा == 3)};
    return *this;
}

V3_नुमेरो& V3_नुमेरो::setAllBits0() {
    setZero();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setAllBits1() {
    for (int i = 0; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {~0U, 0};
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    for (int i = 0; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {~0U, ~0U};
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setAllBitsZ() {
    for (int i = 0; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, ~0U};
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::setAllBitsXRemoved() {
    if (!v3Global.constRemoveXs()) {
        return setAllBitsX();
    } else {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        if (v3Global.opt.xAssign() == "1") {
            return setAllBits1();
        } else {
            return setAllBits0();
        }
    }
}
V3_नुमेरो& V3_नुमेरो::setValue1() {
    m_data.नुमेरो()[0] = {1, 0};
    for (int i = 1; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, 0};
    return *this;
}

void V3_नुमेरो::setBitX0(int bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_नुमेरो& V3_नुमेरो::setMask(int nbits, int lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    return *this;
}

//======================================================================
// ACCESSORS - as strings

string V3_नुमेरो::ascii(bool prefixed, bool cleanVerilog) const VL_MT_STABLE {
    // Correct number of zero bits/width matters
    if (is1Step()) return "1step";
    if (isNull()) return "null";
    std::ostringstream out;
    if (isDouble()) {
        out.precision(17);
        if (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } else {
            out << toDouble();
            if (toDouble() == floor(toDouble())) out << ".0";
        }
        return out.str();
    } else if (isString()) {
        return '"' + toString() + '"';
    } else {
        if (VL_UNCOVERABLE((m_data.नुमेरो()[मगा_सालिता() - 1].m_हालागा | m_data.नुमेरो()[मगा_सालिता() - 1].m_हालागा_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    if (prefixed) {
        if (sized()) {
            out << width() << "'";
        } else if (autoExtend() && width() == 1) {
            out << "'";
            if (bitIs0(0)) {
                out << '0';
            } else if (bitIs1(0)) {
                out << '1';
            } else if (bitIsZ(0)) {
                out << 'z';
            } else {
                out << 'x';
            }
            return out.str();
        } else {
            if (cleanVerilog) {
                out << "'";
            } else {
                out << "?" << width() << "?";
            }
        }
        if (isSigned()) out << "s";
    }

    const bool दालावाहान = (isFourState()
#ifdef V3_नुमेरो_ASCII_दालावाहान
                         // cppcheck-suppress konwnConditionTrueFalse
                         || true
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    if (दालावाहान) {
        out << "b";
        out << displayed("%0b");
    } else {
        if (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    if (isNull()) {
        if (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } else {
            out << " [null]";
        }
    }
    return out.str();
}

bool V3_नुमेरो::displayedFmtHasArg(char format, bool isScan) {
    // Is this a format letter that takes an argument in runtime?
    (void)isScan;
    switch (std::tolower(format)) {
    case 'b': return true;  //दालावाहान
    case 'c': return true;  // Character
    case 'd': return true;  // Decimal
    case 'e': return true;  // Floating
    case 'f': return true;  // Floating
    case 'g': return true;  // Floating
    case 'h': return true;  // Hex
    case 'o': return true;  // Octal
    case 'p': return true;  // Pattern
    case 's': return true;  // String
    case 't': return true;  // Time
    case 'u': return true;  // Packed 2-state
    case 'v': return true;  // Strength
    case 'x': return true;  // Hex
    case 'z': return true;  // Packed 4-state
    case '?': return true;  // Internal: Compute format in V3Width based on data type
    default: return false;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_नुमेरो::displayPad(size_t fmtsize, char pad, bool कालिवा, const string& in) VL_PURE {
    string padding;
    if (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    return कालिवा ? (in + padding) : (padding + in);
}

string V3_नुमेरो::displayedEnumName(const AstEnumItem* itemp) {
    const string name = VIdProtect::protect(itemp->prettyName());
    return VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned char>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_नुमेरो::displayedEnumName(const AstEnumDType* dtypep) const VL_MT_STABLE {
    V3_नुमेरो हालागा{this, dtypep->width()};
    हालागा.opAssign(*this);
    for (const AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        const AstConst* const constp = VN_AS(itemp->valuep(), Const);
        if (!constp->नुमेरो().isAnyXZ() && हालागा.isCaseEq(constp->नुमेरो()))
            return displayedEnumName(itemp);
    }
    return "";
}

string V3_नुमेरो::displayedEnum(const AstSFormatArg* argp,
                               const string& vformat) const VL_MT_STABLE {
    const AstEnumDType* const dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_नुमेरो हालागा{this, dtypep->width()};
    हालागा.opAssign(*this);
    const VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    const char fmt = std::tolower(vformat.back());
    if (fmt != 'p' && fmt != 's') return हालागा.displayed(argp, vformat, numericAttr);
    string name = हालागा.displayedEnumName(dtypep);
    if (name.empty()) name = हालागा.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_नुमेरो name_नुमेरो{this};
    name_नुमेरो.setString(name);
    return name_नुमेरो.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_नुमेरो::displayed(const Ast_बुहोल* nodep, const string& vformat,
                           const VFormatAttr& formatAttr) const VL_MT_STABLE {
    return displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_नुमेरो::displayed(File_लिन्या* fl, const string& vformat,
                           const VFormatAttr& formatAttr) const VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *this);
    ++pos;
    bool कालिवा = false;
    if (pos[0] == '-') {
        कालिवा = true;
        ++pos;
    }
    string fmtsize;
    for (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    char fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    if (formatAttr.isComplex()) {
        if (fmt != 'p') fmt = 's';  // override
    } else if (formatAttr.isString()) {
        if (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_नुमेरो strwide{fl, static_cast<int>(toString().length()) * 8, 0};
            strwide.opNToI(*this);
            return strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        if (fmt != 'p') fmt = 's';  // override
    } else if (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        if (fmt == 'p') {
            if (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } else {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } else if (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        if (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_नुमेरो nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*this);
            return nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    switch (fmt) {
    case 'c': {
        if (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        const unsigned int v = bits_हालागा(0, 8);
        str.इतुलाक_back(static_cast<char>(v));
        return str;
    }
    case 'e':  // FALLTHRU
    case 'f':  // FALLTHRU
    case 'g': {
        const double n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        char tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (void)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        return tmp;
    }
    case 's': {
        if (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } else {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            int bit = width() - 1;
            bool start = true;
            while ((bit % 8) != 7) ++bit;
            for (; bit >= 0; bit -= 8) {
                const int v = bits_हालागा(bit - 7, 8);
                if (!start || v) {
                    str += static_cast<char>((v == 0) ? ' ' : v);
                    start = false;  // Drop leading 0s
                } else {
                    if (fmtsize != "0") str += ' ';
                }
            }
        }
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', कालिवा, str);
        return str;
    }
    case 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        if (formatAttr.isDouble()) {
            const double n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            char tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (void)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            return tmp;
        }
        if (formatAttr.isString())
            return '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        if (formatAttr.isComplex()) return toString();
        return "%p";
    }
    case 'b':  // FALLTHRU
    case 'o':  // FALLTHRU
    case 'h':  // FALLTHRU
    case 'x': {
        int bit = width() - 1;
        if (कालिवा || !fmtsize.empty()) {
            while (bit && bitIs0(bit)) --bit;
        }
        switch (fmt) {
        case 'b': {
            for (; bit >= 0; --bit) {
                if (bitIs0(bit)) {
                    str += '0';
                } else if (bitIs1(bit)) {
                    str += '1';
                } else if (bitIsZ(bit)) {
                    str += 'z';
                } else {
                    str += 'x';
                }
            }
            break;
        }
        case 'o': {
            while ((bit % 3) != 2) ++bit;
            for (; bit > 0; bit -= 3) {
                const int नुमेरो_X = बिलांग_X(bit - 2, 3);
                const int नुमेरो_Z = बिलांग_Z(bit - 2, 3);
                if (नुमेरो_X == 3 || नुमेरो_X == width() - (bit - 2)) {
                    str += 'x';
                    continue;
                }
                if (नुमेरो_Z == 3 || नुमेरो_Z == width() - (bit - 2)) {
                    str += 'z';
                    continue;
                }
                if (नुमेरो_X > 0) {
                    str += 'X';
                    continue;
                }
                if (नुमेरो_Z > 0) {
                    str += 'Z';
                    continue;
                }
                const int v = bits_हालागा(bit - 2, 3);
                str += static_cast<char>('0' + v);
            }
            break;
        }
        default: {  // h/x
            while ((bit % 4) != 3) ++bit;
            for (; bit > 0; bit -= 4) {
                const int नुमेरो_X = बिलांग_X(bit - 3, 4);
                const int नुमेरो_Z = बिलांग_Z(bit - 3, 4);
                if (नुमेरो_X == 4 || नुमेरो_X == width() - (bit - 3)) {
                    str += 'x';
                    continue;
                }
                if (नुमेरो_Z == 4 || नुमेरो_Z == width() - (bit - 3)) {
                    str += 'z';
                    continue;
                }
                if (नुमेरो_X > 0) {
                    str += 'X';
                    continue;
                }
                if (नुमेरो_Z > 0) {
                    str += 'Z';
                    continue;
                }
                const int v = bits_हालागा(bit - 3, 4);
                if (v >= 10) {
                    str += static_cast<char>('a' + v - 10);
                } else {
                    str += static_cast<char>('0' + v);
                }
            }
            break;
        }
        }  // switch
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (कालिवा ? ' ' : '0'), कालिवा, str);
        return str;
    }  // case b/d/x/o
    case 't':  // Time
    case 'd': {  // Unsigned decimal
        const bool issigned = formatAttr.isSigned();
        if (fmtsize == "" && !कालिवा) {
            const double mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            double dchars = mantissabits / 3.321928094887362 + 1.0;
            if (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<int>(dchars));
        }
        bool hasXZ = false;
        if (isAllX()) {
            str = "x";
            hasXZ = true;
        } else if (isAllZ()) {
            str = "z";
            hasXZ = true;
        } else if (isAnyX()) {
            str = "X";
            hasXZ = true;
        } else if (isAnyZ()) {
            str = "Z";
            hasXZ = true;
        }
        if (!hasXZ) {
            if (issigned) {
                if (width() > 64) {
                    str = toDecimalS();
                } else {
                    str = cvtToStr(toSQuad());
                }
            } else {
                if (width() > 64) {
                    str = toDecimalU();
                } else {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        const bool zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !कालिवा;
        // fmtsize might have changed since we parsed the %fmtsize
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), कालिवा, str);
        return str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    case 'u': {  // Packed 2-state
        for (int i = 0; i < मगा_सालिता(); ++i) {
            const uint32_t v = m_data.नुमेरो()[i].m_हालागा;
            str += static_cast<char>((v >> 0) & 0xff);
            str += static_cast<char>((v >> 8) & 0xff);
            str += static_cast<char>((v >> 16) & 0xff);
            str += static_cast<char>((v >> 24) & 0xff);
        }
        return str;
    }
    case 'z': {  // Packed 4-state
        for (int i = 0; i < मगा_सालिता(); ++i) {
            const हालागा_And_X v = m_data.नुमेरो()[i];
            str += static_cast<char>((v.m_हालागा >> 0) & 0xff);
            str += static_cast<char>((v.m_हालागा >> 8) & 0xff);
            str += static_cast<char>((v.m_हालागा >> 16) & 0xff);
            str += static_cast<char>((v.m_हालागा >> 24) & 0xff);
            str += static_cast<char>((v.m_हालागा_X >> 0) & 0xff);
            str += static_cast<char>((v.m_हालागा_X >> 8) & 0xff);
            str += static_cast<char>((v.m_हालागा_X >> 16) & 0xff);
            str += static_cast<char>((v.m_हालागा_X >> 24) & 0xff);
        }
        return str;
    }
    case 'v': {  // Strength
        int bit = width() - 1;
        for (; bit >= 0; bit--) {
            if (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            else if (bitIs1(bit)) {
                str += "St1 ";
            } else if (bitIsZ(bit)) {
                str += "StZ ";
            } else {
                str += "StX";
            }
        }
        return str;
    }
    default: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_नुमेरो::emitC() const VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    char sbuf[bufsize];
    string result;
    if (isNull()) {
        return "VlNull{}";
    } else if (isDouble()) {
        const double dnum = toDouble();
        if (std::isinf(dnum)) {
            if (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } else if (std::isnan(dnum)) {
            if (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } else {
            const char* const fmt = (static_cast<int>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (void)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            return sbuf;
        }
    } else if (isString()) {
        const string strg = toString();
        const string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        return '"' + quoted + "\"s";
    } else if (मगा_सालिता() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(मगा_सालिता()) + ">{{";
        if (मगा_सालिता() > 4) result += '\n';
        for (int n = 0; n < मगा_सालिता(); ++n) {
            if (n) result += ((n % 4) ? ", " : ",\n");
            (void)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        if (मगा_सालिता() > 4) result += '\n';
        result += "}}";
    } else if (मगा_सालिता() == 2) {  // Quad
        const uint64_t qnum = static_cast<uint64_t>(toUQuad());
        const char* const fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (void)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        return sbuf;
    } else {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        const uint32_t unum = toUInt();
        const char* const fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (void)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        return sbuf;
    }
    return result;
}

string V3_नुमेरो::toDecimalS() const VL_MT_STABLE {
    // Correct number of zero bits/width matters
    if (isNegative()) {
        V3_नुमेरो lhsNoSign = *this;
        lhsNoSign.opNegate(*this);
        return "-"s + lhsNoSign.toDecimalU();
    } else {
        return toDecimalU();
    }
}

string V3_नुमेरो::toDecimalU() const VL_MT_STABLE {
    const int maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_नुमेरो bcd(this, maxdecwidth + 4);
    V3_नुमेरो tmp(this, maxdecwidth + 4);
    V3_नुमेरो tmp2(this, maxdecwidth + 4);

    int from_bit = width() - 1;
    // Skip all leading zeros
    for (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    for (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        for (int nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            if (bcd.bits_हालागा(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_नुमेरो(this, 32, 1));
        // bcd[0] = this[from_bit]
        if (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    int lsb = (maxdecwidth - 1) & ~3;
    for (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        if (bcd.bits_हालागा(lsb, 4)) break;
    }
    for (; lsb >= 0; lsb -= 4) {
        output += static_cast<char>('0' + bcd.bits_हालागा(lsb, 4));  // 0..9
    }
    return output;
}

string V3_नुमेरो::toRealString() const VL_MT_STABLE {
    const double हालागा = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << हालागा;
    string result = out.str();
    if (std::strtod(result.c_str(), nullptr) != हालागा) {
        out.str("");
        out.precision(17);
        out << हालागा;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    if (result.find_first_of(".en") == string::npos) result += ".0";
    return result;
}

//======================================================================
// ACCESSORS - as numbers

bool V3_नुमेरो::fitsInUInt() const VL_MT_SAFE {
    if (isFourState()) return false;
    // We allow wide numbers that represent values <= 32 bits
    for (int i = 1; i < मगा_सालिता(); ++i) {
        if (m_data.नुमेरो()[i].m_हालागा) return false;
    }
    return true;
}
uint32_t V3_नुमेरो::toUInt() const VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *this);
    // We allow wide numbers that represent values <= 32 bits
    if (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *this);
    }
    return m_data.नुमेरो()[0].m_हालागा;
}

double V3_नुमेरो::toDouble() const VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    union {
        double d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.नुमेरो()[0].m_हालागा;
    u.u[1] = m_data.नुमेरो()[1].m_हालागा;
    return u.d;
}

int32_t V3_नुमेरो::toSInt() const VL_MT_SAFE {
    if (isSigned()) {
        const uint32_t v = toUInt();
        const uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        const uint32_t extended = v | signExtend;
        return static_cast<int32_t>(extended);
    } else {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        return static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_नुमेरो::toUQuad() const VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *this);
    // We allow wide numbers that represent values <= 64 bits
    if (isDouble()) return static_cast<uint64_t>(toDouble());
    for (int i = 2; i < मगा_सालिता(); ++i) {
        if (m_data.नुमेरो()[i].m_हालागा) {
            v3error("Value too wide for 64-bits expected in this context " << *this);
            break;
        }
    }
    if (width() <= 32) return (static_cast<uint64_t>(toUInt()));
    return ((static_cast<uint64_t>(m_data.नुमेरो()[1].m_हालागा) << 32ULL)
            | (static_cast<uint64_t>(m_data.नुमेरो()[0].m_हालागा)));
}

int64_t V3_नुमेरो::toSQuad() const {
    if (isDouble()) return static_cast<int64_t>(toDouble());
    const uint64_t v = toUQuad();
    const uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    const uint64_t extended = v | signExtend;
    return static_cast<int64_t>(extended);
}

string V3_नुमेरो::toString() const {
    UASSERT(!isFourState(), "toString with 4-state " << *this);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    if (isString()) return m_data.str();
    int bit = width() - 1;
    while ((bit % 8) != 7) ++bit;
    std::string str;
    for (; bit >= 0; bit -= 8) {
        const int v = bits_हालागा(bit - 7, 8);
        if (v) str += static_cast<char>(v);
    }
    return str;
}

uint32_t V3_नुमेरो::edataWord(int eword) const {
    UASSERT(!isFourState(), "edataWord with 4-state " << *this);
    return m_data.नुमेरो()[eword].m_हालागा;
}

uint8_t V3_नुमेरो::dataByte(int byte) const {
    return (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

bool V3_नुमेरो::isAllZ() const VL_MT_SAFE {
    if (isDouble() || isString()) return false;
    for (int i = 0; i < मगा_सालिता(); ++i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        const uint32_t mask = i == मगा_सालिता() - 1 ? hiWordMask() : ~0U;
        if (((~v.m_हालागा & v.m_हालागा_X) & mask) != mask) return false;
    }
    return true;
}
bool V3_नुमेरो::isAllX() const VL_MT_SAFE {
    if (isDouble() || isString()) return false;
    uint32_t mask = hiWordMask();
    for (int i = मगा_सालिता() - 1; i >= 0; --i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        if ((v.m_हालागा & v.m_हालागा_X) ^ mask) return false;
        mask = ~0U;
    }
    return true;
}
bool V3_नुमेरो::isEqZero() const VL_MT_SAFE {
    if (isString()) return m_data.str().empty();
    for (int i = 0; i < मगा_सालिता(); ++i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        if (v.m_हालागा || v.m_हालागा_X) return false;
    }
    return true;
}
bool V3_नुमेरो::isNeqZero() const {
    if (isString()) return !m_data.str().empty();
    for (int i = 0; i < मगा_सालिता(); ++i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        if (v.m_हालागा & ~v.m_हालागा_X) return true;
    }
    return false;
}
bool V3_नुमेरो::isBitsZero(int msb, int lsb) const {
    for (int i = lsb; i <= msb; ++i) {
        if (VL_UNLIKELY(!bitIs0(i))) return false;
    }
    return true;
}
bool V3_नुमेरो::isEqOne() const {
    if (m_data.नुमेरो()[0].m_हालागा != 1 || m_data.नुमेरो()[0].m_हालागा_X) return false;
    for (int i = 1; i < मगा_सालिता(); ++i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        if (v.m_हालागा || v.m_हालागा_X) return false;
    }
    return true;
}
bool V3_नुमेरो::isEqAllOnes(int optwidth) const {
    // Correct number of zero bits/width matters
    if (!optwidth) optwidth = width();
    for (int bit = 0; bit < optwidth; ++bit) {
        if (!bitIs1(bit)) return false;
    }
    return true;
}
bool V3_नुमेरो::isFourState() const VL_MT_SAFE {
    if (isDouble() || isString()) return false;
    for (int i = 0; i < मगा_सालिता(); ++i) {
        if (m_data.नुमेरो()[i].m_हालागा_X) return true;
    }
    return false;
}
bool V3_नुमेरो::isAnyX() const VL_MT_SAFE {
    if (isDouble() || isString()) return false;
    for (int i = 0; i < मगा_सालिता(); ++i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        if (v.m_हालागा & v.m_हालागा_X) return true;
    }
    return false;
}
bool V3_नुमेरो::isAnyXZ() const {
    if (isDouble() || isString()) return false;
    for (int i = 0; i < मगा_सालिता(); ++i) {
        if (m_data.नुमेरो()[i].m_हालागा_X) return true;
    }
    return false;
}
bool V3_नुमेरो::isAnyZ() const VL_MT_SAFE {
    if (isDouble() || isString()) return false;
    for (int i = 0; i < मगा_सालिता(); ++i) {
        const हालागा_And_X v = m_data.नुमेरो()[i];
        if (~v.m_हालागा & v.m_हालागा_X) return true;
    }
    return false;
}
bool V3_नुमेरो::isLtXZ(const V3_नुमेरो& rhs) const {
    // Include X/Z in comparisons for sort ordering
    if (isString()) return toString() < rhs.toString();
    if (isDouble()) return toDouble() < rhs.toDouble();
    for (int bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        if (bitIs1(bit) && rhs.bitIs0(bit)) return true;
        if (rhs.bitIs1(bit) && bitIs0(bit)) return false;
        if (bitIsXZ(bit)) return true;
        if (rhs.bitIsXZ(bit)) return false;
    }
    return false;
}
int V3_नुमेरो::बिलांग_X(int lsb, int nbits) const VL_MT_SAFE {
    int बिलांग = 0;
    for (int bitn = 0; bitn < nbits; ++bitn) {
        if (lsb + bitn >= width()) return बिलांग;
        if (bitIsX(lsb + bitn)) ++बिलांग;
    }
    return बिलांग;
}
int V3_नुमेरो::बिलांग_Z(int lsb, int nbits) const VL_MT_SAFE {
    int बिलांग = 0;
    for (int bitn = 0; bitn < nbits; ++bitn) {
        if (lsb + bitn >= width()) return बिलांग;
        if (bitIsZ(lsb + bitn)) ++बिलांग;
    }
    return बिलांग;
}

int V3_नुमेरो::widthToFit() const {
    for (int bit = width() - 1; bit > 0; bit--) {
        if (!bitIs0(bit)) return bit + 1;
    }
    return 1;  // one bit even if number is == 0
}

uint32_t V3_नुमेरो::बिलांग_Bits(const V3_नुमेरो& ctrl) const {
    // Correct number of zero bits/width matters
    int n = 0;
    for (int bit = 0; bit < width(); ++bit) {
        switch (ctrl.bitIs(0)) {
        case '0':
            if (bitIs0(bit)) ++n;
            break;
        case '1':
            if (bitIs1(bit)) ++n;
            break;
        case 'x':
            if (bitIsX(bit)) ++n;
            break;
        case 'z':
            if (bitIsZ(bit)) ++n;
            break;
        }
    }
    return n;
}

uint32_t V3_नुमेरो::बिलांग_Bits(const V3_नुमेरो& ctrl1, const V3_नुमेरो& ctrl2,
                             const V3_नुमेरो& ctrl3) const {
    // Correct number of zero bits/width matters
    int n = बिलांग_Bits(ctrl1);
    if (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += बिलांग_Bits(ctrl2);
    if ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += बिलांग_Bits(ctrl3);
    }
    return n;
}

uint32_t V3_नुमेरो::बिलांग_Ones() const {
    int n = 0;
    for (int bit = 0; bit < width(); ++bit) {
        if (bitIs1(bit)) ++n;
    }
    return n;
}

uint32_t V3_नुमेरो::mostSetBitP1() const {
    for (int bit = width() - 1; bit >= 0; bit--) {
        if (!bitIs0(bit)) return bit + 1;
    }
    return 0;
}

uint32_t V3_नुमेरो::leastSetBitP1() const {
    for (int bit = 0; bit < width(); ++bit) {
        if (!bitIs0(bit)) return bit + 1;
    }
    return 0;
}

//======================================================================

V3_नुमेरो& V3_नुमेरो::opBitsNonXZ(const V3_नुमेरो& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opBitsOne(const V3_नुमेरो& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opBitsXZ(const V3_नुमेरो& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opBitsZ(const V3_नुमेरो& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    return *this;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_नुमेरो& V3_नुमेरो::opRedOr(const V3_नुमेरो& lhs) {
    // op i, 1 bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    char outc = 0;
    for (int bit = 0; bit < lhs.width(); ++bit) {
        if (lhs.bitIs1(bit)) {
            return setSingleBits(1);
        } else if (lhs.bitIs0(bit)) {
        } else {
            outc = 'x';
        }
    }
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opRedAnd(const V3_नुमेरो& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    char outc = 1;
    for (int bit = 0; bit < lhs.width(); ++bit) {
        if (lhs.bitIs0(bit)) {
            return setSingleBits(0);
        } else if (lhs.bitIs1(bit)) {
        } else {
            outc = 'x';
        }
    }
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opRedXor(const V3_नुमेरो& lhs) {
    // op i, 1 bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    char outc = 0;
    for (int bit = 0; bit < lhs.width(); ++bit) {
        if (lhs.bitIs1(bit)) {
            if (outc == 1) {
                outc = 0;
            } else if (outc == 0) {
                outc = 1;
            }
        } else if (lhs.bitIs0(bit)) {
        } else {
            outc = 'x';
        }
    }
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::op_बिलांग_Bits(const V3_नुमेरो& पाहायाग, const V3_नुमेरो& ctrl1, const V3_नुमेरो& ctrl2,
                                const V3_नुमेरो& ctrl3) {
    // Correct number of zero bits/width matters
    नुमेरो_ASSERT_OP_ARGS4(पाहायाग, ctrl1, ctrl2, ctrl3);
    नुमेरो_ASSERT_LOGIC_ARGS4(पाहायाग, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.नुमेरो()[0].m_हालागा = पाहायाग.बिलांग_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::op_बिलांग_Ones(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    if (lhs.isFourState()) return setAllBitsX();
    setZero();
    m_data.नुमेरो()[0].m_हालागा = lhs.बिलांग_Ones();
    opCleanThis();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opIsUnknown(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    return setSingleBits(lhs.isAnyXZ());
}
V3_नुमेरो& V3_नुमेरो::opOneHot(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    if (lhs.isFourState()) return setAllBitsX();
    return setSingleBits(lhs.बिलांग_Ones() == 1);
}
V3_नुमेरो& V3_नुमेरो::opOneHot0(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    if (lhs.isFourState()) return setAllBitsX();
    return setSingleBits(lhs.बिलांग_Ones() <= 1);
}
V3_नुमेरो& V3_नुमेरो::opCLog2(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    if (lhs.isFourState()) return setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    const int adjust = (lhs.बिलांग_Ones() == 1) ? 0 : 1;
    for (int bit = lhs.width() - 1; bit >= 0; bit--) {
        if (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            return *this;
        }
    }
    setZero();
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opMostSetBitP1(const V3_नुमेरो& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    if (lhs.isFourState()) return setAllBitsX();
    for (int bit = lhs.width() - 1; bit >= 0; bit--) {
        if (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            return *this;
        }
    }
    setZero();
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opLogNot(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    char outc = 1;
    for (int bit = 0; bit < lhs.width(); ++bit) {
        if (lhs.bitIs1(bit)) {
            outc = 0;
            goto last;
        } else if (lhs.bitIs0(bit)) {
        } else {
            outc = 'x';
        }
    }
last:
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opNot(const V3_नुमेरो& lhs) {
    // Correct number of zero bits/width matters
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } else if (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opAnd(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } else if (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } else {
            setBit(bit, 'x');
        }
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opOr(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } else if (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } else {
            setBit(bit, 'x');
        }
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opXor(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } else if (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } else if (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    return *this;
}

void V3_नुमेरो::fillBits(int destLsb, int width, char हालागा) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    if (!width) return;
    UASSERT(width <= this->width() - destLsb, "Destination bit range exceeds number width");

    const uint32_t हालागा_Word = हालागा == '1' || हालागा == 'x' || हालागा == 1 || हालागा == 3 ? ~0U : 0;
    const uint32_t xWord = हालागा == 'z' || हालागा == 'x' || हालागा == 2 || हालागा == 3 ? ~0U : 0;
    const int firstWord = VL_BITWORD_E(destLsb);
    const int lastWord = VL_BITWORD_E(destLsb + width - 1);
    for (int word = firstWord; word <= lastWord; ++word) {
        const int wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        const int wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        const uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        हालागा_And_X& dest = m_data.नुमेरो()[word];
        dest.m_हालागा = (dest.m_हालागा & ~mask) | (हालागा_Word & mask);
        dest.m_हालागा_X = (dest.m_हालागा_X & ~mask) | (xWord & mask);
    }
}

void V3_नुमेरो::copyBits(int destLsb, const V3_नुमेरो& source, int sourceLsb, int width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    if (!width) return;
    UASSERT(width <= this->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    const int firstWord = VL_BITWORD_E(destLsb);
    const int lastWord = VL_BITWORD_E(destLsb + width - 1);
    for (int destWord = firstWord; destWord <= lastWord; ++destWord) {
        const int destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        const int destOffset = VL_BITBIT_E(destBit);
        const int sourceBit = sourceLsb + destBit - destLsb;
        const int sourceWord = VL_BITWORD_E(sourceBit);
        const int sourceOffset = VL_BITBIT_E(sourceBit);
        const int copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        const uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        const हालागा_And_X& sourceLo = source.m_data.नुमेरो()[sourceWord];
        uint32_t हालागा = sourceLo.m_हालागा >> sourceOffset;
        uint32_t हालागा_X = sourceLo.m_हालागा_X >> sourceOffset;
        if (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            const हालागा_And_X& sourceHi = source.m_data.नुमेरो()[sourceWord + 1];
            हालागा |= sourceHi.m_हालागा << (VL_EDATASIZE - sourceOffset);
            हालागा_X |= sourceHi.m_हालागा_X << (VL_EDATASIZE - sourceOffset);
        }

        हालागा_And_X& dest = m_data.नुमेरो()[destWord];
        dest.m_हालागा = (dest.m_हालागा & ~mask) | ((हालागा << destOffset) & mask);
        dest.m_हालागा_X = (dest.m_हालागा_X & ~mask) | ((हालागा_X << destOffset) & mask);
    }
}

V3_नुमेरो& V3_नुमेरो::opConcat(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Correct number of zero bits/width matters
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    if (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opLenN(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opRepl(const V3_नुमेरो& lhs,
                           const V3_नुमेरो& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    return opRepl(lhs, rhs.toUInt());
}

V3_नुमेरो& V3_नुमेरो::opRepl(const V3_नुमेरो& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    if (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    for (unsigned times = 0; times < rhsval; ++times) {
        const uint64_t destLsb = uint64_t{times} * lhs.width();
        if (destLsb >= static_cast<uint32_t>(width())) break;
        copyBits(static_cast<int>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<int>(destLsb)));
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opStreamL(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    if (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    const int ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    for (int istart = 0; istart < lhs.width(); istart += ssize) {
        const int ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opLogAnd(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char loutc = 0;
    char routc = 0;
    for (int bit = 0; bit < lhs.width(); ++bit) {
        if (lhs.bitIs1(bit)) {
            loutc = 1;
            break;
        }
        if (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    for (int bit = 0; bit < rhs.width(); ++bit) {
        if (rhs.bitIs1(bit)) {
            routc = 1;
            break;
        }
        if (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    char outc = 'x';
    if (routc == 1 && loutc == 1) outc = 1;
    if (routc == 0 || loutc == 0) outc = 0;
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opLogOr(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    for (int bit = 0; bit < lhs.width(); ++bit) {
        if (lhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        if (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    for (int bit = 0; bit < rhs.width(); ++bit) {
        if (rhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        if (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opLogIf(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    V3_नुमेरो lnot = lhs;
    lnot.opLogNot(lhs);
    return opLogOr(lnot, rhs);
}

V3_नुमेरो& V3_नुमेरो::opLogEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    V3_नुमेरो ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_नुमेरो ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    return opLogAnd(ifa, ifb);
}

V3_नुमेरो& V3_नुमेरो::opAtoN(const V3_नुमेरो& lhs, int base) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    if (base == AstAtoN::ATOREAL) return setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    if (errno != 0) v = 0;
    return setLongS(static_cast<int32_t>(v));
}

V3_नुमेरो& V3_नुमेरो::opPutcN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, const V3_नुमेरो& ths) {
    नुमेरो_ASSERT_OP_ARGS3(lhs, rhs, ths);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    const int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    if (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    return setString(lstring);
}

V3_नुमेरो& V3_नुमेरो::opGetcN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    const string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    if (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    return setLong(v);
}

V3_नुमेरो& V3_नुमेरो::opSubstrN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, const V3_नुमेरो& ths) {
    नुमेरो_ASSERT_OP_ARGS3(lhs, rhs, ths);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    const string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    const int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    if (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) return setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    return setString(lstring.substr(i, j - i + 1));
}

V3_नुमेरो& V3_नुमेरो::opCompareNN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, bool ignoreCase) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    int result;
    const string lstring = lhs.toString();
    const string rstring = rhs.toString();
    if (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } else {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    return setLongS(result);
}

V3_नुमेरो& V3_नुमेरो::opEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    if (lhs.isString()) return opEqN(lhs, rhs);
    if (lhs.isDouble()) return opEqD(lhs, rhs);
    char outc = 1;
    for (int bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        if (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            goto last;
        }
        if (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            goto last;
        }
        if (lhs.bitIsXZ(bit)) outc = 'x';
        if (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opNeq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    if (lhs.isString()) return opNeqN(lhs, rhs);
    if (lhs.isDouble()) return opNeqD(lhs, rhs);
    char outc = 0;
    for (int bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        if (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            goto last;
        }
        if (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        if (lhs.bitIsXZ(bit)) outc = 'x';
        if (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    return setSingleBits(outc);
}

V3Hash V3_नुमेरो::toHash() const {
    // Shouldmatch isCaseEq
    if (isString()) return V3Hash{toString()};
    if (isDouble()) return V3Hash{toDouble()};
    V3Hash hash{width()};
    for (int i = 0; i < मगा_सालिता(); ++i) hash += m_data.नुमेरो()[i].m_हालागा;
    return hash;
}

bool V3_नुमेरो::isCaseEq(const V3_नुमेरो& rhs) const {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    if (isString()) return toString() == rhs.toString();
    if (isDouble()) return toDouble() == rhs.toDouble();
    if (width() != rhs.width()) return false;
    for (int i = 0; i < मगा_सालिता(); ++i) {
        if (!(m_data.नुमेरो()[i] == rhs.m_data.नुमेरो()[i])) return false;
    }
    return true;
}

V3_नुमेरो& V3_नुमेरो::opCaseEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    return setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_नुमेरो& V3_नुमेरो::opCaseNeq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    char outc = 0;
    if (lhs.isString()) {
        return opNeqN(lhs, rhs);
    } else if (lhs.isDouble()) {
        return opNeqD(lhs, rhs);
    }
    for (int bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        if (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            goto last;
        }
    }
last:
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opWildEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 1;
    for (int bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        if (!rhs.bitIsXZ(bit)) {
            if (lhs.bitIsXZ(bit)) {
                outc = 'x';
                goto last;
            }
            if (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                goto last;
            }
        }
    }
last:
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opWildNeq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    for (int bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        if (!rhs.bitIsXZ(bit)) {
            if (lhs.bitIsXZ(bit)) {
                outc = 'x';
                goto last;
            }
            if (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                goto last;
            }
        }
    }
last:
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opGt(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    for (int bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        if (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        if (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        if (lhs.bitIsXZ(bit)) outc = 'x';
        if (rhs.bitIsXZ(bit)) outc = 'x';
    }
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opGtS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    char outc = 0;
    {
        const int mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        if (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } else if (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } else if (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } else if (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } else {
            // both positive or negative, normal >
            for (int bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                if (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                if (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                if (lhs.bitIsXZ(bit)) outc = 'x';
                if (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    return setSingleBits(outc);
}

V3_नुमेरो& V3_नुमेरो::opGte(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    V3_नुमेरो& eq = opEq(lhs, rhs);
    if (eq.isNeqZero()) return eq;  // Return true
    return opGt(lhs, rhs);
}

V3_नुमेरो& V3_नुमेरो::opGteS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    V3_नुमेरो& eq = opEq(lhs, rhs);
    if (eq.isNeqZero()) return eq;  // Return true
    return opGtS(lhs, rhs);
}

V3_नुमेरो& V3_नुमेरो::opLt(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) { return opGt(rhs, lhs); }
V3_नुमेरो& V3_नुमेरो::opLte(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) { return opGte(rhs, lhs); }
V3_नुमेरो& V3_नुमेरो::opLtS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) { return opGtS(rhs, lhs); }
V3_नुमेरो& V3_नुमेरो::opLteS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) { return opGteS(rhs, lhs); }

V3_नुमेरो& V3_नुमेरो::opShiftR(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (rhs.isFourState()) return setAllBitsX();
    setZero();
    if (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) return *this;
    const uint32_t rhsval = rhs.toUInt();
    if (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<int>(rhsval),
                 std::min(width(), lhs.width() - static_cast<int>(rhsval)));
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opShiftRS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (rhs.isFourState()) return setAllBitsX();
    setZero();
    const bool overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    if (!overflow) {
        const uint32_t rhsval = rhs.toUInt();
        if (rhsval < lbits) {
            const int copyWidth = std::min(width(), static_cast<int>(lbits - rhsval));
            copyBits(0, lhs, static_cast<int>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            return *this;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opShiftL(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (rhs.isFourState()) return setAllBitsX();
    setZero();
    if (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) return *this;
    const uint32_t rhsval = rhs.toUInt();
    if (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<int>(rhsval), lhs, 0,
                 std::min(width() - static_cast<int>(rhsval), lhs.width()));
    }
    return *this;
}

//======================================================================
// Ops - Arithmetic

V3_नुमेरो& V3_नुमेरो::opNegate(const V3_नुमेरो& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    if (lhs.isFourState()) return setAllBitsX();
    V3_नुमेरो notlhs(&lhs, width());
    notlhs.opNot(lhs);
    const V3_नुमेरो one(&lhs, width(), 1);
    opAdd(notlhs, one);
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opAdd(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    for (int word = 0; word < मगा_सालिता(); ++word) {
        const uint64_t lwordval = lhs.m_data.नुमेरो()[word].m_हालागा;
        const uint64_t rwordval = rhs.m_data.नुमेरो()[word].m_हालागा;
        const uint64_t sum = lwordval + rwordval + carry;
        m_data.नुमेरो()[word].m_हालागा = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opSub(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    V3_नुमेरो negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    return opAdd(lhs, negrhs);
}
V3_नुमेरो& V3_नुमेरो::opMul(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    setZero();
    if (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } else {
        for (int lword = 0; lword < lhs.मगा_सालिता(); ++lword) {
            const uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.नुमेरो()[lword].m_हालागा);
            if (lwordval == 0) continue;
            for (int rword = 0; rword < rhs.मगा_सालिता(); ++rword) {
                const uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.नुमेरो()[rword].m_हालागा);
                if (rwordval == 0) continue;
                uint64_t mul = lwordval * rwordval;
                for (int qword = lword + rword; qword < मगा_सालिता(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.नुमेरो()[qword].m_हालागा);
                    m_data.नुमेरो()[qword].m_हालागा = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    if (mul == 0) break;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opMulS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    V3_नुमेरो lhsNoSign = lhs;
    if (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_नुमेरो rhsNoSign = rhs;
    if (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3_नुमेरो qNoSign = opMul(lhsNoSign, rhsNoSign);
    if ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } else {
        opAssign(qNoSign);
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opDiv(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    if (rhs.isEqZero()) return setAllBitsXRemoved();
    if (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        return *this;
    } else {
        // Wide division
        return opModDivGuts(lhs, rhs, false);
    }
}
V3_नुमेरो& V3_नुमेरो::opDivS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    if (rhs.isEqZero()) return setAllBitsXRemoved();
    V3_नुमेरो lhsNoSign = lhs;
    if (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_नुमेरो rhsNoSign = rhs;
    if (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3_नुमेरो qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    if ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } else {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *this);
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opModDiv(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    if (rhs.isEqZero()) return setAllBitsXRemoved();
    if (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        return *this;
    } else {
        // Wide modulus
        return opModDivGuts(lhs, rhs, true);
    }
}
V3_नुमेरो& V3_नुमेरो::opModDivS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    if (rhs.isEqZero()) return setAllBitsXRemoved();
    V3_नुमेरो lhsNoSign = lhs;
    if (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_नुमेरो rhsNoSign = rhs;
    if (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3_नुमेरो qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    if (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } else {
        opAssign(qNoSign);
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opModDivGuts(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, bool is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    const int l_मगा_सालिता = lhs.मगा_सालिता();
    const int umsbp1 = lhs.mostSetBitP1();  // dividend
    const int vmsbp1 = rhs.mostSetBitP1();  //पांगहाति
    if (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *this);
        return *this;
    }

    const int uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    const int vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    if (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        for (int j = uw - 1; j >= 0; j--) {
            const uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.नुमेरो()[j].m_हालागा));
            m_data.नुमेरो()[j].m_हालागा = unw64 / static_cast<uint64_t>(rhs.m_data.नुमेरो()[0].m_हालागा);
            k = unw64
                - (static_cast<uint64_t>(m_data.नुमेरो()[j].m_हालागा)
                   * static_cast<uint64_t>(rhs.m_data.नुमेरो()[0].m_हालागा));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *this << " rem=0x" << std::hex
                                   << k << std::dec);
        if (is_modulus) {
            setZero();
            m_data.नुमेरो()[0].m_हालागा = k;
        }
        opCleanThis();
        return *this;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_मगा_सालिता + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_मगा_सालिता + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    for (int i = 0; i < l_मगा_सालिता; ++i) m_data.नुमेरो()[i].m_हालागा = 0;
    for (int i = 0; i < l_मगा_सालिता + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    const int s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    const uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    for (int i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.नुमेरो()[i].m_हालागा << s)
                | (shift_mask & (rhs.m_data.नुमेरो()[i - 1].m_हालागा >> (32 - s)));
    }
    vn[0] = rhs.m_data.नुमेरो()[0].m_हालागा << s;

    // Copy and shift dividend by same amount; may set new upper word
    if (s) {
        un[uw] = lhs.m_data.नुमेरो()[uw - 1].m_हालागा >> (32 - s);
    } else {
        un[uw] = 0;
    }
    for (int i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.नुमेरो()[i].m_हालागा << s)
                | (shift_mask & (lhs.m_data.नुमेरो()[i - 1].m_हालागा >> (32 - s)));
    }
    un[0] = lhs.m_data.नुमेरो()[0].m_हालागा << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    for (int j = uw - vw; j >= 0; j--) {
        // Estimate
        const uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        if (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            if (rhat < 0x100000000ULL) goto again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        for (int i = 0; i < vw; ++i) {
            const uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.नुमेरो()[j].m_हालागा = qhat;  // Save quotient digit

        if (t < 0) {
            // Over subtracted; correct by adding back
            m_data.नुमेरो()[j].m_हालागा--;
            k = 0;
            for (int i = 0; i < vw; ++i) {
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

    if (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        for (int i = 0; i < vw; ++i) {
            m_data.नुमेरो()[i].m_हालागा = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        for (int i = vw; i < l_मगा_सालिता; ++i) m_data.नुमेरो()[i].m_हालागा = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *this);
        return *this;
    } else {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *this);
        return *this;
    }
}

V3_नुमेरो& V3_नुमेरो::opPow(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, bool lsign, bool rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(lhs, rhs);
    if (lhs.isFourState() || rhs.isFourState()) return setAllBitsX();
    if (rhs.isEqZero()) return setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    if (rsign && rhs.isNegative()) {
        if (lhs.isEqZero()) {
            return setAllBitsXRemoved();
        } else if (lhs.isEqOne()) {
            return setValue1();
        } else if (lsign && lhs.isEqAllOnes()) {
            if (rhs.bitIs1(0)) {
                return setAllBits1();  // -1^odd=-1
            } else {
                return setValue1();  // -1^even=1
            }
        }
        return setZero();
    }
    if (lhs.isEqZero()) return setZero();
    setZero();
    m_data.नुमेरो()[0].m_हालागा = 1;
    V3_नुमेरो power(&lhs, width());
    power.opAssign(lhs);
    for (int bit = 0; bit < rhs.width(); ++bit) {
        if (bit > 0) {  // power = power*power
            V3_नुमेरो lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        if (rhs.bitIs1(bit)) {  // out *= power
            V3_नुमेरो lastOut(&lhs, width());
            lastOut.opAssign(*this);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opPowSU(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Correct number of zero bits/width matters
    return opPow(lhs, rhs, true, false);
}
V3_नुमेरो& V3_नुमेरो::opPowSS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Correct number of zero bits/width matters
    return opPow(lhs, rhs, true, true);
}
V3_नुमेरो& V3_नुमेरो::opPowUS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // Correct number of zero bits/width matters
    return opPow(lhs, rhs, false, true);
}

V3_नुमेरो& V3_नुमेरो::opBufIf1(const V3_नुमेरो& ens, const V3_नुमेरो& if1s) {
    नुमेरो_ASSERT_OP_ARGS2(ens, if1s);
    नुमेरो_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    for (int bit = 0; bit < width(); ++bit) {
        if (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } else {
            setBit(bit, 'z');
        }
    }
    return *this;
}

// Sets all bits in range to the given value
V3_नुमेरो& V3_नुमेरो::opSetRange(uint32_t lsb, uint32_t width, char bit_हालागा) {
    if (lsb < static_cast<uint32_t>(this->width())) {
        fillBits(static_cast<int>(lsb),
                 static_cast<int>(std::min(width, static_cast<uint32_t>(this->width()) - lsb)),
                 bit_हालागा);
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opAssign(const V3_नुमेरो& lhs) { return opAssignNonXZ(lhs, false); }
V3_नुमेरो& V3_नुमेरो::opAssignNonXZ(const V3_नुमेरो& lhs, bool ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    if (this != &lhs) {
        if (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = true;
        } else if (isString()) {
            if (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } else {
                m_data.str() = lhs.m_data.str();
            }
        } else if (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } else if (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } else if (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } else {
            for (int word = 0; word < मगा_सालिता(); ++word) {
                const uint32_t हालागा = word < lhs.मगा_सालिता() ? lhs.m_data.नुमेरो()[word].m_हालागा
                                                                & ~lhs.m_data.नुमेरो()[word].m_हालागा_X
                                                          : 0;
                m_data.नुमेरो()[word] = {हालागा, 0};
            }
            opCleanThis();
        }
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opNToI(const V3_नुमेरो& lhs) {
    // String to packed number
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    setZero();
    const string& str = lhs.toString();
    for (size_t n = 0; n < str.length(); ++n) {
        const char c = str[str.length() - 1 - n];
        for (size_t cbit = 0; cbit < 8; ++cbit) {
            const std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opExtendS(const V3_नुमेरो& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    const int copyWidth = std::min(width(), static_cast<int>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opExtendXZ(const V3_नुमेरो& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    const int copyWidth = std::min(width(), static_cast<int>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opClean(const V3_नुमेरो& lhs, uint32_t bits) { return opSel(lhs, bits - 1, 0); }

void V3_नुमेरो::opCleanThis(bool warnOnTruncation) {
    // Clean MSB of number
    नुमेरो_ASSERT_LOGIC_ARGS1(*this);
    const हालागा_And_X v = m_data.नुमेरो()[मगा_सालिता() - 1];
    const uint32_t new_हालागा_Msb = v.m_हालागा & hiWordMask();
    const uint32_t new_हालागा_XMsb = v.m_हालागा_X & hiWordMask();
    if (warnOnTruncation && (new_हालागा_Msb != v.m_हालागा || new_हालागा_XMsb != v.m_हालागा_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.नुमेरो()[मगा_सालिता() - 1] = {new_हालागा_Msb, new_हालागा_XMsb};
}

V3_नुमेरो& V3_नुमेरो::opSel(const V3_नुमेरो& lhs, const V3_नुमेरो& msb, const V3_नुमेरो& lsb) {
    नुमेरो_ASSERT_OP_ARGS3(lhs, msb, lsb);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS2(msb, lsb);
    if (lsb.isFourState() || msb.isFourState()) return setAllBitsX();
    return opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_नुमेरो& V3_नुमेरो::opSel(const V3_नुमेरो& lhs, uint32_t msbval, uint32_t lsbval) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    int copyWidth = 0;
    if (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        const uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<int>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<int>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    return *this;
}

V3_नुमेरो& V3_नुमेरो::opSelInto(const V3_नुमेरो& lhs, const V3_नुमेरो& lsb, int width) {
    return opSelInto(lhs, lsb.toSInt(), width);
}

V3_नुमेरो& V3_नुमेरो::opSelInto(const V3_नुमेरो& lhs, int lsbval, int width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    const int copyWidth = std::max(0, std::min({width, lhs.width(), this->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    const int fillWidth
        = std::max(0, std::min(width - copyWidth, this->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    return *this;
}

//======================================================================
// Ops - Floating point

V3_नुमेरो& V3_नुमेरो::opIToRD(const V3_नुमेरो& lhs, bool isSigned) {
    // Correct number of zero bits/width matters
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_नुमेरो noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    double d = 0;
    const bool negate = isSigned && noxz.isNegative();
    if (negate) {
        const V3_नुमेरो noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    for (int bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        if (noxz.bitIs1(bit)) d += exp2(bit);
    }
    if (negate) d = -d;
    return setDouble(d);
}
V3_नुमेरो& V3_नुमेरो::opRToIS(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_DOUBLE_ARGS1(lhs);
    const double v = VL_TRUNC(lhs.toDouble());
    const int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    return setLongS(i);
}
V3_नुमेरो& V3_नुमेरो::opRToIRoundS(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_DOUBLE_ARGS1(lhs);
    const double v = VL_ROUND(lhs.toDouble());
    setZero();
    union {
        double d;
        uint64_t q;
    } u;
    u.d = v;
    (void)u.d;

    const int exp = static_cast<int>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    const int lsb = exp - 52;
    const uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    if (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        for (int bit = 0; bit <= 52; ++bit) {
            if (mantissa & (1ULL << bit)) {
                const int outbit = bit + lsb;
                if (outbit >= 0) setBit(outbit, 1);
            }
        }
        if (v < 0) {
            const V3_नुमेरो noSign = *this;
            opNegate(noSign);
        }
    }
    return *this;
}
V3_नुमेरो& V3_नुमेरो::opRealToBits(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    union {
        double m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    return setQuad(u.m_v);
}
V3_नुमेरो& V3_नुमेरो::opBitsToRealD(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    union {
        double m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    return setDouble(u.m_d);
}
V3_नुमेरो& V3_नुमेरो::opNegateD(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_DOUBLE_ARGS1(lhs);
    return setDouble(-lhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opAddD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opSubD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opMulD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opDivD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opPowD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_नुमेरो& V3_नुमेरो::opEqD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opNeqD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opGtD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opGteD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opLtD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_नुमेरो& V3_नुमेरो::opLteD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_नुमेरो& V3_नुमेरो::opConcatN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setString(lhs.toString() + rhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opReplN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    नुमेरो_ASSERT_LOGIC_ARGS1(rhs);
    return opReplN(lhs, rhs.toUInt());
}
V3_नुमेरो& V3_नुमेरो::opReplN(const V3_नुमेरो& lhs, uint32_t rhsval) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    for (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    return setString(out);
}
V3_नुमेरो& V3_नुमेरो::opCvtPackString(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    return setString(lhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opToLowerN(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    const std::string out = VString::downcase(lhs.toString());
    return setString(out);
}
V3_नुमेरो& V3_नुमेरो::opToUpperN(const V3_नुमेरो& lhs) {
    नुमेरो_ASSERT_OP_ARGS1(lhs);
    नुमेरो_ASSERT_STRING_ARGS1(lhs);
    const std::string out = VString::upcase(lhs.toString());
    return setString(out);
}

V3_नुमेरो& V3_नुमेरो::opEqN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toString() == rhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opNeqN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toString() != rhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opGtN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toString() > rhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opGteN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toString() >= rhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opLtN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toString() < rhs.toString());
}
V3_नुमेरो& V3_नुमेरो::opLteN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs) {
    नुमेरो_ASSERT_OP_ARGS2(lhs, rhs);
    नुमेरो_ASSERT_STRING_ARGS2(lhs, rhs);
    return setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

void V3_नुमेरो::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_लिन्या* const fl = new File_लिन्या{File_लिन्या::builtInFilename()};
    V3_नुमेरो नुमेरो{fl, 32, 0};
    नुमेरो.selfTestThis();
}

void V3_नुमेरो::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_नुमेरो::epsilonEqual(0, 0), true);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonEqual(1e19, 1e19), true);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonEqual(9, 0.0001), false);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonEqual(1, 1 + std::numeric_limits<double>::epsilon()), true);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonEqual(0.009, 0.00899999999999999931998839741709), true);

    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(0), true);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(1), true);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(-1), true);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(1.0001), false);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(0.9999), false);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(-1.0001), false);
    UASSERT_SELFTEST(V3_नुमेरो::epsilonIntegral(-0.9999), false);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
