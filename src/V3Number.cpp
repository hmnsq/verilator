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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Number.h"

#அந்தர்பாவ "V3File.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cerrno>
#அந்தர்பாவ <cmath>
#அந்தர்பாவ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr முழு MAX_SPRINTF_DOUBLE_SIZE
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#define இலக்கம்_ASSERT_OP_ARGS1(arg1) \
    UASSERT((இது != &(arg1)), "Number operation called with same source and dest")
#define இலக்கம்_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((இது != &(arg1) && இது != &(arg2)), \
            "Number operation called with same source and dest")
#define இலக்கம்_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((இது != &(arg1) && இது != &(arg2) && இது != &(arg3)), \
            "Number operation called with same source and dest")
#define இலக்கம்_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((இது != &(arg1) && இது != &(arg2) && இது != &(arg3) && இது != &(arg4)), \
            "Number operation called with same source and dest")

#define இலக்கம்_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).dataType() == V3_எண்_Data::V3_எண்_Data_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#define இலக்கம்_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    இலக்கம்_ASSERT_LOGIC_ARGS1(arg1); \
    இலக்கம்_ASSERT_LOGIC_ARGS1(arg2)

#define இலக்கம்_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    இலக்கம்_ASSERT_LOGIC_ARGS1(arg1); \
    இலக்கம்_ASSERT_LOGIC_ARGS1(arg2); \
    இலக்கம்_ASSERT_LOGIC_ARGS1(arg3); \
    இலக்கம்_ASSERT_LOGIC_ARGS1(arg4)

#define இலக்கம்_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#define இலக்கம்_ASSERT_STRING_ARGS2(arg1, arg2) \
    இலக்கம்_ASSERT_STRING_ARGS1(arg1); \
    இலக்கம்_ASSERT_STRING_ARGS1(arg2)

#define இலக்கம்_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#define இலக்கம்_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    இலக்கம்_ASSERT_DOUBLE_ARGS1(arg1); \
    இலக்கம்_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

வெறுமை V3_எண்::v3errorEnd(மாறிலி std::ostringstream& str) மாறிலி VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    எனில் (m_nodep) {
        m_nodep->v3errorEnd(nsstr);
    } இல்லையெனில் எனில் (m_fileline) {
        m_fileline->v3errorEnd(nsstr);
    } இல்லையெனில் {
        V3Error::v3errorEnd(nsstr, "", nullptr);
    }
}

வெறுமை V3_எண்::v3errorEndFatal(மாறிலி std::ostringstream& str) மாறிலி
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ப்ரதிஜ்ஞா(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_எண்::V3_எண்(Ast_முனை* nodep, VerilogStringLiteral, மாறிலி string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    எனில் (str.empty()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } இல்லையெனில் {
        init(nodep, std::max<முழு>(str.length() * 8, 1));
        க்கு (unsigned pos = 0; pos < str.length(); ++pos) {
            மாறிலி முழு topos = str.length() - 1 - pos;
            மதிப்பு_And_X& v = m_data.இலக்கம்()[topos / 4];
            க்கு (முழு bit = 0; bit < 8; ++bit) {
                எனில் (str[pos] & (1UL << bit)) v.m_மதிப்பு |= (1UL << (bit + (topos % 4) * 8));
            }
        }
    }
    m_data.m_fromString = உண்மை;
    opCleanThis(உண்மை);
}

V3_எண்::V3_எண்(Ast_முனை* nodep, மாறிலி Ast_முனை_DType* nodedtypep) {
    எனில் (nodedtypep->isCHandle()) {
        init(nodep);
        setNull();
    } இல்லையெனில் எனில் (nodedtypep->isDouble()) {
        init(nodep, 64);
        setDouble(0.0);
    } இல்லையெனில் எனில் (nodedtypep->isString()) {
        init(nodep);
        setString("");
    } இல்லையெனில் {
        init(nodep, nodedtypep->width(), nodedtypep->widthSized());
    }
}

வெறுமை V3_எண்::create(மாறிலி எழுத்து* sourcep) {
    m_data.setLogic();
    மாறிலி எழுத்து* மதிப்பு_startp = sourcep;
    க்கு (மாறிலி எழுத்து* cp = sourcep; *cp; ++cp) {
        எனில் (*cp == '\'') {
            மதிப்பு_startp = cp + 1;
            நிறுத்து;
        }
    }

    தருக்க userSized = பொய்;
    தருக்க unbased = பொய்;
    எழுத்து base = '\0';
    எனில் (மதிப்பு_startp != sourcep) {  // Has a '
        string widthn;
        மாறிலி எழுத்து* cp = sourcep;
        க்கு (; *cp; ++cp) {
            எனில் (*cp == '\'') {
                ++cp;
                நிறுத்து;
            }
            எனில் (*cp != '_') widthn += *cp;
        }
        போது (*cp == '_') cp++;
        எனில் (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(உண்மை);
        }
        எனில் (*cp) {
            base = *cp;
            ++cp;
        }
        மதிப்பு_startp = cp;

        userSized = widthn.length() != 0;
        எனில் (std::atoi(widthn.c_str())) {
            எனில் (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_இலக்கம்_Width()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                width(v3Global.opt.max_இலக்கம்_Width(), உண்மை);
            } இல்லையெனில் {
                width(std::atoi(widthn.c_str()), உண்மை);
            }
        }
    } இல்லையெனில் {
        unbased = உண்மை;
        base = 'd';
    }

    க்கு (முழு i = 0; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    எனில் (base == '0') {
        width(1, பொய்);  // So we extend it
        setBit(0, 0);
        m_data.m_autoExtend = உண்மை;
    } இல்லையெனில் எனில் (base == '1') {
        width(1, பொய்);  // So we extend it
        setBit(0, 1);
        m_data.m_autoExtend = உண்மை;
    } இல்லையெனில் எனில் (std::tolower(base) == 'z') {
        width(1, பொய்);  // So we extend it
        setBit(0, 'z');
        m_data.m_autoExtend = உண்மை;
    } இல்லையெனில் எனில் (std::tolower(base) == 'x') {
        width(1, பொய்);  // So we extend it
        setBit(0, 'x');
        m_data.m_autoExtend = உண்மை;
    }
    // Otherwise...
    இல்லையெனில் எனில் (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        மாறிலி முழு widthBound = std::max<முழு>(32, std::strlen(மதிப்பு_startp) * 4);
        width(widthBound, பொய்);  // Will change width below
        எனில் (unbased) isSigned(உண்மை);  // Also says the spec.
    }

    // Ignore leading blanks
    போது (*மதிப்பு_startp == '_' || std::isspace(*மதிப்பு_startp)) ++மதிப்பு_startp;
    எனில் (!*மதிப்பு_startp && !m_data.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    எனில் (userSized && m_data.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    முழு obit = 0;  // Start at LSB
    முழு base_align = 1;
    எனில் (std::tolower(base) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        போது (*மதிப்பு_startp == '_' || *மதிப்பு_startp == '0') ++மதிப்பு_startp;
        // Convert decimal number to hex
        முழு olen = 0;
        uint32_t val = 0;
        முழு got_x = 0;
        முழு got_z = 0;
        முழு got_01 = 0;
        தருக்க warnedTooMany = பொய்;
        க்கு (மாறிலி எழுத்து* cp = மதிப்பு_startp; *cp; ++cp) {
            தேர்வு (std::tolower(*cp)) {
            நிலை '0':  // FALLTHRU
            நிலை '1':  // FALLTHRU
            நிலை '2':  // FALLTHRU
            நிலை '3':  // FALLTHRU
            நிலை '4':  // FALLTHRU
            நிலை '5':  // FALLTHRU
            நிலை '6':  // FALLTHRU
            நிலை '7':  // FALLTHRU
            நிலை '8':  // FALLTHRU
            நிலை '9': {
                எனில் (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_data.இலக்கம்()[0].m_மதிப்பு = val;
                } இல்லையெனில் {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_எண் product(இது, width() + 4);  // +4 for overflow detection
                    மாறிலி V3_எண் ten(இது, width() + 4, 10);
                    மாறிலி V3_எண் addend(இது, width(), (*cp - '0'));
                    product.opMul(*இது, ten);
                    opAdd(product, addend);
                    எனில் (!warnedTooMany && product.bits_மதிப்பு(width(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = உண்மை;
                    }
                }
                ++olen;
                got_01 = 1;
                நிறுத்து;
            }
            நிலை 'z':
            நிலை '?': {
                got_z = 1;
                எனில் (!userSized) width(32, பொய்);
                setAllBitsZ();
                நிறுத்து;
            }
            நிலை 'x': {
                got_x = 1;
                எனில் (!userSized) width(32, பொய்);
                setAllBitsX();
                நிறுத்து;
            }
            நிலை '_': நிறுத்து;
            இயல்பு: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                நிறுத்து;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = width();
        எனில் ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << மதிப்பு_startp);
        }
    } இல்லையெனில் {
        // Convert bin/octal number to hex
        க்கு (மாறிலி எழுத்து* cp = மதிப்பு_startp + std::strlen(மதிப்பு_startp) - 1; cp >= மதிப்பு_startp;
             cp--) {
            எனில் (*cp != '_' && *cp != '0' && obit >= width()) {
                warnTooMany(sourcep);
                நிறுத்து;
            }
            தேர்வு (std::tolower(base)) {
            நிலை 'b': {
                base_align = 1;
                தேர்வு (std::tolower(*cp)) {
                நிலை '0': setBit(obit++, 0); நிறுத்து;
                நிலை '1': setBit(obit++, 1); நிறுத்து;
                நிலை 'z':
                நிலை '?': setBit(obit++, 'z'); நிறுத்து;
                நிலை 'x': setBit(obit++, 'x'); நிறுத்து;
                நிலை '_': நிறுத்து;
                இயல்பு: v3error("Illegal character in binary constant: " << *cp);
                }
                நிறுத்து;
            }

            நிலை 'o':
            நிலை 'c': {
                base_align = 3;
                தேர்வு (std::tolower(*cp)) {  // clang-format off
                நிலை '0': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 0);  நிறுத்து;
                நிலை '1': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 0);  நிறுத்து;
                நிலை '2': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 0);  நிறுத்து;
                நிலை '3': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 0);  நிறுத்து;
                நிலை '4': setBit(obit++, 0); setBit(obit++, 0);  setBit(obit++, 1);  நிறுத்து;
                நிலை '5': setBit(obit++, 1); setBit(obit++, 0);  setBit(obit++, 1);  நிறுத்து;
                நிலை '6': setBit(obit++, 0); setBit(obit++, 1);  setBit(obit++, 1);  நிறுத்து;
                நிலை '7': setBit(obit++, 1); setBit(obit++, 1);  setBit(obit++, 1);  நிறுத்து;
                நிலை 'z':
                நிலை '?': setBit(obit++, 'z'); setBit(obit++, 'z'); setBit(obit++, 'z'); நிறுத்து;
                நிலை 'x': setBit(obit++, 'x'); setBit(obit++, 'x'); setBit(obit++, 'x'); நிறுத்து;
                நிலை '_': நிறுத்து;
                // clang-format on
                இயல்பு: v3error("Illegal character in octal constant");
                }
                நிறுத்து;
            }

            நிலை 'h':  // FALLTHRU
            நிலை 'x': {
                base_align = 4;
                தேர்வு (std::tolower(*cp)) {  // clang-format off
                நிலை '0': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); நிறுத்து;
                நிலை '1': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); நிறுத்து;
                நிலை '2': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); நிறுத்து;
                நிலை '3': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); நிறுத்து;
                நிலை '4': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); நிறுத்து;
                நிலை '5': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); நிறுத்து;
                நிலை '6': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); நிறுத்து;
                நிலை '7': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); நிறுத்து;
                நிலை '8': setBit(obit++,0); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); நிறுத்து;
                நிலை '9': setBit(obit++,1); setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); நிறுத்து;
                நிலை 'a': setBit(obit++,0); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); நிறுத்து;
                நிலை 'b': setBit(obit++,1); setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); நிறுத்து;
                நிலை 'c': setBit(obit++,0); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); நிறுத்து;
                நிலை 'd': setBit(obit++,1); setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); நிறுத்து;
                நிலை 'e': setBit(obit++,0); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); நிறுத்து;
                நிலை 'f': setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); setBit(obit++,1); நிறுத்து;
                நிலை 'z':
                நிலை '?': setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); setBit(obit++,'z'); நிறுத்து;
                நிலை 'x': setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); setBit(obit++,'x'); நிறுத்து;
                    // clang-format on
                நிலை '_': நிறுத்து;
                இயல்பு:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    நிறுத்து;
                }  // LCOV_EXCL_STOP
                நிறுத்து;
            }
            இயல்பு:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << base);
                நிறுத்து;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    எனில் (!userSized && !m_data.m_autoExtend) {
        width(std::max(32, base_align * ((widthToFit() + base_align - 1) / base_align)), பொய்);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    போது (obit <= width() && obit && bitIsXZ(obit - 1)) {
        setBit(obit, bitIs(obit - 1));
        ++obit;
    }
    opCleanThis(உண்மை);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

வெறுமை V3_எண்::warnTooMany(மாறிலி string& மதிப்பு) {
    v3warn(WIDTHDIGITS, "Too many digits for " << width() << " bit number: '" << மதிப்பு << "'");
}

வெறுமை V3_எண்::nodep(Ast_முனை* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    எனில் (!nodep) திரும்பு;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

தருக்க V3_எண்::epsilonEqual(இரட்டை a, இரட்டை b) {
    திரும்பு std::fabs(a - b)
           <= (std::numeric_limits<இரட்டை>::epsilon() * std::max(1.0, std::max(a, b)));
}

தருக்க V3_எண்::epsilonIntegral(இரட்டை a) {
    மாறிலி இரட்டை dint = static_cast<இரட்டை>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    திரும்பு epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

முழு V3_எண்::log2b(uint32_t இலக்கம்) {
    // See also opCLog2
    க்கு (முழு bit = 31; bit > 0; bit--) {
        எனில் (இலக்கம் & (1ULL << bit)) திரும்பு bit;
    }
    திரும்பு 0;
}

முழு V3_எண்::log2bQuad(uint64_t இலக்கம்) {
    எனில் (இலக்கம் >> 32ULL) திரும்பு 32 + log2b(இலக்கம் >> 32ULL);
    திரும்பு log2b(இலக்கம்);
}

//======================================================================
// Setters

V3_எண்& V3_எண்::setZero() {
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, 0};
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setQuad(uint64_t மதிப்பு) {
    setZero();
    m_data.இலக்கம்()[0].m_மதிப்பு = மதிப்பு & 0xffffffffULL;
    எனில் (width() > 32) m_data.இலக்கம்()[1].m_மதிப்பு = (மதிப்பு >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setLong(uint32_t மதிப்பு) {
    setZero();
    m_data.இலக்கம்()[0].m_மதிப்பு = மதிப்பு;
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setLongS(int32_t மதிப்பு) {
    setZero();
    ஒன்றிப்பு {
        uint32_t u;
        int32_t s;
    } u;
    u.s = மதிப்பு;
    (வெறுமை)u.s;
    m_data.இலக்கம்()[0].m_மதிப்பு = u.u;
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setDouble(இரட்டை மதிப்பு) {
    UASSERT(width() == 64, "Real operation on wrong sized number");
    m_data.setDouble();
    ஒன்றிப்பு {
        இரட்டை d;
        uint32_t u[2];
    } u;
    u.d = மதிப்பு;
    (வெறுமை)u.d;
    க்கு (முழு i = 2; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, 0};
    m_data.இலக்கம்()[0].m_மதிப்பு = u.u[0];
    m_data.இலக்கம்()[1].m_மதிப்பு = u.u[1];
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setSingleBits(எழுத்து மதிப்பு) {
    க்கு (முழு i = 1 /*upper*/; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, 0};
    m_data.இலக்கம்()[0] = {(மதிப்பு == '1' || மதிப்பு == 'x' || மதிப்பு == 1 || மதிப்பு == 3),
                       (மதிப்பு == 'z' || மதிப்பு == 'x' || மதிப்பு == 2 || மதிப்பு == 3)};
    திரும்பு *இது;
}

V3_எண்& V3_எண்::setAllBits0() {
    setZero();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setAllBits1() {
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {~0U, 0};
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setAllBitsX() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {~0U, ~0U};
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setAllBitsZ() {
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, ~0U};
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::setAllBitsXRemoved() {
    எனில் (!v3Global.constRemoveXs()) {
        திரும்பு setAllBitsX();
    } இல்லையெனில் {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        எனில் (v3Global.opt.xAssign() == "1") {
            திரும்பு setAllBits1();
        } இல்லையெனில் {
            திரும்பு setAllBits0();
        }
    }
}
V3_எண்& V3_எண்::setValue1() {
    m_data.இலக்கம்()[0] = {1, 0};
    க்கு (முழு i = 1; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, 0};
    திரும்பு *இது;
}

வெறுமை V3_எண்::setBitX0(முழு bit) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    setBit(bit, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_எண்& V3_எண்::setMask(முழு nbits, முழு lsb) {
    setZero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fillBits(lsb, std::max(0, std::min(nbits, width() - lsb)), 1);
    திரும்பு *இது;
}

//======================================================================
// ACCESSORS - as strings

string V3_எண்::ascii(தருக்க prefixed, தருக்க cleanVerilog) மாறிலி VL_MT_STABLE {
    // Correct number of zero bits/width matters
    எனில் (is1Step()) திரும்பு "1step";
    எனில் (isNull()) திரும்பு "null";
    std::ostringstream out;
    எனில் (isDouble()) {
        out.precision(17);
        எனில் (VL_UNCOVERABLE(width() != 64)) {
            out << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } இல்லையெனில் {
            out << toDouble();
            எனில் (toDouble() == floor(toDouble())) out << ".0";
        }
        திரும்பு out.str();
    } இல்லையெனில் எனில் (isString()) {
        திரும்பு '"' + toString() + '"';
    } இல்லையெனில் {
        எனில் (VL_UNCOVERABLE((m_data.இலக்கம்()[சொற்கள்() - 1].m_மதிப்பு | m_data.இலக்கம்()[சொற்கள்() - 1].m_மதிப்பு_X)
                           & ~hiWordMask())) {
            out << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    எனில் (prefixed) {
        எனில் (sized()) {
            out << width() << "'";
        } இல்லையெனில் எனில் (autoExtend() && width() == 1) {
            out << "'";
            எனில் (bitIs0(0)) {
                out << '0';
            } இல்லையெனில் எனில் (bitIs1(0)) {
                out << '1';
            } இல்லையெனில் எனில் (bitIsZ(0)) {
                out << 'z';
            } இல்லையெனில் {
                out << 'x';
            }
            திரும்பு out.str();
        } இல்லையெனில் {
            எனில் (cleanVerilog) {
                out << "'";
            } இல்லையெனில் {
                out << "?" << width() << "?";
            }
        }
        எனில் (isSigned()) out << "s";
    }

    மாறிலி தருக்க இருமம் = (isFourState()
#ifdef V3_எண்_ASCII_இருமம்
                         // cppcheck-suppress konwnConditionTrueFalse
                         || உண்மை
#endif
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    எனில் (இருமம்) {
        out << "b";
        out << displayed("%0b");
    } இல்லையெனில் {
        எனில் (prefixed) out << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        out << displayed("%0h");
    }
    எனில் (isNull()) {
        எனில் (VL_UNCOVERABLE(!isEqZero())) {
            out << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } இல்லையெனில் {
            out << " [null]";
        }
    }
    திரும்பு out.str();
}

தருக்க V3_எண்::displayedFmtHasArg(எழுத்து format, தருக்க isScan) {
    // Is this a format letter that takes an argument in runtime?
    (வெறுமை)isScan;
    தேர்வு (std::tolower(format)) {
    நிலை 'b': திரும்பு உண்மை;  //இருமம்
    நிலை 'c': திரும்பு உண்மை;  // Character
    நிலை 'd': திரும்பு உண்மை;  // Decimal
    நிலை 'e': திரும்பு உண்மை;  // Floating
    நிலை 'f': திரும்பு உண்மை;  // Floating
    நிலை 'g': திரும்பு உண்மை;  // Floating
    நிலை 'h': திரும்பு உண்மை;  // Hex
    நிலை 'o': திரும்பு உண்மை;  // Octal
    நிலை 'p': திரும்பு உண்மை;  // Pattern
    நிலை 's': திரும்பு உண்மை;  // String
    நிலை 't': திரும்பு உண்மை;  // Time
    நிலை 'u': திரும்பு உண்மை;  // Packed 2-state
    நிலை 'v': திரும்பு உண்மை;  // Strength
    நிலை 'x': திரும்பு உண்மை;  // Hex
    நிலை 'z': திரும்பு உண்மை;  // Packed 4-state
    நிலை '?': திரும்பு உண்மை;  // Internal: Compute format in V3Width based on data type
    இயல்பு: திரும்பு பொய்;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_எண்::displayPad(size_t fmtsize, எழுத்து pad, தருக்க இடது, மாறிலி string& in) VL_PURE {
    string padding;
    எனில் (in.length() < fmtsize) padding = string(fmtsize - in.length(), pad);
    திரும்பு இடது ? (in + padding) : (padding + in);
}

string V3_எண்::displayedEnumName(மாறிலி AstEnumItem* itemp) {
    மாறிலி string name = VIdProtect::protect(itemp->prettyName());
    திரும்பு VString::isIdentifier(name) && !std::isdigit(static_cast<unsigned எழுத்து>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_எண்::displayedEnumName(மாறிலி AstEnumDType* dtypep) மாறிலி VL_MT_STABLE {
    V3_எண் மதிப்பு{இது, dtypep->width()};
    மதிப்பு.opAssign(*இது);
    க்கு (மாறிலி AstEnumItem* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), EnumItem)) {
        மாறிலி AstConst* மாறிலி constp = VN_AS(itemp->valuep(), Const);
        எனில் (!constp->இலக்கம்().isAnyXZ() && மதிப்பு.isCaseEq(constp->இலக்கம்()))
            திரும்பு displayedEnumName(itemp);
    }
    திரும்பு "";
}

string V3_எண்::displayedEnum(மாறிலி AstSFormatArg* argp,
                               மாறிலி string& vformat) மாறிலி VL_MT_STABLE {
    மாறிலி AstEnumDType* மாறிலி dtypep = VN_AS(argp->dtypep()->skipRefToEnump(), EnumDType);
    V3_எண் மதிப்பு{இது, dtypep->width()};
    மதிப்பு.opAssign(*இது);
    மாறிலி VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    மாறிலி எழுத்து fmt = std::tolower(vformat.back());
    எனில் (fmt != 'p' && fmt != 's') திரும்பு மதிப்பு.displayed(argp, vformat, numericAttr);
    string name = மதிப்பு.displayedEnumName(dtypep);
    எனில் (name.empty()) name = மதிப்பு.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_எண் name_இலக்கம்{இது};
    name_இலக்கம்.setString(name);
    திரும்பு name_இலக்கம்.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_எண்::displayed(மாறிலி Ast_முனை* nodep, மாறிலி string& vformat,
                           மாறிலி VFormatAttr& formatAttr) மாறிலி VL_MT_STABLE {
    திரும்பு displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_எண்::displayed(File_வரி* fl, மாறிலி string& vformat,
                           மாறிலி VFormatAttr& formatAttr) மாறிலி VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    auto pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *இது);
    ++pos;
    தருக்க இடது = பொய்;
    எனில் (pos[0] == '-') {
        இடது = உண்மை;
        ++pos;
    }
    string fmtsize;
    க்கு (; pos != vformat.cend() && (std::isdigit(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    எழுத்து fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    எனில் (formatAttr.isComplex()) {
        எனில் (fmt != 'p') fmt = 's';  // override
    } இல்லையெனில் எனில் (formatAttr.isString()) {
        எனில் (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_எண் strwide{fl, static_cast<முழு>(toString().length()) * 8, 0};
            strwide.opNToI(*இது);
            திரும்பு strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        எனில் (fmt != 'p') fmt = 's';  // override
    } இல்லையெனில் எனில் (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        எனில் (fmt == 'p') {
            எனில் (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } இல்லையெனில் {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } இல்லையெனில் எனில் (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        எனில் (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_எண் nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*இது);
            திரும்பு nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    தேர்வு (fmt) {
    நிலை 'c': {
        எனில் (width() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        மாறிலி unsigned முழு v = bits_மதிப்பு(0, 8);
        str.தள்ளு_back(static_cast<எழுத்து>(v));
        திரும்பு str;
    }
    நிலை 'e':  // FALLTHRU
    நிலை 'f':  // FALLTHRU
    நிலை 'g': {
        மாறிலி இரட்டை n = formatAttr.isDouble()   ? toDouble()
                         : formatAttr.isSigned() ? toSQuad()
                                                 : toUQuad();
        எழுத்து tmp[MAX_SPRINTF_DOUBLE_SIZE];
        (வெறுமை)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, vformat.c_str(), n);
        திரும்பு tmp;
    }
    நிலை 's': {
        எனில் (formatAttr.isString() || formatAttr.isComplex()) {
            str = toString();
        } இல்லையெனில் {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            முழு bit = width() - 1;
            தருக்க start = உண்மை;
            போது ((bit % 8) != 7) ++bit;
            க்கு (; bit >= 0; bit -= 8) {
                மாறிலி முழு v = bits_மதிப்பு(bit - 7, 8);
                எனில் (!start || v) {
                    str += static_cast<எழுத்து>((v == 0) ? ' ' : v);
                    start = பொய்;  // Drop leading 0s
                } இல்லையெனில் {
                    எனில் (fmtsize != "0") str += ' ';
                }
            }
        }
        மாறிலி size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', இடது, str);
        திரும்பு str;
    }
    நிலை 'p': {  // Pattern
        // 'p' with NUMBER was earlier converted to 'd'
        எனில் (formatAttr.isDouble()) {
            மாறிலி இரட்டை n = formatAttr.isDouble()   ? toDouble()
                             : formatAttr.isSigned() ? toSQuad()
                                                     : toUQuad();
            எழுத்து tmp[MAX_SPRINTF_DOUBLE_SIZE];
            (வெறுமை)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_SIZE, "%g", n);
            திரும்பு tmp;
        }
        எனில் (formatAttr.isString())
            திரும்பு '"' + V3OutFormatter::quoteNameControls(toString()) + '"';
        எனில் (formatAttr.isComplex()) திரும்பு toString();
        திரும்பு "%p";
    }
    நிலை 'b':  // FALLTHRU
    நிலை 'o':  // FALLTHRU
    நிலை 'h':  // FALLTHRU
    நிலை 'x': {
        முழு bit = width() - 1;
        எனில் (இடது || !fmtsize.empty()) {
            போது (bit && bitIs0(bit)) --bit;
        }
        தேர்வு (fmt) {
        நிலை 'b': {
            க்கு (; bit >= 0; --bit) {
                எனில் (bitIs0(bit)) {
                    str += '0';
                } இல்லையெனில் எனில் (bitIs1(bit)) {
                    str += '1';
                } இல்லையெனில் எனில் (bitIsZ(bit)) {
                    str += 'z';
                } இல்லையெனில் {
                    str += 'x';
                }
            }
            நிறுத்து;
        }
        நிலை 'o': {
            போது ((bit % 3) != 2) ++bit;
            க்கு (; bit > 0; bit -= 3) {
                மாறிலி முழு இலக்கம்_X = மொத்தம்_X(bit - 2, 3);
                மாறிலி முழு இலக்கம்_Z = மொத்தம்_Z(bit - 2, 3);
                எனில் (இலக்கம்_X == 3 || இலக்கம்_X == width() - (bit - 2)) {
                    str += 'x';
                    தொடர்;
                }
                எனில் (இலக்கம்_Z == 3 || இலக்கம்_Z == width() - (bit - 2)) {
                    str += 'z';
                    தொடர்;
                }
                எனில் (இலக்கம்_X > 0) {
                    str += 'X';
                    தொடர்;
                }
                எனில் (இலக்கம்_Z > 0) {
                    str += 'Z';
                    தொடர்;
                }
                மாறிலி முழு v = bits_மதிப்பு(bit - 2, 3);
                str += static_cast<எழுத்து>('0' + v);
            }
            நிறுத்து;
        }
        இயல்பு: {  // h/x
            போது ((bit % 4) != 3) ++bit;
            க்கு (; bit > 0; bit -= 4) {
                மாறிலி முழு இலக்கம்_X = மொத்தம்_X(bit - 3, 4);
                மாறிலி முழு இலக்கம்_Z = மொத்தம்_Z(bit - 3, 4);
                எனில் (இலக்கம்_X == 4 || இலக்கம்_X == width() - (bit - 3)) {
                    str += 'x';
                    தொடர்;
                }
                எனில் (இலக்கம்_Z == 4 || இலக்கம்_Z == width() - (bit - 3)) {
                    str += 'z';
                    தொடர்;
                }
                எனில் (இலக்கம்_X > 0) {
                    str += 'X';
                    தொடர்;
                }
                எனில் (இலக்கம்_Z > 0) {
                    str += 'Z';
                    தொடர்;
                }
                மாறிலி முழு v = bits_மதிப்பு(bit - 3, 4);
                எனில் (v >= 10) {
                    str += static_cast<எழுத்து>('a' + v - 10);
                } இல்லையெனில் {
                    str += static_cast<எழுத்து>('0' + v);
                }
            }
            நிறுத்து;
        }
        }  // switch
        மாறிலி size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (இடது ? ' ' : '0'), இடது, str);
        திரும்பு str;
    }  // case b/d/x/o
    நிலை 't':  // Time
    நிலை 'd': {  // Unsigned decimal
        மாறிலி தருக்க issigned = formatAttr.isSigned();
        எனில் (fmtsize == "" && !இடது) {
            மாறிலி இரட்டை mantissabits = width() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            இரட்டை dchars = mantissabits / 3.321928094887362 + 1.0;
            எனில் (issigned) ++dchars;  // space for sign
            fmtsize = cvtToStr(static_cast<முழு>(dchars));
        }
        தருக்க hasXZ = பொய்;
        எனில் (isAllX()) {
            str = "x";
            hasXZ = உண்மை;
        } இல்லையெனில் எனில் (isAllZ()) {
            str = "z";
            hasXZ = உண்மை;
        } இல்லையெனில் எனில் (isAnyX()) {
            str = "X";
            hasXZ = உண்மை;
        } இல்லையெனில் எனில் (isAnyZ()) {
            str = "Z";
            hasXZ = உண்மை;
        }
        எனில் (!hasXZ) {
            எனில் (issigned) {
                எனில் (width() > 64) {
                    str = toDecimalS();
                } இல்லையெனில் {
                    str = cvtToStr(toSQuad());
                }
            } இல்லையெனில் {
                எனில் (width() > 64) {
                    str = toDecimalU();
                } இல்லையெனில் {
                    str = cvtToStr(toUQuad());
                }
            }
        }
        மாறிலி தருக்க zeropad = fmtsize.length() > 0 && fmtsize[0] == '0' && !இடது;
        // fmtsize might have changed since we parsed the %fmtsize
        மாறிலி size_t fmtsizen = static_cast<size_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), இடது, str);
        திரும்பு str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    நிலை 'u': {  // Packed 2-state
        க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
            மாறிலி uint32_t v = m_data.இலக்கம்()[i].m_மதிப்பு;
            str += static_cast<எழுத்து>((v >> 0) & 0xff);
            str += static_cast<எழுத்து>((v >> 8) & 0xff);
            str += static_cast<எழுத்து>((v >> 16) & 0xff);
            str += static_cast<எழுத்து>((v >> 24) & 0xff);
        }
        திரும்பு str;
    }
    நிலை 'z': {  // Packed 4-state
        க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
            மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
            str += static_cast<எழுத்து>((v.m_மதிப்பு >> 0) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு >> 8) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு >> 16) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு >> 24) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு_X >> 0) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு_X >> 8) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு_X >> 16) & 0xff);
            str += static_cast<எழுத்து>((v.m_மதிப்பு_X >> 24) & 0xff);
        }
        திரும்பு str;
    }
    நிலை 'v': {  // Strength
        முழு bit = width() - 1;
        க்கு (; bit >= 0; bit--) {
            எனில் (bitIs0(bit)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            இல்லையெனில் எனில் (bitIs1(bit)) {
                str += "St1 ";
            } இல்லையெனில் எனில் (bitIsZ(bit)) {
                str += "StZ ";
            } இல்லையெனில் {
                str += "StX";
            }
        }
        திரும்பு str;
    }
    இயல்பு: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_எண்::emitC() மாறிலி VL_MT_STABLE {
    constexpr size_t bufsize = 1000;
    எழுத்து sbuf[bufsize];
    string result;
    எனில் (isNull()) {
        திரும்பு "VlNull{}";
    } இல்லையெனில் எனில் (isDouble()) {
        மாறிலி இரட்டை dnum = toDouble();
        எனில் (std::isinf(dnum)) {
            எனில் (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::infinity()";
        } இல்லையெனில் எனில் (std::isnan(dnum)) {
            எனில் (std::signbit(dnum)) result += '-';
            result += "std::numeric_limits<double>::quiet_NaN()";
        } இல்லையெனில் {
            மாறிலி எழுத்து* மாறிலி fmt = (static_cast<முழு>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (வெறுமை)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            திரும்பு sbuf;
        }
    } இல்லையெனில் எனில் (isString()) {
        மாறிலி string strg = toString();
        மாறிலி string quoted
            = V3OutFormatter::quoteNameControls(strg, V3OutFormatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        திரும்பு '"' + quoted + "\"s";
    } இல்லையெனில் எனில் (சொற்கள்() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        result = "::VlWide<" + std::to_string(சொற்கள்()) + ">{{";
        எனில் (சொற்கள்() > 4) result += '\n';
        க்கு (முழு n = 0; n < சொற்கள்(); ++n) {
            எனில் (n) result += ((n % 4) ? ", " : ",\n");
            (வெறுமை)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edataWord(n));
            result += sbuf;
        }
        எனில் (சொற்கள்() > 4) result += '\n';
        result += "}}";
    } இல்லையெனில் எனில் (சொற்கள்() == 2) {  // Quad
        மாறிலி uint64_t qnum = static_cast<uint64_t>(toUQuad());
        மாறிலி எழுத்து* மாறிலி fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (வெறுமை)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        திரும்பு sbuf;
    } இல்லையெனில் {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        மாறிலி uint32_t unum = toUInt();
        மாறிலி எழுத்து* மாறிலி fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (width() > 16) ? ("0x%08" PRIx32 "U")
                                : (width() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (வெறுமை)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        திரும்பு sbuf;
    }
    திரும்பு result;
}

string V3_எண்::toDecimalS() மாறிலி VL_MT_STABLE {
    // Correct number of zero bits/width matters
    எனில் (isNegative()) {
        V3_எண் lhsNoSign = *இது;
        lhsNoSign.opNegate(*இது);
        திரும்பு "-"s + lhsNoSign.toDecimalU();
    } இல்லையெனில் {
        திரும்பு toDecimalU();
    }
}

string V3_எண்::toDecimalU() மாறிலி VL_MT_STABLE {
    மாறிலி முழு maxdecwidth = (width() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_எண் bcd(இது, maxdecwidth + 4);
    V3_எண் tmp(இது, maxdecwidth + 4);
    V3_எண் tmp2(இது, maxdecwidth + 4);

    முழு from_bit = width() - 1;
    // Skip all leading zeros
    க்கு (; from_bit >= 0 && bitIs0(from_bit); --from_bit) {}
    // Double-dabble algorithm
    க்கு (; from_bit >= 0; --from_bit) {
        // Any digits >= 5 need an add 3 (via tmp)
        க்கு (முழு nibble_bit = 0; nibble_bit < maxdecwidth; nibble_bit += 4) {
            எனில் (bcd.bits_மதிப்பு(nibble_bit, 4) >= 5) {
                tmp2.setAllBits0();
                tmp2.setBit(nibble_bit, 1);  // Add 3, decomposed as two bits
                tmp2.setBit(nibble_bit + 1, 1);
                tmp.opAssign(bcd);
                bcd.opAdd(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.opAssign(bcd);
        bcd.opShiftL(tmp, V3_எண்(இது, 32, 1));
        // bcd[0] = this[from_bit]
        எனில் (bitIs1(from_bit)) bcd.setBit(0, 1);
    }

    string output;
    முழு lsb = (maxdecwidth - 1) & ~3;
    க்கு (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        எனில் (bcd.bits_மதிப்பு(lsb, 4)) நிறுத்து;
    }
    க்கு (; lsb >= 0; lsb -= 4) {
        output += static_cast<எழுத்து>('0' + bcd.bits_மதிப்பு(lsb, 4));  // 0..9
    }
    திரும்பு output;
}

string V3_எண்::toRealString() மாறிலி VL_MT_STABLE {
    மாறிலி இரட்டை மதிப்பு = toDouble();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream out;
    out.precision(15);
    out << மதிப்பு;
    string result = out.str();
    எனில் (std::strtod(result.c_str(), nullptr) != மதிப்பு) {
        out.str("");
        out.precision(17);
        out << மதிப்பு;
        result = out.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    எனில் (result.find_first_of(".en") == string::npos) result += ".0";
    திரும்பு result;
}

//======================================================================
// ACCESSORS - as numbers

தருக்க V3_எண்::fitsInUInt() மாறிலி VL_MT_SAFE {
    எனில் (isFourState()) திரும்பு பொய்;
    // We allow wide numbers that represent values <= 32 bits
    க்கு (முழு i = 1; i < சொற்கள்(); ++i) {
        எனில் (m_data.இலக்கம்()[i].m_மதிப்பு) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}
uint32_t V3_எண்::toUInt() மாறிலி VL_MT_SAFE {
    UASSERT(!isFourState(), "toUInt with 4-state " << *இது);
    // We allow wide numbers that represent values <= 32 bits
    எனில் (VL_UNCOVERABLE(!fitsInUInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *இது);
    }
    திரும்பு m_data.இலக்கம்()[0].m_மதிப்பு;
}

இரட்டை V3_எண்::toDouble() மாறிலி VL_MT_SAFE {
    UASSERT(isDouble() && width() == 64, "Real operation on wrong sized/non-real number");
    ஒன்றிப்பு {
        இரட்டை d;
        uint32_t u[2];
    } u;
    u.u[0] = m_data.இலக்கம்()[0].m_மதிப்பு;
    u.u[1] = m_data.இலக்கம்()[1].m_மதிப்பு;
    திரும்பு u.d;
}

int32_t V3_எண்::toSInt() மாறிலி VL_MT_SAFE {
    எனில் (isSigned()) {
        மாறிலி uint32_t v = toUInt();
        மாறிலி uint32_t signExtend = (-(v & (1UL << (width() - 1))));
        மாறிலி uint32_t extended = v | signExtend;
        திரும்பு static_cast<int32_t>(extended);
    } இல்லையெனில் {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        திரும்பு static_cast<int32_t>(toUInt());
    }
}

uint64_t V3_எண்::toUQuad() மாறிலி VL_MT_SAFE {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *இது);
    // We allow wide numbers that represent values <= 64 bits
    எனில் (isDouble()) திரும்பு static_cast<uint64_t>(toDouble());
    க்கு (முழு i = 2; i < சொற்கள்(); ++i) {
        எனில் (m_data.இலக்கம்()[i].m_மதிப்பு) {
            v3error("Value too wide for 64-bits expected in this context " << *இது);
            நிறுத்து;
        }
    }
    எனில் (width() <= 32) திரும்பு (static_cast<uint64_t>(toUInt()));
    திரும்பு ((static_cast<uint64_t>(m_data.இலக்கம்()[1].m_மதிப்பு) << 32ULL)
            | (static_cast<uint64_t>(m_data.இலக்கம்()[0].m_மதிப்பு)));
}

int64_t V3_எண்::toSQuad() மாறிலி {
    எனில் (isDouble()) திரும்பு static_cast<int64_t>(toDouble());
    மாறிலி uint64_t v = toUQuad();
    மாறிலி uint64_t signExtend = (-(v & (1ULL << (width() - 1))));
    மாறிலி uint64_t extended = v | signExtend;
    திரும்பு static_cast<int64_t>(extended);
}

string V3_எண்::toString() மாறிலி {
    UASSERT(!isFourState(), "toString with 4-state " << *இது);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    எனில் (isString()) திரும்பு m_data.str();
    முழு bit = width() - 1;
    போது ((bit % 8) != 7) ++bit;
    std::string str;
    க்கு (; bit >= 0; bit -= 8) {
        மாறிலி முழு v = bits_மதிப்பு(bit - 7, 8);
        எனில் (v) str += static_cast<எழுத்து>(v);
    }
    திரும்பு str;
}

uint32_t V3_எண்::edataWord(முழு eword) மாறிலி {
    UASSERT(!isFourState(), "edataWord with 4-state " << *இது);
    திரும்பு m_data.இலக்கம்()[eword].m_மதிப்பு;
}

uint8_t V3_எண்::dataByte(முழு byte) மாறிலி {
    திரும்பு (edataWord(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

தருக்க V3_எண்::isAllZ() மாறிலி VL_MT_SAFE {
    எனில் (isDouble() || isString()) திரும்பு பொய்;
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        மாறிலி uint32_t mask = i == சொற்கள்() - 1 ? hiWordMask() : ~0U;
        எனில் (((~v.m_மதிப்பு & v.m_மதிப்பு_X) & mask) != mask) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}
தருக்க V3_எண்::isAllX() மாறிலி VL_MT_SAFE {
    எனில் (isDouble() || isString()) திரும்பு பொய்;
    uint32_t mask = hiWordMask();
    க்கு (முழு i = சொற்கள்() - 1; i >= 0; --i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        எனில் ((v.m_மதிப்பு & v.m_மதிப்பு_X) ^ mask) திரும்பு பொய்;
        mask = ~0U;
    }
    திரும்பு உண்மை;
}
தருக்க V3_எண்::isEqZero() மாறிலி VL_MT_SAFE {
    எனில் (isString()) திரும்பு m_data.str().empty();
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        எனில் (v.m_மதிப்பு || v.m_மதிப்பு_X) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}
தருக்க V3_எண்::isNeqZero() மாறிலி {
    எனில் (isString()) திரும்பு !m_data.str().empty();
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        எனில் (v.m_மதிப்பு & ~v.m_மதிப்பு_X) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}
தருக்க V3_எண்::isBitsZero(முழு msb, முழு lsb) மாறிலி {
    க்கு (முழு i = lsb; i <= msb; ++i) {
        எனில் (VL_UNLIKELY(!bitIs0(i))) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}
தருக்க V3_எண்::isEqOne() மாறிலி {
    எனில் (m_data.இலக்கம்()[0].m_மதிப்பு != 1 || m_data.இலக்கம்()[0].m_மதிப்பு_X) திரும்பு பொய்;
    க்கு (முழு i = 1; i < சொற்கள்(); ++i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        எனில் (v.m_மதிப்பு || v.m_மதிப்பு_X) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}
தருக்க V3_எண்::isEqAllOnes(முழு optwidth) மாறிலி {
    // Correct number of zero bits/width matters
    எனில் (!optwidth) optwidth = width();
    க்கு (முழு bit = 0; bit < optwidth; ++bit) {
        எனில் (!bitIs1(bit)) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}
தருக்க V3_எண்::isFourState() மாறிலி VL_MT_SAFE {
    எனில் (isDouble() || isString()) திரும்பு பொய்;
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        எனில் (m_data.இலக்கம்()[i].m_மதிப்பு_X) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}
தருக்க V3_எண்::isAnyX() மாறிலி VL_MT_SAFE {
    எனில் (isDouble() || isString()) திரும்பு பொய்;
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        எனில் (v.m_மதிப்பு & v.m_மதிப்பு_X) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}
தருக்க V3_எண்::isAnyXZ() மாறிலி {
    எனில் (isDouble() || isString()) திரும்பு பொய்;
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        எனில் (m_data.இலக்கம்()[i].m_மதிப்பு_X) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}
தருக்க V3_எண்::isAnyZ() மாறிலி VL_MT_SAFE {
    எனில் (isDouble() || isString()) திரும்பு பொய்;
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
        எனில் (~v.m_மதிப்பு & v.m_மதிப்பு_X) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}
தருக்க V3_எண்::isLtXZ(மாறிலி V3_எண்& rhs) மாறிலி {
    // Include X/Z in comparisons for sort ordering
    எனில் (isString()) திரும்பு toString() < rhs.toString();
    எனில் (isDouble()) திரும்பு toDouble() < rhs.toDouble();
    க்கு (முழு bit = 0; bit < std::max(width(), rhs.width()); ++bit) {
        எனில் (bitIs1(bit) && rhs.bitIs0(bit)) திரும்பு உண்மை;
        எனில் (rhs.bitIs1(bit) && bitIs0(bit)) திரும்பு பொய்;
        எனில் (bitIsXZ(bit)) திரும்பு உண்மை;
        எனில் (rhs.bitIsXZ(bit)) திரும்பு பொய்;
    }
    திரும்பு பொய்;
}
முழு V3_எண்::மொத்தம்_X(முழு lsb, முழு nbits) மாறிலி VL_MT_SAFE {
    முழு மொத்தம் = 0;
    க்கு (முழு bitn = 0; bitn < nbits; ++bitn) {
        எனில் (lsb + bitn >= width()) திரும்பு மொத்தம்;
        எனில் (bitIsX(lsb + bitn)) ++மொத்தம்;
    }
    திரும்பு மொத்தம்;
}
முழு V3_எண்::மொத்தம்_Z(முழு lsb, முழு nbits) மாறிலி VL_MT_SAFE {
    முழு மொத்தம் = 0;
    க்கு (முழு bitn = 0; bitn < nbits; ++bitn) {
        எனில் (lsb + bitn >= width()) திரும்பு மொத்தம்;
        எனில் (bitIsZ(lsb + bitn)) ++மொத்தம்;
    }
    திரும்பு மொத்தம்;
}

முழு V3_எண்::widthToFit() மாறிலி {
    க்கு (முழு bit = width() - 1; bit > 0; bit--) {
        எனில் (!bitIs0(bit)) திரும்பு bit + 1;
    }
    திரும்பு 1;  // one bit even if number is == 0
}

uint32_t V3_எண்::மொத்தம்_Bits(மாறிலி V3_எண்& ctrl) மாறிலி {
    // Correct number of zero bits/width matters
    முழு n = 0;
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        தேர்வு (ctrl.bitIs(0)) {
        நிலை '0':
            எனில் (bitIs0(bit)) ++n;
            நிறுத்து;
        நிலை '1':
            எனில் (bitIs1(bit)) ++n;
            நிறுத்து;
        நிலை 'x':
            எனில் (bitIsX(bit)) ++n;
            நிறுத்து;
        நிலை 'z':
            எனில் (bitIsZ(bit)) ++n;
            நிறுத்து;
        }
    }
    திரும்பு n;
}

uint32_t V3_எண்::மொத்தம்_Bits(மாறிலி V3_எண்& ctrl1, மாறிலி V3_எண்& ctrl2,
                             மாறிலி V3_எண்& ctrl3) மாறிலி {
    // Correct number of zero bits/width matters
    முழு n = மொத்தம்_Bits(ctrl1);
    எனில் (ctrl2.bitIs(0) != ctrl1.bitIs(0)) n += மொத்தம்_Bits(ctrl2);
    எனில் ((ctrl3.bitIs(0) != ctrl1.bitIs(0)) && (ctrl3.bitIs(0) != ctrl2.bitIs(0))) {
        n += மொத்தம்_Bits(ctrl3);
    }
    திரும்பு n;
}

uint32_t V3_எண்::மொத்தம்_Ones() மாறிலி {
    முழு n = 0;
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (bitIs1(bit)) ++n;
    }
    திரும்பு n;
}

uint32_t V3_எண்::mostSetBitP1() மாறிலி {
    க்கு (முழு bit = width() - 1; bit >= 0; bit--) {
        எனில் (!bitIs0(bit)) திரும்பு bit + 1;
    }
    திரும்பு 0;
}

uint32_t V3_எண்::leastSetBitP1() மாறிலி {
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (!bitIs0(bit)) திரும்பு bit + 1;
    }
    திரும்பு 0;
}

//======================================================================

V3_எண்& V3_எண்::opBitsNonXZ(மாறிலி V3_எண்& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIs0(bit) || lhs.bitIs1(bit)) setBit(bit, 1);
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opBitsOne(மாறிலி V3_எண்& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIs1(bit)) setBit(bit, 1);
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opBitsXZ(மாறிலி V3_எண்& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIsXZ(bit)) setBit(bit, 1);
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opBitsZ(மாறிலி V3_எண்& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIsZ(bit)) setBit(bit, 1);
    }
    திரும்பு *இது;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_எண்& V3_எண்::opRedOr(மாறிலி V3_எண்& lhs) {
    // op i, 1 bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எழுத்து outc = 0;
    க்கு (முழு bit = 0; bit < lhs.width(); ++bit) {
        எனில் (lhs.bitIs1(bit)) {
            திரும்பு setSingleBits(1);
        } இல்லையெனில் எனில் (lhs.bitIs0(bit)) {
        } இல்லையெனில் {
            outc = 'x';
        }
    }
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opRedAnd(மாறிலி V3_எண்& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எழுத்து outc = 1;
    க்கு (முழு bit = 0; bit < lhs.width(); ++bit) {
        எனில் (lhs.bitIs0(bit)) {
            திரும்பு setSingleBits(0);
        } இல்லையெனில் எனில் (lhs.bitIs1(bit)) {
        } இல்லையெனில் {
            outc = 'x';
        }
    }
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opRedXor(மாறிலி V3_எண்& lhs) {
    // op i, 1 bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எழுத்து outc = 0;
    க்கு (முழு bit = 0; bit < lhs.width(); ++bit) {
        எனில் (lhs.bitIs1(bit)) {
            எனில் (outc == 1) {
                outc = 0;
            } இல்லையெனில் எனில் (outc == 0) {
                outc = 1;
            }
        } இல்லையெனில் எனில் (lhs.bitIs0(bit)) {
        } இல்லையெனில் {
            outc = 'x';
        }
    }
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::op_மொத்தம்_Bits(மாறிலி V3_எண்& கோவை, மாறிலி V3_எண்& ctrl1, மாறிலி V3_எண்& ctrl2,
                                மாறிலி V3_எண்& ctrl3) {
    // Correct number of zero bits/width matters
    இலக்கம்_ASSERT_OP_ARGS4(கோவை, ctrl1, ctrl2, ctrl3);
    இலக்கம்_ASSERT_LOGIC_ARGS4(கோவை, ctrl1, ctrl2, ctrl3);
    setZero();
    m_data.இலக்கம்()[0].m_மதிப்பு = கோவை.மொத்தம்_Bits(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::op_மொத்தம்_Ones(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எனில் (lhs.isFourState()) திரும்பு setAllBitsX();
    setZero();
    m_data.இலக்கம்()[0].m_மதிப்பு = lhs.மொத்தம்_Ones();
    opCleanThis();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opIsUnknown(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    திரும்பு setSingleBits(lhs.isAnyXZ());
}
V3_எண்& V3_எண்::opOneHot(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    எனில் (lhs.isFourState()) திரும்பு setAllBitsX();
    திரும்பு setSingleBits(lhs.மொத்தம்_Ones() == 1);
}
V3_எண்& V3_எண்::opOneHot0(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    எனில் (lhs.isFourState()) திரும்பு setAllBitsX();
    திரும்பு setSingleBits(lhs.மொத்தம்_Ones() <= 1);
}
V3_எண்& V3_எண்::opCLog2(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எனில் (lhs.isFourState()) திரும்பு setAllBitsX();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    மாறிலி முழு adjust = (lhs.மொத்தம்_Ones() == 1) ? 0 : 1;
    க்கு (முழு bit = lhs.width() - 1; bit >= 0; bit--) {
        எனில் (lhs.bitIs1(bit)) {
            setLong(bit + adjust);
            திரும்பு *இது;
        }
    }
    setZero();
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opMostSetBitP1(மாறிலி V3_எண்& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எனில் (lhs.isFourState()) திரும்பு setAllBitsX();
    க்கு (முழு bit = lhs.width() - 1; bit >= 0; bit--) {
        எனில் (lhs.bitIs1(bit)) {
            setLong(bit + 1);
            திரும்பு *இது;
        }
    }
    setZero();
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opLogNot(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    எழுத்து outc = 1;
    க்கு (முழு bit = 0; bit < lhs.width(); ++bit) {
        எனில் (lhs.bitIs1(bit)) {
            outc = 0;
            செல் last;
        } இல்லையெனில் எனில் (lhs.bitIs0(bit)) {
        } இல்லையெனில் {
            outc = 'x';
        }
    }
last:
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opNot(மாறிலி V3_எண்& lhs) {
    // Correct number of zero bits/width matters
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIs0(bit)) {
            setBit(bit, 1);
        } இல்லையெனில் எனில் (lhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opAnd(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIs1(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } இல்லையெனில் எனில் (lhs.bitIs0(bit) || rhs.bitIs0(bit)) {  // 0
        } இல்லையெனில் {
            setBit(bit, 'x');
        }
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opOr(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIs1(bit) || rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } இல்லையெனில் எனில் (lhs.bitIs0(bit) && rhs.bitIs0(bit)) {
            // 0
        } இல்லையெனில் {
            setBit(bit, 'x');
        }
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opXor(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            setBit(bit, 1);
        } இல்லையெனில் எனில் (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            setBit(bit, 1);
        } இல்லையெனில் எனில் (lhs.bitIsXZ(bit) || rhs.bitIsXZ(bit)) {
            setBit(bit, 'x');
        }
        // else zero
    }
    திரும்பு *இது;
}

வெறுமை V3_எண்::fillBits(முழு destLsb, முழு width, எழுத்து மதிப்பு) {
    UASSERT(destLsb >= 0 && width >= 0, "Negative bit range");
    எனில் (!width) திரும்பு;
    UASSERT(width <= இது->width() - destLsb, "Destination bit range exceeds number width");

    மாறிலி uint32_t மதிப்பு_Word = மதிப்பு == '1' || மதிப்பு == 'x' || மதிப்பு == 1 || மதிப்பு == 3 ? ~0U : 0;
    மாறிலி uint32_t xWord = மதிப்பு == 'z' || மதிப்பு == 'x' || மதிப்பு == 2 || மதிப்பு == 3 ? ~0U : 0;
    மாறிலி முழு firstWord = VL_BITWORD_E(destLsb);
    மாறிலி முழு lastWord = VL_BITWORD_E(destLsb + width - 1);
    க்கு (முழு word = firstWord; word <= lastWord; ++word) {
        மாறிலி முழு wordLsb = word == firstWord ? VL_BITBIT_E(destLsb) : 0;
        மாறிலி முழு wordMsb = word == lastWord ? VL_BITBIT_E(destLsb + width - 1) : VL_SIZEBITS_E;
        மாறிலி uint32_t mask = VL_MASK_E(wordMsb - wordLsb + 1) << wordLsb;
        மதிப்பு_And_X& dest = m_data.இலக்கம்()[word];
        dest.m_மதிப்பு = (dest.m_மதிப்பு & ~mask) | (மதிப்பு_Word & mask);
        dest.m_மதிப்பு_X = (dest.m_மதிப்பு_X & ~mask) | (xWord & mask);
    }
}

வெறுமை V3_எண்::copyBits(முழு destLsb, மாறிலி V3_எண்& source, முழு sourceLsb, முழு width) {
    UASSERT(destLsb >= 0 && sourceLsb >= 0 && width >= 0, "Negative bit range");
    எனில் (!width) திரும்பு;
    UASSERT(width <= இது->width() - destLsb, "Destination bit range exceeds number width");
    UASSERT(width <= source.width() - sourceLsb, "Source bit range exceeds number width");

    மாறிலி முழு firstWord = VL_BITWORD_E(destLsb);
    மாறிலி முழு lastWord = VL_BITWORD_E(destLsb + width - 1);
    க்கு (முழு destWord = firstWord; destWord <= lastWord; ++destWord) {
        மாறிலி முழு destBit = std::max(destLsb, destWord * VL_EDATASIZE);
        மாறிலி முழு destOffset = VL_BITBIT_E(destBit);
        மாறிலி முழு sourceBit = sourceLsb + destBit - destLsb;
        மாறிலி முழு sourceWord = VL_BITWORD_E(sourceBit);
        மாறிலி முழு sourceOffset = VL_BITBIT_E(sourceBit);
        மாறிலி முழு copyWidth = std::min(width - (destBit - destLsb), VL_EDATASIZE - destOffset);
        மாறிலி uint32_t mask = VL_MASK_E(copyWidth) << destOffset;

        மாறிலி மதிப்பு_And_X& sourceLo = source.m_data.இலக்கம்()[sourceWord];
        uint32_t மதிப்பு = sourceLo.m_மதிப்பு >> sourceOffset;
        uint32_t மதிப்பு_X = sourceLo.m_மதிப்பு_X >> sourceOffset;
        எனில் (sourceOffset && copyWidth > VL_EDATASIZE - sourceOffset) {
            மாறிலி மதிப்பு_And_X& sourceHi = source.m_data.இலக்கம்()[sourceWord + 1];
            மதிப்பு |= sourceHi.m_மதிப்பு << (VL_EDATASIZE - sourceOffset);
            மதிப்பு_X |= sourceHi.m_மதிப்பு_X << (VL_EDATASIZE - sourceOffset);
        }

        மதிப்பு_And_X& dest = m_data.இலக்கம்()[destWord];
        dest.m_மதிப்பு = (dest.m_மதிப்பு & ~mask) | ((மதிப்பு << destOffset) & mask);
        dest.m_மதிப்பு_X = (dest.m_மதிப்பு_X & ~mask) | ((மதிப்பு_X << destOffset) & mask);
    }
}

V3_எண்& V3_எண்::opConcat(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Correct number of zero bits/width matters
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    எனில் (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copyBits(0, rhs, 0, rhs.width());
    copyBits(rhs.width(), lhs, 0, lhs.width());
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opLenN(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    setQuad(lhs.toString().length());
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opRepl(மாறிலி V3_எண்& lhs,
                           மாறிலி V3_எண்& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    திரும்பு opRepl(lhs, rhs.toUInt());
}

V3_எண்& V3_எண்::opRepl(மாறிலி V3_எண்& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எனில் (v3Global.opt.replicationLimit()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replicationLimit())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replicationLimit() << " is suspect: " << rhsval);
    }
    setZero();
    க்கு (unsigned times = 0; times < rhsval; ++times) {
        மாறிலி uint64_t destLsb = uint64_t{times} * lhs.width();
        எனில் (destLsb >= static_cast<uint32_t>(width())) நிறுத்து;
        copyBits(static_cast<முழு>(destLsb), lhs, 0,
                 std::min(lhs.width(), width() - static_cast<முழு>(destLsb)));
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opStreamL(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // See also error in V3Width
    எனில் (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    மாறிலி முழு ssize = std::min(rhs.toUInt(), static_cast<unsigned>(lhs.width()));
    க்கு (முழு istart = 0; istart < lhs.width(); istart += ssize) {
        மாறிலி முழு ostart = std::max(0, lhs.width() - ssize - istart);
        copyBits(ostart, lhs, istart, std::min(ssize, lhs.width() - istart));
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opLogAnd(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எழுத்து loutc = 0;
    எழுத்து routc = 0;
    க்கு (முழு bit = 0; bit < lhs.width(); ++bit) {
        எனில் (lhs.bitIs1(bit)) {
            loutc = 1;
            நிறுத்து;
        }
        எனில் (lhs.bitIsXZ(bit) && loutc == 0) loutc = 'x';
    }
    க்கு (முழு bit = 0; bit < rhs.width(); ++bit) {
        எனில் (rhs.bitIs1(bit)) {
            routc = 1;
            நிறுத்து;
        }
        எனில் (rhs.bitIsXZ(bit) && routc == 0) routc = 'x';
    }
    எழுத்து outc = 'x';
    எனில் (routc == 1 && loutc == 1) outc = 1;
    எனில் (routc == 0 || loutc == 0) outc = 0;
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opLogOr(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எழுத்து outc = 0;
    க்கு (முழு bit = 0; bit < lhs.width(); ++bit) {
        எனில் (lhs.bitIs1(bit)) {
            outc = 1;
            செல் last;
        }
        எனில் (lhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
    க்கு (முழு bit = 0; bit < rhs.width(); ++bit) {
        எனில் (rhs.bitIs1(bit)) {
            outc = 1;
            செல் last;
        }
        எனில் (rhs.bitIsXZ(bit) && outc == 0) outc = 'x';
    }
last:
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opLogIf(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    V3_எண் lnot = lhs;
    lnot.opLogNot(lhs);
    திரும்பு opLogOr(lnot, rhs);
}

V3_எண்& V3_எண்::opLogEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    V3_எண் ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_எண் ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    திரும்பு opLogAnd(ifa, ifb);
}

V3_எண்& V3_எண்::opAtoN(மாறிலி V3_எண்& lhs, முழு base) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    UASSERT(base == AstAtoN::ATOREAL || base == 2 || base == 8 || base == 10 || base == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.toString();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.end(), '_'), str.end());

    எனில் (base == AstAtoN::ATOREAL) திரும்பு setDouble(std::atof(str.c_str()));

    errno = 0;
    auto v = std::strtol(str.c_str(), nullptr, base);
    எனில் (errno != 0) v = 0;
    திரும்பு setLongS(static_cast<int32_t>(v));
}

V3_எண்& V3_எண்::opPutcN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, மாறிலி V3_எண்& ths) {
    இலக்கம்_ASSERT_OP_ARGS3(lhs, rhs, ths);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.toString();
    மாறிலி int32_t i = rhs.toSInt();
    மாறிலி int32_t c = ths.toSInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    எனில் (0 <= i && static_cast<uint32_t>(i) < lstring.length() && c != 0) lstring[i] = c;
    திரும்பு setString(lstring);
}

V3_எண்& V3_எண்::opGetcN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    மாறிலி string lstring = lhs.toString();
    மாறிலி int32_t i = rhs.toSInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    எனில் (0 <= i && static_cast<uint32_t>(i) < lstring.length()) v = lstring[i];
    திரும்பு setLong(v);
}

V3_எண்& V3_எண்::opSubstrN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, மாறிலி V3_எண்& ths) {
    இலக்கம்_ASSERT_OP_ARGS3(lhs, rhs, ths);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    மாறிலி string lstring = lhs.toString();
    மாறிலி int32_t i = rhs.toSInt();
    மாறிலி int32_t j = ths.toSInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    எனில் (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.length()) திரும்பு setString("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    திரும்பு setString(lstring.substr(i, j - i + 1));
}

V3_எண்& V3_எண்::opCompareNN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, தருக்க ignoreCase) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    முழு result;
    மாறிலி string lstring = lhs.toString();
    மாறிலி string rstring = rhs.toString();
    எனில் (ignoreCase) {
        result = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } இல்லையெனில் {
        result = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    திரும்பு setLongS(result);
}

V3_எண்& V3_எண்::opEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    எனில் (lhs.isString()) திரும்பு opEqN(lhs, rhs);
    எனில் (lhs.isDouble()) திரும்பு opEqD(lhs, rhs);
    எழுத்து outc = 1;
    க்கு (முழு bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        எனில் (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 0;
            செல் last;
        }
        எனில் (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 0;
            செல் last;
        }
        எனில் (lhs.bitIsXZ(bit)) outc = 'x';
        எனில் (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opNeq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    எனில் (lhs.isString()) திரும்பு opNeqN(lhs, rhs);
    எனில் (lhs.isDouble()) திரும்பு opNeqD(lhs, rhs);
    எழுத்து outc = 0;
    க்கு (முழு bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        எனில் (lhs.bitIs1(bit) && rhs.bitIs0(bit)) {
            outc = 1;
            செல் last;
        }
        எனில் (lhs.bitIs0(bit) && rhs.bitIs1(bit)) {
            outc = 1;
            செல் last;
        }
        எனில் (lhs.bitIsXZ(bit)) outc = 'x';
        எனில் (rhs.bitIsXZ(bit)) outc = 'x';
    }
last:
    திரும்பு setSingleBits(outc);
}

V3Hash V3_எண்::toHash() மாறிலி {
    // Shouldmatch isCaseEq
    எனில் (isString()) திரும்பு V3Hash{toString()};
    எனில் (isDouble()) திரும்பு V3Hash{toDouble()};
    V3Hash hash{width()};
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) hash += m_data.இலக்கம்()[i].m_மதிப்பு;
    திரும்பு hash;
}

தருக்க V3_எண்::isCaseEq(மாறிலி V3_எண்& rhs) மாறிலி {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    எனில் (isString()) திரும்பு toString() == rhs.toString();
    எனில் (isDouble()) திரும்பு toDouble() == rhs.toDouble();
    எனில் (width() != rhs.width()) திரும்பு பொய்;
    க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
        எனில் (!(m_data.இலக்கம்()[i] == rhs.m_data.இலக்கம்()[i])) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}

V3_எண்& V3_எண்::opCaseEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_எண்& V3_எண்::opCaseNeq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    எழுத்து outc = 0;
    எனில் (lhs.isString()) {
        திரும்பு opNeqN(lhs, rhs);
    } இல்லையெனில் எனில் (lhs.isDouble()) {
        திரும்பு opNeqD(lhs, rhs);
    }
    க்கு (முழு bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        எனில் (lhs.bitIs(bit) != rhs.bitIs(bit)) {
            outc = 1;
            செல் last;
        }
    }
last:
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opWildEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எழுத்து outc = 1;
    க்கு (முழு bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        எனில் (!rhs.bitIsXZ(bit)) {
            எனில் (lhs.bitIsXZ(bit)) {
                outc = 'x';
                செல் last;
            }
            எனில் (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 0;
                செல் last;
            }
        }
    }
last:
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opWildNeq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எழுத்து outc = 0;
    க்கு (முழு bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        எனில் (!rhs.bitIsXZ(bit)) {
            எனில் (lhs.bitIsXZ(bit)) {
                outc = 'x';
                செல் last;
            }
            எனில் (lhs.bitIs(bit) != rhs.bitIs(bit)) {
                outc = 1;
                செல் last;
            }
        }
    }
last:
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opGt(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எழுத்து outc = 0;
    க்கு (முழு bit = 0; bit < std::max(lhs.width(), rhs.width()); ++bit) {
        எனில் (lhs.bitIs1(bit) && rhs.bitIs0(bit)) outc = 1;
        எனில் (rhs.bitIs1(bit) && lhs.bitIs0(bit)) outc = 0;
        எனில் (lhs.bitIsXZ(bit)) outc = 'x';
        எனில் (rhs.bitIsXZ(bit)) outc = 'x';
    }
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opGtS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எழுத்து outc = 0;
    {
        மாறிலி முழு mbit = std::max(lhs.width() - 1, rhs.width() - 1);
        எனில் (lhs.bitIsXZ(mbit)) {
            outc = 'x';
        } இல்லையெனில் எனில் (rhs.bitIsXZ(mbit)) {
            outc = 'x';
        } இல்லையெனில் எனில் (lhs.bitIs0(mbit) && rhs.bitIs1Extend(mbit)) {
            outc = 1;  // + > -
        } இல்லையெனில் எனில் (lhs.bitIs1Extend(mbit) && rhs.bitIs0(mbit)) {
            outc = 0;  // - !> +
        } இல்லையெனில் {
            // both positive or negative, normal >
            க்கு (முழு bit = 0; bit < std::max(lhs.width() - 1, rhs.width() - 1); ++bit) {
                எனில் (lhs.bitIs1Extend(bit) && rhs.bitIs0(bit)) outc = 1;
                எனில் (rhs.bitIs1Extend(bit) && lhs.bitIs0(bit)) outc = 0;
                எனில் (lhs.bitIsXZ(bit)) outc = 'x';
                எனில் (rhs.bitIsXZ(bit)) outc = 'x';
            }
        }
    }
    திரும்பு setSingleBits(outc);
}

V3_எண்& V3_எண்::opGte(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    V3_எண்& eq = opEq(lhs, rhs);
    எனில் (eq.isNeqZero()) திரும்பு eq;  // Return true
    திரும்பு opGt(lhs, rhs);
}

V3_எண்& V3_எண்::opGteS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    V3_எண்& eq = opEq(lhs, rhs);
    எனில் (eq.isNeqZero()) திரும்பு eq;  // Return true
    திரும்பு opGtS(lhs, rhs);
}

V3_எண்& V3_எண்::opLt(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { திரும்பு opGt(rhs, lhs); }
V3_எண்& V3_எண்::opLte(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { திரும்பு opGte(rhs, lhs); }
V3_எண்& V3_எண்::opLtS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { திரும்பு opGtS(rhs, lhs); }
V3_எண்& V3_எண்::opLteS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) { திரும்பு opGteS(rhs, lhs); }

V3_எண்& V3_எண்::opShiftR(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (rhs.isFourState()) திரும்பு setAllBitsX();
    setZero();
    எனில் (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) திரும்பு *இது;
    மாறிலி uint32_t rhsval = rhs.toUInt();
    எனில் (rhsval < static_cast<uint32_t>(lhs.width())) {
        copyBits(0, lhs, static_cast<முழு>(rhsval),
                 std::min(width(), lhs.width() - static_cast<முழு>(rhsval)));
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opShiftRS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (rhs.isFourState()) திரும்பு setAllBitsX();
    setZero();
    மாறிலி தருக்க overflow = rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32);
    எனில் (!overflow) {
        மாறிலி uint32_t rhsval = rhs.toUInt();
        எனில் (rhsval < lbits) {
            மாறிலி முழு copyWidth = std::min(width(), static_cast<முழு>(lbits - rhsval));
            copyBits(0, lhs, static_cast<முழு>(rhsval), copyWidth);
            fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
            திரும்பு *இது;
        }
    }
    fillBits(0, width(), lhs.bitIs(lbits - 1));  // '0/'1/'x/'z
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opShiftL(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (rhs.isFourState()) திரும்பு setAllBitsX();
    setZero();
    எனில் (rhs.width() > 32 && !rhs.isBitsZero(rhs.width() - 1, 32)) திரும்பு *இது;
    மாறிலி uint32_t rhsval = rhs.toUInt();
    எனில் (rhsval < static_cast<uint32_t>(width())) {
        copyBits(static_cast<முழு>(rhsval), lhs, 0,
                 std::min(width() - static_cast<முழு>(rhsval), lhs.width()));
    }
    திரும்பு *இது;
}

//======================================================================
// Ops - Arithmetic

V3_எண்& V3_எண்::opNegate(மாறிலி V3_எண்& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    எனில் (lhs.isFourState()) திரும்பு setAllBitsX();
    V3_எண் notlhs(&lhs, width());
    notlhs.opNot(lhs);
    மாறிலி V3_எண் one(&lhs, width(), 1);
    opAdd(notlhs, one);
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opAdd(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    setZero();
    // Addem
    uint64_t carry = 0;
    க்கு (முழு word = 0; word < சொற்கள்(); ++word) {
        மாறிலி uint64_t lwordval = lhs.m_data.இலக்கம்()[word].m_மதிப்பு;
        மாறிலி uint64_t rwordval = rhs.m_data.இலக்கம்()[word].m_மதிப்பு;
        மாறிலி uint64_t sum = lwordval + rwordval + carry;
        m_data.இலக்கம்()[word].m_மதிப்பு = sum & 0xffffffffULL;
        carry = sum > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opSub(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    V3_எண் negrhs(&rhs, width());
    negrhs.opNegate(rhs);
    திரும்பு opAdd(lhs, negrhs);
}
V3_எண்& V3_எண்::opMul(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    setZero();
    எனில் (width() <= 64) {
        setQuad(lhs.toUQuad() * rhs.toUQuad());
        opCleanThis();  // Mult produces extra bits in result
    } இல்லையெனில் {
        க்கு (முழு lword = 0; lword < lhs.சொற்கள்(); ++lword) {
            மாறிலி uint64_t lwordval = static_cast<uint64_t>(lhs.m_data.இலக்கம்()[lword].m_மதிப்பு);
            எனில் (lwordval == 0) தொடர்;
            க்கு (முழு rword = 0; rword < rhs.சொற்கள்(); ++rword) {
                மாறிலி uint64_t rwordval = static_cast<uint64_t>(rhs.m_data.இலக்கம்()[rword].m_மதிப்பு);
                எனில் (rwordval == 0) தொடர்;
                uint64_t mul = lwordval * rwordval;
                க்கு (முழு qword = lword + rword; qword < சொற்கள்(); ++qword) {
                    mul += static_cast<uint64_t>(m_data.இலக்கம்()[qword].m_மதிப்பு);
                    m_data.இலக்கம்()[qword].m_மதிப்பு = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    எனில் (mul == 0) நிறுத்து;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opMulS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    V3_எண் lhsNoSign = lhs;
    எனில் (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_எண் rhsNoSign = rhs;
    எனில் (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    மாறிலி V3_எண் qNoSign = opMul(lhsNoSign, rhsNoSign);
    எனில் ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } இல்லையெனில் {
        opAssign(qNoSign);
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opDiv(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    எனில் (rhs.isEqZero()) திரும்பு setAllBitsXRemoved();
    எனில் (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() / rhs.toUQuad());
        திரும்பு *இது;
    } இல்லையெனில் {
        // Wide division
        திரும்பு opModDivGuts(lhs, rhs, பொய்);
    }
}
V3_எண்& V3_எண்::opDivS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    எனில் (rhs.isEqZero()) திரும்பு setAllBitsXRemoved();
    V3_எண் lhsNoSign = lhs;
    எனில் (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_எண் rhsNoSign = rhs;
    எனில் (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    மாறிலி V3_எண் qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    எனில் ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } இல்லையெனில் {
        opAssign(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *இது);
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opModDiv(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    எனில் (rhs.isEqZero()) திரும்பு setAllBitsXRemoved();
    எனில் (lhs.width() <= 64) {
        setQuad(lhs.toUQuad() % rhs.toUQuad());
        திரும்பு *இது;
    } இல்லையெனில் {
        // Wide modulus
        திரும்பு opModDivGuts(lhs, rhs, உண்மை);
    }
}
V3_எண்& V3_எண்::opModDivS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    எனில் (rhs.isEqZero()) திரும்பு setAllBitsXRemoved();
    V3_எண் lhsNoSign = lhs;
    எனில் (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_எண் rhsNoSign = rhs;
    எனில் (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    மாறிலி V3_எண் qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    எனில் (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } இல்லையெனில் {
        opAssign(qNoSign);
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opModDivGuts(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, தருக்க is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    setZero();
    // Find MSB and check for zero.
    மாறிலி முழு l_சொற்கள் = lhs.சொற்கள்();
    மாறிலி முழு umsbp1 = lhs.mostSetBitP1();  // dividend
    மாறிலி முழு vmsbp1 = rhs.mostSetBitP1();  //வகுஎண்
    எனில் (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *இது);
        திரும்பு *இது;
    }

    மாறிலி முழு uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    மாறிலி முழு vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    எனில் (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        க்கு (முழு j = uw - 1; j >= 0; j--) {
            மாறிலி uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_data.இலக்கம்()[j].m_மதிப்பு));
            m_data.இலக்கம்()[j].m_மதிப்பு = unw64 / static_cast<uint64_t>(rhs.m_data.இலக்கம்()[0].m_மதிப்பு);
            k = unw64
                - (static_cast<uint64_t>(m_data.இலக்கம்()[j].m_மதிப்பு)
                   * static_cast<uint64_t>(rhs.m_data.இலக்கம்()[0].m_மதிப்பு));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *இது << " rem=0x" << std::hex
                                   << k << std::dec);
        எனில் (is_modulus) {
            setZero();
            m_data.இலக்கம்()[0].m_மதிப்பு = k;
        }
        opCleanThis();
        திரும்பு *இது;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_சொற்கள் + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_சொற்கள் + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    க்கு (முழு i = 0; i < l_சொற்கள்; ++i) m_data.இலக்கம்()[i].m_மதிப்பு = 0;
    க்கு (முழு i = 0; i < l_சொற்கள் + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    மாறிலி முழு s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    மாறிலி uint32_t shift_mask = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    க்கு (முழு i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_data.இலக்கம்()[i].m_மதிப்பு << s)
                | (shift_mask & (rhs.m_data.இலக்கம்()[i - 1].m_மதிப்பு >> (32 - s)));
    }
    vn[0] = rhs.m_data.இலக்கம்()[0].m_மதிப்பு << s;

    // Copy and shift dividend by same amount; may set new upper word
    எனில் (s) {
        un[uw] = lhs.m_data.இலக்கம்()[uw - 1].m_மதிப்பு >> (32 - s);
    } இல்லையெனில் {
        un[uw] = 0;
    }
    க்கு (முழு i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_data.இலக்கம்()[i].m_மதிப்பு << s)
                | (shift_mask & (lhs.m_data.இலக்கம்()[i - 1].m_மதிப்பு >> (32 - s)));
    }
    un[0] = lhs.m_data.இலக்கம்()[0].m_மதிப்பு << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    க்கு (முழு j = uw - vw; j >= 0; j--) {
        // Estimate
        மாறிலி uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        எனில் (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            எனில் (rhat < 0x100000000ULL) செல் again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        க்கு (முழு i = 0; i < vw; ++i) {
            மாறிலி uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  // Subtract
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_data.இலக்கம்()[j].m_மதிப்பு = qhat;  // Save quotient digit

        எனில் (t < 0) {
            // Over subtracted; correct by adding back
            m_data.இலக்கம்()[j].m_மதிப்பு--;
            k = 0;
            க்கு (முழு i = 0; i < vw; ++i) {
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

    எனில் (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        க்கு (முழு i = 0; i < vw; ++i) {
            m_data.இலக்கம்()[i].m_மதிப்பு = (un[i] >> s) | (shift_mask & (un[i + 1] << (32 - s)));
        }
        க்கு (முழு i = vw; i < l_சொற்கள்; ++i) m_data.இலக்கம்()[i].m_மதிப்பு = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *இது);
        திரும்பு *இது;
    } இல்லையெனில் {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *இது);
        திரும்பு *இது;
    }
}

V3_எண்& V3_எண்::opPow(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, தருக்க lsign, தருக்க rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(lhs, rhs);
    எனில் (lhs.isFourState() || rhs.isFourState()) திரும்பு setAllBitsX();
    எனில் (rhs.isEqZero()) திரும்பு setValue1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    எனில் (rsign && rhs.isNegative()) {
        எனில் (lhs.isEqZero()) {
            திரும்பு setAllBitsXRemoved();
        } இல்லையெனில் எனில் (lhs.isEqOne()) {
            திரும்பு setValue1();
        } இல்லையெனில் எனில் (lsign && lhs.isEqAllOnes()) {
            எனில் (rhs.bitIs1(0)) {
                திரும்பு setAllBits1();  // -1^odd=-1
            } இல்லையெனில் {
                திரும்பு setValue1();  // -1^even=1
            }
        }
        திரும்பு setZero();
    }
    எனில் (lhs.isEqZero()) திரும்பு setZero();
    setZero();
    m_data.இலக்கம்()[0].m_மதிப்பு = 1;
    V3_எண் power(&lhs, width());
    power.opAssign(lhs);
    க்கு (முழு bit = 0; bit < rhs.width(); ++bit) {
        எனில் (bit > 0) {  // power = power*power
            V3_எண் lastPower(&lhs, width());
            lastPower.opAssign(power);
            power.opMul(lastPower, lastPower);
        }
        எனில் (rhs.bitIs1(bit)) {  // out *= power
            V3_எண் lastOut(&lhs, width());
            lastOut.opAssign(*இது);
            opMul(lastOut, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opPowSU(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Correct number of zero bits/width matters
    திரும்பு opPow(lhs, rhs, உண்மை, பொய்);
}
V3_எண்& V3_எண்::opPowSS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Correct number of zero bits/width matters
    திரும்பு opPow(lhs, rhs, உண்மை, உண்மை);
}
V3_எண்& V3_எண்::opPowUS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // Correct number of zero bits/width matters
    திரும்பு opPow(lhs, rhs, பொய், உண்மை);
}

V3_எண்& V3_எண்::opBufIf1(மாறிலி V3_எண்& ens, மாறிலி V3_எண்& if1s) {
    இலக்கம்_ASSERT_OP_ARGS2(ens, if1s);
    இலக்கம்_ASSERT_LOGIC_ARGS2(ens, if1s);
    setZero();
    க்கு (முழு bit = 0; bit < width(); ++bit) {
        எனில் (ens.bitIs1(bit)) {
            setBit(bit, if1s.bitIs(bit));
        } இல்லையெனில் {
            setBit(bit, 'z');
        }
    }
    திரும்பு *இது;
}

// Sets all bits in range to the given value
V3_எண்& V3_எண்::opSetRange(uint32_t lsb, uint32_t width, எழுத்து bit_மதிப்பு) {
    எனில் (lsb < static_cast<uint32_t>(இது->width())) {
        fillBits(static_cast<முழு>(lsb),
                 static_cast<முழு>(std::min(width, static_cast<uint32_t>(இது->width()) - lsb)),
                 bit_மதிப்பு);
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opAssign(மாறிலி V3_எண்& lhs) { திரும்பு opAssignNonXZ(lhs, பொய்); }
V3_எண்& V3_எண்::opAssignNonXZ(மாறிலி V3_எண்& lhs, தருக்க ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    எனில் (இது != &lhs) {
        எனில் (VL_UNLIKELY(lhs.isNull())) {
            m_data.m_isNull = உண்மை;
        } இல்லையெனில் எனில் (isString()) {
            எனில் (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_data.str() = lhs.toString();
            } இல்லையெனில் {
                m_data.str() = lhs.m_data.str();
            }
        } இல்லையெனில் எனில் (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            setZero();
        } இல்லையெனில் எனில் (lhs.isDouble()) {
            setDouble(lhs.toDouble());
        } இல்லையெனில் எனில் (!ignoreXZ) {
            setZero();
            copyBits(0, lhs, 0, std::min(width(), lhs.width()));
        } இல்லையெனில் {
            க்கு (முழு word = 0; word < சொற்கள்(); ++word) {
                மாறிலி uint32_t மதிப்பு = word < lhs.சொற்கள்() ? lhs.m_data.இலக்கம்()[word].m_மதிப்பு
                                                                & ~lhs.m_data.இலக்கம்()[word].m_மதிப்பு_X
                                                          : 0;
                m_data.இலக்கம்()[word] = {மதிப்பு, 0};
            }
            opCleanThis();
        }
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opNToI(மாறிலி V3_எண்& lhs) {
    // String to packed number
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    setZero();
    மாறிலி string& str = lhs.toString();
    க்கு (size_t n = 0; n < str.length(); ++n) {
        மாறிலி எழுத்து c = str[str.length() - 1 - n];
        க்கு (size_t cbit = 0; cbit < 8; ++cbit) {
            மாறிலி std::bitset<8> sbit{static_cast<unsigned long long>(c)};
            setBit(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opExtendS(மாறிலி V3_எண்& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    மாறிலி முழு copyWidth = std::min(width(), static_cast<முழு>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opExtendXZ(மாறிலி V3_எண்& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    மாறிலி முழு copyWidth = std::min(width(), static_cast<முழு>(lbits));
    copyBits(0, lhs, 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, lhs.bitIs(lbits - 1));
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opClean(மாறிலி V3_எண்& lhs, uint32_t bits) { திரும்பு opSel(lhs, bits - 1, 0); }

வெறுமை V3_எண்::opCleanThis(தருக்க warnOnTruncation) {
    // Clean MSB of number
    இலக்கம்_ASSERT_LOGIC_ARGS1(*இது);
    மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[சொற்கள்() - 1];
    மாறிலி uint32_t new_மதிப்பு_Msb = v.m_மதிப்பு & hiWordMask();
    மாறிலி uint32_t new_மதிப்பு_XMsb = v.m_மதிப்பு_X & hiWordMask();
    எனில் (warnOnTruncation && (new_மதிப்பு_Msb != v.m_மதிப்பு || new_மதிப்பு_XMsb != v.m_மதிப்பு_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << width() << " bit number: " << displayed("%d"));
    }
    m_data.இலக்கம்()[சொற்கள்() - 1] = {new_மதிப்பு_Msb, new_மதிப்பு_XMsb};
}

V3_எண்& V3_எண்::opSel(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& msb, மாறிலி V3_எண்& lsb) {
    இலக்கம்_ASSERT_OP_ARGS3(lhs, msb, lsb);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS2(msb, lsb);
    எனில் (lsb.isFourState() || msb.isFourState()) திரும்பு setAllBitsX();
    திரும்பு opSel(lhs, msb.toUInt(), lsb.toUInt());
}

V3_எண்& V3_எண்::opSel(மாறிலி V3_எண்& lhs, uint32_t msbval, uint32_t lsbval) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    setZero();
    முழு copyWidth = 0;
    எனில் (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.width())) {
        மாறிலி uint64_t selectedWidth = uint64_t{msbval} - lsbval + 1;
        copyWidth = static_cast<முழு>(
            std::min({static_cast<uint64_t>(width()), static_cast<uint64_t>(lhs.width()) - lsbval,
                      selectedWidth}));
    }
    copyBits(0, lhs, copyWidth ? static_cast<முழு>(lsbval) : 0, copyWidth);
    fillBits(copyWidth, width() - copyWidth, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    திரும்பு *இது;
}

V3_எண்& V3_எண்::opSelInto(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& lsb, முழு width) {
    திரும்பு opSelInto(lhs, lsb.toSInt(), width);
}

V3_எண்& V3_எண்::opSelInto(மாறிலி V3_எண்& lhs, முழு lsbval, முழு width) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    மாறிலி முழு copyWidth = std::max(0, std::min({width, lhs.width(), இது->width() - lsbval}));
    copyBits(lsbval, lhs, 0, copyWidth);
    மாறிலி முழு fillWidth
        = std::max(0, std::min(width - copyWidth, இது->width() - lsbval - copyWidth));
    fillBits(lsbval + copyWidth, fillWidth, v3Global.constRemoveXs() ? 0 : 'x');
    திரும்பு *இது;
}

//======================================================================
// Ops - Floating point

V3_எண்& V3_எண்::opIToRD(மாறிலி V3_எண்& lhs, தருக்க isSigned) {
    // Correct number of zero bits/width matters
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_எண் noxz(lhs);
    noxz.opAssignNonXZ(lhs);
    இரட்டை d = 0;
    மாறிலி தருக்க negate = isSigned && noxz.isNegative();
    எனில் (negate) {
        மாறிலி V3_எண் noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    க்கு (முழு bit = noxz.width() - 1; bit >= 0; bit--) {
        // Some precision might be lost in this add, that's what we want
        எனில் (noxz.bitIs1(bit)) d += exp2(bit);
    }
    எனில் (negate) d = -d;
    திரும்பு setDouble(d);
}
V3_எண்& V3_எண்::opRToIS(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS1(lhs);
    மாறிலி இரட்டை v = VL_TRUNC(lhs.toDouble());
    மாறிலி int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    திரும்பு setLongS(i);
}
V3_எண்& V3_எண்::opRToIRoundS(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS1(lhs);
    மாறிலி இரட்டை v = VL_ROUND(lhs.toDouble());
    setZero();
    ஒன்றிப்பு {
        இரட்டை d;
        uint64_t q;
    } u;
    u.d = v;
    (வெறுமை)u.d;

    மாறிலி முழு exp = static_cast<முழு>((u.q >> 52ULL) & VL_MASK_Q(11)) - 1023;
    மாறிலி முழு lsb = exp - 52;
    மாறிலி uint64_t mantissa = (u.q & VL_MASK_Q(52)) | (1ULL << 52);
    எனில் (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        க்கு (முழு bit = 0; bit <= 52; ++bit) {
            எனில் (mantissa & (1ULL << bit)) {
                மாறிலி முழு outbit = bit + lsb;
                எனில் (outbit >= 0) setBit(outbit, 1);
            }
        }
        எனில் (v < 0) {
            மாறிலி V3_எண் noSign = *இது;
            opNegate(noSign);
        }
    }
    திரும்பு *இது;
}
V3_எண்& V3_எண்::opRealToBits(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    ஒன்றிப்பு {
        இரட்டை m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.toDouble();
    திரும்பு setQuad(u.m_v);
}
V3_எண்& V3_எண்::opBitsToRealD(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.width() == 64 && width() == 64, "Real operation on wrong sized number");
    ஒன்றிப்பு {
        இரட்டை m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.toUQuad();
    திரும்பு setDouble(u.m_d);
}
V3_எண்& V3_எண்::opNegateD(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS1(lhs);
    திரும்பு setDouble(-lhs.toDouble());
}
V3_எண்& V3_எண்::opAddD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setDouble(lhs.toDouble() + rhs.toDouble());
}
V3_எண்& V3_எண்::opSubD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setDouble(lhs.toDouble() - rhs.toDouble());
}
V3_எண்& V3_எண்::opMulD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setDouble(lhs.toDouble() * rhs.toDouble());
}
V3_எண்& V3_எண்::opDivD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setDouble(lhs.toDouble() / rhs.toDouble());
}
V3_எண்& V3_எண்::opPowD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setDouble(pow(lhs.toDouble(), rhs.toDouble()));
}
V3_எண்& V3_எண்::opEqD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toDouble() == rhs.toDouble());
}
V3_எண்& V3_எண்::opNeqD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toDouble() != rhs.toDouble());
}
V3_எண்& V3_எண்::opGtD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toDouble() > rhs.toDouble());
}
V3_எண்& V3_எண்::opGteD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toDouble() >= rhs.toDouble());
}
V3_எண்& V3_எண்::opLtD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toDouble() < rhs.toDouble());
}
V3_எண்& V3_எண்::opLteD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toDouble() <= rhs.toDouble());
}

//======================================================================
// Ops - String

V3_எண்& V3_எண்::opConcatN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setString(lhs.toString() + rhs.toString());
}
V3_எண்& V3_எண்::opReplN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    இலக்கம்_ASSERT_LOGIC_ARGS1(rhs);
    திரும்பு opReplN(lhs, rhs.toUInt());
}
V3_எண்& V3_எண்::opReplN(மாறிலி V3_எண்& lhs, uint32_t rhsval) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    string out;
    out.reserve(lhs.toString().length() * rhsval);
    க்கு (unsigned times = 0; times < rhsval; ++times) out += lhs.toString();
    திரும்பு setString(out);
}
V3_எண்& V3_எண்::opCvtPackString(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    திரும்பு setString(lhs.toString());
}
V3_எண்& V3_எண்::opToLowerN(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    மாறிலி std::string out = VString::downcase(lhs.toString());
    திரும்பு setString(out);
}
V3_எண்& V3_எண்::opToUpperN(மாறிலி V3_எண்& lhs) {
    இலக்கம்_ASSERT_OP_ARGS1(lhs);
    இலக்கம்_ASSERT_STRING_ARGS1(lhs);
    மாறிலி std::string out = VString::upcase(lhs.toString());
    திரும்பு setString(out);
}

V3_எண்& V3_எண்::opEqN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toString() == rhs.toString());
}
V3_எண்& V3_எண்::opNeqN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toString() != rhs.toString());
}
V3_எண்& V3_எண்::opGtN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toString() > rhs.toString());
}
V3_எண்& V3_எண்::opGteN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toString() >= rhs.toString());
}
V3_எண்& V3_எண்::opLtN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toString() < rhs.toString());
}
V3_எண்& V3_எண்::opLteN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs) {
    இலக்கம்_ASSERT_OP_ARGS2(lhs, rhs);
    இலக்கம்_ASSERT_STRING_ARGS2(lhs, rhs);
    திரும்பு setSingleBits(lhs.toString() <= rhs.toString());
}

//======================================================================

வெறுமை V3_எண்::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_வரி* மாறிலி fl = புதிய File_வரி{File_வரி::builtInFilename()};
    V3_எண் இலக்கம்{fl, 32, 0};
    இலக்கம்.selfTestThis();
}

வெறுமை V3_எண்::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_எண்::epsilonEqual(0, 0), உண்மை);
    UASSERT_SELFTEST(V3_எண்::epsilonEqual(1e19, 1e19), உண்மை);
    UASSERT_SELFTEST(V3_எண்::epsilonEqual(9, 0.0001), பொய்);
    UASSERT_SELFTEST(V3_எண்::epsilonEqual(1, 1 + std::numeric_limits<இரட்டை>::epsilon()), உண்மை);
    UASSERT_SELFTEST(V3_எண்::epsilonEqual(0.009, 0.00899999999999999931998839741709), உண்மை);

    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(0), உண்மை);
    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(1), உண்மை);
    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(-1), உண்மை);
    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(1.0001), பொய்);
    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(0.9999), பொய்);
    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(-1.0001), பொய்);
    UASSERT_SELFTEST(V3_எண்::epsilonIntegral(-0.9999), பொய்);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
