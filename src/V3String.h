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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#அந்தர்பாவ <deque>
#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <sstream>
#அந்தர்பாவ <string>
#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(மாறிலி T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    திரும்பு os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::மதிப்பு, std::string>::type
cvtToHex(மாறிலி T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<மாறிலி வெறுமை*>(tp);
    திரும்பு os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::மதிப்பு, std::string>::type cvtToHex(மாறிலி T t) {
    std::ostringstream os;
    os << std::hex << std::setw(அளவு(T) * 8 / 4) << std::setfill('0') << t;
    திரும்பு os.str();
}

உள்வரி string ucfirst(மாறிலி string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    திரும்பு result;
}

//######################################################################
// VString - String manipulation

class VString final {
    நிலையான bool wildmatchi(மாறிலி எழுத்து* s, மாறிலி எழுத்து* p) VL_PURE;

பொது:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    நிலையான bool wildmatch(மாறிலி எழுத்து* s, மாறிலி எழுத்து* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    நிலையான bool wildmatch(மாறிலி string& s, மாறிலி string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    நிலையான string dot(மாறிலி string& a, மாறிலி string& dot, மாறிலி string& b);
    // Convert string to lowercase (tolower)
    நிலையான string downcase(மாறிலி string& str) VL_PURE;
    // Convert string to upper case (toupper)
    நிலையான string upcase(மாறிலி string& str) VL_PURE;
    // Insert esc just before tgt
    நிலையான string quoteAny(மாறிலி string& str, எழுத்து tgt, எழுத்து esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    நிலையான string quoteBackslash(மாறிலி string& str) VL_PURE { திரும்பு quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    நிலையான string quotePercent(மாறிலி string& str) VL_PURE { திரும்பு quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    நிலையான string dequotePercent(மாறிலி string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    நிலையான string escapeStringForPath(மாறிலி string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    நிலையான string unquoteSVString(மாறிலி string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    நிலையான size_t quotedEnd(மாறிலி string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    நிலையான string quoteStringLiteralForShell(மாறிலி string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    நிலையான string spaceUnprintable(மாறிலி string& str) VL_PURE;
    // Remove any whitespace
    நிலையான string removeWhitespace(மாறிலி string& str);
    // Trim leading/trailing whitespace on each line
    நிலையான string trimWhitespace(மாறிலி string& str);
    // Return true if only identifer or ""
    நிலையான bool isIdentifier(மாறிலி string& str);
    // Return true if char is valid character in C identifiers
    நிலையான bool isIdentifierChar(எழுத்து c) { திரும்பு isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    நிலையான bool isWhitespace(மாறிலி string& str);
    // Return number of spaces/tabs leading in string
    நிலையான string::size_type leading_Whitespace_மொத்தம்(மாறிலி string& str);
    // Return double by parsing string
    நிலையான இரட்டை parseDouble(மாறிலி string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    நிலையான string replaceSubstr(மாறிலி string& str, மாறிலி string& from, மாறிலி string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    நிலையான string replaceWord(மாறிலி string& str, மாறிலி string& from, மாறிலி string& to);
    // Return deque of strings split by the given delimiter
    நிலையான std::deque<string> split(மாறிலி string& str, எழுத்து delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    நிலையான bool startsWith(மாறிலி string& str, மாறிலி எழுத்து* prefixp) {
        திரும்பு !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    நிலையான bool startsWith(மாறிலி string& str, மாறிலி string& prefix) {
        திரும்பு !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    நிலையான bool endsWith(மாறிலி string& str, மாறிலி string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    நிலையான string aOrAn(மாறிலி எழுத்து* word);
    நிலையான string aOrAn(மாறிலி string& word) { திரும்பு aOrAn(word.c_str()); }
    // Hash the string
    நிலையான uint64_t hashMurmur(மாறிலி string& str) VL_PURE;

    // Base 64 encode
    நிலையான string base64Enc(மாறிலி string& str) VL_PURE;

    நிலையான வெறுமை selfTest();  // Test this class
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
பொது:
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
    explicit VHashSha512(மாறிலி string& data)
        : VHashSha512{} {
        செருகு(data);
    }
    ~VHashSha512() = இயல்பு;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    நிலையான constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_இருமம்();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    நிலையான வெறுமை selfTest();  // Test this class

    // Inserting hash data
    வெறுமை செருகு(மாறிலி வெறுமை* datap, size_t length);  // Process data into the digest
    வெறுமை செருகு(மாறிலி string& data) {
        செருகு(data.data(), data.length());
    }  // Process data into the digest
    வெறுமை செருகு(uint64_t மதிப்பு) { செருகு(cvtToStr(மதிப்பு)); }
    வெறுமை செருகு_File(மாறிலி string& filename);

தனிப்பட்ட:
    நிலையான வெறுமை selfTestOne(மாறிலி string& data, மாறிலி string& data2, மாறிலி string& exp,
                            மாறிலி string& exp64, மாறிலி string& exp24);
    வெறுமை finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    நிலையான std::map<string, string> s_dehashMap;  // hashed -> original decoder

    நிலையான size_t s_maxLength;  // Length at which to start hashing
    நிலையான size_t s_minLength;  // Length to preserve if over maxLength

பொது:
    // CONSTRUCTORS
    explicit VName(மாறிலி string& name)
        : m_name{name} {}
    ~VName() = இயல்பு;
    // METHODS
    வெறுமை name(மாறிலி string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() மாறிலி { திரும்பு m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    நிலையான வெறுமை maxLength(size_t flag) { s_maxLength = flag; }
    நிலையான string dehash(மாறிலி string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    நிலையான constexpr unsigned இலக்கம்_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    நிலையான constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned முழு;
    கட்டமைப்பு Candidate final {
        std::string str;  // String to be matched
        std::string செய்தி;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
பொது:
    // CONSTRUCTORS
    VSpellCheck() = இயல்பு;
    ~VSpellCheck() = இயல்பு;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    வெறுமை தள்ளு_Candidate(மாறிலி string& str) { தள்ளு_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    வெறுமை தள்ளு_Candidate(மாறிலி string& str, மாறிலி string& செய்தி) {
        எனில் (m_candidates.size() < இலக்கம்_CANDIDATE_LIMIT) m_candidates.தள்ளு_back({str, செய்தி});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(மாறிலி string& goal) மாறிலி {
        EditDistance dist;
        திரும்பு bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_செய்தி(மாறிலி string& goal) மாறிலி {
        மாறிலி auto candidate = bestCandidate(goal);
        எனில் (candidate.செய்தி.empty()) திரும்பு "";
        திரும்பு "... Suggested alternative: '"s + candidate.செய்தி + "'";
    }
    நிலையான வெறுமை selfTest();
    std::vector<std::string> candidates() மாறிலி {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        க்கு (மாறிலி auto& candidate : m_candidates) { result.தள்ளு_back(candidate.செய்தி); }
        திரும்பு result;
    }

தனிப்பட்ட:
    நிலையான EditDistance editDistance(மாறிலி string& s, மாறிலி string& t);
    நிலையான EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(மாறிலி string& goal, EditDistance& distancer) மாறிலி;
    நிலையான வெறுமை selfTestDistanceOne(மாறிலி string& a, மாறிலி string& b, EditDistance expected);
    நிலையான வெறுமை selfTestSuggestOne(bool matches, மாறிலி string& c, மாறிலி string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
