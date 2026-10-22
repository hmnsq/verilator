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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Number.h"

#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cerrno>
#ಅನ್ತರ್ಭಾವ <cmath>
#ಅನ್ತರ್ಭಾವ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ಪೂರ್ಣಾಙ್ಕ MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define ಅಂಕೆ_ASSERT_OP_ARGS1(arg1) \
    UASSERT((ಏತತ್ != &(arg1)), "Number operation called with same source and dest")
#define ಅಂಕೆ_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((ಏತತ್ != &(arg1) && ಏತತ್ != &(arg2)), \
            "Number operation called with same source and dest")
#define ಅಂಕೆ_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((ಏತತ್ != &(arg1) && ಏತತ್ != &(arg2) && ಏತತ್ != &(arg3)), \
            "Number operation called with same source and dest")
#define ಅಂಕೆ_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((ಏತತ್ != &(arg1) && ಏತತ್ != &(arg2) && ಏತತ್ != &(arg3) && ಏತತ್ != &(arg4)), \
            "Number operation called with same source and dest")

#define ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_ಸಂಖ್ಯೆ_Data::V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define ಅಂಕೆ_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg2)

#define ಅಂಕೆ_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg2); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg3); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg4)

#define ಅಂಕೆ_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define ಅಂಕೆ_ASSERT_STRING_ARGS2(arg1, arg2) \
    ಅಂಕೆ_ASSERT_STRING_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_STRING_ARGS1(arg2)

#define ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::v3errorEnd(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ಯದಿ (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } ಅನ್ಯಥಾ ಯದಿ (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } ಅನ್ಯಥಾ {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::v3errorEndFatal(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_ಸಂಖ್ಯೆ::V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, VerilogStringLiteral, ಧ್ರುವ string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    ಯದಿ (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } ಅನ್ಯಥಾ {
        init(nodep, std::max<ಪೂರ್ಣಾಙ್ಕ>(str.length() * 8, 1));
        ಪ್ರತಿ (unsigned pos = 0; pos < str.length(); ++pos) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ topos = str.length() - 1 - pos;
            ಮೌಲ್ಯ_And_X& v = m_data.ಅಂಕೆ()[topos / 4];
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < 8; ++bit) {
                ಯದಿ (str[pos] & (1UL << bit)) v.m_ಮೌಲ್ಯ |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = ಸತ್ಯ;
    opCleanThis(ಸತ್ಯ);
}

V3_ಸಂಖ್ಯೆ::V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು_DType* nodedtypep) {
    ಯದಿ (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } ಅನ್ಯಥಾ ಯದಿ (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } ಅನ್ಯಥಾ ಯದಿ (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } ಅನ್ಯಥಾ {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::create(ಧ್ರುವ ವರ್ಣ* sourcep) {
    m_data.setLogic();
    ಧ್ರುವ ವರ್ಣ* ಮೌಲ್ಯ_startp = sourcep;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = sourcep; *cp; ++cp) {
        ಯದಿ (*cp == '\'') {
            ಮೌಲ್ಯ_startp = cp + 1;
            ಭಙ್ಗ;
        }
    }

    ತಾರ್ಕಿಕ userSized = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ unbased = ಅಸತ್ಯ;
    ವರ್ಣ base = '\0';
    ಯದಿ (ಮೌಲ್ಯ_startp != sourcep) {  // Has a '
        string widthn;
        ಧ್ರುವ ವರ್ಣ* cp = sourcep;
        ಪ್ರತಿ (; *cp; ++cp) {
            ಯದಿ (*cp == '\'') {
                ++cp;
                ಭಙ್ಗ;
            }
            ಯದಿ (*cp != '_') widthn += *cp;
        }
        ಯಾವತ್ (*cp == '_') cp++;
        ಯದಿ (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(ಸತ್ಯ);
        }
        ಯದಿ (*cp) {
            base = *cp;
            ++cp;
        }
        ಮೌಲ್ಯ_startp = cp;

        userSized = widthn.length() != 0;
        ಯದಿ (std::atoi(widthn.c_str())) {
            ಯದಿ (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_ಅಂಕೆ_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_ಅಂಕೆ_Width(), ಸತ್ಯ);
            } ಅನ್ಯಥಾ {
                width(std::atoi(widthn.c_str()), ಸತ್ಯ);
            }
        }
    } ಅನ್ಯಥಾ {
        unbased = ಸತ್ಯ;
        base = 'd';
    }

    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    ಯದಿ (base == '0') {
        width(1, ಅಸತ್ಯ);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (base == '1') {
        width(1, ಅಸತ್ಯ);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (std::tolower(base) == 'z') {
        width(1, ಅಸತ್ಯ);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (std::tolower(base) == 'x') {
        width(1, ಅಸತ್ಯ);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = ಸತ್ಯ;
    }
    // Otherwise...
    ಅನ್ಯಥಾ ಯದಿ (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthBound = std::max<ಪೂರ್ಣಾಙ್ಕ>(32, std::strlen(ಮೌಲ್ಯ_startp) * 4);
        width(widthBound, ಅಸತ್ಯ);  // Will change width below
        ಯದಿ (unbased) isSigned(ಸತ್ಯ);  // Also says the spec.
    }

    // Ignore leading blanks
    ಯಾವತ್ (*ಮೌಲ್ಯ_startp == '_' || std::isspace(*ಮೌಲ್ಯ_startp)) ++ಮೌಲ್ಯ_startp;
    ಯದಿ (!*ಮೌಲ್ಯ_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    ಯದಿ (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    ಪೂರ್ಣಾಙ್ಕ obit = 0;  // Start at LSB
    ಪೂರ್ಣಾಙ್ಕ base_align = 1;
    ಯದಿ (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        ಯಾವತ್ (*ಮೌಲ್ಯ_startp == '_' || *ಮೌಲ್ಯ_startp == '0') ++ಮೌಲ್ಯ_startp;
        // Convert decimal number to hex
        ಪೂರ್ಣಾಙ್ಕ olen = 0;
        uint32_t val = 0;
        ಪೂರ್ಣಾಙ್ಕ got_x = 0;
        ಪೂರ್ಣಾಙ್ಕ got_z = 0;
        ಪೂರ್ಣಾಙ್ಕ got_01 = 0;
        ತಾರ್ಕಿಕ warnedTooMany = ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = ಮೌಲ್ಯ_startp; *cp; ++cp) {
            ವಿಕಲ್ಪ (std::tolower(*cp)) {
            ಪಕ್ಷ '0':  // FALLTHRU
            ಪಕ್ಷ '1':  // FALLTHRU
            ಪಕ್ಷ '2':  // FALLTHRU
            ಪಕ್ಷ '3':  // FALLTHRU
            ಪಕ್ಷ '4':  // FALLTHRU
            ಪಕ್ಷ '5':  // FALLTHRU
            ಪಕ್ಷ '6':  // FALLTHRU
            ಪಕ್ಷ '7':  // FALLTHRU
            ಪಕ್ಷ '8':  // FALLTHRU
            ಪಕ್ಷ '9': {
                ಯದಿ (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = val;
                } ಅನ್ಯಥಾ {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_ಸಂಖ್ಯೆ product(ಏತತ್, width() + 4);  // +4 for overflow detection
                    ಧ್ರುವ V3_ಸಂಖ್ಯೆ ten(ಏತತ್, width() + 4, 10);
                    ಧ್ರುವ V3_ಸಂಖ್ಯೆ addend(ಏತತ್, width(), (*cp - '0'));
                    product.opMul(*ಏತತ್, ten);
                    opAdd(product, addend);
                    ಯದಿ (!warnedTooMany && product.bits_ಮೌಲ್ಯ(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = ಸತ್ಯ;
                    }
                }
                ++olen;
                got_01 = 1;
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ 'z':
            ಪಕ್ಷ '?': {
                got_z = 1;
                ಯದಿ (!userSized) width(32, ಅಸತ್ಯ);
                setAllBitsZ();
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ 'x': {
                got_x = 1;
                ಯದಿ (!userSized) width(32, ಅಸತ್ಯ);
                setAllBitsX();
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ '_': ಭಙ್ಗ;
            ಸ್ವಭಾವ: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                ಭಙ್ಗ;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        ಯದಿ ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << ಮೌಲ್ಯ_startp);
        }
    } ಅನ್ಯಥಾ {
        // Convert bin/octal number to hex
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = ಮೌಲ್ಯ_startp + std::strlen(ಮೌಲ್ಯ_startp) - 1; cp >= ಮೌಲ್ಯ_startp;
             cp--) {
            ಯದಿ (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                ಭಙ್ಗ;
            }
            ವಿಕಲ್ಪ (std::tolower(base)) {
            ಪಕ್ಷ 'b': {
                base_align = 1;
                ವಿಕಲ್ಪ (std::tolower(*cp)) {
                ಪಕ್ಷ '0': setBit(obit++, 0); ಭಙ್ಗ;
                ಪಕ್ಷ '1': setBit(obit++, 1); ಭಙ್ಗ;
                ಪಕ್ಷ 'z':
                ಪಕ್ಷ '?': setBit(obit++, 'z'); ಭಙ್ಗ;
                ಪಕ್ಷ 'x': setBit(obit++, 'x'); ಭಙ್ಗ;
                ಪಕ್ಷ '_': ಭಙ್ಗ;
                ಸ್ವಭಾವ: v3error("Illegal character in binary constant: " << *cp);
                }
                ಭಙ್ಗ;
            }

            ಪಕ್ಷ 'o':
            ಪಕ್ಷ 'c': {
                base_align = 3;
                ವಿಕಲ್ಪ (std::tolower(*cp)) {  // clang-format off
                ಪಕ್ಷ '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  ಭಙ್ಗ;
                ಪಕ್ಷ '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  ಭಙ್ಗ;
                ಪಕ್ಷ '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  ಭಙ್ಗ;
                ಪಕ್ಷ '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  ಭಙ್ಗ;
                ಪಕ್ಷ '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  ಭಙ್ಗ;
                ಪಕ್ಷ '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  ಭಙ್ಗ;
                ಪಕ್ಷ '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  ಭಙ್ಗ;
                ಪಕ್ಷ '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  ಭಙ್ಗ;
                ಪಕ್ಷ 'z':
                ಪಕ್ಷ '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); ಭಙ್ಗ;
                ಪಕ್ಷ 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); ಭಙ್ಗ;
                ಪಕ್ಷ '_': ಭಙ್ಗ;
                // clang-format on
                ಸ್ವಭಾವ: v3error("Illegal character in octal constant");
                }
                ಭಙ್ಗ;
            }

            ಪಕ್ಷ 'h':  // FALLTHRU
            ಪಕ್ಷ 'x': {
                base_align = 4;
                ವಿಕಲ್ಪ (std::tolower(*cp)) {  // clang-format off
                ಪಕ್ಷ '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); ಭಙ್ಗ;
                ಪಕ್ಷ '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); ಭಙ್ಗ;
                ಪಕ್ಷ 'z':
                ಪಕ್ಷ '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); ಭಙ್ಗ;
                ಪಕ್ಷ 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); ಭಙ್ಗ;
                    // clang-format on
                ಪಕ್ಷ '_': ಭಙ್ಗ;
                ಸ್ವಭಾವ:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    ಭಙ್ಗ;
                }  // LCOV_EXCL_STOP
                ಭಙ್ಗ;
            }
            ಸ್ವಭಾವ:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                ಭಙ್ಗ;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    ಯದಿ (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), ಅಸತ್ಯ);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    ಯಾವತ್ (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(ಸತ್ಯ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::warnTooMany(ಧ್ರುವ string& ಮೌಲ್ಯ) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << ಮೌಲ್ಯ << "'");
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::nodep(Ast_ಗಂಟು* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::epsilonEqual(ದ್ವಿಗುಣ a, ದ್ವಿಗುಣ b) {
    ಪ್ರತ್ಯಾಗಮ std::fabs(a - b)
           <= (std::numeric_limits<ದ್ವಿಗುಣ>::epsilon() * std::max(1.0, std::max(a, b)));
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::epsilonIntegral(ದ್ವಿಗುಣ a) {
    ಧ್ರುವ ದ್ವಿಗುಣ dint = static_cast<ದ್ವಿಗುಣ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ಪ್ರತ್ಯಾಗಮ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

ಪೂರ್ಣಾಙ್ಕ V3_ಸಂಖ್ಯೆ::log2b(uint32_t ಅಂಕೆ) {
    // See also opCLog2
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 31; bit > 0; bit--) {
        ಯದಿ (ಅಂಕೆ & (1ULL << bit)) ಪ್ರತ್ಯಾಗಮ bit;
    }
    ಪ್ರತ್ಯಾಗಮ 0;
}

ಪೂರ್ಣಾಙ್ಕ V3_ಸಂಖ್ಯೆ::log2bQuad(uint64_t ಅಂಕೆ) {
    ಯದಿ (ಅಂಕೆ >> 32ULL) ಪ್ರತ್ಯಾಗಮ 32 + log2b(ಅಂಕೆ >> 32ULL);
    ಪ್ರತ್ಯಾಗಮ log2b(ಅಂಕೆ);
}

//======================================================================
// Setters

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setZero() {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, 0};
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setQuad(uint64_t ಮೌಲ್ಯ) {
    setZero();
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ & 0xffffffffULL;
    ಯದಿ (width() > 32) m_data.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ = (ಮೌಲ್ಯ >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setLong(uint32_t ಮೌಲ್ಯ) {
    setZero();
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setLongS(int32_t ಮೌಲ್ಯ) {
    setZero();
    ಸಙ್ಘ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = ಮೌಲ್ಯ;
    (ಶೂನ್ಯ)u.s;
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = u.u;
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setDouble(ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    ಸಙ್ಘ {
        ದ್ವಿಗುಣ d;
        uint32_t u[2];
    } u;
    u.d = ಮೌಲ್ಯ;
    (ಶೂನ್ಯ)u.d;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 2; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, 0};
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = u.u[0];
    m_data.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ = u.u[1];
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setSingleBits(ವರ್ಣ ಮೌಲ್ಯ) {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1 /*upper*/; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, 0};
    m_data.ಅಂಕೆ()[0] = {(ಮೌಲ್ಯ == '1' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 1 || ಮೌಲ್ಯ == 3),
                       (ಮೌಲ್ಯ == 'z' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 2 || ಮೌಲ್ಯ == 3)};
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setAllBits0() {
    setZero();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setAllBits1() {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {~0U, 0};
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {~0U, ~0U};
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setAllBitsZ() {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, ~0U};
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setAllBitsXRemoved() {
    ಯದಿ (!v3Global.constRemoveXs()) {
        ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    } ಅನ್ಯಥಾ {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        ಯದಿ (v3Global.opt.xAssign() == "1") {
            ಪ್ರತ್ಯಾಗಮ setAllBits1();
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ setAllBits0();
        }
    }
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setValue1() {
    m_data.ಅಂಕೆ()[0] = {1, 0};
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, 0};
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::setBitX0(ಪೂರ್ಣಾಙ್ಕ bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::setMask(ಪೂರ್ಣಾಙ್ಕ nbits, ಪೂರ್ಣಾಙ್ಕ lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

//======================================================================
// ACCESSORS - as strings

string V3_ಸಂಖ್ಯೆ::ascii(ತಾರ್ಕಿಕ prefixed, ತಾರ್ಕಿಕ cleanVerilog) ಧ್ರುವ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ಯದಿ (is1Step()) ಪ್ರತ್ಯಾಗಮ "1step";
    ಯದಿ (isNull()) ಪ್ರತ್ಯಾಗಮ "null";
    std::ostringstream out;
    ಯದಿ (isDouble()) {
        out.precision(17);
        ಯದಿ (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } ಅನ್ಯಥಾ {
            out << toDouble();
            ಯದಿ (toDouble() == floor(toDouble())) out << ".0";
        }
        ಪ್ರತ್ಯಾಗಮ out.str();
    } ಅನ್ಯಥಾ ಯದಿ (isString()) {
        ಪ್ರತ್ಯಾಗಮ '"' + toString() + '"';
    } ಅನ್ಯಥಾ {
        ಯದಿ (VL_UNCOVERABLE((m_data.ಅಂಕೆ()[ಪದಗಳು() - 1].m_ಮೌಲ್ಯ | m_data.ಅಂಕೆ()[ಪದಗಳು() - 1].m_ಮೌಲ್ಯ_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    ಯದಿ (prefixed) {
        ಯದಿ (sized()) {
            out << width() << "'";
        } ಅನ್ಯಥಾ ಯದಿ (autoExtend() && width() == 1) {
            out << "'";
            ಯದಿ (bitIs0(0)) {
                out << '0';
            } ಅನ್ಯಥಾ ಯದಿ (bitIs1(0)) {
                out << '1';
            } ಅನ್ಯಥಾ ಯದಿ (bitIsZ(0)) {
                out << 'z';
            } ಅನ್ಯಥಾ {
                out << 'x';
            }
            ಪ್ರತ್ಯಾಗಮ out.str();
        } ಅನ್ಯಥಾ {
            ಯದಿ (cleanVerilog) {
                out << "'";
            } ಅನ್ಯಥಾ {
                out << "?" << width() << "?";
            }
        }
        ಯದಿ (isSigned()) out << "s";
    }

    ಧ್ರುವ ತಾರ್ಕಿಕ ದ್ವಿಮಾನ = (isFourState()
#ifdef V3_ಸಂಖ್ಯೆ_ASCII_ದ್ವಿಮಾನ
                         // cppcheck-suppress konwnConditionTrueFalse
                         || ಸತ್ಯ
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    ಯದಿ (ದ್ವಿಮಾನ) {
        out << "b";
        out << displayed("%0b");
    } ಅನ್ಯಥಾ {
        ಯದಿ (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    ಯದಿ (isNull()) {
        ಯದಿ (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } ಅನ್ಯಥಾ {
            out << " [null]";
        }
    }
    ಪ್ರತ್ಯಾಗಮ out.str();
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(ವರ್ಣ format, ತಾರ್ಕಿಕ isScan) {
    // Is this a format letter that takes an argument in runtime?
    (ಶೂನ್ಯ)isScan;
    ವಿಕಲ್ಪ (std::tolower(format)) {
    ಪಕ್ಷ 'b': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  //ದ್ವಿಮಾನ
    ಪಕ್ಷ 'c': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Character
    ಪಕ್ಷ 'd': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Decimal
    ಪಕ್ಷ 'e': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Floating
    ಪಕ್ಷ 'f': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Floating
    ಪಕ್ಷ 'g': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Floating
    ಪಕ್ಷ 'h': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Hex
    ಪಕ್ಷ 'o': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Octal
    ಪಕ್ಷ 'p': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Pattern
    ಪಕ್ಷ 's': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // String
    ಪಕ್ಷ 't': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Time
    ಪಕ್ಷ 'u': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Packed 2-state
    ಪಕ್ಷ 'v': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Strength
    ಪಕ್ಷ 'x': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Hex
    ಪಕ್ಷ 'z': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Packed 4-state
    ಪಕ್ಷ '?': ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Internal: Compute format in V3Width based on data type
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_ಸಂಖ್ಯೆ::displayPad(size_t fmtsize, ವರ್ಣ pad, ತಾರ್ಕಿಕ ಎಡ, ಧ್ರುವ string& in) VL_PURE {
    string padding;
    ಯದಿ (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    ಪ್ರತ್ಯಾಗಮ ಎಡ ? (in + padding) : (padding + in);
}

string V3_ಸಂಖ್ಯೆ::displayedEnumName(ಧ್ರುವ AstEnumItem* itemp) {
    ಧ್ರುವ string name = VIdProtect::protect(itemp->prettyName());
    ಪ್ರತ್ಯಾಗಮ VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned ವರ್ಣ>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_ಸಂಖ್ಯೆ::displayedEnumName(ಧ್ರುವ AstEnumDType* dtypep) ಧ್ರುವ VL_MT_STABLE {
    V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{ಏತತ್, dtypep->width()};
    ಮೌಲ್ಯ.opAssign(*ಏತತ್);
    ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(itemp->valuep(), Const);
        ಯದಿ (!constp->ಅಂಕೆ().isAnyXZ() && ಮೌಲ್ಯ.isCaseEq(constp->ಅಂಕೆ()))
            ಪ್ರತ್ಯಾಗಮ displayedEnumName(itemp);
    }
    ಪ್ರತ್ಯಾಗಮ "";
}

string V3_ಸಂಖ್ಯೆ::displayedEnum(ಧ್ರುವ AstSFormatArg* argp,
                               ಧ್ರುವ string& vformat) ಧ್ರುವ VL_MT_STABLE {
    ಧ್ರುವ AstEnumDType* ಧ್ರುವ dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{ಏತತ್, dtypep->width()};
    ಮೌಲ್ಯ.opAssign(*ಏತತ್);
    ಧ್ರುವ VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ಧ್ರುವ ವರ್ಣ fmt = std::tolower(vformat.back());
    ಯದಿ (fmt != 'p' && fmt != 's') ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ.displayed(argp, vformat, numericAttr);
    string name = ಮೌಲ್ಯ.displayedEnumName(dtypep);
    ಯದಿ (name.empty()) name = ಮೌಲ್ಯ.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_ಸಂಖ್ಯೆ name_ಅಂಕೆ{ಏತತ್};
    name_ಅಂಕೆ.setString(name);
    ಪ್ರತ್ಯಾಗಮ name_ಅಂಕೆ.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_ಸಂಖ್ಯೆ::displayed(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& vformat,
                           ಧ್ರುವ VFormatAttr& formatAttr) ಧ್ರುವ VL_MT_STABLE {
    ಪ್ರತ್ಯಾಗಮ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_ಸಂಖ್ಯೆ::displayed(File_ಸಾಲು* fl, ಧ್ರುವ string& vformat,
                           ಧ್ರುವ VFormatAttr& formatAttr) ಧ್ರುವ VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *ಏತತ್);
    ++pos;
    ತಾರ್ಕಿಕ ಎಡ = ಅಸತ್ಯ;
    ಯದಿ (pos[0] == '-') {
        ಎಡ = ಸತ್ಯ;
        ++pos;
    }
    string fmtsize;
    ಪ್ರತಿ (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    ವರ್ಣ fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    ಯದಿ (formatAttr.isComplex()) {
        ಯದಿ (fmt != 'p') fmt = 's';  // override
    } ಅನ್ಯಥಾ ಯದಿ (formatAttr.isString()) {
        ಯದಿ (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_ಸಂಖ್ಯೆ strwide{fl, static_cast<ಪೂರ್ಣಾಙ್ಕ>(toString().length()) * 8, 0};
            strwide.opNToI(*ಏತತ್);
            ಪ್ರತ್ಯಾಗಮ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        ಯದಿ (fmt != 'p') fmt = 's';  // override
    } ಅನ್ಯಥಾ ಯದಿ (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        ಯದಿ (fmt == 'p') {
            ಯದಿ (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } ಅನ್ಯಥಾ {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } ಅನ್ಯಥಾ ಯದಿ (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        ಯದಿ (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_ಸಂಖ್ಯೆ nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*ಏತತ್);
            ಪ್ರತ್ಯಾಗಮ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    ವಿಕಲ್ಪ (fmt) {
    ಪಕ್ಷ 'c': {
        ಯದಿ (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ಧ್ರುವ unsigned ಪೂರ್ಣಾಙ್ಕ v = bits_ಮೌಲ್ಯ(0, 8);
        str.ತಳ್ಳು_back(static_cast<ವರ್ಣ>(v));
        ಪ್ರತ್ಯಾಗಮ str;
    }
    ಪಕ್ಷ 'e':  // FALLTHRU
    ಪಕ್ಷ 'f':  // FALLTHRU
    ಪಕ್ಷ 'g': {
        ಧ್ರುವ ದ್ವಿಗುಣ n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        ವರ್ಣ tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (ಶೂನ್ಯ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        ಪ್ರತ್ಯಾಗಮ tmp;
    }
    ಪಕ್ಷ 's': {
        ಯದಿ (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } ಅನ್ಯಥಾ {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            ಪೂರ್ಣಾಙ್ಕ bit = width() - 1;
            ತಾರ್ಕಿಕ start = ಸತ್ಯ;
            ಯಾವತ್ ((bit % 8) != 7) ++bit;
            ಪ್ರತಿ (; bit >= 0; bit -= 8) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ v = bits_ಮೌಲ್ಯ(bit - 7, 8);
                ಯದಿ (!start || v) {
                    str += static_cast<ವರ್ಣ>((v == 0) ? ' ' : v);
                    start = ಅಸತ್ಯ;  // Drop leading 0s
                } ಅನ್ಯಥಾ {
                    ಯದಿ (fmtsize != "0") str += ' ';
                }
            }
        }
        ಧ್ರುವ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ಎಡ, str);
        ಪ್ರತ್ಯಾಗಮ str;
    }
    ಪಕ್ಷ 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        ಯದಿ (formatAttr.isDouble()) {
            ಧ್ರುವ ದ್ವಿಗುಣ n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            ವರ್ಣ tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (ಶೂನ್ಯ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            ಪ್ರತ್ಯಾಗಮ tmp;
        }
        ಯದಿ (formatAttr.isString())
            ಪ್ರತ್ಯಾಗಮ '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        ಯದಿ (formatAttr.isComplex()) ಪ್ರತ್ಯಾಗಮ toString();
        ಪ್ರತ್ಯಾಗಮ "%p";
    }
    ಪಕ್ಷ 'b':  // FALLTHRU
    ಪಕ್ಷ 'o':  // FALLTHRU
    ಪಕ್ಷ 'h':  // FALLTHRU
    ಪಕ್ಷ 'x': {
        ಪೂರ್ಣಾಙ್ಕ bit = width() - 1;
        ಯದಿ (ಎಡ || !fmtsize.empty()) {
            ಯಾವತ್ (bit && bitIs0(bit)) --bit;
        }
        ವಿಕಲ್ಪ (fmt) {
        ಪಕ್ಷ 'b': {
            ಪ್ರತಿ (; bit >= 0; --bit) {
                ಯದಿ (bitIs0(bit)) {
                    str += '0';
                } ಅನ್ಯಥಾ ಯದಿ (bitIs1(bit)) {
                    str += '1';
                } ಅನ್ಯಥಾ ಯದಿ (bitIsZ(bit)) {
                    str += 'z';
                } ಅನ್ಯಥಾ {
                    str += 'x';
                }
            }
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 'o': {
            ಯಾವತ್ ((bit % 3) != 2) ++bit;
            ಪ್ರತಿ (; bit > 0; bit -= 3) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_X = ಎಣಿಕೆ_X(bit - 2, 3);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Z = ಎಣಿಕೆ_Z(bit - 2, 3);
                ಯದಿ (ಅಂಕೆ_X == 3 || ಅಂಕೆ_X == width() - (bit - 2)) {
                    str += 'x';
                    ಅನುವರ್ತ;
                }
                ಯದಿ (ಅಂಕೆ_Z == 3 || ಅಂಕೆ_Z == width() - (bit - 2)) {
                    str += 'z';
                    ಅನುವರ್ತ;
                }
                ಯದಿ (ಅಂಕೆ_X > 0) {
                    str += 'X';
                    ಅನುವರ್ತ;
                }
                ಯದಿ (ಅಂಕೆ_Z > 0) {
                    str += 'Z';
                    ಅನುವರ್ತ;
                }
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ v = bits_ಮೌಲ್ಯ(bit - 2, 3);
                str += static_cast<ವರ್ಣ>('0' + v);
            }
            ಭಙ್ಗ;
        }
        ಸ್ವಭಾವ: {  // h/x
            ಯಾವತ್ ((bit % 4) != 3) ++bit;
            ಪ್ರತಿ (; bit > 0; bit -= 4) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_X = ಎಣಿಕೆ_X(bit - 3, 4);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Z = ಎಣಿಕೆ_Z(bit - 3, 4);
                ಯದಿ (ಅಂಕೆ_X == 4 || ಅಂಕೆ_X == width() - (bit - 3)) {
                    str += 'x';
                    ಅನುವರ್ತ;
                }
                ಯದಿ (ಅಂಕೆ_Z == 4 || ಅಂಕೆ_Z == width() - (bit - 3)) {
                    str += 'z';
                    ಅನುವರ್ತ;
                }
                ಯದಿ (ಅಂಕೆ_X > 0) {
                    str += 'X';
                    ಅನುವರ್ತ;
                }
                ಯದಿ (ಅಂಕೆ_Z > 0) {
                    str += 'Z';
                    ಅನುವರ್ತ;
                }
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ v = bits_ಮೌಲ್ಯ(bit - 3, 4);
                ಯದಿ (v >= 10) {
                    str += static_cast<ವರ್ಣ>('a' + v - 10);
                } ಅನ್ಯಥಾ {
                    str += static_cast<ವರ್ಣ>('0' + v);
                }
            }
            ಭಙ್ಗ;
        }
        }  // switch
        ಧ್ರುವ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ಎಡ ? ' ' : '0'), ಎಡ, str);
        ಪ್ರತ್ಯಾಗಮ str;
    }  // case b/d/x/o
    ಪಕ್ಷ 't':  // Time
    ಪಕ್ಷ 'd': {  // Unsigned decimal
        ಧ್ರುವ ತಾರ್ಕಿಕ issigned = formatAttr.isSigned();
        ಯದಿ (fmtsize == "" && !ಎಡ) {
            ಧ್ರುವ ದ್ವಿಗುಣ mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            ದ್ವಿಗುಣ dchars = mantissabits / 3.321928094887362 + 1.0;
            ಯದಿ (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<ಪೂರ್ಣಾಙ್ಕ>(dchars));
        }
        ತಾರ್ಕಿಕ hasXZ = ಅಸತ್ಯ;
        ಯದಿ (isAllX()) {
            str = "x";
            hasXZ = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (isAllZ()) {
            str = "z";
            hasXZ = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (isAnyX()) {
            str = "X";
            hasXZ = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (isAnyZ()) {
            str = "Z";
            hasXZ = ಸತ್ಯ;
        }
        ಯದಿ (!hasXZ) {
            ಯದಿ (issigned) {
                ಯದಿ (width() > 64) {
                    str = toDecimalS();
                } ಅನ್ಯಥಾ {
                    str = cvtToStr(toSQuad());
                }
            } ಅನ್ಯಥಾ {
                ಯದಿ (width() > 64) {
                    str = toDecimalU();
                } ಅನ್ಯಥಾ {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !ಎಡ;
        // fmtsize might have changed since we parsed the %fmtsize
        ಧ್ರುವ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ಎಡ, str);
        ಪ್ರತ್ಯಾಗಮ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    ಪಕ್ಷ 'u': {  // Packed 2-state
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
            ಧ್ರುವ uint32_t v = m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ;
            str += static_cast<ವರ್ಣ>((v >> 0) & 0xff);
            str += static_cast<ವರ್ಣ>((v >> 8) & 0xff);
            str += static_cast<ವರ್ಣ>((v >> 16) & 0xff);
            str += static_cast<ವರ್ಣ>((v >> 24) & 0xff);
        }
        ಪ್ರತ್ಯಾಗಮ str;
    }
    ಪಕ್ಷ 'z': {  // Packed 4-state
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
            ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ >> 0) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ >> 8) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ >> 16) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ >> 24) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ_X >> 0) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ_X >> 8) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ_X >> 16) & 0xff);
            str += static_cast<ವರ್ಣ>((v.m_ಮೌಲ್ಯ_X >> 24) & 0xff);
        }
        ಪ್ರತ್ಯಾಗಮ str;
    }
    ಪಕ್ಷ 'v': {  // Strength
        ಪೂರ್ಣಾಙ್ಕ bit = width() - 1;
        ಪ್ರತಿ (; bit >= 0; bit--) {
            ಯದಿ (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            ಅನ್ಯಥಾ ಯದಿ (bitIs1(bit)) {
                str += "St1 ";
            } ಅನ್ಯಥಾ ಯದಿ (bitIsZ(bit)) {
                str += "StZ ";
            } ಅನ್ಯಥಾ {
                str += "StX";
            }
        }
        ಪ್ರತ್ಯಾಗಮ str;
    }
    ಸ್ವಭಾವ: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_ಸಂಖ್ಯೆ::emitC() ಧ್ರುವ VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    ವರ್ಣ sbuf[bufsize];
    string result;
    ಯದಿ (isNull()) {
        ಪ್ರತ್ಯಾಗಮ "VlNull{}";
    } ಅನ್ಯಥಾ ಯದಿ (isDouble()) {
        ಧ್ರುವ ದ್ವಿಗುಣ dnum = toDouble();
        ಯದಿ (std::isinf(dnum)) {
            ಯದಿ (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } ಅನ್ಯಥಾ ಯದಿ (std::isnan(dnum)) {
            ಯದಿ (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ fmt = (static_cast<ಪೂರ್ಣಾಙ್ಕ>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ಪ್ರತ್ಯಾಗಮ sbuf;
        }
    } ಅನ್ಯಥಾ ಯದಿ (isString()) {
        ಧ್ರುವ string strg = toString();
        ಧ್ರುವ string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ಪ್ರತ್ಯಾಗಮ '"' + quoted + "\"s";
    } ಅನ್ಯಥಾ ಯದಿ (ಪದಗಳು() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(ಪದಗಳು()) + ">{{";
        ಯದಿ (ಪದಗಳು() > 4) result += '\n';
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ n = 0; n < ಪದಗಳು(); ++n) {
            ಯದಿ (n) result += ((n % 4) ? ", " : ",\n");
            (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        ಯದಿ (ಪದಗಳು() > 4) result += '\n';
        result += "}}";
    } ಅನ್ಯಥಾ ಯದಿ (ಪದಗಳು() == 2) {  // Quad
        ಧ್ರುವ uint64_t qnum = static_cast<uint64_t>(toUQuad());
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ಪ್ರತ್ಯಾಗಮ sbuf;
    } ಅನ್ಯಥಾ {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ಧ್ರುವ uint32_t unum = toUInt();
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ಪ್ರತ್ಯಾಗಮ sbuf;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string V3_ಸಂಖ್ಯೆ::toDecimalS() ಧ್ರುವ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ಯದಿ (isNegative()) {
        V3_ಸಂಖ್ಯೆ lhsNoSign = *ಏತತ್;
        lhsNoSign.opNegate(*ಏತತ್);
        ಪ್ರತ್ಯಾಗಮ "-"s + lhsNoSign.toDecimalU();
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ toDecimalU();
    }
}

string V3_ಸಂಖ್ಯೆ::toDecimalU() ಧ್ರುವ VL_MT_STABLE {
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_ಸಂಖ್ಯೆ bcd(ಏತತ್, maxdecwidth + 4);
    V3_ಸಂಖ್ಯೆ tmp(ಏತತ್, maxdecwidth + 4);
    V3_ಸಂಖ್ಯೆ tmp2(ಏತತ್, maxdecwidth + 4);

    ಪೂರ್ಣಾಙ್ಕ from_bit = width() - 1;
    // Skip all leading zeros
    ಪ್ರತಿ (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    ಪ್ರತಿ (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            ಯದಿ (bcd.bits_ಮೌಲ್ಯ(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_ಸಂಖ್ಯೆ(ಏತತ್, 32, 1));
        // bcd[0] = this[from_bit]
        ಯದಿ (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    ಪೂರ್ಣಾಙ್ಕ lsb = (maxdecwidth - 1) & ~3;
    ಪ್ರತಿ (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        ಯದಿ (bcd.bits_ಮೌಲ್ಯ(lsb, 4)) ಭಙ್ಗ;
    }
    ಪ್ರತಿ (; lsb >= 0; lsb -= 4) {
        output += static_cast<ವರ್ಣ>('0' + bcd.bits_ಮೌಲ್ಯ(lsb, 4));  // 0..9
    }
    ಪ್ರತ್ಯಾಗಮ output;
}

string V3_ಸಂಖ್ಯೆ::toRealString() ಧ್ರುವ VL_MT_STABLE {
    ಧ್ರುವ ದ್ವಿಗುಣ ಮೌಲ್ಯ = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << ಮೌಲ್ಯ;
    string result = out.str();
    ಯದಿ (std::strtod(result.c_str(), nullptr) != ಮೌಲ್ಯ) {
        out.str("");
        out.precision(17);
        out << ಮೌಲ್ಯ;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    ಯದಿ (result.find_first_of(".en") == string::npos) result += ".0";
    ಪ್ರತ್ಯಾಗಮ result;
}

//======================================================================
// ACCESSORS - as numbers

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::fitsInUInt() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isFourState()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    // We allow wide numbers that represent values <= 32 bits
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i < ಪದಗಳು(); ++i) {
        ಯದಿ (m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
uint32_t V3_ಸಂಖ್ಯೆ::toUInt() ಧ್ರುವ VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *ಏತತ್);
    // We allow wide numbers that represent values <= 32 bits
    ಯದಿ (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *ಏತತ್);
    }
    ಪ್ರತ್ಯಾಗಮ m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ;
}

ದ್ವಿಗುಣ V3_ಸಂಖ್ಯೆ::toDouble() ಧ್ರುವ VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    ಸಙ್ಘ {
        ದ್ವಿಗುಣ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ;
    u.u[1] = m_data.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ;
    ಪ್ರತ್ಯಾಗಮ u.d;
}

int32_t V3_ಸಂಖ್ಯೆ::toSInt() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isSigned()) {
        ಧ್ರುವ uint32_t v = toUInt();
        ಧ್ರುವ uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        ಧ್ರುವ uint32_t extended = v | signExtend;
        ಪ್ರತ್ಯಾಗಮ static_cast<int32_t>(extended);
    } ಅನ್ಯಥಾ {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ಪ್ರತ್ಯಾಗಮ static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_ಸಂಖ್ಯೆ::toUQuad() ಧ್ರುವ VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *ಏತತ್);
    // We allow wide numbers that represent values <= 64 bits
    ಯದಿ (isDouble()) ಪ್ರತ್ಯಾಗಮ static_cast<uint64_t>(toDouble());
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 2; i < ಪದಗಳು(); ++i) {
        ಯದಿ (m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ) {
            v3error("Value too wide for 64-bits expected in this context " << *ಏತತ್);
            ಭಙ್ಗ;
        }
    }
    ಯದಿ (width() <= 32) ಪ್ರತ್ಯಾಗಮ (static_cast<uint64_t>(toUInt()));
    ಪ್ರತ್ಯಾಗಮ ((static_cast<uint64_t>(m_data.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ) << 32ULL)
            | (static_cast<uint64_t>(m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ)));
}

int64_t V3_ಸಂಖ್ಯೆ::toSQuad() ಧ್ರುವ {
    ಯದಿ (isDouble()) ಪ್ರತ್ಯಾಗಮ static_cast<int64_t>(toDouble());
    ಧ್ರುವ uint64_t v = toUQuad();
    ಧ್ರುವ uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    ಧ್ರುವ uint64_t extended = v | signExtend;
    ಪ್ರತ್ಯಾಗಮ static_cast<int64_t>(extended);
}

string V3_ಸಂಖ್ಯೆ::toString() ಧ್ರುವ {
    UASSERT(!isFourState(), "toString with 4-state " << *ಏತತ್);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ m_data.str();
    ಪೂರ್ಣಾಙ್ಕ bit = width() - 1;
    ಯಾವತ್ ((bit % 8) != 7) ++bit;
    std::string str;
    ಪ್ರತಿ (; bit >= 0; bit -= 8) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ v = bits_ಮೌಲ್ಯ(bit - 7, 8);
        ಯದಿ (v) str += static_cast<ವರ್ಣ>(v);
    }
    ಪ್ರತ್ಯಾಗಮ str;
}

uint32_t V3_ಸಂಖ್ಯೆ::edataWord(ಪೂರ್ಣಾಙ್ಕ eword) ಧ್ರುವ {
    UASSERT(!isFourState(), "edataWord with 4-state " << *ಏತತ್);
    ಪ್ರತ್ಯಾಗಮ m_data.ಅಂಕೆ()[eword].m_ಮೌಲ್ಯ;
}

uint8_t V3_ಸಂಖ್ಯೆ::dataByte(ಪೂರ್ಣಾಙ್ಕ byte) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAllZ() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isDouble() || isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಧ್ರುವ uint32_t mask = i == ಪದಗಳು() - 1 ? hiWordMask() : ~0U;
        ಯದಿ (((~v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) & mask) != mask) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAllX() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isDouble() || isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    uint32_t mask = hiWordMask();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = ಪದಗಳು() - 1; i >= 0; --i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಯದಿ ((v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) ^ mask) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        mask = ~0U;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isEqZero() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ m_data.str().empty();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಯದಿ (v.m_ಮೌಲ್ಯ || v.m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isNeqZero() ಧ್ರುವ {
    ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ !m_data.str().empty();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಯದಿ (v.m_ಮೌಲ್ಯ & ~v.m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isBitsZero(ಪೂರ್ಣಾಙ್ಕ msb, ಪೂರ್ಣಾಙ್ಕ lsb) ಧ್ರುವ {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = lsb; i <= msb; ++i) {
        ಯದಿ (VL_UNLIKELY(!bitIs0(i))) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isEqOne() ಧ್ರುವ {
    ಯದಿ (m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ != 1 || m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i < ಪದಗಳು(); ++i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಯದಿ (v.m_ಮೌಲ್ಯ || v.m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isEqAllOnes(ಪೂರ್ಣಾಙ್ಕ optwidth) ಧ್ರುವ {
    // Correct number of zero bits/width matters
    ಯದಿ (!optwidth) optwidth = width();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < optwidth; ++bit) {
        ಯದಿ (!bitIs1(bit)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isFourState() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isDouble() || isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಯದಿ (m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAnyX() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isDouble() || isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಯದಿ (v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAnyXZ() ಧ್ರುವ {
    ಯದಿ (isDouble() || isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಯದಿ (m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAnyZ() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (isDouble() || isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
        ಯದಿ (~v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isLtXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) ಧ್ರುವ {
    // Include X/Z in comparisons for sort ordering
    ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ toString() < rhs.toString();
    ಯದಿ (isDouble()) ಪ್ರತ್ಯಾಗಮ toDouble() < rhs.toDouble();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        ಯದಿ (bitIs1(bit) && rhs.bitIs0(bit)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (rhs.bitIs1(bit) && bitIs0(bit)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (bitIsXZ(bit)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (rhs.bitIsXZ(bit)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}
ಪೂರ್ಣಾಙ್ಕ V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_X(ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ nbits) ಧ್ರುವ VL_MT_SAFE {
    ಪೂರ್ಣಾಙ್ಕ ಎಣಿಕೆ = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bitn = 0; bitn < nbits; ++bitn) {
        ಯದಿ (lsb + bitn >= width()) ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ;
        ಯದಿ (bitIsX(lsb + bitn)) ++ಎಣಿಕೆ;
    }
    ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ;
}
ಪೂರ್ಣಾಙ್ಕ V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_Z(ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ nbits) ಧ್ರುವ VL_MT_SAFE {
    ಪೂರ್ಣಾಙ್ಕ ಎಣಿಕೆ = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bitn = 0; bitn < nbits; ++bitn) {
        ಯದಿ (lsb + bitn >= width()) ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ;
        ಯದಿ (bitIsZ(lsb + bitn)) ++ಎಣಿಕೆ;
    }
    ಪ್ರತ್ಯಾಗಮ ಎಣಿಕೆ;
}

ಪೂರ್ಣಾಙ್ಕ V3_ಸಂಖ್ಯೆ::widthToFit() ಧ್ರುವ {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = width() - 1; bit > 0; bit--) {
        ಯದಿ (!bitIs0(bit)) ಪ್ರತ್ಯಾಗಮ bit + 1;
    }
    ಪ್ರತ್ಯಾಗಮ 1;  // one bit even if number is == 0
}

uint32_t V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_Bits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl) ಧ್ರುವ {
    // Correct number of zero bits/width matters
    ಪೂರ್ಣಾಙ್ಕ n = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ವಿಕಲ್ಪ (ctrl.bitIs(0)) {
        ಪಕ್ಷ '0':
            ಯದಿ (bitIs0(bit)) ++n;
            ಭಙ್ಗ;
        ಪಕ್ಷ '1':
            ಯದಿ (bitIs1(bit)) ++n;
            ಭಙ್ಗ;
        ಪಕ್ಷ 'x':
            ಯದಿ (bitIsX(bit)) ++n;
            ಭಙ್ಗ;
        ಪಕ್ಷ 'z':
            ಯದಿ (bitIsZ(bit)) ++n;
            ಭಙ್ಗ;
        }
    }
    ಪ್ರತ್ಯಾಗಮ n;
}

uint32_t V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_Bits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl1, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl2,
                             ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl3) ಧ್ರುವ {
    // Correct number of zero bits/width matters
    ಪೂರ್ಣಾಙ್ಕ n = ಎಣಿಕೆ_Bits(ctrl1);
    ಯದಿ (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += ಎಣಿಕೆ_Bits(ctrl2);
    ಯದಿ ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += ಎಣಿಕೆ_Bits(ctrl3);
    }
    ಪ್ರತ್ಯಾಗಮ n;
}

uint32_t V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_Ones() ಧ್ರುವ {
    ಪೂರ್ಣಾಙ್ಕ n = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (bitIs1(bit)) ++n;
    }
    ಪ್ರತ್ಯಾಗಮ n;
}

uint32_t V3_ಸಂಖ್ಯೆ::mostSetBitP1() ಧ್ರುವ {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = width() - 1; bit >= 0; bit--) {
        ಯದಿ (!bitIs0(bit)) ಪ್ರತ್ಯಾಗಮ bit + 1;
    }
    ಪ್ರತ್ಯಾಗಮ 0;
}

uint32_t V3_ಸಂಖ್ಯೆ::leastSetBitP1() ಧ್ರುವ {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (!bitIs0(bit)) ಪ್ರತ್ಯಾಗಮ bit + 1;
    }
    ಪ್ರತ್ಯಾಗಮ 0;
}

//======================================================================

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBitsNonXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBitsOne(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBitsXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBitsZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRedOr(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // op i, 1 bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ವರ್ಣ outc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < lhs.width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit)) {
            ಪ್ರತ್ಯಾಗಮ setSingleBits(1);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(bit)) {
        } ಅನ್ಯಥಾ {
            outc = 'x';
        }
    }
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRedAnd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ವರ್ಣ outc = 1;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < lhs.width(); ++bit) {
        ಯದಿ (lhs.bitIs0(bit)) {
            ಪ್ರತ್ಯಾಗಮ setSingleBits(0);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs1(bit)) {
        } ಅನ್ಯಥಾ {
            outc = 'x';
        }
    }
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRedXor(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // op i, 1 bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ವರ್ಣ outc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < lhs.width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit)) {
            ಯದಿ (outc == 1) {
                outc = 0;
            } ಅನ್ಯಥಾ ಯದಿ (outc == 0) {
                outc = 1;
            }
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(bit)) {
        } ಅನ್ಯಥಾ {
            outc = 'x';
        }
    }
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಎಣಿಕೆ_Bits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಭಿವ್ಯಕ್ತಿ, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl1, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl2,
                                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl3) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS4(ಅಭಿವ್ಯಕ್ತಿ, ctrl1, ctrl2, ctrl3);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS4(ಅಭಿವ್ಯಕ್ತಿ, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಅಭಿವ್ಯಕ್ತಿ.ಎಣಿಕೆ_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಎಣಿಕೆ_Ones(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಯದಿ (lhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    setZero();
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = lhs.ಎಣಿಕೆ_Ones();
    opCleanThis();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opIsUnknown(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.isAnyXZ());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opOneHot(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಯದಿ (lhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.ಎಣಿಕೆ_Ones() == 1);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opOneHot0(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಯದಿ (lhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.ಎಣಿಕೆ_Ones() <= 1);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCLog2(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಯದಿ (lhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ adjust = (lhs.ಎಣಿಕೆ_Ones() == 1) ? 0 : 1;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = lhs.width() - 1; bit >= 0; bit--) {
        ಯದಿ (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
    }
    setZero();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMostSetBitP1(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಯದಿ (lhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = lhs.width() - 1; bit >= 0; bit--) {
        ಯದಿ (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
    }
    setZero();
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogNot(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    ವರ್ಣ outc = 1;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < lhs.width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit)) {
            outc = 0;
            ಗಚ್ಛ last;
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(bit)) {
        } ಅನ್ಯಥಾ {
            outc = 'x';
        }
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNot(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAnd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } ಅನ್ಯಥಾ {
            setBit(bit, 'x');
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opOr(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } ಅನ್ಯಥಾ {
            setBit(bit, 'x');
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opXor(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::fillBits(ಪೂರ್ಣಾಙ್ಕ destLsb, ಪೂರ್ಣಾಙ್ಕ width, ವರ್ಣ ಮೌಲ್ಯ) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    ಯದಿ (!width) ಪ್ರತ್ಯಾಗಮ;
    UASSERT(width <= ಏತತ್->width() - destLsb, "Destination bit range exceeds number width");

    ಧ್ರುವ uint32_t ಮೌಲ್ಯ_Word = ಮೌಲ್ಯ == '1' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 1 || ಮೌಲ್ಯ == 3 ? ~0U : 0;
    ಧ್ರುವ uint32_t xWord = ಮೌಲ್ಯ == 'z' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 2 || ಮೌಲ್ಯ == 3 ? ~0U : 0;
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ firstWord = VL_BITWORD_E(destLsb);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lastWord = VL_BITWORD_E(destLsb + width - 1);
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ word = firstWord; word <= lastWord; ++word) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        ಧ್ರುವ uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        ಮೌಲ್ಯ_And_X& dest = m_data.ಅಂಕೆ()[word];
        dest.m_ಮೌಲ್ಯ = (dest.m_ಮೌಲ್ಯ & ~mask) | (ಮೌಲ್ಯ_Word & mask);
        dest.m_ಮೌಲ್ಯ_X = (dest.m_ಮೌಲ್ಯ_X & ~mask) | (xWord & mask);
    }
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::copyBits(ಪೂರ್ಣಾಙ್ಕ destLsb, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& source, ಪೂರ್ಣಾಙ್ಕ sourceLsb, ಪೂರ್ಣಾಙ್ಕ width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    ಯದಿ (!width) ಪ್ರತ್ಯಾಗಮ;
    UASSERT(width <= ಏತತ್->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ firstWord = VL_BITWORD_E(destLsb);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lastWord = VL_BITWORD_E(destLsb + width - 1);
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ destWord = firstWord; destWord <= lastWord; ++destWord) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ destOffset = VL_BITBIT_E(destBit);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sourceBit = sourceLsb + destBit - destLsb;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sourceWord = VL_BITWORD_E(sourceBit);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ sourceOffset = VL_BITBIT_E(sourceBit);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        ಧ್ರುವ uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        ಧ್ರುವ ಮೌಲ್ಯ_And_X& sourceLo = source.m_data.ಅಂಕೆ()[sourceWord];
        uint32_t ಮೌಲ್ಯ = sourceLo.m_ಮೌಲ್ಯ >> sourceOffset;
        uint32_t ಮೌಲ್ಯ_X = sourceLo.m_ಮೌಲ್ಯ_X >> sourceOffset;
        ಯದಿ (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            ಧ್ರುವ ಮೌಲ್ಯ_And_X& sourceHi = source.m_data.ಅಂಕೆ()[sourceWord + 1];
            ಮೌಲ್ಯ |= sourceHi.m_ಮೌಲ್ಯ << (VL_EDATASIZE - sourceOffset);
            ಮೌಲ್ಯ_X |= sourceHi.m_ಮೌಲ್ಯ_X << (VL_EDATASIZE - sourceOffset);
        }

        ಮೌಲ್ಯ_And_X& dest = m_data.ಅಂಕೆ()[destWord];
        dest.m_ಮೌಲ್ಯ = (dest.m_ಮೌಲ್ಯ & ~mask) | ((ಮೌಲ್ಯ << destOffset) & mask);
        dest.m_ಮೌಲ್ಯ_X = (dest.m_ಮೌಲ್ಯ_X & ~mask) | ((ಮೌಲ್ಯ_X << destOffset) & mask);
    }
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opConcat(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    ಯದಿ (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLenN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRepl(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs,
                           ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ಪ್ರತ್ಯಾಗಮ opRepl(lhs, rhs.toUInt());
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRepl(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಯದಿ (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    ಪ್ರತಿ (unsigned times = 0; times < rhsval; ++times) {
        ಧ್ರುವ uint64_t destLsb = uint64_t{times} * lhs.width();
        ಯದಿ (destLsb >= static_cast<uint32_t>(width())) ಭಙ್ಗ;
        copyBits(static_cast<ಪೂರ್ಣಾಙ್ಕ>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<ಪೂರ್ಣಾಙ್ಕ>(destLsb)));
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opStreamL(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    ಯದಿ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ istart = 0; istart < lhs.width(); istart += ssize) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogAnd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ವರ್ಣ loutc = 0;
    ವರ್ಣ routc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < lhs.width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit)) {
            loutc = 1;
            ಭಙ್ಗ;
        }
        ಯದಿ (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < rhs.width(); ++bit) {
        ಯದಿ (rhs.bitIs1(bit)) {
            routc = 1;
            ಭಙ್ಗ;
        }
        ಯದಿ (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    ವರ್ಣ outc = 'x';
    ಯದಿ (routc == 1 && loutc == 1) outc = 1;
    ಯದಿ (routc == 0 || loutc == 0) outc = 0;
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogOr(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ವರ್ಣ outc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < lhs.width(); ++bit) {
        ಯದಿ (lhs.bitIs1(bit)) {
            outc = 1;
            ಗಚ್ಛ last;
        }
        ಯದಿ (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < rhs.width(); ++bit) {
        ಯದಿ (rhs.bitIs1(bit)) {
            outc = 1;
            ಗಚ್ಛ last;
        }
        ಯದಿ (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogIf(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ lnot = lhs;
    lnot.opLogNot(lhs);
    ಪ್ರತ್ಯಾಗಮ opLogOr(lnot, rhs);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_ಸಂಖ್ಯೆ ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ಪ್ರತ್ಯಾಗಮ opLogAnd(ifa, ifb);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAtoN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಙ್ಕ base) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    ಯದಿ (base == AstAtoN::ATOREAL) ಪ್ರತ್ಯಾಗಮ setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    ಯದಿ (errno != 0) v = 0;
    ಪ್ರತ್ಯಾಗಮ setLongS(static_cast<int32_t>(v));
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPutcN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths) {
    ಅಂಕೆ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    ಧ್ರುವ int32_t i = rhs.toSInt();
    ಧ್ರುವ int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    ಯದಿ (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    ಪ್ರತ್ಯಾಗಮ setString(lstring);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGetcN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಧ್ರುವ string lstring = lhs.toString();
    ಧ್ರುವ int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    ಯದಿ (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    ಪ್ರತ್ಯಾಗಮ setLong(v);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSubstrN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths) {
    ಅಂಕೆ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಧ್ರುವ string lstring = lhs.toString();
    ಧ್ರುವ int32_t i = rhs.toSInt();
    ಧ್ರುವ int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    ಯದಿ (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) ಪ್ರತ್ಯಾಗಮ setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ಪ್ರತ್ಯಾಗಮ setString(lstring.substr(i, j - i + 1));
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCompareNN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ತಾರ್ಕಿಕ ignoreCase) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    ಪೂರ್ಣಾಙ್ಕ result;
    ಧ್ರುವ string lstring = lhs.toString();
    ಧ್ರುವ string rstring = rhs.toString();
    ಯದಿ (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } ಅನ್ಯಥಾ {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ಪ್ರತ್ಯಾಗಮ setLongS(result);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isString()) ಪ್ರತ್ಯಾಗಮ opEqN(lhs, rhs);
    ಯದಿ (lhs.isDouble()) ಪ್ರತ್ಯಾಗಮ opEqD(lhs, rhs);
    ವರ್ಣ outc = 1;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ಯದಿ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            ಗಚ್ಛ last;
        }
        ಯದಿ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            ಗಚ್ಛ last;
        }
        ಯದಿ (lhs.bitIsXZ(bit)) outc = 'x';
        ಯದಿ (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNeq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isString()) ಪ್ರತ್ಯಾಗಮ opNeqN(lhs, rhs);
    ಯದಿ (lhs.isDouble()) ಪ್ರತ್ಯಾಗಮ opNeqD(lhs, rhs);
    ವರ್ಣ outc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ಯದಿ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            ಗಚ್ಛ last;
        }
        ಯದಿ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            ಗಚ್ಛ last;
        }
        ಯದಿ (lhs.bitIsXZ(bit)) outc = 'x';
        ಯದಿ (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3Hash V3_ಸಂಖ್ಯೆ::toHash() ಧ್ರುವ {
    // Shouldmatch isCaseEq
    ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ V3Hash{toString()};
    ಯದಿ (isDouble()) ಪ್ರತ್ಯಾಗಮ V3Hash{toDouble()};
    V3Hash hash{width()};
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) hash += m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ;
    ಪ್ರತ್ಯಾಗಮ hash;
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isCaseEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) ಧ್ರುವ {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ toString() == rhs.toString();
    ಯದಿ (isDouble()) ಪ್ರತ್ಯಾಗಮ toDouble() == rhs.toDouble();
    ಯದಿ (width() != rhs.width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಯದಿ (!(m_data.ಅಂಕೆ()[i] == rhs.m_data.ಅಂಕೆ()[i])) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCaseEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCaseNeq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ವರ್ಣ outc = 0;
    ಯದಿ (lhs.isString()) {
        ಪ್ರತ್ಯಾಗಮ opNeqN(lhs, rhs);
    } ಅನ್ಯಥಾ ಯದಿ (lhs.isDouble()) {
        ಪ್ರತ್ಯಾಗಮ opNeqD(lhs, rhs);
    }
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ಯದಿ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            ಗಚ್ಛ last;
        }
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opWildEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ವರ್ಣ outc = 1;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ಯದಿ (!rhs.bitIsXZ(bit)) {
            ಯದಿ (lhs.bitIsXZ(bit)) {
                outc = 'x';
                ಗಚ್ಛ last;
            }
            ಯದಿ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                ಗಚ್ಛ last;
            }
        }
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opWildNeq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ವರ್ಣ outc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ಯದಿ (!rhs.bitIsXZ(bit)) {
            ಯದಿ (lhs.bitIsXZ(bit)) {
                outc = 'x';
                ಗಚ್ಛ last;
            }
            ಯದಿ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                ಗಚ್ಛ last;
            }
        }
    }
last:
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGt(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ವರ್ಣ outc = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ಯದಿ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        ಯದಿ (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        ಯದಿ (lhs.bitIsXZ(bit)) outc = 'x';
        ಯದಿ (rhs.bitIsXZ(bit)) outc = 'x';
    }
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGtS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ವರ್ಣ outc = 0;
    {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        ಯದಿ (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } ಅನ್ಯಥಾ ಯದಿ (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } ಅನ್ಯಥಾ ಯದಿ (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } ಅನ್ಯಥಾ {
            // both positive or negative, normal >
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                ಯದಿ (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                ಯದಿ (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                ಯದಿ (lhs.bitIsXZ(bit)) outc = 'x';
                ಯದಿ (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    ಪ್ರತ್ಯಾಗಮ setSingleBits(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGte(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ& eq = opEq(lhs, rhs);
    ಯದಿ (eq.isNeqZero()) ಪ್ರತ್ಯಾಗಮ eq;  // Return true
    ಪ್ರತ್ಯಾಗಮ opGt(lhs, rhs);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGteS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ& eq = opEq(lhs, rhs);
    ಯದಿ (eq.isNeqZero()) ಪ್ರತ್ಯಾಗಮ eq;  // Return true
    ಪ್ರತ್ಯಾಗಮ opGtS(lhs, rhs);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLt(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { ಪ್ರತ್ಯಾಗಮ opGt(rhs, lhs); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLte(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { ಪ್ರತ್ಯಾಗಮ opGte(rhs, lhs); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLtS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { ಪ್ರತ್ಯಾಗಮ opGtS(rhs, lhs); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLteS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) { ಪ್ರತ್ಯಾಗಮ opGteS(rhs, lhs); }

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opShiftR(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    setZero();
    ಯದಿ (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    ಧ್ರುವ uint32_t rhsval = rhs.toUInt();
    ಯದಿ (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<ಪೂರ್ಣಾಙ್ಕ>(rhsval),
                 std::min(width(), lhs.width() - static_cast<ಪೂರ್ಣಾಙ್ಕ>(rhsval)));
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opShiftRS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    setZero();
    ಧ್ರುವ ತಾರ್ಕಿಕ overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    ಯದಿ (!overflow) {
        ಧ್ರುವ uint32_t rhsval = rhs.toUInt();
        ಯದಿ (rhsval < lbits) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ copyWidth = std::min(width(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(lbits - rhsval));
            copyBits(0, lhs, static_cast<ಪೂರ್ಣಾಙ್ಕ>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opShiftL(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    setZero();
    ಯದಿ (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    ಧ್ರುವ uint32_t rhsval = rhs.toUInt();
    ಯದಿ (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<ಪೂರ್ಣಾಙ್ಕ>(rhsval), lhs, 0,
                 std::min(width() - static_cast<ಪೂರ್ಣಾಙ್ಕ>(rhsval), lhs.width()));
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

//======================================================================
// Ops - Arithmetic

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNegate(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಯದಿ (lhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    V3_ಸಂಖ್ಯೆ notlhs(&lhs, width());
    notlhs.opNot(lhs);
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ one(&lhs, width(), 1);
    opAdd(notlhs, one);
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAdd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ word = 0; word < ಪದಗಳು(); ++word) {
        ಧ್ರುವ uint64_t lwordval = lhs.m_data.ಅಂಕೆ()[word].m_ಮೌಲ್ಯ;
        ಧ್ರುವ uint64_t rwordval = rhs.m_data.ಅಂಕೆ()[word].m_ಮೌಲ್ಯ;
        ಧ್ರುವ uint64_t sum = lwordval + rwordval + carry;
        m_data.ಅಂಕೆ()[word].m_ಮೌಲ್ಯ = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSub(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    V3_ಸಂಖ್ಯೆ negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    ಪ್ರತ್ಯಾಗಮ opAdd(lhs, negrhs);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMul(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    setZero();
    ಯದಿ (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } ಅನ್ಯಥಾ {
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ lword = 0; lword < lhs.ಪದಗಳು(); ++lword) {
            ಧ್ರುವ uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.ಅಂಕೆ()[lword].m_ಮೌಲ್ಯ);
            ಯದಿ (lwordval == 0) ಅನುವರ್ತ;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ rword = 0; rword < rhs.ಪದಗಳು(); ++rword) {
                ಧ್ರುವ uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.ಅಂಕೆ()[rword].m_ಮೌಲ್ಯ);
                ಯದಿ (rwordval == 0) ಅನುವರ್ತ;
                uint64_t mul = lwordval * rwordval;
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ qword = lword + rword; qword < ಪದಗಳು(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.ಅಂಕೆ()[qword].m_ಮೌಲ್ಯ);
                    m_data.ಅಂಕೆ()[qword].m_ಮೌಲ್ಯ = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    ಯದಿ (mul == 0) ಭಙ್ಗ;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMulS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    V3_ಸಂಖ್ಯೆ lhsNoSign = lhs;
    ಯದಿ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ಸಂಖ್ಯೆ rhsNoSign = rhs;
    ಯದಿ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ qNoSign = opMul(lhsNoSign, rhsNoSign);
    ಯದಿ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ಅನ್ಯಥಾ {
        opAssign(qNoSign);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opDiv(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಯದಿ (rhs.isEqZero()) ಪ್ರತ್ಯಾಗಮ setAllBitsXRemoved();
    ಯದಿ (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    } ಅನ್ಯಥಾ {
        // Wide division
        ಪ್ರತ್ಯಾಗಮ opModDivGuts(lhs, rhs, ಅಸತ್ಯ);
    }
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opDivS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಯದಿ (rhs.isEqZero()) ಪ್ರತ್ಯಾಗಮ setAllBitsXRemoved();
    V3_ಸಂಖ್ಯೆ lhsNoSign = lhs;
    ಯದಿ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ಸಂಖ್ಯೆ rhsNoSign = rhs;
    ಯದಿ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    ಯದಿ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ಅನ್ಯಥಾ {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *ಏತತ್);
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opModDiv(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಯದಿ (rhs.isEqZero()) ಪ್ರತ್ಯಾಗಮ setAllBitsXRemoved();
    ಯದಿ (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    } ಅನ್ಯಥಾ {
        // Wide modulus
        ಪ್ರತ್ಯಾಗಮ opModDivGuts(lhs, rhs, ಸತ್ಯ);
    }
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opModDivS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಯದಿ (rhs.isEqZero()) ಪ್ರತ್ಯಾಗಮ setAllBitsXRemoved();
    V3_ಸಂಖ್ಯೆ lhsNoSign = lhs;
    ಯದಿ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ಸಂಖ್ಯೆ rhsNoSign = rhs;
    ಯದಿ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ಧ್ರುವ V3_ಸಂಖ್ಯೆ qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    ಯದಿ (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } ಅನ್ಯಥಾ {
        opAssign(qNoSign);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opModDivGuts(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ತಾರ್ಕಿಕ is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ l_ಪದಗಳು = lhs.ಪದಗಳು();
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ umsbp1 = lhs.mostSetBitP1();  // dividend
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ vmsbp1 = rhs.mostSetBitP1();  //ಭಾಜಕ
    ಯದಿ (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *ಏತತ್);
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    }

    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    ಯದಿ (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ j = uw - 1; j >= 0; j--) {
            ಧ್ರುವ uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ));
            m_data.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ = unw64 / static_cast<uint64_t>(rhs.m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ);
            k = unw64
                - (static_cast<uint64_t>(m_data.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ)
                   * static_cast<uint64_t>(rhs.m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *ಏತತ್ << " rem=0x" << std::hex
                                   << k << std::dec);
        ಯದಿ (is_modulus) {
            setZero();
            m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = k;
        }
        opCleanThis();
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_ಪದಗಳು + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_ಪದಗಳು + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < l_ಪದಗಳು; ++i) m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ = 0;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < l_ಪದಗಳು + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ಧ್ರುವ uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ << s)
                | (shift_mask & (rhs.m_data.ಅಂಕೆ()[i - 1].m_ಮೌಲ್ಯ >> (32 - s)));
    }
    vn[0] = rhs.m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ << s;

    // Copy and shift dividend by same amount; may set new upper word
    ಯದಿ (s) {
        un[uw] = lhs.m_data.ಅಂಕೆ()[uw - 1].m_ಮೌಲ್ಯ >> (32 - s);
    } ಅನ್ಯಥಾ {
        un[uw] = 0;
    }
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ << s)
                | (shift_mask & (lhs.m_data.ಅಂಕೆ()[i - 1].m_ಮೌಲ್ಯ >> (32 - s)));
    }
    un[0] = lhs.m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ j = uw - vw; j >= 0; j--) {
        // Estimate
        ಧ್ರುವ uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        ಯದಿ (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            ಯದಿ (rhat < 0x100000000ULL) ಗಚ್ಛ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < vw; ++i) {
            ಧ್ರುವ uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ = qhat;  // Save quotient digit

        ಯದಿ (t < 0) {
            // Over subtracted; correct by adding back
            m_data.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ--;
            k = 0;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < vw; ++i) {
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

    ಯದಿ (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < vw; ++i) {
            m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = vw; i < l_ಪದಗಳು; ++i) m_data.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *ಏತತ್);
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    } ಅನ್ಯಥಾ {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *ಏತತ್);
        ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
    }
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPow(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ತಾರ್ಕಿಕ lsign, ತಾರ್ಕಿಕ rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಯದಿ (lhs.isFourState() || rhs.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಯದಿ (rhs.isEqZero()) ಪ್ರತ್ಯಾಗಮ setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    ಯದಿ (rsign && rhs.isNegative()) {
        ಯದಿ (lhs.isEqZero()) {
            ಪ್ರತ್ಯಾಗಮ setAllBitsXRemoved();
        } ಅನ್ಯಥಾ ಯದಿ (lhs.isEqOne()) {
            ಪ್ರತ್ಯಾಗಮ setValue1();
        } ಅನ್ಯಥಾ ಯದಿ (lsign && lhs.isEqAllOnes()) {
            ಯದಿ (rhs.bitIs1(0)) {
                ಪ್ರತ್ಯಾಗಮ setAllBits1();  // -1^odd=-1
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ setValue1();  // -1^even=1
            }
        }
        ಪ್ರತ್ಯಾಗಮ setZero();
    }
    ಯದಿ (lhs.isEqZero()) ಪ್ರತ್ಯಾಗಮ setZero();
    setZero();
    m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = 1;
    V3_ಸಂಖ್ಯೆ power(&lhs, width());
    power.opAssign(lhs);
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < rhs.width(); ++bit) {
        ಯದಿ (bit > 0) {  // power = power*power
            V3_ಸಂಖ್ಯೆ lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        ಯದಿ (rhs.bitIs1(bit)) {  // out *= power
            V3_ಸಂಖ್ಯೆ lastOut(&lhs, width());
            lastOut.opAssign(*ಏತತ್);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowSU(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಪ್ರತ್ಯಾಗಮ opPow(lhs, rhs, ಸತ್ಯ, ಅಸತ್ಯ);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowSS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಪ್ರತ್ಯಾಗಮ opPow(lhs, rhs, ಸತ್ಯ, ಸತ್ಯ);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowUS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಪ್ರತ್ಯಾಗಮ opPow(lhs, rhs, ಅಸತ್ಯ, ಸತ್ಯ);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBufIf1(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ens, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& if1s) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(ens, if1s);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit < width(); ++bit) {
        ಯದಿ (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } ಅನ್ಯಥಾ {
            setBit(bit, 'z');
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

// Sets all bits in range to the given value
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSetRange(uint32_t lsb, uint32_t width, ವರ್ಣ bit_ಮೌಲ್ಯ) {
    ಯದಿ (lsb < static_cast<uint32_t>(ಏತತ್->width())) {
        fillBits(static_cast<ಪೂರ್ಣಾಙ್ಕ>(lsb),
                 static_cast<ಪೂರ್ಣಾಙ್ಕ>(std::min(width, static_cast<uint32_t>(ಏತತ್->width()) - lsb)),
                 bit_ಮೌಲ್ಯ);
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAssign(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) { ಪ್ರತ್ಯಾಗಮ opAssignNonXZ(lhs, ಅಸತ್ಯ); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAssignNonXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ತಾರ್ಕಿಕ ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    ಯದಿ (ಏತತ್ != &lhs) {
        ಯದಿ (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (isString()) {
            ಯದಿ (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } ಅನ್ಯಥಾ {
                m_data.str() = lhs.m_data.str();
            }
        } ಅನ್ಯಥಾ ಯದಿ (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } ಅನ್ಯಥಾ ಯದಿ (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } ಅನ್ಯಥಾ ಯದಿ (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ word = 0; word < ಪದಗಳು(); ++word) {
                ಧ್ರುವ uint32_t ಮೌಲ್ಯ = word < lhs.ಪದಗಳು() ? lhs.m_data.ಅಂಕೆ()[word].m_ಮೌಲ್ಯ
                                                                & ~lhs.m_data.ಅಂಕೆ()[word].m_ಮೌಲ್ಯ_X
                                                          : 0;
                m_data.ಅಂಕೆ()[word] = {ಮೌಲ್ಯ, 0};
            }
            opCleanThis();
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNToI(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    // String to packed number
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    setZero();
    ಧ್ರುವ string& str = lhs.toString();
    ಪ್ರತಿ (size_t n = 0; n < str.length(); ++n) {
        ಧ್ರುವ ವರ್ಣ c = str[str.length() - 1 - n];
        ಪ್ರತಿ (size_t cbit = 0; cbit < 8; ++cbit) {
            ಧ್ರುವ std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opExtendS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ copyWidth = std::min(width(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opExtendXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ copyWidth = std::min(width(), static_cast<ಪೂರ್ಣಾಙ್ಕ>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opClean(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t bits) { ಪ್ರತ್ಯಾಗಮ opSel(lhs, bits - 1, 0); }

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::opCleanThis(ತಾರ್ಕಿಕ warnOnTruncation) {
    // Clean MSB of number
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(*ಏತತ್);
    ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[ಪದಗಳು() - 1];
    ಧ್ರುವ uint32_t new_ಮೌಲ್ಯ_Msb = v.m_ಮೌಲ್ಯ & hiWordMask();
    ಧ್ರುವ uint32_t new_ಮೌಲ್ಯ_XMsb = v.m_ಮೌಲ್ಯ_X & hiWordMask();
    ಯದಿ (warnOnTruncation && (new_ಮೌಲ್ಯ_Msb != v.m_ಮೌಲ್ಯ || new_ಮೌಲ್ಯ_XMsb != v.m_ಮೌಲ್ಯ_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.ಅಂಕೆ()[ಪದಗಳು() - 1] = {new_ಮೌಲ್ಯ_Msb, new_ಮೌಲ್ಯ_XMsb};
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSel(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& msb, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lsb) {
    ಅಂಕೆ_ASSERT_OP_ARGS3(lhs, msb, lsb);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(msb, lsb);
    ಯದಿ (lsb.isFourState() || msb.isFourState()) ಪ್ರತ್ಯಾಗಮ setAllBitsX();
    ಪ್ರತ್ಯಾಗಮ opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSel(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t msbval, uint32_t lsbval) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ಪೂರ್ಣಾಙ್ಕ copyWidth = 0;
    ಯದಿ (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        ಧ್ರುವ uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<ಪೂರ್ಣಾಙ್ಕ>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<ಪೂರ್ಣಾಙ್ಕ>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSelInto(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lsb, ಪೂರ್ಣಾಙ್ಕ width) {
    ಪ್ರತ್ಯಾಗಮ opSelInto(lhs, lsb.toSInt(), width);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSelInto(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಙ್ಕ lsbval, ಪೂರ್ಣಾಙ್ಕ width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ copyWidth = std::max(0, std::min({width, lhs.width(), ಏತತ್->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ fillWidth
        = std::max(0, std::min(width - copyWidth, ಏತತ್->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}

//======================================================================
// Ops - Floating point

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opIToRD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ತಾರ್ಕಿಕ isSigned) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_ಸಂಖ್ಯೆ noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    ದ್ವಿಗುಣ d = 0;
    ಧ್ರುವ ತಾರ್ಕಿಕ negate = isSigned && noxz.isNegative();
    ಯದಿ (negate) {
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        ಯದಿ (noxz.bitIs1(bit)) d += exp2(bit);
    }
    ಯದಿ (negate) d = -d;
    ಪ್ರತ್ಯಾಗಮ setDouble(d);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRToIS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    ಧ್ರುವ ದ್ವಿಗುಣ v = VL_TRUNC(lhs.toDouble());
    ಧ್ರುವ int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ಪ್ರತ್ಯಾಗಮ setLongS(i);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRToIRoundS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    ಧ್ರುವ ದ್ವಿಗುಣ v = VL_ROUND(lhs.toDouble());
    setZero();
    ಸಙ್ಘ {
        ದ್ವಿಗುಣ d;
        uint64_t q;
    } u;
    u.d = v;
    (ಶೂನ್ಯ)u.d;

    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ exp = static_cast<ಪೂರ್ಣಾಙ್ಕ>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = exp - 52;
    ಧ್ರುವ uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    ಯದಿ (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bit = 0; bit <= 52; ++bit) {
            ಯದಿ (mantissa & (1ULL << bit)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ outbit = bit + lsb;
                ಯದಿ (outbit >= 0) setBit(outbit, 1);
            }
        }
        ಯದಿ (v < 0) {
            ಧ್ರುವ V3_ಸಂಖ್ಯೆ noSign = *ಏತತ್;
            opNegate(noSign);
        }
    }
    ಪ್ರತ್ಯಾಗಮ *ಏತತ್;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRealToBits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    ಸಙ್ಘ {
        ದ್ವಿಗುಣ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    ಪ್ರತ್ಯಾಗಮ setQuad(u.m_v);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBitsToRealD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    ಸಙ್ಘ {
        ದ್ವಿಗುಣ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    ಪ್ರತ್ಯಾಗಮ setDouble(u.m_d);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNegateD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    ಪ್ರತ್ಯಾಗಮ setDouble(-lhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAddD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSubD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMulD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opDivD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opEqD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNeqD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGtD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGteD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLtD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLteD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opConcatN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setString(lhs.toString() + rhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opReplN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(rhs);
    ಪ್ರತ್ಯಾಗಮ opReplN(lhs, rhs.toUInt());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opReplN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t rhsval) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    ಪ್ರತಿ (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    ಪ್ರತ್ಯಾಗಮ setString(out);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCvtPackString(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಪ್ರತ್ಯಾಗಮ setString(lhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opToLowerN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಧ್ರುವ std::string out = VString::downcase(lhs.toString());
    ಪ್ರತ್ಯಾಗಮ setString(out);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opToUpperN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಧ್ರುವ std::string out = VString::upcase(lhs.toString());
    ಪ್ರತ್ಯಾಗಮ setString(out);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opEqN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toString() == rhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNeqN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toString() != rhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGtN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toString() > rhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGteN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toString() >= rhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLtN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toString() < rhs.toString());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLteN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಪ್ರತ್ಯಾಗಮ setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_ಸಾಲು* ಧ್ರುವ fl = ನೂತನ File_ಸಾಲು{File_ಸಾಲು::builtInFilename()};
    V3_ಸಂಖ್ಯೆ ಅಂಕೆ{fl, 32, 0};
    ಅಂಕೆ.selfTestThis();
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(0, 0), ಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(1e19, 1e19), ಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(9, 0.0001), ಅಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(1, 1 + std::numeric_limits<ದ್ವಿಗುಣ>::epsilon()), ಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(0.009, 0.00899999999999999931998839741709), ಸತ್ಯ);

    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(0), ಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(1), ಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(-1), ಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(1.0001), ಅಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(0.9999), ಅಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(-1.0001), ಅಸತ್ಯ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(-0.9999), ಅಸತ್ಯ);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
