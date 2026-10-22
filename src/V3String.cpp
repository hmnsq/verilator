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

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਜੇ defined(_WIN32) || defined(__MINGW32__)
#ਅਨ੍ਤਰ੍ਭਾਵ <io.h>  // open, read, write, close
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>

ਮਾਪ_t VName::s_min_ਲੰਮਾਈ = 32;
ਮਾਪ_t VName::s_max_ਲੰਮਾਈ = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
ਸਾਂਚਾ <ਤਾਰਕਿਕ Even = ਸੱਚ>
ਸਥਿਰ ਤਾਰਕਿਕ wild_ਮੇਲ_Impl(ਅਚਲ ਅੱਖਰ* s, ਅਚਲ ਅੱਖਰ* p) VL_ਸ਼ੁੱਧ {
    ਲਈ (; *p; s++, p++) {
        ਜੇ (*p != '*') {
            ਜੇ (((*s) != (*p)) && *p != '?') ਵਾਪਸ ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            // Trailing star matches everything.
            ਜੇ (!*++p) ਵਾਪਸ ਸੱਚ;
            ਜਦੋਂ (!wild_ਮੇਲ_Impl<!Even>(s, p)) {
                ਜੇ (*++s == '\0') ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ ਸੱਚ;
        }
    }
    ਵਾਪਸ (*s == '\0');
}

ਤਾਰਕਿਕ VString::wildmatch(ਅਚਲ ਅੱਖਰ* s, ਅਚਲ ਅੱਖਰ* p) VL_ਸ਼ੁੱਧ {
    ਜੇ (*s == '\0') {
        ਜਦੋਂ (*p == '*') ++p;
        ਵਾਪਸ *p == '\0';
    }
    ਵਾਪਸ wild_ਮੇਲ_Impl(s, p);
}

ਤਾਰਕਿਕ VString::wildmatch(ਅਚਲ string& s, ਅਚਲ string& p) VL_ਸ਼ੁੱਧ {
    ਵਾਪਸ wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ਅਚਲ string& a, ਅਚਲ string& dot, ਅਚਲ string& b) {
    ਜੇ (b == "") ਵਾਪਸ a;
    ਜੇ (a == "") ਵਾਪਸ b;
    ਵਾਪਸ a + dot + b;
}

string VString::downcase(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ {
    string ਨਤੀਜਾ = str;
    ਲਈ (ਅੱਖਰ& cr : ਨਤੀਜਾ) cr = std::tolower(cr);
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::upcase(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ {
    string ਨਤੀਜਾ = str;
    ਲਈ (ਅੱਖਰ& cr : ਨਤੀਜਾ) cr = std::ਵੱਡਾ_ਅੱਖਰ(cr);
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::quoteAny(ਅਚਲ string& str, ਅੱਖਰ tgt, ਅੱਖਰ esc) VL_ਸ਼ੁੱਧ {
    string ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (c == tgt) ਨਤੀਜਾ += esc;
        ਨਤੀਜਾ += c;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::dequotePercent(ਅਚਲ string& str) {
    string ਨਤੀਜਾ;
    ਅੱਖਰ ਆਖਰੀ = '\0';
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (ਆਖਰੀ == '%' && c == '%') {
            ਆਖਰੀ = '\0';
        } ਨਹੀਂਤਾਂ {
            ਨਤੀਜਾ += c;
            ਆਖਰੀ = c;
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::quoteStringLiteralForShell(ਅਚਲ string& str) {
    string ਨਤੀਜਾ;
    constexpr ਅੱਖਰ dquote = '"';
    constexpr ਅੱਖਰ escape = '\\';
    ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(dquote);  // Start quoted string
    ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(escape);
    ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(dquote);  // "
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (c == dquote || c == escape) ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(escape);
        ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(c);
    }
    ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(escape);
    ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(dquote);  // "
    ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(dquote);  // Terminate quoted string
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::escape_String_For_ਰਸਤਾ(ਅਚਲ string& str) {
    ਜੇ (str.ਲੱਭੋ(R"(\\)") != string::npos)
        ਵਾਪਸ str;  // if it has been escaped already, don't do it again
    ਜੇ (str.ਲੱਭੋ('/') != string::npos) ਵਾਪਸ str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string ਨਤੀਜਾ;
    constexpr ਅੱਖਰ space = ' ';  // escape space like this `Program Files`
    constexpr ਅੱਖਰ escape = '\\';
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (c == space || c == escape) ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(escape);
        ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(c);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਪੂਰਨ vl_decodexdigit(ਅੱਖਰ c) {
    ਵਾਪਸ std::ਅੰਕ_ਹੈ(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ਅਚਲ string& ਪਾਠ, string& err_ਬਾਹਰਲਾ) {
    ਤਾਰਕਿਕ quoted = ਝੂਠ;
    string newtext;
    newtext.reserve(ਪਾਠ.ਮਾਪ());
    ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ octal_val = 0;
    ਪੂਰਨ octal_digits = 0;
    ਲਈ (string::const_iterator cp = ਪਾਠ.begin(); cp != ਪਾਠ.ਸਿਰਾ(); ++cp) {
        ਜੇ (quoted) {
            ਜੇ (std::ਅੰਕ_ਹੈ(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                ਜੇ (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ਝੂਠ;
                }
            } ਨਹੀਂਤਾਂ {
                ਜੇ (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ਝੂਠ;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    ਜਾਰੀ;
                }
                quoted = ਝੂਠ;
                ਜੇ (*cp == 'n') {
                    newtext += '\n';
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == 'r') {
                    newtext += '\r';
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == 't') {
                    newtext += '\t';
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } ਨਹੀਂਤਾਂ ਜੇ (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<ਅੱਖਰ>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } ਨਹੀਂਤਾਂ ਜੇ (std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(*cp)) {
                    err_ਬਾਹਰਲਾ = "Unknown escape sequence: \\";
                    err_ਬਾਹਰਲਾ += *cp;
                    ਰੋਕੋ;
                } ਨਹੀਂਤਾਂ {
                    newtext += *cp;
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (*cp == '\\') {
            ਜੇ (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = ਸੱਚ;
            octal_digits = 0;
        } ਨਹੀਂਤਾਂ {
            newtext += *cp;
        }
    }
    ਵਾਪਸ newtext;
}

ਮਾਪ_t VString::quoted_ਸਿਰਾ(ਅਚਲ string& str, ਮਾਪ_t pos) VL_ਸ਼ੁੱਧ {
    ਲਈ (ਮਾਪ_t i = pos + 1; i < str.ਮਾਪ(); ++i) {
        ਜੇ (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } ਨਹੀਂਤਾਂ ਜੇ (str[i] == '"') {
            ਵਾਪਸ i + 1;
        }
    }
    ਵਾਪਸ string::npos;
}

string VString::spaceUnprintable(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ {
    string ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (std::isprint(c)) {
            ਨਤੀਜਾ += c;
        } ਨਹੀਂਤਾਂ {
            ਨਤੀਜਾ += ' ';
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::removeWhitespace(ਅਚਲ string& str) {
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(str.ਮਾਪ());
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (!std::ਖਾਲੀ_ਥਾਂ_ਹੈ(c)) ਨਤੀਜਾ += c;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::trimWhitespace(ਅਚਲ string& str) {
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(str.ਮਾਪ());
    string ਜੋੜੋ;
    ਤਾਰਕਿਕ newline = ਝੂਠ;
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (newline && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(c)) ਜਾਰੀ;
        ਜੇ (c == '\n') {
            ਜੋੜੋ = "\n";
            newline = ਸੱਚ;
            ਜਾਰੀ;
        }
        ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(c)) {
            ਜੋੜੋ += c;
            ਜਾਰੀ;
        }
        ਜੇ (!ਜੋੜੋ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਨਤੀਜਾ += ਜੋੜੋ;
            newline = ਝੂਠ;
            ਜੋੜੋ.clear();
        }
        ਨਤੀਜਾ += c;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਤਾਰਕਿਕ VString::isIdentifier(ਅਚਲ string& str) {
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (!isIdentifierChar(c)) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}

ਤਾਰਕਿਕ VString::isWhitespace(ਅਚਲ string& str) {
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ਜੇ (!std::ਖਾਲੀ_ਥਾਂ_ਹੈ(c)) ਵਾਪਸ ਝੂਠ;
    }
    ਵਾਪਸ ਸੱਚ;
}

string::ਮਾਪ_type VString::leading_Whitespace_ਗਿਣਤੀ(ਅਚਲ string& str) {
    string::ਮਾਪ_type ਨਤੀਜਾ = 0;
    ਲਈ (ਅਚਲ ਅੱਖਰ c : str) {
        ++ਨਤੀਜਾ;
        ਜੇ (!std::ਖਾਲੀ_ਥਾਂ_ਹੈ(c)) ਰੋਕੋ;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਦੁੱਗਣਾ VString::parseDouble(ਅਚਲ string& str, ਤਾਰਕਿਕ* successp) {
    ਅੱਖਰ* ਅਚਲ strgp = ਨਵਾਂ ਅੱਖਰ[str.ਮਾਪ() + 1];
    ਅੱਖਰ* dp = strgp;
    ਜੇ (successp) *successp = ਸੱਚ;
    ਲਈ (ਅਚਲ ਅੱਖਰ* sp = str.c_str(); *sp; ++sp) {
        ਜੇ (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    ਅੱਖਰ* endp = strgp;
    ਅਚਲ ਦੁੱਗਣਾ d = strtod(strgp, &endp);
    ਅਚਲ ਮਾਪ_t parsed_len = endp - strgp;
    ਜੇ (parsed_len != std::ਲੜੀ_ਲੰਬਾਈ(strgp)) {
        ਜੇ (successp) *successp = ਝੂਠ;
    }
    VL_DO_DANGLING(ਮਿਟਾਓ[] strgp, strgp);
    ਵਾਪਸ d;
}

string VString::replaceSubstr(ਅਚਲ string& str, ਅਚਲ string& from, ਅਚਲ string& ਮੰਜ਼ਿਲ) {
    string ਨਤੀਜਾ = str;
    ਅਚਲ ਮਾਪ_t fromLen = from.ਮਾਪ();
    ਅਚਲ ਮਾਪ_t ਮੰਜ਼ਿਲ_Len = ਮੰਜ਼ਿਲ.ਮਾਪ();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    ਲਈ (ਮਾਪ_t pos = 0; (pos = ਨਤੀਜਾ.ਲੱਭੋ(from, pos)) != string::npos; pos += ਮੰਜ਼ਿਲ_Len) {
        ਨਤੀਜਾ.replace(pos, fromLen, ਮੰਜ਼ਿਲ);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VString::replace_ਸ਼ਬਦ(ਅਚਲ string& str, ਅਚਲ string& from, ਅਚਲ string& ਮੰਜ਼ਿਲ) {
    string ਨਤੀਜਾ = str;
    ਅਚਲ ਮਾਪ_t len = from.ਮਾਪ();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    ਲਈ (ਮਾਪ_t pos = 0; (pos = ਨਤੀਜਾ.ਲੱਭੋ(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        ਜੇ (((pos > 0) && VString::isIdentifierChar(ਨਤੀਜਾ[pos - 1])) ||  //
            ((pos + len < ਨਤੀਜਾ.ਮਾਪ()) && VString::isIdentifierChar(ਨਤੀਜਾ[pos + len]))) {
            ਜਾਰੀ;
        }
        ਨਤੀਜਾ.replace(pos, len, ਮੰਜ਼ਿਲ);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

std::deque<string> VString::ਵੰਡੋ(ਅਚਲ string& str, ਅੱਖਰ delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    ਜਦੋਂ (std::getline(is, token, delimiter)) results.ਧੱਕੋ_ਪਿੱਠ(token);
    ਵਾਪਸ results;
}

ਤਾਰਕਿਕ VString::ਸਿਰੇ_With(ਅਚਲ string& str, ਅਚਲ string& suffix) {
    ਜੇ (str.ਲੰਮਾਈ() < suffix.ਲੰਮਾਈ()) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ str.compare(str.ਲੰਮਾਈ() - suffix.ਲੰਮਾਈ(), suffix.ਲੰਮਾਈ(), suffix) == 0;
}
string VString::aOrAn(ਅਚਲ ਅੱਖਰ* ਸ਼ਬਦ) {
    ਚੋਣ (ਸ਼ਬਦ[0]) {
    ਹਾਲਤ '\0': ਵਾਪਸ "";
    ਹਾਲਤ 'a':
    ਹਾਲਤ 'e':
    ਹਾਲਤ 'i':
    ਹਾਲਤ 'o':
    ਹਾਲਤ 'u': ਵਾਪਸ "an";
    ਮੂਲ: ਵਾਪਸ "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ {
    ਅਚਲ ਅੱਖਰ* ਕੁੰਜੀ = str.c_str();
    ਅਚਲ ਮਾਪ_t len = str.ਮਾਪ();
    constexpr uint64_t ਬੀਜ = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr ਪੂਰਨ r = 47;

    uint64_t h = ਬੀਜ ^ (len * m);

    ਅਚਲ uint64_t* ਅੰਕੜੇ = reinterpret_cast<ਅਚਲ uint64_t*>(ਕੁੰਜੀ);
    ਅਚਲ uint64_t* ਸਿਰਾ = ਅੰਕੜੇ + (len / 8);

    ਜਦੋਂ (ਅੰਕੜੇ != ਸਿਰਾ) {
        uint64_t k = *ਅੰਕੜੇ++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ* data2 = reinterpret_cast<ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ*>(ਅੰਕੜੇ);

    ਚੋਣ (len & 7) {
    ਹਾਲਤ 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    ਹਾਲਤ 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    ਹਾਲਤ 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    ਹਾਲਤ 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    ਹਾਲਤ 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    ਹਾਲਤ 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    ਹਾਲਤ 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    ਮੂਲ:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    ਵਾਪਸ h;
}

string VString::base64Enc(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ {
    // Non-URL format (+/), versus URL format (-/).
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    ਮਾਪ_t len = str.ਮਾਪ();
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(len * 4 / 3 + 2);
    ਮਾਪ_t pos = 0;
    ਜੇ (len >= 3) {
        ਲਈ (; pos < len - 2; pos += 3) {
            ਨਤੀਜਾ += digits[((str[pos] >> 2) & 0x3f)];
            ਨਤੀਜਾ
                += digits[((str[pos] & 0x3) << 4) | (static_cast<ਪੂਰਨ>(str[pos + 1] & 0xf0) >> 4)];
            ਨਤੀਜਾ += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<ਪੂਰਨ>(str[pos + 2] & 0xc0) >> 6)];
            ਨਤੀਜਾ += digits[((str[pos + 2] & 0x3f))];
        }
    }
    ਜੇ (len >= 2 && pos < len - 1) {  // Pad 2
        ਨਤੀਜਾ += digits[((str[pos] >> 2) & 0x3f)];
        ਨਤੀਜਾ += digits[((str[pos] & 0x3) << 4) | (static_cast<ਪੂਰਨ>(str[pos + 1] & 0xf0) >> 4)];
        ਨਤੀਜਾ += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<ਪੂਰਨ>(0) >> 6)];
        ਨਤੀਜਾ += '=';
    } ਨਹੀਂਤਾਂ ਜੇ (pos < len) {  // Pad 1
        ਨਤੀਜਾ += digits[((str[pos] >> 2) & 0x3f)];
        ਨਤੀਜਾ += digits[((str[pos] & 0x3) << 4) | (static_cast<ਪੂਰਨ>(0) >> 4)];
        ਨਤੀਜਾ += '=';
        ਨਤੀਜਾ += '=';
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਖਾਲੀ VString::selfTest() {
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

ਸਥਿਰ constexpr uint64_t sha512K[]
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
ਸਥਿਰ uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_ਸ਼ੁੱਧ {
    ਵਾਪਸ lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ਸਥਿਰ ਖਾਲੀ sha512_ਖੰਡ(uint64_t* h, ਅਚਲ uint64_t* chunk) VL_ਸ਼ੁੱਧ {
    uint64_t ah[8];
    ਅਚਲ uint64_t* p = chunk;

    // Initialize working variables to current hash value
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < 5; ++i) {
        ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ j = 0; j < 16; ++j) {
            ਜੇ (i == 0) {
                w[j] = *p++;
            } ਨਹੀਂਤਾਂ {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ਅਚਲ uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ਅਚਲ uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ਅਚਲ uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ਅਚਲ uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ਅਚਲ uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ਅਚਲ uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ਅਚਲ uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ਅਚਲ uint64_t temp2 = s0 + maj;

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
    ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < 8; ++i) h[i] += ah[i];
}

ਖਾਲੀ VHashSha512::ਪਾਓ(ਅਚਲ ਖਾਲੀ* datap, ਮਾਪ_t ਲੰਮਾਈ) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_ਲੰਮਾਈ += ਲੰਮਾਈ;

    string ਆਰਜ਼ੀ_ਅੰਕੜੇ;
    ਪੂਰਨ chunkLen;
    ਅਚਲ uint8_t* chunkp;
    ਜੇ (m_remainder == "") {
        chunkLen = ਲੰਮਾਈ;
        chunkp = static_cast<ਅਚਲ uint8_t*>(datap);
    } ਨਹੀਂਤਾਂ {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        ਆਰਜ਼ੀ_ਅੰਕੜੇ = m_remainder + std::string{static_cast<ਅਚਲ ਅੱਖਰ*>(datap), ਲੰਮਾਈ};
        chunkLen = ਆਰਜ਼ੀ_ਅੰਕੜੇ.ਲੰਮਾਈ();
        chunkp = reinterpret_cast<ਅਚਲ uint8_t*>(ਆਰਜ਼ੀ_ਅੰਕੜੇ.ਅੰਕੜੇ());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    ਪੂਰਨ posBegin = 0;  // Position in buffer for start of this block
    ਪੂਰਨ pos_ਸਿਰਾ = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    ਜਦੋਂ (posBegin <= chunkLen - 128) {
        pos_ਸਿਰਾ = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        ਲਈ (ਪੂਰਨ ਗੇੜਾ_Byte = 0; posBegin < pos_ਸਿਰਾ; posBegin += 8) {
            w[ਗੇੜਾ_Byte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
                              | (static_cast<uint64_t>(chunkp[posBegin + 6]) << 8)
                              | (static_cast<uint64_t>(chunkp[posBegin + 5]) << 16)
                              | (static_cast<uint64_t>(chunkp[posBegin + 4]) << 24)
                              | (static_cast<uint64_t>(chunkp[posBegin + 3]) << 32)
                              | (static_cast<uint64_t>(chunkp[posBegin + 2]) << 40)
                              | (static_cast<uint64_t>(chunkp[posBegin + 1]) << 48)
                              | (static_cast<uint64_t>(chunkp[posBegin]) << 56));
        }
        sha512_ਖੰਡ(m_inthash, w);
    }

    m_remainder = std::string(reinterpret_cast<ਅਚਲ ਅੱਖਰ*>(chunkp + posBegin), chunkLen - pos_ਸਿਰਾ);
}

ਖਾਲੀ VHashSha512::ਪਾਓ_File(ਅਚਲ string& filename) {
    ਸਥਿਰ constexpr ਮਾਪ_t ਬਫਰ_ਮਾਪ = 64 * 1024;

    ਅਚਲ ਪੂਰਨ fd = ::open(filename.c_str(), O_RDONLY);
    ਜੇ (fd < 0) ਵਾਪਸ;

    std::array<ਅੱਖਰ, ਬਫਰ_ਮਾਪ + 1> buf;
    ਜਦੋਂ (ਅਚਲ ssize_t got = ::ਪਠ(fd, &buf, ਬਫਰ_ਮਾਪ)) {
        ਜੇ (got <= 0) ਰੋਕੋ;
        ਪਾਓ(&buf, got);
    }
    ::close(fd);
}

ਖਾਲੀ VHashSha512::finalize() {
    ਜੇ (!m_final) {
        // Make sure no 128 byte blocks left
        ਪਾਓ("");
        m_final = ਸੱਚ;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        ਲਈ (ਪੂਰਨ i = 0; i < 16; ++i) w[i] = 0;
        ਮਾਪ_t ਖੰਡ_Pos = 0;
        ਲਈ (; ਖੰਡ_Pos < m_remainder.ਲੰਮਾਈ(); ++ਖੰਡ_Pos) {
            w[ਖੰਡ_Pos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[ਖੰਡ_Pos])))
                    << ((7 - (ਖੰਡ_Pos & 7)) << 3));
        }
        w[ਖੰਡ_Pos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (ਖੰਡ_Pos & 7)) << 3);
        ਜੇ (m_remainder.ਲੰਮਾਈ() >= 112) {
            sha512_ਖੰਡ(m_inthash, w);
            ਲਈ (ਪੂਰਨ i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_ਲੰਮਾਈ << 3;
        sha512_ਖੰਡ(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ਦੁਆਧਾਰੀ() {
    finalize();
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(64);
    ਲਈ (ਮਾਪ_t i = 0; i < 64; ++i) {
        ਨਤੀਜਾ += static_cast<ਅੱਖਰ>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

uint64_t VHashSha512::digestUInt64() {
    ਅਚਲ string& binhash = digest_ਦੁਆਧਾਰੀ();
    uint64_t ਨਤੀਜਾ = 0;
    ਲਈ (ਮਾਪ_t byte = 0; byte < ਆਕਾਰ(uint64_t); ++byte) {
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੱਖਰ c = binhash[byte];
        ਨਤੀਜਾ = (ਨਤੀਜਾ << 8) | c;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VHashSha512::digestHex() {
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ digits = "0123456789abcdef";
    ਅਚਲ string& binhash = digest_ਦੁਆਧਾਰੀ();
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(128);
    ਲਈ (ਮਾਪ_t byte = 0; byte < 64; ++byte) {
        ਨਤੀਜਾ += digits[(binhash[byte] >> 4) & 0xf];
        ਨਤੀਜਾ += digits[(binhash[byte] >> 0) & 0xf];
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    ਵਾਪਸ VString::base64Enc(digest_ਦੁਆਧਾਰੀ());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ਅਚਲ string& binhash = digest_ਦੁਆਧਾਰੀ();
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(84);
    ਪੂਰਨ pos = 0;
    ਲਈ (; pos < (512 / 8) - 2; pos += 3) {
        ਨਤੀਜਾ += digits[((binhash[pos] >> 2) & 0x3f)];
        ਨਤੀਜਾ += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<ਪੂਰਨ>(binhash[pos + 1] & 0xf0) >> 4)];
        ਨਤੀਜਾ += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<ਪੂਰਨ>(binhash[pos + 2] & 0xc0) >> 6)];
        ਨਤੀਜਾ += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        ਜੇ (ਨਤੀਜਾ.ਮਾਪ() >= DIGEST_SYMBOL24_ਲੰਮਾਈ) ਰੋਕੋ;
    }
    // Any leftover bits don't matter for our purpose
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਖਾਲੀ VHashSha512::selfTestOne(ਅਚਲ string& ਅੰਕੜੇ, ਅਚਲ string& data2, ਅਚਲ string& ਘਾਤਾਂਕ,
                              ਅਚਲ string& exp64, ਅਚਲ string& exp24) {
    VHashSha512 digest{ਅੰਕੜੇ};
    ਜੇ (data2 != "") digest.ਪਾਓ(data2);
    ਜੇ (VL_UNCOVERABLE(digest.digestHex() != ਘਾਤਾਂਕ)) {
        std::cerr << "%Error: When hashing '" << ਅੰਕੜੇ + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << ਘਾਤਾਂਕ << ਸਤਰ_ਅੰਤ;  // LCOV_EXCL_LINE
    }
    ਜੇ (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << ਅੰਕੜੇ + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << ਸਤਰ_ਅੰਤ;  // LCOV_EXCL_LINE
    }
    ਜੇ (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << ਅੰਕੜੇ + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << ਸਤਰ_ਅੰਤ;  // LCOV_EXCL_LINE
    }
}

ਖਾਲੀ VHashSha512::selfTest() {
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

string VName::dehash(ਅਚਲ string& ਵਿੱਚ_2) {
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ VHSH[] = "__Vhsh";
    ਸਥਿਰ constexpr ਮਾਪ_t VHSH_LEN = ਆਕਾਰ(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_ਲੰਮਾਈ;
    ਸਥਿਰ ਅਚਲ ਮਾਪ_t DOT_LEN = std::ਲੜੀ_ਲੰਬਾਈ("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    ਲਈ (string::ਮਾਪ_type ਆਖਰੀ_dot_pos = 0; ਆਖਰੀ_dot_pos < ਵਿੱਚ_2.ਮਾਪ();) {
        ਅਚਲ string::ਮਾਪ_type ਅਗਲਾ_dot_pos = ਵਿੱਚ_2.ਲੱਭੋ("__DOT__", ਆਖਰੀ_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਖੋਜ_begin = std::begin(ਵਿੱਚ_2) + ਆਖਰੀ_dot_pos;
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਖੋਜ_ਸਿਰਾ
            = ਅਗਲਾ_dot_pos == string::npos ? std::ਸਿਰਾ(ਵਿੱਚ_2) : std::begin(ਵਿੱਚ_2) + ਅਗਲਾ_dot_pos;

        // Search for __Vhsh between the two dots.
        ਅਚਲ ਸਵੈਚਾਲਿਤ begin_vhsh
            = std::ਖੋਜ(ਖੋਜ_begin, ਖੋਜ_ਸਿਰਾ, std::begin(VHSH), std::ਸਿਰਾ(VHSH) - 1);
        ਜੇ (begin_vhsh != ਖੋਜ_ਸਿਰਾ) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਿਰਾ_vhsh
                = begin_vhsh + std::min<ਮਾਪ_t>(std::ਦੂਰੀ(begin_vhsh, ਖੋਜ_ਸਿਰਾ), VHSH_LEN);
            ਅਚਲ std::string vhsh{begin_vhsh, ਸਿਰਾ_vhsh};
            ਅਚਲ ਸਵੈਚਾਲਿਤ& it = s_dehashMap.ਲੱਭੋ(vhsh);
            UASSERT(it != s_dehashMap.ਸਿਰਾ(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            ਜੇ (ਆਖਰੀ_dot_pos > 0 && dehashed.ਖਾਲੀ_ਥਾਵਾਂ()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = ਵਿੱਚ_2.substr(0, ਆਖਰੀ_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{ਖੋਜ_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->ਦੂਜਾ;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{ਸਿਰਾ_vhsh, ਖੋਜ_ਸਿਰਾ};
        }
        // This component doesn't need dehashing but a previous one might have.
        ਨਹੀਂਤਾਂ ਜੇ (!dehashed.ਖਾਲੀ_ਥਾਵਾਂ()) {
            dehashed += std::string{ਖੋਜ_begin, ਖੋਜ_ਸਿਰਾ};
        }

        ਜੇ (ਅਗਲਾ_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            ਜੇ (!dehashed.ਖਾਲੀ_ਥਾਵਾਂ()) dehashed += "__DOT__";
            ਆਖਰੀ_dot_pos = ਅਗਲਾ_dot_pos + DOT_LEN;
        } ਨਹੀਂਤਾਂ {
            ਆਖਰੀ_dot_pos = string::npos;
        }
    }
    ਵਾਪਸ dehashed.ਖਾਲੀ_ਥਾਵਾਂ() ? ਵਿੱਚ_2 : dehashed;
}

string VName::hashedName() {
    ਜੇ (m_name == "") ਵਾਪਸ "";
    ਜੇ (m_hashed != "") ਵਾਪਸ m_hashed;  // Memoized
    ਜੇ (s_max_ਲੰਮਾਈ == 0 || m_name.ਲੰਮਾਈ() < s_max_ਲੰਮਾਈ) {
        m_hashed = m_name;
        ਵਾਪਸ m_hashed;
    }
    VHashSha512 hash{m_name};
    ਅਚਲ string suffix = "__Vhsh" + hash.digestSymbol24();
    ਜੇ (s_min_ਲੰਮਾਈ < s_max_ਲੰਮਾਈ) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        ਮਾਪ_t pref_ਲੰਮਾਈ = s_min_ਲੰਮਾਈ;
        ਜਦੋਂ (pref_ਲੰਮਾਈ >= 1 && m_name[pref_ਲੰਮਾਈ - 1] != '_') --pref_ਲੰਮਾਈ;
        s_dehashMap[suffix] = m_name.substr(pref_ਲੰਮਾਈ);
        m_hashed = m_name.substr(0, pref_ਲੰਮਾਈ) + suffix;
    } ਨਹੀਂਤਾਂ {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    ਵਾਪਸ m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::Edit_ਦੂਰੀ VSpellCheck::edit_ਦੂਰੀ(ਅਚਲ string& s, ਅਚਲ string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ਅਚਲ ਮਾਪ_t sLen = s.ਲੰਮਾਈ();
    ਅਚਲ ਮਾਪ_t tLen = t.ਲੰਮਾਈ();
    ਜੇ (sLen == 0) ਵਾਪਸ tLen;
    ਜੇ (tLen == 0) ਵਾਪਸ sLen;
    ਜੇ (sLen >= ਲੰਮਾਈ_ਹੱਦਬੰਦੀ) ਵਾਪਸ sLen;
    ਜੇ (tLen >= ਲੰਮਾਈ_ਹੱਦਬੰਦੀ) ਵਾਪਸ tLen;

    ਸਥਿਰ std::array<Edit_ਦੂਰੀ, ਲੰਮਾਈ_ਹੱਦਬੰਦੀ + 1> s_v_two_ago;
    ਸਥਿਰ std::array<Edit_ਦੂਰੀ, ਲੰਮਾਈ_ਹੱਦਬੰਦੀ + 1> s_v_one_ago;
    ਸਥਿਰ std::array<Edit_ਦੂਰੀ, ਲੰਮਾਈ_ਹੱਦਬੰਦੀ + 1> s_v_ਅਗਲਾ;

    ਲਈ (ਮਾਪ_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    ਲਈ (ਮਾਪ_t i = 0; i < tLen; i++) {
        s_v_ਅਗਲਾ[0] = i + 1;
        ਲਈ (ਮਾਪ_t j = 0; j < sLen; j++) {
            ਅਚਲ Edit_ਦੂਰੀ ਲਾਗਤ = (s[j] == t[i] ? 0 : 1);
            ਅਚਲ Edit_ਦੂਰੀ deletion = s_v_ਅਗਲਾ[j] + 1;
            ਅਚਲ Edit_ਦੂਰੀ insertion = s_v_one_ago[j + 1] + 1;
            ਅਚਲ Edit_ਦੂਰੀ substitution = s_v_one_ago[j] + ਲਾਗਤ;
            Edit_ਦੂਰੀ cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            ਜੇ (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ਅਚਲ Edit_ਦੂਰੀ transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_ਅਗਲਾ[j + 1] = cheapest;
        }
        ਲਈ (ਮਾਪ_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_ਅਗਲਾ[j];
        }
    }

    ਅਚਲ Edit_ਦੂਰੀ ਨਤੀਜਾ = s_v_ਅਗਲਾ[sLen];
    ਵਾਪਸ ਨਤੀਜਾ;
}

VSpellCheck::Edit_ਦੂਰੀ VSpellCheck::cutoff_ਦੂਰੀ(ਮਾਪ_t goal_len, ਮਾਪ_t candidate_len) {
    // Return max acceptable edit distance
    ਅਚਲ ਮਾਪ_t max_ਲੰਮਾਈ_2 = std::max(goal_len, candidate_len);
    ਅਚਲ ਮਾਪ_t min_ਲੰਮਾਈ = std::min(goal_len, candidate_len);
    ਜੇ (max_ਲੰਮਾਈ_2 <= 1) ਵਾਪਸ 0;
    ਜੇ (max_ਲੰਮਾਈ_2 - min_ਲੰਮਾਈ <= 1) ਵਾਪਸ std::max(max_ਲੰਮਾਈ_2 / 3, static_cast<ਮਾਪ_t>(1));
    ਵਾਪਸ (max_ਲੰਮਾਈ_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::ਉੱਤਮ_Candidate_Info(ਅਚਲ string& goal,
                                                      Edit_ਦੂਰੀ& distancer) ਅਚਲ {
    Candidate ਉੱਤਮ;
    ਅਚਲ ਮਾਪ_t gLen = goal.ਲੰਮਾਈ();
    distancer = ਲੰਮਾਈ_ਹੱਦਬੰਦੀ * 10;
    ਲਈ (ਅਚਲ Candidate& candidate : m_candidates) {
        ਅਚਲ ਮਾਪ_t cLen = candidate.str.ਲੰਮਾਈ();

        // Min distance must be inserting/deleting to make lengths match
        ਅਚਲ Edit_ਦੂਰੀ min_ਦੂਰੀ = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        ਜੇ (min_ਦੂਰੀ >= distancer) ਜਾਰੀ;  // Short-circuit if already better

        ਅਚਲ Edit_ਦੂਰੀ cutoff = cutoff_ਦੂਰੀ(gLen, cLen);
        ਜੇ (min_ਦੂਰੀ > cutoff) ਜਾਰੀ;  // Short-circuit if already too bad

        ਅਚਲ Edit_ਦੂਰੀ dist = edit_ਦੂਰੀ(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.ਸੁਨੇਹਾ_2
                                      << "'");
        ਜੇ (dist < distancer && dist <= cutoff) {
            distancer = dist;
            ਉੱਤਮ = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    ਜੇ (distancer == 0) ਵਾਪਸ Candidate{.str = ਉੱਤਮ.str, .ਸੁਨੇਹਾ_2 = ""};
    ਵਾਪਸ ਉੱਤਮ;
}

ਖਾਲੀ VSpellCheck::self_Test_ਦੂਰੀ_One(ਅਚਲ string& a, ਅਚਲ string& b, Edit_ਦੂਰੀ expected) {
    UASSERT_SELFTEST(edit_ਦੂਰੀ(a, b), expected);
    UASSERT_SELFTEST(edit_ਦੂਰੀ(b, a), expected);
}

ਖਾਲੀ VSpellCheck::selfTestSuggestOne(ਤਾਰਕਿਕ matches, ਅਚਲ string& c, ਅਚਲ string& goal,
                                     Edit_ਦੂਰੀ dist) {
    Edit_ਦੂਰੀ gdist;
    VSpellCheck speller;
    speller.ਧੱਕੋ_Candidate(c);
    ਅਚਲ string got = speller.ਉੱਤਮ_Candidate_Info(goal, gdist /*ref*/).str;
    ਜੇ (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } ਨਹੀਂਤਾਂ {
        UASSERT_SELFTEST(got, "");
    }
}

ਖਾਲੀ VSpellCheck::selfTest() {
    {
        self_Test_ਦੂਰੀ_One("ab", "ac", 1);
        self_Test_ਦੂਰੀ_One("ab", "a", 1);
        self_Test_ਦੂਰੀ_One("a", "b", 1);
    }
    {
        selfTestSuggestOne(ਸੱਚ, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(ਸੱਚ, "abcdef", "acbdef", 1);
        selfTestSuggestOne(ਸੱਚ, "db", "dc", 1);
        selfTestSuggestOne(ਸੱਚ, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(ਝੂਠ, "x", "y", 1);
        selfTestSuggestOne(ਝੂਠ, "sqrt", "assert", 3);
    }
    {
        ਅਚਲ VSpellCheck speller;
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ਧੱਕੋ_Candidate("fred");
        speller.ਧੱਕੋ_Candidate("wilma");
        speller.ਧੱਕੋ_Candidate("barney");
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < ਅੰਕ_CANDIDATE_ਹੱਦਬੰਦੀ; ++i) speller.ਧੱਕੋ_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.ਮਾਪ(), ਅੰਕ_CANDIDATE_ਹੱਦਬੰਦੀ);
        speller.ਧੱਕੋ_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.ਮਾਪ(), ਅੰਕ_CANDIDATE_ਹੱਦਬੰਦੀ);
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.ਉੱਤਮ_Candidate("wilma").str, "");
    }
}
