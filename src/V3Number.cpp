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
#define अङ्क_ASSERT_OP_ARGS1(arg1) \
    UASSERT((एतत् != &(arg1)), "Number operation called with same source and dest")
#define अङ्क_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2)), \
            "Number operation called with same source and dest")
#define अङ्क_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2) && एतत् != &(arg3)), \
            "Number operation called with same source and dest")
#define अङ्क_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2) && एतत् != &(arg3) && एतत् != &(arg4)), \
            "Number operation called with same source and dest")

#define अङ्क_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_सङ्ख्या_Data::V3_सङ्ख्या_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define अङ्क_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    अङ्क_ASSERT_LOGIC_ARGS1(arg1); \
    अङ्क_ASSERT_LOGIC_ARGS1(arg2)

#define अङ्क_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    अङ्क_ASSERT_LOGIC_ARGS1(arg1); \
    अङ्क_ASSERT_LOGIC_ARGS1(arg2); \
    अङ्क_ASSERT_LOGIC_ARGS1(arg3); \
    अङ्क_ASSERT_LOGIC_ARGS1(arg4)

#define अङ्क_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define अङ्क_ASSERT_STRING_ARGS2(arg1, arg2) \
    अङ्क_ASSERT_STRING_ARGS1(arg1); \
    अङ्क_ASSERT_STRING_ARGS1(arg2)

#define अङ्क_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define अङ्क_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    अङ्क_ASSERT_DOUBLE_ARGS1(arg1); \
    अङ्क_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

शून्य V3_सङ्ख्या::v3errorEnd(अचर std::ostringstream& str) अचर VL_RELEASE(V3Error::s().m_mutex) {
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

शून्य V3_सङ्ख्या::v3errorEndFatal(अचर std::ostringstream& str) अचर
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_सङ्ख्या::V3_सङ्ख्या(Ast_गाँठो* nodep, VerilogStringLiteral, अचर string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    यदि (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } अन्यथा {
        init(nodep, std::max<पूर्णाङ्क>(str.length() * 8, 1));
        लागि (अचिन्हित pos = 0; pos < str.length(); ++pos) {
            अचर पूर्णाङ्क topos = str.length() - 1 - pos;
            मान_And_X& v = m_data.अङ्क()[topos / 4];
            लागि (पूर्णाङ्क bit = 0; bit < 8; ++bit) {
                यदि (str[pos] & (1UL << bit)) v.m_मान |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = सत्य;
    opCleanThis(सत्य);
}

V3_सङ्ख्या::V3_सङ्ख्या(Ast_गाँठो* nodep, अचर Ast_गाँठो_DType* nodedtypep) {
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

शून्य V3_सङ्ख्या::create(अचर वर्ण* sourcep) {
    m_data.setLogic();
    अचर वर्ण* मान_startp = sourcep;
    लागि (अचर वर्ण* cp = sourcep; *cp; ++cp) {
        यदि (*cp == '\'') {
            मान_startp = cp + 1;
            रोक;
        }
    }

    तार्किक userSized = असत्य;
    तार्किक unbased = असत्य;
    वर्ण base = '\0';
    यदि (मान_startp != sourcep) {  // Has a '
        string widthn;
        अचर वर्ण* cp = sourcep;
        लागि (; *cp; ++cp) {
            यदि (*cp == '\'') {
                ++cp;
                रोक;
            }
            यदि (*cp != '_') widthn += *cp;
        }
        जबसम्म (*cp == '_') cp++;
        यदि (*cp && std::छोटे(*cp) == 's') {
            ++cp;
            isSigned(सत्य);
        }
        यदि (*cp) {
            base = *cp;
            ++cp;
        }
        मान_startp = cp;

        userSized = widthn.length() != 0;
        यदि (std::म_से_प(widthn.c_str())) {
            यदि (std::म_से_प(widthn.c_str()) < 0
                || std::म_से_प(widthn.c_str()) > v3Global.opt.max_अङ्क_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_अङ्क_Width(), सत्य);
            } अन्यथा {
                width(std::म_से_प(widthn.c_str()), सत्य);
            }
        }
    } अन्यथा {
        unbased = सत्य;
        base = 'd';
    }

    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    यदि (base == '0') {
        width(1, असत्य);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = सत्य;
    } अन्यथा यदि (base == '1') {
        width(1, असत्य);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = सत्य;
    } अन्यथा यदि (std::छोटे(base) == 'z') {
        width(1, असत्य);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = सत्य;
    } अन्यथा यदि (std::छोटे(base) == 'x') {
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
        अचर पूर्णाङ्क widthBound = std::max<पूर्णाङ्क>(32, std::म_माप(मान_startp) * 4);
        width(widthBound, असत्य);  // Will change width below
        यदि (unbased) isSigned(सत्य);  // Also says the spec.
    }

    // Ignore leading blanks
    जबसम्म (*मान_startp == '_' || std::है_खाली(*मान_startp)) ++मान_startp;
    यदि (!*मान_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    यदि (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    पूर्णाङ्क obit = 0;  // Start at LSB
    पूर्णाङ्क base_align = 1;
    यदि (std::छोटे(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        जबसम्म (*मान_startp == '_' || *मान_startp == '0') ++मान_startp;
        // Convert decimal number to hex
        पूर्णाङ्क olen = 0;
        uint32_t val = 0;
        पूर्णाङ्क got_x = 0;
        पूर्णाङ्क got_z = 0;
        पूर्णाङ्क got_01 = 0;
        तार्किक warnedTooMany = असत्य;
        लागि (अचर वर्ण* cp = मान_startp; *cp; ++cp) {
            विकल्प (std::छोटे(*cp)) {
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
                    m_data.अङ्क()[0].m_मान = val;
                } अन्यथा {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_सङ्ख्या product(एतत्, width() + 4);  // +4 for overflow detection
                    अचर V3_सङ्ख्या ten(एतत्, width() + 4, 10);
                    अचर V3_सङ्ख्या addend(एतत्, width(), (*cp - '0'));
                    product.opMul(*एतत्, ten);
                    opAdd(product, addend);
                    यदि (!warnedTooMany && product.bits_मान(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = सत्य;
                    }
                }
                ++olen;
                got_01 = 1;
                रोक;
            }
            पक्ष 'z':
            पक्ष '?': {
                got_z = 1;
                यदि (!userSized) width(32, असत्य);
                setAllBitsZ();
                रोक;
            }
            पक्ष 'x': {
                got_x = 1;
                यदि (!userSized) width(32, असत्य);
                setAllBitsX();
                रोक;
            }
            पक्ष '_': रोक;
            स्वभाव: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                रोक;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        यदि ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << मान_startp);
        }
    } अन्यथा {
        // Convert bin/octal number to hex
        लागि (अचर वर्ण* cp = मान_startp + std::म_माप(मान_startp) - 1; cp >= मान_startp;
             cp--) {
            यदि (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                रोक;
            }
            विकल्प (std::छोटे(base)) {
            पक्ष 'b': {
                base_align = 1;
                विकल्प (std::छोटे(*cp)) {
                पक्ष '0': setBit(obit++, 0); रोक;
                पक्ष '1': setBit(obit++, 1); रोक;
                पक्ष 'z':
                पक्ष '?': setBit(obit++, 'z'); रोक;
                पक्ष 'x': setBit(obit++, 'x'); रोक;
                पक्ष '_': रोक;
                स्वभाव: v3error("Illegal character in binary constant: " << *cp);
                }
                रोक;
            }

            पक्ष 'o':
            पक्ष 'c': {
                base_align = 3;
                विकल्प (std::छोटे(*cp)) {  // clang-format off
                पक्ष '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  रोक;
                पक्ष '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  रोक;
                पक्ष '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  रोक;
                पक्ष '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  रोक;
                पक्ष '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  रोक;
                पक्ष '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  रोक;
                पक्ष '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  रोक;
                पक्ष '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  रोक;
                पक्ष 'z':
                पक्ष '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); रोक;
                पक्ष 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); रोक;
                पक्ष '_': रोक;
                // clang-format on
                स्वभाव: v3error("Illegal character in octal constant");
                }
                रोक;
            }

            पक्ष 'h':  // FALLTHRU
            पक्ष 'x': {
                base_align = 4;
                विकल्प (std::छोटे(*cp)) {  // clang-format off
                पक्ष '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); रोक;
                पक्ष '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); रोक;
                पक्ष '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); रोक;
                पक्ष '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); रोक;
                पक्ष '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); रोक;
                पक्ष '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); रोक;
                पक्ष '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); रोक;
                पक्ष '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); रोक;
                पक्ष '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); रोक;
                पक्ष '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); रोक;
                पक्ष 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); रोक;
                पक्ष 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); रोक;
                पक्ष 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); रोक;
                पक्ष 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); रोक;
                पक्ष 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); रोक;
                पक्ष 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); रोक;
                पक्ष 'z':
                पक्ष '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); रोक;
                पक्ष 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); रोक;
                    // clang-format on
                पक्ष '_': रोक;
                स्वभाव:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    रोक;
                }  // LCOV_EXCL_STOP
                रोक;
            }
            स्वभाव:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                रोक;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    यदि (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), असत्य);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    जबसम्म (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(सत्य);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

शून्य V3_सङ्ख्या::warnTooMany(अचर string& मान) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << मान << "'");
}

शून्य V3_सङ्ख्या::nodep(Ast_गाँठो* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    यदि (!nodep) फर्काऊ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

तार्किक V3_सङ्ख्या::epsilonEqual(द्विगुण a, द्विगुण b) {
    फर्काऊ std::भ_असल(a - b)
           <= (std::numeric_limits<द्विगुण>::epsilon() * std::max(1.0, std::max(a, b)));
}

तार्किक V3_सङ्ख्या::epsilonIntegral(द्विगुण a) {
    अचर द्विगुण dint = static_cast<द्विगुण>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    फर्काऊ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

पूर्णाङ्क V3_सङ्ख्या::log2b(uint32_t अङ्क) {
    // See also opCLog2
    लागि (पूर्णाङ्क bit = 31; bit > 0; bit--) {
        यदि (अङ्क & (1ULL << bit)) फर्काऊ bit;
    }
    फर्काऊ 0;
}

पूर्णाङ्क V3_सङ्ख्या::log2bQuad(uint64_t अङ्क) {
    यदि (अङ्क >> 32ULL) फर्काऊ 32 + log2b(अङ्क >> 32ULL);
    फर्काऊ log2b(अङ्क);
}

//======================================================================
// Setters

V3_सङ्ख्या& V3_सङ्ख्या::setZero() {
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, 0};
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setQuad(uint64_t मान) {
    setZero();
    m_data.अङ्क()[0].m_मान = मान & 0xffffffffULL;
    यदि (width() > 32) m_data.अङ्क()[1].m_मान = (मान >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setLong(uint32_t मान) {
    setZero();
    m_data.अङ्क()[0].m_मान = मान;
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setLongS(int32_t मान) {
    setZero();
    सङ्घ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = मान;
    (शून्य)u.s;
    m_data.अङ्क()[0].m_मान = u.u;
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setDouble(द्विगुण मान) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    सङ्घ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.d = मान;
    (शून्य)u.d;
    लागि (पूर्णाङ्क i = 2; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, 0};
    m_data.अङ्क()[0].m_मान = u.u[0];
    m_data.अङ्क()[1].m_मान = u.u[1];
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setSingleBits(वर्ण मान) {
    लागि (पूर्णाङ्क i = 1 /*upper*/; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, 0};
    m_data.अङ्क()[0] = {(मान == '1' || मान == 'x' || मान == 1 || मान == 3),
                       (मान == 'z' || मान == 'x' || मान == 2 || मान == 3)};
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::setAllBits0() {
    setZero();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setAllBits1() {
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {~0U, 0};
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {~0U, ~0U};
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setAllBitsZ() {
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, ~0U};
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::setAllBitsXRemoved() {
    यदि (!v3Global.constRemoveXs()) {
        फर्काऊ setAllBitsX();
    } अन्यथा {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        यदि (v3Global.opt.xAssign() == "1") {
            फर्काऊ setAllBits1();
        } अन्यथा {
            फर्काऊ setAllBits0();
        }
    }
}
V3_सङ्ख्या& V3_सङ्ख्या::setValue1() {
    m_data.अङ्क()[0] = {1, 0};
    लागि (पूर्णाङ्क i = 1; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, 0};
    फर्काऊ *एतत्;
}

शून्य V3_सङ्ख्या::setBitX0(पूर्णाङ्क bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_सङ्ख्या& V3_सङ्ख्या::setMask(पूर्णाङ्क nbits, पूर्णाङ्क lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    फर्काऊ *एतत्;
}

//======================================================================
// ACCESSORS - as strings

string V3_सङ्ख्या::ascii(तार्किक prefixed, तार्किक cleanVerilog) अचर VL_MT_STABLE {
    // Correct number of zero bits/width matters
    यदि (is1Step()) फर्काऊ "1step";
    यदि (isNull()) फर्काऊ "null";
    std::ostringstream out;
    यदि (isDouble()) {
        out.precision(17);
        यदि (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << toDouble();
            यदि (toDouble() == न्यूनमान(toDouble())) out << ".0";
        }
        फर्काऊ out.str();
    } अन्यथा यदि (isString()) {
        फर्काऊ '"' + toString() + '"';
    } अन्यथा {
        यदि (VL_UNCOVERABLE((m_data.अङ्क()[शब्दहरू() - 1].m_मान | m_data.अङ्क()[शब्दहरू() - 1].m_मान_X)
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
            फर्काऊ out.str();
        } अन्यथा {
            यदि (cleanVerilog) {
                out << "'";
            } अन्यथा {
                out << "?" << width() << "?";
            }
        }
        यदि (isSigned()) out << "s";
    }

    अचर तार्किक द्विआधारी = (isFourState()
#ifdef V3_सङ्ख्या_ASCII_द्विआधारी
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
    फर्काऊ out.str();
}

तार्किक V3_सङ्ख्या::displayedFmtHasArg(वर्ण format, तार्किक isScan) {
    // Is this a format letter that takes an argument in runtime?
    (शून्य)isScan;
    विकल्प (std::छोटे(format)) {
    पक्ष 'b': फर्काऊ सत्य;  //द्विआधारी
    पक्ष 'c': फर्काऊ सत्य;  // Character
    पक्ष 'd': फर्काऊ सत्य;  // Decimal
    पक्ष 'e': फर्काऊ सत्य;  // Floating
    पक्ष 'f': फर्काऊ सत्य;  // Floating
    पक्ष 'g': फर्काऊ सत्य;  // Floating
    पक्ष 'h': फर्काऊ सत्य;  // Hex
    पक्ष 'o': फर्काऊ सत्य;  // Octal
    पक्ष 'p': फर्काऊ सत्य;  // Pattern
    पक्ष 's': फर्काऊ सत्य;  // String
    पक्ष 't': फर्काऊ सत्य;  // Time
    पक्ष 'u': फर्काऊ सत्य;  // Packed 2-state
    पक्ष 'v': फर्काऊ सत्य;  // Strength
    पक्ष 'x': फर्काऊ सत्य;  // Hex
    पक्ष 'z': फर्काऊ सत्य;  // Packed 4-state
    पक्ष '?': फर्काऊ सत्य;  // Internal: Compute format in V3Width based on data type
    स्वभाव: फर्काऊ असत्य;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_सङ्ख्या::displayPad(माप_प्रकार fmtsize, वर्ण pad, तार्किक बायाँ, अचर string& in) VL_PURE {
    string padding;
    यदि (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    फर्काऊ बायाँ ? (in + padding) : (padding + in);
}

string V3_सङ्ख्या::displayedEnumName(अचर AstEnumItem* itemp) {
    अचर string name = VIdProtect::protect(itemp->prettyName());
    फर्काऊ VString::isIdentifier(name) && !std::है_अंक(static_cast<अचिन्हित वर्ण>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_सङ्ख्या::displayedEnumName(अचर AstEnumDType* dtypep) अचर VL_MT_STABLE {
    V3_सङ्ख्या मान{एतत्, dtypep->width()};
    मान.opAssign(*एतत्);
    लागि (अचर AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        अचर AstConst* अचर constp = VN_AS(itemp->valuep(), Const);
        यदि (!constp->अङ्क().isAnyXZ() && मान.isCaseEq(constp->अङ्क()))
            फर्काऊ displayedEnumName(itemp);
    }
    फर्काऊ "";
}

string V3_सङ्ख्या::displayedEnum(अचर AstSFormatArg* argp,
                               अचर string& vformat) अचर VL_MT_STABLE {
    अचर AstEnumDType* अचर dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_सङ्ख्या मान{एतत्, dtypep->width()};
    मान.opAssign(*एतत्);
    अचर VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    अचर वर्ण fmt = std::छोटे(vformat.back());
    यदि (fmt != 'p' && fmt != 's') फर्काऊ मान.displayed(argp, vformat, numericAttr);
    string name = मान.displayedEnumName(dtypep);
    यदि (name.empty()) name = मान.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_सङ्ख्या name_अङ्क{एतत्};
    name_अङ्क.setString(name);
    फर्काऊ name_अङ्क.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_सङ्ख्या::displayed(अचर Ast_गाँठो* nodep, अचर string& vformat,
                           अचर VFormatAttr& formatAttr) अचर VL_MT_STABLE {
    फर्काऊ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_सङ्ख्या::displayed(File_पङ्क्ति* fl, अचर string& vformat,
                           अचर VFormatAttr& formatAttr) अचर VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    स्वतः pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *एतत्);
    ++pos;
    तार्किक बायाँ = असत्य;
    यदि (pos[0] == '-') {
        बायाँ = सत्य;
        ++pos;
    }
    string fmtsize;
    लागि (; pos != vformat.cend() && (std::है_अंक(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    वर्ण fmt = std::छोटे(pos[0]);

    // Override user's code for certain data-type determined arguments
    यदि (formatAttr.isComplex()) {
        यदि (fmt != 'p') fmt = 's';  // override
    } अन्यथा यदि (formatAttr.isString()) {
        यदि (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_सङ्ख्या strwide{fl, static_cast<पूर्णाङ्क>(toString().length()) * 8, 0};
            strwide.opNToI(*एतत्);
            फर्काऊ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
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
            V3_सङ्ख्या nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*एतत्);
            फर्काऊ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    विकल्प (fmt) {
    पक्ष 'c': {
        यदि (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        अचर अचिन्हित पूर्णाङ्क v = bits_मान(0, 8);
        str.थप_back(static_cast<वर्ण>(v));
        फर्काऊ str;
    }
    पक्ष 'e':  // FALLTHRU
    पक्ष 'f':  // FALLTHRU
    पक्ष 'g': {
        अचर द्विगुण n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        वर्ण tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        फर्काऊ tmp;
    }
    पक्ष 's': {
        यदि (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } अन्यथा {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            पूर्णाङ्क bit = width() - 1;
            तार्किक start = सत्य;
            जबसम्म ((bit % 8) != 7) ++bit;
            लागि (; bit >= 0; bit -= 8) {
                अचर पूर्णाङ्क v = bits_मान(bit - 7, 8);
                यदि (!start || v) {
                    str += static_cast<वर्ण>((v == 0) ? ' ' : v);
                    start = असत्य;  // Drop leading 0s
                } अन्यथा {
                    यदि (fmtsize != "0") str += ' ';
                }
            }
        }
        अचर माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', बायाँ, str);
        फर्काऊ str;
    }
    पक्ष 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        यदि (formatAttr.isDouble()) {
            अचर द्विगुण n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            वर्ण tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            फर्काऊ tmp;
        }
        यदि (formatAttr.isString())
            फर्काऊ '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        यदि (formatAttr.isComplex()) फर्काऊ toString();
        फर्काऊ "%p";
    }
    पक्ष 'b':  // FALLTHRU
    पक्ष 'o':  // FALLTHRU
    पक्ष 'h':  // FALLTHRU
    पक्ष 'x': {
        पूर्णाङ्क bit = width() - 1;
        यदि (बायाँ || !fmtsize.empty()) {
            जबसम्म (bit && bitIs0(bit)) --bit;
        }
        विकल्प (fmt) {
        पक्ष 'b': {
            लागि (; bit >= 0; --bit) {
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
            रोक;
        }
        पक्ष 'o': {
            जबसम्म ((bit % 3) != 2) ++bit;
            लागि (; bit > 0; bit -= 3) {
                अचर पूर्णाङ्क अङ्क_X = गन्ती_X(bit - 2, 3);
                अचर पूर्णाङ्क अङ्क_Z = गन्ती_Z(bit - 2, 3);
                यदि (अङ्क_X == 3 || अङ्क_X == width() - (bit - 2)) {
                    str += 'x';
                    जारी;
                }
                यदि (अङ्क_Z == 3 || अङ्क_Z == width() - (bit - 2)) {
                    str += 'z';
                    जारी;
                }
                यदि (अङ्क_X > 0) {
                    str += 'X';
                    जारी;
                }
                यदि (अङ्क_Z > 0) {
                    str += 'Z';
                    जारी;
                }
                अचर पूर्णाङ्क v = bits_मान(bit - 2, 3);
                str += static_cast<वर्ण>('0' + v);
            }
            रोक;
        }
        स्वभाव: {  // h/x
            जबसम्म ((bit % 4) != 3) ++bit;
            लागि (; bit > 0; bit -= 4) {
                अचर पूर्णाङ्क अङ्क_X = गन्ती_X(bit - 3, 4);
                अचर पूर्णाङ्क अङ्क_Z = गन्ती_Z(bit - 3, 4);
                यदि (अङ्क_X == 4 || अङ्क_X == width() - (bit - 3)) {
                    str += 'x';
                    जारी;
                }
                यदि (अङ्क_Z == 4 || अङ्क_Z == width() - (bit - 3)) {
                    str += 'z';
                    जारी;
                }
                यदि (अङ्क_X > 0) {
                    str += 'X';
                    जारी;
                }
                यदि (अङ्क_Z > 0) {
                    str += 'Z';
                    जारी;
                }
                अचर पूर्णाङ्क v = bits_मान(bit - 3, 4);
                यदि (v >= 10) {
                    str += static_cast<वर्ण>('a' + v - 10);
                } अन्यथा {
                    str += static_cast<वर्ण>('0' + v);
                }
            }
            रोक;
        }
        }  // switch
        अचर माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, (बायाँ ? ' ' : '0'), बायाँ, str);
        फर्काऊ str;
    }  // case b/d/x/o
    पक्ष 't':  // Time
    पक्ष 'd': {  // Unsigned decimal
        अचर तार्किक issigned = formatAttr.isSigned();
        यदि (fmtsize == "" && !बायाँ) {
            अचर द्विगुण mantissabits = width() - (issigned ? 1 : 0);
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
        अचर तार्किक zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !बायाँ;
        // fmtsize might have changed since we parsed the %fmtsize
        अचर माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), बायाँ, str);
        फर्काऊ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    पक्ष 'u': {  // Packed 2-state
        लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
            अचर uint32_t v = m_data.अङ्क()[i].m_मान;
            str += static_cast<वर्ण>((v >> 0) & 0xff);
            str += static_cast<वर्ण>((v >> 8) & 0xff);
            str += static_cast<वर्ण>((v >> 16) & 0xff);
            str += static_cast<वर्ण>((v >> 24) & 0xff);
        }
        फर्काऊ str;
    }
    पक्ष 'z': {  // Packed 4-state
        लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
            अचर मान_And_X v = m_data.अङ्क()[i];
            str += static_cast<वर्ण>((v.m_मान >> 0) & 0xff);
            str += static_cast<वर्ण>((v.m_मान >> 8) & 0xff);
            str += static_cast<वर्ण>((v.m_मान >> 16) & 0xff);
            str += static_cast<वर्ण>((v.m_मान >> 24) & 0xff);
            str += static_cast<वर्ण>((v.m_मान_X >> 0) & 0xff);
            str += static_cast<वर्ण>((v.m_मान_X >> 8) & 0xff);
            str += static_cast<वर्ण>((v.m_मान_X >> 16) & 0xff);
            str += static_cast<वर्ण>((v.m_मान_X >> 24) & 0xff);
        }
        फर्काऊ str;
    }
    पक्ष 'v': {  // Strength
        पूर्णाङ्क bit = width() - 1;
        लागि (; bit >= 0; bit--) {
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
        फर्काऊ str;
    }
    स्वभाव: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_सङ्ख्या::emitC() अचर VL_MT_STABLE {
    constexpr माप_प्रकार bufsize = 1000;
    वर्ण sbuf[bufsize];
    string result;
    यदि (isNull()) {
        फर्काऊ "VlNull{}";
    } अन्यथा यदि (isDouble()) {
        अचर द्विगुण dnum = toDouble();
        यदि (std::isinf(dnum)) {
            यदि (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } अन्यथा यदि (std::isnan(dnum)) {
            यदि (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } अन्यथा {
            अचर वर्ण* अचर fmt = (static_cast<पूर्णाङ्क>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            फर्काऊ sbuf;
        }
    } अन्यथा यदि (isString()) {
        अचर string strg = toString();
        अचर string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        फर्काऊ '"' + quoted + "\"s";
    } अन्यथा यदि (शब्दहरू() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(शब्दहरू()) + ">{{";
        यदि (शब्दहरू() > 4) result += '\n';
        लागि (पूर्णाङ्क n = 0; n < शब्दहरू(); ++n) {
            यदि (n) result += ((n % 4) ? ", " : ",\n");
            (शून्य)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        यदि (शब्दहरू() > 4) result += '\n';
        result += "}}";
    } अन्यथा यदि (शब्दहरू() == 2) {  // Quad
        अचर uint64_t qnum = static_cast<uint64_t>(toUQuad());
        अचर वर्ण* अचर fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        फर्काऊ sbuf;
    } अन्यथा {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        अचर uint32_t unum = toUInt();
        अचर वर्ण* अचर fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        फर्काऊ sbuf;
    }
    फर्काऊ result;
}

string V3_सङ्ख्या::toDecimalS() अचर VL_MT_STABLE {
    // Correct number of zero bits/width matters
    यदि (isNegative()) {
        V3_सङ्ख्या lhsNoSign = *एतत्;
        lhsNoSign.opNegate(*एतत्);
        फर्काऊ "-"s + lhsNoSign.toDecimalU();
    } अन्यथा {
        फर्काऊ toDecimalU();
    }
}

string V3_सङ्ख्या::toDecimalU() अचर VL_MT_STABLE {
    अचर पूर्णाङ्क maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_सङ्ख्या bcd(एतत्, maxdecwidth + 4);
    V3_सङ्ख्या tmp(एतत्, maxdecwidth + 4);
    V3_सङ्ख्या tmp2(एतत्, maxdecwidth + 4);

    पूर्णाङ्क from_bit = width() - 1;
    // Skip all leading zeros
    लागि (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    लागि (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        लागि (पूर्णाङ्क nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            यदि (bcd.bits_मान(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_सङ्ख्या(एतत्, 32, 1));
        // bcd[0] = this[from_bit]
        यदि (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    पूर्णाङ्क lsb = (maxdecwidth - 1) & ~3;
    लागि (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        यदि (bcd.bits_मान(lsb, 4)) रोक;
    }
    लागि (; lsb >= 0; lsb -= 4) {
        output += static_cast<वर्ण>('0' + bcd.bits_मान(lsb, 4));  // 0..9
    }
    फर्काऊ output;
}

string V3_सङ्ख्या::toRealString() अचर VL_MT_STABLE {
    अचर द्विगुण मान = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << मान;
    string result = out.str();
    यदि (std::म_से_भग्न(result.c_str(), nullptr) != मान) {
        out.str("");
        out.precision(17);
        out << मान;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    यदि (result.find_first_of(".en") == string::npos) result += ".0";
    फर्काऊ result;
}

//======================================================================
// ACCESSORS - as numbers

तार्किक V3_सङ्ख्या::fitsInUInt() अचर VL_MT_SAFE {
    यदि (isFourState()) फर्काऊ असत्य;
    // We allow wide numbers that represent values <= 32 bits
    लागि (पूर्णाङ्क i = 1; i < शब्दहरू(); ++i) {
        यदि (m_data.अङ्क()[i].m_मान) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}
uint32_t V3_सङ्ख्या::toUInt() अचर VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *एतत्);
    // We allow wide numbers that represent values <= 32 bits
    यदि (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *एतत्);
    }
    फर्काऊ m_data.अङ्क()[0].m_मान;
}

द्विगुण V3_सङ्ख्या::toDouble() अचर VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    सङ्घ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.अङ्क()[0].m_मान;
    u.u[1] = m_data.अङ्क()[1].m_मान;
    फर्काऊ u.d;
}

int32_t V3_सङ्ख्या::toSInt() अचर VL_MT_SAFE {
    यदि (isSigned()) {
        अचर uint32_t v = toUInt();
        अचर uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        अचर uint32_t extended = v | signExtend;
        फर्काऊ static_cast<int32_t>(extended);
    } अन्यथा {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        फर्काऊ static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_सङ्ख्या::toUQuad() अचर VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *एतत्);
    // We allow wide numbers that represent values <= 64 bits
    यदि (isDouble()) फर्काऊ static_cast<uint64_t>(toDouble());
    लागि (पूर्णाङ्क i = 2; i < शब्दहरू(); ++i) {
        यदि (m_data.अङ्क()[i].m_मान) {
            v3error("Value too wide for 64-bits expected in this context " << *एतत्);
            रोक;
        }
    }
    यदि (width() <= 32) फर्काऊ (static_cast<uint64_t>(toUInt()));
    फर्काऊ ((static_cast<uint64_t>(m_data.अङ्क()[1].m_मान) << 32ULL)
            | (static_cast<uint64_t>(m_data.अङ्क()[0].m_मान)));
}

int64_t V3_सङ्ख्या::toSQuad() अचर {
    यदि (isDouble()) फर्काऊ static_cast<int64_t>(toDouble());
    अचर uint64_t v = toUQuad();
    अचर uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    अचर uint64_t extended = v | signExtend;
    फर्काऊ static_cast<int64_t>(extended);
}

string V3_सङ्ख्या::toString() अचर {
    UASSERT(!isFourState(), "toString with 4-state " << *एतत्);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    यदि (isString()) फर्काऊ m_data.str();
    पूर्णाङ्क bit = width() - 1;
    जबसम्म ((bit % 8) != 7) ++bit;
    std::string str;
    लागि (; bit >= 0; bit -= 8) {
        अचर पूर्णाङ्क v = bits_मान(bit - 7, 8);
        यदि (v) str += static_cast<वर्ण>(v);
    }
    फर्काऊ str;
}

uint32_t V3_सङ्ख्या::edataWord(पूर्णाङ्क eword) अचर {
    UASSERT(!isFourState(), "edataWord with 4-state " << *एतत्);
    फर्काऊ m_data.अङ्क()[eword].m_मान;
}

uint8_t V3_सङ्ख्या::dataByte(पूर्णाङ्क byte) अचर {
    फर्काऊ (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

तार्किक V3_सङ्ख्या::isAllZ() अचर VL_MT_SAFE {
    यदि (isDouble() || isString()) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        अचर uint32_t mask = i == शब्दहरू() - 1 ? hiWordMask() : ~0U;
        यदि (((~v.m_मान & v.m_मान_X) & mask) != mask) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}
तार्किक V3_सङ्ख्या::isAllX() अचर VL_MT_SAFE {
    यदि (isDouble() || isString()) फर्काऊ असत्य;
    uint32_t mask = hiWordMask();
    लागि (पूर्णाङ्क i = शब्दहरू() - 1; i >= 0; --i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        यदि ((v.m_मान & v.m_मान_X) ^ mask) फर्काऊ असत्य;
        mask = ~0U;
    }
    फर्काऊ सत्य;
}
तार्किक V3_सङ्ख्या::isEqZero() अचर VL_MT_SAFE {
    यदि (isString()) फर्काऊ m_data.str().empty();
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        यदि (v.m_मान || v.m_मान_X) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}
तार्किक V3_सङ्ख्या::isNeqZero() अचर {
    यदि (isString()) फर्काऊ !m_data.str().empty();
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        यदि (v.m_मान & ~v.m_मान_X) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}
तार्किक V3_सङ्ख्या::isBitsZero(पूर्णाङ्क msb, पूर्णाङ्क lsb) अचर {
    लागि (पूर्णाङ्क i = lsb; i <= msb; ++i) {
        यदि (VL_UNLIKELY(!bitIs0(i))) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}
तार्किक V3_सङ्ख्या::isEqOne() अचर {
    यदि (m_data.अङ्क()[0].m_मान != 1 || m_data.अङ्क()[0].m_मान_X) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 1; i < शब्दहरू(); ++i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        यदि (v.m_मान || v.m_मान_X) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}
तार्किक V3_सङ्ख्या::isEqAllOnes(पूर्णाङ्क optwidth) अचर {
    // Correct number of zero bits/width matters
    यदि (!optwidth) optwidth = width();
    लागि (पूर्णाङ्क bit = 0; bit < optwidth; ++bit) {
        यदि (!bitIs1(bit)) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}
तार्किक V3_सङ्ख्या::isFourState() अचर VL_MT_SAFE {
    यदि (isDouble() || isString()) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        यदि (m_data.अङ्क()[i].m_मान_X) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}
तार्किक V3_सङ्ख्या::isAnyX() अचर VL_MT_SAFE {
    यदि (isDouble() || isString()) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        यदि (v.m_मान & v.m_मान_X) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}
तार्किक V3_सङ्ख्या::isAnyXZ() अचर {
    यदि (isDouble() || isString()) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        यदि (m_data.अङ्क()[i].m_मान_X) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}
तार्किक V3_सङ्ख्या::isAnyZ() अचर VL_MT_SAFE {
    यदि (isDouble() || isString()) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        अचर मान_And_X v = m_data.अङ्क()[i];
        यदि (~v.m_मान & v.m_मान_X) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}
तार्किक V3_सङ्ख्या::isLtXZ(अचर V3_सङ्ख्या& rhs) अचर {
    // Include X/Z in comparisons for sort ordering
    यदि (isString()) फर्काऊ toString() < rhs.toString();
    यदि (isDouble()) फर्काऊ toDouble() < rhs.toDouble();
    लागि (पूर्णाङ्क bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        यदि (bitIs1(bit) && rhs.bitIs0(bit)) फर्काऊ सत्य;
        यदि (rhs.bitIs1(bit) && bitIs0(bit)) फर्काऊ असत्य;
        यदि (bitIsXZ(bit)) फर्काऊ सत्य;
        यदि (rhs.bitIsXZ(bit)) फर्काऊ असत्य;
    }
    फर्काऊ असत्य;
}
पूर्णाङ्क V3_सङ्ख्या::गन्ती_X(पूर्णाङ्क lsb, पूर्णाङ्क nbits) अचर VL_MT_SAFE {
    पूर्णाङ्क गन्ती = 0;
    लागि (पूर्णाङ्क bitn = 0; bitn < nbits; ++bitn) {
        यदि (lsb + bitn >= width()) फर्काऊ गन्ती;
        यदि (bitIsX(lsb + bitn)) ++गन्ती;
    }
    फर्काऊ गन्ती;
}
पूर्णाङ्क V3_सङ्ख्या::गन्ती_Z(पूर्णाङ्क lsb, पूर्णाङ्क nbits) अचर VL_MT_SAFE {
    पूर्णाङ्क गन्ती = 0;
    लागि (पूर्णाङ्क bitn = 0; bitn < nbits; ++bitn) {
        यदि (lsb + bitn >= width()) फर्काऊ गन्ती;
        यदि (bitIsZ(lsb + bitn)) ++गन्ती;
    }
    फर्काऊ गन्ती;
}

पूर्णाङ्क V3_सङ्ख्या::widthToFit() अचर {
    लागि (पूर्णाङ्क bit = width() - 1; bit > 0; bit--) {
        यदि (!bitIs0(bit)) फर्काऊ bit + 1;
    }
    फर्काऊ 1;  // one bit even if number is == 0
}

uint32_t V3_सङ्ख्या::गन्ती_Bits(अचर V3_सङ्ख्या& ctrl) अचर {
    // Correct number of zero bits/width matters
    पूर्णाङ्क n = 0;
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        विकल्प (ctrl.bitIs(0)) {
        पक्ष '0':
            यदि (bitIs0(bit)) ++n;
            रोक;
        पक्ष '1':
            यदि (bitIs1(bit)) ++n;
            रोक;
        पक्ष 'x':
            यदि (bitIsX(bit)) ++n;
            रोक;
        पक्ष 'z':
            यदि (bitIsZ(bit)) ++n;
            रोक;
        }
    }
    फर्काऊ n;
}

uint32_t V3_सङ्ख्या::गन्ती_Bits(अचर V3_सङ्ख्या& ctrl1, अचर V3_सङ्ख्या& ctrl2,
                             अचर V3_सङ्ख्या& ctrl3) अचर {
    // Correct number of zero bits/width matters
    पूर्णाङ्क n = गन्ती_Bits(ctrl1);
    यदि (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += गन्ती_Bits(ctrl2);
    यदि ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += गन्ती_Bits(ctrl3);
    }
    फर्काऊ n;
}

uint32_t V3_सङ्ख्या::गन्ती_Ones() अचर {
    पूर्णाङ्क n = 0;
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (bitIs1(bit)) ++n;
    }
    फर्काऊ n;
}

uint32_t V3_सङ्ख्या::mostSetBitP1() अचर {
    लागि (पूर्णाङ्क bit = width() - 1; bit >= 0; bit--) {
        यदि (!bitIs0(bit)) फर्काऊ bit + 1;
    }
    फर्काऊ 0;
}

uint32_t V3_सङ्ख्या::leastSetBitP1() अचर {
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (!bitIs0(bit)) फर्काऊ bit + 1;
    }
    फर्काऊ 0;
}

//======================================================================

V3_सङ्ख्या& V3_सङ्ख्या::opBitsNonXZ(अचर V3_सङ्ख्या& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opBitsOne(अचर V3_सङ्ख्या& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opBitsXZ(अचर V3_सङ्ख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opBitsZ(अचर V3_सङ्ख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    फर्काऊ *एतत्;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_सङ्ख्या& V3_सङ्ख्या::opRedOr(अचर V3_सङ्ख्या& lhs) {
    // op i, 1 bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            फर्काऊ setSingleBits(1);
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opRedAnd(अचर V3_सङ्ख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 1;
    लागि (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs0(bit)) {
            फर्काऊ setSingleBits(0);
        } अन्यथा यदि (lhs.bitIs1(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opRedXor(अचर V3_सङ्ख्या& lhs) {
    // op i, 1 bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
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
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::op_गन्ती_Bits(अचर V3_सङ्ख्या& अभिव्यक्ति, अचर V3_सङ्ख्या& ctrl1, अचर V3_सङ्ख्या& ctrl2,
                                अचर V3_सङ्ख्या& ctrl3) {
    // Correct number of zero bits/width matters
    अङ्क_ASSERT_OP_ARGS4(अभिव्यक्ति, ctrl1, ctrl2, ctrl3);
    अङ्क_ASSERT_LOGIC_ARGS4(अभिव्यक्ति, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.अङ्क()[0].m_मान = अभिव्यक्ति.गन्ती_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_गन्ती_Ones(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) फर्काऊ setAllBitsX();
    setZero();
    m_data.अङ्क()[0].m_मान = lhs.गन्ती_Ones();
    opCleanThis();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opIsUnknown(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    फर्काऊ setSingleBits(lhs.isAnyXZ());
}
V3_सङ्ख्या& V3_सङ्ख्या::opOneHot(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    यदि (lhs.isFourState()) फर्काऊ setAllBitsX();
    फर्काऊ setSingleBits(lhs.गन्ती_Ones() == 1);
}
V3_सङ्ख्या& V3_सङ्ख्या::opOneHot0(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    यदि (lhs.isFourState()) फर्काऊ setAllBitsX();
    फर्काऊ setSingleBits(lhs.गन्ती_Ones() <= 1);
}
V3_सङ्ख्या& V3_सङ्ख्या::opCLog2(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) फर्काऊ setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    अचर पूर्णाङ्क adjust = (lhs.गन्ती_Ones() == 1) ? 0 : 1;
    लागि (पूर्णाङ्क bit = lhs.width() - 1; bit >= 0; bit--) {
        यदि (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            फर्काऊ *एतत्;
        }
    }
    setZero();
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opMostSetBitP1(अचर V3_सङ्ख्या& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) फर्काऊ setAllBitsX();
    लागि (पूर्णाङ्क bit = lhs.width() - 1; bit >= 0; bit--) {
        यदि (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            फर्काऊ *एतत्;
        }
    }
    setZero();
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogNot(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    वर्ण outc = 1;
    लागि (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            outc = 0;
            गच्छ last;
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
last:
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opNot(अचर V3_सङ्ख्या& lhs) {
    // Correct number of zero bits/width matters
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opAnd(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opOr(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opXor(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा यदि (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    फर्काऊ *एतत्;
}

शून्य V3_सङ्ख्या::fillBits(पूर्णाङ्क destLsb, पूर्णाङ्क width, वर्ण मान) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    यदि (!width) फर्काऊ;
    UASSERT(width <= एतत्->width() - destLsb, "Destination bit range exceeds number width");

    अचर uint32_t मान_Word = मान == '1' || मान == 'x' || मान == 1 || मान == 3 ? ~0U : 0;
    अचर uint32_t xWord = मान == 'z' || मान == 'x' || मान == 2 || मान == 3 ? ~0U : 0;
    अचर पूर्णाङ्क firstWord = VL_BITWORD_E(destLsb);
    अचर पूर्णाङ्क lastWord = VL_BITWORD_E(destLsb + width - 1);
    लागि (पूर्णाङ्क word = firstWord; word <= lastWord; ++word) {
        अचर पूर्णाङ्क wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        अचर पूर्णाङ्क wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        अचर uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        मान_And_X& dest = m_data.अङ्क()[word];
        dest.m_मान = (dest.m_मान & ~mask) | (मान_Word & mask);
        dest.m_मान_X = (dest.m_मान_X & ~mask) | (xWord & mask);
    }
}

शून्य V3_सङ्ख्या::copyBits(पूर्णाङ्क destLsb, अचर V3_सङ्ख्या& source, पूर्णाङ्क sourceLsb, पूर्णाङ्क width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    यदि (!width) फर्काऊ;
    UASSERT(width <= एतत्->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    अचर पूर्णाङ्क firstWord = VL_BITWORD_E(destLsb);
    अचर पूर्णाङ्क lastWord = VL_BITWORD_E(destLsb + width - 1);
    लागि (पूर्णाङ्क destWord = firstWord; destWord <= lastWord; ++destWord) {
        अचर पूर्णाङ्क destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        अचर पूर्णाङ्क destOffset = VL_BITBIT_E(destBit);
        अचर पूर्णाङ्क sourceBit = sourceLsb + destBit - destLsb;
        अचर पूर्णाङ्क sourceWord = VL_BITWORD_E(sourceBit);
        अचर पूर्णाङ्क sourceOffset = VL_BITBIT_E(sourceBit);
        अचर पूर्णाङ्क copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        अचर uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        अचर मान_And_X& sourceLo = source.m_data.अङ्क()[sourceWord];
        uint32_t मान = sourceLo.m_मान >> sourceOffset;
        uint32_t मान_X = sourceLo.m_मान_X >> sourceOffset;
        यदि (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            अचर मान_And_X& sourceHi = source.m_data.अङ्क()[sourceWord + 1];
            मान |= sourceHi.m_मान << (VL_EDATASIZE - sourceOffset);
            मान_X |= sourceHi.m_मान_X << (VL_EDATASIZE - sourceOffset);
        }

        मान_And_X& dest = m_data.अङ्क()[destWord];
        dest.m_मान = (dest.m_मान & ~mask) | ((मान << destOffset) & mask);
        dest.m_मान_X = (dest.m_मान_X & ~mask) | ((मान_X << destOffset) & mask);
    }
}

V3_सङ्ख्या& V3_सङ्ख्या::opConcat(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    यदि (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opLenN(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opRepl(अचर V3_सङ्ख्या& lhs,
                           अचर V3_सङ्ख्या& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    फर्काऊ opRepl(lhs, rhs.toUInt());
}

V3_सङ्ख्या& V3_सङ्ख्या::opRepl(अचर V3_सङ्ख्या& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    यदि (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    लागि (अचिन्हित बार = 0; बार < rhsval; ++बार) {
        अचर uint64_t destLsb = uint64_t{बार} * lhs.width();
        यदि (destLsb >= static_cast<uint32_t>(width())) रोक;
        copyBits(static_cast<पूर्णाङ्क>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<पूर्णाङ्क>(destLsb)));
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opStreamL(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    यदि (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    अचर पूर्णाङ्क ssize = std::min(rhs.toUInt(), static_cast<अचिन्हित>(lhs.width()));
    लागि (पूर्णाङ्क istart = 0; istart < lhs.width(); istart += ssize) {
        अचर पूर्णाङ्क ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogAnd(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण loutc = 0;
    वर्ण routc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            loutc = 1;
            रोक;
        }
        यदि (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    लागि (पूर्णाङ्क bit = 0; bit < rhs.width(); ++bit) {
        यदि (rhs.bitIs1(bit)) {
            routc = 1;
            रोक;
        }
        यदि (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    वर्ण outc = 'x';
    यदि (routc == 1 && loutc == 1) outc = 1;
    यदि (routc == 0 || loutc == 0) outc = 0;
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogOr(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < lhs.width(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            outc = 1;
            गच्छ last;
        }
        यदि (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    लागि (पूर्णाङ्क bit = 0; bit < rhs.width(); ++bit) {
        यदि (rhs.bitIs1(bit)) {
            outc = 1;
            गच्छ last;
        }
        यदि (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogIf(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या lnot = lhs;
    lnot.opLogNot(lhs);
    फर्काऊ opLogOr(lnot, rhs);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_सङ्ख्या ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    फर्काऊ opLogAnd(ifa, ifb);
}

V3_सङ्ख्या& V3_सङ्ख्या::opAtoN(अचर V3_सङ्ख्या& lhs, पूर्णाङ्क base) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::हटाओ(str.begin(), str.end(), '_'), str.end());

    यदि (base == AstAtoN::ATOREAL) फर्काऊ setDouble(std::म_से_भ(str.c_str()));

    त्रुटि_सं = 0;
    स्वतः v = std::म_से_दीर्घ(str.c_str(), nullptr, base);
    यदि (त्रुटि_सं != 0) v = 0;
    फर्काऊ setLongS(static_cast<int32_t>(v));
}

V3_सङ्ख्या& V3_सङ्ख्या::opPutcN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, अचर V3_सङ्ख्या& ths) {
    अङ्क_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    अचर int32_t i = rhs.toSInt();
    अचर int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    यदि (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    फर्काऊ setString(lstring);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGetcN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    अचर string lstring = lhs.toString();
    अचर int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    यदि (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    फर्काऊ setLong(v);
}

V3_सङ्ख्या& V3_सङ्ख्या::opSubstrN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, अचर V3_सङ्ख्या& ths) {
    अङ्क_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    अचर string lstring = lhs.toString();
    अचर int32_t i = rhs.toSInt();
    अचर int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    यदि (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) फर्काऊ setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    फर्काऊ setString(lstring.substr(i, j - i + 1));
}

V3_सङ्ख्या& V3_सङ्ख्या::opCompareNN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, तार्किक ignoreCase) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    पूर्णाङ्क result;
    अचर string lstring = lhs.toString();
    अचर string rstring = rhs.toString();
    यदि (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } अन्यथा {
        result = std::म_भेद(lstring.c_str(), rstring.c_str());
    }
    फर्काऊ setLongS(result);
}

V3_सङ्ख्या& V3_सङ्ख्या::opEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    यदि (lhs.isString()) फर्काऊ opEqN(lhs, rhs);
    यदि (lhs.isDouble()) फर्काऊ opEqD(lhs, rhs);
    वर्ण outc = 1;
    लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
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
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opNeq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    यदि (lhs.isString()) फर्काऊ opNeqN(lhs, rhs);
    यदि (lhs.isDouble()) फर्काऊ opNeqD(lhs, rhs);
    वर्ण outc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
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
    फर्काऊ setSingleBits(outc);
}

V3Hash V3_सङ्ख्या::toHash() अचर {
    // Shouldmatch isCaseEq
    यदि (isString()) फर्काऊ V3Hash{toString()};
    यदि (isDouble()) फर्काऊ V3Hash{toDouble()};
    V3Hash hash{width()};
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) hash += m_data.अङ्क()[i].m_मान;
    फर्काऊ hash;
}

तार्किक V3_सङ्ख्या::isCaseEq(अचर V3_सङ्ख्या& rhs) अचर {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    यदि (isString()) फर्काऊ toString() == rhs.toString();
    यदि (isDouble()) फर्काऊ toDouble() == rhs.toDouble();
    यदि (width() != rhs.width()) फर्काऊ असत्य;
    लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
        यदि (!(m_data.अङ्क()[i] == rhs.m_data.अङ्क()[i])) फर्काऊ असत्य;
    }
    फर्काऊ सत्य;
}

V3_सङ्ख्या& V3_सङ्ख्या::opCaseEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_सङ्ख्या& V3_सङ्ख्या::opCaseNeq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    यदि (lhs.isString()) {
        फर्काऊ opNeqN(lhs, rhs);
    } अन्यथा यदि (lhs.isDouble()) {
        फर्काऊ opNeqD(lhs, rhs);
    }
    लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            गच्छ last;
        }
    }
last:
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opWildEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 1;
    लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
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
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opWildNeq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
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
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGt(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        यदि (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGtS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    {
        अचर पूर्णाङ्क mbit = std::max(lhs.width() - 1, rhs.width() - 1);
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
            लागि (पूर्णाङ्क bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                यदि (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                यदि (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                यदि (lhs.bitIsXZ(bit)) outc = 'x';
                यदि (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    फर्काऊ setSingleBits(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGte(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या& eq = opEq(lhs, rhs);
    यदि (eq.isNeqZero()) फर्काऊ eq;  // Return true
    फर्काऊ opGt(lhs, rhs);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGteS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या& eq = opEq(lhs, rhs);
    यदि (eq.isNeqZero()) फर्काऊ eq;  // Return true
    फर्काऊ opGtS(lhs, rhs);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLt(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { फर्काऊ opGt(rhs, lhs); }
V3_सङ्ख्या& V3_सङ्ख्या::opLte(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { फर्काऊ opGte(rhs, lhs); }
V3_सङ्ख्या& V3_सङ्ख्या::opLtS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { फर्काऊ opGtS(rhs, lhs); }
V3_सङ्ख्या& V3_सङ्ख्या::opLteS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) { फर्काऊ opGteS(rhs, lhs); }

V3_सङ्ख्या& V3_सङ्ख्या::opShiftR(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) फर्काऊ setAllBitsX();
    setZero();
    यदि (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) फर्काऊ *एतत्;
    अचर uint32_t rhsval = rhs.toUInt();
    यदि (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<पूर्णाङ्क>(rhsval),
                 std::min(width(), lhs.width() - static_cast<पूर्णाङ्क>(rhsval)));
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opShiftRS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) फर्काऊ setAllBitsX();
    setZero();
    अचर तार्किक overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    यदि (!overflow) {
        अचर uint32_t rhsval = rhs.toUInt();
        यदि (rhsval < lbits) {
            अचर पूर्णाङ्क copyWidth = std::min(width(), static_cast<पूर्णाङ्क>(lbits - rhsval));
            copyBits(0, lhs, static_cast<पूर्णाङ्क>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            फर्काऊ *एतत्;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opShiftL(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) फर्काऊ setAllBitsX();
    setZero();
    यदि (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) फर्काऊ *एतत्;
    अचर uint32_t rhsval = rhs.toUInt();
    यदि (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<पूर्णाङ्क>(rhsval), lhs, 0,
                 std::min(width() - static_cast<पूर्णाङ्क>(rhsval), lhs.width()));
    }
    फर्काऊ *एतत्;
}

//======================================================================
// Ops - Arithmetic

V3_सङ्ख्या& V3_सङ्ख्या::opNegate(अचर V3_सङ्ख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) फर्काऊ setAllBitsX();
    V3_सङ्ख्या notlhs(&lhs, width());
    notlhs.opNot(lhs);
    अचर V3_सङ्ख्या one(&lhs, width(), 1);
    opAdd(notlhs, one);
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opAdd(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    लागि (पूर्णाङ्क word = 0; word < शब्दहरू(); ++word) {
        अचर uint64_t lwordval = lhs.m_data.अङ्क()[word].m_मान;
        अचर uint64_t rwordval = rhs.m_data.अङ्क()[word].m_मान;
        अचर uint64_t sum = lwordval + rwordval + carry;
        m_data.अङ्क()[word].m_मान = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opSub(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    V3_सङ्ख्या negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    फर्काऊ opAdd(lhs, negrhs);
}
V3_सङ्ख्या& V3_सङ्ख्या::opMul(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    setZero();
    यदि (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } अन्यथा {
        लागि (पूर्णाङ्क lword = 0; lword < lhs.शब्दहरू(); ++lword) {
            अचर uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.अङ्क()[lword].m_मान);
            यदि (lwordval == 0) जारी;
            लागि (पूर्णाङ्क rword = 0; rword < rhs.शब्दहरू(); ++rword) {
                अचर uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.अङ्क()[rword].m_मान);
                यदि (rwordval == 0) जारी;
                uint64_t mul = lwordval * rwordval;
                लागि (पूर्णाङ्क qword = lword + rword; qword < शब्दहरू(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.अङ्क()[qword].m_मान);
                    m_data.अङ्क()[qword].m_मान = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    यदि (mul == 0) रोक;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opMulS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    V3_सङ्ख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_सङ्ख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    अचर V3_सङ्ख्या qNoSign = opMul(lhsNoSign, rhsNoSign);
    यदि ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opDiv(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    यदि (rhs.isEqZero()) फर्काऊ setAllBitsXRemoved();
    यदि (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        फर्काऊ *एतत्;
    } अन्यथा {
        // Wide division
        फर्काऊ opModDivGuts(lhs, rhs, असत्य);
    }
}
V3_सङ्ख्या& V3_सङ्ख्या::opDivS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    यदि (rhs.isEqZero()) फर्काऊ setAllBitsXRemoved();
    V3_सङ्ख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_सङ्ख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    अचर V3_सङ्ख्या qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    यदि ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *एतत्);
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opModDiv(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    यदि (rhs.isEqZero()) फर्काऊ setAllBitsXRemoved();
    यदि (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        फर्काऊ *एतत्;
    } अन्यथा {
        // Wide modulus
        फर्काऊ opModDivGuts(lhs, rhs, सत्य);
    }
}
V3_सङ्ख्या& V3_सङ्ख्या::opModDivS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    यदि (rhs.isEqZero()) फर्काऊ setAllBitsXRemoved();
    V3_सङ्ख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_सङ्ख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    अचर V3_सङ्ख्या qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    यदि (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opModDivGuts(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, तार्किक is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    अचर पूर्णाङ्क l_शब्दहरू = lhs.शब्दहरू();
    अचर पूर्णाङ्क umsbp1 = lhs.mostSetBitP1();  // dividend
    अचर पूर्णाङ्क vmsbp1 = rhs.mostSetBitP1();  //भाजक
    यदि (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *एतत्);
        फर्काऊ *एतत्;
    }

    अचर पूर्णाङ्क uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    अचर पूर्णाङ्क vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    यदि (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        लागि (पूर्णाङ्क j = uw - 1; j >= 0; j--) {
            अचर uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.अङ्क()[j].m_मान));
            m_data.अङ्क()[j].m_मान = unw64 / static_cast<uint64_t>(rhs.m_data.अङ्क()[0].m_मान);
            k = unw64
                - (static_cast<uint64_t>(m_data.अङ्क()[j].m_मान)
                   * static_cast<uint64_t>(rhs.m_data.अङ्क()[0].m_मान));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *एतत् << " rem=0x" << std::hex
                                   << k << std::dec);
        यदि (is_modulus) {
            setZero();
            m_data.अङ्क()[0].m_मान = k;
        }
        opCleanThis();
        फर्काऊ *एतत्;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_शब्दहरू + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_शब्दहरू + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    लागि (पूर्णाङ्क i = 0; i < l_शब्दहरू; ++i) m_data.अङ्क()[i].m_मान = 0;
    लागि (पूर्णाङ्क i = 0; i < l_शब्दहरू + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    अचर पूर्णाङ्क s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    अचर uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    लागि (पूर्णाङ्क i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.अङ्क()[i].m_मान << s)
                | (shift_mask & (rhs.m_data.अङ्क()[i - 1].m_मान >> (32 - s)));
    }
    vn[0] = rhs.m_data.अङ्क()[0].m_मान << s;

    // Copy and shift dividend by same amount; may set new upper word
    यदि (s) {
        un[uw] = lhs.m_data.अङ्क()[uw - 1].m_मान >> (32 - s);
    } अन्यथा {
        un[uw] = 0;
    }
    लागि (पूर्णाङ्क i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.अङ्क()[i].m_मान << s)
                | (shift_mask & (lhs.m_data.अङ्क()[i - 1].m_मान >> (32 - s)));
    }
    un[0] = lhs.m_data.अङ्क()[0].m_मान << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    लागि (पूर्णाङ्क j = uw - vw; j >= 0; j--) {
        // Estimate
        अचर uint64_t unw64
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
        लागि (पूर्णाङ्क i = 0; i < vw; ++i) {
            अचर uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.अङ्क()[j].m_मान = qhat;  // Save quotient digit

        यदि (t < 0) {
            // Over subtracted; correct by adding back
            m_data.अङ्क()[j].m_मान--;
            k = 0;
            लागि (पूर्णाङ्क i = 0; i < vw; ++i) {
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
        लागि (पूर्णाङ्क i = 0; i < vw; ++i) {
            m_data.अङ्क()[i].m_मान = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        लागि (पूर्णाङ्क i = vw; i < l_शब्दहरू; ++i) m_data.अङ्क()[i].m_मान = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *एतत्);
        फर्काऊ *एतत्;
    } अन्यथा {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *एतत्);
        फर्काऊ *एतत्;
    }
}

V3_सङ्ख्या& V3_सङ्ख्या::opPow(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, तार्किक lsign, तार्किक rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) फर्काऊ setAllBitsX();
    यदि (rhs.isEqZero()) फर्काऊ setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    यदि (rsign && rhs.isNegative()) {
        यदि (lhs.isEqZero()) {
            फर्काऊ setAllBitsXRemoved();
        } अन्यथा यदि (lhs.isEqOne()) {
            फर्काऊ setValue1();
        } अन्यथा यदि (lsign && lhs.isEqAllOnes()) {
            यदि (rhs.bitIs1(0)) {
                फर्काऊ setAllBits1();  // -1^odd=-1
            } अन्यथा {
                फर्काऊ setValue1();  // -1^even=1
            }
        }
        फर्काऊ setZero();
    }
    यदि (lhs.isEqZero()) फर्काऊ setZero();
    setZero();
    m_data.अङ्क()[0].m_मान = 1;
    V3_सङ्ख्या power(&lhs, width());
    power.opAssign(lhs);
    लागि (पूर्णाङ्क bit = 0; bit < rhs.width(); ++bit) {
        यदि (bit > 0) {  // power = power*power
            V3_सङ्ख्या lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        यदि (rhs.bitIs1(bit)) {  // out *= power
            V3_सङ्ख्या lastOut(&lhs, width());
            lastOut.opAssign(*एतत्);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowSU(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    फर्काऊ opPow(lhs, rhs, सत्य, असत्य);
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowSS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    फर्काऊ opPow(lhs, rhs, सत्य, सत्य);
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowUS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    फर्काऊ opPow(lhs, rhs, असत्य, सत्य);
}

V3_सङ्ख्या& V3_सङ्ख्या::opBufIf1(अचर V3_सङ्ख्या& ens, अचर V3_सङ्ख्या& if1s) {
    अङ्क_ASSERT_OP_ARGS2(ens, if1s);
    अङ्क_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    लागि (पूर्णाङ्क bit = 0; bit < width(); ++bit) {
        यदि (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } अन्यथा {
            setBit(bit, 'z');
        }
    }
    फर्काऊ *एतत्;
}

// Sets all bits in range to the given value
V3_सङ्ख्या& V3_सङ्ख्या::opSetRange(uint32_t lsb, uint32_t width, वर्ण bit_मान) {
    यदि (lsb < static_cast<uint32_t>(एतत्->width())) {
        fillBits(static_cast<पूर्णाङ्क>(lsb),
                 static_cast<पूर्णाङ्क>(std::min(width, static_cast<uint32_t>(एतत्->width()) - lsb)),
                 bit_मान);
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opAssign(अचर V3_सङ्ख्या& lhs) { फर्काऊ opAssignNonXZ(lhs, असत्य); }
V3_सङ्ख्या& V3_सङ्ख्या::opAssignNonXZ(अचर V3_सङ्ख्या& lhs, तार्किक ignoreXZ) {
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
            लागि (पूर्णाङ्क word = 0; word < शब्दहरू(); ++word) {
                अचर uint32_t मान = word < lhs.शब्दहरू() ? lhs.m_data.अङ्क()[word].m_मान
                                                                & ~lhs.m_data.अङ्क()[word].m_मान_X
                                                          : 0;
                m_data.अङ्क()[word] = {मान, 0};
            }
            opCleanThis();
        }
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opNToI(अचर V3_सङ्ख्या& lhs) {
    // String to packed number
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    setZero();
    अचर string& str = lhs.toString();
    लागि (माप_प्रकार n = 0; n < str.length(); ++n) {
        अचर वर्ण c = str[str.length() - 1 - n];
        लागि (माप_प्रकार cbit = 0; cbit < 8; ++cbit) {
            अचर std::bitset<8> sbit{static_cast<अचिन्हित दीर्घ दीर्घ>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opExtendS(अचर V3_सङ्ख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    अचर पूर्णाङ्क copyWidth = std::min(width(), static_cast<पूर्णाङ्क>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opExtendXZ(अचर V3_सङ्ख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    अचर पूर्णाङ्क copyWidth = std::min(width(), static_cast<पूर्णाङ्क>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opClean(अचर V3_सङ्ख्या& lhs, uint32_t bits) { फर्काऊ opSel(lhs, bits - 1, 0); }

शून्य V3_सङ्ख्या::opCleanThis(तार्किक warnOnTruncation) {
    // Clean MSB of number
    अङ्क_ASSERT_LOGIC_ARGS1(*एतत्);
    अचर मान_And_X v = m_data.अङ्क()[शब्दहरू() - 1];
    अचर uint32_t new_मान_Msb = v.m_मान & hiWordMask();
    अचर uint32_t new_मान_XMsb = v.m_मान_X & hiWordMask();
    यदि (warnOnTruncation && (new_मान_Msb != v.m_मान || new_मान_XMsb != v.m_मान_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.अङ्क()[शब्दहरू() - 1] = {new_मान_Msb, new_मान_XMsb};
}

V3_सङ्ख्या& V3_सङ्ख्या::opSel(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& msb, अचर V3_सङ्ख्या& lsb) {
    अङ्क_ASSERT_OP_ARGS3(lhs, msb, lsb);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS2(msb, lsb);
    यदि (lsb.isFourState() || msb.isFourState()) फर्काऊ setAllBitsX();
    फर्काऊ opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_सङ्ख्या& V3_सङ्ख्या::opSel(अचर V3_सङ्ख्या& lhs, uint32_t msbval, uint32_t lsbval) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    पूर्णाङ्क copyWidth = 0;
    यदि (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        अचर uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<पूर्णाङ्क>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<पूर्णाङ्क>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    फर्काऊ *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opSelInto(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& lsb, पूर्णाङ्क width) {
    फर्काऊ opSelInto(lhs, lsb.toSInt(), width);
}

V3_सङ्ख्या& V3_सङ्ख्या::opSelInto(अचर V3_सङ्ख्या& lhs, पूर्णाङ्क lsbval, पूर्णाङ्क width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    अचर पूर्णाङ्क copyWidth = std::max(0, std::min({width, lhs.width(), एतत्->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    अचर पूर्णाङ्क fillWidth
        = std::max(0, std::min(width - copyWidth, एतत्->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    फर्काऊ *एतत्;
}

//======================================================================
// Ops - Floating point

V3_सङ्ख्या& V3_सङ्ख्या::opIToRD(अचर V3_सङ्ख्या& lhs, तार्किक isSigned) {
    // Correct number of zero bits/width matters
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_सङ्ख्या noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    द्विगुण d = 0;
    अचर तार्किक negate = isSigned && noxz.isNegative();
    यदि (negate) {
        अचर V3_सङ्ख्या noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    लागि (पूर्णाङ्क bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        यदि (noxz.bitIs1(bit)) d += exp2(bit);
    }
    यदि (negate) d = -d;
    फर्काऊ setDouble(d);
}
V3_सङ्ख्या& V3_सङ्ख्या::opRToIS(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_DOUBLE_ARGS1(lhs);
    अचर द्विगुण v = VL_TRUNC(lhs.toDouble());
    अचर int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    फर्काऊ setLongS(i);
}
V3_सङ्ख्या& V3_सङ्ख्या::opRToIRoundS(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_DOUBLE_ARGS1(lhs);
    अचर द्विगुण v = VL_ROUND(lhs.toDouble());
    setZero();
    सङ्घ {
        द्विगुण d;
        uint64_t q;
    } u;
    u.d = v;
    (शून्य)u.d;

    अचर पूर्णाङ्क exp = static_cast<पूर्णाङ्क>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    अचर पूर्णाङ्क lsb = exp - 52;
    अचर uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    यदि (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        लागि (पूर्णाङ्क bit = 0; bit <= 52; ++bit) {
            यदि (mantissa & (1ULL << bit)) {
                अचर पूर्णाङ्क outbit = bit + lsb;
                यदि (outbit >= 0) setBit(outbit, 1);
            }
        }
        यदि (v < 0) {
            अचर V3_सङ्ख्या noSign = *एतत्;
            opNegate(noSign);
        }
    }
    फर्काऊ *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opRealToBits(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    सङ्घ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    फर्काऊ setQuad(u.m_v);
}
V3_सङ्ख्या& V3_सङ्ख्या::opBitsToRealD(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    सङ्घ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    फर्काऊ setDouble(u.m_d);
}
V3_सङ्ख्या& V3_सङ्ख्या::opNegateD(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_DOUBLE_ARGS1(lhs);
    फर्काऊ setDouble(-lhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opAddD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opSubD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opMulD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opDivD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setDouble(घात(lhs.toDouble(), rhs.toDouble()));
}
V3_सङ्ख्या& V3_सङ्ख्या::opEqD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opNeqD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGtD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGteD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLtD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLteD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_सङ्ख्या& V3_सङ्ख्या::opConcatN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setString(lhs.toString() + rhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opReplN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    अङ्क_ASSERT_LOGIC_ARGS1(rhs);
    फर्काऊ opReplN(lhs, rhs.toUInt());
}
V3_सङ्ख्या& V3_सङ्ख्या::opReplN(अचर V3_सङ्ख्या& lhs, uint32_t rhsval) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    लागि (अचिन्हित बार = 0; बार < rhsval; ++बार) out += lhs.toString();
    फर्काऊ setString(out);
}
V3_सङ्ख्या& V3_सङ्ख्या::opCvtPackString(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    फर्काऊ setString(lhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opToLowerN(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    अचर std::string out = VString::downcase(lhs.toString());
    फर्काऊ setString(out);
}
V3_सङ्ख्या& V3_सङ्ख्या::opToUpperN(अचर V3_सङ्ख्या& lhs) {
    अङ्क_ASSERT_OP_ARGS1(lhs);
    अङ्क_ASSERT_STRING_ARGS1(lhs);
    अचर std::string out = VString::upcase(lhs.toString());
    फर्काऊ setString(out);
}

V3_सङ्ख्या& V3_सङ्ख्या::opEqN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toString() == rhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opNeqN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toString() != rhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGtN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toString() > rhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGteN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toString() >= rhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLtN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toString() < rhs.toString());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLteN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs) {
    अङ्क_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्क_ASSERT_STRING_ARGS2(lhs, rhs);
    फर्काऊ setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

शून्य V3_सङ्ख्या::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_पङ्क्ति* अचर fl = नूतन File_पङ्क्ति{File_पङ्क्ति::builtInFilename()};
    V3_सङ्ख्या अङ्क{fl, 32, 0};
    अङ्क.selfTestThis();
}

शून्य V3_सङ्ख्या::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonEqual(0, 0), सत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonEqual(1e19, 1e19), सत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonEqual(9, 0.0001), असत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonEqual(1, 1 + std::numeric_limits<द्विगुण>::epsilon()), सत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonEqual(0.009, 0.00899999999999999931998839741709), सत्य);

    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(0), सत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(1), सत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(-1), सत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(1.0001), असत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(0.9999), असत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(-1.0001), असत्य);
    UASSERT_SELFTEST(V3_सङ्ख्या::epsilonIntegral(-0.9999), असत्य);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
