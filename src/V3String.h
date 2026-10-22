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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <sstream>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <type_traits>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

template <typename T>
std::string cvtToStr(અચલ T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    પાછા os.str();
}
template <typename T>
typename std::enable_if<std::is_pointer<T>::કિંમત, std::string>::type
cvtToHex(અચલ T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<અચલ શૂન્ય*>(tp);
    પાછા os.str();
}
template <typename T>
typename std::enable_if<std::is_integral<T>::કિંમત, std::string>::type cvtToHex(અચલ T t) {
    std::ostringstream os;
    os << std::hex << std::setw(કદ(T) * 8 / 4) << std::setfill('0') << t;
    પાછા os.str();
}

અંતઃરેખ string ucfirst(અચલ string& text) {
    string result = text;
    result[0] = std::toupper(result[0]);
    પાછા result;
}

//######################################################################
// VString - String manipulation

class VString final {
    સ્થિર bool wildmatchi(અચલ અક્ષર* s, અચલ અક્ષર* p) VL_PURE;

જાહેર:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    સ્થિર bool wildmatch(અચલ અક્ષર* s, અચલ અક્ષર* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    સ્થિર bool wildmatch(અચલ string& s, અચલ string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    સ્થિર string dot(અચલ string& a, અચલ string& dot, અચલ string& b);
    // Convert string to lowercase (tolower)
    સ્થિર string downcase(અચલ string& str) VL_PURE;
    // Convert string to upper case (toupper)
    સ્થિર string upcase(અચલ string& str) VL_PURE;
    // Insert esc just before tgt
    સ્થિર string quoteAny(અચલ string& str, અક્ષર tgt, અક્ષર esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    સ્થિર string quoteBackslash(અચલ string& str) VL_PURE { પાછા quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    સ્થિર string quotePercent(અચલ string& str) VL_PURE { પાછા quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    સ્થિર string dequotePercent(અચલ string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    સ્થિર string escapeStringForPath(અચલ string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    સ્થિર string unquoteSVString(અચલ string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    સ્થિર size_t quotedEnd(અચલ string& str, size_t pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    સ્થિર string quoteStringLiteralForShell(અચલ string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    સ્થિર string spaceUnprintable(અચલ string& str) VL_PURE;
    // Remove any whitespace
    સ્થિર string removeWhitespace(અચલ string& str);
    // Trim leading/trailing whitespace on each line
    સ્થિર string trimWhitespace(અચલ string& str);
    // Return true if only identifer or ""
    સ્થિર bool isIdentifier(અચલ string& str);
    // Return true if char is valid character in C identifiers
    સ્થિર bool isIdentifierChar(અક્ષર c) { પાછા isalnum(c) || c == '_'; }
    // Return true if only whitespace or ""
    સ્થિર bool isWhitespace(અચલ string& str);
    // Return number of spaces/tabs leading in string
    સ્થિર string::size_type leading_Whitespace_ગણતરી(અચલ string& str);
    // Return double by parsing string
    સ્થિર દ્વિગુણ parseDouble(અચલ string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    સ્થિર string replaceSubstr(અચલ string& str, અચલ string& from, અચલ string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    સ્થિર string replaceWord(અચલ string& str, અચલ string& from, અચલ string& to);
    // Return deque of strings split by the given delimiter
    સ્થિર std::deque<string> split(અચલ string& str, અક્ષર delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    સ્થિર bool startsWith(અચલ string& str, અચલ અક્ષર* prefixp) {
        પાછા !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    સ્થિર bool startsWith(અચલ string& str, અચલ string& prefix) {
        પાછા !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    સ્થિર bool endsWith(અચલ string& str, અચલ string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    સ્થિર string aOrAn(અચલ અક્ષર* word);
    સ્થિર string aOrAn(અચલ string& word) { પાછા aOrAn(word.c_str()); }
    // Hash the string
    સ્થિર uint64_t hashMurmur(અચલ string& str) VL_PURE;

    // Base 64 encode
    સ્થિર string base64Enc(અચલ string& str) VL_PURE;

    સ્થિર શૂન્ય selfTest();  // Test this class
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
જાહેર:
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
    explicit VHashSha512(અચલ string& data)
        : VHashSha512{} {
        ઉમેરો(data);
    }
    ~VHashSha512() = મૂળભૂત;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    સ્થિર constexpr size_t DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_દ્વિઅંકી();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    સ્થિર શૂન્ય selfTest();  // Test this class

    // Inserting hash data
    શૂન્ય ઉમેરો(અચલ શૂન્ય* datap, size_t length);  // Process data into the digest
    શૂન્ય ઉમેરો(અચલ string& data) {
        ઉમેરો(data.data(), data.length());
    }  // Process data into the digest
    શૂન્ય ઉમેરો(uint64_t કિંમત) { ઉમેરો(cvtToStr(કિંમત)); }
    શૂન્ય ઉમેરો_File(અચલ string& filename);

ખાનગી:
    સ્થિર શૂન્ય selfTestOne(અચલ string& data, અચલ string& data2, અચલ string& exp,
                            અચલ string& exp64, અચલ string& exp24);
    શૂન્ય finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    સ્થિર std::map<string, string> s_dehashMap;  // hashed -> original decoder

    સ્થિર size_t s_maxLength;  // Length at which to start hashing
    સ્થિર size_t s_minLength;  // Length to preserve if over maxLength

જાહેર:
    // CONSTRUCTORS
    explicit VName(અચલ string& name)
        : m_name{name} {}
    ~VName() = મૂળભૂત;
    // METHODS
    શૂન્ય name(અચલ string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() અચલ { પાછા m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    સ્થિર શૂન્ય maxLength(size_t flag) { s_maxLength = flag; }
    સ્થિર string dehash(અચલ string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    સ્થિર constexpr unsigned અંક_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    સ્થિર constexpr unsigned LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = unsigned પૂર્ણાંક;
    સંરચના Candidate final {
        std::string str;  // String to be matched
        std::string સંદેશ;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
જાહેર:
    // CONSTRUCTORS
    VSpellCheck() = મૂળભૂત;
    ~VSpellCheck() = મૂળભૂત;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    શૂન્ય ધકેલો_Candidate(અચલ string& str) { ધકેલો_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    શૂન્ય ધકેલો_Candidate(અચલ string& str, અચલ string& સંદેશ) {
        જો (m_candidates.size() < અંક_CANDIDATE_LIMIT) m_candidates.ધકેલો_back({str, સંદેશ});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(અચલ string& goal) અચલ {
        EditDistance dist;
        પાછા bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_સંદેશ(અચલ string& goal) અચલ {
        અચલ auto candidate = bestCandidate(goal);
        જો (candidate.સંદેશ.empty()) પાછા "";
        પાછા "... Suggested alternative: '"s + candidate.સંદેશ + "'";
    }
    સ્થિર શૂન્ય selfTest();
    std::vector<std::string> candidates() અચલ {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        માટે (અચલ auto& candidate : m_candidates) { result.ધકેલો_back(candidate.સંદેશ); }
        પાછા result;
    }

ખાનગી:
    સ્થિર EditDistance editDistance(અચલ string& s, અચલ string& t);
    સ્થિર EditDistance cutoffDistance(size_t goal_len, size_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(અચલ string& goal, EditDistance& distancer) અચલ;
    સ્થિર શૂન્ય selfTestDistanceOne(અચલ string& a, અચલ string& b, EditDistance expected);
    સ્થિર શૂન્ય selfTestSuggestOne(bool matches, અચલ string& c, અચલ string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
