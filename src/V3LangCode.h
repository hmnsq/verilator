// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Language code class
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3LANGCODE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3LANGCODE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3_Lang_ಸಂಕೇತ final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        L_ERROR,  // Must be first.
        L1364_1995,
        L1364_2001,
        L1364_2005,
        L1800_2005,
        L1800_2009,
        L1800_2012,
        L1800_2017,
        L1800_2023,
        // ***Add new elements below also***
        ENUM_ಕೊನೆ
    };
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರ V3_Lang_ಸಂಕೇತ mostRecent() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ V3_Lang_ಸಂಕೇತ{L1800_2023}; }
    bool systemVerilog() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != L_ERROR; }
    //
    ಎಣಿಕೆ en m_e;
    V3_Lang_ಸಂಕೇತ()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Lang_ಸಂಕೇತ(en _e)
        : m_e{_e} {}
    explicit V3_Lang_ಸಂಕೇತ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp);
    explicit V3_Lang_ಸಂಕೇತ(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
