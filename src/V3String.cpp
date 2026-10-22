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

#जर defined(_WIN32) || defined(__MINGW32__)
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

माप_प्रकार VName::s_minLength = 32;
माप_प्रकार VName::s_maxLength = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
ढाँचा <तार्किक Even = सत्य>
स्थिर तार्किक wildMatchImpl(स्थिरांक अक्षर* s, स्थिरांक अक्षर* p) VL_PURE {
    साठी (; *p; s++, p++) {
        जर (*p != '*') {
            जर (((*s) != (*p)) && *p != '?') परत असत्य;
        } अन्यथा {
            // Trailing star matches everything.
            जर (!*++p) परत सत्य;
            पर्यंत (!wildMatchImpl<!Even>(s, p)) {
                जर (*++s == '\0') परत असत्य;
            }
            परत सत्य;
        }
    }
    परत (*s == '\0');
}

तार्किक VString::wildmatch(स्थिरांक अक्षर* s, स्थिरांक अक्षर* p) VL_PURE {
    जर (*s == '\0') {
        पर्यंत (*p == '*') ++p;
        परत *p == '\0';
    }
    परत wildMatchImpl(s, p);
}

तार्किक VString::wildmatch(स्थिरांक string& s, स्थिरांक string& p) VL_PURE {
    परत wildmatch(s.c_str(), p.c_str());
}

string VString::dot(स्थिरांक string& a, स्थिरांक string& dot, स्थिरांक string& b) {
    जर (b == "") परत a;
    जर (a == "") परत b;
    परत a + dot + b;
}

string VString::downcase(स्थिरांक string& str) VL_PURE {
    string result = str;
    साठी (अक्षर& cr : result) cr = std::छोटे(cr);
    परत result;
}

string VString::upcase(स्थिरांक string& str) VL_PURE {
    string result = str;
    साठी (अक्षर& cr : result) cr = std::बड़े(cr);
    परत result;
}

string VString::quoteAny(स्थिरांक string& str, अक्षर tgt, अक्षर esc) VL_PURE {
    string result;
    साठी (स्थिरांक अक्षर c : str) {
        जर (c == tgt) result += esc;
        result += c;
    }
    परत result;
}

string VString::dequotePercent(स्थिरांक string& str) {
    string result;
    अक्षर last = '\0';
    साठी (स्थिरांक अक्षर c : str) {
        जर (last == '%' && c == '%') {
            last = '\0';
        } अन्यथा {
            result += c;
            last = c;
        }
    }
    परत result;
}

string VString::quoteStringLiteralForShell(स्थिरांक string& str) {
    string result;
    constexpr अक्षर dquote = '"';
    constexpr अक्षर escape = '\\';
    result.ढकला_back(dquote);  // Start quoted string
    result.ढकला_back(escape);
    result.ढकला_back(dquote);  // "
    साठी (स्थिरांक अक्षर c : str) {
        जर (c == dquote || c == escape) result.ढकला_back(escape);
        result.ढकला_back(c);
    }
    result.ढकला_back(escape);
    result.ढकला_back(dquote);  // "
    result.ढकला_back(dquote);  // Terminate quoted string
    परत result;
}

string VString::escapeStringForPath(स्थिरांक string& str) {
    जर (str.find(R"(\\)") != string::npos)
        परत str;  // if it has been escaped already, don't do it again
    जर (str.find('/') != string::npos) परत str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr अक्षर space = ' ';  // escape space like this `Program Files`
    constexpr अक्षर escape = '\\';
    साठी (स्थिरांक अक्षर c : str) {
        जर (c == space || c == escape) result.ढकला_back(escape);
        result.ढकला_back(c);
    }
    परत result;
}

स्थिर पूर्णांक vl_decodexdigit(अक्षर c) {
    परत std::है_अंक(c) ? c - '0' : std::छोटे(c) - 'a' + 10;
}

string VString::unquoteSVString(स्थिरांक string& text, string& errOut) {
    तार्किक quoted = असत्य;
    string newtext;
    newtext.reserve(text.size());
    अचिन्हित अक्षर octal_val = 0;
    पूर्णांक octal_digits = 0;
    साठी (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        जर (quoted) {
            जर (std::है_अंक(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                जर (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = असत्य;
                }
            } अन्यथा {
                जर (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = असत्य;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    सुरूठेवा;
                }
                quoted = असत्य;
                जर (*cp == 'n') {
                    newtext += '\n';
                } अन्यथा जर (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } अन्यथा जर (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } अन्यथा जर (*cp == 'r') {
                    newtext += '\r';
                } अन्यथा जर (*cp == 't') {
                    newtext += '\t';
                } अन्यथा जर (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } अन्यथा जर (*cp == 'x' && std::है_षष्ठादशक(cp[1])
                           && std::है_षष्ठादशक(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<अक्षर>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } अन्यथा जर (std::है_अक्षर_अंक(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    थांबा;
                } अन्यथा {
                    newtext += *cp;
                }
            }
        } अन्यथा जर (*cp == '\\') {
            जर (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = सत्य;
            octal_digits = 0;
        } अन्यथा {
            newtext += *cp;
        }
    }
    परत newtext;
}

माप_प्रकार VString::quotedEnd(स्थिरांक string& str, माप_प्रकार pos) VL_PURE {
    साठी (माप_प्रकार i = pos + 1; i < str.size(); ++i) {
        जर (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } अन्यथा जर (str[i] == '"') {
            परत i + 1;
        }
    }
    परत string::npos;
}

string VString::spaceUnprintable(स्थिरांक string& str) VL_PURE {
    string result;
    साठी (स्थिरांक अक्षर c : str) {
        जर (std::है_छाप(c)) {
            result += c;
        } अन्यथा {
            result += ' ';
        }
    }
    परत result;
}

string VString::removeWhitespace(स्थिरांक string& str) {
    string result;
    result.reserve(str.size());
    साठी (स्थिरांक अक्षर c : str) {
        जर (!std::है_खाली(c)) result += c;
    }
    परत result;
}

string VString::trimWhitespace(स्थिरांक string& str) {
    string result;
    result.reserve(str.size());
    string add;
    तार्किक newline = असत्य;
    साठी (स्थिरांक अक्षर c : str) {
        जर (newline && std::है_खाली(c)) सुरूठेवा;
        जर (c == '\n') {
            add = "\n";
            newline = सत्य;
            सुरूठेवा;
        }
        जर (std::है_खाली(c)) {
            add += c;
            सुरूठेवा;
        }
        जर (!add.empty()) {
            result += add;
            newline = असत्य;
            add.clear();
        }
        result += c;
    }
    परत result;
}

तार्किक VString::isIdentifier(स्थिरांक string& str) {
    साठी (स्थिरांक अक्षर c : str) {
        जर (!isIdentifierChar(c)) परत असत्य;
    }
    परत सत्य;
}

तार्किक VString::isWhitespace(स्थिरांक string& str) {
    साठी (स्थिरांक अक्षर c : str) {
        जर (!std::है_खाली(c)) परत असत्य;
    }
    परत सत्य;
}

string::size_type VString::leading_Whitespace_मोजणी(स्थिरांक string& str) {
    string::size_type result = 0;
    साठी (स्थिरांक अक्षर c : str) {
        ++result;
        जर (!std::है_खाली(c)) थांबा;
    }
    परत result;
}

द्विगुण VString::parseDouble(स्थिरांक string& str, तार्किक* successp) {
    अक्षर* स्थिरांक strgp = नवीन अक्षर[str.size() + 1];
    अक्षर* dp = strgp;
    जर (successp) *successp = सत्य;
    साठी (स्थिरांक अक्षर* sp = str.c_str(); *sp; ++sp) {
        जर (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    अक्षर* endp = strgp;
    स्थिरांक द्विगुण d = म_से_भग्न(strgp, &endp);
    स्थिरांक माप_प्रकार parsed_len = endp - strgp;
    जर (parsed_len != std::म_माप(strgp)) {
        जर (successp) *successp = असत्य;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    परत d;
}

string VString::replaceSubstr(स्थिरांक string& str, स्थिरांक string& from, स्थिरांक string& to) {
    string result = str;
    स्थिरांक माप_प्रकार fromLen = from.size();
    स्थिरांक माप_प्रकार toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    साठी (माप_प्रकार pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    परत result;
}

string VString::replaceWord(स्थिरांक string& str, स्थिरांक string& from, स्थिरांक string& to) {
    string result = str;
    स्थिरांक माप_प्रकार len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    साठी (माप_प्रकार pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        जर (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            सुरूठेवा;
        }
        result.replace(pos, len, to);
    }
    परत result;
}

std::deque<string> VString::split(स्थिरांक string& str, अक्षर delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    पर्यंत (std::getline(is, token, delimiter)) results.ढकला_back(token);
    परत results;
}

तार्किक VString::endsWith(स्थिरांक string& str, स्थिरांक string& suffix) {
    जर (str.length() < suffix.length()) परत असत्य;
    परत str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(स्थिरांक अक्षर* word) {
    निवड (word[0]) {
    स्थिती '\0': परत "";
    स्थिती 'a':
    स्थिती 'e':
    स्थिती 'i':
    स्थिती 'o':
    स्थिती 'u': परत "an";
    पूर्वनिर्धारित: परत "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(स्थिरांक string& str) VL_PURE {
    स्थिरांक अक्षर* किल्ली = str.c_str();
    स्थिरांक माप_प्रकार len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr पूर्णांक r = 47;

    uint64_t h = seed ^ (len * m);

    स्थिरांक uint64_t* data = reinterpret_cast<स्थिरांक uint64_t*>(किल्ली);
    स्थिरांक uint64_t* end = data + (len / 8);

    पर्यंत (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    स्थिरांक अचिन्हित अक्षर* data2 = reinterpret_cast<स्थिरांक अचिन्हित अक्षर*>(data);

    निवड (len & 7) {
    स्थिती 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    स्थिती 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    स्थिती 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    स्थिती 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    स्थिती 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    स्थिती 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    स्थिती 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    पूर्वनिर्धारित:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    परत h;
}

string VString::base64Enc(स्थिरांक string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    स्थिर constexpr स्थिरांक अक्षर* स्थिरांक digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    माप_प्रकार len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    माप_प्रकार pos = 0;
    जर (len >= 3) {
        साठी (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णांक>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<पूर्णांक>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    जर (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णांक>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<पूर्णांक>(0) >> 6)];
        result += '=';
    } अन्यथा जर (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णांक>(0) >> 4)];
        result += '=';
        result += '=';
    }
    परत result;
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
    परत lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
स्थिर शून्य sha512Block(uint64_t* h, स्थिरांक uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    स्थिरांक uint64_t* p = chunk;

    // Initialize working variables to current hash value
    साठी (अचिन्हित i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    साठी (अचिन्हित i = 0; i < 5; ++i) {
        साठी (अचिन्हित j = 0; j < 16; ++j) {
            जर (i == 0) {
                w[j] = *p++;
            } अन्यथा {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                स्थिरांक uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                स्थिरांक uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            स्थिरांक uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            स्थिरांक uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            स्थिरांक uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            स्थिरांक uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            स्थिरांक uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            स्थिरांक uint64_t temp2 = s0 + maj;

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
    साठी (अचिन्हित i = 0; i < 8; ++i) h[i] += ah[i];
}

शून्य VHashSha512::घाला(स्थिरांक शून्य* datap, माप_प्रकार length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    पूर्णांक chunkLen;
    स्थिरांक uint8_t* chunkp;
    जर (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<स्थिरांक uint8_t*>(datap);
    } अन्यथा {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        tempData = m_remainder + std::string{static_cast<स्थिरांक अक्षर*>(datap), length};
        chunkLen = tempData.length();
        chunkp = reinterpret_cast<स्थिरांक uint8_t*>(tempData.data());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    पूर्णांक posBegin = 0;  // Position in buffer for start of this block
    पूर्णांक posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    पर्यंत (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        साठी (पूर्णांक roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<स्थिरांक अक्षर*>(chunkp + posBegin), chunkLen - posEnd);
}

शून्य VHashSha512::घाला_File(स्थिरांक string& filename) {
    स्थिर constexpr माप_प्रकार BUFFER_SIZE = 64 * 1024;

    स्थिरांक पूर्णांक fd = ::खोलो(filename.c_str(), O_RDONLY);
    जर (fd < 0) परत;

    std::array<अक्षर, BUFFER_SIZE + 1> buf;
    पर्यंत (स्थिरांक ssize_t got = ::पठ(fd, &buf, BUFFER_SIZE)) {
        जर (got <= 0) थांबा;
        घाला(&buf, got);
    }
    ::बंद(fd);
}

शून्य VHashSha512::finalize() {
    जर (!m_final) {
        // Make sure no 128 byte blocks left
        घाला("");
        m_final = सत्य;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        साठी (पूर्णांक i = 0; i < 16; ++i) w[i] = 0;
        माप_प्रकार blockPos = 0;
        साठी (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        जर (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            साठी (पूर्णांक i = 0; i < 16; ++i) w[i] = 0;
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
    साठी (माप_प्रकार i = 0; i < 64; ++i) {
        result += static_cast<अक्षर>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    परत result;
}

uint64_t VHashSha512::digestUInt64() {
    स्थिरांक string& binhash = digest_द्विमान();
    uint64_t result = 0;
    साठी (माप_प्रकार byte = 0; byte < आकार(uint64_t); ++byte) {
        स्थिरांक अचिन्हित अक्षर c = binhash[byte];
        result = (result << 8) | c;
    }
    परत result;
}

string VHashSha512::digestHex() {
    स्थिर constexpr स्थिरांक अक्षर* स्थिरांक digits = "0123456789abcdef";
    स्थिरांक string& binhash = digest_द्विमान();
    string result;
    result.reserve(128);
    साठी (माप_प्रकार byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    परत result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    परत VString::base64Enc(digest_द्विमान());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    स्थिर constexpr स्थिरांक अक्षर* स्थिरांक digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    स्थिरांक string& binhash = digest_द्विमान();
    string result;
    result.reserve(84);
    पूर्णांक pos = 0;
    साठी (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<पूर्णांक>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<पूर्णांक>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        जर (result.size() >= DIGEST_SYMBOL24_LENGTH) थांबा;
    }
    // Any leftover bits don't matter for our purpose
    परत result;
}

शून्य VHashSha512::selfTestOne(स्थिरांक string& data, स्थिरांक string& data2, स्थिरांक string& exp,
                              स्थिरांक string& exp64, स्थिरांक string& exp24) {
    VHashSha512 digest{data};
    जर (data2 != "") digest.घाला(data2);
    जर (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    जर (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    जर (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
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

string VName::dehash(स्थिरांक string& in) {
    स्थिर constexpr स्थिरांक अक्षर VHSH[] = "__Vhsh";
    स्थिर constexpr माप_प्रकार VHSH_LEN = आकार(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    स्थिर स्थिरांक माप_प्रकार DOT_LEN = std::म_माप("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    साठी (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        स्थिरांक string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        स्थिरांक स्वतः शोध_begin = std::begin(in) + last_dot_pos;
        स्थिरांक स्वतः शोध_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        स्थिरांक स्वतः begin_vhsh
            = std::शोध(शोध_begin, शोध_end, std::begin(VHSH), std::end(VHSH) - 1);
        जर (begin_vhsh != शोध_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            स्थिरांक स्वतः end_vhsh
                = begin_vhsh + std::min<माप_प्रकार>(std::distance(begin_vhsh, शोध_end), VHSH_LEN);
            स्थिरांक std::string vhsh{begin_vhsh, end_vhsh};
            स्थिरांक स्वतः& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            जर (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{शोध_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, शोध_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        अन्यथा जर (!dehashed.empty()) {
            dehashed += std::string{शोध_begin, शोध_end};
        }

        जर (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            जर (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } अन्यथा {
            last_dot_pos = string::npos;
        }
    }
    परत dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    जर (m_name == "") परत "";
    जर (m_hashed != "") परत m_hashed;  // Memoized
    जर (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        परत m_hashed;
    }
    VHashSha512 hash{m_name};
    स्थिरांक string suffix = "__Vhsh" + hash.digestSymbol24();
    जर (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        माप_प्रकार prefLength = s_minLength;
        पर्यंत (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } अन्यथा {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    परत m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(स्थिरांक string& s, स्थिरांक string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    स्थिरांक माप_प्रकार sLen = s.length();
    स्थिरांक माप_प्रकार tLen = t.length();
    जर (sLen == 0) परत tLen;
    जर (tLen == 0) परत sLen;
    जर (sLen >= LENGTH_LIMIT) परत sLen;
    जर (tLen >= LENGTH_LIMIT) परत tLen;

    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    साठी (माप_प्रकार i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    साठी (माप_प्रकार i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        साठी (माप_प्रकार j = 0; j < sLen; j++) {
            स्थिरांक EditDistance cost = (s[j] == t[i] ? 0 : 1);
            स्थिरांक EditDistance deletion = s_v_next[j] + 1;
            स्थिरांक EditDistance insertion = s_v_one_ago[j + 1] + 1;
            स्थिरांक EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            जर (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                स्थिरांक EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        साठी (माप_प्रकार j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    स्थिरांक EditDistance result = s_v_next[sLen];
    परत result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(माप_प्रकार goal_len, माप_प्रकार candidate_len) {
    // Return max acceptable edit distance
    स्थिरांक माप_प्रकार max_length = std::max(goal_len, candidate_len);
    स्थिरांक माप_प्रकार min_length = std::min(goal_len, candidate_len);
    जर (max_length <= 1) परत 0;
    जर (max_length - min_length <= 1) परत std::max(max_length / 3, static_cast<माप_प्रकार>(1));
    परत (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(स्थिरांक string& goal,
                                                      EditDistance& distancer) स्थिरांक {
    Candidate best;
    स्थिरांक माप_प्रकार gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    साठी (स्थिरांक Candidate& candidate : m_candidates) {
        स्थिरांक माप_प्रकार cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        स्थिरांक EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        जर (min_distance >= distancer) सुरूठेवा;  // Short-circuit if already better

        स्थिरांक EditDistance cutoff = cutoffDistance(gLen, cLen);
        जर (min_distance > cutoff) सुरूठेवा;  // Short-circuit if already too bad

        स्थिरांक EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.संदेश
                                      << "'");
        जर (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    जर (distancer == 0) परत Candidate{.str = best.str, .संदेश = ""};
    परत best;
}

शून्य VSpellCheck::selfTestDistanceOne(स्थिरांक string& a, स्थिरांक string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

शून्य VSpellCheck::selfTestSuggestOne(तार्किक matches, स्थिरांक string& c, स्थिरांक string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.ढकला_Candidate(c);
    स्थिरांक string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    जर (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } अन्यथा {
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
        selfTestSuggestOne(सत्य, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(सत्य, "abcdef", "acbdef", 1);
        selfTestSuggestOne(सत्य, "db", "dc", 1);
        selfTestSuggestOne(सत्य, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(असत्य, "x", "y", 1);
        selfTestSuggestOne(असत्य, "sqrt", "assert", 3);
    }
    {
        स्थिरांक VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ढकला_Candidate("fred");
        speller.ढकला_Candidate("wilma");
        speller.ढकला_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        साठी (अचिन्हित i = 0; i < अंक_CANDIDATE_LIMIT; ++i) speller.ढकला_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), अंक_CANDIDATE_LIMIT);
        speller.ढकला_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), अंक_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
