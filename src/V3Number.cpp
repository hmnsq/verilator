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

constexpr पूर्णांक MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define अंक_ASSERT_OP_ARGS1(arg1) \
    UASSERT((हे != &(arg1)), "Number operation called with same source and dest")
#define अंक_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((हे != &(arg1) && हे != &(arg2)), \
            "Number operation called with same source and dest")
#define अंक_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((हे != &(arg1) && हे != &(arg2) && हे != &(arg3)), \
            "Number operation called with same source and dest")
#define अंक_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((हे != &(arg1) && हे != &(arg2) && हे != &(arg3) && हे != &(arg4)), \
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

शून्य V3_संख्या::v3errorEnd(स्थिरांक std::ostringstream& str) स्थिरांक VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    जर (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } अन्यथा जर (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } अन्यथा {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

शून्य V3_संख्या::v3errorEndFatal(स्थिरांक std::ostringstream& str) स्थिरांक
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_संख्या::V3_संख्या(Ast_गाठ* nodep, VerilogStringLiteral, स्थिरांक string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    जर (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } अन्यथा {
        init(nodep, std::max<पूर्णांक>(str.length() * 8, 1));
        साठी (अचिन्हित pos = 0; pos < str.length(); ++pos) {
            स्थिरांक पूर्णांक topos = str.length() - 1 - pos;
            मूल्य_And_X& v = m_data.अंक()[topos / 4];
            साठी (पूर्णांक bit = 0; bit < 8; ++bit) {
                जर (str[pos] & (1UL << bit)) v.m_मूल्य |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = सत्य;
    opCleanThis(सत्य);
}

V3_संख्या::V3_संख्या(Ast_गाठ* nodep, स्थिरांक Ast_गाठ_DType* nodedtypep) {
    जर (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } अन्यथा जर (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } अन्यथा जर (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } अन्यथा {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

शून्य V3_संख्या::create(स्थिरांक अक्षर* sourcep) {
    m_data.setLogic();
    स्थिरांक अक्षर* मूल्य_startp = sourcep;
    साठी (स्थिरांक अक्षर* cp = sourcep; *cp; ++cp) {
        जर (*cp == '\'') {
            मूल्य_startp = cp + 1;
            थांबा;
        }
    }

    तार्किक userSized = असत्य;
    तार्किक unbased = असत्य;
    अक्षर base = '\0';
    जर (मूल्य_startp != sourcep) {  // Has a '
        string widthn;
        स्थिरांक अक्षर* cp = sourcep;
        साठी (; *cp; ++cp) {
            जर (*cp == '\'') {
                ++cp;
                थांबा;
            }
            जर (*cp != '_') widthn += *cp;
        }
        पर्यंत (*cp == '_') cp++;
        जर (*cp && std::छोटे(*cp) == 's') {
            ++cp;
            isSigned(सत्य);
        }
        जर (*cp) {
            base = *cp;
            ++cp;
        }
        मूल्य_startp = cp;

        userSized = widthn.length() != 0;
        जर (std::म_से_प(widthn.c_str())) {
            जर (std::म_से_प(widthn.c_str()) < 0
                || std::म_से_प(widthn.c_str()) > v3Global.opt.max_अंक_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_अंक_Width(), सत्य);
            } अन्यथा {
                width(std::म_से_प(widthn.c_str()), सत्य);
            }
        }
    } अन्यथा {
        unbased = सत्य;
        base = 'd';
    }

    साठी (पूर्णांक i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    जर (base == '0') {
        width(1, असत्य);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = सत्य;
    } अन्यथा जर (base == '1') {
        width(1, असत्य);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = सत्य;
    } अन्यथा जर (std::छोटे(base) == 'z') {
        width(1, असत्य);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = सत्य;
    } अन्यथा जर (std::छोटे(base) == 'x') {
        width(1, असत्य);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = सत्य;
    }
    // Otherwise...
    अन्यथा जर (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        स्थिरांक पूर्णांक widthBound = std::max<पूर्णांक>(32, std::म_माप(मूल्य_startp) * 4);
        width(widthBound, असत्य);  // Will change width below
        जर (unbased) isSigned(सत्य);  // Also says the spec.
    }

    // Ignore leading blanks
    पर्यंत (*मूल्य_startp == '_' || std::है_खाली(*मूल्य_startp)) ++मूल्य_startp;
    जर (!*मूल्य_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    जर (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    पूर्णांक obit = 0;  // Start at LSB
    पूर्णांक base_align = 1;
    जर (std::छोटे(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        पर्यंत (*मूल्य_startp == '_' || *मूल्य_startp == '0') ++मूल्य_startp;
        // Convert decimal number to hex
        पूर्णांक olen = 0;
        uint32_t val = 0;
        पूर्णांक got_x = 0;
        पूर्णांक got_z = 0;
        पूर्णांक got_01 = 0;
        तार्किक warnedTooMany = असत्य;
        साठी (स्थिरांक अक्षर* cp = मूल्य_startp; *cp; ++cp) {
            निवड (std::छोटे(*cp)) {
            स्थिती '0':  // FALLTHRU
            स्थिती '1':  // FALLTHRU
            स्थिती '2':  // FALLTHRU
            स्थिती '3':  // FALLTHRU
            स्थिती '4':  // FALLTHRU
            स्थिती '5':  // FALLTHRU
            स्थिती '6':  // FALLTHRU
            स्थिती '7':  // FALLTHRU
            स्थिती '8':  // FALLTHRU
            स्थिती '9': {
                जर (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.अंक()[0].m_मूल्य = val;
                } अन्यथा {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_संख्या product(हे, width() + 4);  // +4 for overflow detection
                    स्थिरांक V3_संख्या ten(हे, width() + 4, 10);
                    स्थिरांक V3_संख्या addend(हे, width(), (*cp - '0'));
                    product.opMul(*हे, ten);
                    opAdd(product, addend);
                    जर (!warnedTooMany && product.bits_मूल्य(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = सत्य;
                    }
                }
                ++olen;
                got_01 = 1;
                थांबा;
            }
            स्थिती 'z':
            स्थिती '?': {
                got_z = 1;
                जर (!userSized) width(32, असत्य);
                setAllBitsZ();
                थांबा;
            }
            स्थिती 'x': {
                got_x = 1;
                जर (!userSized) width(32, असत्य);
                setAllBitsX();
                थांबा;
            }
            स्थिती '_': थांबा;
            पूर्वनिर्धारित: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                थांबा;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        जर ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << मूल्य_startp);
        }
    } अन्यथा {
        // Convert bin/octal number to hex
        साठी (स्थिरांक अक्षर* cp = मूल्य_startp + std::म_माप(मूल्य_startp) - 1; cp >= मूल्य_startp;
             cp--) {
            जर (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                थांबा;
            }
            निवड (std::छोटे(base)) {
            स्थिती 'b': {
                base_align = 1;
                निवड (std::छोटे(*cp)) {
                स्थिती '0': setBit(obit++, 0); थांबा;
                स्थिती '1': setBit(obit++, 1); थांबा;
                स्थिती 'z':
                स्थिती '?': setBit(obit++, 'z'); थांबा;
                स्थिती 'x': setBit(obit++, 'x'); थांबा;
                स्थिती '_': थांबा;
                पूर्वनिर्धारित: v3error("Illegal character in binary constant: " << *cp);
                }
                थांबा;
            }

            स्थिती 'o':
            स्थिती 'c': {
                base_align = 3;
                निवड (std::छोटे(*cp)) {  // clang-format off
                स्थिती '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  थांबा;
                स्थिती '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  थांबा;
                स्थिती '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  थांबा;
                स्थिती '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  थांबा;
                स्थिती '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  थांबा;
                स्थिती '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  थांबा;
                स्थिती '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  थांबा;
                स्थिती '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  थांबा;
                स्थिती 'z':
                स्थिती '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); थांबा;
                स्थिती 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); थांबा;
                स्थिती '_': थांबा;
                // clang-format on
                पूर्वनिर्धारित: v3error("Illegal character in octal constant");
                }
                थांबा;
            }

            स्थिती 'h':  // FALLTHRU
            स्थिती 'x': {
                base_align = 4;
                निवड (std::छोटे(*cp)) {  // clang-format off
                स्थिती '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); थांबा;
                स्थिती '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); थांबा;
                स्थिती '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); थांबा;
                स्थिती '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); थांबा;
                स्थिती '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); थांबा;
                स्थिती '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); थांबा;
                स्थिती '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); थांबा;
                स्थिती '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); थांबा;
                स्थिती '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); थांबा;
                स्थिती '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); थांबा;
                स्थिती 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); थांबा;
                स्थिती 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); थांबा;
                स्थिती 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); थांबा;
                स्थिती 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); थांबा;
                स्थिती 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); थांबा;
                स्थिती 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); थांबा;
                स्थिती 'z':
                स्थिती '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); थांबा;
                स्थिती 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); थांबा;
                    // clang-format on
                स्थिती '_': थांबा;
                पूर्वनिर्धारित:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    थांबा;
                }  // LCOV_EXCL_STOP
                थांबा;
            }
            पूर्वनिर्धारित:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                थांबा;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    जर (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), असत्य);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    पर्यंत (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(सत्य);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

शून्य V3_संख्या::warnTooMany(स्थिरांक string& मूल्य) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << मूल्य << "'");
}

शून्य V3_संख्या::nodep(Ast_गाठ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    जर (!nodep) परत;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

तार्किक V3_संख्या::epsilonEqual(द्विगुण a, द्विगुण b) {
    परत std::भ_असल(a - b)
           <= (std::numeric_limits<द्विगुण>::epsilon() * std::max(1.0, std::max(a, b)));
}

तार्किक V3_संख्या::epsilonIntegral(द्विगुण a) {
    स्थिरांक द्विगुण dint = static_cast<द्विगुण>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    परत epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

पूर्णांक V3_संख्या::log2b(uint32_t अंक) {
    // See also opCLog2
    साठी (पूर्णांक bit = 31; bit > 0; bit--) {
        जर (अंक & (1ULL << bit)) परत bit;
    }
    परत 0;
}

पूर्णांक V3_संख्या::log2bQuad(uint64_t अंक) {
    जर (अंक >> 32ULL) परत 32 + log2b(अंक >> 32ULL);
    परत log2b(अंक);
}

//======================================================================
// Setters

V3_संख्या& V3_संख्या::setZero() {
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};
    परत *हे;
}
V3_संख्या& V3_संख्या::setQuad(uint64_t मूल्य) {
    setZero();
    m_data.अंक()[0].m_मूल्य = मूल्य & 0xffffffffULL;
    जर (width() > 32) m_data.अंक()[1].m_मूल्य = (मूल्य >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::setLong(uint32_t मूल्य) {
    setZero();
    m_data.अंक()[0].m_मूल्य = मूल्य;
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::setLongS(int32_t मूल्य) {
    setZero();
    संघ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = मूल्य;
    (शून्य)u.s;
    m_data.अंक()[0].m_मूल्य = u.u;
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::setDouble(द्विगुण मूल्य) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    संघ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.d = मूल्य;
    (शून्य)u.d;
    साठी (पूर्णांक i = 2; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};
    m_data.अंक()[0].m_मूल्य = u.u[0];
    m_data.अंक()[1].m_मूल्य = u.u[1];
    परत *हे;
}
V3_संख्या& V3_संख्या::setSingleBits(अक्षर मूल्य) {
    साठी (पूर्णांक i = 1 /*upper*/; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};
    m_data.अंक()[0] = {(मूल्य == '1' || मूल्य == 'x' || मूल्य == 1 || मूल्य == 3),
                       (मूल्य == 'z' || मूल्य == 'x' || मूल्य == 2 || मूल्य == 3)};
    परत *हे;
}

V3_संख्या& V3_संख्या::setAllBits0() {
    setZero();
    परत *हे;
}
V3_संख्या& V3_संख्या::setAllBits1() {
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {~0U, 0};
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {~0U, ~0U};
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::setAllBitsZ() {
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {0, ~0U};
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::setAllBitsXRemoved() {
    जर (!v3Global.constRemoveXs()) {
        परत setAllBitsX();
    } अन्यथा {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        जर (v3Global.opt.xAssign() == "1") {
            परत setAllBits1();
        } अन्यथा {
            परत setAllBits0();
        }
    }
}
V3_संख्या& V3_संख्या::setValue1() {
    m_data.अंक()[0] = {1, 0};
    साठी (पूर्णांक i = 1; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};
    परत *हे;
}

शून्य V3_संख्या::setBitX0(पूर्णांक bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_संख्या& V3_संख्या::setMask(पूर्णांक nbits, पूर्णांक lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    परत *हे;
}

//======================================================================
// ACCESSORS - as strings

string V3_संख्या::ascii(तार्किक prefixed, तार्किक cleanVerilog) स्थिरांक VL_MT_STABLE {
    // Correct number of zero bits/width matters
    जर (is1Step()) परत "1step";
    जर (isNull()) परत "null";
    std::ostringstream out;
    जर (isDouble()) {
        out.precision(17);
        जर (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << toDouble();
            जर (toDouble() == न्यूनमान(toDouble())) out << ".0";
        }
        परत out.str();
    } अन्यथा जर (isString()) {
        परत '"' + toString() + '"';
    } अन्यथा {
        जर (VL_UNCOVERABLE((m_data.अंक()[शब्द() - 1].m_मूल्य | m_data.अंक()[शब्द() - 1].m_मूल्य_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    जर (prefixed) {
        जर (sized()) {
            out << width() << "'";
        } अन्यथा जर (autoExtend() && width() == 1) {
            out << "'";
            जर (bitIs0(0)) {
                out << '0';
            } अन्यथा जर (bitIs1(0)) {
                out << '1';
            } अन्यथा जर (bitIsZ(0)) {
                out << 'z';
            } अन्यथा {
                out << 'x';
            }
            परत out.str();
        } अन्यथा {
            जर (cleanVerilog) {
                out << "'";
            } अन्यथा {
                out << "?" << width() << "?";
            }
        }
        जर (isSigned()) out << "s";
    }

    स्थिरांक तार्किक द्विमान = (isFourState()
#ifdef V3_संख्या_ASCII_द्विमान
                         // cppcheck-suppress konwnConditionTrueFalse
                         || सत्य
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    जर (द्विमान) {
        out << "b";
        out << displayed("%0b");
    } अन्यथा {
        जर (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    जर (isNull()) {
        जर (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << " [null]";
        }
    }
    परत out.str();
}

तार्किक V3_संख्या::displayedFmtHasArg(अक्षर format, तार्किक isScan) {
    // Is this a format letter that takes an argument in runtime?
    (शून्य)isScan;
    निवड (std::छोटे(format)) {
    स्थिती 'b': परत सत्य;  //द्विमान
    स्थिती 'c': परत सत्य;  // Character
    स्थिती 'd': परत सत्य;  // Decimal
    स्थिती 'e': परत सत्य;  // Floating
    स्थिती 'f': परत सत्य;  // Floating
    स्थिती 'g': परत सत्य;  // Floating
    स्थिती 'h': परत सत्य;  // Hex
    स्थिती 'o': परत सत्य;  // Octal
    स्थिती 'p': परत सत्य;  // Pattern
    स्थिती 's': परत सत्य;  // String
    स्थिती 't': परत सत्य;  // Time
    स्थिती 'u': परत सत्य;  // Packed 2-state
    स्थिती 'v': परत सत्य;  // Strength
    स्थिती 'x': परत सत्य;  // Hex
    स्थिती 'z': परत सत्य;  // Packed 4-state
    स्थिती '?': परत सत्य;  // Internal: Compute format in V3Width based on data type
    पूर्वनिर्धारित: परत असत्य;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_संख्या::displayPad(माप_प्रकार fmtsize, अक्षर pad, तार्किक डावा, स्थिरांक string& in) VL_PURE {
    string padding;
    जर (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    परत डावा ? (in + padding) : (padding + in);
}

string V3_संख्या::displayedEnumName(स्थिरांक AstEnumItem* itemp) {
    स्थिरांक string name = VIdProtect::protect(itemp->prettyName());
    परत VString::isIdentifier(name) && !std::है_अंक(static_cast<अचिन्हित अक्षर>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_संख्या::displayedEnumName(स्थिरांक AstEnumDType* dtypep) स्थिरांक VL_MT_STABLE {
    V3_संख्या मूल्य{हे, dtypep->width()};
    मूल्य.opAssign(*हे);
    साठी (स्थिरांक AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        स्थिरांक AstConst* स्थिरांक constp = VN_AS(itemp->valuep(), Const);
        जर (!constp->अंक().isAnyXZ() && मूल्य.isCaseEq(constp->अंक()))
            परत displayedEnumName(itemp);
    }
    परत "";
}

string V3_संख्या::displayedEnum(स्थिरांक AstSFormatArg* argp,
                               स्थिरांक string& vformat) स्थिरांक VL_MT_STABLE {
    स्थिरांक AstEnumDType* स्थिरांक dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_संख्या मूल्य{हे, dtypep->width()};
    मूल्य.opAssign(*हे);
    स्थिरांक VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    स्थिरांक अक्षर fmt = std::छोटे(vformat.back());
    जर (fmt != 'p' && fmt != 's') परत मूल्य.displayed(argp, vformat, numericAttr);
    string name = मूल्य.displayedEnumName(dtypep);
    जर (name.empty()) name = मूल्य.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_संख्या name_अंक{हे};
    name_अंक.setString(name);
    परत name_अंक.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_संख्या::displayed(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& vformat,
                           स्थिरांक VFormatAttr& formatAttr) स्थिरांक VL_MT_STABLE {
    परत displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_संख्या::displayed(File_ओळ* fl, स्थिरांक string& vformat,
                           स्थिरांक VFormatAttr& formatAttr) स्थिरांक VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    स्वतः pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *हे);
    ++pos;
    तार्किक डावा = असत्य;
    जर (pos[0] == '-') {
        डावा = सत्य;
        ++pos;
    }
    string fmtsize;
    साठी (; pos != vformat.cend() && (std::है_अंक(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    अक्षर fmt = std::छोटे(pos[0]);

    // Override user's code for certain data-type determined arguments
    जर (formatAttr.isComplex()) {
        जर (fmt != 'p') fmt = 's';  // override
    } अन्यथा जर (formatAttr.isString()) {
        जर (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_संख्या strwide{fl, static_cast<पूर्णांक>(toString().length()) * 8, 0};
            strwide.opNToI(*हे);
            परत strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        जर (fmt != 'p') fmt = 's';  // override
    } अन्यथा जर (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        जर (fmt == 'p') {
            जर (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } अन्यथा {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } अन्यथा जर (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        जर (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_संख्या nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*हे);
            परत nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    निवड (fmt) {
    स्थिती 'c': {
        जर (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        स्थिरांक अचिन्हित पूर्णांक v = bits_मूल्य(0, 8);
        str.ढकला_back(static_cast<अक्षर>(v));
        परत str;
    }
    स्थिती 'e':  // FALLTHRU
    स्थिती 'f':  // FALLTHRU
    स्थिती 'g': {
        स्थिरांक द्विगुण n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        अक्षर tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        परत tmp;
    }
    स्थिती 's': {
        जर (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } अन्यथा {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            पूर्णांक bit = width() - 1;
            तार्किक start = सत्य;
            पर्यंत ((bit % 8) != 7) ++bit;
            साठी (; bit >= 0; bit -= 8) {
                स्थिरांक पूर्णांक v = bits_मूल्य(bit - 7, 8);
                जर (!start || v) {
                    str += static_cast<अक्षर>((v == 0) ? ' ' : v);
                    start = असत्य;  // Drop leading 0s
                } अन्यथा {
                    जर (fmtsize != "0") str += ' ';
                }
            }
        }
        स्थिरांक माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', डावा, str);
        परत str;
    }
    स्थिती 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        जर (formatAttr.isDouble()) {
            स्थिरांक द्विगुण n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            अक्षर tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            परत tmp;
        }
        जर (formatAttr.isString())
            परत '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        जर (formatAttr.isComplex()) परत toString();
        परत "%p";
    }
    स्थिती 'b':  // FALLTHRU
    स्थिती 'o':  // FALLTHRU
    स्थिती 'h':  // FALLTHRU
    स्थिती 'x': {
        पूर्णांक bit = width() - 1;
        जर (डावा || !fmtsize.empty()) {
            पर्यंत (bit && bitIs0(bit)) --bit;
        }
        निवड (fmt) {
        स्थिती 'b': {
            साठी (; bit >= 0; --bit) {
                जर (bitIs0(bit)) {
                    str += '0';
                } अन्यथा जर (bitIs1(bit)) {
                    str += '1';
                } अन्यथा जर (bitIsZ(bit)) {
                    str += 'z';
                } अन्यथा {
                    str += 'x';
                }
            }
            थांबा;
        }
        स्थिती 'o': {
            पर्यंत ((bit % 3) != 2) ++bit;
            साठी (; bit > 0; bit -= 3) {
                स्थिरांक पूर्णांक अंक_X = मोजणी_X(bit - 2, 3);
                स्थिरांक पूर्णांक अंक_Z = मोजणी_Z(bit - 2, 3);
                जर (अंक_X == 3 || अंक_X == width() - (bit - 2)) {
                    str += 'x';
                    सुरूठेवा;
                }
                जर (अंक_Z == 3 || अंक_Z == width() - (bit - 2)) {
                    str += 'z';
                    सुरूठेवा;
                }
                जर (अंक_X > 0) {
                    str += 'X';
                    सुरूठेवा;
                }
                जर (अंक_Z > 0) {
                    str += 'Z';
                    सुरूठेवा;
                }
                स्थिरांक पूर्णांक v = bits_मूल्य(bit - 2, 3);
                str += static_cast<अक्षर>('0' + v);
            }
            थांबा;
        }
        पूर्वनिर्धारित: {  // h/x
            पर्यंत ((bit % 4) != 3) ++bit;
            साठी (; bit > 0; bit -= 4) {
                स्थिरांक पूर्णांक अंक_X = मोजणी_X(bit - 3, 4);
                स्थिरांक पूर्णांक अंक_Z = मोजणी_Z(bit - 3, 4);
                जर (अंक_X == 4 || अंक_X == width() - (bit - 3)) {
                    str += 'x';
                    सुरूठेवा;
                }
                जर (अंक_Z == 4 || अंक_Z == width() - (bit - 3)) {
                    str += 'z';
                    सुरूठेवा;
                }
                जर (अंक_X > 0) {
                    str += 'X';
                    सुरूठेवा;
                }
                जर (अंक_Z > 0) {
                    str += 'Z';
                    सुरूठेवा;
                }
                स्थिरांक पूर्णांक v = bits_मूल्य(bit - 3, 4);
                जर (v >= 10) {
                    str += static_cast<अक्षर>('a' + v - 10);
                } अन्यथा {
                    str += static_cast<अक्षर>('0' + v);
                }
            }
            थांबा;
        }
        }  // switch
        स्थिरांक माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, (डावा ? ' ' : '0'), डावा, str);
        परत str;
    }  // case b/d/x/o
    स्थिती 't':  // Time
    स्थिती 'd': {  // Unsigned decimal
        स्थिरांक तार्किक issigned = formatAttr.isSigned();
        जर (fmtsize == "" && !डावा) {
            स्थिरांक द्विगुण mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            द्विगुण dchars = mantissabits / 3.321928094887362 + 1.0;
            जर (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<पूर्णांक>(dchars));
        }
        तार्किक hasXZ = असत्य;
        जर (isAllX()) {
            str = "x";
            hasXZ = सत्य;
        } अन्यथा जर (isAllZ()) {
            str = "z";
            hasXZ = सत्य;
        } अन्यथा जर (isAnyX()) {
            str = "X";
            hasXZ = सत्य;
        } अन्यथा जर (isAnyZ()) {
            str = "Z";
            hasXZ = सत्य;
        }
        जर (!hasXZ) {
            जर (issigned) {
                जर (width() > 64) {
                    str = toDecimalS();
                } अन्यथा {
                    str = cvtToStr(toSQuad());
                }
            } अन्यथा {
                जर (width() > 64) {
                    str = toDecimalU();
                } अन्यथा {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        स्थिरांक तार्किक zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !डावा;
        // fmtsize might have changed since we parsed the %fmtsize
        स्थिरांक माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), डावा, str);
        परत str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    स्थिती 'u': {  // Packed 2-state
        साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
            स्थिरांक uint32_t v = m_data.अंक()[i].m_मूल्य;
            str += static_cast<अक्षर>((v >> 0) & 0xff);
            str += static_cast<अक्षर>((v >> 8) & 0xff);
            str += static_cast<अक्षर>((v >> 16) & 0xff);
            str += static_cast<अक्षर>((v >> 24) & 0xff);
        }
        परत str;
    }
    स्थिती 'z': {  // Packed 4-state
        साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
            स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
            str += static_cast<अक्षर>((v.m_मूल्य >> 0) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य >> 8) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य >> 16) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य >> 24) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 0) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 8) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 16) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 24) & 0xff);
        }
        परत str;
    }
    स्थिती 'v': {  // Strength
        पूर्णांक bit = width() - 1;
        साठी (; bit >= 0; bit--) {
            जर (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            अन्यथा जर (bitIs1(bit)) {
                str += "St1 ";
            } अन्यथा जर (bitIsZ(bit)) {
                str += "StZ ";
            } अन्यथा {
                str += "StX";
            }
        }
        परत str;
    }
    पूर्वनिर्धारित: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_संख्या::emitC() स्थिरांक VL_MT_STABLE {
    constexpr माप_प्रकार bufsize = 1000;
    अक्षर sbuf[bufsize];
    string result;
    जर (isNull()) {
        परत "VlNull{}";
    } अन्यथा जर (isDouble()) {
        स्थिरांक द्विगुण dnum = toDouble();
        जर (std::isinf(dnum)) {
            जर (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } अन्यथा जर (std::isnan(dnum)) {
            जर (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } अन्यथा {
            स्थिरांक अक्षर* स्थिरांक fmt = (static_cast<पूर्णांक>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            परत sbuf;
        }
    } अन्यथा जर (isString()) {
        स्थिरांक string strg = toString();
        स्थिरांक string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        परत '"' + quoted + "\"s";
    } अन्यथा जर (शब्द() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(शब्द()) + ">{{";
        जर (शब्द() > 4) result += '\n';
        साठी (पूर्णांक n = 0; n < शब्द(); ++n) {
            जर (n) result += ((n % 4) ? ", " : ",\n");
            (शून्य)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        जर (शब्द() > 4) result += '\n';
        result += "}}";
    } अन्यथा जर (शब्द() == 2) {  // Quad
        स्थिरांक uint64_t qnum = static_cast<uint64_t>(toUQuad());
        स्थिरांक अक्षर* स्थिरांक fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        परत sbuf;
    } अन्यथा {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        स्थिरांक uint32_t unum = toUInt();
        स्थिरांक अक्षर* स्थिरांक fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        परत sbuf;
    }
    परत result;
}

string V3_संख्या::toDecimalS() स्थिरांक VL_MT_STABLE {
    // Correct number of zero bits/width matters
    जर (isNegative()) {
        V3_संख्या lhsNoSign = *हे;
        lhsNoSign.opNegate(*हे);
        परत "-"s + lhsNoSign.toDecimalU();
    } अन्यथा {
        परत toDecimalU();
    }
}

string V3_संख्या::toDecimalU() स्थिरांक VL_MT_STABLE {
    स्थिरांक पूर्णांक maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_संख्या bcd(हे, maxdecwidth + 4);
    V3_संख्या tmp(हे, maxdecwidth + 4);
    V3_संख्या tmp2(हे, maxdecwidth + 4);

    पूर्णांक from_bit = width() - 1;
    // Skip all leading zeros
    साठी (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    साठी (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        साठी (पूर्णांक nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            जर (bcd.bits_मूल्य(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_संख्या(हे, 32, 1));
        // bcd[0] = this[from_bit]
        जर (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    पूर्णांक lsb = (maxdecwidth - 1) & ~3;
    साठी (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        जर (bcd.bits_मूल्य(lsb, 4)) थांबा;
    }
    साठी (; lsb >= 0; lsb -= 4) {
        output += static_cast<अक्षर>('0' + bcd.bits_मूल्य(lsb, 4));  // 0..9
    }
    परत output;
}

string V3_संख्या::toRealString() स्थिरांक VL_MT_STABLE {
    स्थिरांक द्विगुण मूल्य = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << मूल्य;
    string result = out.str();
    जर (std::म_से_भग्न(result.c_str(), nullptr) != मूल्य) {
        out.str("");
        out.precision(17);
        out << मूल्य;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    जर (result.find_first_of(".en") == string::npos) result += ".0";
    परत result;
}

//======================================================================
// ACCESSORS - as numbers

तार्किक V3_संख्या::fitsInUInt() स्थिरांक VL_MT_SAFE {
    जर (isFourState()) परत असत्य;
    // We allow wide numbers that represent values <= 32 bits
    साठी (पूर्णांक i = 1; i < शब्द(); ++i) {
        जर (m_data.अंक()[i].m_मूल्य) परत असत्य;
    }
    परत सत्य;
}
uint32_t V3_संख्या::toUInt() स्थिरांक VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *हे);
    // We allow wide numbers that represent values <= 32 bits
    जर (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *हे);
    }
    परत m_data.अंक()[0].m_मूल्य;
}

द्विगुण V3_संख्या::toDouble() स्थिरांक VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    संघ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.अंक()[0].m_मूल्य;
    u.u[1] = m_data.अंक()[1].m_मूल्य;
    परत u.d;
}

int32_t V3_संख्या::toSInt() स्थिरांक VL_MT_SAFE {
    जर (isSigned()) {
        स्थिरांक uint32_t v = toUInt();
        स्थिरांक uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        स्थिरांक uint32_t extended = v | signExtend;
        परत static_cast<int32_t>(extended);
    } अन्यथा {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        परत static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_संख्या::toUQuad() स्थिरांक VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *हे);
    // We allow wide numbers that represent values <= 64 bits
    जर (isDouble()) परत static_cast<uint64_t>(toDouble());
    साठी (पूर्णांक i = 2; i < शब्द(); ++i) {
        जर (m_data.अंक()[i].m_मूल्य) {
            v3error("Value too wide for 64-bits expected in this context " << *हे);
            थांबा;
        }
    }
    जर (width() <= 32) परत (static_cast<uint64_t>(toUInt()));
    परत ((static_cast<uint64_t>(m_data.अंक()[1].m_मूल्य) << 32ULL)
            | (static_cast<uint64_t>(m_data.अंक()[0].m_मूल्य)));
}

int64_t V3_संख्या::toSQuad() स्थिरांक {
    जर (isDouble()) परत static_cast<int64_t>(toDouble());
    स्थिरांक uint64_t v = toUQuad();
    स्थिरांक uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    स्थिरांक uint64_t extended = v | signExtend;
    परत static_cast<int64_t>(extended);
}

string V3_संख्या::toString() स्थिरांक {
    UASSERT(!isFourState(), "toString with 4-state " << *हे);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    जर (isString()) परत m_data.str();
    पूर्णांक bit = width() - 1;
    पर्यंत ((bit % 8) != 7) ++bit;
    std::string str;
    साठी (; bit >= 0; bit -= 8) {
        स्थिरांक पूर्णांक v = bits_मूल्य(bit - 7, 8);
        जर (v) str += static_cast<अक्षर>(v);
    }
    परत str;
}

uint32_t V3_संख्या::edataWord(पूर्णांक eword) स्थिरांक {
    UASSERT(!isFourState(), "edataWord with 4-state " << *हे);
    परत m_data.अंक()[eword].m_मूल्य;
}

uint8_t V3_संख्या::dataByte(पूर्णांक byte) स्थिरांक {
    परत (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

तार्किक V3_संख्या::isAllZ() स्थिरांक VL_MT_SAFE {
    जर (isDouble() || isString()) परत असत्य;
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        स्थिरांक uint32_t mask = i == शब्द() - 1 ? hiWordMask() : ~0U;
        जर (((~v.m_मूल्य & v.m_मूल्य_X) & mask) != mask) परत असत्य;
    }
    परत सत्य;
}
तार्किक V3_संख्या::isAllX() स्थिरांक VL_MT_SAFE {
    जर (isDouble() || isString()) परत असत्य;
    uint32_t mask = hiWordMask();
    साठी (पूर्णांक i = शब्द() - 1; i >= 0; --i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        जर ((v.m_मूल्य & v.m_मूल्य_X) ^ mask) परत असत्य;
        mask = ~0U;
    }
    परत सत्य;
}
तार्किक V3_संख्या::isEqZero() स्थिरांक VL_MT_SAFE {
    जर (isString()) परत m_data.str().empty();
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        जर (v.m_मूल्य || v.m_मूल्य_X) परत असत्य;
    }
    परत सत्य;
}
तार्किक V3_संख्या::isNeqZero() स्थिरांक {
    जर (isString()) परत !m_data.str().empty();
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        जर (v.m_मूल्य & ~v.m_मूल्य_X) परत सत्य;
    }
    परत असत्य;
}
तार्किक V3_संख्या::isBitsZero(पूर्णांक msb, पूर्णांक lsb) स्थिरांक {
    साठी (पूर्णांक i = lsb; i <= msb; ++i) {
        जर (VL_UNLIKELY(!bitIs0(i))) परत असत्य;
    }
    परत सत्य;
}
तार्किक V3_संख्या::isEqOne() स्थिरांक {
    जर (m_data.अंक()[0].m_मूल्य != 1 || m_data.अंक()[0].m_मूल्य_X) परत असत्य;
    साठी (पूर्णांक i = 1; i < शब्द(); ++i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        जर (v.m_मूल्य || v.m_मूल्य_X) परत असत्य;
    }
    परत सत्य;
}
तार्किक V3_संख्या::isEqAllOnes(पूर्णांक optwidth) स्थिरांक {
    // Correct number of zero bits/width matters
    जर (!optwidth) optwidth = width();
    साठी (पूर्णांक bit = 0; bit < optwidth; ++bit) {
        जर (!bitIs1(bit)) परत असत्य;
    }
    परत सत्य;
}
तार्किक V3_संख्या::isFourState() स्थिरांक VL_MT_SAFE {
    जर (isDouble() || isString()) परत असत्य;
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        जर (m_data.अंक()[i].m_मूल्य_X) परत सत्य;
    }
    परत असत्य;
}
तार्किक V3_संख्या::isAnyX() स्थिरांक VL_MT_SAFE {
    जर (isDouble() || isString()) परत असत्य;
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        जर (v.m_मूल्य & v.m_मूल्य_X) परत सत्य;
    }
    परत असत्य;
}
तार्किक V3_संख्या::isAnyXZ() स्थिरांक {
    जर (isDouble() || isString()) परत असत्य;
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        जर (m_data.अंक()[i].m_मूल्य_X) परत सत्य;
    }
    परत असत्य;
}
तार्किक V3_संख्या::isAnyZ() स्थिरांक VL_MT_SAFE {
    जर (isDouble() || isString()) परत असत्य;
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
        जर (~v.m_मूल्य & v.m_मूल्य_X) परत सत्य;
    }
    परत असत्य;
}
तार्किक V3_संख्या::isLtXZ(स्थिरांक V3_संख्या& rhs) स्थिरांक {
    // Include X/Z in comparisons for sort ordering
    जर (isString()) परत toString() < rhs.toString();
    जर (isDouble()) परत toDouble() < rhs.toDouble();
    साठी (पूर्णांक bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        जर (bitIs1(bit) && rhs.bitIs0(bit)) परत सत्य;
        जर (rhs.bitIs1(bit) && bitIs0(bit)) परत असत्य;
        जर (bitIsXZ(bit)) परत सत्य;
        जर (rhs.bitIsXZ(bit)) परत असत्य;
    }
    परत असत्य;
}
पूर्णांक V3_संख्या::मोजणी_X(पूर्णांक lsb, पूर्णांक nbits) स्थिरांक VL_MT_SAFE {
    पूर्णांक मोजणी = 0;
    साठी (पूर्णांक bitn = 0; bitn < nbits; ++bitn) {
        जर (lsb + bitn >= width()) परत मोजणी;
        जर (bitIsX(lsb + bitn)) ++मोजणी;
    }
    परत मोजणी;
}
पूर्णांक V3_संख्या::मोजणी_Z(पूर्णांक lsb, पूर्णांक nbits) स्थिरांक VL_MT_SAFE {
    पूर्णांक मोजणी = 0;
    साठी (पूर्णांक bitn = 0; bitn < nbits; ++bitn) {
        जर (lsb + bitn >= width()) परत मोजणी;
        जर (bitIsZ(lsb + bitn)) ++मोजणी;
    }
    परत मोजणी;
}

पूर्णांक V3_संख्या::widthToFit() स्थिरांक {
    साठी (पूर्णांक bit = width() - 1; bit > 0; bit--) {
        जर (!bitIs0(bit)) परत bit + 1;
    }
    परत 1;  // one bit even if number is == 0
}

uint32_t V3_संख्या::मोजणी_Bits(स्थिरांक V3_संख्या& ctrl) स्थिरांक {
    // Correct number of zero bits/width matters
    पूर्णांक n = 0;
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        निवड (ctrl.bitIs(0)) {
        स्थिती '0':
            जर (bitIs0(bit)) ++n;
            थांबा;
        स्थिती '1':
            जर (bitIs1(bit)) ++n;
            थांबा;
        स्थिती 'x':
            जर (bitIsX(bit)) ++n;
            थांबा;
        स्थिती 'z':
            जर (bitIsZ(bit)) ++n;
            थांबा;
        }
    }
    परत n;
}

uint32_t V3_संख्या::मोजणी_Bits(स्थिरांक V3_संख्या& ctrl1, स्थिरांक V3_संख्या& ctrl2,
                             स्थिरांक V3_संख्या& ctrl3) स्थिरांक {
    // Correct number of zero bits/width matters
    पूर्णांक n = मोजणी_Bits(ctrl1);
    जर (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += मोजणी_Bits(ctrl2);
    जर ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += मोजणी_Bits(ctrl3);
    }
    परत n;
}

uint32_t V3_संख्या::मोजणी_Ones() स्थिरांक {
    पूर्णांक n = 0;
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (bitIs1(bit)) ++n;
    }
    परत n;
}

uint32_t V3_संख्या::mostSetBitP1() स्थिरांक {
    साठी (पूर्णांक bit = width() - 1; bit >= 0; bit--) {
        जर (!bitIs0(bit)) परत bit + 1;
    }
    परत 0;
}

uint32_t V3_संख्या::leastSetBitP1() स्थिरांक {
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (!bitIs0(bit)) परत bit + 1;
    }
    परत 0;
}

//======================================================================

V3_संख्या& V3_संख्या::opBitsNonXZ(स्थिरांक V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opBitsOne(स्थिरांक V3_संख्या& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opBitsXZ(स्थिरांक V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opBitsZ(स्थिरांक V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    परत *हे;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_संख्या& V3_संख्या::opRedOr(स्थिरांक V3_संख्या& lhs) {
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अक्षर outc = 0;
    साठी (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        जर (lhs.bitIs1(bit)) {
            परत setSingleBits(1);
        } अन्यथा जर (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opRedAnd(स्थिरांक V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अक्षर outc = 1;
    साठी (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        जर (lhs.bitIs0(bit)) {
            परत setSingleBits(0);
        } अन्यथा जर (lhs.bitIs1(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opRedXor(स्थिरांक V3_संख्या& lhs) {
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अक्षर outc = 0;
    साठी (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        जर (lhs.bitIs1(bit)) {
            जर (outc == 1) {
                outc = 0;
            } अन्यथा जर (outc == 0) {
                outc = 1;
            }
        } अन्यथा जर (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::op_मोजणी_Bits(स्थिरांक V3_संख्या& अभिव्यक्ती, स्थिरांक V3_संख्या& ctrl1, स्थिरांक V3_संख्या& ctrl2,
                                स्थिरांक V3_संख्या& ctrl3) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS4(अभिव्यक्ती, ctrl1, ctrl2, ctrl3);
    अंक_ASSERT_LOGIC_ARGS4(अभिव्यक्ती, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.अंक()[0].m_मूल्य = अभिव्यक्ती.मोजणी_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::op_मोजणी_Ones(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    जर (lhs.isFourState()) परत setAllBitsX();
    setZero();
    m_data.अंक()[0].m_मूल्य = lhs.मोजणी_Ones();
    opCleanThis();
    परत *हे;
}
V3_संख्या& V3_संख्या::opIsUnknown(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    परत setSingleBits(lhs.isAnyXZ());
}
V3_संख्या& V3_संख्या::opOneHot(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    जर (lhs.isFourState()) परत setAllBitsX();
    परत setSingleBits(lhs.मोजणी_Ones() == 1);
}
V3_संख्या& V3_संख्या::opOneHot0(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    जर (lhs.isFourState()) परत setAllBitsX();
    परत setSingleBits(lhs.मोजणी_Ones() <= 1);
}
V3_संख्या& V3_संख्या::opCLog2(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    जर (lhs.isFourState()) परत setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    स्थिरांक पूर्णांक adjust = (lhs.मोजणी_Ones() == 1) ? 0 : 1;
    साठी (पूर्णांक bit = lhs.width() - 1; bit >= 0; bit--) {
        जर (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            परत *हे;
        }
    }
    setZero();
    परत *हे;
}
V3_संख्या& V3_संख्या::opMostSetBitP1(स्थिरांक V3_संख्या& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    जर (lhs.isFourState()) परत setAllBitsX();
    साठी (पूर्णांक bit = lhs.width() - 1; bit >= 0; bit--) {
        जर (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            परत *हे;
        }
    }
    setZero();
    परत *हे;
}

V3_संख्या& V3_संख्या::opLogNot(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    अक्षर outc = 1;
    साठी (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        जर (lhs.bitIs1(bit)) {
            outc = 0;
            जा last;
        } अन्यथा जर (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
last:
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opNot(स्थिरांक V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा जर (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opAnd(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा जर (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opOr(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा जर (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opXor(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा जर (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा जर (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    परत *हे;
}

शून्य V3_संख्या::fillBits(पूर्णांक destLsb, पूर्णांक width, अक्षर मूल्य) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    जर (!width) परत;
    UASSERT(width <= हे->width() - destLsb, "Destination bit range exceeds number width");

    स्थिरांक uint32_t मूल्य_Word = मूल्य == '1' || मूल्य == 'x' || मूल्य == 1 || मूल्य == 3 ? ~0U : 0;
    स्थिरांक uint32_t xWord = मूल्य == 'z' || मूल्य == 'x' || मूल्य == 2 || मूल्य == 3 ? ~0U : 0;
    स्थिरांक पूर्णांक firstWord = VL_BITWORD_E(destLsb);
    स्थिरांक पूर्णांक lastWord = VL_BITWORD_E(destLsb + width - 1);
    साठी (पूर्णांक word = firstWord; word <= lastWord; ++word) {
        स्थिरांक पूर्णांक wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        स्थिरांक पूर्णांक wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        स्थिरांक uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        मूल्य_And_X& dest = m_data.अंक()[word];
        dest.m_मूल्य = (dest.m_मूल्य & ~mask) | (मूल्य_Word & mask);
        dest.m_मूल्य_X = (dest.m_मूल्य_X & ~mask) | (xWord & mask);
    }
}

शून्य V3_संख्या::copyBits(पूर्णांक destLsb, स्थिरांक V3_संख्या& source, पूर्णांक sourceLsb, पूर्णांक width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    जर (!width) परत;
    UASSERT(width <= हे->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    स्थिरांक पूर्णांक firstWord = VL_BITWORD_E(destLsb);
    स्थिरांक पूर्णांक lastWord = VL_BITWORD_E(destLsb + width - 1);
    साठी (पूर्णांक destWord = firstWord; destWord <= lastWord; ++destWord) {
        स्थिरांक पूर्णांक destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        स्थिरांक पूर्णांक destOffset = VL_BITBIT_E(destBit);
        स्थिरांक पूर्णांक sourceBit = sourceLsb + destBit - destLsb;
        स्थिरांक पूर्णांक sourceWord = VL_BITWORD_E(sourceBit);
        स्थिरांक पूर्णांक sourceOffset = VL_BITBIT_E(sourceBit);
        स्थिरांक पूर्णांक copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        स्थिरांक uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        स्थिरांक मूल्य_And_X& sourceLo = source.m_data.अंक()[sourceWord];
        uint32_t मूल्य = sourceLo.m_मूल्य >> sourceOffset;
        uint32_t मूल्य_X = sourceLo.m_मूल्य_X >> sourceOffset;
        जर (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            स्थिरांक मूल्य_And_X& sourceHi = source.m_data.अंक()[sourceWord + 1];
            मूल्य |= sourceHi.m_मूल्य << (VL_EDATASIZE - sourceOffset);
            मूल्य_X |= sourceHi.m_मूल्य_X << (VL_EDATASIZE - sourceOffset);
        }

        मूल्य_And_X& dest = m_data.अंक()[destWord];
        dest.m_मूल्य = (dest.m_मूल्य & ~mask) | ((मूल्य << destOffset) & mask);
        dest.m_मूल्य_X = (dest.m_मूल्य_X & ~mask) | ((मूल्य_X << destOffset) & mask);
    }
}

V3_संख्या& V3_संख्या::opConcat(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    जर (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    परत *हे;
}

V3_संख्या& V3_संख्या::opLenN(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    परत *हे;
}

V3_संख्या& V3_संख्या::opRepl(स्थिरांक V3_संख्या& lhs,
                           स्थिरांक V3_संख्या& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    परत opRepl(lhs, rhs.toUInt());
}

V3_संख्या& V3_संख्या::opRepl(स्थिरांक V3_संख्या& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    जर (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    साठी (अचिन्हित बार = 0; बार < rhsval; ++बार) {
        स्थिरांक uint64_t destLsb = uint64_t{बार} * lhs.width();
        जर (destLsb >= static_cast<uint32_t>(width())) थांबा;
        copyBits(static_cast<पूर्णांक>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<पूर्णांक>(destLsb)));
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opStreamL(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    जर (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    स्थिरांक पूर्णांक ssize = std::min(rhs.toUInt(), static_cast<अचिन्हित>(lhs.width()));
    साठी (पूर्णांक istart = 0; istart < lhs.width(); istart += ssize) {
        स्थिरांक पूर्णांक ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opLogAnd(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर loutc = 0;
    अक्षर routc = 0;
    साठी (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        जर (lhs.bitIs1(bit)) {
            loutc = 1;
            थांबा;
        }
        जर (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    साठी (पूर्णांक bit = 0; bit < rhs.width(); ++bit) {
        जर (rhs.bitIs1(bit)) {
            routc = 1;
            थांबा;
        }
        जर (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    अक्षर outc = 'x';
    जर (routc == 1 && loutc == 1) outc = 1;
    जर (routc == 0 || loutc == 0) outc = 0;
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opLogOr(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    साठी (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        जर (lhs.bitIs1(bit)) {
            outc = 1;
            जा last;
        }
        जर (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    साठी (पूर्णांक bit = 0; bit < rhs.width(); ++bit) {
        जर (rhs.bitIs1(bit)) {
            outc = 1;
            जा last;
        }
        जर (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opLogIf(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या lnot = lhs;
    lnot.opLogNot(lhs);
    परत opLogOr(lnot, rhs);
}

V3_संख्या& V3_संख्या::opLogEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_संख्या ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    परत opLogAnd(ifa, ifb);
}

V3_संख्या& V3_संख्या::opAtoN(स्थिरांक V3_संख्या& lhs, पूर्णांक base) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::हटाओ(str.begin(), str.end(), '_'), str.end());

    जर (base == AstAtoN::ATOREAL) परत setDouble(std::म_से_भ(str.c_str()));

    त्रुटि_सं = 0;
    स्वतः v = std::म_से_दीर्घ(str.c_str(), nullptr, base);
    जर (त्रुटि_सं != 0) v = 0;
    परत setLongS(static_cast<int32_t>(v));
}

V3_संख्या& V3_संख्या::opPutcN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, स्थिरांक V3_संख्या& ths) {
    अंक_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अंक_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    स्थिरांक int32_t i = rhs.toSInt();
    स्थिरांक int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    जर (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    परत setString(lstring);
}

V3_संख्या& V3_संख्या::opGetcN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिरांक string lstring = lhs.toString();
    स्थिरांक int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    जर (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    परत setLong(v);
}

V3_संख्या& V3_संख्या::opSubstrN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, स्थिरांक V3_संख्या& ths) {
    अंक_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिरांक string lstring = lhs.toString();
    स्थिरांक int32_t i = rhs.toSInt();
    स्थिरांक int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    जर (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) परत setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    परत setString(lstring.substr(i, j - i + 1));
}

V3_संख्या& V3_संख्या::opCompareNN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, तार्किक ignoreCase) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    पूर्णांक result;
    स्थिरांक string lstring = lhs.toString();
    स्थिरांक string rstring = rhs.toString();
    जर (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } अन्यथा {
        result = std::म_भेद(lstring.c_str(), rstring.c_str());
    }
    परत setLongS(result);
}

V3_संख्या& V3_संख्या::opEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    जर (lhs.isString()) परत opEqN(lhs, rhs);
    जर (lhs.isDouble()) परत opEqD(lhs, rhs);
    अक्षर outc = 1;
    साठी (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        जर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            जा last;
        }
        जर (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            जा last;
        }
        जर (lhs.bitIsXZ(bit)) outc = 'x';
        जर (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opNeq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    जर (lhs.isString()) परत opNeqN(lhs, rhs);
    जर (lhs.isDouble()) परत opNeqD(lhs, rhs);
    अक्षर outc = 0;
    साठी (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        जर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            जा last;
        }
        जर (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            जा last;
        }
        जर (lhs.bitIsXZ(bit)) outc = 'x';
        जर (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    परत setSingleBits(outc);
}

V3Hash V3_संख्या::toHash() स्थिरांक {
    // Shouldmatch isCaseEq
    जर (isString()) परत V3Hash{toString()};
    जर (isDouble()) परत V3Hash{toDouble()};
    V3Hash hash{width()};
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) hash += m_data.अंक()[i].m_मूल्य;
    परत hash;
}

तार्किक V3_संख्या::isCaseEq(स्थिरांक V3_संख्या& rhs) स्थिरांक {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    जर (isString()) परत toString() == rhs.toString();
    जर (isDouble()) परत toDouble() == rhs.toDouble();
    जर (width() != rhs.width()) परत असत्य;
    साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
        जर (!(m_data.अंक()[i] == rhs.m_data.अंक()[i])) परत असत्य;
    }
    परत सत्य;
}

V3_संख्या& V3_संख्या::opCaseEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_संख्या& V3_संख्या::opCaseNeq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    जर (lhs.isString()) {
        परत opNeqN(lhs, rhs);
    } अन्यथा जर (lhs.isDouble()) {
        परत opNeqD(lhs, rhs);
    }
    साठी (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        जर (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            जा last;
        }
    }
last:
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opWildEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 1;
    साठी (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        जर (!rhs.bitIsXZ(bit)) {
            जर (lhs.bitIsXZ(bit)) {
                outc = 'x';
                जा last;
            }
            जर (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                जा last;
            }
        }
    }
last:
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opWildNeq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    साठी (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        जर (!rhs.bitIsXZ(bit)) {
            जर (lhs.bitIsXZ(bit)) {
                outc = 'x';
                जा last;
            }
            जर (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                जा last;
            }
        }
    }
last:
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGt(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    साठी (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        जर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        जर (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        जर (lhs.bitIsXZ(bit)) outc = 'x';
        जर (rhs.bitIsXZ(bit)) outc = 'x';
    }
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGtS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    {
        स्थिरांक पूर्णांक mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        जर (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } अन्यथा जर (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } अन्यथा जर (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } अन्यथा जर (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } अन्यथा {
            // both positive or negative, normal >
            साठी (पूर्णांक bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                जर (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                जर (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                जर (lhs.bitIsXZ(bit)) outc = 'x';
                जर (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    परत setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGte(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या& eq = opEq(lhs, rhs);
    जर (eq.isNeqZero()) परत eq;  // Return true
    परत opGt(lhs, rhs);
}

V3_संख्या& V3_संख्या::opGteS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या& eq = opEq(lhs, rhs);
    जर (eq.isNeqZero()) परत eq;  // Return true
    परत opGtS(lhs, rhs);
}

V3_संख्या& V3_संख्या::opLt(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) { परत opGt(rhs, lhs); }
V3_संख्या& V3_संख्या::opLte(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) { परत opGte(rhs, lhs); }
V3_संख्या& V3_संख्या::opLtS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) { परत opGtS(rhs, lhs); }
V3_संख्या& V3_संख्या::opLteS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) { परत opGteS(rhs, lhs); }

V3_संख्या& V3_संख्या::opShiftR(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // L(lhs) bit return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (rhs.isFourState()) परत setAllBitsX();
    setZero();
    जर (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) परत *हे;
    स्थिरांक uint32_t rhsval = rhs.toUInt();
    जर (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<पूर्णांक>(rhsval),
                 std::min(width(), lhs.width() - static_cast<पूर्णांक>(rhsval)));
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opShiftRS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (rhs.isFourState()) परत setAllBitsX();
    setZero();
    स्थिरांक तार्किक overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    जर (!overflow) {
        स्थिरांक uint32_t rhsval = rhs.toUInt();
        जर (rhsval < lbits) {
            स्थिरांक पूर्णांक copyWidth = std::min(width(), static_cast<पूर्णांक>(lbits - rhsval));
            copyBits(0, lhs, static_cast<पूर्णांक>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            परत *हे;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    परत *हे;
}

V3_संख्या& V3_संख्या::opShiftL(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // L(lhs) bit return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (rhs.isFourState()) परत setAllBitsX();
    setZero();
    जर (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) परत *हे;
    स्थिरांक uint32_t rhsval = rhs.toUInt();
    जर (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<पूर्णांक>(rhsval), lhs, 0,
                 std::min(width() - static_cast<पूर्णांक>(rhsval), lhs.width()));
    }
    परत *हे;
}

//======================================================================
// Ops - Arithmetic

V3_संख्या& V3_संख्या::opNegate(स्थिरांक V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    जर (lhs.isFourState()) परत setAllBitsX();
    V3_संख्या notlhs(&lhs, width());
    notlhs.opNot(lhs);
    स्थिरांक V3_संख्या one(&lhs, width(), 1);
    opAdd(notlhs, one);
    परत *हे;
}
V3_संख्या& V3_संख्या::opAdd(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    साठी (पूर्णांक word = 0; word < शब्द(); ++word) {
        स्थिरांक uint64_t lwordval = lhs.m_data.अंक()[word].m_मूल्य;
        स्थिरांक uint64_t rwordval = rhs.m_data.अंक()[word].m_मूल्य;
        स्थिरांक uint64_t sum = lwordval + rwordval + carry;
        m_data.अंक()[word].m_मूल्य = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    परत *हे;
}
V3_संख्या& V3_संख्या::opSub(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    V3_संख्या negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    परत opAdd(lhs, negrhs);
}
V3_संख्या& V3_संख्या::opMul(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    setZero();
    जर (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } अन्यथा {
        साठी (पूर्णांक lword = 0; lword < lhs.शब्द(); ++lword) {
            स्थिरांक uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.अंक()[lword].m_मूल्य);
            जर (lwordval == 0) सुरूठेवा;
            साठी (पूर्णांक rword = 0; rword < rhs.शब्द(); ++rword) {
                स्थिरांक uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.अंक()[rword].m_मूल्य);
                जर (rwordval == 0) सुरूठेवा;
                uint64_t mul = lwordval * rwordval;
                साठी (पूर्णांक qword = lword + rword; qword < शब्द(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.अंक()[qword].m_मूल्य);
                    m_data.अंक()[qword].m_मूल्य = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    जर (mul == 0) थांबा;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opMulS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    V3_संख्या lhsNoSign = lhs;
    जर (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    जर (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    स्थिरांक V3_संख्या qNoSign = opMul(lhsNoSign, rhsNoSign);
    जर ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opDiv(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    जर (rhs.isEqZero()) परत setAllBitsXRemoved();
    जर (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        परत *हे;
    } अन्यथा {
        // Wide division
        परत opModDivGuts(lhs, rhs, असत्य);
    }
}
V3_संख्या& V3_संख्या::opDivS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    जर (rhs.isEqZero()) परत setAllBitsXRemoved();
    V3_संख्या lhsNoSign = lhs;
    जर (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    जर (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    स्थिरांक V3_संख्या qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    जर ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *हे);
    परत *हे;
}
V3_संख्या& V3_संख्या::opModDiv(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    जर (rhs.isEqZero()) परत setAllBitsXRemoved();
    जर (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        परत *हे;
    } अन्यथा {
        // Wide modulus
        परत opModDivGuts(lhs, rhs, सत्य);
    }
}
V3_संख्या& V3_संख्या::opModDivS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    जर (rhs.isEqZero()) परत setAllBitsXRemoved();
    V3_संख्या lhsNoSign = lhs;
    जर (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    जर (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    स्थिरांक V3_संख्या qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    जर (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opModDivGuts(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, तार्किक is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    स्थिरांक पूर्णांक l_शब्द = lhs.शब्द();
    स्थिरांक पूर्णांक umsbp1 = lhs.mostSetBitP1();  // dividend
    स्थिरांक पूर्णांक vmsbp1 = rhs.mostSetBitP1();  //भाजक
    जर (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *हे);
        परत *हे;
    }

    स्थिरांक पूर्णांक uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    स्थिरांक पूर्णांक vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    जर (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        साठी (पूर्णांक j = uw - 1; j >= 0; j--) {
            स्थिरांक uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.अंक()[j].m_मूल्य));
            m_data.अंक()[j].m_मूल्य = unw64 / static_cast<uint64_t>(rhs.m_data.अंक()[0].m_मूल्य);
            k = unw64
                - (static_cast<uint64_t>(m_data.अंक()[j].m_मूल्य)
                   * static_cast<uint64_t>(rhs.m_data.अंक()[0].m_मूल्य));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *हे << " rem=0x" << std::hex
                                   << k << std::dec);
        जर (is_modulus) {
            setZero();
            m_data.अंक()[0].m_मूल्य = k;
        }
        opCleanThis();
        परत *हे;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_शब्द + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_शब्द + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    साठी (पूर्णांक i = 0; i < l_शब्द; ++i) m_data.अंक()[i].m_मूल्य = 0;
    साठी (पूर्णांक i = 0; i < l_शब्द + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    स्थिरांक पूर्णांक s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    स्थिरांक uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    साठी (पूर्णांक i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.अंक()[i].m_मूल्य << s)
                | (shift_mask & (rhs.m_data.अंक()[i - 1].m_मूल्य >> (32 - s)));
    }
    vn[0] = rhs.m_data.अंक()[0].m_मूल्य << s;

    // Copy and shift dividend by same amount; may set new upper word
    जर (s) {
        un[uw] = lhs.m_data.अंक()[uw - 1].m_मूल्य >> (32 - s);
    } अन्यथा {
        un[uw] = 0;
    }
    साठी (पूर्णांक i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.अंक()[i].m_मूल्य << s)
                | (shift_mask & (lhs.m_data.अंक()[i - 1].m_मूल्य >> (32 - s)));
    }
    un[0] = lhs.m_data.अंक()[0].m_मूल्य << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    साठी (पूर्णांक j = uw - vw; j >= 0; j--) {
        // Estimate
        स्थिरांक uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        जर (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            जर (rhat < 0x100000000ULL) जा again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        साठी (पूर्णांक i = 0; i < vw; ++i) {
            स्थिरांक uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.अंक()[j].m_मूल्य = qhat;  // Save quotient digit

        जर (t < 0) {
            // Over subtracted; correct by adding back
            m_data.अंक()[j].m_मूल्य--;
            k = 0;
            साठी (पूर्णांक i = 0; i < vw; ++i) {
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

    जर (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        साठी (पूर्णांक i = 0; i < vw; ++i) {
            m_data.अंक()[i].m_मूल्य = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        साठी (पूर्णांक i = vw; i < l_शब्द; ++i) m_data.अंक()[i].m_मूल्य = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *हे);
        परत *हे;
    } अन्यथा {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *हे);
        परत *हे;
    }
}

V3_संख्या& V3_संख्या::opPow(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, तार्किक lsign, तार्किक rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    जर (lhs.isFourState() || rhs.isFourState()) परत setAllBitsX();
    जर (rhs.isEqZero()) परत setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    जर (rsign && rhs.isNegative()) {
        जर (lhs.isEqZero()) {
            परत setAllBitsXRemoved();
        } अन्यथा जर (lhs.isEqOne()) {
            परत setValue1();
        } अन्यथा जर (lsign && lhs.isEqAllOnes()) {
            जर (rhs.bitIs1(0)) {
                परत setAllBits1();  // -1^odd=-1
            } अन्यथा {
                परत setValue1();  // -1^even=1
            }
        }
        परत setZero();
    }
    जर (lhs.isEqZero()) परत setZero();
    setZero();
    m_data.अंक()[0].m_मूल्य = 1;
    V3_संख्या power(&lhs, width());
    power.opAssign(lhs);
    साठी (पूर्णांक bit = 0; bit < rhs.width(); ++bit) {
        जर (bit > 0) {  // power = power*power
            V3_संख्या lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        जर (rhs.bitIs1(bit)) {  // out *= power
            V3_संख्या lastOut(&lhs, width());
            lastOut.opAssign(*हे);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opPowSU(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    परत opPow(lhs, rhs, सत्य, असत्य);
}
V3_संख्या& V3_संख्या::opPowSS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    परत opPow(lhs, rhs, सत्य, सत्य);
}
V3_संख्या& V3_संख्या::opPowUS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    परत opPow(lhs, rhs, असत्य, सत्य);
}

V3_संख्या& V3_संख्या::opBufIf1(स्थिरांक V3_संख्या& ens, स्थिरांक V3_संख्या& if1s) {
    अंक_ASSERT_OP_ARGS2(ens, if1s);
    अंक_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    साठी (पूर्णांक bit = 0; bit < width(); ++bit) {
        जर (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } अन्यथा {
            setBit(bit, 'z');
        }
    }
    परत *हे;
}

// Sets all bits in range to the given value
V3_संख्या& V3_संख्या::opSetRange(uint32_t lsb, uint32_t width, अक्षर bit_मूल्य) {
    जर (lsb < static_cast<uint32_t>(हे->width())) {
        fillBits(static_cast<पूर्णांक>(lsb),
                 static_cast<पूर्णांक>(std::min(width, static_cast<uint32_t>(हे->width()) - lsb)),
                 bit_मूल्य);
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opAssign(स्थिरांक V3_संख्या& lhs) { परत opAssignNonXZ(lhs, असत्य); }
V3_संख्या& V3_संख्या::opAssignNonXZ(स्थिरांक V3_संख्या& lhs, तार्किक ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    जर (हे != &lhs) {
        जर (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = सत्य;
        } अन्यथा जर (isString()) {
            जर (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } अन्यथा {
                m_data.str() = lhs.m_data.str();
            }
        } अन्यथा जर (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } अन्यथा जर (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } अन्यथा जर (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } अन्यथा {
            साठी (पूर्णांक word = 0; word < शब्द(); ++word) {
                स्थिरांक uint32_t मूल्य = word < lhs.शब्द() ? lhs.m_data.अंक()[word].m_मूल्य
                                                                & ~lhs.m_data.अंक()[word].m_मूल्य_X
                                                          : 0;
                m_data.अंक()[word] = {मूल्य, 0};
            }
            opCleanThis();
        }
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opNToI(स्थिरांक V3_संख्या& lhs) {
    // String to packed number
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    setZero();
    स्थिरांक string& str = lhs.toString();
    साठी (माप_प्रकार n = 0; n < str.length(); ++n) {
        स्थिरांक अक्षर c = str[str.length() - 1 - n];
        साठी (माप_प्रकार cbit = 0; cbit < 8; ++cbit) {
            स्थिरांक std::bitset<8> sbit{static_cast<अचिन्हित दीर्घ दीर्घ>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    परत *हे;
}

V3_संख्या& V3_संख्या::opExtendS(स्थिरांक V3_संख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    स्थिरांक पूर्णांक copyWidth = std::min(width(), static_cast<पूर्णांक>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    परत *हे;
}

V3_संख्या& V3_संख्या::opExtendXZ(स्थिरांक V3_संख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    स्थिरांक पूर्णांक copyWidth = std::min(width(), static_cast<पूर्णांक>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    परत *हे;
}

V3_संख्या& V3_संख्या::opClean(स्थिरांक V3_संख्या& lhs, uint32_t bits) { परत opSel(lhs, bits - 1, 0); }

शून्य V3_संख्या::opCleanThis(तार्किक warnOnTruncation) {
    // Clean MSB of number
    अंक_ASSERT_LOGIC_ARGS1(*हे);
    स्थिरांक मूल्य_And_X v = m_data.अंक()[शब्द() - 1];
    स्थिरांक uint32_t new_मूल्य_Msb = v.m_मूल्य & hiWordMask();
    स्थिरांक uint32_t new_मूल्य_XMsb = v.m_मूल्य_X & hiWordMask();
    जर (warnOnTruncation && (new_मूल्य_Msb != v.m_मूल्य || new_मूल्य_XMsb != v.m_मूल्य_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.अंक()[शब्द() - 1] = {new_मूल्य_Msb, new_मूल्य_XMsb};
}

V3_संख्या& V3_संख्या::opSel(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& msb, स्थिरांक V3_संख्या& lsb) {
    अंक_ASSERT_OP_ARGS3(lhs, msb, lsb);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS2(msb, lsb);
    जर (lsb.isFourState() || msb.isFourState()) परत setAllBitsX();
    परत opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_संख्या& V3_संख्या::opSel(स्थिरांक V3_संख्या& lhs, uint32_t msbval, uint32_t lsbval) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    पूर्णांक copyWidth = 0;
    जर (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        स्थिरांक uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<पूर्णांक>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<पूर्णांक>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    परत *हे;
}

V3_संख्या& V3_संख्या::opSelInto(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& lsb, पूर्णांक width) {
    परत opSelInto(lhs, lsb.toSInt(), width);
}

V3_संख्या& V3_संख्या::opSelInto(स्थिरांक V3_संख्या& lhs, पूर्णांक lsbval, पूर्णांक width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    स्थिरांक पूर्णांक copyWidth = std::max(0, std::min({width, lhs.width(), हे->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    स्थिरांक पूर्णांक fillWidth
        = std::max(0, std::min(width - copyWidth, हे->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    परत *हे;
}

//======================================================================
// Ops - Floating point

V3_संख्या& V3_संख्या::opIToRD(स्थिरांक V3_संख्या& lhs, तार्किक isSigned) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_संख्या noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    द्विगुण d = 0;
    स्थिरांक तार्किक negate = isSigned && noxz.isNegative();
    जर (negate) {
        स्थिरांक V3_संख्या noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    साठी (पूर्णांक bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        जर (noxz.bitIs1(bit)) d += exp2(bit);
    }
    जर (negate) d = -d;
    परत setDouble(d);
}
V3_संख्या& V3_संख्या::opRToIS(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    स्थिरांक द्विगुण v = VL_TRUNC(lhs.toDouble());
    स्थिरांक int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    परत setLongS(i);
}
V3_संख्या& V3_संख्या::opRToIRoundS(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    स्थिरांक द्विगुण v = VL_ROUND(lhs.toDouble());
    setZero();
    संघ {
        द्विगुण d;
        uint64_t q;
    } u;
    u.d = v;
    (शून्य)u.d;

    स्थिरांक पूर्णांक exp = static_cast<पूर्णांक>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    स्थिरांक पूर्णांक lsb = exp - 52;
    स्थिरांक uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    जर (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        साठी (पूर्णांक bit = 0; bit <= 52; ++bit) {
            जर (mantissa & (1ULL << bit)) {
                स्थिरांक पूर्णांक outbit = bit + lsb;
                जर (outbit >= 0) setBit(outbit, 1);
            }
        }
        जर (v < 0) {
            स्थिरांक V3_संख्या noSign = *हे;
            opNegate(noSign);
        }
    }
    परत *हे;
}
V3_संख्या& V3_संख्या::opRealToBits(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    संघ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    परत setQuad(u.m_v);
}
V3_संख्या& V3_संख्या::opBitsToRealD(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    संघ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    परत setDouble(u.m_d);
}
V3_संख्या& V3_संख्या::opNegateD(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    परत setDouble(-lhs.toDouble());
}
V3_संख्या& V3_संख्या::opAddD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_संख्या& V3_संख्या::opSubD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_संख्या& V3_संख्या::opMulD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_संख्या& V3_संख्या::opDivD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_संख्या& V3_संख्या::opPowD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setDouble(घात(lhs.toDouble(), rhs.toDouble()));
}
V3_संख्या& V3_संख्या::opEqD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_संख्या& V3_संख्या::opNeqD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_संख्या& V3_संख्या::opGtD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_संख्या& V3_संख्या::opGteD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_संख्या& V3_संख्या::opLtD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_संख्या& V3_संख्या::opLteD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_संख्या& V3_संख्या::opConcatN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setString(lhs.toString() + rhs.toString());
}
V3_संख्या& V3_संख्या::opReplN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_STRING_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(rhs);
    परत opReplN(lhs, rhs.toUInt());
}
V3_संख्या& V3_संख्या::opReplN(स्थिरांक V3_संख्या& lhs, uint32_t rhsval) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    साठी (अचिन्हित बार = 0; बार < rhsval; ++बार) out += lhs.toString();
    परत setString(out);
}
V3_संख्या& V3_संख्या::opCvtPackString(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    परत setString(lhs.toString());
}
V3_संख्या& V3_संख्या::opToLowerN(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिरांक std::string out = VString::downcase(lhs.toString());
    परत setString(out);
}
V3_संख्या& V3_संख्या::opToUpperN(स्थिरांक V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिरांक std::string out = VString::upcase(lhs.toString());
    परत setString(out);
}

V3_संख्या& V3_संख्या::opEqN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toString() == rhs.toString());
}
V3_संख्या& V3_संख्या::opNeqN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toString() != rhs.toString());
}
V3_संख्या& V3_संख्या::opGtN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toString() > rhs.toString());
}
V3_संख्या& V3_संख्या::opGteN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toString() >= rhs.toString());
}
V3_संख्या& V3_संख्या::opLtN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toString() < rhs.toString());
}
V3_संख्या& V3_संख्या::opLteN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    परत setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

शून्य V3_संख्या::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_ओळ* स्थिरांक fl = नवीन File_ओळ{File_ओळ::builtInFilename()};
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
