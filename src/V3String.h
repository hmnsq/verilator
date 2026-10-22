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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <sstream>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(സ്ഥിരം T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    തിരികെ os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::മൂല്യം, std::string>::type
cvtToHex(സ്ഥിരം T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<സ്ഥിരം ശൂന്യം*>(tp);
    തിരികെ os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::മൂല്യം, std::string>::type cvtToHex(സ്ഥിരം T t) {
    std::ostringstream os;
    os << std::hex << std::setw(വലുപ്പം(T) * 8 / 4) << std::setfill('0') << t;
    തിരികെ os.str();
}

വരിയിൽ string ucfirst(സ്ഥിരം string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    തിരികെ result;
}

//######################################################################
// VString - String manipulation

class VString final {
    സ്ഥിതം bool wildmatchi(സ്ഥിരം അക്ഷരം* s, സ്ഥിരം അക്ഷരം* p) VL_PURE;

പൊതു:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    സ്ഥിതം bool wildmatch(സ്ഥിരം അക്ഷരം* s, സ്ഥിരം അക്ഷരം* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    സ്ഥിതം bool wildmatch(സ്ഥിരം string& s, സ്ഥിരം string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    സ്ഥിതം string dot(സ്ഥിരം string& a, സ്ഥിരം string& dot, സ്ഥിരം string& b);
    // Convert string to lowercase (tolower)
    സ്ഥിതം string downcase(സ്ഥിരം string& str) VL_PURE;
    // Convert string to upper case (toupper)
    സ്ഥിതം string upcase(സ്ഥിരം string& str) VL_PURE;
    // Insert esc just before tgt
    സ്ഥിതം string quoteAny(സ്ഥിരം string& str, അക്ഷരം tgt, അക്ഷരം esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    സ്ഥിതം string quoteBackslash(സ്ഥിരം string& str) VL_PURE { തിരികെ quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    സ്ഥിതം string quotePercent(സ്ഥിരം string& str) VL_PURE { തിരികെ quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    സ്ഥിതം string dequotePercent(സ്ഥിരം string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    സ്ഥിതം string escapeStringForPath(സ്ഥിരം string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    സ്ഥിതം string unquoteSVString(സ്ഥിരം string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    സ്ഥിതം size_t quotedEnd(സ്ഥിരം string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    സ്ഥിതം string quoteStringLiteralForShell(സ്ഥിരം string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    സ്ഥിതം string spaceUnprintable(സ്ഥിരം string& str) VL_PURE;
    // Remove any whitespace
    സ്ഥിതം string removeWhitespace(സ്ഥിരം string& str);
    // Trim leading/trailing whitespace on each line
    സ്ഥിതം string trimWhitespace(സ്ഥിരം string& str);
    // Return true if only identifer or ""
    സ്ഥിതം bool isIdentifier(സ്ഥിരം string& str);
    // Return true if char is valid character in C identifiers
    സ്ഥിതം bool isIdentifierChar(അക്ഷരം c) { തിരികെ isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    സ്ഥിതം bool isWhitespace(സ്ഥിരം string& str);
    // Return number of spaces/tabs leading in string
    സ്ഥിതം string::size_type leading_Whitespace_എണ്ണം(സ്ഥിരം string& str);
    // Return double by parsing string
    സ്ഥിതം ഇരട്ട parseDouble(സ്ഥിരം string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    സ്ഥിതം string replaceSubstr(സ്ഥിരം string& str, സ്ഥിരം string& from, സ്ഥിരം string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    സ്ഥിതം string replaceWord(സ്ഥിരം string& str, സ്ഥിരം string& from, സ്ഥിരം string& to);
    // Return deque of strings split by the given delimiter
    സ്ഥിതം std::deque<string> split(സ്ഥിരം string& str, അക്ഷരം delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    സ്ഥിതം bool startsWith(സ്ഥിരം string& str, സ്ഥിരം അക്ഷരം* prefixp) {
        തിരികെ !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    സ്ഥിതം bool startsWith(സ്ഥിരം string& str, സ്ഥിരം string& prefix) {
        തിരികെ !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    സ്ഥിതം bool endsWith(സ്ഥിരം string& str, സ്ഥിരം string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    സ്ഥിതം string aOrAn(സ്ഥിരം അക്ഷരം* word);
    സ്ഥിതം string aOrAn(സ്ഥിരം string& word) { തിരികെ aOrAn(word.c_str()); }
    // Hash the string
    സ്ഥിതം uint64_t hashMurmur(സ്ഥിരം string& str) VL_PURE;

    // Base 64 encode
    സ്ഥിതം string base64Enc(സ്ഥിരം string& str) VL_PURE;

    സ്ഥിതം ശൂന്യം selfTest();  // Test this class
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
പൊതു:
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
    explicit VHashSha512(സ്ഥിരം string& data)
        : VHashSha512{} {
        തിരുകുക(data);
    }
    ~VHashSha512() = സ്ഥിരസ്ഥിതി;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    സ്ഥിതം constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ദ്വിമാന();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    സ്ഥിതം ശൂന്യം selfTest();  // Test this class

    // Inserting hash data
    ശൂന്യം തിരുകുക(സ്ഥിരം ശൂന്യം* datap, size_t length);  // Process data into the digest
    ശൂന്യം തിരുകുക(സ്ഥിരം string& data) {
        തിരുകുക(data.data(), data.length());
    }  // Process data into the digest
    ശൂന്യം തിരുകുക(uint64_t മൂല്യം) { തിരുകുക(cvtToStr(മൂല്യം)); }
    ശൂന്യം തിരുകുക_File(സ്ഥിരം string& filename);

സ്വകാര്യ:
    സ്ഥിതം ശൂന്യം selfTestOne(സ്ഥിരം string& data, സ്ഥിരം string& data2, സ്ഥിരം string& exp,
                            സ്ഥിരം string& exp64, സ്ഥിരം string& exp24);
    ശൂന്യം finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    സ്ഥിതം std::map<string, string> s_dehashMap;  // hashed -> original decoder

    സ്ഥിതം size_t s_maxLength;  // Length at which to start hashing
    സ്ഥിതം size_t s_minLength;  // Length to preserve if over maxLength

പൊതു:
    // CONSTRUCTORS
    explicit VName(സ്ഥിരം string& name)
        : m_name{name} {}
    ~VName() = സ്ഥിരസ്ഥിതി;
    // METHODS
    ശൂന്യം name(സ്ഥിരം string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() സ്ഥിരം { തിരികെ m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    സ്ഥിതം ശൂന്യം maxLength(size_t flag) { s_maxLength = flag; }
    സ്ഥിതം string dehash(സ്ഥിരം string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    സ്ഥിതം constexpr unsigned അക്കം_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    സ്ഥിതം constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned പൂർണ്ണസംഖ്യ;
    ഘടന Candidate final {
        std::string str;  // String to be matched
        std::string സന്ദേശം;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
പൊതു:
    // CONSTRUCTORS
    VSpellCheck() = സ്ഥിരസ്ഥിതി;
    ~VSpellCheck() = സ്ഥിരസ്ഥിതി;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    ശൂന്യം തള്ളുക_Candidate(സ്ഥിരം string& str) { തള്ളുക_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    ശൂന്യം തള്ളുക_Candidate(സ്ഥിരം string& str, സ്ഥിരം string& സന്ദേശം) {
        എങ്കിൽ (m_candidates.size() < അക്കം_CANDIDATE_LIMIT) m_candidates.തള്ളുക_back({str, സന്ദേശം});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(സ്ഥിരം string& goal) സ്ഥിരം {
        EditDistance dist;
        തിരികെ bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_സന്ദേശം(സ്ഥിരം string& goal) സ്ഥിരം {
        സ്ഥിരം auto candidate = bestCandidate(goal);
        എങ്കിൽ (candidate.സന്ദേശം.empty()) തിരികെ "";
        തിരികെ "... Suggested alternative: '"s + candidate.സന്ദേശം + "'";
    }
    സ്ഥിതം ശൂന്യം selfTest();
    std::vector<std::string> candidates() സ്ഥിരം {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        വേണ്ടി (സ്ഥിരം auto& candidate : m_candidates) { result.തള്ളുക_back(candidate.സന്ദേശം); }
        തിരികെ result;
    }

സ്വകാര്യ:
    സ്ഥിതം EditDistance editDistance(സ്ഥിരം string& s, സ്ഥിരം string& t);
    സ്ഥിതം EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(സ്ഥിരം string& goal, EditDistance& distancer) സ്ഥിരം;
    സ്ഥിതം ശൂന്യം selfTestDistanceOne(സ്ഥിരം string& a, സ്ഥിരം string& b, EditDistance expected);
    സ്ഥിതം ശൂന്യം selfTestSuggestOne(bool matches, സ്ഥിരം string& c, സ്ഥിരം string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
