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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3UNIQUENAMES_H_
#وصف_ڪريو VERILATOR_V3UNIQUENAMES_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Hasher.h"

#ضمن <map>
#ضمن <string>

class V3_Unique_نالا final {
    std::string m_اڳياڙي;  // Prefix to attach to all names

    std::map<std::string, بنانشان> m_multiplicity;  // Suffix number for given key

    bool m_جوڙيو_Suffix = true;  // Ad suffix or not

عام:
    V3_Unique_نالا() = افتراضي;
    explicit V3_Unique_نالا(ثابت std::string& اڳياڙي, bool جوڙيو_Suffix = true)
        : m_اڳياڙي{اڳياڙي}
        , m_جوڙيو_Suffix{جوڙيو_Suffix} {
        إذا (!m_اڳياڙي.خالي_جايون()) {
            UASSERT(VString::startsWith(m_اڳياڙي, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::پڇاڙيون_With(m_اڳياڙي, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ثابت std::string& name) {
        إذا (!m_جوڙيو_Suffix) {
            إذا (m_multiplicity.ڳڻپ(name) == 0) {
                m_multiplicity[name] = 0;
                أرجع name;
            }
            أرجع get(name + "__" + cvt_منزل_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ثابت بنانشان انگ = m_multiplicity[name]++;
        std::string نتيجو;
        إذا (!m_اڳياڙي.خالي_جايون()) {
            نتيجو += m_اڳياڙي;
            نتيجو += "_";
        }
        نتيجو += name;
        نتيجو += "__";
        نتيجو += cvt_منزل_Str(انگ);
        أرجع نتيجو;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ثابت Ast_ڳنڍ* nodep) {
        إذا (nodep == nullptr) { أرجع get(""); }
        أرجع get(V3Hasher::uncachedHash(nodep).منزل_String());
    }

    // Reset to initial state (as if just constructed)
    فراغ reset() { m_multiplicity.clear(); }
};

#جيڪڏهن_پورو  // Guard
