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
#جيڪڏهن_پورو

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3String.h"

#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#جيڪڏهن_پورو

#ضمن <algorithm>
#ضمن <fcntl.h>

سائيز_t VName::s_min_ڊيگهائي = 32;
سائيز_t VName::s_max_ڊيگهائي = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
سانچو <منطقي Even = سچ>
ساكن منطقي wild_ميلاپ_Impl(ثابت حرف* s, ثابت حرف* p) VL_خالص {
    لكل (; *p; s++, p++) {
        إذا (*p != '*') {
            إذا (((*s) != (*p)) && *p != '?') أرجع خطأ;
        } وإلا {
            // Trailing star matches everything.
            إذا (!*++p) أرجع سچ;
            طالما (!wild_ميلاپ_Impl<!Even>(s, p)) {
                إذا (*++s == '\0') أرجع خطأ;
            }
            أرجع سچ;
        }
    }
    أرجع (*s == '\0');
}

منطقي VString::wildmatch(ثابت حرف* s, ثابت حرف* p) VL_خالص {
    إذا (*s == '\0') {
        طالما (*p == '*') ++p;
        أرجع *p == '\0';
    }
    أرجع wild_ميلاپ_Impl(s, p);
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
    string نتيجو = str;
    لكل (حرف& cr : نتيجو) cr = std::tolower(cr);
    أرجع نتيجو;
}

string VString::upcase(ثابت string& str) VL_خالص {
    string نتيجو = str;
    لكل (حرف& cr : نتيجو) cr = std::وڏو_اکر(cr);
    أرجع نتيجو;
}

string VString::quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_خالص {
    string نتيجو;
    لكل (ثابت حرف c : str) {
        إذا (c == tgt) نتيجو += esc;
        نتيجو += c;
    }
    أرجع نتيجو;
}

string VString::dequotePercent(ثابت string& str) {
    string نتيجو;
    حرف آخري = '\0';
    لكل (ثابت حرف c : str) {
        إذا (آخري == '%' && c == '%') {
            آخري = '\0';
        } وإلا {
            نتيجو += c;
            آخري = c;
        }
    }
    أرجع نتيجو;
}

string VString::quoteStringLiteralForShell(ثابت string& str) {
    string نتيجو;
    constexpr حرف dquote = '"';
    constexpr حرف escape = '\\';
    نتيجو.ڌڪو_پٺ(dquote);  // Start quoted string
    نتيجو.ڌڪو_پٺ(escape);
    نتيجو.ڌڪو_پٺ(dquote);  // "
    لكل (ثابت حرف c : str) {
        إذا (c == dquote || c == escape) نتيجو.ڌڪو_پٺ(escape);
        نتيجو.ڌڪو_پٺ(c);
    }
    نتيجو.ڌڪو_پٺ(escape);
    نتيجو.ڌڪو_پٺ(dquote);  // "
    نتيجو.ڌڪو_پٺ(dquote);  // Terminate quoted string
    أرجع نتيجو;
}

string VString::escape_String_For_رستو(ثابت string& str) {
    إذا (str.ڳوليو(R"(\\)") != string::npos)
        أرجع str;  // if it has been escaped already, don't do it again
    إذا (str.ڳوليو('/') != string::npos) أرجع str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string نتيجو;
    constexpr حرف space = ' ';  // escape space like this `Program Files`
    constexpr حرف escape = '\\';
    لكل (ثابت حرف c : str) {
        إذا (c == space || c == escape) نتيجو.ڌڪو_پٺ(escape);
        نتيجو.ڌڪو_پٺ(c);
    }
    أرجع نتيجو;
}

ساكن صحيح vl_decodexdigit(حرف c) {
    أرجع std::انگ_آهي(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ثابت string& متن, string& err_ٻاهريون) {
    منطقي quoted = خطأ;
    string newtext;
    newtext.reserve(متن.سائيز());
    بنانشان حرف octal_val = 0;
    صحيح octal_digits = 0;
    لكل (string::const_iterator cp = متن.begin(); cp != متن.پڇاڙي(); ++cp) {
        إذا (quoted) {
            إذا (std::انگ_آهي(*cp)) {
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
                } وإلا إذا (std::اکر_يا_انگ(*cp)) {
                    err_ٻاهريون = "Unknown escape sequence: \\";
                    err_ٻاهريون += *cp;
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
            quoted = سچ;
            octal_digits = 0;
        } وإلا {
            newtext += *cp;
        }
    }
    أرجع newtext;
}

سائيز_t VString::quoted_پڇاڙي(ثابت string& str, سائيز_t pos) VL_خالص {
    لكل (سائيز_t i = pos + 1; i < str.سائيز(); ++i) {
        إذا (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } وإلا إذا (str[i] == '"') {
            أرجع i + 1;
        }
    }
    أرجع string::npos;
}

string VString::spaceUnprintable(ثابت string& str) VL_خالص {
    string نتيجو;
    لكل (ثابت حرف c : str) {
        إذا (std::isprint(c)) {
            نتيجو += c;
        } وإلا {
            نتيجو += ' ';
        }
    }
    أرجع نتيجو;
}

string VString::removeWhitespace(ثابت string& str) {
    string نتيجو;
    نتيجو.reserve(str.سائيز());
    لكل (ثابت حرف c : str) {
        إذا (!std::خال_آهي(c)) نتيجو += c;
    }
    أرجع نتيجو;
}

string VString::trimWhitespace(ثابت string& str) {
    string نتيجو;
    نتيجو.reserve(str.سائيز());
    string جوڙيو;
    منطقي newline = خطأ;
    لكل (ثابت حرف c : str) {
        إذا (newline && std::خال_آهي(c)) استمر;
        إذا (c == '\n') {
            جوڙيو = "\n";
            newline = سچ;
            استمر;
        }
        إذا (std::خال_آهي(c)) {
            جوڙيو += c;
            استمر;
        }
        إذا (!جوڙيو.خالي_جايون()) {
            نتيجو += جوڙيو;
            newline = خطأ;
            جوڙيو.clear();
        }
        نتيجو += c;
    }
    أرجع نتيجو;
}

منطقي VString::isIdentifier(ثابت string& str) {
    لكل (ثابت حرف c : str) {
        إذا (!isIdentifierChar(c)) أرجع خطأ;
    }
    أرجع سچ;
}

منطقي VString::isWhitespace(ثابت string& str) {
    لكل (ثابت حرف c : str) {
        إذا (!std::خال_آهي(c)) أرجع خطأ;
    }
    أرجع سچ;
}

string::سائيز_type VString::leading_Whitespace_ڳڻپ(ثابت string& str) {
    string::سائيز_type نتيجو = 0;
    لكل (ثابت حرف c : str) {
        ++نتيجو;
        إذا (!std::خال_آهي(c)) اكسر;
    }
    أرجع نتيجو;
}

مزدوج VString::parseDouble(ثابت string& str, منطقي* successp) {
    حرف* ثابت strgp = جديد حرف[str.سائيز() + 1];
    حرف* dp = strgp;
    إذا (successp) *successp = سچ;
    لكل (ثابت حرف* sp = str.c_str(); *sp; ++sp) {
        إذا (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    حرف* endp = strgp;
    ثابت مزدوج d = strtod(strgp, &endp);
    ثابت سائيز_t parsed_len = endp - strgp;
    إذا (parsed_len != std::لڙي_ڊيگهه(strgp)) {
        إذا (successp) *successp = خطأ;
    }
    VL_DO_DANGLING(ميٽيو[] strgp, strgp);
    أرجع d;
}

string VString::replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& منزل) {
    string نتيجو = str;
    ثابت سائيز_t fromLen = from.سائيز();
    ثابت سائيز_t منزل_Len = منزل.سائيز();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    لكل (سائيز_t pos = 0; (pos = نتيجو.ڳوليو(from, pos)) != string::npos; pos += منزل_Len) {
        نتيجو.replace(pos, fromLen, منزل);
    }
    أرجع نتيجو;
}

string VString::replace_لفظ(ثابت string& str, ثابت string& from, ثابت string& منزل) {
    string نتيجو = str;
    ثابت سائيز_t len = from.سائيز();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    لكل (سائيز_t pos = 0; (pos = نتيجو.ڳوليو(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        إذا (((pos > 0) && VString::isIdentifierChar(نتيجو[pos - 1])) ||  //
            ((pos + len < نتيجو.سائيز()) && VString::isIdentifierChar(نتيجو[pos + len]))) {
            استمر;
        }
        نتيجو.replace(pos, len, منزل);
    }
    أرجع نتيجو;
}

std::deque<string> VString::ورهايو(ثابت string& str, حرف delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    طالما (std::getline(is, token, delimiter)) results.ڌڪو_پٺ(token);
    أرجع results;
}

منطقي VString::پڇاڙيون_With(ثابت string& str, ثابت string& suffix) {
    إذا (str.ڊيگهائي() < suffix.ڊيگهائي()) أرجع خطأ;
    أرجع str.compare(str.ڊيگهائي() - suffix.ڊيگهائي(), suffix.ڊيگهائي(), suffix) == 0;
}
string VString::aOrAn(ثابت حرف* لفظ) {
    بدل (لفظ[0]) {
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
    ثابت حرف* ڪنجي = str.c_str();
    ثابت سائيز_t len = str.سائيز();
    constexpr uint64_t ٻج = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr صحيح r = 47;

    uint64_t h = ٻج ^ (len * m);

    ثابت uint64_t* معلومات = reinterpret_cast<ثابت uint64_t*>(ڪنجي);
    ثابت uint64_t* پڇاڙي = معلومات + (len / 8);

    طالما (معلومات != پڇاڙي) {
        uint64_t k = *معلومات++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ثابت بنانشان حرف* data2 = reinterpret_cast<ثابت بنانشان حرف*>(معلومات);

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
    سائيز_t len = str.سائيز();
    string نتيجو;
    نتيجو.reserve(len * 4 / 3 + 2);
    سائيز_t pos = 0;
    إذا (len >= 3) {
        لكل (; pos < len - 2; pos += 3) {
            نتيجو += digits[((str[pos] >> 2) & 0x3f)];
            نتيجو
                += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(str[pos + 1] & 0xf0) >> 4)];
            نتيجو += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<صحيح>(str[pos + 2] & 0xc0) >> 6)];
            نتيجو += digits[((str[pos + 2] & 0x3f))];
        }
    }
    إذا (len >= 2 && pos < len - 1) {  // Pad 2
        نتيجو += digits[((str[pos] >> 2) & 0x3f)];
        نتيجو += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(str[pos + 1] & 0xf0) >> 4)];
        نتيجو += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<صحيح>(0) >> 6)];
        نتيجو += '=';
    } وإلا إذا (pos < len) {  // Pad 1
        نتيجو += digits[((str[pos] >> 2) & 0x3f)];
        نتيجو += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(0) >> 4)];
        نتيجو += '=';
        نتيجو += '=';
    }
    أرجع نتيجو;
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
ساكن فراغ sha512_بلاڪ(uint64_t* h, ثابت uint64_t* chunk) VL_خالص {
    uint64_t ah[8];
    ثابت uint64_t* p = chunk;

    // Initialize working variables to current hash value
    لكل (بنانشان i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    لكل (بنانشان i = 0; i < 5; ++i) {
        لكل (بنانشان j = 0; j < 16; ++j) {
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
    لكل (بنانشان i = 0; i < 8; ++i) h[i] += ah[i];
}

فراغ VHashSha512::وجهو(ثابت فراغ* datap, سائيز_t ڊيگهائي) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_ڊيگهائي += ڊيگهائي;

    string عارضي_معلومات;
    صحيح chunkLen;
    ثابت uint8_t* chunkp;
    إذا (m_remainder == "") {
        chunkLen = ڊيگهائي;
        chunkp = static_cast<ثابت uint8_t*>(datap);
    } وإلا {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        عارضي_معلومات = m_remainder + std::string{static_cast<ثابت حرف*>(datap), ڊيگهائي};
        chunkLen = عارضي_معلومات.ڊيگهائي();
        chunkp = reinterpret_cast<ثابت uint8_t*>(عارضي_معلومات.معلومات());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    صحيح posBegin = 0;  // Position in buffer for start of this block
    صحيح pos_پڇاڙي = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    طالما (posBegin <= chunkLen - 128) {
        pos_پڇاڙي = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        لكل (صحيح ڦيرو_Byte = 0; posBegin < pos_پڇاڙي; posBegin += 8) {
            w[ڦيرو_Byte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
                              | (static_cast<uint64_t>(chunkp[posBegin + 6]) << 8)
                              | (static_cast<uint64_t>(chunkp[posBegin + 5]) << 16)
                              | (static_cast<uint64_t>(chunkp[posBegin + 4]) << 24)
                              | (static_cast<uint64_t>(chunkp[posBegin + 3]) << 32)
                              | (static_cast<uint64_t>(chunkp[posBegin + 2]) << 40)
                              | (static_cast<uint64_t>(chunkp[posBegin + 1]) << 48)
                              | (static_cast<uint64_t>(chunkp[posBegin]) << 56));
        }
        sha512_بلاڪ(m_inthash, w);
    }

    m_remainder = std::string(reinterpret_cast<ثابت حرف*>(chunkp + posBegin), chunkLen - pos_پڇاڙي);
}

فراغ VHashSha512::وجهو_File(ثابت string& filename) {
    ساكن constexpr سائيز_t بفر_سائيز = 64 * 1024;

    ثابت صحيح fd = ::open(filename.c_str(), O_RDONLY);
    إذا (fd < 0) أرجع;

    std::array<حرف, بفر_سائيز + 1> buf;
    طالما (ثابت ssize_t got = ::اقرأ(fd, &buf, بفر_سائيز)) {
        إذا (got <= 0) اكسر;
        وجهو(&buf, got);
    }
    ::close(fd);
}

فراغ VHashSha512::finalize() {
    إذا (!m_final) {
        // Make sure no 128 byte blocks left
        وجهو("");
        m_final = سچ;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        لكل (صحيح i = 0; i < 16; ++i) w[i] = 0;
        سائيز_t بلاڪ_Pos = 0;
        لكل (; بلاڪ_Pos < m_remainder.ڊيگهائي(); ++بلاڪ_Pos) {
            w[بلاڪ_Pos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[بلاڪ_Pos])))
                    << ((7 - (بلاڪ_Pos & 7)) << 3));
        }
        w[بلاڪ_Pos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (بلاڪ_Pos & 7)) << 3);
        إذا (m_remainder.ڊيگهائي() >= 112) {
            sha512_بلاڪ(m_inthash, w);
            لكل (صحيح i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_ڊيگهائي << 3;
        sha512_بلاڪ(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ٻٽو() {
    finalize();
    string نتيجو;
    نتيجو.reserve(64);
    لكل (سائيز_t i = 0; i < 64; ++i) {
        نتيجو += static_cast<حرف>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    أرجع نتيجو;
}

uint64_t VHashSha512::digestUInt64() {
    ثابت string& binhash = digest_ٻٽو();
    uint64_t نتيجو = 0;
    لكل (سائيز_t byte = 0; byte < حجم(uint64_t); ++byte) {
        ثابت بنانشان حرف c = binhash[byte];
        نتيجو = (نتيجو << 8) | c;
    }
    أرجع نتيجو;
}

string VHashSha512::digestHex() {
    ساكن constexpr ثابت حرف* ثابت digits = "0123456789abcdef";
    ثابت string& binhash = digest_ٻٽو();
    string نتيجو;
    نتيجو.reserve(128);
    لكل (سائيز_t byte = 0; byte < 64; ++byte) {
        نتيجو += digits[(binhash[byte] >> 4) & 0xf];
        نتيجو += digits[(binhash[byte] >> 0) & 0xf];
    }
    أرجع نتيجو;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    أرجع VString::base64Enc(digest_ٻٽو());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ساكن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ثابت string& binhash = digest_ٻٽو();
    string نتيجو;
    نتيجو.reserve(84);
    صحيح pos = 0;
    لكل (; pos < (512 / 8) - 2; pos += 3) {
        نتيجو += digits[((binhash[pos] >> 2) & 0x3f)];
        نتيجو += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<صحيح>(binhash[pos + 1] & 0xf0) >> 4)];
        نتيجو += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<صحيح>(binhash[pos + 2] & 0xc0) >> 6)];
        نتيجو += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        إذا (نتيجو.سائيز() >= DIGEST_SYMBOL24_ڊيگهائي) اكسر;
    }
    // Any leftover bits don't matter for our purpose
    أرجع نتيجو;
}

فراغ VHashSha512::selfTestOne(ثابت string& معلومات, ثابت string& data2, ثابت string& قوت_نما,
                              ثابت string& exp64, ثابت string& exp24) {
    VHashSha512 digest{معلومات};
    إذا (data2 != "") digest.وجهو(data2);
    إذا (VL_UNCOVERABLE(digest.digestHex() != قوت_نما)) {
        std::cerr << "%Error: When hashing '" << معلومات + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << قوت_نما << سٽ_پڄاڻي;  // LCOV_EXCL_LINE
    }
    إذا (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << معلومات + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << سٽ_پڄاڻي;  // LCOV_EXCL_LINE
    }
    إذا (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << معلومات + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << سٽ_پڄاڻي;  // LCOV_EXCL_LINE
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

string VName::dehash(ثابت string& اندريون) {
    ساكن constexpr ثابت حرف VHSH[] = "__Vhsh";
    ساكن constexpr سائيز_t VHSH_LEN = حجم(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_ڊيگهائي;
    ساكن ثابت سائيز_t DOT_LEN = std::لڙي_ڊيگهه("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    لكل (string::سائيز_type آخري_dot_pos = 0; آخري_dot_pos < اندريون.سائيز();) {
        ثابت string::سائيز_type ايندڙ_dot_pos = اندريون.ڳوليو("__DOT__", آخري_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ثابت خودڪار ڳولا_begin = std::begin(اندريون) + آخري_dot_pos;
        ثابت خودڪار ڳولا_پڇاڙي
            = ايندڙ_dot_pos == string::npos ? std::پڇاڙي(اندريون) : std::begin(اندريون) + ايندڙ_dot_pos;

        // Search for __Vhsh between the two dots.
        ثابت خودڪار begin_vhsh
            = std::ڳولا(ڳولا_begin, ڳولا_پڇاڙي, std::begin(VHSH), std::پڇاڙي(VHSH) - 1);
        إذا (begin_vhsh != ڳولا_پڇاڙي) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ثابت خودڪار پڇاڙي_vhsh
                = begin_vhsh + std::min<سائيز_t>(std::مفاصلو(begin_vhsh, ڳولا_پڇاڙي), VHSH_LEN);
            ثابت std::string vhsh{begin_vhsh, پڇاڙي_vhsh};
            ثابت خودڪار& it = s_dehashMap.ڳوليو(vhsh);
            UASSERT(it != s_dehashMap.پڇاڙي(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            إذا (آخري_dot_pos > 0 && dehashed.خالي_جايون()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = اندريون.substr(0, آخري_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{ڳولا_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->ٻيو;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{پڇاڙي_vhsh, ڳولا_پڇاڙي};
        }
        // This component doesn't need dehashing but a previous one might have.
        وإلا إذا (!dehashed.خالي_جايون()) {
            dehashed += std::string{ڳولا_begin, ڳولا_پڇاڙي};
        }

        إذا (ايندڙ_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            إذا (!dehashed.خالي_جايون()) dehashed += "__DOT__";
            آخري_dot_pos = ايندڙ_dot_pos + DOT_LEN;
        } وإلا {
            آخري_dot_pos = string::npos;
        }
    }
    أرجع dehashed.خالي_جايون() ? اندريون : dehashed;
}

string VName::hashedName() {
    إذا (m_name == "") أرجع "";
    إذا (m_hashed != "") أرجع m_hashed;  // Memoized
    إذا (s_max_ڊيگهائي == 0 || m_name.ڊيگهائي() < s_max_ڊيگهائي) {
        m_hashed = m_name;
        أرجع m_hashed;
    }
    VHashSha512 hash{m_name};
    ثابت string suffix = "__Vhsh" + hash.digestSymbol24();
    إذا (s_min_ڊيگهائي < s_max_ڊيگهائي) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        سائيز_t pref_ڊيگهائي = s_min_ڊيگهائي;
        طالما (pref_ڊيگهائي >= 1 && m_name[pref_ڊيگهائي - 1] != '_') --pref_ڊيگهائي;
        s_dehashMap[suffix] = m_name.substr(pref_ڊيگهائي);
        m_hashed = m_name.substr(0, pref_ڊيگهائي) + suffix;
    } وإلا {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    أرجع m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::Edit_مفاصلو VSpellCheck::edit_مفاصلو(ثابت string& s, ثابت string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ثابت سائيز_t sLen = s.ڊيگهائي();
    ثابت سائيز_t tLen = t.ڊيگهائي();
    إذا (sLen == 0) أرجع tLen;
    إذا (tLen == 0) أرجع sLen;
    إذا (sLen >= ڊيگهائي_حد_بندي) أرجع sLen;
    إذا (tLen >= ڊيگهائي_حد_بندي) أرجع tLen;

    ساكن std::array<Edit_مفاصلو, ڊيگهائي_حد_بندي + 1> s_v_two_ago;
    ساكن std::array<Edit_مفاصلو, ڊيگهائي_حد_بندي + 1> s_v_one_ago;
    ساكن std::array<Edit_مفاصلو, ڊيگهائي_حد_بندي + 1> s_v_ايندڙ;

    لكل (سائيز_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    لكل (سائيز_t i = 0; i < tLen; i++) {
        s_v_ايندڙ[0] = i + 1;
        لكل (سائيز_t j = 0; j < sLen; j++) {
            ثابت Edit_مفاصلو لاڳت = (s[j] == t[i] ? 0 : 1);
            ثابت Edit_مفاصلو deletion = s_v_ايندڙ[j] + 1;
            ثابت Edit_مفاصلو insertion = s_v_one_ago[j + 1] + 1;
            ثابت Edit_مفاصلو substitution = s_v_one_ago[j] + لاڳت;
            Edit_مفاصلو cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            إذا (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ثابت Edit_مفاصلو transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_ايندڙ[j + 1] = cheapest;
        }
        لكل (سائيز_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_ايندڙ[j];
        }
    }

    ثابت Edit_مفاصلو نتيجو = s_v_ايندڙ[sLen];
    أرجع نتيجو;
}

VSpellCheck::Edit_مفاصلو VSpellCheck::cutoff_مفاصلو(سائيز_t goal_len, سائيز_t candidate_len) {
    // Return max acceptable edit distance
    ثابت سائيز_t max_ڊيگهائي_2 = std::max(goal_len, candidate_len);
    ثابت سائيز_t min_ڊيگهائي = std::min(goal_len, candidate_len);
    إذا (max_ڊيگهائي_2 <= 1) أرجع 0;
    إذا (max_ڊيگهائي_2 - min_ڊيگهائي <= 1) أرجع std::max(max_ڊيگهائي_2 / 3, static_cast<سائيز_t>(1));
    أرجع (max_ڊيگهائي_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::بهترين_Candidate_Info(ثابت string& goal,
                                                      Edit_مفاصلو& distancer) ثابت {
    Candidate بهترين;
    ثابت سائيز_t gLen = goal.ڊيگهائي();
    distancer = ڊيگهائي_حد_بندي * 10;
    لكل (ثابت Candidate& candidate : m_candidates) {
        ثابت سائيز_t cLen = candidate.str.ڊيگهائي();

        // Min distance must be inserting/deleting to make lengths match
        ثابت Edit_مفاصلو min_مفاصلو = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        إذا (min_مفاصلو >= distancer) استمر;  // Short-circuit if already better

        ثابت Edit_مفاصلو cutoff = cutoff_مفاصلو(gLen, cLen);
        إذا (min_مفاصلو > cutoff) استمر;  // Short-circuit if already too bad

        ثابت Edit_مفاصلو dist = edit_مفاصلو(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.نياپو
                                      << "'");
        إذا (dist < distancer && dist <= cutoff) {
            distancer = dist;
            بهترين = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    إذا (distancer == 0) أرجع Candidate{.str = بهترين.str, .نياپو = ""};
    أرجع بهترين;
}

فراغ VSpellCheck::self_Test_مفاصلو_One(ثابت string& a, ثابت string& b, Edit_مفاصلو expected) {
    UASSERT_SELFTEST(edit_مفاصلو(a, b), expected);
    UASSERT_SELFTEST(edit_مفاصلو(b, a), expected);
}

فراغ VSpellCheck::selfTestSuggestOne(منطقي matches, ثابت string& c, ثابت string& goal,
                                     Edit_مفاصلو dist) {
    Edit_مفاصلو gdist;
    VSpellCheck speller;
    speller.ڌڪو_Candidate(c);
    ثابت string got = speller.بهترين_Candidate_Info(goal, gdist /*ref*/).str;
    إذا (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } وإلا {
        UASSERT_SELFTEST(got, "");
    }
}

فراغ VSpellCheck::selfTest() {
    {
        self_Test_مفاصلو_One("ab", "ac", 1);
        self_Test_مفاصلو_One("ab", "a", 1);
        self_Test_مفاصلو_One("a", "b", 1);
    }
    {
        selfTestSuggestOne(سچ, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(سچ, "abcdef", "acbdef", 1);
        selfTestSuggestOne(سچ, "db", "dc", 1);
        selfTestSuggestOne(سچ, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(خطأ, "x", "y", 1);
        selfTestSuggestOne(خطأ, "sqrt", "assert", 3);
    }
    {
        ثابت VSpellCheck speller;
        UASSERT_SELFTEST(speller.بهترين_Candidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ڌڪو_Candidate("fred");
        speller.ڌڪو_Candidate("wilma");
        speller.ڌڪو_Candidate("barney");
        UASSERT_SELFTEST(speller.بهترين_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.بهترين_Candidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.بهترين_Candidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.بهترين_Candidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        لكل (بنانشان i = 0; i < انگ_CANDIDATE_حد_بندي; ++i) speller.ڌڪو_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.سائيز(), انگ_CANDIDATE_حد_بندي);
        speller.ڌڪو_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.سائيز(), انگ_CANDIDATE_حد_بندي);
        UASSERT_SELFTEST(speller.بهترين_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.بهترين_Candidate("wilma").str, "");
    }
}
