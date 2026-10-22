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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Hasher.h"

#অন্তর্ভাব <map>
#অন্তর্ভাব <string>

class V3UniqueNames final {
    std::string m_prefix;  // Prefix to attach to all names

    std::map<std::string, unsigned> m_multiplicity;  // Suffix number for given key

    bool m_addSuffix = true;  // Ad suffix or not

সার্বজনিক:
    V3UniqueNames() = স্বভাব;
    explicit V3UniqueNames(ধ্রুব std::string& prefix, bool addSuffix = true)
        : m_prefix{prefix}
        , m_addSuffix{addSuffix} {
        যদি (!m_prefix.empty()) {
            UASSERT(VString::startsWith(m_prefix, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::endsWith(m_prefix, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ধ্রুব std::string& name) {
        যদি (!m_addSuffix) {
            যদি (m_multiplicity.গুনতি(name) == 0) {
                m_multiplicity[name] = 0;
                প্রত্যাগম name;
            }
            প্রত্যাগম get(name + "__" + cvtToStr(m_multiplicity[name]++));
        }
        // NORMAL mode
        ধ্রুব unsigned অঙ্ক = m_multiplicity[name]++;
        std::string result;
        যদি (!m_prefix.empty()) {
            result += m_prefix;
            result += "_";
        }
        result += name;
        result += "__";
        result += cvtToStr(অঙ্ক);
        প্রত্যাগম result;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ধ্রুব Ast_গ্রন্থি* nodep) {
        যদি (nodep == nullptr) { প্রত্যাগম get(""); }
        প্রত্যাগম get(V3Hasher::uncachedHash(nodep).toString());
    }

    // Reset to initial state (as if just constructed)
    শূন্য reset() { m_multiplicity.clear(); }
};

#endif  // Guard
