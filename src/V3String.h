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

#ifndef VERILATOR_V3STRING_H_
#define VERILATOR_V3STRING_H_

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

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(ثابت T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    بازگشت os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::مقدار, std::string>::type
cvtToHex(ثابت T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<ثابت تهی*>(tp);
    بازگشت os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::مقدار, std::string>::type cvtToHex(ثابت T t) {
    std::ostringstream os;
    os << std::hex << std::setw(اندازه(T) * 8 / 4) << std::setfill('0') << t;
    بازگشت os.str();
}

درونخطی string ucfirst(ثابت string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    بازگشت result;
}

//######################################################################
// VString - String manipulation

class VString final {
    ایستا bool wildmatchi(ثابت نویسه* s, ثابت نویسه* p) VL_PURE;

عمومی:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ایستا bool wildmatch(ثابت نویسه* s, ثابت نویسه* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    ایستا bool wildmatch(ثابت string& s, ثابت string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ایستا string dot(ثابت string& a, ثابت string& dot, ثابت string& b);
    // Convert string to lowercase (tolower)
    ایستا string downcase(ثابت string& str) VL_PURE;
    // Convert string to upper case (toupper)
    ایستا string upcase(ثابت string& str) VL_PURE;
    // Insert esc just before tgt
    ایستا string quoteAny(ثابت string& str, نویسه tgt, نویسه esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    ایستا string quoteBackslash(ثابت string& str) VL_PURE { بازگشت quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ایستا string quotePercent(ثابت string& str) VL_PURE { بازگشت quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ایستا string dequotePercent(ثابت string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ایستا string escapeStringForPath(ثابت string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ایستا string unquoteSVString(ثابت string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ایستا size_t quotedEnd(ثابت string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ایستا string quoteStringLiteralForShell(ثابت string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ایستا string spaceUnprintable(ثابت string& str) VL_PURE;
    // Remove any whitespace
    ایستا string removeWhitespace(ثابت string& str);
    // Trim leading/trailing whitespace on each line
    ایستا string trimWhitespace(ثابت string& str);
    // Return true if only identifer or ""
    ایستا bool isIdentifier(ثابت string& str);
    // Return true if char is valid character in C identifiers
    ایستا bool isIdentifierChar(نویسه c) { بازگشت isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    ایستا bool isWhitespace(ثابت string& str);
    // Return number of spaces/tabs leading in string
    ایستا string::size_type leading_Whitespace_شمار(ثابت string& str);
    // Return double by parsing string
    ایستا دوگانه parseDouble(ثابت string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ایستا string replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ایستا string replaceWord(ثابت string& str, ثابت string& from, ثابت string& to);
    // Return deque of strings split by the given delimiter
    ایستا std::deque<string> split(ثابت string& str, نویسه delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ایستا bool startsWith(ثابت string& str, ثابت نویسه* prefixp) {
        بازگشت !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ایستا bool startsWith(ثابت string& str, ثابت string& prefix) {
        بازگشت !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ایستا bool endsWith(ثابت string& str, ثابت string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ایستا string aOrAn(ثابت نویسه* word);
    ایستا string aOrAn(ثابت string& word) { بازگشت aOrAn(word.c_str()); }
    // Hash the string
    ایستا uint64_t hashMurmur(ثابت string& str) VL_PURE;

    // Base 64 encode
    ایستا string base64Enc(ثابت string& str) VL_PURE;

    ایستا تهی selfTest();  // Test this class
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
    size_t m_totLength = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
عمومی:
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
    explicit VHashSha512(ثابت string& data)
        : VHashSha512{} {
        درج(data);
    }
    ~VHashSha512() = پیشفرض;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ایستا constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_دودویی();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ایستا تهی selfTest();  // Test this class

    // Inserting hash data
    تهی درج(ثابت تهی* datap, size_t length);  // Process data into the digest
    تهی درج(ثابت string& data) {
        درج(data.data(), data.length());
    }  // Process data into the digest
    تهی درج(uint64_t مقدار) { درج(cvtToStr(مقدار)); }
    تهی درج_File(ثابت string& filename);

خصوصی:
    ایستا تهی selfTestOne(ثابت string& data, ثابت string& data2, ثابت string& exp,
                            ثابت string& exp64, ثابت string& exp24);
    تهی finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ایستا std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ایستا size_t s_maxLength;  // Length at which to start hashing
    ایستا size_t s_minLength;  // Length to preserve if over maxLength

عمومی:
    // CONSTRUCTORS
    explicit VName(ثابت string& name)
        : m_name{name} {}
    ~VName() = پیشفرض;
    // METHODS
    تهی name(ثابت string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ثابت { بازگشت m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ایستا تهی maxLength(size_t flag) { s_maxLength = flag; }
    ایستا string dehash(ثابت string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ایستا constexpr unsigned رقم_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    ایستا constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned صحیح;
    ساختار Candidate final {
        std::string str;  // String to be matched
        std::string پیام;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
عمومی:
    // CONSTRUCTORS
    VSpellCheck() = پیشفرض;
    ~VSpellCheck() = پیشفرض;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    تهی افزودن_Candidate(ثابت string& str) { افزودن_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    تهی افزودن_Candidate(ثابت string& str, ثابت string& پیام) {
        اگر (m_candidates.size() < رقم_CANDIDATE_LIMIT) m_candidates.افزودن_back({str, پیام});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(ثابت string& goal) ثابت {
        EditDistance dist;
        بازگشت bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_پیام(ثابت string& goal) ثابت {
        ثابت auto candidate = bestCandidate(goal);
        اگر (candidate.پیام.empty()) بازگشت "";
        بازگشت "... Suggested alternative: '"s + candidate.پیام + "'";
    }
    ایستا تهی selfTest();
    std::vector<std::string> candidates() ثابت {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        برای (ثابت auto& candidate : m_candidates) { result.افزودن_back(candidate.پیام); }
        بازگشت result;
    }

خصوصی:
    ایستا EditDistance editDistance(ثابت string& s, ثابت string& t);
    ایستا EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(ثابت string& goal, EditDistance& distancer) ثابت;
    ایستا تهی selfTestDistanceOne(ثابت string& a, ثابت string& b, EditDistance expected);
    ایستا تهی selfTestSuggestOne(bool matches, ثابت string& c, ثابت string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
