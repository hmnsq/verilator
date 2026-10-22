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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <map>
#شامل <set>
#شامل <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3LangCode final {
عمومی:
    شمارش en : uint8_t {
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
    ثابت نویسه* ascii() ثابت {
        ثابت نویسه* ثابت names[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        بازگشت names[m_e];
    }
    ایستا V3LangCode mostRecent() VL_MT_SAFE { بازگشت V3LangCode{L1800_2023}; }
    bool systemVerilog() ثابت {
        بازگشت m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() ثابت { بازگشت m_e != L_ERROR; }
    //
    شمارش en m_e;
    V3LangCode()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3LangCode(en _e)
        : m_e{_e} {}
    explicit V3LangCode(ثابت نویسه* textp);
    explicit V3LangCode(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
};

//######################################################################

#endif  // guard
