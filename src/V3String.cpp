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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
#അന്തര്ഭാവ <io.h>  // open, read, write, close
#endif

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3String.h"

#ifndef V3ERROR_NO_GLOBAL_
#അന്തര്ഭാവ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <fcntl.h>

size_t VName::s_minLength = 32;
size_t VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <തർക്കം Even = സത്യം>
സ്ഥിതം തർക്കം wildMatchImpl(സ്ഥിരം അക്ഷരം* s, സ്ഥിരം അക്ഷരം* p) VL_PURE {
    വേണ്ടി (; *p; s++, p++) {
        എങ്കിൽ (*p != '*') {
            എങ്കിൽ (((*s) != (*p)) && *p != '?') തിരികെ അസത്യം;
        } അല്ലെങ്കിൽ {
            // Trailing star matches everything.
            എങ്കിൽ (!*++p) തിരികെ സത്യം;
            ആകുമ്പോൾ (!wildMatchImpl<!Even>(s, p)) {
                എങ്കിൽ (*++s == '\0') തിരികെ അസത്യം;
            }
            തിരികെ സത്യം;
        }
    }
    തിരികെ (*s == '\0');
}

തർക്കം VString::wildmatch(സ്ഥിരം അക്ഷരം* s, സ്ഥിരം അക്ഷരം* p) VL_PURE {
    എങ്കിൽ (*s == '\0') {
        ആകുമ്പോൾ (*p == '*') ++p;
        തിരികെ *p == '\0';
    }
    തിരികെ wildMatchImpl(s, p);
}

തർക്കം VString::wildmatch(സ്ഥിരം string& s, സ്ഥിരം string& p) VL_PURE {
    തിരികെ wildmatch(s.c_str(), p.c_str());
}

string VString::dot(സ്ഥിരം string& a, സ്ഥിരം string& dot, സ്ഥിരം string& b) {
    എങ്കിൽ (b == "") തിരികെ a;
    എങ്കിൽ (a == "") തിരികെ b;
    തിരികെ a + dot + b;
}

string VString::downcase(സ്ഥിരം string& str) VL_PURE {
    string result = str;
    വേണ്ടി (അക്ഷരം& cr : result) cr = std::tolower(cr);
    തിരികെ result;
}

string VString::upcase(സ്ഥിരം string& str) VL_PURE {
    string result = str;
    വേണ്ടി (അക്ഷരം& cr : result) cr = std::toupper(cr);
    തിരികെ result;
}

string VString::quoteAny(സ്ഥിരം string& str, അക്ഷരം tgt, അക്ഷരം esc) VL_PURE {
    string result;
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (c == tgt) result += esc;
        result += c;
    }
    തിരികെ result;
}

string VString::dequotePercent(സ്ഥിരം string& str) {
    string result;
    അക്ഷരം last = '\0';
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (last == '%' && c == '%') {
            last = '\0';
        } അല്ലെങ്കിൽ {
            result += c;
            last = c;
        }
    }
    തിരികെ result;
}

string VString::quoteStringLiteralForShell(സ്ഥിരം string& str) {
    string result;
    constexpr അക്ഷരം dquote = '"';
    constexpr അക്ഷരം escape = '\\';
    result.തള്ളുക_back(dquote);  // Start quoted string
    result.തള്ളുക_back(escape);
    result.തള്ളുക_back(dquote);  // "
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (c == dquote || c == escape) result.തള്ളുക_back(escape);
        result.തള്ളുക_back(c);
    }
    result.തള്ളുക_back(escape);
    result.തള്ളുക_back(dquote);  // "
    result.തള്ളുക_back(dquote);  // Terminate quoted string
    തിരികെ result;
}

string VString::escapeStringForPath(സ്ഥിരം string& str) {
    എങ്കിൽ (str.find(R"(\\)") != string::npos)
        തിരികെ str;  // if it has been escaped already, don't do it again
    എങ്കിൽ (str.find('/') != string::npos) തിരികെ str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr അക്ഷരം space = ' ';  // escape space like this `Program Files`
    constexpr അക്ഷരം escape = '\\';
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (c == space || c == escape) result.തള്ളുക_back(escape);
        result.തള്ളുക_back(c);
    }
    തിരികെ result;
}

സ്ഥിതം പൂർണ്ണസംഖ്യ vl_decodexdigit(അക്ഷരം c) {
    തിരികെ std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(സ്ഥിരം string& text, string& errOut) {
    തർക്കം quoted = അസത്യം;
    string newtext;
    newtext.reserve(text.size());
    unsigned അക്ഷരം octal_val = 0;
    പൂർണ്ണസംഖ്യ octal_digits = 0;
    വേണ്ടി (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        എങ്കിൽ (quoted) {
            എങ്കിൽ (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                എങ്കിൽ (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = അസത്യം;
                }
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = അസത്യം;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    തുടരുക;
                }
                quoted = അസത്യം;
                എങ്കിൽ (*cp == 'n') {
                    newtext += '\n';
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == 'r') {
                    newtext += '\r';
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == 't') {
                    newtext += '\t';
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } അല്ലെങ്കിൽ എങ്കിൽ (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<അക്ഷരം>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } അല്ലെങ്കിൽ എങ്കിൽ (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    നിർത്തുക;
                } അല്ലെങ്കിൽ {
                    newtext += *cp;
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (*cp == '\\') {
            എങ്കിൽ (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = സത്യം;
            octal_digits = 0;
        } അല്ലെങ്കിൽ {
            newtext += *cp;
        }
    }
    തിരികെ newtext;
}

size_t VString::quotedEnd(സ്ഥിരം string& str, size_t pos) VL_PURE {
    വേണ്ടി (size_t i = pos + 1; i < str.size(); ++i) {
        എങ്കിൽ (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } അല്ലെങ്കിൽ എങ്കിൽ (str[i] == '"') {
            തിരികെ i + 1;
        }
    }
    തിരികെ string::npos;
}

string VString::spaceUnprintable(സ്ഥിരം string& str) VL_PURE {
    string result;
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (std::isprint(c)) {
            result += c;
        } അല്ലെങ്കിൽ {
            result += ' ';
        }
    }
    തിരികെ result;
}

string VString::removeWhitespace(സ്ഥിരം string& str) {
    string result;
    result.reserve(str.size());
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (!std::isspace(c)) result += c;
    }
    തിരികെ result;
}

string VString::trimWhitespace(സ്ഥിരം string& str) {
    string result;
    result.reserve(str.size());
    string add;
    തർക്കം newline = അസത്യം;
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (newline && std::isspace(c)) തുടരുക;
        എങ്കിൽ (c == '\n') {
            add = "\n";
            newline = സത്യം;
            തുടരുക;
        }
        എങ്കിൽ (std::isspace(c)) {
            add += c;
            തുടരുക;
        }
        എങ്കിൽ (!add.empty()) {
            result += add;
            newline = അസത്യം;
            add.clear();
        }
        result += c;
    }
    തിരികെ result;
}

തർക്കം VString::isIdentifier(സ്ഥിരം string& str) {
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (!isIdentifierChar(c)) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}

തർക്കം VString::isWhitespace(സ്ഥിരം string& str) {
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        എങ്കിൽ (!std::isspace(c)) തിരികെ അസത്യം;
    }
    തിരികെ സത്യം;
}

string::size_type VString::leading_Whitespace_എണ്ണം(സ്ഥിരം string& str) {
    string::size_type result = 0;
    വേണ്ടി (സ്ഥിരം അക്ഷരം c : str) {
        ++result;
        എങ്കിൽ (!std::isspace(c)) നിർത്തുക;
    }
    തിരികെ result;
}

ഇരട്ട VString::parseDouble(സ്ഥിരം string& str, തർക്കം* successp) {
    അക്ഷരം* സ്ഥിരം strgp = പുതിയ അക്ഷരം[str.size() + 1];
    അക്ഷരം* dp = strgp;
    എങ്കിൽ (successp) *successp = സത്യം;
    വേണ്ടി (സ്ഥിരം അക്ഷരം* sp = str.c_str(); *sp; ++sp) {
        എങ്കിൽ (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    അക്ഷരം* endp = strgp;
    സ്ഥിരം ഇരട്ട d = strtod(strgp, &endp);
    സ്ഥിരം size_t parsed_len = endp - strgp;
    എങ്കിൽ (parsed_len != std::strlen(strgp)) {
        എങ്കിൽ (successp) *successp = അസത്യം;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    തിരികെ d;
}

string VString::replaceSubstr(സ്ഥിരം string& str, സ്ഥിരം string& from, സ്ഥിരം string& to) {
    string result = str;
    സ്ഥിരം size_t fromLen = from.size();
    സ്ഥിരം size_t toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    വേണ്ടി (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    തിരികെ result;
}

string VString::replaceWord(സ്ഥിരം string& str, സ്ഥിരം string& from, സ്ഥിരം string& to) {
    string result = str;
    സ്ഥിരം size_t len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    വേണ്ടി (size_t pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        എങ്കിൽ (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            തുടരുക;
        }
        result.replace(pos, len, to);
    }
    തിരികെ result;
}

std::deque<string> VString::split(സ്ഥിരം string& str, അക്ഷരം delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    ആകുമ്പോൾ (std::getline(is, token, delimiter)) results.തള്ളുക_back(token);
    തിരികെ results;
}

തർക്കം VString::endsWith(സ്ഥിരം string& str, സ്ഥിരം string& suffix) {
    എങ്കിൽ (str.length() < suffix.length()) തിരികെ അസത്യം;
    തിരികെ str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(സ്ഥിരം അക്ഷരം* word) {
    തിരഞ്ഞെടുപ്പ് (word[0]) {
    അവസ്ഥ '\0': തിരികെ "";
    അവസ്ഥ 'a':
    അവസ്ഥ 'e':
    അവസ്ഥ 'i':
    അവസ്ഥ 'o':
    അവസ്ഥ 'u': തിരികെ "an";
    സ്ഥിരസ്ഥിതി: തിരികെ "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(സ്ഥിരം string& str) VL_PURE {
    സ്ഥിരം അക്ഷരം* താക്കോൽ = str.c_str();
    സ്ഥിരം size_t len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr പൂർണ്ണസംഖ്യ r = 47;

    uint64_t h = seed ^ (len * m);

    സ്ഥിരം uint64_t* data = reinterpret_cast<സ്ഥിരം uint64_t*>(താക്കോൽ);
    സ്ഥിരം uint64_t* end = data + (len / 8);

    ആകുമ്പോൾ (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    സ്ഥിരം unsigned അക്ഷരം* data2 = reinterpret_cast<സ്ഥിരം unsigned അക്ഷരം*>(data);

    തിരഞ്ഞെടുപ്പ് (len & 7) {
    അവസ്ഥ 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    അവസ്ഥ 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    അവസ്ഥ 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    അവസ്ഥ 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    അവസ്ഥ 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    അവസ്ഥ 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    അവസ്ഥ 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    സ്ഥിരസ്ഥിതി:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    തിരികെ h;
}

string VString::base64Enc(സ്ഥിരം string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    size_t pos = 0;
    എങ്കിൽ (len >= 3) {
        വേണ്ടി (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<പൂർണ്ണസംഖ്യ>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<പൂർണ്ണസംഖ്യ>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    എങ്കിൽ (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<പൂർണ്ണസംഖ്യ>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<പൂർണ്ണസംഖ്യ>(0) >> 6)];
        result += '=';
    } അല്ലെങ്കിൽ എങ്കിൽ (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<പൂർണ്ണസംഖ്യ>(0) >> 4)];
        result += '=';
        result += '=';
    }
    തിരികെ result;
}

ശൂന്യം VString::selfTest() {
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

സ്ഥിതം constexpr uint64_t sha512K[]
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
സ്ഥിതം uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    തിരികെ lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
സ്ഥിതം ശൂന്യം sha512Block(uint64_t* h, സ്ഥിരം uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    സ്ഥിരം uint64_t* p = chunk;

    // Initialize working variables to current hash value
    വേണ്ടി (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    വേണ്ടി (unsigned i = 0; i < 5; ++i) {
        വേണ്ടി (unsigned j = 0; j < 16; ++j) {
            എങ്കിൽ (i == 0) {
                w[j] = *p++;
            } അല്ലെങ്കിൽ {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                സ്ഥിരം uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                സ്ഥിരം uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            സ്ഥിരം uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            സ്ഥിരം uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            സ്ഥിരം uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            സ്ഥിരം uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            സ്ഥിരം uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            സ്ഥിരം uint64_t temp2 = s0 + maj;

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
    വേണ്ടി (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

ശൂന്യം VHashSha512::തിരുകുക(സ്ഥിരം ശൂന്യം* datap, size_t length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    പൂർണ്ണസംഖ്യ chunkLen;
    സ്ഥിരം uint8_t* chunkp;
    എങ്കിൽ (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<സ്ഥിരം uint8_t*>(datap);
    } അല്ലെങ്കിൽ {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<സ്ഥിരം അക്ഷരം*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<സ്ഥിരം uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    പൂർണ്ണസംഖ്യ posBegin = 0;  // Position in buffer for start of this block
    പൂർണ്ണസംഖ്യ posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    ആകുമ്പോൾ (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        വേണ്ടി (പൂർണ്ണസംഖ്യ roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<സ്ഥിരം അക്ഷരം*>(chunkp + posBegin), chunkLen - posEnd);
}

ശൂന്യം VHashSha512::തിരുകുക_File(സ്ഥിരം string& filename) {
    സ്ഥിതം constexpr size_t BUFFER_SIZE = 64 * 1024;

    സ്ഥിരം പൂർണ്ണസംഖ്യ fd = ::open(filename.c_str(), O_RDONLY);
    എങ്കിൽ (fd < 0) തിരികെ;

    std::array<അക്ഷരം, BUFFER_SIZE + 1> buf;
    ആകുമ്പോൾ (സ്ഥിരം ssize_t got = ::പഠ(fd, &buf, BUFFER_SIZE)) {
        എങ്കിൽ (got <= 0) നിർത്തുക;
        തിരുകുക(&buf, got);
    }
    ::close(fd);
}

ശൂന്യം VHashSha512::finalize() {
    എങ്കിൽ (!m_final) {
        // Make sure no 128 byte blocks left
        തിരുകുക("");
        m_final = സത്യം;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < 16; ++i) w[i] = 0;
        size_t blockPos = 0;
        വേണ്ടി (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        എങ്കിൽ (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ദ്വിമാന() {
    finalize();
    string result;
    result.reserve(64);
    വേണ്ടി (size_t i = 0; i < 64; ++i) {
        result += static_cast<അക്ഷരം>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    തിരികെ result;
}

uint64_t VHashSha512::digestUInt64() {
    സ്ഥിരം string& binhash = digest_ദ്വിമാന();
    uint64_t result = 0;
    വേണ്ടി (size_t byte = 0; byte < വലുപ്പം(uint64_t); ++byte) {
        സ്ഥിരം unsigned അക്ഷരം c = binhash[byte];
        result = (result << 8) | c;
    }
    തിരികെ result;
}

string VHashSha512::digestHex() {
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം digits = "0123456789abcdef";
    സ്ഥിരം string& binhash = digest_ദ്വിമാന();
    string result;
    result.reserve(128);
    വേണ്ടി (size_t byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    തിരികെ result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    തിരികെ VString::base64Enc(digest_ദ്വിമാന());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    സ്ഥിരം string& binhash = digest_ദ്വിമാന();
    string result;
    result.reserve(84);
    പൂർണ്ണസംഖ്യ pos = 0;
    വേണ്ടി (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<പൂർണ്ണസംഖ്യ>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<പൂർണ്ണസംഖ്യ>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        എങ്കിൽ (result.size() >= DIGEST_SYMBOL24_LENGTH) നിർത്തുക;
    }
    // Any leftover bits don't matter for our purpose
    തിരികെ result;
}

ശൂന്യം VHashSha512::selfTestOne(സ്ഥിരം string& data, സ്ഥിരം string& data2, സ്ഥിരം string& exp,
                              സ്ഥിരം string& exp64, സ്ഥിരം string& exp24) {
    VHashSha512 digest{data};
    എങ്കിൽ (data2 != "") digest.തിരുകുക(data2);
    എങ്കിൽ (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    എങ്കിൽ (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    എങ്കിൽ (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
}

ശൂന്യം VHashSha512::selfTest() {
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

string VName::dehash(സ്ഥിരം string& in) {
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം VHSH[] = "__Vhsh";
    സ്ഥിതം constexpr size_t VHSH_LEN = വലുപ്പം(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    സ്ഥിതം സ്ഥിരം size_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    വേണ്ടി (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        സ്ഥിരം string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        സ്ഥിരം auto തിരയൽ_begin = std::begin(in) + last_dot_pos;
        സ്ഥിരം auto തിരയൽ_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        സ്ഥിരം auto begin_vhsh
            = std::തിരയൽ(തിരയൽ_begin, തിരയൽ_end, std::begin(VHSH), std::end(VHSH) - 1);
        എങ്കിൽ (begin_vhsh != തിരയൽ_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            സ്ഥിരം auto end_vhsh
                = begin_vhsh + std::min<size_t>(std::distance(begin_vhsh, തിരയൽ_end), VHSH_LEN);
            സ്ഥിരം std::string vhsh{begin_vhsh, end_vhsh};
            സ്ഥിരം auto& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            എങ്കിൽ (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{തിരയൽ_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, തിരയൽ_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        അല്ലെങ്കിൽ എങ്കിൽ (!dehashed.empty()) {
            dehashed += std::string{തിരയൽ_begin, തിരയൽ_end};
        }

        എങ്കിൽ (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            എങ്കിൽ (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } അല്ലെങ്കിൽ {
            last_dot_pos = string::npos;
        }
    }
    തിരികെ dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    എങ്കിൽ (m_name == "") തിരികെ "";
    എങ്കിൽ (m_hashed != "") തിരികെ m_hashed;  // Memoized
    എങ്കിൽ (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        തിരികെ m_hashed;
    }
    VHashSha512 hash{m_name};
    സ്ഥിരം string suffix = "__Vhsh" + hash.digestSymbol24();
    എങ്കിൽ (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        size_t prefLength = s_minLength;
        ആകുമ്പോൾ (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } അല്ലെങ്കിൽ {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    തിരികെ m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(സ്ഥിരം string& s, സ്ഥിരം string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    സ്ഥിരം size_t sLen = s.length();
    സ്ഥിരം size_t tLen = t.length();
    എങ്കിൽ (sLen == 0) തിരികെ tLen;
    എങ്കിൽ (tLen == 0) തിരികെ sLen;
    എങ്കിൽ (sLen >= LENGTH_LIMIT) തിരികെ sLen;
    എങ്കിൽ (tLen >= LENGTH_LIMIT) തിരികെ tLen;

    സ്ഥിതം std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    സ്ഥിതം std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    സ്ഥിതം std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    വേണ്ടി (size_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    വേണ്ടി (size_t i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        വേണ്ടി (size_t j = 0; j < sLen; j++) {
            സ്ഥിരം EditDistance cost = (s[j] == t[i] ? 0 : 1);
            സ്ഥിരം EditDistance deletion = s_v_next[j] + 1;
            സ്ഥിരം EditDistance insertion = s_v_one_ago[j + 1] + 1;
            സ്ഥിരം EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            എങ്കിൽ (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                സ്ഥിരം EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        വേണ്ടി (size_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    സ്ഥിരം EditDistance result = s_v_next[sLen];
    തിരികെ result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(size_t goal_len, size_t candidate_len) {
    // Return max acceptable edit distance
    സ്ഥിരം size_t max_length = std::max(goal_len, candidate_len);
    സ്ഥിരം size_t min_length = std::min(goal_len, candidate_len);
    എങ്കിൽ (max_length <= 1) തിരികെ 0;
    എങ്കിൽ (max_length - min_length <= 1) തിരികെ std::max(max_length / 3, static_cast<size_t>(1));
    തിരികെ (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(സ്ഥിരം string& goal,
                                                      EditDistance& distancer) സ്ഥിരം {
    Candidate best;
    സ്ഥിരം size_t gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    വേണ്ടി (സ്ഥിരം Candidate& candidate : m_candidates) {
        സ്ഥിരം size_t cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        സ്ഥിരം EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        എങ്കിൽ (min_distance >= distancer) തുടരുക;  // Short-circuit if already better

        സ്ഥിരം EditDistance cutoff = cutoffDistance(gLen, cLen);
        എങ്കിൽ (min_distance > cutoff) തുടരുക;  // Short-circuit if already too bad

        സ്ഥിരം EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.സന്ദേശം
                                      << "'");
        എങ്കിൽ (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    എങ്കിൽ (distancer == 0) തിരികെ Candidate{.str = best.str, .സന്ദേശം = ""};
    തിരികെ best;
}

ശൂന്യം VSpellCheck::selfTestDistanceOne(സ്ഥിരം string& a, സ്ഥിരം string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

ശൂന്യം VSpellCheck::selfTestSuggestOne(തർക്കം matches, സ്ഥിരം string& c, സ്ഥിരം string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.തള്ളുക_Candidate(c);
    സ്ഥിരം string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    എങ്കിൽ (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } അല്ലെങ്കിൽ {
        UASSERT_SELFTEST(got, "");
    }
}

ശൂന്യം VSpellCheck::selfTest() {
    {
        selfTestDistanceOne("ab", "ac", 1);
        selfTestDistanceOne("ab", "a", 1);
        selfTestDistanceOne("a", "b", 1);
    }
    {
        selfTestSuggestOne(സത്യം, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(സത്യം, "abcdef", "acbdef", 1);
        selfTestSuggestOne(സത്യം, "db", "dc", 1);
        selfTestSuggestOne(സത്യം, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(അസത്യം, "x", "y", 1);
        selfTestSuggestOne(അസത്യം, "sqrt", "assert", 3);
    }
    {
        സ്ഥിരം VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.തള്ളുക_Candidate("fred");
        speller.തള്ളുക_Candidate("wilma");
        speller.തള്ളുക_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        വേണ്ടി (unsigned i = 0; i < അക്കം_CANDIDATE_LIMIT; ++i) speller.തള്ളുക_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), അക്കം_CANDIDATE_LIMIT);
        speller.തള്ളുക_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), അക്കം_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
