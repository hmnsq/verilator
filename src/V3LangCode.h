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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LANGCODE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LANGCODE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3_Lang_ਕੋਡ final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
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
        ENUM_ਸਿਰਾ
    };
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਸਥਿਰ V3_Lang_ਕੋਡ mostRecent() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ V3_Lang_ਕੋਡ{L1800_2023}; }
    bool systemVerilog() ਅਚਲ {
        ਵਾਪਸ m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() ਅਚਲ { ਵਾਪਸ m_e != L_ERROR; }
    //
    ਗਿਣਤੀ en m_e;
    V3_Lang_ਕੋਡ()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Lang_ਕੋਡ(en _e)
        : m_e{_e} {}
    explicit V3_Lang_ਕੋਡ(ਅਚਲ ਅੱਖਰ* textp);
    explicit V3_Lang_ਕੋਡ(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
