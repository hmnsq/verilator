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

#شامل "config_build.h"
#شامل "verilatedos.h"

#اگر defined(_WIN32) || defined(__MINGW32__)
#شامل <io.h>  // open, read, write, close
#اگر_ختم

#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3String.h"

#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
#شامل "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#اگر_ختم

#شامل <algorithm>
#شامل <fcntl.h>

سائز_t VName::s_min_لمبائی = 32;
سائز_t VName::s_max_لمبائی = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
سانچہ <منطقی Even = سچ>
ساکن منطقی wild_مطابقت_Impl(ثابت حرف* s, ثابت حرف* p) VL_خالص {
    برائے (; *p; s++, p++) {
        اگر (*p != '*') {
            اگر (((*s) != (*p)) && *p != '?') واپس جھوٹ;
        } ورنہ {
            // Trailing star matches everything.
            اگر (!*++p) واپس سچ;
            جب (!wild_مطابقت_Impl<!Even>(s, p)) {
                اگر (*++s == '\0') واپس جھوٹ;
            }
            واپس سچ;
        }
    }
    واپس (*s == '\0');
}

منطقی VString::wildmatch(ثابت حرف* s, ثابت حرف* p) VL_خالص {
    اگر (*s == '\0') {
        جب (*p == '*') ++p;
        واپس *p == '\0';
    }
    واپس wild_مطابقت_Impl(s, p);
}

منطقی VString::wildmatch(ثابت string& s, ثابت string& p) VL_خالص {
    واپس wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ثابت string& a, ثابت string& dot, ثابت string& b) {
    اگر (b == "") واپس a;
    اگر (a == "") واپس b;
    واپس a + dot + b;
}

string VString::downcase(ثابت string& str) VL_خالص {
    string نتیجہ = str;
    برائے (حرف& cr : نتیجہ) cr = std::tolower(cr);
    واپس نتیجہ;
}

string VString::upcase(ثابت string& str) VL_خالص {
    string نتیجہ = str;
    برائے (حرف& cr : نتیجہ) cr = std::بڑا_حرف(cr);
    واپس نتیجہ;
}

string VString::quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_خالص {
    string نتیجہ;
    برائے (ثابت حرف c : str) {
        اگر (c == tgt) نتیجہ += esc;
        نتیجہ += c;
    }
    واپس نتیجہ;
}

string VString::dequotePercent(ثابت string& str) {
    string نتیجہ;
    حرف آخری = '\0';
    برائے (ثابت حرف c : str) {
        اگر (آخری == '%' && c == '%') {
            آخری = '\0';
        } ورنہ {
            نتیجہ += c;
            آخری = c;
        }
    }
    واپس نتیجہ;
}

string VString::quoteStringLiteralForShell(ثابت string& str) {
    string نتیجہ;
    constexpr حرف dquote = '"';
    constexpr حرف escape = '\\';
    نتیجہ.ڈالو_عقب(dquote);  // Start quoted string
    نتیجہ.ڈالو_عقب(escape);
    نتیجہ.ڈالو_عقب(dquote);  // "
    برائے (ثابت حرف c : str) {
        اگر (c == dquote || c == escape) نتیجہ.ڈالو_عقب(escape);
        نتیجہ.ڈالو_عقب(c);
    }
    نتیجہ.ڈالو_عقب(escape);
    نتیجہ.ڈالو_عقب(dquote);  // "
    نتیجہ.ڈالو_عقب(dquote);  // Terminate quoted string
    واپس نتیجہ;
}

string VString::escape_String_For_راستہ(ثابت string& str) {
    اگر (str.ڈھونڈو(R"(\\)") != string::npos)
        واپس str;  // if it has been escaped already, don't do it again
    اگر (str.ڈھونڈو('/') != string::npos) واپس str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string نتیجہ;
    constexpr حرف space = ' ';  // escape space like this `Program Files`
    constexpr حرف escape = '\\';
    برائے (ثابت حرف c : str) {
        اگر (c == space || c == escape) نتیجہ.ڈالو_عقب(escape);
        نتیجہ.ڈالو_عقب(c);
    }
    واپس نتیجہ;
}

ساکن صحیح vl_decodexdigit(حرف c) {
    واپس std::ہندسہ_ہے(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ثابت string& متن, string& err_برآمد) {
    منطقی quoted = جھوٹ;
    string newtext;
    newtext.reserve(متن.سائز());
    غیرعلامتی حرف octal_val = 0;
    صحیح octal_digits = 0;
    برائے (string::const_iterator cp = متن.begin(); cp != متن.انتہا(); ++cp) {
        اگر (quoted) {
            اگر (std::ہندسہ_ہے(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                اگر (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = جھوٹ;
                }
            } ورنہ {
                اگر (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = جھوٹ;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    جاری;
                }
                quoted = جھوٹ;
                اگر (*cp == 'n') {
                    newtext += '\n';
                } ورنہ اگر (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } ورنہ اگر (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } ورنہ اگر (*cp == 'r') {
                    newtext += '\r';
                } ورنہ اگر (*cp == 't') {
                    newtext += '\t';
                } ورنہ اگر (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } ورنہ اگر (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<حرف>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } ورنہ اگر (std::حرف_یا_ہندسہ(*cp)) {
                    err_برآمد = "Unknown escape sequence: \\";
                    err_برآمد += *cp;
                    توڑو;
                } ورنہ {
                    newtext += *cp;
                }
            }
        } ورنہ اگر (*cp == '\\') {
            اگر (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = سچ;
            octal_digits = 0;
        } ورنہ {
            newtext += *cp;
        }
    }
    واپس newtext;
}

سائز_t VString::quoted_انتہا(ثابت string& str, سائز_t pos) VL_خالص {
    برائے (سائز_t i = pos + 1; i < str.سائز(); ++i) {
        اگر (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } ورنہ اگر (str[i] == '"') {
            واپس i + 1;
        }
    }
    واپس string::npos;
}

string VString::spaceUnprintable(ثابت string& str) VL_خالص {
    string نتیجہ;
    برائے (ثابت حرف c : str) {
        اگر (std::isprint(c)) {
            نتیجہ += c;
        } ورنہ {
            نتیجہ += ' ';
        }
    }
    واپس نتیجہ;
}

string VString::removeWhitespace(ثابت string& str) {
    string نتیجہ;
    نتیجہ.reserve(str.سائز());
    برائے (ثابت حرف c : str) {
        اگر (!std::خلا_ہے(c)) نتیجہ += c;
    }
    واپس نتیجہ;
}

string VString::trimWhitespace(ثابت string& str) {
    string نتیجہ;
    نتیجہ.reserve(str.سائز());
    string جوڑو;
    منطقی newline = جھوٹ;
    برائے (ثابت حرف c : str) {
        اگر (newline && std::خلا_ہے(c)) جاری;
        اگر (c == '\n') {
            جوڑو = "\n";
            newline = سچ;
            جاری;
        }
        اگر (std::خلا_ہے(c)) {
            جوڑو += c;
            جاری;
        }
        اگر (!جوڑو.خالی_خانے()) {
            نتیجہ += جوڑو;
            newline = جھوٹ;
            جوڑو.clear();
        }
        نتیجہ += c;
    }
    واپس نتیجہ;
}

منطقی VString::isIdentifier(ثابت string& str) {
    برائے (ثابت حرف c : str) {
        اگر (!isIdentifierChar(c)) واپس جھوٹ;
    }
    واپس سچ;
}

منطقی VString::isWhitespace(ثابت string& str) {
    برائے (ثابت حرف c : str) {
        اگر (!std::خلا_ہے(c)) واپس جھوٹ;
    }
    واپس سچ;
}

string::سائز_type VString::leading_Whitespace_گنتی(ثابت string& str) {
    string::سائز_type نتیجہ = 0;
    برائے (ثابت حرف c : str) {
        ++نتیجہ;
        اگر (!std::خلا_ہے(c)) توڑو;
    }
    واپس نتیجہ;
}

دوہرا VString::parseDouble(ثابت string& str, منطقی* successp) {
    حرف* ثابت strgp = نیا حرف[str.سائز() + 1];
    حرف* dp = strgp;
    اگر (successp) *successp = سچ;
    برائے (ثابت حرف* sp = str.c_str(); *sp; ++sp) {
        اگر (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    حرف* endp = strgp;
    ثابت دوہرا d = strtod(strgp, &endp);
    ثابت سائز_t parsed_len = endp - strgp;
    اگر (parsed_len != std::لڑی_لمبائی(strgp)) {
        اگر (successp) *successp = جھوٹ;
    }
    VL_DO_DANGLING(حذف[] strgp, strgp);
    واپس d;
}

string VString::replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& مقصد) {
    string نتیجہ = str;
    ثابت سائز_t fromLen = from.سائز();
    ثابت سائز_t مقصد_Len = مقصد.سائز();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    برائے (سائز_t pos = 0; (pos = نتیجہ.ڈھونڈو(from, pos)) != string::npos; pos += مقصد_Len) {
        نتیجہ.replace(pos, fromLen, مقصد);
    }
    واپس نتیجہ;
}

string VString::replace_لفظ(ثابت string& str, ثابت string& from, ثابت string& مقصد) {
    string نتیجہ = str;
    ثابت سائز_t len = from.سائز();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    برائے (سائز_t pos = 0; (pos = نتیجہ.ڈھونڈو(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        اگر (((pos > 0) && VString::isIdentifierChar(نتیجہ[pos - 1])) ||  //
            ((pos + len < نتیجہ.سائز()) && VString::isIdentifierChar(نتیجہ[pos + len]))) {
            جاری;
        }
        نتیجہ.replace(pos, len, مقصد);
    }
    واپس نتیجہ;
}

std::deque<string> VString::بانٹو(ثابت string& str, حرف delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    جب (std::getline(is, token, delimiter)) results.ڈالو_عقب(token);
    واپس results;
}

منطقی VString::سرے_With(ثابت string& str, ثابت string& suffix) {
    اگر (str.لمبائی_2() < suffix.لمبائی_2()) واپس جھوٹ;
    واپس str.compare(str.لمبائی_2() - suffix.لمبائی_2(), suffix.لمبائی_2(), suffix) == 0;
}
string VString::aOrAn(ثابت حرف* لفظ) {
    انتخاب (لفظ[0]) {
    حالت '\0': واپس "";
    حالت 'a':
    حالت 'e':
    حالت 'i':
    حالت 'o':
    حالت 'u': واپس "an";
    طےشدہ: واپس "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ثابت string& str) VL_خالص {
    ثابت حرف* کلید = str.c_str();
    ثابت سائز_t len = str.سائز();
    constexpr uint64_t بیج = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr صحیح r = 47;

    uint64_t h = بیج ^ (len * m);

    ثابت uint64_t* ڈیٹا = reinterpret_cast<ثابت uint64_t*>(کلید);
    ثابت uint64_t* انتہا = ڈیٹا + (len / 8);

    جب (ڈیٹا != انتہا) {
        uint64_t k = *ڈیٹا++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ثابت غیرعلامتی حرف* data2 = reinterpret_cast<ثابت غیرعلامتی حرف*>(ڈیٹا);

    انتخاب (len & 7) {
    حالت 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    حالت 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    حالت 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    حالت 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    حالت 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    حالت 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    حالت 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    طےشدہ:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    واپس h;
}

string VString::base64Enc(ثابت string& str) VL_خالص {
    // Non-URL format (+/), versus URL format (-/).
    ساکن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    سائز_t len = str.سائز();
    string نتیجہ;
    نتیجہ.reserve(len * 4 / 3 + 2);
    سائز_t pos = 0;
    اگر (len >= 3) {
        برائے (; pos < len - 2; pos += 3) {
            نتیجہ += digits[((str[pos] >> 2) & 0x3f)];
            نتیجہ
                += digits[((str[pos] & 0x3) << 4) | (static_cast<صحیح>(str[pos + 1] & 0xf0) >> 4)];
            نتیجہ += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<صحیح>(str[pos + 2] & 0xc0) >> 6)];
            نتیجہ += digits[((str[pos + 2] & 0x3f))];
        }
    }
    اگر (len >= 2 && pos < len - 1) {  // Pad 2
        نتیجہ += digits[((str[pos] >> 2) & 0x3f)];
        نتیجہ += digits[((str[pos] & 0x3) << 4) | (static_cast<صحیح>(str[pos + 1] & 0xf0) >> 4)];
        نتیجہ += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<صحیح>(0) >> 6)];
        نتیجہ += '=';
    } ورنہ اگر (pos < len) {  // Pad 1
        نتیجہ += digits[((str[pos] >> 2) & 0x3f)];
        نتیجہ += digits[((str[pos] & 0x3) << 4) | (static_cast<صحیح>(0) >> 4)];
        نتیجہ += '=';
        نتیجہ += '=';
    }
    واپس نتیجہ;
}

خالی VString::selfTest() {
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

ساکن constexpr uint64_t sha512K[]
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
ساکن uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_خالص {
    واپس lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ساکن خالی sha512_بلاک(uint64_t* h, ثابت uint64_t* chunk) VL_خالص {
    uint64_t ah[8];
    ثابت uint64_t* p = chunk;

    // Initialize working variables to current hash value
    برائے (غیرعلامتی i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    برائے (غیرعلامتی i = 0; i < 5; ++i) {
        برائے (غیرعلامتی j = 0; j < 16; ++j) {
            اگر (i == 0) {
                w[j] = *p++;
            } ورنہ {
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
    برائے (غیرعلامتی i = 0; i < 8; ++i) h[i] += ah[i];
}

خالی VHashSha512::داخل_کرو(ثابت خالی* datap, سائز_t لمبائی_2) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_لمبائی += لمبائی_2;

    string عارضی_ڈیٹا;
    صحیح chunkLen;
    ثابت uint8_t* chunkp;
    اگر (m_remainder == "") {
        chunkLen = لمبائی_2;
        chunkp = static_cast<ثابت uint8_t*>(datap);
    } ورنہ {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        عارضی_ڈیٹا = m_remainder + std::string{static_cast<ثابت حرف*>(datap), لمبائی_2};
        chunkLen = عارضی_ڈیٹا.لمبائی_2();
        chunkp = reinterpret_cast<ثابت uint8_t*>(عارضی_ڈیٹا.ڈیٹا());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    صحیح posBegin = 0;  // Position in buffer for start of this block
    صحیح pos_انتہا = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    جب (posBegin <= chunkLen - 128) {
        pos_انتہا = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        برائے (صحیح باری_Byte = 0; posBegin < pos_انتہا; posBegin += 8) {
            w[باری_Byte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
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

    m_remainder = std::string(reinterpret_cast<ثابت حرف*>(chunkp + posBegin), chunkLen - pos_انتہا);
}

خالی VHashSha512::داخل_کرو_File(ثابت string& filename) {
    ساکن constexpr سائز_t بفر_سائز = 64 * 1024;

    ثابت صحیح fd = ::open(filename.c_str(), O_RDONLY);
    اگر (fd < 0) واپس;

    std::array<حرف, بفر_سائز + 1> buf;
    جب (ثابت ssize_t got = ::اقرأ(fd, &buf, بفر_سائز)) {
        اگر (got <= 0) توڑو;
        داخل_کرو(&buf, got);
    }
    ::close(fd);
}

خالی VHashSha512::finalize() {
    اگر (!m_final) {
        // Make sure no 128 byte blocks left
        داخل_کرو("");
        m_final = سچ;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        برائے (صحیح i = 0; i < 16; ++i) w[i] = 0;
        سائز_t بلاک_Pos = 0;
        برائے (; بلاک_Pos < m_remainder.لمبائی_2(); ++بلاک_Pos) {
            w[بلاک_Pos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[بلاک_Pos])))
                    << ((7 - (بلاک_Pos & 7)) << 3));
        }
        w[بلاک_Pos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (بلاک_Pos & 7)) << 3);
        اگر (m_remainder.لمبائی_2() >= 112) {
            sha512_بلاک(m_inthash, w);
            برائے (صحیح i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_لمبائی << 3;
        sha512_بلاک(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ثنائی() {
    finalize();
    string نتیجہ;
    نتیجہ.reserve(64);
    برائے (سائز_t i = 0; i < 64; ++i) {
        نتیجہ += static_cast<حرف>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    واپس نتیجہ;
}

uint64_t VHashSha512::digestUInt64() {
    ثابت string& binhash = digest_ثنائی();
    uint64_t نتیجہ = 0;
    برائے (سائز_t byte = 0; byte < حجم(uint64_t); ++byte) {
        ثابت غیرعلامتی حرف c = binhash[byte];
        نتیجہ = (نتیجہ << 8) | c;
    }
    واپس نتیجہ;
}

string VHashSha512::digestHex() {
    ساکن constexpr ثابت حرف* ثابت digits = "0123456789abcdef";
    ثابت string& binhash = digest_ثنائی();
    string نتیجہ;
    نتیجہ.reserve(128);
    برائے (سائز_t byte = 0; byte < 64; ++byte) {
        نتیجہ += digits[(binhash[byte] >> 4) & 0xf];
        نتیجہ += digits[(binhash[byte] >> 0) & 0xf];
    }
    واپس نتیجہ;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    واپس VString::base64Enc(digest_ثنائی());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ساکن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ثابت string& binhash = digest_ثنائی();
    string نتیجہ;
    نتیجہ.reserve(84);
    صحیح pos = 0;
    برائے (; pos < (512 / 8) - 2; pos += 3) {
        نتیجہ += digits[((binhash[pos] >> 2) & 0x3f)];
        نتیجہ += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<صحیح>(binhash[pos + 1] & 0xf0) >> 4)];
        نتیجہ += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<صحیح>(binhash[pos + 2] & 0xc0) >> 6)];
        نتیجہ += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        اگر (نتیجہ.سائز() >= DIGEST_SYMBOL24_لمبائی) توڑو;
    }
    // Any leftover bits don't matter for our purpose
    واپس نتیجہ;
}

خالی VHashSha512::selfTestOne(ثابت string& ڈیٹا, ثابت string& data2, ثابت string& قوت_نما,
                              ثابت string& exp64, ثابت string& exp24) {
    VHashSha512 digest{ڈیٹا};
    اگر (data2 != "") digest.داخل_کرو(data2);
    اگر (VL_UNCOVERABLE(digest.digestHex() != قوت_نما)) {
        std::cerr << "%Error: When hashing '" << ڈیٹا + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << قوت_نما << سطر_ختم;  // LCOV_EXCL_LINE
    }
    اگر (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << ڈیٹا + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << سطر_ختم;  // LCOV_EXCL_LINE
    }
    اگر (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << ڈیٹا + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << سطر_ختم;  // LCOV_EXCL_LINE
    }
}

خالی VHashSha512::selfTest() {
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

string VName::dehash(ثابت string& اندر) {
    ساکن constexpr ثابت حرف VHSH[] = "__Vhsh";
    ساکن constexpr سائز_t VHSH_LEN = حجم(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_لمبائی;
    ساکن ثابت سائز_t DOT_LEN = std::لڑی_لمبائی("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    برائے (string::سائز_type آخری_dot_pos = 0; آخری_dot_pos < اندر.سائز();) {
        ثابت string::سائز_type اگلا_dot_pos = اندر.ڈھونڈو("__DOT__", آخری_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ثابت خودکار تلاش_begin = std::begin(اندر) + آخری_dot_pos;
        ثابت خودکار تلاش_انتہا
            = اگلا_dot_pos == string::npos ? std::انتہا(اندر) : std::begin(اندر) + اگلا_dot_pos;

        // Search for __Vhsh between the two dots.
        ثابت خودکار begin_vhsh
            = std::تلاش(تلاش_begin, تلاش_انتہا, std::begin(VHSH), std::انتہا(VHSH) - 1);
        اگر (begin_vhsh != تلاش_انتہا) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ثابت خودکار انتہا_vhsh
                = begin_vhsh + std::min<سائز_t>(std::فاصلہ(begin_vhsh, تلاش_انتہا), VHSH_LEN);
            ثابت std::string vhsh{begin_vhsh, انتہا_vhsh};
            ثابت خودکار& it = s_dehashMap.ڈھونڈو(vhsh);
            UASSERT(it != s_dehashMap.انتہا(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            اگر (آخری_dot_pos > 0 && dehashed.خالی_خانے()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = اندر.substr(0, آخری_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{تلاش_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->دوسرا;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{انتہا_vhsh, تلاش_انتہا};
        }
        // This component doesn't need dehashing but a previous one might have.
        ورنہ اگر (!dehashed.خالی_خانے()) {
            dehashed += std::string{تلاش_begin, تلاش_انتہا};
        }

        اگر (اگلا_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            اگر (!dehashed.خالی_خانے()) dehashed += "__DOT__";
            آخری_dot_pos = اگلا_dot_pos + DOT_LEN;
        } ورنہ {
            آخری_dot_pos = string::npos;
        }
    }
    واپس dehashed.خالی_خانے() ? اندر : dehashed;
}

string VName::hashedName() {
    اگر (m_name == "") واپس "";
    اگر (m_hashed != "") واپس m_hashed;  // Memoized
    اگر (s_max_لمبائی == 0 || m_name.لمبائی_2() < s_max_لمبائی) {
        m_hashed = m_name;
        واپس m_hashed;
    }
    VHashSha512 hash{m_name};
    ثابت string suffix = "__Vhsh" + hash.digestSymbol24();
    اگر (s_min_لمبائی < s_max_لمبائی) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        سائز_t pref_لمبائی = s_min_لمبائی;
        جب (pref_لمبائی >= 1 && m_name[pref_لمبائی - 1] != '_') --pref_لمبائی;
        s_dehashMap[suffix] = m_name.substr(pref_لمبائی);
        m_hashed = m_name.substr(0, pref_لمبائی) + suffix;
    } ورنہ {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    واپس m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::Edit_فاصلہ VSpellCheck::edit_فاصلہ(ثابت string& s, ثابت string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ثابت سائز_t sLen = s.لمبائی_2();
    ثابت سائز_t tLen = t.لمبائی_2();
    اگر (sLen == 0) واپس tLen;
    اگر (tLen == 0) واپس sLen;
    اگر (sLen >= لمبائی_حد_بندی) واپس sLen;
    اگر (tLen >= لمبائی_حد_بندی) واپس tLen;

    ساکن std::array<Edit_فاصلہ, لمبائی_حد_بندی + 1> s_v_two_ago;
    ساکن std::array<Edit_فاصلہ, لمبائی_حد_بندی + 1> s_v_one_ago;
    ساکن std::array<Edit_فاصلہ, لمبائی_حد_بندی + 1> s_v_اگلا;

    برائے (سائز_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    برائے (سائز_t i = 0; i < tLen; i++) {
        s_v_اگلا[0] = i + 1;
        برائے (سائز_t j = 0; j < sLen; j++) {
            ثابت Edit_فاصلہ لاگت = (s[j] == t[i] ? 0 : 1);
            ثابت Edit_فاصلہ deletion = s_v_اگلا[j] + 1;
            ثابت Edit_فاصلہ insertion = s_v_one_ago[j + 1] + 1;
            ثابت Edit_فاصلہ substitution = s_v_one_ago[j] + لاگت;
            Edit_فاصلہ cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            اگر (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ثابت Edit_فاصلہ transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_اگلا[j + 1] = cheapest;
        }
        برائے (سائز_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_اگلا[j];
        }
    }

    ثابت Edit_فاصلہ نتیجہ = s_v_اگلا[sLen];
    واپس نتیجہ;
}

VSpellCheck::Edit_فاصلہ VSpellCheck::cutoff_فاصلہ(سائز_t goal_len, سائز_t candidate_len) {
    // Return max acceptable edit distance
    ثابت سائز_t max_لمبائی_2 = std::max(goal_len, candidate_len);
    ثابت سائز_t min_لمبائی = std::min(goal_len, candidate_len);
    اگر (max_لمبائی_2 <= 1) واپس 0;
    اگر (max_لمبائی_2 - min_لمبائی <= 1) واپس std::max(max_لمبائی_2 / 3, static_cast<سائز_t>(1));
    واپس (max_لمبائی_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::بہترین_Candidate_Info(ثابت string& goal,
                                                      Edit_فاصلہ& distancer) ثابت {
    Candidate بہترین;
    ثابت سائز_t gLen = goal.لمبائی_2();
    distancer = لمبائی_حد_بندی * 10;
    برائے (ثابت Candidate& candidate : m_candidates) {
        ثابت سائز_t cLen = candidate.str.لمبائی_2();

        // Min distance must be inserting/deleting to make lengths match
        ثابت Edit_فاصلہ min_فاصلہ = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        اگر (min_فاصلہ >= distancer) جاری;  // Short-circuit if already better

        ثابت Edit_فاصلہ cutoff = cutoff_فاصلہ(gLen, cLen);
        اگر (min_فاصلہ > cutoff) جاری;  // Short-circuit if already too bad

        ثابت Edit_فاصلہ dist = edit_فاصلہ(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.پیغام_2
                                      << "'");
        اگر (dist < distancer && dist <= cutoff) {
            distancer = dist;
            بہترین = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    اگر (distancer == 0) واپس Candidate{.str = بہترین.str, .پیغام_2 = ""};
    واپس بہترین;
}

خالی VSpellCheck::self_Test_فاصلہ_One(ثابت string& a, ثابت string& b, Edit_فاصلہ expected) {
    UASSERT_SELFTEST(edit_فاصلہ(a, b), expected);
    UASSERT_SELFTEST(edit_فاصلہ(b, a), expected);
}

خالی VSpellCheck::selfTestSuggestOne(منطقی matches, ثابت string& c, ثابت string& goal,
                                     Edit_فاصلہ dist) {
    Edit_فاصلہ gdist;
    VSpellCheck speller;
    speller.ڈالو_Candidate(c);
    ثابت string got = speller.بہترین_Candidate_Info(goal, gdist /*ref*/).str;
    اگر (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } ورنہ {
        UASSERT_SELFTEST(got, "");
    }
}

خالی VSpellCheck::selfTest() {
    {
        self_Test_فاصلہ_One("ab", "ac", 1);
        self_Test_فاصلہ_One("ab", "a", 1);
        self_Test_فاصلہ_One("a", "b", 1);
    }
    {
        selfTestSuggestOne(سچ, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(سچ, "abcdef", "acbdef", 1);
        selfTestSuggestOne(سچ, "db", "dc", 1);
        selfTestSuggestOne(سچ, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(جھوٹ, "x", "y", 1);
        selfTestSuggestOne(جھوٹ, "sqrt", "assert", 3);
    }
    {
        ثابت VSpellCheck speller;
        UASSERT_SELFTEST(speller.بہترین_Candidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ڈالو_Candidate("fred");
        speller.ڈالو_Candidate("wilma");
        speller.ڈالو_Candidate("barney");
        UASSERT_SELFTEST(speller.بہترین_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.بہترین_Candidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.بہترین_Candidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.بہترین_Candidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        برائے (غیرعلامتی i = 0; i < عدد_CANDIDATE_حد_بندی; ++i) speller.ڈالو_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.سائز(), عدد_CANDIDATE_حد_بندی);
        speller.ڈالو_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.سائز(), عدد_CANDIDATE_حد_بندی);
        UASSERT_SELFTEST(speller.بہترین_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.بہترین_Candidate("wilma").str, "");
    }
}
