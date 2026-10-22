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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VL_MT_DISABLED_ಸಂಕೇತ_UNIT
#ದೋಷ "V3DebugBisect.h uses global state and is not thread-safe"
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "V3Os.h"

#ಅನ್ತರ್ಭಾವ <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ m_namep;
    // Limit for stopping - 0 means no limit
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ಎಲ್ಲೆ = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    ಗಾತ್ರ_t m_ಎಣಿಕೆ = 0;

ಸಾರ್ವಜನಿಕ:
    explicit V3DebugBisect(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    ಮಾದರಿ <typename Callable>
    bool stop(Callable&& f) {
        ಆದರೆ (VL_LIKELY(!m_ಎಲ್ಲೆ)) ಹಿಂತಿರುಗಿ false;
        ++m_ಎಣಿಕೆ;
        ಆದರೆ (VL_UNLIKELY(m_ಎಣಿಕೆ == m_ಎಲ್ಲೆ)) f();
        ಹಿಂತಿರುಗಿ m_ಎಣಿಕೆ > m_ಎಲ್ಲೆ;
    }

    // Returns true if the limit has been reached
    bool isStopped() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಣಿಕೆ > m_ಎಲ್ಲೆ; }
};
