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

#که_نه_تعریف_شوی VERILATOR_V3UNIQUENAMES_H_
#تعریف_کړه VERILATOR_V3UNIQUENAMES_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Hasher.h"

#ضمن <map>
#ضمن <string>

class V3_Unique_نومونه final {
    std::string m_مختاړی;  // Prefix to attach to all names

    std::map<std::string, بینښه> m_multiplicity;  // Suffix number for given key

    bool m_جمع_کړه_Suffix = true;  // Ad suffix or not

عام:
    V3_Unique_نومونه() = افتراضي;
    explicit V3_Unique_نومونه(ثابت std::string& مختاړی, bool جمع_کړه_Suffix = true)
        : m_مختاړی{مختاړی}
        , m_جمع_کړه_Suffix{جمع_کړه_Suffix} {
        إذا (!m_مختاړی.تشه()) {
            UASSERT(VString::startsWith(m_مختاړی, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::پایونه_With(m_مختاړی, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ثابت std::string& name) {
        إذا (!m_جمع_کړه_Suffix) {
            إذا (m_multiplicity.شمېر(name) == 0) {
                m_multiplicity[name] = 0;
                أرجع name;
            }
            أرجع get(name + "__" + cvt_منزل_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ثابت بینښه عدد = m_multiplicity[name]++;
        std::string پایله;
        إذا (!m_مختاړی.تشه()) {
            پایله += m_مختاړی;
            پایله += "_";
        }
        پایله += name;
        پایله += "__";
        پایله += cvt_منزل_Str(عدد);
        أرجع پایله;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ثابت Ast_غوټه* nodep) {
        إذا (nodep == nullptr) { أرجع get(""); }
        أرجع get(V3Hasher::uncachedHash(nodep).منزل_String());
    }

    // Reset to initial state (as if just constructed)
    فراغ reset() { m_multiplicity.clear(); }
};

#د_که_پای  // Guard
