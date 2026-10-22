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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3STRING_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3STRING_H_

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

using VStringList = std::ಸದಿಶ<std::string>;
using VString_ಗಣ = std::ಗಣ<std::string>;

//######################################################################
// Global string-related functions

ಮಾದರಿ <typename T>
std::string cvt_ಗಮ್ಯ_Str(ಸ್ಥಿರಾಂಕ T& t) VL_ಶುದ್ಧ {
    std::ostringstream os;
    os << t;
    ಹಿಂತಿರುಗಿ os.str();
}
ಮಾದರಿ <typename T>
typename std::enable_if<std::is_ಸೂಚಕ<T>::ಮೌಲ್ಯ, std::string>::type
cvt_ಗಮ್ಯ_Hex(ಸ್ಥಿರಾಂಕ T tp) VL_ಶುದ್ಧ {
    std::ostringstream os;
    os << static_cast<ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*>(tp);
    ಹಿಂತಿರುಗಿ os.str();
}
ಮಾದರಿ <typename T>
typename std::enable_if<std::is_integral<T>::ಮೌಲ್ಯ, std::string>::type cvt_ಗಮ್ಯ_Hex(ಸ್ಥಿರಾಂಕ T t) {
    std::ostringstream os;
    os << std::hex << std::setw(ಗಾತ್ರ(T) * 8 / 4) << std::setfill('0') << t;
    ಹಿಂತಿರುಗಿ os.str();
}

ಸಾಲೊಳಗೆ string ucfirst(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    string ಫಲಿತಾಂಶ = ಪಠ್ಯ;
    ಫಲಿತಾಂಶ[0] = std::ದೊಡ್ಡ_ಅಕ್ಷರ(ಫಲಿತಾಂಶ[0]);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//######################################################################
// VString - String manipulation

class VString final {
    ಸ್ಥಿರ bool wildmatchi(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* s, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p) VL_ಶುದ್ಧ;

ಸಾರ್ವಜನಿಕ:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ಸ್ಥಿರ bool wildmatch(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* s, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p) VL_ಶುದ್ಧ;
    // Return true if p with ? or *'s matches s
    ಸ್ಥಿರ bool wildmatch(ಸ್ಥಿರಾಂಕ string& s, ಸ್ಥಿರಾಂಕ string& p) VL_ಶುದ್ಧ;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ಸ್ಥಿರ string dot(ಸ್ಥಿರಾಂಕ string& a, ಸ್ಥಿರಾಂಕ string& dot, ಸ್ಥಿರಾಂಕ string& b);
    // Convert string to lowercase (tolower)
    ಸ್ಥಿರ string downcase(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ;
    // Convert string to upper case (toupper)
    ಸ್ಥಿರ string upcase(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ;
    // Insert esc just before tgt
    ಸ್ಥಿರ string quoteAny(ಸ್ಥಿರಾಂಕ string& str, ಅಕ್ಷರ tgt, ಅಕ್ಷರ esc) VL_ಶುದ್ಧ;
    // Replace any \'s with \\  (two consecutive backslashes)
    ಸ್ಥಿರ string quoteBackslash(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ಸ್ಥಿರ string quotePercent(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ಸ್ಥಿರ string dequotePercent(ಸ್ಥಿರಾಂಕ string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ಸ್ಥಿರ string escape_String_For_ಮಾರ್ಗ(ಸ್ಥಿರಾಂಕ string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ಸ್ಥಿರ string unquoteSVString(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, string& err_ಹೊರಹೋಗು);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ಸ್ಥಿರ ಗಾತ್ರ_t quoted_ಕೊನೆ(ಸ್ಥಿರಾಂಕ string& str, ಗಾತ್ರ_t pos) VL_ಶುದ್ಧ;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ಸ್ಥಿರ string quoteStringLiteralForShell(ಸ್ಥಿರಾಂಕ string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ಸ್ಥಿರ string spaceUnprintable(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ;
    // Remove any whitespace
    ಸ್ಥಿರ string removeWhitespace(ಸ್ಥಿರಾಂಕ string& str);
    // Trim leading/trailing whitespace on each line
    ಸ್ಥಿರ string trimWhitespace(ಸ್ಥಿರಾಂಕ string& str);
    // Return true if only identifer or ""
    ಸ್ಥಿರ bool isIdentifier(ಸ್ಥಿರಾಂಕ string& str);
    // Return true if char is valid character in C identifiers
    ಸ್ಥಿರ bool isIdentifierChar(ಅಕ್ಷರ c) { ಹಿಂತಿರುಗಿ ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(c) || c == '_'; }
    // Return true if only whitespace or ""
    ಸ್ಥಿರ bool isWhitespace(ಸ್ಥಿರಾಂಕ string& str);
    // Return number of spaces/tabs leading in string
    ಸ್ಥಿರ string::ಗಾತ್ರ_type leading_Whitespace_ಎಣಿಕೆ(ಸ್ಥಿರಾಂಕ string& str);
    // Return double by parsing string
    ಸ್ಥಿರ ದ್ವಿಗುಣ parseDouble(ಸ್ಥಿರಾಂಕ string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ಸ್ಥಿರ string replaceSubstr(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& from, ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ಸ್ಥಿರ string replace_ಶಬ್ದ(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& from, ಸ್ಥಿರಾಂಕ string& ಗಮ್ಯ);
    // Return deque of strings split by the given delimiter
    ಸ್ಥಿರ std::deque<string> ವಿಭಜಿಸು(ಸ್ಥಿರಾಂಕ string& str, ಅಕ್ಷರ delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ಸ್ಥಿರ bool startsWith(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* prefixp) {
        ಹಿಂತಿರುಗಿ !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ಸ್ಥಿರ bool startsWith(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& ಪೂರ್ವಪ್ರತ್ಯಯ) {
        ಹಿಂತಿರುಗಿ !str.rfind(ಪೂರ್ವಪ್ರತ್ಯಯ, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ಸ್ಥಿರ bool ತುದಿಗಳು_With(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ಸ್ಥಿರ string aOrAn(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಶಬ್ದ);
    ಸ್ಥಿರ string aOrAn(ಸ್ಥಿರಾಂಕ string& ಶಬ್ದ) { ಹಿಂತಿರುಗಿ aOrAn(ಶಬ್ದ.c_str()); }
    // Hash the string
    ಸ್ಥಿರ uint64_t hashMurmur(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ;

    // Base 64 encode
    ಸ್ಥಿರ string base64Enc(ಸ್ಥಿರಾಂಕ string& str) VL_ಶುದ್ಧ;

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
    ಗಾತ್ರ_t m_tot_ಉದ್ದಳತೆ = 0;  // Total all-chunk length as needed by output digest
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
    explicit VHashSha512(ಸ್ಥಿರಾಂಕ string& ದತ್ತಾಂಶ)
        : VHashSha512{} {
        ಸೇರಿಸು(ದತ್ತಾಂಶ);
    }
    ~VHashSha512() = ಪೂರ್ವನಿಯೋಜಿತ;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ಸ್ಥಿರ constexpr ಗಾತ್ರ_t DIGEST_SYMBOL24_ಉದ್ದಳತೆ = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ದ್ವಿಮಾನ();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();  // Test this class

    // Inserting hash data
    ಶೂನ್ಯ ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* datap, ಗಾತ್ರ_t ಉದ್ದಳತೆ);  // Process data into the digest
    ಶೂನ್ಯ ಸೇರಿಸು(ಸ್ಥಿರಾಂಕ string& ದತ್ತಾಂಶ) {
        ಸೇರಿಸು(ದತ್ತಾಂಶ.ದತ್ತಾಂಶ(), ದತ್ತಾಂಶ.ಉದ್ದಳತೆ());
    }  // Process data into the digest
    ಶೂನ್ಯ ಸೇರಿಸು(uint64_t ಮೌಲ್ಯ) { ಸೇರಿಸು(cvt_ಗಮ್ಯ_Str(ಮೌಲ್ಯ)); }
    ಶೂನ್ಯ ಸೇರಿಸು_File(ಸ್ಥಿರಾಂಕ string& filename);

ಖಾಸಗಿ:
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestOne(ಸ್ಥಿರಾಂಕ string& ದತ್ತಾಂಶ, ಸ್ಥಿರಾಂಕ string& data2, ಸ್ಥಿರಾಂಕ string& ಘಾತಾಂಕ,
                            ಸ್ಥಿರಾಂಕ string& exp64, ಸ್ಥಿರಾಂಕ string& exp24);
    ಶೂನ್ಯ finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ಸ್ಥಿರ std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ಸ್ಥಿರ ಗಾತ್ರ_t s_max_ಉದ್ದಳತೆ;  // Length at which to start hashing
    ಸ್ಥಿರ ಗಾತ್ರ_t s_min_ಉದ್ದಳತೆ;  // Length to preserve if over maxLength

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit VName(ಸ್ಥಿರಾಂಕ string& name)
        : m_name{name} {}
    ~VName() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    ಶೂನ್ಯ name(ಸ್ಥಿರಾಂಕ string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ಸ್ಥಿರ ಶೂನ್ಯ max_ಉದ್ದಳತೆ(ಗಾತ್ರ_t flag) { s_max_ಉದ್ದಳತೆ = flag; }
    ಸ್ಥಿರ string dehash(ಸ್ಥಿರಾಂಕ string& ಒಳ);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ ಅಂಕೆ_CANDIDATE_ಎಲ್ಲೆ = 10000;  // Avoid searching huge netlists
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ ಉದ್ದಳತೆ_ಎಲ್ಲೆ = 100;  // Maximum string length to search
    // TYPES
    using Edit_ದೂರ = ಚಿಹ್ನೆರಹಿತ ಪೂರ್ಣಾಂಕ;
    ರಚನೆ Candidate final {
        std::string str;  // String to be matched
        std::string ಸಂದೇಶ_2;  // Message to print on a match
    };
    // MEMBERS
    std::ಸದಿಶ<Candidate> m_candidates;  // Strings we try to match
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VSpellCheck() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VSpellCheck() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    ಶೂನ್ಯ ತಳ್ಳು_Candidate(ಸ್ಥಿರಾಂಕ string& str) { ತಳ್ಳು_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    ಶೂನ್ಯ ತಳ್ಳು_Candidate(ಸ್ಥಿರಾಂಕ string& str, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ_2) {
        ಆದರೆ (m_candidates.ಗಾತ್ರ_2() < ಅಂಕೆ_CANDIDATE_ಎಲ್ಲೆ) m_candidates.ತಳ್ಳು_ಹಿಂಭಾಗ({str, ಸಂದೇಶ_2});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate ಉತ್ತಮ_Candidate(ಸ್ಥಿರಾಂಕ string& goal) ಸ್ಥಿರಾಂಕ {
        Edit_ದೂರ dist;
        ಹಿಂತಿರುಗಿ ಉತ್ತಮ_Candidate_Info(goal, dist /*ref*/);
    }
    // Return friendly message
    string ಉತ್ತಮ_Candidate_ಸಂದೇಶ(ಸ್ಥಿರಾಂಕ string& goal) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ candidate = ಉತ್ತಮ_Candidate(goal);
        ಆದರೆ (candidate.ಸಂದೇಶ_2.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ "";
        ಹಿಂತಿರುಗಿ "... Suggested alternative: '"s + candidate.ಸಂದೇಶ_2 + "'";
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
    std::ಸದಿಶ<std::string> candidates() ಸ್ಥಿರಾಂಕ {
        std::ಸದಿಶ<std::string> ಫಲಿತಾಂಶ;
        ಫಲಿತಾಂಶ.reserve(m_candidates.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& candidate : m_candidates) { ಫಲಿತಾಂಶ.ತಳ್ಳು_ಹಿಂಭಾಗ(candidate.ಸಂದೇಶ_2); }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

ಖಾಸಗಿ:
    ಸ್ಥಿರ Edit_ದೂರ edit_ದೂರ(ಸ್ಥಿರಾಂಕ string& s, ಸ್ಥಿರಾಂಕ string& t);
    ಸ್ಥಿರ Edit_ದೂರ cutoff_ದೂರ(ಗಾತ್ರ_t goal_len, ಗಾತ್ರ_t candidate_len);
    VSpellCheck::Candidate ಉತ್ತಮ_Candidate_Info(ಸ್ಥಿರಾಂಕ string& goal, Edit_ದೂರ& distancer) ಸ್ಥಿರಾಂಕ;
    ಸ್ಥಿರ ಶೂನ್ಯ self_Test_ದೂರ_One(ಸ್ಥಿರಾಂಕ string& a, ಸ್ಥಿರಾಂಕ string& b, Edit_ದೂರ expected);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTestSuggestOne(bool matches, ಸ್ಥಿರಾಂಕ string& c, ಸ್ಥಿರಾಂಕ string& goal,
                                   Edit_ದೂರ dist);
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
