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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Number.h"

#అన్తర్భావ "V3File.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <cerrno>
#అన్తర్భావ <cmath>
#అన్తర్భావ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

constexpr పూర్ణాంకం MAX_SPRINTF_DOUBLE_కొలత
    = 1100;  // Maximum characters with a sprintf %e/%f/%g (really 1079)

// Number operations build output in-place so can't call e.g. foo.opX(foo)
#నిర్వచించు అంకె_ASSERT_OP_ARGS1(arg1) \
    UASSERT((ఇది != &(arg1)), "Number operation called with same source and dest")
#నిర్వచించు అంకె_ASSERT_OP_ARGS2(arg1, arg2) \
    UASSERT((ఇది != &(arg1) && ఇది != &(arg2)), \
            "Number operation called with same source and dest")
#నిర్వచించు అంకె_ASSERT_OP_ARGS3(arg1, arg2, arg3) \
    UASSERT((ఇది != &(arg1) && ఇది != &(arg2) && ఇది != &(arg3)), \
            "Number operation called with same source and dest")
#నిర్వచించు అంకె_ASSERT_OP_ARGS4(arg1, arg2, arg3, arg4) \
    UASSERT((ఇది != &(arg1) && ఇది != &(arg2) && ఇది != &(arg3) && ఇది != &(arg4)), \
            "Number operation called with same source and dest")

#నిర్వచించు అంకె_ASSERT_LOGIC_ARGS1(arg1) \
    UASSERT(((arg1).దత్తాంశం_Type() == V3_సంఖ్య_దత్తాంశం::V3_సంఖ్య_దత్తాంశం_Type::LOGIC), \
            "Number operation called with non-logic (double or string) argument: '" << (arg1) \
                                                                                    << '"')
#నిర్వచించు అంకె_ASSERT_LOGIC_ARGS2(arg1, arg2) \
    అంకె_ASSERT_LOGIC_ARGS1(arg1); \
    అంకె_ASSERT_LOGIC_ARGS1(arg2)

#నిర్వచించు అంకె_ASSERT_LOGIC_ARGS4(arg1, arg2, arg3, arg4) \
    అంకె_ASSERT_LOGIC_ARGS1(arg1); \
    అంకె_ASSERT_LOGIC_ARGS1(arg2); \
    అంకె_ASSERT_LOGIC_ARGS1(arg3); \
    అంకె_ASSERT_LOGIC_ARGS1(arg4)

#నిర్వచించు అంకె_ASSERT_STRING_ARGS1(arg1) \
    UASSERT((arg1).isString(), \
            "Number operation called with non-string argument: '" << (arg1) << '"')
#నిర్వచించు అంకె_ASSERT_STRING_ARGS2(arg1, arg2) \
    అంకె_ASSERT_STRING_ARGS1(arg1); \
    అంకె_ASSERT_STRING_ARGS1(arg2)

#నిర్వచించు అంకె_ASSERT_DOUBLE_ARGS1(arg1) \
    UASSERT((arg1).isDouble(), \
            "Number operation called with non-double argument: '" << (arg1) << '"')
#నిర్వచించు అంకె_ASSERT_DOUBLE_ARGS2(arg1, arg2) \
    అంకె_ASSERT_DOUBLE_ARGS1(arg1); \
    అంకె_ASSERT_DOUBLE_ARGS1(arg2)

//======================================================================
// Errors

శూన్య V3_సంఖ్య::v3error_చివర(స్థిరరాశి std::ostringstream& str) స్థిరరాశి VL_RELEASE(V3Error::s().m_mutex) {
    std::ostringstream nsstr;
    nsstr << str.str();
    ఒకవేళ (m_nodep) {
        m_nodep->v3error_చివర(nsstr);
    } లేకపోతే ఒకవేళ (m_fileline) {
        m_fileline->v3error_చివర(nsstr);
    } లేకపోతే {
        V3Error::v3error_చివర(nsstr, "", శూన్యసూచిక);
    }
}

శూన్య V3_సంఖ్య::v3error_చివర_Fatal(స్థిరరాశి std::ostringstream& str) స్థిరరాశి
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_చివర(str);
    ప్రతిజ్ఞా(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Read class functions
// CREATION

V3_సంఖ్య::V3_సంఖ్య(Ast_కణుపు* nodep, VerilogStringLiteral, స్థిరరాశి string& str) {
    // Create a number using a verilog string as the value, thus 8 bits per character.
    ఒకవేళ (str.ఖాళీలు()) {  // IEEE 1800-2023 11.10.3 "" = "\000"
        init(nodep, 8);
    } లేకపోతే {
        init(nodep, std::max<పూర్ణాంకం>(str.నిడివి() * 8, 1));
        కొరకు (గుర్తులేని pos = 0; pos < str.నిడివి(); ++pos) {
            స్థిరరాశి పూర్ణాంకం topos = str.నిడివి() - 1 - pos;
            విలువ_And_X& v = m_దత్తాంశం.అంకె()[topos / 4];
            కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < 8; ++బిట్టు_2) {
                ఒకవేళ (str[pos] & (1UL << బిట్టు_2)) v.m_విలువ |= (1UL << (బిట్టు_2 + (topos % 4) * 8));
            }
        }
    }
    m_దత్తాంశం.m_fromString = నిజం;
    opCleanThis(నిజం);
}

V3_సంఖ్య::V3_సంఖ్య(Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు_DType* nodedtypep) {
    ఒకవేళ (nodedtypep->isCHandle()) {
        init(nodep);
        సమితి_Null();
    } లేకపోతే ఒకవేళ (nodedtypep->isDouble()) {
        init(nodep, 64);
        సమితి_Double(0.0);
    } లేకపోతే ఒకవేళ (nodedtypep->isString()) {
        init(nodep);
        సమితి_String("");
    } లేకపోతే {
        init(nodep, nodedtypep->వెడల్పు(), nodedtypep->వెడల్పు_Sized());
    }
}

శూన్య V3_సంఖ్య::create(స్థిరరాశి అక్షరం* sourcep) {
    m_దత్తాంశం.సమితి_Logic();
    స్థిరరాశి అక్షరం* విలువ_startp = sourcep;
    కొరకు (స్థిరరాశి అక్షరం* cp = sourcep; *cp; ++cp) {
        ఒకవేళ (*cp == '\'') {
            విలువ_startp = cp + 1;
            ఆపు;
        }
    }

    తార్కిక userSized = అబద్ధం;
    తార్కిక unbased = అబద్ధం;
    అక్షరం పునాది = '\0';
    ఒకవేళ (విలువ_startp != sourcep) {  // Has a '
        string widthn;
        స్థిరరాశి అక్షరం* cp = sourcep;
        కొరకు (; *cp; ++cp) {
            ఒకవేళ (*cp == '\'') {
                ++cp;
                ఆపు;
            }
            ఒకవేళ (*cp != '_') widthn += *cp;
        }
        అయితే (*cp == '_') cp++;
        ఒకవేళ (*cp && std::tolower(*cp) == 's') {
            ++cp;
            isSigned(నిజం);
        }
        ఒకవేళ (*cp) {
            పునాది = *cp;
            ++cp;
        }
        విలువ_startp = cp;

        userSized = widthn.నిడివి() != 0;
        ఒకవేళ (std::atoi(widthn.c_str())) {
            ఒకవేళ (std::atoi(widthn.c_str()) < 0
                || std::atoi(widthn.c_str()) > v3Global.opt.max_అంకె_వెడల్పు()) {
                // atoi might convert large number to negative, so can't tell which
                v3error("Unsupported: Width of number exceeds implementation limit: "
                        << sourcep << "  (IEEE 1800-2023 6.9.1)");
                వెడల్పు(v3Global.opt.max_అంకె_వెడల్పు(), నిజం);
            } లేకపోతే {
                వెడల్పు(std::atoi(widthn.c_str()), నిజం);
            }
        }
    } లేకపోతే {
        unbased = నిజం;
        పునాది = 'd';
    }

    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, 0};

    // Special SystemVerilog unsized constructs
    ఒకవేళ (పునాది == '0') {
        వెడల్పు(1, అబద్ధం);  // So we extend it
        సమితి_బిట్టు(0, 0);
        m_దత్తాంశం.m_autoExtend = నిజం;
    } లేకపోతే ఒకవేళ (పునాది == '1') {
        వెడల్పు(1, అబద్ధం);  // So we extend it
        సమితి_బిట్టు(0, 1);
        m_దత్తాంశం.m_autoExtend = నిజం;
    } లేకపోతే ఒకవేళ (std::tolower(పునాది) == 'z') {
        వెడల్పు(1, అబద్ధం);  // So we extend it
        సమితి_బిట్టు(0, 'z');
        m_దత్తాంశం.m_autoExtend = నిజం;
    } లేకపోతే ఒకవేళ (std::tolower(పునాది) == 'x') {
        వెడల్పు(1, అబద్ధం);  // So we extend it
        సమితి_బిట్టు(0, 'x');
        m_దత్తాంశం.m_autoExtend = నిజం;
    }
    // Otherwise...
    లేకపోతే ఒకవేళ (!sized()) {
        // We don't use v3Global.opt.maxNumWidth() here, as it can be arbitrarily large,
        // and cause extremely slow parsing. We will resize the value at the end anyway.
        // We just need a width big enough to fit the constant, so we use a conservative
        // upper bound to start from. Should never need more than 4 bits per digit.
        స్థిరరాశి పూర్ణాంకం వెడల్పు_Bound = std::max<పూర్ణాంకం>(32, std::అక్షరశ్రేణి_పొడవు(విలువ_startp) * 4);
        వెడల్పు(వెడల్పు_Bound, అబద్ధం);  // Will change width below
        ఒకవేళ (unbased) isSigned(నిజం);  // Also says the spec.
    }

    // Ignore leading blanks
    అయితే (*విలువ_startp == '_' || std::ఖాళీనా(*విలువ_startp)) ++విలువ_startp;
    ఒకవేళ (!*విలువ_startp && !m_దత్తాంశం.m_autoExtend) {
        v3error("Number is missing value digits: " << sourcep);
    }
    ఒకవేళ (userSized && m_దత్తాంశం.m_autoExtend) {
        v3error("Syntax error: size cannot be provided with '0/'1/'x/'z: "
                << sourcep << " (IEEE 1800-2023 5.7.1)");
    }

    పూర్ణాంకం obit = 0;  // Start at LSB
    పూర్ణాంకం పునాది_align = 1;
    ఒకవేళ (std::tolower(పునాది) == 'd') {
        // Ignore leading zeros so we don't issue too many digit errors when lots of leading 0's
        అయితే (*విలువ_startp == '_' || *విలువ_startp == '0') ++విలువ_startp;
        // Convert decimal number to hex
        పూర్ణాంకం olen = 0;
        uint32_t val = 0;
        పూర్ణాంకం got_x = 0;
        పూర్ణాంకం got_z = 0;
        పూర్ణాంకం got_01 = 0;
        తార్కిక warnedTooMany = అబద్ధం;
        కొరకు (స్థిరరాశి అక్షరం* cp = విలువ_startp; *cp; ++cp) {
            ఎంపిక (std::tolower(*cp)) {
            స్థితి '0':  // FALLTHRU
            స్థితి '1':  // FALLTHRU
            స్థితి '2':  // FALLTHRU
            స్థితి '3':  // FALLTHRU
            స్థితి '4':  // FALLTHRU
            స్థితి '5':  // FALLTHRU
            స్థితి '6':  // FALLTHRU
            స్థితి '7':  // FALLTHRU
            స్థితి '8':  // FALLTHRU
            స్థితి '9': {
                ఒకవేళ (olen <= 7) {  // 10000000 fits in 32 bits, so ok
                    // Constants are common, so for speed avoid wide math until we need it
                    val = val * 10 + (*cp - '0');
                    m_దత్తాంశం.అంకె()[0].m_విలువ = val;
                } లేకపోతే {  // Wide; all previous digits are already in m_value[0]
                    // this = (this * 10)/*product*/ + (*cp-'0')/*addend*/
                    // Assumed rare; lots of optimizations are possible here
                    V3_సంఖ్య లబ్ధం(ఇది, వెడల్పు() + 4);  // +4 for overflow detection
                    స్థిరరాశి V3_సంఖ్య ten(ఇది, వెడల్పు() + 4, 10);
                    స్థిరరాశి V3_సంఖ్య addend(ఇది, వెడల్పు(), (*cp - '0'));
                    లబ్ధం.opMul(*ఇది, ten);
                    op_కూడు(లబ్ధం, addend);
                    ఒకవేళ (!warnedTooMany && లబ్ధం.బిట్లు_విలువ(వెడల్పు(), 4)) {  // Overflowed
                        warnTooMany(sourcep);
                        warnedTooMany = నిజం;
                    }
                }
                ++olen;
                got_01 = 1;
                ఆపు;
            }
            స్థితి 'z':
            స్థితి '?': {
                got_z = 1;
                ఒకవేళ (!userSized) వెడల్పు(32, అబద్ధం);
                సమితి_All_బిట్లు_Z();
                ఆపు;
            }
            స్థితి 'x': {
                got_x = 1;
                ఒకవేళ (!userSized) వెడల్పు(32, అబద్ధం);
                సమితి_All_బిట్లు_X();
                ఆపు;
            }
            స్థితి '_': ఆపు;
            అప్రమేయ: {  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal character in decimal constant: " << *cp);
                ఆపు;
            }  // LCOV_EXCL_STOP
            }
        }
        obit = వెడల్పు();
        ఒకవేళ ((got_01 + got_x + got_z) > 1) {
            v3error("Mixing X/Z/? with digits not legal in decimal constant: " << విలువ_startp);
        }
    } లేకపోతే {
        // Convert bin/octal number to hex
        కొరకు (స్థిరరాశి అక్షరం* cp = విలువ_startp + std::అక్షరశ్రేణి_పొడవు(విలువ_startp) - 1; cp >= విలువ_startp;
             cp--) {
            ఒకవేళ (*cp != '_' && *cp != '0' && obit >= వెడల్పు()) {
                warnTooMany(sourcep);
                ఆపు;
            }
            ఎంపిక (std::tolower(పునాది)) {
            స్థితి 'b': {
                పునాది_align = 1;
                ఎంపిక (std::tolower(*cp)) {
                స్థితి '0': సమితి_బిట్టు(obit++, 0); ఆపు;
                స్థితి '1': సమితి_బిట్టు(obit++, 1); ఆపు;
                స్థితి 'z':
                స్థితి '?': సమితి_బిట్టు(obit++, 'z'); ఆపు;
                స్థితి 'x': సమితి_బిట్టు(obit++, 'x'); ఆపు;
                స్థితి '_': ఆపు;
                అప్రమేయ: v3error("Illegal character in binary constant: " << *cp);
                }
                ఆపు;
            }

            స్థితి 'o':
            స్థితి 'c': {
                పునాది_align = 3;
                ఎంపిక (std::tolower(*cp)) {  // clang-format off
                స్థితి '0': సమితి_బిట్టు(obit++, 0); సమితి_బిట్టు(obit++, 0);  సమితి_బిట్టు(obit++, 0);  ఆపు;
                స్థితి '1': సమితి_బిట్టు(obit++, 1); సమితి_బిట్టు(obit++, 0);  సమితి_బిట్టు(obit++, 0);  ఆపు;
                స్థితి '2': సమితి_బిట్టు(obit++, 0); సమితి_బిట్టు(obit++, 1);  సమితి_బిట్టు(obit++, 0);  ఆపు;
                స్థితి '3': సమితి_బిట్టు(obit++, 1); సమితి_బిట్టు(obit++, 1);  సమితి_బిట్టు(obit++, 0);  ఆపు;
                స్థితి '4': సమితి_బిట్టు(obit++, 0); సమితి_బిట్టు(obit++, 0);  సమితి_బిట్టు(obit++, 1);  ఆపు;
                స్థితి '5': సమితి_బిట్టు(obit++, 1); సమితి_బిట్టు(obit++, 0);  సమితి_బిట్టు(obit++, 1);  ఆపు;
                స్థితి '6': సమితి_బిట్టు(obit++, 0); సమితి_బిట్టు(obit++, 1);  సమితి_బిట్టు(obit++, 1);  ఆపు;
                స్థితి '7': సమితి_బిట్టు(obit++, 1); సమితి_బిట్టు(obit++, 1);  సమితి_బిట్టు(obit++, 1);  ఆపు;
                స్థితి 'z':
                స్థితి '?': సమితి_బిట్టు(obit++, 'z'); సమితి_బిట్టు(obit++, 'z'); సమితి_బిట్టు(obit++, 'z'); ఆపు;
                స్థితి 'x': సమితి_బిట్టు(obit++, 'x'); సమితి_బిట్టు(obit++, 'x'); సమితి_బిట్టు(obit++, 'x'); ఆపు;
                స్థితి '_': ఆపు;
                // clang-format on
                అప్రమేయ: v3error("Illegal character in octal constant");
                }
                ఆపు;
            }

            స్థితి 'h':  // FALLTHRU
            స్థితి 'x': {
                పునాది_align = 4;
                ఎంపిక (std::tolower(*cp)) {  // clang-format off
                స్థితి '0': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '1': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '2': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '3': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '4': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '5': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '6': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '7': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); ఆపు;
                స్థితి '8': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి '9': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'a': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'b': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'c': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'd': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'e': సమితి_బిట్టు(obit++,0); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'f': సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); సమితి_బిట్టు(obit++,1); ఆపు;
                స్థితి 'z':
                స్థితి '?': సమితి_బిట్టు(obit++,'z'); సమితి_బిట్టు(obit++,'z'); సమితి_బిట్టు(obit++,'z'); సమితి_బిట్టు(obit++,'z'); ఆపు;
                స్థితి 'x': సమితి_బిట్టు(obit++,'x'); సమితి_బిట్టు(obit++,'x'); సమితి_బిట్టు(obit++,'x'); సమితి_బిట్టు(obit++,'x'); ఆపు;
                    // clang-format on
                స్థితి '_': ఆపు;
                అప్రమేయ:  // LCOV_EXCL_START
                    // Likely impossible as parser prevents hitting it
                    v3error("Illegal character in hex constant: " << *cp);
                    ఆపు;
                }  // LCOV_EXCL_STOP
                ఆపు;
            }
            అప్రమేయ:  // LCOV_EXCL_START
                // Likely impossible as parser prevents hitting it
                v3error("Illegal base character: " << పునాది);
                ఆపు;
            }  // LCOV_EXCL_STOP
        }
    }

    // If was unsized, trim width per IEEE 1800-2023 5.7.1
    ఒకవేళ (!userSized && !m_దత్తాంశం.m_autoExtend) {
        వెడల్పు(std::max(32, పునాది_align * ((వెడల్పు_గమ్యం_Fit() + పునాది_align - 1) / పునాది_align)), అబద్ధం);
    }

    // Z or X extend specific width values.  Spec says we don't 1 extend.
    // This fixes 2'bx to become 2'bxx.
    అయితే (obit <= వెడల్పు() && obit && బిట్టు_Is_XZ(obit - 1)) {
        సమితి_బిట్టు(obit, బిట్టు_Is(obit - 1));
        ++obit;
    }
    opCleanThis(నిజం);

    // printf("Dump \"%s\"  CP \"%s\"  B '%c' %d W %d\n", sourcep, value_startp, base, width(),
    // m_value[0]);
}

శూన్య V3_సంఖ్య::warnTooMany(స్థిరరాశి string& విలువ) {
    v3warn(WIDTHDIGITS, "Too many digits for " << వెడల్పు() << " bit number: '" << విలువ << "'");
}

శూన్య V3_సంఖ్య::nodep(Ast_కణుపు* nodep) VL_MT_STABLE {
    m_nodep = nodep;
    ఒకవేళ (!nodep) తిరిగి;
    m_fileline = nodep->fileline();
}

//======================================================================
// Global

తార్కిక V3_సంఖ్య::epsilonEqual(ద్విగుణ a, ద్విగుణ b) {
    తిరిగి std::fabs(a - b)
           <= (std::numeric_limits<ద్విగుణ>::epsilon() * std::max(1.0, std::max(a, b)));
}

తార్కిక V3_సంఖ్య::epsilonIntegral(ద్విగుణ a) {
    స్థిరరాశి ద్విగుణ dint = static_cast<ద్విగుణ>(static_cast<int64_t>(a));
    // Check all the rounding possibilities, as we did truncation above rather than rounding
    తిరిగి epsilonEqual(a, dint) || epsilonEqual(a, dint - 1) || epsilonEqual(a, dint + 1);
}

పూర్ణాంకం V3_సంఖ్య::log2b(uint32_t అంకె) {
    // See also opCLog2
    కొరకు (పూర్ణాంకం బిట్టు_2 = 31; బిట్టు_2 > 0; బిట్టు_2--) {
        ఒకవేళ (అంకె & (1ULL << బిట్టు_2)) తిరిగి బిట్టు_2;
    }
    తిరిగి 0;
}

పూర్ణాంకం V3_సంఖ్య::log2bQuad(uint64_t అంకె) {
    ఒకవేళ (అంకె >> 32ULL) తిరిగి 32 + log2b(అంకె >> 32ULL);
    తిరిగి log2b(అంకె);
}

//======================================================================
// Setters

V3_సంఖ్య& V3_సంఖ్య::సమితి_Zero() {
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, 0};
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_Quad(uint64_t విలువ) {
    సమితి_Zero();
    m_దత్తాంశం.అంకె()[0].m_విలువ = విలువ & 0xffffffffULL;
    ఒకవేళ (వెడల్పు() > 32) m_దత్తాంశం.అంకె()[1].m_విలువ = (విలువ >> 32ULL) & 0xffffffffULL;
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_Long(uint32_t విలువ) {
    సమితి_Zero();
    m_దత్తాంశం.అంకె()[0].m_విలువ = విలువ;
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_Long_S(int32_t విలువ) {
    సమితి_Zero();
    సంఘం {
        uint32_t u;
        int32_t s;
    } u;
    u.s = విలువ;
    (శూన్య)u.s;
    m_దత్తాంశం.అంకె()[0].m_విలువ = u.u;
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_Double(ద్విగుణ విలువ) {
    UASSERT(వెడల్పు() == 64, "Real operation on wrong sized number");
    m_దత్తాంశం.సమితి_Double();
    సంఘం {
        ద్విగుణ d;
        uint32_t u[2];
    } u;
    u.d = విలువ;
    (శూన్య)u.d;
    కొరకు (పూర్ణాంకం i = 2; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, 0};
    m_దత్తాంశం.అంకె()[0].m_విలువ = u.u[0];
    m_దత్తాంశం.అంకె()[1].m_విలువ = u.u[1];
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_Single_బిట్లు(అక్షరం విలువ) {
    కొరకు (పూర్ణాంకం i = 1 /*పై*/; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, 0};
    m_దత్తాంశం.అంకె()[0] = {(విలువ == '1' || విలువ == 'x' || విలువ == 1 || విలువ == 3),
                       (విలువ == 'z' || విలువ == 'x' || విలువ == 2 || విలువ == 3)};
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::సమితి_All_Bits0() {
    సమితి_Zero();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_All_Bits1() {
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {~0U, 0};
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_All_బిట్లు_X() {
    // Use setAllBitsXRemoved if calling this based on a non-X/Z input value such as divide by zero
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {~0U, ~0U};
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_All_బిట్లు_Z() {
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, ~0U};
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_All_బిట్లు_XRemoved() {
    ఒకవేళ (!v3Global.constRemoveXs()) {
        తిరిగి సమితి_All_బిట్లు_X();
    } లేకపోతే {
        // If we get a divide by zero we get Xs.
        // But after V3Unknown we have removed Xs, so use --x-assign to direct-insert 0/1
        ఒకవేళ (v3Global.opt.x_కేటాయించు() == "1") {
            తిరిగి సమితి_All_Bits1();
        } లేకపోతే {
            తిరిగి సమితి_All_Bits0();
        }
    }
}
V3_సంఖ్య& V3_సంఖ్య::సమితి_Value1() {
    m_దత్తాంశం.అంకె()[0] = {1, 0};
    కొరకు (పూర్ణాంకం i = 1; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, 0};
    తిరిగి *ఇది;
}

శూన్య V3_సంఖ్య::సమితి_బిట్టు_X0(పూర్ణాంకం బిట్టు_2) {
    // Selection beyond bounds after V3Premit needs to have 0s
    // in upper bits.  Contrast to setAllBitsXRemoved which honors xAssign
    సమితి_బిట్టు(బిట్టు_2, v3Global.constRemoveXs() ? 0 : 'x');
}

V3_సంఖ్య& V3_సంఖ్య::సమితి_ముసుగు(పూర్ణాంకం nbits, పూర్ణాంకం lsb) {
    సమితి_Zero();
    UASSERT(lsb >= 0, "Negative destination bit range");
    fill_బిట్లు(lsb, std::max(0, std::min(nbits, వెడల్పు() - lsb)), 1);
    తిరిగి *ఇది;
}

//======================================================================
// ACCESSORS - as strings

string V3_సంఖ్య::ascii(తార్కిక prefixed, తార్కిక cleanVerilog) స్థిరరాశి VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ఒకవేళ (is1_అడుగు()) తిరిగి "1step";
    ఒకవేళ (isNull()) తిరిగి "null";
    std::ostringstream బహిర్గతం;
    ఒకవేళ (isDouble()) {
        బహిర్గతం.precision(17);
        ఒకవేళ (VL_UNCOVERABLE(వెడల్పు() != 64)) {
            బహిర్గతం << "%E-bad-width-double";  // LCOV_EXCL_LINE
        } లేకపోతే {
            బహిర్గతం << గమ్యం_Double();
            ఒకవేళ (గమ్యం_Double() == floor(గమ్యం_Double())) బహిర్గతం << ".0";
        }
        తిరిగి బహిర్గతం.str();
    } లేకపోతే ఒకవేళ (isString()) {
        తిరిగి '"' + గమ్యం_String() + '"';
    } లేకపోతే {
        ఒకవేళ (VL_UNCOVERABLE((m_దత్తాంశం.అంకె()[పదాలు() - 1].m_విలువ | m_దత్తాంశం.అంకె()[పదాలు() - 1].m_విలువ_X)
                           & ~hi_మాట_ముసుగు())) {
            బహిర్గతం << "%E-hidden-bits";  // LCOV_EXCL_LINE
        }
    }
    ఒకవేళ (prefixed) {
        ఒకవేళ (sized()) {
            బహిర్గతం << వెడల్పు() << "'";
        } లేకపోతే ఒకవేళ (autoExtend() && వెడల్పు() == 1) {
            బహిర్గతం << "'";
            ఒకవేళ (బిట్టు_Is0(0)) {
                బహిర్గతం << '0';
            } లేకపోతే ఒకవేళ (బిట్టు_Is1(0)) {
                బహిర్గతం << '1';
            } లేకపోతే ఒకవేళ (బిట్టు_Is_Z(0)) {
                బహిర్గతం << 'z';
            } లేకపోతే {
                బహిర్గతం << 'x';
            }
            తిరిగి బహిర్గతం.str();
        } లేకపోతే {
            ఒకవేళ (cleanVerilog) {
                బహిర్గతం << "'";
            } లేకపోతే {
                బహిర్గతం << "?" << వెడల్పు() << "?";
            }
        }
        ఒకవేళ (isSigned()) బహిర్గతం << "s";
    }

    స్థిరరాశి తార్కిక ద్విమాన = (isFourState()
#నిర్వచితమైతే V3_సంఖ్య_ASCII_ద్విమాన
                         // cppcheck-suppress konwnConditionTrueFalse
                         || నిజం
#అయితే_ముగింపు
    );
    // out<<"-"<<hex<<m_value[0]<<"-";

    // cppcheck-suppress konwnConditionTrueFalse
    ఒకవేళ (ద్విమాన) {
        బహిర్గతం << "b";
        బహిర్గతం << displayed("%0b");
    } లేకపోతే {
        ఒకవేళ (prefixed) బహిర్గతం << "h";
        // Always deal with 4 bits at once.  Note no 4-state, it's above.
        బహిర్గతం << displayed("%0h");
    }
    ఒకవేళ (isNull()) {
        ఒకవేళ (VL_UNCOVERABLE(!isEqZero())) {
            బహిర్గతం << "-%E-null-not-zero";  // LCOV_EXCL_LINE
        } లేకపోతే {
            బహిర్గతం << " [null]";
        }
    }
    తిరిగి బహిర్గతం.str();
}

తార్కిక V3_సంఖ్య::displayedFmtHasArg(అక్షరం format, తార్కిక is_పరిశీలన) {
    // Is this a format letter that takes an argument in runtime?
    (శూన్య)is_పరిశీలన;
    ఎంపిక (std::tolower(format)) {
    స్థితి 'b': తిరిగి నిజం;  //ద్విమాన
    స్థితి 'c': తిరిగి నిజం;  // Character
    స్థితి 'd': తిరిగి నిజం;  // Decimal
    స్థితి 'e': తిరిగి నిజం;  // Floating
    స్థితి 'f': తిరిగి నిజం;  // Floating
    స్థితి 'g': తిరిగి నిజం;  // Floating
    స్థితి 'h': తిరిగి నిజం;  // Hex
    స్థితి 'o': తిరిగి నిజం;  // Octal
    స్థితి 'p': తిరిగి నిజం;  //నమూనా
    స్థితి 's': తిరిగి నిజం;  // String
    స్థితి 't': తిరిగి నిజం;  //సమయం
    స్థితి 'u': తిరిగి నిజం;  // Packed 2-state
    స్థితి 'v': తిరిగి నిజం;  // Strength
    స్థితి 'x': తిరిగి నిజం;  // Hex
    స్థితి 'z': తిరిగి నిజం;  // Packed 4-state
    స్థితి '?': తిరిగి నిజం;  // Internal: Compute format in V3Width based on data type
    అప్రమేయ: తిరిగి అబద్ధం;
    }
    // 'l'                  // Library - precomputed front string, so no arg
    // 'm'                  // Module - precomputed front string, so no arg
    // '*'                  // $scan ignore argument
}

string V3_సంఖ్య::displayPad(కొలత_t fmtsize, అక్షరం pad, తార్కిక ఎడమ, స్థిరరాశి string& లోపల) VL_శుద్ధ {
    string padding;
    ఒకవేళ (లోపల.నిడివి() < fmtsize) padding = string(fmtsize - లోపల.నిడివి(), pad);
    తిరిగి ఎడమ ? (లోపల + padding) : (padding + లోపల);
}

string V3_సంఖ్య::displayedEnumName(స్థిరరాశి Ast_Enum_అంశం* itemp) {
    స్థిరరాశి string name = VIdProtect::protect(itemp->prettyName());
    తిరిగి VString::isIdentifier(name) && !std::అంకెనా(static_cast<గుర్తులేని అక్షరం>(name[0]))
               ? name
               : "\\" + name + " ";
}

string V3_సంఖ్య::displayedEnumName(స్థిరరాశి AstEnumDType* dtypep) స్థిరరాశి VL_MT_STABLE {
    V3_సంఖ్య విలువ{ఇది, dtypep->వెడల్పు()};
    విలువ.op_కేటాయించు(*ఇది);
    కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = dtypep->itemsp(); itemp;
         itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(itemp->valuep(), Const);
        ఒకవేళ (!constp->అంకె().isAnyXZ() && విలువ.isCaseEq(constp->అంకె()))
            తిరిగి displayedEnumName(itemp);
    }
    తిరిగి "";
}

string V3_సంఖ్య::displayedEnum(స్థిరరాశి AstSFormatArg* argp,
                               స్థిరరాశి string& vformat) స్థిరరాశి VL_MT_STABLE {
    స్థిరరాశి AstEnumDType* స్థిరరాశి dtypep = VN_AS(argp->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
    V3_సంఖ్య విలువ{ఇది, dtypep->వెడల్పు()};
    విలువ.op_కేటాయించు(*ఇది);
    స్థిరరాశి VFormatAttr numericAttr
        = dtypep->isSigned() ? VFormatAttr::SIGNED : VFormatAttr::UNSIGNED;
    స్థిరరాశి అక్షరం fmt = std::tolower(vformat.వెనుక());
    ఒకవేళ (fmt != 'p' && fmt != 's') తిరిగి విలువ.displayed(argp, vformat, numericAttr);
    string name = విలువ.displayedEnumName(dtypep);
    ఒకవేళ (name.ఖాళీలు()) name = విలువ.displayed(argp, fmt == 'p' ? vformat : "%0d", numericAttr);
    V3_సంఖ్య name_అంకె{ఇది};
    name_అంకె.సమితి_String(name);
    తిరిగి name_అంకె.displayed(argp, vformat, VFormatAttr::COMPLEX);
}

string V3_సంఖ్య::displayed(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& vformat,
                           స్థిరరాశి VFormatAttr& formatAttr) స్థిరరాశి VL_MT_STABLE {
    తిరిగి displayed(nodep->fileline(), vformat, formatAttr);
}

string V3_సంఖ్య::displayed(File_పంక్తి* fl, స్థిరరాశి string& vformat,
                           స్థిరరాశి VFormatAttr& formatAttr) స్థిరరాశి VL_MT_STABLE {
    // Similar code flow in verilated.cpp _vl_vsformat
    // Correct number of zero bits/width matters
    // UINFO(9, "displayed(\"" << vformat << "\", formatAttr=" << formatAttr);
    స్వయంచాలక pos = vformat.cbegin();
    UASSERT(pos != vformat.cend() && pos[0] == '%',
            "$display-like function with non-format argument " << *ఇది);
    ++pos;
    తార్కిక ఎడమ = అబద్ధం;
    ఒకవేళ (pos[0] == '-') {
        ఎడమ = నిజం;
        ++pos;
    }
    string fmtsize;
    కొరకు (; pos != vformat.cend() && (std::అంకెనా(pos[0]) || pos[0] == '.'); ++pos) {
        fmtsize += pos[0];
    }
    string str;

    అక్షరం fmt = std::tolower(pos[0]);

    // Override user's code for certain data-type determined arguments
    ఒకవేళ (formatAttr.isComplex()) {
        ఒకవేళ (fmt != 'p') fmt = 's';  // override
    } లేకపోతే ఒకవేళ (formatAttr.isString()) {
        ఒకవేళ (fmt == 'h' || fmt == 'x') {
            // V3Width errors on const %x of string, but V3Randomize may make a %x on a
            // string, or may have a runtime format
            V3_సంఖ్య strwide{fl, static_cast<పూర్ణాంకం>(గమ్యం_String().నిడివి()) * 8, 0};
            strwide.opNToI(*ఇది);
            తిరిగి strwide.displayed(fl, vformat, VFormatAttr::UNSIGNED);
        }
        ఒకవేళ (fmt != 'p') fmt = 's';  // override
    } లేకపోతే ఒకవేళ (formatAttr.isSigned() || formatAttr.isUnsigned()) {
        ఒకవేళ (fmt == 'p') {
            ఒకవేళ (fmtsize == "0") {  // For %0p, IEEE our choice, use 'h%0h
                str += "'h";
                fmt = 'h';
            } లేకపోతే {  // UVM tests require %0d
                fmtsize = "0";
                fmt = 'd';
            }
        }
    } లేకపోతే ఒకవేళ (formatAttr.isDouble()) {
        // 'p' not converted to 'g' here as vformat string can't be easily changed
        ఒకవేళ (!(fmt == 'e' || fmt == 'f' || fmt == 'g' || fmt == 'p'
              || VL_UNCOVERABLE(fmt == 't'))) {
            // A non-float format with a float, must convert to integer then format
            V3_సంఖ్య nonfloat{fl, 64, 0};
            nonfloat.opRToIRoundS(*ఇది);
            తిరిగి nonfloat.displayed(fl, vformat, VFormatAttr::SIGNED);
        }
    }

    ఎంపిక (fmt) {
    స్థితి 'c': {
        ఒకవేళ (వెడల్పు() > 8)
            fl->v3warn(WIDTHTRUNC, "$display-like format of %c format of > 8 bit value");
        స్థిరరాశి గుర్తులేని పూర్ణాంకం v = బిట్లు_విలువ(0, 8);
        str.నెట్టు_వెనుక(static_cast<అక్షరం>(v));
        తిరిగి str;
    }
    స్థితి 'e':  // FALLTHRU
    స్థితి 'f':  // FALLTHRU
    స్థితి 'g': {
        స్థిరరాశి ద్విగుణ n = formatAttr.isDouble()   ? గమ్యం_Double()
                         : formatAttr.isSigned() ? గమ్యం_SQuad()
                                                 : గమ్యం_UQuad();
        అక్షరం tmp[MAX_SPRINTF_DOUBLE_కొలత];
        (శూన్య)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_కొలత, vformat.c_str(), n);
        తిరిగి tmp;
    }
    స్థితి 's': {
        ఒకవేళ (formatAttr.isString() || formatAttr.isComplex()) {
            str = గమ్యం_String();
        } లేకపోతే {
            // Spec says always drop leading zeros, this isn't quite right, we space pad.
            పూర్ణాంకం బిట్టు_2 = వెడల్పు() - 1;
            తార్కిక ప్రారంభం = నిజం;
            అయితే ((బిట్టు_2 % 8) != 7) ++బిట్టు_2;
            కొరకు (; బిట్టు_2 >= 0; బిట్టు_2 -= 8) {
                స్థిరరాశి పూర్ణాంకం v = బిట్లు_విలువ(బిట్టు_2 - 7, 8);
                ఒకవేళ (!ప్రారంభం || v) {
                    str += static_cast<అక్షరం>((v == 0) ? ' ' : v);
                    ప్రారంభం = అబద్ధం;  // Drop leading 0s
                } లేకపోతే {
                    ఒకవేళ (fmtsize != "0") str += ' ';
                }
            }
        }
        స్థిరరాశి కొలత_t fmtsizen = static_cast<కొలత_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, ' ', ఎడమ, str);
        తిరిగి str;
    }
    స్థితి 'p': {  //నమూనా
        // 'p' with NUMBER was earlier converted to 'd'
        ఒకవేళ (formatAttr.isDouble()) {
            స్థిరరాశి ద్విగుణ n = formatAttr.isDouble()   ? గమ్యం_Double()
                             : formatAttr.isSigned() ? గమ్యం_SQuad()
                                                     : గమ్యం_UQuad();
            అక్షరం tmp[MAX_SPRINTF_DOUBLE_కొలత];
            (శూన్య)VL_SNPRINTF(tmp, MAX_SPRINTF_DOUBLE_కొలత, "%g", n);
            తిరిగి tmp;
        }
        ఒకవేళ (formatAttr.isString())
            తిరిగి '"' + V3_బహిర్గతం_Formatter::quoteNameControls(గమ్యం_String()) + '"';
        ఒకవేళ (formatAttr.isComplex()) తిరిగి గమ్యం_String();
        తిరిగి "%p";
    }
    స్థితి 'b':  // FALLTHRU
    స్థితి 'o':  // FALLTHRU
    స్థితి 'h':  // FALLTHRU
    స్థితి 'x': {
        పూర్ణాంకం బిట్టు_2 = వెడల్పు() - 1;
        ఒకవేళ (ఎడమ || !fmtsize.ఖాళీలు()) {
            అయితే (బిట్టు_2 && బిట్టు_Is0(బిట్టు_2)) --బిట్టు_2;
        }
        ఎంపిక (fmt) {
        స్థితి 'b': {
            కొరకు (; బిట్టు_2 >= 0; --బిట్టు_2) {
                ఒకవేళ (బిట్టు_Is0(బిట్టు_2)) {
                    str += '0';
                } లేకపోతే ఒకవేళ (బిట్టు_Is1(బిట్టు_2)) {
                    str += '1';
                } లేకపోతే ఒకవేళ (బిట్టు_Is_Z(బిట్టు_2)) {
                    str += 'z';
                } లేకపోతే {
                    str += 'x';
                }
            }
            ఆపు;
        }
        స్థితి 'o': {
            అయితే ((బిట్టు_2 % 3) != 2) ++బిట్టు_2;
            కొరకు (; బిట్టు_2 > 0; బిట్టు_2 -= 3) {
                స్థిరరాశి పూర్ణాంకం అంకె_X = లెక్క_X(బిట్టు_2 - 2, 3);
                స్థిరరాశి పూర్ణాంకం అంకె_Z = లెక్క_Z(బిట్టు_2 - 2, 3);
                ఒకవేళ (అంకె_X == 3 || అంకె_X == వెడల్పు() - (బిట్టు_2 - 2)) {
                    str += 'x';
                    కొనసాగు;
                }
                ఒకవేళ (అంకె_Z == 3 || అంకె_Z == వెడల్పు() - (బిట్టు_2 - 2)) {
                    str += 'z';
                    కొనసాగు;
                }
                ఒకవేళ (అంకె_X > 0) {
                    str += 'X';
                    కొనసాగు;
                }
                ఒకవేళ (అంకె_Z > 0) {
                    str += 'Z';
                    కొనసాగు;
                }
                స్థిరరాశి పూర్ణాంకం v = బిట్లు_విలువ(బిట్టు_2 - 2, 3);
                str += static_cast<అక్షరం>('0' + v);
            }
            ఆపు;
        }
        అప్రమేయ: {  // h/x
            అయితే ((బిట్టు_2 % 4) != 3) ++బిట్టు_2;
            కొరకు (; బిట్టు_2 > 0; బిట్టు_2 -= 4) {
                స్థిరరాశి పూర్ణాంకం అంకె_X = లెక్క_X(బిట్టు_2 - 3, 4);
                స్థిరరాశి పూర్ణాంకం అంకె_Z = లెక్క_Z(బిట్టు_2 - 3, 4);
                ఒకవేళ (అంకె_X == 4 || అంకె_X == వెడల్పు() - (బిట్టు_2 - 3)) {
                    str += 'x';
                    కొనసాగు;
                }
                ఒకవేళ (అంకె_Z == 4 || అంకె_Z == వెడల్పు() - (బిట్టు_2 - 3)) {
                    str += 'z';
                    కొనసాగు;
                }
                ఒకవేళ (అంకె_X > 0) {
                    str += 'X';
                    కొనసాగు;
                }
                ఒకవేళ (అంకె_Z > 0) {
                    str += 'Z';
                    కొనసాగు;
                }
                స్థిరరాశి పూర్ణాంకం v = బిట్లు_విలువ(బిట్టు_2 - 3, 4);
                ఒకవేళ (v >= 10) {
                    str += static_cast<అక్షరం>('a' + v - 10);
                } లేకపోతే {
                    str += static_cast<అక్షరం>('0' + v);
                }
            }
            ఆపు;
        }
        }  // switch
        స్థిరరాశి కొలత_t fmtsizen = static_cast<కొలత_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (ఎడమ ? ' ' : '0'), ఎడమ, str);
        తిరిగి str;
    }  // case b/d/x/o
    స్థితి 't':  //సమయం
    స్థితి 'd': {  // Unsigned decimal
        స్థిరరాశి తార్కిక issigned = formatAttr.isSigned();
        ఒకవేళ (fmtsize == "" && !ఎడమ) {
            స్థిరరాశి ద్విగుణ mantissabits = వెడల్పు() - (issigned ? 1 : 0);
            // To get the number of digits required, we want to compute
            // log10(2**mantissabits) and round it up. To be able to handle
            // a very wide mantissa, we use log2(2**mantissabits)/log2(10),
            // which is (+1.0 is for rounding bias):
            ద్విగుణ dchars = mantissabits / 3.321928094887362 + 1.0;
            ఒకవేళ (issigned) ++dchars;  // space for sign
            fmtsize = cvt_గమ్యం_Str(static_cast<పూర్ణాంకం>(dchars));
        }
        తార్కిక hasXZ = అబద్ధం;
        ఒకవేళ (isAllX()) {
            str = "x";
            hasXZ = నిజం;
        } లేకపోతే ఒకవేళ (isAllZ()) {
            str = "z";
            hasXZ = నిజం;
        } లేకపోతే ఒకవేళ (isAnyX()) {
            str = "X";
            hasXZ = నిజం;
        } లేకపోతే ఒకవేళ (isAnyZ()) {
            str = "Z";
            hasXZ = నిజం;
        }
        ఒకవేళ (!hasXZ) {
            ఒకవేళ (issigned) {
                ఒకవేళ (వెడల్పు() > 64) {
                    str = గమ్యం_Decimal_S();
                } లేకపోతే {
                    str = cvt_గమ్యం_Str(గమ్యం_SQuad());
                }
            } లేకపోతే {
                ఒకవేళ (వెడల్పు() > 64) {
                    str = గమ్యం_Decimal_U();
                } లేకపోతే {
                    str = cvt_గమ్యం_Str(గమ్యం_UQuad());
                }
            }
        }
        స్థిరరాశి తార్కిక zeropad = fmtsize.నిడివి() > 0 && fmtsize[0] == '0' && !ఎడమ;
        // fmtsize might have changed since we parsed the %fmtsize
        స్థిరరాశి కొలత_t fmtsizen = static_cast<కొలత_t>(std::atoi(fmtsize.c_str()));
        str = displayPad(fmtsizen, (zeropad ? '0' : ' '), ఎడమ, str);
        తిరిగి str;
    }
    // 'l'   // Library - converted to text by V3LinkResolve
    స్థితి 'u': {  // Packed 2-state
        కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
            స్థిరరాశి uint32_t v = m_దత్తాంశం.అంకె()[i].m_విలువ;
            str += static_cast<అక్షరం>((v >> 0) & 0xff);
            str += static_cast<అక్షరం>((v >> 8) & 0xff);
            str += static_cast<అక్షరం>((v >> 16) & 0xff);
            str += static_cast<అక్షరం>((v >> 24) & 0xff);
        }
        తిరిగి str;
    }
    స్థితి 'z': {  // Packed 4-state
        కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
            స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
            str += static_cast<అక్షరం>((v.m_విలువ >> 0) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ >> 8) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ >> 16) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ >> 24) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ_X >> 0) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ_X >> 8) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ_X >> 16) & 0xff);
            str += static_cast<అక్షరం>((v.m_విలువ_X >> 24) & 0xff);
        }
        తిరిగి str;
    }
    స్థితి 'v': {  // Strength
        పూర్ణాంకం బిట్టు_2 = వెడల్పు() - 1;
        కొరకు (; బిట్టు_2 >= 0; బిట్టు_2--) {
            ఒకవేళ (బిట్టు_Is0(బిట్టు_2)) {
                str += "St0 ";
            }  // Yes, always a space even for bit 0
            లేకపోతే ఒకవేళ (బిట్టు_Is1(బిట్టు_2)) {
                str += "St1 ";
            } లేకపోతే ఒకవేళ (బిట్టు_Is_Z(బిట్టు_2)) {
                str += "StZ ";
            } లేకపోతే {
                str += "StX";
            }
        }
        తిరిగి str;
    }
    అప్రమేయ: fl->v3fatalSrc("Unknown $display-like format code for number: %" << pos[0]);
    }
}

string V3_సంఖ్య::emitC() స్థిరరాశి VL_MT_STABLE {
    constexpr కొలత_t bufsize = 1000;
    అక్షరం sbuf[bufsize];
    string ఫలితం;
    ఒకవేళ (isNull()) {
        తిరిగి "VlNull{}";
    } లేకపోతే ఒకవేళ (isDouble()) {
        స్థిరరాశి ద్విగుణ dnum = గమ్యం_Double();
        ఒకవేళ (std::isinf(dnum)) {
            ఒకవేళ (std::signbit(dnum)) ఫలితం += '-';
            ఫలితం += "std::numeric_limits<double>::infinity()";
        } లేకపోతే ఒకవేళ (std::isnan(dnum)) {
            ఒకవేళ (std::signbit(dnum)) ఫలితం += '-';
            ఫలితం += "std::numeric_limits<double>::quiet_NaN()";
        } లేకపోతే {
            స్థిరరాశి అక్షరం* స్థిరరాశి fmt = (static_cast<పూర్ణాంకం>(dnum) == dnum && -1000 < dnum && dnum < 1000)
                                        ? "%3.1f"  // Force decimal point
                                        : "%.17e";  // %e always yields a float literal
            (శూన్య)VL_SNPRINTF(sbuf, bufsize, fmt, dnum);
            తిరిగి sbuf;
        }
    } లేకపోతే ఒకవేళ (isString()) {
        స్థిరరాశి string strg = గమ్యం_String();
        స్థిరరాశి string quoted
            = V3_బహిర్గతం_Formatter::quoteNameControls(strg, V3_బహిర్గతం_Formatter::Language::LA_C);
        // Note: putsQuoted does not track indentation, so we use this instead
        తిరిగి '"' + quoted + "\"s";
    } లేకపోతే ఒకవేళ (పదాలు() > 2) {
        // Note the double {{ initializer. The first { starts the initializer of the VlWide,
        // and the second starts the initializer of m_storage within the VlWide.
        // Alternative is to have constructor with std::initializer_list
        ఫలితం = "::VlWide<" + std::గమ్యం_string(పదాలు()) + ">{{";
        ఒకవేళ (పదాలు() > 4) ఫలితం += '\n';
        కొరకు (పూర్ణాంకం n = 0; n < పదాలు(); ++n) {
            ఒకవేళ (n) ఫలితం += ((n % 4) ? ", " : ",\n");
            (శూన్య)VL_SNPRINTF(sbuf, bufsize, "0x%08" PRIx32, edata_మాట(n));
            ఫలితం += sbuf;
        }
        ఒకవేళ (పదాలు() > 4) ఫలితం += '\n';
        ఫలితం += "}}";
    } లేకపోతే ఒకవేళ (పదాలు() == 2) {  // Quad
        స్థిరరాశి uint64_t qnum = static_cast<uint64_t>(గమ్యం_UQuad());
        స్థిరరాశి అక్షరం* స్థిరరాశి fmt = (qnum < 10) ? ("%" PRIx64 "ULL") : ("0x%016" PRIx64 "ULL");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (శూన్య)VL_SNPRINTF(sbuf, bufsize, fmt, qnum);
        తిరిగి sbuf;
    } లేకపోతే {
        // Always emit unsigned, if signed, will call correct signed functions
        // The 'U' must be here, to avoid <= comparisons etc ending up signed
        స్థిరరాశి uint32_t unum = గమ్యం_UInt();
        స్థిరరాశి అక్షరం* స్థిరరాశి fmt = (unum < 10)      ? ("%" PRIu32 "U")
                                : (వెడల్పు() > 16) ? ("0x%08" PRIx32 "U")
                                : (వెడల్పు() > 8)  ? ("0x%04" PRIx32 "U")
                                                 : ("0x%02" PRIx32 "U");
        // cppcheck-suppress wrongPrintfScanfArgNum
        (శూన్య)VL_SNPRINTF(sbuf, bufsize, fmt, unum);
        తిరిగి sbuf;
    }
    తిరిగి ఫలితం;
}

string V3_సంఖ్య::గమ్యం_Decimal_S() స్థిరరాశి VL_MT_STABLE {
    // Correct number of zero bits/width matters
    ఒకవేళ (isNegative()) {
        V3_సంఖ్య lhsNoSign = *ఇది;
        lhsNoSign.opNegate(*ఇది);
        తిరిగి "-"s + lhsNoSign.గమ్యం_Decimal_U();
    } లేకపోతే {
        తిరిగి గమ్యం_Decimal_U();
    }
}

string V3_సంఖ్య::గమ్యం_Decimal_U() స్థిరరాశి VL_MT_STABLE {
    స్థిరరాశి పూర్ణాంకం maxdecwidth = (వెడల్పు() + 3) * 4 / 3;

    // Or (maxdecwidth+7)/8], but can't have more than 4 BCD bits per word
    V3_సంఖ్య bcd(ఇది, maxdecwidth + 4);
    V3_సంఖ్య tmp(ఇది, maxdecwidth + 4);
    V3_సంఖ్య tmp2(ఇది, maxdecwidth + 4);

    పూర్ణాంకం from_బిట్టు = వెడల్పు() - 1;
    // Skip all leading zeros
    కొరకు (; from_బిట్టు >= 0 && బిట్టు_Is0(from_బిట్టు); --from_బిట్టు) {}
    // Double-dabble algorithm
    కొరకు (; from_బిట్టు >= 0; --from_బిట్టు) {
        // Any digits >= 5 need an add 3 (via tmp)
        కొరకు (పూర్ణాంకం nibble_బిట్టు = 0; nibble_బిట్టు < maxdecwidth; nibble_బిట్టు += 4) {
            ఒకవేళ (bcd.బిట్లు_విలువ(nibble_బిట్టు, 4) >= 5) {
                tmp2.సమితి_All_Bits0();
                tmp2.సమితి_బిట్టు(nibble_బిట్టు, 1);  // Add 3, decomposed as two bits
                tmp2.సమితి_బిట్టు(nibble_బిట్టు + 1, 1);
                tmp.op_కేటాయించు(bcd);
                bcd.op_కూడు(tmp, tmp2);
            }
        }
        // Shift; bcd = bcd << 1
        tmp.op_కేటాయించు(bcd);
        bcd.op_జరుపు_L(tmp, V3_సంఖ్య(ఇది, 32, 1));
        // bcd[0] = this[from_bit]
        ఒకవేళ (బిట్టు_Is1(from_బిట్టు)) bcd.సమితి_బిట్టు(0, 1);
    }

    string output;
    పూర్ణాంకం lsb = (maxdecwidth - 1) & ~3;
    కొరకు (; lsb > 0; lsb -= 4) {  // Skip leading zeros
        ఒకవేళ (bcd.బిట్లు_విలువ(lsb, 4)) ఆపు;
    }
    కొరకు (; lsb >= 0; lsb -= 4) {
        output += static_cast<అక్షరం>('0' + bcd.బిట్లు_విలువ(lsb, 4));  // 0..9
    }
    తిరిగి output;
}

string V3_సంఖ్య::గమ్యం_Real_String() స్థిరరాశి VL_MT_STABLE {
    స్థిరరాశి ద్విగుణ విలువ = గమ్యం_Double();
    // 15 digits suffice for most values, 17 digits for every value
    std::ostringstream బహిర్గతం;
    బహిర్గతం.precision(15);
    బహిర్గతం << విలువ;
    string ఫలితం = బహిర్గతం.str();
    ఒకవేళ (std::strtod(ఫలితం.c_str(), శూన్యసూచిక) != విలువ) {
        బహిర్గతం.str("");
        బహిర్గతం.precision(17);
        బహిర్గతం << విలువ;
        ఫలితం = బహిర్గతం.str();
    }
    // A real, as in Verilog, unless has an exponent, or is infinite or not-a-number
    ఒకవేళ (ఫలితం.కనుగొను_మొదటి_యొక్క(".en") == string::npos) ఫలితం += ".0";
    తిరిగి ఫలితం;
}

//======================================================================
// ACCESSORS - as numbers

తార్కిక V3_సంఖ్య::సరిపోతుంది_లోపల_UInt() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isFourState()) తిరిగి అబద్ధం;
    // We allow wide numbers that represent values <= 32 bits
    కొరకు (పూర్ణాంకం i = 1; i < పదాలు(); ++i) {
        ఒకవేళ (m_దత్తాంశం.అంకె()[i].m_విలువ) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}
uint32_t V3_సంఖ్య::గమ్యం_UInt() స్థిరరాశి VL_MT_సురక్షిత {
    UASSERT(!isFourState(), "toUInt with 4-state " << *ఇది);
    // We allow wide numbers that represent values <= 32 bits
    ఒకవేళ (VL_UNCOVERABLE(!సరిపోతుంది_లోపల_UInt())) {
        v3error("Value too wide for 32-bits expected in this context "  // LCOV_EXCL_LINE
                << *ఇది);
    }
    తిరిగి m_దత్తాంశం.అంకె()[0].m_విలువ;
}

ద్విగుణ V3_సంఖ్య::గమ్యం_Double() స్థిరరాశి VL_MT_సురక్షిత {
    UASSERT(isDouble() && వెడల్పు() == 64, "Real operation on wrong sized/non-real number");
    సంఘం {
        ద్విగుణ d;
        uint32_t u[2];
    } u;
    u.u[0] = m_దత్తాంశం.అంకె()[0].m_విలువ;
    u.u[1] = m_దత్తాంశం.అంకె()[1].m_విలువ;
    తిరిగి u.d;
}

int32_t V3_సంఖ్య::గమ్యం_SInt() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isSigned()) {
        స్థిరరాశి uint32_t v = గమ్యం_UInt();
        స్థిరరాశి uint32_t signExtend = (-(v & (1UL << (వెడల్పు() - 1))));
        స్థిరరాశి uint32_t extended = v | signExtend;
        తిరిగి static_cast<int32_t>(extended);
    } లేకపోతే {
        // Where we use this (widths, etc) and care about signedness,
        // we can reasonably assume the MSB isn't set on unsigned numbers.
        తిరిగి static_cast<int32_t>(గమ్యం_UInt());
    }
}

uint64_t V3_సంఖ్య::గమ్యం_UQuad() స్థిరరాశి VL_MT_సురక్షిత {
    UASSERT(!isFourState(), "toUQuad with 4-state " << *ఇది);
    // We allow wide numbers that represent values <= 64 bits
    ఒకవేళ (isDouble()) తిరిగి static_cast<uint64_t>(గమ్యం_Double());
    కొరకు (పూర్ణాంకం i = 2; i < పదాలు(); ++i) {
        ఒకవేళ (m_దత్తాంశం.అంకె()[i].m_విలువ) {
            v3error("Value too wide for 64-bits expected in this context " << *ఇది);
            ఆపు;
        }
    }
    ఒకవేళ (వెడల్పు() <= 32) తిరిగి (static_cast<uint64_t>(గమ్యం_UInt()));
    తిరిగి ((static_cast<uint64_t>(m_దత్తాంశం.అంకె()[1].m_విలువ) << 32ULL)
            | (static_cast<uint64_t>(m_దత్తాంశం.అంకె()[0].m_విలువ)));
}

int64_t V3_సంఖ్య::గమ్యం_SQuad() స్థిరరాశి {
    ఒకవేళ (isDouble()) తిరిగి static_cast<int64_t>(గమ్యం_Double());
    స్థిరరాశి uint64_t v = గమ్యం_UQuad();
    స్థిరరాశి uint64_t signExtend = (-(v & (1ULL << (వెడల్పు() - 1))));
    స్థిరరాశి uint64_t extended = v | signExtend;
    తిరిగి static_cast<int64_t>(extended);
}

string V3_సంఖ్య::గమ్యం_String() స్థిరరాశి {
    UASSERT(!isFourState(), "toString with 4-state " << *ఇది);
    // IEEE says \000 are removed
    // If used to print a %s space padding is still required after call to here
    ఒకవేళ (isString()) తిరిగి m_దత్తాంశం.str();
    పూర్ణాంకం బిట్టు_2 = వెడల్పు() - 1;
    అయితే ((బిట్టు_2 % 8) != 7) ++బిట్టు_2;
    std::string str;
    కొరకు (; బిట్టు_2 >= 0; బిట్టు_2 -= 8) {
        స్థిరరాశి పూర్ణాంకం v = బిట్లు_విలువ(బిట్టు_2 - 7, 8);
        ఒకవేళ (v) str += static_cast<అక్షరం>(v);
    }
    తిరిగి str;
}

uint32_t V3_సంఖ్య::edata_మాట(పూర్ణాంకం eword) స్థిరరాశి {
    UASSERT(!isFourState(), "edataWord with 4-state " << *ఇది);
    తిరిగి m_దత్తాంశం.అంకె()[eword].m_విలువ;
}

uint8_t V3_సంఖ్య::దత్తాంశం_Byte(పూర్ణాంకం byte) స్థిరరాశి {
    తిరిగి (edata_మాట(byte / (VL_EDATASIZE / 8)) >> ((byte * 8) % VL_EDATASIZE)) & 0xff;
}

తార్కిక V3_సంఖ్య::isAllZ() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isDouble() || isString()) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        స్థిరరాశి uint32_t ముసుగు = i == పదాలు() - 1 ? hi_మాట_ముసుగు() : ~0U;
        ఒకవేళ (((~v.m_విలువ & v.m_విలువ_X) & ముసుగు) != ముసుగు) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}
తార్కిక V3_సంఖ్య::isAllX() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isDouble() || isString()) తిరిగి అబద్ధం;
    uint32_t ముసుగు = hi_మాట_ముసుగు();
    కొరకు (పూర్ణాంకం i = పదాలు() - 1; i >= 0; --i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        ఒకవేళ ((v.m_విలువ & v.m_విలువ_X) ^ ముసుగు) తిరిగి అబద్ధం;
        ముసుగు = ~0U;
    }
    తిరిగి నిజం;
}
తార్కిక V3_సంఖ్య::isEqZero() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isString()) తిరిగి m_దత్తాంశం.str().ఖాళీలు();
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        ఒకవేళ (v.m_విలువ || v.m_విలువ_X) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}
తార్కిక V3_సంఖ్య::isNeqZero() స్థిరరాశి {
    ఒకవేళ (isString()) తిరిగి !m_దత్తాంశం.str().ఖాళీలు();
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        ఒకవేళ (v.m_విలువ & ~v.m_విలువ_X) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}
తార్కిక V3_సంఖ్య::is_బిట్లు_Zero(పూర్ణాంకం msb, పూర్ణాంకం lsb) స్థిరరాశి {
    కొరకు (పూర్ణాంకం i = lsb; i <= msb; ++i) {
        ఒకవేళ (VL_UNLIKELY(!బిట్టు_Is0(i))) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}
తార్కిక V3_సంఖ్య::isEqOne() స్థిరరాశి {
    ఒకవేళ (m_దత్తాంశం.అంకె()[0].m_విలువ != 1 || m_దత్తాంశం.అంకె()[0].m_విలువ_X) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 1; i < పదాలు(); ++i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        ఒకవేళ (v.m_విలువ || v.m_విలువ_X) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}
తార్కిక V3_సంఖ్య::isEqAllOnes(పూర్ణాంకం optwidth) స్థిరరాశి {
    // Correct number of zero bits/width matters
    ఒకవేళ (!optwidth) optwidth = వెడల్పు();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < optwidth; ++బిట్టు_2) {
        ఒకవేళ (!బిట్టు_Is1(బిట్టు_2)) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}
తార్కిక V3_సంఖ్య::isFourState() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isDouble() || isString()) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        ఒకవేళ (m_దత్తాంశం.అంకె()[i].m_విలువ_X) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}
తార్కిక V3_సంఖ్య::isAnyX() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isDouble() || isString()) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        ఒకవేళ (v.m_విలువ & v.m_విలువ_X) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}
తార్కిక V3_సంఖ్య::isAnyXZ() స్థిరరాశి {
    ఒకవేళ (isDouble() || isString()) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        ఒకవేళ (m_దత్తాంశం.అంకె()[i].m_విలువ_X) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}
తార్కిక V3_సంఖ్య::isAnyZ() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (isDouble() || isString()) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
        ఒకవేళ (~v.m_విలువ & v.m_విలువ_X) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}
తార్కిక V3_సంఖ్య::isLtXZ(స్థిరరాశి V3_సంఖ్య& rhs) స్థిరరాశి {
    // Include X/Z in comparisons for sort ordering
    ఒకవేళ (isString()) తిరిగి గమ్యం_String() < rhs.గమ్యం_String();
    ఒకవేళ (isDouble()) తిరిగి గమ్యం_Double() < rhs.గమ్యం_Double();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (బిట్టు_Is1(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) తిరిగి నిజం;
        ఒకవేళ (rhs.బిట్టు_Is1(బిట్టు_2) && బిట్టు_Is0(బిట్టు_2)) తిరిగి అబద్ధం;
        ఒకవేళ (బిట్టు_Is_XZ(బిట్టు_2)) తిరిగి నిజం;
        ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2)) తిరిగి అబద్ధం;
    }
    తిరిగి అబద్ధం;
}
పూర్ణాంకం V3_సంఖ్య::లెక్క_X(పూర్ణాంకం lsb, పూర్ణాంకం nbits) స్థిరరాశి VL_MT_సురక్షిత {
    పూర్ణాంకం లెక్క = 0;
    కొరకు (పూర్ణాంకం bitn = 0; bitn < nbits; ++bitn) {
        ఒకవేళ (lsb + bitn >= వెడల్పు()) తిరిగి లెక్క;
        ఒకవేళ (బిట్టు_Is_X(lsb + bitn)) ++లెక్క;
    }
    తిరిగి లెక్క;
}
పూర్ణాంకం V3_సంఖ్య::లెక్క_Z(పూర్ణాంకం lsb, పూర్ణాంకం nbits) స్థిరరాశి VL_MT_సురక్షిత {
    పూర్ణాంకం లెక్క = 0;
    కొరకు (పూర్ణాంకం bitn = 0; bitn < nbits; ++bitn) {
        ఒకవేళ (lsb + bitn >= వెడల్పు()) తిరిగి లెక్క;
        ఒకవేళ (బిట్టు_Is_Z(lsb + bitn)) ++లెక్క;
    }
    తిరిగి లెక్క;
}

పూర్ణాంకం V3_సంఖ్య::వెడల్పు_గమ్యం_Fit() స్థిరరాశి {
    కొరకు (పూర్ణాంకం బిట్టు_2 = వెడల్పు() - 1; బిట్టు_2 > 0; బిట్టు_2--) {
        ఒకవేళ (!బిట్టు_Is0(బిట్టు_2)) తిరిగి బిట్టు_2 + 1;
    }
    తిరిగి 1;  // one bit even if number is == 0
}

uint32_t V3_సంఖ్య::లెక్క_బిట్లు(స్థిరరాశి V3_సంఖ్య& ctrl) స్థిరరాశి {
    // Correct number of zero bits/width matters
    పూర్ణాంకం n = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఎంపిక (ctrl.బిట్టు_Is(0)) {
        స్థితి '0':
            ఒకవేళ (బిట్టు_Is0(బిట్టు_2)) ++n;
            ఆపు;
        స్థితి '1':
            ఒకవేళ (బిట్టు_Is1(బిట్టు_2)) ++n;
            ఆపు;
        స్థితి 'x':
            ఒకవేళ (బిట్టు_Is_X(బిట్టు_2)) ++n;
            ఆపు;
        స్థితి 'z':
            ఒకవేళ (బిట్టు_Is_Z(బిట్టు_2)) ++n;
            ఆపు;
        }
    }
    తిరిగి n;
}

uint32_t V3_సంఖ్య::లెక్క_బిట్లు(స్థిరరాశి V3_సంఖ్య& ctrl1, స్థిరరాశి V3_సంఖ్య& ctrl2,
                             స్థిరరాశి V3_సంఖ్య& ctrl3) స్థిరరాశి {
    // Correct number of zero bits/width matters
    పూర్ణాంకం n = లెక్క_బిట్లు(ctrl1);
    ఒకవేళ (ctrl2.బిట్టు_Is(0) != ctrl1.బిట్టు_Is(0)) n += లెక్క_బిట్లు(ctrl2);
    ఒకవేళ ((ctrl3.బిట్టు_Is(0) != ctrl1.బిట్టు_Is(0)) && (ctrl3.బిట్టు_Is(0) != ctrl2.బిట్టు_Is(0))) {
        n += లెక్క_బిట్లు(ctrl3);
    }
    తిరిగి n;
}

uint32_t V3_సంఖ్య::లెక్క_Ones() స్థిరరాశి {
    పూర్ణాంకం n = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (బిట్టు_Is1(బిట్టు_2)) ++n;
    }
    తిరిగి n;
}

uint32_t V3_సంఖ్య::most_సమితి_బిట్టు_P1() స్థిరరాశి {
    కొరకు (పూర్ణాంకం బిట్టు_2 = వెడల్పు() - 1; బిట్టు_2 >= 0; బిట్టు_2--) {
        ఒకవేళ (!బిట్టు_Is0(బిట్టు_2)) తిరిగి బిట్టు_2 + 1;
    }
    తిరిగి 0;
}

uint32_t V3_సంఖ్య::కనిష్ఠ_సమితి_బిట్టు_P1() స్థిరరాశి {
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (!బిట్టు_Is0(బిట్టు_2)) తిరిగి బిట్టు_2 + 1;
    }
    తిరిగి 0;
}

//======================================================================

V3_సంఖ్య& V3_సంఖ్య::op_బిట్లు_Non_XZ(స్థిరరాశి V3_సంఖ్య& lhs) {  // 0/1->1, X/Z->0
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2) || lhs.బిట్టు_Is1(బిట్టు_2)) సమితి_బిట్టు(బిట్టు_2, 1);
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_బిట్లు_One(స్థిరరాశి V3_సంఖ్య& lhs) {  // 1->1, 0/X/Z->0
    // op i, L(lhs) bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) సమితి_బిట్టు(బిట్టు_2, 1);
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_బిట్లు_XZ(స్థిరరాశి V3_సంఖ్య& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) సమితి_బిట్టు(బిట్టు_2, 1);
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_బిట్లు_Z(స్థిరరాశి V3_సంఖ్య& lhs) {  // 0/1->1, X/Z->0
    // op i, L(lhs) bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is_Z(బిట్టు_2)) సమితి_బిట్టు(బిట్టు_2, 1);
    }
    తిరిగి *ఇది;
}

//======================================================================
// Operators - Simple per-bit logical ops

V3_సంఖ్య& V3_సంఖ్య::opRedOr(స్థిరరాశి V3_సంఖ్య& lhs) {
    // op i, 1 bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    అక్షరం outc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < lhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            తిరిగి సమితి_Single_బిట్లు(1);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2)) {
        } లేకపోతే {
            outc = 'x';
        }
    }
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opRedAnd(స్థిరరాశి V3_సంఖ్య& lhs) {
    // Correct number of zero bits/width matters
    // op i, 1 bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    అక్షరం outc = 1;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < lhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2)) {
            తిరిగి సమితి_Single_బిట్లు(0);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
        } లేకపోతే {
            outc = 'x';
        }
    }
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opRedXor(స్థిరరాశి V3_సంఖ్య& lhs) {
    // op i, 1 bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    అక్షరం outc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < lhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            ఒకవేళ (outc == 1) {
                outc = 0;
            } లేకపోతే ఒకవేళ (outc == 0) {
                outc = 1;
            }
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2)) {
        } లేకపోతే {
            outc = 'x';
        }
    }
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::op_లెక్క_బిట్లు(స్థిరరాశి V3_సంఖ్య& సమాసం, స్థిరరాశి V3_సంఖ్య& ctrl1, స్థిరరాశి V3_సంఖ్య& ctrl2,
                                స్థిరరాశి V3_సంఖ్య& ctrl3) {
    // Correct number of zero bits/width matters
    అంకె_ASSERT_OP_ARGS4(సమాసం, ctrl1, ctrl2, ctrl3);
    అంకె_ASSERT_LOGIC_ARGS4(సమాసం, ctrl1, ctrl2, ctrl3);
    సమితి_Zero();
    m_దత్తాంశం.అంకె()[0].m_విలువ = సమాసం.లెక్క_బిట్లు(ctrl1, ctrl2, ctrl3);
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_లెక్క_Ones(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    ఒకవేళ (lhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    సమితి_Zero();
    m_దత్తాంశం.అంకె()[0].m_విలువ = lhs.లెక్క_Ones();
    opCleanThis();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opIsUnknown(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.isAnyXZ());
}
V3_సంఖ్య& V3_సంఖ్య::opOneHot(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    ఒకవేళ (lhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    తిరిగి సమితి_Single_బిట్లు(lhs.లెక్క_Ones() == 1);
}
V3_సంఖ్య& V3_సంఖ్య::opOneHot0(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    ఒకవేళ (lhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    తిరిగి సమితి_Single_బిట్లు(lhs.లెక్క_Ones() <= 1);
}
V3_సంఖ్య& V3_సంఖ్య::opCLog2(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    ఒకవేళ (lhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    // IE if 4, this algorithm didn't pre-subtract 1, so we need to post-correct now
    స్థిరరాశి పూర్ణాంకం adjust = (lhs.లెక్క_Ones() == 1) ? 0 : 1;
    కొరకు (పూర్ణాంకం బిట్టు_2 = lhs.వెడల్పు() - 1; బిట్టు_2 >= 0; బిట్టు_2--) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            సమితి_Long(బిట్టు_2 + adjust);
            తిరిగి *ఇది;
        }
    }
    సమితి_Zero();
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_Most_సమితి_బిట్టు_P1(స్థిరరాశి V3_సంఖ్య& lhs) {
    // Most-significant set bit plus one (bit-width / find-last-set); 0 if value is zero
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    ఒకవేళ (lhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    కొరకు (పూర్ణాంకం బిట్టు_2 = lhs.వెడల్పు() - 1; బిట్టు_2 >= 0; బిట్టు_2--) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            సమితి_Long(బిట్టు_2 + 1);
            తిరిగి *ఇది;
        }
    }
    సమితి_Zero();
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opLogNot(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    // op i, 1 bit return
    అక్షరం outc = 1;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < lhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            outc = 0;
            వెళ్ళు చివరి;
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2)) {
        } లేకపోతే {
            outc = 'x';
        }
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opNot(స్థిరరాశి V3_సంఖ్య& lhs) {
    // Correct number of zero bits/width matters
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    // op i, L(lhs) bit return
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 1);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 'x');
        }
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opAnd(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2) && rhs.బిట్టు_Is1(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 1);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2) || rhs.బిట్టు_Is0(బిట్టు_2)) {  // 0
        } లేకపోతే {
            సమితి_బిట్టు(బిట్టు_2, 'x');
        }
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opOr(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2) || rhs.బిట్టు_Is1(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 1);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) {
            // 0
        } లేకపోతే {
            సమితి_బిట్టు(బిట్టు_2, 'x');
        }
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opXor(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 1);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2) && rhs.బిట్టు_Is1(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 1);
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2) || rhs.బిట్టు_Is_XZ(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, 'x');
        }
        // else zero
    }
    తిరిగి *ఇది;
}

శూన్య V3_సంఖ్య::fill_బిట్లు(పూర్ణాంకం destLsb, పూర్ణాంకం వెడల్పు, అక్షరం విలువ) {
    UASSERT(destLsb >= 0 && వెడల్పు >= 0, "Negative bit range");
    ఒకవేళ (!వెడల్పు) తిరిగి;
    UASSERT(వెడల్పు <= ఇది->వెడల్పు() - destLsb, "Destination bit range exceeds number width");

    స్థిరరాశి uint32_t విలువ_మాట = విలువ == '1' || విలువ == 'x' || విలువ == 1 || విలువ == 3 ? ~0U : 0;
    స్థిరరాశి uint32_t x_మాట = విలువ == 'z' || విలువ == 'x' || విలువ == 2 || విలువ == 3 ? ~0U : 0;
    స్థిరరాశి పూర్ణాంకం మొదటి_మాట = VL_BITWORD_E(destLsb);
    స్థిరరాశి పూర్ణాంకం చివరి_మాట = VL_BITWORD_E(destLsb + వెడల్పు - 1);
    కొరకు (పూర్ణాంకం మాట = మొదటి_మాట; మాట <= చివరి_మాట; ++మాట) {
        స్థిరరాశి పూర్ణాంకం మాట_Lsb = మాట == మొదటి_మాట ? VL_BITBIT_E(destLsb) : 0;
        స్థిరరాశి పూర్ణాంకం మాట_Msb = మాట == చివరి_మాట ? VL_BITBIT_E(destLsb + వెడల్పు - 1) : VL_SIZEBITS_E;
        స్థిరరాశి uint32_t ముసుగు = VL_ముసుగు_E(మాట_Msb - మాట_Lsb + 1) << మాట_Lsb;
        విలువ_And_X& dest = m_దత్తాంశం.అంకె()[మాట];
        dest.m_విలువ = (dest.m_విలువ & ~ముసుగు) | (విలువ_మాట & ముసుగు);
        dest.m_విలువ_X = (dest.m_విలువ_X & ~ముసుగు) | (x_మాట & ముసుగు);
    }
}

శూన్య V3_సంఖ్య::copy_బిట్లు(పూర్ణాంకం destLsb, స్థిరరాశి V3_సంఖ్య& ఆధారం, పూర్ణాంకం ఆధారం_Lsb, పూర్ణాంకం వెడల్పు) {
    UASSERT(destLsb >= 0 && ఆధారం_Lsb >= 0 && వెడల్పు >= 0, "Negative bit range");
    ఒకవేళ (!వెడల్పు) తిరిగి;
    UASSERT(వెడల్పు <= ఇది->వెడల్పు() - destLsb, "Destination bit range exceeds number width");
    UASSERT(వెడల్పు <= ఆధారం.వెడల్పు() - ఆధారం_Lsb, "Source bit range exceeds number width");

    స్థిరరాశి పూర్ణాంకం మొదటి_మాట = VL_BITWORD_E(destLsb);
    స్థిరరాశి పూర్ణాంకం చివరి_మాట = VL_BITWORD_E(destLsb + వెడల్పు - 1);
    కొరకు (పూర్ణాంకం dest_మాట = మొదటి_మాట; dest_మాట <= చివరి_మాట; ++dest_మాట) {
        స్థిరరాశి పూర్ణాంకం dest_బిట్టు = std::max(destLsb, dest_మాట * VL_EDATASIZE);
        స్థిరరాశి పూర్ణాంకం dest_స్థానభ్రంశం = VL_BITBIT_E(dest_బిట్టు);
        స్థిరరాశి పూర్ణాంకం ఆధారం_బిట్టు = ఆధారం_Lsb + dest_బిట్టు - destLsb;
        స్థిరరాశి పూర్ణాంకం ఆధారం_మాట = VL_BITWORD_E(ఆధారం_బిట్టు);
        స్థిరరాశి పూర్ణాంకం ఆధారం_స్థానభ్రంశం = VL_BITBIT_E(ఆధారం_బిట్టు);
        స్థిరరాశి పూర్ణాంకం copy_వెడల్పు = std::min(వెడల్పు - (dest_బిట్టు - destLsb), VL_EDATASIZE - dest_స్థానభ్రంశం);
        స్థిరరాశి uint32_t ముసుగు = VL_ముసుగు_E(copy_వెడల్పు) << dest_స్థానభ్రంశం;

        స్థిరరాశి విలువ_And_X& ఆధారం_Lo = ఆధారం.m_దత్తాంశం.అంకె()[ఆధారం_మాట];
        uint32_t విలువ = ఆధారం_Lo.m_విలువ >> ఆధారం_స్థానభ్రంశం;
        uint32_t విలువ_X = ఆధారం_Lo.m_విలువ_X >> ఆధారం_స్థానభ్రంశం;
        ఒకవేళ (ఆధారం_స్థానభ్రంశం && copy_వెడల్పు > VL_EDATASIZE - ఆధారం_స్థానభ్రంశం) {
            స్థిరరాశి విలువ_And_X& ఆధారం_Hi = ఆధారం.m_దత్తాంశం.అంకె()[ఆధారం_మాట + 1];
            విలువ |= ఆధారం_Hi.m_విలువ << (VL_EDATASIZE - ఆధారం_స్థానభ్రంశం);
            విలువ_X |= ఆధారం_Hi.m_విలువ_X << (VL_EDATASIZE - ఆధారం_స్థానభ్రంశం);
        }

        విలువ_And_X& dest = m_దత్తాంశం.అంకె()[dest_మాట];
        dest.m_విలువ = (dest.m_విలువ & ~ముసుగు) | ((విలువ << dest_స్థానభ్రంశం) & ముసుగు);
        dest.m_విలువ_X = (dest.m_విలువ_X & ~ముసుగు) | ((విలువ_X << dest_స్థానభ్రంశం) & ముసుగు);
    }
}

V3_సంఖ్య& V3_సంఖ్య::opConcat(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Correct number of zero bits/width matters
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    సమితి_Zero();
    // See also error in V3Width
    ఒకవేళ (!lhs.sized() || !rhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
    }
    copy_బిట్లు(0, rhs, 0, rhs.వెడల్పు());
    copy_బిట్లు(rhs.వెడల్పు(), lhs, 0, lhs.వెడల్పు());
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opLenN(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    సమితి_Quad(lhs.గమ్యం_String().నిడివి());
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opRepl(స్థిరరాశి V3_సంఖ్య& lhs,
                           స్థిరరాశి V3_సంఖ్య& rhs) {  // rhs is # of times to replicate
    // Hopefully the using routine has a error check to, &rhso.
    // See also error in V3Width
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
    }
    తిరిగి opRepl(lhs, rhs.గమ్యం_UInt());
}

V3_సంఖ్య& V3_సంఖ్య::opRepl(స్థిరరాశి V3_సంఖ్య& lhs,
                           uint32_t rhsval) {  // rhs is # of times to replicate
    // i op repl, L(i)*value(rhs) bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    ఒకవేళ (v3Global.opt.replication_హద్దు()
        && rhsval > static_cast<uint32_t>(v3Global.opt.replication_హద్దు())) {
        v3warn(WIDTHCONCAT, "Replication of more that --replication-limit "
                                << v3Global.opt.replication_హద్దు() << " is suspect: " << rhsval);
    }
    సమితి_Zero();
    కొరకు (గుర్తులేని times = 0; times < rhsval; ++times) {
        స్థిరరాశి uint64_t destLsb = uint64_t{times} * lhs.వెడల్పు();
        ఒకవేళ (destLsb >= static_cast<uint32_t>(వెడల్పు())) ఆపు;
        copy_బిట్లు(static_cast<పూర్ణాంకం>(destLsb), lhs, 0,
                 std::min(lhs.వెడల్పు(), వెడల్పు() - static_cast<పూర్ణాంకం>(destLsb)));
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::op_ప్రవాహం_L(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    సమితి_Zero();
    // See also error in V3Width
    ఒకవేళ (!lhs.sized()) {
        v3warn(WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
    }
    // Slice size should never exceed the lhs width
    స్థిరరాశి పూర్ణాంకం ssize = std::min(rhs.గమ్యం_UInt(), static_cast<గుర్తులేని>(lhs.వెడల్పు()));
    కొరకు (పూర్ణాంకం istart = 0; istart < lhs.వెడల్పు(); istart += ssize) {
        స్థిరరాశి పూర్ణాంకం ostart = std::max(0, lhs.వెడల్పు() - ssize - istart);
        copy_బిట్లు(ostart, lhs, istart, std::min(ssize, lhs.వెడల్పు() - istart));
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opLogAnd(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    అక్షరం loutc = 0;
    అక్షరం routc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < lhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            loutc = 1;
            ఆపు;
        }
        ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2) && loutc == 0) loutc = 'x';
    }
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < rhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (rhs.బిట్టు_Is1(బిట్టు_2)) {
            routc = 1;
            ఆపు;
        }
        ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2) && routc == 0) routc = 'x';
    }
    అక్షరం outc = 'x';
    ఒకవేళ (routc == 1 && loutc == 1) outc = 1;
    ఒకవేళ (routc == 0 || loutc == 0) outc = 0;
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opLogOr(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    అక్షరం outc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < lhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2)) {
            outc = 1;
            వెళ్ళు చివరి;
        }
        ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2) && outc == 0) outc = 'x';
    }
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < rhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (rhs.బిట్టు_Is1(బిట్టు_2)) {
            outc = 1;
            వెళ్ళు చివరి;
        }
        ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2) && outc == 0) outc = 'x';
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opLogIf(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    V3_సంఖ్య lnot = lhs;
    lnot.opLogNot(lhs);
    తిరిగి opLogOr(lnot, rhs);
}

V3_సంఖ్య& V3_సంఖ్య::opLogEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to
    // X/Z extend. Use definition in IEEE to do this for us.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    V3_సంఖ్య ifa = lhs;
    ifa.opLogIf(lhs, rhs);
    V3_సంఖ్య ifb = rhs;
    ifb.opLogIf(rhs, lhs);
    తిరిగి opLogAnd(ifa, ifb);
}

V3_సంఖ్య& V3_సంఖ్య::opAtoN(స్థిరరాశి V3_సంఖ్య& lhs, పూర్ణాంకం పునాది) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    UASSERT(పునాది == AstAtoN::ATOREAL || పునాది == 2 || పునాది == 8 || పునాది == 10 || పునాది == 16,
            "base must be one of AstAtoN::ATOREAL, 2, 8, 10, or 16.");

    std::string str = lhs.గమ్యం_String();  // new instance to edit later

    // IEEE 1800-2023 6.16.9 says '_' may exist.
    str.erase(std::remove(str.begin(), str.చివర(), '_'), str.చివర());

    ఒకవేళ (పునాది == AstAtoN::ATOREAL) తిరిగి సమితి_Double(std::atof(str.c_str()));

    errno = 0;
    స్వయంచాలక v = std::strtol(str.c_str(), శూన్యసూచిక, పునాది);
    ఒకవేళ (errno != 0) v = 0;
    తిరిగి సమితి_Long_S(static_cast<int32_t>(v));
}

V3_సంఖ్య& V3_సంఖ్య::opPutcN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, స్థిరరాశి V3_సంఖ్య& ths) {
    అంకె_ASSERT_OP_ARGS3(lhs, rhs, ths);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    string lstring = lhs.గమ్యం_String();
    స్థిరరాశి int32_t i = rhs.గమ్యం_SInt();
    స్థిరరాశి int32_t c = ths.గమ్యం_SInt() & 0xFF;
    // 6.16.2:str.putc(i, c) does not change the value when i < 0 || i >= str.len() || c == 0
    // when evaluating the second condition, i must be positive.
    ఒకవేళ (0 <= i && static_cast<uint32_t>(i) < lstring.నిడివి() && c != 0) lstring[i] = c;
    తిరిగి సమితి_String(lstring);
}

V3_సంఖ్య& V3_సంఖ్య::opGetcN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    స్థిరరాశి string lstring = lhs.గమ్యం_String();
    స్థిరరాశి int32_t i = rhs.గమ్యం_SInt();
    int32_t v = 0;
    // 6.16.3:str.getc(i) returns 0 if i < 0 || i >= str.len()
    // when evaluating the second condition, i must be positive.
    ఒకవేళ (0 <= i && static_cast<uint32_t>(i) < lstring.నిడివి()) v = lstring[i];
    తిరిగి సమితి_Long(v);
}

V3_సంఖ్య& V3_సంఖ్య::opSubstrN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, స్థిరరాశి V3_సంఖ్య& ths) {
    అంకె_ASSERT_OP_ARGS3(lhs, rhs, ths);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    స్థిరరాశి string lstring = lhs.గమ్యం_String();
    స్థిరరాశి int32_t i = rhs.గమ్యం_SInt();
    స్థిరరాశి int32_t j = ths.గమ్యం_SInt();
    // 6.16.8:str.substr(i, j) returns an empty string when i < 0 || j < i || j >= str.len()
    // when evaluating the third condition, j must be positive because 0 <= i <= j is guaranteed by
    // the former two conditions.
    ఒకవేళ (i < 0 || j < i || static_cast<uint32_t>(j) >= lstring.నిడివి()) తిరిగి సమితి_String("");
    // The second parameter of std::string::substr(i, n) is length, not position as SystemVerilog.
    తిరిగి సమితి_String(lstring.substr(i, j - i + 1));
}

V3_సంఖ్య& V3_సంఖ్య::opCompareNN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, తార్కిక ignoreCase) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    // SystemVerilog Language Standard does not allow a string variable to contain '\0'.
    // So C functions such as strcmp() can correctly compare strings.
    పూర్ణాంకం ఫలితం;
    స్థిరరాశి string lstring = lhs.గమ్యం_String();
    స్థిరరాశి string rstring = rhs.గమ్యం_String();
    ఒకవేళ (ignoreCase) {
        ఫలితం = VL_STRCASECMP(lstring.c_str(), rstring.c_str());
    } లేకపోతే {
        ఫలితం = std::strcmp(lstring.c_str(), rstring.c_str());
    }
    తిరిగి సమితి_Long_S(ఫలితం);
}

V3_సంఖ్య& V3_సంఖ్య::opEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isString()) తిరిగి opEqN(lhs, rhs);
    ఒకవేళ (lhs.isDouble()) తిరిగి opEqD(lhs, rhs);
    అక్షరం outc = 1;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) {
            outc = 0;
            వెళ్ళు చివరి;
        }
        ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2) && rhs.బిట్టు_Is1(బిట్టు_2)) {
            outc = 0;
            వెళ్ళు చివరి;
        }
        ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
        ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opNeq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isString()) తిరిగి opNeqN(lhs, rhs);
    ఒకవేళ (lhs.isDouble()) తిరిగి opNeqD(lhs, rhs);
    అక్షరం outc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) {
            outc = 1;
            వెళ్ళు చివరి;
        }
        ఒకవేళ (lhs.బిట్టు_Is0(బిట్టు_2) && rhs.బిట్టు_Is1(బిట్టు_2)) {
            outc = 1;
            వెళ్ళు చివరి;
        }
        ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
        ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3Hash V3_సంఖ్య::గమ్యం_Hash() స్థిరరాశి {
    // Shouldmatch isCaseEq
    ఒకవేళ (isString()) తిరిగి V3Hash{గమ్యం_String()};
    ఒకవేళ (isDouble()) తిరిగి V3Hash{గమ్యం_Double()};
    V3Hash hash{వెడల్పు()};
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) hash += m_దత్తాంశం.అంకె()[i].m_విలువ;
    తిరిగి hash;
}

తార్కిక V3_సంఖ్య::isCaseEq(స్థిరరాశి V3_సంఖ్య& rhs) స్థిరరాశి {
    // Should match toHash
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    ఒకవేళ (isString()) తిరిగి గమ్యం_String() == rhs.గమ్యం_String();
    ఒకవేళ (isDouble()) తిరిగి గమ్యం_Double() == rhs.గమ్యం_Double();
    ఒకవేళ (వెడల్పు() != rhs.వెడల్పు()) తిరిగి అబద్ధం;
    కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
        ఒకవేళ (!(m_దత్తాంశం.అంకె()[i] == rhs.m_దత్తాంశం.అంకె()[i])) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}

V3_సంఖ్య& V3_సంఖ్య::opCaseEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.isCaseEq(rhs) ? 1 : 0);
}

V3_సంఖ్య& V3_సంఖ్య::opCaseNeq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అక్షరం outc = 0;
    ఒకవేళ (lhs.isString()) {
        తిరిగి opNeqN(lhs, rhs);
    } లేకపోతే ఒకవేళ (lhs.isDouble()) {
        తిరిగి opNeqD(lhs, rhs);
    }
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is(బిట్టు_2) != rhs.బిట్టు_Is(బిట్టు_2)) {
            outc = 1;
            వెళ్ళు చివరి;
        }
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opWildEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    అక్షరం outc = 1;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (!rhs.బిట్టు_Is_XZ(బిట్టు_2)) {
            ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) {
                outc = 'x';
                వెళ్ళు చివరి;
            }
            ఒకవేళ (lhs.బిట్టు_Is(బిట్టు_2) != rhs.బిట్టు_Is(బిట్టు_2)) {
                outc = 0;
                వెళ్ళు చివరి;
            }
        }
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opWildNeq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    అక్షరం outc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (!rhs.బిట్టు_Is_XZ(బిట్టు_2)) {
            ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) {
                outc = 'x';
                వెళ్ళు చివరి;
            }
            ఒకవేళ (lhs.బిట్టు_Is(బిట్టు_2) != rhs.బిట్టు_Is(బిట్టు_2)) {
                outc = 1;
                వెళ్ళు చివరి;
            }
        }
    }
చివరి:
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opGt(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    అక్షరం outc = 0;
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు(), rhs.వెడల్పు()); ++బిట్టు_2) {
        ఒకవేళ (lhs.బిట్టు_Is1(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) outc = 1;
        ఒకవేళ (rhs.బిట్టు_Is1(బిట్టు_2) && lhs.బిట్టు_Is0(బిట్టు_2)) outc = 0;
        ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
        ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
    }
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opGtS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    అక్షరం outc = 0;
    {
        స్థిరరాశి పూర్ణాంకం mbit = std::max(lhs.వెడల్పు() - 1, rhs.వెడల్పు() - 1);
        ఒకవేళ (lhs.బిట్టు_Is_XZ(mbit)) {
            outc = 'x';
        } లేకపోతే ఒకవేళ (rhs.బిట్టు_Is_XZ(mbit)) {
            outc = 'x';
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is0(mbit) && rhs.బిట్టు_Is1_Extend(mbit)) {
            outc = 1;  // + > -
        } లేకపోతే ఒకవేళ (lhs.బిట్టు_Is1_Extend(mbit) && rhs.బిట్టు_Is0(mbit)) {
            outc = 0;  // - !> +
        } లేకపోతే {
            // both positive or negative, normal >
            కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < std::max(lhs.వెడల్పు() - 1, rhs.వెడల్పు() - 1); ++బిట్టు_2) {
                ఒకవేళ (lhs.బిట్టు_Is1_Extend(బిట్టు_2) && rhs.బిట్టు_Is0(బిట్టు_2)) outc = 1;
                ఒకవేళ (rhs.బిట్టు_Is1_Extend(బిట్టు_2) && lhs.బిట్టు_Is0(బిట్టు_2)) outc = 0;
                ఒకవేళ (lhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
                ఒకవేళ (rhs.బిట్టు_Is_XZ(బిట్టు_2)) outc = 'x';
            }
        }
    }
    తిరిగి సమితి_Single_బిట్లు(outc);
}

V3_సంఖ్య& V3_సంఖ్య::opGte(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    V3_సంఖ్య& eq = opEq(lhs, rhs);
    ఒకవేళ (eq.isNeqZero()) తిరిగి eq;  // Return true
    తిరిగి opGt(lhs, rhs);
}

V3_సంఖ్య& V3_సంఖ్య::opGteS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, 1 bit return, max(L(lhs),L(rhs)) calculation, careful need to X/Z extend.
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    V3_సంఖ్య& eq = opEq(lhs, rhs);
    ఒకవేళ (eq.isNeqZero()) తిరిగి eq;  // Return true
    తిరిగి opGtS(lhs, rhs);
}

V3_సంఖ్య& V3_సంఖ్య::opLt(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { తిరిగి opGt(rhs, lhs); }
V3_సంఖ్య& V3_సంఖ్య::opLte(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { తిరిగి opGte(rhs, lhs); }
V3_సంఖ్య& V3_సంఖ్య::opLtS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { తిరిగి opGtS(rhs, lhs); }
V3_సంఖ్య& V3_సంఖ్య::opLteS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) { తిరిగి opGteS(rhs, lhs); }

V3_సంఖ్య& V3_సంఖ్య::op_జరుపు_R(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // L(lhs) bit return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    సమితి_Zero();
    ఒకవేళ (rhs.వెడల్పు() > 32 && !rhs.is_బిట్లు_Zero(rhs.వెడల్పు() - 1, 32)) తిరిగి *ఇది;
    స్థిరరాశి uint32_t rhsval = rhs.గమ్యం_UInt();
    ఒకవేళ (rhsval < static_cast<uint32_t>(lhs.వెడల్పు())) {
        copy_బిట్లు(0, lhs, static_cast<పూర్ణాంకం>(rhsval),
                 std::min(వెడల్పు(), lhs.వెడల్పు() - static_cast<పూర్ణాంకం>(rhsval)));
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::op_జరుపు_RS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, uint32_t lbits) {
    // Correct number of zero bits/width matters (hence lbits passed)
    // L(lhs) bit return
    // The spec says a unsigned >>> still acts as a normal >>.
    // We presume it is signed; as that's V3Width's job to convert to opShiftR
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    సమితి_Zero();
    స్థిరరాశి తార్కిక అధికప్రవాహం = rhs.వెడల్పు() > 32 && !rhs.is_బిట్లు_Zero(rhs.వెడల్పు() - 1, 32);
    ఒకవేళ (!అధికప్రవాహం) {
        స్థిరరాశి uint32_t rhsval = rhs.గమ్యం_UInt();
        ఒకవేళ (rhsval < lbits) {
            స్థిరరాశి పూర్ణాంకం copy_వెడల్పు = std::min(వెడల్పు(), static_cast<పూర్ణాంకం>(lbits - rhsval));
            copy_బిట్లు(0, lhs, static_cast<పూర్ణాంకం>(rhsval), copy_వెడల్పు);
            fill_బిట్లు(copy_వెడల్పు, వెడల్పు() - copy_వెడల్పు, lhs.బిట్టు_Is(lbits - 1));
            తిరిగి *ఇది;
        }
    }
    fill_బిట్లు(0, వెడల్పు(), lhs.బిట్టు_Is(lbits - 1));  // '0/'1/'x/'z
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::op_జరుపు_L(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // L(lhs) bit return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    సమితి_Zero();
    ఒకవేళ (rhs.వెడల్పు() > 32 && !rhs.is_బిట్లు_Zero(rhs.వెడల్పు() - 1, 32)) తిరిగి *ఇది;
    స్థిరరాశి uint32_t rhsval = rhs.గమ్యం_UInt();
    ఒకవేళ (rhsval < static_cast<uint32_t>(వెడల్పు())) {
        copy_బిట్లు(static_cast<పూర్ణాంకం>(rhsval), lhs, 0,
                 std::min(వెడల్పు() - static_cast<పూర్ణాంకం>(rhsval), lhs.వెడల్పు()));
    }
    తిరిగి *ఇది;
}

//======================================================================
// Ops - Arithmetic

V3_సంఖ్య& V3_సంఖ్య::opNegate(స్థిరరాశి V3_సంఖ్య& lhs) {
    // Correct number of zero bits/width matters
    // op i, L(lhs) bit return
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    ఒకవేళ (lhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    V3_సంఖ్య notlhs(&lhs, వెడల్పు());
    notlhs.opNot(lhs);
    స్థిరరాశి V3_సంఖ్య one(&lhs, వెడల్పు(), 1);
    op_కూడు(notlhs, one);
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_కూడు(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    సమితి_Zero();
    // Addem
    uint64_t వాహకం = 0;
    కొరకు (పూర్ణాంకం మాట = 0; మాట < పదాలు(); ++మాట) {
        స్థిరరాశి uint64_t lwordval = lhs.m_దత్తాంశం.అంకె()[మాట].m_విలువ;
        స్థిరరాశి uint64_t rwordval = rhs.m_దత్తాంశం.అంకె()[మాట].m_విలువ;
        స్థిరరాశి uint64_t కూడిక = lwordval + rwordval + వాహకం;
        m_దత్తాంశం.అంకె()[మాట].m_విలువ = కూడిక & 0xffffffffULL;
        వాహకం = కూడిక > 0xffffffffULL;
    }
    opCleanThis();  // Just in case it produced extra bits in result
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opSub(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Correct number of zero bits/width matters
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    V3_సంఖ్య negrhs(&rhs, వెడల్పు());
    negrhs.opNegate(rhs);
    తిరిగి op_కూడు(lhs, negrhs);
}
V3_సంఖ్య& V3_సంఖ్య::opMul(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    సమితి_Zero();
    ఒకవేళ (వెడల్పు() <= 64) {
        సమితి_Quad(lhs.గమ్యం_UQuad() * rhs.గమ్యం_UQuad());
        opCleanThis();  // Mult produces extra bits in result
    } లేకపోతే {
        కొరకు (పూర్ణాంకం lword = 0; lword < lhs.పదాలు(); ++lword) {
            స్థిరరాశి uint64_t lwordval = static_cast<uint64_t>(lhs.m_దత్తాంశం.అంకె()[lword].m_విలువ);
            ఒకవేళ (lwordval == 0) కొనసాగు;
            కొరకు (పూర్ణాంకం rword = 0; rword < rhs.పదాలు(); ++rword) {
                స్థిరరాశి uint64_t rwordval = static_cast<uint64_t>(rhs.m_దత్తాంశం.అంకె()[rword].m_విలువ);
                ఒకవేళ (rwordval == 0) కొనసాగు;
                uint64_t mul = lwordval * rwordval;
                కొరకు (పూర్ణాంకం qword = lword + rword; qword < పదాలు(); ++qword) {
                    mul += static_cast<uint64_t>(m_దత్తాంశం.అంకె()[qword].m_విలువ);
                    m_దత్తాంశం.అంకె()[qword].m_విలువ = (mul & 0xffffffffULL);
                    mul = (mul >> 32ULL) & 0xffffffffULL;
                    ఒకవేళ (mul == 0) ఆపు;
                }
            }
        }
        opCleanThis();  // Mult produces extra bits in result
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opMulS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Signed multiply
    // Correct number of zero bits/width matters
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    V3_సంఖ్య lhsNoSign = lhs;
    ఒకవేళ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_సంఖ్య rhsNoSign = rhs;
    ఒకవేళ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    స్థిరరాశి V3_సంఖ్య qNoSign = opMul(lhsNoSign, rhsNoSign);
    ఒకవేళ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } లేకపోతే {
        op_కేటాయించు(qNoSign);
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opDiv(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    // UINFO(9, "opdiv " << lhs << " " << rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    ఒకవేళ (rhs.isEqZero()) తిరిగి సమితి_All_బిట్లు_XRemoved();
    ఒకవేళ (lhs.వెడల్పు() <= 64) {
        సమితి_Quad(lhs.గమ్యం_UQuad() / rhs.గమ్యం_UQuad());
        తిరిగి *ఇది;
    } లేకపోతే {
        // Wide division
        తిరిగి opModDivGuts(lhs, rhs, అబద్ధం);
    }
}
V3_సంఖ్య& V3_సంఖ్య::opDivS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Signed divide
    // Correct number of zero bits/width matters
    // UINFO(9, ">>divs-start " << lhs << " " << rhs);
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    ఒకవేళ (rhs.isEqZero()) తిరిగి సమితి_All_బిట్లు_XRemoved();
    V3_సంఖ్య lhsNoSign = lhs;
    ఒకవేళ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_సంఖ్య rhsNoSign = rhs;
    ఒకవేళ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    స్థిరరాశి V3_సంఖ్య qNoSign = opDiv(lhsNoSign, rhsNoSign);
    // UINFO(9, " >divs-mid " << lhs << " " << rhs << " " << qNoSign);
    ఒకవేళ ((lhs.isNegative() && !rhs.isNegative()) || (!lhs.isNegative() && rhs.isNegative())) {
        opNegate(qNoSign);
    } లేకపోతే {
        op_కేటాయించు(qNoSign);
    }
    UINFO(9, " <divs-out " << lhs << " " << rhs << " =" << *ఇది);
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opModDiv(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // i op j, max(L(lhs),L(rhs)) bit return, if any 4-state, 4-state return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    ఒకవేళ (rhs.isEqZero()) తిరిగి సమితి_All_బిట్లు_XRemoved();
    ఒకవేళ (lhs.వెడల్పు() <= 64) {
        సమితి_Quad(lhs.గమ్యం_UQuad() % rhs.గమ్యం_UQuad());
        తిరిగి *ఇది;
    } లేకపోతే {
        // Wide modulus
        తిరిగి opModDivGuts(lhs, rhs, నిజం);
    }
}
V3_సంఖ్య& V3_సంఖ్య::opModDivS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Signed moddiv
    // Correct number of zero bits/width matters
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    ఒకవేళ (rhs.isEqZero()) తిరిగి సమితి_All_బిట్లు_XRemoved();
    V3_సంఖ్య lhsNoSign = lhs;
    ఒకవేళ (lhs.isNegative()) lhsNoSign.opNegate(lhs);
    V3_సంఖ్య rhsNoSign = rhs;
    ఒకవేళ (rhs.isNegative()) rhsNoSign.opNegate(rhs);
    స్థిరరాశి V3_సంఖ్య qNoSign = opModDiv(lhsNoSign, rhsNoSign);
    ఒకవేళ (lhs.isNegative()) {  // Just lhs' sign  (*DIFFERENT FROM PERL, which uses rhs sign*)
        opNegate(qNoSign);
    } లేకపోతే {
        op_కేటాయించు(qNoSign);
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opModDivGuts(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, తార్కిక is_modulus) {
    // See Knuth Algorithm D.  Computes u/v = q.r
    // This isn't massively tuned, as wide division is rare
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    సమితి_Zero();
    // Find MSB and check for zero.
    స్థిరరాశి పూర్ణాంకం l_పదాలు = lhs.పదాలు();
    స్థిరరాశి పూర్ణాంకం umsbp1 = lhs.most_సమితి_బిట్టు_P1();  // dividend
    స్థిరరాశి పూర్ణాంకం vmsbp1 = rhs.most_సమితి_బిట్టు_P1();  //భాజకం
    ఒకవేళ (VL_UNLIKELY(vmsbp1 == 0)  // rwp==0 so division by zero.  Return 0.
        || VL_UNLIKELY(umsbp1 == 0)) {  // 0/x so short circuit and return 0
        UINFO(9, "  opmoddiv-zero " << lhs << " " << rhs << " now=" << *ఇది);
        తిరిగి *ఇది;
    }

    స్థిరరాశి పూర్ణాంకం uw = (umsbp1 + 31) / 32;  // aka "m" in the algorithm
    స్థిరరాశి పూర్ణాంకం vw = (vmsbp1 + 31) / 32;  // aka "n" in the algorithm

    ఒకవేళ (vw == 1) {  // Single divisor word breaks rest of algorithm
        uint64_t k = 0;
        కొరకు (పూర్ణాంకం j = uw - 1; j >= 0; j--) {
            స్థిరరాశి uint64_t unw64
                = ((k << 32ULL) + static_cast<uint64_t>(lhs.m_దత్తాంశం.అంకె()[j].m_విలువ));
            m_దత్తాంశం.అంకె()[j].m_విలువ = unw64 / static_cast<uint64_t>(rhs.m_దత్తాంశం.అంకె()[0].m_విలువ);
            k = unw64
                - (static_cast<uint64_t>(m_దత్తాంశం.అంకె()[j].m_విలువ)
                   * static_cast<uint64_t>(rhs.m_దత్తాంశం.అంకె()[0].m_విలువ));
        }
        UINFO(9, "  opmoddiv-1w  " << lhs << " " << rhs << " q=" << *ఇది << " rem=0x" << std::hex
                                   << k << std::dec);
        ఒకవేళ (is_modulus) {
            సమితి_Zero();
            m_దత్తాంశం.అంకె()[0].m_విలువ = k;
        }
        opCleanThis();
        తిరిగి *ఇది;
    }

    // +1 word as we may shift during normalization
    uint32_t un[VL_MULS_MAX_పదాలు + 1];  // Fixed size, as MSVC++ doesn't allow [words] here
    uint32_t vn[VL_MULS_MAX_పదాలు + 1];  // v normalized

    // Zero for ease of debugging and to save having to zero for shifts
    కొరకు (పూర్ణాంకం i = 0; i < l_పదాలు; ++i) m_దత్తాంశం.అంకె()[i].m_విలువ = 0;
    కొరకు (పూర్ణాంకం i = 0; i < l_పదాలు + 1; ++i) { un[i] = vn[i] = 0; }  // +1 as vn may get extra word

    // Algorithm requires divisor MSB to be set
    // Copy and shift to normalize divisor so MSB of vn[vw-1] is set
    స్థిరరాశి పూర్ణాంకం s = 31 - ((vmsbp1 - 1) & 31);  // shift amount (0...31)
    స్థిరరాశి uint32_t జరుపు_ముసుగు = s ? 0xffffffff : 0;  // otherwise >> 32 won't mask the value
    కొరకు (పూర్ణాంకం i = vw - 1; i > 0; i--) {
        vn[i] = (rhs.m_దత్తాంశం.అంకె()[i].m_విలువ << s)
                | (జరుపు_ముసుగు & (rhs.m_దత్తాంశం.అంకె()[i - 1].m_విలువ >> (32 - s)));
    }
    vn[0] = rhs.m_దత్తాంశం.అంకె()[0].m_విలువ << s;

    // Copy and shift dividend by same amount; may set new upper word
    ఒకవేళ (s) {
        un[uw] = lhs.m_దత్తాంశం.అంకె()[uw - 1].m_విలువ >> (32 - s);
    } లేకపోతే {
        un[uw] = 0;
    }
    కొరకు (పూర్ణాంకం i = uw - 1; i > 0; i--) {
        un[i] = (lhs.m_దత్తాంశం.అంకె()[i].m_విలువ << s)
                | (జరుపు_ముసుగు & (lhs.m_దత్తాంశం.అంకె()[i - 1].m_విలువ >> (32 - s)));
    }
    un[0] = lhs.m_దత్తాంశం.అంకె()[0].m_విలువ << s;

    // printf("  un="); for (int i=5; i>=0; i--) printf(" %08x",un[i]); printf("\n");
    // printf("  vn="); for (int i=5; i>=0; i--) printf(" %08x",vn[i]); printf("\n");
    // printf("  mv="); for (int i=5; i>=0; i--) printf(" %08x",m_value[i]); printf("\n");

    // Main loop
    కొరకు (పూర్ణాంకం j = uw - vw; j >= 0; j--) {
        // Estimate
        స్థిరరాశి uint64_t unw64
            = (static_cast<uint64_t>(un[j + vw]) << 32ULL | static_cast<uint64_t>(un[j + vw - 1]));
        uint64_t qhat = unw64 / static_cast<uint64_t>(vn[vw - 1]);
        uint64_t rhat = unw64 - qhat * static_cast<uint64_t>(vn[vw - 1]);

    again:
        ఒకవేళ (qhat >= 0x100000000ULL || ((qhat * vn[vw - 2]) > ((rhat << 32ULL) + un[j + vw - 2]))) {
            qhat = qhat - 1;
            rhat = rhat + vn[vw - 1];
            ఒకవేళ (rhat < 0x100000000ULL) వెళ్ళు again;
        }

        int64_t t = 0;  // Must be signed
        uint64_t k = 0;
        కొరకు (పూర్ణాంకం i = 0; i < vw; ++i) {
            స్థిరరాశి uint64_t p = qhat * vn[i];  // Multiply by estimate
            t = un[i + j] - k - (p & 0xFFFFFFFFULL);  //వ్యవకలనం
            un[i + j] = t;
            k = (p >> 32ULL) - (t >> 32ULL);
        }
        t = un[j + vw] - k;
        un[j + vw] = t;
        m_దత్తాంశం.అంకె()[j].m_విలువ = qhat;  // Save quotient digit

        ఒకవేళ (t < 0) {
            // Over subtracted; correct by adding back
            m_దత్తాంశం.అంకె()[j].m_విలువ--;
            k = 0;
            కొరకు (పూర్ణాంకం i = 0; i < vw; ++i) {
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

    ఒకవేళ (is_modulus) {  // modulus
        // Need to reverse normalization on copy to output
        కొరకు (పూర్ణాంకం i = 0; i < vw; ++i) {
            m_దత్తాంశం.అంకె()[i].m_విలువ = (un[i] >> s) | (జరుపు_ముసుగు & (un[i + 1] << (32 - s)));
        }
        కొరకు (పూర్ణాంకం i = vw; i < l_పదాలు; ++i) m_దత్తాంశం.అంకె()[i].m_విలువ = 0;
        opCleanThis();
        UINFO(9, "  opmoddiv-mod " << lhs << " " << rhs << " now=" << *ఇది);
        తిరిగి *ఇది;
    } లేకపోతే {  // division
        opCleanThis();
        UINFO(9, "  opmoddiv-div " << lhs << " " << rhs << " now=" << *ఇది);
        తిరిగి *ఇది;
    }
}

V3_సంఖ్య& V3_సంఖ్య::opPow(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, తార్కిక lsign, తార్కిక rsign) {
    // Correct number of zero bits/width matters
    // L(i) bit return, if any 4-state, 4-state return
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_LOGIC_ARGS2(lhs, rhs);
    ఒకవేళ (lhs.isFourState() || rhs.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    ఒకవేళ (rhs.isEqZero()) తిరిగి సమితి_Value1();  // Overrides lhs 0 -> return 1
    // We may want to special case when the lhs is 2, so we can get larger outputs
    ఒకవేళ (rsign && rhs.isNegative()) {
        ఒకవేళ (lhs.isEqZero()) {
            తిరిగి సమితి_All_బిట్లు_XRemoved();
        } లేకపోతే ఒకవేళ (lhs.isEqOne()) {
            తిరిగి సమితి_Value1();
        } లేకపోతే ఒకవేళ (lsign && lhs.isEqAllOnes()) {
            ఒకవేళ (rhs.బిట్టు_Is1(0)) {
                తిరిగి సమితి_All_Bits1();  // -1^odd=-1
            } లేకపోతే {
                తిరిగి సమితి_Value1();  // -1^even=1
            }
        }
        తిరిగి సమితి_Zero();
    }
    ఒకవేళ (lhs.isEqZero()) తిరిగి సమితి_Zero();
    సమితి_Zero();
    m_దత్తాంశం.అంకె()[0].m_విలువ = 1;
    V3_సంఖ్య power(&lhs, వెడల్పు());
    power.op_కేటాయించు(lhs);
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < rhs.వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (బిట్టు_2 > 0) {  // power = power*power
            V3_సంఖ్య చివరి_Power(&lhs, వెడల్పు());
            చివరి_Power.op_కేటాయించు(power);
            power.opMul(చివరి_Power, చివరి_Power);
        }
        ఒకవేళ (rhs.బిట్టు_Is1(బిట్టు_2)) {  // out *= power
            V3_సంఖ్య చివరి_బహిర్గతం(&lhs, వెడల్పు());
            చివరి_బహిర్గతం.op_కేటాయించు(*ఇది);
            opMul(చివరి_బహిర్గతం, power);
            // UINFO(0, "pow "<<lhs<<" "<<rhs<<" b"<<bit<<" pow="<<power<<" now="<<*this);
        }
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::opPowSU(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Correct number of zero bits/width matters
    తిరిగి opPow(lhs, rhs, నిజం, అబద్ధం);
}
V3_సంఖ్య& V3_సంఖ్య::opPowSS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Correct number of zero bits/width matters
    తిరిగి opPow(lhs, rhs, నిజం, నిజం);
}
V3_సంఖ్య& V3_సంఖ్య::opPowUS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // Correct number of zero bits/width matters
    తిరిగి opPow(lhs, rhs, అబద్ధం, నిజం);
}

V3_సంఖ్య& V3_సంఖ్య::opBufIf1(స్థిరరాశి V3_సంఖ్య& ens, స్థిరరాశి V3_సంఖ్య& if1s) {
    అంకె_ASSERT_OP_ARGS2(ens, if1s);
    అంకె_ASSERT_LOGIC_ARGS2(ens, if1s);
    సమితి_Zero();
    కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 < వెడల్పు(); ++బిట్టు_2) {
        ఒకవేళ (ens.బిట్టు_Is1(బిట్టు_2)) {
            సమితి_బిట్టు(బిట్టు_2, if1s.బిట్టు_Is(బిట్టు_2));
        } లేకపోతే {
            సమితి_బిట్టు(బిట్టు_2, 'z');
        }
    }
    తిరిగి *ఇది;
}

// Sets all bits in range to the given value
V3_సంఖ్య& V3_సంఖ్య::op_సమితి_Range(uint32_t lsb, uint32_t వెడల్పు, అక్షరం బిట్టు_విలువ) {
    ఒకవేళ (lsb < static_cast<uint32_t>(ఇది->వెడల్పు())) {
        fill_బిట్లు(static_cast<పూర్ణాంకం>(lsb),
                 static_cast<పూర్ణాంకం>(std::min(వెడల్పు, static_cast<uint32_t>(ఇది->వెడల్పు()) - lsb)),
                 బిట్టు_విలువ);
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::op_కేటాయించు(స్థిరరాశి V3_సంఖ్య& lhs) { తిరిగి op_కేటాయించు_Non_XZ(lhs, అబద్ధం); }
V3_సంఖ్య& V3_సంఖ్య::op_కేటాయించు_Non_XZ(స్థిరరాశి V3_సంఖ్య& lhs, తార్కిక ignoreXZ) {
    // Note may be a width change during the assign.
    // Special case: opAssign unlike other ops, allows this an assignment
    // to itself; V3Simulate does this when hits "foo=foo;"
    // So no: NUM_ASSERT_OP_ARGS1(lhs);
    ఒకవేళ (ఇది != &lhs) {
        ఒకవేళ (VL_UNLIKELY(lhs.isNull())) {
            m_దత్తాంశం.m_isNull = నిజం;
        } లేకపోతే ఒకవేళ (isString()) {
            ఒకవేళ (VL_UNLIKELY(!lhs.isString())) {
                // Numbers can still be strings
                m_దత్తాంశం.str() = lhs.గమ్యం_String();
            } లేకపోతే {
                m_దత్తాంశం.str() = lhs.m_దత్తాంశం.str();
            }
        } లేకపోతే ఒకవేళ (VL_UNLIKELY(lhs.isString())) {
            // Non-compatible types, see also opAToN()
            సమితి_Zero();
        } లేకపోతే ఒకవేళ (lhs.isDouble()) {
            సమితి_Double(lhs.గమ్యం_Double());
        } లేకపోతే ఒకవేళ (!ignoreXZ) {
            సమితి_Zero();
            copy_బిట్లు(0, lhs, 0, std::min(వెడల్పు(), lhs.వెడల్పు()));
        } లేకపోతే {
            కొరకు (పూర్ణాంకం మాట = 0; మాట < పదాలు(); ++మాట) {
                స్థిరరాశి uint32_t విలువ = మాట < lhs.పదాలు() ? lhs.m_దత్తాంశం.అంకె()[మాట].m_విలువ
                                                                & ~lhs.m_దత్తాంశం.అంకె()[మాట].m_విలువ_X
                                                          : 0;
                m_దత్తాంశం.అంకె()[మాట] = {విలువ, 0};
            }
            opCleanThis();
        }
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opNToI(స్థిరరాశి V3_సంఖ్య& lhs) {
    // String to packed number
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    సమితి_Zero();
    స్థిరరాశి string& str = lhs.గమ్యం_String();
    కొరకు (కొలత_t n = 0; n < str.నిడివి(); ++n) {
        స్థిరరాశి అక్షరం c = str[str.నిడివి() - 1 - n];
        కొరకు (కొలత_t cbit = 0; cbit < 8; ++cbit) {
            స్థిరరాశి std::bitset<8> sbit{static_cast<గుర్తులేని దీర్ఘ దీర్ఘ>(c)};
            సమితి_బిట్టు(n * 8 + cbit, sbit.test(cbit) ? 1 : 0);
        }
    }
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opExtendS(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t lbits) {
    // Note may be a width change during the sign extension
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    స్థిరరాశి పూర్ణాంకం copy_వెడల్పు = std::min(వెడల్పు(), static_cast<పూర్ణాంకం>(lbits));
    copy_బిట్లు(0, lhs, 0, copy_వెడల్పు);
    fill_బిట్లు(copy_వెడల్పు, వెడల్పు() - copy_వెడల్పు, lhs.బిట్టు_Is(lbits - 1));
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opExtendXZ(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t lbits) {
    // Note may be a width change during the X/Z extension
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    స్థిరరాశి పూర్ణాంకం copy_వెడల్పు = std::min(వెడల్పు(), static_cast<పూర్ణాంకం>(lbits));
    copy_బిట్లు(0, lhs, 0, copy_వెడల్పు);
    fill_బిట్లు(copy_వెడల్పు, వెడల్పు() - copy_వెడల్పు, lhs.బిట్టు_Is(lbits - 1));
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opClean(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t బిట్లు) { తిరిగి opSel(lhs, బిట్లు - 1, 0); }

శూన్య V3_సంఖ్య::opCleanThis(తార్కిక warnOnTruncation) {
    // Clean MSB of number
    అంకె_ASSERT_LOGIC_ARGS1(*ఇది);
    స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[పదాలు() - 1];
    స్థిరరాశి uint32_t new_విలువ_Msb = v.m_విలువ & hi_మాట_ముసుగు();
    స్థిరరాశి uint32_t new_విలువ_XMsb = v.m_విలువ_X & hi_మాట_ముసుగు();
    ఒకవేళ (warnOnTruncation && (new_విలువ_Msb != v.m_విలువ || new_విలువ_XMsb != v.m_విలువ_X)) {
        // Displaying in decimal avoids hiWordMask truncation
        v3warn(WIDTHTRUNC,
               "Value too large for " << వెడల్పు() << " bit number: " << displayed("%d"));
    }
    m_దత్తాంశం.అంకె()[పదాలు() - 1] = {new_విలువ_Msb, new_విలువ_XMsb};
}

V3_సంఖ్య& V3_సంఖ్య::opSel(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& msb, స్థిరరాశి V3_సంఖ్య& lsb) {
    అంకె_ASSERT_OP_ARGS3(lhs, msb, lsb);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS2(msb, lsb);
    ఒకవేళ (lsb.isFourState() || msb.isFourState()) తిరిగి సమితి_All_బిట్లు_X();
    తిరిగి opSel(lhs, msb.గమ్యం_UInt(), lsb.గమ్యం_UInt());
}

V3_సంఖ్య& V3_సంఖ్య::opSel(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t msbval, uint32_t lsbval) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    సమితి_Zero();
    పూర్ణాంకం copy_వెడల్పు = 0;
    ఒకవేళ (lsbval <= msbval && lsbval < static_cast<uint32_t>(lhs.వెడల్పు())) {
        స్థిరరాశి uint64_t selected_వెడల్పు = uint64_t{msbval} - lsbval + 1;
        copy_వెడల్పు = static_cast<పూర్ణాంకం>(
            std::min({static_cast<uint64_t>(వెడల్పు()), static_cast<uint64_t>(lhs.వెడల్పు()) - lsbval,
                      selected_వెడల్పు}));
    }
    copy_బిట్లు(0, lhs, copy_వెడల్పు ? static_cast<పూర్ణాంకం>(lsbval) : 0, copy_వెడల్పు);
    fill_బిట్లు(copy_వెడల్పు, వెడల్పు() - copy_వెడల్పు, v3Global.constRemoveXs() ? 0 : 'x');
    // UINFO(0, "RANGE " << lhs << " " << msb << " " << lsb << " = " << *this);
    తిరిగి *ఇది;
}

V3_సంఖ్య& V3_సంఖ్య::opSelInto(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& lsb, పూర్ణాంకం వెడల్పు) {
    తిరిగి opSelInto(lhs, lsb.గమ్యం_SInt(), వెడల్పు);
}

V3_సంఖ్య& V3_సంఖ్య::opSelInto(స్థిరరాశి V3_సంఖ్య& lhs, పూర్ణాంకం lsbval, పూర్ణాంకం వెడల్పు) {
    // this[lsbval+width-1 : lsbval] = lhs;  Other bits of this are not affected
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    UASSERT(lsbval >= 0, "Negative destination bit range");
    స్థిరరాశి పూర్ణాంకం copy_వెడల్పు = std::max(0, std::min({వెడల్పు, lhs.వెడల్పు(), ఇది->వెడల్పు() - lsbval}));
    copy_బిట్లు(lsbval, lhs, 0, copy_వెడల్పు);
    స్థిరరాశి పూర్ణాంకం fill_వెడల్పు
        = std::max(0, std::min(వెడల్పు - copy_వెడల్పు, ఇది->వెడల్పు() - lsbval - copy_వెడల్పు));
    fill_బిట్లు(lsbval + copy_వెడల్పు, fill_వెడల్పు, v3Global.constRemoveXs() ? 0 : 'x');
    తిరిగి *ఇది;
}

//======================================================================
// Ops - Floating point

V3_సంఖ్య& V3_సంఖ్య::opIToRD(స్థిరరాశి V3_సంఖ్య& lhs, తార్కిక isSigned) {
    // Correct number of zero bits/width matters
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(lhs);
    // IEEE says we ignore x/z in real conversions
    V3_సంఖ్య noxz(lhs);
    noxz.op_కేటాయించు_Non_XZ(lhs);
    ద్విగుణ d = 0;
    స్థిరరాశి తార్కిక negate = isSigned && noxz.isNegative();
    ఒకవేళ (negate) {
        స్థిరరాశి V3_సంఖ్య noxz_signed = noxz;
        noxz.opNegate(noxz_signed);
    }
    కొరకు (పూర్ణాంకం బిట్టు_2 = noxz.వెడల్పు() - 1; బిట్టు_2 >= 0; బిట్టు_2--) {
        // Some precision might be lost in this add, that's what we want
        ఒకవేళ (noxz.బిట్టు_Is1(బిట్టు_2)) d += exp2(బిట్టు_2);
    }
    ఒకవేళ (negate) d = -d;
    తిరిగి సమితి_Double(d);
}
V3_సంఖ్య& V3_సంఖ్య::opRToIS(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_DOUBLE_ARGS1(lhs);
    స్థిరరాశి ద్విగుణ v = VL_TRUNC(lhs.గమ్యం_Double());
    స్థిరరాశి int32_t i = static_cast<int32_t>(v);  // C converts from double to int32_t
    తిరిగి సమితి_Long_S(i);
}
V3_సంఖ్య& V3_సంఖ్య::opRToIRoundS(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_DOUBLE_ARGS1(lhs);
    స్థిరరాశి ద్విగుణ v = VL_విడత(lhs.గమ్యం_Double());
    సమితి_Zero();
    సంఘం {
        ద్విగుణ d;
        uint64_t q;
    } u;
    u.d = v;
    (శూన్య)u.d;

    స్థిరరాశి పూర్ణాంకం ఘాతాంకం = static_cast<పూర్ణాంకం>((u.q >> 52ULL) & VL_ముసుగు_Q(11)) - 1023;
    స్థిరరాశి పూర్ణాంకం lsb = ఘాతాంకం - 52;
    స్థిరరాశి uint64_t mantissa = (u.q & VL_ముసుగు_Q(52)) | (1ULL << 52);
    ఒకవేళ (v != 0) {
        // IEEE format: [63]=sign [62:52]=exp+1023 [51:0]=mantissa
        // This does not need to support subnormals as they are sub-integral
        కొరకు (పూర్ణాంకం బిట్టు_2 = 0; బిట్టు_2 <= 52; ++బిట్టు_2) {
            ఒకవేళ (mantissa & (1ULL << బిట్టు_2)) {
                స్థిరరాశి పూర్ణాంకం outbit = బిట్టు_2 + lsb;
                ఒకవేళ (outbit >= 0) సమితి_బిట్టు(outbit, 1);
            }
        }
        ఒకవేళ (v < 0) {
            స్థిరరాశి V3_సంఖ్య noSign = *ఇది;
            opNegate(noSign);
        }
    }
    తిరిగి *ఇది;
}
V3_సంఖ్య& V3_సంఖ్య::op_Real_గమ్యం_బిట్లు(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_DOUBLE_ARGS1(lhs);
    UASSERT(lhs.వెడల్పు() == 64 && వెడల్పు() == 64, "Real operation on wrong sized number");
    సంఘం {
        ద్విగుణ m_d;
        uint64_t m_v;
    } u;
    u.m_d = lhs.గమ్యం_Double();
    తిరిగి సమితి_Quad(u.m_v);
}
V3_సంఖ్య& V3_సంఖ్య::op_బిట్లు_గమ్యం_Real_D(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    UASSERT(lhs.వెడల్పు() == 64 && వెడల్పు() == 64, "Real operation on wrong sized number");
    సంఘం {
        ద్విగుణ m_d;
        uint64_t m_v;
    } u;
    u.m_v = lhs.గమ్యం_UQuad();
    తిరిగి సమితి_Double(u.m_d);
}
V3_సంఖ్య& V3_సంఖ్య::opNegateD(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_DOUBLE_ARGS1(lhs);
    తిరిగి సమితి_Double(-lhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::op_కూడు_D(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Double(lhs.గమ్యం_Double() + rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opSubD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Double(lhs.గమ్యం_Double() - rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opMulD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Double(lhs.గమ్యం_Double() * rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opDivD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Double(lhs.గమ్యం_Double() / rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opPowD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    // On exceptions, we just generate 'inf' through floating point
    // IEEE says it's implementation defined what happens
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Double(ఘాతం(lhs.గమ్యం_Double(), rhs.గమ్యం_Double()));
}
V3_సంఖ్య& V3_సంఖ్య::opEqD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_Double() == rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opNeqD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_Double() != rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opGtD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_Double() > rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opGteD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_Double() >= rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opLtD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_Double() < rhs.గమ్యం_Double());
}
V3_సంఖ్య& V3_సంఖ్య::opLteD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_DOUBLE_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_Double() <= rhs.గమ్యం_Double());
}

//======================================================================
// Ops - String

V3_సంఖ్య& V3_సంఖ్య::opConcatN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_String(lhs.గమ్యం_String() + rhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::opReplN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_STRING_ARGS1(lhs);
    అంకె_ASSERT_LOGIC_ARGS1(rhs);
    తిరిగి opReplN(lhs, rhs.గమ్యం_UInt());
}
V3_సంఖ్య& V3_సంఖ్య::opReplN(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t rhsval) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    string బహిర్గతం;
    బహిర్గతం.reserve(lhs.గమ్యం_String().నిడివి() * rhsval);
    కొరకు (గుర్తులేని times = 0; times < rhsval; ++times) బహిర్గతం += lhs.గమ్యం_String();
    తిరిగి సమితి_String(బహిర్గతం);
}
V3_సంఖ్య& V3_సంఖ్య::opCvtPackString(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    తిరిగి సమితి_String(lhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::op_గమ్యం_క్రింది_N(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    స్థిరరాశి std::string బహిర్గతం = VString::downcase(lhs.గమ్యం_String());
    తిరిగి సమితి_String(బహిర్గతం);
}
V3_సంఖ్య& V3_సంఖ్య::op_గమ్యం_పై_N(స్థిరరాశి V3_సంఖ్య& lhs) {
    అంకె_ASSERT_OP_ARGS1(lhs);
    అంకె_ASSERT_STRING_ARGS1(lhs);
    స్థిరరాశి std::string బహిర్గతం = VString::upcase(lhs.గమ్యం_String());
    తిరిగి సమితి_String(బహిర్గతం);
}

V3_సంఖ్య& V3_సంఖ్య::opEqN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_String() == rhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::opNeqN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_String() != rhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::opGtN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_String() > rhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::opGteN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_String() >= rhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::opLtN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_String() < rhs.గమ్యం_String());
}
V3_సంఖ్య& V3_సంఖ్య::opLteN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs) {
    అంకె_ASSERT_OP_ARGS2(lhs, rhs);
    అంకె_ASSERT_STRING_ARGS2(lhs, rhs);
    తిరిగి సమితి_Single_బిట్లు(lhs.గమ్యం_String() <= rhs.గమ్యం_String());
}

//======================================================================

శూన్య V3_సంఖ్య::selfTest() {
    UINFO(2, __FUNCTION__ << ":");
    File_పంక్తి* స్థిరరాశి fl = కొత్త File_పంక్తి{File_పంక్తి::built_లోపల_Filename()};
    V3_సంఖ్య అంకె{fl, 32, 0};
    అంకె.selfTestThis();
}

శూన్య V3_సంఖ్య::selfTestThis() {
    // The self test has a "this" so UASSERT_SELFTEST/errorEndFatal works correctly

    UASSERT_SELFTEST(V3_సంఖ్య::epsilonEqual(0, 0), నిజం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonEqual(1e19, 1e19), నిజం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonEqual(9, 0.0001), అబద్ధం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonEqual(1, 1 + std::numeric_limits<ద్విగుణ>::epsilon()), నిజం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonEqual(0.009, 0.00899999999999999931998839741709), నిజం);

    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(0), నిజం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(1), నిజం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(-1), నిజం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(1.0001), అబద్ధం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(0.9999), అబద్ధం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(-1.0001), అబద్ధం);
    UASSERT_SELFTEST(V3_సంఖ్య::epsilonIntegral(-0.9999), అబద్ధం);

    UASSERT_SELFTEST(log2b(0), 0);
    UASSERT_SELFTEST(log2b(1), 0);
    UASSERT_SELFTEST(log2b(0x40000000UL), 30);
    UASSERT_SELFTEST(log2bQuad(0x4000000000000000ULL), 62);
}
