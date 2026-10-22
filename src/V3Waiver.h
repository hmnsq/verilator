// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Waivers
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3WAIVER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3WAIVER_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class V3Waiver final {
    // TYPES
    using WaiverList = std::ਸਦਿਸ਼<std::string>;
    ਸਥਿਰ V3Mutex s_mutex;  // Protect members
    ਸਥਿਰ WaiverList s_waiverList VL_GUARDED_BY(s_mutex);

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_ਇੰਦਰਾਜ(V3_Error_ਕੋਡ error_ਕੋਡ, ਅਚਲ string& filename, ਅਚਲ std::string& ਸੁਨੇਹਾ_2)
        VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s_mutex);
    ਸਥਿਰ ਖਾਲੀ ਲਿਖ(ਅਚਲ std::string& filename) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s_mutex);
};

#ਜੇ_ਅੰਤ  // Guard
