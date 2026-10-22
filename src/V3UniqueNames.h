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

#اگر_غیر_تعریف_شدہ VERILATOR_V3UNIQUENAMES_H_
#تعریف_کرو VERILATOR_V3UNIQUENAMES_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Hasher.h"

#شامل <map>
#شامل <string>

class V3_Unique_نام final {
    std::string m_سابقہ;  // Prefix to attach to all names

    std::map<std::string, غیرعلامتی> m_multiplicity;  // Suffix number for given key

    bool m_جوڑو_Suffix = true;  // Ad suffix or not

عوامی:
    V3_Unique_نام() = طےشدہ;
    explicit V3_Unique_نام(ثابت std::string& سابقہ, bool جوڑو_Suffix = true)
        : m_سابقہ{سابقہ}
        , m_جوڑو_Suffix{جوڑو_Suffix} {
        اگر (!m_سابقہ.خالی_خانے()) {
            UASSERT(VString::startsWith(m_سابقہ, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::سرے_With(m_سابقہ, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ثابت std::string& name) {
        اگر (!m_جوڑو_Suffix) {
            اگر (m_multiplicity.گنتی(name) == 0) {
                m_multiplicity[name] = 0;
                واپس name;
            }
            واپس get(name + "__" + cvt_مقصد_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ثابت غیرعلامتی عدد = m_multiplicity[name]++;
        std::string نتیجہ;
        اگر (!m_سابقہ.خالی_خانے()) {
            نتیجہ += m_سابقہ;
            نتیجہ += "_";
        }
        نتیجہ += name;
        نتیجہ += "__";
        نتیجہ += cvt_مقصد_Str(عدد);
        واپس نتیجہ;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ثابت Ast_گرہ* nodep) {
        اگر (nodep == nullptr) { واپس get(""); }
        واپس get(V3Hasher::uncachedHash(nodep).مقصد_String());
    }

    // Reset to initial state (as if just constructed)
    خالی reset() { m_multiplicity.clear(); }
};

#اگر_ختم  // Guard
