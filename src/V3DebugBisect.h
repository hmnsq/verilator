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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VL_MT_DISABLED_ਕੋਡ_UNIT
#ਗਲਤੀ "V3DebugBisect.h uses global state and is not thread-safe"
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    ਅਚਲ ਅੱਖਰ* ਅਚਲ m_namep;
    // Limit for stopping - 0 means no limit
    ਅਚਲ ਮਾਪ_t m_ਹੱਦਬੰਦੀ = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    ਮਾਪ_t m_ਗਿਣਤੀ = 0;

ਜਨਤਕ:
    explicit V3DebugBisect(ਅਚਲ ਅੱਖਰ* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    ਸਾਂਚਾ <typename Callable>
    bool stop(Callable&& f) {
        ਜੇ (VL_LIKELY(!m_ਹੱਦਬੰਦੀ)) ਵਾਪਸ false;
        ++m_ਗਿਣਤੀ;
        ਜੇ (VL_UNLIKELY(m_ਗਿਣਤੀ == m_ਹੱਦਬੰਦੀ)) f();
        ਵਾਪਸ m_ਗਿਣਤੀ > m_ਹੱਦਬੰਦੀ;
    }

    // Returns true if the limit has been reached
    bool isStopped() ਅਚਲ { ਵਾਪਸ m_ਗਿਣਤੀ > m_ਹੱਦਬੰਦੀ; }
};
