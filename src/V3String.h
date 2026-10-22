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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#अन्तर्भाव <deque>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <sstream>
#अन्तर्भाव <string>
#अन्तर्भाव <type_traits>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

ढाँचा <typename T>
std::string cvtToStr(स्थिरांक T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    परत os.str();
}
ढाँचा <typename T>
typename std::enable_if<std::is_pointer<T>::मूल्य, std::string>::type
cvtToHex(स्थिरांक T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<स्थिरांक शून्य*>(tp);
    परत os.str();
}
ढाँचा <typename T>
typename std::enable_if<std::is_integral<T>::मूल्य, std::string>::type cvtToHex(स्थिरांक T t) {
    std::ostringstream os;
    os << std::hex << std::setw(आकार(T) * 8 / 4) << std::setfill('0') << t;
    परत os.str();
}

ओळीत string ucfirst(स्थिरांक string& text) {
    string result = text;
    result[0] = std::बड़े(result[0]);
    परत result;
}

//######################################################################
// VString - String manipulation

class VString final {
    स्थिर bool wildmatchi(स्थिरांक अक्षर* s, स्थिरांक अक्षर* p) VL_PURE;

सार्वजनिक:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    स्थिर bool wildmatch(स्थिरांक अक्षर* s, स्थिरांक अक्षर* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    स्थिर bool wildmatch(स्थिरांक string& s, स्थिरांक string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    स्थिर string dot(स्थिरांक string& a, स्थिरांक string& dot, स्थिरांक string& b);
    // Convert string to lowercase (tolower)
    स्थिर string downcase(स्थिरांक string& str) VL_PURE;
    // Convert string to upper case (toupper)
    स्थिर string upcase(स्थिरांक string& str) VL_PURE;
    // Insert esc just before tgt
    स्थिर string quoteAny(स्थिरांक string& str, अक्षर tgt, अक्षर esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    स्थिर string quoteBackslash(स्थिरांक string& str) VL_PURE { परत quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    स्थिर string quotePercent(स्थिरांक string& str) VL_PURE { परत quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    स्थिर string dequotePercent(स्थिरांक string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    स्थिर string escapeStringForPath(स्थिरांक string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    स्थिर string unquoteSVString(स्थिरांक string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    स्थिर माप_प्रकार quotedEnd(स्थिरांक string& str, माप_प्रकार pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    स्थिर string quoteStringLiteralForShell(स्थिरांक string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    स्थिर string spaceUnprintable(स्थिरांक string& str) VL_PURE;
    // Remove any whitespace
    स्थिर string removeWhitespace(स्थिरांक string& str);
    // Trim leading/trailing whitespace on each line
    स्थिर string trimWhitespace(स्थिरांक string& str);
    // Return true if only identifer or ""
    स्थिर bool isIdentifier(स्थिरांक string& str);
    // Return true if char is valid character in C identifiers
    स्थिर bool isIdentifierChar(अक्षर c) { परत है_अक्षर_अंक(c) || c == '_'; }
    // Return true if only whitespace or ""
    स्थिर bool isWhitespace(स्थिरांक string& str);
    // Return number of spaces/tabs leading in string
    स्थिर string::size_type leading_Whitespace_मोजणी(स्थिरांक string& str);
    // Return double by parsing string
    स्थिर द्विगुण parseDouble(स्थिरांक string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    स्थिर string replaceSubstr(स्थिरांक string& str, स्थिरांक string& from, स्थिरांक string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    स्थिर string replaceWord(स्थिरांक string& str, स्थिरांक string& from, स्थिरांक string& to);
    // Return deque of strings split by the given delimiter
    स्थिर std::deque<string> split(स्थिरांक string& str, अक्षर delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    स्थिर bool startsWith(स्थिरांक string& str, स्थिरांक अक्षर* prefixp) {
        परत !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    स्थिर bool startsWith(स्थिरांक string& str, स्थिरांक string& prefix) {
        परत !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    स्थिर bool endsWith(स्थिरांक string& str, स्थिरांक string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    स्थिर string aOrAn(स्थिरांक अक्षर* word);
    स्थिर string aOrAn(स्थिरांक string& word) { परत aOrAn(word.c_str()); }
    // Hash the string
    स्थिर uint64_t hashMurmur(स्थिरांक string& str) VL_PURE;

    // Base 64 encode
    स्थिर string base64Enc(स्थिरांक string& str) VL_PURE;

    स्थिर शून्य selfTest();  // Test this class
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
    माप_प्रकार m_totLength = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
सार्वजनिक:
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
    explicit VHashSha512(स्थिरांक string& data)
        : VHashSha512{} {
        घाला(data);
    }
    ~VHashSha512() = पूर्वनिर्धारित;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    स्थिर constexpr माप_प्रकार DIGEST_SYMBOL24_LENGTH = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_द्विमान();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    स्थिर शून्य selfTest();  // Test this class

    // Inserting hash data
    शून्य घाला(स्थिरांक शून्य* datap, माप_प्रकार length);  // Process data into the digest
    शून्य घाला(स्थिरांक string& data) {
        घाला(data.data(), data.length());
    }  // Process data into the digest
    शून्य घाला(uint64_t मूल्य) { घाला(cvtToStr(मूल्य)); }
    शून्य घाला_File(स्थिरांक string& filename);

खाजगी:
    स्थिर शून्य selfTestOne(स्थिरांक string& data, स्थिरांक string& data2, स्थिरांक string& exp,
                            स्थिरांक string& exp64, स्थिरांक string& exp24);
    शून्य finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    स्थिर std::map<string, string> s_dehashMap;  // hashed -> original decoder

    स्थिर माप_प्रकार s_maxLength;  // Length at which to start hashing
    स्थिर माप_प्रकार s_minLength;  // Length to preserve if over maxLength

सार्वजनिक:
    // CONSTRUCTORS
    explicit VName(स्थिरांक string& name)
        : m_name{name} {}
    ~VName() = पूर्वनिर्धारित;
    // METHODS
    शून्य name(स्थिरांक string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() स्थिरांक { परत m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    स्थिर शून्य maxLength(माप_प्रकार flag) { s_maxLength = flag; }
    स्थिर string dehash(स्थिरांक string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    स्थिर constexpr अचिन्हित अंक_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    स्थिर constexpr अचिन्हित LENGTH_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = अचिन्हित पूर्णांक;
    संरचना Candidate final {
        std::string str;  // String to be matched
        std::string संदेश;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
सार्वजनिक:
    // CONSTRUCTORS
    VSpellCheck() = पूर्वनिर्धारित;
    ~VSpellCheck() = पूर्वनिर्धारित;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    शून्य ढकला_Candidate(स्थिरांक string& str) { ढकला_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    शून्य ढकला_Candidate(स्थिरांक string& str, स्थिरांक string& संदेश) {
        जर (m_candidates.size() < अंक_CANDIDATE_LIMIT) m_candidates.ढकला_back({str, संदेश});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(स्थिरांक string& goal) स्थिरांक {
        EditDistance dist;
        परत bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_संदेश(स्थिरांक string& goal) स्थिरांक {
        स्थिरांक स्वतः candidate = bestCandidate(goal);
        जर (candidate.संदेश.empty()) परत "";
        परत "... Suggested alternative: '"s + candidate.संदेश + "'";
    }
    स्थिर शून्य selfTest();
    std::vector<std::string> candidates() स्थिरांक {
        std::vector<std::string> result;
        result.reserve(m_candidates.size());
        साठी (स्थिरांक स्वतः& candidate : m_candidates) { result.ढकला_back(candidate.संदेश); }
        परत result;
    }

खाजगी:
    स्थिर EditDistance editDistance(स्थिरांक string& s, स्थिरांक string& t);
    स्थिर EditDistance cutoffDistance(माप_प्रकार goal_len, माप_प्रकार candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(स्थिरांक string& goal, EditDistance& distancer) स्थिरांक;
    स्थिर शून्य selfTestDistanceOne(स्थिरांक string& a, स्थिरांक string& b, EditDistance expected);
    स्थिर शून्य selfTestSuggestOne(bool matches, स्थिरांक string& c, स्थिरांक string& goal,
                                   EditDistance dist);
};

//######################################################################

#endif  // guard
