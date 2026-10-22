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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଯଦି defined(_WIN32) || defined(__MINGW32__)
#ଅନ୍ତର୍ଭାଵ <io.h>  // open, read, write, close
#endif

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <ତାର୍କିକ Even = ସତ୍ୟ>
ସ୍ଥିର ତାର୍କିକ wildMatchImpl(ଅଚଳ ଅକ୍ଷର* s, ଅଚଳ ଅକ୍ଷର* p) VL_PURE {
    ପାଇଁ (; *p; s++, p++) {
        ଯଦି (*p != '*') {
            ଯଦି (((*s) != (*p)) && *p != '?') ଫେରନ୍ତୁ ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            // Trailing star matches everything.
            ଯଦି (!*++p) ଫେରନ୍ତୁ ସତ୍ୟ;
            ଯେତେବେଳେ (!wildMatchImpl<!Even>(s, p)) {
                ଯଦି (*++s == '\0') ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }
    }
    ଫେରନ୍ତୁ (*s == '\0');
}

ତାର୍କିକ VString::wildmatch(ଅଚଳ ଅକ୍ଷର* s, ଅଚଳ ଅକ୍ଷର* p) VL_PURE {
    ଯଦି (*s == '\0') {
        ଯେତେବେଳେ (*p == '*') ++p;
        ଫେରନ୍ତୁ *p == '\0';
    }
    ଫେରନ୍ତୁ wildMatchImpl(s, p);
}

ତାର୍କିକ VString::wildmatch(ଅଚଳ string& s, ଅଚଳ string& p) VL_PURE {
    ଫେରନ୍ତୁ wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ଅଚଳ string& a, ଅଚଳ string& dot, ଅଚଳ string& b) {
    ଯଦି (b == "") ଫେରନ୍ତୁ a;
    ଯଦି (a == "") ଫେରନ୍ତୁ b;
    ଫେରନ୍ତୁ a + dot + b;
}

string VString::downcase(ଅଚଳ string& str) VL_PURE {
    string result = str;
    ପାଇଁ (ଅକ୍ଷର& cr : result) cr = std::tolower(cr);
    ଫେରନ୍ତୁ result;
}

string VString::upcase(ଅଚଳ string& str) VL_PURE {
    string result = str;
    ପାଇଁ (ଅକ୍ଷର& cr : result) cr = std::toupper(cr);
    ଫେରନ୍ତୁ result;
}

string VString::quoteAny(ଅଚଳ string& str, ଅକ୍ଷର tgt, ଅକ୍ଷର esc) VL_PURE {
    string result;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (c == tgt) result += esc;
        result += c;
    }
    ଫେରନ୍ତୁ result;
}

string VString::dequotePercent(ଅଚଳ string& str) {
    string result;
    ଅକ୍ଷର last = '\0';
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (last == '%' && c == '%') {
            last = '\0';
        } ଅନ୍ୟଥା {
            result += c;
            last = c;
        }
    }
    ଫେରନ୍ତୁ result;
}

string VString::quoteStringLiteralForShell(ଅଚଳ string& str) {
    string result;
    constexpr ଅକ୍ଷର dquote = '"';
    constexpr ଅକ୍ଷର escape = '\\';
    result.ଠେଲ_back(dquote);  // Start quoted string
    result.ଠେଲ_back(escape);
    result.ଠେଲ_back(dquote);  // "
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (c == dquote || c == escape) result.ଠେଲ_back(escape);
        result.ଠେଲ_back(c);
    }
    result.ଠେଲ_back(escape);
    result.ଠେଲ_back(dquote);  // "
    result.ଠେଲ_back(dquote);  // Terminate quoted string
    ଫେରନ୍ତୁ result;
}

string VString::escapeStringForPath(ଅଚଳ string& str) {
    ଯଦି (str.find(R"(\\)") != string::npos)
        ଫେରନ୍ତୁ str;  // if it has been escaped already, don't do it again
    ଯଦି (str.find('/') != string::npos) ଫେରନ୍ତୁ str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr ଅକ୍ଷର space = ' ';  // escape space like this `Program Files`
    constexpr ଅକ୍ଷର escape = '\\';
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (c == space || c == escape) result.ଠେଲ_back(escape);
        result.ଠେଲ_back(c);
    }
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା vl_decodexdigit(ଅକ୍ଷର c) {
    ଫେରନ୍ତୁ std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ଅଚଳ string& text, string& errOut) {
    ତାର୍କିକ quoted = ମିଥ୍ୟା;
    string newtext;
    newtext.reserve(text.size());
    unsigned ଅକ୍ଷର octal_val = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା octal_digits = 0;
    ପାଇଁ (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        ଯଦି (quoted) {
            ଯଦି (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                ଯଦି (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ମିଥ୍ୟା;
                }
            } ଅନ୍ୟଥା {
                ଯଦି (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ମିଥ୍ୟା;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    ଜାରି;
                }
                quoted = ମିଥ୍ୟା;
                ଯଦି (*cp == 'n') {
                    newtext += '\n';
                } ଅନ୍ୟଥା ଯଦି (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } ଅନ୍ୟଥା ଯଦି (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } ଅନ୍ୟଥା ଯଦି (*cp == 'r') {
                    newtext += '\r';
                } ଅନ୍ୟଥା ଯଦି (*cp == 't') {
                    newtext += '\t';
                } ଅନ୍ୟଥା ଯଦି (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } ଅନ୍ୟଥା ଯଦି (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<ଅକ୍ଷର>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } ଅନ୍ୟଥା ଯଦି (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    ଭାଙ୍ଗ;
                } ଅନ୍ୟଥା {
                    newtext += *cp;
                }
            }
        } ଅନ୍ୟଥା ଯଦି (*cp == '\\') {
            ଯଦି (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = ସତ୍ୟ;
            octal_digits = 0;
        } ଅନ୍ୟଥା {
            newtext += *cp;
        }
    }
    ଫେରନ୍ତୁ newtext;
}

size_t VString::quotedEnd(ଅଚଳ string& str, size_t pos) VL_PURE {
    ପାଇଁ (size_t i = pos + 1; i < str.size(); ++i) {
        ଯଦି (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } ଅନ୍ୟଥା ଯଦି (str[i] == '"') {
            ଫେରନ୍ତୁ i + 1;
        }
    }
    ଫେରନ୍ତୁ string::npos;
}

string VString::spaceUnprintable(ଅଚଳ string& str) VL_PURE {
    string result;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (std::isprint(c)) {
            result += c;
        } ଅନ୍ୟଥା {
            result += ' ';
        }
    }
    ଫେରନ୍ତୁ result;
}

string VString::removeWhitespace(ଅଚଳ string& str) {
    string result;
    result.reserve(str.size());
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (!std::isspace(c)) result += c;
    }
    ଫେରନ୍ତୁ result;
}

string VString::trimWhitespace(ଅଚଳ string& str) {
    string result;
    result.reserve(str.size());
    string add;
    ତାର୍କିକ newline = ମିଥ୍ୟା;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (newline && std::isspace(c)) ଜାରି;
        ଯଦି (c == '\n') {
            add = "\n";
            newline = ସତ୍ୟ;
            ଜାରି;
        }
        ଯଦି (std::isspace(c)) {
            add += c;
            ଜାରି;
        }
        ଯଦି (!add.empty()) {
            result += add;
            newline = ମିଥ୍ୟା;
            add.clear();
        }
        result += c;
    }
    ଫେରନ୍ତୁ result;
}

ତାର୍କିକ VString::isIdentifier(ଅଚଳ string& str) {
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (!isIdentifierChar(c)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ତାର୍କିକ VString::isWhitespace(ଅଚଳ string& str) {
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ଯଦି (!std::isspace(c)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

string::size_type VString::leading_Whitespace_ଗଣନା(ଅଚଳ string& str) {
    string::size_type result = 0;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର c : str) {
        ++result;
        ଯଦି (!std::isspace(c)) ଭାଙ୍ଗ;
    }
    ଫେରନ୍ତୁ result;
}

ଦ୍ୱିଗୁଣ VString::parseDouble(ଅଚଳ string& str, ତାର୍କିକ* successp) {
    ଅକ୍ଷର* ଅଚଳ strgp = ନୂତନ ଅକ୍ଷର[str.size() + 1];
    ଅକ୍ଷର* dp = strgp;
    ଯଦି (successp) *successp = ସତ୍ୟ;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* sp = str.c_str(); *sp; ++sp) {
        ଯଦି (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    ଅକ୍ଷର* endp = strgp;
    ଅଚଳ ଦ୍ୱିଗୁଣ d = strtod(strgp, &endp);
    ଅଚଳ size_t parsed_len = endp - strgp;
    ଯଦି (parsed_len != std::strlen(strgp)) {
        ଯଦି (successp) *successp = ମିଥ୍ୟା;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    ଫେରନ୍ତୁ d;
}

string VString::replaceSubstr(ଅଚଳ string& str, ଅଚଳ string& from, ଅଚଳ string& to) {
    string result = str;
    ଅଚଳ size_t fromLen = from.size();
    ଅଚଳ size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    ପାଇଁ (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    ଫେରନ୍ତୁ result;
}

string VString::replaceWord(ଅଚଳ string& str, ଅଚଳ string& from, ଅଚଳ string& to) {
    string result = str;
    ଅଚଳ size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    ପାଇଁ (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        ଯଦି (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            ଜାରି;
        }
        result.replace(pos, len, to);
    }
    ଫେରନ୍ତୁ result;
}

std::deque<string> VString::split(ଅଚଳ string& str, ଅକ୍ଷର delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    ଯେତେବେଳେ (std::getline(is, token, delimiter)) results.ଠେଲ_back(token);
    ଫେରନ୍ତୁ results;
}

ତାର୍କିକ VString::endsWith(ଅଚଳ string& str, ଅଚଳ string& suffix) {
    ଯଦି (str.length() < suffix.length()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(ଅଚଳ ଅକ୍ଷର* word) {
    ବାଛ (word[0]) {
    ସ୍ଥିତି '\0': ଫେରନ୍ତୁ "";
    ସ୍ଥିତି 'a':
    ସ୍ଥିତି 'e':
    ସ୍ଥିତି 'i':
    ସ୍ଥିତି 'o':
    ସ୍ଥିତି 'u': ଫେରନ୍ତୁ "an";
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ଅଚଳ string& str) VL_PURE {
    ଅଚଳ ଅକ୍ଷର* ଚାବି = str.c_str();
    ଅଚଳ size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା r = 47;

    uint64_t h = seed ^ (len * m);

    ଅଚଳ uint64_t* data = reinterpret_cast<ଅଚଳ uint64_t*>(ଚାବି);
    ଅଚଳ uint64_t* end = data + (len / 8);

    ଯେତେବେଳେ (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ଅଚଳ unsigned ଅକ୍ଷର* data2 = reinterpret_cast<ଅଚଳ unsigned ଅକ୍ଷର*>(data);

    ବାଛ (len & 7) {
    ସ୍ଥିତି 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    ସ୍ଥିତି 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    ସ୍ଥିତି 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    ସ୍ଥିତି 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    ସ୍ଥିତି 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    ସ୍ଥିତି 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    ସ୍ଥିତି 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    ଫେରନ୍ତୁ h;
}

string VString::base64Enc(ଅଚଳ string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    ଯଦି (len >= 3) {
        ପାଇଁ (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    ଯଦି (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(0) >> 6)];
        result += '=';
    } ଅନ୍ୟଥା ଯଦି (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(0) >> 4)];
        result += '=';
        result += '=';
    }
    ଫେରନ୍ତୁ result;
}

ଶୂନ୍ୟ VString::selfTest() {
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

ସ୍ଥିର constexpr uint64_t sha512K[]
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
ସ୍ଥିର uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    ଫେରନ୍ତୁ lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ସ୍ଥିର ଶୂନ୍ୟ sha512Block(uint64_t* h, ଅଚଳ uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ଅଚଳ uint64_t* p = chunk;

    // Initialize working variables to current hash value
    ପାଇଁ (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    ପାଇଁ (unsigned i = 0; i < 5; ++i) {
        ପାଇଁ (unsigned j = 0; j < 16; ++j) {
            ଯଦି (i == 0) {
                w[j] = *p++;
            } ଅନ୍ୟଥା {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ଅଚଳ uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ଅଚଳ uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ଅଚଳ uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ଅଚଳ uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ଅଚଳ uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ଅଚଳ uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ଅଚଳ uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ଅଚଳ uint64_t temp2 = s0 + maj;

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
    ପାଇଁ (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

ଶୂନ୍ୟ VHashSha512::ଭର୍ତ୍ତି(ଅଚଳ ଶୂନ୍ୟ* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା chunkLen;
    ଅଚଳ uint8_t* chunkp;
    ଯଦି (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<ଅଚଳ uint8_t*>(datap);
    } ଅନ୍ୟଥା {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<ଅଚଳ ଅକ୍ଷର*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<ଅଚଳ uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା posBegin = 0;  // Position in buffer for start of this block
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    ଯେତେବେଳେ (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<ଅଚଳ ଅକ୍ଷର*>(chunkp + posBegin), chunkLen - posEnd);
}

ଶୂନ୍ୟ VHashSha512::ଭର୍ତ୍ତି_File(ଅଚଳ string& filename) {
    ସ୍ଥିର constexpr size_t BUFFER_SIZE = 64 * 1024;

    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd = ::open(filename.c_str(), O_RDONLY);
    ଯଦି (fd < 0) ଫେରନ୍ତୁ;

    std::array<ଅକ୍ଷର, BUFFER_SIZE + 1> buf;
    ଯେତେବେଳେ (ଅଚଳ ssize_t got = ::ପଠ(fd, &buf, BUFFER_SIZE)) {
        ଯଦି (got <= 0) ଭାଙ୍ଗ;
        ଭର୍ତ୍ତି(&buf, got);
    }
    ::close(fd);
}

ଶୂନ୍ୟ VHashSha512::finalize() {
    ଯଦି (!m_final) {
        // Make sure no 128 byte blocks left
        ଭର୍ତ୍ତି("");
        m_final = ସତ୍ୟ;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        ପାଇଁ (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        ଯଦି (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ଦ୍ୱିମିକ() {
    finalize();
    string result;
    result.reserve(64);
    ପାଇଁ (size_t i = 0; i < 64; ++i) {
        result += static_cast<ଅକ୍ଷର>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    ଫେରନ୍ତୁ result;
}

uint64_t VHashSha512::digestUInt64() {
    ଅଚଳ string& binhash = digest_ଦ୍ୱିମିକ();
    uint64_t result = 0;
    ପାଇଁ (size_t byte = 0; byte < ଆକାର(uint64_t); ++byte) {
        ଅଚଳ unsigned ଅକ୍ଷର c = binhash[byte];
        result = (result << 8) | c;
    }
    ଫେରନ୍ତୁ result;
}

string VHashSha512::digestHex() {
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ digits = "0123456789abcdef";
    ଅଚଳ string& binhash = digest_ଦ୍ୱିମିକ();
    string result;
    result.reserve(128);
    ପାଇଁ (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    ଫେରନ୍ତୁ result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    ଫେରନ୍ତୁ VString::base64Enc(digest_ଦ୍ୱିମିକ());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ଅଚଳ string& binhash = digest_ଦ୍ୱିମିକ();
    string result;
    result.reserve(84);
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା pos = 0;
    ପାଇଁ (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        ଯଦି (result.size() >= DIGEST_SYMBOL24_LENGTH) ଭାଙ୍ଗ;
    }
    // Any leftover bits don't matter for our purpose
    ଫେରନ୍ତୁ result;
}

ଶୂନ୍ୟ VHashSha512::selfTestOne(ଅଚଳ string& data, ଅଚଳ string& data2, ଅଚଳ string& exp,
                              ଅଚଳ string& exp64, ଅଚଳ string& exp24) {
    VHashSha512 digest{data};
    ଯଦି (data2 != "") digest.ଭର୍ତ୍ତି(data2);
    ଯଦି (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    ଯଦି (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    ଯଦି (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
}

ଶୂନ୍ୟ VHashSha512::selfTest() {
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

string VName::dehash(ଅଚଳ string& in) {
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର VHSH[] = "__Vhsh";
    ସ୍ଥିର constexpr size_t VHSH_LEN = ଆକାର(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    ସ୍ଥିର ଅଚଳ size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    ପାଇଁ (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        ଅଚଳ string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ଅଚଳ auto ସନ୍ଧାନ_begin = std::begin(in) + last_dot_pos;
        ଅଚଳ auto ସନ୍ଧାନ_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        ଅଚଳ auto begin_vhsh
            = std::ସନ୍ଧାନ(ସନ୍ଧାନ_begin, ସନ୍ଧାନ_end, std::begin(VHSH), std::end(VHSH) - 1);
        ଯଦି (begin_vhsh != ସନ୍ଧାନ_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ଅଚଳ auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, ସନ୍ଧାନ_end), VHSH_LEN);
            ଅଚଳ std::string vhsh{begin_vhsh, end_vhsh};
            ଅଚଳ auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            ଯଦି (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{ସନ୍ଧାନ_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, ସନ୍ଧାନ_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        ଅନ୍ୟଥା ଯଦି (!dehashed.empty()) {
            dehashed += std::string{ସନ୍ଧାନ_begin, ସନ୍ଧାନ_end};
        }

        ଯଦି (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            ଯଦି (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } ଅନ୍ୟଥା {
            last_dot_pos = string::npos;
        }
    }
    ଫେରନ୍ତୁ dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    ଯଦି (m_name == "") ଫେରନ୍ତୁ "";
    ଯଦି (m_hashed != "") ଫେରନ୍ତୁ m_hashed;  // Memoized
    ଯଦି (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        ଫେରନ୍ତୁ m_hashed;
    }
    VHashSha512 hash{m_name};
    ଅଚଳ string suffix = "__Vhsh" + hash.digestSymbol24();
    ଯଦି (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        ଯେତେବେଳେ (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } ଅନ୍ୟଥା {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    ଫେରନ୍ତୁ m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ଅଚଳ string& s, ଅଚଳ string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ଅଚଳ size_t sLen = s.length();
    ଅଚଳ size_t tLen = t.length();
    ଯଦି (sLen == 0) ଫେରନ୍ତୁ tLen;
    ଯଦି (tLen == 0) ଫେରନ୍ତୁ sLen;
    ଯଦି (sLen >= LENGTH_LIMIT) ଫେରନ୍ତୁ sLen;
    ଯଦି (tLen >= LENGTH_LIMIT) ଫେରନ୍ତୁ tLen;

    ସ୍ଥିର std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    ସ୍ଥିର std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    ସ୍ଥିର std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    ପାଇଁ (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    ପାଇଁ (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        ପାଇଁ (size_t j = 0; j < sLen; j++) {
            ଅଚଳ EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ଅଚଳ EditDistance deletion = s_v_next[j] + 1;
            ଅଚଳ EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ଅଚଳ EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            ଯଦି (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ଅଚଳ EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        ପାଇଁ (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    ଅଚଳ EditDistance result = s_v_next[sLen];
    ଫେରନ୍ତୁ result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    ଅଚଳ size_t max_length = std::max(goal_len, candidate_len);
    ଅଚଳ size_t min_length = std::min(goal_len, candidate_len);
    ଯଦି (max_length <= 1) ଫେରନ୍ତୁ 0;
    ଯଦି (max_length - min_length <= 1) ଫେରନ୍ତୁ std::max(max_length / 3, static_cast<size_t>(1));
    ଫେରନ୍ତୁ (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ଅଚଳ string& goal,
                                                      EditDistance& distancer) ଅଚଳ {
    Candidate best;
    ଅଚଳ size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    ପାଇଁ (ଅଚଳ Candidate& candidate : m_candidates) {
        ଅଚଳ size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        ଅଚଳ EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        ଯଦି (min_distance >= distancer) ଜାରି;  // Short-circuit if already better

        ଅଚଳ EditDistance cutoff = cutoffDistance(gLen, cLen);
        ଯଦି (min_distance > cutoff) ଜାରି;  // Short-circuit if already too bad

        ଅଚଳ EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.ସନ୍ଦେଶ
                                      << "'");
        ଯଦି (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    ଯଦି (distancer == 0) ଫେରନ୍ତୁ Candidate{.str = best.str, .ସନ୍ଦେଶ = ""};
    ଫେରନ୍ତୁ best;
}

ଶୂନ୍ୟ VSpellCheck::selfTestDistanceOne(ଅଚଳ string& a, ଅଚଳ string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

ଶୂନ୍ୟ VSpellCheck::selfTestSuggestOne(ତାର୍କିକ matches, ଅଚଳ string& c, ଅଚଳ string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.ଠେଲ_Candidate(c);
    ଅଚଳ string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    ଯଦି (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } ଅନ୍ୟଥା {
        UASSERT_SELFTEST(got, "");
    }
}

ଶୂନ୍ୟ VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(ସତ୍ୟ, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(ସତ୍ୟ, "abcdef", "acbdef", 1);
        selfTestSuggestOne(ସତ୍ୟ, "db", "dc", 1);
        selfTestSuggestOne(ସତ୍ୟ, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(ମିଥ୍ୟା, "x", "y", 1);
        selfTestSuggestOne(ମିଥ୍ୟା, "sqrt", "assert", 3);
    }
    {
        ଅଚଳ VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ଠେଲ_Candidate("fred");
        speller.ଠେଲ_Candidate("wilma");
        speller.ଠେଲ_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        ପାଇଁ (unsigned i = 0; i < ଅଙ୍କ_CANDIDATE_LIMIT; ++i) speller.ଠେଲ_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), ଅଙ୍କ_CANDIDATE_LIMIT);
        speller.ଠେଲ_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), ଅଙ୍କ_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
