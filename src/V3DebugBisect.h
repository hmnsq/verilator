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

#अगर_अघोषित VL_MT_DISABLED_CODE_UNIT
#त्रुटि "V3DebugBisect.h uses global state and is not thread-safe"
#पूर्ण_अगर

#समावेश "V3Os.h"

#समावेश <string>

// Please see the inernals documentation for using this utility class with 'verilator_bisect'
class V3DebugBisect final {
    // Name of instance
    स्थिर अक्षर* स्थिर m_namep;
    // Limit for stopping - 0 means no limit
    स्थिर माप_प्रकार m_limit = std::stoull(V3Os::getenvStr("VERILATOR_DEBUG_BISECT_"s + m_namep, "0"));
    // Calls so far
    माप_प्रकार m_गिनती = 0;

खुला:
    explicit V3DebugBisect(स्थिर अक्षर* namep)
        : m_namep{namep} {}

    // Returns 'false' up to m_limit invocations, then returns 'true'.
    // Calls 'f' on the last invocation that returns 'false', which can be used for reporting.
    ढाँचा <typename Callable>
    bool stop(Callable&& f) {
        अगर (VL_LIKELY(!m_limit)) वापस false;
        ++m_गिनती;
        अगर (VL_UNLIKELY(m_गिनती == m_limit)) f();
        वापस m_गिनती > m_limit;
    }

    // Returns true if the limit has been reached
    bool isStopped() स्थिर { वापस m_गिनती > m_limit; }
};
