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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#सचे defined(_WIN32) || defined(__MINGW32__)
#अन्तर्भाव <io.h>  // open, read, write, close
#endif

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#अन्तर्भाव "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#अन्तर्भाव <algorithm>
#अन्तर्भाव <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <तार्किक Even = सच्च>
स्थिर तार्किक wildMatchImpl(ध्रुव वर्ण* s, ध्रुव वर्ण* p) VL_PURE {
    अत्थाय (; *p; s++, p++) {
        सचे (*p != '*') {
            सचे (((*s) != (*p)) && *p != '?') पटिनिवत्त मिच्छा;
        } अञ्ञथा {
            // Trailing star matches everything.
            सचे (!*++p) पटिनिवत्त सच्च;
            याव (!wildMatchImpl<!Even>(s, p)) {
                सचे (*++s == '\0') पटिनिवत्त मिच्छा;
            }
            पटिनिवत्त सच्च;
        }
    }
    पटिनिवत्त (*s == '\0');
}

तार्किक VString::wildmatch(ध्रुव वर्ण* s, ध्रुव वर्ण* p) VL_PURE {
    सचे (*s == '\0') {
        याव (*p == '*') ++p;
        पटिनिवत्त *p == '\0';
    }
    पटिनिवत्त wildMatchImpl(s, p);
}

तार्किक VString::wildmatch(ध्रुव string& s, ध्रुव string& p) VL_PURE {
    पटिनिवत्त wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ध्रुव string& a, ध्रुव string& dot, ध्रुव string& b) {
    सचे (b == "") पटिनिवत्त a;
    सचे (a == "") पटिनिवत्त b;
    पटिनिवत्त a + dot + b;
}

string VString::downcase(ध्रुव string& str) VL_PURE {
    string result = str;
    अत्थाय (वर्ण& cr : result) cr = std::tolower(cr);
    पटिनिवत्त result;
}

string VString::upcase(ध्रुव string& str) VL_PURE {
    string result = str;
    अत्थाय (वर्ण& cr : result) cr = std::toupper(cr);
    पटिनिवत्त result;
}

string VString::quoteAny(ध्रुव string& str, वर्ण tgt, वर्ण esc) VL_PURE {
    string result;
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (c == tgt) result += esc;
        result += c;
    }
    पटिनिवत्त result;
}

string VString::dequotePercent(ध्रुव string& str) {
    string result;
    वर्ण last = '\0';
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (last == '%' && c == '%') {
            last = '\0';
        } अञ्ञथा {
            result += c;
            last = c;
        }
    }
    पटिनिवत्त result;
}

string VString::quoteStringLiteralForShell(ध्रुव string& str) {
    string result;
    constexpr वर्ण dquote = '"';
    constexpr वर्ण escape = '\\';
    result.प्रक्षिप_back(dquote);  // Start quoted string
    result.प्रक्षिप_back(escape);
    result.प्रक्षिप_back(dquote);  // "
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (c == dquote || c == escape) result.प्रक्षिप_back(escape);
        result.प्रक्षिप_back(c);
    }
    result.प्रक्षिप_back(escape);
    result.प्रक्षिप_back(dquote);  // "
    result.प्रक्षिप_back(dquote);  // Terminate quoted string
    पटिनिवत्त result;
}

string VString::escapeStringForPath(ध्रुव string& str) {
    सचे (str.find(R"(\\)") != string::npos)
        पटिनिवत्त str;  // if it has been escaped already, don't do it again
    सचे (str.find('/') != string::npos) पटिनिवत्त str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr वर्ण space = ' ';  // escape space like this `Program Files`
    constexpr वर्ण escape = '\\';
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (c == space || c == escape) result.प्रक्षिप_back(escape);
        result.प्रक्षिप_back(c);
    }
    पटिनिवत्त result;
}

स्थिर पूर्णाङ्क vl_decodexdigit(वर्ण c) {
    पटिनिवत्त std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ध्रुव string& text, string& errOut) {
    तार्किक quoted = मिच्छा;
    string newtext;
    newtext.reserve(text.size());
    unsigned वर्ण octal_val = 0;
    पूर्णाङ्क octal_digits = 0;
    अत्थाय (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        सचे (quoted) {
            सचे (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                सचे (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = मिच्छा;
                }
            } अञ्ञथा {
                सचे (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = मिच्छा;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    अनुवर्त;
                }
                quoted = मिच्छा;
                सचे (*cp == 'n') {
                    newtext += '\n';
                } अञ्ञथा सचे (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } अञ्ञथा सचे (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } अञ्ञथा सचे (*cp == 'r') {
                    newtext += '\r';
                } अञ्ञथा सचे (*cp == 't') {
                    newtext += '\t';
                } अञ्ञथा सचे (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } अञ्ञथा सचे (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<वर्ण>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } अञ्ञथा सचे (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    भङ्ग;
                } अञ्ञथा {
                    newtext += *cp;
                }
            }
        } अञ्ञथा सचे (*cp == '\\') {
            सचे (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = सच्च;
            octal_digits = 0;
        } अञ्ञथा {
            newtext += *cp;
        }
    }
    पटिनिवत्त newtext;
}

size_t VString::quotedEnd(ध्रुव string& str, size_t pos) VL_PURE {
    अत्थाय (size_t i = pos + 1; i < str.size(); ++i) {
        सचे (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } अञ्ञथा सचे (str[i] == '"') {
            पटिनिवत्त i + 1;
        }
    }
    पटिनिवत्त string::npos;
}

string VString::spaceUnprintable(ध्रुव string& str) VL_PURE {
    string result;
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (std::isprint(c)) {
            result += c;
        } अञ्ञथा {
            result += ' ';
        }
    }
    पटिनिवत्त result;
}

string VString::removeWhitespace(ध्रुव string& str) {
    string result;
    result.reserve(str.size());
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (!std::isspace(c)) result += c;
    }
    पटिनिवत्त result;
}

string VString::trimWhitespace(ध्रुव string& str) {
    string result;
    result.reserve(str.size());
    string add;
    तार्किक newline = मिच्छा;
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (newline && std::isspace(c)) अनुवर्त;
        सचे (c == '\n') {
            add = "\n";
            newline = सच्च;
            अनुवर्त;
        }
        सचे (std::isspace(c)) {
            add += c;
            अनुवर्त;
        }
        सचे (!add.empty()) {
            result += add;
            newline = मिच्छा;
            add.clear();
        }
        result += c;
    }
    पटिनिवत्त result;
}

तार्किक VString::isIdentifier(ध्रुव string& str) {
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (!isIdentifierChar(c)) पटिनिवत्त मिच्छा;
    }
    पटिनिवत्त सच्च;
}

तार्किक VString::isWhitespace(ध्रुव string& str) {
    अत्थाय (ध्रुव वर्ण c : str) {
        सचे (!std::isspace(c)) पटिनिवत्त मिच्छा;
    }
    पटिनिवत्त सच्च;
}

string::size_type VString::leading_Whitespace_गणनम्(ध्रुव string& str) {
    string::size_type result = 0;
    अत्थाय (ध्रुव वर्ण c : str) {
        ++result;
        सचे (!std::isspace(c)) भङ्ग;
    }
    पटिनिवत्त result;
}

द्विगुण VString::parseDouble(ध्रुव string& str, तार्किक* successp) {
    वर्ण* ध्रुव strgp = नूतन वर्ण[str.size() + 1];
    वर्ण* dp = strgp;
    सचे (successp) *successp = सच्च;
    अत्थाय (ध्रुव वर्ण* sp = str.c_str(); *sp; ++sp) {
        सचे (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    वर्ण* endp = strgp;
    ध्रुव द्विगुण d = strtod(strgp, &endp);
    ध्रुव size_t parsed_len = endp - strgp;
    सचे (parsed_len != std::strlen(strgp)) {
        सचे (successp) *successp = मिच्छा;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    पटिनिवत्त d;
}

string VString::replaceSubstr(ध्रुव string& str, ध्रुव string& from, ध्रुव string& to) {
    string result = str;
    ध्रुव size_t fromLen = from.size();
    ध्रुव size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    अत्थाय (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    पटिनिवत्त result;
}

string VString::replaceWord(ध्रुव string& str, ध्रुव string& from, ध्रुव string& to) {
    string result = str;
    ध्रुव size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    अत्थाय (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        सचे (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            अनुवर्त;
        }
        result.replace(pos, len, to);
    }
    पटिनिवत्त result;
}

std::deque<string> VString::split(ध्रुव string& str, वर्ण delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    याव (std::getline(is, token, delimiter)) results.प्रक्षिप_back(token);
    पटिनिवत्त results;
}

तार्किक VString::endsWith(ध्रुव string& str, ध्रुव string& suffix) {
    सचे (str.length() < suffix.length()) पटिनिवत्त मिच्छा;
    पटिनिवत्त str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(ध्रुव वर्ण* word) {
    विकल्प (word[0]) {
    पक्ष '\0': पटिनिवत्त "";
    पक्ष 'a':
    पक्ष 'e':
    पक्ष 'i':
    पक्ष 'o':
    पक्ष 'u': पटिनिवत्त "an";
    स्वभाव: पटिनिवत्त "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ध्रुव string& str) VL_PURE {
    ध्रुव वर्ण* कुञ्चिका = str.c_str();
    ध्रुव size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr पूर्णाङ्क r = 47;

    uint64_t h = seed ^ (len * m);

    ध्रुव uint64_t* data = reinterpret_cast<ध्रुव uint64_t*>(कुञ्चिका);
    ध्रुव uint64_t* end = data + (len / 8);

    याव (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ध्रुव unsigned वर्ण* data2 = reinterpret_cast<ध्रुव unsigned वर्ण*>(data);

    विकल्प (len & 7) {
    पक्ष 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    पक्ष 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    पक्ष 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    पक्ष 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    पक्ष 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    पक्ष 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    पक्ष 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    स्वभाव:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    पटिनिवत्त h;
}

string VString::base64Enc(ध्रुव string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    स्थिर constexpr ध्रुव वर्ण* ध्रुव digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    सचे (len >= 3) {
        अत्थाय (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णाङ्क>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<पूर्णाङ्क>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    सचे (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णाङ्क>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<पूर्णाङ्क>(0) >> 6)];
        result += '=';
    } अञ्ञथा सचे (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णाङ्क>(0) >> 4)];
        result += '=';
        result += '=';
    }
    पटिनिवत्त result;
}

शून्य VString::selfTest() {
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

स्थिर constexpr uint64_t sha512K[]
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
स्थिर uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    पटिनिवत्त lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
स्थिर शून्य sha512Block(uint64_t* h, ध्रुव uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ध्रुव uint64_t* p = chunk;

    // Initialize working variables to current hash value
    अत्थाय (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    अत्थाय (unsigned i = 0; i < 5; ++i) {
        अत्थाय (unsigned j = 0; j < 16; ++j) {
            सचे (i == 0) {
                w[j] = *p++;
            } अञ्ञथा {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ध्रुव uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ध्रुव uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ध्रुव uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ध्रुव uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ध्रुव uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ध्रुव uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ध्रुव uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ध्रुव uint64_t temp2 = s0 + maj;

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
    अत्थाय (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

शून्य VHashSha512::निवेशय(ध्रुव शून्य* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    पूर्णाङ्क chunkLen;
    ध्रुव uint8_t* chunkp;
    सचे (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<ध्रुव uint8_t*>(datap);
    } अञ्ञथा {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<ध्रुव वर्ण*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<ध्रुव uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    पूर्णाङ्क posBegin = 0;  // Position in buffer for start of this block
    पूर्णाङ्क posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    याव (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        अत्थाय (पूर्णाङ्क roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<ध्रुव वर्ण*>(chunkp + posBegin), chunkLen - posEnd);
}

शून्य VHashSha512::निवेशय_File(ध्रुव string& filename) {
    स्थिर constexpr size_t BUFFER_SIZE = 64 * 1024;

    ध्रुव पूर्णाङ्क fd = ::open(filename.c_str(), O_RDONLY);
    सचे (fd < 0) पटिनिवत्त;

    std::array<वर्ण, BUFFER_SIZE + 1> buf;
    याव (ध्रुव ssize_t got = ::पठ(fd, &buf, BUFFER_SIZE)) {
        सचे (got <= 0) भङ्ग;
        निवेशय(&buf, got);
    }
    ::close(fd);
}

शून्य VHashSha512::finalize() {
    सचे (!m_final) {
        // Make sure no 128 byte blocks left
        निवेशय("");
        m_final = सच्च;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        अत्थाय (पूर्णाङ्क i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        अत्थाय (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        सचे (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            अत्थाय (पूर्णाङ्क i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_द्विमान() {
    finalize();
    string result;
    result.reserve(64);
    अत्थाय (size_t i = 0; i < 64; ++i) {
        result += static_cast<वर्ण>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    पटिनिवत्त result;
}

uint64_t VHashSha512::digestUInt64() {
    ध्रुव string& binhash = digest_द्विमान();
    uint64_t result = 0;
    अत्थाय (size_t byte = 0; byte < परिमाण(uint64_t); ++byte) {
        ध्रुव unsigned वर्ण c = binhash[byte];
        result = (result << 8) | c;
    }
    पटिनिवत्त result;
}

string VHashSha512::digestHex() {
    स्थिर constexpr ध्रुव वर्ण* ध्रुव digits = "0123456789abcdef";
    ध्रुव string& binhash = digest_द्विमान();
    string result;
    result.reserve(128);
    अत्थाय (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    पटिनिवत्त result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    पटिनिवत्त VString::base64Enc(digest_द्विमान());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    स्थिर constexpr ध्रुव वर्ण* ध्रुव digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ध्रुव string& binhash = digest_द्विमान();
    string result;
    result.reserve(84);
    पूर्णाङ्क pos = 0;
    अत्थाय (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<पूर्णाङ्क>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<पूर्णाङ्क>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        सचे (result.size() >= DIGEST_SYMBOL24_LENGTH) भङ्ग;
    }
    // Any leftover bits don't matter for our purpose
    पटिनिवत्त result;
}

शून्य VHashSha512::selfTestOne(ध्रुव string& data, ध्रुव string& data2, ध्रुव string& exp,
                              ध्रुव string& exp64, ध्रुव string& exp24) {
    VHashSha512 digest{data};
    सचे (data2 != "") digest.निवेशय(data2);
    सचे (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    सचे (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    सचे (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
}

शून्य VHashSha512::selfTest() {
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

string VName::dehash(ध्रुव string& in) {
    स्थिर constexpr ध्रुव वर्ण VHSH[] = "__Vhsh";
    स्थिर constexpr size_t VHSH_LEN = परिमाण(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    स्थिर ध्रुव size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    अत्थाय (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        ध्रुव string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ध्रुव auto अन्वेषणम्_begin = std::begin(in) + last_dot_pos;
        ध्रुव auto अन्वेषणम्_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        ध्रुव auto begin_vhsh
            = std::अन्वेषणम्(अन्वेषणम्_begin, अन्वेषणम्_end, std::begin(VHSH), std::end(VHSH) - 1);
        सचे (begin_vhsh != अन्वेषणम्_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ध्रुव auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, अन्वेषणम्_end), VHSH_LEN);
            ध्रुव std::string vhsh{begin_vhsh, end_vhsh};
            ध्रुव auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            सचे (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{अन्वेषणम्_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, अन्वेषणम्_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        अञ्ञथा सचे (!dehashed.empty()) {
            dehashed += std::string{अन्वेषणम्_begin, अन्वेषणम्_end};
        }

        सचे (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            सचे (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } अञ्ञथा {
            last_dot_pos = string::npos;
        }
    }
    पटिनिवत्त dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    सचे (m_name == "") पटिनिवत्त "";
    सचे (m_hashed != "") पटिनिवत्त m_hashed;  // Memoized
    सचे (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        पटिनिवत्त m_hashed;
    }
    VHashSha512 hash{m_name};
    ध्रुव string suffix = "__Vhsh" + hash.digestSymbol24();
    सचे (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        याव (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } अञ्ञथा {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    पटिनिवत्त m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ध्रुव string& s, ध्रुव string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ध्रुव size_t sLen = s.length();
    ध्रुव size_t tLen = t.length();
    सचे (sLen == 0) पटिनिवत्त tLen;
    सचे (tLen == 0) पटिनिवत्त sLen;
    सचे (sLen >= LENGTH_LIMIT) पटिनिवत्त sLen;
    सचे (tLen >= LENGTH_LIMIT) पटिनिवत्त tLen;

    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    अत्थाय (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    अत्थाय (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        अत्थाय (size_t j = 0; j < sLen; j++) {
            ध्रुव EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ध्रुव EditDistance deletion = s_v_next[j] + 1;
            ध्रुव EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ध्रुव EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            सचे (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ध्रुव EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        अत्थाय (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    ध्रुव EditDistance result = s_v_next[sLen];
    पटिनिवत्त result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    ध्रुव size_t max_length = std::max(goal_len, candidate_len);
    ध्रुव size_t min_length = std::min(goal_len, candidate_len);
    सचे (max_length <= 1) पटिनिवत्त 0;
    सचे (max_length - min_length <= 1) पटिनिवत्त std::max(max_length / 3, static_cast<size_t>(1));
    पटिनिवत्त (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ध्रुव string& goal,
                                                      EditDistance& distancer) ध्रुव {
    Candidate best;
    ध्रुव size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    अत्थाय (ध्रुव Candidate& candidate : m_candidates) {
        ध्रुव size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        ध्रुव EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        सचे (min_distance >= distancer) अनुवर्त;  // Short-circuit if already better

        ध्रुव EditDistance cutoff = cutoffDistance(gLen, cLen);
        सचे (min_distance > cutoff) अनुवर्त;  // Short-circuit if already too bad

        ध्रुव EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.सन्देशः
                                      << "'");
        सचे (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    सचे (distancer == 0) पटिनिवत्त Candidate{.str = best.str, .सन्देशः = ""};
    पटिनिवत्त best;
}

शून्य VSpellCheck::selfTestDistanceOne(ध्रुव string& a, ध्रुव string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

शून्य VSpellCheck::selfTestSuggestOne(तार्किक matches, ध्रुव string& c, ध्रुव string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.प्रक्षिप_Candidate(c);
    ध्रुव string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    सचे (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } अञ्ञथा {
        UASSERT_SELFTEST(got, "");
    }
}

शून्य VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(सच्च, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(सच्च, "abcdef", "acbdef", 1);
        selfTestSuggestOne(सच्च, "db", "dc", 1);
        selfTestSuggestOne(सच्च, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(मिच्छा, "x", "y", 1);
        selfTestSuggestOne(मिच्छा, "sqrt", "assert", 3);
    }
    {
        ध्रुव VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.प्रक्षिप_Candidate("fred");
        speller.प्रक्षिप_Candidate("wilma");
        speller.प्रक्षिप_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        अत्थाय (unsigned i = 0; i < अङ्कः_CANDIDATE_LIMIT; ++i) speller.प्रक्षिप_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), अङ्कः_CANDIDATE_LIMIT);
        speller.प्रक्षिप_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), अङ्कः_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
