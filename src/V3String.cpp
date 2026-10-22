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

#यदि defined(_WIN32) || defined(__MINGW32__)
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
स्थिर तार्किक wildMatchImpl(ध्रुव वर्ण* s, ध्रुव वर्ण* p) VL_PURE {
    प्रति (; *p; s++, p++) {
        यदि (*p != '*') {
            यदि (((*s) != (*p)) && *p != '?') प्रत्यागम असत्य;
        } अन्यथा {
            // Trailing star matches everything.
            यदि (!*++p) प्रत्यागम सत्य;
            यावत् (!wildMatchImpl<!Even>(s, p)) {
                यदि (*++s == '\0') प्रत्यागम असत्य;
            }
            प्रत्यागम सत्य;
        }
    }
    प्रत्यागम (*s == '\0');
}

तार्किक VString::wildmatch(ध्रुव वर्ण* s, ध्रुव वर्ण* p) VL_PURE {
    यदि (*s == '\0') {
        यावत् (*p == '*') ++p;
        प्रत्यागम *p == '\0';
    }
    प्रत्यागम wildMatchImpl(s, p);
}

तार्किक VString::wildmatch(ध्रुव string& s, ध्रुव string& p) VL_PURE {
    प्रत्यागम wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ध्रुव string& a, ध्रुव string& dot, ध्रुव string& b) {
    यदि (b == "") प्रत्यागम a;
    यदि (a == "") प्रत्यागम b;
    प्रत्यागम a + dot + b;
}

string VString::downcase(ध्रुव string& str) VL_PURE {
    string result = str;
    प्रति (वर्ण& cr : result) cr = std::छोटे(cr);
    प्रत्यागम result;
}

string VString::upcase(ध्रुव string& str) VL_PURE {
    string result = str;
    प्रति (वर्ण& cr : result) cr = std::बड़े(cr);
    प्रत्यागम result;
}

string VString::quoteAny(ध्रुव string& str, वर्ण tgt, वर्ण esc) VL_PURE {
    string result;
    प्रति (ध्रुव वर्ण c : str) {
        यदि (c == tgt) result += esc;
        result += c;
    }
    प्रत्यागम result;
}

string VString::dequotePercent(ध्रुव string& str) {
    string result;
    वर्ण last = '\0';
    प्रति (ध्रुव वर्ण c : str) {
        यदि (last == '%' && c == '%') {
            last = '\0';
        } अन्यथा {
            result += c;
            last = c;
        }
    }
    प्रत्यागम result;
}

string VString::quoteStringLiteralForShell(ध्रुव string& str) {
    string result;
    constexpr वर्ण dquote = '"';
    constexpr वर्ण escape = '\\';
    result.धकेलो_back(dquote);  // Start quoted string
    result.धकेलो_back(escape);
    result.धकेलो_back(dquote);  // "
    प्रति (ध्रुव वर्ण c : str) {
        यदि (c == dquote || c == escape) result.धकेलो_back(escape);
        result.धकेलो_back(c);
    }
    result.धकेलो_back(escape);
    result.धकेलो_back(dquote);  // "
    result.धकेलो_back(dquote);  // Terminate quoted string
    प्रत्यागम result;
}

string VString::escapeStringForPath(ध्रुव string& str) {
    यदि (str.find(R"(\\)") != string::npos)
        प्रत्यागम str;  // if it has been escaped already, don't do it again
    यदि (str.find('/') != string::npos) प्रत्यागम str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string result;
    constexpr वर्ण space = ' ';  // escape space like this `Program Files`
    constexpr वर्ण escape = '\\';
    प्रति (ध्रुव वर्ण c : str) {
        यदि (c == space || c == escape) result.धकेलो_back(escape);
        result.धकेलो_back(c);
    }
    प्रत्यागम result;
}

स्थिर पूर्णाङ्क vl_decodexdigit(वर्ण c) {
    प्रत्यागम std::है_अंक(c) ? c - '0' : std::छोटे(c) - 'a' + 10;
}

string VString::unquoteSVString(ध्रुव string& text, string& errOut) {
    तार्किक quoted = असत्य;
    string newtext;
    newtext.reserve(text.size());
    अचिन्हित वर्ण octal_val = 0;
    पूर्णाङ्क octal_digits = 0;
    प्रति (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        यदि (quoted) {
            यदि (std::है_अंक(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                यदि (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = असत्य;
                }
            } अन्यथा {
                यदि (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = असत्य;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    अनुवर्त;
                }
                quoted = असत्य;
                यदि (*cp == 'n') {
                    newtext += '\n';
                } अन्यथा यदि (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } अन्यथा यदि (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } अन्यथा यदि (*cp == 'r') {
                    newtext += '\r';
                } अन्यथा यदि (*cp == 't') {
                    newtext += '\t';
                } अन्यथा यदि (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } अन्यथा यदि (*cp == 'x' && std::है_षष्ठादशक(cp[1])
                           && std::है_षष्ठादशक(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<वर्ण>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } अन्यथा यदि (std::है_अक्षर_अंक(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    भङ्ग;
                } अन्यथा {
                    newtext += *cp;
                }
            }
        } अन्यथा यदि (*cp == '\\') {
            यदि (octal_digits) {
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
    प्रत्यागम newtext;
}

माप_प्रकार VString::quotedEnd(ध्रुव string& str, माप_प्रकार pos) VL_PURE {
    प्रति (माप_प्रकार i = pos + 1; i < str.size(); ++i) {
        यदि (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } अन्यथा यदि (str[i] == '"') {
            प्रत्यागम i + 1;
        }
    }
    प्रत्यागम string::npos;
}

string VString::spaceUnprintable(ध्रुव string& str) VL_PURE {
    string result;
    प्रति (ध्रुव वर्ण c : str) {
        यदि (std::है_छाप(c)) {
            result += c;
        } अन्यथा {
            result += ' ';
        }
    }
    प्रत्यागम result;
}

string VString::removeWhitespace(ध्रुव string& str) {
    string result;
    result.reserve(str.size());
    प्रति (ध्रुव वर्ण c : str) {
        यदि (!std::है_खाली(c)) result += c;
    }
    प्रत्यागम result;
}

string VString::trimWhitespace(ध्रुव string& str) {
    string result;
    result.reserve(str.size());
    string add;
    तार्किक newline = असत्य;
    प्रति (ध्रुव वर्ण c : str) {
        यदि (newline && std::है_खाली(c)) अनुवर्त;
        यदि (c == '\n') {
            add = "\n";
            newline = सत्य;
            अनुवर्त;
        }
        यदि (std::है_खाली(c)) {
            add += c;
            अनुवर्त;
        }
        यदि (!add.empty()) {
            result += add;
            newline = असत्य;
            add.clear();
        }
        result += c;
    }
    प्रत्यागम result;
}

तार्किक VString::isIdentifier(ध्रुव string& str) {
    प्रति (ध्रुव वर्ण c : str) {
        यदि (!isIdentifierChar(c)) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

तार्किक VString::isWhitespace(ध्रुव string& str) {
    प्रति (ध्रुव वर्ण c : str) {
        यदि (!std::है_खाली(c)) प्रत्यागम असत्य;
    }
    प्रत्यागम सत्य;
}

string::size_type VString::leading_Whitespace_गिनती(ध्रुव string& str) {
    string::size_type result = 0;
    प्रति (ध्रुव वर्ण c : str) {
        ++result;
        यदि (!std::है_खाली(c)) भङ्ग;
    }
    प्रत्यागम result;
}

द्विगुण VString::parseDouble(ध्रुव string& str, तार्किक* successp) {
    वर्ण* ध्रुव strgp = नूतन वर्ण[str.size() + 1];
    वर्ण* dp = strgp;
    यदि (successp) *successp = सत्य;
    प्रति (ध्रुव वर्ण* sp = str.c_str(); *sp; ++sp) {
        यदि (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    वर्ण* endp = strgp;
    ध्रुव द्विगुण d = म_से_भग्न(strgp, &endp);
    ध्रुव माप_प्रकार parsed_len = endp - strgp;
    यदि (parsed_len != std::म_माप(strgp)) {
        यदि (successp) *successp = असत्य;
    }
    VL_DO_DANGLING(delete[] strgp, strgp);
    प्रत्यागम d;
}

string VString::replaceSubstr(ध्रुव string& str, ध्रुव string& from, ध्रुव string& to) {
    string result = str;
    ध्रुव माप_प्रकार fromLen = from.size();
    ध्रुव माप_प्रकार toLen = to.size();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    प्रति (माप_प्रकार pos = 0; (pos = result.find(from, pos)) != string::npos; pos += toLen) {
        result.replace(pos, fromLen, to);
    }
    प्रत्यागम result;
}

string VString::replaceWord(ध्रुव string& str, ध्रुव string& from, ध्रुव string& to) {
    string result = str;
    ध्रुव माप_प्रकार len = from.size();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    प्रति (माप_प्रकार pos = 0; (pos = result.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        यदि (((pos > 0) && VString::isIdentifierChar(result[pos - 1])) ||  //
            ((pos + len < result.size()) && VString::isIdentifierChar(result[pos + len]))) {
            अनुवर्त;
        }
        result.replace(pos, len, to);
    }
    प्रत्यागम result;
}

std::deque<string> VString::split(ध्रुव string& str, वर्ण delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    यावत् (std::getline(is, token, delimiter)) results.धकेलो_back(token);
    प्रत्यागम results;
}

तार्किक VString::endsWith(ध्रुव string& str, ध्रुव string& suffix) {
    यदि (str.length() < suffix.length()) प्रत्यागम असत्य;
    प्रत्यागम str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}
string VString::aOrAn(ध्रुव वर्ण* word) {
    विकल्प (word[0]) {
    पक्ष '\0': प्रत्यागम "";
    पक्ष 'a':
    पक्ष 'e':
    पक्ष 'i':
    पक्ष 'o':
    पक्ष 'u': प्रत्यागम "an";
    स्वभाव: प्रत्यागम "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ध्रुव string& str) VL_PURE {
    ध्रुव वर्ण* कुंजी = str.c_str();
    ध्रुव माप_प्रकार len = str.size();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr पूर्णाङ्क r = 47;

    uint64_t h = seed ^ (len * m);

    ध्रुव uint64_t* data = reinterpret_cast<ध्रुव uint64_t*>(कुंजी);
    ध्रुव uint64_t* end = data + (len / 8);

    यावत् (data != end) {
        uint64_t k = *data++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ध्रुव अचिन्हित वर्ण* data2 = reinterpret_cast<ध्रुव अचिन्हित वर्ण*>(data);

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

    प्रत्यागम h;
}

string VString::base64Enc(ध्रुव string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    स्थिर constexpr ध्रुव वर्ण* ध्रुव digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    माप_प्रकार len = str.size();
    string result;
    result.reserve(len * 4 / 3 + 2);
    माप_प्रकार pos = 0;
    यदि (len >= 3) {
        प्रति (; pos < len - 2; pos += 3) {
            result += digits[((str[pos] >> 2) & 0x3f)];
            result
                += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णाङ्क>(str[pos + 1] & 0xf0) >> 4)];
            result += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<पूर्णाङ्क>(str[pos + 2] & 0xc0) >> 6)];
            result += digits[((str[pos + 2] & 0x3f))];
        }
    }
    यदि (len >= 2 && pos < len - 1) {  // Pad 2
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णाङ्क>(str[pos + 1] & 0xf0) >> 4)];
        result += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<पूर्णाङ्क>(0) >> 6)];
        result += '=';
    } अन्यथा यदि (pos < len) {  // Pad 1
        result += digits[((str[pos] >> 2) & 0x3f)];
        result += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णाङ्क>(0) >> 4)];
        result += '=';
        result += '=';
    }
    प्रत्यागम result;
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
    प्रत्यागम lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
स्थिर शून्य sha512Block(uint64_t* h, ध्रुव uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    ध्रुव uint64_t* p = chunk;

    // Initialize working variables to current hash value
    प्रति (अचिन्हित i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    प्रति (अचिन्हित i = 0; i < 5; ++i) {
        प्रति (अचिन्हित j = 0; j < 16; ++j) {
            यदि (i == 0) {
                w[j] = *p++;
            } अन्यथा {
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
    प्रति (अचिन्हित i = 0; i < 8; ++i) h[i] += ah[i];
}

शून्य VHashSha512::जोड़ो(ध्रुव शून्य* datap, माप_प्रकार length) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_totLength += length;

    string tempData;
    पूर्णाङ्क chunkLen;
    ध्रुव uint8_t* chunkp;
    यदि (m_remainder == "") {
        chunkLen = length;
        chunkp = static_cast<ध्रुव uint8_t*>(datap);
    } अन्यथा {
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
    यावत् (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        प्रति (पूर्णाङ्क roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

शून्य VHashSha512::जोड़ो_File(ध्रुव string& filename) {
    स्थिर constexpr माप_प्रकार BUFFER_SIZE = 64 * 1024;

    ध्रुव पूर्णाङ्क fd = ::खोलो(filename.c_str(), O_RDONLY);
    यदि (fd < 0) प्रत्यागम;

    std::array<वर्ण, BUFFER_SIZE + 1> buf;
    यावत् (ध्रुव ssize_t got = ::पठ(fd, &buf, BUFFER_SIZE)) {
        यदि (got <= 0) भङ्ग;
        जोड़ो(&buf, got);
    }
    ::बंद(fd);
}

शून्य VHashSha512::finalize() {
    यदि (!m_final) {
        // Make sure no 128 byte blocks left
        जोड़ो("");
        m_final = सत्य;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        प्रति (पूर्णाङ्क i = 0; i < 16; ++i) w[i] = 0;
        माप_प्रकार blockPos = 0;
        प्रति (; blockPos < m_remainder.length(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        यदि (m_remainder.length() >= 112) {
            sha512Block(m_inthash, w);
            प्रति (पूर्णाङ्क i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_totLength << 3;
        sha512Block(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_द्विआधारी() {
    finalize();
    string result;
    result.reserve(64);
    प्रति (माप_प्रकार i = 0; i < 64; ++i) {
        result += static_cast<वर्ण>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    प्रत्यागम result;
}

uint64_t VHashSha512::digestUInt64() {
    ध्रुव string& binhash = digest_द्विआधारी();
    uint64_t result = 0;
    प्रति (माप_प्रकार byte = 0; byte < परिमाण(uint64_t); ++byte) {
        ध्रुव अचिन्हित वर्ण c = binhash[byte];
        result = (result << 8) | c;
    }
    प्रत्यागम result;
}

string VHashSha512::digestHex() {
    स्थिर constexpr ध्रुव वर्ण* ध्रुव digits = "0123456789abcdef";
    ध्रुव string& binhash = digest_द्विआधारी();
    string result;
    result.reserve(128);
    प्रति (माप_प्रकार byte = 0; byte < 64; ++byte) {
        result += digits[(binhash[byte] >> 4) & 0xf];
        result += digits[(binhash[byte] >> 0) & 0xf];
    }
    प्रत्यागम result;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    प्रत्यागम VString::base64Enc(digest_द्विआधारी());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    स्थिर constexpr ध्रुव वर्ण* ध्रुव digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ध्रुव string& binhash = digest_द्विआधारी();
    string result;
    result.reserve(84);
    पूर्णाङ्क pos = 0;
    प्रति (; pos < (512 / 8) - 2; pos += 3) {
        result += digits[((binhash[pos] >> 2) & 0x3f)];
        result += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<पूर्णाङ्क>(binhash[pos + 1] & 0xf0) >> 4)];
        result += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<पूर्णाङ्क>(binhash[pos + 2] & 0xc0) >> 6)];
        result += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        यदि (result.size() >= DIGEST_SYMBOL24_LENGTH) भङ्ग;
    }
    // Any leftover bits don't matter for our purpose
    प्रत्यागम result;
}

शून्य VHashSha512::selfTestOne(ध्रुव string& data, ध्रुव string& data2, ध्रुव string& exp,
                              ध्रुव string& exp64, ध्रुव string& exp24) {
    VHashSha512 digest{data};
    यदि (data2 != "") digest.जोड़ो(data2);
    यदि (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << endl;  // LCOV_EXCL_LINE
    }
    यदि (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << data + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << endl;  // LCOV_EXCL_LINE
    }
    यदि (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
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
    स्थिर constexpr माप_प्रकार VHSH_LEN = परिमाण(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_LENGTH;
    स्थिर ध्रुव माप_प्रकार DOT_LEN = std::म_माप("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    प्रति (string::size_type last_dot_pos = 0; last_dot_pos < in.size();) {
        ध्रुव string::size_type next_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ध्रुव स्वतः खोज_begin = std::begin(in) + last_dot_pos;
        ध्रुव स्वतः खोज_end
            = next_dot_pos == string::npos ? std::end(in) : std::begin(in) + next_dot_pos;

        // Search for __Vhsh between the two dots.
        ध्रुव स्वतः begin_vhsh
            = std::खोज(खोज_begin, खोज_end, std::begin(VHSH), std::end(VHSH) - 1);
        यदि (begin_vhsh != खोज_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ध्रुव स्वतः end_vhsh
                = begin_vhsh + std::min<माप_प्रकार>(std::distance(begin_vhsh, खोज_end), VHSH_LEN);
            ध्रुव std::string vhsh{begin_vhsh, end_vhsh};
            ध्रुव स्वतः& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            यदि (last_dot_pos > 0 && dehashed.empty()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = in.substr(0, last_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{खोज_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->second;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{end_vhsh, खोज_end};
        }
        // This component doesn't need dehashing but a previous one might have.
        अन्यथा यदि (!dehashed.empty()) {
            dehashed += std::string{खोज_begin, खोज_end};
        }

        यदि (next_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            यदि (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = next_dot_pos + DOT_LEN;
        } अन्यथा {
            last_dot_pos = string::npos;
        }
    }
    प्रत्यागम dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    यदि (m_name == "") प्रत्यागम "";
    यदि (m_hashed != "") प्रत्यागम m_hashed;  // Memoized
    यदि (s_maxLength == 0 || m_name.length() < s_maxLength) {
        m_hashed = m_name;
        प्रत्यागम m_hashed;
    }
    VHashSha512 hash{m_name};
    ध्रुव string suffix = "__Vhsh" + hash.digestSymbol24();
    यदि (s_minLength < s_maxLength) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        माप_प्रकार prefLength = s_minLength;
        यावत् (prefLength >= 1 && m_name[prefLength - 1] != '_') --prefLength;
        s_dehashMap[suffix] = m_name.substr(prefLength);
        m_hashed = m_name.substr(0, prefLength) + suffix;
    } अन्यथा {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    प्रत्यागम m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(ध्रुव string& s, ध्रुव string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ध्रुव माप_प्रकार sLen = s.length();
    ध्रुव माप_प्रकार tLen = t.length();
    यदि (sLen == 0) प्रत्यागम tLen;
    यदि (tLen == 0) प्रत्यागम sLen;
    यदि (sLen >= LENGTH_LIMIT) प्रत्यागम sLen;
    यदि (tLen >= LENGTH_LIMIT) प्रत्यागम tLen;

    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_two_ago;
    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_one_ago;
    स्थिर std::array<EditDistance, LENGTH_LIMIT + 1> s_v_next;

    प्रति (माप_प्रकार i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    प्रति (माप_प्रकार i = 0; i < tLen; i++) {
        s_v_next[0] = i + 1;
        प्रति (माप_प्रकार j = 0; j < sLen; j++) {
            ध्रुव EditDistance cost = (s[j] == t[i] ? 0 : 1);
            ध्रुव EditDistance deletion = s_v_next[j] + 1;
            ध्रुव EditDistance insertion = s_v_one_ago[j + 1] + 1;
            ध्रुव EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            यदि (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ध्रुव EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_next[j + 1] = cheapest;
        }
        प्रति (माप_प्रकार j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_next[j];
        }
    }

    ध्रुव EditDistance result = s_v_next[sLen];
    प्रत्यागम result;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(माप_प्रकार goal_len, माप_प्रकार candidate_len) {
    // Return max acceptable edit distance
    ध्रुव माप_प्रकार max_length = std::max(goal_len, candidate_len);
    ध्रुव माप_प्रकार min_length = std::min(goal_len, candidate_len);
    यदि (max_length <= 1) प्रत्यागम 0;
    यदि (max_length - min_length <= 1) प्रत्यागम std::max(max_length / 3, static_cast<माप_प्रकार>(1));
    प्रत्यागम (max_length + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(ध्रुव string& goal,
                                                      EditDistance& distancer) ध्रुव {
    Candidate best;
    ध्रुव माप_प्रकार gLen = goal.length();
    distancer = LENGTH_LIMIT * 10;
    प्रति (ध्रुव Candidate& candidate : m_candidates) {
        ध्रुव माप_प्रकार cLen = candidate.str.length();

        // Min distance must be inserting/deleting to make lengths match
        ध्रुव EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        यदि (min_distance >= distancer) अनुवर्त;  // Short-circuit if already better

        ध्रुव EditDistance cutoff = cutoffDistance(gLen, cLen);
        यदि (min_distance > cutoff) अनुवर्त;  // Short-circuit if already too bad

        ध्रुव EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.संदेश
                                      << "'");
        यदि (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    यदि (distancer == 0) प्रत्यागम Candidate{.str = best.str, .संदेश = ""};
    प्रत्यागम best;
}

शून्य VSpellCheck::selfTestDistanceOne(ध्रुव string& a, ध्रुव string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

शून्य VSpellCheck::selfTestSuggestOne(तार्किक matches, ध्रुव string& c, ध्रुव string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.धकेलो_Candidate(c);
    ध्रुव string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    यदि (matches) {
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
        ध्रुव VSpellCheck speller;
        UASSERT_SELFTEST(speller.bestCandidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.धकेलो_Candidate("fred");
        speller.धकेलो_Candidate("wilma");
        speller.धकेलो_Candidate("barney");
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.bestCandidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.bestCandidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        प्रति (अचिन्हित i = 0; i < अंक_CANDIDATE_LIMIT; ++i) speller.धकेलो_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.size(), अंक_CANDIDATE_LIMIT);
        speller.धकेलो_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.size(), अंक_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
