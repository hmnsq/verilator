// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Basic data structure to keep names unique
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
//*************************************************************************

#यदि_अपरिभाषितम् VERILATOR_V3UNIQUENAMES_H_
#परिभाषय VERILATOR_V3UNIQUENAMES_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Hasher.h"

#अन्तर्भाव <map>
#अन्तर्भाव <string>

class V3_Unique_नामानि final {
    std::string m_उपसर्गः;  // Prefix to attach to all names

    std::map<std::string, अचिह्नित> m_multiplicity;  // Suffix number for given key

    bool m_सङ्कलय_Suffix = true;  // Ad suffix or not

सार्वजनिक:
    V3_Unique_नामानि() = स्वभाव;
    explicit V3_Unique_नामानि(ध्रुव std::string& उपसर्गः, bool सङ्कलय_Suffix = true)
        : m_उपसर्गः{उपसर्गः}
        , m_सङ्कलय_Suffix{सङ्कलय_Suffix} {
        यदि (!m_उपसर्गः.रिक्तानि()) {
            UASSERT(VString::startsWith(m_उपसर्गः, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::अन्ताः_With(m_उपसर्गः, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ध्रुव std::string& name) {
        यदि (!m_सङ्कलय_Suffix) {
            यदि (m_multiplicity.गणनम्(name) == 0) {
                m_multiplicity[name] = 0;
                प्रत्यागम name;
            }
            प्रत्यागम get(name + "__" + cvt_गन्तव्यम्_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ध्रुव अचिह्नित अङ्कः = m_multiplicity[name]++;
        std::string परिणाम;
        यदि (!m_उपसर्गः.रिक्तानि()) {
            परिणाम += m_उपसर्गः;
            परिणाम += "_";
        }
        परिणाम += name;
        परिणाम += "__";
        परिणाम += cvt_गन्तव्यम्_Str(अङ्कः);
        प्रत्यागम परिणाम;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (nodep == nullptr) { प्रत्यागम get(""); }
        प्रत्यागम get(V3Hasher::uncachedHash(nodep).गन्तव्यम्_String());
    }

    // Reset to initial state (as if just constructed)
    शून्य reset() { m_multiplicity.clear(); }
};

#यदि_अन्तः  // Guard
