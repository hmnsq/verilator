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

constexpr पूर्णाङ्क MAX_SPRINTF_DOUBLE_आकार
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#परिभाषय अङ्कः_ASSERT_OP_ARGS1(arg1) \
    UASSERT((एतत् != &(arg1)), "Number operation called with same source and dest")
#परिभाषय अङ्कः_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2)), \
            "Number operation called with same source and dest")
#परिभाषय अङ्कः_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2) && एतत् != &(arg3)), \
            "Number operation called with same source and dest")
#परिभाषय अङ्कः_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((एतत् != &(arg1) && एतत् != &(arg2) && एतत् != &(arg3) && एतत् != &(arg4)), \
            "Number operation called with same source and dest")

#परिभाषय अङ्कः_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).दत्त_Type() == V3_सङ्ख्या_दत्त::V3_सङ्ख्या_दत्त_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#परिभाषय अङ्कः_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    अङ्कः_ASSERT_LOGIC_ARGS1(arg1); \
    अङ्कः_ASSERT_LOGIC_ARGS1(arg2)

#परिभाषय अङ्कः_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    अङ्कः_ASSERT_LOGIC_ARGS1(arg1); \
    अङ्कः_ASSERT_LOGIC_ARGS1(arg2); \
    अङ्कः_ASSERT_LOGIC_ARGS1(arg3); \
    अङ्कः_ASSERT_LOGIC_ARGS1(arg4)

#परिभाषय अङ्कः_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#परिभाषय अङ्कः_ASSERT_STRING_ARGS2(arg1, arg2) \
    अङ्कः_ASSERT_STRING_ARGS1(arg1); \
    अङ्कः_ASSERT_STRING_ARGS1(arg2)

#परिभाषय अङ्कः_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#परिभाषय अङ्कः_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    अङ्कः_ASSERT_DOUBLE_ARGS1(arg1); \
    अङ्कः_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

शून्य V3_सङ्ख्या::v3error_अन्तः(ध्रुव std::ostringstream& str) ध्रुव VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    यदि (m_nodep) {
        m_nodep->v3error_अन्तः(nsstr);
    } अन्यथा यदि (m_fileline) {
        m_fileline->v3error_अन्तः(nsstr);
    } अन्यथा {
        V3Error::v3error_अन्तः(nsstr, "", शून्यसूचकः);
    }
}

शून्य V3_सङ्ख्या::v3error_अन्तः_Fatal(ध्रुव std::ostringstream& str) ध्रुव
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_अन्तः(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_सङ्ख्या::V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, VerilogStringLiteral, ध्रुव string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    यदि (str.रिक्तानि()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } अन्यथा {
        init(nodep, std::max<पूर्णाङ्क>(str.दीर्घता_2() * 8, 1));
        प्रति (अचिह्नित pos = 0; pos < str.दीर्घता_2(); ++pos) {
            ध्रुव पूर्णाङ्क topos = str.दीर्घता_2() - 1 - pos;
            मानम्_And_X& v = m_दत्त.अङ्कः()[topos / 4];
            प्रति (पूर्णाङ्क bit = 0; bit < 8; ++bit) {
                यदि (str[pos] & (1UL << bit)) v.m_मानम् |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_दत्त.m_fromString = सत्य;
    opCleanThis(सत्य);
}

V3_सङ्ख्या::V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_DType* nodedtypep) {
    यदि (nodedtypep->isCHandle()) {
        init(nodep);
        समुच्चयः_Null();
    } अन्यथा यदि (nodedtypep->isDouble()) {
        init(nodep, 64);
        समुच्चयः_Double(0.0);
    } अन्यथा यदि (nodedtypep->isString()) {
        init(nodep);
        समुच्चयः_String("");
    } अन्यथा {
        init(nodep, nodedtypep->विस्तारः(), nodedtypep->विस्तारः_Sized());
    }
}

शून्य V3_सङ्ख्या::create(ध्रुव वर्ण* sourcep) {
    m_दत्त.समुच्चयः_Logic();
    ध्रुव वर्ण* मानम्_startp = sourcep;
    प्रति (ध्रुव वर्ण* cp = sourcep; *cp; ++cp) {
        यदि (*cp == '\'') {
            मानम्_startp = cp + 1;
            भङ्ग;
        }
    }

    तार्किक userSized = असत्य;
    तार्किक unbased = असत्य;
    वर्ण आधारः = '\0';
    यदि (मानम्_startp != sourcep) {  // Has a '
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
            आधारः = *cp;
            ++cp;
        }
        मानम्_startp = cp;

        userSized = widthn.दीर्घता_2() != 0;
        यदि (std::atoi(widthn.c_str())) {
            यदि (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_अङ्कः_विस्तारः()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                विस्तारः(v3Global.opt.max_अङ्कः_विस्तारः(), सत्य);
            } अन्यथा {
                विस्तारः(std::atoi(widthn.c_str()), सत्य);
            }
        }
    } अन्यथा {
        unbased = सत्य;
        आधारः = 'd';
    }

    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    यदि (आधारः == '0') {
        विस्तारः(1, असत्य);  // So we extend it
        समुच्चयः_Bit(0, 0);
        m_दत्त.m_autoExtend = सत्य;
    } अन्यथा यदि (आधारः == '1') {
        विस्तारः(1, असत्य);  // So we extend it
        समुच्चयः_Bit(0, 1);
        m_दत्त.m_autoExtend = सत्य;
    } अन्यथा यदि (std::tolower(आधारः) == 'z') {
        विस्तारः(1, असत्य);  // So we extend it
        समुच्चयः_Bit(0, 'z');
        m_दत्त.m_autoExtend = सत्य;
    } अन्यथा यदि (std::tolower(आधारः) == 'x') {
        विस्तारः(1, असत्य);  // So we extend it
        समुच्चयः_Bit(0, 'x');
        m_दत्त.m_autoExtend = सत्य;
    }
    // Otherwise...
    अन्यथा यदि (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ध्रुव पूर्णाङ्क विस्तारः_Bound = std::max<पूर्णाङ्क>(32, std::वाक्यदैर्घ्यम्(मानम्_startp) * 4);
        विस्तारः(विस्तारः_Bound, असत्य);  // Will change width below
        यदि (unbased) isSigned(सत्य);  // Also says the spec.
    }

    // Ignore leading blanks
    यावत् (*मानम्_startp == '_' || std::रिक्तम्_वा(*मानम्_startp)) ++मानम्_startp;
    यदि (!*मानम्_startp && !m_दत्त.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    यदि (userSized && m_दत्त.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    पूर्णाङ्क obit = 0;  // Start at LSB
    पूर्णाङ्क आधारः_align = 1;
    यदि (std::tolower(आधारः) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        यावत् (*मानम्_startp == '_' || *मानम्_startp == '0') ++मानम्_startp;
        // Convert decimal number to hex
        पूर्णाङ्क olen = 0;
        uint32_t val = 0;
        पूर्णाङ्क got_x = 0;
        पूर्णाङ्क got_z = 0;
        पूर्णाङ्क got_01 = 0;
        तार्किक warnedTooMany = असत्य;
        प्रति (ध्रुव वर्ण* cp = मानम्_startp; *cp; ++cp) {
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
                    m_दत्त.अङ्कः()[0].m_मानम् = val;
                } अन्यथा {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_सङ्ख्या गुणनफलम्(एतत्, विस्तारः() + 4);  // +4 for overflow detection
                    ध्रुव V3_सङ्ख्या ten(एतत्, विस्तारः() + 4, 10);
                    ध्रुव V3_सङ्ख्या addend(एतत्, विस्तारः(), (*cp - '0'));
                    गुणनफलम्.opMul(*एतत्, ten);
                    op_सङ्कलय(गुणनफलम्, addend);
                    यदि (!warnedTooMany && गुणनफलम्.द्वयङ्काः_मानम्(विस्तारः(), 4)) {  // Overflowed
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
                यदि (!userSized) विस्तारः(32, असत्य);
                समुच्चयः_All_द्वयङ्काः_Z();
                भङ्ग;
            }
            पक्ष 'x': {
                got_x = 1;
                यदि (!userSized) विस्तारः(32, असत्य);
                समुच्चयः_All_द्वयङ्काः_X();
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
        obit = विस्तारः();
        यदि ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << मानम्_startp);
        }
    } अन्यथा {
        // Convert bin/octal number to hex
        प्रति (ध्रुव वर्ण* cp = मानम्_startp + std::वाक्यदैर्घ्यम्(मानम्_startp) - 1; cp >= मानम्_startp;
             cp--) {
            यदि (*cp != '_' && *cp != '0' && obit >= विस्तारः()) {
                warnTooMany(sourcep);
                भङ्ग;
            }
            विकल्प (std::tolower(आधारः)) {
            पक्ष 'b': {
                आधारः_align = 1;
                विकल्प (std::tolower(*cp)) {
                पक्ष '0': समुच्चयः_Bit(obit++, 0); भङ्ग;
                पक्ष '1': समुच्चयः_Bit(obit++, 1); भङ्ग;
                पक्ष 'z':
                पक्ष '?': समुच्चयः_Bit(obit++, 'z'); भङ्ग;
                पक्ष 'x': समुच्चयः_Bit(obit++, 'x'); भङ्ग;
                पक्ष '_': भङ्ग;
                स्वभाव: v3error("Illegal character in binary constant: " << *cp);
                }
                भङ्ग;
            }

            पक्ष 'o':
            पक्ष 'c': {
                आधारः_align = 3;
                विकल्प (std::tolower(*cp)) {  // clang-format off
                पक्ष '0': समुच्चयः_Bit(obit++, 0); समुच्चयः_Bit(obit++, 0);  समुच्चयः_Bit(obit++, 0);  भङ्ग;
                पक्ष '1': समुच्चयः_Bit(obit++, 1); समुच्चयः_Bit(obit++, 0);  समुच्चयः_Bit(obit++, 0);  भङ्ग;
                पक्ष '2': समुच्चयः_Bit(obit++, 0); समुच्चयः_Bit(obit++, 1);  समुच्चयः_Bit(obit++, 0);  भङ्ग;
                पक्ष '3': समुच्चयः_Bit(obit++, 1); समुच्चयः_Bit(obit++, 1);  समुच्चयः_Bit(obit++, 0);  भङ्ग;
                पक्ष '4': समुच्चयः_Bit(obit++, 0); समुच्चयः_Bit(obit++, 0);  समुच्चयः_Bit(obit++, 1);  भङ्ग;
                पक्ष '5': समुच्चयः_Bit(obit++, 1); समुच्चयः_Bit(obit++, 0);  समुच्चयः_Bit(obit++, 1);  भङ्ग;
                पक्ष '6': समुच्चयः_Bit(obit++, 0); समुच्चयः_Bit(obit++, 1);  समुच्चयः_Bit(obit++, 1);  भङ्ग;
                पक्ष '7': समुच्चयः_Bit(obit++, 1); समुच्चयः_Bit(obit++, 1);  समुच्चयः_Bit(obit++, 1);  भङ्ग;
                पक्ष 'z':
                पक्ष '?': समुच्चयः_Bit(obit++, 'z'); समुच्चयः_Bit(obit++, 'z'); समुच्चयः_Bit(obit++, 'z'); भङ्ग;
                पक्ष 'x': समुच्चयः_Bit(obit++, 'x'); समुच्चयः_Bit(obit++, 'x'); समुच्चयः_Bit(obit++, 'x'); भङ्ग;
                पक्ष '_': भङ्ग;
                // clang-format on
                स्वभाव: v3error("Illegal character in octal constant");
                }
                भङ्ग;
            }

            पक्ष 'h':  // FALLTHRU
            पक्ष 'x': {
                आधारः_align = 4;
                विकल्प (std::tolower(*cp)) {  // clang-format off
                पक्ष '0': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '1': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '2': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '3': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '4': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '5': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '6': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '7': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); भङ्ग;
                पक्ष '8': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष '9': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'a': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'b': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'c': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'd': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'e': समुच्चयः_Bit(obit++,0); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'f': समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); समुच्चयः_Bit(obit++,1); भङ्ग;
                पक्ष 'z':
                पक्ष '?': समुच्चयः_Bit(obit++,'z'); समुच्चयः_Bit(obit++,'z'); समुच्चयः_Bit(obit++,'z'); समुच्चयः_Bit(obit++,'z'); भङ्ग;
                पक्ष 'x': समुच्चयः_Bit(obit++,'x'); समुच्चयः_Bit(obit++,'x'); समुच्चयः_Bit(obit++,'x'); समुच्चयः_Bit(obit++,'x'); भङ्ग;
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
                v3error("Illegal base character: " << आधारः);
                भङ्ग;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    यदि (!userSized && !m_दत्त.m_autoExtend) {
        विस्तारः(std::max(32, आधारः_align * ((विस्तारः_गन्तव्यम्_Fit() + आधारः_align - 1) / आधारः_align)), असत्य);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    यावत् (obit <= विस्तारः() && obit && bitIsXZ(obit - 1)) {
        समुच्चयः_Bit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(सत्य);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

शून्य V3_सङ्ख्या::warnTooMany(ध्रुव string& मानम्) {
    v3warn(WIDTHDIGITS, "Too many digits for " << विस्तारः() << " bit number: '" << मानम् << "'");
}

शून्य V3_सङ्ख्या::nodep(Ast_ग्रन्थिः* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    यदि (!nodep) प्रत्यागम;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

तार्किक V3_सङ्ख्या::epsilonEqual(द्विगुण a, द्विगुण b) {
    प्रत्यागम std::fabs(a - b)
           <= (std::numeric_limits<द्विगुण>::epsilon() * std::max(1.0, std::max(a, b)));
}

तार्किक V3_सङ्ख्या::epsilonIntegral(द्विगुण a) {
    ध्रुव द्विगुण dint = static_cast<द्विगुण>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    प्रत्यागम epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

पूर्णाङ्क V3_सङ्ख्या::log2b(uint32_t अङ्कः) {
    // See also opCLog2
    प्रति (पूर्णाङ्क bit = 31; bit > 0; bit--) {
        यदि (अङ्कः & (1ULL << bit)) प्रत्यागम bit;
    }
    प्रत्यागम 0;
}

पूर्णाङ्क V3_सङ्ख्या::log2bQuad(uint64_t अङ्कः) {
    यदि (अङ्कः >> 32ULL) प्रत्यागम 32 + log2b(अङ्कः >> 32ULL);
    प्रत्यागम log2b(अङ्कः);
}

//======================================================================
// Setters

V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Zero() {
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, 0};
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Quad(uint64_t मानम्) {
    समुच्चयः_Zero();
    m_दत्त.अङ्कः()[0].m_मानम् = मानम् & 0xffffffffULL;
    यदि (विस्तारः() > 32) m_दत्त.अङ्कः()[1].m_मानम् = (मानम् >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Long(uint32_t मानम्) {
    समुच्चयः_Zero();
    m_दत्त.अङ्कः()[0].m_मानम् = मानम्;
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Long_S(int32_t मानम्) {
    समुच्चयः_Zero();
    सङ्घ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = मानम्;
    (शून्य)u.s;
    m_दत्त.अङ्कः()[0].m_मानम् = u.u;
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Double(द्विगुण मानम्) {
    UASSERT(विस्तारः() == 64, "Real operation on wrong sized number");
    m_दत्त.समुच्चयः_Double();
    सङ्घ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.d = मानम्;
    (शून्य)u.d;
    प्रति (पूर्णाङ्क i = 2; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, 0};
    m_दत्त.अङ्कः()[0].m_मानम् = u.u[0];
    m_दत्त.अङ्कः()[1].m_मानम् = u.u[1];
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Single_द्वयङ्काः(वर्ण मानम्) {
    प्रति (पूर्णाङ्क i = 1 /*ऊर्ध्वस्थम्*/; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, 0};
    m_दत्त.अङ्कः()[0] = {(मानम् == '1' || मानम् == 'x' || मानम् == 1 || मानम् == 3),
                       (मानम् == 'z' || मानम् == 'x' || मानम् == 2 || मानम् == 3)};
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_All_Bits0() {
    समुच्चयः_Zero();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_All_Bits1() {
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {~0U, 0};
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_All_द्वयङ्काः_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {~0U, ~0U};
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_All_द्वयङ्काः_Z() {
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, ~0U};
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_All_द्वयङ्काः_XRemoved() {
    यदि (!v3Global.constRemoveXs()) {
        प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    } अन्यथा {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        यदि (v3Global.opt.x_नियोजय() == "1") {
            प्रत्यागम समुच्चयः_All_Bits1();
        } अन्यथा {
            प्रत्यागम समुच्चयः_All_Bits0();
        }
    }
}
V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_Value1() {
    m_दत्त.अङ्कः()[0] = {1, 0};
    प्रति (पूर्णाङ्क i = 1; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, 0};
    प्रत्यागम *एतत्;
}

शून्य V3_सङ्ख्या::समुच्चयः_Bit_X0(पूर्णाङ्क bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    समुच्चयः_Bit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_सङ्ख्या& V3_सङ्ख्या::समुच्चयः_आवरणम्(पूर्णाङ्क nbits, पूर्णाङ्क lsb) {
    समुच्चयः_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_द्वयङ्काः(lsb, std::max(0, std::min(nbits, विस्तारः() - lsb)), 1);
    प्रत्यागम *एतत्;
}

//======================================================================
// ACCESSORS - as strings

string V3_सङ्ख्या::ascii(तार्किक prefixed, तार्किक cleanVerilog) ध्रुव VL_MT_STABLE {
    // Correct number of zero bits/width matters
    यदि (is1_चरणः()) प्रत्यागम "1step";
    यदि (isNull()) प्रत्यागम "null";
    std::ostringstream बहिर्गमनम्;
    यदि (isDouble()) {
        बहिर्गमनम्.precision(17);
        यदि (VL_UNCOVERABLE(विस्तारः() != 64)) {
            बहिर्गमनम् << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } अन्यथा {
            बहिर्गमनम् << गन्तव्यम्_Double();
            यदि (गन्तव्यम्_Double() == floor(गन्तव्यम्_Double())) बहिर्गमनम् << ".0";
        }
        प्रत्यागम बहिर्गमनम्.str();
    } अन्यथा यदि (isString()) {
        प्रत्यागम '"' + गन्तव्यम्_String() + '"';
    } अन्यथा {
        यदि (VL_UNCOVERABLE((m_दत्त.अङ्कः()[शब्दाः() - 1].m_मानम् | m_दत्त.अङ्कः()[शब्दाः() - 1].m_मानम्_X)
                           & ~hi_Word_आवरणम्())) {
            बहिर्गमनम् << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    यदि (prefixed) {
        यदि (sized()) {
            बहिर्गमनम् << विस्तारः() << "'";
        } अन्यथा यदि (autoExtend() && विस्तारः() == 1) {
            बहिर्गमनम् << "'";
            यदि (bitIs0(0)) {
                बहिर्गमनम् << '0';
            } अन्यथा यदि (bitIs1(0)) {
                बहिर्गमनम् << '1';
            } अन्यथा यदि (bitIsZ(0)) {
                बहिर्गमनम् << 'z';
            } अन्यथा {
                बहिर्गमनम् << 'x';
            }
            प्रत्यागम बहिर्गमनम्.str();
        } अन्यथा {
            यदि (cleanVerilog) {
                बहिर्गमनम् << "'";
            } अन्यथा {
                बहिर्गमनम् << "?" << विस्तारः() << "?";
            }
        }
        यदि (isSigned()) बहिर्गमनम् << "s";
    }

    ध्रुव तार्किक द्विमान = (isFourState()
#यदि_परिभाषितम् V3_सङ्ख्या_ASCII_द्विमान
                         // cppcheck-suppress konwnConditionTrueFalse
                         || सत्य
#यदि_अन्तः
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    यदि (द्विमान) {
        बहिर्गमनम् << "b";
        बहिर्गमनम् << displayed("%0b");
    } अन्यथा {
        यदि (prefixed) बहिर्गमनम् << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        बहिर्गमनम् << displayed("%0h");
    }
    यदि (isNull()) {
        यदि (VL_UNCOVERABLE(!isEqZero())) {
            बहिर्गमनम् << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } अन्यथा {
            बहिर्गमनम् << " [null]";
        }
    }
    प्रत्यागम बहिर्गमनम्.str();
}

तार्किक V3_सङ्ख्या::displayedFmtHasArg(वर्ण format, तार्किक is_अवलोकनम्) {
    // Is this a format letter that takes an argument in runtime?
    (शून्य)is_अवलोकनम्;
    विकल्प (std::tolower(format)) {
    पक्ष 'b': प्रत्यागम सत्य;  //द्विमान
    पक्ष 'c': प्रत्यागम सत्य;  // Character
    पक्ष 'd': प्रत्यागम सत्य;  // Decimal
    पक्ष 'e': प्रत्यागम सत्य;  // Floating
    पक्ष 'f': प्रत्यागम सत्य;  // Floating
    पक्ष 'g': प्रत्यागम सत्य;  // Floating
    पक्ष 'h': प्रत्यागम सत्य;  // Hex
    पक्ष 'o': प्रत्यागम सत्य;  // Octal
    पक्ष 'p': प्रत्यागम सत्य;  //आदर्शः
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

string V3_सङ्ख्या::displayPad(आकार_t fmtsize, वर्ण pad, तार्किक वामतः, ध्रुव string& अन्तर्) VL_विशुद्धम् {
    string padding;
    यदि (अन्तर्.दीर्घता_2() < fmtsize) padding = string(fmtsize - अन्तर्.दीर्घता_2(), pad);
    प्रत्यागम वामतः ? (अन्तर् + padding) : (padding + अन्तर्);
}

string V3_सङ्ख्या::displayedEnumName(ध्रुव AstEnumItem* itemp) {
    ध्रुव string name = VIdProtect::protect(itemp->prettyName());
    प्रत्यागम VString::isIdentifier(name) && !std::अङ्कः_वा(static_cast<अचिह्नित वर्ण>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_सङ्ख्या::displayedEnumName(ध्रुव AstEnumDType* dtypep) ध्रुव VL_MT_STABLE {
    V3_सङ्ख्या मानम्{एतत्, dtypep->विस्तारः()};
    मानम्.op_नियोजय(*एतत्);
    प्रति (ध्रुव AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
        यदि (!constp->अङ्कः().isAnyXZ() && मानम्.isCaseEq(constp->अङ्कः()))
            प्रत्यागम displayedEnumName(itemp);
    }
    प्रत्यागम "";
}

string V3_सङ्ख्या::displayedEnum(ध्रुव AstSFormatArg* argp,
                               ध्रुव string& vformat) ध्रुव VL_MT_STABLE {
    ध्रुव AstEnumDType* ध्रुव dtypep = VN_AS(argp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType);
    V3_सङ्ख्या मानम्{एतत्, dtypep->विस्तारः()};
    मानम्.op_नियोजय(*एतत्);
    ध्रुव VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ध्रुव वर्ण fmt = std::tolower(vformat.पृष्ठभागः());
    यदि (fmt != 'p' && fmt != 's') प्रत्यागम मानम्.displayed(argp, vformat, numericAttr);
    string name = मानम्.displayedEnumName(dtypep);
    यदि (name.रिक्तानि()) name = मानम्.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_सङ्ख्या name_अङ्कः{एतत्};
    name_अङ्कः.समुच्चयः_String(name);
    प्रत्यागम name_अङ्कः.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_सङ्ख्या::displayed(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& vformat,
                           ध्रुव VFormatAttr& formatAttr) ध्रुव VL_MT_STABLE {
    प्रत्यागम displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_सङ्ख्या::displayed(File_पङ्क्तिः* fl, ध्रुव string& vformat,
                           ध्रुव VFormatAttr& formatAttr) ध्रुव VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    स्वचालित pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *एतत्);
    ++pos;
    तार्किक वामतः = असत्य;
    यदि (pos[0] == '-') {
        वामतः = सत्य;
        ++pos;
    }
    string fmtsize;
    प्रति (; pos != vformat.cend() && (std::अङ्कः_वा(pos[0]) || pos[0] == '.'); ++pos) {
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
            V3_सङ्ख्या strwide{fl, static_cast<पूर्णाङ्क>(गन्तव्यम्_String().दीर्घता_2()) * 8, 0};
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
            V3_सङ्ख्या nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*एतत्);
            प्रत्यागम nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    विकल्प (fmt) {
    पक्ष 'c': {
        यदि (विस्तारः() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ध्रुव अचिह्नित पूर्णाङ्क v = द्वयङ्काः_मानम्(0, 8);
        str.प्रक्षिप_पृष्ठभागः(static_cast<वर्ण>(v));
        प्रत्यागम str;
    }
    पक्ष 'e':  // FALLTHRU
    पक्ष 'f':  // FALLTHRU
    पक्ष 'g': {
        ध्रुव द्विगुण n = formatAttr.isDouble()   ? गन्तव्यम्_Double()
                         : formatAttr.isSigned() ? गन्तव्यम्_SQuad()
                                                 : गन्तव्यम्_UQuad();
        वर्ण tmp[MAX_SPRINTF_DOUBLE_आकार];
        (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_आकार, vformat.c_str(), n);
        प्रत्यागम tmp;
    }
    पक्ष 's': {
        यदि (formatAttr.isString() || formatAttr.isComplex()) {
            str = गन्तव्यम्_String();
        } अन्यथा {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            पूर्णाङ्क bit = विस्तारः() - 1;
            तार्किक start = सत्य;
            यावत् ((bit % 8) != 7) ++bit;
            प्रति (; bit >= 0; bit -= 8) {
                ध्रुव पूर्णाङ्क v = द्वयङ्काः_मानम्(bit - 7, 8);
                यदि (!start || v) {
                    str += static_cast<वर्ण>((v == 0) ? ' ' : v);
                    start = असत्य;  // Drop leading 0s
                } अन्यथा {
                    यदि (fmtsize != "0") str += ' ';
                }
            }
        }
        ध्रुव आकार_t fmtsizen = static_cast<आकार_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', वामतः, str);
        प्रत्यागम str;
    }
    पक्ष 'p': {  //आदर्शः
        // 'p' with NUMBER was earlier converted to 'd'
        यदि (formatAttr.isDouble()) {
            ध्रुव द्विगुण n = formatAttr.isDouble()   ? गन्तव्यम्_Double()
                             : formatAttr.isSigned() ? गन्तव्यम्_SQuad()
                                                     : गन्तव्यम्_UQuad();
            वर्ण tmp[MAX_SPRINTF_DOUBLE_आकार];
            (शून्य)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_आकार, "%g", n);
            प्रत्यागम tmp;
        }
        यदि (formatAttr.isString())
            प्रत्यागम '"' + V3_बहिर्गमनम्_Formatter::quoteNameControls(गन्तव्यम्_String()) + '"';
        यदि (formatAttr.isComplex()) प्रत्यागम गन्तव्यम्_String();
        प्रत्यागम "%p";
    }
    पक्ष 'b':  // FALLTHRU
    पक्ष 'o':  // FALLTHRU
    पक्ष 'h':  // FALLTHRU
    पक्ष 'x': {
        पूर्णाङ्क bit = विस्तारः() - 1;
        यदि (वामतः || !fmtsize.रिक्तानि()) {
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
                ध्रुव पूर्णाङ्क अङ्कः_X = गणनम्_X(bit - 2, 3);
                ध्रुव पूर्णाङ्क अङ्कः_Z = गणनम्_Z(bit - 2, 3);
                यदि (अङ्कः_X == 3 || अङ्कः_X == विस्तारः() - (bit - 2)) {
                    str += 'x';
                    अनुवर्त;
                }
                यदि (अङ्कः_Z == 3 || अङ्कः_Z == विस्तारः() - (bit - 2)) {
                    str += 'z';
                    अनुवर्त;
                }
                यदि (अङ्कः_X > 0) {
                    str += 'X';
                    अनुवर्त;
                }
                यदि (अङ्कः_Z > 0) {
                    str += 'Z';
                    अनुवर्त;
                }
                ध्रुव पूर्णाङ्क v = द्वयङ्काः_मानम्(bit - 2, 3);
                str += static_cast<वर्ण>('0' + v);
            }
            भङ्ग;
        }
        स्वभाव: {  // h/x
            यावत् ((bit % 4) != 3) ++bit;
            प्रति (; bit > 0; bit -= 4) {
                ध्रुव पूर्णाङ्क अङ्कः_X = गणनम्_X(bit - 3, 4);
                ध्रुव पूर्णाङ्क अङ्कः_Z = गणनम्_Z(bit - 3, 4);
                यदि (अङ्कः_X == 4 || अङ्कः_X == विस्तारः() - (bit - 3)) {
                    str += 'x';
                    अनुवर्त;
                }
                यदि (अङ्कः_Z == 4 || अङ्कः_Z == विस्तारः() - (bit - 3)) {
                    str += 'z';
                    अनुवर्त;
                }
                यदि (अङ्कः_X > 0) {
                    str += 'X';
                    अनुवर्त;
                }
                यदि (अङ्कः_Z > 0) {
                    str += 'Z';
                    अनुवर्त;
                }
                ध्रुव पूर्णाङ्क v = द्वयङ्काः_मानम्(bit - 3, 4);
                यदि (v >= 10) {
                    str += static_cast<वर्ण>('a' + v - 10);
                } अन्यथा {
                    str += static_cast<वर्ण>('0' + v);
                }
            }
            भङ्ग;
        }
        }  // switch
        ध्रुव आकार_t fmtsizen = static_cast<आकार_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (वामतः ? ' ' : '0'), वामतः, str);
        प्रत्यागम str;
    }  // case b/d/x/o
    पक्ष 't':  // Time
    पक्ष 'd': {  // Unsigned decimal
        ध्रुव तार्किक issigned = formatAttr.isSigned();
        यदि (fmtsize == "" && !वामतः) {
            ध्रुव द्विगुण mantissabits = विस्तारः() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            द्विगुण dchars = mantissabits / 3.321928094887362 + 1.0;
            यदि (issigned) ++dchars;  // space for sign
            fmtsize = cvt_गन्तव्यम्_Str(static_cast<पूर्णाङ्क>(dchars));
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
                यदि (विस्तारः() > 64) {
                    str = गन्तव्यम्_Decimal_S();
                } अन्यथा {
                    str = cvt_गन्तव्यम्_Str(गन्तव्यम्_SQuad());
                }
            } अन्यथा {
                यदि (विस्तारः() > 64) {
                    str = गन्तव्यम्_Decimal_U();
                } अन्यथा {
                    str = cvt_गन्तव्यम्_Str(गन्तव्यम्_UQuad());
                }
            }
        }
        ध्रुव तार्किक zeropad = fmtsize.दीर्घता_2() > 0 && fmtsize[0] == '0' && !वामतः;
        // fmtsize might have changed since we parsed the %fmtsize
        ध्रुव आकार_t fmtsizen = static_cast<आकार_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), वामतः, str);
        प्रत्यागम str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    पक्ष 'u': {  // Packed 2-state
        प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
            ध्रुव uint32_t v = m_दत्त.अङ्कः()[i].m_मानम्;
            str += static_cast<वर्ण>((v >> 0) & 0xff);
            str += static_cast<वर्ण>((v >> 8) & 0xff);
            str += static_cast<वर्ण>((v >> 16) & 0xff);
            str += static_cast<वर्ण>((v >> 24) & 0xff);
        }
        प्रत्यागम str;
    }
    पक्ष 'z': {  // Packed 4-state
        प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
            ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
            str += static_cast<वर्ण>((v.m_मानम् >> 0) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम् >> 8) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम् >> 16) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम् >> 24) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम्_X >> 0) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम्_X >> 8) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम्_X >> 16) & 0xff);
            str += static_cast<वर्ण>((v.m_मानम्_X >> 24) & 0xff);
        }
        प्रत्यागम str;
    }
    पक्ष 'v': {  // Strength
        पूर्णाङ्क bit = विस्तारः() - 1;
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

string V3_सङ्ख्या::emitC() ध्रुव VL_MT_STABLE {
    constexpr आकार_t bufsize = 1000;
    वर्ण sbuf[bufsize];
    string परिणाम;
    यदि (isNull()) {
        प्रत्यागम "VlNull{}";
    } अन्यथा यदि (isDouble()) {
        ध्रुव द्विगुण dnum = गन्तव्यम्_Double();
        यदि (std::isinf(dnum)) {
            यदि (std::signbit(dnum)) परिणाम += '-';
            परिणाम += "std::numeric_limits<double>::infinity()";
        } अन्यथा यदि (std::isnan(dnum)) {
            यदि (std::signbit(dnum)) परिणाम += '-';
            परिणाम += "std::numeric_limits<double>::quiet_NaN()";
        } अन्यथा {
            ध्रुव वर्ण* ध्रुव fmt = (static_cast<पूर्णाङ्क>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            प्रत्यागम sbuf;
        }
    } अन्यथा यदि (isString()) {
        ध्रुव string strg = गन्तव्यम्_String();
        ध्रुव string quoted
            = V3_बहिर्गमनम्_Formatter::quoteNameControls(strg, V3_बहिर्गमनम्_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        प्रत्यागम '"' + quoted + "\"s";
    } अन्यथा यदि (शब्दाः() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        परिणाम = "::VlWide<" + std::गन्तव्यम्_string(शब्दाः()) + ">{{";
        यदि (शब्दाः() > 4) परिणाम += '\n';
        प्रति (पूर्णाङ्क n = 0; n < शब्दाः(); ++n) {
            यदि (n) परिणाम += ((n % 4) ? ", " : ",\n");
            (शून्य)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            परिणाम += sbuf;
        }
        यदि (शब्दाः() > 4) परिणाम += '\n';
        परिणाम += "}}";
    } अन्यथा यदि (शब्दाः() == 2) {  // Quad
        ध्रुव uint64_t qnum = static_cast<uint64_t>(गन्तव्यम्_UQuad());
        ध्रुव वर्ण* ध्रुव fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        प्रत्यागम sbuf;
    } अन्यथा {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ध्रुव uint32_t unum = गन्तव्यम्_UInt();
        ध्रुव वर्ण* ध्रुव fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (विस्तारः() > 16) ? ("0x%08" PRIx32 "U")
                                : (विस्तारः() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (शून्य)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        प्रत्यागम sbuf;
    }
    प्रत्यागम परिणाम;
}

string V3_सङ्ख्या::गन्तव्यम्_Decimal_S() ध्रुव VL_MT_STABLE {
    // Correct number of zero bits/width matters
    यदि (isNegative()) {
        V3_सङ्ख्या lhsNoSign = *एतत्;
        lhsNoSign.opNegate(*एतत्);
        प्रत्यागम "-"s + lhsNoSign.गन्तव्यम्_Decimal_U();
    } अन्यथा {
        प्रत्यागम गन्तव्यम्_Decimal_U();
    }
}

string V3_सङ्ख्या::गन्तव्यम्_Decimal_U() ध्रुव VL_MT_STABLE {
    ध्रुव पूर्णाङ्क maxdecwidth = (विस्तारः() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_सङ्ख्या bcd(एतत्, maxdecwidth + 4);
    V3_सङ्ख्या tmp(एतत्, maxdecwidth + 4);
    V3_सङ्ख्या tmp2(एतत्, maxdecwidth + 4);

    पूर्णाङ्क from_bit = विस्तारः() - 1;
    // Skip all leading zeros
    प्रति (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    प्रति (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        प्रति (पूर्णाङ्क nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            यदि (bcd.द्वयङ्काः_मानम्(nibble_bit, 4) >= 5) {
                tmp2.समुच्चयः_All_Bits0();
                tmp2.समुच्चयः_Bit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.समुच्चयः_Bit(nibble_bit + 1, 1);
                tmp.op_नियोजय(bcd);
                bcd.op_सङ्कलय(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_नियोजय(bcd);
        bcd.op_सरणम्_L(tmp, V3_सङ्ख्या(एतत्, 32, 1));
        // bcd[0] = this[from_bit]
        यदि (bitIs1(from_bit)) bcd.समुच्चयः_Bit(0, 1);
    }

    string output;
    पूर्णाङ्क lsb = (maxdecwidth - 1) & ~3;
    प्रति (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        यदि (bcd.द्वयङ्काः_मानम्(lsb, 4)) भङ्ग;
    }
    प्रति (; lsb >= 0; lsb -= 4) {
        output += static_cast<वर्ण>('0' + bcd.द्वयङ्काः_मानम्(lsb, 4));  // 0..9
    }
    प्रत्यागम output;
}

string V3_सङ्ख्या::गन्तव्यम्_Real_String() ध्रुव VL_MT_STABLE {
    ध्रुव द्विगुण मानम् = गन्तव्यम्_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream बहिर्गमनम्;
    बहिर्गमनम्.precision(15);
    बहिर्गमनम् << मानम्;
    string परिणाम = बहिर्गमनम्.str();
    यदि (std::strtod(परिणाम.c_str(), शून्यसूचकः) != मानम्) {
        बहिर्गमनम्.str("");
        बहिर्गमनम्.precision(17);
        बहिर्गमनम् << मानम्;
        परिणाम = बहिर्गमनम्.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    यदि (परिणाम.find_first_सम्बन्धि(".en") == string::npos) परिणाम += ".0";
    प्रत्यागम परिणाम;
}

//======================================================================
// ACCESSORS - as numbers

तार्किक V3_सङ्ख्या::माति_अन्तर्_UInt() ध्रुव VL_MT_SAFE {
    यदि (isFourState()) प्रत्यागम असत्य;
    // We allow wide numbers that represent values <= 32 bits
    प्रति (पूर्णाङ्क i = 1; i < शब्दाः(); ++i) {
        यदि (m_दत्त.अङ्कः()[i].m_मानम्) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
uint32_t V3_सङ्ख्या::गन्तव्यम्_UInt() ध्रुव VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *एतत्);
    // We allow wide numbers that represent values <= 32 bits
    यदि (VL_UNCOVERABLE(!माति_अन्तर्_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *एतत्);
    }
    प्रत्यागम m_दत्त.अङ्कः()[0].m_मानम्;
}

द्विगुण V3_सङ्ख्या::गन्तव्यम्_Double() ध्रुव VL_MT_SAFE {
    UASSERT(isDouble() && विस्तारः() == 64, "Real operation on wrong sized/non-real number");
    सङ्घ {
        द्विगुण d;
        uint32_t u[2];
    } u;
    u.u[0] = m_दत्त.अङ्कः()[0].m_मानम्;
    u.u[1] = m_दत्त.अङ्कः()[1].m_मानम्;
    प्रत्यागम u.d;
}

int32_t V3_सङ्ख्या::गन्तव्यम्_SInt() ध्रुव VL_MT_SAFE {
    यदि (isSigned()) {
        ध्रुव uint32_t v = गन्तव्यम्_UInt();
        ध्रुव uint32_t signExtend = (-(v & (1UL << (विस्तारः() - 1))));
        ध्रुव uint32_t extended = v | signExtend;
        प्रत्यागम static_cast<int32_t>(extended);
    } अन्यथा {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        प्रत्यागम static_cast<int32_t>(गन्तव्यम्_UInt());
    }
}

uint64_t V3_सङ्ख्या::गन्तव्यम्_UQuad() ध्रुव VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *एतत्);
    // We allow wide numbers that represent values <= 64 bits
    यदि (isDouble()) प्रत्यागम static_cast<uint64_t>(गन्तव्यम्_Double());
    प्रति (पूर्णाङ्क i = 2; i < शब्दाः(); ++i) {
        यदि (m_दत्त.अङ्कः()[i].m_मानम्) {
            v3error("Value too wide for 64-bits expected in this context " << *एतत्);
            भङ्ग;
        }
    }
    यदि (विस्तारः() <= 32) प्रत्यागम (static_cast<uint64_t>(गन्तव्यम्_UInt()));
    प्रत्यागम ((static_cast<uint64_t>(m_दत्त.अङ्कः()[1].m_मानम्) << 32ULL)
            | (static_cast<uint64_t>(m_दत्त.अङ्कः()[0].m_मानम्)));
}

int64_t V3_सङ्ख्या::गन्तव्यम्_SQuad() ध्रुव {
    यदि (isDouble()) प्रत्यागम static_cast<int64_t>(गन्तव्यम्_Double());
    ध्रुव uint64_t v = गन्तव्यम्_UQuad();
    ध्रुव uint64_t signExtend = (-(v & (1ULL << (विस्तारः() - 1))));
    ध्रुव uint64_t extended = v | signExtend;
    प्रत्यागम static_cast<int64_t>(extended);
}

string V3_सङ्ख्या::गन्तव्यम्_String() ध्रुव {
    UASSERT(!isFourState(), "toString with 4-state " << *एतत्);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    यदि (isString()) प्रत्यागम m_दत्त.str();
    पूर्णाङ्क bit = विस्तारः() - 1;
    यावत् ((bit % 8) != 7) ++bit;
    std::string str;
    प्रति (; bit >= 0; bit -= 8) {
        ध्रुव पूर्णाङ्क v = द्वयङ्काः_मानम्(bit - 7, 8);
        यदि (v) str += static_cast<वर्ण>(v);
    }
    प्रत्यागम str;
}

uint32_t V3_सङ्ख्या::edataWord(पूर्णाङ्क eword) ध्रुव {
    UASSERT(!isFourState(), "edataWord with 4-state " << *एतत्);
    प्रत्यागम m_दत्त.अङ्कः()[eword].m_मानम्;
}

uint8_t V3_सङ्ख्या::दत्त_Byte(पूर्णाङ्क byte) ध्रुव {
    प्रत्यागम (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

तार्किक V3_सङ्ख्या::isAllZ() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        ध्रुव uint32_t आवरणम् = i == शब्दाः() - 1 ? hi_Word_आवरणम्() : ~0U;
        यदि (((~v.m_मानम् & v.m_मानम्_X) & आवरणम्) != आवरणम्) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_सङ्ख्या::isAllX() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    uint32_t आवरणम् = hi_Word_आवरणम्();
    प्रति (पूर्णाङ्क i = शब्दाः() - 1; i >= 0; --i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        यदि ((v.m_मानम् & v.m_मानम्_X) ^ आवरणम्) प्रत्यागम असत्य;
        आवरणम् = ~0U;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_सङ्ख्या::isEqZero() ध्रुव VL_MT_SAFE {
    यदि (isString()) प्रत्यागम m_दत्त.str().रिक्तानि();
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        यदि (v.m_मानम् || v.m_मानम्_X) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_सङ्ख्या::isNeqZero() ध्रुव {
    यदि (isString()) प्रत्यागम !m_दत्त.str().रिक्तानि();
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        यदि (v.m_मानम् & ~v.m_मानम्_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_सङ्ख्या::is_द्वयङ्काः_Zero(पूर्णाङ्क msb, पूर्णाङ्क lsb) ध्रुव {
    प्रति (पूर्णाङ्क i = lsb; i <= msb; ++i) {
        यदि (VL_UNLIKELY(!bitIs0(i))) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_सङ्ख्या::isEqOne() ध्रुव {
    यदि (m_दत्त.अङ्कः()[0].m_मानम् != 1 || m_दत्त.अङ्कः()[0].m_मानम्_X) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 1; i < शब्दाः(); ++i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        यदि (v.m_मानम् || v.m_मानम्_X) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_सङ्ख्या::isEqAllOnes(पूर्णाङ्क optwidth) ध्रुव {
    // Correct number of zero bits/width matters
    यदि (!optwidth) optwidth = विस्तारः();
    प्रति (पूर्णाङ्क bit = 0; bit < optwidth; ++bit) {
        यदि (!bitIs1(bit)) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}
तार्किक V3_सङ्ख्या::isFourState() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        यदि (m_दत्त.अङ्कः()[i].m_मानम्_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_सङ्ख्या::isAnyX() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        यदि (v.m_मानम् & v.m_मानम्_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_सङ्ख्या::isAnyXZ() ध्रुव {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        यदि (m_दत्त.अङ्कः()[i].m_मानम्_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_सङ्ख्या::isAnyZ() ध्रुव VL_MT_SAFE {
    यदि (isDouble() || isString()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
        यदि (~v.m_मानम् & v.m_मानम्_X) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}
तार्किक V3_सङ्ख्या::isLtXZ(ध्रुव V3_सङ्ख्या& rhs) ध्रुव {
    // Include X/Z in comparisons for sort ordering
    यदि (isString()) प्रत्यागम गन्तव्यम्_String() < rhs.गन्तव्यम्_String();
    यदि (isDouble()) प्रत्यागम गन्तव्यम्_Double() < rhs.गन्तव्यम्_Double();
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (bitIs1(bit) && rhs.bitIs0(bit)) प्रत्यागम सत्य;
        यदि (rhs.bitIs1(bit) && bitIs0(bit)) प्रत्यागम असत्य;
        यदि (bitIsXZ(bit)) प्रत्यागम सत्य;
        यदि (rhs.bitIsXZ(bit)) प्रत्यागम असत्य;
    }
    प्रत्यागम असत्य;
}
पूर्णाङ्क V3_सङ्ख्या::गणनम्_X(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE {
    पूर्णाङ्क गणनम् = 0;
    प्रति (पूर्णाङ्क bitn = 0; bitn < nbits; ++bitn) {
        यदि (lsb + bitn >= विस्तारः()) प्रत्यागम गणनम्;
        यदि (bitIsX(lsb + bitn)) ++गणनम्;
    }
    प्रत्यागम गणनम्;
}
पूर्णाङ्क V3_सङ्ख्या::गणनम्_Z(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE {
    पूर्णाङ्क गणनम् = 0;
    प्रति (पूर्णाङ्क bitn = 0; bitn < nbits; ++bitn) {
        यदि (lsb + bitn >= विस्तारः()) प्रत्यागम गणनम्;
        यदि (bitIsZ(lsb + bitn)) ++गणनम्;
    }
    प्रत्यागम गणनम्;
}

पूर्णाङ्क V3_सङ्ख्या::विस्तारः_गन्तव्यम्_Fit() ध्रुव {
    प्रति (पूर्णाङ्क bit = विस्तारः() - 1; bit > 0; bit--) {
        यदि (!bitIs0(bit)) प्रत्यागम bit + 1;
    }
    प्रत्यागम 1;  // one bit even if number is == 0
}

uint32_t V3_सङ्ख्या::गणनम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& ctrl) ध्रुव {
    // Correct number of zero bits/width matters
    पूर्णाङ्क n = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
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

uint32_t V3_सङ्ख्या::गणनम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& ctrl1, ध्रुव V3_सङ्ख्या& ctrl2,
                             ध्रुव V3_सङ्ख्या& ctrl3) ध्रुव {
    // Correct number of zero bits/width matters
    पूर्णाङ्क n = गणनम्_द्वयङ्काः(ctrl1);
    यदि (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += गणनम्_द्वयङ्काः(ctrl2);
    यदि ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += गणनम्_द्वयङ्काः(ctrl3);
    }
    प्रत्यागम n;
}

uint32_t V3_सङ्ख्या::गणनम्_Ones() ध्रुव {
    पूर्णाङ्क n = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (bitIs1(bit)) ++n;
    }
    प्रत्यागम n;
}

uint32_t V3_सङ्ख्या::most_समुच्चयः_Bit_P1() ध्रुव {
    प्रति (पूर्णाङ्क bit = विस्तारः() - 1; bit >= 0; bit--) {
        यदि (!bitIs0(bit)) प्रत्यागम bit + 1;
    }
    प्रत्यागम 0;
}

uint32_t V3_सङ्ख्या::न्यूनतमम्_समुच्चयः_Bit_P1() ध्रुव {
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (!bitIs0(bit)) प्रत्यागम bit + 1;
    }
    प्रत्यागम 0;
}

//======================================================================

V3_सङ्ख्या& V3_सङ्ख्या::op_द्वयङ्काः_Non_XZ(ध्रुव V3_सङ्ख्या& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIs0(bit) || lhs.bitIs1(bit)) समुच्चयः_Bit(bit, 1);
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_द्वयङ्काः_One(ध्रुव V3_सङ्ख्या& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit)) समुच्चयः_Bit(bit, 1);
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_द्वयङ्काः_XZ(ध्रुव V3_सङ्ख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIsXZ(bit)) समुच्चयः_Bit(bit, 1);
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_द्वयङ्काः_Z(ध्रुव V3_सङ्ख्या& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIsZ(bit)) समुच्चयः_Bit(bit, 1);
    }
    प्रत्यागम *एतत्;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_सङ्ख्या& V3_सङ्ख्या::opRedOr(ध्रुव V3_सङ्ख्या& lhs) {
    // op i, 1 bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            प्रत्यागम समुच्चयः_Single_द्वयङ्काः(1);
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opRedAnd(ध्रुव V3_सङ्ख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.विस्तारः(); ++bit) {
        यदि (lhs.bitIs0(bit)) {
            प्रत्यागम समुच्चयः_Single_द्वयङ्काः(0);
        } अन्यथा यदि (lhs.bitIs1(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opRedXor(ध्रुव V3_सङ्ख्या& lhs) {
    // op i, 1 bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.विस्तारः(); ++bit) {
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
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::op_गणनम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& व्यञ्जकम्, ध्रुव V3_सङ्ख्या& ctrl1, ध्रुव V3_सङ्ख्या& ctrl2,
                                ध्रुव V3_सङ्ख्या& ctrl3) {
    // Correct number of zero bits/width matters
    अङ्कः_ASSERT_OP_ARGS4(व्यञ्जकम्, ctrl1, ctrl2, ctrl3);
    अङ्कः_ASSERT_LOGIC_ARGS4(व्यञ्जकम्, ctrl1, ctrl2, ctrl3);
    समुच्चयः_Zero();
    m_दत्त.अङ्कः()[0].m_मानम् = व्यञ्जकम्.गणनम्_द्वयङ्काः(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_गणनम्_Ones(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    समुच्चयः_Zero();
    m_दत्त.अङ्कः()[0].m_मानम् = lhs.गणनम्_Ones();
    opCleanThis();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opIsUnknown(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.isAnyXZ());
}
V3_सङ्ख्या& V3_सङ्ख्या::opOneHot(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गणनम्_Ones() == 1);
}
V3_सङ्ख्या& V3_सङ्ख्या::opOneHot0(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गणनम्_Ones() <= 1);
}
V3_सङ्ख्या& V3_सङ्ख्या::opCLog2(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ध्रुव पूर्णाङ्क adjust = (lhs.गणनम्_Ones() == 1) ? 0 : 1;
    प्रति (पूर्णाङ्क bit = lhs.विस्तारः() - 1; bit >= 0; bit--) {
        यदि (lhs.bitIs1(bit)) {
            समुच्चयः_Long(bit + adjust);
            प्रत्यागम *एतत्;
        }
    }
    समुच्चयः_Zero();
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_Most_समुच्चयः_Bit_P1(ध्रुव V3_सङ्ख्या& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    प्रति (पूर्णाङ्क bit = lhs.विस्तारः() - 1; bit >= 0; bit--) {
        यदि (lhs.bitIs1(bit)) {
            समुच्चयः_Long(bit + 1);
            प्रत्यागम *एतत्;
        }
    }
    समुच्चयः_Zero();
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogNot(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            outc = 0;
            गच्छ चरमः;
        } अन्यथा यदि (lhs.bitIs0(bit)) {
        } अन्यथा {
            outc = 'x';
        }
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opNot(ध्रुव V3_सङ्ख्या& lhs) {
    // Correct number of zero bits/width matters
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIs0(bit)) {
            समुच्चयः_Bit(bit, 1);
        } अन्यथा यदि (lhs.bitIsXZ(bit)) {
            समुच्चयः_Bit(bit, 'x');
        }
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opAnd(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            समुच्चयः_Bit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } अन्यथा {
            समुच्चयः_Bit(bit, 'x');
        }
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opOr(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            समुच्चयः_Bit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } अन्यथा {
            समुच्चयः_Bit(bit, 'x');
        }
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opXor(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            समुच्चयः_Bit(bit, 1);
        } अन्यथा यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            समुच्चयः_Bit(bit, 1);
        } अन्यथा यदि (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            समुच्चयः_Bit(bit, 'x');
        }
        // else zero
    }
    प्रत्यागम *एतत्;
}

शून्य V3_सङ्ख्या::fill_द्वयङ्काः(पूर्णाङ्क destLsb, पूर्णाङ्क विस्तारः, वर्ण मानम्) {
    UASSERT(destLsb >= 0 && विस्तारः >= 0, "Negative bit range");
    यदि (!विस्तारः) प्रत्यागम;
    UASSERT(विस्तारः <= एतत्->विस्तारः() - destLsb, "Destination bit range exceeds number width");

    ध्रुव uint32_t मानम्_Word = मानम् == '1' || मानम् == 'x' || मानम् == 1 || मानम् == 3 ? ~0U : 0;
    ध्रुव uint32_t xWord = मानम् == 'z' || मानम् == 'x' || मानम् == 2 || मानम् == 3 ? ~0U : 0;
    ध्रुव पूर्णाङ्क firstWord = VL_BITWORD_E(destLsb);
    ध्रुव पूर्णाङ्क चरमः_Word = VL_BITWORD_E(destLsb + विस्तारः - 1);
    प्रति (पूर्णाङ्क word = firstWord; word <= चरमः_Word; ++word) {
        ध्रुव पूर्णाङ्क wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ध्रुव पूर्णाङ्क wordMsb = word == चरमः_Word ? VL_BITBIT_E(destLsb + विस्तारः - 1) : VL_SIZEBITS_E;
        ध्रुव uint32_t आवरणम् = VL_आवरणम्_E(wordMsb - wordLsb + 1) << wordLsb;
        मानम्_And_X& dest = m_दत्त.अङ्कः()[word];
        dest.m_मानम् = (dest.m_मानम् & ~आवरणम्) | (मानम्_Word & आवरणम्);
        dest.m_मानम्_X = (dest.m_मानम्_X & ~आवरणम्) | (xWord & आवरणम्);
    }
}

शून्य V3_सङ्ख्या::copy_द्वयङ्काः(पूर्णाङ्क destLsb, ध्रुव V3_सङ्ख्या& source, पूर्णाङ्क sourceLsb, पूर्णाङ्क विस्तारः) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && विस्तारः >= 0, "Negative bit range");
    यदि (!विस्तारः) प्रत्यागम;
    UASSERT(विस्तारः <= एतत्->विस्तारः() - destLsb, "Destination bit range exceeds number width");
    UASSERT(विस्तारः <= source.विस्तारः() - sourceLsb, "Source bit range exceeds number width");

    ध्रुव पूर्णाङ्क firstWord = VL_BITWORD_E(destLsb);
    ध्रुव पूर्णाङ्क चरमः_Word = VL_BITWORD_E(destLsb + विस्तारः - 1);
    प्रति (पूर्णाङ्क destWord = firstWord; destWord <= चरमः_Word; ++destWord) {
        ध्रुव पूर्णाङ्क destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        ध्रुव पूर्णाङ्क dest_विस्थापनम् = VL_BITBIT_E(destBit);
        ध्रुव पूर्णाङ्क sourceBit = sourceLsb + destBit - destLsb;
        ध्रुव पूर्णाङ्क sourceWord = VL_BITWORD_E(sourceBit);
        ध्रुव पूर्णाङ्क source_विस्थापनम् = VL_BITBIT_E(sourceBit);
        ध्रुव पूर्णाङ्क copy_विस्तारः = std::min(विस्तारः - (destBit - destLsb), VL_EDATASIZE - dest_विस्थापनम्);
        ध्रुव uint32_t आवरणम् = VL_आवरणम्_E(copy_विस्तारः) << dest_विस्थापनम्;

        ध्रुव मानम्_And_X& sourceLo = source.m_दत्त.अङ्कः()[sourceWord];
        uint32_t मानम् = sourceLo.m_मानम् >> source_विस्थापनम्;
        uint32_t मानम्_X = sourceLo.m_मानम्_X >> source_विस्थापनम्;
        यदि (source_विस्थापनम् && copy_विस्तारः > VL_EDATASIZE - source_विस्थापनम्) {
            ध्रुव मानम्_And_X& sourceHi = source.m_दत्त.अङ्कः()[sourceWord + 1];
            मानम् |= sourceHi.m_मानम् << (VL_EDATASIZE - source_विस्थापनम्);
            मानम्_X |= sourceHi.m_मानम्_X << (VL_EDATASIZE - source_विस्थापनम्);
        }

        मानम्_And_X& dest = m_दत्त.अङ्कः()[destWord];
        dest.m_मानम् = (dest.m_मानम् & ~आवरणम्) | ((मानम् << dest_विस्थापनम्) & आवरणम्);
        dest.m_मानम्_X = (dest.m_मानम्_X & ~आवरणम्) | ((मानम्_X << dest_विस्थापनम्) & आवरणम्);
    }
}

V3_सङ्ख्या& V3_सङ्ख्या::opConcat(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    समुच्चयः_Zero();
    // See also error in V3Width
    यदि (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_द्वयङ्काः(0, rhs, 0, rhs.विस्तारः());
    copy_द्वयङ्काः(rhs.विस्तारः(), lhs, 0, lhs.विस्तारः());
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opLenN(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    समुच्चयः_Quad(lhs.गन्तव्यम्_String().दीर्घता_2());
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opRepl(ध्रुव V3_सङ्ख्या& lhs,
                           ध्रुव V3_सङ्ख्या& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    प्रत्यागम opRepl(lhs, rhs.गन्तव्यम्_UInt());
}

V3_सङ्ख्या& V3_सङ्ख्या::opRepl(ध्रुव V3_सङ्ख्या& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    यदि (v3Global.opt.replication_सीमा()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_सीमा())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_सीमा() << " is suspect: " << rhsval);
    }
    समुच्चयः_Zero();
    प्रति (अचिह्नित times = 0; times < rhsval; ++times) {
        ध्रुव uint64_t destLsb = uint64_t{times} * lhs.विस्तारः();
        यदि (destLsb >= static_cast<uint32_t>(विस्तारः())) भङ्ग;
        copy_द्वयङ्काः(static_cast<पूर्णाङ्क>(destLsb), lhs, 0,
                 std::min(lhs.विस्तारः(), विस्तारः() - static_cast<पूर्णाङ्क>(destLsb)));
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::op_प्रवाहः_L(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    समुच्चयः_Zero();
    // See also error in V3Width
    यदि (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ध्रुव पूर्णाङ्क ssize = std::min(rhs.गन्तव्यम्_UInt(), static_cast<अचिह्नित>(lhs.विस्तारः()));
    प्रति (पूर्णाङ्क istart = 0; istart < lhs.विस्तारः(); istart += ssize) {
        ध्रुव पूर्णाङ्क ostart = std::max(0, lhs.विस्तारः() - ssize - istart);
        copy_द्वयङ्काः(ostart, lhs, istart, std::min(ssize, lhs.विस्तारः() - istart));
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogAnd(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण loutc = 0;
    वर्ण routc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            loutc = 1;
            भङ्ग;
        }
        यदि (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    प्रति (पूर्णाङ्क bit = 0; bit < rhs.विस्तारः(); ++bit) {
        यदि (rhs.bitIs1(bit)) {
            routc = 1;
            भङ्ग;
        }
        यदि (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    वर्ण outc = 'x';
    यदि (routc == 1 && loutc == 1) outc = 1;
    यदि (routc == 0 || loutc == 0) outc = 0;
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogOr(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < lhs.विस्तारः(); ++bit) {
        यदि (lhs.bitIs1(bit)) {
            outc = 1;
            गच्छ चरमः;
        }
        यदि (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    प्रति (पूर्णाङ्क bit = 0; bit < rhs.विस्तारः(); ++bit) {
        यदि (rhs.bitIs1(bit)) {
            outc = 1;
            गच्छ चरमः;
        }
        यदि (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogIf(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या lnot = lhs;
    lnot.opLogNot(lhs);
    प्रत्यागम opLogOr(lnot, rhs);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLogEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_सङ्ख्या ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    प्रत्यागम opLogAnd(ifa, ifb);
}

V3_सङ्ख्या& V3_सङ्ख्या::opAtoN(ध्रुव V3_सङ्ख्या& lhs, पूर्णाङ्क आधारः) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    UASSERT(आधारः == AstAtoN::ATOREAL || आधारः == 2 || आधारः == 8 || आधारः == 10 || आधारः == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.गन्तव्यम्_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.अन्तः_2(), '_'), str.अन्तः_2());

    यदि (आधारः == AstAtoN::ATOREAL) प्रत्यागम समुच्चयः_Double(std::atof(str.c_str()));

    errno = 0;
    स्वचालित v = std::strtol(str.c_str(), शून्यसूचकः, आधारः);
    यदि (errno != 0) v = 0;
    प्रत्यागम समुच्चयः_Long_S(static_cast<int32_t>(v));
}

V3_सङ्ख्या& V3_सङ्ख्या::opPutcN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, ध्रुव V3_सङ्ख्या& ths) {
    अङ्कः_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.गन्तव्यम्_String();
    ध्रुव int32_t i = rhs.गन्तव्यम्_SInt();
    ध्रुव int32_t c = ths.गन्तव्यम्_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    यदि (0 <= i && static_cast<uint32_t>(i) < lstring.दीर्घता_2() && c != 0) lstring[i] = c;
    प्रत्यागम समुच्चयः_String(lstring);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGetcN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    ध्रुव string lstring = lhs.गन्तव्यम्_String();
    ध्रुव int32_t i = rhs.गन्तव्यम्_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    यदि (0 <= i && static_cast<uint32_t>(i) < lstring.दीर्घता_2()) v = lstring[i];
    प्रत्यागम समुच्चयः_Long(v);
}

V3_सङ्ख्या& V3_सङ्ख्या::opSubstrN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, ध्रुव V3_सङ्ख्या& ths) {
    अङ्कः_ASSERT_OP_ARGS3(lhs, rhs, ths);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    ध्रुव string lstring = lhs.गन्तव्यम्_String();
    ध्रुव int32_t i = rhs.गन्तव्यम्_SInt();
    ध्रुव int32_t j = ths.गन्तव्यम्_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    यदि (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.दीर्घता_2()) प्रत्यागम समुच्चयः_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    प्रत्यागम समुच्चयः_String(lstring.substr(i, j - i + 1));
}

V3_सङ्ख्या& V3_सङ्ख्या::opCompareNN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, तार्किक ignoreCase) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    पूर्णाङ्क परिणाम;
    ध्रुव string lstring = lhs.गन्तव्यम्_String();
    ध्रुव string rstring = rhs.गन्तव्यम्_String();
    यदि (ignoreCase) {
        परिणाम = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } अन्यथा {
        परिणाम = std::वाक्यं_तुलय(lstring.c_str(), rstring.c_str());
    }
    प्रत्यागम समुच्चयः_Long_S(परिणाम);
}

V3_सङ्ख्या& V3_सङ्ख्या::opEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    यदि (lhs.isString()) प्रत्यागम opEqN(lhs, rhs);
    यदि (lhs.isDouble()) प्रत्यागम opEqD(lhs, rhs);
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            गच्छ चरमः;
        }
        यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            गच्छ चरमः;
        }
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opNeq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    यदि (lhs.isString()) प्रत्यागम opNeqN(lhs, rhs);
    यदि (lhs.isDouble()) प्रत्यागम opNeqD(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            गच्छ चरमः;
        }
        यदि (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            गच्छ चरमः;
        }
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3Hash V3_सङ्ख्या::गन्तव्यम्_Hash() ध्रुव {
    // Shouldmatch isCaseEq
    यदि (isString()) प्रत्यागम V3Hash{गन्तव्यम्_String()};
    यदि (isDouble()) प्रत्यागम V3Hash{गन्तव्यम्_Double()};
    V3Hash hash{विस्तारः()};
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) hash += m_दत्त.अङ्कः()[i].m_मानम्;
    प्रत्यागम hash;
}

तार्किक V3_सङ्ख्या::isCaseEq(ध्रुव V3_सङ्ख्या& rhs) ध्रुव {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    यदि (isString()) प्रत्यागम गन्तव्यम्_String() == rhs.गन्तव्यम्_String();
    यदि (isDouble()) प्रत्यागम गन्तव्यम्_Double() == rhs.गन्तव्यम्_Double();
    यदि (विस्तारः() != rhs.विस्तारः()) प्रत्यागम असत्य;
    प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
        यदि (!(m_दत्त.अङ्कः()[i] == rhs.m_दत्त.अङ्कः()[i])) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

V3_सङ्ख्या& V3_सङ्ख्या::opCaseEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_सङ्ख्या& V3_सङ्ख्या::opCaseNeq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    यदि (lhs.isString()) {
        प्रत्यागम opNeqN(lhs, rhs);
    } अन्यथा यदि (lhs.isDouble()) {
        प्रत्यागम opNeqD(lhs, rhs);
    }
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            गच्छ चरमः;
        }
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opWildEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 1;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (!rhs.bitIsXZ(bit)) {
            यदि (lhs.bitIsXZ(bit)) {
                outc = 'x';
                गच्छ चरमः;
            }
            यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                गच्छ चरमः;
            }
        }
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opWildNeq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (!rhs.bitIsXZ(bit)) {
            यदि (lhs.bitIsXZ(bit)) {
                outc = 'x';
                गच्छ चरमः;
            }
            यदि (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                गच्छ चरमः;
            }
        }
    }
चरमः:
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGt(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः(), rhs.विस्तारः()); ++bit) {
        यदि (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        यदि (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        यदि (lhs.bitIsXZ(bit)) outc = 'x';
        यदि (rhs.bitIsXZ(bit)) outc = 'x';
    }
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGtS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    वर्ण outc = 0;
    {
        ध्रुव पूर्णाङ्क mbit = std::max(lhs.विस्तारः() - 1, rhs.विस्तारः() - 1);
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
            प्रति (पूर्णाङ्क bit = 0; bit < std::max(lhs.विस्तारः() - 1, rhs.विस्तारः() - 1); ++bit) {
                यदि (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                यदि (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                यदि (lhs.bitIsXZ(bit)) outc = 'x';
                यदि (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(outc);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGte(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या& eq = opEq(lhs, rhs);
    यदि (eq.isNeqZero()) प्रत्यागम eq;  // Return true
    प्रत्यागम opGt(lhs, rhs);
}

V3_सङ्ख्या& V3_सङ्ख्या::opGteS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    V3_सङ्ख्या& eq = opEq(lhs, rhs);
    यदि (eq.isNeqZero()) प्रत्यागम eq;  // Return true
    प्रत्यागम opGtS(lhs, rhs);
}

V3_सङ्ख्या& V3_सङ्ख्या::opLt(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) { प्रत्यागम opGt(rhs, lhs); }
V3_सङ्ख्या& V3_सङ्ख्या::opLte(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) { प्रत्यागम opGte(rhs, lhs); }
V3_सङ्ख्या& V3_सङ्ख्या::opLtS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) { प्रत्यागम opGtS(rhs, lhs); }
V3_सङ्ख्या& V3_सङ्ख्या::opLteS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) { प्रत्यागम opGteS(rhs, lhs); }

V3_सङ्ख्या& V3_सङ्ख्या::op_सरणम्_R(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    समुच्चयः_Zero();
    यदि (rhs.विस्तारः() > 32 && !rhs.is_द्वयङ्काः_Zero(rhs.विस्तारः() - 1, 32)) प्रत्यागम *एतत्;
    ध्रुव uint32_t rhsval = rhs.गन्तव्यम्_UInt();
    यदि (rhsval < static_cast<uint32_t>(lhs.विस्तारः())) {
        copy_द्वयङ्काः(0, lhs, static_cast<पूर्णाङ्क>(rhsval),
                 std::min(विस्तारः(), lhs.विस्तारः() - static_cast<पूर्णाङ्क>(rhsval)));
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::op_सरणम्_RS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    समुच्चयः_Zero();
    ध्रुव तार्किक अतिप्रवाहः = rhs.विस्तारः() > 32 && !rhs.is_द्वयङ्काः_Zero(rhs.विस्तारः() - 1, 32);
    यदि (!अतिप्रवाहः) {
        ध्रुव uint32_t rhsval = rhs.गन्तव्यम्_UInt();
        यदि (rhsval < lbits) {
            ध्रुव पूर्णाङ्क copy_विस्तारः = std::min(विस्तारः(), static_cast<पूर्णाङ्क>(lbits - rhsval));
            copy_द्वयङ्काः(0, lhs, static_cast<पूर्णाङ्क>(rhsval), copy_विस्तारः);
            fill_द्वयङ्काः(copy_विस्तारः, विस्तारः() - copy_विस्तारः, lhs.bitIs(lbits - 1));
            प्रत्यागम *एतत्;
        }
    }
    fill_द्वयङ्काः(0, विस्तारः(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::op_सरणम्_L(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    समुच्चयः_Zero();
    यदि (rhs.विस्तारः() > 32 && !rhs.is_द्वयङ्काः_Zero(rhs.विस्तारः() - 1, 32)) प्रत्यागम *एतत्;
    ध्रुव uint32_t rhsval = rhs.गन्तव्यम्_UInt();
    यदि (rhsval < static_cast<uint32_t>(विस्तारः())) {
        copy_द्वयङ्काः(static_cast<पूर्णाङ्क>(rhsval), lhs, 0,
                 std::min(विस्तारः() - static_cast<पूर्णाङ्क>(rhsval), lhs.विस्तारः()));
    }
    प्रत्यागम *एतत्;
}

//======================================================================
// Ops - Arithmetic

V3_सङ्ख्या& V3_सङ्ख्या::opNegate(ध्रुव V3_सङ्ख्या& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    यदि (lhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    V3_सङ्ख्या notlhs(&lhs, विस्तारः());
    notlhs.opNot(lhs);
    ध्रुव V3_सङ्ख्या one(&lhs, विस्तारः(), 1);
    op_सङ्कलय(notlhs, one);
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_सङ्कलय(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    समुच्चयः_Zero();
    // Addem
    uint64_t carry = 0;
    प्रति (पूर्णाङ्क word = 0; word < शब्दाः(); ++word) {
        ध्रुव uint64_t lwordval = lhs.m_दत्त.अङ्कः()[word].m_मानम्;
        ध्रुव uint64_t rwordval = rhs.m_दत्त.अङ्कः()[word].m_मानम्;
        ध्रुव uint64_t योग_2 = lwordval + rwordval + carry;
        m_दत्त.अङ्कः()[word].m_मानम् = योग_2 & 0xffffffffULL;
        carry = योग_2 > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opSub(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    V3_सङ्ख्या negrhs(&rhs, विस्तारः());
    negrhs.opNegate(rhs);
    प्रत्यागम op_सङ्कलय(lhs, negrhs);
}
V3_सङ्ख्या& V3_सङ्ख्या::opMul(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    समुच्चयः_Zero();
    यदि (विस्तारः() <= 64) {
        समुच्चयः_Quad(lhs.गन्तव्यम्_UQuad() * rhs.गन्तव्यम्_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } अन्यथा {
        प्रति (पूर्णाङ्क lword = 0; lword < lhs.शब्दाः(); ++lword) {
            ध्रुव uint64_t lwordval = static_cast<uint64_t>(lhs.m_दत्त.अङ्कः()[lword].m_मानम्);
            यदि (lwordval == 0) अनुवर्त;
            प्रति (पूर्णाङ्क rword = 0; rword < rhs.शब्दाः(); ++rword) {
                ध्रुव uint64_t rwordval = static_cast<uint64_t>(rhs.m_दत्त.अङ्कः()[rword].m_मानम्);
                यदि (rwordval == 0) अनुवर्त;
                uint64_t mul = lwordval * rwordval;
                प्रति (पूर्णाङ्क qword = lword + rword; qword < शब्दाः(); ++qword) {
                    mul += static_cast<uint64_t>(m_दत्त.अङ्कः()[qword].m_मानम्);
                    m_दत्त.अङ्कः()[qword].m_मानम् = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    यदि (mul == 0) भङ्ग;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opMulS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    V3_सङ्ख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_सङ्ख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ध्रुव V3_सङ्ख्या qNoSign = opMul(lhsNoSign, rhsNoSign);
    यदि ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        op_नियोजय(qNoSign);
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opDiv(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    यदि (rhs.isEqZero()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_XRemoved();
    यदि (lhs.विस्तारः() <= 64) {
        समुच्चयः_Quad(lhs.गन्तव्यम्_UQuad() / rhs.गन्तव्यम्_UQuad());
        प्रत्यागम *एतत्;
    } अन्यथा {
        // Wide division
        प्रत्यागम opModDivGuts(lhs, rhs, असत्य);
    }
}
V3_सङ्ख्या& V3_सङ्ख्या::opDivS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    यदि (rhs.isEqZero()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_XRemoved();
    V3_सङ्ख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_सङ्ख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ध्रुव V3_सङ्ख्या qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    यदि ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } अन्यथा {
        op_नियोजय(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *एतत्);
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opModDiv(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    यदि (rhs.isEqZero()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_XRemoved();
    यदि (lhs.विस्तारः() <= 64) {
        समुच्चयः_Quad(lhs.गन्तव्यम्_UQuad() % rhs.गन्तव्यम्_UQuad());
        प्रत्यागम *एतत्;
    } अन्यथा {
        // Wide modulus
        प्रत्यागम opModDivGuts(lhs, rhs, सत्य);
    }
}
V3_सङ्ख्या& V3_सङ्ख्या::opModDivS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    यदि (rhs.isEqZero()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_XRemoved();
    V3_सङ्ख्या lhsNoSign = lhs;
    यदि (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_सङ्ख्या rhsNoSign = rhs;
    यदि (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ध्रुव V3_सङ्ख्या qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    यदि (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } अन्यथा {
        op_नियोजय(qNoSign);
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opModDivGuts(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, तार्किक is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    समुच्चयः_Zero();
    // Find MSB and check for zero.
    ध्रुव पूर्णाङ्क l_शब्दाः = lhs.शब्दाः();
    ध्रुव पूर्णाङ्क umsbp1 = lhs.most_समुच्चयः_Bit_P1();  // dividend
    ध्रुव पूर्णाङ्क vmsbp1 = rhs.most_समुच्चयः_Bit_P1();  //भाजकः
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
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_दत्त.अङ्कः()[j].m_मानम्));
            m_दत्त.अङ्कः()[j].m_मानम् = unw64 / static_cast<uint64_t>(rhs.m_दत्त.अङ्कः()[0].m_मानम्);
            k = unw64
                - (static_cast<uint64_t>(m_दत्त.अङ्कः()[j].m_मानम्)
                   * static_cast<uint64_t>(rhs.m_दत्त.अङ्कः()[0].m_मानम्));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *एतत् << " rem=0x" << std::hex
                                   << k << std::dec);
        यदि (is_modulus) {
            समुच्चयः_Zero();
            m_दत्त.अङ्कः()[0].m_मानम् = k;
        }
        opCleanThis();
        प्रत्यागम *एतत्;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_शब्दाः + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_शब्दाः + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    प्रति (पूर्णाङ्क i = 0; i < l_शब्दाः; ++i) m_दत्त.अङ्कः()[i].m_मानम् = 0;
    प्रति (पूर्णाङ्क i = 0; i < l_शब्दाः + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ध्रुव पूर्णाङ्क s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ध्रुव uint32_t सरणम्_आवरणम् = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    प्रति (पूर्णाङ्क i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_दत्त.अङ्कः()[i].m_मानम् << s)
                | (सरणम्_आवरणम् & (rhs.m_दत्त.अङ्कः()[i - 1].m_मानम् >> (32 - s)));
    }
    vn[0] = rhs.m_दत्त.अङ्कः()[0].m_मानम् << s;

    // Copy and shift dividend by same amount; may set new upper word
    यदि (s) {
        un[uw] = lhs.m_दत्त.अङ्कः()[uw - 1].m_मानम् >> (32 - s);
    } अन्यथा {
        un[uw] = 0;
    }
    प्रति (पूर्णाङ्क i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_दत्त.अङ्कः()[i].m_मानम् << s)
                | (सरणम्_आवरणम् & (lhs.m_दत्त.अङ्कः()[i - 1].m_मानम् >> (32 - s)));
    }
    un[0] = lhs.m_दत्त.अङ्कः()[0].m_मानम् << s;

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
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //व्यवकलय
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_दत्त.अङ्कः()[j].m_मानम् = qhat;  // Save quotient digit

        यदि (t < 0) {
            // Over subtracted; correct by adding back
            m_दत्त.अङ्कः()[j].m_मानम्--;
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
            m_दत्त.अङ्कः()[i].m_मानम् = (un[i] >> s) | (सरणम्_आवरणम् & (un[i + 1] << (32 - s)));
        }
        प्रति (पूर्णाङ्क i = vw; i < l_शब्दाः; ++i) m_दत्त.अङ्कः()[i].m_मानम् = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *एतत्);
        प्रत्यागम *एतत्;
    } अन्यथा {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *एतत्);
        प्रत्यागम *एतत्;
    }
}

V3_सङ्ख्या& V3_सङ्ख्या::opPow(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, तार्किक lsign, तार्किक rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(lhs, rhs);
    यदि (lhs.isFourState() || rhs.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    यदि (rhs.isEqZero()) प्रत्यागम समुच्चयः_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    यदि (rsign && rhs.isNegative()) {
        यदि (lhs.isEqZero()) {
            प्रत्यागम समुच्चयः_All_द्वयङ्काः_XRemoved();
        } अन्यथा यदि (lhs.isEqOne()) {
            प्रत्यागम समुच्चयः_Value1();
        } अन्यथा यदि (lsign && lhs.isEqAllOnes()) {
            यदि (rhs.bitIs1(0)) {
                प्रत्यागम समुच्चयः_All_Bits1();  // -1^odd=-1
            } अन्यथा {
                प्रत्यागम समुच्चयः_Value1();  // -1^even=1
            }
        }
        प्रत्यागम समुच्चयः_Zero();
    }
    यदि (lhs.isEqZero()) प्रत्यागम समुच्चयः_Zero();
    समुच्चयः_Zero();
    m_दत्त.अङ्कः()[0].m_मानम् = 1;
    V3_सङ्ख्या power(&lhs, विस्तारः());
    power.op_नियोजय(lhs);
    प्रति (पूर्णाङ्क bit = 0; bit < rhs.विस्तारः(); ++bit) {
        यदि (bit > 0) {  // power = power*power
            V3_सङ्ख्या चरमः_Power(&lhs, विस्तारः());
            चरमः_Power.op_नियोजय(power);
            power.opMul(चरमः_Power, चरमः_Power);
        }
        यदि (rhs.bitIs1(bit)) {  // out *= power
            V3_सङ्ख्या चरमः_बहिर्गमनम्(&lhs, विस्तारः());
            चरमः_बहिर्गमनम्.op_नियोजय(*एतत्);
            opMul(चरमः_बहिर्गमनम्, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowSU(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    प्रत्यागम opPow(lhs, rhs, सत्य, असत्य);
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowSS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    प्रत्यागम opPow(lhs, rhs, सत्य, सत्य);
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowUS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // Correct number of zero bits/width matters
    प्रत्यागम opPow(lhs, rhs, असत्य, सत्य);
}

V3_सङ्ख्या& V3_सङ्ख्या::opBufIf1(ध्रुव V3_सङ्ख्या& ens, ध्रुव V3_सङ्ख्या& if1s) {
    अङ्कः_ASSERT_OP_ARGS2(ens, if1s);
    अङ्कः_ASSERT_LOGIC_ARGS2(ens, if1s);
    समुच्चयः_Zero();
    प्रति (पूर्णाङ्क bit = 0; bit < विस्तारः(); ++bit) {
        यदि (ens.bitIs1(bit)) {
            समुच्चयः_Bit(bit, if1s.bitIs(bit));
        } अन्यथा {
            समुच्चयः_Bit(bit, 'z');
        }
    }
    प्रत्यागम *एतत्;
}

// Sets all bits in range to the given value
V3_सङ्ख्या& V3_सङ्ख्या::op_समुच्चयः_Range(uint32_t lsb, uint32_t विस्तारः, वर्ण bit_मानम्) {
    यदि (lsb < static_cast<uint32_t>(एतत्->विस्तारः())) {
        fill_द्वयङ्काः(static_cast<पूर्णाङ्क>(lsb),
                 static_cast<पूर्णाङ्क>(std::min(विस्तारः, static_cast<uint32_t>(एतत्->विस्तारः()) - lsb)),
                 bit_मानम्);
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::op_नियोजय(ध्रुव V3_सङ्ख्या& lhs) { प्रत्यागम op_नियोजय_Non_XZ(lhs, असत्य); }
V3_सङ्ख्या& V3_सङ्ख्या::op_नियोजय_Non_XZ(ध्रुव V3_सङ्ख्या& lhs, तार्किक ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    यदि (एतत् != &lhs) {
        यदि (VL_UNLIKELY(lhs.isNull())) {
            m_दत्त.m_isNull = सत्य;
        } अन्यथा यदि (isString()) {
            यदि (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_दत्त.str() = lhs.गन्तव्यम्_String();
            } अन्यथा {
                m_दत्त.str() = lhs.m_दत्त.str();
            }
        } अन्यथा यदि (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            समुच्चयः_Zero();
        } अन्यथा यदि (lhs.isDouble()) {
            समुच्चयः_Double(lhs.गन्तव्यम्_Double());
        } अन्यथा यदि (!ignoreXZ) {
            समुच्चयः_Zero();
            copy_द्वयङ्काः(0, lhs, 0, std::min(विस्तारः(), lhs.विस्तारः()));
        } अन्यथा {
            प्रति (पूर्णाङ्क word = 0; word < शब्दाः(); ++word) {
                ध्रुव uint32_t मानम् = word < lhs.शब्दाः() ? lhs.m_दत्त.अङ्कः()[word].m_मानम्
                                                                & ~lhs.m_दत्त.अङ्कः()[word].m_मानम्_X
                                                          : 0;
                m_दत्त.अङ्कः()[word] = {मानम्, 0};
            }
            opCleanThis();
        }
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opNToI(ध्रुव V3_सङ्ख्या& lhs) {
    // String to packed number
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    समुच्चयः_Zero();
    ध्रुव string& str = lhs.गन्तव्यम्_String();
    प्रति (आकार_t n = 0; n < str.दीर्घता_2(); ++n) {
        ध्रुव वर्ण c = str[str.दीर्घता_2() - 1 - n];
        प्रति (आकार_t cbit = 0; cbit < 8; ++cbit) {
            ध्रुव std::bitset<8> sbit{static_cast<अचिह्नित दीर्घ दीर्घ>(c)};
            समुच्चयः_Bit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opExtendS(ध्रुव V3_सङ्ख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    ध्रुव पूर्णाङ्क copy_विस्तारः = std::min(विस्तारः(), static_cast<पूर्णाङ्क>(lbits));
    copy_द्वयङ्काः(0, lhs, 0, copy_विस्तारः);
    fill_द्वयङ्काः(copy_विस्तारः, विस्तारः() - copy_विस्तारः, lhs.bitIs(lbits - 1));
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opExtendXZ(ध्रुव V3_सङ्ख्या& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    ध्रुव पूर्णाङ्क copy_विस्तारः = std::min(विस्तारः(), static_cast<पूर्णाङ्क>(lbits));
    copy_द्वयङ्काः(0, lhs, 0, copy_विस्तारः);
    fill_द्वयङ्काः(copy_विस्तारः, विस्तारः() - copy_विस्तारः, lhs.bitIs(lbits - 1));
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opClean(ध्रुव V3_सङ्ख्या& lhs, uint32_t द्वयङ्काः) { प्रत्यागम opSel(lhs, द्वयङ्काः - 1, 0); }

शून्य V3_सङ्ख्या::opCleanThis(तार्किक warnOnTruncation) {
    // Clean MSB of number
    अङ्कः_ASSERT_LOGIC_ARGS1(*एतत्);
    ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[शब्दाः() - 1];
    ध्रुव uint32_t new_मानम्_Msb = v.m_मानम् & hi_Word_आवरणम्();
    ध्रुव uint32_t new_मानम्_XMsb = v.m_मानम्_X & hi_Word_आवरणम्();
    यदि (warnOnTruncation && (new_मानम्_Msb != v.m_मानम् || new_मानम्_XMsb != v.m_मानम्_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << विस्तारः() << " bit number: " << displayed("%d"));
    }
    m_दत्त.अङ्कः()[शब्दाः() - 1] = {new_मानम्_Msb, new_मानम्_XMsb};
}

V3_सङ्ख्या& V3_सङ्ख्या::opSel(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& msb, ध्रुव V3_सङ्ख्या& lsb) {
    अङ्कः_ASSERT_OP_ARGS3(lhs, msb, lsb);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS2(msb, lsb);
    यदि (lsb.isFourState() || msb.isFourState()) प्रत्यागम समुच्चयः_All_द्वयङ्काः_X();
    प्रत्यागम opSel(lhs, msb.गन्तव्यम्_UInt(), lsb.गन्तव्यम्_UInt());
}

V3_सङ्ख्या& V3_सङ्ख्या::opSel(ध्रुव V3_सङ्ख्या& lhs, uint32_t msbval, uint32_t lsbval) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    समुच्चयः_Zero();
    पूर्णाङ्क copy_विस्तारः = 0;
    यदि (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.विस्तारः())) {
        ध्रुव uint64_t selected_विस्तारः = uint64_t{msbval} - lsbval + 1;
        copy_विस्तारः = static_cast<पूर्णाङ्क>(
            std::min({static_cast<uint64_t>(विस्तारः()), static_cast<uint64_t>(lhs.विस्तारः()) - lsbval,
                      selected_विस्तारः}));
    }
    copy_द्वयङ्काः(0, lhs, copy_विस्तारः ? static_cast<पूर्णाङ्क>(lsbval) : 0, copy_विस्तारः);
    fill_द्वयङ्काः(copy_विस्तारः, विस्तारः() - copy_विस्तारः, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    प्रत्यागम *एतत्;
}

V3_सङ्ख्या& V3_सङ्ख्या::opSelInto(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& lsb, पूर्णाङ्क विस्तारः) {
    प्रत्यागम opSelInto(lhs, lsb.गन्तव्यम्_SInt(), विस्तारः);
}

V3_सङ्ख्या& V3_सङ्ख्या::opSelInto(ध्रुव V3_सङ्ख्या& lhs, पूर्णाङ्क lsbval, पूर्णाङ्क विस्तारः) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ध्रुव पूर्णाङ्क copy_विस्तारः = std::max(0, std::min({विस्तारः, lhs.विस्तारः(), एतत्->विस्तारः() - lsbval}));
    copy_द्वयङ्काः(lsbval, lhs, 0, copy_विस्तारः);
    ध्रुव पूर्णाङ्क fill_विस्तारः
        = std::max(0, std::min(विस्तारः - copy_विस्तारः, एतत्->विस्तारः() - lsbval - copy_विस्तारः));
    fill_द्वयङ्काः(lsbval + copy_विस्तारः, fill_विस्तारः, v3Global.constRemoveXs() ? 0 : 'x');
    प्रत्यागम *एतत्;
}

//======================================================================
// Ops - Floating point

V3_सङ्ख्या& V3_सङ्ख्या::opIToRD(ध्रुव V3_सङ्ख्या& lhs, तार्किक isSigned) {
    // Correct number of zero bits/width matters
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_सङ्ख्या noxz(lhs);
    noxz.op_नियोजय_Non_XZ(lhs);
    द्विगुण d = 0;
    ध्रुव तार्किक negate = isSigned && noxz.isNegative();
    यदि (negate) {
        ध्रुव V3_सङ्ख्या noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    प्रति (पूर्णाङ्क bit = noxz.विस्तारः() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        यदि (noxz.bitIs1(bit)) d += exp2(bit);
    }
    यदि (negate) d = -d;
    प्रत्यागम समुच्चयः_Double(d);
}
V3_सङ्ख्या& V3_सङ्ख्या::opRToIS(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_DOUBLE_ARGS1(lhs);
    ध्रुव द्विगुण v = VL_TRUNC(lhs.गन्तव्यम्_Double());
    ध्रुव int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    प्रत्यागम समुच्चयः_Long_S(i);
}
V3_सङ्ख्या& V3_सङ्ख्या::opRToIRoundS(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_DOUBLE_ARGS1(lhs);
    ध्रुव द्विगुण v = VL_आवर्तनम्(lhs.गन्तव्यम्_Double());
    समुच्चयः_Zero();
    सङ्घ {
        द्विगुण d;
        uint64_t q;
    } u;
    u.d = v;
    (शून्य)u.d;

    ध्रुव पूर्णाङ्क घातांकः = static_cast<पूर्णाङ्क>((u.q >> 52ULL) & VL_आवरणम्_Q(11)) - 1023;
    ध्रुव पूर्णाङ्क lsb = घातांकः - 52;
    ध्रुव uint64_t mantissa = (u.q & VL_आवरणम्_Q(52)) | (1ULL << 52);
    यदि (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        प्रति (पूर्णाङ्क bit = 0; bit <= 52; ++bit) {
            यदि (mantissa & (1ULL << bit)) {
                ध्रुव पूर्णाङ्क outbit = bit + lsb;
                यदि (outbit >= 0) समुच्चयः_Bit(outbit, 1);
            }
        }
        यदि (v < 0) {
            ध्रुव V3_सङ्ख्या noSign = *एतत्;
            opNegate(noSign);
        }
    }
    प्रत्यागम *एतत्;
}
V3_सङ्ख्या& V3_सङ्ख्या::op_Real_गन्तव्यम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.विस्तारः() == 64 && विस्तारः() == 64, "Real operation on wrong sized number");
    सङ्घ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.गन्तव्यम्_Double();
    प्रत्यागम समुच्चयः_Quad(u.m_v);
}
V3_सङ्ख्या& V3_सङ्ख्या::op_द्वयङ्काः_गन्तव्यम्_Real_D(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.विस्तारः() == 64 && विस्तारः() == 64, "Real operation on wrong sized number");
    सङ्घ {
        द्विगुण m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.गन्तव्यम्_UQuad();
    प्रत्यागम समुच्चयः_Double(u.m_d);
}
V3_सङ्ख्या& V3_सङ्ख्या::opNegateD(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_DOUBLE_ARGS1(lhs);
    प्रत्यागम समुच्चयः_Double(-lhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::op_सङ्कलय_D(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Double(lhs.गन्तव्यम्_Double() + rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opSubD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Double(lhs.गन्तव्यम्_Double() - rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opMulD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Double(lhs.गन्तव्यम्_Double() * rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opDivD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Double(lhs.गन्तव्यम्_Double() / rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opPowD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Double(घातः(lhs.गन्तव्यम्_Double(), rhs.गन्तव्यम्_Double()));
}
V3_सङ्ख्या& V3_सङ्ख्या::opEqD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_Double() == rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opNeqD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_Double() != rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGtD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_Double() > rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGteD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_Double() >= rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLtD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_Double() < rhs.गन्तव्यम्_Double());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLteD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_Double() <= rhs.गन्तव्यम्_Double());
}

//======================================================================
// Ops - String

V3_सङ्ख्या& V3_सङ्ख्या::opConcatN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_String(lhs.गन्तव्यम्_String() + rhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::opReplN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    अङ्कः_ASSERT_LOGIC_ARGS1(rhs);
    प्रत्यागम opReplN(lhs, rhs.गन्तव्यम्_UInt());
}
V3_सङ्ख्या& V3_सङ्ख्या::opReplN(ध्रुव V3_सङ्ख्या& lhs, uint32_t rhsval) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    string बहिर्गमनम्;
    बहिर्गमनम्.reserve(lhs.गन्तव्यम्_String().दीर्घता_2() * rhsval);
    प्रति (अचिह्नित times = 0; times < rhsval; ++times) बहिर्गमनम् += lhs.गन्तव्यम्_String();
    प्रत्यागम समुच्चयः_String(बहिर्गमनम्);
}
V3_सङ्ख्या& V3_सङ्ख्या::opCvtPackString(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    प्रत्यागम समुच्चयः_String(lhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::op_गन्तव्यम्_अधस्तनम्_N(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    ध्रुव std::string बहिर्गमनम् = VString::downcase(lhs.गन्तव्यम्_String());
    प्रत्यागम समुच्चयः_String(बहिर्गमनम्);
}
V3_सङ्ख्या& V3_सङ्ख्या::op_गन्तव्यम्_ऊर्ध्वस्थम्_N(ध्रुव V3_सङ्ख्या& lhs) {
    अङ्कः_ASSERT_OP_ARGS1(lhs);
    अङ्कः_ASSERT_STRING_ARGS1(lhs);
    ध्रुव std::string बहिर्गमनम् = VString::upcase(lhs.गन्तव्यम्_String());
    प्रत्यागम समुच्चयः_String(बहिर्गमनम्);
}

V3_सङ्ख्या& V3_सङ्ख्या::opEqN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_String() == rhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::opNeqN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_String() != rhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGtN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_String() > rhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::opGteN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_String() >= rhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLtN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_String() < rhs.गन्तव्यम्_String());
}
V3_सङ्ख्या& V3_सङ्ख्या::opLteN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs) {
    अङ्कः_ASSERT_OP_ARGS2(lhs, rhs);
    अङ्कः_ASSERT_STRING_ARGS2(lhs, rhs);
    प्रत्यागम समुच्चयः_Single_द्वयङ्काः(lhs.गन्तव्यम्_String() <= rhs.गन्तव्यम्_String());
}

//======================================================================

शून्य V3_सङ्ख्या::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_पङ्क्तिः* ध्रुव fl = नूतन File_पङ्क्तिः{File_पङ्क्तिः::built_अन्तर्_Filename()};
    V3_सङ्ख्या अङ्कः{fl, 32, 0};
    अङ्कः.selfTestThis();
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
