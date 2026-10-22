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

#समावेश "V3PchAstMT.h"

#समावेश "V3Number.h"

#समावेश "V3File.h"

#समावेश <algorithm>
#समावेश <cerrno>
#समावेश <cmath>
#समावेश <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr पूर्णांक MAX_SPRINTF_DOUBLE_आकार
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#घोषणा अंक_ASSERT_OP_ARGS1(arg1) \
    UASSERT((यह != &(arg1)), "Number operation called with same source and dest")
#घोषणा अंक_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((यह != &(arg1) && यह != &(arg2)), \
            "Number operation called with same source and dest")
#घोषणा अंक_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((यह != &(arg1) && यह != &(arg2) && यह != &(arg3)), \
            "Number operation called with same source and dest")
#घोषणा अंक_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((यह != &(arg1) && यह != &(arg2) && यह != &(arg3) && यह != &(arg4)), \
            "Number operation called with same source and dest")

#घोषणा अंक_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).तथ्य_Type() == V3_संख्या_तथ्य::V3_संख्या_तथ्य_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#घोषणा अंक_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    अंक_ASSERT_LOGIC_ARGS1(arg1); \
    अंक_ASSERT_LOGIC_ARGS1(arg2)

#घोषणा अंक_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    अंक_ASSERT_LOGIC_ARGS1(arg1); \
    अंक_ASSERT_LOGIC_ARGS1(arg2); \
    अंक_ASSERT_LOGIC_ARGS1(arg3); \
    अंक_ASSERT_LOGIC_ARGS1(arg4)

#घोषणा अंक_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#घोषणा अंक_ASSERT_STRING_ARGS2(arg1, arg2) \
    अंक_ASSERT_STRING_ARGS1(arg1); \
    अंक_ASSERT_STRING_ARGS1(arg2)

#घोषणा अंक_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#घोषणा अंक_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    अंक_ASSERT_DOUBLE_ARGS1(arg1); \
    अंक_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

व्योम V3_संख्या::v3errorEnd(स्थिर std::ostringstream& str) स्थिर VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    अगर (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } अन्यथा अगर (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } अन्यथा {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

व्योम V3_संख्या::v3errorEndFatal(स्थिर std::ostringstream& str) स्थिर
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    निश्चित(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_संख्या::V3_संख्या(Ast_गाँठ* nodep, VerilogStringLiteral, स्थिर string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    अगर (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } अन्यथा {
        init(nodep, std::max<पूर्णांक>(str.लंबाई() * 8, 1));
        क्रम (अचिन्हित pos = 0; pos < str.लंबाई(); ++pos) {
            स्थिर पूर्णांक topos = str.लंबाई() - 1 - pos;
            मूल्य_And_X& v = m_तथ्य.अंक()[topos / 4];
            क्रम (पूर्णांक bit = 0; bit < 8; ++bit) {
                अगर (str[pos] & (1UL << bit)) v.m_मूल्य |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_तथ्य.m_fromString = true;
    opCleanThis(true);
}

V3_संख्या::V3_संख्या(Ast_गाँठ* nodep, स्थिर Ast_गाँठ_DType* nodedtypep) {
    अगर (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } अन्यथा अगर (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } अन्यथा अगर (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } अन्यथा {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

व्योम V3_संख्या::create(स्थिर अक्षर* sourcep) {
    m_तथ्य.setLogic();
    स्थिर अक्षर* मूल्य_startp = sourcep;
    क्रम (स्थिर अक्षर* cp = sourcep; *cp; ++cp) {
        अगर (*cp == '\'') {
            मूल्य_startp = cp + 1;
            अवरोध;
        }
    }

    bool userSized = false;
    bool unbased = false;
    अक्षर base = '\0';
    अगर (मूल्य_startp != sourcep) {  // Has a '
        string widthn;
        स्थिर अक्षर* cp = sourcep;
        क्रम (; *cp; ++cp) {
            अगर (*cp == '\'') {
                ++cp;
                अवरोध;
            }
            अगर (*cp != '_') widthn += *cp;
        }
        जबतक (*cp == '_') cp++;
        अगर (*cp && std::छोटे(*cp) == 's') {
            ++cp;
            isSigned(true);
        }
        अगर (*cp) {
            base = *cp;
            ++cp;
        }
        मूल्य_startp = cp;

        userSized = widthn.लंबाई() != 0;
        अगर (std::म_से_प(widthn.c_str())) {
            अगर (std::म_से_प(widthn.c_str()) < 0
                || std::म_से_प(widthn.c_str()) > v3Global.opt.max_अंक_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_अंक_Width(), true);
            } अन्यथा {
                width(std::म_से_प(widthn.c_str()), true);
            }
        }
    } अन्यथा {
        unbased = true;
        base = 'd';
    }

    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    अगर (base == '0') {
        width(1, false);  // So we extend it
        setBit(0, 0);
        m_तथ्य.m_autoExtend = true;
    } अन्यथा अगर (base == '1') {
        width(1, false);  // So we extend it
        setBit(0, 1);
        m_तथ्य.m_autoExtend = true;
    } अन्यथा अगर (std::छोटे(base) == 'z') {
        width(1, false);  // So we extend it
        setBit(0, 'z');
        m_तथ्य.m_autoExtend = true;
    } अन्यथा अगर (std::छोटे(base) == 'x') {
        width(1, false);  // So we extend it
        setBit(0, 'x');
        m_तथ्य.m_autoExtend = true;
    }
    // Otherwise...
    अन्यथा अगर (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        स्थिर पूर्णांक widthBound = std::max<पूर्णांक>(32, std::म_माप(मूल्य_startp) * 4);
        width(widthBound, false);  // Will change width below
        अगर (unbased) isSigned(true);  // Also says the spec.
    }

    // Ignore leading blanks
    जबतक (*मूल्य_startp == '_' || std::है_खाली(*मूल्य_startp)) ++मूल्य_startp;
    अगर (!*मूल्य_startp && !m_तथ्य.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    अगर (userSized && m_तथ्य.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    पूर्णांक obit = 0;  // Start at LSB
    पूर्णांक base_align = 1;
    अगर (std::छोटे(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        जबतक (*मूल्य_startp == '_' || *मूल्य_startp == '0') ++मूल्य_startp;
        // Convert decimal number to hex
        पूर्णांक olen = 0;
        uint32_t val = 0;
        पूर्णांक got_x = 0;
        पूर्णांक got_z = 0;
        पूर्णांक got_01 = 0;
        bool warnedTooMany = false;
        क्रम (स्थिर अक्षर* cp = मूल्य_startp; *cp; ++cp) {
            चयन (std::छोटे(*cp)) {
            हाल '0':  // FALLTHRU
            हाल '1':  // FALLTHRU
            हाल '2':  // FALLTHRU
            हाल '3':  // FALLTHRU
            हाल '4':  // FALLTHRU
            हाल '5':  // FALLTHRU
            हाल '6':  // FALLTHRU
            हाल '7':  // FALLTHRU
            हाल '8':  // FALLTHRU
            हाल '9': {
                अगर (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_तथ्य.अंक()[0].m_मूल्य = val;
                } अन्यथा {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_संख्या product(यह, width() + 4);  // +4 for overflow detection
                    स्थिर V3_संख्या ten(यह, width() + 4, 10);
                    स्थिर V3_संख्या addend(यह, width(), (*cp - '0'));
                    product.opMul(*यह, ten);
                    opAdd(product, addend);
                    अगर (!warnedTooMany && product.bits_मूल्य(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = true;
                    }
                }
                ++olen;
                got_01 = 1;
                अवरोध;
            }
            हाल 'z':
            हाल '?': {
                got_z = 1;
                अगर (!userSized) width(32, false);
                setAllBitsZ();
                अवरोध;
            }
            हाल 'x': {
                got_x = 1;
                अगर (!userSized) width(32, false);
                setAllBitsX();
                अवरोध;
            }
            हाल '_': अवरोध;
            शेष: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                अवरोध;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        अगर ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << मूल्य_startp);
        }
    } अन्यथा {
        // Convert bin/octal number to hex
        क्रम (स्थिर अक्षर* cp = मूल्य_startp + std::म_माप(मूल्य_startp) - 1; cp >= मूल्य_startp;
             cp--) {
            अगर (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                अवरोध;
            }
            चयन (std::छोटे(base)) {
            हाल 'b': {
                base_align = 1;
                चयन (std::छोटे(*cp)) {
                हाल '0': setBit(obit++, 0); अवरोध;
                हाल '1': setBit(obit++, 1); अवरोध;
                हाल 'z':
                हाल '?': setBit(obit++, 'z'); अवरोध;
                हाल 'x': setBit(obit++, 'x'); अवरोध;
                हाल '_': अवरोध;
                शेष: v3error("Illegal character in binary constant: " << *cp);
                }
                अवरोध;
            }

            हाल 'o':
            हाल 'c': {
                base_align = 3;
                चयन (std::छोटे(*cp)) {  // clang-format off
                हाल '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  अवरोध;
                हाल '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  अवरोध;
                हाल '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  अवरोध;
                हाल '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  अवरोध;
                हाल '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  अवरोध;
                हाल '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  अवरोध;
                हाल '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  अवरोध;
                हाल '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  अवरोध;
                हाल 'z':
                हाल '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); अवरोध;
                हाल 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); अवरोध;
                हाल '_': अवरोध;
                // clang-format on
                शेष: v3error("Illegal character in octal constant");
                }
                अवरोध;
            }

            हाल 'h':  // FALLTHRU
            हाल 'x': {
                base_align = 4;
                चयन (std::छोटे(*cp)) {  // clang-format off
                हाल '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); अवरोध;
                हाल '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); अवरोध;
                हाल '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); अवरोध;
                हाल '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); अवरोध;
                हाल '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); अवरोध;
                हाल '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); अवरोध;
                हाल '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); अवरोध;
                हाल '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); अवरोध;
                हाल '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); अवरोध;
                हाल '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); अवरोध;
                हाल 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); अवरोध;
                हाल 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); अवरोध;
                हाल 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); अवरोध;
                हाल 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); अवरोध;
                हाल 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); अवरोध;
                हाल 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); अवरोध;
                हाल 'z':
                हाल '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); अवरोध;
                हाल 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); अवरोध;
                    // clang-format on
                हाल '_': अवरोध;
                शेष:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    अवरोध;
                }  // LCOV_EXCL_STOP
                अवरोध;
            }
            शेष:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                अवरोध;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    अगर (!userSized && !m_तथ्य.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), false);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    जबतक (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(true);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

व्योम V3_संख्या::warnTooMany(स्थिर string& मूल्य) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << मूल्य << "'");
}

व्योम V3_संख्या::nodep(Ast_गाँठ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    अगर (!nodep) वापस;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

bool V3_संख्या::epsilonEqual(द्विगुन a, द्विगुन b) {
    वापस std::भ_असल(a - b)
           <= (std::numeric_limits<द्विगुन>::epsilon() * std::max(1.0, std::max(a, b)));
}

bool V3_संख्या::epsilonIntegral(द्विगुन a) {
    स्थिर द्विगुन dint = static_cast<द्विगुन>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    वापस epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

पूर्णांक V3_संख्या::log2b(uint32_t अंक) {
    // See also opCLog2
    क्रम (पूर्णांक bit = 31; bit > 0; bit--) {
        अगर (अंक & (1ULL << bit)) वापस bit;
    }
    वापस 0;
}

पूर्णांक V3_संख्या::log2bQuad(uint64_t अंक) {
    अगर (अंक >> 32ULL) वापस 32 + log2b(अंक >> 32ULL);
    वापस log2b(अंक);
}

//======================================================================
// Setters

V3_संख्या& V3_संख्या::setZero() {
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, 0};
    वापस *यह;
}
V3_संख्या& V3_संख्या::setQuad(uint64_t मूल्य) {
    setZero();
    m_तथ्य.अंक()[0].m_मूल्य = मूल्य & 0xffffffffULL;
    अगर (width() > 32) m_तथ्य.अंक()[1].m_मूल्य = (मूल्य >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setLong(uint32_t मूल्य) {
    setZero();
    m_तथ्य.अंक()[0].m_मूल्य = मूल्य;
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setLongS(int32_t मूल्य) {
    setZero();
    जोड़ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = मूल्य;
    (व्योम)u.s;
    m_तथ्य.अंक()[0].m_मूल्य = u.u;
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setDouble(द्विगुन मूल्य) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_तथ्य.setDouble();
    जोड़ {
        द्विगुन d;
        uint32_t u[2];
    } u;
    u.d = मूल्य;
    (व्योम)u.d;
    क्रम (पूर्णांक i = 2; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, 0};
    m_तथ्य.अंक()[0].m_मूल्य = u.u[0];
    m_तथ्य.अंक()[1].m_मूल्य = u.u[1];
    वापस *यह;
}
V3_संख्या& V3_संख्या::setSingleBits(अक्षर मूल्य) {
    क्रम (पूर्णांक i = 1 /*upper*/; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, 0};
    m_तथ्य.अंक()[0] = {(मूल्य == '1' || मूल्य == 'x' || मूल्य == 1 || मूल्य == 3),
                       (मूल्य == 'z' || मूल्य == 'x' || मूल्य == 2 || मूल्य == 3)};
    वापस *यह;
}

V3_संख्या& V3_संख्या::setAllBits0() {
    setZero();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setAllBits1() {
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {~0U, 0};
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {~0U, ~0U};
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setAllBitsZ() {
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, ~0U};
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::setAllBitsXRemoved() {
    अगर (!v3Global.constRemoveXs()) {
        वापस setAllBitsX();
    } अन्यथा {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        अगर (v3Global.opt.xAssign() == "1") {
            वापस setAllBits1();
        } अन्यथा {
            वापस setAllBits0();
        }
    }
}
V3_संख्या& V3_संख्या::setValue1() {
    m_तथ्य.अंक()[0] = {1, 0};
    क्रम (पूर्णांक i = 1; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, 0};
    वापस *यह;
}

व्योम V3_संख्या::setBitX0(पूर्णांक bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_संख्या& V3_संख्या::setMask(पूर्णांक nbits, पूर्णांक lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    वापस *यह;
}

//======================================================================
// ACCESSORS - as strings

string V3_संख्या::ascii(bool prefixed, bool cleanVerilog) स्थिर VL_MT_STABLE {
    // Correct number of zero bits/width matters
    अगर (is1Step()) वापस "1step";
    अगर (isNull()) वापस "null";
    std::ostringstream out;
    अगर (isDouble()) {
        out.precision(17);
        अगर (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << toDouble();
            अगर (toDouble() == न्यूनमान(toDouble())) out << ".0";
        }
        वापस out.str();
    } अन्यथा अगर (isString()) {
        वापस '"' + toString() + '"';
    } अन्यथा {
        अगर (VL_UNCOVERABLE((m_तथ्य.अंक()[शब्दगण() - 1].m_मूल्य | m_तथ्य.अंक()[शब्दगण() - 1].m_मूल्य_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    अगर (prefixed) {
        अगर (sized()) {
            out << width() << "'";
        } अन्यथा अगर (autoExtend() && width() == 1) {
            out << "'";
            अगर (bitIs0(0)) {
                out << '0';
            } अन्यथा अगर (bitIs1(0)) {
                out << '1';
            } अन्यथा अगर (bitIsZ(0)) {
                out << 'z';
            } अन्यथा {
                out << 'x';
            }
            वापस out.str();
        } अन्यथा {
            अगर (cleanVerilog) {
                out << "'";
            } अन्यथा {
                out << "?" << width() << "?";
            }
        }
        अगर (isSigned()) out << "s";
    }

    स्थिर bool द्विग = (isFourState()
#अगर_घोषित V3_संख्या_ASCII_द्विआधारी
                         // cppcheck-suppress konwnConditionTrueFalse
                         || true
#पूर्ण_अगर
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    अगर (द्विग) {
        out << "b";
        out << displayed("%0b");
    } अन्यथा {
        अगर (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    अगर (isNull()) {
        अगर (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } अन्यथा {
            out << " [null]";
        }
    }
    वापस out.str();
}

bool V3_संख्या::displayedFmtHasArg(अक्षर format, bool isScan) {
    // Is this a format letter that takes an argument in runtime?
    (व्योम)isScan;
    चयन (std::छोटे(format)) {
    हाल 'b': वापस true;  //द्विआधारी
    हाल 'c': वापस true;  // Character
    हाल 'd': वापस true;  // Decimal
    हाल 'e': वापस true;  // Floating
    हाल 'f': वापस true;  // Floating
    हाल 'g': वापस true;  // Floating
    हाल 'h': वापस true;  // Hex
    हाल 'o': वापस true;  // Octal
    हाल 'p': वापस true;  // Pattern
    हाल 's': वापस true;  // String
    हाल 't': वापस true;  // Time
    हाल 'u': वापस true;  // Packed 2-state
    हाल 'v': वापस true;  // Strength
    हाल 'x': वापस true;  // Hex
    हाल 'z': वापस true;  // Packed 4-state
    हाल '?': वापस true;  // Internal: Compute format in V3Width based on data type
    शेष: वापस false;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_संख्या::displayPad(माप_प्रकार fmtsize, अक्षर pad, bool बाँए, स्थिर string& in) VL_PURE {
    string padding;
    अगर (in.लंबाई() < fmtsize) padding = string(fmtsize - in.लंबाई(), pad);
    वापस बाँए ? (in + padding) : (padding + in);
}

string V3_संख्या::displayedEnumName(स्थिर AstEnumItem* itemp) {
    स्थिर string name = VIdProtect::protect(itemp->prettyName());
    वापस VString::isIdentifier(name) && !std::है_अंक(static_cast<अचिन्हित अक्षर>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_संख्या::displayedEnumName(स्थिर AstEnumDType* dtypep) स्थिर VL_MT_STABLE {
    V3_संख्या मूल्य{यह, dtypep->width()};
    मूल्य.opAssign(*यह);
    क्रम (स्थिर AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        स्थिर AstConst* स्थिर constp = VN_AS(itemp->valuep(), Const);
        अगर (!constp->अंक().isAnyXZ() && मूल्य.isCaseEq(constp->अंक()))
            वापस displayedEnumName(itemp);
    }
    वापस "";
}

string V3_संख्या::displayedEnum(स्थिर AstSFormatArg* argp,
                               स्थिर string& vformat) स्थिर VL_MT_STABLE {
    स्थिर AstEnumDType* स्थिर dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_संख्या मूल्य{यह, dtypep->width()};
    मूल्य.opAssign(*यह);
    स्थिर VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    स्थिर अक्षर fmt = std::छोटे(vformat.back());
    अगर (fmt != 'p' && fmt != 's') वापस मूल्य.displayed(argp, vformat, numericAttr);
    string name = मूल्य.displayedEnumName(dtypep);
    अगर (name.empty()) name = मूल्य.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_संख्या name_अंक{यह};
    name_अंक.setString(name);
    वापस name_अंक.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_संख्या::displayed(स्थिर Ast_गाँठ* nodep, स्थिर string& vformat,
                           स्थिर VFormatAttr& formatAttr) स्थिर VL_MT_STABLE {
    वापस displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_संख्या::displayed(File_पंक्ति* fl, स्थिर string& vformat,
                           स्थिर VFormatAttr& formatAttr) स्थिर VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    स्वतः pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *यह);
    ++pos;
    bool बाँए = false;
    अगर (pos[0] == '-') {
        बाँए = true;
        ++pos;
    }
    string fmtsize;
    क्रम (; pos != vformat.cend() && (std::है_अंक(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    अक्षर fmt = std::छोटे(pos[0]);

    // Override user's code for certain data-type determined arguments
    अगर (formatAttr.isComplex()) {
        अगर (fmt != 'p') fmt = 's';  // override
    } अन्यथा अगर (formatAttr.isString()) {
        अगर (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_संख्या strwide{fl, static_cast<पूर्णांक>(toString().लंबाई()) * 8, 0};
            strwide.opNToI(*यह);
            वापस strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        अगर (fmt != 'p') fmt = 's';  // override
    } अन्यथा अगर (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        अगर (fmt == 'p') {
            अगर (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } अन्यथा {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } अन्यथा अगर (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        अगर (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_संख्या nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*यह);
            वापस nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    चयन (fmt) {
    हाल 'c': {
        अगर (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        स्थिर अचिन्हित पूर्णांक v = bits_मूल्य(0, 8);
        str.धकेलो_back(static_cast<अक्षर>(v));
        वापस str;
    }
    हाल 'e':  // FALLTHRU
    हाल 'f':  // FALLTHRU
    हाल 'g': {
        स्थिर द्विगुन n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        अक्षर tmp[MAX_SPRINTF_DOUBLE_आकार];
        (व्योम)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_आकार, vformat.c_str(), n);
        वापस tmp;
    }
    हाल 's': {
        अगर (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } अन्यथा {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            पूर्णांक bit = width() - 1;
            bool start = true;
            जबतक ((bit % 8) != 7) ++bit;
            क्रम (; bit >= 0; bit -= 8) {
                स्थिर पूर्णांक v = bits_मूल्य(bit - 7, 8);
                अगर (!start || v) {
                    str += static_cast<अक्षर>((v == 0) ? ' ' : v);
                    start = false;  // Drop leading 0s
                } अन्यथा {
                    अगर (fmtsize != "0") str += ' ';
                }
            }
        }
        स्थिर माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', बाँए, str);
        वापस str;
    }
    हाल 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        अगर (formatAttr.isDouble()) {
            स्थिर द्विगुन n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            अक्षर tmp[MAX_SPRINTF_DOUBLE_आकार];
            (व्योम)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_आकार, "%g", n);
            वापस tmp;
        }
        अगर (formatAttr.isString())
            वापस '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        अगर (formatAttr.isComplex()) वापस toString();
        वापस "%p";
    }
    हाल 'b':  // FALLTHRU
    हाल 'o':  // FALLTHRU
    हाल 'h':  // FALLTHRU
    हाल 'x': {
        पूर्णांक bit = width() - 1;
        अगर (बाँए || !fmtsize.empty()) {
            जबतक (bit && bitIs0(bit)) --bit;
        }
        चयन (fmt) {
        हाल 'b': {
            क्रम (; bit >= 0; --bit) {
                अगर (bitIs0(bit)) {
                    str += '0';
                } अन्यथा अगर (bitIs1(bit)) {
                    str += '1';
                } अन्यथा अगर (bitIsZ(bit)) {
                    str += 'z';
                } अन्यथा {
                    str += 'x';
                }
            }
            अवरोध;
        }
        हाल 'o': {
            जबतक ((bit % 3) != 2) ++bit;
            क्रम (; bit > 0; bit -= 3) {
                स्थिर पूर्णांक अंक_X = गिनती_X(bit - 2, 3);
                स्थिर पूर्णांक अंक_Z = गिनती_Z(bit - 2, 3);
                अगर (अंक_X == 3 || अंक_X == width() - (bit - 2)) {
                    str += 'x';
                    जारी;
                }
                अगर (अंक_Z == 3 || अंक_Z == width() - (bit - 2)) {
                    str += 'z';
                    जारी;
                }
                अगर (अंक_X > 0) {
                    str += 'X';
                    जारी;
                }
                अगर (अंक_Z > 0) {
                    str += 'Z';
                    जारी;
                }
                स्थिर पूर्णांक v = bits_मूल्य(bit - 2, 3);
                str += static_cast<अक्षर>('0' + v);
            }
            अवरोध;
        }
        शेष: {  // h/x
            जबतक ((bit % 4) != 3) ++bit;
            क्रम (; bit > 0; bit -= 4) {
                स्थिर पूर्णांक अंक_X = गिनती_X(bit - 3, 4);
                स्थिर पूर्णांक अंक_Z = गिनती_Z(bit - 3, 4);
                अगर (अंक_X == 4 || अंक_X == width() - (bit - 3)) {
                    str += 'x';
                    जारी;
                }
                अगर (अंक_Z == 4 || अंक_Z == width() - (bit - 3)) {
                    str += 'z';
                    जारी;
                }
                अगर (अंक_X > 0) {
                    str += 'X';
                    जारी;
                }
                अगर (अंक_Z > 0) {
                    str += 'Z';
                    जारी;
                }
                स्थिर पूर्णांक v = bits_मूल्य(bit - 3, 4);
                अगर (v >= 10) {
                    str += static_cast<अक्षर>('a' + v - 10);
                } अन्यथा {
                    str += static_cast<अक्षर>('0' + v);
                }
            }
            अवरोध;
        }
        }  // switch
        स्थिर माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, (बाँए ? ' ' : '0'), बाँए, str);
        वापस str;
    }  // case b/d/x/o
    हाल 't':  // Time
    हाल 'd': {  // Unsigned decimal
        स्थिर bool issigned = formatAttr.isSigned();
        अगर (fmtsize == "" && !बाँए) {
            स्थिर द्विगुन mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            द्विगुन dchars = mantissabits / 3.321928094887362 + 1.0;
            अगर (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<पूर्णांक>(dchars));
        }
        bool hasXZ = false;
        अगर (isAllX()) {
            str = "x";
            hasXZ = true;
        } अन्यथा अगर (isAllZ()) {
            str = "z";
            hasXZ = true;
        } अन्यथा अगर (isAnyX()) {
            str = "X";
            hasXZ = true;
        } अन्यथा अगर (isAnyZ()) {
            str = "Z";
            hasXZ = true;
        }
        अगर (!hasXZ) {
            अगर (issigned) {
                अगर (width() > 64) {
                    str = toDecimalS();
                } अन्यथा {
                    str = cvtToStr(toSQuad());
                }
            } अन्यथा {
                अगर (width() > 64) {
                    str = toDecimalU();
                } अन्यथा {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        स्थिर bool zeropad = fmtsize.लंबाई() > 0 && fmtsize[0] == '0' && !बाँए;
        // fmtsize might have changed since we parsed the %fmtsize
        स्थिर माप_प्रकार fmtsizen = static_cast<माप_प्रकार>(std::म_से_प(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), बाँए, str);
        वापस str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    हाल 'u': {  // Packed 2-state
        क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
            स्थिर uint32_t v = m_तथ्य.अंक()[i].m_मूल्य;
            str += static_cast<अक्षर>((v >> 0) & 0xff);
            str += static_cast<अक्षर>((v >> 8) & 0xff);
            str += static_cast<अक्षर>((v >> 16) & 0xff);
            str += static_cast<अक्षर>((v >> 24) & 0xff);
        }
        वापस str;
    }
    हाल 'z': {  // Packed 4-state
        क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
            स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
            str += static_cast<अक्षर>((v.m_मूल्य >> 0) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य >> 8) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य >> 16) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य >> 24) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 0) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 8) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 16) & 0xff);
            str += static_cast<अक्षर>((v.m_मूल्य_X >> 24) & 0xff);
        }
        वापस str;
    }
    हाल 'v': {  // Strength
        पूर्णांक bit = width() - 1;
        क्रम (; bit >= 0; bit--) {
            अगर (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            अन्यथा अगर (bitIs1(bit)) {
                str += "St1 ";
            } अन्यथा अगर (bitIsZ(bit)) {
                str += "StZ ";
            } अन्यथा {
                str += "StX";
            }
        }
        वापस str;
    }
    शेष: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_संख्या::emitC() स्थिर VL_MT_STABLE {
    constexpr माप_प्रकार bufsize = 1000;
    अक्षर sbuf[bufsize];
    string परिणाम;
    अगर (isNull()) {
        वापस "VlNull{}";
    } अन्यथा अगर (isDouble()) {
        स्थिर द्विगुन dnum = toDouble();
        अगर (std::isinf(dnum)) {
            अगर (std::signbit(dnum)) परिणाम += '-';
            परिणाम += "std::numeric_limits<double>::infinity()";
        } अन्यथा अगर (std::isnan(dnum)) {
            अगर (std::signbit(dnum)) परिणाम += '-';
            परिणाम += "std::numeric_limits<double>::quiet_NaN()";
        } अन्यथा {
            स्थिर अक्षर* स्थिर fmt = (static_cast<पूर्णांक>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (व्योम)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            वापस sbuf;
        }
    } अन्यथा अगर (isString()) {
        स्थिर string strg = toString();
        स्थिर string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        वापस '"' + quoted + "\"s";
    } अन्यथा अगर (शब्दगण() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        परिणाम = "::VlWide<" + std::to_string(शब्दगण()) + ">{{";
        अगर (शब्दगण() > 4) परिणाम += '\n';
        क्रम (पूर्णांक n = 0; n < शब्दगण(); ++n) {
            अगर (n) परिणाम += ((n % 4) ? ", " : ",\n");
            (व्योम)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            परिणाम += sbuf;
        }
        अगर (शब्दगण() > 4) परिणाम += '\n';
        परिणाम += "}}";
    } अन्यथा अगर (शब्दगण() == 2) {  // Quad
        स्थिर uint64_t qnum = static_cast<uint64_t>(toUQuad());
        स्थिर अक्षर* स्थिर fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (व्योम)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        वापस sbuf;
    } अन्यथा {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        स्थिर uint32_t unum = toUInt();
        स्थिर अक्षर* स्थिर fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (व्योम)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        वापस sbuf;
    }
    वापस परिणाम;
}

string V3_संख्या::toDecimalS() स्थिर VL_MT_STABLE {
    // Correct number of zero bits/width matters
    अगर (isNegative()) {
        V3_संख्या lhsNoSign = *यह;
        lhsNoSign.opNegate(*यह);
        वापस "-"s + lhsNoSign.toDecimalU();
    } अन्यथा {
        वापस toDecimalU();
    }
}

string V3_संख्या::toDecimalU() स्थिर VL_MT_STABLE {
    स्थिर पूर्णांक maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_संख्या bcd(यह, maxdecwidth + 4);
    V3_संख्या tmp(यह, maxdecwidth + 4);
    V3_संख्या tmp2(यह, maxdecwidth + 4);

    पूर्णांक from_bit = width() - 1;
    // Skip all leading zeros
    क्रम (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    क्रम (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        क्रम (पूर्णांक nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            अगर (bcd.bits_मूल्य(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_संख्या(यह, 32, 1));
        // bcd[0] = this[from_bit]
        अगर (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    पूर्णांक lsb = (maxdecwidth - 1) & ~3;
    क्रम (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        अगर (bcd.bits_मूल्य(lsb, 4)) अवरोध;
    }
    क्रम (; lsb >= 0; lsb -= 4) {
        output += static_cast<अक्षर>('0' + bcd.bits_मूल्य(lsb, 4));  // 0..9
    }
    वापस output;
}

string V3_संख्या::toRealString() स्थिर VL_MT_STABLE {
    स्थिर द्विगुन मूल्य = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << मूल्य;
    string परिणाम = out.str();
    अगर (std::म_से_भग्न(परिणाम.c_str(), nullptr) != मूल्य) {
        out.str("");
        out.precision(17);
        out << मूल्य;
        परिणाम = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    अगर (परिणाम.find_first_of(".en") == string::npos) परिणाम += ".0";
    वापस परिणाम;
}

//======================================================================
// ACCESSORS - as numbers

bool V3_संख्या::fitsInUInt() स्थिर VL_MT_SAFE {
    अगर (isFourState()) वापस false;
    // We allow wide numbers that represent values <= 32 bits
    क्रम (पूर्णांक i = 1; i < शब्दगण(); ++i) {
        अगर (m_तथ्य.अंक()[i].m_मूल्य) वापस false;
    }
    वापस true;
}
uint32_t V3_संख्या::toUInt() स्थिर VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *यह);
    // We allow wide numbers that represent values <= 32 bits
    अगर (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *यह);
    }
    वापस m_तथ्य.अंक()[0].m_मूल्य;
}

द्विगुन V3_संख्या::toDouble() स्थिर VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    जोड़ {
        द्विगुन d;
        uint32_t u[2];
    } u;
    u.u[0] = m_तथ्य.अंक()[0].m_मूल्य;
    u.u[1] = m_तथ्य.अंक()[1].m_मूल्य;
    वापस u.d;
}

int32_t V3_संख्या::toSInt() स्थिर VL_MT_SAFE {
    अगर (isSigned()) {
        स्थिर uint32_t v = toUInt();
        स्थिर uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        स्थिर uint32_t extended = v | signExtend;
        वापस static_cast<int32_t>(extended);
    } अन्यथा {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        वापस static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_संख्या::toUQuad() स्थिर VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *यह);
    // We allow wide numbers that represent values <= 64 bits
    अगर (isDouble()) वापस static_cast<uint64_t>(toDouble());
    क्रम (पूर्णांक i = 2; i < शब्दगण(); ++i) {
        अगर (m_तथ्य.अंक()[i].m_मूल्य) {
            v3error("Value too wide for 64-bits expected in this context " << *यह);
            अवरोध;
        }
    }
    अगर (width() <= 32) वापस (static_cast<uint64_t>(toUInt()));
    वापस ((static_cast<uint64_t>(m_तथ्य.अंक()[1].m_मूल्य) << 32ULL)
            | (static_cast<uint64_t>(m_तथ्य.अंक()[0].m_मूल्य)));
}

int64_t V3_संख्या::toSQuad() स्थिर {
    अगर (isDouble()) वापस static_cast<int64_t>(toDouble());
    स्थिर uint64_t v = toUQuad();
    स्थिर uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    स्थिर uint64_t extended = v | signExtend;
    वापस static_cast<int64_t>(extended);
}

string V3_संख्या::toString() स्थिर {
    UASSERT(!isFourState(), "toString with 4-state " << *यह);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    अगर (isString()) वापस m_तथ्य.str();
    पूर्णांक bit = width() - 1;
    जबतक ((bit % 8) != 7) ++bit;
    std::string str;
    क्रम (; bit >= 0; bit -= 8) {
        स्थिर पूर्णांक v = bits_मूल्य(bit - 7, 8);
        अगर (v) str += static_cast<अक्षर>(v);
    }
    वापस str;
}

uint32_t V3_संख्या::edataWord(पूर्णांक eword) स्थिर {
    UASSERT(!isFourState(), "edataWord with 4-state " << *यह);
    वापस m_तथ्य.अंक()[eword].m_मूल्य;
}

uint8_t V3_संख्या::तथ्य_Byte(पूर्णांक byte) स्थिर {
    वापस (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

bool V3_संख्या::isAllZ() स्थिर VL_MT_SAFE {
    अगर (isDouble() || isString()) वापस false;
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        स्थिर uint32_t mask = i == शब्दगण() - 1 ? hiWordMask() : ~0U;
        अगर (((~v.m_मूल्य & v.m_मूल्य_X) & mask) != mask) वापस false;
    }
    वापस true;
}
bool V3_संख्या::isAllX() स्थिर VL_MT_SAFE {
    अगर (isDouble() || isString()) वापस false;
    uint32_t mask = hiWordMask();
    क्रम (पूर्णांक i = शब्दगण() - 1; i >= 0; --i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        अगर ((v.m_मूल्य & v.m_मूल्य_X) ^ mask) वापस false;
        mask = ~0U;
    }
    वापस true;
}
bool V3_संख्या::isEqZero() स्थिर VL_MT_SAFE {
    अगर (isString()) वापस m_तथ्य.str().empty();
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        अगर (v.m_मूल्य || v.m_मूल्य_X) वापस false;
    }
    वापस true;
}
bool V3_संख्या::isNeqZero() स्थिर {
    अगर (isString()) वापस !m_तथ्य.str().empty();
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        अगर (v.m_मूल्य & ~v.m_मूल्य_X) वापस true;
    }
    वापस false;
}
bool V3_संख्या::isBitsZero(पूर्णांक msb, पूर्णांक lsb) स्थिर {
    क्रम (पूर्णांक i = lsb; i <= msb; ++i) {
        अगर (VL_UNLIKELY(!bitIs0(i))) वापस false;
    }
    वापस true;
}
bool V3_संख्या::isEqOne() स्थिर {
    अगर (m_तथ्य.अंक()[0].m_मूल्य != 1 || m_तथ्य.अंक()[0].m_मूल्य_X) वापस false;
    क्रम (पूर्णांक i = 1; i < शब्दगण(); ++i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        अगर (v.m_मूल्य || v.m_मूल्य_X) वापस false;
    }
    वापस true;
}
bool V3_संख्या::isEqAllOnes(पूर्णांक optwidth) स्थिर {
    // Correct number of zero bits/width matters
    अगर (!optwidth) optwidth = width();
    क्रम (पूर्णांक bit = 0; bit < optwidth; ++bit) {
        अगर (!bitIs1(bit)) वापस false;
    }
    वापस true;
}
bool V3_संख्या::isFourState() स्थिर VL_MT_SAFE {
    अगर (isDouble() || isString()) वापस false;
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        अगर (m_तथ्य.अंक()[i].m_मूल्य_X) वापस true;
    }
    वापस false;
}
bool V3_संख्या::isAnyX() स्थिर VL_MT_SAFE {
    अगर (isDouble() || isString()) वापस false;
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        अगर (v.m_मूल्य & v.m_मूल्य_X) वापस true;
    }
    वापस false;
}
bool V3_संख्या::isAnyXZ() स्थिर {
    अगर (isDouble() || isString()) वापस false;
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        अगर (m_तथ्य.अंक()[i].m_मूल्य_X) वापस true;
    }
    वापस false;
}
bool V3_संख्या::isAnyZ() स्थिर VL_MT_SAFE {
    अगर (isDouble() || isString()) वापस false;
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
        अगर (~v.m_मूल्य & v.m_मूल्य_X) वापस true;
    }
    वापस false;
}
bool V3_संख्या::isLtXZ(स्थिर V3_संख्या& rhs) स्थिर {
    // Include X/Z in comparisons for sort ordering
    अगर (isString()) वापस toString() < rhs.toString();
    अगर (isDouble()) वापस toDouble() < rhs.toDouble();
    क्रम (पूर्णांक bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        अगर (bitIs1(bit) && rhs.bitIs0(bit)) वापस true;
        अगर (rhs.bitIs1(bit) && bitIs0(bit)) वापस false;
        अगर (bitIsXZ(bit)) वापस true;
        अगर (rhs.bitIsXZ(bit)) वापस false;
    }
    वापस false;
}
पूर्णांक V3_संख्या::गिनती_X(पूर्णांक lsb, पूर्णांक nbits) स्थिर VL_MT_SAFE {
    पूर्णांक गिनती = 0;
    क्रम (पूर्णांक bitn = 0; bitn < nbits; ++bitn) {
        अगर (lsb + bitn >= width()) वापस गिनती;
        अगर (bitIsX(lsb + bitn)) ++गिनती;
    }
    वापस गिनती;
}
पूर्णांक V3_संख्या::गिनती_Z(पूर्णांक lsb, पूर्णांक nbits) स्थिर VL_MT_SAFE {
    पूर्णांक गिनती = 0;
    क्रम (पूर्णांक bitn = 0; bitn < nbits; ++bitn) {
        अगर (lsb + bitn >= width()) वापस गिनती;
        अगर (bitIsZ(lsb + bitn)) ++गिनती;
    }
    वापस गिनती;
}

पूर्णांक V3_संख्या::widthToFit() स्थिर {
    क्रम (पूर्णांक bit = width() - 1; bit > 0; bit--) {
        अगर (!bitIs0(bit)) वापस bit + 1;
    }
    वापस 1;  // one bit even if number is == 0
}

uint32_t V3_संख्या::गिनती_Bits(स्थिर V3_संख्या& ctrl) स्थिर {
    // Correct number of zero bits/width matters
    पूर्णांक n = 0;
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        चयन (ctrl.bitIs(0)) {
        हाल '0':
            अगर (bitIs0(bit)) ++n;
            अवरोध;
        हाल '1':
            अगर (bitIs1(bit)) ++n;
            अवरोध;
        हाल 'x':
            अगर (bitIsX(bit)) ++n;
            अवरोध;
        हाल 'z':
            अगर (bitIsZ(bit)) ++n;
            अवरोध;
        }
    }
    वापस n;
}

uint32_t V3_संख्या::गिनती_Bits(स्थिर V3_संख्या& ctrl1, स्थिर V3_संख्या& ctrl2,
                             स्थिर V3_संख्या& ctrl3) स्थिर {
    // Correct number of zero bits/width matters
    पूर्णांक n = गिनती_Bits(ctrl1);
    अगर (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += गिनती_Bits(ctrl2);
    अगर ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += गिनती_Bits(ctrl3);
    }
    वापस n;
}

uint32_t V3_संख्या::गिनती_Ones() स्थिर {
    पूर्णांक n = 0;
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (bitIs1(bit)) ++n;
    }
    वापस n;
}

uint32_t V3_संख्या::mostSetBitP1() स्थिर {
    क्रम (पूर्णांक bit = width() - 1; bit >= 0; bit--) {
        अगर (!bitIs0(bit)) वापस bit + 1;
    }
    वापस 0;
}

uint32_t V3_संख्या::leastSetBitP1() स्थिर {
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (!bitIs0(bit)) वापस bit + 1;
    }
    वापस 0;
}

//======================================================================

V3_संख्या& V3_संख्या::opBitsNonXZ(स्थिर V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opBitsOne(स्थिर V3_संख्या& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opBitsXZ(स्थिर V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opBitsZ(स्थिर V3_संख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    वापस *यह;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_संख्या& V3_संख्या::opRedOr(स्थिर V3_संख्या& lhs) {
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अक्षर outc = 0;
    क्रम (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        अगर (lhs.bitIs1(bit)) {
            वापस setSingleBits(1);
        } अन्यथा अगर (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opRedAnd(स्थिर V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अक्षर outc = 1;
    क्रम (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        अगर (lhs.bitIs0(bit)) {
            वापस setSingleBits(0);
        } अन्यथा अगर (lhs.bitIs1(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opRedXor(स्थिर V3_संख्या& lhs) {
    // op i, 1 bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अक्षर outc = 0;
    क्रम (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        अगर (lhs.bitIs1(bit)) {
            अगर (outc == 1) {
                outc = 0;
            } अन्यथा अगर (outc == 0) {
                outc = 1;
            }
        } अन्यथा अगर (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::op_गिनती_Bits(स्थिर V3_संख्या& व्यंजक, स्थिर V3_संख्या& ctrl1, स्थिर V3_संख्या& ctrl2,
                                स्थिर V3_संख्या& ctrl3) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS4(व्यंजक, ctrl1, ctrl2, ctrl3);
    अंक_ASSERT_LOGIC_ARGS4(व्यंजक, ctrl1, ctrl2, ctrl3);
    setZero();
    m_तथ्य.अंक()[0].m_मूल्य = व्यंजक.गिनती_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::op_गिनती_Ones(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अगर (lhs.isFourState()) वापस setAllBitsX();
    setZero();
    m_तथ्य.अंक()[0].m_मूल्य = lhs.गिनती_Ones();
    opCleanThis();
    वापस *यह;
}
V3_संख्या& V3_संख्या::opIsUnknown(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    वापस setSingleBits(lhs.isAnyXZ());
}
V3_संख्या& V3_संख्या::opOneHot(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अगर (lhs.isFourState()) वापस setAllBitsX();
    वापस setSingleBits(lhs.गिनती_Ones() == 1);
}
V3_संख्या& V3_संख्या::opOneHot0(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अगर (lhs.isFourState()) वापस setAllBitsX();
    वापस setSingleBits(lhs.गिनती_Ones() <= 1);
}
V3_संख्या& V3_संख्या::opCLog2(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अगर (lhs.isFourState()) वापस setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    स्थिर पूर्णांक adjust = (lhs.गिनती_Ones() == 1) ? 0 : 1;
    क्रम (पूर्णांक bit = lhs.width() - 1; bit >= 0; bit--) {
        अगर (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            वापस *यह;
        }
    }
    setZero();
    वापस *यह;
}
V3_संख्या& V3_संख्या::opMostSetBitP1(स्थिर V3_संख्या& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अगर (lhs.isFourState()) वापस setAllBitsX();
    क्रम (पूर्णांक bit = lhs.width() - 1; bit >= 0; bit--) {
        अगर (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            वापस *यह;
        }
    }
    setZero();
    वापस *यह;
}

V3_संख्या& V3_संख्या::opLogNot(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    अक्षर outc = 1;
    क्रम (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        अगर (lhs.bitIs1(bit)) {
            outc = 0;
            जाओ last;
        } अन्यथा अगर (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
last:
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opNot(स्थिर V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा अगर (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opAnd(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा अगर (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opOr(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा अगर (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } अन्यथा {
            setBit(bit, 'x');
        }
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opXor(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } अन्यथा अगर (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } अन्यथा अगर (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    वापस *यह;
}

व्योम V3_संख्या::fillBits(पूर्णांक destLsb, पूर्णांक width, अक्षर मूल्य) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    अगर (!width) वापस;
    UASSERT(width <= यह->width() - destLsb, "Destination bit range exceeds number width");

    स्थिर uint32_t मूल्य_Word = मूल्य == '1' || मूल्य == 'x' || मूल्य == 1 || मूल्य == 3 ? ~0U : 0;
    स्थिर uint32_t xWord = मूल्य == 'z' || मूल्य == 'x' || मूल्य == 2 || मूल्य == 3 ? ~0U : 0;
    स्थिर पूर्णांक firstWord = VL_BITWORD_E(destLsb);
    स्थिर पूर्णांक lastWord = VL_BITWORD_E(destLsb + width - 1);
    क्रम (पूर्णांक word = firstWord; word <= lastWord; ++word) {
        स्थिर पूर्णांक wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        स्थिर पूर्णांक wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        स्थिर uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        मूल्य_And_X& dest = m_तथ्य.अंक()[word];
        dest.m_मूल्य = (dest.m_मूल्य & ~mask) | (मूल्य_Word & mask);
        dest.m_मूल्य_X = (dest.m_मूल्य_X & ~mask) | (xWord & mask);
    }
}

व्योम V3_संख्या::copyBits(पूर्णांक destLsb, स्थिर V3_संख्या& source, पूर्णांक sourceLsb, पूर्णांक width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    अगर (!width) वापस;
    UASSERT(width <= यह->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    स्थिर पूर्णांक firstWord = VL_BITWORD_E(destLsb);
    स्थिर पूर्णांक lastWord = VL_BITWORD_E(destLsb + width - 1);
    क्रम (पूर्णांक destWord = firstWord; destWord <= lastWord; ++destWord) {
        स्थिर पूर्णांक destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        स्थिर पूर्णांक destOffset = VL_BITBIT_E(destBit);
        स्थिर पूर्णांक sourceBit = sourceLsb + destBit - destLsb;
        स्थिर पूर्णांक sourceWord = VL_BITWORD_E(sourceBit);
        स्थिर पूर्णांक sourceOffset = VL_BITBIT_E(sourceBit);
        स्थिर पूर्णांक copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        स्थिर uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        स्थिर मूल्य_And_X& sourceLo = source.m_तथ्य.अंक()[sourceWord];
        uint32_t मूल्य = sourceLo.m_मूल्य >> sourceOffset;
        uint32_t मूल्य_X = sourceLo.m_मूल्य_X >> sourceOffset;
        अगर (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            स्थिर मूल्य_And_X& sourceHi = source.m_तथ्य.अंक()[sourceWord + 1];
            मूल्य |= sourceHi.m_मूल्य << (VL_EDATASIZE - sourceOffset);
            मूल्य_X |= sourceHi.m_मूल्य_X << (VL_EDATASIZE - sourceOffset);
        }

        मूल्य_And_X& dest = m_तथ्य.अंक()[destWord];
        dest.m_मूल्य = (dest.m_मूल्य & ~mask) | ((मूल्य << destOffset) & mask);
        dest.m_मूल्य_X = (dest.m_मूल्य_X & ~mask) | ((मूल्य_X << destOffset) & mask);
    }
}

V3_संख्या& V3_संख्या::opConcat(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    अगर (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    वापस *यह;
}

V3_संख्या& V3_संख्या::opLenN(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().लंबाई());
    वापस *यह;
}

V3_संख्या& V3_संख्या::opRepl(स्थिर V3_संख्या& lhs,
                           स्थिर V3_संख्या& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    वापस opRepl(lhs, rhs.toUInt());
}

V3_संख्या& V3_संख्या::opRepl(स्थिर V3_संख्या& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अगर (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    क्रम (अचिन्हित बार = 0; बार < rhsval; ++बार) {
        स्थिर uint64_t destLsb = uint64_t{बार} * lhs.width();
        अगर (destLsb >= static_cast<uint32_t>(width())) अवरोध;
        copyBits(static_cast<पूर्णांक>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<पूर्णांक>(destLsb)));
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opStreamL(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    अगर (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    स्थिर पूर्णांक ssize = std::min(rhs.toUInt(), static_cast<अचिन्हित>(lhs.width()));
    क्रम (पूर्णांक istart = 0; istart < lhs.width(); istart += ssize) {
        स्थिर पूर्णांक ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opLogAnd(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर loutc = 0;
    अक्षर routc = 0;
    क्रम (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        अगर (lhs.bitIs1(bit)) {
            loutc = 1;
            अवरोध;
        }
        अगर (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    क्रम (पूर्णांक bit = 0; bit < rhs.width(); ++bit) {
        अगर (rhs.bitIs1(bit)) {
            routc = 1;
            अवरोध;
        }
        अगर (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    अक्षर outc = 'x';
    अगर (routc == 1 && loutc == 1) outc = 1;
    अगर (routc == 0 || loutc == 0) outc = 0;
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opLogOr(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    क्रम (पूर्णांक bit = 0; bit < lhs.width(); ++bit) {
        अगर (lhs.bitIs1(bit)) {
            outc = 1;
            जाओ last;
        }
        अगर (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    क्रम (पूर्णांक bit = 0; bit < rhs.width(); ++bit) {
        अगर (rhs.bitIs1(bit)) {
            outc = 1;
            जाओ last;
        }
        अगर (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opLogIf(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या lnot = lhs;
    lnot.opLogNot(lhs);
    वापस opLogOr(lnot, rhs);
}

V3_संख्या& V3_संख्या::opLogEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_संख्या ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    वापस opLogAnd(ifa, ifb);
}

V3_संख्या& V3_संख्या::opAtoN(स्थिर V3_संख्या& lhs, पूर्णांक base) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::हटाओ(str.begin(), str.end(), '_'), str.end());

    अगर (base == AstAtoN::ATOREAL) वापस setDouble(std::म_से_भ(str.c_str()));

    त्रुटि_सं = 0;
    स्वतः v = std::म_से_दीर्घ(str.c_str(), nullptr, base);
    अगर (त्रुटि_सं != 0) v = 0;
    वापस setLongS(static_cast<int32_t>(v));
}

V3_संख्या& V3_संख्या::opPutcN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, स्थिर V3_संख्या& ths) {
    अंक_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अंक_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    स्थिर int32_t i = rhs.toSInt();
    स्थिर int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    अगर (0 <= i && static_cast<uint32_t>(i) < lstring.लंबाई() && c != 0) lstring[i] = c;
    वापस setString(lstring);
}

V3_संख्या& V3_संख्या::opGetcN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिर string lstring = lhs.toString();
    स्थिर int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    अगर (0 <= i && static_cast<uint32_t>(i) < lstring.लंबाई()) v = lstring[i];
    वापस setLong(v);
}

V3_संख्या& V3_संख्या::opSubstrN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, स्थिर V3_संख्या& ths) {
    अंक_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिर string lstring = lhs.toString();
    स्थिर int32_t i = rhs.toSInt();
    स्थिर int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    अगर (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.लंबाई()) वापस setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    वापस setString(lstring.substr(i, j - i + 1));
}

V3_संख्या& V3_संख्या::opCompareNN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, bool ignoreCase) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    पूर्णांक परिणाम;
    स्थिर string lstring = lhs.toString();
    स्थिर string rstring = rhs.toString();
    अगर (ignoreCase) {
        परिणाम = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } अन्यथा {
        परिणाम = std::म_भेद(lstring.c_str(), rstring.c_str());
    }
    वापस setLongS(परिणाम);
}

V3_संख्या& V3_संख्या::opEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अगर (lhs.isString()) वापस opEqN(lhs, rhs);
    अगर (lhs.isDouble()) वापस opEqD(lhs, rhs);
    अक्षर outc = 1;
    क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        अगर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            जाओ last;
        }
        अगर (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            जाओ last;
        }
        अगर (lhs.bitIsXZ(bit)) outc = 'x';
        अगर (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opNeq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अगर (lhs.isString()) वापस opNeqN(lhs, rhs);
    अगर (lhs.isDouble()) वापस opNeqD(lhs, rhs);
    अक्षर outc = 0;
    क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        अगर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            जाओ last;
        }
        अगर (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            जाओ last;
        }
        अगर (lhs.bitIsXZ(bit)) outc = 'x';
        अगर (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    वापस setSingleBits(outc);
}

V3Hash V3_संख्या::toHash() स्थिर {
    // Shouldmatch isCaseEq
    अगर (isString()) वापस V3Hash{toString()};
    अगर (isDouble()) वापस V3Hash{toDouble()};
    V3Hash hash{width()};
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) hash += m_तथ्य.अंक()[i].m_मूल्य;
    वापस hash;
}

bool V3_संख्या::isCaseEq(स्थिर V3_संख्या& rhs) स्थिर {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अगर (isString()) वापस toString() == rhs.toString();
    अगर (isDouble()) वापस toDouble() == rhs.toDouble();
    अगर (width() != rhs.width()) वापस false;
    क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
        अगर (!(m_तथ्य.अंक()[i] == rhs.m_तथ्य.अंक()[i])) वापस false;
    }
    वापस true;
}

V3_संख्या& V3_संख्या::opCaseEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_संख्या& V3_संख्या::opCaseNeq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    अगर (lhs.isString()) {
        वापस opNeqN(lhs, rhs);
    } अन्यथा अगर (lhs.isDouble()) {
        वापस opNeqD(lhs, rhs);
    }
    क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        अगर (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            जाओ last;
        }
    }
last:
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opWildEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 1;
    क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        अगर (!rhs.bitIsXZ(bit)) {
            अगर (lhs.bitIsXZ(bit)) {
                outc = 'x';
                जाओ last;
            }
            अगर (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                जाओ last;
            }
        }
    }
last:
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opWildNeq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        अगर (!rhs.bitIsXZ(bit)) {
            अगर (lhs.bitIsXZ(bit)) {
                outc = 'x';
                जाओ last;
            }
            अगर (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                जाओ last;
            }
        }
    }
last:
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGt(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        अगर (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        अगर (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        अगर (lhs.bitIsXZ(bit)) outc = 'x';
        अगर (rhs.bitIsXZ(bit)) outc = 'x';
    }
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGtS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अक्षर outc = 0;
    {
        स्थिर पूर्णांक mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        अगर (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } अन्यथा अगर (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } अन्यथा अगर (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } अन्यथा अगर (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } अन्यथा {
            // both positive or negative, normal >
            क्रम (पूर्णांक bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                अगर (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                अगर (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                अगर (lhs.bitIsXZ(bit)) outc = 'x';
                अगर (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    वापस setSingleBits(outc);
}

V3_संख्या& V3_संख्या::opGte(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या& eq = opEq(lhs, rhs);
    अगर (eq.isNeqZero()) वापस eq;  // Return true
    वापस opGt(lhs, rhs);
}

V3_संख्या& V3_संख्या::opGteS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    V3_संख्या& eq = opEq(lhs, rhs);
    अगर (eq.isNeqZero()) वापस eq;  // Return true
    वापस opGtS(lhs, rhs);
}

V3_संख्या& V3_संख्या::opLt(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { वापस opGt(rhs, lhs); }
V3_संख्या& V3_संख्या::opLte(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { वापस opGte(rhs, lhs); }
V3_संख्या& V3_संख्या::opLtS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { वापस opGtS(rhs, lhs); }
V3_संख्या& V3_संख्या::opLteS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) { वापस opGteS(rhs, lhs); }

V3_संख्या& V3_संख्या::opShiftR(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // L(lhs) bit return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (rhs.isFourState()) वापस setAllBitsX();
    setZero();
    अगर (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) वापस *यह;
    स्थिर uint32_t rhsval = rhs.toUInt();
    अगर (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<पूर्णांक>(rhsval),
                 std::min(width(), lhs.width() - static_cast<पूर्णांक>(rhsval)));
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opShiftRS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (rhs.isFourState()) वापस setAllBitsX();
    setZero();
    स्थिर bool overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    अगर (!overflow) {
        स्थिर uint32_t rhsval = rhs.toUInt();
        अगर (rhsval < lbits) {
            स्थिर पूर्णांक copyWidth = std::min(width(), static_cast<पूर्णांक>(lbits - rhsval));
            copyBits(0, lhs, static_cast<पूर्णांक>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            वापस *यह;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    वापस *यह;
}

V3_संख्या& V3_संख्या::opShiftL(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // L(lhs) bit return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (rhs.isFourState()) वापस setAllBitsX();
    setZero();
    अगर (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) वापस *यह;
    स्थिर uint32_t rhsval = rhs.toUInt();
    अगर (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<पूर्णांक>(rhsval), lhs, 0,
                 std::min(width() - static_cast<पूर्णांक>(rhsval), lhs.width()));
    }
    वापस *यह;
}

//======================================================================
// Ops - Arithmetic

V3_संख्या& V3_संख्या::opNegate(स्थिर V3_संख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अगर (lhs.isFourState()) वापस setAllBitsX();
    V3_संख्या notlhs(&lhs, width());
    notlhs.opNot(lhs);
    स्थिर V3_संख्या one(&lhs, width(), 1);
    opAdd(notlhs, one);
    वापस *यह;
}
V3_संख्या& V3_संख्या::opAdd(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    क्रम (पूर्णांक word = 0; word < शब्दगण(); ++word) {
        स्थिर uint64_t lwordval = lhs.m_तथ्य.अंक()[word].m_मूल्य;
        स्थिर uint64_t rwordval = rhs.m_तथ्य.अंक()[word].m_मूल्य;
        स्थिर uint64_t योग_2 = lwordval + rwordval + carry;
        m_तथ्य.अंक()[word].m_मूल्य = योग_2 & 0xffffffffULL;
        carry = योग_2 > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    वापस *यह;
}
V3_संख्या& V3_संख्या::opSub(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    V3_संख्या negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    वापस opAdd(lhs, negrhs);
}
V3_संख्या& V3_संख्या::opMul(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    setZero();
    अगर (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } अन्यथा {
        क्रम (पूर्णांक lword = 0; lword < lhs.शब्दगण(); ++lword) {
            स्थिर uint64_t lwordval = static_cast<uint64_t>(lhs.m_तथ्य.अंक()[lword].m_मूल्य);
            अगर (lwordval == 0) जारी;
            क्रम (पूर्णांक rword = 0; rword < rhs.शब्दगण(); ++rword) {
                स्थिर uint64_t rwordval = static_cast<uint64_t>(rhs.m_तथ्य.अंक()[rword].m_मूल्य);
                अगर (rwordval == 0) जारी;
                uint64_t mul = lwordval * rwordval;
                क्रम (पूर्णांक qword = lword + rword; qword < शब्दगण(); ++qword) {
                    mul += static_cast<uint64_t>(m_तथ्य.अंक()[qword].m_मूल्य);
                    m_तथ्य.अंक()[qword].m_मूल्य = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    अगर (mul == 0) अवरोध;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opMulS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    V3_संख्या lhsNoSign = lhs;
    अगर (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    अगर (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    स्थिर V3_संख्या qNoSign = opMul(lhsNoSign, rhsNoSign);
    अगर ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opDiv(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    अगर (rhs.isEqZero()) वापस setAllBitsXRemoved();
    अगर (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        वापस *यह;
    } अन्यथा {
        // Wide division
        वापस opModDivGuts(lhs, rhs, false);
    }
}
V3_संख्या& V3_संख्या::opDivS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    अगर (rhs.isEqZero()) वापस setAllBitsXRemoved();
    V3_संख्या lhsNoSign = lhs;
    अगर (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    अगर (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    स्थिर V3_संख्या qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    अगर ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *यह);
    वापस *यह;
}
V3_संख्या& V3_संख्या::opModDiv(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    अगर (rhs.isEqZero()) वापस setAllBitsXRemoved();
    अगर (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        वापस *यह;
    } अन्यथा {
        // Wide modulus
        वापस opModDivGuts(lhs, rhs, true);
    }
}
V3_संख्या& V3_संख्या::opModDivS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    अगर (rhs.isEqZero()) वापस setAllBitsXRemoved();
    V3_संख्या lhsNoSign = lhs;
    अगर (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_संख्या rhsNoSign = rhs;
    अगर (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    स्थिर V3_संख्या qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    अगर (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } अन्यथा {
        opAssign(qNoSign);
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opModDivGuts(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, bool is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    स्थिर पूर्णांक l_शब्दगण = lhs.शब्दगण();
    स्थिर पूर्णांक umsbp1 = lhs.mostSetBitP1();  // dividend
    स्थिर पूर्णांक vmsbp1 = rhs.mostSetBitP1();  //भाजक
    अगर (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *यह);
        वापस *यह;
    }

    स्थिर पूर्णांक uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    स्थिर पूर्णांक vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    अगर (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        क्रम (पूर्णांक j = uw - 1; j >= 0; j--) {
            स्थिर uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_तथ्य.अंक()[j].m_मूल्य));
            m_तथ्य.अंक()[j].m_मूल्य = unw64 / static_cast<uint64_t>(rhs.m_तथ्य.अंक()[0].m_मूल्य);
            k = unw64
                - (static_cast<uint64_t>(m_तथ्य.अंक()[j].m_मूल्य)
                   * static_cast<uint64_t>(rhs.m_तथ्य.अंक()[0].m_मूल्य));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *यह << " rem=0x" << std::षष्ठ
                                   << k << std::डेसी);
        अगर (is_modulus) {
            setZero();
            m_तथ्य.अंक()[0].m_मूल्य = k;
        }
        opCleanThis();
        वापस *यह;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_शब्दगण + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_शब्दगण + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    क्रम (पूर्णांक i = 0; i < l_शब्दगण; ++i) m_तथ्य.अंक()[i].m_मूल्य = 0;
    क्रम (पूर्णांक i = 0; i < l_शब्दगण + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    स्थिर पूर्णांक s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    स्थिर uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    क्रम (पूर्णांक i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_तथ्य.अंक()[i].m_मूल्य << s)
                | (shift_mask & (rhs.m_तथ्य.अंक()[i - 1].m_मूल्य >> (32 - s)));
    }
    vn[0] = rhs.m_तथ्य.अंक()[0].m_मूल्य << s;

    // Copy and shift dividend by same amount; may set new upper word
    अगर (s) {
        un[uw] = lhs.m_तथ्य.अंक()[uw - 1].m_मूल्य >> (32 - s);
    } अन्यथा {
        un[uw] = 0;
    }
    क्रम (पूर्णांक i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_तथ्य.अंक()[i].m_मूल्य << s)
                | (shift_mask & (lhs.m_तथ्य.अंक()[i - 1].m_मूल्य >> (32 - s)));
    }
    un[0] = lhs.m_तथ्य.अंक()[0].m_मूल्य << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    क्रम (पूर्णांक j = uw - vw; j >= 0; j--) {
        // Estimate
        स्थिर uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        अगर (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            अगर (rhat < 0x100000000ULL) जाओ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        क्रम (पूर्णांक i = 0; i < vw; ++i) {
            स्थिर uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_तथ्य.अंक()[j].m_मूल्य = qhat;  // Save quotient digit

        अगर (t < 0) {
            // Over subtracted; correct by adding back
            m_तथ्य.अंक()[j].m_मूल्य--;
            k = 0;
            क्रम (पूर्णांक i = 0; i < vw; ++i) {
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

    अगर (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        क्रम (पूर्णांक i = 0; i < vw; ++i) {
            m_तथ्य.अंक()[i].m_मूल्य = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        क्रम (पूर्णांक i = vw; i < l_शब्दगण; ++i) m_तथ्य.अंक()[i].m_मूल्य = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *यह);
        वापस *यह;
    } अन्यथा {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *यह);
        वापस *यह;
    }
}

V3_संख्या& V3_संख्या::opPow(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, bool lsign, bool rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_LOGIC_ARGS2(lhs, rhs);
    अगर (lhs.isFourState() || rhs.isFourState()) वापस setAllBitsX();
    अगर (rhs.isEqZero()) वापस setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    अगर (rsign && rhs.isNegative()) {
        अगर (lhs.isEqZero()) {
            वापस setAllBitsXRemoved();
        } अन्यथा अगर (lhs.isEqOne()) {
            वापस setValue1();
        } अन्यथा अगर (lsign && lhs.isEqAllOnes()) {
            अगर (rhs.bitIs1(0)) {
                वापस setAllBits1();  // -1^odd=-1
            } अन्यथा {
                वापस setValue1();  // -1^even=1
            }
        }
        वापस setZero();
    }
    अगर (lhs.isEqZero()) वापस setZero();
    setZero();
    m_तथ्य.अंक()[0].m_मूल्य = 1;
    V3_संख्या power(&lhs, width());
    power.opAssign(lhs);
    क्रम (पूर्णांक bit = 0; bit < rhs.width(); ++bit) {
        अगर (bit > 0) {  // power = power*power
            V3_संख्या lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        अगर (rhs.bitIs1(bit)) {  // out *= power
            V3_संख्या lastOut(&lhs, width());
            lastOut.opAssign(*यह);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opPowSU(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    वापस opPow(lhs, rhs, true, false);
}
V3_संख्या& V3_संख्या::opPowSS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    वापस opPow(lhs, rhs, true, true);
}
V3_संख्या& V3_संख्या::opPowUS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // Correct number of zero bits/width matters
    वापस opPow(lhs, rhs, false, true);
}

V3_संख्या& V3_संख्या::opBufIf1(स्थिर V3_संख्या& ens, स्थिर V3_संख्या& if1s) {
    अंक_ASSERT_OP_ARGS2(ens, if1s);
    अंक_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    क्रम (पूर्णांक bit = 0; bit < width(); ++bit) {
        अगर (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } अन्यथा {
            setBit(bit, 'z');
        }
    }
    वापस *यह;
}

// Sets all bits in range to the given value
V3_संख्या& V3_संख्या::opSetRange(uint32_t lsb, uint32_t width, अक्षर bit_मूल्य) {
    अगर (lsb < static_cast<uint32_t>(यह->width())) {
        fillBits(static_cast<पूर्णांक>(lsb),
                 static_cast<पूर्णांक>(std::min(width, static_cast<uint32_t>(यह->width()) - lsb)),
                 bit_मूल्य);
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opAssign(स्थिर V3_संख्या& lhs) { वापस opAssignNonXZ(lhs, false); }
V3_संख्या& V3_संख्या::opAssignNonXZ(स्थिर V3_संख्या& lhs, bool ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    अगर (यह != &lhs) {
        अगर (VL_UNLIKELY(lhs.isNull())) {
            m_तथ्य.m_isNull = true;
        } अन्यथा अगर (isString()) {
            अगर (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_तथ्य.str() = lhs.toString();
            } अन्यथा {
                m_तथ्य.str() = lhs.m_तथ्य.str();
            }
        } अन्यथा अगर (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } अन्यथा अगर (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } अन्यथा अगर (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } अन्यथा {
            क्रम (पूर्णांक word = 0; word < शब्दगण(); ++word) {
                स्थिर uint32_t मूल्य = word < lhs.शब्दगण() ? lhs.m_तथ्य.अंक()[word].m_मूल्य
                                                                & ~lhs.m_तथ्य.अंक()[word].m_मूल्य_X
                                                          : 0;
                m_तथ्य.अंक()[word] = {मूल्य, 0};
            }
            opCleanThis();
        }
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opNToI(स्थिर V3_संख्या& lhs) {
    // String to packed number
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    setZero();
    स्थिर string& str = lhs.toString();
    क्रम (माप_प्रकार n = 0; n < str.लंबाई(); ++n) {
        स्थिर अक्षर c = str[str.लंबाई() - 1 - n];
        क्रम (माप_प्रकार cbit = 0; cbit < 8; ++cbit) {
            स्थिर std::bitset<8> sbit{static_cast<अचिन्हित दीर्घ दीर्घ>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    वापस *यह;
}

V3_संख्या& V3_संख्या::opExtendS(स्थिर V3_संख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    स्थिर पूर्णांक copyWidth = std::min(width(), static_cast<पूर्णांक>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    वापस *यह;
}

V3_संख्या& V3_संख्या::opExtendXZ(स्थिर V3_संख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    स्थिर पूर्णांक copyWidth = std::min(width(), static_cast<पूर्णांक>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    वापस *यह;
}

V3_संख्या& V3_संख्या::opClean(स्थिर V3_संख्या& lhs, uint32_t bits) { वापस opSel(lhs, bits - 1, 0); }

व्योम V3_संख्या::opCleanThis(bool warnOnTruncation) {
    // Clean MSB of number
    अंक_ASSERT_LOGIC_ARGS1(*यह);
    स्थिर मूल्य_And_X v = m_तथ्य.अंक()[शब्दगण() - 1];
    स्थिर uint32_t new_मूल्य_Msb = v.m_मूल्य & hiWordMask();
    स्थिर uint32_t new_मूल्य_XMsb = v.m_मूल्य_X & hiWordMask();
    अगर (warnOnTruncation && (new_मूल्य_Msb != v.m_मूल्य || new_मूल्य_XMsb != v.m_मूल्य_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_तथ्य.अंक()[शब्दगण() - 1] = {new_मूल्य_Msb, new_मूल्य_XMsb};
}

V3_संख्या& V3_संख्या::opSel(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& msb, स्थिर V3_संख्या& lsb) {
    अंक_ASSERT_OP_ARGS3(lhs, msb, lsb);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS2(msb, lsb);
    अगर (lsb.isFourState() || msb.isFourState()) वापस setAllBitsX();
    वापस opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_संख्या& V3_संख्या::opSel(स्थिर V3_संख्या& lhs, uint32_t msbval, uint32_t lsbval) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    पूर्णांक copyWidth = 0;
    अगर (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        स्थिर uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<पूर्णांक>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<पूर्णांक>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    वापस *यह;
}

V3_संख्या& V3_संख्या::opSelInto(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& lsb, पूर्णांक width) {
    वापस opSelInto(lhs, lsb.toSInt(), width);
}

V3_संख्या& V3_संख्या::opSelInto(स्थिर V3_संख्या& lhs, पूर्णांक lsbval, पूर्णांक width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    स्थिर पूर्णांक copyWidth = std::max(0, std::min({width, lhs.width(), यह->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    स्थिर पूर्णांक fillWidth
        = std::max(0, std::min(width - copyWidth, यह->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    वापस *यह;
}

//======================================================================
// Ops - Floating point

V3_संख्या& V3_संख्या::opIToRD(स्थिर V3_संख्या& lhs, bool isSigned) {
    // Correct number of zero bits/width matters
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_संख्या noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    द्विगुन d = 0;
    स्थिर bool negate = isSigned && noxz.isNegative();
    अगर (negate) {
        स्थिर V3_संख्या noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    क्रम (पूर्णांक bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        अगर (noxz.bitIs1(bit)) d += exp2(bit);
    }
    अगर (negate) d = -d;
    वापस setDouble(d);
}
V3_संख्या& V3_संख्या::opRToIS(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    स्थिर द्विगुन v = VL_TRUNC(lhs.toDouble());
    स्थिर int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    वापस setLongS(i);
}
V3_संख्या& V3_संख्या::opRToIRoundS(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    स्थिर द्विगुन v = VL_ROUND(lhs.toDouble());
    setZero();
    जोड़ {
        द्विगुन d;
        uint64_t q;
    } u;
    u.d = v;
    (व्योम)u.d;

    स्थिर पूर्णांक exp = static_cast<पूर्णांक>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    स्थिर पूर्णांक lsb = exp - 52;
    स्थिर uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    अगर (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        क्रम (पूर्णांक bit = 0; bit <= 52; ++bit) {
            अगर (mantissa & (1ULL << bit)) {
                स्थिर पूर्णांक outbit = bit + lsb;
                अगर (outbit >= 0) setBit(outbit, 1);
            }
        }
        अगर (v < 0) {
            स्थिर V3_संख्या noSign = *यह;
            opNegate(noSign);
        }
    }
    वापस *यह;
}
V3_संख्या& V3_संख्या::opRealToBits(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    जोड़ {
        द्विगुन m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    वापस setQuad(u.m_v);
}
V3_संख्या& V3_संख्या::opBitsToRealD(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    जोड़ {
        द्विगुन m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    वापस setDouble(u.m_d);
}
V3_संख्या& V3_संख्या::opNegateD(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_DOUBLE_ARGS1(lhs);
    वापस setDouble(-lhs.toDouble());
}
V3_संख्या& V3_संख्या::opAddD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_संख्या& V3_संख्या::opSubD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_संख्या& V3_संख्या::opMulD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_संख्या& V3_संख्या::opDivD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_संख्या& V3_संख्या::opPowD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setDouble(घात(lhs.toDouble(), rhs.toDouble()));
}
V3_संख्या& V3_संख्या::opEqD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_संख्या& V3_संख्या::opNeqD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_संख्या& V3_संख्या::opGtD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_संख्या& V3_संख्या::opGteD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_संख्या& V3_संख्या::opLtD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_संख्या& V3_संख्या::opLteD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_संख्या& V3_संख्या::opConcatN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setString(lhs.toString() + rhs.toString());
}
V3_संख्या& V3_संख्या::opReplN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_STRING_ARGS1(lhs);
    अंक_ASSERT_LOGIC_ARGS1(rhs);
    वापस opReplN(lhs, rhs.toUInt());
}
V3_संख्या& V3_संख्या::opReplN(स्थिर V3_संख्या& lhs, uint32_t rhsval) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().लंबाई() * rhsval);
    क्रम (अचिन्हित बार = 0; बार < rhsval; ++बार) out += lhs.toString();
    वापस setString(out);
}
V3_संख्या& V3_संख्या::opCvtPackString(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    वापस setString(lhs.toString());
}
V3_संख्या& V3_संख्या::opToLowerN(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिर std::string out = VString::downcase(lhs.toString());
    वापस setString(out);
}
V3_संख्या& V3_संख्या::opToUpperN(स्थिर V3_संख्या& lhs) {
    अंक_ASSERT_OP_ARGS1(lhs);
    अंक_ASSERT_STRING_ARGS1(lhs);
    स्थिर std::string out = VString::upcase(lhs.toString());
    वापस setString(out);
}

V3_संख्या& V3_संख्या::opEqN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toString() == rhs.toString());
}
V3_संख्या& V3_संख्या::opNeqN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toString() != rhs.toString());
}
V3_संख्या& V3_संख्या::opGtN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toString() > rhs.toString());
}
V3_संख्या& V3_संख्या::opGteN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toString() >= rhs.toString());
}
V3_संख्या& V3_संख्या::opLtN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toString() < rhs.toString());
}
V3_संख्या& V3_संख्या::opLteN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs) {
    अंक_ASSERT_OP_ARGS2(lhs, rhs);
    अंक_ASSERT_STRING_ARGS2(lhs, rhs);
    वापस setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

व्योम V3_संख्या::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_पंक्ति* स्थिर fl = नया File_पंक्ति{File_पंक्ति::builtInFilename()};
    V3_संख्या अंक{fl, 32, 0};
    अंक.selfTestThis();
}

व्योम V3_संख्या::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(0, 0), true);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(1e19, 1e19), true);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(9, 0.0001), false);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(1, 1 + std::numeric_limits<द्विगुन>::epsilon()), true);
    UASSERT_SELFTEST(V3_संख्या::epsilonEqual(0.009, 0.00899999999999999931998839741709), true);

    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(0), true);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(1), true);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(-1), true);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(1.0001), false);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(0.9999), false);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(-1.0001), false);
    UASSERT_SELFTEST(V3_संख्या::epsilonIntegral(-0.9999), false);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
