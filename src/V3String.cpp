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

আকার_t VName::s_min_দৈর্ঘ্য = 32;
আকার_t VName::s_max_দৈর্ঘ্য = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
template <যৌক্তিক Even = সত্য>
স্থির যৌক্তিক wildMatchImpl(ধ্রুবক অক্ষর* s, ধ্রুবক অক্ষর* p) VL_PURE {
    জন্য (; *p; s++, p++) {
        যদি (*p != '*') {
            যদি (((*s) != (*p)) && *p != '?') ফেরত মিথ্যা;
        } অন্যথা {
            // Trailing star matches everything.
            যদি (!*++p) ফেরত সত্য;
            যখন (!wildMatchImpl<!Even>(s, p)) {
                যদি (*++s == '\0') ফেরত মিথ্যা;
            }
            ফেরত সত্য;
        }
    }
    ফেরত (*s == '\0');
}

যৌক্তিক VString::wildmatch(ধ্রুবক অক্ষর* s, ধ্রুবক অক্ষর* p) VL_PURE {
    যদি (*s == '\0') {
        যখন (*p == '*') ++p;
        ফেরত *p == '\0';
    }
    ফেরত wildMatchImpl(s, p);
}

যৌক্তিক VString::wildmatch(ধ্রুবক string& s, ধ্রুবক string& p) VL_PURE {
    ফেরত wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ধ্রুবক string& a, ধ্রুবক string& dot, ধ্রুবক string& b) {
    যদি (b == "") ফেরত a;
    যদি (a == "") ফেরত b;
    ফেরত a + dot + b;
}

string VString::downcase(ধ্রুবক string& str) VL_PURE {
    string ফলাফল = str;
    জন্য (অক্ষর& cr : ফলাফল) cr = std::tolower(cr);
    ফেরত ফলাফল;
}

string VString::upcase(ধ্রুবক string& str) VL_PURE {
    string ফলাফল = str;
    জন্য (অক্ষর& cr : ফলাফল) cr = std::toupper(cr);
    ফেরত ফলাফল;
}

string VString::quoteAny(ধ্রুবক string& str, অক্ষর tgt, অক্ষর esc) VL_PURE {
    string ফলাফল;
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (c == tgt) ফলাফল += esc;
        ফলাফল += c;
    }
    ফেরত ফলাফল;
}

string VString::dequotePercent(ধ্রুবক string& str) {
    string ফলাফল;
    অক্ষর last = '\0';
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (last == '%' && c == '%') {
            last = '\0';
        } অন্যথা {
            ফলাফল += c;
            last = c;
        }
    }
    ফেরত ফলাফল;
}

string VString::quoteStringLiteralForShell(ধ্রুবক string& str) {
    string ফলাফল;
    constexpr অক্ষর dquote = '"';
    constexpr অক্ষর escape = '\\';
    ফলাফল.ঠেলো_back(dquote);  // Start quoted string
    ফলাফল.ঠেলো_back(escape);
    ফলাফল.ঠেলো_back(dquote);  // "
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (c == dquote || c == escape) ফলাফল.ঠেলো_back(escape);
        ফলাফল.ঠেলো_back(c);
    }
    ফলাফল.ঠেলো_back(escape);
    ফলাফল.ঠেলো_back(dquote);  // "
    ফলাফল.ঠেলো_back(dquote);  // Terminate quoted string
    ফেরত ফলাফল;
}

string VString::escapeStringForPath(ধ্রুবক string& str) {
    যদি (str.find(R"(\\)") != string::npos)
        ফেরত str;  // if it has been escaped already, don't do it again
    যদি (str.find('/') != string::npos) ফেরত str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string ফলাফল;
    constexpr অক্ষর space = ' ';  // escape space like this `Program Files`
    constexpr অক্ষর escape = '\\';
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (c == space || c == escape) ফলাফল.ঠেলো_back(escape);
        ফলাফল.ঠেলো_back(c);
    }
    ফেরত ফলাফল;
}

স্থির পূর্ণসংখ্যা vl_decodexdigit(অক্ষর c) {
    ফেরত std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ধ্রুবক string& text, string& errOut) {
    যৌক্তিক quoted = মিথ্যা;
    string newtext;
    newtext.reserve(text.আকার_2());
    unsigned অক্ষর octal_val = 0;
    পূর্ণসংখ্যা octal_digits = 0;
    জন্য (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        যদি (quoted) {
            যদি (std::isdigit(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                যদি (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = মিথ্যা;
                }
            } অন্যথা {
                যদি (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = মিথ্যা;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    চালু;
                }
                quoted = মিথ্যা;
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
                        += static_cast<অক্ষর>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } অন্যথা যদি (std::isalnum(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    বিরতি;
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
    ফেরত newtext;
}

আকার_t VString::quotedEnd(ধ্রুবক string& str, আকার_t pos) VL_PURE {
    জন্য (আকার_t i = pos + 1; i < str.আকার_2(); ++i) {
        যদি (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } অন্যথা যদি (str[i] == '"') {
            ফেরত i + 1;
        }
    }
    ফেরত string::npos;
}

string VString::spaceUnprintable(ধ্রুবক string& str) VL_PURE {
    string ফলাফল;
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (std::isprint(c)) {
            ফলাফল += c;
        } অন্যথা {
            ফলাফল += ' ';
        }
    }
    ফেরত ফলাফল;
}

string VString::removeWhitespace(ধ্রুবক string& str) {
    string ফলাফল;
    ফলাফল.reserve(str.আকার_2());
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (!std::isspace(c)) ফলাফল += c;
    }
    ফেরত ফলাফল;
}

string VString::trimWhitespace(ধ্রুবক string& str) {
    string ফলাফল;
    ফলাফল.reserve(str.আকার_2());
    string add;
    যৌক্তিক newline = মিথ্যা;
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (newline && std::isspace(c)) চালু;
        যদি (c == '\n') {
            add = "\n";
            newline = সত্য;
            চালু;
        }
        যদি (std::isspace(c)) {
            add += c;
            চালু;
        }
        যদি (!add.empty()) {
            ফলাফল += add;
            newline = মিথ্যা;
            add.clear();
        }
        ফলাফল += c;
    }
    ফেরত ফলাফল;
}

যৌক্তিক VString::isIdentifier(ধ্রুবক string& str) {
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (!isIdentifierChar(c)) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}

যৌক্তিক VString::isWhitespace(ধ্রুবক string& str) {
    জন্য (ধ্রুবক অক্ষর c : str) {
        যদি (!std::isspace(c)) ফেরত মিথ্যা;
    }
    ফেরত সত্য;
}

string::আকার_type VString::leading_Whitespace_গুনতি(ধ্রুবক string& str) {
    string::আকার_type ফলাফল = 0;
    জন্য (ধ্রুবক অক্ষর c : str) {
        ++ফলাফল;
        যদি (!std::isspace(c)) বিরতি;
    }
    ফেরত ফলাফল;
}

দ্বিগুণ VString::parseDouble(ধ্রুবক string& str, যৌক্তিক* successp) {
    অক্ষর* ধ্রুবক strgp = নতুন অক্ষর[str.আকার_2() + 1];
    অক্ষর* dp = strgp;
    যদি (successp) *successp = সত্য;
    জন্য (ধ্রুবক অক্ষর* sp = str.c_str(); *sp; ++sp) {
        যদি (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    অক্ষর* endp = strgp;
    ধ্রুবক দ্বিগুণ d = strtod(strgp, &endp);
    ধ্রুবক আকার_t parsed_len = endp - strgp;
    যদি (parsed_len != std::strlen(strgp)) {
        যদি (successp) *successp = মিথ্যা;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    ফেরত d;
}

string VString::replaceSubstr(ধ্রুবক string& str, ধ্রুবক string& from, ধ্রুবক string& to) {
    string ফলাফল = str;
    ধ্রুবক আকার_t fromLen = from.আকার_2();
    ধ্রুবক আকার_t toLen = to.আকার_2();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    জন্য (আকার_t pos = 0; (pos = ফলাফল.find(from, pos)) != string::npos; pos += toLen) {
        ফলাফল.replace(pos, fromLen, to);
    }
    ফেরত ফলাফল;
}

string VString::replaceWord(ধ্রুবক string& str, ধ্রুবক string& from, ধ্রুবক string& to) {
    string ফলাফল = str;
    ধ্রুবক আকার_t len = from.আকার_2();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    জন্য (আকার_t pos = 0; (pos = ফলাফল.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        যদি (((pos > 0) && VString::isIdentifierChar(ফলাফল[pos - 1])) ||  //
            ((pos + len < ফলাফল.আকার_2()) && VString::isIdentifierChar(ফলাফল[pos + len]))) {
            চালু;
        }
        ফলাফল.replace(pos, len, to);
    }
    ফেরত ফলাফল;
}

std::deque<string> VString::split(ধ্রুবক string& str, অক্ষর delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    যখন (std::getline(is, token, delimiter)) results.ঠেলো_back(token);
    ফেরত results;
}

যৌক্তিক VString::endsWith(ধ্রুবক string& str, ধ্রুবক string& suffix) {
    যদি (str.দৈর্ঘ্য() < suffix.দৈর্ঘ্য()) ফেরত মিথ্যা;
    ফেরত str.compare(str.দৈর্ঘ্য() - suffix.দৈর্ঘ্য(), suffix.দৈর্ঘ্য(), suffix) == 0;
}
string VString::aOrAn(ধ্রুবক অক্ষর* word) {
    নির্বাচন (word[0]) {
    ক্ষেত্র '\0': ফেরত "";
    ক্ষেত্র 'a':
    ক্ষেত্র 'e':
    ক্ষেত্র 'i':
    ক্ষেত্র 'o':
    ক্ষেত্র 'u': ফেরত "an";
    পূর্বনির্ধারিত: ফেরত "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ধ্রুবক string& str) VL_PURE {
    ধ্রুবক অক্ষর* চাবি = str.c_str();
    ধ্রুবক আকার_t len = str.আকার_2();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr পূর্ণসংখ্যা r = 47;

    uint64_t h = seed ^ (len * m);

    ধ্রুবক uint64_t* তথ্য = reinterpret_cast<ধ্রুবক uint64_t*>(চাবি);
    ধ্রুবক uint64_t* end = তথ্য + (len / 8);

    যখন (তথ্য != end) {
        uint64_t k = *তথ্য++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ধ্রুবক unsigned অক্ষর* data2 = reinterpret_cast<ধ্রুবক unsigned অক্ষর*>(তথ্য);

    নির্বাচন (len & 7) {
    ক্ষেত্র 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    ক্ষেত্র 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    ক্ষেত্র 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    ক্ষেত্র 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    ক্ষেত্র 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    ক্ষেত্র 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    ক্ষেত্র 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    পূর্বনির্ধারিত:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    ফেরত h;
}

string VString::base64Enc(ধ্রুবক string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    আকার_t len = str.আকার_2();
    string ফলাফল;
    ফলাফল.reserve(len * 4 / 3 + 2);
    আকার_t pos = 0;
    যদি (len >= 3) {
        জন্য (; pos < len - 2; pos += 3) {
            ফলাফল += digits[((str[pos] >> 2) & 0x3f)];
            ফলাফল
                += digits[((str[pos] & 0x3) << 4) | (static_cast<পূর্ণসংখ্যা>(str[pos + 1] & 0xf0) >> 4)];
            ফলাফল += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<পূর্ণসংখ্যা>(str[pos + 2] & 0xc0) >> 6)];
            ফলাফল += digits[((str[pos + 2] & 0x3f))];
        }
    }
    যদি (len >= 2 && pos < len - 1) {  // Pad 2
        ফলাফল += digits[((str[pos] >> 2) & 0x3f)];
        ফলাফল += digits[((str[pos] & 0x3) << 4) | (static_cast<পূর্ণসংখ্যা>(str[pos + 1] & 0xf0) >> 4)];
        ফলাফল += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<পূর্ণসংখ্যা>(0) >> 6)];
        ফলাফল += '=';
    } অন্যথা যদি (pos < len) {  // Pad 1
        ফলাফল += digits[((str[pos] >> 2) & 0x3f)];
        ফলাফল += digits[((str[pos] & 0x3) << 4) | (static_cast<পূর্ণসংখ্যা>(0) >> 4)];
        ফলাফল += '=';
        ফলাফল += '=';
    }
    ফেরত ফলাফল;
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
    ফেরত lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
স্থির শূন্য sha512Block(uint64_t* h, ধ্রুবক uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ধ্রুবক uint64_t* p = chunk;

    // Initialize working variables to current hash value
    জন্য (unsigned i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    জন্য (unsigned i = 0; i < 5; ++i) {
        জন্য (unsigned j = 0; j < 16; ++j) {
            যদি (i == 0) {
                w[j] = *p++;
            } অন্যথা {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ধ্রুবক uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ধ্রুবক uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ধ্রুবক uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ধ্রুবক uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ধ্রুবক uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ধ্রুবক uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ধ্রুবক uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ধ্রুবক uint64_t temp2 = s0 + maj;

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
    জন্য (unsigned i = 0; i < 8; ++i) h[i] += ah[i];
}

শূন্য VHashSha512::ঢোকাও(ধ্রুবক শূন্য* datap, আকার_t দৈর্ঘ্য) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_দৈর্ঘ্য += দৈর্ঘ্য;

    string অস্থায়ী_তথ্য;
    পূর্ণসংখ্যা chunkLen;
    ধ্রুবক uint8_t* chunkp;
    যদি (m_remainder == "") {
        chunkLen = দৈর্ঘ্য;
        chunkp = static_cast<ধ্রুবক uint8_t*>(datap);
    } অন্যথা {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        অস্থায়ী_তথ্য = m_remainder + std::string{static_cast<ধ্রুবক অক্ষর*>(datap), দৈর্ঘ্য};
        chunkLen = অস্থায়ী_তথ্য.দৈর্ঘ্য();
        chunkp = reinterpret_cast<ধ্রুবক uint8_t*>(অস্থায়ী_তথ্য.তথ্য());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    পূর্ণসংখ্যা posBegin = 0;  // Position in buffer for start of this block
    পূর্ণসংখ্যা posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    যখন (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        জন্য (পূর্ণসংখ্যা roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<ধ্রুবক অক্ষর*>(chunkp + posBegin), chunkLen - posEnd);
}

শূন্য VHashSha512::ঢোকাও_File(ধ্রুবক string& filename) {
    স্থির constexpr আকার_t বাফার_আকার = 64 * 1024;

    ধ্রুবক পূর্ণসংখ্যা fd = ::open(filename.c_str(), O_RDONLY);
    যদি (fd < 0) ফেরত;

    std::array<অক্ষর, বাফার_আকার + 1> buf;
    যখন (ধ্রুবক ssize_t got = ::পঠ(fd, &buf, বাফার_আকার)) {
        যদি (got <= 0) বিরতি;
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
        জন্য (পূর্ণসংখ্যা i = 0; i < 16; ++i) w[i] = 0;
        আকার_t blockPos = 0;
        জন্য (; blockPos < m_remainder.দৈর্ঘ্য(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        যদি (m_remainder.দৈর্ঘ্য() >= 112) {
            sha512Block(m_inthash, w);
            জন্য (পূর্ণসংখ্যা i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_দৈর্ঘ্য << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_দ্বিমিক() {
    finalize();
    string ফলাফল;
    ফলাফল.reserve(64);
    জন্য (আকার_t i = 0; i < 64; ++i) {
        ফলাফল += static_cast<অক্ষর>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    ফেরত ফলাফল;
}

uint64_t VHashSha512::digestUInt64() {
    ধ্রুবক string& binhash = digest_দ্বিমিক();
    uint64_t ফলাফল = 0;
    জন্য (আকার_t byte = 0; byte < আকার(uint64_t); ++byte) {
        ধ্রুবক unsigned অক্ষর c = binhash[byte];
        ফলাফল = (ফলাফল << 8) | c;
    }
    ফেরত ফলাফল;
}

string VHashSha512::digestHex() {
    স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক digits = "0123456789abcdef";
    ধ্রুবক string& binhash = digest_দ্বিমিক();
    string ফলাফল;
    ফলাফল.reserve(128);
    জন্য (আকার_t byte = 0; byte < 64; ++byte) {
        ফলাফল += digits[(binhash[byte] >> 4) & 0xf];
        ফলাফল += digits[(binhash[byte] >> 0) & 0xf];
    }
    ফেরত ফলাফল;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    ফেরত VString::base64Enc(digest_দ্বিমিক());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ধ্রুবক string& binhash = digest_দ্বিমিক();
    string ফলাফল;
    ফলাফল.reserve(84);
    পূর্ণসংখ্যা pos = 0;
    জন্য (; pos < (512 / 8) - 2; pos += 3) {
        ফলাফল += digits[((binhash[pos] >> 2) & 0x3f)];
        ফলাফল += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<পূর্ণসংখ্যা>(binhash[pos + 1] & 0xf0) >> 4)];
        ফলাফল += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<পূর্ণসংখ্যা>(binhash[pos + 2] & 0xc0) >> 6)];
        ফলাফল += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        যদি (ফলাফল.আকার_2() >= DIGEST_SYMBOL24_দৈর্ঘ্য) বিরতি;
    }
    // Any leftover bits don't matter for our purpose
    ফেরত ফলাফল;
}

শূন্য VHashSha512::selfTestOne(ধ্রুবক string& তথ্য, ধ্রুবক string& data2, ধ্রুবক string& exp,
                              ধ্রুবক string& exp64, ধ্রুবক string& exp24) {
    VHashSha512 digest{তথ্য};
    যদি (data2 != "") digest.ঢোকাও(data2);
    যদি (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << তথ্য + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    যদি (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << তথ্য + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    যদি (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << তথ্য + data2 << "'\n"  // LCOV_EXCL_LINE
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

string VName::dehash(ধ্রুবক string& in) {
    স্থির constexpr ধ্রুবক অক্ষর VHSH[] = "__Vhsh";
    স্থির constexpr আকার_t VHSH_LEN = আকার(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_দৈর্ঘ্য;
    স্থির ধ্রুবক আকার_t DOT_LEN = std::strlen("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    জন্য (string::আকার_type last_dot_pos = 0; last_dot_pos < in.আকার_2();) {
        ধ্রুবক string::আকার_type পরবর্তী_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ধ্রুবক auto অনুসন্ধান_begin = std::begin(in) + last_dot_pos;
        ধ্রুবক auto অনুসন্ধান_end
            = পরবর্তী_dot_pos == string::npos ? std::end(in) : std::begin(in) + পরবর্তী_dot_pos;

        // Search for __Vhsh between the two dots.
        ধ্রুবক auto begin_vhsh
            = std::অনুসন্ধান(অনুসন্ধান_begin, অনুসন্ধান_end, std::begin(VHSH), std::end(VHSH) - 1);
        যদি (begin_vhsh != অনুসন্ধান_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ধ্রুবক auto end_vhsh
                = begin_vhsh + std::min<আকার_t>(std::distance(begin_vhsh, অনুসন্ধান_end), VHSH_LEN);
            ধ্রুবক std::string vhsh{begin_vhsh, end_vhsh};
            ধ্রুবক auto& it = s_dehashMap.find(vhsh);
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

        যদি (পরবর্তী_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            যদি (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = পরবর্তী_dot_pos + DOT_LEN;
        } অন্যথা {
            last_dot_pos = string::npos;
        }
    }
    ফেরত dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    যদি (m_name == "") ফেরত "";
    যদি (m_hashed != "") ফেরত m_hashed;  // Memoized
    যদি (s_max_দৈর্ঘ্য == 0 || m_name.দৈর্ঘ্য() < s_max_দৈর্ঘ্য) {
        m_hashed = m_name;
        ফেরত m_hashed;
    }
    VHashSha512 hash{m_name};
    ধ্রুবক string suffix = "__Vhsh" + hash.digestSymbol24();
    যদি (s_min_দৈর্ঘ্য < s_max_দৈর্ঘ্য) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        আকার_t pref_দৈর্ঘ্য = s_min_দৈর্ঘ্য;
        যখন (pref_দৈর্ঘ্য >= 1 && m_name[pref_দৈর্ঘ্য - 1] != '_') --pref_দৈর্ঘ্য;
        s_dehashMap[suffix] = m_name.substr(pref_দৈর্ঘ্য);
        m_hashed = m_name.substr(0, pref_দৈর্ঘ্য) + suffix;
    } অন্যথা {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    ফেরত m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ধ্রুবক string& s, ধ্রুবক string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ধ্রুবক আকার_t sLen = s.দৈর্ঘ্য();
    ধ্রুবক আকার_t tLen = t.দৈর্ঘ্য();
    যদি (sLen == 0) ফেরত tLen;
    যদি (tLen == 0) ফেরত sLen;
    যদি (sLen >= দৈর্ঘ্য_LIMIT) ফেরত sLen;
    যদি (tLen >= দৈর্ঘ্য_LIMIT) ফেরত tLen;

    স্থির std::array<EditDistance, দৈর্ঘ্য_LIMIT + 1> s_v_two_ago;
    স্থির std::array<EditDistance, দৈর্ঘ্য_LIMIT + 1> s_v_one_ago;
    স্থির std::array<EditDistance, দৈর্ঘ্য_LIMIT + 1> s_v_পরবর্তী;

    জন্য (আকার_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    জন্য (আকার_t i = 0; i < tLen; i++) {
        s_v_পরবর্তী[0] = i + 1;
        জন্য (আকার_t j = 0; j < sLen; j++) {
            ধ্রুবক EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ধ্রুবক EditDistance deletion = s_v_পরবর্তী[j] + 1;
            ধ্রুবক EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ধ্রুবক EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            যদি (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ধ্রুবক EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_পরবর্তী[j + 1] = cheapest;
        }
        জন্য (আকার_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_পরবর্তী[j];
        }
    }

    ধ্রুবক EditDistance ফলাফল = s_v_পরবর্তী[sLen];
    ফেরত ফলাফল;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(আকার_t goal_len, আকার_t candidate_len) {
    // Return max acceptable edit distance
    ধ্রুবক আকার_t max_দৈর্ঘ্য_2 = std::max(goal_len, candidate_len);
    ধ্রুবক আকার_t min_দৈর্ঘ্য = std::min(goal_len, candidate_len);
    যদি (max_দৈর্ঘ্য_2 <= 1) ফেরত 0;
    যদি (max_দৈর্ঘ্য_2 - min_দৈর্ঘ্য <= 1) ফেরত std::max(max_দৈর্ঘ্য_2 / 3, static_cast<আকার_t>(1));
    ফেরত (max_দৈর্ঘ্য_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ধ্রুবক string& goal,
                                                      EditDistance& distancer) ধ্রুবক {
    Candidate best;
    ধ্রুবক আকার_t gLen = goal.দৈর্ঘ্য();
    distancer = দৈর্ঘ্য_LIMIT * 10;
    জন্য (ধ্রুবক Candidate& candidate : m_candidates) {
        ধ্রুবক আকার_t cLen = candidate.str.দৈর্ঘ্য();

        // Min distance must be inserting/deleting to make lengths match
        ধ্রুবক EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        যদি (min_distance >= distancer) চালু;  // Short-circuit if already better

        ধ্রুবক EditDistance cutoff = cutoffDistance(gLen, cLen);
        যদি (min_distance > cutoff) চালু;  // Short-circuit if already too bad

        ধ্রুবক EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.বার্তা
                                      << "'");
        যদি (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    যদি (distancer == 0) ফেরত Candidate{.str = best.str, .বার্তা = ""};
    ফেরত best;
}

শূন্য VSpellCheck::selfTestDistanceOne(ধ্রুবক string& a, ধ্রুবক string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

শূন্য VSpellCheck::selfTestSuggestOne(যৌক্তিক matches, ধ্রুবক string& c, ধ্রুবক string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.ঠেলো_Candidate(c);
    ধ্রুবক string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
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
        selfTestSuggestOne(মিথ্যা, "x", "y", 1);
        selfTestSuggestOne(মিথ্যা, "sqrt", "assert", 3);
    }
    {
        ধ্রুবক VSpellCheck speller;
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
        জন্য (unsigned i = 0; i < অঙ্ক_CANDIDATE_LIMIT; ++i) speller.ঠেলো_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.আকার_2(), অঙ্ক_CANDIDATE_LIMIT);
        speller.ঠেলো_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.আকার_2(), অঙ্ক_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
