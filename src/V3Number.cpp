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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Number.h"

#અન્તર્ભાવ "V3File.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cerrno>
#અન્તર્ભાવ <cmath>
#અન્તર્ભાવ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr પૂર્ણાંક MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define અંક_ASSERT_OP_ARGS1(arg1) \
    UASSERT((આ != &(arg1)), "Number operation called with same source and dest")
#define અંક_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((આ != &(arg1) && આ != &(arg2)), \
            "Number operation called with same source and dest")
#define અંક_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((આ != &(arg1) && આ != &(arg2) && આ != &(arg3)), \
            "Number operation called with same source and dest")
#define અંક_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((આ != &(arg1) && આ != &(arg2) && આ != &(arg3) && આ != &(arg4)), \
            "Number operation called with same source and dest")

#define અંક_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_સંખ્યા_Data::V3_સંખ્યા_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define અંક_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    અંક_ASSERT_LOGIC_ARGS1(arg1); \
    અંક_ASSERT_LOGIC_ARGS1(arg2)

#define અંક_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    અંક_ASSERT_LOGIC_ARGS1(arg1); \
    અંક_ASSERT_LOGIC_ARGS1(arg2); \
    અંક_ASSERT_LOGIC_ARGS1(arg3); \
    અંક_ASSERT_LOGIC_ARGS1(arg4)

#define અંક_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define અંક_ASSERT_STRING_ARGS2(arg1, arg2) \
    અંક_ASSERT_STRING_ARGS1(arg1); \
    અંક_ASSERT_STRING_ARGS1(arg2)

#define અંક_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define અંક_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    અંક_ASSERT_DOUBLE_ARGS1(arg1); \
    અંક_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

શૂન્ય V3_સંખ્યા::v3errorEnd(અચલ std::ostringstream& str) અચલ VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    જો (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } નહીંતર જો (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } નહીંતર {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

શૂન્ય V3_સંખ્યા::v3errorEndFatal(અચલ std::ostringstream& str) અચલ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    પ્રતિજ્ઞા(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_સંખ્યા::V3_સંખ્યા(Ast_ગાંઠ* nodep, VerilogStringLiteral, અચલ string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    જો (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } નહીંતર {
        init(nodep, std::max<પૂર્ણાંક>(str.length() * 8, 1));
        માટે (unsigned pos = 0; pos < str.length(); ++pos) {
            અચલ પૂર્ણાંક topos = str.length() - 1 - pos;
            કિંમત_And_X& v = m_data.અંક()[topos / 4];
            માટે (પૂર્ણાંક bit = 0; bit < 8; ++bit) {
                જો (str[pos] & (1UL << bit)) v.m_કિંમત |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = સાચું;
    opCleanThis(સાચું);
}

V3_સંખ્યા::V3_સંખ્યા(Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ_DType* nodedtypep) {
    જો (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } નહીંતર જો (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } નહીંતર જો (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } નહીંતર {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

શૂન્ય V3_સંખ્યા::create(અચલ અક્ષર* sourcep) {
    m_data.setLogic();
    અચલ અક્ષર* કિંમત_startp = sourcep;
    માટે (અચલ અક્ષર* cp = sourcep; *cp; ++cp) {
        જો (*cp == '\'') {
            કિંમત_startp = cp + 1;
            તોડો;
        }
    }

    તાર્કિક userSized = ખોટું;
    તાર્કિક unbased = ખોટું;
    અક્ષર base = '\0';
    જો (કિંમત_startp != sourcep) {  // Has a '
        string widthn;
        અચલ અક્ષર* cp = sourcep;
        માટે (; *cp; ++cp) {
            જો (*cp == '\'') {
                ++cp;
                તોડો;
            }
            જો (*cp != '_') widthn += *cp;
        }
        જ્યારે (*cp == '_') cp++;
        જો (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(સાચું);
        }
        જો (*cp) {
            base = *cp;
            ++cp;
        }
        કિંમત_startp = cp;

        userSized = widthn.length() != 0;
        જો (std::atoi(widthn.c_str())) {
            જો (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_અંક_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_અંક_Width(), સાચું);
            } નહીંતર {
                width(std::atoi(widthn.c_str()), સાચું);
            }
        }
    } નહીંતર {
        unbased = સાચું;
        base = 'd';
    }

    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    જો (base == '0') {
        width(1, ખોટું);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = સાચું;
    } નહીંતર જો (base == '1') {
        width(1, ખોટું);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = સાચું;
    } નહીંતર જો (std::tolower(base) == 'z') {
        width(1, ખોટું);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = સાચું;
    } નહીંતર જો (std::tolower(base) == 'x') {
        width(1, ખોટું);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = સાચું;
    }
    // Otherwise...
    નહીંતર જો (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        અચલ પૂર્ણાંક widthBound = std::max<પૂર્ણાંક>(32, std::strlen(કિંમત_startp) * 4);
        width(widthBound, ખોટું);  // Will change width below
        જો (unbased) isSigned(સાચું);  // Also says the spec.
    }

    // Ignore leading blanks
    જ્યારે (*કિંમત_startp == '_' || std::isspace(*કિંમત_startp)) ++કિંમત_startp;
    જો (!*કિંમત_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    જો (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    પૂર્ણાંક obit = 0;  // Start at LSB
    પૂર્ણાંક base_align = 1;
    જો (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        જ્યારે (*કિંમત_startp == '_' || *કિંમત_startp == '0') ++કિંમત_startp;
        // Convert decimal number to hex
        પૂર્ણાંક olen = 0;
        uint32_t val = 0;
        પૂર્ણાંક got_x = 0;
        પૂર્ણાંક got_z = 0;
        પૂર્ણાંક got_01 = 0;
        તાર્કિક warnedTooMany = ખોટું;
        માટે (અચલ અક્ષર* cp = કિંમત_startp; *cp; ++cp) {
            પસંદગી (std::tolower(*cp)) {
            સ્થિતિ '0':  // FALLTHRU
            સ્થિતિ '1':  // FALLTHRU
            સ્થિતિ '2':  // FALLTHRU
            સ્થિતિ '3':  // FALLTHRU
            સ્થિતિ '4':  // FALLTHRU
            સ્થિતિ '5':  // FALLTHRU
            સ્થિતિ '6':  // FALLTHRU
            સ્થિતિ '7':  // FALLTHRU
            સ્થિતિ '8':  // FALLTHRU
            સ્થિતિ '9': {
                જો (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.અંક()[0].m_કિંમત = val;
                } નહીંતર {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_સંખ્યા product(આ, width() + 4);  // +4 for overflow detection
                    અચલ V3_સંખ્યા ten(આ, width() + 4, 10);
                    અચલ V3_સંખ્યા addend(આ, width(), (*cp - '0'));
                    product.opMul(*આ, ten);
                    opAdd(product, addend);
                    જો (!warnedTooMany && product.bits_કિંમત(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = સાચું;
                    }
                }
                ++olen;
                got_01 = 1;
                તોડો;
            }
            સ્થિતિ 'z':
            સ્થિતિ '?': {
                got_z = 1;
                જો (!userSized) width(32, ખોટું);
                setAllBitsZ();
                તોડો;
            }
            સ્થિતિ 'x': {
                got_x = 1;
                જો (!userSized) width(32, ખોટું);
                setAllBitsX();
                તોડો;
            }
            સ્થિતિ '_': તોડો;
            મૂળભૂત: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                તોડો;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        જો ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << કિંમત_startp);
        }
    } નહીંતર {
        // Convert bin/octal number to hex
        માટે (અચલ અક્ષર* cp = કિંમત_startp + std::strlen(કિંમત_startp) - 1; cp >= કિંમત_startp;
             cp--) {
            જો (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                તોડો;
            }
            પસંદગી (std::tolower(base)) {
            સ્થિતિ 'b': {
                base_align = 1;
                પસંદગી (std::tolower(*cp)) {
                સ્થિતિ '0': setBit(obit++, 0); તોડો;
                સ્થિતિ '1': setBit(obit++, 1); તોડો;
                સ્થિતિ 'z':
                સ્થિતિ '?': setBit(obit++, 'z'); તોડો;
                સ્થિતિ 'x': setBit(obit++, 'x'); તોડો;
                સ્થિતિ '_': તોડો;
                મૂળભૂત: v3error("Illegal character in binary constant: " << *cp);
                }
                તોડો;
            }

            સ્થિતિ 'o':
            સ્થિતિ 'c': {
                base_align = 3;
                પસંદગી (std::tolower(*cp)) {  // clang-format off
                સ્થિતિ '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  તોડો;
                સ્થિતિ '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  તોડો;
                સ્થિતિ '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  તોડો;
                સ્થિતિ '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  તોડો;
                સ્થિતિ '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  તોડો;
                સ્થિતિ '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  તોડો;
                સ્થિતિ '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  તોડો;
                સ્થિતિ '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  તોડો;
                સ્થિતિ 'z':
                સ્થિતિ '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); તોડો;
                સ્થિતિ 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); તોડો;
                સ્થિતિ '_': તોડો;
                // clang-format on
                મૂળભૂત: v3error("Illegal character in octal constant");
                }
                તોડો;
            }

            સ્થિતિ 'h':  // FALLTHRU
            સ્થિતિ 'x': {
                base_align = 4;
                પસંદગી (std::tolower(*cp)) {  // clang-format off
                સ્થિતિ '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); તોડો;
                સ્થિતિ '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); તોડો;
                સ્થિતિ '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); તોડો;
                સ્થિતિ '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); તોડો;
                સ્થિતિ '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); તોડો;
                સ્થિતિ '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); તોડો;
                સ્થિતિ '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); તોડો;
                સ્થિતિ '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); તોડો;
                સ્થિતિ '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); તોડો;
                સ્થિતિ '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); તોડો;
                સ્થિતિ 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); તોડો;
                સ્થિતિ 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); તોડો;
                સ્થિતિ 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); તોડો;
                સ્થિતિ 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); તોડો;
                સ્થિતિ 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); તોડો;
                સ્થિતિ 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); તોડો;
                સ્થિતિ 'z':
                સ્થિતિ '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); તોડો;
                સ્થિતિ 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); તોડો;
                    // clang-format on
                સ્થિતિ '_': તોડો;
                મૂળભૂત:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    તોડો;
                }  // LCOV_EXCL_STOP
                તોડો;
            }
            મૂળભૂત:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                તોડો;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    જો (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), ખોટું);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    જ્યારે (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(સાચું);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

શૂન્ય V3_સંખ્યા::warnTooMany(અચલ string& કિંમત) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << કિંમત << "'");
}

શૂન્ય V3_સંખ્યા::nodep(Ast_ગાંઠ* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    જો (!nodep) પાછા;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

તાર્કિક V3_સંખ્યા::epsilonEqual(દ્વિગુણ a, દ્વિગુણ b) {
    પાછા std::fabs(a - b)
           <= (std::numeric_limits<દ્વિગુણ>::epsilon() * std::max(1.0, std::max(a, b)));
}

તાર્કિક V3_સંખ્યા::epsilonIntegral(દ્વિગુણ a) {
    અચલ દ્વિગુણ dint = static_cast<દ્વિગુણ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    પાછા epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

પૂર્ણાંક V3_સંખ્યા::log2b(uint32_t અંક) {
    // See also opCLog2
    માટે (પૂર્ણાંક bit = 31; bit > 0; bit--) {
        જો (અંક & (1ULL << bit)) પાછા bit;
    }
    પાછા 0;
}

પૂર્ણાંક V3_સંખ્યા::log2bQuad(uint64_t અંક) {
    જો (અંક >> 32ULL) પાછા 32 + log2b(અંક >> 32ULL);
    પાછા log2b(અંક);
}

//======================================================================
// Setters

V3_સંખ્યા& V3_સંખ્યા::setZero() {
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, 0};
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setQuad(uint64_t કિંમત) {
    setZero();
    m_data.અંક()[0].m_કિંમત = કિંમત & 0xffffffffULL;
    જો (width() > 32) m_data.અંક()[1].m_કિંમત = (કિંમત >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setLong(uint32_t કિંમત) {
    setZero();
    m_data.અંક()[0].m_કિંમત = કિંમત;
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setLongS(int32_t કિંમત) {
    setZero();
    સંઘ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = કિંમત;
    (શૂન્ય)u.s;
    m_data.અંક()[0].m_કિંમત = u.u;
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setDouble(દ્વિગુણ કિંમત) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    સંઘ {
        દ્વિગુણ d;
        uint32_t u[2];
    } u;
    u.d = કિંમત;
    (શૂન્ય)u.d;
    માટે (પૂર્ણાંક i = 2; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, 0};
    m_data.અંક()[0].m_કિંમત = u.u[0];
    m_data.અંક()[1].m_કિંમત = u.u[1];
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setSingleBits(અક્ષર કિંમત) {
    માટે (પૂર્ણાંક i = 1 /*upper*/; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, 0};
    m_data.અંક()[0] = {(કિંમત == '1' || કિંમત == 'x' || કિંમત == 1 || કિંમત == 3),
                       (કિંમત == 'z' || કિંમત == 'x' || કિંમત == 2 || કિંમત == 3)};
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::setAllBits0() {
    setZero();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setAllBits1() {
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) m_data.અંક()[i] = {~0U, 0};
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) m_data.અંક()[i] = {~0U, ~0U};
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setAllBitsZ() {
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, ~0U};
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::setAllBitsXRemoved() {
    જો (!v3Global.constRemoveXs()) {
        પાછા setAllBitsX();
    } નહીંતર {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        જો (v3Global.opt.xAssign() == "1") {
            પાછા setAllBits1();
        } નહીંતર {
            પાછા setAllBits0();
        }
    }
}
V3_સંખ્યા& V3_સંખ્યા::setValue1() {
    m_data.અંક()[0] = {1, 0};
    માટે (પૂર્ણાંક i = 1; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, 0};
    પાછા *આ;
}

શૂન્ય V3_સંખ્યા::setBitX0(પૂર્ણાંક bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_સંખ્યા& V3_સંખ્યા::setMask(પૂર્ણાંક nbits, પૂર્ણાંક lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    પાછા *આ;
}

//======================================================================
// ACCESSORS - as strings

string V3_સંખ્યા::ascii(તાર્કિક prefixed, તાર્કિક cleanVerilog) અચલ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    જો (is1Step()) પાછા "1step";
    જો (isNull()) પાછા "null";
    std::ostringstream out;
    જો (isDouble()) {
        out.precision(17);
        જો (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } નહીંતર {
            out << toDouble();
            જો (toDouble() == floor(toDouble())) out << ".0";
        }
        પાછા out.str();
    } નહીંતર જો (isString()) {
        પાછા '"' + toString() + '"';
    } નહીંતર {
        જો (VL_UNCOVERABLE((m_data.અંક()[શબ્દો() - 1].m_કિંમત | m_data.અંક()[શબ્દો() - 1].m_કિંમત_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    જો (prefixed) {
        જો (sized()) {
            out << width() << "'";
        } નહીંતર જો (autoExtend() && width() == 1) {
            out << "'";
            જો (bitIs0(0)) {
                out << '0';
            } નહીંતર જો (bitIs1(0)) {
                out << '1';
            } નહીંતર જો (bitIsZ(0)) {
                out << 'z';
            } નહીંતર {
                out << 'x';
            }
            પાછા out.str();
        } નહીંતર {
            જો (cleanVerilog) {
                out << "'";
            } નહીંતર {
                out << "?" << width() << "?";
            }
        }
        જો (isSigned()) out << "s";
    }

    અચલ તાર્કિક દ્વિઅંકી = (isFourState()
#ifdef V3_સંખ્યા_ASCII_દ્વિઅંકી
                         // cppcheck-suppress konwnConditionTrueFalse
                         || સાચું
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    જો (દ્વિઅંકી) {
        out << "b";
        out << displayed("%0b");
    } નહીંતર {
        જો (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    જો (isNull()) {
        જો (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } નહીંતર {
            out << " [null]";
        }
    }
    પાછા out.str();
}

તાર્કિક V3_સંખ્યા::displayedFmtHasArg(અક્ષર format, તાર્કિક isScan) {
    // Is this a format letter that takes an argument in runtime?
    (શૂન્ય)isScan;
    પસંદગી (std::tolower(format)) {
    સ્થિતિ 'b': પાછા સાચું;  //દ્વિઅંકી
    સ્થિતિ 'c': પાછા સાચું;  // Character
    સ્થિતિ 'd': પાછા સાચું;  // Decimal
    સ્થિતિ 'e': પાછા સાચું;  // Floating
    સ્થિતિ 'f': પાછા સાચું;  // Floating
    સ્થિતિ 'g': પાછા સાચું;  // Floating
    સ્થિતિ 'h': પાછા સાચું;  // Hex
    સ્થિતિ 'o': પાછા સાચું;  // Octal
    સ્થિતિ 'p': પાછા સાચું;  // Pattern
    સ્થિતિ 's': પાછા સાચું;  // String
    સ્થિતિ 't': પાછા સાચું;  // Time
    સ્થિતિ 'u': પાછા સાચું;  // Packed 2-state
    સ્થિતિ 'v': પાછા સાચું;  // Strength
    સ્થિતિ 'x': પાછા સાચું;  // Hex
    સ્થિતિ 'z': પાછા સાચું;  // Packed 4-state
    સ્થિતિ '?': પાછા સાચું;  // Internal: Compute format in V3Width based on data type
    મૂળભૂત: પાછા ખોટું;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_સંખ્યા::displayPad(size_t fmtsize, અક્ષર pad, તાર્કિક ડાબું, અચલ string& in) VL_PURE {
    string padding;
    જો (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    પાછા ડાબું ? (in + padding) : (padding + in);
}

string V3_સંખ્યા::displayedEnumName(અચલ AstEnumItem* itemp) {
    અચલ string name = VIdProtect::protect(itemp->prettyName());
    પાછા VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned અક્ષર>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_સંખ્યા::displayedEnumName(અચલ AstEnumDType* dtypep) અચલ VL_MT_STABLE {
    V3_સંખ્યા કિંમત{આ, dtypep->width()};
    કિંમત.opAssign(*આ);
    માટે (અચલ AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        અચલ AstConst* અચલ constp = VN_AS(itemp->valuep(), Const);
        જો (!constp->અંક().isAnyXZ() && કિંમત.isCaseEq(constp->અંક()))
            પાછા displayedEnumName(itemp);
    }
    પાછા "";
}

string V3_સંખ્યા::displayedEnum(અચલ AstSFormatArg* argp,
                               અચલ string& vformat) અચલ VL_MT_STABLE {
    અચલ AstEnumDType* અચલ dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_સંખ્યા કિંમત{આ, dtypep->width()};
    કિંમત.opAssign(*આ);
    અચલ VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    અચલ અક્ષર fmt = std::tolower(vformat.back());
    જો (fmt != 'p' && fmt != 's') પાછા કિંમત.displayed(argp, vformat, numericAttr);
    string name = કિંમત.displayedEnumName(dtypep);
    જો (name.empty()) name = કિંમત.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_સંખ્યા name_અંક{આ};
    name_અંક.setString(name);
    પાછા name_અંક.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_સંખ્યા::displayed(અચલ Ast_ગાંઠ* nodep, અચલ string& vformat,
                           અચલ VFormatAttr& formatAttr) અચલ VL_MT_STABLE {
    પાછા displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_સંખ્યા::displayed(File_લીટી* fl, અચલ string& vformat,
                           અચલ VFormatAttr& formatAttr) અચલ VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *આ);
    ++pos;
    તાર્કિક ડાબું = ખોટું;
    જો (pos[0] == '-') {
        ડાબું = સાચું;
        ++pos;
    }
    string fmtsize;
    માટે (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    અક્ષર fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    જો (formatAttr.isComplex()) {
        જો (fmt != 'p') fmt = 's';  // override
    } નહીંતર જો (formatAttr.isString()) {
        જો (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_સંખ્યા strwide{fl, static_cast<પૂર્ણાંક>(toString().length()) * 8, 0};
            strwide.opNToI(*આ);
            પાછા strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        જો (fmt != 'p') fmt = 's';  // override
    } નહીંતર જો (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        જો (fmt == 'p') {
            જો (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } નહીંતર {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } નહીંતર જો (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        જો (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_સંખ્યા nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*આ);
            પાછા nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    પસંદગી (fmt) {
    સ્થિતિ 'c': {
        જો (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        અચલ unsigned પૂર્ણાંક v = bits_કિંમત(0, 8);
        str.ધકેલો_back(static_cast<અક્ષર>(v));
        પાછા str;
    }
    સ્થિતિ 'e':  // FALLTHRU
    સ્થિતિ 'f':  // FALLTHRU
    સ્થિતિ 'g': {
        અચલ દ્વિગુણ n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        અક્ષર tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (શૂન્ય)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        પાછા tmp;
    }
    સ્થિતિ 's': {
        જો (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } નહીંતર {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            પૂર્ણાંક bit = width() - 1;
            તાર્કિક start = સાચું;
            જ્યારે ((bit % 8) != 7) ++bit;
            માટે (; bit >= 0; bit -= 8) {
                અચલ પૂર્ણાંક v = bits_કિંમત(bit - 7, 8);
                જો (!start || v) {
                    str += static_cast<અક્ષર>((v == 0) ? ' ' : v);
                    start = ખોટું;  // Drop leading 0s
                } નહીંતર {
                    જો (fmtsize != "0") str += ' ';
                }
            }
        }
        અચલ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ડાબું, str);
        પાછા str;
    }
    સ્થિતિ 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        જો (formatAttr.isDouble()) {
            અચલ દ્વિગુણ n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            અક્ષર tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (શૂન્ય)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            પાછા tmp;
        }
        જો (formatAttr.isString())
            પાછા '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        જો (formatAttr.isComplex()) પાછા toString();
        પાછા "%p";
    }
    સ્થિતિ 'b':  // FALLTHRU
    સ્થિતિ 'o':  // FALLTHRU
    સ્થિતિ 'h':  // FALLTHRU
    સ્થિતિ 'x': {
        પૂર્ણાંક bit = width() - 1;
        જો (ડાબું || !fmtsize.empty()) {
            જ્યારે (bit && bitIs0(bit)) --bit;
        }
        પસંદગી (fmt) {
        સ્થિતિ 'b': {
            માટે (; bit >= 0; --bit) {
                જો (bitIs0(bit)) {
                    str += '0';
                } નહીંતર જો (bitIs1(bit)) {
                    str += '1';
                } નહીંતર જો (bitIsZ(bit)) {
                    str += 'z';
                } નહીંતર {
                    str += 'x';
                }
            }
            તોડો;
        }
        સ્થિતિ 'o': {
            જ્યારે ((bit % 3) != 2) ++bit;
            માટે (; bit > 0; bit -= 3) {
                અચલ પૂર્ણાંક અંક_X = ગણતરી_X(bit - 2, 3);
                અચલ પૂર્ણાંક અંક_Z = ગણતરી_Z(bit - 2, 3);
                જો (અંક_X == 3 || અંક_X == width() - (bit - 2)) {
                    str += 'x';
                    ચાલુ;
                }
                જો (અંક_Z == 3 || અંક_Z == width() - (bit - 2)) {
                    str += 'z';
                    ચાલુ;
                }
                જો (અંક_X > 0) {
                    str += 'X';
                    ચાલુ;
                }
                જો (અંક_Z > 0) {
                    str += 'Z';
                    ચાલુ;
                }
                અચલ પૂર્ણાંક v = bits_કિંમત(bit - 2, 3);
                str += static_cast<અક્ષર>('0' + v);
            }
            તોડો;
        }
        મૂળભૂત: {  // h/x
            જ્યારે ((bit % 4) != 3) ++bit;
            માટે (; bit > 0; bit -= 4) {
                અચલ પૂર્ણાંક અંક_X = ગણતરી_X(bit - 3, 4);
                અચલ પૂર્ણાંક અંક_Z = ગણતરી_Z(bit - 3, 4);
                જો (અંક_X == 4 || અંક_X == width() - (bit - 3)) {
                    str += 'x';
                    ચાલુ;
                }
                જો (અંક_Z == 4 || અંક_Z == width() - (bit - 3)) {
                    str += 'z';
                    ચાલુ;
                }
                જો (અંક_X > 0) {
                    str += 'X';
                    ચાલુ;
                }
                જો (અંક_Z > 0) {
                    str += 'Z';
                    ચાલુ;
                }
                અચલ પૂર્ણાંક v = bits_કિંમત(bit - 3, 4);
                જો (v >= 10) {
                    str += static_cast<અક્ષર>('a' + v - 10);
                } નહીંતર {
                    str += static_cast<અક્ષર>('0' + v);
                }
            }
            તોડો;
        }
        }  // switch
        અચલ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ડાબું ? ' ' : '0'), ડાબું, str);
        પાછા str;
    }  // case b/d/x/o
    સ્થિતિ 't':  // Time
    સ્થિતિ 'd': {  // Unsigned decimal
        અચલ તાર્કિક issigned = formatAttr.isSigned();
        જો (fmtsize == "" && !ડાબું) {
            અચલ દ્વિગુણ mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            દ્વિગુણ dchars = mantissabits / 3.321928094887362 + 1.0;
            જો (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<પૂર્ણાંક>(dchars));
        }
        તાર્કિક hasXZ = ખોટું;
        જો (isAllX()) {
            str = "x";
            hasXZ = સાચું;
        } નહીંતર જો (isAllZ()) {
            str = "z";
            hasXZ = સાચું;
        } નહીંતર જો (isAnyX()) {
            str = "X";
            hasXZ = સાચું;
        } નહીંતર જો (isAnyZ()) {
            str = "Z";
            hasXZ = સાચું;
        }
        જો (!hasXZ) {
            જો (issigned) {
                જો (width() > 64) {
                    str = toDecimalS();
                } નહીંતર {
                    str = cvtToStr(toSQuad());
                }
            } નહીંતર {
                જો (width() > 64) {
                    str = toDecimalU();
                } નહીંતર {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        અચલ તાર્કિક zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !ડાબું;
        // fmtsize might have changed since we parsed the %fmtsize
        અચલ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ડાબું, str);
        પાછા str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    સ્થિતિ 'u': {  // Packed 2-state
        માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
            અચલ uint32_t v = m_data.અંક()[i].m_કિંમત;
            str += static_cast<અક્ષર>((v >> 0) & 0xff);
            str += static_cast<અક્ષર>((v >> 8) & 0xff);
            str += static_cast<અક્ષર>((v >> 16) & 0xff);
            str += static_cast<અક્ષર>((v >> 24) & 0xff);
        }
        પાછા str;
    }
    સ્થિતિ 'z': {  // Packed 4-state
        માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
            અચલ કિંમત_And_X v = m_data.અંક()[i];
            str += static_cast<અક્ષર>((v.m_કિંમત >> 0) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત >> 8) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત >> 16) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત >> 24) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત_X >> 0) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત_X >> 8) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત_X >> 16) & 0xff);
            str += static_cast<અક્ષર>((v.m_કિંમત_X >> 24) & 0xff);
        }
        પાછા str;
    }
    સ્થિતિ 'v': {  // Strength
        પૂર્ણાંક bit = width() - 1;
        માટે (; bit >= 0; bit--) {
            જો (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            નહીંતર જો (bitIs1(bit)) {
                str += "St1 ";
            } નહીંતર જો (bitIsZ(bit)) {
                str += "StZ ";
            } નહીંતર {
                str += "StX";
            }
        }
        પાછા str;
    }
    મૂળભૂત: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_સંખ્યા::emitC() અચલ VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    અક્ષર sbuf[bufsize];
    string result;
    જો (isNull()) {
        પાછા "VlNull{}";
    } નહીંતર જો (isDouble()) {
        અચલ દ્વિગુણ dnum = toDouble();
        જો (std::isinf(dnum)) {
            જો (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } નહીંતર જો (std::isnan(dnum)) {
            જો (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } નહીંતર {
            અચલ અક્ષર* અચલ fmt = (static_cast<પૂર્ણાંક>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (શૂન્ય)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            પાછા sbuf;
        }
    } નહીંતર જો (isString()) {
        અચલ string strg = toString();
        અચલ string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        પાછા '"' + quoted + "\"s";
    } નહીંતર જો (શબ્દો() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(શબ્દો()) + ">{{";
        જો (શબ્દો() > 4) result += '\n';
        માટે (પૂર્ણાંક n = 0; n < શબ્દો(); ++n) {
            જો (n) result += ((n % 4) ? ", " : ",\n");
            (શૂન્ય)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        જો (શબ્દો() > 4) result += '\n';
        result += "}}";
    } નહીંતર જો (શબ્દો() == 2) {  // Quad
        અચલ uint64_t qnum = static_cast<uint64_t>(toUQuad());
        અચલ અક્ષર* અચલ fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (શૂન્ય)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        પાછા sbuf;
    } નહીંતર {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        અચલ uint32_t unum = toUInt();
        અચલ અક્ષર* અચલ fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (શૂન્ય)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        પાછા sbuf;
    }
    પાછા result;
}

string V3_સંખ્યા::toDecimalS() અચલ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    જો (isNegative()) {
        V3_સંખ્યા lhsNoSign = *આ;
        lhsNoSign.opNegate(*આ);
        પાછા "-"s + lhsNoSign.toDecimalU();
    } નહીંતર {
        પાછા toDecimalU();
    }
}

string V3_સંખ્યા::toDecimalU() અચલ VL_MT_STABLE {
    અચલ પૂર્ણાંક maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_સંખ્યા bcd(આ, maxdecwidth + 4);
    V3_સંખ્યા tmp(આ, maxdecwidth + 4);
    V3_સંખ્યા tmp2(આ, maxdecwidth + 4);

    પૂર્ણાંક from_bit = width() - 1;
    // Skip all leading zeros
    માટે (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    માટે (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        માટે (પૂર્ણાંક nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            જો (bcd.bits_કિંમત(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_સંખ્યા(આ, 32, 1));
        // bcd[0] = this[from_bit]
        જો (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    પૂર્ણાંક lsb = (maxdecwidth - 1) & ~3;
    માટે (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        જો (bcd.bits_કિંમત(lsb, 4)) તોડો;
    }
    માટે (; lsb >= 0; lsb -= 4) {
        output += static_cast<અક્ષર>('0' + bcd.bits_કિંમત(lsb, 4));  // 0..9
    }
    પાછા output;
}

string V3_સંખ્યા::toRealString() અચલ VL_MT_STABLE {
    અચલ દ્વિગુણ કિંમત = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << કિંમત;
    string result = out.str();
    જો (std::strtod(result.c_str(), nullptr) != કિંમત) {
        out.str("");
        out.precision(17);
        out << કિંમત;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    જો (result.find_first_of(".en") == string::npos) result += ".0";
    પાછા result;
}

//======================================================================
// ACCESSORS - as numbers

તાર્કિક V3_સંખ્યા::fitsInUInt() અચલ VL_MT_SAFE {
    જો (isFourState()) પાછા ખોટું;
    // We allow wide numbers that represent values <= 32 bits
    માટે (પૂર્ણાંક i = 1; i < શબ્દો(); ++i) {
        જો (m_data.અંક()[i].m_કિંમત) પાછા ખોટું;
    }
    પાછા સાચું;
}
uint32_t V3_સંખ્યા::toUInt() અચલ VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *આ);
    // We allow wide numbers that represent values <= 32 bits
    જો (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *આ);
    }
    પાછા m_data.અંક()[0].m_કિંમત;
}

દ્વિગુણ V3_સંખ્યા::toDouble() અચલ VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    સંઘ {
        દ્વિગુણ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.અંક()[0].m_કિંમત;
    u.u[1] = m_data.અંક()[1].m_કિંમત;
    પાછા u.d;
}

int32_t V3_સંખ્યા::toSInt() અચલ VL_MT_SAFE {
    જો (isSigned()) {
        અચલ uint32_t v = toUInt();
        અચલ uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        અચલ uint32_t extended = v | signExtend;
        પાછા static_cast<int32_t>(extended);
    } નહીંતર {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        પાછા static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_સંખ્યા::toUQuad() અચલ VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *આ);
    // We allow wide numbers that represent values <= 64 bits
    જો (isDouble()) પાછા static_cast<uint64_t>(toDouble());
    માટે (પૂર્ણાંક i = 2; i < શબ્દો(); ++i) {
        જો (m_data.અંક()[i].m_કિંમત) {
            v3error("Value too wide for 64-bits expected in this context " << *આ);
            તોડો;
        }
    }
    જો (width() <= 32) પાછા (static_cast<uint64_t>(toUInt()));
    પાછા ((static_cast<uint64_t>(m_data.અંક()[1].m_કિંમત) << 32ULL)
            | (static_cast<uint64_t>(m_data.અંક()[0].m_કિંમત)));
}

int64_t V3_સંખ્યા::toSQuad() અચલ {
    જો (isDouble()) પાછા static_cast<int64_t>(toDouble());
    અચલ uint64_t v = toUQuad();
    અચલ uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    અચલ uint64_t extended = v | signExtend;
    પાછા static_cast<int64_t>(extended);
}

string V3_સંખ્યા::toString() અચલ {
    UASSERT(!isFourState(), "toString with 4-state " << *આ);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    જો (isString()) પાછા m_data.str();
    પૂર્ણાંક bit = width() - 1;
    જ્યારે ((bit % 8) != 7) ++bit;
    std::string str;
    માટે (; bit >= 0; bit -= 8) {
        અચલ પૂર્ણાંક v = bits_કિંમત(bit - 7, 8);
        જો (v) str += static_cast<અક્ષર>(v);
    }
    પાછા str;
}

uint32_t V3_સંખ્યા::edataWord(પૂર્ણાંક eword) અચલ {
    UASSERT(!isFourState(), "edataWord with 4-state " << *આ);
    પાછા m_data.અંક()[eword].m_કિંમત;
}

uint8_t V3_સંખ્યા::dataByte(પૂર્ણાંક byte) અચલ {
    પાછા (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

તાર્કિક V3_સંખ્યા::isAllZ() અચલ VL_MT_SAFE {
    જો (isDouble() || isString()) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        અચલ uint32_t mask = i == શબ્દો() - 1 ? hiWordMask() : ~0U;
        જો (((~v.m_કિંમત & v.m_કિંમત_X) & mask) != mask) પાછા ખોટું;
    }
    પાછા સાચું;
}
તાર્કિક V3_સંખ્યા::isAllX() અચલ VL_MT_SAFE {
    જો (isDouble() || isString()) પાછા ખોટું;
    uint32_t mask = hiWordMask();
    માટે (પૂર્ણાંક i = શબ્દો() - 1; i >= 0; --i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        જો ((v.m_કિંમત & v.m_કિંમત_X) ^ mask) પાછા ખોટું;
        mask = ~0U;
    }
    પાછા સાચું;
}
તાર્કિક V3_સંખ્યા::isEqZero() અચલ VL_MT_SAFE {
    જો (isString()) પાછા m_data.str().empty();
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        જો (v.m_કિંમત || v.m_કિંમત_X) પાછા ખોટું;
    }
    પાછા સાચું;
}
તાર્કિક V3_સંખ્યા::isNeqZero() અચલ {
    જો (isString()) પાછા !m_data.str().empty();
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        જો (v.m_કિંમત & ~v.m_કિંમત_X) પાછા સાચું;
    }
    પાછા ખોટું;
}
તાર્કિક V3_સંખ્યા::isBitsZero(પૂર્ણાંક msb, પૂર્ણાંક lsb) અચલ {
    માટે (પૂર્ણાંક i = lsb; i <= msb; ++i) {
        જો (VL_UNLIKELY(!bitIs0(i))) પાછા ખોટું;
    }
    પાછા સાચું;
}
તાર્કિક V3_સંખ્યા::isEqOne() અચલ {
    જો (m_data.અંક()[0].m_કિંમત != 1 || m_data.અંક()[0].m_કિંમત_X) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 1; i < શબ્દો(); ++i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        જો (v.m_કિંમત || v.m_કિંમત_X) પાછા ખોટું;
    }
    પાછા સાચું;
}
તાર્કિક V3_સંખ્યા::isEqAllOnes(પૂર્ણાંક optwidth) અચલ {
    // Correct number of zero bits/width matters
    જો (!optwidth) optwidth = width();
    માટે (પૂર્ણાંક bit = 0; bit < optwidth; ++bit) {
        જો (!bitIs1(bit)) પાછા ખોટું;
    }
    પાછા સાચું;
}
તાર્કિક V3_સંખ્યા::isFourState() અચલ VL_MT_SAFE {
    જો (isDouble() || isString()) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        જો (m_data.અંક()[i].m_કિંમત_X) પાછા સાચું;
    }
    પાછા ખોટું;
}
તાર્કિક V3_સંખ્યા::isAnyX() અચલ VL_MT_SAFE {
    જો (isDouble() || isString()) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        જો (v.m_કિંમત & v.m_કિંમત_X) પાછા સાચું;
    }
    પાછા ખોટું;
}
તાર્કિક V3_સંખ્યા::isAnyXZ() અચલ {
    જો (isDouble() || isString()) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        જો (m_data.અંક()[i].m_કિંમત_X) પાછા સાચું;
    }
    પાછા ખોટું;
}
તાર્કિક V3_સંખ્યા::isAnyZ() અચલ VL_MT_SAFE {
    જો (isDouble() || isString()) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        અચલ કિંમત_And_X v = m_data.અંક()[i];
        જો (~v.m_કિંમત & v.m_કિંમત_X) પાછા સાચું;
    }
    પાછા ખોટું;
}
તાર્કિક V3_સંખ્યા::isLtXZ(અચલ V3_સંખ્યા& rhs) અચલ {
    // Include X/Z in comparisons for sort ordering
    જો (isString()) પાછા toString() < rhs.toString();
    જો (isDouble()) પાછા toDouble() < rhs.toDouble();
    માટે (પૂર્ણાંક bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        જો (bitIs1(bit) && rhs.bitIs0(bit)) પાછા સાચું;
        જો (rhs.bitIs1(bit) && bitIs0(bit)) પાછા ખોટું;
        જો (bitIsXZ(bit)) પાછા સાચું;
        જો (rhs.bitIsXZ(bit)) પાછા ખોટું;
    }
    પાછા ખોટું;
}
પૂર્ણાંક V3_સંખ્યા::ગણતરી_X(પૂર્ણાંક lsb, પૂર્ણાંક nbits) અચલ VL_MT_SAFE {
    પૂર્ણાંક ગણતરી = 0;
    માટે (પૂર્ણાંક bitn = 0; bitn < nbits; ++bitn) {
        જો (lsb + bitn >= width()) પાછા ગણતરી;
        જો (bitIsX(lsb + bitn)) ++ગણતરી;
    }
    પાછા ગણતરી;
}
પૂર્ણાંક V3_સંખ્યા::ગણતરી_Z(પૂર્ણાંક lsb, પૂર્ણાંક nbits) અચલ VL_MT_SAFE {
    પૂર્ણાંક ગણતરી = 0;
    માટે (પૂર્ણાંક bitn = 0; bitn < nbits; ++bitn) {
        જો (lsb + bitn >= width()) પાછા ગણતરી;
        જો (bitIsZ(lsb + bitn)) ++ગણતરી;
    }
    પાછા ગણતરી;
}

પૂર્ણાંક V3_સંખ્યા::widthToFit() અચલ {
    માટે (પૂર્ણાંક bit = width() - 1; bit > 0; bit--) {
        જો (!bitIs0(bit)) પાછા bit + 1;
    }
    પાછા 1;  // one bit even if number is == 0
}

uint32_t V3_સંખ્યા::ગણતરી_Bits(અચલ V3_સંખ્યા& ctrl) અચલ {
    // Correct number of zero bits/width matters
    પૂર્ણાંક n = 0;
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        પસંદગી (ctrl.bitIs(0)) {
        સ્થિતિ '0':
            જો (bitIs0(bit)) ++n;
            તોડો;
        સ્થિતિ '1':
            જો (bitIs1(bit)) ++n;
            તોડો;
        સ્થિતિ 'x':
            જો (bitIsX(bit)) ++n;
            તોડો;
        સ્થિતિ 'z':
            જો (bitIsZ(bit)) ++n;
            તોડો;
        }
    }
    પાછા n;
}

uint32_t V3_સંખ્યા::ગણતરી_Bits(અચલ V3_સંખ્યા& ctrl1, અચલ V3_સંખ્યા& ctrl2,
                             અચલ V3_સંખ્યા& ctrl3) અચલ {
    // Correct number of zero bits/width matters
    પૂર્ણાંક n = ગણતરી_Bits(ctrl1);
    જો (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += ગણતરી_Bits(ctrl2);
    જો ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += ગણતરી_Bits(ctrl3);
    }
    પાછા n;
}

uint32_t V3_સંખ્યા::ગણતરી_Ones() અચલ {
    પૂર્ણાંક n = 0;
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (bitIs1(bit)) ++n;
    }
    પાછા n;
}

uint32_t V3_સંખ્યા::mostSetBitP1() અચલ {
    માટે (પૂર્ણાંક bit = width() - 1; bit >= 0; bit--) {
        જો (!bitIs0(bit)) પાછા bit + 1;
    }
    પાછા 0;
}

uint32_t V3_સંખ્યા::leastSetBitP1() અચલ {
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (!bitIs0(bit)) પાછા bit + 1;
    }
    પાછા 0;
}

//======================================================================

V3_સંખ્યા& V3_સંખ્યા::opBitsNonXZ(અચલ V3_સંખ્યા& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opBitsOne(અચલ V3_સંખ્યા& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opBitsXZ(અચલ V3_સંખ્યા& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opBitsZ(અચલ V3_સંખ્યા& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    પાછા *આ;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_સંખ્યા& V3_સંખ્યા::opRedOr(અચલ V3_સંખ્યા& lhs) {
    // op i, 1 bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    અક્ષર outc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < lhs.width(); ++bit) {
        જો (lhs.bitIs1(bit)) {
            પાછા setSingleBits(1);
        } નહીંતર જો (lhs.bitIs0(bit)) {
        } નહીંતર {
            outc = 'x';
        }
    }
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opRedAnd(અચલ V3_સંખ્યા& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    અક્ષર outc = 1;
    માટે (પૂર્ણાંક bit = 0; bit < lhs.width(); ++bit) {
        જો (lhs.bitIs0(bit)) {
            પાછા setSingleBits(0);
        } નહીંતર જો (lhs.bitIs1(bit)) {
        } નહીંતર {
            outc = 'x';
        }
    }
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opRedXor(અચલ V3_સંખ્યા& lhs) {
    // op i, 1 bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    અક્ષર outc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < lhs.width(); ++bit) {
        જો (lhs.bitIs1(bit)) {
            જો (outc == 1) {
                outc = 0;
            } નહીંતર જો (outc == 0) {
                outc = 1;
            }
        } નહીંતર જો (lhs.bitIs0(bit)) {
        } નહીંતર {
            outc = 'x';
        }
    }
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::op_ગણતરી_Bits(અચલ V3_સંખ્યા& પદાવલિ, અચલ V3_સંખ્યા& ctrl1, અચલ V3_સંખ્યા& ctrl2,
                                અચલ V3_સંખ્યા& ctrl3) {
    // Correct number of zero bits/width matters
    અંક_ASSERT_OP_ARGS4(પદાવલિ, ctrl1, ctrl2, ctrl3);
    અંક_ASSERT_LOGIC_ARGS4(પદાવલિ, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.અંક()[0].m_કિંમત = પદાવલિ.ગણતરી_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::op_ગણતરી_Ones(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    જો (lhs.isFourState()) પાછા setAllBitsX();
    setZero();
    m_data.અંક()[0].m_કિંમત = lhs.ગણતરી_Ones();
    opCleanThis();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opIsUnknown(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    પાછા setSingleBits(lhs.isAnyXZ());
}
V3_સંખ્યા& V3_સંખ્યા::opOneHot(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    જો (lhs.isFourState()) પાછા setAllBitsX();
    પાછા setSingleBits(lhs.ગણતરી_Ones() == 1);
}
V3_સંખ્યા& V3_સંખ્યા::opOneHot0(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    જો (lhs.isFourState()) પાછા setAllBitsX();
    પાછા setSingleBits(lhs.ગણતરી_Ones() <= 1);
}
V3_સંખ્યા& V3_સંખ્યા::opCLog2(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    જો (lhs.isFourState()) પાછા setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    અચલ પૂર્ણાંક adjust = (lhs.ગણતરી_Ones() == 1) ? 0 : 1;
    માટે (પૂર્ણાંક bit = lhs.width() - 1; bit >= 0; bit--) {
        જો (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            પાછા *આ;
        }
    }
    setZero();
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opMostSetBitP1(અચલ V3_સંખ્યા& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    જો (lhs.isFourState()) પાછા setAllBitsX();
    માટે (પૂર્ણાંક bit = lhs.width() - 1; bit >= 0; bit--) {
        જો (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            પાછા *આ;
        }
    }
    setZero();
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opLogNot(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    અક્ષર outc = 1;
    માટે (પૂર્ણાંક bit = 0; bit < lhs.width(); ++bit) {
        જો (lhs.bitIs1(bit)) {
            outc = 0;
            જાઓ last;
        } નહીંતર જો (lhs.bitIs0(bit)) {
        } નહીંતર {
            outc = 'x';
        }
    }
last:
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opNot(અચલ V3_સંખ્યા& lhs) {
    // Correct number of zero bits/width matters
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } નહીંતર જો (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opAnd(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } નહીંતર જો (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } નહીંતર {
            setBit(bit, 'x');
        }
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opOr(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } નહીંતર જો (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } નહીંતર {
            setBit(bit, 'x');
        }
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opXor(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } નહીંતર જો (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } નહીંતર જો (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    પાછા *આ;
}

શૂન્ય V3_સંખ્યા::fillBits(પૂર્ણાંક destLsb, પૂર્ણાંક width, અક્ષર કિંમત) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    જો (!width) પાછા;
    UASSERT(width <= આ->width() - destLsb, "Destination bit range exceeds number width");

    અચલ uint32_t કિંમત_Word = કિંમત == '1' || કિંમત == 'x' || કિંમત == 1 || કિંમત == 3 ? ~0U : 0;
    અચલ uint32_t xWord = કિંમત == 'z' || કિંમત == 'x' || કિંમત == 2 || કિંમત == 3 ? ~0U : 0;
    અચલ પૂર્ણાંક firstWord = VL_BITWORD_E(destLsb);
    અચલ પૂર્ણાંક lastWord = VL_BITWORD_E(destLsb + width - 1);
    માટે (પૂર્ણાંક word = firstWord; word <= lastWord; ++word) {
        અચલ પૂર્ણાંક wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        અચલ પૂર્ણાંક wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        અચલ uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        કિંમત_And_X& dest = m_data.અંક()[word];
        dest.m_કિંમત = (dest.m_કિંમત & ~mask) | (કિંમત_Word & mask);
        dest.m_કિંમત_X = (dest.m_કિંમત_X & ~mask) | (xWord & mask);
    }
}

શૂન્ય V3_સંખ્યા::copyBits(પૂર્ણાંક destLsb, અચલ V3_સંખ્યા& source, પૂર્ણાંક sourceLsb, પૂર્ણાંક width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    જો (!width) પાછા;
    UASSERT(width <= આ->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    અચલ પૂર્ણાંક firstWord = VL_BITWORD_E(destLsb);
    અચલ પૂર્ણાંક lastWord = VL_BITWORD_E(destLsb + width - 1);
    માટે (પૂર્ણાંક destWord = firstWord; destWord <= lastWord; ++destWord) {
        અચલ પૂર્ણાંક destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        અચલ પૂર્ણાંક destOffset = VL_BITBIT_E(destBit);
        અચલ પૂર્ણાંક sourceBit = sourceLsb + destBit - destLsb;
        અચલ પૂર્ણાંક sourceWord = VL_BITWORD_E(sourceBit);
        અચલ પૂર્ણાંક sourceOffset = VL_BITBIT_E(sourceBit);
        અચલ પૂર્ણાંક copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        અચલ uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        અચલ કિંમત_And_X& sourceLo = source.m_data.અંક()[sourceWord];
        uint32_t કિંમત = sourceLo.m_કિંમત >> sourceOffset;
        uint32_t કિંમત_X = sourceLo.m_કિંમત_X >> sourceOffset;
        જો (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            અચલ કિંમત_And_X& sourceHi = source.m_data.અંક()[sourceWord + 1];
            કિંમત |= sourceHi.m_કિંમત << (VL_EDATASIZE - sourceOffset);
            કિંમત_X |= sourceHi.m_કિંમત_X << (VL_EDATASIZE - sourceOffset);
        }

        કિંમત_And_X& dest = m_data.અંક()[destWord];
        dest.m_કિંમત = (dest.m_કિંમત & ~mask) | ((કિંમત << destOffset) & mask);
        dest.m_કિંમત_X = (dest.m_કિંમત_X & ~mask) | ((કિંમત_X << destOffset) & mask);
    }
}

V3_સંખ્યા& V3_સંખ્યા::opConcat(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Correct number of zero bits/width matters
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    જો (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opLenN(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opRepl(અચલ V3_સંખ્યા& lhs,
                           અચલ V3_સંખ્યા& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    પાછા opRepl(lhs, rhs.toUInt());
}

V3_સંખ્યા& V3_સંખ્યા::opRepl(અચલ V3_સંખ્યા& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    જો (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    માટે (unsigned times = 0; times < rhsval; ++times) {
        અચલ uint64_t destLsb = uint64_t{times} * lhs.width();
        જો (destLsb >= static_cast<uint32_t>(width())) તોડો;
        copyBits(static_cast<પૂર્ણાંક>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<પૂર્ણાંક>(destLsb)));
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opStreamL(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    જો (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    અચલ પૂર્ણાંક ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    માટે (પૂર્ણાંક istart = 0; istart < lhs.width(); istart += ssize) {
        અચલ પૂર્ણાંક ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opLogAnd(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    અક્ષર loutc = 0;
    અક્ષર routc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < lhs.width(); ++bit) {
        જો (lhs.bitIs1(bit)) {
            loutc = 1;
            તોડો;
        }
        જો (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    માટે (પૂર્ણાંક bit = 0; bit < rhs.width(); ++bit) {
        જો (rhs.bitIs1(bit)) {
            routc = 1;
            તોડો;
        }
        જો (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    અક્ષર outc = 'x';
    જો (routc == 1 && loutc == 1) outc = 1;
    જો (routc == 0 || loutc == 0) outc = 0;
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opLogOr(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    અક્ષર outc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < lhs.width(); ++bit) {
        જો (lhs.bitIs1(bit)) {
            outc = 1;
            જાઓ last;
        }
        જો (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    માટે (પૂર્ણાંક bit = 0; bit < rhs.width(); ++bit) {
        જો (rhs.bitIs1(bit)) {
            outc = 1;
            જાઓ last;
        }
        જો (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opLogIf(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    V3_સંખ્યા lnot = lhs;
    lnot.opLogNot(lhs);
    પાછા opLogOr(lnot, rhs);
}

V3_સંખ્યા& V3_સંખ્યા::opLogEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    V3_સંખ્યા ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_સંખ્યા ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    પાછા opLogAnd(ifa, ifb);
}

V3_સંખ્યા& V3_સંખ્યા::opAtoN(અચલ V3_સંખ્યા& lhs, પૂર્ણાંક base) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    જો (base == AstAtoN::ATOREAL) પાછા setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    જો (errno != 0) v = 0;
    પાછા setLongS(static_cast<int32_t>(v));
}

V3_સંખ્યા& V3_સંખ્યા::opPutcN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, અચલ V3_સંખ્યા& ths) {
    અંક_ASSERT_OP_ARGS3(lhs, rhs, ths);
    અંક_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    અચલ int32_t i = rhs.toSInt();
    અચલ int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    જો (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    પાછા setString(lstring);
}

V3_સંખ્યા& V3_સંખ્યા::opGetcN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    અચલ string lstring = lhs.toString();
    અચલ int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    જો (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    પાછા setLong(v);
}

V3_સંખ્યા& V3_સંખ્યા::opSubstrN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, અચલ V3_સંખ્યા& ths) {
    અંક_ASSERT_OP_ARGS3(lhs, rhs, ths);
    અંક_ASSERT_STRING_ARGS1(lhs);
    અચલ string lstring = lhs.toString();
    અચલ int32_t i = rhs.toSInt();
    અચલ int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    જો (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) પાછા setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    પાછા setString(lstring.substr(i, j - i + 1));
}

V3_સંખ્યા& V3_સંખ્યા::opCompareNN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, તાર્કિક ignoreCase) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    પૂર્ણાંક result;
    અચલ string lstring = lhs.toString();
    અચલ string rstring = rhs.toString();
    જો (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } નહીંતર {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    પાછા setLongS(result);
}

V3_સંખ્યા& V3_સંખ્યા::opEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    જો (lhs.isString()) પાછા opEqN(lhs, rhs);
    જો (lhs.isDouble()) પાછા opEqD(lhs, rhs);
    અક્ષર outc = 1;
    માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        જો (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            જાઓ last;
        }
        જો (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            જાઓ last;
        }
        જો (lhs.bitIsXZ(bit)) outc = 'x';
        જો (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opNeq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    જો (lhs.isString()) પાછા opNeqN(lhs, rhs);
    જો (lhs.isDouble()) પાછા opNeqD(lhs, rhs);
    અક્ષર outc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        જો (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            જાઓ last;
        }
        જો (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            જાઓ last;
        }
        જો (lhs.bitIsXZ(bit)) outc = 'x';
        જો (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    પાછા setSingleBits(outc);
}

V3Hash V3_સંખ્યા::toHash() અચલ {
    // Shouldmatch isCaseEq
    જો (isString()) પાછા V3Hash{toString()};
    જો (isDouble()) પાછા V3Hash{toDouble()};
    V3Hash hash{width()};
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) hash += m_data.અંક()[i].m_કિંમત;
    પાછા hash;
}

તાર્કિક V3_સંખ્યા::isCaseEq(અચલ V3_સંખ્યા& rhs) અચલ {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    જો (isString()) પાછા toString() == rhs.toString();
    જો (isDouble()) પાછા toDouble() == rhs.toDouble();
    જો (width() != rhs.width()) પાછા ખોટું;
    માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
        જો (!(m_data.અંક()[i] == rhs.m_data.અંક()[i])) પાછા ખોટું;
    }
    પાછા સાચું;
}

V3_સંખ્યા& V3_સંખ્યા::opCaseEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_સંખ્યા& V3_સંખ્યા::opCaseNeq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અક્ષર outc = 0;
    જો (lhs.isString()) {
        પાછા opNeqN(lhs, rhs);
    } નહીંતર જો (lhs.isDouble()) {
        પાછા opNeqD(lhs, rhs);
    }
    માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        જો (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            જાઓ last;
        }
    }
last:
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opWildEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    અક્ષર outc = 1;
    માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        જો (!rhs.bitIsXZ(bit)) {
            જો (lhs.bitIsXZ(bit)) {
                outc = 'x';
                જાઓ last;
            }
            જો (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                જાઓ last;
            }
        }
    }
last:
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opWildNeq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    અક્ષર outc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        જો (!rhs.bitIsXZ(bit)) {
            જો (lhs.bitIsXZ(bit)) {
                outc = 'x';
                જાઓ last;
            }
            જો (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                જાઓ last;
            }
        }
    }
last:
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opGt(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    અક્ષર outc = 0;
    માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        જો (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        જો (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        જો (lhs.bitIsXZ(bit)) outc = 'x';
        જો (rhs.bitIsXZ(bit)) outc = 'x';
    }
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opGtS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    અક્ષર outc = 0;
    {
        અચલ પૂર્ણાંક mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        જો (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } નહીંતર જો (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } નહીંતર જો (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } નહીંતર જો (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } નહીંતર {
            // both positive or negative, normal >
            માટે (પૂર્ણાંક bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                જો (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                જો (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                જો (lhs.bitIsXZ(bit)) outc = 'x';
                જો (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    પાછા setSingleBits(outc);
}

V3_સંખ્યા& V3_સંખ્યા::opGte(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    V3_સંખ્યા& eq = opEq(lhs, rhs);
    જો (eq.isNeqZero()) પાછા eq;  // Return true
    પાછા opGt(lhs, rhs);
}

V3_સંખ્યા& V3_સંખ્યા::opGteS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    V3_સંખ્યા& eq = opEq(lhs, rhs);
    જો (eq.isNeqZero()) પાછા eq;  // Return true
    પાછા opGtS(lhs, rhs);
}

V3_સંખ્યા& V3_સંખ્યા::opLt(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { પાછા opGt(rhs, lhs); }
V3_સંખ્યા& V3_સંખ્યા::opLte(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { પાછા opGte(rhs, lhs); }
V3_સંખ્યા& V3_સંખ્યા::opLtS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { પાછા opGtS(rhs, lhs); }
V3_સંખ્યા& V3_સંખ્યા::opLteS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) { પાછા opGteS(rhs, lhs); }

V3_સંખ્યા& V3_સંખ્યા::opShiftR(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // L(lhs) bit return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (rhs.isFourState()) પાછા setAllBitsX();
    setZero();
    જો (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) પાછા *આ;
    અચલ uint32_t rhsval = rhs.toUInt();
    જો (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<પૂર્ણાંક>(rhsval),
                 std::min(width(), lhs.width() - static_cast<પૂર્ણાંક>(rhsval)));
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opShiftRS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (rhs.isFourState()) પાછા setAllBitsX();
    setZero();
    અચલ તાર્કિક overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    જો (!overflow) {
        અચલ uint32_t rhsval = rhs.toUInt();
        જો (rhsval < lbits) {
            અચલ પૂર્ણાંક copyWidth = std::min(width(), static_cast<પૂર્ણાંક>(lbits - rhsval));
            copyBits(0, lhs, static_cast<પૂર્ણાંક>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            પાછા *આ;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opShiftL(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // L(lhs) bit return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (rhs.isFourState()) પાછા setAllBitsX();
    setZero();
    જો (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) પાછા *આ;
    અચલ uint32_t rhsval = rhs.toUInt();
    જો (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<પૂર્ણાંક>(rhsval), lhs, 0,
                 std::min(width() - static_cast<પૂર્ણાંક>(rhsval), lhs.width()));
    }
    પાછા *આ;
}

//======================================================================
// Ops - Arithmetic

V3_સંખ્યા& V3_સંખ્યા::opNegate(અચલ V3_સંખ્યા& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    જો (lhs.isFourState()) પાછા setAllBitsX();
    V3_સંખ્યા notlhs(&lhs, width());
    notlhs.opNot(lhs);
    અચલ V3_સંખ્યા one(&lhs, width(), 1);
    opAdd(notlhs, one);
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opAdd(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    માટે (પૂર્ણાંક word = 0; word < શબ્દો(); ++word) {
        અચલ uint64_t lwordval = lhs.m_data.અંક()[word].m_કિંમત;
        અચલ uint64_t rwordval = rhs.m_data.અંક()[word].m_કિંમત;
        અચલ uint64_t sum = lwordval + rwordval + carry;
        m_data.અંક()[word].m_કિંમત = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opSub(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    V3_સંખ્યા negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    પાછા opAdd(lhs, negrhs);
}
V3_સંખ્યા& V3_સંખ્યા::opMul(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    setZero();
    જો (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } નહીંતર {
        માટે (પૂર્ણાંક lword = 0; lword < lhs.શબ્દો(); ++lword) {
            અચલ uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.અંક()[lword].m_કિંમત);
            જો (lwordval == 0) ચાલુ;
            માટે (પૂર્ણાંક rword = 0; rword < rhs.શબ્દો(); ++rword) {
                અચલ uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.અંક()[rword].m_કિંમત);
                જો (rwordval == 0) ચાલુ;
                uint64_t mul = lwordval * rwordval;
                માટે (પૂર્ણાંક qword = lword + rword; qword < શબ્દો(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.અંક()[qword].m_કિંમત);
                    m_data.અંક()[qword].m_કિંમત = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    જો (mul == 0) તોડો;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opMulS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    V3_સંખ્યા lhsNoSign = lhs;
    જો (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_સંખ્યા rhsNoSign = rhs;
    જો (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    અચલ V3_સંખ્યા qNoSign = opMul(lhsNoSign, rhsNoSign);
    જો ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } નહીંતર {
        opAssign(qNoSign);
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opDiv(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    જો (rhs.isEqZero()) પાછા setAllBitsXRemoved();
    જો (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        પાછા *આ;
    } નહીંતર {
        // Wide division
        પાછા opModDivGuts(lhs, rhs, ખોટું);
    }
}
V3_સંખ્યા& V3_સંખ્યા::opDivS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    જો (rhs.isEqZero()) પાછા setAllBitsXRemoved();
    V3_સંખ્યા lhsNoSign = lhs;
    જો (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_સંખ્યા rhsNoSign = rhs;
    જો (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    અચલ V3_સંખ્યા qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    જો ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } નહીંતર {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *આ);
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opModDiv(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    જો (rhs.isEqZero()) પાછા setAllBitsXRemoved();
    જો (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        પાછા *આ;
    } નહીંતર {
        // Wide modulus
        પાછા opModDivGuts(lhs, rhs, સાચું);
    }
}
V3_સંખ્યા& V3_સંખ્યા::opModDivS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    જો (rhs.isEqZero()) પાછા setAllBitsXRemoved();
    V3_સંખ્યા lhsNoSign = lhs;
    જો (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_સંખ્યા rhsNoSign = rhs;
    જો (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    અચલ V3_સંખ્યા qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    જો (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } નહીંતર {
        opAssign(qNoSign);
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opModDivGuts(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, તાર્કિક is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    અચલ પૂર્ણાંક l_શબ્દો = lhs.શબ્દો();
    અચલ પૂર્ણાંક umsbp1 = lhs.mostSetBitP1();  // dividend
    અચલ પૂર્ણાંક vmsbp1 = rhs.mostSetBitP1();  //ભાજક
    જો (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *આ);
        પાછા *આ;
    }

    અચલ પૂર્ણાંક uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    અચલ પૂર્ણાંક vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    જો (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        માટે (પૂર્ણાંક j = uw - 1; j >= 0; j--) {
            અચલ uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.અંક()[j].m_કિંમત));
            m_data.અંક()[j].m_કિંમત = unw64 / static_cast<uint64_t>(rhs.m_data.અંક()[0].m_કિંમત);
            k = unw64
                - (static_cast<uint64_t>(m_data.અંક()[j].m_કિંમત)
                   * static_cast<uint64_t>(rhs.m_data.અંક()[0].m_કિંમત));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *આ << " rem=0x" << std::hex
                                   << k << std::dec);
        જો (is_modulus) {
            setZero();
            m_data.અંક()[0].m_કિંમત = k;
        }
        opCleanThis();
        પાછા *આ;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_શબ્દો + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_શબ્દો + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    માટે (પૂર્ણાંક i = 0; i < l_શબ્દો; ++i) m_data.અંક()[i].m_કિંમત = 0;
    માટે (પૂર્ણાંક i = 0; i < l_શબ્દો + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    અચલ પૂર્ણાંક s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    અચલ uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    માટે (પૂર્ણાંક i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.અંક()[i].m_કિંમત << s)
                | (shift_mask & (rhs.m_data.અંક()[i - 1].m_કિંમત >> (32 - s)));
    }
    vn[0] = rhs.m_data.અંક()[0].m_કિંમત << s;

    // Copy and shift dividend by same amount; may set new upper word
    જો (s) {
        un[uw] = lhs.m_data.અંક()[uw - 1].m_કિંમત >> (32 - s);
    } નહીંતર {
        un[uw] = 0;
    }
    માટે (પૂર્ણાંક i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.અંક()[i].m_કિંમત << s)
                | (shift_mask & (lhs.m_data.અંક()[i - 1].m_કિંમત >> (32 - s)));
    }
    un[0] = lhs.m_data.અંક()[0].m_કિંમત << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    માટે (પૂર્ણાંક j = uw - vw; j >= 0; j--) {
        // Estimate
        અચલ uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        જો (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            જો (rhat < 0x100000000ULL) જાઓ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        માટે (પૂર્ણાંક i = 0; i < vw; ++i) {
            અચલ uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.અંક()[j].m_કિંમત = qhat;  // Save quotient digit

        જો (t < 0) {
            // Over subtracted; correct by adding back
            m_data.અંક()[j].m_કિંમત--;
            k = 0;
            માટે (પૂર્ણાંક i = 0; i < vw; ++i) {
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

    જો (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        માટે (પૂર્ણાંક i = 0; i < vw; ++i) {
            m_data.અંક()[i].m_કિંમત = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        માટે (પૂર્ણાંક i = vw; i < l_શબ્દો; ++i) m_data.અંક()[i].m_કિંમત = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *આ);
        પાછા *આ;
    } નહીંતર {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *આ);
        પાછા *આ;
    }
}

V3_સંખ્યા& V3_સંખ્યા::opPow(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, તાર્કિક lsign, તાર્કિક rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_LOGIC_ARGS2(lhs, rhs);
    જો (lhs.isFourState() || rhs.isFourState()) પાછા setAllBitsX();
    જો (rhs.isEqZero()) પાછા setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    જો (rsign && rhs.isNegative()) {
        જો (lhs.isEqZero()) {
            પાછા setAllBitsXRemoved();
        } નહીંતર જો (lhs.isEqOne()) {
            પાછા setValue1();
        } નહીંતર જો (lsign && lhs.isEqAllOnes()) {
            જો (rhs.bitIs1(0)) {
                પાછા setAllBits1();  // -1^odd=-1
            } નહીંતર {
                પાછા setValue1();  // -1^even=1
            }
        }
        પાછા setZero();
    }
    જો (lhs.isEqZero()) પાછા setZero();
    setZero();
    m_data.અંક()[0].m_કિંમત = 1;
    V3_સંખ્યા power(&lhs, width());
    power.opAssign(lhs);
    માટે (પૂર્ણાંક bit = 0; bit < rhs.width(); ++bit) {
        જો (bit > 0) {  // power = power*power
            V3_સંખ્યા lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        જો (rhs.bitIs1(bit)) {  // out *= power
            V3_સંખ્યા lastOut(&lhs, width());
            lastOut.opAssign(*આ);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opPowSU(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Correct number of zero bits/width matters
    પાછા opPow(lhs, rhs, સાચું, ખોટું);
}
V3_સંખ્યા& V3_સંખ્યા::opPowSS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Correct number of zero bits/width matters
    પાછા opPow(lhs, rhs, સાચું, સાચું);
}
V3_સંખ્યા& V3_સંખ્યા::opPowUS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // Correct number of zero bits/width matters
    પાછા opPow(lhs, rhs, ખોટું, સાચું);
}

V3_સંખ્યા& V3_સંખ્યા::opBufIf1(અચલ V3_સંખ્યા& ens, અચલ V3_સંખ્યા& if1s) {
    અંક_ASSERT_OP_ARGS2(ens, if1s);
    અંક_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    માટે (પૂર્ણાંક bit = 0; bit < width(); ++bit) {
        જો (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } નહીંતર {
            setBit(bit, 'z');
        }
    }
    પાછા *આ;
}

// Sets all bits in range to the given value
V3_સંખ્યા& V3_સંખ્યા::opSetRange(uint32_t lsb, uint32_t width, અક્ષર bit_કિંમત) {
    જો (lsb < static_cast<uint32_t>(આ->width())) {
        fillBits(static_cast<પૂર્ણાંક>(lsb),
                 static_cast<પૂર્ણાંક>(std::min(width, static_cast<uint32_t>(આ->width()) - lsb)),
                 bit_કિંમત);
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opAssign(અચલ V3_સંખ્યા& lhs) { પાછા opAssignNonXZ(lhs, ખોટું); }
V3_સંખ્યા& V3_સંખ્યા::opAssignNonXZ(અચલ V3_સંખ્યા& lhs, તાર્કિક ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    જો (આ != &lhs) {
        જો (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = સાચું;
        } નહીંતર જો (isString()) {
            જો (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } નહીંતર {
                m_data.str() = lhs.m_data.str();
            }
        } નહીંતર જો (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } નહીંતર જો (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } નહીંતર જો (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } નહીંતર {
            માટે (પૂર્ણાંક word = 0; word < શબ્દો(); ++word) {
                અચલ uint32_t કિંમત = word < lhs.શબ્દો() ? lhs.m_data.અંક()[word].m_કિંમત
                                                                & ~lhs.m_data.અંક()[word].m_કિંમત_X
                                                          : 0;
                m_data.અંક()[word] = {કિંમત, 0};
            }
            opCleanThis();
        }
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opNToI(અચલ V3_સંખ્યા& lhs) {
    // String to packed number
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    setZero();
    અચલ string& str = lhs.toString();
    માટે (size_t n = 0; n < str.length(); ++n) {
        અચલ અક્ષર c = str[str.length() - 1 - n];
        માટે (size_t cbit = 0; cbit < 8; ++cbit) {
            અચલ std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opExtendS(અચલ V3_સંખ્યા& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    અચલ પૂર્ણાંક copyWidth = std::min(width(), static_cast<પૂર્ણાંક>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opExtendXZ(અચલ V3_સંખ્યા& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    અચલ પૂર્ણાંક copyWidth = std::min(width(), static_cast<પૂર્ણાંક>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opClean(અચલ V3_સંખ્યા& lhs, uint32_t bits) { પાછા opSel(lhs, bits - 1, 0); }

શૂન્ય V3_સંખ્યા::opCleanThis(તાર્કિક warnOnTruncation) {
    // Clean MSB of number
    અંક_ASSERT_LOGIC_ARGS1(*આ);
    અચલ કિંમત_And_X v = m_data.અંક()[શબ્દો() - 1];
    અચલ uint32_t new_કિંમત_Msb = v.m_કિંમત & hiWordMask();
    અચલ uint32_t new_કિંમત_XMsb = v.m_કિંમત_X & hiWordMask();
    જો (warnOnTruncation && (new_કિંમત_Msb != v.m_કિંમત || new_કિંમત_XMsb != v.m_કિંમત_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.અંક()[શબ્દો() - 1] = {new_કિંમત_Msb, new_કિંમત_XMsb};
}

V3_સંખ્યા& V3_સંખ્યા::opSel(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& msb, અચલ V3_સંખ્યા& lsb) {
    અંક_ASSERT_OP_ARGS3(lhs, msb, lsb);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS2(msb, lsb);
    જો (lsb.isFourState() || msb.isFourState()) પાછા setAllBitsX();
    પાછા opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_સંખ્યા& V3_સંખ્યા::opSel(અચલ V3_સંખ્યા& lhs, uint32_t msbval, uint32_t lsbval) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    પૂર્ણાંક copyWidth = 0;
    જો (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        અચલ uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<પૂર્ણાંક>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<પૂર્ણાંક>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    પાછા *આ;
}

V3_સંખ્યા& V3_સંખ્યા::opSelInto(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& lsb, પૂર્ણાંક width) {
    પાછા opSelInto(lhs, lsb.toSInt(), width);
}

V3_સંખ્યા& V3_સંખ્યા::opSelInto(અચલ V3_સંખ્યા& lhs, પૂર્ણાંક lsbval, પૂર્ણાંક width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    અચલ પૂર્ણાંક copyWidth = std::max(0, std::min({width, lhs.width(), આ->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    અચલ પૂર્ણાંક fillWidth
        = std::max(0, std::min(width - copyWidth, આ->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    પાછા *આ;
}

//======================================================================
// Ops - Floating point

V3_સંખ્યા& V3_સંખ્યા::opIToRD(અચલ V3_સંખ્યા& lhs, તાર્કિક isSigned) {
    // Correct number of zero bits/width matters
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_સંખ્યા noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    દ્વિગુણ d = 0;
    અચલ તાર્કિક negate = isSigned && noxz.isNegative();
    જો (negate) {
        અચલ V3_સંખ્યા noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    માટે (પૂર્ણાંક bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        જો (noxz.bitIs1(bit)) d += exp2(bit);
    }
    જો (negate) d = -d;
    પાછા setDouble(d);
}
V3_સંખ્યા& V3_સંખ્યા::opRToIS(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_DOUBLE_ARGS1(lhs);
    અચલ દ્વિગુણ v = VL_TRUNC(lhs.toDouble());
    અચલ int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    પાછા setLongS(i);
}
V3_સંખ્યા& V3_સંખ્યા::opRToIRoundS(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_DOUBLE_ARGS1(lhs);
    અચલ દ્વિગુણ v = VL_ROUND(lhs.toDouble());
    setZero();
    સંઘ {
        દ્વિગુણ d;
        uint64_t q;
    } u;
    u.d = v;
    (શૂન્ય)u.d;

    અચલ પૂર્ણાંક exp = static_cast<પૂર્ણાંક>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    અચલ પૂર્ણાંક lsb = exp - 52;
    અચલ uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    જો (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        માટે (પૂર્ણાંક bit = 0; bit <= 52; ++bit) {
            જો (mantissa & (1ULL << bit)) {
                અચલ પૂર્ણાંક outbit = bit + lsb;
                જો (outbit >= 0) setBit(outbit, 1);
            }
        }
        જો (v < 0) {
            અચલ V3_સંખ્યા noSign = *આ;
            opNegate(noSign);
        }
    }
    પાછા *આ;
}
V3_સંખ્યા& V3_સંખ્યા::opRealToBits(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    સંઘ {
        દ્વિગુણ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    પાછા setQuad(u.m_v);
}
V3_સંખ્યા& V3_સંખ્યા::opBitsToRealD(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    સંઘ {
        દ્વિગુણ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    પાછા setDouble(u.m_d);
}
V3_સંખ્યા& V3_સંખ્યા::opNegateD(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_DOUBLE_ARGS1(lhs);
    પાછા setDouble(-lhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opAddD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opSubD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opMulD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opDivD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opPowD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_સંખ્યા& V3_સંખ્યા::opEqD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opNeqD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opGtD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opGteD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opLtD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_સંખ્યા& V3_સંખ્યા::opLteD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_સંખ્યા& V3_સંખ્યા::opConcatN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setString(lhs.toString() + rhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opReplN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_STRING_ARGS1(lhs);
    અંક_ASSERT_LOGIC_ARGS1(rhs);
    પાછા opReplN(lhs, rhs.toUInt());
}
V3_સંખ્યા& V3_સંખ્યા::opReplN(અચલ V3_સંખ્યા& lhs, uint32_t rhsval) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    માટે (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    પાછા setString(out);
}
V3_સંખ્યા& V3_સંખ્યા::opCvtPackString(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    પાછા setString(lhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opToLowerN(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    અચલ std::string out = VString::downcase(lhs.toString());
    પાછા setString(out);
}
V3_સંખ્યા& V3_સંખ્યા::opToUpperN(અચલ V3_સંખ્યા& lhs) {
    અંક_ASSERT_OP_ARGS1(lhs);
    અંક_ASSERT_STRING_ARGS1(lhs);
    અચલ std::string out = VString::upcase(lhs.toString());
    પાછા setString(out);
}

V3_સંખ્યા& V3_સંખ્યા::opEqN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toString() == rhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opNeqN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toString() != rhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opGtN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toString() > rhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opGteN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toString() >= rhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opLtN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toString() < rhs.toString());
}
V3_સંખ્યા& V3_સંખ્યા::opLteN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs) {
    અંક_ASSERT_OP_ARGS2(lhs, rhs);
    અંક_ASSERT_STRING_ARGS2(lhs, rhs);
    પાછા setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

શૂન્ય V3_સંખ્યા::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_લીટી* અચલ fl = નવું File_લીટી{File_લીટી::builtInFilename()};
    V3_સંખ્યા અંક{fl, 32, 0};
    અંક.selfTestThis();
}

શૂન્ય V3_સંખ્યા::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_સંખ્યા::epsilonEqual(0, 0), સાચું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonEqual(1e19, 1e19), સાચું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonEqual(9, 0.0001), ખોટું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonEqual(1, 1 + std::numeric_limits<દ્વિગુણ>::epsilon()), સાચું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonEqual(0.009, 0.00899999999999999931998839741709), સાચું);

    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(0), સાચું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(1), સાચું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(-1), સાચું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(1.0001), ખોટું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(0.9999), ખોટું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(-1.0001), ખોટું);
    UASSERT_SELFTEST(V3_સંખ્યા::epsilonIntegral(-0.9999), ખોટું);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
