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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#إذا defined(_WIN32) || defined(__MINGW32__)
#ضمن <io.h>  // open, read, write, close
#د_که_پای

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3String.h"

#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#د_که_پای

#ضمن <algorithm>
#ضمن <fcntl.h>

اندازه_t VName::s_min_اوږدتیا = 32;
اندازه_t VName::s_max_اوږدتیا = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
کينډۍ <منطقي Even = ریښتیا>
ساكن منطقي wild_سمون_Impl(ثابت حرف* s, ثابت حرف* p) VL_خالص {
    لكل (; *p; s++, p++) {
        إذا (*p != '*') {
            إذا (((*s) != (*p)) && *p != '?') أرجع خطأ;
        } وإلا {
            // Trailing star matches everything.
            إذا (!*++p) أرجع ریښتیا;
            طالما (!wild_سمون_Impl<!Even>(s, p)) {
                إذا (*++s == '\0') أرجع خطأ;
            }
            أرجع ریښتیا;
        }
    }
    أرجع (*s == '\0');
}

منطقي VString::wildmatch(ثابت حرف* s, ثابت حرف* p) VL_خالص {
    إذا (*s == '\0') {
        طالما (*p == '*') ++p;
        أرجع *p == '\0';
    }
    أرجع wild_سمون_Impl(s, p);
}

منطقي VString::wildmatch(ثابت string& s, ثابت string& p) VL_خالص {
    أرجع wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ثابت string& a, ثابت string& dot, ثابت string& b) {
    إذا (b == "") أرجع a;
    إذا (a == "") أرجع b;
    أرجع a + dot + b;
}

string VString::downcase(ثابت string& str) VL_خالص {
    string پایله = str;
    لكل (حرف& cr : پایله) cr = std::tolower(cr);
    أرجع پایله;
}

string VString::upcase(ثابت string& str) VL_خالص {
    string پایله = str;
    لكل (حرف& cr : پایله) cr = std::لوی_توری(cr);
    أرجع پایله;
}

string VString::quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_خالص {
    string پایله;
    لكل (ثابت حرف c : str) {
        إذا (c == tgt) پایله += esc;
        پایله += c;
    }
    أرجع پایله;
}

string VString::dequotePercent(ثابت string& str) {
    string پایله;
    حرف وروستی = '\0';
    لكل (ثابت حرف c : str) {
        إذا (وروستی == '%' && c == '%') {
            وروستی = '\0';
        } وإلا {
            پایله += c;
            وروستی = c;
        }
    }
    أرجع پایله;
}

string VString::quoteStringLiteralForShell(ثابت string& str) {
    string پایله;
    constexpr حرف dquote = '"';
    constexpr حرف escape = '\\';
    پایله.ټېل_شا(dquote);  // Start quoted string
    پایله.ټېل_شا(escape);
    پایله.ټېل_شا(dquote);  // "
    لكل (ثابت حرف c : str) {
        إذا (c == dquote || c == escape) پایله.ټېل_شا(escape);
        پایله.ټېل_شا(c);
    }
    پایله.ټېل_شا(escape);
    پایله.ټېل_شا(dquote);  // "
    پایله.ټېل_شا(dquote);  // Terminate quoted string
    أرجع پایله;
}

string VString::escape_String_For_لار(ثابت string& str) {
    إذا (str.پیدا_کړه(R"(\\)") != string::npos)
        أرجع str;  // if it has been escaped already, don't do it again
    إذا (str.پیدا_کړه('/') != string::npos) أرجع str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string پایله;
    constexpr حرف space = ' ';  // escape space like this `Program Files`
    constexpr حرف escape = '\\';
    لكل (ثابت حرف c : str) {
        إذا (c == space || c == escape) پایله.ټېل_شا(escape);
        پایله.ټېل_شا(c);
    }
    أرجع پایله;
}

ساكن صحيح vl_decodexdigit(حرف c) {
    أرجع std::که_عدد(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ثابت string& متن, string& err_بهر) {
    منطقي quoted = خطأ;
    string newtext;
    newtext.reserve(متن.اندازه());
    بینښه حرف octal_val = 0;
    صحيح octal_digits = 0;
    لكل (string::const_iterator cp = متن.begin(); cp != متن.پای_ټکی(); ++cp) {
        إذا (quoted) {
            إذا (std::که_عدد(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                إذا (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = خطأ;
                }
            } وإلا {
                إذا (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = خطأ;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    استمر;
                }
                quoted = خطأ;
                إذا (*cp == 'n') {
                    newtext += '\n';
                } وإلا إذا (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } وإلا إذا (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } وإلا إذا (*cp == 'r') {
                    newtext += '\r';
                } وإلا إذا (*cp == 't') {
                    newtext += '\t';
                } وإلا إذا (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } وإلا إذا (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<حرف>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } وإلا إذا (std::که_توری_یا_عدد(*cp)) {
                    err_بهر = "Unknown escape sequence: \\";
                    err_بهر += *cp;
                    اكسر;
                } وإلا {
                    newtext += *cp;
                }
            }
        } وإلا إذا (*cp == '\\') {
            إذا (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = ریښتیا;
            octal_digits = 0;
        } وإلا {
            newtext += *cp;
        }
    }
    أرجع newtext;
}

اندازه_t VString::quoted_پای_ټکی(ثابت string& str, اندازه_t pos) VL_خالص {
    لكل (اندازه_t i = pos + 1; i < str.اندازه(); ++i) {
        إذا (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } وإلا إذا (str[i] == '"') {
            أرجع i + 1;
        }
    }
    أرجع string::npos;
}

string VString::spaceUnprintable(ثابت string& str) VL_خالص {
    string پایله;
    لكل (ثابت حرف c : str) {
        إذا (std::isprint(c)) {
            پایله += c;
        } وإلا {
            پایله += ' ';
        }
    }
    أرجع پایله;
}

string VString::removeWhitespace(ثابت string& str) {
    string پایله;
    پایله.reserve(str.اندازه());
    لكل (ثابت حرف c : str) {
        إذا (!std::که_تشه(c)) پایله += c;
    }
    أرجع پایله;
}

string VString::trimWhitespace(ثابت string& str) {
    string پایله;
    پایله.reserve(str.اندازه());
    string جمع_کړه;
    منطقي newline = خطأ;
    لكل (ثابت حرف c : str) {
        إذا (newline && std::که_تشه(c)) استمر;
        إذا (c == '\n') {
            جمع_کړه = "\n";
            newline = ریښتیا;
            استمر;
        }
        إذا (std::که_تشه(c)) {
            جمع_کړه += c;
            استمر;
        }
        إذا (!جمع_کړه.تشه()) {
            پایله += جمع_کړه;
            newline = خطأ;
            جمع_کړه.clear();
        }
        پایله += c;
    }
    أرجع پایله;
}

منطقي VString::isIdentifier(ثابت string& str) {
    لكل (ثابت حرف c : str) {
        إذا (!isIdentifierChar(c)) أرجع خطأ;
    }
    أرجع ریښتیا;
}

منطقي VString::isWhitespace(ثابت string& str) {
    لكل (ثابت حرف c : str) {
        إذا (!std::که_تشه(c)) أرجع خطأ;
    }
    أرجع ریښتیا;
}

string::اندازه_type VString::leading_Whitespace_شمېر(ثابت string& str) {
    string::اندازه_type پایله = 0;
    لكل (ثابت حرف c : str) {
        ++پایله;
        إذا (!std::که_تشه(c)) اكسر;
    }
    أرجع پایله;
}

مزدوج VString::parseDouble(ثابت string& str, منطقي* successp) {
    حرف* ثابت strgp = جديد حرف[str.اندازه() + 1];
    حرف* dp = strgp;
    إذا (successp) *successp = ریښتیا;
    لكل (ثابت حرف* sp = str.c_str(); *sp; ++sp) {
        إذا (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    حرف* endp = strgp;
    ثابت مزدوج d = strtod(strgp, &endp);
    ثابت اندازه_t parsed_len = endp - strgp;
    إذا (parsed_len != std::مزي_اوږدوالی(strgp)) {
        إذا (successp) *successp = خطأ;
    }
    VL_DO_DANGLING(ړنګول[] strgp, strgp);
    أرجع d;
}

string VString::replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& منزل) {
    string پایله = str;
    ثابت اندازه_t fromLen = from.اندازه();
    ثابت اندازه_t منزل_Len = منزل.اندازه();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    لكل (اندازه_t pos = 0; (pos = پایله.پیدا_کړه(from, pos)) != string::npos; pos += منزل_Len) {
        پایله.replace(pos, fromLen, منزل);
    }
    أرجع پایله;
}

string VString::replace_کلمه(ثابت string& str, ثابت string& from, ثابت string& منزل) {
    string پایله = str;
    ثابت اندازه_t len = from.اندازه();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    لكل (اندازه_t pos = 0; (pos = پایله.پیدا_کړه(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        إذا (((pos > 0) && VString::isIdentifierChar(پایله[pos - 1])) ||  //
            ((pos + len < پایله.اندازه()) && VString::isIdentifierChar(پایله[pos + len]))) {
            استمر;
        }
        پایله.replace(pos, len, منزل);
    }
    أرجع پایله;
}

std::deque<string> VString::وېشل(ثابت string& str, حرف delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    طالما (std::getline(is, token, delimiter)) results.ټېل_شا(token);
    أرجع results;
}

منطقي VString::پایونه_With(ثابت string& str, ثابت string& suffix) {
    إذا (str.اوږدتیا() < suffix.اوږدتیا()) أرجع خطأ;
    أرجع str.compare(str.اوږدتیا() - suffix.اوږدتیا(), suffix.اوږدتیا(), suffix) == 0;
}
string VString::aOrAn(ثابت حرف* کلمه) {
    بدل (کلمه[0]) {
    حالة '\0': أرجع "";
    حالة 'a':
    حالة 'e':
    حالة 'i':
    حالة 'o':
    حالة 'u': أرجع "an";
    افتراضي: أرجع "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ثابت string& str) VL_خالص {
    ثابت حرف* کلی = str.c_str();
    ثابت اندازه_t len = str.اندازه();
    constexpr uint64_t تخم = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr صحيح r = 47;

    uint64_t h = تخم ^ (len * m);

    ثابت uint64_t* معلومات = reinterpret_cast<ثابت uint64_t*>(کلی);
    ثابت uint64_t* پای_ټکی = معلومات + (len / 8);

    طالما (معلومات != پای_ټکی) {
        uint64_t k = *معلومات++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ثابت بینښه حرف* data2 = reinterpret_cast<ثابت بینښه حرف*>(معلومات);

    بدل (len & 7) {
    حالة 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    حالة 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    حالة 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    حالة 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    حالة 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    حالة 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    حالة 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    افتراضي:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    أرجع h;
}

string VString::base64Enc(ثابت string& str) VL_خالص {
    // Non-URL format (+/), versus URL format (-/).
    ساكن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    اندازه_t len = str.اندازه();
    string پایله;
    پایله.reserve(len * 4 / 3 + 2);
    اندازه_t pos = 0;
    إذا (len >= 3) {
        لكل (; pos < len - 2; pos += 3) {
            پایله += digits[((str[pos] >> 2) & 0x3f)];
            پایله
                += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(str[pos + 1] & 0xf0) >> 4)];
            پایله += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<صحيح>(str[pos + 2] & 0xc0) >> 6)];
            پایله += digits[((str[pos + 2] & 0x3f))];
        }
    }
    إذا (len >= 2 && pos < len - 1) {  // Pad 2
        پایله += digits[((str[pos] >> 2) & 0x3f)];
        پایله += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(str[pos + 1] & 0xf0) >> 4)];
        پایله += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<صحيح>(0) >> 6)];
        پایله += '=';
    } وإلا إذا (pos < len) {  // Pad 1
        پایله += digits[((str[pos] >> 2) & 0x3f)];
        پایله += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(0) >> 4)];
        پایله += '=';
        پایله += '=';
    }
    أرجع پایله;
}

فراغ VString::selfTest() {
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

ساكن constexpr uint64_t sha512K[]
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
ساكن uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_خالص {
    أرجع lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ساكن فراغ sha512_بلاک(uint64_t* h, ثابت uint64_t* chunk) VL_خالص {
    uint64_t ah[8];
    ثابت uint64_t* p = chunk;

    // Initialize working variables to current hash value
    لكل (بینښه i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    لكل (بینښه i = 0; i < 5; ++i) {
        لكل (بینښه j = 0; j < 16; ++j) {
            إذا (i == 0) {
                w[j] = *p++;
            } وإلا {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ثابت uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ثابت uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ثابت uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ثابت uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ثابت uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ثابت uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ثابت uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ثابت uint64_t temp2 = s0 + maj;

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
    لكل (بینښه i = 0; i < 8; ++i) h[i] += ah[i];
}

فراغ VHashSha512::ورننباسه(ثابت فراغ* datap, اندازه_t اوږدتیا) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_اوږدتیا += اوږدتیا;

    string لنډمهاله_معلومات;
    صحيح chunkLen;
    ثابت uint8_t* chunkp;
    إذا (m_remainder == "") {
        chunkLen = اوږدتیا;
        chunkp = static_cast<ثابت uint8_t*>(datap);
    } وإلا {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        لنډمهاله_معلومات = m_remainder + std::string{static_cast<ثابت حرف*>(datap), اوږدتیا};
        chunkLen = لنډمهاله_معلومات.اوږدتیا();
        chunkp = reinterpret_cast<ثابت uint8_t*>(لنډمهاله_معلومات.معلومات());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    صحيح posBegin = 0;  // Position in buffer for start of this block
    صحيح pos_پای_ټکی = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    طالما (posBegin <= chunkLen - 128) {
        pos_پای_ټکی = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        لكل (صحيح پړاو_Byte = 0; posBegin < pos_پای_ټکی; posBegin += 8) {
            w[پړاو_Byte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
                              | (static_cast<uint64_t>(chunkp[posBegin + 6]) << 8)
                              | (static_cast<uint64_t>(chunkp[posBegin + 5]) << 16)
                              | (static_cast<uint64_t>(chunkp[posBegin + 4]) << 24)
                              | (static_cast<uint64_t>(chunkp[posBegin + 3]) << 32)
                              | (static_cast<uint64_t>(chunkp[posBegin + 2]) << 40)
                              | (static_cast<uint64_t>(chunkp[posBegin + 1]) << 48)
                              | (static_cast<uint64_t>(chunkp[posBegin]) << 56));
        }
        sha512_بلاک(m_inthash, w);
    }

    m_remainder = std::string(reinterpret_cast<ثابت حرف*>(chunkp + posBegin), chunkLen - pos_پای_ټکی);
}

فراغ VHashSha512::ورننباسه_File(ثابت string& filename) {
    ساكن constexpr اندازه_t بفر_اندازه = 64 * 1024;

    ثابت صحيح fd = ::open(filename.c_str(), O_RDONLY);
    إذا (fd < 0) أرجع;

    std::array<حرف, بفر_اندازه + 1> buf;
    طالما (ثابت ssize_t got = ::اقرأ(fd, &buf, بفر_اندازه)) {
        إذا (got <= 0) اكسر;
        ورننباسه(&buf, got);
    }
    ::close(fd);
}

فراغ VHashSha512::finalize() {
    إذا (!m_final) {
        // Make sure no 128 byte blocks left
        ورننباسه("");
        m_final = ریښتیا;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        لكل (صحيح i = 0; i < 16; ++i) w[i] = 0;
        اندازه_t بلاک_Pos = 0;
        لكل (; بلاک_Pos < m_remainder.اوږدتیا(); ++بلاک_Pos) {
            w[بلاک_Pos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[بلاک_Pos])))
                    << ((7 - (بلاک_Pos & 7)) << 3));
        }
        w[بلاک_Pos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (بلاک_Pos & 7)) << 3);
        إذا (m_remainder.اوږدتیا() >= 112) {
            sha512_بلاک(m_inthash, w);
            لكل (صحيح i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_اوږدتیا << 3;
        sha512_بلاک(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_دوګونی() {
    finalize();
    string پایله;
    پایله.reserve(64);
    لكل (اندازه_t i = 0; i < 64; ++i) {
        پایله += static_cast<حرف>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    أرجع پایله;
}

uint64_t VHashSha512::digestUInt64() {
    ثابت string& binhash = digest_دوګونی();
    uint64_t پایله = 0;
    لكل (اندازه_t byte = 0; byte < حجم(uint64_t); ++byte) {
        ثابت بینښه حرف c = binhash[byte];
        پایله = (پایله << 8) | c;
    }
    أرجع پایله;
}

string VHashSha512::digestHex() {
    ساكن constexpr ثابت حرف* ثابت digits = "0123456789abcdef";
    ثابت string& binhash = digest_دوګونی();
    string پایله;
    پایله.reserve(128);
    لكل (اندازه_t byte = 0; byte < 64; ++byte) {
        پایله += digits[(binhash[byte] >> 4) & 0xf];
        پایله += digits[(binhash[byte] >> 0) & 0xf];
    }
    أرجع پایله;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    أرجع VString::base64Enc(digest_دوګونی());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ساكن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ثابت string& binhash = digest_دوګونی();
    string پایله;
    پایله.reserve(84);
    صحيح pos = 0;
    لكل (; pos < (512 / 8) - 2; pos += 3) {
        پایله += digits[((binhash[pos] >> 2) & 0x3f)];
        پایله += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<صحيح>(binhash[pos + 1] & 0xf0) >> 4)];
        پایله += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<صحيح>(binhash[pos + 2] & 0xc0) >> 6)];
        پایله += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        إذا (پایله.اندازه() >= DIGEST_SYMBOL24_اوږدتیا) اكسر;
    }
    // Any leftover bits don't matter for our purpose
    أرجع پایله;
}

فراغ VHashSha512::selfTestOne(ثابت string& معلومات, ثابت string& data2, ثابت string& توانیز_تابع,
                              ثابت string& exp64, ثابت string& exp24) {
    VHashSha512 digest{معلومات};
    إذا (data2 != "") digest.ورننباسه(data2);
    إذا (VL_UNCOVERABLE(digest.digestHex() != توانیز_تابع)) {
        std::cerr << "%Error: When hashing '" << معلومات + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << توانیز_تابع << کرښه_پای;  // LCOV_EXCL_LINE
    }
    إذا (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << معلومات + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << کرښه_پای;  // LCOV_EXCL_LINE
    }
    إذا (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << معلومات + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << کرښه_پای;  // LCOV_EXCL_LINE
    }
}

فراغ VHashSha512::selfTest() {
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

string VName::dehash(ثابت string& دننه) {
    ساكن constexpr ثابت حرف VHSH[] = "__Vhsh";
    ساكن constexpr اندازه_t VHSH_LEN = حجم(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_اوږدتیا;
    ساكن ثابت اندازه_t DOT_LEN = std::مزي_اوږدوالی("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    لكل (string::اندازه_type وروستی_dot_pos = 0; وروستی_dot_pos < دننه.اندازه();) {
        ثابت string::اندازه_type بل_dot_pos = دننه.پیدا_کړه("__DOT__", وروستی_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ثابت خپلکاری پلټنه_begin = std::begin(دننه) + وروستی_dot_pos;
        ثابت خپلکاری پلټنه_پای_ټکی
            = بل_dot_pos == string::npos ? std::پای_ټکی(دننه) : std::begin(دننه) + بل_dot_pos;

        // Search for __Vhsh between the two dots.
        ثابت خپلکاری begin_vhsh
            = std::پلټنه(پلټنه_begin, پلټنه_پای_ټکی, std::begin(VHSH), std::پای_ټکی(VHSH) - 1);
        إذا (begin_vhsh != پلټنه_پای_ټکی) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ثابت خپلکاری پای_ټکی_vhsh
                = begin_vhsh + std::min<اندازه_t>(std::واټن(begin_vhsh, پلټنه_پای_ټکی), VHSH_LEN);
            ثابت std::string vhsh{begin_vhsh, پای_ټکی_vhsh};
            ثابت خپلکاری& it = s_dehashMap.پیدا_کړه(vhsh);
            UASSERT(it != s_dehashMap.پای_ټکی(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            إذا (وروستی_dot_pos > 0 && dehashed.تشه()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = دننه.substr(0, وروستی_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{پلټنه_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->دوهم;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{پای_ټکی_vhsh, پلټنه_پای_ټکی};
        }
        // This component doesn't need dehashing but a previous one might have.
        وإلا إذا (!dehashed.تشه()) {
            dehashed += std::string{پلټنه_begin, پلټنه_پای_ټکی};
        }

        إذا (بل_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            إذا (!dehashed.تشه()) dehashed += "__DOT__";
            وروستی_dot_pos = بل_dot_pos + DOT_LEN;
        } وإلا {
            وروستی_dot_pos = string::npos;
        }
    }
    أرجع dehashed.تشه() ? دننه : dehashed;
}

string VName::hashedName() {
    إذا (m_name == "") أرجع "";
    إذا (m_hashed != "") أرجع m_hashed;  // Memoized
    إذا (s_max_اوږدتیا == 0 || m_name.اوږدتیا() < s_max_اوږدتیا) {
        m_hashed = m_name;
        أرجع m_hashed;
    }
    VHashSha512 hash{m_name};
    ثابت string suffix = "__Vhsh" + hash.digestSymbol24();
    إذا (s_min_اوږدتیا < s_max_اوږدتیا) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        اندازه_t pref_اوږدتیا = s_min_اوږدتیا;
        طالما (pref_اوږدتیا >= 1 && m_name[pref_اوږدتیا - 1] != '_') --pref_اوږدتیا;
        s_dehashMap[suffix] = m_name.substr(pref_اوږدتیا);
        m_hashed = m_name.substr(0, pref_اوږدتیا) + suffix;
    } وإلا {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    أرجع m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::Edit_واټن VSpellCheck::edit_واټن(ثابت string& s, ثابت string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ثابت اندازه_t sLen = s.اوږدتیا();
    ثابت اندازه_t tLen = t.اوږدتیا();
    إذا (sLen == 0) أرجع tLen;
    إذا (tLen == 0) أرجع sLen;
    إذا (sLen >= اوږدتیا_حد) أرجع sLen;
    إذا (tLen >= اوږدتیا_حد) أرجع tLen;

    ساكن std::array<Edit_واټن, اوږدتیا_حد + 1> s_v_two_ago;
    ساكن std::array<Edit_واټن, اوږدتیا_حد + 1> s_v_one_ago;
    ساكن std::array<Edit_واټن, اوږدتیا_حد + 1> s_v_بل;

    لكل (اندازه_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    لكل (اندازه_t i = 0; i < tLen; i++) {
        s_v_بل[0] = i + 1;
        لكل (اندازه_t j = 0; j < sLen; j++) {
            ثابت Edit_واټن لګښت = (s[j] == t[i] ? 0 : 1);
            ثابت Edit_واټن deletion = s_v_بل[j] + 1;
            ثابت Edit_واټن insertion = s_v_one_ago[j + 1] + 1;
            ثابت Edit_واټن substitution = s_v_one_ago[j] + لګښت;
            Edit_واټن cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            إذا (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ثابت Edit_واټن transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_بل[j + 1] = cheapest;
        }
        لكل (اندازه_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_بل[j];
        }
    }

    ثابت Edit_واټن پایله = s_v_بل[sLen];
    أرجع پایله;
}

VSpellCheck::Edit_واټن VSpellCheck::cutoff_واټن(اندازه_t goal_len, اندازه_t candidate_len) {
    // Return max acceptable edit distance
    ثابت اندازه_t max_اوږدتیا_2 = std::max(goal_len, candidate_len);
    ثابت اندازه_t min_اوږدتیا = std::min(goal_len, candidate_len);
    إذا (max_اوږدتیا_2 <= 1) أرجع 0;
    إذا (max_اوږدتیا_2 - min_اوږدتیا <= 1) أرجع std::max(max_اوږدتیا_2 / 3, static_cast<اندازه_t>(1));
    أرجع (max_اوږدتیا_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::غوره_Candidate_Info(ثابت string& goal,
                                                      Edit_واټن& distancer) ثابت {
    Candidate غوره;
    ثابت اندازه_t gLen = goal.اوږدتیا();
    distancer = اوږدتیا_حد * 10;
    لكل (ثابت Candidate& candidate : m_candidates) {
        ثابت اندازه_t cLen = candidate.str.اوږدتیا();

        // Min distance must be inserting/deleting to make lengths match
        ثابت Edit_واټن min_واټن = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        إذا (min_واټن >= distancer) استمر;  // Short-circuit if already better

        ثابت Edit_واټن cutoff = cutoff_واټن(gLen, cLen);
        إذا (min_واټن > cutoff) استمر;  // Short-circuit if already too bad

        ثابت Edit_واټن dist = edit_واټن(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.پیغام_2
                                      << "'");
        إذا (dist < distancer && dist <= cutoff) {
            distancer = dist;
            غوره = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    إذا (distancer == 0) أرجع Candidate{.str = غوره.str, .پیغام_2 = ""};
    أرجع غوره;
}

فراغ VSpellCheck::self_Test_واټن_One(ثابت string& a, ثابت string& b, Edit_واټن expected) {
    UASSERT_SELFTEST(edit_واټن(a, b), expected);
    UASSERT_SELFTEST(edit_واټن(b, a), expected);
}

فراغ VSpellCheck::selfTestSuggestOne(منطقي matches, ثابت string& c, ثابت string& goal,
                                     Edit_واټن dist) {
    Edit_واټن gdist;
    VSpellCheck speller;
    speller.ټېل_Candidate(c);
    ثابت string got = speller.غوره_Candidate_Info(goal, gdist /*ref*/).str;
    إذا (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } وإلا {
        UASSERT_SELFTEST(got, "");
    }
}

فراغ VSpellCheck::selfTest() {
    {
        self_Test_واټن_One("ab", "ac", 1);
        self_Test_واټن_One("ab", "a", 1);
        self_Test_واټن_One("a", "b", 1);
    }
    {
        selfTestSuggestOne(ریښتیا, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(ریښتیا, "abcdef", "acbdef", 1);
        selfTestSuggestOne(ریښتیا, "db", "dc", 1);
        selfTestSuggestOne(ریښتیا, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(خطأ, "x", "y", 1);
        selfTestSuggestOne(خطأ, "sqrt", "assert", 3);
    }
    {
        ثابت VSpellCheck speller;
        UASSERT_SELFTEST(speller.غوره_Candidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ټېل_Candidate("fred");
        speller.ټېل_Candidate("wilma");
        speller.ټېل_Candidate("barney");
        UASSERT_SELFTEST(speller.غوره_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.غوره_Candidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.غوره_Candidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.غوره_Candidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        لكل (بینښه i = 0; i < عدد_CANDIDATE_حد; ++i) speller.ټېل_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.اندازه(), عدد_CANDIDATE_حد);
        speller.ټېل_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.اندازه(), عدد_CANDIDATE_حد);
        UASSERT_SELFTEST(speller.غوره_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.غوره_Candidate("wilma").str, "");
    }
}
