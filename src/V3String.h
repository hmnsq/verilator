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

#अगर_अघोषित VERILATOR_V3STRING_H_
#घोषणा VERILATOR_V3STRING_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#समावेश <deque>
#समावेश <iomanip>
#समावेश <map>
#समावेश <set>
#समावेश <sstream>
#समावेश <string>
#समावेश <type_traits>
#समावेश <unordered_map>
#समावेश <vector>

using VStringList = std::vector<std::string>;
using VStringSet = std::set<std::string>;

//######################################################################
// Global string-related functions

ढाँचा <typename T>
std::string cvtToStr(स्थिर T& t) VL_PURE {
    std::ostringstream os;
    os << t;
    वापस os.str();
}
ढाँचा <typename T>
typename std::enable_if<std::is_pointer<T>::मूल्य, std::string>::type
cvtToHex(स्थिर T tp) VL_PURE {
    std::ostringstream os;
    os << static_cast<स्थिर व्योम*>(tp);
    वापस os.str();
}
ढाँचा <typename T>
typename std::enable_if<std::is_integral<T>::मूल्य, std::string>::type cvtToHex(स्थिर T t) {
    std::ostringstream os;
    os << std::hex << std::setw(माप(T) * 8 / 4) << std::setfill('0') << t;
    वापस os.str();
}

अंतरभूत string ucfirst(स्थिर string& text) {
    string परिणाम = text;
    परिणाम[0] = std::बड़े(परिणाम[0]);
    वापस परिणाम;
}

//######################################################################
// VString - String manipulation

class VString final {
    अटल bool wildmatchi(स्थिर अक्षर* s, स्थिर अक्षर* p) VL_PURE;

खुला:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    अटल bool wildmatch(स्थिर अक्षर* s, स्थिर अक्षर* p) VL_PURE;
    // Return true if p with ? or *'s matches s
    अटल bool wildmatch(स्थिर string& s, स्थिर string& p) VL_PURE;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    अटल string dot(स्थिर string& a, स्थिर string& dot, स्थिर string& b);
    // Convert string to lowercase (tolower)
    अटल string downcase(स्थिर string& str) VL_PURE;
    // Convert string to upper case (toupper)
    अटल string upcase(स्थिर string& str) VL_PURE;
    // Insert esc just before tgt
    अटल string quoteAny(स्थिर string& str, अक्षर tgt, अक्षर esc) VL_PURE;
    // Replace any \'s with \\  (two consecutive backslashes)
    अटल string quoteBackslash(स्थिर string& str) VL_PURE { वापस quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    अटल string quotePercent(स्थिर string& str) VL_PURE { वापस quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    अटल string dequotePercent(स्थिर string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    अटल string escapeStringForPath(स्थिर string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    अटल string unquoteSVString(स्थिर string& text, string& errOut);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    अटल माप_प्रकार quotedEnd(स्थिर string& str, माप_प्रकार pos) VL_PURE;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    अटल string quoteStringLiteralForShell(स्थिर string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    अटल string spaceUnprintable(स्थिर string& str) VL_PURE;
    // Remove any whitespace
    अटल string removeWhitespace(स्थिर string& str);
    // Trim leading/trailing whitespace on each line
    अटल string trimWhitespace(स्थिर string& str);
    // Return true if only identifer or ""
    अटल bool isIdentifier(स्थिर string& str);
    // Return true if char is valid character in C identifiers
    अटल bool isIdentifierChar(अक्षर c) { वापस है_अक्षर_अंक(c) || c == '_'; }
    // Return true if only whitespace or ""
    अटल bool isWhitespace(स्थिर string& str);
    // Return number of spaces/tabs leading in string
    अटल string::आकार_type leading_Whitespace_गिनती(स्थिर string& str);
    // Return double by parsing string
    अटल द्विगुन parseDouble(स्थिर string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    अटल string replaceSubstr(स्थिर string& str, स्थिर string& from, स्थिर string& to);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    अटल string replaceWord(स्थिर string& str, स्थिर string& from, स्थिर string& to);
    // Return deque of strings split by the given delimiter
    अटल std::deque<string> split(स्थिर string& str, अक्षर delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    अटल bool startsWith(स्थिर string& str, स्थिर अक्षर* prefixp) {
        वापस !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    अटल bool startsWith(स्थिर string& str, स्थिर string& prefix) {
        वापस !str.rfind(prefix, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    अटल bool endsWith(स्थिर string& str, स्थिर string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    अटल string aOrAn(स्थिर अक्षर* word);
    अटल string aOrAn(स्थिर string& word) { वापस aOrAn(word.c_str()); }
    // Hash the string
    अटल uint64_t hashMurmur(स्थिर string& str) VL_PURE;

    // Base 64 encode
    अटल string base64Enc(स्थिर string& str) VL_PURE;

    अटल व्योम selfTest();  // Test this class
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
    माप_प्रकार m_tot_लंबाई = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
खुला:
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
    explicit VHashSha512(स्थिर string& तथ्य)
        : VHashSha512{} {
        जोड़ो(तथ्य);
    }
    ~VHashSha512() = शेष;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    अटल constexpr माप_प्रकार DIGEST_SYMBOL24_लंबाई = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_द्विआधारी();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    अटल व्योम selfTest();  // Test this class

    // Inserting hash data
    व्योम जोड़ो(स्थिर व्योम* datap, माप_प्रकार लंबाई);  // Process data into the digest
    व्योम जोड़ो(स्थिर string& तथ्य) {
        जोड़ो(तथ्य.तथ्य(), तथ्य.लंबाई());
    }  // Process data into the digest
    व्योम जोड़ो(uint64_t मूल्य) { जोड़ो(cvtToStr(मूल्य)); }
    व्योम जोड़ो_File(स्थिर string& filename);

निजी:
    अटल व्योम selfTestOne(स्थिर string& तथ्य, स्थिर string& data2, स्थिर string& exp,
                            स्थिर string& exp64, स्थिर string& exp24);
    व्योम finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    अटल std::map<string, string> s_dehashMap;  // hashed -> original decoder

    अटल माप_प्रकार s_max_लंबाई;  // Length at which to start hashing
    अटल माप_प्रकार s_min_लंबाई;  // Length to preserve if over maxLength

खुला:
    // CONSTRUCTORS
    explicit VName(स्थिर string& name)
        : m_name{name} {}
    ~VName() = शेष;
    // METHODS
    व्योम name(स्थिर string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() स्थिर { वापस m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    अटल व्योम max_लंबाई(माप_प्रकार flag) { s_max_लंबाई = flag; }
    अटल string dehash(स्थिर string& in);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    अटल constexpr अचिन्हित अंक_CANDIDATE_LIMIT = 10000;  // Avoid searching huge netlists
    अटल constexpr अचिन्हित लंबाई_LIMIT = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = अचिन्हित पूर्णांक;
    काष्ठा Candidate final {
        std::string str;  // String to be matched
        std::string संदेश;  // Message to print on a match
    };
    // MEMBERS
    std::vector<Candidate> m_candidates;  // Strings we try to match
खुला:
    // CONSTRUCTORS
    VSpellCheck() = शेष;
    ~VSpellCheck() = शेष;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    व्योम धकेलो_Candidate(स्थिर string& str) { धकेलो_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    व्योम धकेलो_Candidate(स्थिर string& str, स्थिर string& संदेश) {
        अगर (m_candidates.आकार() < अंक_CANDIDATE_LIMIT) m_candidates.धकेलो_back({str, संदेश});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(स्थिर string& goal) स्थिर {
        EditDistance dist;
        वापस bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_संदेश(स्थिर string& goal) स्थिर {
        स्थिर स्वतः candidate = bestCandidate(goal);
        अगर (candidate.संदेश.empty()) वापस "";
        वापस "... Suggested alternative: '"s + candidate.संदेश + "'";
    }
    अटल व्योम selfTest();
    std::vector<std::string> candidates() स्थिर {
        std::vector<std::string> परिणाम;
        परिणाम.reserve(m_candidates.आकार());
        क्रम (स्थिर स्वतः& candidate : m_candidates) { परिणाम.धकेलो_back(candidate.संदेश); }
        वापस परिणाम;
    }

निजी:
    अटल EditDistance editDistance(स्थिर string& s, स्थिर string& t);
    अटल EditDistance cutoffDistance(माप_प्रकार goal_len, माप_प्रकार candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(स्थिर string& goal, EditDistance& distancer) स्थिर;
    अटल व्योम selfTestDistanceOne(स्थिर string& a, स्थिर string& b, EditDistance expected);
    अटल व्योम selfTestSuggestOne(bool matches, स्थिर string& c, स्थिर string& goal,
                                   EditDistance dist);
};

//######################################################################

#पूर्ण_अगर  // guard
