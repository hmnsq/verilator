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

constexpr ಪೂರ್ಣಾಂಕ MAX_SPRINTF_DOUBLE_ಗಾತ್ರ
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_OP_ARGS1(arg1) \
    UASSERT((ಇದು != &(arg1)), "Number operation called with same source and dest")
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((ಇದು != &(arg1) && ಇದು != &(arg2)), \
            "Number operation called with same source and dest")
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((ಇದು != &(arg1) && ಇದು != &(arg2) && ಇದು != &(arg3)), \
            "Number operation called with same source and dest")
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((ಇದು != &(arg1) && ಇದು != &(arg2) && ಇದು != &(arg3) && ಇದು != &(arg4)), \
            "Number operation called with same source and dest")

#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).ದತ್ತಾಂಶ_Type() == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ::V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg2)

#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg2); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg3); \
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(arg4)

#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_STRING_ARGS2(arg1, arg2) \
    ಅಂಕೆ_ASSERT_STRING_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_STRING_ARGS1(arg2)

#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#ವ್ಯಾಖ್ಯಾನಿಸು ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(arg1); \
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ಆದರೆ (m_nodep) {
        m_nodep->v3error_ಕೊನೆ(nsstr);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_fileline) {
        m_fileline->v3error_ಕೊನೆ(nsstr);
    } ಇಲ್ಲದಿದ್ದರೆ {
        V3Error::v3error_ಕೊನೆ(nsstr, "", ಶೂನ್ಯಸೂಚಕ);
    }
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::v3error_ಕೊನೆ_Fatal(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_ಕೊನೆ(str);
    ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_ಸಂಖ್ಯೆ::V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, VerilogStringLiteral, ಸ್ಥಿರಾಂಕ string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    ಆದರೆ (str.ಖಾಲಿಗಳು()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } ಇಲ್ಲದಿದ್ದರೆ {
        init(nodep, std::max<ಪೂರ್ಣಾಂಕ>(str.ಉದ್ದಳತೆ() * 8, 1));
        ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ pos = 0; pos < str.ಉದ್ದಳತೆ(); ++pos) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ topos = str.ಉದ್ದಳತೆ() - 1 - pos;
            ಮೌಲ್ಯ_And_X& v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[topos / 4];
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < 8; ++ಬಿಟ್_2) {
                ಆದರೆ (str[pos] & (1UL << ಬಿಟ್_2)) v.m_ಮೌಲ್ಯ |= (1UL << (ಬಿಟ್_2 + (topos % 4) * 8));
            }
        }
    }
    m_ದತ್ತಾಂಶ.m_fromString = ನಿಜ;
    opCleanThis(ನಿಜ);
}

V3_ಸಂಖ್ಯೆ::V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* nodedtypep) {
    ಆದರೆ (nodedtypep->isCHandle()) {
        init(nodep);
        ಗಣ_Null();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodedtypep->isDouble()) {
        init(nodep, 64);
        ಗಣ_Double(0.0);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodedtypep->isString()) {
        init(nodep);
        ಗಣ_String("");
    } ಇಲ್ಲದಿದ್ದರೆ {
        init(nodep, nodedtypep->ಅಗಲ(), nodedtypep->ಅಗಲ_Sized());
    }
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::create(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep) {
    m_ದತ್ತಾಂಶ.ಗಣ_Logic();
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಮೌಲ್ಯ_startp = sourcep;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = sourcep; *cp; ++cp) {
        ಆದರೆ (*cp == '\'') {
            ಮೌಲ್ಯ_startp = cp + 1;
            ನಿಲ್ಲಿಸು;
        }
    }

    ತಾರ್ಕಿಕ userSized = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ unbased = ಸುಳ್ಳು;
    ಅಕ್ಷರ ತಳ = '\0';
    ಆದರೆ (ಮೌಲ್ಯ_startp != sourcep) {  // Has a '
        string widthn;
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = sourcep;
        ಗಾಗಿ (; *cp; ++cp) {
            ಆದರೆ (*cp == '\'') {
                ++cp;
                ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (*cp != '_') widthn += *cp;
        }
        ಆಗ (*cp == '_') cp++;
        ಆದರೆ (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(ನಿಜ);
        }
        ಆದರೆ (*cp) {
            ತಳ = *cp;
            ++cp;
        }
        ಮೌಲ್ಯ_startp = cp;

        userSized = widthn.ಉದ್ದಳತೆ() != 0;
        ಆದರೆ (std::atoi(widthn.c_str())) {
            ಆದರೆ (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_ಅಂಕೆ_ಅಗಲ()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                ಅಗಲ(v3Global.opt.max_ಅಂಕೆ_ಅಗಲ(), ನಿಜ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಅಗಲ(std::atoi(widthn.c_str()), ನಿಜ);
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        unbased = ನಿಜ;
        ತಳ = 'd';
    }

    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    ಆದರೆ (ತಳ == '0') {
        ಅಗಲ(1, ಸುಳ್ಳು);  // So we extend it
        ಗಣ_ಬಿಟ್(0, 0);
        m_ದತ್ತಾಂಶ.m_autoExtend = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ == '1') {
        ಅಗಲ(1, ಸುಳ್ಳು);  // So we extend it
        ಗಣ_ಬಿಟ್(0, 1);
        m_ದತ್ತಾಂಶ.m_autoExtend = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::tolower(ತಳ) == 'z') {
        ಅಗಲ(1, ಸುಳ್ಳು);  // So we extend it
        ಗಣ_ಬಿಟ್(0, 'z');
        m_ದತ್ತಾಂಶ.m_autoExtend = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::tolower(ತಳ) == 'x') {
        ಅಗಲ(1, ಸುಳ್ಳು);  // So we extend it
        ಗಣ_ಬಿಟ್(0, 'x');
        m_ದತ್ತಾಂಶ.m_autoExtend = ನಿಜ;
    }
    // Otherwise...
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Bound = std::max<ಪೂರ್ಣಾಂಕ>(32, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(ಮೌಲ್ಯ_startp) * 4);
        ಅಗಲ(ಅಗಲ_Bound, ಸುಳ್ಳು);  // Will change width below
        ಆದರೆ (unbased) isSigned(ನಿಜ);  // Also says the spec.
    }

    // Ignore leading blanks
    ಆಗ (*ಮೌಲ್ಯ_startp == '_' || std::ಖಾಲಿಯೇ(*ಮೌಲ್ಯ_startp)) ++ಮೌಲ್ಯ_startp;
    ಆದರೆ (!*ಮೌಲ್ಯ_startp && !m_ದತ್ತಾಂಶ.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    ಆದರೆ (userSized && m_ದತ್ತಾಂಶ.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    ಪೂರ್ಣಾಂಕ obit = 0;  // Start at LSB
    ಪೂರ್ಣಾಂಕ ತಳ_align = 1;
    ಆದರೆ (std::tolower(ತಳ) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        ಆಗ (*ಮೌಲ್ಯ_startp == '_' || *ಮೌಲ್ಯ_startp == '0') ++ಮೌಲ್ಯ_startp;
        // Convert decimal number to hex
        ಪೂರ್ಣಾಂಕ olen = 0;
        uint32_t val = 0;
        ಪೂರ್ಣಾಂಕ got_x = 0;
        ಪೂರ್ಣಾಂಕ got_z = 0;
        ಪೂರ್ಣಾಂಕ got_01 = 0;
        ತಾರ್ಕಿಕ warnedTooMany = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = ಮೌಲ್ಯ_startp; *cp; ++cp) {
            ಆಯ್ಕೆ (std::tolower(*cp)) {
            ಸ್ಥಿತಿ '0':  // FALLTHRU
            ಸ್ಥಿತಿ '1':  // FALLTHRU
            ಸ್ಥಿತಿ '2':  // FALLTHRU
            ಸ್ಥಿತಿ '3':  // FALLTHRU
            ಸ್ಥಿತಿ '4':  // FALLTHRU
            ಸ್ಥಿತಿ '5':  // FALLTHRU
            ಸ್ಥಿತಿ '6':  // FALLTHRU
            ಸ್ಥಿತಿ '7':  // FALLTHRU
            ಸ್ಥಿತಿ '8':  // FALLTHRU
            ಸ್ಥಿತಿ '9': {
                ಆದರೆ (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = val;
                } ಇಲ್ಲದಿದ್ದರೆ {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_ಸಂಖ್ಯೆ ಗುಣಲಬ್ಧ(ಇದು, ಅಗಲ() + 4);  // +4 for overflow detection
                    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ ten(ಇದು, ಅಗಲ() + 4, 10);
                    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ addend(ಇದು, ಅಗಲ(), (*cp - '0'));
                    ಗುಣಲಬ್ಧ.opMul(*ಇದು, ten);
                    op_ಕೂಡಿಸು(ಗುಣಲಬ್ಧ, addend);
                    ಆದರೆ (!warnedTooMany && ಗುಣಲಬ್ಧ.ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(ಅಗಲ(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = ನಿಜ;
                    }
                }
                ++olen;
                got_01 = 1;
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ 'z':
            ಸ್ಥಿತಿ '?': {
                got_z = 1;
                ಆದರೆ (!userSized) ಅಗಲ(32, ಸುಳ್ಳು);
                ಗಣ_All_ಬಿಟ್ಟುಗಳು_Z();
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ 'x': {
                got_x = 1;
                ಆದರೆ (!userSized) ಅಗಲ(32, ಸುಳ್ಳು);
                ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ '_': ನಿಲ್ಲಿಸು;
            ಪೂರ್ವನಿಯೋಜಿತ: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                ನಿಲ್ಲಿಸು;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = ಅಗಲ();
        ಆದರೆ ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << ಮೌಲ್ಯ_startp);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Convert bin/octal number to hex
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = ಮೌಲ್ಯ_startp + std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(ಮೌಲ್ಯ_startp) - 1; cp >= ಮೌಲ್ಯ_startp;
             cp--) {
            ಆದರೆ (*cp != '_' && *cp != '0' && obit >= ಅಗಲ()) {
                warnTooMany(sourcep);
                ನಿಲ್ಲಿಸು;
            }
            ಆಯ್ಕೆ (std::tolower(ತಳ)) {
            ಸ್ಥಿತಿ 'b': {
                ತಳ_align = 1;
                ಆಯ್ಕೆ (std::tolower(*cp)) {
                ಸ್ಥಿತಿ '0': ಗಣ_ಬಿಟ್(obit++, 0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '1': ಗಣ_ಬಿಟ್(obit++, 1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'z':
                ಸ್ಥಿತಿ '?': ಗಣ_ಬಿಟ್(obit++, 'z'); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'x': ಗಣ_ಬಿಟ್(obit++, 'x'); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '_': ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ: v3error("Illegal character in binary constant: " << *cp);
                }
                ನಿಲ್ಲಿಸು;
            }

            ಸ್ಥಿತಿ 'o':
            ಸ್ಥಿತಿ 'c': {
                ತಳ_align = 3;
                ಆಯ್ಕೆ (std::tolower(*cp)) {  // clang-format off
                ಸ್ಥಿತಿ '0': ಗಣ_ಬಿಟ್(obit++, 0); ಗಣ_ಬಿಟ್(obit++, 0);  ಗಣ_ಬಿಟ್(obit++, 0);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '1': ಗಣ_ಬಿಟ್(obit++, 1); ಗಣ_ಬಿಟ್(obit++, 0);  ಗಣ_ಬಿಟ್(obit++, 0);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '2': ಗಣ_ಬಿಟ್(obit++, 0); ಗಣ_ಬಿಟ್(obit++, 1);  ಗಣ_ಬಿಟ್(obit++, 0);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '3': ಗಣ_ಬಿಟ್(obit++, 1); ಗಣ_ಬಿಟ್(obit++, 1);  ಗಣ_ಬಿಟ್(obit++, 0);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '4': ಗಣ_ಬಿಟ್(obit++, 0); ಗಣ_ಬಿಟ್(obit++, 0);  ಗಣ_ಬಿಟ್(obit++, 1);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '5': ಗಣ_ಬಿಟ್(obit++, 1); ಗಣ_ಬಿಟ್(obit++, 0);  ಗಣ_ಬಿಟ್(obit++, 1);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '6': ಗಣ_ಬಿಟ್(obit++, 0); ಗಣ_ಬಿಟ್(obit++, 1);  ಗಣ_ಬಿಟ್(obit++, 1);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '7': ಗಣ_ಬಿಟ್(obit++, 1); ಗಣ_ಬಿಟ್(obit++, 1);  ಗಣ_ಬಿಟ್(obit++, 1);  ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'z':
                ಸ್ಥಿತಿ '?': ಗಣ_ಬಿಟ್(obit++, 'z'); ಗಣ_ಬಿಟ್(obit++, 'z'); ಗಣ_ಬಿಟ್(obit++, 'z'); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'x': ಗಣ_ಬಿಟ್(obit++, 'x'); ಗಣ_ಬಿಟ್(obit++, 'x'); ಗಣ_ಬಿಟ್(obit++, 'x'); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '_': ನಿಲ್ಲಿಸು;
                // clang-format on
                ಪೂರ್ವನಿಯೋಜಿತ: v3error("Illegal character in octal constant");
                }
                ನಿಲ್ಲಿಸು;
            }

            ಸ್ಥಿತಿ 'h':  // FALLTHRU
            ಸ್ಥಿತಿ 'x': {
                ತಳ_align = 4;
                ಆಯ್ಕೆ (std::tolower(*cp)) {  // clang-format off
                ಸ್ಥಿತಿ '0': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '1': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '2': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '3': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '4': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '5': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '6': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '7': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '8': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ '9': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'a': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'b': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'c': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'd': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'e': ಗಣ_ಬಿಟ್(obit++,0); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'f': ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ಗಣ_ಬಿಟ್(obit++,1); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'z':
                ಸ್ಥಿತಿ '?': ಗಣ_ಬಿಟ್(obit++,'z'); ಗಣ_ಬಿಟ್(obit++,'z'); ಗಣ_ಬಿಟ್(obit++,'z'); ಗಣ_ಬಿಟ್(obit++,'z'); ನಿಲ್ಲಿಸು;
                ಸ್ಥಿತಿ 'x': ಗಣ_ಬಿಟ್(obit++,'x'); ಗಣ_ಬಿಟ್(obit++,'x'); ಗಣ_ಬಿಟ್(obit++,'x'); ಗಣ_ಬಿಟ್(obit++,'x'); ನಿಲ್ಲಿಸು;
                    // clang-format on
                ಸ್ಥಿತಿ '_': ನಿಲ್ಲಿಸು;
                ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    ನಿಲ್ಲಿಸು;
                }  // LCOV_EXCL_STOP
                ನಿಲ್ಲಿಸು;
            }
            ಪೂರ್ವನಿಯೋಜಿತ:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << ತಳ);
                ನಿಲ್ಲಿಸು;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    ಆದರೆ (!userSized && !m_ದತ್ತಾಂಶ.m_autoExtend) {
        ಅಗಲ(std::max(32, ತಳ_align * ((ಅಗಲ_ಗಮ್ಯ_Fit() + ತಳ_align - 1) / ತಳ_align)), ಸುಳ್ಳು);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    ಆಗ (obit <= ಅಗಲ() && obit && ಬಿಟ್_Is_XZ(obit - 1)) {
        ಗಣ_ಬಿಟ್(obit, ಬಿಟ್_Is(obit - 1));
        ++obit;
    }
    opCleanThis(ನಿಜ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::warnTooMany(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {
    v3warn(WIDTHDIGITS, "Too many digits for " << ಅಗಲ() << " bit number: '" << ಮೌಲ್ಯ << "'");
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::nodep(Ast_ಗಂಟು* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::epsilonEqual(ದ್ವಿಗುಣ a, ದ್ವಿಗುಣ b) {
    ಹಿಂತಿರುಗಿ std::fabs(a - b)
           <= (std::numeric_limits<ದ್ವಿಗುಣ>::epsilon() * std::max(1.0, std::max(a, b)));
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::epsilonIntegral(ದ್ವಿಗುಣ a) {
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ dint = static_cast<ದ್ವಿಗುಣ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ಹಿಂತಿರುಗಿ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

ಪೂರ್ಣಾಂಕ V3_ಸಂಖ್ಯೆ::log2b(uint32_t ಅಂಕೆ) {
    // See also opCLog2
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 31; ಬಿಟ್_2 > 0; ಬಿಟ್_2--) {
        ಆದರೆ (ಅಂಕೆ & (1ULL << ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಬಿಟ್_2;
    }
    ಹಿಂತಿರುಗಿ 0;
}

ಪೂರ್ಣಾಂಕ V3_ಸಂಖ್ಯೆ::log2bQuad(uint64_t ಅಂಕೆ) {
    ಆದರೆ (ಅಂಕೆ >> 32ULL) ಹಿಂತಿರುಗಿ 32 + log2b(ಅಂಕೆ >> 32ULL);
    ಹಿಂತಿರುಗಿ log2b(ಅಂಕೆ);
}

//======================================================================
// Setters

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Zero() {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, 0};
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Quad(uint64_t ಮೌಲ್ಯ) {
    ಗಣ_Zero();
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ & 0xffffffffULL;
    ಆದರೆ (ಅಗಲ() > 32) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ = (ಮೌಲ್ಯ >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Long(uint32_t ಮೌಲ್ಯ) {
    ಗಣ_Zero();
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Long_S(int32_t ಮೌಲ್ಯ) {
    ಗಣ_Zero();
    ಒಕ್ಕೂಟ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = ಮೌಲ್ಯ;
    (ಶೂನ್ಯ)u.s;
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = u.u;
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Double(ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
    UASSERT(ಅಗಲ() == 64, "Real operation on wrong sized number");
    m_ದತ್ತಾಂಶ.ಗಣ_Double();
    ಒಕ್ಕೂಟ {
        ದ್ವಿಗುಣ d;
        uint32_t u[2];
    } u;
    u.d = ಮೌಲ್ಯ;
    (ಶೂನ್ಯ)u.d;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 2; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, 0};
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = u.u[0];
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ = u.u[1];
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Single_ಬಿಟ್ಟುಗಳು(ಅಕ್ಷರ ಮೌಲ್ಯ) {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1 /*ಊರ್ಧ್ವ*/; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, 0};
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0] = {(ಮೌಲ್ಯ == '1' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 1 || ಮೌಲ್ಯ == 3),
                       (ಮೌಲ್ಯ == 'z' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 2 || ಮೌಲ್ಯ == 3)};
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_All_Bits0() {
    ಗಣ_Zero();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_All_Bits1() {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {~0U, 0};
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_All_ಬಿಟ್ಟುಗಳು_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {~0U, ~0U};
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_All_ಬಿಟ್ಟುಗಳು_Z() {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, ~0U};
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved() {
    ಆದರೆ (!v3Global.constRemoveXs()) {
        ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    } ಇಲ್ಲದಿದ್ದರೆ {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        ಆದರೆ (v3Global.opt.x_ನಿಯೋಜಿಸು() == "1") {
            ಹಿಂತಿರುಗಿ ಗಣ_All_Bits1();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ ಗಣ_All_Bits0();
        }
    }
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_Value1() {
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0] = {1, 0};
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, 0};
    ಹಿಂತಿರುಗಿ *ಇದು;
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::ಗಣ_ಬಿಟ್_X0(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    ಗಣ_ಬಿಟ್(ಬಿಟ್_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::ಗಣ_ಮುಖವಾಡ(ಪೂರ್ಣಾಂಕ nbits, ಪೂರ್ಣಾಂಕ lsb) {
    ಗಣ_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_ಬಿಟ್ಟುಗಳು(lsb, std::max(0, std::min(nbits, ಅಗಲ() - lsb)), 1);
    ಹಿಂತಿರುಗಿ *ಇದು;
}

//======================================================================
// ACCESSORS - as strings

string V3_ಸಂಖ್ಯೆ::ascii(ತಾರ್ಕಿಕ prefixed, ತಾರ್ಕಿಕ cleanVerilog) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ಆದರೆ (is1_ಹೆಜ್ಜೆ()) ಹಿಂತಿರುಗಿ "1step";
    ಆದರೆ (isNull()) ಹಿಂತಿರುಗಿ "null";
    std::ostringstream ಹೊರಹೋಗು;
    ಆದರೆ (isDouble()) {
        ಹೊರಹೋಗು.precision(17);
        ಆದರೆ (VL_UNCOVERABLE(ಅಗಲ() != 64)) {
            ಹೊರಹೋಗು << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹೊರಹೋಗು << ಗಮ್ಯ_Double();
            ಆದರೆ (ಗಮ್ಯ_Double() == floor(ಗಮ್ಯ_Double())) ಹೊರಹೋಗು << ".0";
        }
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isString()) {
        ಹಿಂತಿರುಗಿ '"' + ಗಮ್ಯ_String() + '"';
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (VL_UNCOVERABLE((m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಪದಗಳು() - 1].m_ಮೌಲ್ಯ | m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಪದಗಳು() - 1].m_ಮೌಲ್ಯ_X)
                           & ~hi_ಶಬ್ದ_ಮುಖವಾಡ())) {
            ಹೊರಹೋಗು << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    ಆದರೆ (prefixed) {
        ಆದರೆ (sized()) {
            ಹೊರಹೋಗು << ಅಗಲ() << "'";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (autoExtend() && ಅಗಲ() == 1) {
            ಹೊರಹೋಗು << "'";
            ಆದರೆ (ಬಿಟ್_Is0(0)) {
                ಹೊರಹೋಗು << '0';
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಟ್_Is1(0)) {
                ಹೊರಹೋಗು << '1';
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಟ್_Is_Z(0)) {
                ಹೊರಹೋಗು << 'z';
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೊರಹೋಗು << 'x';
            }
            ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (cleanVerilog) {
                ಹೊರಹೋಗು << "'";
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹೊರಹೋಗು << "?" << ಅಗಲ() << "?";
            }
        }
        ಆದರೆ (isSigned()) ಹೊರಹೋಗು << "s";
    }

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ದ್ವಿಮಾನ = (isFourState()
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ V3_ಸಂಖ್ಯೆ_ASCII_ದ್ವಿಮಾನ
                         // cppcheck-suppress konwnConditionTrueFalse
                         || ನಿಜ
#ಆದರೆ_ಅಂತ್ಯ
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    ಆದರೆ (ದ್ವಿಮಾನ) {
        ಹೊರಹೋಗು << "b";
        ಹೊರಹೋಗು << displayed("%0b");
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (prefixed) ಹೊರಹೋಗು << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        ಹೊರಹೋಗು << displayed("%0h");
    }
    ಆದರೆ (isNull()) {
        ಆದರೆ (VL_UNCOVERABLE(!isEqZero())) {
            ಹೊರಹೋಗು << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹೊರಹೋಗು << " [null]";
        }
    }
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::displayedFmtHasArg(ಅಕ್ಷರ format, ತಾರ್ಕಿಕ is_ಪರಿಶೀಲನೆ) {
    // Is this a format letter that takes an argument in runtime?
    (ಶೂನ್ಯ)is_ಪರಿಶೀಲನೆ;
    ಆಯ್ಕೆ (std::tolower(format)) {
    ಸ್ಥಿತಿ 'b': ಹಿಂತಿರುಗಿ ನಿಜ;  //ದ್ವಿಮಾನ
    ಸ್ಥಿತಿ 'c': ಹಿಂತಿರುಗಿ ನಿಜ;  // Character
    ಸ್ಥಿತಿ 'd': ಹಿಂತಿರುಗಿ ನಿಜ;  // Decimal
    ಸ್ಥಿತಿ 'e': ಹಿಂತಿರುಗಿ ನಿಜ;  // Floating
    ಸ್ಥಿತಿ 'f': ಹಿಂತಿರುಗಿ ನಿಜ;  // Floating
    ಸ್ಥಿತಿ 'g': ಹಿಂತಿರುಗಿ ನಿಜ;  // Floating
    ಸ್ಥಿತಿ 'h': ಹಿಂತಿರುಗಿ ನಿಜ;  // Hex
    ಸ್ಥಿತಿ 'o': ಹಿಂತಿರುಗಿ ನಿಜ;  // Octal
    ಸ್ಥಿತಿ 'p': ಹಿಂತಿರುಗಿ ನಿಜ;  //ನಮೂನೆ
    ಸ್ಥಿತಿ 's': ಹಿಂತಿರುಗಿ ನಿಜ;  // String
    ಸ್ಥಿತಿ 't': ಹಿಂತಿರುಗಿ ನಿಜ;  //ಸಮಯ
    ಸ್ಥಿತಿ 'u': ಹಿಂತಿರುಗಿ ನಿಜ;  // Packed 2-state
    ಸ್ಥಿತಿ 'v': ಹಿಂತಿರುಗಿ ನಿಜ;  // Strength
    ಸ್ಥಿತಿ 'x': ಹಿಂತಿರುಗಿ ನಿಜ;  // Hex
    ಸ್ಥಿತಿ 'z': ಹಿಂತಿರುಗಿ ನಿಜ;  // Packed 4-state
    ಸ್ಥಿತಿ '?': ಹಿಂತಿರುಗಿ ನಿಜ;  // Internal: Compute format in V3Width based on data type
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_ಸಂಖ್ಯೆ::displayPad(ಗಾತ್ರ_t fmtsize, ಅಕ್ಷರ pad, ತಾರ್ಕಿಕ ಎಡ, ಸ್ಥಿರಾಂಕ string& ಒಳ) VL_ಶುದ್ಧ {
    string padding;
    ಆದರೆ (ಒಳ.ಉದ್ದಳತೆ() < fmtsize) padding = string(fmtsize - ಒಳ.ಉದ್ದಳತೆ(), pad);
    ಹಿಂತಿರುಗಿ ಎಡ ? (ಒಳ + padding) : (padding + ಒಳ);
}

string V3_ಸಂಖ್ಯೆ::displayedEnumName(ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp) {
    ಸ್ಥಿರಾಂಕ string name = VIdProtect::protect(itemp->prettyName());
    ಹಿಂತಿರುಗಿ VString::isIdentifier(name) && !std::ಅಂಕಿಯೇ(static_cast<ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_ಸಂಖ್ಯೆ::displayedEnumName(ಸ್ಥಿರಾಂಕ AstEnumDType* dtypep) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{ಇದು, dtypep->ಅಗಲ()};
    ಮೌಲ್ಯ.op_ನಿಯೋಜಿಸು(*ಇದು);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_ಅಂಶ)) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_AS(itemp->valuep(), Const);
        ಆದರೆ (!constp->ಅಂಕೆ().isAnyXZ() && ಮೌಲ್ಯ.isCaseEq(constp->ಅಂಕೆ()))
            ಹಿಂತಿರುಗಿ displayedEnumName(itemp);
    }
    ಹಿಂತಿರುಗಿ "";
}

string V3_ಸಂಖ್ಯೆ::displayedEnum(ಸ್ಥಿರಾಂಕ AstSFormatArg* argp,
                               ಸ್ಥಿರಾಂಕ string& vformat) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ dtypep = VN_AS(argp->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
    V3_ಸಂಖ್ಯೆ ಮೌಲ್ಯ{ಇದು, dtypep->ಅಗಲ()};
    ಮೌಲ್ಯ.op_ನಿಯೋಜಿಸು(*ಇದು);
    ಸ್ಥಿರಾಂಕ VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ fmt = std::tolower(vformat.ಹಿಂಭಾಗ());
    ಆದರೆ (fmt != 'p' && fmt != 's') ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ.displayed(argp, vformat, numericAttr);
    string name = ಮೌಲ್ಯ.displayedEnumName(dtypep);
    ಆದರೆ (name.ಖಾಲಿಗಳು()) name = ಮೌಲ್ಯ.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_ಸಂಖ್ಯೆ name_ಅಂಕೆ{ಇದು};
    name_ಅಂಕೆ.ಗಣ_String(name);
    ಹಿಂತಿರುಗಿ name_ಅಂಕೆ.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_ಸಂಖ್ಯೆ::displayed(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& vformat,
                           ಸ್ಥಿರಾಂಕ VFormatAttr& formatAttr) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಹಿಂತಿರುಗಿ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_ಸಂಖ್ಯೆ::displayed(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& vformat,
                           ಸ್ಥಿರಾಂಕ VFormatAttr& formatAttr) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    ಸ್ವಯಂಚಾಲಿತ pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *ಇದು);
    ++pos;
    ತಾರ್ಕಿಕ ಎಡ = ಸುಳ್ಳು;
    ಆದರೆ (pos[0] == '-') {
        ಎಡ = ನಿಜ;
        ++pos;
    }
    string fmtsize;
    ಗಾಗಿ (; pos != vformat.cend() && (std::ಅಂಕಿಯೇ(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    ಅಕ್ಷರ fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    ಆದರೆ (formatAttr.isComplex()) {
        ಆದರೆ (fmt != 'p') fmt = 's';  // override
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (formatAttr.isString()) {
        ಆದರೆ (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_ಸಂಖ್ಯೆ strwide{fl, static_cast<ಪೂರ್ಣಾಂಕ>(ಗಮ್ಯ_String().ಉದ್ದಳತೆ()) * 8, 0};
            strwide.opNToI(*ಇದು);
            ಹಿಂತಿರುಗಿ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        ಆದರೆ (fmt != 'p') fmt = 's';  // override
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        ಆದರೆ (fmt == 'p') {
            ಆದರೆ (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } ಇಲ್ಲದಿದ್ದರೆ {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        ಆದರೆ (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_ಸಂಖ್ಯೆ nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*ಇದು);
            ಹಿಂತಿರುಗಿ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    ಆಯ್ಕೆ (fmt) {
    ಸ್ಥಿತಿ 'c': {
        ಆದರೆ (ಅಗಲ() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಪೂರ್ಣಾಂಕ v = ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(0, 8);
        str.ತಳ್ಳು_ಹಿಂಭಾಗ(static_cast<ಅಕ್ಷರ>(v));
        ಹಿಂತಿರುಗಿ str;
    }
    ಸ್ಥಿತಿ 'e':  // FALLTHRU
    ಸ್ಥಿತಿ 'f':  // FALLTHRU
    ಸ್ಥಿತಿ 'g': {
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ n = formatAttr.isDouble()   ? ಗಮ್ಯ_Double()
                         : formatAttr.isSigned() ? ಗಮ್ಯ_SQuad()
                                                 : ಗಮ್ಯ_UQuad();
        ಅಕ್ಷರ tmp[MAX_SPRINTF_DOUBLE_ಗಾತ್ರ];
        (ಶೂನ್ಯ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_ಗಾತ್ರ, vformat.c_str(), n);
        ಹಿಂತಿರುಗಿ tmp;
    }
    ಸ್ಥಿತಿ 's': {
        ಆದರೆ (formatAttr.isString() || formatAttr.isComplex()) {
            str = ಗಮ್ಯ_String();
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ() - 1;
            ತಾರ್ಕಿಕ ಆರಂಭ = ನಿಜ;
            ಆಗ ((ಬಿಟ್_2 % 8) != 7) ++ಬಿಟ್_2;
            ಗಾಗಿ (; ಬಿಟ್_2 >= 0; ಬಿಟ್_2 -= 8) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ v = ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(ಬಿಟ್_2 - 7, 8);
                ಆದರೆ (!ಆರಂಭ || v) {
                    str += static_cast<ಅಕ್ಷರ>((v == 0) ? ' ' : v);
                    ಆರಂಭ = ಸುಳ್ಳು;  // Drop leading 0s
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಆದರೆ (fmtsize != "0") str += ' ';
                }
            }
        }
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t fmtsizen = static_cast<ಗಾತ್ರ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ಎಡ, str);
        ಹಿಂತಿರುಗಿ str;
    }
    ಸ್ಥಿತಿ 'p': {  //ನಮೂನೆ
        // 'p' with NUMBER was earlier converted to 'd'
        ಆದರೆ (formatAttr.isDouble()) {
            ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ n = formatAttr.isDouble()   ? ಗಮ್ಯ_Double()
                             : formatAttr.isSigned() ? ಗಮ್ಯ_SQuad()
                                                     : ಗಮ್ಯ_UQuad();
            ಅಕ್ಷರ tmp[MAX_SPRINTF_DOUBLE_ಗಾತ್ರ];
            (ಶೂನ್ಯ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_ಗಾತ್ರ, "%g", n);
            ಹಿಂತಿರುಗಿ tmp;
        }
        ಆದರೆ (formatAttr.isString())
            ಹಿಂತಿರುಗಿ '"' + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(ಗಮ್ಯ_String()) + '"';
        ಆದರೆ (formatAttr.isComplex()) ಹಿಂತಿರುಗಿ ಗಮ್ಯ_String();
        ಹಿಂತಿರುಗಿ "%p";
    }
    ಸ್ಥಿತಿ 'b':  // FALLTHRU
    ಸ್ಥಿತಿ 'o':  // FALLTHRU
    ಸ್ಥಿತಿ 'h':  // FALLTHRU
    ಸ್ಥಿತಿ 'x': {
        ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ() - 1;
        ಆದರೆ (ಎಡ || !fmtsize.ಖಾಲಿಗಳು()) {
            ಆಗ (ಬಿಟ್_2 && ಬಿಟ್_Is0(ಬಿಟ್_2)) --ಬಿಟ್_2;
        }
        ಆಯ್ಕೆ (fmt) {
        ಸ್ಥಿತಿ 'b': {
            ಗಾಗಿ (; ಬಿಟ್_2 >= 0; --ಬಿಟ್_2) {
                ಆದರೆ (ಬಿಟ್_Is0(ಬಿಟ್_2)) {
                    str += '0';
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಟ್_Is1(ಬಿಟ್_2)) {
                    str += '1';
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಟ್_Is_Z(ಬಿಟ್_2)) {
                    str += 'z';
                } ಇಲ್ಲದಿದ್ದರೆ {
                    str += 'x';
                }
            }
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 'o': {
            ಆಗ ((ಬಿಟ್_2 % 3) != 2) ++ಬಿಟ್_2;
            ಗಾಗಿ (; ಬಿಟ್_2 > 0; ಬಿಟ್_2 -= 3) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_X = ಎಣಿಕೆ_X(ಬಿಟ್_2 - 2, 3);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_Z = ಎಣಿಕೆ_Z(ಬಿಟ್_2 - 2, 3);
                ಆದರೆ (ಅಂಕೆ_X == 3 || ಅಂಕೆ_X == ಅಗಲ() - (ಬಿಟ್_2 - 2)) {
                    str += 'x';
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಅಂಕೆ_Z == 3 || ಅಂಕೆ_Z == ಅಗಲ() - (ಬಿಟ್_2 - 2)) {
                    str += 'z';
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಅಂಕೆ_X > 0) {
                    str += 'X';
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಅಂಕೆ_Z > 0) {
                    str += 'Z';
                    ಮುಂದುವರಿ;
                }
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ v = ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(ಬಿಟ್_2 - 2, 3);
                str += static_cast<ಅಕ್ಷರ>('0' + v);
            }
            ನಿಲ್ಲಿಸು;
        }
        ಪೂರ್ವನಿಯೋಜಿತ: {  // h/x
            ಆಗ ((ಬಿಟ್_2 % 4) != 3) ++ಬಿಟ್_2;
            ಗಾಗಿ (; ಬಿಟ್_2 > 0; ಬಿಟ್_2 -= 4) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_X = ಎಣಿಕೆ_X(ಬಿಟ್_2 - 3, 4);
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_Z = ಎಣಿಕೆ_Z(ಬಿಟ್_2 - 3, 4);
                ಆದರೆ (ಅಂಕೆ_X == 4 || ಅಂಕೆ_X == ಅಗಲ() - (ಬಿಟ್_2 - 3)) {
                    str += 'x';
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಅಂಕೆ_Z == 4 || ಅಂಕೆ_Z == ಅಗಲ() - (ಬಿಟ್_2 - 3)) {
                    str += 'z';
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಅಂಕೆ_X > 0) {
                    str += 'X';
                    ಮುಂದುವರಿ;
                }
                ಆದರೆ (ಅಂಕೆ_Z > 0) {
                    str += 'Z';
                    ಮುಂದುವರಿ;
                }
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ v = ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(ಬಿಟ್_2 - 3, 4);
                ಆದರೆ (v >= 10) {
                    str += static_cast<ಅಕ್ಷರ>('a' + v - 10);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    str += static_cast<ಅಕ್ಷರ>('0' + v);
                }
            }
            ನಿಲ್ಲಿಸು;
        }
        }  // switch
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t fmtsizen = static_cast<ಗಾತ್ರ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ಎಡ ? ' ' : '0'), ಎಡ, str);
        ಹಿಂತಿರುಗಿ str;
    }  // case b/d/x/o
    ಸ್ಥಿತಿ 't':  //ಸಮಯ
    ಸ್ಥಿತಿ 'd': {  // Unsigned decimal
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ issigned = formatAttr.isSigned();
        ಆದರೆ (fmtsize == "" && !ಎಡ) {
            ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ mantissabits = ಅಗಲ() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            ದ್ವಿಗುಣ dchars = mantissabits / 3.321928094887362 + 1.0;
            ಆದರೆ (issigned) ++dchars;  // space for sign
            fmtsize = cvt_ಗಮ್ಯ_Str(static_cast<ಪೂರ್ಣಾಂಕ>(dchars));
        }
        ತಾರ್ಕಿಕ hasXZ = ಸುಳ್ಳು;
        ಆದರೆ (isAllX()) {
            str = "x";
            hasXZ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isAllZ()) {
            str = "z";
            hasXZ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isAnyX()) {
            str = "X";
            hasXZ = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isAnyZ()) {
            str = "Z";
            hasXZ = ನಿಜ;
        }
        ಆದರೆ (!hasXZ) {
            ಆದರೆ (issigned) {
                ಆದರೆ (ಅಗಲ() > 64) {
                    str = ಗಮ್ಯ_Decimal_S();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    str = cvt_ಗಮ್ಯ_Str(ಗಮ್ಯ_SQuad());
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (ಅಗಲ() > 64) {
                    str = ಗಮ್ಯ_Decimal_U();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    str = cvt_ಗಮ್ಯ_Str(ಗಮ್ಯ_UQuad());
                }
            }
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ zeropad = fmtsize.ಉದ್ದಳತೆ() > 0 && fmtsize[0] == '0' && !ಎಡ;
        // fmtsize might have changed since we parsed the %fmtsize
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t fmtsizen = static_cast<ಗಾತ್ರ_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ಎಡ, str);
        ಹಿಂತಿರುಗಿ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    ಸ್ಥಿತಿ 'u': {  // Packed 2-state
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
            ಸ್ಥಿರಾಂಕ uint32_t v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ;
            str += static_cast<ಅಕ್ಷರ>((v >> 0) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v >> 8) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v >> 16) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v >> 24) & 0xff);
        }
        ಹಿಂತಿರುಗಿ str;
    }
    ಸ್ಥಿತಿ 'z': {  // Packed 4-state
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
            ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ >> 0) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ >> 8) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ >> 16) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ >> 24) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ_X >> 0) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ_X >> 8) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ_X >> 16) & 0xff);
            str += static_cast<ಅಕ್ಷರ>((v.m_ಮೌಲ್ಯ_X >> 24) & 0xff);
        }
        ಹಿಂತಿರುಗಿ str;
    }
    ಸ್ಥಿತಿ 'v': {  // Strength
        ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ() - 1;
        ಗಾಗಿ (; ಬಿಟ್_2 >= 0; ಬಿಟ್_2--) {
            ಆದರೆ (ಬಿಟ್_Is0(ಬಿಟ್_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಟ್_Is1(ಬಿಟ್_2)) {
                str += "St1 ";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಟ್_Is_Z(ಬಿಟ್_2)) {
                str += "StZ ";
            } ಇಲ್ಲದಿದ್ದರೆ {
                str += "StX";
            }
        }
        ಹಿಂತಿರುಗಿ str;
    }
    ಪೂರ್ವನಿಯೋಜಿತ: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_ಸಂಖ್ಯೆ::emitC() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    constexpr ಗಾತ್ರ_t bufsize = 1000;
    ಅಕ್ಷರ sbuf[bufsize];
    string ಫಲಿತಾಂಶ;
    ಆದರೆ (isNull()) {
        ಹಿಂತಿರುಗಿ "VlNull{}";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isDouble()) {
        ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ dnum = ಗಮ್ಯ_Double();
        ಆದರೆ (std::isinf(dnum)) {
            ಆದರೆ (std::signbit(dnum)) ಫಲಿತಾಂಶ += '-';
            ಫಲಿತಾಂಶ += "std::numeric_limits<double>::infinity()";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::isnan(dnum)) {
            ಆದರೆ (std::signbit(dnum)) ಫಲಿತಾಂಶ += '-';
            ಫಲಿತಾಂಶ += "std::numeric_limits<double>::quiet_NaN()";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ fmt = (static_cast<ಪೂರ್ಣಾಂಕ>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ಹಿಂತಿರುಗಿ sbuf;
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isString()) {
        ಸ್ಥಿರಾಂಕ string strg = ಗಮ್ಯ_String();
        ಸ್ಥಿರಾಂಕ string quoted
            = V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(strg, V3_ಹೊರಹೋಗು_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ಹಿಂತಿರುಗಿ '"' + quoted + "\"s";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಪದಗಳು() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        ಫಲಿತಾಂಶ = "::VlWide<" + std::ಗಮ್ಯ_string(ಪದಗಳು()) + ">{{";
        ಆದರೆ (ಪದಗಳು() > 4) ಫಲಿತಾಂಶ += '\n';
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ n = 0; n < ಪದಗಳು(); ++n) {
            ಆದರೆ (n) ಫಲಿತಾಂಶ += ((n % 4) ? ", " : ",\n");
            (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_ಶಬ್ದ(n));
            ಫಲಿತಾಂಶ += sbuf;
        }
        ಆದರೆ (ಪದಗಳು() > 4) ಫಲಿತಾಂಶ += '\n';
        ಫಲಿತಾಂಶ += "}}";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಪದಗಳು() == 2) {  // Quad
        ಸ್ಥಿರಾಂಕ uint64_t qnum = static_cast<uint64_t>(ಗಮ್ಯ_UQuad());
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ಹಿಂತಿರುಗಿ sbuf;
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ಸ್ಥಿರಾಂಕ uint32_t unum = ಗಮ್ಯ_UInt();
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (ಅಗಲ() > 16) ? ("0x%08" PRIx32 "U")
                                : (ಅಗಲ() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ಶೂನ್ಯ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ಹಿಂತಿರುಗಿ sbuf;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_Decimal_S() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ಆದರೆ (isNegative()) {
        V3_ಸಂಖ್ಯೆ lhsNoSign = *ಇದು;
        lhsNoSign.opNegate(*ಇದು);
        ಹಿಂತಿರುಗಿ "-"s + lhsNoSign.ಗಮ್ಯ_Decimal_U();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ ಗಮ್ಯ_Decimal_U();
    }
}

string V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_Decimal_U() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ maxdecwidth = (ಅಗಲ() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_ಸಂಖ್ಯೆ bcd(ಇದು, maxdecwidth + 4);
    V3_ಸಂಖ್ಯೆ tmp(ಇದು, maxdecwidth + 4);
    V3_ಸಂಖ್ಯೆ tmp2(ಇದು, maxdecwidth + 4);

    ಪೂರ್ಣಾಂಕ from_ಬಿಟ್ = ಅಗಲ() - 1;
    // Skip all leading zeros
    ಗಾಗಿ (; from_ಬಿಟ್ >= 0 && ಬಿಟ್_Is0(from_ಬಿಟ್); --from_ಬಿಟ್) {}
    // Double-dabble algorithm
    ಗಾಗಿ (; from_ಬಿಟ್ >= 0; --from_ಬಿಟ್) {
        // Any digits >= 5 need an add 3 (via tmp)
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ nibble_ಬಿಟ್ = 0; nibble_ಬಿಟ್ < maxdecwidth; nibble_ಬಿಟ್ += 4) {
            ಆದರೆ (bcd.ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(nibble_ಬಿಟ್, 4) >= 5) {
                tmp2.ಗಣ_All_Bits0();
                tmp2.ಗಣ_ಬಿಟ್(nibble_ಬಿಟ್, 1);  // Add 3, decomposed as two bits
                tmp2.ಗಣ_ಬಿಟ್(nibble_ಬಿಟ್ + 1, 1);
                tmp.op_ನಿಯೋಜಿಸು(bcd);
                bcd.op_ಕೂಡಿಸು(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_ನಿಯೋಜಿಸು(bcd);
        bcd.op_ಸರಿಸು_L(tmp, V3_ಸಂಖ್ಯೆ(ಇದು, 32, 1));
        // bcd[0] = this[from_bit]
        ಆದರೆ (ಬಿಟ್_Is1(from_ಬಿಟ್)) bcd.ಗಣ_ಬಿಟ್(0, 1);
    }

    string output;
    ಪೂರ್ಣಾಂಕ lsb = (maxdecwidth - 1) & ~3;
    ಗಾಗಿ (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        ಆದರೆ (bcd.ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(lsb, 4)) ನಿಲ್ಲಿಸು;
    }
    ಗಾಗಿ (; lsb >= 0; lsb -= 4) {
        output += static_cast<ಅಕ್ಷರ>('0' + bcd.ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(lsb, 4));  // 0..9
    }
    ಹಿಂತಿರುಗಿ output;
}

string V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_Real_String() ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಮೌಲ್ಯ = ಗಮ್ಯ_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream ಹೊರಹೋಗು;
    ಹೊರಹೋಗು.precision(15);
    ಹೊರಹೋಗು << ಮೌಲ್ಯ;
    string ಫಲಿತಾಂಶ = ಹೊರಹೋಗು.str();
    ಆದರೆ (std::strtod(ಫಲಿತಾಂಶ.c_str(), ಶೂನ್ಯಸೂಚಕ) != ಮೌಲ್ಯ) {
        ಹೊರಹೋಗು.str("");
        ಹೊರಹೋಗು.precision(17);
        ಹೊರಹೋಗು << ಮೌಲ್ಯ;
        ಫಲಿತಾಂಶ = ಹೊರಹೋಗು.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    ಆದರೆ (ಫಲಿತಾಂಶ.ಕಂಡುಹಿಡಿ_ಮೊದಲ_ದ(".en") == string::npos) ಫಲಿತಾಂಶ += ".0";
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//======================================================================
// ACCESSORS - as numbers

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::ಹೊಂದುತ್ತದೆ_ಒಳ_UInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isFourState()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    // We allow wide numbers that represent values <= 32 bits
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < ಪದಗಳು(); ++i) {
        ಆದರೆ (m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
uint32_t V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_UInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    UASSERT(!isFourState(), "toUInt with 4-state " << *ಇದು);
    // We allow wide numbers that represent values <= 32 bits
    ಆದರೆ (VL_UNCOVERABLE(!ಹೊಂದುತ್ತದೆ_ಒಳ_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *ಇದು);
    }
    ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ;
}

ದ್ವಿಗುಣ V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_Double() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    UASSERT(isDouble() && ಅಗಲ() == 64, "Real operation on wrong sized/non-real number");
    ಒಕ್ಕೂಟ {
        ದ್ವಿಗುಣ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ;
    u.u[1] = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ;
    ಹಿಂತಿರುಗಿ u.d;
}

int32_t V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_SInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isSigned()) {
        ಸ್ಥಿರಾಂಕ uint32_t v = ಗಮ್ಯ_UInt();
        ಸ್ಥಿರಾಂಕ uint32_t signExtend = (-(v & (1UL << (ಅಗಲ() - 1))));
        ಸ್ಥಿರಾಂಕ uint32_t extended = v | signExtend;
        ಹಿಂತಿರುಗಿ static_cast<int32_t>(extended);
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ಹಿಂತಿರುಗಿ static_cast<int32_t>(ಗಮ್ಯ_UInt());
    }
}

uint64_t V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_UQuad() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *ಇದು);
    // We allow wide numbers that represent values <= 64 bits
    ಆದರೆ (isDouble()) ಹಿಂತಿರುಗಿ static_cast<uint64_t>(ಗಮ್ಯ_Double());
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 2; i < ಪದಗಳು(); ++i) {
        ಆದರೆ (m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ) {
            v3error("Value too wide for 64-bits expected in this context " << *ಇದು);
            ನಿಲ್ಲಿಸು;
        }
    }
    ಆದರೆ (ಅಗಲ() <= 32) ಹಿಂತಿರುಗಿ (static_cast<uint64_t>(ಗಮ್ಯ_UInt()));
    ಹಿಂತಿರುಗಿ ((static_cast<uint64_t>(m_ದತ್ತಾಂಶ.ಅಂಕೆ()[1].m_ಮೌಲ್ಯ) << 32ULL)
            | (static_cast<uint64_t>(m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ)));
}

int64_t V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_SQuad() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (isDouble()) ಹಿಂತಿರುಗಿ static_cast<int64_t>(ಗಮ್ಯ_Double());
    ಸ್ಥಿರಾಂಕ uint64_t v = ಗಮ್ಯ_UQuad();
    ಸ್ಥಿರಾಂಕ uint64_t signExtend = (-(v & (1ULL << (ಅಗಲ() - 1))));
    ಸ್ಥಿರಾಂಕ uint64_t extended = v | signExtend;
    ಹಿಂತಿರುಗಿ static_cast<int64_t>(extended);
}

string V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_String() ಸ್ಥಿರಾಂಕ {
    UASSERT(!isFourState(), "toString with 4-state " << *ಇದು);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.str();
    ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ() - 1;
    ಆಗ ((ಬಿಟ್_2 % 8) != 7) ++ಬಿಟ್_2;
    std::string str;
    ಗಾಗಿ (; ಬಿಟ್_2 >= 0; ಬಿಟ್_2 -= 8) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ v = ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(ಬಿಟ್_2 - 7, 8);
        ಆದರೆ (v) str += static_cast<ಅಕ್ಷರ>(v);
    }
    ಹಿಂತಿರುಗಿ str;
}

uint32_t V3_ಸಂಖ್ಯೆ::edata_ಶಬ್ದ(ಪೂರ್ಣಾಂಕ eword) ಸ್ಥಿರಾಂಕ {
    UASSERT(!isFourState(), "edataWord with 4-state " << *ಇದು);
    ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.ಅಂಕೆ()[eword].m_ಮೌಲ್ಯ;
}

uint8_t V3_ಸಂಖ್ಯೆ::ದತ್ತಾಂಶ_Byte(ಪೂರ್ಣಾಂಕ byte) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (edata_ಶಬ್ದ(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAllZ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isDouble() || isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಸ್ಥಿರಾಂಕ uint32_t ಮುಖವಾಡ = i == ಪದಗಳು() - 1 ? hi_ಶಬ್ದ_ಮುಖವಾಡ() : ~0U;
        ಆದರೆ (((~v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) & ಮುಖವಾಡ) != ಮುಖವಾಡ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAllX() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isDouble() || isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    uint32_t ಮುಖವಾಡ = hi_ಶಬ್ದ_ಮುಖವಾಡ();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = ಪದಗಳು() - 1; i >= 0; --i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಆದರೆ ((v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) ^ ಮುಖವಾಡ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಮುಖವಾಡ = ~0U;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isEqZero() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.str().ಖಾಲಿಗಳು();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಆದರೆ (v.m_ಮೌಲ್ಯ || v.m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isNeqZero() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ !m_ದತ್ತಾಂಶ.str().ಖಾಲಿಗಳು();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಆದರೆ (v.m_ಮೌಲ್ಯ & ~v.m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::is_ಬಿಟ್ಟುಗಳು_Zero(ಪೂರ್ಣಾಂಕ msb, ಪೂರ್ಣಾಂಕ lsb) ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = lsb; i <= msb; ++i) {
        ಆದರೆ (VL_UNLIKELY(!ಬಿಟ್_Is0(i))) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isEqOne() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ != 1 || m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i < ಪದಗಳು(); ++i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಆದರೆ (v.m_ಮೌಲ್ಯ || v.m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isEqAllOnes(ಪೂರ್ಣಾಂಕ optwidth) ಸ್ಥಿರಾಂಕ {
    // Correct number of zero bits/width matters
    ಆದರೆ (!optwidth) optwidth = ಅಗಲ();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < optwidth; ++ಬಿಟ್_2) {
        ಆದರೆ (!ಬಿಟ್_Is1(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isFourState() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isDouble() || isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಆದರೆ (m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAnyX() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isDouble() || isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಆದರೆ (v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAnyXZ() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (isDouble() || isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಆದರೆ (m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isAnyZ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (isDouble() || isString()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
        ಆದರೆ (~v.m_ಮೌಲ್ಯ & v.m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isLtXZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) ಸ್ಥಿರಾಂಕ {
    // Include X/Z in comparisons for sort ordering
    ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ ಗಮ್ಯ_String() < rhs.ಗಮ್ಯ_String();
    ಆದರೆ (isDouble()) ಹಿಂತಿರುಗಿ ಗಮ್ಯ_Double() < rhs.ಗಮ್ಯ_Double();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (ಬಿಟ್_Is1(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (rhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && ಬಿಟ್_Is0(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}
ಪೂರ್ಣಾಂಕ V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_X(ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ nbits) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_2 = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ bitn = 0; bitn < nbits; ++bitn) {
        ಆದರೆ (lsb + bitn >= ಅಗಲ()) ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
        ಆದರೆ (ಬಿಟ್_Is_X(lsb + bitn)) ++ಎಣಿಕೆ_2;
    }
    ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
}
ಪೂರ್ಣಾಂಕ V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_Z(ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ nbits) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_2 = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ bitn = 0; bitn < nbits; ++bitn) {
        ಆದರೆ (lsb + bitn >= ಅಗಲ()) ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
        ಆದರೆ (ಬಿಟ್_Is_Z(lsb + bitn)) ++ಎಣಿಕೆ_2;
    }
    ಹಿಂತಿರುಗಿ ಎಣಿಕೆ_2;
}

ಪೂರ್ಣಾಂಕ V3_ಸಂಖ್ಯೆ::ಅಗಲ_ಗಮ್ಯ_Fit() ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ() - 1; ಬಿಟ್_2 > 0; ಬಿಟ್_2--) {
        ಆದರೆ (!ಬಿಟ್_Is0(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಬಿಟ್_2 + 1;
    }
    ಹಿಂತಿರುಗಿ 1;  // one bit even if number is == 0
}

uint32_t V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl) ಸ್ಥಿರಾಂಕ {
    // Correct number of zero bits/width matters
    ಪೂರ್ಣಾಂಕ n = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆಯ್ಕೆ (ctrl.ಬಿಟ್_Is(0)) {
        ಸ್ಥಿತಿ '0':
            ಆದರೆ (ಬಿಟ್_Is0(ಬಿಟ್_2)) ++n;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ '1':
            ಆದರೆ (ಬಿಟ್_Is1(ಬಿಟ್_2)) ++n;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 'x':
            ಆದರೆ (ಬಿಟ್_Is_X(ಬಿಟ್_2)) ++n;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 'z':
            ಆದರೆ (ಬಿಟ್_Is_Z(ಬಿಟ್_2)) ++n;
            ನಿಲ್ಲಿಸು;
        }
    }
    ಹಿಂತಿರುಗಿ n;
}

uint32_t V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl1, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl2,
                             ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl3) ಸ್ಥಿರಾಂಕ {
    // Correct number of zero bits/width matters
    ಪೂರ್ಣಾಂಕ n = ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ctrl1);
    ಆದರೆ (ctrl2.ಬಿಟ್_Is(0) != ctrl1.ಬಿಟ್_Is(0)) n += ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ctrl2);
    ಆದರೆ ((ctrl3.ಬಿಟ್_Is(0) != ctrl1.ಬಿಟ್_Is(0)) && (ctrl3.ಬಿಟ್_Is(0) != ctrl2.ಬಿಟ್_Is(0))) {
        n += ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ctrl3);
    }
    ಹಿಂತಿರುಗಿ n;
}

uint32_t V3_ಸಂಖ್ಯೆ::ಎಣಿಕೆ_Ones() ಸ್ಥಿರಾಂಕ {
    ಪೂರ್ಣಾಂಕ n = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (ಬಿಟ್_Is1(ಬಿಟ್_2)) ++n;
    }
    ಹಿಂತಿರುಗಿ n;
}

uint32_t V3_ಸಂಖ್ಯೆ::most_ಗಣ_ಬಿಟ್_P1() ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = ಅಗಲ() - 1; ಬಿಟ್_2 >= 0; ಬಿಟ್_2--) {
        ಆದರೆ (!ಬಿಟ್_Is0(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಬಿಟ್_2 + 1;
    }
    ಹಿಂತಿರುಗಿ 0;
}

uint32_t V3_ಸಂಖ್ಯೆ::ಕನಿಷ್ಠ_ಗಣ_ಬಿಟ್_P1() ಸ್ಥಿರಾಂಕ {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (!ಬಿಟ್_Is0(ಬಿಟ್_2)) ಹಿಂತಿರುಗಿ ಬಿಟ್_2 + 1;
    }
    ಹಿಂತಿರುಗಿ 0;
}

//======================================================================

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಬಿಟ್ಟುಗಳು_Non_XZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2) || lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಬಿಟ್ಟುಗಳು_One(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಬಿಟ್ಟುಗಳು_XZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಬಿಟ್ಟುಗಳು_Z(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is_Z(ಬಿಟ್_2)) ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRedOr(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // op i, 1 bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಅಕ್ಷರ outc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < lhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(1);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
        } ಇಲ್ಲದಿದ್ದರೆ {
            outc = 'x';
        }
    }
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRedAnd(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಅಕ್ಷರ outc = 1;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < lhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
            ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(0);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
        } ಇಲ್ಲದಿದ್ದರೆ {
            outc = 'x';
        }
    }
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRedXor(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // op i, 1 bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಅಕ್ಷರ outc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < lhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಆದರೆ (outc == 1) {
                outc = 0;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (outc == 0) {
                outc = 1;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
        } ಇಲ್ಲದಿದ್ದರೆ {
            outc = 'x';
        }
    }
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಭಿವ್ಯಕ್ತಿ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl1, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl2,
                                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl3) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS4(ಅಭಿವ್ಯಕ್ತಿ, ctrl1, ctrl2, ctrl3);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS4(ಅಭಿವ್ಯಕ್ತಿ, ctrl1, ctrl2, ctrl3);
    ಗಣ_Zero();
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಅಭಿವ್ಯಕ್ತಿ.ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಎಣಿಕೆ_Ones(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಆದರೆ (lhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಣ_Zero();
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = lhs.ಎಣಿಕೆ_Ones();
    opCleanThis();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opIsUnknown(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.isAnyXZ());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opOneHot(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಆದರೆ (lhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಎಣಿಕೆ_Ones() == 1);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opOneHot0(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಆದರೆ (lhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಎಣಿಕೆ_Ones() <= 1);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCLog2(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಆದರೆ (lhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ adjust = (lhs.ಎಣಿಕೆ_Ones() == 1) ? 0 : 1;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = lhs.ಅಗಲ() - 1; ಬಿಟ್_2 >= 0; ಬಿಟ್_2--) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಗಣ_Long(ಬಿಟ್_2 + adjust);
            ಹಿಂತಿರುಗಿ *ಇದು;
        }
    }
    ಗಣ_Zero();
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_Most_ಗಣ_ಬಿಟ್_P1(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಆದರೆ (lhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = lhs.ಅಗಲ() - 1; ಬಿಟ್_2 >= 0; ಬಿಟ್_2--) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಗಣ_Long(ಬಿಟ್_2 + 1);
            ಹಿಂತಿರುಗಿ *ಇದು;
        }
    }
    ಗಣ_Zero();
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogNot(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    ಅಕ್ಷರ outc = 1;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < lhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            outc = 0;
            ಹೋಗು ಕೊನೆಯ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
        } ಇಲ್ಲದಿದ್ದರೆ {
            outc = 'x';
        }
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNot(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'x');
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAnd(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2) || rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {  // 0
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'x');
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opOr(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2) || rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
            // 0
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'x');
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opXor(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 1);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2) || rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'x');
        }
        // else zero
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::fill_ಬಿಟ್ಟುಗಳು(ಪೂರ್ಣಾಂಕ destLsb, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಅಕ್ಷರ ಮೌಲ್ಯ) {
    UASSERT(destLsb >= 0 && ಅಗಲ >= 0, "Negative bit range");
    ಆದರೆ (!ಅಗಲ) ಹಿಂತಿರುಗಿ;
    UASSERT(ಅಗಲ <= ಇದು->ಅಗಲ() - destLsb, "Destination bit range exceeds number width");

    ಸ್ಥಿರಾಂಕ uint32_t ಮೌಲ್ಯ_ಶಬ್ದ = ಮೌಲ್ಯ == '1' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 1 || ಮೌಲ್ಯ == 3 ? ~0U : 0;
    ಸ್ಥಿರಾಂಕ uint32_t x_ಶಬ್ದ = ಮೌಲ್ಯ == 'z' || ಮೌಲ್ಯ == 'x' || ಮೌಲ್ಯ == 2 || ಮೌಲ್ಯ == 3 ? ~0U : 0;
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೊದಲ_ಶಬ್ದ = VL_BITWORD_E(destLsb);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_ಶಬ್ದ = VL_BITWORD_E(destLsb + ಅಗಲ - 1);
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಶಬ್ದ = ಮೊದಲ_ಶಬ್ದ; ಶಬ್ದ <= ಕೊನೆಯ_ಶಬ್ದ; ++ಶಬ್ದ) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಶಬ್ದ_Lsb = ಶಬ್ದ == ಮೊದಲ_ಶಬ್ದ ? VL_BITBIT_E(destLsb) : 0;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಶಬ್ದ_Msb = ಶಬ್ದ == ಕೊನೆಯ_ಶಬ್ದ ? VL_BITBIT_E(destLsb + ಅಗಲ - 1) : VL_SIZEBITS_E;
        ಸ್ಥಿರಾಂಕ uint32_t ಮುಖವಾಡ = VL_ಮುಖವಾಡ_E(ಶಬ್ದ_Msb - ಶಬ್ದ_Lsb + 1) << ಶಬ್ದ_Lsb;
        ಮೌಲ್ಯ_And_X& dest = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ];
        dest.m_ಮೌಲ್ಯ = (dest.m_ಮೌಲ್ಯ & ~ಮುಖವಾಡ) | (ಮೌಲ್ಯ_ಶಬ್ದ & ಮುಖವಾಡ);
        dest.m_ಮೌಲ್ಯ_X = (dest.m_ಮೌಲ್ಯ_X & ~ಮುಖವಾಡ) | (x_ಶಬ್ದ & ಮುಖವಾಡ);
    }
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::copy_ಬಿಟ್ಟುಗಳು(ಪೂರ್ಣಾಂಕ destLsb, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೂಲ, ಪೂರ್ಣಾಂಕ ಮೂಲ_Lsb, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
    UASSERT(destLsb >= 0 && ಮೂಲ_Lsb >= 0 && ಅಗಲ >= 0, "Negative bit range");
    ಆದರೆ (!ಅಗಲ) ಹಿಂತಿರುಗಿ;
    UASSERT(ಅಗಲ <= ಇದು->ಅಗಲ() - destLsb, "Destination bit range exceeds number width");
    UASSERT(ಅಗಲ <= ಮೂಲ.ಅಗಲ() - ಮೂಲ_Lsb, "Source bit range exceeds number width");

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೊದಲ_ಶಬ್ದ = VL_BITWORD_E(destLsb);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_ಶಬ್ದ = VL_BITWORD_E(destLsb + ಅಗಲ - 1);
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ dest_ಶಬ್ದ = ಮೊದಲ_ಶಬ್ದ; dest_ಶಬ್ದ <= ಕೊನೆಯ_ಶಬ್ದ; ++dest_ಶಬ್ದ) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dest_ಬಿಟ್ = std::max(destLsb, dest_ಶಬ್ದ * VL_EDATASIZE);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ dest_ಸ್ಥಾನಾಂತರ = VL_BITBIT_E(dest_ಬಿಟ್);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೂಲ_ಬಿಟ್ = ಮೂಲ_Lsb + dest_ಬಿಟ್ - destLsb;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೂಲ_ಶಬ್ದ = VL_BITWORD_E(ಮೂಲ_ಬಿಟ್);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೂಲ_ಸ್ಥಾನಾಂತರ = VL_BITBIT_E(ಮೂಲ_ಬಿಟ್);
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ copy_ಅಗಲ = std::min(ಅಗಲ - (dest_ಬಿಟ್ - destLsb), VL_EDATASIZE - dest_ಸ್ಥಾನಾಂತರ);
        ಸ್ಥಿರಾಂಕ uint32_t ಮುಖವಾಡ = VL_ಮುಖವಾಡ_E(copy_ಅಗಲ) << dest_ಸ್ಥಾನಾಂತರ;

        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X& ಮೂಲ_Lo = ಮೂಲ.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಮೂಲ_ಶಬ್ದ];
        uint32_t ಮೌಲ್ಯ = ಮೂಲ_Lo.m_ಮೌಲ್ಯ >> ಮೂಲ_ಸ್ಥಾನಾಂತರ;
        uint32_t ಮೌಲ್ಯ_X = ಮೂಲ_Lo.m_ಮೌಲ್ಯ_X >> ಮೂಲ_ಸ್ಥಾನಾಂತರ;
        ಆದರೆ (ಮೂಲ_ಸ್ಥಾನಾಂತರ && copy_ಅಗಲ > VL_EDATASIZE - ಮೂಲ_ಸ್ಥಾನಾಂತರ) {
            ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X& ಮೂಲ_Hi = ಮೂಲ.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಮೂಲ_ಶಬ್ದ + 1];
            ಮೌಲ್ಯ |= ಮೂಲ_Hi.m_ಮೌಲ್ಯ << (VL_EDATASIZE - ಮೂಲ_ಸ್ಥಾನಾಂತರ);
            ಮೌಲ್ಯ_X |= ಮೂಲ_Hi.m_ಮೌಲ್ಯ_X << (VL_EDATASIZE - ಮೂಲ_ಸ್ಥಾನಾಂತರ);
        }

        ಮೌಲ್ಯ_And_X& dest = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[dest_ಶಬ್ದ];
        dest.m_ಮೌಲ್ಯ = (dest.m_ಮೌಲ್ಯ & ~ಮುಖವಾಡ) | ((ಮೌಲ್ಯ << dest_ಸ್ಥಾನಾಂತರ) & ಮುಖವಾಡ);
        dest.m_ಮೌಲ್ಯ_X = (dest.m_ಮೌಲ್ಯ_X & ~ಮುಖವಾಡ) | ((ಮೌಲ್ಯ_X << dest_ಸ್ಥಾನಾಂತರ) & ಮುಖವಾಡ);
    }
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opConcat(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಗಣ_Zero();
    // See also error in V3Width
    ಆದರೆ (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_ಬಿಟ್ಟುಗಳು(0, rhs, 0, rhs.ಅಗಲ());
    copy_ಬಿಟ್ಟುಗಳು(rhs.ಅಗಲ(), lhs, 0, lhs.ಅಗಲ());
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLenN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಗಣ_Quad(lhs.ಗಮ್ಯ_String().ಉದ್ದಳತೆ());
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRepl(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs,
                           ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ಹಿಂತಿರುಗಿ opRepl(lhs, rhs.ಗಮ್ಯ_UInt());
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRepl(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಆದರೆ (v3Global.opt.replication_ಎಲ್ಲೆ()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_ಎಲ್ಲೆ())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_ಎಲ್ಲೆ() << " is suspect: " << rhsval);
    }
    ಗಣ_Zero();
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ times = 0; times < rhsval; ++times) {
        ಸ್ಥಿರಾಂಕ uint64_t destLsb = uint64_t{times} * lhs.ಅಗಲ();
        ಆದರೆ (destLsb >= static_cast<uint32_t>(ಅಗಲ())) ನಿಲ್ಲಿಸು;
        copy_ಬಿಟ್ಟುಗಳು(static_cast<ಪೂರ್ಣಾಂಕ>(destLsb), lhs, 0,
                 std::min(lhs.ಅಗಲ(), ಅಗಲ() - static_cast<ಪೂರ್ಣಾಂಕ>(destLsb)));
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಹರಿವು_L(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಗಣ_Zero();
    // See also error in V3Width
    ಆದರೆ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ssize = std::min(rhs.ಗಮ್ಯ_UInt(), static_cast<ಚಿಹ್ನೆರಹಿತ>(lhs.ಅಗಲ()));
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ istart = 0; istart < lhs.ಅಗಲ(); istart += ssize) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ostart = std::max(0, lhs.ಅಗಲ() - ssize - istart);
        copy_ಬಿಟ್ಟುಗಳು(ostart, lhs, istart, std::min(ssize, lhs.ಅಗಲ() - istart));
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogAnd(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಅಕ್ಷರ loutc = 0;
    ಅಕ್ಷರ routc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < lhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            loutc = 1;
            ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2) && loutc == 0) loutc = 'x';
    }
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < rhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            routc = 1;
            ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2) && routc == 0) routc = 'x';
    }
    ಅಕ್ಷರ outc = 'x';
    ಆದರೆ (routc == 1 && loutc == 1) outc = 1;
    ಆದರೆ (routc == 0 || loutc == 0) outc = 0;
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogOr(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಅಕ್ಷರ outc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < lhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            outc = 1;
            ಹೋಗು ಕೊನೆಯ;
        }
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2) && outc == 0) outc = 'x';
    }
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < rhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            outc = 1;
            ಹೋಗು ಕೊನೆಯ;
        }
        ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2) && outc == 0) outc = 'x';
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogIf(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ lnot = lhs;
    lnot.opLogNot(lhs);
    ಹಿಂತಿರುಗಿ opLogOr(lnot, rhs);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLogEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_ಸಂಖ್ಯೆ ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ಹಿಂತಿರುಗಿ opLogAnd(ifa, ifb);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opAtoN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಂಕ ತಳ) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    UASSERT(ತಳ == AstAtoN::ATOREAL || ತಳ == 2 || ತಳ == 8 || ತಳ == 10 || ತಳ == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.ಗಮ್ಯ_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.ಕೊನೆ(), '_'), str.ಕೊನೆ());

    ಆದರೆ (ತಳ == AstAtoN::ATOREAL) ಹಿಂತಿರುಗಿ ಗಣ_Double(std::atof(str.c_str()));

    errno = 0;
    ಸ್ವಯಂಚಾಲಿತ v = std::strtol(str.c_str(), ಶೂನ್ಯಸೂಚಕ, ತಳ);
    ಆದರೆ (errno != 0) v = 0;
    ಹಿಂತಿರುಗಿ ಗಣ_Long_S(static_cast<int32_t>(v));
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPutcN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths) {
    ಅಂಕೆ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.ಗಮ್ಯ_String();
    ಸ್ಥಿರಾಂಕ int32_t i = rhs.ಗಮ್ಯ_SInt();
    ಸ್ಥಿರಾಂಕ int32_t c = ths.ಗಮ್ಯ_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    ಆದರೆ (0 <= i && static_cast<uint32_t>(i) < lstring.ಉದ್ದಳತೆ() && c != 0) lstring[i] = c;
    ಹಿಂತಿರುಗಿ ಗಣ_String(lstring);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGetcN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಸ್ಥಿರಾಂಕ string lstring = lhs.ಗಮ್ಯ_String();
    ಸ್ಥಿರಾಂಕ int32_t i = rhs.ಗಮ್ಯ_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    ಆದರೆ (0 <= i && static_cast<uint32_t>(i) < lstring.ಉದ್ದಳತೆ()) v = lstring[i];
    ಹಿಂತಿರುಗಿ ಗಣ_Long(v);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSubstrN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths) {
    ಅಂಕೆ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಸ್ಥಿರಾಂಕ string lstring = lhs.ಗಮ್ಯ_String();
    ಸ್ಥಿರಾಂಕ int32_t i = rhs.ಗಮ್ಯ_SInt();
    ಸ್ಥಿರಾಂಕ int32_t j = ths.ಗಮ್ಯ_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    ಆದರೆ (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.ಉದ್ದಳತೆ()) ಹಿಂತಿರುಗಿ ಗಣ_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ಹಿಂತಿರುಗಿ ಗಣ_String(lstring.substr(i, j - i + 1));
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCompareNN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ತಾರ್ಕಿಕ ignoreCase) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    ಪೂರ್ಣಾಂಕ ಫಲಿತಾಂಶ;
    ಸ್ಥಿರಾಂಕ string lstring = lhs.ಗಮ್ಯ_String();
    ಸ್ಥಿರಾಂಕ string rstring = rhs.ಗಮ್ಯ_String();
    ಆದರೆ (ignoreCase) {
        ಫಲಿತಾಂಶ = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಫಲಿತಾಂಶ = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ಹಿಂತಿರುಗಿ ಗಣ_Long_S(ಫಲಿತಾಂಶ);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isString()) ಹಿಂತಿರುಗಿ opEqN(lhs, rhs);
    ಆದರೆ (lhs.isDouble()) ಹಿಂತಿರುಗಿ opEqD(lhs, rhs);
    ಅಕ್ಷರ outc = 1;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
            outc = 0;
            ಹೋಗು ಕೊನೆಯ;
        }
        ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            outc = 0;
            ಹೋಗು ಕೊನೆಯ;
        }
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
        ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNeq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isString()) ಹಿಂತಿರುಗಿ opNeqN(lhs, rhs);
    ಆದರೆ (lhs.isDouble()) ಹಿಂತಿರುಗಿ opNeqD(lhs, rhs);
    ಅಕ್ಷರ outc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) {
            outc = 1;
            ಹೋಗು ಕೊನೆಯ;
        }
        ಆದರೆ (lhs.ಬಿಟ್_Is0(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            outc = 1;
            ಹೋಗು ಕೊನೆಯ;
        }
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
        ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3Hash V3_ಸಂಖ್ಯೆ::ಗಮ್ಯ_Hash() ಸ್ಥಿರಾಂಕ {
    // Shouldmatch isCaseEq
    ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ V3Hash{ಗಮ್ಯ_String()};
    ಆದರೆ (isDouble()) ಹಿಂತಿರುಗಿ V3Hash{ಗಮ್ಯ_Double()};
    V3Hash hash{ಅಗಲ()};
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) hash += m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ;
    ಹಿಂತಿರುಗಿ hash;
}

ತಾರ್ಕಿಕ V3_ಸಂಖ್ಯೆ::isCaseEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) ಸ್ಥಿರಾಂಕ {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ ಗಮ್ಯ_String() == rhs.ಗಮ್ಯ_String();
    ಆದರೆ (isDouble()) ಹಿಂತಿರುಗಿ ಗಮ್ಯ_Double() == rhs.ಗಮ್ಯ_Double();
    ಆದರೆ (ಅಗಲ() != rhs.ಅಗಲ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
        ಆದರೆ (!(m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] == rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i])) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCaseEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCaseNeq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಕ್ಷರ outc = 0;
    ಆದರೆ (lhs.isString()) {
        ಹಿಂತಿರುಗಿ opNeqN(lhs, rhs);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.isDouble()) {
        ಹಿಂತಿರುಗಿ opNeqD(lhs, rhs);
    }
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is(ಬಿಟ್_2) != rhs.ಬಿಟ್_Is(ಬಿಟ್_2)) {
            outc = 1;
            ಹೋಗು ಕೊನೆಯ;
        }
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opWildEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಅಕ್ಷರ outc = 1;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (!rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
            ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
                outc = 'x';
                ಹೋಗು ಕೊನೆಯ;
            }
            ಆದರೆ (lhs.ಬಿಟ್_Is(ಬಿಟ್_2) != rhs.ಬಿಟ್_Is(ಬಿಟ್_2)) {
                outc = 0;
                ಹೋಗು ಕೊನೆಯ;
            }
        }
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opWildNeq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಅಕ್ಷರ outc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (!rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
            ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) {
                outc = 'x';
                ಹೋಗು ಕೊನೆಯ;
            }
            ಆದರೆ (lhs.ಬಿಟ್_Is(ಬಿಟ್_2) != rhs.ಬಿಟ್_Is(ಬಿಟ್_2)) {
                outc = 1;
                ಹೋಗು ಕೊನೆಯ;
            }
        }
    }
ಕೊನೆಯ:
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGt(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಅಕ್ಷರ outc = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ(), rhs.ಅಗಲ()); ++ಬಿಟ್_2) {
        ಆದರೆ (lhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) outc = 1;
        ಆದರೆ (rhs.ಬಿಟ್_Is1(ಬಿಟ್_2) && lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) outc = 0;
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
        ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
    }
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGtS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಅಕ್ಷರ outc = 0;
    {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ mbit = std::max(lhs.ಅಗಲ() - 1, rhs.ಅಗಲ() - 1);
        ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(mbit)) {
            outc = 'x';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(mbit)) {
            outc = 'x';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is0(mbit) && rhs.ಬಿಟ್_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.ಬಿಟ್_Is1_Extend(mbit) && rhs.ಬಿಟ್_Is0(mbit)) {
            outc = 0;  // - !> +
        } ಇಲ್ಲದಿದ್ದರೆ {
            // both positive or negative, normal >
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < std::max(lhs.ಅಗಲ() - 1, rhs.ಅಗಲ() - 1); ++ಬಿಟ್_2) {
                ಆದರೆ (lhs.ಬಿಟ್_Is1_Extend(ಬಿಟ್_2) && rhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) outc = 1;
                ಆದರೆ (rhs.ಬಿಟ್_Is1_Extend(ಬಿಟ್_2) && lhs.ಬಿಟ್_Is0(ಬಿಟ್_2)) outc = 0;
                ಆದರೆ (lhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
                ಆದರೆ (rhs.ಬಿಟ್_Is_XZ(ಬಿಟ್_2)) outc = 'x';
            }
        }
    }
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(outc);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGte(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ& eq = opEq(lhs, rhs);
    ಆದರೆ (eq.isNeqZero()) ಹಿಂತಿರುಗಿ eq;  // Return true
    ಹಿಂತಿರುಗಿ opGt(lhs, rhs);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGteS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ಸಂಖ್ಯೆ& eq = opEq(lhs, rhs);
    ಆದರೆ (eq.isNeqZero()) ಹಿಂತಿರುಗಿ eq;  // Return true
    ಹಿಂತಿರುಗಿ opGtS(lhs, rhs);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLt(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹಿಂತಿರುಗಿ opGt(rhs, lhs); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLte(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹಿಂತಿರುಗಿ opGte(rhs, lhs); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLtS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹಿಂತಿರುಗಿ opGtS(rhs, lhs); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLteS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) { ಹಿಂತಿರುಗಿ opGteS(rhs, lhs); }

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಸರಿಸು_R(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಣ_Zero();
    ಆದರೆ (rhs.ಅಗಲ() > 32 && !rhs.is_ಬಿಟ್ಟುಗಳು_Zero(rhs.ಅಗಲ() - 1, 32)) ಹಿಂತಿರುಗಿ *ಇದು;
    ಸ್ಥಿರಾಂಕ uint32_t rhsval = rhs.ಗಮ್ಯ_UInt();
    ಆದರೆ (rhsval < static_cast<uint32_t>(lhs.ಅಗಲ())) {
        copy_ಬಿಟ್ಟುಗಳು(0, lhs, static_cast<ಪೂರ್ಣಾಂಕ>(rhsval),
                 std::min(ಅಗಲ(), lhs.ಅಗಲ() - static_cast<ಪೂರ್ಣಾಂಕ>(rhsval)));
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಸರಿಸು_RS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಣ_Zero();
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅತಿಪ್ರವಾಹ = rhs.ಅಗಲ() > 32 && !rhs.is_ಬಿಟ್ಟುಗಳು_Zero(rhs.ಅಗಲ() - 1, 32);
    ಆದರೆ (!ಅತಿಪ್ರವಾಹ) {
        ಸ್ಥಿರಾಂಕ uint32_t rhsval = rhs.ಗಮ್ಯ_UInt();
        ಆದರೆ (rhsval < lbits) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ copy_ಅಗಲ = std::min(ಅಗಲ(), static_cast<ಪೂರ್ಣಾಂಕ>(lbits - rhsval));
            copy_ಬಿಟ್ಟುಗಳು(0, lhs, static_cast<ಪೂರ್ಣಾಂಕ>(rhsval), copy_ಅಗಲ);
            fill_ಬಿಟ್ಟುಗಳು(copy_ಅಗಲ, ಅಗಲ() - copy_ಅಗಲ, lhs.ಬಿಟ್_Is(lbits - 1));
            ಹಿಂತಿರುಗಿ *ಇದು;
        }
    }
    fill_ಬಿಟ್ಟುಗಳು(0, ಅಗಲ(), lhs.ಬಿಟ್_Is(lbits - 1));  // '0/'1/'x/'z
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಸರಿಸು_L(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಣ_Zero();
    ಆದರೆ (rhs.ಅಗಲ() > 32 && !rhs.is_ಬಿಟ್ಟುಗಳು_Zero(rhs.ಅಗಲ() - 1, 32)) ಹಿಂತಿರುಗಿ *ಇದು;
    ಸ್ಥಿರಾಂಕ uint32_t rhsval = rhs.ಗಮ್ಯ_UInt();
    ಆದರೆ (rhsval < static_cast<uint32_t>(ಅಗಲ())) {
        copy_ಬಿಟ್ಟುಗಳು(static_cast<ಪೂರ್ಣಾಂಕ>(rhsval), lhs, 0,
                 std::min(ಅಗಲ() - static_cast<ಪೂರ್ಣಾಂಕ>(rhsval), lhs.ಅಗಲ()));
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

//======================================================================
// Ops - Arithmetic

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNegate(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಆದರೆ (lhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    V3_ಸಂಖ್ಯೆ notlhs(&lhs, ಅಗಲ());
    notlhs.opNot(lhs);
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ one(&lhs, ಅಗಲ(), 1);
    op_ಕೂಡಿಸು(notlhs, one);
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಣ_Zero();
    // Addem
    uint64_t ವಾಹಕ = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಶಬ್ದ = 0; ಶಬ್ದ < ಪದಗಳು(); ++ಶಬ್ದ) {
        ಸ್ಥಿರಾಂಕ uint64_t lwordval = lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ].m_ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ uint64_t rwordval = rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ].m_ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ uint64_t ಮೊತ್ತ = lwordval + rwordval + ವಾಹಕ;
        m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ].m_ಮೌಲ್ಯ = ಮೊತ್ತ & 0xffffffffULL;
        ವಾಹಕ = ಮೊತ್ತ > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSub(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    V3_ಸಂಖ್ಯೆ negrhs(&rhs, ಅಗಲ());
    negrhs.opNegate(rhs);
    ಹಿಂತಿರುಗಿ op_ಕೂಡಿಸು(lhs, negrhs);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMul(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಗಣ_Zero();
    ಆದರೆ (ಅಗಲ() <= 64) {
        ಗಣ_Quad(lhs.ಗಮ್ಯ_UQuad() * rhs.ಗಮ್ಯ_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ lword = 0; lword < lhs.ಪದಗಳು(); ++lword) {
            ಸ್ಥಿರಾಂಕ uint64_t lwordval = static_cast<uint64_t>(lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[lword].m_ಮೌಲ್ಯ);
            ಆದರೆ (lwordval == 0) ಮುಂದುವರಿ;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ rword = 0; rword < rhs.ಪದಗಳು(); ++rword) {
                ಸ್ಥಿರಾಂಕ uint64_t rwordval = static_cast<uint64_t>(rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[rword].m_ಮೌಲ್ಯ);
                ಆದರೆ (rwordval == 0) ಮುಂದುವರಿ;
                uint64_t mul = lwordval * rwordval;
                ಗಾಗಿ (ಪೂರ್ಣಾಂಕ qword = lword + rword; qword < ಪದಗಳು(); ++qword) {
                    mul += static_cast<uint64_t>(m_ದತ್ತಾಂಶ.ಅಂಕೆ()[qword].m_ಮೌಲ್ಯ);
                    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[qword].m_ಮೌಲ್ಯ = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    ಆದರೆ (mul == 0) ನಿಲ್ಲಿಸು;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMulS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    V3_ಸಂಖ್ಯೆ lhsNoSign = lhs;
    ಆದರೆ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ಸಂಖ್ಯೆ rhsNoSign = rhs;
    ಆದರೆ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ qNoSign = opMul(lhsNoSign, rhsNoSign);
    ಆದರೆ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ಇಲ್ಲದಿದ್ದರೆ {
        op_ನಿಯೋಜಿಸು(qNoSign);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opDiv(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಆದರೆ (rhs.isEqZero()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved();
    ಆದರೆ (lhs.ಅಗಲ() <= 64) {
        ಗಣ_Quad(lhs.ಗಮ್ಯ_UQuad() / rhs.ಗಮ್ಯ_UQuad());
        ಹಿಂತಿರುಗಿ *ಇದು;
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Wide division
        ಹಿಂತಿರುಗಿ opModDivGuts(lhs, rhs, ಸುಳ್ಳು);
    }
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opDivS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಆದರೆ (rhs.isEqZero()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved();
    V3_ಸಂಖ್ಯೆ lhsNoSign = lhs;
    ಆದರೆ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ಸಂಖ್ಯೆ rhsNoSign = rhs;
    ಆದರೆ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    ಆದರೆ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ಇಲ್ಲದಿದ್ದರೆ {
        op_ನಿಯೋಜಿಸು(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *ಇದು);
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opModDiv(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಆದರೆ (rhs.isEqZero()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved();
    ಆದರೆ (lhs.ಅಗಲ() <= 64) {
        ಗಣ_Quad(lhs.ಗಮ್ಯ_UQuad() % rhs.ಗಮ್ಯ_UQuad());
        ಹಿಂತಿರುಗಿ *ಇದು;
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Wide modulus
        ಹಿಂತಿರುಗಿ opModDivGuts(lhs, rhs, ನಿಜ);
    }
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opModDivS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಆದರೆ (rhs.isEqZero()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved();
    V3_ಸಂಖ್ಯೆ lhsNoSign = lhs;
    ಆದರೆ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ಸಂಖ್ಯೆ rhsNoSign = rhs;
    ಆದರೆ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    ಆದರೆ (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } ಇಲ್ಲದಿದ್ದರೆ {
        op_ನಿಯೋಜಿಸು(qNoSign);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opModDivGuts(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ತಾರ್ಕಿಕ is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಗಣ_Zero();
    // Find MSB and check for zero.
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ l_ಪದಗಳು = lhs.ಪದಗಳು();
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ umsbp1 = lhs.most_ಗಣ_ಬಿಟ್_P1();  // dividend
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ vmsbp1 = rhs.most_ಗಣ_ಬಿಟ್_P1();  //ಭಾಜಕ
    ಆದರೆ (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *ಇದು);
        ಹಿಂತಿರುಗಿ *ಇದು;
    }

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    ಆದರೆ (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ j = uw - 1; j >= 0; j--) {
            ಸ್ಥಿರಾಂಕ uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ));
            m_ದತ್ತಾಂಶ.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ = unw64 / static_cast<uint64_t>(rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ);
            k = unw64
                - (static_cast<uint64_t>(m_ದತ್ತಾಂಶ.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ)
                   * static_cast<uint64_t>(rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *ಇದು << " rem=0x" << std::hex
                                   << k << std::dec);
        ಆದರೆ (is_modulus) {
            ಗಣ_Zero();
            m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = k;
        }
        opCleanThis();
        ಹಿಂತಿರುಗಿ *ಇದು;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_ಪದಗಳು + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_ಪದಗಳು + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < l_ಪದಗಳು; ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ = 0;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < l_ಪದಗಳು + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ಸ್ಥಿರಾಂಕ uint32_t ಸರಿಸು_ಮುಖವಾಡ = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ << s)
                | (ಸರಿಸು_ಮುಖವಾಡ & (rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i - 1].m_ಮೌಲ್ಯ >> (32 - s)));
    }
    vn[0] = rhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ << s;

    // Copy and shift dividend by same amount; may set new upper word
    ಆದರೆ (s) {
        un[uw] = lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[uw - 1].m_ಮೌಲ್ಯ >> (32 - s);
    } ಇಲ್ಲದಿದ್ದರೆ {
        un[uw] = 0;
    }
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ << s)
                | (ಸರಿಸು_ಮುಖವಾಡ & (lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i - 1].m_ಮೌಲ್ಯ >> (32 - s)));
    }
    un[0] = lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ j = uw - vw; j >= 0; j--) {
        // Estimate
        ಸ್ಥಿರಾಂಕ uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        ಆದರೆ (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            ಆದರೆ (rhat < 0x100000000ULL) ಹೋಗು again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < vw; ++i) {
            ಸ್ಥಿರಾಂಕ uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //ಕಳೆ
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_ದತ್ತಾಂಶ.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ = qhat;  // Save quotient digit

        ಆದರೆ (t < 0) {
            // Over subtracted; correct by adding back
            m_ದತ್ತಾಂಶ.ಅಂಕೆ()[j].m_ಮೌಲ್ಯ--;
            k = 0;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < vw; ++i) {
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

    ಆದರೆ (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < vw; ++i) {
            m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ = (un[i] >> s) | (ಸರಿಸು_ಮುಖವಾಡ & (un[i + 1] << (32 - s)));
        }
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = vw; i < l_ಪದಗಳು; ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i].m_ಮೌಲ್ಯ = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *ಇದು);
        ಹಿಂತಿರುಗಿ *ಇದು;
    } ಇಲ್ಲದಿದ್ದರೆ {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *ಇದು);
        ಹಿಂತಿರುಗಿ *ಇದು;
    }
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPow(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ತಾರ್ಕಿಕ lsign, ತಾರ್ಕಿಕ rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ಆದರೆ (lhs.isFourState() || rhs.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಆದರೆ (rhs.isEqZero()) ಹಿಂತಿರುಗಿ ಗಣ_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    ಆದರೆ (rsign && rhs.isNegative()) {
        ಆದರೆ (lhs.isEqZero()) {
            ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.isEqOne()) {
            ಹಿಂತಿರುಗಿ ಗಣ_Value1();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lsign && lhs.isEqAllOnes()) {
            ಆದರೆ (rhs.ಬಿಟ್_Is1(0)) {
                ಹಿಂತಿರುಗಿ ಗಣ_All_Bits1();  // -1^odd=-1
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ ಗಣ_Value1();  // -1^even=1
            }
        }
        ಹಿಂತಿರುಗಿ ಗಣ_Zero();
    }
    ಆದರೆ (lhs.isEqZero()) ಹಿಂತಿರುಗಿ ಗಣ_Zero();
    ಗಣ_Zero();
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = 1;
    V3_ಸಂಖ್ಯೆ power(&lhs, ಅಗಲ());
    power.op_ನಿಯೋಜಿಸು(lhs);
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < rhs.ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (ಬಿಟ್_2 > 0) {  // power = power*power
            V3_ಸಂಖ್ಯೆ ಕೊನೆಯ_Power(&lhs, ಅಗಲ());
            ಕೊನೆಯ_Power.op_ನಿಯೋಜಿಸು(power);
            power.opMul(ಕೊನೆಯ_Power, ಕೊನೆಯ_Power);
        }
        ಆದರೆ (rhs.ಬಿಟ್_Is1(ಬಿಟ್_2)) {  // out *= power
            V3_ಸಂಖ್ಯೆ ಕೊನೆಯ_ಹೊರಹೋಗು(&lhs, ಅಗಲ());
            ಕೊನೆಯ_ಹೊರಹೋಗು.op_ನಿಯೋಜಿಸು(*ಇದು);
            opMul(ಕೊನೆಯ_ಹೊರಹೋಗು, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowSU(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಹಿಂತಿರುಗಿ opPow(lhs, rhs, ನಿಜ, ಸುಳ್ಳು);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowSS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಹಿಂತಿರುಗಿ opPow(lhs, rhs, ನಿಜ, ನಿಜ);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowUS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // Correct number of zero bits/width matters
    ಹಿಂತಿರುಗಿ opPow(lhs, rhs, ಸುಳ್ಳು, ನಿಜ);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opBufIf1(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ens, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& if1s) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(ens, if1s);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(ens, if1s);
    ಗಣ_Zero();
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 < ಅಗಲ(); ++ಬಿಟ್_2) {
        ಆದರೆ (ens.ಬಿಟ್_Is1(ಬಿಟ್_2)) {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, if1s.ಬಿಟ್_Is(ಬಿಟ್_2));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಣ_ಬಿಟ್(ಬಿಟ್_2, 'z');
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

// Sets all bits in range to the given value
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಗಣ_Range(uint32_t lsb, uint32_t ಅಗಲ, ಅಕ್ಷರ ಬಿಟ್_ಮೌಲ್ಯ) {
    ಆದರೆ (lsb < static_cast<uint32_t>(ಇದು->ಅಗಲ())) {
        fill_ಬಿಟ್ಟುಗಳು(static_cast<ಪೂರ್ಣಾಂಕ>(lsb),
                 static_cast<ಪೂರ್ಣಾಂಕ>(std::min(ಅಗಲ, static_cast<uint32_t>(ಇದು->ಅಗಲ()) - lsb)),
                 ಬಿಟ್_ಮೌಲ್ಯ);
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) { ಹಿಂತಿರುಗಿ op_ನಿಯೋಜಿಸು_Non_XZ(lhs, ಸುಳ್ಳು); }
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ನಿಯೋಜಿಸು_Non_XZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ತಾರ್ಕಿಕ ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    ಆದರೆ (ಇದು != &lhs) {
        ಆದರೆ (VL_UNLIKELY(lhs.isNull())) {
            m_ದತ್ತಾಂಶ.m_isNull = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isString()) {
            ಆದರೆ (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_ದತ್ತಾಂಶ.str() = lhs.ಗಮ್ಯ_String();
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_ದತ್ತಾಂಶ.str() = lhs.m_ದತ್ತಾಂಶ.str();
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            ಗಣ_Zero();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs.isDouble()) {
            ಗಣ_Double(lhs.ಗಮ್ಯ_Double());
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ignoreXZ) {
            ಗಣ_Zero();
            copy_ಬಿಟ್ಟುಗಳು(0, lhs, 0, std::min(ಅಗಲ(), lhs.ಅಗಲ()));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಶಬ್ದ = 0; ಶಬ್ದ < ಪದಗಳು(); ++ಶಬ್ದ) {
                ಸ್ಥಿರಾಂಕ uint32_t ಮೌಲ್ಯ = ಶಬ್ದ < lhs.ಪದಗಳು() ? lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ].m_ಮೌಲ್ಯ
                                                                & ~lhs.m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ].m_ಮೌಲ್ಯ_X
                                                          : 0;
                m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಶಬ್ದ] = {ಮೌಲ್ಯ, 0};
            }
            opCleanThis();
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNToI(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    // String to packed number
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಗಣ_Zero();
    ಸ್ಥಿರಾಂಕ string& str = lhs.ಗಮ್ಯ_String();
    ಗಾಗಿ (ಗಾತ್ರ_t n = 0; n < str.ಉದ್ದಳತೆ(); ++n) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c = str[str.ಉದ್ದಳತೆ() - 1 - n];
        ಗಾಗಿ (ಗಾತ್ರ_t cbit = 0; cbit < 8; ++cbit) {
            ಸ್ಥಿರಾಂಕ std::bitset<8> sbit{static_cast<ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ>(c)};
            ಗಣ_ಬಿಟ್(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opExtendS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ copy_ಅಗಲ = std::min(ಅಗಲ(), static_cast<ಪೂರ್ಣಾಂಕ>(lbits));
    copy_ಬಿಟ್ಟುಗಳು(0, lhs, 0, copy_ಅಗಲ);
    fill_ಬಿಟ್ಟುಗಳು(copy_ಅಗಲ, ಅಗಲ() - copy_ಅಗಲ, lhs.ಬಿಟ್_Is(lbits - 1));
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opExtendXZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ copy_ಅಗಲ = std::min(ಅಗಲ(), static_cast<ಪೂರ್ಣಾಂಕ>(lbits));
    copy_ಬಿಟ್ಟುಗಳು(0, lhs, 0, copy_ಅಗಲ);
    fill_ಬಿಟ್ಟುಗಳು(copy_ಅಗಲ, ಅಗಲ() - copy_ಅಗಲ, lhs.ಬಿಟ್_Is(lbits - 1));
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opClean(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t ಬಿಟ್ಟುಗಳು) { ಹಿಂತಿರುಗಿ opSel(lhs, ಬಿಟ್ಟುಗಳು - 1, 0); }

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::opCleanThis(ತಾರ್ಕಿಕ warnOnTruncation) {
    // Clean MSB of number
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(*ಇದು);
    ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಪದಗಳು() - 1];
    ಸ್ಥಿರಾಂಕ uint32_t new_ಮೌಲ್ಯ_Msb = v.m_ಮೌಲ್ಯ & hi_ಶಬ್ದ_ಮುಖವಾಡ();
    ಸ್ಥಿರಾಂಕ uint32_t new_ಮೌಲ್ಯ_XMsb = v.m_ಮೌಲ್ಯ_X & hi_ಶಬ್ದ_ಮುಖವಾಡ();
    ಆದರೆ (warnOnTruncation && (new_ಮೌಲ್ಯ_Msb != v.m_ಮೌಲ್ಯ || new_ಮೌಲ್ಯ_XMsb != v.m_ಮೌಲ್ಯ_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << ಅಗಲ() << " bit number: " << displayed("%d"));
    }
    m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಪದಗಳು() - 1] = {new_ಮೌಲ್ಯ_Msb, new_ಮೌಲ್ಯ_XMsb};
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSel(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& msb, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lsb) {
    ಅಂಕೆ_ASSERT_OP_ARGS3(lhs, msb, lsb);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS2(msb, lsb);
    ಆದರೆ (lsb.isFourState() || msb.isFourState()) ಹಿಂತಿರುಗಿ ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    ಹಿಂತಿರುಗಿ opSel(lhs, msb.ಗಮ್ಯ_UInt(), lsb.ಗಮ್ಯ_UInt());
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSel(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t msbval, uint32_t lsbval) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    ಗಣ_Zero();
    ಪೂರ್ಣಾಂಕ copy_ಅಗಲ = 0;
    ಆದರೆ (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.ಅಗಲ())) {
        ಸ್ಥಿರಾಂಕ uint64_t selected_ಅಗಲ = uint64_t{msbval} - lsbval + 1;
        copy_ಅಗಲ = static_cast<ಪೂರ್ಣಾಂಕ>(
            std::min({static_cast<uint64_t>(ಅಗಲ()), static_cast<uint64_t>(lhs.ಅಗಲ()) - lsbval,
                      selected_ಅಗಲ}));
    }
    copy_ಬಿಟ್ಟುಗಳು(0, lhs, copy_ಅಗಲ ? static_cast<ಪೂರ್ಣಾಂಕ>(lsbval) : 0, copy_ಅಗಲ);
    fill_ಬಿಟ್ಟುಗಳು(copy_ಅಗಲ, ಅಗಲ() - copy_ಅಗಲ, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ಹಿಂತಿರುಗಿ *ಇದು;
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSelInto(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lsb, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
    ಹಿಂತಿರುಗಿ opSelInto(lhs, lsb.ಗಮ್ಯ_SInt(), ಅಗಲ);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSelInto(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಂಕ lsbval, ಪೂರ್ಣಾಂಕ ಅಗಲ) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ copy_ಅಗಲ = std::max(0, std::min({ಅಗಲ, lhs.ಅಗಲ(), ಇದು->ಅಗಲ() - lsbval}));
    copy_ಬಿಟ್ಟುಗಳು(lsbval, lhs, 0, copy_ಅಗಲ);
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ fill_ಅಗಲ
        = std::max(0, std::min(ಅಗಲ - copy_ಅಗಲ, ಇದು->ಅಗಲ() - lsbval - copy_ಅಗಲ));
    fill_ಬಿಟ್ಟುಗಳು(lsbval + copy_ಅಗಲ, fill_ಅಗಲ, v3Global.constRemoveXs() ? 0 : 'x');
    ಹಿಂತಿರುಗಿ *ಇದು;
}

//======================================================================
// Ops - Floating point

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opIToRD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ತಾರ್ಕಿಕ isSigned) {
    // Correct number of zero bits/width matters
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_ಸಂಖ್ಯೆ noxz(lhs);
    noxz.op_ನಿಯೋಜಿಸು_Non_XZ(lhs);
    ದ್ವಿಗುಣ d = 0;
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ negate = isSigned && noxz.isNegative();
    ಆದರೆ (negate) {
        ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = noxz.ಅಗಲ() - 1; ಬಿಟ್_2 >= 0; ಬಿಟ್_2--) {
        // Some precision might be lost in this add, that's what we want
        ಆದರೆ (noxz.ಬಿಟ್_Is1(ಬಿಟ್_2)) d += exp2(ಬಿಟ್_2);
    }
    ಆದರೆ (negate) d = -d;
    ಹಿಂತಿರುಗಿ ಗಣ_Double(d);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRToIS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ v = VL_TRUNC(lhs.ಗಮ್ಯ_Double());
    ಸ್ಥಿರಾಂಕ int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ಹಿಂತಿರುಗಿ ಗಣ_Long_S(i);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opRToIRoundS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ v = VL_ಸುತ್ತು(lhs.ಗಮ್ಯ_Double());
    ಗಣ_Zero();
    ಒಕ್ಕೂಟ {
        ದ್ವಿಗುಣ d;
        uint64_t q;
    } u;
    u.d = v;
    (ಶೂನ್ಯ)u.d;

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಘಾತಾಂಕ = static_cast<ಪೂರ್ಣಾಂಕ>((u.q >> 52ULL) & VL_ಮುಖವಾಡ_Q(11)) - 1023;
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = ಘಾತಾಂಕ - 52;
    ಸ್ಥಿರಾಂಕ uint64_t mantissa = (u.q & VL_ಮುಖವಾಡ_Q(52)) | (1ULL << 52);
    ಆದರೆ (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಬಿಟ್_2 = 0; ಬಿಟ್_2 <= 52; ++ಬಿಟ್_2) {
            ಆದರೆ (mantissa & (1ULL << ಬಿಟ್_2)) {
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ outbit = ಬಿಟ್_2 + lsb;
                ಆದರೆ (outbit >= 0) ಗಣ_ಬಿಟ್(outbit, 1);
            }
        }
        ಆದರೆ (v < 0) {
            ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ noSign = *ಇದು;
            opNegate(noSign);
        }
    }
    ಹಿಂತಿರುಗಿ *ಇದು;
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.ಅಗಲ() == 64 && ಅಗಲ() == 64, "Real operation on wrong sized number");
    ಒಕ್ಕೂಟ {
        ದ್ವಿಗುಣ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.ಗಮ್ಯ_Double();
    ಹಿಂತಿರುಗಿ ಗಣ_Quad(u.m_v);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.ಅಗಲ() == 64 && ಅಗಲ() == 64, "Real operation on wrong sized number");
    ಒಕ್ಕೂಟ {
        ದ್ವಿಗುಣ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.ಗಮ್ಯ_UQuad();
    ಹಿಂತಿರುಗಿ ಗಣ_Double(u.m_d);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNegateD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS1(lhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Double(-lhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಕೂಡಿಸು_D(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Double(lhs.ಗಮ್ಯ_Double() + rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opSubD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Double(lhs.ಗಮ್ಯ_Double() - rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opMulD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Double(lhs.ಗಮ್ಯ_Double() * rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opDivD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Double(lhs.ಗಮ್ಯ_Double() / rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opPowD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Double(ಘಾತ(lhs.ಗಮ್ಯ_Double(), rhs.ಗಮ್ಯ_Double()));
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opEqD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_Double() == rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNeqD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_Double() != rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGtD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_Double() > rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGteD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_Double() >= rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLtD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_Double() < rhs.ಗಮ್ಯ_Double());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLteD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_Double() <= rhs.ಗಮ್ಯ_Double());
}

//======================================================================
// Ops - String

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opConcatN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_String(lhs.ಗಮ್ಯ_String() + rhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opReplN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_LOGIC_ARGS1(rhs);
    ಹಿಂತಿರುಗಿ opReplN(lhs, rhs.ಗಮ್ಯ_UInt());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opReplN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t rhsval) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    string ಹೊರಹೋಗು;
    ಹೊರಹೋಗು.reserve(lhs.ಗಮ್ಯ_String().ಉದ್ದಳತೆ() * rhsval);
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ times = 0; times < rhsval; ++times) ಹೊರಹೋಗು += lhs.ಗಮ್ಯ_String();
    ಹಿಂತಿರುಗಿ ಗಣ_String(ಹೊರಹೋಗು);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opCvtPackString(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಹಿಂತಿರುಗಿ ಗಣ_String(lhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಗಮ್ಯ_ಅಧೋ_N(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಸ್ಥಿರಾಂಕ std::string ಹೊರಹೋಗು = VString::downcase(lhs.ಗಮ್ಯ_String());
    ಹಿಂತಿರುಗಿ ಗಣ_String(ಹೊರಹೋಗು);
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::op_ಗಮ್ಯ_ಊರ್ಧ್ವ_N(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS1(lhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS1(lhs);
    ಸ್ಥಿರಾಂಕ std::string ಹೊರಹೋಗು = VString::upcase(lhs.ಗಮ್ಯ_String());
    ಹಿಂತಿರುಗಿ ಗಣ_String(ಹೊರಹೋಗು);
}

V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opEqN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_String() == rhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opNeqN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_String() != rhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGtN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_String() > rhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opGteN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_String() >= rhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLtN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_String() < rhs.ಗಮ್ಯ_String());
}
V3_ಸಂಖ್ಯೆ& V3_ಸಂಖ್ಯೆ::opLteN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) {
    ಅಂಕೆ_ASSERT_OP_ARGS2(lhs, rhs);
    ಅಂಕೆ_ASSERT_STRING_ARGS2(lhs, rhs);
    ಹಿಂತಿರುಗಿ ಗಣ_Single_ಬಿಟ್ಟುಗಳು(lhs.ಗಮ್ಯ_String() <= rhs.ಗಮ್ಯ_String());
}

//======================================================================

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::built_ಒಳ_Filename()};
    V3_ಸಂಖ್ಯೆ ಅಂಕೆ{fl, 32, 0};
    ಅಂಕೆ.selfTestThis();
}

ಶೂನ್ಯ V3_ಸಂಖ್ಯೆ::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(0, 0), ನಿಜ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(1e19, 1e19), ನಿಜ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(9, 0.0001), ಸುಳ್ಳು);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(1, 1 + std::numeric_limits<ದ್ವಿಗುಣ>::epsilon()), ನಿಜ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonEqual(0.009, 0.00899999999999999931998839741709), ನಿಜ);

    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(0), ನಿಜ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(1), ನಿಜ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(-1), ನಿಜ);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(1.0001), ಸುಳ್ಳು);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(0.9999), ಸುಳ್ಳು);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(-1.0001), ಸುಳ್ಳು);
    UASSERT_SELFTEST(V3_ಸಂಖ್ಯೆ::epsilonIntegral(-0.9999), ಸುಳ್ಳು);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
