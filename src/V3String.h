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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3STRING_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3STRING_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

using VStringList = std::ਸਦਿਸ਼<std::string>;
using VString_ਸੈੱਟ = std::ਸੈੱਟ<std::string>;

//######################################################################
// Global string-related functions

ਸਾਂਚਾ <typename T>
std::string cvt_ਮੰਜ਼ਿਲ_Str(ਅਚਲ T& t) VL_ਸ਼ੁੱਧ {
    std::ostringstream os;
    os << t;
    ਵਾਪਸ os.str();
}
ਸਾਂਚਾ <typename T>
typename std::enable_if<std::is_ਸੂਚਕ<T>::ਮੁੱਲ, std::string>::type
cvt_ਮੰਜ਼ਿਲ_Hex(ਅਚਲ T tp) VL_ਸ਼ੁੱਧ {
    std::ostringstream os;
    os << static_cast<ਅਚਲ ਖਾਲੀ*>(tp);
    ਵਾਪਸ os.str();
}
ਸਾਂਚਾ <typename T>
typename std::enable_if<std::is_integral<T>::ਮੁੱਲ, std::string>::type cvt_ਮੰਜ਼ਿਲ_Hex(ਅਚਲ T t) {
    std::ostringstream os;
    os << std::hex << std::setw(ਆਕਾਰ(T) * 8 / 4) << std::setfill('0') << t;
    ਵਾਪਸ os.str();
}

ਸਤਰਅੰਦਰ string ucfirst(ਅਚਲ string& ਪਾਠ) {
    string ਨਤੀਜਾ = ਪਾਠ;
    ਨਤੀਜਾ[0] = std::ਵੱਡਾ_ਅੱਖਰ(ਨਤੀਜਾ[0]);
    ਵਾਪਸ ਨਤੀਜਾ;
}

//######################################################################
// VString - String manipulation

class VString final {
    ਸਥਿਰ bool wildmatchi(ਅਚਲ ਅੱਖਰ* s, ਅਚਲ ਅੱਖਰ* p) VL_ਸ਼ੁੱਧ;

ਜਨਤਕ:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    ਸਥਿਰ bool wildmatch(ਅਚਲ ਅੱਖਰ* s, ਅਚਲ ਅੱਖਰ* p) VL_ਸ਼ੁੱਧ;
    // Return true if p with ? or *'s matches s
    ਸਥਿਰ bool wildmatch(ਅਚਲ string& s, ਅਚਲ string& p) VL_ਸ਼ੁੱਧ;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    ਸਥਿਰ string dot(ਅਚਲ string& a, ਅਚਲ string& dot, ਅਚਲ string& b);
    // Convert string to lowercase (tolower)
    ਸਥਿਰ string downcase(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ;
    // Convert string to upper case (toupper)
    ਸਥਿਰ string upcase(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ;
    // Insert esc just before tgt
    ਸਥਿਰ string quoteAny(ਅਚਲ string& str, ਅੱਖਰ tgt, ਅੱਖਰ esc) VL_ਸ਼ੁੱਧ;
    // Replace any \'s with \\  (two consecutive backslashes)
    ਸਥਿਰ string quoteBackslash(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ { ਵਾਪਸ quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    ਸਥਿਰ string quotePercent(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ { ਵਾਪਸ quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    ਸਥਿਰ string dequotePercent(ਅਚਲ string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    ਸਥਿਰ string escape_String_For_ਰਸਤਾ(ਅਚਲ string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    ਸਥਿਰ string unquoteSVString(ਅਚਲ string& ਪਾਠ, string& err_ਬਾਹਰਲਾ);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    ਸਥਿਰ ਮਾਪ_t quoted_ਸਿਰਾ(ਅਚਲ string& str, ਮਾਪ_t pos) VL_ਸ਼ੁੱਧ;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    ਸਥਿਰ string quoteStringLiteralForShell(ਅਚਲ string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    ਸਥਿਰ string spaceUnprintable(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ;
    // Remove any whitespace
    ਸਥਿਰ string removeWhitespace(ਅਚਲ string& str);
    // Trim leading/trailing whitespace on each line
    ਸਥਿਰ string trimWhitespace(ਅਚਲ string& str);
    // Return true if only identifer or ""
    ਸਥਿਰ bool isIdentifier(ਅਚਲ string& str);
    // Return true if char is valid character in C identifiers
    ਸਥਿਰ bool isIdentifierChar(ਅੱਖਰ c) { ਵਾਪਸ ਅੱਖਰ_ਜਾਂ_ਅੰਕ(c) || c == '_'; }
    // Return true if only whitespace or ""
    ਸਥਿਰ bool isWhitespace(ਅਚਲ string& str);
    // Return number of spaces/tabs leading in string
    ਸਥਿਰ string::ਮਾਪ_type leading_Whitespace_ਗਿਣਤੀ(ਅਚਲ string& str);
    // Return double by parsing string
    ਸਥਿਰ ਦੁੱਗਣਾ parseDouble(ਅਚਲ string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    ਸਥਿਰ string replaceSubstr(ਅਚਲ string& str, ਅਚਲ string& from, ਅਚਲ string& ਮੰਜ਼ਿਲ);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    ਸਥਿਰ string replace_ਸ਼ਬਦ(ਅਚਲ string& str, ਅਚਲ string& from, ਅਚਲ string& ਮੰਜ਼ਿਲ);
    // Return deque of strings split by the given delimiter
    ਸਥਿਰ std::deque<string> ਵੰਡੋ(ਅਚਲ string& str, ਅੱਖਰ delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    ਸਥਿਰ bool startsWith(ਅਚਲ string& str, ਅਚਲ ਅੱਖਰ* prefixp) {
        ਵਾਪਸ !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    ਸਥਿਰ bool startsWith(ਅਚਲ string& str, ਅਚਲ string& ਅਗੇਤਰ) {
        ਵਾਪਸ !str.rfind(ਅਗੇਤਰ, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    ਸਥਿਰ bool ਸਿਰੇ_With(ਅਚਲ string& str, ਅਚਲ string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    ਸਥਿਰ string aOrAn(ਅਚਲ ਅੱਖਰ* ਸ਼ਬਦ);
    ਸਥਿਰ string aOrAn(ਅਚਲ string& ਸ਼ਬਦ) { ਵਾਪਸ aOrAn(ਸ਼ਬਦ.c_str()); }
    // Hash the string
    ਸਥਿਰ uint64_t hashMurmur(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ;

    // Base 64 encode
    ਸਥਿਰ string base64Enc(ਅਚਲ string& str) VL_ਸ਼ੁੱਧ;

    ਸਥਿਰ ਖਾਲੀ selfTest();  // Test this class
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
    ਮਾਪ_t m_tot_ਲੰਮਾਈ = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
ਜਨਤਕ:
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
    explicit VHashSha512(ਅਚਲ string& ਅੰਕੜੇ)
        : VHashSha512{} {
        ਪਾਓ(ਅੰਕੜੇ);
    }
    ~VHashSha512() = ਮੂਲ;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    ਸਥਿਰ constexpr ਮਾਪ_t DIGEST_SYMBOL24_ਲੰਮਾਈ = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ਦੁਆਧਾਰੀ();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    ਸਥਿਰ ਖਾਲੀ selfTest();  // Test this class

    // Inserting hash data
    ਖਾਲੀ ਪਾਓ(ਅਚਲ ਖਾਲੀ* datap, ਮਾਪ_t ਲੰਮਾਈ);  // Process data into the digest
    ਖਾਲੀ ਪਾਓ(ਅਚਲ string& ਅੰਕੜੇ) {
        ਪਾਓ(ਅੰਕੜੇ.ਅੰਕੜੇ(), ਅੰਕੜੇ.ਲੰਮਾਈ());
    }  // Process data into the digest
    ਖਾਲੀ ਪਾਓ(uint64_t ਮੁੱਲ) { ਪਾਓ(cvt_ਮੰਜ਼ਿਲ_Str(ਮੁੱਲ)); }
    ਖਾਲੀ ਪਾਓ_File(ਅਚਲ string& filename);

ਨਿਜੀ:
    ਸਥਿਰ ਖਾਲੀ selfTestOne(ਅਚਲ string& ਅੰਕੜੇ, ਅਚਲ string& data2, ਅਚਲ string& ਘਾਤਾਂਕ,
                            ਅਚਲ string& exp64, ਅਚਲ string& exp24);
    ਖਾਲੀ finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    ਸਥਿਰ std::map<string, string> s_dehashMap;  // hashed -> original decoder

    ਸਥਿਰ ਮਾਪ_t s_max_ਲੰਮਾਈ;  // Length at which to start hashing
    ਸਥਿਰ ਮਾਪ_t s_min_ਲੰਮਾਈ;  // Length to preserve if over maxLength

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit VName(ਅਚਲ string& name)
        : m_name{name} {}
    ~VName() = ਮੂਲ;
    // METHODS
    ਖਾਲੀ name(ਅਚਲ string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ਅਚਲ { ਵਾਪਸ m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    ਸਥਿਰ ਖਾਲੀ max_ਲੰਮਾਈ(ਮਾਪ_t flag) { s_max_ਲੰਮਾਈ = flag; }
    ਸਥਿਰ string dehash(ਅਚਲ string& ਵਿੱਚ_2);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੰਕ_CANDIDATE_ਹੱਦਬੰਦੀ = 10000;  // Avoid searching huge netlists
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾਈ_ਹੱਦਬੰਦੀ = 100;  // Maximum string length to search
    // TYPES
    using Edit_ਦੂਰੀ = ਬਿਨਾਨਿਸ਼ਾਨੀ ਪੂਰਨ;
    ਬਣਤਰ Candidate final {
        std::string str;  // String to be matched
        std::string ਸੁਨੇਹਾ_2;  // Message to print on a match
    };
    // MEMBERS
    std::ਸਦਿਸ਼<Candidate> m_candidates;  // Strings we try to match
ਜਨਤਕ:
    // CONSTRUCTORS
    VSpellCheck() = ਮੂਲ;
    ~VSpellCheck() = ਮੂਲ;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    ਖਾਲੀ ਧੱਕੋ_Candidate(ਅਚਲ string& str) { ਧੱਕੋ_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    ਖਾਲੀ ਧੱਕੋ_Candidate(ਅਚਲ string& str, ਅਚਲ string& ਸੁਨੇਹਾ_2) {
        ਜੇ (m_candidates.ਮਾਪ() < ਅੰਕ_CANDIDATE_ਹੱਦਬੰਦੀ) m_candidates.ਧੱਕੋ_ਪਿੱਠ({str, ਸੁਨੇਹਾ_2});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate ਉੱਤਮ_Candidate(ਅਚਲ string& goal) ਅਚਲ {
        Edit_ਦੂਰੀ dist;
        ਵਾਪਸ ਉੱਤਮ_Candidate_Info(goal, dist /*ref*/);
    }
    // Return friendly message
    string ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(ਅਚਲ string& goal) ਅਚਲ {
        ਅਚਲ ਸਵੈਚਾਲਿਤ candidate = ਉੱਤਮ_Candidate(goal);
        ਜੇ (candidate.ਸੁਨੇਹਾ_2.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ "";
        ਵਾਪਸ "... Suggested alternative: '"s + candidate.ਸੁਨੇਹਾ_2 + "'";
    }
    ਸਥਿਰ ਖਾਲੀ selfTest();
    std::ਸਦਿਸ਼<std::string> candidates() ਅਚਲ {
        std::ਸਦਿਸ਼<std::string> ਨਤੀਜਾ;
        ਨਤੀਜਾ.reserve(m_candidates.ਮਾਪ());
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& candidate : m_candidates) { ਨਤੀਜਾ.ਧੱਕੋ_ਪਿੱਠ(candidate.ਸੁਨੇਹਾ_2); }
        ਵਾਪਸ ਨਤੀਜਾ;
    }

ਨਿਜੀ:
    ਸਥਿਰ Edit_ਦੂਰੀ edit_ਦੂਰੀ(ਅਚਲ string& s, ਅਚਲ string& t);
    ਸਥਿਰ Edit_ਦੂਰੀ cutoff_ਦੂਰੀ(ਮਾਪ_t goal_len, ਮਾਪ_t candidate_len);
    VSpellCheck::Candidate ਉੱਤਮ_Candidate_Info(ਅਚਲ string& goal, Edit_ਦੂਰੀ& distancer) ਅਚਲ;
    ਸਥਿਰ ਖਾਲੀ self_Test_ਦੂਰੀ_One(ਅਚਲ string& a, ਅਚਲ string& b, Edit_ਦੂਰੀ expected);
    ਸਥਿਰ ਖਾਲੀ selfTestSuggestOne(bool matches, ਅਚਲ string& c, ਅਚਲ string& goal,
                                   Edit_ਦੂਰੀ dist);
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
