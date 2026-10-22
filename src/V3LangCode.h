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

#నిర్వచితం_కాకపోతే VERILATOR_V3LANGCODE_H_
#నిర్వచించు VERILATOR_V3LANGCODE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3_Lang_సంకేతం final {
బహిరంగ:
    గణన en : uint8_t {
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
        ENUM_చివర
    };
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిర V3_Lang_సంకేతం mostRecent() VL_MT_సురక్షిత { తిరిగి V3_Lang_సంకేతం{L1800_2023}; }
    bool systemVerilog() స్థిరరాశి {
        తిరిగి m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() స్థిరరాశి { తిరిగి m_e != L_ERROR; }
    //
    గణన en m_e;
    V3_Lang_సంకేతం()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Lang_సంకేతం(en _e)
        : m_e{_e} {}
    explicit V3_Lang_సంకేతం(స్థిరరాశి అక్షరం* textp);
    explicit V3_Lang_సంకేతం(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};

//######################################################################

#అయితే_ముగింపు  // guard
