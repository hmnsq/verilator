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

#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
#ಅನ್ತರ್ಭಾವ <io.h>  // open, read, write, close
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
#ಅನ್ತರ್ಭಾವ "V3Global.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <fcntl.h>

ಗಾತ್ರ_t VName::s_min_ಉದ್ದಳತೆ = 32;
ಗಾತ್ರ_t VName::s_max_ಉದ್ದಳತೆ = 0;  // Disabled
std::map<string, string> VName::s_dehashMap;

//######################################################################
// Wildcard

// Double procedures, inlined, unrolls loop much better
ಮಾದರಿ <ತಾರ್ಕಿಕ Even = ನಿಜ>
ಸ್ಥಿರ ತಾರ್ಕಿಕ wild_ಹೊಂದಿಕೆ_Impl(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* s, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p) VL_ಶುದ್ಧ {
    ಗಾಗಿ (; *p; s++, p++) {
        ಆದರೆ (*p != '*') {
            ಆದರೆ (((*s) != (*p)) && *p != '?') ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Trailing star matches everything.
            ಆದರೆ (!*++p) ಹಿಂತಿರುಗಿ ನಿಜ;
            ಆಗ (!wild_ಹೊಂದಿಕೆ_Impl<!Even>(s, p)) {
                ಆದರೆ (*++s == '\0') ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }
    }
    ಹಿಂತಿರುಗಿ (*s == '\0');
}

ತಾರ್ಕಿಕ VString::wildmatch(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* s, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p) VL_ಶುದ್ಧ {
    ಆದರೆ (*s == '\0') {
        ಆಗ (*p == '*') ++p;
        ಹಿಂತಿರುಗಿ *p == '\0';
    }
    ಹಿಂತಿರುಗಿ wild_ಹೊಂದಿಕೆ_Impl(s, p);
}

ತಾರ್ಕಿಕ VString::wildmatch(ಸ್ಥಿರಾಂಕ string& s, ಸ್ಥಿರಾಂಕ string& p) VL_ಶುದ್ಧ {
    ಹಿಂತಿರುಗಿ wildmatch(s.c_str(), p.c_str());
}

string VString::dot(ಸ್ಥಿರಾಂಕ string& a, ಸ್ಥಿರಾಂಕ string& dot, ಸ್ಥಿರಾಂಕ string& b) {
    ಆದರೆ (b == "") ಹಿಂತಿರುಗಿ a;
    ಆದರೆ (a == "") ಹಿಂತಿರುಗಿ b;
    ಹಿಂತಿರುಗಿ a + dot + b;
}

string VString::downcase(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ {
    string ಫಲಿತಾಂಶ = str;
    ಗಾಗಿ (ಅಕ್ಷರ& cr : ಫಲಿತಾಂಶ) cr = std::tolower(cr);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::upcase(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ {
    string ಫಲಿತಾಂಶ = str;
    ಗಾಗಿ (ಅಕ್ಷರ& cr : ಫಲಿತಾಂಶ) cr = std::ದೊಡ್ಡ_ಅಕ್ಷರ(cr);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::quoteAny(ಸ್ಥಿರಾಂಕ string& str, ಅಕ್ಷರ tgt, ಅಕ್ಷರ esc) VL_ಶುದ್ಧ {
    string ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (c == tgt) ಫಲಿತಾಂಶ += esc;
        ಫಲಿತಾಂಶ += c;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::dequotePercent(ಸ್ಥಿರಾಂಕ string& str) {
    string ಫಲಿತಾಂಶ;
    ಅಕ್ಷರ ಕೊನೆಯ = '\0';
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (ಕೊನೆಯ == '%' && c == '%') {
            ಕೊನೆಯ = '\0';
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಫಲಿತಾಂಶ += c;
            ಕೊನೆಯ = c;
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::quoteStringLiteralForShell(ಸ್ಥಿರಾಂಕ string& str) {
    string ಫಲಿತಾಂಶ;
    constexpr ಅಕ್ಷರ dquote = '"';
    constexpr ಅಕ್ಷರ escape = '\\';
    ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(dquote);  // Start quoted string
    ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(escape);
    ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(dquote);  // "
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (c == dquote || c == escape) ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(escape);
        ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(c);
    }
    ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(escape);
    ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(dquote);  // "
    ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(dquote);  // Terminate quoted string
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::escape_String_For_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& str) {
    ಆದರೆ (str.ಕಂಡುಹಿಡಿ(R"(\\)") != string::npos)
        ಹಿಂತಿರುಗಿ str;  // if it has been escaped already, don't do it again
    ಆದರೆ (str.ಕಂಡುಹಿಡಿ('/') != string::npos) ಹಿಂತಿರುಗಿ str;  // can be replaced by `__MINGW32__` or `_WIN32`
    string ಫಲಿತಾಂಶ;
    constexpr ಅಕ್ಷರ space = ' ';  // escape space like this `Program Files`
    constexpr ಅಕ್ಷರ escape = '\\';
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (c == space || c == escape) ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(escape);
        ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(c);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ vl_decodexdigit(ಅಕ್ಷರ c) {
    ಹಿಂತಿರುಗಿ std::ಅಂಕಿಯೇ(c) ? c - '0' : std::tolower(c) - 'a' + 10;
}

string VString::unquoteSVString(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, string& err_ಹೊರಹೋಗು) {
    ತಾರ್ಕಿಕ quoted = ಸುಳ್ಳು;
    string newtext;
    newtext.reserve(ಪಠ್ಯ.ಗಾತ್ರ_2());
    ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ octal_val = 0;
    ಪೂರ್ಣಾಂಕ octal_digits = 0;
    ಗಾಗಿ (string::const_iterator cp = ಪಠ್ಯ.begin(); cp != ಪಠ್ಯ.ಕೊನೆ(); ++cp) {
        ಆದರೆ (quoted) {
            ಆದರೆ (std::ಅಂಕಿಯೇ(*cp)) {
                octal_val = octal_val * 8 + (*cp - '0');
                ಆದರೆ (++octal_digits == 3) {
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ಸುಳ್ಳು;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (octal_digits) {
                    // Spec allows 1-3 digits
                    newtext += octal_val;
                    octal_digits = 0;
                    octal_val = 0;
                    quoted = ಸುಳ್ಳು;
                    --cp;  // Backup to reprocess terminating character as non-escaped
                    ಮುಂದುವರಿ;
                }
                quoted = ಸುಳ್ಳು;
                ಆದರೆ (*cp == 'n') {
                    newtext += '\n';
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == 'a') {
                    newtext += '\a';  // SystemVerilog 3.1
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == 'f') {
                    newtext += '\f';  // SystemVerilog 3.1
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == 'r') {
                    newtext += '\r';
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == 't') {
                    newtext += '\t';
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == 'v') {
                    newtext += '\v';  // SystemVerilog 3.1
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == 'x' && std::isxdigit(cp[1])
                           && std::isxdigit(cp[2])) {  // SystemVerilog 3.1
                    newtext
                        += static_cast<ಅಕ್ಷರ>(16 * vl_decodexdigit(cp[1]) + vl_decodexdigit(cp[2]));
                    cp += 2;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(*cp)) {
                    err_ಹೊರಹೋಗು = "Unknown escape sequence: \\";
                    err_ಹೊರಹೋಗು += *cp;
                    ನಿಲ್ಲಿಸು;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    newtext += *cp;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*cp == '\\') {
            ಆದರೆ (octal_digits) {
                newtext += octal_val;
                // below: octal_digits = 0;
                octal_val = 0;
            }
            quoted = ನಿಜ;
            octal_digits = 0;
        } ಇಲ್ಲದಿದ್ದರೆ {
            newtext += *cp;
        }
    }
    ಹಿಂತಿರುಗಿ newtext;
}

ಗಾತ್ರ_t VString::quoted_ಕೊನೆ(ಸ್ಥಿರಾಂಕ string& str, ಗಾತ್ರ_t pos) VL_ಶುದ್ಧ {
    ಗಾಗಿ (ಗಾತ್ರ_t i = pos + 1; i < str.ಗಾತ್ರ_2(); ++i) {
        ಆದರೆ (str[i] == '\\') {
            ++i;  // Past the escaped character, as of '\"'
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (str[i] == '"') {
            ಹಿಂತಿರುಗಿ i + 1;
        }
    }
    ಹಿಂತಿರುಗಿ string::npos;
}

string VString::spaceUnprintable(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ {
    string ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (std::isprint(c)) {
            ಫಲಿತಾಂಶ += c;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಫಲಿತಾಂಶ += ' ';
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::removeWhitespace(ಸ್ಥಿರಾಂಕ string& str) {
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(str.ಗಾತ್ರ_2());
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (!std::ಖಾಲಿಯೇ(c)) ಫಲಿತಾಂಶ += c;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::trimWhitespace(ಸ್ಥಿರಾಂಕ string& str) {
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(str.ಗಾತ್ರ_2());
    string ಕೂಡಿಸು;
    ತಾರ್ಕಿಕ newline = ಸುಳ್ಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (newline && std::ಖಾಲಿಯೇ(c)) ಮುಂದುವರಿ;
        ಆದರೆ (c == '\n') {
            ಕೂಡಿಸು = "\n";
            newline = ನಿಜ;
            ಮುಂದುವರಿ;
        }
        ಆದರೆ (std::ಖಾಲಿಯೇ(c)) {
            ಕೂಡಿಸು += c;
            ಮುಂದುವರಿ;
        }
        ಆದರೆ (!ಕೂಡಿಸು.ಖಾಲಿಗಳು()) {
            ಫಲಿತಾಂಶ += ಕೂಡಿಸು;
            newline = ಸುಳ್ಳು;
            ಕೂಡಿಸು.clear();
        }
        ಫಲಿತಾಂಶ += c;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ತಾರ್ಕಿಕ VString::isIdentifier(ಸ್ಥಿರಾಂಕ string& str) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (!isIdentifierChar(c)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

ತಾರ್ಕಿಕ VString::isWhitespace(ಸ್ಥಿರಾಂಕ string& str) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ಆದರೆ (!std::ಖಾಲಿಯೇ(c)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

string::ಗಾತ್ರ_type VString::leading_Whitespace_ಎಣಿಕೆ(ಸ್ಥಿರಾಂಕ string& str) {
    string::ಗಾತ್ರ_type ಫಲಿತಾಂಶ = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c : str) {
        ++ಫಲಿತಾಂಶ;
        ಆದರೆ (!std::ಖಾಲಿಯೇ(c)) ನಿಲ್ಲಿಸು;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ದ್ವಿಗುಣ VString::parseDouble(ಸ್ಥಿರಾಂಕ string& str, ತಾರ್ಕಿಕ* successp) {
    ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ strgp = ಹೊಸ ಅಕ್ಷರ[str.ಗಾತ್ರ_2() + 1];
    ಅಕ್ಷರ* dp = strgp;
    ಆದರೆ (successp) *successp = ನಿಜ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sp = str.c_str(); *sp; ++sp) {
        ಆದರೆ (*sp != '_') *dp++ = *sp;
    }
    *dp++ = '\0';
    ಅಕ್ಷರ* endp = strgp;
    ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ d = strtod(strgp, &endp);
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t parsed_len = endp - strgp;
    ಆದರೆ (parsed_len != std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(strgp)) {
        ಆದರೆ (successp) *successp = ಸುಳ್ಳು;
    }
    VL_DO_DANGLING(ಅಳಿಸು[] strgp, strgp);
    ಹಿಂತಿರುಗಿ d;
}

string VString::replaceSubstr(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& from, ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ) {
    string ಫಲಿತಾಂಶ = str;
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t fromLen = from.ಗಾತ್ರ_2();
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಗಮ್ಯ_Len = ಗಮ್ಯ.ಗಾತ್ರ_2();
    UASSERT_STATIC(fromLen > 0, "Cannot replace empty string");
    ಗಾಗಿ (ಗಾತ್ರ_t pos = 0; (pos = ಫಲಿತಾಂಶ.ಕಂಡುಹಿಡಿ(from, pos)) != string::npos; pos += ಗಮ್ಯ_Len) {
        ಫಲಿತಾಂಶ.replace(pos, fromLen, ಗಮ್ಯ);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VString::replace_ಶಬ್ದ(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& from, ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ) {
    string ಫಲಿತಾಂಶ = str;
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t len = from.ಗಾತ್ರ_2();
    UASSERT_STATIC(len > 0, "Cannot replace empty string");
    ಗಾಗಿ (ಗಾತ್ರ_t pos = 0; (pos = ಫಲಿತಾಂಶ.ಕಂಡುಹಿಡಿ(from, pos)) != string::npos; pos += len) {
        // Only replace whole words
        ಆದರೆ (((pos > 0) && VString::isIdentifierChar(ಫಲಿತಾಂಶ[pos - 1])) ||  //
            ((pos + len < ಫಲಿತಾಂಶ.ಗಾತ್ರ_2()) && VString::isIdentifierChar(ಫಲಿತಾಂಶ[pos + len]))) {
            ಮುಂದುವರಿ;
        }
        ಫಲಿತಾಂಶ.replace(pos, len, ಗಮ್ಯ);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

std::deque<string> VString::ವಿಭಜಿಸು(ಸ್ಥಿರಾಂಕ string& str, ಅಕ್ಷರ delimiter) {
    std::deque<std::string> results;
    std::istringstream is{str};
    std::string token;
    ಆಗ (std::getline(is, token, delimiter)) results.ತಳ್ಳು_ಹಿಂಭಾಗ(token);
    ಹಿಂತಿರುಗಿ results;
}

ತಾರ್ಕಿಕ VString::ತುದಿಗಳು_With(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& suffix) {
    ಆದರೆ (str.ಉದ್ದಳತೆ() < suffix.ಉದ್ದಳತೆ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ str.compare(str.ಉದ್ದಳತೆ() - suffix.ಉದ್ದಳತೆ(), suffix.ಉದ್ದಳತೆ(), suffix) == 0;
}
string VString::aOrAn(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಶಬ್ದ) {
    ಆಯ್ಕೆ (ಶಬ್ದ[0]) {
    ಸ್ಥಿತಿ '\0': ಹಿಂತಿರುಗಿ "";
    ಸ್ಥಿತಿ 'a':
    ಸ್ಥಿತಿ 'e':
    ಸ್ಥಿತಿ 'i':
    ಸ್ಥಿತಿ 'o':
    ಸ್ಥಿತಿ 'u': ಹಿಂತಿರುಗಿ "an";
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "a";
    }
}

// MurmurHash64A
uint64_t VString::hashMurmur(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ {
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಕೀಲಿ = str.c_str();
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t len = str.ಗಾತ್ರ_2();
    constexpr uint64_t ಬೀಜ = 0;
    constexpr uint64_t m = 0xc6a4a7935bd1e995ULL;
    constexpr ಪೂರ್ಣಾಂಕ r = 47;

    uint64_t h = ಬೀಜ ^ (len * m);

    ಸ್ಥಿರಾಂಕ uint64_t* ದತ್ತಾಂಶ = reinterpret_cast<ಸ್ಥಿರಾಂಕ uint64_t*>(ಕೀಲಿ);
    ಸ್ಥಿರಾಂಕ uint64_t* ಕೊನೆ = ದತ್ತಾಂಶ + (len / 8);

    ಆಗ (ದತ್ತಾಂಶ != ಕೊನೆ) {
        uint64_t k = *ದತ್ತಾಂಶ++;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;
    }

    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ* data2 = reinterpret_cast<ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ*>(ದತ್ತಾಂಶ);

    ಆಯ್ಕೆ (len & 7) {
    ಸ್ಥಿತಿ 7: h ^= static_cast<uint64_t>(data2[6]) << 48;  // FALLTHRU
    ಸ್ಥಿತಿ 6: h ^= static_cast<uint64_t>(data2[5]) << 40;  // FALLTHRU
    ಸ್ಥಿತಿ 5: h ^= static_cast<uint64_t>(data2[4]) << 32;  // FALLTHRU
    ಸ್ಥಿತಿ 4: h ^= static_cast<uint64_t>(data2[3]) << 24;  // FALLTHRU
    ಸ್ಥಿತಿ 3: h ^= static_cast<uint64_t>(data2[2]) << 16;  // FALLTHRU
    ಸ್ಥಿತಿ 2: h ^= static_cast<uint64_t>(data2[1]) << 8;  // FALLTHRU
    ಸ್ಥಿತಿ 1: h ^= static_cast<uint64_t>(data2[0]); h *= m;  // FALLTHRU
    ಪೂರ್ವನಿಯೋಜಿತ:;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    ಹಿಂತಿರುಗಿ h;
}

string VString::base64Enc(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ {
    // Non-URL format (+/), versus URL format (-/).
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    ಗಾತ್ರ_t len = str.ಗಾತ್ರ_2();
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(len * 4 / 3 + 2);
    ಗಾತ್ರ_t pos = 0;
    ಆದರೆ (len >= 3) {
        ಗಾಗಿ (; pos < len - 2; pos += 3) {
            ಫಲಿತಾಂಶ += digits[((str[pos] >> 2) & 0x3f)];
            ಫಲಿತಾಂಶ
                += digits[((str[pos] & 0x3) << 4) | (static_cast<ಪೂರ್ಣಾಂಕ>(str[pos + 1] & 0xf0) >> 4)];
            ಫಲಿತಾಂಶ += digits[((str[pos + 1] & 0xf) << 2)
                             | (static_cast<ಪೂರ್ಣಾಂಕ>(str[pos + 2] & 0xc0) >> 6)];
            ಫಲಿತಾಂಶ += digits[((str[pos + 2] & 0x3f))];
        }
    }
    ಆದರೆ (len >= 2 && pos < len - 1) {  // Pad 2
        ಫಲಿತಾಂಶ += digits[((str[pos] >> 2) & 0x3f)];
        ಫಲಿತಾಂಶ += digits[((str[pos] & 0x3) << 4) | (static_cast<ಪೂರ್ಣಾಂಕ>(str[pos + 1] & 0xf0) >> 4)];
        ಫಲಿತಾಂಶ += digits[((str[pos + 1] & 0xf) << 2) | (static_cast<ಪೂರ್ಣಾಂಕ>(0) >> 6)];
        ಫಲಿತಾಂಶ += '=';
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos < len) {  // Pad 1
        ಫಲಿತಾಂಶ += digits[((str[pos] >> 2) & 0x3f)];
        ಫಲಿತಾಂಶ += digits[((str[pos] & 0x3) << 4) | (static_cast<ಪೂರ್ಣಾಂಕ>(0) >> 4)];
        ಫಲಿತಾಂಶ += '=';
        ಫಲಿತಾಂಶ += '=';
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
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
ಸ್ಥಿರ uint64_t shaRotr64(uint64_t lhs, uint64_t rhs) VL_ಶುದ್ಧ {
    ಹಿಂತಿರುಗಿ lhs >> rhs | lhs << (64 - rhs);
}

VL_ATTR_ALWINLINE
ಸ್ಥಿರ ಶೂನ್ಯ sha512_ಖಂಡ(uint64_t* h, ಸ್ಥಿರಾಂಕ uint64_t* chunk) VL_ಶುದ್ಧ {
    uint64_t ah[8];
    ಸ್ಥಿರಾಂಕ uint64_t* p = chunk;

    // Initialize working variables to current hash value
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < 8; i++) ah[i] = h[i];
    // Compression function main loop
    uint64_t w[16] = {};
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < 5; ++i) {
        ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ j = 0; j < 16; ++j) {
            ಆದರೆ (i == 0) {
                w[j] = *p++;
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Extend the first 16 words into the remaining
                // 64 words w[16..79] of the message schedule array:
                ಸ್ಥಿರಾಂಕ uint64_t s0 = shaRotr64(w[(j + 1) & 0xf], 1) ^ shaRotr64(w[(j + 1) & 0xf], 8)
                                    ^ (w[(j + 1) & 0xf] >> 7);
                ಸ್ಥಿರಾಂಕ uint64_t s1 = shaRotr64(w[(j + 14) & 0xf], 19)
                                    ^ shaRotr64(w[(j + 14) & 0xf], 61) ^ (w[(j + 14) & 0xf] >> 6);
                w[j] = w[j] + s0 + w[(j + 9) & 0xf] + s1;
            }
            ಸ್ಥಿರಾಂಕ uint64_t s1 = shaRotr64(ah[4], 14) ^ shaRotr64(ah[4], 18) ^ shaRotr64(ah[4], 41);
            ಸ್ಥಿರಾಂಕ uint64_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
            ಸ್ಥಿರಾಂಕ uint64_t temp1 = ah[7] + s1 + ch + sha512K[i * 16 + j] + w[j];
            ಸ್ಥಿರಾಂಕ uint64_t s0 = shaRotr64(ah[0], 28) ^ shaRotr64(ah[0], 34) ^ shaRotr64(ah[0], 39);
            ಸ್ಥಿರಾಂಕ uint64_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
            ಸ್ಥಿರಾಂಕ uint64_t temp2 = s0 + maj;

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
    ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < 8; ++i) h[i] += ah[i];
}

ಶೂನ್ಯ VHashSha512::ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* datap, ಗಾತ್ರ_t ಉದ್ದಳತೆ) {
    UASSERT(!m_final, "Called VHashSha512::insert after finalized the hash value");
    m_tot_ಉದ್ದಳತೆ += ಉದ್ದಳತೆ;

    string ತಾತ್ಕಾಲಿಕ_ದತ್ತಾಂಶ;
    ಪೂರ್ಣಾಂಕ chunkLen;
    ಸ್ಥಿರಾಂಕ uint8_t* chunkp;
    ಆದರೆ (m_remainder == "") {
        chunkLen = ಉದ್ದಳತೆ;
        chunkp = static_cast<ಸ್ಥಿರಾಂಕ uint8_t*>(datap);
    } ಇಲ್ಲದಿದ್ದರೆ {
        // If there are large inserts it would be more efficient to avoid this copy
        // by copying bytes in the loop below from either m_remainder or the data
        // as appropriate.
        ತಾತ್ಕಾಲಿಕ_ದತ್ತಾಂಶ = m_remainder + std::string{static_cast<ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*>(datap), ಉದ್ದಳತೆ};
        chunkLen = ತಾತ್ಕಾಲಿಕ_ದತ್ತಾಂಶ.ಉದ್ದಳತೆ();
        chunkp = reinterpret_cast<ಸ್ಥಿರಾಂಕ uint8_t*>(ತಾತ್ಕಾಲಿಕ_ದತ್ತಾಂಶ.ದತ್ತಾಂಶ());
    }

    // See wikipedia SHA-2 algorithm summary
    uint64_t w[80];  // Round buffer, [0..15] are input data, rest used by rounds
    ಪೂರ್ಣಾಂಕ posBegin = 0;  // Position in buffer for start of this block
    ಪೂರ್ಣಾಂಕ pos_ಕೊನೆ = 0;  // Position in buffer for end of this block

    // Process complete 128-byte blocks
    ಆಗ (posBegin <= chunkLen - 128) {
        pos_ಕೊನೆ = posBegin + 128;
        // 128 byte round input data, being careful to swap on big, keep on little
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಸುತ್ತು_Byte = 0; posBegin < pos_ಕೊನೆ; posBegin += 8) {
            w[ಸುತ್ತು_Byte++] = (static_cast<uint64_t>(chunkp[posBegin + 7])
                              | (static_cast<uint64_t>(chunkp[posBegin + 6]) << 8)
                              | (static_cast<uint64_t>(chunkp[posBegin + 5]) << 16)
                              | (static_cast<uint64_t>(chunkp[posBegin + 4]) << 24)
                              | (static_cast<uint64_t>(chunkp[posBegin + 3]) << 32)
                              | (static_cast<uint64_t>(chunkp[posBegin + 2]) << 40)
                              | (static_cast<uint64_t>(chunkp[posBegin + 1]) << 48)
                              | (static_cast<uint64_t>(chunkp[posBegin]) << 56));
        }
        sha512_ಖಂಡ(m_inthash, w);
    }

    m_remainder = std::string(reinterpret_cast<ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*>(chunkp + posBegin), chunkLen - pos_ಕೊನೆ);
}

ಶೂನ್ಯ VHashSha512::ಸೇರಿಸು_File(ಸ್ಥಿರಾಂಕ string& filename) {
    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t ಬಫರ್_ಗಾತ್ರ = 64 * 1024;

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ fd = ::open(filename.c_str(), O_RDONLY);
    ಆದರೆ (fd < 0) ಹಿಂತಿರುಗಿ;

    std::array<ಅಕ್ಷರ, ಬಫರ್_ಗಾತ್ರ + 1> buf;
    ಆಗ (ಸ್ಥಿರಾಂಕ ssize_t got = ::ಪಠ(fd, &buf, ಬಫರ್_ಗಾತ್ರ)) {
        ಆದರೆ (got <= 0) ನಿಲ್ಲಿಸು;
        ಸೇರಿಸು(&buf, got);
    }
    ::close(fd);
}

ಶೂನ್ಯ VHashSha512::finalize() {
    ಆದರೆ (!m_final) {
        // Make sure no 128 byte blocks left
        ಸೇರಿಸು("");
        m_final = ನಿಜ;

        // Process final possibly non-complete 128-byte block
        uint64_t w[16];  // Round buffer, [0..15] are input data
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < 16; ++i) w[i] = 0;
        ಗಾತ್ರ_t ಖಂಡ_Pos = 0;
        ಗಾಗಿ (; ಖಂಡ_Pos < m_remainder.ಉದ್ದಳತೆ(); ++ಖಂಡ_Pos) {
            w[ಖಂಡ_Pos >> 3]
                |= ((static_cast<uint64_t>(static_cast<uint8_t>(m_remainder[ಖಂಡ_Pos])))
                    << ((7 - (ಖಂಡ_Pos & 7)) << 3));
        }
        w[ಖಂಡ_Pos >> 3] |= static_cast<uint64_t>(0x80) << ((7 - (ಖಂಡ_Pos & 7)) << 3);
        ಆದರೆ (m_remainder.ಉದ್ದಳತೆ() >= 112) {
            sha512_ಖಂಡ(m_inthash, w);
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < 16; ++i) w[i] = 0;
        }
        // Only supporting 2^61 bytes max
        w[14] = 0;
        w[15] = m_tot_ಉದ್ದಳತೆ << 3;
        sha512_ಖಂಡ(m_inthash, w);

        m_remainder.clear();
    }
}

string VHashSha512::digest_ದ್ವಿಮಾನ() {
    finalize();
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(64);
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < 64; ++i) {
        ಫಲಿತಾಂಶ += static_cast<ಅಕ್ಷರ>((m_inthash[i >> 3] >> (((7 - i) & 0x7) << 3)) & 0xff);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

uint64_t VHashSha512::digestUInt64() {
    ಸ್ಥಿರಾಂಕ string& binhash = digest_ದ್ವಿಮಾನ();
    uint64_t ಫಲಿತಾಂಶ = 0;
    ಗಾಗಿ (ಗಾತ್ರ_t byte = 0; byte < ಗಾತ್ರ(uint64_t); ++byte) {
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಅಕ್ಷರ c = binhash[byte];
        ಫಲಿತಾಂಶ = (ಫಲಿತಾಂಶ << 8) | c;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VHashSha512::digestHex() {
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ digits = "0123456789abcdef";
    ಸ್ಥಿರಾಂಕ string& binhash = digest_ದ್ವಿಮಾನ();
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(128);
    ಗಾಗಿ (ಗಾತ್ರ_t byte = 0; byte < 64; ++byte) {
        ಫಲಿತಾಂಶ += digits[(binhash[byte] >> 4) & 0xf];
        ಫಲಿತಾಂಶ += digits[(binhash[byte] >> 0) & 0xf];
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

string VHashSha512::digestBase64() {
    // Return base64 from hash.  Complete representation of the binary/reversable.
    ಹಿಂತಿರುಗಿ VString::base64Enc(digest_ದ್ವಿಮಾನ());
}

string VHashSha512::digestSymbol24() {
    // Make a symbol name from hash.  Similar to base64, however base 64
    // has + and / for last two digits, but need C symbol, and we also
    // avoid conflicts with use of _, so use "AB" at the end.
    // Thus this function is non-reversible.
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ digits
        = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789AB";
    ಸ್ಥಿರಾಂಕ string& binhash = digest_ದ್ವಿಮಾನ();
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(84);
    ಪೂರ್ಣಾಂಕ pos = 0;
    ಗಾಗಿ (; pos < (512 / 8) - 2; pos += 3) {
        ಫಲಿತಾಂಶ += digits[((binhash[pos] >> 2) & 0x3f)];
        ಫಲಿತಾಂಶ += digits[((binhash[pos] & 0x3) << 4)
                         | (static_cast<ಪೂರ್ಣಾಂಕ>(binhash[pos + 1] & 0xf0) >> 4)];
        ಫಲಿತಾಂಶ += digits[((binhash[pos + 1] & 0xf) << 2)
                         | (static_cast<ಪೂರ್ಣಾಂಕ>(binhash[pos + 2] & 0xc0) >> 6)];
        ಫಲಿತಾಂಶ += digits[((binhash[pos + 2] & 0x3f))];
        // Keep symbols short-ish, with 24 chars/144 bits we won't have hash collisions
        ಆದರೆ (ಫಲಿತಾಂಶ.ಗಾತ್ರ_2() >= DIGEST_SYMBOL24_ಉದ್ದಳತೆ) ನಿಲ್ಲಿಸು;
    }
    // Any leftover bits don't matter for our purpose
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಶೂನ್ಯ VHashSha512::selfTestOne(ಸ್ಥಿರಾಂಕ string& ದತ್ತಾಂಶ, ಸ್ಥಿರಾಂಕ string& data2, ಸ್ಥಿರಾಂಕ string& ಘಾತಾಂಕ,
                              ಸ್ಥಿರಾಂಕ string& exp64, ಸ್ಥಿರಾಂಕ string& exp24) {
    VHashSha512 digest{ದತ್ತಾಂಶ};
    ಆದರೆ (data2 != "") digest.ಸೇರಿಸು(data2);
    ಆದರೆ (VL_UNCOVERABLE(digest.digestHex() != ಘಾತಾಂಕ)) {
        std::cerr << "%Error: When hashing '" << ದತ್ತಾಂಶ + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestHex() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << ಘಾತಾಂಕ << ಸಾಲು_ಅಂತ್ಯ;  // LCOV_EXCL_LINE
    }
    ಆದರೆ (VL_UNCOVERABLE(digest.digestBase64() != exp64)) {
        std::cerr << "%Error: When hashing '" << ದತ್ತಾಂಶ + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestBase64() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << ಸಾಲು_ಅಂತ್ಯ;  // LCOV_EXCL_LINE
    }
    ಆದರೆ (VL_UNCOVERABLE(digest.digestSymbol24() != exp24)) {
        std::cerr << "%Error: When hashing '" << ದತ್ತಾಂಶ + data2 << "'\n"  // LCOV_EXCL_LINE
                  << "        ... got=" << digest.digestSymbol24() << '\n'  // LCOV_EXCL_LINE
                  << "        ... exp=" << exp64 << ಸಾಲು_ಅಂತ್ಯ;  // LCOV_EXCL_LINE
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

string VName::dehash(ಸ್ಥಿರಾಂಕ string& ಒಳ) {
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ VHSH[] = "__Vhsh";
    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t VHSH_LEN = ಗಾತ್ರ(VHSH) - 1 + VHashSha512::DIGEST_SYMBOL24_ಉದ್ದಳತೆ;
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t DOT_LEN = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__DOT__");
    std::string dehashed;

    // Need to split 'in' into components separated by __DOT__, 'last_dot_pos'
    // keeps track of the position after the most recently found instance of __DOT__
    ಗಾಗಿ (string::ಗಾತ್ರ_type ಕೊನೆಯ_dot_pos = 0; ಕೊನೆಯ_dot_pos < ಒಳ.ಗಾತ್ರ_2();) {
        ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type ಮುಂದಿನ_dot_pos = ಒಳ.ಕಂಡುಹಿಡಿ("__DOT__", ಕೊನೆಯ_dot_pos);
        // Two iterators defining the range between the last and next dots.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಹುಡುಕು_begin = std::begin(ಒಳ) + ಕೊನೆಯ_dot_pos;
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಹುಡುಕು_ಕೊನೆ
            = ಮುಂದಿನ_dot_pos == string::npos ? std::ಕೊನೆ(ಒಳ) : std::begin(ಒಳ) + ಮುಂದಿನ_dot_pos;

        // Search for __Vhsh between the two dots.
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ begin_vhsh
            = std::ಹುಡುಕು(ಹುಡುಕು_begin, ಹುಡುಕು_ಕೊನೆ, std::begin(VHSH), std::ಕೊನೆ(VHSH) - 1);
        ಆದರೆ (begin_vhsh != ಹುಡುಕು_ಕೊನೆ) {
            // V3Decompose and V3BitBlast append suffixes to names already hashed, so the
            // hash does not always reach the end of the component.
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಕೊನೆ_vhsh
                = begin_vhsh + std::min<ಗಾತ್ರ_t>(std::ದೂರ(begin_vhsh, ಹುಡುಕು_ಕೊನೆ), VHSH_LEN);
            ಸ್ಥಿರಾಂಕ std::string vhsh{begin_vhsh, ಕೊನೆ_vhsh};
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& it = s_dehashMap.ಕಂಡುಹಿಡಿ(vhsh);
            UASSERT(it != s_dehashMap.ಕೊನೆ(), "String not in reverse hash map '" << vhsh << "'");
            // Is this not the first component, but the first to require dehashing?
            ಆದರೆ (ಕೊನೆಯ_dot_pos > 0 && dehashed.ಖಾಲಿಗಳು()) {
                // Seed 'dehashed' with the previously processed components.
                dehashed = ಒಳ.substr(0, ಕೊನೆಯ_dot_pos);
            }
            // Append the unhashed part of the component.
            dehashed += std::string{ಹುಡುಕು_begin, begin_vhsh};
            // Append the bit that was lost to truncation but retrieved from the dehash map.
            dehashed += it->ಎರಡನೇ;
            // Append what follows the hash, such as a split variable bit range.
            dehashed += std::string{ಕೊನೆ_vhsh, ಹುಡುಕು_ಕೊನೆ};
        }
        // This component doesn't need dehashing but a previous one might have.
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!dehashed.ಖಾಲಿಗಳು()) {
            dehashed += std::string{ಹುಡುಕು_begin, ಹುಡುಕು_ಕೊನೆ};
        }

        ಆದರೆ (ಮುಂದಿನ_dot_pos != string::npos) {
            // Is there a __DOT__ to add to the dehashed version of 'in'?
            ಆದರೆ (!dehashed.ಖಾಲಿಗಳು()) dehashed += "__DOT__";
            ಕೊನೆಯ_dot_pos = ಮುಂದಿನ_dot_pos + DOT_LEN;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೊನೆಯ_dot_pos = string::npos;
        }
    }
    ಹಿಂತಿರುಗಿ dehashed.ಖಾಲಿಗಳು() ? ಒಳ : dehashed;
}

string VName::hashedName() {
    ಆದರೆ (m_name == "") ಹಿಂತಿರುಗಿ "";
    ಆದರೆ (m_hashed != "") ಹಿಂತಿರುಗಿ m_hashed;  // Memoized
    ಆದರೆ (s_max_ಉದ್ದಳತೆ == 0 || m_name.ಉದ್ದಳತೆ() < s_max_ಉದ್ದಳತೆ) {
        m_hashed = m_name;
        ಹಿಂತಿರುಗಿ m_hashed;
    }
    VHashSha512 hash{m_name};
    ಸ್ಥಿರಾಂಕ string suffix = "__Vhsh" + hash.digestSymbol24();
    ಆದರೆ (s_min_ಉದ್ದಳತೆ < s_max_ಉದ್ದಳತೆ) {
        // Keep a prefix from the original name
        // Backup over digits so adding __Vhash doesn't look like a encoded hex digit
        // ("__0__Vhsh")
        ಗಾತ್ರ_t pref_ಉದ್ದಳತೆ = s_min_ಉದ್ದಳತೆ;
        ಆಗ (pref_ಉದ್ದಳತೆ >= 1 && m_name[pref_ಉದ್ದಳತೆ - 1] != '_') --pref_ಉದ್ದಳತೆ;
        s_dehashMap[suffix] = m_name.substr(pref_ಉದ್ದಳತೆ);
        m_hashed = m_name.substr(0, pref_ಉದ್ದಳತೆ) + suffix;
    } ಇಲ್ಲದಿದ್ದರೆ {
        s_dehashMap[suffix] = m_name;
        m_hashed = suffix;
    }
    ಹಿಂತಿರುಗಿ m_hashed;
}

//######################################################################
// VSpellCheck - Algorithm same as GCC's spellcheck.c

VSpellCheck::Edit_ದೂರ VSpellCheck::edit_ದೂರ(ಸ್ಥಿರಾಂಕ string& s, ಸ್ಥಿರಾಂಕ string& t) {
    // Wagner-Fischer algorithm for the Damerau-Levenshtein distance
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t sLen = s.ಉದ್ದಳತೆ();
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t tLen = t.ಉದ್ದಳತೆ();
    ಆದರೆ (sLen == 0) ಹಿಂತಿರುಗಿ tLen;
    ಆದರೆ (tLen == 0) ಹಿಂತಿರುಗಿ sLen;
    ಆದರೆ (sLen >= ಉದ್ದಳತೆ_ಎಲ್ಲೆ) ಹಿಂತಿರುಗಿ sLen;
    ಆದರೆ (tLen >= ಉದ್ದಳತೆ_ಎಲ್ಲೆ) ಹಿಂತಿರುಗಿ tLen;

    ಸ್ಥಿರ std::array<Edit_ದೂರ, ಉದ್ದಳತೆ_ಎಲ್ಲೆ + 1> s_v_two_ago;
    ಸ್ಥಿರ std::array<Edit_ದೂರ, ಉದ್ದಳತೆ_ಎಲ್ಲೆ + 1> s_v_one_ago;
    ಸ್ಥಿರ std::array<Edit_ದೂರ, ಉದ್ದಳತೆ_ಎಲ್ಲೆ + 1> s_v_ಮುಂದಿನ;

    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < sLen + 1; i++) s_v_one_ago[i] = i;

    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < tLen; i++) {
        s_v_ಮುಂದಿನ[0] = i + 1;
        ಗಾಗಿ (ಗಾತ್ರ_t j = 0; j < sLen; j++) {
            ಸ್ಥಿರಾಂಕ Edit_ದೂರ ವೆಚ್ಚ = (s[j] == t[i] ? 0 : 1);
            ಸ್ಥಿರಾಂಕ Edit_ದೂರ deletion = s_v_ಮುಂದಿನ[j] + 1;
            ಸ್ಥಿರಾಂಕ Edit_ದೂರ insertion = s_v_one_ago[j + 1] + 1;
            ಸ್ಥಿರಾಂಕ Edit_ದೂರ substitution = s_v_one_ago[j] + ವೆಚ್ಚ;
            Edit_ದೂರ cheapest = std::min(deletion, insertion);
            cheapest = std::min(cheapest, substitution);
            ಆದರೆ (i > 0 && j > 0 && s[j] == t[i - 1] && s[j - 1] == t[i]) {
                ಸ್ಥಿರಾಂಕ Edit_ದೂರ transposition = s_v_two_ago[j - 1] + 1;
                cheapest = std::min(cheapest, transposition);
            }
            s_v_ಮುಂದಿನ[j + 1] = cheapest;
        }
        ಗಾಗಿ (ಗಾತ್ರ_t j = 0; j < sLen + 1; j++) {
            s_v_two_ago[j] = s_v_one_ago[j];
            s_v_one_ago[j] = s_v_ಮುಂದಿನ[j];
        }
    }

    ಸ್ಥಿರಾಂಕ Edit_ದೂರ ಫಲಿತಾಂಶ = s_v_ಮುಂದಿನ[sLen];
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

VSpellCheck::Edit_ದೂರ VSpellCheck::cutoff_ದೂರ(ಗಾತ್ರ_t goal_len, ಗಾತ್ರ_t candidate_len) {
    // Return max acceptable edit distance
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t max_ಉದ್ದಳತೆ_2 = std::max(goal_len, candidate_len);
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t min_ಉದ್ದಳತೆ = std::min(goal_len, candidate_len);
    ಆದರೆ (max_ಉದ್ದಳತೆ_2 <= 1) ಹಿಂತಿರುಗಿ 0;
    ಆದರೆ (max_ಉದ್ದಳತೆ_2 - min_ಉದ್ದಳತೆ <= 1) ಹಿಂತಿರುಗಿ std::max(max_ಉದ್ದಳತೆ_2 / 3, static_cast<ಗಾತ್ರ_t>(1));
    ಹಿಂತಿರುಗಿ (max_ಉದ್ದಳತೆ_2 + 2) / 3;
}

VSpellCheck::Candidate VSpellCheck::ಉತ್ತಮ_Candidate_Info(ಸ್ಥಿರಾಂಕ string& goal,
                                                      Edit_ದೂರ& distancer) ಸ್ಥಿರಾಂಕ {
    Candidate ಉತ್ತಮ;
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t gLen = goal.ಉದ್ದಳತೆ();
    distancer = ಉದ್ದಳತೆ_ಎಲ್ಲೆ * 10;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Candidate& candidate : m_candidates) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t cLen = candidate.str.ಉದ್ದಳತೆ();

        // Min distance must be inserting/deleting to make lengths match
        ಸ್ಥಿರಾಂಕ Edit_ದೂರ min_ದೂರ = (cLen > gLen ? (cLen - gLen) : (gLen - cLen));
        ಆದರೆ (min_ದೂರ >= distancer) ಮುಂದುವರಿ;  // Short-circuit if already better

        ಸ್ಥಿರಾಂಕ Edit_ದೂರ cutoff = cutoff_ದೂರ(gLen, cLen);
        ಆದರೆ (min_ದೂರ > cutoff) ಮುಂದುವರಿ;  // Short-circuit if already too bad

        ಸ್ಥಿರಾಂಕ Edit_ದೂರ dist = edit_ದೂರ(goal, candidate.str);
        UINFO(9, "EditDistance dist=" << dist << " cutoff=" << cutoff << " goal=" << goal
                                      << " candidate=" << candidate.str << ":'" << candidate.ಸಂದೇಶ_2
                                      << "'");
        ಆದರೆ (dist < distancer && dist <= cutoff) {
            distancer = dist;
            ಉತ್ತಮ = candidate;
        }
    }

    // If goal matches candidate avoid suggesting replacing with self
    ಆದರೆ (distancer == 0) ಹಿಂತಿರುಗಿ Candidate{.str = ಉತ್ತಮ.str, .ಸಂದೇಶ_2 = ""};
    ಹಿಂತಿರುಗಿ ಉತ್ತಮ;
}

ಶೂನ್ಯ VSpellCheck::self_Test_ದೂರ_One(ಸ್ಥಿರಾಂಕ string& a, ಸ್ಥಿರಾಂಕ string& b, Edit_ದೂರ expected) {
    UASSERT_SELFTEST(edit_ದೂರ(a, b), expected);
    UASSERT_SELFTEST(edit_ದೂರ(b, a), expected);
}

ಶೂನ್ಯ VSpellCheck::selfTestSuggestOne(ತಾರ್ಕಿಕ matches, ಸ್ಥಿರಾಂಕ string& c, ಸ್ಥಿರಾಂಕ string& goal,
                                     Edit_ದೂರ dist) {
    Edit_ದೂರ gdist;
    VSpellCheck speller;
    speller.ತಳ್ಳು_Candidate(c);
    ಸ್ಥಿರಾಂಕ string got = speller.ಉತ್ತಮ_Candidate_Info(goal, gdist /*ref*/).str;
    ಆದರೆ (matches) {
        UASSERT_SELFTEST(got, c);
        UASSERT_SELFTEST(gdist, dist);
    } ಇಲ್ಲದಿದ್ದರೆ {
        UASSERT_SELFTEST(got, "");
    }
}

ಶೂನ್ಯ VSpellCheck::selfTest() {
    {
        self_Test_ದೂರ_One("ab", "ac", 1);
        self_Test_ದೂರ_One("ab", "a", 1);
        self_Test_ದೂರ_One("a", "b", 1);
    }
    {
        selfTestSuggestOne(ನಿಜ, "DEL_ETE", "DELETE", 1);
        selfTestSuggestOne(ನಿಜ, "abcdef", "acbdef", 1);
        selfTestSuggestOne(ನಿಜ, "db", "dc", 1);
        selfTestSuggestOne(ನಿಜ, "db", "dba", 1);
        // Negative suggestions
        selfTestSuggestOne(ಸುಳ್ಳು, "x", "y", 1);
        selfTestSuggestOne(ಸುಳ್ಳು, "sqrt", "assert", 3);
    }
    {
        ಸ್ಥಿರಾಂಕ VSpellCheck speller;
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("").str, "");
    }
    {
        VSpellCheck speller;
        speller.ತಳ್ಳು_Candidate("fred");
        speller.ತಳ್ಳು_Candidate("wilma");
        speller.ತಳ್ಳು_Candidate("barney");
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("whilma").str, "wilma");
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("Barney").str, "barney");
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("nothing close").str, "");
    }
    {
        VSpellCheck speller;
        ಗಾಗಿ (ಚಿಹ್ನೆರಹಿತ i = 0; i < ಅಂಕೆ_CANDIDATE_ಎಲ್ಲೆ; ++i) speller.ತಳ್ಳು_Candidate("fred");
        UASSERT_SELFTEST(speller.m_candidates.ಗಾತ್ರ_2(), ಅಂಕೆ_CANDIDATE_ಎಲ್ಲೆ);
        speller.ತಳ್ಳು_Candidate("wilma");
        UASSERT_SELFTEST(speller.m_candidates.ಗಾತ್ರ_2(), ಅಂಕೆ_CANDIDATE_ಎಲ್ಲೆ);
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("fre").str, "fred");
        UASSERT_SELFTEST(speller.ಉತ್ತಮ_Candidate("wilma").str, "");
    }
}
