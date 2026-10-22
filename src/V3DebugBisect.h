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

#ifndef VL_MT_DISABLED_CODE_UNIT
#error "V3DebugBisect.h uses global state and is not thread-safe"
#endif

#ଅନ୍ତର୍ଭାଵ "V3Os.h"

#ଅନ୍ତର୍ଭାଵ <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    ଅଚଳ ଅକ୍ଷର* ଅଚଳ m_namep;
    // Limit for stopping - 0 means no limit
    ଅଚଳ size_t m_limit = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    size_t m_ଗଣନା = 0;

ସର୍ବସାଧାରଣ:
    explicit V3DebugBisect(ଅଚଳ ଅକ୍ଷର* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    template <typename Callable>
    bool stop(Callable&& f) {
        ଯଦି (VL_LIKELY(!m_limit)) ଫେରନ୍ତୁ false;
        ++m_ଗଣନା;
        ଯଦି (VL_UNLIKELY(m_ଗଣନା == m_limit)) f();
        ଫେରନ୍ତୁ m_ଗଣନା > m_limit;
    }

    // Returns true if the limit has been reached
    bool isStopped() ଅଚଳ { ଫେରନ୍ତୁ m_ଗଣନା > m_limit; }
};
