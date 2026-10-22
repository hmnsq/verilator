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

#ifndef VERILATOR_V3LANGCODE_H_
#define VERILATOR_V3LANGCODE_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3LangCode final {
જાહેર:
    ગણના en : uint8_t {
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
        _ENUM_END
    };
    અચલ અક્ષર* ascii() અચલ {
        અચલ અક્ષર* અચલ names[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        પાછા names[m_e];
    }
    સ્થિર V3LangCode mostRecent() VL_MT_SAFE { પાછા V3LangCode{L1800_2023}; }
    bool systemVerilog() અચલ {
        પાછા m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() અચલ { પાછા m_e != L_ERROR; }
    //
    ગણના en m_e;
    V3LangCode()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3LangCode(en _e)
        : m_e{_e} {}
    explicit V3LangCode(અચલ અક્ષર* textp);
    explicit V3LangCode(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
};

//######################################################################

#endif  // guard
