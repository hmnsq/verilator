// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Bisection serach debugging utility
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

#నిర్వచితం_కాకపోతే VL_MT_DISABLED_సంకేతం_UNIT
#దోషం "V3DebugBisect.h uses global state and is not thread-safe"
#అయితే_ముగింపు

#అన్తర్భావ "V3Os.h"

#అన్తర్భావ <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    స్థిరరాశి అక్షరం* స్థిరరాశి m_namep;
    // Limit for stopping - 0 means no limit
    స్థిరరాశి కొలత_t m_హద్దు = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    కొలత_t m_లెక్క = 0;

బహిరంగ:
    explicit V3DebugBisect(స్థిరరాశి అక్షరం* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    మూస <typename Callable>
    bool stop(Callable&& f) {
        ఒకవేళ (VL_LIKELY(!m_హద్దు)) తిరిగి false;
        ++m_లెక్క;
        ఒకవేళ (VL_UNLIKELY(m_లెక్క == m_హద్దు)) f();
        తిరిగి m_లెక్క > m_హద్దు;
    }

    // Returns true if the limit has been reached
    bool isStopped() స్థిరరాశి { తిరిగి m_లెక్క > m_హద్దు; }
};
