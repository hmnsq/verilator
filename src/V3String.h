// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: String manipulation
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

#جے_بنا_تعریف VERILATOR_V3STRING_H_
#تعریف_کرو VERILATOR_V3STRING_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#ضمن <deque>
#ضمن <iomanip>
#ضمن <map>
#ضمن <set>
#ضمن <sstream>
#ضمن <string>
#ضمن <type_traits>
#ضمن <unordered_map>
#ضمن <vector>

using VStringList = std::سمتیہ<std::string>;
using VString_سیٹ = std::سیٹ<std::string>;

//######################################################################
// Global string-related functions

سانچا <typename T>
std::string cvt_منزل_Str(ثابت T& t) VL_خالص {
    std::ostringstream os;
    os << t;
    أرجع os.str();
}
سانچا <typename T>
typename std::enable_if<std::is_اشارہ_گر<T>::قدر, std::string>::type
cvt_منزل_Hex(ثابت T tp) VL_خالص {
    std::ostringstream os;
    os << static_cast<ثابت فراغ*>(tp);
    أرجع os.str();
}
سانچا <typename T>
typename std::enable_if<std::is_integral<T>::قدر, std::string>::type cvt_منزل_Hex(ثابت T t) {
    std::ostringstream os;
    os << std::hex << std::setw(حجم(T) * 8 / 4) << std::setfill('0') << t;
    أرجع os.str();
}

مضمن string ucfirst(ثابت string& متن) {
    string نتیجہ = متن;
    نتیجہ[0] = std::وڈا_حرف(نتیجہ[0]);
    أرجع نتیجہ;
}

//######################################################################
// VString - String manipulation

class VString final {
    ساكن bool wildmatchi(ثابت حرف* s, ثابت حرف* p) VL_خالص;

عام:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ساكن bool wildmatch(ثابت حرف* s, ثابت حرف* p) VL_خالص;
    // Return true if p with ? or *'s matches s
    ساكن bool wildmatch(ثابت string& s, ثابت string& p) VL_خالص;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ساكن string dot(ثابت string& a, ثابت string& dot, ثابت string& b);
    // Convert string to lowercase (tolower)
    ساكن string downcase(ثابت string& str) VL_خالص;
    // Convert string to upper case (toupper)
    ساكن string upcase(ثابت string& str) VL_خالص;
    // Insert esc just before tgt
    ساكن string quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_خالص;
    // Replace any \'s with \\  (two consecutive backslashes)
    ساكن string quoteBackslash(ثابت string& str) VL_خالص { أرجع quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ساكن string quotePercent(ثابت string& str) VL_خالص { أرجع quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ساكن string dequotePercent(ثابت string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ساكن string escape_String_For_رستہ(ثابت string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ساكن string unquoteSVString(ثابت string& متن, string& err_باہرلا);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ساكن ناپ_t quoted_سرا(ثابت string& str, ناپ_t pos) VL_خالص;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ساكن string quoteStringLiteralForShell(ثابت string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ساكن string spaceUnprintable(ثابت string& str) VL_خالص;
    // Remove any whitespace
    ساكن string removeWhitespace(ثابت string& str);
    // Trim leading/trailing whitespace on each line
    ساكن string trimWhitespace(ثابت string& str);
    // Return true if only identifer or ""
    ساكن bool isIdentifier(ثابت string& str);
    // Return true if char is valid character in C identifiers
    ساكن bool isIdentifierChar(حرف c) { أرجع حرف_یا_ہندسہ(c) || c == '_'; }
    // Return true if only whitespace or ""
    ساكن bool isWhitespace(ثابت string& str);
    // Return number of spaces/tabs leading in string
    ساكن string::ناپ_type leading_Whitespace_گنتی(ثابت string& str);
    // Return double by parsing string
    ساكن مزدوج parseDouble(ثابت string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ساكن string replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& منزل);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ساكن string replace_لفظ(ثابت string& str, ثابت string& from, ثابت string& منزل);
    // Return deque of strings split by the given delimiter
    ساكن std::deque<string> ونڈو(ثابت string& str, حرف delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ساكن bool startsWith(ثابت string& str, ثابت حرف* prefixp) {
        أرجع !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ساكن bool startsWith(ثابت string& str, ثابت string& اگیتر) {
        أرجع !str.rfind(اگیتر, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ساكن bool سرے_With(ثابت string& str, ثابت string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ساكن string aOrAn(ثابت حرف* لفظ);
    ساكن string aOrAn(ثابت string& لفظ) { أرجع aOrAn(لفظ.c_str()); }
    // Hash the string
    ساكن uint64_t hashMurmur(ثابت string& str) VL_خالص;

    // Base 64 encode
    ساكن string base64Enc(ثابت string& str) VL_خالص;

    ساكن فراغ selfTest();  // Test this class
};

//######################################################################
// VHashSha512 - Compute Sha512 hashes

class VHashSha512 final {
    // As blocks must be processed in 128 byte chunks, this does not at present
    // support calling input() on multiple non-128B chunks and getting the correct
    // hash. To do that first combine the string before calling here.
    // Or improve to store 0-127 bytes of data between calls to input().

    // MEMBERS
    uint64_t m_inthash[8];  // Intermediate hash, in host order
    string m_remainder;  // Unhashed data
    ناپ_t m_tot_لمائی = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
عام:
    // CONSTRUCTORS
    VHashSha512() {
        m_inthash[0] = 0x6a09e667f3bcc908ULL;
        m_inthash[1] = 0xbb67ae8584caa73bULL;
        m_inthash[2] = 0x3c6ef372fe94f82bULL;
        m_inthash[3] = 0xa54ff53a5f1d36f1ULL;
        m_inthash[4] = 0x510e527fade682d1ULL;
        m_inthash[5] = 0x9b05688c2b3e6c1fULL;
        m_inthash[6] = 0x1f83d9abfb41bd6bULL;
        m_inthash[7] = 0x5be0cd19137e2179ULL;
    }
    explicit VHashSha512(ثابت string& انکڑے)
        : VHashSha512{} {
        داخل_کرو(انکڑے);
    }
    ~VHashSha512() = افتراضي;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ساكن constexpr ناپ_t DIGEST_SYMBOL24_لمائی = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ثنائی();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ساكن فراغ selfTest();  // Test this class

    // Inserting hash data
    فراغ داخل_کرو(ثابت فراغ* datap, ناپ_t لمائی);  // Process data into the digest
    فراغ داخل_کرو(ثابت string& انکڑے) {
        داخل_کرو(انکڑے.انکڑے(), انکڑے.لمائی());
    }  // Process data into the digest
    فراغ داخل_کرو(uint64_t قدر) { داخل_کرو(cvt_منزل_Str(قدر)); }
    فراغ داخل_کرو_File(ثابت string& filename);

خاص:
    ساكن فراغ selfTestOne(ثابت string& انکڑے, ثابت string& data2, ثابت string& قوت_نما,
                            ثابت string& exp64, ثابت string& exp24);
    فراغ finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ساكن std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ساكن ناپ_t s_max_لمائی;  // Length at which to start hashing
    ساكن ناپ_t s_min_لمائی;  // Length to preserve if over maxLength

عام:
    // CONSTRUCTORS
    explicit VName(ثابت string& name)
        : m_name{name} {}
    ~VName() = افتراضي;
    // METHODS
    فراغ name(ثابت string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ثابت { أرجع m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ساكن فراغ max_لمائی(ناپ_t flag) { s_max_لمائی = flag; }
    ساكن string dehash(ثابت string& وچ);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ساكن constexpr بنانشانی عدد_CANDIDATE_حد_بندی = 10000;  // Avoid searching huge netlists
    ساكن constexpr بنانشانی لمائی_حد_بندی = 100;  // Maximum string length to search
    // TYPES
    using Edit_فاصلہ = بنانشانی صحيح;
    بنية Candidate final {
        std::string str;  // String to be matched
        std::string پیغام;  // Message to print on a match
    };
    // MEMBERS
    std::سمتیہ<Candidate> m_candidates;  // Strings we try to match
عام:
    // CONSTRUCTORS
    VSpellCheck() = افتراضي;
    ~VSpellCheck() = افتراضي;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    فراغ ڈالو_Candidate(ثابت string& str) { ڈالو_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    فراغ ڈالو_Candidate(ثابت string& str, ثابت string& پیغام) {
        إذا (m_candidates.ناپ() < عدد_CANDIDATE_حد_بندی) m_candidates.ڈالو_پٹھ({str, پیغام});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate بہترین_Candidate(ثابت string& goal) ثابت {
        Edit_فاصلہ dist;
        أرجع بہترین_Candidate_Info(goal, dist /*ref*/);
    }
    // Return friendly message
    string بہترین_Candidate_پیغام(ثابت string& goal) ثابت {
        ثابت خودکار candidate = بہترین_Candidate(goal);
        إذا (candidate.پیغام.خالی_تھاواں()) أرجع "";
        أرجع "... Suggested alternative: '"s + candidate.پیغام + "'";
    }
    ساكن فراغ selfTest();
    std::سمتیہ<std::string> candidates() ثابت {
        std::سمتیہ<std::string> نتیجہ;
        نتیجہ.reserve(m_candidates.ناپ());
        لكل (ثابت خودکار& candidate : m_candidates) { نتیجہ.ڈالو_پٹھ(candidate.پیغام); }
        أرجع نتیجہ;
    }

خاص:
    ساكن Edit_فاصلہ edit_فاصلہ(ثابت string& s, ثابت string& t);
    ساكن Edit_فاصلہ cutoff_فاصلہ(ناپ_t goal_len, ناپ_t candidate_len);
    VSpellCheck::Candidate بہترین_Candidate_Info(ثابت string& goal, Edit_فاصلہ& distancer) ثابت;
    ساكن فراغ self_Test_فاصلہ_One(ثابت string& a, ثابت string& b, Edit_فاصلہ expected);
    ساكن فراغ selfTestSuggestOne(bool matches, ثابت string& c, ثابت string& goal,
                                   Edit_فاصلہ dist);
};

//######################################################################

#جے_انت  // guard
