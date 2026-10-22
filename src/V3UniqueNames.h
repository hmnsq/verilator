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

#جے_بنا_تعریف VERILATOR_V3UNIQUENAMES_H_
#تعریف_کرو VERILATOR_V3UNIQUENAMES_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Hasher.h"

#ضمن <map>
#ضمن <string>

class V3_Unique_ناں final {
    std::string m_اگیتر;  // Prefix to attach to all names

    std::map<std::string, بنانشانی> m_multiplicity;  // Suffix number for given key

    bool m_جوڑو_Suffix = true;  // Ad suffix or not

عام:
    V3_Unique_ناں() = افتراضي;
    explicit V3_Unique_ناں(ثابت std::string& اگیتر, bool جوڑو_Suffix = true)
        : m_اگیتر{اگیتر}
        , m_جوڑو_Suffix{جوڑو_Suffix} {
        إذا (!m_اگیتر.خالی_تھاواں()) {
            UASSERT(VString::startsWith(m_اگیتر, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::سرے_With(m_اگیتر, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ثابت std::string& name) {
        إذا (!m_جوڑو_Suffix) {
            إذا (m_multiplicity.گنتی(name) == 0) {
                m_multiplicity[name] = 0;
                أرجع name;
            }
            أرجع get(name + "__" + cvt_منزل_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ثابت بنانشانی عدد = m_multiplicity[name]++;
        std::string نتیجہ;
        إذا (!m_اگیتر.خالی_تھاواں()) {
            نتیجہ += m_اگیتر;
            نتیجہ += "_";
        }
        نتیجہ += name;
        نتیجہ += "__";
        نتیجہ += cvt_منزل_Str(عدد);
        أرجع نتیجہ;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ثابت Ast_گرہ* nodep) {
        إذا (nodep == nullptr) { أرجع get(""); }
        أرجع get(V3Hasher::uncachedHash(nodep).منزل_String());
    }

    // Reset to initial state (as if just constructed)
    فراغ reset() { m_multiplicity.clear(); }
};

#جے_انت  // Guard
