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

#অন্তর্ভাব "V3Os.h"

#অন্তর্ভাব <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    ধ্রুবক অক্ষর* ধ্রুবক m_namep;
    // Limit for stopping - 0 means no limit
    ধ্রুবক আকার_t m_limit = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    আকার_t m_গুনতি = 0;

সর্বজনীন:
    explicit V3DebugBisect(ধ্রুবক অক্ষর* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    template <typename Callable>
    bool stop(Callable&& f) {
        যদি (VL_LIKELY(!m_limit)) ফেরত false;
        ++m_গুনতি;
        যদি (VL_UNLIKELY(m_গুনতি == m_limit)) f();
        ফেরত m_গুনতি > m_limit;
    }

    // Returns true if the limit has been reached
    bool isStopped() ধ্রুবক { ফেরত m_গুনতি > m_limit; }
};
