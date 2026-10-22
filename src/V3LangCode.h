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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3LangCode final {
सार्वजनिक:
    गणना en : uint8_t {
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
    स्थिरांक अक्षर* ascii() स्थिरांक {
        स्थिरांक अक्षर* स्थिरांक names[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        परत names[m_e];
    }
    स्थिर V3LangCode mostRecent() VL_MT_SAFE { परत V3LangCode{L1800_2023}; }
    bool systemVerilog() स्थिरांक {
        परत m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() स्थिरांक { परत m_e != L_ERROR; }
    //
    गणना en m_e;
    V3LangCode()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3LangCode(en _e)
        : m_e{_e} {}
    explicit V3LangCode(स्थिरांक अक्षर* textp);
    explicit V3LangCode(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिरांक { परत m_e; }
};

//######################################################################

#endif  // guard
