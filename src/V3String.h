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

#यदि_अपरिभाषितम् VERILATOR_V3STRING_H_
#परिभाषय VERILATOR_V3STRING_H_

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

using VStringList = std::सदिशम्<std::string>;
using VString_समुच्चयः = std::समुच्चयः<std::string>;

//######################################################################
// Global string-related functions

प्रतिरूपम् <typename T>
std::string cvt_गन्तव्यम्_Str(ध्रुव T& t) VL_विशुद्धम् {
    std::ostringstream os;
    os << t;
    प्रत्यागम os.str();
}
प्रतिरूपम् <typename T>
typename std::enable_if<std::is_pointer<T>::मानम्, std::string>::type
cvt_गन्तव्यम्_Hex(ध्रुव T tp) VL_विशुद्धम् {
    std::ostringstream os;
    os << static_cast<ध्रुव शून्य*>(tp);
    प्रत्यागम os.str();
}
प्रतिरूपम् <typename T>
typename std::enable_if<std::is_integral<T>::मानम्, std::string>::type cvt_गन्तव्यम्_Hex(ध्रुव T t) {
    std::ostringstream os;
    os << std::hex << std::setw(परिमाण(T) * 8 / 4) << std::setfill('0') << t;
    प्रत्यागम os.str();
}

अन्तःस्थ string ucfirst(ध्रुव string& text) {
    string परिणाम = text;
    परिणाम[0] = std::महाक्षरम्(परिणाम[0]);
    प्रत्यागम परिणाम;
}

//######################################################################
// VString - String manipulation

class VString final {
    स्थिर bool wildmatchi(ध्रुव वर्ण* s, ध्रुव वर्ण* p) VL_विशुद्धम्;

सार्वजनिक:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    स्थिर bool wildmatch(ध्रुव वर्ण* s, ध्रुव वर्ण* p) VL_विशुद्धम्;
    // Return true if p with ? or *'s matches s
    स्थिर bool wildmatch(ध्रुव string& s, ध्रुव string& p) VL_विशुद्धम्;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    स्थिर string dot(ध्रुव string& a, ध्रुव string& dot, ध्रुव string& b);
    // Convert string to lowercase (tolower)
    स्थिर string downcase(ध्रुव string& str) VL_विशुद्धम्;
    // Convert string to upper case (toupper)
    स्थिर string upcase(ध्रुव string& str) VL_विशुद्धम्;
    // Insert esc just before tgt
    स्थिर string quoteAny(ध्रुव string& str, वर्ण tgt, वर्ण esc) VL_विशुद्धम्;
    // Replace any \'s with \\  (two consecutive backslashes)
    स्थिर string quoteBackslash(ध्रुव string& str) VL_विशुद्धम् { प्रत्यागम quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    स्थिर string quotePercent(ध्रुव string& str) VL_विशुद्धम् { प्रत्यागम quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    स्थिर string dequotePercent(ध्रुव string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    स्थिर string escape_String_For_मार्गः(ध्रुव string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    स्थिर string unquoteSVString(ध्रुव string& text, string& err_बहिर्गमनम्);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    स्थिर आकार_t quoted_अन्तः(ध्रुव string& str, आकार_t pos) VL_विशुद्धम्;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    स्थिर string quoteStringLiteralForShell(ध्रुव string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    स्थिर string spaceUnprintable(ध्रुव string& str) VL_विशुद्धम्;
    // Remove any whitespace
    स्थिर string removeWhitespace(ध्रुव string& str);
    // Trim leading/trailing whitespace on each line
    स्थिर string trimWhitespace(ध्रुव string& str);
    // Return true if only identifer or ""
    स्थिर bool isIdentifier(ध्रुव string& str);
    // Return true if char is valid character in C identifiers
    स्थिर bool isIdentifierChar(वर्ण c) { प्रत्यागम अक्षरम्_अङ्कः_वा(c) || c == '_'; }
    // Return true if only whitespace or ""
    स्थिर bool isWhitespace(ध्रुव string& str);
    // Return number of spaces/tabs leading in string
    स्थिर string::आकार_type leading_Whitespace_गणनम्(ध्रुव string& str);
    // Return double by parsing string
    स्थिर द्विगुण parseDouble(ध्रुव string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    स्थिर string replaceSubstr(ध्रुव string& str, ध्रुव string& from, ध्रुव string& गन्तव्यम्);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    स्थिर string replaceWord(ध्रुव string& str, ध्रुव string& from, ध्रुव string& गन्तव्यम्);
    // Return deque of strings split by the given delimiter
    स्थिर std::deque<string> विभज(ध्रुव string& str, वर्ण delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    स्थिर bool startsWith(ध्रुव string& str, ध्रुव वर्ण* prefixp) {
        प्रत्यागम !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    स्थिर bool startsWith(ध्रुव string& str, ध्रुव string& उपसर्गः) {
        प्रत्यागम !str.rfind(उपसर्गः, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    स्थिर bool अन्ताः_With(ध्रुव string& str, ध्रुव string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    स्थिर string aOrAn(ध्रुव वर्ण* word);
    स्थिर string aOrAn(ध्रुव string& word) { प्रत्यागम aOrAn(word.c_str()); }
    // Hash the string
    स्थिर uint64_t hashMurmur(ध्रुव string& str) VL_विशुद्धम्;

    // Base 64 encode
    स्थिर string base64Enc(ध्रुव string& str) VL_विशुद्धम्;

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
    आकार_t m_tot_दीर्घता = 0;  // Total all-chunk length as needed by output digest
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
    explicit VHashSha512(ध्रुव string& दत्त)
        : VHashSha512{} {
        निवेशय(दत्त);
    }
    ~VHashSha512() = स्वभाव;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    स्थिर constexpr आकार_t DIGEST_SYMBOL24_दीर्घता = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_द्विमान();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    स्थिर शून्य selfTest();  // Test this class

    // Inserting hash data
    शून्य निवेशय(ध्रुव शून्य* datap, आकार_t दीर्घता_2);  // Process data into the digest
    शून्य निवेशय(ध्रुव string& दत्त) {
        निवेशय(दत्त.दत्त(), दत्त.दीर्घता_2());
    }  // Process data into the digest
    शून्य निवेशय(uint64_t मानम्) { निवेशय(cvt_गन्तव्यम्_Str(मानम्)); }
    शून्य निवेशय_File(ध्रुव string& filename);

गुप्त:
    स्थिर शून्य selfTestOne(ध्रुव string& दत्त, ध्रुव string& data2, ध्रुव string& घातांकः,
                            ध्रुव string& exp64, ध्रुव string& exp24);
    शून्य finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    स्थिर std::map<string, string> s_dehashMap;  // hashed -> original decoder

    स्थिर आकार_t s_max_दीर्घता;  // Length at which to start hashing
    स्थिर आकार_t s_min_दीर्घता;  // Length to preserve if over maxLength

सार्वजनिक:
    // CONSTRUCTORS
    explicit VName(ध्रुव string& name)
        : m_name{name} {}
    ~VName() = स्वभाव;
    // METHODS
    शून्य name(ध्रुव string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() ध्रुव { प्रत्यागम m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    स्थिर शून्य max_दीर्घता(आकार_t flag) { s_max_दीर्घता = flag; }
    स्थिर string dehash(ध्रुव string& अन्तर्);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    स्थिर constexpr अचिह्नित अङ्कः_CANDIDATE_सीमा = 10000;  // Avoid searching huge netlists
    स्थिर constexpr अचिह्नित दीर्घता_सीमा = 100;  // Maximum string length to search
    // TYPES
    using EditDistance = अचिह्नित पूर्णाङ्क;
    संरचना Candidate final {
        std::string str;  // String to be matched
        std::string सन्देशः_2;  // Message to print on a match
    };
    // MEMBERS
    std::सदिशम्<Candidate> m_candidates;  // Strings we try to match
सार्वजनिक:
    // CONSTRUCTORS
    VSpellCheck() = स्वभाव;
    ~VSpellCheck() = स्वभाव;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    शून्य प्रक्षिप_Candidate(ध्रुव string& str) { प्रक्षिप_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    शून्य प्रक्षिप_Candidate(ध्रुव string& str, ध्रुव string& सन्देशः_2) {
        यदि (m_candidates.आकार() < अङ्कः_CANDIDATE_सीमा) m_candidates.प्रक्षिप_पृष्ठभागः({str, सन्देशः_2});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate bestCandidate(ध्रुव string& goal) ध्रुव {
        EditDistance dist;
        प्रत्यागम bestCandidateInfo(goal, dist /*ref*/);
    }
    // Return friendly message
    string best_Candidate_सन्देशः(ध्रुव string& goal) ध्रुव {
        ध्रुव स्वचालित candidate = bestCandidate(goal);
        यदि (candidate.सन्देशः_2.रिक्तानि()) प्रत्यागम "";
        प्रत्यागम "... Suggested alternative: '"s + candidate.सन्देशः_2 + "'";
    }
    स्थिर शून्य selfTest();
    std::सदिशम्<std::string> candidates() ध्रुव {
        std::सदिशम्<std::string> परिणाम;
        परिणाम.reserve(m_candidates.आकार());
        प्रति (ध्रुव स्वचालित& candidate : m_candidates) { परिणाम.प्रक्षिप_पृष्ठभागः(candidate.सन्देशः_2); }
        प्रत्यागम परिणाम;
    }

गुप्त:
    स्थिर EditDistance editDistance(ध्रुव string& s, ध्रुव string& t);
    स्थिर EditDistance cutoffDistance(आकार_t goal_len, आकार_t candidate_len);
    VSpellCheck::Candidate bestCandidateInfo(ध्रुव string& goal, EditDistance& distancer) ध्रुव;
    स्थिर शून्य selfTestDistanceOne(ध्रुव string& a, ध्रुव string& b, EditDistance expected);
    स्थिर शून्य selfTestSuggestOne(bool matches, ध्रुव string& c, ध्रुव string& goal,
                                   EditDistance dist);
};

//######################################################################

#यदि_अन्तः  // guard
