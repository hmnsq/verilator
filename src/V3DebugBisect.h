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

#यदि_अपरिभाषितम् VL_MT_DISABLED_संहिता_UNIT
#दोषः "V3DebugBisect.h uses global state and is not thread-safe"
#यदि_अन्तः

#अन्तर्भाव "V3Os.h"

#अन्तर्भाव <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    ध्रुव वर्ण* ध्रुव m_namep;
    // Limit for stopping - 0 means no limit
    ध्रुव आकार_t m_सीमा = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    आकार_t m_गणनम् = 0;

सार्वजनिक:
    explicit V3DebugBisect(ध्रुव वर्ण* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    प्रतिरूपम् <typename Callable>
    bool stop(Callable&& f) {
        यदि (VL_LIKELY(!m_सीमा)) प्रत्यागम false;
        ++m_गणनम्;
        यदि (VL_UNLIKELY(m_गणनम् == m_सीमा)) f();
        प्रत्यागम m_गणनम् > m_सीमा;
    }

    // Returns true if the limit has been reached
    bool isStopped() ध्रुव { प्रत्यागम m_गणनम् > m_सीमा; }
};
