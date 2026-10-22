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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3Number.h"

#অন্তর্ভাব "V3File.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <cerrno>
#অন্তর্ভাব <cmath>
#অন্তর্ভাব <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr পূর্ণসংখ্যা MAX_SPRINTF_DOUBLE_আকার
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define অঙ্ক_ASSERT_OP_ARGS1(arg1) \
    UASSERT((এই != &(arg1)), "Number operation called with same source and dest")
#define অঙ্ক_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((এই != &(arg1) && এই != &(arg2)), \
            "Number operation called with same source and dest")
#define অঙ্ক_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((এই != &(arg1) && এই != &(arg2) && এই != &(arg3)), \
            "Number operation called with same source and dest")
#define অঙ্ক_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((এই != &(arg1) && এই != &(arg2) && এই != &(arg3) && এই != &(arg4)), \
            "Number operation called with same source and dest")

#define অঙ্ক_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).তথ্য_Type() == V3_সংখ্যা_তথ্য::V3_সংখ্যা_তথ্য_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define অঙ্ক_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    অঙ্ক_ASSERT_LOGIC_ARGS1(arg1); \
    অঙ্ক_ASSERT_LOGIC_ARGS1(arg2)

#define অঙ্ক_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    অঙ্ক_ASSERT_LOGIC_ARGS1(arg1); \
    অঙ্ক_ASSERT_LOGIC_ARGS1(arg2); \
    অঙ্ক_ASSERT_LOGIC_ARGS1(arg3); \
    অঙ্ক_ASSERT_LOGIC_ARGS1(arg4)

#define অঙ্ক_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define অঙ্ক_ASSERT_STRING_ARGS2(arg1, arg2) \
    অঙ্ক_ASSERT_STRING_ARGS1(arg1); \
    অঙ্ক_ASSERT_STRING_ARGS1(arg2)

#define অঙ্ক_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define অঙ্ক_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    অঙ্ক_ASSERT_DOUBLE_ARGS1(arg1); \
    অঙ্ক_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

শূন্য V3_সংখ্যা::v3errorEnd(ধ্রুবক std::ostringstream& str) ধ্রুবক VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    যদি (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } অন্যথা যদি (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } অন্যথা {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

শূন্য V3_সংখ্যা::v3errorEndFatal(ধ্রুবক std::ostringstream& str) ধ্রুবক
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    প্রতিজ্ঞা(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_সংখ্যা::V3_সংখ্যা(Ast_গ্রন্থি* nodep, VerilogStringLiteral, ধ্রুবক string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    যদি (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } অন্যথা {
        init(nodep, std::max<পূর্ণসংখ্যা>(str.দৈর্ঘ্য() * 8, 1));
        জন্য (unsigned pos = 0; pos < str.দৈর্ঘ্য(); ++pos) {
            ধ্রুবক পূর্ণসংখ্যা topos = str.দৈর্ঘ্য() - 1 - pos;
            মান_And_X& v = m_তথ্য.অঙ্ক()[topos / 4];
            জন্য (পূর্ণসংখ্যা bit = 0; bit < 8; ++bit) {
                যদি (str[pos] & (1UL << bit)) v.m_মান |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_তথ্য.m_fromString = সত্য;
    opCleanThis(সত্য);
}

V3_সংখ্যা::V3_সংখ্যা(Ast_গ্রন্থি* nodep, ধ্রুবক Ast_গ্রন্থি_DType* nodedtypep) {
    যদি (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } অন্যথা যদি (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } অন্যথা যদি (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } অন্যথা {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

শূন্য V3_সংখ্যা::create(ধ্রুবক অক্ষর* sourcep) {
    m_তথ্য.setLogic();
    ধ্রুবক অক্ষর* মান_startp = sourcep;
    জন্য (ধ্রুবক অক্ষর* cp = sourcep; *cp; ++cp) {
        যদি (*cp == '\'') {
            মান_startp = cp + 1;
            বিরতি;
        }
    }

    যৌক্তিক userSized = মিথ্যা;
    যৌক্তিক unbased = মিথ্যা;
    অক্ষর base = '\0';
    যদি (মান_startp != sourcep) {  // Has a '
        string widthn;
        ধ্রুবক অক্ষর* cp = sourcep;
        জন্য (; *cp; ++cp) {
            যদি (*cp == '\'') {
                ++cp;
                বিরতি;
            }
            যদি (*cp != '_') widthn += *cp;
        }
        যখন (*cp == '_') cp++;
        যদি (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(সত্য);
        }
        যদি (*cp) {
            base = *cp;
            ++cp;
        }
        মান_startp = cp;

        userSized = widthn.দৈর্ঘ্য() != 0;
        যদি (std::atoi(widthn.c_str())) {
            যদি (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_অঙ্ক_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_অঙ্ক_Width(), সত্য);
            } অন্যথা {
                width(std::atoi(widthn.c_str()), সত্য);
            }
        }
    } অন্যথা {
        unbased = সত্য;
        base = 'd';
    }

    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    যদি (base == '0') {
        width(1, মিথ্যা);  // So we extend it
        setBit(0, 0);
        m_তথ্য.m_autoExtend = সত্য;
    } অন্যথা যদি (base == '1') {
        width(1, মিথ্যা);  // So we extend it
        setBit(0, 1);
        m_তথ্য.m_autoExtend = সত্য;
    } অন্যথা যদি (std::tolower(base) == 'z') {
        width(1, মিথ্যা);  // So we extend it
        setBit(0, 'z');
        m_তথ্য.m_autoExtend = সত্য;
    } অন্যথা যদি (std::tolower(base) == 'x') {
        width(1, মিথ্যা);  // So we extend it
        setBit(0, 'x');
        m_তথ্য.m_autoExtend = সত্য;
    }
    // Otherwise...
    অন্যথা যদি (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ধ্রুবক পূর্ণসংখ্যা widthBound = std::max<পূর্ণসংখ্যা>(32, std::strlen(মান_startp) * 4);
        width(widthBound, মিথ্যা);  // Will change width below
        যদি (unbased) isSigned(সত্য);  // Also says the spec.
    }

    // Ignore leading blanks
    যখন (*মান_startp == '_' || std::isspace(*মান_startp)) ++মান_startp;
    যদি (!*মান_startp && !m_তথ্য.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    যদি (userSized && m_তথ্য.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    পূর্ণসংখ্যা obit = 0;  // Start at LSB
    পূর্ণসংখ্যা base_align = 1;
    যদি (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        যখন (*মান_startp == '_' || *মান_startp == '0') ++মান_startp;
        // Convert decimal number to hex
        পূর্ণসংখ্যা olen = 0;
        uint32_t val = 0;
        পূর্ণসংখ্যা got_x = 0;
        পূর্ণসংখ্যা got_z = 0;
        পূর্ণসংখ্যা got_01 = 0;
        যৌক্তিক warnedTooMany = মিথ্যা;
        জন্য (ধ্রুবক অক্ষর* cp = মান_startp; *cp; ++cp) {
            নির্বাচন (std::tolower(*cp)) {
            ক্ষেত্র '0':  // FALLTHRU
            ক্ষেত্র '1':  // FALLTHRU
            ক্ষেত্র '2':  // FALLTHRU
            ক্ষেত্র '3':  // FALLTHRU
            ক্ষেত্র '4':  // FALLTHRU
            ক্ষেত্র '5':  // FALLTHRU
            ক্ষেত্র '6':  // FALLTHRU
            ক্ষেত্র '7':  // FALLTHRU
            ক্ষেত্র '8':  // FALLTHRU
            ক্ষেত্র '9': {
                যদি (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_তথ্য.অঙ্ক()[0].m_মান = val;
                } অন্যথা {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_সংখ্যা product(এই, width() + 4);  // +4 for overflow detection
                    ধ্রুবক V3_সংখ্যা ten(এই, width() + 4, 10);
                    ধ্রুবক V3_সংখ্যা addend(এই, width(), (*cp - '0'));
                    product.opMul(*এই, ten);
                    opAdd(product, addend);
                    যদি (!warnedTooMany && product.bits_মান(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = সত্য;
                    }
                }
                ++olen;
                got_01 = 1;
                বিরতি;
            }
            ক্ষেত্র 'z':
            ক্ষেত্র '?': {
                got_z = 1;
                যদি (!userSized) width(32, মিথ্যা);
                setAllBitsZ();
                বিরতি;
            }
            ক্ষেত্র 'x': {
                got_x = 1;
                যদি (!userSized) width(32, মিথ্যা);
                setAllBitsX();
                বিরতি;
            }
            ক্ষেত্র '_': বিরতি;
            পূর্বনির্ধারিত: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                বিরতি;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        যদি ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << মান_startp);
        }
    } অন্যথা {
        // Convert bin/octal number to hex
        জন্য (ধ্রুবক অক্ষর* cp = মান_startp + std::strlen(মান_startp) - 1; cp >= মান_startp;
             cp--) {
            যদি (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                বিরতি;
            }
            নির্বাচন (std::tolower(base)) {
            ক্ষেত্র 'b': {
                base_align = 1;
                নির্বাচন (std::tolower(*cp)) {
                ক্ষেত্র '0': setBit(obit++, 0); বিরতি;
                ক্ষেত্র '1': setBit(obit++, 1); বিরতি;
                ক্ষেত্র 'z':
                ক্ষেত্র '?': setBit(obit++, 'z'); বিরতি;
                ক্ষেত্র 'x': setBit(obit++, 'x'); বিরতি;
                ক্ষেত্র '_': বিরতি;
                পূর্বনির্ধারিত: v3error("Illegal character in binary constant: " << *cp);
                }
                বিরতি;
            }

            ক্ষেত্র 'o':
            ক্ষেত্র 'c': {
                base_align = 3;
                নির্বাচন (std::tolower(*cp)) {  // clang-format off
                ক্ষেত্র '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  বিরতি;
                ক্ষেত্র '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  বিরতি;
                ক্ষেত্র '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  বিরতি;
                ক্ষেত্র '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  বিরতি;
                ক্ষেত্র '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  বিরতি;
                ক্ষেত্র '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  বিরতি;
                ক্ষেত্র '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  বিরতি;
                ক্ষেত্র '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  বিরতি;
                ক্ষেত্র 'z':
                ক্ষেত্র '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); বিরতি;
                ক্ষেত্র 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); বিরতি;
                ক্ষেত্র '_': বিরতি;
                // clang-format on
                পূর্বনির্ধারিত: v3error("Illegal character in octal constant");
                }
                বিরতি;
            }

            ক্ষেত্র 'h':  // FALLTHRU
            ক্ষেত্র 'x': {
                base_align = 4;
                নির্বাচন (std::tolower(*cp)) {  // clang-format off
                ক্ষেত্র '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); বিরতি;
                ক্ষেত্র '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); বিরতি;
                ক্ষেত্র '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); বিরতি;
                ক্ষেত্র '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); বিরতি;
                ক্ষেত্র '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); বিরতি;
                ক্ষেত্র '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); বিরতি;
                ক্ষেত্র '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); বিরতি;
                ক্ষেত্র '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); বিরতি;
                ক্ষেত্র '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); বিরতি;
                ক্ষেত্র '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); বিরতি;
                ক্ষেত্র 'z':
                ক্ষেত্র '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); বিরতি;
                ক্ষেত্র 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); বিরতি;
                    // clang-format on
                ক্ষেত্র '_': বিরতি;
                পূর্বনির্ধারিত:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    বিরতি;
                }  // LCOV_EXCL_STOP
                বিরতি;
            }
            পূর্বনির্ধারিত:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                বিরতি;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    যদি (!userSized && !m_তথ্য.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), মিথ্যা);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    যখন (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(সত্য);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

শূন্য V3_সংখ্যা::warnTooMany(ধ্রুবক string& মান) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << মান << "'");
}

শূন্য V3_সংখ্যা::nodep(Ast_গ্রন্থি* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    যদি (!nodep) ফেরত;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

যৌক্তিক V3_সংখ্যা::epsilonEqual(দ্বিগুণ a, দ্বিগুণ b) {
    ফেরত std::fabs(a - b)
           <= (std::numeric_limits<দ্বিগুণ>::epsilon() * std::max(1.0, std::max(a, b)));
}

যৌক্তিক V3_সংখ্যা::epsilonIntegral(দ্বিগুণ a) {
    ধ্রুবক দ্বিগুণ dint = static_cast<দ্বিগুণ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ফেরত epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

পূর্ণসংখ্যা V3_সংখ্যা::log2b(uint32_t অঙ্ক) {
    // See also opCLog2
    জন্য (পূর্ণসংখ্যা bit = 31; bit > 0; bit--) {
        যদি (অঙ্ক & (1ULL << bit)) ফেরত bit;
    }
    ফেরত 0;
}

পূর্ণসংখ্যা V3_সংখ্যা::log2bQuad(uint64_t অঙ্ক) {
    যদি (অঙ্ক >> 32ULL) ফেরত 32 + log2b(অঙ্ক >> 32ULL);
    ফেরত log2b(অঙ্ক);
}

//======================================================================
// Setters

V3_সংখ্যা& V3_সংখ্যা::setZero() {
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, 0};
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setQuad(uint64_t মান) {
    setZero();
    m_তথ্য.অঙ্ক()[0].m_মান = মান & 0xffffffffULL;
    যদি (width() > 32) m_তথ্য.অঙ্ক()[1].m_মান = (মান >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setLong(uint32_t মান) {
    setZero();
    m_তথ্য.অঙ্ক()[0].m_মান = মান;
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setLongS(int32_t মান) {
    setZero();
    সংঘ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = মান;
    (শূন্য)u.s;
    m_তথ্য.অঙ্ক()[0].m_মান = u.u;
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setDouble(দ্বিগুণ মান) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_তথ্য.setDouble();
    সংঘ {
        দ্বিগুণ d;
        uint32_t u[2];
    } u;
    u.d = মান;
    (শূন্য)u.d;
    জন্য (পূর্ণসংখ্যা i = 2; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, 0};
    m_তথ্য.অঙ্ক()[0].m_মান = u.u[0];
    m_তথ্য.অঙ্ক()[1].m_মান = u.u[1];
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setSingleBits(অক্ষর মান) {
    জন্য (পূর্ণসংখ্যা i = 1 /*upper*/; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, 0};
    m_তথ্য.অঙ্ক()[0] = {(মান == '1' || মান == 'x' || মান == 1 || মান == 3),
                       (মান == 'z' || মান == 'x' || মান == 2 || মান == 3)};
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::setAllBits0() {
    setZero();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setAllBits1() {
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {~0U, 0};
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {~0U, ~0U};
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setAllBitsZ() {
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, ~0U};
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::setAllBitsXRemoved() {
    যদি (!v3Global.constRemoveXs()) {
        ফেরত setAllBitsX();
    } অন্যথা {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        যদি (v3Global.opt.xAssign() == "1") {
            ফেরত setAllBits1();
        } অন্যথা {
            ফেরত setAllBits0();
        }
    }
}
V3_সংখ্যা& V3_সংখ্যা::setValue1() {
    m_তথ্য.অঙ্ক()[0] = {1, 0};
    জন্য (পূর্ণসংখ্যা i = 1; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, 0};
    ফেরত *এই;
}

শূন্য V3_সংখ্যা::setBitX0(পূর্ণসংখ্যা bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_সংখ্যা& V3_সংখ্যা::setMask(পূর্ণসংখ্যা nbits, পূর্ণসংখ্যা lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    ফেরত *এই;
}

//======================================================================
// ACCESSORS - as strings

string V3_সংখ্যা::ascii(যৌক্তিক prefixed, যৌক্তিক cleanVerilog) ধ্রুবক VL_MT_STABLE {
    // Correct number of zero bits/width matters
    যদি (is1Step()) ফেরত "1step";
    যদি (isNull()) ফেরত "null";
    std::ostringstream out;
    যদি (isDouble()) {
        out.precision(17);
        যদি (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } অন্যথা {
            out << toDouble();
            যদি (toDouble() == floor(toDouble())) out << ".0";
        }
        ফেরত out.str();
    } অন্যথা যদি (isString()) {
        ফেরত '"' + toString() + '"';
    } অন্যথা {
        যদি (VL_UNCOVERABLE((m_তথ্য.অঙ্ক()[শব্দগুলি() - 1].m_মান | m_তথ্য.অঙ্ক()[শব্দগুলি() - 1].m_মান_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    যদি (prefixed) {
        যদি (sized()) {
            out << width() << "'";
        } অন্যথা যদি (autoExtend() && width() == 1) {
            out << "'";
            যদি (bitIs0(0)) {
                out << '0';
            } অন্যথা যদি (bitIs1(0)) {
                out << '1';
            } অন্যথা যদি (bitIsZ(0)) {
                out << 'z';
            } অন্যথা {
                out << 'x';
            }
            ফেরত out.str();
        } অন্যথা {
            যদি (cleanVerilog) {
                out << "'";
            } অন্যথা {
                out << "?" << width() << "?";
            }
        }
        যদি (isSigned()) out << "s";
    }

    ধ্রুবক যৌক্তিক দ্বিমিক = (isFourState()
#ifdef V3_সংখ্যা_ASCII_দ্বিমিক
                         // cppcheck-suppress konwnConditionTrueFalse
                         || সত্য
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    যদি (দ্বিমিক) {
        out << "b";
        out << displayed("%0b");
    } অন্যথা {
        যদি (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    যদি (isNull()) {
        যদি (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } অন্যথা {
            out << " [null]";
        }
    }
    ফেরত out.str();
}

যৌক্তিক V3_সংখ্যা::displayedFmtHasArg(অক্ষর format, যৌক্তিক isScan) {
    // Is this a format letter that takes an argument in runtime?
    (শূন্য)isScan;
    নির্বাচন (std::tolower(format)) {
    ক্ষেত্র 'b': ফেরত সত্য;  //দ্বিমিক
    ক্ষেত্র 'c': ফেরত সত্য;  // Character
    ক্ষেত্র 'd': ফেরত সত্য;  // Decimal
    ক্ষেত্র 'e': ফেরত সত্য;  // Floating
    ক্ষেত্র 'f': ফেরত সত্য;  // Floating
    ক্ষেত্র 'g': ফেরত সত্য;  // Floating
    ক্ষেত্র 'h': ফেরত সত্য;  // Hex
    ক্ষেত্র 'o': ফেরত সত্য;  // Octal
    ক্ষেত্র 'p': ফেরত সত্য;  // Pattern
    ক্ষেত্র 's': ফেরত সত্য;  // String
    ক্ষেত্র 't': ফেরত সত্য;  // Time
    ক্ষেত্র 'u': ফেরত সত্য;  // Packed 2-state
    ক্ষেত্র 'v': ফেরত সত্য;  // Strength
    ক্ষেত্র 'x': ফেরত সত্য;  // Hex
    ক্ষেত্র 'z': ফেরত সত্য;  // Packed 4-state
    ক্ষেত্র '?': ফেরত সত্য;  // Internal: Compute format in V3Width based on data type
    পূর্বনির্ধারিত: ফেরত মিথ্যা;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_সংখ্যা::displayPad(আকার_t fmtsize, অক্ষর pad, যৌক্তিক বাম, ধ্রুবক string& in) VL_PURE {
    string padding;
    যদি (in.দৈর্ঘ্য() < fmtsize) padding = string(fmtsize - in.দৈর্ঘ্য(), pad);
    ফেরত বাম ? (in + padding) : (padding + in);
}

string V3_সংখ্যা::displayedEnumName(ধ্রুবক AstEnumItem* itemp) {
    ধ্রুবক string name = VIdProtect::protect(itemp->prettyName());
    ফেরত VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned অক্ষর>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_সংখ্যা::displayedEnumName(ধ্রুবক AstEnumDType* dtypep) ধ্রুবক VL_MT_STABLE {
    V3_সংখ্যা মান{এই, dtypep->width()};
    মান.opAssign(*এই);
    জন্য (ধ্রুবক AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ধ্রুবক AstConst* ধ্রুবক constp = VN_AS(itemp->valuep(), Const);
        যদি (!constp->অঙ্ক().isAnyXZ() && মান.isCaseEq(constp->অঙ্ক()))
            ফেরত displayedEnumName(itemp);
    }
    ফেরত "";
}

string V3_সংখ্যা::displayedEnum(ধ্রুবক AstSFormatArg* argp,
                               ধ্রুবক string& vformat) ধ্রুবক VL_MT_STABLE {
    ধ্রুবক AstEnumDType* ধ্রুবক dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_সংখ্যা মান{এই, dtypep->width()};
    মান.opAssign(*এই);
    ধ্রুবক VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ধ্রুবক অক্ষর fmt = std::tolower(vformat.back());
    যদি (fmt != 'p' && fmt != 's') ফেরত মান.displayed(argp, vformat, numericAttr);
    string name = মান.displayedEnumName(dtypep);
    যদি (name.empty()) name = মান.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_সংখ্যা name_অঙ্ক{এই};
    name_অঙ্ক.setString(name);
    ফেরত name_অঙ্ক.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_সংখ্যা::displayed(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& vformat,
                           ধ্রুবক VFormatAttr& formatAttr) ধ্রুবক VL_MT_STABLE {
    ফেরত displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_সংখ্যা::displayed(File_সারি* fl, ধ্রুবক string& vformat,
                           ধ্রুবক VFormatAttr& formatAttr) ধ্রুবক VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *এই);
    ++pos;
    যৌক্তিক বাম = মিথ্যা;
    যদি (pos[0] == '-') {
        বাম = সত্য;
        ++pos;
    }
    string fmtsize;
    জন্য (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    অক্ষর fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    যদি (formatAttr.isComplex()) {
        যদি (fmt != 'p') fmt = 's';  // override
    } অন্যথা যদি (formatAttr.isString()) {
        যদি (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_সংখ্যা strwide{fl, static_cast<পূর্ণসংখ্যা>(toString().দৈর্ঘ্য()) * 8, 0};
            strwide.opNToI(*এই);
            ফেরত strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        যদি (fmt != 'p') fmt = 's';  // override
    } অন্যথা যদি (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        যদি (fmt == 'p') {
            যদি (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } অন্যথা {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } অন্যথা যদি (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        যদি (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_সংখ্যা nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*এই);
            ফেরত nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    নির্বাচন (fmt) {
    ক্ষেত্র 'c': {
        যদি (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ধ্রুবক unsigned পূর্ণসংখ্যা v = bits_মান(0, 8);
        str.ঠেলো_back(static_cast<অক্ষর>(v));
        ফেরত str;
    }
    ক্ষেত্র 'e':  // FALLTHRU
    ক্ষেত্র 'f':  // FALLTHRU
    ক্ষেত্র 'g': {
        ধ্রুবক দ্বিগুণ n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        অক্ষর tmp[MAX_SPRINTF_DOUBLE_আকার];
        (শূন্য)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_আকার, vformat.c_str(), n);
        ফেরত tmp;
    }
    ক্ষেত্র 's': {
        যদি (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } অন্যথা {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            পূর্ণসংখ্যা bit = width() - 1;
            যৌক্তিক start = সত্য;
            যখন ((bit % 8) != 7) ++bit;
            জন্য (; bit >= 0; bit -= 8) {
                ধ্রুবক পূর্ণসংখ্যা v = bits_মান(bit - 7, 8);
                যদি (!start || v) {
                    str += static_cast<অক্ষর>((v == 0) ? ' ' : v);
                    start = মিথ্যা;  // Drop leading 0s
                } অন্যথা {
                    যদি (fmtsize != "0") str += ' ';
                }
            }
        }
        ধ্রুবক আকার_t fmtsizen = static_cast<আকার_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', বাম, str);
        ফেরত str;
    }
    ক্ষেত্র 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        যদি (formatAttr.isDouble()) {
            ধ্রুবক দ্বিগুণ n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            অক্ষর tmp[MAX_SPRINTF_DOUBLE_আকার];
            (শূন্য)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_আকার, "%g", n);
            ফেরত tmp;
        }
        যদি (formatAttr.isString())
            ফেরত '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        যদি (formatAttr.isComplex()) ফেরত toString();
        ফেরত "%p";
    }
    ক্ষেত্র 'b':  // FALLTHRU
    ক্ষেত্র 'o':  // FALLTHRU
    ক্ষেত্র 'h':  // FALLTHRU
    ক্ষেত্র 'x': {
        পূর্ণসংখ্যা bit = width() - 1;
        যদি (বাম || !fmtsize.empty()) {
            যখন (bit && bitIs0(bit)) --bit;
        }
        নির্বাচন (fmt) {
        ক্ষেত্র 'b': {
            জন্য (; bit >= 0; --bit) {
                যদি (bitIs0(bit)) {
                    str += '0';
                } অন্যথা যদি (bitIs1(bit)) {
                    str += '1';
                } অন্যথা যদি (bitIsZ(bit)) {
                    str += 'z';
                } অন্যথা {
                    str += 'x';
                }
            }
            বিরতি;
        }
        ক্ষেত্র 'o': {
            যখন ((bit % 3) != 2) ++bit;
            জন্য (; bit > 0; bit -= 3) {
                ধ্রুবক পূর্ণসংখ্যা অঙ্ক_X = গুনতি_X(bit - 2, 3);
                ধ্রুবক পূর্ণসংখ্যা অঙ্ক_Z = গুনতি_Z(bit - 2, 3);
                যদি (অঙ্ক_X == 3 || অঙ্ক_X == width() - (bit - 2)) {
                    str += 'x';
                    চালু;
                }
                যদি (অঙ্ক_Z == 3 || অঙ্ক_Z == width() - (bit - 2)) {
                    str += 'z';
                    চালু;
                }
                যদি (অঙ্ক_X > 0) {
                    str += 'X';
                    চালু;
                }
                যদি (অঙ্ক_Z > 0) {
                    str += 'Z';
                    চালু;
                }
                ধ্রুবক পূর্ণসংখ্যা v = bits_মান(bit - 2, 3);
                str += static_cast<অক্ষর>('0' + v);
            }
            বিরতি;
        }
        পূর্বনির্ধারিত: {  // h/x
            যখন ((bit % 4) != 3) ++bit;
            জন্য (; bit > 0; bit -= 4) {
                ধ্রুবক পূর্ণসংখ্যা অঙ্ক_X = গুনতি_X(bit - 3, 4);
                ধ্রুবক পূর্ণসংখ্যা অঙ্ক_Z = গুনতি_Z(bit - 3, 4);
                যদি (অঙ্ক_X == 4 || অঙ্ক_X == width() - (bit - 3)) {
                    str += 'x';
                    চালু;
                }
                যদি (অঙ্ক_Z == 4 || অঙ্ক_Z == width() - (bit - 3)) {
                    str += 'z';
                    চালু;
                }
                যদি (অঙ্ক_X > 0) {
                    str += 'X';
                    চালু;
                }
                যদি (অঙ্ক_Z > 0) {
                    str += 'Z';
                    চালু;
                }
                ধ্রুবক পূর্ণসংখ্যা v = bits_মান(bit - 3, 4);
                যদি (v >= 10) {
                    str += static_cast<অক্ষর>('a' + v - 10);
                } অন্যথা {
                    str += static_cast<অক্ষর>('0' + v);
                }
            }
            বিরতি;
        }
        }  // switch
        ধ্রুবক আকার_t fmtsizen = static_cast<আকার_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (বাম ? ' ' : '0'), বাম, str);
        ফেরত str;
    }  // case b/d/x/o
    ক্ষেত্র 't':  // Time
    ক্ষেত্র 'd': {  // Unsigned decimal
        ধ্রুবক যৌক্তিক issigned = formatAttr.isSigned();
        যদি (fmtsize == "" && !বাম) {
            ধ্রুবক দ্বিগুণ mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            দ্বিগুণ dchars = mantissabits / 3.321928094887362 + 1.0;
            যদি (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<পূর্ণসংখ্যা>(dchars));
        }
        যৌক্তিক hasXZ = মিথ্যা;
        যদি (isAllX()) {
            str = "x";
            hasXZ = সত্য;
        } অন্যথা যদি (isAllZ()) {
            str = "z";
            hasXZ = সত্য;
        } অন্যথা যদি (isAnyX()) {
            str = "X";
            hasXZ = সত্য;
        } অন্যথা যদি (isAnyZ()) {
            str = "Z";
            hasXZ = সত্য;
        }
        যদি (!hasXZ) {
            যদি (issigned) {
                যদি (width() > 64) {
                    str = toDecimalS();
                } অন্যথা {
                    str = cvtToStr(toSQuad());
                }
            } অন্যথা {
                যদি (width() > 64) {
                    str = toDecimalU();
                } অন্যথা {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        ধ্রুবক যৌক্তিক zeropad = fmtsize.দৈর্ঘ্য() > 0 && fmtsize[0] == '0' && !বাম;
        // fmtsize might have changed since we parsed the %fmtsize
        ধ্রুবক আকার_t fmtsizen = static_cast<আকার_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), বাম, str);
        ফেরত str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    ক্ষেত্র 'u': {  // Packed 2-state
        জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
            ধ্রুবক uint32_t v = m_তথ্য.অঙ্ক()[i].m_মান;
            str += static_cast<অক্ষর>((v >> 0) & 0xff);
            str += static_cast<অক্ষর>((v >> 8) & 0xff);
            str += static_cast<অক্ষর>((v >> 16) & 0xff);
            str += static_cast<অক্ষর>((v >> 24) & 0xff);
        }
        ফেরত str;
    }
    ক্ষেত্র 'z': {  // Packed 4-state
        জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
            ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
            str += static_cast<অক্ষর>((v.m_মান >> 0) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান >> 8) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান >> 16) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান >> 24) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান_X >> 0) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান_X >> 8) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান_X >> 16) & 0xff);
            str += static_cast<অক্ষর>((v.m_মান_X >> 24) & 0xff);
        }
        ফেরত str;
    }
    ক্ষেত্র 'v': {  // Strength
        পূর্ণসংখ্যা bit = width() - 1;
        জন্য (; bit >= 0; bit--) {
            যদি (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            অন্যথা যদি (bitIs1(bit)) {
                str += "St1 ";
            } অন্যথা যদি (bitIsZ(bit)) {
                str += "StZ ";
            } অন্যথা {
                str += "StX";
            }
        }
        ফেরত str;
    }
    পূর্বনির্ধারিত: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_সংখ্যা::emitC() ধ্রুবক VL_MT_STABLE {
    constexpr আকার_t bufsize = 1000;
    অক্ষর sbuf[bufsize];
    string ফলাফল;
    যদি (isNull()) {
        ফেরত "VlNull{}";
    } অন্যথা যদি (isDouble()) {
        ধ্রুবক দ্বিগুণ dnum = toDouble();
        যদি (std::isinf(dnum)) {
            যদি (std::signbit(dnum)) ফলাফল += '-';
            ফলাফল += "std::numeric_limits<double>::infinity()";
        } অন্যথা যদি (std::isnan(dnum)) {
            যদি (std::signbit(dnum)) ফলাফল += '-';
            ফলাফল += "std::numeric_limits<double>::quiet_NaN()";
        } অন্যথা {
            ধ্রুবক অক্ষর* ধ্রুবক fmt = (static_cast<পূর্ণসংখ্যা>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (শূন্য)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ফেরত sbuf;
        }
    } অন্যথা যদি (isString()) {
        ধ্রুবক string strg = toString();
        ধ্রুবক string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ফেরত '"' + quoted + "\"s";
    } অন্যথা যদি (শব্দগুলি() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        ফলাফল = "::VlWide<" + std::to_string(শব্দগুলি()) + ">{{";
        যদি (শব্দগুলি() > 4) ফলাফল += '\n';
        জন্য (পূর্ণসংখ্যা n = 0; n < শব্দগুলি(); ++n) {
            যদি (n) ফলাফল += ((n % 4) ? ", " : ",\n");
            (শূন্য)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            ফলাফল += sbuf;
        }
        যদি (শব্দগুলি() > 4) ফলাফল += '\n';
        ফলাফল += "}}";
    } অন্যথা যদি (শব্দগুলি() == 2) {  // Quad
        ধ্রুবক uint64_t qnum = static_cast<uint64_t>(toUQuad());
        ধ্রুবক অক্ষর* ধ্রুবক fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (শূন্য)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ফেরত sbuf;
    } অন্যথা {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ধ্রুবক uint32_t unum = toUInt();
        ধ্রুবক অক্ষর* ধ্রুবক fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (শূন্য)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ফেরত sbuf;
    }
    ফেরত ফলাফল;
}

string V3_সংখ্যা::toDecimalS() ধ্রুবক VL_MT_STABLE {
    // Correct number of zero bits/width matters
    যদি (isNegative()) {
        V3_সংখ্যা lhsNoSign = *এই;
        lhsNoSign.opNegate(*এই);
        ফেরত "-"s + lhsNoSign.toDecimalU();
    } অন্যথা {
        ফেরত toDecimalU();
    }
}

string V3_সংখ্যা::toDecimalU() ধ্রুবক VL_MT_STABLE {
    ধ্রুবক পূর্ণসংখ্যা maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_সংখ্যা bcd(এই, maxdecwidth + 4);
    V3_সংখ্যা tmp(এই, maxdecwidth + 4);
    V3_সংখ্যা tmp2(এই, maxdecwidth + 4);

    পূর্ণসংখ্যা from_bit = width() - 1;
    // Skip all leading zeros
    জন্য (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    জন্য (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        জন্য (পূর্ণসংখ্যা nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            যদি (bcd.bits_মান(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_সংখ্যা(এই, 32, 1));
        // bcd[0] = this[from_bit]
        যদি (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    পূর্ণসংখ্যা lsb = (maxdecwidth - 1) & ~3;
    জন্য (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        যদি (bcd.bits_মান(lsb, 4)) বিরতি;
    }
    জন্য (; lsb >= 0; lsb -= 4) {
        output += static_cast<অক্ষর>('0' + bcd.bits_মান(lsb, 4));  // 0..9
    }
    ফেরত output;
}

string V3_সংখ্যা::toRealString() ধ্রুবক VL_MT_STABLE {
    ধ্রুবক দ্বিগুণ মান = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << মান;
    string ফলাফল = out.str();
    যদি (std::strtod(ফলাফল.c_str(), nullptr) != মান) {
        out.str("");
        out.precision(17);
        out << মান;
        ফলাফল = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    যদি (ফলাফল.find_first_of(".en") == string::npos) ফলাফল += ".0";
    ফেরত ফলাফল;
}

//======================================================================
// ACCESSORS - as numbers

যৌক্তিক V3_সংখ্যা::fitsInUInt() ধ্রুবক VL_MT_SAFE {
    যদি (isFourState()) ফেরত মিথ্যা;
    // We allow wide numbers that represent values <= 32 bits
    জন্য (পূর্ণসংখ্যা i = 1; i < শব্দগুলি(); ++i) {
        যদি (m_তথ্য.অঙ্ক()[i].m_মান) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}
uint32_t V3_সংখ্যা::toUInt() ধ্রুবক VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *এই);
    // We allow wide numbers that represent values <= 32 bits
    যদি (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *এই);
    }
    ফেরত m_তথ্য.অঙ্ক()[0].m_মান;
}

দ্বিগুণ V3_সংখ্যা::toDouble() ধ্রুবক VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    সংঘ {
        দ্বিগুণ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_তথ্য.অঙ্ক()[0].m_মান;
    u.u[1] = m_তথ্য.অঙ্ক()[1].m_মান;
    ফেরত u.d;
}

int32_t V3_সংখ্যা::toSInt() ধ্রুবক VL_MT_SAFE {
    যদি (isSigned()) {
        ধ্রুবক uint32_t v = toUInt();
        ধ্রুবক uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        ধ্রুবক uint32_t extended = v | signExtend;
        ফেরত static_cast<int32_t>(extended);
    } অন্যথা {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ফেরত static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_সংখ্যা::toUQuad() ধ্রুবক VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *এই);
    // We allow wide numbers that represent values <= 64 bits
    যদি (isDouble()) ফেরত static_cast<uint64_t>(toDouble());
    জন্য (পূর্ণসংখ্যা i = 2; i < শব্দগুলি(); ++i) {
        যদি (m_তথ্য.অঙ্ক()[i].m_মান) {
            v3error("Value too wide for 64-bits expected in this context " << *এই);
            বিরতি;
        }
    }
    যদি (width() <= 32) ফেরত (static_cast<uint64_t>(toUInt()));
    ফেরত ((static_cast<uint64_t>(m_তথ্য.অঙ্ক()[1].m_মান) << 32ULL)
            | (static_cast<uint64_t>(m_তথ্য.অঙ্ক()[0].m_মান)));
}

int64_t V3_সংখ্যা::toSQuad() ধ্রুবক {
    যদি (isDouble()) ফেরত static_cast<int64_t>(toDouble());
    ধ্রুবক uint64_t v = toUQuad();
    ধ্রুবক uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    ধ্রুবক uint64_t extended = v | signExtend;
    ফেরত static_cast<int64_t>(extended);
}

string V3_সংখ্যা::toString() ধ্রুবক {
    UASSERT(!isFourState(), "toString with 4-state " << *এই);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    যদি (isString()) ফেরত m_তথ্য.str();
    পূর্ণসংখ্যা bit = width() - 1;
    যখন ((bit % 8) != 7) ++bit;
    std::string str;
    জন্য (; bit >= 0; bit -= 8) {
        ধ্রুবক পূর্ণসংখ্যা v = bits_মান(bit - 7, 8);
        যদি (v) str += static_cast<অক্ষর>(v);
    }
    ফেরত str;
}

uint32_t V3_সংখ্যা::edataWord(পূর্ণসংখ্যা eword) ধ্রুবক {
    UASSERT(!isFourState(), "edataWord with 4-state " << *এই);
    ফেরত m_তথ্য.অঙ্ক()[eword].m_মান;
}

uint8_t V3_সংখ্যা::তথ্য_Byte(পূর্ণসংখ্যা byte) ধ্রুবক {
    ফেরত (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

যৌক্তিক V3_সংখ্যা::isAllZ() ধ্রুবক VL_MT_SAFE {
    যদি (isDouble() || isString()) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        ধ্রুবক uint32_t mask = i == শব্দগুলি() - 1 ? hiWordMask() : ~0U;
        যদি (((~v.m_মান & v.m_মান_X) & mask) != mask) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}
যৌক্তিক V3_সংখ্যা::isAllX() ধ্রুবক VL_MT_SAFE {
    যদি (isDouble() || isString()) ফেরত মিথ্যা;
    uint32_t mask = hiWordMask();
    জন্য (পূর্ণসংখ্যা i = শব্দগুলি() - 1; i >= 0; --i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        যদি ((v.m_মান & v.m_মান_X) ^ mask) ফেরত মিথ্যা;
        mask = ~0U;
    }
    ফেরত সত্য;
}
যৌক্তিক V3_সংখ্যা::isEqZero() ধ্রুবক VL_MT_SAFE {
    যদি (isString()) ফেরত m_তথ্য.str().empty();
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        যদি (v.m_মান || v.m_মান_X) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}
যৌক্তিক V3_সংখ্যা::isNeqZero() ধ্রুবক {
    যদি (isString()) ফেরত !m_তথ্য.str().empty();
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        যদি (v.m_মান & ~v.m_মান_X) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}
যৌক্তিক V3_সংখ্যা::isBitsZero(পূর্ণসংখ্যা msb, পূর্ণসংখ্যা lsb) ধ্রুবক {
    জন্য (পূর্ণসংখ্যা i = lsb; i <= msb; ++i) {
        যদি (VL_UNLIKELY(!bitIs0(i))) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}
যৌক্তিক V3_সংখ্যা::isEqOne() ধ্রুবক {
    যদি (m_তথ্য.অঙ্ক()[0].m_মান != 1 || m_তথ্য.অঙ্ক()[0].m_মান_X) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 1; i < শব্দগুলি(); ++i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        যদি (v.m_মান || v.m_মান_X) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}
যৌক্তিক V3_সংখ্যা::isEqAllOnes(পূর্ণসংখ্যা optwidth) ধ্রুবক {
    // Correct number of zero bits/width matters
    যদি (!optwidth) optwidth = width();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < optwidth; ++bit) {
        যদি (!bitIs1(bit)) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}
যৌক্তিক V3_সংখ্যা::isFourState() ধ্রুবক VL_MT_SAFE {
    যদি (isDouble() || isString()) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        যদি (m_তথ্য.অঙ্ক()[i].m_মান_X) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}
যৌক্তিক V3_সংখ্যা::isAnyX() ধ্রুবক VL_MT_SAFE {
    যদি (isDouble() || isString()) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        যদি (v.m_মান & v.m_মান_X) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}
যৌক্তিক V3_সংখ্যা::isAnyXZ() ধ্রুবক {
    যদি (isDouble() || isString()) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        যদি (m_তথ্য.অঙ্ক()[i].m_মান_X) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}
যৌক্তিক V3_সংখ্যা::isAnyZ() ধ্রুবক VL_MT_SAFE {
    যদি (isDouble() || isString()) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
        যদি (~v.m_মান & v.m_মান_X) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}
যৌক্তিক V3_সংখ্যা::isLtXZ(ধ্রুবক V3_সংখ্যা& rhs) ধ্রুবক {
    // Include X/Z in comparisons for sort ordering
    যদি (isString()) ফেরত toString() < rhs.toString();
    যদি (isDouble()) ফেরত toDouble() < rhs.toDouble();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        যদি (bitIs1(bit) && rhs.bitIs0(bit)) ফেরত সত্য;
        যদি (rhs.bitIs1(bit) && bitIs0(bit)) ফেরত মিথ্যা;
        যদি (bitIsXZ(bit)) ফেরত সত্য;
        যদি (rhs.bitIsXZ(bit)) ফেরত মিথ্যা;
    }
    ফেরত মিথ্যা;
}
পূর্ণসংখ্যা V3_সংখ্যা::গুনতি_X(পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা nbits) ধ্রুবক VL_MT_SAFE {
    পূর্ণসংখ্যা গুনতি = 0;
    জন্য (পূর্ণসংখ্যা bitn = 0; bitn < nbits; ++bitn) {
        যদি (lsb + bitn >= width()) ফেরত গুনতি;
        যদি (bitIsX(lsb + bitn)) ++গুনতি;
    }
    ফেরত গুনতি;
}
পূর্ণসংখ্যা V3_সংখ্যা::গুনতি_Z(পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা nbits) ধ্রুবক VL_MT_SAFE {
    পূর্ণসংখ্যা গুনতি = 0;
    জন্য (পূর্ণসংখ্যা bitn = 0; bitn < nbits; ++bitn) {
        যদি (lsb + bitn >= width()) ফেরত গুনতি;
        যদি (bitIsZ(lsb + bitn)) ++গুনতি;
    }
    ফেরত গুনতি;
}

পূর্ণসংখ্যা V3_সংখ্যা::widthToFit() ধ্রুবক {
    জন্য (পূর্ণসংখ্যা bit = width() - 1; bit > 0; bit--) {
        যদি (!bitIs0(bit)) ফেরত bit + 1;
    }
    ফেরত 1;  // one bit even if number is == 0
}

uint32_t V3_সংখ্যা::গুনতি_Bits(ধ্রুবক V3_সংখ্যা& ctrl) ধ্রুবক {
    // Correct number of zero bits/width matters
    পূর্ণসংখ্যা n = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        নির্বাচন (ctrl.bitIs(0)) {
        ক্ষেত্র '0':
            যদি (bitIs0(bit)) ++n;
            বিরতি;
        ক্ষেত্র '1':
            যদি (bitIs1(bit)) ++n;
            বিরতি;
        ক্ষেত্র 'x':
            যদি (bitIsX(bit)) ++n;
            বিরতি;
        ক্ষেত্র 'z':
            যদি (bitIsZ(bit)) ++n;
            বিরতি;
        }
    }
    ফেরত n;
}

uint32_t V3_সংখ্যা::গুনতি_Bits(ধ্রুবক V3_সংখ্যা& ctrl1, ধ্রুবক V3_সংখ্যা& ctrl2,
                             ধ্রুবক V3_সংখ্যা& ctrl3) ধ্রুবক {
    // Correct number of zero bits/width matters
    পূর্ণসংখ্যা n = গুনতি_Bits(ctrl1);
    যদি (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += গুনতি_Bits(ctrl2);
    যদি ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += গুনতি_Bits(ctrl3);
    }
    ফেরত n;
}

uint32_t V3_সংখ্যা::গুনতি_Ones() ধ্রুবক {
    পূর্ণসংখ্যা n = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (bitIs1(bit)) ++n;
    }
    ফেরত n;
}

uint32_t V3_সংখ্যা::mostSetBitP1() ধ্রুবক {
    জন্য (পূর্ণসংখ্যা bit = width() - 1; bit >= 0; bit--) {
        যদি (!bitIs0(bit)) ফেরত bit + 1;
    }
    ফেরত 0;
}

uint32_t V3_সংখ্যা::leastSetBitP1() ধ্রুবক {
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (!bitIs0(bit)) ফেরত bit + 1;
    }
    ফেরত 0;
}

//======================================================================

V3_সংখ্যা& V3_সংখ্যা::opBitsNonXZ(ধ্রুবক V3_সংখ্যা& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opBitsOne(ধ্রুবক V3_সংখ্যা& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opBitsXZ(ধ্রুবক V3_সংখ্যা& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opBitsZ(ধ্রুবক V3_সংখ্যা& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    ফেরত *এই;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_সংখ্যা& V3_সংখ্যা::opRedOr(ধ্রুবক V3_সংখ্যা& lhs) {
    // op i, 1 bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    অক্ষর outc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < lhs.width(); ++bit) {
        যদি (lhs.bitIs1(bit)) {
            ফেরত setSingleBits(1);
        } অন্যথা যদি (lhs.bitIs0(bit)) {
        } অন্যথা {
            outc = 'x';
        }
    }
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opRedAnd(ধ্রুবক V3_সংখ্যা& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    অক্ষর outc = 1;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < lhs.width(); ++bit) {
        যদি (lhs.bitIs0(bit)) {
            ফেরত setSingleBits(0);
        } অন্যথা যদি (lhs.bitIs1(bit)) {
        } অন্যথা {
            outc = 'x';
        }
    }
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opRedXor(ধ্রুবক V3_সংখ্যা& lhs) {
    // op i, 1 bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    অক্ষর outc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < lhs.width(); ++bit) {
        যদি (lhs.bitIs1(bit)) {
            যদি (outc == 1) {
                outc = 0;
            } অন্যথা যদি (outc == 0) {
                outc = 1;
            }
        } অন্যথা যদি (lhs.bitIs0(bit)) {
        } অন্যথা {
            outc = 'x';
        }
    }
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::op_গুনতি_Bits(ধ্রুবক V3_সংখ্যা& রাশিমালা, ধ্রুবক V3_সংখ্যা& ctrl1, ধ্রুবক V3_সংখ্যা& ctrl2,
                                ধ্রুবক V3_সংখ্যা& ctrl3) {
    // Correct number of zero bits/width matters
    অঙ্ক_ASSERT_OP_ARGS4(রাশিমালা, ctrl1, ctrl2, ctrl3);
    অঙ্ক_ASSERT_LOGIC_ARGS4(রাশিমালা, ctrl1, ctrl2, ctrl3);
    setZero();
    m_তথ্য.অঙ্ক()[0].m_মান = রাশিমালা.গুনতি_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::op_গুনতি_Ones(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    যদি (lhs.isFourState()) ফেরত setAllBitsX();
    setZero();
    m_তথ্য.অঙ্ক()[0].m_মান = lhs.গুনতি_Ones();
    opCleanThis();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opIsUnknown(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    ফেরত setSingleBits(lhs.isAnyXZ());
}
V3_সংখ্যা& V3_সংখ্যা::opOneHot(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    যদি (lhs.isFourState()) ফেরত setAllBitsX();
    ফেরত setSingleBits(lhs.গুনতি_Ones() == 1);
}
V3_সংখ্যা& V3_সংখ্যা::opOneHot0(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    যদি (lhs.isFourState()) ফেরত setAllBitsX();
    ফেরত setSingleBits(lhs.গুনতি_Ones() <= 1);
}
V3_সংখ্যা& V3_সংখ্যা::opCLog2(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    যদি (lhs.isFourState()) ফেরত setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ধ্রুবক পূর্ণসংখ্যা adjust = (lhs.গুনতি_Ones() == 1) ? 0 : 1;
    জন্য (পূর্ণসংখ্যা bit = lhs.width() - 1; bit >= 0; bit--) {
        যদি (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            ফেরত *এই;
        }
    }
    setZero();
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opMostSetBitP1(ধ্রুবক V3_সংখ্যা& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    যদি (lhs.isFourState()) ফেরত setAllBitsX();
    জন্য (পূর্ণসংখ্যা bit = lhs.width() - 1; bit >= 0; bit--) {
        যদি (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            ফেরত *এই;
        }
    }
    setZero();
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opLogNot(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    অক্ষর outc = 1;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < lhs.width(); ++bit) {
        যদি (lhs.bitIs1(bit)) {
            outc = 0;
            যাও last;
        } অন্যথা যদি (lhs.bitIs0(bit)) {
        } অন্যথা {
            outc = 'x';
        }
    }
last:
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opNot(ধ্রুবক V3_সংখ্যা& lhs) {
    // Correct number of zero bits/width matters
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } অন্যথা যদি (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opAnd(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } অন্যথা যদি (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } অন্যথা {
            setBit(bit, 'x');
        }
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opOr(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } অন্যথা যদি (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } অন্যথা {
            setBit(bit, 'x');
        }
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opXor(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } অন্যথা যদি (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } অন্যথা যদি (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    ফেরত *এই;
}

শূন্য V3_সংখ্যা::fillBits(পূর্ণসংখ্যা destLsb, পূর্ণসংখ্যা width, অক্ষর মান) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    যদি (!width) ফেরত;
    UASSERT(width <= এই->width() - destLsb, "Destination bit range exceeds number width");

    ধ্রুবক uint32_t মান_Word = মান == '1' || মান == 'x' || মান == 1 || মান == 3 ? ~0U : 0;
    ধ্রুবক uint32_t xWord = মান == 'z' || মান == 'x' || মান == 2 || মান == 3 ? ~0U : 0;
    ধ্রুবক পূর্ণসংখ্যা firstWord = VL_BITWORD_E(destLsb);
    ধ্রুবক পূর্ণসংখ্যা lastWord = VL_BITWORD_E(destLsb + width - 1);
    জন্য (পূর্ণসংখ্যা word = firstWord; word <= lastWord; ++word) {
        ধ্রুবক পূর্ণসংখ্যা wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ধ্রুবক পূর্ণসংখ্যা wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        ধ্রুবক uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        মান_And_X& dest = m_তথ্য.অঙ্ক()[word];
        dest.m_মান = (dest.m_মান & ~mask) | (মান_Word & mask);
        dest.m_মান_X = (dest.m_মান_X & ~mask) | (xWord & mask);
    }
}

শূন্য V3_সংখ্যা::copyBits(পূর্ণসংখ্যা destLsb, ধ্রুবক V3_সংখ্যা& source, পূর্ণসংখ্যা sourceLsb, পূর্ণসংখ্যা width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    যদি (!width) ফেরত;
    UASSERT(width <= এই->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    ধ্রুবক পূর্ণসংখ্যা firstWord = VL_BITWORD_E(destLsb);
    ধ্রুবক পূর্ণসংখ্যা lastWord = VL_BITWORD_E(destLsb + width - 1);
    জন্য (পূর্ণসংখ্যা destWord = firstWord; destWord <= lastWord; ++destWord) {
        ধ্রুবক পূর্ণসংখ্যা destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        ধ্রুবক পূর্ণসংখ্যা destOffset = VL_BITBIT_E(destBit);
        ধ্রুবক পূর্ণসংখ্যা sourceBit = sourceLsb + destBit - destLsb;
        ধ্রুবক পূর্ণসংখ্যা sourceWord = VL_BITWORD_E(sourceBit);
        ধ্রুবক পূর্ণসংখ্যা sourceOffset = VL_BITBIT_E(sourceBit);
        ধ্রুবক পূর্ণসংখ্যা copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        ধ্রুবক uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        ধ্রুবক মান_And_X& sourceLo = source.m_তথ্য.অঙ্ক()[sourceWord];
        uint32_t মান = sourceLo.m_মান >> sourceOffset;
        uint32_t মান_X = sourceLo.m_মান_X >> sourceOffset;
        যদি (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            ধ্রুবক মান_And_X& sourceHi = source.m_তথ্য.অঙ্ক()[sourceWord + 1];
            মান |= sourceHi.m_মান << (VL_EDATASIZE - sourceOffset);
            মান_X |= sourceHi.m_মান_X << (VL_EDATASIZE - sourceOffset);
        }

        মান_And_X& dest = m_তথ্য.অঙ্ক()[destWord];
        dest.m_মান = (dest.m_মান & ~mask) | ((মান << destOffset) & mask);
        dest.m_মান_X = (dest.m_মান_X & ~mask) | ((মান_X << destOffset) & mask);
    }
}

V3_সংখ্যা& V3_সংখ্যা::opConcat(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Correct number of zero bits/width matters
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    যদি (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opLenN(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().দৈর্ঘ্য());
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opRepl(ধ্রুবক V3_সংখ্যা& lhs,
                           ধ্রুবক V3_সংখ্যা& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ফেরত opRepl(lhs, rhs.toUInt());
}

V3_সংখ্যা& V3_সংখ্যা::opRepl(ধ্রুবক V3_সংখ্যা& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    যদি (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    জন্য (unsigned times = 0; times < rhsval; ++times) {
        ধ্রুবক uint64_t destLsb = uint64_t{times} * lhs.width();
        যদি (destLsb >= static_cast<uint32_t>(width())) বিরতি;
        copyBits(static_cast<পূর্ণসংখ্যা>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<পূর্ণসংখ্যা>(destLsb)));
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opStreamL(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    যদি (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ধ্রুবক পূর্ণসংখ্যা ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    জন্য (পূর্ণসংখ্যা istart = 0; istart < lhs.width(); istart += ssize) {
        ধ্রুবক পূর্ণসংখ্যা ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opLogAnd(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    অক্ষর loutc = 0;
    অক্ষর routc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < lhs.width(); ++bit) {
        যদি (lhs.bitIs1(bit)) {
            loutc = 1;
            বিরতি;
        }
        যদি (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    জন্য (পূর্ণসংখ্যা bit = 0; bit < rhs.width(); ++bit) {
        যদি (rhs.bitIs1(bit)) {
            routc = 1;
            বিরতি;
        }
        যদি (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    অক্ষর outc = 'x';
    যদি (routc == 1 && loutc == 1) outc = 1;
    যদি (routc == 0 || loutc == 0) outc = 0;
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opLogOr(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    অক্ষর outc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < lhs.width(); ++bit) {
        যদি (lhs.bitIs1(bit)) {
            outc = 1;
            যাও last;
        }
        যদি (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    জন্য (পূর্ণসংখ্যা bit = 0; bit < rhs.width(); ++bit) {
        যদি (rhs.bitIs1(bit)) {
            outc = 1;
            যাও last;
        }
        যদি (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opLogIf(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    V3_সংখ্যা lnot = lhs;
    lnot.opLogNot(lhs);
    ফেরত opLogOr(lnot, rhs);
}

V3_সংখ্যা& V3_সংখ্যা::opLogEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    V3_সংখ্যা ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_সংখ্যা ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ফেরত opLogAnd(ifa, ifb);
}

V3_সংখ্যা& V3_সংখ্যা::opAtoN(ধ্রুবক V3_সংখ্যা& lhs, পূর্ণসংখ্যা base) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    যদি (base == AstAtoN::ATOREAL) ফেরত setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    যদি (errno != 0) v = 0;
    ফেরত setLongS(static_cast<int32_t>(v));
}

V3_সংখ্যা& V3_সংখ্যা::opPutcN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, ধ্রুবক V3_সংখ্যা& ths) {
    অঙ্ক_ASSERT_OP_ARGS3(lhs, rhs, ths);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    ধ্রুবক int32_t i = rhs.toSInt();
    ধ্রুবক int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    যদি (0 <= i && static_cast<uint32_t>(i) < lstring.দৈর্ঘ্য() && c != 0) lstring[i] = c;
    ফেরত setString(lstring);
}

V3_সংখ্যা& V3_সংখ্যা::opGetcN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    ধ্রুবক string lstring = lhs.toString();
    ধ্রুবক int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    যদি (0 <= i && static_cast<uint32_t>(i) < lstring.দৈর্ঘ্য()) v = lstring[i];
    ফেরত setLong(v);
}

V3_সংখ্যা& V3_সংখ্যা::opSubstrN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, ধ্রুবক V3_সংখ্যা& ths) {
    অঙ্ক_ASSERT_OP_ARGS3(lhs, rhs, ths);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    ধ্রুবক string lstring = lhs.toString();
    ধ্রুবক int32_t i = rhs.toSInt();
    ধ্রুবক int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    যদি (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.দৈর্ঘ্য()) ফেরত setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ফেরত setString(lstring.substr(i, j - i + 1));
}

V3_সংখ্যা& V3_সংখ্যা::opCompareNN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, যৌক্তিক ignoreCase) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    পূর্ণসংখ্যা ফলাফল;
    ধ্রুবক string lstring = lhs.toString();
    ধ্রুবক string rstring = rhs.toString();
    যদি (ignoreCase) {
        ফলাফল = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } অন্যথা {
        ফলাফল = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ফেরত setLongS(ফলাফল);
}

V3_সংখ্যা& V3_সংখ্যা::opEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    যদি (lhs.isString()) ফেরত opEqN(lhs, rhs);
    যদি (lhs.isDouble()) ফেরত opEqD(lhs, rhs);
    অক্ষর outc = 1;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        যদি (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            যাও last;
        }
        যদি (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            যাও last;
        }
        যদি (lhs.bitIsXZ(bit)) outc = 'x';
        যদি (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opNeq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    যদি (lhs.isString()) ফেরত opNeqN(lhs, rhs);
    যদি (lhs.isDouble()) ফেরত opNeqD(lhs, rhs);
    অক্ষর outc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        যদি (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            যাও last;
        }
        যদি (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            যাও last;
        }
        যদি (lhs.bitIsXZ(bit)) outc = 'x';
        যদি (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ফেরত setSingleBits(outc);
}

V3Hash V3_সংখ্যা::toHash() ধ্রুবক {
    // Shouldmatch isCaseEq
    যদি (isString()) ফেরত V3Hash{toString()};
    যদি (isDouble()) ফেরত V3Hash{toDouble()};
    V3Hash hash{width()};
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) hash += m_তথ্য.অঙ্ক()[i].m_মান;
    ফেরত hash;
}

যৌক্তিক V3_সংখ্যা::isCaseEq(ধ্রুবক V3_সংখ্যা& rhs) ধ্রুবক {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    যদি (isString()) ফেরত toString() == rhs.toString();
    যদি (isDouble()) ফেরত toDouble() == rhs.toDouble();
    যদি (width() != rhs.width()) ফেরত মিথ্যা;
    জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
        যদি (!(m_তথ্য.অঙ্ক()[i] == rhs.m_তথ্য.অঙ্ক()[i])) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}

V3_সংখ্যা& V3_সংখ্যা::opCaseEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_সংখ্যা& V3_সংখ্যা::opCaseNeq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অক্ষর outc = 0;
    যদি (lhs.isString()) {
        ফেরত opNeqN(lhs, rhs);
    } অন্যথা যদি (lhs.isDouble()) {
        ফেরত opNeqD(lhs, rhs);
    }
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        যদি (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            যাও last;
        }
    }
last:
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opWildEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    অক্ষর outc = 1;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        যদি (!rhs.bitIsXZ(bit)) {
            যদি (lhs.bitIsXZ(bit)) {
                outc = 'x';
                যাও last;
            }
            যদি (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                যাও last;
            }
        }
    }
last:
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opWildNeq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    অক্ষর outc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        যদি (!rhs.bitIsXZ(bit)) {
            যদি (lhs.bitIsXZ(bit)) {
                outc = 'x';
                যাও last;
            }
            যদি (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                যাও last;
            }
        }
    }
last:
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opGt(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    অক্ষর outc = 0;
    জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        যদি (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        যদি (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        যদি (lhs.bitIsXZ(bit)) outc = 'x';
        যদি (rhs.bitIsXZ(bit)) outc = 'x';
    }
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opGtS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    অক্ষর outc = 0;
    {
        ধ্রুবক পূর্ণসংখ্যা mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        যদি (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } অন্যথা যদি (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } অন্যথা যদি (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } অন্যথা যদি (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } অন্যথা {
            // both positive or negative, normal >
            জন্য (পূর্ণসংখ্যা bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                যদি (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                যদি (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                যদি (lhs.bitIsXZ(bit)) outc = 'x';
                যদি (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    ফেরত setSingleBits(outc);
}

V3_সংখ্যা& V3_সংখ্যা::opGte(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    V3_সংখ্যা& eq = opEq(lhs, rhs);
    যদি (eq.isNeqZero()) ফেরত eq;  // Return true
    ফেরত opGt(lhs, rhs);
}

V3_সংখ্যা& V3_সংখ্যা::opGteS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    V3_সংখ্যা& eq = opEq(lhs, rhs);
    যদি (eq.isNeqZero()) ফেরত eq;  // Return true
    ফেরত opGtS(lhs, rhs);
}

V3_সংখ্যা& V3_সংখ্যা::opLt(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { ফেরত opGt(rhs, lhs); }
V3_সংখ্যা& V3_সংখ্যা::opLte(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { ফেরত opGte(rhs, lhs); }
V3_সংখ্যা& V3_সংখ্যা::opLtS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { ফেরত opGtS(rhs, lhs); }
V3_সংখ্যা& V3_সংখ্যা::opLteS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) { ফেরত opGteS(rhs, lhs); }

V3_সংখ্যা& V3_সংখ্যা::opShiftR(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (rhs.isFourState()) ফেরত setAllBitsX();
    setZero();
    যদি (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ফেরত *এই;
    ধ্রুবক uint32_t rhsval = rhs.toUInt();
    যদি (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<পূর্ণসংখ্যা>(rhsval),
                 std::min(width(), lhs.width() - static_cast<পূর্ণসংখ্যা>(rhsval)));
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opShiftRS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (rhs.isFourState()) ফেরত setAllBitsX();
    setZero();
    ধ্রুবক যৌক্তিক overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    যদি (!overflow) {
        ধ্রুবক uint32_t rhsval = rhs.toUInt();
        যদি (rhsval < lbits) {
            ধ্রুবক পূর্ণসংখ্যা copyWidth = std::min(width(), static_cast<পূর্ণসংখ্যা>(lbits - rhsval));
            copyBits(0, lhs, static_cast<পূর্ণসংখ্যা>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            ফেরত *এই;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opShiftL(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (rhs.isFourState()) ফেরত setAllBitsX();
    setZero();
    যদি (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ফেরত *এই;
    ধ্রুবক uint32_t rhsval = rhs.toUInt();
    যদি (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<পূর্ণসংখ্যা>(rhsval), lhs, 0,
                 std::min(width() - static_cast<পূর্ণসংখ্যা>(rhsval), lhs.width()));
    }
    ফেরত *এই;
}

//======================================================================
// Ops - Arithmetic

V3_সংখ্যা& V3_সংখ্যা::opNegate(ধ্রুবক V3_সংখ্যা& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    যদি (lhs.isFourState()) ফেরত setAllBitsX();
    V3_সংখ্যা notlhs(&lhs, width());
    notlhs.opNot(lhs);
    ধ্রুবক V3_সংখ্যা one(&lhs, width(), 1);
    opAdd(notlhs, one);
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opAdd(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    জন্য (পূর্ণসংখ্যা word = 0; word < শব্দগুলি(); ++word) {
        ধ্রুবক uint64_t lwordval = lhs.m_তথ্য.অঙ্ক()[word].m_মান;
        ধ্রুবক uint64_t rwordval = rhs.m_তথ্য.অঙ্ক()[word].m_মান;
        ধ্রুবক uint64_t যোগ_2 = lwordval + rwordval + carry;
        m_তথ্য.অঙ্ক()[word].m_মান = যোগ_2 & 0xffffffffULL;
        carry = যোগ_2 > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opSub(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    V3_সংখ্যা negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    ফেরত opAdd(lhs, negrhs);
}
V3_সংখ্যা& V3_সংখ্যা::opMul(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    setZero();
    যদি (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } অন্যথা {
        জন্য (পূর্ণসংখ্যা lword = 0; lword < lhs.শব্দগুলি(); ++lword) {
            ধ্রুবক uint64_t lwordval = static_cast<uint64_t>(lhs.m_তথ্য.অঙ্ক()[lword].m_মান);
            যদি (lwordval == 0) চালু;
            জন্য (পূর্ণসংখ্যা rword = 0; rword < rhs.শব্দগুলি(); ++rword) {
                ধ্রুবক uint64_t rwordval = static_cast<uint64_t>(rhs.m_তথ্য.অঙ্ক()[rword].m_মান);
                যদি (rwordval == 0) চালু;
                uint64_t mul = lwordval * rwordval;
                জন্য (পূর্ণসংখ্যা qword = lword + rword; qword < শব্দগুলি(); ++qword) {
                    mul += static_cast<uint64_t>(m_তথ্য.অঙ্ক()[qword].m_মান);
                    m_তথ্য.অঙ্ক()[qword].m_মান = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    যদি (mul == 0) বিরতি;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opMulS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    V3_সংখ্যা lhsNoSign = lhs;
    যদি (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_সংখ্যা rhsNoSign = rhs;
    যদি (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ধ্রুবক V3_সংখ্যা qNoSign = opMul(lhsNoSign, rhsNoSign);
    যদি ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } অন্যথা {
        opAssign(qNoSign);
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opDiv(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    যদি (rhs.isEqZero()) ফেরত setAllBitsXRemoved();
    যদি (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        ফেরত *এই;
    } অন্যথা {
        // Wide division
        ফেরত opModDivGuts(lhs, rhs, মিথ্যা);
    }
}
V3_সংখ্যা& V3_সংখ্যা::opDivS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    যদি (rhs.isEqZero()) ফেরত setAllBitsXRemoved();
    V3_সংখ্যা lhsNoSign = lhs;
    যদি (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_সংখ্যা rhsNoSign = rhs;
    যদি (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ধ্রুবক V3_সংখ্যা qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    যদি ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } অন্যথা {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *এই);
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opModDiv(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    যদি (rhs.isEqZero()) ফেরত setAllBitsXRemoved();
    যদি (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        ফেরত *এই;
    } অন্যথা {
        // Wide modulus
        ফেরত opModDivGuts(lhs, rhs, সত্য);
    }
}
V3_সংখ্যা& V3_সংখ্যা::opModDivS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    যদি (rhs.isEqZero()) ফেরত setAllBitsXRemoved();
    V3_সংখ্যা lhsNoSign = lhs;
    যদি (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_সংখ্যা rhsNoSign = rhs;
    যদি (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ধ্রুবক V3_সংখ্যা qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    যদি (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } অন্যথা {
        opAssign(qNoSign);
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opModDivGuts(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, যৌক্তিক is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    ধ্রুবক পূর্ণসংখ্যা l_শব্দগুলি = lhs.শব্দগুলি();
    ধ্রুবক পূর্ণসংখ্যা umsbp1 = lhs.mostSetBitP1();  // dividend
    ধ্রুবক পূর্ণসংখ্যা vmsbp1 = rhs.mostSetBitP1();  //ভাজক
    যদি (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *এই);
        ফেরত *এই;
    }

    ধ্রুবক পূর্ণসংখ্যা uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ধ্রুবক পূর্ণসংখ্যা vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    যদি (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        জন্য (পূর্ণসংখ্যা j = uw - 1; j >= 0; j--) {
            ধ্রুবক uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_তথ্য.অঙ্ক()[j].m_মান));
            m_তথ্য.অঙ্ক()[j].m_মান = unw64 / static_cast<uint64_t>(rhs.m_তথ্য.অঙ্ক()[0].m_মান);
            k = unw64
                - (static_cast<uint64_t>(m_তথ্য.অঙ্ক()[j].m_মান)
                   * static_cast<uint64_t>(rhs.m_তথ্য.অঙ্ক()[0].m_মান));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *এই << " rem=0x" << std::hex
                                   << k << std::dec);
        যদি (is_modulus) {
            setZero();
            m_তথ্য.অঙ্ক()[0].m_মান = k;
        }
        opCleanThis();
        ফেরত *এই;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_শব্দগুলি + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_শব্দগুলি + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    জন্য (পূর্ণসংখ্যা i = 0; i < l_শব্দগুলি; ++i) m_তথ্য.অঙ্ক()[i].m_মান = 0;
    জন্য (পূর্ণসংখ্যা i = 0; i < l_শব্দগুলি + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ধ্রুবক পূর্ণসংখ্যা s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ধ্রুবক uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    জন্য (পূর্ণসংখ্যা i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_তথ্য.অঙ্ক()[i].m_মান << s)
                | (shift_mask & (rhs.m_তথ্য.অঙ্ক()[i - 1].m_মান >> (32 - s)));
    }
    vn[0] = rhs.m_তথ্য.অঙ্ক()[0].m_মান << s;

    // Copy and shift dividend by same amount; may set new upper word
    যদি (s) {
        un[uw] = lhs.m_তথ্য.অঙ্ক()[uw - 1].m_মান >> (32 - s);
    } অন্যথা {
        un[uw] = 0;
    }
    জন্য (পূর্ণসংখ্যা i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_তথ্য.অঙ্ক()[i].m_মান << s)
                | (shift_mask & (lhs.m_তথ্য.অঙ্ক()[i - 1].m_মান >> (32 - s)));
    }
    un[0] = lhs.m_তথ্য.অঙ্ক()[0].m_মান << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    জন্য (পূর্ণসংখ্যা j = uw - vw; j >= 0; j--) {
        // Estimate
        ধ্রুবক uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        যদি (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            যদি (rhat < 0x100000000ULL) যাও again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        জন্য (পূর্ণসংখ্যা i = 0; i < vw; ++i) {
            ধ্রুবক uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_তথ্য.অঙ্ক()[j].m_মান = qhat;  // Save quotient digit

        যদি (t < 0) {
            // Over subtracted; correct by adding back
            m_তথ্য.অঙ্ক()[j].m_মান--;
            k = 0;
            জন্য (পূর্ণসংখ্যা i = 0; i < vw; ++i) {
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

    যদি (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        জন্য (পূর্ণসংখ্যা i = 0; i < vw; ++i) {
            m_তথ্য.অঙ্ক()[i].m_মান = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        জন্য (পূর্ণসংখ্যা i = vw; i < l_শব্দগুলি; ++i) m_তথ্য.অঙ্ক()[i].m_মান = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *এই);
        ফেরত *এই;
    } অন্যথা {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *এই);
        ফেরত *এই;
    }
}

V3_সংখ্যা& V3_সংখ্যা::opPow(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, যৌক্তিক lsign, যৌক্তিক rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(lhs, rhs);
    যদি (lhs.isFourState() || rhs.isFourState()) ফেরত setAllBitsX();
    যদি (rhs.isEqZero()) ফেরত setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    যদি (rsign && rhs.isNegative()) {
        যদি (lhs.isEqZero()) {
            ফেরত setAllBitsXRemoved();
        } অন্যথা যদি (lhs.isEqOne()) {
            ফেরত setValue1();
        } অন্যথা যদি (lsign && lhs.isEqAllOnes()) {
            যদি (rhs.bitIs1(0)) {
                ফেরত setAllBits1();  // -1^odd=-1
            } অন্যথা {
                ফেরত setValue1();  // -1^even=1
            }
        }
        ফেরত setZero();
    }
    যদি (lhs.isEqZero()) ফেরত setZero();
    setZero();
    m_তথ্য.অঙ্ক()[0].m_মান = 1;
    V3_সংখ্যা power(&lhs, width());
    power.opAssign(lhs);
    জন্য (পূর্ণসংখ্যা bit = 0; bit < rhs.width(); ++bit) {
        যদি (bit > 0) {  // power = power*power
            V3_সংখ্যা lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        যদি (rhs.bitIs1(bit)) {  // out *= power
            V3_সংখ্যা lastOut(&lhs, width());
            lastOut.opAssign(*এই);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opPowSU(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Correct number of zero bits/width matters
    ফেরত opPow(lhs, rhs, সত্য, মিথ্যা);
}
V3_সংখ্যা& V3_সংখ্যা::opPowSS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Correct number of zero bits/width matters
    ফেরত opPow(lhs, rhs, সত্য, সত্য);
}
V3_সংখ্যা& V3_সংখ্যা::opPowUS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // Correct number of zero bits/width matters
    ফেরত opPow(lhs, rhs, মিথ্যা, সত্য);
}

V3_সংখ্যা& V3_সংখ্যা::opBufIf1(ধ্রুবক V3_সংখ্যা& ens, ধ্রুবক V3_সংখ্যা& if1s) {
    অঙ্ক_ASSERT_OP_ARGS2(ens, if1s);
    অঙ্ক_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    জন্য (পূর্ণসংখ্যা bit = 0; bit < width(); ++bit) {
        যদি (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } অন্যথা {
            setBit(bit, 'z');
        }
    }
    ফেরত *এই;
}

// Sets all bits in range to the given value
V3_সংখ্যা& V3_সংখ্যা::opSetRange(uint32_t lsb, uint32_t width, অক্ষর bit_মান) {
    যদি (lsb < static_cast<uint32_t>(এই->width())) {
        fillBits(static_cast<পূর্ণসংখ্যা>(lsb),
                 static_cast<পূর্ণসংখ্যা>(std::min(width, static_cast<uint32_t>(এই->width()) - lsb)),
                 bit_মান);
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opAssign(ধ্রুবক V3_সংখ্যা& lhs) { ফেরত opAssignNonXZ(lhs, মিথ্যা); }
V3_সংখ্যা& V3_সংখ্যা::opAssignNonXZ(ধ্রুবক V3_সংখ্যা& lhs, যৌক্তিক ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    যদি (এই != &lhs) {
        যদি (VL_UNLIKELY(lhs.isNull())) {
            m_তথ্য.m_isNull = সত্য;
        } অন্যথা যদি (isString()) {
            যদি (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_তথ্য.str() = lhs.toString();
            } অন্যথা {
                m_তথ্য.str() = lhs.m_তথ্য.str();
            }
        } অন্যথা যদি (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } অন্যথা যদি (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } অন্যথা যদি (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } অন্যথা {
            জন্য (পূর্ণসংখ্যা word = 0; word < শব্দগুলি(); ++word) {
                ধ্রুবক uint32_t মান = word < lhs.শব্দগুলি() ? lhs.m_তথ্য.অঙ্ক()[word].m_মান
                                                                & ~lhs.m_তথ্য.অঙ্ক()[word].m_মান_X
                                                          : 0;
                m_তথ্য.অঙ্ক()[word] = {মান, 0};
            }
            opCleanThis();
        }
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opNToI(ধ্রুবক V3_সংখ্যা& lhs) {
    // String to packed number
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    setZero();
    ধ্রুবক string& str = lhs.toString();
    জন্য (আকার_t n = 0; n < str.দৈর্ঘ্য(); ++n) {
        ধ্রুবক অক্ষর c = str[str.দৈর্ঘ্য() - 1 - n];
        জন্য (আকার_t cbit = 0; cbit < 8; ++cbit) {
            ধ্রুবক std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opExtendS(ধ্রুবক V3_সংখ্যা& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ধ্রুবক পূর্ণসংখ্যা copyWidth = std::min(width(), static_cast<পূর্ণসংখ্যা>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opExtendXZ(ধ্রুবক V3_সংখ্যা& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ধ্রুবক পূর্ণসংখ্যা copyWidth = std::min(width(), static_cast<পূর্ণসংখ্যা>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opClean(ধ্রুবক V3_সংখ্যা& lhs, uint32_t bits) { ফেরত opSel(lhs, bits - 1, 0); }

শূন্য V3_সংখ্যা::opCleanThis(যৌক্তিক warnOnTruncation) {
    // Clean MSB of number
    অঙ্ক_ASSERT_LOGIC_ARGS1(*এই);
    ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[শব্দগুলি() - 1];
    ধ্রুবক uint32_t new_মান_Msb = v.m_মান & hiWordMask();
    ধ্রুবক uint32_t new_মান_XMsb = v.m_মান_X & hiWordMask();
    যদি (warnOnTruncation && (new_মান_Msb != v.m_মান || new_মান_XMsb != v.m_মান_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_তথ্য.অঙ্ক()[শব্দগুলি() - 1] = {new_মান_Msb, new_মান_XMsb};
}

V3_সংখ্যা& V3_সংখ্যা::opSel(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& msb, ধ্রুবক V3_সংখ্যা& lsb) {
    অঙ্ক_ASSERT_OP_ARGS3(lhs, msb, lsb);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS2(msb, lsb);
    যদি (lsb.isFourState() || msb.isFourState()) ফেরত setAllBitsX();
    ফেরত opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_সংখ্যা& V3_সংখ্যা::opSel(ধ্রুবক V3_সংখ্যা& lhs, uint32_t msbval, uint32_t lsbval) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    পূর্ণসংখ্যা copyWidth = 0;
    যদি (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        ধ্রুবক uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<পূর্ণসংখ্যা>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<পূর্ণসংখ্যা>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ফেরত *এই;
}

V3_সংখ্যা& V3_সংখ্যা::opSelInto(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& lsb, পূর্ণসংখ্যা width) {
    ফেরত opSelInto(lhs, lsb.toSInt(), width);
}

V3_সংখ্যা& V3_সংখ্যা::opSelInto(ধ্রুবক V3_সংখ্যা& lhs, পূর্ণসংখ্যা lsbval, পূর্ণসংখ্যা width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ধ্রুবক পূর্ণসংখ্যা copyWidth = std::max(0, std::min({width, lhs.width(), এই->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    ধ্রুবক পূর্ণসংখ্যা fillWidth
        = std::max(0, std::min(width - copyWidth, এই->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    ফেরত *এই;
}

//======================================================================
// Ops - Floating point

V3_সংখ্যা& V3_সংখ্যা::opIToRD(ধ্রুবক V3_সংখ্যা& lhs, যৌক্তিক isSigned) {
    // Correct number of zero bits/width matters
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_সংখ্যা noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    দ্বিগুণ d = 0;
    ধ্রুবক যৌক্তিক negate = isSigned && noxz.isNegative();
    যদি (negate) {
        ধ্রুবক V3_সংখ্যা noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    জন্য (পূর্ণসংখ্যা bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        যদি (noxz.bitIs1(bit)) d += exp2(bit);
    }
    যদি (negate) d = -d;
    ফেরত setDouble(d);
}
V3_সংখ্যা& V3_সংখ্যা::opRToIS(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS1(lhs);
    ধ্রুবক দ্বিগুণ v = VL_TRUNC(lhs.toDouble());
    ধ্রুবক int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ফেরত setLongS(i);
}
V3_সংখ্যা& V3_সংখ্যা::opRToIRoundS(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS1(lhs);
    ধ্রুবক দ্বিগুণ v = VL_ROUND(lhs.toDouble());
    setZero();
    সংঘ {
        দ্বিগুণ d;
        uint64_t q;
    } u;
    u.d = v;
    (শূন্য)u.d;

    ধ্রুবক পূর্ণসংখ্যা exp = static_cast<পূর্ণসংখ্যা>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    ধ্রুবক পূর্ণসংখ্যা lsb = exp - 52;
    ধ্রুবক uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    যদি (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        জন্য (পূর্ণসংখ্যা bit = 0; bit <= 52; ++bit) {
            যদি (mantissa & (1ULL << bit)) {
                ধ্রুবক পূর্ণসংখ্যা outbit = bit + lsb;
                যদি (outbit >= 0) setBit(outbit, 1);
            }
        }
        যদি (v < 0) {
            ধ্রুবক V3_সংখ্যা noSign = *এই;
            opNegate(noSign);
        }
    }
    ফেরত *এই;
}
V3_সংখ্যা& V3_সংখ্যা::opRealToBits(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    সংঘ {
        দ্বিগুণ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    ফেরত setQuad(u.m_v);
}
V3_সংখ্যা& V3_সংখ্যা::opBitsToRealD(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    সংঘ {
        দ্বিগুণ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    ফেরত setDouble(u.m_d);
}
V3_সংখ্যা& V3_সংখ্যা::opNegateD(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS1(lhs);
    ফেরত setDouble(-lhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opAddD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opSubD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opMulD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opDivD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opPowD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_সংখ্যা& V3_সংখ্যা::opEqD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opNeqD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opGtD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opGteD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opLtD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_সংখ্যা& V3_সংখ্যা::opLteD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_সংখ্যা& V3_সংখ্যা::opConcatN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setString(lhs.toString() + rhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opReplN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    অঙ্ক_ASSERT_LOGIC_ARGS1(rhs);
    ফেরত opReplN(lhs, rhs.toUInt());
}
V3_সংখ্যা& V3_সংখ্যা::opReplN(ধ্রুবক V3_সংখ্যা& lhs, uint32_t rhsval) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().দৈর্ঘ্য() * rhsval);
    জন্য (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    ফেরত setString(out);
}
V3_সংখ্যা& V3_সংখ্যা::opCvtPackString(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    ফেরত setString(lhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opToLowerN(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    ধ্রুবক std::string out = VString::downcase(lhs.toString());
    ফেরত setString(out);
}
V3_সংখ্যা& V3_সংখ্যা::opToUpperN(ধ্রুবক V3_সংখ্যা& lhs) {
    অঙ্ক_ASSERT_OP_ARGS1(lhs);
    অঙ্ক_ASSERT_STRING_ARGS1(lhs);
    ধ্রুবক std::string out = VString::upcase(lhs.toString());
    ফেরত setString(out);
}

V3_সংখ্যা& V3_সংখ্যা::opEqN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toString() == rhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opNeqN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toString() != rhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opGtN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toString() > rhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opGteN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toString() >= rhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opLtN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toString() < rhs.toString());
}
V3_সংখ্যা& V3_সংখ্যা::opLteN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs) {
    অঙ্ক_ASSERT_OP_ARGS2(lhs, rhs);
    অঙ্ক_ASSERT_STRING_ARGS2(lhs, rhs);
    ফেরত setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

শূন্য V3_সংখ্যা::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_সারি* ধ্রুবক fl = নতুন File_সারি{File_সারি::builtInFilename()};
    V3_সংখ্যা অঙ্ক{fl, 32, 0};
    অঙ্ক.selfTestThis();
}

শূন্য V3_সংখ্যা::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_সংখ্যা::epsilonEqual(0, 0), সত্য);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonEqual(1e19, 1e19), সত্য);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonEqual(9, 0.0001), মিথ্যা);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonEqual(1, 1 + std::numeric_limits<দ্বিগুণ>::epsilon()), সত্য);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonEqual(0.009, 0.00899999999999999931998839741709), সত্য);

    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(0), সত্য);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(1), সত্য);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(-1), সত্য);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(1.0001), মিথ্যা);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(0.9999), মিথ্যা);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(-1.0001), মিথ্যা);
    UASSERT_SELFTEST(V3_সংখ্যা::epsilonIntegral(-0.9999), মিথ্যা);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
