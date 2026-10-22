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

#ifndef VERILATOR_V3UNIQUENAMES_H_
#define VERILATOR_V3UNIQUENAMES_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Hasher.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <string>

class V3UniqueNames final {
    std::string m_prefix;  // Prefix to attach to all names

    std::map<std::string, unsigned> m_multiplicity;  // Suffix number for given key

    bool m_addSuffix = true;  // Ad suffix or not

பொது:
    V3UniqueNames() = இயல்பு;
    explicit V3UniqueNames(மாறிலி std::string& prefix, bool addSuffix = true)
        : m_prefix{prefix}
        , m_addSuffix{addSuffix} {
        எனில் (!m_prefix.empty()) {
            UASSERT(VString::startsWith(m_prefix, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::endsWith(m_prefix, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(மாறிலி std::string& name) {
        எனில் (!m_addSuffix) {
            எனில் (m_multiplicity.மொத்தம்(name) == 0) {
                m_multiplicity[name] = 0;
                திரும்பு name;
            }
            திரும்பு get(name + "__" + cvtToStr(m_multiplicity[name]++));
        }
        // NORMAL mode
        மாறிலி unsigned இலக்கம் = m_multiplicity[name]++;
        std::string result;
        எனில் (!m_prefix.empty()) {
            result += m_prefix;
            result += "_";
        }
        result += name;
        result += "__";
        result += cvtToStr(இலக்கம்);
        திரும்பு result;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(மாறிலி Ast_முனை* nodep) {
        எனில் (nodep == nullptr) { திரும்பு get(""); }
        திரும்பு get(V3Hasher::uncachedHash(nodep).toString());
    }

    // Reset to initial state (as if just constructed)
    வெறுமை reset() { m_multiplicity.clear(); }
};

#endif  // Guard
