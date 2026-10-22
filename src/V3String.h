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

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(ثابت T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    أرجع os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::value, std::string>::type
cvtToHex(ثابت T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<ثابت فراغ*>(tp);
    أرجع os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::value, std::string>::type cvtToHex(ثابت T t) {
    std::ostringstream os;
    os << std::hex << std::setw(حجم(T) * 8 / 4) << std::setfill('0') << t;
    أرجع os.str();
}

مضمن string ucfirst(ثابت string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    أرجع result;
}

//######################################################################
// VString - String manipulation

class VString final {
    ساكن bool wildmatchi(ثابت حرف* s, ثابت حرف* p) VL_PURE;

عام:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ساكن bool wildmatch(ثابت حرف* s, ثابت حرف* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    ساكن bool wildmatch(ثابت string& s, ثابت string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ساكن string dot(ثابت string& a, ثابت string& dot, ثابت string& b);
    // Convert string to lowercase (tolower)
    ساكن string downcase(ثابت string& str) VL_PURE;
    // Convert string to upper case (toupper)
    ساكن string upcase(ثابت string& str) VL_PURE;
    // Insert esc just before tgt
    ساكن string quoteAny(ثابت string& str, حرف tgt, حرف esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    ساكن string quoteBackslash(ثابت string& str) VL_PURE { أرجع quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ساكن string quotePercent(ثابت string& str) VL_PURE { أرجع quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ساكن string dequotePercent(ثابت string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ساكن string escapeStringForPath(ثابت string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ساكن string unquoteSVString(ثابت string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ساكن size_t quotedEnd(ثابت string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ساكن string quoteStringLiteralForShell(ثابت string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ساكن string spaceUnprintable(ثابت string& str) VL_PURE;
    // Remove any whitespace
    ساكن string removeWhitespace(ثابت string& str);
    // Trim leading/trailing whitespace on each line
    ساكن string trimWhitespace(ثابت string& str);
    // Return true if only identifer or ""
    ساكن bool isIdentifier(ثابت string& str);
    // Return true if char is valid character in C identifiers
    ساكن bool isIdentifierChar(حرف c) { أرجع isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    ساكن bool isWhitespace(ثابت string& str);
    // Return number of spaces/tabs leading in string
    ساكن string::size_type leadingWhitespaceCount(ثابت string& str);
    // Return double by parsing string
    ساكن مزدوج parseDouble(ثابت string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ساكن string replaceSubstr(ثابت string& str, ثابت string& from, ثابت string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ساكن string replaceWord(ثابت string& str, ثابت string& from, ثابت string& to);
    // Return deque of strings split by the given delimiter
    ساكن std::deque<string> split(ثابت string& str, حرف delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ساكن bool startsWith(ثابت string& str, ثابت حرف* prefixp) {
        أرجع !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ساكن bool startsWith(ثابت string& str, ثابت string& prefix) {
        أرجع !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ساكن bool endsWith(ثابت string& str, ثابت string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ساكن string aOrAn(ثابت حرف* word);
    ساكن string aOrAn(ثابت string& word) { أرجع aOrAn(word.c_str()); }
    // Hash the string
    ساكن uint64_t hashMurmur(ثابت string& str) VL_PURE;

    // Base 64 encode
    ساكن string base64Enc(ثابت string& str) VL_PURE;

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
    size_t m_totLength = 0;  // Total all-chunk length as needed by output digest
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
    explicit VHashSha512(ثابت string& data)
        : VHashSha512{} {
        insert(data);
    }
    ~VHashSha512() = افتراضي;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ساكن constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digestBinary();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ساكن فراغ selfTest();  // Test this class

    // Inserting hash data
    فراغ insert(ثابت فراغ* datap, size_t length);  // Process data into the digest
    فراغ insert(ثابت string& data) {
        insert(data.data(), data.length());
    }  // Process data into the digest
    فراغ insert(uint64_t value) { insert(cvtToStr(value)); }
    فراغ insertFile(ثابت string& filename);

خاص:
    ساكن فراغ selfTestOne(ثابت string& data, ثابت string& data2, ثابت string& exp,
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

    ساكن size_t s_maxLength;  // Length at which to start hashing
    ساكن size_t s_minLength;  // Length to preserve if over maxLength

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
    ساكن فراغ maxLength(size_t flag) { s_maxLength = flag; }
    ساكن string dehash(ثابت string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ساكن constexpr unsigned NUM_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    ساكن constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned صحيح;
    بنية Candidate final {
        std::string str;  // String to be matched
        std::string msg;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
عام:
    // CONSTRUCTORS
    VSpellCheck() = افتراضي;
    ~VSpellCheck() = افتراضي;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    فراغ pushCandidate(ثابت string& str) { pushCandidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    فراغ pushCandidate(ثابت string& str, ثابت string& msg) {
        𐡄𐡍 (m_candidates.size() < NUM_CANDIDATE_LIMIT) m_candidates.push_back({str, msg});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(ثابت string& goal) ثابت {
        EditDistance dist;
        أرجع bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string bestCandidateMsg(ثابت string& goal) ثابت {
        ثابت auto candidate = bestCandidate(goal);
        𐡄𐡍 (candidate.msg.empty()) أرجع "";
        أرجع "... Suggested alternative: '"s + candidate.msg + "'";
    }
    ساكن فراغ selfTest();
    std::vector<std::string> candidates() ثابت {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        لكل (ثابت auto& candidate : m_candidates) { result.push_back(candidate.msg); }
        أرجع result;
    }

خاص:
    ساكن EditDistance editDistance(ثابت string& s, ثابت string& t);
    ساكن EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(ثابت string& goal, EditDistance& distancer) ثابت;
    ساكن فراغ selfTestDistanceOne(ثابت string& a, ثابت string& b, EditDistance expected);
    ساكن فراغ selfTestSuggestOne(bool matches, ثابت string& c, ثابت string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
