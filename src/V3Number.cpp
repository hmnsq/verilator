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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Number.h"

#അന്തര്ഭാവ "V3File.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cerrno>
#അന്തര്ഭാവ <cmath>
#അന്തര്ഭാവ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr പൂർണ്ണസംഖ്യ MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define അക്കം_ASSERT_OP_ARGS1(arg1) \
    UASSERT((ഇത് != &(arg1)), "Number operation called with same source and dest")
#define അക്കം_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((ഇത് != &(arg1) && ഇത് != &(arg2)), \
            "Number operation called with same source and dest")
#define അക്കം_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((ഇത് != &(arg1) && ഇത് != &(arg2) && ഇത് != &(arg3)), \
            "Number operation called with same source and dest")
#define അക്കം_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((ഇത് != &(arg1) && ഇത് != &(arg2) && ഇത് != &(arg3) && ഇത് != &(arg4)), \
            "Number operation called with same source and dest")

#define അക്കം_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_സംഖ്യ_Data::V3_സംഖ്യ_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define അക്കം_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    അക്കം_ASSERT_LOGIC_ARGS1(arg1); \
    അക്കം_ASSERT_LOGIC_ARGS1(arg2)

#define അക്കം_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    അക്കം_ASSERT_LOGIC_ARGS1(arg1); \
    അക്കം_ASSERT_LOGIC_ARGS1(arg2); \
    അക്കം_ASSERT_LOGIC_ARGS1(arg3); \
    അക്കം_ASSERT_LOGIC_ARGS1(arg4)

#define അക്കം_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define അക്കം_ASSERT_STRING_ARGS2(arg1, arg2) \
    അക്കം_ASSERT_STRING_ARGS1(arg1); \
    അക്കം_ASSERT_STRING_ARGS1(arg2)

#define അക്കം_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define അക്കം_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    അക്കം_ASSERT_DOUBLE_ARGS1(arg1); \
    അക്കം_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

ശൂന്യം V3_സംഖ്യ::v3errorEnd(സ്ഥിരം std::ostringstream& str) സ്ഥിരം VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    എങ്കിൽ (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } അല്ലെങ്കിൽ എങ്കിൽ (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } അല്ലെങ്കിൽ {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

ശൂന്യം V3_സംഖ്യ::v3errorEndFatal(സ്ഥിരം std::ostringstream& str) സ്ഥിരം
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    പ്രതിജ്ഞാ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_സംഖ്യ::V3_സംഖ്യ(Ast_കണ്ണി* nodep, VerilogStringLiteral, സ്ഥിരം string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    എങ്കിൽ (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } അല്ലെങ്കിൽ {
        init(nodep, std::max<പൂർണ്ണസംഖ്യ>(str.length() * 8, 1));
        വേണ്ടി (unsigned pos = 0; pos < str.length(); ++pos) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ topos = str.length() - 1 - pos;
            മൂല്യം_And_X& v = m_data.അക്കം()[topos / 4];
            വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < 8; ++bit) {
                എങ്കിൽ (str[pos] & (1UL << bit)) v.m_മൂല്യം |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = സത്യം;
    opCleanThis(സത്യം);
}

V3_സംഖ്യ::V3_സംഖ്യ(Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി_DType* nodedtypep) {
    എങ്കിൽ (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } അല്ലെങ്കിൽ എങ്കിൽ (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } അല്ലെങ്കിൽ എങ്കിൽ (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } അല്ലെങ്കിൽ {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

ശൂന്യം V3_സംഖ്യ::create(സ്ഥിരം അക്ഷരം* sourcep) {
    m_data.setLogic();
    സ്ഥിരം അക്ഷരം* മൂല്യം_startp = sourcep;
    വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = sourcep; *cp; ++cp) {
        എങ്കിൽ (*cp == '\'') {
            മൂല്യം_startp = cp + 1;
            നിർത്തുക;
        }
    }

    തർക്കം userSized = അസത്യം;
    തർക്കം unbased = അസത്യം;
    അക്ഷരം base = '\0';
    എങ്കിൽ (മൂല്യം_startp != sourcep) {  // Has a '
        string widthn;
        സ്ഥിരം അക്ഷരം* cp = sourcep;
        വേണ്ടി (; *cp; ++cp) {
            എങ്കിൽ (*cp == '\'') {
                ++cp;
                നിർത്തുക;
            }
            എങ്കിൽ (*cp != '_') widthn += *cp;
        }
        ആകുമ്പോൾ (*cp == '_') cp++;
        എങ്കിൽ (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(സത്യം);
        }
        എങ്കിൽ (*cp) {
            base = *cp;
            ++cp;
        }
        മൂല്യം_startp = cp;

        userSized = widthn.length() != 0;
        എങ്കിൽ (std::atoi(widthn.c_str())) {
            എങ്കിൽ (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_അക്കം_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_അക്കം_Width(), സത്യം);
            } അല്ലെങ്കിൽ {
                width(std::atoi(widthn.c_str()), സത്യം);
            }
        }
    } അല്ലെങ്കിൽ {
        unbased = സത്യം;
        base = 'd';
    }

    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    എങ്കിൽ (base == '0') {
        width(1, അസത്യം);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (base == '1') {
        width(1, അസത്യം);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (std::tolower(base) == 'z') {
        width(1, അസത്യം);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (std::tolower(base) == 'x') {
        width(1, അസത്യം);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = സത്യം;
    }
    // Otherwise...
    അല്ലെങ്കിൽ എങ്കിൽ (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        സ്ഥിരം പൂർണ്ണസംഖ്യ widthBound = std::max<പൂർണ്ണസംഖ്യ>(32, std::strlen(മൂല്യം_startp) * 4);
        width(widthBound, അസത്യം);  // Will change width below
        എങ്കിൽ (unbased) isSigned(സത്യം);  // Also says the spec.
    }

    // Ignore leading blanks
    ആകുമ്പോൾ (*മൂല്യം_startp == '_' || std::isspace(*മൂല്യം_startp)) ++മൂല്യം_startp;
    എങ്കിൽ (!*മൂല്യം_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    എങ്കിൽ (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    പൂർണ്ണസംഖ്യ obit = 0;  // Start at LSB
    പൂർണ്ണസംഖ്യ base_align = 1;
    എങ്കിൽ (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        ആകുമ്പോൾ (*മൂല്യം_startp == '_' || *മൂല്യം_startp == '0') ++മൂല്യം_startp;
        // Convert decimal number to hex
        പൂർണ്ണസംഖ്യ olen = 0;
        uint32_t val = 0;
        പൂർണ്ണസംഖ്യ got_x = 0;
        പൂർണ്ണസംഖ്യ got_z = 0;
        പൂർണ്ണസംഖ്യ got_01 = 0;
        തർക്കം warnedTooMany = അസത്യം;
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = മൂല്യം_startp; *cp; ++cp) {
            തിരഞ്ഞെടുപ്പ് (std::tolower(*cp)) {
            അവസ്ഥ '0':  // FALLTHRU
            അവസ്ഥ '1':  // FALLTHRU
            അവസ്ഥ '2':  // FALLTHRU
            അവസ്ഥ '3':  // FALLTHRU
            അവസ്ഥ '4':  // FALLTHRU
            അവസ്ഥ '5':  // FALLTHRU
            അവസ്ഥ '6':  // FALLTHRU
            അവസ്ഥ '7':  // FALLTHRU
            അവസ്ഥ '8':  // FALLTHRU
            അവസ്ഥ '9': {
                എങ്കിൽ (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.അക്കം()[0].m_മൂല്യം = val;
                } അല്ലെങ്കിൽ {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_സംഖ്യ product(ഇത്, width() + 4);  // +4 for overflow detection
                    സ്ഥിരം V3_സംഖ്യ ten(ഇത്, width() + 4, 10);
                    സ്ഥിരം V3_സംഖ്യ addend(ഇത്, width(), (*cp - '0'));
                    product.opMul(*ഇത്, ten);
                    opAdd(product, addend);
                    എങ്കിൽ (!warnedTooMany && product.bits_മൂല്യം(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = സത്യം;
                    }
                }
                ++olen;
                got_01 = 1;
                നിർത്തുക;
            }
            അവസ്ഥ 'z':
            അവസ്ഥ '?': {
                got_z = 1;
                എങ്കിൽ (!userSized) width(32, അസത്യം);
                setAllBitsZ();
                നിർത്തുക;
            }
            അവസ്ഥ 'x': {
                got_x = 1;
                എങ്കിൽ (!userSized) width(32, അസത്യം);
                setAllBitsX();
                നിർത്തുക;
            }
            അവസ്ഥ '_': നിർത്തുക;
            സ്ഥിരസ്ഥിതി: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                നിർത്തുക;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        എങ്കിൽ ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << മൂല്യം_startp);
        }
    } അല്ലെങ്കിൽ {
        // Convert bin/octal number to hex
        വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = മൂല്യം_startp + std::strlen(മൂല്യം_startp) - 1; cp >= മൂല്യം_startp;
             cp--) {
            എങ്കിൽ (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                നിർത്തുക;
            }
            തിരഞ്ഞെടുപ്പ് (std::tolower(base)) {
            അവസ്ഥ 'b': {
                base_align = 1;
                തിരഞ്ഞെടുപ്പ് (std::tolower(*cp)) {
                അവസ്ഥ '0': setBit(obit++, 0); നിർത്തുക;
                അവസ്ഥ '1': setBit(obit++, 1); നിർത്തുക;
                അവസ്ഥ 'z':
                അവസ്ഥ '?': setBit(obit++, 'z'); നിർത്തുക;
                അവസ്ഥ 'x': setBit(obit++, 'x'); നിർത്തുക;
                അവസ്ഥ '_': നിർത്തുക;
                സ്ഥിരസ്ഥിതി: v3error("Illegal character in binary constant: " << *cp);
                }
                നിർത്തുക;
            }

            അവസ്ഥ 'o':
            അവസ്ഥ 'c': {
                base_align = 3;
                തിരഞ്ഞെടുപ്പ് (std::tolower(*cp)) {  // clang-format off
                അവസ്ഥ '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  നിർത്തുക;
                അവസ്ഥ '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  നിർത്തുക;
                അവസ്ഥ '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  നിർത്തുക;
                അവസ്ഥ '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  നിർത്തുക;
                അവസ്ഥ '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  നിർത്തുക;
                അവസ്ഥ '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  നിർത്തുക;
                അവസ്ഥ '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  നിർത്തുക;
                അവസ്ഥ '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  നിർത്തുക;
                അവസ്ഥ 'z':
                അവസ്ഥ '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); നിർത്തുക;
                അവസ്ഥ 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); നിർത്തുക;
                അവസ്ഥ '_': നിർത്തുക;
                // clang-format on
                സ്ഥിരസ്ഥിതി: v3error("Illegal character in octal constant");
                }
                നിർത്തുക;
            }

            അവസ്ഥ 'h':  // FALLTHRU
            അവസ്ഥ 'x': {
                base_align = 4;
                തിരഞ്ഞെടുപ്പ് (std::tolower(*cp)) {  // clang-format off
                അവസ്ഥ '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); നിർത്തുക;
                അവസ്ഥ '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); നിർത്തുക;
                അവസ്ഥ 'z':
                അവസ്ഥ '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); നിർത്തുക;
                അവസ്ഥ 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); നിർത്തുക;
                    // clang-format on
                അവസ്ഥ '_': നിർത്തുക;
                സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    നിർത്തുക;
                }  // LCOV_EXCL_STOP
                നിർത്തുക;
            }
            സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                നിർത്തുക;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    എങ്കിൽ (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), അസത്യം);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    ആകുമ്പോൾ (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(സത്യം);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

ശൂന്യം V3_സംഖ്യ::warnTooMany(സ്ഥിരം string& മൂല്യം) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << മൂല്യം << "'");
}

ശൂന്യം V3_സംഖ്യ::nodep(Ast_കണ്ണി* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    എങ്കിൽ (!nodep) തിരികെ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

തർക്കം V3_സംഖ്യ::epsilonEqual(ഇരട്ട a, ഇരട്ട b) {
    തിരികെ std::fabs(a - b)
           <= (std::numeric_limits<ഇരട്ട>::epsilon() * std::max(1.0, std::max(a, b)));
}

തർക്കം V3_സംഖ്യ::epsilonIntegral(ഇരട്ട a) {
    സ്ഥിരം ഇരട്ട dint = static_cast<ഇരട്ട>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    തിരികെ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

പൂർണ്ണസംഖ്യ V3_സംഖ്യ::log2b(uint32_t അക്കം) {
    // See also opCLog2
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 31; bit > 0; bit--) {
        എങ്കിൽ (അക്കം & (1ULL << bit)) തിരികെ bit;
    }
    തിരികെ 0;
}

പൂർണ്ണസംഖ്യ V3_സംഖ്യ::log2bQuad(uint64_t അക്കം) {
    എങ്കിൽ (അക്കം >> 32ULL) തിരികെ 32 + log2b(അക്കം >> 32ULL);
    തിരികെ log2b(അക്കം);
}

//======================================================================
// Setters

V3_സംഖ്യ& V3_സംഖ്യ::setZero() {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, 0};
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setQuad(uint64_t മൂല്യം) {
    setZero();
    m_data.അക്കം()[0].m_മൂല്യം = മൂല്യം & 0xffffffffULL;
    എങ്കിൽ (width() > 32) m_data.അക്കം()[1].m_മൂല്യം = (മൂല്യം >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setLong(uint32_t മൂല്യം) {
    setZero();
    m_data.അക്കം()[0].m_മൂല്യം = മൂല്യം;
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setLongS(int32_t മൂല്യം) {
    setZero();
    സംഘം {
        uint32_t u;
        int32_t s;
    } u;
    u.s = മൂല്യം;
    (ശൂന്യം)u.s;
    m_data.അക്കം()[0].m_മൂല്യം = u.u;
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setDouble(ഇരട്ട മൂല്യം) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    സംഘം {
        ഇരട്ട d;
        uint32_t u[2];
    } u;
    u.d = മൂല്യം;
    (ശൂന്യം)u.d;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 2; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, 0};
    m_data.അക്കം()[0].m_മൂല്യം = u.u[0];
    m_data.അക്കം()[1].m_മൂല്യം = u.u[1];
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setSingleBits(അക്ഷരം മൂല്യം) {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1 /*upper*/; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, 0};
    m_data.അക്കം()[0] = {(മൂല്യം == '1' || മൂല്യം == 'x' || മൂല്യം == 1 || മൂല്യം == 3),
                       (മൂല്യം == 'z' || മൂല്യം == 'x' || മൂല്യം == 2 || മൂല്യം == 3)};
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::setAllBits0() {
    setZero();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setAllBits1() {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {~0U, 0};
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {~0U, ~0U};
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setAllBitsZ() {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, ~0U};
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::setAllBitsXRemoved() {
    എങ്കിൽ (!v3Global.constRemoveXs()) {
        തിരികെ setAllBitsX();
    } അല്ലെങ്കിൽ {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        എങ്കിൽ (v3Global.opt.xAssign() == "1") {
            തിരികെ setAllBits1();
        } അല്ലെങ്കിൽ {
            തിരികെ setAllBits0();
        }
    }
}
V3_സംഖ്യ& V3_സംഖ്യ::setValue1() {
    m_data.അക്കം()[0] = {1, 0};
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, 0};
    തിരികെ *ഇത്;
}

ശൂന്യം V3_സംഖ്യ::setBitX0(പൂർണ്ണസംഖ്യ bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_സംഖ്യ& V3_സംഖ്യ::setMask(പൂർണ്ണസംഖ്യ nbits, പൂർണ്ണസംഖ്യ lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    തിരികെ *ഇത്;
}

//======================================================================
// ACCESSORS - as strings

string V3_സംഖ്യ::ascii(തർക്കം prefixed, തർക്കം cleanVerilog) സ്ഥിരം VL_MT_STABLE {
    // Correct number of zero bits/width matters
    എങ്കിൽ (is1Step()) തിരികെ "1step";
    എങ്കിൽ (isNull()) തിരികെ "null";
    std::ostringstream out;
    എങ്കിൽ (isDouble()) {
        out.precision(17);
        എങ്കിൽ (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } അല്ലെങ്കിൽ {
            out << toDouble();
            എങ്കിൽ (toDouble() == floor(toDouble())) out << ".0";
        }
        തിരികെ out.str();
    } അല്ലെങ്കിൽ എങ്കിൽ (isString()) {
        തിരികെ '"' + toString() + '"';
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (VL_UNCOVERABLE((m_data.അക്കം()[വാക്കുകൾ() - 1].m_മൂല്യം | m_data.അക്കം()[വാക്കുകൾ() - 1].m_മൂല്യം_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    എങ്കിൽ (prefixed) {
        എങ്കിൽ (sized()) {
            out << width() << "'";
        } അല്ലെങ്കിൽ എങ്കിൽ (autoExtend() && width() == 1) {
            out << "'";
            എങ്കിൽ (bitIs0(0)) {
                out << '0';
            } അല്ലെങ്കിൽ എങ്കിൽ (bitIs1(0)) {
                out << '1';
            } അല്ലെങ്കിൽ എങ്കിൽ (bitIsZ(0)) {
                out << 'z';
            } അല്ലെങ്കിൽ {
                out << 'x';
            }
            തിരികെ out.str();
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (cleanVerilog) {
                out << "'";
            } അല്ലെങ്കിൽ {
                out << "?" << width() << "?";
            }
        }
        എങ്കിൽ (isSigned()) out << "s";
    }

    സ്ഥിരം തർക്കം ദ്വിമാന = (isFourState()
#ifdef V3_സംഖ്യ_ASCII_ദ്വിമാന
                         // cppcheck-suppress konwnConditionTrueFalse
                         || സത്യം
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    എങ്കിൽ (ദ്വിമാന) {
        out << "b";
        out << displayed("%0b");
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    എങ്കിൽ (isNull()) {
        എങ്കിൽ (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } അല്ലെങ്കിൽ {
            out << " [null]";
        }
    }
    തിരികെ out.str();
}

തർക്കം V3_സംഖ്യ::displayedFmtHasArg(അക്ഷരം format, തർക്കം isScan) {
    // Is this a format letter that takes an argument in runtime?
    (ശൂന്യം)isScan;
    തിരഞ്ഞെടുപ്പ് (std::tolower(format)) {
    അവസ്ഥ 'b': തിരികെ സത്യം;  //ദ്വിമാന
    അവസ്ഥ 'c': തിരികെ സത്യം;  // Character
    അവസ്ഥ 'd': തിരികെ സത്യം;  // Decimal
    അവസ്ഥ 'e': തിരികെ സത്യം;  // Floating
    അവസ്ഥ 'f': തിരികെ സത്യം;  // Floating
    അവസ്ഥ 'g': തിരികെ സത്യം;  // Floating
    അവസ്ഥ 'h': തിരികെ സത്യം;  // Hex
    അവസ്ഥ 'o': തിരികെ സത്യം;  // Octal
    അവസ്ഥ 'p': തിരികെ സത്യം;  // Pattern
    അവസ്ഥ 's': തിരികെ സത്യം;  // String
    അവസ്ഥ 't': തിരികെ സത്യം;  // Time
    അവസ്ഥ 'u': തിരികെ സത്യം;  // Packed 2-state
    അവസ്ഥ 'v': തിരികെ സത്യം;  // Strength
    അവസ്ഥ 'x': തിരികെ സത്യം;  // Hex
    അവസ്ഥ 'z': തിരികെ സത്യം;  // Packed 4-state
    അവസ്ഥ '?': തിരികെ സത്യം;  // Internal: Compute format in V3Width based on data type
    സ്ഥിരസ്ഥിതി: തിരികെ അസത്യം;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_സംഖ്യ::displayPad(size_t fmtsize, അക്ഷരം pad, തർക്കം ഇടത്, സ്ഥിരം string& in) VL_PURE {
    string padding;
    എങ്കിൽ (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    തിരികെ ഇടത് ? (in + padding) : (padding + in);
}

string V3_സംഖ്യ::displayedEnumName(സ്ഥിരം AstEnumItem* itemp) {
    സ്ഥിരം string name = VIdProtect::protect(itemp->prettyName());
    തിരികെ VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned അക്ഷരം>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_സംഖ്യ::displayedEnumName(സ്ഥിരം AstEnumDType* dtypep) സ്ഥിരം VL_MT_STABLE {
    V3_സംഖ്യ മൂല്യം{ഇത്, dtypep->width()};
    മൂല്യം.opAssign(*ഇത്);
    വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(itemp->valuep(), Const);
        എങ്കിൽ (!constp->അക്കം().isAnyXZ() && മൂല്യം.isCaseEq(constp->അക്കം()))
            തിരികെ displayedEnumName(itemp);
    }
    തിരികെ "";
}

string V3_സംഖ്യ::displayedEnum(സ്ഥിരം AstSFormatArg* argp,
                               സ്ഥിരം string& vformat) സ്ഥിരം VL_MT_STABLE {
    സ്ഥിരം AstEnumDType* സ്ഥിരം dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_സംഖ്യ മൂല്യം{ഇത്, dtypep->width()};
    മൂല്യം.opAssign(*ഇത്);
    സ്ഥിരം VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    സ്ഥിരം അക്ഷരം fmt = std::tolower(vformat.back());
    എങ്കിൽ (fmt != 'p' && fmt != 's') തിരികെ മൂല്യം.displayed(argp, vformat, numericAttr);
    string name = മൂല്യം.displayedEnumName(dtypep);
    എങ്കിൽ (name.empty()) name = മൂല്യം.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_സംഖ്യ name_അക്കം{ഇത്};
    name_അക്കം.setString(name);
    തിരികെ name_അക്കം.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_സംഖ്യ::displayed(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& vformat,
                           സ്ഥിരം VFormatAttr& formatAttr) സ്ഥിരം VL_MT_STABLE {
    തിരികെ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_സംഖ്യ::displayed(File_വരി* fl, സ്ഥിരം string& vformat,
                           സ്ഥിരം VFormatAttr& formatAttr) സ്ഥിരം VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *ഇത്);
    ++pos;
    തർക്കം ഇടത് = അസത്യം;
    എങ്കിൽ (pos[0] == '-') {
        ഇടത് = സത്യം;
        ++pos;
    }
    string fmtsize;
    വേണ്ടി (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    അക്ഷരം fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    എങ്കിൽ (formatAttr.isComplex()) {
        എങ്കിൽ (fmt != 'p') fmt = 's';  // override
    } അല്ലെങ്കിൽ എങ്കിൽ (formatAttr.isString()) {
        എങ്കിൽ (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_സംഖ്യ strwide{fl, static_cast<പൂർണ്ണസംഖ്യ>(toString().length()) * 8, 0};
            strwide.opNToI(*ഇത്);
            തിരികെ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        എങ്കിൽ (fmt != 'p') fmt = 's';  // override
    } അല്ലെങ്കിൽ എങ്കിൽ (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        എങ്കിൽ (fmt == 'p') {
            എങ്കിൽ (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } അല്ലെങ്കിൽ {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        എങ്കിൽ (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_സംഖ്യ nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*ഇത്);
            തിരികെ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    തിരഞ്ഞെടുപ്പ് (fmt) {
    അവസ്ഥ 'c': {
        എങ്കിൽ (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        സ്ഥിരം unsigned പൂർണ്ണസംഖ്യ v = bits_മൂല്യം(0, 8);
        str.തള്ളുക_back(static_cast<അക്ഷരം>(v));
        തിരികെ str;
    }
    അവസ്ഥ 'e':  // FALLTHRU
    അവസ്ഥ 'f':  // FALLTHRU
    അവസ്ഥ 'g': {
        സ്ഥിരം ഇരട്ട n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        അക്ഷരം tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (ശൂന്യം)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        തിരികെ tmp;
    }
    അവസ്ഥ 's': {
        എങ്കിൽ (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } അല്ലെങ്കിൽ {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            പൂർണ്ണസംഖ്യ bit = width() - 1;
            തർക്കം start = സത്യം;
            ആകുമ്പോൾ ((bit % 8) != 7) ++bit;
            വേണ്ടി (; bit >= 0; bit -= 8) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ v = bits_മൂല്യം(bit - 7, 8);
                എങ്കിൽ (!start || v) {
                    str += static_cast<അക്ഷരം>((v == 0) ? ' ' : v);
                    start = അസത്യം;  // Drop leading 0s
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (fmtsize != "0") str += ' ';
                }
            }
        }
        സ്ഥിരം size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ഇടത്, str);
        തിരികെ str;
    }
    അവസ്ഥ 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        എങ്കിൽ (formatAttr.isDouble()) {
            സ്ഥിരം ഇരട്ട n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            അക്ഷരം tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (ശൂന്യം)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            തിരികെ tmp;
        }
        എങ്കിൽ (formatAttr.isString())
            തിരികെ '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        എങ്കിൽ (formatAttr.isComplex()) തിരികെ toString();
        തിരികെ "%p";
    }
    അവസ്ഥ 'b':  // FALLTHRU
    അവസ്ഥ 'o':  // FALLTHRU
    അവസ്ഥ 'h':  // FALLTHRU
    അവസ്ഥ 'x': {
        പൂർണ്ണസംഖ്യ bit = width() - 1;
        എങ്കിൽ (ഇടത് || !fmtsize.empty()) {
            ആകുമ്പോൾ (bit && bitIs0(bit)) --bit;
        }
        തിരഞ്ഞെടുപ്പ് (fmt) {
        അവസ്ഥ 'b': {
            വേണ്ടി (; bit >= 0; --bit) {
                എങ്കിൽ (bitIs0(bit)) {
                    str += '0';
                } അല്ലെങ്കിൽ എങ്കിൽ (bitIs1(bit)) {
                    str += '1';
                } അല്ലെങ്കിൽ എങ്കിൽ (bitIsZ(bit)) {
                    str += 'z';
                } അല്ലെങ്കിൽ {
                    str += 'x';
                }
            }
            നിർത്തുക;
        }
        അവസ്ഥ 'o': {
            ആകുമ്പോൾ ((bit % 3) != 2) ++bit;
            വേണ്ടി (; bit > 0; bit -= 3) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_X = എണ്ണം_X(bit - 2, 3);
                സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_Z = എണ്ണം_Z(bit - 2, 3);
                എങ്കിൽ (അക്കം_X == 3 || അക്കം_X == width() - (bit - 2)) {
                    str += 'x';
                    തുടരുക;
                }
                എങ്കിൽ (അക്കം_Z == 3 || അക്കം_Z == width() - (bit - 2)) {
                    str += 'z';
                    തുടരുക;
                }
                എങ്കിൽ (അക്കം_X > 0) {
                    str += 'X';
                    തുടരുക;
                }
                എങ്കിൽ (അക്കം_Z > 0) {
                    str += 'Z';
                    തുടരുക;
                }
                സ്ഥിരം പൂർണ്ണസംഖ്യ v = bits_മൂല്യം(bit - 2, 3);
                str += static_cast<അക്ഷരം>('0' + v);
            }
            നിർത്തുക;
        }
        സ്ഥിരസ്ഥിതി: {  // h/x
            ആകുമ്പോൾ ((bit % 4) != 3) ++bit;
            വേണ്ടി (; bit > 0; bit -= 4) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_X = എണ്ണം_X(bit - 3, 4);
                സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_Z = എണ്ണം_Z(bit - 3, 4);
                എങ്കിൽ (അക്കം_X == 4 || അക്കം_X == width() - (bit - 3)) {
                    str += 'x';
                    തുടരുക;
                }
                എങ്കിൽ (അക്കം_Z == 4 || അക്കം_Z == width() - (bit - 3)) {
                    str += 'z';
                    തുടരുക;
                }
                എങ്കിൽ (അക്കം_X > 0) {
                    str += 'X';
                    തുടരുക;
                }
                എങ്കിൽ (അക്കം_Z > 0) {
                    str += 'Z';
                    തുടരുക;
                }
                സ്ഥിരം പൂർണ്ണസംഖ്യ v = bits_മൂല്യം(bit - 3, 4);
                എങ്കിൽ (v >= 10) {
                    str += static_cast<അക്ഷരം>('a' + v - 10);
                } അല്ലെങ്കിൽ {
                    str += static_cast<അക്ഷരം>('0' + v);
                }
            }
            നിർത്തുക;
        }
        }  // switch
        സ്ഥിരം size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ഇടത് ? ' ' : '0'), ഇടത്, str);
        തിരികെ str;
    }  // case b/d/x/o
    അവസ്ഥ 't':  // Time
    അവസ്ഥ 'd': {  // Unsigned decimal
        സ്ഥിരം തർക്കം issigned = formatAttr.isSigned();
        എങ്കിൽ (fmtsize == "" && !ഇടത്) {
            സ്ഥിരം ഇരട്ട mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            ഇരട്ട dchars = mantissabits / 3.321928094887362 + 1.0;
            എങ്കിൽ (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<പൂർണ്ണസംഖ്യ>(dchars));
        }
        തർക്കം hasXZ = അസത്യം;
        എങ്കിൽ (isAllX()) {
            str = "x";
            hasXZ = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (isAllZ()) {
            str = "z";
            hasXZ = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (isAnyX()) {
            str = "X";
            hasXZ = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (isAnyZ()) {
            str = "Z";
            hasXZ = സത്യം;
        }
        എങ്കിൽ (!hasXZ) {
            എങ്കിൽ (issigned) {
                എങ്കിൽ (width() > 64) {
                    str = toDecimalS();
                } അല്ലെങ്കിൽ {
                    str = cvtToStr(toSQuad());
                }
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (width() > 64) {
                    str = toDecimalU();
                } അല്ലെങ്കിൽ {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        സ്ഥിരം തർക്കം zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !ഇടത്;
        // fmtsize might have changed since we parsed the %fmtsize
        സ്ഥിരം size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ഇടത്, str);
        തിരികെ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    അവസ്ഥ 'u': {  // Packed 2-state
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
            സ്ഥിരം uint32_t v = m_data.അക്കം()[i].m_മൂല്യം;
            str += static_cast<അക്ഷരം>((v >> 0) & 0xff);
            str += static_cast<അക്ഷരം>((v >> 8) & 0xff);
            str += static_cast<അക്ഷരം>((v >> 16) & 0xff);
            str += static_cast<അക്ഷരം>((v >> 24) & 0xff);
        }
        തിരികെ str;
    }
    അവസ്ഥ 'z': {  // Packed 4-state
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
            സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം >> 0) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം >> 8) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം >> 16) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം >> 24) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം_X >> 0) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം_X >> 8) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം_X >> 16) & 0xff);
            str += static_cast<അക്ഷരം>((v.m_മൂല്യം_X >> 24) & 0xff);
        }
        തിരികെ str;
    }
    അവസ്ഥ 'v': {  // Strength
        പൂർണ്ണസംഖ്യ bit = width() - 1;
        വേണ്ടി (; bit >= 0; bit--) {
            എങ്കിൽ (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            അല്ലെങ്കിൽ എങ്കിൽ (bitIs1(bit)) {
                str += "St1 ";
            } അല്ലെങ്കിൽ എങ്കിൽ (bitIsZ(bit)) {
                str += "StZ ";
            } അല്ലെങ്കിൽ {
                str += "StX";
            }
        }
        തിരികെ str;
    }
    സ്ഥിരസ്ഥിതി: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_സംഖ്യ::emitC() സ്ഥിരം VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    അക്ഷരം sbuf[bufsize];
    string result;
    എങ്കിൽ (isNull()) {
        തിരികെ "VlNull{}";
    } അല്ലെങ്കിൽ എങ്കിൽ (isDouble()) {
        സ്ഥിരം ഇരട്ട dnum = toDouble();
        എങ്കിൽ (std::isinf(dnum)) {
            എങ്കിൽ (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } അല്ലെങ്കിൽ എങ്കിൽ (std::isnan(dnum)) {
            എങ്കിൽ (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } അല്ലെങ്കിൽ {
            സ്ഥിരം അക്ഷരം* സ്ഥിരം fmt = (static_cast<പൂർണ്ണസംഖ്യ>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (ശൂന്യം)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            തിരികെ sbuf;
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (isString()) {
        സ്ഥിരം string strg = toString();
        സ്ഥിരം string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        തിരികെ '"' + quoted + "\"s";
    } അല്ലെങ്കിൽ എങ്കിൽ (വാക്കുകൾ() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(വാക്കുകൾ()) + ">{{";
        എങ്കിൽ (വാക്കുകൾ() > 4) result += '\n';
        വേണ്ടി (പൂർണ്ണസംഖ്യ n = 0; n < വാക്കുകൾ(); ++n) {
            എങ്കിൽ (n) result += ((n % 4) ? ", " : ",\n");
            (ശൂന്യം)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        എങ്കിൽ (വാക്കുകൾ() > 4) result += '\n';
        result += "}}";
    } അല്ലെങ്കിൽ എങ്കിൽ (വാക്കുകൾ() == 2) {  // Quad
        സ്ഥിരം uint64_t qnum = static_cast<uint64_t>(toUQuad());
        സ്ഥിരം അക്ഷരം* സ്ഥിരം fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ശൂന്യം)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        തിരികെ sbuf;
    } അല്ലെങ്കിൽ {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        സ്ഥിരം uint32_t unum = toUInt();
        സ്ഥിരം അക്ഷരം* സ്ഥിരം fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ശൂന്യം)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        തിരികെ sbuf;
    }
    തിരികെ result;
}

string V3_സംഖ്യ::toDecimalS() സ്ഥിരം VL_MT_STABLE {
    // Correct number of zero bits/width matters
    എങ്കിൽ (isNegative()) {
        V3_സംഖ്യ lhsNoSign = *ഇത്;
        lhsNoSign.opNegate(*ഇത്);
        തിരികെ "-"s + lhsNoSign.toDecimalU();
    } അല്ലെങ്കിൽ {
        തിരികെ toDecimalU();
    }
}

string V3_സംഖ്യ::toDecimalU() സ്ഥിരം VL_MT_STABLE {
    സ്ഥിരം പൂർണ്ണസംഖ്യ maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_സംഖ്യ bcd(ഇത്, maxdecwidth + 4);
    V3_സംഖ്യ tmp(ഇത്, maxdecwidth + 4);
    V3_സംഖ്യ tmp2(ഇത്, maxdecwidth + 4);

    പൂർണ്ണസംഖ്യ from_bit = width() - 1;
    // Skip all leading zeros
    വേണ്ടി (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    വേണ്ടി (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        വേണ്ടി (പൂർണ്ണസംഖ്യ nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            എങ്കിൽ (bcd.bits_മൂല്യം(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_സംഖ്യ(ഇത്, 32, 1));
        // bcd[0] = this[from_bit]
        എങ്കിൽ (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    പൂർണ്ണസംഖ്യ lsb = (maxdecwidth - 1) & ~3;
    വേണ്ടി (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        എങ്കിൽ (bcd.bits_മൂല്യം(lsb, 4)) നിർത്തുക;
    }
    വേണ്ടി (; lsb >= 0; lsb -= 4) {
        output += static_cast<അക്ഷരം>('0' + bcd.bits_മൂല്യം(lsb, 4));  // 0..9
    }
    തിരികെ output;
}

string V3_സംഖ്യ::toRealString() സ്ഥിരം VL_MT_STABLE {
    സ്ഥിരം ഇരട്ട മൂല്യം = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << മൂല്യം;
    string result = out.str();
    എങ്കിൽ (std::strtod(result.c_str(), nullptr) != മൂല്യം) {
        out.str("");
        out.precision(17);
        out << മൂല്യം;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    എങ്കിൽ (result.find_first_of(".en") == string::npos) result += ".0";
    തിരികെ result;
}

//======================================================================
// ACCESSORS - as numbers

തർക്കം V3_സംഖ്യ::fitsInUInt() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isFourState()) തിരികെ അസത്യം;
    // We allow wide numbers that represent values <= 32 bits
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i < വാക്കുകൾ(); ++i) {
        എങ്കിൽ (m_data.അക്കം()[i].m_മൂല്യം) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}
uint32_t V3_സംഖ്യ::toUInt() സ്ഥിരം VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *ഇത്);
    // We allow wide numbers that represent values <= 32 bits
    എങ്കിൽ (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *ഇത്);
    }
    തിരികെ m_data.അക്കം()[0].m_മൂല്യം;
}

ഇരട്ട V3_സംഖ്യ::toDouble() സ്ഥിരം VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    സംഘം {
        ഇരട്ട d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.അക്കം()[0].m_മൂല്യം;
    u.u[1] = m_data.അക്കം()[1].m_മൂല്യം;
    തിരികെ u.d;
}

int32_t V3_സംഖ്യ::toSInt() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isSigned()) {
        സ്ഥിരം uint32_t v = toUInt();
        സ്ഥിരം uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        സ്ഥിരം uint32_t extended = v | signExtend;
        തിരികെ static_cast<int32_t>(extended);
    } അല്ലെങ്കിൽ {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        തിരികെ static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_സംഖ്യ::toUQuad() സ്ഥിരം VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *ഇത്);
    // We allow wide numbers that represent values <= 64 bits
    എങ്കിൽ (isDouble()) തിരികെ static_cast<uint64_t>(toDouble());
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 2; i < വാക്കുകൾ(); ++i) {
        എങ്കിൽ (m_data.അക്കം()[i].m_മൂല്യം) {
            v3error("Value too wide for 64-bits expected in this context " << *ഇത്);
            നിർത്തുക;
        }
    }
    എങ്കിൽ (width() <= 32) തിരികെ (static_cast<uint64_t>(toUInt()));
    തിരികെ ((static_cast<uint64_t>(m_data.അക്കം()[1].m_മൂല്യം) << 32ULL)
            | (static_cast<uint64_t>(m_data.അക്കം()[0].m_മൂല്യം)));
}

int64_t V3_സംഖ്യ::toSQuad() സ്ഥിരം {
    എങ്കിൽ (isDouble()) തിരികെ static_cast<int64_t>(toDouble());
    സ്ഥിരം uint64_t v = toUQuad();
    സ്ഥിരം uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    സ്ഥിരം uint64_t extended = v | signExtend;
    തിരികെ static_cast<int64_t>(extended);
}

string V3_സംഖ്യ::toString() സ്ഥിരം {
    UASSERT(!isFourState(), "toString with 4-state " << *ഇത്);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    എങ്കിൽ (isString()) തിരികെ m_data.str();
    പൂർണ്ണസംഖ്യ bit = width() - 1;
    ആകുമ്പോൾ ((bit % 8) != 7) ++bit;
    std::string str;
    വേണ്ടി (; bit >= 0; bit -= 8) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ v = bits_മൂല്യം(bit - 7, 8);
        എങ്കിൽ (v) str += static_cast<അക്ഷരം>(v);
    }
    തിരികെ str;
}

uint32_t V3_സംഖ്യ::edataWord(പൂർണ്ണസംഖ്യ eword) സ്ഥിരം {
    UASSERT(!isFourState(), "edataWord with 4-state " << *ഇത്);
    തിരികെ m_data.അക്കം()[eword].m_മൂല്യം;
}

uint8_t V3_സംഖ്യ::dataByte(പൂർണ്ണസംഖ്യ byte) സ്ഥിരം {
    തിരികെ (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

തർക്കം V3_സംഖ്യ::isAllZ() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isDouble() || isString()) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        സ്ഥിരം uint32_t mask = i == വാക്കുകൾ() - 1 ? hiWordMask() : ~0U;
        എങ്കിൽ (((~v.m_മൂല്യം & v.m_മൂല്യം_X) & mask) != mask) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}
തർക്കം V3_സംഖ്യ::isAllX() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isDouble() || isString()) തിരികെ അസത്യം;
    uint32_t mask = hiWordMask();
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = വാക്കുകൾ() - 1; i >= 0; --i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        എങ്കിൽ ((v.m_മൂല്യം & v.m_മൂല്യം_X) ^ mask) തിരികെ അസത്യം;
        mask = ~0U;
    }
    തിരികെ സത്യം;
}
തർക്കം V3_സംഖ്യ::isEqZero() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isString()) തിരികെ m_data.str().empty();
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        എങ്കിൽ (v.m_മൂല്യം || v.m_മൂല്യം_X) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}
തർക്കം V3_സംഖ്യ::isNeqZero() സ്ഥിരം {
    എങ്കിൽ (isString()) തിരികെ !m_data.str().empty();
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        എങ്കിൽ (v.m_മൂല്യം & ~v.m_മൂല്യം_X) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}
തർക്കം V3_സംഖ്യ::isBitsZero(പൂർണ്ണസംഖ്യ msb, പൂർണ്ണസംഖ്യ lsb) സ്ഥിരം {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = lsb; i <= msb; ++i) {
        എങ്കിൽ (VL_UNLIKELY(!bitIs0(i))) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}
തർക്കം V3_സംഖ്യ::isEqOne() സ്ഥിരം {
    എങ്കിൽ (m_data.അക്കം()[0].m_മൂല്യം != 1 || m_data.അക്കം()[0].m_മൂല്യം_X) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i < വാക്കുകൾ(); ++i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        എങ്കിൽ (v.m_മൂല്യം || v.m_മൂല്യം_X) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}
തർക്കം V3_സംഖ്യ::isEqAllOnes(പൂർണ്ണസംഖ്യ optwidth) സ്ഥിരം {
    // Correct number of zero bits/width matters
    എങ്കിൽ (!optwidth) optwidth = width();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < optwidth; ++bit) {
        എങ്കിൽ (!bitIs1(bit)) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}
തർക്കം V3_സംഖ്യ::isFourState() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isDouble() || isString()) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        എങ്കിൽ (m_data.അക്കം()[i].m_മൂല്യം_X) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}
തർക്കം V3_സംഖ്യ::isAnyX() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isDouble() || isString()) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        എങ്കിൽ (v.m_മൂല്യം & v.m_മൂല്യം_X) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}
തർക്കം V3_സംഖ്യ::isAnyXZ() സ്ഥിരം {
    എങ്കിൽ (isDouble() || isString()) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        എങ്കിൽ (m_data.അക്കം()[i].m_മൂല്യം_X) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}
തർക്കം V3_സംഖ്യ::isAnyZ() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (isDouble() || isString()) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
        എങ്കിൽ (~v.m_മൂല്യം & v.m_മൂല്യം_X) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}
തർക്കം V3_സംഖ്യ::isLtXZ(സ്ഥിരം V3_സംഖ്യ& rhs) സ്ഥിരം {
    // Include X/Z in comparisons for sort ordering
    എങ്കിൽ (isString()) തിരികെ toString() < rhs.toString();
    എങ്കിൽ (isDouble()) തിരികെ toDouble() < rhs.toDouble();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        എങ്കിൽ (bitIs1(bit) && rhs.bitIs0(bit)) തിരികെ സത്യം;
        എങ്കിൽ (rhs.bitIs1(bit) && bitIs0(bit)) തിരികെ അസത്യം;
        എങ്കിൽ (bitIsXZ(bit)) തിരികെ സത്യം;
        എങ്കിൽ (rhs.bitIsXZ(bit)) തിരികെ അസത്യം;
    }
    തിരികെ അസത്യം;
}
പൂർണ്ണസംഖ്യ V3_സംഖ്യ::എണ്ണം_X(പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ nbits) സ്ഥിരം VL_MT_SAFE {
    പൂർണ്ണസംഖ്യ എണ്ണം = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bitn = 0; bitn < nbits; ++bitn) {
        എങ്കിൽ (lsb + bitn >= width()) തിരികെ എണ്ണം;
        എങ്കിൽ (bitIsX(lsb + bitn)) ++എണ്ണം;
    }
    തിരികെ എണ്ണം;
}
പൂർണ്ണസംഖ്യ V3_സംഖ്യ::എണ്ണം_Z(പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ nbits) സ്ഥിരം VL_MT_SAFE {
    പൂർണ്ണസംഖ്യ എണ്ണം = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bitn = 0; bitn < nbits; ++bitn) {
        എങ്കിൽ (lsb + bitn >= width()) തിരികെ എണ്ണം;
        എങ്കിൽ (bitIsZ(lsb + bitn)) ++എണ്ണം;
    }
    തിരികെ എണ്ണം;
}

പൂർണ്ണസംഖ്യ V3_സംഖ്യ::widthToFit() സ്ഥിരം {
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = width() - 1; bit > 0; bit--) {
        എങ്കിൽ (!bitIs0(bit)) തിരികെ bit + 1;
    }
    തിരികെ 1;  // one bit even if number is == 0
}

uint32_t V3_സംഖ്യ::എണ്ണം_Bits(സ്ഥിരം V3_സംഖ്യ& ctrl) സ്ഥിരം {
    // Correct number of zero bits/width matters
    പൂർണ്ണസംഖ്യ n = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        തിരഞ്ഞെടുപ്പ് (ctrl.bitIs(0)) {
        അവസ്ഥ '0':
            എങ്കിൽ (bitIs0(bit)) ++n;
            നിർത്തുക;
        അവസ്ഥ '1':
            എങ്കിൽ (bitIs1(bit)) ++n;
            നിർത്തുക;
        അവസ്ഥ 'x':
            എങ്കിൽ (bitIsX(bit)) ++n;
            നിർത്തുക;
        അവസ്ഥ 'z':
            എങ്കിൽ (bitIsZ(bit)) ++n;
            നിർത്തുക;
        }
    }
    തിരികെ n;
}

uint32_t V3_സംഖ്യ::എണ്ണം_Bits(സ്ഥിരം V3_സംഖ്യ& ctrl1, സ്ഥിരം V3_സംഖ്യ& ctrl2,
                             സ്ഥിരം V3_സംഖ്യ& ctrl3) സ്ഥിരം {
    // Correct number of zero bits/width matters
    പൂർണ്ണസംഖ്യ n = എണ്ണം_Bits(ctrl1);
    എങ്കിൽ (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += എണ്ണം_Bits(ctrl2);
    എങ്കിൽ ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += എണ്ണം_Bits(ctrl3);
    }
    തിരികെ n;
}

uint32_t V3_സംഖ്യ::എണ്ണം_Ones() സ്ഥിരം {
    പൂർണ്ണസംഖ്യ n = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (bitIs1(bit)) ++n;
    }
    തിരികെ n;
}

uint32_t V3_സംഖ്യ::mostSetBitP1() സ്ഥിരം {
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = width() - 1; bit >= 0; bit--) {
        എങ്കിൽ (!bitIs0(bit)) തിരികെ bit + 1;
    }
    തിരികെ 0;
}

uint32_t V3_സംഖ്യ::leastSetBitP1() സ്ഥിരം {
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (!bitIs0(bit)) തിരികെ bit + 1;
    }
    തിരികെ 0;
}

//======================================================================

V3_സംഖ്യ& V3_സംഖ്യ::opBitsNonXZ(സ്ഥിരം V3_സംഖ്യ& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opBitsOne(സ്ഥിരം V3_സംഖ്യ& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opBitsXZ(സ്ഥിരം V3_സംഖ്യ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opBitsZ(സ്ഥിരം V3_സംഖ്യ& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    തിരികെ *ഇത്;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_സംഖ്യ& V3_സംഖ്യ::opRedOr(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // op i, 1 bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    അക്ഷരം outc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < lhs.width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            തിരികെ setSingleBits(1);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(bit)) {
        } അല്ലെങ്കിൽ {
            outc = 'x';
        }
    }
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opRedAnd(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    അക്ഷരം outc = 1;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < lhs.width(); ++bit) {
        എങ്കിൽ (lhs.bitIs0(bit)) {
            തിരികെ setSingleBits(0);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs1(bit)) {
        } അല്ലെങ്കിൽ {
            outc = 'x';
        }
    }
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opRedXor(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // op i, 1 bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    അക്ഷരം outc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < lhs.width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            എങ്കിൽ (outc == 1) {
                outc = 0;
            } അല്ലെങ്കിൽ എങ്കിൽ (outc == 0) {
                outc = 1;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(bit)) {
        } അല്ലെങ്കിൽ {
            outc = 'x';
        }
    }
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::op_എണ്ണം_Bits(സ്ഥിരം V3_സംഖ്യ& വ്യഞ്ജകം, സ്ഥിരം V3_സംഖ്യ& ctrl1, സ്ഥിരം V3_സംഖ്യ& ctrl2,
                                സ്ഥിരം V3_സംഖ്യ& ctrl3) {
    // Correct number of zero bits/width matters
    അക്കം_ASSERT_OP_ARGS4(വ്യഞ്ജകം, ctrl1, ctrl2, ctrl3);
    അക്കം_ASSERT_LOGIC_ARGS4(വ്യഞ്ജകം, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.അക്കം()[0].m_മൂല്യം = വ്യഞ്ജകം.എണ്ണം_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::op_എണ്ണം_Ones(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    എങ്കിൽ (lhs.isFourState()) തിരികെ setAllBitsX();
    setZero();
    m_data.അക്കം()[0].m_മൂല്യം = lhs.എണ്ണം_Ones();
    opCleanThis();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opIsUnknown(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    തിരികെ setSingleBits(lhs.isAnyXZ());
}
V3_സംഖ്യ& V3_സംഖ്യ::opOneHot(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    എങ്കിൽ (lhs.isFourState()) തിരികെ setAllBitsX();
    തിരികെ setSingleBits(lhs.എണ്ണം_Ones() == 1);
}
V3_സംഖ്യ& V3_സംഖ്യ::opOneHot0(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    എങ്കിൽ (lhs.isFourState()) തിരികെ setAllBitsX();
    തിരികെ setSingleBits(lhs.എണ്ണം_Ones() <= 1);
}
V3_സംഖ്യ& V3_സംഖ്യ::opCLog2(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    എങ്കിൽ (lhs.isFourState()) തിരികെ setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    സ്ഥിരം പൂർണ്ണസംഖ്യ adjust = (lhs.എണ്ണം_Ones() == 1) ? 0 : 1;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = lhs.width() - 1; bit >= 0; bit--) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            തിരികെ *ഇത്;
        }
    }
    setZero();
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opMostSetBitP1(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    എങ്കിൽ (lhs.isFourState()) തിരികെ setAllBitsX();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = lhs.width() - 1; bit >= 0; bit--) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            തിരികെ *ഇത്;
        }
    }
    setZero();
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opLogNot(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    അക്ഷരം outc = 1;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < lhs.width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            outc = 0;
            പോകുക last;
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(bit)) {
        } അല്ലെങ്കിൽ {
            outc = 'x';
        }
    }
last:
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opNot(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // Correct number of zero bits/width matters
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opAnd(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } അല്ലെങ്കിൽ {
            setBit(bit, 'x');
        }
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opOr(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } അല്ലെങ്കിൽ {
            setBit(bit, 'x');
        }
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opXor(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    തിരികെ *ഇത്;
}

ശൂന്യം V3_സംഖ്യ::fillBits(പൂർണ്ണസംഖ്യ destLsb, പൂർണ്ണസംഖ്യ width, അക്ഷരം മൂല്യം) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    എങ്കിൽ (!width) തിരികെ;
    UASSERT(width <= ഇത്->width() - destLsb, "Destination bit range exceeds number width");

    സ്ഥിരം uint32_t മൂല്യം_Word = മൂല്യം == '1' || മൂല്യം == 'x' || മൂല്യം == 1 || മൂല്യം == 3 ? ~0U : 0;
    സ്ഥിരം uint32_t xWord = മൂല്യം == 'z' || മൂല്യം == 'x' || മൂല്യം == 2 || മൂല്യം == 3 ? ~0U : 0;
    സ്ഥിരം പൂർണ്ണസംഖ്യ firstWord = VL_BITWORD_E(destLsb);
    സ്ഥിരം പൂർണ്ണസംഖ്യ lastWord = VL_BITWORD_E(destLsb + width - 1);
    വേണ്ടി (പൂർണ്ണസംഖ്യ word = firstWord; word <= lastWord; ++word) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        സ്ഥിരം പൂർണ്ണസംഖ്യ wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        സ്ഥിരം uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        മൂല്യം_And_X& dest = m_data.അക്കം()[word];
        dest.m_മൂല്യം = (dest.m_മൂല്യം & ~mask) | (മൂല്യം_Word & mask);
        dest.m_മൂല്യം_X = (dest.m_മൂല്യം_X & ~mask) | (xWord & mask);
    }
}

ശൂന്യം V3_സംഖ്യ::copyBits(പൂർണ്ണസംഖ്യ destLsb, സ്ഥിരം V3_സംഖ്യ& source, പൂർണ്ണസംഖ്യ sourceLsb, പൂർണ്ണസംഖ്യ width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    എങ്കിൽ (!width) തിരികെ;
    UASSERT(width <= ഇത്->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    സ്ഥിരം പൂർണ്ണസംഖ്യ firstWord = VL_BITWORD_E(destLsb);
    സ്ഥിരം പൂർണ്ണസംഖ്യ lastWord = VL_BITWORD_E(destLsb + width - 1);
    വേണ്ടി (പൂർണ്ണസംഖ്യ destWord = firstWord; destWord <= lastWord; ++destWord) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        സ്ഥിരം പൂർണ്ണസംഖ്യ destOffset = VL_BITBIT_E(destBit);
        സ്ഥിരം പൂർണ്ണസംഖ്യ sourceBit = sourceLsb + destBit - destLsb;
        സ്ഥിരം പൂർണ്ണസംഖ്യ sourceWord = VL_BITWORD_E(sourceBit);
        സ്ഥിരം പൂർണ്ണസംഖ്യ sourceOffset = VL_BITBIT_E(sourceBit);
        സ്ഥിരം പൂർണ്ണസംഖ്യ copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        സ്ഥിരം uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        സ്ഥിരം മൂല്യം_And_X& sourceLo = source.m_data.അക്കം()[sourceWord];
        uint32_t മൂല്യം = sourceLo.m_മൂല്യം >> sourceOffset;
        uint32_t മൂല്യം_X = sourceLo.m_മൂല്യം_X >> sourceOffset;
        എങ്കിൽ (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            സ്ഥിരം മൂല്യം_And_X& sourceHi = source.m_data.അക്കം()[sourceWord + 1];
            മൂല്യം |= sourceHi.m_മൂല്യം << (VL_EDATASIZE - sourceOffset);
            മൂല്യം_X |= sourceHi.m_മൂല്യം_X << (VL_EDATASIZE - sourceOffset);
        }

        മൂല്യം_And_X& dest = m_data.അക്കം()[destWord];
        dest.m_മൂല്യം = (dest.m_മൂല്യം & ~mask) | ((മൂല്യം << destOffset) & mask);
        dest.m_മൂല്യം_X = (dest.m_മൂല്യം_X & ~mask) | ((മൂല്യം_X << destOffset) & mask);
    }
}

V3_സംഖ്യ& V3_സംഖ്യ::opConcat(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Correct number of zero bits/width matters
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    എങ്കിൽ (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opLenN(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opRepl(സ്ഥിരം V3_സംഖ്യ& lhs,
                           സ്ഥിരം V3_സംഖ്യ& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    തിരികെ opRepl(lhs, rhs.toUInt());
}

V3_സംഖ്യ& V3_സംഖ്യ::opRepl(സ്ഥിരം V3_സംഖ്യ& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    എങ്കിൽ (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    വേണ്ടി (unsigned times = 0; times < rhsval; ++times) {
        സ്ഥിരം uint64_t destLsb = uint64_t{times} * lhs.width();
        എങ്കിൽ (destLsb >= static_cast<uint32_t>(width())) നിർത്തുക;
        copyBits(static_cast<പൂർണ്ണസംഖ്യ>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<പൂർണ്ണസംഖ്യ>(destLsb)));
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opStreamL(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    എങ്കിൽ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    സ്ഥിരം പൂർണ്ണസംഖ്യ ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    വേണ്ടി (പൂർണ്ണസംഖ്യ istart = 0; istart < lhs.width(); istart += ssize) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opLogAnd(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    അക്ഷരം loutc = 0;
    അക്ഷരം routc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < lhs.width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            loutc = 1;
            നിർത്തുക;
        }
        എങ്കിൽ (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < rhs.width(); ++bit) {
        എങ്കിൽ (rhs.bitIs1(bit)) {
            routc = 1;
            നിർത്തുക;
        }
        എങ്കിൽ (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    അക്ഷരം outc = 'x';
    എങ്കിൽ (routc == 1 && loutc == 1) outc = 1;
    എങ്കിൽ (routc == 0 || loutc == 0) outc = 0;
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opLogOr(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    അക്ഷരം outc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < lhs.width(); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit)) {
            outc = 1;
            പോകുക last;
        }
        എങ്കിൽ (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < rhs.width(); ++bit) {
        എങ്കിൽ (rhs.bitIs1(bit)) {
            outc = 1;
            പോകുക last;
        }
        എങ്കിൽ (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opLogIf(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    V3_സംഖ്യ lnot = lhs;
    lnot.opLogNot(lhs);
    തിരികെ opLogOr(lnot, rhs);
}

V3_സംഖ്യ& V3_സംഖ്യ::opLogEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    V3_സംഖ്യ ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_സംഖ്യ ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    തിരികെ opLogAnd(ifa, ifb);
}

V3_സംഖ്യ& V3_സംഖ്യ::opAtoN(സ്ഥിരം V3_സംഖ്യ& lhs, പൂർണ്ണസംഖ്യ base) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    എങ്കിൽ (base == AstAtoN::ATOREAL) തിരികെ setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    എങ്കിൽ (errno != 0) v = 0;
    തിരികെ setLongS(static_cast<int32_t>(v));
}

V3_സംഖ്യ& V3_സംഖ്യ::opPutcN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, സ്ഥിരം V3_സംഖ്യ& ths) {
    അക്കം_ASSERT_OP_ARGS3(lhs, rhs, ths);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    സ്ഥിരം int32_t i = rhs.toSInt();
    സ്ഥിരം int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    എങ്കിൽ (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    തിരികെ setString(lstring);
}

V3_സംഖ്യ& V3_സംഖ്യ::opGetcN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    സ്ഥിരം string lstring = lhs.toString();
    സ്ഥിരം int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    എങ്കിൽ (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    തിരികെ setLong(v);
}

V3_സംഖ്യ& V3_സംഖ്യ::opSubstrN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, സ്ഥിരം V3_സംഖ്യ& ths) {
    അക്കം_ASSERT_OP_ARGS3(lhs, rhs, ths);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    സ്ഥിരം string lstring = lhs.toString();
    സ്ഥിരം int32_t i = rhs.toSInt();
    സ്ഥിരം int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    എങ്കിൽ (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) തിരികെ setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    തിരികെ setString(lstring.substr(i, j - i + 1));
}

V3_സംഖ്യ& V3_സംഖ്യ::opCompareNN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, തർക്കം ignoreCase) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    പൂർണ്ണസംഖ്യ result;
    സ്ഥിരം string lstring = lhs.toString();
    സ്ഥിരം string rstring = rhs.toString();
    എങ്കിൽ (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } അല്ലെങ്കിൽ {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    തിരികെ setLongS(result);
}

V3_സംഖ്യ& V3_സംഖ്യ::opEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isString()) തിരികെ opEqN(lhs, rhs);
    എങ്കിൽ (lhs.isDouble()) തിരികെ opEqD(lhs, rhs);
    അക്ഷരം outc = 1;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            പോകുക last;
        }
        എങ്കിൽ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            പോകുക last;
        }
        എങ്കിൽ (lhs.bitIsXZ(bit)) outc = 'x';
        എങ്കിൽ (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opNeq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isString()) തിരികെ opNeqN(lhs, rhs);
    എങ്കിൽ (lhs.isDouble()) തിരികെ opNeqD(lhs, rhs);
    അക്ഷരം outc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            പോകുക last;
        }
        എങ്കിൽ (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            പോകുക last;
        }
        എങ്കിൽ (lhs.bitIsXZ(bit)) outc = 'x';
        എങ്കിൽ (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    തിരികെ setSingleBits(outc);
}

V3Hash V3_സംഖ്യ::toHash() സ്ഥിരം {
    // Shouldmatch isCaseEq
    എങ്കിൽ (isString()) തിരികെ V3Hash{toString()};
    എങ്കിൽ (isDouble()) തിരികെ V3Hash{toDouble()};
    V3Hash hash{width()};
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) hash += m_data.അക്കം()[i].m_മൂല്യം;
    തിരികെ hash;
}

തർക്കം V3_സംഖ്യ::isCaseEq(സ്ഥിരം V3_സംഖ്യ& rhs) സ്ഥിരം {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    എങ്കിൽ (isString()) തിരികെ toString() == rhs.toString();
    എങ്കിൽ (isDouble()) തിരികെ toDouble() == rhs.toDouble();
    എങ്കിൽ (width() != rhs.width()) തിരികെ അസത്യം;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
        എങ്കിൽ (!(m_data.അക്കം()[i] == rhs.m_data.അക്കം()[i])) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}

V3_സംഖ്യ& V3_സംഖ്യ::opCaseEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_സംഖ്യ& V3_സംഖ്യ::opCaseNeq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്ഷരം outc = 0;
    എങ്കിൽ (lhs.isString()) {
        തിരികെ opNeqN(lhs, rhs);
    } അല്ലെങ്കിൽ എങ്കിൽ (lhs.isDouble()) {
        തിരികെ opNeqD(lhs, rhs);
    }
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        എങ്കിൽ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            പോകുക last;
        }
    }
last:
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opWildEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    അക്ഷരം outc = 1;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        എങ്കിൽ (!rhs.bitIsXZ(bit)) {
            എങ്കിൽ (lhs.bitIsXZ(bit)) {
                outc = 'x';
                പോകുക last;
            }
            എങ്കിൽ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                പോകുക last;
            }
        }
    }
last:
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opWildNeq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    അക്ഷരം outc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        എങ്കിൽ (!rhs.bitIsXZ(bit)) {
            എങ്കിൽ (lhs.bitIsXZ(bit)) {
                outc = 'x';
                പോകുക last;
            }
            എങ്കിൽ (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                പോകുക last;
            }
        }
    }
last:
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opGt(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    അക്ഷരം outc = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        എങ്കിൽ (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        എങ്കിൽ (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        എങ്കിൽ (lhs.bitIsXZ(bit)) outc = 'x';
        എങ്കിൽ (rhs.bitIsXZ(bit)) outc = 'x';
    }
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opGtS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    അക്ഷരം outc = 0;
    {
        സ്ഥിരം പൂർണ്ണസംഖ്യ mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        എങ്കിൽ (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } അല്ലെങ്കിൽ എങ്കിൽ (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } അല്ലെങ്കിൽ {
            // both positive or negative, normal >
            വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                എങ്കിൽ (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                എങ്കിൽ (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                എങ്കിൽ (lhs.bitIsXZ(bit)) outc = 'x';
                എങ്കിൽ (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    തിരികെ setSingleBits(outc);
}

V3_സംഖ്യ& V3_സംഖ്യ::opGte(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    V3_സംഖ്യ& eq = opEq(lhs, rhs);
    എങ്കിൽ (eq.isNeqZero()) തിരികെ eq;  // Return true
    തിരികെ opGt(lhs, rhs);
}

V3_സംഖ്യ& V3_സംഖ്യ::opGteS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    V3_സംഖ്യ& eq = opEq(lhs, rhs);
    എങ്കിൽ (eq.isNeqZero()) തിരികെ eq;  // Return true
    തിരികെ opGtS(lhs, rhs);
}

V3_സംഖ്യ& V3_സംഖ്യ::opLt(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { തിരികെ opGt(rhs, lhs); }
V3_സംഖ്യ& V3_സംഖ്യ::opLte(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { തിരികെ opGte(rhs, lhs); }
V3_സംഖ്യ& V3_സംഖ്യ::opLtS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { തിരികെ opGtS(rhs, lhs); }
V3_സംഖ്യ& V3_സംഖ്യ::opLteS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) { തിരികെ opGteS(rhs, lhs); }

V3_സംഖ്യ& V3_സംഖ്യ::opShiftR(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (rhs.isFourState()) തിരികെ setAllBitsX();
    setZero();
    എങ്കിൽ (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) തിരികെ *ഇത്;
    സ്ഥിരം uint32_t rhsval = rhs.toUInt();
    എങ്കിൽ (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<പൂർണ്ണസംഖ്യ>(rhsval),
                 std::min(width(), lhs.width() - static_cast<പൂർണ്ണസംഖ്യ>(rhsval)));
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opShiftRS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (rhs.isFourState()) തിരികെ setAllBitsX();
    setZero();
    സ്ഥിരം തർക്കം overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    എങ്കിൽ (!overflow) {
        സ്ഥിരം uint32_t rhsval = rhs.toUInt();
        എങ്കിൽ (rhsval < lbits) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ copyWidth = std::min(width(), static_cast<പൂർണ്ണസംഖ്യ>(lbits - rhsval));
            copyBits(0, lhs, static_cast<പൂർണ്ണസംഖ്യ>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            തിരികെ *ഇത്;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opShiftL(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (rhs.isFourState()) തിരികെ setAllBitsX();
    setZero();
    എങ്കിൽ (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) തിരികെ *ഇത്;
    സ്ഥിരം uint32_t rhsval = rhs.toUInt();
    എങ്കിൽ (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<പൂർണ്ണസംഖ്യ>(rhsval), lhs, 0,
                 std::min(width() - static_cast<പൂർണ്ണസംഖ്യ>(rhsval), lhs.width()));
    }
    തിരികെ *ഇത്;
}

//======================================================================
// Ops - Arithmetic

V3_സംഖ്യ& V3_സംഖ്യ::opNegate(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    എങ്കിൽ (lhs.isFourState()) തിരികെ setAllBitsX();
    V3_സംഖ്യ notlhs(&lhs, width());
    notlhs.opNot(lhs);
    സ്ഥിരം V3_സംഖ്യ one(&lhs, width(), 1);
    opAdd(notlhs, one);
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opAdd(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ word = 0; word < വാക്കുകൾ(); ++word) {
        സ്ഥിരം uint64_t lwordval = lhs.m_data.അക്കം()[word].m_മൂല്യം;
        സ്ഥിരം uint64_t rwordval = rhs.m_data.അക്കം()[word].m_മൂല്യം;
        സ്ഥിരം uint64_t sum = lwordval + rwordval + carry;
        m_data.അക്കം()[word].m_മൂല്യം = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opSub(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    V3_സംഖ്യ negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    തിരികെ opAdd(lhs, negrhs);
}
V3_സംഖ്യ& V3_സംഖ്യ::opMul(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    setZero();
    എങ്കിൽ (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } അല്ലെങ്കിൽ {
        വേണ്ടി (പൂർണ്ണസംഖ്യ lword = 0; lword < lhs.വാക്കുകൾ(); ++lword) {
            സ്ഥിരം uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.അക്കം()[lword].m_മൂല്യം);
            എങ്കിൽ (lwordval == 0) തുടരുക;
            വേണ്ടി (പൂർണ്ണസംഖ്യ rword = 0; rword < rhs.വാക്കുകൾ(); ++rword) {
                സ്ഥിരം uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.അക്കം()[rword].m_മൂല്യം);
                എങ്കിൽ (rwordval == 0) തുടരുക;
                uint64_t mul = lwordval * rwordval;
                വേണ്ടി (പൂർണ്ണസംഖ്യ qword = lword + rword; qword < വാക്കുകൾ(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.അക്കം()[qword].m_മൂല്യം);
                    m_data.അക്കം()[qword].m_മൂല്യം = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    എങ്കിൽ (mul == 0) നിർത്തുക;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opMulS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    V3_സംഖ്യ lhsNoSign = lhs;
    എങ്കിൽ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_സംഖ്യ rhsNoSign = rhs;
    എങ്കിൽ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    സ്ഥിരം V3_സംഖ്യ qNoSign = opMul(lhsNoSign, rhsNoSign);
    എങ്കിൽ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } അല്ലെങ്കിൽ {
        opAssign(qNoSign);
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opDiv(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    എങ്കിൽ (rhs.isEqZero()) തിരികെ setAllBitsXRemoved();
    എങ്കിൽ (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        തിരികെ *ഇത്;
    } അല്ലെങ്കിൽ {
        // Wide division
        തിരികെ opModDivGuts(lhs, rhs, അസത്യം);
    }
}
V3_സംഖ്യ& V3_സംഖ്യ::opDivS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    എങ്കിൽ (rhs.isEqZero()) തിരികെ setAllBitsXRemoved();
    V3_സംഖ്യ lhsNoSign = lhs;
    എങ്കിൽ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_സംഖ്യ rhsNoSign = rhs;
    എങ്കിൽ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    സ്ഥിരം V3_സംഖ്യ qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    എങ്കിൽ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } അല്ലെങ്കിൽ {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *ഇത്);
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opModDiv(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    എങ്കിൽ (rhs.isEqZero()) തിരികെ setAllBitsXRemoved();
    എങ്കിൽ (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        തിരികെ *ഇത്;
    } അല്ലെങ്കിൽ {
        // Wide modulus
        തിരികെ opModDivGuts(lhs, rhs, സത്യം);
    }
}
V3_സംഖ്യ& V3_സംഖ്യ::opModDivS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    എങ്കിൽ (rhs.isEqZero()) തിരികെ setAllBitsXRemoved();
    V3_സംഖ്യ lhsNoSign = lhs;
    എങ്കിൽ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_സംഖ്യ rhsNoSign = rhs;
    എങ്കിൽ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    സ്ഥിരം V3_സംഖ്യ qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    എങ്കിൽ (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } അല്ലെങ്കിൽ {
        opAssign(qNoSign);
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opModDivGuts(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, തർക്കം is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    സ്ഥിരം പൂർണ്ണസംഖ്യ l_വാക്കുകൾ = lhs.വാക്കുകൾ();
    സ്ഥിരം പൂർണ്ണസംഖ്യ umsbp1 = lhs.mostSetBitP1();  // dividend
    സ്ഥിരം പൂർണ്ണസംഖ്യ vmsbp1 = rhs.mostSetBitP1();  //ഹാരകം
    എങ്കിൽ (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *ഇത്);
        തിരികെ *ഇത്;
    }

    സ്ഥിരം പൂർണ്ണസംഖ്യ uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    സ്ഥിരം പൂർണ്ണസംഖ്യ vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    എങ്കിൽ (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        വേണ്ടി (പൂർണ്ണസംഖ്യ j = uw - 1; j >= 0; j--) {
            സ്ഥിരം uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.അക്കം()[j].m_മൂല്യം));
            m_data.അക്കം()[j].m_മൂല്യം = unw64 / static_cast<uint64_t>(rhs.m_data.അക്കം()[0].m_മൂല്യം);
            k = unw64
                - (static_cast<uint64_t>(m_data.അക്കം()[j].m_മൂല്യം)
                   * static_cast<uint64_t>(rhs.m_data.അക്കം()[0].m_മൂല്യം));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *ഇത് << " rem=0x" << std::hex
                                   << k << std::dec);
        എങ്കിൽ (is_modulus) {
            setZero();
            m_data.അക്കം()[0].m_മൂല്യം = k;
        }
        opCleanThis();
        തിരികെ *ഇത്;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_വാക്കുകൾ + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_വാക്കുകൾ + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < l_വാക്കുകൾ; ++i) m_data.അക്കം()[i].m_മൂല്യം = 0;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < l_വാക്കുകൾ + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    സ്ഥിരം പൂർണ്ണസംഖ്യ s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    സ്ഥിരം uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.അക്കം()[i].m_മൂല്യം << s)
                | (shift_mask & (rhs.m_data.അക്കം()[i - 1].m_മൂല്യം >> (32 - s)));
    }
    vn[0] = rhs.m_data.അക്കം()[0].m_മൂല്യം << s;

    // Copy and shift dividend by same amount; may set new upper word
    എങ്കിൽ (s) {
        un[uw] = lhs.m_data.അക്കം()[uw - 1].m_മൂല്യം >> (32 - s);
    } അല്ലെങ്കിൽ {
        un[uw] = 0;
    }
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.അക്കം()[i].m_മൂല്യം << s)
                | (shift_mask & (lhs.m_data.അക്കം()[i - 1].m_മൂല്യം >> (32 - s)));
    }
    un[0] = lhs.m_data.അക്കം()[0].m_മൂല്യം << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    വേണ്ടി (പൂർണ്ണസംഖ്യ j = uw - vw; j >= 0; j--) {
        // Estimate
        സ്ഥിരം uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        എങ്കിൽ (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            എങ്കിൽ (rhat < 0x100000000ULL) പോകുക again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < vw; ++i) {
            സ്ഥിരം uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.അക്കം()[j].m_മൂല്യം = qhat;  // Save quotient digit

        എങ്കിൽ (t < 0) {
            // Over subtracted; correct by adding back
            m_data.അക്കം()[j].m_മൂല്യം--;
            k = 0;
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < vw; ++i) {
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

    എങ്കിൽ (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < vw; ++i) {
            m_data.അക്കം()[i].m_മൂല്യം = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = vw; i < l_വാക്കുകൾ; ++i) m_data.അക്കം()[i].m_മൂല്യം = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *ഇത്);
        തിരികെ *ഇത്;
    } അല്ലെങ്കിൽ {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *ഇത്);
        തിരികെ *ഇത്;
    }
}

V3_സംഖ്യ& V3_സംഖ്യ::opPow(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, തർക്കം lsign, തർക്കം rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_LOGIC_ARGS2(lhs, rhs);
    എങ്കിൽ (lhs.isFourState() || rhs.isFourState()) തിരികെ setAllBitsX();
    എങ്കിൽ (rhs.isEqZero()) തിരികെ setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    എങ്കിൽ (rsign && rhs.isNegative()) {
        എങ്കിൽ (lhs.isEqZero()) {
            തിരികെ setAllBitsXRemoved();
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.isEqOne()) {
            തിരികെ setValue1();
        } അല്ലെങ്കിൽ എങ്കിൽ (lsign && lhs.isEqAllOnes()) {
            എങ്കിൽ (rhs.bitIs1(0)) {
                തിരികെ setAllBits1();  // -1^odd=-1
            } അല്ലെങ്കിൽ {
                തിരികെ setValue1();  // -1^even=1
            }
        }
        തിരികെ setZero();
    }
    എങ്കിൽ (lhs.isEqZero()) തിരികെ setZero();
    setZero();
    m_data.അക്കം()[0].m_മൂല്യം = 1;
    V3_സംഖ്യ power(&lhs, width());
    power.opAssign(lhs);
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < rhs.width(); ++bit) {
        എങ്കിൽ (bit > 0) {  // power = power*power
            V3_സംഖ്യ lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        എങ്കിൽ (rhs.bitIs1(bit)) {  // out *= power
            V3_സംഖ്യ lastOut(&lhs, width());
            lastOut.opAssign(*ഇത്);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opPowSU(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Correct number of zero bits/width matters
    തിരികെ opPow(lhs, rhs, സത്യം, അസത്യം);
}
V3_സംഖ്യ& V3_സംഖ്യ::opPowSS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Correct number of zero bits/width matters
    തിരികെ opPow(lhs, rhs, സത്യം, സത്യം);
}
V3_സംഖ്യ& V3_സംഖ്യ::opPowUS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // Correct number of zero bits/width matters
    തിരികെ opPow(lhs, rhs, അസത്യം, സത്യം);
}

V3_സംഖ്യ& V3_സംഖ്യ::opBufIf1(സ്ഥിരം V3_സംഖ്യ& ens, സ്ഥിരം V3_സംഖ്യ& if1s) {
    അക്കം_ASSERT_OP_ARGS2(ens, if1s);
    അക്കം_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit < width(); ++bit) {
        എങ്കിൽ (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } അല്ലെങ്കിൽ {
            setBit(bit, 'z');
        }
    }
    തിരികെ *ഇത്;
}

// Sets all bits in range to the given value
V3_സംഖ്യ& V3_സംഖ്യ::opSetRange(uint32_t lsb, uint32_t width, അക്ഷരം bit_മൂല്യം) {
    എങ്കിൽ (lsb < static_cast<uint32_t>(ഇത്->width())) {
        fillBits(static_cast<പൂർണ്ണസംഖ്യ>(lsb),
                 static_cast<പൂർണ്ണസംഖ്യ>(std::min(width, static_cast<uint32_t>(ഇത്->width()) - lsb)),
                 bit_മൂല്യം);
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opAssign(സ്ഥിരം V3_സംഖ്യ& lhs) { തിരികെ opAssignNonXZ(lhs, അസത്യം); }
V3_സംഖ്യ& V3_സംഖ്യ::opAssignNonXZ(സ്ഥിരം V3_സംഖ്യ& lhs, തർക്കം ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    എങ്കിൽ (ഇത് != &lhs) {
        എങ്കിൽ (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (isString()) {
            എങ്കിൽ (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } അല്ലെങ്കിൽ {
                m_data.str() = lhs.m_data.str();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } അല്ലെങ്കിൽ എങ്കിൽ (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } അല്ലെങ്കിൽ എങ്കിൽ (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } അല്ലെങ്കിൽ {
            വേണ്ടി (പൂർണ്ണസംഖ്യ word = 0; word < വാക്കുകൾ(); ++word) {
                സ്ഥിരം uint32_t മൂല്യം = word < lhs.വാക്കുകൾ() ? lhs.m_data.അക്കം()[word].m_മൂല്യം
                                                                & ~lhs.m_data.അക്കം()[word].m_മൂല്യം_X
                                                          : 0;
                m_data.അക്കം()[word] = {മൂല്യം, 0};
            }
            opCleanThis();
        }
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opNToI(സ്ഥിരം V3_സംഖ്യ& lhs) {
    // String to packed number
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    setZero();
    സ്ഥിരം string& str = lhs.toString();
    വേണ്ടി (size_t n = 0; n < str.length(); ++n) {
        സ്ഥിരം അക്ഷരം c = str[str.length() - 1 - n];
        വേണ്ടി (size_t cbit = 0; cbit < 8; ++cbit) {
            സ്ഥിരം std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opExtendS(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    സ്ഥിരം പൂർണ്ണസംഖ്യ copyWidth = std::min(width(), static_cast<പൂർണ്ണസംഖ്യ>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opExtendXZ(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    സ്ഥിരം പൂർണ്ണസംഖ്യ copyWidth = std::min(width(), static_cast<പൂർണ്ണസംഖ്യ>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opClean(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t bits) { തിരികെ opSel(lhs, bits - 1, 0); }

ശൂന്യം V3_സംഖ്യ::opCleanThis(തർക്കം warnOnTruncation) {
    // Clean MSB of number
    അക്കം_ASSERT_LOGIC_ARGS1(*ഇത്);
    സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[വാക്കുകൾ() - 1];
    സ്ഥിരം uint32_t new_മൂല്യം_Msb = v.m_മൂല്യം & hiWordMask();
    സ്ഥിരം uint32_t new_മൂല്യം_XMsb = v.m_മൂല്യം_X & hiWordMask();
    എങ്കിൽ (warnOnTruncation && (new_മൂല്യം_Msb != v.m_മൂല്യം || new_മൂല്യം_XMsb != v.m_മൂല്യം_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.അക്കം()[വാക്കുകൾ() - 1] = {new_മൂല്യം_Msb, new_മൂല്യം_XMsb};
}

V3_സംഖ്യ& V3_സംഖ്യ::opSel(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& msb, സ്ഥിരം V3_സംഖ്യ& lsb) {
    അക്കം_ASSERT_OP_ARGS3(lhs, msb, lsb);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS2(msb, lsb);
    എങ്കിൽ (lsb.isFourState() || msb.isFourState()) തിരികെ setAllBitsX();
    തിരികെ opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_സംഖ്യ& V3_സംഖ്യ::opSel(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t msbval, uint32_t lsbval) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    പൂർണ്ണസംഖ്യ copyWidth = 0;
    എങ്കിൽ (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        സ്ഥിരം uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<പൂർണ്ണസംഖ്യ>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<പൂർണ്ണസംഖ്യ>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    തിരികെ *ഇത്;
}

V3_സംഖ്യ& V3_സംഖ്യ::opSelInto(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& lsb, പൂർണ്ണസംഖ്യ width) {
    തിരികെ opSelInto(lhs, lsb.toSInt(), width);
}

V3_സംഖ്യ& V3_സംഖ്യ::opSelInto(സ്ഥിരം V3_സംഖ്യ& lhs, പൂർണ്ണസംഖ്യ lsbval, പൂർണ്ണസംഖ്യ width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    സ്ഥിരം പൂർണ്ണസംഖ്യ copyWidth = std::max(0, std::min({width, lhs.width(), ഇത്->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    സ്ഥിരം പൂർണ്ണസംഖ്യ fillWidth
        = std::max(0, std::min(width - copyWidth, ഇത്->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    തിരികെ *ഇത്;
}

//======================================================================
// Ops - Floating point

V3_സംഖ്യ& V3_സംഖ്യ::opIToRD(സ്ഥിരം V3_സംഖ്യ& lhs, തർക്കം isSigned) {
    // Correct number of zero bits/width matters
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_സംഖ്യ noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    ഇരട്ട d = 0;
    സ്ഥിരം തർക്കം negate = isSigned && noxz.isNegative();
    എങ്കിൽ (negate) {
        സ്ഥിരം V3_സംഖ്യ noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    വേണ്ടി (പൂർണ്ണസംഖ്യ bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        എങ്കിൽ (noxz.bitIs1(bit)) d += exp2(bit);
    }
    എങ്കിൽ (negate) d = -d;
    തിരികെ setDouble(d);
}
V3_സംഖ്യ& V3_സംഖ്യ::opRToIS(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_DOUBLE_ARGS1(lhs);
    സ്ഥിരം ഇരട്ട v = VL_TRUNC(lhs.toDouble());
    സ്ഥിരം int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    തിരികെ setLongS(i);
}
V3_സംഖ്യ& V3_സംഖ്യ::opRToIRoundS(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_DOUBLE_ARGS1(lhs);
    സ്ഥിരം ഇരട്ട v = VL_ROUND(lhs.toDouble());
    setZero();
    സംഘം {
        ഇരട്ട d;
        uint64_t q;
    } u;
    u.d = v;
    (ശൂന്യം)u.d;

    സ്ഥിരം പൂർണ്ണസംഖ്യ exp = static_cast<പൂർണ്ണസംഖ്യ>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = exp - 52;
    സ്ഥിരം uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    എങ്കിൽ (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        വേണ്ടി (പൂർണ്ണസംഖ്യ bit = 0; bit <= 52; ++bit) {
            എങ്കിൽ (mantissa & (1ULL << bit)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ outbit = bit + lsb;
                എങ്കിൽ (outbit >= 0) setBit(outbit, 1);
            }
        }
        എങ്കിൽ (v < 0) {
            സ്ഥിരം V3_സംഖ്യ noSign = *ഇത്;
            opNegate(noSign);
        }
    }
    തിരികെ *ഇത്;
}
V3_സംഖ്യ& V3_സംഖ്യ::opRealToBits(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    സംഘം {
        ഇരട്ട m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    തിരികെ setQuad(u.m_v);
}
V3_സംഖ്യ& V3_സംഖ്യ::opBitsToRealD(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    സംഘം {
        ഇരട്ട m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    തിരികെ setDouble(u.m_d);
}
V3_സംഖ്യ& V3_സംഖ്യ::opNegateD(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_DOUBLE_ARGS1(lhs);
    തിരികെ setDouble(-lhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opAddD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opSubD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opMulD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opDivD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opPowD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_സംഖ്യ& V3_സംഖ്യ::opEqD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opNeqD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opGtD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opGteD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opLtD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_സംഖ്യ& V3_സംഖ്യ::opLteD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_സംഖ്യ& V3_സംഖ്യ::opConcatN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setString(lhs.toString() + rhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opReplN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    അക്കം_ASSERT_LOGIC_ARGS1(rhs);
    തിരികെ opReplN(lhs, rhs.toUInt());
}
V3_സംഖ്യ& V3_സംഖ്യ::opReplN(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t rhsval) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    വേണ്ടി (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    തിരികെ setString(out);
}
V3_സംഖ്യ& V3_സംഖ്യ::opCvtPackString(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    തിരികെ setString(lhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opToLowerN(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    സ്ഥിരം std::string out = VString::downcase(lhs.toString());
    തിരികെ setString(out);
}
V3_സംഖ്യ& V3_സംഖ്യ::opToUpperN(സ്ഥിരം V3_സംഖ്യ& lhs) {
    അക്കം_ASSERT_OP_ARGS1(lhs);
    അക്കം_ASSERT_STRING_ARGS1(lhs);
    സ്ഥിരം std::string out = VString::upcase(lhs.toString());
    തിരികെ setString(out);
}

V3_സംഖ്യ& V3_സംഖ്യ::opEqN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toString() == rhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opNeqN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toString() != rhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opGtN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toString() > rhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opGteN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toString() >= rhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opLtN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toString() < rhs.toString());
}
V3_സംഖ്യ& V3_സംഖ്യ::opLteN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs) {
    അക്കം_ASSERT_OP_ARGS2(lhs, rhs);
    അക്കം_ASSERT_STRING_ARGS2(lhs, rhs);
    തിരികെ setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

ശൂന്യം V3_സംഖ്യ::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_വരി* സ്ഥിരം fl = പുതിയ File_വരി{File_വരി::builtInFilename()};
    V3_സംഖ്യ അക്കം{fl, 32, 0};
    അക്കം.selfTestThis();
}

ശൂന്യം V3_സംഖ്യ::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonEqual(0, 0), സത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonEqual(1e19, 1e19), സത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonEqual(9, 0.0001), അസത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonEqual(1, 1 + std::numeric_limits<ഇരട്ട>::epsilon()), സത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonEqual(0.009, 0.00899999999999999931998839741709), സത്യം);

    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(0), സത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(1), സത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(-1), സത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(1.0001), അസത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(0.9999), അസത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(-1.0001), അസത്യം);
    UASSERT_SELFTEST(V3_സംഖ്യ::epsilonIntegral(-0.9999), അസത്യം);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
