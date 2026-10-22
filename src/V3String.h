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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <sstream>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(ଅଚଳ T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    ଫେରନ୍ତୁ os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::ମୂଲ୍ୟ, std::string>::type
cvtToHex(ଅଚଳ T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<ଅଚଳ ଶୂନ୍ୟ*>(tp);
    ଫେରନ୍ତୁ os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::ମୂଲ୍ୟ, std::string>::type cvtToHex(ଅଚଳ T t) {
    std::ostringstream os;
    os << std::hex << std::setw(ଆକାର(T) * 8 / 4) << std::setfill('0') << t;
    ଫେରନ୍ତୁ os.str();
}

ଧାଡ଼ିଭିତରେ string ucfirst(ଅଚଳ string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    ଫେରନ୍ତୁ result;
}

//######################################################################
// VString - String manipulation

class VString final {
    ସ୍ଥିର bool wildmatchi(ଅଚଳ ଅକ୍ଷର* s, ଅଚଳ ଅକ୍ଷର* p) VL_PURE;

ସର୍ବସାଧାରଣ:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ସ୍ଥିର bool wildmatch(ଅଚଳ ଅକ୍ଷର* s, ଅଚଳ ଅକ୍ଷର* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    ସ୍ଥିର bool wildmatch(ଅଚଳ string& s, ଅଚଳ string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ସ୍ଥିର string dot(ଅଚଳ string& a, ଅଚଳ string& dot, ଅଚଳ string& b);
    // Convert string to lowercase (tolower)
    ସ୍ଥିର string downcase(ଅଚଳ string& str) VL_PURE;
    // Convert string to upper case (toupper)
    ସ୍ଥିର string upcase(ଅଚଳ string& str) VL_PURE;
    // Insert esc just before tgt
    ସ୍ଥିର string quoteAny(ଅଚଳ string& str, ଅକ୍ଷର tgt, ଅକ୍ଷର esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    ସ୍ଥିର string quoteBackslash(ଅଚଳ string& str) VL_PURE { ଫେରନ୍ତୁ quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ସ୍ଥିର string quotePercent(ଅଚଳ string& str) VL_PURE { ଫେରନ୍ତୁ quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ସ୍ଥିର string dequotePercent(ଅଚଳ string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ସ୍ଥିର string escapeStringForPath(ଅଚଳ string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ସ୍ଥିର string unquoteSVString(ଅଚଳ string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ସ୍ଥିର size_t quotedEnd(ଅଚଳ string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ସ୍ଥିର string quoteStringLiteralForShell(ଅଚଳ string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ସ୍ଥିର string spaceUnprintable(ଅଚଳ string& str) VL_PURE;
    // Remove any whitespace
    ସ୍ଥିର string removeWhitespace(ଅଚଳ string& str);
    // Trim leading/trailing whitespace on each line
    ସ୍ଥିର string trimWhitespace(ଅଚଳ string& str);
    // Return true if only identifer or ""
    ସ୍ଥିର bool isIdentifier(ଅଚଳ string& str);
    // Return true if char is valid character in C identifiers
    ସ୍ଥିର bool isIdentifierChar(ଅକ୍ଷର c) { ଫେରନ୍ତୁ isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    ସ୍ଥିର bool isWhitespace(ଅଚଳ string& str);
    // Return number of spaces/tabs leading in string
    ସ୍ଥିର string::size_type leading_Whitespace_ଗଣନା(ଅଚଳ string& str);
    // Return double by parsing string
    ସ୍ଥିର ଦ୍ୱିଗୁଣ parseDouble(ଅଚଳ string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ସ୍ଥିର string replaceSubstr(ଅଚଳ string& str, ଅଚଳ string& from, ଅଚଳ string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ସ୍ଥିର string replaceWord(ଅଚଳ string& str, ଅଚଳ string& from, ଅଚଳ string& to);
    // Return deque of strings split by the given delimiter
    ସ୍ଥିର std::deque<string> split(ଅଚଳ string& str, ଅକ୍ଷର delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ସ୍ଥିର bool startsWith(ଅଚଳ string& str, ଅଚଳ ଅକ୍ଷର* prefixp) {
        ଫେରନ୍ତୁ !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ସ୍ଥିର bool startsWith(ଅଚଳ string& str, ଅଚଳ string& prefix) {
        ଫେରନ୍ତୁ !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ସ୍ଥିର bool endsWith(ଅଚଳ string& str, ଅଚଳ string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ସ୍ଥିର string aOrAn(ଅଚଳ ଅକ୍ଷର* word);
    ସ୍ଥିର string aOrAn(ଅଚଳ string& word) { ଫେରନ୍ତୁ aOrAn(word.c_str()); }
    // Hash the string
    ସ୍ଥିର uint64_t hashMurmur(ଅଚଳ string& str) VL_PURE;

    // Base 64 encode
    ସ୍ଥିର string base64Enc(ଅଚଳ string& str) VL_PURE;

    ସ୍ଥିର ଶୂନ୍ୟ selfTest();  // Test this class
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
ସର୍ବସାଧାରଣ:
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
    explicit VHashSha512(ଅଚଳ string& data)
        : VHashSha512{} {
        ଭର୍ତ୍ତି(data);
    }
    ~VHashSha512() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ସ୍ଥିର constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ଦ୍ୱିମିକ();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ସ୍ଥିର ଶୂନ୍ୟ selfTest();  // Test this class

    // Inserting hash data
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(ଅଚଳ ଶୂନ୍ୟ* datap, size_t length);  // Process data into the digest
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(ଅଚଳ string& data) {
        ଭର୍ତ୍ତି(data.data(), data.length());
    }  // Process data into the digest
    ଶୂନ୍ୟ ଭର୍ତ୍ତି(uint64_t ମୂଲ୍ୟ) { ଭର୍ତ୍ତି(cvtToStr(ମୂଲ୍ୟ)); }
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_File(ଅଚଳ string& filename);

ବ୍ୟକ୍ତିଗତ:
    ସ୍ଥିର ଶୂନ୍ୟ selfTestOne(ଅଚଳ string& data, ଅଚଳ string& data2, ଅଚଳ string& exp,
                            ଅଚଳ string& exp64, ଅଚଳ string& exp24);
    ଶୂନ୍ୟ finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ସ୍ଥିର std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ସ୍ଥିର size_t s_maxLength;  // Length at which to start hashing
    ସ୍ଥିର size_t s_minLength;  // Length to preserve if over maxLength

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit VName(ଅଚଳ string& name)
        : m_name{name} {}
    ~VName() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    ଶୂନ୍ୟ name(ଅଚଳ string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ସ୍ଥିର ଶୂନ୍ୟ maxLength(size_t flag) { s_maxLength = flag; }
    ସ୍ଥିର string dehash(ଅଚଳ string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ସ୍ଥିର constexpr unsigned ଅଙ୍କ_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    ସ୍ଥିର constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned ପୂର୍ଣ୍ଣସଂଖ୍ୟା;
    ସଂରଚନା Candidate final {
        std::string str;  // String to be matched
        std::string ସନ୍ଦେଶ;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VSpellCheck() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VSpellCheck() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    ଶୂନ୍ୟ ଠେଲ_Candidate(ଅଚଳ string& str) { ଠେଲ_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    ଶୂନ୍ୟ ଠେଲ_Candidate(ଅଚଳ string& str, ଅଚଳ string& ସନ୍ଦେଶ) {
        ଯଦି (m_candidates.size() < ଅଙ୍କ_CANDIDATE_LIMIT) m_candidates.ଠେଲ_back({str, ସନ୍ଦେଶ});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(ଅଚଳ string& goal) ଅଚଳ {
        EditDistance dist;
        ଫେରନ୍ତୁ bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_ସନ୍ଦେଶ(ଅଚଳ string& goal) ଅଚଳ {
        ଅଚଳ auto candidate = bestCandidate(goal);
        ଯଦି (candidate.ସନ୍ଦେଶ.empty()) ଫେରନ୍ତୁ "";
        ଫେରନ୍ତୁ "... Suggested alternative: '"s + candidate.ସନ୍ଦେଶ + "'";
    }
    ସ୍ଥିର ଶୂନ୍ୟ selfTest();
    std::vector<std::string> candidates() ଅଚଳ {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        ପାଇଁ (ଅଚଳ auto& candidate : m_candidates) { result.ଠେଲ_back(candidate.ସନ୍ଦେଶ); }
        ଫେରନ୍ତୁ result;
    }

ବ୍ୟକ୍ତିଗତ:
    ସ୍ଥିର EditDistance editDistance(ଅଚଳ string& s, ଅଚଳ string& t);
    ସ୍ଥିର EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(ଅଚଳ string& goal, EditDistance& distancer) ଅଚଳ;
    ସ୍ଥିର ଶୂନ୍ୟ selfTestDistanceOne(ଅଚଳ string& a, ଅଚଳ string& b, EditDistance expected);
    ସ୍ଥିର ଶୂନ୍ୟ selfTestSuggestOne(bool matches, ଅଚଳ string& c, ଅଚଳ string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
