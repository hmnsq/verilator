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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3LANGCODE_H_
#وصف_ڪريو VERILATOR_V3LANGCODE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <map>
#ضمن <set>
#ضمن <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3_Lang_ڪوڊ final {
عام:
    تعداد en : uint8_t {
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
        ENUM_پڇاڙي
    };
    ثابت حرف* ascii() ثابت {
        ثابت حرف* ثابت نالا[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        أرجع نالا[m_e];
    }
    ساكن V3_Lang_ڪوڊ mostRecent() VL_MT_محفوظ { أرجع V3_Lang_ڪوڊ{L1800_2023}; }
    bool systemVerilog() ثابت {
        أرجع m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() ثابت { أرجع m_e != L_ERROR; }
    //
    تعداد en m_e;
    V3_Lang_ڪوڊ()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Lang_ڪوڊ(en _e)
        : m_e{_e} {}
    explicit V3_Lang_ڪوڊ(ثابت حرف* textp);
    explicit V3_Lang_ڪوڊ(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
};

//######################################################################

#جيڪڏهن_پورو  // guard
