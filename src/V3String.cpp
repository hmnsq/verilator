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
#endif

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#ضمن "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#ضمن <algorithm>
#ضمن <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <منطقي Even = true>
ساكن منطقي wildMatchImpl(ثابت حرف* s, ثابت حرف* p) VL_PURE {
    لكل (; *p; s++, p++) {
        إذا (*p != '*') {
            إذا (((*s) != (*p)) && *p != '?') أرجع خطأ;
        } وإلا {
            // Trailing star matches everything.
            إذا (!*++p) أرجع true;
            طالما (!wildMatchImpl<!Even>(s, p)) {
                إذا (*++s == '\0') أرجع خطأ;
            }
            أرجع true;
        }
    }
    أرجع (*s == '\0');
}

منطقي VString::wildmatch(ثابت حرف* s, ثابت حرف* p) VL_PURE {
    إذا (*s == '\0') {
        طالما (*p == '*') ++p;
        أرجع *p == '\0';
    }
    أرجع wildMatchImpl(s, p);
}

منطقي VString::wildmatch(ثابت string& s, ثابت string& p) VL_PURE {
    أرجع wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ثابت string& a, ثابت string& dot, ثابت string& b) {
    إذا (b == "") أرجع a;
    إذا (a == "") أرجع b;
    أرجع a + dot + b;
}

string VString::downcase(ثابت string& str) VL_PURE {
    string result = str;
    لكل (حرف& cr : result) cr = std::tolower(cr);
    أرجع result;
}

string VString::upcase(ثابت string& str) VL_PURE {
    string result = str;
    لكل (حرف& cr : result) cr = std::toupper(cr);
    أرجع result;
}

string VString::quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_PURE {
    string result;
    لكل (ثابت حرف c : str) {
        إذا (c == tgt) result += esc;
        result += c;
    }
    أرجع result;
}

string VString::dequotePercent(ثابت string& str) {
    string result;
    حرف last = '\0';
    لكل (ثابت حرف c : str) {
        إذا (last == '%' && c == '%') {
            last = '\0';
        } وإلا {
            result += c;
            last = c;
        }
    }
    أرجع result;
}

string VString::quoteStringLiteralForShell(ثابت string& str) {
    string result;
    constexpr حرف dquote = '"';
    constexpr حرف escape = '\\';
    result.پاڵنان_back(dquote);  // Start quoted string
    result.پاڵنان_back(escape);
    result.پاڵنان_back(dquote);  // "
    لكل (ثابت حرف c : str) {
        إذا (c == dquote || c == escape) result.پاڵنان_back(escape);
        result.پاڵنان_back(c);
    }
    result.پاڵنان_back(escape);
    result.پاڵنان_back(dquote);  // "
    result.پاڵنان_back(dquote);  // Terminate quoted string
    أرجع result;
}

string VString::escapeStringForPath(ثابت string& str) {
    إذا (str.find(R"(\\)") != string::npos)
        أرجع str;  // if it has been escaped already, don't do it again
    إذا (str.find('/') != string::npos) أرجع str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr حرف space = ' ';  // escape space like this `Program Files`
    constexpr حرف escape = '\\';
    لكل (ثابت حرف c : str) {
        إذا (c == space || c == escape) result.پاڵنان_back(escape);
        result.پاڵنان_back(c);
    }
    أرجع result;
}

ساكن صحيح vl_decodexdigit(حرف c) {
    أرجع std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ثابت string& text, string& errOut) {
    منطقي quoted = خطأ;
    string newtext;
    newtext.reserve(text.size());
    unsigned حرف octal_val = 0;
    صحيح octal_digits = 0;
    لكل (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        إذا (quoted) {
            إذا (std::isdigit(*cp)) {
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
                } وإلا إذا (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
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
            quoted = true;
            octal_digits = 0;
        } وإلا {
            newtext += *cp;
        }
    }
    أرجع newtext;
}

size_t VString::quotedEnd(ثابت string& str, size_t pos) VL_PURE {
    لكل (size_t i = pos + 1; i < str.size(); ++i) {
        إذا (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } وإلا إذا (str[i] == '"') {
            أرجع i + 1;
        }
    }
    أرجع string::npos;
}

string VString::spaceUnprintable(ثابت string& str) VL_PURE {
    string result;
    لكل (ثابت حرف c : str) {
        إذا (std::isprint(c)) {
            result += c;
        } وإلا {
            result += ' ';
        }
    }
    أرجع result;
}

string VString::removeWhitespace(ثابت string& str) {
    string result;
    result.reserve(str.size());
    لكل (ثابت حرف c : str) {
        إذا (!std::isspace(c)) result += c;
    }
    أرجع result;
}

string VString::trimWhitespace(ثابت string& str) {
    string result;
    result.reserve(str.size());
    string add;
    منطقي newline = خطأ;
    لكل (ثابت حرف c : str) {
        إذا (newline && std::isspace(c)) استمر;
        إذا (c == '\n') {
            add = "\n";
            newline = true;
            استمر;
        }
        إذا (std::isspace(c)) {
            add += c;
            استمر;
        }
        إذا (!add.empty()) {
            result += add;
            newline = خطأ;
            add.clear();
        }
        result += c;
    }
    أرجع result;
}

منطقي VString::isIdentifier(ثابت string& str) {
    لكل (ثابت حرف c : str) {
        إذا (!isIdentifierChar(c)) أرجع خطأ;
    }
    أرجع true;
}

منطقي VString::isWhitespace(ثابت string& str) {
    لكل (ثابت حرف c : str) {
        إذا (!std::isspace(c)) أرجع خطأ;
    }
    أرجع true;
}

string::size_type VString::leading_Whitespace_ژماردن(ثابت string& str) {
    string::size_type result = 0;
    لكل (ثابت حرف c : str) {
        ++result;
        إذا (!std::isspace(c)) اكسر;
    }
    أرجع result;
}

مزدوج VString::parseDouble(ثابت string& str, منطقي* successp) {
    حرف* ثابت strgp = جديد حرف[str.size() + 1];
    حرف* dp = strgp;
    إذا (successp) *successp = true;
    لكل (ثابت حرف* sp = str.c_str(); *sp; ++sp) {
        إذا (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    حرف* endp = strgp;
    ثابت مزدوج d = strtod(strgp, &endp);
    ثابت size_t parsed_len = endp - strgp;
    إذا (parsed_len != std::strlen(strgp)) {
        إذا (successp) *successp = خطأ;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    أرجع d;
}

string VString::replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& to) {
    string result = str;
    ثابت size_t fromLen = from.size();
    ثابت size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    لكل (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    أرجع result;
}

string VString::replaceWord(ثابت string& str, ثابت string& from, ثابت string& to) {
    string result = str;
    ثابت size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    لكل (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        إذا (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            استمر;
        }
        result.replace(pos, len, to);
    }
    أرجع result;
}

std::deque<string> VString::split(ثابت string& str, حرف delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    طالما (std::getline(is, token, delimiter)) results.پاڵنان_back(token);
    أرجع results;
}

منطقي VString::endsWith(ثابت string& str, ثابت string& suffix) {
    إذا (str.length() < suffix.length()) أرجع خطأ;
    أرجع str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(ثابت حرف* word) {
    بدل (word[0]) {
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
uint64_t VString::hashMurmur(ثابت string& str) VL_PURE {
    ثابت حرف* کلیل = str.c_str();
    ثابت size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr صحيح r = 47;

    uint64_t h = seed ^ (len * m);

    ثابت uint64_t* data = reinterpret_cast<ثابت uint64_t*>(کلیل);
    ثابت uint64_t* end = data + (len / 8);

    طالما (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ثابت unsigned حرف* data2 = reinterpret_cast<ثابت unsigned حرف*>(data);

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

string VString::base64Enc(ثابت string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    ساكن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    إذا (len >= 3) {
        لكل (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<صحيح>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    إذا (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<صحيح>(0) >> 6)];
        result += '=';
    } وإلا إذا (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<صحيح>(0) >> 4)];
        result += '=';
        result += '=';
    }
    أرجع result;
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
ساكن uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    أرجع lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ساكن فراغ sha512Block(uint64_t* h, ثابت uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ثابت uint64_t* p = chunk;

    // Initialize working variables to current hash value
    لكل (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    لكل (unsigned i = 0; i < 5; ++i) {
        لكل (unsigned j = 0; j < 16; ++j) {
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
    لكل (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

فراغ VHashSha512::زیادکردن(ثابت فراغ* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    صحيح chunkLen;
    ثابت uint8_t* chunkp;
    إذا (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<ثابت uint8_t*>(datap);
    } وإلا {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<ثابت حرف*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<ثابت uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    صحيح posBegin = 0;  // Position in buffer for start of this block
    صحيح posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    طالما (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        لكل (صحيح roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<ثابت حرف*>(chunkp + posBegin), chunkLen - posEnd);
}

فراغ VHashSha512::زیادکردن_File(ثابت string& filename) {
    ساكن constexpr size_t BUFFER_SIZE = 64 * 1024;

    ثابت صحيح fd = ::open(filename.c_str(), O_RDONLY);
    إذا (fd < 0) أرجع;

    std::array<حرف, BUFFER_SIZE + 1> buf;
    طالما (ثابت ssize_t got = ::اقرأ(fd, &buf, BUFFER_SIZE)) {
        إذا (got <= 0) اكسر;
        زیادکردن(&buf, got);
    }
    ::close(fd);
}

فراغ VHashSha512::finalize() {
    إذا (!m_final) {
        // Make sure no 128 byte blocks left
        زیادکردن("");
        m_final = true;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        لكل (صحيح i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        لكل (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        إذا (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            لكل (صحيح i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_دووانەیی() {
    finalize();
    string result;
    result.reserve(64);
    لكل (size_t i = 0; i < 64; ++i) {
        result += static_cast<حرف>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    أرجع result;
}

uint64_t VHashSha512::digestUInt64() {
    ثابت string& binhash = digest_دووانەیی();
    uint64_t result = 0;
    لكل (size_t byte = 0; byte < حجم(uint64_t); ++byte) {
        ثابت unsigned حرف c = binhash[byte];
        result = (result << 8) | c;
    }
    أرجع result;
}

string VHashSha512::digestHex() {
    ساكن constexpr ثابت حرف* ثابت digits = "0123456789abcdef";
    ثابت string& binhash = digest_دووانەیی();
    string result;
    result.reserve(128);
    لكل (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    أرجع result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    أرجع VString::base64Enc(digest_دووانەیی());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ساكن constexpr ثابت حرف* ثابت digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ثابت string& binhash = digest_دووانەیی();
    string result;
    result.reserve(84);
    صحيح pos = 0;
    لكل (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<صحيح>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<صحيح>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        إذا (result.size() >= DIGEST_SYMBOL24_LENGTH) اكسر;
    }
    // Any leftover bits don't matter for our purpose
    أرجع result;
}

فراغ VHashSha512::selfTestOne(ثابت string& data, ثابت string& data2, ثابت string& exp,
                              ثابت string& exp64, ثابت string& exp24) {
    VHashSha512 digest{data};
    إذا (data2 != "") digest.زیادکردن(data2);
    إذا (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    إذا (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    إذا (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
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

string VName::dehash(ثابت string& in) {
    ساكن constexpr ثابت حرف VHSH[] = "__Vhsh";
    ساكن constexpr size_t VHSH_LEN = حجم(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    ساكن ثابت size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    لكل (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        ثابت string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ثابت auto گەڕان_begin = std::begin(in) + last_dot_pos;
        ثابت auto گەڕان_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        ثابت auto begin_vhsh
            = std::گەڕان(گەڕان_begin, گەڕان_end, std::begin(VHSH), std::end(VHSH) - 1);
        إذا (begin_vhsh != گەڕان_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ثابت auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, گەڕان_end), VHSH_LEN);
            ثابت std::string vhsh{begin_vhsh, end_vhsh};
            ثابت auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            إذا (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{گەڕان_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, گەڕان_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        وإلا إذا (!dehashed.empty()) {
            dehashed += std::string{گەڕان_begin, گەڕان_end};
        }

        إذا (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            إذا (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } وإلا {
            last_dot_pos = string::npos;
        }
    }
    أرجع dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    إذا (m_name == "") أرجع "";
    إذا (m_hashed != "") أرجع m_hashed;  // Memoized
    إذا (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        أرجع m_hashed;
    }
    VHashSha512 hash{m_name};
    ثابت string suffix = "__Vhsh" + hash.digestSymbol24();
    إذا (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        طالما (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } وإلا {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    أرجع m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ثابت string& s, ثابت string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ثابت size_t sLen = s.length();
    ثابت size_t tLen = t.length();
    إذا (sLen == 0) أرجع tLen;
    إذا (tLen == 0) أرجع sLen;
    إذا (sLen >= LENGTH_LIMIT) أرجع sLen;
    إذا (tLen >= LENGTH_LIMIT) أرجع tLen;

    ساكن std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    ساكن std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    ساكن std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    لكل (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    لكل (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        لكل (size_t j = 0; j < sLen; j++) {
            ثابت EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ثابت EditDistance deletion = s_v_next[j] + 1;
            ثابت EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ثابت EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            إذا (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ثابت EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        لكل (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    ثابت EditDistance result = s_v_next[sLen];
    أرجع result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    ثابت size_t max_length = std::max(goal_len, candidate_len);
    ثابت size_t min_length = std::min(goal_len, candidate_len);
    إذا (max_length <= 1) أرجع 0;
    إذا (max_length - min_length <= 1) أرجع std::max(max_length / 3, static_cast<size_t>(1));
    أرجع (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ثابت string& goal,
                                                      EditDistance& distancer) ثابت {
    Candidate best;
    ثابت size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    لكل (ثابت Candidate& candidate : m_candidates) {
        ثابت size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        ثابت EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        إذا (min_distance >= distancer) استمر;  // Short-circuit if already better

        ثابت EditDistance cutoff = cutoffDistance(gLen, cLen);
        إذا (min_distance > cutoff) استمر;  // Short-circuit if already too bad

        ثابت EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.پەیام
                                      << "'");
        إذا (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    إذا (distancer == 0) أرجع Candidate{.str = best.str, .پەیام = ""};
    أرجع best;
}

فراغ VSpellCheck::selfTestDistanceOne(ثابت string& a, ثابت string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

فراغ VSpellCheck::selfTestSuggestOne(منطقي matches, ثابت string& c, ثابت string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.پاڵنان_Candidate(c);
    ثابت string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    إذا (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } وإلا {
        UASSERT_SELFTEST(got, "");
    }
}

فراغ VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(true, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(true, "abcdef", "acbdef", 1);
        selfTestSuggestOne(true, "db", "dc", 1);
        selfTestSuggestOne(true, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(خطأ, "x", "y", 1);
        selfTestSuggestOne(خطأ, "sqrt", "assert", 3);
    }
    {
        ثابت VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.پاڵنان_Candidate("fred");
        speller.پاڵنان_Candidate("wilma");
        speller.پاڵنان_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        لكل (unsigned i = 0; i < ژمارە_CANDIDATE_LIMIT; ++i) speller.پاڵنان_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), ژمارە_CANDIDATE_LIMIT);
        speller.پاڵنان_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), ژمارە_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
