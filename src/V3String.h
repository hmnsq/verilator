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

#اگر_غیر_تعریف_شدہ VERILATOR_V3STRING_H_
#تعریف_کرو VERILATOR_V3STRING_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#شامل <deque>
#شامل <iomanip>
#شامل <map>
#شامل <set>
#شامل <sstream>
#شامل <string>
#شامل <type_traits>
#شامل <unordered_map>
#شامل <vector>

using VStringList = std::سمتیہ<std::string>;
using VString_سیٹ = std::سیٹ<std::string>;

//######################################################################
// Global string-related functions

سانچہ <typename T>
std::string cvt_مقصد_Str(ثابت T& t) VL_خالص {
    std::ostringstream os;
    os << t;
    واپس os.str();
}
سانچہ <typename T>
typename std::enable_if<std::is_اشارہ_گر<T>::قدر, std::string>::type
cvt_مقصد_Hex(ثابت T tp) VL_خالص {
    std::ostringstream os;
    os << static_cast<ثابت خالی*>(tp);
    واپس os.str();
}
سانچہ <typename T>
typename std::enable_if<std::is_integral<T>::قدر, std::string>::type cvt_مقصد_Hex(ثابت T t) {
    std::ostringstream os;
    os << std::hex << std::setw(حجم(T) * 8 / 4) << std::setfill('0') << t;
    واپس os.str();
}

اندرونی string ucfirst(ثابت string& متن) {
    string نتیجہ = متن;
    نتیجہ[0] = std::بڑا_حرف(نتیجہ[0]);
    واپس نتیجہ;
}

//######################################################################
// VString - String manipulation

class VString final {
    ساکن bool wildmatchi(ثابت حرف* s, ثابت حرف* p) VL_خالص;

عوامی:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ساکن bool wildmatch(ثابت حرف* s, ثابت حرف* p) VL_خالص;
    // Return true if p with ? or *'s matches s
    ساکن bool wildmatch(ثابت string& s, ثابت string& p) VL_خالص;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ساکن string dot(ثابت string& a, ثابت string& dot, ثابت string& b);
    // Convert string to lowercase (tolower)
    ساکن string downcase(ثابت string& str) VL_خالص;
    // Convert string to upper case (toupper)
    ساکن string upcase(ثابت string& str) VL_خالص;
    // Insert esc just before tgt
    ساکن string quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_خالص;
    // Replace any \'s with \\  (two consecutive backslashes)
    ساکن string quoteBackslash(ثابت string& str) VL_خالص { واپس quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ساکن string quotePercent(ثابت string& str) VL_خالص { واپس quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ساکن string dequotePercent(ثابت string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ساکن string escape_String_For_راستہ(ثابت string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ساکن string unquoteSVString(ثابت string& متن, string& err_برآمد);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ساکن سائز_t quoted_انتہا(ثابت string& str, سائز_t pos) VL_خالص;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ساکن string quoteStringLiteralForShell(ثابت string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ساکن string spaceUnprintable(ثابت string& str) VL_خالص;
    // Remove any whitespace
    ساکن string removeWhitespace(ثابت string& str);
    // Trim leading/trailing whitespace on each line
    ساکن string trimWhitespace(ثابت string& str);
    // Return true if only identifer or ""
    ساکن bool isIdentifier(ثابت string& str);
    // Return true if char is valid character in C identifiers
    ساکن bool isIdentifierChar(حرف c) { واپس حرف_یا_ہندسہ(c) || c == '_'; }
    // Return true if only whitespace or ""
    ساکن bool isWhitespace(ثابت string& str);
    // Return number of spaces/tabs leading in string
    ساکن string::سائز_type leading_Whitespace_گنتی(ثابت string& str);
    // Return double by parsing string
    ساکن دوہرا parseDouble(ثابت string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ساکن string replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& مقصد);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ساکن string replace_لفظ(ثابت string& str, ثابت string& from, ثابت string& مقصد);
    // Return deque of strings split by the given delimiter
    ساکن std::deque<string> بانٹو(ثابت string& str, حرف delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ساکن bool startsWith(ثابت string& str, ثابت حرف* prefixp) {
        واپس !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ساکن bool startsWith(ثابت string& str, ثابت string& سابقہ) {
        واپس !str.rfind(سابقہ, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ساکن bool سرے_With(ثابت string& str, ثابت string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ساکن string aOrAn(ثابت حرف* لفظ);
    ساکن string aOrAn(ثابت string& لفظ) { واپس aOrAn(لفظ.c_str()); }
    // Hash the string
    ساکن uint64_t hashMurmur(ثابت string& str) VL_خالص;

    // Base 64 encode
    ساکن string base64Enc(ثابت string& str) VL_خالص;

    ساکن خالی selfTest();  // Test this class
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
    سائز_t m_tot_لمبائی = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
عوامی:
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
    explicit VHashSha512(ثابت string& ڈیٹا)
        : VHashSha512{} {
        داخل_کرو(ڈیٹا);
    }
    ~VHashSha512() = طےشدہ;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ساکن constexpr سائز_t DIGEST_SYMBOL24_لمبائی = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ثنائی();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ساکن خالی selfTest();  // Test this class

    // Inserting hash data
    خالی داخل_کرو(ثابت خالی* datap, سائز_t لمبائی_2);  // Process data into the digest
    خالی داخل_کرو(ثابت string& ڈیٹا) {
        داخل_کرو(ڈیٹا.ڈیٹا(), ڈیٹا.لمبائی_2());
    }  // Process data into the digest
    خالی داخل_کرو(uint64_t قدر) { داخل_کرو(cvt_مقصد_Str(قدر)); }
    خالی داخل_کرو_File(ثابت string& filename);

نجی:
    ساکن خالی selfTestOne(ثابت string& ڈیٹا, ثابت string& data2, ثابت string& قوت_نما,
                            ثابت string& exp64, ثابت string& exp24);
    خالی finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ساکن std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ساکن سائز_t s_max_لمبائی;  // Length at which to start hashing
    ساکن سائز_t s_min_لمبائی;  // Length to preserve if over maxLength

عوامی:
    // CONSTRUCTORS
    explicit VName(ثابت string& name)
        : m_name{name} {}
    ~VName() = طےشدہ;
    // METHODS
    خالی name(ثابت string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ثابت { واپس m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ساکن خالی max_لمبائی(سائز_t flag) { s_max_لمبائی = flag; }
    ساکن string dehash(ثابت string& اندر);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ساکن constexpr غیرعلامتی عدد_CANDIDATE_حد_بندی = 10000;  // Avoid searching huge netlists
    ساکن constexpr غیرعلامتی لمبائی_حد_بندی = 100;  // Maximum string length to search
    // TYPES
    using Edit_فاصلہ = غیرعلامتی صحیح;
    ساخت Candidate final {
        std::string str;  // String to be matched
        std::string پیغام_2;  // Message to print on a match
    };
    // MEMBERS
    std::سمتیہ<Candidate> m_candidates;  // Strings we try to match
عوامی:
    // CONSTRUCTORS
    VSpellCheck() = طےشدہ;
    ~VSpellCheck() = طےشدہ;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    خالی ڈالو_Candidate(ثابت string& str) { ڈالو_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    خالی ڈالو_Candidate(ثابت string& str, ثابت string& پیغام_2) {
        اگر (m_candidates.سائز() < عدد_CANDIDATE_حد_بندی) m_candidates.ڈالو_عقب({str, پیغام_2});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate بہترین_Candidate(ثابت string& goal) ثابت {
        Edit_فاصلہ dist;
        واپس بہترین_Candidate_Info(goal, dist /*ref*/);
    }
    // Return friendly message
    string بہترین_Candidate_پیغام(ثابت string& goal) ثابت {
        ثابت خودکار candidate = بہترین_Candidate(goal);
        اگر (candidate.پیغام_2.خالی_خانے()) واپس "";
        واپس "... Suggested alternative: '"s + candidate.پیغام_2 + "'";
    }
    ساکن خالی selfTest();
    std::سمتیہ<std::string> candidates() ثابت {
        std::سمتیہ<std::string> نتیجہ;
        نتیجہ.reserve(m_candidates.سائز());
        برائے (ثابت خودکار& candidate : m_candidates) { نتیجہ.ڈالو_عقب(candidate.پیغام_2); }
        واپس نتیجہ;
    }

نجی:
    ساکن Edit_فاصلہ edit_فاصلہ(ثابت string& s, ثابت string& t);
    ساکن Edit_فاصلہ cutoff_فاصلہ(سائز_t goal_len, سائز_t candidate_len);
    VSpellCheck::Candidate بہترین_Candidate_Info(ثابت string& goal, Edit_فاصلہ& distancer) ثابت;
    ساکن خالی self_Test_فاصلہ_One(ثابت string& a, ثابت string& b, Edit_فاصلہ expected);
    ساکن خالی selfTestSuggestOne(bool matches, ثابت string& c, ثابت string& goal,
                                   Edit_فاصلہ dist);
};

//######################################################################

#اگر_ختم  // guard
