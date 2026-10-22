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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#எனில் defined(_WIN32) || defined(__MINGW32__)
#அந்தர்பாவ <io.h>  // open, read, write, close
#endif

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#அந்தர்பாவ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <தருக்க Even = உண்மை>
நிலையான தருக்க wildMatchImpl(மாறிலி எழுத்து* s, மாறிலி எழுத்து* p) VL_PURE {
    க்கு (; *p; s++, p++) {
        எனில் (*p != '*') {
            எனில் (((*s) != (*p)) && *p != '?') திரும்பு பொய்;
        } இல்லையெனில் {
            // Trailing star matches everything.
            எனில் (!*++p) திரும்பு உண்மை;
            போது (!wildMatchImpl<!Even>(s, p)) {
                எனில் (*++s == '\0') திரும்பு பொய்;
            }
            திரும்பு உண்மை;
        }
    }
    திரும்பு (*s == '\0');
}

தருக்க VString::wildmatch(மாறிலி எழுத்து* s, மாறிலி எழுத்து* p) VL_PURE {
    எனில் (*s == '\0') {
        போது (*p == '*') ++p;
        திரும்பு *p == '\0';
    }
    திரும்பு wildMatchImpl(s, p);
}

தருக்க VString::wildmatch(மாறிலி string& s, மாறிலி string& p) VL_PURE {
    திரும்பு wildmatch(s.c_str(), p.c_str());
}

string VString::dot(மாறிலி string& a, மாறிலி string& dot, மாறிலி string& b) {
    எனில் (b == "") திரும்பு a;
    எனில் (a == "") திரும்பு b;
    திரும்பு a + dot + b;
}

string VString::downcase(மாறிலி string& str) VL_PURE {
    string result = str;
    க்கு (எழுத்து& cr : result) cr = std::tolower(cr);
    திரும்பு result;
}

string VString::upcase(மாறிலி string& str) VL_PURE {
    string result = str;
    க்கு (எழுத்து& cr : result) cr = std::toupper(cr);
    திரும்பு result;
}

string VString::quoteAny(மாறிலி string& str, எழுத்து tgt, எழுத்து esc) VL_PURE {
    string result;
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (c == tgt) result += esc;
        result += c;
    }
    திரும்பு result;
}

string VString::dequotePercent(மாறிலி string& str) {
    string result;
    எழுத்து last = '\0';
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (last == '%' && c == '%') {
            last = '\0';
        } இல்லையெனில் {
            result += c;
            last = c;
        }
    }
    திரும்பு result;
}

string VString::quoteStringLiteralForShell(மாறிலி string& str) {
    string result;
    constexpr எழுத்து dquote = '"';
    constexpr எழுத்து escape = '\\';
    result.தள்ளு_back(dquote);  // Start quoted string
    result.தள்ளு_back(escape);
    result.தள்ளு_back(dquote);  // "
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (c == dquote || c == escape) result.தள்ளு_back(escape);
        result.தள்ளு_back(c);
    }
    result.தள்ளு_back(escape);
    result.தள்ளு_back(dquote);  // "
    result.தள்ளு_back(dquote);  // Terminate quoted string
    திரும்பு result;
}

string VString::escapeStringForPath(மாறிலி string& str) {
    எனில் (str.find(R"(\\)") != string::npos)
        திரும்பு str;  // if it has been escaped already, don't do it again
    எனில் (str.find('/') != string::npos) திரும்பு str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr எழுத்து space = ' ';  // escape space like this `Program Files`
    constexpr எழுத்து escape = '\\';
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (c == space || c == escape) result.தள்ளு_back(escape);
        result.தள்ளு_back(c);
    }
    திரும்பு result;
}

நிலையான முழு vl_decodexdigit(எழுத்து c) {
    திரும்பு std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(மாறிலி string& text, string& errOut) {
    தருக்க quoted = பொய்;
    string newtext;
    newtext.reserve(text.size());
    unsigned எழுத்து octal_val = 0;
    முழு octal_digits = 0;
    க்கு (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        எனில் (quoted) {
            எனில் (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                எனில் (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = பொய்;
                }
            } இல்லையெனில் {
                எனில் (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = பொய்;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    தொடர்;
                }
                quoted = பொய்;
                எனில் (*cp == 'n') {
                    newtext += '\n';
                } இல்லையெனில் எனில் (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } இல்லையெனில் எனில் (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } இல்லையெனில் எனில் (*cp == 'r') {
                    newtext += '\r';
                } இல்லையெனில் எனில் (*cp == 't') {
                    newtext += '\t';
                } இல்லையெனில் எனில் (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } இல்லையெனில் எனில் (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<எழுத்து>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } இல்லையெனில் எனில் (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    நிறுத்து;
                } இல்லையெனில் {
                    newtext += *cp;
                }
            }
        } இல்லையெனில் எனில் (*cp == '\\') {
            எனில் (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = உண்மை;
            octal_digits = 0;
        } இல்லையெனில் {
            newtext += *cp;
        }
    }
    திரும்பு newtext;
}

size_t VString::quotedEnd(மாறிலி string& str, size_t pos) VL_PURE {
    க்கு (size_t i = pos + 1; i < str.size(); ++i) {
        எனில் (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } இல்லையெனில் எனில் (str[i] == '"') {
            திரும்பு i + 1;
        }
    }
    திரும்பு string::npos;
}

string VString::spaceUnprintable(மாறிலி string& str) VL_PURE {
    string result;
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (std::isprint(c)) {
            result += c;
        } இல்லையெனில் {
            result += ' ';
        }
    }
    திரும்பு result;
}

string VString::removeWhitespace(மாறிலி string& str) {
    string result;
    result.reserve(str.size());
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (!std::isspace(c)) result += c;
    }
    திரும்பு result;
}

string VString::trimWhitespace(மாறிலி string& str) {
    string result;
    result.reserve(str.size());
    string add;
    தருக்க newline = பொய்;
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (newline && std::isspace(c)) தொடர்;
        எனில் (c == '\n') {
            add = "\n";
            newline = உண்மை;
            தொடர்;
        }
        எனில் (std::isspace(c)) {
            add += c;
            தொடர்;
        }
        எனில் (!add.empty()) {
            result += add;
            newline = பொய்;
            add.clear();
        }
        result += c;
    }
    திரும்பு result;
}

தருக்க VString::isIdentifier(மாறிலி string& str) {
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (!isIdentifierChar(c)) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}

தருக்க VString::isWhitespace(மாறிலி string& str) {
    க்கு (மாறிலி எழுத்து c : str) {
        எனில் (!std::isspace(c)) திரும்பு பொய்;
    }
    திரும்பு உண்மை;
}

string::size_type VString::leading_Whitespace_மொத்தம்(மாறிலி string& str) {
    string::size_type result = 0;
    க்கு (மாறிலி எழுத்து c : str) {
        ++result;
        எனில் (!std::isspace(c)) நிறுத்து;
    }
    திரும்பு result;
}

இரட்டை VString::parseDouble(மாறிலி string& str, தருக்க* successp) {
    எழுத்து* மாறிலி strgp = புதிய எழுத்து[str.size() + 1];
    எழுத்து* dp = strgp;
    எனில் (successp) *successp = உண்மை;
    க்கு (மாறிலி எழுத்து* sp = str.c_str(); *sp; ++sp) {
        எனில் (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    எழுத்து* endp = strgp;
    மாறிலி இரட்டை d = strtod(strgp, &endp);
    மாறிலி size_t parsed_len = endp - strgp;
    எனில் (parsed_len != std::strlen(strgp)) {
        எனில் (successp) *successp = பொய்;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    திரும்பு d;
}

string VString::replaceSubstr(மாறிலி string& str, மாறிலி string& from, மாறிலி string& to) {
    string result = str;
    மாறிலி size_t fromLen = from.size();
    மாறிலி size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    க்கு (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    திரும்பு result;
}

string VString::replaceWord(மாறிலி string& str, மாறிலி string& from, மாறிலி string& to) {
    string result = str;
    மாறிலி size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    க்கு (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        எனில் (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            தொடர்;
        }
        result.replace(pos, len, to);
    }
    திரும்பு result;
}

std::deque<string> VString::split(மாறிலி string& str, எழுத்து delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    போது (std::getline(is, token, delimiter)) results.தள்ளு_back(token);
    திரும்பு results;
}

தருக்க VString::endsWith(மாறிலி string& str, மாறிலி string& suffix) {
    எனில் (str.length() < suffix.length()) திரும்பு பொய்;
    திரும்பு str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(மாறிலி எழுத்து* word) {
    தேர்வு (word[0]) {
    நிலை '\0': திரும்பு "";
    நிலை 'a':
    நிலை 'e':
    நிலை 'i':
    நிலை 'o':
    நிலை 'u': திரும்பு "an";
    இயல்பு: திரும்பு "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(மாறிலி string& str) VL_PURE {
    மாறிலி எழுத்து* சாவி = str.c_str();
    மாறிலி size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr முழு r = 47;

    uint64_t h = seed ^ (len * m);

    மாறிலி uint64_t* data = reinterpret_cast<மாறிலி uint64_t*>(சாவி);
    மாறிலி uint64_t* end = data + (len / 8);

    போது (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    மாறிலி unsigned எழுத்து* data2 = reinterpret_cast<மாறிலி unsigned எழுத்து*>(data);

    தேர்வு (len & 7) {
    நிலை 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    நிலை 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    நிலை 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    நிலை 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    நிலை 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    நிலை 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    நிலை 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    இயல்பு:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    திரும்பு h;
}

string VString::base64Enc(மாறிலி string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    நிலையான constexpr மாறிலி எழுத்து* மாறிலி digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    எனில் (len >= 3) {
        க்கு (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<முழு>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<முழு>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    எனில் (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<முழு>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<முழு>(0) >> 6)];
        result += '=';
    } இல்லையெனில் எனில் (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<முழு>(0) >> 4)];
        result += '=';
        result += '=';
    }
    திரும்பு result;
}

வெறுமை VString::selfTest() {
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

நிலையான constexpr uint64_t sha512K[]
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
நிலையான uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    திரும்பு lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
நிலையான வெறுமை sha512Block(uint64_t* h, மாறிலி uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    மாறிலி uint64_t* p = chunk;

    // Initialize working variables to current hash value
    க்கு (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    க்கு (unsigned i = 0; i < 5; ++i) {
        க்கு (unsigned j = 0; j < 16; ++j) {
            எனில் (i == 0) {
                w[j] = *p++;
            } இல்லையெனில் {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                மாறிலி uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                மாறிலி uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            மாறிலி uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            மாறிலி uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            மாறிலி uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            மாறிலி uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            மாறிலி uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            மாறிலி uint64_t temp2 = s0 + maj;

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
    க்கு (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

வெறுமை VHashSha512::செருகு(மாறிலி வெறுமை* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    முழு chunkLen;
    மாறிலி uint8_t* chunkp;
    எனில் (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<மாறிலி uint8_t*>(datap);
    } இல்லையெனில் {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<மாறிலி எழுத்து*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<மாறிலி uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    முழு posBegin = 0;  // Position in buffer for start of this block
    முழு posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    போது (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        க்கு (முழு roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<மாறிலி எழுத்து*>(chunkp + posBegin), chunkLen - posEnd);
}

வெறுமை VHashSha512::செருகு_File(மாறிலி string& filename) {
    நிலையான constexpr size_t BUFFER_SIZE = 64 * 1024;

    மாறிலி முழு fd = ::open(filename.c_str(), O_RDONLY);
    எனில் (fd < 0) திரும்பு;

    std::array<எழுத்து, BUFFER_SIZE + 1> buf;
    போது (மாறிலி ssize_t got = ::பட(fd, &buf, BUFFER_SIZE)) {
        எனில் (got <= 0) நிறுத்து;
        செருகு(&buf, got);
    }
    ::close(fd);
}

வெறுமை VHashSha512::finalize() {
    எனில் (!m_final) {
        // Make sure no 128 byte blocks left
        செருகு("");
        m_final = உண்மை;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        க்கு (முழு i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        க்கு (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        எனில் (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            க்கு (முழு i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_இருமம்() {
    finalize();
    string result;
    result.reserve(64);
    க்கு (size_t i = 0; i < 64; ++i) {
        result += static_cast<எழுத்து>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    திரும்பு result;
}

uint64_t VHashSha512::digestUInt64() {
    மாறிலி string& binhash = digest_இருமம்();
    uint64_t result = 0;
    க்கு (size_t byte = 0; byte < அளவு(uint64_t); ++byte) {
        மாறிலி unsigned எழுத்து c = binhash[byte];
        result = (result << 8) | c;
    }
    திரும்பு result;
}

string VHashSha512::digestHex() {
    நிலையான constexpr மாறிலி எழுத்து* மாறிலி digits = "0123456789abcdef";
    மாறிலி string& binhash = digest_இருமம்();
    string result;
    result.reserve(128);
    க்கு (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    திரும்பு result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    திரும்பு VString::base64Enc(digest_இருமம்());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    நிலையான constexpr மாறிலி எழுத்து* மாறிலி digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    மாறிலி string& binhash = digest_இருமம்();
    string result;
    result.reserve(84);
    முழு pos = 0;
    க்கு (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<முழு>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<முழு>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        எனில் (result.size() >= DIGEST_SYMBOL24_LENGTH) நிறுத்து;
    }
    // Any leftover bits don't matter for our purpose
    திரும்பு result;
}

வெறுமை VHashSha512::selfTestOne(மாறிலி string& data, மாறிலி string& data2, மாறிலி string& exp,
                              மாறிலி string& exp64, மாறிலி string& exp24) {
    VHashSha512 digest{data};
    எனில் (data2 != "") digest.செருகு(data2);
    எனில் (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    எனில் (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    எனில் (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
}

வெறுமை VHashSha512::selfTest() {
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

string VName::dehash(மாறிலி string& in) {
    நிலையான constexpr மாறிலி எழுத்து VHSH[] = "__Vhsh";
    நிலையான constexpr size_t VHSH_LEN = அளவு(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    நிலையான மாறிலி size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    க்கு (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        மாறிலி string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        மாறிலி auto தேடல்_begin = std::begin(in) + last_dot_pos;
        மாறிலி auto தேடல்_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        மாறிலி auto begin_vhsh
            = std::தேடல்(தேடல்_begin, தேடல்_end, std::begin(VHSH), std::end(VHSH) - 1);
        எனில் (begin_vhsh != தேடல்_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            மாறிலி auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, தேடல்_end), VHSH_LEN);
            மாறிலி std::string vhsh{begin_vhsh, end_vhsh};
            மாறிலி auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            எனில் (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{தேடல்_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, தேடல்_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        இல்லையெனில் எனில் (!dehashed.empty()) {
            dehashed += std::string{தேடல்_begin, தேடல்_end};
        }

        எனில் (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            எனில் (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } இல்லையெனில் {
            last_dot_pos = string::npos;
        }
    }
    திரும்பு dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    எனில் (m_name == "") திரும்பு "";
    எனில் (m_hashed != "") திரும்பு m_hashed;  // Memoized
    எனில் (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        திரும்பு m_hashed;
    }
    VHashSha512 hash{m_name};
    மாறிலி string suffix = "__Vhsh" + hash.digestSymbol24();
    எனில் (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        போது (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } இல்லையெனில் {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    திரும்பு m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(மாறிலி string& s, மாறிலி string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    மாறிலி size_t sLen = s.length();
    மாறிலி size_t tLen = t.length();
    எனில் (sLen == 0) திரும்பு tLen;
    எனில் (tLen == 0) திரும்பு sLen;
    எனில் (sLen >= LENGTH_LIMIT) திரும்பு sLen;
    எனில் (tLen >= LENGTH_LIMIT) திரும்பு tLen;

    நிலையான std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    நிலையான std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    நிலையான std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    க்கு (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    க்கு (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        க்கு (size_t j = 0; j < sLen; j++) {
            மாறிலி EditDistance cost = (s[j] == t[i] ? 0 : 1);
            மாறிலி EditDistance deletion = s_v_next[j] + 1;
            மாறிலி EditDistance insertion = s_v_one_ago[j + 1] + 1;
            மாறிலி EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            எனில் (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                மாறிலி EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        க்கு (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    மாறிலி EditDistance result = s_v_next[sLen];
    திரும்பு result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    மாறிலி size_t max_length = std::max(goal_len, candidate_len);
    மாறிலி size_t min_length = std::min(goal_len, candidate_len);
    எனில் (max_length <= 1) திரும்பு 0;
    எனில் (max_length - min_length <= 1) திரும்பு std::max(max_length / 3, static_cast<size_t>(1));
    திரும்பு (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(மாறிலி string& goal,
                                                      EditDistance& distancer) மாறிலி {
    Candidate best;
    மாறிலி size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    க்கு (மாறிலி Candidate& candidate : m_candidates) {
        மாறிலி size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        மாறிலி EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        எனில் (min_distance >= distancer) தொடர்;  // Short-circuit if already better

        மாறிலி EditDistance cutoff = cutoffDistance(gLen, cLen);
        எனில் (min_distance > cutoff) தொடர்;  // Short-circuit if already too bad

        மாறிலி EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.செய்தி
                                      << "'");
        எனில் (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    எனில் (distancer == 0) திரும்பு Candidate{.str = best.str, .செய்தி = ""};
    திரும்பு best;
}

வெறுமை VSpellCheck::selfTestDistanceOne(மாறிலி string& a, மாறிலி string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

வெறுமை VSpellCheck::selfTestSuggestOne(தருக்க matches, மாறிலி string& c, மாறிலி string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.தள்ளு_Candidate(c);
    மாறிலி string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    எனில் (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } இல்லையெனில் {
        UASSERT_SELFTEST(got, "");
    }
}

வெறுமை VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(உண்மை, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(உண்மை, "abcdef", "acbdef", 1);
        selfTestSuggestOne(உண்மை, "db", "dc", 1);
        selfTestSuggestOne(உண்மை, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(பொய், "x", "y", 1);
        selfTestSuggestOne(பொய், "sqrt", "assert", 3);
    }
    {
        மாறிலி VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.தள்ளு_Candidate("fred");
        speller.தள்ளு_Candidate("wilma");
        speller.தள்ளு_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        க்கு (unsigned i = 0; i < இலக்கம்_CANDIDATE_LIMIT; ++i) speller.தள்ளு_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), இலக்கம்_CANDIDATE_LIMIT);
        speller.தள்ளு_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), இலக்கம்_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
