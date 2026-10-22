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

#నిర్వచితం_కాకపోతే VERILATOR_V3STRING_H_
#నిర్వచించు VERILATOR_V3STRING_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

// No V3 headers here - this is a base class for Vlc etc

#అన్తర్భావ <deque>
#అన్తర్భావ <iomanip>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <sstream>
#అన్తర్భావ <string>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

using VStringList = std::సదిశ<std::string>;
using VString_సమితి = std::సమితి<std::string>;

//######################################################################
// Global string-related functions

మూస <typename T>
std::string cvt_గమ్యం_Str(స్థిరరాశి T& t) VL_శుద్ధ {
    std::ostringstream os;
    os << t;
    తిరిగి os.str();
}
మూస <typename T>
typename std::enable_if<std::is_సూచిక<T>::విలువ, std::string>::type
cvt_గమ్యం_Hex(స్థిరరాశి T tp) VL_శుద్ధ {
    std::ostringstream os;
    os << static_cast<స్థిరరాశి శూన్య*>(tp);
    తిరిగి os.str();
}
మూస <typename T>
typename std::enable_if<std::is_integral<T>::విలువ, std::string>::type cvt_గమ్యం_Hex(స్థిరరాశి T t) {
    std::ostringstream os;
    os << std::hex << std::setw(పరిమాణం(T) * 8 / 4) << std::setfill('0') << t;
    తిరిగి os.str();
}

అంతఃవరుస string ucfirst(స్థిరరాశి string& పాఠ్యం) {
    string ఫలితం = పాఠ్యం;
    ఫలితం[0] = std::పెద్ద_అక్షరం(ఫలితం[0]);
    తిరిగి ఫలితం;
}

//######################################################################
// VString - String manipulation

class VString final {
    స్థిర bool wildmatchi(స్థిరరాశి అక్షరం* s, స్థిరరాశి అక్షరం* p) VL_శుద్ధ;

బహిరంగ:
    // METHODS (generic string utilities)
    // Return true if p with ? or *'s matches s
    స్థిర bool wildmatch(స్థిరరాశి అక్షరం* s, స్థిరరాశి అక్షరం* p) VL_శుద్ధ;
    // Return true if p with ? or *'s matches s
    స్థిర bool wildmatch(స్థిరరాశి string& s, స్థిరరాశి string& p) VL_శుద్ధ;
    // Return {a}{dot}{b}, omitting dot if a or b are empty
    స్థిర string dot(స్థిరరాశి string& a, స్థిరరాశి string& dot, స్థిరరాశి string& b);
    // Convert string to lowercase (tolower)
    స్థిర string downcase(స్థిరరాశి string& str) VL_శుద్ధ;
    // Convert string to upper case (toupper)
    స్థిర string upcase(స్థిరరాశి string& str) VL_శుద్ధ;
    // Insert esc just before tgt
    స్థిర string quoteAny(స్థిరరాశి string& str, అక్షరం tgt, అక్షరం esc) VL_శుద్ధ;
    // Replace any \'s with \\  (two consecutive backslashes)
    స్థిర string quoteBackslash(స్థిరరాశి string& str) VL_శుద్ధ { తిరిగి quoteAny(str, '\\', '\\'); }
    // Replace any %'s with %%
    స్థిర string quotePercent(స్థిరరాశి string& str) VL_శుద్ధ { తిరిగి quoteAny(str, '%', '%'); }
    // Replace any %%'s with %
    స్థిర string dequotePercent(స్థిరరాశి string& str);
    // Surround a raw string by double quote and escape if necessary
    // e.g. input abc's  becomes "\"abc\'s\""
    స్థిర string escape_String_For_మార్గం(స్థిరరాశి string& str);
    // Convert SV quoted string input from source code to normal form.
    // Reverse is V3OutFormatter::quoteNameControls(...)
    స్థిర string unquoteSVString(స్థిరరాశి string& పాఠ్యం, string& err_బహిర్గతం);
    // Position just past the string literal 'pos' opens with '"', as written by
    // V3OutFormatter::quoteNameControls(...), whose escapes each begin with '\', or npos if the
    // literal does not end
    స్థిర కొలత_t quoted_చివర(స్థిరరాశి string& str, కొలత_t pos) VL_శుద్ధ;
    // Escape path in Windows
    // e.g. input `C:\Program Files\My Program\My Program.exe` becomes
    // `C:\\Program\ Files\\My\ Program\\My\ Program.exe`
    స్థిర string quoteStringLiteralForShell(స్థిరరాశి string& str);
    // Replace any unprintable with space
    // This includes removing tabs, so column tracking is correct
    స్థిర string spaceUnprintable(స్థిరరాశి string& str) VL_శుద్ధ;
    // Remove any whitespace
    స్థిర string removeWhitespace(స్థిరరాశి string& str);
    // Trim leading/trailing whitespace on each line
    స్థిర string trimWhitespace(స్థిరరాశి string& str);
    // Return true if only identifer or ""
    స్థిర bool isIdentifier(స్థిరరాశి string& str);
    // Return true if char is valid character in C identifiers
    స్థిర bool isIdentifierChar(అక్షరం c) { తిరిగి అక్షరమా_అంకెనా(c) || c == '_'; }
    // Return true if only whitespace or ""
    స్థిర bool isWhitespace(స్థిరరాశి string& str);
    // Return number of spaces/tabs leading in string
    స్థిర string::కొలత_type leading_Whitespace_లెక్క(స్థిరరాశి string& str);
    // Return double by parsing string
    స్థిర ద్విగుణ parseDouble(స్థిరరాశి string& str, bool* successp);
    // Replace substring. Often replaceWord is more appropriate.
    స్థిర string replaceSubstr(స్థిరరాశి string& str, స్థిరరాశి string& from, స్థిరరాశి string& గమ్యం);
    // Replace all occurrences of the word 'from' in 'str' with 'to'. A word is considered
    // to be a consecutive sequence of the characters [a-zA-Z0-9_]. Sub-words are not replaced.
    // e.g.: replaceWords("one apple bad_apple", "apple", "banana") -> "one banana bad_apple"
    స్థిర string replace_మాట(స్థిరరాశి string& str, స్థిరరాశి string& from, స్థిరరాశి string& గమ్యం);
    // Return deque of strings split by the given delimiter
    స్థిర std::deque<string> విభజించు(స్థిరరాశి string& str, అక్షరం delimiter = ',');
    // Predicate to check if 'str' starts with 'prefix'
    స్థిర bool startsWith(స్థిరరాశి string& str, స్థిరరాశి అక్షరం* prefixp) {
        తిరిగి !str.rfind(prefixp, 0);  // Faster than .find(_) == 0
    }
    స్థిర bool startsWith(స్థిరరాశి string& str, స్థిరరాశి string& ఉపసర్గ) {
        తిరిగి !str.rfind(ఉపసర్గ, 0);  // Faster than .find(_) == 0
    }
    // Predicate to check if 'str' ends with 'suffix'
    స్థిర bool చివర్లు_With(స్థిరరాశి string& str, స్థిరరాశి string& suffix);
    // Return proper article (a/an) for a word. May be inaccurate for some special words
    స్థిర string aOrAn(స్థిరరాశి అక్షరం* మాట);
    స్థిర string aOrAn(స్థిరరాశి string& మాట) { తిరిగి aOrAn(మాట.c_str()); }
    // Hash the string
    స్థిర uint64_t hashMurmur(స్థిరరాశి string& str) VL_శుద్ధ;

    // Base 64 encode
    స్థిర string base64Enc(స్థిరరాశి string& str) VL_శుద్ధ;

    స్థిర శూన్య selfTest();  // Test this class
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
    కొలత_t m_tot_నిడివి = 0;  // Total all-chunk length as needed by output digest
    bool m_final = false;  // Finalized
బహిరంగ:
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
    explicit VHashSha512(స్థిరరాశి string& దత్తాంశం)
        : VHashSha512{} {
        చొప్పించు(దత్తాంశం);
    }
    ~VHashSha512() = అప్రమేయ;

    // CONSTANTS
    // Characters digestSymbol24 keeps, 6 bits each, so 144 bits and no collisions in practice
    స్థిర constexpr కొలత_t DIGEST_SYMBOL24_నిడివి = 24;

    // METHODS
    string digestBase64();  // Return digest formatted in Base64 (non-URL flavor)
    string digest_ద్విమాన();  // Return digest as 64 character binary
    string digestHex();  // Return digest formatted as a hex string
    string digestSymbol24();  // Return non-reversable short-formatted as C symbol/base64ish
    uint64_t digestUInt64();  // Return 64-bits of digest
    స్థిర శూన్య selfTest();  // Test this class

    // Inserting hash data
    శూన్య చొప్పించు(స్థిరరాశి శూన్య* datap, కొలత_t నిడివి);  // Process data into the digest
    శూన్య చొప్పించు(స్థిరరాశి string& దత్తాంశం) {
        చొప్పించు(దత్తాంశం.దత్తాంశం(), దత్తాంశం.నిడివి());
    }  // Process data into the digest
    శూన్య చొప్పించు(uint64_t విలువ) { చొప్పించు(cvt_గమ్యం_Str(విలువ)); }
    శూన్య చొప్పించు_File(స్థిరరాశి string& filename);

అంతరంగ:
    స్థిర శూన్య selfTestOne(స్థిరరాశి string& దత్తాంశం, స్థిరరాశి string& data2, స్థిరరాశి string& ఘాతాంకం,
                            స్థిరరాశి string& exp64, స్థిరరాశి string& exp24);
    శూన్య finalize();  // Process remaining data
};

//######################################################################
// VName - string which contains a possibly hashed string
// TODO use this wherever there is currently a "string m_name"

class VName final {
    string m_name;  // Pre-hashed symbol name
    string m_hashed;  // Hashed version of symbol name
    స్థిర std::map<string, string> s_dehashMap;  // hashed -> original decoder

    స్థిర కొలత_t s_max_నిడివి;  // Length at which to start hashing
    స్థిర కొలత_t s_min_నిడివి;  // Length to preserve if over maxLength

బహిరంగ:
    // CONSTRUCTORS
    explicit VName(స్థిరరాశి string& name)
        : m_name{name} {}
    ~VName() = అప్రమేయ;
    // METHODS
    శూన్య name(స్థిరరాశి string& name) {
        m_name = name;
        m_hashed = "";
    }
    string name() స్థిరరాశి { తిరిగి m_name; }
    string hashedName();
    // CONFIG STATIC METHODS
    // Length at which to start hashing, 0=disable
    స్థిర శూన్య max_నిడివి(కొలత_t flag) { s_max_నిడివి = flag; }
    స్థిర string dehash(స్థిరరాశి string& లోపల);
};

//######################################################################
// VSpellCheck - Find near-match spelling suggestions given list of possibilities

class VSpellCheck final {
    // CONSTANTS
    స్థిర constexpr గుర్తులేని అంకె_CANDIDATE_హద్దు = 10000;  // Avoid searching huge netlists
    స్థిర constexpr గుర్తులేని నిడివి_హద్దు = 100;  // Maximum string length to search
    // TYPES
    using Edit_దూరం = గుర్తులేని పూర్ణాంకం;
    నిర్మాణం Candidate final {
        std::string str;  // String to be matched
        std::string సందేశం_2;  // Message to print on a match
    };
    // MEMBERS
    std::సదిశ<Candidate> m_candidates;  // Strings we try to match
బహిరంగ:
    // CONSTRUCTORS
    VSpellCheck() = అప్రమేయ;
    ~VSpellCheck() = అప్రమేయ;
    // METHODS
    // Push a symbol table value to be considered as a candidate
    // The first item pushed has highest priority, all else being equal
    శూన్య నెట్టు_Candidate(స్థిరరాశి string& str) { నెట్టు_Candidate(str, str); }
    // First argument is string to be matched, the second one is message to print on success
    శూన్య నెట్టు_Candidate(స్థిరరాశి string& str, స్థిరరాశి string& సందేశం_2) {
        ఒకవేళ (m_candidates.కొలత() < అంకె_CANDIDATE_హద్దు) m_candidates.నెట్టు_వెనుక({str, సందేశం_2});
    }
    // Return candidate is closest to provided string, or "" for none
    VSpellCheck::Candidate ఉత్తమ_Candidate(స్థిరరాశి string& goal) స్థిరరాశి {
        Edit_దూరం dist;
        తిరిగి ఉత్తమ_Candidate_Info(goal, dist /*ref*/);
    }
    // Return friendly message
    string ఉత్తమ_Candidate_సందేశం(స్థిరరాశి string& goal) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక candidate = ఉత్తమ_Candidate(goal);
        ఒకవేళ (candidate.సందేశం_2.ఖాళీలు()) తిరిగి "";
        తిరిగి "... Suggested alternative: '"s + candidate.సందేశం_2 + "'";
    }
    స్థిర శూన్య selfTest();
    std::సదిశ<std::string> candidates() స్థిరరాశి {
        std::సదిశ<std::string> ఫలితం;
        ఫలితం.reserve(m_candidates.కొలత());
        కొరకు (స్థిరరాశి స్వయంచాలక& candidate : m_candidates) { ఫలితం.నెట్టు_వెనుక(candidate.సందేశం_2); }
        తిరిగి ఫలితం;
    }

అంతరంగ:
    స్థిర Edit_దూరం edit_దూరం(స్థిరరాశి string& s, స్థిరరాశి string& t);
    స్థిర Edit_దూరం cutoff_దూరం(కొలత_t goal_len, కొలత_t candidate_len);
    VSpellCheck::Candidate ఉత్తమ_Candidate_Info(స్థిరరాశి string& goal, Edit_దూరం& distancer) స్థిరరాశి;
    స్థిర శూన్య self_Test_దూరం_One(స్థిరరాశి string& a, స్థిరరాశి string& b, Edit_దూరం expected);
    స్థిర శూన్య selfTestSuggestOne(bool matches, స్థిరరాశి string& c, స్థిరరాశి string& goal,
                                   Edit_దూరం dist);
};

//######################################################################

#అయితే_ముగింపు  // guard
