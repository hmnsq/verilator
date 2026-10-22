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

#اگر_تعریف_نشده VERILATOR_V3UNIQUENAMES_H_
#تعریف_کن VERILATOR_V3UNIQUENAMES_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Hasher.h"

#ضمن <map>
#ضمن <string>

class V3_Unique_نامها final {
    std::string m_پیشوند;  // Prefix to attach to all names

    std::map<std::string, بیعلامت> m_multiplicity;  // Suffix number for given key

    bool m_جمع_کن_Suffix = true;  // Ad suffix or not

عام:
    V3_Unique_نامها() = افتراضي;
    explicit V3_Unique_نامها(ثابت std::string& پیشوند, bool جمع_کن_Suffix = true)
        : m_پیشوند{پیشوند}
        , m_جمع_کن_Suffix{جمع_کن_Suffix} {
        إذا (!m_پیشوند.خالی()) {
            UASSERT(VString::startsWith(m_پیشوند, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::دوسر_With(m_پیشوند, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ثابت std::string& name) {
        إذا (!m_جمع_کن_Suffix) {
            إذا (m_multiplicity.شمار(name) == 0) {
                m_multiplicity[name] = 0;
                أرجع name;
            }
            أرجع get(name + "__" + cvt_مقصد_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ثابت بیعلامت رقم = m_multiplicity[name]++;
        std::string نتیجه;
        إذا (!m_پیشوند.خالی()) {
            نتیجه += m_پیشوند;
            نتیجه += "_";
        }
        نتیجه += name;
        نتیجه += "__";
        نتیجه += cvt_مقصد_Str(رقم);
        أرجع نتیجه;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ثابت Ast_گره* nodep) {
        إذا (nodep == nullptr) { أرجع get(""); }
        أرجع get(V3Hasher::uncachedHash(nodep).مقصد_String());
    }

    // Reset to initial state (as if just constructed)
    فراغ reset() { m_multiplicity.clear(); }
};

#پایان_اگر  // Guard
