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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(ಧ್ರುವ T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    ಪ್ರತ್ಯಾಗಮ os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::ಮೌಲ್ಯ, std::string>::type
cvtToHex(ಧ್ರುವ T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<ಧ್ರುವ ಶೂನ್ಯ*>(tp);
    ಪ್ರತ್ಯಾಗಮ os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::ಮೌಲ್ಯ, std::string>::type cvtToHex(ಧ್ರುವ T t) {
    std::ostringstream os;
    os << std::hex << std::setw(ಪರಿಮಾಣ(T) * 8 / 4) << std::setfill('0') << t;
    ಪ್ರತ್ಯಾಗಮ os.str();
}

ಅನ್ತಃಸ್ಥ string ucfirst(ಧ್ರುವ string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    ಪ್ರತ್ಯಾಗಮ result;
}

//######################################################################
// VString - String manipulation

class VString final {
    ಸ್ಥಿರ bool wildmatchi(ಧ್ರುವ ವರ್ಣ* s, ಧ್ರುವ ವರ್ಣ* p) VL_PURE;

ಸಾರ್ವಜನಿಕ:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ಸ್ಥಿರ bool wildmatch(ಧ್ರುವ ವರ್ಣ* s, ಧ್ರುವ ವರ್ಣ* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    ಸ್ಥಿರ bool wildmatch(ಧ್ರುವ string& s, ಧ್ರುವ string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ಸ್ಥಿರ string dot(ಧ್ರುವ string& a, ಧ್ರುವ string& dot, ಧ್ರುವ string& b);
    // Convert string to lowercase (tolower)
    ಸ್ಥಿರ string downcase(ಧ್ರುವ string& str) VL_PURE;
    // Convert string to upper case (toupper)
    ಸ್ಥಿರ string upcase(ಧ್ರುವ string& str) VL_PURE;
    // Insert esc just before tgt
    ಸ್ಥಿರ string quoteAny(ಧ್ರುವ string& str, ವರ್ಣ tgt, ವರ್ಣ esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    ಸ್ಥಿರ string quoteBackslash(ಧ್ರುವ string& str) VL_PURE { ಪ್ರತ್ಯಾಗಮ quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ಸ್ಥಿರ string quotePercent(ಧ್ರುವ string& str) VL_PURE { ಪ್ರತ್ಯಾಗಮ quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ಸ್ಥಿರ string dequotePercent(ಧ್ರುವ string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ಸ್ಥಿರ string escapeStringForPath(ಧ್ರುವ string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ಸ್ಥಿರ string unquoteSVString(ಧ್ರುವ string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ಸ್ಥಿರ size_t quotedEnd(ಧ್ರುವ string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ಸ್ಥಿರ string quoteStringLiteralForShell(ಧ್ರುವ string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ಸ್ಥಿರ string spaceUnprintable(ಧ್ರುವ string& str) VL_PURE;
    // Remove any whitespace
    ಸ್ಥಿರ string removeWhitespace(ಧ್ರುವ string& str);
    // Trim leading/trailing whitespace on each line
    ಸ್ಥಿರ string trimWhitespace(ಧ್ರುವ string& str);
    // Return true if only identifer or ""
    ಸ್ಥಿರ bool isIdentifier(ಧ್ರುವ string& str);
    // Return true if char is valid character in C identifiers
    ಸ್ಥಿರ bool isIdentifierChar(ವರ್ಣ c) { ಪ್ರತ್ಯಾಗಮ isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    ಸ್ಥಿರ bool isWhitespace(ಧ್ರುವ string& str);
    // Return number of spaces/tabs leading in string
    ಸ್ಥಿರ string::size_type leading_Whitespace_ಎಣಿಕೆ(ಧ್ರುವ string& str);
    // Return double by parsing string
    ಸ್ಥಿರ ದ್ವಿಗುಣ parseDouble(ಧ್ರುವ string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ಸ್ಥಿರ string replaceSubstr(ಧ್ರುವ string& str, ಧ್ರುವ string& from, ಧ್ರುವ string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ಸ್ಥಿರ string replaceWord(ಧ್ರುವ string& str, ಧ್ರುವ string& from, ಧ್ರುವ string& to);
    // Return deque of strings split by the given delimiter
    ಸ್ಥಿರ std::deque<string> split(ಧ್ರುವ string& str, ವರ್ಣ delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ಸ್ಥಿರ bool startsWith(ಧ್ರುವ string& str, ಧ್ರುವ ವರ್ಣ* prefixp) {
        ಪ್ರತ್ಯಾಗಮ !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ಸ್ಥಿರ bool startsWith(ಧ್ರುವ string& str, ಧ್ರುವ string& prefix) {
        ಪ್ರತ್ಯಾಗಮ !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ಸ್ಥಿರ bool endsWith(ಧ್ರುವ string& str, ಧ್ರುವ string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ಸ್ಥಿರ string aOrAn(ಧ್ರುವ ವರ್ಣ* word);
    ಸ್ಥಿರ string aOrAn(ಧ್ರುವ string& word) { ಪ್ರತ್ಯಾಗಮ aOrAn(word.c_str()); }
    // Hash the string
    ಸ್ಥಿರ uint64_t hashMurmur(ಧ್ರುವ string& str) VL_PURE;

    // Base 64 encode
    ಸ್ಥಿರ string base64Enc(ಧ್ರುವ string& str) VL_PURE;

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();  // Test this class
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
ಸಾರ್ವಜನಿಕ:
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
    explicit VHashSha512(ಧ್ರುವ string& data)
        : VHashSha512{} {
        ಸೇರಿಸು(data);
    }
    ~VHashSha512() = ಸ್ವಭಾವ;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ಸ್ಥಿರ constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ದ್ವಿಮಾನ();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();  // Test this class

    // Inserting hash data
    ಶೂನ್ಯ ಸೇರಿಸು(ಧ್ರುವ ಶೂನ್ಯ* datap, size_t length);  // Process data into the digest
    ಶೂನ್ಯ ಸೇರಿಸು(ಧ್ರುವ string& data) {
        ಸೇರಿಸು(data.data(), data.length());
    }  // Process data into the digest
    ಶೂನ್ಯ ಸೇರಿಸು(uint64_t ಮೌಲ್ಯ) { ಸೇರಿಸು(cvtToStr(ಮೌಲ್ಯ)); }
    ಶೂನ್ಯ ಸೇರಿಸು_File(ಧ್ರುವ string& filename);

ಗುಪ್ತ:
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestOne(ಧ್ರುವ string& data, ಧ್ರುವ string& data2, ಧ್ರುವ string& exp,
                            ಧ್ರುವ string& exp64, ಧ್ರುವ string& exp24);
    ಶೂನ್ಯ finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ಸ್ಥಿರ std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ಸ್ಥಿರ size_t s_maxLength;  // Length at which to start hashing
    ಸ್ಥಿರ size_t s_minLength;  // Length to preserve if over maxLength

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit VName(ಧ್ರುವ string& name)
        : m_name{name} {}
    ~VName() = ಸ್ವಭಾವ;
    // METHODS
    ಶೂನ್ಯ name(ಧ್ರುವ string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ಸ್ಥಿರ ಶೂನ್ಯ maxLength(size_t flag) { s_maxLength = flag; }
    ಸ್ಥಿರ string dehash(ಧ್ರುವ string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ಸ್ಥಿರ constexpr unsigned ಅಂಕೆ_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    ಸ್ಥಿರ constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned ಪೂರ್ಣಾಙ್ಕ;
    ಸಂರಚನಾ Candidate final {
        std::string str;  // String to be matched
        std::string ಸಂದೇಶ;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VSpellCheck() = ಸ್ವಭಾವ;
    ~VSpellCheck() = ಸ್ವಭಾವ;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    ಶೂನ್ಯ ತಳ್ಳು_Candidate(ಧ್ರುವ string& str) { ತಳ್ಳು_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    ಶೂನ್ಯ ತಳ್ಳು_Candidate(ಧ್ರುವ string& str, ಧ್ರುವ string& ಸಂದೇಶ) {
        ಯದಿ (m_candidates.size() < ಅಂಕೆ_CANDIDATE_LIMIT) m_candidates.ತಳ್ಳು_back({str, ಸಂದೇಶ});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(ಧ್ರುವ string& goal) ಧ್ರುವ {
        EditDistance dist;
        ಪ್ರತ್ಯಾಗಮ bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_ಸಂದೇಶ(ಧ್ರುವ string& goal) ಧ್ರುವ {
        ಧ್ರುವ auto candidate = bestCandidate(goal);
        ಯದಿ (candidate.ಸಂದೇಶ.empty()) ಪ್ರತ್ಯಾಗಮ "";
        ಪ್ರತ್ಯಾಗಮ "... Suggested alternative: '"s + candidate.ಸಂದೇಶ + "'";
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
    std::vector<std::string> candidates() ಧ್ರುವ {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        ಪ್ರತಿ (ಧ್ರುವ auto& candidate : m_candidates) { result.ತಳ್ಳು_back(candidate.ಸಂದೇಶ); }
        ಪ್ರತ್ಯಾಗಮ result;
    }

ಗುಪ್ತ:
    ಸ್ಥಿರ EditDistance editDistance(ಧ್ರುವ string& s, ಧ್ರುವ string& t);
    ಸ್ಥಿರ EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(ಧ್ರುವ string& goal, EditDistance& distancer) ಧ್ರುವ;
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestDistanceOne(ಧ್ರುವ string& a, ಧ್ರುವ string& b, EditDistance expected);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestSuggestOne(bool matches, ಧ್ರುವ string& c, ಧ್ರುವ string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
