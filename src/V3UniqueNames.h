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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Hasher.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <string>

class V3UniqueNames final {
    std::string m_prefix;  // Prefix to attach to all names

    std::map<std::string, unsigned> m_multiplicity;  // Suffix number for given key

    bool m_addSuffix = true;  // Ad suffix or not

જાહેર:
    V3UniqueNames() = મૂળભૂત;
    explicit V3UniqueNames(અચલ std::string& prefix, bool addSuffix = true)
        : m_prefix{prefix}
        , m_addSuffix{addSuffix} {
        જો (!m_prefix.empty()) {
            UASSERT(VString::startsWith(m_prefix, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::endsWith(m_prefix, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(અચલ std::string& name) {
        જો (!m_addSuffix) {
            જો (m_multiplicity.ગણતરી(name) == 0) {
                m_multiplicity[name] = 0;
                પાછા name;
            }
            પાછા get(name + "__" + cvtToStr(m_multiplicity[name]++));
        }
        // NORMAL mode
        અચલ unsigned અંક = m_multiplicity[name]++;
        std::string result;
        જો (!m_prefix.empty()) {
            result += m_prefix;
            result += "_";
        }
        result += name;
        result += "__";
        result += cvtToStr(અંક);
        પાછા result;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(અચલ Ast_ગાંઠ* nodep) {
        જો (nodep == nullptr) { પાછા get(""); }
        પાછા get(V3Hasher::uncachedHash(nodep).toString());
    }

    // Reset to initial state (as if just constructed)
    શૂન્ય reset() { m_multiplicity.clear(); }
};

#endif  // Guard
