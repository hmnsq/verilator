// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Options parsing
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

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
#అన్తర్భావ <io.h>  // open, read, write, close
#అయితే_ముగింపు

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3String.h"

#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
#అన్తర్భావ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#అయితే_ముగింపు

#అన్తర్భావ <algorithm>
#అన్తర్భావ <fcntl.h>

కొలత_t VName::s_min_నిడివి = 32;
కొలత_t VName::s_max_నిడివి = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
మూస <తార్కిక Even = నిజం>
స్థిర తార్కిక wild_సరిపోలిక_Impl(స్థిరరాశి అక్షరం* s, స్థిరరాశి అక్షరం* p) VL_శుద్ధ {
    కొరకు (; *p; s++, p++) {
        ఒకవేళ (*p != '*') {
            ఒకవేళ (((*s) != (*p)) && *p != '?') తిరిగి అబద్ధం;
        } లేకపోతే {
            // Trailing star matches everything.
            ఒకవేళ (!*++p) తిరిగి నిజం;
            అయితే (!wild_సరిపోలిక_Impl<!Even>(s, p)) {
                ఒకవేళ (*++s == '\0') తిరిగి అబద్ధం;
            }
            తిరిగి నిజం;
        }
    }
    తిరిగి (*s == '\0');
}

తార్కిక VString::wildmatch(స్థిరరాశి అక్షరం* s, స్థిరరాశి అక్షరం* p) VL_శుద్ధ {
    ఒకవేళ (*s == '\0') {
        అయితే (*p == '*') ++p;
        తిరిగి *p == '\0';
    }
    తిరిగి wild_సరిపోలిక_Impl(s, p);
}

తార్కిక VString::wildmatch(స్థిరరాశి string& s, స్థిరరాశి string& p) VL_శుద్ధ {
    తిరిగి wildmatch(s.c_str(), p.c_str());
}

string VString::dot(స్థిరరాశి string& a, స్థిరరాశి string& dot, స్థిరరాశి string& b) {
    ఒకవేళ (b == "") తిరిగి a;
    ఒకవేళ (a == "") తిరిగి b;
    తిరిగి a + dot + b;
}

string VString::downcase(స్థిరరాశి string& str) VL_శుద్ధ {
    string ఫలితం = str;
    కొరకు (అక్షరం& cr : ఫలితం) cr = std::tolower(cr);
    తిరిగి ఫలితం;
}

string VString::upcase(స్థిరరాశి string& str) VL_శుద్ధ {
    string ఫలితం = str;
    కొరకు (అక్షరం& cr : ఫలితం) cr = std::పెద్ద_అక్షరం(cr);
    తిరిగి ఫలితం;
}

string VString::quoteAny(స్థిరరాశి string& str, అక్షరం tgt, అక్షరం esc) VL_శుద్ధ {
    string ఫలితం;
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (c == tgt) ఫలితం += esc;
        ఫలితం += c;
    }
    తిరిగి ఫలితం;
}

string VString::dequotePercent(స్థిరరాశి string& str) {
    string ఫలితం;
    అక్షరం చివరి = '\0';
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (చివరి == '%' && c == '%') {
            చివరి = '\0';
        } లేకపోతే {
            ఫలితం += c;
            చివరి = c;
        }
    }
    తిరిగి ఫలితం;
}

string VString::quoteStringLiteralForShell(స్థిరరాశి string& str) {
    string ఫలితం;
    constexpr అక్షరం dquote = '"';
    constexpr అక్షరం escape = '\\';
    ఫలితం.నెట్టు_వెనుక(dquote);  // Start quoted string
    ఫలితం.నెట్టు_వెనుక(escape);
    ఫలితం.నెట్టు_వెనుక(dquote);  // "
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (c == dquote || c == escape) ఫలితం.నెట్టు_వెనుక(escape);
        ఫలితం.నెట్టు_వెనుక(c);
    }
    ఫలితం.నెట్టు_వెనుక(escape);
    ఫలితం.నెట్టు_వెనుక(dquote);  // "
    ఫలితం.నెట్టు_వెనుక(dquote);  // Terminate quoted string
    తిరిగి ఫలితం;
}

string VString::escape_String_For_మార్గం(స్థిరరాశి string& str) {
    ఒకవేళ (str.కనుగొను(R"(\\)") != string::npos)
        తిరిగి str;  // if it has been escaped already, don't do it again
    ఒకవేళ (str.కనుగొను('/') != string::npos) తిరిగి str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string ఫలితం;
    constexpr అక్షరం space = ' ';  // escape space like this `Program Files`
    constexpr అక్షరం escape = '\\';
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (c == space || c == escape) ఫలితం.నెట్టు_వెనుక(escape);
        ఫలితం.నెట్టు_వెనుక(c);
    }
    తిరిగి ఫలితం;
}

స్థిర పూర్ణాంకం vl_decodexdigit(అక్షరం c) {
    తిరిగి std::అంకెనా(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(స్థిరరాశి string& పాఠ్యం, string& err_బహిర్గతం) {
    తార్కిక quoted = అబద్ధం;
    string newtext;
    newtext.reserve(పాఠ్యం.కొలత());
    గుర్తులేని అక్షరం octal_val = 0;
    పూర్ణాంకం octal_digits = 0;
    కొరకు (string::const_iterator cp = పాఠ్యం.begin(); cp != పాఠ్యం.చివర(); ++cp) {
        ఒకవేళ (quoted) {
            ఒకవేళ (std::అంకెనా(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                ఒకవేళ (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = అబద్ధం;
                }
            } లేకపోతే {
                ఒకవేళ (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = అబద్ధం;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    కొనసాగు;
                }
                quoted = అబద్ధం;
                ఒకవేళ (*cp == 'n') {
                    newtext += '\n';
                } లేకపోతే ఒకవేళ (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } లేకపోతే ఒకవేళ (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } లేకపోతే ఒకవేళ (*cp == 'r') {
                    newtext += '\r';
                } లేకపోతే ఒకవేళ (*cp == 't') {
                    newtext += '\t';
                } లేకపోతే ఒకవేళ (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } లేకపోతే ఒకవేళ (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<అక్షరం>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } లేకపోతే ఒకవేళ (std::అక్షరమా_అంకెనా(*cp)) {
                    err_బహిర్గతం = "Unknown escape sequence: \\";
                    err_బహిర్గతం += *cp;
                    ఆపు;
                } లేకపోతే {
                    newtext += *cp;
                }
            }
        } లేకపోతే ఒకవేళ (*cp == '\\') {
            ఒకవేళ (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = నిజం;
            octal_digits = 0;
        } లేకపోతే {
            newtext += *cp;
        }
    }
    తిరిగి newtext;
}

కొలత_t VString::quoted_చివర(స్థిరరాశి string& str, కొలత_t pos) VL_శుద్ధ {
    కొరకు (కొలత_t i = pos + 1; i < str.కొలత(); ++i) {
        ఒకవేళ (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } లేకపోతే ఒకవేళ (str[i] == '"') {
            తిరిగి i + 1;
        }
    }
    తిరిగి string::npos;
}

string VString::spaceUnprintable(స్థిరరాశి string& str) VL_శుద్ధ {
    string ఫలితం;
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (std::isprint(c)) {
            ఫలితం += c;
        } లేకపోతే {
            ఫలితం += ' ';
        }
    }
    తిరిగి ఫలితం;
}

string VString::removeWhitespace(స్థిరరాశి string& str) {
    string ఫలితం;
    ఫలితం.reserve(str.కొలత());
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (!std::ఖాళీనా(c)) ఫలితం += c;
    }
    తిరిగి ఫలితం;
}

string VString::trimWhitespace(స్థిరరాశి string& str) {
    string ఫలితం;
    ఫలితం.reserve(str.కొలత());
    string కూడు;
    తార్కిక newline = అబద్ధం;
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (newline && std::ఖాళీనా(c)) కొనసాగు;
        ఒకవేళ (c == '\n') {
            కూడు = "\n";
            newline = నిజం;
            కొనసాగు;
        }
        ఒకవేళ (std::ఖాళీనా(c)) {
            కూడు += c;
            కొనసాగు;
        }
        ఒకవేళ (!కూడు.ఖాళీలు()) {
            ఫలితం += కూడు;
            newline = అబద్ధం;
            కూడు.clear();
        }
        ఫలితం += c;
    }
    తిరిగి ఫలితం;
}

తార్కిక VString::isIdentifier(స్థిరరాశి string& str) {
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (!isIdentifierChar(c)) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}

తార్కిక VString::isWhitespace(స్థిరరాశి string& str) {
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ఒకవేళ (!std::ఖాళీనా(c)) తిరిగి అబద్ధం;
    }
    తిరిగి నిజం;
}

string::కొలత_type VString::leading_Whitespace_లెక్క(స్థిరరాశి string& str) {
    string::కొలత_type ఫలితం = 0;
    కొరకు (స్థిరరాశి అక్షరం c : str) {
        ++ఫలితం;
        ఒకవేళ (!std::ఖాళీనా(c)) ఆపు;
    }
    తిరిగి ఫలితం;
}

ద్విగుణ VString::parseDouble(స్థిరరాశి string& str, తార్కిక* successp) {
    అక్షరం* స్థిరరాశి strgp = కొత్త అక్షరం[str.కొలత() + 1];
    అక్షరం* dp = strgp;
    ఒకవేళ (successp) *successp = నిజం;
    కొరకు (స్థిరరాశి అక్షరం* sp = str.c_str(); *sp; ++sp) {
        ఒకవేళ (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    అక్షరం* endp = strgp;
    స్థిరరాశి ద్విగుణ d = strtod(strgp, &endp);
    స్థిరరాశి కొలత_t parsed_len = endp - strgp;
    ఒకవేళ (parsed_len != std::అక్షరశ్రేణి_పొడవు(strgp)) {
        ఒకవేళ (successp) *successp = అబద్ధం;
    }
    VL_DO_DANGLING(తొలగించు[] strgp, strgp);
    తిరిగి d;
}

string VString::replaceSubstr(స్థిరరాశి string& str, స్థిరరాశి string& from, స్థిరరాశి string& గమ్యం) {
    string ఫలితం = str;
    స్థిరరాశి కొలత_t fromLen = from.కొలత();
    స్థిరరాశి కొలత_t గమ్యం_Len = గమ్యం.కొలత();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    కొరకు (కొలత_t pos = 0; (pos = ఫలితం.కనుగొను(from, pos)) != string::npos; pos += గమ్యం_Len) {
        ఫలితం.replace(pos, fromLen, గమ్యం);
    }
    తిరిగి ఫలితం;
}

string VString::replace_మాట(స్థిరరాశి string& str, స్థిరరాశి string& from, స్థిరరాశి string& గమ్యం) {
    string ఫలితం = str;
    స్థిరరాశి కొలత_t len = from.కొలత();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    కొరకు (కొలత_t pos = 0; (pos = ఫలితం.కనుగొను(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        ఒకవేళ (((pos > 0) && VString::isIdentifierChar(ఫలితం[pos - 1])) ||  //
            ((pos + len < ఫలితం.కొలత()) && VString::isIdentifierChar(ఫలితం[pos + len]))) {
            కొనసాగు;
        }
        ఫలితం.replace(pos, len, గమ్యం);
    }
    తిరిగి ఫలితం;
}

std::deque<string> VString::విభజించు(స్థిరరాశి string& str, అక్షరం delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    అయితే (std::getline(is, token, delimiter)) results.నెట్టు_వెనుక(token);
    తిరిగి results;
}

తార్కిక VString::చివర్లు_With(స్థిరరాశి string& str, స్థిరరాశి string& suffix) {
    ఒకవేళ (str.నిడివి() < suffix.నిడివి()) తిరిగి అబద్ధం;
    తిరిగి str.compare(str.నిడివి() - suffix.నిడివి(), suffix.నిడివి(), suffix) == 0;
}
string VString::aOrAn(స్థిరరాశి అక్షరం* మాట) {
    ఎంపిక (మాట[0]) {
    స్థితి '\0': తిరిగి "";
    స్థితి 'a':
    స్థితి 'e':
    స్థితి 'i':
    స్థితి 'o':
    స్థితి 'u': తిరిగి "an";
    అప్రమేయ: తిరిగి "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(స్థిరరాశి string& str) VL_శుద్ధ {
    స్థిరరాశి అక్షరం* కీలకం = str.c_str();
    స్థిరరాశి కొలత_t len = str.కొలత();
    constexpr uint64_t విత్తనం = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr పూర్ణాంకం r = 47;

    uint64_t h = విత్తనం ^ (len * m);

    స్థిరరాశి uint64_t* దత్తాంశం = reinterpret_cast<స్థిరరాశి uint64_t*>(కీలకం);
    స్థిరరాశి uint64_t* చివర = దత్తాంశం + (len / 8);

    అయితే (దత్తాంశం != చివర) {
        uint64_t k = *దత్తాంశం++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    స్థిరరాశి గుర్తులేని అక్షరం* data2 = reinterpret_cast<స్థిరరాశి గుర్తులేని అక్షరం*>(దత్తాంశం);

    ఎంపిక (len & 7) {
    స్థితి 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    స్థితి 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    స్థితి 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    స్థితి 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    స్థితి 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    స్థితి 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    స్థితి 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    అప్రమేయ:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    తిరిగి h;
}

string VString::base64Enc(స్థిరరాశి string& str) VL_శుద్ధ {
    // Non-URL format (+/), versus URL format (-/).
    స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    కొలత_t len = str.కొలత();
    string ఫలితం;
    ఫలితం.reserve(len * 4 / 3 + 2);
    కొలత_t pos = 0;
    ఒకవేళ (len >= 3) {
        కొరకు (; pos < len - 2; pos += 3) {
            ఫలితం += digits[((str[pos] >> 2) & 0x3f)];
            ఫలితం
                += digits[((str[pos] & 0x3) << 4) | (static_cast<పూర్ణాంకం>(str[pos + 1] & 0xf0) >> 4)];
            ఫలితం += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<పూర్ణాంకం>(str[pos + 2] & 0xc0) >> 6)];
            ఫలితం += digits[((str[pos + 2] & 0x3f))];
        }
    }
    ఒకవేళ (len >= 2 && pos < len - 1) {  // Pad 2
        ఫలితం += digits[((str[pos] >> 2) & 0x3f)];
        ఫలితం += digits[((str[pos] & 0x3) << 4) | (static_cast<పూర్ణాంకం>(str[pos + 1] & 0xf0) >> 4)];
        ఫలితం += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<పూర్ణాంకం>(0) >> 6)];
        ఫలితం += '=';
    } లేకపోతే ఒకవేళ (pos < len) {  // Pad 1
        ఫలితం += digits[((str[pos] >> 2) & 0x3f)];
        ఫలితం += digits[((str[pos] & 0x3) << 4) | (static_cast<పూర్ణాంకం>(0) >> 4)];
        ఫలితం += '=';
        ఫలితం += '=';
    }
    తిరిగి ఫలితం;
}

శూన్య VString::selfTest() {
    UASSERT_SELFTEST(VString::replaceSubstr("aa", "a", "ba"), "baba");

    // Cross-checked with 'base64'
    UASSERT_SELFTEST(VString::base64Enc(""), "");
    UASSERT_SELFTEST(VString::base64Enc("x"), "eA==");
    UASSERT_SELFTEST(VString::base64Enc("xy"), "eHk=");
    UASSERT_SELFTEST(VString::base64Enc("xyz"), "eHl6");
    UASSERT_SELFTEST(
        VString::base64Enc("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/~"),
        "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVphYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5ejAxM"
        "jM0NTY3ODkrL34=");
}

//######################################################################
// VHashSha512

స్థిర constexpr uint64_t sha512K[]
    = {0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
       0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
       0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
       0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
       0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
       0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
       0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
       0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
       0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
       0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
       0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
       0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
       0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
       0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
       0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
       0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
       0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
       0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
       0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
       0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

VL_ATTR_ALWINLINE
స్థిర uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_శుద్ధ {
    తిరిగి lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
స్థిర శూన్య sha512_ఖండం(uint64_t* h, స్థిరరాశి uint64_t* chunk) VL_శుద్ధ {
    uint64_t ah[8];
    స్థిరరాశి uint64_t* p = chunk;

    // Initialize working variables to current hash value
    కొరకు (గుర్తులేని i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    కొరకు (గుర్తులేని i = 0; i < 5; ++i) {
        కొరకు (గుర్తులేని j = 0; j < 16; ++j) {
            ఒకవేళ (i == 0) {
                w[j] = *p++;
            } లేకపోతే {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                స్థిరరాశి uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                స్థిరరాశి uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            స్థిరరాశి uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            స్థిరరాశి uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            స్థిరరాశి uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            స్థిరరాశి uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            స్థిరరాశి uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            స్థిరరాశి uint64_t temp2 = s0 + maj;

            ah[7] = ah[6];
            ah[6] = ah[5];
            ah[5] = ah[4];
            ah[4] = ah[3] + temp1;
            ah[3] = ah[2];
            ah[2] = ah[1];
            ah[1] = ah[0];
            ah[0] = temp1 + temp2;
        }
    }
    కొరకు (గుర్తులేని i = 0; i < 8; ++i) h[i] += ah[i];
}

శూన్య VHashSha512::చొప్పించు(స్థిరరాశి శూన్య* datap, కొలత_t నిడివి) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_నిడివి += నిడివి;

    string తాత్కాలిక_దత్తాంశం;
    పూర్ణాంకం chunkLen;
    స్థిరరాశి uint8_t* chunkp;
    ఒకవేళ (m_remainder == "") {
        chunkLen = నిడివి;
        chunkp = static_cast<స్థిరరాశి uint8_t*>(datap);
    } లేకపోతే {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        తాత్కాలిక_దత్తాంశం = m_remainder + std::string{static_cast<స్థిరరాశి అక్షరం*>(datap), నిడివి};
        chunkLen = తాత్కాలిక_దత్తాంశం.నిడివి();
        chunkp = reinterpret_cast<స్థిరరాశి uint8_t*>(తాత్కాలిక_దత్తాంశం.దత్తాంశం());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    పూర్ణాంకం posBegin = 0;  // Position in buffer for start of this block
    పూర్ణాంకం pos_చివర = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    అయితే (posBegin <= chunkLen - 128) {
        pos_చివర = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        కొరకు (పూర్ణాంకం విడత_Byte = 0; posBegin < pos_చివర; posBegin += 8) {
            w[విడత_Byte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
                              | (static_cast<uint64_t>(chunkp[posBegin + 6]) << 8)
                              | (static_cast<uint64_t>(chunkp[posBegin + 5]) << 16)
                              | (static_cast<uint64_t>(chunkp[posBegin + 4]) << 24)
                              | (static_cast<uint64_t>(chunkp[posBegin + 3]) << 32)
                              | (static_cast<uint64_t>(chunkp[posBegin + 2]) << 40)
                              | (static_cast<uint64_t>(chunkp[posBegin + 1]) << 48)
                              | (static_cast<uint64_t>(chunkp[posBegin]) << 56));
        }
        sha512_ఖండం(m_inthash, w);
    }

    m_remainder = std::string(reinterpret_cast<స్థిరరాశి అక్షరం*>(chunkp + posBegin), chunkLen - pos_చివర);
}

శూన్య VHashSha512::చొప్పించు_File(స్థిరరాశి string& filename) {
    స్థిర constexpr కొలత_t బఫరు_కొలత = 64 * 1024;

    స్థిరరాశి పూర్ణాంకం fd = ::open(filename.c_str(), O_RDONLY);
    ఒకవేళ (fd < 0) తిరిగి;

    std::array<అక్షరం, బఫరు_కొలత + 1> buf;
    అయితే (స్థిరరాశి ssize_t got = ::పఠ(fd, &buf, బఫరు_కొలత)) {
        ఒకవేళ (got <= 0) ఆపు;
        చొప్పించు(&buf, got);
    }
    ::close(fd);
}

శూన్య VHashSha512::finalize() {
    ఒకవేళ (!m_final) {
        // Make sure no 128 byte blocks left
        చొప్పించు("");
        m_final = నిజం;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        కొరకు (పూర్ణాంకం i = 0; i < 16; ++i) w[i] = 0;
        కొలత_t ఖండం_Pos = 0;
        కొరకు (; ఖండం_Pos < m_remainder.నిడివి(); ++ఖండం_Pos) {
            w[ఖండం_Pos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[ఖండం_Pos])))
                    << ((7 - (ఖండం_Pos & 7)) << 3));
        }
        w[ఖండం_Pos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (ఖండం_Pos & 7)) << 3);
        ఒకవేళ (m_remainder.నిడివి() >= 112) {
            sha512_ఖండం(m_inthash, w);
            కొరకు (పూర్ణాంకం i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_నిడివి << 3;
        sha512_ఖండం(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ద్విమాన() {
    finalize();
    string ఫలితం;
    ఫలితం.reserve(64);
    కొరకు (కొలత_t i = 0; i < 64; ++i) {
        ఫలితం += static_cast<అక్షరం>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    తిరిగి ఫలితం;
}

uint64_t VHashSha512::digestUInt64() {
    స్థిరరాశి string& binhash = digest_ద్విమాన();
    uint64_t ఫలితం = 0;
    కొరకు (కొలత_t byte = 0; byte < పరిమాణం(uint64_t); ++byte) {
        స్థిరరాశి గుర్తులేని అక్షరం c = binhash[byte];
        ఫలితం = (ఫలితం << 8) | c;
    }
    తిరిగి ఫలితం;
}

string VHashSha512::digestHex() {
    స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి digits = "0123456789abcdef";
    స్థిరరాశి string& binhash = digest_ద్విమాన();
    string ఫలితం;
    ఫలితం.reserve(128);
    కొరకు (కొలత_t byte = 0; byte < 64; ++byte) {
        ఫలితం += digits[(binhash[byte] >> 4) & 0xf];
        ఫలితం += digits[(binhash[byte] >> 0) & 0xf];
    }
    తిరిగి ఫలితం;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    తిరిగి VString::base64Enc(digest_ద్విమాన());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    స్థిరరాశి string& binhash = digest_ద్విమాన();
    string ఫలితం;
    ఫలితం.reserve(84);
    పూర్ణాంకం pos = 0;
    కొరకు (; pos < (512 / 8) - 2; pos += 3) {
        ఫలితం += digits[((binhash[pos] >> 2) & 0x3f)];
        ఫలితం += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<పూర్ణాంకం>(binhash[pos + 1] & 0xf0) >> 4)];
        ఫలితం += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<పూర్ణాంకం>(binhash[pos + 2] & 0xc0) >> 6)];
        ఫలితం += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        ఒకవేళ (ఫలితం.కొలత() >= DIGEST_SYMBOL24_నిడివి) ఆపు;
    }
    // Any leftover bits don't matter for our purpose
    తిరిగి ఫలితం;
}

శూన్య VHashSha512::selfTestOne(స్థిరరాశి string& దత్తాంశం, స్థిరరాశి string& data2, స్థిరరాశి string& ఘాతాంకం,
                              స్థిరరాశి string& exp64, స్థిరరాశి string& exp24) {
    VHashSha512 digest{దత్తాంశం};
    ఒకవేళ (data2 != "") digest.చొప్పించు(data2);
    ఒకవేళ (VL_UNCOVERABLE(digest.digestHex() != ఘాతాంకం)) {
        std::cerr << "%Error: When hashing '" << దత్తాంశం + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << ఘాతాంకం << పంక్తి_ముగింపు;  // LCOV_EXCL_LINE
    }
    ఒకవేళ (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << దత్తాంశం + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << పంక్తి_ముగింపు;  // LCOV_EXCL_LINE
    }
    ఒకవేళ (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << దత్తాంశం + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << పంక్తి_ముగింపు;  // LCOV_EXCL_LINE
    }
}

శూన్య VHashSha512::selfTest() {
    // Cross-checked with sha512sum
    selfTestOne(
        "", "",
        "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
        "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
        "z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg==",
        "z4PhNX7vuL3xVChQ1m2AB9Yg");
    selfTestOne(
        "a", "",
        "1f40fc92da241694750979ee6cf582f2d5d7d28e18335de05abc54d0560e0f53"
        "02860c652bf08d560252aa5e74210546f369fbbbce8c12cfc7957b2652fe9a75",
        "H0D8ktokFpR1CXnubPWC8tXX0o4YM13gWrxU0FYOD1MChgxlK/CNVgJSql50IQVG82n7u86MEs/HlXsmUv6adQ==",
        "H0D8ktokFpR1CXnubPWC8tXX");
    selfTestOne(
        "The quick brown fox jumps over the lazy dog", "",
        "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb64"
        "2e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6",
        "B+VH2VhvanP3P7rAQ17XaVEhj7fQyNeIownXhUNru2Quk6JSqVTyORJUfR6KO17W4b/XCXghIz+gU489uFT+5g==",
        "BAVH2VhvanP3P7rAQ17XaVEh");
    selfTestOne(
        "The quick brown fox jumps over the lazy", " dog",
        "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb64"
        "2e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6",
        "B+VH2VhvanP3P7rAQ17XaVEhj7fQyNeIownXhUNru2Quk6JSqVTyORJUfR6KO17W4b/XCXghIz+gU489uFT+5g==",
        "BAVH2VhvanP3P7rAQ17XaVEh");
    selfTestOne(
        "Test using larger than block-size key and larger than one block-size data."
        " SHA512 has a 128 byte block size so this needs to be more than 128 characters long.",
        "",
        "6f51a6ddad3a86fccd8d1d6584712567ee60b00d6d31bfecb69b0e288f45fbbd"
        "91b785c218f1e7c019088ad9f47680a93720bada029294dd7a7fb8119137dbf1",
        "b1Gm3a06hvzNjR1lhHElZ+5gsA1tMb/stpsOKI9F+72Rt4XCGPHnwBkIitn0doCpNyC62gKSlN16f7gRkTfb8Q==",
        "b1Gm3a06hvzNjR1lhHElZA5g");
    selfTestOne(
        "Test using",
        " larger than block-size key and larger than one block-size data."
        " SHA512 has a 128 byte block size so this needs to be more than 128 characters long.",
        "6f51a6ddad3a86fccd8d1d6584712567ee60b00d6d31bfecb69b0e288f45fbbd"
        "91b785c218f1e7c019088ad9f47680a93720bada029294dd7a7fb8119137dbf1",
        "b1Gm3a06hvzNjR1lhHElZ+5gsA1tMb/stpsOKI9F+72Rt4XCGPHnwBkIitn0doCpNyC62gKSlN16f7gRkTfb8Q==",
        "b1Gm3a06hvzNjR1lhHElZA5g");
}

//######################################################################
// VName

string VName::dehash(స్థిరరాశి string& లోపల) {
    స్థిర constexpr స్థిరరాశి అక్షరం VHSH[] = "__Vhsh";
    స్థిర constexpr కొలత_t VHSH_LEN = పరిమాణం(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_నిడివి;
    స్థిర స్థిరరాశి కొలత_t DOT_LEN = std::అక్షరశ్రేణి_పొడవు("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    కొరకు (string::కొలత_type చివరి_dot_pos = 0; చివరి_dot_pos < లోపల.కొలత();) {
        స్థిరరాశి string::కొలత_type తదుపరి_dot_pos = లోపల.కనుగొను("__DOT__", చివరి_dot_pos);
        // Two iterators defining the range between the last and next dots.
        స్థిరరాశి స్వయంచాలక శోధన_begin = std::begin(లోపల) + చివరి_dot_pos;
        స్థిరరాశి స్వయంచాలక శోధన_చివర
            = తదుపరి_dot_pos == string::npos ? std::చివర(లోపల) : std::begin(లోపల) + తదుపరి_dot_pos;

        // Search for __Vhsh between the two dots.
        స్థిరరాశి స్వయంచాలక begin_vhsh
            = std::శోధన(శోధన_begin, శోధన_చివర, std::begin(VHSH), std::చివర(VHSH) - 1);
        ఒకవేళ (begin_vhsh != శోధన_చివర) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            స్థిరరాశి స్వయంచాలక చివర_vhsh
                = begin_vhsh + std::min<కొలత_t>(std::దూరం(begin_vhsh, శోధన_చివర), VHSH_LEN);
            స్థిరరాశి std::string vhsh{begin_vhsh, చివర_vhsh};
            స్థిరరాశి స్వయంచాలక& it = s_dehashMap.కనుగొను(vhsh);
            UASSERT(it != s_dehashMap.చివర(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            ఒకవేళ (చివరి_dot_pos > 0 && dehashed.ఖాళీలు()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = లోపల.substr(0, చివరి_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{శోధన_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->రెండవ;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{చివర_vhsh, శోధన_చివర};
        }
        // This component doesn't need dehashing but a previous one might have.
        లేకపోతే ఒకవేళ (!dehashed.ఖాళీలు()) {
            dehashed += std::string{శోధన_begin, శోధన_చివర};
        }

        ఒకవేళ (తదుపరి_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            ఒకవేళ (!dehashed.ఖాళీలు()) dehashed += "__DOT__";
            చివరి_dot_pos = తదుపరి_dot_pos + DOT_LEN;
        } లేకపోతే {
            చివరి_dot_pos = string::npos;
        }
    }
    తిరిగి dehashed.ఖాళీలు() ? లోపల : dehashed;
}

string VName::hashedName() {
    ఒకవేళ (m_name == "") తిరిగి "";
    ఒకవేళ (m_hashed != "") తిరిగి m_hashed;  // Memoized
    ఒకవేళ (s_max_నిడివి == 0 || m_name.నిడివి() < s_max_నిడివి) {
        m_hashed = m_name;
        తిరిగి m_hashed;
    }
    VHashSha512 hash{m_name};
    స్థిరరాశి string suffix = "__Vhsh" + hash.digestSymbol24();
    ఒకవేళ (s_min_నిడివి < s_max_నిడివి) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        కొలత_t pref_నిడివి = s_min_నిడివి;
        అయితే (pref_నిడివి >= 1 && m_name[pref_నిడివి - 1] != '_') --pref_నిడివి;
        s_dehashMap[suffix] = m_name.substr(pref_నిడివి);
        m_hashed = m_name.substr(0, pref_నిడివి) + suffix;
    } లేకపోతే {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    తిరిగి m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::Edit_దూరం VSpellCheck::edit_దూరం(స్థిరరాశి string& s, స్థిరరాశి string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    స్థిరరాశి కొలత_t sLen = s.నిడివి();
    స్థిరరాశి కొలత_t tLen = t.నిడివి();
    ఒకవేళ (sLen == 0) తిరిగి tLen;
    ఒకవేళ (tLen == 0) తిరిగి sLen;
    ఒకవేళ (sLen >= నిడివి_హద్దు) తిరిగి sLen;
    ఒకవేళ (tLen >= నిడివి_హద్దు) తిరిగి tLen;

    స్థిర std::array<Edit_దూరం, నిడివి_హద్దు + 1> s_v_two_ago;
    స్థిర std::array<Edit_దూరం, నిడివి_హద్దు + 1> s_v_one_ago;
    స్థిర std::array<Edit_దూరం, నిడివి_హద్దు + 1> s_v_తదుపరి;

    కొరకు (కొలత_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    కొరకు (కొలత_t i = 0; i < tLen; i++) {
        s_v_తదుపరి[0] = i + 1;
        కొరకు (కొలత_t j = 0; j < sLen; j++) {
            స్థిరరాశి Edit_దూరం ఖర్చు = (s[j] == t[i] ? 0 : 1);
            స్థిరరాశి Edit_దూరం deletion = s_v_తదుపరి[j] + 1;
            స్థిరరాశి Edit_దూరం insertion = s_v_one_ago[j + 1] + 1;
            స్థిరరాశి Edit_దూరం substitution = s_v_one_ago[j] + ఖర్చు;
            Edit_దూరం cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            ఒకవేళ (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                స్థిరరాశి Edit_దూరం transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_తదుపరి[j + 1] = cheapest;
        }
        కొరకు (కొలత_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_తదుపరి[j];
        }
    }

    స్థిరరాశి Edit_దూరం ఫలితం = s_v_తదుపరి[sLen];
    తిరిగి ఫలితం;
}

VSpellCheck::Edit_దూరం VSpellCheck::cutoff_దూరం(కొలత_t goal_len, కొలత_t candidate_len) {
    // Return max acceptable edit distance
    స్థిరరాశి కొలత_t max_నిడివి_2 = std::max(goal_len, candidate_len);
    స్థిరరాశి కొలత_t min_నిడివి = std::min(goal_len, candidate_len);
    ఒకవేళ (max_నిడివి_2 <= 1) తిరిగి 0;
    ఒకవేళ (max_నిడివి_2 - min_నిడివి <= 1) తిరిగి std::max(max_నిడివి_2 / 3, static_cast<కొలత_t>(1));
    తిరిగి (max_నిడివి_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::ఉత్తమ_Candidate_Info(స్థిరరాశి string& goal,
                                                      Edit_దూరం& distancer) స్థిరరాశి {
    Candidate ఉత్తమ;
    స్థిరరాశి కొలత_t gLen = goal.నిడివి();
    distancer = నిడివి_హద్దు * 10;
    కొరకు (స్థిరరాశి Candidate& candidate : m_candidates) {
        స్థిరరాశి కొలత_t cLen = candidate.str.నిడివి();

        // Min distance must be inserting/deleting to make lengths match
        స్థిరరాశి Edit_దూరం min_దూరం = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        ఒకవేళ (min_దూరం >= distancer) కొనసాగు;  // Short-circuit if already better

        స్థిరరాశి Edit_దూరం cutoff = cutoff_దూరం(gLen, cLen);
        ఒకవేళ (min_దూరం > cutoff) కొనసాగు;  // Short-circuit if already too bad

        స్థిరరాశి Edit_దూరం dist = edit_దూరం(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.సందేశం_2
                                      << "'");
        ఒకవేళ (dist < distancer && dist <= cutoff) {
            distancer = dist;
            ఉత్తమ = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    ఒకవేళ (distancer == 0) తిరిగి Candidate{.str = ఉత్తమ.str, .సందేశం_2 = ""};
    తిరిగి ఉత్తమ;
}

శూన్య VSpellCheck::self_Test_దూరం_One(స్థిరరాశి string& a, స్థిరరాశి string& b, Edit_దూరం expected) {
    UASSERT_SELFTEST(edit_దూరం(a, b), expected);
    UASSERT_SELFTEST(edit_దూరం(b, a), expected);
}

శూన్య VSpellCheck::selfTestSuggestOne(తార్కిక matches, స్థిరరాశి string& c, స్థిరరాశి string& goal,
                                     Edit_దూరం dist) {
    Edit_దూరం gdist;
    VSpellCheck speller;
    speller.నెట్టు_Candidate(c);
    స్థిరరాశి string got = speller.ఉత్తమ_Candidate_Info(goal, gdist /*ref*/).str;
    ఒకవేళ (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } లేకపోతే {
        UASSERT_SELFTEST(got, "");
    }
}

శూన్య VSpellCheck::selfTest() {
    {
        self_Test_దూరం_One("ab", "ac", 1);
        self_Test_దూరం_One("ab", "a", 1);
        self_Test_దూరం_One("a", "b", 1);
    }
    {
        selfTestSuggestOne(నిజం, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(నిజం, "abcdef", "acbdef", 1);
        selfTestSuggestOne(నిజం, "db", "dc", 1);
        selfTestSuggestOne(నిజం, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(అబద్ధం, "x", "y", 1);
        selfTestSuggestOne(అబద్ధం, "sqrt", "assert", 3);
    }
    {
        స్థిరరాశి VSpellCheck speller;
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.నెట్టు_Candidate("fred");
        speller.నెట్టు_Candidate("wilma");
        speller.నెట్టు_Candidate("barney");
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        కొరకు (గుర్తులేని i = 0; i < అంకె_CANDIDATE_హద్దు; ++i) speller.నెట్టు_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.కొలత(), అంకె_CANDIDATE_హద్దు);
        speller.నెట్టు_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.కొలత(), అంకె_CANDIDATE_హద్దు);
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.ఉత్తమ_Candidate("wilma").str, "");
    }
}
