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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
#ಅನ್ತರ್ಭಾವ <io.h>  // open, read, write, close
#endif

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#ಅನ್ತರ್ಭಾವ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <ತಾರ್ಕಿಕ Even = ಸತ್ಯ>
ಸ್ಥಿರ ತಾರ್ಕಿಕ wildMatchImpl(ಧ್ರುವ ವರ್ಣ* s, ಧ್ರುವ ವರ್ಣ* p) VL_PURE {
    ಪ್ರತಿ (; *p; s++, p++) {
        ಯದಿ (*p != '*') {
            ಯದಿ (((*s) != (*p)) && *p != '?') ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            // Trailing star matches everything.
            ಯದಿ (!*++p) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            ಯಾವತ್ (!wildMatchImpl<!Even>(s, p)) {
                ಯದಿ (*++s == '\0') ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
    }
    ಪ್ರತ್ಯಾಗಮ (*s == '\0');
}

ತಾರ್ಕಿಕ VString::wildmatch(ಧ್ರುವ ವರ್ಣ* s, ಧ್ರುವ ವರ್ಣ* p) VL_PURE {
    ಯದಿ (*s == '\0') {
        ಯಾವತ್ (*p == '*') ++p;
        ಪ್ರತ್ಯಾಗಮ *p == '\0';
    }
    ಪ್ರತ್ಯಾಗಮ wildMatchImpl(s, p);
}

ತಾರ್ಕಿಕ VString::wildmatch(ಧ್ರುವ string& s, ಧ್ರುವ string& p) VL_PURE {
    ಪ್ರತ್ಯಾಗಮ wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ಧ್ರುವ string& a, ಧ್ರುವ string& dot, ಧ್ರುವ string& b) {
    ಯದಿ (b == "") ಪ್ರತ್ಯಾಗಮ a;
    ಯದಿ (a == "") ಪ್ರತ್ಯಾಗಮ b;
    ಪ್ರತ್ಯಾಗಮ a + dot + b;
}

string VString::downcase(ಧ್ರುವ string& str) VL_PURE {
    string result = str;
    ಪ್ರತಿ (ವರ್ಣ& cr : result) cr = std::tolower(cr);
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::upcase(ಧ್ರುವ string& str) VL_PURE {
    string result = str;
    ಪ್ರತಿ (ವರ್ಣ& cr : result) cr = std::toupper(cr);
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::quoteAny(ಧ್ರುವ string& str, ವರ್ಣ tgt, ವರ್ಣ esc) VL_PURE {
    string result;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (c == tgt) result += esc;
        result += c;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::dequotePercent(ಧ್ರುವ string& str) {
    string result;
    ವರ್ಣ last = '\0';
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (last == '%' && c == '%') {
            last = '\0';
        } ಅನ್ಯಥಾ {
            result += c;
            last = c;
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::quoteStringLiteralForShell(ಧ್ರುವ string& str) {
    string result;
    constexpr ವರ್ಣ dquote = '"';
    constexpr ವರ್ಣ escape = '\\';
    result.ತಳ್ಳು_back(dquote);  // Start quoted string
    result.ತಳ್ಳು_back(escape);
    result.ತಳ್ಳು_back(dquote);  // "
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (c == dquote || c == escape) result.ತಳ್ಳು_back(escape);
        result.ತಳ್ಳು_back(c);
    }
    result.ತಳ್ಳು_back(escape);
    result.ತಳ್ಳು_back(dquote);  // "
    result.ತಳ್ಳು_back(dquote);  // Terminate quoted string
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::escapeStringForPath(ಧ್ರುವ string& str) {
    ಯದಿ (str.find(R"(\\)") != string::npos)
        ಪ್ರತ್ಯಾಗಮ str;  // if it has been escaped already, don't do it again
    ಯದಿ (str.find('/') != string::npos) ಪ್ರತ್ಯಾಗಮ str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr ವರ್ಣ space = ' ';  // escape space like this `Program Files`
    constexpr ವರ್ಣ escape = '\\';
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (c == space || c == escape) result.ತಳ್ಳು_back(escape);
        result.ತಳ್ಳು_back(c);
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ vl_decodexdigit(ವರ್ಣ c) {
    ಪ್ರತ್ಯಾಗಮ std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ಧ್ರುವ string& text, string& errOut) {
    ತಾರ್ಕಿಕ quoted = ಅಸತ್ಯ;
    string newtext;
    newtext.reserve(text.size());
    unsigned ವರ್ಣ octal_val = 0;
    ಪೂರ್ಣಾಙ್ಕ octal_digits = 0;
    ಪ್ರತಿ (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        ಯದಿ (quoted) {
            ಯದಿ (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                ಯದಿ (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ಅಸತ್ಯ;
                }
            } ಅನ್ಯಥಾ {
                ಯದಿ (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ಅಸತ್ಯ;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    ಅನುವರ್ತ;
                }
                quoted = ಅಸತ್ಯ;
                ಯದಿ (*cp == 'n') {
                    newtext += '\n';
                } ಅನ್ಯಥಾ ಯದಿ (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } ಅನ್ಯಥಾ ಯದಿ (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } ಅನ್ಯಥಾ ಯದಿ (*cp == 'r') {
                    newtext += '\r';
                } ಅನ್ಯಥಾ ಯದಿ (*cp == 't') {
                    newtext += '\t';
                } ಅನ್ಯಥಾ ಯದಿ (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } ಅನ್ಯಥಾ ಯದಿ (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<ವರ್ಣ>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } ಅನ್ಯಥಾ ಯದಿ (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    ಭಙ್ಗ;
                } ಅನ್ಯಥಾ {
                    newtext += *cp;
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (*cp == '\\') {
            ಯದಿ (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = ಸತ್ಯ;
            octal_digits = 0;
        } ಅನ್ಯಥಾ {
            newtext += *cp;
        }
    }
    ಪ್ರತ್ಯಾಗಮ newtext;
}

size_t VString::quotedEnd(ಧ್ರುವ string& str, size_t pos) VL_PURE {
    ಪ್ರತಿ (size_t i = pos + 1; i < str.size(); ++i) {
        ಯದಿ (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } ಅನ್ಯಥಾ ಯದಿ (str[i] == '"') {
            ಪ್ರತ್ಯಾಗಮ i + 1;
        }
    }
    ಪ್ರತ್ಯಾಗಮ string::npos;
}

string VString::spaceUnprintable(ಧ್ರುವ string& str) VL_PURE {
    string result;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (std::isprint(c)) {
            result += c;
        } ಅನ್ಯಥಾ {
            result += ' ';
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::removeWhitespace(ಧ್ರುವ string& str) {
    string result;
    result.reserve(str.size());
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (!std::isspace(c)) result += c;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::trimWhitespace(ಧ್ರುವ string& str) {
    string result;
    result.reserve(str.size());
    string add;
    ತಾರ್ಕಿಕ newline = ಅಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (newline && std::isspace(c)) ಅನುವರ್ತ;
        ಯದಿ (c == '\n') {
            add = "\n";
            newline = ಸತ್ಯ;
            ಅನುವರ್ತ;
        }
        ಯದಿ (std::isspace(c)) {
            add += c;
            ಅನುವರ್ತ;
        }
        ಯದಿ (!add.empty()) {
            result += add;
            newline = ಅಸತ್ಯ;
            add.clear();
        }
        result += c;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ತಾರ್ಕಿಕ VString::isIdentifier(ಧ್ರುವ string& str) {
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (!isIdentifierChar(c)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

ತಾರ್ಕಿಕ VString::isWhitespace(ಧ್ರುವ string& str) {
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ಯದಿ (!std::isspace(c)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

string::size_type VString::leading_Whitespace_ಎಣಿಕೆ(ಧ್ರುವ string& str) {
    string::size_type result = 0;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ c : str) {
        ++result;
        ಯದಿ (!std::isspace(c)) ಭಙ್ಗ;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ದ್ವಿಗುಣ VString::parseDouble(ಧ್ರುವ string& str, ತಾರ್ಕಿಕ* successp) {
    ವರ್ಣ* ಧ್ರುವ strgp = ನೂತನ ವರ್ಣ[str.size() + 1];
    ವರ್ಣ* dp = strgp;
    ಯದಿ (successp) *successp = ಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* sp = str.c_str(); *sp; ++sp) {
        ಯದಿ (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    ವರ್ಣ* endp = strgp;
    ಧ್ರುವ ದ್ವಿಗುಣ d = strtod(strgp, &endp);
    ಧ್ರುವ size_t parsed_len = endp - strgp;
    ಯದಿ (parsed_len != std::strlen(strgp)) {
        ಯದಿ (successp) *successp = ಅಸತ್ಯ;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    ಪ್ರತ್ಯಾಗಮ d;
}

string VString::replaceSubstr(ಧ್ರುವ string& str, ಧ್ರುವ string& from, ಧ್ರುವ string& to) {
    string result = str;
    ಧ್ರುವ size_t fromLen = from.size();
    ಧ್ರುವ size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    ಪ್ರತಿ (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VString::replaceWord(ಧ್ರುವ string& str, ಧ್ರುವ string& from, ಧ್ರುವ string& to) {
    string result = str;
    ಧ್ರುವ size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    ಪ್ರತಿ (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        ಯದಿ (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            ಅನುವರ್ತ;
        }
        result.replace(pos, len, to);
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

std::deque<string> VString::split(ಧ್ರುವ string& str, ವರ್ಣ delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    ಯಾವತ್ (std::getline(is, token, delimiter)) results.ತಳ್ಳು_back(token);
    ಪ್ರತ್ಯಾಗಮ results;
}

ತಾರ್ಕಿಕ VString::endsWith(ಧ್ರುವ string& str, ಧ್ರುವ string& suffix) {
    ಯದಿ (str.length() < suffix.length()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(ಧ್ರುವ ವರ್ಣ* word) {
    ವಿಕಲ್ಪ (word[0]) {
    ಪಕ್ಷ '\0': ಪ್ರತ್ಯಾಗಮ "";
    ಪಕ್ಷ 'a':
    ಪಕ್ಷ 'e':
    ಪಕ್ಷ 'i':
    ಪಕ್ಷ 'o':
    ಪಕ್ಷ 'u': ಪ್ರತ್ಯಾಗಮ "an";
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ಧ್ರುವ string& str) VL_PURE {
    ಧ್ರುವ ವರ್ಣ* ಕೀಲಿ = str.c_str();
    ಧ್ರುವ size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr ಪೂರ್ಣಾಙ್ಕ r = 47;

    uint64_t h = seed ^ (len * m);

    ಧ್ರುವ uint64_t* data = reinterpret_cast<ಧ್ರುವ uint64_t*>(ಕೀಲಿ);
    ಧ್ರುವ uint64_t* end = data + (len / 8);

    ಯಾವತ್ (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ಧ್ರುವ unsigned ವರ್ಣ* data2 = reinterpret_cast<ಧ್ರುವ unsigned ವರ್ಣ*>(data);

    ವಿಕಲ್ಪ (len & 7) {
    ಪಕ್ಷ 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    ಪಕ್ಷ 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    ಪಕ್ಷ 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    ಪಕ್ಷ 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    ಪಕ್ಷ 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    ಪಕ್ಷ 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    ಪಕ್ಷ 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    ಸ್ವಭಾವ:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    ಪ್ರತ್ಯಾಗಮ h;
}

string VString::base64Enc(ಧ್ರುವ string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    ಯದಿ (len >= 3) {
        ಪ್ರತಿ (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    ಯದಿ (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(0) >> 6)];
        result += '=';
    } ಅನ್ಯಥಾ ಯದಿ (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(0) >> 4)];
        result += '=';
        result += '=';
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ಶೂನ್ಯ VString::selfTest() {
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

ಸ್ಥಿರ constexpr uint64_t sha512K[]
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
ಸ್ಥಿರ uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    ಪ್ರತ್ಯಾಗಮ lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ಸ್ಥಿರ ಶೂನ್ಯ sha512Block(uint64_t* h, ಧ್ರುವ uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ಧ್ರುವ uint64_t* p = chunk;

    // Initialize working variables to current hash value
    ಪ್ರತಿ (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    ಪ್ರತಿ (unsigned i = 0; i < 5; ++i) {
        ಪ್ರತಿ (unsigned j = 0; j < 16; ++j) {
            ಯದಿ (i == 0) {
                w[j] = *p++;
            } ಅನ್ಯಥಾ {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ಧ್ರುವ uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ಧ್ರುವ uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ಧ್ರುವ uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ಧ್ರುವ uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ಧ್ರುವ uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ಧ್ರುವ uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ಧ್ರುವ uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ಧ್ರುವ uint64_t temp2 = s0 + maj;

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
    ಪ್ರತಿ (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

ಶೂನ್ಯ VHashSha512::ಸೇರಿಸು(ಧ್ರುವ ಶೂನ್ಯ* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    ಪೂರ್ಣಾಙ್ಕ chunkLen;
    ಧ್ರುವ uint8_t* chunkp;
    ಯದಿ (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<ಧ್ರುವ uint8_t*>(datap);
    } ಅನ್ಯಥಾ {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<ಧ್ರುವ ವರ್ಣ*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<ಧ್ರುವ uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    ಪೂರ್ಣಾಙ್ಕ posBegin = 0;  // Position in buffer for start of this block
    ಪೂರ್ಣಾಙ್ಕ posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    ಯಾವತ್ (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<ಧ್ರುವ ವರ್ಣ*>(chunkp + posBegin), chunkLen - posEnd);
}

ಶೂನ್ಯ VHashSha512::ಸೇರಿಸು_File(ಧ್ರುವ string& filename) {
    ಸ್ಥಿರ constexpr size_t BUFFER_SIZE = 64 * 1024;

    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ fd = ::open(filename.c_str(), O_RDONLY);
    ಯದಿ (fd < 0) ಪ್ರತ್ಯಾಗಮ;

    std::array<ವರ್ಣ, BUFFER_SIZE + 1> buf;
    ಯಾವತ್ (ಧ್ರುವ ssize_t got = ::ಪಠ(fd, &buf, BUFFER_SIZE)) {
        ಯದಿ (got <= 0) ಭಙ್ಗ;
        ಸೇರಿಸು(&buf, got);
    }
    ::close(fd);
}

ಶೂನ್ಯ VHashSha512::finalize() {
    ಯದಿ (!m_final) {
        // Make sure no 128 byte blocks left
        ಸೇರಿಸು("");
        m_final = ಸತ್ಯ;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        ಪ್ರತಿ (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        ಯದಿ (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ದ್ವಿಮಾನ() {
    finalize();
    string result;
    result.reserve(64);
    ಪ್ರತಿ (size_t i = 0; i < 64; ++i) {
        result += static_cast<ವರ್ಣ>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

uint64_t VHashSha512::digestUInt64() {
    ಧ್ರುವ string& binhash = digest_ದ್ವಿಮಾನ();
    uint64_t result = 0;
    ಪ್ರತಿ (size_t byte = 0; byte < ಪರಿಮಾಣ(uint64_t); ++byte) {
        ಧ್ರುವ unsigned ವರ್ಣ c = binhash[byte];
        result = (result << 8) | c;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VHashSha512::digestHex() {
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ digits = "0123456789abcdef";
    ಧ್ರುವ string& binhash = digest_ದ್ವಿಮಾನ();
    string result;
    result.reserve(128);
    ಪ್ರತಿ (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    ಪ್ರತ್ಯಾಗಮ VString::base64Enc(digest_ದ್ವಿಮಾನ());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ಧ್ರುವ string& binhash = digest_ದ್ವಿಮಾನ();
    string result;
    result.reserve(84);
    ಪೂರ್ಣಾಙ್ಕ pos = 0;
    ಪ್ರತಿ (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<ಪೂರ್ಣಾಙ್ಕ>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        ಯದಿ (result.size() >= DIGEST_SYMBOL24_LENGTH) ಭಙ್ಗ;
    }
    // Any leftover bits don't matter for our purpose
    ಪ್ರತ್ಯಾಗಮ result;
}

ಶೂನ್ಯ VHashSha512::selfTestOne(ಧ್ರುವ string& data, ಧ್ರುವ string& data2, ಧ್ರುವ string& exp,
                              ಧ್ರುವ string& exp64, ಧ್ರುವ string& exp24) {
    VHashSha512 digest{data};
    ಯದಿ (data2 != "") digest.ಸೇರಿಸು(data2);
    ಯದಿ (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    ಯದಿ (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    ಯದಿ (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
}

ಶೂನ್ಯ VHashSha512::selfTest() {
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

string VName::dehash(ಧ್ರುವ string& in) {
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ VHSH[] = "__Vhsh";
    ಸ್ಥಿರ constexpr size_t VHSH_LEN = ಪರಿಮಾಣ(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    ಸ್ಥಿರ ಧ್ರುವ size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    ಪ್ರತಿ (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        ಧ್ರುವ string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ಧ್ರುವ auto ಹುಡುಕು_begin = std::begin(in) + last_dot_pos;
        ಧ್ರುವ auto ಹುಡುಕು_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        ಧ್ರುವ auto begin_vhsh
            = std::ಹುಡುಕು(ಹುಡುಕು_begin, ಹುಡುಕು_end, std::begin(VHSH), std::end(VHSH) - 1);
        ಯದಿ (begin_vhsh != ಹುಡುಕು_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ಧ್ರುವ auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, ಹುಡುಕು_end), VHSH_LEN);
            ಧ್ರುವ std::string vhsh{begin_vhsh, end_vhsh};
            ಧ್ರುವ auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            ಯದಿ (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{ಹುಡುಕು_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, ಹುಡುಕು_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        ಅನ್ಯಥಾ ಯದಿ (!dehashed.empty()) {
            dehashed += std::string{ಹುಡುಕು_begin, ಹುಡುಕು_end};
        }

        ಯದಿ (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            ಯದಿ (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } ಅನ್ಯಥಾ {
            last_dot_pos = string::npos;
        }
    }
    ಪ್ರತ್ಯಾಗಮ dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    ಯದಿ (m_name == "") ಪ್ರತ್ಯಾಗಮ "";
    ಯದಿ (m_hashed != "") ಪ್ರತ್ಯಾಗಮ m_hashed;  // Memoized
    ಯದಿ (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        ಪ್ರತ್ಯಾಗಮ m_hashed;
    }
    VHashSha512 hash{m_name};
    ಧ್ರುವ string suffix = "__Vhsh" + hash.digestSymbol24();
    ಯದಿ (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        ಯಾವತ್ (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } ಅನ್ಯಥಾ {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    ಪ್ರತ್ಯಾಗಮ m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ಧ್ರುವ string& s, ಧ್ರುವ string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ಧ್ರುವ size_t sLen = s.length();
    ಧ್ರುವ size_t tLen = t.length();
    ಯದಿ (sLen == 0) ಪ್ರತ್ಯಾಗಮ tLen;
    ಯದಿ (tLen == 0) ಪ್ರತ್ಯಾಗಮ sLen;
    ಯದಿ (sLen >= LENGTH_LIMIT) ಪ್ರತ್ಯಾಗಮ sLen;
    ಯದಿ (tLen >= LENGTH_LIMIT) ಪ್ರತ್ಯಾಗಮ tLen;

    ಸ್ಥಿರ std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    ಸ್ಥಿರ std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    ಸ್ಥಿರ std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    ಪ್ರತಿ (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    ಪ್ರತಿ (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        ಪ್ರತಿ (size_t j = 0; j < sLen; j++) {
            ಧ್ರುವ EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ಧ್ರುವ EditDistance deletion = s_v_next[j] + 1;
            ಧ್ರುವ EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ಧ್ರುವ EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            ಯದಿ (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ಧ್ರುವ EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        ಪ್ರತಿ (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    ಧ್ರುವ EditDistance result = s_v_next[sLen];
    ಪ್ರತ್ಯಾಗಮ result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    ಧ್ರುವ size_t max_length = std::max(goal_len, candidate_len);
    ಧ್ರುವ size_t min_length = std::min(goal_len, candidate_len);
    ಯದಿ (max_length <= 1) ಪ್ರತ್ಯಾಗಮ 0;
    ಯದಿ (max_length - min_length <= 1) ಪ್ರತ್ಯಾಗಮ std::max(max_length / 3, static_cast<size_t>(1));
    ಪ್ರತ್ಯಾಗಮ (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ಧ್ರುವ string& goal,
                                                      EditDistance& distancer) ಧ್ರುವ {
    Candidate best;
    ಧ್ರುವ size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    ಪ್ರತಿ (ಧ್ರುವ Candidate& candidate : m_candidates) {
        ಧ್ರುವ size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        ಧ್ರುವ EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        ಯದಿ (min_distance >= distancer) ಅನುವರ್ತ;  // Short-circuit if already better

        ಧ್ರುವ EditDistance cutoff = cutoffDistance(gLen, cLen);
        ಯದಿ (min_distance > cutoff) ಅನುವರ್ತ;  // Short-circuit if already too bad

        ಧ್ರುವ EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.ಸಂದೇಶ
                                      << "'");
        ಯದಿ (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    ಯದಿ (distancer == 0) ಪ್ರತ್ಯಾಗಮ Candidate{.str = best.str, .ಸಂದೇಶ = ""};
    ಪ್ರತ್ಯಾಗಮ best;
}

ಶೂನ್ಯ VSpellCheck::selfTestDistanceOne(ಧ್ರುವ string& a, ಧ್ರುವ string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

ಶೂನ್ಯ VSpellCheck::selfTestSuggestOne(ತಾರ್ಕಿಕ matches, ಧ್ರುವ string& c, ಧ್ರುವ string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.ತಳ್ಳು_Candidate(c);
    ಧ್ರುವ string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    ಯದಿ (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } ಅನ್ಯಥಾ {
        UASSERT_SELFTEST(got, "");
    }
}

ಶೂನ್ಯ VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(ಸತ್ಯ, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(ಸತ್ಯ, "abcdef", "acbdef", 1);
        selfTestSuggestOne(ಸತ್ಯ, "db", "dc", 1);
        selfTestSuggestOne(ಸತ್ಯ, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(ಅಸತ್ಯ, "x", "y", 1);
        selfTestSuggestOne(ಅಸತ್ಯ, "sqrt", "assert", 3);
    }
    {
        ಧ್ರುವ VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ತಳ್ಳು_Candidate("fred");
        speller.ತಳ್ಳು_Candidate("wilma");
        speller.ತಳ್ಳು_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        ಪ್ರತಿ (unsigned i = 0; i < ಅಂಕೆ_CANDIDATE_LIMIT; ++i) speller.ತಳ್ಳು_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), ಅಂಕೆ_CANDIDATE_LIMIT);
        speller.ತಳ್ಳು_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), ಅಂಕೆ_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
