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

#اگر_تعریف_نشده VERILATOR_V3STRING_H_
#تعریف_کن VERILATOR_V3STRING_H_

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

using VStringList = std::وکتور<std::string>;
using VString_مجموعه = std::مجموعه<std::string>;

//######################################################################
// Global string-related functions

قالب <typename T>
std::string cvt_مقصد_Str(ثابت T& t) VL_خالص {
    std::ostringstream os;
    os << t;
    أرجع os.str();
}
قالب <typename T>
typename std::enable_if<std::is_اشاره_گر<T>::مقدار, std::string>::type
cvt_مقصد_Hex(ثابت T tp) VL_خالص {
    std::ostringstream os;
    os << static_cast<ثابت فراغ*>(tp);
    أرجع os.str();
}
قالب <typename T>
typename std::enable_if<std::is_integral<T>::مقدار, std::string>::type cvt_مقصد_Hex(ثابت T t) {
    std::ostringstream os;
    os << std::hex << std::setw(حجم(T) * 8 / 4) << std::setfill('0') << t;
    أرجع os.str();
}

مضمن string ucfirst(ثابت string& متن) {
    string نتیجه = متن;
    نتیجه[0] = std::به_بزرگ(نتیجه[0]);
    أرجع نتیجه;
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
    ساكن string escape_String_For_مسیر(ثابت string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ساكن string unquoteSVString(ثابت string& متن, string& err_بیرون);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ساكن اندازه_t quoted_انتها(ثابت string& str, اندازه_t pos) VL_خالص;
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
    ساكن bool isIdentifierChar(حرف c) { أرجع آیا_حرف_یا_رقم(c) || c == '_'; }
    // Return true if only whitespace or ""
    ساكن bool isWhitespace(ثابت string& str);
    // Return number of spaces/tabs leading in string
    ساكن string::اندازه_type leading_Whitespace_شمار(ثابت string& str);
    // Return double by parsing string
    ساكن مزدوج parseDouble(ثابت string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ساكن string replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& مقصد);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ساكن string replace_کلمه(ثابت string& str, ثابت string& from, ثابت string& مقصد);
    // Return deque of strings split by the given delimiter
    ساكن std::deque<string> تقسیم(ثابت string& str, حرف delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ساكن bool startsWith(ثابت string& str, ثابت حرف* prefixp) {
        أرجع !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ساكن bool startsWith(ثابت string& str, ثابت string& پیشوند) {
        أرجع !str.rfind(پیشوند, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ساكن bool دوسر_With(ثابت string& str, ثابت string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ساكن string aOrAn(ثابت حرف* کلمه);
    ساكن string aOrAn(ثابت string& کلمه) { أرجع aOrAn(کلمه.c_str()); }
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
    اندازه_t m_tot_درازی = 0;  // Total all-chunk length as needed by output digest
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
    explicit VHashSha512(ثابت string& داده)
        : VHashSha512{} {
        درج(داده);
    }
    ~VHashSha512() = افتراضي;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ساكن constexpr اندازه_t DIGEST_SYMBOL24_درازی = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_دودویی();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ساكن فراغ selfTest();  // Test this class

    // Inserting hash data
    فراغ درج(ثابت فراغ* datap, اندازه_t درازی);  // Process data into the digest
    فراغ درج(ثابت string& داده) {
        درج(داده.داده(), داده.درازی());
    }  // Process data into the digest
    فراغ درج(uint64_t مقدار) { درج(cvt_مقصد_Str(مقدار)); }
    فراغ درج_File(ثابت string& filename);

خاص:
    ساكن فراغ selfTestOne(ثابت string& داده, ثابت string& data2, ثابت string& نمایی_تابع,
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

    ساكن اندازه_t s_max_درازی;  // Length at which to start hashing
    ساكن اندازه_t s_min_درازی;  // Length to preserve if over maxLength

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
    ساكن فراغ max_درازی(اندازه_t flag) { s_max_درازی = flag; }
    ساكن string dehash(ثابت string& درون);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ساكن constexpr بیعلامت رقم_CANDIDATE_حد = 10000;  // Avoid searching huge netlists
    ساكن constexpr بیعلامت درازی_حد = 100;  // Maximum string length to search
    // TYPES
    using Edit_فاصله = بیعلامت صحيح;
    بنية Candidate final {
        std::string str;  // String to be matched
        std::string پیام_2;  // Message to print on a match
    };
    // MEMBERS
    std::وکتور<Candidate> m_candidates;  // Strings we try to match
عام:
    // CONSTRUCTORS
    VSpellCheck() = افتراضي;
    ~VSpellCheck() = افتراضي;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    فراغ افزودن_Candidate(ثابت string& str) { افزودن_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    فراغ افزودن_Candidate(ثابت string& str, ثابت string& پیام_2) {
        إذا (m_candidates.اندازه() < رقم_CANDIDATE_حد) m_candidates.افزودن_پشت({str, پیام_2});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate بهترین_Candidate(ثابت string& goal) ثابت {
        Edit_فاصله dist;
        أرجع بهترین_Candidate_Info(goal, dist /*ref*/);
    }
    // Return friendly message
    string بهترین_Candidate_پیام(ثابت string& goal) ثابت {
        ثابت خودکار candidate = بهترین_Candidate(goal);
        إذا (candidate.پیام_2.خالی()) أرجع "";
        أرجع "... Suggested alternative: '"s + candidate.پیام_2 + "'";
    }
    ساكن فراغ selfTest();
    std::وکتور<std::string> candidates() ثابت {
        std::وکتور<std::string> نتیجه;
        نتیجه.reserve(m_candidates.اندازه());
        لكل (ثابت خودکار& candidate : m_candidates) { نتیجه.افزودن_پشت(candidate.پیام_2); }
        أرجع نتیجه;
    }

خاص:
    ساكن Edit_فاصله edit_فاصله(ثابت string& s, ثابت string& t);
    ساكن Edit_فاصله cutoff_فاصله(اندازه_t goal_len, اندازه_t candidate_len);
    VSpellCheck::Candidate بهترین_Candidate_Info(ثابت string& goal, Edit_فاصله& distancer) ثابت;
    ساكن فراغ self_Test_فاصله_One(ثابت string& a, ثابت string& b, Edit_فاصله expected);
    ساكن فراغ selfTestSuggestOne(bool matches, ثابت string& c, ثابت string& goal,
                                   Edit_فاصله dist);
};

//######################################################################

#پایان_اگر  // guard
