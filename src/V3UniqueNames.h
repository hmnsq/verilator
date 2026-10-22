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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3UNIQUENAMES_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3UNIQUENAMES_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Hasher.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <string>

class V3_Unique_ಹೆಸರುಗಳು final {
    std::string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Prefix to attach to all names

    std::map<std::string, ಚಿಹ್ನೆರಹಿತ> m_multiplicity;  // Suffix number for given key

    bool m_ಕೂಡಿಸು_Suffix = true;  // Ad suffix or not

ಸಾರ್ವಜನಿಕ:
    V3_Unique_ಹೆಸರುಗಳು() = ಪೂರ್ವನಿಯೋಜಿತ;
    explicit V3_Unique_ಹೆಸರುಗಳು(ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ, bool ಕೂಡಿಸು_Suffix = true)
        : m_ಪೂರ್ವಪ್ರತ್ಯಯ{ಪೂರ್ವಪ್ರತ್ಯಯ}
        , m_ಕೂಡಿಸು_Suffix{ಕೂಡಿಸು_Suffix} {
        ಆದರೆ (!m_ಪೂರ್ವಪ್ರತ್ಯಯ.ಖಾಲಿಗಳು()) {
            UASSERT(VString::startsWith(m_ಪೂರ್ವಪ್ರತ್ಯಯ, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::ತುದಿಗಳು_With(m_ಪೂರ್ವಪ್ರತ್ಯಯ, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(ಸ್ಥಿರಾಂಕ std::string& name) {
        ಆದರೆ (!m_ಕೂಡಿಸು_Suffix) {
            ಆದರೆ (m_multiplicity.ಎಣಿಕೆ_2(name) == 0) {
                m_multiplicity[name] = 0;
                ಹಿಂತಿರುಗಿ name;
            }
            ಹಿಂತಿರುಗಿ get(name + "__" + cvt_ಗಮ್ಯ_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಅಂಕೆ = m_multiplicity[name]++;
        std::string ಫಲಿತಾಂಶ;
        ಆದರೆ (!m_ಪೂರ್ವಪ್ರತ್ಯಯ.ಖಾಲಿಗಳು()) {
            ಫಲಿತಾಂಶ += m_ಪೂರ್ವಪ್ರತ್ಯಯ;
            ಫಲಿತಾಂಶ += "_";
        }
        ಫಲಿತಾಂಶ += name;
        ಫಲಿತಾಂಶ += "__";
        ಫಲಿತಾಂಶ += cvt_ಗಮ್ಯ_Str(ಅಂಕೆ);
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಆದರೆ (nodep == nullptr) { ಹಿಂತಿರುಗಿ get(""); }
        ಹಿಂತಿರುಗಿ get(V3Hasher::uncachedHash(nodep).ಗಮ್ಯ_String());
    }

    // Reset to initial state (as if just constructed)
    ಶೂನ್ಯ reset() { m_multiplicity.clear(); }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
