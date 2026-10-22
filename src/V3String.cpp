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

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#अगर defined(_WIN32) || defined(__MINGW32__)
#समावेश <io.h>  // open, read, write, close
#पूर्ण_अगर

#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3String.h"

#अगर_अघोषित V3ERROR_NO_GLOBAL_
#समावेश "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#पूर्ण_अगर

#समावेश <algorithm>
#समावेश <fcntl.h>

माप_प्रकार VName::s_min_लंबाई = 32;
माप_प्रकार VName::s_max_लंबाई = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
ढाँचा <bool Even = true>
अटल bool wildMatchImpl(स्थिर अक्षर* s, स्थिर अक्षर* p) VL_PURE {
    क्रम (; *p; s++, p++) {
        अगर (*p != '*') {
            अगर (((*s) != (*p)) && *p != '?') वापस false;
        } अन्यथा {
            // Trailing star matches everything.
            अगर (!*++p) वापस true;
            जबतक (!wildMatchImpl<!Even>(s, p)) {
                अगर (*++s == '\0') वापस false;
            }
            वापस true;
        }
    }
    वापस (*s == '\0');
}

bool VString::wildmatch(स्थिर अक्षर* s, स्थिर अक्षर* p) VL_PURE {
    अगर (*s == '\0') {
        जबतक (*p == '*') ++p;
        वापस *p == '\0';
    }
    वापस wildMatchImpl(s, p);
}

bool VString::wildmatch(स्थिर string& s, स्थिर string& p) VL_PURE {
    वापस wildmatch(s.c_str(), p.c_str());
}

string VString::dot(स्थिर string& a, स्थिर string& dot, स्थिर string& b) {
    अगर (b == "") वापस a;
    अगर (a == "") वापस b;
    वापस a + dot + b;
}

string VString::downcase(स्थिर string& str) VL_PURE {
    string परिणाम = str;
    क्रम (अक्षर& cr : परिणाम) cr = std::छोटे(cr);
    वापस परिणाम;
}

string VString::upcase(स्थिर string& str) VL_PURE {
    string परिणाम = str;
    क्रम (अक्षर& cr : परिणाम) cr = std::बड़े(cr);
    वापस परिणाम;
}

string VString::quoteAny(स्थिर string& str, अक्षर tgt, अक्षर esc) VL_PURE {
    string परिणाम;
    क्रम (स्थिर अक्षर c : str) {
        अगर (c == tgt) परिणाम += esc;
        परिणाम += c;
    }
    वापस परिणाम;
}

string VString::dequotePercent(स्थिर string& str) {
    string परिणाम;
    अक्षर last = '\0';
    क्रम (स्थिर अक्षर c : str) {
        अगर (last == '%' && c == '%') {
            last = '\0';
        } अन्यथा {
            परिणाम += c;
            last = c;
        }
    }
    वापस परिणाम;
}

string VString::quoteStringLiteralForShell(स्थिर string& str) {
    string परिणाम;
    constexpr अक्षर dquote = '"';
    constexpr अक्षर escape = '\\';
    परिणाम.धकेलो_back(dquote);  // Start quoted string
    परिणाम.धकेलो_back(escape);
    परिणाम.धकेलो_back(dquote);  // "
    क्रम (स्थिर अक्षर c : str) {
        अगर (c == dquote || c == escape) परिणाम.धकेलो_back(escape);
        परिणाम.धकेलो_back(c);
    }
    परिणाम.धकेलो_back(escape);
    परिणाम.धकेलो_back(dquote);  // "
    परिणाम.धकेलो_back(dquote);  // Terminate quoted string
    वापस परिणाम;
}

string VString::escapeStringForPath(स्थिर string& str) {
    अगर (str.find(R"(\\)") != string::npos)
        वापस str;  // if it has been escaped already, don't do it again
    अगर (str.find('/') != string::npos) वापस str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string परिणाम;
    constexpr अक्षर space = ' ';  // escape space like this `Program Files`
    constexpr अक्षर escape = '\\';
    क्रम (स्थिर अक्षर c : str) {
        अगर (c == space || c == escape) परिणाम.धकेलो_back(escape);
        परिणाम.धकेलो_back(c);
    }
    वापस परिणाम;
}

अटल पूर्णांक vl_decodexdigit(अक्षर c) {
    वापस std::है_अंक(c) ? c - '0' : std::छोटे(c) - 'a' + 10;
}

string VString::unquoteSVString(स्थिर string& text, string& errOut) {
    bool quoted = false;
    string newtext;
    newtext.reserve(text.आकार());
    अचिन्हित अक्षर octal_val = 0;
    पूर्णांक octal_digits = 0;
    क्रम (string::const_iterator cp = text.begin(); cp != text.end(); ++cp) {
        अगर (quoted) {
            अगर (std::है_अंक(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                अगर (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = false;
                }
            } अन्यथा {
                अगर (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = false;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    जारी;
                }
                quoted = false;
                अगर (*cp == 'n') {
                    newtext += '\n';
                } अन्यथा अगर (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } अन्यथा अगर (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } अन्यथा अगर (*cp == 'r') {
                    newtext += '\r';
                } अन्यथा अगर (*cp == 't') {
                    newtext += '\t';
                } अन्यथा अगर (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } अन्यथा अगर (*cp == 'x' && std::है_षष्ठादशक(cp[1])
                           && std::है_षष्ठादशक(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<अक्षर>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } अन्यथा अगर (std::है_अक्षर_अंक(*cp)) {
                    errOut = "Unknown escape sequence: \\";
                    errOut += *cp;
                    अवरोध;
                } अन्यथा {
                    newtext += *cp;
                }
            }
        } अन्यथा अगर (*cp == '\\') {
            अगर (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = true;
            octal_digits = 0;
        } अन्यथा {
            newtext += *cp;
        }
    }
    वापस newtext;
}

माप_प्रकार VString::quotedEnd(स्थिर string& str, माप_प्रकार pos) VL_PURE {
    क्रम (माप_प्रकार i = pos + 1; i < str.आकार(); ++i) {
        अगर (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } अन्यथा अगर (str[i] == '"') {
            वापस i + 1;
        }
    }
    वापस string::npos;
}

string VString::spaceUnprintable(स्थिर string& str) VL_PURE {
    string परिणाम;
    क्रम (स्थिर अक्षर c : str) {
        अगर (std::है_छाप(c)) {
            परिणाम += c;
        } अन्यथा {
            परिणाम += ' ';
        }
    }
    वापस परिणाम;
}

string VString::removeWhitespace(स्थिर string& str) {
    string परिणाम;
    परिणाम.reserve(str.आकार());
    क्रम (स्थिर अक्षर c : str) {
        अगर (!std::है_खाली(c)) परिणाम += c;
    }
    वापस परिणाम;
}

string VString::trimWhitespace(स्थिर string& str) {
    string परिणाम;
    परिणाम.reserve(str.आकार());
    string add;
    bool newline = false;
    क्रम (स्थिर अक्षर c : str) {
        अगर (newline && std::है_खाली(c)) जारी;
        अगर (c == '\n') {
            add = "\n";
            newline = true;
            जारी;
        }
        अगर (std::है_खाली(c)) {
            add += c;
            जारी;
        }
        अगर (!add.empty()) {
            परिणाम += add;
            newline = false;
            add.सफा();
        }
        परिणाम += c;
    }
    वापस परिणाम;
}

bool VString::isIdentifier(स्थिर string& str) {
    क्रम (स्थिर अक्षर c : str) {
        अगर (!isIdentifierChar(c)) वापस false;
    }
    वापस true;
}

bool VString::isWhitespace(स्थिर string& str) {
    क्रम (स्थिर अक्षर c : str) {
        अगर (!std::है_खाली(c)) वापस false;
    }
    वापस true;
}

string::आकार_type VString::leading_Whitespace_गिनती(स्थिर string& str) {
    string::आकार_type परिणाम = 0;
    क्रम (स्थिर अक्षर c : str) {
        ++परिणाम;
        अगर (!std::है_खाली(c)) अवरोध;
    }
    वापस परिणाम;
}

द्विगुन VString::parseDouble(स्थिर string& str, bool* successp) {
    अक्षर* स्थिर strgp = नया अक्षर[str.आकार() + 1];
    अक्षर* dp = strgp;
    अगर (successp) *successp = true;
    क्रम (स्थिर अक्षर* sp = str.c_str(); *sp; ++sp) {
        अगर (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    अक्षर* endp = strgp;
    स्थिर द्विगुन d = म_से_भग्न(strgp, &endp);
    स्थिर माप_प्रकार parsed_len = endp - strgp;
    अगर (parsed_len != std::म_माप(strgp)) {
        अगर (successp) *successp = false;
    }
    VL_DO_DANGLING(मिटाओ[] strgp, strgp);
    वापस d;
}

string VString::replaceSubstr(स्थिर string& str, स्थिर string& from, स्थिर string& to) {
    string परिणाम = str;
    स्थिर माप_प्रकार fromLen = from.आकार();
    स्थिर माप_प्रकार toLen = to.आकार();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    क्रम (माप_प्रकार pos = 0; (pos = परिणाम.find(from, pos)) != string::npos; pos += toLen) {
        परिणाम.replace(pos, fromLen, to);
    }
    वापस परिणाम;
}

string VString::replaceWord(स्थिर string& str, स्थिर string& from, स्थिर string& to) {
    string परिणाम = str;
    स्थिर माप_प्रकार len = from.आकार();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    क्रम (माप_प्रकार pos = 0; (pos = परिणाम.find(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        अगर (((pos > 0) && VString::isIdentifierChar(परिणाम[pos - 1])) ||  //
            ((pos + len < परिणाम.आकार()) && VString::isIdentifierChar(परिणाम[pos + len]))) {
            जारी;
        }
        परिणाम.replace(pos, len, to);
    }
    वापस परिणाम;
}

std::deque<string> VString::split(स्थिर string& str, अक्षर delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    जबतक (std::लो_पंक्ति(is, token, delimiter)) results.धकेलो_back(token);
    वापस results;
}

bool VString::endsWith(स्थिर string& str, स्थिर string& suffix) {
    अगर (str.लंबाई() < suffix.लंबाई()) वापस false;
    वापस str.compare(str.लंबाई() - suffix.लंबाई(), suffix.लंबाई(), suffix) == 0;
}
string VString::aOrAn(स्थिर अक्षर* word) {
    चयन (word[0]) {
    हाल '\0': वापस "";
    हाल 'a':
    हाल 'e':
    हाल 'i':
    हाल 'o':
    हाल 'u': वापस "an";
    शेष: वापस "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(स्थिर string& str) VL_PURE {
    स्थिर अक्षर* कुंजी = str.c_str();
    स्थिर माप_प्रकार len = str.आकार();
    constexpr uint64_t seed = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr पूर्णांक r = 47;

    uint64_t h = seed ^ (len * m);

    स्थिर uint64_t* तथ्य = reinterpret_cast<स्थिर uint64_t*>(कुंजी);
    स्थिर uint64_t* end = तथ्य + (len / 8);

    जबतक (तथ्य != end) {
        uint64_t k = *तथ्य++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    स्थिर अचिन्हित अक्षर* data2 = reinterpret_cast<स्थिर अचिन्हित अक्षर*>(तथ्य);

    चयन (len & 7) {
    हाल 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    हाल 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    हाल 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    हाल 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    हाल 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    हाल 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    हाल 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    शेष:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    वापस h;
}

string VString::base64Enc(स्थिर string& str) VL_PURE {
    // Non-URL format (+/), versus URL format (-/).
    अटल constexpr स्थिर अक्षर* स्थिर digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    माप_प्रकार len = str.आकार();
    string परिणाम;
    परिणाम.reserve(len * 4 / 3 + 2);
    माप_प्रकार pos = 0;
    अगर (len >= 3) {
        क्रम (; pos < len - 2; pos += 3) {
            परिणाम += digits[((str[pos] >> 2) & 0x3f)];
            परिणाम
                += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णांक>(str[pos + 1] & 0xf0) >> 4)];
            परिणाम += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<पूर्णांक>(str[pos + 2] & 0xc0) >> 6)];
            परिणाम += digits[((str[pos + 2] & 0x3f))];
        }
    }
    अगर (len >= 2 && pos < len - 1) {  // Pad 2
        परिणाम += digits[((str[pos] >> 2) & 0x3f)];
        परिणाम += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णांक>(str[pos + 1] & 0xf0) >> 4)];
        परिणाम += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<पूर्णांक>(0) >> 6)];
        परिणाम += '=';
    } अन्यथा अगर (pos < len) {  // Pad 1
        परिणाम += digits[((str[pos] >> 2) & 0x3f)];
        परिणाम += digits[((str[pos] & 0x3) << 4) | (static_cast<पूर्णांक>(0) >> 4)];
        परिणाम += '=';
        परिणाम += '=';
    }
    वापस परिणाम;
}

व्योम VString::selfTest() {
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

अटल constexpr uint64_t sha512K[]
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
अटल uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_PURE {
    वापस lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
अटल व्योम sha512Block(uint64_t* h, स्थिर uint64_t* chunk) VL_PURE {
    uint64_t ah[8];
    स्थिर uint64_t* p = chunk;

    // Initialize working variables to current hash value
    क्रम (अचिन्हित i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    क्रम (अचिन्हित i = 0; i < 5; ++i) {
        क्रम (अचिन्हित j = 0; j < 16; ++j) {
            अगर (i == 0) {
                w[j] = *p++;
            } अन्यथा {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                स्थिर uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                स्थिर uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            स्थिर uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            स्थिर uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            स्थिर uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            स्थिर uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            स्थिर uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            स्थिर uint64_t temp2 = s0 + maj;

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
    क्रम (अचिन्हित i = 0; i < 8; ++i) h[i] += ah[i];
}

व्योम VHashSha512::जोड़ो(स्थिर व्योम* datap, माप_प्रकार लंबाई) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_लंबाई += लंबाई;

    string अस्थायी_तथ्य;
    पूर्णांक chunkLen;
    स्थिर uint8_t* chunkp;
    अगर (m_remainder == "") {
        chunkLen = लंबाई;
        chunkp = static_cast<स्थिर uint8_t*>(datap);
    } अन्यथा {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        अस्थायी_तथ्य = m_remainder + std::string{static_cast<स्थिर अक्षर*>(datap), लंबाई};
        chunkLen = अस्थायी_तथ्य.लंबाई();
        chunkp = reinterpret_cast<स्थिर uint8_t*>(अस्थायी_तथ्य.तथ्य());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    पूर्णांक posBegin = 0;  // Position in buffer for start of this block
    पूर्णांक posEnd = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    जबतक (posBegin <= chunkLen - 128) {
        posEnd = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        क्रम (पूर्णांक roundByte = 0; posBegin < posEnd; posBegin += 8) {
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

    m_remainder = std::string(reinterpret_cast<स्थिर अक्षर*>(chunkp + posBegin), chunkLen - posEnd);
}

व्योम VHashSha512::जोड़ो_File(स्थिर string& filename) {
    अटल constexpr माप_प्रकार बफर_आकार = 64 * 1024;

    स्थिर पूर्णांक fd = ::खोलो(filename.c_str(), O_RDONLY);
    अगर (fd < 0) वापस;

    std::array<अक्षर, बफर_आकार + 1> buf;
    जबतक (स्थिर ssize_t got = ::पढ़ो(fd, &buf, बफर_आकार)) {
        अगर (got <= 0) अवरोध;
        जोड़ो(&buf, got);
    }
    ::बंद(fd);
}

व्योम VHashSha512::finalize() {
    अगर (!m_final) {
        // Make sure no 128 byte blocks left
        जोड़ो("");
        m_final = true;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        क्रम (पूर्णांक i = 0; i < 16; ++i) w[i] = 0;
        माप_प्रकार blockPos = 0;
        क्रम (; blockPos < m_remainder.लंबाई(); ++blockPos) {
            w[blockPos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[blockPos])))
                    << ((7 - (blockPos & 7)) << 3));
        }
        w[blockPos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (blockPos & 7)) << 3);
        अगर (m_remainder.लंबाई() >= 112) {
            sha512Block(m_inthash, w);
            क्रम (पूर्णांक i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_लंबाई << 3;
        sha512Block(m_inthash, w);

        m_remainder.सफा();
    }
}

string VHashSha512::digest_द्विआधारी() {
    finalize();
    string परिणाम;
    परिणाम.reserve(64);
    क्रम (माप_प्रकार i = 0; i < 64; ++i) {
        परिणाम += static_cast<अक्षर>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    वापस परिणाम;
}

uint64_t VHashSha512::digestUInt64() {
    स्थिर string& binhash = digest_द्विआधारी();
    uint64_t परिणाम = 0;
    क्रम (माप_प्रकार byte = 0; byte < माप(uint64_t); ++byte) {
        स्थिर अचिन्हित अक्षर c = binhash[byte];
        परिणाम = (परिणाम << 8) | c;
    }
    वापस परिणाम;
}

string VHashSha512::digestHex() {
    अटल constexpr स्थिर अक्षर* स्थिर digits = "0123456789abcdef";
    स्थिर string& binhash = digest_द्विआधारी();
    string परिणाम;
    परिणाम.reserve(128);
    क्रम (माप_प्रकार byte = 0; byte < 64; ++byte) {
        परिणाम += digits[(binhash[byte] >> 4) & 0xf];
        परिणाम += digits[(binhash[byte] >> 0) & 0xf];
    }
    वापस परिणाम;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    वापस VString::base64Enc(digest_द्विआधारी());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    अटल constexpr स्थिर अक्षर* स्थिर digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    स्थिर string& binhash = digest_द्विआधारी();
    string परिणाम;
    परिणाम.reserve(84);
    पूर्णांक pos = 0;
    क्रम (; pos < (512 / 8) - 2; pos += 3) {
        परिणाम += digits[((binhash[pos] >> 2) & 0x3f)];
        परिणाम += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<पूर्णांक>(binhash[pos + 1] & 0xf0) >> 4)];
        परिणाम += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<पूर्णांक>(binhash[pos + 2] & 0xc0) >> 6)];
        परिणाम += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        अगर (परिणाम.आकार() >= DIGEST_SYMBOL24_लंबाई) अवरोध;
    }
    // Any leftover bits don't matter for our purpose
    वापस परिणाम;
}

व्योम VHashSha512::selfTestOne(स्थिर string& तथ्य, स्थिर string& data2, स्थिर string& exp,
                              स्थिर string& exp64, स्थिर string& exp24) {
    VHashSha512 digest{तथ्य};
    अगर (data2 != "") digest.जोड़ो(data2);
    अगर (VL_UNCOVERABLE(digest.digestHex() != exp)) {
        std::अ_त्रुटि << "%Error: When hashing '" << तथ्य + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp << प_पूर्ण;  // LCOV_EXCL_LINE
    }
    अगर (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::अ_त्रुटि << "%Error: When hashing '" << तथ्य + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << प_पूर्ण;  // LCOV_EXCL_LINE
    }
    अगर (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::अ_त्रुटि << "%Error: When hashing '" << तथ्य + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << प_पूर्ण;  // LCOV_EXCL_LINE
    }
}

व्योम VHashSha512::selfTest() {
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

string VName::dehash(स्थिर string& in) {
    अटल constexpr स्थिर अक्षर VHSH[] = "__Vhsh";
    अटल constexpr माप_प्रकार VHSH_LEN = माप(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_लंबाई;
    अटल स्थिर माप_प्रकार DOT_LEN = std::म_माप("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    क्रम (string::आकार_type last_dot_pos = 0; last_dot_pos < in.आकार();) {
        स्थिर string::आकार_type अग्र_dot_pos = in.find("__DOT__", last_dot_pos);
        // Two iterators defining the range between the last and next dots.
        स्थिर स्वतः खोज_begin = std::begin(in) + last_dot_pos;
        स्थिर स्वतः खोज_end
            = अग्र_dot_pos == string::npos ? std::end(in) : std::begin(in) + अग्र_dot_pos;

        // Search for __Vhsh between the two dots.
        स्थिर स्वतः begin_vhsh
            = std::खोज(खोज_begin, खोज_end, std::begin(VHSH), std::end(VHSH) - 1);
        अगर (begin_vhsh != खोज_end) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            स्थिर स्वतः end_vhsh
                = begin_vhsh + std::min<माप_प्रकार>(std::distance(begin_vhsh, खोज_end), VHSH_LEN);
            स्थिर std::string vhsh{begin_vhsh, end_vhsh};
            स्थिर स्वतः& it = s_dehashMap.find(vhsh);
            UASSERT(it != s_dehashMap.end(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            अगर (last_dot_pos > 0 && dehashed.empty()) {
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
        अन्यथा अगर (!dehashed.empty()) {
            dehashed += std::string{खोज_begin, खोज_end};
        }

        अगर (अग्र_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            अगर (!dehashed.empty()) dehashed += "__DOT__";
            last_dot_pos = अग्र_dot_pos + DOT_LEN;
        } अन्यथा {
            last_dot_pos = string::npos;
        }
    }
    वापस dehashed.empty() ? in : dehashed;
}

string VName::hashedName() {
    अगर (m_name == "") वापस "";
    अगर (m_hashed != "") वापस m_hashed;  // Memoized
    अगर (s_max_लंबाई == 0 || m_name.लंबाई() < s_max_लंबाई) {
        m_hashed = m_name;
        वापस m_hashed;
    }
    VHashSha512 hash{m_name};
    स्थिर string suffix = "__Vhsh" + hash.digestSymbol24();
    अगर (s_min_लंबाई < s_max_लंबाई) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        माप_प्रकार pref_लंबाई = s_min_लंबाई;
        जबतक (pref_लंबाई >= 1 && m_name[pref_लंबाई - 1] != '_') --pref_लंबाई;
        s_dehashMap[suffix] = m_name.substr(pref_लंबाई);
        m_hashed = m_name.substr(0, pref_लंबाई) + suffix;
    } अन्यथा {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    वापस m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::EditDistance VSpellCheck::editDistance(स्थिर string& s, स्थिर string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    स्थिर माप_प्रकार sLen = s.लंबाई();
    स्थिर माप_प्रकार tLen = t.लंबाई();
    अगर (sLen == 0) वापस tLen;
    अगर (tLen == 0) वापस sLen;
    अगर (sLen >= लंबाई_LIMIT) वापस sLen;
    अगर (tLen >= लंबाई_LIMIT) वापस tLen;

    अटल std::array<EditDistance, लंबाई_LIMIT + 1> s_v_two_ago;
    अटल std::array<EditDistance, लंबाई_LIMIT + 1> s_v_one_ago;
    अटल std::array<EditDistance, लंबाई_LIMIT + 1> s_v_अग्र;

    क्रम (माप_प्रकार i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    क्रम (माप_प्रकार i = 0; i < tLen; i++) {
        s_v_अग्र[0] = i + 1;
        क्रम (माप_प्रकार j = 0; j < sLen; j++) {
            स्थिर EditDistance cost = (s[j] == t[i] ? 0 : 1);
            स्थिर EditDistance deletion = s_v_अग्र[j] + 1;
            स्थिर EditDistance insertion = s_v_one_ago[j + 1] + 1;
            स्थिर EditDistance substitution = s_v_one_ago[j] + cost;
            EditDistance cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            अगर (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                स्थिर EditDistance transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_अग्र[j + 1] = cheapest;
        }
        क्रम (माप_प्रकार j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_अग्र[j];
        }
    }

    स्थिर EditDistance परिणाम = s_v_अग्र[sLen];
    वापस परिणाम;
}

VSpellCheck::EditDistance VSpellCheck::cutoffDistance(माप_प्रकार goal_len, माप_प्रकार candidate_len) {
    // Return max acceptable edit distance
    स्थिर माप_प्रकार max_लंबाई_2 = std::max(goal_len, candidate_len);
    स्थिर माप_प्रकार min_लंबाई = std::min(goal_len, candidate_len);
    अगर (max_लंबाई_2 <= 1) वापस 0;
    अगर (max_लंबाई_2 - min_लंबाई <= 1) वापस std::max(max_लंबाई_2 / 3, static_cast<माप_प्रकार>(1));
    वापस (max_लंबाई_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::bestCandidateInfo(स्थिर string& goal,
                                                      EditDistance& distancer) स्थिर {
    Candidate best;
    स्थिर माप_प्रकार gLen = goal.लंबाई();
    distancer = लंबाई_LIMIT * 10;
    क्रम (स्थिर Candidate& candidate : m_candidates) {
        स्थिर माप_प्रकार cLen = candidate.str.लंबाई();

        // Min distance must be inserting/deleting to make lengths match
        स्थिर EditDistance min_distance = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        अगर (min_distance >= distancer) जारी;  // Short-circuit if already better

        स्थिर EditDistance cutoff = cutoffDistance(gLen, cLen);
        अगर (min_distance > cutoff) जारी;  // Short-circuit if already too bad

        स्थिर EditDistance dist = editDistance(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.संदेश
                                      << "'");
        अगर (dist < distancer && dist <= cutoff) {
            distancer = dist;
            best = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    अगर (distancer == 0) वापस Candidate{.str = best.str, .संदेश = ""};
    वापस best;
}

व्योम VSpellCheck::selfTestDistanceOne(स्थिर string& a, स्थिर string& b, EditDistance expected) {
    UASSERT_SELFTEST(editDistance(a, b), expected);
    UASSERT_SELFTEST(editDistance(b, a), expected);
}

व्योम VSpellCheck::selfTestSuggestOne(bool matches, स्थिर string& c, स्थिर string& goal,
                                     EditDistance dist) {
    EditDistance gdist;
    VSpellCheck speller;
    speller.धकेलो_Candidate(c);
    स्थिर string got = speller.bestCandidateInfo(goal, gdist /*ref*/).str;
    अगर (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } अन्यथा {
        UASSERT_SELFTEST(got, "");
    }
}

व्योम VSpellCheck::selfTest() {
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
        selfTestSuggestOne(false, "x", "y", 1);
        selfTestSuggestOne(false, "sqrt", "assert", 3);
    }
    {
        स्थिर VSpellCheck speller;
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
        क्रम (अचिन्हित i = 0; i < अंक_CANDIDATE_LIMIT; ++i) speller.धकेलो_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.आकार(), अंक_CANDIDATE_LIMIT);
        speller.धकेलो_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.आकार(), अंक_CANDIDATE_LIMIT);
        UASSERT_SELFTEST(speller.bestCandidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.bestCandidate("wilma").str, "");
    }
}
