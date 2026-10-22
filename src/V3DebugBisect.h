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

#که_نه_تعریف_شوی VL_MT_DISABLED_کوډ_UNIT
#تېروتنه "V3DebugBisect.h uses global state and is not thread-safe"
#د_که_پای

#ضمن "V3Os.h"

#ضمن <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    ثابت حرف* ثابت m_namep;
    // Limit for stopping - 0 means no limit
    ثابت اندازه_t m_حد = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    اندازه_t m_شمېر = 0;

عام:
    explicit V3DebugBisect(ثابت حرف* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    کينډۍ <typename Callable>
    bool stop(Callable&& f) {
        إذا (VL_LIKELY(!m_حد)) أرجع false;
        ++m_شمېر;
        إذا (VL_UNLIKELY(m_شمېر == m_حد)) f();
        أرجع m_شمېر > m_حد;
    }

    // Returns true if the limit has been reached
    bool isStopped() ثابت { أرجع m_شمېر > m_حد; }
};
