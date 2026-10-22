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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#যদি defined(_WIN32) || defined(__MINGW32__)
#অন্তর্ভাব <io.h>  // open, read, write, close
#endif

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#অন্তর্ভাব "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <তার্কিক Even = সত্য>
স্থির তার্কিক wildMatchImpl(ধ্রুব বর্ণ* s, ধ্রুব বর্ণ* p) VL_PURE {
    প্রতি (; *p; s++, p++) {
        যদি (*p != '*') {
            যদি (((*s) != (*p)) && *p != '?') প্রত্যাগম অসত্য;
        } অন্যথা {
            // Trailing star matches everything.
            যদি (!*++p) প্রত্যাগম সত্য;
            যাবত্ (!wildMatchImpl<!Even>(s, p)) {
                যদি (*++s == '\0') প্রত্যাগম অসত্য;
            }
            প্রত্যাগম সত্য;
        }
    }
    প্রত্যাগম (*s == '\0');
}

তার্কিক VString::wildmatch(ধ্রুব বর্ণ* s, ধ্রুব বর্ণ* p) VL_PURE {
    যদি (*s == '\0') {
        যাবত্ (*p == '*') ++p;
        প্রত্যাগম *p == '\0';
    }
    প্রত্যাগম wildMatchImpl(s, p);
}

তার্কিক VString::wildmatch(ধ্রুব string& s, ধ্রুব string& p) VL_PURE {
    প্রত্যাগম wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ধ্রুব string& a, ধ্রুব string& dot, ধ্রুব string& b) {
    যদি (b == "") প্রত্যাগম a;
    যদি (a == "") প্রত্যাগম b;
    প্রত্যাগম a + dot + b;
}

string VString::downcase(ধ্রুব string& str) VL_PURE {
    string result = str;
    প্রতি (বর্ণ& cr : result) cr = std::tolower(cr);
    প্রত্যাগম result;
}

string VString::upcase(ধ্রুব string& str) VL_PURE {
    string result = str;
    প্রতি (বর্ণ& cr : result) cr = std::toupper(cr);
    প্রত্যাগম result;
}

string VString::quoteAny(ধ্রুব string& str, বর্ণ tgt, বর্ণ esc) VL_PURE {
    string result;
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (c == tgt) result += esc;
        result += c;
    }
    প্রত্যাগম result;
}

string VString::dequotePercent(ধ্রুব string& str) {
    string result;
    বর্ণ last = '\0';
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (last == '%' && c == '%') {
            last = '\0';
        } অন্যথা {
            result += c;
            last = c;
        }
    }
    প্রত্যাগম result;
}

string VString::quoteStringLiteralForShell(ধ্রুব string& str) {
    string result;
    constexpr বর্ণ dquote = '"';
    constexpr বর্ণ escape = '\\';
    result.ঠেলো_back(dquote);  // Start quoted string
    result.ঠেলো_back(escape);
    result.ঠেলো_back(dquote);  // "
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (c == dquote || c == escape) result.ঠেলো_back(escape);
        result.ঠেলো_back(c);
    }
    result.ঠেলো_back(escape);
    result.ঠেলো_back(dquote);  // "
    result.ঠেলো_back(dquote);  // Terminate quoted string
    প্রত্যাগম result;
}

string VString::escapeStringForPath(ধ্রুব string& str) {
    যদি (str.find(R"(\\)") != string::npos)
        প্রত্যাগম str;  // if it has been escaped already, don't do it again
    যদি (str.find('/') != string::npos) প্রত্যাগম str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr বর্ণ space = ' ';  // escape space like this `Program Files`
    constexpr বর্ণ escape = '\\';
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (c == space || c == escape) result.ঠেলো_back(escape);
        result.ঠেলো_back(c);
    }
    প্রত্যাগম result;
}

স্থির পূর্ণাঙ্ক vl_decodexdigit(বর্ণ c) {
    প্রত্যাগম std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ধ্রুব string& text, string& errOut) {
    তার্কিক quoted = অসত্য;
    string newtext;
    newtext.reserve(text.size());
    unsigned বর্ণ octal_val = 0;
    পূর্ণাঙ্ক octal_digits = 0;
    প্রতি (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        যদি (quoted) {
            যদি (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                যদি (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = অসত্য;
                }
            } অন্যথা {
                যদি (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = অসত্য;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    অনুবর্ত;
                }
                quoted = অসত্য;
                যদি (*cp == 'n') {
                    newtext += '\n';
                } অন্যথা যদি (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } অন্যথা যদি (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } অন্যথা যদি (*cp == 'r') {
                    newtext += '\r';
                } অন্যথা যদি (*cp == 't') {
                    newtext += '\t';
                } অন্যথা যদি (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } অন্যথা যদি (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<বর্ণ>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } অন্যথা যদি (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    ভঙ্গ;
                } অন্যথা {
                    newtext += *cp;
                }
            }
        } অন্যথা যদি (*cp == '\\') {
            যদি (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = সত্য;
            octal_digits = 0;
        } অন্যথা {
            newtext += *cp;
        }
    }
    প্রত্যাগম newtext;
}

size_t VString::quotedEnd(ধ্রুব string& str, size_t pos) VL_PURE {
    প্রতি (size_t i = pos + 1; i < str.size(); ++i) {
        যদি (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } অন্যথা যদি (str[i] == '"') {
            প্রত্যাগম i + 1;
        }
    }
    প্রত্যাগম string::npos;
}

string VString::spaceUnprintable(ধ্রুব string& str) VL_PURE {
    string result;
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (std::isprint(c)) {
            result += c;
        } অন্যথা {
            result += ' ';
        }
    }
    প্রত্যাগম result;
}

string VString::removeWhitespace(ধ্রুব string& str) {
    string result;
    result.reserve(str.size());
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (!std::isspace(c)) result += c;
    }
    প্রত্যাগম result;
}

string VString::trimWhitespace(ধ্রুব string& str) {
    string result;
    result.reserve(str.size());
    string add;
    তার্কিক newline = অসত্য;
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (newline && std::isspace(c)) অনুবর্ত;
        যদি (c == '\n') {
            add = "\n";
            newline = সত্য;
            অনুবর্ত;
        }
        যদি (std::isspace(c)) {
            add += c;
            অনুবর্ত;
        }
        যদি (!add.empty()) {
            result += add;
            newline = অসত্য;
            add.clear();
        }
        result += c;
    }
    প্রত্যাগম result;
}

তার্কিক VString::isIdentifier(ধ্রুব string& str) {
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (!isIdentifierChar(c)) প্রত্যাগম অসত্য;
    }
    প্রত্যাগম সত্য;
}

তার্কিক VString::isWhitespace(ধ্রুব string& str) {
    প্রতি (ধ্রুব বর্ণ c : str) {
        যদি (!std::isspace(c)) প্রত্যাগম অসত্য;
    }
    প্রত্যাগম সত্য;
}

string::size_type VString::leading_Whitespace_গুনতি(ধ্রুব string& str) {
    string::size_type result = 0;
    প্রতি (ধ্রুব বর্ণ c : str) {
        ++result;
        যদি (!std::isspace(c)) ভঙ্গ;
    }
    প্রত্যাগম result;
}

দ্বিগুণ VString::parseDouble(ধ্রুব string& str, তার্কিক* successp) {
    বর্ণ* ধ্রুব strgp = নূতন বর্ণ[str.size() + 1];
    বর্ণ* dp = strgp;
    যদি (successp) *successp = সত্য;
    প্রতি (ধ্রুব বর্ণ* sp = str.c_str(); *sp; ++sp) {
        যদি (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    বর্ণ* endp = strgp;
    ধ্রুব দ্বিগুণ d = strtod(strgp, &endp);
    ধ্রুব size_t parsed_len = endp - strgp;
    যদি (parsed_len != std::strlen(strgp)) {
        যদি (successp) *successp = অসত্য;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    প্রত্যাগম d;
}

string VString::replaceSubstr(ধ্রুব string& str, ধ্রুব string& from, ধ্রুব string& to) {
    string result = str;
    ধ্রুব size_t fromLen = from.size();
    ধ্রুব size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    প্রতি (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    প্রত্যাগম result;
}

string VString::replaceWord(ধ্রুব string& str, ধ্রুব string& from, ধ্রুব string& to) {
    string result = str;
    ধ্রুব size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    প্রতি (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        যদি (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            অনুবর্ত;
        }
        result.replace(pos, len, to);
    }
    প্রত্যাগম result;
}

std::deque<string> VString::split(ধ্রুব string& str, বর্ণ delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    যাবত্ (std::getline(is, token, delimiter)) results.ঠেলো_back(token);
    প্রত্যাগম results;
}

তার্কিক VString::endsWith(ধ্রুব string& str, ধ্রুব string& suffix) {
    যদি (str.length() < suffix.length()) প্রত্যাগম অসত্য;
    প্রত্যাগম str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(ধ্রুব বর্ণ* word) {
    বিকল্প (word[0]) {
    পক্ষ '\0': প্রত্যাগম "";
    পক্ষ 'a':
    পক্ষ 'e':
    পক্ষ 'i':
    পক্ষ 'o':
    পক্ষ 'u': প্রত্যাগম "an";
    স্বভাব: প্রত্যাগম "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ধ্রুব string& str) VL_PURE {
    ধ্রুব বর্ণ* চাবি = str.c_str();
    ধ্রুব size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr পূর্ণাঙ্ক r = 47;

    uint64_t h = seed ^ (len * m);

    ধ্রুব uint64_t* data = reinterpret_cast<ধ্রুব uint64_t*>(চাবি);
    ধ্রুব uint64_t* end = data + (len / 8);

    যাবত্ (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ধ্রুব unsigned বর্ণ* data2 = reinterpret_cast<ধ্রুব unsigned বর্ণ*>(data);

    বিকল্প (len & 7) {
    পক্ষ 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    পক্ষ 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    পক্ষ 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    পক্ষ 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    পক্ষ 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    পক্ষ 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    পক্ষ 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    স্বভাব:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    প্রত্যাগম h;
}

string VString::base64Enc(ধ্রুব string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    স্থির constexpr ধ্রুব বর্ণ* ধ্রুব digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    যদি (len >= 3) {
        প্রতি (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<পূর্ণাঙ্ক>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<পূর্ণাঙ্ক>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    যদি (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<পূর্ণাঙ্ক>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<পূর্ণাঙ্ক>(0) >> 6)];
        result += '=';
    } অন্যথা যদি (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<পূর্ণাঙ্ক>(0) >> 4)];
        result += '=';
        result += '=';
    }
    প্রত্যাগম result;
}

শূন্য VString::selfTest() {
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

স্থির constexpr uint64_t sha512K[]
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
স্থির uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    প্রত্যাগম lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
স্থির শূন্য sha512Block(uint64_t* h, ধ্রুব uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ধ্রুব uint64_t* p = chunk;

    // Initialize working variables to current hash value
    প্রতি (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    প্রতি (unsigned i = 0; i < 5; ++i) {
        প্রতি (unsigned j = 0; j < 16; ++j) {
            যদি (i == 0) {
                w[j] = *p++;
            } অন্যথা {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ধ্রুব uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ধ্রুব uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ধ্রুব uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ধ্রুব uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ধ্রুব uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ধ্রুব uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ধ্রুব uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ধ্রুব uint64_t temp2 = s0 + maj;

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
    প্রতি (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

শূন্য VHashSha512::ঢোকাও(ধ্রুব শূন্য* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    পূর্ণাঙ্ক chunkLen;
    ধ্রুব uint8_t* chunkp;
    যদি (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<ধ্রুব uint8_t*>(datap);
    } অন্যথা {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<ধ্রুব বর্ণ*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<ধ্রুব uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    পূর্ণাঙ্ক posBegin = 0;  // Position in buffer for start of this block
    পূর্ণাঙ্ক posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    যাবত্ (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        প্রতি (পূর্ণাঙ্ক roundByte = 0; posBegin < posEnd; posBegin += 8) {
            w[roundByte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
                              | (static_cast<uint64_t>(chunkp[posBegin + 6]) << 8)
                              | (static_cast<uint64_t>(chunkp[posBegin + 5]) << 16)
                              | (static_cast<uint64_t>(chunkp[posBegin + 4]) << 24)
                              | (static_cast<uint64_t>(chunkp[posBegin + 3]) << 32)
                              | (static_cast<uint64_t>(chunkp[posBegin + 2]) << 40)
                              | (static_cast<uint64_t>(chunkp[posBegin + 1]) << 48)
                              | (static_cast<uint64_t>(chunkp[posBegin]) << 56));
        }
        sha512Block(m_inthash, w);
    }

    m_remainder = std::string(reinterpret_cast<ধ্রুব বর্ণ*>(chunkp + posBegin), chunkLen - posEnd);
}

শূন্য VHashSha512::ঢোকাও_File(ধ্রুব string& filename) {
    স্থির constexpr size_t BUFFER_SIZE = 64 * 1024;

    ধ্রুব পূর্ণাঙ্ক fd = ::open(filename.c_str(), O_RDONLY);
    যদি (fd < 0) প্রত্যাগম;

    std::array<বর্ণ, BUFFER_SIZE + 1> buf;
    যাবত্ (ধ্রুব ssize_t got = ::পঠ(fd, &buf, BUFFER_SIZE)) {
        যদি (got <= 0) ভঙ্গ;
        ঢোকাও(&buf, got);
    }
    ::close(fd);
}

শূন্য VHashSha512::finalize() {
    যদি (!m_final) {
        // Make sure no 128 byte blocks left
        ঢোকাও("");
        m_final = সত্য;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        প্রতি (পূর্ণাঙ্ক i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        প্রতি (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        যদি (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            প্রতি (পূর্ণাঙ্ক i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_দ্বিমিক() {
    finalize();
    string result;
    result.reserve(64);
    প্রতি (size_t i = 0; i < 64; ++i) {
        result += static_cast<বর্ণ>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    প্রত্যাগম result;
}

uint64_t VHashSha512::digestUInt64() {
    ধ্রুব string& binhash = digest_দ্বিমিক();
    uint64_t result = 0;
    প্রতি (size_t byte = 0; byte < পরিমাণ(uint64_t); ++byte) {
        ধ্রুব unsigned বর্ণ c = binhash[byte];
        result = (result << 8) | c;
    }
    প্রত্যাগম result;
}

string VHashSha512::digestHex() {
    স্থির constexpr ধ্রুব বর্ণ* ধ্রুব digits = "0123456789abcdef";
    ধ্রুব string& binhash = digest_দ্বিমিক();
    string result;
    result.reserve(128);
    প্রতি (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    প্রত্যাগম result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    প্রত্যাগম VString::base64Enc(digest_দ্বিমিক());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    স্থির constexpr ধ্রুব বর্ণ* ধ্রুব digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ধ্রুব string& binhash = digest_দ্বিমিক();
    string result;
    result.reserve(84);
    পূর্ণাঙ্ক pos = 0;
    প্রতি (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<পূর্ণাঙ্ক>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<পূর্ণাঙ্ক>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        যদি (result.size() >= DIGEST_SYMBOL24_LENGTH) ভঙ্গ;
    }
    // Any leftover bits don't matter for our purpose
    প্রত্যাগম result;
}

শূন্য VHashSha512::selfTestOne(ধ্রুব string& data, ধ্রুব string& data2, ধ্রুব string& exp,
                              ধ্রুব string& exp64, ধ্রুব string& exp24) {
    VHashSha512 digest{data};
    যদি (data2 != "") digest.ঢোকাও(data2);
    যদি (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    যদি (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    যদি (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
}

শূন্য VHashSha512::selfTest() {
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

string VName::dehash(ধ্রুব string& in) {
    স্থির constexpr ধ্রুব বর্ণ VHSH[] = "__Vhsh";
    স্থির constexpr size_t VHSH_LEN = পরিমাণ(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    স্থির ধ্রুব size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    প্রতি (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        ধ্রুব string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ধ্রুব auto অনুসন্ধান_begin = std::begin(in) + last_dot_pos;
        ধ্রুব auto অনুসন্ধান_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        ধ্রুব auto begin_vhsh
            = std::অনুসন্ধান(অনুসন্ধান_begin, অনুসন্ধান_end, std::begin(VHSH), std::end(VHSH) - 1);
        যদি (begin_vhsh != অনুসন্ধান_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ধ্রুব auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, অনুসন্ধান_end), VHSH_LEN);
            ধ্রুব std::string vhsh{begin_vhsh, end_vhsh};
            ধ্রুব auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            যদি (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{অনুসন্ধান_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, অনুসন্ধান_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        অন্যথা যদি (!dehashed.empty()) {
            dehashed += std::string{অনুসন্ধান_begin, অনুসন্ধান_end};
        }

        যদি (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            যদি (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } অন্যথা {
            last_dot_pos = string::npos;
        }
    }
    প্রত্যাগম dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    যদি (m_name == "") প্রত্যাগম "";
    যদি (m_hashed != "") প্রত্যাগম m_hashed;  // Memoized
    যদি (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        প্রত্যাগম m_hashed;
    }
    VHashSha512 hash{m_name};
    ধ্রুব string suffix = "__Vhsh" + hash.digestSymbol24();
    যদি (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        যাবত্ (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } অন্যথা {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    প্রত্যাগম m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ধ্রুব string& s, ধ্রুব string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ধ্রুব size_t sLen = s.length();
    ধ্রুব size_t tLen = t.length();
    যদি (sLen == 0) প্রত্যাগম tLen;
    যদি (tLen == 0) প্রত্যাগম sLen;
    যদি (sLen >= LENGTH_LIMIT) প্রত্যাগম sLen;
    যদি (tLen >= LENGTH_LIMIT) প্রত্যাগম tLen;

    স্থির std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    স্থির std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    স্থির std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    প্রতি (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    প্রতি (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        প্রতি (size_t j = 0; j < sLen; j++) {
            ধ্রুব EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ধ্রুব EditDistance deletion = s_v_next[j] + 1;
            ধ্রুব EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ধ্রুব EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            যদি (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ধ্রুব EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        প্রতি (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    ধ্রুব EditDistance result = s_v_next[sLen];
    প্রত্যাগম result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    ধ্রুব size_t max_length = std::max(goal_len, candidate_len);
    ধ্রুব size_t min_length = std::min(goal_len, candidate_len);
    যদি (max_length <= 1) প্রত্যাগম 0;
    যদি (max_length - min_length <= 1) প্রত্যাগম std::max(max_length / 3, static_cast<size_t>(1));
    প্রত্যাগম (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ধ্রুব string& goal,
                                                      EditDistance& distancer) ধ্রুব {
    Candidate best;
    ধ্রুব size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    প্রতি (ধ্রুব Candidate& candidate : m_candidates) {
        ধ্রুব size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        ধ্রুব EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        যদি (min_distance >= distancer) অনুবর্ত;  // Short-circuit if already better

        ধ্রুব EditDistance cutoff = cutoffDistance(gLen, cLen);
        যদি (min_distance > cutoff) অনুবর্ত;  // Short-circuit if already too bad

        ধ্রুব EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.বার্তা
                                      << "'");
        যদি (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    যদি (distancer == 0) প্রত্যাগম Candidate{.str = best.str, .বার্তা = ""};
    প্রত্যাগম best;
}

শূন্য VSpellCheck::selfTestDistanceOne(ধ্রুব string& a, ধ্রুব string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

শূন্য VSpellCheck::selfTestSuggestOne(তার্কিক matches, ধ্রুব string& c, ধ্রুব string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.ঠেলো_Candidate(c);
    ধ্রুব string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    যদি (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } অন্যথা {
        UASSERT_SELFTEST(got, "");
    }
}

শূন্য VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(সত্য, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(সত্য, "abcdef", "acbdef", 1);
        selfTestSuggestOne(সত্য, "db", "dc", 1);
        selfTestSuggestOne(সত্য, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(অসত্য, "x", "y", 1);
        selfTestSuggestOne(অসত্য, "sqrt", "assert", 3);
    }
    {
        ধ্রুব VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ঠেলো_Candidate("fred");
        speller.ঠেলো_Candidate("wilma");
        speller.ঠেলো_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        প্রতি (unsigned i = 0; i < অঙ্ক_CANDIDATE_LIMIT; ++i) speller.ঠেলো_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), অঙ্ক_CANDIDATE_LIMIT);
        speller.ঠেলো_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), অঙ্ক_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
