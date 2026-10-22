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

constexpr පූර්ණ_සංඛ්‍යා MAX_SPRINTF_DOUBLE_SIZE
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
    නම් (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } නැතහොත් නම් (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } නැතහොත් {
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
    නම් (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } නැතහොත් {
        init(nodep, std::max<පූර්ණ_සංඛ්‍යා>(str.length() * 8, 1));
        සඳහා (unsigned pos = 0; pos < str.length(); ++pos) {
            const පූර්ණ_සංඛ්‍යා topos = str.length() - 1 - pos;
            ValueAndX& v = m_data.num()[topos / 4];
            සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < 8; ++bit) {
                නම් (str[pos] & (1UL << bit)) v.m_value |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = true;
    opCleanThis(true);
}

V3Number::V3Number(AstNode* nodep, const AstNodeDType* nodedtypep) {
    නම් (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } නැතහොත් නම් (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } නැතහොත් නම් (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } නැතහොත් {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

void V3Number::create(const var_na* sourcep) {
    m_data.setLogic();
    const var_na* value_startp = sourcep;
    සඳහා (const var_na* cp = sourcep; *cp; ++cp) {
        නම් (*cp == '\'') {
            value_startp = cp + 1;
            bha_nga;
        }
    }

    bool userSized = false;
    bool unbased = false;
    var_na base = '\0';
    නම් (value_startp != sourcep) {  // Has a '
        string widthn;
        const var_na* cp = sourcep;
        සඳහා (; *cp; ++cp) {
            නම් (*cp == '\'') {
                ++cp;
                bha_nga;
            }
            නම් (*cp != '_') widthn += *cp;
        }
        අතරතුර (*cp == '_') cp++;
        නම් (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(true);
        }
        නම් (*cp) {
            base = *cp;
            ++cp;
        }
        value_startp = cp;

        userSized = widthn.length() != 0;
        නම් (std::atoi(widthn.c_str())) {
            නම් (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.maxNumWidth()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.maxNumWidth(), true);
            } නැතහොත් {
                width(std::atoi(widthn.c_str()), true);
            }
        }
    } නැතහොත් {
        unbased = true;
        base = 'd';
    }

    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    නම් (base == '0') {
        width(1, false);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = true;
    } නැතහොත් නම් (base == '1') {
        width(1, false);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = true;
    } නැතහොත් නම් (std::tolower(base) == 'z') {
        width(1, false);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = true;
    } නැතහොත් නම් (std::tolower(base) == 'x') {
        width(1, false);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = true;
    }
    // Otherwise...
    නැතහොත් නම් (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        const පූර්ණ_සංඛ්‍යා widthBound = std::max<පූර්ණ_සංඛ්‍යා>(32, std::strlen(value_startp) * 4);
        width(widthBound, false);  // Will change width below
        නම් (unbased) isSigned(true);  // Also says the spec.
    }

    // Ignore leading blanks
    අතරතුර (*value_startp == '_' || std::isspace(*value_startp)) ++value_startp;
    නම් (!*value_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    නම් (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    පූර්ණ_සංඛ්‍යා obit = 0;  // Start at LSB
    පූර්ණ_සංඛ්‍යා base_align = 1;
    නම් (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        අතරතුර (*value_startp == '_' || *value_startp == '0') ++value_startp;
        // Convert decimal number to hex
        පූර්ණ_සංඛ්‍යා olen = 0;
        uint32_t val = 0;
        පූර්ණ_සංඛ්‍යා got_x = 0;
        පූර්ණ_සංඛ්‍යා got_z = 0;
        පූර්ණ_සංඛ්‍යා got_01 = 0;
        bool warnedTooMany = false;
        සඳහා (const var_na* cp = value_startp; *cp; ++cp) {
            switch (std::tolower(*cp)) {
            pak_sa '0':  // FALLTHRU
            pak_sa '1':  // FALLTHRU
            pak_sa '2':  // FALLTHRU
            pak_sa '3':  // FALLTHRU
            pak_sa '4':  // FALLTHRU
            pak_sa '5':  // FALLTHRU
            pak_sa '6':  // FALLTHRU
            pak_sa '7':  // FALLTHRU
            pak_sa '8':  // FALLTHRU
            pak_sa '9': {
                නම් (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.num()[0].m_value = val;
                } නැතහොත් {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3Number product(this, width() + 4);  // +4 for overflow detection
                    const V3Number ten(this, width() + 4, 10);
                    const V3Number addend(this, width(), (*cp - '0'));
                    product.opMul(*this, ten);
                    opAdd(product, addend);
                    නම් (!warnedTooMany && product.bitsValue(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = true;
                    }
                }
                ++olen;
                got_01 = 1;
                bha_nga;
            }
            pak_sa 'z':
            pak_sa '?': {
                got_z = 1;
                නම් (!userSized) width(32, false);
                setAllBitsZ();
                bha_nga;
            }
            pak_sa 'x': {
                got_x = 1;
                නම් (!userSized) width(32, false);
                setAllBitsX();
                bha_nga;
            }
            pak_sa '_': bha_nga;
            default: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                bha_nga;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        නම් ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << value_startp);
        }
    } නැතහොත් {
        // Convert bin/octal number to hex
        සඳහා (const var_na* cp = value_startp + std::strlen(value_startp) - 1; cp >= value_startp;
             cp--) {
            නම් (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                bha_nga;
            }
            switch (std::tolower(base)) {
            pak_sa 'b': {
                base_align = 1;
                switch (std::tolower(*cp)) {
                pak_sa '0': setBit(obit++, 0); bha_nga;
                pak_sa '1': setBit(obit++, 1); bha_nga;
                pak_sa 'z':
                pak_sa '?': setBit(obit++, 'z'); bha_nga;
                pak_sa 'x': setBit(obit++, 'x'); bha_nga;
                pak_sa '_': bha_nga;
                default: v3error("Illegal character in binary constant: " << *cp);
                }
                bha_nga;
            }

            pak_sa 'o':
            pak_sa 'c': {
                base_align = 3;
                switch (std::tolower(*cp)) {  // clang-format off
                pak_sa '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  bha_nga;
                pak_sa '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  bha_nga;
                pak_sa '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  bha_nga;
                pak_sa '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  bha_nga;
                pak_sa '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  bha_nga;
                pak_sa '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  bha_nga;
                pak_sa '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  bha_nga;
                pak_sa '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  bha_nga;
                pak_sa 'z':
                pak_sa '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); bha_nga;
                pak_sa 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); bha_nga;
                pak_sa '_': bha_nga;
                // clang-format on
                default: v3error("Illegal character in octal constant");
                }
                bha_nga;
            }

            pak_sa 'h':  // FALLTHRU
            pak_sa 'x': {
                base_align = 4;
                switch (std::tolower(*cp)) {  // clang-format off
                pak_sa '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); bha_nga;
                pak_sa '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); bha_nga;
                pak_sa '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); bha_nga;
                pak_sa '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); bha_nga;
                pak_sa '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); bha_nga;
                pak_sa '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); bha_nga;
                pak_sa '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); bha_nga;
                pak_sa '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); bha_nga;
                pak_sa '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); bha_nga;
                pak_sa '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); bha_nga;
                pak_sa 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); bha_nga;
                pak_sa 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); bha_nga;
                pak_sa 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); bha_nga;
                pak_sa 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); bha_nga;
                pak_sa 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); bha_nga;
                pak_sa 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); bha_nga;
                pak_sa 'z':
                pak_sa '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); bha_nga;
                pak_sa 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); bha_nga;
                    // clang-format on
                pak_sa '_': bha_nga;
                default:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    bha_nga;
                }  // LCOV_EXCL_STOP
                bha_nga;
            }
            default:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                bha_nga;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    නම් (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), false);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    අතරතුර (obit <= width() && obit && bitIsXZ(obit - 1)) {
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
    නම් (!nodep) ආපසු;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

bool V3Number::epsilonEqual(dviguu_na a, dviguu_na b) {
    ආපසු std::fabs(a - b)
           <= (std::numeric_limits<dviguu_na>::epsilon() * std::max(1.0, std::max(a, b)));
}

bool V3Number::epsilonIntegral(dviguu_na a) {
    const dviguu_na dint = static_cast<dviguu_na>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ආපසු epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

පූර්ණ_සංඛ්‍යා V3Number::log2b(uint32_t num) {
    // See also opCLog2
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 31; bit > 0; bit--) {
        නම් (num & (1ULL << bit)) ආපසු bit;
    }
    ආපසු 0;
}

පූර්ණ_සංඛ්‍යා V3Number::log2bQuad(uint64_t num) {
    නම් (num >> 32ULL) ආපසු 32 + log2b(num >> 32ULL);
    ආපසු log2b(num);
}

//======================================================================
// Setters

V3Number& V3Number::setZero() {
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};
    ආපසු *this;
}
V3Number& V3Number::setQuad(uint64_t value) {
    setZero();
    m_data.num()[0].m_value = value & 0xffffffffULL;
    නම් (width() > 32) m_data.num()[1].m_value = (value >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::setLong(uint32_t value) {
    setZero();
    m_data.num()[0].m_value = value;
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::setLongS(int32_t value) {
    setZero();
    sa_ngha {
        uint32_t u;
        int32_t s;
    } u;
    u.s = value;
    (void)u.s;
    m_data.num()[0].m_value = u.u;
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::setDouble(dviguu_na value) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    sa_ngha {
        dviguu_na d;
        uint32_t u[2];
    } u;
    u.d = value;
    (void)u.d;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 2; i < words(); ++i) m_data.num()[i] = {0, 0};
    m_data.num()[0].m_value = u.u[0];
    m_data.num()[1].m_value = u.u[1];
    ආපසු *this;
}
V3Number& V3Number::setSingleBits(var_na value) {
    සඳහා (පූර්ණ_සංඛ්‍යා i = 1 /*upper*/; i < words(); ++i) m_data.num()[i] = {0, 0};
    m_data.num()[0] = {(value == '1' || value == 'x' || value == 1 || value == 3),
                       (value == 'z' || value == 'x' || value == 2 || value == 3)};
    ආපසු *this;
}

V3Number& V3Number::setAllBits0() {
    setZero();
    ආපසු *this;
}
V3Number& V3Number::setAllBits1() {
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) m_data.num()[i] = {~0U, 0};
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) m_data.num()[i] = {~0U, ~0U};
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::setAllBitsZ() {
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) m_data.num()[i] = {0, ~0U};
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::setAllBitsXRemoved() {
    නම් (!v3Global.constRemoveXs()) {
        ආපසු setAllBitsX();
    } නැතහොත් {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        නම් (v3Global.opt.xAssign() == "1") {
            ආපසු setAllBits1();
        } නැතහොත් {
            ආපසු setAllBits0();
        }
    }
}
V3Number& V3Number::setValue1() {
    m_data.num()[0] = {1, 0};
    සඳහා (පූර්ණ_සංඛ්‍යා i = 1; i < words(); ++i) m_data.num()[i] = {0, 0};
    ආපසු *this;
}

void V3Number::setBitX0(පූර්ණ_සංඛ්‍යා bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3Number& V3Number::setMask(පූර්ණ_සංඛ්‍යා nbits, පූර්ණ_සංඛ්‍යා lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    ආපසු *this;
}

//======================================================================
// ACCESSORS - as strings

string V3Number::ascii(bool prefixed, bool cleanVerilog) const VL_MT_STABLE {
    // Correct number of zero bits/width matters
    නම් (is1Step()) ආපසු "1step";
    නම් (isNull()) ආපසු "null";
    std::ostringstream out;
    නම් (isDouble()) {
        out.precision(17);
        නම් (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } නැතහොත් {
            out << toDouble();
            නම් (toDouble() == floor(toDouble())) out << ".0";
        }
        ආපසු out.str();
    } නැතහොත් නම් (isString()) {
        ආපසු '"' + toString() + '"';
    } නැතහොත් {
        නම් (VL_UNCOVERABLE((m_data.num()[words() - 1].m_value | m_data.num()[words() - 1].m_valueX)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    නම් (prefixed) {
        නම් (sized()) {
            out << width() << "'";
        } නැතහොත් නම් (autoExtend() && width() == 1) {
            out << "'";
            නම් (bitIs0(0)) {
                out << '0';
            } නැතහොත් නම් (bitIs1(0)) {
                out << '1';
            } නැතහොත් නම් (bitIsZ(0)) {
                out << 'z';
            } නැතහොත් {
                out << 'x';
            }
            ආපසු out.str();
        } නැතහොත් {
            නම් (cleanVerilog) {
                out << "'";
            } නැතහොත් {
                out << "?" << width() << "?";
            }
        }
        නම් (isSigned()) out << "s";
    }

    const bool binary = (isFourState()
#ifdef V3NUMBER_ASCII_BINARY
                         // cppcheck-suppress konwnConditionTrueFalse
                         || true
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    නම් (binary) {
        out << "b";
        out << displayed("%0b");
    } නැතහොත් {
        නම් (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    නම් (isNull()) {
        නම් (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } නැතහොත් {
            out << " [null]";
        }
    }
    ආපසු out.str();
}

bool V3Number::displayedFmtHasArg(var_na format, bool isScan) {
    // Is this a format letter that takes an argument in runtime?
    (void)isScan;
    switch (std::tolower(format)) {
    pak_sa 'b': ආපසු true;  // Binary
    pak_sa 'c': ආපසු true;  // Character
    pak_sa 'd': ආපසු true;  // Decimal
    pak_sa 'e': ආපසු true;  // Floating
    pak_sa 'f': ආපසු true;  // Floating
    pak_sa 'g': ආපසු true;  // Floating
    pak_sa 'h': ආපසු true;  // Hex
    pak_sa 'o': ආපසු true;  // Octal
    pak_sa 'p': ආපසු true;  // Pattern
    pak_sa 's': ආපසු true;  // String
    pak_sa 't': ආපසු true;  // Time
    pak_sa 'u': ආපසු true;  // Packed 2-state
    pak_sa 'v': ආපසු true;  // Strength
    pak_sa 'x': ආපසු true;  // Hex
    pak_sa 'z': ආපසු true;  // Packed 4-state
    pak_sa '?': ආපසු true;  // Internal: Compute format in V3Width based on data type
    default: ආපසු false;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3Number::displayPad(size_t fmtsize, var_na pad, bool left, const string& in) VL_PURE {
    string padding;
    නම් (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    ආපසු left ? (in + padding) : (padding + in);
}

string V3Number::displayedEnumName(const AstEnumItem* itemp) {
    const string name = VIdProtect::protect(itemp->prettyName());
    ආපසු VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned var_na>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3Number::displayedEnumName(const AstEnumDType* dtypep) const VL_MT_STABLE {
    V3Number value{this, dtypep->width()};
    value.opAssign(*this);
    සඳහා (const AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        const AstConst* const constp = VN_AS(itemp->valuep(), Const);
        නම් (!constp->num().isAnyXZ() && value.isCaseEq(constp->num()))
            ආපසු displayedEnumName(itemp);
    }
    ආපසු "";
}

string V3Number::displayedEnum(const AstSFormatArg* argp,
                               const string& vformat) const VL_MT_STABLE {
    const AstEnumDType* const dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3Number value{this, dtypep->width()};
    value.opAssign(*this);
    const VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    const var_na fmt = std::tolower(vformat.back());
    නම් (fmt != 'p' && fmt != 's') ආපසු value.displayed(argp, vformat, numericAttr);
    string name = value.displayedEnumName(dtypep);
    නම් (name.empty()) name = value.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3Number nameNum{this};
    nameNum.setString(name);
    ආපසු nameNum.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3Number::displayed(const AstNode* nodep, const string& vformat,
                           const VFormatAttr& formatAttr) const VL_MT_STABLE {
    ආපසු displayed(nodep->fileline(), vformat, formatAttr);
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
    නම් (pos[0] == '-') {
        left = true;
        ++pos;
    }
    string fmtsize;
    සඳහා (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    var_na fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    නම් (formatAttr.isComplex()) {
        නම් (fmt != 'p') fmt = 's';  // override
    } නැතහොත් නම් (formatAttr.isString()) {
        නම් (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3Number strwide{fl, static_cast<පූර්ණ_සංඛ්‍යා>(toString().length()) * 8, 0};
            strwide.opNToI(*this);
            ආපසු strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        නම් (fmt != 'p') fmt = 's';  // override
    } නැතහොත් නම් (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        නම් (fmt == 'p') {
            නම් (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } නැතහොත් {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } නැතහොත් නම් (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        නම් (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3Number nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*this);
            ආපසු nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    switch (fmt) {
    pak_sa 'c': {
        නම් (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        const unsigned පූර්ණ_සංඛ්‍යා v = bitsValue(0, 8);
        str.push_back(static_cast<var_na>(v));
        ආපසු str;
    }
    pak_sa 'e':  // FALLTHRU
    pak_sa 'f':  // FALLTHRU
    pak_sa 'g': {
        const dviguu_na n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        var_na tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (void)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        ආපසු tmp;
    }
    pak_sa 's': {
        නම් (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } නැතහොත් {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            පූර්ණ_සංඛ්‍යා bit = width() - 1;
            bool start = true;
            අතරතුර ((bit % 8) != 7) ++bit;
            සඳහා (; bit >= 0; bit -= 8) {
                const පූර්ණ_සංඛ්‍යා v = bitsValue(bit - 7, 8);
                නම් (!start || v) {
                    str += static_cast<var_na>((v == 0) ? ' ' : v);
                    start = false;  // Drop leading 0s
                } නැතහොත් {
                    නම් (fmtsize != "0") str += ' ';
                }
            }
        }
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', left, str);
        ආපසු str;
    }
    pak_sa 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        නම් (formatAttr.isDouble()) {
            const dviguu_na n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            var_na tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (void)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            ආපසු tmp;
        }
        නම් (formatAttr.isString())
            ආපසු '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        නම් (formatAttr.isComplex()) ආපසු toString();
        ආපසු "%p";
    }
    pak_sa 'b':  // FALLTHRU
    pak_sa 'o':  // FALLTHRU
    pak_sa 'h':  // FALLTHRU
    pak_sa 'x': {
        පූර්ණ_සංඛ්‍යා bit = width() - 1;
        නම් (left || !fmtsize.empty()) {
            අතරතුර (bit && bitIs0(bit)) --bit;
        }
        switch (fmt) {
        pak_sa 'b': {
            සඳහා (; bit >= 0; --bit) {
                නම් (bitIs0(bit)) {
                    str += '0';
                } නැතහොත් නම් (bitIs1(bit)) {
                    str += '1';
                } නැතහොත් නම් (bitIsZ(bit)) {
                    str += 'z';
                } නැතහොත් {
                    str += 'x';
                }
            }
            bha_nga;
        }
        pak_sa 'o': {
            අතරතුර ((bit % 3) != 2) ++bit;
            සඳහා (; bit > 0; bit -= 3) {
                const පූර්ණ_සංඛ්‍යා numX = countX(bit - 2, 3);
                const පූර්ණ_සංඛ්‍යා numZ = countZ(bit - 2, 3);
                නම් (numX == 3 || numX == width() - (bit - 2)) {
                    str += 'x';
                    continue;
                }
                නම් (numZ == 3 || numZ == width() - (bit - 2)) {
                    str += 'z';
                    continue;
                }
                නම් (numX > 0) {
                    str += 'X';
                    continue;
                }
                නම් (numZ > 0) {
                    str += 'Z';
                    continue;
                }
                const පූර්ණ_සංඛ්‍යා v = bitsValue(bit - 2, 3);
                str += static_cast<var_na>('0' + v);
            }
            bha_nga;
        }
        default: {  // h/x
            අතරතුර ((bit % 4) != 3) ++bit;
            සඳහා (; bit > 0; bit -= 4) {
                const පූර්ණ_සංඛ්‍යා numX = countX(bit - 3, 4);
                const පූර්ණ_සංඛ්‍යා numZ = countZ(bit - 3, 4);
                නම් (numX == 4 || numX == width() - (bit - 3)) {
                    str += 'x';
                    continue;
                }
                නම් (numZ == 4 || numZ == width() - (bit - 3)) {
                    str += 'z';
                    continue;
                }
                නම් (numX > 0) {
                    str += 'X';
                    continue;
                }
                නම් (numZ > 0) {
                    str += 'Z';
                    continue;
                }
                const පූර්ණ_සංඛ්‍යා v = bitsValue(bit - 3, 4);
                නම් (v >= 10) {
                    str += static_cast<var_na>('a' + v - 10);
                } නැතහොත් {
                    str += static_cast<var_na>('0' + v);
                }
            }
            bha_nga;
        }
        }  // switch
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (left ? ' ' : '0'), left, str);
        ආපසු str;
    }  // case b/d/x/o
    pak_sa 't':  // Time
    pak_sa 'd': {  // Unsigned decimal
        const bool issigned = formatAttr.isSigned();
        නම් (fmtsize == "" && !left) {
            const dviguu_na mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            dviguu_na dchars = mantissabits / 3.321928094887362 + 1.0;
            නම් (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<පූර්ණ_සංඛ්‍යා>(dchars));
        }
        bool hasXZ = false;
        නම් (isAllX()) {
            str = "x";
            hasXZ = true;
        } නැතහොත් නම් (isAllZ()) {
            str = "z";
            hasXZ = true;
        } නැතහොත් නම් (isAnyX()) {
            str = "X";
            hasXZ = true;
        } නැතහොත් නම් (isAnyZ()) {
            str = "Z";
            hasXZ = true;
        }
        නම් (!hasXZ) {
            නම් (issigned) {
                නම් (width() > 64) {
                    str = toDecimalS();
                } නැතහොත් {
                    str = cvtToStr(toSQuad());
                }
            } නැතහොත් {
                නම් (width() > 64) {
                    str = toDecimalU();
                } නැතහොත් {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        const bool zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !left;
        // fmtsize might have changed since we parsed the %fmtsize
        const size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), left, str);
        ආපසු str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    pak_sa 'u': {  // Packed 2-state
        සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
            const uint32_t v = m_data.num()[i].m_value;
            str += static_cast<var_na>((v >> 0) & 0xff);
            str += static_cast<var_na>((v >> 8) & 0xff);
            str += static_cast<var_na>((v >> 16) & 0xff);
            str += static_cast<var_na>((v >> 24) & 0xff);
        }
        ආපසු str;
    }
    pak_sa 'z': {  // Packed 4-state
        සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
            const ValueAndX v = m_data.num()[i];
            str += static_cast<var_na>((v.m_value >> 0) & 0xff);
            str += static_cast<var_na>((v.m_value >> 8) & 0xff);
            str += static_cast<var_na>((v.m_value >> 16) & 0xff);
            str += static_cast<var_na>((v.m_value >> 24) & 0xff);
            str += static_cast<var_na>((v.m_valueX >> 0) & 0xff);
            str += static_cast<var_na>((v.m_valueX >> 8) & 0xff);
            str += static_cast<var_na>((v.m_valueX >> 16) & 0xff);
            str += static_cast<var_na>((v.m_valueX >> 24) & 0xff);
        }
        ආපසු str;
    }
    pak_sa 'v': {  // Strength
        පූර්ණ_සංඛ්‍යා bit = width() - 1;
        සඳහා (; bit >= 0; bit--) {
            නම් (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            නැතහොත් නම් (bitIs1(bit)) {
                str += "St1 ";
            } නැතහොත් නම් (bitIsZ(bit)) {
                str += "StZ ";
            } නැතහොත් {
                str += "StX";
            }
        }
        ආපසු str;
    }
    default: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3Number::emitC() const VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    var_na sbuf[bufsize];
    string result;
    නම් (isNull()) {
        ආපසු "VlNull{}";
    } නැතහොත් නම් (isDouble()) {
        const dviguu_na dnum = toDouble();
        නම් (std::isinf(dnum)) {
            නම් (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } නැතහොත් නම් (std::isnan(dnum)) {
            නම් (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } නැතහොත් {
            const var_na* const fmt = (static_cast<පූර්ණ_සංඛ්‍යා>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (void)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ආපසු sbuf;
        }
    } නැතහොත් නම් (isString()) {
        const string strg = toString();
        const string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ආපසු '"' + quoted + "\"s";
    } නැතහොත් නම් (words() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(words()) + ">{{";
        නම් (words() > 4) result += '\n';
        සඳහා (පූර්ණ_සංඛ්‍යා n = 0; n < words(); ++n) {
            නම් (n) result += ((n % 4) ? ", " : ",\n");
            (void)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        නම් (words() > 4) result += '\n';
        result += "}}";
    } නැතහොත් නම් (words() == 2) {  // Quad
        const uint64_t qnum = static_cast<uint64_t>(toUQuad());
        const var_na* const fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (void)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ආපසු sbuf;
    } නැතහොත් {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        const uint32_t unum = toUInt();
        const var_na* const fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (void)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ආපසු sbuf;
    }
    ආපසු result;
}

string V3Number::toDecimalS() const VL_MT_STABLE {
    // Correct number of zero bits/width matters
    නම් (isNegative()) {
        V3Number lhsNoSign = *this;
        lhsNoSign.opNegate(*this);
        ආපසු "-"s + lhsNoSign.toDecimalU();
    } නැතහොත් {
        ආපසු toDecimalU();
    }
}

string V3Number::toDecimalU() const VL_MT_STABLE {
    const පූර්ණ_සංඛ්‍යා maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3Number bcd(this, maxdecwidth + 4);
    V3Number tmp(this, maxdecwidth + 4);
    V3Number tmp2(this, maxdecwidth + 4);

    පූර්ණ_සංඛ්‍යා from_bit = width() - 1;
    // Skip all leading zeros
    සඳහා (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    සඳහා (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        සඳහා (පූර්ණ_සංඛ්‍යා nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            නම් (bcd.bitsValue(nibble_bit, 4) >= 5) {
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
        නම් (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    පූර්ණ_සංඛ්‍යා lsb = (maxdecwidth - 1) & ~3;
    සඳහා (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        නම් (bcd.bitsValue(lsb, 4)) bha_nga;
    }
    සඳහා (; lsb >= 0; lsb -= 4) {
        output += static_cast<var_na>('0' + bcd.bitsValue(lsb, 4));  // 0..9
    }
    ආපසු output;
}

string V3Number::toRealString() const VL_MT_STABLE {
    const dviguu_na value = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << value;
    string result = out.str();
    නම් (std::strtod(result.c_str(), nullptr) != value) {
        out.str("");
        out.precision(17);
        out << value;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    නම් (result.find_first_of(".en") == string::npos) result += ".0";
    ආපසු result;
}

//======================================================================
// ACCESSORS - as numbers

bool V3Number::fitsInUInt() const VL_MT_SAFE {
    නම් (isFourState()) ආපසු false;
    // We allow wide numbers that represent values <= 32 bits
    සඳහා (පූර්ණ_සංඛ්‍යා i = 1; i < words(); ++i) {
        නම් (m_data.num()[i].m_value) ආපසු false;
    }
    ආපසු true;
}
uint32_t V3Number::toUInt() const VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *this);
    // We allow wide numbers that represent values <= 32 bits
    නම් (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *this);
    }
    ආපසු m_data.num()[0].m_value;
}

dviguu_na V3Number::toDouble() const VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    sa_ngha {
        dviguu_na d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.num()[0].m_value;
    u.u[1] = m_data.num()[1].m_value;
    ආපසු u.d;
}

int32_t V3Number::toSInt() const VL_MT_SAFE {
    නම් (isSigned()) {
        const uint32_t v = toUInt();
        const uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        const uint32_t extended = v | signExtend;
        ආපසු static_cast<int32_t>(extended);
    } නැතහොත් {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ආපසු static_cast<int32_t>(toUInt());
    }
}

uint64_t V3Number::toUQuad() const VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *this);
    // We allow wide numbers that represent values <= 64 bits
    නම් (isDouble()) ආපසු static_cast<uint64_t>(toDouble());
    සඳහා (පූර්ණ_සංඛ්‍යා i = 2; i < words(); ++i) {
        නම් (m_data.num()[i].m_value) {
            v3error("Value too wide for 64-bits expected in this context " << *this);
            bha_nga;
        }
    }
    නම් (width() <= 32) ආපසු (static_cast<uint64_t>(toUInt()));
    ආපසු ((static_cast<uint64_t>(m_data.num()[1].m_value) << 32ULL)
            | (static_cast<uint64_t>(m_data.num()[0].m_value)));
}

int64_t V3Number::toSQuad() const {
    නම් (isDouble()) ආපසු static_cast<int64_t>(toDouble());
    const uint64_t v = toUQuad();
    const uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    const uint64_t extended = v | signExtend;
    ආපසු static_cast<int64_t>(extended);
}

string V3Number::toString() const {
    UASSERT(!isFourState(), "toString with 4-state " << *this);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    නම් (isString()) ආපසු m_data.str();
    පූර්ණ_සංඛ්‍යා bit = width() - 1;
    අතරතුර ((bit % 8) != 7) ++bit;
    std::string str;
    සඳහා (; bit >= 0; bit -= 8) {
        const පූර්ණ_සංඛ්‍යා v = bitsValue(bit - 7, 8);
        නම් (v) str += static_cast<var_na>(v);
    }
    ආපසු str;
}

uint32_t V3Number::edataWord(පූර්ණ_සංඛ්‍යා eword) const {
    UASSERT(!isFourState(), "edataWord with 4-state " << *this);
    ආපසු m_data.num()[eword].m_value;
}

uint8_t V3Number::dataByte(පූර්ණ_සංඛ්‍යා byte) const {
    ආපසු (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

bool V3Number::isAllZ() const VL_MT_SAFE {
    නම් (isDouble() || isString()) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        const uint32_t mask = i == words() - 1 ? hiWordMask() : ~0U;
        නම් (((~v.m_value & v.m_valueX) & mask) != mask) ආපසු false;
    }
    ආපසු true;
}
bool V3Number::isAllX() const VL_MT_SAFE {
    නම් (isDouble() || isString()) ආපසු false;
    uint32_t mask = hiWordMask();
    සඳහා (පූර්ණ_සංඛ්‍යා i = words() - 1; i >= 0; --i) {
        const ValueAndX v = m_data.num()[i];
        නම් ((v.m_value & v.m_valueX) ^ mask) ආපසු false;
        mask = ~0U;
    }
    ආපසු true;
}
bool V3Number::isEqZero() const VL_MT_SAFE {
    නම් (isString()) ආපසු m_data.str().empty();
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        නම් (v.m_value || v.m_valueX) ආපසු false;
    }
    ආපසු true;
}
bool V3Number::isNeqZero() const {
    නම් (isString()) ආපසු !m_data.str().empty();
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        නම් (v.m_value & ~v.m_valueX) ආපසු true;
    }
    ආපසු false;
}
bool V3Number::isBitsZero(පූර්ණ_සංඛ්‍යා msb, පූර්ණ_සංඛ්‍යා lsb) const {
    සඳහා (පූර්ණ_සංඛ්‍යා i = lsb; i <= msb; ++i) {
        නම් (VL_UNLIKELY(!bitIs0(i))) ආපසු false;
    }
    ආපසු true;
}
bool V3Number::isEqOne() const {
    නම් (m_data.num()[0].m_value != 1 || m_data.num()[0].m_valueX) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 1; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        නම් (v.m_value || v.m_valueX) ආපසු false;
    }
    ආපසු true;
}
bool V3Number::isEqAllOnes(පූර්ණ_සංඛ්‍යා optwidth) const {
    // Correct number of zero bits/width matters
    නම් (!optwidth) optwidth = width();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < optwidth; ++bit) {
        නම් (!bitIs1(bit)) ආපසු false;
    }
    ආපසු true;
}
bool V3Number::isFourState() const VL_MT_SAFE {
    නම් (isDouble() || isString()) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        නම් (m_data.num()[i].m_valueX) ආපසු true;
    }
    ආපසු false;
}
bool V3Number::isAnyX() const VL_MT_SAFE {
    නම් (isDouble() || isString()) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        නම් (v.m_value & v.m_valueX) ආපසු true;
    }
    ආපසු false;
}
bool V3Number::isAnyXZ() const {
    නම් (isDouble() || isString()) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        නම් (m_data.num()[i].m_valueX) ආපසු true;
    }
    ආපසු false;
}
bool V3Number::isAnyZ() const VL_MT_SAFE {
    නම් (isDouble() || isString()) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        const ValueAndX v = m_data.num()[i];
        නම් (~v.m_value & v.m_valueX) ආපසු true;
    }
    ආපසු false;
}
bool V3Number::isLtXZ(const V3Number& rhs) const {
    // Include X/Z in comparisons for sort ordering
    නම් (isString()) ආපසු toString() < rhs.toString();
    නම් (isDouble()) ආපසු toDouble() < rhs.toDouble();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        නම් (bitIs1(bit) && rhs.bitIs0(bit)) ආපසු true;
        නම් (rhs.bitIs1(bit) && bitIs0(bit)) ආපසු false;
        නම් (bitIsXZ(bit)) ආපසු true;
        නම් (rhs.bitIsXZ(bit)) ආපසු false;
    }
    ආපසු false;
}
පූර්ණ_සංඛ්‍යා V3Number::countX(පූර්ණ_සංඛ්‍යා lsb, පූර්ණ_සංඛ්‍යා nbits) const VL_MT_SAFE {
    පූර්ණ_සංඛ්‍යා count = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bitn = 0; bitn < nbits; ++bitn) {
        නම් (lsb + bitn >= width()) ආපසු count;
        නම් (bitIsX(lsb + bitn)) ++count;
    }
    ආපසු count;
}
පූර්ණ_සංඛ්‍යා V3Number::countZ(පූර්ණ_සංඛ්‍යා lsb, පූර්ණ_සංඛ්‍යා nbits) const VL_MT_SAFE {
    පූර්ණ_සංඛ්‍යා count = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bitn = 0; bitn < nbits; ++bitn) {
        නම් (lsb + bitn >= width()) ආපසු count;
        නම් (bitIsZ(lsb + bitn)) ++count;
    }
    ආපසු count;
}

පූර්ණ_සංඛ්‍යා V3Number::widthToFit() const {
    සඳහා (පූර්ණ_සංඛ්‍යා bit = width() - 1; bit > 0; bit--) {
        නම් (!bitIs0(bit)) ආපසු bit + 1;
    }
    ආපසු 1;  // one bit even if number is == 0
}

uint32_t V3Number::countBits(const V3Number& ctrl) const {
    // Correct number of zero bits/width matters
    පූර්ණ_සංඛ්‍යා n = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        switch (ctrl.bitIs(0)) {
        pak_sa '0':
            නම් (bitIs0(bit)) ++n;
            bha_nga;
        pak_sa '1':
            නම් (bitIs1(bit)) ++n;
            bha_nga;
        pak_sa 'x':
            නම් (bitIsX(bit)) ++n;
            bha_nga;
        pak_sa 'z':
            නම් (bitIsZ(bit)) ++n;
            bha_nga;
        }
    }
    ආපසු n;
}

uint32_t V3Number::countBits(const V3Number& ctrl1, const V3Number& ctrl2,
                             const V3Number& ctrl3) const {
    // Correct number of zero bits/width matters
    පූර්ණ_සංඛ්‍යා n = countBits(ctrl1);
    නම් (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += countBits(ctrl2);
    නම් ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += countBits(ctrl3);
    }
    ආපසු n;
}

uint32_t V3Number::countOnes() const {
    පූර්ණ_සංඛ්‍යා n = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (bitIs1(bit)) ++n;
    }
    ආපසු n;
}

uint32_t V3Number::mostSetBitP1() const {
    සඳහා (පූර්ණ_සංඛ්‍යා bit = width() - 1; bit >= 0; bit--) {
        නම් (!bitIs0(bit)) ආපසු bit + 1;
    }
    ආපසු 0;
}

uint32_t V3Number::leastSetBitP1() const {
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (!bitIs0(bit)) ආපසු bit + 1;
    }
    ආපසු 0;
}

//======================================================================

V3Number& V3Number::opBitsNonXZ(const V3Number& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ආපසු *this;
}
V3Number& V3Number::opBitsOne(const V3Number& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ආපසු *this;
}
V3Number& V3Number::opBitsXZ(const V3Number& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    ආපසු *this;
}
V3Number& V3Number::opBitsZ(const V3Number& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    ආපසු *this;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3Number& V3Number::opRedOr(const V3Number& lhs) {
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    var_na outc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < lhs.width(); ++bit) {
        නම් (lhs.bitIs1(bit)) {
            ආපසු setSingleBits(1);
        } නැතහොත් නම් (lhs.bitIs0(bit)) {
        } නැතහොත් {
            outc = 'x';
        }
    }
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opRedAnd(const V3Number& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    var_na outc = 1;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < lhs.width(); ++bit) {
        නම් (lhs.bitIs0(bit)) {
            ආපසු setSingleBits(0);
        } නැතහොත් නම් (lhs.bitIs1(bit)) {
        } නැතහොත් {
            outc = 'x';
        }
    }
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opRedXor(const V3Number& lhs) {
    // op i, 1 bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    var_na outc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < lhs.width(); ++bit) {
        නම් (lhs.bitIs1(bit)) {
            නම් (outc == 1) {
                outc = 0;
            } නැතහොත් නම් (outc == 0) {
                outc = 1;
            }
        } නැතහොත් නම් (lhs.bitIs0(bit)) {
        } නැතහොත් {
            outc = 'x';
        }
    }
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opCountBits(const V3Number& expr, const V3Number& ctrl1, const V3Number& ctrl2,
                                const V3Number& ctrl3) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS4(expr, ctrl1, ctrl2, ctrl3);
    NUM_ASSERT_LOGIC_ARGS4(expr, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.num()[0].m_value = expr.countBits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::opCountOnes(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    නම් (lhs.isFourState()) ආපසු setAllBitsX();
    setZero();
    m_data.num()[0].m_value = lhs.countOnes();
    opCleanThis();
    ආපසු *this;
}
V3Number& V3Number::opIsUnknown(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    ආපසු setSingleBits(lhs.isAnyXZ());
}
V3Number& V3Number::opOneHot(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    නම් (lhs.isFourState()) ආපසු setAllBitsX();
    ආපසු setSingleBits(lhs.countOnes() == 1);
}
V3Number& V3Number::opOneHot0(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    නම් (lhs.isFourState()) ආපසු setAllBitsX();
    ආපසු setSingleBits(lhs.countOnes() <= 1);
}
V3Number& V3Number::opCLog2(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    නම් (lhs.isFourState()) ආපසු setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    const පූර්ණ_සංඛ්‍යා adjust = (lhs.countOnes() == 1) ? 0 : 1;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = lhs.width() - 1; bit >= 0; bit--) {
        නම් (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            ආපසු *this;
        }
    }
    setZero();
    ආපසු *this;
}
V3Number& V3Number::opMostSetBitP1(const V3Number& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    නම් (lhs.isFourState()) ආපසු setAllBitsX();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = lhs.width() - 1; bit >= 0; bit--) {
        නම් (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            ආපසු *this;
        }
    }
    setZero();
    ආපසු *this;
}

V3Number& V3Number::opLogNot(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    var_na outc = 1;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < lhs.width(); ++bit) {
        නම් (lhs.bitIs1(bit)) {
            outc = 0;
            goto last;
        } නැතහොත් නම් (lhs.bitIs0(bit)) {
        } නැතහොත් {
            outc = 'x';
        }
    }
last:
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opNot(const V3Number& lhs) {
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } නැතහොත් නම් (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    ආපසු *this;
}

V3Number& V3Number::opAnd(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } නැතහොත් නම් (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } නැතහොත් {
            setBit(bit, 'x');
        }
    }
    ආපසු *this;
}

V3Number& V3Number::opOr(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } නැතහොත් නම් (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } නැතහොත් {
            setBit(bit, 'x');
        }
    }
    ආපසු *this;
}

V3Number& V3Number::opXor(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } නැතහොත් නම් (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } නැතහොත් නම් (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    ආපසු *this;
}

void V3Number::fillBits(පූර්ණ_සංඛ්‍යා destLsb, පූර්ණ_සංඛ්‍යා width, var_na value) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    නම් (!width) ආපසු;
    UASSERT(width <= this->width() - destLsb, "Destination bit range exceeds number width");

    const uint32_t valueWord = value == '1' || value == 'x' || value == 1 || value == 3 ? ~0U : 0;
    const uint32_t xWord = value == 'z' || value == 'x' || value == 2 || value == 3 ? ~0U : 0;
    const පූර්ණ_සංඛ්‍යා firstWord = VL_BITWORD_E(destLsb);
    const පූර්ණ_සංඛ්‍යා lastWord = VL_BITWORD_E(destLsb + width - 1);
    සඳහා (පූර්ණ_සංඛ්‍යා word = firstWord; word <= lastWord; ++word) {
        const පූර්ණ_සංඛ්‍යා wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        const පූර්ණ_සංඛ්‍යා wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        const uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        ValueAndX& dest = m_data.num()[word];
        dest.m_value = (dest.m_value & ~mask) | (valueWord & mask);
        dest.m_valueX = (dest.m_valueX & ~mask) | (xWord & mask);
    }
}

void V3Number::copyBits(පූර්ණ_සංඛ්‍යා destLsb, const V3Number& source, පූර්ණ_සංඛ්‍යා sourceLsb, පූර්ණ_සංඛ්‍යා width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    නම් (!width) ආපසු;
    UASSERT(width <= this->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    const පූර්ණ_සංඛ්‍යා firstWord = VL_BITWORD_E(destLsb);
    const පූර්ණ_සංඛ්‍යා lastWord = VL_BITWORD_E(destLsb + width - 1);
    සඳහා (පූර්ණ_සංඛ්‍යා destWord = firstWord; destWord <= lastWord; ++destWord) {
        const පූර්ණ_සංඛ්‍යා destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        const පූර්ණ_සංඛ්‍යා destOffset = VL_BITBIT_E(destBit);
        const පූර්ණ_සංඛ්‍යා sourceBit = sourceLsb + destBit - destLsb;
        const පූර්ණ_සංඛ්‍යා sourceWord = VL_BITWORD_E(sourceBit);
        const පූර්ණ_සංඛ්‍යා sourceOffset = VL_BITBIT_E(sourceBit);
        const පූර්ණ_සංඛ්‍යා copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        const uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        const ValueAndX& sourceLo = source.m_data.num()[sourceWord];
        uint32_t value = sourceLo.m_value >> sourceOffset;
        uint32_t valueX = sourceLo.m_valueX >> sourceOffset;
        නම් (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
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
    නම් (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    ආපසු *this;
}

V3Number& V3Number::opLenN(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    ආපසු *this;
}

V3Number& V3Number::opRepl(const V3Number& lhs,
                           const V3Number& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ආපසු opRepl(lhs, rhs.toUInt());
}

V3Number& V3Number::opRepl(const V3Number& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    නම් (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    සඳහා (unsigned times = 0; times < rhsval; ++times) {
        const uint64_t destLsb = uint64_t{times} * lhs.width();
        නම් (destLsb >= static_cast<uint32_t>(width())) bha_nga;
        copyBits(static_cast<පූර්ණ_සංඛ්‍යා>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<පූර්ණ_සංඛ්‍යා>(destLsb)));
    }
    ආපසු *this;
}

V3Number& V3Number::opStreamL(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    නම් (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    const පූර්ණ_සංඛ්‍යා ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    සඳහා (පූර්ණ_සංඛ්‍යා istart = 0; istart < lhs.width(); istart += ssize) {
        const පූර්ණ_සංඛ්‍යා ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    ආපසු *this;
}

V3Number& V3Number::opLogAnd(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    var_na loutc = 0;
    var_na routc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < lhs.width(); ++bit) {
        නම් (lhs.bitIs1(bit)) {
            loutc = 1;
            bha_nga;
        }
        නම් (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < rhs.width(); ++bit) {
        නම් (rhs.bitIs1(bit)) {
            routc = 1;
            bha_nga;
        }
        නම් (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    var_na outc = 'x';
    නම් (routc == 1 && loutc == 1) outc = 1;
    නම් (routc == 0 || loutc == 0) outc = 0;
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opLogOr(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    var_na outc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < lhs.width(); ++bit) {
        නම් (lhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        නම් (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < rhs.width(); ++bit) {
        නම් (rhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        නම් (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opLogIf(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number lnot = lhs;
    lnot.opLogNot(lhs);
    ආපසු opLogOr(lnot, rhs);
}

V3Number& V3Number::opLogEq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3Number ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ආපසු opLogAnd(ifa, ifb);
}

V3Number& V3Number::opAtoN(const V3Number& lhs, පූර්ණ_සංඛ්‍යා base) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    නම් (base == AstAtoN::ATOREAL) ආපසු setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    නම් (errno != 0) v = 0;
    ආපසු setLongS(static_cast<int32_t>(v));
}

V3Number& V3Number::opPutcN(const V3Number& lhs, const V3Number& rhs, const V3Number& ths) {
    NUM_ASSERT_OP_ARGS3(lhs, rhs, ths);
    NUM_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    const int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    නම් (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    ආපසු setString(lstring);
}

V3Number& V3Number::opGetcN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const string lstring = lhs.toString();
    const int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    නම් (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    ආපසු setLong(v);
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
    නම් (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) ආපසු setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ආපසු setString(lstring.substr(i, j - i + 1));
}

V3Number& V3Number::opCompareNN(const V3Number& lhs, const V3Number& rhs, bool ignoreCase) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    පූර්ණ_සංඛ්‍යා result;
    const string lstring = lhs.toString();
    const string rstring = rhs.toString();
    නම් (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } නැතහොත් {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ආපසු setLongS(result);
}

V3Number& V3Number::opEq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    නම් (lhs.isString()) ආපසු opEqN(lhs, rhs);
    නම් (lhs.isDouble()) ආපසු opEqD(lhs, rhs);
    var_na outc = 1;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        නම් (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            goto last;
        }
        නම් (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            goto last;
        }
        නම් (lhs.bitIsXZ(bit)) outc = 'x';
        නම් (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opNeq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    නම් (lhs.isString()) ආපසු opNeqN(lhs, rhs);
    නම් (lhs.isDouble()) ආපසු opNeqD(lhs, rhs);
    var_na outc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        නම් (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            goto last;
        }
        නම් (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            goto last;
        }
        නම් (lhs.bitIsXZ(bit)) outc = 'x';
        නම් (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ආපසු setSingleBits(outc);
}

V3Hash V3Number::toHash() const {
    // Shouldmatch isCaseEq
    නම් (isString()) ආපසු V3Hash{toString()};
    නම් (isDouble()) ආපසු V3Hash{toDouble()};
    V3Hash hash{width()};
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) hash += m_data.num()[i].m_value;
    ආපසු hash;
}

bool V3Number::isCaseEq(const V3Number& rhs) const {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    නම් (isString()) ආපසු toString() == rhs.toString();
    නම් (isDouble()) ආපසු toDouble() == rhs.toDouble();
    නම් (width() != rhs.width()) ආපසු false;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < words(); ++i) {
        නම් (!(m_data.num()[i] == rhs.m_data.num()[i])) ආපසු false;
    }
    ආපසු true;
}

V3Number& V3Number::opCaseEq(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3Number& V3Number::opCaseNeq(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    var_na outc = 0;
    නම් (lhs.isString()) {
        ආපසු opNeqN(lhs, rhs);
    } නැතහොත් නම් (lhs.isDouble()) {
        ආපසු opNeqD(lhs, rhs);
    }
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        නම් (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            goto last;
        }
    }
last:
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opWildEq(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    var_na outc = 1;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        නම් (!rhs.bitIsXZ(bit)) {
            නම් (lhs.bitIsXZ(bit)) {
                outc = 'x';
                goto last;
            }
            නම් (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                goto last;
            }
        }
    }
last:
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opWildNeq(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    var_na outc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        නම් (!rhs.bitIsXZ(bit)) {
            නම් (lhs.bitIsXZ(bit)) {
                outc = 'x';
                goto last;
            }
            නම් (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                goto last;
            }
        }
    }
last:
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opGt(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    var_na outc = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        නම් (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        නම් (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        නම් (lhs.bitIsXZ(bit)) outc = 'x';
        නම් (rhs.bitIsXZ(bit)) outc = 'x';
    }
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opGtS(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    var_na outc = 0;
    {
        const පූර්ණ_සංඛ්‍යා mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        නම් (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } නැතහොත් නම් (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } නැතහොත් නම් (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } නැතහොත් නම් (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } නැතහොත් {
            // both positive or negative, normal >
            සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                නම් (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                නම් (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                නම් (lhs.bitIsXZ(bit)) outc = 'x';
                නම් (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    ආපසු setSingleBits(outc);
}

V3Number& V3Number::opGte(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number& eq = opEq(lhs, rhs);
    නම් (eq.isNeqZero()) ආපසු eq;  // Return true
    ආපසු opGt(lhs, rhs);
}

V3Number& V3Number::opGteS(const V3Number& lhs, const V3Number& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    V3Number& eq = opEq(lhs, rhs);
    නම් (eq.isNeqZero()) ආපසු eq;  // Return true
    ආපසු opGtS(lhs, rhs);
}

V3Number& V3Number::opLt(const V3Number& lhs, const V3Number& rhs) { ආපසු opGt(rhs, lhs); }
V3Number& V3Number::opLte(const V3Number& lhs, const V3Number& rhs) { ආපසු opGte(rhs, lhs); }
V3Number& V3Number::opLtS(const V3Number& lhs, const V3Number& rhs) { ආපසු opGtS(rhs, lhs); }
V3Number& V3Number::opLteS(const V3Number& lhs, const V3Number& rhs) { ආපසු opGteS(rhs, lhs); }

V3Number& V3Number::opShiftR(const V3Number& lhs, const V3Number& rhs) {
    // L(lhs) bit return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (rhs.isFourState()) ආපසු setAllBitsX();
    setZero();
    නම් (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ආපසු *this;
    const uint32_t rhsval = rhs.toUInt();
    නම් (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<පූර්ණ_සංඛ්‍යා>(rhsval),
                 std::min(width(), lhs.width() - static_cast<පූර්ණ_සංඛ්‍යා>(rhsval)));
    }
    ආපසු *this;
}

V3Number& V3Number::opShiftRS(const V3Number& lhs, const V3Number& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (rhs.isFourState()) ආපසු setAllBitsX();
    setZero();
    const bool overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    නම් (!overflow) {
        const uint32_t rhsval = rhs.toUInt();
        නම් (rhsval < lbits) {
            const පූර්ණ_සංඛ්‍යා copyWidth = std::min(width(), static_cast<පූර්ණ_සංඛ්‍යා>(lbits - rhsval));
            copyBits(0, lhs, static_cast<පූර්ණ_සංඛ්‍යා>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            ආපසු *this;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    ආපසු *this;
}

V3Number& V3Number::opShiftL(const V3Number& lhs, const V3Number& rhs) {
    // L(lhs) bit return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (rhs.isFourState()) ආපසු setAllBitsX();
    setZero();
    නම් (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ආපසු *this;
    const uint32_t rhsval = rhs.toUInt();
    නම් (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<පූර්ණ_සංඛ්‍යා>(rhsval), lhs, 0,
                 std::min(width() - static_cast<පූර්ණ_සංඛ්‍යා>(rhsval), lhs.width()));
    }
    ආපසු *this;
}

//======================================================================
// Ops - Arithmetic

V3Number& V3Number::opNegate(const V3Number& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    නම් (lhs.isFourState()) ආපසු setAllBitsX();
    V3Number notlhs(&lhs, width());
    notlhs.opNot(lhs);
    const V3Number one(&lhs, width(), 1);
    opAdd(notlhs, one);
    ආපසු *this;
}
V3Number& V3Number::opAdd(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා word = 0; word < words(); ++word) {
        const uint64_t lwordval = lhs.m_data.num()[word].m_value;
        const uint64_t rwordval = rhs.m_data.num()[word].m_value;
        const uint64_t sum = lwordval + rwordval + carry;
        m_data.num()[word].m_value = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ආපසු *this;
}
V3Number& V3Number::opSub(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    V3Number negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    ආපසු opAdd(lhs, negrhs);
}
V3Number& V3Number::opMul(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    setZero();
    නම් (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } නැතහොත් {
        සඳහා (පූර්ණ_සංඛ්‍යා lword = 0; lword < lhs.words(); ++lword) {
            const uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.num()[lword].m_value);
            නම් (lwordval == 0) continue;
            සඳහා (පූර්ණ_සංඛ්‍යා rword = 0; rword < rhs.words(); ++rword) {
                const uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.num()[rword].m_value);
                නම් (rwordval == 0) continue;
                uint64_t mul = lwordval * rwordval;
                සඳහා (පූර්ණ_සංඛ්‍යා qword = lword + rword; qword < words(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.num()[qword].m_value);
                    m_data.num()[qword].m_value = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    නම් (mul == 0) bha_nga;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ආපසු *this;
}
V3Number& V3Number::opMulS(const V3Number& lhs, const V3Number& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    V3Number lhsNoSign = lhs;
    නම් (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    නම් (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3Number qNoSign = opMul(lhsNoSign, rhsNoSign);
    නම් ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } නැතහොත් {
        opAssign(qNoSign);
    }
    ආපසු *this;
}
V3Number& V3Number::opDiv(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    නම් (rhs.isEqZero()) ආපසු setAllBitsXRemoved();
    නම් (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        ආපසු *this;
    } නැතහොත් {
        // Wide division
        ආපසු opModDivGuts(lhs, rhs, false);
    }
}
V3Number& V3Number::opDivS(const V3Number& lhs, const V3Number& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    නම් (rhs.isEqZero()) ආපසු setAllBitsXRemoved();
    V3Number lhsNoSign = lhs;
    නම් (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    නම් (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3Number qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    නම් ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } නැතහොත් {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *this);
    ආපසු *this;
}
V3Number& V3Number::opModDiv(const V3Number& lhs, const V3Number& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    නම් (rhs.isEqZero()) ආපසු setAllBitsXRemoved();
    නම් (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        ආපසු *this;
    } නැතහොත් {
        // Wide modulus
        ආපසු opModDivGuts(lhs, rhs, true);
    }
}
V3Number& V3Number::opModDivS(const V3Number& lhs, const V3Number& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    නම් (rhs.isEqZero()) ආපසු setAllBitsXRemoved();
    V3Number lhsNoSign = lhs;
    නම් (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3Number rhsNoSign = rhs;
    නම් (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    const V3Number qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    නම් (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } නැතහොත් {
        opAssign(qNoSign);
    }
    ආපසු *this;
}
V3Number& V3Number::opModDivGuts(const V3Number& lhs, const V3Number& rhs, bool is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    const පූර්ණ_සංඛ්‍යා lWords = lhs.words();
    const පූර්ණ_සංඛ්‍යා umsbp1 = lhs.mostSetBitP1();  // dividend
    const පූර්ණ_සංඛ්‍යා vmsbp1 = rhs.mostSetBitP1();  // divisor
    නම් (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *this);
        ආපසු *this;
    }

    const පූර්ණ_සංඛ්‍යා uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    const පූර්ණ_සංඛ්‍යා vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    නම් (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        සඳහා (පූර්ණ_සංඛ්‍යා j = uw - 1; j >= 0; j--) {
            const uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.num()[j].m_value));
            m_data.num()[j].m_value = unw64 / static_cast<uint64_t>(rhs.m_data.num()[0].m_value);
            k = unw64
                - (static_cast<uint64_t>(m_data.num()[j].m_value)
                   * static_cast<uint64_t>(rhs.m_data.num()[0].m_value));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *this << " rem=0x" << std::hex
                                   << k << std::dec);
        නම් (is_modulus) {
            setZero();
            m_data.num()[0].m_value = k;
        }
        opCleanThis();
        ආපසු *this;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_WORDS + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_WORDS + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < lWords; ++i) m_data.num()[i].m_value = 0;
    සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < lWords + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    const පූර්ණ_සංඛ්‍යා s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    const uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    සඳහා (පූර්ණ_සංඛ්‍යා i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.num()[i].m_value << s)
                | (shift_mask & (rhs.m_data.num()[i - 1].m_value >> (32 - s)));
    }
    vn[0] = rhs.m_data.num()[0].m_value << s;

    // Copy and shift dividend by same amount; may set new upper word
    නම් (s) {
        un[uw] = lhs.m_data.num()[uw - 1].m_value >> (32 - s);
    } නැතහොත් {
        un[uw] = 0;
    }
    සඳහා (පූර්ණ_සංඛ්‍යා i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.num()[i].m_value << s)
                | (shift_mask & (lhs.m_data.num()[i - 1].m_value >> (32 - s)));
    }
    un[0] = lhs.m_data.num()[0].m_value << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    සඳහා (පූර්ණ_සංඛ්‍යා j = uw - vw; j >= 0; j--) {
        // Estimate
        const uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        නම් (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            නම් (rhat < 0x100000000ULL) goto again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < vw; ++i) {
            const uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.num()[j].m_value = qhat;  // Save quotient digit

        නම් (t < 0) {
            // Over subtracted; correct by adding back
            m_data.num()[j].m_value--;
            k = 0;
            සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < vw; ++i) {
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

    නම් (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        සඳහා (පූර්ණ_සංඛ්‍යා i = 0; i < vw; ++i) {
            m_data.num()[i].m_value = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        සඳහා (පූර්ණ_සංඛ්‍යා i = vw; i < lWords; ++i) m_data.num()[i].m_value = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *this);
        ආපසු *this;
    } නැතහොත් {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *this);
        ආපසු *this;
    }
}

V3Number& V3Number::opPow(const V3Number& lhs, const V3Number& rhs, bool lsign, bool rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_LOGIC_ARGS2(lhs, rhs);
    නම් (lhs.isFourState() || rhs.isFourState()) ආපසු setAllBitsX();
    නම් (rhs.isEqZero()) ආපසු setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    නම් (rsign && rhs.isNegative()) {
        නම් (lhs.isEqZero()) {
            ආපසු setAllBitsXRemoved();
        } නැතහොත් නම් (lhs.isEqOne()) {
            ආපසු setValue1();
        } නැතහොත් නම් (lsign && lhs.isEqAllOnes()) {
            නම් (rhs.bitIs1(0)) {
                ආපසු setAllBits1();  // -1^odd=-1
            } නැතහොත් {
                ආපසු setValue1();  // -1^even=1
            }
        }
        ආපසු setZero();
    }
    නම් (lhs.isEqZero()) ආපසු setZero();
    setZero();
    m_data.num()[0].m_value = 1;
    V3Number power(&lhs, width());
    power.opAssign(lhs);
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < rhs.width(); ++bit) {
        නම් (bit > 0) {  // power = power*power
            V3Number lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        නම් (rhs.bitIs1(bit)) {  // out *= power
            V3Number lastOut(&lhs, width());
            lastOut.opAssign(*this);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ආපසු *this;
}
V3Number& V3Number::opPowSU(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    ආපසු opPow(lhs, rhs, true, false);
}
V3Number& V3Number::opPowSS(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    ආපසු opPow(lhs, rhs, true, true);
}
V3Number& V3Number::opPowUS(const V3Number& lhs, const V3Number& rhs) {
    // Correct number of zero bits/width matters
    ආපසු opPow(lhs, rhs, false, true);
}

V3Number& V3Number::opBufIf1(const V3Number& ens, const V3Number& if1s) {
    NUM_ASSERT_OP_ARGS2(ens, if1s);
    NUM_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit < width(); ++bit) {
        නම් (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } නැතහොත් {
            setBit(bit, 'z');
        }
    }
    ආපසු *this;
}

// Sets all bits in range to the given value
V3Number& V3Number::opSetRange(uint32_t lsb, uint32_t width, var_na bitValue) {
    නම් (lsb < static_cast<uint32_t>(this->width())) {
        fillBits(static_cast<පූර්ණ_සංඛ්‍යා>(lsb),
                 static_cast<පූර්ණ_සංඛ්‍යා>(std::min(width, static_cast<uint32_t>(this->width()) - lsb)),
                 bitValue);
    }
    ආපසු *this;
}

V3Number& V3Number::opAssign(const V3Number& lhs) { ආපසු opAssignNonXZ(lhs, false); }
V3Number& V3Number::opAssignNonXZ(const V3Number& lhs, bool ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    නම් (this != &lhs) {
        නම් (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = true;
        } නැතහොත් නම් (isString()) {
            නම් (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } නැතහොත් {
                m_data.str() = lhs.m_data.str();
            }
        } නැතහොත් නම් (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } නැතහොත් නම් (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } නැතහොත් නම් (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } නැතහොත් {
            සඳහා (පූර්ණ_සංඛ්‍යා word = 0; word < words(); ++word) {
                const uint32_t value = word < lhs.words() ? lhs.m_data.num()[word].m_value
                                                                & ~lhs.m_data.num()[word].m_valueX
                                                          : 0;
                m_data.num()[word] = {value, 0};
            }
            opCleanThis();
        }
    }
    ආපසු *this;
}

V3Number& V3Number::opNToI(const V3Number& lhs) {
    // String to packed number
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    setZero();
    const string& str = lhs.toString();
    සඳහා (size_t n = 0; n < str.length(); ++n) {
        const var_na c = str[str.length() - 1 - n];
        සඳහා (size_t cbit = 0; cbit < 8; ++cbit) {
            const std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ආපසු *this;
}

V3Number& V3Number::opExtendS(const V3Number& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    const පූර්ණ_සංඛ්‍යා copyWidth = std::min(width(), static_cast<පූර්ණ_සංඛ්‍යා>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ආපසු *this;
}

V3Number& V3Number::opExtendXZ(const V3Number& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    const පූර්ණ_සංඛ්‍යා copyWidth = std::min(width(), static_cast<පූර්ණ_සංඛ්‍යා>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ආපසු *this;
}

V3Number& V3Number::opClean(const V3Number& lhs, uint32_t bits) { ආපසු opSel(lhs, bits - 1, 0); }

void V3Number::opCleanThis(bool warnOnTruncation) {
    // Clean MSB of number
    NUM_ASSERT_LOGIC_ARGS1(*this);
    const ValueAndX v = m_data.num()[words() - 1];
    const uint32_t newValueMsb = v.m_value & hiWordMask();
    const uint32_t newValueXMsb = v.m_valueX & hiWordMask();
    නම් (warnOnTruncation && (newValueMsb != v.m_value || newValueXMsb != v.m_valueX)) {
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
    නම් (lsb.isFourState() || msb.isFourState()) ආපසු setAllBitsX();
    ආපසු opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3Number& V3Number::opSel(const V3Number& lhs, uint32_t msbval, uint32_t lsbval) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    පූර්ණ_සංඛ්‍යා copyWidth = 0;
    නම් (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        const uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<පූර්ණ_සංඛ්‍යා>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<පූර්ණ_සංඛ්‍යා>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ආපසු *this;
}

V3Number& V3Number::opSelInto(const V3Number& lhs, const V3Number& lsb, පූර්ණ_සංඛ්‍යා width) {
    ආපසු opSelInto(lhs, lsb.toSInt(), width);
}

V3Number& V3Number::opSelInto(const V3Number& lhs, පූර්ණ_සංඛ්‍යා lsbval, පූර්ණ_සංඛ්‍යා width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    const පූර්ණ_සංඛ්‍යා copyWidth = std::max(0, std::min({width, lhs.width(), this->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    const පූර්ණ_සංඛ්‍යා fillWidth
        = std::max(0, std::min(width - copyWidth, this->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    ආපසු *this;
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
    dviguu_na d = 0;
    const bool negate = isSigned && noxz.isNegative();
    නම් (negate) {
        const V3Number noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    සඳහා (පූර්ණ_සංඛ්‍යා bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        නම් (noxz.bitIs1(bit)) d += exp2(bit);
    }
    නම් (negate) d = -d;
    ආපසු setDouble(d);
}
V3Number& V3Number::opRToIS(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    const dviguu_na v = VL_TRUNC(lhs.toDouble());
    const int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ආපසු setLongS(i);
}
V3Number& V3Number::opRToIRoundS(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    const dviguu_na v = VL_ROUND(lhs.toDouble());
    setZero();
    sa_ngha {
        dviguu_na d;
        uint64_t q;
    } u;
    u.d = v;
    (void)u.d;

    const පූර්ණ_සංඛ්‍යා exp = static_cast<පූර්ණ_සංඛ්‍යා>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    const පූර්ණ_සංඛ්‍යා lsb = exp - 52;
    const uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    නම් (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        සඳහා (පූර්ණ_සංඛ්‍යා bit = 0; bit <= 52; ++bit) {
            නම් (mantissa & (1ULL << bit)) {
                const පූර්ණ_සංඛ්‍යා outbit = bit + lsb;
                නම් (outbit >= 0) setBit(outbit, 1);
            }
        }
        නම් (v < 0) {
            const V3Number noSign = *this;
            opNegate(noSign);
        }
    }
    ආපසු *this;
}
V3Number& V3Number::opRealToBits(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    sa_ngha {
        dviguu_na m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    ආපසු setQuad(u.m_v);
}
V3Number& V3Number::opBitsToRealD(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    sa_ngha {
        dviguu_na m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    ආපසු setDouble(u.m_d);
}
V3Number& V3Number::opNegateD(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_DOUBLE_ARGS1(lhs);
    ආපසු setDouble(-lhs.toDouble());
}
V3Number& V3Number::opAddD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setDouble(lhs.toDouble() + rhs.toDouble());
}
V3Number& V3Number::opSubD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setDouble(lhs.toDouble() - rhs.toDouble());
}
V3Number& V3Number::opMulD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setDouble(lhs.toDouble() * rhs.toDouble());
}
V3Number& V3Number::opDivD(const V3Number& lhs, const V3Number& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setDouble(lhs.toDouble() / rhs.toDouble());
}
V3Number& V3Number::opPowD(const V3Number& lhs, const V3Number& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3Number& V3Number::opEqD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3Number& V3Number::opNeqD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3Number& V3Number::opGtD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3Number& V3Number::opGteD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3Number& V3Number::opLtD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3Number& V3Number::opLteD(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3Number& V3Number::opConcatN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setString(lhs.toString() + rhs.toString());
}
V3Number& V3Number::opReplN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_STRING_ARGS1(lhs);
    NUM_ASSERT_LOGIC_ARGS1(rhs);
    ආපසු opReplN(lhs, rhs.toUInt());
}
V3Number& V3Number::opReplN(const V3Number& lhs, uint32_t rhsval) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    සඳහා (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    ආපසු setString(out);
}
V3Number& V3Number::opCvtPackString(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    ආපසු setString(lhs.toString());
}
V3Number& V3Number::opToLowerN(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const std::string out = VString::downcase(lhs.toString());
    ආපසු setString(out);
}
V3Number& V3Number::opToUpperN(const V3Number& lhs) {
    NUM_ASSERT_OP_ARGS1(lhs);
    NUM_ASSERT_STRING_ARGS1(lhs);
    const std::string out = VString::upcase(lhs.toString());
    ආපසු setString(out);
}

V3Number& V3Number::opEqN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toString() == rhs.toString());
}
V3Number& V3Number::opNeqN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toString() != rhs.toString());
}
V3Number& V3Number::opGtN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toString() > rhs.toString());
}
V3Number& V3Number::opGteN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toString() >= rhs.toString());
}
V3Number& V3Number::opLtN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toString() < rhs.toString());
}
V3Number& V3Number::opLteN(const V3Number& lhs, const V3Number& rhs) {
    NUM_ASSERT_OP_ARGS2(lhs, rhs);
    NUM_ASSERT_STRING_ARGS2(lhs, rhs);
    ආපසු setSingleBits(lhs.toString() <= rhs.toString());
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
    UASSERT_SELFTEST(V3Number::epsilonEqual(1, 1 + std::numeric_limits<dviguu_na>::epsilon()), true);
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
