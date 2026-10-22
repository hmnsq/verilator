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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Hasher.h"

#شامل <map>
#شامل <string>

class V3UniqueNames final {
    std::string m_prefix;  // Prefix to attach to all names

    std::map<std::string, unsigned> m_multiplicity;  // Suffix number for given key

    bool m_addSuffix = true;  // Ad suffix or not

عمومی:
    V3UniqueNames() = پیشفرض;
    explicit V3UniqueNames(ثابت std::string& prefix, bool addSuffix = true)
        : m_prefix{prefix}
        , m_addSuffix{addSuffix} {
        اگر (!m_prefix.empty()) {
            UASSERT(VString::startsWith(m_prefix, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::endsWith(m_prefix, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ثابت std::string& name) {
        اگر (!m_addSuffix) {
            اگر (m_multiplicity.شمار(name) == 0) {
                m_multiplicity[name] = 0;
                بازگشت name;
            }
            بازگشت get(name + "__" + cvtToStr(m_multiplicity[name]++));
        }
        // NORMAL mode
        ثابت unsigned رقم = m_multiplicity[name]++;
        std::string result;
        اگر (!m_prefix.empty()) {
            result += m_prefix;
            result += "_";
        }
        result += name;
        result += "__";
        result += cvtToStr(رقم);
        بازگشت result;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ثابت Ast_گره* nodep) {
        اگر (nodep == nullptr) { بازگشت get(""); }
        بازگشت get(V3Hasher::uncachedHash(nodep).toString());
    }

    // Reset to initial state (as if just constructed)
    تهی reset() { m_multiplicity.clear(); }
};

#endif  // Guard
