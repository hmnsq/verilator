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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3UNIQUENAMES_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3UNIQUENAMES_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>

class V3_Unique_ਨਾਂ final {
    std::string m_ਅਗੇਤਰ;  // Prefix to attach to all names

    std::map<std::string, ਬਿਨਾਨਿਸ਼ਾਨੀ> m_multiplicity;  // Suffix number for given key

    bool m_ਜੋੜੋ_Suffix = true;  // Ad suffix or not

ਜਨਤਕ:
    V3_Unique_ਨਾਂ() = ਮੂਲ;
    explicit V3_Unique_ਨਾਂ(ਅਚਲ std::string& ਅਗੇਤਰ, bool ਜੋੜੋ_Suffix = true)
        : m_ਅਗੇਤਰ{ਅਗੇਤਰ}
        , m_ਜੋੜੋ_Suffix{ਜੋੜੋ_Suffix} {
        ਜੇ (!m_ਅਗੇਤਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            UASSERT(VString::startsWith(m_ਅਗੇਤਰ, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::ਸਿਰੇ_With(m_ਅਗੇਤਰ, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ਅਚਲ std::string& name) {
        ਜੇ (!m_ਜੋੜੋ_Suffix) {
            ਜੇ (m_multiplicity.ਗਿਣਤੀ_2(name) == 0) {
                m_multiplicity[name] = 0;
                ਵਾਪਸ name;
            }
            ਵਾਪਸ get(name + "__" + cvt_ਮੰਜ਼ਿਲ_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਅੰਕ = m_multiplicity[name]++;
        std::string ਨਤੀਜਾ;
        ਜੇ (!m_ਅਗੇਤਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਨਤੀਜਾ += m_ਅਗੇਤਰ;
            ਨਤੀਜਾ += "_";
        }
        ਨਤੀਜਾ += name;
        ਨਤੀਜਾ += "__";
        ਨਤੀਜਾ += cvt_ਮੰਜ਼ਿਲ_Str(ਅੰਕ);
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep == nullptr) { ਵਾਪਸ get(""); }
        ਵਾਪਸ get(V3Hasher::uncachedHash(nodep).ਮੰਜ਼ਿਲ_String());
    }

    // Reset to initial state (as if just constructed)
    ਖਾਲੀ reset() { m_multiplicity.clear(); }
};

#ਜੇ_ਅੰਤ  // Guard
