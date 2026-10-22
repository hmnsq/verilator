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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#অন্তর্ভাব <deque>
#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <sstream>
#অন্তর্ভাব <string>
#অন্তর্ভাব <type_traits>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(ধ্রুব T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    প্রত্যাগম os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::মান, std::string>::type
cvtToHex(ধ্রুব T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<ধ্রুব শূন্য*>(tp);
    প্রত্যাগম os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::মান, std::string>::type cvtToHex(ধ্রুব T t) {
    std::ostringstream os;
    os << std::hex << std::setw(পরিমাণ(T) * 8 / 4) << std::setfill('0') << t;
    প্রত্যাগম os.str();
}

অন্তঃস্থ string ucfirst(ধ্রুব string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    প্রত্যাগম result;
}

//######################################################################
// VString - String manipulation

class VString final {
    স্থির bool wildmatchi(ধ্রুব বর্ণ* s, ধ্রুব বর্ণ* p) VL_PURE;

সার্বজনিক:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    স্থির bool wildmatch(ধ্রুব বর্ণ* s, ধ্রুব বর্ণ* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    স্থির bool wildmatch(ধ্রুব string& s, ধ্রুব string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    স্থির string dot(ধ্রুব string& a, ধ্রুব string& dot, ধ্রুব string& b);
    // Convert string to lowercase (tolower)
    স্থির string downcase(ধ্রুব string& str) VL_PURE;
    // Convert string to upper case (toupper)
    স্থির string upcase(ধ্রুব string& str) VL_PURE;
    // Insert esc just before tgt
    স্থির string quoteAny(ধ্রুব string& str, বর্ণ tgt, বর্ণ esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    স্থির string quoteBackslash(ধ্রুব string& str) VL_PURE { প্রত্যাগম quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    স্থির string quotePercent(ধ্রুব string& str) VL_PURE { প্রত্যাগম quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    স্থির string dequotePercent(ধ্রুব string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    স্থির string escapeStringForPath(ধ্রুব string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    স্থির string unquoteSVString(ধ্রুব string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    স্থির size_t quotedEnd(ধ্রুব string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    স্থির string quoteStringLiteralForShell(ধ্রুব string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    স্থির string spaceUnprintable(ধ্রুব string& str) VL_PURE;
    // Remove any whitespace
    স্থির string removeWhitespace(ধ্রুব string& str);
    // Trim leading/trailing whitespace on each line
    স্থির string trimWhitespace(ধ্রুব string& str);
    // Return true if only identifer or ""
    স্থির bool isIdentifier(ধ্রুব string& str);
    // Return true if char is valid character in C identifiers
    স্থির bool isIdentifierChar(বর্ণ c) { প্রত্যাগম isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    স্থির bool isWhitespace(ধ্রুব string& str);
    // Return number of spaces/tabs leading in string
    স্থির string::size_type leading_Whitespace_গুনতি(ধ্রুব string& str);
    // Return double by parsing string
    স্থির দ্বিগুণ parseDouble(ধ্রুব string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    স্থির string replaceSubstr(ধ্রুব string& str, ধ্রুব string& from, ধ্রুব string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    স্থির string replaceWord(ধ্রুব string& str, ধ্রুব string& from, ধ্রুব string& to);
    // Return deque of strings split by the given delimiter
    স্থির std::deque<string> split(ধ্রুব string& str, বর্ণ delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    স্থির bool startsWith(ধ্রুব string& str, ধ্রুব বর্ণ* prefixp) {
        প্রত্যাগম !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    স্থির bool startsWith(ধ্রুব string& str, ধ্রুব string& prefix) {
        প্রত্যাগম !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    স্থির bool endsWith(ধ্রুব string& str, ধ্রুব string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    স্থির string aOrAn(ধ্রুব বর্ণ* word);
    স্থির string aOrAn(ধ্রুব string& word) { প্রত্যাগম aOrAn(word.c_str()); }
    // Hash the string
    স্থির uint64_t hashMurmur(ধ্রুব string& str) VL_PURE;

    // Base 64 encode
    স্থির string base64Enc(ধ্রুব string& str) VL_PURE;

    স্থির শূন্য selfTest();  // Test this class
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
সার্বজনিক:
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
    explicit VHashSha512(ধ্রুব string& data)
        : VHashSha512{} {
        ঢোকাও(data);
    }
    ~VHashSha512() = স্বভাব;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    স্থির constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_দ্বিমিক();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    স্থির শূন্য selfTest();  // Test this class

    // Inserting hash data
    শূন্য ঢোকাও(ধ্রুব শূন্য* datap, size_t length);  // Process data into the digest
    শূন্য ঢোকাও(ধ্রুব string& data) {
        ঢোকাও(data.data(), data.length());
    }  // Process data into the digest
    শূন্য ঢোকাও(uint64_t মান) { ঢোকাও(cvtToStr(মান)); }
    শূন্য ঢোকাও_File(ধ্রুব string& filename);

গুপ্ত:
    স্থির শূন্য selfTestOne(ধ্রুব string& data, ধ্রুব string& data2, ধ্রুব string& exp,
                            ধ্রুব string& exp64, ধ্রুব string& exp24);
    শূন্য finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    স্থির std::map<string, string> s_dehashMap;  // hashed -> original decoder

    স্থির size_t s_maxLength;  // Length at which to start hashing
    স্থির size_t s_minLength;  // Length to preserve if over maxLength

সার্বজনিক:
    // CONSTRUCTORS
    explicit VName(ধ্রুব string& name)
        : m_name{name} {}
    ~VName() = স্বভাব;
    // METHODS
    শূন্য name(ধ্রুব string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ধ্রুব { প্রত্যাগম m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    স্থির শূন্য maxLength(size_t flag) { s_maxLength = flag; }
    স্থির string dehash(ধ্রুব string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    স্থির constexpr unsigned অঙ্ক_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    স্থির constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned পূর্ণাঙ্ক;
    সংরচনা Candidate final {
        std::string str;  // String to be matched
        std::string বার্তা;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
সার্বজনিক:
    // CONSTRUCTORS
    VSpellCheck() = স্বভাব;
    ~VSpellCheck() = স্বভাব;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    শূন্য ঠেলো_Candidate(ধ্রুব string& str) { ঠেলো_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    শূন্য ঠেলো_Candidate(ধ্রুব string& str, ধ্রুব string& বার্তা) {
        যদি (m_candidates.size() < অঙ্ক_CANDIDATE_LIMIT) m_candidates.ঠেলো_back({str, বার্তা});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(ধ্রুব string& goal) ধ্রুব {
        EditDistance dist;
        প্রত্যাগম bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_বার্তা(ধ্রুব string& goal) ধ্রুব {
        ধ্রুব auto candidate = bestCandidate(goal);
        যদি (candidate.বার্তা.empty()) প্রত্যাগম "";
        প্রত্যাগম "... Suggested alternative: '"s + candidate.বার্তা + "'";
    }
    স্থির শূন্য selfTest();
    std::vector<std::string> candidates() ধ্রুব {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        প্রতি (ধ্রুব auto& candidate : m_candidates) { result.ঠেলো_back(candidate.বার্তা); }
        প্রত্যাগম result;
    }

গুপ্ত:
    স্থির EditDistance editDistance(ধ্রুব string& s, ধ্রুব string& t);
    স্থির EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(ধ্রুব string& goal, EditDistance& distancer) ধ্রুব;
    স্থির শূন্য selfTestDistanceOne(ধ্রুব string& a, ধ্রুব string& b, EditDistance expected);
    স্থির শূন্য selfTestSuggestOne(bool matches, ধ্রুব string& c, ধ্রুব string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
