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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Number.h"

#अन्तर्भाव "V3File.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <cerrno>
#अन्तर्भाव <cmath>
#अन्तर्भाव <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr पूर्णाङ्क MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define अंक_ASSERT_OP_ARGS1(arg1) \
    UASSERT((एतत् != &(arg1)), "Number operation called with same source and dest")
#define अंक_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2)), \
            "Number operation called with same source and dest")
#define अंक_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2) && एतत् != &(arg3)), \
            "Number operation called with same source and dest")
#define अंक_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2) && एतत् != &(arg3) && एतत् != &(arg4)), \
            "Number operation called with same source and dest")

#define अंक_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_संख्या_Data::V3_संख्या_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define अंक_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    अंक_ASSERT_LOGIC_ARGS1(arg1); \
    अंक_ASSERT_LOGIC_ARGS1(arg2)

#define अंक_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    अंक_ASSERT_LOGIC_ARGS1(arg1); \
    अंक_ASSERT_LOGIC_ARGS1(arg2); \
    अंक_ASSERT_LOGIC_ARGS1(arg3); \
    अंक_ASSERT_LOGIC_ARGS1(arg4)

#define अंक_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define अंक_ASSERT_STRING_ARGS2(arg1, arg2) \
    अंक_ASSERT_STRING_ARGS1(arg1); \
    अंक_ASSERT_STRING_ARGS1(arg2)

#define अंक_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define अंक_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    अंक_ASSERT_DOUBLE_ARGS1(arg1); \
    अंक_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

शून्य V3_संख्या::v3errorEnd(ध्रुव std::ostringstream& str) ध्रुव VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    यदि (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } अन्यथा यदि (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } अन्यथा {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

शून्य V3_संख्या::v3errorEndFatal(ध्रुव std::ostringstream& str) ध्रुव
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_संख्या::V3_संख्या(Ast_गाँठ* nodep, VerilogStringLiteral, ध्रुव string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    यदि (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } अन्यथा {
        init(nodep, std::max<पूर्णाङ्क>(str.length() * 8, 1));
        प्रति (unsigned pos = 0; pos < str.length(); ++pos) {
            ध्रुव पूर्णाङ्क topos = str.length() - 1 - pos;
            मूल्य_And_X& v = m_data.अंक()[topos / 4];
            प्रति (पूर्णाङ्क bit = 0; bit < 8; ++bit) {
                यदि (str[pos] & (1UL << bit)) v.m_मूल्य |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = सत्य;
    opCleanThis(सत्य);
}

V3_संख्या::V3_संख्या(Ast_गाँठ* nodep, ध्रुव Ast_गाँठ_DType* nodedtypep) {
    यदि (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } अन्यथा यदि (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } अन्यथा यदि (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } अन्यथा {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

शून्य V3_संख्या::create(ध्रुव वर्ण* sourcep) {
    m_data.setLogic();
    ध्रुव वर्ण* मूल्य_startp = sourcep;
    प्रति (ध्रुव वर्ण* cp = sourcep; *cp; ++cp) {
        यदि (*cp == '\'') {
            मूल्य_startp = cp + 1;
            भङ्ग;
        }
    }

    तार्किक userSized = असत्य;
    तार्किक unbased = असत्य;
    वर्ण base = '\0';
    यदि (मूल्य_startp != sourcep) {  // Has a '
        string widthn;
        ध्रुव वर्ण* cp = sourcep;
        प्रति (; *cp; ++cp) {
            यदि (*cp == '\'') {
                ++cp;
                भङ्ग;
            }
            यदि (*cp != '_') widthn += *cp;
        }
        यावत् (*cp == '_') cp++;
        यदि (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(सत्य);
        }
        यदि (*cp) {
            base = *cp;
            ++cp;
        }
        मूल्य_startp = cp;

        userSized = widthn.length() != 0;
        यदि (std::atoi(widthn.c_str())) {
            यदि (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_अंक_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_अंक_Width(), सत्य);
            } अन्यथा {
                width(std::atoi(widthn.c_str()), सत्य);
            }
        }
    } अन्यथा {
        unbased = सत्य;
        base = 'd';
    }

    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) m_data.अंक()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    यदि (base == '0') {
        width(1, असत्य);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = सत्य;
    } अन्यथा यदि (base == '1') {
        width(1, असत्य);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = सत्य;
    } अन्यथा यदि (std::tolower(base) == 'z') {
        width(1, असत्य);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = सत्य;
    } अन्यथा यदि (std::tolower(base) == 'x') {
        width(1, असत्य);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = सत्य;
    }
    // Otherwise...
    अन्यथा यदि (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ध्रुव पूर्णाङ्क widthBound = std::max<पूर्णाङ्क>(32, std::strlen(मूल्य_startp) * 4);
        width(widthBound, असत्य);  // Will change width below
        यदि (unbased) isSigned(सत्य);  // Also says the spec.
    }

    // Ignore leading blanks
    यावत् (*मूल्य_startp == '_' || std::isspace(*मूल्य_startp)) ++मूल्य_startp;
    यदि (!*मूल्य_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    यदि (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    पूर्णाङ्क obit = 0;  // Start at LSB
    पूर्णाङ्क base_align = 1;
    यदि (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        यावत् (*मूल्य_startp == '_' || *मूल्य_startp == '0') ++मूल्य_startp;
        // Convert decimal number to hex
        पूर्णाङ्क olen = 0;
        uint32_t val = 0;
        पूर्णाङ्क got_x = 0;
        पूर्णाङ्क got_z = 0;
        पूर्णाङ्क got_01 = 0;
        तार्किक warnedTooMany = असत्य;
        प्रति (ध्रुव वर्ण* cp = मूल्य_startp; *cp; ++cp) {
            विकल्प (std::tolower(*cp)) {
            पक्ष '0':  // FALLTHRU
            पक्ष '1':  // FALLTHRU
            पक्ष '2':  // FALLTHRU
            पक्ष '3':  // FALLTHRU
            पक्ष '4':  // FALLTHRU
            पक्ष '5':  // FALLTHRU
            पक्ष '6':  // FALLTHRU
            पक्ष '7':  // FALLTHRU
            पक्ष '8':  // FALLTHRU
            पक्ष '9': {
                यदि (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.अंक()[0].m_मूल्य = val;
                } अन्यथा {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_संख्या product(एतत्, width() + 4);  // +4 for overflow detection
                    ध्रुव V3_संख्या ten(एतत्, width() + 4, 10);
                    ध्रुव V3_संख्या addend(एतत्, width(), (*cp - '0'));
                    product.opMul(*एतत्, ten);
                    opAdd(product, addend);
                    यदि (!warnedTooMany && product.bits_मूल्य(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = सत्य;
                    }
                }
                ++olen;
                got_01 = 1;
                भङ्ग;
            }
            पक्ष 'z':
            पक्ष '?': {
                got_z = 1;
                यदि (!userSized) width(32, असत्य);
                setAllBitsZ();
                भङ्ग;
            }
            पक्ष 'x': {
                got_x = 1;
                यदि (!userSized) width(32, असत्य);
                setAllBitsX();
                भङ्ग;
            }
            पक्ष '_': भङ्ग;
            स्वभाव: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                भङ्ग;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        यदि ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << मूल्य_startp);
        }
    } अन्यथा {
        // Convert bin/octal number to hex
        प्रति (ध्रुव वर्ण* cp = मूल्य_startp + std::strlen(मूल्य_startp) - 1; cp >= मूल्य_startp;
             cp--) {
            यदि (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                भङ्ग;
            }
            विकल्प (std::tolower(base)) {
            पक्ष 'b': {
                base_align = 1;
                विकल्प (std::tolower(*cp)) {
                पक्ष '0': setBit(obit++, 0); भङ्ग;
                पक्ष '1': setBit(obit++, 1); भङ्ग;
                पक्ष 'z':
                पक्ष '?': setBit(obit++, 'z'); भङ्ग;
                पक्ष 'x': setBit(obit++, 'x'); भङ्ग;
                पक्ष '_': भङ्ग;
                स्वभाव: v3error("Illegal character in binary constant: " << *cp);
                }
                भङ्ग;
            }

            पक्ष 'o':
            पक्ष 'c': {
                base_align = 3;
                विकल्प (std::tolower(*cp)) {  // clang-format off
                पक्ष '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  भङ्ग;
                पक्ष '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  भङ्ग;
                पक्ष '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  भङ्ग;
                पक्ष '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  भङ्ग;
                पक्ष '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  भङ्ग;
                पक्ष '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  भङ्ग;
                पक्ष '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  भङ्ग;
                पक्ष '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  भङ्ग;
                पक्ष 'z':
                पक्ष '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); भङ्ग;
                पक्ष 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); भङ्ग;
                पक्ष '_': भङ्ग;
                // clang-format on
                स्वभाव: v3error("Illegal character in octal constant");
                }
                भङ्ग;
            }

            पक्ष 'h':  // FALLTHRU
            पक्ष 'x': {
                base_align = 4;
                विकल्प (std::tolower(*cp)) {  // clang-format off
                पक्ष '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); भङ्ग;
                पक्ष '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); भङ्ग;
                पक्ष '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); भङ्ग;
                पक्ष '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); भङ्ग;
                पक्ष '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); भङ्ग;
                पक्ष '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); भङ्ग;
                पक्ष '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); भङ्ग;
                पक्ष '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); भङ्ग;
                पक्ष '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); भङ्ग;
                पक्ष '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); भङ्ग;
                पक्ष 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); भङ्ग;
                पक्ष 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); भङ्ग;
                पक्ष 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); भङ्ग;
                पक्ष 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); भङ्ग;
                पक्ष 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); भङ्ग;
                पक्ष 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); भङ्ग;
                पक्ष 'z':
                पक्ष '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); भङ्ग;
                पक्ष 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); भङ्ग;
                    // clang-format on
                पक्ष '_': भङ्ग;
                स्वभाव:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    भङ्ग;
                }  // LCOV_EXCL_STOP
                भङ्ग;
            }
            स्वभाव:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                भङ्ग;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    यदि (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), असत्य);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    यावत् (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(सत्य);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

शून्य V3_संख्या::warnTooMany(ध्रुव string& मूल्य) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << मूल्य << "'");
}

शून्य V3_संख्या::nodep(Ast_गाँठ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    यदि (!nodep) प्रत्यागम;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

तार्किक V3_संख्या::epsilonEqual(द्विगुण a, द्विगुण b) {
    प्रत्यागम std::fabs(a - b)
           <= (std::numeric_limits<द्विगुण>::epsilon() * std::max(1.0, std::max(a, b)));
}

तार्किक V3_संख्या::epsilonIntegral(द्विगुण a) {
    ध्रुव द्विगुण dint = static_cast<द्विगुण>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    प्रत्यागम epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

पूर्णाङ्क V3_संख्या::log2b(uint32_t अंक) {
    // See also opCLog2
    प्रति (पूर्णाङ्क bit = 31; bit > 0; bit--) {
        यदि (अंक & (1ULL << bit)) प्रत्यागम bit;
    }
    प्रत्यागम 0;
}

पूर्णाङ्क V3_संख्या::log2bQuad(uint64_t अंक) {
    यदि (अंक >> 32ULL) प्रत्यागम 32 + log2b(अंक >> 32ULL);
    प्रत्यागम log2b(अंक);
}

//======================================================================
// Setters

V3_संख्या& V3_संख्या::setZero() {
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) m_data.अंक()[i] = {0, 0};
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setQuad(uint64_t मूल्य) {
    setZero();
    m_data.अंक()[0].m_मूल्य = मूल्य & 0xffffffffULL;
    यदि (width() > 32) m_data.अंक()[1].m_मूल्य = (मूल्य >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setLong(uint32_t मूल्य) {
    setZero();
    m_data.अंक()[0].m_मूल्य = मूल्य;
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setLongS(int32_t मूल्य) {
    setZero();
    सङ्घ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = मूल्य;
    (शून्य)u.s;
    m_data.अंक()[0].m_मूल्य = u.u;
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setDouble(द्विगुण मूल्य) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    सङ्घ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.d = मूल्य;
    (शून्य)u.d;
    प्रति (पूर्णाङ्क i = 2; i < शब्दगण(); ++i) m_data.अंक()[i] = {0, 0};
    m_data.अंक()[0].m_मूल्य = u.u[0];
    m_data.अंक()[1].m_मूल्य = u.u[1];
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setSingleBits(वर्ण मूल्य) {
    प्रति (पूर्णाङ्क i = 1 /*upper*/; i < शब्दगण(); ++i) m_data.अंक()[i] = {0, 0};
    m_data.अंक()[0] = {(मूल्य == '1' || मूल्य == 'x' || मूल्य == 1 || मूल्य == 3),
                       (मूल्य == 'z' || मूल्य == 'x' || मूल्य == 2 || मूल्य == 3)};
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::setAllBits0() {
    setZero();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setAllBits1() {
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) m_data.अंक()[i] = {~0U, 0};
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) m_data.अंक()[i] = {~0U, ~0U};
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setAllBitsZ() {
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) m_data.अंक()[i] = {0, ~0U};
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::setAllBitsXRemoved() {
    यदि (!v3Global.constRemoveXs()) {
        प्रत्यागम setAllBitsX();
    } अन्यथा {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        यदि (v3Global.opt.xAssign() == "1") {
            प्रत्यागम setAllBits1();
        } अन्यथा {
            प्रत्यागम setAllBits0();
        }
    }
}
V3_संख्या& V3_संख्या::setValue1() {
    m_data.अंक()[0] = {1, 0};
    प्रति (पूर्णाङ्क i = 1; i < शब्दगण(); ++i) m_data.अंक()[i] = {0, 0};
    प्रत्यागम *एतत्;
}

शून्य V3_संख्या::setBitX0(पूर्णाङ्क bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_संख्या& V3_संख्या::setMask(पूर्णाङ्क nbits, पूर्णाङ्क lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    प्रत्यागम *एतत्;
}

//======================================================================
// ACCESSORS - as strings

string V3_संख्या::ascii(तार्किक prefixed, तार्किक cleanVerilog) ध्रुव VL_MT_STABLE {
    // Correct number of zero bits/width matters
    यदि (is1Step()) प्रत्यागम "1step";
    यदि (isNull()) प्रत्यागम "null";
    std::ostringstream out;
    यदि (isDouble()) {
        out.precision(17);
        यदि (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << toDouble();
            यदि (toDouble() == floor(toDouble())) out << ".0";
        }
        प्रत्यागम out.str();
    } अन्यथा यदि (isString()) {
        प्रत्यागम '"' + toString() + '"';
    } अन्यथा {
        यदि (VL_UNCOVERABLE((m_data.अंक()[शब्दगण() - 1].m_मूल्य | m_data.अंक()[शब्दगण() - 1].m_मूल्य_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    यदि (prefixed) {
        यदि (sized()) {
            out << width() << "'";
        } अन्यथा यदि (autoExtend() && width() == 1) {
            out << "'";
            यदि (bitIs0(0)) {
                out << '0';
            } अन्यथा यदि (bitIs1(0)) {
                out << '1';
            } अन्यथा यदि (bitIsZ(0)) {
                out << 'z';
            } अन्यथा {
                out << 'x';
            }
            प्रत्यागम out.str();
        } अन्यथा {
            यदि (cleanVerilog) {
                out << "'";
            } अन्यथा {
                out << "?" << width() << "?";
            }
        }
        यदि (isSigned()) out << "s";
    }

    ध्रुव तार्किक द्विआधारी = (isFourState()
#ifdef V3_संख्या_ASCII_द्विआधारी
                         // cppcheck-suppress konwnConditionTrueFalse
                         || सत्य
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    यदि (द्विआधारी) {
        out << "b";
        out << displayed("%0b");
    } अन्यथा {
        यदि (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    यदि (isNull()) {
        यदि (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << " [null]";
        }
    }
    प्रत्यागम out.str();
}

तार्किक V3_संख्या::displayedFmtHasArg(वर्ण format, तार्किक isScan) {
    // Is this a format letter that takes an argument in runtime?
    (शून्य)isScan;
    विकल्प (std::tolower(format)) {
    पक्ष 'b': प्रत्यागम सत्य;  //द्विआधारी
    पक्ष 'c': प्रत्यागम सत्य;  // Character
    पक्ष 'd': प्रत्यागम सत्य;  // Decimal
    पक्ष 'e': प्रत्यागम सत्य;  // Floating
    पक्ष 'f': प्रत्यागम सत्य;  // Floating
    पक्ष 'g': प्रत्यागम सत्य;  // Floating
    पक्ष 'h': प्रत्यागम सत्य;  // Hex
    पक्ष 'o': प्रत्यागम सत्य;  // Octal
    पक्ष 'p': प्रत्यागम सत्य;  // Pattern
    पक्ष 's': प्रत्यागम सत्य;  // String
    पक्ष 't': प्रत्यागम सत्य;  // Time
    पक्ष 'u': प्रत्यागम सत्य;  // Packed 2-state
    पक्ष 'v': प्रत्यागम सत्य;  // Strength
    पक्ष 'x': प्रत्यागम सत्य;  // Hex
    पक्ष 'z': प्रत्यागम सत्य;  // Packed 4-state
    पक्ष '?': प्रत्यागम सत्य;  // Internal: Compute format in V3Width based on data type
    स्वभाव: प्रत्यागम असत्य;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_संख्या::displayPad(size_t fmtsize, वर्ण pad, तार्किक बायाँ, ध्रुव string& in) VL_PURE {
    string padding;
    यदि (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    प्रत्यागम बायाँ ? (in + padding) : (padding + in);
}

string V3_संख्या::displayedEnumName(ध्रुव AstEnumItem* itemp) {
    ध्रुव string name = VIdProtect::protect(itemp->prettyName());
    प्रत्यागम VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned वर्ण>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_संख्या::displayedEnumName(ध्रुव AstEnumDType* dtypep) ध्रुव VL_MT_STABLE {
    V3_संख्या मूल्य{एतत्, dtypep->width()};
    मूल्य.opAssign(*एतत्);
    प्रति (ध्रुव AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
        यदि (!constp->अंक().isAnyXZ() && मूल्य.isCaseEq(constp->अंक()))
            प्रत्यागम displayedEnumName(itemp);
    }
    प्रत्यागम "";
}

string V3_संख्या::displayedEnum(ध्रुव AstSFormatArg* argp,
                               ध्रुव string& vformat) ध्रुव VL_MT_STABLE {
    ध्रुव AstEnumDType* ध्रुव dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_संख्या मूल्य{एतत्, dtypep->width()};
    मूल्य.opAssign(*एतत्);
    ध्रुव VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ध्रुव वर्ण fmt = std::tolower(vformat.back());
    यदि (fmt != 'p' && fmt != 's') प्रत्यागम मूल्य.displayed(argp, vformat, numericAttr);
    string name = मूल्य.displayedEnumName(dtypep);
    यदि (name.empty()) name = मूल्य.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_संख्या name_अंक{एतत्};
    name_अंक.setString(name);
    प्रत्यागम name_अंक.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_संख्या::displayed(ध्रुव Ast_गाँठ* nodep, ध्रुव string& vformat,
                           ध्रुव VFormatAttr& formatAttr) ध्रुव VL_MT_STABLE {
    प्रत्यागम displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_संख्या::displayed(File_पंक्ति* fl, ध्रुव string& vformat,
                           ध्रुव VFormatAttr& formatAttr) ध्रुव VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *एतत्);
    ++pos;
    तार्किक बायाँ = असत्य;
    यदि (pos[0] == '-') {
        बायाँ = सत्य;
        ++pos;
    }
    string fmtsize;
    प्रति (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    वर्ण fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    यदि (formatAttr.isComplex()) {
        यदि (fmt != 'p') fmt = 's';  // override
    } अन्यथा यदि (formatAttr.isString()) {
        यदि (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_संख्या strwide{fl, static_cast<पूर्णाङ्क>(toString().length()) * 8, 0};
            strwide.opNToI(*एतत्);
            प्रत्यागम strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        यदि (fmt != 'p') fmt = 's';  // override
    } अन्यथा यदि (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        यदि (fmt == 'p') {
            यदि (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } अन्यथा {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } अन्यथा यदि (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        यदि (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_संख्या nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*एतत्);
            प्रत्यागम nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    विकल्प (fmt) {
    पक्ष 'c': {
        यदि (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ध्रुव unsigned पूर्णाङ्क v = bits_मूल्य(0, 8);
        str.धकेलो_back(static_cast<वर्ण>(v));
        प्रत्यागम str;
    }
    पक्ष 'e':  // FALLTHRU
    पक्ष 'f':  // FALLTHRU
    पक्ष 'g': {
        ध्रुव द्विगुण n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        वर्ण tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        प्रत्यागम tmp;
    }
    पक्ष 's': {
        यदि (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } अन्यथा {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            पूर्णाङ्क bit = width() - 1;
            तार्किक start = सत्य;
            यावत् ((bit % 8) != 7) ++bit;
            प्रति (; bit >= 0; bit -= 8) {
                ध्रुव पूर्णाङ्क v = bits_मूल्य(bit - 7, 8);
                यदि (!start || v) {
                    str += static_cast<वर्ण>((v == 0) ? ' ' : v);
                    start = असत्य;  // Drop leading 0s
                } अन्यथा {
                    यदि (fmtsize != "0") str += ' ';
                }
            }
        }
        ध्रुव size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', बायाँ, str);
        प्रत्यागम str;
    }
    पक्ष 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        यदि (formatAttr.isDouble()) {
            ध्रुव द्विगुण n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            वर्ण tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            प्रत्यागम tmp;
        }
        यदि (formatAttr.isString())
            प्रत्यागम '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        यदि (formatAttr.isComplex()) प्रत्यागम toString();
        प्रत्यागम "%p";
    }
    पक्ष 'b':  // FALLTHRU
    पक्ष 'o':  // FALLTHRU
    पक्ष 'h':  // FALLTHRU
    पक्ष 'x': {
        पूर्णाङ्क bit = width() - 1;
        यदि (बायाँ || !fmtsize.empty()) {
            यावत् (bit && bitIs0(bit)) --bit;
        }
        विकल्प (fmt) {
        पक्ष 'b': {
            प्रति (; bit >= 0; --bit) {
                यदि (bitIs0(bit)) {
                    str += '0';
                } अन्यथा यदि (bitIs1(bit)) {
                    str += '1';
                } अन्यथा यदि (bitIsZ(bit)) {
                    str += 'z';
                } अन्यथा {
                    str += 'x';
                }
            }
            भङ्ग;
        }
        पक्ष 'o': {
            यावत् ((bit % 3) != 2) ++bit;
            प्रति (; bit > 0; bit -= 3) {
                ध्रुव पूर्णाङ्क अंक_X = गिनती_X(bit - 2, 3);
                ध्रुव पूर्णाङ्क अंक_Z = गिनती_Z(bit - 2, 3);
                यदि (अंक_X == 3 || अंक_X == width() - (bit - 2)) {
                    str += 'x';
                    अनुवर्त;
                }
                यदि (अंक_Z == 3 || अंक_Z == width() - (bit - 2)) {
                    str += 'z';
                    अनुवर्त;
                }
                यदि (अंक_X > 0) {
                    str += 'X';
                    अनुवर्त;
                }
                यदि (अंक_Z > 0) {
                    str += 'Z';
                    अनुवर्त;
                }
                ध्रुव पूर्णाङ्क v = bits_मूल्य(bit - 2, 3);
                str += static_cast<वर्ण>('0' + v);
            }
            भङ्ग;
        }
        स्वभाव: {  // h/x
            यावत् ((bit % 4) != 3) ++bit;
            प्रति (; bit > 0; bit -= 4) {
                ध्रुव पूर्णाङ्क अंक_X = गिनती_X(bit - 3, 4);
                ध्रुव पूर्णाङ्क अंक_Z = गिनती_Z(bit - 3, 4);
                यदि (अंक_X == 4 || अंक_X == width() - (bit - 3)) {
                    str += 'x';
                    अनुवर्त;
                }
                यदि (अंक_Z == 4 || अंक_Z == width() - (bit - 3)) {
                    str += 'z';
                    अनुवर्त;
                }
                यदि (अंक_X > 0) {
                    str += 'X';
                    अनुवर्त;
                }
                यदि (अंक_Z > 0) {
                    str += 'Z';
                    अनुवर्त;
                }
                ध्रुव पूर्णाङ्क v = bits_मूल्य(bit - 3, 4);
                यदि (v >= 10) {
                    str += static_cast<वर्ण>('a' + v - 10);
                } अन्यथा {
                    str += static_cast<वर्ण>('0' + v);
                }
            }
            भङ्ग;
        }
        }  // switch
        ध्रुव size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (बायाँ ? ' ' : '0'), बायाँ, str);
        प्रत्यागम str;
    }  // case b/d/x/o
    पक्ष 't':  // Time
    पक्ष 'd': {  // Unsigned decimal
        ध्रुव तार्किक issigned = formatAttr.isSigned();
        यदि (fmtsize == "" && !बायाँ) {
            ध्रुव द्विगुण mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            द्विगुण dchars = mantissabits / 3.321928094887362 + 1.0;
            यदि (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<पूर्णाङ्क>(dchars));
        }
        तार्किक hasXZ = असत्य;
        यदि (isAllX()) {
            str = "x";
            hasXZ = सत्य;
        } अन्यथा यदि (isAllZ()) {
            str = "z";
            hasXZ = सत्य;
        } अन्यथा यदि (isAnyX()) {
            str = "X";
            hasXZ = सत्य;
        } अन्यथा यदि (isAnyZ()) {
            str = "Z";
            hasXZ = सत्य;
        }
        यदि (!hasXZ) {
            यदि (issigned) {
                यदि (width() > 64) {
                    str = toDecimalS();
                } अन्यथा {
                    str = cvtToStr(toSQuad());
                }
            } अन्यथा {
                यदि (width() > 64) {
                    str = toDecimalU();
                } अन्यथा {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        ध्रुव तार्किक zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !बायाँ;
        // fmtsize might have changed since we parsed the %fmtsize
        ध्रुव size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), बायाँ, str);
        प्रत्यागम str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    पक्ष 'u': {  // Packed 2-state
        प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
            ध्रुव uint32_t v = m_data.अंक()[i].m_मूल्य;
            str += static_cast<वर्ण>((v >> 0) & 0xff);
            str += static_cast<वर्ण>((v >> 8) & 0xff);
            str += static_cast<वर्ण>((v >> 16) & 0xff);
            str += static_cast<वर्ण>((v >> 24) & 0xff);
        }
        प्रत्यागम str;
    }
    पक्ष 'z': {  // Packed 4-state
        प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
            ध्रुव मूल्य_And_X v = m_data.अंक()[i];
            str += static_cast<वर्ण>((v.m_मूल्य >> 0) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य >> 8) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य >> 16) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य >> 24) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य_X >> 0) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य_X >> 8) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य_X >> 16) & 0xff);
            str += static_cast<वर्ण>((v.m_मूल्य_X >> 24) & 0xff);
        }
        प्रत्यागम str;
    }
    पक्ष 'v': {  // Strength
        पूर्णाङ्क bit = width() - 1;
        प्रति (; bit >= 0; bit--) {
            यदि (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            अन्यथा यदि (bitIs1(bit)) {
                str += "St1 ";
            } अन्यथा यदि (bitIsZ(bit)) {
                str += "StZ ";
            } अन्यथा {
                str += "StX";
            }
        }
        प्रत्यागम str;
    }
    स्वभाव: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_संख्या::emitC() ध्रुव VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    वर्ण sbuf[bufsize];
    string result;
    यदि (isNull()) {
        प्रत्यागम "VlNull{}";
    } अन्यथा यदि (isDouble()) {
        ध्रुव द्विगुण dnum = toDouble();
        यदि (std::isinf(dnum)) {
            यदि (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } अन्यथा यदि (std::isnan(dnum)) {
            यदि (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } अन्यथा {
            ध्रुव वर्ण* ध्रुव fmt = (static_cast<पूर्णाङ्क>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            प्रत्यागम sbuf;
        }
    } अन्यथा यदि (isString()) {
        ध्रुव string strg = toString();
        ध्रुव string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        प्रत्यागम '"' + quoted + "\"s";
    } अन्यथा यदि (शब्दगण() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(शब्दगण()) + ">{{";
        यदि (शब्दगण() > 4) result += '\n';
        प्रति (पूर्णाङ्क n = 0; n < शब्दगण(); ++n) {
            यदि (n) result += ((n % 4) ? ", " : ",\n");
            (शून्य)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        यदि (शब्दगण() > 4) result += '\n';
        result += "}}";
    } अन्यथा यदि (शब्दगण() == 2) {  // Quad
        ध्रुव uint64_t qnum = static_cast<uint64_t>(toUQuad());
        ध्रुव वर्ण* ध्रुव fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        प्रत्यागम sbuf;
    } अन्यथा {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ध्रुव uint32_t unum = toUInt();
        ध्रुव वर्ण* ध्रुव fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        प्रत्यागम sbuf;
    }
    प्रत्यागम result;
}

string V3_संख्या::toDecimalS() ध्रुव VL_MT_STABLE {
    // Correct number of zero bits/width matters
    यदि (isNegative()) {
        V3_संख्या lhsNoSign = *एतत्;
        lhsNoSign.opNegate(*एतत्);
        प्रत्यागम "-"s + lhsNoSign.toDecimalU();
    } अन्यथा {
        प्रत्यागम toDecimalU();
    }
}

string V3_संख्या::toDecimalU() ध्रुव VL_MT_STABLE {
    ध्रुव पूर्णाङ्क maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_संख्या bcd(एतत्, maxdecwidth + 4);
    V3_संख्या tmp(एतत्, maxdecwidth + 4);
    V3_संख्या tmp2(एतत्, maxdecwidth + 4);

    पूर्णाङ्क from_bit = width() - 1;
    // Skip all leading zeros
    प्रति (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    प्रति (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        प्रति (पूर्णाङ्क nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            यदि (bcd.bits_मूल्य(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_संख्या(एतत्, 32, 1));
        // bcd[0] = this[from_bit]
        यदि (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    पूर्णाङ्क lsb = (maxdecwidth - 1) & ~3;
    प्रति (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        यदि (bcd.bits_मूल्य(lsb, 4)) भङ्ग;
    }
    प्रति (; lsb >= 0; lsb -= 4) {
        output += static_cast<वर्ण>('0' + bcd.bits_मूल्य(lsb, 4));  // 0..9
    }
    प्रत्यागम output;
}

string V3_संख्या::toRealString() ध्रुव VL_MT_STABLE {
    ध्रुव द्विगुण मूल्य = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << मूल्य;
    string result = out.str();
    यदि (std::strtod(result.c_str(), nullptr) != मूल्य) {
        out.str("");
        out.precision(17);
        out << मूल्य;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    यदि (result.find_first_of(".en") == string::npos) result += ".0";
    प्रत्यागम result;
}

//======================================================================
// ACCESSORS - as numbers

तार्किक V3_संख्या::fitsInUInt() ध्रुव VL_MT_SAFE {
    यदि (isFourState()) प्रत्यागम असत्य;
    // We allow wide numbers that represent values <= 32 bits
    प्रति (पूर्णाङ्क i = 1; i < शब्दगण(); ++i) {
        यदि (m_data.अंक()[i].m_मूल्य) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
uint32_t V3_संख्या::toUInt() ध्रुव VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *एतत्);
    // We allow wide numbers that represent values <= 32 bits
    यदि (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *एतत्);
    }
    प्रत्यागम m_data.अंक()[0].m_मूल्य;
}

द्विगुण V3_संख्या::toDouble() ध्रुव VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    सङ्घ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.अंक()[0].m_मूल्य;
    u.u[1] = m_data.अंक()[1].m_मूल्य;
    प्रत्यागम u.d;
}

int32_t V3_संख्या::toSInt() ध्रुव VL_MT_SAFE {
    यदि (isSigned()) {
        ध्रुव uint32_t v = toUInt();
        ध्रुव uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        ध्रुव uint32_t extended = v | signExtend;
        प्रत्यागम static_cast<int32_t>(extended);
    } अन्यथा {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        प्रत्यागम static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_संख्या::toUQuad() ध्रुव VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *एतत्);
    // We allow wide numbers that represent values <= 64 bits
    यदि (isDouble()) प्रत्यागम static_cast<uint64_t>(toDouble());
    प्रति (पूर्णाङ्क i = 2; i < शब्दगण(); ++i) {
        यदि (m_data.अंक()[i].m_मूल्य) {
            v3error("Value too wide for 64-bits expected in this context " << *एतत्);
            भङ्ग;
        }
    }
    यदि (width() <= 32) प्रत्यागम (static_cast<uint64_t>(toUInt()));
    प्रत्यागम ((static_cast<uint64_t>(m_data.अंक()[1].m_मूल्य) << 32ULL)
            | (static_cast<uint64_t>(m_data.अंक()[0].m_मूल्य)));
}

int64_t V3_संख्या::toSQuad() ध्रुव {
    यदि (isDouble()) प्रत्यागम static_cast<int64_t>(toDouble());
    ध्रुव uint64_t v = toUQuad();
    ध्रुव uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    ध्रुव uint64_t extended = v | signExtend;
    प्रत्यागम static_cast<int64_t>(extended);
}

string V3_संख्या::toString() ध्रुव {
    UASSERT(!isFourState(), "toString with 4-state " << *एतत्);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    यदि (isString()) प्रत्यागम m_data.str();
    पूर्णाङ्क bit = width() - 1;
    यावत् ((bit % 8) != 7) ++bit;
    std::string str;
    प्रति (; bit >= 0; bit -= 8) {
        ध्रुव पूर्णाङ्क v = bits_मूल्य(bit - 7, 8);
        यदि (v) str += static_cast<वर्ण>(v);
    }
    प्रत्यागम str;
}

uint32_t V3_संख्या::edataWord(पूर्णाङ्क eword) ध्रुव {
    UASSERT(!isFourState(), "edataWord with 4-state " << *एतत्);
    प्रत्यागम m_data.अंक()[eword].m_मूल्य;
}

uint8_t V3_संख्या::dataByte(पूर्णाङ्क byte) ध्रुव {
    प्रत्यागम (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

तार्किक V3_संख्या::isAllZ() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        ध्रुव uint32_t mask = i == शब्दगण() - 1 ? hiWordMask() : ~0U;
        यदि (((~v.m_मूल्य & v.m_मूल्य_X) & mask) != mask) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_संख्या::isAllX() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    uint32_t mask = hiWordMask();
    प्रति (पूर्णाङ्क i = शब्दगण() - 1; i >= 0; --i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        यदि ((v.m_मूल्य & v.m_मूल्य_X) ^ mask) प्रत्यागम असत्य;
        mask = ~0U;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_संख्या::isEqZero() ध्रुव VL_MT_SAFE {
    यदि (isString()) प्रत्यागम m_data.str().empty();
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        यदि (v.m_मूल्य || v.m_मूल्य_X) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_संख्या::isNeqZero() ध्रुव {
    यदि (isString()) प्रत्यागम !m_data.str().empty();
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        यदि (v.m_मूल्य & ~v.m_मूल्य_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_संख्या::isBitsZero(पूर्णाङ्क msb, पूर्णाङ्क lsb) ध्रुव {
    प्रति (पूर्णाङ्क i = lsb; i <= msb; ++i) {
        यदि (VL_UNLIKELY(!bitIs0(i))) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_संख्या::isEqOne() ध्रुव {
    यदि (m_data.अंक()[0].m_मूल्य != 1 || m_data.अंक()[0].m_मूल्य_X) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 1; i < शब्दगण(); ++i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        यदि (v.m_मूल्य || v.m_मूल्य_X) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_संख्या::isEqAllOnes(पूर्णाङ्क optwidth) ध्रुव {
    // Correct number of zero bits/width matters
    यदि (!optwidth) optwidth = width();
    प्रति (पूर्णाङ्क bit = 0; bit < optwidth; ++bit) {
        यदि (!bitIs1(bit)) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_संख्या::isFourState() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        यदि (m_data.अंक()[i].m_मूल्य_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_संख्या::isAnyX() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        यदि (v.m_मूल्य & v.m_मूल्य_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_संख्या::isAnyXZ() ध्रुव {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        यदि (m_data.अंक()[i].m_मूल्य_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_संख्या::isAnyZ() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        ध्रुव मूल्य_And_X v = m_data.अंक()[i];
        यदि (~v.m_मूल्य & v.m_मूल्य_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_संख्या::isLtXZ(ध्रुव V3_संख्या& rhs) ध्रुव {
    // Include X/Z in comparisons for sort ordering
    यदि (isString()) प्रत्यागम toString() < rhs.toString();
    यदि (isDouble()) प्रत्यागम toDouble() < rhs.toDouble();
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        यदि (bitIs1(bit) && rhs.bitIs0(bit)) प्रत्यागम सत्य;
        यदि (rhs.bitIs1(bit) && bitIs0(bit)) प्रत्यागम असत्य;
        यदि (bitIsXZ(bit)) प्रत्यागम सत्य;
        यदि (rhs.bitIsXZ(bit)) प्रत्यागम असत्य;
    }
    प्रत्यागम असत्य;
}
पूर्णाङ्क V3_संख्या::गिनती_X(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE {
    पूर्णाङ्क गिनती = 0;
    प्रति (पूर्णाङ्क bitn = 0; bitn < nbits; ++bitn) {
        यदि (lsb + bitn >= width()) प्रत्यागम गिनती;
        यदि (bitIsX(lsb + bitn)) ++गिनती;
    }
    प्रत्यागम गिनती;
}
पूर्णाङ्क V3_संख्या::गिनती_Z(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE {
    पूर्णाङ्क गिनती = 0;
    प्रति (पूर्णाङ्क bitn = 0; bitn < nbits; ++bitn) {
        यदि (lsb + bitn >= width()) प्रत्यागम गिनती;
        यदि (bitIsZ(lsb + bitn)) ++गिनती;
    }
    प्रत्यागम गिनती;
}

पूर्णाङ्क V3_संख्या::widthToFit() ध्रुव {
    प्रति (पूर्णाङ्क bit = width() - 1; bit > 0; bit--) {
        यदि (!bitIs0(bit)) प्रत्यागम bit + 1;
    }
    प्रत्यागम 1;  // one bit even if number is == 0
}

uint32_t V3_संख्या::गिनती_Bits(ध्रुव V3_संख्या& ctrl) ध्रुव {
    // Correct number of zero bits/width matters
    पूर्णाङ्क n = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        विकल्प (ctrl.bitIs(0)) {
        पक्ष '0':
            यदि (bitIs0(bit)) ++n;
            भङ्ग;
        पक्ष '1':
            यदि (bitIs1(bit)) ++n;
            भङ्ग;
        पक्ष 'x':
            यदि (bitIsX(bit)) ++n;
            भङ्ग;
        पक्ष 'z':
            यदि (bitIsZ(bit)) ++n;
            भङ्ग;
        }
    }
    प्रत्यागम n;
}

uint32_t V3_संख्या::गिनती_Bits(ध्रुव V3_संख्या& ctrl1, ध्रुव V3_संख्या& ctrl2,
                             ध्रुव V3_संख्या& ctrl3) ध्रुव {
    // Correct number of zero bits/width matters
    पूर्णाङ्क n = गिनती_Bits(ctrl1);
    यदि (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += गिनती_Bits(ctrl2);
    यदि ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += गिनती_Bits(ctrl3);
    }
    प्रत्यागम n;
}

uint32_t V3_संख्या::गिनती_Ones() ध्रुव {
    पूर्णाङ्क n = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (bitIs1(bit)) ++n;
    }
    प्रत्यागम n;
}

uint32_t V3_संख्या::mostSetBitP1() ध्रुव {
    प्रति (पूर्णाङ्क bit = width() - 1; bit >= 0; bit--) {
        यदि (!bitIs0(bit)) प्रत्यागम bit + 1;
    }
    प्रत्यागम 0;
}

uint32_t V3_संख्या::leastSetBitP1() ध्रुव {
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (!bitIs0(bit)) प्रत्यागम bit + 1;
    }
    प्रत्यागम 0;
}

//======================================================================

V3_संख्या& V3_संख्या::opBitsNonXZ(ध्रुव V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opBitsOne(ध्रुव V3_संख्या& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opBitsXZ(ध्रुव V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opBitsZ(ध्रुव V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    प्रत्यागम *एतत्;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_संख्या& V3_संख्या::opRedOr(ध्रुव V3_संख्या& lhs) {
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            प्रत्यागम setSingleBits(1);
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opRedAnd(ध्रुव V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs0(bit)) {
            प्रत्यागम setSingleBits(0);
        } अन्यथा यदि (lhs.bitIs1(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opRedXor(ध्रुव V3_संख्या& lhs) {
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            यदि (outc == 1) {
                outc = 0;
            } अन्यथा यदि (outc == 0) {
                outc = 1;
            }
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::op_गिनती_Bits(ध्रुव V3_संख्या& व्यंजक, ध्रुव V3_संख्या& ctrl1, ध्रुव V3_संख्या& ctrl2,
                                ध्रुव V3_संख्या& ctrl3) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS4(व्यंजक, ctrl1, ctrl2, ctrl3);
    अंक_ASSERT_LOGIC_ARGS4(व्यंजक, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.अंक()[0].m_मूल्य = व्यंजक.गिनती_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::op_गिनती_Ones(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम setAllBitsX();
    setZero();
    m_data.अंक()[0].m_मूल्य = lhs.गिनती_Ones();
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opIsUnknown(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    प्रत्यागम setSingleBits(lhs.isAnyXZ());
}
V3_संख्या& V3_संख्या::opOneHot(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम setAllBitsX();
    प्रत्यागम setSingleBits(lhs.गिनती_Ones() == 1);
}
V3_संख्या& V3_संख्या::opOneHot0(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम setAllBitsX();
    प्रत्यागम setSingleBits(lhs.गिनती_Ones() <= 1);
}
V3_संख्या& V3_संख्या::opCLog2(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ध्रुव पूर्णाङ्क adjust = (lhs.गिनती_Ones() == 1) ? 0 : 1;
    प्रति (पूर्णाङ्क bit = lhs.width() - 1; bit >= 0; bit--) {
        यदि (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            प्रत्यागम *एतत्;
        }
    }
    setZero();
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opMostSetBitP1(ध्रुव V3_संख्या& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम setAllBitsX();
    प्रति (पूर्णाङ्क bit = lhs.width() - 1; bit >= 0; bit--) {
        यदि (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            प्रत्यागम *एतत्;
        }
    }
    setZero();
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opLogNot(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            outc = 0;
            गच्छ last;
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opNot(ध्रुव V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opAnd(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opOr(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opXor(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    प्रत्यागम *एतत्;
}

शून्य V3_संख्या::fillBits(पूर्णाङ्क destLsb, पूर्णाङ्क width, वर्ण मूल्य) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    यदि (!width) प्रत्यागम;
    UASSERT(width <= एतत्->width() - destLsb, "Destination bit range exceeds number width");

    ध्रुव uint32_t मूल्य_Word = मूल्य == '1' || मूल्य == 'x' || मूल्य == 1 || मूल्य == 3 ? ~0U : 0;
    ध्रुव uint32_t xWord = मूल्य == 'z' || मूल्य == 'x' || मूल्य == 2 || मूल्य == 3 ? ~0U : 0;
    ध्रुव पूर्णाङ्क firstWord = VL_BITWORD_E(destLsb);
    ध्रुव पूर्णाङ्क lastWord = VL_BITWORD_E(destLsb + width - 1);
    प्रति (पूर्णाङ्क word = firstWord; word <= lastWord; ++word) {
        ध्रुव पूर्णाङ्क wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ध्रुव पूर्णाङ्क wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        ध्रुव uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        मूल्य_And_X& dest = m_data.अंक()[word];
        dest.m_मूल्य = (dest.m_मूल्य & ~mask) | (मूल्य_Word & mask);
        dest.m_मूल्य_X = (dest.m_मूल्य_X & ~mask) | (xWord & mask);
    }
}

शून्य V3_संख्या::copyBits(पूर्णाङ्क destLsb, ध्रुव V3_संख्या& source, पूर्णाङ्क sourceLsb, पूर्णाङ्क width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    यदि (!width) प्रत्यागम;
    UASSERT(width <= एतत्->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    ध्रुव पूर्णाङ्क firstWord = VL_BITWORD_E(destLsb);
    ध्रुव पूर्णाङ्क lastWord = VL_BITWORD_E(destLsb + width - 1);
    प्रति (पूर्णाङ्क destWord = firstWord; destWord <= lastWord; ++destWord) {
        ध्रुव पूर्णाङ्क destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        ध्रुव पूर्णाङ्क destOffset = VL_BITBIT_E(destBit);
        ध्रुव पूर्णाङ्क sourceBit = sourceLsb + destBit - destLsb;
        ध्रुव पूर्णाङ्क sourceWord = VL_BITWORD_E(sourceBit);
        ध्रुव पूर्णाङ्क sourceOffset = VL_BITBIT_E(sourceBit);
        ध्रुव पूर्णाङ्क copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        ध्रुव uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        ध्रुव मूल्य_And_X& sourceLo = source.m_data.अंक()[sourceWord];
        uint32_t मूल्य = sourceLo.m_मूल्य >> sourceOffset;
        uint32_t मूल्य_X = sourceLo.m_मूल्य_X >> sourceOffset;
        यदि (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            ध्रुव मूल्य_And_X& sourceHi = source.m_data.अंक()[sourceWord + 1];
            मूल्य |= sourceHi.m_मूल्य << (VL_EDATASIZE - sourceOffset);
            मूल्य_X |= sourceHi.m_मूल्य_X << (VL_EDATASIZE - sourceOffset);
        }

        मूल्य_And_X& dest = m_data.अंक()[destWord];
        dest.m_मूल्य = (dest.m_मूल्य & ~mask) | ((मूल्य << destOffset) & mask);
        dest.m_मूल्य_X = (dest.m_मूल्य_X & ~mask) | ((मूल्य_X << destOffset) & mask);
    }
}

V3_संख्या& V3_संख्या::opConcat(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    यदि (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opLenN(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opRepl(ध्रुव V3_संख्या& lhs,
                           ध्रुव V3_संख्या& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    प्रत्यागम opRepl(lhs, rhs.toUInt());
}

V3_संख्या& V3_संख्या::opRepl(ध्रुव V3_संख्या& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    यदि (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    प्रति (unsigned times = 0; times < rhsval; ++times) {
        ध्रुव uint64_t destLsb = uint64_t{times} * lhs.width();
        यदि (destLsb >= static_cast<uint32_t>(width())) भङ्ग;
        copyBits(static_cast<पूर्णाङ्क>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<पूर्णाङ्क>(destLsb)));
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opStreamL(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    यदि (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ध्रुव पूर्णाङ्क ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    प्रति (पूर्णाङ्क istart = 0; istart < lhs.width(); istart += ssize) {
        ध्रुव पूर्णाङ्क ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opLogAnd(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण loutc = 0;
    वर्ण routc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            loutc = 1;
            भङ्ग;
        }
        यदि (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    प्रति (पूर्णाङ्क bit = 0; bit < rhs.width(); ++bit) {
        यदि (rhs.bitIs1(bit)) {
            routc = 1;
            भङ्ग;
        }
        यदि (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    वर्ण outc = 'x';
    यदि (routc == 1 && loutc == 1) outc = 1;
    यदि (routc == 0 || loutc == 0) outc = 0;
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opLogOr(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            outc = 1;
            गच्छ last;
        }
        यदि (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    प्रति (पूर्णाङ्क bit = 0; bit < rhs.width(); ++bit) {
        यदि (rhs.bitIs1(bit)) {
            outc = 1;
            गच्छ last;
        }
        यदि (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opLogIf(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या lnot = lhs;
    lnot.opLogNot(lhs);
    प्रत्यागम opLogOr(lnot, rhs);
}

V3_संख्या& V3_संख्या::opLogEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_संख्या ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    प्रत्यागम opLogAnd(ifa, ifb);
}

V3_संख्या& V3_संख्या::opAtoN(ध्रुव V3_संख्या& lhs, पूर्णाङ्क base) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    यदि (base == AstAtoN::ATOREAL) प्रत्यागम setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    यदि (errno != 0) v = 0;
    प्रत्यागम setLongS(static_cast<int32_t>(v));
}

V3_संख्या& V3_संख्या::opPutcN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, ध्रुव V3_संख्या& ths) {
    अंक_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अंक_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    ध्रुव int32_t i = rhs.toSInt();
    ध्रुव int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    यदि (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    प्रत्यागम setString(lstring);
}

V3_संख्या& V3_संख्या::opGetcN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    ध्रुव string lstring = lhs.toString();
    ध्रुव int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    यदि (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    प्रत्यागम setLong(v);
}

V3_संख्या& V3_संख्या::opSubstrN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, ध्रुव V3_संख्या& ths) {
    अंक_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अंक_ASSERT_STRING_ARGS1(lhs);
    ध्रुव string lstring = lhs.toString();
    ध्रुव int32_t i = rhs.toSInt();
    ध्रुव int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    यदि (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) प्रत्यागम setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    प्रत्यागम setString(lstring.substr(i, j - i + 1));
}

V3_संख्या& V3_संख्या::opCompareNN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, तार्किक ignoreCase) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    पूर्णाङ्क result;
    ध्रुव string lstring = lhs.toString();
    ध्रुव string rstring = rhs.toString();
    यदि (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } अन्यथा {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    प्रत्यागम setLongS(result);
}

V3_संख्या& V3_संख्या::opEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    यदि (lhs.isString()) प्रत्यागम opEqN(lhs, rhs);
    यदि (lhs.isDouble()) प्रत्यागम opEqD(lhs, rhs);
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            गच्छ last;
        }
        यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            गच्छ last;
        }
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opNeq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    यदि (lhs.isString()) प्रत्यागम opNeqN(lhs, rhs);
    यदि (lhs.isDouble()) प्रत्यागम opNeqD(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            गच्छ last;
        }
        यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            गच्छ last;
        }
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3Hash V3_संख्या::toHash() ध्रुव {
    // Shouldmatch isCaseEq
    यदि (isString()) प्रत्यागम V3Hash{toString()};
    यदि (isDouble()) प्रत्यागम V3Hash{toDouble()};
    V3Hash hash{width()};
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) hash += m_data.अंक()[i].m_मूल्य;
    प्रत्यागम hash;
}

तार्किक V3_संख्या::isCaseEq(ध्रुव V3_संख्या& rhs) ध्रुव {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    यदि (isString()) प्रत्यागम toString() == rhs.toString();
    यदि (isDouble()) प्रत्यागम toDouble() == rhs.toDouble();
    यदि (width() != rhs.width()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दगण(); ++i) {
        यदि (!(m_data.अंक()[i] == rhs.m_data.अंक()[i])) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

V3_संख्या& V3_संख्या::opCaseEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_संख्या& V3_संख्या::opCaseNeq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    यदि (lhs.isString()) {
        प्रत्यागम opNeqN(lhs, rhs);
    } अन्यथा यदि (lhs.isDouble()) {
        प्रत्यागम opNeqD(lhs, rhs);
    }
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            गच्छ last;
        }
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opWildEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (!rhs.bitIsXZ(bit)) {
            यदि (lhs.bitIsXZ(bit)) {
                outc = 'x';
                गच्छ last;
            }
            यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                गच्छ last;
            }
        }
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opWildNeq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (!rhs.bitIsXZ(bit)) {
            यदि (lhs.bitIsXZ(bit)) {
                outc = 'x';
                गच्छ last;
            }
            यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                गच्छ last;
            }
        }
    }
last:
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGt(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        यदि (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGtS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    {
        ध्रुव पूर्णाङ्क mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        यदि (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } अन्यथा यदि (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } अन्यथा यदि (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } अन्यथा यदि (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } अन्यथा {
            // both positive or negative, normal >
            प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                यदि (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                यदि (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                यदि (lhs.bitIsXZ(bit)) outc = 'x';
                यदि (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    प्रत्यागम setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGte(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या& eq = opEq(lhs, rhs);
    यदि (eq.isNeqZero()) प्रत्यागम eq;  // Return true
    प्रत्यागम opGt(lhs, rhs);
}

V3_संख्या& V3_संख्या::opGteS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या& eq = opEq(lhs, rhs);
    यदि (eq.isNeqZero()) प्रत्यागम eq;  // Return true
    प्रत्यागम opGtS(lhs, rhs);
}

V3_संख्या& V3_संख्या::opLt(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { प्रत्यागम opGt(rhs, lhs); }
V3_संख्या& V3_संख्या::opLte(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { प्रत्यागम opGte(rhs, lhs); }
V3_संख्या& V3_संख्या::opLtS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { प्रत्यागम opGtS(rhs, lhs); }
V3_संख्या& V3_संख्या::opLteS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) { प्रत्यागम opGteS(rhs, lhs); }

V3_संख्या& V3_संख्या::opShiftR(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // L(lhs) bit return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) प्रत्यागम setAllBitsX();
    setZero();
    यदि (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) प्रत्यागम *एतत्;
    ध्रुव uint32_t rhsval = rhs.toUInt();
    यदि (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<पूर्णाङ्क>(rhsval),
                 std::min(width(), lhs.width() - static_cast<पूर्णाङ्क>(rhsval)));
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opShiftRS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) प्रत्यागम setAllBitsX();
    setZero();
    ध्रुव तार्किक overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    यदि (!overflow) {
        ध्रुव uint32_t rhsval = rhs.toUInt();
        यदि (rhsval < lbits) {
            ध्रुव पूर्णाङ्क copyWidth = std::min(width(), static_cast<पूर्णाङ्क>(lbits - rhsval));
            copyBits(0, lhs, static_cast<पूर्णाङ्क>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            प्रत्यागम *एतत्;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opShiftL(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // L(lhs) bit return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) प्रत्यागम setAllBitsX();
    setZero();
    यदि (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) प्रत्यागम *एतत्;
    ध्रुव uint32_t rhsval = rhs.toUInt();
    यदि (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<पूर्णाङ्क>(rhsval), lhs, 0,
                 std::min(width() - static_cast<पूर्णाङ्क>(rhsval), lhs.width()));
    }
    प्रत्यागम *एतत्;
}

//======================================================================
// Ops - Arithmetic

V3_संख्या& V3_संख्या::opNegate(ध्रुव V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम setAllBitsX();
    V3_संख्या notlhs(&lhs, width());
    notlhs.opNot(lhs);
    ध्रुव V3_संख्या one(&lhs, width(), 1);
    opAdd(notlhs, one);
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opAdd(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    प्रति (पूर्णाङ्क word = 0; word < शब्दगण(); ++word) {
        ध्रुव uint64_t lwordval = lhs.m_data.अंक()[word].m_मूल्य;
        ध्रुव uint64_t rwordval = rhs.m_data.अंक()[word].m_मूल्य;
        ध्रुव uint64_t sum = lwordval + rwordval + carry;
        m_data.अंक()[word].m_मूल्य = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opSub(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    V3_संख्या negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    प्रत्यागम opAdd(lhs, negrhs);
}
V3_संख्या& V3_संख्या::opMul(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    setZero();
    यदि (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } अन्यथा {
        प्रति (पूर्णाङ्क lword = 0; lword < lhs.शब्दगण(); ++lword) {
            ध्रुव uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.अंक()[lword].m_मूल्य);
            यदि (lwordval == 0) अनुवर्त;
            प्रति (पूर्णाङ्क rword = 0; rword < rhs.शब्दगण(); ++rword) {
                ध्रुव uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.अंक()[rword].m_मूल्य);
                यदि (rwordval == 0) अनुवर्त;
                uint64_t mul = lwordval * rwordval;
                प्रति (पूर्णाङ्क qword = lword + rword; qword < शब्दगण(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.अंक()[qword].m_मूल्य);
                    m_data.अंक()[qword].m_मूल्य = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    यदि (mul == 0) भङ्ग;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opMulS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    V3_संख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ध्रुव V3_संख्या qNoSign = opMul(lhsNoSign, rhsNoSign);
    यदि ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opDiv(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    यदि (rhs.isEqZero()) प्रत्यागम setAllBitsXRemoved();
    यदि (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        प्रत्यागम *एतत्;
    } अन्यथा {
        // Wide division
        प्रत्यागम opModDivGuts(lhs, rhs, असत्य);
    }
}
V3_संख्या& V3_संख्या::opDivS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    यदि (rhs.isEqZero()) प्रत्यागम setAllBitsXRemoved();
    V3_संख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ध्रुव V3_संख्या qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    यदि ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *एतत्);
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opModDiv(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    यदि (rhs.isEqZero()) प्रत्यागम setAllBitsXRemoved();
    यदि (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        प्रत्यागम *एतत्;
    } अन्यथा {
        // Wide modulus
        प्रत्यागम opModDivGuts(lhs, rhs, सत्य);
    }
}
V3_संख्या& V3_संख्या::opModDivS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    यदि (rhs.isEqZero()) प्रत्यागम setAllBitsXRemoved();
    V3_संख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ध्रुव V3_संख्या qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    यदि (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opModDivGuts(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, तार्किक is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    ध्रुव पूर्णाङ्क l_शब्दगण = lhs.शब्दगण();
    ध्रुव पूर्णाङ्क umsbp1 = lhs.mostSetBitP1();  // dividend
    ध्रुव पूर्णाङ्क vmsbp1 = rhs.mostSetBitP1();  //भाजक
    यदि (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *एतत्);
        प्रत्यागम *एतत्;
    }

    ध्रुव पूर्णाङ्क uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ध्रुव पूर्णाङ्क vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    यदि (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        प्रति (पूर्णाङ्क j = uw - 1; j >= 0; j--) {
            ध्रुव uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.अंक()[j].m_मूल्य));
            m_data.अंक()[j].m_मूल्य = unw64 / static_cast<uint64_t>(rhs.m_data.अंक()[0].m_मूल्य);
            k = unw64
                - (static_cast<uint64_t>(m_data.अंक()[j].m_मूल्य)
                   * static_cast<uint64_t>(rhs.m_data.अंक()[0].m_मूल्य));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *एतत् << " rem=0x" << std::hex
                                   << k << std::dec);
        यदि (is_modulus) {
            setZero();
            m_data.अंक()[0].m_मूल्य = k;
        }
        opCleanThis();
        प्रत्यागम *एतत्;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_शब्दगण + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_शब्दगण + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    प्रति (पूर्णाङ्क i = 0; i < l_शब्दगण; ++i) m_data.अंक()[i].m_मूल्य = 0;
    प्रति (पूर्णाङ्क i = 0; i < l_शब्दगण + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ध्रुव पूर्णाङ्क s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ध्रुव uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    प्रति (पूर्णाङ्क i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.अंक()[i].m_मूल्य << s)
                | (shift_mask & (rhs.m_data.अंक()[i - 1].m_मूल्य >> (32 - s)));
    }
    vn[0] = rhs.m_data.अंक()[0].m_मूल्य << s;

    // Copy and shift dividend by same amount; may set new upper word
    यदि (s) {
        un[uw] = lhs.m_data.अंक()[uw - 1].m_मूल्य >> (32 - s);
    } अन्यथा {
        un[uw] = 0;
    }
    प्रति (पूर्णाङ्क i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.अंक()[i].m_मूल्य << s)
                | (shift_mask & (lhs.m_data.अंक()[i - 1].m_मूल्य >> (32 - s)));
    }
    un[0] = lhs.m_data.अंक()[0].m_मूल्य << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    प्रति (पूर्णाङ्क j = uw - vw; j >= 0; j--) {
        // Estimate
        ध्रुव uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        यदि (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            यदि (rhat < 0x100000000ULL) गच्छ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        प्रति (पूर्णाङ्क i = 0; i < vw; ++i) {
            ध्रुव uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.अंक()[j].m_मूल्य = qhat;  // Save quotient digit

        यदि (t < 0) {
            // Over subtracted; correct by adding back
            m_data.अंक()[j].m_मूल्य--;
            k = 0;
            प्रति (पूर्णाङ्क i = 0; i < vw; ++i) {
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

    यदि (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        प्रति (पूर्णाङ्क i = 0; i < vw; ++i) {
            m_data.अंक()[i].m_मूल्य = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        प्रति (पूर्णाङ्क i = vw; i < l_शब्दगण; ++i) m_data.अंक()[i].m_मूल्य = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *एतत्);
        प्रत्यागम *एतत्;
    } अन्यथा {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *एतत्);
        प्रत्यागम *एतत्;
    }
}

V3_संख्या& V3_संख्या::opPow(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, तार्किक lsign, तार्किक rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम setAllBitsX();
    यदि (rhs.isEqZero()) प्रत्यागम setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    यदि (rsign && rhs.isNegative()) {
        यदि (lhs.isEqZero()) {
            प्रत्यागम setAllBitsXRemoved();
        } अन्यथा यदि (lhs.isEqOne()) {
            प्रत्यागम setValue1();
        } अन्यथा यदि (lsign && lhs.isEqAllOnes()) {
            यदि (rhs.bitIs1(0)) {
                प्रत्यागम setAllBits1();  // -1^odd=-1
            } अन्यथा {
                प्रत्यागम setValue1();  // -1^even=1
            }
        }
        प्रत्यागम setZero();
    }
    यदि (lhs.isEqZero()) प्रत्यागम setZero();
    setZero();
    m_data.अंक()[0].m_मूल्य = 1;
    V3_संख्या power(&lhs, width());
    power.opAssign(lhs);
    प्रति (पूर्णाङ्क bit = 0; bit < rhs.width(); ++bit) {
        यदि (bit > 0) {  // power = power*power
            V3_संख्या lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        यदि (rhs.bitIs1(bit)) {  // out *= power
            V3_संख्या lastOut(&lhs, width());
            lastOut.opAssign(*एतत्);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opPowSU(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    प्रत्यागम opPow(lhs, rhs, सत्य, असत्य);
}
V3_संख्या& V3_संख्या::opPowSS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    प्रत्यागम opPow(lhs, rhs, सत्य, सत्य);
}
V3_संख्या& V3_संख्या::opPowUS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    प्रत्यागम opPow(lhs, rhs, असत्य, सत्य);
}

V3_संख्या& V3_संख्या::opBufIf1(ध्रुव V3_संख्या& ens, ध्रुव V3_संख्या& if1s) {
    अंक_ASSERT_OP_ARGS2(ens, if1s);
    अंक_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    प्रति (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } अन्यथा {
            setBit(bit, 'z');
        }
    }
    प्रत्यागम *एतत्;
}

// Sets all bits in range to the given value
V3_संख्या& V3_संख्या::opSetRange(uint32_t lsb, uint32_t width, वर्ण bit_मूल्य) {
    यदि (lsb < static_cast<uint32_t>(एतत्->width())) {
        fillBits(static_cast<पूर्णाङ्क>(lsb),
                 static_cast<पूर्णाङ्क>(std::min(width, static_cast<uint32_t>(एतत्->width()) - lsb)),
                 bit_मूल्य);
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opAssign(ध्रुव V3_संख्या& lhs) { प्रत्यागम opAssignNonXZ(lhs, असत्य); }
V3_संख्या& V3_संख्या::opAssignNonXZ(ध्रुव V3_संख्या& lhs, तार्किक ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    यदि (एतत् != &lhs) {
        यदि (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = सत्य;
        } अन्यथा यदि (isString()) {
            यदि (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } अन्यथा {
                m_data.str() = lhs.m_data.str();
            }
        } अन्यथा यदि (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } अन्यथा यदि (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } अन्यथा यदि (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } अन्यथा {
            प्रति (पूर्णाङ्क word = 0; word < शब्दगण(); ++word) {
                ध्रुव uint32_t मूल्य = word < lhs.शब्दगण() ? lhs.m_data.अंक()[word].m_मूल्य
                                                                & ~lhs.m_data.अंक()[word].m_मूल्य_X
                                                          : 0;
                m_data.अंक()[word] = {मूल्य, 0};
            }
            opCleanThis();
        }
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opNToI(ध्रुव V3_संख्या& lhs) {
    // String to packed number
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    setZero();
    ध्रुव string& str = lhs.toString();
    प्रति (size_t n = 0; n < str.length(); ++n) {
        ध्रुव वर्ण c = str[str.length() - 1 - n];
        प्रति (size_t cbit = 0; cbit < 8; ++cbit) {
            ध्रुव std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opExtendS(ध्रुव V3_संख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ध्रुव पूर्णाङ्क copyWidth = std::min(width(), static_cast<पूर्णाङ्क>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opExtendXZ(ध्रुव V3_संख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ध्रुव पूर्णाङ्क copyWidth = std::min(width(), static_cast<पूर्णाङ्क>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opClean(ध्रुव V3_संख्या& lhs, uint32_t bits) { प्रत्यागम opSel(lhs, bits - 1, 0); }

शून्य V3_संख्या::opCleanThis(तार्किक warnOnTruncation) {
    // Clean MSB of number
    अंक_ASSERT_LOGIC_ARGS1(*एतत्);
    ध्रुव मूल्य_And_X v = m_data.अंक()[शब्दगण() - 1];
    ध्रुव uint32_t new_मूल्य_Msb = v.m_मूल्य & hiWordMask();
    ध्रुव uint32_t new_मूल्य_XMsb = v.m_मूल्य_X & hiWordMask();
    यदि (warnOnTruncation && (new_मूल्य_Msb != v.m_मूल्य || new_मूल्य_XMsb != v.m_मूल्य_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.अंक()[शब्दगण() - 1] = {new_मूल्य_Msb, new_मूल्य_XMsb};
}

V3_संख्या& V3_संख्या::opSel(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& msb, ध्रुव V3_संख्या& lsb) {
    अंक_ASSERT_OP_ARGS3(lhs, msb, lsb);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS2(msb, lsb);
    यदि (lsb.isFourState() || msb.isFourState()) प्रत्यागम setAllBitsX();
    प्रत्यागम opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_संख्या& V3_संख्या::opSel(ध्रुव V3_संख्या& lhs, uint32_t msbval, uint32_t lsbval) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    पूर्णाङ्क copyWidth = 0;
    यदि (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        ध्रुव uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<पूर्णाङ्क>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<पूर्णाङ्क>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    प्रत्यागम *एतत्;
}

V3_संख्या& V3_संख्या::opSelInto(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& lsb, पूर्णाङ्क width) {
    प्रत्यागम opSelInto(lhs, lsb.toSInt(), width);
}

V3_संख्या& V3_संख्या::opSelInto(ध्रुव V3_संख्या& lhs, पूर्णाङ्क lsbval, पूर्णाङ्क width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ध्रुव पूर्णाङ्क copyWidth = std::max(0, std::min({width, lhs.width(), एतत्->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    ध्रुव पूर्णाङ्क fillWidth
        = std::max(0, std::min(width - copyWidth, एतत्->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    प्रत्यागम *एतत्;
}

//======================================================================
// Ops - Floating point

V3_संख्या& V3_संख्या::opIToRD(ध्रुव V3_संख्या& lhs, तार्किक isSigned) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_संख्या noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    द्विगुण d = 0;
    ध्रुव तार्किक negate = isSigned && noxz.isNegative();
    यदि (negate) {
        ध्रुव V3_संख्या noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    प्रति (पूर्णाङ्क bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        यदि (noxz.bitIs1(bit)) d += exp2(bit);
    }
    यदि (negate) d = -d;
    प्रत्यागम setDouble(d);
}
V3_संख्या& V3_संख्या::opRToIS(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    ध्रुव द्विगुण v = VL_TRUNC(lhs.toDouble());
    ध्रुव int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    प्रत्यागम setLongS(i);
}
V3_संख्या& V3_संख्या::opRToIRoundS(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    ध्रुव द्विगुण v = VL_ROUND(lhs.toDouble());
    setZero();
    सङ्घ {
        द्विगुण d;
        uint64_t q;
    } u;
    u.d = v;
    (शून्य)u.d;

    ध्रुव पूर्णाङ्क exp = static_cast<पूर्णाङ्क>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    ध्रुव पूर्णाङ्क lsb = exp - 52;
    ध्रुव uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    यदि (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        प्रति (पूर्णाङ्क bit = 0; bit <= 52; ++bit) {
            यदि (mantissa & (1ULL << bit)) {
                ध्रुव पूर्णाङ्क outbit = bit + lsb;
                यदि (outbit >= 0) setBit(outbit, 1);
            }
        }
        यदि (v < 0) {
            ध्रुव V3_संख्या noSign = *एतत्;
            opNegate(noSign);
        }
    }
    प्रत्यागम *एतत्;
}
V3_संख्या& V3_संख्या::opRealToBits(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    सङ्घ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    प्रत्यागम setQuad(u.m_v);
}
V3_संख्या& V3_संख्या::opBitsToRealD(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    सङ्घ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    प्रत्यागम setDouble(u.m_d);
}
V3_संख्या& V3_संख्या::opNegateD(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    प्रत्यागम setDouble(-lhs.toDouble());
}
V3_संख्या& V3_संख्या::opAddD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_संख्या& V3_संख्या::opSubD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_संख्या& V3_संख्या::opMulD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_संख्या& V3_संख्या::opDivD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_संख्या& V3_संख्या::opPowD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_संख्या& V3_संख्या::opEqD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_संख्या& V3_संख्या::opNeqD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_संख्या& V3_संख्या::opGtD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_संख्या& V3_संख्या::opGteD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_संख्या& V3_संख्या::opLtD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_संख्या& V3_संख्या::opLteD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_संख्या& V3_संख्या::opConcatN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setString(lhs.toString() + rhs.toString());
}
V3_संख्या& V3_संख्या::opReplN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_STRING_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(rhs);
    प्रत्यागम opReplN(lhs, rhs.toUInt());
}
V3_संख्या& V3_संख्या::opReplN(ध्रुव V3_संख्या& lhs, uint32_t rhsval) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    प्रति (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    प्रत्यागम setString(out);
}
V3_संख्या& V3_संख्या::opCvtPackString(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    प्रत्यागम setString(lhs.toString());
}
V3_संख्या& V3_संख्या::opToLowerN(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    ध्रुव std::string out = VString::downcase(lhs.toString());
    प्रत्यागम setString(out);
}
V3_संख्या& V3_संख्या::opToUpperN(ध्रुव V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    ध्रुव std::string out = VString::upcase(lhs.toString());
    प्रत्यागम setString(out);
}

V3_संख्या& V3_संख्या::opEqN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toString() == rhs.toString());
}
V3_संख्या& V3_संख्या::opNeqN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toString() != rhs.toString());
}
V3_संख्या& V3_संख्या::opGtN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toString() > rhs.toString());
}
V3_संख्या& V3_संख्या::opGteN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toString() >= rhs.toString());
}
V3_संख्या& V3_संख्या::opLtN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toString() < rhs.toString());
}
V3_संख्या& V3_संख्या::opLteN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

शून्य V3_संख्या::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_पंक्ति* ध्रुव fl = नूतन File_पंक्ति{File_पंक्ति::builtInFilename()};
    V3_संख्या अंक{fl, 32, 0};
    अंक.selfTestThis();
}

शून्य V3_संख्या::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(0, 0), सत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(1e19, 1e19), सत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(9, 0.0001), असत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(1, 1 + std::numeric_limits<द्विगुण>::epsilon()), सत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(0.009, 0.00899999999999999931998839741709), सत्य);

    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(0), सत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(1), सत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(-1), सत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(1.0001), असत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(0.9999), असत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(-1.0001), असत्य);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(-0.9999), असत्य);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
