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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Number.h"

#ଅନ୍ତର୍ଭାଵ "V3File.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cerrno>
#ଅନ୍ତର୍ଭାଵ <cmath>
#ଅନ୍ତର୍ଭାଵ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define ଅଙ୍କ_ASSERT_OP_ARGS1(arg1) \
    UASSERT((ଏହି != &(arg1)), "Number operation called with same source and dest")
#define ଅଙ୍କ_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((ଏହି != &(arg1) && ଏହି != &(arg2)), \
            "Number operation called with same source and dest")
#define ଅଙ୍କ_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((ଏହି != &(arg1) && ଏହି != &(arg2) && ଏହି != &(arg3)), \
            "Number operation called with same source and dest")
#define ଅଙ୍କ_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((ଏହି != &(arg1) && ଏହି != &(arg2) && ଏହି != &(arg3) && ଏହି != &(arg4)), \
            "Number operation called with same source and dest")

#define ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_ସଂଖ୍ୟା_Data::V3_ସଂଖ୍ୟା_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define ଅଙ୍କ_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg1); \
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg2)

#define ଅଙ୍କ_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg1); \
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg2); \
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg3); \
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(arg4)

#define ଅଙ୍କ_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define ଅଙ୍କ_ASSERT_STRING_ARGS2(arg1, arg2) \
    ଅଙ୍କ_ASSERT_STRING_ARGS1(arg1); \
    ଅଙ୍କ_ASSERT_STRING_ARGS1(arg2)

#define ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(arg1); \
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::v3errorEnd(ଅଚଳ std::ostringstream& str) ଅଚଳ VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ଯଦି (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } ଅନ୍ୟଥା ଯଦି (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } ଅନ୍ୟଥା {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::v3errorEndFatal(ଅଚଳ std::ostringstream& str) ଅଚଳ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ପ୍ରତିଜ୍ଞା(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_ସଂଖ୍ୟା::V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, VerilogStringLiteral, ଅଚଳ string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    ଯଦି (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } ଅନ୍ୟଥା {
        init(nodep, std::max<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(str.length() * 8, 1));
        ପାଇଁ (unsigned pos = 0; pos < str.length(); ++pos) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା topos = str.length() - 1 - pos;
            ମୂଲ୍ୟ_And_X& v = m_data.ଅଙ୍କ()[topos / 4];
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < 8; ++bit) {
                ଯଦି (str[pos] & (1UL << bit)) v.m_ମୂଲ୍ୟ |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = ସତ୍ୟ;
    opCleanThis(ସତ୍ୟ);
}

V3_ସଂଖ୍ୟା::V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* nodedtypep) {
    ଯଦି (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } ଅନ୍ୟଥା ଯଦି (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } ଅନ୍ୟଥା ଯଦି (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } ଅନ୍ୟଥା {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::create(ଅଚଳ ଅକ୍ଷର* sourcep) {
    m_data.setLogic();
    ଅଚଳ ଅକ୍ଷର* ମୂଲ୍ୟ_startp = sourcep;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = sourcep; *cp; ++cp) {
        ଯଦି (*cp == '\'') {
            ମୂଲ୍ୟ_startp = cp + 1;
            ଭାଙ୍ଗ;
        }
    }

    ତାର୍କିକ userSized = ମିଥ୍ୟା;
    ତାର୍କିକ unbased = ମିଥ୍ୟା;
    ଅକ୍ଷର base = '\0';
    ଯଦି (ମୂଲ୍ୟ_startp != sourcep) {  // Has a '
        string widthn;
        ଅଚଳ ଅକ୍ଷର* cp = sourcep;
        ପାଇଁ (; *cp; ++cp) {
            ଯଦି (*cp == '\'') {
                ++cp;
                ଭାଙ୍ଗ;
            }
            ଯଦି (*cp != '_') widthn += *cp;
        }
        ଯେତେବେଳେ (*cp == '_') cp++;
        ଯଦି (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(ସତ୍ୟ);
        }
        ଯଦି (*cp) {
            base = *cp;
            ++cp;
        }
        ମୂଲ୍ୟ_startp = cp;

        userSized = widthn.length() != 0;
        ଯଦି (std::atoi(widthn.c_str())) {
            ଯଦି (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_ଅଙ୍କ_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_ଅଙ୍କ_Width(), ସତ୍ୟ);
            } ଅନ୍ୟଥା {
                width(std::atoi(widthn.c_str()), ସତ୍ୟ);
            }
        }
    } ଅନ୍ୟଥା {
        unbased = ସତ୍ୟ;
        base = 'd';
    }

    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    ଯଦି (base == '0') {
        width(1, ମିଥ୍ୟା);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (base == '1') {
        width(1, ମିଥ୍ୟା);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (std::tolower(base) == 'z') {
        width(1, ମିଥ୍ୟା);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (std::tolower(base) == 'x') {
        width(1, ମିଥ୍ୟା);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = ସତ୍ୟ;
    }
    // Otherwise...
    ଅନ୍ୟଥା ଯଦି (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthBound = std::max<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(32, std::strlen(ମୂଲ୍ୟ_startp) * 4);
        width(widthBound, ମିଥ୍ୟା);  // Will change width below
        ଯଦି (unbased) isSigned(ସତ୍ୟ);  // Also says the spec.
    }

    // Ignore leading blanks
    ଯେତେବେଳେ (*ମୂଲ୍ୟ_startp == '_' || std::isspace(*ମୂଲ୍ୟ_startp)) ++ମୂଲ୍ୟ_startp;
    ଯଦି (!*ମୂଲ୍ୟ_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    ଯଦି (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା obit = 0;  // Start at LSB
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା base_align = 1;
    ଯଦି (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        ଯେତେବେଳେ (*ମୂଲ୍ୟ_startp == '_' || *ମୂଲ୍ୟ_startp == '0') ++ମୂଲ୍ୟ_startp;
        // Convert decimal number to hex
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା olen = 0;
        uint32_t val = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା got_x = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା got_z = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା got_01 = 0;
        ତାର୍କିକ warnedTooMany = ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = ମୂଲ୍ୟ_startp; *cp; ++cp) {
            ବାଛ (std::tolower(*cp)) {
            ସ୍ଥିତି '0':  // FALLTHRU
            ସ୍ଥିତି '1':  // FALLTHRU
            ସ୍ଥିତି '2':  // FALLTHRU
            ସ୍ଥିତି '3':  // FALLTHRU
            ସ୍ଥିତି '4':  // FALLTHRU
            ସ୍ଥିତି '5':  // FALLTHRU
            ସ୍ଥିତି '6':  // FALLTHRU
            ସ୍ଥିତି '7':  // FALLTHRU
            ସ୍ଥିତି '8':  // FALLTHRU
            ସ୍ଥିତି '9': {
                ଯଦି (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = val;
                } ଅନ୍ୟଥା {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_ସଂଖ୍ୟା product(ଏହି, width() + 4);  // +4 for overflow detection
                    ଅଚଳ V3_ସଂଖ୍ୟା ten(ଏହି, width() + 4, 10);
                    ଅଚଳ V3_ସଂଖ୍ୟା addend(ଏହି, width(), (*cp - '0'));
                    product.opMul(*ଏହି, ten);
                    opAdd(product, addend);
                    ଯଦି (!warnedTooMany && product.bits_ମୂଲ୍ୟ(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = ସତ୍ୟ;
                    }
                }
                ++olen;
                got_01 = 1;
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି 'z':
            ସ୍ଥିତି '?': {
                got_z = 1;
                ଯଦି (!userSized) width(32, ମିଥ୍ୟା);
                setAllBitsZ();
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି 'x': {
                got_x = 1;
                ଯଦି (!userSized) width(32, ମିଥ୍ୟା);
                setAllBitsX();
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି '_': ଭାଙ୍ଗ;
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                ଭାଙ୍ଗ;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        ଯଦି ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << ମୂଲ୍ୟ_startp);
        }
    } ଅନ୍ୟଥା {
        // Convert bin/octal number to hex
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = ମୂଲ୍ୟ_startp + std::strlen(ମୂଲ୍ୟ_startp) - 1; cp >= ମୂଲ୍ୟ_startp;
             cp--) {
            ଯଦି (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                ଭାଙ୍ଗ;
            }
            ବାଛ (std::tolower(base)) {
            ସ୍ଥିତି 'b': {
                base_align = 1;
                ବାଛ (std::tolower(*cp)) {
                ସ୍ଥିତି '0': setBit(obit++, 0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '1': setBit(obit++, 1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'z':
                ସ୍ଥିତି '?': setBit(obit++, 'z'); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'x': setBit(obit++, 'x'); ଭାଙ୍ଗ;
                ସ୍ଥିତି '_': ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3error("Illegal character in binary constant: " << *cp);
                }
                ଭାଙ୍ଗ;
            }

            ସ୍ଥିତି 'o':
            ସ୍ଥିତି 'c': {
                base_align = 3;
                ବାଛ (std::tolower(*cp)) {  // clang-format off
                ସ୍ଥିତି '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  ଭାଙ୍ଗ;
                ସ୍ଥିତି '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  ଭାଙ୍ଗ;
                ସ୍ଥିତି 'z':
                ସ୍ଥିତି '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); ଭାଙ୍ଗ;
                ସ୍ଥିତି '_': ଭାଙ୍ଗ;
                // clang-format on
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3error("Illegal character in octal constant");
                }
                ଭାଙ୍ଗ;
            }

            ସ୍ଥିତି 'h':  // FALLTHRU
            ସ୍ଥିତି 'x': {
                base_align = 4;
                ବାଛ (std::tolower(*cp)) {  // clang-format off
                ସ୍ଥିତି '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); ଭାଙ୍ଗ;
                ସ୍ଥିତି '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'z':
                ସ୍ଥିତି '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); ଭାଙ୍ଗ;
                ସ୍ଥିତି 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); ଭାଙ୍ଗ;
                    // clang-format on
                ସ୍ଥିତି '_': ଭାଙ୍ଗ;
                ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    ଭାଙ୍ଗ;
                }  // LCOV_EXCL_STOP
                ଭାଙ୍ଗ;
            }
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                ଭାଙ୍ଗ;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    ଯଦି (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), ମିଥ୍ୟା);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    ଯେତେବେଳେ (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(ସତ୍ୟ);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::warnTooMany(ଅଚଳ string& ମୂଲ୍ୟ) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << ମୂଲ୍ୟ << "'");
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::nodep(Ast_ଗଣ୍ଠି* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    ଯଦି (!nodep) ଫେରନ୍ତୁ;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

ତାର୍କିକ V3_ସଂଖ୍ୟା::epsilonEqual(ଦ୍ୱିଗୁଣ a, ଦ୍ୱିଗୁଣ b) {
    ଫେରନ୍ତୁ std::fabs(a - b)
           <= (std::numeric_limits<ଦ୍ୱିଗୁଣ>::epsilon() * std::max(1.0, std::max(a, b)));
}

ତାର୍କିକ V3_ସଂଖ୍ୟା::epsilonIntegral(ଦ୍ୱିଗୁଣ a) {
    ଅଚଳ ଦ୍ୱିଗୁଣ dint = static_cast<ଦ୍ୱିଗୁଣ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    ଫେରନ୍ତୁ epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3_ସଂଖ୍ୟା::log2b(uint32_t ଅଙ୍କ) {
    // See also opCLog2
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 31; bit > 0; bit--) {
        ଯଦି (ଅଙ୍କ & (1ULL << bit)) ଫେରନ୍ତୁ bit;
    }
    ଫେରନ୍ତୁ 0;
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3_ସଂଖ୍ୟା::log2bQuad(uint64_t ଅଙ୍କ) {
    ଯଦି (ଅଙ୍କ >> 32ULL) ଫେରନ୍ତୁ 32 + log2b(ଅଙ୍କ >> 32ULL);
    ଫେରନ୍ତୁ log2b(ଅଙ୍କ);
}

//======================================================================
// Setters

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setZero() {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, 0};
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setQuad(uint64_t ମୂଲ୍ୟ) {
    setZero();
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = ମୂଲ୍ୟ & 0xffffffffULL;
    ଯଦି (width() > 32) m_data.ଅଙ୍କ()[1].m_ମୂଲ୍ୟ = (ମୂଲ୍ୟ >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setLong(uint32_t ମୂଲ୍ୟ) {
    setZero();
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = ମୂଲ୍ୟ;
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setLongS(int32_t ମୂଲ୍ୟ) {
    setZero();
    ସଂଘ {
        uint32_t u;
        int32_t s;
    } u;
    u.s = ମୂଲ୍ୟ;
    (ଶୂନ୍ୟ)u.s;
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = u.u;
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setDouble(ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    ସଂଘ {
        ଦ୍ୱିଗୁଣ d;
        uint32_t u[2];
    } u;
    u.d = ମୂଲ୍ୟ;
    (ଶୂନ୍ୟ)u.d;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 2; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, 0};
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = u.u[0];
    m_data.ଅଙ୍କ()[1].m_ମୂଲ୍ୟ = u.u[1];
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setSingleBits(ଅକ୍ଷର ମୂଲ୍ୟ) {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1 /*upper*/; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, 0};
    m_data.ଅଙ୍କ()[0] = {(ମୂଲ୍ୟ == '1' || ମୂଲ୍ୟ == 'x' || ମୂଲ୍ୟ == 1 || ମୂଲ୍ୟ == 3),
                       (ମୂଲ୍ୟ == 'z' || ମୂଲ୍ୟ == 'x' || ମୂଲ୍ୟ == 2 || ମୂଲ୍ୟ == 3)};
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setAllBits0() {
    setZero();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setAllBits1() {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {~0U, 0};
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {~0U, ~0U};
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setAllBitsZ() {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, ~0U};
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setAllBitsXRemoved() {
    ଯଦି (!v3Global.constRemoveXs()) {
        ଫେରନ୍ତୁ setAllBitsX();
    } ଅନ୍ୟଥା {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        ଯଦି (v3Global.opt.xAssign() == "1") {
            ଫେରନ୍ତୁ setAllBits1();
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ setAllBits0();
        }
    }
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setValue1() {
    m_data.ଅଙ୍କ()[0] = {1, 0};
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, 0};
    ଫେରନ୍ତୁ *ଏହି;
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::setBitX0(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::setMask(ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    ଫେରନ୍ତୁ *ଏହି;
}

//======================================================================
// ACCESSORS - as strings

string V3_ସଂଖ୍ୟା::ascii(ତାର୍କିକ prefixed, ତାର୍କିକ cleanVerilog) ଅଚଳ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ଯଦି (is1Step()) ଫେରନ୍ତୁ "1step";
    ଯଦି (isNull()) ଫେରନ୍ତୁ "null";
    std::ostringstream out;
    ଯଦି (isDouble()) {
        out.precision(17);
        ଯଦି (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } ଅନ୍ୟଥା {
            out << toDouble();
            ଯଦି (toDouble() == floor(toDouble())) out << ".0";
        }
        ଫେରନ୍ତୁ out.str();
    } ଅନ୍ୟଥା ଯଦି (isString()) {
        ଫେରନ୍ତୁ '"' + toString() + '"';
    } ଅନ୍ୟଥା {
        ଯଦି (VL_UNCOVERABLE((m_data.ଅଙ୍କ()[ଶବ୍ଦଗୁଡ଼ିକ() - 1].m_ମୂଲ୍ୟ | m_data.ଅଙ୍କ()[ଶବ୍ଦଗୁଡ଼ିକ() - 1].m_ମୂଲ୍ୟ_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    ଯଦି (prefixed) {
        ଯଦି (sized()) {
            out << width() << "'";
        } ଅନ୍ୟଥା ଯଦି (autoExtend() && width() == 1) {
            out << "'";
            ଯଦି (bitIs0(0)) {
                out << '0';
            } ଅନ୍ୟଥା ଯଦି (bitIs1(0)) {
                out << '1';
            } ଅନ୍ୟଥା ଯଦି (bitIsZ(0)) {
                out << 'z';
            } ଅନ୍ୟଥା {
                out << 'x';
            }
            ଫେରନ୍ତୁ out.str();
        } ଅନ୍ୟଥା {
            ଯଦି (cleanVerilog) {
                out << "'";
            } ଅନ୍ୟଥା {
                out << "?" << width() << "?";
            }
        }
        ଯଦି (isSigned()) out << "s";
    }

    ଅଚଳ ତାର୍କିକ ଦ୍ୱିମିକ = (isFourState()
#ifdef V3_ସଂଖ୍ୟା_ASCII_ଦ୍ୱିମିକ
                         // cppcheck-suppress konwnConditionTrueFalse
                         || ସତ୍ୟ
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    ଯଦି (ଦ୍ୱିମିକ) {
        out << "b";
        out << displayed("%0b");
    } ଅନ୍ୟଥା {
        ଯଦି (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    ଯଦି (isNull()) {
        ଯଦି (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } ଅନ୍ୟଥା {
            out << " [null]";
        }
    }
    ଫେରନ୍ତୁ out.str();
}

ତାର୍କିକ V3_ସଂଖ୍ୟା::displayedFmtHasArg(ଅକ୍ଷର format, ତାର୍କିକ isScan) {
    // Is this a format letter that takes an argument in runtime?
    (ଶୂନ୍ୟ)isScan;
    ବାଛ (std::tolower(format)) {
    ସ୍ଥିତି 'b': ଫେରନ୍ତୁ ସତ୍ୟ;  //ଦ୍ୱିମିକ
    ସ୍ଥିତି 'c': ଫେରନ୍ତୁ ସତ୍ୟ;  // Character
    ସ୍ଥିତି 'd': ଫେରନ୍ତୁ ସତ୍ୟ;  // Decimal
    ସ୍ଥିତି 'e': ଫେରନ୍ତୁ ସତ୍ୟ;  // Floating
    ସ୍ଥିତି 'f': ଫେରନ୍ତୁ ସତ୍ୟ;  // Floating
    ସ୍ଥିତି 'g': ଫେରନ୍ତୁ ସତ୍ୟ;  // Floating
    ସ୍ଥିତି 'h': ଫେରନ୍ତୁ ସତ୍ୟ;  // Hex
    ସ୍ଥିତି 'o': ଫେରନ୍ତୁ ସତ୍ୟ;  // Octal
    ସ୍ଥିତି 'p': ଫେରନ୍ତୁ ସତ୍ୟ;  // Pattern
    ସ୍ଥିତି 's': ଫେରନ୍ତୁ ସତ୍ୟ;  // String
    ସ୍ଥିତି 't': ଫେରନ୍ତୁ ସତ୍ୟ;  // Time
    ସ୍ଥିତି 'u': ଫେରନ୍ତୁ ସତ୍ୟ;  // Packed 2-state
    ସ୍ଥିତି 'v': ଫେରନ୍ତୁ ସତ୍ୟ;  // Strength
    ସ୍ଥିତି 'x': ଫେରନ୍ତୁ ସତ୍ୟ;  // Hex
    ସ୍ଥିତି 'z': ଫେରନ୍ତୁ ସତ୍ୟ;  // Packed 4-state
    ସ୍ଥିତି '?': ଫେରନ୍ତୁ ସତ୍ୟ;  // Internal: Compute format in V3Width based on data type
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_ସଂଖ୍ୟା::displayPad(size_t fmtsize, ଅକ୍ଷର pad, ତାର୍କିକ ବାମ, ଅଚଳ string& in) VL_PURE {
    string padding;
    ଯଦି (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    ଫେରନ୍ତୁ ବାମ ? (in + padding) : (padding + in);
}

string V3_ସଂଖ୍ୟା::displayedEnumName(ଅଚଳ AstEnumItem* itemp) {
    ଅଚଳ string name = VIdProtect::protect(itemp->prettyName());
    ଫେରନ୍ତୁ VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned ଅକ୍ଷର>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_ସଂଖ୍ୟା::displayedEnumName(ଅଚଳ AstEnumDType* dtypep) ଅଚଳ VL_MT_STABLE {
    V3_ସଂଖ୍ୟା ମୂଲ୍ୟ{ଏହି, dtypep->width()};
    ମୂଲ୍ୟ.opAssign(*ଏହି);
    ପାଇଁ (ଅଚଳ AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_AS(itemp->valuep(), Const);
        ଯଦି (!constp->ଅଙ୍କ().isAnyXZ() && ମୂଲ୍ୟ.isCaseEq(constp->ଅଙ୍କ()))
            ଫେରନ୍ତୁ displayedEnumName(itemp);
    }
    ଫେରନ୍ତୁ "";
}

string V3_ସଂଖ୍ୟା::displayedEnum(ଅଚଳ AstSFormatArg* argp,
                               ଅଚଳ string& vformat) ଅଚଳ VL_MT_STABLE {
    ଅଚଳ AstEnumDType* ଅଚଳ dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_ସଂଖ୍ୟା ମୂଲ୍ୟ{ଏହି, dtypep->width()};
    ମୂଲ୍ୟ.opAssign(*ଏହି);
    ଅଚଳ VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    ଅଚଳ ଅକ୍ଷର fmt = std::tolower(vformat.back());
    ଯଦି (fmt != 'p' && fmt != 's') ଫେରନ୍ତୁ ମୂଲ୍ୟ.displayed(argp, vformat, numericAttr);
    string name = ମୂଲ୍ୟ.displayedEnumName(dtypep);
    ଯଦି (name.empty()) name = ମୂଲ୍ୟ.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_ସଂଖ୍ୟା name_ଅଙ୍କ{ଏହି};
    name_ଅଙ୍କ.setString(name);
    ଫେରନ୍ତୁ name_ଅଙ୍କ.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_ସଂଖ୍ୟା::displayed(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& vformat,
                           ଅଚଳ VFormatAttr& formatAttr) ଅଚଳ VL_MT_STABLE {
    ଫେରନ୍ତୁ displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_ସଂଖ୍ୟା::displayed(File_ଧାଡ଼ି* fl, ଅଚଳ string& vformat,
                           ଅଚଳ VFormatAttr& formatAttr) ଅଚଳ VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *ଏହି);
    ++pos;
    ତାର୍କିକ ବାମ = ମିଥ୍ୟା;
    ଯଦି (pos[0] == '-') {
        ବାମ = ସତ୍ୟ;
        ++pos;
    }
    string fmtsize;
    ପାଇଁ (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    ଅକ୍ଷର fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    ଯଦି (formatAttr.isComplex()) {
        ଯଦି (fmt != 'p') fmt = 's';  // override
    } ଅନ୍ୟଥା ଯଦି (formatAttr.isString()) {
        ଯଦି (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_ସଂଖ୍ୟା strwide{fl, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(toString().length()) * 8, 0};
            strwide.opNToI(*ଏହି);
            ଫେରନ୍ତୁ strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        ଯଦି (fmt != 'p') fmt = 's';  // override
    } ଅନ୍ୟଥା ଯଦି (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        ଯଦି (fmt == 'p') {
            ଯଦି (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } ଅନ୍ୟଥା {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } ଅନ୍ୟଥା ଯଦି (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        ଯଦି (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_ସଂଖ୍ୟା nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*ଏହି);
            ଫେରନ୍ତୁ nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    ବାଛ (fmt) {
    ସ୍ଥିତି 'c': {
        ଯଦି (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        ଅଚଳ unsigned ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = bits_ମୂଲ୍ୟ(0, 8);
        str.ଠେଲ_back(static_cast<ଅକ୍ଷର>(v));
        ଫେରନ୍ତୁ str;
    }
    ସ୍ଥିତି 'e':  // FALLTHRU
    ସ୍ଥିତି 'f':  // FALLTHRU
    ସ୍ଥିତି 'g': {
        ଅଚଳ ଦ୍ୱିଗୁଣ n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        ଅକ୍ଷର tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (ଶୂନ୍ୟ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        ଫେରନ୍ତୁ tmp;
    }
    ସ୍ଥିତି 's': {
        ଯଦି (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } ଅନ୍ୟଥା {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width() - 1;
            ତାର୍କିକ start = ସତ୍ୟ;
            ଯେତେବେଳେ ((bit % 8) != 7) ++bit;
            ପାଇଁ (; bit >= 0; bit -= 8) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = bits_ମୂଲ୍ୟ(bit - 7, 8);
                ଯଦି (!start || v) {
                    str += static_cast<ଅକ୍ଷର>((v == 0) ? ' ' : v);
                    start = ମିଥ୍ୟା;  // Drop leading 0s
                } ଅନ୍ୟଥା {
                    ଯଦି (fmtsize != "0") str += ' ';
                }
            }
        }
        ଅଚଳ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ବାମ, str);
        ଫେରନ୍ତୁ str;
    }
    ସ୍ଥିତି 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        ଯଦି (formatAttr.isDouble()) {
            ଅଚଳ ଦ୍ୱିଗୁଣ n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            ଅକ୍ଷର tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (ଶୂନ୍ୟ)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            ଫେରନ୍ତୁ tmp;
        }
        ଯଦି (formatAttr.isString())
            ଫେରନ୍ତୁ '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        ଯଦି (formatAttr.isComplex()) ଫେରନ୍ତୁ toString();
        ଫେରନ୍ତୁ "%p";
    }
    ସ୍ଥିତି 'b':  // FALLTHRU
    ସ୍ଥିତି 'o':  // FALLTHRU
    ସ୍ଥିତି 'h':  // FALLTHRU
    ସ୍ଥିତି 'x': {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width() - 1;
        ଯଦି (ବାମ || !fmtsize.empty()) {
            ଯେତେବେଳେ (bit && bitIs0(bit)) --bit;
        }
        ବାଛ (fmt) {
        ସ୍ଥିତି 'b': {
            ପାଇଁ (; bit >= 0; --bit) {
                ଯଦି (bitIs0(bit)) {
                    str += '0';
                } ଅନ୍ୟଥା ଯଦି (bitIs1(bit)) {
                    str += '1';
                } ଅନ୍ୟଥା ଯଦି (bitIsZ(bit)) {
                    str += 'z';
                } ଅନ୍ୟଥା {
                    str += 'x';
                }
            }
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 'o': {
            ଯେତେବେଳେ ((bit % 3) != 2) ++bit;
            ପାଇଁ (; bit > 0; bit -= 3) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_X = ଗଣନା_X(bit - 2, 3);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Z = ଗଣନା_Z(bit - 2, 3);
                ଯଦି (ଅଙ୍କ_X == 3 || ଅଙ୍କ_X == width() - (bit - 2)) {
                    str += 'x';
                    ଜାରି;
                }
                ଯଦି (ଅଙ୍କ_Z == 3 || ଅଙ୍କ_Z == width() - (bit - 2)) {
                    str += 'z';
                    ଜାରି;
                }
                ଯଦି (ଅଙ୍କ_X > 0) {
                    str += 'X';
                    ଜାରି;
                }
                ଯଦି (ଅଙ୍କ_Z > 0) {
                    str += 'Z';
                    ଜାରି;
                }
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = bits_ମୂଲ୍ୟ(bit - 2, 3);
                str += static_cast<ଅକ୍ଷର>('0' + v);
            }
            ଭାଙ୍ଗ;
        }
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: {  // h/x
            ଯେତେବେଳେ ((bit % 4) != 3) ++bit;
            ପାଇଁ (; bit > 0; bit -= 4) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_X = ଗଣନା_X(bit - 3, 4);
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Z = ଗଣନା_Z(bit - 3, 4);
                ଯଦି (ଅଙ୍କ_X == 4 || ଅଙ୍କ_X == width() - (bit - 3)) {
                    str += 'x';
                    ଜାରି;
                }
                ଯଦି (ଅଙ୍କ_Z == 4 || ଅଙ୍କ_Z == width() - (bit - 3)) {
                    str += 'z';
                    ଜାରି;
                }
                ଯଦି (ଅଙ୍କ_X > 0) {
                    str += 'X';
                    ଜାରି;
                }
                ଯଦି (ଅଙ୍କ_Z > 0) {
                    str += 'Z';
                    ଜାରି;
                }
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = bits_ମୂଲ୍ୟ(bit - 3, 4);
                ଯଦି (v >= 10) {
                    str += static_cast<ଅକ୍ଷର>('a' + v - 10);
                } ଅନ୍ୟଥା {
                    str += static_cast<ଅକ୍ଷର>('0' + v);
                }
            }
            ଭାଙ୍ଗ;
        }
        }  // switch
        ଅଚଳ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ବାମ ? ' ' : '0'), ବାମ, str);
        ଫେରନ୍ତୁ str;
    }  // case b/d/x/o
    ସ୍ଥିତି 't':  // Time
    ସ୍ଥିତି 'd': {  // Unsigned decimal
        ଅଚଳ ତାର୍କିକ issigned = formatAttr.isSigned();
        ଯଦି (fmtsize == "" && !ବାମ) {
            ଅଚଳ ଦ୍ୱିଗୁଣ mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            ଦ୍ୱିଗୁଣ dchars = mantissabits / 3.321928094887362 + 1.0;
            ଯଦି (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(dchars));
        }
        ତାର୍କିକ hasXZ = ମିଥ୍ୟା;
        ଯଦି (isAllX()) {
            str = "x";
            hasXZ = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (isAllZ()) {
            str = "z";
            hasXZ = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (isAnyX()) {
            str = "X";
            hasXZ = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (isAnyZ()) {
            str = "Z";
            hasXZ = ସତ୍ୟ;
        }
        ଯଦି (!hasXZ) {
            ଯଦି (issigned) {
                ଯଦି (width() > 64) {
                    str = toDecimalS();
                } ଅନ୍ୟଥା {
                    str = cvtToStr(toSQuad());
                }
            } ଅନ୍ୟଥା {
                ଯଦି (width() > 64) {
                    str = toDecimalU();
                } ଅନ୍ୟଥା {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        ଅଚଳ ତାର୍କିକ zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !ବାମ;
        // fmtsize might have changed since we parsed the %fmtsize
        ଅଚଳ size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ବାମ, str);
        ଫେରନ୍ତୁ str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    ସ୍ଥିତି 'u': {  // Packed 2-state
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
            ଅଚଳ uint32_t v = m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ;
            str += static_cast<ଅକ୍ଷର>((v >> 0) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v >> 8) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v >> 16) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v >> 24) & 0xff);
        }
        ଫେରନ୍ତୁ str;
    }
    ସ୍ଥିତି 'z': {  // Packed 4-state
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
            ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ >> 0) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ >> 8) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ >> 16) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ >> 24) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ_X >> 0) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ_X >> 8) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ_X >> 16) & 0xff);
            str += static_cast<ଅକ୍ଷର>((v.m_ମୂଲ୍ୟ_X >> 24) & 0xff);
        }
        ଫେରନ୍ତୁ str;
    }
    ସ୍ଥିତି 'v': {  // Strength
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width() - 1;
        ପାଇଁ (; bit >= 0; bit--) {
            ଯଦି (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            ଅନ୍ୟଥା ଯଦି (bitIs1(bit)) {
                str += "St1 ";
            } ଅନ୍ୟଥା ଯଦି (bitIsZ(bit)) {
                str += "StZ ";
            } ଅନ୍ୟଥା {
                str += "StX";
            }
        }
        ଫେରନ୍ତୁ str;
    }
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_ସଂଖ୍ୟା::emitC() ଅଚଳ VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    ଅକ୍ଷର sbuf[bufsize];
    string result;
    ଯଦି (isNull()) {
        ଫେରନ୍ତୁ "VlNull{}";
    } ଅନ୍ୟଥା ଯଦି (isDouble()) {
        ଅଚଳ ଦ୍ୱିଗୁଣ dnum = toDouble();
        ଯଦି (std::isinf(dnum)) {
            ଯଦି (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } ଅନ୍ୟଥା ଯଦି (std::isnan(dnum)) {
            ଯଦି (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } ଅନ୍ୟଥା {
            ଅଚଳ ଅକ୍ଷର* ଅଚଳ fmt = (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (ଶୂନ୍ୟ)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            ଫେରନ୍ତୁ sbuf;
        }
    } ଅନ୍ୟଥା ଯଦି (isString()) {
        ଅଚଳ string strg = toString();
        ଅଚଳ string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        ଫେରନ୍ତୁ '"' + quoted + "\"s";
    } ଅନ୍ୟଥା ଯଦି (ଶବ୍ଦଗୁଡ଼ିକ() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(ଶବ୍ଦଗୁଡ଼ିକ()) + ">{{";
        ଯଦି (ଶବ୍ଦଗୁଡ଼ିକ() > 4) result += '\n';
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0; n < ଶବ୍ଦଗୁଡ଼ିକ(); ++n) {
            ଯଦି (n) result += ((n % 4) ? ", " : ",\n");
            (ଶୂନ୍ୟ)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        ଯଦି (ଶବ୍ଦଗୁଡ଼ିକ() > 4) result += '\n';
        result += "}}";
    } ଅନ୍ୟଥା ଯଦି (ଶବ୍ଦଗୁଡ଼ିକ() == 2) {  // Quad
        ଅଚଳ uint64_t qnum = static_cast<uint64_t>(toUQuad());
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ଶୂନ୍ୟ)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        ଫେରନ୍ତୁ sbuf;
    } ଅନ୍ୟଥା {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        ଅଚଳ uint32_t unum = toUInt();
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (ଶୂନ୍ୟ)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        ଫେରନ୍ତୁ sbuf;
    }
    ଫେରନ୍ତୁ result;
}

string V3_ସଂଖ୍ୟା::toDecimalS() ଅଚଳ VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ଯଦି (isNegative()) {
        V3_ସଂଖ୍ୟା lhsNoSign = *ଏହି;
        lhsNoSign.opNegate(*ଏହି);
        ଫେରନ୍ତୁ "-"s + lhsNoSign.toDecimalU();
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ toDecimalU();
    }
}

string V3_ସଂଖ୍ୟା::toDecimalU() ଅଚଳ VL_MT_STABLE {
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_ସଂଖ୍ୟା bcd(ଏହି, maxdecwidth + 4);
    V3_ସଂଖ୍ୟା tmp(ଏହି, maxdecwidth + 4);
    V3_ସଂଖ୍ୟା tmp2(ଏହି, maxdecwidth + 4);

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା from_bit = width() - 1;
    // Skip all leading zeros
    ପାଇଁ (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    ପାଇଁ (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            ଯଦି (bcd.bits_ମୂଲ୍ୟ(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_ସଂଖ୍ୟା(ଏହି, 32, 1));
        // bcd[0] = this[from_bit]
        ଯଦି (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = (maxdecwidth - 1) & ~3;
    ପାଇଁ (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        ଯଦି (bcd.bits_ମୂଲ୍ୟ(lsb, 4)) ଭାଙ୍ଗ;
    }
    ପାଇଁ (; lsb >= 0; lsb -= 4) {
        output += static_cast<ଅକ୍ଷର>('0' + bcd.bits_ମୂଲ୍ୟ(lsb, 4));  // 0..9
    }
    ଫେରନ୍ତୁ output;
}

string V3_ସଂଖ୍ୟା::toRealString() ଅଚଳ VL_MT_STABLE {
    ଅଚଳ ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << ମୂଲ୍ୟ;
    string result = out.str();
    ଯଦି (std::strtod(result.c_str(), nullptr) != ମୂଲ୍ୟ) {
        out.str("");
        out.precision(17);
        out << ମୂଲ୍ୟ;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    ଯଦି (result.find_first_of(".en") == string::npos) result += ".0";
    ଫେରନ୍ତୁ result;
}

//======================================================================
// ACCESSORS - as numbers

ତାର୍କିକ V3_ସଂଖ୍ୟା::fitsInUInt() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isFourState()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    // We allow wide numbers that represent values <= 32 bits
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଯଦି (m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
uint32_t V3_ସଂଖ୍ୟା::toUInt() ଅଚଳ VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *ଏହି);
    // We allow wide numbers that represent values <= 32 bits
    ଯଦି (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *ଏହି);
    }
    ଫେରନ୍ତୁ m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ;
}

ଦ୍ୱିଗୁଣ V3_ସଂଖ୍ୟା::toDouble() ଅଚଳ VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    ସଂଘ {
        ଦ୍ୱିଗୁଣ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ;
    u.u[1] = m_data.ଅଙ୍କ()[1].m_ମୂଲ୍ୟ;
    ଫେରନ୍ତୁ u.d;
}

int32_t V3_ସଂଖ୍ୟା::toSInt() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isSigned()) {
        ଅଚଳ uint32_t v = toUInt();
        ଅଚଳ uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        ଅଚଳ uint32_t extended = v | signExtend;
        ଫେରନ୍ତୁ static_cast<int32_t>(extended);
    } ଅନ୍ୟଥା {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        ଫେରନ୍ତୁ static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_ସଂଖ୍ୟା::toUQuad() ଅଚଳ VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *ଏହି);
    // We allow wide numbers that represent values <= 64 bits
    ଯଦି (isDouble()) ଫେରନ୍ତୁ static_cast<uint64_t>(toDouble());
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 2; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଯଦି (m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ) {
            v3error("Value too wide for 64-bits expected in this context " << *ଏହି);
            ଭାଙ୍ଗ;
        }
    }
    ଯଦି (width() <= 32) ଫେରନ୍ତୁ (static_cast<uint64_t>(toUInt()));
    ଫେରନ୍ତୁ ((static_cast<uint64_t>(m_data.ଅଙ୍କ()[1].m_ମୂଲ୍ୟ) << 32ULL)
            | (static_cast<uint64_t>(m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ)));
}

int64_t V3_ସଂଖ୍ୟା::toSQuad() ଅଚଳ {
    ଯଦି (isDouble()) ଫେରନ୍ତୁ static_cast<int64_t>(toDouble());
    ଅଚଳ uint64_t v = toUQuad();
    ଅଚଳ uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    ଅଚଳ uint64_t extended = v | signExtend;
    ଫେରନ୍ତୁ static_cast<int64_t>(extended);
}

string V3_ସଂଖ୍ୟା::toString() ଅଚଳ {
    UASSERT(!isFourState(), "toString with 4-state " << *ଏହି);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    ଯଦି (isString()) ଫେରନ୍ତୁ m_data.str();
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width() - 1;
    ଯେତେବେଳେ ((bit % 8) != 7) ++bit;
    std::string str;
    ପାଇଁ (; bit >= 0; bit -= 8) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା v = bits_ମୂଲ୍ୟ(bit - 7, 8);
        ଯଦି (v) str += static_cast<ଅକ୍ଷର>(v);
    }
    ଫେରନ୍ତୁ str;
}

uint32_t V3_ସଂଖ୍ୟା::edataWord(ପୂର୍ଣ୍ଣସଂଖ୍ୟା eword) ଅଚଳ {
    UASSERT(!isFourState(), "edataWord with 4-state " << *ଏହି);
    ଫେରନ୍ତୁ m_data.ଅଙ୍କ()[eword].m_ମୂଲ୍ୟ;
}

uint8_t V3_ସଂଖ୍ୟା::dataByte(ପୂର୍ଣ୍ଣସଂଖ୍ୟା byte) ଅଚଳ {
    ଫେରନ୍ତୁ (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

ତାର୍କିକ V3_ସଂଖ୍ୟା::isAllZ() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isDouble() || isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଅଚଳ uint32_t mask = i == ଶବ୍ଦଗୁଡ଼ିକ() - 1 ? hiWordMask() : ~0U;
        ଯଦି (((~v.m_ମୂଲ୍ୟ & v.m_ମୂଲ୍ୟ_X) & mask) != mask) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isAllX() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isDouble() || isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    uint32_t mask = hiWordMask();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = ଶବ୍ଦଗୁଡ଼ିକ() - 1; i >= 0; --i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଯଦି ((v.m_ମୂଲ୍ୟ & v.m_ମୂଲ୍ୟ_X) ^ mask) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        mask = ~0U;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isEqZero() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isString()) ଫେରନ୍ତୁ m_data.str().empty();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଯଦି (v.m_ମୂଲ୍ୟ || v.m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isNeqZero() ଅଚଳ {
    ଯଦି (isString()) ଫେରନ୍ତୁ !m_data.str().empty();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଯଦି (v.m_ମୂଲ୍ୟ & ~v.m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isBitsZero(ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb) ଅଚଳ {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = lsb; i <= msb; ++i) {
        ଯଦି (VL_UNLIKELY(!bitIs0(i))) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isEqOne() ଅଚଳ {
    ଯଦି (m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ != 1 || m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଯଦି (v.m_ମୂଲ୍ୟ || v.m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isEqAllOnes(ପୂର୍ଣ୍ଣସଂଖ୍ୟା optwidth) ଅଚଳ {
    // Correct number of zero bits/width matters
    ଯଦି (!optwidth) optwidth = width();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < optwidth; ++bit) {
        ଯଦି (!bitIs1(bit)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isFourState() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isDouble() || isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଯଦି (m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isAnyX() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isDouble() || isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଯଦି (v.m_ମୂଲ୍ୟ & v.m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isAnyXZ() ଅଚଳ {
    ଯଦି (isDouble() || isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଯଦି (m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isAnyZ() ଅଚଳ VL_MT_SAFE {
    ଯଦି (isDouble() || isString()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
        ଯଦି (~v.m_ମୂଲ୍ୟ & v.m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
ତାର୍କିକ V3_ସଂଖ୍ୟା::isLtXZ(ଅଚଳ V3_ସଂଖ୍ୟା& rhs) ଅଚଳ {
    // Include X/Z in comparisons for sort ordering
    ଯଦି (isString()) ଫେରନ୍ତୁ toString() < rhs.toString();
    ଯଦି (isDouble()) ଫେରନ୍ତୁ toDouble() < rhs.toDouble();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        ଯଦି (bitIs1(bit) && rhs.bitIs0(bit)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (rhs.bitIs1(bit) && bitIs0(bit)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (bitIsXZ(bit)) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (rhs.bitIsXZ(bit)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}
ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3_ସଂଖ୍ୟା::ଗଣନା_X(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits) ଅଚଳ VL_MT_SAFE {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_2 = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitn = 0; bitn < nbits; ++bitn) {
        ଯଦି (lsb + bitn >= width()) ଫେରନ୍ତୁ ଗଣନା_2;
        ଯଦି (bitIsX(lsb + bitn)) ++ଗଣନା_2;
    }
    ଫେରନ୍ତୁ ଗଣନା_2;
}
ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3_ସଂଖ୍ୟା::ଗଣନା_Z(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits) ଅଚଳ VL_MT_SAFE {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_2 = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitn = 0; bitn < nbits; ++bitn) {
        ଯଦି (lsb + bitn >= width()) ଫେରନ୍ତୁ ଗଣନା_2;
        ଯଦି (bitIsZ(lsb + bitn)) ++ଗଣନା_2;
    }
    ଫେରନ୍ତୁ ଗଣନା_2;
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3_ସଂଖ୍ୟା::widthToFit() ଅଚଳ {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width() - 1; bit > 0; bit--) {
        ଯଦି (!bitIs0(bit)) ଫେରନ୍ତୁ bit + 1;
    }
    ଫେରନ୍ତୁ 1;  // one bit even if number is == 0
}

uint32_t V3_ସଂଖ୍ୟା::ଗଣନା_Bits(ଅଚଳ V3_ସଂଖ୍ୟା& ctrl) ଅଚଳ {
    // Correct number of zero bits/width matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ବାଛ (ctrl.bitIs(0)) {
        ସ୍ଥିତି '0':
            ଯଦି (bitIs0(bit)) ++n;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି '1':
            ଯଦି (bitIs1(bit)) ++n;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି 'x':
            ଯଦି (bitIsX(bit)) ++n;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି 'z':
            ଯଦି (bitIsZ(bit)) ++n;
            ଭାଙ୍ଗ;
        }
    }
    ଫେରନ୍ତୁ n;
}

uint32_t V3_ସଂଖ୍ୟା::ଗଣନା_Bits(ଅଚଳ V3_ସଂଖ୍ୟା& ctrl1, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl2,
                             ଅଚଳ V3_ସଂଖ୍ୟା& ctrl3) ଅଚଳ {
    // Correct number of zero bits/width matters
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = ଗଣନା_Bits(ctrl1);
    ଯଦି (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += ଗଣନା_Bits(ctrl2);
    ଯଦି ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += ଗଣନା_Bits(ctrl3);
    }
    ଫେରନ୍ତୁ n;
}

uint32_t V3_ସଂଖ୍ୟା::ଗଣନା_Ones() ଅଚଳ {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା n = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (bitIs1(bit)) ++n;
    }
    ଫେରନ୍ତୁ n;
}

uint32_t V3_ସଂଖ୍ୟା::mostSetBitP1() ଅଚଳ {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = width() - 1; bit >= 0; bit--) {
        ଯଦି (!bitIs0(bit)) ଫେରନ୍ତୁ bit + 1;
    }
    ଫେରନ୍ତୁ 0;
}

uint32_t V3_ସଂଖ୍ୟା::leastSetBitP1() ଅଚଳ {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (!bitIs0(bit)) ଫେରନ୍ତୁ bit + 1;
    }
    ଫେରନ୍ତୁ 0;
}

//======================================================================

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opBitsNonXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opBitsOne(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opBitsXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opBitsZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    ଫେରନ୍ତୁ *ଏହି;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRedOr(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // op i, 1 bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଅକ୍ଷର outc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < lhs.width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit)) {
            ଫେରନ୍ତୁ setSingleBits(1);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(bit)) {
        } ଅନ୍ୟଥା {
            outc = 'x';
        }
    }
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRedAnd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଅକ୍ଷର outc = 1;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < lhs.width(); ++bit) {
        ଯଦି (lhs.bitIs0(bit)) {
            ଫେରନ୍ତୁ setSingleBits(0);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs1(bit)) {
        } ଅନ୍ୟଥା {
            outc = 'x';
        }
    }
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRedXor(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // op i, 1 bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଅକ୍ଷର outc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < lhs.width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit)) {
            ଯଦି (outc == 1) {
                outc = 0;
            } ଅନ୍ୟଥା ଯଦି (outc == 0) {
                outc = 1;
            }
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(bit)) {
        } ଅନ୍ୟଥା {
            outc = 'x';
        }
    }
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::op_ଗଣନା_Bits(ଅଚଳ V3_ସଂଖ୍ୟା& ପଦସମଷ୍ଟି, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl1, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl2,
                                ଅଚଳ V3_ସଂଖ୍ୟା& ctrl3) {
    // Correct number of zero bits/width matters
    ଅଙ୍କ_ASSERT_OP_ARGS4(ପଦସମଷ୍ଟି, ctrl1, ctrl2, ctrl3);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS4(ପଦସମଷ୍ଟି, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = ପଦସମଷ୍ଟି.ଗଣନା_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::op_ଗଣନା_Ones(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଯଦି (lhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    setZero();
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = lhs.ଗଣନା_Ones();
    opCleanThis();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opIsUnknown(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.isAnyXZ());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opOneHot(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଯଦି (lhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଫେରନ୍ତୁ setSingleBits(lhs.ଗଣନା_Ones() == 1);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opOneHot0(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଯଦି (lhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଫେରନ୍ତୁ setSingleBits(lhs.ଗଣନା_Ones() <= 1);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opCLog2(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଯଦି (lhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା adjust = (lhs.ଗଣନା_Ones() == 1) ? 0 : 1;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = lhs.width() - 1; bit >= 0; bit--) {
        ଯଦି (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            ଫେରନ୍ତୁ *ଏହି;
        }
    }
    setZero();
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opMostSetBitP1(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଯଦି (lhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = lhs.width() - 1; bit >= 0; bit--) {
        ଯଦି (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            ଫେରନ୍ତୁ *ଏହି;
        }
    }
    setZero();
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLogNot(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    ଅକ୍ଷର outc = 1;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < lhs.width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit)) {
            outc = 0;
            ଯାଅ last;
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(bit)) {
        } ଅନ୍ୟଥା {
            outc = 'x';
        }
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNot(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // Correct number of zero bits/width matters
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opAnd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } ଅନ୍ୟଥା {
            setBit(bit, 'x');
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opOr(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } ଅନ୍ୟଥା {
            setBit(bit, 'x');
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opXor(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    ଫେରନ୍ତୁ *ଏହି;
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::fillBits(ପୂର୍ଣ୍ଣସଂଖ୍ୟା destLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ଅକ୍ଷର ମୂଲ୍ୟ) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    ଯଦି (!width) ଫେରନ୍ତୁ;
    UASSERT(width <= ଏହି->width() - destLsb, "Destination bit range exceeds number width");

    ଅଚଳ uint32_t ମୂଲ୍ୟ_Word = ମୂଲ୍ୟ == '1' || ମୂଲ୍ୟ == 'x' || ମୂଲ୍ୟ == 1 || ମୂଲ୍ୟ == 3 ? ~0U : 0;
    ଅଚଳ uint32_t xWord = ମୂଲ୍ୟ == 'z' || ମୂଲ୍ୟ == 'x' || ମୂଲ୍ୟ == 2 || ମୂଲ୍ୟ == 3 ? ~0U : 0;
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstWord = VL_BITWORD_E(destLsb);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastWord = VL_BITWORD_E(destLsb + width - 1);
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା word = firstWord; word <= lastWord; ++word) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        ଅଚଳ uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        ମୂଲ୍ୟ_And_X& dest = m_data.ଅଙ୍କ()[word];
        dest.m_ମୂଲ୍ୟ = (dest.m_ମୂଲ୍ୟ & ~mask) | (ମୂଲ୍ୟ_Word & mask);
        dest.m_ମୂଲ୍ୟ_X = (dest.m_ମୂଲ୍ୟ_X & ~mask) | (xWord & mask);
    }
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::copyBits(ପୂର୍ଣ୍ଣସଂଖ୍ୟା destLsb, ଅଚଳ V3_ସଂଖ୍ୟା& source, ପୂର୍ଣ୍ଣସଂଖ୍ୟା sourceLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    ଯଦି (!width) ଫେରନ୍ତୁ;
    UASSERT(width <= ଏହି->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstWord = VL_BITWORD_E(destLsb);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastWord = VL_BITWORD_E(destLsb + width - 1);
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା destWord = firstWord; destWord <= lastWord; ++destWord) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା destOffset = VL_BITBIT_E(destBit);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sourceBit = sourceLsb + destBit - destLsb;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sourceWord = VL_BITWORD_E(sourceBit);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା sourceOffset = VL_BITBIT_E(sourceBit);
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        ଅଚଳ uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        ଅଚଳ ମୂଲ୍ୟ_And_X& sourceLo = source.m_data.ଅଙ୍କ()[sourceWord];
        uint32_t ମୂଲ୍ୟ = sourceLo.m_ମୂଲ୍ୟ >> sourceOffset;
        uint32_t ମୂଲ୍ୟ_X = sourceLo.m_ମୂଲ୍ୟ_X >> sourceOffset;
        ଯଦି (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            ଅଚଳ ମୂଲ୍ୟ_And_X& sourceHi = source.m_data.ଅଙ୍କ()[sourceWord + 1];
            ମୂଲ୍ୟ |= sourceHi.m_ମୂଲ୍ୟ << (VL_EDATASIZE - sourceOffset);
            ମୂଲ୍ୟ_X |= sourceHi.m_ମୂଲ୍ୟ_X << (VL_EDATASIZE - sourceOffset);
        }

        ମୂଲ୍ୟ_And_X& dest = m_data.ଅଙ୍କ()[destWord];
        dest.m_ମୂଲ୍ୟ = (dest.m_ମୂଲ୍ୟ & ~mask) | ((ମୂଲ୍ୟ << destOffset) & mask);
        dest.m_ମୂଲ୍ୟ_X = (dest.m_ମୂଲ୍ୟ_X & ~mask) | ((ମୂଲ୍ୟ_X << destOffset) & mask);
    }
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opConcat(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Correct number of zero bits/width matters
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    ଯଦି (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLenN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRepl(ଅଚଳ V3_ସଂଖ୍ୟା& lhs,
                           ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    ଫେରନ୍ତୁ opRepl(lhs, rhs.toUInt());
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRepl(ଅଚଳ V3_ସଂଖ୍ୟା& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଯଦି (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    ପାଇଁ (unsigned times = 0; times < rhsval; ++times) {
        ଅଚଳ uint64_t destLsb = uint64_t{times} * lhs.width();
        ଯଦି (destLsb >= static_cast<uint32_t>(width())) ଭାଙ୍ଗ;
        copyBits(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(destLsb)));
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opStreamL(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    ଯଦି (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା istart = 0; istart < lhs.width(); istart += ssize) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLogAnd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଅକ୍ଷର loutc = 0;
    ଅକ୍ଷର routc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < lhs.width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit)) {
            loutc = 1;
            ଭାଙ୍ଗ;
        }
        ଯଦି (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < rhs.width(); ++bit) {
        ଯଦି (rhs.bitIs1(bit)) {
            routc = 1;
            ଭାଙ୍ଗ;
        }
        ଯଦି (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    ଅକ୍ଷର outc = 'x';
    ଯଦି (routc == 1 && loutc == 1) outc = 1;
    ଯଦି (routc == 0 || loutc == 0) outc = 0;
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLogOr(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଅକ୍ଷର outc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < lhs.width(); ++bit) {
        ଯଦି (lhs.bitIs1(bit)) {
            outc = 1;
            ଯାଅ last;
        }
        ଯଦି (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < rhs.width(); ++bit) {
        ଯଦି (rhs.bitIs1(bit)) {
            outc = 1;
            ଯାଅ last;
        }
        ଯଦି (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLogIf(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ସଂଖ୍ୟା lnot = lhs;
    lnot.opLogNot(lhs);
    ଫେରନ୍ତୁ opLogOr(lnot, rhs);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLogEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ସଂଖ୍ୟା ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_ସଂଖ୍ୟା ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    ଫେରନ୍ତୁ opLogAnd(ifa, ifb);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opAtoN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ପୂର୍ଣ୍ଣସଂଖ୍ୟା base) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    ଯଦି (base == AstAtoN::ATOREAL) ଫେରନ୍ତୁ setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    ଯଦି (errno != 0) v = 0;
    ଫେରନ୍ତୁ setLongS(static_cast<int32_t>(v));
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opPutcN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ଅଚଳ V3_ସଂଖ୍ୟା& ths) {
    ଅଙ୍କ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    ଅଚଳ int32_t i = rhs.toSInt();
    ଅଚଳ int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    ଯଦି (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    ଫେରନ୍ତୁ setString(lstring);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGetcN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    ଅଚଳ string lstring = lhs.toString();
    ଅଚଳ int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    ଯଦି (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    ଫେରନ୍ତୁ setLong(v);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSubstrN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ଅଚଳ V3_ସଂଖ୍ୟା& ths) {
    ଅଙ୍କ_ASSERT_OP_ARGS3(lhs, rhs, ths);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    ଅଚଳ string lstring = lhs.toString();
    ଅଚଳ int32_t i = rhs.toSInt();
    ଅଚଳ int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    ଯଦି (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) ଫେରନ୍ତୁ setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    ଫେରନ୍ତୁ setString(lstring.substr(i, j - i + 1));
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opCompareNN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ତାର୍କିକ ignoreCase) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା result;
    ଅଚଳ string lstring = lhs.toString();
    ଅଚଳ string rstring = rhs.toString();
    ଯଦି (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } ଅନ୍ୟଥା {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    ଫେରନ୍ତୁ setLongS(result);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଯଦି (lhs.isString()) ଫେରନ୍ତୁ opEqN(lhs, rhs);
    ଯଦି (lhs.isDouble()) ଫେରନ୍ତୁ opEqD(lhs, rhs);
    ଅକ୍ଷର outc = 1;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ଯଦି (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            ଯାଅ last;
        }
        ଯଦି (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            ଯାଅ last;
        }
        ଯଦି (lhs.bitIsXZ(bit)) outc = 'x';
        ଯଦି (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNeq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଯଦି (lhs.isString()) ଫେରନ୍ତୁ opNeqN(lhs, rhs);
    ଯଦି (lhs.isDouble()) ଫେରନ୍ତୁ opNeqD(lhs, rhs);
    ଅକ୍ଷର outc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ଯଦି (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            ଯାଅ last;
        }
        ଯଦି (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            ଯାଅ last;
        }
        ଯଦି (lhs.bitIsXZ(bit)) outc = 'x';
        ଯଦି (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3Hash V3_ସଂଖ୍ୟା::toHash() ଅଚଳ {
    // Shouldmatch isCaseEq
    ଯଦି (isString()) ଫେରନ୍ତୁ V3Hash{toString()};
    ଯଦି (isDouble()) ଫେରନ୍ତୁ V3Hash{toDouble()};
    V3Hash hash{width()};
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) hash += m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ;
    ଫେରନ୍ତୁ hash;
}

ତାର୍କିକ V3_ସଂଖ୍ୟା::isCaseEq(ଅଚଳ V3_ସଂଖ୍ୟା& rhs) ଅଚଳ {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଯଦି (isString()) ଫେରନ୍ତୁ toString() == rhs.toString();
    ଯଦି (isDouble()) ଫେରନ୍ତୁ toDouble() == rhs.toDouble();
    ଯଦି (width() != rhs.width()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
        ଯଦି (!(m_data.ଅଙ୍କ()[i] == rhs.m_data.ଅଙ୍କ()[i])) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opCaseEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opCaseNeq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅକ୍ଷର outc = 0;
    ଯଦି (lhs.isString()) {
        ଫେରନ୍ତୁ opNeqN(lhs, rhs);
    } ଅନ୍ୟଥା ଯଦି (lhs.isDouble()) {
        ଫେରନ୍ତୁ opNeqD(lhs, rhs);
    }
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ଯଦି (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            ଯାଅ last;
        }
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opWildEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଅକ୍ଷର outc = 1;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ଯଦି (!rhs.bitIsXZ(bit)) {
            ଯଦି (lhs.bitIsXZ(bit)) {
                outc = 'x';
                ଯାଅ last;
            }
            ଯଦି (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                ଯାଅ last;
            }
        }
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opWildNeq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଅକ୍ଷର outc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ଯଦି (!rhs.bitIsXZ(bit)) {
            ଯଦି (lhs.bitIsXZ(bit)) {
                outc = 'x';
                ଯାଅ last;
            }
            ଯଦି (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                ଯାଅ last;
            }
        }
    }
last:
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGt(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଅକ୍ଷର outc = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        ଯଦି (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        ଯଦି (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        ଯଦି (lhs.bitIsXZ(bit)) outc = 'x';
        ଯଦି (rhs.bitIsXZ(bit)) outc = 'x';
    }
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGtS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଅକ୍ଷର outc = 0;
    {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        ଯଦି (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } ଅନ୍ୟଥା ଯଦି (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } ଅନ୍ୟଥା ଯଦି (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } ଅନ୍ୟଥା {
            // both positive or negative, normal >
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                ଯଦି (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                ଯଦି (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                ଯଦି (lhs.bitIsXZ(bit)) outc = 'x';
                ଯଦି (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    ଫେରନ୍ତୁ setSingleBits(outc);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGte(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ସଂଖ୍ୟା& eq = opEq(lhs, rhs);
    ଯଦି (eq.isNeqZero()) ଫେରନ୍ତୁ eq;  // Return true
    ଫେରନ୍ତୁ opGt(lhs, rhs);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGteS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    V3_ସଂଖ୍ୟା& eq = opEq(lhs, rhs);
    ଯଦି (eq.isNeqZero()) ଫେରନ୍ତୁ eq;  // Return true
    ଫେରନ୍ତୁ opGtS(lhs, rhs);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLt(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { ଫେରନ୍ତୁ opGt(rhs, lhs); }
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLte(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { ଫେରନ୍ତୁ opGte(rhs, lhs); }
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLtS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { ଫେରନ୍ତୁ opGtS(rhs, lhs); }
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLteS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) { ଫେରନ୍ତୁ opGteS(rhs, lhs); }

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opShiftR(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    setZero();
    ଯଦି (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ଫେରନ୍ତୁ *ଏହି;
    ଅଚଳ uint32_t rhsval = rhs.toUInt();
    ଯଦି (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rhsval),
                 std::min(width(), lhs.width() - static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rhsval)));
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opShiftRS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    setZero();
    ଅଚଳ ତାର୍କିକ overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    ଯଦି (!overflow) {
        ଅଚଳ uint32_t rhsval = rhs.toUInt();
        ଯଦି (rhsval < lbits) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyWidth = std::min(width(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(lbits - rhsval));
            copyBits(0, lhs, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            ଫେରନ୍ତୁ *ଏହି;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opShiftL(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    setZero();
    ଯଦି (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) ଫେରନ୍ତୁ *ଏହି;
    ଅଚଳ uint32_t rhsval = rhs.toUInt();
    ଯଦି (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rhsval), lhs, 0,
                 std::min(width() - static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(rhsval), lhs.width()));
    }
    ଫେରନ୍ତୁ *ଏହି;
}

//======================================================================
// Ops - Arithmetic

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNegate(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଯଦି (lhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    V3_ସଂଖ୍ୟା notlhs(&lhs, width());
    notlhs.opNot(lhs);
    ଅଚଳ V3_ସଂଖ୍ୟା one(&lhs, width(), 1);
    opAdd(notlhs, one);
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opAdd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା word = 0; word < ଶବ୍ଦଗୁଡ଼ିକ(); ++word) {
        ଅଚଳ uint64_t lwordval = lhs.m_data.ଅଙ୍କ()[word].m_ମୂଲ୍ୟ;
        ଅଚଳ uint64_t rwordval = rhs.m_data.ଅଙ୍କ()[word].m_ମୂଲ୍ୟ;
        ଅଚଳ uint64_t sum = lwordval + rwordval + carry;
        m_data.ଅଙ୍କ()[word].m_ମୂଲ୍ୟ = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSub(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    V3_ସଂଖ୍ୟା negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    ଫେରନ୍ତୁ opAdd(lhs, negrhs);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opMul(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    setZero();
    ଯଦି (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } ଅନ୍ୟଥା {
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା lword = 0; lword < lhs.ଶବ୍ଦଗୁଡ଼ିକ(); ++lword) {
            ଅଚଳ uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.ଅଙ୍କ()[lword].m_ମୂଲ୍ୟ);
            ଯଦି (lwordval == 0) ଜାରି;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା rword = 0; rword < rhs.ଶବ୍ଦଗୁଡ଼ିକ(); ++rword) {
                ଅଚଳ uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.ଅଙ୍କ()[rword].m_ମୂଲ୍ୟ);
                ଯଦି (rwordval == 0) ଜାରି;
                uint64_t mul = lwordval * rwordval;
                ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା qword = lword + rword; qword < ଶବ୍ଦଗୁଡ଼ିକ(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.ଅଙ୍କ()[qword].m_ମୂଲ୍ୟ);
                    m_data.ଅଙ୍କ()[qword].m_ମୂଲ୍ୟ = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    ଯଦି (mul == 0) ଭାଙ୍ଗ;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opMulS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    V3_ସଂଖ୍ୟା lhsNoSign = lhs;
    ଯଦି (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ସଂଖ୍ୟା rhsNoSign = rhs;
    ଯଦି (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ଅଚଳ V3_ସଂଖ୍ୟା qNoSign = opMul(lhsNoSign, rhsNoSign);
    ଯଦି ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ଅନ୍ୟଥା {
        opAssign(qNoSign);
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opDiv(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଯଦି (rhs.isEqZero()) ଫେରନ୍ତୁ setAllBitsXRemoved();
    ଯଦି (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        ଫେରନ୍ତୁ *ଏହି;
    } ଅନ୍ୟଥା {
        // Wide division
        ଫେରନ୍ତୁ opModDivGuts(lhs, rhs, ମିଥ୍ୟା);
    }
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opDivS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଯଦି (rhs.isEqZero()) ଫେରନ୍ତୁ setAllBitsXRemoved();
    V3_ସଂଖ୍ୟା lhsNoSign = lhs;
    ଯଦି (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ସଂଖ୍ୟା rhsNoSign = rhs;
    ଯଦି (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ଅଚଳ V3_ସଂଖ୍ୟା qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    ଯଦି ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } ଅନ୍ୟଥା {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *ଏହି);
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opModDiv(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଯଦି (rhs.isEqZero()) ଫେରନ୍ତୁ setAllBitsXRemoved();
    ଯଦି (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        ଫେରନ୍ତୁ *ଏହି;
    } ଅନ୍ୟଥା {
        // Wide modulus
        ଫେରନ୍ତୁ opModDivGuts(lhs, rhs, ସତ୍ୟ);
    }
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opModDivS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଯଦି (rhs.isEqZero()) ଫେରନ୍ତୁ setAllBitsXRemoved();
    V3_ସଂଖ୍ୟା lhsNoSign = lhs;
    ଯଦି (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_ସଂଖ୍ୟା rhsNoSign = rhs;
    ଯଦି (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    ଅଚଳ V3_ସଂଖ୍ୟା qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    ଯଦି (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } ଅନ୍ୟଥା {
        opAssign(qNoSign);
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opModDivGuts(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ତାର୍କିକ is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା l_ଶବ୍ଦଗୁଡ଼ିକ = lhs.ଶବ୍ଦଗୁଡ଼ିକ();
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା umsbp1 = lhs.mostSetBitP1();  // dividend
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା vmsbp1 = rhs.mostSetBitP1();  //ଭାଜକ
    ଯଦି (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *ଏହି);
        ଫେରନ୍ତୁ *ଏହି;
    }

    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    ଯଦି (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା j = uw - 1; j >= 0; j--) {
            ଅଚଳ uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.ଅଙ୍କ()[j].m_ମୂଲ୍ୟ));
            m_data.ଅଙ୍କ()[j].m_ମୂଲ୍ୟ = unw64 / static_cast<uint64_t>(rhs.m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ);
            k = unw64
                - (static_cast<uint64_t>(m_data.ଅଙ୍କ()[j].m_ମୂଲ୍ୟ)
                   * static_cast<uint64_t>(rhs.m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *ଏହି << " rem=0x" << std::hex
                                   << k << std::dec);
        ଯଦି (is_modulus) {
            setZero();
            m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = k;
        }
        opCleanThis();
        ଫେରନ୍ତୁ *ଏହି;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_ଶବ୍ଦଗୁଡ଼ିକ + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_ଶବ୍ଦଗୁଡ଼ିକ + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < l_ଶବ୍ଦଗୁଡ଼ିକ; ++i) m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ = 0;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < l_ଶବ୍ଦଗୁଡ଼ିକ + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    ଅଚଳ uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ << s)
                | (shift_mask & (rhs.m_data.ଅଙ୍କ()[i - 1].m_ମୂଲ୍ୟ >> (32 - s)));
    }
    vn[0] = rhs.m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ << s;

    // Copy and shift dividend by same amount; may set new upper word
    ଯଦି (s) {
        un[uw] = lhs.m_data.ଅଙ୍କ()[uw - 1].m_ମୂଲ୍ୟ >> (32 - s);
    } ଅନ୍ୟଥା {
        un[uw] = 0;
    }
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ << s)
                | (shift_mask & (lhs.m_data.ଅଙ୍କ()[i - 1].m_ମୂଲ୍ୟ >> (32 - s)));
    }
    un[0] = lhs.m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା j = uw - vw; j >= 0; j--) {
        // Estimate
        ଅଚଳ uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        ଯଦି (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            ଯଦି (rhat < 0x100000000ULL) ଯାଅ again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < vw; ++i) {
            ଅଚଳ uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.ଅଙ୍କ()[j].m_ମୂଲ୍ୟ = qhat;  // Save quotient digit

        ଯଦି (t < 0) {
            // Over subtracted; correct by adding back
            m_data.ଅଙ୍କ()[j].m_ମୂଲ୍ୟ--;
            k = 0;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < vw; ++i) {
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

    ଯଦି (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < vw; ++i) {
            m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = vw; i < l_ଶବ୍ଦଗୁଡ଼ିକ; ++i) m_data.ଅଙ୍କ()[i].m_ମୂଲ୍ୟ = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *ଏହି);
        ଫେରନ୍ତୁ *ଏହି;
    } ଅନ୍ୟଥା {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *ଏହି);
        ଫେରନ୍ତୁ *ଏହି;
    }
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opPow(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ତାର୍କିକ lsign, ତାର୍କିକ rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ଯଦି (lhs.isFourState() || rhs.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଯଦି (rhs.isEqZero()) ଫେରନ୍ତୁ setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    ଯଦି (rsign && rhs.isNegative()) {
        ଯଦି (lhs.isEqZero()) {
            ଫେରନ୍ତୁ setAllBitsXRemoved();
        } ଅନ୍ୟଥା ଯଦି (lhs.isEqOne()) {
            ଫେରନ୍ତୁ setValue1();
        } ଅନ୍ୟଥା ଯଦି (lsign && lhs.isEqAllOnes()) {
            ଯଦି (rhs.bitIs1(0)) {
                ଫେରନ୍ତୁ setAllBits1();  // -1^odd=-1
            } ଅନ୍ୟଥା {
                ଫେରନ୍ତୁ setValue1();  // -1^even=1
            }
        }
        ଫେରନ୍ତୁ setZero();
    }
    ଯଦି (lhs.isEqZero()) ଫେରନ୍ତୁ setZero();
    setZero();
    m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = 1;
    V3_ସଂଖ୍ୟା power(&lhs, width());
    power.opAssign(lhs);
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < rhs.width(); ++bit) {
        ଯଦି (bit > 0) {  // power = power*power
            V3_ସଂଖ୍ୟା lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        ଯଦି (rhs.bitIs1(bit)) {  // out *= power
            V3_ସଂଖ୍ୟା lastOut(&lhs, width());
            lastOut.opAssign(*ଏହି);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opPowSU(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Correct number of zero bits/width matters
    ଫେରନ୍ତୁ opPow(lhs, rhs, ସତ୍ୟ, ମିଥ୍ୟା);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opPowSS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Correct number of zero bits/width matters
    ଫେରନ୍ତୁ opPow(lhs, rhs, ସତ୍ୟ, ସତ୍ୟ);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opPowUS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // Correct number of zero bits/width matters
    ଫେରନ୍ତୁ opPow(lhs, rhs, ମିଥ୍ୟା, ସତ୍ୟ);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opBufIf1(ଅଚଳ V3_ସଂଖ୍ୟା& ens, ଅଚଳ V3_ସଂଖ୍ୟା& if1s) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(ens, if1s);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit < width(); ++bit) {
        ଯଦି (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } ଅନ୍ୟଥା {
            setBit(bit, 'z');
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}

// Sets all bits in range to the given value
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSetRange(uint32_t lsb, uint32_t width, ଅକ୍ଷର bit_ମୂଲ୍ୟ) {
    ଯଦି (lsb < static_cast<uint32_t>(ଏହି->width())) {
        fillBits(static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(lsb),
                 static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(std::min(width, static_cast<uint32_t>(ଏହି->width()) - lsb)),
                 bit_ମୂଲ୍ୟ);
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opAssign(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) { ଫେରନ୍ତୁ opAssignNonXZ(lhs, ମିଥ୍ୟା); }
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opAssignNonXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ତାର୍କିକ ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    ଯଦି (ଏହି != &lhs) {
        ଯଦି (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (isString()) {
            ଯଦି (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } ଅନ୍ୟଥା {
                m_data.str() = lhs.m_data.str();
            }
        } ଅନ୍ୟଥା ଯଦି (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } ଅନ୍ୟଥା ଯଦି (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } ଅନ୍ୟଥା ଯଦି (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } ଅନ୍ୟଥା {
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା word = 0; word < ଶବ୍ଦଗୁଡ଼ିକ(); ++word) {
                ଅଚଳ uint32_t ମୂଲ୍ୟ = word < lhs.ଶବ୍ଦଗୁଡ଼ିକ() ? lhs.m_data.ଅଙ୍କ()[word].m_ମୂଲ୍ୟ
                                                                & ~lhs.m_data.ଅଙ୍କ()[word].m_ମୂଲ୍ୟ_X
                                                          : 0;
                m_data.ଅଙ୍କ()[word] = {ମୂଲ୍ୟ, 0};
            }
            opCleanThis();
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNToI(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    // String to packed number
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    setZero();
    ଅଚଳ string& str = lhs.toString();
    ପାଇଁ (size_t n = 0; n < str.length(); ++n) {
        ଅଚଳ ଅକ୍ଷର c = str[str.length() - 1 - n];
        ପାଇଁ (size_t cbit = 0; cbit < 8; ++cbit) {
            ଅଚଳ std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opExtendS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyWidth = std::min(width(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opExtendXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyWidth = std::min(width(), static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opClean(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t bits) { ଫେରନ୍ତୁ opSel(lhs, bits - 1, 0); }

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::opCleanThis(ତାର୍କିକ warnOnTruncation) {
    // Clean MSB of number
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(*ଏହି);
    ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[ଶବ୍ଦଗୁଡ଼ିକ() - 1];
    ଅଚଳ uint32_t new_ମୂଲ୍ୟ_Msb = v.m_ମୂଲ୍ୟ & hiWordMask();
    ଅଚଳ uint32_t new_ମୂଲ୍ୟ_XMsb = v.m_ମୂଲ୍ୟ_X & hiWordMask();
    ଯଦି (warnOnTruncation && (new_ମୂଲ୍ୟ_Msb != v.m_ମୂଲ୍ୟ || new_ମୂଲ୍ୟ_XMsb != v.m_ମୂଲ୍ୟ_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.ଅଙ୍କ()[ଶବ୍ଦଗୁଡ଼ିକ() - 1] = {new_ମୂଲ୍ୟ_Msb, new_ମୂଲ୍ୟ_XMsb};
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSel(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& msb, ଅଚଳ V3_ସଂଖ୍ୟା& lsb) {
    ଅଙ୍କ_ASSERT_OP_ARGS3(lhs, msb, lsb);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS2(msb, lsb);
    ଯଦି (lsb.isFourState() || msb.isFourState()) ଫେରନ୍ତୁ setAllBitsX();
    ଫେରନ୍ତୁ opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSel(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t msbval, uint32_t lsbval) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyWidth = 0;
    ଯଦି (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        ଅଚଳ uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    ଫେରନ୍ତୁ *ଏହି;
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSelInto(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
    ଫେରନ୍ତୁ opSelInto(lhs, lsb.toSInt(), width);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSelInto(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsbval, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା copyWidth = std::max(0, std::min({width, lhs.width(), ଏହି->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା fillWidth
        = std::max(0, std::min(width - copyWidth, ଏହି->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    ଫେରନ୍ତୁ *ଏହି;
}

//======================================================================
// Ops - Floating point

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opIToRD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ତାର୍କିକ isSigned) {
    // Correct number of zero bits/width matters
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_ସଂଖ୍ୟା noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    ଦ୍ୱିଗୁଣ d = 0;
    ଅଚଳ ତାର୍କିକ negate = isSigned && noxz.isNegative();
    ଯଦି (negate) {
        ଅଚଳ V3_ସଂଖ୍ୟା noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        ଯଦି (noxz.bitIs1(bit)) d += exp2(bit);
    }
    ଯଦି (negate) d = -d;
    ଫେରନ୍ତୁ setDouble(d);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRToIS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(lhs);
    ଅଚଳ ଦ୍ୱିଗୁଣ v = VL_TRUNC(lhs.toDouble());
    ଅଚଳ int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    ଫେରନ୍ତୁ setLongS(i);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRToIRoundS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(lhs);
    ଅଚଳ ଦ୍ୱିଗୁଣ v = VL_ROUND(lhs.toDouble());
    setZero();
    ସଂଘ {
        ଦ୍ୱିଗୁଣ d;
        uint64_t q;
    } u;
    u.d = v;
    (ଶୂନ୍ୟ)u.d;

    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା exp = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = exp - 52;
    ଅଚଳ uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    ଯଦି (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit = 0; bit <= 52; ++bit) {
            ଯଦି (mantissa & (1ULL << bit)) {
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା outbit = bit + lsb;
                ଯଦି (outbit >= 0) setBit(outbit, 1);
            }
        }
        ଯଦି (v < 0) {
            ଅଚଳ V3_ସଂଖ୍ୟା noSign = *ଏହି;
            opNegate(noSign);
        }
    }
    ଫେରନ୍ତୁ *ଏହି;
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opRealToBits(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    ସଂଘ {
        ଦ୍ୱିଗୁଣ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    ଫେରନ୍ତୁ setQuad(u.m_v);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opBitsToRealD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    ସଂଘ {
        ଦ୍ୱିଗୁଣ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    ଫେରନ୍ତୁ setDouble(u.m_d);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNegateD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS1(lhs);
    ଫେରନ୍ତୁ setDouble(-lhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opAddD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opSubD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opMulD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opDivD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opPowD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opEqD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNeqD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGtD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGteD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLtD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLteD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opConcatN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setString(lhs.toString() + rhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opReplN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_LOGIC_ARGS1(rhs);
    ଫେରନ୍ତୁ opReplN(lhs, rhs.toUInt());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opReplN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t rhsval) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    ପାଇଁ (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    ଫେରନ୍ତୁ setString(out);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opCvtPackString(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଫେରନ୍ତୁ setString(lhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opToLowerN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    ଅଚଳ std::string out = VString::downcase(lhs.toString());
    ଫେରନ୍ତୁ setString(out);
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opToUpperN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS1(lhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS1(lhs);
    ଅଚଳ std::string out = VString::upcase(lhs.toString());
    ଫେରନ୍ତୁ setString(out);
}

V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opEqN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toString() == rhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opNeqN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toString() != rhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGtN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toString() > rhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opGteN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toString() >= rhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLtN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toString() < rhs.toString());
}
V3_ସଂଖ୍ୟା& V3_ସଂଖ୍ୟା::opLteN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) {
    ଅଙ୍କ_ASSERT_OP_ARGS2(lhs, rhs);
    ଅଙ୍କ_ASSERT_STRING_ARGS2(lhs, rhs);
    ଫେରନ୍ତୁ setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_ଧାଡ଼ି* ଅଚଳ fl = ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::builtInFilename()};
    V3_ସଂଖ୍ୟା ଅଙ୍କ{fl, 32, 0};
    ଅଙ୍କ.selfTestThis();
}

ଶୂନ୍ୟ V3_ସଂଖ୍ୟା::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonEqual(0, 0), ସତ୍ୟ);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonEqual(1e19, 1e19), ସତ୍ୟ);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonEqual(9, 0.0001), ମିଥ୍ୟା);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonEqual(1, 1 + std::numeric_limits<ଦ୍ୱିଗୁଣ>::epsilon()), ସତ୍ୟ);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonEqual(0.009, 0.00899999999999999931998839741709), ସତ୍ୟ);

    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(0), ସତ୍ୟ);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(1), ସତ୍ୟ);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(-1), ସତ୍ୟ);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(1.0001), ମିଥ୍ୟା);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(0.9999), ମିଥ୍ୟା);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(-1.0001), ମିଥ୍ୟା);
    UASSERT_SELFTEST(V3_ସଂଖ୍ୟା::epsilonIntegral(-0.9999), ମିଥ୍ୟା);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
